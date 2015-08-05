///////////////////////////////////////////////////////////////////////////////
//
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
//
///////////////////////////////////////////////////////////////////////////////

#ifdef USE_SEAPILOT
#include <qpid/messaging/Connection.h>
#include <qpid/messaging/Session.h>
#include <qpid/messaging/Message.h>
#include <qpid/messaging/Address.h>
#include <qpid/messaging/Sender.h>
#include <qpid/messaging/Receiver.h>
#include <qpid/types/Variant.h>

#include  "common/monitor.instance_down.pb.h"
#include  "common/monitor.instance_up.pb.h"
#include  "common/monitor.node_down.pb.h"
#include  "common/monitor.node_up.pb.h"
#include  "common/monitor.process_close.pb.h"
#include  "common/monitor.process_exit.pb.h"
#include  "common/monitor.process_kill.pb.h"
#include  "common/monitor.process_open.pb.h"
#include  "common/monitor.process_start.pb.h"


#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>

using namespace qpid;

using std::stringstream;
using std::string;

extern bool amqp_initialized;
#endif

void AMQP_INITIALIZE();
void NODEUP (void *msg);
void NODEDOWN (void *msg);
void PROCESSCLOSE (void *msg);
void PROCESSEXIT (void *msg);
void PROCESSKILL (void *msg);
void PROCESSOPEN (void *msg);
void PROCESSSTART (void *msg);
void STARTUP ();
void SHUTDOWN (void *msg);


