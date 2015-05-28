/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1995-2014 Hewlett-Packard Development Company, L.P.
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


// The purpose of these functions is that every class which is
// derived from SqlciNode has to implement these functions.  So
// certain commands will be allowed  in SIP and certain commands
// wonte be allowed in RW mode. For those classes these fucntions
// will return TRUE and FALSE respectively. 

NABoolean SqlciNode::isAllowedInSIP()
{
	cerr << "Error: virtual function isAllowedInSIP must be redefined. \n";
	return FALSE;
};

// All commanda will be allowed in RWmode execpte those which are
// exclusive to SIP  like CANCEL, LIST ALL, LIST FIRST, LIST NEXT.
// So the default behaviour is TRUE and for those commands alone it will be 
// FALSE.

NABoolean SqlciNode::isAllowedInRWMode()
{
	cerr << "Error: virtual function isAllowedInRWMode must be redefined. \n";
	return TRUE;
};

NABoolean SqlciNode::isAllowedInCSMode()
{
  	cerr << "Error: virtual function isAllowedInCSMode must be redefined. \n";
	return TRUE;
}
