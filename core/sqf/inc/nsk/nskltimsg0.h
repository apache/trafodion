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
#ifndef  NSKLTIMSG0_H
#define  NSKLTIMSG0_H

#ifdef __cplusplus
extern "C"{
#endif

#include "seaquest/sqtypes.h"

#pragma pack(push, one, 8)

/*
  LISTENTRY:
           LISTENTRY is doubly linked list for NSKMSG driver internal
            queues.
  PIRP:
           PIRP is a pointer to save IRP for NSKMSG driver.

   PORTID:
          PORTID is port number for this user request and is used by NSKMSG
          driber/dll.

   SYSTEM:
          SYSTEM is remote system number for this request used by
          NSKMSG driver/dll.

   NODE:
          NODE is remote node number for this request and is used by
          NSKMSG driver/dll.


   OFFSET:
         OFFSET defines the offset of the received user message
         for the NSMsgGetData() call and used by driver/dll.

  LENGTH:
         LENGTH defines number of bytes to be read
         for the NSMsgGetData() call and used by NSKMSG driver/dll.

  MOREDATA:
         Additional data length available for this message.
   RC:   RC is error code returned by non-blocking call.
*/


typedef struct _NSKMSG_IO_PARAMS_ {
    /* PIRP,  LISTENTRY are driver specific fields */
    LIST_ENTRY  ListEntry;        /* List Entry for the driver */
    void        *pIrp;            /* pointer to IRP                  */
    short       portID;           /* port ID                         */
    short       system;
    short       node;
    int         moredata;        /* additional data length avaibale */
    int         offset;
    int         length;
    /* error code returned by LTI */
    int         rc; /* see lcuxprt.h and winerror.h for details */
    ULONG       transferSize;
} NSKMSG_IO_PARAMS, *PNSKMSG_IO_PARAMS;


/*
      IOADDR:
            IOADDR is reserved for the LCU transport.
      BUF:
            BUF is pointer to user buffer.

      BUFLEN:
           BUFLEN is user data buffer length
*/
typedef struct NSKMSG_SGENT{
    char             *ioaddr;                 /* reserved for LCU transport */
    char             *buf;                    /* pointer to a buffer        */
    unsigned int    buflen;                 /* buffer length              */
} NSKMSG_SGENT, *PNSKMSG_SGENT;


/*
  SGCNT:
        SGCNT defines number of vaild entries pointed by SGADDR.

  CTRLLEN:
        CTRLLEN defines length of the user control buffer.

   CTRLBUF:
        CTRLBUF is pointer to user control buffer.

   DATALEN:
        DATALEN is total length of user data buffer.

   _BUF:
        _BUF defines user MBUF structrue.


*/


typedef struct _NSKMSG_MBUF{
    unsigned short    sgcnt;                /* #scatter/gather entries */
    unsigned short    ctrllen;                /* length of the control portion of a msg */
    char             *ctrlbuf;             /* ptr to the control portion of a message */
    unsigned int    datalen;                /* length of the data portion of a message */
    union {
      char          *databuf;              /* sgcnt = 0 --> message data buffer addr */
      NSKMSG_SGENT *sgaddr;                 /* sgcnt != 0 --> SG array addr */
    } _buf;
} NSKMSG_MBUF, *PNSKMSG_MBUF;


/*
   RESERVED:
            RESERVED is a 4 byte pointer space for LCU transport.

   TAG:
            TAG is a 4 byte pointer  reserved  for NSKMSG driver.
   SYSNUM:
            SYSNUM is remote system number and it is READONLY for
            client application and READ/WRITE for NSKMSG driver.
   NODE:
            NODE is remote system node number and it is READONLY for
            client application and READ/WRITE for NSKMSG driver.

   PORT:
            PORT is remote system port number and it is READONLY for
            client application and READ/WRITE for NSKMSG driver.

   MSGTYPE:
           MSGTYPE is type of this message and is reserved for NSKMSG driver.

   FLAGS:  FLAGS defines how this message should be delivered by LTI driver
           and is reserved for NSKMSG driver.

   REQMBUF:
           REQMBUF descibes the user control and data buffers for this
           message.

*/
typedef struct _NSKMSG_MESSAGE{
         void                   *reserved;
          void                   *tag;
          unsigned short       sysnum;
          unsigned short       node;
          unsigned short          port;
          char                   msgtype;
      /* flags are used to indicate the status of the node, delivery mechanism */
#define         NSKMSG_NODEDOWN        0x0001
#define         NSKMSG_NODEUP          0x0002
#define         NSKMSG_ORD_DELIVERY    0x0004
#define         NSKMSG_PIO             0x0008  /* request is a PIO request */
#define         NSKMSG_MBUF_SET        0x0010  /* request contains MBUF */
#define         NSKMSG_INLINEDATA      0x0020  /* in-line is true for this request */
#define         NSKMSG_FLOW_CONTROL    0x0040  /* flow control is true for this request */
          char                   flags;
          NSKMSG_MBUF          reqmbuf;
        }NSKMSG_MESSAGE, *PNSKMSG_MESSAGE;


/*
   PARAMS:
           PARAMS is used by NSKMSG driver and NSKMSG.DLL
           applciation should not refer to this structrue exceptthe
           RC element to check the error code returned in non-blocking
           calls.

    LCUMSG:
          LCUMSG is reserved for NSKMSG driver this application must not
          modify this element. NSKMSG kernel mode driver saves pointer
          to a LCU message strcutrue (lcumsg_t) here for freeing up
          resources reserved for an I/O operation.

     MESSAGES:
          MESSAGE structure is used as applcaition and NSKMSG driver
         as desribed above.



*/

typedef struct _NSKMSG_IO {
   NSKMSG_IO_PARAMS    Params;     /* reserved: useed by NSKMSG driver */
   void                *lcumsg;     /* reserved: used as reference pointer in GETDATA call */
   NSKMSG_MESSAGE      message;
   } NSKMSG_IO, *PNSKMSG_IO;



#ifndef FILE_ANY_ACCESS
#define FILE_ANY_ACCESS                 0
#endif

#ifndef METHOD_BUFFERED
#define METHOD_BUFFERED                 0
#endif

#ifndef METHOD_IN_DIRECT
#define METHOD_IN_DIRECT                1
#endif

#ifndef METHOD_OUT_DIRECT
#define METHOD_OUT_DIRECT               2
#endif

#ifndef METHOD_NEITHER
#define METHOD_NEITHER                 3
#endif

#define FILE_DEVICE_NSKMSG                    0x8001


#define NSKMSG_IOCTL_SEND_PIO           (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0x800,    \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)

#define NSKMSG_IOCTL_ACCEPT              (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0x801,    \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NSKMSG_IOCTL_CANCEL              (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0x802,    \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NSKMSG_IOCTL_NODESTATUS          (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0x803,    \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)

#define NSKMSG_IOCTL_LISTEN              (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0x804,    \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NSKMSG_IOCTL_SEND                  (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0x805,    \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NSKMSG_IOCTL_LISTEN_PIO                  (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0x806,    \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NSKMSG_IOCTL_UNLISTEN                (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0x807,    \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NSKMSG_IOCTL_GETDATA                (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0x808,    \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NSKMSG_IOCTL_MSGDONE                (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0x809,    \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NSKMSG_IOCTL_CONFIG                (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0x80a,    \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NSKMSG_IOCTL_MSGALLOC                (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0x80b,    \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NSKMSG_IOCTL_MSGFREE                (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0x80c,    \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NSKMSG_IOCTL_INITPORT                (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0x80d,    \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NSKMSG_IOCTL_DEINITPORT            (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0x80e,    \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NSKMSG_IOCTL_GETNODEINFO            (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0X80f,  \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)
#define NSKMSG_IOCTL_GETVERSION             (ULONG)CTL_CODE(FILE_DEVICE_NSKMSG, 0X810,  \
                                                      METHOD_BUFFERED, FILE_ANY_ACCESS)


#define    NSKMSG_MIN_INPUT_SIZE    8
#define    NSKMSG_MAX_OUTPUT_SIZE   (sizeof(NSKMSG_PARAMS) )

#define PNSKMSG_RECV_PARAMS         PNSKMSG_PARAMS
#define PNSKMSG_UNLISTEN_PARAMS     PNSKMSG_PARAMS
#define PNSKMSG_INITPORT_PARAMS     PNSKMSG_PARAMS
#define PNSKMSG_DEINITPORT_PARAMS   PNSKMSG_PARAMS
#define PNSKMSG_CANCEL_PARAMS       PNSKMSG_PARAMS
#define PNSKMSG_ACCEPT_PARAMS       PNSKMSG_PARAMS
#define PNSKMSG_NODESTATUS_PARAMS   PNSKMSG_PARAMS
#define PNSKMSG_GETNODEINFO_PARAMS  PNSKMSG_PARAMS

#define NSKMSG_RECV_PARAMS          NSKMSG_PARAMS
#define NSKMSG_UNLISTEN_PARAMS      NSKMSG_PARAMS
#define NSKMSG_INITPORT_PARAMS      NSKMSG_PARAMS
#define NSKMSG_DEINITPORT_PARAMS    NSKMSG_PARAMS
#define NSKMSG_CANCEL_PARAMS        NSKMSG_PARAMS
#define NSKMSG_ACCEPT_PARAMS        NSKMSG_PARAMS
#define NSKMSG_NODESTATUS_PARAMS    NSKMSG_PARAMS
#define NSKMSG_GETNODEINFO_PARAMS   NSKMSG_PARAMS
#pragma pack(pop, one)

#ifdef __cplusplus
}
#endif

#endif // NSKLTIMSG0_H
