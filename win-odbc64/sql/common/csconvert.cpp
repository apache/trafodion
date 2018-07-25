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

//
// This source file contains interface routines to the Open Source 
// code character set conversion routines that are coded in C.
//
// NOTE: These routines are coded very generically so that the source
//       for them can be used in not only the SQL/MX compiler build,
//       but also used by the ODBC build and maybe others.

#include <limits.h>
#include "multi-byte.h"
#include "fcconv.h"
#include "csconvert.h"

#pragma nowarn(161)   // warning elimination
#include "from_GB18030.c"
#include "from_GB2312.c"
#include "from_GBK.c"

#define  USE_OUR_MB_WC_DATA_TABLES
#include "UCS_jp_data.c"
#include "UCS_zs_data.c"
#include "UCS_zb_data.c"
#include "UCS_ko_data.c"
#include "mb_iconv.c"
#include "iconv_gen.c"

#pragma warn(161)   // warning elimination

#include "mb_lconv.c"
#undef   USE_OUR_MB_WC_DATA_TABLES

#if 0 // Don't need these since we chose to support GBK (a superset of GB2312) instead
#define CODESET gb2312
#define OUR_CS_GB2312_specific
#define OUR_CS_GBK_specific
#include "mb_lconv.c"
#undef  OUR_CS_GBK_specific
#undef  OUR_CS_GB2312_specific
#undef  CODESET
#endif // Don't need these since we chose to support GBK (a superset of GB2312) instead

#define CODESET gbk
#define OUR_CS_GBK_specific
#define OUR_CS_GB2312_specific
#include "mb_lconv.c"
#undef  OUR_CS_GBK_specific
#undef  OUR_CS_GB2312_specific
#undef  CODESET

#define CODESET gb18030
#define OUR_CS_GB18030_specific
#include "mb_lconv.c"
#undef  OUR_CS_GB18030_specific
#undef  CODESET

#define ENSURE_VALID_CHARSET()                                               \
  {                                                                          \
     if ( (charset == cnv_UnknownCharSet) || (charset > cnv_Last_Valid_CS) ) \
        return( CNV_ERR_INVALID_CS );                                        \
  }

#define ENSURE_VALID_INPUT() \
  { if ( (in_bufr == NULL) || (in_len <= 0) ) return CNV_ERR_NOINPUT; }

#define ENSURE_VALID_OUTPUT() \
  { if ( (out_bufr == NULL) || (out_len <= 0) ) return CNV_ERR_BUFFER_OVERRUN; }

#define CHECK_FOR_SERIOUS_ERRORS() \
  { ENSURE_VALID_CHARSET(); ENSURE_VALID_INPUT(); ENSURE_VALID_OUTPUT(); }

#define SET_TRANSLATED_CHAR_CNT()                         \
  {                                                       \
     if ( translated_char_cnt_p != NULL )                 \
         *translated_char_cnt_p = (translated_char_cnt) ; \
  }

#define SET_OUTPUT_DATA_LEN()                                        \
  {                                                                  \
     if ( output_data_len_p != NULL )                                \
         *output_data_len_p = ( (char *)target - (char *)out_bufr ); \
  }

#define INITIALIZE_VARIABLES()                              \
     first_untranslated_char = (char *) in_bufr;            \
     unsigned int translated_char_cnt = 0;                  \
     unsigned char * source    = (unsigned char *) in_bufr; \
     unsigned char * endSource = source + in_len ;          \
     SET_TRANSLATED_CHAR_CNT();                             \

typedef size_t (*csc_mbtowc_funcPtr) ( WChar_t *pwc, const char *ts,
                                       size_t maxlen, _LC_charmap_t *hdl ) ;
typedef int    (*csc_input_utfPtr)   ( _LC_fcconv_iconv_t *, uchar_t **, int ) ;
typedef int    (*csc_wctomb_funcPtr) ( char *s, WChar_t wc,
                                                _LC_charmap_t *hdl ) ;
typedef int    (*csc_output_utfPtr)  ( _LC_fcconv_iconv_t *, uchar_t *,
                                                             int, ucs4_t) ;

static const csc_mbtowc_funcPtr  csc_mbtowc_ptrs[ cnv_Last_Valid_CS + 1] = {
   NULL,                   // cnv_UnknownCharset
   NULL,                   // cnv_UTF8
   NULL,                   // cnv_UTF16,
   NULL,                   // cnv_UTF32,
   NULL,                   // cnv_ISO88591
   Our_mbtowc_sjis_ucs4,   // See Our_mbtowc_sjis_ucs4()  in mb_lconv.c
   Our_mbtowc_eucjp_ucs4,  // See Our_mbtowc_eucjp_ucs4() in mb_lconv.c
   Our_mbtowc_cp949_ucs4,  // See Our_mbtowc_cp949_ucs4() in mb_lconv.c
   Our_mbtowc_big5_ucs4,   // See Our_mbtowc_big5_ucs4()  in mb_lconv.c
   __mbtowc_gbk_ucs4,      // See MBTOWC()  in mb_lconv.c
   __mbtowc_gb18030_ucs4,  // See MBTOWC()  in mb_lconv.c
   __mbtowc_gbk_ucs4       // See MBTOWC()  in mb_lconv.c
};

static const csc_input_utfPtr  csc_input_utf_ptrs[ cnv_Last_Valid_CS + 1] = {
   NULL,                   // cnv_UnknownCharset
   __input_utf8,           // cnv_UTF8
   __input_ucs2,           // cnv_UTF16,
   __input_ucs4,           // cnv_UTF32,
   NULL,                   // cnv_ISO88591
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
};

static const csc_wctomb_funcPtr  csc_wctomb_ptrs[ cnv_Last_Valid_CS + 1] = {
   NULL,                   // cnv_UnknownCharset
   NULL,                   // cnv_UTF8
   NULL,                   // cnv_UTF16,
   NULL,                   // cnv_UTF32,
   NULL,                   // cnv_ISO88591
   Our_wctomb_sjis_ucs4,   // See Our_mbtowc_sjis_ucs4()  in mb_lconv.c
   Our_wctomb_eucjp_ucs4,  // See Our_mbtowc_eucjp_ucs4() in mb_lconv.c
   Our_wctomb_cp949_ucs4,  // See Our_mbtowc_cp949_ucs4() in mb_lconv.c
   Our_wctomb_big5_ucs4,   // See Our_mbtowc_big5_ucs4()  in mb_lconv.c
   __wctomb_gbk_ucs4,      // See MBTOWC()  in mb_lconv.c
   __wctomb_gb18030_ucs4,  // See MBTOWC()  in mb_lconv.c
   __wctomb_gbk_ucs4       // See MBTOWC()  in mb_lconv.c
};

static const csc_output_utfPtr  csc_output_utf_ptrs[ cnv_Last_Valid_CS + 1] = {
   NULL,                   // cnv_UnknownCharset
   __output_utf8,          // cnv_UTF8
   __output_ucs2,          // cnv_UTF16,
   __output_ucs4,          // cnv_UTF32,
   NULL,                   // cnv_ISO88591
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
};

//
//  LocaleToUTF16() - Convert a string of characters in the specified
//                    character set to UTF-16.
//
int  LocaleToUTF16( const enum cnv_version version ,
                    const char *in_bufr ,  const int in_len ,
                    const char *out_bufr , const int out_len ,
                    enum cnv_charset charset ,
                    char * & first_untranslated_char ,
                    unsigned int *output_data_len_p ,
                    const int cnv_flags ,
                    const int addNullAtEnd_flag ,
                    unsigned int * translated_char_cnt_p ,
                    unsigned int max_chars_to_convert )
{
    if ( version != cnv_version1 )
        return CNV_ERR_INVALID_VERS;

    // Initialize some return values early ... in case we give error

    INITIALIZE_VARIABLES();

    ucs2_t * target  = (ucs2_t *) out_bufr;
    ucs2_t * endTarget = target + ( out_len / sizeof(ucs2_t) );

    SET_OUTPUT_DATA_LEN();

    CHECK_FOR_SERIOUS_ERRORS();

    // We initialize a  _LC_fcconv_iconv_rec  struct here.
    // NOTE: For our purposes, the ONLY thing that 
    //       must be initialized is the flags word.
    //
    _LC_fcconv_iconv_rec  cd;

    int revBytes = cnv_flags & CNV_REVERSE_OUTBYTES;
    cd.flags = CONV_BOM_WRITTEN | CONV_INPUT_PROCESSED |
               (revBytes ? CONV_REVERSE_OUTBYTE : 0);

    if ( max_chars_to_convert == 0xFFFFFFFF )
         max_chars_to_convert  = (unsigned int)in_len ;
    //
    // Fast path where charset is ISO88591 or a multi-byte charset.
    // An assumption made here is that non-ASCII chars will rarely be seen.
    // If one is found, we break out of this fast path and go down the
    // slow path.
    //
    int charsetIsWide = 0;
    if ( (charset == cnv_UTF16) || (charset == cnv_UTF32) )
       charsetIsWide = 1 ;

    if ( ! charsetIsWide )
    {
       unsigned int UCS4 = 0; 

       int  maxLoopCnt = (int)( endTarget - target );
       if ( maxLoopCnt > in_len )
            maxLoopCnt = in_len ;
       if ( maxLoopCnt > (int) max_chars_to_convert )
            maxLoopCnt = (int) max_chars_to_convert ;

       unsigned int maxCharToHandle = (charset == cnv_ISO88591) ? 0x0FF : 0x7F;

       if ( revBytes )
       {
          while ( ( --maxLoopCnt >= 0 ) &&
                  ( (UCS4 = *source) <= maxCharToHandle ) )
          {
             source++;
             *target++ = UCS4 << 8;
          }
       }
       else
       {
          while ( ( --maxLoopCnt >= 0 ) &&
                  ( (UCS4 = *source) <= maxCharToHandle ) )
          {
             source++;
             *target++ = UCS4;
          }
       }
       translated_char_cnt = source - (unsigned char *)in_bufr ;
    }

    //
    // Slower path that handles all locales.
    //
    csc_mbtowc_funcPtr inputFuncPtr  ;
    csc_input_utfPtr   inputFuncPtr2 = NULL; 

    inputFuncPtr = csc_mbtowc_ptrs[ charset ];
    if ( ( inputFuncPtr == NULL ) && ( charset != cnv_ISO88591 ) )
    {
       inputFuncPtr2 = csc_input_utf_ptrs[ charset ];
       if ( inputFuncPtr2 == NULL )
          return( CNV_ERR_INVALID_CS ); // Shouldn't ever happen ...
    }

    while ( (source < endSource) &&
            (translated_char_cnt < max_chars_to_convert) ) {

      int UCS4 = 0;  // Init. in loop in case new char longer than prev one.

      if ( ( (UCS4 = *source) < 0x080 )  &&  // If ASCII and
           ( target < endTarget )        &&  // output buffer has space yet
           ( ! charsetIsWide    ) )
      {
         source++ ;
         if ( revBytes )
            UCS4 <<= 8;
         *target++ = UCS4;
         translated_char_cnt += 1;
      }
      else
      {
        size_t mblen ;
        int ct   = -1; // Init. - assume an error.
        unsigned char * tmpsrc = source;
        first_untranslated_char = (char *)source; //...in case char is bad

        if ( charset == cnv_ISO88591 ) {
           UCS4 = *source;
           mblen = 1;
        }
        else {
           if ( inputFuncPtr != NULL )
                mblen = (*inputFuncPtr)( (WChar_t *) &UCS4,
                                    (const char *)source, endSource - source,
                                    NULL );
           else {
                UCS4 = (*inputFuncPtr2)( &cd, &tmpsrc, endSource - source );
                if ( UCS4 < 0 ) mblen = -1;
                else {
                       mblen = tmpsrc - source;
                }
           }
        }
        if ( mblen == 0 ) /* mblen==0 when data is starts with '\0' */
             mblen = 1;
        if ( (mblen > 0) && (mblen < 0x7FFFFFFF) ) {
            if ( UCS4 < 0x0000D800 ) {  // If simple UCS2, just store it!
               if ( target < endTarget ) {
                  if ( revBytes )
                     UCS4 = ( ( UCS4 & 0x00FF ) << 8 ) |  ( UCS4 >> 8 ) ;

                  *target = UCS4;
                  ct = 2;
               }
               else ct = ERR_BUFFER_OVERRUN;
            }
            else {  // Not simple UCS2, so call routine that can handle it
               ct = __output_ucs2( &cd, (uchar_t *)target,
                               (endTarget - target)*sizeof(ucs2_t) , UCS4);
            }
        }
        else
            ct = mblen; /* Put error code in ct */

        if ( ct < 0 ) {
           // About to issue an error, so update return values first.

           SET_TRANSLATED_CHAR_CNT();
           SET_OUTPUT_DATA_LEN();

           if ( ct == ERR_BUFFER_OVERRUN )
               return CNV_ERR_BUFFER_OVERRUN;
           else
               return CNV_ERR_INVALID_CHAR;
        }
        target    += ct/sizeof( ucs2_t ) ;
        translated_char_cnt += 1;
        source    += mblen;
      }
    }
    first_untranslated_char = (char *) source;
    SET_TRANSLATED_CHAR_CNT();

    int rtnVal = 0;
    if ( addNullAtEnd_flag == TRUE ) {
        if ( target < endTarget ) {
           *target++ = 0;  // Store a 16-bit NULL
        }
        else {
           rtnVal = CNV_ERR_BUFFER_OVERRUN;
        }
    }
    SET_OUTPUT_DATA_LEN();
    return rtnVal;
}

//
//  LocaleToUTF8() - Convert a string of characters in the specified
//                    character set to UTF8.
//
int  LocaleToUTF8( const enum cnv_version version ,
                    const char *in_bufr ,  const int in_len ,
                    const char *out_bufr , const int out_len ,
                    enum cnv_charset charset ,
                    char * & first_untranslated_char ,
                    unsigned int *output_data_len_p ,
                    const int addNullAtEnd_flag ,
                    unsigned int * translated_char_cnt_p )
{
    if ( version != cnv_version1 )
        return CNV_ERR_INVALID_VERS;

    INITIALIZE_VARIABLES();

    unsigned char * target    = (unsigned char *) out_bufr;
    unsigned char * endTarget = target + out_len ;

    SET_OUTPUT_DATA_LEN();

    CHECK_FOR_SERIOUS_ERRORS();

    // We initialize a  _LC_fcconv_iconv_rec  struct here.
    // NOTE: For our purposes, the ONLY thing that 
    //       must be initialized is the flags word.
    //
    _LC_fcconv_iconv_rec  cd;

    cd.flags = CONV_BOM_WRITTEN | CONV_INPUT_PROCESSED ;

    //
    // Fast path where charset is ISO88591 or a multi-byte charset.
    // An assumption made here is that non-ASCII chars will rarely be seen.
    // If one is found, we break out of this fast path and go down the
    // slow path.
    //
    int charsetIsWide = 0;
    if ( (charset == cnv_UTF16) || (charset == cnv_UTF32) )
       charsetIsWide = 1 ;

    if ( ! charsetIsWide )
    {
       unsigned int UCS4 = 0;

       int  maxLoopCnt = endTarget - target ;
       if ( maxLoopCnt > in_len )
            maxLoopCnt = in_len ;

       while ( ( --maxLoopCnt >= 0) && ( (UCS4 = *source) < 0x080 ) )
       {
          source++ ;
          *target++ = UCS4;
       }
       translated_char_cnt = source - (unsigned char *)in_bufr ;
    }

    //
    // Slower path that handles all locales.
    //
    csc_mbtowc_funcPtr inputFuncPtr  ;
    csc_input_utfPtr   inputFuncPtr2 = NULL; 

    inputFuncPtr = csc_mbtowc_ptrs[ charset ];
    if ( ( inputFuncPtr == NULL ) && ( charset != cnv_ISO88591 ) )
    {
       inputFuncPtr2 = csc_input_utf_ptrs[ charset ];
       if ( inputFuncPtr2 == NULL )
          return( CNV_ERR_INVALID_CS ); // Shouldn't ever happen ...
    }

    while ( source < endSource ) {

      int UCS4 = 0;  // Init. in loop in case new char longer than prev one.

      if ( ( (UCS4 = *source) < 0x080 )  &&   // If ASCII and
           ( target < endTarget )        &&   // output buffer has space yet
           ( ! charsetIsWide ) )
      {
         source++ ;
         *target++ = UCS4;
         translated_char_cnt += 1;
      }
      else
      {
        size_t mblen ;
        int ct   = -1; // Init. - assume an error.
        unsigned char * tmpsrc = source;
        first_untranslated_char = (char *)source; //...in case char is bad

        if ( charset == cnv_ISO88591 ) {
           UCS4 = *source;
           mblen = 1;
        }
        else if ( inputFuncPtr != NULL )
           mblen = (*inputFuncPtr)( (WChar_t *) &UCS4,
                               (const char *)source, endSource - source,
                               NULL );
        else {
           UCS4 = (*inputFuncPtr2)( &cd, &tmpsrc, endSource - source );
           if ( UCS4 < 0 ) mblen = -1;
           else            mblen = tmpsrc - source;
        }

        if ( mblen == 0 ) /* mblen==0 when data is starts with '\0' */
             mblen = 1;
        if ( (mblen > 0) && (mblen < 0x7FFFFFFF) )
            ct = __output_utf8( &cd, target, endTarget - target , UCS4);
        else
            ct = mblen; /* Put error code in ct */

        if ( ct < 0 ) {
           // About to give an error, so update return values first.

           SET_TRANSLATED_CHAR_CNT();
           SET_OUTPUT_DATA_LEN();

           if ( ct == ERR_BUFFER_OVERRUN )
               return CNV_ERR_BUFFER_OVERRUN;
           else
               return CNV_ERR_INVALID_CHAR;
        }
        source    += mblen;
        target    += ct;
        translated_char_cnt += 1;
      }
    }
    first_untranslated_char = (char *) source;
    SET_TRANSLATED_CHAR_CNT();

    int rtnVal = 0;
    if ( addNullAtEnd_flag == TRUE ) {
        if ( target < endTarget ) {
           *target++ = 0;  // Store an 8-bit NULL
        }
        else {
           rtnVal = CNV_ERR_BUFFER_OVERRUN;
        }
    }
    SET_OUTPUT_DATA_LEN();
    return rtnVal;
}

//
// LocaleCharToUCS4() converts the FIRST char in the input string to its
// UCS4 value.  Returns the UCS4 value at location specified AND the
// length of the input character in bytes as the return value.
//
int  LocaleCharToUCS4( const char *in_bufr,       //Ptr to Input string
                       const int in_len,          //Len of Input string (bytes)
                       unsigned int *UCS4ptr ,    //Ptr to output location
                       enum cnv_charset charset )  //Locale Character Set
{
        ENSURE_VALID_CHARSET();
        ENSURE_VALID_INPUT();

        unsigned char * tmpsrc = (unsigned char *) in_bufr;
        size_t mblen ;
        int UCS4 = 0;

        // We initialize a  _LC_fcconv_iconv_rec  struct here.
        // NOTE: For our purposes, the ONLY thing that 
        //       must be initialized is the flags word.
        //
        _LC_fcconv_iconv_rec  cd;

        cd.flags = CONV_BOM_WRITTEN | CONV_INPUT_PROCESSED ;

        if ( charset == cnv_ISO88591 ) {
           UCS4 = *(unsigned char *)in_bufr;
           mblen = 1;
        }
        else {
           csc_mbtowc_funcPtr inputFuncPtr  ;
           csc_input_utfPtr   inputFuncPtr2 ; 

           inputFuncPtr = csc_mbtowc_ptrs[ charset ];
           if ( inputFuncPtr != NULL )
                mblen = (*inputFuncPtr)( (WChar_t *) &UCS4,
                                    (const char *)in_bufr, in_len, NULL );
           else {
                inputFuncPtr2 = csc_input_utf_ptrs[ charset ];
                if ( inputFuncPtr2 == NULL )
                   return( CNV_ERR_INVALID_CS ); // Shouldn't ever happen ...

                UCS4 = (*inputFuncPtr2)( &cd, &tmpsrc, in_len );
                if ( UCS4 < 0 ) mblen = -1;
                else            mblen = tmpsrc - (unsigned char *)in_bufr;
           }
        }

        if ( mblen == 0 ) /* mblen==0 when data is starts with '\0' */
             mblen = 1;
        if ( (mblen > 0) && (mblen < 0x7FFFFFFF) ) {
            if ( UCS4ptr != NULL )  *UCS4ptr = UCS4; // Return the UCS4 value
            return (mblen);
        }
        return (CNV_ERR_INVALID_CHAR);
}

//
// UCS4ToLocaleChar() converts the UCS4 value to the specified character set
// and stores the character in the output buffer specified.
// Returns length of the output character in bytes as the return value.
//
int  UCS4ToLocaleChar( const unsigned int *UCS4ptr , //Ptr to input char
                       const char *out_bufr,         //Ptr to output bufr
                       const int out_len,            //Len of output bufr
                       enum cnv_charset charset )    //Locale Character Set
{
        char tmpspace[8]; /* big enough to ensure no buffer overflow */

        ENSURE_VALID_CHARSET();

        char * target = (char *) out_bufr;
        int UCS4 = *UCS4ptr;
        int ct = -1;

        // We initialize a  _LC_fcconv_iconv_rec  struct here.
        // NOTE: For our purposes, the ONLY thing that 
        //       must be initialized is the flags word.
        //
        _LC_fcconv_iconv_rec  cd;

        cd.flags = CONV_BOM_WRITTEN | CONV_INPUT_PROCESSED ;

        if ( UCS4ptr == NULL )
            return CNV_ERR_NOINPUT;

        if ( charset == cnv_ISO88591 ) {
           if ( UCS4 <= 0x0FF ) { /* If valid ISO88591 char */
                tmpspace[0] = UCS4;
                ct = 1;
           }
        }
        else {
           csc_wctomb_funcPtr  outputFuncPtr  ;
           csc_output_utfPtr   outputFuncPtr2 ; 

           outputFuncPtr = csc_wctomb_ptrs[ charset ];
           if ( outputFuncPtr != NULL )
                ct = (int) (*outputFuncPtr)( tmpspace, (WChar_t) UCS4,
                                             (_LC_charmap_t *)NULL );
           else {
                outputFuncPtr2 = csc_output_utf_ptrs[ charset ];
                if ( outputFuncPtr2 == NULL )
                     return( CNV_ERR_INVALID_CS ); // Shouldn't ever happen ...

                ct = (*outputFuncPtr2)( &cd, (unsigned char *)(&tmpspace),
                                  sizeof(tmpspace), UCS4 );
           }
        }

        if ( ct < 0 )                // If Bad character or conversion error
                return (CNV_ERR_INVALID_CHAR);

        if ( ct <= out_len ) {
            if ( target != NULL ) {
                 char * tmpPtr = &tmpspace[0];
                 int iii = ct;
                 while (iii-- > 0 )
                      *target++ = *tmpPtr++;
            }
            return ( ct );
        }
        return CNV_ERR_BUFFER_OVERRUN;
}

//
// csc_get_subst_char() -- Get substitution char and its length
//
// Arguments:  substitution_char - pointer to user's specified char
//             tmpspace - pointer to caller's place to put the char
//             charset - an "enum cnv_charset" value indicating the
//                       target character set
//
// Return value: Length of substitution char in bytes
//
static int csc_get_subst_char( const char * substitution_char,
                               char * tmpspace,
                               enum cnv_charset charset )
{
    int sc_ln = 1;           //Default: 1-byte substitution char
    tmpspace[0] = '?';
    if ( substitution_char != NULL ) {
       if ( charset == cnv_UTF16 ) {
          sc_ln = 2;
          tmpspace[0] = substitution_char[0] ;
          tmpspace[1] = substitution_char[1] ;
       }
       else if ( charset == cnv_UTF32 ) {
          sc_ln = 4;
          tmpspace[0] = substitution_char[0] ;
          tmpspace[1] = substitution_char[1] ;
          tmpspace[2] = substitution_char[2] ;
          tmpspace[3] = substitution_char[3] ;
       }
       else {
          //
          // If 1st byte of substitution char string is 0, use '?'.
          // Else, if string is 1 byte long, use it as is.
          // Else, if string is 2 bytes long, use it as is.
          // Else use '?'.
          //
          if (   ( substitution_char[0] != 0 )     &&
               ( ( substitution_char[1] == 0 )     ||
                 ( substitution_char[2] == 0 ) ) ) {
               tmpspace[0] = substitution_char[0] ;
               tmpspace[1] = substitution_char[1] ;
               if ( tmpspace[1] != 0 )
                  sc_ln = 2;
          }
       }
    }
    return ( sc_ln );
}

int addVariableLengthNull( unsigned char * & target, 
                           unsigned char * endTarget,
                           int len_of_NULL )
{
   if ( len_of_NULL <= (endTarget - target) ) {
      if ( len_of_NULL >= 2 ) {
         *target++ = 0;
         if ( len_of_NULL == 4 ) {
            *target++ = 0; *target++ = 0;
         }
      }
      *target++ = 0;
      return 0;
   }
   return CNV_ERR_BUFFER_OVERRUN;
}

//
//  UTF16ToLocale() - Convert a string of UTF-16 characters 
//                    to the specified character set.
//
int  UTF16ToLocale( const enum cnv_version version ,
                    const char *in_bufr ,  const int in_len ,
                    const char *out_bufr , const int out_len ,
                    enum cnv_charset charset ,
                    char * & first_untranslated_char ,
                    unsigned int *output_data_len_p ,
                    const int cnv_flags ,
                    const int addNullAtEnd_flag ,
                    const int allow_invalids ,
                    unsigned int * translated_char_cnt_p ,
                    const char *substitution_char )
{
    if ( version != cnv_version1 )
        return CNV_ERR_INVALID_VERS;

    INITIALIZE_VARIABLES();

    unsigned char * target    = (unsigned char *)out_bufr;
    unsigned char * endTarget = target + out_len ;

    SET_OUTPUT_DATA_LEN();

    CHECK_FOR_SERIOUS_ERRORS();

    int len_of_NULL = 1;
    int ct = 0;

    // We initialize a  _LC_fcconv_iconv_rec  struct here.
    // NOTE: For our purposes, the ONLY thing that 
    //       must be initialized is the flags word.
    //
    _LC_fcconv_iconv_rec  cd;

    cd.flags = CONV_INPUT_PROCESSED | CONV_BOM_WRITTEN |
               ((cnv_flags && CNV_REVERSE_INBYTES) ? CONV_REVERSE_INBYTE : 0);

    //
    // Fast path where charset is ISO88591 or a multi-byte charset.
    // An assumption made here is that non-valid chars will rarely be seen.
    // If one is found, we break out of this fast path and go down the
    // slow path.
    //
    int charsetIsWide = 0;
    if ( (charset == cnv_UTF16) || (charset == cnv_UTF32) )
       charsetIsWide = 1 ;

    if ( ( ! charsetIsWide ) &&
         ( (cnv_flags & CNV_REVERSE_INBYTES) == 0 ) )
    {
       unsigned int UCS4    = 0;

       int  maxLoopCnt = endTarget - target ;
       if ( maxLoopCnt > (int) ( in_len / sizeof(ucs2_t) ) )
            maxLoopCnt = (int) ( in_len / sizeof(ucs2_t) ) ;

       unsigned int maxCharToHandle = (charset == cnv_ISO88591) ? 0x0FF : 0x7F;

       UCS4 = *( (ucs2_t *)source );
       if ( cnv_flags & CNV_REVERSE_INBYTES )
       {
          while ( --maxLoopCnt >= 0 )      // While more to do
          {
             UCS4 = *( (ucs2_t *)source ) ;
             UCS4 = ( ( UCS4 & 0x00FF ) << 8 ) |  ( UCS4 >> 8 ) ;
             if ( UCS4 <= maxCharToHandle )
             {
                source   += sizeof(ucs2_t);
                *target++ = UCS4;
             }
             else break;
          }
       }
       else while ( ( --maxLoopCnt >= 0 )   &&   // While more to do and
                    ( ( UCS4 = *( (ucs2_t *)source ) ) <= maxCharToHandle ) )
       {
          source   += sizeof(ucs2_t);
          *target++ = UCS4;
       }

       translated_char_cnt = target - (unsigned char *)out_bufr ;
    }

    //
    // Slower path that handles all locales.
    //
    csc_wctomb_funcPtr  outputFuncPtr  ;
    csc_output_utfPtr   outputFuncPtr2 = NULL; 

    outputFuncPtr = csc_wctomb_ptrs[ charset ];
    if ( ( outputFuncPtr == NULL ) && ( charset != cnv_ISO88591 ) )
    {
       outputFuncPtr2 = csc_output_utf_ptrs[ charset ];
       if ( outputFuncPtr2 == NULL )
          return( CNV_ERR_INVALID_CS ); // Shouldn't ever happen ...
    }

    while ( source < endSource ) {

      unsigned int UCS4 = *((ucs2_t *)source);

      if ( cnv_flags & CNV_REVERSE_INBYTES )
         UCS4 = ( ( UCS4 & 0x00FF ) << 8 ) |  ( UCS4 >> 8 ) ;

      if ( ( UCS4 < 0x080 )       &&  // If ASCII and
           ( target < endTarget ) &&  // there is space yet and
           ( ! charsetIsWide ) )      // output is not wide characters
      {
         source   += sizeof(ucs2_t);
         *target++ = UCS4;
         translated_char_cnt += 1 ;
      }
      else
      {
        char tmpspace[8]; /* big enough to ensure no buffer overflow */

        first_untranslated_char = (char *) source; //...in case char is bad

        if ( UCS4 < 0xD800 )  // If simple UCS2, use it as already retrieved
           source += sizeof(ucs2_t);
        else
           UCS4 = __input_ucs2( &cd, &source, endSource - source );

        ct = -1;
        if ( (UCS4 != ERR_INPUT_INCOMPLETE) && (UCS4 != ERR_INVALID_CHAR) ) {
           if ( charset == cnv_ISO88591 ) {
                if ( UCS4 <= 0x0FF ) {      // If valid ISO88591 char
                     tmpspace[0] = UCS4;
                     ct = 1;
                }
           }
           else {
              if ( outputFuncPtr != NULL )
                 ct = (*outputFuncPtr)( tmpspace, (WChar_t) UCS4,
                                  (_LC_charmap_t *)NULL);
              else {
                 ct = (*outputFuncPtr2)( &cd, (unsigned char *)(tmpspace),
                                   sizeof(tmpspace), UCS4 );

                 if ( charset == cnv_UTF16 )
                    len_of_NULL = 2;
                 else if ( charset == cnv_UTF32 )
                    len_of_NULL = 4;
              }
           }
        }

        if ( ct < 0 ) {  // If Bad character or conversion error
            if ( allow_invalids == FALSE ) {
                SET_TRANSLATED_CHAR_CNT();
                SET_OUTPUT_DATA_LEN();
                return (CNV_ERR_INVALID_CHAR);
            }

            ct = csc_get_subst_char( substitution_char, tmpspace , charset );

            if ( (UCS4 == ERR_INPUT_INCOMPLETE) || (UCS4 == ERR_INVALID_CHAR) )
                source += 2 ;   // Skip bad character
            //else source was already incremented by __input_ucs2()
        }
        if ( ct <= (endTarget - target) ) {
            char * tmpPtr = &tmpspace[0];
            while (ct-- > 0 )
                 *target++ = *tmpPtr++;
            translated_char_cnt += 1;
        }
        else {
            SET_TRANSLATED_CHAR_CNT();
            SET_OUTPUT_DATA_LEN();
            return CNV_ERR_BUFFER_OVERRUN;
        }
      }
    }
    first_untranslated_char = (char *) source;
    SET_TRANSLATED_CHAR_CNT();

    int rtnVal = 0;
    if ( addNullAtEnd_flag == TRUE ) {
       rtnVal = addVariableLengthNull( target, endTarget, len_of_NULL );
    }
    SET_OUTPUT_DATA_LEN();
    return rtnVal;
}
#define TWO_BYTE_UTF8(firstByte, src, nxtB) ( (((firstByte) & 0xE0) == 0xC0) && \
                                        (( (nxtB=(*((src)+1))) & 0xC0) == 0x80) )
//
//  UTF8ToLocale() - Convert a string of UTF8 characters 
//                    to the specified character set.
//
int  UTF8ToLocale( const enum cnv_version version ,
                    const char *in_bufr ,  const int in_len ,
                    const char *out_bufr , const int out_len ,
                    enum cnv_charset charset ,
                    char * & first_untranslated_char ,
                    unsigned int *output_data_len_p ,
                    const int addNullAtEnd_flag ,
                    const int allow_invalids ,
                    unsigned int * translated_char_cnt_p ,
                    const char *substitution_char )
{
    if ( version != cnv_version1 )
        return CNV_ERR_INVALID_VERS;

    INITIALIZE_VARIABLES();
 
    unsigned char * target    = (unsigned char *)out_bufr;
    unsigned char * endTarget = target + out_len ;

    SET_OUTPUT_DATA_LEN();

    CHECK_FOR_SERIOUS_ERRORS();

    int len_of_NULL = 1;
    int ct = 0;

    // We initialize a  _LC_fcconv_iconv_rec  struct here.
    // NOTE: For our purposes, the ONLY thing that 
    //       must be initialized is the flags word.
    //
    _LC_fcconv_iconv_rec  cd;

    cd.flags = CONV_BOM_WRITTEN | CONV_INPUT_PROCESSED ;

    //
    // Fast path where charset is ISO88591 or a multi-byte charset.
    // An assumption made here is that invalid chars will rarely be seen.
    // If one is found, we break out of this fast path and go down the
    // slow path.
    //
    int charsetIsWide = 0;
    if ( (charset == cnv_UTF16) || (charset == cnv_UTF32) )
       charsetIsWide = 1 ;

    if ( ! charsetIsWide )
    {
      unsigned int UCS4    = 0;

      int  maxLoopCnt = endTarget - target ;
      if ( maxLoopCnt > in_len )
           maxLoopCnt = in_len ;

      while ( --maxLoopCnt >= 0 )
      {
         // If character is valid ASCII
         if ( (UCS4 = *source) < 0x080 ) {
            source++;
            *target++ = UCS4;
         }
         else
         {
            if (charset != cnv_ISO88591)
               // Let slower path handle the rest of the buffer.
               break;

            int nxtByte = 0;
            if ( ( maxLoopCnt > 0 )  && TWO_BYTE_UTF8( UCS4, source, nxtByte ) )
            {
               // Convert from UTF8 to UCS4.
               UCS4 = (UCS4 & 0x1F) << 6 | ( nxtByte & 0x3F );
               if ( UCS4 > 0x0FF )
                  break; // Non-ISO88591.  Let slower path handle the rest.
               source   += 2 ;
               *target++ = UCS4;
               if ( maxLoopCnt > (int) ( endSource - source ) )
                    maxLoopCnt-- ;  // Ensure we don't overrun input buffer
            }
            else break; // Let slower path handle the rest.
         }
      }
      translated_char_cnt = target - (unsigned char *)out_bufr ;
    }
    else if ( charset == cnv_UTF16 )
    {
       unsigned int UCS4    = 0;
       while ( ( source < endSource )         &&  // more input and
               ( ( UCS4 = *source ) < 0x080 ) &&  // it is ASCII and
               ( (endTarget - target) >= 2  ) )   // there is space left
       {
          *((ucs2_t *)target) = UCS4;
          source++;
          target += sizeof(ucs2_t);
       }
       translated_char_cnt = source - (unsigned char *)in_bufr;
       len_of_NULL = 2;
    }

    //
    // Slower path that handles all locales.
    //
    csc_wctomb_funcPtr  outputFuncPtr  ;
    csc_output_utfPtr   outputFuncPtr2 = NULL; 

    outputFuncPtr = csc_wctomb_ptrs[ charset ];
    if ( ( outputFuncPtr == NULL ) && ( charset != cnv_ISO88591 ) )
    {
       outputFuncPtr2 = csc_output_utf_ptrs[ charset ];
       if ( outputFuncPtr2 == NULL )
          return( CNV_ERR_INVALID_CS ); // Shouldn't ever happen ...
    }

    while ( source < endSource ) {

      unsigned int UCS4 = *source;
      if ( ( UCS4 < 0x080 )       &&  // If ASCII and
           ( target < endTarget ) &&  // there is space yet and
           ( ! charsetIsWide ) )      // output is not wide characters
      {
         *target++ = UCS4;
         source++;
         translated_char_cnt += 1;
      }
      else
      {
        char tmpspace[8]; /* big enough to ensure no buffer overflow */

        first_untranslated_char = (char *) source; //...in case char is bad

        int UCS4 = __input_utf8( &cd, &source, endSource - source);

        ct = -1;
        if ( (UCS4 != ERR_INPUT_INCOMPLETE) && (UCS4 != ERR_INVALID_CHAR) ) {
           if ( charset == cnv_ISO88591 ) {
              if ( UCS4 <= 0x0FF )     {     // If valid ISO88591 char
                   tmpspace[0] = UCS4;
                   ct = 1;
              }
           }
           else {
              if ( outputFuncPtr != NULL )
                   ct = (*outputFuncPtr)( tmpspace, (WChar_t) UCS4, NULL );
              else {
                   ct = (*outputFuncPtr2)( &cd, (unsigned char *)(tmpspace),
                                           sizeof(tmpspace), UCS4);

                   if ( charset == cnv_UTF16 )
                      len_of_NULL = 2;
                   else if ( charset == cnv_UTF32 )
                      len_of_NULL = 4;
              }
           }
        }

        if ( ct < 0 ) {  // If Bad character or conversion error
            if ( allow_invalids == FALSE ) {
                SET_TRANSLATED_CHAR_CNT();
                SET_OUTPUT_DATA_LEN();
                return (CNV_ERR_INVALID_CHAR);
            }

            ct = csc_get_subst_char( substitution_char, tmpspace , charset );

            if ( (UCS4 == ERR_INPUT_INCOMPLETE) || (UCS4 == ERR_INVALID_CHAR) )
                source += 1 ;   // Skip bad character
            //else source was already incremented by __input_utf8()
        }
        if ( ct <= (endTarget - target) ) {
            char * tmpPtr = &tmpspace[0];
            translated_char_cnt += 1;
            while (ct-- > 0 )
                 *target++ = *tmpPtr++;
        }
        else {
           SET_TRANSLATED_CHAR_CNT();
           SET_OUTPUT_DATA_LEN();
           return CNV_ERR_BUFFER_OVERRUN;
        }
      }
    }
    first_untranslated_char = (char *) source;
    SET_TRANSLATED_CHAR_CNT();

    int rtnVal = 0;
    if ( addNullAtEnd_flag == TRUE ) {
       rtnVal = addVariableLengthNull( target, endTarget, len_of_NULL );
    }
    SET_OUTPUT_DATA_LEN();
    return rtnVal;
}


int lightValidateUTF8Str(const char *bufr,
                         int in_len,
                         int max_chars,
                         int ignore_trailing_blanks)
{
  unsigned char c;
  int pos  = 0;
  int numc = 0;
  int maxc = ( max_chars ? max_chars : in_len );
  int byte = 1;
  int last_good_pos = 0;

  if ( (in_len < 0) || (max_chars < 0) ) // Defensive programming: Ensure no memory access exceptions.
    return -1;                           // Shouldn't ever happen, of course.

  while (pos < in_len && numc < maxc)
    {
      c = bufr[pos];

      if (c < 0x80 && byte == 1) // ascii
        numc++;
      else if (c >= 0x80 && c < 0xc0 && byte > 1) // second, third, or fourth byte of a multi-byte sequence 
        {
          if (--byte == 1)
            numc++;
        }
      else if (c >= 0xc0 && c < 0xe0 && byte == 1) // start of 2-byte sequence
        byte = 2;
      else if (c >= 0xe0 && c < 0xf0 && byte == 1) // start of 3-byte sequence
        byte = 3;
      else if (c >= 0xf0 && c < 0xfc && byte == 1) // start of 4-byte sequence
        byte = 4;
      else
        return -1; // invalid byte sequence

      pos++;
    }

  if (byte == 1 && numc <= maxc)
    return pos; // string is valid and has valid char count, pos == in_len

  // We encountered too many characters or a partial character. The string
  // bufr[0..pos-1] contains numc entire characters and maybe one partial character.

  // check whether the extra characters are all blanks and it's safe to ignore them
  if (ignore_trailing_blanks && byte == 1)
    {
      int blankPos = pos-1; // the previous character is already past the char. limit

      while (blankPos < in_len && bufr[blankPos] == ' ')
        blankPos++;

      if (blankPos >= in_len)
        return in_len; // extra chars were all blanks
    }

  // back up until the end of the valid characters

  while (byte > 1 || numc > maxc)
    {
      pos--;
      c = bufr[pos];

      if (c < 0x80 || c >= 0xc0)
        {
          // this is the first byte of a character
          if (byte > 1)
            byte = 1;
          else
            numc--;
        }
    }

  return pos; // string needs to be truncated at position "pos" (to length "pos")
}

#if 0 /* Not currently called anywhere.*/
int lightValidateUTF8StrAndPad(char *bufr,
                               int in_len,
                               int max_chars,
                               int ignore_trailing_blanks)
{
  int trunc = lightValidateUTF8Str(bufr, in_len, max_chars, ignore_trailing_blanks);

  if (trunc < in_len && trunc >= 0)
    {
      for (int i=trunc; i<in_len; i++)
        bufr[i] = ' ';
    }

  return trunc;
}
#endif /* Not currently called anywhere.*/
