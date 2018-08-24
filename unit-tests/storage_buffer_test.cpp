// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#include <unordered_map>

#include <time.h>

#include "../db/event_manager.h"
#include "../db/storage_buffer.h"
#include "../util/sequence_generator.h"
#include "../util/file_logger.h"
#include "../structure/test_harness.h"

class StorageBufferTest { };

TEST(StorageBufferTest, GetFromBufferAndFlush)
{
	srand(987654321);
	std::unordered_map<std::string, std::string> kv;
	for (int i = 0; i < 100; ++i)
	{
		std::string key = RandomString(25);
		std::string value(key.rbegin(), key.rend());
		kv[key] = value;
	}

	FileLogger logger("./log.txt", LogLevelTrace, false, true);
	EventManager event_manager;
	StorageBuffer buffer(1024 * 1024, &logger, &event_manager);
		
	for (auto& item : kv)
	{
		buffer.Add(Put, ByteArray(item.first.c_str(), item.first.size()),
			ByteArray(item.second.c_str(), item.second.size()));
	}

	for (auto& item : kv)
	{
		std::string key(item.first);
		std::string value_out;
		int status = buffer.Get(key, value_out);
		ASSERT_EQ(status, 0);
		ASSERT_EQ(item.second, value_out);
	}
		
	std::unordered_map<std::string, uint32_t> key_offset;
	FILE* file_stream = fopen("./data/0_00000000", "w");

	buffer.SwapBuffer();
	buffer.Flush(file_stream, key_offset);
	ASSERT_EQ(key_offset.size(), kv.size());
}

int main()
{
	return RunAllTests();
}
