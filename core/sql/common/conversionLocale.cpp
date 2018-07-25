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
 * File:         conversionLocale.h
 * RCS:          $Id: 
 * Description:  The implementation of locale related conversion routins
 *               
 *               
 * Created:      7/8/98
 * Modified:     $ $Date: 1998/08/10 16:00:39 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "ComOperators.h"
#include "ComASSERT.h"
#include "NLSConversion.h"
#include "charinfo.h"
#include "csconvert.h"
cnv_charset convertCharsetEnum (Int32 inset)
{
 switch(inset){ 
   case CharInfo::ISO88591:   return cnv_ISO88591; break;
   case CharInfo::SJIS:       return cnv_SJIS; break;
   case CharInfo::UNICODE:    return cnv_UTF16; break;
   case CharInfo::EUCJP:      return cnv_EUCJP; break;
   case CharInfo::BIG5:       return cnv_BIG5; break;
   case CharInfo::GB18030:    return cnv_GB18030; break;
   case CharInfo::UTF8:       return cnv_UTF8; break;
   case CharInfo::KSC5601:    return cnv_KSC; break;
   case CharInfo::GB2312:     return cnv_GB2312; break;
   case CharInfo::GBK:        return cnv_GBK;    break;
   default:                   return cnv_UnknownCharSet; break;
   }
}


// Unicode to UTF8 conversion. 
//
// This function takes a Unicode UCS2/UTF16 string and returns its UTF8 equivalent.
// The optional utf8String argument holds the buffer into which the Unicode string
// will be stored. In case the argument is NULL or it is not big enough, 
// the function allocates memory from the heap (if the heap pointer is not NULL), 
// or from the C run-time system heap.
// If the memory allocation fails, the function returns 0. If any illegal 
// characters are encountered, the function also returns 0.
//
charBuf* unicodeToUtf8(const NAWcharBuf& unicodeString, CollHeap *heap,
                       charBuf*& utf8String, NABoolean addNullAtEnd,
                       NABoolean allowInvalidCodePoint)
{
  charBuf* cbufPtr = NULL; // tell unicodeTocset() to allocate a new buffer
  charBuf* res = NULL;
  Int32 errorcode = 0;
  res = unicodeTocset ( unicodeString // in - const NAWcharBuf &
                      , heap          // in - CollHeap *
                      , cbufPtr
                      , (Lng32) SQLCHARSETCODE_UTF8 // in - Lng32 targetCharSet      
                      , errorcode     // out - Int32 &
                      , addNullAtEnd
                      , allowInvalidCodePoint 
                      );
   if ( res == NULL || errorcode != 0) // translation failed
     return NULL;

   charBuf* output = checkSpace(heap, res->getStrLen(), utf8String, addNullAtEnd);

   if ( output == NULL )
   {
     NADELETE(res, charBuf, heap);
     return 0;
   }

   Int32 finalLengthInBytes = res->getStrLen();
   memmove(output->data(), res->data(), finalLengthInBytes);
   output->setStrLen(res->getStrLen());
  
   if ( addNullAtEnd == TRUE )
     output->data()[finalLengthInBytes] = '\0';

   NADELETE(res, charBuf, heap);
   return output;
}

Lng32 UnicodeStringToLocale(Lng32 charset, const NAWchar* wstr, Lng32 wstrLen, 
                           char* buf, Lng32 bufLen, NABoolean addNullAtEnd,
                           NABoolean allowInvalidCodePoint)
{
   charBuf cbuf((unsigned char*)buf, bufLen);
   charBuf* cbufPtr = &cbuf;
   charBuf* res = 0;
   Int32 errorcode = 0;

   switch (charset)
   {
#ifdef IS_MP /* :cnu -- As of 8/30/2011, not used in SQ SQL */
    case CharInfo::KANJI_MP:
      res = unicodeToSjis(
                NAWcharBuf((NAWchar*)wstr, wstrLen), 0, cbufPtr, addNullAtEnd,
                           allowInvalidCodePoint 
                        );
      break;
    case CharInfo::KSC5601_MP:
      res = unicodeToKsc5601(
                NAWcharBuf((NAWchar*)wstr, wstrLen), 0, cbufPtr, addNullAtEnd,
                           allowInvalidCodePoint 
                        );
      break;
#endif
    case CharInfo::ISO88591:
     res = unicodeToISO88591(
                NAWcharBuf((NAWchar*)wstr, wstrLen), 0, cbufPtr, addNullAtEnd,
                           allowInvalidCodePoint 
                        );
     break;
 //	 case CharInfo::ISO88591:
	 case CharInfo::EUCJP:
	 case CharInfo::GB18030:
	 case CharInfo::GB2312:
	 case CharInfo::GBK:
	 case CharInfo::KSC5601:
	 case CharInfo::BIG5:
	 case CharInfo::UTF8:
	 case CharInfo::SJIS:
	  res = unicodeTocset(
                NAWcharBuf((NAWchar*)wstr, wstrLen), 0, cbufPtr, charset, errorcode, addNullAtEnd,
                           allowInvalidCodePoint 
                        );
     break;
    default:
     break;
   }

   return (res) ? res->getStrLen() : 0;
}


Lng32 
LocaleStringToUnicode(Lng32 charset, const char* str, Lng32 strLen, 
                      NAWchar* wstrBuf, Lng32 wstrBufLen, NABoolean addNullAtEnd)
{
   // Changed the algorithm to call the new LocaleToUTF16() but keep
   // the old call to old ISO88591ToUnicode() when the character set is
   // ISO88591.  We want to keep the old "pass through" behavior so
   // Use of ISO 8859-15 characters (a.k.a., Latin-9) in
   // CHARACTER SET ISO88591 target column continues to work.

   if (charset == (Lng32) CharInfo::ISO88591)
   {
     NAWcharBuf wcbuf(wstrBuf, wstrBufLen);
     NAWcharBuf* wcbufPtr = &wcbuf;
     NAWcharBuf* res = 0;
     res = ISO88591ToUnicode(
                charBuf((unsigned char*)str, strLen), 0,
                wcbufPtr, addNullAtEnd
                        );
     return (res) ? res->getStrLen() : 0;
   }

   //
   // else (charset != (Lng32) CharInfo::ISO88591)
   //

   enum cnv_charset convCS = convertCharsetEnum(charset);
   if (convCS == cnv_UnknownCharSet)
     return 0; // nothing we can do; exit the routine

   UInt32 outBufSizeInBytes = wstrBufLen*sizeof(NAWchar);
   char * pFirstUntranslatedChar = NULL;
   UInt32 outputDataLenInBytes = 0;
   UInt32 translatedtCharCount = 0;
   Int32 convStatus =
     LocaleToUTF16(cnv_version1,           // const enum cnv_version version
                   str,                    // const char *in_bufr
                   strLen,                 // const int in_len in # of bytes
                   (const char *)wstrBuf,  // const char *out_bufr
                   (const Int32)outBufSizeInBytes,
                   convCS,       // enum cnv_charset charset -- output charset
                   pFirstUntranslatedChar, // char * & first_untranslated_char
                   &outputDataLenInBytes,  // unsigned int *output_data_len_p
                   0,                      // const int cnv_flags (default is 0)
                   (const Int32)addNullAtEnd,
                   &translatedtCharCount); // unsigned int *translated_char_cnt_p

   UInt32 outLenInW = outputDataLenInBytes/sizeof(NAWchar);
   if (convStatus == 0) // success
     return outLenInW;  // include the NULL terminator if (addNullAtEnd == TRUE)

   // If convStatus != 0, LocaleToUTF16 will not add the NULL terminator
   if (addNullAtEnd && wstrBuf && wstrBufLen > 0)
   {
     if (outLenInW < (UInt32)wstrBufLen)
       wstrBuf[outLenInW] = WIDE_('\0');
     else
     {
       // assume the specified wstrBufLen includes room for the NULL terminator
       // when the passed-in addNullAtEnd parameter is set to TRUE
       wstrBuf[wstrBufLen-1] = WIDE_('\0');
     }
   }
   return 0; // tell the caller not to use data in wstrBuf
}

  // *************************************************************** 
  // * Convert the string to UTF8
  // *************************************************************** 
#if 0 /* As of 8/30/2011, there are no callers in SQ SQL */
  Int32 localeConvertToUTF8(char* source,
                    Lng32 sourceLen,
                    char* target,
                    Lng32 targetLen,
                    Lng32 charset,
                    CollHeap * heap,
                    Int32  *rtnCharCount,
                    Int32  *errorByteOff)
  {
 
     char * buffer = NULL;
     Lng32 bufferLen = 0;
     Int32 retCode = 1;
     char *OrigSource = source;
 
     // If the input string is invalid, simply return
     if (source == NULL ||
         strlen(source) == 0)
     {
       return CNV_ERR_NOINPUT;
     }
	
     // If the input charset is UTF8, simply return.
     if (charset == cnv_UTF8)
     {
        return CNV_ERR_NO_CONVERSION_NEEDED;
     }

     // The resulting string will be bigger by a given factor.
     Int32 multiplier = 8;  // Includes future charset multiplier sizes
     bufferLen = sourceLen + ((Lng32)strlen(source) * multiplier) + 1;

     // Check that the target buffer was provided and its length
     // will be able to hold the converted characters

     if (target == NULL ||
         targetLen < bufferLen)
     {
       return CNV_ERR_TARGET_SIZE_INVALID;
     }

     target[0] = '\0';

     buffer = new char [bufferLen];
     memset(buffer, '\0', sizeof(buffer));

     char * pinstr = source; // Pointer to the input buffer.
     char * p1stUnstranslatedChar = NULL;
     UInt32  utf8StrLenInBytes = 0;  // 64-bit
     UInt32  charCount = 0;  // number of characters translated/converted
     Lng32 inStrLen = (Lng32)strlen(source);
 
     for (Lng32 loopCounter = 0; retCode != 0 &&
           (retCode != CNV_ERR_INVALID_CHAR ||
            retCode != CNV_ERR_INVALID_CS    || 
            retCode != CNV_ERR_NOINPUT) &&
            loopCounter < 16;  // avoid infinite loop
          loopCounter++)
     {
	 retCode = LocaleToUTF8(cnv_version1,
   			pinstr, 
			inStrLen,
			(const char*)buffer, 
			bufferLen,
			(cnv_charset)charset,
			(char* &)p1stUnstranslatedChar, 
			&utf8StrLenInBytes,  // 64-bit
			(const Int32)1, // addNullAtEnd_flag == TRUE
			&charCount  // 64-bit
			);
        if (rtnCharCount) *rtnCharCount = (Int32)charCount;
        if (errorByteOff) *errorByteOff = p1stUnstranslatedChar - OrigSource;

	switch(retCode)
	{
  	  case 0:
            if (strlen(target) + strlen(buffer) >= (size_t)targetLen) // avoid overflow
            {
              retCode = CNV_ERR_TARGET_SIZE_INVALID;
              // no need to be fancy - this condition is not supposed to happen anyway
              // just chop off the extra bytes - last character in target may be chopped
              // right in the middle
              if ((size_t)targetLen > strlen(target))
              {
                buffer[targetLen - strlen(target) - 1] = '\0';
                if (target[0] == '\0')
                  strcpy(target, buffer);
                else
                  strcat(target, buffer);
              }
              loopCounter = 8888;    // exit loop
            }
            else // have enough room
            {
            if (target[0] == '\0')
	      strcpy(target,buffer); // We're assuming that the input buffer was null terminated.
            else
              strcat(target,buffer);
            loopCounter = 8888;      // exit loop
            }
	    break;

	  case CNV_ERR_BUFFER_OVERRUN:
          if (strlen(target) + strlen(buffer) < (size_t)targetLen) // have enough room
          {
            if (target[0] == '\0')
            {
	      strncpy(target, buffer, strlen(buffer));
              strcat(target,"?");      // Force a question mark as output
            }
            else
            {
              strncat(target, buffer, strlen(buffer));
              strcat(target,"?");      // Force a question mark as output
            }

	    pinstr = p1stUnstranslatedChar; // We're going again, adjust the pointer to the input buffer.
	    // prepare local variables for the next conversion
            inStrLen = (Lng32)(inStrLen - (p1stUnstranslatedChar - pinstr));
            p1stUnstranslatedChar = NULL;
            // intentionally keep the retCode == CNV_ERR_BUFFER_OVERRUN setting
            // just in case we exceed the loop count limit

            // go back to the beginning of the for loop
            }
            else // avoid overflow
            {
              retCode = CNV_ERR_TARGET_SIZE_INVALID;
              // no need to be fancy - this condition is not supposed to happen anyway
              // just chop off the extra bytes - last character in target may be chopped
              // right in the middle
              if ((size_t)targetLen > strlen(target))
              {
                buffer[targetLen - strlen(target) - 1] = '\0';
                if (target[0] == '\0')
                  strcpy(target, buffer);
                else
                  strcat(target, buffer);
              }
              loopCounter = 8888;    // exit loop
            }

            break;

	  case CNV_ERR_INVALID_CHAR:
            retCode = CNV_ERR_INVALID_CHAR;
            break;
	  case CNV_ERR_INVALID_CS:
            retCode = CNV_ERR_INVALID_CS;
            break;
	  case CNV_ERR_NOINPUT:
            retCode = CNV_ERR_NOINPUT;
            break;
          default:
            retCode = CNV_ERR_INVALID_CHAR;  // Bad character set conversion
            break;
	}
     } 

        delete[] buffer;
        buffer = NULL;

        return retCode;
  }
#endif /* As of 8/30/2011, no callers in SQ SQL */

  // *************************************************************** 
  // * Encode the string from a UTF8 multibyte string to the
  // * designated charset
  // *************************************************************** 

#if 0 /* As of 8/30/2011, there are no callers in SQ SQL */
  Int32 UTF8ConvertToLocale(char* source,
                          Lng32 sourceLen,
                          char* target,
                          Lng32 targetLen,
                          Lng32 charset,
                          CollHeap *heap,
                          Int32  *charCount,
                          Int32  *errorByteOff)
  {
    Int32 retCode = 1;
    char * OrigSource = source;
 
    // If the input string is invalid, simply return
    if (source == NULL ||
        strlen(source) == 0)
      {
         return CNV_ERR_NOINPUT;
      }
	
    // If the ISO_MAPPING is UTF8, simply return the string since it is already UTF8.
    if (charset == cnv_UTF8)
      {
        return CNV_ERR_NO_CONVERSION_NEEDED;
      }

    Lng32 bufferLen = (Lng32)strlen(source) + 1;
    char * buffer = NULL;

    // Check that the target buffer was provided and its length
    // will be able to hold the converted characters

     if (target == NULL ||
         targetLen < bufferLen)
     {
       return CNV_ERR_TARGET_SIZE_INVALID;
     }

     target[0] = '\0';

    buffer = new char [bufferLen];
    memset(buffer, '\0', sizeof(buffer));

    char* punstr = 0;           // Pointer to the first unconverted character
                                // (either due to small buffer size or
                                // conversion error).

    UInt32 outLen = 0;    // output data length in number of bytes
    UInt32 numTran = 0;   // number of characters translated


    for (Lng32 loopCounter = 0; retCode != 0 &&
          (retCode != CNV_ERR_INVALID_CHAR ||
           retCode != CNV_ERR_INVALID_CS    || 
           retCode != CNV_ERR_NOINPUT) &&
           loopCounter < 16;      // Avoid infinite loop
         loopCounter++)
      {
        retCode = UTF8ToLocale(cnv_version1,
			      (const char*)source, 
			      (Lng32)strlen(source),
			      (const char*)buffer, 
			      bufferLen,
			      (cnv_charset)charset,
			      (char* &)punstr, 
			      &outLen,
			      1, // addNullAtEnd_flag == TRUE
			      0, // allow_invalids == FALSE   
 			      &numTran,  // 64-bit
			      0);
        if (charCount) *charCount = (Int32)numTran;
        if (errorByteOff) *errorByteOff = punstr - OrigSource;

        switch(retCode)
        {
  	  case 0:
            if (strlen(target) + strlen(buffer) >= (size_t)targetLen) // avoid overflow
            {
              retCode = CNV_ERR_TARGET_SIZE_INVALID;
              // no need to be fancy - this condition is not supposed to happen anyway
              // just chop off the extra bytes - last character in target may be chopped
              // right in the middle
              buffer[targetLen - strlen(target) - 1] = '\0';
              if (target[0] == '\0')
                strcpy(target, buffer);
              else
                strcat(target, buffer);
              loopCounter = 8888;    // exit loop
            }
            else
            {
            if (target[0] == '\0')
	      strcpy(target,buffer); // We're assuming that the input buffer was null terminated.
            else
              strcat(target,buffer);
            loopCounter = 8888;        // exit loop
            }
            break;

  	  case CNV_ERR_BUFFER_OVERRUN:
          if (strlen(target) + strlen(buffer) < (size_t)targetLen)
          {
  	    if (target[0] == '\0')
  	      strncpy(target, buffer, strlen(buffer));
  	    else
  	      strncat(target, buffer, strlen(buffer));
  	    source = punstr; // We're going again, adjust the pointer to the input buffer.
  	    // intentionally keep the retCode == CNV_ERR_BUFFER_OVERRUN setting
  	    // just in case we exceed the loop count limit

            // go back to the beginning of the for loop
            }
            else // avoid overflow
            {
              retCode = CNV_ERR_TARGET_SIZE_INVALID;
              // no need to be fancy - this condition is not supposed to happen anyway
              // just chop off the extra bytes - last character in target may be chopped
              // right in the middle
              buffer[targetLen - strlen(target) - 1] = '\0';
              if (target[0] == '\0')
                strcpy(target, buffer);
              else
                strcat(target, buffer);
              loopCounter = 8888;    // exit loop
            }

  	    break;
 
  	  case CNV_ERR_INVALID_CHAR:
  	    retCode = CNV_ERR_INVALID_CHAR;
  	    break;
  	  case CNV_ERR_INVALID_CS:
  	    retCode = CNV_ERR_INVALID_CS;
  	    break;
  	  case CNV_ERR_NOINPUT:
  	    retCode = CNV_ERR_NOINPUT;
  	    break;
  	  default:
  	    retCode = CNV_ERR_INVALID_CHAR;  // Bad character set conversion
  	    break;
  	  }
	} 

        delete[] buffer;
        buffer = NULL;

	return retCode;
  }
#endif /* As of 8/30/2011, no callers in SQ SQL */

// -----------------------------------------------------------------------
// ComputeWidthInBytesOfMbsForDisplay:
//
// Returns the display width (in bytes) that is the closest to the
// specified maximum display width (in bytes) without chopping the
// rightmost multi-byte characters into two parts so that we do not
// encounter the situation where the first part of the multi-byte
// character is in the current display line and the other part of
// the character is in the next display line.
//
// If encounters an error, return the error code (a negative number)
// define in w:/common/csconvert.h.
//
//
// In parameter pv_eCharSet contains the character set attribute
// of the input string passed in via the parameter pp_cMultiByteStr.
//
// The out parameter pr_iNumOfTranslatedChars contains the number of
// the actual (i.e., UCS4) characters translated.
//
// The out parameter pr_iNumOfNAWchars contains the number of UCS2
// characters (NAwchar[acters]) instead of the number of the actual
// (i.e., UCS4) characters.
//
// Note that classes NAMemory and CollHeap are the same except for
// the names.
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// ComputeStrLenInNAWchars:
//
// Returns the length of the input string (in the specified character set)
// in number of NAWchar(acters) - Note that a UTF16 character (i.e., a
// surrogate pair) will have a count of 2 NAWchar(acters).
//
// Return an error code (a negative number) if encounters an error.  The
// error code values are defined in w:/common/csconvert.h.
// -----------------------------------------------------------------------
Int32 ComputeStrLenInNAWchars (const char * pStr,
                               const Int32 strLenInBytes,
                               const CharInfo::CharSet strCS,
                               NAMemory *workspaceHeap) // in - default is NULL (the C++ runtime heap)
{
  if (pStr == NULL || strLenInBytes == 0)
    return 0;

  if (strCS == CharInfo::UCS2)
    return strLenInBytes / BYTES_PER_NAWCHAR;

  Int32        lenInNAWchars = 0;
  char *       pFirstByteOfUntranslatedChar = NULL;
  UInt32       outputDataLen = 0;
  Int32        rtnCode = 0;

  cnv_charset  cnvCharSet = convertCharsetEnum(strCS);

  // Compute the size of the to-be-allocated output buffer, include a UCS-2 NULL terminator, for the worst case.
  const Int32  bufSizeInBytes = (BYTES_PER_NAWCHAR+1) * strLenInBytes + BYTES_PER_NAWCHAR;
  char *       charBuf = new (workspaceHeap) char [bufSizeInBytes];

  if (charBuf EQU NULL)
    return CNV_ERR_INVALID_HEAP;

  rtnCode =
    LocaleToUTF16 ( cnv_version1                    // in  - const enum cnv_version
                  , pStr                            // in  - const char *   in_buf
                  , strLenInBytes                   // in  - const int      in_len
                  , charBuf                         // out - const char *   out_buf - plenty of room
                  , bufSizeInBytes                  // in  - const int      out_len - buffer size in bytes
                  , cnvCharSet                      // in  - const int      cnv_charset
                  , pFirstByteOfUntranslatedChar    // out - char *       & ptr_to_first_untranslated_char
                  , & outputDataLen                 // out - unsigned int * output_data_len_p     = NULL
               // , 0                               // in  - const int      cnv_flags             = 0
               // , (Int32)FALSE                    // in  - const int      addNullAtEnd_flag     = FALSE
               // , & translatedCharCount           // out - unsigned int * translated_char_cnt_p = NULL
               // ,                                 // in  - unsigned int   max_chars_to_convert  = 0xffffffff
                  );
  lenInNAWchars = outputDataLen / BYTES_PER_NAWCHAR;
  NADELETEBASIC(charBuf, workspaceHeap);

  if (rtnCode == 0)
    return lenInNAWchars; // a positive integer value
  else
    return rtnCode;       // a negative integer value

  return lenInNAWchars;
} // ComputeStrLenInNAWchars()

// -----------------------------------------------------------------------
// ComputeStrLenInUCS4chars:
//
// Returns the actual (i.e., UCS4) character count of the input string
// (in the specified character set) in the actual (i.e., UCS4) characters.
// Return an error code (a negative number) if encounters an error.  The
// error code values are defined in w:/common/csconvert.h.  Note that
// this function does not need to use a workspace heap.
// -----------------------------------------------------------------------
Int32 ComputeStrLenInUCS4chars (const char * pStr,
                                const Int32 strLenInBytes,
                                const CharInfo::CharSet cs)
{
  if (cs == CharInfo::ISO88591 || strLenInBytes == 0)
    return strLenInBytes;

  Int32 numberOfUCS4chars = 0;
  Int32 firstCharLenInBuf = 0;
  UInt32 /*ucs4_t*/ UCS4value;
  cnv_charset cnvCharSet = convertCharsetEnum(cs);
  const char *s = pStr;
  Int32 num_trailing_zeros = 0;
  Int32 len = (Int32)strLenInBytes;

  while (len > 0)
  {
    firstCharLenInBuf = LocaleCharToUCS4 (s, len, &UCS4value, cnvCharSet);

    if (firstCharLenInBuf <= 0)
      return CNV_ERR_INVALID_CHAR;

    numberOfUCS4chars++;
    if ( *s == '\0' )
       num_trailing_zeros += 1;
    else 
       num_trailing_zeros = 0;
    s += firstCharLenInBuf;
    len -= firstCharLenInBuf;
  }

  return numberOfUCS4chars - num_trailing_zeros ; //NOTE: Don't count trailing zeros !

} // ComputeStrLenInUCS4chars ()

