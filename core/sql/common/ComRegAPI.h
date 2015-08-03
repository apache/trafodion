/* -*-C++-*-
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
 *****************************************************************************
 *
 * File:         ComRegAPI.h
 * Description:  Common routines used to access system APIs.
 *
 * Created:      10/08/97
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#ifndef COMREGAPI_H_
#define COMREGAPI_H_

#include "Platform.h"

#include "ComSmallDefs.h"

// log the error message in the event log.
void logErrorMessageInEventLog(Int32 msgId, const char * msg);


// retrieve the Tandem volume.
ComString getTandemSysVol();

// retrieve the Tandem metadata versions.
ComString getTandemMetaDataVersions();

#endif // COMREGAPI_H_
