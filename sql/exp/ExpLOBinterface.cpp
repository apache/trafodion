/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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
**********************************************************************/
#include <iostream>
using std::cerr;
using std::endl;

#include <fstream>
using std::ofstream;

#include "Platform.h"
#include "SQLCLIdev.h"

#include "str.h"
#include "ExpLOBinterface.h"


Lng32 ExpLOBinterfaceInit(void *& lobGlob, void * lobHeap,NABoolean isHive)
{
  Ex_Lob_Error err;

  Int64 dummyParam = 0;
  Int64 cliError = -1; 
  Ex_Lob_Error status;
  
  char dir[100];
  strcpy(dir, "/h/temp");
  err = ExLobsOper(dir,
		   NULL, 0,
		   NULL, 0, 
		   NULL, dummyParam, 0, dummyParam, 
                   dummyParam, 0, dummyParam, status, cliError,
		   dir, Lob_HDFS_File,
		   NULL, 0, 
		   0,NULL,
		   Lob_Init,
		   Lob_None,
                   1, // waited op
		   lobGlob,
		   0,
		   NULL, 0
		   );
  if (lobGlob)
    {
      ((ExLobGlobals *)lobGlob)->setIsHive(isHive);
      NAHeap *heap = new ((NAHeap *)lobHeap) NAHeap("LOB Heap", (NAHeap *)lobHeap);
      
      heap->setThreadSafe();
      ((ExLobGlobals *)lobGlob)->setHeap(heap);
      
    }
    

  if (err != LOB_OPER_OK)
    return -1;
  else
    return 0;
}

Lng32 ExpLOBinterfaceCleanup(void *& lobGlob, void * lobHeap)
{
  Ex_Lob_Error err;

  Int64 dummyParam = 0;
  Int64 cliError = -1; 
  Ex_Lob_Error status;
  
  char dir[100];
  strcpy(dir, "/h/temp");
  err = ExLobsOper(dir,
		   NULL, 0,
		   NULL, 0, 
		   NULL, dummyParam, 0, dummyParam, 
                   dummyParam, 0, dummyParam, status, cliError,
		   dir, Lob_HDFS_File,
		   NULL, 0, 
		   0,NULL,
		   Lob_Cleanup, // Lob_Cleanup
		   Lob_None,
                   1, // waited op
		   lobGlob,
		   0,
		   NULL, 0
		   );
  if (err != LOB_OPER_OK)
    return -1;
  else
    return 0;
}

Lng32 ExpLOBinterfaceCreate(
			    void * lobGlob, char * lobName, char * lobLoc,
			    Lng32 lobType,
			    char * lobHdfsServer,
			    Lng32 lobHdfsPort,
	                    int    bufferSize ,
	                    short  replication ,
	                    int    blockSize)
{
  Ex_Lob_Error err;

  Int64 dummyParam;
  Ex_Lob_Error status;
  Int64 cliError = -1;
  
  err = ExLobsOper(lobName,
		   NULL, 0, 
		   lobHdfsServer, lobHdfsPort,
		   NULL, dummyParam, 0, dummyParam, 
                   dummyParam, 0, dummyParam, status, cliError, 
		   lobLoc, (LobsStorage)lobType, //Lob_HDFS_File,
		   NULL, 0,
		   0,NULL,
		   Lob_Create,
		   Lob_None,
                   1, // waited op
		   lobGlob,
		   0, NULL, 0,
                   bufferSize ,
                   replication,
                   blockSize
		   );

  if (err != LOB_OPER_OK)
    return -(short)err;
  else
    return 0;
}
/*
 * Lng32 ExpLOBinterfaceEmptyDirectory(void * lobGlob,
                            char * lobName,
                            char * lobLoc,
                            Lng32 lobType = (Lng32)Lob_Empty_Directory,
                            char * lobHdfsServer = NULL,
                            Lng32 lobHdfsPort = 0,
                            int    bufferSize = 0,
                            short  replication =0,
                            int    blocksize=0);
 */
Lng32 ExpLOBinterfaceEmptyDirectory(
                            void * lobGlob,
                            char * lobName,
                            char * lobLoc,
                            Lng32 lobType,
                            char * lobHdfsServer,
                            Lng32 lobHdfsPort,
                            int    bufferSize ,
                            short  replication ,
                            int    blockSize)
{
  Ex_Lob_Error err;

  Int64 dummyParam;
  Ex_Lob_Error status;
  Int64 cliError = -1;

  err = ExLobsOper(lobName,
                   NULL, 0,
                   lobHdfsServer, lobHdfsPort,
                   NULL, dummyParam, 0, dummyParam,
                   dummyParam, 0, dummyParam, status, cliError,
                   lobLoc, (LobsStorage)lobType, //Lob_HDFS_File,
                   NULL, 0,
		   0,NULL,
                   Lob_Empty_Directory,
                   Lob_None,
                   1, // waited op
                   lobGlob,
                   0, NULL, 0,
                   bufferSize ,
                   replication,
                   blockSize
                   );

  if (err != LOB_OPER_OK)
    return -(short)err;
  else
    return 0;
}
Lng32 ExpLOBinterfaceDrop(void * lobGlob,  char * lobHdfsServer ,
			  Lng32 lobHdfsPort,char * lobName, char * lobLoc)
{
  Ex_Lob_Error err;

  Int64 dummyParam;
  Ex_Lob_Error status;
  Int64 cliError = -1;
  
  err = ExLobsOper(lobName,
		   NULL, 0,
		   lobHdfsServer, lobHdfsPort, 
		   NULL, dummyParam, 0, dummyParam, 
                   dummyParam, 0, dummyParam, status, cliError,
		   lobLoc, Lob_HDFS_File,
		   NULL, 0,
		   0,NULL,
		   Lob_Drop,
		   Lob_None,
                   1, // waited op
		   lobGlob, 
		   0, NULL, 0
		   );

  if (err != LOB_OPER_OK)
    return -(short)err;
  else
    return 0;
}

Lng32 ExpLOBInterfacePurgedata(void * lobGlob,  
			       char * lobHdfsServer ,
			       Lng32 lobHdfsPort ,char * lobName, char * lobLoc)
{
  Ex_Lob_Error err;

  Int64 dummyParam;
  Ex_Lob_Error status;
  Int64 cliError = -1;
  
  //  char dir[100];
  //  strcpy(dir, "/h/vshah");
  err = ExLobsOper(lobName, 
		   lobHdfsServer, lobHdfsPort, 
		   NULL, 0,
		   NULL, dummyParam, 0, dummyParam, 
                   dummyParam, 0, dummyParam, status, cliError,
		   lobLoc, Lob_HDFS_File,
		   NULL, 0,
		   0,NULL,
		   Lob_Purge,
		   Lob_None,
                   1, // waited
                   lobGlob,
		   0, NULL, 0
		   );

  if (err != LOB_OPER_OK)
    return -(short)err;
  else
    return LOB_ACCESS_SUCCESS;
}

Lng32 ExpLOBinterfaceCloseFile(void * lobGlob, 
			       char * lobName,
			       char * lobLoc,
			       Lng32 lobType,
			       char * lobHdfsServer,
			       Lng32 lobHdfsPort)
{
  Ex_Lob_Error err;

  Int64 dummyParam;
  Ex_Lob_Error status;
  Int64 cliError = -1;
  
  LobsStorage ls = (LobsStorage)lobType;

  err = ExLobsOper(lobName, 
		   NULL, 0, 
		   lobHdfsServer, lobHdfsPort,
		   NULL, dummyParam, 0, dummyParam, 
                   dummyParam, 0, dummyParam, status, cliError,
		   lobLoc, ls, //Lob_HDFS_File,
		   NULL, 0,
		   0,NULL,
		   Lob_CloseFile,
		  
		   Lob_None,
                   1, // waited
                   lobGlob,
		   0, NULL, 0
		   );

  if (err != LOB_OPER_OK)
    return -(short)err;
  else
    return LOB_ACCESS_SUCCESS;
}


Lng32 ExpLOBInterfaceInsert(void * lobGlob, 
			    char * tgtLobName,
			    char * lobStorageLocation,
			    Lng32 lobType,
			    char * lobHdfsServer,
			    Lng32 lobHdfsPort,

			    Lng32 handleLen,
			    char * lobHandle,
			    Int64 *outHandleLen,
			    char * outLobHandle,

			    Int64 blackBoxLen,
			    char * blackBox,

			    Int64 &requestTag,
			    Int64 xnId,
			    Int64 &descSyskey,
			    LobsOper lo,

			    Lng32 * cliError,
			    LobsSubOper so,
			    
			    Lng32 waitedOp,
			    
			    char * srcLobData, 
			    Int64  srcLobLen,
			    Int64 lobMaxSize,
			    int   bufferSize ,
			    short replication ,
			    int   blockSize)
{
  Ex_Lob_Error err;

  Int64 dummyParam;
  Ex_Lob_Error status;
  Int64 ce = 0;
  
  Int64 inDescSyskey = -1;
  Int64 outDescSyskey  = -1;
  if (srcLobData)
    inDescSyskey = descSyskey;
 
 
  if (( lo == Lob_InsertDataSimple) || 
      ( lo == Lob_InsertDesc) || 
      (lo == Lob_InsertData))
    requestTag = -1;
      

  LobsStorage ls = (LobsStorage)lobType;

  //  Int64 lobLen = (tgtLobLen ? *tgtLobLen : 0);
  err = ExLobsOper(tgtLobName, 
		   lobHandle, handleLen, 
		   lobHdfsServer, lobHdfsPort,
		   outLobHandle, *outHandleLen,
		   inDescSyskey, outDescSyskey, // for flat files 
		   dummyParam, //lobLen,
		   requestTag, requestTag,
                   status, ce,
		   lobStorageLocation, ls, //Lob_HDFS_File,
		   srcLobData, srcLobLen, //strlen(srcLobData),
		   0,NULL,
		   lo, 
		   so,
                   waitedOp,
		   lobGlob,
		   xnId, 
		   blackBox, blackBoxLen,
		   lobMaxSize,
		   bufferSize,
		   replication,
		   blockSize
		   );


  if ((err == LOB_OPER_OK) &&
      (status == LOB_OPER_REQ_IN_PROGRESS))
    {
      // this request has been sent and is in progress.
      // requestTag contains the id of the request.
      return LOB_ACCESS_PREEMPT;      
    }
    
  // done with the request (success or error)
  requestTag = -1;
  if (err != LOB_OPER_OK)
    {
      if ((cliError) &&
	  (ce != 0))
	*cliError = (Lng32)ce;
	
      return -(short)err;
    }

  descSyskey = outDescSyskey;
  if ((waitedOp) ||
      ((Ex_Lob_Error)status == LOB_OPER_OK))
    {
      return LOB_ACCESS_SUCCESS;
    }

  // error. Status contains the error code.
  return -(short)status;
}

Lng32 ExpLOBInterfaceInsertSelect(void * lobGlob, 
				  char * lobHdfsServer ,
				  Lng32 lobHdfsPort ,
				  char * tgtLobName,
				  char * lobStorageLocation,
				  
				  Lng32 handleLen,
				  char * lobHandle,
				  
				  Int64 &descSyskey,
				  Int64 &lobLen,
				  
				  char * lobData, 
				  
				  char * srcLobName, 
				  short srcDescSchNameLen,
				  char * srcDescSchName,
				  
				  Int64 srcDescKey, 
				  Int64 srcDescTS)
{
  Ex_Lob_Error err;

  LobsSubOper lso = Lob_Foreign_Lob;
  Ex_Lob_Error status;
  Int64 dummyParam;
  Int64 cliError = -1;

  err = ExLobsOper(tgtLobName, 
		   lobHandle, handleLen, 
		   NULL, 0, // hdfs server/port
                   NULL, dummyParam, -1, descSyskey, lobLen,
                   0, dummyParam, status, cliError, 
		   lobStorageLocation, Lob_HDFS_File,
		   lobData, lobLen, //strlen(srcLobData),
		   0,NULL,
		   Lob_Insert,
		   lso,
                   1, 
		   lobGlob,
		   0, NULL, 0
		   );

  if (err != LOB_OPER_OK)
    return -(short)err;

  return 0;
}

Lng32 ExpLOBInterfaceUpdateAppend(void * lobGlob, 
				  char * lobHdfsServer ,
				  Lng32 lobHdfsPort ,
				  char * tgtLobName,
				  char * lobStorageLocation,
				  Lng32 handleLen,
				  char * lobHandle,
				  Int64 *outHandleLen,
				  char * outLobHandle,
				  Int64 &requestTag,
				  Int64 xnId,
				  Lng32 checkStatus,
				  Lng32 waitedOp,
				  LobsSubOper so,
				  
				  Int64 &tgtDescSyskey,
				  Int64 tgtLobLen,
				  char * srcLobData, 
				  char * srcLobName, 
				  short srcDescSchNameLen,
				  char * srcDescSchName,
				  Int64 srcDescKey, 
				  Int64 srcDescTS
				  )
{
  Ex_Lob_Error err;

  Int64 operLen = 0;
  Int64 dummyParam = 0;
  Int64 savedTgtLobLen = tgtLobLen;
  Ex_Lob_Error status;
  Int64 cliError = -1;

  err = ExLobsOper(tgtLobName, 
                   lobHandle, handleLen, 
		   lobHdfsServer, lobHdfsPort, // hdfs server/port
                   outLobHandle, *outHandleLen,
		   tgtDescSyskey, dummyParam, operLen,
                   0, dummyParam, status, cliError,
                   lobStorageLocation, Lob_HDFS_File,
                   srcLobData, strlen(srcLobData), //strlen(srcLobData),
		   0,NULL,
                   Lob_Append,
                   so,
                   1, 
                   lobGlob,
                   0, NULL, 0
                   );

  if ((err == LOB_OPER_OK) &&
      (status == LOB_OPER_REQ_IN_PROGRESS))
    {
      // this request has been sent and is in progress.
      // requestTag contains the id of the request.
      return LOB_ACCESS_PREEMPT;      
    }
    
  // done with the request (success or error)
  requestTag = -1;
  if (err != LOB_OPER_OK)
    {
      return -(short)err;
    }

  if ((waitedOp) ||
      ((Ex_Lob_Error)status == LOB_OPER_OK))
    {
      return LOB_ACCESS_SUCCESS;
    }

  // error. Status contains the error code.
  return -(short)status;
}

Lng32 ExpLOBInterfaceUpdate(void * lobGlob, 
			    char * lobHdfsServer ,
			    Lng32 lobHdfsPort,

			    char * tgtLobName,
			    char * lobStorageLocation,
			    Lng32 handleLen,
			    char * lobHandle,
			    Int64 *outHandleLen,
			    char * outLobHandle,
			    Int64 &requestTag,
			    Int64 xnId,
			    Lng32 checkStatus,
			    Lng32 waitedOp,
			    LobsSubOper so,
			    
			    Int64 &tgtDescSyskey,
			    Int64 tgtLobLen,
			    char * srcLobData, 
			    char * srcLobName, 
			    short srcDescSchNameLen,
			    char * srcDescSchName,
			    Int64 srcDescKey, 
			    Int64 srcDescTS)
{
  Ex_Lob_Error err;

  Int64 operLen = 0;
  Int64 dummyParam = 0;
  Int64 savedTgtLobLen = tgtLobLen;
  Ex_Lob_Error status;
  Int64 cliError = -1;

  err = ExLobsOper(tgtLobName, 
                   lobHandle, handleLen, 
		   lobHdfsServer, lobHdfsPort, // hdfs server/port
                   outLobHandle, *outHandleLen,
		   tgtDescSyskey, dummyParam, operLen,
                   0, dummyParam, status, cliError,
                   lobStorageLocation, Lob_HDFS_File,
                   srcLobData, strlen(srcLobData), //strlen(srcLobData),
		   0,NULL,
                   Lob_Update,
                   so,
                   1, 
                   lobGlob,
                   xnId, 
		   NULL, 0
                   );

  if ((err == LOB_OPER_OK) &&
      (status == LOB_OPER_REQ_IN_PROGRESS))
    {
      // this request has been sent and is in progress.
      // requestTag contains the id of the request.
      return LOB_ACCESS_PREEMPT;      
    }
    
  // done with the request (success or error)
  requestTag = -1;
  if (err != LOB_OPER_OK)
    {
      return -(short)err;
    }

  if ((waitedOp) ||
      ((Ex_Lob_Error)status == LOB_OPER_OK))
    {
      return LOB_ACCESS_SUCCESS;
    }

  // error. Status contains the error code.
  return -(short)status;
}

Lng32 ExpLOBInterfaceDelete(void * lobGlob, 
			    char * lobHdfsServer ,
			    Lng32 lobHdfsPort ,
			    char * lobName,
		    char * lobLoc,
			    Lng32 handleLen,
			    char * lobHandle,
			    Int64 &requestTag,
			    Int64 xnId,
			    Int64 descSyskey,
			    Lng32 checkStatus,
			    Lng32 waitedOp)
{
  Ex_Lob_Error err;

  Int64 dummyParam;
  Ex_Lob_Error status;
  Int64 cliError = -1;

  LobsOper lo;
  if (checkStatus)
    lo = Lob_Check_Status;
  else
    {
      requestTag = -1;
      lo = Lob_Delete;
    }

  err = ExLobsOper(lobName, 
		   lobHandle, handleLen, 
		   lobHdfsServer, lobHdfsPort, // hdfs server/port
                   NULL, dummyParam, 
		   descSyskey, dummyParam, 
		   dummyParam,
		   requestTag, requestTag,
                   status, cliError,
		   lobLoc, Lob_HDFS_File,
		   NULL, 0, 
		   0,NULL,
		   lo,
		   Lob_None,
                   waitedOp, 
		   lobGlob,
		   xnId, 
		   NULL, 0
		   );

  if ((err == LOB_OPER_OK) &&
      (status == LOB_OPER_REQ_IN_PROGRESS))
    {
      // this request has been sent and is in progress.
      // requestTag contains the id of the request.
      return LOB_ACCESS_PREEMPT;      
    }
    
  // done with the request (success or error)
  requestTag = -1;
  if (err != LOB_OPER_OK)
    {
      return -(short)err;
    }

  if ((waitedOp) ||
      ((Ex_Lob_Error)status == LOB_OPER_OK))
    {
      return LOB_ACCESS_SUCCESS;
    }

  // error. Status contains the error code.
  return -(short)status;

}

Lng32 ExpLOBInterfaceSelect(void * lobGlob, 
			    char * lobName, 
			    char * lobLoc,
			    Lng32 lobType,
			    char * lobHdfsServer,
			    Lng32 lobHdfsPort,

			    Lng32 handleLen,
			    char * lobHandle,
			    Int64 &requestTag,
			    LobsSubOper so,
			    Int64 xnId,
			    Lng32 checkStatus,
			    Lng32 waitedOp,

			    Int64 srcOffset, Int64 inLen, 
			    Int64 &outLen, char * lobData)
{
  Ex_Lob_Error err;

  Int64 dummyParam;
  Ex_Lob_Error status;
  Int64 cliError;
  
  LobsOper lo;
  if (checkStatus)
    lo = Lob_Check_Status;
  else if (lobHandle == NULL)
    {
      requestTag = -1;
      lo = Lob_ReadDataSimple;
    }
  else
    {
      requestTag = -1;
      lo = Lob_Read;
    }
 
  LobsStorage ls = (LobsStorage)lobType;

  err = ExLobsOper(lobName, 
		   lobHandle, handleLen, 
		   lobHdfsServer, lobHdfsPort,
                   NULL, dummyParam, 
		   srcOffset, dummyParam, 
		   outLen,
                   requestTag, requestTag,
		   status, cliError, 
		   lobLoc, ls, //Lob_HDFS_File,
		   lobData, inLen, 
		   0, NULL,
		   lo,
		   so,
                   waitedOp, 
		   lobGlob,
		   xnId, 
		   NULL, 0
		   );

  if ((err == LOB_OPER_OK) &&
      (status == LOB_OPER_REQ_IN_PROGRESS))
    {
      // this request has been sent and is in progress.
      // requestTag contains the id of the request.
      return LOB_ACCESS_PREEMPT;      
    }

  // done with the request (success or error)
  requestTag = -1;
  if (err != LOB_OPER_OK)
    {
      return -(short)err;
    }

  if ((waitedOp) ||
      ((Ex_Lob_Error)status == LOB_OPER_OK))
    {
      return LOB_ACCESS_SUCCESS;
    }

  // error. Status contains the error code.
  return -(short)status;
}

Lng32 ExpLOBInterfaceSelectCursor(void * lobGlob, 
				  char * lobName, 
				  char * lobLoc,
				  Lng32 lobType,
				  char * lobHdfsServer,
				  Lng32 lobHdfsPort,

				  Int64 handleLen, 
				  char * lobHandle,

				  Int64 cursorBytes,
				  char *cursorId,
				  
				  Int64 &requestTag,
				  Lng32 checkStatus,
				  Lng32 waitedOp,

                                  Int64 srcOffset, Int64 inLen, 
			          Int64 &outLen, char * lobData,
				  
				  Lng32 oper, // 1: open. 2: fetch. 3: close
                                  Lng32 openType // 0: not applicable. 1: preOpen. 2: mustOpen.
				  )
{
  Ex_Lob_Error err;
  
  Int64 dummyParam = 0;
  Ex_Lob_Error status;
  Int64 cliError;
  
  LobsOper lo;

  //if (lobHandle == NULL)
    {
      if (oper == 1)
	lo = Lob_OpenDataCursorSimple;
      else if (oper == 2)
	lo = Lob_ReadDataCursorSimple;
      else if (oper == 3)
	lo = Lob_CloseDataCursorSimple;
      else
	return -1;
    }
  /*else
    {
      if (oper == 1)
	lo = Lob_OpenCursor;
      else if (oper == 2)
	lo = Lob_ReadCursor;
      else if (oper == 3)
	lo = Lob_CloseCursor;
      else
	return -1;
    }
*/
  if (checkStatus)
    lo = Lob_Check_Status;
  else
    requestTag = -1;

  LobsStorage ls = (LobsStorage)lobType;

  err = ExLobsOper(lobName, 
		   lobHandle, handleLen, 
		   lobHdfsServer, lobHdfsPort,
                   NULL, dummyParam, 
		   srcOffset, dummyParam, 
		   outLen,
                   requestTag, requestTag,
		   status, cliError, 
		   lobLoc, ls, //Lob_HDFS_File,
		   lobData, inLen, 
		   cursorBytes,cursorId,
		   lo,
		   Lob_Memory,
                   waitedOp,
		   lobGlob,
		   0,
		   NULL, 0,0,0,0,
                   openType
		   );

  if (err != LOB_OPER_OK)
    {
      return -(short)err;
    }
  
  return LOB_ACCESS_SUCCESS;
}

Lng32 ExpLOBinterfaceStats(
			    void * lobGlob, 
			    ExLobStats * lobStats,
			    char * lobName, char * lobLoc,
			    Lng32 lobType,
			    char * lobHdfsServer,
			    Lng32 lobHdfsPort)
{
  Ex_Lob_Error err;

  Int64 dummyParam;
  Ex_Lob_Error status;
  Int64 cliError = -1;
  
  err = ExLobsOper(lobName,
		   NULL, 0, 
		   lobHdfsServer, lobHdfsPort,
		   NULL, dummyParam, 0, dummyParam, 
                   dummyParam, 0, dummyParam, status, cliError, 
		   lobLoc, (LobsStorage)lobType, 
		   (char*)lobStats, 0,
		   0,NULL,
		   Lob_Stats,
		   Lob_None,
                   1, // waited op
		   lobGlob,
		   0, NULL, 0
		   );

  if (err != LOB_OPER_OK)
    return -(short)err;
  else
    return 0;
}
 
char * getLobErrStr(Lng32 errEnum)
{
  return (char*)lobErrorEnumStr[errEnum - (Lng32)LOB_MIN_ERROR_NUM];
}
