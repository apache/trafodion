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
#if _MSC_VER >= 1100
#pragma once
#endif
#ifndef  NSKLTIMSG_H
#define  NSKLTIMSG_H

/*
    NSKMSG.DLL and NSKMSG.SYS together provide an interface to
    user mode NT application to access servernet services. NSKMSG.DLL
    is specifically designed to be used only by NSK LITE.

                       INSTALLATION

   LTI.SYS and SPAD.SYS are kernel mode driver. They must be
   installed first before an applicatin can access NSKMSG.SYS.Please
   refer to LTI/SPAD documentation for installation of these drivers.

   To install NSKMSG kernel mode driver copy NSKMSG.SYS and NSKMSG.DLL
   in your application(NSK LITE) directory. When NSKMsgInit() is
   invoked NSKMSG.SYS is installed from application current directory
   and started as NT service. Alternativerly, NSKMSG driver
   can be installed  by the follwowing command.

   sc create nskmsg binPath= {PATH} type= kernel start= demand

   N.B you may require space after =

   example
   sc create   nskmsg binPath=c:\winnt40\system32\drivers\nskmsg.sys type= kernel start= demand

   Once installed you can start NSKMSG driver by the following command

   sc start nskmsg

   Yo can stop nskmsg driver by invking by the following command

   sc stop nskmsg



                   NSK APPLICATION INTERFACE

      NSKMSG.DLL provides basic functions to send and receive messages
      by user mode application over servernet. This DLL interfaces with the
      kernel mode intermediate NSKMSG driver.

      NSKMSGInit() must be called first to open NSKMSG.SYS kernel driver.
      This call opens a handle to NSKMSG driver which is started as
      NT service kernel mode driver. This call also configure the driver
      in the NT registry if it is not previously configured.


      NSKMsgInitPort() must be called to initialize a NSK port
      for LTI driver. Similarly NSKMsgDeinitPort() must be called to close
      a previously opened port.

      NSKMSG.DLL assumes that a cluster manager or some other process
      will declare reemote servernet node up and will establish a
      connection. If you do not have a cluster manager you may call
      NSKMsgNodeSetup() to update status of remote servernet
      nodes. A process must not start I/O for a remote node until
      that node is declared UP. You can also declare remote node
      down by calling NSKSMGNodeSetup().

      All messages are sent/received using direst IO and IOCTL
      mechanism of NT IO manager.

                   DATA STRUCTURES

      Please refer to NSKLTIMSG0.h for the description of data structures.
      ---------------------------------------------------------------------------------
      Sender Example:

      NSKMSG_IO  Msg;
       if (NSKMsgInit())
              {
          MessageBox( "NSKMsgInit failed", "NSK API Error", MB_ICONSTOP | MB_OK );
          exit();
         }
       else
          MessageBox( "NSKMsgInit Success", "NSK API ",  MB_OK );
       memset(&Msg, 0x00, sizeof(Msg));
       Msg.message.reqmbuf.ctrllen = m_control;
       Msg.message.reqmbuf.ctrlbuf = (char *)malloc(m_control);
       Msg.message.reqmbuf.datalen    = m_data;
       Msg.message.reqmbuf._buf.databuf = (char *)malloc(m_data);
         if (NSKMsgSend(&Msg, m_port, m_system, m_node, NULL))
      {
          MessageBox( "NSKMsgSend failed", "NSK API Error", MB_ICONSTOP | MB_OK );
         exit();
      }
    else
       MessageBox( "NSKMsgSend uccess", "NSK API ",  MB_OK );

     In this example first NSK driver is initializez and then NSKMSG_IO
     structre is initialized and then NSKMsgSend() is called to send
     a message as blocking operation to a remote node m_node on
    system m_system and a port m_port.

   Finally application should close handle to NSKMSG by calling
      NSKMsgDeinit();

-------------------------------------------------------------------------------
   Listener Example

    static     NSKMSG_IO  Msg;
               PNSKMSG_IO  pMsg = &Msg;
    static      OVERLAPPED   Ovl;
                LPOVERLAPPED pOvl = &Ovl;

       if (NSKMsgInit())
         {
              MessageBox( "NSKMsgInit failed", "NSK API Error", MB_ICONSTOP | MB_OK );
            exit();
          }
       else
          MessageBox( "NSKMsgInit Success", "NSK API ",  MB_OK );

        if (NSKMsgInitPort(m_port))
      {
          MessageBox( "NSKMsgInitPort failed", "NSK API Error", MB_ICONSTOP | MB_OK );
       exit();
      }
    else
        MessageBox( "NSKMsgInitPort Success", "NSK API ",  MB_OK );

       memset(&Msg, 0x00, sizeof(Msg));
       Msg.message.reqmbuf.ctrllen = 1048;
       Msg.message.reqmbuf.ctrlbuf = (char *)malloc(1048);
       Msg.message.reqmbuf.datalen    = 0;
       Msg.message.reqmbuf._buf.databuf = NULL;

       rc = NSKMsgListen(&Msg,m_port, pOvl);
       if (rc && rc != ERROR_IO_PENDING)
      {
          MessageBox( "NSKMsgListen failed", "NSK API Error", MB_ICONSTOP | MB_OK );
       exit();
      }
       else
          MessageBox( "NSKMsgListen press OK to start waiting", "NSK API ",  MB_OK );

         WaitForSingleObject(pOvl->hEvent,INFINITE);
       if (Msg.Params.rc)
      {
           MessageBox( "NSKMsgListen failed", "NSK API Error", MB_ICONSTOP | MB_OK );
         exit();
      }
       else
           MessageBox( "NSKMsgListen success", "NSK API ",  MB_OK );

       Here NSKMSG driver is initialized and then a non blocking listen is
       posted when a message arrives the control portion of
       the message is delivered to the application. To receive rest of the
       data, application must call NSKMsgGetData() as follows.

       if (NSKMsgGetData(pMsg, m_offset, m_length, NULL))
       {
              MessageBox( "NSKMsgGetData failed", "NSK API Error", MB_ICONSTOP | MB_OK );
           exit();
        }
       else
         MessageBox( "NSKMsgGetData Success", "NSK API ",  MB_OK );

      Finally application calls NSMsgDone() to freeup all resources for this
      message.

      if (NSKMsgDone(pMsg, NULL))
      {
          MessageBox( "NSKMsgDone failed", "NSK API Error", MB_ICONSTOP | MB_OK );
         exit();
      }
       else
       MessageBox( "NSKMsgDone Success", "NSK API ",  MB_OK );

   Before termination application must call NSKMsgDeinitPort() for
   all previously initialized ports.

    if (NSKMsgDeinitPort(m_port))
          MessageBox( "NSKMsgDeInit failed", "NSK API Error", MB_ICONSTOP | MB_OK );
    else
      MessageBox( "NSKMsgDeInit Success", "NSK API ",  MB_OK );

   Finally application should close handle to NSKMSG by calling
      NSKMsgDeinit();


   N.B: TO GAIN HIGH I/O PERFORMANCE, INPUT PARAMETERS ARE NOT CHECKED.
        CLIENT APPLICATIONS ARE ASSUMED TO BE TRUSTED.

Revision History:

--*/
#ifdef __cplusplus
extern "C"{
#endif

#include "seaquest/sqtypes.h"
#include "nsk/nskltimsg0.h"
#include "nsk/nskltimsg2.h"

#pragma pack(push, one, 8)
#ifdef TDM_NSKTRANSPORT_DLL
#define NSKMSGIMPORT    __declspec( dllexport )
#else
#define NSKMSGIMPORT    __declspec( dllimport )
#endif

#define NSKMSG_INLINE_MAX 1024  /* == LCU_INLINE_MAX */

NSKMSGIMPORT DWORD   NSKMsgInitPort(WORD portID);
/*
   portID
      identifies LTI port number for I/O requests.

   Return Value
         This function returns FALSE to indicate success of the request
         otherwise an error code is returned.

   Comments:
       A port must be initialized before a message can be sent for this port.
       NSKMSG driver must implement this call as a blocking call
NSKMSG driver NSKMSG_PARAM request with  the following format:
    port   ==    portID
    flags  ==   inLineData | ordereredDelivery | flowControl
    datalen  == inLineDataLen
   NSKMsgInitPort() is a blocking call
 N.B: PARAMETERS ARE NOT CHECKED/APPLICATION IS ASSUMED TO BE TRUSTED

*/

NSKMSGIMPORT DWORD  NSKMsgInitPioPort(WORD portID);
/*
   portID
      identifies LTI port number for I/O requests.

   Return Value
         This function returns FALSE to indicate success of the request
         otherwise an error code is returned.

   Comments:
       A port must be initialized before a message can be sent for this port.
       NSKMSG driver must implement this call as a blocking call
NSKMSG driver NSKMSG_PARAM request with  the following format:
    port   ==    portID
    flags  ==   inLineData | ordereredDelivery | flowControl  | Pio
    datalen  == inLineDataLen
   NSKMsgInitPort() is a blocking call
 N.B: PARAMETERS ARE NOT CHECKED/APPLICATION IS ASSUMED TO BE TRUSTED

*/

NSKMSGIMPORT DWORD   NSKMsgDeinitPort(WORD portID);

/*
   portID
      identifies a previously initialized  LTI port number.

   Return Value:
         This function returns FALSE to indicate success of the fucntion
         otherwise an error code is returned.

   Comments:
      All outstanding IO for this port will be returned with an error
      code.

*/





NSKMSGIMPORT DWORD   NSKMsgNodeSetup(WORD System, WORD Node, BOOL Status,
                           DWORD tnetID);

/*

   System
      identifies a system ID.

   Node
      identifies a node id.

   Status
       identifies the Node status. If TRUE node is marked up and
       a connect request is initiated. Oherwise node is marked
       down and all outstanding requests to this node is returned
       with an error code.

    tnetID
      identifies a tnetid for the remote node.

   Return Value
       This function returns FALSE to indicate success of the request
       otherwise an error code is returned.

   Comments:
       A node must be marked UP before an I/O request can be made
       for this node. Caller must declare node down by calling this
       function to retrive outstanding I/O request for downed node.

*/






NSKMSGIMPORT DWORD   NSKMsgAccept(WORD System, WORD Node);
/*

   System
      identifies a system ID.

   Node
      identifies a node id.

   Return Value:
         This function returns FALSE to indicate success of the request
         otherwise an error code is returned.

   Comments:

*/




// Get the node and system number a known to Lti and the driver
//
NSKMSGIMPORT  DWORD   NSKMsgGetNodeInfo( 
                           PWORD pSystem
                         , PWORD pNode
                         );
/*
   pSystem
        the system number as Lti knows it

   pNode
        the node number as Lti knows it

  Return Value:
        Returns a nt error (0== success)
*/

//
// Remove the driver and its event registry keys from the system
//  (for use in an uninstall)

NSKMSGIMPORT  DWORD NSKMsgDelete();


#pragma pack(pop, one)
#ifdef __cplusplus
}
#endif

#endif // NSKLTIMSG_H
