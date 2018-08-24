// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef READ_WRITE_LOCK_H_
#define READ_WRITE_LOCK_H_

#include <mutex>
#include <condition_variable>

class ReadWriteLock
{
private:
	std::mutex mutex_;
	std::condition_variable cond_;
	int stat;	// == 0 no lock; > 0 number of read lock; < 0 already write lock

public:
	ReadWriteLock() : stat(0) { }	

	void ReadLock()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		while (stat < 0)
		{
			cond_.wait(lock);
		}

		++stat;
	}

	void ReadUnlock()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		--stat;
		if (stat == 0)
		{
			cond_.notify_one();
		}
	}

	void WriteLock()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		while (stat != 0)
		{
			cond_.wait(lock);
		}

		--stat;
	}

	void WriteUnlock()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		++stat;
		cond_.notify_all();
	}
};

#endif  // READ_WRITE_LOCK_H_
