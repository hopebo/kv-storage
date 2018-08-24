// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef TASK_H_
#define TASK_H_

#include <thread>
#include <stdint.h>

class Task
{
private:
	bool stop_requested_;

public:
	Task() : stop_requested_(false) { }
	virtual ~Task() { }
	virtual void RunInLock(std::thread::id tid) = 0;
	virtual void Run(std::thread::id tid) = 0;
	bool IsStopRequested() { return stop_requested_; }
	void Stop() { stop_requested_ = true; }
};

#endif  // TASK_H_
