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
 * File:         ComExeTrace.h
 * Description:  Common interface for all executor (run-time) traces
 *
 * Created:      10/26/2011
 * Language:     C++
 *
 *
 *****************************************************************************
 */
#ifndef COMEXETRACE_H
#define COMEXETRACE_H

#include "Platform.h"
#include "NABasicObject.h"
#include "Collections.h"

// max of the trace name length and its field name length
#define MAX_TRACE_NAME_LEN 40
#define MAX_FIELD_NAME_LEN 40

// forward declaration
class ExeTrace;
class ExeTraceInfo;

//----------------------------------------------------------------------
// This part is online document on how to add your trace to executor
// runtime trace repository and be displayed by sqstate
// 
// To add trace information, the trace host class has to add/do the
// the following:
//   1. include "ComExeTrace.h"
//   2. add "void *traceRef_;" to keep the reference to the trace
//      registration in the repository
//   3. define a static wrapper function, e.g.
//        static Int32 getALine(void * mine, Int32 lineno, char * buf)
//                { return ((ExScheduler *) mine)->printALiner(lineno, buf); }
//   4. define the print method named in the wrapper function, e.g.
//        Int32 printALiner(Int32 lineno, char *buf) {...};
//      which prints all fields of one trace entry referenced by the lineno
//      to buf and returns number of bytes added to the buffer.
//   5. use the following 3 methods in the ExeTraceInfo class to register
//      and deregister the trace
// 
// That's all.
// 
// More about ExeTraceInfo methods used by trace host classes:
// 
// Use this method to register trace to global trace info repository
// Int32
// ExeTraceInfo::addTrace(const char * traceName, void * traceId,
//                       Int32 numEntries, Int32 numFields, void *target,
//                       GetALineProcPtr getALineProc, void * indexLoc,
//                       Int32 lineWidth, const char *desc,
//                       void **exeTrace)
// 
//   parameters:
//     TraceName(i) - trace name to be shown at output.
//     traceId(i) - unique reference to the trace from outside in the
//                  process, normally some address value
//     numEntries(i) - the number of rows in the trace (array size)
//                     if the number of entries is dynamic, enter -1
//     numFields(i) - the number of field for each trace entry
//     target(i) - normally target class "this" pointer
//     getALineProc(i) - pointer to the function that can obtain one 
//                       trace entry in string buffer
//     indexLoc(i) - the address to obtain current trace index, can be NULL
//     lineWidth(i) - the printed width for each trace entry in bytes
//     desc(i) - the detailed information about the trace, its value
//               will be copied to trace info repository
//     exeTrace(o) - the reference of the trace in the repository, used
//                   later by caller to add trace columns and remove trace
//   return value:
//     0 - trace added successfullly
//     -1 - trace not added, could be deplicate or other reason
//   note:
//     the exeTrace will need to be save by caller for later reference
// 
// Use this method to add trace column information,
// void
// ExeTraceInfo::addTraceField(void * exeTrace, const char * name,
//                             UInt32 fieldIdx, ExeTrace::FieldType fType)
// 
//   parameters:
//     exeTrace(i) - the input obtained when the addTrace method was called.
//     name(i) - column name.
//     fieldIdx(i) - the sequence number of the column, starting from 0.
//     fType(i) - one of the ExeTrace::FieldType enum values
//   return value: none
// 
// 
// Use this method to remove the trace info when the objects contain
// the trace are to be deleted/deallocated.
// void
// ExeTraceInfo::removeTrace(void * exeTrace)
// 
//   parameters:
//     exeTrace(i) - the input obtained when the addTrace method was called.
//   return value: none
// 
//   note:
//     to avoid invalid access to deleted trace, you must call this
//     before deleting the host instance of the trace.
// 
//----------------------------------------------------------------------

// types for pointers to methods
typedef Int32 (*GetALineProcPtr)(void *target, Int32 lineno, char *buf);

// The following is not used now because we assume that all trace array
// indexes are of type Int32:
//
// typedef Int32 (*GetIndexProcPtr) (void);

class ExeTrace
{
  friend class ExeTraceInfo;

public:
 // typedef Int32 (ExeTrace::*GetALiner)(Int32 lineno, char *buf) const;
 // typedef Int32 (ExeTrace::*GetIndex) (void) const;

  enum FieldType {
    TR_CHAR = 1,
    TR_INT16,
    TR_INT32,
    TR_INT64,
    TR_STRING,
    TR_POINTER32,
    TR_POINTER64,
    TR_TRANSID,
    TR_TMSTAMP
  };

  struct TraceField {
      char name_[MAX_FIELD_NAME_LEN];
      UInt32 nameLen_;
      FieldType fieldType_;
  };

  ExeTrace(char *name, void *id, void *target, GetALineProcPtr getALineProc,
           void *indexLoc, Int32 numEntries, Int32 lineLen,
           const char *desc);
  ~ExeTrace() {};

  inline char * getTraceName() { return name_; };
  inline void * getTraceId() { return traceId_; };
  inline Int32 getNumEntries() { return numEntries_; };
  inline Int32 getNumFields() { return numFields_; };
  inline Int32 getLineWidth() { return lineWidth_; };
  inline Int32 getIndex() { return (indexLoc_ == NULL)? -1:
                                   *((Int32 *)indexLoc_); };
  inline GetALineProcPtr getLineProc() { return getALineProc_; };
  inline void * getTarget() { return target_; };
  Int32 getTitleLineWidth();

private:
  char name_[MAX_TRACE_NAME_LEN];        // name of the trace
  void * traceId_;     // identifer
  void * target_;      // target this pointer
  GetALineProcPtr getALineProc_;    // pointer to function sending one trace
                                    // entry to given buffer
  void * indexLoc_;    // pointer to the trace index area
  Int32  numEntries_;  // size of the trace
  Int32  numFields_;   // size of the trace
  Int32  lineWidth_;   // number of bytes to sprint one trace entry
  char * describes_;
  TraceField fields_[1];
};

class ExeTraceInfo : public NABasicObject
{
  friend class ExeTrace;
public:

  // ctor
  ExeTraceInfo();
  ~ExeTraceInfo();

  // used by general public
  // create and add the trace info
  Int32 addTrace(const char * traceName, void * traceId,
                 Int32 numEntries, Int32 numFields, void *target,
                 GetALineProcPtr getALineProc, void *indexLoc,
                 Int32 lineWidth, const char *desc,
                 void **exeTrace);
  // add trace field info
  void addTraceField(void *exeTrace, const char * fieldName,
                     UInt32 fieldIdx, ExeTrace::FieldType fType);
  // unregister the given trace
  void removeTrace(void * exeTrace);

  // all kinds of helpers
  bool isRegistered(ExeTrace *trace)
                   { return exeTraces_.contains(trace) == TRUE; };
  bool isValidTraceId(void *traceId) { return true; }; // tbd

  Int32 getExeTraceInfoAll(char *outBuf, Int32 maxBufLen, Int32 *bufLen,
                          Int32 startTid=0);
  Int32 getExeTraceAll(char *outBuf, Int32 maxBufLen, Int32 *bufLen,
                      Int32 startTid=0);
  void * getExeTraceId(char *name, Int32 nameLen) const;
  Int32 getExeTraceInfoById(void *traceId, char *buf, Int32 maxBufLen,
                           Int32 *bufLen);

  Int32 getExeTraceById(void *traceId, char *buf, Int32 maxBufLen,
                       Int32 *bufLen);

private:
  LIST(ExeTrace *) exeTraces_;

};

#endif // COMEXETRACE_H
