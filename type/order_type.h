// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef ORDER_TYPE_H_
#define ORDER_TYPE_H_

enum OrderType
{
	Put = 0,
	Delete = 1,
	Get = 2,
};

extern const char* OrderTypeString[];

#endif  // ORDER_TYPE_H_
