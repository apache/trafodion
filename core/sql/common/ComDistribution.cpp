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
 * File:         ComDistribution.cpp
 * Description:  Supports distributed databases.
 *               
 * Created:      5/23/2003
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "ComCextdecs.h"

/*****************************************************************************

  Function ComCheckPartitionAvailability (char * partName, 
                                          AvailabilityErrorCode *errorCode)

From the IS:

- It checks that the partition name passed as an input parameter is a valid 
  fully qualified Guardian name in external file format.
- It calls FILE_GETINFOLISTBYNAME_ to check to see if the partition exists 
  and can be accessed, i.e., that there are no errors retrieving information 
  about the partition, and the corrupt and rollforwardneeded flags are not set 
  in the label.
- If FILE_GETINFOLISTBYNAME_ does not return any errors, 
  ComCheckPartitionAvailability returns TRUE, indicating that the partition is 
  available. 
- If FILE_GETINFOLISTBYNAME_ does return an error, ComCheckPartitionAvailability
  returns FALSE, indicating that the partition is not available. 

Also has some debug-only support for testing, using env vars. Set the env vars 
to make ComCheckPartitionAvailability return FALSE. A helper function
ComCheckPartAvailabilityFromEnvVars provides this functionality.

  DIST_UNAVAIL_NODE_<nodename>              #no volumes avail on this node
  DIST_UNAVAIL_NODEVOL_<nodename>_<vol>     #specific vol unavail on this node
  DIST_UNAVAIL_PART_<nodename>_<vol>_<subvol>_<part>     
                                            #specific part unavail on this node

  For example:

  export DIST_UNAVAIL_NODE_TEXMEX           #note that the value doesn't matter
  unset  DIST_UNAVAIL_NODE_TEXMEX

  export DIST_UNAVAIL_NODE_TEXMEX_DATA14    #note no dollar sign on the vol.
  unset  DIST_UNAVAIL_NODE_TEXMEX_DATA14   

  export DIST_UNAVAIL_PART_TEXMEX_DATA14_ZSD1GTBD_F7DZMV00
  unset  DIST_UNAVAIL_PART_TEXMEX_DATA14_ZSD1GTBD_F7DZMV00

In addition to this, an error code is returned that indicates the type of 
availability error.

******************************************************************************/

#include "NABoolean.h"
#include "NAStdlib.h"
#include "NAString.h"

#include "ComDistribution.h"
#include "ComASSERT.h"

#include "ExpError.h"
#include "csconvert.h"
#include "NLSConversion.h"

// -----------------------------------------------------------------------
// ANSI SQL Name Conversion helpers
//
// returned error code described in w:/common/csconvert.h
//
// MBCS stands for the default ANSI name variable-length/width Multi-Byte
// Character Set (i.e., the UTF8 character set).
// -----------------------------------------------------------------------

Int32 ComAnsiNameToUTF8 ( const NAWchar * inAnsiNameInUCS2  // in  - valid ANSI SQL name in UCS2
                        , char *      outBuf4AnsiNameInMBCS // out - out buffer
                        , const Int32 outBufSizeInBytes     // in  - out buffer max len in bytes
                        )
{
  if (outBuf4AnsiNameInMBCS == NULL || outBufSizeInBytes <= 0)
    return -2; // CNV_ERR_BUFFER_OVERRUN - No output buffer or not big enough
  if (inAnsiNameInUCS2 == NULL)
    return -3; // CNV_ERR_NOINPUT - No input buffer or input cnt <= 0 
  else if (NAWstrlen(inAnsiNameInUCS2) == 0)
  {
    outBuf4AnsiNameInMBCS[0] = 0;
    return 0; // success
  }

  const Int32 inAnsiNameInBytes = NAWstrlen(inAnsiNameInUCS2) * BYTES_PER_NAWCHAR;
  Int32 ansiNameCharSet = (Int32)ComGetNameInterfaceCharSet();
  Int32 convAnsiNameCS  = (Int32)/*cnv_charset*/convertCharsetEnum (ansiNameCharSet);
  char * pFirstByteOfTheUntranslatedChar = NULL;
  UInt32 iOutStrLenInBytesIncludingNull = 0;
  UInt32 iNumTranslatedChars = 0;
  Int32  iConvErrorCode = UTF16ToLocale
    ( cnv_version1                         // in     - const enum cnv_version
    , (const char *) inAnsiNameInUCS2      // in     - const char *     in_bufr
    , (Int32) inAnsiNameInBytes            // in     - const Int32      in_len_in_bytes
    , (const char *) outBuf4AnsiNameInMBCS // in/out - const char *     out_bufr
    , (Int32) outBufSizeInBytes            // in     - const Int32      out_bufr_max_len_in bytes
    , (cnv_charset) convAnsiNameCS         // in     - enum cnv_charset conv_charset
    , pFirstByteOfTheUntranslatedChar      // out    - char * &         first_untranslated_char
    , &iOutStrLenInBytesIncludingNull      // out    - UInt32 *         output_data_len_p
    , 0                                    // in     - const Int32      conv_flags
    , (Int32) TRUE                         // in     - const Int32      add_null_at_end_Flag
    , (Int32) FALSE                        // in     - const int32      allow_invalids
    , &iNumTranslatedChars                 // out    - UInt32 *         translated_char_cnt_p
    , (const char *) NULL /* i.e. "?" */   // in     - const char *     substitution_char = NULL
    );

  return iConvErrorCode;
}

// returned error code described in w:/common/csconvert.h
Int32 ComAnsiNameToUCS2 ( const char * inAnsiNameInMBCS      // in  - valid name in default ANSI name char set
                        , NAWchar *    outBuf4AnsiNameInUCS2 // out - out buffer
                        , const Int32  outBufSizeInNAWchars  // in  - out buffer max len in NAWchars
                        , const NABoolean padWithSpaces      // in  - default is FALSE
                        )
{
  if (outBuf4AnsiNameInUCS2 == NULL || outBufSizeInNAWchars <= 0)
    return -2; // CNV_ERR_BUFFER_OVERRUN - No output buffer or not big enough
  if (inAnsiNameInMBCS == NULL)
    return -3; // CNV_ERR_NOINPUT - No input buffer or input cnt <= 0 
  else if (strlen(inAnsiNameInMBCS) == 0)
  {
    outBuf4AnsiNameInUCS2[0] = 0;
    return 0; // success
  }

  Int32 inAnsiNameLenInBytes = (Int32)strlen(inAnsiNameInMBCS);
  Int32 outBufSizeInBytes = outBufSizeInNAWchars * BYTES_PER_NAWCHAR;
  Int32 ansiNameCharSet = (Int32)ComGetNameInterfaceCharSet();
  Int32 convAnsiNameCS  = (Int32)/*cnv_charset*/convertCharsetEnum (ansiNameCharSet);
  char * pFirstByteOfTheUntranslatedChar = NULL;
  UInt32 iTranslatedStrLenInBytes = 0;
  UInt32 iNumberOfTranslatedChars = 0;
  Int32  iConvErrorCode = LocaleToUTF16
    ( cnv_version1                         // in  - const enum cnv_version version
    , inAnsiNameInMBCS                     // in  - const char *  in_bufr
    , (Int32) inAnsiNameLenInBytes         // in  - const Int32   in_len_in_bytes
    , (const char *) outBuf4AnsiNameInUCS2 // out - const char *  out_bufr
    , (Int32)(outBufSizeInBytes -  BYTES_PER_NAWCHAR) // in - const Int32 out_bufr_max_len_in_bytes
    , (cnv_charset) convAnsiNameCS         // in  - enum cnv_charset conv_charset
    , pFirstByteOfTheUntranslatedChar      // out - char * &      first_untranslated_char
    , &iTranslatedStrLenInBytes            // out - UInt32 *      output_data_len_p
    , (Int32) 0                            // in  - const Int32   conv_flags
    , (Int32) FALSE                        // in  - const Int32   addNullAtEnd_flag
    , &iNumberOfTranslatedChars            // out - UInt32 *      translated_char_cnt_p
 // , 0xffffffff                           // in  - UInt32 max_chars_to_convert = 0xffffffff
    );
  Int32 outStrLenInNAWchars = iTranslatedStrLenInBytes / BYTES_PER_NAWCHAR;
  outBuf4AnsiNameInUCS2[outStrLenInNAWchars]  = 0; // Append the NULL terminator

  if (iConvErrorCode == 0 && padWithSpaces)
  {
    wc_str_pad ( (NAWchar *) &outBuf4AnsiNameInUCS2[outStrLenInNAWchars] // out - NAWchar *str
               , outBufSizeInNAWchars - outStrLenInNAWchars - 1 // in  - Int32 length
               , unicode_char_set::SPACE   // in  - NAWchar padchar = unicode_char_set::SPACE
               );
    outBuf4AnsiNameInUCS2[outBufSizeInNAWchars-1] = 0; // Append the NULL terminator
  }
  return iConvErrorCode;
}


// -----------------------------------------------------------------------
// Meatadata Distribution
// -----------------------------------------------------------------------

//----------------------------------------------------------------------
//
//  Build an ANSI schema name from its individual parts
//
void 
ComBuildSchemaName ( const char * catalogName,   // in, catalog name (internal format)
                     const char * schemaName,    // in, schema name (internal format)
                     char * ansiSchemaName,      // out, ANSI schema name (external format)
                     const Int32 ansiSchNameBufSize) // in, ANSI schema name output buffer size in bytes
{
  size_t actualLength;
  char * ptr = ansiSchemaName;

  // Convert the catalog name to external format
  ToAnsiIdentifier3 (catalogName, 
                     strlen(catalogName),
                     ptr, 
                     ansiSchNameBufSize,
                     &actualLength);

  ComASSERT (actualLength);
  ptr[actualLength] = '.';
  ptr += (actualLength + 1);

  // Convert the schema name to external format
  ToAnsiIdentifier3 (schemaName, 
                     strlen(schemaName),
                     ptr, 
                     ansiSchNameBufSize - actualLength - 1, // remaining available space in ouput buffer
                     &actualLength);

  ComASSERT (actualLength);
}
//----------------------------------------------------------------------
//
//  Build an ANSI name from its individual parts
//
void 
ComBuildANSIName ( const char * catalogName,   // in, catalog name (internal format) 
		   const char * schemaName,    // in, schema name (internal format)
                   const char * objectName,    // in, object name (internal format)
                   char * ansiName,            // out, ANSI name (external format)
                   const Int32 ansiNameOutBufSize) // in, ANSI name output buffer size in bytes
{

  ComBuildSchemaName (catalogName, schemaName, ansiName, ansiNameOutBufSize);

  size_t actualLength = strlen(ansiName);
  char * ptr = ansiName;

  ptr[actualLength] = '.';
  ptr += (actualLength + 1);
  ComASSERT (actualLength);
}

// General enum to literal translation
void enumToLiteral ( const literalAndEnumStruct * conversionTable,
                     const Int32 noOfElements,
                     const Int32 enumValue,
                     char * literal,
                     NABoolean & found)
{
  for (Int32 i = 0;i < noOfElements; i++)
  {
    const literalAndEnumStruct & elem = conversionTable[i];
    if (elem.enum_ == enumValue)
    {
      strcpy (literal, elem.literal_);
      found = TRUE;
      return;
    }
  }
  // Didn't find it in the table - bummer!
  found = FALSE;
}

// General literal to enum translation
Int32 literalToEnum (const literalAndEnumStruct * conversionTable,
                   const Int32 noOfElements,
                   const char * literal,
                   NABoolean & found)
{
  for (Int32 i = 0;i < noOfElements; i++)
  {
    const literalAndEnumStruct & elem = conversionTable[i];
    if (!strcmp (elem.literal_, literal))
    {
      found = TRUE;
      return elem.enum_;
    }
  }
  // Didn't find it in the table - bummer!
  found = FALSE;
  return 0;
}

const literalAndEnumStruct qiTypeConversionTable [] =
{
  {COM_QI_INVALID_ACTIONTYPE, COM_QI_INVALID_ACTIONTYPE_LIT},
  {COM_QI_GRANT_ROLE, COM_QI_GRANT_ROLE_LIT},
  {COM_QI_USER_GRANT_ROLE, COM_QI_USER_GRANT_ROLE_LIT},
  {COM_QI_ROLE_GRANT_ROLE, COM_QI_ROLE_GRANT_ROLE_LIT},
  {COM_QI_COLUMN_SELECT, COM_QI_COLUMN_SELECT_LIT},
  {COM_QI_COLUMN_INSERT, COM_QI_COLUMN_INSERT_LIT},
  {COM_QI_COLUMN_UPDATE, COM_QI_COLUMN_UPDATE_LIT},
  {COM_QI_COLUMN_REFERENCES, COM_QI_COLUMN_REFERENCES_LIT},
  {COM_QI_OBJECT_SELECT, COM_QI_OBJECT_SELECT_LIT},
  {COM_QI_OBJECT_INSERT, COM_QI_OBJECT_INSERT_LIT},
  {COM_QI_OBJECT_DELETE, COM_QI_OBJECT_DELETE_LIT},
  {COM_QI_OBJECT_UPDATE, COM_QI_OBJECT_UPDATE_LIT},
  {COM_QI_OBJECT_USAGE, COM_QI_OBJECT_USAGE_LIT},
  {COM_QI_OBJECT_REFERENCES, COM_QI_OBJECT_REFERENCES_LIT},
  {COM_QI_SCHEMA_SELECT, COM_QI_SCHEMA_SELECT_LIT},
  {COM_QI_SCHEMA_INSERT, COM_QI_SCHEMA_INSERT_LIT},
  {COM_QI_SCHEMA_DELETE, COM_QI_SCHEMA_DELETE_LIT},
  {COM_QI_SCHEMA_UPDATE, COM_QI_SCHEMA_UPDATE_LIT},
  {COM_QI_SCHEMA_USAGE, COM_QI_SCHEMA_USAGE_LIT},
  {COM_QI_SCHEMA_REFERENCES, COM_QI_SCHEMA_REFERENCES_LIT},
  {COM_QI_OBJECT_EXECUTE, COM_QI_OBJECT_EXECUTE_LIT},
  {COM_QI_SCHEMA_EXECUTE, COM_QI_SCHEMA_EXECUTE_LIT},
  {COM_QI_USER_GRANT_SPECIAL_ROLE, COM_QI_USER_GRANT_SPECIAL_ROLE_LIT},
  {COM_QI_OBJECT_REDEF, COM_QI_OBJECT_REDEF_LIT},
  {COM_QI_STATS_UPDATED, COM_QI_STATS_UPDATED_LIT}
};

//----------------------------------------------------------------------
//
// Query Invalidation Action type translations
//
void ComQIActionTypeEnumToLiteral (const ComQIActionType qiType,
                             char* qiTypeLiteral)
{
  NABoolean found;
  enumToLiteral ( qiTypeConversionTable, occurs(qiTypeConversionTable), qiType, qiTypeLiteral, found);

  ComASSERT (found);
}

ComQIActionType ComQIActionTypeLiteralToEnum (const char * qiTypeLiteral)
{
  NABoolean found;
  ComQIActionType result =
          (ComQIActionType) literalToEnum (qiTypeConversionTable, occurs(qiTypeConversionTable), qiTypeLiteral, found);
  ComASSERT (found);

  return result;
}
