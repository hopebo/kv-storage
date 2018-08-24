// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

// The code below was copied from LevelDB. A few changes were applied to make it
// self-sufficient.

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef CODING_H_
#define CODING_H_

#include <stdint.h>
#include <string.h>

extern char* EncodeVarint32(char* dst, uint32_t value);

extern char* EncodeVarint64(char* dst, uint64_t value);

extern void EncodeFixed32(char* dst, uint32_t value);

extern void EncodeFixed64(char* dst, uint64_t value);

extern void GetFixed32(const char* src, uint32_t* value);

extern void GetFixed64(const char* src, uint64_t* value);

extern int GetVarint32(const char* input, uint64_t size, uint32_t* value);

extern int GetVarint64(const char* input, uint64_t size, uint64_t* value);

extern const char* GetVarint32Ptr(const char* p, const char* limit, uint32_t* v);

extern const char* GetVarint64Ptr(const char* p, const char* limit, uint64_t* v);

extern int VarintLength(uint64_t v);

#endif  // CODING_H_
