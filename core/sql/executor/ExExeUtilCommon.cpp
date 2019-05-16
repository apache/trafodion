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

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExExeUtilCommon.cpp
 * Description:  
 *               
 *               
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComCextdecs.h"
#include "ComSizeDefs.h"
#include  "cli_stdh.h"
#include  "ex_stdh.h"
#include  "sql_id.h"
#include  "ex_transaction.h"
#include  "ComTdb.h"
#include  "ex_tcb.h"
#include  "ComSqlId.h"

#include  "ExExeUtil.h"
#include  "ex_exe_stmt_globals.h"
#include  "exp_expr.h"
#include  "exp_clause_derived.h"
#include  "ComRtUtils.h"
#include  "ExStats.h"
#include  "ComSmallDefs.h"

#include "logmxevent.h"

// Generate a lock name from the input simple object name and the given suffix.
// If the generated name is longer than 128 NAWchar's, remove the extra char's
// from the simple object name part in the generated lock name without changing
// the suffix.
//
static NABoolean generateLockName(const char * objInternal1PartName, // in UTF-8 format
                                  const char * lockSuffix, // contains 7-bit ASCII chars only
                                  char * lockInternal1PartNameOutBuf, // in UTF-8 format
                                  Int32 lockInInternal1PartNameOutBufLen) // in bytes
{
  Int32 objInt1PartNameLen = str_len(objInternal1PartName);
  char *tempPtr;
  char buf[20];
  Int32 hashValue = 0;
  Int32 suffixLen = str_len(lockSuffix)+8; // 8 characters for hashvalue of truncated
                                           // object name
  // More than 60 characters won't be truncated and hence 8 characters should be
  // good enough
  Int32 allowedLen = lightValidateUTF8Str(objInternal1PartName,
                                          objInt1PartNameLen,
                      // max allowed chars in UTF-8 (same as allowed bytes in ISO)
                            ComMAX_1_PART_INTERNAL_ISO88591_NAME_LEN -suffixLen);

  if (lockInternal1PartNameOutBuf == NULL ||
      allowedLen + suffixLen >= lockInInternal1PartNameOutBufLen)
    return FALSE;
  if (allowedLen < objInt1PartNameLen)
  {
     // Just simple hashing by adding the byte contents
     tempPtr = (char *)objInternal1PartName + allowedLen;
     for (; *tempPtr != '\0';)
        hashValue += *tempPtr++;
  }
  str_cpy_all(lockInternal1PartNameOutBuf,
              objInternal1PartName,
              allowedLen);
  lockInternal1PartNameOutBuf[allowedLen] = '\0';
  if (hashValue > 0)
  {
     str_sprintf(buf, "%d", hashValue); 
     buf[8] = '\0';
     strcat(lockInternal1PartNameOutBuf, buf);
  }
  strcat(lockInternal1PartNameOutBuf, lockSuffix);
  return TRUE;
}

ex_tcb * ExExeUtilTdb::build(ex_globals * glob)
{
  ExExeUtilTcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilTcb(*this, NULL, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}


////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilTcb
///////////////////////////////////////////////////////////////
ExExeUtilTcb::ExExeUtilTcb(const ComTdbExeUtil & exe_util_tdb,
			   const ex_tcb * child_tcb, // for child queue
			   ex_globals * glob)
     : ex_tcb( exe_util_tdb, 1, glob),
       workAtp_(NULL),
       query_(NULL),
       infoList_(NULL),
       infoListIsOutputInfo_(TRUE),
       explQuery_(NULL),
       childQueryId_(NULL),
       childQueryIdLen_(0),
       outputBuf_(NULL)
{
  Space * space = (glob ? glob->getSpace() : 0);
  CollHeap * heap = (glob ? glob->getDefaultHeap() : 0);

  // Allocate the buffer pool
  pool_ = new(space) sql_buffer_pool(exe_util_tdb.numBuffers_,
				     exe_util_tdb.bufferSize_,
				     space);
  
  childTcb_ = child_tcb;
  if (childTcb_)
    {
      qchild_  = childTcb_->getParentQueue();
    }

  // Allocate the queue to communicate with parent
  qparent_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
				      exe_util_tdb.queueSizeDown_,
				      exe_util_tdb.criDescDown_,
				      space);
  
  qparent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
				    exe_util_tdb.queueSizeUp_,
				    exe_util_tdb.criDescUp_,
				    space);
  
  if (exe_util_tdb.workCriDesc_)
    {
      workAtp_ = allocateAtp(exe_util_tdb.workCriDesc_, glob->getSpace());
      pool_->get_free_tuple(workAtp_->getTupp(((ComTdbExeUtil&)exe_util_tdb).workAtpIndex()), 0);
    }
  
  tcbFlags_ = 0;
  
  if (exe_util_tdb.inputExpr_)
    (void)exe_util_tdb.inputExpr_->fixup(0, getExpressionMode(), this,
					 space, heap, FALSE, glob);
  if (exe_util_tdb.outputExpr_)
    (void)exe_util_tdb.outputExpr_->fixup(0, getExpressionMode(), this, 
					  space, heap, FALSE, glob);

  if (exe_util_tdb.scanExpr_)
    (void)exe_util_tdb.scanExpr_->fixup(0, getExpressionMode(), this, 
					  space, heap, FALSE, glob);

  ContextCli * currContext = 
    (glob->castToExExeStmtGlobals()->castToExMasterStmtGlobals() ?
     glob->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->getStatement()->getContext() :
     NULL);
    
  // internal queries are already in isoMapping and do not need to be
  // translated before sending to mxcmp.
  // Set the ISO_MAPPING charset code to indicate that.
  char *parentQid;
  if (glob->castToExExeStmtGlobals()->castToExMasterStmtGlobals())
    parentQid = glob->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->getStatement()->getUniqueStmtId();
  else
  if (glob->castToExExeStmtGlobals()->castToExEspStmtGlobals() &&
      glob->castToExExeStmtGlobals()->castToExEspStmtGlobals()->getStmtStats())
    parentQid = glob->castToExExeStmtGlobals()->castToExEspStmtGlobals()->getStmtStats()->getQueryId();
  else
    parentQid = NULL;

  cliInterface_ = new(heap) ExeCliInterface(heap,
					    SQLCHARSETCODE_ISO88591,  // ISO_MAPPING=ISO88591
					    currContext,
                                            parentQid);
  
  cliInterface2_ = new(heap) ExeCliInterface(heap,
					     SQLCHARSETCODE_ISO88591,  // ISO_MAPPING=ISO88591
					     currContext,
					     parentQid);

  diagsArea_ = NULL;
  
  pqStep_ = PROLOGUE_;

  VersionToString(COM_VERS_MXV, versionStr_);
  versionStrLen_ = DIGITS_IN_VERSION_NUMBER; 

  VersionToString(COM_VERS_MXV, sysVersionStr_);
  sysVersionStrLen_ = DIGITS_IN_VERSION_NUMBER;

  extractedPartsObj_ = NULL;
};

ExExeUtilTcb::~ExExeUtilTcb()
{
  delete qparent_.up;
  delete qparent_.down;
  if (workAtp_)
    {
      workAtp_->release();
      deallocateAtp(workAtp_, getGlobals()->getSpace());
      workAtp_ = NULL;
    }
  freeResources();

  if (extractedPartsObj_)
    {
      delete extractedPartsObj_;
      extractedPartsObj_ = NULL;
    }
  if (explQuery_)
    NADELETEBASIC(explQuery_, getHeap());
  if (childQueryId_ != NULL)
  {
    NADELETEBASIC(childQueryId_, getHeap());
    childQueryId_ = NULL;
    childQueryIdLen_ = 0;
  }
  if (outputBuf_ != NULL)
  {
    NADELETEBASIC(outputBuf_, getHeap());
    outputBuf_ = NULL;
    outputBuf_ = 0;
  }

};

ex_queue_pair ExExeUtilTcb::getParentQueue() const {return qparent_;}

Int32 ExExeUtilTcb::orderedQueueProtocol() const
{
  return ((const ExExeUtilTdb &)tdb).orderedQueueProtocol();
}

Int32 ExExeUtilTcb::numChildren() const 
{ 
  if (childTcb_)
    return 1;
  else
    return 0; 
}   

const ex_tcb* ExExeUtilTcb::getChild(Int32 pos) const 
{
  if (pos == 0)
    return childTcb_;
  else
    return NULL;
};

void ExExeUtilTcb::freeResources()
{
  delete pool_;
  pool_ = 0;
}

short ExExeUtilTcb::work()
{
  // nothing implemented here, the derived classes contains real work.
  ex_assert(0, "ExExeUtilTcb::work()  MUST be redefined");
  return -1;
}

NABoolean ExExeUtilTcb::isUpQueueFull(short size)
{
  if ((qparent_.up->getSize() - qparent_.up->getLength()) < size)
    return TRUE;
  else
    return FALSE;
}

short ExExeUtilTcb::moveRowToUpQueue(const char * row, Lng32 len, 
				     short * rc, NABoolean isVarchar)
{
  short retcode = ex_tcb::moveRowToUpQueue(&qparent_, exeUtilTdb().tuppIndex_,
                                           row, len, rc, isVarchar);
  return  retcode;
}

char * ExExeUtilTcb::getTimeAsString(Int64 elapsedTime, char * timeBuf,
                                     NABoolean noUsec)
{
  ULng32 sec = (ULng32) (elapsedTime / 1000000);
  ULng32 usec = (ULng32) (elapsedTime % 1000000);
  ULng32 min = sec/60;
  sec = sec % 60;
  ULng32 hour = min/60;
  min = min % 60;
  
  if (noUsec)
    str_sprintf (timeBuf,  "%02u:%02u:%02u",
                 hour, min, sec);
  else
    str_sprintf (timeBuf,  "%02u:%02u:%02u.%03u",
                 hour, min, sec, TO_FMT3u(usec));
   
  return timeBuf;
}

char * ExExeUtilTcb::getTimestampAsString(Int64 juliantimestamp, 
					  char * timeBuf)
{
  short timestamp[8];
  Int64 localTimestamp = CONVERTTIMESTAMP(juliantimestamp,0,-1,0);
  INTERPRETTIMESTAMP(localTimestamp, timestamp);
  short year = timestamp[0];
  char month = (char) timestamp[1];
  char day = (char) timestamp[2];
  char hour = (char) timestamp[3];
  char minute = (char) timestamp[4];
  char second = (char) timestamp[5];
  Lng32 fraction = timestamp[6] * 1000 + timestamp[7];
  
  str_sprintf (timeBuf,  "%04u-%02u-%02u %02u:%02u:%02u.%03u",
	       year, month, day, hour, minute, second, fraction);

  return timeBuf;
}

// ----------------------------------------------------------------------------
// Method:  glueQueryFragments
//
// This method combines the pieces of the metadata query into a single
// statement.  As part of this process, the leading spaces are removed.
//
// Input:  queryArraySize - number of fragments to glue together
//         QueryString - the fragments
//
// Output:  gluedQuery - the concatenated fragments
//          gluedQuerySize - the final length
//
// Space is allocated for the gluedQuery
// ----------------------------------------------------------------------------
void ExExeUtilTcb::glueQueryFragments(Lng32 queryArraySize,
				      const QueryString * queryArray,
				      char * &gluedQuery,
				      Lng32 &gluedQuerySize)
{
  Int32 i = 0;
  gluedQuerySize = 0;
  gluedQuery = NULL;
  NAString concatenatedQuery;
  NAString tempStr;

  for (i = 0; i < queryArraySize; i++)
    {
      tempStr = queryArray[i].str;
      concatenatedQuery += tempStr.strip(NAString::leading, ' ');
    }
  
  gluedQuerySize = concatenatedQuery.length();
  gluedQuery = new(getMyHeap()) char[gluedQuerySize + 100];
  strncpy(gluedQuery, concatenatedQuery.data(), gluedQuerySize);
  gluedQuery[gluedQuerySize] = '\0';
}

Lng32 ExExeUtilTcb::changeAuditAttribute(char * tableName,
					NABoolean makeAudited,
					NABoolean isVolatile,
					NABoolean isIndex,
					NABoolean parallelAlter)
{
  Lng32 retcode = 0;

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  // set sqlparserflags to allow change of audit attribute
  masterGlob->getStatement()->getContext()->setSqlParserFlags(0x400); // ALLOW_AUDIT_CHANGE	
  
  // make table unaudited
  char stmt[500];
  strcpy(stmt, "alter ");
  if (isVolatile)
    strcat(stmt, "volatile ");
  strcat(stmt, "table ");
  strcat(stmt, tableName);
  if (makeAudited)
    strcat(stmt, " attribute audit");
  else
    strcat(stmt, " attribute no audit");

  if (parallelAlter)
    strcat(stmt, " no label update");

  strcat(stmt, ";");
  ComDiagsArea *diagsArea = getDiagsArea();
  retcode = cliInterface()->executeImmediate
    (stmt, NULL, NULL, TRUE, NULL, 0, &diagsArea); 
  setDiagsArea(diagsArea);
  masterGlob->getStatement()->getContext()->resetSqlParserFlags(0x400); // ALLOW_AUDIT_CHANGE
  
  if (retcode < 0)
    return retcode;

  if (parallelAlter)
    {
      retcode = alterAuditFlag(makeAudited, tableName, isIndex);
      if (retcode < 0)
	return retcode;
    }

  return 0;
}

void ExExeUtilTcb::handleErrors(Lng32 rc)
{ 
  cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
}

short ExExeUtilTcb::initializeInfoList(Queue* &infoList)
{
  return cliInterface()->initializeInfoList(infoList, infoListIsOutputInfo_);
}

short ExExeUtilTcb::fetchAllRows(Queue * &infoList, 
				 char * query,
				 Lng32 numOutputEntries,
				 NABoolean varcharFormat,
				 short &rc,
				 NABoolean monitorThis)
{
  rc = cliInterface()->fetchAllRows(infoList, query, numOutputEntries, varcharFormat,
				    monitorThis);
  if (rc < 0)
    {
      handleErrors(rc);
      return -1;
    }
  
  return 0;
}

char * ExExeUtilTcb::getStatusString(const char * operation,
				     const char * status,
				     const char * object,
				     char * outBuf,
				     NABoolean isET, 
				     char * timeBuf,
				     char * queryBuf,
				     char * errorBuf)
{
  if (! outBuf)
    return NULL;

  char o[16];
  char s[10];
  byte_str_cpy(o, 15, operation, strlen(operation), ' ');
  o[15] = 0;
  byte_str_cpy(s, 9, status, strlen(status), ' ');
  s[9] = 0;
  if (queryBuf)
    {
      str_sprintf(outBuf, "Task: %s  Status: %s  Command: %s",
		  o, s, queryBuf);
    }
  else if (timeBuf && isET)
    {
      str_sprintf(outBuf, "Task: %s  Status: %s  Elapsed Time:    %s",
		  o, s, timeBuf);
    }
  else if (errorBuf)
    {
      str_sprintf(outBuf, "Task: %s  Status: %s  Details: %s",
		  o, s, errorBuf);
    }
  else if (timeBuf)
    {
       str_sprintf(outBuf, "Task: %s  Status: %s  Time: %s",
		    o, s, timeBuf);
    }
  else
    {
      if (object)
         str_sprintf(outBuf, "Task: %s  Status: %s  Object: %s",
		    o, s, object);
      else
         str_sprintf(outBuf, "Task: %s  Status: %s",
		    o, s);
    }

  return outBuf;
}

short ExExeUtilTcb::executeQuery(char * task, 
				 char * object,
				 char * query,
				 NABoolean displayStartTime,
				 NABoolean displayEndTime,
				 short &rc,
				 short * warning,
				 Lng32 * ec,
				 NABoolean moveErrorRow,
				 NABoolean continueOnError,
				 NABoolean monitorThis)
{
  short retcode = 0;
  char buf[BUFFER_SIZE];
  char timeBuf[200];

  while (1)
    {
      switch (pqStep_)
	{
	case PROLOGUE_:
	  {
	    warning_ = 0;
	    startTime_ = NA_JulianTimestamp();
	    elapsedTime_ = 0;
	    if (displayStartTime)
	      {
		getTimestampAsString(startTime_, timeBuf);
		getStatusString(task, "Started", object, buf, FALSE, timeBuf);
		if (moveRowToUpQueue(buf, 0, &rc))
		  return 1;
	      }

	    pqStep_ = EXECUTE_;
	    rc = WORK_RESCHEDULE_AND_RETURN;
	    return 1;
	  }
	  break;
	
	case EXECUTE_:
	  {
	    retcode = cliInterface()->fetchRowsPrologue(query,FALSE,monitorThis);
	    if (retcode < 0)
	      {
		pqStep_ = ERROR_RETURN_;
		break;
	      }
	    
	    pqStep_ = FETCH_ROW_;
	  }
	  break;
	
	case FETCH_ROW_:
	  {
	    retcode = (short)cliInterface()->fetch();
	    if (retcode < 0)
	      {
		pqStep_ = ERROR_RETURN_;
		break;
	      }
	    
	    if ((retcode > 0) &&
		(retcode != 100))
	      warning_ = retcode;

	    if ((retcode != 100) &&
		(cliInterface()->outputBuf()))
	      pqStep_ = RETURN_ROW_;
	    else
	      pqStep_ = CLOSE_;
	  }
	  break;
	
	case RETURN_ROW_:
	  {
	    char * ptr;
	    Lng32   len;
	    
	    cliInterface()->getPtrAndLen(1, ptr, len);
	    retcode = moveRowToUpQueue(ptr, len, &rc);
	    if (retcode)
	      return 1;
	    
	    pqStep_ = FETCH_ROW_;
	  }
	  break;
	
	case CLOSE_:
	  {
	    retcode = cliInterface()->fetchRowsEpilogue("");
	    if (retcode < 0)
	      {
		pqStep_ = ERROR_RETURN_;
		break;
	      }

	    pqStep_ = EPILOGUE_;
	  }
	  break;
	
	case EPILOGUE_:
	  {
            endTime_ = NA_JulianTimestamp();
            elapsedTime_ = endTime_ - startTime_;

	    if (displayEndTime)
	      {
		getTimestampAsString(endTime_, timeBuf);
		getStatusString(task, "Ended", object, buf, FALSE, timeBuf);
		if (moveRowToUpQueue(buf, 0, &rc))
		  return 1;
		getTimeAsString(elapsedTime_, timeBuf);
		getStatusString(task, "Ended", object, buf, TRUE, timeBuf);
		if (moveRowToUpQueue(buf, 0, &rc))
		  return 1;
	      }

	    pqStep_ = ALL_DONE_;
	    rc = WORK_RESCHEDULE_AND_RETURN;
	    return 1;
	  }
	  break;
	
	case ERROR_RETURN_:
	  {
	    Lng32 sqlcode = 0;
	    char * stringParam1 = NULL;
	    Lng32   intParam1 = ComDiags_UnInitialized_Int;

            cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
            if (getDiagsArea() != NULL)
	        retcode = 0;
	    if (moveErrorRow)
	      {
		if (retcode == 0)
		  {
		    ComDiagsArea * da = getDiagsArea();
		    
		    sqlcode = (short)da->mainSQLCODE();
		    
		    ComCondition * cc;
		    if (sqlcode < 0)
		      cc = da->getErrorEntry(1);
		    else
		      cc = da->getWarningEntry(1);
		    
		    if (sqlcode < 0 && ec != NULL)
		      *ec = sqlcode;
		    
		    if (cc->getOptionalStringCharSet(0) == CharInfo::ISO88591 || cc->getOptionalStringCharSet(0) == CharInfo::UTF8)
		      stringParam1 = (char*)cc->getOptionalString(0);
		    else
		      stringParam1 = NULL;
		    intParam1    = cc->getOptionalInteger(0);
		  }
		else
		  {
		    sqlcode = retcode;
		  }
		
	 Lng32 errorBufLen = 
		  200 + (stringParam1 ? strlen(stringParam1) : 0);
		
		char * errorBuf = new(getHeap()) char[errorBufLen];
		
		str_sprintf(errorBuf, "%d", sqlcode);
		if (stringParam1)
		  str_sprintf(&errorBuf[strlen(errorBuf)], ", %s", stringParam1);
		if (intParam1 != ComDiags_UnInitialized_Int)
		  str_sprintf(&errorBuf[strlen(errorBuf)], ", %d", intParam1);
		
		char * outBuf = new(getHeap()) char[errorBufLen+400];
		getStatusString(task, "Error", NULL, outBuf,
				FALSE, NULL,
				errorBuf);
		
		NADELETEBASIC(errorBuf, getHeap());
		
		if ((moveErrorRow) &&
		    (moveRowToUpQueue(outBuf, 0, &rc)))
		  {
		    NADELETEBASIC(outBuf, getHeap());
		    
		    return 1;
		  }

		NADELETEBASIC(outBuf, getHeap());
	      }

	    // close cursor, etc. Ignore errors.
	    cliInterface()->fetchRowsEpilogue("");

	    if (continueOnError)
	      {
		pqStep_ = ALL_DONE_;

		rc = WORK_RESCHEDULE_AND_RETURN;
		return 1;
	      }
	    else
	      {
		pqStep_ = PROLOGUE_;
		return -1;
	      }
	  }
	  break;

	case ALL_DONE_:
	  {
	    pqStep_ = PROLOGUE_;

	    if (warning)
	      *warning = warning_;

	    return 0;
	  }
	  break;
	
	}
    }
}

short ExExeUtilTcb::holdAndSetCQD(const char * defaultName, const char * defaultValue,
				  ComDiagsArea * globalDiags)
{
  Lng32 cliRC;

  cliRC = holdAndSetCQD(defaultName, defaultValue, cliInterface(),
			globalDiags);
  if (cliRC < 0)
    {
      handleErrors(cliRC);
      return -1;
    }

  return 0;
}

short ExExeUtilTcb::restoreCQD(const char * defaultName, ComDiagsArea * globalDiags)
{
  Lng32 cliRC;

  cliRC = restoreCQD(defaultName, cliInterface(), globalDiags);
  if (cliRC < 0)
    {
      handleErrors(cliRC);
      return -1;
    }

  return 0;
}

Lng32 ExExeUtilTcb::holdAndSetCQD(const char * defaultName, const char * defaultValue,
				 ExeCliInterface * cliInterface,
				 ComDiagsArea * globalDiags)
{
  Lng32 cliRC;
  
  cliRC = cliInterface->holdAndSetCQD(defaultName, defaultValue, globalDiags);
  return cliRC;
}

Lng32 ExExeUtilTcb::restoreCQD(const char * defaultName, 
			      ExeCliInterface * cliInterface,
			      ComDiagsArea * globalDiags)
{
  Lng32 cliRC;

  cliRC = cliInterface->restoreCQD(defaultName, globalDiags);
  
  return cliRC;
}

short ExExeUtilTcb::setCS(const char * csName, char * csValue, 
			  ComDiagsArea * globalDiags)
{
  Lng32 cliRC;

  cliRC = setCS(csName, csValue, cliInterface(), globalDiags);
  if (cliRC < 0)
    {
      handleErrors(cliRC);
      return -1;
    }

  return 0;
}

short ExExeUtilTcb::resetCS(const char * csName, ComDiagsArea * globalDiags)
{
  Lng32 cliRC;

  cliRC = resetCS(csName, cliInterface(), globalDiags);
  if (cliRC < 0)
    {
      handleErrors(cliRC);
      return -1;
    }

  return 0;
}

Lng32 ExExeUtilTcb::setCS(const char * csName, char * csValue,
			 ExeCliInterface * cliInterface,
			 ComDiagsArea * globalDiags)
{
  Lng32 cliRC;

  char buf[400];
  
  strcpy(buf, "control session '");
  strcat(buf, csName);
  strcat(buf, "' '");
  strcat(buf, csValue);
  strcat(buf, "';");

  cliRC = 
    cliInterface->executeImmediate(buf, NULL, NULL, TRUE, NULL,FALSE,
				   &globalDiags);
  if (cliRC < 0)
    {
      return cliRC;
    }

  return 0;
}

Lng32 ExExeUtilTcb::resetCS(const char * csName, 
			   ExeCliInterface * cliInterface,
			   ComDiagsArea * globalDiags)
{
  Lng32 cliRC;

  char buf[400];
  
  strcpy(buf, "control session '");
  strcat(buf, csName);
  strcat(buf, "' reset;");
  cliRC = cliInterface->executeImmediate(buf, NULL, NULL, TRUE, NULL,FALSE,
					 &globalDiags);
  if (cliRC < 0)
    {
      return cliRC;
    }

  return 0;
}

short ExExeUtilTcb::disableCQS()
{
  // disable any CQS in affect
  Lng32 rc = cliInterface()->
    executeImmediate("control query shape hold;");
  if (rc < 0)
    {
      handleErrors(rc);
      return -1;
    }

  return 0;
}

short ExExeUtilTcb::restoreCQS()
{
  Lng32 rc = cliInterface()->
    executeImmediate("control query shape restore;");
  if (rc < 0)
    {
      handleErrors(rc);
      return -1;
    }

  return 0;
}

void ExExeUtilTcb::setMaintainControlTableTimeout(char * catalog)
{
  Lng32 cliRC;
  char buf[400+ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES];
  Lng32 markValue = -1;
  char timeoutHoldBuf[100];
  Lng32 timeoutHoldBufLen = 0;

  // The MAINTAIN_CONTROL_TABLE_TIMEOUT CQD default is '30000', 5 minutes
  restoreTimeout_ = FALSE;

  strcpy(buf, "showcontrol default ");
  strcat(buf, "MAINTAIN_CONTROL_TABLE_TIMEOUT");
  strcat(buf, " , match full, no header;");

  markValue = getDiagsArea()->mark();
   
  // get the current value of MAINTAIN_CONTROL_TABLE_TIMEOUT

  cliRC = 
    cliInterface()->
    executeImmediate(buf, timeoutHoldBuf, &timeoutHoldBufLen);

  // If we were unable to obtain the default value,
  // then just return and allow the 60 second general
  // timeout for a table to remain.

  if (cliRC < 0)
    {
      // Ignore any error occurring from the executeImmediate call.
      // Rewind to not report this one error.
      getDiagsArea()->rewind(markValue);
      SQL_EXEC_ClearDiagnostics(NULL);
      return;
    }

  // The timeout setting will be retained for the duration
  // of executing the maintain tasks.
  
  // Execute the set table timeout statement.
  // The SET TABLE TIMEOUT statement sets a dynamic timeout value for a lock timeout
  // or a stream timeout in the environment of the current session. The dynamic timeout
  // value overrides the compiled static timeout value in the execution of subsequent DML
  // statements.

  markValue = getDiagsArea()->mark();

  strcpy(buf, "SET TABLE ");
  strcat(buf, catalog);
  strcat(buf, ".\"@MAINTAIN_SCHEMA@\".\"@MAINTAIN_CONTROL_INFO@\" ");
  strcat(buf, " TIMEOUT ");
  strcat(buf, " '");
  strcat(buf, timeoutHoldBuf);
  strcat(buf, "';");
  
  cliRC = 
    cliInterface()->executeImmediate(buf);

  if (cliRC < 0)
    {
      // Ignore any error occurring from the executeImmediate call.
      // Rewind to not report this one error.
      getDiagsArea()->rewind(markValue);
      SQL_EXEC_ClearDiagnostics(NULL);
      return;
    }

  // Set the flag to indicate the timeout
  // should be restored

  restoreTimeout_ = TRUE;

  return;
}

void ExExeUtilTcb::restoreMaintainControlTableTimeout(char * catalog)
{
  Lng32 cliRC;
  char buf[400+ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES];
  Lng32 markValue = -1;

  // If the restoration timeout flag is not set,
  // then just return.

  if (!restoreTimeout_)
    return;

  markValue = getDiagsArea()->mark();
 
  // Reset the timeout to its previous setting.
  // Errors are ignored.

  strcpy(buf, "SET TABLE ");
  strcat(buf, catalog);
  strcat(buf, ".\"@MAINTAIN_SCHEMA@\".\"@MAINTAIN_CONTROL_INFO@\" ");
  strcat(buf, " TIMEOUT RESET;");

  cliRC = 
    cliInterface()->executeImmediate(buf);

  if (cliRC < 0)
    {
      // ignore any error occurring from the executeImmediate call
      // rewind to not report this one error
      getDiagsArea()->rewind(markValue);
      SQL_EXEC_ClearDiagnostics(NULL);
      return;
    }

  restoreTimeout_ = FALSE;

  return;
}

short ExExeUtilTcb::setSchemaVersion(char * param1)
{
  // For SeaQuest, the version number will always be the same
  // this is a no-op.
  return 0;
}

short ExExeUtilTcb::setSystemVersion()
{
  Lng32 cliRC = 0;

  if (sysVersionStrLen_ == 0)
    {
      // since SUBSTRING isn't currently supported for UTF-8 strings
      // and GET VERSION OF SYSTEM returns a UTF-8 string, convert it
      // to UCS2, do the substring, then convert to ISO88591 (we
      // expect a number here so it would be an error to get anything
      // other than ISO88591 characters).
      cliRC = 
	cliInterface()->executeImmediate("select translate(substring(translate(a using utf8toucs2), 10, 4) using ucs2toiso88591) from (get version of system) x(a)",
					 sysVersionStr_, &sysVersionStrLen_);
      if (cliRC < 0)
	{
          cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
	  return -1;
	}
      
      sysVersionStr_[sysVersionStrLen_] = 0;
    }

  return 0;
}

static const QueryString getObjectUidQuery[] =
{
  {" select cast(O.object_uid as char(24)) from "},
  {"   HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"   \"%s\".HP_DEFINITION_SCHEMA.objects O "},
  {" where S.schema_name = '%s' and "},
  {"       S.schema_uid = O.schema_uid and "},
  {"       O.object_name = '%s' and "},
  {"       O.object_name_space = '%s' and "},
  {"       O.object_type = '%s' "},
  {" for read uncommitted access "},
  {" ; "}
};

short ExExeUtilTcb::getObjectUid(char * catName, char * schName, 
				 char * objName, 
				 NABoolean isIndex, NABoolean isMv,
				 char* uid)
{
  Lng32 cliRC = 0;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  const QueryString * qs;
  Int32 sizeOfqs = 0;
  versionStrLen_ = 0;
  
  qs = getObjectUidQuery;
  sizeOfqs = sizeof(getObjectUidQuery);
  
  Int32 qryArraySize = sizeOfqs / sizeof(QueryString);
  char * gluedQuery;
  Lng32 gluedQuerySize;
  glueQueryFragments(qryArraySize,  qs,
		     gluedQuery, gluedQuerySize);
  
  Lng32 extraSpace = 10 /*segment name*/+ ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES/*cat/sch/obj name in UTF8*/ + 100;
  
  char * infoQuery =
    new(getHeap()) char[gluedQuerySize + extraSpace + 1];
  
  str_sprintf(infoQuery, gluedQuery,
	      catName, schName, objName, 
	      (isIndex ? "IX" : "TA"),
	      (isMv ? "MV" : (isIndex ? "IX" : "BT")));

  NADELETEBASIC(gluedQuery, getMyHeap());
  
  Lng32 uidLen;
  cliRC = 
    cliInterface()->executeImmediate(infoQuery, 
				     uid, &uidLen);
  if (cliRC < 0)
    {
      cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
      return -1;
    }
  uid[uidLen] = 0;

  return 0;
}

short ExExeUtilTcb::alterObjectState(NABoolean online,
				     char * tableName,
				     char * failReason,
				     NABoolean forPurgedata)
{
  char buf[4000];
  Lng32 cliRC = 0;

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  // make object online
  str_sprintf(buf, "ALTER TABLE %s %s %s",
	      tableName,
	      (online ? "ONLINE" : "OFFLINE"),
	      (forPurgedata ? "FOR PURGEDATA" : " "));
  
  // set sqlparserflags to allow 'FOR PURGEDATA' syntax
  masterGlob->getStatement()->getContext()->setSqlParserFlags(0x1);
  
  cliRC = cliInterface()->executeImmediate(buf);
  
  masterGlob->getStatement()->getContext()->resetSqlParserFlags(0x1);
  
  if (cliRC < 0)
    {
      str_sprintf(failReason, "Could not alter the state of table %s to %s.",
		  tableName, (online ? "online" : "offline"));

      return -1;
    }
  
  return 0;
}

short ExExeUtilTcb::lockUnlockObject(char * tableName,
				     NABoolean lock,
				     NABoolean parallel,
				     char * failReason)
{
  char buf[4000];
  Lng32 cliRC = 0;

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  // lock or unlock the table.
  if (parallel)
    {
      if (lock)
	str_sprintf(buf, "LOCK TABLE %s IN PROTECTED MODE NO INDEX LOCK PARALLEL EXECUTION ON",
		    tableName);
      else
	str_sprintf(buf, "UNLOCK TABLE %s PARALLEL EXECUTION ON",
		    tableName);
    }
  else
    {
      if (lock)
	str_sprintf(buf, "LOCK TABLE %s IN PROTECTED MODE",
		    tableName);
      else
	str_sprintf(buf, "UNLOCK TABLE %s",
		    tableName);
    }
  masterGlob->getStatement()->getContext()->setSqlParserFlags(0x100001);
  
  cliRC = cliInterface()->executeImmediate(buf);
  
  masterGlob->getStatement()->getContext()->resetSqlParserFlags(0x100001);
  
  if (cliRC < 0)
    {
      if (lock)
	str_sprintf(failReason, "Could not lock table %s in protected mode using parallel execution.",
		    tableName);
      else
	str_sprintf(failReason, "Could not unlock table %s using parallel execution.",
		    tableName);

      return -1;
    }
  
  return 0;
}

short ExExeUtilTcb::doubleQuoteStr(char * str, char * newStr, 
				   NABoolean singleQuote)
{
  // replace single quotes with 2 single quotes
  unsigned short i = 0;
  unsigned short j = 0;
  short len = (short)(str ? strlen(str) : 0);
  while (i < len)
    {
      if (singleQuote)
	{
	  if (str[i] == '\'')
	    {
	      newStr[j] = '\'';
	      j++;
	      newStr[j] = '\'';
	    }
	  else
	    newStr[j] = str[i];
	}
      else
	{
	  if (str[i] == '\"')
	    {
	      newStr[j] = '\"';
	      j++;
	      newStr[j] = '\"';
	    }
	  else
	    newStr[j] = str[i];
	}

      i++;
      j++;
    }
  newStr[j] = 0;
  
  return 0;
}

// lockType: COM_UTIL_PURGEDATA (= 9), COM_UTIL_REPLICATE (= 19).
//          (definition in common/ComSmallDefs.h).
short ExExeUtilTcb::alterDDLLock(NABoolean add, char * tableName,
				 char * failReason, NABoolean isMV,
				 Int32 lockType,
				 const char * lockSuffix,
				 NABoolean skipDDLLockCheck)
{
  char buf[4000];
  Lng32 cliRC = 0;

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  AnsiName aonn(tableName);
  aonn.convertAnsiName(FALSE);
  char * parts[4];
  Lng32 numParts;
  aonn.extractParts(numParts, parts);
  
  char * quotedParts0 = NULL;
  char * quotedParts1 = NULL;
  
  if (numParts == 3)
    {
      quotedParts0 = 
	new(getGlobals()->getDefaultHeap()) char[strlen(parts[0]) * 2 + 1];
      quotedParts1 = 
	new(getGlobals()->getDefaultHeap()) char[strlen(parts[1]) * 2 + 1];

      doubleQuoteStr(parts[0], quotedParts0, FALSE);
      doubleQuoteStr(parts[1], quotedParts1, FALSE);
    }

  ////////////////////////////////////////////////////////////////
  // see sqlshare/catapirequest.h for details on CAT API params.
  // CatApi has the form:
  //  CREATE TANDEM_CAT_REQUEST&1 <request-type> <num-params> 
  //     <lock-name> <object-name> <object-type> <operation-type>
  //  request-type:   1   (create lock)    or 2 (drop lock)
  //  num-params:     5
  //  lock-name:      getTableName() appended with _DDL_LOCK
  //  object-name:    getTableName()
  //  object-type:    0 (table)   or  2 (MV)
  //  operation-type: 9 (purgedata)
  //  lockStatus:     9 (parallel purgedata)
  ////////////////////////////////////////////////////////////////
  
  char sdlc[200];
  if (skipDDLLockCheck)
    {
      str_sprintf(sdlc, "<> <0> <1>");
    }

  // alter(add or drop) DDL lock
  if (numParts == 3)
  {
    char lockNameSuffix[200];
    str_sprintf(lockNameSuffix, "_DDL_LOCK%s", (lockSuffix ? lockSuffix : ""));
    generateLockName(parts[2], lockNameSuffix, buf, sizeof buf - 20);
    char quotedLockName[ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+200]; // a big buffer
    doubleQuoteStr(buf, quotedLockName, FALSE);
    str_sprintf(buf, "CREATE TANDEM_CAT_REQUEST&1 %s %d <\"%s\".\"%s\".\"%s\"> <%s%s> <%s> <%d> %s %s",
		(add ? "1" : "2"),
		(skipDDLLockCheck ? 8 : 5), //(lockType == COM_UTIL_PURGEDATA ? 5 : 4),
		quotedParts0, quotedParts1, quotedLockName,
		tableName, "", 
		(isMV ? "2" : "0"),
		lockType, 
		(lockType == COM_UTIL_PURGEDATA ? "<9>" : "<0>"),
		skipDDLLockCheck ? sdlc : "");
  }
  else
    str_sprintf(buf, "CREATE TANDEM_CAT_REQUEST&1 %s %d <%s_DDL_LOCK%s> <%s%s> <%s> <%d> %s %s",
		(add ? "1" : "2"),
		//		(lockType == COM_UTIL_PURGEDATA ? 5 : 4),
		(skipDDLLockCheck ? 8 : 5), //(lockType == COM_UTIL_PURGEDATA ? 5 : 4),
		tableName, 
		(lockSuffix ? lockSuffix : ""),
		tableName, "",
		(isMV ? "2" : "0"),
		lockType, 
		(lockType == COM_UTIL_PURGEDATA ? "<9>" : "<0>"),
		skipDDLLockCheck ? sdlc : "");
  
  // set sqlparserflags to allow CAT_API_REQUEST
  masterGlob->getStatement()->getContext()->setSqlParserFlags(0x1);
  
  cliRC = cliInterface()->executeImmediate(buf);
  
  masterGlob->getStatement()->getContext()->resetSqlParserFlags(0x1);
  
  NADELETEBASIC(quotedParts0, getGlobals()->getDefaultHeap());
  NADELETEBASIC(quotedParts1, getGlobals()->getDefaultHeap());

  if (cliRC < 0)
    {
      str_sprintf(failReason, "Could not %s ddl lock for object %s.",
		  (add ? "add" : "drop"), tableName);

      return (short)cliRC;
    }
  else
    return 0;
}

short ExExeUtilTcb::alterCorruptBit(short val, char * tableName,
				    char * failReason, Queue* indexList)
{
  char buf[4000];
  Lng32 cliRC = 0;

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  // change the corrupt bit in the label
  str_sprintf(buf, "LABEL_ALTER TABLE %s PARALLEL EXECUTION ON OPCODE 9 '%s'",
  	      tableName, (val == 1 ? "1" : "0"));
  
  // set sqlparserflags to allow 'label_alter' syntax
  masterGlob->getStatement()->getContext()->setSqlParserFlags(0x1);
  
  cliRC = cliInterface()->executeImmediate(buf);
  
  masterGlob->getStatement()->getContext()->resetSqlParserFlags(0x1);
  
  if (cliRC < 0)
    {
      str_sprintf(failReason, "Could not %s the corrupt bit on table %s.",
		  (val == 1 ? "set" : "reset"), tableName);
      return -1;
    }

  if (indexList)
    {
      indexList->position();
      
      while (NOT indexList->atEnd())
	{
	  char * indexName = (char*)indexList->getNext();
	  
	  str_sprintf(buf, "LABEL_ALTER INDEX_TABLE %s PARALLEL EXECUTION ON OPCODE 9 '%s'",
	  	      indexName, (val == 1 ? "1" : "0"));

	  // set sqlparserflags to allow 'label_alter' syntax
	  masterGlob->getStatement()->getContext()->setSqlParserFlags(0x1);
	  
	  cliRC = cliInterface()->executeImmediate(buf);
	  
	  masterGlob->getStatement()->getContext()->resetSqlParserFlags(0x1);
	  
	  if (cliRC < 0)
	    {
	      str_sprintf(failReason, "Could not %s the corrupt bit on index %s.",
			  (val == 1 ? "set" : "reset"), indexName);

	      return -1;
	    }
	} // while
      
    } // index present
  
  return 0;
}

short ExExeUtilTcb::alterAuditFlag(NABoolean audited, char * tableName,
				   NABoolean isIndex)
{
  char buf[4000];
  Lng32 cliRC = 0;

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  // change the corrupt bit in the label
  if (isIndex)
    str_sprintf(buf, "LABEL_ALTER INDEX_TABLE %s PARALLEL EXECUTION ON OPCODE %s ''",
		tableName, (audited ? "65" : "66"));
  else
    str_sprintf(buf, "LABEL_ALTER TABLE %s PARALLEL EXECUTION ON OPCODE %s ''",
  	      tableName, (audited ? "65" : "66"));
  
  // set sqlparserflags to allow 'label_alter' syntax
  masterGlob->getStatement()->getContext()->setSqlParserFlags(0x1);
  
  cliRC = cliInterface()->executeImmediate(buf);
  
  masterGlob->getStatement()->getContext()->resetSqlParserFlags(0x1);
  
  if (cliRC < 0)
    {
      return -1;
    }
  
  return 0;
}

short ExExeUtilTcb::handleError()
{
  short rc = ex_tcb::handleError(&qparent_, getDiagsArea());
  if (diagsArea_ != NULL)
     diagsArea_->deAllocate();
  diagsArea_ = NULL;
  return rc;
}

short ExExeUtilTcb::handleDone()
{
  short rc = ex_tcb::handleDone(&qparent_, getDiagsArea());
  if (diagsArea_ != NULL)
  { 
     diagsArea_->deAllocate();
     diagsArea_ = NULL;
  }
  return rc;
}
    
short ExExeUtilTcb::createServer(char *serverName,
				 const char * inPName,
				 IpcServerTypeEnum serverType,
				 IpcServerAllocationMethod servAllocMethod,
				 char *nodeName,
				 short cpu,
				 const char *partnName,
				 Lng32 priority,
				 IpcServer* &ipcServer,
				 NABoolean logError,
				 const char * operation)
{
  short error = 0;

 // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  IpcEnvironment * env = masterGlob->getCliGlobals()->getEnvironment(); 
  NAHeap *ipcHeap = masterGlob->getCliGlobals()->getIpcHeap();


  IpcServerClass * sc =
    new (ipcHeap) IpcServerClass(env, serverType,
				     servAllocMethod, //IPC_LAUNCH_GUARDIAN_PROCESS,
				     COM_VERS_MXV, nodeName);
  if (!sc)
    {
      if (logError)
        {
          char emsText[400+ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES];
          str_sprintf(emsText, "Failure creating IpcServerClass on \\%s cpu %d to %s %s for %s.", 
		      nodeName, cpu, operation, partnName, 
		      (char *)exeUtilTdb().getTableName());
          SQLMXLoggingArea::logExecRtInfo(NULL, 0, emsText, 0);
        }
      return -1;
    }

  const char * pName = NULL;
  char pNameBuf[20];
  short pNameLen = 0;

  if (inPName)
    pName = inPName;
  else
    {
      pName = pNameBuf;

      pNameBuf[pNameLen] = 0;
    }

  ipcServer =
    sc->allocateServerProcess(&diagsArea_,
			      ipcHeap,
			      nodeName,
			      cpu,
			      priority, 
			      1, // espLevel (not relevent)
			      FALSE, // no Xn
			      TRUE, // waited creation
			      0, // maxNowaitRequests
			      serverName,
			      pName
			      );
  if (!ipcServer)
    {
      if (logError && diagsArea_)
        {
          char emsText[400+ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES];
          str_sprintf(emsText, "allocateServerProcess() failed with error %d on \\%s cpu %d to %s %s for %s.", 
		      diagsArea_->mainSQLCODE(), nodeName, cpu, operation,
		      partnName, (char *)exeUtilTdb().getTableName());
          SQLMXLoggingArea::logExecRtInfo(NULL, 0, emsText, 0);
        }
      NADELETE(sc, IpcServerClass, ipcHeap);
      return -3;
    }

  return 0;
}

void ExExeUtilTcb::deleteServer(IpcServer *ipcServer)
{
  IpcServerClass * sc = ipcServer->getServerClass();
  IpcEnvironment *env = sc->getEnv();

  ipcServer->release();
  NADELETE(sc, IpcServerClass, env->getHeap());
}

NABoolean ExExeUtilTcb::isProcessObsolete(
     short cpu, pid_t pin, short segmentNum,
     Int64 procCreateTime)
{
  Lng32 retcode = 0;

  // see if process exists. If it exists, check if it is the same
  // process that is specified in the schemaName.
  short errorDetail = 0;
  Int64 l_procCreateTime = 0;
  retcode = ComRtGetProcessCreateTime(&cpu, &pin, &segmentNum,
				      l_procCreateTime,
				      errorDetail);
  if (retcode == XZFIL_ERR_OK)
  {
     // process specified exists.
     if (l_procCreateTime != procCreateTime)
	// but is a different process. Input process is obsolete.
        return -1;
     else
	// process is still alive.
        return 0;
  }
  else if (retcode == XZFIL_ERR_NOSUCHDEV)
     // process doesn't exist. process is obsolete.
     return -1;
  else
     // some other error while trying to access process.
     // process is not obsolete.
     return 0;
}

Lng32 ExExeUtilTcb::extractParts
(char * objectName,
 char ** paramParts0,
 char ** paramParts1,
 char ** paramParts2
 )
{

  char * parts[4];
  Lng32 numParts = 0;
  Lng32 rc = 0;

  // We want to ignore any "." dots within a delimited
  // name.  The AnsiName object is ultimately deleted
  // in the ExExeUtilMainObjectTcb destructor.
  
  if (extractedPartsObj_)
    delete extractedPartsObj_;

  extractedPartsObj_ = new (getHeap()) AnsiName(objectName);
  if ((rc = extractedPartsObj_->extractParts(numParts, parts)) != 0 ||
      (numParts != 3))
    {
      ExRaiseSqlError(getHeap(), &diagsArea_, -CLI_INTERNAL_ERROR);
      return -1;
    }


  char * parts0 = NULL;
  char * parts1 = NULL;
  char * parts2 = NULL;

  Lng32 parts0Len = strlen(parts[0]);
  Lng32 parts1Len = strlen(parts[1]);
  Lng32 parts2Len = strlen(parts[2]);

  Lng32 parts0OffsetLen = 0;
  Lng32 parts1OffsetLen = 0;
  Lng32 parts2OffsetLen = 0;

  Lng32 foundParts0 = 0;
  Lng32 foundParts1 = 0;
  Lng32 foundParts2 = 0;

  char * testParts = NULL;
  char * ptr = NULL;

  testParts = parts[0];

  ptr = (char *) strchr (testParts, '\'');
  while (ptr != NULL)
  {
    foundParts0++;
    ptr = (char *) strchr (ptr+1,'\'');
  }

  testParts = parts[1];
 
  ptr = (char *) strchr (testParts, '\'');
  while (ptr != NULL)
  {
    foundParts1++;
    ptr = (char *) strchr (ptr+1,'\'');
  }

  testParts = parts[2];
 
  ptr = (char *) strchr (testParts, '\'');
  while (ptr != NULL)
  {
    foundParts2++;
    ptr = (char *) strchr (ptr+1,'\'');
  }

  Lng32 lenToCopy = 0;
  char * beginTestParts = NULL;
  char * formattedParts = NULL;
  Lng32 totalLen = 0;

  if (foundParts0)
  {
    totalLen = parts0Len + foundParts0 + 1;
    parts0 = new(getHeap()) char[totalLen];

    for (Int32 i = 0; i < totalLen; i++)
      parts0[i] = ' ';

    parts0[totalLen-1] = '\0';

    testParts = parts[0];
    ptr = NULL;
  
    ptr = (char *) strchr (testParts, '\'');

    while (ptr != NULL)
    {
      lenToCopy = ptr - testParts;
    
      strncpy(parts0 + parts0OffsetLen,testParts,++lenToCopy);
       
      strncpy(parts0 + parts0OffsetLen + lenToCopy, "'",1);
         
      testParts = testParts + lenToCopy;
      parts0OffsetLen += lenToCopy;
      parts0OffsetLen++;

      ptr = (char *) strchr (ptr+1,'\'');
    }

    strncpy(parts0 + parts0OffsetLen, testParts,strlen(testParts));
    parts0[totalLen-1] = '\0';
  }
 else
  {
    totalLen = parts0Len + 1;
    parts0 = new(getHeap()) char[totalLen];
    strcpy(parts0, parts[0]);
  }

 if (foundParts1)
  {
    totalLen = parts1Len + foundParts1 + 1;
    parts1 = new(getHeap()) char[totalLen];

    for (Int32 i = 0; i < totalLen; i++)
      parts1[i] = ' ';

    parts1[totalLen-1] = '\0';

    testParts = parts[1];
    ptr = NULL;
  
    ptr = (char *) strchr (testParts, '\'');

    while (ptr != NULL)
    {
      lenToCopy = ptr - testParts;
    
      strncpy(parts1 + parts1OffsetLen,testParts,++lenToCopy);
       
      strncpy(parts1 + parts1OffsetLen + lenToCopy, "'",1);
         
      testParts = testParts + lenToCopy;
      parts1OffsetLen += lenToCopy;
      parts1OffsetLen++;

      ptr = (char *) strchr (ptr+1,'\'');
    }

    strncpy(parts1 + parts1OffsetLen, testParts,strlen(testParts));
    parts1[totalLen-1] = '\0';
  }
 else
  {
    totalLen = parts1Len + 1;
    parts1 = new(getHeap()) char[totalLen];
    strcpy(parts1, parts[1]);
  }

 if (foundParts2)
  {
    totalLen = parts2Len + foundParts2 + 1;
    parts2 = new(getHeap()) char[totalLen];

    for (Int32 i = 0; i < totalLen; i++)
      parts2[i] = ' ';

    parts2[totalLen-1] = '\0';

    testParts = parts[2];
    ptr = NULL;
  
    ptr = (char *) strchr (testParts, '\'');

    while (ptr != NULL)
    {
      lenToCopy = ptr - testParts;
 
      strncpy(parts2 + parts2OffsetLen,testParts,++lenToCopy);
      
      strncpy(parts2 + parts2OffsetLen + lenToCopy, "'",1);
         
      testParts = testParts + lenToCopy;
      parts2OffsetLen += lenToCopy;
      parts2OffsetLen++;

      ptr = (char *) strchr (ptr+1,'\'');
    }

    strncpy(parts2 + parts2OffsetLen, testParts,strlen(testParts));
    parts2[totalLen-1] = '\0';
  }
 else
  {
    totalLen = parts2Len + 1;
    parts2 = new(getHeap()) char[totalLen];
    strcpy(parts2, parts[2]);
  }

  /* The AnsiName() method strips out the leading
     and ending double quotes.  The following code
     is no longer needed.

  // 
  // Strip out the delimited name quotes.
  // If these are not stripped out, then a maximum
  // name of 128 characters will cause an overflow
  // by actually having 130 characters.

  char maxName[129];
  maxName[0] = '\0';

  if (parts0[0] == '"')
  {
    strncpy(maxName,parts0+1,strlen(parts0) -2);
    maxName[strlen(parts0)-2] = '\0';
    strncpy(parts0,maxName,strlen(parts0) -2);
    parts0[strlen(maxName)] = '\0';
  }

  maxName[0] = '\0';

  if (parts1[0] == '"')
  {
    strncpy(maxName,parts1+1,strlen(parts1) -2);
    maxName[strlen(parts1)-2] = '\0';
    strncpy(parts1,maxName,strlen(parts1) -2);
    parts1[strlen(maxName)] = '\0';
  }

  maxName[0] = '\0';

  if (parts2[0] == '"')
  {
    strncpy(maxName,parts2+1,strlen(parts2) -2);
    maxName[strlen(parts2)-2] = '\0';
    strncpy(parts2,maxName,strlen(parts2) -2);
    parts2[strlen(maxName)] = '\0';
  }

  maxName[0] = '\0';
*/

  *paramParts0 = parts0;
  *paramParts1 = parts1;
  *paramParts2 = parts2;

  return 0;
}

ex_expr::exp_return_type ExExeUtilTcb::evalScanExpr(char * ptr, Lng32 len,
                                                    NABoolean copyToVCbuf)
{
  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;

  if (exeUtilTdb().scanExpr_)
    {
      ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();

      char * exprPtr = ptr;
      if (copyToVCbuf)
        {
          exprPtr = new(getGlobals()->getDefaultHeap())
            char[SQL_VARCHAR_HDR_SIZE + len];
          short shortLen = (short)len;
          str_cpy_all((char*)exprPtr, (char*)&shortLen, SQL_VARCHAR_HDR_SIZE);
          str_cpy_all(&exprPtr[SQL_VARCHAR_HDR_SIZE], ptr, shortLen);
        }

      workAtp_->getTupp(exeUtilTdb().workAtpIndex())
	.setDataPointer(exprPtr);

      exprRetCode =
	exeUtilTdb().scanExpr_->eval(pentry_down->getAtp(), workAtp_);

      if (exprPtr != ptr)
        NADELETEBASIC(exprPtr, getGlobals()->getDefaultHeap());
    }

  return exprRetCode;
}


/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilPrivateState::ExExeUtilPrivateState(const ExExeUtilTcb * /*tcb*/)
{
  step_ = ExExeUtilTcb::EMPTY_;
  matches_ = 0;
}

ExExeUtilPrivateState::~ExExeUtilPrivateState()
{
};

ex_tcb_private_state * ExExeUtilPrivateState::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace()) ExExeUtilPrivateState((ExExeUtilTcb *) tcb);
};

