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
#ifndef SCRATCHFILECONN_H
#define SCRATCHFILECONN_H
/******************************************************************************
*
* File:         ScratchFileConnection.h
*
* Description:  This class is used to define all scratchfile related
*               scheduler events
* Created:      04/01/02
* Language:     C++
* Status:       $State: Exp $
*
*
*
*
******************************************************************************
*/

#include "ScratchFile.h"
#include "Ipc.h"
class ex_queue;
#include "ExScheduler.h"
class ScratchFile;

class ScratchFileConnection: public IpcConnection
{
public :
    ScratchFileConnection(Int32 index, ScratchFile *sf,
        ExSubtask *eventHandler,
        IpcEnvironment *env,
        ex_tcb *tcb,
        char *eye = (char *)eye_SCRATCH_FILE_CONNECTION);
    ~ScratchFileConnection();
    void ioStarted();
    void ioStopped();
    WaitReturnStatus wait(IpcTimeout timeout, UInt32 *eventConsumed = NULL, IpcAwaitiox *ipcAwaitiox = NULL);
    void ioError();
    short isIoOutstanding() {return ioOutstanding_; }
    NABoolean isWriteIO() {return isWriteIO_;}
    void setWriteIO(NABoolean isWrite) {isWriteIO_ = isWrite;}
    // The following methods are pure virtual methods in the base class
    // and therefore need to be redefined. They will abort the program
    // if called, though. Use only the methods above this comment.
    void send(IpcMessageBuffer *);
    void receive(IpcMessageStreamBase *);
    Int32 numQueuedSendMessages();
    Int32 numQueuedReceiveMessages(); 
    void populateDiagsArea(ComDiagsArea *&, CollHeap *);
private:
    // pointer back to the ScratchFile object
    ScratchFile *scratchFile_;
    ExSubtask *eventHandler_;
    ex_tcb *callingTcb_;
    short ioOutstanding_;
    NABoolean isWriteIO_; // Indicates if read or write IO
    Int32 fileIndex_; //Corresponds to a perticular open on the scratch file among multiple opens.
};
#endif
