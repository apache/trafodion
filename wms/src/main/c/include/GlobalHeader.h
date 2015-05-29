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
#ifndef GLOBAL_HEADER_H
#define GLOBAL_HEADER_H

#include <string>
#include <sstream>
#include <cctype>
#include <iostream>
#include <list>
#include <deque>
#include <iomanip>
#include <algorithm>
#include <vector>
#include <dirent.h>
#include <map>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <ctype.h>
#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>       // for file i/o constants
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <poll.h>
#include <sys/utsname.h>
#include <sys/shm.h>
#include <limits.h>
#include <sched.h>
#include <semaphore.h>      /* for p-thread semaphores        */
#include <pthread.h>
#include <dlfcn.h>
#include <inttypes.h>
#include <regex.h>

#include <zookeeper/zookeeper.h>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include "GlobalData.h"
#endif
