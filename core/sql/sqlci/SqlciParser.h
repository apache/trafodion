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
#ifndef SQLCIPARSER_H
#define SQLCIPARSER_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciParser.h
 * RCS:          $Id: SqlciParser.h,v 1.5 1998/06/29 06:17:22  Exp $
 *               
 * Modified:     $ $Date: 1998/06/29 06:17:22 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
// Change history:
// 
// $Log: SqlciParser.h,v $
// Revision 1.5  1998/06/29 06:17:22
// *** empty log message ***
//
// Revision 1.4  1997/12/02 16:51:06
// Made changes required to compile sqlci dir on NSK.
//
// Revision 1.3  1997/06/17 22:28:30
//
// Modifications to export functions required from tdm_sqlcli.dll.
//
// Revision 1.2  1997/04/23 00:31:05
// Merge of MDAM/Costing changes into SDK thread
//
// Revision 1.1.1.1.2.1  1997/04/11 23:24:55
// Checking in partially resolved conflicts from merge with MDAM/Costing
// thread. Final fixes, if needed, will follow later.
//
// Revision 1.1.4.1  1997/04/10 18:33:17
// *** empty log message ***
//
// Revision 1.1.1.1  1997/03/28 01:39:44
// These are the source files from SourceSafe.
//
// 
// 5     1/22/97 11:04p 
// Merged UNIX and NT versions.
// 
// 3     1/14/97 4:55a 
// Merged UNIX and  NT versions.
// 
// 1     12/04/96 11:27a 
// Revision 1.1  1996/11/21 03:18:13
// Initial revision
//
// 
// -----------------------------------------------------------------------

#include "SqlCliDllDefines.h"

class SqlciNode;
class SqlciEnv;

Int32 sqlci_parser(char *instr, char *origstr, SqlciNode ** node, SqlciEnv *sqlci_env);
Int32 sqlci_parser_syntax_error_cleanup(char *instr, SqlciEnv *sqlci_env);
Int32 sqlci_parser_handle_report_writer(SqlciEnv *sqlci_env, Lng32 retval);
Int32 sqlci_parser_handle_error(SqlciNode **node, Lng32 retval);

#endif /* SQLCIPARSER_H */
