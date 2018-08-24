// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef CACHE_H_
#define CACHE_H_

#include <unordered_map>
#include <string>
#include <mutex>

#include <stdint.h>

// Node is designed for key-offset table.
struct LRUCacheNode
{
	int key;
	std::unordered_map<std::string, uint32_t> value;
	LRUCacheNode* prev;
	LRUCacheNode* next;
	LRUCacheNode() : key(0), prev(nullptr), next(nullptr) { }
};

class LRUCache
{
private:
	std::unordered_map<int, LRUCacheNode*> m;
	LRUCacheNode* head;
	LRUCacheNode* tail;
	int capacity;
	int count;

	// TODO: Maybe using some shared-memory like things can eliminate mutex.
	// Running these codes in a single thread is efficient enough? 
	// It involves too many modification operations.
	std::mutex mutex_;

public:
	LRUCache(int capacity);
	~LRUCache();
	uint32_t Get(int key, std::string& key_str, bool& if_exists);
	void Set(int key, std::unordered_map<std::string, uint32_t>& value);
	void Clear();	

private:
	void RemoveLRUNode();
	void DetachNode(LRUCacheNode* node);
	void InsertToFront(LRUCacheNode* node);
};

#endif  // CACHE_H_
