/**************************************************************************
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
**************************************************************************/
#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <stdio.h>
#include <sys/times.h>
#include <sys/types.h>
#include "jdbcUtil.h"

#define FUNCTION_ENTRY(function_name, args) static Benchmark *bench=NULL; if (bench==NULL) bench = new Benchmark(function_name); bench->Entry();
#define FUNCTION_ENTRY_LEVEL(level,function_name,params) FUNCTION_ENTRY(function_name,params)
#define FUNCTION_ENTRY_LEVEL_LOC(level,function_name,params,filename,line) FUNCTION_ENTRY(function_name,params)
#define FUNCTION_RETURN_NUMERIC(rc,comment) { bench->Exit(); return(rc); }
#define FUNCTION_RETURN_NUMERIC_LOC(rc,comment,fileanme,line) FUNCTION_RETURN_NUMERIC(rc,comment)
#define FUNCTION_RETURN_INT64(rc,comment) FUNCTION_RETURN_NUMERIC(rc,comment)
#define FUNCTION_RETURN_INT64_LOC(rc,comment,fileanme,line) FUNCTION_RETURN_INT64(rc,comment)
#define FUNCTION_RETURN_FLOAT(rc,comment) FUNCTION_RETURN_NUMERIC(rc,comment)
#define FUNCTION_RETURN_FLOAT_LOC(rc,comment,fileanme,line) FUNCTION_RETURN_FLOAT(rc,comment)
#define FUNCTION_RETURN_DOUBLE(rc,comment) FUNCTION_RETURN_NUMERIC(rc,comment)
#define FUNCTION_RETURN_DOUBLE_LOC(rc,comment,fileanme,line) FUNCTION_RETURN_DOUBLE(rc,comment)
#define FUNCTION_RETURN_PTR(rc,comment) FUNCTION_RETURN_NUMERIC(rc,comment)
#define FUNCTION_RETURN_PTR_LOC(rc,comment,fileanme,line) FUNCTION_RETURN_PTR(rc,comment)
#define FUNCTION_RETURN_VOID(comment) { bench->Exit(); return; }
#define FUNCTION_RETURN_VOID_LOC(comment,fileanme,line) FUNCTION_RETURN_VOID(comment)
#define FUNCTION_RETURN_TYPE(type_str,rc,comment) FUNCTION_RETURN_NUMERIC(rc,comment)
#define CLI_DEBUG_RETURN_SQL(code) FUNCTION_RETURN_NUMERIC(code,(NULL))
#define CLI_DEBUG_RETURN_SQL_LOC(code,filename,line) FUNCTION_RETURN_NUMERIC(code,(NULL))

#define BENCHMARK_ADD_COUNTER(index,name,value) bench->addCounter(index,name,value)

#define BENCHMARK_MAX_COUNTERS 10

class Benchmark
{
private:

	class Time;

	class Timer
	{
	private:
		clock_t totalRealTicksStart;
		clock_t functionRealTicksStart;
		Time *time;
		Benchmark *benchmark;
		Timer *next;
	public:
		Timer(Benchmark *owner_benchmark, Time *owner_time);
		void Pause(void);
		void Unpause(clock_t curr_ticks);
		void Stop(clock_t curr_ticks);
		clock_t getTotalRealTicksStart(void) { return totalRealTicksStart; };
		clock_t getFunctionRealTicksStart(void) { return functionRealTicksStart; };
		Timer *getNext(void) { return next; };
		void setNext(Timer *timer) { next = timer; };
		Benchmark *getBenchmark(void) { return benchmark; };
		Time *getTime(void) { return time; };
	};

	class Time
	{
	private:
		unsigned long threadId;
		unsigned long callCount;
		clock_t totalRealTicks;
		clock_t functionRealTicks;
		clock_t runningTotalRealTicks;
		clock_t runningFunctionRealTicks;
		int activeTimers;
		Time *next;
	public:
		Time(unsigned long thread_id);
		void setRunningTime(Timer *timer, clock_t curr_ticks);
		void addFunctionTime(clock_t ticks);
		void addTotalTime(clock_t ticks);
		void addActiveTimers(int offset) { activeTimers += offset; }; 
		void addCallCount(int offset) { callCount += offset; }; 
		unsigned long getThreadId(void) { return threadId; };
		Time *getNext(void) { return next; };
		void setNext(Time *next_time) { next = next_time; };
		clock_t getRunningFunctionRealTicks(void) { return runningFunctionRealTicks; };
		clock_t getRunningTotalRealTicks(void) { return runningTotalRealTicks; };
		clock_t getCallCount(void) { return callCount; };
		int getActiveTimers(void) { return activeTimers; }; 
		void Clear(void);
	};

	class Thread
	{
	private:
		unsigned long threadId;
		Timer *timerList;
		struct JdbcUtilProcessTableStruct pendingCmd;
		Thread *next;
	public:
		Thread(unsigned long thead_id);
		static void setCurrentThread(unsigned long thread_id);
		void Pause(void);
		Benchmark *Stop(void);
		struct JdbcUtilProcessTableStruct *getPendingCmd(void) { return &pendingCmd; };
		Thread *getNext(void) { return next; };
		void addTimer(Timer *timer);
		void computeRunningTime(clock_t curr_ticks);
		unsigned int getThreadId(void) { return threadId; };
	};

	static Thread *threadList;
	static Thread *currentThread;
	static Benchmark *benchList;
	static long benchListSize;
	static bool Initalized;
	static JdbcUtil *jdbcUtil;
	static bool errorProcessing;
	static int jdbcUtilProcessId;

	const char *functionName;
	long counter[BENCHMARK_MAX_COUNTERS];
	const char *counterName[BENCHMARK_MAX_COUNTERS];
	Time *timeList;
	Time *currentTime;
	Benchmark *next;


	static void exitHandler(void);
	static void computeRunningTime(unsigned long thread_id, clock_t curr_ticks);
	static void BSort(Benchmark **sorted_benchmark_list, long low, long high);
	static void QSort(Benchmark **sorted_benchmark_list, long plow, long phigh);
	static clock_t Benchmark::Report(Benchmark *benchmark, FILE *outFile);
	static unsigned long getThreadId(void);

	void setCurrentTime(unsigned long thread_id);
	Time *getCurrentTime(void) { return currentTime; };
	void processCommands(void);
	Benchmark *getNext(void) { return next; };

public:
	Benchmark(const char *func_name);
	static void Error(const char *method, const char *msg);
	void Entry(void);
	void Exit(void);
	static void Report(const char *title, const char *dirname);
	const char *getFunctionName(void) { if (functionName) return functionName; return ""; };
	void addCounter(int counter_index, const char *name, long value);
};
#endif /* BENCHMARK_H */