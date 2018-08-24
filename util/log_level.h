// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef LOG_LEVEL_H_
#define LOG_LEVEL_H_

enum LogLevel
{
	LogLevelTrace = 0,
	LogLevelDebug = 1,
	LogLevelInfo = 2,
	LogLevelWarn = 3,
	LogLevelError = 4,
};

extern const char* LogLevelString[];

#endif  // LOG_LEVEL_H_
