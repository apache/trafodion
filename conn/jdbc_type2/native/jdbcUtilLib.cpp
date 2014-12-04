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
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>
#include <sys/times.h>
#include "jdbcUtil.h"

#define MAX_PROCESSES	100
#define SHARED_MEMORY_SIZE (sizeof(struct JdbcUtilMemoryStruct) + (MAX_PROCESSES-1) * sizeof(JdbcUtilProcessTableStruct))

#define SEM_MEMORY_ACCESS	0
#define SEM_SLEEP			1
#define TOTAL_SEM			2

JdbcUtil::JdbcUtil()
{
	errorMsg[0] = 0;
	sharedMemory = NULL;
	sharedMemoryId = -1;
	semaphoreId = -1;
	memoryKey = 1225;
	memoryIdx = -1;
	thisProcessId = -1;
}

JdbcUtil::~JdbcUtil(void)
{
	detachMemory();
}

void JdbcUtil::setPerror(const char *action)
{
	setPerror(errno,action);
}

void JdbcUtil::setPerror(int err, const char *action)
{
	errorCode = err;
	char *perr = new char[1024]; //1K should be enough for system error message.
#if (_POSIX_C_SOURCE >= 200112L || _XOPEN_SOURCE >= 600) && ! _GNU_SOURCE
	int ret=strerror_r(errorCode,perr,1024);
	if (ret==-1) strcpy(perr,"Unknown");
#else
	strerror_r(errorCode,perr,1024);
#endif

	sprintf(errorMsg,"%s.  Error=%d - %s",action,errorCode,perr);
	delete []perr;
}

void JdbcUtil::setError(const char *action)
{
	if (action)
	{
		sprintf(errorMsg,"%s.",action);
		errorCode = -1;
	}
	else
	{
		errorMsg[0] = 0;
		errorCode = 0;
	}
}

const char *JdbcUtil::getError(void)
{
	return errorMsg;
}

int JdbcUtil::getErrorCode(void)
{
	return errorCode;
}

bool JdbcUtil::getSemaphores(bool create)
{
	setError(NULL);
	
	if (semaphoreId == -1)
	{
		int semid;
		if ((semid = semget(memoryKey, 0, 0666 ))==-1)
		{
			if ((errno==ENOENT) && create)
			{
				if ((semid = semget(memoryKey, TOTAL_SEM, IPC_CREAT | 0666 ))==-1)
				{
					setPerror("Cannot create semaphores");
					return false;
				}
			}
			else
			{
				setPerror("Cannot get semaphores");
				return false;
			}
		}
		semaphoreId = semid;
	}
	return true;
}

bool JdbcUtil::lockSemaphore(unsigned short sem_idx, bool lock)
{
	setError(NULL);
	
	if (lock && !getSemaphores(false)) return false;

	const char *lock_str;
	if (lock) lock_str = "lock";
	else lock_str = "unlock";
	if (semaphoreId==-1)
	{
		char rc[80];
		sprintf(rc,"Cannot %s semaphore when semaphore not attached",lock_str);
		setError(rc);
		return false;
	}
	struct sembuf sem_op;
	sem_op.sem_num = sem_idx;
	if (lock) sem_op.sem_op = -1;
	else sem_op.sem_op = 1;
	sem_op.sem_flg = 0;
	if (semop(semaphoreId,&sem_op,1)==-1)
	{
		char rc[80];
		sprintf(rc,"Error during %s of semaphore",lock_str);
		setError(rc);
		return false;
	}
	return true;
}

bool JdbcUtil::removeSemaphores(void)
{
	setError(NULL);
	
	if (semaphoreId!=-1)
	{
		if (semctl(semaphoreId, 0, IPC_RMID) == -1)
		{
			setPerror("Cannot remove semaphore");
			return false;
		}
		semaphoreId = -1;
	}
	return true;

}

bool JdbcUtil::getMemory(bool create)
{
	setError(NULL);

	if (sharedMemoryId==-1)
	{
		int shmid;

		// Try to get the memory
		if ((shmid = shmget(
			memoryKey,
			0,
			0666)) < 0)
		{
			if ((errno==ENOENT) && create)
			{
				// Does not exist.  Try to create.
				if ((shmid = shmget(
					memoryKey,
					SHARED_MEMORY_SIZE,
					IPC_CREAT | 0666)) < 0)
				{
					setPerror("Cannot create shared memory");
					return false;
				}
			}
			else
			{
				setPerror("Cannot open shared memory");
				return false;
			}
		}

		sharedMemoryId = shmid;
	}
	return true;
}

bool JdbcUtil::attachMemory(void)
{
	setError(NULL);
	
	// If we have not accessed the memory, get it
	//   only if it has already been created.
	if ((sharedMemoryId==-1) &&
	    !getMemory(false)) return false;

	if (sharedMemory && !sharedMemory->active)
	{
		// We are attached to an old memory segment.
		// Detach it so we can attach the new one.
		if (!detachMemory()) return false;
	}

	if (sharedMemory==NULL)
	{
		// At this point we have an id to the memory, now we
		//   can attach it.
		void *shm;
		if ((shm = shmat(sharedMemoryId, NULL, 0)) == (void *) -1)
		{
			setPerror("Cannot attach shared memory");
			return false;
		}
		sharedMemory = (struct JdbcUtilMemoryStruct *) shm;
	}
	return true;
}

bool JdbcUtil::detachMemory(void)
{
	setError(NULL);
	
	if (sharedMemory)
	{
		// Detach the memory segment
		if (shmdt(sharedMemory)==-1)
		{
			setPerror("Cannot detach memory");
			return false;
		}
		sharedMemory = NULL;
	}
	return true;
}

bool JdbcUtil::removeMemory(void)
{
	setError(NULL);
	
	if (!getMemory(false))
	{
		// If not found, it does not exist
		if (errorCode == ENOENT) return true;
		return false;
	}
	
	// deactivate the old memory in case some still has it attached
	if (attachMemory())
	{
		sharedMemory->active = false;
		detachMemory();
	}

	// Remove the memory
	if (shmctl(sharedMemoryId,IPC_RMID,NULL)==-1)
	{
		setPerror("Cannot remove shared memory");
		return false;
	}
	sharedMemoryId = -1;
	return true;
}

bool JdbcUtil::memoryInit(void)
{
	setError(NULL);
	if (!removeMemory()) return false;
	if (!removeSemaphores()) return false;
	if (!getMemory(true)) return false;
	if (!attachMemory()) return false;
	memset(sharedMemory,0,SHARED_MEMORY_SIZE);
	sharedMemory->active = true;

	if (!getSemaphores(true)) return false;
	if (semctl(semaphoreId, 0, SETVAL, 1) == -1)
	{
		int err = errno;
		removeMemory();
		removeSemaphores();
		setPerror(err,"Cannot initalize semaphore value");
		return false;
	}
	return true;
}


int JdbcUtil::findProcess(pid_t pid)
{
	int idx=0;
	for (idx=0; idx<MAX_PROCESSES; idx++)
	{
		if (sharedMemory->processTable[idx].processId==pid) return idx;
	}
	return -1;
}

int JdbcUtil::addProcess(pid_t pid)
{
	if (!attachMemory()) return -1;
	if (!lockSemaphore(SEM_MEMORY_ACCESS,true)) return -1;

	int rc = -1;
	thisProcessId = pid;
	int idx = findProcess(thisProcessId);
	if (idx>=0)
	{
		setError("Cannot add existing process to table");
	}
	else
	{
		idx = findProcess(0);
		if (idx<0)
		{
			if (lockSemaphore(SEM_MEMORY_ACCESS, false))
			{
				if (cleanup())
				{
					if (lockSemaphore(SEM_MEMORY_ACCESS, true))
					{
						idx = findProcess(0);
						if (idx<0)
						{
							setError("Too many processes");
						}
					}
				}
			}
		}
		if (idx>=0)
		{
			sharedMemory->processTable[idx].processId = thisProcessId;
			sharedMemory->processTable[idx].cmd = JDBCUTIL_CMD_NOP;
			sharedMemory->processTable[idx].pending = false;
			rc = idx;
			memoryIdx = idx;
		}
	}
	if (!lockSemaphore(SEM_MEMORY_ACCESS, false)) rc = -1;
	return rc;
}

bool JdbcUtil::removeProcess(int idx)
{
	bool rc = true;
	if (!attachMemory()) return false;
	if (!lockSemaphore(SEM_MEMORY_ACCESS, true)) return false;
	sharedMemory->processTable[idx].processId = 0;
	if (!lockSemaphore(SEM_MEMORY_ACCESS, false)) return false;
	return rc;
}


bool JdbcUtil::showStatus(void)
{
	if (!attachMemory()) return false;

	int idx=0;
	int freeCount=0;
	printf("Process Status:\n");
	for (idx=0; idx<MAX_PROCESSES; idx++)
	{
		if (sharedMemory->processTable[idx].processId)
		{
			char *pending;
			if (sharedMemory->processTable[idx].pending) pending = "Yes";
			else pending = "No";
			printf("    Pid: %u\n",
				sharedMemory->processTable[idx].processId);
			printf("      Cmd:       %s\n",getCmdName(sharedMemory->processTable[idx].cmd));
			printf("      Pending:   %s\n",pending);
			printf("      Parameter: %s\n",sharedMemory->processTable[idx].parameter);
		}
		else freeCount++;
	}
	if (freeCount==MAX_PROCESSES) printf("    Process Table Empty.\n");
	return true;
}

bool JdbcUtil::cleanup(void)
{
	if (!attachMemory()) return false;

	bool cleanList[MAX_PROCESSES];
	bool cleanTable = false;
	int idx=0;
	for (idx=0; idx<MAX_PROCESSES; idx++)
	{
		cleanList[idx] = false;
		if (sharedMemory->processTable[idx].processId)
		{
			if (kill(sharedMemory->processTable[idx].processId,0)==-1)
			{
				if (errno==ESRCH)
				{
					cleanList[idx] = true;
					cleanTable = true;
				}
			}
		}
	}

	if (cleanTable)
	{
		printf("Cleaning out the following processes:\n");
		for (idx=0; idx<MAX_PROCESSES; idx++)
		{
			if (cleanList[idx])
			{
				printf("    %u\n",sharedMemory->processTable[idx].processId);
			}
		}
		if (!lockSemaphore(SEM_MEMORY_ACCESS, true)) return false;
		for (idx=0; idx<MAX_PROCESSES; idx++)
		{
			if (cleanList[idx])
			{
				sharedMemory->processTable[idx].processId = 0;
			}
		}
		if (!lockSemaphore(SEM_MEMORY_ACCESS, false)) return false;
	}
	return true;
}

const struct JdbcUtilProcessTableStruct *JdbcUtil::getCmd(void)
{
	if (!attachMemory()) return NULL;

	static struct JdbcUtilProcessTableStruct rc;
	rc.cmd = JDBCUTIL_CMD_NOP;

	if (sharedMemory->processTable[memoryIdx].processId!=thisProcessId)
	{
		setError("Process Id was removed from process table");
		return NULL;
	}

	if (sharedMemory->processTable[memoryIdx].pending)
	{
		memcpy(&rc,sharedMemory->processTable+memoryIdx,sizeof(rc));
		sharedMemory->processTable[memoryIdx].pending = false;
	}
	return &rc;
}

bool JdbcUtil::setCmd(int cmd, const char *param1, const char *param2, bool force)
{
	if (!attachMemory()) return false;

	if (param1 &&
	    (strlen(param1) >= sizeof(sharedMemory->processTable[0].parameter[0])))
	{
		setError("First parameter is too long");
		return false;
	}

	if (param2 &&
	    (strlen(param2) >= sizeof(sharedMemory->processTable[0].parameter[0])))
	{
		setError("Second parameter is too long");
		return false;
	}

	int idx=0;
	for (idx=0; idx<MAX_PROCESSES; idx++)
	{
		if (sharedMemory->processTable[idx].processId)
		{
			if (sharedMemory->processTable[idx].pending && !force)
			{
				printf("    Pid: %u  - Cannot send command.  A command is already pending.\n",
					sharedMemory->processTable[idx].processId);
			}
			else
			{
				sharedMemory->processTable[idx].cmd = cmd;
				if (param1) strcpy(sharedMemory->processTable[idx].parameter[0],param1);
				else sharedMemory->processTable[idx].parameter[0][0] = 0;
				if (param2) strcpy(sharedMemory->processTable[idx].parameter[1],param2);
				else sharedMemory->processTable[idx].parameter[1][0] = 0;
				sharedMemory->processTable[idx].pending = true;
				printf("    Pid: %u - Sent Command\n",
					sharedMemory->processTable[idx].processId);
			}
		}
	}
	return true;
}

const char *JdbcUtil::getCmdName(int cmd)
{
	static char rc[40];
	switch (cmd)
	{
	case JDBCUTIL_CMD_NOP:
		return("NOP");
	case JDBCUTIL_CMD_BENCHMARK_REPORT:
		return("Benchmark Report");
	}
	sprintf(rc,"Unknown Command (%d)",cmd);
	return(rc);
}
