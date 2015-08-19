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
#ifndef MXVERSION_H
#define MXVERSION_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         MXVersion.h
 * Description:  SQL/MX version number.
 *
 * Created:      5/21/2001
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

//------------------------------------------------------------------------
// This version reflects the system SQL/MX version - it should be updated
// for every release.
//------------------------------------------------------------------------
#define SQLMX_VERSION 15

//------------------------------------------------------------------------
// This is the system version to indicate systems that do not support
// SQL/MX versioning.
//------------------------------------------------------------------------
#define PRE_SQLMX_VERSION 10

//------------------------------------------------------------------------
// Various versions of SQL/MX. A define should be added each time a new
// version is created.
//------------------------------------------------------------------------
#define R10           10
#define R15           15

#define PRE_SQLMX_VERSION 10
#endif
