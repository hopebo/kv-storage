// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#include <vector>
#include <utility>
#include <unordered_map>

#include <time.h>

#include "../db/data_base.h"
#include "../util/file_logger.h"
#include "../util/sequence_generator.h"
#include "../structure/test_harness.h"

class DataBaseTest { };

TEST(DataBaseTest, PutAndGet)
{
	EventManager event_manager;
	FileLogger file_logger("./log.txt", LogLevelTrace, true, true);
	StorageBuffer storage_buffer(10240, &file_logger, &event_manager);
	LRUCache cache(2);
	StorageEngine storage_engine(&file_logger, 2, &event_manager, &storage_buffer);

	DataBase data_base(&event_manager, &storage_buffer, &storage_engine, &file_logger, &cache);

	data_base.Start();

	srand(102000);
	std::unordered_map<std::string, std::string> kv;

	for (int i = 0; i < 1000; ++i)
	{
		std::string key = RandomString(50);
		std::string value(key.rbegin(), key.rend());
		kv[key] = value;
	}

	for (auto& item : kv)
	{
		std::string key(item.first);
		std::string value(item.second);
		data_base.Add(Put, key, value);
	}

	std::vector<std::pair<std::string, std::string>> kv_vector;
	for (auto& item : kv)
	{
		std::string key(item.first);
		std::string value(item.second);
		kv_vector.push_back(std::pair<std::string, std::string>(key, value));
	}

	for (int i = 0; i < 1000; ++i)
	{
		int index = rand() % 1000;
		std::string value_out;
		int status = data_base.Get(kv_vector[index].first, value_out);
		ASSERT_EQ(status, 0);
		ASSERT_EQ(kv_vector[index].second, value_out);
	}

	data_base.ShutDown();
}

int main()
{
	return RunAllTests();
}
