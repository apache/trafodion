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
****************************************************************************
*
* File:         CmpMessage.cpp
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

// -----------------------------------------------------------------------

#include "Platform.h"



#include "CmpMessage.h"
#include "NAExecTrans.h"
#include "time.h"
#include "charinfo.h"
#include "FragmentDir.h"
#include "ComSysUtils.h"

// -----------------------------------------------------------------------
// methods for CmpMessageObj
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// helper routines for CmpMessageObj class
// -----------------------------------------------------------------------

IpcMessageObjSize CmpMessageObj::packIntoBuffer(IpcMessageBufferPtr& buffer,
						char* strPtr)
{
  CmpMsgBufLenType length;
  if (strPtr==NULL)
     length=0;
  else
     length=str_len(strPtr)+1; // 1 is for the null-terminator char

   // NOT a recursive call.
  IpcMessageObjSize result = ::packIntoBuffer(buffer,length);

  if (strPtr!=NULL)
     str_cpy_all((char*)buffer,strPtr,length);
  buffer += length;
  return result+length;
}

IpcMessageObjSize CmpMessageObj::packIntoBuffer(IpcMessageBufferPtr& buffer,
						void* strPtr, Lng32 sz)
{

   // NOT a recursive call.
  IpcMessageObjSize result = ::packIntoBuffer(buffer,sz);

  if (strPtr!=NULL)
     str_cpy_all((char*)buffer,(char*)strPtr,sz);
  buffer += sz;
  return result+sz;
}

void CmpMessageObj::unpackBuffer(IpcConstMessageBufferPtr& buffer,
				 char* &strPtr,
				 CollHeap* h)
{
  NADELETEBASIC(strPtr,h);
   
  CmpMsgBufLenType length;
  // NOT a recursive call.
  ::unpackBuffer(buffer,length);
  if (length==0)
     strPtr = NULL;
  else {
     strPtr = new(h) char[length];
     assert(strPtr!=NULL);
     str_cpy_all((char*)strPtr,(char*) buffer,length);
     buffer += length;
  }
}

void CmpMessageObj::unpackBuffer(IpcConstMessageBufferPtr& buffer,
				 char* strPtr, ULng32 maxSize,
                                 ULng32& sizeMoved)
{  
  CmpMsgBufLenType length;
  // NOT a recursive call.
  ::unpackBuffer(buffer,length);
  assert(length <= maxSize);
  if ( length > 0 )
  {
     str_cpy_all((char*)strPtr,(char*) buffer,length);
     buffer += length;
  }
  sizeMoved = length;
}


// -----------------------------------------------------------------------
// constructors
// -----------------------------------------------------------------------

CmpMessageObj::CmpMessageObj(IpcMessageObjType t, CollHeap* h) :
     IpcMessageObj(t,EXECMPIPCVERSION)
{
  h_ = h;

  NA_timeval tp;
  NA_gettimeofday(&tp, 0);

  id_ = tp.tv_sec * 1000000 + tp.tv_usec;

}

IpcMessageRefCount CmpMessageObj::decrRefCount() 
{
  if (getRefCount()==1) 
    {
      if (h_) 
	{
          destroyMe();
	  h_->deallocateMemory(this);
        }
      else
	delete this;
      return 0;
    }

  return IpcMessageObj::decrRefCount();
}

IpcMessageObjSize CmpMessageObj::mypackedLength(void)
{
  IpcMessageObjSize size = baseClassPackedLength();
  size += sizeof(id_);
  return size;
}

IpcMessageObjSize CmpMessageObj::packMyself(IpcMessageBufferPtr& buffer)
{
  IpcMessageObjSize size = packBaseClassIntoMessage(buffer);
  size += packIntoBuffer(buffer,(ID)id_);
  return size;
}

void CmpMessageObj::unpackMyself(IpcMessageObjType,
				  IpcMessageObjVersion,
				  NABoolean,
				  IpcMessageObjSize,
				  IpcConstMessageBufferPtr&  buffer)
{
  unpackBaseClass(buffer);

  unpackBuffer(buffer, id_);
}

// -----------------------------------------------------------------------
// methods for CmpMessageRequestBasic
// -----------------------------------------------------------------------

CmpMessageRequestBasic::CmpMessageRequestBasic(MessageTypeEnum t,
					       CollHeap* h) :
  CmpMessageObj(t,h)
{
}

CmpMessageRequestBasic::~CmpMessageRequestBasic()
{
}

IpcMessageObjSize CmpMessageRequestBasic::mypackedLength(void)
{
  IpcMessageObjSize size = CmpMessageObj::mypackedLength();
  return size;
}

IpcMessageObjSize CmpMessageRequestBasic::packMyself
(IpcMessageBufferPtr& buffer)
{
  IpcMessageObjSize size = CmpMessageObj::packMyself(buffer);
  return size;
}

void CmpMessageRequestBasic::unpackMyself(IpcMessageObjType objType,
					  IpcMessageObjVersion objVersion,
					  NABoolean sameEndianness,
					  IpcMessageObjSize objSize,
					  IpcConstMessageBufferPtr&  buffer)
{
  CmpMessageObj::unpackMyself(objType, objVersion, sameEndianness,
			      objSize, buffer);    
}

// -----------------------------------------------------------------------
// methods for CmpMessageReplyBasic
// -----------------------------------------------------------------------
CmpMessageReplyBasic::CmpMessageReplyBasic(MessageTypeEnum t, ID id,
					   CollHeap* h) :
     CmpMessageObj(t,h),
     request_(id)
{
}

IpcMessageObjSize CmpMessageReplyBasic::mypackedLength(void)
{
  IpcMessageObjSize size = CmpMessageObj::mypackedLength();
  size += sizeof(request_);
  return size;
}

IpcMessageObjSize CmpMessageReplyBasic::packMyself(IpcMessageBufferPtr& buffer)
{
  IpcMessageObjSize size = CmpMessageObj::packMyself(buffer);
  size += packIntoBuffer(buffer,request_);
  return size;
}

void CmpMessageReplyBasic::unpackMyself(IpcMessageObjType objType,
					IpcMessageObjVersion objVersion,
					NABoolean sameEndianness,
					IpcMessageObjSize objSize,
					IpcConstMessageBufferPtr&  buffer)
{
  CmpMessageObj::unpackMyself(objType, objVersion, sameEndianness,
			      objSize, buffer);    
  unpackBuffer(buffer, request_);
}

CmpCompileInfo::CmpCompileInfo(char * sourceStr, Lng32 sourceStrLen,
			       Lng32 sourceStrCharSet,
			       char * schemaName, Lng32 schemaNameLen,
                               ULng32 inputArrayMaxsize, short atomicity)
     : flags_(0),
       sqltext_(sourceStr), sqlTextLen_(sourceStrLen),
       sqlTextCharSet_(sourceStrCharSet),
       schemaName_(schemaName), schemaNameLen_(schemaNameLen),
       inputArrayMaxsize_(inputArrayMaxsize),
       unused2_(0)
{
  if (atomicity == 1) {
      flags_ |= ROWSET_ATOMICITY_SPECIFIED;
      flags_ &= ~ROWSET_NOT_ATOMIC; 
  }
  else if (atomicity == 2) {
    flags_ |= ROWSET_ATOMICITY_SPECIFIED;
    flags_ |= ROWSET_NOT_ATOMIC; 
  }
  else 
    flags_ &= ~ROWSET_ATOMICITY_SPECIFIED;
  str_pad(fillerBytes_, FILLERSIZE, '\0');
}

CmpCompileInfo::CmpCompileInfo()
  : flags_(0),
    sqltext_(NULL), 
    sqlTextLen_(0),
    sqlTextCharSet_(0),
    schemaName_(NULL), 
    schemaNameLen_(0),
    inputArrayMaxsize_(0)
{
  str_pad(fillerBytes_, FILLERSIZE, '\0');
}

void CmpCompileInfo::init()
{
  flags_ = 0;
  sqltext_ = NULL; 
  sqlTextLen_ = 0;
  sqlTextCharSet_ = 0;
  schemaName_ = NULL; 
  schemaNameLen_ = 0;
  inputArrayMaxsize_ = 0;
}

Lng32 CmpCompileInfo::getLength()
{
  Lng32 sizeOfThis = getClassSize();
  Lng32 totalLength = ROUND8(sizeOfThis) + getVarLength();
  return totalLength;
}

Lng32 CmpCompileInfo::getVarLength()
{
  return ROUND8(sqlTextLen_) + 
    ROUND8(schemaNameLen_);
}

void CmpCompileInfo::packVars(char * buffer, CmpCompileInfo *ci,
                              Lng32 &nextOffset)
{
  if (sqltext_ && (sqlTextLen_ > 0))
    {
      str_cpy_all(&buffer[nextOffset], sqltext_, sqlTextLen_);
      ci->sqltext_ = (char *)((long)nextOffset);
      nextOffset += ROUND8(sqlTextLen_);
    }
  
  if (schemaName_ && (schemaNameLen_ > 0))
    {
      str_cpy_all(&buffer[nextOffset], (char *)schemaName_, schemaNameLen_);
      ci->schemaName_ = (char *)((long)nextOffset);
      nextOffset += ROUND8(schemaNameLen_);
    }
}

void CmpCompileInfo::pack(char * buffer)
{
  CmpCompileInfo * ci = (CmpCompileInfo *)buffer;

  Lng32 classSize = getClassSize();
  Lng32 nextOffset = 0;
  str_cpy_all(buffer, (char *)this, classSize);

  nextOffset += ROUND8(classSize);

  packVars(buffer, ci, nextOffset);
}

void CmpCompileInfo::unpack(char * base)
{
  if (sqltext_)
    {
      sqltext_ = base + (Lng32)((Long)sqltext_);
    }
 
  if (schemaName_)
    {
      schemaName_ = base + (Lng32)(Long)schemaName_;
    }

}

void CmpCompileInfo::getUnpackedFields(char* &sqltext,
				       char* &schemaName)
{
  char * base = (char *)this;

  if (sqltext_)
    {
      sqltext = base + (Lng32)(Long)sqltext_;
    }
  
  if (schemaName_)
    {
      schemaName = base + (Lng32)(Long)schemaName_;
    }

}

const short CmpCompileInfo::getRowsetAtomicity()
{
  // 0 denotes unspecified, 1 denotes atomic and 2 denotes not_atomic

  if (!(flags_ & ROWSET_ATOMICITY_SPECIFIED))
    return 0;
  // The previous bit shows that atomicity has been specified. 
  // Read the next bit to distinguish between ATOMIC and NOT ATOMIC
  if (flags_ & ROWSET_NOT_ATOMIC)
    return 2;
  else
    return 1;
}

const short CmpCompileInfo::getHoldableAttr()
{
  if (isAnsiHoldable())
    return SQLCLIDEV_ANSI_HOLDABLE;
  else
  if (isPubsubHoldable())
    return SQLCLIDEV_PUBSUB_HOLDABLE;
  else
    return SQLCLIDEV_NONHOLDABLE;
}

// ---------------------------------------------------------
// methos for CmpDDLwithStatusInfo
// ---------------------------------------------------------
CmpDDLwithStatusInfo::CmpDDLwithStatusInfo()
    : CmpCompileInfo()
{
  statusFlags_ = 0;
  step_ = 0;
  substep_ = 0;
  statusFlags_ = 0;
  msgLen_ = 0;
  blackBoxLen_ = 0;
  blackBox_ = NULL;
};

CmpDDLwithStatusInfo::CmpDDLwithStatusInfo(char * sourceStr, Lng32 sourceStrLen,
                                           Lng32 sourceStrCharSet,
                                           char * schemaName, Lng32 schemaNameLen)
  : CmpCompileInfo(sourceStr, sourceStrLen, sourceStrCharSet,
                   schemaName, schemaNameLen,
                   0, 0)
{
  statusFlags_ = 0;
  step_ = 0;
  substep_ = 0;
  statusFlags_ = 0;
  msgLen_ = 0;
  blackBoxLen_ = 0;
  blackBox_ = NULL;
};

void CmpDDLwithStatusInfo::copyStatusInfo(CmpDDLwithStatusInfo *from)
{
  if (! from)
    return;

  statusFlags_ = from->statusFlags_;
  step_ = from->step_;
  substep_ = from->substep_;
  statusFlags_ = from->statusFlags_;
}

Lng32 CmpDDLwithStatusInfo::getLength()
{
  Lng32 sizeOfThis = getClassSize();
  Lng32 totalLength = ROUND8(sizeOfThis) + getVarLength();

  if ((blackBoxLen_ > 0) && (blackBox_))
    totalLength += ROUND8(blackBoxLen_);
  return totalLength;
}

void CmpDDLwithStatusInfo::pack(char * buffer)
{
  CmpDDLwithStatusInfo * ci = (CmpDDLwithStatusInfo *)buffer;

  Lng32 classSize = getClassSize();
  Lng32 nextOffset = 0;
  str_cpy_all(buffer, (char *)this, classSize);

  nextOffset += ROUND8(classSize);

  packVars(buffer, ci, nextOffset);

  if (blackBox_ && (blackBoxLen_ > 0))
    {
      str_cpy_all(&buffer[nextOffset], blackBox_, blackBoxLen_);
      ci->blackBox_ = (char *)((long)nextOffset);
      nextOffset += ROUND8(blackBoxLen_);
    }

}

void CmpDDLwithStatusInfo::unpack(char * base)
{
  CmpCompileInfo::unpack(base);

  if (blackBox_)
    {
      blackBox_ = base + (Lng32)((Long)blackBox_);
    }
}

// -----------------------------------------------------------------------
// methods for CmpMessageReply
// -----------------------------------------------------------------------

CmpMessageReply::CmpMessageReply(MessageTypeEnum e, 
				 ID request,
				 CollHeap* h, 
                                 char* preAllocatedData,
                                 ULng32 preAllocatedSize,
				 CollHeap* outh) :
				 CmpMessageReplyBasic(e,request, h) 
{
  data_ = 0;
  sz_ = 0;
  dataAllocated_ = FALSE;
  preAllocatedData_ = preAllocatedData;
  preAllocatedSize_ = preAllocatedSize;
  outh_ = outh;
}

void CmpMessageReply::destroyMe() 
{
  if ( dataAllocated_)
    NADELETEBASIC(data_, outh_);
}

CmpMessageReply::~CmpMessageReply()
{
  destroyMe();
}

IpcMessageObjSize CmpMessageReply::mypackedLength()
{
  IpcMessageObjSize size = CmpMessageReplyBasic::mypackedLength();
  size += sizeof(sz_);
  advanceSize(size,data_,sz_);
  return size;
}

IpcMessageObjSize CmpMessageReply::packMyself(IpcMessageBufferPtr& buffer)
{
  IpcMessageObjSize size= CmpMessageReplyBasic::packMyself(buffer);
  size += ::packIntoBuffer(buffer,sz_);
  size += packIntoBuffer(buffer,data_,sz_);
  return size;
}

void CmpMessageReply::unpackMyself(IpcMessageObjType objType,
				   IpcMessageObjVersion objVersion,
				   NABoolean sameEndianness,
				   IpcMessageObjSize objSize,
				   IpcConstMessageBufferPtr& buffer)
{
  CmpMessageReplyBasic::unpackMyself(objType, objVersion, sameEndianness,
			       objSize, buffer);
  ::unpackBuffer(buffer, sz_);
  if ( !dataAllocated_ )
    // This is to avoid the deallocate of space insize unpackBuffer for data_, 
    // if the data is not allocated internally, should not delete it either.
    data_ = 0;
  if ( preAllocatedData_ && preAllocatedSize_ >= sz_ ) 
  {
    // use the preAllocatedData_ to hold the reply
    ULng32 temp;
    unpackBuffer(buffer, preAllocatedData_, preAllocatedSize_, temp);
    data_ = preAllocatedData_;
    dataAllocated_ = FALSE;
  }
  else
  {
    unpackBuffer(buffer, data_, outh_);   
    dataAllocated_ = TRUE;
  }
}

void CmpMessageReplyCode::destroyMe() 
{
  CmpMessageReply::destroyMe();

  if (fragmentDir_)
    delete fragmentDir_;

  fragmentDir_ = NULL;
}

CmpMessageReplyCode::~CmpMessageReplyCode()
{
  destroyMe();
}

IpcMessageObjSize CmpMessageReplyCode::mypackedLength()
{
  if (fragmentDir_ == NULL)
    return CmpMessageReply::mypackedLength();

  IpcMessageObjSize size = CmpMessageReplyBasic::mypackedLength();
  size += sizeof(getSize());
  size += sizeof(getSize()) + getSize();
  return size;
}

IpcMessageObjSize CmpMessageReplyCode::packMyself(IpcMessageBufferPtr& buffer)
{
  if (fragmentDir_ == NULL)
    return CmpMessageReply::packMyself(buffer);

  IpcMessageObjSize size= CmpMessageReplyBasic::packMyself(buffer);
  size += ::packIntoBuffer(buffer,getSize());

  // copy the objects of all spaces into one big buffer
  size += ::packIntoBuffer(buffer,getSize());
  if (getSize() > 0)
    {
      Lng32 outputLengthSoFar = 0;
      ULng32 out_buflen = getSize();
      for (CollIndex i = 0; i < fragmentDir_->entries(); i++)
	{
	  // copy the next space into the buffer
	  if (fragmentDir_->getSpace(i)->makeContiguous(
	       &buffer[outputLengthSoFar],
	       out_buflen - outputLengthSoFar) == 0)
	    return 0;
	  outputLengthSoFar += fragmentDir_->getFragmentLength(i);
	}

      size += getSize();
    }

  return size;
}

IpcMessageObjSize
CmpMessageReplyCode::copyFragsToBuffer(IpcMessageBufferPtr& buffer)
{
  IpcMessageObjSize size = 0;
  if (getSize() > 0)
    {
      Lng32 outputLengthSoFar = 0;
      ULng32 out_buflen = getSize();
      for (CollIndex i = 0; i < fragmentDir_->entries(); i++)
        {
          // copy the next space into the buffer
          if (fragmentDir_->getSpace(i)->makeContiguous(
               &buffer[outputLengthSoFar],
               out_buflen - outputLengthSoFar) == 0)
            return 0;
          outputLengthSoFar += fragmentDir_->getFragmentLength(i);
        }

      size += getSize();
    }

  return size;
}

// -----------------------------------------------------------------------
// methods for CmpMessageRequest
// -----------------------------------------------------------------------

CmpMessageRequest::CmpMessageRequest(MessageTypeEnum e,
				     char* data,
				     CmpMsgBufLenType l,
				     CollHeap* h, Lng32 cs,
                                     const char *parentQid, Lng32 parentQidLen) :
				     CmpMessageRequestBasic(e,h) 
{
  data_ = 0;
  sz_ = 0;
  flags_ = 0;
  copyToString(data_, sz_, data, l);
  charSet_ = cs==SQLCHARSETCODE_UNKNOWN ? CharInfo::findLocaleCharSet() : cs; 
  parentQid_ = parentQid;
  parentQidLen_ = parentQidLen;
  allocated_ = FALSE;
}

void CmpMessageRequest::destroyMe()
{
   NADELETEBASIC(data_, getHeap());
   if (allocated_ && parentQid_ != NULL)
   {
     NADELETEBASIC(parentQid_, getHeap());
     parentQid_ = NULL;
     parentQidLen_ = 0;
   }
} 
  
void CmpMessageRequest::copyToString(char* &dest, CmpMsgBufLenType& sz,
				     char* source, CmpMsgBufLenType sz1) 
{
  NADELETEBASIC(dest, getHeap());
  if ( source ) 
    {
      sz = sz1 + 1;
      dest = new (getHeap()) char[sz];
      str_cpy_all(dest, source, sz1);      
      dest[sz1] = 0;
    }
  else
    {
      sz = 0;
      dest = 0;
    }
}

IpcMessageObjSize CmpMessageRequest::mypackedLength()
{
  IpcMessageObjSize size = CmpMessageRequestBasic::mypackedLength();
  size += sizeof(sz_);
  size += sizeof(flags_);
  size += sizeof(charSet_);
  advanceSize(size, data_, sz_);
  size += sizeof(parentQidLen_);
  size += parentQidLen_;
  return size;  
}

IpcMessageObjSize CmpMessageRequest::packMyself(IpcMessageBufferPtr& buffer)
{
  IpcMessageObjSize size= CmpMessageRequestBasic::packMyself(buffer);
  size += ::packIntoBuffer(buffer, sz_);
  size += ::packIntoBuffer(buffer, flags_);
  size += ::packIntoBuffer(buffer, charSet_);
  size += packIntoBuffer(buffer, data_, sz_);
  size += ::packIntoBuffer(buffer, parentQidLen_);
  if (parentQidLen_ != 0 && parentQid_ != NULL)
      size += packStrIntoBuffer(buffer, (char *)parentQid_, parentQidLen_);
  return size;
}

void CmpMessageRequest::unpackMyself(IpcMessageObjType objType,
				     IpcMessageObjVersion objVersion,
				     NABoolean sameEndianness,
				     IpcMessageObjSize objSize,
				     IpcConstMessageBufferPtr& buffer)
{
  CmpMessageRequestBasic::unpackMyself(objType, objVersion, sameEndianness,
				       objSize, buffer);
  ::unpackBuffer(buffer, sz_);
  ::unpackBuffer(buffer, flags_);
  ::unpackBuffer(buffer, charSet_);
  unpackBuffer(buffer, data_, getHeap());
  ::unpackBuffer(buffer, parentQidLen_);
  if (parentQidLen_ != 0)
  {
    allocated_ = TRUE;
    parentQid_ = new ((NAHeap *)(getHeap())) char[parentQidLen_+1];
    unpackStrFromBuffer(buffer, (char *)parentQid_, parentQidLen_);
    *((char *)(parentQid_+ parentQidLen_)) = '\0';
  }
}


// -----------------------------------------------------------------------
// Methods for CmpMessageEnvs
// -----------------------------------------------------------------------

CmpMessageEnvs::CmpMessageEnvs(EnvsOperatorEnum o, 
			       char ** inEnviron, 
			       NABoolean emptyOne, CollHeap* h):
CmpMessageObj(ENVS_REFRESH,h),envs_(0),operator_(o), cwd_(0)
{
  if (emptyOne)
    // This is just to create an empty CmpMessageEnvs object in arkcmp
    // side.  The contents will later be filled in unpack method to
    // get the data passed from executor.  In arkcmp, this object will
    // be created in (CmpConnection::actOnReceive) and processed. The
    // contents will be used in ProcessEnvs object to set up the exact
    // environment variables.
    return; 

  // Create a CmpMessageEnvs from executor side to include the environmental 
  // information to be sent to arkcmp.
  // The storage allocated will later be deallocated in destroyMe() method.
  // setup envs_
  Lng32 count = 0;
  char ** e = NULL;
  if (inEnviron)
    {
      e = inEnviron;
      
      for (count=0; e[count]; count++);
    }

  nEnvs_ = count;
  envs_ = new(getHeap()) char*[nEnvs_];
  for (count=0; ((e != NULL) && (count < nEnvs_)); count++) 
    { 
      Lng32 l = str_len(e[count])+1;
      envs_[count] = new(getHeap()) char[l];
      str_cpy_all(envs_[count], e[count], l);
    } 

  if (inEnviron)
    {
      // setup cwd_
      char cwd[500];
      getcwd(cwd, 500);
      Lng32 l = str_len(cwd)+1;
      cwd_ = new(getHeap()) char[l];
      str_cpy_all(cwd_, cwd, l);
      //free(cwd); // free the one from getcwd() function,
      //supposed to be from malloc.
    }
  else
    cwd_ = NULL;


  // if a transaction is active, get the transid by calling
  // the CLI procedure. 

  Int64 transid = -1;
  if (NAExecTrans(0, transid))
  {
    activeTrans_ = 1;
    transId_ = transid;
  }
  else
  {
    transId_ = -1 ;
    activeTrans_ = 0;
  }
}

IpcMessageObjSize CmpMessageEnvs::mypackedLength()
{
  IpcMessageObjSize size = CmpMessageObj::mypackedLength();
  size += sizeof(operator_);
  size += sizeof(nEnvs_);
  for(Int32 i=0;i<nEnvs_; i++) 
    advanceSize(size, envs_[i]);
  advanceSize(size, cwd_);
  size += sizeof(activeTrans_);
  advanceSize(size, transId_);
  return size;  
}

IpcMessageObjSize CmpMessageEnvs::packMyself(IpcMessageBufferPtr& buffer)
{
  IpcMessageObjSize size= CmpMessageObj::packMyself(buffer);
  size += ::packIntoBuffer(buffer,operator_);
  size += ::packIntoBuffer(buffer, nEnvs_);
  for ( Int32 i=0; i < nEnvs_; i++ )
    size += packIntoBuffer(buffer, envs_[i]);
  size += packIntoBuffer(buffer, cwd_);
  size += ::packIntoBuffer(buffer, activeTrans_);
  size += packIntoBuffer(buffer, transId_);
  return size;
}

void CmpMessageEnvs::unpackMyself(IpcMessageObjType objType,
				  IpcMessageObjVersion objVersion,
				  NABoolean sameEndianness,
				  IpcMessageObjSize objSize,
				  IpcConstMessageBufferPtr& buffer) 
{
  CmpMessageObj::unpackMyself(objType, objVersion, sameEndianness,
			      objSize, buffer);
  ::unpackBuffer(buffer, operator_);
  ::unpackBuffer(buffer, nEnvs_);
  if (nEnvs_)
  {
    envs_ = new(getHeap()) char*[nEnvs_];
    for (Int32 i=0; i < nEnvs_; i++) 
	{
	  envs_[i] = 0;
          unpackBuffer(buffer, envs_[i],getHeap());
	}
  }
  else
    envs_ = 0;

  unpackBuffer(buffer, cwd_, getHeap());
  ::unpackBuffer(buffer, activeTrans_);
  unpackBuffer(buffer, transId_);
}

void CmpMessageEnvs::destroyMe() 
{
}

CmpMessageEnvs::~CmpMessageEnvs() 
{
  destroyMe();
}

// -----------------------------------------------------------------------
// Methods for CmpMessageISPRequest
// -----------------------------------------------------------------------

CmpMessageISPRequest::CmpMessageISPRequest(char* procName,
					   void* inputExpr,
					   ULng32 inputExprSize,
					   void* outputExpr,
					   ULng32 outputExprSize,
					   void* keyExpr,
					   ULng32 keyExprSize,
					   void* inputData,
					   ULng32 inputDataSize,
					   ULng32 outputRowSize,
					   ULng32 outputTotalSize,
					   CollHeap* h,
                                           const char *parentQid,
                                           Lng32 parentQidLen)
  : CmpMessageRequest(INTERNALSP_REQUEST, NULL, 0, h, SQLCHARSETCODE_UNKNOWN, parentQid, parentQidLen)
{
  procName_ = procName;
  inputExpr_ = inputExpr;
  inputExprSize_ = inputExprSize;
  outputExpr_ = outputExpr;
  outputExprSize_ = outputExprSize;
  keyExpr_ = keyExpr;
  keyExprSize_ = keyExprSize;
  inputData_ = inputData;
  inputDataSize_ = inputDataSize;
  keyDataSize_  = 0;
  keyData_ = 0;
  outputRowSize_ = outputRowSize;
  outputTotalSize_ = outputTotalSize;
  allocated_ = FALSE;
}

CmpMessageISPRequest::~CmpMessageISPRequest()
{  
  destroyMe();  
}

void CmpMessageISPRequest::destroyMe() 
{
  if (allocated_)
    {
      NADELETEBASIC(procName_, getHeap());
      NADELETEBASIC((char*)inputExpr_, getHeap());
      NADELETEBASIC((char*)outputExpr_, getHeap());
      NADELETEBASIC((char*)keyExpr_, getHeap());
      NADELETEBASIC((char*)inputData_, getHeap());
    }
}

IpcMessageObjSize CmpMessageISPRequest::mypackedLength()
{
  IpcMessageObjSize size = CmpMessageRequest::mypackedLength();
  advanceSize(size, procName_);
  size += sizeof(inputExprSize_);
  advanceSize(size,inputExpr_,inputExprSize_);
  size += sizeof(outputExprSize_);
  advanceSize(size,outputExpr_,outputExprSize_);
  size += sizeof(keyExprSize_);
  advanceSize(size,keyExpr_,keyExprSize_);
  size += sizeof(inputDataSize_);
  advanceSize(size,inputData_,inputDataSize_);
  size += sizeof(outputRowSize_);
  size += sizeof(outputTotalSize_);
  
  return size;  
}

IpcMessageObjSize CmpMessageISPRequest::packMyself(IpcMessageBufferPtr& buffer)
{
  IpcMessageObjSize size= CmpMessageRequest::packMyself(buffer);
  size += packIntoBuffer(buffer, procName_);

  size += ::packIntoBuffer(buffer,inputExprSize_);
  size += packIntoBuffer(buffer, inputExpr_, inputExprSize_);
  size += ::packIntoBuffer(buffer,outputExprSize_);
  size += packIntoBuffer(buffer, outputExpr_, outputExprSize_);
  size += ::packIntoBuffer(buffer, keyExprSize_);
  size += packIntoBuffer(buffer, keyExpr_, keyExprSize_);
  size += ::packIntoBuffer(buffer,inputDataSize_);
  size += packIntoBuffer(buffer, inputData_, inputDataSize_);
  size += ::packIntoBuffer(buffer,outputRowSize_);
  size += ::packIntoBuffer(buffer,outputTotalSize_);

  return size;
}

void CmpMessageISPRequest::unpackMyself(IpcMessageObjType objType,
					IpcMessageObjVersion objVersion,
					NABoolean sameEndianness,
					IpcMessageObjSize objSize,
					IpcConstMessageBufferPtr& buffer)
{
  allocated_ = TRUE; // so that the allocated storages will be destroyed later.
  CmpMessageRequest::unpackMyself(objType, objVersion, sameEndianness,
				  objSize, buffer);
  unpackBuffer(buffer, procName_, getHeap());
  ::unpackBuffer(buffer, inputExprSize_);
  unpackBuffer(buffer, inputExpr_, getHeap());
  ::unpackBuffer(buffer, outputExprSize_);
  unpackBuffer(buffer, outputExpr_, getHeap());
  ::unpackBuffer(buffer, keyExprSize_);
  unpackBuffer(buffer, keyExpr_, getHeap());
  ::unpackBuffer(buffer, inputDataSize_);
  unpackBuffer(buffer, inputData_, getHeap());
  ::unpackBuffer(buffer, outputRowSize_);
  ::unpackBuffer(buffer, outputTotalSize_);
}

// -----------------------------------------------------------------------
// Methods for CmpMessageISPGetNext
// -----------------------------------------------------------------------

CmpMessageISPGetNext::CmpMessageISPGetNext(ULng32 bufSize,
					   ID ispRequest,
					   Lng32 serialNo,
					   CollHeap* h,
                                           const char *parentQid,
                                           Lng32 parentQidLen)
  : CmpMessageRequest(INTERNALSP_GETNEXT, NULL, 0, h, SQLCHARSETCODE_UNKNOWN, parentQid, parentQidLen)
{
  bufSize_ = bufSize;
  ispRequest_ = ispRequest;
  serialNo_ = serialNo;  
}

CmpMessageISPGetNext::~CmpMessageISPGetNext()
{  
}

IpcMessageObjSize CmpMessageISPGetNext::mypackedLength()
{
  IpcMessageObjSize size = CmpMessageRequest::mypackedLength();
  size += sizeof(bufSize_);
  size += sizeof(ispRequest_);
  size += sizeof(serialNo_);
  
  return size;  
}

IpcMessageObjSize CmpMessageISPGetNext::packMyself(IpcMessageBufferPtr& buffer)
{
  IpcMessageObjSize size= CmpMessageRequest::packMyself(buffer);

  size += ::packIntoBuffer(buffer,bufSize_);
  size += ::packIntoBuffer(buffer,ispRequest_);
  size += ::packIntoBuffer(buffer,serialNo_);

  return size;
}

void CmpMessageISPGetNext::unpackMyself(IpcMessageObjType objType,
					IpcMessageObjVersion objVersion,
					NABoolean sameEndianness,
					IpcMessageObjSize objSize,
					IpcConstMessageBufferPtr& buffer)
{
  CmpMessageRequest::unpackMyself(objType, objVersion, sameEndianness,
				  objSize, buffer);
  ::unpackBuffer(buffer, bufSize_);
  ::unpackBuffer(buffer, ispRequest_);
  ::unpackBuffer(buffer, serialNo_);
  
}

IpcMessageObjSize CmpMessageReplyISP::mypackedLength()
{
  IpcMessageObjSize size = CmpMessageReply::mypackedLength();
  size += sizeof(areMore_);
  
  return size;  
}

IpcMessageObjSize CmpMessageReplyISP::packMyself(IpcMessageBufferPtr& buffer)
{
  IpcMessageObjSize size= CmpMessageReply::packMyself(buffer);

  size += ::packIntoBuffer(buffer,areMore_);

  return size;
}

void CmpMessageReplyISP::unpackMyself(IpcMessageObjType objType,
                                      IpcMessageObjVersion objVersion,
                                      NABoolean sameEndianness,
                                      IpcMessageObjSize objSize,
                                      IpcConstMessageBufferPtr& buffer)
{
  CmpMessageReply::unpackMyself(objType, objVersion, sameEndianness,
    objSize, buffer);
  ::unpackBuffer(buffer, areMore_);
}


// -----------------------------------------------------------------------
// Methods for CmpMessageConnectionType
// -----------------------------------------------------------------------

CmpMessageConnectionType::CmpMessageConnectionType(ConnectionTypeEnum t,
                                                   CollHeap* h)
  : CmpMessageRequestBasic(CONNECTION_TYPE, h)
{
  type_ = t;
}

IpcMessageObjSize CmpMessageConnectionType::mypackedLength()
{
  IpcMessageObjSize size = CmpMessageRequestBasic::mypackedLength();
  size += sizeof(Lng32);
  
  return size;  
}

IpcMessageObjSize CmpMessageConnectionType::packMyself(IpcMessageBufferPtr& buffer)
{
  IpcMessageObjSize size= CmpMessageRequestBasic::packMyself(buffer);

  Lng32 t = (Lng32) type_;
  size += ::packIntoBuffer(buffer,t);

  return size;
}

void CmpMessageConnectionType::unpackMyself(IpcMessageObjType objType,
                                            IpcMessageObjVersion objVersion,
                                            NABoolean sameEndianness,
                                            IpcMessageObjSize objSize,
                                            IpcConstMessageBufferPtr& buffer)
{
  CmpMessageRequestBasic::unpackMyself(objType, objVersion, sameEndianness,
				       objSize, buffer);
  Lng32 t;
  ::unpackBuffer(buffer, t);
  type_ = (( t==DMLDDL ) ? DMLDDL : ISP);
}

IpcMessageObjSize CmpMessageEndSession::mypackedLength()
{
  IpcMessageObjSize size = CmpMessageObj::mypackedLength();
  size += sizeof(flags_);
  return size;  
}

IpcMessageObjSize CmpMessageEndSession::packMyself(IpcMessageBufferPtr& buffer)
{
  IpcMessageObjSize size= CmpMessageObj::packMyself(buffer);
  size += ::packIntoBuffer(buffer, flags_);
  return size;
}

void CmpMessageEndSession::unpackMyself(IpcMessageObjType objType,
					IpcMessageObjVersion objVersion,
					NABoolean sameEndianness,
					IpcMessageObjSize objSize,
					IpcConstMessageBufferPtr& buffer) 
{
  CmpMessageObj::unpackMyself(objType, objVersion, sameEndianness,
			      objSize, buffer);
  ::unpackBuffer(buffer, flags_);
}





