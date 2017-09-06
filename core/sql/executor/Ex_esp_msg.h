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
#ifndef EX_ESP_MSG_H
#define EX_ESP_MSG_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ex_esp_msg.h
 * Description:  Requests and replies exchanged between master executor and
 *               ESPs and dependent objects sent in them.
 *
 * Created:      1/22/96
 * Language:     C++
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "Platform.h"

#include "Int64.h"
#include "ex_frag_inst.h"
#include "sql_buffer.h"
// -----------------------------------------------------------------------
// Contents of this file
// -----------------------------------------------------------------------

// enum ESPMessageTypeEnum
// enum ESPMessageObjTypeEnum

// The NSK compiler doesn't like forward declarations of "struct" and
// the NT compiler doesn't like forward "class" followed by "struct".
// Sigh.
#define FWD_STRUCT struct

class ExEspMsgObj;
class ExEspRequestHeader;
class ExEspReplyHeader;
FWD_STRUCT ExEspInputDataReqHeader;
FWD_STRUCT ExEspContinueReqHeader;
FWD_STRUCT ExEspReturnDataReplyHeader;
FWD_STRUCT ExEspLoadFragmentReqHeader;
FWD_STRUCT ExEspFixupFragmentReqHeader;
FWD_STRUCT ExEspReleaseFragmentReqHeader;
FWD_STRUCT ExEspOpenReqHeader;
FWD_STRUCT ExEspPartInputDataReqHeader;
FWD_STRUCT ExEspWorkReqHeader;
FWD_STRUCT ExEspReleaseWorkReqHeader;
FWD_STRUCT ExEspReturnStatusReplyHeader;
class TupMsgBuffer;
class ExProcessIdsOfFrag;
class ExProcessIdsOfFragList;
class ExResolvedNameObj;
class ExMsgFragment;
class ExMsgTransId;
class ExMsgTimeouts;
class ExMsgResourceInfo;
class ExMsgSecurityInfo;
class ExSMDownloadInfo;

// -----------------------------------------------------------------------
// Forward references
// -----------------------------------------------------------------------
class TimeoutData;

// -----------------------------------------------------------------------
// Enum type for message streams used by ESPs
// (see IpcMessageTypeEnum in file IpcMessageType.h)
// -----------------------------------------------------------------------
enum ESPMessageTypeEnum
{
  IPC_MSG_SQLESP_LOW = IPC_MSG_SQLESP_FIRST, // from IpcMessageType.h
  IPC_MSG_SQLESP_CONTROL_REQUEST,
  IPC_MSG_SQLESP_CONTROL_REPLY,
  IPC_MSG_SQLESP_DATA_REQUEST,
  IPC_MSG_SQLESP_DATA_REPLY,
  IPC_MSG_SQLESP_CANCEL_REQUEST,
  IPC_MSG_SQLESP_CANCEL_REPLY,
  IPC_MSG_SQLESP_SERVER_ROUTING,
  IPC_MSG_SQLESP_SERVER_INCOMING,
  // ---------> must add any new entries just above this line!! <---------
  IPC_MSG_SQLESP_HIGH
};

const char *getESPStreamTypeString(ESPMessageTypeEnum t);

const IpcMessageObjVersion CurrEspRequestMessageVersion = 100;
const IpcMessageObjVersion CurrEspReplyMessageVersion = 100;

// -----------------------------------------------------------------------
// Request and reply types sent between ESPs
//
// - An input data request contains down queue entries from the parent
//   node of a fragment.
//   The server takes the down queue entries, passes them on to its
//   child and returns the results in a return data reply.
//
//   Objects following the input data request:
//   -> 1 ESP_INPUT_SQL_BUFFER object
//
// - If a server got an input data request and has more reply data than
//   fits in one reply, it replies with some of the data (the parent
//   detects that from the missing end of data up entry). The parent
//   (client) then sends a continue request down to the server. The
//   continue message can also carry a transaction id.
//
//   Objects following the continue request:
//   -> none
//
// - The return data reply is the reply to an input data or a continue
//   request and contains up queue entries.
//
//   Objects following the return data reply:
//   -> 1 ESP_OUTPUT_SQL_BUFFER object
//   -> 0 or more ESP_DIAGNOSTICS_AREA objects
//
// - The load fragment request comes from the process that manages the
//   ESP and it is a request to install a fragment instance on the ESP
//   and to assign a fragment instance id to it. Unlike most other
//   messages, this one does not contain a complete fragment instance id.
//   The ESP replies with a status that contains that id.
//
//   Objects following the load fragment request:
//   -> 1 or more ESP_FRAGMENT objects
//
// - The fixup request requests fixup of a previously downloaded fragment.
//
//   Objects following the fixup request:
//   -> 1 or more ESP_PROCESS_IDS_OF_FRAG objects
//   -> 0 or 1 ESP_LATE_NAME_INFO objects
//   -> 0 or more ESP_RESOURCE_INFO objects
//   -> 0 or 1 ESP_TIMEOUT_DATA object
//   -> 0 or 1 ESP_SM_DOWNLOAD_INFO objects
//
// - The release request requests complete release of a downloaded
//   fragment. The fragment instance id becomes invalid after release.
//
//   Objects following the release request:
//   -> none
//
// - Partition input data requests come from either the parent node or
//   the ESP manager process. They contain the partition input data which
//   tells an ESP which partition to work on. Partition input data does
//   not contain down queue status neither does it contain a transaction id.
//   There are 3 flavors of partition input data requests: a) a static
//   assignment that lasts until cancelled b) a dynamic assignment that
//   lasts until an end-of-data is received from the child of the
//   split bottom node and that requires it to ask for more data, and c)
//   an indication that there are no more dynamic assignments and that
//   the split bottom node may return end-of-data to its consumer ESPs.
//
//   Objects following the partition input data request:
//   -> 0 or 1 ESP_INPUT_SQL_BUFFER objects
//
// - Work requests may carry a transaction id with them and
//   allow the ESP to do work on behalf of the transaction. The ESP usually
//   does not immediately reply to such requests. It waits until either
//   a time limit has expired, until it gets a work release request,
//   or until it is done with its current work (end-of-data from child).
//   The work request comes from the master executor and is processed by
//   the split bottom node in the ESP.
//
//   Objects following the transaction work request:
//   -> 0 or 1 ESP_TRANSID objects
//
// - Work release requests signal that the caller wants to have its
//   work request back. The ESP replies to two requests when it gets
//   such a message, to the previous work request and to the
//   work release request.
//
//   Objects following the work release request:
//   -> none
//
// - The return status reply is the standard reply to requests that
//   come from the ESP manager (all requests except input data, and
//   continue, which expect a return data reply). The status reply
//   contains a diagnostics area.
//
//   Objects following the return status reply:
//   -> 0 or 1 ESP_DIAGNOSTICS_AREA objects
//   -> 0 or 1 ESP_STATISTICS objects
//
// - Messages can piggyback on other messages. This is done by including
//   multiple request headers in a single message buffer. Examples:
//
//   -> load + fixup
//   -> partition input data (static) + work
//   -> ...
//
// - Each request type has an associated header that contains information
//   global to the message, such as an error status, number of rows, etc.
//
// - The request objects are declared as structs rather than classes, this
//   means that all their members are public by default. Request objects
//   are sent over the wire and different processes may use them on each
//   side. We do not want the methods to receive a message to be exported
//   to the sender process. Therefore we avoid having any methods on the
//   request and reply class, instead those methods are implemented in
//   the callback functions of IpcMessageStream objects that are used to
//   send and receive these requests.
//
// Objects sent in ESP messages for different requests:
//
// - Input SQL buffers are objects of type sql_buffer, containing two
//   tupps for each row sent: the first tupp contains the down state
//   from the input queue and the second tupp contains the actual data
//   row. In other words, there is always an even number of tupps in
//   the buffer. The format is the same as in the protocol between
//   executor, filesystem, and DP2. See class TupMsgBuffer.
//
// - Output SQL buffers are the same as input SQL buffers, except that
//   they contain up states and rows in two separate tupps per returned
//   row. See class TupMsgBuffer.
//
// - Diagnostics areas are a compressed form of the SQL diagnostics area.
//
// - Statistics are a compressed form of the SQLSA.
//
// - Fragments are the tdbs and expressions generated by the SQL compiler
//   for a given ESP. A fragment has to be unpacked and fixed up before
//   it can be executed in the ESP. See class ExFragmentData in file
//   ex_load_esp.h.
//
// - Fixup messages carry lists of process ids for each input fragment
//   of the fragment to be fixed up, see class ExProcessIdsOfFrag.
//
// - Transaction ids encapsulate NSK transactino ids (maybe others
//   someday). Right now they are for aestetic reasons only, to symbolize
//   the fact that NSK sends the transaction id automatically.
// -----------------------------------------------------------------------
enum ESPMessageObjTypeEnum
{
  ESP_INPUT_DATA_HDR                 = 101,
  ESP_CONTINUE_HDR                   = 102,
  ESP_RETURN_DATA_HDR                = 103,
  ESP_LOAD_FRAGMENT_HDR              = 104,
  ESP_FIXUP_FRAGMENT_HDR             = 106,
  ESP_RELEASE_FRAGMENT_HDR           = 107,
  ESP_OPEN_HDR                       = 108,
  ESP_PARTITION_INPUT_DATA_HDR       = 110,
  ESP_WORK_TRANSACTION_HDR           = 111,
  ESP_RELEASE_TRANSACTION_HDR        = 112,
  ESP_CANCEL_HDR                     = 113,
  ESP_LATE_CANCEL_HDR                = 114,
  ESP_RETURN_STATUS_HDR              = 120,
  ESP_RETURN_CANCEL_HDR              = 121,

  ESP_FRAGMENT_KEY                   = 140,
  ESP_INPUT_SQL_BUFFER               = 151,
  ESP_OUTPUT_SQL_BUFFER              = 152,
  ESP_PROCESS_IDS_OF_FRAG            = 160,
  ESP_LATE_NAME_INFO                 = 161,
  ESP_DIAGNOSTICS_AREA               = IPC_SQL_DIAG_AREA,
  ESP_STATISTICS                     = 185,
  ESP_FRAGMENT                       = 190,
  ESP_TRANSID                        = 195,
  ESP_TIMEOUT_DATA                   = 198,
  ESP_SM_DOWNLOAD_INFO               = 199,
  ESP_RESOURCE_INFO                  = 200,
  ESP_SECURITY_INFO                  = 220
};

const char *getESPMessageObjTypeString(ESPMessageObjTypeEnum t);

const IpcMessageObjVersion CurrInputDataHdrVersion = 100;
const IpcMessageObjVersion CurrContinueHdrVersion = 100;
const IpcMessageObjVersion CurrReturnDataHdrVersion = 100;
const IpcMessageObjVersion CurrLoadFragmentHdrVersion = 100;
const IpcMessageObjVersion CurrFixupFragmentHdrVersion = 100;
const IpcMessageObjVersion CurrReleaseFragmentHdrVersion = 100;
const IpcMessageObjVersion CurrOpenHdrVersion = 100;
const IpcMessageObjVersion CurrReturnStatusHdrVersion = 100;
const IpcMessageObjVersion CurrPartitionInputDataHdrVersion = 100;
const IpcMessageObjVersion CurrWorkTransactionHdrVersion = 100;
const IpcMessageObjVersion CurrReleaseTransactionHdrVersion = 100;

const IpcMessageObjVersion CurrFragmentKeyVersion = 100;
const IpcMessageObjVersion CurrInputSqlBufferVersion = 100;
const IpcMessageObjVersion CurrOutputSqlBufferVersion = 100;
const IpcMessageObjVersion CurrProcessIdsOfFragVersion = 100;
const IpcMessageObjVersion CurrResolvedNameObjVersion = 101;
const IpcMessageObjVersion Pre1800ResolvedNameObjVersion = 100;
const IpcMessageObjVersion CurrDiagnosticsAreaVersion =
                                            IpcCurrSqlDiagnosticsAreaVersion;
const IpcMessageObjVersion CurrStatisticsVersion = 100;
const IpcMessageObjVersion CurrFragmentVersion = 100;
const IpcMessageObjVersion CurrTransidVersion = 100;
const IpcMessageObjVersion CurrTimeoutDataVersion = 100;
const IpcMessageObjVersion CurrSMDownloadInfoVersion = 100;
const IpcMessageObjVersion CurrResourceInfoVersion = 100;
const IpcMessageObjVersion CurrCancelHdrVersion = 100;
const IpcMessageObjVersion CurrCancelReplyHdrVersion = 100;
const IpcMessageObjVersion CurrLateCancelHdrVersion = 100;

// When we send a boolean over the network, it gets sent as a long
// (just to hedge ourselves against a possible redefinition of this type)
typedef Int32 ExIpcMsgBoolean;

// -----------------------------------------------------------------------
// Base class for all objects sent to an ESP
// -----------------------------------------------------------------------
class ExEspMsgObj : public IpcMessageObj
{
public:
   ExEspMsgObj(ESPMessageObjTypeEnum objType,
		     IpcMessageObjVersion objVersion,
		     NAMemory *heap) :
       IpcMessageObj(objType,objVersion), heap_(heap) {}

  ExEspMsgObj(IpcBufferedMsgStream* msgStream) :
       IpcMessageObj(msgStream), heap_(NULL) {}

  // the real reason for the existence of this object: manage the heap
  void operator delete(void *p);


  // accessor method (mostly for derived classes)
  inline NAMemory * getHeap() const                  { return heap_; }

  // Override some virtual functions from IpcMessageObj, so that the
  // heap_ member does not get sent in a message, that it does not
  // get overwritten when an object is unpacked, and that our operator
  // delete (see above) is called when an object is no longer needed.
  virtual IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);
  virtual void unpackObj(IpcMessageObjType objType,
			 IpcMessageObjVersion objVersion,
			 NABoolean sameEndianness,
			 IpcMessageObjSize objSize,
			 IpcConstMessageBufferPtr buffer);
  virtual IpcMessageRefCount decrRefCount();

private:
  // the heap on which this object is allocated on
  // (a NULL pointer for the heap indicates that the object is allocated
  // directly inside a message buffer with the copyless IPC protocol)
  NAMemory * heap_;       // not really used when sending the object, but
  void * fillerFor64Bit_; // a filler for 64 bits may someday avoid a copy
};

// -----------------------------------------------------------------------
// Base class for all request messages to ESPs
// -----------------------------------------------------------------------
class ExEspRequestHeader : public ExEspMsgObj
{
public:

  // constructor
  ExEspRequestHeader(ESPMessageObjTypeEnum objType,
		     IpcMessageObjVersion objVersion,
		     NAMemory *heap);

  // constructor used to perform copyless receive, maps packed objects in place
  ExEspRequestHeader(IpcBufferedMsgStream* msgStream);
};

// -----------------------------------------------------------------------
// Base class for all reply messages from ESPs
// -----------------------------------------------------------------------
class ExEspReplyHeader : public ExEspMsgObj
{
public:

  // constructor
  ExEspReplyHeader(ESPMessageObjTypeEnum objType,
		   IpcMessageObjVersion objVersion,
		   NAMemory *heap);

  // constructor used to perform copyless receive, maps packed objects in place
  ExEspReplyHeader(IpcBufferedMsgStream* msgStream);
};

// -----------------------------------------------------------------------
// Input data request:
//
// The parent node of a fragment instance sends down one or more down
// queue entries and expects the ESP to start or continue working on the
// instance.
//
// Objects that follow this header:
//
// An input TupMsgBuffer.
// -----------------------------------------------------------------------
struct ExEspInputDataReqHeader : public ExEspRequestHeader
{
  ExEspInputDataReqHeader(NAMemory *heap);

  // constructor used to perform copyless receive, maps packed objects in place
  ExEspInputDataReqHeader(IpcBufferedMsgStream* msgStream);

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();

  // data members

  ExFragInstanceHandle handle_;
  Int32  myInstanceNum_;
  char   endianness_; // big-endian, little endian
  char   spare1_;
  Int16  spare2_;
  UInt32 injectErrorAtQueueFreq_;
  Int32  spare3_, spare4_, spare5_;
};

// -----------------------------------------------------------------------
// Continue request:
//
// An ESP has replied with a return data reply, but there are still
// outstanding up queue entries from the ESP (there are still down queue
// entries that haven't received end of data yet). This request is to
// redrive the ESP to continue working. For slow cursors, no work will
// be done by the ESP between its reply and receiving the continue
// request.
//
// Objects that follow this header:
//
// None.
// -----------------------------------------------------------------------
struct ExEspContinueReqHeader : public ExEspRequestHeader
{
  ExEspContinueReqHeader(NAMemory *heap);

  // constructor used to perform copyless receive, maps packed objects in place
  ExEspContinueReqHeader(IpcBufferedMsgStream* msgStream);

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();

  // data members

  ExFragInstanceHandle handle_;
  Int32 myInstanceNum_;
  char endianness_; // big-endian, little endian
  char spare1_;
  Int16 spare2_;
  Int32 spare3_;
};

// -----------------------------------------------------------------------
// Return data reply:
//
// An ESP replies to an input data request or a continue request with up
// queue entries. None, some, or all reply entries may be included in the
// message.
//
// Objects that follow this header:
//
// An output TupMsgBuffer and a SQL Diagnostics area.
// -----------------------------------------------------------------------
struct ExEspReturnDataReplyHeader : public ExEspReplyHeader
{
  ExEspReturnDataReplyHeader(NAMemory *heap);

  // constructor used to perform copyless receive, maps packed objects in place
  ExEspReturnDataReplyHeader(IpcBufferedMsgStream* msgStream);

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();

  // data members

  ExIpcMsgBoolean stopSendingData_; // server saturated, continue requests only
  char endianness_; // big-endian, little endian
  char spare1_;
  Int16 spare2_;
  Int32 spare3_, spare4_;
};

// -----------------------------------------------------------------------
// Load fragment request:
//
// A process requests to download a fragment and to assign a fragment
// handle to it.
//
// Objects that follow this header:
//
// An ExFragmentData object (see file ex_load_esp.h).
// -----------------------------------------------------------------------
struct ExEspLoadFragmentReqHeader : public ExEspRequestHeader
{
  ExEspLoadFragmentReqHeader(NAMemory *heap);

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();

  Int32 spare1_;
  Int32 spare2_;
};

// -----------------------------------------------------------------------
// Perform fixup on a downloaded fragment:
//
// The master executor requests fixup of a downloaded fragment, which
// causes the input ESPs to be opened,
//
// Objects that follow this header:
//
// One or more ExProcessIdsOfFrag objects.
// Zero or 1 ExResolvedNameObj objects
// Zero or more ExMsgResourceInfo objects.
// Zero or 1 ExMsgTimeoutData object.
// Zero or 1 ExSMDownloadInfo objects
// -----------------------------------------------------------------------
struct ExEspFixupFragmentReqHeader : public ExEspRequestHeader
{
  enum Flags
  {
    // indicates that stat collection at runtime is enabled.
    // Set when fixup request is sent from master to esps.
    // Used to collect stats info at fixup time (like num opens).
    STATS_ENABLED = 0x0001,
    // indicates that a connection "error" due to receipt
    // of close message while a request is outstanding
    // (not replied to) should be logged to EMS
    ESP_CLOSE_ERROR_LOGGING = 0x0002,
    PERSISTENT_OPENS_1 = 0x0004,
    PERSISTENT_OPENS_2 = 0x0008
  };

  ExEspFixupFragmentReqHeader(NAMemory *heap);

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();

  NABoolean statsEnabled() {return (flags_ & STATS_ENABLED) != 0;};
  void setStatsEnabled(NABoolean v)
  { (v ? flags_ |= STATS_ENABLED : flags_ &= ~STATS_ENABLED); };
  void setPersistentOpens(Lng32 v)
  {
    switch (v)
    {
    case 0:
      flags_ &= ~(PERSISTENT_OPENS_1 || PERSISTENT_OPENS_2);
      break;
    case 1:
      flags_ |= PERSISTENT_OPENS_1;
      break;
    case 2:
      flags_ |= PERSISTENT_OPENS_2;
      break;
    }
  }
  Lng32 getPersistentOpens()
  {
    if (flags_ & PERSISTENT_OPENS_1)
      return 1;
    else if (flags_ & PERSISTENT_OPENS_2)
      return 2;
    else
      return 0;
  }

  void setEspFixupPriority(IpcPriority p)
  {
    espFixupPriority_ = (short)p;
  }
  IpcPriority getEspFixupPriority()
  {
    return (IpcPriority)espFixupPriority_;
  }

  void setEspExecutePriority(IpcPriority p)
  {
    espExecutePriority_ = (short)p;
  }
  IpcPriority getEspExecutePriority()
  {
    return (IpcPriority)espExecutePriority_;
  }
  void setMaxPollingInterval(Lng32 p)
  {
    maxPollingInterval_ = p;
  }
  Lng32 getMaxPollingInterval()
  {
    return maxPollingInterval_;
  }

  void setEspFreeMemTimeout(Lng32 p)
  {
    espFreeMemTimeout_ = p;
  }
  Lng32 getEspFreeMemTimeout()
  {
    return espFreeMemTimeout_;
  }
  void setEspCloseErrorLogging(NABoolean v)
  { (v ? flags_ |= ESP_CLOSE_ERROR_LOGGING : flags_ &= ~ESP_CLOSE_ERROR_LOGGING); };
  NABoolean getEspCloseErrorLogging() {return (flags_ & ESP_CLOSE_ERROR_LOGGING) != 0;};


  // data members

  ExFragKey        key_;
  Lng32             numOfParentInstances_;

  UInt32           flags_;

  // priority that ESP should execute the stmt at.
  // ESP changes its own priority to this value after 'fixup' stage.
  // Once the query is finished after release fragment request, priority is
  // changed back to the original fixup priority value which is saved
  // in esp stmt globals after fixup stage.
  short            espExecutePriority_;
  short            espFixupPriority_;

  Lng32             maxPollingInterval_;
  Lng32             espFreeMemTimeout_;
};

// -----------------------------------------------------------------------
// Release a downloaded fragment instance:
//
// The master executor indicates that it no longer needs a downloaded
// fragment. It is ok to release all the resources it currently uses.
//
// Objects that follow this header:
//
// None.
// -----------------------------------------------------------------------
struct ExEspReleaseFragmentReqHeader : public ExEspRequestHeader
{
  enum Flags
  {
    // delete all relatives within stmt as well?
    DELETE_STMT = 0x0001,

    // do real close of all tables instead reusing opens
    CLOSE_ALL_OPENS = 0x0002
  };

  ExEspReleaseFragmentReqHeader(NAMemory *heap);

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();

  NABoolean deleteStmt() {return (flags_ & DELETE_STMT) != 0;};
  void setDeleteStmt(NABoolean v)
  { (v ? flags_ |= DELETE_STMT : flags_ &= ~DELETE_STMT); };

  NABoolean closeAllOpens() {return (flags_ & CLOSE_ALL_OPENS) != 0;};
  void setCloseAllOpens(NABoolean v)
  { (v ? flags_ |= CLOSE_ALL_OPENS : flags_ &= ~CLOSE_ALL_OPENS); };

  // data members

  ExFragKey       key_;
  Int32           idleTimeout_;
  UInt32          flags_;
  ExIpcMsgBoolean detachFromMaster_; // does master need my service any more?
  Int32           spare1_;
  Int32           spare2_;
};

// -----------------------------------------------------------------------
// A parent fragment instance tries to open this ESP and is giving the
// identity of the fragment instance it tries to reach. Note that the
// actual identity of the parent ESP has to be verified by calling an
// operating system service, just like for most other request messages.
// In Guardian that can be done by having OPEN system messages delivered.
//
// Objects that follow this header:
//
// None.
// -----------------------------------------------------------------------
struct ExEspOpenReqHeader : public ExEspRequestHeader
{
  ExEspOpenReqHeader(NAMemory *heap);

  // constructor used to perform copyless receive, maps packed objects in place
  ExEspOpenReqHeader(IpcBufferedMsgStream* msgStream);

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();

  // typedef for open types
  typedef enum
  {
    NORMAL = 0,
    PARALLEL_EXTRACT
  } ExIpcOpenTypes;

  // method to get/set open types
  ExIpcOpenTypes getOpenType() { return (ExIpcOpenTypes) openType_; }
  void setOpenType(ExIpcOpenTypes type) { openType_ = type; }

  // data members

  ExFragKey            key_;
  // the parent need to say which instance number it has (for repartitioning)
  Int32                myInstanceNum_;
  char                 endianness_; // big-endian, little endian
  char                 openType_;   // type of open
  Int16                spare2_;
  Int64                statID_; // ID assigned to parent stat entry.
  Int32                spare3_;
  Int32                spare4_;
};

// -----------------------------------------------------------------------
// A parent ESP, master executor, or a third process is sending partition
// input data. This identifies the partition this particular instance is
// supposed to work on. Partition input data is passed on to other nodes
// (especially file scan nodes) in the fragment. If no TupMsgBuffer
// follows this request then there is no (more) work to do for the
// fragment instance identified by <key_>.
//
// Objects that follow this header:
//
// An input TupMsgBuffer.
// -----------------------------------------------------------------------
struct ExEspPartInputDataReqHeader : public ExEspRequestHeader
{
  ExEspPartInputDataReqHeader(NAMemory *heap);

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();

  // data members

  ExFragKey        key_;

  // This flag indicates whether the ESP may get more partitions assigned
  // to it dynamically or whether this assignment is permanent over the
  // lifetime of the fragment instance. The ESP will reply immediately
  // to this request if staticAssigment_ is set to TRUE, it will
  // reply after it is done working on the request if staticAssignment_
  // is set to FALSE. Once a static assignment is set, its remains in
  // place indefinitely.
  ExIpcMsgBoolean  staticAssignment_;

  // This boolean data member indicates whether the ESP should do work
  // as a result of receiving this request or whether the request is
  // simply sent because there is no more work for the ESP. The latter
  // happens if work is assigned dynamically and the ESP keeps asking
  // for more assignments.
  ExIpcMsgBoolean  askForMoreWorkWhenDone_;

  Int32 spare1_;
  Int32 spare2_;
};

// -----------------------------------------------------------------------
// The master executor or a third process is sending a transaction id
// to be used for a certain fragment instance. The ESP may reply to this
// request in periodic intervals, it must reply if it gets an
// ExEspReleaseWorkReqHeader request. Once it has replied, the
// transaction becomes inactive and can no longer be used to work.
//
// Objects that follow this header:
//
// An ExMsgTransId object.
// -----------------------------------------------------------------------
struct ExEspWorkReqHeader : public ExEspRequestHeader
{
  ExEspWorkReqHeader(NAMemory *heap);

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();

  // data members

  ExFragKey        key_;
  Int32            spare1_;
  Int32            spare2_;
};

// -----------------------------------------------------------------------
// The master executor or a third process is requesting the ESP to
// release a previously sent transaction id (via ExEspWorkReqHeader)
// and to reply to this and to the ExEspWorkReqHeader request.
//
// Objects that follow this header:
//
// None.
// -----------------------------------------------------------------------
struct ExEspReleaseWorkReqHeader : public ExEspRequestHeader
{
  ExEspReleaseWorkReqHeader(NAMemory *heap);

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();

  // data members

  ExFragKey        key_;
  ExIpcMsgBoolean  allWorkRequests_;
  Int32            inactiveTimeout_;
  Int32            spare1_;
};

// -----------------------------------------------------------------------
// Cancel request:
//
// The parent node of a fragment instance sends down one or more down
// queue entries.  The queue entries are used by send_bottom in the ESP
// to identify canceled queue entries by their parent index.
//
// Objects that follow this header:
//
// An input TupMsgBuffer.
// -----------------------------------------------------------------------
struct ExEspCancelReqHeader : public ExEspRequestHeader
{
  ExEspCancelReqHeader(NAMemory *heap);

  // constructor used to perform copyless receive, maps packed objects in place
  ExEspCancelReqHeader(IpcBufferedMsgStream* msgStream);

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();

  // data members
  char endianness_; // big-endian, little endian
  ExFragInstanceHandle handle_;
  Int32 myInstanceNum_;
  Int32 spare1_;
  Int32 spare2_;
};

// -----------------------------------------------------------------------
// Reply to Cancel.
//
// A simple reply to a cancel request.
//
// Objects that follow this header:
//
//  none.
// -----------------------------------------------------------------------
struct ExEspCancelReplyHeader : public ExEspReplyHeader
{
  ExEspCancelReplyHeader(NAMemory *heap);

  // constructor used to perform copyless receive, maps packed objects in place
  ExEspCancelReplyHeader(IpcBufferedMsgStream* msgStream);

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();

  Int32 spare1_;
  Int32 spare2_;

};

// -----------------------------------------------------------------------
// Late Cancel request:
//
// The parent of a fragment instance (consumer) sends down an indication
// to the producer that it has lost interest in a request before a down
// queue entry reached the send top node of the parent. This message
// makes it known to the send bottom node (producer) that it will never
// have to produce any rows for this request.
//
// Objects that follow this header:
//
// An input TupMsgBuffer.
// -----------------------------------------------------------------------
struct ExEspLateCancelReqHeader : public ExEspRequestHeader
{
  ExEspLateCancelReqHeader(NAMemory *heap);

  // constructor used for copyless receive
  ExEspLateCancelReqHeader(IpcBufferedMsgStream* msgStream);

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();

  // data members
  ExFragKey key_;
  Int32 myInstanceNum_;
  Int32 spare1_;
  Int32 spare2_;
  Int32 spare3_;
  Int32 spare4_;
};

// -----------------------------------------------------------------------
// The reply message for requests other than input data requests and
// continue requests.
// The reply indicates whether the request was successful or not.
//
// Objects that follow this header:
//
// An SQL Diagnostics Area.
// -----------------------------------------------------------------------
struct ExEspReturnStatusReplyHeader : public ExEspReplyHeader
{

  // possible return states for the instance (one status reply header
  // is sent for each instance)
  //
  // DOWNLOADED: instance has been downloaded but is waiting for
  //             fixup request, fragment handle has been assigned
  // READY:      instance has successfully been fixed up and is ready
  //             to execute
  // ACTIVE:     instance is executing
  // RELEASED:   the instance has been released (this may be due to an
  //             error or due to a release request)
  // BROKEN:     as a result of an error, the instance is broken and
  //             cannot be reused
  //
  // Note: the instance return status does not necessarily indicate
  // success or failure of a request, to find out about this examine
  // the attached diagnostics area. The instance status is mainly
  // for the communication protocol to avoid cascading errors. It is
  // not always possible to know the state of the ESP instance from
  // the error returned in the diagnostics area.
  enum
  {
    INSTANCE_DOWNLOADED = 111,
    INSTANCE_READY      = 112,
    INSTANCE_ACTIVE     = 113,
    INSTANCE_RELEASED   = 114,
    INSTANCE_BROKEN     = 115
  };

  ExEspReturnStatusReplyHeader(NAMemory *heap);

  // constructor used to perform copyless receive, maps packed objects in place
  ExEspReturnStatusReplyHeader(IpcBufferedMsgStream* msgStream);

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();

  // data members

  // the status reply contains both the input key data and the resulting
  // fragment instance handle
  ExFragKey            key_;

  // assigned handle; if a NULL handle is passed back this means that
  // no instance is downloaded (either a serious error occurred or
  // this is the reply to a release message)
  ExFragInstanceHandle handle_;

  // return the state of the instance (as a long, enums are not portable)
  Int32                instanceState_;
  char endianness_; // big-endian, little endian
  char spare1_;
  Int16 spare2_;
  Int32 spare3_;
  Int32 spare4_;
};

// -----------------------------------------------------------------------
// A message buffer to contain tuples and the corresponding control info.
// This class is a wrapper around the sql_buffer class which is common
// between the filesystem interface and the ESP message interface.
// -----------------------------------------------------------------------
class TupMsgBuffer : public ExEspMsgObj
{
public:

  enum InOut
    {
      MSG_IN,
      MSG_OUT
    };

  // constructor with a given buffer length
  TupMsgBuffer(Lng32 bufferLen, InOut inOut, NAMemory *heap);

  // Constructor used to create a packed send message object.
  TupMsgBuffer(Lng32 bufferLen,
               InOut inOut,
               IpcBufferedMsgStream* msgStream);

  // Constructor used to perform copyless receive. maps packed objects in place.
  TupMsgBuffer(IpcBufferedMsgStream* msgStream);

  ~TupMsgBuffer();

  // get a pointer to the sql_buffer object associated with this object
  // derived classes can implement this differently
  inline SqlBuffer *get_sql_buffer()               { return theBuffer_; }

  // virtual methods needed for the base class
  IpcMessageObjSize packedLength();
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);
  void unpackObj(IpcMessageObjType objType,
		 IpcMessageObjVersion objVersion,
		 NABoolean sameEndianness,
		 IpcMessageObjSize objSize,
		 IpcConstMessageBufferPtr buffer);

  // determine if TupMsgBuffer in IpcMessageBuffer is available for recycle.
  virtual NABoolean msgObjIsFree()
    { return theBuffer_->isFree(); }

  // deal with transport issues, change pointers to offsets, etc.
  virtual void prepMsgObjForSend()
    { theBuffer_->drivePack(); }

private:

  SqlBuffer  *theBuffer_;
  Int32       filler64BitPtr_;
  Int32       allocSize_; //may be larger than sql_buffer's size
  char        endianness_; // big-endian, little endian
  char        spare1_;
  Int16       spare2_;
  Int32       spare3_;
  Int32       spare4_;
};

// -----------------------------------------------------------------------
// The process ids of the instances of a fragment, in a form that can
// be shipped in a message.
// -----------------------------------------------------------------------
class ExProcessIdsOfFrag : public ExEspMsgObj
{
public:

  ExProcessIdsOfFrag(NAMemory *heap, ExFragId fragId = 0);

  virtual ~ExProcessIdsOfFrag() {}

  // accessor functions
  inline CollIndex entries() const       { return processIds_.entries(); }
  inline ExFragId getFragId() const              { return fragmentId_; }
  inline const IpcProcessId & operator[] (CollIndex i) const
                                                { return processIds_[i]; }
  inline const IpcProcessId & getProcessId(CollIndex i) const
                                                { return processIds_[i]; }
  // add another process id at the end of the list
  inline void addProcessId(const IpcProcessId &newPid)
                                           { processIds_.insert(newPid); }

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);
  void unpackObj(IpcMessageObjType objType,
		 IpcMessageObjVersion objVersion,
		 NABoolean sameEndianness,
		 IpcMessageObjSize objSize,
		 IpcConstMessageBufferPtr buffer);

  // operator == is needed for objects that are stored in collections
  inline NABoolean operator == (const ExProcessIdsOfFrag &other) const
                              { return fragmentId_ == other.fragmentId_; }

private:

  ExFragId            fragmentId_;
  Int64               spare_;
  LIST(IpcProcessId)  processIds_;
};

// -----------------------------------------------------------------------
// Outside of a message, it is more convenient to keep multiple
// ExProcessIdsOfFrag objects in a list.
// -----------------------------------------------------------------------
class ExProcessIdsOfFragList : public LIST(ExProcessIdsOfFrag *)
{
public:

  ExProcessIdsOfFragList(NAMemory *heap) : LIST(ExProcessIdsOfFrag *)(heap) {}

  // find an entry with a particular fragment id
  ExProcessIdsOfFrag * findEntry(ExFragId fragId) const;

  // how many instances does this fragment have
  inline Lng32 getNumOfInstances(ExFragId fragId) const
                                  { return findEntry(fragId)->entries(); }

  // get the process id of a fragment instance
  inline const IpcProcessId & getProcessId(ExFragId fragId,
					   CollIndex instanceNum) const
                  { return findEntry(fragId)->getProcessId(instanceNum); }

};

// -----------------------------------------------------------------------
// The generated data of a fragment, in a form that can be shipped in
// a message.
// -----------------------------------------------------------------------
class ExMsgFragment : public ExEspMsgObj
{
public:

  ExMsgFragment(NAMemory *heap); // for objects that get unpacked from a msg
  ExMsgFragment(
       const ExFragKey                &key,
       ExFragDir::ExFragEntryType      fragType,
       ExFragId                        parentId,
       Lng32                           topNodeOffset,
       IpcMessageObjSize               fragmentLength,
       char                           *fragment,
       Lng32                           numTemps,
       unsigned short                  mxv,
       unsigned short                  planVersion,
       NABoolean                       needsTransaction,
       ULng32                          injectErrorAtExprFreq,
       NAMemory                       *heap,
       NABoolean                       takeOwnership,
       NABoolean                       displayInGui,
       const char                     *queryId,
       Lng32                           queryIdLen,
       Lng32                           userID,
       const char                     *userName,
       Lng32                           userNameLen,
       IpcMessageObjSize               compressedLength);
  ~ExMsgFragment();

  inline const ExFragKey & getKey() const                 { return key_; }
  inline ExFragDir::ExFragEntryType getFragType() const
                { return (ExFragDir::ExFragEntryType) f_.fragType_; }
  inline ExFragId getParentId() const           { return f_.parentId_; }
  inline Lng32 getTopNodeOffset() const       { return f_.topNodeOffset_; }
  inline IpcMessageObjSize getFragmentLength() const
                                            { return f_.fragmentLength_; }
  inline char *getFragment() const                   { return fragment_; }
  inline Lng32 getNumTemps() const                 { return f_.numTemps_; }
  inline NABoolean getNeedsTransaction() const
                              { return (NABoolean) f_.needsTransaction_; }
  inline NABoolean getDisplayInGui() const
                                  { return (NABoolean) f_.displayInGui_; }

  inline ULng32 getInjectErrorAtExpr() const
                                       { return f_.injectErrorAtExprFreq_; }

  inline UInt16 getMxvOfOriginator() const { return f_.mxvOfOriginator_; }

  inline UInt16 getPlanVersion() const { return f_.planVersion_; }

  inline const char *getQueryId() const { return f_.queryId_; }
  inline Lng32 getQueryIdLen() { return f_.queryIdLen_; }

  Lng32 getDatabaseUserID() const { return f_.userID_; }
  const char *getDatabaseUserName() { return f_.userName_; }
  Lng32 getDatabaseUserNameLen() const { return f_.userNameLen_; }

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);
  void unpackObj(IpcMessageObjType objType,
		 IpcMessageObjVersion objVersion,
		 NABoolean sameEndianness,
		 IpcMessageObjSize objSize,
		 IpcConstMessageBufferPtr buffer);

private:

  // data members

  // unique identifier of the fragment
  ExFragKey         key_;

  // pointer to the fragment
  char              *fragment_;

  // to make packing/unpacking easier, put all fixed size stuff into a struct
  struct
  {
    // data from the master's fragment directory
    Int32             fragType_; // really ExFragDir::ExFragEntryType
    ExFragId          parentId_;
    Int32             topNodeOffset_;
    IpcMessageObjSize fragmentLength_;

    Int32             numTemps_;
    ExIpcMsgBoolean   needsTransaction_;
    ExIpcMsgBoolean   iOwnTheFragment_; // TRUE means destructor deletes frag
    ExIpcMsgBoolean   displayInGui_;
    UInt32            injectErrorAtExprFreq_;
    UInt16            mxvOfOriginator_;
    UInt16            planVersion_;
    char             *queryId_;
    Lng32             queryIdLen_;
    IpcMessageObjSize compressedLength_;
    Lng32             userID_;
    char             *userName_;
    Lng32             userNameLen_;
    Int32             reserved[6];
  } f_;
};

// -----------------------------------------------------------------------
// A transaction id that travels on an IPC message
// -----------------------------------------------------------------------

const Int64 ExInvalidInt64Transid = -1;

class ExMsgTransId : public ExEspMsgObj
{
public:

  ExMsgTransId(NAMemory *heap,
	       Int64 tid = ExInvalidInt64Transid, short *phandle = 0);
  ~ExMsgTransId();

  // accessor methods
  Int64 getTransIdAsInt() const                       { return localTransId_; }
  void setTransId(Int64 tid)                           { localTransId_ = tid; }

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();

private:

  Int64 localTransId_;
  Int64 originatingTransId_;
  Int64 spare2_;
};



class ExMsgTimeoutData : public ExEspMsgObj
{
public:

  ExMsgTimeoutData(TimeoutData *toData, NAMemory *heap );
  ~ExMsgTimeoutData();

  // accessor methods
  TimeoutData       * getTimeoutData() { return timeoutData_; } ;

  // methods needed to pack and unpack this object
  IpcMessageObjSize packedLength();
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);
  void unpackObj(IpcMessageObjType objType,
		 IpcMessageObjVersion objVersion,
		 NABoolean sameEndianness,
		 IpcMessageObjSize objSize,
		 IpcConstMessageBufferPtr buffer);

private:

  TimeoutData       * timeoutData_;
  ExIpcMsgBoolean   iOwnTD_; // TRUE means destructor deletes timeoutData_
};



//-------------------------------------------------------------------------
// A message object to hold SeaMonster properties for the query
//
// Properties include:
// * SM query ID
// * SM trace level
// * SM trace file prefix
//-------------------------------------------------------------------------
class ExSMDownloadInfo : public ExEspMsgObj
{
public:

  // Constructor to send a message
  ExSMDownloadInfo(NAMemory *heap, Int64 smQueryID, Int32 smTraceLevel,
                   const char *smTraceFilePrefix, Int32 flags);

  // Constructor to receive a message
  ExSMDownloadInfo(NAMemory *heap);

  ExSMDownloadInfo(); // Do not implement

  virtual ~ExSMDownloadInfo();

  // Virtual methods needed to pack and unpack this object
  IpcMessageObjSize packedLength();
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);
  void unpackObj(IpcMessageObjType objType,
		 IpcMessageObjVersion objVersion,
		 NABoolean sameEndianness,
		 IpcMessageObjSize objSize,
		 IpcConstMessageBufferPtr buffer);

  // Accessor methods
  Int64 getQueryID() const { return smQueryID_; }
  Int32 getTraceLevel() const { return smTraceLevel_; }
  Int32 getFlags() const { return flags_; }
  const char *getTraceFilePrefix() const { return smTraceFilePrefix_; }

private:
  Int64 smQueryID_;
  Int32 smTraceLevel_;
  char *smTraceFilePrefix_;
  Int32 flags_;
};

class ExMsgResourceInfo : public ExEspMsgObj
{
public:

  ExMsgResourceInfo(const ExScratchFileOptions *sfo,
		    NAMemory *heap);
  ~ExMsgResourceInfo();

  // accessor methods
  const ExScratchFileOptions * getScratchFileOptions() const { return sfo_; }

  // method needed to pack and unpack this object
  IpcMessageObjSize packedLength();
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);
  void unpackObj(IpcMessageObjType objType,
		 IpcMessageObjVersion objVersion,
		 NABoolean sameEndianness,
		 IpcMessageObjSize objSize,
		 IpcConstMessageBufferPtr buffer);

private:

  const ExScratchFileOptions * sfo_;
  Int32                      totalNameLength_;
  Int32                      spare_;
  char                       * bufferForDependentObjects_;

};

// ----------------------------------------------------------
// ExMsgSecurityInfo:
// - For Parallel Extract operations, an object of this class
//   will have the security key and user id from the consumer
//   query. This object is sent along with the OPEN message
//   from consumer's sendTop to the producer's splitBottom nodes.
// - The packing/unpacking function implementations should consider
//   that the sender uses buffered stream and the recipient uses
//   unbuffered stream.
class ExMsgSecurityInfo : public ExEspMsgObj
{
public:

  ExMsgSecurityInfo(NAMemory *heap);
  ~ExMsgSecurityInfo();

  // pack/unpack methods
  IpcMessageObjSize packedLength();
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);
  void unpackObj(IpcMessageObjType objType,
		 IpcMessageObjVersion objVersion,
		 NABoolean sameEndianness,
		 IpcMessageObjSize objSize,
		 IpcConstMessageBufferPtr buffer);

  // accessor methods
  char * getSecurityKey() { return securityKey_; } ;
  void setSecurityKey(char *str) { securityKey_ = str; };
  char * getAuthID() { return authid_; } ;
  void setAuthID(char *str) { authid_ = str; };

private:

  char *securityKey_;
  char *authid_;
};

#endif /* EX_ESP_MSG_H */
