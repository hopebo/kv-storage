// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

// The code below was copied from LevelDB. A few changes were applied to make it
// self-sufficient.

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef MEMORY_H_
#define MEMORY_H_

#include <vector>

#include <stdint.h>

class Memory
{
private:
	const int kBlockSize = 4096;

	char* alloc_ptr_;
	int alloc_bytes_remaining_;

	std::vector<char*> blocks_;
	int blocks_memory_;

	Memory(const Memory&) = delete;
	void operator=(const Memory&) = delete;

	char* AllocateFallback(int bytes);
	char* AllocateNewBlock(int block_bytes);

public:
	Memory();
	~Memory();

	char* Allocate(int bytes);
	char* AllocateAligned(int bytes);
	
	int MemoryUsage() const
	{
		return blocks_memory_ + blocks_.capacity() * sizeof(char*);
	}
};

#endif  // MEMORY_H_
