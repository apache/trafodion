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
 * File:         str.cpp
 * Description:
 *
 * Language:     C++
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "Platform.h"
#include "NABoolean.h"
#include "str.h"
#include "NAStdlib.h"
#include "NAAssert.h"
#include "BaseTypes.h"
#include "Int64.h"
#include "NAString.h"

#include <stdarg.h>

#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/evp.h>

#include "ComResWords.h"

/*
 ******************************************************************
 * Helper functions for ISO 8859_1 (8-bit european) alphabet processing
 *
 */

Int32 isUpper8859_1(NAWchar c)
{
  if ((c >= 'A') && (c <= 'Z'))
    return TRUE;

  if ((c >= 0xc0) && (c <= 0xde))  // between cap A with grave accent
  {                                // and cap icelandic letter thorn
    if (c == 0xd7)     // but not multiplication symbol
       return FALSE;
    else
       return TRUE;
  }

  return FALSE;
}


Int32 isLower8859_1(NAWchar c)
{
  if ((c >= 'a') && (c <= 'z'))
    return TRUE;

  if ((c >= 0xdf) && (c <= 0xff))  // between lower german sharp S
  {                                // and lower y with diaeresis
    if (c == 0xf7)     // but not division symbol
       return FALSE;
    else
       return TRUE;
  }

  return FALSE;
}


Int32 isAlpha8859_1(NAWchar c)
{
  if (((c >= 'a') && (c <= 'z')) ||
      ((c >= 'A') && (c <= 'Z')))   //North american english alphabetic
  {
    return TRUE;
  }

  if ((c >= 0xc0) && (c <= 0xff))   // possible european letter
  {
    if ((c == 0xd7) || (c == 0xf7))  // multiple or divide sign
      return FALSE;
    else
      return TRUE;
  }

  return FALSE;
}

Int32 isHexDigit8859_1(NAWchar c)
{
   return (isDigit8859_1(c) || ('A' <= c AND c <= 'F' ) || ( 'a' <= c AND c <= 'f'));
}

Int32 isAlNum8859_1(NAWchar c)
{
  return (isAlpha8859_1(c) || isDigit8859_1(c));
}

Int32 isSpace8859_1(NAWchar c)
{
  if (((c >= 0x09) && (c <= 0x0d))  ||
      (c == 0x20))
    return TRUE;

  return FALSE;
}

Int32 isDigit8859_1(NAWchar c) // ISO 8859-1 char set safe isdigit routine
{
  if ((c >= '0') && (c <= '9'))
    return TRUE;
  return FALSE;
}

Int32 isCaseInsensitive8859_1(NAWchar c) // ISO 8859-1 char for which there is no
                               // upcase equivalent.  hex values 0xDF & 0xFF
{
  if ((c==0xDF) || (c==0xFF))
    return TRUE;

  return FALSE;
}



// Dummy routine to ensure that str_cpy_all gets inlined.  Once 
// the compiler is fixed to inline routines with calls to assert, 
// Remove callAssert() in callers and replace with direct call to 
// assert.
void callAssert(const char* tgt, const char* src, Lng32 length) {
  assert((tgt && src) || !length);
}

Int32 str_cmp_ne(const char *left, const char *right)
{
  if (!left) return right ? -3 : 0;	// -3 = not equal, 0 = eq (both NULL)
  if (!right) return +3;		// +3 = not equal
  Int32 len1 = str_len(left);
  Int32 len2 = str_len(right);
  if (len1 != len2) return 2;		// 2 = not equal
  return str_cmp(left, right, len1);	// 0 = equal, neg/pos = not equal
}

Int32 str_cpy(char *tgt, const char *src, Int32 tgtlen, char padchar)
{
  assert((tgt && src) || !tgtlen);

  Int32 copy_len = 0;
  while ((copy_len < tgtlen) && (src[copy_len] != 0))
    copy_len++;

  str_cpy_all(tgt, src, copy_len);

  if (tgtlen > copy_len)
    str_pad(&tgt[copy_len],
	    (tgtlen - copy_len),
	    padchar);

  return 0;
}

Int32
byte_str_cpy(char *tgt, Int32 tgtlen, const char *src, Int32 srclen, char padchar)
{
  assert((tgt && src) || !tgtlen);

  Int32 copy_len;
  if ( srclen < tgtlen )
    copy_len = srclen;
  else
    copy_len = tgtlen;

  str_cpy_all(tgt, src, copy_len);

  if (tgtlen > copy_len)
    str_pad(&tgt[copy_len],
	    (tgtlen - copy_len),
	    padchar);

  return 0;
}

Int32 str_cat(const char *first, const char *second, char *result)
{
  assert(first && second && result);

  Int32 firstlen = str_len(first);
  Int32 secondlen = str_len(second);

  if(result != first)
    str_cpy_all(result,first,firstlen);
  str_cpy_all(&result[firstlen],second,secondlen);
  result[firstlen+secondlen] = 0;
  return 0;
}

char *str_itoa(ULng32 i, char *outstr)
{
  assert(outstr);

  if (i == 0)
    {
      outstr[0] = '0';
      outstr[1] = 0;
    }
  else
    {
      short j = 0;
      ULng32 temp = i;

      // check how many digits there are in the output string
      while (temp > 0)
	{
	  temp = temp / 10;
	  j++;
	}

      // set the NULL byte at the end of the string
      outstr[j--] = 0;

      // produce the ASCII digits right to left
      temp = i;
      while (temp > 0)
	{
	  outstr[j--] = '0' + (char) (temp%10);
	  temp = temp / 10;
	}
    }

  return outstr;
}

char *str_ltoa(Int64 i, char *outstr)
{
  assert(outstr);

  Int64 ii = i;
  NABoolean neg = FALSE;
  if (i < 0)
    {
      ii = -i;
      neg = TRUE;
    }

  if (ii == 0)
    {
      outstr[0] = '0';
      outstr[1] = 0;
    }
  else
    {
      short j = 0;
      Int64 temp = ii;

      // check how many digits there are in the output string
      while (temp > 0)
	{
	  temp = temp / 10;
	  j++;
	}

      if (neg)
	j++;

      // set the NULL byte at the end of the string
      outstr[j--] = 0;

      // produce the ASCII digits right to left
      temp = ii;
      while (temp > 0)
	{
	  outstr[j--] = '0' + (char) (temp%10);
	  temp = temp / 10;
	}
      if (neg)
	outstr[0] = '-';
    }

  return outstr;
}

Int64 str_atoi(const char * instr, Lng32 instrLen)
{
  assert(instr);

  Int64 v = 0;

  Int32 i = 0;
  // skip leading blanks
  while ((i < instrLen) && (instr[i] == ' '))
    i++;

  if (i == instrLen)
    return -1;

  //  for (Int32 i = 0; i < instrLen; i++)
  while (i < instrLen)
    {
      if ((instr[i] >= '0') &&
	  (instr[i] <= '9'))
	{
	  v = v*10 + (instr[i] - '0');
	}
      else if (instr[i] == ' ')
	{
	  // skip trailing blanks
	  while ((i < instrLen) && (instr[i] == ' '))
	    i++;

	  // error, not trailing blanks
	  if (i < instrLen)
	    return -1;
	}
      else
	{
	  // error
	  return -1;
	}
      i++;
    }

  return v;
}

// convert a scaled exact numeric string and return as float.
double str_ftoi(const char * instr, Lng32 instrLen)
{
  assert(instr);

  double v = 0;

  Int32 i = 0;
 
  // look for decimal point
  while ((i < instrLen) && (instr[i] != '.'))
    i++;

  if (i == instrLen)
    {
      // not a scaled number.
      v = (double)str_atoi(instr, instrLen);
    }
  else
    {
      // found decimal point at 'i'
      
      // extract the mantissa
      Int64 m = 0;
      if (i > 0)
	{
	  m = str_atoi(instr, i);
	  if (m < 0)
	    return -1;
	}

      // extract the fraction
      Int64 f;
      Lng32 scaleLen = instrLen - (i + 1);
      f = str_atoi(&instr[i+1], scaleLen);
      if (f < 0)
	return -1;

      v = (double)m;
      Int64 tf = f;
      Int64 tens = 10;
      while (tf > 0)
	{
	  tf = tf / 10;
	  tens = tens * 10;
	}
      v = (v*tens + f) / tens;
    }

  return v;

}

Int32 mem_cpy_all(void *tgt, const void *src, Lng32 length)
{
  memmove(tgt, src, length);
  return 0;
}

void str_memmove(char *tgt, const char *src, Lng32 length)
{
  assert((tgt && src) || !length);
  memmove(tgt, src, length);
}



// copies src to tgt for length bytes.
// Removes trailing blanks and puts the end_char.
Int32 str_cpy_and_null(char * tgt,
		       const char * src,
		       Lng32 length,
		       char end_char,
		       char blank_char,
		       NABoolean nullTerminate)
{
assert((tgt && src) || !length);

  Lng32 i = 0;
  for (; i < length; i++)
    {
      tgt[i] = src[i];
    }

  i = length-1;
  while ((i > 0) && (tgt[i] == blank_char))
    i--;

  if (i < length-1)
    tgt[i+1] = end_char;
  else if (nullTerminate)
    tgt[i+1] = end_char;

  return 0;
}

// ---------------------------------------------------------------
// copies src to tgt for length bytes and upshifts, if upshift <> 0,
// else downshifts.
// Src and Tgt may point to the same location.
// ---------------------------------------------------------------
Int32 str_cpy_convert(char * tgt, char * src,
		    Lng32 length, Int32 upshift)
{
  assert((tgt && src) || !length);

   for (Lng32 i = 0; i < length; i++)
    {
      if (upshift)
	tgt[i] = TOUPPER(src[i]);

      if (!upshift)
	tgt[i] = TOLOWER(src[i]);
    }

   return 0;
}

Int32 str_len(const char * s)
{
  Int32 i = 0;

  while (s[i] != 0) i++;

  return i;
}


Int32 str_inc(const ULng32 length, char * s)
{
  unsigned char * s_ = (unsigned char *)s;
  ULng32 i;
  Int32 carry = 1;
  for (i = length; i > 0 && carry; i--)
    {
      if (s_[i-1] == 255)
        {
          s_[i-1] = 0;
        }
      else
        {
          s_[i-1]++;
          carry = 0;
        }
    }
  //  If final carry is not zero, report failure.
  if (carry)
    {
      return 1;
    }
  return 0;
}

void str_complement(const ULng32 length, char * s)
{
  for (ULng32 i = 0; i < length; i++)
    s[i] = ~(s[i]);
}

// ----------------------------------------------------------------------
// How many bytes are needed to encode byteLen bytes in ASCII?
// -----------------------------------------------------------------------
Lng32 str_encoded_len(Lng32 byteLen)
{
  // As mentioned below, we always make groups of 4 characters for
  // 3 input bytes and add extra bytes as needed for odd lots

  Lng32 threes = byteLen / 3;

  switch (byteLen % 3)
    {
    case 0:
      // encoding is threes*4 groups of 4 chars with 3 bytes in them
      return threes*4;
    case 1:
      // with one extra byte add two more characters (containing 6+2 bits)
      return threes*4+2;
    case 2:
      // with two extra bytes add three more characters (6+6+4 bits)
      return threes*4+3;
    }
  assert(0);
  return 0; // should be hard to get here but compiler doesn't know that
}

// -----------------------------------------------------------------------
// encode the source buffer (may contain embedded NULLs, not NULL
// terminated) into printable ASCII characters and return the length
// of the encoded string
// -----------------------------------------------------------------------
Lng32 str_encode(char *tgt, Lng32 tgtMaxLen, void *src, Lng32 srcLen)
{
  // We expand every 6 bits to 8 bits.  Bias the 8-bit value by 32
  // (ASCII blank) to turn it into a printable char value.  This in
  // effect converts every 3 bytes to 4 bytes. This routine was
  // formerly called CatRWAccessPath::explodeKey().

  // start character for encoding (a range of 64 chars is used)
  // NOTE: this char is also defined in str_decode below!!!
  const char minChar = '!';

  const unsigned char * key_in = (const unsigned char *)src;
  unsigned char * key_out = (unsigned char *) tgt;
  Lng32 length = str_encoded_len(srcLen);

  assert(tgtMaxLen >= length);

  Lng32 srcix = 0;
  Lng32 tgtix = 0;

  // move in groups of 3 source bytes and 4 target characters
  while (srcix < srcLen)
    {
      // high-order 6 bits of input byte 0 go into output char 0
      key_out[tgtix]   = (key_in[srcix] >> 2) + minChar;
      // low-order 2 bits of input byte 0 go into output char 1
      key_out[tgtix+1] = ((key_in[srcix] & 0x3) << 4) + minChar;

      if (srcix+1 < srcLen)
	{
	  // add high-order 4 bits of input byte 1 to output char 1
	  key_out[tgtix+1] += key_in[srcix+1] >> 4;
	  // low-order 4 bits of input byte 1 go to output char 2
	  key_out[tgtix+2]  = ((key_in[srcix+1] & 0xf) << 2) + minChar;
	}

      if (srcix+2 < srcLen)
	{
	  // add high-order 2 bits of input byte 2 to output char 2
	  key_out[tgtix+2] += key_in[srcix+2] >> 6;
	  // low-order 6 bits of input byte 2 go to output char 3
	  key_out[tgtix+3]  = (key_in[srcix+2] & 0x3f) + minChar;
	}

      srcix += 3;
      tgtix += 4;
    }
  return length;
}

// -----------------------------------------------------------------------
// compute how many bytes are encoded in an ASCII string of length
// charLen, assuming it was created by str_encode
// -----------------------------------------------------------------------
Lng32 str_decoded_len(Lng32 charLen)
{
  // find out how many groups of 4 chars and how many extra chars
  Lng32 fours = charLen / 4;

  switch (charLen % 4)
    {
    case 0:
      // an even number of four groups, return 3 times as many bytes
      return fours * 3;
    case 1:
      // this length cannot have been generated by str_encoded_len!!
      assert(0);
    case 2:
      // one extra byte in two extra characters
      return fours*3+1;
    case 3:
      // two extra bytes in the three extra characters
      return fours*3+2;
    }
  assert(0);
  return 0;
}

// -----------------------------------------------------------------------
// the inverse of str_encode
// -----------------------------------------------------------------------
Lng32 str_decode(void *tgt, Lng32 tgtMaxLen, const char *src, Lng32 srcLen)
{
  // start character for encoding (a range of 64 chars is used)
  // NOTE: this char is also defined in str_encode above!!!
  const char minChar = '!';

  unsigned char *target = (unsigned char *) tgt;
  unsigned char *src1   = (unsigned char *) src;

  Lng32 length = str_decoded_len(srcLen);

  //  assert(tgtMaxLen >= length);
  if (NOT (tgtMaxLen >= length))
    return -1;

  Lng32 srcix = 0;
  Lng32 tgtix = 0;

  // move in groups of 4 source chars, at this point we have 0, 2, 3, or >3
  // characters left in the source
  while (srcix+1 < srcLen)
    {
      // first byte comes from 6 bits of first char and 2 bits of second char
      if (NOT (src1[srcix]-minChar < 64 && src1[srcix+1]-minChar < 64))
        return -1;

      //      assert(src1[srcix]-minChar < 64 && src1[srcix+1]-minChar < 64);
      target[tgtix]  = (src1[srcix]-minChar) << 2;
      target[tgtix] |= (src1[srcix+1]-minChar) >> 4;

      if (srcix+2 < srcLen)
	{
          // second byte gets 4 bits from second and 4 bits from third char
          if (NOT (src1[srcix+2]-minChar < 64))
            return -1;
          //          assert(src1[srcix+2]-minChar < 64);
          target[tgtix+1]  = ((src1[srcix+1]-minChar) & 0xf) << 4;
	  target[tgtix+1] |= (src1[srcix+2]-minChar) >> 2;
	}

      if (srcix+3 < srcLen)
	{
	  // third byte gets 2 bits from third and 6 bits from fourth char
          if (NOT (src1[srcix+3]-minChar < 64))
            return -1;
          //          assert(src1[srcix+3]-minChar < 64);
          target[tgtix+2]  = ((src1[srcix+2]-minChar) & 0x3) << 6;
	  target[tgtix+2] |= ((src1[srcix+3]-minChar) & 0x3f);
	}

      srcix += 4;
      tgtix += 3;
    }

  return length;
}

Lng32 str_encoded_len_base64(Lng32 len)
{
  return ((4 * len / 3) + 3) & ~3;
}

Lng32 str_decoded_len_base64(Lng32 len)
{
  return (len * 3) / 4;
}

Lng32 str_encode_base64(const unsigned char* in, Lng32 in_len,
                        char *out, Lng32 out_len)
{
  Lng32 ret = 0;

  BIO *b64 = BIO_new(BIO_f_base64());
  BIO *bio = BIO_new(BIO_s_mem());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  BIO_push(b64, bio);

  ret = BIO_write(b64, in, in_len);
  BIO_flush(b64);
  if (ret > 0)
    {
      ret = BIO_read(bio, out, out_len);
    }

  BIO_free(b64);
  return ret;
}

Lng32 str_decode_base64(const unsigned char* in, Lng32 in_len,
                        char *out, Lng32 out_len)
{
  Lng32 ret = 0;

  BIO *b64 = BIO_new(BIO_f_base64());
  BIO *bio = BIO_new(BIO_s_mem());
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  BIO_push(b64, bio);

  ret = BIO_write(bio, in, in_len);
  BIO_flush(bio);
  if (ret)
    {
      ret = BIO_read(b64, out, out_len);
    }

  BIO_free(b64);
  return ret;
}

// Strips leading and/or trailing blanks. src will contain a NULL after the
// end of the first non-blank character.The length of the "stripped" string
// is returned in len.
// Returns pointer to the start of string after leading blanks.
char * str_strip_blanks(char *src , Lng32 &len, 
                        NABoolean stripLeading,
                        NABoolean stripTrailing
                        )
{
  if (! src)
    return NULL;

  len = str_len(src);
  if (len == 0) // empty
    return src;

  if (stripTrailing)
    {
      len--;
      while ((len >= 0) && (src[len] == ' '))
        len--;

      len++;
      src[len] = 0;
    }

  Lng32 i = 0;
  if (stripLeading)
    {
      while ((i < len) && (src[i] == ' '))
        i++;
      len = len - i;
    }

  return &src[i];
}

//------------------------------------------------
//See .h file for explanation on parameters etc
//------------------------------------------------
Lng32 str_to_ansi_id(char *src, char *tgt,Lng32 &tgtLen, short mustValidate, char *allowedChars)
{
  UInt32 i;
  register char *pBuf = src;
  NABoolean dQuoteSeen = FALSE;

  assert(tgt && src) ;
  tgtLen = str_len(src);

  str_cpy_all(tgt,src,str_len(src));

  tgt[tgtLen] = '\0';

  if (tgtLen == 0)
    return -1;

  // Check to see if this is a delimited identifier
  if ((tgt[0] == '"') && (tgt[tgtLen-1] == '"'))
    {
      dQuoteSeen = TRUE;
      // strip the first double quote out
      pBuf = tgt;
      tgt++;
      // strip the ending double quote out
      tgt[tgtLen-2] = '\0';
      tgtLen = tgtLen-2;
    }

  // If it is delimited, make sure it is not a string with just blanks
  NABoolean empty = TRUE;
  if (dQuoteSeen)
    {
      for (i = 0; i < tgtLen;i++)
	{
	  if (isSpace8859_1(tgt[i])) // Convert all tabs to spaces
	    tgt[i] = ' ';
      if (tgt[i] != ' ')
        empty = FALSE;
	}
      if (empty == TRUE)
	return -1;
    }

  if (tgtLen == 0)
    return -1;

  if (tgtLen > 128)
    return -1;

  UInt32 j = 0;

  i = 0;
  for (i = 0; i < tgtLen; i++)
    {
      if (dQuoteSeen)
	{
	  // This is a delimited identifier. Do the foll:
	  // 1.  We have removed the surrounding quotes by now
	  // 2.  Replace any double quote symbols by a single double quote.
	  // 3.  Leave the case of each character untouched

	  if ((tgt[i] == '"') && (tgt[i+1] == '"'))
	    {
	      // a double quote has been seen inside the string
	      // remove the second double quote by shifting all
	      // the chars to the right of it by
	      for (j = i; j < tgtLen; j++)
		tgt[j] = tgt[j+1];
	      tgtLen--;

	    }
	} //if dQuoteSeen
      else
	{
	  // Check if this character is an alpha numeric or
	  // contains of the allowed chars

	  if( NOT isAlNum8859_1((unsigned char)(tgt[i])) && (tgt[i] != '_'))
	    {
	      if (allowedChars)
		{
		  short found = 0;
		  for (UInt32 j = 0; j <str_len(allowedChars); j++)
		    {
		      if (tgt[i] == allowedChars[j])
			found = 1;
		    }
		  // If it is not in the allowed char list then it is invalid
		  if (!found)
		    return -1;
		}
	      else
		return -1;
	    }
	  tgt[i] = TOUPPER(tgt[i]);
	}
    } // end for

  // In case it is not a delimited id then do this additional check
  if ((!dQuoteSeen) && (mustValidate))
    {

      // Check if it is a SQL reserved word
      //      if (ComResWords::isSqlReservedWord(tgt))
      if (IsSqlReservedWord(tgt))
	return -1;
    }

  //Copy everything back to the original pointer
  str_cpy_all(pBuf,tgt,tgtLen);
  tgt = pBuf;
  return 0;
}

// -----------------------------------------------------------------------
// following two functions are used to return the catalog and schema names
// given a qualified table name. Either the catalog or schema name can be
// a delimited identifier name.
// -----------------------------------------------------------------------
Int32 extractDelimitedName (char* tgt,  const char* const src, const char sep)
{
   Int32 i = 0, j = 0;

   assert(tgt);

   // delimited identifier case
   if (src[0] == '\"')
   {
      // look for the first period after an even number of double quotes
      while ((src[i] != '\0') && ((j % 2 != 0) || (src[i] != sep)))
      {
        if (src[i] == '\"')
          j++;
        tgt[i] = src[i];
        i++;
      }
   }
   else // regular identifier case
   {
      while ((src[i] != '\0') && (src[i] != sep))
      {
        tgt[i] = src[i];
        i++;
      }
   }

   tgt[i] = '\0';

   // return the length of the returned buffer
   return (i);
}

void extractCatSchemaNames (char* catName, char *schName, char* qualTabName)
{
   assert(catName && schName && qualTabName);

   char* src = qualTabName;

   // extract the catalog name
   Int32 buffLength = extractDelimitedName (catName, src);

   // advance to the start of the schema name
   src =  src + buffLength + ((src[0]=='\"')?2:1);

   // extract the schema name
   extractDelimitedName (schName, src);
}

/* str_str */
char *(str_str)(const char *s1, const char *s2)
{
    size_t s2len;
    /* Check for the null s2 case.  */
    if (*s2 == '\0')
        return (char *) s1;
    s2len = str_len(s2);
    for (; (s1 = str_chr(s1, *s2)) != NULL; s1++)
        if (str_ncmp(s1, s2, s2len) == 0)
            return (char *) s1;
    return NULL;
}


/* str_replace */
char *(str_replace)(char *s1, const char *s2, const char *s3)
{
  size_t s2len;
  size_t s3len;
  /* Check for the null s2 case.  */
  if (! s1)
    return NULL;
  
  if (! s2 || ! s3)
    return NULL;
  
  if ((*s2 == '\0') || (*s3 == '\0'))
    return (char *) s1;

  s2len = str_len(s2);
  s3len = str_len(s3);
  
  if (s2len != s3len)
    return NULL;

  NABoolean matchFound = FALSE;
  for (; (s1 = str_chr(s1, *s2)) != NULL; )
    {
      if (str_ncmp(s1, s2, s2len) == 0)
	{
	  matchFound = TRUE;
	  
	  str_cpy_all(s1, s3, s2len);

	  s1 += s2len;
	}
      else
	s1++;
    }

  return (char *) s1;
}


/* str_ncmp */
Int32 (str_ncmp)(const char *s1, const char *s2, size_t n)
{
    unsigned char uc1, uc2;
    /* Nothing to compare?  Return zero.  */
    if (n == 0)
        return 0;
    /* Loop, comparing bytes.  */
    while (n-- > 0 && *s1 == *s2) {
        /* If we've run out of bytes or hit a null, return zero
           since we already know *s1 == *s2.  */
        if (n == 0 || *s1 == '\0')
            return 0;
        s1++;
        s2++;
    }
    uc1 = (*(unsigned char *) s1);
    uc2 = (*(unsigned char *) s2);
    return ((uc1 < uc2) ? -1 : (uc1 > uc2));
}

/* str_chr */
char *(str_chr)(const char *s, Int32 c)
{
    /* Scan s for the character.  When this loop is finished,
       s will either point to the end of the string or the
       character we were looking for.  */
    while (*s != '\0' && *s != (char)c)
        s++;
    return ( (*s == c) ? (char *) s : NULL );
}


/* str_cpy_c */
char *(str_cpy_c)(char *s1, const char *s2)
{
    char *dst = s1;
    const char *src = s2;
    /* Do the copying in a loop.  */
    while ((*dst++ = *src++) != '\0')
        ;
    /* Return the destination string.  */
    return s1;
}

/* str_ncpy */
char *(str_ncpy)(char *s1, const char *s2, size_t n)
{
    char *dst = s1;
    const char *src = s2;
    /* Copy bytes, one at a time.  */
    while (n > 0) {
        n--;
        if ((*dst++ = *src++) == '\0') {
            /* If we get here, we found a null character at the end
               of s2, so use memset to put null bytes at the end of
               s1.  */
            memset(dst, '\0', n);
            break;
        }
    }
    return s1;
}


Int32 (str_cmp_c)(const char *s1, const char *s2)
{
    unsigned char uc1, uc2;
    /* Move s1 and s2 to the first differing characters
       in each string, or the ends of the strings if they
       are identical.  */
    while (*s1 != '\0' && *s1 == *s2) {
        s1++;
        s2++;
    }
    /* Compare the characters as unsigned char and
       return the difference.  */
    uc1 = (*(unsigned char *) s1);
    uc2 = (*(unsigned char *) s2);
    return ((uc1 < uc2) ? -1 : (uc1 > uc2));
}

char *(str_cat_c)(char *s1, const char *s2)
{
    char *s = s1;
    /* Move s so that it points to the end of s1.  */
    while (*s != '\0')
        s++;
    /* Copy the contents of s2 into the space at the end of s1.  */
    strcpy(s, s2);
    return s1;
}


char *str_tok(char *inStr, char c, char **internal)
{
  char *ptr;
  char *tempPtr;

  if (inStr != NULL)
    ptr = inStr;
  else
    ptr = *internal;
  if (ptr == NULL)
    return NULL;
  tempPtr = ptr;
  while (*tempPtr != '\0' && *tempPtr == ' ')
      tempPtr++;
  if (*tempPtr == '\0')
  {
     *internal = NULL;
     return NULL;
  }
  else
    ptr = tempPtr;
  while (*tempPtr != '\0' && *tempPtr != c)
    tempPtr++;
  if (*tempPtr == '\0')
    *internal = NULL;
  else
  {
    *tempPtr = '\0';
    tempPtr++;
    *internal = tempPtr;
  }
  return ptr;
}


/*

Algorithm - Run length encoding (RLE)

If the element repeats less than 2, copy as is.

If an element in the list repeats at least 2 times, copy it twice and
then the count, which is n - 2, where n is the number of repeats.

Note in some cases, the size of compressed could be longer than non-compressed

Examples -
Normal:
  aaaab...                aa<2>b...
  aa<repeat 300>c         aa<255>aa<41>c
  abbbbbc...              abb<3>c...
  aabbcc...               aa<0>bb<0>cc<0>...       -- bad case

where <n> represend counter with value of n, taking one element space.

*/

// max counter value, affacted by element size, e.g 0x80 for char
#define MAXCNT 0xFF //

size_t str_compress_size(const char *src, const size_t len)
{
  // len is original size before compression

  if (len < 3) return len;  // too short to compress

  size_t i = 0;
  size_t j = 0;
  unsigned char k;
  while (i + 2 < len)
  {
    if (src[i] == src[i+1]) // repeated elements
    {
      k = 0;
/* some optimization: don't check count size, 
                  count the repeated element at once
      while (i+k+2 < len && src[i] == src[i+k+2])
      {
        k++;
      };

      j += (k / (MAXCNT + 2) + 1) * 3;
      if (k > MAXCNT && (k % MAXCNT) == 1)
        j -= 2;    // meaning if an element repeats <n> * MAXCNT times,
                   // where <n> is great than 1, we won't compress the
                   // last element. This is because the last element
                   // will not be compressable
*/
      while (src[i] == src[i+k+2] && k < (MAXCNT-1) && i+k+2 < len)
      {
        k++;
      };

      j += 3;
      i += k + 2;
    }
    else // non-repeat element
    {
      j++; i++;
    }
  }

  while (i++ < len)  // last few elements
    j++;

  return j;  // compressed size
}

size_t str_compress(char *tgt, const char *src, const size_t len)
{
  // tgt - compressed
  // src - before compress
  // len is original size before compression

  if (len < 3) return len;  // too short to compress

  size_t i = 0;
  size_t j = 0;
  unsigned char k;
  while (i + 2 < len)
  {
    if (src[i] == src[i+1]) // repeated elements
    {
      k = 0;
      while (src[i] == src[i+k+2] && k < (MAXCNT-1) && i+k+2 < len)
      {
        k++;
      };

      tgt[j++] = src[i];  // first of the repeated elements
      tgt[j++] = src[i+1];  // second of the repeated elements
      tgt[j++] = k;    // repeat count
      i += k + 2;
    }
    else // non-repeat element
    {
      tgt[j++] = src[i++];
    }
  }

  while (i < len)  // last few elements
    tgt[j++] = src[i++];

  return j;  // new size of a, or compressed size
}

size_t str_decompress(char *tgt, const char *src, const size_t srcLen)
{
  // tgt - target space;
  // src - source; srcLen - size of compressed source;
  // return decompressed size.

  size_t i = 0;
  size_t j = 0;
  unsigned char k;

  while (i + 2 < srcLen)
  {
    if (src[i] == src[i+1])
    {  // compressed
      tgt[j++] = src[i++];  // first repeated element
      tgt[j++] = src[i];    // second repeated element
      k = src[i+1];         // the counter

      // uncompress by copying 2nd element
      while (k-- > 0) tgt[j++] = src[i];
      i += 2;
    }
    else
    {
      // not compressed or unable to compress
      tgt[j++] = src[i++];
    }
  }

  while (i < srcLen)  // last few elements
    tgt[j++] = src[i++];

  return j;  // decompressed size
}

// -----------------------------------------------------------------------
// How many bytes are needed to encode byteLen bytes in Hex ASCII?
// Each byte of input string is converted into 2 hexadecimal digit
// ASCII characters output string; for example, the ASCII character 0
// in the input string is converted into 30 in the output string.
// The computed length includes neither the NULL terminator character
// nor the 0x (or 0X) prefix.
// -----------------------------------------------------------------------
size_t str_computeHexAsciiLen(size_t srcByteLen)
{
  return 2*srcByteLen;
}

// -----------------------------------------------------------------------
// Convert the input string (a stream of bytes) into the encoded
// hexadecimal digit ASCII characters returned via the parameter "result".
// The output string does not include the 0x prefix.  By default a
// NULL character - i.e. '\0' - is appended to the output string.
// -----------------------------------------------------------------------
Int32 str_convertToHexAscii(const char * src,               // in
                            const size_t srcLength,         // in
                            char *       result,            // out
                            const size_t maxResultSize,     // in
                            NABoolean    addNullAtEnd)      // in - default is TRUE
{
  const char hexArray[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                             'A', 'B', 'C', 'D', 'E', 'F'};

  if (src == NULL || result == NULL || srcLength <= 0  || maxResultSize <= 0)
    return -590; // ZFIL_ERR_BADPARMVALUE - bad parameter value(s)

  size_t computedHexAsciiStrLen = str_computeHexAsciiLen(srcLength);
  if (computedHexAsciiStrLen + (addNullAtEnd ? 1 : 0) > maxResultSize)
    return -563; // ZFIL_ERR_BUFTOOSMALL - output buffer too small

  const char * srcTemp = src;
  size_t upper4bits, lower4bits;
  char * resultTemp = &result[0];

  // Since source length may be a odd value, it is not possible to
  // convert between little or big endian. We just convert the
  // memory into hex and put it in the string.
  for (size_t i = 0; i < srcLength; i++)
  {
    lower4bits = (*srcTemp) & 0x0F;
    upper4bits = (*srcTemp) & 0xF0;
    upper4bits >>= 4;
    resultTemp[2*i  ] = hexArray[upper4bits];
    resultTemp[2*i+1] = hexArray[lower4bits];
    srcTemp++;
  }

  if (addNullAtEnd)
    result[computedHexAsciiStrLen] = '\0';

  return computedHexAsciiStrLen;
}

// Print the data pointed at by a tupp. The data type
// is inferred from the characters. The arguments
// are obtained from a tupp as follows.
//
//    char * dataPointer = getDataPointer();
//    Lng32 len = tupp_.getAllocatedSize();
//
//    printBrief(dataPointer, len) if you want an end of line
//
//    printBrief(dataPointer, len, FALSE) if you don't 
//    
void printBrief(char* dataPointer, Lng32 len, NABoolean endLine) 
{
   // We don't know what the data type is, but we do know how
   // long the field is. So we will guess the data type.
 
   // Note that varchar length fields are not handled here. For
   // certain Tupp such as MdamPoint, this is OK because the Generator 
   // transforms varchars to chars.
 
   // We might have a null indicator, but we have no way of knowing
   // that here. So we will ignore that possibility. (Sorry!)
 
   // If the length is 2 or 4 or 8, we'll guess that it is an
   // integer and print a signed integer interpretation.
 
   // If the length is 7 and the first two bytes, when interpreted
   // as Big Endian, looks like a year within 100 years of 2000,
   // we'll interpret it as a TIMESTAMP(0).
 
   // There are other possibilities of course which can be added
   // over time but a better solution would be to change the 
   // Generator and Executor to simply give us the data type info.
 
   char local[1001];  // will assume our length is <= 1000
   local[0] = '\0';
 
   if (dataPointer)
     {
     bool allNulls = true;
     bool allFFs = true;
     bool allPrintable = true;
     size_t i = 0;
     while (i < len && (allNulls || allFFs))
       {
       if (dataPointer[i] != '\0') allNulls = false;
       if (dataPointer[i] != -1) allFFs = false;
       if (!isprint(dataPointer[i])) allPrintable = false;
       i++;
       }
     if (allNulls)
       {
       strcpy(local,"*lo*");  // hopefully there won't be a legitimate value of *lo*
       }
     else if (allFFs)
       {
       strcpy(local,"*hi*");  // hopefully there won't be a legitimate value of *hi*
       }
     else if (allPrintable)
       {
       size_t lengthToMove = sizeof(local) - 1;
       if (len < lengthToMove)
         lengthToMove = len;
       strncpy(local,dataPointer,lengthToMove);
       local[lengthToMove] = '\0';
       }
     else  
       {
       // create a hex representation of the first 498 characters
       strcpy(local,"hex ");
       char * nextTarget = local + strlen(local);
       size_t repdChars = ((sizeof(local) - 1)/2) - 4; // -4 to allow for "hex "
       if (len < repdChars)
         repdChars = len;
 
       for (size_t i = 0; i < repdChars; i++)
         {
         unsigned char nibbles[2];
         nibbles[0] = ((unsigned char)dataPointer[i] & 
                       (unsigned char)0xf0)/16;
         nibbles[1] = (unsigned char)dataPointer[i] & (unsigned char)0x0f;
         for (size_t j = 0; j < 2; j++)
           {
           if (nibbles[j] < 10)
             *nextTarget = '0' + nibbles[j];
           else
             *nextTarget = 'a' + (nibbles[j] - 10);
           nextTarget++;
           }  // for j
         }  // for i
 
       *nextTarget = '\0';         
       }
 
     if (len == 2)  // if it might be a short
       {
       // append an interpretation as a short (note that there
       // is room in local for this purpose)
 
       // the value is big-endian hence the weird computation
       long value = 256 * dataPointer[0] + 
                    (unsigned char)dataPointer[1];                  
       sprintf(local + strlen(local), " (short %ld)",value);
       }
     else if (len == 4)  // if it might be a long
       {
       // append an interpretation as a long (note that there
       // is room in local for this purpose)
 
       // the value is big-endian hence the weird computation
       long value = 256 * 256 * 256 * dataPointer[0] + 
                    256 * 256 * (unsigned char)dataPointer[1] +  
                    256 * (unsigned char)dataPointer[2] + 
                    (unsigned char)dataPointer[3];           
       sprintf(local + strlen(local), " (long %ld)",value);
       }
     else if (len == 8)  // if it might be a 64-bit integer
       {
       // append an interpretation as a short (note that there
       // is room in local for this purpose)
 
       // the value is big-endian hence the weird computation
       long long value = 256 * 256 * 256 * dataPointer[0] + 
                    256 * 256 * (unsigned char)dataPointer[1] +  
                    256 * (unsigned char)dataPointer[2] + 
                    (unsigned char)dataPointer[3]; 
       value = (long long)256 * 256 * 256 * 256 * value +   
                    256 * 256 * 256 * (unsigned char)dataPointer[4] + 
                    256 * 256 * (unsigned char)dataPointer[5] +  
                    256 * (unsigned char)dataPointer[6] + 
                    (unsigned char)dataPointer[7];        
       sprintf(local + strlen(local), " (long long %lld)",value);
       }
     else if (len == 7)  // a TIMESTAMP(0) perhaps?
       {
       long year = 256 * dataPointer[0] +
                           (unsigned char)dataPointer[1];
       if ((year >= 1900) && (year <= 2100))
         {
         // looks like a TIMESTAMP(0); look further
         long month = (unsigned char)dataPointer[2];
         long day = (unsigned char)dataPointer[3];
         long hour = (unsigned char)dataPointer[4];
         long minute = (unsigned char)dataPointer[5];
         long second = (unsigned char)dataPointer[6];
 
         if ((month >= 1) && (month <= 12) &&
             (day >= 1) && (day <= 31) &&
             (hour >= 0) && (hour <= 23) &&
             (minute >= 0) && (minute <= 59) &&
             (second >= 0) && (second <= 59))
           {
           sprintf(local + strlen(local), 
                   " (TIMESTAMP(0) %ld-%02ld-%02ld %02ld:%02ld:%02ld)",
                   year,month,day,hour,minute,second);
           }
         }
       }
     }    
   cout << local;
   if (endLine)
     cout << endl;
 }

