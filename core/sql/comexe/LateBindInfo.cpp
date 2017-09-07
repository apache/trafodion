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
#include "ComResWords.h"
#include "ComDistribution.h"

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

AnsiName::AnsiName(char *inName)
{
  Int32 len;

  extName_[0] = '\0';
  intName_[0] = '\0';
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

Int16 AnsiName::convertAnsiName(bool doCheck)
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
	  if (*(ptr) == '$')
	     dollarFound = TRUE;
	}
      }
    }
  }
  else
    return -1; // Error when all are spaces
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

        if (noOfParts_ >= 2)
          return -1;

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
        if (*ptr != '\0' && *ptr != '\"') // Check that char is Alpha if not in quotes
	  {
            if (! isAlpha8859_1((unsigned char)(*ptr)))
              return -1;
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

Int16 AnsiName::extractParts(Lng32 &numParts,
		char *parts[])
{
  Int16 ret;
  Int16 i;

  if (! isValid_)
  {
    if ((ret = convertAnsiName(FALSE)) == -1)
      return ret;
  }
  numParts = noOfParts_;
  for (i = 0 ; i < noOfParts_ ; i++)
    parts[i] = parts_[i];
  return 0;
}

Int16 AnsiName::equals(AnsiName *inName)
{
  // Check if the external name are equal 
  if (str_cmp_ne(inName->extName_, extName_) == 0)
    return 1;
  else
  {
    if (! isValid_)
    {
      if (convertAnsiName(FALSE) != 0)
	return -1;
    }
    if (! inName->isValid_)
    {
      if (inName->convertAnsiName() != 0)
	  return -1;
    }
    if (str_cmp_ne(inName->intName_, intName_) == 0)
	return 1;
    else
	return 0;
  }
}

char *AnsiName::getInternalName()
{
  if (! isValid_)
  {
    if (convertAnsiName() != 0)
      return NULL;
  }
  return intName_;
}

char *AnsiName::getExternalName()
{
  if (isValid_)
    // Build the ANSI name from the individual parts
    ComBuildANSIName (parts_[0], parts_[1], parts_[2], extName_, sizeof(extName_));
  return extName_;
}

AnsiName *LateNameInfo::getLastUsedName(NAMemory *heap)
{
  AnsiName *ansiName;
  NABasicPtr    nameStr;
  void *  	addr;

  if (! isLastUsedNameEmbedded())
  {
    addr = *((void **)lastUsedAnsiName_);
    return (AnsiName *)addr;
  }
  else
  {
    if (heap == NULL)
      return NULL;
    if (isLastUsedNameCompEmbedded())
      ansiName = new (heap) AnsiName(lastUsedAnsiName_);
    else
    {
      memcpy((void *)&nameStr, lastUsedAnsiName_, sizeof(nameStr));
      ansiName = new (heap) AnsiName(nameStr.getPointer());
    }
    *(void **)lastUsedAnsiName_ = (void *)ansiName;
    setLastUsedNameMode(TRUE);
    return ansiName;
  }
}

Int16 AnsiName::fillInMissingParts(char *schemaName)
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
      if (convertAnsiName() != 0)
	return -1;
    }
    partsToFill = (Int16)(3 - noOfParts_);
    if (partsToFill == 0)
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
    retCode = convertAnsiName();
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

void LateNameInfo::setLastUsedName(AnsiName *name)
{
  AnsiName *ansiName;

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
  AnsiName *name;
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
  AnsiName *name;
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
  AnsiName *ansiName;

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

///////////////////////////////////////////////////////////////////
// class TrafSimilarityTableInfo
///////////////////////////////////////////////////////////////////
TrafSimilarityTableInfo::TrafSimilarityTableInfo(char * tableName,
                                                 NABoolean isHive,
                                                 char * hdfsRootDir, 
                                                 Int64 modTS, Int32 numPartnLevels,
                                                 Queue * hdfsDirsToCheck,
                                                 char * hdfsHostName,
                                                 Int32 hdfsPort)
     : NAVersionedObject(-1),
       tableName_(tableName),
       hdfsRootDir_(hdfsRootDir),
       modTS_(modTS), numPartnLevels_(numPartnLevels),
       hdfsDirsToCheck_(hdfsDirsToCheck),
       hdfsHostName_(hdfsHostName), hdfsPort_(hdfsPort),
       flags_(0)
{
  if (isHive)
    setIsHive(TRUE);
}

TrafSimilarityTableInfo::TrafSimilarityTableInfo()
     : NAVersionedObject(-1),
       tableName_(NULL),
       hdfsRootDir_(NULL),
       modTS_(-1), numPartnLevels_(-1),
       hdfsDirsToCheck_(NULL),
       hdfsHostName_(NULL), hdfsPort_(NULL),
       flags_(0)
{
}

TrafSimilarityTableInfo::~TrafSimilarityTableInfo()
{
}

NABoolean TrafSimilarityTableInfo::operator==(TrafSimilarityTableInfo &o)
{ 
  return ((isHive() == o.isHive()) &&
          (strcmp(tableName(), ((TrafSimilarityTableInfo&)o).tableName()) == 0));
}

Long TrafSimilarityTableInfo::pack(void * space)
{
  tableName_.pack(space);
  hdfsRootDir_.pack(space);
  hdfsHostName_.pack(space);

  hdfsDirsToCheck_.pack(space);

  return NAVersionedObject::pack(space);
}

Lng32 TrafSimilarityTableInfo::unpack(void * base, void * reallocator)
{
  if(tableName_.unpack(base)) return -1;
  if(hdfsRootDir_.unpack(base)) return -1;
  if(hdfsHostName_.unpack(base)) return -1;

  if(hdfsDirsToCheck_.unpack(base, reallocator)) return -1;
  
  return NAVersionedObject::unpack(base, reallocator);
}

///////////////////////////////////////////////////////////////////
// class TrafQuerySimilarityInfo
///////////////////////////////////////////////////////////////////
TrafQuerySimilarityInfo::TrafQuerySimilarityInfo(Queue * siList)
     : NAVersionedObject(-1),
       siList_(siList)
{
}

TrafQuerySimilarityInfo::TrafQuerySimilarityInfo()
     : NAVersionedObject(-1),
       siList_(NULL)
{
}

TrafQuerySimilarityInfo::~TrafQuerySimilarityInfo()
{
}

Long TrafQuerySimilarityInfo::pack(void * space)
{
  PackQueueOfNAVersionedObjects(siList_,space,TrafSimilarityTableInfo);

  return NAVersionedObject::pack(space);
}

Lng32 TrafQuerySimilarityInfo::unpack(void * base, void * reallocator)
{
  UnpackQueueOfNAVersionedObjects(siList_,base,TrafSimilarityTableInfo,reallocator);
  
  return NAVersionedObject::unpack(base, reallocator);
}


// End

