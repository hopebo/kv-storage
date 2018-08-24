// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef BYTE_ARRAY_H_
#define BYTE_ARRAY_H_

#include <stdint.h>

class ByteArray
{
private:
	const char* data_;
	uint32_t size_;

public:
	ByteArray(const char* data, uint32_t size)
		: data_(data), size_(size) { }
	~ByteArray() { }
	const char* Data() const { return data_; }
	uint32_t Size() const { return size_; }
};

#endif  // BYTE_ARRAY_H_
