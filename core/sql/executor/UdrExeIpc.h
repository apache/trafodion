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
#ifndef _UDR_EXE_IPC_H_
#define _UDR_EXE_IPC_H_
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         UdrExeIpc.h
 * Description:  IPC streams and message objects shared by the SQL/MX
 *               executor and the UDR server
 *
 * Created:      08/20/2000
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#include "Ipc.h"
#include "ComSmallDefs.h"
#include "ComSizeDefs.h"
#include "Int64.h"
#include "ExCollections.h"
#include <stdio.h>

#ifdef UDRSERV_BUILD
#include "udrdefs.h"
#endif // UDRSERV_BUILD

#include "seabed/fs.h"
#include "seabed/ms.h"

#include "sqlcli.h"  // need SQLSTMT_ID * for ResultSetInfo::stmtID_

//
// Define UDR_DEBUG only if we are doing a debug build with the C
// runtime library
//
#ifdef _DEBUG
#ifndef UDR_DEBUG
#define UDR_DEBUG 1
#endif // not UDR_DEBUG
#else  // _DEBUG
#undef UDR_DEBUG
#endif // _DEBUG else

//
// Define a local assert macro. We define UdrExeAssert to something
// other than ex_assert in the UDR Server build.
//
#ifdef UDRSERV_BUILD
#define UdrExeAssert(a,b) UDR_ASSERT(a,b)
#else
#define UdrExeAssert(a,b) ex_assert(a,b)
#endif // UDRSERV_BUILD

//
// UDR printf-style debugging function. Only defined for UDR_DEBUG builds.
//
// Usage notes
//
// - The UdrPrintf() function takes a variable length of arguments just
//   like fprintf. For example: UdrPrintf(f, "%s %d", string, int)
//
// - UdrPrintf() is not meant to be called throughout the code because
//   it is only defined in the debug build and it is a function not a
//   macro. The suggested usage is to wrap calls to UdrPrintf() in
//   macros that resolve to a no-op for release builds.
//
// - Calling UdrPrintf() with a NULL file pointer is allowed but has
//   no effect.
//
#ifdef UDR_DEBUG
extern void UdrPrintf(FILE *, const char *formatString, ...);
#define PLURAL_SUFFIX(x) ( (x) == (1) ? ("") : ("s") )
#endif

// Forward class references
class SqlBuffer;
class ExUdrTdb;
class NAMemory;
class TransAttrComplexObject;
class CtrlStmtComplexObject;
class MessageOperator;
class InputContainer;
class OutputContainer;

// Classes defined in this file
class UdrControlStream;
class UdrMessageObj;
class UdrControlMsg;
class UdrLoadMsg;
class UdrLoadReply;
class UdrUnloadMsg;
class UdrUnloadReply;
class UdrSessionMsg;
class UdrSessionReply;
class UdrDataHeader;
class UdrDataBuffer;
class UdrContinueMsg;
class UdrErrorReply;
class UdrEnterTxMsg;
class UdrEnterTxReply;
class UdrSuspendTxMsg;
class UdrSuspendTxReply;
class UdrExitTxMsg;
class UdrExitTxReply;
class UdrRSMessageObj;
class UdrRSControlMsg;
class UdrRSLoadMsg;
class UdrRSLoadReply;
class UdrRSDataHeader;
class UdrRSContinueMsg;
class UdrRSCloseMsg;
class UdrRSCloseReply;
class UdrRSUnloadMsg;
class UdrRSUnloadReply;
class UdrRSInfoMsg;

// Version numbers
const Int32 UdrControlMsgStreamVersionNumber = 100;
const Int32 UdrLoadMsgVersionNumber = 103;
const Int32 UdrLoadReplyVersionNumber = 100;
const Int32 UdrUnloadMsgVersionNumber = 100;
const Int32 UdrUnloadReplyVersionNumber = 100;
const Int32 UdrSessionMsgVersionNumber = 200;
const Int32 UdrSessionReplyVersionNumber = 200;
const Int32 UdrDataHeaderVersionNumber = 100;
const Int32 UdrDataBufferVersionNumber = 101;
const Int32 UdrContinueMsgVersionNumber = 100;
const Int32 UdrErrorReplyVersionNumber = 100;
const Int32 UdrEnterTxMsgVersionNumber = 100;
const Int32 UdrEnterTxReplyVersionNumber = 100;
const Int32 UdrSuspendTxMsgVersionNumber = 100;
const Int32 UdrSuspendTxReplyVersionNumber = 100;
const Int32 UdrExitTxMsgVersionNumber = 100;
const Int32 UdrExitTxReplyVersionNumber = 100;
const Int32 UdrRSLoadMsgVersionNumber = 100;
const Int32 UdrRSLoadReplyVersionNumber = 100;
const Int32 UdrRSDataHeaderVersionNumber = 100;
const Int32 UdrRSContinueMsgVersionNumber = 100;
const Int32 UdrRSCloseMsgVersionNumber = 100;
const Int32 UdrRSCloseReplyVersionNumber = 100;
const Int32 UdrRSUnloadMsgVersionNumber = 100;
const Int32 UdrRSUnloadReplyVersionNumber = 100;
const Int32 UdrRSInfoMsgVersionNumber = 100;
const Int32 UdrTmudfDataHeaderVersionNumber = 100;

// An enumeration of all IPC objects for UDRs. Includes both message
// objects and stream objects.
// 
// We use the 8000-8999 range for UDR IPC objects. The range is
// reserved in the IpcMessageTypeEnum enumeration (see
// common/IpcMessageType.h).
enum UdrIpcObjectType
{
  UDR_IPC_FIRST = IPC_MSG_SQLUDR_FIRST,    // 8000

  //
  // Message types
  //
  UDR_MSG_LOAD,                            // 8001
  UDR_MSG_LOAD_REPLY,                      // 8002

  UDR_MSG_UNLOAD,                          // 8003
  UDR_MSG_UNLOAD_REPLY,                    // 8004

  UDR_MSG_DATA_HEADER,                     // 8005
  UDR_MSG_DATA_REQUEST,                    // 8006
  UDR_MSG_DATA_REPLY,                      // 8007
  UDR_MSG_CONTINUE_REQUEST,                // 8008

  UDR_MSG_ERROR_REPLY,                     // 8009

  UDR_MSG_SESSION,                         // 8010
  UDR_MSG_SESSION_REPLY,                   // 8011

  UDR_MSG_ENTER_TX,                        // 8012
  UDR_MSG_ENTER_TX_REPLY,                  // 8013

  UDR_MSG_EXIT_TX,                         // 8014
  UDR_MSG_EXIT_TX_REPLY,                   // 8015

  UDR_MSG_RS_LOAD,                         // 8016
  UDR_MSG_RS_LOAD_REPLY,                   // 8017

  UDR_MSG_RS_DATA_HEADER,                  // 8018

  UDR_MSG_RS_CONTINUE,                     // 8019

  UDR_MSG_RS_CLOSE,                        // 8020
  UDR_MSG_RS_CLOSE_REPLY,                  // 8021

  UDR_MSG_RS_UNLOAD,                       // 8022
  UDR_MSG_RS_UNLOAD_REPLY,                 // 8023

  UDR_MSG_RS_INFO,			   // 8024

  UDR_MSG_SUSPEND_TX,                      // 8025
  UDR_MSG_SUSPEND_TX_REPLY,                // 8026
  UDR_MSG_TMUDF_DATA_HEADER,               // 8027              

  // ----> Add new message types just above this line <----

  UDR_STREAM_FIRST = UDR_IPC_FIRST + 800,

  //
  // Stream types
  //
  UDR_STREAM_CLIENT_CONTROL,               // 8801
  UDR_STREAM_SERVER_CONTROL,               // 8802

  UDR_STREAM_CLIENT_DATA,                  // 8803
  UDR_STREAM_SERVER_DATA,                  // 8804

  UDR_STREAM_SERVER_REPLY,                 // 8805

  // ----> Add new stream types just above this line <----

  UDR_IPC_LAST = IPC_MSG_SQLUDR_LAST,      // 8999

  UDR_IPC_INVALID = UDR_IPC_LAST
};

// A global function to return message type enum values as strings
const char *GetUdrIpcTypeString(UdrIpcObjectType t);

//----------------------------------------------------------------------
// UDR handle
//----------------------------------------------------------------------
typedef Int64 UdrHandle;
#define INVALID_UDR_HANDLE 0
#define UdrHandleIsValid(x) ( (x) != (INVALID_UDR_HANDLE) )

//----------------------------------------------------------------------
// RS handle
//----------------------------------------------------------------------
typedef Int64 RSHandle;
#define INVALID_RS_HANDLE 0
#define RSHandleIsValid(x) ( (x) != (INVALID_RS_HANDLE) )

// Define a printf format specifier for 64-bit integers
#define INT64_PRINTF_SPEC "%ld"

const Int32 UDRMAXOPENERS_V100 = 1;
const Int32 UDRMAXSTRINGSIZE = 255;
const Int32 UDRMAXNUMPARAMETERS = 255;
const Int32 UDRMAXBUFFERSIZE = 4000;
const Int32 UDRNUMBERSTATISTICS_V100 = 32;
const Int32 UDRSIZESTATDESC_V100 = 30;

//----------------------------------------------------------------------
// UDR parameter info
//----------------------------------------------------------------------
struct UdrParameterInfo
{
  friend class UdrLoadMsg;
  friend class UdrRSLoadMsg;
  friend struct UdrTableInputInfo;

public:

  UdrParameterInfo(
    ComUInt32 position,
    ComUInt32 flags,
    ComSInt16 fsType,
    ComSInt16 ansiType,
    ComUInt16 paramNameLen,
    const char *paramName,
    ComUInt16 prec,
    ComUInt16 scale,
    ComUInt16 encodingCharSet,
    ComUInt16 collation,
    ComUInt32 dataLength,
    ComSInt16 nullIndicatorLength,
    ComSInt32 nullIndicatorOffset,
    ComUInt32 dataOffset,
    ComSInt32 vcLenIndOffset,
    ComSInt16 vcIndicatorLength
    );

  inline ComUInt32 getPosition() const { return position_; }
  inline ComUInt32 getFlags() const { return flags_; }
  inline ComSInt16 getFSType() const { return fsType_; }
  inline ComSInt16 getAnsiType() const { return ansiType_; }
  inline ComUInt16 getPrec() const { return prec_; }
  inline ComUInt16 getScale() const { return scale_; }
  inline ComUInt16 getEncodingCharSet() const { return encodingCharSet_; }
  inline ComUInt16 getCollation() const { return collation_; }
  inline ComUInt32 getDataLength() const { return dataLength_; }
  inline ComSInt16 getNullIndicatorLength() const
  { return nullIndicatorLength_; }
  inline ComSInt32 getNullIndicatorOffset() const
  { return nullIndicatorOffset_; }
  inline ComUInt32 getDataOffset() const { return dataOffset_; }
  inline ComSInt32 getVCLenIndOffset() const { return vcLenIndOffset_; }
  inline ComSInt16 getVCIndicatorLength() const { return vcIndicatorLength_; }
  inline const char *getParamName() const { return paramName_; }
  inline ComUInt16 getParamNameLen() const { return paramNameLen_; }

  inline NABoolean isIn() const
  { return (flags_ & UDR_PARAM_IN)? TRUE : FALSE; }
  inline NABoolean isOut() const
  { return (flags_ & UDR_PARAM_OUT)? TRUE : FALSE; }
  inline NABoolean isInOut() const
  { return isIn() && isOut(); }
  inline NABoolean isNullable() const
  { return (flags_ & UDR_PARAM_NULLABLE)? TRUE : FALSE; }
  inline NABoolean isLmObjType() const
  { return (flags_ & UDR_PARAM_LM_OBJ_TYPE)? TRUE : FALSE; }

  //
  // Display function
  //
  void display(FILE *f, Lng32 indent, UdrParameterInfo *pi) const;

  //
  // Assignment operator
  //
  UdrParameterInfo& operator=(const UdrParameterInfo &other);

protected:

  //
  // This class does not derive from IpcMessageObj but we need
  // similar methods to pack/unpack the object in a message buffer.
  // Only UdrLoadMsg objects will call these methods, when it is
  // packing/unpacking itself.
  //
  IpcMessageObjSize packedLength() const;
  IpcMessageObjSize pack(IpcMessageBufferPtr &buffer) const;
  void unpack(IpcConstMessageBufferPtr &buffer);

  //
  // This first set of fields describe the UDR formal parameter
  //
  ComUInt32 position_;          // Position in formal parameter list
  ComUInt32 flags_;             // ComUdrParamFlags in ComSmallDefs.h
  ComSInt16 fsType_;            // Formal type (FS type)
  ComSInt16 ansiType_;          // Formal type (ANSI type defined in sqlcli.h)
  short paramNameLen_;          // Length of the parameter name
  char paramName_[129];         // Null-terminated parameter name
  ComUInt16 prec_;              // Numeric precision or datetime subtype
  ComUInt16 scale_;             // Scale for numeric and time types
  ComUInt16 encodingCharSet_;   // Encoding Charset for char & varchar
  ComUInt16 collation_;         // Collation for char & varchar

  //
  // These fields describe the actual data that is sent to the UDR
  // server. In some cases the formal type is not acceptable as input
  // to the language manager, so the physical representation of a
  // value does not correspond with the SQL type of the formal
  // parameter. These fields come directly from the Attributes
  // object that describes the actual value being passed to the UDR
  // server and Language Manager.
  //
  ComUInt32 dataLength_;            // Length of data area
  ComSInt16 nullIndicatorLength_;   // 2 or 0
  ComSInt32 nullIndicatorOffset_;   // Offset of null indicator or -1
  ComUInt32 dataOffset_;            // Offset of data area
  ComSInt32 vcLenIndOffset_;        // Offset of VarChar Indicator
  ComSInt16 vcIndicatorLength_;     // Length of VarChar Indicator

private:

  //
  // Default constructor should not be used outside this class
  //
  UdrParameterInfo()
  {
  }

};



//----------------------------------------------------------------------
// UDR table input info
//----------------------------------------------------------------------
struct UdrTableInputInfo
{
  friend class UdrLoadMsg;
  friend struct UdrParameterInfo;

public:
  // this version isused in the uderserve for ipc unpack
  UdrTableInputInfo(
    ComUInt16 tabIndex,
    ComUInt16 tableNameLen,
    const char *tableName,
    ComUInt16 numColumns,
    ComUInt32 rowLength
   
    );
 
 
  
  inline ComSInt16 getTabIndex() const { return tabIndex_; }
  inline ComSInt16 getTableNameLen() const { return tableNameLen_; }  
  inline  const char *getTableName() const { return tableName_; }
  inline UdrParameterInfo *getInTableColumnDescs(){return inTableColumnDescs_;}
  void setInTableColumnDescs(UdrParameterInfo *inTableColumnDescs)
  {inTableColumnDescs_ = inTableColumnDescs;}
  inline ComUInt16 getNumColumns() const {return numColumns_;}

  //
  // Display function
  //
  void display(FILE *f, Lng32 indent, UdrTableInputInfo *pi) const{};

  //
  // Assignment operator
  //
  UdrTableInputInfo& operator=(const UdrTableInputInfo &other);
  const UdrParameterInfo &getInTableColumnDesc(ComUInt32 i) const;
  void setInTableColumnDesc(ComUInt32 i, const UdrParameterInfo &info,
			    NAMemory *heap);
  
  void setRowLength(UInt32 val) { outputRowLen_ = val;}
  UInt32 getRowLength() const { return outputRowLen_; }
protected:

  //
  // This class does not derive from IpcMessageObj but we need
  // similar methods to pack/unpack the object in a message buffer.
  // Only UdrLoadMsg objects will call these methods, when it is
  // packing/unpacking itself.
  //
  IpcMessageObjSize packedLength() const;
  IpcMessageObjSize pack(IpcMessageBufferPtr &buffer) const ;
  void unpack(IpcConstMessageBufferPtr &buffer, NAMemory *heap);
 
  //
  // This set of fields describe the UDR table input info

  ComUInt16 tabIndex_;
  ComUInt16 tableNameLen_;
  char tableName_[ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+1];
  ComUInt16 numColumns_;
  UInt32 outputRowLen_;
  UdrParameterInfo *inTableColumnDescs_;
  


  
private:

  //
  // Default constructor should not be used outside this class
  //
  UdrTableInputInfo()
  {
  }

};

//----------------------------------------------------------------------
// UDR control message stream base class
//
// Client-side and Server-side control streams will inherit
// from this base class.
//----------------------------------------------------------------------
class UdrControlStream : public IpcMessageStream
{
public:
  UdrControlStream(IpcEnvironment *env,
    IpcMessageType msgType,
    IpcMessageObjVersion version);

  virtual ~UdrControlStream();

  virtual void actOnSend(IpcConnection *connection);
  virtual void actOnReceive(IpcConnection *connection);

protected:
  ULng32 sendCount_;
  ULng32 recvCount_;

};

//----------------------------------------------------------------------
// About memory management of UDR message objects
//
//  All UDR message objects will derive from UdrMessageObj which
//  derives from IpcMessageObj. IpcMessageObj comes from the export
//  library, does not inherit from NABasicObject and does not provide
//  any NAMemory heap management. The UdrMessageObj class is introduced
//  only to provide a delete operator that handles deallocation of
//  objects from an NAMemory heap.
//
//  In the release build, any UDR message objects that do their own
//  memory allocations must have a heap pointer. UDR message objects
//  allocated using in-place stream buffering will not have a heap
//  pointer and cannot do their own memory allocations.
//
//  Client-side code that sends UDR messages on a non-buffered stream
//  will do the following:
//  - Allocate a UDR message object using an appropriate constructor
//  - Put the message object into the stream using the << operator
//  - Initiate a nowait send() on the stream
//  - Decrement the message's reference count. The stream may have
//    incremented the reference count if the stream is holding a
//    pointer to the message. If so the object won't go away until
//    the reference count drops to zero, which triggers the
//    UdrMessageObj delete operator.
//
//  Client-side code that sends UDR messages on a buffered stream
//  using copyless IPC will do the following:
//  - Allocate a UDR message object directly in the stream's I/O
//    buffer using an appropriate constructor
//  - Call sendRequest() on the stream
//  - If multi-buffer replies are possible, allocate and send
//    one or more continue requests
//
//  Server-side code that receives UDR messages on a non-buffered
//  stream will do the following:
//  - Call stream.moreObjects() to see if a request has arrived
//  - Allocate an uninitialized UDR message object using an
//    appropriate constructor
//  - Extract message object contents into the uninitialized object
//    using the >> operator
//  - If the message is to be routed to a buffered stream:
//      - Obtain a pointer to the other stream
//      - Call stream.giveReceiveMsgTo(other) to route the message to
//        the other stream. Routing logic will call other.actOnReceive()
//  - Otherwise:
//      - Put one or more reply message objects into the stream using
//        the << operator
//      - Initiate a waited send() on the stream
//      - Decrement the message's reference count
//
//  Server-side code that receives UDR messages on a buffered stream
//  using copyless IPC will do the following:
//  - Inside the actOnReceive() method:
//      - Call stream.getNextReceiveMsg() to see if a complete request
//        has arrived
//      - Obtain a message object from the stream's I/O buffer using an
//        appropriate constructor
//      - Put one or more reply message objects into the stream using
//        either in-place buffering or the << operator
//      - Call sendRequest() on the stream
//
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// About packing and unpacking UDR message objects
//
// - Packing and unpacking do not take place when messages are
//   moved to/from a buffered stream using copyless IPC.
//
// - Packing and unpacking take place when messages are sent on a
//   non-buffered stream, or when the >> operator is used to extract
//   objects from a stream, or when the extractNextObj() method is
//   called on a stream.
//
// - Any new message object class that is not an abstract class
//   must implement packedLength()
//
// - If translation is necessary for packing and unpacking then the
//   new class must implement packObjIntoMessage() and unpackObj().
//   Message classes that require no translation (e.g. the
//   object introduces no new data members) can use inherited
//   implementations of packObjIntoMessage() and unpackObj().
//
// - All message classes that travel in UDR replies must also
//   implement a checkObj() method (or inherit an appropriate one) so
//   that IPC integrity checks will be performed in the executor upon
//   receipt of an instance of that class.
//
//----------------------------------------------------------------------

//----------------------------------------------------------------------
// UDR message base class
//
// Currently the only functionality provided by this class is memory
// management on NAMemory heaps. The code for this class is a copy of
// the code from the ExEspMsgObj class which is responsible for heap
// management of ESP message objects.
//----------------------------------------------------------------------
class UdrMessageObj : public IpcMessageObj
{
  typedef IpcMessageObj super;

public:
  //
  // Constructor for allocation on an NAMemory heap
  //
  UdrMessageObj(UdrIpcObjectType objType,
                IpcMessageObjVersion objVersion,
                NAMemory *heap);

  //
  // Constructor for copyless receive
  //
  UdrMessageObj(IpcBufferedMsgStream *msgStream)
    : IpcMessageObj(msgStream), heap_(NULL)
  {
  }

  //
  // The delete operator
  // The heap management in this operator is the real reason for the
  // existence of this class
  //
  void operator delete(void *p);


  //
  // Accessor/Mutator methods
  //
  inline NAMemory *getHeap() const { return heap_; }
  inline const UdrHandle &getHandle() const { return handle_; }
  inline void setHandle(const UdrHandle &h) { handle_ = h; }

  // Override pack/unpack/check methods from IpcMessageObj. Note that
  // packObjIntoMessage() does not advance the buffer pointer like we
  // do in the packUdrBaseClass() protected method declared below.
  IpcMessageObjSize packedLength();
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);

  void unpackObj(IpcMessageObjType objType,
                 IpcMessageObjVersion objVersion,
                 NABoolean sameEndianness,
                 IpcMessageObjSize objSize,
                 IpcConstMessageBufferPtr buffer);

  NABoolean checkObj(IpcMessageObjType t,
                     IpcMessageObjVersion v,
                     NABoolean sameEndianness,
                     IpcMessageObjSize size,
                     IpcConstMessageBufferPtr buffer) const;

  //
  // We need our own decrRefCount() method so that the correct
  // operator delete gets called once an object is no longer needed
  //
  virtual IpcMessageRefCount decrRefCount();

protected:
  //
  // Helper functions to manage the NAMemory heap
  //
  char *allocateMemory(ComUInt32 nBytes);
  void deallocateMemory(char *s);
  char *allocateString(const char *s);
  void deallocateString(char *&s);

  // Methods that allow derived classes to pack/unpack/check fields
  // defined in this base class. Derived classes can also implement
  // these functions and will typically do so by first calling the
  // parent class implementation and then packing/unpacking/checking
  // their own data members.
  //
  // Note that the heap_ member does not get sent in a message and
  // will not be overwritten when a message is unpacked.
  virtual IpcMessageObjSize udrBaseClassPackedLength();
  virtual IpcMessageObjSize packUdrBaseClass(IpcMessageBufferPtr &buffer);
  virtual void unpackUdrBaseClass(IpcConstMessageBufferPtr &buffer);
  virtual NABoolean checkUdrBaseClass(IpcMessageObjType t,
                                      IpcMessageObjVersion v,
                                      NABoolean sameEndianness,
                                      IpcMessageObjSize size,
                                      IpcConstMessageBufferPtr &buffer) const;

private:

  UdrHandle handle_;

  //
  // We store a pointer to the heap on which this object is allocated.
  // A NULL pointer indicates that the object is allocated directly
  // inside a message buffer with the copyless IPC protocol used by
  // buffered streams.
  //
  NAMemory *heap_;

  //
  // Do not implement default constructors or an assignment operator
  //
  UdrMessageObj();
  UdrMessageObj(const UdrMessageObj &);
  UdrMessageObj &operator=(const UdrMessageObj &);

}; // class UdrMessageObj

//----------------------------------------------------------------------
// UDR control message base class
//
// Currently this class provides no functionality. It just gives us
// a way to distinguish a control message from a data message.
//----------------------------------------------------------------------
class UdrControlMsg : public UdrMessageObj
{
public:
  UdrControlMsg(UdrIpcObjectType msgType, IpcMessageObjVersion version,
    NAMemory *heap)
    : UdrMessageObj(msgType, version, heap)
  {
  }

  virtual ~UdrControlMsg() {}

private:

  //
  // Do not implement default constructors or an assignment operator
  //
  UdrControlMsg();
  UdrControlMsg(const UdrControlMsg &);
  UdrControlMsg &operator=(const UdrControlMsg &);

}; // class UdrControlMsg

//----------------------------------------------------------------------
// UDR control message subclasses
// - Load, Load reply
// - Unload, Unload reply
// - Session, Session reply
//----------------------------------------------------------------------
class UdrLoadMsg : public UdrControlMsg
{
public:
  UdrLoadMsg(NAMemory *heap);

  UdrLoadMsg(NAMemory *heap,
    const char *sqlName,
    const char *routineName,
    const char *routineSignature,
    const char *containerName,
    const char *externalPath,
    const char *librarySqlName,
    ComRoutineTransactionAttributes transactionAttrs,
    ComRoutineSQLAccess sqlAccessMode,
    ComRoutineLanguage language,
    ComRoutineParamStyle paramStyle,
    ComRoutineExternalSecurity externalSecurity,
    ComUInt32 maxNumResultSets,
    ComUInt32 numParameters,
    ComUInt32 numInValues,
    ComUInt32 numOutValues,
    ComUInt32 inBufferSize,           // Size of input SqlBuffer
    ComUInt32 outBufferSize,          // Size of output SqlBuffer
    ComUInt32 inputRowSize,           // Size of input row
    ComUInt32 outputRowSize,          // Size of output row
    ComUInt32 udrFlags,
    Int32 routineOwnerId,
    const char *parentQid,
    ComUInt32 udrSerInvocationInfoLen, // objects for new C++ interface
    const char *udrSerInvocationInfo,
    ComUInt32 udrSerPlanInfoLen,
    const char *udrSerPlanInfo,
    Int32 javaDebugPort,
    Int32 javaDebugTimeout,
    ComUInt32 instanceNum,
    ComUInt32 numInstances);

  virtual ~UdrLoadMsg();

  //
  // Accessor methods
  //
  inline const char *getSqlName() const { return sqlName_; }
  inline const char *getRoutineName() const { return routineName_; }
  inline const char *getSignature() const { return routineSignature_; }
  inline const char *getContainerName() const { return containerName_; }
  inline const char *getExternalPath() const { return externalPath_; }
  inline const char *getLibrarySqlName() const { return librarySqlName_; }  
  inline ComUInt32 getMaxResultSets() const { return maxNumResultSets_; }
  inline ComUInt32 getNumParameters() const { return numParameters_; }
  inline ComUInt32 getNumInValues() const { return numInValues_; }
  inline ComUInt32 getNumOutValues() const { return numOutValues_; }
  inline ComUInt32 getInBufferSize() const { return inBufferSize_; }
  inline ComUInt32 getOutBufferSize() const { return outBufferSize_; }
  inline ComUInt32 getInputRowSize() const { return inputRowSize_; }
  inline ComUInt32 getOutputRowSize() const { return outputRowSize_; }
  inline ComUInt32 getUdrFlags() const { return udrFlags_; }
  inline ComRoutineTransactionAttributes getTransactionAttrs() const
  { return (ComRoutineTransactionAttributes) transactionAttrs_; }
  inline ComRoutineSQLAccess getSqlAccessMode() const
  { return (ComRoutineSQLAccess) sqlAccessMode_; }
  inline ComRoutineLanguage getLanguage() const
  { return (ComRoutineLanguage) language_; }
  inline ComRoutineParamStyle getParamStyle() const
  { return (ComRoutineParamStyle) paramStyle_; }
  inline ComRoutineExternalSecurity getExternalSecurity() const
  { return (ComRoutineExternalSecurity) externalSecurity_; }
  inline Int32 getRoutineOwnerId() const { return routineOwnerId_; }
  inline const char *getParentQid() const { return parentQid_; }
  inline NABoolean isIsolate() const
  { return (udrFlags_ & UDR_ISOLATE)? TRUE : FALSE; }
  inline NABoolean isCallOnNull() const
  { return (udrFlags_ & UDR_CALL_ON_NULL)? TRUE : FALSE; }
  inline NABoolean isExtraCall() const
  { return (udrFlags_ & UDR_EXTRA_CALL)? TRUE : FALSE; }
  inline NABoolean isDeterministic() const
  { return (udrFlags_ & UDR_DETERMINISTIC)? TRUE : FALSE; }
  inline NABoolean isLMNoLoad() const
  { return (udrFlags_ & UDR_LM_NOLOAD)? TRUE : FALSE; }

  // Methods to get/set parameter info structures
  const UdrParameterInfo &getInParam(ComUInt32 i) const;
  const UdrParameterInfo &getOutParam(ComUInt32 i) const;
  UdrParameterInfo *setInParam(ComUInt32 i, const UdrParameterInfo &info);
  UdrParameterInfo *setOutParam(ComUInt32 i, const UdrParameterInfo &info);

  inline ComUInt16 getNumInputTables() {return numInputTables_;}
  inline void setNumInputTables(ComUInt16 n){numInputTables_ = n;}
  inline UdrTableInputInfo *getInputTables() {return inTables_;}
  inline void setParamStyle(ComRoutineParamStyle ps){ paramStyle_ = (ComUInt16)ps;}
  inline void setExternalSecurity(ComRoutineExternalSecurity ps){ externalSecurity_ = (ComUInt16)ps;}
  inline void setRoutineOwnerId(Int32 o){routineOwnerId_ = o;}

  inline ComUInt32 getNumInstances() {return numInstances_;}
  inline void setNumInstances(ComUInt32 n){numInstances_ = n;}

  inline ComUInt32 getInstanceNum() {return instanceNum_;}
  inline void setInstanceNum(ComUInt32 n){instanceNum_ = n;}
  void setChildTableInput(ComUInt32 i, const UdrTableInputInfo &info);
 
  // Methods to get/set the collection of optional data buffers
  void initOptionalDataBufs(ComUInt32 i, NABoolean isShared);
  ComUInt32 getNumOptionalDataBufs();
  void setOptionalDataBuf(ComUInt32 i, const char *buf, ComUInt32 bufLen);
  char *getOptionalDataBuf(ComUInt32 i);

  // methods for C++ interface data
  inline ComUInt32 getUDRSerInvocationInfoLen()
                                          { return udrSerInvocationInfoLen_; }
  inline const char *getUDRSerInvocationInfo()
                                             { return udrSerInvocationInfo_; }
  inline ComUInt32 getUDRSerPlanInfoLen()       { return udrSerPlanInfoLen_; }
  inline const char *getUDRSerPlanInfo()           { return udrSerPlanInfo_; }

  // debugging Java UDRs (works for Trafodion user or debug build only)
  inline Int32 getUdrJavaDebugPort() const       { return udrJavaDebugPort_; }
  inline Int32 getUdrJavaDebugTimeout() const { return udrJavaDebugTimeout_; }
  // Redefine pack/unpack methods from IpcMessageObj
  IpcMessageObjSize packedLength();
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);
  void unpackObj(IpcMessageObjType objType,
    IpcMessageObjVersion objVersion,
    NABoolean sameEndianness,
    IpcMessageObjSize objSize,
    IpcConstMessageBufferPtr buffer);
    void allocateTableInputInfo();
    void deallocateTableInputInfo();
protected:

  // Helper functions for memory management
  void allocateParamInfo();
  void deallocateParamInfo();
 
  void deallocateOptionalDataBufs();

  void allocateInvocationInfo();
  void deallocateInvocationInfo();
  

  char *sqlName_;                    // Fully qualified SQL name
  char *routineName_;                // e.g. a Java method name
  char *routineSignature_;           // e.g. (ii)v for Java "void f(int,int)"
  char *containerName_;              // e.g. a Java class name
  char *externalPath_;               // File system path
  char *librarySqlName_;             // Fully qualified sql name of Jar/DLL
  ComUInt16 transactionAttrs_;           // ComRoutineTransactionAttributes in ComSmallDefs.h
  ComUInt16 sqlAccessMode_;          // ComRoutineSQLAccess in ComSmallDefs.h
  ComUInt16 language_;               // ComRoutineLanguage in ComSmallDefs.h
  ComUInt16 paramStyle_;             // ComRoutineParamStyle in ComSmallDefs.h
  ComUInt16 externalSecurity_;       // ComRoutineExternalSecurity in ComSmallDefs.h
  ComUInt32 maxNumResultSets_;       // Max result sets
  ComUInt32 numParameters_;          // Num formal params
  ComUInt32 numInValues_;            // Num IN/INOUT params
  ComUInt32 numOutValues_;           // Num OUT/INOUT params
  ComUInt32 inBufferSize_;           // Size of input SqlBuffer
  ComUInt32 outBufferSize_;          // Size of output SqlBuffer
  ComUInt32 inputRowSize_;           // Size of input row
  ComUInt32 outputRowSize_;          // Size of output row
  ComUInt32 udrFlags_;               // ComUdrFlags in ComSmallDefs.h
  Int32 routineOwnerId_;             // Owner of routine for Definer Rights
  char *parentQid_;                  // Query id of the CALL statement
  UdrParameterInfo *inParamInfo_;    // Info for IN/INOUT params
  UdrParameterInfo *outParamInfo_;   // Info for OUT/INOUT params
  ComUInt16 numInputTables_;          // number of child table inputs
  UdrTableInputInfo *inTables_;         // array of table inputs
  ComUInt32 numOptionalDataBufs_;    // Number of optional data buffers
  char **optionalData_;              // Optional data buffers
  NABoolean optionalDataIsShared_;   // Buffers owned by this instance?
  ComUInt32 numInstances_;           // num if instances of this udr tcb
  ComUInt32 instanceNum_;            // instance number of this udr tcb
  ComUInt32 udrSerInvocationInfoLen_; // serialized objects for new C++ interface
  const char *udrSerInvocationInfo_;
  ComUInt32 udrSerPlanInfoLen_;
  const char *udrSerPlanInfo_;
  Int32 udrJavaDebugPort_;           // port for Java debugger
  Int32 udrJavaDebugTimeout_;        // timeout to wait for Java debugger


private:

  //
  // Do not implement default constructors or an assignment operator
  //
  UdrLoadMsg();
  UdrLoadMsg(const UdrLoadMsg &);
  UdrLoadMsg &operator=(const UdrLoadMsg &);

}; // class UdrLoadMsg

class UdrLoadReply : public UdrControlMsg
{
public:
  UdrLoadReply(NAMemory *heap)
    : UdrControlMsg(UDR_MSG_LOAD_REPLY, UdrLoadReplyVersionNumber, heap)
  {
  }

  virtual ~UdrLoadReply()
  {
  }

private:

  //
  // Do not implement default constructors or an assignment operator
  //
  UdrLoadReply();
  UdrLoadReply(const UdrLoadReply &);
  UdrLoadReply &operator=(const UdrLoadReply &);

}; // class UdrLoadReply

class UdrUnloadMsg : public UdrControlMsg
{
public:
  UdrUnloadMsg(NAMemory *heap)
    : UdrControlMsg(UDR_MSG_UNLOAD, UdrUnloadMsgVersionNumber, heap)
  {
  }

  virtual ~UdrUnloadMsg()
  {
  }

private:

  //
  // Do not implement default constructors or an assignment operator
  //
  UdrUnloadMsg();
  UdrUnloadMsg(const UdrUnloadMsg &);
  UdrUnloadMsg &operator=(const UdrUnloadMsg &);

}; // class UdrUnloadMsg

class UdrUnloadReply : public UdrControlMsg
{
public:
  UdrUnloadReply(NAMemory *heap)
    : UdrControlMsg(UDR_MSG_UNLOAD_REPLY, UdrUnloadReplyVersionNumber, heap)
  {
  }

  virtual ~UdrUnloadReply()
  {
  }

private:

  //
  // Do not implement default constructors or an assignment operator
  //
  UdrUnloadReply();
  UdrUnloadReply(const UdrUnloadReply &);
  UdrUnloadReply &operator=(const UdrUnloadReply &);

}; // class UdrUnloadReply

//----------------------------------------------------------------------
// UdrSessionMsg and UdrSessionReply message subclasses
//
// The contents of a session message are a type field, a 32-bit flags
// field, and an arbitrary number of strings. The number of strings
// and their meaning depend on the message type. Currently we support
// the following types of session messages:
//
//   JVM startup options
//     string 1: a delimited series of JVM options
//     string 2: the set of delimiting characters
//
//----------------------------------------------------------------------
class UdrSessionMsg : public UdrControlMsg
{
public:

  enum UdrSessionAttrType
  {
    UDR_SESSION_TYPE_UNKNOWN = 0,
    UDR_SESSION_TYPE_JAVA_OPTIONS = 1
  };

  enum UdrSessionFlags
  {
    UDR_SESSION_FLAG_RESET = 0x0001
  };

  UdrSessionMsg(UdrSessionAttrType attrType,
                ComUInt32 flags,
                NAMemory *heap);

  UdrSessionMsg(NAMemory *heap);

  virtual ~UdrSessionMsg();

  void addString(const char *option);

  ComUInt32 numStrings() const { return strings_.entries(); }
  const char *getString(ComUInt32 i) const { return strings_[i]; }
  const char *operator[] (ComUInt32 i) const { return strings_[i]; }

  ComUInt32 getFlags() const { return flags_; }
  UdrSessionAttrType getType() const { return (UdrSessionAttrType) attrType_; }

  // Redefine pack/unpack methods from IpcMessageObj
  IpcMessageObjSize packedLength();
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);
  void unpackObj(IpcMessageObjType objType,
                 IpcMessageObjVersion objVersion,
                 NABoolean sameEndianness,
                 IpcMessageObjSize objSize,
                 IpcConstMessageBufferPtr buffer);

#ifdef UDR_DEBUG
  void display(FILE *f, const char *prefix) const;
#endif

protected:

  ComUInt32 attrType_;
  ComUInt32 flags_;
  LIST(char *) strings_;

private:

  // Do not implement default constructors or an assignment operator
  UdrSessionMsg();
  UdrSessionMsg(const UdrSessionMsg &);
  UdrSessionMsg &operator=(const UdrSessionMsg &);

}; // class UdrSessionMsg

class UdrSessionReply : public UdrControlMsg
{
public:
  UdrSessionReply(NAMemory *heap)
    : UdrControlMsg(UDR_MSG_SESSION_REPLY,
                    UdrSessionReplyVersionNumber, heap)
  {
  }

  virtual ~UdrSessionReply()
  {
  }

private:

  // Do not implement default constructors or an assignment operator
  UdrSessionReply();
  UdrSessionReply(const UdrSessionReply &);
  UdrSessionReply &operator=(const UdrSessionReply &);

}; // class UdrSessionReply

//----------------------------------------------------------------------
// UDR data header
//
// This header object currently contains no extra fields. It will
// precede a UDR data buffer in all invoke requests. The inherited
// UDR handle from superclass UdrMessageObj will be used in the UDR
// server to route the data buffer to the appropriate message stream.
//----------------------------------------------------------------------
class UdrDataHeader : public UdrMessageObj
{
public:

  // Constructor for allocation on a heap
  UdrDataHeader(const UdrHandle &h, NAMemory *heap)
    : UdrMessageObj(UDR_MSG_DATA_HEADER, UdrDataHeaderVersionNumber, heap)
  {
    setHandle(h);
 
  }

  // Constructor for copyless send
  UdrDataHeader(const UdrHandle &h)
    : UdrMessageObj(UDR_MSG_DATA_HEADER, UdrDataHeaderVersionNumber, NULL)
  {
    setHandle(h);
 
  }

  // Constructor for copyless receive
  UdrDataHeader(IpcBufferedMsgStream *msgStream)
    : UdrMessageObj(msgStream)
  {}

  virtual ~UdrDataHeader()
  {}
  
 

private:
  
  // Do not implement default constructors or an assignment operator
  UdrDataHeader();
  UdrDataHeader(const UdrDataHeader &);
  UdrDataHeader &operator=(const UdrDataHeader &);
  

}; // class UdrDataHeader
//----------------------------------------------------------------------
// UDR TMUDFdata header
//
// This header object currently contains no extra fields. It will
// precede a UDR data buffer in all invoke requests. The inherited
// UDR handle from superclass UdrMessageObj will be used in the UDR
// server to route the data buffer to the appropriate message stream.
//----------------------------------------------------------------------
class UdrTmudfDataHeader : public UdrMessageObj
{
public:

  // Constructor for allocation on a heap
  UdrTmudfDataHeader(const UdrHandle &h, NAMemory *heap)
    : UdrMessageObj(UDR_MSG_TMUDF_DATA_HEADER, UdrTmudfDataHeaderVersionNumber, heap)
  {
    setHandle(h);
 
  }

  // Constructor for copyless send
  UdrTmudfDataHeader(const UdrHandle &h)
    : UdrMessageObj(UDR_MSG_TMUDF_DATA_HEADER, UdrTmudfDataHeaderVersionNumber, NULL)
  {
    setHandle(h);
 
  }

  // Constructor for copyless receive
  UdrTmudfDataHeader(IpcBufferedMsgStream *msgStream)
    : UdrMessageObj(msgStream)
  {}

  virtual ~UdrTmudfDataHeader()
  {}
  
 

private:
  
  // Do not implement default constructors or an assignment operator
  UdrTmudfDataHeader();
  UdrTmudfDataHeader(const UdrTmudfDataHeader &);
  UdrTmudfDataHeader &operator=(const UdrTmudfDataHeader &);
  

}; // class UdrTmudfDataHeader

//----------------------------------------------------------------------
// UDR data buffers
//
// The packed format of this class is all IpcMessageObj members
// followed by a SqlBuffer.
//----------------------------------------------------------------------
class UdrDataBuffer : public UdrMessageObj
{
public:

  enum InOut { UDR_DATA_IN, UDR_DATA_OUT };

  enum UdrDataBufferFlags
  {
    UDR_DATA_BUFFER_LAST = 0x0001,   // Last reply buffer for a given request
    UDR_SEND_NEXT_DATA_BUFFER=0x0002, // flag set to indicate the client to send
                                    // the next data buffer. If FALSE this 
                                    // means client (ExUdrTCB) is supposed to 
                                    // send a continueRequest.
    UDR_SENDING_SCALAR_VALUES=0x0004 // flag to indicate the udrserever
                                     // that we are sending scalar values in
                                     //  this data buffer
  };

  //
  // Constructor for allocation on a heap or in a buffered stream
  //
  UdrDataBuffer(ULng32 sqlBufferLength, InOut mode, NAMemory *heap);

  //
  // Constructors for copyless receive
  //
  UdrDataBuffer(IpcBufferedMsgStream *msgStream,
                NABoolean driveUnPack = TRUE);
  UdrDataBuffer(/* IN  */ IpcBufferedMsgStream *msgStream,
                /* IN  */ IpcMessageObjSize objSize,
                /* OUT */ NABoolean &integrityCheckResult );

  virtual ~UdrDataBuffer();

  inline SqlBuffer *getSqlBuffer() { return theBuffer_; }
  inline const SqlBuffer *getSqlBuffer() const { return theBuffer_; }
  inline ULng32 getSqlBufferLength() const { return sqlBufferLength_; }
  inline void setSqlBuffer(SqlBuffer *sqlbuf)
  {
    theBuffer_ = sqlbuf;
  }
  NABoolean moreRows() const;

  // Each reply buffer carries a flag to indicate whether it is the
  // final reply buffer for a given request buffer. Our server-side
  // code sets the flag correctly the flag is
  // not actually used by client-side executor code.
  inline NABoolean isLastBuffer() const
  { return (flags_ & UDR_DATA_BUFFER_LAST) ? TRUE : FALSE; }

  inline void setLastBuffer(NABoolean yes)
  {
    flags_ = (yes) ? flags_ | UDR_DATA_BUFFER_LAST
                   : flags_ & ~UDR_DATA_BUFFER_LAST;
  }

  inline NABoolean sendMoreData() const
  { return (flags_ & UDR_SEND_NEXT_DATA_BUFFER) ? TRUE : FALSE;}

  inline void setSendMoreData(NABoolean yes)
  {
    flags_ = (yes) ? flags_ | UDR_SEND_NEXT_DATA_BUFFER
                   : flags_ & ~UDR_SEND_NEXT_DATA_BUFFER;
  }
  inline NABoolean sendingScalarValues() const
  {
   return (flags_ & UDR_SENDING_SCALAR_VALUES) ? TRUE : FALSE; 
  }
  inline void setSendingScalarValues(NABoolean yes)
  {
    flags_ = (yes) ? flags_ | UDR_SENDING_SCALAR_VALUES
                   : flags_ & ~UDR_SENDING_SCALAR_VALUES;
  }

  inline ComSInt16 tableIndex()
  { return tableIndex_;}
  
  inline void setTableIndex(ComSInt16 ind) { tableIndex_ = ind;}
  //
  // Virtual methods for use with buffered streams
  //
  virtual NABoolean msgObjIsFree();
  virtual void prepMsgObjForSend();

  //
  // Redefine pack/unpack methods from IpcMessageObj
  //
  IpcMessageObjSize packedLength();
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);
  void unpackObj(IpcMessageObjType objType,
                 IpcMessageObjVersion objVersion,
                 NABoolean sameEndianness,
                 IpcMessageObjSize objSize,
                 IpcConstMessageBufferPtr buffer);
  NABoolean checkObj(IpcMessageObjType t,
                     IpcMessageObjVersion v,
                     NABoolean sameEndianness,
                     IpcMessageObjSize size,
                     IpcConstMessageBufferPtr buffer) const;

protected:

  NABoolean copylessUnpack(NABoolean doChecks,
                           IpcMessageObjSize objSize,
                           NABoolean driveUnPack = TRUE);

  ComUInt32 flags_;
  ULng32 sqlBufferLength_;
  SqlBuffer *theBuffer_;
  ComSInt16 tableIndex_; // applies only to TMUDFs will be -1 for scalar udfs

private:

  //
  // Do not implement default constructors or an assignment operator
  //
  UdrDataBuffer();
  UdrDataBuffer(const UdrDataBuffer &);
  UdrDataBuffer &operator=(const UdrDataBuffer &);

}; // class UdrDataBuffer

//----------------------------------------------------------------------
// UDR continue request
//
// Currently a continue request contains no extra fields. The inherited
// UDR handle from superclass UdrMessageObj will be used in the UDR
// server to route the request to the appropriate data stream.
//----------------------------------------------------------------------
class UdrContinueMsg : public UdrMessageObj
{
public:
  //
  // Constructor for allocation on a heap
  //
  UdrContinueMsg(const UdrHandle &h, NAMemory *heap)
    : UdrMessageObj(UDR_MSG_CONTINUE_REQUEST, UdrContinueMsgVersionNumber, heap)
  {
    setHandle(h);
  }

  //
  // Constructor for copyless send
  //
  UdrContinueMsg(const UdrHandle &h)
    : UdrMessageObj(UDR_MSG_CONTINUE_REQUEST, UdrContinueMsgVersionNumber, NULL)
  {
    setHandle(h);
    
  }

  //
  // Constructor for copyless receive
  //
  UdrContinueMsg(IpcBufferedMsgStream *msgStream)
    : UdrMessageObj(msgStream)
  {}

  virtual ~UdrContinueMsg()
  {}

private:

  //
  // Do not implement default constructors or an assignment operator
  //
  UdrContinueMsg();
  UdrContinueMsg(const UdrContinueMsg &);
  UdrContinueMsg &operator=(const UdrContinueMsg &);

}; // class UdrContinueMsg

class UdrErrorReply : public UdrControlMsg
{
public:
  UdrErrorReply(NAMemory *heap)
    : UdrControlMsg(UDR_MSG_ERROR_REPLY, UdrErrorReplyVersionNumber, heap)
  {
  }

  virtual ~UdrErrorReply()
  {
  }

private:

  //
  // Do not implement default constructors or an assignment operator
  //
  UdrErrorReply();
  UdrErrorReply(const UdrErrorReply &);
  UdrErrorReply &operator=(const UdrErrorReply &);

}; // class UdrErrorReply

//----------------------------------------------------------------------
// Following are Transaction related messages
//  Enter Tx, Enter Tx reply
//  Exit Tx, Exit Tx reply
//----------------------------------------------------------------------
class UdrEnterTxMsg : public UdrControlMsg
{
public:
  UdrEnterTxMsg(NAMemory *heap)
    : UdrControlMsg(UDR_MSG_ENTER_TX, UdrEnterTxMsgVersionNumber, heap)
  {
  }

  virtual ~UdrEnterTxMsg()
  {
  }

private:
  //
  // Do not implement default constructors or an assignment operator
  //
  UdrEnterTxMsg();
  UdrEnterTxMsg(const UdrEnterTxMsg &);
  UdrEnterTxMsg &operator=(const UdrEnterTxMsg &);
}; // class UdrEnterTxMsg

class UdrEnterTxReply : public UdrControlMsg
{
public:
  UdrEnterTxReply(NAMemory *heap)
    : UdrControlMsg(UDR_MSG_ENTER_TX_REPLY, UdrEnterTxReplyVersionNumber, heap)
  {
  }

  virtual ~UdrEnterTxReply()
  {
  }

private:
  //
  // Do not implement default constructors or an assignment operator
  //
  UdrEnterTxReply();
  UdrEnterTxReply(const UdrEnterTxReply &);
  UdrEnterTxReply &operator=(const UdrEnterTxReply &);
}; // class UdrEnterTxReply

class UdrSuspendTxMsg : public UdrControlMsg
{
  typedef UdrControlMsg super;

public:
  UdrSuspendTxMsg(NAMemory *heap)
    : UdrControlMsg(UDR_MSG_SUSPEND_TX, UdrSuspendTxMsgVersionNumber, heap)
  {
  }

  virtual ~UdrSuspendTxMsg()
  {
  }

private:
  // Do not implement default constructors or an assignment operator
  UdrSuspendTxMsg();
  UdrSuspendTxMsg(const UdrSuspendTxMsg &);
  UdrSuspendTxMsg &operator=(const UdrSuspendTxMsg &);

}; // class UdrSuspendTxMsg

class UdrSuspendTxReply : public UdrControlMsg
{
public:
  UdrSuspendTxReply(NAMemory *heap)
    : UdrControlMsg(UDR_MSG_SUSPEND_TX_REPLY,
                    UdrSuspendTxReplyVersionNumber, heap)
  {
  }

  virtual ~UdrSuspendTxReply()
  {
  }

private:
  // Do not implement default constructors or an assignment operator
  UdrSuspendTxReply();
  UdrSuspendTxReply(const UdrSuspendTxReply &);
  UdrSuspendTxReply &operator=(const UdrSuspendTxReply &);

}; // class UdrSuspendTxReply

class UdrExitTxMsg : public UdrControlMsg
{
  typedef UdrControlMsg super;

public:
  UdrExitTxMsg(NAMemory *heap)
    : UdrControlMsg(UDR_MSG_EXIT_TX, UdrExitTxMsgVersionNumber, heap)
  {
  }

  virtual ~UdrExitTxMsg()
  {
  }

private:
  //
  // Do not implement default constructors or an assignment operator
  //
  UdrExitTxMsg();
  UdrExitTxMsg(const UdrExitTxMsg &);
  UdrExitTxMsg &operator=(const UdrExitTxMsg &);
}; // class UdrExitTxMsg

class UdrExitTxReply : public UdrControlMsg
{
public:
  UdrExitTxReply(NAMemory *heap)
    : UdrControlMsg(UDR_MSG_EXIT_TX_REPLY, UdrExitTxReplyVersionNumber, heap)
  {
  }

  virtual ~UdrExitTxReply()
  {
  }

private:
  //
  // Do not implement default constructors or an assignment operator
  //
  UdrExitTxReply();
  UdrExitTxReply(const UdrExitTxReply &);
  UdrExitTxReply &operator=(const UdrExitTxReply &);
}; // class UdrExitTxReply

//----------------------------------------------------------------------
// This class adds a data member for RS Handle and is base class
// for all RS message classes.
//----------------------------------------------------------------------
class UdrRSMessageObj : public UdrMessageObj
{
  typedef UdrMessageObj super;

public:

  // Constructor for allocation on a heap
  UdrRSMessageObj(UdrIpcObjectType msgType,
                  IpcMessageObjVersion version,
                  NAMemory *heap)
    : UdrMessageObj(msgType, version, heap),
      rshandle_(INVALID_RS_HANDLE)
  {
  }

  // Constructor for copyless send
  UdrRSMessageObj(UdrIpcObjectType msgType,
                  IpcMessageObjVersion version)
    : UdrMessageObj(msgType, version, NULL),
      rshandle_(INVALID_RS_HANDLE)
  {
  }

  // Constructor for copyless receive
  UdrRSMessageObj(IpcBufferedMsgStream *msgStream)
    : UdrMessageObj(msgStream)
  {
  }

  virtual ~UdrRSMessageObj()
  {
  }

  // Accessor/Mutator methods
  inline const RSHandle &getRSHandle() const { return rshandle_; }
  inline void setRSHandle(const RSHandle &h) { rshandle_ = h; }

  // Pack/unpack/check methods in the IpcMessageObj interface
  IpcMessageObjSize packedLength();
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);
  void unpackObj(IpcMessageObjType objType,
                 IpcMessageObjVersion objVersion,
                 NABoolean sameEndianness,
                 IpcMessageObjSize objSize,
                 IpcConstMessageBufferPtr buffer);
  NABoolean checkObj(IpcMessageObjType t,
                     IpcMessageObjVersion v,
                     NABoolean sameEndianness,
                     IpcMessageObjSize size,
                     IpcConstMessageBufferPtr buffer) const;

protected:

  // Pack/unpack/check methods in the UdrMessageObj interface
  virtual IpcMessageObjSize udrBaseClassPackedLength();
  virtual IpcMessageObjSize packUdrBaseClass(IpcMessageBufferPtr &buffer);
  virtual void unpackUdrBaseClass(IpcConstMessageBufferPtr &buffer);
  virtual NABoolean checkUdrBaseClass(IpcMessageObjType t,
                                      IpcMessageObjVersion v,
                                      NABoolean sameEndianness,
                                      IpcMessageObjSize size,
                                      IpcConstMessageBufferPtr &buffer) const;

private:

  // Do not implement default constructors or an assignment operator
  UdrRSMessageObj();
  UdrRSMessageObj(const UdrRSMessageObj &);
  UdrRSMessageObj &operator=(const UdrRSMessageObj &);

  RSHandle rshandle_;

}; // class UdrRSMessageObj

//----------------------------------------------------------------------
// Currently this class provides no functionality. It just gives us
// a way to distinguish a control message from a data message.
//----------------------------------------------------------------------
class UdrRSControlMsg : public UdrRSMessageObj
{
public:
  UdrRSControlMsg(UdrIpcObjectType msgType, IpcMessageObjVersion version,
    NAMemory *heap)
    : UdrRSMessageObj(msgType, version, heap)
  {
  }

  virtual ~UdrRSControlMsg() {}

private:
  //
  // Do not implement default constructors or an assignment operator
  //
  UdrRSControlMsg();
  UdrRSControlMsg(const UdrRSControlMsg &);
  UdrRSControlMsg &operator=(const UdrRSControlMsg &);
}; // class UdrRSControlMsg

//----------------------------------------------------------------------
// Following are RS control messages
//  RS Load, RS Load reply
//  RS Continue
//  RS Close, RS Close reply
//  RS Unload, RS Unload reply
//----------------------------------------------------------------------
class UdrRSLoadMsg : public UdrRSControlMsg
{
  typedef UdrRSControlMsg super;

public:
  UdrRSLoadMsg(NAMemory *heap);

  UdrRSLoadMsg(ComUInt32 rsIndex, ComUInt32 numRSCols, ComUInt32 rowSize,
               ComUInt32 bufferSize, ComUInt32 flags, NAMemory *heap);

  virtual ~UdrRSLoadMsg();

  // Accessor methods
  inline ComUInt32 getRsIndex() const { return rsIndex_; }
  inline ComUInt32 getNumRSColumns() const { return numRSCols_; }
  inline ComUInt32 getRowSize() const { return outputRowSize_; }
  inline ComUInt32 getBufferSize() const { return outBufferSize_; }
  inline ComUInt32 getFlags() const { return rsLoadFlags_; }

  // Methods to get/set column desc structures
  UdrParameterInfo *getColumnDesc() const
  {
    return rsColumnDesc_;
  }
  UdrParameterInfo &getColumnDesc(ComUInt32 i) const;
  UdrParameterInfo *setColumnDesc(ComUInt32 i, const UdrParameterInfo &info);

  //
  // Redefine pack/unpack methods from IpcMessageObj
  //
  IpcMessageObjSize packedLength();
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr buffer);
  void unpackObj(IpcMessageObjType objType,
    IpcMessageObjVersion objVersion,
    NABoolean sameEndianness,
    IpcMessageObjSize objSize,
    IpcConstMessageBufferPtr buffer);

private:
  //
  // Do not implement default constructors or an assignment operator
  //
  UdrRSLoadMsg();
  UdrRSLoadMsg(const UdrRSLoadMsg &);
  UdrRSLoadMsg &operator=(const UdrRSLoadMsg &);

  void allocateColumnDescs();
  void deallocateColumnDescs();

  ComUInt32 rsIndex_;

  //
  // UDR server needs similar information for RS columns that we have
  // in UdrParameterInfo. So UdrParameterInfo is reused here.
  //
  UdrParameterInfo *rsColumnDesc_;
  ComUInt32 numRSCols_;                // Number of Columns in the RS
  ComUInt32 outputRowSize_;            // RS row size
  ComUInt32 outBufferSize_;            // Buffer size that UDR server uses
                                       // to send multiple rows
  ComUInt32 rsLoadFlags_;              // Flags for future use

}; // class UdrRSLoadMsg

class UdrRSLoadReply : public UdrRSControlMsg
{
public:
  UdrRSLoadReply(NAMemory *heap)
    : UdrRSControlMsg(UDR_MSG_RS_LOAD_REPLY, UdrRSLoadReplyVersionNumber, heap)
  {
  }

  virtual ~UdrRSLoadReply()
  {
  }

private:
  //
  // Do not implement default constructors or an assignment operator
  //
  UdrRSLoadReply();
  UdrRSLoadReply(const UdrRSLoadReply &);
  UdrRSLoadReply &operator=(const UdrRSLoadReply &);
}; // class UdrRSLoadReply

//----------------------------------------------------------------------
// RS data header
//
// This header object derives from UDRRSMessageObj and contains no
// extra fields. It will precede the data buffer in all RS invoke
// requests. The inherited handles from parent class UdrRSMessageObj
// will be used in the UDR server to route the data buffer to the
// appropriate message stream.
//----------------------------------------------------------------------
class UdrRSDataHeader : public UdrRSMessageObj
{
public:

  // Constructor for allocation on a heap
  UdrRSDataHeader(const UdrHandle &h, const RSHandle &rs, NAMemory *heap)
    : UdrRSMessageObj(UDR_MSG_RS_DATA_HEADER,
                      UdrRSDataHeaderVersionNumber,
                      heap)
  {
    setHandle(h);
    setRSHandle(rs);
  }

  // Constructor for copyless send
  UdrRSDataHeader(const UdrHandle &h, const RSHandle &rs)
    : UdrRSMessageObj(UDR_MSG_RS_DATA_HEADER,
                      UdrRSDataHeaderVersionNumber,
                      NULL)
  {
    setHandle(h);
    setRSHandle(rs);
  }

  // Constructor for copyless receive
  UdrRSDataHeader(IpcBufferedMsgStream *msgStream)
    : UdrRSMessageObj(msgStream)
  {}

  virtual ~UdrRSDataHeader()
  {}

private:

  // Do not implement default constructors or an assignment operator
  UdrRSDataHeader();
  UdrRSDataHeader(const UdrRSDataHeader &);
  UdrRSDataHeader &operator=(const UdrRSDataHeader &);

}; // class UdrRSDataHeader

class UdrRSContinueMsg : public UdrRSMessageObj
{
public:

  // Constructor for allocation on a heap
  UdrRSContinueMsg(NAMemory *heap)
    : UdrRSMessageObj(UDR_MSG_RS_CONTINUE, UdrRSContinueMsgVersionNumber, heap)
  {
  }

  // Constructor for copyless send
  UdrRSContinueMsg(const UdrHandle &handle, const RSHandle &rsHandle)
    : UdrRSMessageObj(UDR_MSG_RS_CONTINUE, UdrRSContinueMsgVersionNumber)
  {
    setHandle(handle);
    setRSHandle(rsHandle);
  }

  // Constructor for copyless receive
  UdrRSContinueMsg(IpcBufferedMsgStream *msgStream)
    : UdrRSMessageObj(msgStream)
  {
  }

  virtual ~UdrRSContinueMsg()
  {
  }

private:
  //
  // Do not implement default constructors or an assignment operator
  //
  UdrRSContinueMsg();
  UdrRSContinueMsg(const UdrRSContinueMsg &);
  UdrRSContinueMsg &operator=(const UdrRSContinueMsg &);
}; // class UdrRSContinueMsg

class UdrRSCloseMsg : public UdrRSControlMsg
{
public:
  UdrRSCloseMsg(NAMemory *heap)
    : UdrRSControlMsg(UDR_MSG_RS_CLOSE, UdrRSCloseMsgVersionNumber, heap)
  {
  }

  virtual ~UdrRSCloseMsg()
  {
  }

private:
  //
  // Do not implement default constructors or an assignment operator
  //
  UdrRSCloseMsg();
  UdrRSCloseMsg(const UdrRSCloseMsg &);
  UdrRSCloseMsg &operator=(const UdrRSCloseMsg &);
}; // class UdrRSCloseMsg

class UdrRSCloseReply : public UdrRSControlMsg
{
public:
  UdrRSCloseReply(NAMemory *heap)
    : UdrRSControlMsg(UDR_MSG_RS_CLOSE_REPLY, UdrRSCloseReplyVersionNumber, heap)
  {
  }

  virtual ~UdrRSCloseReply()
  {
  }

private:
  //
  // Do not implement default constructors or an assignment operator
  //
  UdrRSCloseReply();
  UdrRSCloseReply(const UdrRSCloseReply &);
  UdrRSCloseReply &operator=(const UdrRSCloseReply &);
}; // class UdrRSColseReply

class UdrRSUnloadMsg : public UdrRSControlMsg
{
public:
  UdrRSUnloadMsg(NAMemory *heap)
    : UdrRSControlMsg(UDR_MSG_RS_UNLOAD, UdrRSUnloadMsgVersionNumber, heap)
  {
  }

  virtual ~UdrRSUnloadMsg()
  {
  }

private:
  //
  // Do not implement default constructors or an assignment operator
  //
  UdrRSUnloadMsg();
  UdrRSUnloadMsg(const UdrRSUnloadMsg &);
  UdrRSUnloadMsg &operator=(const UdrRSUnloadMsg &);
}; // class UdrRSUnloadMsg

class UdrRSUnloadReply : public UdrRSControlMsg
{
public:
  UdrRSUnloadReply(NAMemory *heap)
    : UdrRSControlMsg(UDR_MSG_RS_UNLOAD_REPLY, UdrRSUnloadReplyVersionNumber, heap)
  {
  }

  virtual ~UdrRSUnloadReply()
  {
  }

private:
  //
  // Do not implement default constructors or an assignment operator
  //
  UdrRSUnloadReply();
  UdrRSUnloadReply(const UdrRSUnloadReply &);
  UdrRSUnloadReply &operator=(const UdrRSUnloadReply &);
}; // class UDRRSUnloadReply

//----------------------------------------------------------------------
// struct ResultSetInfo
// A place to store all the result set info that's passed from UDR
// server to Executor. Currently we store info such as
//  * proxy statement text
//  * Statement handle   // Needs more work
//  * Context handle     // --ditto--
//
// Note: The constructor callers of this struct should make sure that
// heap_ pointer is set. This is important if the object is instantiated
// in (release version) Executor where global new is not available.
// So setHeap() must be called after an object is instantiated
// using default constructor.
//----------------------------------------------------------------------
class ResultSetInfo
{
  friend class UdrRSInfoMsg;

public:
  ResultSetInfo(const char *proxy, SQLSTMT_ID *stmtID,
                ComUInt32 cntxID, NAMemory *heap);

  ~ResultSetInfo();

  // Accessor methods
  const char *getProxySyntax() const { return proxySyntax_; }
  SQLSTMT_ID *getStmtID() const { return stmtID_; }
  ComUInt32 getContextID() const { return cntxID_; }

  // Assignment operator
  ResultSetInfo& operator=(const ResultSetInfo &src);

protected:
  //
  // This class does not derive from IpcMessageObj but we need
  // similar methods to pack/unpack the object in a message
  // buffer. Only UdrRSInfoMsg objects will call these
  // methods, when it is packing/unpacking itself.
  //
  IpcMessageObjSize packedLength();
  IpcMessageObjSize pack(IpcMessageBufferPtr &buffer) const;
  void unpack(IpcConstMessageBufferPtr &buffer);

  static
  NABoolean checkResultSetInfoClass(IpcMessageObjType t,
                                      IpcMessageObjVersion v,
                                      NABoolean sameEndianness,
                                      IpcMessageObjSize size,
                                      IpcConstMessageBufferPtr &buffer);

private:
  // Only friends can call default constructor and setHeap()
  ResultSetInfo()
    : heap_(NULL), proxySyntax_(NULL), stmtID_(NULL), cntxID_(0)
  {
  }

  // Helper functions for memory management
  char *allocateString(const char *s);
  void deallocateString(char *&s);

  void setHeap(NAMemory *heap) { heap_ = heap; }

  NAMemory *heap_;

  char *proxySyntax_;     // proxy statement text

  // TBD: Ramana
  // The following information is added for debugging/measure data
  // We have not completely understood what information needs to be here.
  // Right now we have the following but these values will be set to 0
  // by UDR Server and we should not depend on these fields right now.
  // This part of this class will be updated later.
  SQLSTMT_ID *stmtID_;      // Pointer to Stmt_ID struct
  ComUInt32 cntxID_;      // context handle where stmtID_ is valid
}; // struct ResultSetInfo

//----------------------------------------------------------------------
// UdrRSInfoMsg will be sent by UDR server along with UDR INVOKE
// reply message. This object contains one or more ResultSetInfo structs
//----------------------------------------------------------------------
class UdrRSInfoMsg : public UdrMessageObj
{
  typedef UdrMessageObj super;

public:
  UdrRSInfoMsg(NAMemory *heap)
    : UdrMessageObj(UDR_MSG_RS_INFO, UdrRSInfoMsgVersionNumber, heap),
      numRS_(0), resultSetInfo_(NULL)
  {
  }

  UdrRSInfoMsg(ComUInt32 numRS, NAMemory *heap);

  virtual ~UdrRSInfoMsg();

  // Redefine pack/unpack methods from IpcMessageObj
  IpcMessageObjSize packedLength();
  IpcMessageObjSize packObjIntoMessage(IpcMessageBufferPtr b);
  void unpackObj(IpcMessageObjType objType,
                 IpcMessageObjVersion objVersion,
                 NABoolean sameEndianness,
                 IpcMessageObjSize objSize,
                 IpcConstMessageBufferPtr buffer);

  NABoolean checkObj(IpcMessageObjType t,
                     IpcMessageObjVersion v,
                     NABoolean sameEndianness,
                     IpcMessageObjSize size,
                     IpcConstMessageBufferPtr buffer) const;

  // Accessor methods
  inline ComUInt32 getNumResultSets() const { return numRS_; }

  // Methods to get/set ResultSet info structures
  ResultSetInfo &getRSInfo(ComUInt32 i) const;
  void setRSInfo(ComUInt32 i, const ResultSetInfo &info);

protected:
  virtual NABoolean checkUdrBaseClass(IpcMessageObjType t,
                                      IpcMessageObjVersion v,
                                      NABoolean sameEndianness,
                                      IpcMessageObjSize size,
                                      IpcConstMessageBufferPtr &buffer) const;

private:
  // Number of result sets returned by the UDR
  ComUInt32 numRS_;
  ResultSetInfo *resultSetInfo_;

  // Helper functions for memory management of ResultSet
  // info arrays
  void allocateResultSetInfo();
  void deallocateResultSetInfo();

  //
  // Do not implement default constructors or an assignment operator
  //
  UdrRSInfoMsg();
  UdrRSInfoMsg(const UdrRSInfoMsg &);
  UdrRSInfoMsg &operator=(const UdrRSInfoMsg &);

}; // class UdrRSInfoMsg

#endif // _UDR_EXE_IPC_H_
