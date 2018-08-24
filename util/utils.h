// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef UTILS_H_
#define UTILS_H_

#include <assert.h>
#include <string>
#include <stdio.h>

#include "coding.h"
#include "../type/byte_array.h"

inline ByteArray ExtractUserKey(const ByteArray& entry)
{
	const char* p = entry.Data();
	uint32_t key_size;
	int length = GetVarint32(p, 5, &key_size);
	p += length;
	return ByteArray(p, key_size);
}

inline ByteArray ExtractUserKey(const char* entry)
{
	const char* p = entry;
	uint32_t key_size;
	int length = GetVarint32(p, 5, &key_size);
	p += length;
	return ByteArray(p, key_size);
}

inline uint32_t EntrySize(const char* entry)
{
	uint32_t total_len = 0, size = 0;
	int length = GetVarint32(entry, 5, &size);
	total_len += length + size;
	entry += length + size;
	length = GetVarint32(entry, 5, &size);
	return total_len + length + size;
}

inline ByteArray ExtractUserValue(const char* entry)
{
	const char* p = entry;
	uint32_t size;
	int length = GetVarint32(p, 5, &size);
	p += length + size;
	length = GetVarint32(p, 5, &size);
	return ByteArray(p + length, size);
}

inline ByteArray ExtractUserValue(const ByteArray& entry)
{
	const char* p = entry.Data();
	uint32_t size;
	int length = GetVarint32(p, 5, &size);
	p += length + size;
	length = GetVarint32(p, 5, &size);
	return ByteArray(p + length, size);
}

inline ByteArray WrapUserKey(const ByteArray& key)
{
	uint32_t encoded_len = VarintLength(key.Size()) + key.Size();
	char* buf = new char[encoded_len];
	char* p = buf;
	p = EncodeVarint32(p, key.Size());
	memcpy(p, key.Data(), key.Size());
	return ByteArray(buf, encoded_len);
}

inline int Compare(const ByteArray& akey, const ByteArray& bkey)
{
	std::string akey_str(akey.Data(), akey.Size());
	std::string bkey_str(bkey.Data(), bkey.Size());
	return akey_str < bkey_str ? -1 
		: (akey_str == bkey_str ? 0 : 1);
}

inline std::string FileName(int level_id, int file_id)                                                            
{   
    char* cfile_id = new char[9];
    sprintf(cfile_id, "%08d", file_id);                                           
    cfile_id[8] = '\0';
    std::string file_name = std::to_string(level_id);                            
    file_name.push_back('_');
    file_name.append(std::string(cfile_id));                                       
    delete[] cfile_id;                                                             
    return file_name;                                                             
}       

#endif  // UTILS_H_
