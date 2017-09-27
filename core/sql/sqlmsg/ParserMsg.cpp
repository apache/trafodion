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
//
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ParserMsg.C
 * RCS:          $Id: ParserMsg.cpp,v 1.13 1998/08/25 17:19:09  Exp $
 * Description:  This file contains the shared routines for the parsers in sqlci
 *               and compiler (i.e. parser directory)
 *
 * Created:      12/11/96
 * Modified:     $ $Date: 1998/08/25 17:19:09 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *****************************************************************************
 */

#include <ctype.h>
#include <string.h>
#if !defined(__GNUC__) || __GNUC__ < 3
#include <strstream.h>
#endif
#include "NAWinNT.h"
#include "ComDiags.h"
#include "wstr.h"
#include "nawstring.h"
#include "NLSConversion.h"
#include "ErrorMessage.h"
#include "ParserMsg.h"	// header for StoreSyntaxError, implemented herein
#include "csconvert.h"


// This function takes as an argument a pointer to a ComCondition object.
// This function accesses the global variables: input_str, input_pos in
// order to format a string indicating where in the given SQL statement
// a syntax error occurred.
//
// This routine prints out each format-line of the buffer until the line   
// is reached which contains the error position.  That line is the last    
// line printed.                                                           
//                                                                         
// Then the next line output consists of a sequence of zero or more spaces 
// and then a caret.  This gives the appearance of the caret indicating  
// where in the buffer's text the error has been detected.                 
//                                                                         
// Finally, if there is even one more format-line in the buffer then       
// a line is output saying "further output omitted," rather than actually       
// continually printing the lines.                                         
//                                                                         
// A format-line is determined as follows.  It is a line ending at a newline
// character.  It is a line ending at the end of the buffer (that would    
// be the last format-line of the buffer).  It is a line that ends after   
// running for some number of characters, MAX_FORMAT_LINE.  Therefore, 
// in effect, you can see that the final rule defining a format-line       
// is due to the line-wrapping effect on a terminal or printer, and essentially
// inserts artificial linebreaks into the buffer.  If the user doesn't     
// like this, then the work around is to supply input that has newlines to 
// the user's liking in the first place.                                   
//                                                                         
// Implementation Tricks:                                                    
// In the case that the input_pos was at the end of                   
// the buffer, we slide it back by a single character. This should have no 
// ill-effects on the user detecting where the problem is, and it eliminates
// us having to specially handle this case beyond what we are doing here.
//                                                                         
// The rest of the routine is fairly straightforward --- particularly if you
// read above about the purpose of this function.                          


void StoreSyntaxError(const char *input_str, Int32 input_pos,
                      ComDiagsArea& diags, Int32 dgStrNum, 
                      CharInfo::CharSet input_str_cs,
                      CharInfo::CharSet terminal_cs)
{
  if (!input_str) return;

  NAWchar wCharBuffer[ErrorMessage::MSG_BUF_SIZE+1]; // extra space for NULL
#pragma nowarn(1506)   // warning elimination 
  Int32 slen = strlen(input_str);
#pragma warn(1506)  // warning elimination 

  Int32 input_pos_in_numOfNAWchars = input_pos;
  if (input_str_cs != CharInfo::ISO88591)
  {
    // Compute to get the input position in number of NAWcharacters
    enum cnv_charset eCnvCS = convertCharsetEnum((Int32)input_str_cs);
    char * p1stUnstranslatedChar = NULL;
    UInt32 outputOctetLen = 0;
    UInt32 translatedCharCount = 0;
    Int32 cnvErrStatus = LocaleToUTF16(
        cnv_version1              // in  - const enum cnv_version version
      , input_str                 // in  - const char *in_bufr
      , input_pos                 // in  - const int input_str_octet_len
      , (const char *)wCharBuffer // out - const char *out_bufr
      , ErrorMessage::MSG_BUF_SIZE * sizeof(NAWchar) // no NULL terminator
                                  // in  - const int out_bufr_max_octet_len
      , eCnvCS                    // in  - enum cnv_charset charset
      , p1stUnstranslatedChar     // out - char * & first_untranslated_char
      , &outputOctetLen           // out - unsigned int *output_data_len_p
      , 0                         // in  - const int cnv_flags
      , (const Int32)FALSE          // in  - const int addNullAtEnd_flag
      , &translatedCharCount      // out - unsigned int * translated_char_cnt_p
      );
    if (outputOctetLen > 0)
      input_pos_in_numOfNAWchars = (Int32)((outputOctetLen - 1) / sizeof(NAWchar));
  }

  Int32 ulen = LocaleStringToUnicode(input_str_cs, input_str, slen,
                        wCharBuffer, ErrorMessage::MSG_BUF_SIZE+1, TRUE);

  if ( ulen == 0 || ( input_str_cs == CharInfo::ISO88591 && ulen < slen ) )
    cerr << input_str << endl;
  else 
    StoreSyntaxError(wCharBuffer, input_pos_in_numOfNAWchars, diags, dgStrNum, terminal_cs);

} // StoreSyntaxError, single-byte version

static const size_t MAX_FORMAT_LINE = 79;
static const size_t MAX_DGSTRING_SIZE =
		      MINOF(ErrorMessage::MSG_BUF_SIZE,1024) - MAX_FORMAT_LINE;

#pragma nowarn(1506)  // warning elimination
void StoreSyntaxError(const NAWchar *input_str, Int32 input_pos, 
                      ComDiagsArea& diags, Int32 dgStrNum, 
	              CharInfo::CharSet charset)
{
  if (!input_str) return;

  NAWString errMsg;
  NABoolean internalError = FALSE;
  
  Int32 buffLen = na_wcslen(input_str);
  if (buffLen == 0) {
    errMsg.append(WIDE_("\n;\n^"));
  }
  else {
    if (input_pos > buffLen) {
      if (input_pos > buffLen+1) {
        cerr << "*** StoreSyntaxError: (input_pos > buffLen): "
             << input_pos << " " << buffLen;
        internalError = TRUE;
      }
      input_pos = buffLen;
    }
    else if (input_pos < 0)
      input_pos = 0;

    // in case there was any text prior on the current line
    errMsg.append(WIDE_("\n"));			// Start errMsg with a '\n'
    const NAWchar *errPos = input_str+input_pos;
    if (input_pos == buffLen)
      errPos--;
  
    const NAWchar *right = input_str;
    const NAWchar *left = NULL;
  
    while (errPos >= right) {
      left = right;
      size_t charCount = 0;
      while ( 1 ) {
        charCount++;
        right++;
        if (*right == '\n' ||
            *right == 0    ||
            charCount == MAX_FORMAT_LINE)
          break;
      }
      
      // At this point, *right is a null char, a newline char, or
      // the char at (MAX_FORMAT_LINE+1).  We want to print the chars
      // from left up to but not include right.  That's very easy to write
      // as a loop!  Then, we want to output a newline char.  Finally,
      // if (*right == newline char) then increment right.
      //             
      // We are careful not to touch (left) in here because we may need
      // it later in order to determine the number of chars to space over
      // from the start of the final line to where the caret belongeth.
      
      // 3/20/98:	Replaced this codeblock:
      //	const char *currency = left;
      //	while (currency != right) 
      //	  errMsg << *currency++;
      // with this:
      errMsg.append(left, right - left);
      errMsg.append(WIDE_("\n"));

      if (*right == '\n')
        right++;
    }
    // The number given by (errPos - left) tells us exactly how many 
    // characters we must space over before printing the caret.         
    // We print the spaces, the caret, and then a newline.    
    {

      size_t charCount = errPos - left;
      assert(charCount < MAX_FORMAT_LINE);

      // 3/20/98: Since the WCHAR version of the SQl text can contain
      // half or full width characters, we need to get the exact length
      // of the last WCHAR string segment in current locale, then pump that 
      // many of WCHAR space characters into the buffer. 
      // Assumption: the fonts used are fixed width!
      //    Thus, replaced this codeblock:
      //	while (charCount-- != 0)
      //	  errMsg << ' ';
      //    with this:

      char bufferInLocale[(MAX_FORMAT_LINE+1)*8];  // allow for utf8 characters
      Int32 byteCount = UnicodeStringToLocale(charset, (NAWchar*)left, charCount,
                                            bufferInLocale, (MAX_FORMAT_LINE+1)*8
                                           );
 

      for (Int32 i=0; i<byteCount; i++)
        errMsg.append(WIDE_(" "));

      errMsg.append(WIDE_("^ ("));
	  char bytestr_str[10];
// get the size of the error offset as a string input_pos is 0 based
      str_itoa( input_pos+1, bytestr_str);
      NAWString  wBytestr_str(CharInfo::ISO88591,bytestr_str);
   	  errMsg.append(wBytestr_str);		  
	  errMsg.append(L" characters from start of SQL statement)");
    }
    
    // Don't do this; it looks crappy:
    //if (right-input_str != buffLen)
    //  errMsg << endl << "...";  // to indicate rest of SQL stmt not shown
  }

  NAWchar * const entireBuf = (NAWchar *)errMsg.data();
  NAWchar *diagBuf = entireBuf;

  // If this is too big, truncate to fit.
  // Align to next newline if at least two complete lines will still appear.
  //
  size_t len = errMsg.length();
  if (len > MAX_DGSTRING_SIZE)
    {
      diagBuf += len - MAX_DGSTRING_SIZE;
      NAWchar *newline = na_wcschr(diagBuf, L'\n');
      if (newline  && na_wcschr(newline, L'\n'))
        diagBuf = newline;			// Start errMsg with a '\n'
      else
	diagBuf[0] = L'\n';			// Start errmsg with a '\n'
      diagBuf[1] = L'.';			// Interpolate an ellipsis
      diagBuf[2] = L'.';
      diagBuf[3] = L'.';
    }

  // convert tabs (but not newlines) to spaces so the caret aligns correctly
  for (NAWchar *d = diagBuf; *d; d++)
    if (*d == NAWchar('\t'))
      *d = ' ';

  if (dgStrNum == 0)
    diags << DgWString0(diagBuf);
  else
    diags << DgWString1(diagBuf);

  if (internalError) {

    char bufferInLocale[MAX_DGSTRING_SIZE];
    Lng32 len = UnicodeStringToLocale(charset, entireBuf, na_wcslen(entireBuf), 
                                     bufferInLocale, MAX_DGSTRING_SIZE
                                    );

    if (len>0) {
      cerr << bufferInLocale << endl;
    }
  }
  
} // StoreSyntaxError, Unicode version 
#pragma warn(1506)  // warning elimination

