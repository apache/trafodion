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
#include "TimeLogger.h"

long double TimeLogger::getTimeMilli()
{
	long double CurrentTime;
    	struct timeval tv;
	struct timezone tz;
	gettimeofday( &tv, &tz );
	CurrentTime =(long double)tv.tv_sec*1000 + ((long double)tv.tv_usec / (long double)1000);
    	return CurrentTime;
};

TimeLogger::TimeLogger()
{
	startTime=0; finishTime=0; startToFinish=0;
	pfile = NULL;
	fileName[0]='\0';
};

void TimeLogger::createLogFile(int nid, int pid, char* procName)
{
	char pidStr[10];
	char nidStr[10];
	char    tmpTimeStamp[256];
	startTime=0; finishTime=0; startToFinish=0;
	pfile = NULL;
	fileName[0]='\0';
	tmpTimeStamp[0]='\0';
	sprintf(tmpTimeStamp,"_%ld", time(NULL));
	sprintf(nidStr, ".%d.", nid);
	sprintf(pidStr, "%d", pid);
	strcpy(fileName, procName);
	strcat(fileName, nidStr);
	strcat(fileName, pidStr);
	strcat(fileName, tmpTimeStamp);
};

void TimeLogger::start(char* fName, char* arg1, char* arg2)
{
	miscArg1[0]='\0';
	miscArg2[0]='\0';
	startTime=0; finishTime=0;
	memset(startTimeLineText,'\0', sizeof(startTimeLineText));
	memset(endTimeLineText,'\0', sizeof(endTimeLineText));
	hasEnd=false;
	stkMessageStack.push(fName);
	struct timeb currentTime;
	ftime(&currentTime);
        char* starttimeline = ctime( &currentTime.time  );
        *(starttimeline + 20 + 4 ) = 0;
        *(starttimeline + 19 ) = 0;
        sprintf(startTimeLineText," %.19s.%.3hu ",&starttimeline[11], currentTime.millitm );

	startTime = getTimeMilli();
	//strcpy(funcName, fName);
	if(arg1 != NULL)
	strcpy(miscArg1, arg1);
	if(arg2 != NULL)
	strcpy(miscArg2, arg2);

};
void TimeLogger::setObjRef(char* strObjRef)
{
	strcpy(funcName,"(");
	strcat(funcName,strObjRef);
	strcat(funcName,")");
}
void TimeLogger::end()
{
	struct timeb time;
	string strMessage;
	ftime(&time);
        char* endtimeline = ctime( &time.time  );
        *(endtimeline + 20 + 4 ) = 0;
        *(endtimeline + 19 ) = 0;
        sprintf(endTimeLineText," %.19s.%.3hu ",&endtimeline[11], time.millitm );

	finishTime =  getTimeMilli();
	startToFinish = finishTime - startTime;
    	pfile = fopen(fileName,"a+");
	fprintf(pfile,"-----------------------------------------------------------------------------------------------------------------\n");
		if(!stkMessageStack.empty())
		{
			strMessage=stkMessageStack.top();
			stkMessageStack.pop();
			strMessage+=funcName;
		}
    	fprintf(pfile,"%-120.120s", strMessage.c_str());
		funcName[0]='\0';
		hasEnd=true;
	if(miscArg1[0] != '\0')
    		fprintf(pfile,"%-8.8s", miscArg1);
	else
    		fprintf(pfile,"%-8.8s", miscArg1);
	if(miscArg2[0] != '\0')
    		fprintf(pfile,"%-8.8s", miscArg2);
	else
    		fprintf(pfile,"%-8.8s", miscArg2);
	fprintf(pfile,"%s %s %8.3Lf\n", startTimeLineText, endTimeLineText, startToFinish);
	fclose(pfile);
};

/*
int main()
{

	TimeLogger aTimeLogger(1,2, "$abc");
	aTimeLogger.start("Hello", "test", "test1");
	sleep(2);
	aTimeLogger.end();
	aTimeLogger.start("Hello2");
	sleep(5);
	aTimeLogger.end();
	aTimeLogger.start("Hello3", "hello");
	sleep(2);
	aTimeLogger.end();
	aTimeLogger.start("Hello4");
	sleep(1);
	aTimeLogger.end();

}
*/
