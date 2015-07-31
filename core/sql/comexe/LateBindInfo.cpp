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
* File:         LateBindInfo.cpp
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

#include "ComPackDefs.h"
#include "LateBindInfo.h"
#include "NAMemory.h"
#include "str.h"
#include "exp_stdh.h"
#include "exp_tuple_desc.h"
#include "ComQueue.h"

#if !defined(__EID) && !defined(ARKFS_OPEN)
#include "ComResWords.h"
#include "ComDistribution.h"
#endif
//////////////////////////////////////////////////////////////////
//    Constructor: UninitializedMvName
//    Description:
//      Initialize class member variables.                             
//////////////////////////////////////////////////////////////////
UninitializedMvName::UninitializedMvName()
{ 
    physicalName_[0]=0; 
    ansiName_[0]=0;
}
//////////////////////////////////////////////////////////////////
//    Method: setPhysicalName
//    Description:
//      This method will copy the given parameter into the
//      class member variable physicalName_                               
//////////////////////////////////////////////////////////////////
void UninitializedMvName::setPhysicalName( const char *physicalName )
{
    assert( physicalName );
    assert( str_len(physicalName) <= MAX_PHYSICAL_NAME_LENGTH );

    strcpy( physicalName_, physicalName );
}
//////////////////////////////////////////////////////////////////
//    Method: setAnsiName
//    Description:
//      This method will copy the given parameter into the
//      class member variable ansiName_                        
//////////////////////////////////////////////////////////////////
void UninitializedMvName::setAnsiName( const char *ansiName )
{
    assert( ansiName );
    assert( str_len(ansiName) <= MAX_ANSI_NAME_LENGTH );

    strcpy( ansiName_, ansiName );
}


// LCOV_EXCL_START
// exclude from code coverage analysis since it is not called from anywhere
NABoolean LateNameInfo::makeSQLIdentifier(char * invalue, 
					  char * outvalue)
{
  NABoolean dQuoteSeen = FALSE;
  
  UInt32 invalueLen = str_len(invalue);
  UInt32 i = 0;
  for (i = 0; i < invalueLen; i++)
    {
      if (invalue[i] == '"')
	dQuoteSeen = NOT dQuoteSeen;

      if (dQuoteSeen)
	outvalue[i] = invalue[i];
      else
#pragma nowarn(1506)   // warning elimination 
	outvalue[i] = TOUPPER(invalue[i]);
#pragma warn(1506)  // warning elimination 
    }

  // remove trailing blanks
  i = invalueLen - 1;
  while (outvalue[i] == ' ')
    i--;

  outvalue[i + 1] = 0;

  return TRUE;
}
// LCOV_EXCL_STOP

// return code: TRUE, if error. FALSE, if all is ok.
// This method assumes that the parts array passed in has 
// larger size than the number of parts in 'inName'. If this 
// cannot be guaranteed it is best to call this method with
// a parts array of size at least 4, if 'inName' is a pointing
// to output of the convertTableName method, because convertTableName
// will raise INVALID_SQL_ID error if a name has more than 4 parts.
   
NABoolean LateNameInfo::extractParts
  (const char * inName,  // IN: inName separated by "."s
   char * outBuffer,     // IN/OUT: space where parts will be moved.
                         // Must be allocated by caller
   Lng32 &numParts,       // OUT: number of parts extracted
   char * parts[],       // IN/OUT: array entries initialized to parts on return
   NABoolean dQuote)     // IN: if TRUE, parts are double quoted.
{
  Int32 len = str_len(inName);
  numParts = 0;
  Int32 outPos = 0;
  parts[numParts++] = &outBuffer[outPos];
  if (dQuote)
    outBuffer[outPos++] = '"';
  for (Int32 inPos = 0; inPos < len; inPos++)
    {
      if (inName[inPos] == '.')
	{
	  if (dQuote)
	    outBuffer[outPos++] = '"';
	  outBuffer[outPos++] = 0;
	  parts[numParts++] = &outBuffer[outPos];
	  if (dQuote)
	    outBuffer[outPos++] = '"';
	}
      else
	{
	  outBuffer[outPos++] = inName[inPos];
	}
    }
  if (dQuote)
    outBuffer[outPos++] = '"';
  outBuffer[outPos++] = 0;
  return FALSE;
}

// LCOV_EXCL_START
// exclude from code coverage analysis since it is not called from anywhere
static void extractPartsLocal(char * invalue, char *inVal[], short inValLen[])
{
  // apply defaults to invalue
  UInt32 invalueLen = str_len(invalue);
#pragma nowarn(1506)   // warning elimination 
  Int32 i = invalueLen-1;
#pragma warn(1506)  // warning elimination 
  Int32 j = 2;
  Int32 k = 0;
  for (; i >= 0; i--)
    {
      if (invalue[i] == '.')
	{
	  inVal[j] = &invalue[i+1];
#pragma nowarn(1506)   // warning elimination 
	  inValLen[j] = k;
#pragma warn(1506)  // warning elimination 
	  k = 0;
	  j--;
	}
      else
	k++;
    }
  inVal[j] = &invalue[i+1];
#pragma nowarn(1506)   // warning elimination 
  inValLen[j] = k;
#pragma warn(1506)  // warning elimination 

}

NABoolean LateNameInfo::applyMPAliasDefaults(char * invalue, 
					     char * outvalue,
					     char * defValString)
{

  char * defVal[3] = {NULL, NULL, NULL};
  short defValLen[3] = {0, 0, 0};
  char * inVal[3] = {NULL, NULL, NULL};
  short inValLen[3] = {0, 0, 0};

  // extract cat and schema name from defValString
  extractPartsLocal(defValString, defVal, defValLen);

  // extract parts from invalue
  extractPartsLocal(invalue, inVal, inValLen);

  // fill in the missing parts of invalue and create outvalue
  Int32 outlen=0;
  if (inVal[0] == NULL)
    {
      outlen = defValLen[0] + 1;
      str_cpy_all(outvalue, defVal[0], outlen);
    }
  else
    {
      outlen = inValLen[0] + 1;
      str_cpy_all(outvalue, inVal[0], outlen);
    }

  if (inVal[1] == NULL)
    {
      str_cpy_all(&outvalue[outlen], defVal[1], defValLen[1]+1);
      outlen += defValLen[1]+1;
    }
  else
    {
      str_cpy_all(&outvalue[outlen], inVal[1], inValLen[1]+1);
      outlen += inValLen[1]+1;
    }

  str_cpy_all(&outvalue[outlen], inVal[2], inValLen[2]);
  outlen += inValLen[2];

  outvalue[outlen] = 0;

  return TRUE;
}
// LCOV_EXCL_STOP

Long LateNameInfoList::pack(void *space)
{
#pragma nowarn(1506)   // warning elimination 
  lateNameInfo_.pack(space,numEntries_);
#pragma warn(1506)  // warning elimination 
  return NAVersionedObject::pack(space);
}

Lng32 LateNameInfoList::unpack(void * base, void * reallocator)
{
#pragma nowarn(1506)   // warning elimination 
  if(lateNameInfo_.unpack(base,numEntries_,reallocator)) return -1;
#pragma warn(1506)  // warning elimination 
  return NAVersionedObject::unpack(base, reallocator);
}

// returns the length of total info that needs to be sent to compiler
// at recomp time. This info is used to get to the actual tablename
// (and not the prototype name) that was specified thru a hvar/param/env
// var.
ULng32 LateNameInfoList::getRecompLateNameInfoListLen()
{
  ULng32 numEntriesToSend = 0;
  for (ULng32 i = 0; i < getNumEntries(); i++)
    {
      if (lateNameInfo_[i]->isVariable())
	numEntriesToSend++;
    }
  
  if (numEntriesToSend > 0)
    return sizeof(RecompLateNameInfoList) +
      ((numEntriesToSend-1) * sizeof(RecompLateNameInfo));
  else
    return 0;
}

// puts recomp info into 'buffer'. Space is to be allocated by caller.
void LateNameInfoList::getRecompLateNameInfoList(char * buffer)
{
  RecompLateNameInfoList * rlnil = (RecompLateNameInfoList *)buffer;

  Int32 j = 0;
  for (UInt32 i = 0; i < getNumEntries(); i++)
    {
      if (lateNameInfo_[i]->isVariable())
	{
	  str_cpy_and_null(rlnil->getRecompLateNameInfo(j).varName(),
			   lateNameInfo_[i]->variableName(),
			   str_len(lateNameInfo_[i]->variableName()) + 1);
	  
	  str_cpy_and_null(rlnil->getRecompLateNameInfo(j).compileTimeAnsiName(),
			   lateNameInfo_[i]->compileTimeAnsiName(),
			   str_len(lateNameInfo_[i]->compileTimeAnsiName())+1);

	  str_cpy_and_null(rlnil->getRecompLateNameInfo(j).actualAnsiName(),
			   lateNameInfo_[i]->lastUsedExtAnsiName(),
			   str_len(lateNameInfo_[i]->lastUsedExtAnsiName())+1);
	
#pragma nowarn(1506)   // warning elimination 
	  rlnil->getRecompLateNameInfo(j).setMPalias(lateNameInfo_[i]->isMPalias());
#pragma warn(1506)  // warning elimination 
	
	  j++;
        } // if
    
    } // for
  
  if (j > 0)
    rlnil->numEntries() = j;
}

ULng32 LateNameInfoList::getRecompLateNameInfoListLenPre1800()
{
  ULng32 numEntriesToSend = 0;
  for (ULng32 i = 0; i < getNumEntries(); i++)
    {
      if (lateNameInfo_[i]->isVariable())
	numEntriesToSend++;
    }
  
  if (numEntriesToSend > 0)
    return sizeof(RecompLateNameInfoListPre1800) +
      ((numEntriesToSend-1) * sizeof(RecompLateNameInfoPre1800));
  else
    return 0;
}


// puts recomp info into 'buffer'. Space is to be allocated by caller.
void LateNameInfoList::getRecompLateNameInfoListPre1800(char * buffer)
{
  RecompLateNameInfoListPre1800 * rlnil = (RecompLateNameInfoListPre1800 *)buffer;

  Int32 j = 0;
  for (UInt32 i = 0; i < getNumEntries(); i++)
    {
      if (lateNameInfo_[i]->isVariable())
	{
	  str_cpy_and_null(rlnil->getRecompLateNameInfo(j).varName(),
			   lateNameInfo_[i]->variableName(),
			   str_len(lateNameInfo_[i]->variableName()) + 1);
	  
	  str_cpy_and_null(rlnil->getRecompLateNameInfo(j).compileTimeAnsiName(),
			   lateNameInfo_[i]->compileTimeAnsiName(),
			   str_len(lateNameInfo_[i]->compileTimeAnsiName())+1);

	   
	  str_cpy_and_null(rlnil->getRecompLateNameInfo(j).actualAnsiName(),
			     lateNameInfo_[i]->lastUsedExtAnsiName(),
			     str_len(lateNameInfo_[i]->lastUsedExtAnsiName())+1);

#pragma nowarn(1506)   // warning elimination 
	  rlnil->getRecompLateNameInfo(j).setMPalias(lateNameInfo_[i]->isMPalias());
#pragma warn(1506)  // warning elimination 
	  
	  j++;
        } // if
	
    } // for
  
  if (j > 0)
    rlnil->numEntries() = j;
}


void LateNameInfoList::resetRuntimeFlags()
{
  for (UInt32 i = 0; i < getNumEntries(); i++)
    {
      lateNameInfo_[i]->resetRuntimeFlags();
    }
}



SimilarityTableInfo::SimilarityTableInfo()
  : NAVersionedObject(-1)
{
  flags_ = 0;
}

SimilarityTableInfo::~SimilarityTableInfo()
{
}

Long SimilarityTableInfo::pack(void * space)
{
  return NAVersionedObject::pack(space);
}

Lng32 SimilarityTableInfo::unpack(void * base, void * reallocator)
{
  return 0;
}

SimilarityInfo::SimilarityInfo(CollHeap * heap)
  : NAVersionedObject(-1)
{
  tupleDesc_      = 0;
  colNameList_    = 0;
  sti_            = 0;
  runtimeFlags_ = 0;
  compiletimeFlags_ = 0;
  mvAttributesBitmap_ = 0;

}

SimilarityInfo::~SimilarityInfo()
{
}

Long SimilarityInfo::pack(void * space)
{
  return NAVersionedObject::pack(space);
}

Lng32 SimilarityInfo::unpack(void * base, void * reallocator)
{
  // ------------------------------------------------------------------------
  // Notice that objects referenced by keyClass_ and indexMapArray_ have been
  // packed based on a different base (see SimilarityInfo::pack()). Hence,
  // they have to be unpacked correspondingly.
  // ------------------------------------------------------------------------
  if(tupleDesc_.unpack(base, reallocator)) return -1;
  if(colNameList_.unpack(base, reallocator)) return -1;
  if(sti_.unpack(base, reallocator)) return -1;
  return NAVersionedObject::unpack(base, reallocator);
}

QuerySimilarityInfo::QuerySimilarityInfo()
  : heap_(NULL), option_(RECOMP_ON_TS_MISMATCH), NAVersionedObject(-1), siList_(NULL),
    indexInfoList_(NULL)
{
}

QuerySimilarityInfo::QuerySimilarityInfo(CollHeap * heap)
  : heap_(heap), option_(RECOMP_ON_TS_MISMATCH), NAVersionedObject(-1)
{
  siList_ = new(heap) Queue(heap);
  indexInfoList_ = 0;
}

QuerySimilarityInfo::~QuerySimilarityInfo()
{

}

Long QuerySimilarityInfo::pack(void * space)
{
  PackQueueOfNAVersionedObjects(siList_,space,SimilarityInfo);

  //PackQueueOfNonNAVersionedObjects(indexInfoList_,space,IndexInfo);
  PackQueueOfNAVersionedObjects(indexInfoList_,space,IndexInfo);

  return NAVersionedObject::pack(space);
}

Lng32 QuerySimilarityInfo::unpack(void * base, void * reallocator)
{
  UnpackQueueOfNAVersionedObjects(siList_,base,SimilarityInfo,reallocator);
  
  //UnpackQueueOfNonNAVersionedObjects(indexInfoList_,base,IndexInfo);
  UnpackQueueOfNAVersionedObjects(indexInfoList_,base,IndexInfo,reallocator);

  return NAVersionedObject::unpack(base, reallocator);
}

Long IndexInfo::pack(void * space)
{
  return NAVersionedObject::pack(space);
}

Lng32 IndexInfo::unpack(void * base, void * reallocator)
{
  return NAVersionedObject::unpack(base, reallocator);
}

AnsiOrNskName::AnsiOrNskName(char *inName)
{
  Int32 len;

  extName_[0] = '\0';
  intName_[0] = '\0';
  isNskName_ = FALSE;
  noOfParts_ = 0;
  len = str_len(inName);
  if (len < sizeof(extName_))
  {
    str_cpy_all(extName_, inName, len);
    extName_[len] ='\0';
    isError_ = FALSE;
  }
  else
  {
    str_cpy_all(extName_, inName, sizeof(extName_)-1);
    extName_[sizeof(extName_)-1] ='\0';
    isError_ = TRUE;
  }
  isValid_ = FALSE;
}

Int16 AnsiOrNskName::convertAnsiOrNskName(bool doCheck)
{

  // If doCheck is false, this function will not uppercase the name and will not check for
  // reserved word. 
  // The "doCheck" is set to FALSE at places wherein executor could
  // encounter such names. However, these places will have ansi name set to proper case.
  if (isError_)
      return -1;
  if (isValid_)
      return 0;
  if (extName_[0] == '\0')
  {
      isValid_ = TRUE;
      noOfParts_ = 0;
      return 0;
  }

  isError_ = TRUE;
  char *ptr, *tgtPtr, *partPtr;
  short partLen = 0;
  short totalLen = 0;
  bool inQuotes = FALSE;
  bool delimited = FALSE;
  bool trailSpace = FALSE;
  bool dollarFound = FALSE;
  bool partBegin = TRUE;
  char c;
  short partCheckLen;
	
  ptr = extName_;
  tgtPtr = intName_;
  noOfParts_ = 0;
  partPtr = parts_[noOfParts_];
  
  isNskName_ = FALSE;
  // Remove leading spaces
  while (*ptr != '\0' && *ptr == ' ')
    *ptr++;  

  if (ptr != '\0') 
  {
    if (*(ptr) != '\"') // Check that the first char is Alpha if not in quotes
    {
      if (! isAlpha8859_1((unsigned char)(*ptr)))
      {
	if (*(ptr) != '\\' && *(ptr) != '$')
	  return -1;
	else
	{
	  isNskName_ = TRUE;
	  if (*(ptr) == '$')
	     dollarFound = TRUE;
	}
      }
    }
  }
  else
    return -1; // Error when all are spaces
  if (isNskName_)
     partCheckLen = 8;	// NskNameLen
  else
     partCheckLen = ComAnsiNamePart::MAX_IDENTIFIER_INT_LEN;
  while(*ptr != '\0')
  {
    switch (*ptr)
    {
    case '\"':
      if (trailSpace)
	return -1;
      if (! inQuotes)
      {
	if (partBegin)
	{
	  ptr++;	
	  inQuotes = TRUE;
	  delimited = TRUE;
	  partBegin = FALSE;
	}
	else
	  return -1;
      }
      else
      {
	// Check if next is quotes
	if (*(ptr+1) != '\0')
	{
	  if (*(ptr+1) == '\"')
	  {
    	    if (partLen < partCheckLen)
	    {
	      *tgtPtr = *ptr;
	      *partPtr = *ptr;
	      tgtPtr++;
 	      partPtr++;	
	      ptr++;
	      ptr++; // Skip one Quote
	      partLen++;
	      totalLen++;
	    }
	    else
	      return -1;
	  }
	  else
	  {
	    if (*(ptr+1) != '.')
	      return -1;
	    inQuotes = FALSE; // Skip the ending quote
	    ptr++;
	  }
	}
	else
	{
	  inQuotes = FALSE;
	  ptr++;
	}
      }
      break;
    case '.':
      if (trailSpace)
	return -1;
      if (! inQuotes)
      {
	if (totalLen < ComAnsiNamePart::MAX_ANSI_NAME_INT_LEN)
	{
	   *tgtPtr = *ptr;
	   tgtPtr++;
	   ptr++;
	   totalLen++;
	}
	if (isNskName_)
	{
	  if (noOfParts_ >= 3)  
  	    return -1;
	}
	else
	{
	  if (noOfParts_ >= 2)
	    return -1;
	}
	partBegin = TRUE;
	partLen = 0;
	*partPtr = '\0';
	if (!delimited)
	{
	  // Check if it is a SQL reserved word
	  if (doCheck && IsSqlReservedWord(parts_[noOfParts_]))
	    return -1;
	}
	delimited = FALSE;
	noOfParts_++;
        partPtr = parts_[noOfParts_];
	if (isNskName_)
	{
	  if (noOfParts_ == 1 && *ptr != '\0')
	  {
	     if (! dollarFound)
	     {
	       if (*ptr != '$')
		 return -1;
	     }
	     else
	     {
	       if (*ptr != '\"')
	       {
		 if (! isAlpha8859_1((unsigned char)(*ptr)))
		  return -1;
	       } 
	     }
	  }
	  else
	  {
	    if (*ptr != '\0' && *ptr != '\"')
	    {
	      if (! isAlpha8859_1((unsigned char)(*ptr)))
		return -1;
	    }
  	  }
	}
	else
	{
	  if (*ptr != '\0' && *ptr != '\"') // Check that char is Alpha if not in quotes
	  {
	      if (! isAlpha8859_1((unsigned char)(*ptr)))
		return -1;
	  }
	}
      }
      else
      {
        if (partLen < partCheckLen)
        {
	  *tgtPtr = *ptr;
	  *partPtr = *ptr;
	  tgtPtr++;
	  partPtr++;
	  ptr++;
	  totalLen++;
	  partLen++;
	}
        else
  	  return -1;
      }
      break;
    case  '\t':
      if (! inQuotes)
	return -1;
      partBegin = FALSE;
      if (partLen < partCheckLen)
      {
	*tgtPtr = ' ';
	*partPtr = ' ';
	tgtPtr++;
	partPtr++;
	ptr++;
	partLen++;
	totalLen++;
      }
      else
	return -1;
      break;
    case '@': 
    case '#':
      partBegin = FALSE;
      if (partLen < partCheckLen)
      {
	*tgtPtr = *ptr;
	*partPtr = *ptr;
	tgtPtr++;
	partPtr++;
	ptr++;
	partLen++;
	totalLen++;
      }
      else
	return -1;
      break;
    case ' ':
      partBegin = FALSE;
      if (! inQuotes)
      {
	trailSpace = TRUE;
	ptr++;
      }
      else
      {
	if (partLen < partCheckLen)
	{
	  *tgtPtr = *ptr;
	  *partPtr = *ptr;
	  tgtPtr++;
	  partPtr++;
	  ptr++;
	  partLen++;
	  totalLen++;
	}
        else
  	  return -1;
      }
      break;
    default:
       // Copy the character
      if (trailSpace)
	return -1;
      partBegin = FALSE;
      if (inQuotes)
	c= *ptr;
      else
      {
	if (partLen != 0) // Don't check the first character, it is already checked
	{
	  if (NOT isAlNum8859_1((unsigned char)(*ptr)) && (*ptr != '_'))
	     return -1;
	}
	if (doCheck)
	{
	  c = (char)TOUPPER(*ptr);
	}
	else
	  c = *ptr;
      }
      if (partLen < partCheckLen)
      {
	*tgtPtr = c;
        *partPtr = c;
	tgtPtr++;
	partPtr++;
	ptr++;
	partLen++;
	totalLen++;
      }
      else
	return -1;
      break;
    }
  } 

  if (inQuotes)
    return -1;
  *tgtPtr = '\0';
  *partPtr = '\0';
  if (!delimited)
  {
    // Check if it is a SQL reserved word
    if (doCheck && IsSqlReservedWord(parts_[noOfParts_]))
      return -1;
  }
  noOfParts_++;
  isValid_ = TRUE;
  isError_ = FALSE;
  return 0;
}

Int16 AnsiOrNskName::extractParts(Lng32 &numParts,
		char *parts[])
{
  Int16 ret;
  Int16 i;

  if (! isValid_)
  {
    if ((ret = convertAnsiOrNskName(FALSE)) == -1)
      return ret;
  }
  numParts = noOfParts_;
  for (i = 0 ; i < noOfParts_ ; i++)
    parts[i] = parts_[i];
  return 0;
}

Int16 AnsiOrNskName::equals(AnsiOrNskName *inName)
{
  // Check if the external name are equal 
  if (str_cmp_ne(inName->extName_, extName_) == 0)
    return 1;
  else
  {
    if (! isValid_)
    {
      if (convertAnsiOrNskName(FALSE) != 0)
	return -1;
    }
    if (! inName->isValid_)
    {
      if (inName->convertAnsiOrNskName() != 0)
	  return -1;
    }
    if (str_cmp_ne(inName->intName_, intName_) == 0)
	return 1;
    else
	return 0;
  }
}

char *AnsiOrNskName::getInternalName()
{
  if (! isValid_)
  {
    if (convertAnsiOrNskName() != 0)
      return NULL;
  }
  return intName_;
}

char *AnsiOrNskName::getExternalName()
{
  if (isValid_ && !isNskName_)
    // Build the ANSI name from the individual parts
    ComBuildANSIName (parts_[0], parts_[1], parts_[2], extName_, sizeof(extName_));
  return extName_;
}

AnsiOrNskName *LateNameInfo::getLastUsedName(NAMemory *heap)
{
  AnsiOrNskName *ansiName;
  NABasicPtr    nameStr;
  void *  	addr;

  if (! isLastUsedNameEmbedded())
  {
    addr = *((void **)lastUsedAnsiName_);
    return (AnsiOrNskName *)addr;
  }
  else
  {
    if (heap == NULL)
      return NULL;
    if (isLastUsedNameCompEmbedded())
      ansiName = new (heap) AnsiOrNskName(lastUsedAnsiName_);
    else
    {
      memcpy((void *)&nameStr, lastUsedAnsiName_, sizeof(nameStr));
      ansiName = new (heap) AnsiOrNskName(nameStr.getPointer());
    }
    *(void **)lastUsedAnsiName_ = (void *)ansiName;
    setLastUsedNameMode(TRUE);
    return ansiName;
  }
}

Int16 AnsiOrNskName::fillInMissingParts(char *schemaName)
{
    char *ptr;
    char *tgt;
    char tempName[sizeof(extName_)+16]; // plus a few extra bytes to avoid boundary condition
    Int16 partsToFill;
    Int16 partsFilled = 0;
    short totalLen = 0;
    bool  inQuotes = FALSE;
    Int16 retCode;

    if (! isValid_)
    {
      if (convertAnsiOrNskName() != 0)
	return -1;
    }
    partsToFill = (Int16)(3 - noOfParts_);
    if (isNskName_ || partsToFill == 0)
      return 1;
    // Remove leading blanks in the schemaName
    ptr = schemaName;
    tgt = tempName;
    *tgt = '\0';
    while (*ptr != '\0' && *ptr == ' ')
      *ptr++;  
    while(*ptr != '\0' && partsFilled != partsToFill)
    {
      switch (*ptr)
      {
	case '\"':
	  if (totalLen < sizeof(extName_))
	  {
	    *tgt = *ptr;
	    ptr++;
	    tgt++;
	    totalLen++;
	  }
	  else
	    return -1;
	  if (!inQuotes)
	    inQuotes = TRUE;
	  else
	    inQuotes = FALSE;
	  break;
	case '.':
	  if (totalLen < sizeof(extName_))
	  {
	    *tgt = *ptr;
	    ptr++;
	    tgt++;
	    totalLen++;
	  }
	  else
	    return -1;
	  if (! inQuotes)
	    partsFilled++;
	  break;
	default:
	  if (totalLen < sizeof(extName_))
	  {
	    *tgt = *ptr;
	    ptr++;
	    tgt++;
	    totalLen++;
	  }
	  else
	    return -1;
	  break;
      }
    }
    // Copy '.'
    if (*(tgt-1) != '.' && totalLen < sizeof(extName_))
    {
      *tgt = '.';
      tgt++;
      totalLen++;
    }
    //Append the extName
    ptr = extName_;
    while (*ptr != '\0')
    {
      if (totalLen < sizeof(extName_))
      {
	*tgt = *ptr;
	ptr++;
	tgt++;
	totalLen++;
      }
      else
	return -1;
    }
    *tgt ='\0';
    str_cpy_all(extName_, tempName,totalLen);
    extName_[totalLen] ='\0';
    isValid_ = FALSE;
    isError_ = FALSE;
    retCode = convertAnsiOrNskName();
    if (retCode != -1 && noOfParts_ != 3)
      retCode = -1;
    return retCode;
}

// This function is used only at the  compile time
void LateNameInfo::setLastUsedName(char *name, NAMemory *heap)
{
  NABasicPtr ansiName;
  Int32 len;
  char *ptr;

  len = str_len(name);
  if (len > ComAnsiNamePart::MAX_OLD_ANSI_IDENTIFIER_LEN)
  {
    ptr = new (heap) char[len+1];
    str_cpy_all(ptr, name, len);
    ptr[len]='\0';
    ansiName = ptr;
    memcpy(lastUsedAnsiName_, (const void *)&ansiName, sizeof(ansiName));
    setLastUsedNameCompMode(TRUE);
  }
  else
  {
     str_cpy_all(lastUsedAnsiName_, name, len);
     lastUsedAnsiName_[len] ='\0';
     setLastUsedNameCompMode(FALSE);
  }

}

void LateNameInfo::setLastUsedName(AnsiOrNskName *name)
{
  AnsiOrNskName *ansiName;

  if (! isLastUsedNameEmbedded())
  {
    ansiName = getLastUsedName(NULL);
    delete ansiName;
  }
  *(void **)lastUsedAnsiName_ = (void *)name; 
  setLastUsedNameMode(TRUE);
}

void LateNameInfo::setCompileTimeName(char *name, NAMemory *heap)
{
  NABasicPtr ansiName;
  Int32 len;
  char *ptr;

  len = str_len(name);
  if (len > ComAnsiNamePart::MAX_OLD_ANSI_IDENTIFIER_LEN)
  {
    ptr = new (heap) char[len+1];
    str_cpy_all(ptr, name, len);
    ptr[len]='\0';
    ansiName = ptr;
    memcpy(compileTimeAnsiName_, (const void *)&ansiName, sizeof(ansiName));
    setCompileNameCompMode(TRUE);
  }
  else
  {
     str_cpy_all(compileTimeAnsiName_, name, len);
     compileTimeAnsiName_[len] = '\0';
     setCompileNameCompMode(FALSE);
  }

}

char *LateNameInfo::lastUsedAnsiName()
{
  AnsiOrNskName *name;
  NABasicPtr	nameStr;
  
  name = getLastUsedName(NULL);
  if (name != NULL)
    return name->getInternalName();
  else
  {
    if (isLastUsedNameCompEmbedded())
      return lastUsedAnsiName_; 
    else
    {
      memcpy((void *)&nameStr, lastUsedAnsiName_, sizeof(nameStr));
      return (nameStr.getPointer());
    }
  }
}

char *LateNameInfo::lastUsedExtAnsiName()
{
  AnsiOrNskName *name;
  NABasicPtr     nameStr;

  name = getLastUsedName(NULL);
  if (name != NULL)
    return name->getExternalName();
  else
  {
    if (isLastUsedNameCompEmbedded())
      return lastUsedAnsiName_; 
    else
    {
      memcpy((void *)&nameStr, lastUsedAnsiName_, sizeof(nameStr));
      return (nameStr.getPointer());
    }
  }
}

void LateNameInfo::zeroLastUsedAnsiName()
{ 
  AnsiOrNskName *ansiName;

  if (! isLastUsedNameEmbedded())
  {
    ansiName = getLastUsedName(NULL);
    delete ansiName;
  }
  setLastUsedNameMode(FALSE);
  setLastUsedNameCompMode(FALSE);
  memset(lastUsedAnsiName_, 0, sizeof(lastUsedAnsiName_)); 
}

char *LateNameInfo::compileTimeAnsiName()
{
  NABasicPtr nameStr;

  if (isCompileNameCompEmbedded())
    return compileTimeAnsiName_; 
  else
  {
    memcpy((void *)&nameStr, compileTimeAnsiName_, sizeof(nameStr));
    return (nameStr.getPointer());
  }
}

Long LateNameInfo::pack(void *space)
{
  NABasicPtr name;

  if (! isCompileNameCompEmbedded())
  {
    memcpy((void *)&name, compileTimeAnsiName_, sizeof(name));
    name.pack(space);
    memcpy((void *)compileTimeAnsiName_, (void *)&name, sizeof(name));
  }
  if (! isLastUsedNameCompEmbedded())
  {
    memcpy((void *)&name, lastUsedAnsiName_, sizeof(name));
    name.pack(space);
    memcpy((void *)lastUsedAnsiName_, (void *)&name, sizeof(name));
  }
  return NAVersionedObject::pack(space);
}

Lng32 LateNameInfo::unpack(void * base, void * reallocator)
{
  NABasicPtr name;
  
  if (! isCompileNameCompEmbedded())
  {
    memcpy((void *)&name, compileTimeAnsiName_, sizeof(name));
    name.unpack(base);
    memcpy(compileTimeAnsiName_, (void *)&name, sizeof(name));
  }
  if (! isLastUsedNameCompEmbedded())
  {
    memcpy((void *)&name, lastUsedAnsiName_, sizeof(name));
    name.unpack(base);
    memcpy(lastUsedAnsiName_, (void *)&name, sizeof(name));
  }
  return NAVersionedObject::unpack(base, reallocator);;
    
}

void LateNameInfo::resetRuntimeFlags()
{ 
  if (! isLastUsedNameEmbedded())
        runtimeFlags_ = LASTUSED_NAME_CLASS_PTR;
  else
  if (! isLastUsedNameCompEmbedded())
      runtimeFlags_ = LASTUSED_NAME_STR_PTR ;
  else
      runtimeFlags_ = 0; 
}

bool AnsiOrNskName::isNskName()
{
  if (! isValid_)
  {
    if (convertAnsiOrNskName() != 0)
      return FALSE;
  }
  return isNskName_;
}

Int16 AnsiOrNskName::updateNSKInternalName(char *inName)
{
  if (isError_ || (! isValid_))
    return -1;
  if (noOfParts_ == 4)
    return -1;
  char *ptr, *tgt, *partPtr;
  Int32 len;
  Int16 partsToFill, partsFilled;
  

  char temp[ComAnsiNamePart::MAX_ANSI_NAME_EXT_LEN+1];

  len = str_len(extName_);
  str_cpy_all(temp, extName_, len);
  temp[len] = '\0';

  partsToFill = (Int16)(4 - noOfParts_);
  partsFilled = 0;
  // Move down the parts
  Int32 i, j;
  for (i = 3, j = noOfParts_-1 ; j >= 0 ; i--, j--)
     memcpy(parts_[i], parts_[j], ComAnsiNamePart::MAX_IDENTIFIER_INT_LEN+1);
  ptr = inName;
  partPtr = parts_[0];
  tgt = extName_;
  // Copy the system name and volume name if needed
  while (*ptr != '\0')
  {
    *tgt = *ptr;
    tgt++;
    if (*ptr == '.')
    {
      *partPtr = '\0';
      partsFilled++;
      partPtr = parts_[partsFilled];
      if (partsFilled == partsToFill || partsFilled == 2)
      {
        ptr++;
        break;
      }
    }
    else
    {
      *partPtr = *ptr;
      partPtr++;
    }
    ptr++;
  }
  *tgt = '\0';
  // Copy the subvolume and quote if reserved name in extName
  if (partsFilled < partsToFill)
  {
    while (*ptr != '\0' && *ptr != '.')
    {
      *partPtr = *ptr;
      ptr++;
      partPtr++;
    }
    // zero-terminate
    *partPtr = '\0';

    if (*ptr == '\0')
      return -1;
    if (IsSqlReservedWord(parts_[2]))
    {
      *tgt = '\"';
      tgt++;
      *tgt = '\0';
      str_cat(extName_, parts_[2], extName_);
      str_cat(extName_, "\".", extName_);
    }
    else
    {
      str_cat(extName_, parts_[2], extName_);
      str_cat(extName_, ".", extName_);
    }
    partsFilled++;
  }
  str_cat(extName_, temp, extName_);
  // copy the internal name
  len = str_len(inName);
  str_cpy_all(intName_, inName, len);
  intName_[len] = '\0';
  noOfParts_ = 4;
  isNskName_ = TRUE;
  return 0;
}

Int16 AnsiOrNskName::quoteNSKExtName()
{
  Int32 len;

  if (isError_ || (! isValid_))
    return -1;
  if (noOfParts_ != 4)
    return -1;
  if (IsSqlReservedWord(parts_[3]))
  {
    // copy just the table name with quotes
    extName_[0] = '\"';
    len = str_len(parts_[3]);
    str_cpy_all(extName_+1, parts_[3], len);
    extName_[len+1] =  '\"';
    extName_[len+2] = '\0';
    noOfParts_ = 1;
    return updateNSKInternalName(intName_);
  }
  else
  if (IsSqlReservedWord(parts_[2]))
  {
    // copy just the table name
    len = str_len(parts_[3]);
    str_cpy_all(extName_, parts_[3], len);
    extName_[len] = '\0';
    noOfParts_ = 1;
    return updateNSKInternalName(intName_);
  }
  else
    return 0;
}

void ResolvedNameListPre1800::translateFromNewVersion(ResolvedNameList *newrnl)
{
  this->numEntries()=  newrnl->numEntries();
  this->resetFlags();
  for (Int32 i = 0;  i < (Int32) (newrnl->numEntries());i++)
    {
      strcpy(this->getResolvedName(i).resolvedGuardianName(),
             newrnl->getResolvedName(i).resolvedGuardianName());
    
      this->getResolvedName(i).setIgnoreTS((short)newrnl->getResolvedName(i).ignoreTS());
      this->getResolvedName(i).setResolvedAnsiName(newrnl->getResolvedName(i).resolvedAnsiName());
      this->getResolvedName(i).setValidateNumIndexes((short)newrnl->getResolvedName(i).validateNumIndexes());
      this->getResolvedName(i).setNumIndexes(newrnl->getResolvedName(i).numIndexes());
    }
}

void ResolvedNameList::translateFromOldVersion(ResolvedNameListPre1800 *oldrnl)
{
  this->numEntries() = oldrnl->numEntries();
  this->resetFlags();
  this->resetFiller();
  for (Int32 i = 0;  i < (Int32) (oldrnl->numEntries());i++)
    {
      strcpy(this->getResolvedName(i).resolvedGuardianName(),
	     oldrnl->getResolvedName(i).resolvedGuardianName());
    
      this->getResolvedName(i).setIgnoreTS((short)oldrnl->getResolvedName(i).ignoreTS());
      this->getResolvedName(i).setResolvedAnsiName(oldrnl->getResolvedName(i).resolvedAnsiName());
      this->getResolvedName(i).setValidateNumIndexes((short)oldrnl->getResolvedName(i).validateNumIndexes());
      this->getResolvedName(i).setNumIndexes(oldrnl->getResolvedName(i).numIndexes());
						      }
    }
// End

