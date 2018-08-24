// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef FILE_H_
#define FILE_H_

#include <string>

#include <stdint.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include "../type/constant.h"
#include "../util/coding.h"

class File
{
private:
	int level_id_;
	int file_id_;
	uint32_t file_size_;
	std::string file_name_;

	std::string lower_bound_;
	std::string upper_bound_;

	const char* mmap_;

	uint32_t GetFileSize(const char* file_path)
	{
		uint32_t file_size = -1;
		struct stat statbuff;
		if (!(stat(file_path, &statbuff) < 0))
		{
			file_size = statbuff.st_size;
		}

		return file_size;
	}

public:
	std::string FilePath()
	{
		return FilePath(file_name_);
	}

	std::string FileName()
	{
		return file_name_;
	}

	std::string FilePath(std::string file_name)
	{
		std::string file_path = Constant::DataFolder;
		file_path.append("/");
		file_path.append(file_name);
		return file_path;
	}

	File(std::string file_name)
	{
		auto pos = file_name.find("_");
		if (pos == std::string::npos)
		{
			printf("Parsing file name \"%s\" failed", file_name.c_str());
		}

		level_id_ = stoi(file_name.substr(0, pos));
		file_id_ = stoi(file_name.substr(pos + 1));
		
		std::string file_path = FilePath(file_name);
		file_size_ = GetFileSize(file_path.c_str());
		if (file_size_ < 0)
		{
			printf("Acquire file size failed");
		}

		auto fd = open(file_path.c_str(), O_RDONLY);
		mmap_ = static_cast<const char*>(mmap(0, file_size_, PROT_READ, MAP_SHARED, fd, 0));
		close(fd);

		char* p = const_cast<char*>(mmap_);
		uint32_t key_size;
		
		auto len = GetVarint32(p, 5, &key_size);
		p += len;
		lower_bound_ = std::string(p, key_size);
		p += key_size;
		
		len = GetVarint32(p, 5, &key_size);
		p += len;
		upper_bound_ = std::string(p, key_size);

		file_name_ = file_name;
	}

	~File()
	{
		munmap((void*)mmap_, file_size_);
	}

	int Delete()
	{
		return remove(FilePath(file_name_).c_str());		
	}

	int LevelId() const
	{
		return level_id_;
	}

	int FileId() const
	{
		return file_id_;
	}

	std::string LowerBound() const
	{
		return lower_bound_;
	}

	std::string UpperBound() const
	{
		return upper_bound_;
	}

	const char* MMap()
	{
		return mmap_;
	}

	uint32_t FileSize()
	{
		return file_size_;
	}
};

#endif  // FILE_H_
