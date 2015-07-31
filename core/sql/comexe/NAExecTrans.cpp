/* -*-C++-*-
****************************************************************************
*
* File:         NAExecTrans.cpp
* Description:  
*
* Created:      5/6/98
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
****************************************************************************
*/

#include "NAExecTrans.h"
#include "sqlcli.h"
#include "sql_id.h"

NABoolean NAExecTrans(Lng32 command,
                      Int64& transId)
{
  SQLDESC_ID transid_desc;
  init_SQLDESC_ID(&transid_desc);

  SQLMODULE_ID module;
  init_SQLMODULE_ID(&module);

  module.module_name = 0;

  transid_desc.module = &module;
  transid_desc.name_mode = desc_handle;
  transid_desc.handle = 0L;
  Int32 rc;

  if ( rc=SQL_EXEC_AllocDesc(&transid_desc, 1) )
    return FALSE;

  Int64 transid = transId;
  if ( rc = SQL_EXEC_SetDescItem(&transid_desc, 1, SQLDESC_VAR_PTR,
    (Long)&transid, 0) )
    {
      SQL_EXEC_DeallocDesc(&transid_desc);
      return FALSE;
    }

  // command set to TRUE means SQLTRANS_SET, but this is not
  // currently supported
  if (command)
    return FALSE;
  rc = SQL_EXEC_Xact(SQLTRANS_STATUS, &transid_desc);
  if (rc == 0)
  {
    transId = transid;
  }
  SQL_EXEC_DeallocDesc(&transid_desc);
  return rc == 0;
}


