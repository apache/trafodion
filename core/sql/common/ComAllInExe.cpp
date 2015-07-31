/**********************************************************************
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
**********************************************************************/
// -----------------------------------------------------------------------
// Files in the common directory that are needed by the CLI, by ARKESP,
// and by the components below. Don't add any files that have dependencies
// to outside components to this list.
// -----------------------------------------------------------------------
#include "BaseTypes.cpp"
#include "ComDistribution.cpp"
#include "ComGuardianFileNameParts.cpp"
#include "ComVersionNodeInfo.cpp"
#include "ComMvAttributeBitmap.cpp"
#include "ComRtUtils.cpp"
#include "ComSpace.cpp"
#include "ComSqlId.cpp"
#include "ComSysUtils.cpp"
#include "ComTransInfo.cpp"
#include "ComVersionPrivate.cpp"
#include "ComVersionPublic.cpp"
#include "conversionISO88591.cpp"
#include "conversionLocale.cpp"
#include "conversionSJIS.cpp"
#include "conversionKSC5601.cpp"
#include "DgBaseType.cpp"
#include "ErrorCondition.cpp"
#include "Int64.cpp"
#include "Ipc.cpp"
#include "IpcGuardian.cpp"
#include "IpcSockets.cpp"
#include "NAAssert.cpp"
#include "NAMemory.cpp"
#include "str.cpp"	
#include "unicode_char_set.cpp"
#include "charinfo.cpp"
#include "wstr.cpp"
//#include "NLSConversion.cpp"
#include "stringBuf.cpp"

#include "ComResWords.cpp"
#include "NAString2.cpp"
#include "ComplexObject.cpp"
