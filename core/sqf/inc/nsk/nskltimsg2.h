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
#ifndef  NSKLTIMSG2_H
#define  NSKLTIMSG2_H
#pragma pack(push, one, 8)

/*  byte aligned structure */
typedef struct _NSKMSG_HEADER
   {
#define NSKMSG_ALL_CPU  -1
       short   SrcTmfCpu;   /* source TmfCpu Number */
       short   DestTmfCpu;  /* destination TmfCpu Number */
       ULONG   SendSeqNum;  /* Sender Sequence Number */
   }NSKMSG_HEADER, *PNSKMSG_HEADER;


/* 8 byte aligned structure */
typedef struct _NSKMSG_PARAMS_ {
    /* PIRP, PLISTPORT, LISTENTRY are driver specific fields */
    LIST_ENTRY  ListEntry;        /* List Entry for the driver */
    void        *pIrp;            /* pointer to IRP                  */
    short       sysnum;
    short       node;
    short       port;
#define         NSKMSG_UNACKMSG 0x0001 /* same as LCU_UNACKMSG */
#define         NSKMSG_ACKMSG   0x0002 /* same as LCU_UNACKMSG */
    short       msgtype;

    short       flags;
#define        LCU_ASYNCH 0x1
#define        LCU_SYNCH  0x2
    short       mode;
    short       TmfCpu;             /*  for 8 byte alignment */
    short       datalen;
    int         tnetID;             /* server net TNETID */
    int         rc;
} NSKMSG_PARAMS, *PNSKMSG_PARAMS;

typedef struct _NSKMSG_PARAMS_VERSION
{
    unsigned long FileVersionMS;    // Most significant part of 64 bit file version
    unsigned long FileVersionLS;    // Least significant part of 64 bit file version
} NSKMSG_PARAMS_VERSION,*PNSKMSG_PARAMS_VERSION;



typedef struct _NSKMSG_PIO_PARAMS_ {
    /* PIRP,  LISTENTRY are driver specific fields */
    LIST_ENTRY    ListEntry;           /* List Entry for the driver */
    void          *pIrp;               /* pointer to IRP                  */
    void          *lcumsg;             /* pointer to IRP                  */
    short         sysnum;
    short         node;
    short         port;
    short         TmfCpu;             /* port id or Tmf cpu number */
    short         datalen;            /* data length, also used as in-line data length */
    ULONG         transferSize;
    int           rc;
    /* these two fields must be the last two fields of this structure */
    char          *buf;              /* data buffer */
} NSKMSG_PIO_PARAMS, *PNSKMSG_PIO_PARAMS;
#define NSKMSG_SEND_PIO_PARAMS         NSKMSG_PIO_PARAMS
#define NSKMSG_LISTEN_PIO_PARAMS       NSKMSG_PIO_PARAMS
#define PNSKMSG_SEND_PIO_PARAMS        PNSKMSG_PIO_PARAMS
#define PNSKMSG_LISTEN_PIO_PARAMS      PNSKMSG_PIO_PARAMS



typedef struct _NSKMSG_CONFIG_PARAMS_ {
    int        cmd;            /* configuration command */
    void        *config_arg;        /* config command specific argument */
} NSKMSG_CONFIG_PARAMS, *PNSKMSG_CONFIG_PARAMS;
#pragma pack(pop, one)

#endif // NSKLTIMSG2_H
