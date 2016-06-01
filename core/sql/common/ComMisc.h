/* -*-C++-*- */
#ifndef COMMISC_H
#define COMMISC_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComMisc.h
 * Description:  Miscellaneous global functions
 *
 * Created:      1/20/2010
 * Language:     C++
 *
 *
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
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "ComSmallDefs.h"
#include "NAString.h"

// schema names of pattern  "_%_" are reserved for internal system schemas.
NABoolean ComIsTrafodionReservedSchemaName(
                                           const NAString &schName);

// list of schemas that have been created as reserved schemas.
NABoolean ComIsTrafodionReservedSchema(
                                       const NAString &systemCatalog,
                                       const NAString &catName,
                                       const NAString &schName);

// schema names of pattern "_HV_ ... _" and "_HB_ ... _" are reserved to store
// external hive and hbase tables
NABoolean ComIsTrafodionExternalSchemaName (
                                            const NAString &schName);

NAString ComConvertNativeNameToTrafName ( 
                                         const NAString &catalogName,
                                         const NAString &schemaName,
                                         const NAString &objectName);

NAString ComConvertTrafNameToNativeName(
                                         const NAString &catalogName,
                                         const NAString &schemaName,
                                         const NAString &objectName);

// returns TRUE if specified name is a reserved name.
// Currently, reserved names for traf internal usage are:
//   SYSKEY
//   _SALT_
//   _DIVISION_*_   :_DIVISION_ prefix followed by division number and ending
//                   with underscore(_)
NABoolean ComTrafReservedColName(const NAString &colName);

#endif // COMMISC_H
