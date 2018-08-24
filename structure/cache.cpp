// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#include "cache.h"

LRUCache::LRUCache(int capacity)
{
	this->capacity = capacity;
	this->count = 0;
	head = new LRUCacheNode;
	tail = new LRUCacheNode;
	head->prev = NULL;
	head->next = tail;
	tail->prev = head;
	tail->next = NULL;
}

LRUCache::~LRUCache()
{
	delete head;
	delete tail;
	for (auto& item : m)
	{
		delete item.second;	
	}
}

void LRUCache::Clear()
{
	std::unique_lock<std::mutex> lock(mutex_);
	for (auto& item : m)
	{
		delete item.second;
	}

	m.clear();
	count = 0;
	head->next = tail;
	tail->prev = head;
}

uint32_t LRUCache::Get(int key, std::string& key_str, bool& if_exists)
{
	uint32_t offset = 0;
	if_exists = false;
	mutex_.lock();
	if(m.find(key) != m.end())
	{
		if_exists = true;
		LRUCacheNode* node = m[key];
		if (node->value.find(key_str) != node->value.end())
		{
			offset = node->value[key_str];
			DetachNode(node); 
			InsertToFront(node);
		}
	}

	mutex_.unlock();
	return offset;
}

void LRUCache::Set(int key, std::unordered_map<std::string, uint32_t>& value)
{
	mutex_.lock();
	if(m.find(key) == m.end())
	{
		LRUCacheNode* node = new LRUCacheNode;
		if(count == capacity) 
			RemoveLRUNode();

		node->key = key;
		node->value = value;
		m[key] = node;         
		InsertToFront(node);
		++count;
	}
	else
	{
		LRUCacheNode* node = m[key];
		DetachNode(node);
		node->value = value;
		InsertToFront(node);
	}
	
	mutex_.unlock();
}

void LRUCache::RemoveLRUNode()
{
	LRUCacheNode* node = tail->prev;
	DetachNode(node);
	m.erase(node->key);
	delete node;
	--count;
}

void LRUCache::DetachNode(LRUCacheNode* node)
{
	node->prev->next = node->next;
	node->next->prev = node->prev;
}


void LRUCache::InsertToFront(LRUCacheNode* node)
{
	node->next = head->next;
	node->prev = head;
	head->next = node;
	node->next->prev = node;
}

