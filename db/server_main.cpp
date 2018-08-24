// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/ip.h>
#include <sys/epoll.h>
#include <errno.h>

#include "data_base.h"
#include "../util/file_logger.h"
#include "../structure/task.h"
#include "../structure/thread_pool.h"

#define MAX_PENDING 50
#define MAX_EVENTS 100
#define THREAD_NUM 16
#define CACHE_NUM 100
#define LEVEL0_FILE_NUM_LIMIT 4

class NetworkTask : public Task
{
private:
	DataBase* data_base_;
	Logger* log_;
	int socket_fd_;

public:
	NetworkTask(DataBase* data_base, Logger* log,  int socket_fd) : data_base_(data_base), log_(log), socket_fd_(socket_fd) { }
	void RunInLock(std::thread::id tid) override { }
	void Run(std::thread::id tid) override
	{
		char buffer_recv[1400];
		int message_size = 0;
		while ((message_size = recv(socket_fd_, buffer_recv, 1400, 0)) > 0)
		{
			OrderType order_type;
			char* p = buffer_recv;
			if (message_size > 3 && memcmp(buffer_recv, "Get", 3) == 0)
			{
				order_type = Get;
				p += 3;
			}
			else if (message_size > 3 && memcmp(buffer_recv, "Put", 3) == 0)
			{
				order_type = Put;
				p += 3;
			}
			else if (message_size > 6 && memcmp(buffer_recv, "Delete", 6) == 0)
			{
				order_type = Delete;
				p += 6;
			}
			else
			{
				log_->Error("%s parse order type failed", __FUNCTION__);
				exit(1);
			}

			uint32_t key_size = 0;
			int length = 0;
			if ((length = GetVarint32(p, 5, &key_size)) < 0)
			{
				log_->Error("%s parse key size failed", __FUNCTION__);
				exit(1);
			}

			p += length;
			if ((message_size - (p - buffer_recv)) < key_size)
			{
				log_->Error("%s packet not complete for key", __FUNCTION__);
				exit(1);
			}

			std::string key(p, key_size);
			
			p += key_size;
			std::string return_msg;
			if (order_type == Get)
			{
				std::string value_out;
				data_base_->Get(key, value_out);
				return_msg.append(value_out);
			}
			else
			{
				std::string value;
				if (order_type == Put)
				{
					uint32_t value_size = 0;
					if ((length = GetVarint32(p, 5, &value_size)) < 0)
					{
						log_->Trace("%s parse value size failed", __FUNCTION__);
						exit(1);
					}

					p += length;
					if ((message_size - (p - buffer_recv)) < value_size)
					{
						log_->Trace("%s packet not complete for value", __FUNCTION__);
						exit(1);
					}

					value = std::string(p, value_size);
				}
				else if (order_type == Delete)
				{
					value = Constant::TombValue;
				}

				data_base_->Add(Put, key, value);
				return_msg.append("OK");
			}

			if (send(socket_fd_, return_msg.c_str(), return_msg.size(), 0) != return_msg.size())
			{
				log_->Error("%s send return packet failed", __FUNCTION__);
			}
		}
		
		close(socket_fd_);
	}
};

int setnonblocking(int fd)
{
	int old_flags = fcntl(fd, F_GETFL);
	int new_flags = old_flags | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_flags);
	return old_flags;
}

int main()
{
	EventManager event_manager;
    FileLogger file_logger("./log.txt", LogLevelTrace, true, true);
    StorageBuffer storage_buffer(4 << 20, &file_logger, &event_manager);
    LRUCache cache(CACHE_NUM);
    StorageEngine storage_engine(&file_logger, LEVEL0_FILE_NUM_LIMIT, &event_manager, &storage_buffer);
    DataBase data_base(&event_manager, &storage_buffer, &storage_engine, &file_logger, &cache);

    data_base.Start();

	ThreadPool thread_pool(THREAD_NUM);
	thread_pool.Start();

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(7777);

	if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0)
	{
		printf("Binding Socket Failed");
		exit(1);
	}

	if (listen(server_socket, MAX_PENDING) < 0)
	{
		printf("Listening for Connections Failed");
		exit(1);
	}

	struct epoll_event ev, events[MAX_EVENTS];
	int epoll_fd = epoll_create1(0);
	if (epoll_fd == -1)
	{
		printf("epoll createl");
		exit(1);
	}

	ev.events = EPOLLIN;
	ev.data.fd = server_socket;
	if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &ev) == -1)
	{
		printf("epoll_ctl: server_socket");
		exit(1);
	}

	struct sockaddr_in client_addr;
	unsigned int addr_len = sizeof(client_addr);
	int client_socket;

	for (;;)
	{
		int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
		if (nfds == -1)
		{
			printf("epoll wait");
			exit(1);
		}

		for (int i = 0; i < nfds; ++i)
		{
			if (events[i].data.fd == server_socket)
			{
				client_socket = accept(server_socket, (struct sockaddr*)& client_addr, &addr_len);
				if (client_socket == -1)
				{
					printf("accept, Error Code: %s\n", strerror(errno));
					exit(1);
				}
				
				setnonblocking(client_socket);
				ev.events = EPOLLIN | EPOLLET;
				ev.data.fd = client_socket;
				if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &ev) == -1)
				{
					printf("epoll_ctl: client_socket");
					exit(1);
				}
			}
			else
			{
				thread_pool.AddTask(new NetworkTask(&data_base, &file_logger, events[i].data.fd));
				epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, 0);
			}
		}
	}
}
