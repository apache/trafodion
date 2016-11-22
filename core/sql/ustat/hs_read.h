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
#ifndef HSREAD_H
#define HSREAD_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         hs_read.h
 * Description:  Function to read histograms tables.
 * Created:      04/27/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "Stats.h"

/******************************************************************************/
/*                             FetchHistograms()                              */
/* FUNCTION   Provides the histograms of the specified table and columns,     */
/*            if they exist.  If histograms do not exist, the function will   */
/*            estimate the rowcount and UEC and return these (that is, there  */
/*            will be only 1 interval in this case.                           */
/*                                                                            */
/* RETCODE    Success: (0)                                                    */
/*                                                                            */
/******************************************************************************/
Lng32 FetchHistograms( const QualifiedName & qualifiedName
                    , const ExtendedQualName::SpecialTableType type
                    , const NAColumnArray & colArray
                    , StatsList & colStatsList
                    , NABoolean isSQLMPTable
                    , NAMemory * heap
                    , Int64 & statsTime
                    , NABoolean & allFakeStats
                    , const NABoolean preFetch=FALSE
                    , Int64 createStatsSize=0
                    );


/******************************************************************************/
/*                           FetchCount()                                     */
/* FUNCTION   Provides the rowcount of a table after a predicate has been     */
/*            applied.                                                        */
/*                                                                            */
/* RETCODE    Success: (0)                                                    */
/*                                                                            */
/******************************************************************************/
Lng32 FetchCount( const QualifiedName &qualifiedName,
                 const ExtendedQualName::SpecialTableType type,
                 NAString &predicate,
                 Int64 &count,
                 float &sampleFraction,
                 Int64 &tableSize);

#endif  /* HSREAD_H */






