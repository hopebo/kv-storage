// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef THREAD_POOL_H_
#define THREAD_POOL_H_

#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <map>

#include "task.h"

class ThreadPool
{
public:
	int num_threads_;
	bool stop_requested_;
	std::queue<Task*> queue_;
	std::condition_variable cv_;
	std::mutex mutex_;
	std::vector<std::thread> threads_;
	std::map<std::thread::id, Task*> tid_to_task_;

	ThreadPool(int num_threads) : num_threads_(num_threads), stop_requested_(false) { }

	~ThreadPool() { }

	void ProcessingLoop()
	{
		while (!IsStopRequested())
		{
			std::unique_lock<std::mutex> lock(mutex_);
			while (queue_.empty())
			{
				cv_.wait(lock);
				if (IsStopRequested())
				{
					break;
				}
			}

			if (IsStopRequested())
			{
				break;
			}

			Task* task = queue_.front();
			queue_.pop();
			if (task == nullptr)
			{
				continue;
			}

			auto tid = std::this_thread::get_id();
			tid_to_task_[tid] = task;
			task->RunInLock(tid);
			lock.unlock();

			task->Run(tid);

			mutex_.lock();
			delete task;
			tid_to_task_.erase(tid);
			mutex_.unlock();
		}
	}

	void AddTask(Task* task)
	{
		std::unique_lock<std::mutex> lock(mutex_);
		queue_.push(task);
		cv_.notify_one();
	}

	void Start()
	{
		for (auto i = 0; i < num_threads_; ++i)
		{
			threads_.push_back(std::thread(&ThreadPool::ProcessingLoop, this));
		}
	}

	void Stop()
	{
		stop_requested_ = true;
		cv_.notify_all();
		for (auto& t : threads_)
		{
			t.join();
		}

		mutex_.lock();
		for (auto& tid_task : tid_to_task_)
		{
			Task* task = tid_task.second;
			task->Stop();
			delete task;
		}

		while (!queue_.empty())
		{
			Task* task = queue_.front();
			queue_.pop();
			delete task;
		}

		mutex_.unlock();
	}

	void BlockUntilAllTaskHaveCompleted()
	{
		while (!queue_.empty())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(500));
		}
	}

	bool IsStopRequested()
	{
		return stop_requested_;
	}
};

#endif  // THREAD_POOL_H_
