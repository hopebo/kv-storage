// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef CONCURRENT_QUEUE_H_
#define CONCURRENT_QUEUE_H_

#include <queue>
#include <mutex>

template <class T>
class ConcurrentQueue 
{
public:
	T pop() 
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		auto item = queue_.front();
		queue_.pop();
		--size_;
		return item;
	}

	void push(const T& item) 
	{
		std::unique_lock<std::mutex> mlock(mutex_);
		queue_.push(item);
		++size_;
	}

	unsigned int size() 
	{
		return size_;
	}

	bool empty() 
	{
		return size_ == 0;
	}
	
private:
	std::queue<T> queue_;
	std::mutex mutex_;
	unsigned int size_ = 0;
};

#endif	// CONCURRENT_QUEUE_H_
