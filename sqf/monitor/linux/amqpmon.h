///////////////////////////////////////////////////////////////////////////////
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2014 Hewlett-Packard Development Company, L.P.
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
//
///////////////////////////////////////////////////////////////////////////////

#ifdef USE_SEAPILOT
#include <qpid/client/Connection.h>
#include <qpid/client/Session.h>
#include <qpid/client/AsyncSession.h>
#include <qpid/client/Message.h>
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

extern qpid::client::Connection connection;
extern qpid::client::Session session;
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


