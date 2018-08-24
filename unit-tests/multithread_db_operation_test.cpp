// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#include "db_operation_task.h"
#include "../db/data_base.h"
#include "../util/file_logger.h"
#include "../util/sequence_generator.h"
#include "../structure/thread_pool.h"
#include "../structure/test_harness.h"
#include "../structure/concurrent_queue.h"

#define TEST_NUM 2000000

class MultithreadDBOperation { };

TEST(MultithreadDBOperation, WriteAndGet)
{
	EventManager event_manager;
    FileLogger file_logger("./log.txt", LogLevelTrace, true, true);
    StorageBuffer storage_buffer(2097152, &file_logger, &event_manager);
    LRUCache cache(5);
    StorageEngine storage_engine(&file_logger, 4, &event_manager, &storage_buffer);

    DataBase data_base(&event_manager, &storage_buffer, &storage_engine, &file_logger, &cache);

    data_base.Start();
	ConcurrentQueue<std::pair<std::string, std::string>> result_queue;

	ThreadPool thread_pool(3);
	thread_pool.Start();	

    srand(102000);
    std::unordered_map<std::string, std::string> kv;

    for (int i = 0; i < TEST_NUM; ++i)
    {
        std::string key = RandomString(500);
        std::string value(key.rbegin(), key.rend());
        kv[key] = value;
    }

	printf("Size: %d\n", kv.size());
	int count = 0;
    for (auto& item : kv)
    {
		count++;
        std::string key(item.first);
        std::string value(item.second);
        thread_pool.AddTask(new DBOperationTask(&data_base, &result_queue, Put, key, value));
    }

	printf("Put %d Entry\n", count);

	thread_pool.BlockUntilAllTaskHaveCompleted();
	thread_pool.Stop();

    data_base.ShutDown();
}

int main()
{
	return RunAllTests();
}
