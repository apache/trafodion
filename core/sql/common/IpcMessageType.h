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
#ifndef IPCMESSAGETYPE_H
#define IPCMESSAGETYPE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         IpcMessageType.h
 * Description:  Associate numbers with SQL/ARK message types
 *
 * Created:      10/17/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// This file defines simple data types used for exchanging messages
// between processes. Each message has a message header and a sequence of
// zero or more other message objects in it (the header is a message
// object itself). Each object that a user wants to add to a message
// needs to be a class (or struct, or union), derived from a base
// class, IpcMessageObject. That base class adds certain information to
// the object, namely:
//
// - an object type (can be used to call the correct constructor at
//   the receiving side, since we can't automatically send virtual
//   function pointers over the net)
// - an object version, to allow interaction between processes of
//   different releases
// - an object length
// - a reference count, used to count the users of an object that is
//   shared by multiple components or processes
//
// This file describes the basic types used for the above information:
//
// IpcMessageType             Enum type for message header ranges
// IpcMessageObjType          Type of an object in the message
// IpcMessageObjVersion       Version of a message object
// IpcMessageObjSize          Size of a message object
// IpcMessageObjRefCount      Reference count to a message object
//
// Furthermore, some additional types to identify network domains
// and data representation across different hardware platforms are
// declared here as well. Note that not all of the hardware platforms
// are actually supported by the current implementation.
// -----------------------------------------------------------------------

#include "Platform.h"

// -----------------------------------------------------------------------
// Forward references
// -----------------------------------------------------------------------
class IpcMessageStreamBase;
class IpcMessageStream;
class IpcMessageBuffer;
class IpcConnection;

// -----------------------------------------------------------------------
// We want to support interoperation, so most data structures can
// handle both Guardian and Unix-style personalities. An enumeration
// type tells which communication protocol we are using.
// -----------------------------------------------------------------------
typedef enum IpcNetworkDomainEnum
{
  IPC_DOM_INVALID,
  IPC_DOM_GUA_PHANDLE,
  IPC_DOM_INTERNET
} IpcNetworkDomain;

// -----------------------------------------------------------------------
// Types of server processes supported (see file ExIPC.C for the
// logic that associates a name with each literal) and default port
// numbers for these servers in case sockets are used
// -----------------------------------------------------------------------
typedef enum IpcServerTypeEnum
{
  IPC_CLIENT_OR_UNSPECIFIED_SERVER = -1,
  IPC_SQLUSTAT_SERVER = 0,
  IPC_SQLCAT_SERVER,
  IPC_SQLCOMP_SERVER,
  IPC_SQLESP_SERVER,
  IPC_SQLUDR_SERVER,
  IPC_GENERIC_SERVER,
  IPC_SQLSSCP_SERVER,
  IPC_SQLSSMP_SERVER,
  IPC_SQLQMS_SERVER,
  IPC_SQLQMP_SERVER,
  IPC_SQLQMM_SERVER,
  IPC_SQLBDRS_SERVER,  // persistent BDR Service
  IPC_SQLBDRR_SERVER   // BDR replicator process
} IpcServerType;

typedef enum IpcServerPortNumberEnum
{
  IPC_INVALID_SERVER_PORTNUMBER  = 0,
  IPC_SQLUSTAT_PORTNUMBER        = 3692,
  IPC_SQLUSTAT_DEBUG_PORTNUMBER  = 3693,
  IPC_SQLCAT_PORTNUMBER          = 3694,
  IPC_SQLCAT_DEBUG_PORTNUMBER    = 3695,
  IPC_SQLCOMP_PORTNUMBER         = 3696,
  IPC_SQLCOMP_DEBUG_PORTNUMBER   = 3697,
  IPC_SQLESP_PORTNUMBER          = 3698,
  IPC_SQLESP_DEBUG_PORTNUMBER    = 3699,
  IPC_SQLUDR_PORTNUMBER          = 3700,
  IPC_SQLUDR_DEBUG_PORTNUMBER    = 3701,
  IPC_GENERIC_PORTNUMBER         = 3702,
  IPC_GENERIC_DEBUG_PORTNUMBER   = 3703,
  IPC_SQLQMS_PORTNUMBER          = 3704,
  IPC_SQLQMS_DEBUG_PORTNUMBER    = 3705,
  IPC_SQLQMP_PORTNUMBER          = 3706,
  IPC_SQLQMP_DEBUG_PORTNUMBER    = 3707,
  IPC_SQLQMM_PORTNUMBER          = 3708,
  IPC_SQLQMM_DEBUG_PORTNUMBER    = 3709
} IpcServerPortNumber;

// -----------------------------------------------------------------------
// Methods to allocate server processes. Right now we support Guardian
// servers that are created using GUARDIAN PROCESS_LAUNCH_ and OSS
// processes that are created with PROCESS_SPAWN_. For both OSS and Unix
// we support processes that are created by an inetd running on a machine
// and listening to a port listed in /etc/services. Another supported
// method (local CPU only) is simply to do a fork(), followed by exec().
// In the first two cases the className is the name of a (Guardian/OSS)
// object file, in the third case it's the name of a network service, in
// the fourth case it's the name of an object file. The allocation method
// is determined from defaults, unless specified by the caller.
// -----------------------------------------------------------------------
//
// -----------------------------------------------------------------------

typedef enum IpcServerAllocationMethodEnum
{
  IPC_ALLOC_DONT_CARE, // system decides
  IPC_LAUNCH_GUARDIAN_PROCESS,
  IPC_SPAWN_OSS_PROCESS,
  IPC_INETD,
  IPC_POSIX_FORK_EXEC,
  IPC_LAUNCH_NT_PROCESS,
  IPC_USE_PROCESS
} IpcServerAllocationMethod;

// -----------------------------------------------------------------------
// Message types used by this protocol.
//
// Rather than recompiling everything when this changes, one could also
// reserve ranges of numbers in this file and manage them separately.
// To allow this, the type definition for a message type is not the
// enum listed below, but a long.
// -----------------------------------------------------------------------
enum IpcMessageTypeEnum
{
  // messages to and from the SQL catalog manager
  // for the actual message types see file ???
  IPC_MSG_SQLCAT_FIRST = 2000,
  IPC_MSG_SQLCAT_LAST = 2999,

  // messages to and from the SQL compiler process
  // for the actual mesage types see file ???
  IPC_MSG_SQLCOMP_FIRST = 3000,
  IPC_MSG_SQLCOMP_LAST = 3999,

  // messages to and from SQL ESP processes
  // for the actual message types see file ../executor/ex_esp_msg.h
  IPC_MSG_SQLESP_FIRST = 4000,
  IPC_MSG_SQLESP_LAST = 4999,

  // messages to and from the SQL shadow process from the CLI (Windows/NT only)
  // for the actual message types see file ???
  IPC_MSG_SQLCLI_FIRST = 5000,
  IPC_MSG_SQLCLI_LAST = 5999,

  // messages to and from the SQL update statistics server
  // for the actual message types see file ???
  IPC_MSG_SQLUSTAT_FIRST = 6000,
  IPC_MSG_SQLUSTAT_LAST = 6999,

  // misc. message types, used mainly for testing and debugging
  IPC_MSG_INVALID = 7000,
  IPC_MSG_TEST = 7001,
  IPC_MSG_TEST_REPLY = 7002,

  // messages to and from SQL UDR Server processes
  // for the actual message types see file ../executor/ExUdrMsg.h
  IPC_MSG_SQLUDR_FIRST = 8000,
  IPC_MSG_SQLUDR_LAST = 8999,

  IPC_MSG_RTS_FIRST = 9000,
  IPC_MSG_RTS_LAST  = 9999,

  // messages among processes involved in query rewrite
  // for the actual message types, see file ../qmscommon/QRIpc.h
  IPC_MSG_QR_FIRST = 10000,
  IPC_MSG_QR_LAST  = 10999


};

typedef Int32  IpcMessageType;

// -----------------------------------------------------------------------
// Types for objects inside messages (a different space of numbers for
// each message type, if so desired). Procedures that deal with message
// types and message object types all take parameters of type
// IpcMessageObjType, since enum types are non-portable. This also
// allows us to manage the enum types separately per component of the
// SQL system without forcing global recompiles on small changes.
// -----------------------------------------------------------------------
typedef Int32  IpcMessageObjType;

// -----------------------------------------------------------------------
// Version of a message header or of an object in a message
// -----------------------------------------------------------------------
typedef Int32  IpcMessageObjVersion;

// -----------------------------------------------------------------------
// Reserved numbers for certain common objects in messages, to be
// used as values for variables of type IpcMessageObjType. Do not use
// the reserved values below in your own enum types for message object
// types!!!
// -----------------------------------------------------------------------
enum IpcCommonObjectTypeEnum
{
  IPC_COMMON_OBJ_TYPE_START = 13500,

  IPC_SQL_DIAG_AREA         = 13501,
  IPC_SQL_STATISTICS        = 13502,
  IPC_PROCESS_ID            = 13503,
  IPC_SQL_CONDITION         = 13504,
  IPC_SQL_STATS_AREA        = 13505,
  IPC_SQL_STATS_ENTRY       = 13506,
  IPC_SQL_STATS_COUNTER     = 13507,
  IPC_SQL_STATS_TIMER       = 13508,

  IPC_COMMON_OBJ_TYPE_END   = 13999
};

// -----------------------------------------------------------------------
// Versions of the common objects
// -----------------------------------------------------------------------

const IpcMessageObjVersion IpcCurrSqlDiagnosticsAreaVersion = 100;
// Stats are version 0 in Release 1, version 100 in Release 1.5
const IpcMessageObjVersion IpcCurrSqlStatisticsVersion      = 100;
const IpcMessageObjVersion IpcCurrProcessIdVersion          = 100;

typedef Int32  IpcMessageType;

// -----------------------------------------------------------------------
// size of objects in bytes
// -----------------------------------------------------------------------

typedef UInt32  IpcMessageObjSize;

// -----------------------------------------------------------------------
// Reference count of a message header or of an object in a message
// (used mainly when objects are shared across the IPC interface)
// -----------------------------------------------------------------------
typedef Int32   IpcMessageRefCount;

// -----------------------------------------------------------------------
// A buffer pointer to a raw, byte-adressable  message buffer, used for
// packing and unpacking of objects contained in message buffers
// -----------------------------------------------------------------------
typedef char *IpcMessageBufferPtr;
typedef const char *IpcConstMessageBufferPtr;

// -----------------------------------------------------------------------
// Endianness (define literals for all options and for what this
// process is using)
// -----------------------------------------------------------------------

const char IpcLittleEndian = 1;
const char IpcBigEndian    = 2;

#ifdef NA_LITTLE_ENDIAN
const char IpcMyEndianness = IpcLittleEndian;
#else
const char IpcMyEndianness = IpcBigEndian;
#endif

// -----------------------------------------------------------------------
// Data alignment
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Alignment8 means that bytes are on any address, 16 bit numbers
// are on even addresses, 32 bit numbers are on addresses divisible
// by 4, and 64 bit numbers are on addresses divisible by 8.
// -----------------------------------------------------------------------
const short IpcAlignment8 = 8;

// -----------------------------------------------------------------------
// right now, everybody is using Alignment8 and our message structures
// are laid out to have no fillers when this alignment is used
// -----------------------------------------------------------------------
const short IpcMyAlignment = IpcAlignment8;

// -----------------------------------------------------------------------
// timeout value in (long, expressed in 10 ms units,
// a value of 100 means 1 sec), 0 means return immediately,
// IpcInfiniteTimeout waits forever
// -----------------------------------------------------------------------
typedef Int32  IpcTimeout;
const IpcTimeout IpcImmediately = 0;
const IpcTimeout IpcInfiniteTimeout = -1;

// -----------------------------------------------------------------------
// CPU number in a node
// -----------------------------------------------------------------------
typedef Int32  IpcCpuNum;

// an invalid CPU number in a node
const IpcCpuNum IPC_CPU_DONT_CARE = -1;

// -----------------------------------------------------------------------
// Priority of a started process
// -----------------------------------------------------------------------
typedef Int32  IpcPriority;

const IpcPriority IPC_PRIORITY_DONT_CARE = -1;

#endif /* IPCMESSAGETYPE_H */
