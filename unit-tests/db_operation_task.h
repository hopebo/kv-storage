// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef DB_OPERATION_TASK_H_
#define DB_OPERATION_TASK_H_

#include <string>
#include <utility>

#include <assert.h>

#include "../db/data_base.h"
#include "../type/order_type.h"
#include "../type/constant.h"
#include "../structure/task.h"
#include "../structure/concurrent_queue.h"

class DBOperationTask : public Task
{
private:
	DataBase* data_base_;
	ConcurrentQueue<std::pair<std::string, std::string>>* result_queue_;
	OrderType order_type_;
	std::string key_;
	std::string value_;
	std::string value_out_ = "";
	std::string tomb_value_;

public:
	DBOperationTask(DataBase* data_base, ConcurrentQueue<std::pair<std::string, std::string>>* result_queue, OrderType order_type, std::string key, std::string value = "")
		: data_base_(data_base),
		  result_queue_(result_queue),
		  order_type_(order_type),
		  key_(key),
		  value_(value)
	{
		tomb_value_ = Constant::TombValue;
	}
	
	void RunInLock(std::thread::id tid) override { }
	void Run(std::thread::id tid) override
	{
		switch (order_type_)
		{
			case Put:
				data_base_->Add(Put, key_, value_);
				break;

			case Delete:
				data_base_->Add(Put, key_, tomb_value_);
				break;

			case Get:
				data_base_->Get(key_, value_out_);
				result_queue_->push(std::pair<std::string, std::string>(value_, value_out_));
				break;

			default:
				break;
		}
	}
};

#endif  // DB_OPERATION_TASK_H_

