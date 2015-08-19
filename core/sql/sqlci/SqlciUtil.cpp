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
 *****************************************************************************
 *
 * File:         SqlciUtil.cpp
 * Description:  
 *               
 *               
 * Created:      11/12/2005
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */


#include "SqlciUtil.h"
#include "SqlciEnv.h"
#include "SqlciError.h"
extern ComDiagsArea sqlci_DA;

UtilCmd::UtilCmd(const UtilCmdType commandType, char * argument)
                  : SqlciNode(SqlciNode::UTIL_CMD_TYPE),
		    commandType_(commandType)
{
  argument_ = new char[strlen(argument)+1];
  strcpy(argument_, argument);
};

short UtilCmd::process(SqlciEnv * sqlciEnv)
{
  return 0;
}

