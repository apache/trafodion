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
 * File:         ComExeTrace.cpp
 * Description:  Common interface for all executor (run-time) traces
 *
 * Created:      10/26/2011
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#include "ComExeTrace.h"

// must match with ExeTrace::FieldType, see ComExeTrace.h
static const char * TraceFieldTypeName[] =
   {
      "NOTYPE ",
      "CHAR   ",
      "INT16  ",
      "INT32  ",
      "INT64  ",
      "STRING ",
      "PTR32  ",
      "PTR64  ",
      "TRANSID",  // transaction id (64-bit)
      "TMSTAMP"   // time stamp
   };

// not really used
ExeTrace::ExeTrace(char *name, void *id, void *target,
                   GetALineProcPtr getALineProc,
                   void *indexLoc, Int32 numEntries, Int32 lineLen,
                   const char *desc)
  : traceId_(id),
    target_(target),
    getALineProc_(getALineProc),
    indexLoc_(indexLoc),
    numEntries_(numEntries),
    lineWidth_(lineLen)
{
  describes_ = (char*) malloc(strlen(desc)+1);
  strcpy(describes_, desc);
  strncpy(name_, name, MAX_TRACE_NAME_LEN - 1);
};


//
// getTitleLineWidth - returned formatted line width based on the field name
//
Int32
ExeTrace::getTitleLineWidth()
{
  Int32 len = 0;
  for (CollIndex i = 0; i < (UInt32) numFields_; i++) {
    len += (strlen(fields_[i].name_) + 1); // add 1 as field separator
  }
  return --len;  // remove the last field separator just added
};


ExeTraceInfo::ExeTraceInfo()
{
};

ExeTraceInfo::~ExeTraceInfo()
{
};

//----------------------------------------------------------------
// getExeTraceInfoAll -- query all trace info registered and save to the buffer
//
//   return value: >0 - the sequence number for next trace 
//                 =0 - no more trace info to return
//   parameters: outBuf - (i/o) give buffer to store trace info
//               maxBufLen - (i) max buffer length
//               bufLen - (o) actual buffer size filled
//               startTid - (i) starting trace info to query
//
//   The buffer contains trace information in ascii:
//
//   e.g.
//     <trace1_name> (id=<id>):
//       <trace_desc>
//       Number of entries: <value>
//       Fields:
//         <field1_name> <data_type>
//         <field2_name> <data_type>
//         ...
//     End
//
//     <trace2_name>:
//     ...
//
//----------------------------------------------------------------
Int32
ExeTraceInfo::getExeTraceInfoAll(char *outBuf, Int32 maxBufLen,
                               Int32 *bufLen, Int32 startTid)
{
  CollIndex curr = startTid;

  Int32 len = 0;
  *bufLen = 0;
  if (exeTraces_.entries() <= curr)
    return 0;  // no more traces

  for (; curr < exeTraces_.entries(); curr++)
  {
    ExeTrace *t = exeTraces_.at(curr);

    // get the size of one exeTrace info first
    Int32 totalSize = MAX_TRACE_NAME_LEN + 13 + sizeof(t->describes_) + 2;

    totalSize += 2 + sizeof("Number of entries: ") + 4;
    for (Int32 i = 0; i < t->numFields_; i++)
    {
      // 2 tabs + field name length + field type
      totalSize += 4 + t->fields_[i].nameLen_ + 8;
    }
    totalSize += 4;  // plus "End"

    if (totalSize + *bufLen > maxBufLen)
      return curr;  // buffer not large enough and return what we had

    // now start dump this trace info to the buffer
    len += sprintf((outBuf+len),
                  "%s (id=%p):\n\t%s\n\tNumber of entries: %4d\n\tFields:\n",
                  t->name_, t->traceId_, t->describes_, t->numEntries_);
    for (Int32 i = 0; i < t->numFields_; i++)
    {
      len += sprintf((outBuf+len), "\t\t%s %s\n",
                     TraceFieldTypeName[t->fields_[i].fieldType_],
                     t->fields_[i].name_);
    }

    len += sprintf((outBuf+len), "End\n");
    *bufLen = len;
  }
  
  return (Int32) curr;
};

//----------------------------------------------------------------
// getExeTraceAll -- save all registered trace data in the buffer
//
//   return value: >0 - the sequence number for next trace 
//                 =0 - no more trace info to return
//   parameters: outBuf - (i/o) give buffer to store trace info
//               maxBufLen - (i) max buffer length
//               bufLen - (o) actual buffer size filled
//               startTid - (i) starting trace id to query
//
//   The buffer contains formatted trace data one line per trace
//   entry, followed by trace index value
//
//   e.g.
//     <trace1_name> (nid=<nid>, pid=<pid>, <value> entries):
//     <field1_name> <field2_name> ...
//     <field1_value> <field2_value> ...
//     ......
//     Current Index: <index>
//
//     <trace2_name> (nid=<nid>, pid=<pid>, <value> entries):
//     ...
//
//   Note, given buffer outBuf should be big enough to hold data for
//   at least one trace
//----------------------------------------------------------------
Int32
ExeTraceInfo::getExeTraceAll(char *outBuf, Int32 maxBufLen, Int32 *bufLen,
                           Int32 startTid)
{
  *bufLen = 0;
  return startTid;
};


//----------------------------------------------------------------
// getExeTraceId -- return trace id with given trace name
//   return value: >0 - valid trace id
//                 =0 - no trace match the name
//   parameters: name - trace name
//               nameLen - length of the name
//----------------------------------------------------------------
void *
ExeTraceInfo::getExeTraceId(char *name, Int32 nameLen) const
{
  UInt32 noTraces = exeTraces_.entries();

  if (noTraces) {
    for (CollIndex i = 0; i < noTraces; i++) {
      if (strncmp(exeTraces_[i]->getTraceName(), name, nameLen) == 0)
        // a match
        return exeTraces_[i]->getTraceId();
    }
    // found no match
  }

  return 0;
};


//----------------------------------------------------------------
// getExeTraceInfoById -- query trace parameters with given trace id
//   return value: <0 - no trace info for this trace id
//                 =0 - trace info returned completely
//                 >0 - returned partial info only, buffer too small
//   parameters:
//               traceId - (i) trace id to query
//               outBuf - (i/o) give buffer to store trace info
//               maxBufLen - (i) max buffer length
//               bufLen - (o) actual buffer size filled
//
//   Note, the buffer contents have the same format as in getExeTraceInfoAll
//
//----------------------------------------------------------------
Int32
ExeTraceInfo::getExeTraceInfoById(void *traceId, char *buf,
                                Int32 maxBufLen, Int32 *bufLen)
{
  return 0;
};


//----------------------------------------------------------------
// getExeTraceById -- retreive trace data for given trace id
//   return value: <0 - no trace data for this trace id
//                 =0 - trace data returned
//                 >0 - returned partial data only, buffer too small
//   parameters:
//               traceId - (i) trace id to query
//               outBuf - (i/o) give buffer to store trace info
//               maxBufLen - (i) max buffer length
//               bufLen - (o) actual buffer size filled
//
//   The buffer contains formatted trace data one line per trace
//   entry, followed by trace index value
//
//   e.g.
//     Trace ID: <id>
//     (<trace description>)
//     <trace1_name> (<value> entries):
//     <field1_name> <field2_name> ...
//     <field1_value> <field2_value> ...
//     ......
//     Current Index: <index>
//     End
//
//----------------------------------------------------------------
Int32
ExeTraceInfo::getExeTraceById(void *traceId, char *buf, Int32 maxBufLen,
                            Int32 *bufLen)
{
  CollIndex curr = FIRST_COLL_INDEX;

  Int32 len = 0;
  *bufLen = len;  // initializing

  if (exeTraces_.entries() == 0)
  {
    return -1;  // no more traces
  }

  ExeTrace *t = 0;
  for (; curr < exeTraces_.entries(); curr++)
  {
    t = exeTraces_.at(curr);
    if (t->getTraceId() == traceId)
      break;
  }

  if (curr >= exeTraces_.entries())
    return -1;  // no such trace

  // get the size of one exeTrace info first
  // length of first and second lines, see above
  Int32 totalSize = 21 + sizeof(t->describes_) + MAX_TRACE_NAME_LEN + 51;
  // get the number of entries
  Int32 nEntries = t->getNumEntries();

  // the field name line
  for (Int32 i = 0; i < t->numFields_; i++)
  {
    // field name length + space
    totalSize += t->fields_[i].nameLen_ + 1;
  }
  totalSize += 6; // the field name line is adjusted to allow index column
  // all lines of trace entries
  totalSize += t->getLineWidth() + 1;  // plus newline char
  totalSize += 22;  // current index
  totalSize += 4;   // plus "End"

  if (totalSize + *bufLen > maxBufLen)
    return totalSize;  // buffer not large enough and return what we had

  // now start dump this trace info to the buffer
  if (nEntries >= 0)
    len += sprintf((buf+len),
                   "\nTrace ID: %p\n(%s)\n%s Trace (%d entries):\n",
                   traceId, t->describes_, t->name_, nEntries);
  else  // the number of entries for this trace is not fixed
    len += sprintf((buf+len),
                   "\nTrace ID: %p\n(%s)\n%s Trace (variable entries):\n",
                   traceId, t->describes_, t->name_);

  // field name line
  len += sprintf((buf+len), "Indx  ");
  for (Int32 i = 0; i < t->numFields_; i++)
  {
    len += sprintf((buf+len), "%s ", t->fields_[i].name_);
  }
  len += sprintf((buf+len), "\n");
  
  // print trace entries
  Int32 j = 0, plen = 0;
  do {
    plen = (t->getLineProc())(t->getTarget(), j++, (buf+len));
    len += plen;
  } while (plen);  // keep printing until exhausted

  Int32 idx = t->getIndex();
  if (idx >= 0) // idx < 0 means that the trace has no cyclic buffer
    len += sprintf((buf+len), "Current Index: %8d\n", idx);
  len += sprintf((buf+len), "End\n");
  *bufLen = len;

  return 0;
};


//----------------------------------------------------------------
// addTrace - add trace to the trace info list
//----------------------------------------------------------------
Int32
ExeTraceInfo::addTrace(const char * traceName, void * traceId,
                      Int32 numEntries, Int32 numFields, void *target,
                      GetALineProcPtr getALineProc, void * indexLoc,
                      Int32 lineWidth, const char *desc,
                      void **exeTrace)
{
  // to do: use globalheap?
  ExeTrace *t = (ExeTrace *) malloc(sizeof(ExeTrace) +
                                  numFields * sizeof(ExeTrace::TraceField));
  *exeTrace = 0;  // set to 0 initially

  // in general, we don't like any of the followings to be 0
  if (!t || !traceId || !getALineProc || !numFields || !lineWidth)
    return -1;

  // check if trace id already exists!
  if (!isValidTraceId(traceId))
    return -1;

  strncpy(t->name_, traceName, MAX_TRACE_NAME_LEN-1);
  t->name_[MAX_TRACE_NAME_LEN-1] = '\0';
  t->traceId_ = traceId;
  t->target_ = target;
  t->getALineProc_ = getALineProc;
  t->indexLoc_ = indexLoc;
  t->numEntries_ = numEntries;
  t->numFields_ = numFields;
  t->lineWidth_ = lineWidth;
  t->describes_ = (char*) malloc(strlen(desc)+1);
  strcpy(t->describes_, desc);

  // nullifies all field name and lenth
  memset((char*)(t->fields_), 0, sizeof(ExeTrace::TraceField) * t->getNumFields());

  // insert it to the list
  exeTraces_.insert(t);

  // give the trace back to caller as reference for later use
  *exeTrace = t;

  return 0;

};

void
ExeTraceInfo::addTraceField(void * exeTrace, const char * name,
                            UInt32 fieldIdx, ExeTrace::FieldType fType)
{
  ExeTrace *t = (ExeTrace*) exeTrace;
  if (!isRegistered(t))
    return;  // ignore error for now
  if (fieldIdx >= (UInt32)t->getNumFields())
    return;  // ignore error for now
  strncpy(t->fields_[fieldIdx].name_, name, MAX_FIELD_NAME_LEN-1);
  t->fields_[fieldIdx].name_[MAX_FIELD_NAME_LEN-1] = '\0';  //null terminate
  t->fields_[fieldIdx].nameLen_ = MINOF(strlen(name), MAX_FIELD_NAME_LEN-1);
  t->fields_[fieldIdx].fieldType_ = fType;
};

void
ExeTraceInfo::removeTrace(void * exeTrace)
{
  ExeTrace *t = (ExeTrace*) exeTrace;
  if (!t)
    return;
  if (!isRegistered(t))
    return;

  exeTraces_.remove(t);
  free(t->describes_);
  free(t);
};

