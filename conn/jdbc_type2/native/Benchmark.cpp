/**************************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2005-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <sys/times.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h> //added by venu
//#include "spthread.h" commented by venu for TSLX
#include <thread_safe_extended.h>
#include <tslxExt.h>
#include "Benchmark.h"

//#define TRACE(func_name,args) { printf("TRACE: %s: ",func_name); printf args; printf("\n"); fflush(stdout); }
#define TRACE(func_name,args)

Benchmark *Benchmark::benchList;
long Benchmark::benchListSize;
struct Benchmark::Thread *Benchmark::threadList;
struct Benchmark::Thread *Benchmark::currentThread;
bool Benchmark::Initalized = false;
JdbcUtil *Benchmark::jdbcUtil;
int Benchmark::jdbcUtilProcessId = -1;
bool Benchmark::errorProcessing = false;

void Benchmark::exitHandler(void)
{
	TRACE("Benchmark::exitHandler",(""));
	// Called once per thread to produce report
	fflush(stdout);
	if (!errorProcessing)
	{
		Thread *thread = threadList;
		while (thread)
		{
			Thread::setCurrentThread(thread->getThreadId());
			Benchmark::Report("End of Program",NULL);
			thread = thread->getNext();
		}
	}
	// Remove from jdbcUtil active processes
	if (jdbcUtilProcessId>=0) jdbcUtil->removeProcess(jdbcUtilProcessId);
}

Benchmark::Benchmark(const char *func_name)
{
	int i;

	// One benchmark object is created for each function/method.
	TRACE("Benchmark::Benchmark",("function name=%s",func_name));
	if (!Initalized)
	{
		TRACE("Benchmark::Benchmark",("Initialized"));
		// One time class initialization
		printf("Warning: Benchmarking active!\n");
		fflush(stdout);
		// Initialize jdbcUtil communications
		jdbcUtil = new JdbcUtil();
		if ((jdbcUtilProcessId=jdbcUtil->addProcess(getpid()))<0)
		{
			printf("Error: Cannot add process to Benchmarking!\n");
			const char *err = jdbcUtil->getError();
			if (err) printf("Error: %s\n",err);
			exit(1);
		}
		benchList = NULL;
		threadList = NULL;
		benchListSize = 0;
		currentThread = NULL;
		atexit(exitHandler);
		Initalized = true;
	}
	// Save function name
	functionName = func_name;
	// Initialize benchmark information
	next = benchList;
	benchList = this;
	benchListSize++;
	timeList = NULL;
	currentTime = NULL;
	for (i=0; i<BENCHMARK_MAX_COUNTERS; i++)
	{
		counter[i] = 0;
		counterName[i] = "";
	}
}

Benchmark::Thread::Thread(unsigned long thread_id)
{
	TRACE("Benchmark::Thread::Thread",("thread_id=%lu",thread_id));
	next = threadList;
	threadList = this;
	threadId = thread_id;
	pendingCmd.cmd = JDBCUTIL_CMD_NOP;
	timerList = NULL;
}


Benchmark::Time::Time(unsigned long thread_id)
{
	TRACE("Benchmark::Time::Time",("thread_id=%lu",thread_id));
	threadId = thread_id;
	totalRealTicks = 0;
	functionRealTicks = 0;
	callCount = 0;
	activeTimers = 0;
}

Benchmark::Timer::Timer(Benchmark *owner_benchmark, Time *owner_time)
{
	TRACE("Benchmark::Timer::Timer",("function name=%s",owner_benchmark->getFunctionName()));
	struct tms curr_tms;
	clock_t curr_ticks = times(&curr_tms);
	totalRealTicksStart = curr_ticks;
	functionRealTicksStart = curr_ticks;
	benchmark = owner_benchmark;
	time = owner_time;
	time->addActiveTimers(1);
}

void Benchmark::Thread::addTimer(Timer *timer)
{
	TRACE("Benchmark::Thread::addTimer",("function name=%s",timer->getBenchmark()->getFunctionName()));
	timer->setNext(timerList);
	timerList = timer;
}

void Benchmark::Thread::setCurrentThread(unsigned long thread_id)
{
	TRACE("Benchmark::Thread::setCurrentThread",("thread id=%lu",thread_id));
	unsigned long threadId;

	if (thread_id) threadId = thread_id;
	else threadId = Benchmark::getThreadId();

	if (!currentThread ||
		(currentThread->threadId != threadId))
	{
		currentThread = threadList;
		while (currentThread &&
			  (currentThread->threadId != threadId))
			currentThread = currentThread->next;
		if (!currentThread)
		{
			currentThread = new Thread(threadId);
			TRACE("Benchmark::Thread::setCurrentThread",("Added new thread=%lu",currentThread->threadId));
		}
	}
	TRACE("Benchmark::Thread::setCurrentThread",("Returning"));
}

void Benchmark::setCurrentTime(unsigned long thread_id)
{
	TRACE("Benchmark::setCurrentTime",("function name=%s thread id=%lu",
		functionName,thread_id));
	unsigned long threadId;

	if (thread_id) threadId = thread_id;
	else threadId = getThreadId();

	if (!currentTime ||
		(currentTime->getThreadId() != threadId))
	{
		currentTime = timeList;
		while (currentTime &&
			  (currentTime->getThreadId() != threadId))
			currentTime = currentTime->getNext();
		if (!currentTime)
		{
			currentTime = new Time(threadId);
			currentTime->setNext(timeList);
			timeList = currentTime;
		}
	}
	TRACE("Benchmark::setCurrentTime",("Returning"));
}

void Benchmark::Time::addFunctionTime(clock_t ticks)
{
	TRACE("Benchmark::Time::addFunctionTime",("ticks=%Ld",ticks));
	functionRealTicks += ticks;
}

void Benchmark::Time::addTotalTime(clock_t ticks)
{
	TRACE("Benchmark::Time::addTotalTime",("ticks=%Ld",ticks));
	totalRealTicks += ticks;
}

void Benchmark::Time::Clear(void)
{
	TRACE("Benchmark::Time::Clear",(""));
	totalRealTicks = 0;
	functionRealTicks = 0;
	callCount = activeTimers;
}

void Benchmark::Time::setRunningTime(Timer *timer, clock_t curr_ticks)
{
	if (timer==NULL)
	{
		TRACE("Benchmark::Time::setRunningTime",("NULL timer."));
		runningTotalRealTicks = totalRealTicks;
		runningFunctionRealTicks = functionRealTicks;
	}
	else
	{
		TRACE("Benchmark::Time::setRunningTime",("function_name=%s",timer->getBenchmark()->getFunctionName()));
		runningTotalRealTicks = curr_ticks - timer->getTotalRealTicksStart() + totalRealTicks;
		clock_t func_ticks_start = timer->getFunctionRealTicksStart();
		if (func_ticks_start) runningFunctionRealTicks = curr_ticks - func_ticks_start + functionRealTicks;
	}
	TRACE("Benchmark::Time::setRunningTime",("functionRealTicks=%Ld, totalRealTicks=%Ld",
		functionRealTicks,totalRealTicks));
	TRACE("Benchmark::Time::setRunningTime",("runningFunctionRealTicks=%Ld, runningTotalRealTicks=%Ld",
		runningFunctionRealTicks,runningTotalRealTicks));
}

void Benchmark::Timer::Pause(void)
{
	TRACE("Benchmark::Timer::Pause",("function_name=%s",benchmark->getFunctionName()));
	struct tms curr_tms;
	clock_t curr_ticks = times(&curr_tms);
	time->addFunctionTime(curr_ticks - functionRealTicksStart);
	functionRealTicksStart = 0;
	TRACE("Benchmark::Timer::Pause",("Returning"));
}

void Benchmark::Timer::Unpause(clock_t curr_ticks)
{
	TRACE("Benchmark::Timer::Unpause",("function_name=%s",benchmark->getFunctionName()));
	functionRealTicksStart = curr_ticks;
}

void Benchmark::Timer::Stop(clock_t curr_ticks)
{
	TRACE("Benchmark::Timer::Stop",("function_name=%s",benchmark->getFunctionName()));
	if (functionRealTicksStart==0) Benchmark::Error("Timer::Stop","Time is paused");
	if (totalRealTicksStart==0) Benchmark::Error("Timer::Stop","Time is stopped");
	time->addFunctionTime(curr_ticks - functionRealTicksStart);
	time->addTotalTime(curr_ticks - totalRealTicksStart);
	functionRealTicksStart = 0;
	totalRealTicksStart = 0;
	time->addActiveTimers(-1);
	TRACE("Benchmark::Timer::Stop",("Returning"));
}

void Benchmark::Thread::Pause(void)
{
	TRACE("Benchmark::Thread::Pause",("thread_id=%lu",threadId));
	if (timerList) timerList->Pause();
	TRACE("Benchmark::Thread::Pause",("Returning"));
}

Benchmark *Benchmark::Thread::Stop(void)
{
	TRACE("Benchmark::Thread::Stop",(""));
	if (timerList==NULL) Benchmark::Error("Thread::Stop","Null timer list");
	struct tms curr_tms;
	clock_t curr_ticks = times(&curr_tms);
	Timer *timer = timerList;
	timerList = timerList->getNext();
	timer->Stop(curr_ticks);
	Benchmark *benchmark = timer->getBenchmark();
	delete timer;
	if (timerList) timerList->Unpause(curr_ticks);
	TRACE("Benchmark::Thread::Stop",("Returning"));
	return benchmark;
}

void Benchmark::Thread::computeRunningTime(clock_t curr_ticks)
{
	TRACE("Benchmark::Thread::computeRunningTime",(""));
	Timer *timer = timerList;
	while (timer)
	{
		timer->getTime()->setRunningTime(timer,curr_ticks);
		timer = timer->getNext();
	}
	TRACE("Benchmark::Thread::computeRunningTime",("Returning"));
}

void Benchmark::Error(const char *method, const char *msg)
{
	errorProcessing = true;
	printf("**** Benchmark Error - Method %s(): %s.\n",method,msg);
	fflush(stdout);
	exit(1);
}

void Benchmark::processCommands(void)
{
	// Process any util commands
	TRACE("Benchmark::processCommands",(""));
	const struct JdbcUtilProcessTableStruct *cmd;
	if (((cmd = jdbcUtil->getCmd())!=NULL) &&
	       (cmd->cmd!=JDBCUTIL_CMD_NOP))
	{
		Thread *thread = threadList;
		while (thread)
		{
			struct JdbcUtilProcessTableStruct *pendingCmd = thread->getPendingCmd();
			if (pendingCmd->cmd == JDBCUTIL_CMD_NOP)
			{
				memcpy(pendingCmd,cmd,sizeof(*pendingCmd));
				TRACE("Benchmark::processCommands",("Command queued for thread %ul",thread->getThreadId()));
			}
			thread = thread->getNext();
		}
	}

	Thread::setCurrentThread(0);
	struct JdbcUtilProcessTableStruct *pendingCmd = currentThread->getPendingCmd();
	switch (pendingCmd->cmd)
	{
		case JDBCUTIL_CMD_BENCHMARK_REPORT:
			Report(pendingCmd->parameter[0],
			       pendingCmd->parameter[1]);
			pendingCmd->cmd = JDBCUTIL_CMD_NOP;
			break;
		case JDBCUTIL_CMD_NOP:
			break;
		default:
			Error("processCommands","Unknown command entered");
	}
	TRACE("Benchmark::processCommands",("Returning"));
}

void Benchmark::Entry(void)
{
	TRACE("Benchmark::Entry",("function name=%s",getFunctionName()));
	unsigned long threadId = getThreadId();

	processCommands();

	setCurrentTime(threadId);
	Thread::setCurrentThread(threadId);
	currentThread->Pause();
	currentThread->addTimer(new Timer(this,currentTime));
	currentTime->addCallCount(1);
	TRACE("Benchmark::Entry",("Returning"));
}

void Benchmark::Exit(void)
{
	TRACE("Benchmark::Exit",("function name=%s",getFunctionName()));
	unsigned long threadId = getThreadId();
	setCurrentTime(threadId);
	Thread::setCurrentThread(threadId);

	if (currentThread->Stop() != this) Error("Benchmark::Exit","Wrong timer active");
	TRACE("Benchmark::Exit",("Returning"));
}

void Benchmark::computeRunningTime(unsigned long thread_id, clock_t curr_ticks)
{
	TRACE("Benchmark::computeRunningTime",(""));
	Benchmark *benchmark = benchList;
	while (benchmark)
	{
		TRACE("Benchmark::computeRunningTime",("function name=%s",benchmark->getFunctionName()));
		benchmark->setCurrentTime(thread_id);
		benchmark->getCurrentTime()->setRunningTime(NULL,curr_ticks);
		benchmark = benchmark->getNext();
	}
    Thread::setCurrentThread(thread_id);
	currentThread->computeRunningTime(curr_ticks);
	TRACE("Benchmark::computeRunningTime",("Returning"));
}

static double benchTime(clock_t ticks)
{
	static double ticksPerSec = 0;
	if (ticksPerSec==0) ticksPerSec = sysconf(_SC_CLK_TCK);
	return(((double) ticks)/ticksPerSec);
}

void Benchmark::BSort(Benchmark **sorted_benchmark_list, long low, long high)
{
	if (low!=high)
		for (long j=high; j>low; j--)
			for (long i=low; i<j; i++)
			{
				if (sorted_benchmark_list[i]->getCurrentTime()->getRunningFunctionRealTicks() >
					sorted_benchmark_list[i+1]->getCurrentTime()->getRunningFunctionRealTicks())
				{
					Benchmark *temp = sorted_benchmark_list[i];
					sorted_benchmark_list[i] = sorted_benchmark_list[i+1];
					sorted_benchmark_list[i+1] = temp;
				}
			}
}

void Benchmark::QSort(Benchmark **sorted_benchmark_list, long plow, long phigh)
{
	// Quicksort
	long low = plow;
	long high = phigh;
	if ((high-low)<=6)
	{
		BSort(sorted_benchmark_list,low,high);
		return;
	}
	long idx = (low+high)/2;
	Benchmark *pivot = sorted_benchmark_list[idx];
	sorted_benchmark_list[idx] = sorted_benchmark_list[high];
	sorted_benchmark_list[high] = pivot;

	while (low<high)
	{
		while ((sorted_benchmark_list[low]->getCurrentTime()->getRunningFunctionRealTicks() <=
			    pivot->getCurrentTime()->getRunningFunctionRealTicks()) &&
			   (low<high)) low++;
		while ((pivot->getCurrentTime()->getRunningFunctionRealTicks() <=
			    sorted_benchmark_list[high]->getCurrentTime()->getRunningFunctionRealTicks()) &&
			   (low<high)) high--;
		if (low<high)
		{
			Benchmark *temp = sorted_benchmark_list[low];
			sorted_benchmark_list[low] = sorted_benchmark_list[high];
			sorted_benchmark_list[high] = temp;
		}
	}
	sorted_benchmark_list[phigh] = sorted_benchmark_list[high];
	sorted_benchmark_list[high] = pivot;
	QSort(sorted_benchmark_list,plow,low-1);
	QSort(sorted_benchmark_list,high+1,phigh);
}

clock_t Benchmark::Report(Benchmark *benchmark, FILE *outFile)
{
	if (benchmark==NULL)
	{
		fprintf(outFile,"Function Time Total Time    Calls Name\n");
		fprintf(outFile,"------------- ------------- ----- ----\n");
		return(0);
	}
	TRACE("Benchmark::Report",("function name=%s",benchmark->getFunctionName()));

	Time *time = benchmark->getCurrentTime();
	unsigned int count = time->getCallCount();
	int i;
	clock_t function_ticks = time->getRunningFunctionRealTicks();
	if (count)
	{
		const char *state = "";
		if (benchmark->currentTime->getActiveTimers()) state = " (Active)";
		fprintf(outFile,"%13.6f %13.6f %5lu %s%s\n",
			benchTime(function_ticks),
			benchTime(time->getRunningTotalRealTicks()),
			count,
			benchmark->getFunctionName(),
			state);
		for (i=0; i<BENCHMARK_MAX_COUNTERS; i++)
			if (benchmark->counter[i] || benchmark->counterName[i][0])
				fprintf(outFile,"                                    Counter %d (%s): %ld\n",
					i,benchmark->counterName[i],benchmark->counter[i]);
	}
	time->Clear();
	return(function_ticks);
}

void Benchmark::Report(const char *title, const char *dirname)
{
	TRACE("Benchmark::Report",(""));
	FILE *outFile = NULL;
	char filename[256];

	time_t curr_time = time(NULL);
	struct tm local_time;
	struct tm *plocaltime=NULL;
	plocaltime=localtime_r(&curr_time,&local_time);

	struct tms curr_tms;
	clock_t curr_ticks = times(&curr_tms);

	computeRunningTime(currentThread->getThreadId(), curr_ticks);
	filename[0] = 0;
	if (dirname &&
		dirname[0] &&
		((strlen(dirname)+27) <= sizeof(filename)))
	{
		mkdir(dirname,0777);
		sprintf(filename,"%s/%lu_%lu.txt",dirname,getpid(),currentThread->getThreadId());
		if ((outFile=fopen(filename,"a"))==NULL)
		{
			printf("**** Cannot open log file %s ****\n",filename);
			filename[0] = 0;
		}
	}

	if (filename[0]==0) outFile = stdout;

	if(plocaltime)
		fprintf(outFile,"------------------------------ %02d/%02d/%04d %02d:%02d:%02d ------------------------------\n",
				  local_time.tm_mon+1,
				  local_time.tm_mday,
				  local_time.tm_year+1900,
				  local_time.tm_hour,
				  local_time.tm_min,
				  local_time.tm_sec);
	else
	fprintf(outFile,"------------------------------ %02d/%02d/%04d %02d:%02d:%02d ------------------------------\n",
			0,
			0,
			0,
			0,
			0,
			0);

	if (title) fprintf(outFile,"Benchmark Report: %s\n",title);
	fprintf(outFile,"Process Id: %u  Thread Id: %lu\n",getpid(),currentThread->getThreadId());
	Report(NULL,outFile);
	clock_t totalFunctionTicks = 0;
	Benchmark **sortedBenchmark = NULL;
	if (benchListSize)
	{
		Benchmark *benchmark = benchList;
		sortedBenchmark = new Benchmark *[benchListSize];
		for (int i=0; i<benchListSize; i++)
		{
			sortedBenchmark[i] = benchmark;
			benchmark = benchmark->next;
		}
		QSort(sortedBenchmark,0,benchListSize-1);
		for (int i=benchListSize-1; i>=0; i--)
			totalFunctionTicks += Report(sortedBenchmark[i],outFile);
	}
	fprintf(outFile,"------------------------------------------------------------------------------\n");
	fprintf(outFile,"Total benchmarked function time: %11.6f\n",benchTime(totalFunctionTicks));
	fprintf(outFile,"Total benchmarked functions: %ld\n",benchListSize);
	fprintf(outFile,"------------------------------------------------------------------------------\n");
	if (sortedBenchmark) delete sortedBenchmark;
	if (filename[0]) fclose(outFile);
	else fflush(outFile);
}

unsigned long Benchmark::getThreadId(void)
{
	_TSLX_t thread_id = tslx_ext_pthread_self();

	return((unsigned long) thread_id.field1);
}

void Benchmark::addCounter(int counter_index, const char *name, long value)
{
	if (counter_index>=BENCHMARK_MAX_COUNTERS)
	{
		printf("Benchmark: '%s' Invalid counter index.\n",getFunctionName());
		exit(1);
	}
	counter[counter_index] += value;
	if (name) counterName[counter_index] = name;
}
