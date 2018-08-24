// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#include <unistd.h>
#include <sys/time.h>

#include "cpu_monitor.h"
#include "../db/data_base.h"
#include "../util/file_logger.h"
#include "../util/sequence_generator.h"
#include "../structure/thread_pool.h"
#include "../structure/concurrent_queue.h"
#include "../unit-tests/db_operation_task.h"

#define TEST_NUM 500000
#define CACHE_NUM 500
#define THREAD_NUM 1
#define LEVEL0_FILE_NUM 4
#define BUFFER_SIZE 2 << 20

struct PerfReport
{
	unsigned int SequentialReads;
	unsigned int RandomReads;
	unsigned int SequentialWrites;
	unsigned int RandomWrites;
};

double TimeInterval(struct timeval start, struct timeval end)
{
	return (double)end.tv_sec - start.tv_sec + ((double)end.tv_usec - start.tv_usec) * 1e-6;
}

int main()
{
	// Initial DataBase
    EventManager event_manager;
    FileLogger file_logger("./log.txt", LogLevelInfo, true, true);
    StorageBuffer storage_buffer(BUFFER_SIZE, &file_logger, &event_manager);
    LRUCache cache(CACHE_NUM);
    StorageEngine storage_engine(&file_logger, LEVEL0_FILE_NUM, &event_manager, &storage_buffer);

    DataBase data_base(&event_manager, &storage_buffer, &storage_engine, &file_logger, &cache);

    data_base.Start();

	// Initial Thread Pool
    ThreadPool thread_pool(THREAD_NUM);
    thread_pool.Start();    

	// Initial Concurrent Queue, holding the Get results to make sure results correct.
    ConcurrentQueue<std::pair<std::string, std::string>> result_queue;

	FILE* fd = fopen("db_performance.txt", "w");
	int value_len[2] = { 100, 1000 };
	for (int i = 0; i < 2; ++i)	
	{
		int key_len = 25;

		printf("TEST Key Size: %d, Value Size: %d\n", key_len, value_len[i]); 

		auto kv_pairs = RandomKvPairs(TEST_NUM, key_len, value_len[i]);
		std::sort(kv_pairs.begin(), kv_pairs.end());
		
		PerfReport report;
		struct timeval start, end;			
		
		std::vector<double> cost_time;
		std::vector<double> cpu_occupy;
		CpuMonitor cpu_monitor(getpid());

		// Sequential Writes
		printf("Starting Sequential Writes Test...\n");
		gettimeofday(&start, NULL);
		cpu_monitor.RecordStart();
		for (int i = 0; i < TEST_NUM; ++i)
		{
			thread_pool.AddTask(new DBOperationTask(&data_base, &result_queue, Put, kv_pairs[i].first, kv_pairs[i].second));
		}

		thread_pool.BlockUntilAllTaskHaveCompleted();
		gettimeofday(&end, NULL);
		cpu_monitor.RecordEnd();
		report.SequentialWrites = TEST_NUM / TimeInterval(start, end);
		cost_time.push_back(TimeInterval(start, end));
		cpu_occupy.push_back(cpu_monitor.CpuOccupyRatio());

		printf("Finishing Sequential Writes Test...\n");
		
		sleep(2);

		// Random Writes
		printf("Starting Random Writes Test...\n");
		gettimeofday(&start, NULL);
		cpu_monitor.RecordStart();
		for (int i = 0; i < TEST_NUM; ++i)
		{
			int index = rand() % TEST_NUM;
			thread_pool.AddTask(new DBOperationTask(&data_base, &result_queue, Put, kv_pairs[index].first, kv_pairs[index].second));
		}

		thread_pool.BlockUntilAllTaskHaveCompleted();
		gettimeofday(&end, NULL);
		cpu_monitor.RecordEnd();
		report.RandomWrites = TEST_NUM / TimeInterval(start, end);
		cost_time.push_back(TimeInterval(start, end));
		cpu_occupy.push_back(cpu_monitor.CpuOccupyRatio());

		printf("Finishing Random Writes Test...\n");
		
		sleep(2);

		// Sequential Reads
		printf("Starting Sequential Reads Test...\n");
		gettimeofday(&start, NULL);
		cpu_monitor.RecordStart();
		for (int i = 0; i < TEST_NUM; ++i)
		{
			thread_pool.AddTask(new DBOperationTask(&data_base, &result_queue, Get, kv_pairs[i].first, kv_pairs[i].second));
		}

		thread_pool.BlockUntilAllTaskHaveCompleted();
		gettimeofday(&end, NULL);
		cpu_monitor.RecordEnd();
		report.SequentialReads = TEST_NUM / TimeInterval(start, end);
		cost_time.push_back(TimeInterval(start, end));
		cpu_occupy.push_back(cpu_monitor.CpuOccupyRatio());

		printf("Finishing Sequential Reads Test...\n");
		
		sleep(2);
		
		// Random Reads
		printf("Starting Random Reads Test...\n");
		gettimeofday(&start, NULL);
		cpu_monitor.RecordStart();
		for (int i = 0; i < TEST_NUM; ++i)
		{
			int index = rand() % TEST_NUM;
			thread_pool.AddTask(new DBOperationTask(&data_base, &result_queue, Get, kv_pairs[index].first, kv_pairs[index].second));
		}

		thread_pool.BlockUntilAllTaskHaveCompleted();
		gettimeofday(&end, NULL);
		cpu_monitor.RecordEnd();
		report.RandomReads = TEST_NUM / TimeInterval(start, end);
		cost_time.push_back(TimeInterval(start, end));
		cpu_occupy.push_back(cpu_monitor.CpuOccupyRatio());

		printf("Finishing Random Reads Test...\n");

		int count = 0;
		int sum = result_queue.size();
		while (!result_queue.empty())
    	{
        	auto item = result_queue.pop();
			assert(item.first == item.second);
    	}

		fprintf(fd, "Key Length: %d, Value Length: %d, Test Num: %d\n", key_len, value_len[i], TEST_NUM);
		fprintf(fd, "SequentialWrites: %d ops/s, CostTime: %f s, CpuOccupy: %f\nRandomWrites: %d ops/s, CostTime: %f s, CpuOccupy: %f\nSequentialReads: %d ops/s, CostTime: %f s, CpuOccupy: %f\nRandomReads: %d ops/s, CostTime: %f s, CpuOccupy: %f\n", report.SequentialWrites, cost_time[0], cpu_occupy[0], report.RandomWrites, cost_time[1], cpu_occupy[1], report.SequentialReads, cost_time[2], cpu_occupy[2], report.RandomReads, cost_time[3], cpu_occupy[3]);
	}

	thread_pool.Stop();
	data_base.ShutDown();
	printf("Finishing DataBase Benchmark. Results Saved in \"db_performance.txt\"\n");
}
