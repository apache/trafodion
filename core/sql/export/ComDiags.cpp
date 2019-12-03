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
* File:         ComDiags.cpp (previously under /common)
* Description:
*
* Created:      5/6/98
* Language:     C++
*
*
****************************************************************************
*/

#include "Platform.h"



#include "NAStdlib.h"

#include "ComDiags.h"
#include "IpcMessageObj.h"
#include "str.h"
#include "Int64.h"
#include "ExpError.h"

#include "seabed/ms.h"
#include <stdlib.h>
#include <unistd.h>
extern void releaseRTSSemaphore();  // Functions implemented in SqlStats.cpp
#include "logmxevent.h"

#include <byteswap.h>

#include "ComRtUtils.h"

#ifdef _DEBUG
#include <time.h>
#include <sys/time.h>
#include "PortProcessCalls.h"
#endif


// This is a "helper" function that factors out a bunch of code
// from the packedLength() routines.

static
inline void advanceSize(IpcMessageObjSize &size, const char * const buffPtr)
{
  const Int32 lenSize = sizeof(  Lng32);
  size += lenSize;
  if (buffPtr != NULL)
    size += str_len(buffPtr) + 1; // 1 is for the buffer's null-terminator
}

//UR2
static
inline void advanceSize(IpcMessageObjSize &size, const NAWchar * const buffPtr)
{
  const Int32 lenSize = sizeof(  Lng32);  
  size += lenSize;
  if (buffPtr != NULL)
    size += (na_wcslen(buffPtr) + 1)*sizeof(NAWchar); // 1 is for null-terminator
}



// static NABoolean isValidIsoMappingCharSet(CharInfo::CharSet cs)
// {
//   if (cs == CharInfo::ISO88591 ||
//       cs == CharInfo::SJIS     ||
//       cs == CharInfo::UTF8)
//     return TRUE;
//   else
//     return FALSE;
// }

static NABoolean isSingleByteCharSet(CharInfo::CharSet cs)
{
  if (cs == CharInfo::ISO88591) return TRUE;
  if (cs == CharInfo::UTF8) return TRUE; // is variable-length/width multi-byte char-set but treat it as a C/C++ string
  if (cs == CharInfo::SJIS) return TRUE; // is variable-length/width multi-byte char-set but treat it as a C/C++ string
  if (cs == CharInfo::UNICODE)  return FALSE;

  // a "mini-cache" to avoid proc call, for performance.
  static THREAD_P CharInfo::CharSet cachedCS    = CharInfo::UnknownCharSet;
  static THREAD_P Int32               cachedSByte = TRUE;

  if (cachedCS != cs) {
    cachedCS = cs;
    cachedSByte = (CharInfo::maxBytesPerChar(cs) == 1);
  }
  return cachedSByte;
}


// The type of message is IPC_SQL_DIAG_AREA, and the version
// is hard coded to zero here.
//
// What should the version of IpcMessageObj be set to?

ComCondition::ComCondition (CollHeap* heapPtr) :
                                         IpcMessageObj(IPC_SQL_CONDITION,0),
                                         serverName_(NULL),
                                         connectionName_(NULL),
                                         constraintCatalog_(NULL),
                                         constraintSchema_(NULL),
                                         constraintName_(NULL),
                                         triggerCatalog_(NULL),
                                         triggerSchema_(NULL),
                                         triggerName_(NULL),
                                         catalogName_(NULL),
                                         schemaName_(NULL),
                                         tableName_(NULL),
					 customSQLState_(NULL),
                                         columnName_(NULL),
                                         sqlID_(NULL),
                                         messageText_(NULL),
                                         messageLen_(0),
                                         conditionNumber_(0),
					 usageMap_(0),
                                         rowNumber_(ComCondition::INVALID_ROWNUMBER),
					 nskCode_(0),
					 numStringParamsUsed_(0),
					 numIntParamsUsed_(0),
					 flagsTBS_(0),
                                         theSQLCODE_(0),
                                         emsEventVisits_(0),
                                         // iso88591MappingCharSet_(CharInfo::UnknownCharSet),
                                         collHeapPtr_(heapPtr),
                                         isLocked_(FALSE)
{
   // Set optional string parameters to NULL
   // and optional integers to a theoretically recognizably bad value.
   for (Int32 i=NumOptionalParms; i--; ) {
      optionalString_[i]=NULL;
      optionalStringCharSet_[i]=CharInfo::UTF8;
      optionalInteger_[i]=ComDiags_UnInitialized_Int;
   }
   // Initialize fillers space to 0
   memset(fillers_, 0, sizeof(fillers_));
   // Make sure the size of ComCondition remains constant
   // If you hit this after change or add new member, adjust the fillers_ size
   Int32 classSize = sizeof(ComCondition);
   assert(classSize == 376);
}

ComCondition::ComCondition () :
                                         IpcMessageObj(IPC_SQL_CONDITION,0),
                                         serverName_(NULL),
                                         connectionName_(NULL),
                                         constraintCatalog_(NULL),
                                         constraintSchema_(NULL),
                                         constraintName_(NULL),
                                         triggerCatalog_(NULL),
                                         triggerSchema_(NULL),
                                         triggerName_(NULL),
                                         catalogName_(NULL),
                                         schemaName_(NULL),
                                         tableName_(NULL),
					 customSQLState_(NULL),
                                         columnName_(NULL),
                                         sqlID_(NULL),
                                         messageText_(NULL),
                                         messageLen_(0),
                                         conditionNumber_(0),
					 usageMap_(0),
                                         rowNumber_(ComCondition::INVALID_ROWNUMBER),
					 nskCode_(0),
					 numStringParamsUsed_(0),
					 numIntParamsUsed_(0),
					 flagsTBS_(0),
                                         theSQLCODE_(0),
                                         emsEventVisits_(0),
                                         // iso88591MappingCharSet_(CharInfo::UnknownCharSet),
                                         collHeapPtr_(NULL),
                                         isLocked_(FALSE)
{
   // Set optional string parameters to NULL
   // and optional integers to a theoretically recognizably bad value.
   for (Int32 i=NumOptionalParms; i--; ) {
      optionalString_[i]=NULL;
      optionalStringCharSet_[i]=CharInfo::UTF8;
      optionalInteger_[i]=ComDiags_UnInitialized_Int;
   }
   // Initialize fillers space to 0
   memset(fillers_, 0, sizeof(fillers_));
   // Make sure the size of ComCondition remains constant
   // If you hit this after change or add new member, adjust the fillers_ size
   Int32 classSize = sizeof(ComCondition);
   assert(classSize == 376);
}

// The destructor must free all of the char buffers which
// this object owns, and it must do so using the proper heap.

ComCondition::~ComCondition ()
{
   clear();
   collHeapPtr_=NULL;
}

// The assignment operator pretty much just copies each of
// the members over ``by hand.''

ComCondition& ComCondition::operator=(const ComCondition& c)
{
    this->IpcMessageObj::operator=(c);

    assignStringMember(serverName_,         c.serverName_);
    assignStringMember(connectionName_,     c.connectionName_);
    assignStringMember(constraintCatalog_,  c.constraintCatalog_);
    assignStringMember(constraintSchema_,   c.constraintSchema_);
    assignStringMember(constraintName_,     c.constraintName_);
    assignStringMember(triggerCatalog_,     c.triggerCatalog_);
    assignStringMember(triggerSchema_,      c.triggerSchema_);
    assignStringMember(triggerName_,        c.triggerName_);
    assignStringMember(catalogName_,        c.catalogName_);
    assignStringMember(schemaName_,         c.schemaName_);
    assignStringMember(tableName_,          c.tableName_);
    assignStringMember(customSQLState_,     c.customSQLState_);
    assignStringMember(columnName_,         c.columnName_);
    assignStringMember(sqlID_,              c.sqlID_);
    assignStringMember(messageText_,        c.messageText_);

    for (Int32 i=NumOptionalParms; i--; ) {
      if ( isSingleByteCharSet(c.optionalStringCharSet_[i]) )
         assignStringMember((char* &)optionalString_[i], (char*)c.optionalString_[i]);
      else
         assignStringMember((NAWchar*&)optionalString_[i], (NAWchar*)c.optionalString_[i]);

      optionalStringCharSet_[i] = c.optionalStringCharSet_[i];
      optionalInteger_[i] = c.optionalInteger_[i];
    }

                messageLen_ = c.messageLen_;
//           conditionNumber_ = c.conditionNumber_;
                  usageMap_ = c.usageMap_;
                 rowNumber_ = c.rowNumber_;
		   nskCode_ = c.nskCode_;
       numStringParamsUsed_ = c.numStringParamsUsed_;
	  numIntParamsUsed_ = c.numIntParamsUsed_;
                  flagsTBS_ = c.flagsTBS_;
                theSQLCODE_ = c.theSQLCODE_;
                  isLocked_ = c.isLocked_;
            emsEventVisits_ = c.emsEventVisits_;
            // iso88591MappingCharSet_ = c.iso88591MappingCharSet_;

    return *this;
}

// The clear function resets this ComCondition object to what it
// would be just after construction, EXCEPT, that the collHeapPtr_
// member is left as is.

void ComCondition::clear()
{
   if (collHeapPtr_==NULL) {

      // We delete the individual buffers, and then the elements of
      // the array of the optional string parameters.
      delete [] serverName_;
      delete [] connectionName_;
      delete [] constraintCatalog_;
      delete [] constraintSchema_;
      delete [] constraintName_;
      delete [] triggerCatalog_;
      delete [] triggerSchema_;
      delete [] triggerName_;
      delete [] catalogName_;
      delete [] schemaName_;
      delete [] tableName_;
      delete [] customSQLState_;
      delete [] columnName_;
      delete [] sqlID_;
      delete [] messageText_;

      for (Int32 i=NumOptionalParms; i--; ) {
        if (optionalString_[i])
          if ( isSingleByteCharSet(optionalStringCharSet_[i]) )
	    delete (char*)optionalString_[i];
          else
	    delete (NAWchar*)optionalString_[i];
      }
   }
   else {

     // Deallocate space for all buffers: these are simple char arrays
     // so there is no destructor to call.

     collHeapPtr_->deallocateMemory(serverName_);
     collHeapPtr_->deallocateMemory(connectionName_);
     collHeapPtr_->deallocateMemory(constraintCatalog_);
     collHeapPtr_->deallocateMemory(constraintSchema_);
     collHeapPtr_->deallocateMemory(constraintName_);
     collHeapPtr_->deallocateMemory(triggerCatalog_);
     collHeapPtr_->deallocateMemory(triggerSchema_);
     collHeapPtr_->deallocateMemory(triggerName_);
     collHeapPtr_->deallocateMemory(catalogName_);
     collHeapPtr_->deallocateMemory(schemaName_);
     collHeapPtr_->deallocateMemory(tableName_);
     collHeapPtr_->deallocateMemory(customSQLState_);
     collHeapPtr_->deallocateMemory(columnName_);
     collHeapPtr_->deallocateMemory(sqlID_);
     collHeapPtr_->deallocateMemory(messageText_);

     // Deallocate space for optional string parameters: no
     // destructor need be called.

     for (Int32 i=NumOptionalParms; i--; ) {
       if (optionalString_[i])
	 if ( isSingleByteCharSet(optionalStringCharSet_[i]) )
	    collHeapPtr_->deallocateMemory(optionalString_[i]);
	 else
	    collHeapPtr_->deallocateMemory((NAWchar*)optionalString_[i]);
     }
   }

   // Set pointers to NULL for safety, in case anybody's using
   // these pointers and shouldn't be.

   serverName_ = NULL;
   connectionName_ = NULL;
   constraintCatalog_ = NULL;
   constraintSchema_ = NULL;
   constraintName_ = NULL;
   triggerCatalog_ = NULL;
   triggerSchema_ = NULL;
   triggerName_ = NULL;
   catalogName_ = NULL;
   schemaName_ = NULL;
   tableName_ = NULL;
   customSQLState_ = NULL;
   columnName_ = NULL;
   sqlID_ = NULL;
   messageText_ = NULL;

   // set optional string parameters to NULL
   // and optional integers to a theoretically recognizably bad value.

   for (Int32 i=NumOptionalParms; i--; ) {
     optionalString_[i]=NULL;
     optionalStringCharSet_[i]=CharInfo::UTF8;
     optionalInteger_[i]=ComDiags_UnInitialized_Int;
   }

   messageLen_ = 0;
   rowNumber_ = INVALID_ROWNUMBER;
   nskCode_ = 0;
   flagsTBS_ = 0;
   conditionNumber_ = 0;
   usageMap_ = 0;
   theSQLCODE_ = 0;
   isLocked_ = FALSE;
   emsEventVisits_ = 0;
   // iso88591MappingCharSet_ = CharInfo::UnknownCharSet;

   // Reset the filler_ space to 0
   memset(fillers_, 0, sizeof(fillers_));
}

IpcMessageRefCount ComDiagsArea::decrRefCount()
{
  if (getRefCount() == 1)
    {
      deAllocate();
      return 0;
    }

  // Let base class do the work.
  return this->IpcMessageObj::decrRefCount();
}

// These IPC functions are definitions for virtual functions.
// We do not currently make (any) use of most of the
// arguments of unpackObj().

void ComCondition::unpackObj(IpcMessageObjType objType,
			     IpcMessageObjVersion objVersion,
			     NABoolean sameEndianness,
			     IpcMessageObjSize objSize,
			     IpcConstMessageBufferPtr buffer)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength()
  // - packObjIntoMessage()
  // - unpackObj()
  // - checkObj()

  NABoolean foundUprevFieldsInBuffer = FALSE;

  unpackBaseClass(buffer);
  // unconditional fields
  unpackBuffer(buffer,conditionNumber_);
  unpackBuffer(buffer,usageMap_);
  unpackBuffer(buffer,emsEventVisits_);

  // fields that are sent conditionally
  if (usageMap_ & USED_FLAGS)
    unpackBuffer(buffer,flagsTBS_);
  if (usageMap_ & USED_SQLCODE)
    unpackBuffer(buffer,theSQLCODE_);
  else
    assert(0); // should never have a missing SQLCODE field
  if (usageMap_ & USED_SERVER_NAME)
    unpackBuffer(buffer,serverName_,collHeapPtr_);
  if (usageMap_ & USED_CONNECTION_NAME)
    unpackBuffer(buffer,connectionName_,collHeapPtr_);
  if (usageMap_ & USED_CONSTRAINT_CATALOG)
    unpackBuffer(buffer,constraintCatalog_,collHeapPtr_);
  if (usageMap_ & USED_CONSTRAINT_SCHEMA)
    unpackBuffer(buffer,constraintSchema_,collHeapPtr_);
  if (usageMap_ & USED_CONSTRAINT_NAME)
    unpackBuffer(buffer,constraintName_,collHeapPtr_);
  if (usageMap_ & USED_TRIGGER_CATALOG)
    unpackBuffer(buffer,triggerCatalog_,collHeapPtr_);
  if (usageMap_ & USED_TRIGGER_SCHEMA)
    unpackBuffer(buffer,triggerSchema_,collHeapPtr_);
  if (usageMap_ & USED_TRIGGER_NAME)
    unpackBuffer(buffer,triggerName_,collHeapPtr_);
  if (usageMap_ & USED_CATALOG_NAME)
    unpackBuffer(buffer,catalogName_,collHeapPtr_);
  if (usageMap_ & USED_SCHEMA_NAME)
    unpackBuffer(buffer,schemaName_,collHeapPtr_);
  if (usageMap_ & USED_TABLE_NAME)
    unpackBuffer(buffer,tableName_,collHeapPtr_);
  if (usageMap_ & USED_COLUMN_NAME)
    unpackBuffer(buffer,columnName_,collHeapPtr_);
  if (usageMap_ & USED_SQLID)
    unpackBuffer(buffer,sqlID_,collHeapPtr_);
  if (usageMap_ & USED_ROW_NUMBER)
    unpackBuffer(buffer,rowNumber_);
  if (usageMap_ & USED_NSK_CODE)
    unpackBuffer(buffer,nskCode_);
  if (usageMap_ & USED_CUSTOM_SQLSTATE)
    unpackBuffer(buffer,customSQLState_,collHeapPtr_);
  // if (usageMap_ & USED_ISO_MAPPING_CHARSET)
  //   unpackBuffer(buffer,iso88591MappingCharSet_);

  if (usageMap_ & USED_NUM_STRING_PARAMS)
    {
      Lng32 charSet;

      unpackBuffer(buffer,numStringParamsUsed_);

      for (Int32 i=0; i < numStringParamsUsed_; i++)
	{
	  if (i < NumOptionalParms)
	    {
	      unpackBuffer(buffer, (char*&)optionalString_[i],collHeapPtr_);
	  
	      unpackBuffer(buffer,charSet);
	      optionalStringCharSet_[i] = (CharInfo::CharSet)charSet;
	    }
	  else
	    {
	      // Some future release might increase NumOptionalParms.
	      // In this case, ignore those additional parameters
	      // and set a flag indicating this.
	      skipCharStarInBuffer(buffer);
	      unpackBuffer(buffer,charSet);
	      foundUprevFieldsInBuffer = TRUE;
	    }
	}
    }

  if (usageMap_ & USED_NUM_INT_PARAMS)
    {
      Lng32 dummy;

      unpackBuffer(buffer,numIntParamsUsed_);

      for (Int32 i=0; i < numIntParamsUsed_; i++)
	{
	  if (i < NumOptionalParms)
	    {
	      unpackBuffer(buffer,optionalInteger_[i]);
	    }
	  else
	    {
	      // Some future release might increase NumOptionalParms.
	      // In this case, ignore those additional parameters
	      // and set a flag indicating this.
	      unpackBuffer(buffer,dummy);
	      foundUprevFieldsInBuffer = TRUE;
	    }
	}
    }

  // Check whether there are more fields than we expect. For certain
  // simple additions, we will not increase the version number of
  // the diagnostics area or of the ComCondition object.
  if ((usageMap_ & USED_FUTURE_FIELDS) || foundUprevFieldsInBuffer)
    flagsTBS_ |= FLAGS_TBS_SUPPRESSED_UPREV_FIELDS;
}

// These IPC functions are definitions for virtual functions.
// We do not currently make (any) use of most of the
// arguments of unpackObj().

void ComCondition::unpackObj32(IpcMessageObjType objType,
			       IpcMessageObjVersion objVersion,
			       NABoolean sameEndianness,
			       IpcMessageObjSize objSize,
			       IpcConstMessageBufferPtr buffer)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength()
  // - packObjIntoMessage()
  // - unpackObj()
  // - checkObj()

  NABoolean foundUprevFieldsInBuffer = FALSE;

  unpackBaseClass32(buffer);
  // unconditional fields
  unpackBuffer(buffer,conditionNumber_);
  unpackBuffer(buffer,usageMap_);
  unpackBuffer(buffer,emsEventVisits_);

  // fields that are sent conditionally
  if (usageMap_ & USED_FLAGS)
    unpackBuffer(buffer,flagsTBS_);
  if (usageMap_ & USED_SQLCODE)
    unpackBuffer(buffer,theSQLCODE_);
  else
    assert(0); // should never have a missing SQLCODE field
  if (usageMap_ & USED_SERVER_NAME)
    unpackBuffer(buffer,serverName_,collHeapPtr_);
  if (usageMap_ & USED_CONNECTION_NAME)
    unpackBuffer(buffer,connectionName_,collHeapPtr_);
  if (usageMap_ & USED_CONSTRAINT_CATALOG)
    unpackBuffer(buffer,constraintCatalog_,collHeapPtr_);
  if (usageMap_ & USED_CONSTRAINT_SCHEMA)
    unpackBuffer(buffer,constraintSchema_,collHeapPtr_);
  if (usageMap_ & USED_CONSTRAINT_NAME)
    unpackBuffer(buffer,constraintName_,collHeapPtr_);
  if (usageMap_ & USED_TRIGGER_CATALOG)
    unpackBuffer(buffer,triggerCatalog_,collHeapPtr_);
  if (usageMap_ & USED_TRIGGER_SCHEMA)
    unpackBuffer(buffer,triggerSchema_,collHeapPtr_);
  if (usageMap_ & USED_TRIGGER_NAME)
    unpackBuffer(buffer,triggerName_,collHeapPtr_);
  if (usageMap_ & USED_CATALOG_NAME)
    unpackBuffer(buffer,catalogName_,collHeapPtr_);
  if (usageMap_ & USED_SCHEMA_NAME)
    unpackBuffer(buffer,schemaName_,collHeapPtr_);
  if (usageMap_ & USED_TABLE_NAME)
    unpackBuffer(buffer,tableName_,collHeapPtr_);
  if (usageMap_ & USED_COLUMN_NAME)
    unpackBuffer(buffer,columnName_,collHeapPtr_);
  if (usageMap_ & USED_SQLID)
    unpackBuffer(buffer,sqlID_,collHeapPtr_);
  if (usageMap_ & USED_ROW_NUMBER)
    unpackBuffer(buffer,rowNumber_);
  if (usageMap_ & USED_NSK_CODE)
    unpackBuffer(buffer,nskCode_);
  if (usageMap_ & USED_CUSTOM_SQLSTATE)
    unpackBuffer(buffer,customSQLState_,collHeapPtr_);
  // if (usageMap_ & USED_ISO_MAPPING_CHARSET)
  //   unpackBuffer(buffer,iso88591MappingCharSet_);

  if (usageMap_ & USED_NUM_STRING_PARAMS)
    {
      Lng32 charSet;

      unpackBuffer(buffer,numStringParamsUsed_);

      for (Int32 i=0; i < numStringParamsUsed_; i++)
	{
	  if (i < NumOptionalParms)
	    {
	      unpackBuffer(buffer, (char*&)optionalString_[i],collHeapPtr_);
	  
	      unpackBuffer(buffer,charSet);
	      optionalStringCharSet_[i] = (CharInfo::CharSet)charSet;
	    }
	  else
	    {
	      // Some future release might increase NumOptionalParms.
	      // In this case, ignore those additional parameters
	      // and set a flag indicating this.
	      skipCharStarInBuffer(buffer);
	      unpackBuffer(buffer,charSet);
	      foundUprevFieldsInBuffer = TRUE;
	    }
	}
    }

  if (usageMap_ & USED_NUM_INT_PARAMS)
    {
      Lng32 dummy;

      unpackBuffer(buffer,numIntParamsUsed_);

      for (Int32 i=0; i < numIntParamsUsed_; i++)
	{
	  if (i < NumOptionalParms)
	    {
	      unpackBuffer(buffer,optionalInteger_[i]);
	    }
	  else
	    {
	      // Some future release might increase NumOptionalParms.
	      // In this case, ignore those additional parameters
	      // and set a flag indicating this.
	      unpackBuffer(buffer,dummy);
	      foundUprevFieldsInBuffer = TRUE;
	    }
	}
    }

  // Check whether there are more fields than we expect. For certain
  // simple additions, we will not increase the version number of
  // the diagnostics area or of the ComCondition object.
  if ((usageMap_ & USED_FUTURE_FIELDS) || foundUprevFieldsInBuffer)
    flagsTBS_ |= FLAGS_TBS_SUPPRESSED_UPREV_FIELDS;
}

// So for each
// integer, we take its size.  For each string, we add sizeof(long)
// to the length of the string plus 1 (except in the case of empty
// strings, that plus 1 is omitted, since NULL strings are considered
// empty and so have no buffers).
//
// Note that "long" is the type we use within the buffer
// to represent the length of a null-terminted buffer array.
//
// collHeapPtr_ is never passed via IPC; it gets setup in the constructor
// and the IPC unpack method fills in the other data using the heap
// specified in collHeapPtr_ to allocate string buffers as needed.

IpcMessageObjSize ComCondition::packedLength(void)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength()
  // - packObjIntoMessage()
  // - unpackObj()
  // - checkObj()

  IpcMessageObjSize  size=baseClassPackedLength();

  // for now we set the usageMap_ flags here, but they
  // could also be set in the accessor methods. This fields is needed
  // in method packObjIntoMessage()
  usageMap_ = 0;

  // the first two fields get sent unconditionally
  size += sizeof(conditionNumber_);
  size += sizeof(usageMap_);
  size += sizeof(emsEventVisits_);

  // all other fields get sent only when needed
  if (flagsTBS_)
    {
      size += sizeof(flagsTBS_);
      usageMap_ += USED_FLAGS;
    }
  if (theSQLCODE_)
    {
      size += sizeof(theSQLCODE_);
      usageMap_ |= USED_SQLCODE;
    }
  if (serverName_)
    {
      advanceSize(size,serverName_);
      usageMap_ |= USED_SERVER_NAME;
    }
  if (connectionName_)
    {
      advanceSize(size,connectionName_);
      usageMap_ |= USED_CONNECTION_NAME;
    }
  if (constraintCatalog_)
    {
      advanceSize(size,constraintCatalog_);
      usageMap_ |= USED_CONSTRAINT_CATALOG;
    }
  if (constraintSchema_)
    {
      advanceSize(size,constraintSchema_);
      usageMap_ |= USED_CONSTRAINT_SCHEMA;
    }
  if (constraintName_)
    {
      advanceSize(size,constraintName_);
      usageMap_ |= USED_CONSTRAINT_NAME;
    }
  if (triggerCatalog_)
    {
      advanceSize(size,triggerCatalog_);
      usageMap_ |= USED_TRIGGER_CATALOG;
    }
  if (triggerSchema_)
    {
      advanceSize(size,triggerSchema_);
      usageMap_ |= USED_TRIGGER_SCHEMA;
    }
  if (triggerName_)
    {
      advanceSize(size,triggerName_);
      usageMap_ |= USED_TRIGGER_NAME;
    }
  if (catalogName_)
    {
      advanceSize(size,catalogName_);
      usageMap_ |= USED_CATALOG_NAME;
    }
  if (schemaName_)
    {
      advanceSize(size,schemaName_);
      usageMap_ |= USED_SCHEMA_NAME;
    }
  if (tableName_)
    {
      advanceSize(size,tableName_);
      usageMap_ |= USED_TABLE_NAME;
    }
  if (columnName_)
    {
      advanceSize(size,columnName_);
      usageMap_ |= USED_COLUMN_NAME;
    }
  if (sqlID_)
    {
      advanceSize(size,sqlID_);
      usageMap_ |= USED_SQLID;
    }
  if (rowNumber_ != INVALID_ROWNUMBER)
    {
      size += sizeof(rowNumber_);
      usageMap_ |= USED_ROW_NUMBER;
    }
  if (nskCode_)
    {
      size += sizeof(nskCode_);
      usageMap_ += USED_NSK_CODE;
    }
  if (customSQLState_)
  {
    advanceSize(size,customSQLState_);
    usageMap_ |= USED_CUSTOM_SQLSTATE;
  }
//   if (isValidIsoMappingCharSet(iso88591MappingCharSet_))
//   {
//     size += sizeof(iso88591MappingCharSet_);
//     usageMap_ |= USED_ISO_MAPPING_CHARSET;
//   }
  
  // set these two variables for later use in packObjIntoMessage()
  numStringParamsUsed_  = 0;
  numIntParamsUsed_ = 0;
  for (Int32 i=NumOptionalParms; i--; )
    {
      if (optionalString_[i] && numStringParamsUsed_ == 0)
	{
	  // found the last string parameter used
	  numStringParamsUsed_ = i+1;
	  usageMap_ |= USED_NUM_STRING_PARAMS;
	  size += sizeof(numStringParamsUsed_);
	}
      if (optionalInteger_[i] != ComDiags_UnInitialized_Int &&
	  numIntParamsUsed_ == 0)
	{
	  // found the last integer parameter used
	  numIntParamsUsed_ = i+1;
	  usageMap_ |= USED_NUM_INT_PARAMS;
	  size += sizeof(numIntParamsUsed_);
	}

      if (numStringParamsUsed_ > i)
	{
	  // this string parameter needs to be sent
	  if ( isSingleByteCharSet(optionalStringCharSet_[i]) )
	    advanceSize(size,(char*)optionalString_[i]);
	  else
	    advanceSize(size,(NAWchar*)optionalString_[i]);
	  size += sizeof(Lng32); // for optionalStringCharSet_[];
	}
      if (numIntParamsUsed_ > i)
	{
	  // this integer parameter needs to be sent
	  size += sizeof(optionalInteger_[0]);
	}
    }

  return size;
}

IpcMessageObjSize ComCondition::packedLength32(void)
{
  IpcMessageObjSize size = packedLength();
  // packed items should not include Longs or pointers
  return size - (baseClassPackedLength() - baseClassPackedLength32());
}

IpcMessageObjSize ComCondition::packObjIntoMessage(char* buffer)
{
  return packObjIntoMessage(buffer, FALSE);
}

// Let us observe consistency (and thereby, hopefully, simplicity) by
// packing this object following the same order of visiting member data
// as we did in the packedLength() member function above.

IpcMessageObjSize ComCondition::packObjIntoMessage(char* buffer,
                          NABoolean swapBytes)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength()
  // - packObjIntoMessage()
  // - unpackObj()
  // - checkObj()

  IpcMessageObjSize  size=packBaseClassIntoMessage(buffer, swapBytes);

  // should have been set by packedLength()
  assert(usageMap_);

  // pack some fields unconditionally
  size += packIntoBuffer(buffer,conditionNumber_, swapBytes);
  size += packIntoBuffer(buffer,usageMap_, swapBytes);
  size += packIntoBuffer(buffer,emsEventVisits_, swapBytes);

  // fields packed conditionally
  if (usageMap_ & USED_FLAGS)
    size += packIntoBuffer(buffer,flagsTBS_, swapBytes);
  if (usageMap_ & USED_SQLCODE)
    size += packIntoBuffer(buffer,theSQLCODE_, swapBytes);
  if (usageMap_ & USED_SERVER_NAME)
    size += packCharStarIntoBuffer(buffer,serverName_, swapBytes);
  if (usageMap_ & USED_CONNECTION_NAME)
    size += packCharStarIntoBuffer(buffer,connectionName_, swapBytes);
  if (usageMap_ & USED_CONSTRAINT_CATALOG)
    size += packCharStarIntoBuffer(buffer,constraintCatalog_, swapBytes);
  if (usageMap_ & USED_CONSTRAINT_SCHEMA)
    size += packCharStarIntoBuffer(buffer,constraintSchema_, swapBytes);
  if (usageMap_ & USED_CONSTRAINT_NAME)
    size += packCharStarIntoBuffer(buffer,constraintName_, swapBytes);
  if (usageMap_ & USED_TRIGGER_CATALOG)
    size += packCharStarIntoBuffer(buffer,triggerCatalog_, swapBytes);
  if (usageMap_ & USED_TRIGGER_SCHEMA)
    size += packCharStarIntoBuffer(buffer,triggerSchema_, swapBytes);
  if (usageMap_ & USED_TRIGGER_NAME)
    size += packCharStarIntoBuffer(buffer,triggerName_, swapBytes);
  if (usageMap_ & USED_CATALOG_NAME)
    size += packCharStarIntoBuffer(buffer,catalogName_, swapBytes);
  if (usageMap_ & USED_SCHEMA_NAME)
    size += packCharStarIntoBuffer(buffer,schemaName_, swapBytes);
  if (usageMap_ & USED_TABLE_NAME)
    size += packCharStarIntoBuffer(buffer,tableName_, swapBytes);
  if (usageMap_ & USED_COLUMN_NAME)
    size += packCharStarIntoBuffer(buffer,columnName_, swapBytes);
  if (usageMap_ & USED_SQLID)
    size += packCharStarIntoBuffer(buffer,sqlID_, swapBytes);
  if (usageMap_ & USED_ROW_NUMBER)
    size += packIntoBuffer(buffer,rowNumber_, swapBytes);
  if (usageMap_ & USED_NSK_CODE)
    size += packIntoBuffer(buffer,nskCode_, swapBytes);
  if (usageMap_ & USED_CUSTOM_SQLSTATE)
    size += packCharStarIntoBuffer(buffer,customSQLState_, swapBytes);
  // if (usageMap_ & USED_ISO_MAPPING_CHARSET)
  //   size += packIntoBuffer(buffer,iso88591MappingCharSet_);
 
  // pack string parameters up to the last used one
  if (usageMap_ & USED_NUM_STRING_PARAMS)
  {
      size += packIntoBuffer(buffer,numStringParamsUsed_, swapBytes);

      for (Int32 i=0; i < numStringParamsUsed_; i++)
	{
	  if ( isSingleByteCharSet(optionalStringCharSet_[i]) )
	    size += packCharStarIntoBuffer(buffer,
                      (char*)optionalString_[i], swapBytes);
	  else
	    size += packCharStarIntoBuffer(buffer,
			   (NAWchar*)optionalString_[i], swapBytes);
	  
	  size += packIntoBuffer(buffer,(Lng32)optionalStringCharSet_[i], swapBytes);
	}
    }

  // pack integer parameters up to the last used one
  if (usageMap_ & USED_NUM_INT_PARAMS)
    {
      size += packIntoBuffer(buffer, numIntParamsUsed_, swapBytes);
      for (Int32 i=0; i < numIntParamsUsed_; i++)
	{
	  size += packIntoBuffer(buffer,optionalInteger_[i], swapBytes);
	}
    }

  return size;
}

IpcMessageObjSize ComCondition::packObjIntoMessage32(char* buffer)
{
  return packObjIntoMessage32(buffer, FALSE);
}

// Let us observe consistency (and thereby, hopefully, simplicity) by
// packing this object following the same order of visiting member data
// as we did in the packedLength() member function above.

IpcMessageObjSize ComCondition::packObjIntoMessage32(char* buffer,
                          NABoolean swapBytes)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength32()
  // - packObjIntoMessage32()
  // - unpackObj()
  // - checkObj()

  IpcMessageObjSize  size=packBaseClassIntoMessage32(buffer, swapBytes);

  // should have been set by packedLength()
  assert(usageMap_);

  // pack some fields unconditionally
  size += packIntoBuffer(buffer,conditionNumber_, swapBytes);
  size += packIntoBuffer(buffer,usageMap_, swapBytes);
  size += packIntoBuffer(buffer,emsEventVisits_, swapBytes);

  // fields packed conditionally
  if (usageMap_ & USED_FLAGS)
    size += packIntoBuffer(buffer,flagsTBS_, swapBytes);
  if (usageMap_ & USED_SQLCODE)
    size += packIntoBuffer(buffer,theSQLCODE_, swapBytes);
  if (usageMap_ & USED_SERVER_NAME)
    size += packCharStarIntoBuffer(buffer,serverName_, swapBytes);
  if (usageMap_ & USED_CONNECTION_NAME)
    size += packCharStarIntoBuffer(buffer,connectionName_, swapBytes);
  if (usageMap_ & USED_CONSTRAINT_CATALOG)
    size += packCharStarIntoBuffer(buffer,constraintCatalog_, swapBytes);
  if (usageMap_ & USED_CONSTRAINT_SCHEMA)
    size += packCharStarIntoBuffer(buffer,constraintSchema_, swapBytes);
  if (usageMap_ & USED_CONSTRAINT_NAME)
    size += packCharStarIntoBuffer(buffer,constraintName_, swapBytes);
  if (usageMap_ & USED_TRIGGER_CATALOG)
    size += packCharStarIntoBuffer(buffer,triggerCatalog_, swapBytes);
  if (usageMap_ & USED_TRIGGER_SCHEMA)
    size += packCharStarIntoBuffer(buffer,triggerSchema_, swapBytes);
  if (usageMap_ & USED_TRIGGER_NAME)
    size += packCharStarIntoBuffer(buffer,triggerName_, swapBytes);
  if (usageMap_ & USED_CATALOG_NAME)
    size += packCharStarIntoBuffer(buffer,catalogName_, swapBytes);
  if (usageMap_ & USED_SCHEMA_NAME)
    size += packCharStarIntoBuffer(buffer,schemaName_, swapBytes);
  if (usageMap_ & USED_TABLE_NAME)
    size += packCharStarIntoBuffer(buffer,tableName_, swapBytes);
  if (usageMap_ & USED_COLUMN_NAME)
    size += packCharStarIntoBuffer(buffer,columnName_, swapBytes);
  if (usageMap_ & USED_SQLID)
    size += packCharStarIntoBuffer(buffer,sqlID_, swapBytes);
  if (usageMap_ & USED_ROW_NUMBER)
    size += packIntoBuffer(buffer,rowNumber_, swapBytes);
  if (usageMap_ & USED_NSK_CODE)
    size += packIntoBuffer(buffer,nskCode_, swapBytes);
  if (usageMap_ & USED_CUSTOM_SQLSTATE)
    size += packCharStarIntoBuffer(buffer,customSQLState_, swapBytes);
  // if (usageMap_ & USED_ISO_MAPPING_CHARSET)
  //   size += packIntoBuffer(buffer,iso88591MappingCharSet_);
 
  // pack string parameters up to the last used one
  if (usageMap_ & USED_NUM_STRING_PARAMS)
  {
      size += packIntoBuffer(buffer,numStringParamsUsed_, swapBytes);

      for (Int32 i=0; i < numStringParamsUsed_; i++)
	{
	  if ( isSingleByteCharSet(optionalStringCharSet_[i]) )
	    size += packCharStarIntoBuffer(buffer,
                      (char*)optionalString_[i], swapBytes);
	  else
	    size += packCharStarIntoBuffer(buffer,
			   (NAWchar*)optionalString_[i], swapBytes);
	  
	  size += packIntoBuffer(buffer,(Lng32)optionalStringCharSet_[i], swapBytes);
	}
    }

  // pack integer parameters up to the last used one
  if (usageMap_ & USED_NUM_INT_PARAMS)
    {
      size += packIntoBuffer(buffer, numIntParamsUsed_, swapBytes);
      for (Int32 i=0; i < numIntParamsUsed_; i++)
	{
	  size += packIntoBuffer(buffer,optionalInteger_[i], swapBytes);
	}
    }

  return size;
}

NABoolean ComCondition::checkObj(IpcMessageObjType objType,
                                 IpcMessageObjVersion objVersion,
                                 NABoolean sameEndianness,
                                 IpcMessageObjSize objSize,
                                 IpcConstMessageBufferPtr buffer) const
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength()
  // - packObjIntoMessage()
  // - unpackObj()
  // - checkObj()

  const IpcConstMessageBufferPtr lastByte = buffer + objSize - 1;

  if (!checkBaseClass(objType, objVersion, sameEndianness, objSize, buffer))
    return FALSE;

  // Some fields are packed unconditionally
  if (!checkBuffer(buffer, sizeof(conditionNumber_), lastByte))
    return FALSE;

  Int32  map = 0;
  if (!checkAndUnpackBuffer(buffer, sizeof(map), (char *) &map, lastByte))
    return FALSE;
  if (!sameEndianness)
    swapFourBytes(map);
  
  if (!checkBuffer(buffer, sizeof(emsEventVisits_), lastByte))
    return FALSE;

  // Some fields are packed conditionally
  if (map & USED_FLAGS)
    if (!checkBuffer(buffer, sizeof(flagsTBS_), lastByte))
      return FALSE;
  
  if (map & USED_SQLCODE)
  {
    if (!checkBuffer(buffer, sizeof(theSQLCODE_), lastByte))
      return FALSE;
  }
  else
  {
    return FALSE;
  }
  
  if (map & USED_SERVER_NAME)
    if (!checkCharStarInBuffer(buffer, sameEndianness, lastByte))
      return FALSE;
  if (map & USED_CONNECTION_NAME)
    if (!checkCharStarInBuffer(buffer, sameEndianness, lastByte))
      return FALSE;
  if (map & USED_CONSTRAINT_CATALOG)
    if (!checkCharStarInBuffer(buffer, sameEndianness, lastByte))
      return FALSE;
  if (map & USED_CONSTRAINT_SCHEMA)
    if (!checkCharStarInBuffer(buffer, sameEndianness, lastByte))
      return FALSE;
  if (map & USED_CONSTRAINT_NAME)
    if (!checkCharStarInBuffer(buffer, sameEndianness, lastByte))
      return FALSE;
  if (map & USED_TRIGGER_CATALOG)
    if (!checkCharStarInBuffer(buffer, sameEndianness, lastByte))
      return FALSE;
  if (map & USED_TRIGGER_SCHEMA)
    if (!checkCharStarInBuffer(buffer, sameEndianness, lastByte))
      return FALSE;
  if (map & USED_TRIGGER_NAME)
    if (!checkCharStarInBuffer(buffer, sameEndianness, lastByte))
      return FALSE;
  if (map & USED_CATALOG_NAME)
    if (!checkCharStarInBuffer(buffer, sameEndianness, lastByte))
      return FALSE;
  if (map & USED_SCHEMA_NAME)
    if (!checkCharStarInBuffer(buffer, sameEndianness, lastByte))
      return FALSE;
  if (map & USED_TABLE_NAME)
    if (!checkCharStarInBuffer(buffer, sameEndianness, lastByte))
      return FALSE;
  if (map & USED_COLUMN_NAME)
    if (!checkCharStarInBuffer(buffer, sameEndianness, lastByte))
      return FALSE;
  if (map & USED_SQLID)
    if (!checkCharStarInBuffer(buffer, sameEndianness, lastByte))
      return FALSE;
  if (map & USED_ROW_NUMBER)
    if (!checkBuffer(buffer, sizeof(rowNumber_), lastByte))
      return FALSE;
  if (map & USED_NSK_CODE)
    if (!checkBuffer(buffer, sizeof(nskCode_), lastByte))
      return FALSE;
  if (map & USED_CUSTOM_SQLSTATE)
    if (!checkCharStarInBuffer(buffer, sameEndianness, lastByte))
      return FALSE;
  // if (map & USED_ISO_MAPPING_CHARSET)
  //   if (!checkBuffer(buffer, sizeof(iso88591MappingCharSet_), lastByte))
  //     return FALSE;

  // Optional string parameters
  if (map & USED_NUM_STRING_PARAMS)
  {
    Int32  numParams = 0;
    if (!checkAndUnpackBuffer(buffer, sizeof(numParams),
                              (char *) &numParams, lastByte))
      return FALSE;
    if (!sameEndianness)
      swapFourBytes(numParams);
    
    for (Lng32 i = 0; i < numParams; i++)
    {
      if (!checkCharStarInBuffer(buffer, sameEndianness, lastByte))
        return FALSE;
      if (!checkBuffer(buffer, sizeof(Lng32), lastByte)) // character set
        return FALSE;
    }
  }
  
  // Optional integer parameters
  if (map & USED_NUM_INT_PARAMS)
  {
    Int32  numParams = 0;
    if (!checkAndUnpackBuffer(buffer, sizeof(numParams),
                              (char *) &numParams, lastByte))
      return FALSE;
    if (!sameEndianness)
      swapFourBytes(numParams);
    
    for (Lng32 i = 0; i < numParams; i++)
      if (!checkBuffer(buffer, sizeof(Lng32), lastByte))
        return FALSE;
  }
  
  return TRUE;
}

// The set methods.
//
// Questions
//
// * Since each of these member functions is so similar, how about
//   using C++ templates, or some mechanism, to factor the code?
//
// * Would the ``properties'' trick, shown in C++ Report magazine do the
//   job?
//
// The pattern for how set strings

void ComCondition::setServerName         (const char *const name)
{
   assert(!isLocked_);
   assignStringMember(serverName_,name);
}

// We can use this function in each of the char* ``set'' routines
// of the ComCondition class.

// UR2
void ComCondition::assignStringMember(NAWchar *& memberBuff,const NAWchar *const src)
{
   if (collHeapPtr_ != NULL) {
      // Just a char array, so no destructor call.
      collHeapPtr_->deallocateMemory(memberBuff);
   }
   else
      delete [] memberBuff;
   memberBuff=NULL;  // now memberBuff is cleared...

   // if src is non-NULL then we need to create a buff, copy it over...
   if (src != NULL) {
     UInt32 sourceLen = na_wcslen(src);       // length in wide characters
     UInt32 buffsize = (sourceLen + 1) * sizeof(NAWchar); // length in bytes
     if (collHeapPtr_ != NULL) {
       // Just a char array, so no construction needed.
       memberBuff = (NAWchar*) (collHeapPtr_->allocateMemory(buffsize));
       assert(memberBuff != NULL);
     }
     else {
       memberBuff = new NAWchar[sourceLen + 1];
       assert(memberBuff != NULL);
     }
     na_wcscpy(memberBuff,src);
     memberBuff[sourceLen] = 0;
   }
}

void ComCondition::assignStringMember(char *& memberBuff,const char *const src)
{
   if (collHeapPtr_ != NULL) {
      // Just a char array, so no destructor call.
      collHeapPtr_->deallocateMemory(memberBuff);
   }
   else
      delete [] memberBuff;
   memberBuff=NULL;  // now memberBuff is cleared...

   // if src is non-NULL then we need to create a buff, copy it over...
   if (src != NULL) {
     UInt32     buffsize = str_len(src) + 1;
     if (collHeapPtr_ != NULL) {
       // Just a char array, so no construction needed.
       memberBuff = (char*) (collHeapPtr_->allocateMemory(buffsize));
       assert(memberBuff != NULL);
     }
     else {
       memberBuff = new char[buffsize];
       assert(memberBuff != NULL);
     }
     str_cpy(memberBuff,src,buffsize);
     memberBuff[buffsize-1]=0;
   }
}

// Now we continue with the rest of the code for implementing the
// string-set functions.

void ComCondition::setConnectionName     (const char *const name)
{
   assert(!isLocked_);
   assert(name != NULL);
   assignStringMember(connectionName_,name);
}

void ComCondition::setConstraintCatalog  (const char *const catalog)
{
   assert(!isLocked_);
   assert(catalog != NULL);
   assignStringMember(constraintCatalog_,catalog);
}

void ComCondition::setConstraintSchema   (const char *const schema)
{
   assert(!isLocked_);
   assert(schema != NULL);
   assignStringMember(constraintSchema_,schema);
}

void ComCondition::setConstraintName     (const char *const name)
{
   assert(!isLocked_);
   assert(name != NULL);
   assignStringMember(constraintName_,name);
}

void ComCondition::setTriggerCatalog  (const char *const catalog)
{
   assert(!isLocked_);
   assert(catalog != NULL);
   assignStringMember(triggerCatalog_,catalog);
}

void ComCondition::setTriggerSchema   (const char *const schema)
{
   assert(!isLocked_);
   assert(schema != NULL);
   assignStringMember(triggerSchema_,schema);
}

void ComCondition::setTriggerName     (const char *const name)
{
   assert(!isLocked_);
   assert(name != NULL);
   assignStringMember(triggerName_,name);
}

void ComCondition::setCatalogName        (const char *const name)
{
   assert(!isLocked_);
   assert(name != NULL);
   assignStringMember(catalogName_,name);
}

void ComCondition::setSchemaName         (const char *const name)
{
   assert(!isLocked_);
   assert(name != NULL);
   assignStringMember(schemaName_,name);
}

void ComCondition::setTableName          (const char *const name)
{
   assert(!isLocked_);
   assert(name != NULL);
   assignStringMember(tableName_,name);
}

void ComCondition::setCustomSQLState     (const char *const customSQLState)
{
   assert(!isLocked_);
   assert(customSQLState != NULL);
   assignStringMember(customSQLState_,customSQLState);
}

void ComCondition::setColumnName         (const char *const name)
{
   assert(!isLocked_);
   assert(name != NULL);
   assignStringMember(columnName_,name);
}

void ComCondition::setSqlID         (const char *const name)
{
   assert(!isLocked_);
   assert(name != NULL);
   assignStringMember(sqlID_,name);
}

// Now we have three members which are concerned with setting intlike
// values.

void ComCondition::setConditionNumber(ComDiagBigInt newCondition)
{
   conditionNumber_ = newCondition;
}

void ComCondition::setSQLCODE (Lng32 newSQLCODE)
{
  theSQLCODE_ = newSQLCODE;

  if ( ! (theSQLCODE_ < 0) ) return; // if not an error return

  Lng32 theError = (theSQLCODE_ < 0) ? -theSQLCODE_ : theSQLCODE_;
  
  char  *reqErrorStr = NULL;  // user requested error to loop/abort on
  Lng32  reqError = 0;

  if ( reqErrorStr = getenv("LOOP_ON_ERROR") ) {
    reqError = strtol( reqErrorStr, (char **)NULL, 10) ;
    if ( reqError > INT_MIN && reqError < INT_MAX &&  // no error in env var
	 reqError == theError ) // and this one is the requested error 
      {
	UInt32 timeDelay = 3 ; // 3 seconds
	short loopCount = 60; // 60 * 3 seconds = 3 minutes
	Lng32 loopError = 1;
      
	while ( loopError ) // To exit loop in gdb do: set var loopError=0
	  {
#ifdef _DEBUG
            // In the debug build, notify the user we are looping by
            // printing to stdout every 60 seconds
            if (loopCount % 20 == 0)
            {
              NAProcessHandle myPhandle;
              myPhandle.getmine();
              myPhandle.decompose();
              int pid = (int) myPhandle.getPin();
              int node = (int) myPhandle.getNodeNumber();
              
              char timeString[40];
              timeval tv;
              tm tx;
              gettimeofday(&tv, NULL);
              localtime_r(&tv.tv_sec, &tx);
              snprintf(timeString, 40, "%04d-%02d-%02d %02d:%02d:%02d.%06d",
                      tx.tm_year + 1900, tx.tm_mon + 1, tx.tm_mday,
                      tx.tm_hour, tx.tm_min, tx.tm_sec, (int) tv.tv_usec);
              
              printf("(%d,%d) %s LOOP_ON_ERROR %d\n",
                     node, pid, timeString, (int) theError);
              fflush(stdout);
            }
#endif

	    // log a message (with the process ID) every three minutes
	    if ( loopCount == 60 )
	      SQLMXLoggingArea::logSQLMXDebugEvent("Loop on error", reqError,__LINE__);

	    // Suspend the process for 'timeDelay'
	    sleep( timeDelay );
	    
	    loopCount = (loopCount == 1) ? 60 : --loopCount;
	  }
      }
  }

  if ( reqErrorStr = getenv("ABORT_ON_ERROR") ) {
    reqError = strtol( reqErrorStr, (char **)NULL, 10) ;
    if ( reqError > INT_MIN && reqError < INT_MAX &&  // no error in env var
	 reqError == theError ) // and this one is the requested error 
      {
	releaseRTSSemaphore();

	// log a message
	SQLMXLoggingArea::logSQLMXDebugEvent("Abort on error", reqError,__LINE__);

	abort();  // dump core
      }
  }

// This code has been unit tested and most is also tested by
// executor/TEST082
  if ((theError == CLI_TCB_EXECUTE_ERROR) ||  // 8816
      (theError == CLI_INTERNAL_ERROR   ) ||  // 8898
      (theError == EXE_INTERNAL_ERROR   ))    // 8001
  {
     // make a core-file to help understand this scenario.  But do 
     // return the error to the user.  Notice that this Linux-only
     // code is not compiled for EID.  If this behavior make problems,
     // it can be controlled by setting envvars .
     static bool InternErrorMakesCorefileInitialized = false;
     static bool corefile8816 = false;
     static bool corefile8898 = false;
     static bool corefile8001 = false;
     if (!InternErrorMakesCorefileInitialized)
     {
       InternErrorMakesCorefileInitialized = true;
       char *envvar = NULL;
       envvar = getenv("SQLMX_COREFILE_8816");
       if (envvar && envvar[0] == '1')
         corefile8816 = true;
       envvar = getenv("SQLMX_COREFILE_8898");
       if (envvar && envvar[0] == '1')
         corefile8898 = true;
       envvar = getenv("SQLMX_COREFILE_8001");
       if (envvar && envvar[0] == '1')
         corefile8001 = true;
     }
     if ( (corefile8816 && theError == CLI_TCB_EXECUTE_ERROR) ||
          (corefile8898 && theError == CLI_INTERNAL_ERROR   ) ||
          (corefile8001 && theError == EXE_INTERNAL_ERROR   ) )
       genLinuxCorefile( (char *)
         "Generating core-file to capture internal error scenario.");
  }
}

void ComCondition::setRowNumber(  Lng32 newRowNumber)
{
   rowNumber_ = newRowNumber;
}

void ComCondition::setNskCode(  Lng32 newNskCode)
{
   nskCode_ = newNskCode;

   char *reqErrorStr = NULL;
   Lng32 reqError = 0;

   if ( reqErrorStr = getenv("LOOP_ON_NSK_ERROR") )
   {
     reqError = strtol(reqErrorStr, (char **)NULL, 10);
     if ( reqError > INT_MIN && reqError < INT_MAX &&  // no error in env var
          reqError == newNskCode ) // and this one is the requested NSK error
     {
       UInt32 timeDelay = 3; // 3 seconds
       short loopCount = 60; // 60 * 3 seconds = 3 minutes
       Lng32 loopError = 1;
      
       while ( loopError ) // To exit loop in gdb do: set var loopError=0
       {
         // log a message (with the process ID) every three minutes
         if ( loopCount == 60 )
           SQLMXLoggingArea::logSQLMXDebugEvent("Loop on error", reqError,__LINE__);

         // Suspend the process for 'timeDelay'
         sleep( timeDelay );

         loopCount = (loopCount == 1) ? 60 : --loopCount;
       }
     }
   }

   if ( reqErrorStr = getenv("ABORT_ON_NSK_ERROR") )
   {
     reqError = strtol(reqErrorStr, (char **)NULL, 10);
     if ( reqError > INT_MIN && reqError < INT_MAX &&  // no error in env var
          reqError == newNskCode ) // and this one is the requested NSK error 
     {
       releaseRTSSemaphore();

       // log a message
       SQLMXLoggingArea::logSQLMXDebugEvent("Abort on NSK error", reqError,__LINE__);

       abort();  // dump core
     }
   }
}

// Getting and Setting the Optional Parameters

Lng32 ComCondition::getOptionalInteger(Lng32     index) const
{
   assert(index < NumOptionalParms);
   // add the following to prevent false alarm on "index"
   // without considering the above assert
   // coverity[overrun_local]
   return optionalInteger_[index];
}

void ComCondition::setOptionalInteger(Lng32     index, Lng32 newValue)
{
   assert(index < NumOptionalParms);
   // add the following to prevent false alarm on "index"
   // without considering the above assert
   // coverity[overrun_local]
   optionalInteger_[index] = newValue;
}

CharInfo::CharSet 
ComCondition::getOptionalStringCharSet(Lng32 index) const
{
   assert(index < NumOptionalParms);
   // add the following to prevent false alarm on "index"
   // without considering the above assert
   // coverity[overrun_local]
   return optionalStringCharSet_[index];
}

NABoolean ComCondition::hasOptionalString(Lng32 index) const
{
   assert(index < NumOptionalParms);

   return optionalString_[index] != NULL;
}

const char * ComCondition::getOptionalString(Lng32 index) const
{
   assert(index < NumOptionalParms);
   // add the following to prevent false alarm on "index"
   // without considering the above assert
   // coverity[overrun_local]
   if (isSingleByteCharSet(optionalStringCharSet_[index]))
     return (const char*)optionalString_[index];
   return NULL;
}

const NAWchar *
ComCondition::getOptionalWString(Lng32 index) const
{
   assert(index < NumOptionalParms);
   // add the following to prevent false alarm on "index"
   // without considering the above assert
   // coverity[overrun_local]
   if (optionalString_[index] == NULL)
     return NULL;
   if (! isSingleByteCharSet(optionalStringCharSet_[index]))
     return (const NAWchar*)optionalString_[index];
   return NULL;
}

void ComCondition::setOptionalString(Lng32 index,
	const char* const source,
	CharInfo::CharSet cs)
{
   if (isSingleByteCharSet(cs)) {
     // Do NOT "assert(source != NULL);" -- it's ok to pass in a NULL string
     assert(index < NumOptionalParms);
     // add the following to prevent false alarm on "index"
     // without considering the above assert
     // coverity[overrun_local]
     optionalStringCharSet_[index] = cs;
     assignStringMember((char*&)optionalString_[index],source);
   }
   else
     setOptionalWString(index, (NAWchar*)source);
}

void ComCondition::setOptionalWString(Lng32 index,
	const NAWchar* const source)
	// CharInfo::CharSet cs)		##hardcoded for now
{
   CharInfo::CharSet cs = CharInfo::UNICODE;	//hardcoded
   // if (! isSingleByteCharSet(cs)) { ...as above...

   // Do NOT "assert(source != NULL);" -- it's ok to pass in a NULL string
   assert(index < NumOptionalParms);
   // add the following to prevent false alarm on "index"
   // without considering the above assert
   // coverity[overrun_local]
   optionalStringCharSet_[index] = cs;
   assignStringMember((NAWchar*&)optionalString_[index],source);
}

// CharInfo::CharSet
// ComCondition::getIso88591MappingCharSet() const
// {
//   // Return CharInfo::UnknownCharSet if the data member not set yet
//   if ((usageMap_ & USED_ISO_MAPPING_CHARSET) &&
//       isValidIsoMappingCharSet(iso88591MappingCharSet_))
//     return iso88591MappingCharSet_;
//   else
//     return CharInfo::UnknownCharSet;
// }

// void ComCondition::setIso88591MappingCharSet(CharInfo::CharSet cs)
// {
//   // assert(isValidIsoMappingCharSet(cs));
//   usageMap_ |= USED_ISO_MAPPING_CHARSET;
//   iso88591MappingCharSet_ = cs;
// }

//  ComDiags Methods Implemented
//

ComDiagsArea::ComDiagsArea (CollHeap* ptr): IpcMessageObj(IPC_SQL_DIAG_AREA,0),
                                            collHeapPtr_(ptr),
                                            errors_(ptr),
                                            warnings_(ptr),
                                            newCondition_(NULL),
					    lengthLimit_(30),
                                            areMore_(ComCondition::NO_MORE),
                                            maxDiagsId_(0),
                                            rowCount_(0),
                                            avgStreamWaitTime_(-1),
                                            cost_(0),
                                            theSQLFunction_(NULL_FUNCTION),
                                            flags_(0),
					    rowsetRowCountArray_(NULL)
{
   // Initialize fillers space to 0
   memset(fillers_, 0, sizeof(fillers_));
   // Make sure the size of ComDiagsArea remains constant
   // If you hit this after change or add new member, adjust the fillers_ size
   Int32 classSize = sizeof(ComDiagsArea);
   assert(classSize == 328);
}

ComDiagsArea::ComDiagsArea () :             IpcMessageObj(IPC_SQL_DIAG_AREA,0),
                                            collHeapPtr_(NULL),
                                            errors_(NULL),
                                            warnings_(NULL),
                                            newCondition_(NULL),
					    lengthLimit_(30),
                                            areMore_(ComCondition::NO_MORE),
                                            maxDiagsId_(0),
                                            rowCount_(0),
                                            avgStreamWaitTime_(-1),
                                            cost_(0),
                                            theSQLFunction_(NULL_FUNCTION),
                                            flags_(0),
					    rowsetRowCountArray_(NULL)
{
   // Initialize fillers space to 0
   memset(fillers_, 0, sizeof(fillers_));
   // Make sure the size of ComDiagsArea remains constant
   // If you hit this after change or add new member, adjust the fillers_ size
   Int32 classSize = sizeof(ComDiagsArea);


   // if (classSize != 320) printf("classSize=%d @ %d\n", classSize, __LINE__);
   assert(classSize == 328);
}


// We want to clear out the errors and warnings lists, and free
// the objects contained therein.  We want to set collHeapPtr_ to
// NULL (just in case!).
//
// Questions
//
// * Is there a better way to iterate over all members of a LIST?

ComDiagsArea::~ComDiagsArea ()
{
   CollIndex i=0;
   while (i!=errors_.entries())        errors_[i++]->deAllocate();
   i=0;
   while (i!=warnings_.entries())      warnings_[i++]->deAllocate();
   if (newCondition_ != NULL) {
      newCondition_->deAllocate();
      newCondition_=NULL;
   }
   errors_.clear();
   warnings_.clear();
   flags_ = 0;
   if (rowsetRowCountArray_ != NULL) {
    rowsetRowCountArray_->deallocate();
    rowsetRowCountArray_ = NULL;
   }
}

void ComDiagsArea::enforceLengthLimit()
{
   const Lng32     currentCount = getNumber();
   if ((lengthLimit_ != ComCondition::NO_LIMIT_ON_ERROR_CONDITIONS) &&
	(currentCount > lengthLimit_)) {
        Lng32 numToDiscard = currentCount - lengthLimit_;
	// soln:10-050204-4441 : maxDiagsId_ member needs to be updated so as to 
        // maintain consistency between count(error) + count(warnings) and maxDiagsId_
        // upon exceeding lengthLimit_.This is done here as numToDiscard is modified 
	// in this function.
        maxDiagsId_ = maxDiagsId_ - numToDiscard;
      if (numToDiscard > (Lng32) warnings_.entries()) {
         assert(errors_.entries() >= (numToDiscard-warnings_.entries()));

         // First, drop all warnings.
         CollIndex i=0;
         while (i!=warnings_.entries())      warnings_[i++]->deAllocate();
         warnings_.clear();

         // Second, drop the proper number of errors from the end of the list.
         // make errors a sequence 1..j, where j = errors_.entries() -
         //                             (numToDiscard-warnings_.entries())

         numToDiscard =  numToDiscard - warnings_.entries();
         CollIndex j = errors_.entries() - numToDiscard;
         while (numToDiscard-- != 0) {
            errors_[j]->deAllocate();// remove near end and slide towards front
            NABoolean removeResult = errors_.removeAt(j);
            assert(removeResult);  // sanity check
         }
	 areMore_ = ComCondition::MORE_ERRORS ;
      }
      else {
         // make warnings a sequence 1..j, where j = warnings_.entries() -
         //                                          numToDiscard
         CollIndex j = warnings_.entries() - numToDiscard;
         while (numToDiscard-- != 0) {
            warnings_[j]->deAllocate();// remove near end and slide towards front
            NABoolean removeResult = warnings_.removeAt(j);
            assert(removeResult);  // sanity check
         }
	 areMore_ = ComCondition::MORE_WARNINGS ;
      }
   }
}


void ComDiagsArea::unpackObj(IpcMessageObjType objType,
                             IpcMessageObjVersion objVersion,
                             Int32  sameEndianness,
                             IpcMessageObjSize objSize,
                             const char* buffer)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength()
  // - packObjIntoMessage()
  // - unpackObj()
  // - checkObj()

  short extraFieldsLen;

  assert(newCondition_ == NULL);
  // rowsetRowCountArray should be null as it is shipped across process boundaries.
  assert(rowsetRowCountArray_ == NULL);
  unpackBaseClass(buffer);
  unpackBuffer(buffer,areMore_);
  unpackBuffer(buffer,lengthLimit_);
  unpackBuffer(buffer,rowCount_);
  unpackBuffer(buffer,avgStreamWaitTime_);
  unpackBuffer(buffer,cost_);
  unpackBuffer(buffer,theSQLFunction_);
  unpackBuffer(buffer,maxDiagsId_);
  unpackBuffer(buffer,flags_);
  // length of fields added in a later version, just skip them
  unpackBuffer(buffer,extraFieldsLen);
  buffer += extraFieldsLen;

    short  num;
  unpackBuffer(buffer,num);
  CollIndex index=0;
  while (index!=(CollIndex)num) {
     // don't forget to append a new object...created on which heap
     // is the right one!
     DiagsCondition  *newObject = DiagsCondition::allocate(collHeapPtr_);
     assert(newObject != NULL);
     newObject->unpackDependentObjFromBuffer(buffer, sameEndianness);
     errors_.insert(newObject);
     index++;
  }
  unpackBuffer(buffer,num);
  index = 0;
  while (index!=(CollIndex) num) {
     // don't forget to append a new object...created on which heap
     // is the right one!  And, yes, it's true that this loop body
     // is a copy/paste of the loop body of the above while loop.
     DiagsCondition  *newObject = DiagsCondition::allocate(collHeapPtr_);
     assert(newObject != NULL);
     newObject->unpackDependentObjFromBuffer(buffer, sameEndianness);
     warnings_.insert(newObject);
     index++;
  }
  if (lengthLimit_ != ComCondition::NO_LIMIT_ON_ERROR_CONDITIONS)
    assert( getNumber() <= lengthLimit_ );

  // unpack fillers_ part
  unpackStrFromBuffer(buffer, fillers_, sizeof(fillers_));
}


// unpack ComDiagsArea sent from 32-bit (BDR) server
void ComDiagsArea::unpackObj32(IpcMessageObjType objType,
                               IpcMessageObjVersion objVersion,
                               Int32  sameEndianness,
                               IpcMessageObjSize objSize,
                               const char* buffer)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength()
  // - packObjIntoMessage()
  // - unpackObj()
  // - checkObj()

  short extraFieldsLen;

  assert(newCondition_ == NULL);
  // rowsetRowCountArray should be null as it is shipped across process boundaries.
  assert(rowsetRowCountArray_ == NULL);
  unpackBaseClass32(buffer);
  unpackBuffer(buffer,areMore_);
  unpackBuffer(buffer,lengthLimit_);
  unpackBuffer(buffer,rowCount_);
  unpackBuffer(buffer,avgStreamWaitTime_);
  unpackBuffer(buffer,cost_);
  unpackBuffer(buffer,theSQLFunction_);
  unpackBuffer(buffer,maxDiagsId_);
  unpackBuffer(buffer,flags_);
  // length of fields added in a later version, just skip them
  unpackBuffer(buffer,extraFieldsLen);
  buffer += extraFieldsLen;

    short  num;
  unpackBuffer(buffer,num);
  CollIndex index=0;
  while (index!=(CollIndex)num) {
     // don't forget to append a new object...created on which heap
     // is the right one!
     DiagsCondition  *newObject = DiagsCondition::allocate(collHeapPtr_);
     assert(newObject != NULL);
     newObject->unpackDependentObjFromBuffer32(buffer, sameEndianness);
     errors_.insert(newObject);
     index++;
  }
  unpackBuffer(buffer,num);
  index = 0;
  while (index!=(CollIndex) num) {
     // don't forget to append a new object...created on which heap
     // is the right one!  And, yes, it's true that this loop body
     // is a copy/paste of the loop body of the above while loop.
     DiagsCondition  *newObject = DiagsCondition::allocate(collHeapPtr_);
     assert(newObject != NULL);
     newObject->unpackDependentObjFromBuffer32(buffer, sameEndianness);
     warnings_.insert(newObject);
     index++;
  }
  if (lengthLimit_ != ComCondition::NO_LIMIT_ON_ERROR_CONDITIONS)
    assert( getNumber() <= lengthLimit_ );

  // unpack fillers_ part
  unpackStrFromBuffer(buffer, fillers_, sizeof(fillers_));
}


// The packed length of a ComDiagsObject is the sum of the sizes
// of the integer members plus the sum of the sizes of the string
// members (see the comments for ComCondition::packedLength() to see
// how string length is determined).  We omit the collHeapPtr_ since
// it is not transferred via IPC (see comments for
// ComCondition::packedLength).
//
// We must include the length of the warning and error lists as well.
// the length of a list is the sum of the size of an   short
// (which tells number of elements) and the sum of the sizes of the
// individual member ComCondition objects.
//
// Let us not forget the size of the base class.
//
// We do not pack newCondition_.
IpcMessageObjSize ComDiagsArea::packedLength(void)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength()
  // - packObjIntoMessage()
  // - unpackObj()
  // - checkObj()

  IpcMessageObjSize  size=baseClassPackedLength();

  short extraFieldsLen = 0; // no extra fields yet in Release 1.5 

  // rowsetRowCountArray was added in after R2. It does not need
  // to be packed/unpacked as it is used only in the master. Just 
  // skip this data member during pack/unpack.
  extraFieldsLen = sizeof(rowsetRowCountArray_);

  size += sizeof(areMore_);
  size += sizeof(lengthLimit_);
  size += sizeof(rowCount_);
  size += sizeof(avgStreamWaitTime_);
  size += sizeof(cost_);
  size += sizeof(theSQLFunction_);
  size += sizeof(maxDiagsId_);
  size += sizeof(flags_);
  size += sizeof(extraFieldsLen);
  size += sizeof(rowsetRowCountArray_);

  // For the error and warning list (in that order) we add the size
  // for two shorts, for lengths, then we visit all
  // DiagsCondition objects and sum their sizes as well.

  size += sizeof(short);
  CollIndex  index = 0;
  while (index != errors_.entries()) {
     alignSizeForNextObj(size);
     size += (errors_[index])->packedLength();
     index++;
  }
  size += sizeof(short);
  index = 0;
  while (index != warnings_.entries()) {
     alignSizeForNextObj(size);
     size += (warnings_[index])->packedLength();
     index++;
  }

  // Add the fillers_ size too
  size += sizeof(fillers_);

  return size;
}


IpcMessageObjSize ComDiagsArea::packedLength32(void)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength32()
  // - packObjIntoMessage32()
  // - unpackObj()
  // - checkObj()

  IpcMessageObjSize  size=baseClassPackedLength32();

  short extraFieldsLen = 0; // no extra fields yet in Release 1.5 

  // rowsetRowCountArray was added in after R2. It does not need
  // to be packed/unpacked as it is used only in the master. Just 
  // skip this data member during pack/unpack.
  extraFieldsLen = sizeof(rowsetRowCountArray_);

  size += sizeof(areMore_);
  size += sizeof(lengthLimit_);
  size += sizeof(rowCount_);
  size += sizeof(avgStreamWaitTime_);
  size += sizeof(cost_);
  size += sizeof(theSQLFunction_);
  size += sizeof(maxDiagsId_);
  size += sizeof(flags_);
  size += sizeof(extraFieldsLen);
  size += sizeof(rowsetRowCountArray_);

  // For the error and warning list (in that order) we add the size
  // for two shorts, for lengths, then we visit all
  // DiagsCondition objects and sum their sizes as well.

  size += sizeof(short);
  CollIndex  index = 0;
  while (index != errors_.entries()) {
     alignSizeForNextObj(size);
     size += (errors_[index])->packedLength32();
     index++;
  }
  size += sizeof(short);
  index = 0;
  while (index != warnings_.entries()) {
     alignSizeForNextObj(size);
     size += (warnings_[index])->packedLength32();
     index++;
  }

  // Add the fillers_ size too
  size += sizeof(fillers_);

  return size;
}

IpcMessageObjSize ComDiagsArea::packObjIntoMessage(char *buffer)
{
  return packObjIntoMessage(buffer, FALSE);
}

IpcMessageObjSize ComDiagsArea::packObjIntoMessage(char* buffer,
                                NABoolean swapBytes)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength()
  // - packObjIntoMessage()
  // - unpackObj()
  // - checkObj()
  IpcMessageObjSize  size=packBaseClassIntoMessage(buffer, swapBytes);
  
 
  short extraFieldsLen = 0; // no extra fields yet in Release 1.5

  // rowsetRowCountArray was added in after R2. It does not need
  // to be packed/unpacked as it is used only in the master. Just 
  // skip this data member during pack/unpack.
  extraFieldsLen = sizeof(rowsetRowCountArray_);

  size += packIntoBuffer(buffer,areMore_, swapBytes);
  size += packIntoBuffer(buffer,lengthLimit_, swapBytes);
  size += packIntoBuffer(buffer,rowCount_, swapBytes);
  size += packIntoBuffer(buffer,avgStreamWaitTime_, swapBytes);
  double tempCost = cost_;
  // Double scalar data type will not work with bswap_64
  // hence doing it here by casting it Int64
  if (swapBytes)
      tempCost = bswap_64((Int64)cost_);
  size += packIntoBuffer(buffer,tempCost);
  size += packIntoBuffer(buffer,theSQLFunction_, swapBytes);
  size += packIntoBuffer(buffer,maxDiagsId_, swapBytes);
  size += packIntoBuffer(buffer,flags_, swapBytes);
  size += packIntoBuffer(buffer,extraFieldsLen, swapBytes);

  // If fields get added after Release 1.5 without wanting to do
  // major versioning changes, add them here and set extraFieldsLen
  // to their length. Release 1.5 code will then simply ignore the extra
  // fields.

  // rowsetRowCountArray should be null as it is shipped across process boundaries.
  assert(rowsetRowCountArray_ == NULL);
  size += packIntoBuffer(buffer,rowsetRowCountArray_);

  short num = (short) errors_.entries();
  short numToPack = num;

#ifdef _DEBUG
  // In the debug build we allow the UDR server to generate a corrupt
  // packed object. This allows us to test error handling in the
  // executor. The MXUDR_DEBUG_BUILD variable is always set by the
  // debug UDR server and we only test it once. We test
  // MXUDR_CORRUPT_DIAGS_REPLY every time we come here in the UDR
  // server.
  static NABoolean inUdrServer = (getenv("MXUDR_DEBUG_BUILD") != NULL);
  if (inUdrServer)
  {
    char *val = getenv("MXUDR_CORRUPT_DIAGS_REPLY");
    if (val && val[0])
    {
      numToPack++;
    }
  }
#endif
  
  size += packIntoBuffer(buffer, numToPack, swapBytes);
 

  CollIndex index=0;
  while (index!= (CollIndex) num) {
     IpcMessageObjSize temp =
       (errors_[index])->packDependentObjIntoMessage(buffer, swapBytes);
     size += temp;
     buffer += temp;
     index++;
  }
  num = (  short) warnings_.entries();
  size += packIntoBuffer(buffer,num, swapBytes);
  index = 0;
  while (index!= (CollIndex) num) {
     IpcMessageObjSize temp =
       (warnings_[index])->packDependentObjIntoMessage(buffer, swapBytes);
     size += temp;
     buffer += temp;
     index++;
  }

  // Pack the fillers part too
  size += packStrIntoBuffer(buffer, fillers_, sizeof(fillers_));

  return size;
}


// pack object to buffer to be read by 32-bit (BDR) client
IpcMessageObjSize ComDiagsArea::packObjIntoMessage32(char *buffer)
{
  return packObjIntoMessage32(buffer, FALSE);
}

IpcMessageObjSize ComDiagsArea::packObjIntoMessage32(char* buffer,
                                NABoolean swapBytes)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength32()
  // - packObjIntoMessage32()
  // - unpackObj()
  // - checkObj()
  IpcMessageObjSize  size=packBaseClassIntoMessage32(buffer, swapBytes);
  
 
  short extraFieldsLen = 0; // no extra fields yet in Release 1.5

  // rowsetRowCountArray was added in after R2. It does not need
  // to be packed/unpacked as it is used only in the master. Just 
  // skip this data member during pack/unpack.
  extraFieldsLen = sizeof(rowsetRowCountArray_);

  size += packIntoBuffer(buffer,areMore_, swapBytes);
  size += packIntoBuffer(buffer,lengthLimit_, swapBytes);
  size += packIntoBuffer(buffer,rowCount_, swapBytes);
  size += packIntoBuffer(buffer,avgStreamWaitTime_, swapBytes);
  double tempCost = cost_;
  // Double scalar data type will not work with bswap_64
  // hence doing it here by casting it Int64
  if (swapBytes)
      tempCost = bswap_64((Int64)cost_);
  size += packIntoBuffer(buffer,tempCost);
  size += packIntoBuffer(buffer,theSQLFunction_, swapBytes);
  size += packIntoBuffer(buffer,maxDiagsId_, swapBytes);
  size += packIntoBuffer(buffer,flags_, swapBytes);
  size += packIntoBuffer(buffer,extraFieldsLen, swapBytes);

  // If fields get added after Release 1.5 without wanting to do
  // major versioning changes, add them here and set extraFieldsLen
  // to their length. Release 1.5 code will then simply ignore the extra
  // fields.

  // rowsetRowCountArray should be null as it is shipped across process boundaries.
  assert(rowsetRowCountArray_ == NULL);
  size += packIntoBuffer(buffer,rowsetRowCountArray_);

  short num = (short) errors_.entries();
  short numToPack = num;

#ifdef _DEBUG
  // In the debug build we allow the UDR server to generate a corrupt
  // packed object. This allows us to test error handling in the
  // executor. The MXUDR_DEBUG_BUILD variable is always set by the
  // debug UDR server and we only test it once. We test
  // MXUDR_CORRUPT_DIAGS_REPLY every time we come here in the UDR
  // server.
  static NABoolean inUdrServer = (getenv("MXUDR_DEBUG_BUILD") != NULL);
  if (inUdrServer)
  {
    char *val = getenv("MXUDR_CORRUPT_DIAGS_REPLY");
    if (val && val[0])
    {
      numToPack++;
    }
  }
#endif
  
  size += packIntoBuffer(buffer, numToPack, swapBytes);
 

  CollIndex index=0;
  while (index!= (CollIndex) num) {
     IpcMessageObjSize temp =
       (errors_[index])->packDependentObjIntoMessage32(buffer, swapBytes);
     size += temp;
     buffer += temp;
     index++;
  }
  num = (  short) warnings_.entries();
  size += packIntoBuffer(buffer,num, swapBytes);
  index = 0;
  while (index!= (CollIndex) num) {
     IpcMessageObjSize temp =
       (warnings_[index])->packDependentObjIntoMessage32(buffer, swapBytes);
     size += temp;
     buffer += temp;
     index++;
  }

  // Pack the fillers part too
  size += packStrIntoBuffer(buffer, fillers_, sizeof(fillers_));

  return size;
}

NABoolean ComDiagsArea::checkObj(IpcMessageObjType objType,
                                 IpcMessageObjVersion objVersion,
                                 NABoolean sameEndianness,
                                 IpcMessageObjSize objSize,
                                 IpcConstMessageBufferPtr buffer) const
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength()
  // - packObjIntoMessage()
  // - unpackObj()
  // - checkObj()

  const IpcConstMessageBufferPtr lastByte = buffer + objSize - 1;

  if (!checkBaseClass(objType, objVersion, sameEndianness, objSize, buffer))
    return FALSE;

  if (!checkBuffer(buffer, sizeof(areMore_), lastByte))
    return FALSE;

  Int32 lengthLimit = 0;
  if (!checkAndUnpackBuffer(buffer, sizeof(lengthLimit),
                            (char *) &lengthLimit, lastByte))
    return FALSE;
  if (!sameEndianness)
    swapFourBytes(lengthLimit);

  if (!checkBuffer(buffer,
                   sizeof(rowCount_)
                   + sizeof(avgStreamWaitTime_)
                   + sizeof(cost_)
                   + sizeof(theSQLFunction_)
                   + sizeof(maxDiagsId_)
                   + sizeof(flags_)
                   , lastByte))
    return FALSE;
  
  short extraFieldsLen = 0;
  if (!checkAndUnpackBuffer(buffer, sizeof(extraFieldsLen),
                            (char *) &extraFieldsLen, lastByte))
    return FALSE;
  if (!sameEndianness)
    swapTwoBytes(extraFieldsLen);

  // from NEO/Coyote releases extraFieldsLen should include
  // sizeof(rowsetRowCountArray_). There is no unpacking
  // to be done here as this data member is NULL at this point.
  if (!checkBuffer(buffer, extraFieldsLen, lastByte))
    return FALSE;

  short i;
  short numErrors = 0;
  short numWarnings = 0;
  DiagsCondition cond;

  // Error conditions
  if (!checkAndUnpackBuffer(buffer, sizeof(numErrors),
                            (char *) &numErrors, lastByte))
    return FALSE;
  if (!sameEndianness)
    swapTwoBytes(numErrors);

  for (i = 0; i < numErrors; i++)
    if (!cond.checkDependentObj(buffer, sameEndianness))
      return FALSE;
  
  // Warning conditions
  if (!checkAndUnpackBuffer(buffer, sizeof(numWarnings),
                            (char *) &numWarnings, lastByte))
    return FALSE;
  if (!sameEndianness)
    swapTwoBytes(numWarnings);

  for (i = 0; i < numWarnings; i++)
    if (!cond.checkDependentObj(buffer, sameEndianness))
      return FALSE;
  
  if ((numErrors + numWarnings) > lengthLimit)
  {
    ipcIntegrityCheckEpilogue(FALSE);
    return FALSE;
  }
  
  // Check fillers_ part
  if (!checkBuffer(buffer, sizeof(fillers_), lastByte))
    return FALSE;

  return TRUE;
}

// Accessors for Basic Info
//
// This returns the sum of the number of
// elements in errors_ and warnings_.

Lng32     ComDiagsArea::getNumber () const
{
   return errors_.entries() + warnings_.entries();
}

Lng32     ComDiagsArea::getNumber (DgSqlCode::ErrorOrWarning type) const
{
   switch (type) {
     case DgSqlCode::ERROR_:	return errors_.entries();
     case DgSqlCode::WARNING_:	return warnings_.entries();
     default:			return -1;
   }
}


NABoolean ComDiagsArea::areMore () const
{
  return (areMore_ != ComCondition::NO_MORE);
}

NABoolean ComDiagsArea::canAcceptMoreErrors () const
{
  return (areMore_ != ComCondition::MORE_ERRORS);
}


Int64 ComDiagsArea::getRowCount () const
{
   return rowCount_;
}

void ComDiagsArea::addRowCount (Int64 newRowCount)
{
   rowCount_ += newRowCount;
}

void ComDiagsArea::setRowCount (Int64 newRowCount)
{
   rowCount_ = newRowCount;
}

ComDiagBigInt ComDiagsArea::getAvgStreamWaitTime () const
{
   return avgStreamWaitTime_;
}

void ComDiagsArea::setAvgStreamWaitTime (ComDiagBigInt avgStreamWaitTime )
{
   avgStreamWaitTime_ = avgStreamWaitTime;
}

double ComDiagsArea::getCost () const
{
   return cost_;
}

void ComDiagsArea::setCost (double newCost)
{
   cost_ = newCost;
}

//
// setAllSqlID
//
// Traverse through the conditions from the latest to the earliest and
// set any unknown sql ids.  Stop at the first one that is already set
// since we've already seen it in a past execution.  (This functions
// the same way as setAllRowNumber.)
//

void ComDiagsArea::setAllSqlID (char *sqlID)
{
  if (!sqlID) return;

  Lng32 errorCount = getNumber(DgSqlCode::ERROR_);
  Int32 i = 0;
  for(i=0; i < errorCount; i++) {
    ComCondition* errCond = getErrorEntry(errorCount-i);
    if (errCond->getSqlID() == NULL)
      errCond->setSqlID(sqlID); 
    else
      break ;
  }
  Lng32 warnCount = getNumber(DgSqlCode::WARNING_);
  for(i=0; i < warnCount; i++) {
    ComCondition* warnCond = getWarningEntry(warnCount-i);
    if (warnCond->getSqlID() == NULL)
      warnCond->setSqlID(sqlID); 
    else
      break ;
  }
}

void ComDiagsArea::setAllRowNumber (Lng32 rowNum, DgSqlCode::ErrorOrWarning errOrWarn)
{
  if (errOrWarn != DgSqlCode::WARNING_)
  {
    Lng32 errorCount = getNumber(DgSqlCode::ERROR_);
    for(Int32 i=0; i < errorCount; i++) {
      ComCondition* errCond = getErrorEntry(errorCount-i);
      if (errCond->getRowNumber() < 0)
        errCond->setRowNumber(rowNum); 
      else
        break ;
    }
  }
  else
  {
    Lng32 warnCount = getNumber(DgSqlCode::WARNING_);
    for(Int32 i=0; i < warnCount; i++) {
      ComCondition* warnCond = getWarningEntry(warnCount-i);
      if (warnCond->getRowNumber() < 0)
        warnCond->setRowNumber(rowNum); 
      else
        break ;
    }
  }
}


Lng32 ComDiagsArea::getNextRowNumber (Lng32 indexValue) const
{
  Lng32 errorCount = getNumber(DgSqlCode::ERROR_);
  Lng32 nextRowNumber = ComCondition::INVALID_ROWNUMBER;

  for(Int32 i=0; i < errorCount; i++) {
    ComCondition* errCond = ((ComDiagsArea *) this)->getErrorEntry(i+1);
    if ((errCond->getRowNumber() != ComCondition::INVALID_ROWNUMBER) &&
	(errCond->getRowNumber() >= indexValue)) {
	  if ((nextRowNumber == ComCondition::INVALID_ROWNUMBER) ||
	      (errCond->getRowNumber() < nextRowNumber)) {
		nextRowNumber = errCond->getRowNumber();
	  }
    }
  }
  return nextRowNumber ;
}


NABoolean ComDiagsArea::hasValidRowsetRowCountArray () const
{
  return (rowsetRowCountArray_ != NULL);
}


Lng32  ComDiagsArea::numEntriesInRowsetRowCountArray () const
{
  if (rowsetRowCountArray_)
    return (Lng32) rowsetRowCountArray_->entries();
  else
    return 0;
}

void  ComDiagsArea::insertIntoRowsetRowCountArray (Lng32 index, Int64 value, 
					    Lng32 arraySize, CollHeap* heapPtr)
{
  if (rowsetRowCountArray_ == NULL)
  {
    if (heapPtr)
      rowsetRowCountArray_ = new (heapPtr) NAArray<Int64>(heapPtr,arraySize);
    else
      rowsetRowCountArray_ = new (collHeapPtr_) NAArray<Int64>(collHeapPtr_, arraySize);
  }
  // index is assumed to be zero based. It is also assumed to be non-negative
  // it is OK if index is > arraySize, though this should 
  // not happen as arraySize is expected to be maximum rowset size
  rowsetRowCountArray_->insertAt((CollIndex)index, value);
}

Int64 ComDiagsArea::getValueFromRowsetRowCountArray (Lng32 index) const
{
  if ((rowsetRowCountArray_ == NULL) ||
      (index < 0) ||
      (!((rowsetRowCountArray_->used((CollIndex) index))))) {
    return -1;
  }

  return (*rowsetRowCountArray_)[(CollIndex) index];
}


// The Function Name
//
// Now we provide implementation for the functions that get and set
// the ``function name.''

void ComDiagsArea::setFunction(FunctionEnum newFunction)
{
   theSQLFunction_=newFunction;
}

ComDiagsArea::FunctionEnum ComDiagsArea::getFunction() const
{
   return (ComDiagsArea::FunctionEnum) theSQLFunction_;
}

// For the purpose of returning a function name, depending
// on the value of theSQLFunction_, we shall declare an
// array static (or private) to this source file.  This
// array has entries, one each, giving a char* that
// is the name of the SQL function represented by each of the
// FunctionEnum values.
//
// WARNING:  The entries in this array must correspond
//           with the definitions of the members of FunctionEnum.

static const char *const functionNames[ComDiagsArea::MAX_FUNCTION_ENUM] = {
    "NULL_FUNCTION",
    "ALLOCATE_CURSOR",
    "ALLOCATE_DESCRIPTOR",
    "ALTER_DOMAIN",
    "ALTER_TABLE",
    "CREATE_ASSERTION",
    "CREATE_CHARACTER_SET",
    "CLOSE_CURSOR",
    "CREATE_COLLATION",
    "COMMIT_WORK",
    "CONNECT",
    "DEALLOCATE_DESCRIPTOR",
    "DEALLOCATE_PREPARE",
    "DELETE_CURSOR",
    "DELETE_WHERE",
    "DESCRIBE",
    "SELECT",
    "DISCONNECT",
    "CREATE_DOMAIN",
    "DROP_ASSERTION",
    "DROP_CHARACTER_SET",
    "DROP_COLLATION",
    "DROP_DOMAIN",
    "DROP_SCHEMA",
    "DROP_TABLE",
    "DROP_TRANSLATION",
    "DROP_VIEW",
    "DYNAMIC_CLOSE",
    "DYNAMIC_DELETE_CURSOR",
    "DYNAMIC_FETCH",
    "DYNAMIC_OPEN",
    "DYNAMIC_UPDATE_CURSOR",
    "EXECUTE_IMMEDIATE",
    "EXECUTE",
    "FETCH",
    "GET_DESCRIPTOR",
    "GET_DIAGNOSTICS",
    "GRANT",
    "INSERT",
    "OPEN",
    "PREPARE",
    "REVOKE",
    "ROLLBACK_WORK",
    "CREATE_SCHEMA",
    "SET_CATALOG",
    "SET_CONNECTION",
    "SET_CONSTRAINT",
    "SET_DESCRIPTOR",
    "SET_TIME_ZONE",
    "SET_NAMES",
    "SET_SCHEMA",
    "SET_TRANSACTION",
    "SET_SESSION_AUTHORIZATION",
    "CREATE_TABLE",
    "CREATE_TRANSLATION",
    "UPDATE_CURSOR",
    "UPDATE_WHERE",
    "CREATE_VIEW"
};


const char * ComDiagsArea::getFunctionName () const
{
   return functionNames[theSQLFunction_];
}

// Class DiagsCondition Implementation
// ...is very simple indeed.  A very simple constructor
// and destructor, and some get/get methods for the diagsId_.

ComDiagsArea::DiagsCondition::DiagsCondition (CollHeap* ptr) :
                   ComCondition(ptr)
{ }

ComDiagsArea::DiagsCondition::DiagsCondition () :
                   ComCondition()
{ }

ComDiagsArea::DiagsCondition::~DiagsCondition ()
{
}

// And for the set/get methods:


void ComDiagsArea::DiagsCondition::setDiagsId(Lng32     newDiagsId)
{
   diagsId_ = newDiagsId;
}

Lng32     ComDiagsArea::DiagsCondition::getDiagsId () const
{
   return diagsId_;
}


// Finally, the IPC methods:

IpcMessageObjSize ComDiagsArea::DiagsCondition::packedLength ()
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength()
  // - packObjIntoMessage()
  // - unpackObj()
  // - checkObj()

  return ComCondition::packedLength()+sizeof(diagsId_);
}

IpcMessageObjSize ComDiagsArea::DiagsCondition::packedLength32()
{
  return ComCondition::packedLength32()+sizeof(diagsId_);
}

IpcMessageObjSize ComDiagsArea::DiagsCondition::packObjIntoMessage (
          IpcMessageBufferPtr buffer)
{
  return packObjIntoMessage(buffer, FALSE);
}
// To pack a DiagsCondition object, you just pack the
// the base class --- a ComCondition object --- and then pack in the
// diagsId_ member.  Easy, no?

IpcMessageObjSize ComDiagsArea::DiagsCondition::packObjIntoMessage (
          IpcMessageBufferPtr buffer, NABoolean swapBytes)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength()
  // - packObjIntoMessage()
  // - unpackObj()
  // - checkObj()

  IpcMessageObjSize  size = ComCondition::packObjIntoMessage(buffer, swapBytes);
  buffer += size;
  size += packIntoBuffer(buffer,diagsId_, swapBytes);
  return size;
}

IpcMessageObjSize ComDiagsArea::DiagsCondition::packObjIntoMessage32 (
          IpcMessageBufferPtr buffer)
{
  return packObjIntoMessage32(buffer, FALSE);
}
IpcMessageObjSize ComDiagsArea::DiagsCondition::packObjIntoMessage32 (
          IpcMessageBufferPtr buffer, NABoolean swapBytes)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength32()
  // - packObjIntoMessage32()
  // - unpackObj()
  // - checkObj()

  IpcMessageObjSize  size = ComCondition::packObjIntoMessage32(buffer, swapBytes);
  buffer += size;
  size += packIntoBuffer(buffer,diagsId_, swapBytes);
  return size;
}

void ComDiagsArea::DiagsCondition::unpackObj(IpcMessageObjType objType,
				    IpcMessageObjVersion objVersion,
				    NABoolean sameEndianness,
				    IpcMessageObjSize objSize,
				    IpcConstMessageBufferPtr buffer)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength()
  // - packObjIntoMessage()
  // - unpackObj()
  // - checkObj()

  ComCondition::unpackObj(objType,objVersion,sameEndianness,
			  objSize,buffer);
  buffer += objSize - sizeof(diagsId_);
  unpackBuffer(buffer,diagsId_);
}

void ComDiagsArea::DiagsCondition::unpackObj32(IpcMessageObjType objType,
				    IpcMessageObjVersion objVersion,
				    NABoolean sameEndianness,
				    IpcMessageObjSize objSize,
				    IpcConstMessageBufferPtr buffer)
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength()
  // - packObjIntoMessage()
  // - unpackObj()
  // - checkObj()

  ComCondition::unpackObj32(objType,objVersion,sameEndianness,
			    objSize,buffer);
  buffer += objSize - sizeof(diagsId_);
  unpackBuffer(buffer,diagsId_);
}

NABoolean
ComDiagsArea::DiagsCondition::checkObj(IpcMessageObjType t,
                                       IpcMessageObjVersion v,
                                       NABoolean sameEndianness,
                                       IpcMessageObjSize objSize,
                                       IpcConstMessageBufferPtr buffer) const
{
  // NOTE: changes to any of the following methods also require
  // corresponding changes in the others:
  // - packedLength()
  // - packObjIntoMessage()
  // - unpackObj()
  // - checkObj()

  const IpcConstMessageBufferPtr lastByte = buffer + objSize - 1;

  if (!ComCondition::checkObj(t, v, sameEndianness, objSize, buffer))
    return FALSE;

  if (!checkBuffer(buffer, sizeof(diagsId_), lastByte))
    return FALSE;

  return TRUE;
}

// Condition Addition
// The code in this section is the implementation for the ComDiagsArea
// members that allow the user to create and insert ComCondition
// objects.
//
// To create a new ComCondition we allocate a new DiagsCondition
// object on the proper heap.
//
// Note that the first condition's diagsId will be 1 (not 0).
//

ComCondition * ComDiagsArea::makeNewCondition()
{
   assert(newCondition_==NULL);
   newCondition_ = DiagsCondition::allocate(collHeapPtr_);
   assert(newCondition_ != NULL);
   newCondition_->setDiagsId(++maxDiagsId_);		// PREincrement
   newCondition_->setConditionNumber(newCondition_->getDiagsId());
   return newCondition_;
}

// All that this chunk's function must do is to free
// the current DiagsCondition object using the proper heap.
//

void ComDiagsArea::discardNewCondition()
{
    assert(newCondition_ != NULL);
    newCondition_->deAllocate();
    newCondition_ = NULL;
    maxDiagsId_--;
}

void ComDiagsArea::insertNewWarning()
{
  warnings_.insert(newCondition_);
}

void ComDiagsArea::insertNewEODWarning()
{
  warnings_.insert(newCondition_);
}

void ComDiagsArea::insertNewError()
{

  errors_.insert(newCondition_);
  // for non-atomic inserts, if any error is inserted after NonFatalErrorSeen flag
  // is set, then we want to unset the flag so that mainSQLCODE does not return 30022.
  if (getNonFatalErrorSeen())
    setNonFatalErrorSeen(FALSE);
}

// What we do here is:
// Make sure that newCondition_ is not NULL.
// If *newCondition_ is for SQLCODE of zero, assert a failure.
// If *newCondition_ is for a positive SQLCODE, add it to warnings_.
// Otherwise, *newCondition_ is for a negative SQLCODE, and
// add it to errors_.
// Set *newCondition_ to NULL.

void ComDiagsArea::acceptNewCondition()
{
   assert(newCondition_!=NULL);
   assert(newCondition_->getSQLCODE()!=0);
   if (newCondition_->getSQLCODE() < 0)
     insertNewError();
   else if (newCondition_->getSQLCODE() == 100)
     insertNewEODWarning();
   else
     insertNewWarning();

   newCondition_=NULL;
   enforceLengthLimit();
}

// Access to sequence of DiagsCondition (ComCondition plus diagsId sequencenum)
//
// This operator makes it look like ComDiagsAreas are indexed 1..getNumber()
// while the internal errors_/warnings_ lists are indexed from 0..entries()-1.
//
// This function used to return all errors entries before all warnings,
// regardless of their actual arrival sequence; this was motivated by the
// desire to be able to output completed conditions by severity.
// Unfortunately, this led to problems when in the midst of assembling a
// *new* (not yet complete) condition when previous conditions were an
// interleaved mixture of warnings and errors:  the current condition's
// DgBase parameters are assigned to the comDiags[comDiags.getNumber()]
// condition, which would not necessarily be the current condition,
// but the last warning in the area.
//
// Now, this function returns entries by arrival sequence,
// *even if this means a warning is returned (to be output) before an error*.
// (Contrast this behavior to mainSQLCode().)
//

ComCondition &ComDiagsArea::operator[] (Lng32 index) const
{
   CollIndex numErrors   = errors_.entries();
   CollIndex numWarnings = warnings_.entries();
   assert(index > 0);
   assert((CollIndex)index <= numErrors + numWarnings);
   index--; // convert to 0 based
   // If only one of the lists is populated, fast lookup of index'th condition.
   if (numErrors == 0)
     return *warnings_[index];
   else if (numWarnings == 0)
     return *errors_[index];

   // diagsId is unique among all conditions
   // the conditions in errors_[] warnings_[] are sorted on diags Id
   // respectively, because conditions are created with a incrementing
   // diags Id counter and appended to the array.
   // sometimes, the conditions are removed, so their assigned diags Id is no
   // more continuous. This is why we need to find the right condition
   // by searching through the two arrays, relying on the diags Id in
   // the conditions as the order.
   CollIndex errorIdx = 0;
   CollIndex warnIdx = 0;
   Lng32 pos = 0;

   while(pos < index){
     if(errorIdx == numErrors)
       warnIdx++;
     else if(warnIdx == numWarnings)
       errorIdx++;
     else{
       if(errors_[errorIdx]->getDiagsId() < 
	  warnings_[warnIdx]->getDiagsId())
	 errorIdx++;
       else if(errors_[errorIdx]->getDiagsId() > 
	       warnings_[warnIdx]->getDiagsId())
	 warnIdx++;
       else {
	 //assert("duplicated diagsId" == 0);
	 //return *errors_[0];
	 warnIdx++;
       }
     }
     pos++;
   }
   
   if(errorIdx == numErrors)
     return *warnings_[warnIdx];
   else if(warnIdx == numWarnings)
     return *errors_[errorIdx];
   else{
     if(errors_[errorIdx]->getDiagsId() < 
	warnings_[warnIdx]->getDiagsId())
       return *errors_[errorIdx];
     else if(errors_[errorIdx]->getDiagsId() > 
	     warnings_[warnIdx]->getDiagsId())
       return *warnings_[warnIdx];
     else {
       //assert("duplicated diagsId" == 0);
       //return *errors_[0];
       return *warnings_[warnIdx];
     }
   }
}

// the following 2 methods retrieve the warning/error ComCondition at 
// warnings_[index] or errors_[index]
// index ranges from 1..getNumber(DgSqlCode::WARNING_ or ERROR_)
// MARIA
ComCondition*  ComDiagsArea::getWarningEntry(Lng32 index) {
  assert((warnings_.entries() >= (CollIndex)index) && (index > 0));
  return warnings_[--index]; 
}

ComCondition* ComDiagsArea::getErrorEntry(Lng32 index) {
  assert((errors_.entries() >= (CollIndex)index) && (index > 0));
  return errors_[--index];
}

ComCondition* ComDiagsArea::findCondition(Lng32 sqlCode, Lng32 *entryNumber)
{
  if (sqlCode == 0)
    return NULL;

  ComCondition * c = NULL;
  CollIndex i = 0;

  if (sqlCode > 0)
    {
      for (i = 0; i < warnings_.entries(); ++i)
        if (warnings_[i]->getSQLCODE() == sqlCode)
          {
            c = warnings_[i];
            if (entryNumber != NULL)
                *entryNumber = i;
            break;
          }
      return c;
    }

  // sqlCode < 0

  for (i = 0; i < errors_.entries(); ++i)
    if (errors_[i]->getSQLCODE() == sqlCode)
      {
        c = errors_[i];
        if (entryNumber != NULL)
           *entryNumber = i;
        break;
      }
 return c;
}



// The clearConditionsOnly method just deletes and clears each element of
// both sequences (errors_ and warnings_), and resets the maxDiagsId_.

void ComDiagsArea::clearConditionsOnly()
{
   DiagsCondition  *ptr;
   while (errors_.getFirst(ptr))   ptr->deAllocate();
   while (warnings_.getFirst(ptr)) ptr->deAllocate();
   maxDiagsId_ = 0;
}

// The clear method just deletes and clears each element of
// both sequences (errors_ and warnings_) and additionally sets things
// like areMore_ to false (NO_MORE).

void ComDiagsArea::clear()
{
   clearConditionsOnly();
   areMore_=ComCondition::NO_MORE;
   flags_ = 0;
   rowCount_ = 0;
   avgStreamWaitTime_ = -1;
   cost_ = 0.0;
   if (rowsetRowCountArray_ != NULL) {
    rowsetRowCountArray_->deallocate();
    rowsetRowCountArray_ = NULL;
   }

}

void ComDiagsArea::clearErrorConditionsOnly()
{
   DiagsCondition  *ptr;
   while (errors_.getFirst(ptr)) 
   {
     ptr->deAllocate();
     --maxDiagsId_;
   }
}

void ComDiagsArea::clearWarnings()
{
   DiagsCondition  *ptr;
   while (warnings_.getFirst(ptr)) 
   {
     ptr->deAllocate();
     --maxDiagsId_;
   }
}

// Returnes the SQLSTATE value of the last SIGNAL statement.
// Assumes the SIGNAL condition is the highest priority error.
const char *ComDiagsArea::getSignalSQLSTATE() const
{
  if (errors_.entries() > 0) {
    ComCondition *signal = errors_[0];

    if (signal->getSQLCODE() == -ComDiags_SignalSQLCODE)
      return signal->getOptionalString(0);
  }
  return NULL;
}
  
// The code for mainSQLCODE is rather straightforward
// (see the .h file for a specification).
// This value is what GET DIAGNOSTICS CONDITION_NUMBER=1 should return
// (all other conditionNumber's are implementation-dependent, so can be random).

Lng32 ComDiagsArea::mainSQLCODE() const
{
   if (errors_.entries() > 0) {
      if (containsError(-EXE_CANCELED))
        return -EXE_CANCELED;
      else if (errors_[0]->getSQLCODE() == -EXE_INTERNALLY_GENERATED_COMMAND && errors_.entries() > 1)
        return errors_[1]->getSQLCODE();
      else if (errors_[errors_.entries()-1]->getSQLCODE() == -EXE_NONATOMIC_FAILURE_LIMIT_EXCEEDED)
        return -EXE_NONATOMIC_FAILURE_LIMIT_EXCEEDED;
      else if (getNonFatalErrorSeen()) {
	if (containsWarning(EXE_NONFATAL_ERROR_SEEN))
	  return EXE_NONFATAL_ERROR_SEEN;
	else
	  return EXE_NONFATAL_ERROR_ON_ALL_ROWS;
      }
      else
		return errors_[0]->getSQLCODE();
   }
   else if (warnings_.entries() > 1 && warnings_[0]->getSQLCODE() == EXE_INTERNALLY_GENERATED_COMMAND)
      return warnings_[1]->getSQLCODE();
   else if (warnings_.entries() > 0)
      return warnings_[0]->getSQLCODE();
   else
      return 0L;
}

// Negation of ComConditions
//
// Negation of a condition in a ComDiagsArea is somewhat tricky,
// and definitely possible.  Here's the algorithm we implement:
//
// Assert that the index is in range.
//
// Assert that the ComCondition, c, referenced by index
// is not locked yet.
//
// Remove c from the list (errors, warnings) that it is currently
// resident in.
//
// Negate the value of the SQLCODE member of c.
//
// Insert c into the other list (warnings,errors) than the one
// from which it came.  Do the insertion preserving the ascending
// ordering of the list based on the DiagsId as a key.
//
// It is the insertion into the ordered list that is the trickiest part ---
// but definitely possible.

void ComDiagsArea::negateCondition(CollIndex index)
{
   CollIndex numErrorEntries = errors_.entries();
   CollIndex numWarningEntries = warnings_.entries();
   assert(numErrorEntries+numWarningEntries != 0);//index invalid if this fails
   assert(index < numErrorEntries+numWarningEntries);
   NABoolean sourceIsWarning =
                  (numErrorEntries==0 || (index > numErrorEntries-1));
   LIST(DiagsCondition*)   &source = sourceIsWarning ? warnings_ : errors_;
   LIST(DiagsCondition*)   &dest   = sourceIsWarning ? errors_ : warnings_;
   if (sourceIsWarning) index -= numErrorEntries;

   // We declare ptr and do a lookup, and then a removeAt to
   // extract the pointer to the ComCondition in question from
   // the source list.
   //

   DiagsCondition   *ptr = source[index];

   NABoolean removed = source.removeAt(index);
   assert(removed);

   assert(ptr);
   ptr->isLocked_   = FALSE;
   ptr->theSQLCODE_ = -ptr->theSQLCODE_;

   // We presume/know that dest is ordered in an ascending order
   // by DiagsId.  We want to add ptr to dest preserving this ordering.
   //
   // We can use a loop to determine an index such that an insertion will
   // preserve ordering.  Then, we simply insert.
   //
   // Fortunately (for us in this chunk) the insertAt of NAList
   // does indeed ``slide down'' all members of the list that
   // are beyond the insertion point.
   //
   // When the loop terminates, we want that all of the elements
   // of dest which have a DiagsId less than the DiagsId of
   // *ptr are before the place where insertionPoint points.
   // Note that no two DiagsConditions should have the same DiagsId
   // value, if they are resident in the same ComDiagsArea.

   CollIndex insertionPoint = 0;
   while (insertionPoint != dest.entries() &&
          dest[insertionPoint]->getDiagsId() < ptr->getDiagsId())
            insertionPoint++;
   dest.insertAt(insertionPoint,ptr);
}

// Merging of ComDiagsAreas
// See the header file for the specification for what this
// function does.
//
// Steps
// =====
// Assert that the function enums are the same for this and source.
//
// Copy conditions from source into this object in source insertion sequence,
// using the [] operator.
//
// Add source's rowCount_ to this rowCount_.
// 
// Replace avgStreamWaitTime_ with source's, if valid.

void ComDiagsArea::mergeAfter(const ComDiagsArea& source)
{
  Lng32 nfMark = -1;
   if (this == &source) return;
  
   // if NAR errors are flowing up the tree avoid merging them if
   // they are the same.
   if (((source.mainSQLCODE() == EXE_NONFATAL_ERROR_SEEN)  &&
	(mainSQLCODE() == EXE_NONFATAL_ERROR_SEEN)) ||
       ((source.mainSQLCODE() == EXE_NONFATAL_ERROR_ON_ALL_ROWS)  &&
	 (mainSQLCODE() == EXE_NONFATAL_ERROR_ON_ALL_ROWS)))     
     {
       if ((source.getNumber() <= getNumber()))
	 return ;
       else	
	 nfMark = ((ComDiagsArea *)&source)->markDupNFConditions();
     }
   
   assert(   newCondition_ == NULL );

   // I, (Bill), am commenting this out because with our current
   // interfaces between subcomponents of the architecture it is
   // not always desirable to perform a gratuitous SQL-FUNCTION change
   // just to keep this assert from failing.  Conceptually, however,
   // it makes sense to me that merging two diags areas only should
   // work if they apply to the same SQL-FUNCTION. :-)
   //
   //   assert( theSQLFunction_ == source.theSQLFunction_);

   // if lengthLimit of source is greater than that of target
   // then the  target lengthLimit is assigned that of the source. 
   // Note that NO_LIMIT_ON_ERROR_CONDITIONS
   // is the largest value lengthLimit can have, even though this enum = -1.
   if (source.lengthLimit_ == ComCondition::NO_LIMIT_ON_ERROR_CONDITIONS) {
     lengthLimit_ = ComCondition::NO_LIMIT_ON_ERROR_CONDITIONS;
   }
   else if ((lengthLimit_ != ComCondition::NO_LIMIT_ON_ERROR_CONDITIONS) && 
	    (source.lengthLimit_ > lengthLimit_)) {
    lengthLimit_ = source.lengthLimit_ ;
   }

   //    changed to preserve insertion order of the conditions

   for (Int32 index = (((nfMark != -1) ? (nfMark+1):1)); index <= source.getNumber(); index++)
   {
      ComCondition * newCondition = this->makeNewCondition();
      
      *newCondition = source[index];
      this->acceptNewCondition();
   }

   rowCount_ += source.rowCount_;

   if (source.avgStreamWaitTime_ != -1)
    avgStreamWaitTime_ = source.avgStreamWaitTime_ ;

   if (areMore_ == ComCondition::NO_MORE)
     areMore_ = source.areMore_;

   enforceLengthLimit();

   flags_ = source.flags_;

   assert((rowsetRowCountArray_ == NULL) || (source.rowsetRowCountArray_ == NULL));
   if (source.rowsetRowCountArray_) {
     rowsetRowCountArray_ = new (collHeapPtr_) 
       NAArray<Int64>(*(source.rowsetRowCountArray_), collHeapPtr_);
   }

}

//
// Helper routine to copy a zero-terminated string to
// particular heap.
//
/*
static char *copyStringMember(char* src, CollHeap *heap)
{
  if (src==NULL) return src;

  char *copy;
  unsigned buffsize = str_len(src) + 1;

  copy = (char*) heap->allocateMemory(buffsize);
  assert(copy!=NULL);

  str_cpy(copy,src,buffsize);
  copy[buffsize-1]=0;

  return copy;
}
*/

ComDiagsArea::DiagsCondition* ComDiagsArea::DiagsCondition::copy()
{
  DiagsCondition *diagsCond = DiagsCondition::allocate(collHeapPtr_);
  assert(diagsCond!=NULL);

  *diagsCond = *this;

  return diagsCond;
}

ComDiagsArea* ComDiagsArea::copy()
{
  ComDiagsArea* diagsArea = ComDiagsArea::allocate(collHeapPtr_);
  assert(diagsArea!=NULL);

  diagsArea->collHeapPtr_ = collHeapPtr_;
  diagsArea->newCondition_ = newCondition_;
  diagsArea->areMore_ = areMore_;
  diagsArea->rowCount_ = rowCount_;
  diagsArea->avgStreamWaitTime_ = avgStreamWaitTime_;
  diagsArea->cost_ = cost_;
  diagsArea->theSQLFunction_ = theSQLFunction_;
  diagsArea->maxDiagsId_ = maxDiagsId_;
  diagsArea->flags_ = flags_;

  CollIndex i = 0;
  for (i=0; i < warnings_.entries(); i++)
    diagsArea->warnings_.insert(warnings_[i]->copy());

  for (i=0; i < errors_.entries(); i++)
    diagsArea->errors_.insert(errors_[i]->copy());

  if (rowsetRowCountArray_) {
     diagsArea->rowsetRowCountArray_ = new (collHeapPtr_) 
       NAArray<Int64>(*(rowsetRowCountArray_), collHeapPtr_);
   }
  else
    diagsArea->rowsetRowCountArray_ = NULL;


  return diagsArea;
}





// Marking and Rewinding
//
// The marking function can actually do what it is really supposed to:
// just return the value of the maxDiagsId_ member function.
// In the case that there is a current newCondition_ object (i.e.,
// that pointer is not NULL) then the returned value should be
// maxDiagsId_ - 1.

Lng32 ComDiagsArea::mark() const
{
   if (newCondition_ == NULL)
     return maxDiagsId_;
   else
     return maxDiagsId_-1;
}

// It iterates over the two linked lists,
// and deletes the tail portions that are ComConditioins
// with diags-id values greater
// than the given mark value.
//
// Alternative:  Under the category of ``road not taken''
// we have the following idea.  Have a data member of ComDiagsArea
// that is a stack of markValues, where the top is the most recently
// obtained mark.  When rewind is called, it checks the stack
// to see if the stack contains the given mark value.  If the the mark value
// is found to be valid (by being on the stack) then
// the stack is popped to just below the given mark value, and
// the deletion is performed.  Otherwise (and this is the big reason
// for having the stack in any case) we assertion fail since the mark
// is invalid.
//
// One more twist on this scheme that might be a further improvement
// would be to make the mark value an index into the sequence which
// is the stack.
//

void ComDiagsArea::rewind(Lng32 markValue, NABoolean decId)
{
   CollIndex  maxError = errors_.entries()-1;
   while (maxError != -1 && errors_[maxError]->getDiagsId() > markValue) {
       errors_[maxError]->deAllocate();
       NABoolean removed = errors_.removeAt(maxError--); 
       assert(removed);
       if (decId) --maxDiagsId_; // This is the merged line
   }
   CollIndex maxWarning = warnings_.entries()-1;
   while (maxWarning != -1 && warnings_[maxWarning]->getDiagsId() > markValue){
       warnings_[maxWarning]->deAllocate();
       NABoolean removed = warnings_.removeAt(maxWarning--);
       assert(removed);
       if (decId) --maxDiagsId_;
   }
}

// The algorithm for this function is two steps:
//   1) if the destPtr validly does not point to *this, then append/copy
//      the to-be-discarded ComConditions from this to *destPtr,
//      preserving order (and not reversing it, which would be easy to do).
//   2) perform the rewind of "this," discarding certain ComCondition objects.
//
// For step 1 we use a pair of for loops, the first for errors and the
// second for warnings.  Not that it should matter, but this WILL result
// in the ComCondition objects of *destPtr having a different chronological
// ordering than the ComConditions have originally in *this.  However,
// that should be of no consequence since, to those outside a ComDiagsArea,
// the ComConditions are always ordered by priority, with all errors before
// all warnings.

void ComDiagsArea::rewindAndMergeIfDifferent(Lng32 markValue,
					     ComDiagsArea  *destPtr)
{
  if (destPtr != NULL && destPtr != this) {
    CollIndex     i;
    for (i=0; i!=errors_.entries();i++)
      if (errors_[i]->getDiagsId() > markValue) {
	// copy/append errors_[i] to destPtr->errors_

	DiagsCondition *newPtr =
	  DiagsCondition::allocate(destPtr->collHeapPtr_);

	*newPtr = *(errors_[i]);
        newPtr->setDiagsId(++destPtr->maxDiagsId_);
	destPtr->errors_.insert(newPtr);
      }
    for (i=0; i!=warnings_.entries();i++)
      if (warnings_[i]->getDiagsId() > markValue) {
	// copy/append warnings_[i] to destPtr->warnings_
	DiagsCondition *newPtr =
	  DiagsCondition::allocate(destPtr->collHeapPtr_);

	*newPtr = *(warnings_[i]);
        newPtr->setDiagsId(++destPtr->maxDiagsId_);
	destPtr->warnings_.insert(newPtr);
      }
  rewind(markValue);
  }
}

void ComDiagsArea::deleteWarning(Lng32 entryNumber) {
  warnings_[entryNumber]->deAllocate();
  NABoolean removed = warnings_.removeAt(entryNumber);
  assert(removed);
  --maxDiagsId_;
}


void ComDiagsArea::deleteError(Lng32 entryNumber) {
  errors_[entryNumber]->deAllocate();
  NABoolean removed = errors_.removeAt(entryNumber);
  assert(removed);
  --maxDiagsId_;
}

ComCondition * ComDiagsArea::removeError(Lng32 entryNumber) {
  ComCondition * errCond = getErrorEntry(entryNumber+1);
  NABoolean removed = errors_.removeAt(entryNumber);
  assert(removed);
  return errCond ;
}


void ComCondition::destroyMe()
{
   this -> ~ComCondition();
}

void ComDiagsArea::destroyMe()
{
   this -> ~ComDiagsArea();
}

void ComDiagsArea::DiagsCondition::destroyMe()
{
  this -> DiagsCondition::~DiagsCondition();
}

ComDiagsArea::DiagsCondition &
ComDiagsArea::DiagsCondition::operator= (const ComDiagsArea::DiagsCondition&d)
{
    ComCondition::operator=(d);
    diagsId_ = d.diagsId_;
    return *this;
}

void ComDiagsArea::removeFinalCondition100()
{
    // Might not have a SQL_EOF condition, because these
    // are not used on NT.

  if (warnings_.entries() == 0 ) 
    {
      return;
    }

  const CollIndex i = warnings_.entries() - 1;
  DiagsCondition *w100 = warnings_[i];
  if ((w100->getDiagsId() != maxDiagsId_ ) ||
      (w100->getSQLCODE() != 100))
    {
      return;
    }

  w100->deAllocate();
  warnings_.removeAt(i);
  --maxDiagsId_;
  return;
}

void ComDiagsArea::removeLastErrorCondition()
{
    // Might not have a SQL_EOF condition, because these
    // are not used on NT.

  if (errors_.entries() == 0 ) 
    {
      return;
    }

  const CollIndex i = errors_.entries() - 1;
  DiagsCondition *errCond = errors_[i];
  if (errCond->getDiagsId() != maxDiagsId_ ) 
    {
      return;
    }

  deleteError((Lng32) i);
  return;
}


///////////////////////////////////////////////////////////////
// returns TRUE, if any ComCondition in the diagsArea contains
// error SQLCode.
// returns FALSE, otherwise.
///////////////////////////////////////////////////////////////
NABoolean ComDiagsArea::contains(Lng32 SQLCode) const
{

  return containsError(SQLCode) || containsWarning(SQLCode);

} // end of ComDiagsArea::contains

NABoolean ComDiagsArea::containsError(Lng32 SQLCode) const
{
  for ( CollIndex i=0; i < errors_.entries(); i++ )
    {
      if ( errors_[i]->getSQLCODE() == SQLCode )
        {
          return TRUE;
        }
    }
  return FALSE;
}

NABoolean ComDiagsArea::containsWarning(Lng32 SQLCode) const
{
  return containsWarning(0, SQLCode);
}

// Check if warnings_ contains SQLCODE within the range [begin, warnings_.entries()). 
// Note begin is 0-based.
NABoolean ComDiagsArea::containsWarning(CollIndex begin, Lng32 SQLCode) const
{
  for ( CollIndex i=begin; i<warnings_.entries(); i++ )
    {
      if ( warnings_[i]->getSQLCODE() == SQLCode )
        {
          return TRUE;
        }
    }
   return FALSE;
}

  // returns TRUE, if diagsArea contains the fileName for error SQLCode.
  // returns FALSE, otherwise.
NABoolean ComDiagsArea::containsForFile(Lng32 SqlCd, const char * fileName)
{
	 ComDiagsArea* DA = ComDiagsArea::copy();
    if (!(*DA).contains(SqlCd))
		 return FALSE;


	 NABoolean fileNameExists = FALSE;

	 //Loop through Diagnostic area to check if extFileSetName is
	 //already present.
	 for (Lng32 j = 1 ; ((j <= (*DA).getNumber()) && (!(fileNameExists))); j++)
	 {
	     if( ((*DA)[j].getSQLCODE() == SqlCd) && 
		 ((strcmp(fileName, ((*DA)[j].getTableName()))) == 0) )
	     {
		 fileNameExists = TRUE;
	     }
	 }
	 return fileNameExists;
}

//returnIndex returns the index number of a given SQLCODE in this diagsarea
//If the given SQLCODE is not found in the diagsarea then NULL_COLL_INDEX is returned.
//The index is zero based. The index number for warnings is obtained by
//incrementing the array position of the particular SQLCODE by the total
//number of errors in this diagsarea. The CollIndex value returned is in a form suitable
//to be used by a method like ComDiagsArea::negateCondition(CollIndex ), if it is
//not equal to NULL_COLL_INDEX. 
CollIndex ComDiagsArea::returnIndex(Lng32 SQLCode) const
{
CollIndex i;
for ( i=0; i < errors_.entries(); i++ )
    {
      if ( errors_[i]->getSQLCODE() == SQLCode )
        {
          return i;
        }
    } 

  for ( i = 0; i < warnings_.entries(); i++ )
    {
      if ( warnings_[i]->getSQLCODE() == SQLCode )
        {
          return i + errors_.entries();
        }
    }
  return NULL_COLL_INDEX ;
} // end of ComDiagsArea::returnIndex




void ComDiagsArea::removeLastNonFatalCondition()
{
  if (warnings_.entries() == 0 ) 
    {
      return;
    }
  
  const CollIndex i = warnings_.entries() - 1;
  DiagsCondition *wNF = warnings_[i];
  
  if ((wNF->getDiagsId() != maxDiagsId_ ) ||
      ((wNF->getSQLCODE() != EXE_NONFATAL_ERROR_SEEN) &&
       (wNF->getSQLCODE() != EXE_NONFATAL_ERROR_ON_ALL_ROWS))
      )
    {
      return;
    }

  wNF->deAllocate();
  warnings_.removeAt(i);
  --maxDiagsId_;
  setNonFatalErrorSeen(FALSE);
  return;     
}

Lng32 ComDiagsArea::markDupNFConditions()
{ 
  CollIndex lnfw = 0;
  DiagsCondition *warningMark = NULL;

 if ((lnfw = returnIndex(EXE_NONFATAL_ERROR_ON_ALL_ROWS)) != NULL_COLL_INDEX)
    warningMark = warnings_[lnfw-errors_.entries()];
  else 
    if ((lnfw=returnIndex(EXE_NONFATAL_ERROR_SEEN)) != NULL_COLL_INDEX)
      warningMark= warnings_[lnfw-errors_.entries()];

 if(warningMark)
   return warningMark->getDiagsId();
 else
   return -1;
	      
  
}
// -----------------------------------------------------------------------
// ComDiagsTranslator::translateDiagsArea
//    A driver for the translation of individual conditions in a diags area
// -----------------------------------------------------------------------

void ComDiagsTranslator::translateDiagsArea 
                          ( ComDiagsArea &diags, 
                            const NABoolean twoPass)
{
  Int32 index;

  if (twoPass)
  {
    // First, run through the individual conditions and call the virtual
    // condition analyzer method, to allow our caller to figure out
    // what's in the diags area before translating conditions.
    firstError_ = TRUE;
    beforeAnalyze();
    for (index = 1; index <= diags.getNumber(); index++)
    {
      analyzeCondition (diags[index]);
      firstError_ = FALSE;
    }
  }

  ComDiagsArea * localDiags = diags.copy();
  diags.clearConditionsOnly ();
  firstError_ = TRUE;
  beforeTranslate();
  for (index = 1; index <= localDiags->getNumber(); index++)
  {
    // Run through the individual conditions, call the 
    // virtual condition translation method for each condition
    ComCondition &condition = (*localDiags)[index];
    if (!translateCondition (diags, condition))
    {
      // the condition translator method didn't translate the condition,
      // copy it unchanged
      ComCondition * newCondition = diags.makeNewCondition();
      *newCondition = condition;
      diags.acceptNewCondition();
    }
    firstError_ = FALSE;
  }
  afterTranslate();
  // deallocate the copy
  localDiags->deAllocate();
}


// -----------------------------------------------------------------------
// ComDiagsTranslator::analyzeCondition
//    Don't implement - derived classes that want to use this one
//    must provide it themselves
// -----------------------------------------------------------------------

void ComDiagsTranslator::analyzeCondition (const ComCondition &cond)
{ assert(0); }


// -----------------------------------------------------------------------
// Virtual methods, to be called before the first condition is processed.
// Derived classes that want to know about this can redefine.
// -----------------------------------------------------------------------

void ComDiagsTranslator::beforeAnalyze (void) {};
void ComDiagsTranslator::beforeTranslate (void) {};

// -----------------------------------------------------------------------
// Virtual method, to be called after the last condition is processed.
// Derived classes that want to know about this can redefine.
// Note that there is no "afterAnalyze" since that would be identical to
// "beforeTranslate".
// -----------------------------------------------------------------------

void ComDiagsTranslator::afterTranslate (void) {};


