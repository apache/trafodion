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
 * File:         spinfo.cpp
 * Description:  The SPInfo and SPList Classes
 *               Methods for SPInfo and SPList classes
 *
 *
 * Created:      01/01/2001
 * Language:     C++
 *
 *****************************************************************************
 */


#include "Platform.h"
#include "spinfo.h"
#include "UdrStreams.h"
#include "udrutil.h"
#include "str.h"
#include <time.h>
#include "UdrResultSet.h"
#include "LmRoutine.h"
#include "LmParameter.h"
#include "LmRoutineCSqlRowTM.h"
#include "sql_buffer.h"
#include "UdrResultSet.h"
#include "ex_queue.h"
#include "ComRtUtils.h"
#include "UdfDllInteraction.h"

#include "sqlcli.h"
#include "sqludr.h"
#include "udrdecs.h"


#include "dtm/tm.h"
 
extern void ServerDebug(const char *, ...);
extern void doMessageBox(UdrGlobals *UdrGlob, Int32 trLevel,
                         NABoolean moduleType, const char *moduleName);

extern void backoutTupps(SqlBuffer &b, Lng32 numTuppsBefore);

extern NABoolean allocateReplyRowAndEOD(UdrGlobals *UdrGlob,
                                SqlBuffer &replyBuffer,
                                queue_index parentIndex,
                                char *&replyData,
                                Int32 rowLen,
                                ControlInfo *&newControlInfo);
extern NABoolean allocateEODRow(UdrGlobals *UdrGlob,
                         SqlBuffer &replyBuffer,
                         queue_index parentIndex);

extern NABoolean convertReplyRowToErrorRow(SqlBuffer *sqlBuf,
                                    Lng32 numTuppsBefore,
                                    queue_index requestQueueIndex,
                                    UdrServerDataStream &msgStream,
                                    UdrGlobals *UdrGlob);

//**********************************************************
// SPInfo constructor & Destructor
//
//
//**********************************************************
SPInfo::SPInfo(UdrGlobals *udrGlobals,
               NAHeap *heapPtr,
               const UdrHandle &udrHandle,
               char *pSqlName,
               char *pExternalName,
               char *pRoutineSig,
               char *pContainerName,
               char *pExternalPathName,
               char *pLibrarySqlName,
               ComUInt32 pNumParams,
	       ComUInt32 pNumInParams,
	       ComUInt32 pNumOutParams,
               ComUInt32 pMaxRSets,
               ComRoutineTransactionAttributes ptransactionAttrs,
               ComRoutineSQLAccess psqlAccessMode,
	       ComRoutineLanguage pLanguage,
	       ComRoutineParamStyle pParamStyle,
	       NABoolean pIsolate,
	       NABoolean pCallOnNull,
	       NABoolean pExtraCall,
	       NABoolean pDeterministic,
               ComRoutineExternalSecurity pExternalSecurity,
               Int32     pRoutineOwnerId,
               ComUInt32 requestBufferSize,
               ComUInt32 replyBufferSize,
               ComUInt32 requestRowSize,
               ComUInt32 replyRowSize,
               ComDiagsArea &d,
               char *parentQid)
      : udrGlobals_(udrGlobals),
        udrHandle_(udrHandle),
        lmHandle_(FALSE),
#ifdef UDR_MULTIPLE_CONTEXTS
        cliContextHandle_(0),
#endif
        spInfoState_(INITIAL),
        transactionAttrs_(ptransactionAttrs),
        sqlAccessMode_(psqlAccessMode),
	numParameters_(pNumParams),
	numInParams_(pNumInParams),
  	numOutParams_(pNumOutParams),
	lmParameters_(NULL),
	returnValue_(NULL),
	sqlName_(NULL),
	externalName_(NULL),
	routineSig_(NULL),
	containerName_(NULL),
	externalPathName_(NULL),
	librarySqlName_(NULL),
	numCalls_(0),
        lastCallTs_(0),
	language_(pLanguage),
	paramStyle_(pParamStyle),
	isolate_(pIsolate),
	callOnNull_(pCallOnNull),
	extraCall_(pExtraCall),
	deterministic_(pDeterministic),
	externalSecurity_(pExternalSecurity),
	routineOwnerId_(pRoutineOwnerId),
	maxNumResultSets_(pMaxRSets),
	numResultSets_(0),
   	requestBufferSize_(requestBufferSize),
	replyBufferSize_(replyBufferSize),
	requestRowSize_(requestRowSize),
	replyRowSize_(replyRowSize),
        dataStream_(NULL),
        txStream_(NULL),
        currentRequest_(NULL),
        rsList_(heapPtr),
        udrHeapPtr_(heapPtr),
        numTableInfo_(0),
        tableInfo_(NULL),
        sqlBufferScalar_(NULL),
        sqlBufferTVF_(NULL),
        parentIndex_(0),
        parentQid_(NULL),
        rowDiags_(NULL)
{
  doMessageBox(udrGlobals_, TRACE_SHOW_DIALOGS,
               udrGlobals_->showSPInfo_, "SPInfo Constructor");


  str_cpy_all(&eyeCatcher_[0], EYE_SP, 4);
  udrGlobals_->getSPList()->addToSPList(this);
  lastCallTs_ = createUniqueIdentifier();

  if (pSqlName == NULL)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_ERROR);
    d << DgString0("Stored procedure SQL Name is NULL");
    // EH_THROW(EH_INTERNAL_EXCEPTION);
  }
  else
  {
    assignStringMember(sqlName_, pSqlName);
  }
  
  if (pExternalName == NULL)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_ERROR);
    d << DgString0("Stored procedure external name is NULL");
    // EH_THROW(EH_INTERNAL_EXCEPTION);
  }
  else
  {
    assignStringMember(externalName_, pExternalName);
  }
  
  if (pRoutineSig == NULL)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_ERROR);
    d << DgString0("Stored procedure Routine Signature is NULL");
    // EH_THROW(EH_INTERNAL_EXCEPTION);
  }
  else
  {
    assignStringMember(routineSig_, pRoutineSig);
  }
  
  if (pContainerName == NULL)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_ERROR);
    d << DgString0("Stored procedure Container Name is NULL");
    // EH_THROW(EH_INTERNAL_EXCEPTION);
  }
  else
  {
    assignStringMember(containerName_, pContainerName);
  }
  
  if (pExternalPathName == NULL)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_ERROR);
    d << DgString0("Stored procedure External Path Name is NULL");
    // EH_THROW(EH_INTERNAL_EXCEPTION);
  }
  else
  {
    assignStringMember(externalPathName_, pExternalPathName);
  }

  if (pLibrarySqlName == NULL)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_ERROR);
    d << DgString0("Stored procedure library SQL Name is NULL");
    // EH_THROW(EH_INTERNAL_EXCEPTION);
  }
  else
  {
    assignStringMember(librarySqlName_, pLibrarySqlName);
  }
  
  assignStringMember(parentQid_, parentQid);

  if (getNumParameters() > 0)
  {
    ComUInt32 lmParamBytes = getNumParameters() * sizeof(LmParameter);

    lmParameters_ = (LmParameter *) udrHeapPtr_->allocateMemory(lmParamBytes);
    memset(lmParameters_, 0, lmParamBytes);
  }

  // Now we create the data stream that will be used by this SPInfo
  // instance. All we need to do is call a constructor, but first there
  // is logic to compute data buffer sizes.
  //
  // We currently assume the following about data buffer sizes:
  // - On a given stream, request and reply buffers are the same size
  // - All message objects are aligned on 8-byte boundaries
  // - A data request consists of a UdrDataHeader followed by a
  //   UdrDataBuffer
  // - The sql_buffer size for the stream was sent to the server as
  //   part of the load request and is being stored inside this SPInfo
  //   object
  // - Data buffers contain a pad for diagnostics. The pad does not
  //   cause extra bytes to go on the wire. It allows data plus
  //   diags to exist in a single reply buffer which eliminates the
  //   need for the client to send a continue request to retrieve diags.
  //
  IpcMessageObjSize sqlBufSize = 0;
  IpcMessageObjSize maxBufSize = 0;

  sqlBufSize = MAXOF (
    getRequestBufferSize(),
    getReplyBufferSize()
    );

  maxBufSize += sizeof(UdrDataHeader);
  IpcMessageBuffer::alignOffset(maxBufSize);

  maxBufSize += sizeof(UdrDataBuffer);
  IpcMessageBuffer::alignOffset(maxBufSize);

  maxBufSize += sqlBufSize;
  IpcMessageBuffer::alignOffset(maxBufSize);

  Lng32 diagsPad = 1000;

#ifdef _DEBUG
  //
  // In a debug build we can get the pad size from the Java system
  // property UDR_BUFFER_PAD.
  //
  Lng32 tempPad = 0;
  if (udrGlobals_ && udrGlobals_->getJavaLM())
  {
    if (getLmProperty(*(udrGlobals_->getJavaLM()),
                      "UDR_BUFFER_PAD", tempPad, NULL))
    {
      diagsPad = tempPad;
    }
  }
#endif // _DEBUG

  maxBufSize += diagsPad;
  IpcMessageBuffer::alignOffset(maxBufSize);

  //
  // Normally all data requests and replies involve a single request
  // buffer and a single output buffer. If diags do not fit into a
  // data reply buffer then a second reply buffer is created and is
  // returned to the client as the response to a continue request. To
  // allow for continue requests/responses our server-side data streams
  // can have up to 2 send buffers.
  //
  dataStream_ = new (udrHeapPtr_) UdrServerDataStream (
    udrGlobals_->getIpcEnvironment(),
    2,           // long sendBufferLimit
    1,           // long inUseBufferLimit
    maxBufSize,  // IpcMessageObjSize bufferSize
    udrGlobals_,
    this
    );

  txStream_ = new (udrHeapPtr_) UdrServerReplyStream(
    udrGlobals_->getIpcEnvironment(),
    udrGlobals_,
    UDR_STREAM_SERVER_REPLY,
    UdrServerReplyStreamVersionNumber); 

} // SPInfo::SPInfo()

SPInfo::SPInfo(UdrGlobals *udrGlobals,
               NAHeap *heapPtr,
               const UdrHandle &udrHandle)
      : udrGlobals_(udrGlobals),
        udrHandle_(udrHandle),
        lmHandle_(FALSE),
#ifdef UDR_MULTIPLE_CONTEXTS
        cliContextHandle_(0),
#endif
        spInfoState_(INITIAL),
        transactionAttrs_(COM_UNKNOWN_ROUTINE_TRANSACTION_ATTRIBUTE),
        sqlAccessMode_(COM_UNKNOWN_ROUTINE_SQL_ACCESS),
	numParameters_(0),
	numInParams_(0),
  	numOutParams_(0),
	lmParameters_(NULL),
	returnValue_(NULL),
	sqlName_(NULL),
	externalName_(NULL),
	routineSig_(NULL),
	containerName_(NULL),
	externalPathName_(NULL),
	librarySqlName_(NULL),
	numCalls_(0),
        lastCallTs_(0),
	language_(COM_UNKNOWN_ROUTINE_LANGUAGE),
	paramStyle_(COM_UNKNOWN_ROUTINE_PARAM_STYLE),
	isolate_(FALSE),
	callOnNull_(FALSE),
	extraCall_(FALSE),
	deterministic_(FALSE),
	externalSecurity_(COM_ROUTINE_EXTERNAL_SECURITY_INVOKER),
	routineOwnerId_(0),
	maxNumResultSets_(0),
	numResultSets_(0),
   	requestBufferSize_(0),
	replyBufferSize_(0),
	requestRowSize_(0),
	replyRowSize_(0),
        dataStream_(NULL),
        txStream_(NULL),
        currentRequest_(NULL),
        rsList_(heapPtr),
        udrHeapPtr_(heapPtr),
        numTableInfo_(0),
        tableInfo_(NULL),
        sqlBufferScalar_(NULL),
        sqlBufferTVF_(NULL),
        parentIndex_(0),
        parentQid_(NULL),
        rowDiags_(NULL)
{
  str_cpy_all(&eyeCatcher_[0], EYE_SP , 4);
  lastCallTs_ = createUniqueIdentifier();
  udrGlobals_->getSPList()->addToSPList(this);
}

SPInfo::~SPInfo()
{
  udrGlobals_->getSPList()->removeFromSPList(this);

  if (dataStream_) delete dataStream_;

  if (txStream_) delete txStream_;

  //
  // Note: we do not bother checking for NULL inputs to the delete and
  // deallocate calls below because they are no-ops when the input is
  // NULL.
  //

  if (udrHeapPtr_ == NULL)
  {
    delete [] sqlName_;
    delete [] externalName_;
    delete [] routineSig_;
    delete [] containerName_;
    delete [] externalPathName_;
    delete [] librarySqlName_;
    if (parentQid_)
      delete [] parentQid_;
  }
  else
  {
    udrHeapPtr_->deallocateMemory(sqlName_);
    udrHeapPtr_->deallocateMemory(externalName_);
    udrHeapPtr_->deallocateMemory(routineSig_);
    udrHeapPtr_->deallocateMemory(containerName_);
    udrHeapPtr_->deallocateMemory(externalPathName_);
    udrHeapPtr_->deallocateMemory(librarySqlName_);
    if (parentQid_)
      udrHeapPtr_->deallocateMemory(parentQid_);

    // Deallocate the LmParameter array
    if (lmParameters_)
    {
      for (ComUInt32 i = 0; i < numParameters_; i++)
      {
        LmParameter &p = lmParameters_[i];
        p.freeResources();
      }
      udrHeapPtr_->deallocateMemory(lmParameters_);
    }

    //Deallocate tableInfo
    if(tableInfo_)
    {
      for(ComUInt32 i = 0; i < numTableInfo_; i++)
      {
        tableInfo_[i].freeResources(udrHeapPtr_);
      }
      udrHeapPtr_->deallocateMemory(tableInfo_);
    }
  }

  if (rowDiags_)
    rowDiags_->decrRefCount();
  
} // SPInfo::~SPInfo()

void SPInfo::assignStringMember(
		char *&memberBuff,
		const char *const src)
{
	if (udrHeapPtr_ != NULL)
	{
		udrHeapPtr_->deallocateMemory(memberBuff);
		memberBuff = NULL;
	}
	else
	{
		delete [] memberBuff;
		memberBuff = NULL;
	}

	// if src is non-NULL then we need to create a buff, copy it over...
	if (src != NULL)
	{
          UInt32 buffsize = str_len(src) + 1;
          if (udrHeapPtr_ != NULL)
          {
            memberBuff = (char *) (udrHeapPtr_->allocateMemory(buffsize));
          }
          else
          {
            memberBuff = new char[buffsize];
          }
          
          UDR_ASSERT(memberBuff != NULL, "Out of Memory");
          str_cpy(memberBuff, src, (Lng32) buffsize);
          memberBuff[buffsize-1] = '\0';
	}
        
}  // assignStringMember

void SPInfo::setInParam(ComUInt32 i, const UdrParameterInfo &info)
{
  UDR_ASSERT(numInParams_ > i,
             "An invalid index was passed to SPInfo::setInParam()");
  initLmParameter(info, TRUE);
}

void SPInfo::setOutParam(ComUInt32 i,
                         const UdrParameterInfo &info)
{
  UDR_ASSERT(numOutParams_ > i,
             "An invalid index was passed to SPInfo::setOutParam()");
  initLmParameter(info, FALSE);
}

// setUdrContext
NABoolean SPInfo::setUdrContext(ComDiagsArea &d) const
{

  return TRUE;

} // setUdrContext

// Set corresponding LmParameter using given UdrParamterInfo object
void SPInfo::initLmParameter(const UdrParameterInfo &pInfo,
                             NABoolean isInput)
{
  LmParameter &lmp = getLmParameter(pInfo.getPosition());

  // Cases to consider
  //
  // a. This is an IN parameter. isInput will be TRUE. All LmParameter
  // fields must be initialized.
  // 
  // b. This is an OUT parameter. isInput will be FALSE. All
  // LmParameter fields must be initialized.
  // 
  // c. This is an INOUT parameter and isInput is TRUE. All
  // LmParameter fields must be initialized.
  //
  // d. This is an INOUT parameter and isInput is FALSE. We assume the
  // LmParameter instance is partially initialized because this method
  // was already called once for the same LmParameter with isInput set
  // to TRUE. Now we only need to initialze the output offset and
  // length fields of the LmParameter.

  // First, check for case d
  if (pInfo.isInOut() && isInput == FALSE)
  {
    lmp.setOutDataInfo(pInfo.getDataOffset(),
                       pInfo.getDataLength(),
                       pInfo.getNullIndicatorOffset(),
                       pInfo.getNullIndicatorLength(),
                       pInfo.getVCLenIndOffset(),
                       pInfo.getVCIndicatorLength());
  }

  else
  {
    // Cases a, b, and c. We need to initialize all fields of the
    // LmParameter.
    
    ComColumnDirection direction;
    if (pInfo.isInOut())
      direction = COM_INOUT_COLUMN;
    else if (pInfo.isIn())
      direction = COM_INPUT_COLUMN;
    else
      direction = COM_OUTPUT_COLUMN;
    
    // Determine the encoding charset. We will use =_SQL_MX_ISO_MAPPING
    // define value if the param's charset is ISO88591.
    CharInfo::CharSet charset = (CharInfo::CharSet) pInfo.getEncodingCharSet();
    if (charset == CharInfo::ISO88591)
      charset = udrGlobals_->getIsoMapping();
    
    if (isInput)
    {
      lmp.init((ComFSDataType) pInfo.getFSType(),
               pInfo.getPrec(),
               pInfo.getScale(),
               charset,
               (CharInfo::Collation) pInfo.getCollation(),
               direction,
               pInfo.isLmObjType(),
               RS_NONE,
               pInfo.getDataOffset(),
               pInfo.getDataLength(),
               pInfo.getNullIndicatorOffset(),
               pInfo.getNullIndicatorLength(),
               pInfo.getVCLenIndOffset(),
               pInfo.getVCIndicatorLength(),
               0, 0, 0, 0, 0, 0, // output offsets and lengths
               pInfo.getParamName());
    }
    else
    {
      lmp.init((ComFSDataType) pInfo.getFSType(),
               pInfo.getPrec(),
               pInfo.getScale(),
               charset,
               (CharInfo::Collation) pInfo.getCollation(),
               direction,
               pInfo.isLmObjType(),
               RS_NONE,
               0, 0, 0, 0, 0, 0, // input offsets and lengths
               pInfo.getDataOffset(),
               pInfo.getDataLength(),
               pInfo.getNullIndicatorOffset(),
               pInfo.getNullIndicatorLength(),
               pInfo.getVCLenIndOffset(),
               pInfo.getVCIndicatorLength(),
               pInfo.getParamName());
    }

  } // Cases a, b, and c
}

void SPInfo::resetLastCallTs()
{
  lastCallTs_ = createUniqueIdentifier();
  numCalls_++;
  numResultSets_ = 0;
}

// displaySPInfo - Used when tracing objects
void SPInfo::displaySPInfo(Lng32 indent)
{
  char ind[100];

  Lng32 indmax = (indent > 99) ? 99 : indent;

  Lng32 indIdx = 0;
  for (indIdx = 0; indIdx < indmax; indIdx++)
	  ind[indIdx] = ' ';
  ind[indIdx] = '\0';

  ServerDebug("%sContents of SPInfo:", ind );
  ServerDebug("%s---------------------", ind );
  ServerDebug("%sEye Catcher         : %s", ind, &eyeCatcher_ );
  ServerDebug("%sUdr Handle          : " INT64_SPEC , ind, udrHandle_ );
  if (lmHandle_ == NULL)
      ServerDebug("%sLM Handle           : NULL", ind);
  else
      ServerDebug("%sLM Handle           : %p", ind, lmHandle_ );

#ifdef UDR_MULTIPLE_CONTEXTS
  ServerDebug("%sCLI Context Handle  : %d", ind, (Lng32) cliContextHandle_ );
#endif // UDR_MULTIPLE_CONTEXTS
  ServerDebug("%sSPInfoState         : %i", ind, (Int32) spInfoState_ );  
  ServerDebug("%sTransaction Required     : %i", ind, (Int32) transactionAttrs_ );
  ServerDebug("%sSQL Access Mode     : %i", ind, (Int32) sqlAccessMode_ );
  ServerDebug("%sNumber of Params    : %u", ind, numParameters_ );
  ServerDebug("%sNumber of In Params : %u", ind, numInParams_ );
  ServerDebug("%sNumber of Out Params: %u", ind, numOutParams_ );

  ServerDebug("%sSQL Name            : %s", ind, sqlName_ );
  ServerDebug("%sExternal Name       : %s", ind, externalName_ );
  ServerDebug("%sRoutine SQL         : %s", ind, routineSig_ );
  ServerDebug("%sContainer Name      : %s", ind, containerName_ );
  ServerDebug("%sExternal Path Name  : %s", ind, externalPathName_ );
  ServerDebug("%sLibrary SQL Name    : %s", ind, librarySqlName_ );

  ServerDebug("%sNumber of Invokes   : " INT64_SPEC , ind, numCalls_ );
  ServerDebug("%sLast Call Timestamp : " INT64_SPEC , ind, lastCallTs_ );

  const char *lang = "***Unknown***";
  switch (language_)
  {
    case COM_UNKNOWN_ROUTINE_LANGUAGE:
	  lang = "***Unknown***"; break;

    case COM_LANGUAGE_JAVA:
	  lang = "Java"; break;

    case COM_LANGUAGE_C:
	  lang = "C"; break;

    case COM_LANGUAGE_CPP:
	  lang = "C++"; break;

    case COM_LANGUAGE_SQL:
	  lang = "SQL"; break;

    default:
	  break;
  }
  ServerDebug("%sLanguage            : %s", ind, lang );

  const char *paramStyle = "UNKNOWN";
  switch (paramStyle_)
  {
    case COM_UNKNOWN_ROUTINE_PARAM_STYLE:
	  paramStyle = "UNKNOWN"; break;

    case COM_STYLE_GENERAL:
	  paramStyle = "GENERAL"; break;

    case COM_STYLE_JAVA_CALL:
	  paramStyle = "JAVA"; break;

    case COM_STYLE_JAVA_OBJ:
	  paramStyle = "JAVA_OBJ"; break;

    case COM_STYLE_SQL:
	  paramStyle = "SQL"; break;

    case COM_STYLE_SQLROW:
          paramStyle = "SQLROW"; break;

    case COM_STYLE_SQLROW_TM:
          paramStyle = "SQLROW_TM"; break;

    case COM_STYLE_CPP_OBJ:
          paramStyle = "C++"; break;

    default:
	  break;
  }
  ServerDebug("%sParameter Style      : %s", ind, paramStyle );

  ServerDebug("%sIsolate?            : %i", ind, (Int32) isolate_ );
  ServerDebug("%sCallOnNull?         : %i", ind, (Int32) callOnNull_ );
  ServerDebug("%sExtraCall?          : %i", ind, (Int32) extraCall_ );
  ServerDebug("%sDeterministic?      : %i", ind, (Int32) deterministic_ );

  const char *externalSecurity = "UNKNOWN";
  switch (externalSecurity_)
  {
    case COM_ROUTINE_EXTERNAL_SECURITY_INVOKER:
      externalSecurity = "EXTERNAL SECURITY INVOKER";
      break;

    case COM_ROUTINE_EXTERNAL_SECURITY_DEFINER:
      externalSecurity = "EXTERNAL SECURITY DEFINER";
      break;

    default:
      break;
  }
  ServerDebug("%sExternalSecurity    : %s", ind, externalSecurity );
  ServerDebug("%sRoutine Owner Id    : %u", ind, routineOwnerId_ );

  ServerDebug("%sMax RSets           : %u", ind, maxNumResultSets_ );
  ServerDebug("%sNum Current RSets   : %u", ind, numResultSets_ );

  ServerDebug("%sRequest Buffer Size : %u", ind, requestBufferSize_ );
  ServerDebug("%sReply Buffer Size   : %u", ind, replyBufferSize_ );

  ServerDebug("%sRequest Row Size    : %u", ind, requestRowSize_ );
  ServerDebug("%sReply Row Size      : %u", ind, replyRowSize_ );

  if (dataStream_ == NULL)
      ServerDebug("%sDataStreamPtr       : NULL", ind);
  else
      ServerDebug("%sDataStreamPtr       : %p", ind, dataStream_ );

  if (udrHeapPtr_ == NULL)
      ServerDebug("%sHeapPtr             : NULL", ind);
  else
      ServerDebug("%sHeapPtr             : %p", ind, udrHeapPtr_ );

  ServerDebug("");

} // SPInfo::displaySPInfo

void SPInfo::displaySPInfoId(Lng32 indent)
{
  char ind[100];

  Lng32 indmax = (indent > 99) ? 99 : indent;
  
  Lng32 indIdx = 0;
  for (indIdx = 0; indIdx < indmax; indIdx++)
    ind[indIdx] = ' ';
  ind[indIdx] = '\0';
  
  ServerDebug("%sObject ID         : " INT64_SPEC , ind, udrHandle_ );

} // SPInfo::displaySPInfoId

// createUniqueIdentifier
Int64 SPInfo::createUniqueIdentifier()
{
  Int64 result;
  //
  // During R1.6 testing it was discovered that JULIANTIMESTAMP was
  // not always returning unique values on Windows. We believe this is
  // due to occasional clock synchronization performed by the NonStop
  // Cluster services. To avoid the duplicate timestamp problem we use
  // a sequential counter to generate process-wide unique identifiers.
  //
  result = udrGlobals_->nextUniqueIdentifier_++;
  return result;
}


// releaseSP
Lng32 SPInfo::releaseSP(NABoolean reportErrors,
                       ComDiagsArea &d)
{
  const char *moduleName = "SPInfo releaseSP";
  char errorText[MAXERRTEXT];
  LmResult lmResult = LM_OK;
  LmRoutine *lmr;

  doMessageBox(udrGlobals_, TRACE_SHOW_DIALOGS,
               udrGlobals_->showSPInfo_, moduleName);

  // Deallocate UdrResultSet objects
  for (ComUInt32 index = 0; index < rsList_.entries(); index++)
  {
    UdrResultSet *rs = rsList_[index];
    NADELETE(rs, UdrResultSet, udrHeapPtr_);
  }

  rsList_.clear();

  // drop LM Handle...
  lmr = this->getLMHandle();
  if (lmr != NULL)
  {
    lmResult = udrGlobals_->getLM(getLanguage())->putRoutine(lmr, &d);

    if (lmResult == LM_ERR)
    {
      if (reportErrors)
      {
        if (udrGlobals_->verbose_ &&
            udrGlobals_->traceLevel_ >= TRACE_DATA_AREAS &&
            udrGlobals_->showSPInfo_)
        {

          sprintf(errorText,
                  "[UdrServ (%.30s)]  LM putRoutine call resulted in error.",
                  moduleName);
          ServerDebug(errorText);
        }
      }
      udrGlobals_->numErrLMCall_++;
    }
  } // if (lmr != NULL)

  setLMHandle(NULL);
  delete this;
  return 0;
    
} // SPInfo::releaseSP

const char *SPInfo::getSPInfoStateString() const
{
  switch (spInfoState_)
  {
    case INITIAL:                 return "INITIAL";
    case LOADED:                  return "LOADED";
    case LOAD_FAILED:             return "LOAD_FAILED";
    case INVOKED:                 return "INVOKED";
    case INVOKE_FAILED:           return "INVOKE_FAILED";
    case INVOKED_EMITROWS:        return "INVOKED_EMITROWS";
    case INVOKED_GETROWS:         return "INVOKED_GETROWS";
    case INVOKED_GETROWS_FAILED:  return "INVOKED_GETROWS_FAILED";
    case UNLOADING:               return "UNLOADING";
    default:
      return ComRtGetUnknownString((Int32) spInfoState_);
  }
}

// (Re)Initializes rsList_ field after a udr is invoked.
NABoolean SPInfo::setupUdrResultSets(ComDiagsArea &d)
{
  ComUInt32 numRS = lmHandle_->getNumResultSets();

  // If UDR invocation produced no resultsets, just return
  if ( numRS <= 0)
    return TRUE;

  for (ComUInt32 index = 0; index < numRS; index++)
  {
    LmResultSet *lmRS = lmHandle_->getLmResultSet(index);

    if (rsList_.entries() < index+1)
    {
      // We do not have a UdrResultSet object at 'index' location
      // So create one & insert in rsList_.
      UdrResultSet *udrRS = new (udrHeapPtr_) UdrResultSet(this, lmRS, d);
      if (d.getNumber(DgSqlCode::ERROR_) > 0)
      {
        delete udrRS;
        return FALSE;
      }

      rsList_.insert(udrRS);
    }
    else
    {
      // A UdrResultSet object exists at 'index' location, lets update it
      if (rsList_[index]->reInit(lmRS, d) != 0)
        return FALSE;
    }
  }


  setNumResultSets(rsList_.entries());
  return TRUE;
}

SQLSTMT_ID *SPInfo::executeSqlStmt(const char *sql_str, ComDiagsArea &d)
{
  Lng32 retcode = 0;

  retcode = SQL_EXEC_ClearDiagnostics(NULL);
  if (retcode != 0)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
      << DgString0("SQL_EXEC_ClearDiagnostics")
      << DgInt0(retcode);

    return NULL;
  }

  // Allocate a SQL statement
  SQLMODULE_ID *module = new (udrHeapPtr_) SQLMODULE_ID;
  memset(module, 0, sizeof(SQLMODULE_ID));
  module->module_name = 0;

  SQLSTMT_ID *stmt = new (udrHeapPtr_) SQLSTMT_ID;
  memset(stmt, 0, sizeof(SQLSTMT_ID));
  stmt->name_mode = stmt_handle;
  stmt->module = module;
  stmt->identifier = 0;
  stmt->handle = 0;

  retcode = SQL_EXEC_AllocStmt(stmt, 0);
  if (retcode != 0)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
      << DgString0("SQL_EXEC_AllocStmt")
      << DgInt0(retcode);

    udrHeapPtr_->deallocateMemory(module);
    udrHeapPtr_->deallocateMemory(stmt);
    return NULL;
  }

  // Allocate a descriptor to hold the sql statement source
  SQLMODULE_ID descmodule;
  memset(&descmodule, 0, sizeof(SQLMODULE_ID));
  descmodule.module_name = 0;

  SQLDESC_ID sqlsrc_desc;
  memset(&sqlsrc_desc, 0, sizeof(SQLDESC_ID));
  sqlsrc_desc.name_mode = desc_handle;
  sqlsrc_desc.module = &descmodule;
  sqlsrc_desc.identifier = 0;
  sqlsrc_desc.handle = 0;
  retcode = SQL_EXEC_AllocDesc(&sqlsrc_desc, 1);
  if (retcode != 0)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
      << DgString0("SQL_EXEC_AllocDesc")
      << DgInt0(retcode);

    SQL_EXEC_DeallocStmt(stmt);

    udrHeapPtr_->deallocateMemory(module);
    udrHeapPtr_->deallocateMemory(stmt);
    return NULL;
  }

  SQLDESC_ITEM desc_items[3];

  desc_items[0].item_id = SQLDESC_TYPE;
  desc_items[0].entry = 1;
  desc_items[0].num_val_or_len = SQLTYPECODE_VARCHAR;

  desc_items[1].item_id = SQLDESC_VAR_PTR;
  desc_items[1].entry = 1;
  desc_items[1].num_val_or_len = (Long) sql_str;

  desc_items[2].item_id = SQLDESC_LENGTH;
  desc_items[2].entry = 1;
  desc_items[2].num_val_or_len = (Lng32) strlen(sql_str) + 1;

  retcode = SQL_EXEC_SetDescItems2(&sqlsrc_desc, 3, desc_items);
  if (retcode != 0)
  {
    d << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
      << DgString0("SQL_EXEC_SetDescItem2")
      << DgInt0(retcode);

    SQL_EXEC_DeallocDesc(&sqlsrc_desc);
    SQL_EXEC_DeallocStmt(stmt);

    udrHeapPtr_->deallocateMemory(module);
    udrHeapPtr_->deallocateMemory(stmt);
    return NULL;
  }

  // Prepare the statement; stmt has the prepared plan
  retcode = SQL_EXEC_Prepare(stmt, &sqlsrc_desc);
  if (retcode != 0)
  {
    SQL_EXEC_MergeDiagnostics_Internal(d);

    d << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
      << DgString0("SQL_EXEC_Prepare")
      << DgInt0(retcode);

    SQL_EXEC_DeallocDesc(&sqlsrc_desc);
    SQL_EXEC_DeallocStmt(stmt);

    udrHeapPtr_->deallocateMemory(module);
    udrHeapPtr_->deallocateMemory(stmt);
    return NULL;
  }

  // Execute the statement
  retcode = SQL_EXEC_ExecClose(stmt, 0, 0, 0);
  if (retcode != 0)
  {
    SQL_EXEC_MergeDiagnostics_Internal(d);

    d << DgSqlCode(-UDR_ERR_INTERNAL_CLI_ERROR)
      << DgString0("SQL_EXEC_Exec")
      << DgInt0(retcode);

    SQL_EXEC_DeallocDesc(&sqlsrc_desc);
    SQL_EXEC_DeallocStmt(stmt);

    udrHeapPtr_->deallocateMemory(module);
    udrHeapPtr_->deallocateMemory(stmt);
    return NULL;
  }

  SQL_EXEC_DeallocDesc(&sqlsrc_desc);

  return stmt;
}

// loadUdrResultSet() loads a UdrResultSet object at given location.
// When a UdrResultSet is loaded, it will have all the column information
// (got from executor) filled in.
void SPInfo::loadUdrResultSet(ComUInt32 index,
                              RSHandle handle,
                              ComUInt32 numRSCols,
                              ComUInt32 rowSize,
                              ComUInt32 bufferSize,
                              UdrParameterInfo *columnDesc,
                              ComDiagsArea &d)
{
  const char *moduleName = "SPInfo::loadUdrResultSet";
  UdrResultSet *rs = rsList_[index];

  if (udrGlobals_->verbose_ &&
      udrGlobals_->traceLevel_ >= TRACE_DETAILS &&
      udrGlobals_->showRSLoad_)
    ServerDebug("[Udrserv (%s)] Loading ResultSet at position %d",
                moduleName, index + 1);

  NABoolean result =
    rs->load(handle, numRSCols, rowSize, bufferSize, columnDesc, d);

  if (udrGlobals_->verbose_ &&
      udrGlobals_->traceLevel_ >= TRACE_DETAILS &&
      udrGlobals_->showRSLoad_)
  ServerDebug("[Udrserv (%s)] ResultSet load %s",
              moduleName, (result) ? "succeded" : "failed");

  return ;
}

UdrResultSet *SPInfo::getUdrResultSetByHandle(RSHandle handle)
{
  UdrResultSet *rs = NULL;

  ComUInt32 entries = rsList_.entries();
  for (ComUInt32 i = 0; i < entries; i++)
  {
    rs = rsList_.at(i);
    if (rs != NULL && rs->getRSHandle() == handle)
      return rs;
  }

  return rs;
}

void SPInfo::prepareForReinvoke(ComDiagsArea *diags)
{
  ComUInt32 index;

  // Reset UdrResultSet object on rsList_ for UDR reinvocation.
  // This will reset all the fields that are generated by udrserv.
  for (index = 0; index < rsList_.entries(); index++)
  {
    UdrResultSet *rs = rsList_[index];
    rs->prepareForReinvoke();
  }

  // Let LM cleanup any resources.
  lmHandle_->cleanupResultSets(diags);

}

// If there is an active Enter Tx request for this SP, activate
// it's transaction
NABoolean SPInfo::activateTransaction()
{
  const char *moduleName = "SPInfo::activateTransaction";
  NABoolean result = FALSE;

#ifdef UDR_DEBUG
  if (udrGlobals_->verbose_)
  {
    Int64 tx_id = 0;
    short tmfRetcode = GETTRANSID((short*) &tx_id);

    ServerDebug("[UdrServ (%s)] Activating transaction...", moduleName);
    ServerDebug("         Before setting transaction...");
    ServerDebug("           GETTRANSID returned %d. Transid is %Ld.",
                 tmfRetcode, tx_id);
  }
#endif

  if (txStream_->getState() == IpcMessageStream::RECEIVED)
  {
    txStream_->activateCurrentMsgTransaction();
    result = TRUE;
  }

#ifdef UDR_DEBUG
  if (udrGlobals_->verbose_)
  {
    Int64 tx_id = 0;
    short tmfRetcode = GETTRANSID((short*) &tx_id);

    ServerDebug("         After setting transaction...");
    ServerDebug("           GETTRANSID returned %d. Transid is %Ld.",
                 tmfRetcode, tx_id);

    ServerDebug("[UdrServ (%s)] Done", moduleName);
  }
#endif

  return result;

} // SPInfo::activateTransaction()

void SPInfo::replyToEnterTxMsg(NABoolean doneWithRS)
{
  const char *moduleName = "SPInfo::replyToEnterTxMsg";

  NABoolean doTrace =
    (udrGlobals_->verbose_ && udrGlobals_->traceLevel_ >= TRACE_IPMS) ?
    TRUE : FALSE;

  if (doTrace)
  {
    ServerDebug("[UdrServ (%s)] Sending ENTER TX Reply", moduleName);
  }

  // Restore the transaction associated with Enter Tx message and
  // quiesce executor if needed.
  activateTransaction();
  quiesceExecutor();

  // Reply to the Enter Tx message
  UdrEnterTxReply *txreply = new (udrHeapPtr_) UdrEnterTxReply(udrHeapPtr_);
  txreply->setHandle(udrHandle_);

  txStream_->clearAllObjects();
  *txStream_ << *txreply;

  NABoolean waited = TRUE;
  txStream_->send(waited);

  if (doTrace)
  {
    ServerDebug("[UdrServ (%s)] Sent ENTER TX Reply", moduleName);
  }

  txreply->decrRefCount();

  if (doneWithRS)
  {
    // If we are not going to use RS of this SPInfo, cleanup
    // RS resources. diags will be ignored since anyway we are closing
    // the statement.
    ComDiagsArea *diags = ComDiagsArea::allocate(udrGlobals_->getIpcHeap());
    prepareForReinvoke(diags);
    diags->decrRefCount();
  }

} // SPInfo::replyToEnterTxMsg()

void SPInfo::prepareToReply(UdrServerReplyStream &msgStream)
{
  const char *moduleName = "SPInfo::prepareToReply";
  NABoolean doTrace =
    (udrGlobals_->verbose_ && udrGlobals_->traceLevel_ >= TRACE_IPMS) ?
    TRUE : FALSE;

  if (doTrace)
    ServerDebug("[UdrServ (%s)] Preparing to reply", moduleName);

  // Check if we have to Quiesce the executor.
  // If the message came with transaciton, we may have to quiesce
  // the executor.

  // If we do have a request waiting in txStream_ then, we are sure
  // that the message did not carry transaction. 
  if (txStream_->getState() == IpcMessageStream::RECEIVED)
  {
    // We are under an Enter Tx message, no need to quiesce.
    if (doTrace)
      ServerDebug("         Quiesce is not needed for the current reply.");
  }
  else
  {
    // We are not under Enter Tx message, so we have to quiesce
    // executor if necessary
    msgStream.activateCurrentMsgTransaction();
    quiesceExecutor();
  }

  if (doTrace)
    ServerDebug("[UdrServ (%s)] Done.", moduleName);

} // SPInfo::prepareToReply()

void SPInfo::prepareToReply(UdrServerDataStream &msgStream)
{
  const char *moduleName = "SPInfo::prepareToReply";
  NABoolean doTrace =
    (udrGlobals_->verbose_ && udrGlobals_->traceLevel_ >= TRACE_IPMS) ?
    TRUE : FALSE;

  if (doTrace)
    ServerDebug("[UdrServ (%s)] Preparing to reply", moduleName);

  // Check if we have to Quiesce the executor.

  // If we do have a request waiting in txStream_ then, we are sure
  // that the message did not carry transaction. 
  if (txStream_->getState() == IpcMessageStream::RECEIVED)
  {
    // We are under an Enter Tx message, no need to quiesce.
    if (doTrace)
      ServerDebug("         Quiesce is not needed for the current reply.");
  }
  else
  {
    // We are not under Enter Tx message, so we have to quiesce
    // executor if needed.
    msgStream.activateCurrentMsgTransaction(); 
    quiesceExecutor();
  }

  if (doTrace)
    ServerDebug("[UdrServ (%s)] Done.", moduleName);

} // SPInfo::prepareToReply()

void SPInfo::quiesceExecutor()
{
  NABoolean doTrace =
    (udrGlobals_->verbose_ && udrGlobals_->traceLevel_ >= TRACE_IPMS) ?
    TRUE : FALSE;

  // With UDFs, the messages do not carry transaction id. Just return
  // from here without even checking if transaction exists.
  // $$$$ It would be better to check the routine type here rather
  // than the parameter style
  if (getParamStyle() == COM_STYLE_SQLROW ||
      getParamStyle() == COM_STYLE_SQL)
  {
    if (doTrace)
      ServerDebug( "         Quiescing is not needed for UDFs.");

    return ;
  }

  // With SPJs, we don't do any SQL in UDR Server process on NEO since
  // we use Type 4 JDBC. But SQL execution is possible in UDR Server on
  // Windows platform.
  if (getParamStyle() == COM_STYLE_JAVA_CALL)
  {
    Int64 tx_id = 0;
    short tmfCode = GETTRANSID((short*) &tx_id);
    
    if (doTrace)
    {
      ServerDebug("         GetTransID returned %d.", tmfCode);
      ServerDebug("         Current Transid is %Ld.", tx_id);
    }
    
    // Quiesce the executor if the message carried a transaction
    if (tmfCode == 0)
    {
      if (doTrace)
        ServerDebug("         "
                    "Message carried a transaction. About to quiesce.");
      
      Lng32 sqlcode = SQL_EXEC_Xact(SQLTRANS_QUIESCE, NULL);
      if (sqlcode != 0)
      {
        char msg[MAXERRTEXT];
        str_sprintf(msg, "SQL_EXEC_Xact returned error %d", (Int32) sqlcode);
        UDR_ASSERT(FALSE, msg);
      }
      
      if (doTrace)
        ServerDebug("         Quiesce was successful");
    }
    else if (tmfCode == 75 || tmfCode == 78)
    {
      if (doTrace)
        ServerDebug("         "
                    "Message did not have a transaction. "
                    "Quiescing is not neeeded.");
    }
    else
    {
      char msg[MAXERRTEXT];
      str_sprintf(msg,
                  "Unable to get transaction state. TMF returned error %d",
                  tmfCode);
      if (doTrace)
      {
        ServerDebug(msg);
        ServerDebug("Aborting...");
      }
      
      UDR_ASSERT(FALSE, msg);
    }
  } // if COM_STYLE_JAVA

} // SPInfo::quiesceExecutor()

void SPInfo::work()
{
  const char *moduleName = "SPInfo::work";

  NABoolean traceInvokeIPMS = false;
  if (udrGlobals_->verbose_ && udrGlobals_->showInvoke_ &&
      udrGlobals_->traceLevel_ >= TRACE_IPMS)
    traceInvokeIPMS = true;

  NABoolean traceInvokeDataAreas = false;
  if (udrGlobals_->verbose_ && udrGlobals_->showInvoke_ &&
      udrGlobals_->traceLevel_ >= TRACE_DATA_AREAS)
    traceInvokeDataAreas = true;

  doMessageBox(udrGlobals_,
               TRACE_SHOW_DIALOGS,
               udrGlobals_->showInvoke_,
               moduleName);

  // Do we have a request to process?
  if (! currentRequest_)
  {
    if (traceInvokeIPMS)
      ServerDebug("[UdrServ (%s)]  No current request buffer to process",
                  moduleName);

    return;
  }

  if (traceInvokeIPMS)
    ServerDebug("[UdrServ (%s)]  Processing Invoke Request", moduleName);

  // Free up any receive buffers no longer in use
  dataStream_->cleanupBuffers();

  // Extract the SqlBuffer from the request
  SqlBuffer *reqSqlBuf = currentRequest_->getSqlBuffer();
  if (reqSqlBuf == NULL)
  {
    if (traceInvokeIPMS)
      ServerDebug("[UdrServ (%s)]  Empty request buffer in the request",
              moduleName);

    dataErrorReply(udrGlobals_, *dataStream_, UDR_ERR_MESSAGE_PROCESSING,
                   INVOKE_ERR_NO_REQUEST_BUFFER, NULL);

    sendDataReply(udrGlobals_, *dataStream_, NULL);
    currentRequest_ = NULL;
    return;
  }

  if (reqSqlBuf->atEOTD())
  {
    // This is the case where the rows in the request buffer
    // are all processed in the previous iteration of work()
    // and reply is sent to client but end of tupps is not seen
    // in request buffer.
    // In this case we just have to do some cleanup and return.

    if (traceInvokeIPMS)
      ServerDebug("[UdrServ (%s)]  No request rows to process in the"
                  " request SQL buffer", moduleName);

    reqSqlBuf->bufferFull();
    currentRequest_ = NULL;
    return ;
  }

  ComUInt32 initialNumRequestTupps = reqSqlBuf->getTotalTuppDescs();
  if (traceInvokeIPMS)
    ServerDebug("[UdrServ (%s)]  There are %d tupps in the request buffer",
               moduleName, initialNumRequestTupps);

  // Does stream have room for next buffer?
  if (dataStream_->sendLimitReached())
  {
    if (traceInvokeIPMS)
      ServerDebug("[UdrServ (%s)]  Send limit reached in the data stream",
                  moduleName);
    return;
  }

  // Allocate a reply buffer
  UdrDataBuffer *reply = new (*dataStream_, getReplyBufferSize())
    UdrDataBuffer(getReplyBufferSize(), UdrDataBuffer::UDR_DATA_OUT, NULL);
  
  if (reply == NULL || reply->getSqlBuffer() == NULL)
  {
    dataErrorReply(udrGlobals_, *dataStream_, UDR_ERR_MESSAGE_PROCESSING,
                   INVOKE_ERR_NO_REPLY_DATA_BUFFER, NULL);

    if (traceInvokeIPMS)
      ServerDebug("[UdrServ (%s)] Send Invoke Error Reply", moduleName);

    sendDataReply(udrGlobals_, *dataStream_, NULL);

    // We will not need the request buffer anymore. Mark as such so
    // the stream deletes this buffer later.
    reqSqlBuf->bufferFull();
    currentRequest_ = NULL;

    return;
  }

  // Activate Enter TX transaction if we have already seen an
  // Enter Tx msg. Otherwise, activate this INVOKE msg's transaction
  if (! activateTransaction())
    dataStream_->activateCurrentMsgTransaction();

  // Prepare for re-invocation on the same SP
  if (isInvoked() || isInvokeFailed())
  {
    // !FIXME: WHAT TO DO ABOUT THESE DIAGS
    //prepareForReinvoke(diags);
    prepareForReinvoke(NULL);
  }

  resetLastCallTs();

  // Process request rows until all rows are processed or ther is no
  // room left in reply buffer
  Lng32 numRowsProcessed = 0;
  RequestRowProcessingStatus status;
  do
  {
    status = processOneRequestRow(reqSqlBuf,
                                  reply->getSqlBuffer(),
                                  numRowsProcessed);

  } while (status != ALL_ROWS_PROCESSED && status != REPLY_BUFFER_FULL);

  // Print reply buffer
  if (traceInvokeDataAreas)
  {
    ServerDebug("");
    ServerDebug("[UdrServ (%s)] Processed %d rows from the request buffer.",
      moduleName, numRowsProcessed);
    ServerDebug("[UdrServ (%s)] %d tupps remaining in request buffer.",
      moduleName, initialNumRequestTupps - reqSqlBuf->getProcessedTuppDescs());
    ServerDebug("[UdrServ (%s)] Reply Row Length %d", moduleName,
      (Lng32) getReplyRowSize());
    ServerDebug("[UdrServ (%s)] Invoke Reply SQL Buffer", moduleName);
    displaySqlBuffer(reply->getSqlBuffer(), (Lng32)reply->getSqlBufferLength());
  }

  // If all the rows in request buffer are processed, make sure
  // it gets cleaned up and then release it's reference here
  if (status == ALL_ROWS_PROCESSED || reqSqlBuf->atEOTD())
  {
    // This is the last reply buffer for the current request buffer
    reply->setLastBuffer(TRUE);

    if (traceInvokeDataAreas)
      ServerDebug("[UdrServ (%s)] Processed all rows from the request buffer.",
        moduleName);

    // We need the request buffer in a state where the stream knows
    // it can be freed. We call SqlBuffer::bufferFull() to accomplish
    // this even though the method name is a bit misleading.
    reqSqlBuf->bufferFull();
    currentRequest_ = NULL;
  }

  // Send reply if we have processed any rows
  if (numRowsProcessed > 0)
  {
    if (traceInvokeIPMS)
      ServerDebug("[UdrServ (%s)] Send Invoke Reply", moduleName);
    sendDataReply(udrGlobals_, *dataStream_, this);
  }

}  // SPInfo::work


RequestRowProcessingStatus
SPInfo::processOneRequestRow(SqlBuffer *reqSqlBuf,
                             SqlBuffer *replySqlBuf,
                             Lng32 &numRowsProcessed)
{
  const char *moduleName = "SPInfo::processOneRequestRow";

  NABoolean traceInvokeDataAreas = false;
  if (udrGlobals_->verbose_ && udrGlobals_->showInvoke_ &&
      udrGlobals_->traceLevel_ >= TRACE_DATA_AREAS)
    traceInvokeDataAreas = true;

  NABoolean traceInvokeIPMS = false;
  if (udrGlobals_->verbose_ && udrGlobals_->showInvoke_ &&
      udrGlobals_->traceLevel_ >= TRACE_IPMS)
    traceInvokeIPMS = true;

  Lng32 numTuppsBefore = replySqlBuf->getTotalTuppDescs();

  // First, allocate space in the reply buffer for one row plus EOD
  char *replyData = NULL;
  ControlInfo *replyControlInfo = NULL;
  NABoolean replyRowAllocated =
    allocateReplyRowAndEOD(udrGlobals_,
                           *replySqlBuf,
                           0, // dummy queue index, will be updated later
                           replyData,
                           (Lng32) getReplyRowSize(),
                           replyControlInfo);

  if (! replyRowAllocated)
  {
    // There are two cases.
    // Case 1: This is the first row in the reply buffer and row
    // allocation failed.
    if (numTuppsBefore == 0)
    {
      dataErrorReply(udrGlobals_, *dataStream_, UDR_ERR_MESSAGE_PROCESSING,
                     INVOKE_ERR_NO_REPLY_ROW, NULL);

      return ALL_ROWS_PROCESSED;
    }

    // Case 2: We have already inserted few reply rows. There is no more
    // room for reply row and EOD in reply buffer.
    // Also, it is possible that Data row is allocated and EOD is not
    // allocated. So lets' try to back out if there are any new tupps.
    backoutTupps(*replySqlBuf, numTuppsBefore);
    return REPLY_BUFFER_FULL;
  }
  
  // Extract a row from the request buffer
  down_state downState;
  tupp requestRow;
  NABoolean endOfData = reqSqlBuf->moveOutSendOrReplyData(
      TRUE,          // [IN] sending? (vs. replying)
      &downState,    // [OUT] queue state
      requestRow,    // [OUT] new data tupp_descriptor
      NULL,          // [OUT] new ControlInfo area
      NULL,          // [OUT] new diags tupp_descriptor
      NULL);         // [OUT] new stats area

  if (endOfData)
  {
    // delete the allocated row in reply buffer
    backoutTupps(*replySqlBuf, numTuppsBefore);

    return ALL_ROWS_PROCESSED;
  }

  numRowsProcessed++;
  char *requestData = requestRow.getDataPointer();
  queue_index requestQueueIndex = downState.parentIndex;

  // correct the queue index of reply row
  replyControlInfo->getUpState().parentIndex = requestQueueIndex;

  if (traceInvokeDataAreas)
  {
    ComUInt32 requestRowLen = requestRow.getAllocatedSize();
    ServerDebug("[UdrServ (%s)] Request info: queue index %d", moduleName,
                (Lng32) requestQueueIndex);
    ServerDebug("[UdrServ (%s)] Request Row Length %u", moduleName,
                (ULng32) requestRowLen);
    ServerDebug("[UdrServ (%s)] Dump of Request Invoke Data", moduleName);
    dumpBuffer((unsigned char *) requestData, requestRowLen);
  }

  if (rowDiags_ == NULL)
    rowDiags_ = ComDiagsArea::allocate(udrGlobals_->getIpcHeap());
  UDR_ASSERT(rowDiags_, "Unable to allocate memory for SQL diagnostics area");

  if (traceInvokeIPMS)
    ServerDebug("[UdrServ (%s)]  Call LM invokeRoutine", moduleName);

  // Call into Language manager
  LmLanguageManager *lm = udrGlobals_->getLM(getLanguage());
  UDR_ASSERT(lm,
             "No Language Manager available to process this INVOKE message");
  UDR_ASSERT(getLMHandle(), "Missing LM Routine handle");

  LmResult lmResult = lm->invokeRoutine(getLMHandle(),
					requestData, replyData,
                                        rowDiags_);

  if (traceInvokeIPMS)
  {
    char errorText[MAXERRTEXT];
    if (lmResult == LM_OK)
      sprintf(errorText,
              "[UdrServ (%.30s)]  LM::invokeRoutine was successful.",
              moduleName);
    else
      sprintf(errorText,
              "[UdrServ (%.30s)]  LM::invokeRoutine resulted in error.",
              moduleName);

    ServerDebug(errorText);
    doMessageBox(udrGlobals_,
                 TRACE_SHOW_DIALOGS,
                 udrGlobals_->showInvoke_,
                 errorText);
  }

  if (lmResult == LM_OK)
  {
    // Routine execution successful. Set state to INVOKED.
    if (getParamStyle() != COM_STYLE_SQLROW)
    {
      // $$$$ It would be better to check the routine type or the
      // number of result sets here rather than the parameter style
      if (getParamStyle() == COM_STYLE_JAVA_CALL)
        setupUdrResultSets(*rowDiags_);
    }

    setSPInfoState(SPInfo::INVOKED);
  }
  else
  {
    // Routine returned error. do result set cleanup in LM.
    // Set state to INVOKE_FAILED.
    getLMHandle()->cleanupResultSets(NULL);
    setSPInfoState(SPInfo::INVOKE_FAILED);
    NABoolean ok = convertReplyRowToErrorRow(replySqlBuf,
                                             numTuppsBefore,
                                             requestQueueIndex,
                                             *dataStream_,
                                             udrGlobals_);

    RequestRowProcessingStatus retcode;
    if (ok)
    {
      moveDiagsIntoStream(rowDiags_, replyControlInfo);
      retcode = A_ROW_IS_PROCESSED;
    }
    else
      retcode = ALL_ROWS_PROCESSED;

    rowDiags_->decrRefCount();
    rowDiags_ = NULL;
    return retcode;
  }

  // If the routine returned RS then add the details
  // about each RS into the reply.
  if (getNumResultSets() > 0)
  {
    NABoolean rsMoveOk = moveRSInfoIntoStream();
    if (! rsMoveOk)
    {
      // FIXME: We need to see if this is correct
      backoutTupps(*replySqlBuf, 0);
      dataErrorReply(udrGlobals_, *dataStream_, UDR_ERR_MESSAGE_PROCESSING,
                     INVOKE_ERR_NO_REPLY_BUFFER, NULL);

      rowDiags_->decrRefCount();
      rowDiags_ = NULL;
      return ALL_ROWS_PROCESSED;
    }
  }

  // Move diags into stream
  if (rowDiags_->getNumber() > 0)
  {
    moveDiagsIntoStream(rowDiags_, replyControlInfo);
    rowDiags_->decrRefCount();
    rowDiags_ = NULL;

    udrGlobals_->numErrUDR_++;
    udrGlobals_->numErrSP_++;
    udrGlobals_->numErrInvokeSP_++;
  }

#ifdef _DEBUG
  // In the debug build we allow the UDR server to corrupt the reply
  // by reversing the diags flag for this reply row. In response the
  // executor should cause the statement to fail due to some sort of
  // IPC error. The executor should not crash or allow the statement
  // to complete successfully.
  char *val = getenv("MXUDR_CORRUPT_DIAGS_FLAG");
  if (val && val[0] && replyControlInfo)
  {
    NABoolean oldValue = replyControlInfo->getIsExtDiagsAreaPresent();
    replyControlInfo->setIsExtDiagsAreaPresent(!oldValue);
  }
#endif

  return A_ROW_IS_PROCESSED;
}  // SPInfo::processOneRequestRow

void SPInfo::workTM()
{
  const char *moduleName = "SPInfo::workTM";

  NABoolean traceInvokeIPMS = false;
  if (udrGlobals_->verbose_ && udrGlobals_->showInvoke_ &&
      udrGlobals_->traceLevel_ >= TRACE_IPMS)
    traceInvokeIPMS = true;

  NABoolean traceInvokeDataAreas = false;
  if (udrGlobals_->verbose_ && udrGlobals_->showInvoke_ &&
      udrGlobals_->traceLevel_ >= TRACE_DATA_AREAS)
    traceInvokeDataAreas = true;

  doMessageBox(udrGlobals_,
               TRACE_SHOW_DIALOGS,
               udrGlobals_->showInvoke_,
               moduleName);

  // Do we have a request to process?
  if (! currentRequest_)
  {
    if (traceInvokeIPMS)
      ServerDebug("[UdrServ (%s)]  No current request buffer to process",
                  moduleName);

    return;
  }

  if (traceInvokeIPMS)
    ServerDebug("[UdrServ (%s)]  Processing Invoke Request", moduleName);

  // At this point we have three possibilities.
  // 1. Sql Buffer may not even be shipped if the server had 
  //    requested the client to send a continue request.
  // 2. Data buffer contains a flag indicating whether this is a
  //    scalar values buffer or data row buffer. In both these cases
  //    sql buffer is shipped.

  // Lets check if server had requested a continue request.
  if(spInfoState_ == INVOKED_EMITROWS)
  {
    //Nothing much to do except return.
    if (traceInvokeIPMS)
      ServerDebug("[UdrServ (%s)]  Continue request Received",
                   moduleName);
    
    //check to make sure there is no sql buffer to be released.
    //also make sure tmudf is already isInvoked()
    //PV
    return ;
  }

  // Print request buffer
  if (traceInvokeDataAreas)
  {
    ServerDebug("");
    ServerDebug("[UdrServ (%s)] Request SQL Buffer", moduleName);
    ServerDebug("[UdrServ (%s)] SPInfo State %s", moduleName, getSPInfoStateString());

    //Note that in the case of TM, we do not get a unpacked buffer.
    SqlBuffer *tempSqlBuffer = 
      (SqlBuffer*)udrHeapPtr_->allocateMemory(getRequestBufferSize());
    memset(tempSqlBuffer, '\0', getRequestBufferSize());
  
    // Copy the packed sql buffer from IPc stream.
    memcpy(tempSqlBuffer,currentRequest_->getSqlBuffer(),getRequestBufferSize());
    tempSqlBuffer->driveUnpack();

    displaySqlBuffer(tempSqlBuffer, getRequestBufferSize());
    udrHeapPtr_->deallocateMemory(tempSqlBuffer);
  }

  // Extract the SqlBuffer from the request. Note that this buffer is 
  // still in packed form. First the packed buffer must be copied and then
  // drive unpack on the copied buffer.
  SqlBuffer *reqSqlBuf = currentRequest_->getSqlBuffer();
  if (reqSqlBuf == NULL)
  {
    if (traceInvokeIPMS)
      ServerDebug("[UdrServ (%s)]  Empty request buffer in the request",
              moduleName);

    // Note that we need to do reply to client only if Invoke rountine
    // is not called. Because, we may enter here as part of getNextRow
    // callback, in which case, just return by setting error
    // codes.
    if(spInfoState_ == INVOKED_GETROWS)
    {
      spInfoState_ = INVOKED_GETROWS_FAILED;
      return;
    }
    
    dataErrorReply(udrGlobals_, *dataStream_, UDR_ERR_MESSAGE_PROCESSING,
                 INVOKE_ERR_NO_REQUEST_BUFFER, NULL);

    sendDataReply(udrGlobals_, *dataStream_, NULL);
    currentRequest_ = NULL;
    return ;
  }

  if (traceInvokeIPMS)
  {
    ComUInt32 initialNumRequestTupps = reqSqlBuf->getTotalTuppDescs();
    ServerDebug("[UdrServ (%s)]  There are %d tupps in the request buffer",
               moduleName, initialNumRequestTupps);
  }

  // Irrespective of sqlbuffer containing scalar values or data rows, 
  // we make a copy of the sqlBuffer to release the current sql buffer 
  // in IPC stream. This is because we may need to reuse the IPC buffer
  // for call backs to client from getNextRow and emitRow.
  
  //First init a preexisting copy if any
  SqlBuffer *sqlBufCopy = getReqSqlBuffer(currentRequest_->tableIndex());

  if(sqlBufCopy == NULL)
  {
    // Now allocate a new copy
    sqlBufCopy =
      (SqlBuffer*)udrHeapPtr_->allocateMemory(getRequestBufferSize());
  }

  // Initilize pre existing buffer rather than allocate new.
  //sqlBufCopy->driveInit(getRequestBufferSize(),
  //                      TRUE, SqlBuffer::NORMAL_);
  
  memset(sqlBufCopy, '\0', getRequestBufferSize());
  
  // Copy the packed sql buffer from IPc stream.
  memcpy(sqlBufCopy,reqSqlBuf,getRequestBufferSize());
  
  // Now drive unpack on the copied sql buffer.
  sqlBufCopy->driveUnpack();

  // Set flag indicating that buffer in use.
  sqlBufCopy->bufferInUse();

  // Set this unpacked buffer inside corresponding table info.
  setReqSqlBufferCopy(sqlBufCopy,currentRequest_->tableIndex());

  // Set the flag to indicate this is the last buffer sent by client.
  if(currentRequest_->isLastBuffer())
    setLastReqSqlBuffer(currentRequest_->tableIndex());

  //now release reqSqlBuf
  reqSqlBuf->bufferFull();

  // Free up any receive buffers no longer in use
  dataStream_->cleanupBuffers();

  //check if row data
  if(spInfoState_ == INVOKED_GETROWS)
  {
    //nothing more to do. just return, it
    // should be getNextRow processing

    // make sure client is sending data rows when the server
    // is expecting data rows. Client indicates that it is 
    // data rows by not setting scalar values flag.
    if(currentRequest_->sendingScalarValues())
    {
      spInfoState_ =  INVOKED_GETROWS_FAILED;
    }
    return;
  }

  // We reach here means initial scalar values to be processed.
  
  // Check Spinfo state to be in LOADED state if we are going to 
  // process scalar values.
  if(spInfoState_ != LOADED)
  {
    dataErrorReply(udrGlobals_, *dataStream_, UDR_ERR_MESSAGE_PROCESSING,
                   INVOKE_ERR_NO_REQUEST_BUFFER, NULL);

    sendDataReply(udrGlobals_, *dataStream_, NULL);
    currentRequest_ = NULL;
    return;
  }

  // Extract a row from the request buffer
  up_state  upState;
  tupp requestRow;
  NABoolean endOfData = sqlBufCopy->moveOutSendOrReplyData(
      TRUE,          // [IN] sending? (vs. replying)
      &upState,    // [OUT] queue state
      requestRow,    // [OUT] new data tupp_descriptor
      NULL,          // [OUT] new ControlInfo area
      NULL,          // [OUT] new diags tupp_descriptor
      NULL);         // [OUT] new stats area

 if(endOfData || (upState.status == ex_queue::Q_NO_DATA))
 {
   //nothing much we can do. return PV
 }

  char *requestData = requestRow.getDataPointer();
  queue_index requestQueueIndex = upState.parentIndex;

  // Store the parent index in spinfo for reply message later
  setParentIndex(requestQueueIndex);

  // Allocate diags area
  if (rowDiags_ == NULL)
    rowDiags_ = ComDiagsArea::allocate(udrGlobals_->getIpcHeap());
  UDR_ASSERT(rowDiags_, "Unable to allocate memory for SQL diagnostics area");
  
  // Call into Language manager
  LmLanguageManager *lm = udrGlobals_->getLM(getLanguage());
  UDR_ASSERT(lm,
             "No Language Manager available to process this INVOKE message");
  UDR_ASSERT(getLMHandle(), "Missing LM Routine handle");

  setSPInfoState(SPInfo::INVOKED);

  LmResult lmResult = lm->invokeRoutine(getLMHandle(),
					requestData, NULL,
                                        rowDiags_);

  if (traceInvokeIPMS)
  {
    char errorText[MAXERRTEXT];
    if (lmResult == LM_OK)
      sprintf(errorText,
              "[UdrServ (%.30s)]  LM::invokeRoutine was successful.",
              moduleName);
    else
      sprintf(errorText,
              "[UdrServ (%.30s)]  LM::invokeRoutine resulted in error.",
              moduleName);

    ServerDebug(errorText);
    doMessageBox(udrGlobals_,
                 TRACE_SHOW_DIALOGS,
                 udrGlobals_->showInvoke_,
                 errorText);
  }

  if (lmResult != LM_OK)
  {
    dataErrorReply(udrGlobals_, *dataStream_, UDR_ERR_MESSAGE_PROCESSING,
                   INVOKE_ERR_NO_REPLY_DATA_BUFFER, NULL, rowDiags_);
    rowDiags_ = NULL;

    if (traceInvokeIPMS)
      ServerDebug("[UdrServ (%s)] Send Invoke Error Reply", moduleName);

    sendDataReply(udrGlobals_, *dataStream_, NULL);
    currentRequest_ = NULL;
    return;
  }

  // release the row in the SqlBuffer before we allocate new buffers
  // and do other cleanup
  requestRow.release();
  requestData = NULL;

  if (paramStyle_ != COM_STYLE_CPP_OBJ &&
      paramStyle_ != COM_STYLE_JAVA_OBJ)
    {
      // should happen in the C interface only, C++ interface
      // sends this inside LmRoutine::invoke()

      //allocate reply buffer
      UdrDataBuffer *reply = new (*dataStream_, getReplyBufferSize())
        UdrDataBuffer(getReplyBufferSize(), UdrDataBuffer::UDR_DATA_OUT, NULL);
  
      if (reply == NULL || reply->getSqlBuffer() == NULL)
        {
          dataErrorReply(udrGlobals_, *dataStream_, UDR_ERR_MESSAGE_PROCESSING,
                         INVOKE_ERR_NO_REPLY_DATA_BUFFER, NULL);

          if (traceInvokeIPMS)
            ServerDebug("[UdrServ (%s)] Send Invoke Error Reply", moduleName);

          sendDataReply(udrGlobals_, *dataStream_, NULL);
          currentRequest_ = NULL;
          return;
        }

      // Allocate a reply row with EOD reply
      SqlBuffer *replyBuffer = reply->getSqlBuffer();
      allocateEODRow(udrGlobals_, *replyBuffer, requestQueueIndex); 

      // Indicate this is the last buffer reply from server
      reply->setLastBuffer(TRUE);

      // Also reset any flag that indicates send more data, just incase.
      reply->setSendMoreData(FALSE);

      sendDataReply(udrGlobals_, *dataStream_, this);
    }

  // Free up any receive buffers no longer in use
  dataStream_->cleanupBuffers();

  //Reset internal buffers and flags and prepare for reinvoke.
  reset();

  currentRequest_ = NULL;

}  // SPInfo::workTM



NABoolean SPInfo::moveRSInfoIntoStream()
{
  const char *moduleName = "SPInfo::moveRSInfoIntoStream";

  NABoolean traceInvokeIPMS = false;
  if (udrGlobals_->verbose_ && udrGlobals_->showInvoke_ &&
      udrGlobals_->traceLevel_ >= TRACE_IPMS)
    traceInvokeIPMS = true;

  // We are using copy buffering model of the buffered stream to
  // construct the UdrRSInfo object instead of the "in-place" or
  // copyless buffering of buffered streams since it's not clear
  // if the copyless IPC would result in any significant performance
  // improvement.
  UdrRSInfoMsg *rsInfoMsg = new (udrGlobals_->getIpcHeap())
    UdrRSInfoMsg(getNumResultSets(), udrGlobals_->getIpcHeap());

  if (rsInfoMsg == NULL)
    return false;

  // Add the result sets information to the invoke reply message
  for (ComUInt32 i = 0; i < getNumResultSets(); i++)
  {
    UdrResultSet *udrRS = getUdrResultSetByIndex(i);
    UDR_ASSERT(udrRS, "UDR Result Set object is Null");

    // +++ [TBD]: This will change when the ResultSetInfo is updated
    // to send back more meaningful data for measure and debug.
    ResultSetInfo rsInfo(udrRS->getProxySyntax(),
                         udrRS->getStatementHandle(),
                         udrRS->getContextHandle(),
                         udrGlobals_->getIpcHeap());
    rsInfoMsg->setRSInfo(i, rsInfo);
  }

  if (traceInvokeIPMS)
  {
    ServerDebug("[UdrServ (%s)] Number of result sets returned %d",
                moduleName,
                (Lng32) rsInfoMsg->getNumResultSets());
    ServerDebug("Dump of result sets Data");

    for (ComUInt32 i = 0; i < rsInfoMsg->getNumResultSets(); i++)
    {
      ServerDebug("  Data in result set #%d", (Lng32) i+1);
      ServerDebug("    Proxy syntax = %s",
                  rsInfoMsg->getRSInfo(i).getProxySyntax());
      ServerDebug("    CLI statement ID pointer = %p",
                  rsInfoMsg->getRSInfo(i).getStmtID());
      ServerDebug("    CLI context handle = %u",
                  rsInfoMsg->getRSInfo(i).getContextID());
      ServerDebug("");
    }
  }

  *dataStream_ << *rsInfoMsg; 
  rsInfoMsg->decrRefCount();

  return true;

} // SPInfo::moveRSInfoIntoStream


void SPInfo::moveDiagsIntoStream(ComDiagsArea *diags,
                                 ControlInfo *replyControlInfo)
{
  if (diags && diags->getNumber() > 0)
  {
    if (replyControlInfo)
      replyControlInfo->setIsExtDiagsAreaPresent(TRUE);

    *dataStream_ << *diags;
  }

  return;
}

//***********************************************************************
// SPList constructors
//
//
//***********************************************************************

SPList::SPList(UdrGlobals *udrGlobals)
  : udrGlobals_(udrGlobals),
    spInfoElement_(NULL) // on system heap
{
  str_cpy_all( &eyeCatcher_[0], EYE_SPLIST + '\0' + '\0', 4 );
  spInfoElement_.resize(0);  // default size
} // SPList::SPList()

//***********************************************************************
// SPList methods
//
//***********************************************************************

void SPList::displaySPList(Lng32 indent)
{
  SPInfo *sp;

  Lng32 indmax = (indent > 99) ? 99 : indent;
  
  char ind[100];
  Lng32 indIdx = 0;
  for (indIdx = 0; indIdx < indmax; indIdx++)
    ind[indIdx] = ' ';
  ind[indIdx] = '\0';
  
  ServerDebug("");
  ServerDebug("%sSPList Data Structure", ind );
  ServerDebug("%s---------------------", ind );
  
  ComUInt32 maxEntries = spInfoElement_.entries();
  for (ComUInt32 i = 0; i < maxEntries; i++)
  {
    sp = spInfoElement_.at(i);
    if (sp->getUdrHandle() == (UdrHandle)0)
    {
      ServerDebug("%sSPList[%u]: NULL", ind, i );
    }
    else
    {
      ServerDebug("%sSPList[%u]:", ind, i);
      sp->displaySPInfo(indmax + 2);
    }
  }
} // SPList::displaySPList

void SPList::displaySPListId(Lng32 indent)
{
  SPInfo *sp;
  
  Lng32 indmax = (indent > 99) ? 99 : indent;
  
  char ind[100];
  Lng32 indIdx = 0;
  for (indIdx = 0; indIdx < indmax; indIdx++)
    ind[indIdx] = ' ';
  ind[indIdx] = '\0';
  
  ServerDebug("");
  ServerDebug("%sSPList Identifiers", ind );
  ServerDebug("%s---------------------", ind );
  
  ComUInt32 maxEntries = spInfoElement_.entries();
  for (ComUInt32 i = 0; i < maxEntries; i++)
  {
    sp = spInfoElement_.at(i);
    if (sp->getUdrHandle() == (UdrHandle)0)
    {
      ServerDebug("%sSPList[%u]: NULL", ind, i );
    }
    else
    {
      ServerDebug("%sSPList[%u]:", ind, i );
      sp->displaySPInfoId(indmax + 2);
    }
  }

  ServerDebug("");

} // SPList::displaySPListId


// Seach SPList to find SPInfo that matches the input udrHandle.
// Return SPInfo object or NULL.
SPInfo *SPList::spFind(const UdrHandle &udrHandle)
{
  const char *moduleName = "SPList::spFind";
  
  SPInfo *sp;
  
  doMessageBox(udrGlobals_, TRACE_SHOW_DIALOGS,
               udrGlobals_->showSPInfo_, moduleName);
  
  ComUInt32 maxEntries = spInfoElement_.entries();
  for (ComUInt32 i = 0; i < maxEntries; i++)
  {
    sp = spInfoElement_.at(i);
    if (sp != NULL)
    {
      if (sp->getUdrHandle() == udrHandle) {
        udrGlobals_->setCurrSP(sp);
        return sp;
      }
    }
  }
  
  return NULL;
  
} // SPList::spFind


// Find oldest, least used SPJ and free up all resources allocated
// and mark as unloaded.
// Used to recover from memory problems in UDR Server.
void SPList::releaseOldestSPJ(ComDiagsArea &d)
{
  const char *moduleName = "SPList::releaseOldestSPJ";

  doMessageBox(udrGlobals_, TRACE_SHOW_DIALOGS,
               udrGlobals_->showSPInfo_, moduleName);
  
  if (!spInfoElement_.isEmpty())
  {
    Int64 oldestTS;
    Int64 leastNumCalls;
    Int64 numCalls;
    Int64 lastTS;
    CollIndex releaseIdx;
    
    SPInfo *spi;
    
    spi = spInfoElement_.at(0);
    oldestTS = spi->getLastCallTs();
    leastNumCalls = spi->getNumCalls();
    releaseIdx = 0;
    
    ComUInt32 SPListMax = spInfoElement_.entries();
    
    // loop over SPList entries finding SP with least calls
    // having oldest last timestamp.
    
    for (ComUInt32 idx = 1; idx < SPListMax; idx++)
    {
      spi = spInfoElement_.at(idx);
      numCalls = spi->getNumCalls();
      lastTS = spi->getLastCallTs();
      if (numCalls < leastNumCalls)
      {
        leastNumCalls = numCalls;
        oldestTS = lastTS;
        releaseIdx = idx;
      }
      else
      {
        if (numCalls == leastNumCalls)
        {
          if (spi->getLastCallTs() <= oldestTS)
          {
            oldestTS = lastTS;
            releaseIdx = idx;
          }
        }
      }
    }
    
    // release oldest entry...
    spi = spInfoElement_.at(releaseIdx);
    
    spi->releaseSP(FALSE /* report no errors */, d);
    
  }
  
} // SPList::releaseOldestSPJ

void SPList::addToSPList(SPInfo *spinfo)
{
  spInfoElement_.resize(spInfoElement_.entries() + 1);
  spInfoElement_.insert(spinfo);

  udrGlobals_->numTotalSPs_++;
  udrGlobals_->numCurrSPs_++;

  if (udrGlobals_->verbose_ &&
      udrGlobals_->traceLevel_ >= TRACE_DETAILS &&
      udrGlobals_->showSPInfo_)
  {
    ServerDebug("[UdrServ (SPINFO)] Added Element to SPList. %d entries",
                (Lng32) spInfoElement_.entries());
    displaySPListId(2);
  }
} // SPList::addToSPList

void SPList::removeFromSPList(SPInfo *spinfo)
{
  udrGlobals_->numTotalSPs_--;
  udrGlobals_->numCurrSPs_--;

  // TBD: Ramana
  // For the following to be meaningful, we need to make sure that
  // numCurrRSets_ incremented after a UDR invocation. We do not have
  // code to do that right now.
  udrGlobals_->numCurrRSets_-= spinfo->getNumResultSets();

  spInfoElement_.remove(spinfo);
} // SPList::removeFromSPList

LmParameter &SPInfo::getLmParameter(ComUInt32 i)
{
  UDR_ASSERT(i < (numParameters_ + maxNumResultSets_),
             "An invalid index was passed to SPInfo::getLmParameter()");
  return lmParameters_[i];
}

void SPInfo::setNumTableInfo(ComUInt32 t)
{
  numTableInfo_ = t;
  if (numTableInfo_ > 0)
  {
    ComUInt32 inputTableBytes= numTableInfo_ * sizeof(LmTableInfo);

    tableInfo_ = (LmTableInfo *) udrHeapPtr_->allocateMemory(inputTableBytes);
    memset(tableInfo_, 0, inputTableBytes);
  }
}

void SPInfo::setTableInputInfo(const UdrTableInputInfo info[])
{
  for(ComUInt32 i = 0; i < numTableInfo_; i++)
  {
    LmTableInfo *tableInfo = &tableInfo_[info[i].getTabIndex()];
    tableInfo->tabIndex_ = info[i].getTabIndex();
    tableInfo->tableNameLen_ = info[i].getTableNameLen();
    tableInfo->rowLength_ = info[i].getRowLength();
    tableInfo->tableName_ = (char*)udrHeapPtr_->allocateMemory(tableInfo->tableNameLen_ + 1);
    strncpy(tableInfo->tableName_, info[i].getTableName(), tableInfo->tableNameLen_);
    tableInfo->tableName_[tableInfo->tableNameLen_] = '\0';

    // initialize individual column descriptors
    tableInfo->numInColumns_ = info[i].getNumColumns();
    if(tableInfo->inColumnParam_ == NULL)
    {
      ComUInt32 lmParamBytes = tableInfo->numInColumns_ * sizeof(LmParameter);
      tableInfo->inColumnParam_ = (LmParameter *) udrHeapPtr_->allocateMemory(lmParamBytes);
      memset(tableInfo->inColumnParam_, 0, lmParamBytes);
    }

    for(ComUInt32 j = 0; j < tableInfo->numInColumns_; j++)
    {
      const UdrParameterInfo &pInfo = info[i].getInTableColumnDesc(j);

      // Determine the encoding charset. We will use =_SQL_MX_ISO_MAPPING
      // define value if the param's charset is ISO88591.
      CharInfo::CharSet charset = (CharInfo::CharSet) pInfo.getEncodingCharSet();
      if (charset == CharInfo::ISO88591)
        charset = udrGlobals_->getIsoMapping();

      tableInfo->inColumnParam_[j].init((ComFSDataType) pInfo.getFSType(),
                                         pInfo.getPrec(),
                                         pInfo.getScale(),
                                         charset,
                                         (CharInfo::Collation) pInfo.getCollation(),
                                         COM_INPUT_COLUMN,
                                         pInfo.isLmObjType(),
                                         RS_NONE,
                                         pInfo.getDataOffset(),
                                         pInfo.getDataLength(),
                                         pInfo.getNullIndicatorOffset(),
                                         pInfo.getNullIndicatorLength(),
                                         pInfo.getVCLenIndOffset(),
                                         pInfo.getVCIndicatorLength(),
                                         0, 0, 0, 0, 0, 0, // output offsets and lengths
                                         pInfo.getParamName());
    }
  }
}

SqlBuffer* SPInfo::getReqSqlBuffer(ComSInt32 tableIndex)
{
  if(tableIndex < 0 ) return sqlBufferScalar_;

  return tableInfo_[tableIndex].getReqSqlBuffer();
}

SqlBuffer* SPInfo::getEmitSqlBuffer(ComSInt32 tableIndex)
{
  if (numTableInfo_ == 0)
    return sqlBufferTVF_;
  else
    return tableInfo_[tableIndex].getEmitSqlBuffer();
}

void SPInfo::setEmitSqlBuffer(SqlBuffer *buf, ComSInt32 tableIndex)
{
  if (numTableInfo_ == 0)
    sqlBufferTVF_ = buf;
  else
    tableInfo_[tableIndex].setEmitSqlBuffer(buf);
}

void SPInfo::reset(void)
{
   if(sqlBufferScalar_ != NULL)
   {
     sqlBufferScalar_->bufferFull();
     udrHeapPtr_->deallocateMemory(sqlBufferScalar_);
     sqlBufferScalar_ = NULL;
   }
   if(sqlBufferTVF_ != NULL)
   {
     sqlBufferTVF_->bufferFull();
     udrHeapPtr_->deallocateMemory(sqlBufferTVF_);
     sqlBufferTVF_ = NULL;
   }
   for(ComUInt32 i = 0; i < getNumTables(); i++)
   {
      tableInfo_[i].reset(udrHeapPtr_);
   }
   parentIndex_ = 0;

   setSPInfoState(SPInfo::LOADED);
}

ComUInt32 SPInfo::getInputRowLength(ComSInt32 tableIndex)
{
  return tableInfo_[tableIndex].getRequestRowSize();
}


void SPInfo::setReqSqlBufferCopy(SqlBuffer *sqlBufCopy, ComSInt32 tableIndex)
{
  if(tableIndex < 0)
  {
    //SQL Buffer containing Scalar values
    sqlBufferScalar_ = sqlBufCopy;
  }
  else
  {
    // check for error PV
    tableInfo_[tableIndex].setReqSqlBuffer(sqlBufCopy);
  }
}

NABoolean SPInfo::isLastReqSqlBuffer(ComSInt32 tableIndex)
{
  return tableInfo_[tableIndex].isLastReqSqlBuffer();
}

void SPInfo::setLastReqSqlBuffer(ComSInt32 tableIndex)
{
  if(tableIndex < 0 ) return; // do nothing.

  tableInfo_[tableIndex].setLastReqSqlBuffer();
}
