// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#include "../structure/test_harness.h"
#include "../structure/cache.h"

class CacheTest { };

TEST(CacheTest, Get)
{
	LRUCache cache(5);
	std::string key_str = "hope";
	std::unordered_map<std::string, uint32_t> test_map;
	test_map[key_str] = 1;
	for (int i = 0; i < 6; i++)
	{
		if (i == 5)
		{
			cache.Get(0, key_str);
		}

		cache.Set(i, test_map);
	}

	auto offset = cache.Get(0, key_str);
	ASSERT_TRUE(offset != 0);
	offset = cache.Get(1, key_str);
	ASSERT_TRUE(offset == 0);
}

int main()
{
	return RunAllTests();
}

