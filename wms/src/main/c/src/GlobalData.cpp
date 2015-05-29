/**********************************************************************
// @@@ START COPYRIGHT @@@
//
//        HP CONFIDENTIAL: NEED TO KNOW ONLY
//
//        Copyright 2013
//        Hewlett-Packard Development Company, L.P.
//        Protected as an unpublished work.
//
//  The computer program listings, specifications and documentation
//  herein are the property of Hewlett-Packard Development Company,
//  L.P., or a third party supplier and shall not be reproduced,
//  copied, disclosed, or used in whole or in part for any reason
//  without the prior express written permission of Hewlett-Packard
//  Development Company, L.P.
//
// @@@ END COPYRIGHT @@@
********************************************************************/
#include "GlobalHeader.h"
//
// zookeeper
//
zhandle_t *zh;
clientid_t myid;
stringstream zk_ip_port;
void *watcherCtx;

pthread_cond_t cond=PTHREAD_COND_INITIALIZER;
pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;

char *wmshost;
int wmsport;
deque<string> wmsServers;
bool isOpen = false;

pid_t pid;//Linux process id
char programName[128];//Linux program name from /proc
char userName[256];
char hostName[256];
char myHostName[HOST_NAME_MAX];
char myIpAddr[256];

long connectionInfo;

short lastErrorType;
long  lastErrorNum;
stringstream lastErrorText;

bool myZkHandle;

void setProcessInfo() {
	FILE *fp = NULL;
	int num;
	char buf[256] = {0};
	char procId[12] = {0};
	char progName[128] = {0};
	char tmp[256] = {0};
	pid = getpid();

	sprintf(tmp,"/proc/%d/stat",pid);

	if (!fp){
		fp = (fopen(tmp, "r"));
		if (fp){
			if (fgets(buf, sizeof(buf), fp)){
				num = sscanf(buf, "%s%s", procId, progName);
				strncpy(programName,&progName[1],strlen(progName)-2);//don't copy parenthesis
			}
		}
	}

	if(fp) {
		fclose(fp);
	}

	cuserid(userName);
	gethostname(hostName, 256);

}
