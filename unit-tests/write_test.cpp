// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#include <time.h>

#include "../db/data_base.h"
#include "../util/file_logger.h"
#include "../structure/test_harness.h"
#include "../util/sequence_generator.h"

#define BufferSize 10240
#define SequenceLength 20
#define FlushTimes 5

class WriteTest { };

TEST(WriteTest, WriteBufferAndFlush)
{
	// instantiation
	EventManager event_manager;
	FileLogger file_logger("./log.txt", LogLevelTrace, false, true);
	FilePool file_pool;
	StorageBuffer storage_buffer(BufferSize, &file_logger, &event_manager);
	StorageEngine storage_engine(&file_logger, &file_pool);
	DataBase data_base(&event_manager, &storage_buffer, &storage_engine, &file_pool, &file_logger);

	data_base.Start();

	srand(987654321);
	std::map<std::string, std::string> kv_pairs;
	for (int i = 0; i < BufferSize * FlushTimes / (2 * SequenceLength); ++i)
	{
		std::string key(RandomString(SequenceLength));
		std::string value(key.rbegin(), key.rend());
		kv_pairs[key] = value;
	}

	for (auto item : kv_pairs)
	{
		char* key = new char[SequenceLength + 1];
		char* value = new char[SequenceLength + 1];
		memcpy(key, item.first.c_str(), SequenceLength);
		memcpy(value, item.second.c_str(), SequenceLength);
		key[SequenceLength] = '\0';
		value[SequenceLength] = '\0';
		data_base.Add(Put, key, SequenceLength, value, SequenceLength);
	}

	printf("%d\n", kv_pairs.size());
	int i = 0;
	for (auto item : kv_pairs)
	{
		i++;
		std::string value_out;
		std::string key(item.first);
		std::string value(item.second);
		data_base.Get(key, value_out);
		ASSERT_EQ(value, value_out);
		if (value != value_out)
		{
			printf("failed");
		}
	}
	printf("%d\n", i);
	getchar();
}

int main()
{
	return RunAllTests();
}
