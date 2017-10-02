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
#ifndef CSCONVERT_H
#define CSCONVERT_H

//
// This source file and csconvert.cpp contain interface routines to
// the character set conversion routines that are coded in C.
//
// NOTE: These routines are coded very generically so that the source
//       for them can be used in not only the SQL/MX compiler build,
//       but also used by the ODBC build and maybe others.

enum cnv_version { cnv_version1 = 1 }; /* For future expansion */

#ifndef cnv_charset_DEFINED
#define cnv_charset_DEFINED
enum cnv_charset { cnv_UnknownCharSet =  0,  cnv_UTF8     = 1,
                   cnv_UTF16    = 2,         cnv_UTF32    = 3,
                   cnv_ISO88591 = 4,         cnv_SJIS     = 5,
                   cnv_EUCJP    = 6,         cnv_KSC      = 7, 
                   cnv_BIG5     = 8,         cnv_GB2312   = 9,
                   cnv_GB18030  = 10,        cnv_GBK      = 11,
                   cnv_Last_Valid_CS = 11
                  };
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

//NOTE: The following definitions assume that FALSE = 0 and TRUE <> 0
int  LocaleToUTF8(  const enum cnv_version version,
                    const char *in_bufr,  const int in_len, 
                    const char *out_bufr, const int out_len,
                    enum cnv_charset charset, 
                    char * & first_untranslated_char,
                    unsigned int *output_data_len_p      = NULL ,
                    const int addNullAtEnd_flag          = FALSE,
                    unsigned int *translated_char_cnt_p  = NULL );

int  UTF8ToLocale(  const enum cnv_version version,
                    const char *in_bufr,  const int in_len, 
                    const char *out_bufr, const int out_len,
                    enum cnv_charset charset, 
                    char * & first_untranslated_char, 
                    unsigned int *output_data_len_p      = NULL ,
                    const int addNullAtEnd_flag          = FALSE , 
                    const int allow_invalids             = FALSE ,
                    unsigned int * translated_char_cnt_p = NULL ,
                    const char *substitution_char        = NULL );

int  LocaleToUTF16( const enum cnv_version version,
                    const char *in_bufr,  const int in_len, 
                    const char *out_bufr, const int out_len,
                    enum cnv_charset charset, 
                    char * & first_untranslated_char,
                    unsigned int *output_data_len_p      = NULL ,
                    const int cnv_flags                  = 0 ,
                    const int addNullAtEnd_flag          = FALSE,
                    unsigned int *translated_char_cnt_p  = NULL ,
                    unsigned int max_chars_to_convert    = 0xffffffff);

int  UTF16ToLocale( const enum cnv_version version,
                    const char *in_bufr,  const int in_len, 
                    const char *out_bufr, const int out_len,
                    enum cnv_charset charset, 
                    char * & first_untranslated_char, 
                    unsigned int *output_data_len_p      = NULL ,
                    const int cnv_flags                  = 0 ,
                    const int addNullAtEnd_flag          = FALSE , 
                    const int allow_invalids             = FALSE ,
                    unsigned int * translated_char_cnt_p = NULL  ,
                    const char *substitution_char        = NULL );

int gbkToUtf8(char* gbkString, size_t gbklen,
              char* result ,size_t outlen, bool addNullAtEnd=FALSE);

/*
 * LocaleCharToUCS4() converts the FIRST char in the input string to its
 * UCS4 value.  Returns the UCS4 value at location specified AND the
 * length of the input character in bytes as the return value.
 */
int  LocaleCharToUCS4( const char *in_bufr,       //Ptr to Input string
                       const int in_len,          //Len of Input string (bytes)
                       unsigned int *UCS4ptr ,    //Ptr to output location
                       enum cnv_charset charset ); //Locale Character Set
/*
 * UCS4ToLocaleChar() converts the UCS4 value to the specified character set
 * and stores the character in the output buffer specified.
 * Returns length of the output character in bytes as the return value.
 */
int  UCS4ToLocaleChar( const unsigned int *UCS4ptr , //Ptr to input char
                       const char *out_bufr,         //Ptr to output bufr
                       const int out_len,            //Len of output bufr
                       enum cnv_charset charset );   //Locale Character Set

/*
 * For each routine, the return value is 0 for success.  
 * Otherwise, it is one of the following error codes.
 */
#define CNV_ERR_INVALID_CHAR         -1 // Character in input cannot be converted
#define CNV_ERR_BUFFER_OVERRUN       -2 // No output buffer or not big enough
#define CNV_ERR_NOINPUT              -3 // No input buffer or input cnt <= 0 
#define CNV_ERR_INVALID_CS           -4 // Invalid Character Set specified
#define CNV_ERR_INVALID_VERS         -5 // Invalid version specified
#define CNV_ERR_NO_CONVERSION_NEEDED -6 // Source and target Character Sets are the same
#define CNV_ERR_TARGET_SIZE_INVALID  -7 // Provided target buffer is not large enough
                                        // to handle the conversion
#define CNV_ERR_INVALID_HEAP         -8 // A valid HEAP pointer was not provided

/*
 * For the cnv_flags argument to LocaleToUTF16(), the following is defined:
 */
#define CNV_REVERSE_OUTBYTES 0x1 // Set TRUE when output data must be Big-Endian
//                                  and running on a Little-Endian machine
//                                  or vice versa.

/*
 * For the cnv_flags argument to UTF16ToLocale(), the following is defined:
 */
#define CNV_REVERSE_INBYTES  0x2 // Set TRUE when input data is Big-Endian
//                                  and running on a Little-Endian machine
//                                  or vice versa.

/* NOTES:
 *
 * All buffer lengths are in BYTES.  
 *
 * The caller is responsible for allocating the output buffer
 * and ensuring it is big enough (or dealing with a non-zero
 * return value--looping or something--if the output buffer
 * isn't big enough.)
 *
 * The first_untranslated_char pointer will be set to point
 * within the input buffer to the first character position
 * that was not processed (either because it was a bad
 * character OR because the output buffer was full OR because
 * the caller-specified maximum (max_chars_to_convert) limit
 * was reached).  If the caller's input buffer is exhausted
 * without returning early, the first_untranslated_char pointer
 * will be set to point to the end of the input buffer.
 *
 * For the cnv_flags arg, see the related #defines above.
 * 
 * All 4 of these interface routines assume that the caller
 * will deal with any BOM (Byte-Order-Mark) at the start of
 * any file that the input data might be coming from...and
 * that the caller will prepend any BOM needed before any
 * output data from these routines is put into a file.
 * It is anticipated that, if there are any such files, they
 * will be in Big-Endian format, although that is up to the
 * callers.
 * 
 * The addNullAtEnd_flag, if TRUE, specifies to add a NULL
 * (1 or 2 bytes of binary 0) at the end of the valid data in
 * the output buffer (provided, of course, that there is
 * sufficient room in the output buffer to do so.)
 *
 * The optional translated_char_cnt pointer argument, if
 * supplied, is where the routine returns the count of
 * successfully translated characters (whether an error is
 * encountered or not.)  If not supplied or if NULL is
 * supplied, the count is not returned.  This is a character
 * count, not a byte count.
 *
 * The allow_invalids flag, if true, results in a substitution
 * character (see next paragraph) being put in the output buffer
 * whenever UTF16ToLocale() encounters a Unicode character
 * that it cannot translate to the specified character set.
 * For UTF16ToLocale(), after putting the substitution char in
 * the output buffer, the routine will keep going after skipping
 * the "bad" character in the input buffer.  If the "bad" 
 * character was a valid UTF16 character but just couldn't
 * be translated to the specified output locale character
 * set, the entire character will be skipped.  Otherwise
 * two bytes of the input buffer will be skipped.
 * For UTF8ToLocale(), after putting the substitution char in
 * the output buffer, the routine will keep going after skipping
 * the "bad" character in the input buffer.  If the "bad" 
 * character was a valid UTF8 character but just couldn't
 * be translated to the specified output locale character
 * set, the entire character will be skipped.  Otherwise
 * one byte of the input buffer will be skipped.
 *
 * The substitution_char pointer, if not NULL, should point to
 * a 1-byte or 2-byte substitution character followed immediately
 * by a byte containing a binary 0.  See description of 
 * the allow_invalids flag above.  If NULL is specified and the
 * allow_invalids flag is non-zero, then the default substitution
 * character, namely a ? (question mark), is used.
 */

/*
 * Two methods to validate a UTF8 string to ensure that it has
 * no partial characters and that it has no more than a given
 * number of characters.
 *
 * Return value:
 *  negative value:
 *                  The string contains invalid
 *                  UTF-8 or parameter error. Note
 *                  that this light validation routine
 *                  does NOT recognize invalid code points,
 *                  overlong encodings, or UTF-16 surrogate
 *                  pairs encoded as two UTF-8 chars, and
 *                  possibly other problems.
 *  non-negative value:
 *                  The valid length of the string,
 *                  after removing partial characters
 *                  and extraneous characters
 *
 * The second method also pads any bytes that are
 * truncated with blanks
 */

int lightValidateUTF8Str(const char *bufr,                 // ptr to buffer to validate
                         int in_len,                       // len in bytes of buffer
                         int max_chars = 0,                // max chars allowed in buffer or 0 for unlimited
                         int ignore_trailing_blanks = 1);  // don't count trailing blanks as chars   

int lightValidateUTF8StrAndPad(char *bufr,
                               int in_len,
                               int max_chars = 0,
                               int ignore_trailing_blanks = 1);

/* A method to create the minimum/maximum valid UTF-8 character
   string that fits into a given buffer. Used to form low/high keys.
   If max_chars is > 0, generates at most max_chars and pads
   the remaining bytes with blanks. Returns the space occupied
   by actual characters, not padding (same as in_len if max_chars == 0).
*/
int fillWithMinUTF8Chars(char *bufr,
                         int in_len, // in bytes
                         int max_chars);
int fillWithMaxUTF8Chars(char *bufr,
                         int in_len, // in bytes
                         int max_chars);
inline int fillWithMinMaxUTF8Chars(char *bufr,
                                   int in_len, // in bytes
                                   int max_chars,
                                   int is_max)
{ if (is_max) return fillWithMaxUTF8Chars(bufr, in_len, max_chars);
  else        return fillWithMinUTF8Chars(bufr, in_len, max_chars); }
                                   

/* A method to find the beginning of a UTF8 char that is at the end off
   a buffer.
*/
char * findStartOfChar( char *someByteInChar, char *startOfBuffer );


#endif /* CSCONVERT_H */
