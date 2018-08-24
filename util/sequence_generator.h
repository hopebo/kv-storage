// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef SEQUENCE_GENERATOR_H_
#define SEQUENCE_GENERATOR_H_

#include <string>
#include <vector>
#include <utility>

std::string RandomString(const int len);
std::vector<std::pair<std::string, std::string>> RandomKvPairs(int num, int key_len, int value_len, int random_seed = 987654321);

#endif  // SEQUENCE_GENERATOR_H_
