// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

// The code below was copied from LevelDB. A few changes were applied to make it
// self-sufficient.

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include <assert.h>

#include "memory.h"

char* Memory::Allocate(int bytes)
{
	assert(bytes > 0);
	if (bytes <= alloc_bytes_remaining_)
	{
		char* result = alloc_ptr_;
		alloc_ptr_ += bytes;
		alloc_bytes_remaining_ -= bytes;
		return result;
	}

	return AllocateFallback(bytes);
}

char* Memory::AllocateFallback(int bytes)
{
	if (bytes > kBlockSize / 4)
	{
		char* result = AllocateNewBlock(bytes);
		return result;
	}

	alloc_ptr_ = AllocateNewBlock(kBlockSize);
	char* result = alloc_ptr_;
	alloc_ptr_ += bytes;
	alloc_bytes_remaining_ = kBlockSize - bytes;
	return result;
}

char* Memory::AllocateNewBlock(int block_bytes)
{
	char* result = new char[block_bytes];
	blocks_memory_ += block_bytes;
	blocks_.push_back(result);
	return result;
}

char* Memory::AllocateAligned(int bytes) 
{
	const int align = sizeof(void*);
	assert((align & (align - 1)) == 0);
	int current_mod = reinterpret_cast<uintptr_t>(alloc_ptr_) & (align - 1);
	int slop = (current_mod == 0 ? 0 : align - current_mod);
	int needed = bytes + slop;
	char* result;
	if (needed <= alloc_bytes_remaining_)
	{
		result = alloc_ptr_ + slop;
		alloc_ptr_ += needed;
		alloc_bytes_remaining_ -= needed;
	}
	else
	{
		result = AllocateFallback(bytes);
	}

	assert((reinterpret_cast<uintptr_t>(result) & (align - 1)) == 0);
	return result;
}

Memory::Memory()
{
	blocks_memory_ = 0;
	alloc_ptr_ = nullptr;
	alloc_bytes_remaining_ = 0;
	blocks_.clear();
}

Memory::~Memory()
{
	for (int i = 0; i < blocks_.size(); ++i)
	{
		delete[] blocks_[i];
	}
}
