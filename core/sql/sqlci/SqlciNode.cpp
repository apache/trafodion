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
 * File:         SqlciNode.C
 * Description:  
 *
 * Created:      4/15/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"


#include "SqlciNode.h"
#include "SqlciEnv.h"
#include "str.h"


SqlciNode::SqlciNode(const sqlci_node_type node_type_)
                 : node_type(node_type_),
		   next(NULL),
		   errcode(0)
{
  str_cpy_all(eye_catcher, "CI  ", 4);
};

SqlciNode::~SqlciNode()
{
};


short SqlciNode::process(SqlciEnv * sqlci_env)
{
  cerr << "Error: virtual function process must be redefined. \n";
  return -1;
};
