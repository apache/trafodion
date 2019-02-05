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
#include <iostream>
using std::cerr;
using std::endl;

#include <fstream>
using std::ofstream;

#include "Platform.h"
#include "SQLCLIdev.h"
#include "Context.h"
#include "str.h"
#include "ExpLOBinterface.h"
#include "ex_globals.h"

Lng32 ExpLOBinterfaceInit(ExLobGlobals *& exLobGlob, NAHeap *lobHeap,
                          ContextCli *currContext,NABoolean isHiveRead,
                          char *hdfsServer, 
                          Int32 port)
{
  Ex_Lob_Error err;

  Int64 dummyParam = 0;
  Int64 cliError = -1; 
  Ex_Lob_Error status;
  Int32 dummyParam2 = 0;

  err = ExLobsOper((char*)"dummy",
                   NULL, 
		   NULL, 0,
		   NULL, 0, 
		   NULL, dummyParam2, 0, dummyParam, 
                   dummyParam, dummyParam, dummyParam, status, cliError,
		   NULL, Lob_HDFS_File,
		   NULL, 0, 
		   0,NULL,
		   Lob_Init,
		   Lob_None,
                   1, // waited op
		   exLobGlob,
		   0,
		   lobHeap, 0,
		   0);
  if (exLobGlob)
    {
      if (exLobGlob->useLibHdfs_) 
      {
        if (isHiveRead)
        {
          ((ExLobGlobals *)exLobGlob)->startWorkerThreads();
          lobHeap->setThreadSafe();
        }
      }
    }
  if (exLobGlob->useLibHdfs_)
  {
  //set hdfsConnection from context global 
  ContextCli *localContext = (ContextCli *)currContext;
  if (localContext)
    {
      hdfsFS fs = localContext->getHdfsServerConnection(hdfsServer,port);
      if (fs == NULL)
        {
          return LOB_HDFS_CONNECT_ERROR;
        }
      else
        {
          ((ExLobGlobals *)exLobGlob)->setHdfsFs(fs);
        }
    }
  }
 
  if (err != LOB_OPER_OK)
    return err;
  else
    return 0;
}

Lng32 ExpLOBinterfacePerformGC(ExLobGlobals *& exLobGlob, char *lobName,void *descChunksArray, Int32 numEntries, char *hdfsServer, Int32 hdfsPort,char *lobLoc,Int64 lobMaxChunkMemSize)
{
  Ex_Lob_Error err;
  Ex_Lob_Error status;
  Int64 dummyParam = 0;
  Int64 cliError = -1; 
  Int32 dummyParam2 = 0;
  err = ExLobsOper(lobName,
                   NULL, // ExHdfsScanStats * 
		   NULL, 0,
		   hdfsServer, hdfsPort, 
		   NULL, dummyParam2, 0, dummyParam, 
                   dummyParam, dummyParam, dummyParam, status, cliError,
		   lobLoc, Lob_HDFS_File,
                   (char *)descChunksArray, numEntries, 		  
		   0,NULL,
		   Lob_PerformGC, // Lob_GC
		   Lob_None,
                   1, // waited op
		   exLobGlob,
		   0,
		   NULL, 0
		   );
  if (err != LOB_OPER_OK)
    return err;
  else
    return 0;
}

Lng32 ExpLOBinterfaceRestoreLobDataFile(ExLobGlobals *& exLobGlob, char *hdfsServer, Int32 hdfsPort,char *lobLoc,char *lobName)
{
  Ex_Lob_Error err;
  Ex_Lob_Error status;
  Int64 dummyParam = 0;
  Int64 cliError = -1; 
  Int32 dummyParam2 = 0;
  err = ExLobsOper(lobName,
                   NULL, // ExHdfsScanStats * 
		   NULL, 0,
		   hdfsServer, hdfsPort, 
		   NULL, dummyParam2, 0, dummyParam, 
                   dummyParam, dummyParam, dummyParam, status, cliError,
		   lobLoc, Lob_HDFS_File,
		   NULL, 0,
		   0,NULL,
		   Lob_RestoreLobDataFile, // Lob_GC
		   Lob_None,
                   1, // waited op
		   exLobGlob,
		   0,
		   NULL, 0
		   );
  if (err != LOB_OPER_OK)
    return err;
  else
    return 0;
}
Lng32 ExpLOBinterfacePurgeBackupLobDataFile(ExLobGlobals *& exLobGlob, char *hdfsServer, Int32 hdfsPort,char *lobLoc,char *lobName)
{
  Ex_Lob_Error err;
  Ex_Lob_Error status;
  Int64 dummyParam = 0;
  Int64 cliError = -1; 
  Int32 dummyParam2 = 0;
  err = ExLobsOper(lobName,
                   NULL, // ExHdfsScanStats * 
		   NULL, 0,
		   hdfsServer, hdfsPort, 
		   NULL, dummyParam2, 0, dummyParam, 
                   dummyParam, dummyParam, dummyParam, status, cliError,
		   lobLoc, Lob_HDFS_File,
		   NULL, 0, 
		   0,NULL,
		   Lob_PurgeBackupLobDataFile, // Lob_GC
		   Lob_None,
                   1, // waited op
		   exLobGlob,
		   0,
		   NULL, 0
		   );
  if (err != LOB_OPER_OK)
    return err;
  else
    return 0;
}
Lng32 ExpLOBinterfaceCleanup(ExLobGlobals *& exLobGlob)
{
  Ex_Lob_Error err;
  Ex_Lob_Error status;
  Int64 dummyParam = 0;
  Int64 cliError = -1; 
  Int32 dummyParam2 = 0;
  err = ExLobsOper((char *)"dummy",
                   NULL, // ExHdfsScanStats * 
		   NULL, 0,
		   NULL, 0, 
		   NULL, dummyParam2, 0, dummyParam, 
                   dummyParam, dummyParam, dummyParam, status, cliError,
		   NULL, Lob_HDFS_File,
		   NULL, 0, 
		   0,NULL,
		   Lob_Cleanup, // Lob_Cleanup
		   Lob_None,
                   1, // waited op
		   exLobGlob,
		   0,
		   NULL, 0
		   );
  if (err != LOB_OPER_OK)
    return -err;
  else
    return 0;
}

Lng32 ExpLOBinterfaceCreate(
			    ExLobGlobals * exLobGlob, char * lobName, char * lobLoc,
			    Lng32 lobType,
			    char * lobHdfsServer,
			    Int64 lobMaxSize,
			    Lng32 lobHdfsPort,
	                    int    bufferSize ,
	                    short  replication ,
	                    int    blockSize)
{
  Ex_Lob_Error err;

  Int64 dummyParam = 0;
  Int32 dummyParam2 = 0;
  Ex_Lob_Error status;
  Int64 cliError = -1;
  
  err = ExLobsOper(lobName,
                   NULL, // ExHdfsScanStats * 
		   NULL, 0, 
		   lobHdfsServer, lobHdfsPort,
		   NULL, dummyParam2, 0, dummyParam, 
                   dummyParam, dummyParam, dummyParam, status, cliError, 
		   lobLoc, (LobsStorage)lobType, //Lob_HDFS_File,
		   NULL, 0,
		   0,NULL,
		   Lob_Create,
		   Lob_None,
                   1, // waited op
		   exLobGlob,
		   0, NULL, 0,
		   lobMaxSize,
                   bufferSize ,
                   replication,
                   blockSize
		   );

  if (err != LOB_OPER_OK)
    return -err;
  else
    return 0;
}


Lng32 ExpLOBinterfaceEmptyDirectory(
                            ExLobGlobals * exLobGlob,
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

  Int64 dummyParam=0;
  Int32 dummyParam2 = 0;
  Ex_Lob_Error status;
  Int64 cliError = -1;

  err = ExLobsOper(lobName,
                   NULL, // ExHdfsScanStats * 
                   NULL, 0,
                   lobHdfsServer, lobHdfsPort,
                   NULL, dummyParam2, 0, dummyParam,
                   dummyParam, dummyParam, dummyParam, status, cliError,
                   lobLoc, (LobsStorage)lobType, //Lob_HDFS_File,
                   NULL, 0,
		   0,NULL,
                   Lob_Empty_Directory,
                   Lob_None,
                   1, // waited op
                   exLobGlob,
                   0, NULL, 0,
                   bufferSize ,
                   replication,
                   blockSize
                   );

  if (err != LOB_OPER_OK)
    return -err;
  else
    return 0;
}
Lng32 ExpLOBinterfaceDrop(ExLobGlobals * exLobGlob,  char * lobHdfsServer ,
			  Lng32 lobHdfsPort,char * lobName, char * lobLoc)
{
  Ex_Lob_Error err;

  Int64 dummyParam = 0;
  Int32 dummyParam2 = 0;
  Ex_Lob_Error status;
  Int64 cliError = -1;
  
  err = ExLobsOper(lobName,
                   NULL, // ExHdfsScanStats * 
		   NULL, 0,
		   lobHdfsServer, lobHdfsPort, 
		   NULL, dummyParam2, 0, dummyParam, 
                   dummyParam, dummyParam, dummyParam, status, cliError,
		   lobLoc, Lob_HDFS_File,
		   NULL, 0,
		   0,NULL,
		   Lob_Drop,
		   Lob_None,
                   1, // waited op
		   exLobGlob, 
		   0, NULL, 0
		   );

  if (err != LOB_OPER_OK)
    return -err;
  else
    return 0;
}

Lng32 ExpLOBInterfacePurgedata(ExLobGlobals * exLobGlob,  
			       char * lobName, char * lobLoc)
{
  Ex_Lob_Error err;

  Int64 dummyParam=0;
  Int32 dummyParam2 = 0;
  Ex_Lob_Error status;
  Int64 cliError = -1;
  
  
  err = ExLobsOper(lobName, 
                   NULL, // ExHdfsScanStats * 
		   NULL,0, 
		   NULL, 0,
		   NULL, dummyParam2, 0, dummyParam, 
                   dummyParam, dummyParam, dummyParam, status, cliError,
		   lobLoc, Lob_HDFS_File,
		   NULL, 0,
		   0,NULL,
		   Lob_Purge,
		   Lob_None,
                   1, // waited
                   exLobGlob,
		   0, NULL, 0
		   );

  if (err != LOB_OPER_OK)
    return -err;
  else
    return LOB_ACCESS_SUCCESS;
}

Lng32 ExpLOBinterfaceCloseFile(ExLobGlobals * exLobGlob, 
                               ExHdfsScanStats *hdfsAccessStats,
			       char * lobName,
			       char * lobLoc,
			       Lng32 lobType,
			       char * lobHdfsServer,
			       Lng32 lobHdfsPort)
{
  Ex_Lob_Error err;

  Int64 dummyParam=0;
  Int32 dummyParam2 = 0;
  Ex_Lob_Error status;
  Int64 cliError = -1;
  
  LobsStorage ls = (LobsStorage)lobType;

  err = ExLobsOper(lobName, 
                   hdfsAccessStats,
		   NULL, 0, 
		   lobHdfsServer, lobHdfsPort,
		   NULL, dummyParam2, 0, dummyParam, 
                   dummyParam, dummyParam, dummyParam, status, cliError,
		   lobLoc, ls, //Lob_HDFS_File,
		   NULL, 0,
		   0,NULL,
		   Lob_CloseFile,
		  
		   Lob_None,
                   1, // waited
                   exLobGlob,
		   0, NULL, 0
		   );

  if (err != LOB_OPER_OK)
    return -err;
  else
    return LOB_ACCESS_SUCCESS;
}


Lng32 ExpLOBInterfaceInsert(ExLobGlobals * exLobGlob, 
                            ExHdfsScanStats *hdfsAccessStats,
			    char * tgtLobName,
			    char * lobStorageLocation,
			    Lng32 lobType,
			    char * lobHdfsServer,
			    Lng32 lobHdfsPort,

			    Lng32 handleLen,
			    char * lobHandle,
			    Int32 *outHandleLen,
			    char * outLobHandle,

			    Int64 blackBoxLen,
			    char * blackBox,

			    Int64 &lobHdfsDataOffset,
			    Int64 xnId,
			    Int64 &descSyskey,
			    LobsOper lo,

			    Lng32 * cliError,
			    LobsSubOper so,
			    
			    Lng32 waitedOp,
			    
			    char * srcLobData, 
			    Int64  srcLobLen,
			    Int64 lobMaxSize,
			    Int64 lobMaxChunkMemSize,
                            Int64 lobGCLimit,
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
  Int64 retOperLen = 0;
      

  LobsStorage ls = (LobsStorage)lobType;

  
    err = ExLobsOper(tgtLobName, 
                   hdfsAccessStats,
		   lobHandle, handleLen, 
		   lobHdfsServer, lobHdfsPort,
		   outLobHandle, *outHandleLen,
		   inDescSyskey, outDescSyskey, 
		   retOperLen,
		   lobHdfsDataOffset, dummyParam,
                   status, ce,
		   lobStorageLocation, ls, //Lob_HDFS_File,
		   srcLobData, srcLobLen, //strlen(srcLobData),
		   0,NULL,
		   lo, 
		   so,
                   waitedOp,
		   exLobGlob,
		   xnId, 
		   blackBox, blackBoxLen,
		   lobMaxSize,
		   lobMaxChunkMemSize,
                   lobGCLimit,
		   bufferSize,
		   replication,
		   blockSize
		   );


 
  if (err != LOB_OPER_OK)
    {
      if ((cliError) &&
	  (ce != 0))
	*cliError = (Lng32)ce;
	
      return -err;
    }

  descSyskey = outDescSyskey;
  
 
  return 0;
}

Lng32 ExpLOBInterfaceInsertSelect(ExLobGlobals * exLobGlob, 
                                  ExHdfsScanStats *hdfsAccessStats,
				  char * lobHdfsServer ,
				  Lng32 lobHdfsPort ,
				  char * tgtLobName,
                                  LobsSubOper so,
				  char * lobStorageLocation,
                                  Int32 lobType,
                                  Int64 xnId,
				   Lng32 handleLen,
				  char * lobHandle,
                                  Int32 *outHandleLen,
                                  char * outLobHandle,
                                  char * lobData, 
                                  Int64 lobLen,
                                  char * blackBox,
                                  Int64 blackBoxLen,
                                  Int64 lobMaxSize,
                                  Int64 lobMaxChunkMemSize ,
                                  Int64 lobGCLimit ,
                                  int    bufferSize ,
                                  short  replication ,
                                  int    blocksize
				  
				 )
{
  Ex_Lob_Error err;

  
  Ex_Lob_Error status;
  Int64 dummyParam=0;
  Int32 dummyParam2 = 0;
  Int64 cliError = -1;
  LobsStorage ls = (LobsStorage)lobType;
 
  err = ExLobsOper(tgtLobName, 
                   hdfsAccessStats,
		   lobHandle, handleLen, 
		   lobHdfsServer, lobHdfsPort,
                   outLobHandle, *outHandleLen,
                   dummyParam, dummyParam,
                   dummyParam,
                   dummyParam, dummyParam,
                   status,cliError,
                   lobStorageLocation, ls, //Lob_HDFS_File,
		   lobData, lobLen, 
		   0,NULL,
		   Lob_InsSel, 
		   so,
                   dummyParam,
		   exLobGlob,
		   xnId, 
		   blackBox, blackBoxLen,
		   lobMaxSize,
		   lobMaxChunkMemSize,
                   lobGCLimit,
		   bufferSize,
                   replication,
                   blocksize
		   );
  if (err != LOB_OPER_OK) 
    return -err;
    

  
  
  return 0;
}

Lng32 ExpLOBInterfaceUpdateAppend(ExLobGlobals * exLobGlob, 
                                  ExHdfsScanStats *hdfsAccessStats,
				  char * lobHdfsServer ,
				  Lng32 lobHdfsPort ,
				  char * tgtLobName,
				  char * lobStorageLocation,
				  Lng32 handleLen,
				  char * lobHandle,
				  Int32 *outHandleLen,
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
				  Int64 srcDescTS,
				  Int64 lobMaxSize,
				  Int64 lobMaxChunkMemSize,
                                  Int64 lobGCLimit
				  )
{
  Ex_Lob_Error err;

  Int64 operLen = 0;
  Int64 dummyParam = 0;
  Int64 savedTgtLobLen = tgtLobLen;
  Ex_Lob_Error status;
  Int64 cliError = -1;
  Int64 srcLen = 0;
  if(so== Lob_External_File)
    srcLen = strlen(srcLobData);
  else if ((so == Lob_Buffer) || (so == Lob_Memory))
    srcLen = tgtLobLen;
  err = ExLobsOper(tgtLobName, 
                   hdfsAccessStats,
                   lobHandle, handleLen, 
		   lobHdfsServer, lobHdfsPort, // hdfs server/port
                   outLobHandle, *outHandleLen,
		   tgtDescSyskey, dummyParam, operLen,
                   dummyParam, dummyParam, status, cliError,
                   lobStorageLocation, Lob_HDFS_File,
                   srcLobData, srcLen, //strlen(srcLobData),
		   0,NULL,
                   Lob_Append,
                   so,
                   1, 
                   exLobGlob,
                   xnId, NULL, 0,
		   lobMaxSize,
		   lobMaxChunkMemSize,
                   lobGCLimit
                   );

  
    
  if (err != LOB_OPER_OK)
    {
      return -err;
    }


  return 0;
}

Lng32 ExpLOBInterfaceUpdate(ExLobGlobals * exLobGlob, 
                            ExHdfsScanStats *hdfsAccessStats,
			    char * lobHdfsServer ,
			    Lng32 lobHdfsPort,
			    char * tgtLobName,
			    char * lobStorageLocation,
			    Lng32 handleLen,
			    char * lobHandle,
			    Int32 *outHandleLen,
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
			    Int64 srcDescTS,
			    Int64 lobMaxSize ,
			    Int64 lobMaxChunkMemSize,
                            Int64 lobGCLimit)
{
  Ex_Lob_Error err;

  Int64 operLen = 0;
  Int64 dummyParam = 0;
  Int64 savedTgtLobLen = tgtLobLen;
  Ex_Lob_Error status;
  Int64 cliError = -1;
  Int64 sourceLen = 0;
  if(so == Lob_External_File)
    sourceLen = strlen(srcLobData);
  else if ((so == Lob_Buffer) || (so == Lob_Memory))
    sourceLen = tgtLobLen;

  err = ExLobsOper(tgtLobName, 
                   hdfsAccessStats,
                   lobHandle, handleLen, 
		   lobHdfsServer, lobHdfsPort, // hdfs server/port
                   outLobHandle, *outHandleLen,
		   tgtDescSyskey, dummyParam, operLen,
                   dummyParam, dummyParam, status, cliError,
                   lobStorageLocation, Lob_HDFS_File,
                   srcLobData, sourceLen, 
		   0,NULL,
                   Lob_Update,
                   so,
                   1, 
                   exLobGlob,
                   xnId, 
		   NULL, 0,
		   lobMaxSize,
		   lobMaxChunkMemSize,
                   lobGCLimit
                   );

  
    
  if (err != LOB_OPER_OK)
    {
      return -err;
    }


  
  return 0;
}

Lng32 ExpLOBInterfaceDelete(ExLobGlobals * exLobGlob, 
                            ExHdfsScanStats *hdfsAccessStats,
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

  Int64 dummyParam=0;
  Int32 dummyParam2 = 0;
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
                   hdfsAccessStats,
		   lobHandle, handleLen, 
		   lobHdfsServer, lobHdfsPort, // hdfs server/port
                   NULL, dummyParam2, 
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
		   exLobGlob,
		   xnId, 
		   NULL, 0
		   );

    
  if (err != LOB_OPER_OK)
    {
      return -err;
    }


  return 0;

}

Lng32 ExpLOBInterfaceSelect(ExLobGlobals * exLobGlob, 
                            ExHdfsScanStats *hdfsAccessStats,
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
			    Int64 &outLen, char * lobData,
			    Int64 lobMaxMemChunkLen,
			    Int32 inputFlags
			    )
{
  Ex_Lob_Error err;
  Ex_Lob_Error status;
  Int64 dummyParam=0;
  Int32 dummyParam2 = 0;
  Int64 cliError=0;
  
  LobsOper lo;
  
   if (lobHandle == NULL)
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
                   hdfsAccessStats,
		   lobHandle, handleLen, 
		   lobHdfsServer, lobHdfsPort,
                   NULL, dummyParam2, 
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
		   exLobGlob,
		   xnId, 
		   NULL, 0,
		   0,
		   lobMaxMemChunkLen,
		   0,0,0,0,inputFlags
		   );

  
  // done with the request (success or error)
  requestTag = -1;
  if (err != LOB_OPER_OK)
    {
      return -err;
    }


  // error. Status contains the error code.
  return 0;
}

Lng32 ExpLOBInterfaceSelectCursor(ExLobGlobals * exLobGlob, 
                                  ExHdfsScanStats *hdfsAccessStats,
				  char * lobName, 
				  char * lobLoc,
				  Lng32 lobType,
				  char * lobHdfsServer,
				  Lng32 lobHdfsPort,

				  Int32 handleLen, 
				  char * lobHandle,

				  Int64 cursorBytes,
				  char *cursorId,
				  
				  Int64 &requestTag,
				  LobsSubOper so,
				  Lng32 checkStatus,
				  Lng32 waitedOp,

                                  Int64 srcOffset, Int64 inLen, 
			          Int64 &outLen, char * lobData,
				  
				  Lng32 oper, // 1: open. 2: fetch. 3: close
                                  Lng32 openType, // 0: not applicable. 1: preOpen. 2: mustOpen.
                                  Int32 *hdfsDetailError
				  )
{
  Ex_Lob_Error err;
  
  Int64 dummyParam = 0;
  Int32 dummyParam2 = 0;
  Ex_Lob_Error status;
  Int64 cliError=0;
  
  LobsOper lo;

  if (lobHandle == NULL)
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
  else
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

  if (checkStatus)
    lo = Lob_Check_Status;
  else
    requestTag = -1;

  LobsStorage ls = (LobsStorage)lobType;

  err = ExLobsOper(lobName, 
                   hdfsAccessStats,
		   lobHandle, handleLen, 
		   lobHdfsServer, lobHdfsPort,
                   NULL, dummyParam2, 
		   srcOffset, dummyParam, 
		   outLen,
                   requestTag, requestTag,
		   status, cliError, 
		   lobLoc, ls, //Lob_HDFS_File,
		   lobData, inLen, 
		   cursorBytes,cursorId,
		   lo,
		   so,
                   waitedOp,
		   exLobGlob,
		   0,
		   hdfsDetailError,0,0,0,0,0,0,0,
                   openType
		   );

  if (err != LOB_OPER_OK)
    {
      return -err;
    }
  
  return LOB_ACCESS_SUCCESS;
}
Lng32 ExpLOBInterfaceGetFileSize(ExLobGlobals * exLobGlob, 
                                 char * filename, 
                                 char * lobHdfsServer,
                                 Lng32 lobHdfsPort,
                                 Int64 &outFileSize
                                 
				  )
{
  Ex_Lob_Error err;
  
  Int64 dummyParam = 0;
  Int32 dummyParam2 = 0;
  Ex_Lob_Error status;
  Int64 cliError=0;
  
  LobsOper lo;
  LobsSubOper so;
 
 
  so = Lob_Buffer;
   err = ExLobsOper(filename, 
                   NULL, // ExHdfsScanStats * 
		   NULL, 0, 
		   lobHdfsServer, lobHdfsPort,
                   NULL, dummyParam2, 
		   dummyParam, dummyParam, 
		   outFileSize,
                   dummyParam, dummyParam,
		   status, cliError, 
		   NULL, Lob_HDFS_File,
		   NULL, 0, 
		   dummyParam,NULL,
		   Lob_GetFileSize,
		   so,
                   TRUE,
		   exLobGlob,
		   0,
		   0, 0,0,0,0,0,0,0,
                   0
		   );

  if (err != LOB_OPER_OK)
    {
      return -err;
    }
  
  return LOB_ACCESS_SUCCESS;
}
Lng32 ExpLOBInterfaceGetLobLength(ExLobGlobals * exLobGlob, 
                               char * lobName, 
                               char * lobLoc,
                               Lng32 lobType,
                               char * lobHdfsServer,
                               Lng32 lobHdfsPort,                              
                               Int32 handleLen,
                               char *lobHandle,
                               Int64 &outLobLen)
				  
{
  Ex_Lob_Error err;
  
  Int64 dummyParam = 0;
  Int32 dummyParam2 = 0;
  Ex_Lob_Error status;
  Int64 cliError=0;
  
  LobsOper lo;
  LobsSubOper so;
  LobsStorage ls = (LobsStorage)lobType;
  if (ls == Lob_External_HDFS_File)
    so = Lob_External_File;
  else 
    so = Lob_Buffer;
   err = ExLobsOper(lobName, 
                   NULL, // ExHdfsScanStats * 
		   lobHandle, handleLen, 
		   lobHdfsServer, lobHdfsPort,
                   NULL, dummyParam2, 
		   dummyParam, dummyParam, 
		   outLobLen,
                   dummyParam, dummyParam,
		   status, cliError, 
		   lobLoc, ls, //Lob_HDFS_File,
		   NULL, 0, 
		   dummyParam,NULL,
		   Lob_GetLength,
		   so,
                   TRUE,
		   exLobGlob,
		   0,
		   0, 0,0,0,0,0,0,0,
                   0
		   );

  if (err != LOB_OPER_OK)
    {
      return -err;
    }
  
  return LOB_ACCESS_SUCCESS;
}

Lng32 ExpLOBInterfaceReadSourceFile(ExLobGlobals * exLobGlob, 
                                    char * filename, 
                                    char * lobHdfsServer,
                                    Lng32 lobHdfsPort,
                                    Int64  readOffset, 
                                    Int64  allocMemSize,
                                    char *&retBuf,
                                    Int64 &retReadLen
                                    )
{
  Ex_Lob_Error err;
  
  Int64 dummyParam = 0;
  Int32 dummyParam2 = 0;
  Ex_Lob_Error status;
  Int64 cliError=0;
  
  LobsOper lo;
  LobsSubOper so= Lob_Buffer;
   err = ExLobsOper(filename, 
                   NULL, // ExHdfsScanStats * 
		   NULL, 0, 
		   lobHdfsServer, lobHdfsPort,
                   NULL, dummyParam2, 
		   readOffset, allocMemSize, 
		   retReadLen,
                    dummyParam, (Int64 &)retBuf,
		   status, cliError, 
		   NULL, Lob_HDFS_File,
		   NULL, dummyParam, 
		   dummyParam,NULL,
		   Lob_ReadSourceFile,
		   so,
                   TRUE,
		   exLobGlob,
		   0,
		   0,0,0,0,0,0,0,0,
                   0
		   );

  if (err != LOB_OPER_OK)
    {
      return -err;
    }
  
  return LOB_ACCESS_SUCCESS;
}

Lng32 ExpLOBInterfaceGetOffset(ExLobGlobals * exLobGlob, 
				  char * lobName, 
				  char * lobLoc,
				  Lng32 lobType,
				  char * lobHdfsServer,
				  Lng32 lobHdfsPort,
				  Int32 handleLen, 
				  char * lobHandle,
			          Int64 &outLobOffset
                                 
				  )
{
  Ex_Lob_Error err;
  
  Int64 dummyParam = 0;
  Int32 dummyParam2 = 0;
  Ex_Lob_Error status;
  Int64 cliError=0;
  
  LobsOper lo;
  LobsSubOper so;
  LobsStorage ls = (LobsStorage)lobType;
  if (ls == Lob_External_HDFS_File)
    {
      so = Lob_External_File;
      outLobOffset = 0;
      return LOB_ACCESS_SUCCESS;
    }
  else 
    so = Lob_Buffer;
  err = ExLobsOper(lobName, 
                   NULL, // ExHdfsScanStats * 
		   lobHandle, handleLen, 
		   lobHdfsServer, lobHdfsPort,
                   NULL, dummyParam2, 
		   dummyParam, dummyParam, 
		   outLobOffset,
                   dummyParam, dummyParam,
		   status, cliError, 
		   lobLoc, ls, //Lob_HDFS_File,
		   NULL, 0, 
		   dummyParam,NULL,
		   Lob_GetOffset,
		   so,
                   TRUE,
		   exLobGlob,
		   0,
		   0, 0,0,0,0,0,0,0,
                   0
		   );

  if (err != LOB_OPER_OK)
    {
      return -err;
    }
  
  return LOB_ACCESS_SUCCESS;
}

Lng32 ExpLOBInterfaceGetFileName(ExLobGlobals * exLobGlob, 
				  char * lobName, 
				  char * lobLoc,
				  Lng32 lobType,
				  char * lobHdfsServer,
				  Lng32 lobHdfsPort,
				  Int32 handleLen, 
				  char * lobHandle,
                                  char * fileName,
			          Int32 &outFileLen
                                 
				  )
{
  Ex_Lob_Error err;
  
  Int64 dummyParam = 0;
  Int32 dummyParam2 = 0;
  Ex_Lob_Error status;
  Int64 cliError=0;
  Int64 hdfsFileLen = 0;
  LobsOper lo;
  LobsSubOper so;
  LobsStorage ls = (LobsStorage)lobType;
  if (ls == Lob_External_HDFS_File)
    so = Lob_External_File;
  else 
    so = Lob_Buffer;
  err = ExLobsOper(lobName, 
                   NULL, // ExHdfsScanStats * 
		   lobHandle, handleLen, 
		   lobHdfsServer, lobHdfsPort,
                   NULL, dummyParam2, 
		   dummyParam, dummyParam, 
		   dummyParam,
                   dummyParam, dummyParam,
		   status, cliError, 
		   lobLoc, ls, //Lob_HDFS_File,
		   NULL, 0, 
		   dummyParam,NULL,
		   Lob_GetFileName,
		   so,
                   TRUE,
		   exLobGlob,
		   0,
		   fileName, outFileLen,
                   0,0,0,0,0,0,
                   0
		   );

  if (err != LOB_OPER_OK)
    {
      return -err;
    }
  
  return LOB_ACCESS_SUCCESS;
}
 
char * getLobErrStr(Lng32 errEnum)
{
  if (errEnum < LOB_MIN_ERROR_NUM || errEnum > LOB_MAX_ERROR_NUM)
    return (char *)"Unknown LOB error";
else
  return (char*)lobErrorEnumStr[errEnum - (Lng32)LOB_MIN_ERROR_NUM];
}
