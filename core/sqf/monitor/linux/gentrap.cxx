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

#include <stdio.h>
#include <stdlib.h>
#include "common/evl_sqlog_eventnum.h"
#include "montrace.h"
#include "monlogging.h"

extern int genSnmpTrapEnabled;

void genSnmpTrap ( const char *event )
{
    const char method_name[] = "genSnmpTrap";
    TRACE_ENTRY;

    if (trace_settings & TRACE_EVLOG_MSG)
       trace_printf( "%s@%d - snmptrap(%d): %s\n"
                   , method_name, __LINE__, genSnmpTrapEnabled, event);

    mon_log_write( MON_GEN_SNMP_TRAP, SQ_LOG_CRIT, (char *)event );

    if ( genSnmpTrapEnabled )
    {
        char snmpCmd[300];
        snprintf( snmpCmd, sizeof(snmpCmd), 
                  "snmptrap -v 1 -c public 172.31.2.240 NET-SNMP-EXAMPLES-MIB::netSnmpExampleHeartbeatNotification \"\" 5  99 \"\"  text string \"%s\"", event);
        system( snmpCmd );

        snprintf( snmpCmd, sizeof(snmpCmd),
                  "snmptrap -v 1 -c public 172.31.2.241 NET-SNMP-EXAMPLES-MIB::netSnmpExampleHeartbeatNotification \"\" 5  99 \"\"  text string \"%s\"", event);
        system( snmpCmd );
    }

    TRACE_EXIT;
}
