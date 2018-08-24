// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef EVENT_MANAGER_H_
#define EVENT_MANAGER_H_

#include "event.h"

class EventManager
{
public:
	Event event_flush_buffer_;
	Event event_compact_;
};

#endif  // EVENT_MANAGER_H_
