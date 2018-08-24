// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#include <set>

#include <time.h>
#include <string.h>

#include "sequence_generator.h"

std::string RandomString(const int len)
{
	std::string str;
	for (int i = 0; i < len; ++i)
	{
		switch (rand() % 3)
		{
			case 0:
				str.push_back('A' + rand() % 26);
				break;
			case 1:
				str.push_back('a' + rand() % 26);
				break;
			default:
				str.push_back('0' + rand() % 10);
				break;
		}
	}

	return str;
}

std::vector<std::pair<std::string, std::string>> RandomKvPairs(int num, int key_len, int value_len, int random_seed)
{
	srand(random_seed);
	std::set<std::pair<std::string, std::string>> ret;
	for (int i = 0; i < num; ++i)
	{
		ret.insert(std::pair<std::string, std::string>(RandomString(key_len), RandomString(value_len)));
	}

	return std::vector<std::pair<std::string, std::string>>(ret.begin(), ret.end());
}
