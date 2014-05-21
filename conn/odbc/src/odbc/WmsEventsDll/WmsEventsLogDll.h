/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
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
********************************************************************/

#ifndef WMS_EVENTS_LOG_DLL_H
#define WMS_EVENTS_LOG_DLL_H

#define MAX_EVT_BUF_SIZE 3900

enum experienceTypes {ADVANCED, BEGINNER};
enum severityTypes {CRTCL, MAJOR, MINOR, INFRM};
enum targetTypes {DIALOUT, DBADMIN, LOGONLY };

void send_to_eventlog (short evt_num, short EventLogType, char *ComponentName, char *ObjectRef, short nToken, va_list marker);
void set_critical_dialout();
void set_is_wms();
void set_trace_variables(bool trace_ems_dll, bool trace_legacy_dll );

#endif
