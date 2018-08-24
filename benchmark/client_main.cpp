// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#include <unordered_map>
#include <string>

#include <sys/socket.h>                                                              
#include <sys/types.h>
#include <netinet/ip.h>                                                              
#include <arpa/inet.h>
#include <sys/epoll.h> 
#include <sys/time.h>
#include <assert.h>
#include <unistd.h>
#include <algorithm>
#include <errno.h>

#include "../type/order_type.h"
#include "../util/coding.h"
#include "../util/sequence_generator.h"
#include "../structure/thread_pool.h"
#include "../structure/concurrent_queue.h"

#define TEST_NUM 500000
#define THREAD_NUM 16

long long TimeInterval(struct timeval start, struct timeval end)
{
    return ((long long)end.tv_sec - start.tv_sec) * 1e6 + end.tv_usec - start.tv_usec;
}
	
class ClientTask : public Task
{
private:
	ConcurrentQueue<long long>* timedelay_queue_;
	OrderType order_type_;
	std::string key_;
	std::string value_;	

public:
	ClientTask(ConcurrentQueue<long long>* timedelay_queue, OrderType order_type, std::string key, std::string value = "")
		: timedelay_queue_(timedelay_queue),
		  order_type_(order_type),
		  key_(key),
		  value_(value)
	{
	}

	void RunInLock(std::thread::id tid) override { }
	void Run(std::thread::id tid) override
	{
		int encoded_len = 0;
		int order_len = strlen(OrderTypeString[order_type_]);
		encoded_len += order_len;

		encoded_len += VarintLength(key_.size()) + key_.size();
		if (order_type_ == Put)
		{
			encoded_len += VarintLength(value_.size()) + value_.size();
		}

		char* buf = new char[encoded_len];
		char* p = buf;
		memcpy(p, OrderTypeString[order_type_], order_len);
		p += order_len;
		
		p = EncodeVarint32(p, key_.size());
		memcpy(p, key_.c_str(), key_.size());
		p += key_.size();
		if (order_type_ == Put)
		{
			p = EncodeVarint32(p, value_.size());
			memcpy(p, value_.c_str(), value_.size());
			p += value_.size();
		}

		assert(p - buf == encoded_len);

		int client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    	if (client_socket < 0)
    	{
        	printf("client socket initialization failed\n");
			exit(1);
    	}
    
		struct sockaddr_in server_addr; 
		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
		server_addr.sin_port = htons(7777);

		struct timeval start, end;
		gettimeofday(&start, NULL);
		
		if (connect(client_socket, (struct sockaddr*)& server_addr, sizeof(server_addr)) < 0)
		{
			printf("connect server failed\n");
			exit(1);
		}
		
		if (send(client_socket, buf, encoded_len, 0) != encoded_len)
		{
			printf("send incomplete message\n");
		}

		char recv_buffer[1005];
		int recv_bytes = recv(client_socket, recv_buffer, 1005, 0);
		
		gettimeofday(&end, NULL);
		timedelay_queue_->push(TimeInterval(start, end));

		if (recv_bytes < 0)
		{
			printf("Error Code: %s\n", strerror(errno));
			printf("recv bytes error");
		}

		std::string message_(recv_buffer, recv_bytes);
		if (order_type_ == Get)
		{
			assert(message_ == value_);
		}

		// printf("%s\n", message_.c_str());
		delete[] buf;
		close(client_socket);
	}
};

enum SequenceType { Sequence = 0, Random = 1 };
std::string SequenceTypeString[2] = { "Sequence", "Random" };

struct BenchmarkReport
{
	SequenceType sequence_type_;
	OrderType order_type_;
	int qps_;
	double time_interval_;	// unit: sec
	double delay_;	// unit: usec
	BenchmarkReport(SequenceType sequence_type, OrderType order_type, int qps, double time_interval, double delay) 
		: sequence_type_(sequence_type), 
		  order_type_(order_type), 
		  qps_(qps), 
		  time_interval_(time_interval), 
		  delay_(delay) 
	{
	}
};

int main()
{
	int key_len = 25;
	int value_len_array[2] = { 100, 1000 };

	ThreadPool thread_pool(THREAD_NUM);		
	thread_pool.Start();

	ConcurrentQueue<long long> timedelay_queue;

	FILE* fd = fopen("server_performance.txt", "w");

	for (int i = 0; i < 2; ++i)
	{
		printf("Key Length: %d, Value Length: %d\n", key_len, value_len_array[i]);

		std::vector<BenchmarkReport> reports;
		
		std::vector<long long> time_intervals;

		auto kv_pairs = RandomKvPairs(TEST_NUM, key_len, value_len_array[i]); 
		std::sort(kv_pairs.begin(), kv_pairs.end());

		std::vector<int> sequence_index;
		std::vector<int> random_index;
		for (int i = 0; i < TEST_NUM; ++i)
		{
			sequence_index.push_back(i);
			random_index.push_back(rand() % TEST_NUM);
		}

		SequenceType sequence_types[2] = { Random, Sequence };
		OrderType order_types[2] = { Put, Get };
		
		for (int j = 0; j < 2; ++j)
		{
			for (int k = 0; k < 2; ++k)
			{
				auto order_type = order_types[j];
				auto sequence_type = sequence_types[k];
				printf("Starting %s %s Test...\n", SequenceTypeString[sequence_type].c_str(), OrderTypeString[order_type]);

				std::vector<int> indexes;
				if (sequence_type == Sequence)
				{
					indexes.assign(sequence_index.begin(), sequence_index.end());
				}
				else
				{
					indexes.assign(random_index.begin(), random_index.end());
				}

				struct timeval start, end;
				gettimeofday(&start, NULL);

				for (auto& index : indexes)
				{
					thread_pool.AddTask(new ClientTask(&timedelay_queue, order_type, kv_pairs[index].first, kv_pairs[index].second));
				}

				thread_pool.BlockUntilAllTaskHaveCompleted();
				
				gettimeofday(&end, NULL);
				auto time_interval = TimeInterval(start, end);
				
				assert(timedelay_queue.size() == TEST_NUM);
				
				long long delay_sum = 0;
				while (!timedelay_queue.empty())
				{
					auto delay = timedelay_queue.pop();
					delay_sum += delay;
				}

				reports.push_back(BenchmarkReport(sequence_type, order_type, TEST_NUM / (time_interval * 1e-6), time_interval * 1e-6, (double)delay_sum / TEST_NUM));
				printf("Ending %s %s Test...\n", SequenceTypeString[sequence_type].c_str(), OrderTypeString[order_type]);
			}
		}

		fprintf(fd, "Key Length: %d, Value Length: %d, Test Num: %d\n", key_len, value_len_array[i], TEST_NUM);
		for (auto& report : reports)
		{
			fprintf(fd, "Sequence Type: %s, Order Type: %s, Cost Time: %f, QPS: %d, Delay: %f\n", SequenceTypeString[report.sequence_type_].c_str(), OrderTypeString[report.order_type_], report.time_interval_, report.qps_, report.delay_);
		}
	}

	fclose(fd);
	thread_pool.Stop();
	return 0;
}
