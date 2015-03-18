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
// (C) Copyright 2010-2015 Hewlett-Packard Development Company, L.P.
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

#endif // COMMISC_H
