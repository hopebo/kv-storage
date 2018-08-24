// Copyright (c) 2018, Bo Li(hopelee1994@gmail.com). All rights reserved.
// Use of this source code is governed by the BSD 3-Clause License,
// that can be found in the LICENSE file.

#ifndef CPU_MONITOR_H_
#define CPU_MONITOR_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SKIP_ITEMS 13

struct Total_Cpu_Occupy_t
{
	unsigned long user;
	unsigned long nice;
	unsigned long system;
	unsigned long idle;
	unsigned long Sum() { return user + nice + system + idle; }
};

struct Proc_Cpu_Occupy_t
{
	unsigned int pid;
	unsigned long utime;	// user time
	unsigned long stime;	// kernel time
	unsigned long cutime;	// all dead threads' user time
	unsigned long cstime;	// all dead threads' kernel time
	unsigned long Sum() { return utime + stime + cutime + cstime; }
};

class CpuMonitor
{
private:
	Total_Cpu_Occupy_t total_start_, total_end_;
	Proc_Cpu_Occupy_t proc_start_, proc_end_;
	int pid_;
	Total_Cpu_Occupy_t GetCpuTotalOccupy()
	{
		FILE* fd;
		char buff[1024];
		Total_Cpu_Occupy_t t;

		fd = fopen("/proc/stat", "r");
		if (fd == nullptr)
		{
			printf("Opening /proc/stat Failed.\n");
			exit(1);
		}

		fgets(buff, sizeof(buff), fd);
		char name[64];
		sscanf(buff, "%s %ld %ld %ld %ld", name, &t.user, &t.nice, &t.system, &t.idle);
		fclose(fd);

		return t;
	}

	Proc_Cpu_Occupy_t GetCpuProcOccupy()
	{
		char file_name[64];
		Proc_Cpu_Occupy_t t;
		FILE* fd;
		char line_buff[1024];
		sprintf(file_name, "/proc/%d/stat", pid_);

		fd = fopen(file_name, "r");
		if (fd == nullptr)
		{
			printf("Opening /proc/<pid>/stat Failed.\n");
			exit(1);
		}

		fgets(line_buff, sizeof(line_buff), fd);
		
		char* p = line_buff;
		int len = strlen(p);
		int count = 0;
		for (int i = 0; i < len; ++i, ++p)
		{
			if (*p == ' ')
			{
				if (++count == SKIP_ITEMS)
				{
					break;
				}
			}
		}

		++p;
		sscanf(p, "%ld %ld %ld %ld", &t.utime, &t.stime, &t.cutime, &t.cstime);
		fclose(fd);

		return t;
	}

public:
	CpuMonitor(int pid) : pid_(pid) { }

	void RecordStart()
	{
		total_start_ = GetCpuTotalOccupy();
		proc_start_ = GetCpuProcOccupy();
	}

	void RecordEnd()
	{
		total_end_ = GetCpuTotalOccupy();
		proc_end_ = GetCpuProcOccupy();
	}

	double CpuOccupyRatio()
	{
		return ((double)(proc_end_.Sum() - proc_start_.Sum())) / (total_end_.Sum() - total_start_.Sum());
	}
};

#endif  // CPU_MONITOR_H_
