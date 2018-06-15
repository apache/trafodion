/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComAnsiNamePart.C
 * Description:  methods for class ComAnsiNamePart
 *               
 *               
 * Created:      7/21/95
 * Language:     C++
 *
 *
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
 *
 *
 *****************************************************************************
 */


#include "Platform.h"

#include "ComASSERT.h"
#include "csconvert.h"
#include "ComDistribution.h"
#include "ComAnsiNamePart.h"
#include "ComSqlText.h"
#include "NAString.h"
#include "CatSQLShare.h"
#include "nawstring.h"

extern cnv_charset convertCharsetEnum (Int32 /* SQLCHARSET_CODE */ charSet);

// -----------------------------------------------------------------------
// Functions to convert ANSI SQL names from the common for-internal-
// processing character set (e.g., the UTF8 character set) to UCS2/UTF16
// character set and vice versa.
//
// Similar APIs with char* and NAWchar* parameters in place of
// NAString and NAWString parameters are declared and defined
// in the header and source files w:/common/ComDistribution.h and
// .cpp for used by low-level code like the one in the source
// file w:/comexe/LateBindInfo.cpp which cannot use the process heap.
// -----------------------------------------------------------------------

// returned error code described in w:/common/csconvert.h
Int32 ComAnsiNameToUTF8 ( const NAWString &inAnsiNameInUCS2 // in  - valid ANSI SQL name in UCS2
                        , NAString & outAnsiNameInMBCS      // out - out buffer
                        )
{
  if (inAnsiNameInUCS2.length() <= 0)
  {
    outAnsiNameInMBCS.remove(0); // set to an empty string
    return 0; // success
  }
  const Int32 outBufSizeInBytes = ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES + 1 + 8;
  char outBufInChars[outBufSizeInBytes + 8]; // prepare for the worst case + allocate a few more bytes
  Int32 iErrorCode = 0;
  iErrorCode = ComAnsiNameToUTF8 ( inAnsiNameInUCS2.data()  // in  - const NAWchar *
                                 , outBufInChars            // out - char *
                                 , outBufSizeInBytes        // in  - const Int32
                                 );
  if (iErrorCode == 0)
    outAnsiNameInMBCS = outBufInChars;  // a replace operation - i.e., NOT append
  else
    outAnsiNameInMBCS.remove(0);        // set to an empty string
  return iErrorCode;
}

// returned error code described in w:/common/csconvert.h
Int32 ComAnsiNameToUCS2 ( const NAString & inAnsiNameInMBCS // in  - valid name in default ANSI name char set
                        , NAWString & outAnsiNameInUCS2     // out - out buffer
                        )
{
  if (inAnsiNameInMBCS.isNull())
  {
    outAnsiNameInUCS2.remove(0); // set to an empty string
    return 0; // success
  }
  const Int32 outBufSizeInNAWchars = ComMAX_3_PART_EXTERNAL_UCS2_NAME_LEN_IN_NAWCHARS + 1 + 4;
  NAWchar outBufInNAWchars[outBufSizeInNAWchars + 4]; // allocate a few more NAWchar elements
  Int32 iErrorCode = 0;
  iErrorCode = ComAnsiNameToUCS2 ( inAnsiNameInMBCS.data()  // in  - const char *
                                 , outBufInNAWchars         // out - NAWchar *
                                 , outBufSizeInNAWchars     // in  - const Int32
                                 );
  if (iErrorCode == 0)
    outAnsiNameInUCS2 = outBufInNAWchars; // a replace operation - i.e., NOT append
  else
    outAnsiNameInUCS2.remove(0);          // set to an empty string
  return iErrorCode;
}

// -----------------------------------------------------------------------
// friend functions
// -----------------------------------------------------------------------

//
// ostream::operator<<
//
ostream& operator<< (ostream &out, const ComAnsiNamePart &name)
{
  out << "CLASS:  ComAnsiNamePart" << endl;
  out << "   externalName_[] = ";
  out << '[' << name.externalName_.length() << "] ";
  out << '"' << name.externalName_ << '"';
  return (out);
}

// -----------------------------------------------------------------------
// Constructors
// -----------------------------------------------------------------------

ComAnsiNamePart::ComAnsiNamePart (CollHeap * h,
                                  unsigned short toInternalIdentifierFlags) 
     : internalName_(h), 
       externalName_(h),
       toInternalIdentifierFlags_(toInternalIdentifierFlags),
       heap_ (h)
{}

ComAnsiNamePart::ComAnsiNamePart (const ComAnsiNamePart & orig,
                                  CollHeap * h)
     : internalName_(orig.internalName_, h),
       externalName_(orig.externalName_, h),
       toInternalIdentifierFlags_(orig.toInternalIdentifierFlags_),
       heap_ (h)
{}

ComAnsiNamePart::ComAnsiNamePart(const char *namePart,
                                 size_t nameLen,
                                 formatEnum format,
                                 CollHeap * h,
                                 unsigned short toInternalIdentifierFlags) 
     : internalName_(h),
       externalName_(h),
       toInternalIdentifierFlags_(toInternalIdentifierFlags),
       heap_ (h)
{
  if (!namePart) return;
  NAString namePartTmp(namePart, nameLen, h);

  if (format EQU EXTERNAL_FORMAT)
    copyExternalName(namePartTmp);
  else
    copyInternalName(namePartTmp);
}

ComAnsiNamePart::ComAnsiNamePart(const NAString &namePart,
                                 formatEnum format,
                                 CollHeap * h,
                                 unsigned short toInternalIdentifierFlags) 
     : internalName_(h),
       externalName_(h),
       toInternalIdentifierFlags_(toInternalIdentifierFlags),
       heap_ (h)
{
  if (format EQU EXTERNAL_FORMAT)
    copyExternalName(namePart);
  else
    copyInternalName(namePart);
}

ComAnsiNamePart::ComAnsiNamePart(const char *externalNameParts,
                                 size_t externalNamePartsLen,
                                 size_t &count,
                                 CollHeap * h,
                                 unsigned short toInternalIdentifierFlags)
     : internalName_(h),
       externalName_(h),
       toInternalIdentifierFlags_(toInternalIdentifierFlags),
       heap_ (h)
{
  if (!externalNameParts) return;
  NAString nameParts(externalNameParts, externalNamePartsLen, h);
  scanAnsiNamePart(nameParts, count, FALSE/*createDropAlias*/,
                   FALSE/*acceptCircumflex*/, toInternalIdentifierFlags);
}

ComAnsiNamePart::ComAnsiNamePart( const NAString &externalNameParts
                                 ,size_t &count
                                 ,CollHeap * h
                                 ,NABoolean createDropAlias
                                 ,NABoolean acceptCircumflex
                                 ,unsigned short toInternalIdentifierFlags
                                )
     : internalName_(h),
       externalName_(h),
       toInternalIdentifierFlags_(toInternalIdentifierFlags),
       heap_ (h)
{
  scanAnsiNamePart(externalNameParts, count, createDropAlias, 
                   acceptCircumflex, toInternalIdentifierFlags);
}

// -----------------------------------------------------------------------
// Virtual Destructor
// -----------------------------------------------------------------------

ComAnsiNamePart::~ComAnsiNamePart () {}

// -----------------------------------------------------------------------
// Virtual cast functions
// -----------------------------------------------------------------------

const ComRoutineActionNamePart *
ComAnsiNamePart::castToComRoutineActionNamePart() const
{
  return NULL;
}

ComRoutineActionNamePart *
ComAnsiNamePart::castToComRoutineActionNamePart()
{
  return NULL;
}

// -----------------------------------------------------------------------
// Operators
// -----------------------------------------------------------------------

// assignment operator
//
//   The specified external name on the right-hand side
//   must be a valid ANSI SQL name component, in external
//   format.  If not, this object (on the left-hand side)
//   will be cleared.
//
ComAnsiNamePart &
ComAnsiNamePart::operator = (const NAString &externalName)
{
  const ComAnsiNamePart namePart(externalName, heap_);
  return operator=(namePart);
}

ComAnsiNamePart &
ComAnsiNamePart::operator = (const ComAnsiNamePart &name)
{
  setExternalName(name.getExternalName());
  setInternalName(name.getInternalName());
  toInternalIdentifierFlags_ = name.toInternalIdentifierFlags_;
  return *this;
}

//
// operator ==
//
NABoolean
ComAnsiNamePart::operator == (const ComAnsiNamePart &rhs) const
{
  return (this EQU &rhs OR internalName_ EQU rhs.internalName_);
}

// -----------------------------------------------------------------------
// Accessors
// -----------------------------------------------------------------------

// isDelimitedIdentifier
//   is the ANSI SQL name part a delimited identifier?
//
NABoolean
ComAnsiNamePart::isDelimitedIdentifier() const
{
  return (NOT externalName_.isNull() AND
	  externalName_[(size_t)0] EQU ComSqlText.getDoubleQuote());
}

// -----------------------------------------------------------------------
// private methods
// -----------------------------------------------------------------------

// copyExternalName
//
//   The method returns FALSE if the value in the input ANSI SQL name
//   part, in external format, is illegal; otherwise, this method
//   computes and saves the internal name and recomputes the external
//   (*recomputes* to handle trailing blanks, uppercasing, reserved words, etc).
//
NABoolean
ComAnsiNamePart::copyExternalName(const NAString &externalName)
{
  NAString internalName(externalName, heap_);
  if (ToInternalIdentifier ( internalName
                           , TRUE   // int       upCase           - default is TRUE
                           , FALSE  // NABoolean acceptCircumflex - default is FALSE
                           , toInternalIdentifierFlags_ // unsigned short pv_flags
                           ))
    return FALSE;

  if (castToComRoutineActionNamePart() NEQ NULL AND
      internalName.length() > ComMAX_ROUTINE_ACTION_NAME_INTERNAL_LEN)
    return FALSE;

  internalName_ = internalName;
  externalName_ = ToAnsiIdentifier(internalName_);

  return TRUE;

}

// copyInternalName
//
// Remove trailing white spaces, if any (could be blank-padding
// from an identifier fetched via SQL from our own metadata tables).
// Save the result, and, TRUSTING THAT ITS VALIDITY WAS CHECKED BEFORE
// STORING IN THE METADATA TABLES WHERE WE GOT IT FROM, compute the
// external name.
//
NABoolean
ComAnsiNamePart::copyInternalName(const NAString &internalName)
{
  NAString name(internalName, heap_);
  TrimNAStringSpace(name, FALSE/*leading*/, TRUE/*trailing*/);

  if (castToComRoutineActionNamePart() NEQ NULL AND
      internalName.length() > ComMAX_ROUTINE_ACTION_NAME_INTERNAL_LEN)
    return FALSE;

  internalName_ = name;
  externalName_ = ToAnsiIdentifier(internalName_);

  return TRUE;

}

// scANANSI the Spider...
//
NABoolean
ComAnsiNamePart::scanAnsiNamePart( const NAString &externalNameParts
				  ,size_t &count
                                  ,NABoolean createDropAlias
                                  ,NABoolean acceptCircumflex
                                  ,unsigned short toInternalIdentifierFlags
                                 )
{
  NAString name(externalNameParts, heap_);

  // Note that count is not just an OUT but an IN-OUT parameter!
  // The motivation for this can be seen in ComObjectName::scan()!
  //
  size_t scanTillBadChar = count;

  // Help our callers avoid uninitialized-count-parameter errors...
  #ifndef NDEBUG
    ComASSERT(scanTillBadChar <= 1);
  #endif

  Int32 state = 1;		// initial state nonzero: not within dquotes

  for (count = 0; count < name.length(); count++)
  {
    if (state == 1 && name[count] == '.')
    {
      //For NSK names, system name along with volume name
      //is equivalent to a catalog in ANSI .So, we need to
      //parse until the end of volume name.

      if ( (name.length() > 1) && 
           (count < (name.length() - 1)) &&
           (name[count + 1] != '$') )
       {
          state = 99;		// unquoted dot seen
          break;
       }
    }
    else if (name[count] == '"')
    {
      state = -state;		// dquote seen: toggle state: +1, -1, +, -, ...
    }
  }

  if (state == 99)
    name.remove(count);		// remove from demarking dot to the right
  else if (state < 0)
    return FALSE;		// illegal, unterminated dquote

  Lng32 err = 0L;
  if (createDropAlias)
    err = ToInternalIdentifier(name, FALSE, acceptCircumflex); //do not upshift
  else
    err = ToInternalIdentifier( name
                              , TRUE  // do upshift // int upCase - default is TRUE
                              , acceptCircumflex    // NABoolean acceptCircumflex
                              , toInternalIdentifierFlags // unsigned short pv_flags
                              );
  if (err) {
    if (scanTillBadChar && err == -3127) {
      scanTillBadChar = (size_t)name[(size_t)0];
      name = externalNameParts;
      name.remove(scanTillBadChar);
      err = ToInternalIdentifier ( name
                                 , TRUE             // int upCase - default is TRUE
                                 , acceptCircumflex // NABoolean acceptCircumflex
                                 , toInternalIdentifierFlags // unsigned short pv_flags
                                 );
    }
    if (err) return FALSE;	// other syntax error
    count = scanTillBadChar;
  }

  if (castToComRoutineActionNamePart() NEQ NULL)
  {
    NAWString internalFormatNameInUCS2;
    ComAnsiNameToUCS2 ( name                     // in - const ComString &
                      , internalFormatNameInUCS2 // out - NAWString &
                      );
    if (internalFormatNameInUCS2.length() > ComMAX_ROUTINE_ACTION_1_PART_INTERNAL_UCS2_NAME_LEN_IN_NAWCHARS)
      return FALSE;
  }

  internalName_ = name;
  externalName_ = ToAnsiIdentifier(internalName_);
  return TRUE;

} // ComAnsiNamePart::scanAnsiNamePart()

// -----------------------------------------------------------------------
// definition of function ComDeriveInternalRandomName
// -----------------------------------------------------------------------

//  Generate a name from input simple object name by truncating to 20
//  (or less to keep within the 128-byte size limit) multibyte chars
//  and appending _, 9-byte timestamp, _, and 4 uniqueid digits.
//
ComBoolean ComDeriveRandomInternalName( Lng32 nameCharSet,
                                        const ComString &inputNameInInternalFormat,
                                        ComString &generatedNameInInternalFormat,
                                        NAHeap *h )
{
  if (inputNameInInternalFormat.length() == 0)
  {
    // make up a name in this format _random_name_nnnnnnnnn_nnnn

    char *tmpBuffer = new (h) char[50]; // should be big enough
    strcpy(tmpBuffer, "_random_name"/*InInternalFormat*/);
    // Append the 15-byte funny name suffix for uniqueness
    // 12: the length in bytes of _random_name
    generateFunnyName (UID_GENERATED_ANSI_NAME, &tmpBuffer[12]);
    generatedNameInInternalFormat = tmpBuffer;
    NADELETEBASIC (tmpBuffer, h);
    return TRUE;
  }

  enum cnv_charset eCnvCS = convertCharsetEnum((Int32)nameCharSet);

  Int32 inputNameLen = (Int32)inputNameInInternalFormat.length();
  char *tmp_in_bufr = new (h) char[inputNameLen + 50]; // allocate extra space for NULL
                                                       // terminator and _nnnnnnnnn_nnnn
                                                       // suffix to be added later
  strcpy(tmp_in_bufr, inputNameInInternalFormat.data());
  const char *str_to_test = tmp_in_bufr;
  const Int32 max_bytes2cnv = (const Int32)inputNameInInternalFormat.length();
  const char *tmp_out_bufr = new (h) char[max_bytes2cnv * 4 + 10 /* Ensure big enough! */];
  char * p1stUnstranslatedChar = NULL;
  Int32 nameLenInBytes = 0;
  Int32 maxCharsToConvert = 20;

  for (; maxCharsToConvert > 0; maxCharsToConvert--)
  {
    Int32 cnvErrStatus = LocaleToUTF16(
                        cnv_version1          // in  - const enum cnv_version version
                      , str_to_test           // in  - const char *in_bufr
                      , max_bytes2cnv         // in  - const int in_len
                      , tmp_out_bufr          // out - const char *out_bufr
                      , max_bytes2cnv * 4     // in  - const int out_len
                      , eCnvCS                // in  - enum cnv_charset charset
                      , p1stUnstranslatedChar // out - char * & first_untranslated_char
                      , NULL                  // out - unsigned int *output_data_len_p
                      , 0                     // in  - const int cnv_flags
                      , (Int32)FALSE            // in  - const int addNullAtEnd_flag
                      , NULL                  // out - unsigned int * translated_char_cnt_p
                      , maxCharsToConvert     // in  - unsigned int max_chars_to_convert
                      );
    // do not need to check cnvErrStatus for errors
    // we check nameLenInBytes and maxCharsToConvert instead

    // compute the length in bytes of the truncated 20 (or less) multibyte
    // character input internal format name
    nameLenInBytes = p1stUnstranslatedChar - str_to_test;
    // 15: the length in bytes of the _nnnnnnnnn_nnnn suffix
    if (nameLenInBytes + 15 <= ComMAX_ANSI_IDENTIFIER_INTERNAL_LEN)
      break;
  }

  if (nameLenInBytes == 0 || maxCharsToConvert == 0) // check for errors
  {
    // either the first multibyte character in the input name is illegal
    // or the specified nameCharSet is incorrect

    // make up a name in this format _random_name_nnnnnnnnn_nnnn
    // the tmp_in_bufr should have plenty of room for the generated name

    strcpy(tmp_in_bufr, "_random_name"/*InInternalFormat*/);
    // Append the 15-byte funny name suffix for uniqueness
    // 12: the length in bytes of _random_name
    generateFunnyName (UID_GENERATED_ANSI_NAME, &tmp_in_bufr[12]);
    generatedNameInInternalFormat = tmp_in_bufr;

    NADELETEBASIC (tmp_in_bufr, h);
    NADELETEBASIC (tmp_out_bufr, h);

    return TRUE;
  }

  tmp_in_bufr[nameLenInBytes] = '\0';
  // Append the 15-byte funny name suffix for uniqueness
  generateFunnyName (UID_GENERATED_ANSI_NAME, &tmp_in_bufr[nameLenInBytes]);
  generatedNameInInternalFormat = tmp_in_bufr;

  NADELETEBASIC (tmp_in_bufr, h);
  NADELETEBASIC (tmp_out_bufr, h);

  return TRUE;
}

// A random name generated by method ComDeriveRandomInternalName has
// the format: [someBytes]_[9-byte timestamp]_[4 uniqueid digits]  
//      (total 15 bytes suffix)
//  [someBytes]_ddddddddd_dddd
//             012345678901234
// Check if inputName has that format.
ComBoolean ComIsRandomInternalName(const ComString &inputName)
{
  if (inputName.length() <= 15)
    return FALSE;

  const char * suffix = &inputName.data()[inputName.length()-15];

  if (NOT ((suffix[0] == '_') && (suffix[10] == '_')))
    return FALSE;

  // check that timestamp and uniqueid bytes are all numbers
  if ((str_atoi(&suffix[1], 9) < 0) ||
      (str_atoi(&suffix[11], 4) < 0))
    return FALSE;

  return TRUE;
}

// // ---------------------------------------------------------------------
// // InternalIdentifierHasDivColNamePrefix() returns TRUE if the internal
// // identifier has the DIVISION_ name prefix; otherwise, returns FALSE.
// // ---------------------------------------------------------------------
// NABoolean InternalIdentifierHasDivColNamePrefix (const char * internalColName) // in
// {
//   return (strncmp(internalColName,
//                   DIVISION_COLUMN_NAME_PREFIX,
//                   DIVISION_COLUMN_NAME_PREFIX_LEN_IN_BYTES) == 0);
// }

