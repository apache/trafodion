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
#include <sys/time.h>
#include <sys/timeb.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <stack>

using namespace std;

class TimeLogger
{
	private:
		int              nid;
		int              pid;
		char             fileName[1024]; 
		char             funcName[128];
		char             miscArg1[128];
		char             miscArg2[128];
		long double startTime, finishTime, startToFinish;
		char startTimeLineText[50];
		char endTimeLineText[50];
		FILE* pfile;
		long double getTimeMilli();
		bool hasEnd;
		//Daniel - this stack is for recursive message.
		stack<string> stkMessageStack;
	public:
		TimeLogger();
		void createLogFile(int nid, int pid, char* procName);
		void setObjRef(char* strObjRef);
		void start(char* fName, char* arg1=NULL, char* arg2=NULL);
		void end();
};
