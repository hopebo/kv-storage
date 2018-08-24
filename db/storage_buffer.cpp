// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#include <set>

#include <unistd.h>
#include <errno.h>

#include "storage_buffer.h"
#include "../util/coding.h"

void StorageBuffer::Add(OrderType order_type, const ByteArray& key, const ByteArray& value)
{	
	uint32_t key_size = key.Size();
	uint32_t value_size = value.Size();
	const uint32_t encoded_len = 
		VarintLength(key_size) + key_size +
		VarintLength(value_size) + value_size;

	std::unique_lock<std::mutex> lock(mutex_);

	char* buf = income_memory_->Allocate(encoded_len);

	char* p = EncodeVarint32(buf, key_size);
	memcpy(p, key.Data(), key_size);
	p += key_size;
	p = EncodeVarint32(p, value_size);
	memcpy(p, value.Data(), value_size);

	assert((p + value_size) - buf == encoded_len);

	income_buffer_->Insert(buf);
	income_size_ += encoded_len;

	if (income_size_ > buffer_size_ && flush_thread_ready_)
	{
		// TODO: What if the speed of flushing is not comparable to the speed of writing, 
		// which may cause income_buffer_ continuing growing.

		SetFlushThreadBusy();

		SwapBuffer();

		event_manager_->event_flush_buffer_.Notify();

		log_->Info("Flush Notify");
	}
}

void StorageBuffer::SwapBuffer()
{
	income_size_ = 0;

	flush_memory_ = income_memory_;
	income_memory_ = new Memory();

	flush_buffer_ = income_buffer_;
	income_buffer_ = new SkipList<const char*, Comparator>(cmp_, income_memory_);

	flush_buffer_ready_ = true;
}

void StorageBuffer::Flush(FILE* stream, std::vector<ByteArray>& content, std::unordered_map<std::string, uint32_t>& key_offset, uint32_t data_size)
{
	if (content.empty())
	{
		log_->Info("Flush content is empty");
	}

	uint32_t offset = 0;

	char encoded_uint32[6] = { 0 };
	char* encoded_ptr = nullptr;
	int len = 0;

	ByteArray lower = content.front();
	ByteArray upper = content.back();

	uint32_t key_size;
	const char* p = lower.Data();
	len = GetVarint32(p, 5, &key_size);

	fwrite(lower.Data(), sizeof(char), len + key_size, stream);
	offset += len + key_size;

	p = upper.Data();
	len = GetVarint32(p, 5, &key_size);

	fwrite(upper.Data(), sizeof(char), len + key_size, stream);
	offset += len + key_size;

	uint32_t index_offset = offset + 4 + data_size;
	EncodeFixed32(encoded_uint32, index_offset);
	fwrite(encoded_uint32, sizeof(char), 4, stream);
	offset += 4;

	std::string prev = "00000";	// DEBUG
	for (auto& entry : content)
	{
		ByteArray key(ExtractUserKey(entry));
		std::string user_key(key.Data(), key.Size());
		key_offset[user_key] = offset;

		if (user_key < prev)
		{
			log_->Info("wrong order! prev: %s, user_key: %s.", prev.c_str(), user_key.c_str());
		}

		assert(user_key >= prev);	// DEBUG
		prev = user_key;	// DEBUG

		fwrite(entry.Data(), sizeof(char), entry.Size(), stream);
		offset += entry.Size();
	}

	for (auto& item : key_offset)
	{
		encoded_ptr = encoded_uint32;
		encoded_ptr = EncodeVarint32(encoded_ptr, item.first.size());
		
		fwrite(encoded_uint32, sizeof(char), encoded_ptr - encoded_uint32, stream);

		fwrite(item.first.c_str(), sizeof(char), item.first.size(), stream);

		encoded_ptr = encoded_uint32;
		encoded_ptr = EncodeVarint32(encoded_ptr, item.second);

		fwrite(encoded_uint32, sizeof(char), encoded_ptr - encoded_uint32, stream);
	}

	fclose(stream);
}

void StorageBuffer::FlushBuffer(FILE* stream, std::unordered_map<std::string, uint32_t>& key_offset)
{
	log_->Info("Starting Flush");

	assert(flush_buffer_ != nullptr);

	std::vector<ByteArray> content;
	uint32_t flush_size = 0;

	SkipList<const char*, Comparator>::Iterator it(flush_buffer_);
	
	for (it.SeekToFirst(); it.Valid(); it.Next())
	{
		uint32_t entry_size = EntrySize(it.key());
		content.push_back(ByteArray(it.key(), entry_size));

		flush_size += entry_size;
	}

	Flush(stream, content, key_offset, flush_size);

	log_->Info("Ending Flush. Content Size: %d, %d Entries Flushed.", flush_size, content.size());
}

void StorageBuffer::ClearFlushBuffer()
{
	std::unique_lock<std::mutex> lock(mutex_);

	flush_buffer_ready_ = false;

	delete flush_memory_;
	flush_memory_ = nullptr;

	delete flush_buffer_;
	flush_buffer_ = nullptr;
}

int StorageBuffer::Get(std::string& key, std::string& value_out)
{
	// TODO: Use better enum type to indicate the return status.
	int status = -1;

	int encoded_len = VarintLength(key.size()) + key.size();
	char* temp = new char[encoded_len];
	char* p = temp;
	p = EncodeVarint32(p, key.size());
	memcpy(p, key.c_str(), key.size());

	std::unique_lock<std::mutex> lock(mutex_);
	if (income_buffer_ != nullptr)
	{
		SkipList<const char*, Comparator>::Iterator it(income_buffer_);
		it.Seek(temp);
		if (it.Valid())
		{
			status = 0;
			ByteArray value(ExtractUserValue(it.key()));
			value_out.assign(value.Data(), value.Size());
		}
	}

	if (status != 0 && flush_buffer_ != nullptr)
	{
		SkipList<const char*, Comparator>::Iterator it(flush_buffer_);
		it.Seek(temp);
		if (it.Valid())
		{
			status = 0;
			ByteArray value(ExtractUserValue(it.key()));
			value_out.assign(value.Data(), value.Size());
		}
	}

	delete[] temp;

	return status;
}
