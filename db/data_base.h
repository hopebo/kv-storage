// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef DATA_BASE_H_
#define DATA_BASE_H_

#include <thread>

#include "event_manager.h"
#include "storage_buffer.h"
#include "storage_engine.h"
#include "../util/logger.h"
#include "../structure/cache.h"

class DataBase
{
private:
	bool is_stop_ = false;
	std::thread thread_flush_;
	std::thread thread_compact_;

	EventManager* event_manager_;
	StorageBuffer* storage_buffer_;
	StorageEngine* storage_engine_;
	Logger* log_;
	LRUCache* cache_;

public:
	DataBase(EventManager* event_manager, StorageBuffer* storage_buffer, StorageEngine* storage_engine, Logger* logger, LRUCache* cache) : event_manager_(event_manager), storage_buffer_(storage_buffer), log_(logger), storage_engine_(storage_engine), cache_(cache) { }
	~DataBase() { }
	// Backend thread doing flushing work
	void ProcessingLoopFlushBuffer();
	// Backend thread doing compaction work
	void ProcessingLoopCompact();
	// Put/Delete Opeartion
	void Add(OrderType order_type, std::string& key, std::string& value);
	// Get Operation
	int Get(std::string& key, std::string& value_out);
	// DataBase Start
	void Start();
	// DataBase ShutDown
	void ShutDown();
	// Clear LRU Cache, containing Key-Offset tables
	void ClearCache();
};

#endif  // DATA_BASE_H_
