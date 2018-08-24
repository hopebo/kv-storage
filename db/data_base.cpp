// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#include <unistd.h>

#include "data_base.h"
#include "../type/constant.h"

void DataBase::Start()
{
	thread_flush_ = std::thread(&DataBase::ProcessingLoopFlushBuffer, this);
	thread_compact_ = std::thread(&DataBase::ProcessingLoopCompact, this);
	log_->Info("Database Starts Successfully.");
	printf("Database Starts Successfully.\n");
}

void DataBase::ProcessingLoopFlushBuffer()
{
	while (!is_stop_)
	{
		storage_buffer_->SetFlushThreadIdle();

		while (!storage_buffer_->FlushBufferReady() && !is_stop_)
		{
			event_manager_->event_flush_buffer_.Wait();
		}

		if (is_stop_)
		{
			break;
		}

		int file_id;
		std::string file_name;
		auto stream = storage_engine_->NewWritableFile(file_id, file_name);

		std::unordered_map<std::string, uint32_t> key_offset;

		storage_buffer_->FlushBuffer(stream, key_offset);

		storage_buffer_->ClearFlushBuffer();

		cache_->Set(file_id, key_offset);	

		storage_engine_->AddFile(file_name);
	}
}

void DataBase::ProcessingLoopCompact()
{
	while (!is_stop_)
	{
		event_manager_->event_compact_.Wait();

		if (is_stop_)
		{
			break;
		}

		storage_engine_->Compact(0);
	}
}

void DataBase::ShutDown()
{
	is_stop_ = true;
	event_manager_->event_flush_buffer_.Notify();
	event_manager_->event_compact_.Notify();
	thread_flush_.join();
	thread_compact_.join();
}

void DataBase::Add(OrderType order_type, std::string& key, std::string& value)
{
	log_->Info("%s Key: %s, Value: %s", OrderTypeString[order_type], key.c_str(), value.c_str());
	if (order_type == Delete)
	{
		order_type = Put;
		value = Constant::TombValue;
	}
	
	storage_buffer_->Add(order_type, ByteArray(key.c_str(), key.size()), ByteArray(value.c_str(), value.size()));
}

int DataBase::Get(std::string& key, std::string& value_out)
{
	int status = -1;
	if (is_stop_)
	{
		return status;
	}

	if ((status = storage_buffer_->Get(key, value_out)) == 0)
	{
		return status;
	}

	std::vector<std::vector<File*>> contains_files;
	storage_engine_->ReadLock();
	storage_engine_->GetContainsFiles(key, contains_files);
	uint32_t offset = 0;
	for (auto& vec : contains_files)
	{
		for (auto& file : vec)
		{
			bool if_exists = false;
			offset = cache_->Get(file->FileId(), key, if_exists);
			if (!if_exists)
			{
				std::unordered_map<std::string, uint32_t> key_offset;
				storage_engine_->LoadKeyOffset(file->FileId(), key_offset);
				if (key_offset.find(key) != key_offset.end())
				{
					offset = key_offset[key];
				}

				cache_->Set(file->FileId(), key_offset);
			}

			if (offset != 0)
			{
				storage_engine_->GetValueByOffset(file->FileId(), offset, value_out);
				status = 0;
				storage_engine_->ReadUnlock();
				return status;
			}
		}
	}

	storage_engine_->ReadUnlock();
	return status;
}

void DataBase::ClearCache()
{
	cache_->Clear();
}
