/*********************************************************************
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
 * File:         <file>
 * Description:  
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"

#include <ctype.h>
#include <string.h>

#include "ComCextdecs.h"

#include "NLSConversion.h"
#include "nawstring.h"
#include "exp_stdh.h"
#include "exp_clause_derived.h"
#include "exp_function.h"

#include "ExpLOB.h"
#include "ExpLOBinterface.h"
#include "ExpLOBexternal.h"
#include "ex_globals.h"
#include "ex_god.h"

ExLobGlobals *ExpLOBoper::initLOBglobal(NAHeap *parentHeap, ContextCli *currContext, NABoolean useLibHdfs, NABoolean isHiveRead)
{
  NAHeap *lobHeap = new (parentHeap) NAHeap("LOB Heap", parentHeap);
  ExLobGlobals *exLobGlobals = new (lobHeap) ExLobGlobals(lobHeap);
  exLobGlobals->setUseLibHdfs(useLibHdfs);
  exLobGlobals->initialize();
  // initialize lob interface
  ExpLOBoper::initLOBglobal(exLobGlobals, lobHeap, currContext, (char *)"default", (Int32)0, isHiveRead);
  return exLobGlobals; 
}


Lng32 ExpLOBoper::initLOBglobal(ExLobGlobals *& exLobGlobals, NAHeap *heap, ContextCli *currContext, char *hdfsServer ,Int32 port, NABoolean isHiveRead)
{
  // call ExeLOBinterface to initialize lob globals
  ExpLOBinterfaceInit(exLobGlobals, heap, currContext, isHiveRead, hdfsServer, port);
  return 0;
}

void ExpLOBoper::deleteLOBglobal(ExLobGlobals *exLobGlobals, NAHeap *heap)
{
  NAHeap *lobHeap = exLobGlobals->getHeap();
  NADELETE(exLobGlobals, ExLobGlobals, lobHeap);
  NADELETE(lobHeap, NAHeap, heap);
}

char * ExpLOBoper::ExpGetLOBname(Int64 uid, Lng32 num, char * outBuf, Lng32 outBufLen)
{
  if (outBufLen < 31)
    return NULL;

  str_sprintf(outBuf, "LOBP_%020ld_%04d",
	      uid, num);

  return outBuf;
}

char * ExpLOBoper::ExpGetLOBDescName(Lng32 schNameLen, char * schName,
				     Int64 uid, Lng32 num, 
				     char * outBuf, Lng32 outBufLen)
{
  if (outBufLen < 512)
    return NULL;

  str_sprintf(outBuf, "%s.\"LOBDsc_%020ld_%04d\"",
	      schName, uid, num);

  return outBuf;
}

char * ExpLOBoper::ExpGetLOBDescHandleObjNamePrefix(Int64 uid, 
					   char * outBuf, Lng32 outBufLen)
{
  if (outBufLen < 512)
    return NULL;
  
  str_sprintf(outBuf, "%s_%020ld", LOB_DESC_HANDLE_PREFIX,uid);
  
  return outBuf;
}

char * ExpLOBoper::ExpGetLOBDescHandleName(Lng32 schNameLen, char * schName,
					   Int64 uid, Lng32 num, 
					   char * outBuf, Lng32 outBufLen)
{
  if ((outBufLen < 512) ||
      (schName == NULL))
    return NULL;
  
  str_sprintf(outBuf, "%s.\"%s_%020ld_%04d\"",
	      schName, LOB_DESC_HANDLE_PREFIX,uid, num);
  
  return outBuf;
}

Lng32 ExpLOBoper::ExpGetLOBnumFromDescName(char * descName, Lng32 descNameLen)
{
  // Desc Name Format: LOBDescHandle_%020ld_%04d
  char * lobNumPtr = &descName[sizeof(LOB_DESC_HANDLE_PREFIX) + 20 + 1];
  Lng32 lobNum = str_atoi(lobNumPtr, 4);
  
  return lobNum;
}

char * ExpLOBoper::ExpGetLOBDescChunksName(Lng32 schNameLen, char * schName,
					   Int64 uid, Lng32 num, 
					   char * outBuf, Lng32 outBufLen)
{
  if ((outBufLen < 512) ||
      (schNameLen == 0) ||
      (schName == NULL))
    return NULL;
  
  str_sprintf(outBuf, "%s.\"%s_%020ld_%04d\"",
	      schName, LOB_DESC_CHUNK_PREFIX,uid, num);

  return outBuf;
}



char * ExpLOBoper::ExpGetLOBMDName(Lng32 schNameLen, char * schName,
				    Int64 uid,  
				    char * outBuf, Lng32 outBufLen)
{
  if (outBufLen < 512)
    return NULL;

  str_sprintf(outBuf, "%s.\"%s_%020ld\"",
	      schName, LOB_MD_PREFIX,uid);

  return outBuf;
}
Lng32 ExpLOBoper::createLOB(ExLobGlobals * exLobGlob, ContextCli *currContext, 
			    char * lobLoc,Int32 hdfsPort,char *hdfsServer,
			    Int64 uid, Lng32 num, Int64 lobMaxSize )
{
  char buf[LOB_NAME_LEN];
  
  char * lobName = ExpGetLOBname(uid, num, buf, LOB_NAME_LEN);
  if (lobName == NULL)
    return -1;

  Lng32 rc = 0;
  
  // Call ExeLOBinterface to create the LOB

  rc = ExpLOBinterfaceCreate(exLobGlob, lobName, lobLoc, Lob_HDFS_File,hdfsServer,lobMaxSize, hdfsPort);
  
  return rc;
}
void ExpLOBoper::calculateNewOffsets(ExLobInMemoryDescChunksEntry *dcArray, Lng32 numEntries)
{
  Int32 i = 0;
  //Check if there is a hole right up front for the first entry. If so start compacting with the first entry.
  if (dcArray[0].getCurrentOffset() != 0)
    {
      dcArray[0].setNewOffset(0);
      for (i = 1; i < numEntries; i++)
        {
          dcArray[i].setNewOffset(dcArray[i-1].getNewOffset() + dcArray[i-1].getChunkLen());
        }
    }
  else
    //Look for the first unused section and start compacting from there.
    {
      NABoolean done = FALSE;
      i = 0;
      Int32 j = 0;
      while (i < numEntries && !done )
        {
          if ((dcArray[i].getCurrentOffset()+dcArray[i].getChunkLen()) != 
              dcArray[i+1].getCurrentOffset())
            {
              j = i+1;
              while (j < numEntries)
                {
                   dcArray[j].setNewOffset(dcArray[j-1].getNewOffset()+dcArray[j-1].getChunkLen());
                   j++;
                }
              done = TRUE;
            }
          i++;
        }
    }
  return ;
}

Lng32 ExpLOBoper::compactLobDataFile(ExLobGlobals *exLobGlob,ExLobInMemoryDescChunksEntry *dcArray,Int32 numEntries,char *tgtLobName,Int64 lobMaxChunkMemSize, NAHeap *lobHeap,  ContextCli *currContext,char *hdfsServer, Int32 hdfsPort, char *lobLoc)
{
  Int32 rc = 0;
  ExLobGlobals * exLobGlobL = NULL;
  // Call ExeLOBinterface to create the LOB
  if (exLobGlob == NULL)
    {
      rc = initLOBglobal(exLobGlobL, lobHeap,currContext,hdfsServer,hdfsPort);
      if (rc)
	return rc;
    }
  else
    exLobGlobL = exLobGlob;
 
   
  rc = ExpLOBinterfacePerformGC(exLobGlobL,tgtLobName, (void *)dcArray, numEntries,hdfsServer,hdfsPort,lobLoc,lobMaxChunkMemSize);
  
  if (exLobGlob == NULL)
     ExpLOBinterfaceCleanup(exLobGlobL);
  return rc;
}

Int32 ExpLOBoper::restoreLobDataFile(ExLobGlobals *exLobGlob, char *lobName, NAHeap *lobHeap, ContextCli *currContext,char *hdfsServer, Int32 hdfsPort, char *lobLoc)
{
  Int32 rc = 0;
  ExLobGlobals * exLobGlobL = NULL;
   if (exLobGlob == NULL)
    {
      rc = initLOBglobal(exLobGlobL, lobHeap,currContext, hdfsServer,hdfsPort);
      if (rc)
	return rc;
    }
  else
    exLobGlobL = exLobGlob;
  rc = ExpLOBinterfaceRestoreLobDataFile(exLobGlobL,hdfsServer,hdfsPort,lobLoc,lobName);
  if (exLobGlob == NULL)
     ExpLOBinterfaceCleanup(exLobGlobL);
  return rc;

}

Int32 ExpLOBoper::purgeBackupLobDataFile(ExLobGlobals *exLobGlob,char *lobName, NAHeap *lobHeap, ContextCli *currContext, char * hdfsServer, Int32 hdfsPort, char *lobLoc)
{
  Int32 rc = 0;
  ExLobGlobals * exLobGlobL = NULL;
  if (exLobGlob == NULL)
    {
      rc = initLOBglobal(exLobGlobL, lobHeap,currContext,hdfsServer,hdfsPort);
      if (rc)
	return rc;
    }
  else
    exLobGlobL = exLobGlob;
  rc = ExpLOBinterfacePurgeBackupLobDataFile(exLobGlobL,(char *)hdfsServer,hdfsPort,lobLoc,lobName);
  if (exLobGlob == NULL)
     ExpLOBinterfaceCleanup(exLobGlobL);
  return rc;
}


Lng32 ExpLOBoper::dropLOB(ExLobGlobals * exLobGlob, ContextCli *currContext,
			  char * lobLoc,Int32 hdfsPort, char *hdfsServer,
			  Int64 uid, Lng32 num)
{
  char buf[LOB_NAME_LEN];
  
  char * lobName = ExpGetLOBname(uid, num, buf, LOB_NAME_LEN);
  if (lobName == NULL)
    return -1;

  Lng32 rc = 0;
 
 
  // Call ExeLOBinterface to drop the LOB
  rc = ExpLOBinterfaceDrop(exLobGlob,hdfsServer, hdfsPort, lobName, lobLoc);
  
  return rc;
}

Lng32 ExpLOBoper::purgedataLOB(ExLobGlobals * exLobGlob, char * lobLoc, 
                               
			       Int64 uid, Lng32 num)
{
  char buf[LOB_NAME_LEN];
  
  char * lobName = ExpGetLOBname(uid, num, buf, LOB_NAME_LEN);
  if (lobName == NULL)
    return -1;

  // Call ExeLOBinterface to purgedata the LOB
  Lng32 rc = ExpLOBInterfacePurgedata(exLobGlob, lobName, lobLoc);
  if (rc < 0)
    return ex_expr::EXPR_ERROR;

  return ex_expr::EXPR_OK;
}

ExpLOBoper::ExpLOBoper(){};
ExpLOBoper::ExpLOBoper(OperatorTypeEnum oper_type,
		       short num_operands,
		       Attributes ** attr,
		       Space * space)
  : ex_clause(ex_clause::LOB_TYPE, oper_type, num_operands, attr, space),
    flags_(0),
    lobNum_(-1),
    lobHandleLenSaved_(0),
    lobStorageType_((short)Lob_Invalid_Storage),
    requestTag_(-1),
    descSchNameLen_(0)
{
  lobHandleSaved_[0] = 0;
  lobStorageLocation_[0] = 0;
  lobHdfsServer_[0] = 0;
  strcpy(lobHdfsServer_,"");
  lobHdfsPort_ = -1;
  descSchName_[0] = 0;
  lobSize_ = 0;
  lobMaxSize_ = 0;
  lobMaxChunkMemSize_ = 0;
  lobGCLimit_ = 0;
};


void ExpLOBoper::displayContents(Space * space, const char * displayStr, 
				 Int32 clauseNum, char * constsArea)
{
  ex_clause::displayContents(space, displayStr, clauseNum, constsArea);

  char buf[100];
  str_sprintf(buf, "    lobNum_ = %d, lobStorageType_ = %d",
	      lobNum_, lobStorageType_);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

  Lng32 len = MINOF(strlen(lobStorageLocation_), 50);
  char loc[100];
  str_cpy_all(loc, lobStorageLocation_, len);
  loc[len] = 0;
  str_sprintf(buf, "    lobStorageLocation_ = %s\n",
	      loc);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
}

Lng32 findNumDigits(Int64 val)
{
  Int64 v = val/10;
  Lng32 n = 1;
  while (v > 0)
    {
      n++;
      
      v = v / 10;


    }

  return n;
}

// Generates LOB handle that is stored in the SQL row.
// LOB handle len:  64 bytes
// <flags><LOBnum><objectUid><descKey><descTS><schNameLen><schName>
// <--4--><--4---><----8----><---8---><--8---><-----2----><--vc--->
void ExpLOBoper::genLOBhandle(Int64 uid, 
			      Lng32 lobNum,
			      Int32 lobType,
			      Int64 descKey, 
			      Int64 descTS,
			      Lng32 flags,
			      short schNameLen,
			      char  * schName,
			      Lng32 &handleLen,
			      char * ptr)
{
  LOBHandle * lobHandle = (LOBHandle*)ptr;
  lobHandle->flags_ = flags;
  lobHandle->lobType_ = lobType;
  lobHandle->lobNum_ = lobNum;
  lobHandle->objUID_ = uid;
  lobHandle->descSyskey_ = descKey;
  lobHandle->descPartnkey_ = descTS;
  lobHandle->schNameLen_ = schNameLen;
  handleLen = sizeof(LOBHandle);
  if (schNameLen > 0)
    {
      char * s = &lobHandle->schName_;
      str_cpy_all(s, schName, schNameLen);
      s[schNameLen] = 0;

      handleLen += schNameLen;
    }
  //  lobHandle->inlinedLOBlen_ = 0;
}

void ExpLOBoper::updLOBhandle(Int64 descSyskey, 
			      Lng32 flags,                            
			      char * ptr)
{
  LOBHandle * lobHandle = (LOBHandle*)ptr;
  lobHandle->flags_ = flags;  
  lobHandle->descSyskey_ = descSyskey;
}

// Extracts values from the LOB handle stored at ptr
Lng32 ExpLOBoper::extractFromLOBhandle(Int16 *flags,
				       Lng32 *lobType,
				       Lng32 *lobNum,
				       Int64 *uid, 
				       Int64 *descSyskey, 
				       Int64 *descPartnKey,
				       short *schNameLen,
				       char * schName,
				       char * ptrToLobHandle,
				       Lng32 handleLen)
{
  LOBHandle * lobHandle = (LOBHandle*)ptrToLobHandle;
  if (flags)
    *flags = lobHandle->flags_;
  if (lobType)
    *lobType = lobHandle->lobType_;
  if (lobNum)
    *lobNum = lobHandle->lobNum_;
  if (uid)
    *uid = lobHandle->objUID_;
  if (descSyskey)
    *descSyskey = lobHandle->descSyskey_;
  if (descPartnKey)
    *descPartnKey = lobHandle->descPartnkey_;
  if (schNameLen)
    *schNameLen = lobHandle->schNameLen_;
  if ((schNameLen) && (*schNameLen > 0) && (schName != NULL))
    {
      str_cpy_all(schName, &lobHandle->schName_, *schNameLen);
      schName[*schNameLen] = 0;
    }

  return 0;
}
// 12 byte lock identifier uniquely identifies the LOB file that is being 
// locked.
// <object UID + lob number > Each LOB column has a unique lob number and 
// each column has a unique data file.
void ExpLOBoper::genLobLockId(Int64 objid, Int32 lobNum, char *llid)
{
  memset(llid,'\0',LOB_LOCK_ID_SIZE);
  if (objid != -1 && lobNum != -1)
    {
      memcpy(llid,&objid,sizeof(Int64)) ;
      memcpy(&(llid[sizeof(Int64)]),&lobNum,sizeof(Int32));
    }    
}

// creates LOB handle in string format.
void ExpLOBoper::createLOBhandleString(Int16 flags,
				       Lng32 lobType,
				       Int64 uid, 
				       Lng32 lobNum,
				       Int64 descKey, 
				       Int64 descTS,
				       short schNameLen,
				       char * schName,
				       char * lobHandleBuf)
{
  str_sprintf(lobHandleBuf, "LOBH%04d%04d%04d%020ld%02d%ld%02d%ld%03d%s",
	      flags, lobType, lobNum, uid,
	      findNumDigits(descKey), descKey, 
	      findNumDigits(descTS), descTS,
	      schNameLen, schName);

 
}

// Extracts values from the string format of LOB handle 
Lng32 ExpLOBoper::extractFromLOBstring(Int64 &uid, 
				       Lng32 &lobNum,
				       Int64 &descPartnKey,
				       Int64 &descSyskey,
				       Int16 &flags,
				       Lng32 &lobType,
				       short &schNameLen,
				       char * schName,
				       char * handle,
				       Lng32 handleLen)
{
  // opp of:
  //  str_sprintf(lobHandleBuf, "LOBH%04d%04d%04d%020ld%02d%ld%02d%ld%03d%s",
  //	      flags, lobType, lobNum, uid,
  //	      findNumDigits(descKey), descKey, 
  //	      findNumDigits(descTS), descTS,
  //	      schNameLen, schName)


  if (handleLen < (4 + 4 + 4  + 20 + 2)) // Minimum sanity check.
    return -1;

  Lng32 curPos = 4;
  
  flags = (Lng32)str_atoi(&handle[curPos], 4);
  curPos += 4;

  lobType = (Lng32)str_atoi(&handle[curPos], 4);
  curPos += 4;

  lobNum = (Lng32)str_atoi(&handle[curPos], 4);
  curPos += 4;

  uid = str_atoi(&handle[curPos], 20);
  curPos += 20;

  short len1;
  len1 = (short)str_atoi(&handle[curPos], 2);
  curPos += 2;

  if (handleLen < (curPos + len1 + 2))
    return -1;

  descPartnKey = str_atoi(&handle[curPos], len1);
  curPos += len1;

  short len2;
  len2 = (short)str_atoi(&handle[curPos], 2);
  curPos += 2;

  if (handleLen < (curPos + len2 + 3))
    return -1;
  descSyskey = str_atoi(&handle[curPos], len2);
  curPos += len2;

  schNameLen = (short)str_atoi(&handle[curPos], 3);
  curPos += 3;
  if (handleLen < (curPos + schNameLen))
    return -1;
  
  str_cpy_and_null(schName, &handle[curPos], 
		   schNameLen, '\0', ' ', TRUE);

  //  flags = 0;

  return 0;
}

Lng32 ExpLOBoper::genLOBhandleFromHandleString(char * lobHandleString,
					       Lng32 lobHandleStringLen,
					       char * lobHandle,
					       Lng32 &lobHandleLen)
{
  
  Int64 uid;
  Lng32 lobNum;
  Int32 lobType;
  Int64 descPartnKey;
  Int64 descSyskey;
  Int16 flags;
  short schNameLen;
  char  schNameLenBuf[1024];
  Lng32 handleLen;

  if (extractFromLOBstring(uid, lobNum, descPartnKey, descSyskey, flags,
			   lobType,schNameLen, schNameLenBuf, 
			   lobHandleString, lobHandleStringLen))
    return -1;

  
  char lLobHandle[4096];
  genLOBhandle(uid, lobNum, lobType, descPartnKey, descSyskey, flags,
	       schNameLen, schNameLenBuf, handleLen, lLobHandle);

  if (handleLen > lobHandleLen)
    {
      return -1;
    }

  str_cpy_all(lobHandle, lLobHandle, handleLen);
  lobHandleLen = handleLen;

  return 0;
}

Long ExpLOBoper::pack(void * space)
{
  //  if (lobStorageLocation_)
  //    lobStorageLocation_.pack(space);

  return packClause(space, sizeof(ExpLOBoper));
}

Lng32 ExpLOBoper::unpack (void * base, void * reallocator)
{
  //  if (lobStorageLocation_.unpack(base))
  //    return -1;

  return unpackClause(base, reallocator);
}

Lng32 ExpLOBoper::initClause()
{
  requestTag_ = -1;

  return 0;
}

///////////////////////////////////////////////////////////////////////
//
///////////////////////////////////////////////////////////////////////

Lng32 ExpLOBoper::checkLobOperStatus()
{
  Lng32 lobOperStatus = 0;

  if (requestTag_ == -1) // this clause has not been evaluated yet
    lobOperStatus = START_LOB_OPER_;   // start lob process
  else if (requestTag_ == -2) // this request has completed
    lobOperStatus = DO_NOTHING_; // do nothing
  else // a valid request tag
    lobOperStatus = CHECK_STATUS_; // check status for this clause
  
  return lobOperStatus;
}

////////////////////////////////////////////////////////
// ExpLOBiud
////////////////////////////////////////////////////////
ExpLOBiud::ExpLOBiud(){};
ExpLOBiud::ExpLOBiud(OperatorTypeEnum oper_type,
		     Lng32 numAttrs,
		     Attributes ** attr, 
		     Int64 objectUID,
		     short descSchNameLen,
		     char * descSchName,
		     Space * space)
  : ExpLOBoper(oper_type, numAttrs, attr, space),
    objectUID_(objectUID),
    liudFlags_(0)
   
    //    descSchNameLen_(descSchNameLen),
    //    liFlags_(0)
{
  str_cpy_and_null(descSchName_, descSchName, descSchNameLen, 
		   '\0', ' ', TRUE);

  setDescSchNameLen(descSchNameLen);
};

////////////////////////////////////////////////////////
// ExpLOBinsert
////////////////////////////////////////////////////////
ExpLOBinsert::ExpLOBinsert(){};
ExpLOBinsert::ExpLOBinsert(OperatorTypeEnum oper_type,
			   Lng32 numAttrs,
			   Attributes ** attr,			   
			   Int64 objectUID,
			   short descSchNameLen,
			   char * descSchName,
			   Space * space)
  : ExpLOBiud(oper_type, numAttrs, attr, objectUID, descSchNameLen, descSchName, space),
    //    objectUID_(objectUID),
    //    descSchNameLen_(descSchNameLen),
    liFlags_(0)
{
};

void ExpLOBinsert::displayContents(Space * space, const char * displayStr, 
				   Int32 clauseNum, char * constsArea)

{
  ex_clause::displayContents(space, "ExpLOBinsert", clauseNum, constsArea);

  char buf[100];

  str_sprintf(buf, "    liFlags_ = %d", liFlags_);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
}




ex_expr::exp_return_type ExpLOBiud::insertDesc(char *op_data[],
                                                char *lobData,
                                                Int64 lobLen,
                                                Int64 lobHdfsDataOffset,
                                                char *&result,
                                                CollHeap*h,
                                                ComDiagsArea** diagsArea)
{
  Lng32 rc; 
  Lng32 handleLen = 0;
  char lobHandleBuf[LOB_HANDLE_LEN];
  Int64 descTS = NA_JulianTimestamp();
  char * lobHandle = NULL; 
  lobHandle = lobHandleBuf;
  ExpLOBoper::genLOBhandle(objectUID_, lobNum(), (short)lobStorageType(),
                           -1, descTS, -1,
                           descSchNameLen_, descSchName(),
                           handleLen, lobHandle);
 

  // get the lob name where data need to be inserted
  if (lobNum() == -1)
    {
      Int32 intparam = LOB_PTR_ERROR;
      Int32 detailError = 0;
     
      ExRaiseSqlError(h, diagsArea, 
		      (ExeErrorCode)(8434));
      return ex_expr::EXPR_ERROR;
    }
  char tgtLobNameBuf[LOB_NAME_LEN];
  char * tgtLobName = ExpGetLOBname(objectUID_, lobNum(), tgtLobNameBuf, LOB_NAME_LEN);

  if (tgtLobName == NULL)
    return ex_expr::EXPR_ERROR;

  // call function with the lobname and source value
  // to insert it in the LOB.
  // Get back offset and len of the LOB.
  Int64 descSyskey = 0;

  
  LobsSubOper so = Lob_None;
  if (fromFile())
    so = Lob_File;
  else if (fromString() || fromLoad())
    so = Lob_Memory;
  else if (fromLob())
    so = Lob_Lob;
  else if (fromLobExternal())
    so = Lob_External_Lob;
  else if (fromBuffer())
    so = Lob_Buffer;
  else if (fromExternal())
    so = Lob_External_File;
  else if (fromEmpty())
    {
      so = Lob_Memory;
    }

  

  Lng32 waitedOp = 0;
  waitedOp = 1;

  
  // until SQL_EXEC_LOBcliInterface is changed to allow for unlimited
  // black box sizes, we have to prevent over-sized file names from
  // being stored
  if ((so == Lob_External_File) && (lobLen > MAX_LOB_FILE_NAME_LEN))
    {
      ExRaiseSqlError(h, diagsArea, 
		      (ExeErrorCode)(8557));
      return ex_expr::EXPR_ERROR;
    }
  
  blackBoxLen_ = 0;
  if (fromExternal())
    {
      blackBoxLen_ = lobLen;
      str_cpy_and_null(blackBox_, lobData, (Lng32)blackBoxLen_,
		       '\0', ' ', TRUE);
    }

  Lng32 cliError = 0;

  
 
  LobsOper lo ;
 
 if (lobHandle == NULL)       
    lo = Lob_InsertDataSimple;
  else
    lo = Lob_InsertDesc;
  Int64 lobMaxSize = 0;
  if (getLobSize() > 0)
    {
      lobMaxSize = MINOF(getLobSize(), getLobMaxSize());
    }
  else
    lobMaxSize = getLobMaxSize();
    
  if ((so == Lob_Lob) || (so == Lob_External_Lob))
    {
      
      rc = ExpLOBInterfaceInsertSelect
        (getExeGlobals()->getExLobGlobal(), 
         (getTcb()->getStatsEntry() != NULL ? getTcb()->getStatsEntry()->castToExHdfsScanStats() : NULL),
         getLobHdfsServer(), getLobHdfsPort(),
         tgtLobName, 
         so,
         lobStorageLocation(),lobStorageType(),
         -1,
         handleLen, lobHandle,  &outHandleLen_, outLobHandle_,            
         lobData, lobLen, blackBox_, blackBoxLen_,lobMaxSize, getLobMaxChunkMemSize(),getLobGCLimit()); 
        
    }
else
  rc = ExpLOBInterfaceInsert
    (getExeGlobals()->getExLobGlobal(), 
     (getTcb()->getStatsEntry() != NULL ? getTcb()->getStatsEntry()->castToExHdfsScanStats() : NULL),
     tgtLobName, 
     lobStorageLocation(),
     lobStorageType(),
     getLobHdfsServer(), getLobHdfsPort(),

     handleLen, lobHandle,
     &outHandleLen_, outLobHandle_,
     blackBoxLen_, blackBox_,
     lobHdfsDataOffset,
     -1,
     descSyskey,
     lo,
     &cliError,
     so,
     waitedOp,
     lobData, lobLen, lobMaxSize, getLobMaxChunkMemSize(),getLobGCLimit());
  
  if (rc == LOB_ACCESS_PREEMPT)
    {
      // save the handle so it could be used after return from preempt
      lobHandleLenSaved_ = handleLen;
      str_cpy_all(lobHandleSaved_, lobHandle, handleLen);
      
      return ex_expr::EXPR_PREEMPT;
    }

  if (rc < 0)
    {
      Lng32 intParam1 = -rc;
      ExRaiseSqlError(h, diagsArea, 
		      (ExeErrorCode)(8442), NULL, &intParam1, 
		      &cliError, NULL, (char*)"ExpLOBInterfaceInsert",
		      getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
      return ex_expr::EXPR_ERROR;
    }

  // extract and update lob handle with the returned values
  if (outHandleLen_ > 0)
    {
      Int32 lobType = 0;
      ExpLOBoper::extractFromLOBhandle(NULL, &lobType, NULL, NULL, &descSyskey,
				       NULL, NULL, NULL, outLobHandle_);
      
      ExpLOBoper::updLOBhandle(descSyskey, 0, lobHandle); 
    }

  str_cpy_all(result, lobHandle, handleLen);
  getOperand(0)->setVarLength(handleLen, op_data[-MAX_OPERANDS]);
 

  return ex_expr::EXPR_OK;
}



ex_expr::exp_return_type ExpLOBiud::insertData(Lng32 handleLen,
                                                char *&handle,
                                                char *lobData,
                                                Int64 lobLen,
                                                Int64 &hdfsDataOffset,
                                                CollHeap*h,
                                                ComDiagsArea** diagsArea)
{
  Lng32 rc = 0;

  Lng32 lobType;
  Int64 uid;
  Lng32 lobColNum;
  
 
  Int64 descSyskey = -1;
 
  short numChunks = 0;
 
  
   // get the lob name where data need to be inserted
  char tgtLobNameBuf[LOB_NAME_LEN];
  char * tgtLobName = ExpGetLOBname(objectUID_, lobNum(), tgtLobNameBuf, LOB_NAME_LEN);

  if (tgtLobName == NULL)
    return ex_expr::EXPR_ERROR;

  
  if (fromExternal() || fromLob() || fromLobExternal())
    {
      //no need to insert any data. All data resides in the external file
      // for external LOB and it has already been read/inserted during 
      // ::insertDesc for insert from another  LOB
      return ex_expr::EXPR_OK;
    }
 
  LobsOper lo ;
 
  if (handle == NULL)       
    lo = Lob_InsertDataSimple;
  else
    lo = Lob_InsertData;

  LobsSubOper so = Lob_None;
  if (fromFile())    
    so = Lob_File;       
  else if (fromString() || fromLoad())
    so = Lob_Memory;
  else if (fromLob())
    so = Lob_Lob;
  else if(fromBuffer())
    so = Lob_Buffer;
  else if (fromExternal())
    so = Lob_External_File;

 
  Lng32 waitedOp = 0;
  waitedOp = 1;

  Lng32 cliError = 0;

  blackBoxLen_ = 0;

  
      rc = ExpLOBInterfaceInsert(getExeGlobals()->getExLobGlobal(),
                                 (getTcb()->getStatsEntry() != NULL ? getTcb()->getStatsEntry()->castToExHdfsScanStats() : NULL),
				 tgtLobName, 
				 lobStorageLocation(),
				 lobType,
				 getLobHdfsServer(), getLobHdfsPort(),

				 handleLen, handle,
				 &outHandleLen_, outLobHandle_,

				 blackBoxLen_, blackBox_,

				 hdfsDataOffset,
				 -1,
				 
				 descSyskey, 
				 lo,
				 &cliError,
				 so,
				 waitedOp,				 
				 lobData,
				 lobLen,getLobMaxSize(),
                                 getLobMaxChunkMemSize(),
                                 getLobGCLimit());
    

  if (rc == LOB_ACCESS_PREEMPT)
    {
      return ex_expr::EXPR_PREEMPT;
    }

  if (rc < 0)
    {
      Lng32 intParam1 = -rc;
      ExRaiseSqlError(h, diagsArea, 
		      (ExeErrorCode)(8442), NULL, &intParam1, 
		      &cliError, NULL, (char*)"ExpLOBInterfaceInsert",
		      getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
      return ex_expr::EXPR_ERROR;
    }

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExpLOBinsert::eval(char *op_data[],
					    CollHeap*h,
					    ComDiagsArea** diagsArea)
{
  char timeBuf[1024] = "";
  ex_expr::exp_return_type err;
  Int32 retcode = 0;
  Int32 cliError = 0;
  Int64 lobHdfsOffset = 0;  
  char * result = op_data[0];
  Int64 lobLen = 0; 
  char *lobData = NULL;
  Int64 chunkMemSize = 0;
  char *inputAddr = NULL;
  Int64 sourceFileSize = 0; // If the input is from a file get total file size
  Int64 sourceFileReadOffset = 0; 
  char *retBuf = 0;
  Int64 retReadLen = 0;
  Lng32 sLobType;
  Int64 sUid;
  Lng32 sLobNum;
  Int64 sDescSyskey = -1;
  Int64 sDescTS = -1;
  Int16 sFlags;
  short sSchNameLen = 0;
  char sSchName[500];
  char tgtLobNameBuf[LOB_NAME_LEN];
  char *tgtLobName = NULL;
  Int32 handleLen = 0;

  if (lobNum() == -1)
    {
      Int32 intparam = LOB_PTR_ERROR;
      Int32 detailError = 0;
     
      ExRaiseSqlError(h, diagsArea, 
		      (ExeErrorCode)(8434));
      return ex_expr::EXPR_ERROR;
    }
     
  char llid[LOB_LOCK_ID_SIZE];
  if (lobLocking())
    {
      ExpLOBoper::genLobLockId(objectUID_,lobNum(),llid);;
      NABoolean found = FALSE;
      retcode = SQL_EXEC_CheckLobLock(llid, &found);
      if (! retcode && !found) 
        {    
          retcode = SQL_EXEC_SetLobLock(llid);
        }
      else if (found)
        {
          Int32 lobError = LOB_DATA_FILE_LOCK_ERROR;
          ExRaiseSqlError(h, diagsArea, 
                          (ExeErrorCode)(8558), NULL,(Int32 *)&lobError, 
                          NULL, NULL, (char*)"ExpLOBInterfaceInsert",
                          getLobErrStr(LOB_DATA_FILE_LOCK_ERROR),NULL);
          return ex_expr::EXPR_ERROR;
        }
    }
  if (!fromEmpty())
    lobLen = getOperand(1)->getLength();
                 
  if(fromString())
    {
      //If source is a varchar, find the actual length
      if (getOperand(1)->getVCIndicatorLength() >0)   
        lobLen = getOperand(1)->getLength(op_data[1]- getOperand(1)->getVCIndicatorLength());  
    } 
 
  if(fromFile())
    {
      lobData = new (h) char[lobLen];  
      str_cpy_and_null(lobData,op_data[1],lobLen,'\0',' ',TRUE);
      retcode = ExpLOBInterfaceGetFileSize(getExeGlobals()->getExLobGlobal(), 
                                           lobData,//filename 
                                           getLobHdfsServer(),
                                           getLobHdfsPort(),
                                           sourceFileSize);
      if (retcode < 0)
        {
          if (lobLocking())
            retcode = SQL_EXEC_ReleaseLobLock(llid);
          Lng32 intParam1 = -retcode;
          ExRaiseSqlError(h, diagsArea, 
                          (ExeErrorCode)(8442), NULL, &intParam1, 
                          &cliError, NULL, (char*)"ExpLOBInterfaceGetFileSize",
                          getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
          return ex_expr::EXPR_ERROR;
        }
      lobLen = sourceFileSize;
    }
    else
      lobData = op_data[1];
  long inputSize = lobLen;
  if (fromBuffer())
    {
      memcpy(&lobLen, op_data[2],sizeof(Int64)); // user specified buffer length
      Int64 userBufAddr = 0;
      memcpy(&userBufAddr,op_data[1],sizeof(Int64));
      lobData = (char *)userBufAddr;
    }
 
  inputAddr = lobData ;
 
  chunkMemSize = MINOF(getLobMaxChunkMemSize(), inputSize);
  if(!fromEmpty())
    {
     
      if (fromFile())
        {
          //read a chunk of the file input data
          retcode= ExpLOBInterfaceReadSourceFile(getExeGlobals()->getExLobGlobal(),
                                                 lobData, 
                                                 getLobHdfsServer(),
                                                 getLobHdfsPort(),
                                                 sourceFileReadOffset,
                                                 chunkMemSize,
                                                 retBuf,
                                                 retReadLen);
          if (retcode < 0)
            {
              if (lobLocking())
                retcode = SQL_EXEC_ReleaseLobLock(llid);
              Lng32 intParam1 = -retcode;
              ExRaiseSqlError(h, diagsArea, 
                              (ExeErrorCode)(8442), NULL, &intParam1, 
                              &cliError, NULL, (char*)"ExpLOBInterfaceReadSourceFile",
                              getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
              return ex_expr::EXPR_ERROR;
            }

         
          inputAddr = retBuf;
                                               
        }
      char * handle = op_data[0];
      handleLen = getOperand(0)->getLength();
      err = insertData(handleLen, handle,inputAddr, chunkMemSize,lobHdfsOffset,h, diagsArea);
      if (err == ex_expr::EXPR_ERROR)  
        {
          if (lobLocking())
            retcode = SQL_EXEC_ReleaseLobLock(llid);
          return err;      
        }
    }
 
 
  err = insertDesc(op_data,inputAddr, chunkMemSize,lobHdfsOffset, result,h, diagsArea);
  if (err == ex_expr::EXPR_ERROR)  
    {
      if (lobLocking())
        retcode = SQL_EXEC_ReleaseLobLock(llid);
          return err;      
    }
  

  inputSize -= chunkMemSize;
  if (fromFile())
    sourceFileReadOffset +=chunkMemSize;       
  inputAddr += chunkMemSize;

  
  if(inputSize > 0)
    {
      char * lobHandle = op_data[0];
      handleLen = getOperand(0)->getLength();
      extractFromLOBhandle(&sFlags, &sLobType, &sLobNum, &sUid,
                           &sDescSyskey, &sDescTS, 
                           &sSchNameLen, sSchName,
                           lobHandle); 
     
       tgtLobName = ExpGetLOBname(sUid, sLobNum, tgtLobNameBuf, LOB_NAME_LEN);
    }
  while(inputSize > 0) // chunk rest of the input into lobMaxChunkMemSize chunks and append.
    {
      chunkMemSize = MINOF(getLobMaxChunkMemSize(), inputSize);
      if(!fromEmpty())
        {
     
          if (fromFile())
            {
              //read a chunk of the file input data
              retcode= ExpLOBInterfaceReadSourceFile(getExeGlobals()->getExLobGlobal(),
                                                     lobData, 
                                                     getLobHdfsServer(),
                                                     getLobHdfsPort(),
                                                     sourceFileReadOffset,
                                                     chunkMemSize,
                                                     retBuf,
                                                     retReadLen);
              if (retcode < 0)
                {
                  if (lobLocking())
                    retcode = SQL_EXEC_ReleaseLobLock(llid);
                  Lng32 intParam1 = -retcode;
                  ExRaiseSqlError(h, diagsArea, 
                              (ExeErrorCode)(8442), NULL, &intParam1, 
                              &cliError, NULL, (char*)"ExpLOBInterfaceReadSourceFile",
                              getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
                  return ex_expr::EXPR_ERROR;
                }
              inputAddr = retBuf;
                                               
            }
          char * handle = op_data[0];
          handleLen = getOperand(0)->getLength();
            LobsSubOper so = Lob_None;
          if (fromFile())
            so = Lob_Memory; //It's already been read into memory above
          else if (fromString()) {
            if (getOperand(1)->getVCIndicatorLength() > 0)
              lobLen = getOperand(1)->getLength(op_data[1]-getOperand(1)->getVCIndicatorLength());
            so = Lob_Memory;
          }
          else if (fromLob())
            so = Lob_Lob;
          else if (fromBuffer())
            so= Lob_Buffer;
          else if (fromExternal())
          so = Lob_External_File;
         

          retcode = ExpLOBInterfaceUpdateAppend
            (getExeGlobals()->getExLobGlobal(), 
             (getTcb()->getStatsEntry() != NULL ? getTcb()->getStatsEntry()->castToExHdfsScanStats() : NULL),
             getLobHdfsServer(),
             getLobHdfsPort(),
             tgtLobName, 
             lobStorageLocation(),
             handleLen, handle,
             &outHandleLen_, outLobHandle_,
             lobHdfsOffset,
             -1,	 
             0,
             0,
             so,
             sDescSyskey,
             chunkMemSize, 
             inputAddr,
             NULL, 0, NULL,
             -1, 0,
             getLobMaxSize(), getLobMaxChunkMemSize(),getLobGCLimit());
          if (retcode < 0)
            {
               if (lobLocking())
                 retcode = SQL_EXEC_ReleaseLobLock(llid);
              Lng32 intParam1 = -retcode;
              ExRaiseSqlError(h, diagsArea, 
                              (ExeErrorCode)(8442), NULL, &intParam1, 
                              &cliError, NULL, (char*)"ExpLOBInterfaceReadSourceFile",
                              getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
              return ex_expr::EXPR_ERROR;
            }
          inputSize -= chunkMemSize;
          if (fromFile())
            {
              sourceFileReadOffset +=chunkMemSize; 
              getExeGlobals()->getExLobGlobal()->getHeap()->deallocateMemory(inputAddr);
              
            }
                        
          inputAddr += chunkMemSize;
        }
    }

  if (lobLocking())
    retcode = SQL_EXEC_ReleaseLobLock(llid);
  return err;
}
////////////////////////////////////////////////////////
// ExpLOBdelete
////////////////////////////////////////////////////////
ExpLOBdelete::ExpLOBdelete(){};
ExpLOBdelete::ExpLOBdelete(OperatorTypeEnum oper_type,
			   Attributes ** attr, 
			   Space * space)
  : ExpLOBiud(oper_type, 2, attr, -1, 0, NULL, space),
    ldFlags_(0)
{
};

void ExpLOBdelete::displayContents(Space * space, const char * displayStr, 
				   Int32 clauseNum, char * constsArea)

{
  ExpLOBoper::displayContents(space, "ExpLOBdelete", clauseNum, constsArea);

}


ex_expr::exp_return_type ExpLOBdelete::eval(char *op_data[],
					    CollHeap*h,
					    ComDiagsArea** diagsArea)
{
  Lng32 rc = 0;

  Lng32 lobOperStatus = checkLobOperStatus();
  if (lobOperStatus == DO_NOTHING_)
    return ex_expr::EXPR_OK;

  char * result = op_data[0];

  Lng32 lobType;
  Int64 uid;
  Lng32 lobNum;
  Int64 descSyskey;
  Int64 descTS = -1;
  extractFromLOBhandle(NULL, &lobType, &lobNum, &uid,
		       &descSyskey, NULL, //descTS, 
		       NULL, NULL,
		       op_data[1]);
  
  Lng32 handleLen = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);

  // get the lob name where data need to be deleted
  char lobNameBuf[LOB_NAME_LEN];
  char * lobName = ExpGetLOBname(uid, lobNum, lobNameBuf, LOB_NAME_LEN);
  if (lobName == NULL)
    return ex_expr::EXPR_ERROR;

  Lng32 waitedOp = 0;
  waitedOp = 1;

  Lng32 cliError = 0;

  // call function with the lobname and offset to delete it.
  rc = ExpLOBInterfaceDelete
    (
     getExeGlobals()->getExLobGlobal(),
     (getTcb()->getStatsEntry() != NULL ? getTcb()->getStatsEntry()->castToExHdfsScanStats() : NULL),
     getLobHdfsServer(),
     getLobHdfsPort(),
     lobName, 
     lobStorageLocation(),
     handleLen, op_data[1],
     requestTag_,
     -1,
     descSyskey,
     //     (getExeGlobals()->lobGlobals()->getCurrLobOperInProgress() ? 1 : 0),
     (lobOperStatus == CHECK_STATUS_ ? 1 : 0),
     waitedOp);
  
  if (rc == LOB_ACCESS_PREEMPT)
    {
      return ex_expr::EXPR_PREEMPT;
    }

  if (rc < 0)
    {
      Lng32 intParam1 = -rc;
      ExRaiseSqlError(h, diagsArea, 
		      (ExeErrorCode)(8442), NULL, &intParam1, 
		      &cliError, NULL, (char*)"ExpLOBInterfaceDelete",
		      getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
      return ex_expr::EXPR_ERROR;
    }

  return ex_expr::EXPR_OK;
}

////////////////////////////////////////////////////////
// ExpLOBupdate
////////////////////////////////////////////////////////
ExpLOBupdate::ExpLOBupdate(){};
ExpLOBupdate::ExpLOBupdate(OperatorTypeEnum oper_type,
			   Lng32 numAttrs,
			   Attributes ** attr, 
			   Int64 objectUID,
			   short descSchNameLen,
			   char * descSchName,
			   Space * space)
  : ExpLOBiud(oper_type, numAttrs, attr, objectUID, 
	      descSchNameLen, descSchName, space),
    luFlags_(0),
    nullValue_(0)
{
};

void ExpLOBupdate::displayContents(Space * space, const char * displayStr, 
				   Int32 clauseNum, char * constsArea)

{
  ExpLOBoper::displayContents(space, "ExpLOBupdate", clauseNum, constsArea);

  char buf[100];

  str_sprintf(buf, "    luFlags_ = %d", luFlags_);
  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
}

ex_expr::exp_return_type 
ExpLOBupdate::processNulls(char *null_data[], CollHeap *heap,
			   ComDiagsArea **diagsArea
			   )
{
  nullValue_ = 0;

  if ((getOperand(1)->getNullFlag()) &&    // nullable
      (! null_data[1]))                    // missing value 
    {
      ExpTupleDesc::setNullValue( null_data[0],
				  getOperand(0)->getNullBitIndex(),
				  getOperand(0)->getTupleFormat() );

      return ex_expr::EXPR_NULL;
    }
  
  if ((getOperand(2)->getNullFlag()) &&    // nullable
      (! null_data[2]))                    // missing value 
    {
      nullValue_ = 1;
    }

  // move 0 to the null bytes of result
  if (getOperand(0)->getNullFlag())
    {
      ExpTupleDesc::clearNullValue( null_data[0],
                                    getOperand(0)->getNullBitIndex(),
                                    getOperand(0)->getTupleFormat() );
    }

  return ex_expr::EXPR_OK;
}

ex_expr::exp_return_type ExpLOBupdate::eval(char *op_data[],
					    CollHeap*h,
					    ComDiagsArea** diagsArea)
{
  Lng32 rc, retcode = 0;

  Lng32 lobOperStatus = checkLobOperStatus();
  if (lobOperStatus == DO_NOTHING_)
    return ex_expr::EXPR_OK;

  char * result = op_data[0];
  Int64 lobHdfsOffset = 0;  
  char * lobHandle = NULL;
  Lng32 handleLen = 0;
  char *inputAddr = NULL;
  Int64 lobLen = 0;
  char *lobData = NULL;
  Int64 sourceFileSize = 0; // If the input is from a file get total file size
  Int64 sourceFileReadOffset = 0; 
  Int64 chunkMemSize = 0;
  char *retBuf = 0;
  Int64 retReadLen = 0;
  Int32 cliError = 0;
  Lng32 sLobType;
  Int64 sUid;
  Lng32 sLobNum;
  Int64 sDescSyskey = -1;
  Int64 sDescTS = -1;
  Int16 sFlags;
  short sSchNameLen = 0;
  char sSchName[500];
  char tgtLobNameBuf[LOB_NAME_LEN];
  char *tgtLobName = NULL;
  lobLen = getOperand(1)->getLength();

  char llid[LOB_LOCK_ID_SIZE];
  if (lobLocking())
    {
      ExpLOBoper::genLobLockId(objectUID_,lobNum(),llid);;
      NABoolean found = FALSE;
      retcode = SQL_EXEC_CheckLobLock(llid, &found);
      if (! retcode && !found) 
        {    
          retcode = SQL_EXEC_SetLobLock(llid);
        }
      else if (found)
        {
          Int32 lobError = LOB_DATA_FILE_LOCK_ERROR;
          ExRaiseSqlError(h, diagsArea, 
                          (ExeErrorCode)(8558), NULL,(Int32 *)&lobError, 
                          NULL, NULL, (char*)"ExpLOBInterfaceInsert",
                          getLobErrStr(LOB_DATA_FILE_LOCK_ERROR),NULL);
          return ex_expr::EXPR_ERROR;
        }
    }

  if(fromString())
    {
      //If source is a varchar, find the actual length
      if (getOperand(1)->getVCIndicatorLength() >0)   
        lobLen = getOperand(1)->getLength(op_data[1]- getOperand(1)->getVCIndicatorLength());  
     
    } 
  if(fromFile())
    {
      
      lobData = new (h) char[lobLen];  
      str_cpy_and_null(lobData,op_data[1],lobLen,'\0',' ',TRUE);
    }
    else
      lobData = op_data[1];
  if (fromBuffer())
    {
      memcpy(&lobLen, op_data[2],sizeof(Int64)); // user specified buffer length
      Int64 userBufAddr = 0;
      memcpy(&userBufAddr,op_data[1],sizeof(Int64));
      lobData = (char *)userBufAddr;
    }
 
  long inputSize = lobLen;
  inputAddr = lobData ;
  if (getOperand(2)->getNullFlag() &&
      nullValue_)
    {
      if(fromFile())
        {
          
          retcode = ExpLOBInterfaceGetFileSize(getExeGlobals()->getExLobGlobal(), 
                                               lobData,//filename 
                                               getLobHdfsServer(),
                                               getLobHdfsPort(),
                                               sourceFileSize);
          if (retcode < 0)
            {
              if (lobLocking())
                retcode = SQL_EXEC_ReleaseLobLock(llid);
              Lng32 intParam1 = -retcode;
              ExRaiseSqlError(h, diagsArea, 
                              (ExeErrorCode)(8442), NULL, &intParam1, 
                              &cliError, NULL, (char*)"ExpLOBInterfaceGetFileSize",
                              getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
              return ex_expr::EXPR_ERROR;
            }
          inputSize = sourceFileSize;
        }
      chunkMemSize = MINOF(getLobMaxChunkMemSize(), inputSize);
      lobHandle = op_data[0];
      handleLen = getOperand(0)->getLength();

      if (handleLen == 0)
        {
          ExRaiseSqlError(h, diagsArea, 
                          (ExeErrorCode)(8443), NULL, NULL);
          
          return ex_expr::EXPR_ERROR;
        }

      if (fromFile())
        {
          //read a chunk of the file input data
          retcode= ExpLOBInterfaceReadSourceFile(getExeGlobals()->getExLobGlobal(),
                                                 lobData, 
                                                 getLobHdfsServer(),
                                                 getLobHdfsPort(),
                                                 sourceFileReadOffset,
                                                 chunkMemSize,
                                                 retBuf,
                                                 retReadLen);
          if (retcode < 0)
            {
              if (lobLocking())
                retcode = SQL_EXEC_ReleaseLobLock(llid);
              Lng32 intParam1 = -retcode;
              ExRaiseSqlError(h, diagsArea, 
                              (ExeErrorCode)(8442), NULL, &intParam1, 
                              &cliError, NULL, (char*)"ExpLOBInterfaceReadSourceFile",
                              getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
              return ex_expr::EXPR_ERROR;
            }

         
          inputAddr = retBuf;
                                               
        }

      ex_expr::exp_return_type err = insertData(handleLen, lobHandle, inputAddr, chunkMemSize, lobHdfsOffset, h, diagsArea);;

      if (err == ex_expr::EXPR_ERROR)
	return err;
      err = insertDesc(op_data, inputAddr,chunkMemSize, lobHdfsOffset, result,h, diagsArea);
      if (err == ex_expr::EXPR_ERROR)
	return err;
      return err;

      inputSize -= chunkMemSize;
      if (fromFile())
        sourceFileReadOffset +=chunkMemSize;       
      inputAddr += chunkMemSize;

      if(inputSize > 0)
        {
          char * lobHandle = op_data[0];
          handleLen = getOperand(0)->getLength();
          extractFromLOBhandle(&sFlags, &sLobType, &sLobNum, &sUid,
                               &sDescSyskey, &sDescTS, 
                               &sSchNameLen, sSchName,
                               lobHandle); 
     
          tgtLobName = ExpGetLOBname(sUid, sLobNum, tgtLobNameBuf, LOB_NAME_LEN);
        }
      while(inputSize > 0) // chunk rest of the input into lobMaxChunkMemSize chunks and append.
        {
          chunkMemSize = MINOF(getLobMaxChunkMemSize(), inputSize);
          if(!fromEmpty())
            {
     
              if (fromFile())
                {
                  //read a chunk of the file input data
                  retcode= ExpLOBInterfaceReadSourceFile(getExeGlobals()->getExLobGlobal(),
                                                         lobData, 
                                                         getLobHdfsServer(),
                                                         getLobHdfsPort(),
                                                         sourceFileReadOffset,
                                                         chunkMemSize,
                                                         retBuf,
                                                         retReadLen);
                  if (retcode < 0)
                    {
                      if (lobLocking())
                        retcode = SQL_EXEC_ReleaseLobLock(llid);
                      Lng32 intParam1 = -retcode;
                      ExRaiseSqlError(h, diagsArea, 
                                      (ExeErrorCode)(8442), NULL, &intParam1, 
                                      &cliError, NULL, (char*)"ExpLOBInterfaceReadSourceFile",
                                      getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
                      return ex_expr::EXPR_ERROR;
                    }
                  inputAddr = retBuf;
                                               
                }
              char * handle = op_data[0];
              handleLen = getOperand(0)->getLength();
              LobsSubOper so = Lob_None;
              if (fromFile())
                so = Lob_Memory; // It's already been read into memory above.
              else if (fromString()) {
                if (getOperand(1)->getVCIndicatorLength() > 0)
                  lobLen = getOperand(1)->getLength(op_data[1]-getOperand(1)->getVCIndicatorLength());
                so = Lob_Memory;
              }
              else if (fromLob())
                so = Lob_Lob;
              else if (fromBuffer())
                so= Lob_Buffer;
              else if (fromExternal())
              so = Lob_External_File;
                
              retcode = ExpLOBInterfaceUpdateAppend
                (getExeGlobals()->getExLobGlobal(), 
                 (getTcb()->getStatsEntry() != NULL ? getTcb()->getStatsEntry()->castToExHdfsScanStats() : NULL),
                 getLobHdfsServer(),
                 getLobHdfsPort(),
                 tgtLobName, 
                 lobStorageLocation(),
                 handleLen, handle,
                 &outHandleLen_, outLobHandle_,
                 lobHdfsOffset,
                 -1,	 
                 0,
                 0,
                 so,
                 sDescSyskey,
                 chunkMemSize, 
                 inputAddr,
                 NULL, 0, NULL,
                 -1, 0,
                 getLobMaxSize(), getLobMaxChunkMemSize(),getLobGCLimit());
              if (retcode < 0)
                {
                  if (lobLocking())
                    retcode = SQL_EXEC_ReleaseLobLock(llid);
                  Lng32 intParam1 = -retcode;
                  ExRaiseSqlError(h, diagsArea, 
                                  (ExeErrorCode)(8442), NULL, &intParam1, 
                                  &cliError, NULL, (char*)"ExpLOBInterfaceReadSourceFile",
                                  getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
                  return ex_expr::EXPR_ERROR;
                }
              inputSize -= chunkMemSize;
              if (fromFile())
                {
                  sourceFileReadOffset +=chunkMemSize;
                  getExeGlobals()->getExLobGlobal()->getHeap()->deallocateMemory(inputAddr);
                }     
              inputAddr += chunkMemSize;
            }
        }


    }
  else
    {
      lobHandle = op_data[2];

      handleLen = getOperand(2)->getLength(op_data[-MAX_OPERANDS+2]);
    
     
      extractFromLOBhandle(&sFlags, &sLobType, &sLobNum, &sUid,
                           &sDescSyskey, &sDescTS, 
                           &sSchNameLen, sSchName,
                           lobHandle); //op_data[2]);
      if (sDescSyskey == -1) //updating empty lob
        {
      
          char llid[LOB_LOCK_ID_SIZE];
          if (lobLocking())
            {
              ExpLOBoper::genLobLockId(objectUID_,lobNum(),llid);;
              NABoolean found = FALSE;
              retcode = SQL_EXEC_CheckLobLock(llid, &found);
              if (! retcode && !found) 
                {    
                  retcode = SQL_EXEC_SetLobLock(llid);
                }
              else if (found)
                {
                  ExRaiseSqlError(h, diagsArea, 
                                  (ExeErrorCode)(EXE_LOB_CONCURRENT_ACCESS_ERROR));
                  return ex_expr::EXPR_ERROR;
                }
            }
     
          char * handle = op_data[0];
          handleLen = getOperand(0)->getLength();
          ex_expr::exp_return_type err = insertData(handleLen, handle, inputAddr, inputSize, lobHdfsOffset, h, diagsArea);
          if (err == ex_expr::EXPR_ERROR)
            return err;
          err = insertDesc(op_data,inputAddr,inputSize, lobHdfsOffset, result, h, diagsArea);
          if (err == ex_expr::EXPR_ERROR)
            return err;
          if (lobLocking())
            retcode = SQL_EXEC_ReleaseLobLock(llid);
          return err;
        }
    
  
      // get the lob name where data need to be updated
      char tgtLobNameBuf[LOB_NAME_LEN];
      char * tgtLobName = ExpGetLOBname(sUid, sLobNum, tgtLobNameBuf, LOB_NAME_LEN);

      if (tgtLobName == NULL)
        return ex_expr::EXPR_ERROR;

      char fromLobNameBuf[LOB_NAME_LEN];
      char * fromLobName = NULL;
      Int64 fromDescKey = 0;
      Int64 fromDescTS = 0;
      short fromSchNameLen = 0;
      char  fromSchName[ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN+1];
      // call function with the lobname and source value
      // to update it in the LOB.
      // Get back offset and len of the LOB.
      //  Int64 offset = 0;
      lobLen = getOperand(1)->getLength();
 
      LobsSubOper so = Lob_None;
      if (fromFile())
        so = Lob_File;
      else if (fromString()) {
        if (getOperand(1)->getVCIndicatorLength() > 0)
          lobLen = getOperand(1)->getLength(op_data[1]-getOperand(1)->getVCIndicatorLength());
        so = Lob_Memory;
      }
      else if (fromLob())
        so = Lob_Lob;
      else if (fromBuffer())
        so= Lob_Buffer;
      else if (fromExternal())
        so = Lob_External_File;
 
   
 
      Int64 lobMaxSize = 0;
      if (getLobSize() > 0)
        {
          lobMaxSize = MINOF(getLobSize(), getLobMaxSize());
        }
      else
        lobMaxSize = getLobMaxSize();
      Lng32 waitedOp = 0;
      waitedOp = 1;

      Lng32 cliError = 0;

      char * data = op_data[1];
      if (fromFile())
        data[lobLen] = '\0';
      if (fromBuffer())
        {
          memcpy(&lobLen, op_data[3],sizeof(Int64)); // user specified buffer length
          Int64 userBufAddr = 0;
          memcpy(&userBufAddr,op_data[1],sizeof(Int64));
          data = (char *)userBufAddr;
        }

      if(fromEmpty())
        {
          lobLen = 0;
          so = Lob_Memory;
        }

     
          
      if (isAppend() && !fromEmpty())
        {
          rc = ExpLOBInterfaceUpdateAppend
            (getExeGlobals()->getExLobGlobal(), 
             (getTcb()->getStatsEntry() != NULL ? getTcb()->getStatsEntry()->castToExHdfsScanStats() : NULL),
             getLobHdfsServer(),
             getLobHdfsPort(),
             tgtLobName, 
             lobStorageLocation(),
             handleLen, lobHandle,
             &outHandleLen_, outLobHandle_,
             requestTag_,
             -1,
	 
             (lobOperStatus == CHECK_STATUS_ ? 1 : 0),
             waitedOp,
             so,
             sDescSyskey,
             lobLen, 
             data,
             fromLobName, fromSchNameLen, fromSchName,
             fromDescKey, fromDescTS,
             lobMaxSize, getLobMaxChunkMemSize(),getLobGCLimit());
        }
      else
        {
          rc = ExpLOBInterfaceUpdate
            (getExeGlobals()->getExLobGlobal(), 
             (getTcb()->getStatsEntry() != NULL ? getTcb()->getStatsEntry()->castToExHdfsScanStats() : NULL),
             getLobHdfsServer(),
             getLobHdfsPort(),
             tgtLobName, 
             lobStorageLocation(),
             handleLen, lobHandle,
             &outHandleLen_, outLobHandle_,
             requestTag_,
             -1,
	 
             (lobOperStatus == CHECK_STATUS_ ? 1 : 0),
             waitedOp,
             so,
             sDescSyskey,
             lobLen, 
             data,
             fromLobName, fromSchNameLen, fromSchName,
             fromDescKey, fromDescTS,
             lobMaxSize, getLobMaxChunkMemSize(),getLobGCLimit());
        }
      if (lobLocking())
        retcode = SQL_EXEC_ReleaseLobLock(llid);
      if (rc  < 0)
        {
          Lng32 intParam1 = -rc;
          ExRaiseSqlError(h, diagsArea, 
                          (ExeErrorCode)(8442), NULL, &intParam1, 
                          &cliError, NULL, (char*)"ExpLOBInterfaceUpdate",
                          getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
          return ex_expr::EXPR_ERROR;
        }
    }
  // update lob handle with the returned values
  str_cpy_all(result, lobHandle, handleLen);
 
  getOperand(0)->setVarLength(handleLen, op_data[-MAX_OPERANDS]);

  return ex_expr::EXPR_OK;
}


////////////////////////////////////////////////////////
// ExpLOBselect
////////////////////////////////////////////////////////
ExpLOBselect::ExpLOBselect(){};
ExpLOBselect::ExpLOBselect(OperatorTypeEnum oper_type,
			   Attributes ** attr, 
			   Space * space)
  : ExpLOBoper(oper_type, 2, attr, space),
    lsFlags_(0)
   
{
  tgtLocation_[0] = 0;
  tgtFile_[0] = 0;
};

void ExpLOBselect::displayContents(Space * space, const char * displayStr, 
				   Int32 clauseNum, char * constsArea)

{
  ExpLOBoper::displayContents(space, "ExpLOBselect", clauseNum, constsArea); 
}


ex_expr::exp_return_type ExpLOBselect::eval(char *op_data[],
					    CollHeap*h,
					    ComDiagsArea** diagsArea)
{
  char * result = op_data[0];
  Lng32 rc = 0;
  Int64 uid;
  Lng32 lobType;
  Lng32 lobNum;
  Int64 descKey;
  Int16 flags;
  Int64 descTS = -1;
  short schNameLen = 0;
  char  schName[500];
  LobsSubOper so;
  Lng32 waitedOp = 0;
  Int64 lobLen = 0; 
  char *lobData = NULL;
  Lng32 cliError = 0;
  Lng32 lobOperStatus = checkLobOperStatus();
  if (lobOperStatus == DO_NOTHING_)
    return ex_expr::EXPR_OK;
  Int32 handleLen = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
  char * lobHandle = op_data[1];
  extractFromLOBhandle(&flags, &lobType, &lobNum, &uid,
		       &descKey, &descTS, 
		       &schNameLen, schName,
		       lobHandle);
  // select returns lobhandle only
  str_pad(result, getOperand(0)->getLength());
  str_cpy_all(result, lobHandle, strlen(lobHandle));
  
  return ex_expr::EXPR_OK;
}

////////////////////////////////////////////////////////
// ExpLOBconvert
////////////////////////////////////////////////////////
ExpLOBconvert::ExpLOBconvert(){};
ExpLOBconvert::ExpLOBconvert(OperatorTypeEnum oper_type,
					 Attributes ** attr, 
					 Space * space)
  : ExpLOBoper(oper_type, 2, attr, space),
    lcFlags_(0),
    convertSize_(0)
{
};

void ExpLOBconvert::displayContents(Space * space, const char * displayStr, 
				   Int32 clauseNum, char * constsArea)

{
  ExpLOBoper::displayContents(space, "ExpLOBconvert", clauseNum, constsArea);

}

ex_expr::exp_return_type ExpLOBconvert::eval(char *op_data[],
					     CollHeap*h,
					     ComDiagsArea** diagsArea)
{
  Lng32 rc = 0;

  Lng32 lobOperStatus = checkLobOperStatus();
  if (lobOperStatus == DO_NOTHING_)
    return ex_expr::EXPR_OK;

  char * result = op_data[0];
  char * lobHandle = op_data[1];
  char *tgtFileName = NULL;
  Int32 handleLen = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);

  Int64 uid;
  Lng32 lobType;
  Lng32 lobNum;
  Int64 descKey;
  Int64 descTS;
  Int16 flags;
  short schNameLen = 0;
  char schName[500];
  LobsSubOper so;
  Lng32 cliError = 0;

  Lng32 waitedOp = 0;
  Int64 lobLen = 0; 
  char *lobData = NULL;
  waitedOp = 1;

  extractFromLOBhandle(&flags, &lobType, &lobNum, &uid,
			   &descKey, &descTS, 
			   &schNameLen, schName,
			   lobHandle);
  // get the lob name where data need to be inserted
  char lobNameBuf[LOB_NAME_LEN];
  char * lobName = ExpGetLOBname(uid, lobNum, lobNameBuf, LOB_NAME_LEN);
      
  if (descKey == -1) //This is an empty_blob/clob
    {
      Int32 intParam1 = LOB_DATA_EMPTY_ERROR;
      Int32 cliError = 0;
       ExRaiseSqlError(h, diagsArea, 
			  (ExeErrorCode)(8442), NULL, &intParam1, 
			  &cliError, NULL, (char*)"ExpLOBInterfaceSelect",
		          getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
	  return ex_expr::EXPR_ERROR;
    }
  if(toFile())
    {
      so = Lob_File;
      tgtFileName = tgtFileName_;
      rc = ExpLOBInterfaceSelect(getExeGlobals()->getExLobGlobal(), 
                                 (getTcb()->getStatsEntry() != NULL ? getTcb()->getStatsEntry()->castToExHdfsScanStats() : NULL),
				 lobName, 
				 lobStorageLocation(),
				 lobType,
				 getLobHdfsServer(), getLobHdfsPort(),

				 handleLen, lobHandle,
				 requestTag_,
                                 so,
				 -1,
				 (lobOperStatus == CHECK_STATUS_ ? 1 : 0),
 				 waitedOp,

				 0, lobLen, lobLen, tgtFileName,getLobMaxChunkMemSize());
    }
  else if (toString())
    {
      so = Lob_Memory;
      
      if (lobName == NULL)
	return ex_expr::EXPR_ERROR;
      
      
      lobLen = getConvertSize(); 
      lobData = new(h) char[(Lng32)lobLen];
      rc = ExpLOBInterfaceSelect(getExeGlobals()->getExLobGlobal(), 
                                 (getTcb()->getStatsEntry() != NULL ? getTcb()->getStatsEntry()->castToExHdfsScanStats() : NULL),
				 lobName, 
				 lobStorageLocation(),
				 lobType,
				 getLobHdfsServer(), getLobHdfsPort(),

				 handleLen, lobHandle,
				 requestTag_,
                                 so,
				 -1,
				 (lobOperStatus == CHECK_STATUS_ ? 1 : 0),
 				 waitedOp,

				 0, lobLen, lobLen, lobData,getLobMaxChunkMemSize());

      if (rc == LOB_ACCESS_PREEMPT)
	{
	  return ex_expr::EXPR_PREEMPT;
	}
      
      if (rc < 0)
	{
	  Lng32 intParam1 = -rc;
	  ExRaiseSqlError(h, diagsArea, 
			  (ExeErrorCode)(8442), NULL, &intParam1, 
			  &cliError, NULL, (char*)"ExpLOBInterfaceSelect",
		          getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
	  return ex_expr::EXPR_ERROR;
	}

      // store the length of substring in the varlen indicator.
      if (getOperand(0)->getVCIndicatorLength() > 0)
	{
	  getOperand(0)->setVarLength((UInt32)lobLen, op_data[-MAX_OPERANDS] );
	  if (lobLen > 0) 
	    str_cpy_all(op_data[0], lobData, (Lng32)lobLen);
	}
      else
	{
	  str_pad(result, getOperand(0)->getLength());
	  str_cpy_all(result, lobData, strlen(lobData));
	}
      NADELETEBASIC(lobData, h);
    }
  else
    return ex_expr::EXPR_ERROR;

  return ex_expr::EXPR_OK;
}

////////////////////////////////////////////////////////
// ExpLOBconvertHandle
////////////////////////////////////////////////////////
ExpLOBconvertHandle::ExpLOBconvertHandle(){};
ExpLOBconvertHandle::ExpLOBconvertHandle(OperatorTypeEnum oper_type,
					 Attributes ** attr, 
					 Space * space)
  : ExpLOBoper(oper_type, 2, attr, space),
    lchFlags_(0)
{
};

void ExpLOBconvertHandle::displayContents(Space * space, const char * displayStr, 
				   Int32 clauseNum, char * constsArea)

{
  ExpLOBoper::displayContents(space, "ExpLOBconvertHandle", clauseNum, constsArea);

}


ex_expr::exp_return_type ExpLOBconvertHandle::eval(char *op_data[],
						   CollHeap*,
						   ComDiagsArea** diagsArea)
{
  char * result = op_data[0];
  char * source = op_data[1];

  Lng32 lobType;
  Int64 uid = 0;
  Lng32 lobNum = 0;
  Int64 descPartnKey = 0;
  Int64 descSysKey = 0;
  Int64 descTS = 0;
  Int16 flags = 0;
  Int16 schNameLen = 0;
  char schName[1024];
  long handleLen = 0;
  if (toString())
    {
      extractFromLOBhandle(&flags, &lobType, &lobNum, &uid,
			   &descSysKey, &descTS, 
			   &schNameLen, schName,
			   source);
      
      char lobHandleBuf[LOB_HANDLE_LEN];
      createLOBhandleString(flags, lobType, uid, lobNum, 
			    descSysKey, descTS, 
			    schNameLen, schName,
			    lobHandleBuf);
      
      str_cpy_all(result, lobHandleBuf, strlen(lobHandleBuf));

      getOperand(0)->setVarLength(strlen(lobHandleBuf), op_data[-MAX_OPERANDS]);
    }
  else if (toLob())
    {
       Int64 addrOfLobHandleLen = (Int64)source-sizeof(short) ;  
       Int32 lobHandleLen = *(short *)addrOfLobHandleLen;
      if (strncmp(source,"LOBH",4)== 0)
	{
	  // This is the external string format of the handle

	  extractFromLOBstring(uid, 
			       lobNum, 
			       descPartnKey,
			       descSysKey, 
			       flags,
			       lobType,
			       schNameLen,
			       schName,
			       source, 
			       lobHandleLen);
      
	  Lng32 handleLen = 0;
	  short lobType = 1;
	  genLOBhandle(uid, lobNum, lobType, descPartnKey, descSysKey, flags,
		   schNameLen, schName,
		   handleLen,
		   result);

	  getOperand(0)->setVarLength(handleLen, op_data[-MAX_OPERANDS]);
	}
      else 
	{
	  // Source is pointing to the internal packed format as stored on disk.
	  // Simply cast result to source so it can be interpreted/cast 
	  // as LOBhandle.
	  str_cpy_all(result, source, sizeof(Int64));
	  getOperand(0)->setVarLength(lobHandleLen, op_data[-MAX_OPERANDS]);
	}
    }

  return ex_expr::EXPR_OK;
}


//////////////////////////////////////////////////
// ExpLOBfunction
//////////////////////////////////////////////////
ExpLOBfunction::ExpLOBfunction(){};
ExpLOBfunction::ExpLOBfunction(OperatorTypeEnum oper_type,
			       short num_operands,
			       Attributes ** attr,
			       Space * space)
  : ExpLOBoper(oper_type, num_operands, attr, space),
    funcFlags_(0)
{};


//////////////////////////////////////////////////
// ExpLOBfuncSubstring
//////////////////////////////////////////////////
ExpLOBfuncSubstring::ExpLOBfuncSubstring(){};
ExpLOBfuncSubstring::ExpLOBfuncSubstring(OperatorTypeEnum oper_type,
					 short num_operands,
					 Attributes ** attr,
					 Space * space)
  : ExpLOBfunction(oper_type, num_operands, attr, space)
{};

void ExpLOBfuncSubstring::displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea)
  
{
  ex_clause::displayContents(space, "ExpLOBfuncSubstring", clauseNum, constsArea);

}

ex_expr::exp_return_type ExpLOBfuncSubstring::eval(char *op_data[],
						   CollHeap *heap,
						   ComDiagsArea** diagsArea)
{

  Int32 len1_bytes = getOperand(1)->getLength(op_data[-MAX_OPERANDS+1]);
  Int32 startPos = *(Lng32 *)op_data[2];
  
  Int32 specifiedLen = len1_bytes;
  if (getNumOperands() == 4)
    specifiedLen = *(Lng32 *)op_data[3]; 

  return ex_expr::EXPR_OK;
}
  
Lng32 LOBsql2loaderInterface
(
 /*IN*/     char * fileName,
 /*IN*/     Lng32  fileNameLen,
 /*IN*/     char * loaderInfo,
 /*IN*/     Lng32  loaderInfoLen,
 /*IN*/     char * handle,
 /*IN*/     Lng32  handleLen,
 /*IN*/     char * lobInfo,
 /*IN*/     Lng32  lobInfoLen
 )
{
  return 0;
}
