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
#ifndef STR_H
#define STR_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         str.h
 * Description:  SQL/MX string functions. One big reason for declaring our
 *               own string functions is that much of the SQL/MX code
 *               can not use the C runtime (see also file NAStdlib.h).
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
#include <string.h>
#include "Platform.h"

#include "NAWinNT.h"

#include "NAStdlib.h"
#include "unicode_char_set.h"
#include "Int64.h"

#include "NAAssert.h"

// -----------------------------------------------------------------------
// toupper() and tolower() equivalence
// -----------------------------------------------------------------------
#ifndef TOUPPER
#define TOUPPER(c) (((c >= 'a') && (c <= 'z')) ? (c - 32) : c)
#endif
#ifndef TOLOWER
#define TOLOWER(c) (((c >= 'A') && (c <= 'Z')) ? (c + 32) : c)
#endif


//
// ---------------------------------------------------------------------
//  Helper functions for ISO 8859_1 (8-bit european) alphabet processing
// ---------------------------------------------------------------------
// 
Int32 isUpper8859_1(NAWchar c);
Int32 isLower8859_1(NAWchar c);
Int32 isAlpha8859_1(NAWchar c);
Int32 isAlNum8859_1(NAWchar c);
Int32 isSpace8859_1(NAWchar c);
Int32 isHexDigit8859_1(NAWchar c); // 0, 1, .., A, B, C, D, E, F (case insensitive)

Int32 isDigit8859_1(NAWchar c);

Int32 isCaseInsensitive8859_1(NAWchar c); // ISO 8859-1 char for which there is no
                               // upcase equivalent.  hex values 0xDF & 0xFF

// -----------------------------------------------------------------------
// Compare strings <left> and <right> (using unsigned comparison).
// for <length> characters.
// Return -1 if left < right,
// return 0 if left == right,
// return 1 if left > right.
// -----------------------------------------------------------------------
inline Int32 str_cmp(const char *left, const char *right, Int32 length)
{
  Int32 result = memcmp(left, right, length);
  if (result == 0)
    return 0;
  else
    return ((result > 0)? 1: -1);
}
Int32 str_cmp_ne(const char *left, const char *right);

// str_cmp_c() takes two strings as arguments and returns a value less
// than zero if the first is lexographically less than the second, a 
// value greater than zero if the first is lexographically greater 
// than the second, or zero if the two strings are equal. The 
// comparison is done by comparing the coded (ascii) value of the 
// chararacters, character by character.
Int32 (str_cmp_c)(const char *s1, const char *s2);

// The str_ncmp() function shall compare not more than n bytes 
// (bytes that follow a null byte are not compared) from the array 
// pointed to by s1 to the array pointed to by s2. The sign of a 
// non-zero return value is determined by the sign of the difference 
// between the values of the first pair of bytes (both interpreted
//  as type unsigned char) that differ in the strings being compared.
Int32 (str_ncmp)(const char *s1, const char *s2, size_t n);

// The str_str() function shall locate the first occurrence in the 
// string pointed to by s1 of the sequence of bytes (excluding the
// terminating null byte) in the string pointed to by s2. The 
// function returns the pointer to the matching string in s1 or a 
// null pointer if a match is not found. If s2 is an empty string,
//  the function returns s1.
char *(str_str)(const char *s1, const char *s2);

// The str_chr() function shall locate the first occurrence of c 
// (converted to a char) in the string pointed to by s. The terminating
//  null byte is considered to be part of the string. The function 
//  returns the location of the found character, or a null pointer 
//  if the character was not found.
//  This function is used to find certain characters in strings.
char *(str_chr)(const char *s, Int32 c);

// The str_replace() function shall locate all occurrences in the 
// string pointed to by s1 of the sequence of bytes (excluding the
// terminating null byte) in the string pointed to by s2 and replace
// them with string pointed to by s3.
// s2 and s3 must have the same length.
// The function returns the pointer to the replaced string s1 or a 
// null pointer if a match is not found. If s2 or s3 are empty strings,
//  or do not have the same length, the function returns NULL.
char *(str_replace)(char *s1, const char *s2, const char *s3);

// -----------------------------------------------------------------------
// fill string <str>  for <length> bytes with <padchar>
// -----------------------------------------------------------------------
inline
Int32 str_pad(char *str, Int32 length, char padchar = ' ')
{
  // Below is a more efficient version of str_pad. The C++ runtime
  // routine memset() provides the same service, but hopefully with
  // better performance. On NSK, memset is part of the system library.
  // int i;
  //  
  // for (i=0; i<length; i++)
  //  str[i] = padchar;

  memset(str, padchar, length);

  return 0;
  
}

// -----------------------------------------------------------------------
// copy <tgtlen> bytes of string <src> into <tgt>, if <src> is
// shorter than <tgtlen> bytes then pad <tgt> with <padchar>
//
// WARNING: This routine *does not* work exactly like the C-runtime strcpy(),
//          nor the strncpy() functions.
//
//          For example: str_cpy(dest,src,str_len(src)) is guaranteed
//          to result in a non-null-terminated string.  Adding one or
//          more to the result of str_len() won't help.
//
//          But          str_cpy(dest,src,str_len(src)+1,'\0') should
//          work.  You may want to use str_cpy_all() instead; that
//          routine works more like memcpy() and behaves well.
// -----------------------------------------------------------------------
Int32 str_cpy(char *tgt, const char *src, Int32 tgtlen, char padchar = ' ');

// -----------------------------------------------------------------------
// This routine behaves like str_cpy except that the length of
// the target is also supplied.
// -----------------------------------------------------------------------
Int32 byte_str_cpy(char *tgt, Int32 tgtlen, 
		 const char *src, Int32 srclen, char padchar);

// Perform a true C style strcpy()
// The str_cpy_c() function shall copy the string pointed to by s2 
// (including the terminating null byte) into the array pointed to by 
// s1. If copying takes place between objects that overlap, 
// the behavior is undefined. The function returns s1. No value is 
// used to indicate an error.
char *(str_cpy_c)(char *s1, const char *s2);

// The str_ncpy() function shall copy not more than n bytes (bytes 
// that follow a null byte are not copied) from the array pointed to
// by s2 to the array pointed to by s1. If copying takes place between
// objects that overlap, the behavior is undefined. If the array 
// pointed to by s2 is a string that is shorter than n bytes, null bytes
// shall be appended to the copy in the array pointed to by s1, until
//  n bytes in all are written. The function shall return s1; 
//  no return value is reserved to indicate an error.
char *(str_ncpy)(char *s1, const char *s2, size_t n);

// -----------------------------------------------------------------------
// concatenate <first> and <second> and store the result in <result>
// -----------------------------------------------------------------------
Int32 str_cat(const char *first, const char *second, char *result);


// The str_cat_c() function shall append a copy of the string pointed to
// by s2 (including the terminating null byte) to the end of the string 
// pointed to by s1. The initial byte of s2 overwrites the null byte 
// at the end of s1. If copying takes place between objects that 
// overlap, the behavior is undefined. The function returns s1.
// This function is used to attach one string to the end of another
// string. It is imperative that the first string (s1) have the space 
// needed to store both strings.
char *(str_cat_c)(char *s1, const char *s2);

// -----------------------------------------------------------------------
// convert <i> to ASCII and store the result in <outstr>
// -----------------------------------------------------------------------
char *str_itoa(ULng32 i, char *outstr);
char *str_ltoa(Int64 i, char *outstr);

// -----------------------------------------------------------------------
// convert instr to numeric and return in i. String must be numbers only.
// -----------------------------------------------------------------------
Int64 str_atoi(const char * instr, Lng32 instrLen);

// convert a scaled exact numeric string and return as float.
// input of the form: mmm.ff
double str_ftoi(const char * instr, Lng32 instrLen);

// Dummy routine to ensure that str_cpy_all gets inlined.  Once 
// the compiler is fixed to inline routines with calls to assert, 
// Remove callAssert() in callers and replace with direct call to 
// assert.
void callAssert(const char* tgt, const char* src, Lng32 length);

// -----------------------------------------------------------------------
// move <length> bytes from <src> to <tgt>
// -----------------------------------------------------------------------
inline void str_cpy_all(char *tgt, const char *src, Lng32 length)
{
  if ((!tgt || !src) && length)
    callAssert(tgt, src, length);
  // Below is a more inefficient version of str_cpy_all. The C++ runtime
  // procedure "memcpy()" provides the same service, but hopefully with
  // better performance, by copying entire words where possible.
  // We do not source in a header file for memcpy, since we want to
  // avoid dependencies to the C++ run-time, instead it is simply declared
  // as an external procedure (hopefully with the same parameters on all
  // platforms). On NSK, memcpy is part of the system library.
  //
  //It has been noticed that Executor can call this function with overlapped
  // memory. Change to memmove() during DUMA work.
  memmove(tgt, src, length);
}

// -----------------------------------------------------------------------
// move <length> bytes from <src> to <tgt>
//   handles overlapping memory between target and source
// -----------------------------------------------------------------------
void str_memmove(char *tgt, const char *src, Lng32 length);

// -----------------------------------------------------------------------
// allocate new <tgt>,
// move <length> characters from <src> into <tgt>, reserve the
// the first 4 bytes of <tgt> for the varchar length and set the varchar
// length to <length>.
//
// Compare convertNAString in String.h
// -----------------------------------------------------------------------
Int32 str_varchar_alloc_and_copy(char *tgt, const char *src, Lng32 length);

// -----------------------------------------------------------------------
// Copies <src> to <tgt> for <length> bytes.
// Removes trailing <blank_char>s by putting an <end_char> as terminator.
// -----------------------------------------------------------------------
Int32 str_cpy_and_null(char * tgt,
		       const char * src,
		       Lng32 length,
		       char end_char = '\0',
		       char blank_char = ' ',
		       NABoolean nullTerminate = FALSE);
		
// ---------------------------------------------------------------
// copies src to tgt for length bytes and upshifts, if upshift <> 0,
// else downshifts.
// Src and Tgt may point to the same location.
// ---------------------------------------------------------------
Int32 str_cpy_convert(char * tgt, char * src,
		Lng32 length, Int32 upshift);
     
Int32 str_len(const char * s);

// -----------------------------------------------------------------------
// "Increments" a string.  If successful, 0 is returned.  Otherwise, 1
// is returned.
// -----------------------------------------------------------------------
Int32 str_inc(const ULng32 length, char * s);

// -----------------------------------------------------------------------
// Complements a string.  
// -----------------------------------------------------------------------
void str_complement(const ULng32 length, char * s);

//------------------------------------------------------------------------
// Our own version of sprintf that provides a subset of the functionality
// of sprintf as we cannot call C runtime from the SQL executor.
//------------------------------------------------------------------------

#define str_sprintf sprintf
Int32 str_sprintf(char *, const char *, ...);

// -----------------------------------------------------------------------
// Encoding and decoding of an array of bytes into characters
//   -- how many string bytes are needed to encode byteLen bytes?
//   -- encode srcLen bytes into a string (not null terminated),
//      return the encoded length
//   -- how many bytes are in a given encoded string?
//   -- decode a string into byte representation and return number of
//      bytes decoded
// -----------------------------------------------------------------------

Lng32 str_encoded_len(Lng32 byteLen);
Lng32 str_encode(char *tgt, Lng32 tgtMaxLen, void *src, Lng32 srcLen);
Lng32 str_decoded_len(Lng32 srcLen);
Lng32 str_decode(void *tgt, Lng32 tgtMaxLen, const char *src, Lng32 srcLen);

// Base64 encoding and decoding.
Lng32 str_encoded_len_base64(Lng32 len);
Lng32 str_decoded_len_base64(Lng32 len);
Lng32 str_encode_base64(const unsigned char* in, Lng32 in_len,
                        char *out, Lng32 out_len);
Lng32 str_decode_base64(const unsigned char* in, Lng32 in_len,
                        char *out, Lng32 out_len);

//------------------------------------------------------------------------
// Strips leading and/or trailing blanks. src will contain a NULL after the
// end of the first non-blank character.The length of the "stripped" string
// is returned in len.
// Returns pointer to the start of string after leading blanks.
//------------------------------------------------------------------------
char * str_strip_blanks(char *src , Lng32 &len, 
                      NABoolean stripLeading = FALSE,
                      NABoolean stripTrailing = TRUE
                        );
//------------------------------------------------------------------------
// This funtion transforms src into the form of an SQL identifier
// Input : src,allowedChar
// Output: tgt, tgtLen
// The caller needs to pass in the string to be tranformed in src.
// tgt needs to be a pointer pointing to a buffer atleast as long as src
// allowedChars is generally NULL. But if the caller wants certain special
// characters to be allowed as part of the Id, then a null termintated string
// of these chars needs to be passed in eg : "$#"
//------------------------------------------------------------------------
Lng32 str_to_ansi_id(char *src, char *tgt,Lng32 &tgtLen,short mustValidate = 1, char *allowedChar = 0);
//int strSprintf(char * buffer, char * format, ...);

// -----------------------------------------------------------------------
// following function is used to extract a name that may be delimited
// from a given string.  The name may end with a separator or end of line. 
// -----------------------------------------------------------------------
Int32 extractDelimitedName (char* tgt,  const char* const src, const char sep = '.');

// -----------------------------------------------------------------------
// following function is used to return the catalog and schema names
// given a qualified table name. Either the catalog or schema name can be
// a delimited identifier name.
// -----------------------------------------------------------------------

void extractCatSchemaNames (char* catName, char *schName, char* qualTabName);

//inline
Int32 mem_cpy_all(void *tgt, const void *src, Lng32 length);
//{
//#if defined (NA_YOS)
//  for (int i = 0; i < length; i++)
//    ((unsigned char *)tgt)[i] = ((unsigned char *)src)[i];
//#else
//  memcpy(tgt, src, length);
//#endif
//  return 0;
//}


// Similar to strtok, but for the following
// Only one character can be delimiter
// Third parameter char **internal should not be manipulated by the caller
// Removes blanks at the begining of the tokem automatically
char *str_tok(char *inStr, char c, char **internal);


//------------------------------------------------------------------------
// String compression and decompression functions using RLE method
// See str.cpp for details
//------------------------------------------------------------------------
size_t str_compress_size(const char *src, const size_t len);
size_t str_compress(char *tgt, const char *src, const size_t len);
size_t str_decompress(char *tgt, const char *src, const size_t srcLen);

// -----------------------------------------------------------------------
// How many bytes are needed to encode byteLen bytes in Hex ASCII?
// Each byte of input string is converted into 2 hexadecimal digit
// ASCII characters output string; for example, the ASCII character 0
// in the input string is converted into 30 in the output string.
// The computed length includes neither the NULL terminator character
// nor the 0x (or 0X) prefix.
// -----------------------------------------------------------------------
size_t str_computeHexAsciiLen(size_t srcByteLen);

// -----------------------------------------------------------------------
// Convert the input string (a stream of bytes) into the encoded
// hexadecimal digit ASCII characters returned via the parameter "result".
// The output string does not include the 0x prefix.  By default a
// NULL character - i.e. '\0' - is appended to the output string.
// -----------------------------------------------------------------------
Int32 str_convertToHexAscii(const char * src,                   // in
                          const size_t srcLength,             // in
                          char *       result,                // out
                          const size_t maxResultSize,         // in - including NULL terminator if addNullAtEnd
                          NABoolean    addNullAtEnd = TRUE);  // in


// Print the data pointed at by a tupp. The data type
// is inferred from the characters. The arguments
// are the obtained from a tupp as follows.
//
//    char * dataPointer = getDataPointer();
//    Lng32 len = tupp_.getAllocatedSize();
//
//    printBrief(dataPointer, len) if you want an end of line
//
//    printBrief(dataPointer, len, FALSE) if you don't 
//    
void printBrief(char* dataPointer, Lng32 keyLen, NABoolean endLine = TRUE); 
#endif // STR_H
