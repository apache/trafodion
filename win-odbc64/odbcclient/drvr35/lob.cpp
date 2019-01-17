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
********************************************************************/
/**************************************************************************
**************************************************************************/
//
//

#include "lob.h"
#include "cconnect.h"
#include "cstmt.h"

namespace ODBC {
    IDL_long getLobLength(CConnect *m_ConnectHandle, SQLHANDLE InputHandle, IDL_string lobHandle, IDL_long lobHandleLen)
    {
        SQLHSTMT hstmt;
        m_ConnectHandle->AllocHandle(SQL_HANDLE_STMT, InputHandle, &hstmt);

        IDL_long totalLength;
        BYTE * data;
        CStmt *pStatement = (CStmt *)hstmt;
        pStatement->ExtractLob(0,
            lobHandle, lobHandleLen, totalLength, data);
        pStatement->Close(SQL_CLOSE);
        return totalLength;
    }

    bool getLobData(CConnect *m_ConnectHandle, SQLHANDLE InputHandle, IDL_string lobHandle, IDL_long lobHandleLen, SQLPOINTER target, IDL_long &length)
    {
        SQLHSTMT hstmt;
        m_ConnectHandle->AllocHandle(SQL_HANDLE_STMT, InputHandle, &hstmt);

        IDL_long totalLength = 0;
        BYTE * data;
        CStmt *pStatement = (CStmt *)hstmt;
        IDL_long extractLen = length;

        IDL_string lobHandleTemp = (IDL_string)malloc(lobHandleLen + 1);
        memset(lobHandleTemp, 0, lobHandleLen + 1);
        memcpy(lobHandleTemp, lobHandle, lobHandleLen);

        if (pStatement->ExtractLob(1,
            lobHandleTemp, lobHandleLen + 1, extractLen, data) != SQL_SUCCESS)
        {
            pStatement->Close(SQL_CLOSE);
            free(lobHandleTemp);
            lobHandleTemp = NULL;

            return false;
        }

        if (length > extractLen)
        {
            memcpy(target, data, extractLen);
            length = extractLen;
        }
        else
        {
            length = 0;
        }

        pStatement->Close(SQL_CLOSE);
        free(lobHandleTemp);
        lobHandleTemp = NULL;

        return true;
    }

}