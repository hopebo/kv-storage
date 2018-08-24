// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

// The code below was copied from LevelDB. A few changes were applied to make it
// self-sufficient.

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "endian.h"

endian_t getEndianness() {
  if ((0xffffffff & 1) == kBytesLittleEndian) {
    return kBytesLittleEndian;
  } else if ((0xffffffff & 1) == kBytesBigEndian) {
    return kBytesBigEndian;
  } else if ((0xffffffff & 1) == kBytesLittleEndianWord) {
    return kBytesLittleEndianWord;
  } else if ((0xffffffff & 1) == kBytesBigEndianWord) {
    return kBytesBigEndianWord;
  }
  return kBytesUnknownEndian;
}

const bool kLittleEndian = (getEndianness() == kBytesLittleEndian);
const bool kBigEndian = (getEndianness() == kBytesBigEndian);
