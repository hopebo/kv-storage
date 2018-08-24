// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef EVENT_H_
#define EVENT_H_

#include <thread>
#include <mutex>
#include <condition_variable>

class Event
{
public:
	Event() { }
	~Event() { }
	void Wait()
	{
		std::unique_lock<std::mutex> lock(mutex_);
		cv_.wait(lock);
	}

	void Notify()
	{
		cv_.notify_one();
	}

private:
	std::mutex mutex_;
	std::condition_variable cv_;
};

#endif  // EVENT_H_
