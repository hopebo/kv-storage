// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef MEMORY_MONITOR_H_
#define MEMORY_MONITOR_H_

#include <map>
#include <vector>
#include <thread>

#include <unistd.h>

struct MemInfo
{
	unsigned long vm_size;
	unsigned long vm_rss;
	MemInfo(unsigned long size, unsigned long rss) : vm_size(size), vm_rss(rss) { }
};

class MemoryMonitor
{
private:
	std::vector<MemInfo> records_;
	enum MemType { VmSize, VmRss };
	std::map<MemType, int> vm_line_;
	unsigned int pid_;
	bool is_stop_ = false;
	std::thread thread_;

	unsigned int GetProcMem(MemType mem_type)
	{
		char file_name[64];
		FILE* fd;
		char line_buff[512];
		sprintf(file_name, "/proc/%d/status", pid_);

		fd = fopen(file_name, "r");
		if (fd == nullptr)
		{
			printf("Opening /proc/<pid>/status File Failed.\n");
			exit(1);
		}

		char name[64];
		int ret;
		for (int i = 0; i < vm_line_[mem_type] - 1; ++i)
		{
			fgets(line_buff, sizeof(line_buff), fd);
		}

		fgets(line_buff, sizeof(line_buff), fd);
		sscanf(line_buff, "%s %d", name, &ret);
		fclose(fd);

		return ret;
	}

public:
	MemoryMonitor(unsigned int pid) : pid_(pid)
	{
		vm_line_[VmSize] = 15;
		vm_line_[VmRss] = 19;	
	};

	void Record()
	{
		auto vm_size = GetProcMem(VmSize);
		auto vm_rss = GetProcMem(VmRss);
		records_.push_back(MemInfo(vm_size, vm_rss));	
	}

	void BackgroundRecord()
	{
		while (!is_stop_)
		{
			usleep(100000);
			Record();
		}
	}

	void AutoRecord()
	{
		thread_ = std::thread(&MemoryMonitor::BackgroundRecord, this);
	}

	void Stop()
	{
		is_stop_ = true;
		thread_.join();
	}

	void PrintRecord(FILE* stream)
	{
		for (auto& record : records_)
		{
			fprintf(stream, "VmSize: %d, VmRss: %d\n", record.vm_size, record.vm_rss);
		}
	}
};

#endif  // MEMORY_MONITOR_H_
