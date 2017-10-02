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
/* -*-C++-*- ***************************************************************
 *
 * File:         ComSqlId.cpp
 * Description:  
 *
 * Created:      10/31/2006
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#include "ComSqlId.h"
#include "ExpError.h"
#include "str.h"


ComSqlId::ComSqlId(CollHeap * heap)
     : heap_(heap)
{
}

Lng32 ComSqlId::createSqlSessionId
(char * sessionId,            // INOUT
 Lng32 maxSessionIdLen,       // IN
 Lng32 &actualSessionIdLen,   // OUT
 Lng32 nodeNumber,            // IN
 Lng32 cpu,                   // IN
 Lng32 pin,                   // IN
 Int64 processStartTS,        // IN
 Int64 sessionUniqueNum,      // IN
 Lng32 userNameLen,           // IN
 const char *userName,        // IN
 Lng32 userSessionNameLen,    // IN
 const char *userSessionName  // IN
 )
{
  // if input buffer space is less then the needed space,
  // return error.
  if (maxSessionIdLen < MAX_SESSION_ID_LEN)
    return -CLI_INVALID_ATTR_VALUE;

  // The format of current session id is:
  //  MXID<version><segment><cpu><pin><processStartTS><sessNum><unLen><userName><snLen><sessionName>
  str_sprintf
    (sessionId, "%s%02d%03d%06d%018ld%010d%02d%s%02d%s",
     COM_SESSION_ID_PREFIX,
     SQ_SQL_ID_VERSION,
     nodeNumber,                      // 3 digits
     pin,                             // 6 digits
     processStartTS,                  // 18 digits
     (Lng32)sessionUniqueNum,         // 10 digits
     userNameLen,                     // 2 digits
     userName,                        // 
     userSessionNameLen,              // 2 digits
     userSessionName
     );

  actualSessionIdLen = (Lng32)strlen(sessionId);

  return 0;
}

Lng32 ComSqlId::createSqlQueryId
(char * queryId,           // INOUT
 Lng32 maxQueryIdLen,       // IN
 Lng32 &actualQueryIdLen,   // OUT
 Lng32 sessionIdLen,        // IN
 char * sessionId,         // IN
 Int64 queryUniqueNum,     // IN
 Lng32 queryNameLen,        // IN
 char * queryName          // IN
 )
{
  // if input buffer space is less then the needed space,
  // return error.
  if (maxQueryIdLen < MAX_QUERY_ID_LEN)
    return -CLI_INVALID_ATTR_VALUE;

  // The format of current query id is:
  //  <session_id>_<unique_number>_<identifier_or_handle>
  // <unique_number> is max 18 digits. Stored value is the actual
  // number of digits delimited by underscores. 
  // Ex, queryNum of 12 will be store as _12_
  //
  str_sprintf(queryId, "%s_%ld_",
	      sessionId,
	      queryUniqueNum
	      );

  Int32 queryIdLen = (Int32)strlen(queryId);
  // check queryName length 
  if(queryNameLen > maxQueryIdLen - queryIdLen -1)
     queryNameLen = maxQueryIdLen - queryIdLen -1;

  // copy queryName to queryId
  strncpy(&queryId[queryIdLen],queryName, queryNameLen);

  actualQueryIdLen = queryIdLen + queryNameLen;
  queryId[actualQueryIdLen] =0;

  return 0;
}

Lng32 ComSqlId::getSqlIdAttr
(Lng32 attr,          // which attr (SqlQueryIDAttr)
 const char * queryId,// query ID
 Lng32 queryIdLen,    // query ID len
 Int64 &value,       // If returned attr is numeric, 
                     //    this field contains the returned value.
                     // If attr is of string type, this value is:
                     //   on input,the max length of the buffer pointed to by stringValue
                     //   on output, the actual length of the string attribute
 char * stringValue) // null terminated string returned here
{
  Lng32 retcode = 0;
  
  Int64 version = str_atoi(&queryId[VERSION_OFFSET], VERSION_LEN);

  switch (attr)
    {
    case SQLQUERYID_VERSION:
      value = version;
      if (value < 0)
         return -CLI_INVALID_ATTR_VALUE;
      break;
    case SQLQUERYID_SEGMENTNUM:
      {
        // on SQ, the SEGMENT_NUM and CPUNUM is the same field - 3 digits
        if (version == NEO_SQL_ID_VERSION)
	  value = str_atoi(&queryId[SEGMENT_OFFSET], NEO_SEGMENT_LEN);
        else
	  value = str_atoi(&queryId[SEGMENT_OFFSET], SEGMENT_LEN);
	if (value < 0)
	  return -CLI_INVALID_ATTR_VALUE;
      }
    break;
    
    case SQLQUERYID_SEGMENTNAME:
      {
	// not implemented yet
        if(stringValue && value)
          stringValue[0] = 0;
      }
    break;

    case SQLQUERYID_CPUNUM:
      {
        // on SQ, the SEGMENT_NUM and CPUNUM is the same field - 3 digits
        if (version == NEO_SQL_ID_VERSION)
	  value = str_atoi(&queryId[NEO_CPU_OFFSET], NEO_CPU_LEN);
        else
	  value = str_atoi(&queryId[CPU_OFFSET], CPU_LEN);
	if (value < 0)
	  return -CLI_INVALID_ATTR_VALUE;
      }
    break;

    case SQLQUERYID_PIN:
      {
        if (version == NEO_SQL_ID_VERSION)
	   value = str_atoi(&queryId[NEO_PIN_OFFSET], NEO_PIN_LEN);
        else
	   value = str_atoi(&queryId[PIN_OFFSET], PIN_LEN);
	if (value < 0)
	  return -CLI_INVALID_ATTR_VALUE;
      }
    break;

    case SQLQUERYID_EXESTARTTIME:
      {
	value = str_atoi(&queryId[STARTTS_OFFSET], STARTTS_LEN);
	if (value < 0)
	  return -CLI_INVALID_ATTR_VALUE;
      }
    break;

    case SQLQUERYID_SESSIONNUM:
      {
	value = str_atoi(&queryId[SESSIONNUM_OFFSET], SESSIONNUM_LEN);
	if (value < 0)
	  return -CLI_INVALID_ATTR_VALUE;
      }
    break;

    case SQLQUERYID_USERNAME:
      {
	Lng32 userNameLen = 
	  (Lng32)str_atoi(&queryId[USERNAMELEN_OFFSET], USERNAMELEN_LEN);
	if (userNameLen < 0)
	  return -CLI_INVALID_ATTR_VALUE;
	if (stringValue && value > userNameLen)
	  {
            strncpy(stringValue, &queryId[USERNAME_OFFSET], userNameLen);
            stringValue[userNameLen] = 0;
	  }
        else
            retcode = -CLI_INVALID_ATTR_VALUE ;

        value = userNameLen;
      }
    break;

    case SQLQUERYID_SESSIONNAME:
      {
	Lng32 userNameLen = 
	  (Lng32)str_atoi(&queryId[USERNAMELEN_OFFSET], USERNAMELEN_LEN);
	if (userNameLen < 0)
	  return -CLI_INVALID_ATTR_VALUE;

	Lng32 sessionNameLenOffset =
	  USERNAMELEN_OFFSET + USERNAMELEN_LEN + userNameLen;
	Lng32 sessionNameLen = 
	  (Lng32)str_atoi(&queryId[sessionNameLenOffset], SESSIONNAMELEN_LEN);
	if (sessionNameLen < 0)
	  return -CLI_INVALID_ATTR_VALUE;
	if (stringValue && value > sessionNameLen )
	  {
	    strncpy(stringValue, 
		    &queryId[sessionNameLenOffset+SESSIONNAMELEN_LEN], 
		    sessionNameLen);

	    stringValue[sessionNameLen] = 0;
          }
        else
            retcode = -CLI_INVALID_ATTR_VALUE;
       
        value = sessionNameLen;
      }
    break;

    case SQLQUERYID_SESSIONID:
      {
        // session Id = qid from start thru SessionName
        
        // set current offset to userNameLen
        Int32 currOffset = USERNAMELEN_OFFSET;
        
        // get userNameLen
        Int32 nameLen = (Int32)str_atoi(&queryId[currOffset], USERNAMELEN_LEN);
        
        // go past Username
        currOffset += USERNAMELEN_LEN + nameLen;
        
        // get sessionName length
        nameLen = (Int32)str_atoi(&queryId[currOffset], SESSIONNAMELEN_LEN);
	if (nameLen < 0)
	  return -CLI_INVALID_ATTR_VALUE;
        
        // go past sessionName
        currOffset += SESSIONNAMELEN_LEN + nameLen; 
	
        // check value for length
        if(stringValue && value > currOffset)
        { 
          // copy sessionId to stringValue
          strncpy(stringValue, queryId, currOffset);
          stringValue[currOffset]=0;
        }
        else
            retcode = -CLI_INVALID_ATTR_VALUE;
         
        value = currOffset;
     }
    break;

    case SQLQUERYID_QUERYNUM:
      {
	Lng32 currOffset = USERNAMELEN_OFFSET;
	Lng32 nameLen = 
	  (Lng32)str_atoi(&queryId[currOffset], USERNAMELEN_LEN);
	if (nameLen < 0)
	  return -CLI_INVALID_ATTR_VALUE;

	// skip user name
	currOffset += USERNAMELEN_LEN + nameLen;

	// skip session name
	nameLen = 
	  (Lng32)str_atoi(&queryId[currOffset], SESSIONNAMELEN_LEN);
	if (nameLen < 0)
	  return -CLI_INVALID_ATTR_VALUE;
	currOffset += SESSIONNAMELEN_LEN + nameLen;

	if (currOffset >= queryIdLen)
	  return -CLI_INVALID_ATTR_VALUE;
	
	if (queryId[currOffset] != '_')
	  return -CLI_INVALID_ATTR_VALUE;

	// skip "_" separator
	currOffset += 1;

	// find the next occurance of "_"
	char * us = str_chr(&queryId[currOffset], '_');
	if (us == NULL)
	  return -CLI_INVALID_ATTR_VALUE;

	if ((currOffset + (us - (char *)(&queryId[currOffset]))) >= queryIdLen)
	  return -CLI_INVALID_ATTR_VALUE;

	value = str_atoi(&queryId[currOffset], 
			 us - (char *)(&queryId[currOffset]));
	if (value < 0)
	  return -CLI_INVALID_ATTR_VALUE;
      }
    break;

    case SQLQUERYID_STMTNAME:
      {
	Lng32 currOffset = USERNAMELEN_OFFSET;
	Lng32 nameLen = 
	  (Lng32)str_atoi(&queryId[currOffset], USERNAMELEN_LEN);
	if (nameLen < 0)
	  return -CLI_INVALID_ATTR_VALUE;

	// skip user name
	currOffset += USERNAMELEN_LEN + nameLen;

	// skip session name
	nameLen = 
	  (Lng32)str_atoi(&queryId[currOffset], SESSIONNAMELEN_LEN);
	if (nameLen < 0)
	  return -CLI_INVALID_ATTR_VALUE;
	currOffset += SESSIONNAMELEN_LEN + nameLen;
	
	if (currOffset >= queryIdLen)
	  return -CLI_INVALID_ATTR_VALUE;
	  
	if (queryId[currOffset] != '_')
	  return -CLI_INVALID_ATTR_VALUE;
	  
	// skip "_" separator
	currOffset += 1;

	// find the next occurance of "_"
	char * us = str_chr(&queryId[currOffset], '_');
	if (us == NULL)
	  return -CLI_INVALID_ATTR_VALUE;

	// skip queryNum
	currOffset += us - (char *)(&queryId[currOffset]);

	if (currOffset >= queryIdLen)
	  return -CLI_INVALID_ATTR_VALUE;

	// skip "_" separator
	currOffset += 1;
        
        //get statementName length as remaining string except trailing blanks
        while (queryIdLen && queryId[queryIdLen-1] == ' ')
          queryIdLen--;

        Int32 qidStatementNameLen = queryIdLen - currOffset;
        
        // check for enough space in StringValue
        if(stringValue && value > qidStatementNameLen )
        {        
	  // copy and null terminate 
          strncpy(stringValue, &queryId[currOffset], qidStatementNameLen);
          stringValue[qidStatementNameLen] = 0;
        }
        else
	  retcode = -CLI_INVALID_ATTR_VALUE;
        
        value = qidStatementNameLen;

      }
    break;

    default:
      {
        retcode = -CLI_INVALID_ATTR_NAME;
      } 

    }

  return retcode;
}

Lng32 ComSqlId::getSqlQueryIdAttr
(Lng32 attr,         // which attr (SqlQueryIDAttr)
 char * queryId,    // query ID
 Lng32 queryIdLen,   // query ID len
 Int64 &value,      // If returned attr is of string type, this value is the
                    // max length of the buffer pointed to by stringValue.
                    // If returned attr is numeric, this field contains
                    // the returned value.
 char * stringValue) // null terminated returned value for string attrs.
{
  Lng32 retcode = 0;

  if ((queryId == NULL) ||
      (queryIdLen < MIN_QUERY_ID_LEN) ||
      (strncmp(queryId, COM_SESSION_ID_PREFIX, 
	       strlen(COM_SESSION_ID_PREFIX)) != 0))
    return -CLI_INVALID_ATTR_VALUE;

  retcode = getSqlIdAttr(attr, queryId, queryIdLen, value, stringValue);
  return retcode;
}

Lng32 ComSqlId::getSqlSessionIdAttr
(Lng32 attr,         // which attr (SqlQueryIDAttr)
 char * queryId,    // query ID
 Lng32 queryIdLen,   // query ID len
 Int64 &value,      // If returned attr is of string type, this value is the
                    // max length of the buffer pointed to by stringValue.
                    // If returned attr is numeric, this field contains
                    // the returned value.
 char * stringValue) // null terminated returned value for string attrs.
{
  Lng32 retcode = 0;

  if ((queryId == NULL) ||
      (queryIdLen < MIN_SESSION_ID_LEN) ||
      (strncmp(queryId, COM_SESSION_ID_PREFIX, 
	       strlen(COM_SESSION_ID_PREFIX)) != 0))
    return -CLI_INVALID_ATTR_VALUE;

  retcode = getSqlIdAttr(attr, queryId, queryIdLen, value, stringValue);
  return retcode;
}

Lng32 ComSqlId::extractSqlSessionIdAttrs
(const char * sessionId,       // IN
 Lng32 maxSessionIdLen,        // IN
 Int64 &segmentNumber,         // OUT
 Int64 &cpu,                   // OUT
 Int64 &pin,                   // OUT
 Int64 &processStartTS,       // OUT
 Int64 &sessionUniqueNum,     // OUT
 Lng32 &userNameLen,           // OUT
 char * userName,             // OUT
 Lng32 &userSessionNameLen,    // OUT
 char * userSessionName,       // OUT
 Lng32 *version
 )
{
  Lng32 retcode = 0;
  Int64 lc_version;

  if ((sessionId == NULL) ||
      ((maxSessionIdLen > 0) && (maxSessionIdLen < MIN_SESSION_ID_LEN)) ||
      (strncmp(sessionId, COM_SESSION_ID_PREFIX, 
	       strlen(COM_SESSION_ID_PREFIX)) != 0))
    return -1;
  
  retcode = getSqlIdAttr(SQLQUERYID_SEGMENTNUM, sessionId,
			 maxSessionIdLen, segmentNumber, NULL);
  if (retcode)
    return retcode;

  retcode = getSqlIdAttr(SQLQUERYID_CPUNUM, sessionId,
			 maxSessionIdLen, cpu, NULL);
  if (retcode)
    return retcode;

  retcode = getSqlIdAttr(SQLQUERYID_PIN, sessionId,
			 maxSessionIdLen, pin, NULL);
  if (retcode)
    return retcode;

  retcode = getSqlIdAttr(SQLQUERYID_EXESTARTTIME, sessionId,
			 maxSessionIdLen, processStartTS, NULL);
  if (retcode)
    return retcode;

  retcode = getSqlIdAttr(SQLQUERYID_SESSIONNUM, sessionId,
			 maxSessionIdLen, sessionUniqueNum, NULL);
  if (retcode)
    return retcode;

  if (userName)
    {
      // TBD
      return -1;
    }

  if (userSessionName)
    {
      // TBD
      return -1;
    }
  if (version != NULL)
  {
     retcode = getSqlIdAttr(SQLQUERYID_VERSION, sessionId,
			 maxSessionIdLen, lc_version, NULL);
     if (retcode)
        return retcode;
     *version = (Lng32)lc_version;
  }
  return 0;
}


/*
  DP2 Query Id Format:
    Each of the part is in binary numeric format.

  <segment><cpu><pin><processStartTS><queryNum>

  <segment>        :      1 byte
  <cpu>            :      1 byte
  <pin>            :      2 bytes(short)
  <processStartTS> :      8 bytes (Int64)
  <queryNum>       :      4 bytes (Lng32)
*/
Lng32 ComSqlId::getDp2QueryIdString
(char * queryId,                 // IN
 Lng32 queryIdLen,                // IN
 char * dp2QueryId,              // INOUT
 Lng32 &dp2QueryIdLen             // OUT
 )
{
  if ((!queryId) ||
      (queryIdLen < MAX_DP2_QUERY_ID_LEN))
    return -1;

  Int64 value;
  Lng32 currOffset = 0;
  // In case of Linux and NT, segment num and cpu num are same
  // Copy them as short
  // get cpu
  getSqlQueryIdAttr(SQLQUERYID_CPUNUM, queryId, queryIdLen,
		    value, NULL);
  *(short*)&dp2QueryId[currOffset] = (short)value;
  currOffset += sizeof(short);

  // get pin
  getSqlQueryIdAttr(SQLQUERYID_PIN, queryId, queryIdLen,
		    value, NULL);
  *(short*)&dp2QueryId[currOffset] = (short)value;
  currOffset += sizeof(short);

  // get process start time
  getSqlQueryIdAttr(SQLQUERYID_EXESTARTTIME, queryId, queryIdLen,
		    value, NULL);
  str_cpy_all(&dp2QueryId[currOffset], (char*)&value, sizeof(Int64));
  currOffset += sizeof(Int64);

  // get query num
  getSqlQueryIdAttr(SQLQUERYID_QUERYNUM, queryId, queryIdLen,
		    value, NULL);
  Lng32 qNum = (Lng32)value;
  str_cpy_all(&dp2QueryId[currOffset], (char*)&qNum, sizeof(Lng32));

  currOffset += sizeof(Lng32);

  dp2QueryIdLen = currOffset;

  return 0;
}

Lng32 ComSqlId::decomposeDp2QueryIdString
(char * queryId,                    // input: buffer containing dp2 query id
 Lng32 queryIdLen,                   // input: length of query id
 Lng32 *queryNum,                    // output: unique query number
 Lng32 *segment,                     // output: segment number of master exe
 Lng32 *cpu,                         // output: cpu number
 Lng32 *pin,                         // output: pin
 Int64 *timestamp                   // output: master exe process 
                                    //         start time
 )
{
  if ((!queryId) ||
      (queryIdLen < MAX_DP2_QUERY_ID_LEN))
    return -1;

  Lng32 currOffset = 0;
  *segment = *(short*)&queryId[currOffset];
  *cpu = *segment;  
  currOffset += sizeof(short);

  *pin = *(short*)&queryId[currOffset];
  currOffset += sizeof(short);

  str_cpy_all((char*)timestamp, &queryId[currOffset], sizeof(Int64));
  currOffset += sizeof(Int64);

  str_cpy_all((char*)queryNum, &queryId[currOffset], sizeof(Lng32));
  currOffset += sizeof(Lng32);

  return 0;
}








