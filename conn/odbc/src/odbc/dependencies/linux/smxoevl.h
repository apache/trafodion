// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
#ifndef SMXOEVL_H
#define SMXOEVL_H
#include "zspic"
#include "zmxoc.h"

typedef struct EVENTDATA_TABLE
{
   DWORD eventId;
   char* msgPtr[10];
} EVENTDATA_TABLE;

/*------------------------------------------------------------------------------
 When transferring these messages over from ems/smxotmpl, tokens 1, 2 and 3 are
 reserved for Event Type, component Name and Object Reference. 99% of the
 messages fit in this category. These token are labelled as:

 Token
 ---------------------
 Event Type        <1>
 Component Name    <2>
 Object Reference  <3>

 All remaining tokens must begin with <4>, <5> ... <n>

Therefore any messages that do NOT use tokens alll three of the first three
tokens, will have a gap in the numbering scheme, i.e

{ZMXO_EVT_COLLECTOR_ERROR,
   "ODBC/MX server failed to write to <7> collector due to error <5>. \n",
   "Session ID: <4>\n",


   "Error Message: <6>\n",
   "Collector Name: <8>\n",
   NULL, NULL, NULL, NULL },

In this case, Event Type is not in the message, so token <1> is not used

------------------------------------------------------------------------------*/

EVENTDATA_TABLE eventDataMap[] =
{
//  eventId, msgPtr1-msgPtr10 ...

{ZMXO_EVT_REG_SRVR_ERR,
   "ODBC/MX server failed to register. \n",

/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
    NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SRVST_CHNG_ERR,
   "ODBC/MX server changed state from <5> to <6>. \n",

   "Server ObjRef: <4>\n",
/*---------------------------------------------------
     "<*IF 7>Event Experience level : <8>\n<*ENDIF>"
     "<*IF 9>Event Severity : <10>\n<*ENDIF>"
     "<*IF 11>Event Owner : <12>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_START_SRVR_ERR,
   "Start ODBC/MX server <5> failed on port <4>. \n",

/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_STOP_SRVR_ERR,
   "Stop ODBC/MX server failed on port <4>. \n",

/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_DSST_CHNG_FAIL,
   "ODBC/MX data source <4> changed state from <5> to <6>. \n",

/*---------------------------------------------------
     "<*IF 7>Event Experience level : <8>\n<*ENDIF>"
     "<*IF 9>Event Severity : <10>\n<*ENDIF>"
     "<*IF 11>Event Owner : <12>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_PORT_NOTAVAIL,
   "No more port available to start ODBC/MX servers. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SRVC_STARTED,
   "ODBC/MX service is started. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SRVC_INITIALIZED,
   "ODBC/MX service Initialized. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_DS_STARTED,
   "ODBC/MX data source <4> is started. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SRVC_STRT_FAIL,
   "ODBC/MX service failed to start due to previous error. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SRVC_STRT_INFO,
   "ODBC/MX service started with some information. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_DS_STRT_FAIL,
   "Start ODBC/MX data source <4> failed. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_DS_STRT_INFO,
   "The ODBC/MX data source <4> started with some information. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SRVC_STOPPED,
   "ODBC/MX service is stopped. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_DS_STOPPED,
   "The ODBC/MX data source <4> is stopped for <5>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SVC_STOPPED_INFO,
   "ODBC/MX Service Stopped for <4>. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_DS_STOPPED_INFO,
   "ODBC/MX data source <4> stopped for <5>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

//this event is never reported
{ZMXO_EVT_STSRV_CNTXT_FAIL,
   "Setting initial ODBC/MX server context failed. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SRV_STRT_INDBG,
   "ODBC/MX server started in debug mode. Debug flag: <4>. DS Id: <5>. CEEcfg Param: <6>\n",



/*---------------------------------------------------
     "<*IF 7>Event Experience level : <8>\n<*ENDIF>"
     "<*IF 9>Event Severity : <10>\n<*ENDIF>"
     "<*IF 11>Event Owner : <12>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SRV_ST_CHNG_INFO,
   "ODBC/MX server <4> changing state from <5> to <6>. \n",



/*---------------------------------------------------
     "<*IF 7>Event Experience level : <8>\n<*ENDIF>"
     "<*IF 9>Event Severity : <10>\n<*ENDIF>"
     "<*IF 11>Event Owner : <12>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_DS_STOPPING,
   "ODBC/MX data source <4> stopping for <5>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_DS_STOP_ABRUPT,
   "ODBC/MX data source <4> stopped abruptly for <5>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_DSTOP_ABRPT_INFO,
   "ODBC/MX data source <4> stopped abruptly for <5> with some information. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_DS_STOPIN_ABRUPT,
   "ODBC/MX data source <4> is stopping abruptly for <5>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_DS_STOPIN_DISCON,
   "ODBC/MX data source <4> is stopping in Stop-When-Disconnected mode for <5>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SVC_STOP_ABRUPT,
   "ODBC/MX service is stopped abruptly for <4>. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SVC_STRT_WARNING,
   "ODBC/MX service started with warning <4> from configuration server. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

// EVENT NOT REPORTED
{ZMXO_EVT_SQL_NOT_INIT,
   "SQL/MX has not successfully completed its initialization. \n",
   "ODBC/MX services cannot be started. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SAV_DSSTAT_FAIL,
   "ODBC/MX service failed to save data source <4> status <5>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SAV_ASSTAT_FAIL,
   "ODBC/MX service failed to save status <4> of ODBC/MX service. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_INTRNLCNTR_RECAL,
   "ODBC/MX service recalculated its internal counters. \n",
   "Old values - server registered: <6>, server connected <7>. \n",
   "New values - server registered: <4>, server connected <5>. \n",



/*---------------------------------------------------
     "<*IF 8>Event Experience level : <9>\n<*ENDIF>"
     "<*IF 10>Event Severity : <11>\n<*ENDIF>"
     "<*IF 12>Event Owner : <13>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL },

{ZMXO_EVT_COLLECTOR_ERROR,
   "ODBC/MX server failed to write to <7> collector due to error <5>. \n",
   "Session ID: <4>\n",


   "Error Message: <6>\n",
   "Collector Name: <8>\n",
/*---------------------------------------------------
     "<*IF 8>Event Experience level : <9>\n<*ENDIF>"
     "<*IF 10>Event Severity : <11>\n<*ENDIF>"
     "<*IF 12>Event Owner : <13>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL },

{ZMXO_EVT_TRACE_INFO,
   "ODBC/MX Trace:\n",
   "Session ID: <4>\n",
   "Function: <5>\n",
   "Sequence Number: <6>\n",
   "<7>\n",
/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL },

{ZMXO_EVT_RES_STAT_INFO,
   "ODBC/MX Statistics:\n",
   "Session ID: <4>\n",
   "Message Attribute: <5>\n",
   "Sequence Number: <6>\n",
   "Message Info: <7>\n",
/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_QUERY_STATUS_INFO,
   "ODBC/MX Query Status:\n",
   "Session ID: <4>\n",
   "Message Attribute: <5>\n",
   "Sequence Number: <6>\n",
   "Message Info: <7>\n",
/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_ODBCINIT_STARTED,
   "ODBC/MX Initialization Operation <4> Started.\n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_PROGRAM_ERR,
   "An unexpected programming exception <4> has occurred. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SQL_ERR,
   "A SQL/MX query failed. SQLCODE: <4>.\n",
   "Error Text: <5>.\n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_KRYPTON_ERR,
   "A network component error <4> has occurred.\n",
   "CEE Error Text: <5>.\n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SRVR_REG_ERR,
   "ODBC/MX server failed to register back to ODBC/MX service. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_NSK_ERR,
   "A NonStop Process Service error <4> has occurred. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SRVR_ENV,
   "ODBC/MX service starting parameters: <4>. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_MALLOC_ERR,
   "Memory allocation error in the function: <4>. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SQL_WARNING,
   "A SQL/MX warning <4> has occurred. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_DEFINESETATTR_ERR,
   "An error <4> has occurred while setting the attribute <5> with value <6> for Define <7>.\n",



/*---------------------------------------------------
     "<*IF 8>Event Experience level : <9>\n<*ENDIF>"
     "<*IF 10>Event Severity : <11>\n<*ENDIF>"
     "<*IF 12>Event Owner : <13>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_DEFINESAVE_ERR,
   "A Define error <4> has occurred while saving the Define <5>.\n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_RG_STOP,
   "Query <6> estimated cost <4>, exceeds resource policy <5>. \n",
   "Query marked un-excutable. \n",



/*---------------------------------------------------
     "<*IF 7>Event Experience level : <8>\n<*ENDIF>"
     "<*IF 9>Event Severity : <10>\n<*ENDIF>"
     "<*IF 11>Event Owner : <12>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SRV_MONCAL_FAIL,
   "The monitor object call <4> to the ODBC/MX service failed. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_SRV_ITOUT_ERR,
   "Failed to timeout while in idle state, due to an error in the\n",
   "object call to ODBC/MX service, "
   "or due to timer creation error. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL },

{ZMXO_EVT_UPDT_SRV_ST_FAIL,
   "ODBC/MX server failed to update its state in the ODBC/MX service. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_TIP_NOTCONNECT,
   "ODBC/MX server failed due to user disconnected from the TIP gateway. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_TIP_NOTCONFIG,
   "ODBC/MX server failed due to TIP gateway not configured. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_TIP_ERR,
   "ODBC/MX server failed due to TIP gateway error. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_POST_CONCT_ERR,
   "ODBC/MX client fails to connect due to error <5> in setting up \n",
   "the connection context <4>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
  NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CFG_SRVR_INIT,
   "ODBC/MX configuration server Initialized. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

// EVENT SAME AS ZMXO_EVT_RG_LOG
{ZMXO_EVT_RG_OVER_LIMIT,
   "Resource Policy Exceeds Limit. \n",
   "Estimated cost: <4>\n",
   "Limit cost: <5>\n",
   "SQL Text: <6>\n",



/*---------------------------------------------------
     "<*IF 7>Event Experience level : <8>\n<*ENDIF>"
     "<*IF 9>Event Severity : <10>\n<*ENDIF>"
     "<*IF 11>Event Owner : <12>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL },

{ZMXO_EVT_INSUF_PRIVLGS,
   "The user has Insufficient Privileges to start ODBC/MX Service. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_REG_SRVR_ERR,
   "SQL/MX server failed to register. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SRVST_CHNG_ERR,
   "SQL/MX server changed state from <5> to <6>. \n",



   "Server ObjRef: <4>\n",
/*---------------------------------------------------
     "<*IF 7>Event Experience level : <8>\n<*ENDIF>"
     "<*IF 9>Event Severity : <10>\n<*ENDIF>"
     "<*IF 11>Event Owner : <12>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_START_SRVR_ERR,
   "Start SQL/MX server <5> failed on port <4>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_STOP_SRVR_ERR,
   "Stop SQL/MX server failed on port <4>. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_DSST_CHNG_FAIL,
   "MXCS data source <4> changed state from <5> to <6>. \n",



/*---------------------------------------------------
     "<*IF 7>Event Experience level : <8>\n<*ENDIF>"
     "<*IF 9>Event Severity : <10>\n<*ENDIF>"
     "<*IF 11>Event Owner : <12>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_PORT_NOTAVAIL,
   "No more port available to start SQL/MX servers. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SRVC_STARTED,
   "MX Connectivity Service is started. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SRVC_INITIALIZED,
   "MX Connectivity Service Initialized. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_DS_STARTED,
   "MXCS data source <4> is started. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SRVC_STRT_FAIL,
   "MX Connectivity Service failed to start due to previous error. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SRVC_STRT_INFO,
   "MX Connectivity Service started with some information. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_DS_STRT_FAIL,
   "Start MXCS data source <4> failed. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_DS_STRT_INFO,
   "The MXCS data source <4> started with some information. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SRVC_STOPPED,
   "MX Connectivity Service is stopped. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_DS_STOPPED,
   "The MXCS data source <4> is stopped for <5>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SVC_STOPPED_INFO,
   "MX Connectivity Service stopped with some failure(s). See previous event(s). \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_DS_STOPPED_INFO,
   "MXCS data source <4> stopped for <5>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

// this event is never reported
// this message needed a <4> token, added w/Linux port. SBoand.
{ZMXO_EVT_CS_STSRV_CNTXT_FAIL,
   "Setting initial SQL/MX server context failed: <4> \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SRV_STRT_INDBG,
   "SQL/MX server started in debug mode. Debug flag: <4>. DS Id: <5>. CEEcfg Param: <6>\n",



/*---------------------------------------------------
     "<*IF 7>Event Experience level : <8>\n<*ENDIF>"
     "<*IF 9>Event Severity : <10>\n<*ENDIF>"
     "<*IF 11>Event Owner : <12>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SRV_ST_CHNG_INFO,
   "SQL/MX server <4> changing state from <5> to <6>. \n",



/*---------------------------------------------------
     "<*IF 7>Event Experience level : <8>\n<*ENDIF>"
     "<*IF 9>Event Severity : <10>\n<*ENDIF>"
     "<*IF 11>Event Owner : <12>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_DS_STOPPING,
   "MXCS data source <4> stopping for <5>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_DS_STOP_ABRUPT,
   "MXCS data source <4> stopped abruptly for <5>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_DSTOP_ABRPT_INFO,
   "MXCS data source <4> stopped abruptly for <5> with some information. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_DS_STOPIN_ABRUPT,
   "MXCS data source <4> is stopping abruptly for <5>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_DS_STOPIN_DISCON,
   "MXCS data source <4> is stopping in Stop-When-Disconnected mode for <5>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SVC_STOP_ABRUPT,
   "MX Connectivity Service is stopped abruptly for <4>. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SVC_STRT_WARNING,
   "MX Connectivity Service started with warning <4> from configuration server. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

// EVENT NOT REPORTED
{ZMXO_EVT_CS_SQL_NOT_INIT,
   "SQL/MX has not successfully completed its initialization. \n",
   "MX Connectivity Services cannot be started. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SAV_DSSTAT_FAIL,
   "MX Connectivity Service failed to save data source <4> status <5>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SAV_ASSTAT_FAIL,
   "MX Connectivity Service failed to save status <4> of MX Connectivity Service. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_INTRNLCNTR_RECAL,
   "MX Connectivity Service recalculated its internal counters. \n",
   "Old values - server registered: <6>, server connected <7>. \n",
   "New values - server registered: <4>, server connected <5>. \n",



/*---------------------------------------------------
     "<*IF 8>Event Experience level : <9>\n<*ENDIF>"
     "<*IF 10>Event Severity : <11>\n<*ENDIF>"
     "<*IF 12>Event Owner : <13>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_COLLECTOR_ERROR,
   "SQL/MX server failed to write to <7> collector due to error <5>. \n",
   "Session ID: <4>\n",


   "Error Message: <6>\n",
   "Collector Name: <8>\n",
/*---------------------------------------------------
     "<*IF 8>Event Experience level : <9>\n<*ENDIF>"
     "<*IF 10>Event Severity : <11>\n<*ENDIF>"
     "<*IF 12>Event Owner : <13>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_TRACE_INFO,
   "MXCS Trace:\n",
   "Session ID: <4>\n",
   "Function: <5>\n",
   "Sequence Number: <6>\n",
   "<7>\n",
/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_RES_STAT_INFO,
   "MXCS Statistics:\n",
   "Session ID: <4>\n",
   "Message Attribute: <5>\n",
   "Sequence Number: <6>\n",
   "Message Info: <7>\n",
/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_QUERY_STATUS_INFO,   //21036
   "MXCS Query Status:\n",
   "Session ID: <4>\n",
   "Message Attribute: <5>\n",
   "Sequence Number: <6>\n",
   "Message Info: <7>\n",
/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_ODBCINIT_STARTED,
   "MXCS Initialization Operation <4> Started.\n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_PROGRAM_ERR,
   "An unexpected programming exception <4> has occurred. \n",



   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SQL_ERR,
   "A SQL/MX query failed. SQLCODE: <4>.\n",
   "Error Text: <5>.\n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_KRYPTON_ERR,
   "A network component error <4> has occurred.\n",
   "CEE Error Text: <5>.\n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
 NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SRVR_REG_ERR,
   "SQL/MX server failed to register back to MX Connectivity Service. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_NSK_ERR,
   "A NonStop Process Service error <4> has occurred. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SRVR_ENV,
   "MX Connectivity Service starting parameters: <4>. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },



{ZMXO_EVT_CS_MALLOC_ERR,
   "Memory allocation error in the function: <4>. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SQL_WARNING,
   "A SQL/MX warning <4> has occurred. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_DEFINESETATTR_ERR,
   "An error <4> has occurred while setting the attribute <5> with value <6> for Define <7>.\n",



/*---------------------------------------------------
     "<*IF 8>Event Experience level : <9>\n<*ENDIF>"
     "<*IF 10>Event Severity : <11>\n<*ENDIF>"
     "<*IF 12>Event Owner : <13>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_DEFINESAVE_ERR,
   "A Define error <4> has occurred while saving the Define <5>.\n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_RG_STOP,
   "Query <6> estimated cost <4>, exceeds resource policy <5>. \n",
   "Query marked un-excutable. \n",



/*---------------------------------------------------
     "<*IF 7>Event Experience level : <8>\n<*ENDIF>"
     "<*IF 9>Event Severity : <10>\n<*ENDIF>"
     "<*IF 11>Event Owner : <12>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SRV_MONCAL_FAIL,
   "The monitor object call <4> to the MX Connectivity Service failed. \n",



/*---------------------------------------------------
     "<*IF 5>Event Experience level : <6>\n<*ENDIF>"
     "<*IF 7>Event Severity : <8>\n<*ENDIF>"
     "<*IF 9>Event Owner : <10>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_SRV_ITOUT_ERR,
   "Failed to timeout while in idle state, due to an error in the\n",
   "object call to MX Connectivity Service, "
   "or due to timer creation error. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_UPDT_SRV_ST_FAIL,
   "SQL/MX server failed to update its state in the MX Connectivity Service. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_TIP_NOTCONNECT,
   "SQL/MX server failed due to user disconnected from the TIP gateway. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_TIP_NOTCONFIG,
   "SQL/MX server failed due to TIP gateway not configured. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_TIP_ERR,
   "SQL/MX server failed due to TIP gateway error. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

{ZMXO_EVT_CS_POST_CONCT_ERR,
   "ODBC/MX client fails to connect due to error <5> in setting up \n",
   "the connection context <4>. \n",



/*---------------------------------------------------
     "<*IF 6>Event Experience level : <7>\n<*ENDIF>"
     "<*IF 8>Event Severity : <9>\n<*ENDIF>"
     "<*IF 10>Event Owner : <11>\n<*ENDIF>"
---------------------------------------------------*/
  NULL, NULL, NULL, NULL, NULL },

 {ZMXO_EVT_CS_CFG_SRVR_INIT,
   "MXCS Configuration server Initialized. \n",



/*---------------------------------------------------
     "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
     "<*IF 6>Event Severity : <7>\n<*ENDIF>"
     "<*IF 8>Event Owner : <9>\n<*ENDIF>"
---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

// EVENT SAME AS ZMXO_EVT_CS_RG_LOG
{ZMXO_EVT_CS_RG_OVER_LIMIT,
   "Resource Policy Exceeds Limit. \n",
   "Estimated cost: <4>\n",
   "Limit cost: <5>\n",
   "SQL Text: <6>\n",



   NULL, NULL, NULL },


 {ZMXO_EVT_CS_INSUF_PRIVLGS,
   "The user has Insufficient Privileges to start MX Connectivity Service. \n",



 /*---------------------------------------------------
      "<*IF 4>Event Experience level : <5>\n<*ENDIF>"
      "<*IF 6>Event Severity : <7>\n<*ENDIF>"
      "<*IF 8>Event Owner : <9>\n<*ENDIF>"
 ---------------------------------------------------*/
   NULL, NULL, NULL, NULL, NULL, NULL },

//SIGNALS END OF ARRAY, DO NOT REMOVE
{0L, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL}
 };

#endif
