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
//
**********************************************************************/
/* -*-C++-*-
******************************************************************************
*
* File:         RuMessages.cpp
* Description:  Standard messages for printing to the output file
*
*
* Created:      07/01/2001
* Language:     C++
* 
*
* 
******************************************************************************
*/

// Statements for standard printing into the output file

const char * RefreshDiags [] = 
{	
	// General
	"Started the REFRESH operation.",	// 0
	"Finished the REFRESH operation.",	// 1
	
	// Refresh task executor
	"The materialized views ",	// 2
	"The materialized view ",	// 3

	" are being refreshed",	// 4			
	" is being refreshed",		// 5

	" were found up to date",	// 6
	" have been refreshed",		// 7

	" was found up to date",	// 8
	" has been refreshed",		// 9

	// Simple Refresh task executor
	" (by recompute)",	// 10
	" in a single transaction",	// 11
	" (in ",	// 12
	" phases)",	// 13
	
	// Multi-Txn task executor
	" in multiple transactions...", // 14
	" in ",	// 15
	" transaction(s).",	// 16

	// Log Cleanup task executor
	"Starting the log cleanup of table ",	// 17
	"Finished the log cleanup of table ",	// 18

	// Specify the number of deltas
	" from a single delta", // 19
	" from ",               // 20
	" deltas"               // 21
};
