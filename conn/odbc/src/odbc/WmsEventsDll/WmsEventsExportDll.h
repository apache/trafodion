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

#ifndef WMS_EVENTS_EXPORT_DLL_H
#define WMS_EVENTS_EXPORT_DLL_H

#include "QpidQueryStats.h"

extern "C" void
sendTokenizedEvent(qpid_struct_type qst, void* ss);

extern "C"void
sendToEventLog (short evt_num, short EventLogType, char *ComponentName, char *ObjectRef, short nToken, va_list marker);

extern "C"void
setCriticalDialoutDll();

extern "C"void
setTraceVariables(bool, bool);

#endif
