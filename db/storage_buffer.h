#ifndef STORAGE_BUFFER_H_
#define STORAGE_BUFFER_H_

#include <unordered_map>
#include <mutex>
#include <string>

#include <stdio.h>

#include "event_manager.h"
#include "../type/byte_array.h"
#include "../type/order_type.h"
#include "../util/logger.h"
#include "../util/comparator.h"
#include "../structure/memory.h"
#include "../structure/skip_list.h"

class StorageBuffer
{
private:
	uint32_t buffer_size_;
	uint32_t income_size_ = 0;
	std::mutex mutex_;
	SkipList<const char*, Comparator>* income_buffer_ = nullptr;
	SkipList<const char*, Comparator>* flush_buffer_ = nullptr;
	Memory* income_memory_ = nullptr;
	Memory* flush_memory_ = nullptr;
	EventManager* event_manager_;
	Logger* log_;
	Comparator cmp_;
	bool flush_thread_ready_;
	bool flush_buffer_ready_;

public:
	StorageBuffer(uint32_t buffer_size, Logger* log, EventManager* event_manager) : buffer_size_(buffer_size), log_(log), event_manager_(event_manager), flush_thread_ready_(false), flush_buffer_ready_(false)
	{
		income_memory_ = new Memory();
		income_buffer_ = new SkipList<const char*, Comparator>(cmp_, income_memory_); 
	}

	~StorageBuffer() 
	{
		if (income_memory_ != nullptr)
		{
			delete income_memory_;
		}

		if (flush_memory_ != nullptr)
		{
			delete flush_memory_;
		}

		if (income_buffer_ != nullptr)
		{
			delete income_buffer_;
		}

		if (flush_buffer_ != nullptr)
		{
			delete flush_buffer_;
		}
	}

	// Put record to Income Buffer
	void Add(OrderType order_type, const ByteArray& key, const ByteArray& value);
	// Flush Flush Buffer
	void FlushBuffer(FILE* data_file, std::unordered_map<std::string, uint32_t>& key_offset);
	// General Flush function, reused by compaction process
	void Flush(FILE* stream, std::vector<ByteArray>& content, std::unordered_map<std::string, uint32_t>& key_offset, uint32_t data_size);
	// Clear Flush Buffer
	void ClearFlushBuffer();
	// Get Operation from Buffers
	int Get(std::string& key, std::string& value_out);
	// Swap Income Buffer and Flush Buffer
	void SwapBuffer();
	uint32_t BufferSize()
	{
		return buffer_size_;
	}

	void SetFlushThreadIdle()
	{
		flush_thread_ready_ = true;
	}

	void SetFlushThreadBusy()
	{
		flush_thread_ready_ = false;
	}

	bool FlushBufferReady()
	{
		return flush_buffer_ready_;
	}
};

#endif  // STORAGE_BUFFER_H_
