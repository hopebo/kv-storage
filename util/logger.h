// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef LOGGER_H_
#define LOGGER_H_

#include <cstdarg>

#include "log_level.h"

class Logger {
protected:
	LogLevel log_level_;

public:
	void Trace(const char* format, ...)
	{
		if (LogLevelTrace < log_level_)
		{
			return;
		}

		va_list args;
		va_start(args, format);
		Logv(LogLevelTrace, format, args);
		va_end(args);
	}

	void Debug(const char* format, ...)
	{
		if (LogLevelDebug < log_level_)
		{
			return;
		}

		va_list args;
		va_start(args, format);
		Logv(LogLevelDebug, format, args);
		va_end(args);
	}

	void Info(const char* format, ...)
	{
		if (LogLevelInfo < log_level_)
		{
			return;
		}

        va_list args;
		va_start(args, format);
		Logv(LogLevelInfo, format, args);
		va_end(args);
	}

	void Warn(const char* format, ...)
	{
		if (LogLevelWarn < log_level_)
		{
			return;
		}

		va_list args;
		va_start(args, format);
		Logv(LogLevelWarn, format, args);
		va_end(args);
	}

	void Error(const char* format, ...)
	{
		if (LogLevelError < log_level_)
		{
			return;
		}

		va_list args;
		va_start(args, format);
		Logv(LogLevelError, format, args);
		va_end(args);
	}

	virtual void Logv(LogLevel log_level, const char* format, va_list args) = 0;
};

#endif  // LOGGER_H_
