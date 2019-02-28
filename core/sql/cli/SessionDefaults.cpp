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
 * File:         SessionDefaults.cpp
 * Description:  Default settings for cli.
 *               
 * Created:      9/8/2005
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "cli_stdh.h"
#include "ex_stdh.h"
#include "ex_tcb.h"
#include "ExExeUtil.h"
#include "SessionDefaults.h"
#include "exp_clause_derived.h"
#include "sql_id.h"
#include "SQLCLIdev.h"
#include "charinfo.h"
#include "ComRtUtils.h"
#include "ComTdbExeUtil.h"
#include "NLSConversion.h"

#define SDEntry(sesDef, sesDefStr, datatype, isCQD, defTab, isSSD, ext) {sesDef, "" # sesDefStr "", datatype, isCQD, defTab, isSSD, ext}

static const SessionDefaults::SessionDefaultMap sessionDefaultMap[] = 
{
  // Attribute                                       Attr String                 Attr Datatype                        IsCQD    InDef  IsSSD  Ext
  //                                                                                                                           Table
  // ==============================================================================================================================================
  SDEntry(SessionDefaults::ALTPRI_ESP,               ALTPRI_ESP,                 SessionDefaults::SDT_BOOLEAN,        FALSE,   TRUE,  TRUE,  TRUE),
  SDEntry(SessionDefaults::ALTPRI_FIRST_FETCH,       ALTPRI_FIRST_FETCH,         SessionDefaults::SDT_BOOLEAN,        FALSE,   FALSE, TRUE,  FALSE),
  SDEntry(SessionDefaults::ALTPRI_MASTER,            ALTPRI_MASTER,              SessionDefaults::SDT_BOOLEAN,        FALSE,   TRUE,  TRUE,  TRUE),
  SDEntry(SessionDefaults::ALTPRI_MASTER_SEQ_EXE,    ALTPRI_MASTER_SEQ_EXE,      SessionDefaults::SDT_BOOLEAN,        FALSE,   FALSE, TRUE,  FALSE),
  SDEntry(SessionDefaults::AQR_ENTRIES,              AQR_ENTRIES,                SessionDefaults::SDT_ASCII,          FALSE,   TRUE,  TRUE,  FALSE),
  SDEntry(SessionDefaults::AUTO_QUERY_RETRY_WARNINGS,AUTO_QUERY_RETRY_WARNINGS,  SessionDefaults::SDT_BOOLEAN,        TRUE,    TRUE,  FALSE, FALSE),
  SDEntry(SessionDefaults::CALL_EMBEDDED_ARKCMP,     CALL_EMBEDDED_ARKCMP,       SessionDefaults::SDT_BOOLEAN,        TRUE,    FALSE, TRUE,  FALSE),
  SDEntry(SessionDefaults::CANCEL_ESCALATION_INTERVAL,CANCEL_ESCALATION_INTERVAL,SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   FALSE, TRUE,  FALSE),
  SDEntry(SessionDefaults::CANCEL_ESCALATION_MXOSRVR_INTERVAL,
                                                     CANCEL_ESCALATION_MXOSRVR_INTERVAL,
                                                                                 SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   FALSE, TRUE,  FALSE),
  SDEntry(SessionDefaults::CANCEL_ESCALATION_SAVEABEND,
                                                     CANCEL_ESCALATION_SAVEABEND,
                                                                                 SessionDefaults::SDT_BOOLEAN,        FALSE,   FALSE, TRUE,  FALSE),
  SDEntry(SessionDefaults::CANCEL_LOGGING,           CANCEL_LOGGING,             SessionDefaults::SDT_BOOLEAN,        FALSE,   FALSE, TRUE,  FALSE),
  SDEntry(SessionDefaults::CANCEL_QUERY_ALLOWED,     CANCEL_QUERY_ALLOWED,       SessionDefaults::SDT_BOOLEAN,        TRUE,    TRUE,  TRUE,  FALSE),
  SDEntry(SessionDefaults::CANCEL_UNIQUE_QUERY,      CANCEL_UNIQUE_QUERY,        SessionDefaults::SDT_BOOLEAN,        FALSE,   FALSE, TRUE,  FALSE),
  SDEntry(SessionDefaults::CATALOG,                  CATALOG,                    SessionDefaults::SDT_ASCII,          TRUE,    TRUE,  FALSE, FALSE),
  SDEntry(SessionDefaults::COMPILER_IDLE_TIMEOUT,    COMPILER_IDLE_TIMEOUT,      SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   TRUE,  TRUE,  TRUE),  
  SDEntry(SessionDefaults::DBTR_PROCESS,             DBTR_PROCESS,               SessionDefaults::SDT_BOOLEAN,        TRUE,    FALSE, FALSE, FALSE),
  SDEntry(SessionDefaults::ESP_ASSIGN_DEPTH,         ESP_ASSIGN_DEPTH,           SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   TRUE,  TRUE,  TRUE),
  SDEntry(SessionDefaults::ESP_ASSIGN_TIME_WINDOW,   ESP_ASSIGN_TIME_WINDOW,     SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   TRUE,  TRUE,  TRUE),
  SDEntry(SessionDefaults::ESP_CLOSE_ERROR_LOGGING,  ESP_CLOSE_ERROR_LOGGING,    SessionDefaults::SDT_BOOLEAN,        FALSE,   TRUE,  TRUE,  TRUE),
  SDEntry(SessionDefaults::ESP_FIXUP_PRIORITY,       ESP_FIXUP_PRIORITY,         SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   FALSE, TRUE,  FALSE),
  SDEntry(SessionDefaults::ESP_FIXUP_PRIORITY_DELTA, ESP_FIXUP_PRIORITY_DELTA,   SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   TRUE,  TRUE,  TRUE),
  SDEntry(SessionDefaults::ESP_FREEMEM_TIMEOUT,      ESP_FREEMEM_TIMEOUT,        SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   FALSE, TRUE,  TRUE),
  SDEntry(SessionDefaults::ESP_IDLE_TIMEOUT,         ESP_IDLE_TIMEOUT,           SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   TRUE,  TRUE,  TRUE),  
  SDEntry(SessionDefaults::ESP_INACTIVE_TIMEOUT,     ESP_INACTIVE_TIMEOUT,       SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   TRUE,  TRUE,  TRUE),  
  SDEntry(SessionDefaults::ESP_PRIORITY,             ESP_PRIORITY,               SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   TRUE,  TRUE,  TRUE),
  SDEntry(SessionDefaults::ESP_PRIORITY_DELTA,       ESP_PRIORITY_DELTA,         SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   TRUE,  TRUE,  TRUE),
  SDEntry(SessionDefaults::ESP_STOP_IDLE_TIMEOUT,    ESP_STOP_IDLE_TIMEOUT,      SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   TRUE,  TRUE,  TRUE),
  SDEntry(SessionDefaults::ESP_RELEASE_WORK_TIMEOUT, ESP_RELEASE_WORK_TIMEOUT,   SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   TRUE,  TRUE,  TRUE),
  SDEntry(SessionDefaults::INTERNAL_FORMAT_IO,       INTERNAL_FORMAT_IO,         SessionDefaults::SDT_BOOLEAN,        FALSE,   FALSE, TRUE,  FALSE),
  SDEntry(SessionDefaults::ISO_MAPPING,              ISO_MAPPING,                SessionDefaults::SDT_ASCII,          FALSE,   TRUE,  TRUE,  FALSE),
  SDEntry(SessionDefaults::MASTER_PRIORITY,          MASTER_PRIORITY,            SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   TRUE,  TRUE,  TRUE),
  SDEntry(SessionDefaults::MASTER_PRIORITY_DELTA,    MASTER_PRIORITY_DELTA,      SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   TRUE,  TRUE,  TRUE),
  SDEntry(SessionDefaults::MAX_POLLING_INTERVAL,     MAX_POLLING_INTERVAL,       SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   TRUE, TRUE, TRUE),
  SDEntry(SessionDefaults::MXCMP_PRIORITY,           MXCMP_PRIORITY,             SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   TRUE,  TRUE,  TRUE),
  SDEntry(SessionDefaults::MXCMP_PRIORITY_DELTA,     MXCMP_PRIORITY_DELTA,       SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   TRUE,  TRUE,  TRUE),
  SDEntry(SessionDefaults::PARENT_QID,               PARENT_QID,                 SessionDefaults::SDT_ASCII,          FALSE,   FALSE, TRUE,  FALSE),
  SDEntry(SessionDefaults::PARENT_QID_SYSTEM,        PARENT_QID_SYSTEM,          SessionDefaults::SDT_ASCII,          FALSE,   FALSE, TRUE,  FALSE),
  SDEntry(SessionDefaults::PARSER_FLAGS,             PARSER_FLAGS,               SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   FALSE, TRUE,  FALSE),
  SDEntry(SessionDefaults::PERSISTENT_OPENS,         PERSISTENT_OPENS,           SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   TRUE, TRUE, TRUE),
  SDEntry(SessionDefaults::RECLAIM_FREE_MEMORY_RATIO, RECLAIM_FREE_MEMORY_RATIO, SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   FALSE, TRUE,  TRUE),
  SDEntry(SessionDefaults::RECLAIM_FREE_PFS_RATIO,   RECLAIM_FREE_PFS_RATIO,     SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   FALSE, TRUE,  TRUE),
  SDEntry(SessionDefaults::RECLAIM_MEMORY_AFTER,     RECLAIM_MEMORY_AFTER,       SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   FALSE, TRUE,  TRUE),
  SDEntry(SessionDefaults::ROWSET_ATOMICITY,         ROWSET_ATOMICITY,           SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   FALSE, FALSE, FALSE),
  SDEntry(SessionDefaults::RTS_TIMEOUT,              RTS_TIMEOUT,                SessionDefaults::SDT_BINARY_SIGNED,  FALSE,   FALSE, TRUE,  TRUE),
  SDEntry(SessionDefaults::SCHEMA,                   SCHEMA,                     SessionDefaults::SDT_ASCII,          TRUE,    TRUE,  FALSE, FALSE),
  SDEntry(SessionDefaults::STATISTICS_VIEW_TYPE,     STATISTICS_VIEW_TYPE,       SessionDefaults::SDT_ASCII,          FALSE,   FALSE, TRUE,  TRUE),
  SDEntry(SessionDefaults::SUSPEND_LOGGING,          SUSPEND_LOGGING,            SessionDefaults::SDT_BOOLEAN,        FALSE,   FALSE, TRUE,  FALSE),
  SDEntry(SessionDefaults::USE_LIBHDFS,              USE_LIBHDFS,                SessionDefaults::SDT_BOOLEAN,        TRUE,    TRUE,  FALSE, FALSE),
  SDEntry(SessionDefaults::USER_EXPERIENCE_LEVEL,    USER_EXPERIENCE_LEVEL,      SessionDefaults::SDT_ASCII,          TRUE,    TRUE,  FALSE, FALSE),
  SDEntry(SessionDefaults::WMS_PROCESS,              WMS_PROCESS,                SessionDefaults::SDT_BOOLEAN,        FALSE,   FALSE, TRUE,  FALSE)
};

SessionDefaults::SessionDefaults(CollHeap * heap)
     : heap_(heap),
       dbtrProcess_(FALSE),
       nvciProcess_(FALSE),
       mariaQuestProcess_(FALSE),
       internalCli_(FALSE),
       espCloseErrorLogging_(FALSE), // Don't log EMS event for outstanding request at close time
       cliBulkMove_(TRUE),
       aqrEmsEvent_(FALSE),
       aqrType_(1), // SYSTEM
       catalog_(NULL),
       schema_(NULL),
       uel_(NULL),
       internalFormatIO_(FALSE),
       isoMappingName_(NULL),
       isoMappingEnum_(0),
       parserFlags_(0),
       rowsetAtomicity_(-1),
       parentQid_(NULL),
       wmsProcess_(FALSE),
       cancelEscalationInterval_(60),
       cancelEscalationMxosrvrInterval_(120),
       cancelEscalationSaveabend_(FALSE), 
       cancelQueryAllowed_(TRUE),
       cancelUniqueQuery_(FALSE),
       cancelLogging_(TRUE),
       suspendLogging_(TRUE),
       aqrWarn_(0),
       redriveCTAS_(FALSE),
       callEmbeddedArkcmp_(FALSE),
       exsmTraceLevel_(0),
       exsmTraceFilePrefix_(NULL)
{
  parentQidSystem_[0] = '\0'; 
  Lng32 len = sizeof(sessionDefaultMap) / sizeof(SessionDefaultMap);
  ex_assert(len == LAST_SESSION_DEFAULT_ATTRIBUTE, "Mismatch between sessionDefaultMap and SessionDefaultAttribute");
  
  defaultsValueString_ = new(heap) char * [len];
  for (Int32 i = 0; i < len; i++)
    {
      defaultsValueString_[i] = NULL;
    }

  savedDefaultsValueString_ = new(heap) char * [len];
  for (Int32 i = 0; i < len; i++)
    {
      savedDefaultsValueString_[i] = NULL;
    }

  setJdbcProcess(FALSE);
  setOdbcProcess(FALSE);
  setMxciProcess(FALSE);
  char *enableValue;
  enableValue = getenv("ENABLE_EMBEDDED_ARKCMP"); 
  if (enableValue != NULL && enableValue[0] == '0')
     setCallEmbeddedArkcmp(FALSE);
  else
    setCallEmbeddedArkcmp(TRUE);
  setEspPriority(-1);
  setMxcmpPriority(-1);
  // on Linux, we don't change ESP priorities so the related logic was
  // commented off. To enable it remove this an related ifdefs
  // in executor/ex_frag_rt.cpp with mark >>ESP_PRIORITY
  setEspPriorityDelta(0);
  setMxcmpPriorityDelta(6);
  setEspFixupPriority(-1);
  // ESP has higher priority when fixup
  setEspFixupPriorityDelta(1);
  char *espAssignDepthEnvvar = getenv("ESP_ASSIGN_DEPTH");
  if (espAssignDepthEnvvar != NULL)
    setEspAssignDepth(atoi(espAssignDepthEnvvar));
  else
    setEspAssignDepth(-1);
  // Default is 60 (master won't use this esp if it has this or less seconds
  // left to be idle timed out.
  // This value can not be less 30, see ex_control.cpp how it is handled
  // when processing this set session default command
  setEspAssignTimeWindow(60);
  // Default is 60 (master kills ESPs that were idling for 1 minute or more)
  setEspStopIdleTimeout(60);
  // Default is 1800 (idle ESPs time out in 30 minutes)
  setEspIdleTimeout(30*60);
  // Default is 1800 (Compiler Idle time out in 30 minutes)
  setCompilerIdleTimeout(30*60);
  // Default is 0 (inactive ESPs never time out)
  setEspInactiveTimeout(0);
  // how long master waits for release work reply from esps (default is 15
  // minutes)
  setEspReleaseWorkTimeout(15*60);
  setMaxPollingInterval(300);
  char *perOpenssEnvvar = getenv("ESP_PERSISTENT_OPENS");
  if (perOpenssEnvvar != NULL) 
    setPersistentOpens(atoi(perOpenssEnvvar));
  else
    setPersistentOpens(0);
  // Default idle esp freemem timeout is 10 seconds
  setEspFreeMemTimeout(10);
  setRtsTimeout(0);
  
  setAltpriMaster(TRUE);
  setAltpriMasterSeqExe(FALSE);
  setAltpriEsp(FALSE);
  setAltpriFirstFetch(FALSE);

  setEspFreeMemTimeout(espFreeMemTimeout_);
  setEspCloseErrorLogging(espCloseErrorLogging_);

  // set the default to ISO88591
  setIsoMappingName(SQLCHARSETSTRING_ISO88591, strlen(SQLCHARSETSTRING_ISO88591));

  sessionEnvvars_ = new(heap) NAList<SessionEnvvar>(heap);

  setParentQid(NULL, 0);
  setParentQidSystem(NULL, 0);
  setWmsProcess(FALSE);
  aqrInfo_ = new(heap) AQRInfo(heap);
  setRtsTimeout(rtsTimeout_);
  setStatisticsViewType(SQLCLI_PERTABLE_STATS);
  setReclaimTotalMemorySize(DEFAULT_RECLAIM_MEMORY_AFTER);
  setReclaimFreeMemoryRatio(DEFAULT_RECLAIM_FREE_MEMORY_RATIO);
  setReclaimFreePFSRatio(DEFAULT_RECLAIM_FREE_PFS_RATIO);
  setCancelEscalationInterval(60);
  setCancelEscalationMxosrvrInterval(120);
  setCancelEscalationSaveabend(FALSE);
  setModeSeabase(FALSE);
  setUseLibHdfs(FALSE);
}
  
SessionDefaults::~SessionDefaults()
{
  delete sessionEnvvars_;
}

SessionDefaults::SessionDefaultMap SessionDefaults::getSessionDefaultMap
(char * attribute, Lng32 attrLen)
{
  Lng32 len = sizeof(sessionDefaultMap) / sizeof(SessionDefaultMap);
  for (Int32 i = 0; i < len; i++)
    {
      if ((sessionDefaultMap[i].fromDefaultsTable) &&
	  (strcmp(sessionDefaultMap[i].attributeString, attribute)
	  == 0))
	{
	  return sessionDefaultMap[i];
	}
    }

  SessionDefaultMap sdm;
  sdm.attribute = INVALID_SESSION_DEFAULT;
  sdm.attributeString = NULL;
  sdm.attributeType = SessionDefaults::SDT_BOOLEAN;
  sdm.fromDefaultsTable = sdm.isCQD = sdm.isSSD = sdm.externalized = FALSE;

  return sdm;
}

void SessionDefaults::setIsoMappingName(const char * attrValue, Lng32 attrValueLen)
{
  if (isoMappingName_)
    {
      NADELETEBASIC(isoMappingName_, heap_);
    }
  
  isoMappingName_ = new(heap_) char[attrValueLen + 1];
  strncpy(isoMappingName_, attrValue, attrValueLen);
  isoMappingName_[attrValueLen] = '\0';
  
  // upcase isoMappingName_
  str_cpy_convert(isoMappingName_, isoMappingName_, attrValueLen, 1);

  setIsoMappingEnum();
}

void SessionDefaults::setIsoMappingEnum()
{
  isoMappingEnum_ = (Lng32)CharInfo::getCharSetEnum(isoMappingName_);

}

void SessionDefaults::setSessionDefaultAttributeValue
(SessionDefaultMap sda, char * attrValue, Lng32 attrValueLen)
{
  Lng32 defaultValueAsLong = -1;
  NABoolean defaultValueAsBoolean = FALSE;

  if (attrValue)
    {
      if (sda.attributeType == SessionDefaults::SDT_BINARY_SIGNED)
	{
	  ex_expr::exp_return_type rc =
	    convDoIt(attrValue, attrValueLen, REC_BYTE_F_ASCII, 
		     0, 0,
		     (char*)&defaultValueAsLong, sizeof(Lng32), 
		     REC_BIN32_SIGNED, 
		     0, 0, 
		     NULL, 0);
	  if (rc != ex_expr::EXPR_OK)
	    {
	      return; // error
	    }
	}
      else if (sda.attributeType == SessionDefaults::SDT_BOOLEAN)
	{
	  if ((strcmp(attrValue, "ON") == 0) ||
	      (strcmp(attrValue, "TRUE") == 0))
	    defaultValueAsBoolean = TRUE;
	}
    }

  switch (sda.attribute)
    {
    case AQR_ENTRIES:
      {
	aqrInfo()->setAQREntriesFromInputStr(attrValue, attrValueLen);
      }
    break;

    case AUTO_QUERY_RETRY_WARNINGS:
      {
	if (defaultValueAsBoolean)
	  setAQRWarnings(1);
	else
	  setAQRWarnings(0);
      }
    break;

    case DBTR_PROCESS:
      {
	setDbtrProcess(defaultValueAsBoolean);
      }
    break;

    case MXCMP_PRIORITY:
      {
	setMxcmpPriority(defaultValueAsLong);
      }
    break;
    
    case MXCMP_PRIORITY_DELTA:
      {
	setMxcmpPriorityDelta(defaultValueAsLong);
      }
    break;
    
    case ESP_PRIORITY:
      {
	setEspPriority(defaultValueAsLong);
      }
    break;
    
    case ESP_PRIORITY_DELTA:
      {
	setEspPriorityDelta(defaultValueAsLong);
      }
    break;
    
    case ESP_FIXUP_PRIORITY:
      {
	setEspFixupPriority(defaultValueAsLong);
      }
    break;
    
    case ESP_FIXUP_PRIORITY_DELTA:
      {
	setEspFixupPriorityDelta(defaultValueAsLong);
      }
    break;
    
    case ESP_ASSIGN_DEPTH:
      {
	setEspAssignDepth(defaultValueAsLong);
      }
    break;

    case ESP_ASSIGN_TIME_WINDOW:
      {
	setEspAssignTimeWindow(defaultValueAsLong);
      }
    break;

    case ESP_STOP_IDLE_TIMEOUT:
      {
	setEspStopIdleTimeout(defaultValueAsLong);
      }
    break;

    case ESP_IDLE_TIMEOUT:
      {
	setEspIdleTimeout(defaultValueAsLong);
      }
    break;

    case COMPILER_IDLE_TIMEOUT:
      {
	setCompilerIdleTimeout(defaultValueAsLong);
      }
    break;

    case ESP_INACTIVE_TIMEOUT:
      {
	setEspInactiveTimeout(defaultValueAsLong);
      }
    break;

    case ESP_RELEASE_WORK_TIMEOUT:
      {
	setEspReleaseWorkTimeout(defaultValueAsLong);
      }
    break;

    case MAX_POLLING_INTERVAL:
      {
	setMaxPollingInterval(defaultValueAsLong);
      }
    break;
    case PERSISTENT_OPENS:
      {
	setPersistentOpens(defaultValueAsLong);
      }
    break;

    case ESP_CLOSE_ERROR_LOGGING:
      {
	setEspCloseErrorLogging(defaultValueAsBoolean);
      }
    break;

    case CATALOG:
      {
	setCatalog(attrValue, attrValueLen);
      };
    break;

    case SCHEMA:
      {
	setSchema(attrValue, attrValueLen);
      };
    break;

    case USE_LIBHDFS:
      {
         setUseLibHdfs(defaultValueAsBoolean);

      }
      break;
    case USER_EXPERIENCE_LEVEL:
      {
	setUEL(attrValue, attrValueLen);
      };
    break;

    case RTS_TIMEOUT:
      {
        setRtsTimeout(defaultValueAsLong);
      }
      break;

    case ALTPRI_MASTER:
      {
	setAltpriMaster(defaultValueAsBoolean);
      }
    break;

    case ALTPRI_MASTER_SEQ_EXE:
      {
	setAltpriMasterSeqExe(defaultValueAsBoolean);
      }
    break;

    case ALTPRI_FIRST_FETCH:
      {
	setAltpriFirstFetch(defaultValueAsBoolean);
      }
    break;

    case ALTPRI_ESP:
      {
	setAltpriEsp(defaultValueAsBoolean);
      }
    break;
 
    case INTERNAL_FORMAT_IO:
      {
	setInternalFormatIO(defaultValueAsBoolean);
      }
      break;
    case ISO_MAPPING:
      {
        if (attrValueLen != strlen(SQLCHARSETSTRING_ISO88591) ||
            strcmp(attrValue, SQLCHARSETSTRING_ISO88591) != 0)
        {
          // Ignore the specified ISO_MAPPING setting
        }
        setIsoMappingName(SQLCHARSETSTRING_ISO88591, strlen(SQLCHARSETSTRING_ISO88591));
      }
      break;
    case PARENT_QID:
      setParentQid(attrValue, attrValueLen);
      break;
    case PARENT_QID_SYSTEM:
      setParentQidSystem(attrValue, attrValueLen);
      break;
    case WMS_PROCESS:
      setWmsProcess(defaultValueAsBoolean);
      break;
    case ESP_FREEMEM_TIMEOUT:
      {
	setEspFreeMemTimeout(defaultValueAsLong);
      }
      break;
    case STATISTICS_VIEW_TYPE:
      setStatisticsViewType(defaultValueAsLong);
      break;
    case RECLAIM_MEMORY_AFTER:
      setReclaimTotalMemorySize(defaultValueAsLong);
      break;
    case RECLAIM_FREE_MEMORY_RATIO:
      setReclaimFreeMemoryRatio(defaultValueAsLong);
      break;
    case RECLAIM_FREE_PFS_RATIO:
      setReclaimFreePFSRatio(defaultValueAsLong);
      break;
    case CANCEL_ESCALATION_INTERVAL:
      {
	setCancelEscalationInterval(defaultValueAsLong);
      }
      break;
    case CANCEL_ESCALATION_MXOSRVR_INTERVAL:
      {
	setCancelEscalationMxosrvrInterval(defaultValueAsLong);
      }
      break;
    case CANCEL_ESCALATION_SAVEABEND:
      {
	setCancelEscalationSaveabend(defaultValueAsBoolean);
      }
      break;
    case CANCEL_LOGGING:
      {
	setCancelLogging(defaultValueAsBoolean);
      }
      break;
    case CANCEL_QUERY_ALLOWED:
      {
	setCancelQueryAllowed(defaultValueAsBoolean);
      }
      break;
    case CANCEL_UNIQUE_QUERY:
      {
	setCancelUniqueQuery(defaultValueAsBoolean);
      }
      break;
    case SUSPEND_LOGGING:
      {
        setSuspendLogging(defaultValueAsBoolean);
      }
      break;
    case CALL_EMBEDDED_ARKCMP:
      {
	setCallEmbeddedArkcmp(defaultValueAsBoolean);
      }
      break;
    default:
      {
      }
    break;
    };
}

void SessionDefaults::initializeSessionDefault
(char * attribute, Lng32 attrLen,
 char * attrValue, Lng32 attrValueLen)
{
  SessionDefaultMap sda = 
    getSessionDefaultMap(attribute, attrLen);

  if (sda.attribute == INVALID_SESSION_DEFAULT)
    return;

  setSessionDefaultAttributeValue(sda, attrValue, attrValueLen);
}

void SessionDefaults::updateDefaultsValueString(SessionDefaultAttribute sda, 
						const Int16 DisAmbiguate,
						NABoolean value)
{
  if (defaultsValueString_[sda])
    NADELETEBASIC(defaultsValueString_[sda], heap_);

  defaultsValueString_[sda] = new(heap_) char[6];

  if (value == TRUE)
    strcpy(defaultsValueString_[sda], "TRUE");
  else
    strcpy(defaultsValueString_[sda], "FALSE");
}

void SessionDefaults::updateDefaultsValueString(SessionDefaultAttribute sda, 
						Lng32 value)
{
  if (defaultsValueString_[sda])
    NADELETEBASIC(defaultsValueString_[sda], heap_);

  defaultsValueString_[sda] = new(heap_) char[12];
  
  if (value < 0)
    {
      defaultsValueString_[sda][0] = '-';
      
      str_itoa((ULng32)(-value), &defaultsValueString_[sda][1]);
    }
  else
    str_itoa((ULng32)(value), defaultsValueString_[sda]);
}

void SessionDefaults::updateDefaultsValueString(SessionDefaultAttribute sda, 
						char * value)
{
  if (defaultsValueString_[sda])
    NADELETEBASIC(defaultsValueString_[sda], heap_);
  
  if (value)
  {
    defaultsValueString_[sda] = new(heap_) char[strlen(value) + 1];
    strcpy(defaultsValueString_[sda], value);
  }
  else
    defaultsValueString_[sda] = NULL;
}

Lng32 SessionDefaults::setIsoMappingDefine()
{

  return 0;
}

void SessionDefaults::saveSessionDefaults()
{
  Lng32 len = sizeof(sessionDefaultMap) / sizeof(SessionDefaultMap);

  for (Int32 i = 0; i < len; i++)
    {
      NADELETEBASIC(savedDefaultsValueString_[i], heap_);

      if (defaultsValueString_[i])
	{
	  savedDefaultsValueString_[i] = 
	    new(heap_) char[strlen(defaultsValueString_[i]) + 1];
	  strcpy(savedDefaultsValueString_[i], defaultsValueString_[i]);
	}
      else
	{
	  savedDefaultsValueString_[i] = NULL;
	}
    }

}

void SessionDefaults::restoreSessionDefaults()
{
  Lng32 len = sizeof(sessionDefaultMap) / sizeof(SessionDefaultMap);

  for (Int32 i = 0; i < len; i++)
    {
      if (savedDefaultsValueString_[i])
	{
	  setSessionDefaultAttributeValue(
	       sessionDefaultMap[i],
	       savedDefaultsValueString_[i],
	       strlen(savedDefaultsValueString_[i]));
	}
    }

}

Lng32 SessionDefaults::readFromDefaultsTable(CliGlobals * cliGlobals)
{
  // Read system defaults from configuration file
  // keep this name in sync with file sqlcomp/nadefaults.cpp
  NAString confFile(getenv("TRAF_CONF"));
  confFile += "/SQSystemDefaults.conf";

  FILE *f = fopen(confFile, "r");

  if (f)
    {
      char attrName[101];
      char attrValue[4000];
      int rc = 0;

      while (rc = fscanf(f, " %100[A-Za-z0-9_#] ,", attrName) == 0)
        {
          fgets(attrValue, sizeof(attrValue), f);
          NAString attrNameUp(attrName);
          
          attrNameUp.toUpper();
          if (attrNameUp[0] != '#')
            initializeSessionDefault((char *) attrNameUp.data(),
                                     attrNameUp.length(),
                                     attrValue,
                                     str_len(attrValue));
        }
    }

  return 0;
}

void SessionDefaults::position()
{
  currDef_ = 0;
}

short SessionDefaults::getNextSessionDefault(char* &attributeString, 
					     char* &attributeValue,
					     Lng32  &isCQD,
					     Lng32  &fromDefaultsTable,
					     Lng32  &isSSD,
					     Lng32  &isExternalized)
{
  Lng32 len = sizeof(sessionDefaultMap) / sizeof(SessionDefaultMap);
  if (currDef_ == len)
    return -1;

  attributeString = (char*)sessionDefaultMap[currDef_].attributeString;

  attributeValue  = 
    defaultsValueString_[sessionDefaultMap[currDef_].attribute];

  isCQD                   = sessionDefaultMap[currDef_].isCQD;
  fromDefaultsTable       = sessionDefaultMap[currDef_].fromDefaultsTable;
  isSSD                   = sessionDefaultMap[currDef_].isSSD;
  isExternalized          = sessionDefaultMap[currDef_].externalized;
  currDef_++;

  return 0;
}

//////////////////////////////////////////////////////////////////////
// class AQRInfo
//////////////////////////////////////////////////////////////////////
static const QueryString cqdInfo[] =
{
  {"unique_hash_joins"}, {"OFF"}
, {"transform_to_sidetree_insert"}, {"OFF"}
, {"METADATA_CACHE_SIZE"}, {"0"}
, {"QUERY_CACHE"}, {"0"}
, {"TRAF_RELOAD_NATABLE_CACHE"}, {"ON"}
};

static const AQRInfo::AQRErrorMap aqrErrorMap[] = 
{
  //      SQLCODE NSKCODE  RETRIES   DELAY   TYPE NUM  CQD   CMP  INTERNAL
  //                                              CQDS STR   INFO
  // =====================================================================

  // object does not exist in Trafodion
  AQREntry(   1009,      0,      1,     0,      1,   1, "03",  0,     0),
  AQREntry(   1254,      0,      1,     0,      1,   1, "03",  0,     0),
  AQREntry(   1389,      0,      1,     0,      1,   1, "03",  0,     0),
  AQREntry(   1584,      0,      1,     0,      1,   1, "03",  0,     0),

  // process could not be created (40)
  AQREntry(   2012,      0,      3,    60,      0,   0, "",    0,     0),  

  // comm failure with esp (201, 246, 249)
  AQREntry(   2034,      0,      1,     0,      2,   0, "",    0,     0), 
  AQREntry(   2235,      0,      1,     0,      1,   1, "03",  0,     0),
  AQREntry(   4001,      0,      1,     0,      1,   1, "03",  0,     0),
  AQREntry(   4023,      0,      1,     0,      1,   1, "03",  0,     0),
  AQREntry(   4039,      0,      1,     0,      1,   1, "03",  0,     0),

  // parallel purgedata failed
  AQREntry(   8022,      0,      3,    60,      0,   0, "",    0,     1),

  // hive data modification timestamp mismatch.
  // query will be AQR'd and hive metadata will be reloaded.
  AQREntry(   8436,      0,      1,     0,      0,   2, "04:05",  0,     0),

  // FS memory errors
  AQREntry(   8550,     30,      1,    60,      0,   0, "",    0,     0),
  AQREntry(   8550,     31,      1,    60,      0,   0, "",    0,     0),
  AQREntry(   8550,     33,      1,    60,      0,   0, "",    0,     0),
  AQREntry(   8550,     35,      1,    60,      0,   0, "",    0,     0),

  // locked row timeout
  AQREntry(   8550,     73,      2,     0,      0,   0, "",    0,     0),  
  AQREntry(   8550,     78,      1,    60,      0,   0, "",    0,     0),
  AQREntry(   8551,     12,      1,    60,      0,   0, "",    0,     0),

  // DP2/FS memory errors
  AQREntry(   8551,     30,      1,    60,      0,   0, "",    0,     0),
  AQREntry(   8551,     31,      1,    60,      0,   0, "",    0,     0),
  AQREntry(   8551,     33,      1,    60,      0,   0, "",    0,     0),
  AQREntry(   8551,     35,      1,    60,      0,   0, "",    0,     0),

  // operation timeout against dp2
  AQREntry(   8551,     40,      1,    60,      0,   0, "",    0,     0),

  // locked row timeout
  AQREntry(   8551,     73,      2,     0,      0,   0, "",    0,     0),
  AQREntry(   8551,     78,      1,    60,      0,   0, "",    0,     0),
  AQREntry(   8551,    200,      1,    30,      0,   0, "",    0,     0),

  AQREntry(   8551,   1054,      1,    60,      0,   0, "",    0,     0),

  // out of sqlmxbufferspace in dp2
  AQREntry(   8551,   1187,      1,    60,      0,   0, "",    0,     0),

  // out of server storage
  AQREntry(   8551,   3502,      1,    60,      0,   0, "",    0,     0),

  // locked row timeout
  AQREntry(   8558,     0 ,      2,    10,      0,   0, "",    0,     0), 

  // lost open
  AQREntry(   8574,      0,      1,     0,      1,   0, "",    0,     0), 

  // timestamp mismatch
  AQREntry(   8575,      0,      1,     0,      1,   0, "",    0,     0), 

  // table not found
  AQREntry(   8577,      0,      1,     0,      1,   2, "04:05",    0,  0), 

  // sim check failure
  AQREntry(   8579,      0,      1,     0,      1,   0, "",    0,     0),

  // unavailable partitions
  AQREntry(   8580,      0,      1,     0,      0,   0, "",    0,     0),

  // in-memory join out of memory
  AQREntry(   8581,      0,      1,     0,      1,   1, "01",  0,     0), 

 // No generated plan 
  AQREntry(   8583,      0,      1,     0,      1,   0, "",    0,     1), 
  // Schema security changed
  AQREntry(   8585,      0,      1,     0,      1,   0, "",    0,     0), 

  // esp could have died during fragment assignment
  AQREntry(   8586,      0,      1,    60,      2,   0, "",    0,     0), 

  // could not transform to sidetree insert. Reprepare as regular no rollback insert.
  AQREntry(   8587,      0,      1,     0,      1,   1, "02",    0,     0), 

  // dead esp
  AQREntry(   8596,      0,      1,     0,      2,   0, "",    0,     0), 

  // view not found
  AQREntry(   8598,      0,      1,     0,      1,   0, "",    0,     0), 

  AQREntry(   8606,     73,      3,    60,      0,   0, "",    0,     0), 

  AQREntry(   8606,     97,      1,     0,      5,   0, "",    0,     0),
  // privileges may have been revoked
  AQREntry(   8734,      0,      1,     0,      1,   0, "",    0,     0),

  // DDL change detected before query started
  AQREntry(   8738,      0,      1,     0,      1,   0, "",    0,     0),

  // internal error, no entries in diags.
  AQREntry(   8810,      0,      1,    60,      0,   0, "",    0,     1), 

  // transaction mode mismatch
  AQREntry(   8814,      0,      1,     0,      0,   0, "",    0,     0),

  // compiler died
  AQREntry(   8838,      0,      1,     0,      0,   0, "",    0,     1),

  // transaction state needs to be cleaned up in compiler - kill compiler
  // only valid after prepare errors
  AQREntry(  8841,       0,      1,     0,      4,   0, "",    0,     0),

  // transaction state needs to be cleaned up in compiler - kill compiler
  // only valid after prepare errors.
  AQREntry(  8844,       0,      1,     0,      4,   0, "",    0,     0),

  // Error was returned by the SeaMonster API
  AQREntry(   8951,  10001,      1,    30,      0,   0, "",    0,     0),
  AQREntry(   8951,  10012,      1,     0,      2,   0, "",    0,     0),

  // transaction state needs to be cleaned up in compiler - kill compiler
  // only valid after prepare errors.
  AQREntry(  8613,       0,      1,     0,      4,   0, "",    0,     0),
  // memory condition  in compiler - kill compiler
  // only valid after prepare errors.
  AQREntry(  2008,       0,      1,     0,      4,   0, "",    0,     0),
  // UNLOAD failed to open file
  // all three UNLOAD errors are retried only in NO APPEND mode
  AQREntry(  8960,      0,      1,     0,      2,   0, "",    0,     0),
  // UNLOAD failed during write to file
  AQREntry(  8961,      0,      1,     0,      2,   0, "",    0,     0), 
  // UNLOAD failed to allocate buffers at start of execution
  AQREntry(  8962,      0,      1,     0,      2,   0, "",    0,     0) 

};


AQRInfo::AQRInfo(CollHeap * heap)
     : heap_(heap),
       currErr_(0),
       aqrStmtInfo_(NULL),
       flags_(0)
{
  Lng32 numEntries = sizeof(aqrErrorMap) / sizeof(AQRErrorMap);

  aqrErrorList_ = new(heap) LIST(AQRErrorMap) (heap);
  for (Int32 i = 0; i < numEntries; i++)
    {
      AQRErrorMap tgt;
      AQRErrorMap src = aqrErrorMap[i];
      tgt.sqlcode = src.sqlcode;
      tgt.nskcode = src.nskcode;
      tgt.retries = src.retries;
      tgt.delay = src.delay;
      tgt.type = src.type;
      tgt.numCQDs = src.numCQDs;
      tgt.cqdStr = NULL;
      tgt.cmpInfo = src.cmpInfo;
      tgt.intAQR  = src.intAQR;
      if (tgt.numCQDs > 0)
	{
	  tgt.cqdStr = new(heap_) char[strlen(src.cqdStr) + 1];
	  strcpy((char*)tgt.cqdStr, src.cqdStr);
	}

      aqrErrorList_->insert(tgt); //aqrErrorMap[i]);
    }

  origAqrErrorList_ = new(heap) LIST(AQRErrorMap) (heap);
  origAqrErrorList_->insert(*aqrErrorList_);


}

AQRInfo::~AQRInfo()
{
}

short AQRInfo::setAQREntry(Lng32 task,
			   Lng32 sqlcode, Lng32 nskcode,
			   Int32 retries, Int32 delay, Int32 type,
			   Int32 numCQDs, char * cqdStr,
			   Lng32 cmpInfo, Lng32 intAQR)
{
  if ((sqlcode < 0) || (nskcode < 0) || (retries < 0) || (delay < 0) ||
      (type < 0) || (numCQDs < 0) || (cmpInfo < 0) || (cmpInfo > 1))
    return -1;

  UInt32 entry = 0;
  NABoolean found = FALSE;
  while ((NOT found) && (entry < aqrErrorList_->entries()))
    {
      entry++;

      Int64 currSqlcode = (*aqrErrorList_)[(entry-1)].sqlcode;
      Int64 currNskcode = (*aqrErrorList_)[(entry-1)].nskcode;
      if ((currSqlcode == sqlcode) &&
	  (currNskcode == nskcode))
	{
	  found = TRUE;
      }
    } // while
  
  switch (task)
    {
    case ComTdbExeUtilAQR::ADD_: // add
      {
	if (found)
	  {
	    // check if it's the same string 
	    if ( ((*aqrErrorList_)[(entry-1)].delay == delay ) &&
		 ((*aqrErrorList_)[(entry-1)].retries == retries ))
	      //ignore . it's the same entry
	      return 0;
	    else
	      {
		return -1;
	      }
	    
	    
	  }
	else
	  {
	    // insert the new entry sorted on sqlcode/nskcode.
	    AQRErrorMap r;
	    r.sqlcode = sqlcode;
	    r.nskcode = nskcode;
	    r.retries = retries;
	    r.delay = delay;
	    r.type = type;
	    r.numCQDs = numCQDs;
	    r.cqdStr = NULL;
	    if (r.numCQDs > 0)
	      {
		r.cqdStr = new(heap_) char[strlen(cqdStr) + 1];
		strcpy((char*)r.cqdStr, cqdStr);
	      }
	    r.cmpInfo = cmpInfo;
	    r.intAQR  = intAQR;

	    NABoolean done = FALSE;
	    entry = 0;
	    while ((NOT done) && (entry < aqrErrorList_->entries()))
	      {
		Int64 currSqlcode = (*aqrErrorList_)[entry].sqlcode;
		Int64 currNskcode = (*aqrErrorList_)[entry].nskcode;
		if ((sqlcode < currSqlcode) ||
		    ((sqlcode == currSqlcode) &&
		     (nskcode < currNskcode)))
		  {
		    done = TRUE;
		  }
		else
		  entry++;
	      } // while
	    
	    aqrErrorList_->insertAt(entry, r);
	  }
      } // add
    break;

    case ComTdbExeUtilAQR::DELETE_: // delete
      {
	// remove errors
	if (found)
	  {
	    NADELETEBASIC((*aqrErrorList_)[(entry-1)].cqdStr, heap_);
	    aqrErrorList_->removeAt(entry-1);
	  }
	else
	  {
	    // not found, ignore
	    return 0;
	  }
      }
    break;

    case ComTdbExeUtilAQR::UPDATE_: // update
      {
	if (found) 	  
	  if ( ((*aqrErrorList_)[(entry-1)].delay == delay ) &&
	       ((*aqrErrorList_)[(entry-1)].retries == retries ))
	    {
	      // ignore the update - it's the same 
	      return 0;
	    }
	  else
	    {
	      (*aqrErrorList_)[(entry-1)].retries = retries;
	      (*aqrErrorList_)[(entry-1)].delay = delay;
	      (*aqrErrorList_)[(entry-1)].type = type;
	      (*aqrErrorList_)[(entry-1)].cmpInfo = cmpInfo;
	      (*aqrErrorList_)[(entry-1)].intAQR = intAQR;
	    }
	else
	  {
	    // not found, return error.
	    return -1;
	  }
      }
    break;
    
    } // switch

  return 0;
}

short AQRInfo::setAQREntriesFromInputStr(char * inStr, Lng32 inStrLen)
{
  if ((! inStr) || (inStrLen <= 0))
    return -1;

  char * newStr = new(heap_) char[inStrLen + 1 + 1];
  str_cpy_all(newStr, inStr, inStrLen);
  newStr[inStrLen] = 0;
  Lng32 n = 0;

  Int32 i = 0;
  while (i < inStrLen)
    {
      if ((inStr[i] != '+') &&
	  (inStr[i] != '-') &&
	  (inStr[i] != '.') &&
	  (inStr[i] != ' ') &&
	  (inStr[i] != ',') &&
	  (inStr[i] != '|') &&
	  (NOT ((inStr[i] >= '0') &&
		(inStr[i] <= '9'))))
	return -1;

      if (inStr[i] != ' ')
	{
	  newStr[n] = inStr[i];
	  n++;
	}

      i++;
    }

  if (newStr[n-1] != '|')
    {
      newStr[n] = '|';
      n++;
    }
  newStr[n] = 0;

  i = 0;
  Int32 j = 0;
  Int32 k = 1;
  Lng32 sqlcode = -1;
  Lng32 nskcode = 0;
  Lng32 retries = 1;
  Lng32 delay = 60;
  Lng32 type = 0;
  Lng32 numCQDs = 0;
  char * cqdStr = NULL;
  Lng32 cmpInfo = 0;
  Lng32 intAQR = 0;
  Lng32 task = ComTdbExeUtilAQR::NONE_;
  NABoolean numberSeen = FALSE;
  while (i < n)
    {
      if ((newStr[i] >= '0') &&
	  (newStr[i] <= '9'))
	numberSeen = TRUE;

      if (newStr[i] == '+')
	{
	  if ((numberSeen) ||
	      (task != ComTdbExeUtilAQR::NONE_))
	    return -1;

	  task = ComTdbExeUtilAQR::ADD_;
	  j++;
	}
      else if (newStr[i] == '-')
	{
	  if ((numberSeen) ||
	      (task != ComTdbExeUtilAQR::NONE_))
	    return -1;

	  task = ComTdbExeUtilAQR::DELETE_;
	  j++;
	}
      else if (newStr[i] == '.')
	{
	  if ((numberSeen) ||
	      (task != ComTdbExeUtilAQR::NONE_))
	    return -1;

	  task = ComTdbExeUtilAQR::UPDATE_;
	  j++;
	}

      if ((newStr[i] == ',') ||
	  (newStr[i] == '|'))
	{
	  if (i > j)
	    {
	      Lng32 v = 0;
	      if ((k < 7) || (k == 8) || (k == 9))
		{
		  Int64 bigV = str_atoi(&newStr[j], i-j);
		  if (bigV == -1)
		    return -1;
		  
		  if (bigV > INT_MAX)
		    return -1;

		  v = (Lng32)bigV;
		}

	      switch (k)
		{
		case 1:
		  sqlcode = v;
		  break;
		  
		case 2:
		  nskcode = v;
		  break;
		  
		case 3:
		  retries = v;
		  break;
		  
		case 4:
		  delay = v;
		  break;
		  
		case 5:
		  type = v;
		  break;

		case 6:
		  numCQDs = v;
		  break;

		case 7:
		  cqdStr = new(heap_) char[i-j+1];
		  str_cpy_all(cqdStr, &newStr[j], i-j);
		  cqdStr[i-j] = 0;
		  break;
		  
		case 8:
		  cmpInfo = v;
		  break;

		case 9:
		  intAQR = v;
		  break;

		} // switch
	    }

	  k++;

	  j = i + 1;
	}

      if (newStr[i] == '|')
	{
	  if (task == ComTdbExeUtilAQR::NONE_)
	    task = ComTdbExeUtilAQR::ADD_;

	  if (setAQREntry(task, sqlcode, nskcode, retries, delay, type,
			  numCQDs, cqdStr, cmpInfo, intAQR))
	    return -1;

	  sqlcode = -1;
	  nskcode = 0;
	  retries = 1;
	  delay = 60;
	  type = 0;
	  numCQDs = 0;
	  cqdStr = NULL;
	  cmpInfo = 0;
	  intAQR = 0;

	  numberSeen = FALSE;
	  task = ComTdbExeUtilAQR::NONE_;

	  k = 1;
	  j = i+1;
	}
	
      i++;
    }

  return 0;
}


short AQRInfo::getAQREntry(Lng32 sqlcode, Lng32 nskcode,
			   Lng32 &retries, Lng32 &delay, 
			   Lng32 &type,
			   Int32 &numCQDs, char* &cqdStr,
			   Lng32 &cmpInfo, Lng32 &intAQR)
{
  NABoolean sqlcodeZeroFound = FALSE;
  UInt32 sqlcodeZeroEntryNum = 0;
  NABoolean nskcodeZeroFound = FALSE;
  UInt32 nskcodeZeroEntryNum = 0;
  for (UInt32 i = 0; i < aqrErrorList_->entries(); i++)
    {
      NABoolean found = FALSE;

      // if '0' has been set as sqlcode by user, then retry on all errors.
      if (0 == (*aqrErrorList_)[i].sqlcode)
	{
	  sqlcodeZeroFound = TRUE;
	  sqlcodeZeroEntryNum = i;
	}
      else if ((ABS(sqlcode) == (*aqrErrorList_)[i].sqlcode) &&
	       ((*aqrErrorList_)[i].nskcode == 0))
	{
	  nskcodeZeroFound = TRUE;
	  nskcodeZeroEntryNum = i;
	}
      else if ((ABS(sqlcode) == (*aqrErrorList_)[i].sqlcode) &&
	       (ABS(nskcode) == (*aqrErrorList_)[i].nskcode))
	found = TRUE;
      
      if (found)
	{
	  retries = (Int32)(*aqrErrorList_)[i].retries;
	  delay = (Int32)(*aqrErrorList_)[i].delay;
	  type = (Int32)(*aqrErrorList_)[i].type;
	  numCQDs = (Int32)(*aqrErrorList_)[i].numCQDs;
	  cqdStr = (char*)(*aqrErrorList_)[i].cqdStr;
	  cmpInfo = (Int32)(*aqrErrorList_)[i].cmpInfo;
	  intAQR = (Int32)(*aqrErrorList_)[i].intAQR;

	  return 0;
	}
    }

  if ((nskcodeZeroFound) || (sqlcodeZeroFound))
    {
      UInt32 i = 
	(nskcodeZeroFound ? nskcodeZeroEntryNum : sqlcodeZeroEntryNum);
      
      retries = (Int32)(*aqrErrorList_)[i].retries;
      delay = (Int32)(*aqrErrorList_)[i].delay;
      type = (Int32)(*aqrErrorList_)[i].type;
      numCQDs = (Int32)(*aqrErrorList_)[i].numCQDs;
      cqdStr = (char*)(*aqrErrorList_)[i].cqdStr;
      cmpInfo = (Int32)(*aqrErrorList_)[i].cmpInfo;
      intAQR = (Int32)(*aqrErrorList_)[i].intAQR;

      return 0;
    }

  return -1;
}

void AQRInfo::position()
{
  currErr_ = 0;
}

short AQRInfo::getNextAQREntry(Lng32 &sqlcode, Lng32 &nskcode,
			       Lng32 &retries, Lng32 &delay, 
			       Lng32 &type, Lng32 &intAQR)
{
  if (currErr_ == aqrErrorList_->entries())
    return -1;

  sqlcode = (*aqrErrorList_)[currErr_].sqlcode;
  nskcode = (*aqrErrorList_)[currErr_].nskcode;
  retries = (*aqrErrorList_)[currErr_].retries;
  delay   = (*aqrErrorList_)[currErr_].delay;
  type    = (*aqrErrorList_)[currErr_].type;
  intAQR  = (*aqrErrorList_)[currErr_].intAQR;

  currErr_++;

  return 0;
}

void AQRInfo::saveAQRErrors()
{
 aqrErrorList_->clear();
 aqrErrorList_->insert(*origAqrErrorList_);
}

NABoolean AQRInfo::restoreAQRErrors()
{
  if (origAqrErrorList_)
    {
      if (origAqrErrorList_->isEmpty())
	{
	  return FALSE;
	}
    
      aqrErrorList_->clear();
      aqrErrorList_->insert(*origAqrErrorList_);
    }
  return TRUE;
}


void AQRInfo::clearRetryInfo()
{
  if (aqrStmtInfo_)
    {
      aqrStmtInfo_ = NULL;
    }

  flags_ = 0;
}

AQRStatementInfo::AQRStatementInfo(CollHeap * heap)
{
  heap_ = heap; 
  savedStmtAttributes_ = new (heap) AQRStatementAttributes(heap);
  clearRetryInfo();
  
}

void AQRStatementInfo::clearRetryInfo()
{
  retryStatementId_ = NULL;
  retrySqlSource_   = NULL;
  retryInputDesc_   = NULL;
  retryOutputDesc_  = NULL;
  retryTempInputDesc_   = NULL;
  retryTempOutputDesc_  = NULL;
  retryPrepareFlags_ = 0;
  retryFlags_ = 0;
  retryStatementHandle_ = 0;
  if (savedStmtAttributes_)
    savedStmtAttributes_->clear();
   
}

void AQRStatementInfo::saveAttributesFromStmt(Statement *fromStmt)
{
  if (savedStmtAttributes_)
    savedStmtAttributes_->getAttributesFromStatement(fromStmt);
}
void AQRStatementInfo::copyAttributesToStmt(Statement *toStmt)
{
  if (savedStmtAttributes_)
    savedStmtAttributes_->setAttributesInStatement(toStmt);
}
AQRStatementAttributes::AQRStatementAttributes(CollHeap *heap)
{
  heap_ = heap;
  holdable_ = SQLCLIDEV_NONHOLDABLE;
  rowsetAtomicity_ = Statement::UNSPECIFIED_; 
  inputArrayMaxsize_ = 0;
  uniqueStmtId_ = NULL;
  uniqueStmtIdLen_ = 0;
  parentQID_ = NULL;
  parentQIDSystem_[0] = '\0';
  exeStartTime_ = -1;    
}

AQRStatementAttributes::~AQRStatementAttributes()
{
  if (uniqueStmtId_)
    NADELETEBASIC(uniqueStmtId_,heap_);
  if (parentQID_)
    NADELETEBASIC(parentQID_,heap_);
}
void AQRStatementAttributes::clear()
{
  holdable_ = SQLCLIDEV_NONHOLDABLE;
  rowsetAtomicity_ = Statement::UNSPECIFIED_; 
  inputArrayMaxsize_ = 0;
  uniqueStmtId_ = NULL;
  uniqueStmtIdLen_ = 0;
  parentQID_ = NULL;
  parentQIDSystem_[0] = '\0';
  exeStartTime_ = -1;
}
void AQRStatementAttributes::setAttributesInStatement(Statement *targetStmt)
{
  ComDiagsArea d;
  if(targetStmt)
    {
      targetStmt->setHoldable(holdable_);
      targetStmt->setRowsetAtomicity(d,rowsetAtomicity_);
      targetStmt->setInputArrayMaxsize(d,inputArrayMaxsize_);
      targetStmt->setUniqueStmtId(uniqueStmtId_);
      targetStmt->setParentQid(parentQID_);
      targetStmt->setParentQidSystem(parentQIDSystem_);
      targetStmt->setExeStartTime(exeStartTime_);
    }
  return;
  
}
void AQRStatementAttributes::getAttributesFromStatement(Statement *fromStmt)
{
  Lng32 len;
  if (fromStmt)
    {
      holdable_ = fromStmt->getHoldable();
      rowsetAtomicity_ = fromStmt->getRowsetAtomicity();
      inputArrayMaxsize_ = fromStmt->getInputArrayMaxsize();
      if (fromStmt->getUniqueStmtId())
	{
	  uniqueStmtId_ = new(heap_) char[fromStmt->getUniqueStmtIdLen() + 1];
	  str_cpy_all(uniqueStmtId_, fromStmt->getUniqueStmtId(), fromStmt->getUniqueStmtIdLen());
	  uniqueStmtId_[fromStmt->getUniqueStmtIdLen()] = 0;
	}
      uniqueStmtIdLen_ = fromStmt->getUniqueStmtIdLen();
 
      if (fromStmt->getParentQid())
	{
	  len = str_len(fromStmt->getParentQid());
	  parentQID_ = new(heap_) char[len+1];
	  str_cpy_all(parentQID_, fromStmt->getParentQid(),len);
	  parentQID_[len] = 0;
	}
       len = str_len(fromStmt->getParentQidSystem());
       str_cpy_all(parentQIDSystem_, fromStmt->getParentQidSystem(), len);
       parentQIDSystem_[len] = '\0';
       if (fromStmt->getStatsArea() && fromStmt->getStatsArea()->getMasterStats())
           exeStartTime_ = fromStmt->getStatsArea()->getMasterStats()->getExeStartTime();
 
    }
  return;
}

Lng32 AQRInfo::setCQDs(Lng32 numCQDs, char * cqdStr,
		      ContextCli * context)
{
  Lng32 rc = 0;
  if (numCQDs > 0)
    {
      for (Lng32 i = 0; i < numCQDs; i++)
	{
	  // for each entry in the cqdStr, extract the corresponding
	  // entry from cqdInfo array and execute that cqd.
	  // Entries in cqdStr are of the form:
	  //  NN:NN:....NN
	  //  For ex:   numCQDs=2 and cqdStr = 01:03   
	  // would indicate that entries #1 and #3 from the cqdInfo
	  // array need to be executed and set.
	  Int64 v = str_atoi(&cqdStr[i*3], 2);
	  if (v <= 0)
	    return -1;

	  rc = context->holdAndSetCQD(cqdInfo[(v-1)*2].str,
				      cqdInfo[(v-1)*2+1].str);
	  if (rc < 0)
	    return rc;
	}
    }
  
  return 0;
}

Lng32 AQRInfo::resetCQDs(Lng32 numCQDs, char * cqdStr,
			  ContextCli * context)
{
  Lng32 rc = 0;
  if (numCQDs > 0)
    {
      for (Lng32 i = 0; i < numCQDs; i++)
	{
	  Int64 v = str_atoi(&cqdStr[i*3], 2);
	  if (v <= 0)
	    return -1;

	  rc = context->restoreCQD(cqdInfo[(v-1)*2].str);
	  if (rc < 0)
	    return rc;
	}
    }

  return rc;
}

Lng32 AQRInfo::setCompilerInfo(char * queryId,
			      ComCondition * errCond,
			      ContextCli * context)
{
  Lng32 rc = 0;
  char * csVal = NULL;
  
  Lng32 len = 0;
  len = strlen("QUERY_ID: ") + strlen(queryId) + 1;

  csVal = new(heap_) char[len];

  str_sprintf(csVal, "QUERY_ID: %s", queryId);

  rc = context->setCS("AQR_COMPILER_INFO",
		      csVal);

  return rc;
}

Lng32 AQRInfo::resetCompilerInfo(char * queryId,
				ComCondition * errCond,
				ContextCli * context)
{
  Lng32 rc = 0;

  rc = context->resetCS("AQR_COMPILER_INFO");

  return rc;
}

void SessionDefaults::resetSessionOnlyAttributes()
{
  nvciProcess_ = FALSE;
  internalCli_ = FALSE;
}

SessionEnvvar::SessionEnvvar(CollHeap * heap, 
			     char * envvarName, char * envvarValue)
  : heap_(heap),
    envvarName_(NULL), envvarValue_(NULL)
{
  if (! envvarName)
    return;

  envvarName_ = new(heap) char[strlen(envvarName)+1];
  strcpy(envvarName_, envvarName);

  if (envvarValue)
    {
      envvarValue_ = new(heap) char[strlen(envvarValue)+1];
      strcpy(envvarValue_, envvarValue);
    }
  else
    envvarValue_ = NULL;
}

SessionEnvvar::SessionEnvvar()
{
  heap_ = NULL;
  envvarName_ = NULL;
  envvarValue_ = NULL;
}

SessionEnvvar::~SessionEnvvar()
{
  if (envvarName_)
    NADELETEBASIC(envvarName_, heap_);

  if (envvarValue_)
    NADELETEBASIC(envvarValue_, heap_);
}

NABoolean SessionEnvvar::operator ==(const SessionEnvvar &other) const
{
  if ((! envvarName_) || (! other.envvarName_))
    return FALSE;

  if (strcmp(envvarName_, other.envvarName_) == 0)
    return TRUE;
  else
    return FALSE;
}

SessionEnvvar& SessionEnvvar::operator =(const SessionEnvvar &other)
{
  if (envvarName_)
    NADELETEBASIC(envvarName_, heap_);

  if (envvarValue_)
    NADELETEBASIC(envvarValue_, heap_);

  heap_ = other.heap_;

  if (other.envvarName_)
    {
      envvarName_ = new(heap_) char[strlen(other.envvarName_)+1];
      strcpy(envvarName_, other.envvarName_);
    }
  else
    envvarName_ = NULL;

  if (other.envvarValue_)
    {
      envvarValue_ = new(heap_) char[strlen(other.envvarValue_)+1];
      strcpy(envvarValue_, other.envvarValue_);
    }
  else
    envvarValue_ = NULL;

  return (*this);
}

SessionEnvvar::SessionEnvvar(const SessionEnvvar &other)
{
  *this = other;
}

void SessionDefaults::setParentQid(char *attrValue, Lng32 attrValueLen)
{
  if (parentQid_)
  {
    NADELETEBASIC(parentQid_, heap_);
  }
  if (attrValue)
  {
    if (attrValueLen == 4 && strncmp(attrValue, "NONE", 4) == 0)
      parentQid_ = NULL;
    else
    {
      parentQid_ = new(heap_) char[attrValueLen + 1];
      strncpy(parentQid_, attrValue, attrValueLen);
      parentQid_[attrValueLen] = '\0';
    }
  }
  else
    parentQid_ = NULL;
  updateDefaultsValueString(PARENT_QID, parentQid_);
}

void SessionDefaults::setParentQidSystem(char *attrValue, Lng32 attrValueLen)
{
  if (attrValue != NULL)
  {
    if (attrValueLen == 4 && strncmp(attrValue, "NONE", 4) == 0)
       parentQidSystem_[0] = '\0';
    else
    {
       str_cpy_all(parentQidSystem_, attrValue, attrValueLen);
       parentQidSystem_[attrValueLen] = '\0';
    }
  }
  else
     parentQidSystem_[0] = '\0';
  updateDefaultsValueString(PARENT_QID_SYSTEM, parentQidSystem_);
}

void SessionDefaults::beginSession()
{
  setParentQid(NULL, 0);
  setParentQidSystem(NULL, 0);
  setWmsProcess(FALSE);
  setStatisticsViewType(SQLCLI_PERTABLE_STATS);
  setReclaimTotalMemorySize(DEFAULT_RECLAIM_MEMORY_AFTER);
  setReclaimFreeMemoryRatio(DEFAULT_RECLAIM_FREE_MEMORY_RATIO);
  setReclaimFreePFSRatio(DEFAULT_RECLAIM_FREE_PFS_RATIO);
  setRedriveCTAS(FALSE);
  setExSMTraceFilePrefix(NULL, 0);
}


void SessionDefaults::setExSMTraceFilePrefix(const char *attrValue,
                                           Int32 attrValueLen)
{
  if (exsmTraceFilePrefix_)
  {
    NADELETEBASICARRAY(exsmTraceFilePrefix_, heap_);
    exsmTraceFilePrefix_ = NULL;
  }

  if (attrValue == NULL || attrValue[0] == 0 || attrValueLen == 0)
    return;

  exsmTraceFilePrefix_ = new(heap_) char[attrValueLen + 1];
  strncpy(exsmTraceFilePrefix_, attrValue, attrValueLen);
  exsmTraceFilePrefix_[attrValueLen] = 0;
}
