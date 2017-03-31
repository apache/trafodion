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
//
// Debug Routines
//
// See header for use.
//
// Debug routines in this file are thread safe, except for the debug buffering.
//   There is only a small window where DebugNextLine() is not thread safe.
//   Later, locks could be added.

// The routines where written to allocate most string buffers at load time (static).
//   This means that the debug routines take up more space initially and the strings
//     are fixed length, but it avoids dynamic memory allocations at run time to be less
//     intrusive.
//   This also means that when buffering debug output is selected, a large chunk of
//     memory is allocated during initialization.

#include "Debug.h"
#include "org_apache_trafodion_jdbc_t2_DataWrapper.h"

#ifdef _DEBUG

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#ifdef NSK_PLATFORM
	#include <sqlWin.h>
#else
	#include <sql.h>
#endif
#include "CoreCommon.h"
#include "SrvrCommon.h"
#include "SrvrKds.h"
#include "SqlInterface.h"
//#include "spthread.h" commented by Venu for TSLX

#include "jni.h"

#include "Vproc.h"

// Maximum output line length
#define DEBUG_MAX_LINE_LEN 1024UL

#ifndef FALSE
#define TRUE  1
#define FALSE 0
#endif

struct MemoryAllocStruct {
	void *mem_ptr;
	bool perm;
	char alloc_type;
	unsigned long alloc_size;
	const char *filename;
	unsigned long line;
	unsigned long size;
};
static struct MemoryAllocStruct *MemAlloc;
static unsigned long totalMemAlloc = 0;
static unsigned long freeMemAllocIdx = 0;

static size_t debugBufferSize = 0;
static char debugInitialized = FALSE;
static char *debugLine = NULL;
static bool debugBufferEmpty = TRUE;
static size_t debugLineHead;
static size_t debugLineTail;

static char functionReturnMessage[2048];
static unsigned long debugLevel;

struct CallStackStruct
{
	const char *functionName;
	unsigned long level;
	bool returnProcessed;
	struct CallStackStruct *next;
};

struct DebugInfoStruct
{
	// Thread independent variables
	unsigned long threadId;
	// Current debug level set by developer through env variable
	struct CallStackStruct *callStack;
	long callStackSize;
	struct DebugInfoStruct *next;
};

static struct DebugInfoStruct *debugInfo = NULL;
static struct DebugInfoStruct *currentDebugInfo = NULL;
static struct CallStackStruct *currentCallStack = NULL;

// Table of valid Debug Levels.
// Enter new levels and the text user will use in env variable
static struct {
	unsigned long level;
	const char *text;
} levelInfo[] = {
	{ DEBUG_FLAG_THREADS,	"Threads" },
	{ DEBUG_FLAG_DIRTY_MEM,	"DirtyMem" },
	{ DEBUG_FLAG_TIMER,		"Timer" },
	{ DEBUG_FLAG_NOTIME,	"NoTime" },
	{ DEBUG_FLAG_TESTWARE,	"Testware" },
	{ DEBUG_LEVEL_ENTRY,	"Entry" },
	{ DEBUG_LEVEL_JAVA,		"Java" },
	{ DEBUG_LEVEL_MEM,		"Mem" },
	{ DEBUG_LEVEL_MEMLEAK,	"MemLeak" },
	{ DEBUG_LEVEL_CLI,		"Cli" },
	{ DEBUG_LEVEL_DATA,		"Data" },
	{ DEBUG_LEVEL_TXN,		"Txn" },
	{ DEBUG_LEVEL_ROWSET,	"Rowset" },
	{ DEBUG_LEVEL_ERROR,	"Error" },
	{ DEBUG_LEVEL_METADATA,	"Metadata" },
	{ DEBUG_LEVEL_UNICODE,	"Unicode" },
	{ DEBUG_LEVEL_POOLING,	"Pooling" },
	{ DEBUG_LEVEL_STMT,		"Stmt" },
	{ DEBUG_LEVEL_ALL,		"All" },
	{ 0, NULL } };

static const char *Basename(const char *filename)
{
	// Returns the base name of the full file name
	size_t base_idx=0,idx;

	if (filename==NULL) return("");

	for (idx=0; filename[idx]; idx++)
	{
		if (filename[idx]=='\\') base_idx = idx+1;
	}
	return(filename+base_idx);
}

static const char *UCase(const char *text)
{
	// Returns a string that is all upper case
	static char *rc = NULL;
	static size_t rc_len = 0;
	size_t idx = 0;
	if (text==NULL) return(NULL);
	size_t len = strlen(text);
	if ((rc==NULL) || (len>rc_len))
	{
		if (rc) delete rc;
		rc = new char[len+1];
		rc_len = len;
	}
	while (text[idx])
	{
		rc[idx] = toupper(text[idx]);
		idx++;
	}
	rc[idx] = 0;
	return(rc);
}

static unsigned long GetThreadId(void)
{
        return syscall(SYS_gettid);
}

static const char *GetId(void)
{
	// Returns a string for the id of the thread.
	// We do not use process id since always the same on NSK.
	// If they have not requested threads, we return blank
	//   to save space on output line.
	static char rc[80];
	if (debugLevel & DEBUG_FLAG_THREADS) sprintf(rc,"[%lu] ",GetThreadId());
	else rc[0] = 0;
	return(rc);
}


static unsigned long FindMemoryIdx(void *alloc_ptr)
{
	// Searches the memory list for the pointer and returns the index.
	// It also updates where a free entry in the list is for later use.

	unsigned long idx=0,found_idx = totalMemAlloc;

	// Search each entry until found or not.
	while ((idx<totalMemAlloc) && (found_idx==totalMemAlloc))
	{
		// If free entry, remember where it is.
		if (MemAlloc[idx].mem_ptr==NULL)
		{
			freeMemAllocIdx = idx;
			// If alloc_ptr is NULL, then it was looking for a free one.
			if (alloc_ptr==NULL) found_idx = idx;
		} else if (MemAlloc[idx].mem_ptr==alloc_ptr) found_idx = idx;
		idx++;
	}
	return(found_idx);
}

static unsigned long FindFreeMemoryIdx(const char *filename, unsigned long line)
{
	// If the freeMemAllocIdx is pointing to a free entry, use it.
	if ((freeMemAllocIdx!=totalMemAlloc) && (MemAlloc[freeMemAllocIdx].mem_ptr==NULL)) return(freeMemAllocIdx);
	// Otherwise, we need to search for a new free entry
	unsigned long idx = FindMemoryIdx(NULL);

	// If the list is full, allocate more entries
	if (idx==totalMemAlloc)
	{
		// Allocate a larger list
		const int EXPAND_SIZE = 100;

		// Allocate the new list
		struct MemoryAllocStruct *new_list = new struct MemoryAllocStruct[totalMemAlloc+EXPAND_SIZE];

		// Check for out of memory
		if (new_list==NULL)
		{
			DebugOutput("Out of Memory",filename,line);
			exit(1);
		}

		// Zero out the new entries
        memset(new_list+totalMemAlloc,0,EXPAND_SIZE * sizeof(MemAlloc[0]));

		// Copy in the old list
		if (totalMemAlloc)
		{
			memcpy(new_list,MemAlloc,totalMemAlloc*sizeof(MemAlloc[0]));
			delete MemAlloc;
		}
		MemAlloc = new_list;
		// Use the first new entry as the free one.
		idx = totalMemAlloc;
		// Expand the size
		totalMemAlloc += EXPAND_SIZE;
	}
	// Return the free entry
	return(idx);
}

static void CheckDebugInfo(bool call_stack_required, const char *filename, unsigned long line)
{
	unsigned long threadId = GetThreadId();

	if (!currentDebugInfo || (currentDebugInfo->threadId != threadId))
	{
		// Another thread
		// See if this is a new thread
		currentDebugInfo = debugInfo;
		while (currentDebugInfo && (currentDebugInfo->threadId != threadId))
			currentDebugInfo = currentDebugInfo->next;

		if (currentDebugInfo==NULL)
		{
			// We have a new thread
			currentDebugInfo = new struct DebugInfoStruct;
			currentDebugInfo->threadId = threadId;
			currentDebugInfo->callStackSize = 0;
			currentDebugInfo->callStack = NULL;
			currentDebugInfo->next = debugInfo;
			debugInfo = currentDebugInfo;
		}
	}
	currentCallStack = currentDebugInfo->callStack;
	if (call_stack_required && !currentCallStack)
	{
		if (filename) printf("Call stack is NULL in %s at line %ul\n",filename,line);
		else printf("Call stack is NULL\n");
		exit(1);
	}
}

static void PopCallStack(void)
{
	currentCallStack = currentDebugInfo->callStack->next;
	delete currentDebugInfo->callStack;
	currentDebugInfo->callStack = currentCallStack;
	currentDebugInfo->callStackSize--;
}

void DebugFunctionEntry(const char *function_name, unsigned long level,
						const char *params,
                        const char *filename, unsigned long line)
{
	// Called everytime a function is entered
	if (!function_name)
	{
		DebugOutput("Function Entry Error: function name is NULL",filename,line);
		exit(1);
	}
	CheckDebugInfo(false,filename,line);
	currentCallStack = new struct CallStackStruct;
	currentCallStack->functionName = function_name;
	currentCallStack->returnProcessed = false;
	currentCallStack->level = level;
	currentCallStack->next = currentDebugInfo->callStack;
	currentDebugInfo->callStack = currentCallStack;
	currentDebugInfo->callStackSize++;
	if (DebugActive(DEBUG_LEVEL_ENTRY|currentCallStack->level,filename,line))
	{
		if (params) sprintf(functionReturnMessage,"ENTERING FUNCTION (%s)",params);
		else strcpy(functionReturnMessage,"ENTERING FUNCTION");
		DebugOutput(functionReturnMessage,filename,line);
	}
}

void DebugFunctionReturnNumeric(long rc, const char *msg, bool pop_stack,
						        const char *filename, unsigned long line)
{
	// Called for returning an integral value on function exit
	CheckDebugInfo(true,filename,line);
	if (!currentCallStack->returnProcessed)
	{
		if (DebugActive(DEBUG_LEVEL_ENTRY|currentCallStack->level,filename,line))
		{
			if (msg) sprintf(functionReturnMessage,"RETURNING FROM FUNCTION Return %ld - %s",rc,msg);
			else sprintf(functionReturnMessage,"RETURNING FROM FUNCTION Return=%ld",rc);
			DebugOutput(functionReturnMessage,filename,line);
		}
		currentCallStack->returnProcessed = true;
	}
	if (pop_stack) PopCallStack();
}
void DebugFunctionReturnInt64(long long rc, const char *msg, bool pop_stack,
						        const char *filename, unsigned long line)
{
	// Called for returning an integral value on function exit
	CheckDebugInfo(true,filename,line);
	if (!currentCallStack->returnProcessed)
	{
		if (DebugActive(DEBUG_LEVEL_ENTRY|currentCallStack->level,filename,line))
		{
			if (msg) sprintf(functionReturnMessage,"RETURNING FROM FUNCTION Return %Ld - %s",rc,msg);
			else sprintf(functionReturnMessage,"RETURNING FROM FUNCTION Return=%Ld",rc);
			DebugOutput(functionReturnMessage,filename,line);
		}
		currentCallStack->returnProcessed = true;
	}
	if (pop_stack) PopCallStack();
}

void DebugFunctionReturnDouble(double rc, const char *msg, bool pop_stack,
						        const char *filename, unsigned long line)
{
	// Called for returning an integral value on function exit
	CheckDebugInfo(true,filename,line);
	if (!currentCallStack->returnProcessed)
	{
		if (DebugActive(DEBUG_LEVEL_ENTRY|currentCallStack->level,filename,line))
		{
			if (msg) sprintf(functionReturnMessage,"RETURNING FROM FUNCTION Return %g - %s",rc,msg);
			else sprintf(functionReturnMessage,"RETURNING FROM FUNCTION Return=%g",rc);
			DebugOutput(functionReturnMessage,filename,line);
		}
		currentCallStack->returnProcessed = true;
	}
	if (pop_stack) PopCallStack();
}

void DebugFunctionReturnPtr(void *rc, const char *msg, bool pop_stack,
                            const char *filename, unsigned long line)
{
	// Called for returning a pointer value on function exit
	CheckDebugInfo(true,filename,line);
	if (!currentCallStack->returnProcessed)
	{
		if (DebugActive(DEBUG_LEVEL_ENTRY|currentCallStack->level,filename,line))
		{
			if (msg) sprintf(functionReturnMessage,"RETURNING FROM FUNCTION Return 0x%08x - %s",rc,msg);
			else sprintf(functionReturnMessage,"RETURNING FROM FUNCTION Return=0x%08x",rc);
			DebugOutput(functionReturnMessage,filename,line);
		}
		currentCallStack->returnProcessed = true;
	}
	if (pop_stack) PopCallStack();
}

void DebugFunctionReturn(const char *tag, const char *comment,
						 bool pop_stack, const char *call_type,
						 const char *filename, unsigned long line)

{
	// Called for returning from a void function on function exit
	CheckDebugInfo(true,filename,line);
	if (!currentCallStack->returnProcessed)
	{
		if (DebugActive(DEBUG_LEVEL_ENTRY|currentCallStack->level,filename,line))
		{
			if (tag) sprintf(functionReturnMessage,"%s FROM %s FUNCTION",call_type,tag);
			else sprintf(functionReturnMessage,"%s FROM FUNCTION",call_type);
			if (comment)
			{
				strcat(functionReturnMessage," - ");
				strcat(functionReturnMessage,comment);
			}
			DebugOutput(functionReturnMessage,filename,line);
		}
		currentCallStack->returnProcessed = true;
	}
	if (pop_stack) PopCallStack();
}

void DebugPrintBuffer(void)
{
	// Prints the debug buffer (if set)
	if (debugBufferSize)
	{
		// Loop through the ring buffer and print the lines
		printf("------------------- DEBUG LOG START -----------------------\n");
		if (!debugBufferEmpty)
		{
			size_t idx = debugLineHead;
			do {
				printf("%s\n",&debugLine[idx*(DEBUG_MAX_LINE_LEN+1)]);
				if (++idx==debugBufferSize)
				{
					/* Wrap buffer */
					idx = 0;
				}
			} while (idx!=debugLineTail); 
		}
		printf("-------------------- DEBUG LOG END ------------------------\n");
		fflush(stdout);
		debugBufferEmpty = TRUE;
	}
}

static char *NextDebugLine(void)
{
	// Return the next line in the debug buffer
	// Even if buffering not set, there is at least one line allocated.
	static char *line = NULL;
	char *rc;

	// If empty, init the head and tail pointers for allocation
	if (debugBufferEmpty)
	{
		debugLineHead = 1;
		debugLineTail = 0;
	}

	// Get the line at the tail for the caller
	rc = &debugLine[debugLineTail*(DEBUG_MAX_LINE_LEN+1)];

	// Update the head and tail pointers
	if (debugLineTail==debugLineHead)
	{
		// Buffer full.  Move the head pointer to overwrite the line.
		if (++debugLineHead>=debugBufferSize)
		{
			// Wrap buffer pointer
			debugLineHead = 0;
		}
	}

	if (++debugLineTail>=debugBufferSize)
	{
		// Wrap buffer pointer
		debugLineTail = 0;
	}

	if (debugBufferEmpty)
	{
		// Buffer was empty so now setup the first line
		debugLineHead = 0;
		debugBufferEmpty = FALSE;
	}
	return(rc);
}

static const char *DebugLevelString(void)
{
	// Returns a string of all the debug levels set.
	static char rc[80];
	int idx;
	bool first = TRUE;

	strcpy(rc,"Active Debug Levels: ");

	for (idx=0; levelInfo[idx].text; idx++)
	{
		if ((levelInfo[idx].level!=DEBUG_LEVEL_ALL) && (debugLevel & levelInfo[idx].level))
		{
			if (first) first = FALSE;
			else strcat(rc,",");
			strcat(rc,levelInfo[idx].text);
		}
	}
	return(rc);
}

static void DebugExit(void)
{
	// Called at program exit
	// Always dump the buffer if buffering the debug lines.
	if (debugBufferSize) DebugPrintBuffer();
	// Print the final memory allocation information if needed
	DebugPrintMemoryAlloc(__FILE__,__LINE__);
	// Check for missmatched function calls
	struct DebugInfoStruct *currDebugInfo = debugInfo;
	while (currDebugInfo)
	{
        struct CallStackStruct *currCallStack = currDebugInfo->callStack;
		while (currCallStack)
		{
			char text[256];
			sprintf(text,"Call Stack not empty at exit for function %s()",
				currCallStack->functionName);
			DebugOutput(text,__FILE__,__LINE__);
			currCallStack = currCallStack->next;
		}
		currDebugInfo = currDebugInfo->next;
	}
}

static const char *Timestamp(bool force)
{
	static char timestamp[80];

	if (!force && (debugLevel & DEBUG_FLAG_NOTIME)) return("");

	if (!force && (debugLevel & DEBUG_FLAG_TIMER))
	{
		struct tms time_buf;
		double timer = times(&time_buf)/1000000.0;
		sprintf(timestamp,"%11.6f ",timer);
	}
	else
	{
		time_t curr_time = time(NULL);
		struct tm local_time;
		if(localtime_r(&curr_time,&local_time))
			sprintf(timestamp,"%02d/%02d/%04d %02d:%02d:%02d ",
					  local_time.tm_mon+1,
					  local_time.tm_mday,
					  local_time.tm_year+1900,
					  local_time.tm_hour,
					  local_time.tm_min,
					  local_time.tm_sec);
		else
			sprintf(timestamp,"%02d/%02d/%04d %02d:%02d:%02d ",
					  0,
					  0,
					  0,
					  0,
					  0,
					  0);
	}
	return(timestamp);
}


static void InitDebug(void)
{
	// Initialize the debug system.  Called when first debug macro is called.

	// Get the environment variable for debug level and set the levels.
	const char *debug_level_str = getenv("JDBC_DEBUG_LEVEL");
	size_t size;

	if (debug_level_str) {
		char opt[80];
		size_t src_idx=0,opt_idx=0;
		while (debug_level_str[src_idx])
		{
			// String is comma delimited
			while (debug_level_str[src_idx] &&
				   debug_level_str[src_idx]!=',')
			{
				if (debug_level_str[src_idx]!=' ')
				{
					if (opt_idx==(sizeof(opt)-1))
					{
						// Ignore too long of debug level
						opt_idx--;
					}
					opt[opt_idx++] = toupper(debug_level_str[src_idx]);
				}
				src_idx++;
			}
			if (opt_idx)
			{
				// We have a debug level
				opt[opt_idx] = 0;
				int idx = 0;
				while (levelInfo[idx].text &&
				       (strcmp(opt,UCase(levelInfo[idx].text))!=0)) idx++;
				if (levelInfo[idx].text)
				{
					// Found
					debugLevel |= levelInfo[idx].level;
				}
				else
				{
					printf("Warrning: Debug level '%s' ignored.\n",opt);
				}
				opt_idx = 0;
			}
			if (debug_level_str[src_idx]) src_idx++;
		}
	}

	// Get the environment variable for buffer size.
	const char *debug_buffer_size_str = getenv("JDBC_DEBUG_BUFFER_LINES");
	if (debug_buffer_size_str) debugBufferSize = atol(debug_buffer_size_str);
	else debugBufferSize = 0;

	// Compute the size of the buffer.  Lines * line length.  Always allocate at lease one line.
	if (debugBufferSize) size = debugBufferSize * (DEBUG_MAX_LINE_LEN+1);
	else size = DEBUG_MAX_LINE_LEN + 1;
	
	debugLine = new char[size];
	if (debugLine==NULL)
	{
		printf("Out of Memory.  Tried to allocate %ld bytes)\n",size);
		exit(1);
	}
	
	// Force the initalization of the memory list.
	FindFreeMemoryIdx(__FILE__,__LINE__);

	// Setup for exit processing
	atexit(DebugExit);

	debugInitialized = TRUE;
	printf("%s: DEBUG ACTIVE - %s\n",
		Timestamp(true),
		DebugLevelString());
	printf("%s: VPROC: %s\n",
		Timestamp(true),
		driverVproc);
	fflush(stdout);
}

const char *DebugFormat(const char *fmt, ...)
{
	// Formats a printf argument into an internal buffer.
	static char buffer[2048];

	if (fmt==NULL) return(NULL);

	va_list marker;
	va_start( marker, fmt );     /* Initialize variable arguments. */
	vsprintf(buffer, fmt, marker);
	va_end( marker );            /* Reset variable arguments.      */
	return(buffer);
}

bool DebugActive(unsigned long debug_level,
				 const char *filename, unsigned long line)
{
	// Outputs debug line if debug level selected.
	if (!debugInitialized) InitDebug();

	// See if level selected
	CheckDebugInfo(false,filename,line);

	// See if level active
	if ((debug_level & debugLevel) ||
		(debug_level==DEBUG_LEVEL_ALL))
		return(true);
	return(false);
}

void DebugOutput(const char *msg,
				 const char *filename, unsigned long line)
{
	// Build first part of debug output line
	size_t msg_len,prolog_len;
	char prolog[256];
	char *out_line = NextDebugLine();
	char location[256];
	if (filename) sprintf(location,"%s(%ld)",Basename(filename),line);
	else location[0] = 0;

	const char *function_name;
	if (currentCallStack) function_name = currentCallStack->functionName;
	else function_name = "...";
	sprintf(prolog,"%s%02ld %s%s() %s: ",
		Timestamp(false),
		currentDebugInfo->callStackSize,
		GetId(),
		function_name,
		location);

	// Append the user message
	if (msg) msg_len = strlen(msg);
	else msg_len = 0;
	prolog_len = strlen(prolog);
	// Truncate the message if too long of line
	if (prolog_len+msg_len>DEBUG_MAX_LINE_LEN)
	{
		msg_len = DEBUG_MAX_LINE_LEN-prolog_len;
	}
	if (prolog_len) memcpy(out_line,prolog,prolog_len);
	if (msg_len) memcpy(out_line+prolog_len,msg,msg_len);
	out_line[prolog_len+msg_len] = 0;
	// If not buffering, print the line.
	if (debugBufferSize==0) 
	{
		printf("%s\n",out_line);
		fflush(stdout);
	}
}

void DebugSetLevel(unsigned long debug_level)
{
	// Sets the debug level(s)
	if (!debugInitialized) InitDebug();
	debugLevel |= debug_level;
}

void DebugClearLevel(unsigned long debug_level)
{
	// Clears the debug level(s)
	if (!debugInitialized) InitDebug();
	debugLevel &= (DEBUG_LEVEL_ALL^debug_level);
}

const char *DebugString(const char *text)
{
	// Returns a non NULL string for the text.
	// Note: Use of this is nested in the DEBUG_OUT() calls, so
	//         must not use static buffer if changing in the future.
	if (text) return(text);
	return("(null)");
}

const char *DebugString(const char *text, size_t len)
{
	char rc[1024];

	if (text==NULL)	return("(null)");
	if (len>=sizeof(rc)) len = sizeof(rc) - 1;
	memcpy(rc,text,len);
	rc[len] = 0;
	return(rc);
}

void DebugAssertFailed(const char *message,
					   const char *filename, unsigned long line)
{
	// Called when assert fails
	DebugOutput("**** Assert Failed ****",filename,line);
	if (message) DebugOutput(message,filename,line);
	exit(1);
}

void DebugMemoryCheckAlloc(void *alloc_var,
						   const char *filename, unsigned long line)
{
	// Called before memory allocation to verify that the pointer is
	//   not already allocated.

	// Ignore NULL
	if (alloc_var==NULL) return;

	unsigned long idx = FindMemoryIdx(alloc_var);
	if (idx!=totalMemAlloc)
	{
		// Found the memory in the list.
		// Warn the developer since this should never happen.
		// Force memory tracing on so we will get the memory list on exit.
		DebugSetLevel(DEBUG_LEVEL_MEM);
		DebugOutput(DebugFormat("Memory 0x%08x was already allocated at File:%s Line:%lu",
		                        alloc_var,MemAlloc[idx].filename,MemAlloc[idx].line),
					filename,line);
		exit(1);
	}

	// If not found, but the pointer was not NULL, caller did not
	//   initialize the variable to NULL or we have a memory leak.
	// Warn the developer.
	if (alloc_var)
		DebugAssertFailed(DebugFormat("**** Memory pointer not NULL before allocation (0x%08x) ****", alloc_var),
						  filename,line);
}

static const char *MemoryAllocTypeStr(char alloc_type)
{
	switch (alloc_type)
	{
	case MEMORY_ALLOC_MEMORY:
		return("Memory");
	case MEMORY_ALLOC_OBJECT:
		return("Object");
	}
	return("Unknown Memory Type");
}

void DebugMemoryAlloc(void *alloc_var, char alloc_type, unsigned long alloc_size,
					  bool perm, const char *filename, unsigned long line)
{
	// Called after the memory allocation.  Loads the new memory into the list.

	// Find a free entry.
	unsigned long idx = FindFreeMemoryIdx(filename,line);

	// Populate the entry
	MemAlloc[idx].mem_ptr = alloc_var;
	MemAlloc[idx].alloc_type = alloc_type;
	MemAlloc[idx].alloc_size = alloc_size;
	MemAlloc[idx].filename = Basename(filename);
	MemAlloc[idx].line = line;
	MemAlloc[idx].perm = perm;
	if (alloc_size &&
		(debugLevel & DEBUG_FLAG_DIRTY_MEM) &&
		(alloc_type==MEMORY_ALLOC_MEMORY))
	{
		// Fill memory allocation with non-zero values
		memset(alloc_var,0xaa,alloc_size);
	}

	if (DebugActive(DEBUG_LEVEL_MEM,filename,line))
		DebugOutput(DebugFormat("%s at 0x%08x allocated",
						MemoryAllocTypeStr(alloc_type),
						MemAlloc[idx].mem_ptr),
					filename,line);
}

void DebugMemoryDelete(void *alloc_var, char alloc_type,
					   const char *filename, unsigned long line)
{
	// Called after a memory deallocation.
	// Removes the memory from the memory list

	// Find the memory in the memory list
	unsigned long idx = FindMemoryIdx(alloc_var);

	// If the memory is not found in the list, it was never allocated
	//   through the macros, or it was deleted already.
	if (idx==totalMemAlloc)
	{
		DebugSetLevel(DEBUG_LEVEL_MEM);
		DebugOutput(DebugFormat("Deallocation: Memory/Object at 0x%08x not allocated",alloc_var),filename,line);
		exit(1);
	}

	if (MemAlloc[idx].alloc_type!=alloc_type)
	{
		DebugSetLevel(DEBUG_LEVEL_MEM);
		DebugOutput(DebugFormat("Deallocation: %s allocated at 0x%08x was deallocated as %s",
								MemoryAllocTypeStr(MemAlloc[idx].alloc_type),
								alloc_var,
								MemoryAllocTypeStr(alloc_type)),
					filename,line);
		exit(1);
	}

	if (DebugActive(DEBUG_LEVEL_MEM,filename,line))
		DebugOutput(DebugFormat("%s at 0x%08x %s(%lu) deallocated",
								MemoryAllocTypeStr(MemAlloc[idx].alloc_type),
								MemAlloc[idx].mem_ptr,
								MemAlloc[idx].filename,
								MemAlloc[idx].line),
					filename,line);
	// Free up the entry
	MemAlloc[idx].mem_ptr = NULL;
}

void DebugPrintMemoryAlloc(const char *filename, unsigned long line)
{
	// Prints the current memory allocation list to the console.
	// Called at exit or whenever the developer wants to.
	if (DebugActive(DEBUG_LEVEL_MEM|DEBUG_LEVEL_MEMLEAK,filename,line))
	{
		unsigned long idx;
		bool memory_allocated = false;

		DebugOutput("*** Allocated Memory ***",filename,line);
		if (totalMemAlloc)
		{
			for (idx=0; idx<totalMemAlloc; idx++) if (MemAlloc[idx].mem_ptr && !MemAlloc[idx].perm)
			{
				// Each entry in the memory list is printed
				memory_allocated = true;
				memory_allocated = true;
				DebugOutput(DebugFormat("%s Address: 0x%08x %s(%lu)",
								MemoryAllocTypeStr(MemAlloc[idx].alloc_type),
								MemAlloc[idx].mem_ptr,
								MemAlloc[idx].filename,
								MemAlloc[idx].line),
							filename,line);
			}
		}
		if (!memory_allocated) DebugOutput("All allocated memory is free",filename,line);
		DebugOutput("************************",filename,line);
	}
}

void DebugPrintMemoryDump(unsigned long debug_level,
						  const char *addr, unsigned long len,
						  const char *filename, unsigned long line)
{
	if (DebugActive(debug_level,filename,line))
	{
		unsigned long line_idx;
		for (line_idx=0; line_idx<len; line_idx += 16)
		{
			char output_line[80];
			int i;
			sprintf(output_line,"0x%08x: ",addr+line_idx);
			size_t line_len = strlen(output_line);
			for (i=0; i<16; i++)
			{
				if ((line_idx+i)<len)
					sprintf(output_line+line_len,"%02x ",addr[line_idx+i]);
				else strcpy(output_line+line_len,"   ");
				line_len += 3;
			}
			for (i=0; ((line_idx+i)<len) && (i<16); i++)
			{
				if (((addr[line_idx+i]>='a') && (addr[line_idx+i]<='z')) ||
					((addr[line_idx+i]>='A') && (addr[line_idx+i]<='Z')) ||
					((addr[line_idx+i]>='0') && (addr[line_idx+i]<='9')) ||
					(addr[line_idx+i]==' '))
				sprintf(output_line+line_len,"%c",addr[line_idx+i]);
				else strcpy(output_line+line_len,".");
				line_len++;
			}
			DebugOutput(output_line,filename,line);
		}
	}
}

const char *DebugTimestampStr(long long timestamp)
{
	static char rc[80];
	sprintf(rc,"Timestamp[%Ld]",timestamp);
	return(rc);
}

const char *DebugJString(void *jenv_ptr, void * jstring_ptr)
{
	static char buffer[5][1024];
	static int curr_idx = 0;
	JNIEnv *jenv = (JNIEnv *) jenv_ptr;
	jstring java_str = (jstring) jstring_ptr;

	if (java_str==NULL) return(DebugString(NULL));

	const char *value = jenv->GetStringUTFChars(java_str, NULL);
	if (value==NULL) return(DebugString(NULL));

	size_t len = strlen(value);
	if (len>=sizeof(buffer[0])) len = sizeof(buffer[0]) - 1;
	if (curr_idx==5) curr_idx = 0;
	memcpy(buffer[curr_idx],value,len);
	buffer[curr_idx][len] = 0;
	jenv->ReleaseStringUTFChars(java_str, value);
	return(buffer[curr_idx++]);
}

void DebugTransTag(const char * filename, unsigned long line)
{
	if (DebugActive(DEBUG_LEVEL_TXN,filename,line))
	{
		char buffer[256];
		short status = 0;
		short length = 0;
		short txn_status = 0;
		long long txnId;

		// Get current txn status
		status = STATUSTRANSACTION(&txn_status);
		DebugOutput(DebugFormat("status from STATUSTRANSACTION=%d; txn status=%d", txn_status, status),
					filename, line);

		// Get the transId for the current transaction (if one exists)
		status = GETTRANSID((short *)&txnId);
		if (status != 0)
		{
			// Report error status and return null
			DebugOutput(DebugFormat("Status from GETTRANSID = %d", status),
						filename, line);
			return;
		}

		buffer[length] = 0;
		DebugOutput(DebugFormat("transTagASCII = '%s'", buffer),
					filename, line);
		return;
	}
}

const char *DebugBoolStr(bool isTrue)
{
	if (isTrue) return("TRUE");
	return("FALSE");
}

const char *CliDebugSqlError(long retcode)
{
	static char rc[80];
	
	// SQL_NO_DATA_FOUND can be defined as SQL_NO_DATA
	if (retcode==SQL_NO_DATA_FOUND)
	{
		if (SQL_NO_DATA_FOUND==SQL_NO_DATA) return("SQL_NO_DATA_FOUND|SQL_NO_DATA");
		return("SQL_NO_DATA_FOUND");
	}
	switch (retcode)
	{
	case SQL_SUCCESS:
		return("SQL_SUCCESS");
	case SQL_SUCCESS_WITH_INFO:
		return("SQL_SUCCESS_WITH_INFO");
	case SQL_NO_DATA:
		return("SQL_NO_DATA");
	case SQL_ERROR:
		return("SQL_ERROR");
	case SQL_INVALID_HANDLE:
		return("SQL_INVALID_HANDLE");
	case SQL_STILL_EXECUTING:
		return("SQL_STILL_EXECUTING");
	case SQL_NEED_DATA:
		return("SQL_NEED_DATA");
	case STMT_ID_MISMATCH_ERROR:
		return("STMT_ID_MISMATCH_ERROR");
	case DIALOGUE_ID_NULL_ERROR:
		return("DIALOGUE_ID_NULL_ERROR");
	case STMT_ID_NULL_ERROR:
		return("STMT_ID_NULL_ERROR");
	case NOWAIT_PENDING:
		return("NOWAIT_PENDING");
	case STMT_ALREADY_EXISTS:
		return("STMT_ALREADY_EXISTS");
	case STMT_DOES_NOT_EXIST:
		return("STMT_DOES_NOT_EXIST");
	case STMT_IS_NOT_CALL:
		return("STMT_IS_NOT_CALL");
	case RS_INDEX_OUT_OF_RANGE:
		return("RS_INDEX_OUT_OF_RANGE");
	case RS_ALREADY_EXISTS:
		return("RS_ALREADY_EXISTS");
	case RS_ALLOC_ERROR:
		return("RS_ALLOC_ERROR");
	case RS_DOES_NOT_EXIST:
		return("RS_DOES_NOT_EXIST");
	case PROGRAM_ERROR:
		return("PROGRAM_ERROR");
	case ODBC_SERVER_ERROR:
		return("ODBC_SERVER_ERROR");
	case ODBC_RG_ERROR:
		return("ODBC_RG_ERROR");
	case ODBC_RG_WARNING:
		return("ODBC_RG_WARNING");
	case SQL_RETRY_COMPILE_AGAIN:
		return("SQL_RETRY_COMPILE_AGAIN");
	case SQL_QUERY_CANCELLED:
		return("SQL_QUERY_CANCELLED");
	case CANCEL_NOT_POSSIBLE:
		return("CANCEL_NOT_POSSIBLE");
	case NOWAIT_ERROR:
		return("NOWAIT_ERROR");
	}
	sprintf(rc,"Unknown SQL Error (%ld)",retcode);
	return(rc);
}

const char *CliDebugStatementType(int stmtType)
{
	static char rc[80];
	switch (stmtType) {
	case INTERNAL_STMT:
		return("INTERNAL_STMT");
	case EXTERNAL_STMT:
		return("EXTERNAL_STMT");
	}
	sprintf(rc,"Statement Type %d Unknown",stmtType);
	return(rc);
}

const char *CliDebugSqlStatementType(int stmtType)
{
	static char rc[256];

	if (stmtType == TYPE_UNKNOWN) return("TYPE_UNKNOWN");
	rc[0] = 0;
	if (stmtType & TYPE_SELECT) strcat(rc,"|TYPE_SELECT");
	if (stmtType & TYPE_UPDATE) strcat(rc,"|TYPE_UPDATE");
	if (stmtType & TYPE_DELETE) strcat(rc,"|TYPE_DELETE");
	if (stmtType & TYPE_INSERT) strcat(rc,"|TYPE_INSERT");
	if (stmtType & TYPE_EXPLAIN) strcat(rc,"|TYPE_EXPLAIN");
	if (stmtType & TYPE_CREATE) strcat(rc,"|TYPE_CREATE");
	if (stmtType & TYPE_GRANT) strcat(rc,"|TYPE_GRANT");
	if (stmtType & TYPE_DROP) strcat(rc,"|TYPE_DROP");
	if (stmtType & TYPE_CALL) strcat(rc,"|TYPE_CALL");
	if (rc[0]==0) sprintf(rc," Unknown(0x%08x)",stmtType);
	return(rc+1);
}

const char *CliDebugSqlQueryStatementType(int stmtType)
{
	static char rc[80];

	switch (stmtType)
	{
	case INVALID_SQL_QUERY_STMT_TYPE:
		return("INVALID_SQL_QUERY_STMT_TYPE");
	case SQL_OTHER:
		return("SQL_OTHER");
	case SQL_UNKNOWN:
		return("SQL_UNKNOWN");
	case SQL_SELECT_UNIQUE:
		return("SQL_SELECT_UNIQUE");
	case SQL_SELECT_NON_UNIQUE:
		return("SQL_SELECT_NON_UNIQUE");
	case SQL_INSERT_UNIQUE:
		return("SQL_INSERT_UNIQUE");
	case SQL_INSERT_NON_UNIQUE:
		return("SQL_INSERT_NON_UNIQUE");
	case SQL_UPDATE_UNIQUE:
		return("SQL_UPDATE_UNIQUE");
	case SQL_UPDATE_NON_UNIQUE:
		return("SQL_UPDATE_NON_UNIQUE");
	case SQL_DELETE_UNIQUE:
		return("SQL_DELETE_UNIQUE");
	case SQL_DELETE_NON_UNIQUE:
		return("SQL_DELETE_NON_UNIQUE");
	case SQL_CONTROL:
		return("SQL_CONTROL");
	case SQL_SET_TRANSACTION:
		return("SQL_SET_TRANSACTION");
	case SQL_SET_CATALOG:
		return("SQL_SET_CATALOG");
	case SQL_SET_SCHEMA:
		return("SQL_SET_SCHEMA");
	case SQL_CALL_NO_RESULT_SETS:
		return("SQL_CALL_NO_RESULT_SETS");
	case SQL_CALL_WITH_RESULT_SETS:
		return("SQL_CALL_WITH_RESULT_SETS");
	case SQL_SP_RESULT_SET:
		return("SQL_SP_RESULT_SET");
	}
	sprintf(rc,"Unknown (%d)",stmtType);
	return(rc);
};

const char *CliDebugSqlAttrType(int code)
{
	static char rc[80];
	switch (code)
	{
		case SQL_ATTR_CURSOR_HOLDABLE:
			return("SQL_ATTR_CURSOR_HOLDABLE");
		case SQL_ATTR_INPUT_ARRAY_MAXSIZE:
			return("SQL_ATTR_INPUT_ARRAY_MAXSIZE");
		case SQL_ATTR_QUERY_TYPE:
			return("SQL_ATTR_QUERY_TYPE");
		case SQL_ATTR_MAX_RESULT_SETS:
			return("SQL_ATTR_MAX_RESULT_SETS");
	}
	sprintf(rc,"<Unknown(%d)>",code);
	return(rc);
}

const char *CliDebugSqlTypeCode(int code)
{
	static char rc[80];
	switch (code)
	{
		case SQLTYPECODE_CHAR:
			return("SQLTYPECODE_CHAR");
		case SQLTYPECODE_NUMERIC:
			return("SQLTYPECODE_NUMERIC");
		case SQLTYPECODE_NUMERIC_UNSIGNED:
			return("SQLTYPECODE_NUMERIC_UNSIGNED");
		case SQLTYPECODE_DECIMAL:
			return("SQLTYPECODE_DECIMAL");
		case SQLTYPECODE_DECIMAL_UNSIGNED:
			return("SQLTYPECODE_DECIMAL_UNSIGNED");
		case SQLTYPECODE_DECIMAL_LARGE:
			return("SQLTYPECODE_DECIMAL_LARGE");
		case SQLTYPECODE_DECIMAL_LARGE_UNSIGNED:
			return("SQLTYPECODE_DECIMAL_LARGE_UNSIGNED");
		case SQLTYPECODE_INTEGER:
			return("SQLTYPECODE_INTEGER");
		case SQLTYPECODE_INTEGER_UNSIGNED:
			return("SQLTYPECODE_INTEGER_UNSIGNED");
		case SQLTYPECODE_LARGEINT:
			return("SQLTYPECODE_LARGEINT");
		case SQLTYPECODE_SMALLINT:
			return("SQLTYPECODE_SMALLINT");
		case SQLTYPECODE_SMALLINT_UNSIGNED:
			return("SQLTYPECODE_SMALLINT_UNSIGNED");
		case SQLTYPECODE_BPINT_UNSIGNED:
			return("SQLTYPECODE_BPINT_UNSIGNED");
		case SQLTYPECODE_TDM_FLOAT:
			return("SQLTYPECODE_TDM_FLOAT");
		case SQLTYPECODE_IEEE_FLOAT:
			return("SQLTYPECODE_IEEE_FLOAT");
		case SQLTYPECODE_TDM_REAL:
			return("SQLTYPECODE_TDM_REAL");
		case SQLTYPECODE_IEEE_REAL:
			return("SQLTYPECODE_IEEE_REAL");
		case SQLTYPECODE_TDM_DOUBLE:
			return("SQLTYPECODE_TDM_DOUBLE");
		case SQLTYPECODE_IEEE_DOUBLE:
			return("SQLTYPECODE_IEEE_DOUBLE");
		case SQLTYPECODE_DATETIME:
			return("SQLTYPECODE_DATETIME");
		case SQLTYPECODE_INTERVAL:
			return("SQLTYPECODE_INTERVAL");
		case SQLTYPECODE_VARCHAR:
			return("SQLTYPECODE_VARCHAR");
		case SQLTYPECODE_VARCHAR_WITH_LENGTH:
			return("SQLTYPECODE_VARCHAR_WITH_LENGTH");
		case SQLTYPECODE_VARCHAR_LONG:
			return("SQLTYPECODE_VARCHAR_LONG");
		case SQLTYPECODE_BIT:
			return("SQLTYPECODE_BIT");
		case SQLTYPECODE_BITVAR:
			return("SQLTYPECODE_BITVAR");
	};

	sprintf(rc,"<Unknown(%d)>",code);
	return(rc);
}

static const char *Trunc(const char *text, size_t targ_size)
{
	static char rc[1024];
	if (text==NULL) return("");
	
	size_t src_len = strlen(text);
	if (src_len<targ_size) return(text);
	if (targ_size>sizeof(rc)) targ_size = sizeof(rc);
	memcpy(rc,text,targ_size-1);
	rc[targ_size-1] = 0;
	return(rc);
}

void CliDebugShowServerStatement(void *vSrvrStmt,
                                 const char *filename, unsigned long line)
{
	SRVR_STMT_HDL *pSrvrStmt = (SRVR_STMT_HDL *) vSrvrStmt;
	if (pSrvrStmt)
	{
		DEBUG_OUT_LOC(DEBUG_LEVEL_CLI|DEBUG_LEVEL_DATA|DEBUG_LEVEL_STMT,
			("Server Statement Address=0x%08x", pSrvrStmt),
			filename,
			line);
		DEBUG_OUT_LOC(DEBUG_LEVEL_CLI|DEBUG_LEVEL_DATA|DEBUG_LEVEL_STMT,
			("  Statement Address=0x%08x, Statement Type=%s",
				&pSrvrStmt->stmt,
				CliDebugStatementType(pSrvrStmt->stmtType)),
			filename,
			line);
		DEBUG_OUT_LOC(DEBUG_LEVEL_CLI|DEBUG_LEVEL_DATA|DEBUG_LEVEL_STMT,
			("  SQL Statement Type=%s",
				CliDebugSqlStatementType(pSrvrStmt->sqlStmtType)),
			filename,
			line);
		DEBUG_OUT_LOC(DEBUG_LEVEL_CLI|DEBUG_LEVEL_DATA|DEBUG_LEVEL_STMT,
			("  SQL Attribute Query Statement Type=%s",
				CliDebugSqlQueryStatementType(pSrvrStmt->getSqlQueryStatementType())),
			filename,
			line);
		DEBUG_OUT_LOC(DEBUG_LEVEL_CLI|DEBUG_LEVEL_DATA|DEBUG_LEVEL_STMT,
			("  Statement='%s'",
				DebugString((const char *)pSrvrStmt->sqlString.dataValue._buffer)),
			filename,
			line);
		DEBUG_OUT_LOC(DEBUG_LEVEL_CLI|DEBUG_LEVEL_DATA|DEBUG_LEVEL_STMT,
			("  Statement Name='%s'",
				pSrvrStmt->stmtName),
			filename,
			line);
		DEBUG_OUT_LOC(DEBUG_LEVEL_CLI|DEBUG_LEVEL_DATA|DEBUG_LEVEL_STMT,
			("  RSMax=%d, RSIndex=%d, isSPJRS=%d",
				pSrvrStmt->RSMax,
				pSrvrStmt->RSIndex,
				pSrvrStmt->isSPJRS),
			filename,
			line);
		DEBUG_OUT_LOC(DEBUG_LEVEL_CLI|DEBUG_LEVEL_DATA|DEBUG_LEVEL_STMT,
			("  Param Count=%ld, Column Count=%ld",
				pSrvrStmt->paramCount,
				pSrvrStmt->columnCount),
			filename,
			line);
		DEBUG_OUT_LOC(DEBUG_LEVEL_CLI|DEBUG_LEVEL_DATA|DEBUG_LEVEL_STMT,
			("  Fetch Quad Entries=%ld Rowset Size=%ld Field=0x%08x",
				pSrvrStmt->fetchQuadEntries, pSrvrStmt->fetchRowsetSize, pSrvrStmt->fetchQuadField),
			filename,
			line);
		DEBUG_OUT_LOC(DEBUG_LEVEL_CLI|DEBUG_LEVEL_DATA|DEBUG_LEVEL_STMT,
			("  Batch Quad Entries=%ld Rowset Size=%ld Field=0x%08x",
				pSrvrStmt->batchQuadEntries, pSrvrStmt->batchRowsetSize, pSrvrStmt->batchQuadField),
			filename,
			line);
		DEBUG_OUT_LOC(DEBUG_LEVEL_CLI|DEBUG_LEVEL_DATA|DEBUG_LEVEL_STMT,
			("  EndOfData=0x%08x",
				pSrvrStmt->endOfData),
			filename,
			line);
		DEBUG_OUT_LOC(DEBUG_LEVEL_CLI|DEBUG_LEVEL_DATA|DEBUG_LEVEL_STMT,
			("  Module_name=%s",
				pSrvrStmt->moduleId.module_name),
			filename,
			line);
	} else {
		DEBUG_OUT_LOC(DEBUG_LEVEL_ENTRY|DEBUG_LEVEL_DATA|DEBUG_LEVEL_STMT,("Server Statement = (null)"),
			filename,
			line);
	}
}

const char *CliDebugSqlValueStr(const void *data_value)
{
	static char returnMessage[2048];
	const SQLValue_def *data_value_ptr = (const SQLValue_def *) data_value;

	if (data_value_ptr==NULL) return("NULL SQLValue");

	switch (data_value_ptr->dataType)
	{
		case SQLTYPECODE_VARCHAR:
		case SQLTYPECODE_VARCHAR_LONG:
			{
				long len = data_value_ptr->dataValue._length;
				if (len>=sizeof(returnMessage)) len = sizeof(returnMessage) - 1;
				memcpy(returnMessage,data_value_ptr->dataValue._buffer,len);
				returnMessage[len] = 0;
			}
			break;
	default:
		sprintf(returnMessage,"<SQLValue_def=%s>",
			CliDebugSqlTypeCode(data_value_ptr->dataType));
	}
	return(returnMessage);
}

const char *CliDebugDescTypeStr(int descType)
{
	switch (descType)
	{
		case SRVR_STMT_HDL::Input:
			return("Input");
		case SRVR_STMT_HDL::Output:
			return("Output");
	}
	return("Unknown");
}

const char *CliDebugDescItemStr(long descItem)
{
	static char rc[80];
	switch (descItem)
	{
	case SQLDESC_TYPE:
		return("SQLDESC_TYPE");
	case SQLDESC_DATETIME_CODE:
		return("SQLDESC_DATETIME_CODE");
	case SQLDESC_LENGTH:
		return("SQLDESC_LENGTH");
	case SQLDESC_OCTET_LENGTH:
		return("SQLDESC_OCTET_LENGTH");
	case SQLDESC_PRECISION:
		return("SQLDESC_PRECISION");
	case SQLDESC_SCALE:
		return("SQLDESC_SCALE");
	case SQLDESC_INT_LEAD_PREC:
		return("SQLDESC_INT_LEAD_PREC");
	case SQLDESC_NULLABLE:
		return("SQLDESC_NULLABLE");
	case SQLDESC_CHAR_SET:
		return("SQLDESC_CHAR_SET");
	case SQLDESC_CHAR_SET_CAT:
		return("SQLDESC_CHAR_SET_CAT");
	case SQLDESC_CHAR_SET_SCH:
		return("SQLDESC_CHAR_SET_SCH");
	case SQLDESC_CHAR_SET_NAM:
		return("SQLDESC_CHAR_SET_NAM");
	case SQLDESC_COLL_CAT:
		return("SQLDESC_COLL_CAT");
	case SQLDESC_COLL_SCH:
		return("SQLDESC_COLL_SCH");
	case SQLDESC_COLL_NAM:
		return("SQLDESC_COLL_NAM");
	case SQLDESC_NAME:
		return("SQLDESC_NAME");
	case SQLDESC_UNNAMED:
		return("SQLDESC_UNNAMED");
	case SQLDESC_HEADING:
		return("SQLDESC_HEADING");
	case SQLDESC_IND_TYPE:
		return("SQLDESC_IND_TYPE");
	case SQLDESC_VAR_PTR:
		return("SQLDESC_VAR_PTR");
	case SQLDESC_IND_PTR:
		return("SQLDESC_IND_PTR");
	case SQLDESC_RET_LEN:
		return("SQLDESC_RET_LEN");
	case SQLDESC_RET_OCTET_LEN:
		return("SQLDESC_RET_OCTET_LEN");
	case SQLDESC_VAR_DATA:
		return("SQLDESC_VAR_DATA");
	case SQLDESC_IND_DATA:
		return("SQLDESC_IND_DATA");
	case SQLDESC_TYPE_FS:
		return("SQLDESC_TYPE_FS");
	case SQLDESC_IND_LENGTH:
		return("SQLDESC_IND_LENGTH");
	case SQLDESC_ROWSET_VAR_LAYOUT_SIZE:
		return("SQLDESC_ROWSET_VAR_LAYOUT_SIZE");
	case SQLDESC_ROWSET_IND_LAYOUT_SIZE:
		return("SQLDESC_ROWSET_IND_LAYOUT_SIZE");
	case SQLDESC_ROWSET_SIZE:
		return("SQLDESC_ROWSET_SIZE");
	case SQLDESC_ROWSET_HANDLE:
		return("SQLDESC_ROWSET_HANDLE");
	case SQLDESC_ROWSET_NUM_PROCESSED:
		return("SQLDESC_ROWSET_NUM_PROCESSED");
	case SQLDESC_ROWSET_ADD_NUM_PROCESSED:
		return("SQLDESC_ROWSET_ADD_NUM_PROCESSED");
	case SQLDESC_ROWSET_STATUS_PTR:
		return("SQLDESC_ROWSET_STATUS_PTR");
	case SQLDESC_TABLE_NAME:
		return("SQLDESC_TABLE_NAME");
	case SQLDESC_SCHEMA_NAME:
		return("SQLDESC_SCHEMA_NAME");
	case SQLDESC_CATALOG_NAME:
		return("SQLDESC_CATALOG_NAME");
	case SQLDESC_PARAMETER_MODE:
		return("SQLDESC_PARAMETER_MODE");
	case SQLDESC_ORDINAL_POSITION:
		return("SQLDESC_ORDINAL_POSITION");
	case SQLDESC_PARAMETER_INDEX:
		return("SQLDESC_PARAMETER_INDEX");
	case SQLDESC_DESCRIPTOR_TYPE:
		return("SQLDESC_DESCRIPTOR_TYPE");
	}
	sprintf(rc,"Unknown(%ld)",descItem);
	return(rc);
}

void CliDebugShowDesc(void *vSrvrStmt, int uDescType,
                      const char *filename, unsigned long line)
{

	long debug_level = DEBUG_LEVEL_CLI|DEBUG_LEVEL_DATA|DEBUG_LEVEL_ROWSET;
	SRVR_STMT_HDL *pSrvrStmt = (SRVR_STMT_HDL *) vSrvrStmt;
	SRVR_STMT_HDL::DESC_TYPE descType = (SRVR_STMT_HDL::DESC_TYPE) uDescType;
	SQLDESC_ID *pDesc = pSrvrStmt->getDesc(descType);
	long numEntries = pSrvrStmt->getDescEntryCount(descType);
	enum ValueType { Numeric, String };
	struct {
		long Value;
		enum ValueType valueType;
	} descItem[] = {
		{ SQLDESC_NAME, String },
		{ SQLDESC_TYPE, Numeric },
		{ SQLDESC_LENGTH, Numeric },
		{ SQLDESC_VAR_PTR, Numeric },
		{ SQLDESC_ROWSET_VAR_LAYOUT_SIZE, Numeric },
		{ SQLDESC_IND_PTR, Numeric },
		{ SQLDESC_IND_LENGTH, Numeric },
		{ SQLDESC_ROWSET_IND_LAYOUT_SIZE, Numeric },
		{ SQLDESC_ROWSET_SIZE, Numeric },
		{ SQLDESC_ROWSET_HANDLE, Numeric },
		{ SQLDESC_ROWSET_STATUS_PTR, Numeric },
		{ SQLDESC_ROWSET_NUM_PROCESSED, Numeric },
		{ SQLDESC_DATETIME_CODE, Numeric },
		{ SQLDESC_OCTET_LENGTH, Numeric },
		{ SQLDESC_PRECISION, Numeric },
		{ SQLDESC_SCALE, Numeric },
		{ SQLDESC_INT_LEAD_PREC, Numeric },
		{ SQLDESC_NULLABLE, Numeric },
		{ SQLDESC_CHAR_SET, Numeric },
		{ SQLDESC_CHAR_SET_CAT, Numeric },
		{ SQLDESC_CHAR_SET_SCH, Numeric },
		{ SQLDESC_CHAR_SET_NAM, Numeric },
		{ SQLDESC_COLL_CAT, Numeric },
		{ SQLDESC_COLL_SCH, Numeric },
		{ SQLDESC_COLL_NAM, Numeric },
		{ SQLDESC_UNNAMED, Numeric },
		{ SQLDESC_IND_TYPE, Numeric },
		{ SQLDESC_RET_LEN, Numeric },
		{ SQLDESC_RET_OCTET_LEN, Numeric },
		{ SQLDESC_TYPE_FS, Numeric },
		{ SQLDESC_CATALOG_NAME, String },
		{ SQLDESC_SCHEMA_NAME, String },
		{ SQLDESC_TABLE_NAME, String },
		{ SQLDESC_PARAMETER_MODE, Numeric },
		{ SQLDESC_ORDINAL_POSITION, Numeric },
		{ SQLDESC_PARAMETER_INDEX, Numeric },
		{ SQLDESC_DESCRIPTOR_TYPE, Numeric },
		{ -999, Numeric }};

	DEBUG_OUT_LOC(debug_level,
		("Server Statement=0x%08x, Descriptor=0x%08x Type=%s",
		 pSrvrStmt,
		 pDesc,
		 CliDebugDescTypeStr(descType)),
		filename,
		line);
	long entry = 0;
	long retcode;
	while (entry<numEntries)
	{
		int descItemIdx = 0;
		long descSQLType = -999;
		long var_length=-1;
		DEBUG_OUT_LOC(debug_level,
			("Desc Entry %ld", entry),
			filename,
			line);
		while (descItem[descItemIdx].Value!=-999)
		{
			switch (descItem[descItemIdx].valueType)
			{
			case Numeric:
				{
				int lValue;
				retcode = SQL_EXEC_GetDescItem(pDesc,
			                                   entry+1,
											   descItem[descItemIdx].Value,
										       &lValue,
											   NULL, 0, NULL, 0);
				if (retcode==SQL_SUCCESS) switch (descItem[descItemIdx].Value)
				{
				case SQLDESC_LENGTH:
					var_length = lValue;
					DEBUG_OUT_LOC(debug_level,
						("%s=%ld", CliDebugDescItemStr(descItem[descItemIdx].Value),lValue),
						filename,
						line);
					break;
				case SQLDESC_TYPE:
					DEBUG_OUT_LOC(debug_level,
						("%s=%s", CliDebugDescItemStr(descItem[descItemIdx].Value),CliDebugSqlTypeCode(lValue)),
						filename,
						line);
					descSQLType = lValue;
					break;
				case SQLDESC_VAR_PTR:
					if (lValue) {
						switch (descSQLType) 
						{
						case SQLTYPECODE_INTEGER:
							DEBUG_OUT_LOC(debug_level,
								("%s: Value=%ld", CliDebugDescItemStr(descItem[descItemIdx].Value),*((long *) lValue)),
								filename,
								line);
							break;
						case SQLTYPECODE_SMALLINT:
							DEBUG_OUT_LOC(debug_level,
								("%s: Value=%d", CliDebugDescItemStr(descItem[descItemIdx].Value),*((short *) lValue)),
								filename,
								line);
							break;
						case SQLTYPECODE_LARGEINT:
							DEBUG_OUT_LOC(debug_level,
								("%s: Value=%Ld", CliDebugDescItemStr(descItem[descItemIdx].Value),*((long long *) lValue)),
								filename,
								line);
							break;
						case SQLTYPECODE_VARCHAR:
							if (var_length != -1) {
								char buffer[4096];
								memcpy(buffer,(char *)lValue, var_length);
								buffer[var_length]=0;
								DEBUG_OUT_LOC(debug_level,
									("%s: Value='%s'", CliDebugDescItemStr(descItem[descItemIdx].Value),buffer),
									filename,
									line);
								break;
							}
						default:
							break;
						} 
					}
					DEBUG_OUT_LOC(debug_level,
						("%s=0x%08x", CliDebugDescItemStr(descItem[descItemIdx].Value),lValue),
						filename,
						line);
					break;
				case SQLDESC_IND_PTR:
					DEBUG_OUT_LOC(debug_level,
						("%s=0x%08x", CliDebugDescItemStr(descItem[descItemIdx].Value),lValue),
						filename,
						line);
					break;
				case SQLDESC_CHAR_SET:
					DEBUG_OUT_LOC(debug_level,
						("%s=%s", CliDebugDescItemStr(descItem[descItemIdx].Value),getCharsetEncoding(lValue)),
						filename,
						line);
					break;

				default:
					DEBUG_OUT_LOC(debug_level,
						("%s=%ld", CliDebugDescItemStr(descItem[descItemIdx].Value),lValue),
						filename,
						line);
				}
				}
				break;
			case String:
				{
				char sValue[1024];
				int len;
				retcode = SQL_EXEC_GetDescItem(pDesc,
                                               entry+1,
											   descItem[descItemIdx].Value,
										       NULL,
											   sValue, sizeof(sValue)-1, &len, 0);
				if (retcode==SQL_SUCCESS) 
				{
					sValue[len] = 0;
					DEBUG_OUT_LOC(debug_level,
					("%s='%s'", CliDebugDescItemStr(descItem[descItemIdx].Value),sValue),
					filename,
					line);
				}
				}
				break;
			default:
                                break;
			}
			descItemIdx++;
		}
		entry++;
	}

	int row_count = 0;
	retcode =  SQL_EXEC_GetDiagnosticsStmtInfo2(&pSrvrStmt->stmt,
												SQLDIAG_ROW_COUNT,
					                            &row_count,
					                            NULL, 0, NULL);
	if (retcode==SQL_SUCCESS)
	{
		DEBUG_OUT_LOC(debug_level,
			("SQLDIAG_ROW_COUNT=%ld", row_count),
			filename,
			line);
	}
}

void CliDebugShowQuad(void *vSrvrStmt, int uDescType,
                      const char *filename, unsigned long line)
{
	long debug_level = DEBUG_LEVEL_CLI|DEBUG_LEVEL_DATA|DEBUG_LEVEL_ROWSET;
	SRVR_STMT_HDL *pSrvrStmt = (SRVR_STMT_HDL *) vSrvrStmt;
	SRVR_STMT_HDL::DESC_TYPE descType = (SRVR_STMT_HDL::DESC_TYPE) uDescType;
	struct SQLCLI_QUAD_FIELDS *quadField = pSrvrStmt->getQuadField(descType);
	long quadEntries = pSrvrStmt->getQuadEntryCount(descType);
	long rowsetSize = pSrvrStmt->getRowsetSize(descType);
	long idx;
	DEBUG_OUT_LOC(debug_level,
		("Server Statement=0x%08x, quadField=0x%08x Type=%s",
		 pSrvrStmt,
		 quadField,
		 CliDebugDescTypeStr(descType)),
		filename,
		line);
	for (idx=0; idx<quadEntries; idx++)
	{
		DEBUG_OUT_LOC(debug_level,
			("  quadField[%ld].var_layout = %ld", idx, quadField[idx].var_layout),
			filename,
			line);
		DEBUG_OUT_LOC(debug_level,
			("               var_ptr    = 0x%08x", quadField[idx].var_ptr),
			filename,
			line);
		DEBUG_OUT_LOC(debug_level,
			("               ind_layout = %ld", quadField[idx].ind_layout),
			filename,
			line);
		if (quadField[idx].ind_ptr)
		{
			long rowset_idx;
			char ind_line[1024];
			ind_line[0] = 0;
			for (rowset_idx=0; rowset_idx<rowsetSize; rowset_idx++)
			{
				char str_val[80];
				sprintf(str_val," %-3d",((short *)quadField[idx].ind_ptr)[rowset_idx]);
				strcat(ind_line,str_val);
			}
			DEBUG_OUT_LOC(debug_level,
				("               ind_ptr    = 0x%08x(%s)", quadField[idx].ind_ptr, ind_line),
				filename,
				line);
		} else {
			DEBUG_OUT_LOC(debug_level,
				("               ind_ptr    = 0x%08x", quadField[idx].ind_ptr),
				filename,
				line);
		}
	}
}

void DebugShowOutputValueList(void *output_value_list, long colCount, const char *fcn_name,
		                      const char *filename, unsigned long line)

{
	long debug_level = DEBUG_LEVEL_DATA|DEBUG_LEVEL_METADATA;
	SQLValueList_def *oVL = (SQLValueList_def *) output_value_list;

	DEBUG_OUT_LOC(DEBUG_LEVEL_METADATA,
			("%s 0x%08x", fcn_name, oVL),
			filename,
			line);
	for (int row=0; row < oVL->_length/colCount; row++)
	{
		DEBUG_OUT_LOC(debug_level,
				("%s row=%d",fcn_name,row),
				filename,
				line);
		for (int col=0; col < colCount && col < 4; col++)
		{
			int index = row * colCount + col;
			if (oVL->_buffer->dataType == SQLTYPECODE_VARCHAR)
			{
				char *text = new char[oVL->_buffer[index].dataValue._length+1];
				memcpy(text, oVL->_buffer[index].dataValue._buffer, oVL->_buffer[index].dataValue._length);
				text[oVL->_buffer->dataValue._length] = 0;
				DEBUG_OUT_LOC(debug_level,
						("  SQLTYPECODE_VARCHAR oVL[%d]=%s",col,text),
						filename,
						line);

				//Soln. No.: 10-111229-1174 added correct delete call, commented incorrect syntax below
				//delete text;
				delete[] text;
			}
			if (oVL->_buffer->dataType == SQLTYPECODE_VARCHAR_WITH_LENGTH)
			{ 
				short * len = (short *)oVL->_buffer[index].dataValue._buffer;
				char *text = new char[*len+1];	
				memcpy(text, (char *)oVL->_buffer[index].dataValue._buffer+2, *len);
				text[*len] = 0;
				DEBUG_OUT_LOC(debug_level,
						("  SQLTYPECODE_VARCHAR_WITH_LENGTH oVL[%d]=%s",col,text),
						filename,
						line);

				//Soln. No.: 10-111229-1174 added correct delete call, commented incorrect syntax below
				//delete text;
				delete[] text;
			}
		}
	}
}

const char *DebugJavaObjectInfo(void *param_jenv, void *param_jobj)
{
	static jclass classClass = NULL; 
	static jmethodID getName_id = NULL;
	static char rc[1024];
	JNIEnv *jenv = (JNIEnv *) param_jenv;
	jobject jobj = (jobject) param_jobj;

	if (classClass==NULL)
	{
		jclass cls = jenv->FindClass("java/lang/Class");
		if (cls)
		{
			classClass = (jclass)jenv->NewGlobalRef(cls);
			if (classClass) getName_id = jenv->GetMethodID(classClass, "getName","()Ljava/lang/String;");
		}
	}

	rc[0] = 0;
	jclass objClass = jenv->GetObjectClass(jobj);
	if (objClass==NULL) return("Cannot find class for object");

	if (classClass && getName_id)
	{
		jmethodID getClass_id = jenv->GetMethodID(objClass, "getClass","()Ljava/lang/Class;");
		if (getClass_id)
		{
			jobject classObj = jenv->CallObjectMethod(jobj,getClass_id);
			if (classObj)
			{
				jstring strObj = (jstring) jenv->CallObjectMethod(classObj,getName_id);
				if (strObj)
				{
					const char *text = jenv->GetStringUTFChars(strObj,NULL);
					strcat(rc,"Class Name=");
					if (text)
					{
						strcat(rc,text);
						jenv->ReleaseStringUTFChars(strObj,text);
					} else strcat(rc,"null");
					strcat(rc," ");
				}
			}
		}
	}

	jmethodID toString_id = jenv->GetMethodID(objClass, "toString","()Ljava/lang/String;");
	if (toString_id)
	{
		jstring strObj = (jstring) jenv->CallObjectMethod(jobj,toString_id);
		if (strObj)
		{
			const char *text = jenv->GetStringUTFChars(strObj,NULL);
			strcat(rc,"toString()=");
			if (text)
			{
				size_t text_len = strlen(text);
				size_t rc_len = strlen(rc);
				if ((rc_len+text_len+1)>sizeof(rc)) text_len = sizeof(rc) - rc_len - 1;
				memcpy(rc+rc_len,text,text_len);
				rc[rc_len+text_len] = 0;
				jenv->ReleaseStringUTFChars(strObj,text);
			} else strcat(rc,"null");
		}
	}
	return(rc);
}

const char *DebugSqlAttrTypeStr(long attrName)
{
	static char rc[80];
	switch (attrName)
	{
	case SQL_ATTR_CURSOR_HOLDABLE:
		return("SQL_ATTR_CURSOR_HOLDABLE");
	case SQL_ATTR_INPUT_ARRAY_MAXSIZE:
		return("SQL_ATTR_INPUT_ARRAY_MAXSIZE");
	}
	sprintf(rc,"Unknown (%ld)",attrName);
	return(rc);
}

const char *DebugSqlDiagCondStr(long cond)
{
	static char rc[80];

	switch (cond)
	{
	case SQLDIAG_COND_NUMBER:
		return("SQLDIAG_COND_NUMBER");
	case SQLDIAG_RET_SQLSTATE:
		return("SQLDIAG_RET_SQLSTATE");
	case SQLDIAG_CLASS_ORIG:
		return("SQLDIAG_CLASS_ORIG");
	case SQLDIAG_SUBCLASS_ORIG:
		return("SQLDIAG_SUBCLASS_ORIG");
	case SQLDIAG_SERVER_NAME:
		return("SQLDIAG_SERVER_NAME");
	case SQLDIAG_CONNECT_NAME:
		return("SQLDIAG_CONNECT_NAME");
	case SQLDIAG_CONSTR_CAT:
		return("SQLDIAG_CONSTR_CAT");
	case SQLDIAG_CONSTR_SCHEMA:
		return("SQLDIAG_CONSTR_SCHEMA");
	case SQLDIAG_CONSTR_NAME:
		return("SQLDIAG_CONSTR_NAME");
	case SQLDIAG_CATALOG_NAME:
		return("SQLDIAG_CATALOG_NAME");
	case SQLDIAG_SCHEMA_NAME:
		return("SQLDIAG_SCHEMA_NAME");
	case SQLDIAG_TABLE_NAME:
		return("SQLDIAG_TABLE_NAME");
	case SQLDIAG_COLUMN_NAME:
		return("SQLDIAG_COLUMN_NAME");
	case SQLDIAG_CURSOR_NAME:
		return("SQLDIAG_CURSOR_NAME");
	case SQLDIAG_MSG_TEXT:
		return("SQLDIAG_MSG_TEXT");
	case SQLDIAG_MSG_LEN:
		return("SQLDIAG_MSG_LEN");
	case SQLDIAG_MSG_OCTET_LEN:
		return("SQLDIAG_MSG_OCTET_LEN");
	case SQLDIAG_TRIGGER_CAT:
		return("SQLDIAG_TRIGGER_CAT");
	case SQLDIAG_TRIGGER_SCHEMA:
		return("SQLDIAG_TRIGGER_SCHEMA");
	case SQLDIAG_TRIGGER_NAME:
		return("SQLDIAG_TRIGGER_NAME");
	case SQLDIAG_COLUMN_NUMBER:
		return("SQLDIAG_COLUMN_NUMBER");
	case SQLDIAG_NATIVE:
		return("SQLDIAG_NATIVE");
	case SQLDIAG_ROW_NUMBER:
		return("SQLDIAG_ROW_NUMBER");
	case SQLDIAG_SOURCE_FILE:
		return("SQLDIAG_SOURCE_FILE");
	case SQLDIAG_LINE_NUMBER:
		return("SQLDIAG_LINE_NUMBER");
	case SQLDIAG_SUBSYSTEM_ID:
		return("SQLDIAG_SUBSYSTEM_ID");
	case SQLDIAG_SQLCODE:
		return("SQLDIAG_SQLCODE");
	case SQLDIAG_NSK_CODE:
		return("SQLDIAG_NSK_CODE");
	}
	sprintf(rc,"Unknown(%ld)",cond);
	return(rc);
}

const char *DebugSqlWhatDescStr(long what_desc)
{
	static char rc[80];
	switch (what_desc)
	{
	case SQLWHAT_INPUT_DESC:
		return("SQLWHAT_INPUT_DESC");
	case SQLWHAT_OUTPUT_DESC:
		return("SQLWHAT_OUTPUT_DESC");
	}
	sprintf(rc,"Unknown(%ld)",what_desc);
	return(rc);
}

#endif /* _DEBUG */

SQLCLI_LIB_FUNC long CliDebug_SetStmtAttr(SQLSTMT_ID *statement_id, long attrName,
										  long numeric_value, char *string_value,
										  const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_SetStmtAttr",("statement_id=0x%08x,attrName=%s,numeric_value=%ld,string_value=%s",
			statement_id,
			DebugSqlAttrTypeStr(attrName),
			numeric_value,
			DebugString(string_value)),
		filename, line);
	
	long rc = SQL_EXEC_SetStmtAttr(statement_id, attrName, numeric_value, string_value);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_SetDescItem(SQLDESC_ID *sql_descriptor, long entry,
										  long what_to_set, long numeric_value, char * string_value,
										  const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_SetDescItem",("sql_descriptor=0x%08x, entry=%d, what_to_set=%s, numeric_value=%ld, string_value=%s",
		   sql_descriptor,
		   entry,
		   CliDebugDescItemStr(what_to_set),
		   numeric_value,
		   DebugString(string_value)),
		filename, line);

	long rc = SQL_EXEC_SetDescItem(sql_descriptor, entry, what_to_set, numeric_value, string_value);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_SETROWSETDESCPOINTERS(SQLDESC_ID * desc_id, long rowset_size, int *rowset_status_ptr,
													long starting_entry, long num_quadruple_fields,
													struct SQLCLI_QUAD_FIELDS quad_fields[],
													const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_SETROWSETDESCPOINTERS",("desc_id=0x%08x, rowset_size=%ld, rowset_status_ptr=0x%08x, starting_entry=%ld, num_quadruple_fields=%ld, quad_fields=0x%08x",
			desc_id,
			rowset_size,
			rowset_status_ptr,
			starting_entry,
			num_quadruple_fields,
			quad_fields),
		filename, line);

	long rc = SQL_EXEC_SETROWSETDESCPOINTERS(desc_id, rowset_size, rowset_status_ptr, starting_entry, num_quadruple_fields, quad_fields);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_GetDescItems2(SQLDESC_ID * sql_descriptor,
											long no_of_desc_items, SQLDESC_ITEM desc_items[],
											const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_GetDescItems2",("sql_descriptor=0x%08x, no_of_desc_items=%d, desc_items=0x%08x",
			sql_descriptor,
			no_of_desc_items,
			desc_items),
		filename, line);

	long rc = SQL_EXEC_GetDescItems2(sql_descriptor, no_of_desc_items, desc_items);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_SetCursorName(SQLSTMT_ID * statement_id, SQLSTMT_ID * cursor_name,
											const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_SetCursorName",("statement_id=0x%08x, cursor_name=0x%08x)",
			statement_id,
			cursor_name),
		filename, line);
			
	long rc = SQL_EXEC_SetCursorName(statement_id, cursor_name);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_ExecFetch(SQLSTMT_ID * statement_id,
										SQLDESC_ID * input_descriptor, long num_ptr_pairs,
										const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_ExecFetch",("statement_id=0x%08x, input_descriptor=0x%08x, num_ptr_pairs=%d",
			statement_id,
			input_descriptor,
			num_ptr_pairs),
		filename, line);

	long rc = SQL_EXEC_ExecFetch(statement_id, input_descriptor, num_ptr_pairs);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_CloseStmt(SQLSTMT_ID *statement_id,
										const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_CloseStmt",("statement_id=0x%08x",
			statement_id),
		filename, line);

	long rc = SQL_EXEC_CloseStmt(statement_id);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_GetDiagnosticsCondInfo2(long what_to_get, long conditionNum,
													  int *numeric_value, char * string_value,
													  long max_string_len, int *len_of_item,
													  const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_GetDiagnosticsCondInfo2",("what_to_get=%s, conditionNum=%ld, numeric_value=0x%08x, string_value=0x%08x, max_string_len=%ld, len_of_item=0x%08x",
			DebugSqlDiagCondStr(what_to_get),
			conditionNum,
			numeric_value,
			string_value,
			max_string_len,
			len_of_item),
		filename, line);

	if (numeric_value) *numeric_value = 0;
	if (max_string_len) *string_value = 0;
	long rc = SQL_EXEC_GetDiagnosticsCondInfo2(what_to_get, conditionNum, numeric_value, string_value, max_string_len, len_of_item);
	if (numeric_value)
		DEBUG_OUT_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,("numeric_value=%ld",*numeric_value),filename,line);
	if (max_string_len)
		DEBUG_OUT_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,("string_value=%s",string_value),filename,line);

	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_SwitchContext(SQLCTX_HANDLE context_handle, SQLCTX_HANDLE * prev_context_handle,
										    const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_SwitchContext",("context_handle=%ld, prev_context_handle=0x%08x",
			context_handle,
			prev_context_handle),
		filename, line);
	SQLCTX_HANDLE prev_handle = 0;

	long rc = SQL_EXEC_SwitchContext(context_handle, &prev_handle);
	if (prev_context_handle) *prev_context_handle = prev_handle;
	DEBUG_OUT_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,("prev_handle=%ld",prev_handle),
		filename,line);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_DeleteContext(SQLCTX_HANDLE contextHandle,
										    const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_DeleteContext",("contextHandle=%ld",
			contextHandle),
		filename, line);

	long rc = SQL_EXEC_DeleteContext(contextHandle);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_CreateContext(SQLCTX_HANDLE *context_handle,
											char* sqlAuthId, long suppressAutoXactStart,
										    const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_CreateContext",("context_handle=0x%08x, sqlAuthId=0x%08x, suppressAutoXactStart=%ld",
			context_handle,
			sqlAuthId,
			suppressAutoXactStart),
		filename, line);

	long rc = SQL_EXEC_CreateContext(context_handle, sqlAuthId, suppressAutoXactStart);
	DEBUG_OUT_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,("context_handle=%ld",*context_handle),
		filename,line);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_CurrentContext(SQLCTX_HANDLE *contextHandle,
						const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_CurrentContext",("contextHandle=0x%08x",
			contextHandle),
		filename, line);

	long rc = SQL_EXEC_CurrentContext(contextHandle);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_ClearDiagnostics (SQLSTMT_ID *statement_id,
											    const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_ClearDiagnostics",("statement_id=0x%08x",
			statement_id),
		filename, line);

	long rc = SQL_EXEC_ClearDiagnostics(statement_id);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_GetDiagnosticsStmtInfo2(SQLSTMT_ID *statement_id, long what_to_get, void *numeric_value, char *string_value,
													  long max_string_len, int *len_of_item,
													  const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_GetDiagnosticsStmtInfo2",("statement_id=0x%08x, what_to_get=%s, numeric_value=0x%08x, string_value=0x%08x, max_string_len=%ld, len_of_item=%ld",
			statement_id,
			DebugSqlDiagCondStr(what_to_get),
			numeric_value,
			string_value,
			max_string_len,
			len_of_item),
		filename, line);

	long rc = SQL_EXEC_GetDiagnosticsStmtInfo2(statement_id, what_to_get, numeric_value, string_value, max_string_len, len_of_item);
	if (numeric_value)
		DEBUG_OUT_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,("numeric_value=%ld",(Int64 *)numeric_value),filename,line);
	if (max_string_len)
		DEBUG_OUT_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,("string_value=%s",string_value),filename,line);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_DeallocDesc(SQLDESC_ID *desc_id,
										  const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_DeallocDesc",("desc_id=0x%08x",
			desc_id),
		filename, line);

	long rc = SQL_EXEC_DeallocDesc(desc_id);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_DeallocStmt(SQLSTMT_ID *statement_id,
										  const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_DeallocStmt",("statement_id=0x%08x",
			statement_id),
		filename, line);

	long rc = SQL_EXEC_DeallocStmt(statement_id);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}
SQLCLI_LIB_FUNC long CliDebug_ExecDirect(SQLSTMT_ID *statement_id, SQLDESC_ID *sql_source,
										 SQLDESC_ID *input_descriptor, long num_ptr_pairs,
										 const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_ExecDirect",("statement_id=0x%08x, sql_source=0x%08x, input_descriptor=0x%08x, num_ptr_pairs=%ld",
			statement_id,
			sql_source,
			input_descriptor,
			num_ptr_pairs),
		filename, line);

	long rc = SQL_EXEC_ExecDirect(statement_id, sql_source, input_descriptor, num_ptr_pairs);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_Exec(SQLSTMT_ID *statement_id, SQLDESC_ID *input_descriptor,long num_ptr_pairs,
								   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_Exec",("statement_id=0x%08x, input_descriptor=0x%08x, num_ptr_pairs=%ld",
			statement_id,
			input_descriptor,
			num_ptr_pairs),
		filename, line);


	long rc = SQL_EXEC_Exec(statement_id, input_descriptor, num_ptr_pairs);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_Prepare(SQLSTMT_ID *statement_id, SQLDESC_ID *sql_source,
									  const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_Prepare",("statement_id=0x%08x,sql_source=0x%08x",
			statement_id,
			sql_source),
		filename, line);

	long rc = SQL_EXEC_Prepare(statement_id, sql_source);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_DescribeStmt(SQLSTMT_ID *statement_id,
										   SQLDESC_ID *input_descriptor, SQLDESC_ID *output_descriptor,
										   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_DescribeStmt",("statement_id=0x%08x,input_descriptor=0x%08x,output_descriptor=0x%08x",
			statement_id,
			input_descriptor,
			output_descriptor),
		filename, line);

	long rc = SQL_EXEC_DescribeStmt(statement_id, input_descriptor, output_descriptor);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_GetDescEntryCount(SQLDESC_ID *sql_descriptor, int *num_entries,
												const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_GetDescEntryCount",("sql_descriptor=0x%08x, num_entries=0x%08x",
			sql_descriptor,
			num_entries),
		filename, line);

	long rc = SQL_EXEC_GetDescEntryCount(sql_descriptor, num_entries);
	if (num_entries)
		DEBUG_OUT_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,("num_entries=%ld",*num_entries),filename,line);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_Fetch(SQLSTMT_ID *statement_id, SQLDESC_ID *output_descriptor, long num_ptr_pairs,
									const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_Fetch",("statement_id=0x%08x, output_descriptor=0x%08x, num_ptr_pairs=%ld",
			statement_id,
			output_descriptor,
			num_ptr_pairs),
		filename, line);

	long rc = SQL_EXEC_Fetch(statement_id, output_descriptor, num_ptr_pairs);

	DEBUG_OUT_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,("statement_id=0x%08x, output_descriptor=0x%08x, num_ptr_pairs=%ld",
		statement_id,output_descriptor,num_ptr_pairs),
		filename,line);

	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_GetDescItem(SQLDESC_ID *sql_descriptor, long entry,
										  long what_to_get, int *numeric_value, char *string_value, long max_string_len,
										  int *len_of_item, long start_from_offset,
										  const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_GetDescItem",("sql_descriptor=0x%08x, entry=%ld, what_to_get=%s, numeric_value=0x%08x, string_value=0x%08x, max_string_len=%ld, len_of_item=0x%08x, start_from_offset=%ld",
			sql_descriptor,
			entry,
			CliDebugDescItemStr(what_to_get),
			numeric_value,
			string_value,
			max_string_len,
			len_of_item,
			start_from_offset),
		filename, line);

	if (numeric_value) *numeric_value = 0;
	if (max_string_len) *string_value = 0;
	long rc = SQL_EXEC_GetDescItem(sql_descriptor, entry, what_to_get, numeric_value, string_value, max_string_len, len_of_item, start_from_offset);
	if (numeric_value)
		DEBUG_OUT_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,("numeric_value=%ld",*numeric_value),filename,line);
	if (max_string_len)
		DEBUG_OUT_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,("string_value=%s",string_value),filename,line);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_Cancel(SQLSTMT_ID *statement_id,
									 const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_Cancel",("statement_id=0x%08x",
			statement_id),
		filename, line);

	long rc = SQL_EXEC_Cancel(statement_id);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}
SQLCLI_LIB_FUNC long CliDebug_AllocStmt(SQLSTMT_ID * new_statement_id, SQLSTMT_ID *cloned_statement,
										const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_AllocStmt",("new_statement_id=0x%08x, cloned_statement=0x%08x",
			new_statement_id,
			cloned_statement),
		filename, line);

	long rc = SQL_EXEC_AllocStmt(new_statement_id,cloned_statement);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_ResDescName(SQLDESC_ID *statement_id, SQLSTMT_ID *from_statement, long what_desc,
										  const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_ResDescName",("statement_id=0x%08x, from_statement=0x%08x, what_desc=%s",
			statement_id,
			from_statement,
			DebugSqlWhatDescStr(what_desc)),
		filename, line);

	long rc = SQL_EXEC_ResDescName(statement_id, from_statement, what_desc);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_AllocDesc(SQLDESC_ID *desc_id, SQLDESC_ID *input_descriptor,
										const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_AllocDesc",("desc_id=0x%08x, input_descriptor=0x%08x",
			desc_id,
			input_descriptor),
		filename, line);

	long rc = SQL_EXEC_AllocDesc(desc_id, input_descriptor);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_AllocStmtForRS (SQLSTMT_ID *callStmtId, long resultSetIndex, 
											  SQLSTMT_ID *resultSetStmtId, const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelStmt,"SQL_EXEC_AllocStmtForRS",
		("callStmtId=0x%08x, resultSetIndex=%ld, resultSetStmtId=0x%08x",
			callStmtId,
			resultSetIndex,
			resultSetStmtId),
		filename, line);

	long retcode = SQL_SUCCESS;
#ifdef NSK_PLATFORM	// Linux port 	
	wAllocStmtForRS(callStmtId, resultSetIndex, resultSetStmtId);
#endif
	CLI_DEBUG_RETURN_SQL_LOC(retcode,filename,line);

}

SQLCLI_LIB_FUNC long CliDebug_AssocFileNumber(SQLSTMT_ID *statement_id, short file_number,
											  const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_AssocFileNumber",("statement_id=0x%08x, file_number=%d",
			statement_id,
			file_number),
		filename, line);

	long rc = SQL_EXEC_AssocFileNumber(statement_id, file_number);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_CLI_VERSION(const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_CLI_VERSION",(NULL),
		filename, line);

	long rc = SQL_EXEC_CLI_VERSION();
	FUNCTION_RETURN_NUMERIC_LOC(rc,(NULL),filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_ClearExecFetchClose(SQLSTMT_ID *statement_id, SQLDESC_ID *input_descriptor,
												  SQLDESC_ID* output_descriptor, long num_input_ptr_pairs,
												  long num_output_ptr_pairs, long num_total_ptr_pairs,
												  const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_ClearExecFetchClose",
		("statement_id=0x%08x, input_descriptor=0x%08x, output_descriptor=0x%08x, num_input_ptr_pairs=%ld, num_output_ptr_pairs=%ld, num_total_ptr_pairs=%ld",
			statement_id,
			input_descriptor,
			output_descriptor,
			num_input_ptr_pairs,
			num_output_ptr_pairs,
			num_total_ptr_pairs),
		filename, line);

	long rc = SQL_EXEC_ClearExecFetchClose(statement_id,
										   input_descriptor,
                                           output_descriptor,
										   num_input_ptr_pairs,
										   num_output_ptr_pairs,
										   num_total_ptr_pairs);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

SQLCLI_LIB_FUNC long CliDebug_GetStmtAttr(SQLSTMT_ID *statement_id, long attrName,
										  int *numeric_value, char *string_value,
										  long max_string_len, int *len_of_item,
										  const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,"SQL_EXEC_GetStmtAttr",
		("statement_id=0x%08x, attrName=%s, numeric_value=0x%08x, string_value=0x%08x, max_string_len=%ld, len_of_item=0x%08x",
			statement_id,
			CliDebugSqlAttrType(attrName),
			numeric_value,
			string_value,
			max_string_len,
			len_of_item),
		filename, line);

	if (numeric_value) *numeric_value = 0;
	if (max_string_len) *string_value = 0;
	long rc = SQL_EXEC_GetStmtAttr(statement_id,
								   attrName,
								   numeric_value,
								   string_value,
                                   max_string_len,
								   len_of_item);
	if (numeric_value)
		DEBUG_OUT_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,("numeric_value=%ld",*numeric_value),filename,line);
	if (max_string_len)
		DEBUG_OUT_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelCLI,("string_value=%s",string_value),filename,line);
	CLI_DEBUG_RETURN_SQL_LOC(rc,filename,line);
}

jsize JNIDebug_GetArrayLength(JNIEnv *jenv, jarray array,
							  const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->GetArrayLength",(NULL),
		filename, line);
	jsize dataLen = jenv->GetArrayLength(array);
	FUNCTION_RETURN_NUMERIC_LOC(dataLen,(NULL),filename,line);
}


jobject JNIDebug_GetObjectArrayElement(JNIEnv *jenv, jobjectArray array, jsize index,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->GetObjectArrayElement",
		("index=%ld",index),
		filename, line);
	jobject jobj = jenv->GetObjectArrayElement(array, index);
	FUNCTION_RETURN_PTR_LOC(jobj,(NULL),filename,line);
}

jobject JNIDebug_GetObjectField(JNIEnv *jenv, jobject jobj, jfieldID fieldID,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->GetObjectField",(NULL),
		filename, line);
	jobject rc = jenv->GetObjectField(jobj,fieldID);
	FUNCTION_RETURN_PTR_LOC(rc,(NULL),filename,line);
}

jbyte *JNIDebug_GetByteArrayElements(JNIEnv *jenv, jbyteArray array, jboolean *isCopy,
									const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->GetByteArrayElements",(NULL),
		filename, line);
	jbyte *count = jenv->GetByteArrayElements(array, isCopy);
	FUNCTION_RETURN_PTR_LOC(count,(NULL),filename,line);
}

void JNIDebug_ReleaseByteArrayElements(JNIEnv *jenv, jbyteArray array, jbyte *elems, jint mode,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->ReleaseByteArrayElements",(NULL),
		filename, line);
	jenv->ReleaseByteArrayElements(array, elems, mode);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

jshort *JNIDebug_GetShortArrayElements(JNIEnv *jenv, jshortArray array, jboolean *isCopy,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->GetShortArrayElements",(NULL),
		filename, line);
	jshort *rc = jenv->GetShortArrayElements(array,isCopy);
	FUNCTION_RETURN_PTR_LOC(rc,(NULL),filename,line);
}

void JNIDebug_ReleaseShortArrayElements(JNIEnv *jenv, jshortArray array, jshort *elems, jint mode,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->ReleaseShortArrayElements",(NULL),
		filename, line);
	jenv->ReleaseShortArrayElements(array, elems, mode);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

jint *JNIDebug_GetIntArrayElements(JNIEnv *jenv, jintArray array, jboolean *isCopy,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->GetIntArrayElements",(NULL),
		filename, line);
	jint *rc = jenv->GetIntArrayElements(array,isCopy);
	FUNCTION_RETURN_PTR_LOC(rc,(NULL),filename,line);
}

void JNIDebug_ReleaseIntArrayElements(JNIEnv *jenv, jintArray array, jint *elems, jint mode,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->ReleaseIntArrayElements",(NULL),
		filename, line);
	jenv->ReleaseIntArrayElements(array, elems, mode);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

jlong *JNIDebug_GetLongArrayElements(JNIEnv *jenv, jlongArray array, jboolean *isCopy,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->GetLongArrayElements",(NULL),
		filename, line);
	jlong *rc = jenv->GetLongArrayElements(array,isCopy);
	FUNCTION_RETURN_PTR_LOC(rc,(NULL),filename,line);
}

void JNIDebug_ReleaseLongArrayElements(JNIEnv *jenv, jlongArray array, jlong *elems, jint mode,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->ReleaseLongArrayElements",(NULL),
		filename, line);
	jenv->ReleaseLongArrayElements(array, elems, mode);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

jfloat *JNIDebug_GetFloatArrayElements(JNIEnv *jenv, jfloatArray array, jboolean *isCopy,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->GetFloatArrayElements",(NULL),
		filename, line);
	jfloat *rc = jenv->GetFloatArrayElements(array,isCopy);
	FUNCTION_RETURN_PTR_LOC(rc,(NULL),filename,line);
}

void JNIDebug_ReleaseFloatArrayElements(JNIEnv *jenv, jfloatArray array, jfloat *elems, jint mode,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->ReleaseFloatArrayElements",(NULL),
		filename, line);
	jenv->ReleaseFloatArrayElements(array, elems, mode);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

jdouble *JNIDebug_GetDoubleArrayElements(JNIEnv *jenv, jdoubleArray array, jboolean *isCopy,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->GetDoubleArrayElements",(NULL),
		filename, line);
	jdouble *rc = jenv->GetDoubleArrayElements(array,isCopy);
	FUNCTION_RETURN_PTR_LOC(rc,(NULL),filename,line);
}

void JNIDebug_ReleaseDoubleArrayElements(JNIEnv *jenv, jdoubleArray array, jdouble *elems, jint mode,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->ReleaseDoubleArrayElements",(NULL),
		filename, line);
	jenv->ReleaseDoubleArrayElements(array, elems, mode);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

jboolean *JNIDebug_GetBooleanArrayElements(JNIEnv *jenv, jbooleanArray array, jboolean *isCopy,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->GetBooleanArrayElements",(NULL),
		filename, line);
	jboolean *rc = jenv->GetBooleanArrayElements(array,isCopy);
	FUNCTION_RETURN_PTR_LOC(rc,(NULL),filename,line);
}

void JNIDebug_ReleaseBooleanArrayElements(JNIEnv *jenv, jbooleanArray array, jboolean *elems, jint mode,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->ReleaseBooleanArrayElements",(NULL),
		filename, line);
	jenv->ReleaseBooleanArrayElements(array, elems, mode);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

const char *JNIDebug_GetStringUTFChars(JNIEnv *jenv, jstring jstr, jboolean *isCopy,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->GetStringUTFChars",(NULL),
		filename, line);
	const char *rc = jenv->GetStringUTFChars(jstr,isCopy);
	FUNCTION_RETURN_PTR_LOC(rc,(NULL),filename,line);
}

void JNIDebug_ReleaseStringUTFChars(JNIEnv *jenv, jstring jstr, const char* chars,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->ReleaseStringUTFChars",(NULL),
		filename, line);
	jenv->ReleaseStringUTFChars(jstr, chars);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

jmethodID JNIDebug_GetMethodID(JNIEnv *jenv, jclass jcls, const char *name, const char *sig,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->GetMethodID",
		("name='%s' sig='%s'",name,sig),
		filename, line);
	jmethodID rc = jenv->GetMethodID(jcls, name, sig);
	FUNCTION_RETURN_PTR_LOC(rc,(NULL),filename,line);
}

jfieldID JNIDebug_GetFieldID(JNIEnv *jenv, jclass jcls, const char *name, const char *sig,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->GetFieldID",
		("name='%s' sig='%s'",name,sig),
		filename, line);
	jfieldID rc = jenv->GetFieldID(jcls, name, sig);
	FUNCTION_RETURN_PTR_LOC(rc,(NULL),filename,line);
}

jclass JNIDebug_GetObjectClass(JNIEnv *jenv, jobject jobj,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->GetObjectClass",(NULL),
		filename, line);
	jclass rc = jenv->GetObjectClass(jobj);
	FUNCTION_RETURN_PTR_LOC(rc,(NULL),filename,line);
}

jboolean JNIDebug_IsInstanceOf(JNIEnv *jenv, jobject jobj, jclass clazz,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->IsInstanceOf",
		("jobj=0x%08x, clazz=0x%08x",jobj,clazz),
		filename, line);
	jboolean rc = jenv->IsInstanceOf(jobj,clazz);
	FUNCTION_RETURN_NUMERIC_LOC(rc,(NULL),filename,line);
}

void JNIDebug_SetObjectField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, jobject val,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetObjectField",
		(NULL),
		filename, line);
	jenv->SetObjectField(jobj,fieldID,val);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

void JNIDebug_SetBooleanField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, jboolean val,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetBooleanField",
		("val=%d",val),
		filename, line);
	jenv->SetBooleanField(jobj, fieldID, val);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

void JNIDebug_SetByteField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, jbyte val,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetByteField",
		("val=%d",val),
		filename, line);
	jenv->SetByteField(jobj, fieldID, val);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

void JNIDebug_SetShortField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, jshort val,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetShortField",
		("val=%d",val),
		filename, line);
	jenv->SetShortField(jobj, fieldID, val);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}
void JNIDebug_SetIntField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, jint val,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetIntField",
		("val=%ld",val),
		filename, line);
	jenv->SetIntField(jobj, fieldID, val);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

void JNIDebug_SetLongField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, jlong val,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetLongField",
		("val=%Ld",val),
		filename, line);
	jenv->SetLongField(jobj, fieldID, val);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

void JNIDebug_SetFloatField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, jfloat val,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetFloatField",
		("val=%f",val),
		filename, line);
	jenv->SetFloatField(jobj, fieldID, val);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

void JNIDebug_SetDoubleField(JNIEnv *jenv, jobject jobj, jfieldID fieldID, jdouble val,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetDoubleField",
		("val=%g",val),
		filename, line);
	jenv->SetDoubleField(jobj, fieldID, val);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

void JNIDebug_SetObjectArrayElement(JNIEnv *jenv, jobjectArray array, jsize index, jobject val,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetObjectArrayElement",
		("index=%ld",index),
		filename, line);
	jenv->SetObjectArrayElement(array, index, val);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}


void JNIDebug_SetBooleanArrayRegion(JNIEnv *jenv, jbooleanArray array, jsize start, jsize len, jboolean *buf,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetBooleanArrayRegion",
		((NULL)),
		filename, line);
	jenv->SetBooleanArrayRegion(array, start, len, buf);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

void JNIDebug_SetByteArrayRegion(JNIEnv *jenv, jbyteArray array, jsize start, jsize len, jbyte *buf,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetByteArrayRegion",
		((NULL)),
		filename, line);
	jenv->SetByteArrayRegion(array, start, len, buf);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

void JNIDebug_SetCharArrayRegion(JNIEnv *jenv, jcharArray array, jsize start, jsize len, jchar *buf,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetCharArrayRegion",
		((NULL)),
		filename, line);
	jenv->SetCharArrayRegion(array, start, len, buf);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

void JNIDebug_SetShortArrayRegion(JNIEnv *jenv, jshortArray array, jsize start, jsize len, jshort *buf,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetShortArrayRegion",
		((NULL)),
		filename, line);
	jenv->SetShortArrayRegion(array, start, len, buf);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

void JNIDebug_SetIntArrayRegion(JNIEnv *jenv, jintArray array, jsize start, jsize len, jint *buf,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetIntArrayRegion",
		((NULL)),
		filename, line);
	jenv->SetIntArrayRegion(array, start, len, buf);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

void JNIDebug_SetLongArrayRegion(JNIEnv *jenv, jlongArray array, jsize start, jsize len, jlong *buf,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetLongArrayRegion",
		((NULL)),
		filename, line);
	jenv->SetLongArrayRegion(array, start, len, buf);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

void JNIDebug_SetFloatArrayRegion(JNIEnv *jenv, jfloatArray array, jsize start, jsize len, jfloat *buf,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetFloatArrayRegion",
		((NULL)),
		filename, line);
	jenv->SetFloatArrayRegion(array, start, len, buf);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

void JNIDebug_SetDoubleArrayRegion(JNIEnv *jenv, jdoubleArray array, jsize start, jsize len, jdouble *buf,
									   const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->SetDoubleArrayRegion",
		((NULL)),
		filename, line);
	jenv->SetDoubleArrayRegion(array, start, len, buf);
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

jbyteArray JNIDebug_NewByteArray(JNIEnv *jenv, jsize len,
								 const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->NewByteArray",
		("len=%ld",len),
		filename, line);
	jbyteArray rc = jenv->NewByteArray(len);
	FUNCTION_RETURN_PTR_LOC(rc,(NULL),filename,line);
}

jintArray JNIDebug_NewIntArray(JNIEnv *jenv, jsize len,
								 const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->NewIntArray",
		("len=%ld",len),
		filename, line);
	jintArray rc = jenv->NewIntArray(len);
	FUNCTION_RETURN_PTR_LOC(rc,(NULL),filename,line);
}

jobjectArray JNIDebug_NewObjectArray(JNIEnv *jenv, jsize len, jclass clazz, jobject init,
								 const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->NewObjectArray",
		("len=%ld",len),
		filename, line);
	jobjectArray rc = jenv->NewObjectArray(len, clazz, init);
	FUNCTION_RETURN_PTR_LOC(rc,(NULL),filename,line);
}

void JNIDebug_ExceptionClear(JNIEnv *jenv,
							 const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->ExceptionClear",
		((NULL)),
		filename, line);
	jenv->ExceptionClear();
	FUNCTION_RETURN_VOID_LOC((NULL),filename,line);
}

jint JNIDebug_Throw(JNIEnv *jenv, jthrowable obj,
					const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->Throw",
		((NULL)),
		filename, line);
	jint rc = jenv->Throw(obj);
	FUNCTION_RETURN_NUMERIC_LOC(rc,(NULL),filename,line);
}

jclass JNIDebug_FindClass(JNIEnv *jenv, const char *name,
					const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->FindClass",
		(("name=%s",name)),
		filename, line);
	jclass rc = jenv->FindClass(name);
	FUNCTION_RETURN_PTR_LOC(rc,(NULL),filename,line);
}
jobject JNIDebug_NewGlobalRef(JNIEnv *jenv, jobject lobj,
					const char *filename, unsigned long line)
{
	FUNCTION_ENTRY_LEVEL_LOC(org_apache_trafodion_jdbc_t2_JdbcDebug_debugLevelEntry,"jenv->NewGlobalRef",
		((NULL)),
		filename, line);
	jobject rc = jenv->NewGlobalRef(lobj);
	FUNCTION_RETURN_PTR_LOC(rc,(NULL),filename,line);
}

const char *WrapperDataTypeStr(jbyte dataType)
{
	static char rc[80];
	switch (dataType)
	{
	case org_apache_trafodion_jdbc_t2_DataWrapper_UNKNOWN:
		return("DataWrapper.UNKNOWN");
	case org_apache_trafodion_jdbc_t2_DataWrapper_BYTE:
		return("DataWrapper.BYTE");
	case org_apache_trafodion_jdbc_t2_DataWrapper_SHORT:
		return("DataWrapper.SHORT");
	case org_apache_trafodion_jdbc_t2_DataWrapper_INTEGER:
		return("DataWrapper.INTEGER");
	case org_apache_trafodion_jdbc_t2_DataWrapper_LONG:
		return("DataWrapper.LONG");
	case org_apache_trafodion_jdbc_t2_DataWrapper_FLOAT:
		return("DataWrapper.FLOAT");
	case org_apache_trafodion_jdbc_t2_DataWrapper_DOUBLE:
		return("DataWrapper.DOUBLE");
	case org_apache_trafodion_jdbc_t2_DataWrapper_BOOLEAN:
		return("DataWrapper.BOOLEAN");
	case org_apache_trafodion_jdbc_t2_DataWrapper_STRING:
		return("DataWrapper.STRING");
	case org_apache_trafodion_jdbc_t2_DataWrapper_BYTES:
		return("DataWrapper.BYTES");
	case org_apache_trafodion_jdbc_t2_DataWrapper_BLOB:
		return("DataWrapper.BLOB");
	case org_apache_trafodion_jdbc_t2_DataWrapper_CLOB:
		return("DataWrapper.CLOB");
	case org_apache_trafodion_jdbc_t2_DataWrapper_BIG_DECIMAL:
		return("DataWrapper.BIG_DECIMAL");
	case org_apache_trafodion_jdbc_t2_DataWrapper_OBJECT:
		return("DataWrapper.OBJECT");
	}
	sprintf(rc,"Unknown DataWrapper data type (%ld)",dataType);
	return(rc);
}
