// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef FILE_LOGGER_H_
#define FILE_LOGGER_H_

#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <mutex>
#include <thread>
#include <cstdarg>
#include <sstream>

#include "logger.h"

class FileLogger : public Logger {
private:
	FILE* file_;
	std::mutex mutex_;
	bool multi_thread_env_;
	bool flush_;

public:
	FileLogger(const char* log_file_path, LogLevel log_level, bool multi_thread_env, bool if_flush) : multi_thread_env_(multi_thread_env), flush_(if_flush) 
	{
		log_level_ = log_level;
		if ((file_ = fopen(log_file_path, "a+")) == nullptr)
		{
			printf("open log file failed");
			exit(1);
		}
	};

	virtual ~FileLogger()
	{
		if (file_ != nullptr)
		{
			fclose(file_);
		}
	};

	void Logv(LogLevel log_level, const char* format, va_list args) override
	{
		char buffer[500];
		for (int iter = 0; iter < 2; iter++)
		{
			char* base;
			int buffer_size;
			if (iter == 0)
			{
				buffer_size = sizeof(buffer);
				base = buffer;
			}
			else
			{
				buffer_size = 30000;
				base = new char[buffer_size];
			}
			
			char* p = base;
			char* limit = base + buffer_size;
			struct timeval now;
			gettimeofday(&now, nullptr);
			const time_t seconds = now.tv_sec;
			struct tm t;
			localtime_r(&seconds, &t);

			std::ostringstream ss;
			ss << std::this_thread::get_id();
	
			p += snprintf(p, limit - p,
						  "[%s] %04d/%02d/%02d-%02d:%02d:%02d.%02d thread(%s) ",
						  LogLevelString[log_level],
						  t.tm_year + 1900,
						  t.tm_mon + 1,
						  t.tm_mday,
						  t.tm_hour,
						  t.tm_min,
						  t.tm_sec,
						  static_cast<int>(now.tv_usec),
						  ss.str().c_str()); 
			
			if (p < limit)
			{
				va_list backup_args;
				va_copy(backup_args, args);
				p += vsnprintf(p, limit - p, format, backup_args);
				va_end(backup_args);
			}

			if (p >= limit)
			{
				if (iter == 0)
				{
					continue;
				}
				else
				{
					p = limit - 1;
				}
			}

			if (p == base || p[-1] != '\0')
			{
				*p++ = '\n';
				*p++ = '\0';
			}
			
			if (multi_thread_env_)
			{
				mutex_.lock();
			}

			fwrite(base, sizeof(char), p - base - 1, file_);

			if (flush_)
			{
				fflush(file_);
			}
		
			if (multi_thread_env_)
			{
				mutex_.unlock();
			}

			if (base != buffer)
			{
				delete[] base;
			}

			break;
		}
	}
};

#endif  // FILE_LOGGER_H_
