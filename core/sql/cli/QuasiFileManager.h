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
#ifndef QUASIFILEMANAGER_H
#define QUASIFILEMANAGER_H

/* -*-C++-*-
******************************************************************************
*
* File:         QuasiFile.h
* Description:  This file contains definitions of the QuasiFileManager class
*               and QuasiFile class. 
*               
* Created:      3/26/2002
* Language:     C++
*
*
*
******************************************************************************
*/
#include "NoWaitOp.h"

class Statement;
class IpcEnvironment;
class ComDiagsArea;
class HashQueue;
class NoWaitOp;
class QuasiFile;
class QuasiFileManager;

// ------------------------------------------------------------------
// Classes that keep state for no-wait SQL operations
//
// One QuasiFileManager object keeps track of all SQL pseudo-files
// used by a process.
//
// One QuasiFile object exists for each SQL pseudo-file.
//
// One NoWaitOp object exists for each pending no-wait SQL operation.
//
// ------------------------------------------------------------------
#ifdef EX_GOD_H    // compile the following only if ex_god.h also included
#ifdef CLI_STDH_H  // compile the following only if CliDefs.h also included

class QuasiFileManager : public NABasicObject 
  {
  public:

    QuasiFileManager(NAHeap * noWaitHeap,IpcEnvironment * ipcEnv);
    virtual ~QuasiFileManager(void);

    RETCODE assocFileNumber(ComDiagsArea &diagsArea,short fileNumber,
			    Statement * statement);
    RETCODE disassocFileNumber(ComDiagsArea &diagsArea,
			       Statement * statement,
			       NABoolean force = FALSE);
    RETCODE deleteNoWaitOps(ComDiagsArea &diagsArea,short fileNumber,
			    Statement * statement);
    RETCODE awaitIox(Lng32 fileNumber, Lng32 * tag, short * feError);

    // returns QuasiFile object if one exists, 0 otherwise
    QuasiFile * getQuasiFile(short fileNumber);

    // get the QuasiFile if it exists, call QuasiFile::closeNoWaitOpsPending,
    // removes the QuasiFile from quasiFileList_, and deletes the QuasiFile
    void closeQuasiFile(short fileNumber);

    // called whenever a new NoWaitOp object is created
    void notifyOfNewNoWaitOp(void);

    // called whenever a NowaitOp is destroyed
    inline void notifyOfDeletedNoWaitOp(void)
      { pendingNoWaitOperations_--; };
    inline Lng32 getPendingNowaitOps() { return pendingNoWaitOperations_; }
  
  private:

    Lng32 pendingNoWaitOperations_; // number of pending operations

    // list of QuasiFile objects 
    Queue * quasiFileList_; 

    // Ipc environment
    IpcEnvironment * ipcEnv_;

    // heap used by no-wait SQL procedures
    NAHeap * noWaitHeap_;

  };

// Where methods in the QuasiFile class do not raise errors, they assume
// that the QuasiFileManager (their caller) has done all necessary
// validation.

class QuasiFile : public NABasicObject
  {
  public:

    QuasiFile(NAHeap * noWaitHeap,short fileNumber,QuasiFileManager * fnm);
    ~QuasiFile(void);

    void associateStatement(Statement * stmt);
    NABoolean disassociateStatement(Statement * stmt);
    void disableNoWaitOps(void);
    void deleteNoWaitOps(Statement * stmt);
    void closeNoWaitOpsPending();

    RETCODE awaitIox(IpcEnvironment * ipcEnv, Lng32 * tag, short * feError);

    RETCODE queueNoWaitOp(ComDiagsArea &diagsArea,
      Statement * stmt,
      Descriptor * inputDesc,
      Descriptor * outputDesc,
      NoWaitOp::opType op,
      NABoolean operationStarted,
      Lng32 tag);

    inline short getFileNumber() {return fileNumber_;};
    inline NABoolean noWaitOpsPending(void)
      { return !pendingNoWaitOps_->isEmpty(); } ;

  private:

    short fileNumber_;
    NAHeap * noWaitHeap_;
    QuasiFileManager * quasiFileManager_;

    // a list of Statement objects associated with this QuasiFile
    // in no particular order
    HashQueue * associatedStatements_;
    
    // a list of NoWaitOp objects (representing pending no-wait
    // operations), in order of initiation
    Queue * pendingNoWaitOps_;

  } ;

#endif // CLI_STDH_H
#endif // EX_GOD_H
#endif /* QUASIFILEMANAGER_H */
