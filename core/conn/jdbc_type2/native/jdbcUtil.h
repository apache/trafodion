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
#ifndef JDBCUTIL_H
#define JDBCUTIL_H

#include <sys/types.h>

#define JDBCUTIL_CMD_NOP				0
#define JDBCUTIL_CMD_BENCHMARK_REPORT	1

struct JdbcUtilProcessTableStruct
{
	pid_t processId;
	int cmd;
	char parameter[2][128];
	bool pending;
};

struct JdbcUtilMemoryStruct
{
	bool active;
	struct JdbcUtilProcessTableStruct processTable[1];
};

class JdbcUtil
{
private:
	char errorMsg[256];
	int errorCode;
	struct JdbcUtilMemoryStruct *sharedMemory;
	int sharedMemoryId;
	int semaphoreId;
	int memoryKey;
	int memoryIdx;
	pid_t thisProcessId;
	void setPerror(int err, const char *action);
	void setPerror(const char *action);
	void setError(const char *action);
	bool getSemaphores(bool create);
	bool removeSemaphores(void);
	bool getMemory(bool create);
	bool attachMemory(void);
	bool detachMemory(void);
	bool removeMemory(void);
	bool lockSemaphore(unsigned short sem_idx, bool lock);
	int findProcess(pid_t pid);

public:
	JdbcUtil();
	~JdbcUtil();
	const char *getError(void);
	int getErrorCode(void);
	bool memoryInit(void);
	int addProcess(pid_t pid);
	bool removeProcess(int idx);
	bool showStatus(void);
	bool cleanup(void);
	const struct JdbcUtilProcessTableStruct *getCmd(void);
	bool setCmd(int cmd, const char *param1, const char *param2, bool force);
	static const char *getCmdName(int cmd);
};

#endif /* JDBCUTIL_H */
