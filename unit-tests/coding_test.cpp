// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../util/coding.h"
#include "../structure/test_harness.h"

class Coding { };

TEST(Coding, EncodingVarint)
{
	uint32_t value32 = 874324239;
	char* dst = new char[5];
	EncodeVarint32(dst, value32);
	uint32_t ret32;
	GetVarint32(dst, 5, &ret32);
	delete[] dst;
	ASSERT_EQ(value32, ret32);

	uint64_t value64 = 4294967295839472;
	dst = new char[20];
	EncodeVarint64(dst, value64);
	uint64_t ret64;
	GetVarint64(dst, 20, &ret64);
	delete[] dst;
	ASSERT_EQ(value64, ret64);
}

int main(int argc, char** argv)
{
	return RunAllTests();
}
