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
 * File:         DefaultValidator.cpp
 * Description:  Validation of Defaults values.
 *
 *
 *****************************************************************************
 */

#define  SQLPARSERGLOBALS_NADEFAULTS
#define  SQLPARSERGLOBALS_FLAGS
#include "Platform.h"

#include <ctype.h>
#include <stdio.h>
#include "charinfo.h"
#include "CmpCommon.h"
#include "ComMPLoc.h"
#include "DefaultValidator.h"
#include "ObjectNames.h"
#include "SchemaDB.h"
#include "ComLocationNames.h"  // for ComIsGuardianVolumeNamePartValid()
#include "ComSqlText.h" 

#include "ComSchemaName.h" // for ComSchemaName
#include "BindWA.h"        // for extractOverrideSchemas()

#include "SqlParserGlobals.h"			// must be last


#define ERRWARN(msg)    ToErrorOrWarning(msg, errOrWarn)


void DefaultValidator::applyUpper(NAString &value) const
{
  switch (caseSensitive()) {
    case CASE_SENSITIVE:	return;
    case CASE_INSENSITIVE:	value.toUpper(); return;
    case CASE_SENSITIVE_ANSI:	{
				  // Make copy of value so we can own its data()
				  // for casting away const-ness.
				  NAString tmp(value);
				  char *s = (char *)tmp.data();
				  for (NABoolean quoted = FALSE; *s; s++) {
				    if (*s == '"')
				      quoted = !quoted;
				    else if (!quoted)
#pragma nowarn(1506)   // warning elimination 
				      *s = toupper(*s);
#pragma warn(1506)  // warning elimination 
				  }
				  value = tmp;
				  return;
    				}
    default:			CMPASSERT(FALSE);
  }
}

NAString DefaultValidator::getTypeText(DefaultValidatorType vt)
{
  switch (vt) {
    case VALID_ANY:	return "ANY";
    case VALID_INT:	return "INTEGER";
    case VALID_UINT:	return "INTEGER UNSIGNED";
    case VALID_FLT:	return "FLOAT";
    case VALID_KWD:	return "DEFAULTS-KEYWORD";
    case VALID_NSKVOL:		return "NSKVOL";
    case VALID_NSKSUBVOL:	return "NSKSUBVOL";
    case VALID_NSKSYSTEM:	return "NSKSYSTEM";
    case VALID_NSKMPLOC:        return "NSKMPLOC";
    case VALID_NSKDISKS:        return "NSKDISKS";
    case VALID_NTDISKS:         return "NTDISKS";

    default:		return "???dv???";
  }
}

Int32 DefaultValidator::validate(	  const char *,
				  const NADefaults *,
				  Int32,
				  Int32,
				  float *) const
{
  return TRUE;					// anything is valid; no error
}

Int32 ValidateKeyword::validate(	  const char *value,
				  const NADefaults *nad,
				  Int32 attrEnum,
				  Int32 errOrWarn,
				  float *) const
{
  NAString tmp(value);
  DefaultToken tok = nad->token(attrEnum, tmp, TRUE, errOrWarn);
  return (tok != DF_noSuchToken);
}

Int32 ValidateNSKNamePart::validate(const char *value,
				  const NADefaults *nad,
				  Int32 attrEnum,
				  Int32 errOrWarn,
				  float *) const
{
  NABoolean ok = TRUE;
  size_t len = strlen(value);
  size_t i = initialPunc_ ? 1 : 0;
  if (len == 0) return blankOK_;
  if (value[0] == '"' && value[len-1] == '"') {// ignore begin and end "
// LCOV_EXCL_START :rfi
    len -= 2;
    i++;
// LCOV_EXCL_STOP
  }

  if (len > 8) ok = FALSE;
  else if (initialPunc_ && *value != initialPunc_) ok = FALSE;
  else {
    if (!isalpha(value[i])) ok = FALSE;
    else {
      for (i++; i < len; i++) {
        if (!isalnum(value[i])) {
	  ok = FALSE;
	  break;
	}
      }
    }
  }
  if (!ok && errOrWarn)
    *CmpCommon::diags() << DgSqlCode(ERRWARN(2055))
	<< DgString0(value)
	<< DgString1(nad->lookupAttrName(attrEnum, errOrWarn));
  return ok;
}

Int32 ValidateNSKMPLoc::validate(	  const char *value,
				  const NADefaults *nad,
				  Int32 attrEnum,
				  Int32 errOrWarn,
				  float *) const
{
  ComMPLoc loc(value, ComMPLoc::SUBVOL);
  NABoolean ok = loc.isValid(ComMPLoc::SUBVOL);

  // For MP_CATALOG, cannot safely apply MP_VOLUME default if no vol specified.
  if (!loc.hasVolumeName()) ok = FALSE;

  if (!ok && errOrWarn)
    *CmpCommon::diags() << DgSqlCode(ERRWARN(2055))
	<< DgString0(value)
	<< DgString1(nad->lookupAttrName(attrEnum, errOrWarn));
  return ok;
}

Int32 ValidateDiskListNSK::validate( const char *value,
				   const NADefaults *nad,
				   Int32 attrEnum,
				   Int32 errOrWarn,
				   float *alreadyHaveFloat) const
{
  const char *curr = value;
  const char *dummyCName;
  Lng32 dummyCNameLen;
  const char *dummyDName;
  Lng32 dummyDNameLen;

  while (*curr)
    {
      if (getNextDiskAndAdvance(curr,dummyCName,dummyCNameLen,
				dummyDName, dummyDNameLen))
	{
	  *CmpCommon::diags()
	    << DgSqlCode(ERRWARN(2055))
	    << DgString0(value)
	    << DgString1(nad->lookupAttrName(attrEnum, errOrWarn));
	  return FALSE;
	}
    }
  return TRUE;
}

Int32 ValidateDiskListNSK::getNextDiskAndAdvance(const char *&defaultString,
					       const char *&clusterName,
					       Lng32 &clusterNameLen,
					       const char *&diskName,
					       Lng32 &diskNameLen)
{
  // We want to see a string "\node.$disk" or "$disk", optionally
  // followed by a separator ";" or ",". We assume that defaultString
  // is null-terminated.

  //## Should be rewritten to:
  //## a) tokenize by blanks and separators, and
  //## b) use "ComMPLoc x(token, ComMPLoc::VOL)" to parse each token.
  //##
  //## That is, should be almost the same way that
  //## ValidateCollationList::validate() below tokenizes + parses.

  NABoolean separatorSeen;
  short nameLen;

  clusterNameLen = 0;
  diskNameLen = 0;

  // skip white space (spaces may get replaced by underscores)
  while (*defaultString == ' '  ||
	 *defaultString == '\t' ||
	 *defaultString == '_')
    defaultString++;

  // this is not exactly the right define, but we want to make sure
  // that the environment is or simulates NSK

  // should not reach here, no NSK methods available, just advance to the
  // next separator
  for (nameLen = 0;
       defaultString[nameLen] && defaultString[nameLen] != ';'
	 && defaultString[nameLen] != ',';
       nameLen++)
    ;
   NAString aName(defaultString, nameLen);
   ComLocationName locName(aName, 
                           ComLocationName::GUARDIAN_LOCATION_NAME_FORMAT);

  const char* sysName = locName.getGuardianSystemNamePart().castToConstChar();
  Int32 sysNameLen = locName.getGuardianSystemNamePart().length();

  switch (locName.getInputFormat()) {
// LCOV_EXCL_START :mp
    case ComLocationName::NODE_VOLUME_INPUT_FORMAT:
      if (sysNameLen != 4 || strncmp(sysName, "\\NSK", 4) != 0) 
        return -1;
      break;
// LCOV_EXCL_STOP
   case ComLocationName::VOLUME_INPUT_FORMAT:
     if (NOT locName.isValid()) 
       return -1;
     break;

   default:
     return -1;
  }


  // default is an empty cluster name
  clusterName = defaultString;

  if (*defaultString == '\\')
    {
// LCOV_EXCL_START :mp
      // seen a node name, check its length
      while (*defaultString && *defaultString != '.')
	{
	  defaultString++;
	  clusterNameLen++;
	  nameLen--;
	}
      if (*defaultString != '.')
	return -1; // '.' between \node and $disk is missing

      // skip over the dot
      defaultString++;
      nameLen--;
// LCOV_EXCL_STOP
    }

  if (*defaultString == '$')
    {
      diskName = defaultString;
      defaultString += nameLen;
      diskNameLen = nameLen;
    }
  else
    return -2; // expected $disk

  // skip white space after drive letter and skip separators
  separatorSeen = FALSE;
  while (*defaultString && (*defaultString == ' ' ||
			    *defaultString == '\t' ||
			    *defaultString == ';' ||
			    *defaultString == ','))
    {
      if (*defaultString == ';' || *defaultString == ',')
	separatorSeen = TRUE;
      defaultString++;
    }

  if (*defaultString && !separatorSeen)
    return -3; // disk is followed by something other than a separator

  return 0;
}
		     
Int32 ValidateDiskListNT::validate( const char *value,
				  const NADefaults *nad,
				  Int32 attrEnum,
				  Int32 errOrWarn,
				  float *alreadyHaveFloat) const
{
  const char *curr = value;
  Lng32 dummyNNum;
  char dummyLetter;

  while (*curr)
    {
      if (getNextDriveLetterAndAdvance(curr,dummyNNum,dummyLetter))
	{
	  *CmpCommon::diags()
	    << DgSqlCode(ERRWARN(2055))
	    << DgString0(value)
	    << DgString1(nad->lookupAttrName(attrEnum, errOrWarn));
	  return FALSE;
	}
    }
  return TRUE;
}

// LCOV_EXCL_START :nt
Int32 ValidateDiskListNT::getNextDriveLetterAndAdvance(
     const char *&defaultString,
     Lng32 &nodeNum,
     char &driveLetter)
{
  NABoolean separatorSeen;

  // skip white space
  while (*defaultString == ' ' || *defaultString == '\t')
    defaultString++;

  if (!isdigit(*defaultString))
    return -1; // expected a number

  nodeNum = 0;
  while (isdigit(*defaultString) && nodeNum < 16)
    {
      nodeNum = 10*nodeNum + *defaultString - '0';
      defaultString++;
    }
  
  // skip white space between node and drive letter
  while (*defaultString == ' ' || *defaultString == '\t')
    defaultString++;

  if (*defaultString >= 'A' && *defaultString <= 'Z')
    {
      driveLetter = *defaultString;
      defaultString++;
    }
  else if (*defaultString >= 'a' && *defaultString <= 'z')
    {
#pragma nowarn(1506)   // warning elimination 
      driveLetter = *defaultString - 'a' + 'A';
#pragma warn(1506)  // warning elimination 
      defaultString++;
    }
  else
    return -2; // drive letter was missing

  // skip white space after drive letter and skip separators
  separatorSeen = FALSE;
  while (*defaultString == ' '  ||
	 *defaultString == '\t' ||
	 *defaultString == ';'  ||
	 *defaultString == ',')
    {
      if (*defaultString == ';' || *defaultString == ',')
	separatorSeen = TRUE;
      defaultString++;
    }

  if (*defaultString && !separatorSeen)
    return -3; // drive letter is followed by something other than a separator

  return 0;
}
// LCOV_EXCL_STOP

Int32 ValidateVolumeList::validate( const char *value,
				  const NADefaults *nad,
				  Int32 attrEnum,
				  Int32 errOrWarn,
				  float *) const
{
  // Validate using a copy of "*value"
  char tempStr[1000];  // max length of ATTR_VALUE 
  if ( strlen(value) >= 1000 ) return FALSE;  // just in case
  if ( strlen(value) == 0 ) return TRUE;  // empty string ATTR_VALUE is OK
  strcpy(tempStr, value);

  // prepare to extract the partitions/tokens from the default string
  const char *token, *sep = " ,:" ;
  token = strtok( tempStr, sep );
  
  // iterate thru list of volume names; return false iff any name is invalid
  // (Also an appropriate error/warning would be issued.)
  while ( token != NULL ) 
  {
    NAString systemNameOnly(token);
    NAString volumeNameOnly(token);
    
    // check if a period is in the name that indicates that the volume
    // name is prefixed by the node name. The call to "first" returns
    // NA_POS if the period pattern does not exist in the name. If the 
    // period pattern exists then the system name is valid if it is at
    // least two characters long.
    size_t periodPos = systemNameOnly.first('.');
    if ((periodPos != NA_NPOS) && (periodPos > 1))
    {
       // extract the prefix
       systemNameOnly.remove(periodPos);

       // make sure that both system and volume names are valid
       // Although, system names are not valid for the HIST_SCRATCH_VOL attribute

       ComNodeName systemName (systemNameOnly.data());
       if (attrEnum == HIST_SCRATCH_VOL || NOT systemName.isValid())
       {
          if (errOrWarn)
	    *CmpCommon::diags() << DgSqlCode(ERRWARN(2055)) << DgString0(token) 
			        << DgString1(nad->lookupAttrName(attrEnum, errOrWarn));
          return FALSE;
       }
       
       // extract the volume name after the period
       volumeNameOnly.remove(0, periodPos+1);
    }

    // validate the volume name
    ComVolumeName volumeName (volumeNameOnly.data());
    if ( NOT volumeName.isValid() ) {
      if (errOrWarn)
	*CmpCommon::diags() << DgSqlCode(ERRWARN(2055)) << DgString0(token) 
			    << DgString1(nad->lookupAttrName(attrEnum, errOrWarn));
      return FALSE;
    }

    token = strtok( NULL, sep );
  }

  return TRUE;
}

Int32 ValidateTraceStr::validate( const char       *value,
                                const NADefaults *nad,
                                Int32               attrEnum,
                                Int32               errOrWarn,
                                float            *alreadyHaveFloat) const
{
  //  const char *curr = value;

  return TRUE;
}

// LCOV_EXCL_START :cnu
Int32 ValidateAnsiList::validate( const char *value,  
				const NADefaults *nad,
			 Int32 attrEnum,
			 Int32 errOrWarn,
				float *) const
{
  // Validate using a copy of "*value"
  char tempStr[1000];  // max length of ATTR_VALUE 
  if ( strlen(value) >= 1000 ) return FALSE;  // just in case
  if ( strlen(value) == 0 ) return TRUE;  // empty string ATTR_VALUE is OK
  strcpy(tempStr, value);

  // prepare to extract the partitions/tokens from the default string
  const char *token, *sep = " ,:" ;
  token = strtok( tempStr, sep );
  
  // iterate thru list of volume names; return false iff any name is invalid
  // (Also an appropriate error/warning would be issued.)
  while ( token != NULL ) {
    NAString tokenObj(token);
    Int32 countPeriods = 0, inBetween = 0;
    NABoolean anyError = tokenObj.isNull() ;
    // check three part ANSI name
    for (Int32 i = 0; !anyError && i < (Int32)tokenObj.length() ; i++ ) {
      if ( ComSqlText.isDigit(token[i]) ||
	   ComSqlText.isSimpleLatinLetter(token[i]) ) inBetween++;
      else {
	if ( ComSqlText.getPeriod() == token[i] &&  // it is a period
	     countPeriods++ < 2 ) {
	  if ( inBetween == 0 ) anyError = TRUE; // no CATALOG or SCHEMA
	  else inBetween = 0 ; // start counting the next ( SCHEMA or NAME )
	}
	else anyError = TRUE;
      }
    }
    if ( countPeriods != 2 || inBetween == 0 ) anyError = TRUE;

    if ( anyError ) {
      if (errOrWarn)
	*CmpCommon::diags() << DgSqlCode(ERRWARN(2055)) << DgString0(token) 
			    << DgString1("INVALID QUALIFIED NAME");
      return FALSE;
    }
    token = strtok( NULL, sep );
  }

  return TRUE;
}

Int32 ValidateRoleNameList::validate( const char *value,   
				const NADefaults *nad,
			 Int32 attrEnum,
			 Int32 errOrWarn,
				float *) const
{
  // Validate a list of role names.  Based on ValidateAnsiList
  // List format:  comma separated list of role names which use either . or _ 
  //  example:  "role.user1", "ROLE.user2", "role_user3"
  // SeaQuest example:  DB__Root, DB__Services, db_role12, db_user3

  // Validate using a copy of "*value"
  char tempStr[1000];  // max length of ATTR_VALUE 
  if ( strlen(value) >= 1000 ) return FALSE;  // just in case
  if ( strlen(value) == 0 ) return TRUE;  // empty string ATTR_VALUE is OK
  strcpy(tempStr, value);

  // prepare to extract the role names/tokens from the default string
  const char *token, *sep = " ," ;
  token = strtok( tempStr, sep );
  
  // iterate thru list of role names; return false iff any name is invalid
  // (Also an appropriate error/warning would be issued.)
  while ( token != NULL ) {
    NAString tokenObj(token);
    Lng32 sqlcode = ToInternalIdentifier(tokenObj, TRUE /*upCase*/,
                                         FALSE /*acceptCircumflex*/,
                                         0 /*toInternalIdentifierFlags*/);
    if (sqlcode && ABS(sqlcode) != 3128)
    {
      // 3004 A delimited identifier must contain at least one character.
      // 3118 Identifier too long.
      // 3127 Illegal character in identifier $0~string0.
      // 3128 $1~string1 is a reserved word.
      if (errOrWarn)
	*CmpCommon::diags() << DgSqlCode(ERRWARN(2055)) << DgString0(token) 
			    << DgString1("INVALID AUTHORIZATION IDENTIFIER");
    }
    token = strtok( NULL, sep );
  }

  return TRUE;
}
// LCOV_EXCL_STOP

Int32 ValidatePOSTableSizes::validate(const char *value,
                                    const NADefaults *nad,
                                    Int32 attrEnum,
                                    Int32 errOrWarn,
                                    float *) const
{
  char tempStr[1000];  // max length of ATTR_VALUE

  if ( strlen(value) == 0 ) return TRUE;  // empty string ATTR_VALUE is OK

  if ( strlen(value) > 1000 )
  {
    *CmpCommon::diags() << DgSqlCode(ERRWARN(2055))
                        << DgString0(value)
                        << DgString1(nad->lookupAttrName(attrEnum, errOrWarn));

    return FALSE;
  }

  strcpy(tempStr, value);

  const char *token, *sep = " ,";
  token = strtok(tempStr, sep);

  float initialSize = -1;
  float maxSize = -1;
  ValidateUInt  uint;
  
  if (token != NULL)
  {
    // check if the first value is an unsigned int
    if (!uint.validate(token, nad, attrEnum, -1))
      return FALSE;
    else
    {
      sscanf(token, "%g", &initialSize);
      token = strtok(NULL, sep);
      if (token != NULL)
      {
        // check if the second value is an unsigned int
        if (!uint.validate(token, nad, attrEnum, -1))
          return FALSE;
        else
        {
          // if there is a third value or max table size is smaller than
          // initial table size raise an error
          sscanf(token, "%g", &maxSize);
          token = strtok(NULL, sep);
          if (token != NULL || maxSize < initialSize)
          {
            *CmpCommon::diags() << DgSqlCode(ERRWARN(2055))
              << DgString0(value)
              << DgString1(nad->lookupAttrName(attrEnum, errOrWarn));
  
            if (maxSize < initialSize)
            {
              *CmpCommon::diags() << DgSqlCode(ERRWARN(2077))
                << DgInt0((Lng32)maxSize)
                << DgInt1((Lng32)initialSize);
            }
            return FALSE;
          }
        }
      }
    }
  }
  else
  {
    *CmpCommon::diags() << DgSqlCode(ERRWARN(2055))
                        << DgString0(value)
                        << DgString1(nad->lookupAttrName(attrEnum, errOrWarn));

     return FALSE;
  }

  return TRUE;
}


Int32 ValidateNumericRange::validate( const char *value,
				    const NADefaults *nad,
				    Int32 attrEnum,
				    Int32 errOrWarn,
				    float *alreadyHaveFloat) const
{
  Int32 isValid = FALSE, rangeOK = FALSE, multipleOK = FALSE;
  float flt;

  if (alreadyHaveFloat)
    flt = *alreadyHaveFloat;
  else {
    isValid = nad->validateFloat(value, flt, attrEnum, SilentIfSYSTEM);
    if (isValid == SilentIfSYSTEM) return SilentIfSYSTEM;
  }

  if (alreadyHaveFloat || isValid) {

    rangeOK = (min_ <= flt && flt <= max_);
    if (rangeOK) {
      multipleOK = (multiple_ == 0 || ((ULng32)flt) % multiple_ == 0);
      if (multipleOK) {
	return TRUE;				// valid
      }
    }
  }

  if (alreadyHaveFloat) *alreadyHaveFloat = min_;

  if (errOrWarn) {

    *CmpCommon::diags() << DgSqlCode(ERRWARN(2055))
      << DgString0(value)
      << DgString1(nad->lookupAttrName(attrEnum, errOrWarn));

    if (!rangeOK) {
      char minbuf[50], maxbuf[50];
      
      if (type_ == VALID_INT) {
	sprintf(minbuf, "%d", (Lng32)min_);
	

	// A fudge factor of 64 is added for NT/Yos/Neo to include 
	// the precision lost. The value printed is 2147483520, 
	// while values are accepted till 2147483584. 
	sprintf(maxbuf, "%d", (Lng32)max_+64);
     }
      else if (type_ == VALID_UINT) {
	sprintf(minbuf, "%u", (ULng32)min_);
	if (max_ == 2147483520)
	  sprintf(maxbuf, "%u", (ULng32)max_+64);
        else
	  sprintf(maxbuf, "%u", (ULng32)max_);

      }
      else {
	sprintf(minbuf, "%g", min_);
	sprintf(maxbuf, "%g", max_);
      }
      NAString range = NAString("[") + minbuf + "," + maxbuf + "]";
      *CmpCommon::diags() << DgSqlCode(ERRWARN(2056)) << DgString0(range);
    }

    if (multiple_ && !multipleOK)
#pragma nowarn(1506)   // warning elimination 
      *CmpCommon::diags() << DgSqlCode(ERRWARN(2057)) << DgInt0(multiple_);
#pragma warn(1506)  // warning elimination 

  }

  return FALSE;					// not valid
}

//-----------------------------------------------------------------------------
// ValidateCollationList:
//	The motivation for this class is as a temporary STOPGAP measure
//	in MX-NSK-Rel1, to (half)support MP COLLATEd columns.
//	See the MP_COLLATIONS default attr, the ReadTableDef code that
//	also calls the insertIntoCDB (CollationDB) method below, and
//	the "SYNTAX OF THE MP_COLLATIONS LIST" comments below.
//
//	A more fully fledged support would require a new ReadTableDef request
//	to garner information about the collation from the MP CPRULES table,
//	namely, a) validation that the collation exists,
//	b) the CPRULES.CHARACTERISTICS value,
//	c) CPRULES.CHARACTERSET value.
//-----------------------------------------------------------------------------

Int32 ValidateCollationList::validate(const char *value,
				    const NADefaults *nad,
				    Int32 attrEnum,
				    Int32 errOrWarn,
				    float *) const
{
  if (errOrWarn) {
    CMPASSERT(nad && attrEnum);
    errOrWarn = +1;			// warnings only (else silent)
  }

  // Cast away constness on various private members which are really implicit
  // arguments (the validate method by itself vs. from insertIntoCDB).
  ValidateCollationList *ncThis = (ValidateCollationList *)this;

  #ifndef NDEBUG
    if (getenv("NCHAR_DEBUG")) ncThis->formatRetry_ = TRUE;
  #endif

  // If cdb is NULL, we are validating only; if nonNULL, we are inserting.
  CollationDB *cdb = ncThis->cdb_;
  const ComMPLoc   *defaultMPLoc  = NULL;
  const SchemaName *defaultSchema = NULL;
  if (cdb) {
    CMPASSERT(sdb_);
    defaultMPLoc  = &SqlParser_MPLOC;
    defaultSchema = &sdb_->getDefaultSchema();
  }

  // Make our own copy of the value string data,
  // on which we can safely cast away const-ness for the loop,
  // which chops the list up into individual names by replacing
  // (in place, i.e. w/o additional copying)
  // each semicolon with a string-terminating nul.
  NAString collNAS(value);
  char *collList = (char *)collNAS.data();

  while (*collList) {

    // SYNTAX OF THE MP_COLLATIONS LIST:
    //
    // The list may look like any of these:
    //	''		(empty)
    //	'collname'
    //	'c1; sv.c2; $v.sv.c3; \s.$v.sv.c4'
    //	'c1; sv.c2; $v.sv.c3; \s.$v.sv.c4;'
    //	' = c1; =sv.c2;=$v.sv.c3;\s.$v.sv.c4;'
    // We also ignore pathologies like
    //	'   '  or  '=========;;;;;===;    ; =  = == =; ; ; '
    //
    // The '=' flags the following collation name as one that has a 1:1 mapping,
    // i.e. its CPRULES.CHARACTERISTICS == 'O',
    // i.e. in Rel1 it allows only equality comparisons
    //      (SQL '=', '<>', DISTINCT, GROUP BY),
    // although no other operations
    // (other predicates, ordering, MIN/MAX, partitioning)
    // would be disallowed -- the column wouldn't even be updatable,
    // but it could be at least read and appear in some limited other places.
    //
    // The absence of a '=' means the collation's CHARACTERISTICS == 'N',
    // and in Rel1 we cannot support *any* comparisons, predicates, MIN/MAX,
    // ordering, partitioning, on it --
    // it's basically just a name attached to a column,
    // but the intent was to allow the column to be at least readable.
    //
    // See ReadTableDef and NATable to see if MP COLLATEd column support
    // is or is not currently being enabled.

    // Find the next semicolon separator outside of delim-ident quotes,
    // or find end of string.
    // Set collStr to be the fragment up to (excluding) the semicolon or zero;
    // set collList to be the rest of itself (after the semicolon).
    //
// LCOV_EXCL_START :mp
    char *s = collList;
    for (NABoolean quoted = FALSE; *s; s++) {
      if (*s == '"')
	quoted = !quoted;
      else if (*s == ';' && !quoted)
	break;
    }
    char sep = *s;			// sep is either ';' or '\0'
    *s = '\0';
    NAString collStr(collList);
    collList = sep ? ++s : s;		// get past ';' OR stay on final '\0'
    
    CollationInfo::CollationFlags flags = CollationInfo::ALL_CMP_ILLEGAL;

    TrimNAStringSpace(collStr);		// remove leading/trailing blanks
    size_t i = 0, n = collStr.length();
    while (i < n)
      if (collStr[i] == '=' || collStr[i] == ' ')
	i++;				// get past leading '=' (and blanks)
      else
        break;
    if (i) {				// an '=' was seen, EQ_NE_CMP is LEGAL
      flags = CollationInfo::ORDERED_CMP_ILLEGAL;
      collStr.remove(0, i);
    }

    if (!collStr.isNull()) {
      NABoolean ok = FALSE;
      NABoolean nsk = formatNSK_;
      NABoolean retry = formatRetry_;

      retry_as_other_format:

	if (nsk) {
	  ComMPLoc collMP(collStr, ComMPLoc::FILE);
	  if (collMP.isValid(ComMPLoc::FILE)) {
	    ok = TRUE;
	    if (cdb) {
	      ncThis->lastCoInserted_ =
	        cdb->insert(collMP, defaultMPLoc, flags);
	    }
	  }
	}
	else {
	  ComObjectName collMX(collStr);
	  if (collMX.isValid()) {
	    ok = TRUE;
	    if (cdb) {
	      QualifiedName collQN(collMX);
	      ncThis->lastCoInserted_ =
	        cdb->insert(collQN, defaultSchema, flags);
	    }
	  }
	}

      if (ok) ncThis->cntCoParsed_++;

      if (!ok && retry) {
        retry = FALSE;			// Retry only once,
	nsk = !nsk;			// using the other name format.
	goto retry_as_other_format;
      }

      if (!ok && errOrWarn)
	*CmpCommon::diags() << DgSqlCode(ERRWARN(2055))
	    << DgString0(collStr)
	    << DgString1(nad->lookupAttrName(attrEnum, errOrWarn));

// LCOV_EXCL_STOP
    }	// !collStr.isNull()
  } // while

  return TRUE;				// warnings only; all is valid
}

UInt32 ValidateCollationList::insertIntoCDB(SchemaDB *sdb,
					      CollationDB *cdb,
					      const char *value,
					      NABoolean nsk)
{
  NABoolean savformatNSK = formatNSK_;
  formatNSK_ = nsk;

  lastCoInserted_ = CharInfo::UNKNOWN_COLLATION;
  cntCoParsed_ = 0;

  CMPASSERT(!cdb_ && !sdb_);	// our kludgy/dodgy passing mechanism...
  cdb_ = cdb;
  sdb_ = sdb;
  validate(value, NULL, 0, 0/*silent*/);
  cdb_ = NULL;
  sdb_ = NULL;

  formatNSK_ = savformatNSK;

  #ifndef NDEBUG
    if (getenv("NCHAR_DEBUG")) CollationDB::Display();
  #endif

  return cntCoParsed_;
}

// validate OVERRIDE_SCHEMA FROM_SCHEMA:TO_SCHEMA setting
// format validation for xxx:yyy
Int32 ValidateOverrideSchema::validate( const char *value,
                                    const NADefaults *nad,
                                    Int32 attrEnum,
                                    Int32 errOrWarn,
                                    float *flt ) const
{
  NABoolean ok = TRUE;
  Int32 len = strlen(value);

  if (len == 0)                             // empty is ok
    return ok;

  NAString fromSchema, toSchema;

  extractOverrideSchemas(value, fromSchema, toSchema);

  if ( (fromSchema.isNull()) || (toSchema.isNull()) )
    ok = FALSE;  
  else
  {
    ComSchemaName targetSchema(toSchema);
    if (!targetSchema.isValid())
      ok = FALSE;
    else
      if (fromSchema!="*")  // reserve * for wildcard operation
      {
        ComSchemaName sourceSchema(fromSchema);
        if (!sourceSchema.isValid())
          ok = FALSE; 
      }
  }
 
  if (!ok && errOrWarn)
    *CmpCommon::diags() << DgSqlCode(ERRWARN(2055))
	<< DgString0(value)
	<< DgString1(nad->lookupAttrName(attrEnum, errOrWarn));

  return ok;

}

// validate PUBLIC_SCHEMA_NAME setting
// setting may be schema or catalog.schema
Int32 ValidatePublicSchema::validate( const char *value,
                                    const NADefaults *nad,
                                    Int32 attrEnum,
                                    Int32 errOrWarn,
                                    float *flt ) const
{
  NABoolean ok = TRUE;
  Int32 len = strlen(value);

  if (len == 0)                             // empty is ok
    return ok;

  ComSchemaName pubSchema(value);
  if (!pubSchema.isValid())
    ok = FALSE;
 
  if (!ok && errOrWarn)
    *CmpCommon::diags() << DgSqlCode(ERRWARN(2055))
	<< DgString0(value)
	<< DgString1(nad->lookupAttrName(attrEnum, errOrWarn));

  return ok;

}

// validate REPLICATE_IO_VERSION setting
// setting may be schema or catalog.schema
Int32 ValidateReplIoVersion::validate( const char *value,
                                    const NADefaults *nad,
                                    Int32 attrEnum,
                                    Int32 errOrWarn,
                                    float *alreadyHaveFloat ) const
{
  Int32 isValid = FALSE;
  float flt;
  Int32 min;

  if (alreadyHaveFloat)
    flt = *alreadyHaveFloat;
  else {
    isValid = nad->validateFloat(value, flt, attrEnum, SilentIfSYSTEM);
    if (isValid == SilentIfSYSTEM) return SilentIfSYSTEM;
  }

  if (alreadyHaveFloat || isValid) 
  {
    if (Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
       min = 1;
    else
       min = min_; 
    if (flt >= min && flt <= max_)
       return TRUE;                            // valid
  }

  if (alreadyHaveFloat) *alreadyHaveFloat = (float)min_;

  if (errOrWarn)
    *CmpCommon::diags() << DgSqlCode(ERRWARN(2055))
        << DgString0(value)
        << DgString1(nad->lookupAttrName(attrEnum, errOrWarn));

  return FALSE;

}

// validate parameter for MV_AGE
Int32 ValidateMVAge::validate( const char *value,
                               const NADefaults *nad,
                               Int32 attrEnum,
                               Int32 errOrWarn,
                               float *alreadyHaveFloat ) const
{
  Int32 isOK = FALSE;
  float number=0;
  char textChars[20];

  if (strlen(value) < 15)
  {
    if (sscanf(value, "%f %s", &number, textChars) == 2)
    {
      const NAString text(textChars);
      if (!text.compareTo("Seconds", NAString::ignoreCase))
      {
	isOK = TRUE;
      }
      else if (!text.compareTo("Minutes", NAString::ignoreCase))
      {
	isOK = TRUE;
      }
      else if (!text.compareTo("Hours", NAString::ignoreCase))
      {
	isOK = TRUE;
      }
      else if (!text.compareTo("Days", NAString::ignoreCase))
      {
	isOK = TRUE;
      }
    }
  }

  if (!isOK)
  {
    if (errOrWarn)
      *CmpCommon::diags() << DgSqlCode(ERRWARN(2055))
          << DgString0(value)
          << DgString1(nad->lookupAttrName(attrEnum, errOrWarn));
  }
  
  return isOK;
}
