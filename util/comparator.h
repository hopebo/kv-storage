// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef COMPARATOR_H_
#define COMPARATOR_H_

#include "utils.h"
#include "../type/byte_array.h"

class Comparator
{
public:
	~Comparator() { }
	Comparator() { }

	int operator()(const char* a, const char* b) const
	{
		// TODO: Use a new data structure to avoid repetitive decoding process in comparison.
		ByteArray akey(ExtractUserKey(a));
		ByteArray bkey(ExtractUserKey(b));
		std::string astr_key(akey.Data(), akey.Size());
		std::string bstr_key(bkey.Data(), bkey.Size());
		return astr_key < bstr_key ? -1 : (astr_key == bstr_key ? 0 : 1);
	}
};

#endif  // COMPARATOR_H_
