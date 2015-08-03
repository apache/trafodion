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
                    , Int64 & histModTime
                    , Int64 & statsTime
                    , NABoolean & allFakeStats
                    , const NABoolean preFetch=FALSE
                    , Int64 createStatsSize=0
                    );

/******************************************************************************/
/*                             GetHSModifyTime()                              */
/* FUNCTION   Provides the last modification time of the HISTOGRAM table.     */
/*            For example, CAT.SCH.HISTOGRM                                   */
/*                                                                            */
/* RETCODE    Success: (0)                                                    */
/*                                                                            */
/* NOTES      If unable to determine modified time, the result is zero(0)     */
/******************************************************************************/
Lng32 GetHSModifyTime( const QualifiedName & qualifiedName,
                      const ExtendedQualName::SpecialTableType type,
                      Int64 &histModTime,
                      NABoolean isSQLMPTable);

/******************************************************************************/
/*                           FetchStatsTime()                                 */
/* FUNCTION   Provide the most recent STATS_TIME value for the table in       */
/*            question.  This function will also update the READ_TIME value   */
/*            of the histogram if update stats automation is on.  This is     */
/*            necessary so that update stats can keep track of the optimizer's*/
/*            use of histograms (even if they're from the cache).             */
/*                                                                            */
/* RETCODE    Success: (0)                                                    */
/*                                                                            */
/* NOTES      If unable to determine modified time, the result is zero(0)     */
/******************************************************************************/
Lng32 FetchStatsTime ( const QualifiedName & qualifiedName,
                      const ExtendedQualName::SpecialTableType type,
                      const NAColumnArray & colArray,
                      Int64 &histModTime,
                      NABoolean isSQLMPTable);

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






