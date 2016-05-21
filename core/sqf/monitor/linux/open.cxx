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

#include <iostream>

using namespace std;

#include <string.h>

#include "monlogging.h"
#include "monsonar.h"
#include "montrace.h"
#include "monitor.h"
#include "clusterconf.h"
#include "lock.h"
#include "lnode.h"
#include "pnode.h"
#include "open.h"

extern int trace_level;
extern int MyPNID;
extern char Node_name[MPI_MAX_PROCESSOR_NAME];
extern CMonitor *Monitor;
extern CNodeContainer *Nodes;
extern CMonStats *MonStats;

extern char *ErrorMsg (int error_code);

COpen::COpen( CProcess *process )
      :Nid(process->GetNid())
      ,Pid(process->GetPid())
      ,Verifier(process->GetVerifier())
      ,Next(NULL)
      ,Prev(NULL)
{
    const char method_name[] = "COpen::COpen";
    TRACE_ENTRY;

    // Add eyecatcher sequence as a debugging aid
    memcpy(&eyecatcher_, "OPEN", 4);

    strcpy( Name, process->GetName() );
    Monitor->IncOpenCount();

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->NumOpensIncr();

    TRACE_EXIT;
}

COpen::~COpen (void)
{
    const char method_name[] = "COpen::~COpen";
    TRACE_ENTRY;
    Monitor->DecrOpenCount();

    // Record statistics (sonar counters)
    if (sonar_verify_state(SONAR_ENABLED | SONAR_MONITOR_ENABLED))
       MonStats->NumOpensDecr();

    // Alter eyecatcher sequence as a debugging aid to identify deleted object
    memcpy(&eyecatcher_, "open", 4);

    TRACE_EXIT;
}

void COpen::DeLink (COpen ** head, COpen ** tail)
{
    const char method_name[] = "COpen::DeLink";
    TRACE_ENTRY;
    if (*head == this)
        *head = Next;
    if (*tail == this)
        *tail = Prev;
    if (Prev)
        Prev->Next = Next;
    if (Next)
        Next->Prev = Prev;
    TRACE_EXIT;
}

COpen *COpen::GetNext (void)
{
    const char method_name[] = "COpen::GetNext";
    TRACE_ENTRY;
    TRACE_EXIT;
    return Next;
}

COpen *COpen::Link (COpen * entry)
{
    const char method_name[] = "COpen::Link";
    TRACE_ENTRY;
    Next = entry;
    entry->Prev = this;

    TRACE_EXIT;
    return entry;
}
