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
#ifndef EXP_LOB_INTERFACE_H
#define EXP_LOB_INTERFACE_H

#include "NAVersionedObject.h"
#include "ComQueue.h"

class HdfsFileInfo
{
 public:
  char * fileName() { return fileName_; }

  // used for text/seq file access
  Int64 getStartOffset() { return startOffset_; }
  Int64 getBytesToRead() { return bytesToRead_; }

  // used for ORC access
  Int64 getStartRow() { return startOffset_; }
  Int64 getNumRows() { return bytesToRead_; }

  Lng32 getFlags() { return flags_; }

  void setFileIsLocal(NABoolean v)
  {(v ? flags_ |= HDFSFILEFLAGS_LOCAL : flags_ &= ~HDFSFILEFLAGS_LOCAL); };
  NABoolean fileIsLocal() { return (flags_ & HDFSFILEFLAGS_LOCAL) != 0; };

  void setFileIsSplitBegin(NABoolean v)
  {(v ? flags_ |= HDFSFILE_IS_SPLIT_BEGIN : flags_ &= ~HDFSFILE_IS_SPLIT_BEGIN); };
  NABoolean fileIsSplitBegin() { return (flags_ & HDFSFILE_IS_SPLIT_BEGIN) != 0; };

  void setFileIsSplitEnd(NABoolean v)
  {(v ? flags_ |= HDFSFILE_IS_SPLIT_END : flags_ &= ~HDFSFILE_IS_SPLIT_END); };
  NABoolean fileIsSplitEnd() { return (flags_ & HDFSFILE_IS_SPLIT_END) != 0; };

  enum HdfsFileInfoFlags 
  { 
    HDFSFILEFLAGS_LOCAL          = 0x0001,
    HDFSFILE_IS_SPLIT_BEGIN      = 0x0002,
    HDFSFILE_IS_SPLIT_END        = 0x0004
  };
  Lng32 entryNum_; // 0 based, first entry is entry num 0.
  Lng32 flags_;
  NABasicPtr  fileName_;
  Int64 startOffset_;
  Int64 bytesToRead_;
};

#include "ExpLOBaccess.h"

#define LOB_ACCESS_SUCCESS 0
#define LOB_ACCESS_PREEMPT 1
enum ExpLOBinterfaceInputFlags
  {
    TRUNCATE_TGT_FILE_ =        0x0001,
    CREATE_TGT_FILE_   =        0x0002,
    ERROR_IF_TGT_FILE_EXISTS_ =  0x0004
  };

Lng32 ExpLOBinterfaceInit(void *& lobGlob, void * lobHeap, NABoolean isHive=FALSE, Int64   lobMaxSize =0);

Lng32 ExpLOBinterfaceCleanup(void *& lobGlob, void * lobHeap);

Lng32 ExpLOBinterfaceCreate(void * lobGlob, 
			    char * lobName,
			    char * lobLoc,
			    Lng32 lobType = (Lng32)Lob_HDFS_File,
			    char * lobHdfsServer = (char *)"default",
			    Int64 lobMaxSize = 0,
			    Lng32 lobHdfsPort = 0,
	                    int    bufferSize = 0,
	                    short  replication =0,
	                    int    blocksize=0);

Lng32 ExpLOBinterfaceDrop(void * lobGlob,
			  char * lobHdfsServer ,
			  Lng32 lobHdfsPort ,
			  char * lobName,
			  char * lobLoc);

Lng32 ExpLOBInterfacePurgedata(void * lobGlob, 			      
			       char * lobName,
			       char * lobLoc);

Lng32 ExpLOBinterfaceCloseFile(void * lobGlob, 
			       char * lobName,
			       char * lobLoc,
			       Lng32 lobType,
			       char * lobHdfsServer ,
			       Lng32 lobHdfsPort );
Lng32 ExpLOBInterfaceInsertSelect(void * lobGlob, 
				  char * lobHdfsServer,
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
				  Int64 srcDescTS);
Lng32 ExpLOBInterfaceInsert(void * lobGlob, 
			    char * tgtLobName,
			    char * lobLocation,
			    Lng32 lobType,
			    char * lobHdfsServer,
			    Lng32 lobHdfsPort,

			    Lng32 handleLen,
			    char * lobHandle,
			    Int32 * outHandleLen,
			    char * outLobHandle,

			    Int64 blackBoxLen,
			    char * blackBox,

			    Int64 &requestTag,
			    Int64 xnId,

			    Int64 &tgtDescSyskey,
			    LobsOper lo,

			    Lng32 * cliError = 0,

			    LobsSubOper so = Lob_Memory,
			    Lng32 waited = 0,

			    char * srcLobData = NULL, 
			    Int64  srcLobLen  = 0,
			    Int64 lobMaxSize = 0,
			    Int64 lobMaxChunkMemSize = 0,
                            Int64 lobGCLimit = 0,
			    int    bufferSize = 0,
			    short  replication =0,
			    int    blocksize=0
			    );

Lng32 ExpLOBInterfaceUpdate(void * lobGlob, 
			    char * lobHdfsServer ,
			    Lng32 lobHdfsPort,	 
			    char * tgtLobName,
			    char * lobLocation,
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
			    Int64 lobMaxSize = 0,
			    Int64 lobMaxChunkMemSize = 0,
                            Int64 lobGCLimit = 0);

Lng32 ExpLOBInterfaceUpdateAppend(void * lobGlob, 
				  char * lobHdfsServer ,
				  Lng32 lobHdfsPort ,
				  char * tgtLobName,
				  char * lobLocation,
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
				  Int64 lobMaxSize = 0,
				  Int64 lobMaxChunkMemSize = 0,
                                  Int64 lobGCLimit = 0
				  );

Lng32 ExpLOBInterfaceDelete(void * lobGlob, 
			    char * lobHdfsServer ,
			    Lng32 lobHdfsPort ,
			    char * lobName,
			    char * lobLocation,
			    Lng32 handleLen,
			    char * lobHandle,
			    Int64 &requestTag,
			    Int64 xnId,
			    Int64 descSyskey,
			    Lng32 checkStatus,
			    Lng32 waitedOp);

Lng32 ExpLOBInterfaceSelect(void * lobGlob, 
			    char * lobName, 
			    char * lobLoc,
			    Lng32  lobType,
			    char * lobHdfsServer,
			    Lng32 lobHdfsPort,

			    Lng32 handleLen,
			    char * lobHandle,
			    Int64 &requestTag,
                            LobsSubOper so,
			    Int64 xnId,
			    Lng32 checkStatus,
			    Lng32 waited,

			    Int64 offset, Int64 inLen, 
			    Int64 &outLen, char * lobData,
			    Int64 lobMaxChunkMemlen,
			    Int32 inputFlags=0);

Lng32 ExpLOBInterfaceSelectCursor(void * lobGlob, 
				  char * lobName, 
				  char * lobLoc,
				  Lng32 lobType,
				  char * lobHdfsServer,
				  Lng32 lobHdfsPort,

				  Int32 handleLen,  
				  char * lobHandle,
				  Int64 cusrorBytes,
				  char *cursorId,
				  Int64 &requestTag,
				  LobsSubOper so,
				  Lng32 checkStatus,
				  Lng32 waitedOp,

                                  Int64 offset, Int64 inLen, 
			          Int64 &outLen, char * lobData,
				  
				  Lng32 oper, // 1: open. 2: fetch. 3: close
                                  Lng32 openType // 0: not applicable. 1: preOpen. 2: mustOpen. 
				  );

Lng32 ExpLOBinterfaceStats(void * lobGlob, 
			   ExLobStats * lobStats,
			   char * lobName,
			   char * lobLoc,
			   Lng32 lobType = (Lng32)Lob_HDFS_File,
			   char * lobHdfsServer = (char *)"default",
			   Lng32 lobHdfsPort = 0);

char * getLobErrStr(Lng32 errEnum);

Lng32 ExpLOBinterfacePerformGC(void *& lobGlob, char *lobName,void *descChunksArray, Int32 numEntries, char *hdfsServer, Int32 hdfsPort,char *LOBlOC,Int64 lobMaxChunkMemSize);
Lng32 ExpLOBinterfaceRestoreLobDataFile(void *& lobGlob, char *hdfsServer, Int32 hdfsPort,char *lobLoc,char *lobName);
Lng32 ExpLOBinterfacePurgeBackupLobDataFile(void *& lobGlob,  char *hdfsServer, Int32 hdfsPort,char *lobLoc,char *lobName);

// dirPath: path to needed directory (includes directory name)
// modTS is the latest timestamp on any file/dir under dirPath.
// This method validates that current modTS is not greater then input modTS.
// Return: 1, if check fails. 0, if passes. -1, if error.
Lng32 ExpLOBinterfaceDataModCheck(void * lobGlob,
                                  char * dirPath,
                                  char * lobHdfsServer,
                                  Lng32  lobHdfsPort,
                                  Int64  modTS,
                                  Lng32  numOfPartLevels);

Lng32 ExpLOBinterfaceEmptyDirectory(void * lobGlob,
                            char * lobName,
                            char * lobLoc,
                            Lng32 lobType = (Lng32)Lob_Empty_Directory,
			    char * lobHdfsServer = (char *)"default",
                            Lng32 lobHdfsPort = 0,
                            int    bufferSize = 0,
                            short  replication =0,
                            int    blocksize=0);

/*
class HdfsFileInfo
{
 public:
  char * fileName() { return fileName_; }
  Int64 getStartOffset() { return startOffset_; }
  Int64 getBytesToRead() { return bytesToRead_; }
  Lng32 entryNum_; // 0 based, first entry is entry num 0.
  NABasicPtr  fileName_;
  Int64 startOffset_;
  Int64 bytesToRead_;
};
*/
#endif




