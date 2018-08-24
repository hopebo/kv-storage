// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef STORAGE_ENGINE_H_
#define STORAGE_ENGINE_H_

#include <map>
#include <vector>
#include <algorithm>
#include <mutex>
#include <queue>
#include <utility>
#include <unordered_map>
#include <string>

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <fcntl.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "file.h"
#include "storage_buffer.h"
#include "event_manager.h"
#include "../util/utils.h"
#include "../util/logger.h"
#include "../type/byte_array.h"
#include "../type/constant.h"
#include "../structure/read_write_lock.h"

class StorageEngine
{
private:
	int file_id_ = -1;
	int level0_files_number_limit_;

	std::map<int, std::vector<File*>> level_files_;
	std::unordered_map<int, File*> files_map_;

	std::mutex mutex_;

	Logger* log_;
	EventManager* event_manager_;
	StorageBuffer* storage_buffer_;
	ReadWriteLock rw_lock_;

	struct Entry
	{
		std::string key_;
		int level_id_;
		int file_id_;
		int index_;
		bool is_delete_;
		char* p_;
		uint32_t size_;
		
		Entry(std::string key, int level_id, int file_id, bool is_delete, char* p, uint32_t size, int index)
			: key_(key),
			  level_id_(level_id),
			  file_id_(file_id),
			  is_delete_(is_delete),
			  p_(p),
			  size_(size),
			  index_(index)
		{
		}
	};

	struct priority_queue_cmp
	{
		bool operator()(Entry& a, Entry& b)
		{
			return (a.key_ > b.key_) || (a.key_ == b.key_ && (a.level_id_ > b.level_id_ || (a.level_id_ == b.level_id_ && a.file_id_ < b.file_id_)));
		}
	};

	static bool cmp(const File* file1, const File* file2)
	{
		return file1->LowerBound() < file2->LowerBound();
	}

	// Only one file to compact, just move it to next level.
	std::vector<File*> TrivialMove(std::vector<File*>& compact_files);

	// Update File Map after compaction finishing.
	void UpdateMapAfterCompaction(std::vector<File*>& compacted_files, std::vector<File*>& compact_files, bool need_remove_file);

	// N Way Compaction on compact_files
	std::vector<File*> NWayCompaction(std::vector<File*>& compact_files, int level_id);

	// Flush single file during N Way Compaction process
	std::string FlushCompactedFile(std::vector<ByteArray>& content, uint32_t content_size, int level_id);

	// Extract Entry from Data File
	Entry ExtractEntry(char*& p, File* file, int index);

	// Find Overlap Files in Next Level
	void FindOverlapFilesBasedOnBound(std::vector<File*>& candidate_files, std::vector<File*>& compact_files, std::string& lowerbound, std::string& upperbound);

	// Find Overlap Files in Level 0
	void FindOverlapFilesLevel0(std::vector<File*>& candidate_files, std::vector<File*>& compact_files, std::string& lowerbound, std::string& upperbound);

public:
	// Create New File for Flush
	FILE* NewWritableFile(int& file_id, std::string& file_name, int level_id = 0);

	StorageEngine(Logger* log, int level0_files_number_limit, EventManager* event_manager, StorageBuffer* storage_buffer);

	// Add New File to File Map
	void AddFile(std::string file_name);

	// Get possible files possible to contain corresponding key
	void GetContainsFiles(std::string& key, std::vector<std::vector<File*>>& contains_files); 

	// Read Key-Offset table from file
	void LoadKeyOffset(int file_id, std::unordered_map<std::string, uint32_t>& key_offset);

	// Get value from file
	void GetValueByOffset(int file_id, uint32_t offset, std::string& value_out);

	// Compaction on given Level
	void Compact(int level_id);

	void ReadLock() { rw_lock_.ReadLock(); }
	void ReadUnlock() { rw_lock_.ReadUnlock(); }
	void WriteLock() { rw_lock_.WriteLock(); }
	void WriteUnlock() { rw_lock_.WriteUnlock(); }
};

#endif  // STORAGE_ENGINE_H_
