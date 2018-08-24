// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#include <string>

#include "../type/byte_array.h"
#include "../util/comparator.h"
#include "../structure/test_harness.h"
#include "../structure/skip_list.h"

typedef SkipList<ByteArray, Comparator> Table;

class SkipListTest { };

std::string strs[5] = 
{
	"My name is Bo Li",
	"I'm an intern in Tencent",
	"This is a part of KV work",
	"Can I finish on time?",
	"World Cup is ongoing"
};

void Insert(Table& skip_list)
{
	for (int i = 0; i < 5; i++)
	{
		skip_list.Insert(WrapUserKey(ByteArray(strs[i].c_str(), strs[i].size())));
	}
}

TEST(SkipListTest, Insert)
{
	Table skip_list;
	Insert(skip_list);

	for (int i = 0; i < 5; i++)
	{
		ASSERT_TRUE(skip_list.Contains(WrapUserKey(ByteArray(strs[i].c_str(), strs[i].size()))));
	}
}

TEST(SkipListTest, Iterate)
{
	Table skip_list;
	Insert(skip_list);

	int alphabetic_order[5] = { 3, 1, 0, 2, 4 };
	Table::Iterator it(&skip_list);
	int i = 0;
	for (it.SeekToFirst(); it.Valid(); ++i, it.Next())
	{
		ByteArray key(ExtractUserKey(it.key()));
		std::string key_str = std::string(key.Data(), key.Size());
		ASSERT_EQ(strs[alphabetic_order[i]], key_str);
	}
}

int main()
{
	return RunAllTests();
}
