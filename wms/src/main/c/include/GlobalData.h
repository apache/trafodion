/**
 *(C) Copyright 2013 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef LIBWMS_WMSGLOBALDATA_H
#define LIBWMS_WMSGLOBALDATA_H

#define WAIT_TIME_SECONDS       15
//
// zookeeper
//
extern zhandle_t *zh;
extern clientid_t myid;
extern stringstream zk_ip_port;
extern pthread_cond_t cond;
extern pthread_mutex_t lock;
//extern void *watcherCtx;
//
extern char *wmshost;
extern int wmsport;
extern deque<string> wmsServers;
extern bool isOpen;

extern pid_t pid;//Linux process id
extern char programName[128];//Linux program name from /proc
extern char userName[256];
extern char hostName[256];
extern char myHostName[HOST_NAME_MAX];
extern char myIpAddr[256];

extern long connectionInfo;

extern short lastErrorType;
extern long  lastErrorNum;
extern stringstream lastErrorText;

extern bool myZkHandle;

#endif /* LIBWMS_WMSGLOBALDATA_H */
