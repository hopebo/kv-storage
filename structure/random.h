// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

// The code below was copied from LevelDB. A few changes were applied to make it
// self-sufficient.

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef RANDOM_H_
#define RANDOM_H_

#include <stdint.h>

class Random
{
private:
	uint32_t seed_;

public:
	explicit Random(uint32_t s) : seed_(s & 0x7fffffffu)
	{
		if (seed_ == 0 || seed_ == 2147483647L)
		{
			seed_ = 1;
		}
	}

	uint32_t Next()
	{
		static const uint32_t M = 2147483647L;
		static const uint64_t A = 16807;

		uint64_t product = seed_ * A;

		seed_ = static_cast<uint32_t>((product >> 31) + (product & M));

		if (seed_ > M)
		{
			seed_ -= M;
		}

		return seed_;
	}
};

#endif  // RANDOM_H_
