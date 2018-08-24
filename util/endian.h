// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

// The code below was copied from LevelDB. A few changes were applied to make it
// self-sufficient.

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef ENDIAN_H_
#define ENDIAN_H_

#include <stdint.h>

enum endian_t : uint32_t {
    kBytesLittleEndian     = 0x00000001, // byte-swapped little-endian
    kBytesBigEndian        = 0x01000000, // byte-swapped big-endian
    kBytesLittleEndianWord = 0x00010000, // word-swapped little-endian
    kBytesBigEndianWord    = 0x00000100, // word-swapped big-endian
    kBytesUnknownEndian    = 0xffffffff
};

endian_t getEndianness();

extern const bool kLittleEndian;
extern const bool kBigEndian;

#endif  // ENDIAN_H_
