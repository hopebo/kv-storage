// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

// The code below was copied from LevelDB. A few changes were applied to make it
// self-sufficient.

// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef SKIP_LIST_H_
#define SKIP_LIST_H_

#include <assert.h>

#include "random.h"
#include "memory.h"

template<typename Key, class Comparator>
class SkipList
{
private:
	struct Node;
	
	enum { kMaxHeight = 12 };

	Comparator const compare_;

	Memory* const memory_;

	Node* const head_;

	Random rand_;

	int max_height_ = 1;

	Node* NewNode(const Key& key, int height);
	int RandomHeight();
	bool Equal(const Key& a, const Key& b) const { return (compare_(a, b) == 0); }
	
	// Return true if key is greater than the data stored in "n"	
	bool KeyIsAfterNode(const Key& key, Node* n) const;
	
	// Return the earliest node that comes at or after key.
	// Return nullptr if there is no such node.
	//
	// If prev is non-null, fills prev[level] with pointer to previous
	// node at "level" for every level in [0..max_height - 1].
	Node* FindGreaterOrEqual(const Key& key, Node** prev) const;

	Node* FindLessThan(const Key& key) const;

	Node* FindLast() const;

	SkipList(const SkipList&);
	void operator=(const SkipList&);

	inline int GetMaxHeight() const 
	{
		return max_height_;
	}

public:
	// Create a new SkipList object that will use "cmp" for comparing keys,
	// and will allocate memory using "memory". Objects allocated in the memory
	// must remain allocated for the lifetime of the skiplist object.
	SkipList(Comparator cmp, Memory* memory);

	// Insert key into the list
	// REQUIRES: nothing that compares equal to key is currently in the list.
	void Insert(const Key& key);

	// Returns true if an entry that compares equal to key is in the list.
	bool Contains(const Key& key) const;

	class Iterator
	{
	public:
		explicit Iterator(const SkipList* list);

		bool Valid() const;

		const Key& key() const;

		void Next();

		void Prev();

		void Seek(const Key& target);

		void SeekToFirst();

		void SeekToLast();
	
	private:
		const SkipList* list_;
		Node* node_;
	};
};

template<typename Key, class Comparator>
SkipList<Key,Comparator>::SkipList(Comparator cmp, Memory* memory)
    : head_(NewNode("00000", kMaxHeight)),
	  compare_(cmp),
	  memory_(memory),
      max_height_(1),
      rand_(0xdeadbeef) {
  for (int i = 0; i < kMaxHeight; i++) {
    head_->SetNext(i, nullptr);
  }
}

template<typename Key, class Comparator>
inline SkipList<Key,Comparator>::Iterator::Iterator(const SkipList* list) {
  list_ = list;
  node_ = nullptr;
}

template<typename Key, class Comparator>
inline bool SkipList<Key,Comparator>::Iterator::Valid() const {
  return node_ != nullptr;
}

template<typename Key, class Comparator>
inline const Key& SkipList<Key,Comparator>::Iterator::key() const {
  assert(Valid());
  return node_->key;
}

template<typename Key, class Comparator>
inline void SkipList<Key,Comparator>::Iterator::Next() {
  assert(Valid());
  node_ = node_->Next(0);
}

template<typename Key, class Comparator>
inline void SkipList<Key,Comparator>::Iterator::Prev() {
  // Instead of using explicit "prev" links, we just search for the
  // last node that falls before key.
  assert(Valid());
  node_ = list_->FindLessThan(node_->key);
  if (node_ == list_->head_) {
    node_ = nullptr;
  }
}

template<typename Key, class Comparator>
inline void SkipList<Key,Comparator>::Iterator::Seek(const Key& target) {
  node_ = list_->FindGreaterOrEqual(target, nullptr);
  if (node_ != nullptr && !list_->Equal(node_->key, target)) {
	node_ = nullptr;
  }
}

template<typename Key, class Comparator>
inline void SkipList<Key,Comparator>::Iterator::SeekToFirst() {
  node_ = list_->head_->Next(0);
}

template<typename Key, class Comparator>
inline void SkipList<Key,Comparator>::Iterator::SeekToLast() {
  node_ = list_->FindLast();
  if (node_ == list_->head_) {
    node_ = nullptr;
  }
}

template<typename Key, class Comparator>
struct SkipList<Key, Comparator>::Node
{
	explicit Node(const Key& k) : key(k) { }

	Key key;

	Node* Next(int n)
	{
		assert(n >= 0);
		return reinterpret_cast<Node*>(next_[n]);
	}

	void SetNext(int n, Node* x)
	{
		assert(n >= 0);
		next_[n] = x;
	}

private:
	void* next_[1];
};

template<typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::NewNode(const Key& key, int height)
{
	char* mem = memory_->AllocateAligned(
		sizeof(Node) + sizeof(void*) * (height - 1));
	return new (mem) Node(key);
}

template<typename Key, class Comparator>
int SkipList<Key, Comparator>::RandomHeight()
{
	static const unsigned int kBranching = 4;
	int height = 1;
	while (height < kMaxHeight && ((rand_.Next() % kBranching) == 0))
	{
		height++;
	}

	assert(height > 0);
	assert(height <= kMaxHeight);
	return height;
}

template<typename Key, class Comparator>
bool SkipList<Key, Comparator>::KeyIsAfterNode(const Key& key, Node* n) const
{
	return (n != nullptr) && (compare_(n->key, key) < 0);
}

template<typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::FindGreaterOrEqual(const Key& key, Node** prev) const
{
	Node* x = head_;
	int level = GetMaxHeight() - 1;
	while (true)
	{
		Node* next = x->Next(level);
		if (KeyIsAfterNode(key, next))
		{
			// Keep searching in this list
			x = next;
		}
		else
		{
			if (prev != nullptr)
			{
				prev[level] = x;
			}

			if (level == 0)
			{
				return next;
			}
			else
			{
				--level;
			}
		}
	}
}

template<typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::FindLessThan(const Key& key) const
{
	Node* x = head_;
	int level = GetMaxHeight() - 1;
	while (true)
	{
		assert(x == head_ || compare_(x->key, key) < 0);
		Node* next = x->Next(level);
		if (next == nullptr || compare_(next->key, key) >= 0)
		{
			if (level == 0)
			{
				return x;
			}
			else
			{
				level--;
			}
		}
		else
		{
			x = next;
		}
	}
}

template<typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::FindLast() const
{
	Node* x = head_;
	int level = GetMaxHeight() - 1;
	while (true)
	{
		Node* next = x->Next(level);
		if (next == nullptr)
		{
			if (level == 0)
			{
				return x;
			}
			else
			{
				level--;
			}
		}
		else
		{
			x = next;
		}
	}
}

template<typename Key, class Comparator>
void SkipList<Key, Comparator>::Insert(const Key& key)
{
	Node* prev[kMaxHeight];
	Node* x = FindGreaterOrEqual(key, prev);

	if (x != nullptr && Equal(x->key, key))
	{
		x->key = key;
		return;
	}

	int height = RandomHeight();
	if (height > GetMaxHeight())
	{
		for (int i = GetMaxHeight(); i < height; ++i)
		{
			prev[i] = head_;
		}

		max_height_ = height;
	}

	x = NewNode(key, height);
	for (int i = 0;i < height; ++i)
	{
		x->SetNext(i, prev[i]->Next(i));
		prev[i]->SetNext(i, x);
	}
}

template<typename Key, class Comparator>
bool SkipList<Key, Comparator>::Contains(const Key& key) const
{
	Node* x = FindGreaterOrEqual(key, nullptr);
	if (x != nullptr && Equal(key, x->key))
	{
		return true;
	}
	else
	{
		return false;
	}
}


#endif  // SKIP_LIST_H_
