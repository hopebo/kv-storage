// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

// The code below was copied from LevelDB. A few changes were applied to make it
// self-sufficient.

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "coding.h"
#include "endian.h"

char* EncodeVarint32(char* dst, uint32_t v)
{
	unsigned char* ptr = reinterpret_cast<unsigned char*>(dst);
	static const int B = 128;
	if (v < (1<<7))
	{
		*(ptr++) = v;
	}
	else if (v < (1<<14))
	{
		*(ptr++) = v | B;
		*(ptr++) = v>>7;
	}
	else if (v < (1<<21))
	{
		*(ptr++) = v | B;
		*(ptr++) = (v>>7) | B;
		*(ptr++) = v>>14;
	}
	else if (v < (1<<28))
	{
		*(ptr++) = v | B;
		*(ptr++) = (v>>7) | B;
		*(ptr++) = (v>>14) | B;
		*(ptr++) = v>>21;
	}
	else
	{
		*(ptr++) = v | B;
		*(ptr++) = (v>>7) | B;
		*(ptr++) = (v>>14) | B;
		*(ptr++) = (v>>21) | B;
		*(ptr++) = v>>28;
	}

	return reinterpret_cast<char*>(ptr);
}

char* EncodeVarint64(char* dst, uint64_t v)
{
	static const int B = 128;
	unsigned char* ptr = reinterpret_cast<unsigned char*>(dst);
	while (v >= B)
	{
		*(ptr++) = (v & (B - 1)) | B;
		v >>= 7;
	}

	*(ptr++) = static_cast<unsigned char>(v);
	return reinterpret_cast<char*>(ptr);
}

void EncodeFixed32(char* buf, uint32_t value) 
{
	if (kLittleEndian) 
	{
		memcpy(buf, &value, sizeof(value));
	} 
	else 
	{
		buf[0] = value & 0xff;
		buf[1] = (value >> 8) & 0xff;
		buf[2] = (value >> 16) & 0xff;
		buf[3] = (value >> 24) & 0xff;
	}
}

void EncodeFixed64(char* buf, uint64_t value)
{
	if (kLittleEndian)
	{
		memcpy(buf, &value, sizeof(value));
	}
	else
	{
		buf[0] = value & 0xff;
    	buf[1] = (value >> 8) & 0xff;
    	buf[2] = (value >> 16) & 0xff;
    	buf[3] = (value >> 24) & 0xff;
    	buf[4] = (value >> 32) & 0xff;
    	buf[5] = (value >> 40) & 0xff;
    	buf[6] = (value >> 48) & 0xff;
    	buf[7] = (value >> 56) & 0xff;	
	}
}

void GetFixed32(const char* buf, uint32_t* value) 
{
	if (kLittleEndian) 
	{
		memcpy(value, buf, sizeof(*value));
	} 
	else 
	{
		*value =  (uint32_t)buf[0]
				| (uint32_t)buf[1] << 8
				| (uint32_t)buf[2] << 16
				| (uint32_t)buf[3] << 24;
	}
}

void GetFixed64(const char* buf, uint64_t* value)
{
	if (kLittleEndian) 
	{
    	memcpy(value, buf, sizeof(*value));
	} 
	else 
	{
    	*value =  (uint64_t)buf[0]
             	| (uint64_t)buf[1] <<  8
             	| (uint64_t)buf[2] << 16
             	| (uint64_t)buf[3] << 24
             	| (uint64_t)buf[4] << 32
             	| (uint64_t)buf[5] << 40
             	| (uint64_t)buf[6] << 48
             	| (uint64_t)buf[7] << 56;
	}
}

int GetVarint32(const char* data, uint64_t size, uint32_t* value)
{
	const char* p = data;
	const char* limit = p + size;
	const char* q = GetVarint32Ptr(p, limit, value);
	if (q == NULL)
	{
		return -1;
	}
	else
	{
		return q - p;
	}
}

int GetVarint64(const char* data, uint64_t size, uint64_t* value)
{
	const char* p = data;
	const char* limit = p + size;
	const char* q = GetVarint64Ptr(p, limit, value);
	if (q == NULL)
	{
		return -1;
	}
	else
	{
		return q - p;
	}
}

const char* GetVarint32Ptr(const char* p, const char* limit, uint32_t* value)
{
	uint32_t result = 0;
	for (uint32_t shift = 0; shift <= 28 && p < limit; shift += 7)
	{
		uint32_t byte = *(reinterpret_cast<const unsigned char*>(p));
		p++;
		if (byte & 128)
		{
			result |= ((byte & 127) << shift);
		}
		else
		{
			result |= (byte << shift);
			*value = result;
			return reinterpret_cast<const char*>(p);
		}
	}

	return NULL;
}
		
const char* GetVarint64Ptr(const char* p, const char* limit, uint64_t* value)
{
	uint64_t result = 0;
	for (uint32_t shift = 0; shift <= 63 && p < limit; shift += 7)
	{
		uint64_t byte = *(reinterpret_cast<const unsigned char*>(p));
		p++;
		if (byte & 128)
		{
			result |= ((byte & 127) << shift);
		}
		else
		{
			result |= (byte << shift);
			*value = result;
			return reinterpret_cast<const char*>(p);
		}
	}
	
	return NULL;
}

int VarintLength(uint64_t v)
{
	int len = 1;
	while (v >= 128)
	{
		v >>= 7;
		++len;
	}

	return len;
}
