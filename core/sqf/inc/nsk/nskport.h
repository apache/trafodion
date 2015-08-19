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
/*++

Module name:

NSKport.h

Abstract:

   This module defines the types and constants that are defined by the
     Tandem NSK port suite.

Revision History:

--*/

#ifndef _NSK_port_
#define _NSK_port_

#include "rosetta/rosgen.h"
#include "nsk/nskmem.h"

#include "seabed/int/types.h"

#ifdef NA_64BIT
// dg64 - these are equivalent, but this one's easier to see
#pragma pack(push, enter_NSK_port, 4)
#else
#pragma pack(push, enter_NSK_port)

#pragma pack(4)
#endif

// -----------------------------------------
//
//NRL - named resource table - for msg ports
//
// -----------------------------------------

#define NRL_NAME_MAX_LENGTH 30


// ----------------------------------------------------------------------------------
//
// NSK port handle
//
// note: here we marry platform's process_handle definition (as NSK_phandle_internal_Rosetta defined
//            in jphanz.h) with our NSK-NT process_phandle and named it as NSK_PORT_HANDLE.
//            All the clients module will point to (or use)NSK_PORT_HANDLE in their codes.
//            Also, here we kind of merging in jphanz.h and make NSKport.h a superset of jphanz.h.
//            We do so in the hope that NSKport.h should be one-stop place for all client's need for
//            phandle related definition.
//
// ----------------------------------------------------------------------------------

// values for TYPE (below in phandle replacement)

enum {
   PROCESSHANDLE_INVALID = 0,
    PROCESSHANDLE_UNNAMED_ = 1,
    PROCESSHANDLE_NAMED_ = 2,
    PROCESSHANDLE_OLD_UNNAMED_ = 3,    // now invalid
    PROCESSHANDLE_OLD_NAMED_ = 4,      // now invalid
    PROCESSHANDLE_OLD_LDEV_ = 5,       // now invalid
    PROCESSHANDLE_NULL_ = 15
};

// Definitions of the type values used by Rosetta clients
enum {PH_INVALID    = PROCESSHANDLE_INVALID}; // Invalid phandle
enum {PH_UNAMED        = PROCESSHANDLE_UNNAMED_};  // Unnamed post D00 process
enum {PH_NAMED    = PROCESSHANDLE_NAMED_};   // Named post D00 process
enum {PH_OUNAMED    = PROCESSHANDLE_OLD_UNNAMED_}; // Unnamed pre D00 process
enum {PH_ONAMED        = PROCESSHANDLE_OLD_NAMED_};  // Named pre D00 process
enum {PH_LDEV    = PROCESSHANDLE_OLD_LDEV_};    // Logical device process
enum {PH_NULL    = PROCESSHANDLE_NULL_};   // "null" Phandles are all ones


// Definitions for the special port device type and device subtype.
// Special ports are used for clients of NSKPort routines to add the
// port that is not visible from any Rosetta DCT interface routine.
//
// For example if a port $A is been add by NSKPortAdd() with
// SPECIAL_PORT_DEVTYPE, the NSKPortFind will find it but DCT_GETNEXT_ENTRY_
// will filtered it out
//
// In Guardian valid device type are from 0-63. So in NSK-lite we choose
// 100 as the special port dev type. Also if a port has devtype >= 100,
// DCT_ interface routine will not retur it

#define SPECIAL_PORT_DEVTYPE 100



// Define for NSK style longname struct
struct NSKLongname {
  unsigned_char    zeros [3];       // These bytes should be zero.
  union {
    unsigned_char  name5 [5];       // A 5 character name.
    unsigned_char  name5_firstByte; // An alias for checking the 1st byte for 0.
                                    // This byte will be <> 0 for type 4 phandles
  };                                // iff (iff and only if )
                                    // the process has a 5 character name.
};

//////////////////////////////////////////////////////////////////////
//
// <!! NOTE !!>
//        If you change the layout of NSK_PORT_HANDLE. You should
//        also update NSK_PHandle in "guardian\kphandlz.h". The two
//        should be synchronized.
//
///////////////////////////////////////////////////////////////////////

typedef SB_Phandle_Type _NSK_PORT_HANDLE;
typedef SB_Phandle_Type NSK_PORT_HANDLE;
typedef SB_Phandle_Type PHANDLE_TEMPLATE;
typedef SB_Phandle_Type *PNSK_PORT_HANDLE; 
typedef SB_Phandle_Type PROCESSHANDLE_TEMPLATE_;
typedef SB_Phandle_Type nsk_port_handle; 
typedef SB_Phandle_Type phandle_template; 
typedef SB_Phandle_Type processhandle_template; 
typedef SB_Phandle_Type processhandle_template_;
typedef SB_Phandle_Type NSK_phandle_internal_Rosetta;

#define PHANDLE_BYTE_LEN ( _len( NSK_phandle_internal_Rosetta ) )
#define PHANDLE_WORD_LEN ( PHANDLE_BYTE_LEN >> 1 )


// information returned from NSKPortInfo





// nrl update actions

#define NRL_ACTION_BROTHER 1    // add brother
#define NRL_ACTION_DELETE  2    // delete entry
#define NRL_ACTION_PRIMARY 3    // set primary
#define NRL_ACTION_MISC	   4	// set misc flags


// this is the buffer for updating ports.

typedef class _NRL_UPDATE{
public:

   DWORD             action;
   DWORD             processid;
   NSK_PORT_HANDLE   NSKphandle;
   DWORD			 exit_code;
   SHORT			 killpair;
   SHORT			 misc;

} NRL_UPDATE, *PNRL_UPDATE;



#pragma pack(pop, enter_NSK_port)


//=========================================================================
// procedure declarations
//=========================================================================


#ifndef NSKPort_C  // these routines used to be in NSKmsghi.h

typedef class  _NSK_PCB *PNSK_PCB;
typedef class  _NSK_QCB *PNSK_QCB;
typedef class  _NSK_NRL *PNSK_NRL;
typedef class  _NSK_MQC *PNSK_MQC;
typedef class  _NSK_BSD *PNSK_BSD;
typedef struct _NSK_SG  *PNSK_SG;

   // port routines

   DllImport
   DWORD NSKPortAdd( PCHAR pportname,               // port name
                     SHORT portclass,               // port class
                     SHORT portsubclass,             // port subclass
                     PNSK_PORT_HANDLE pNSKphandle );  // pointer to port handle
                                                          // struct allocated by caller.
   DllImport
   DWORD NSKPortDelete( PNSK_PORT_HANDLE pNSKphandle);  // pointer to port handle

   DllImport
   LONG NSKCleanDCTDownPE( DWORD pe );

   DllImport 
   LONG NSKCleanDCT( DWORD pe );

   DllImport
   LONG NSKPortNametoHandle( PCHAR pportname, PNSK_PORT_HANDLE pNSKphandle );

   DllImport
   LONG NSKPortHandletoName( PNSK_PORT_HANDLE pNSKphandle, PCHAR pportname );

   DllImport
   LONG NSKPortFind(   PLONG ptoken,
                       SHORT portclass,
                       SHORT portsubclass,
                       PCHAR psubstring,
                       PNSK_PORT_HANDLE pNSKphandle );
   DllImport
   LONG NSKPortHandleNullIt( PNSK_PORT_HANDLE pNSKphandle );


   DllImport
   LONG NSKPortGetMine(   PLONG ptoken,
                          PNSK_PORT_HANDLE pNSKphandle );
   DllImport
   LONG NSKPortGetBrother(  PNSK_PORT_HANDLE pNSKphandle,
                            PNSK_PORT_HANDLE pBrother );

   DllImport
   LONG NSKPortAttachBackup ( PCHAR pportname,
                              PNSK_PORT_HANDLE pBrother);

   DllImport
   LONG NSKPortSetMePrimary ( PNSK_PORT_HANDLE pNSKhandle);

   DllImport
   LONG NSKPortSetBackup ( PNSK_PORT_HANDLE  pNSKphandle,
                           BOOL              allow_glup );

   DllImport
   VOID NSKMsgsysPhandleRefresh( PNSK_MQC pmqc );

   DllImport
   UINT NSKNameHash( PCHAR pname );

   DllImport
   DWORD NameInNRL( PCHAR pname );


   DllImport
   DWORD NSKModifyNRL( PNSK_SG pnsk,
                       PNRL_UPDATE pupdate );


   // following routines added for Priority Queuing in msg system.
   DllImport
   BOOL NSKIsMsgQEmpty( PNSK_QCB pqcb );

   DllImport
   LONG NSKMsgQInsert( PNSK_QCB pqcb, PNSK_MQC pmqc );

	DllImport
    void *NSKGetFirstInMsgQ ( PNSK_QCB pqcb );

	DllImport
    USHORT NSK_PRIORITY_ ( PNSK_PORT_HANDLE phandle, USHORT newpri, USHORT *initpri );

	DllImport
	USHORT NSK_PORT_MSGQUEUING_SETMODE_( PNSK_PORT_HANDLE phandle , USHORT newmode = 10);

	DllImport
	short NSK_NEXT_LISTEN_PRI_ ( PNSK_PORT_HANDLE phandle, USHORT *nextpri);

	DllImport			
	void NSKMsgQueueInit( PNSK_QCB pqcb );

    DllImport
    DWORD NSKPortMeasureAdd (PNSK_QCB pqcb, int cid, int val);

#ifdef   NSK_DISABLE_MEASURE
#define  NSKPortMeasureAdd(a,b,c)  do { ; } while( 0 )
#endif

#endif

#endif
