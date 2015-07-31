/**************************************************************************
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
**************************************************************************/

#ifndef __CHARSETCONVH
#define __CHARSETCONVH

#include "windows.h"
typedef bool    *LPBOOL;

/* defines needed to mimic MultiByteToWideChar() and WideCharToMultiByte() on *nix */
#define CP_ACP                    0U           // default to ANSI code page
#define CP_UTF8                   65001U       // UTF-8 translation
#define CP_ISO88591                  65002U       // For ISO88591

#define MB_ERR_INVALID_CHARS      0x00000008  // error for invalid chars

#ifndef ERROR_INSUFFICIENT_BUFFER
//#define ERROR_INSUFFICIENT_BUFFER        122L // defined in windows.h
#endif
#define ERROR_NO_UNICODE_TRANSLATION     1113L

#if defined (MXHPUXIA) || (MXHPUXPA)
#define TMP_ALINGNED_BUFSIZE			98304 //32K * 3
#endif

/* ICU headers */
#include "unicode/utypes.h"   /* Basic ICU data types */
#include "unicode/ucnv.h"     /* C   Converter API    */
#include "unicode/ustring.h"  /* some more string fcns*/
#include "unicode/uchar.h"    /* char names           */
#include "unicode/uloc.h"
#include "unicode/unistr.h"
#include "sqlcli.h"

#ifdef __TEST_ICUCONVERTER

/* 
 * some defines if we need to test the ICU conversion routines standalone
 * when building the driver, these defines are already set in ./dependencies/windows/windows.h
 */

typedef unsigned int DWORD;
typedef unsigned int UINT;
typedef unsigned short WCHAR;
typedef WCHAR   *LPWSTR;
//typedef WCHAR    const *LPCWSTR;

#define LPCSTR   const char *
#define LPSTR    char *

#endif /* __TEST_ICUCONVERTER */

typedef enum APP_UNICODE_TYPE
{
   APP_UNICODE_TYPE_UTF8  = 0,
   APP_UNICODE_TYPE_UTF16 = 1
} APP_UNICODE_TYPE;

typedef enum APP_TYPE
{
	APP_TYPE_NONE  = -1,
	APP_TYPE_UNICODE  = 0,
	APP_TYPE_ANSI = 1
} APP_TYPE;

typedef enum CASE_TYPE
{
	CASE_NONE  = -1,
	CASE_NO = 0,
	CASE_YES  = 1
} CASE_TYPE;



const short UCharNull = 0;

class ICUConverter {

public:
	static int m_AppType;
	static int m_AppUnicodeType;  //  UTF-8 or UTF-16  => Not assigned. 
	bool m_Connection;
	bool m_iso88591Translation;
public:

	ICUConverter();
	void setClientLocale(const char *cp_acp);

        ~ICUConverter();

	bool setReplacementChar(char *replacementchar,int size, bool IsoMappingConv=false);
	inline bool isAppUTF16(){if (m_AppUnicodeType > 0) return true; else return false;};
	inline bool isIso88591Translation(){return m_iso88591Translation;};
	inline void setUCS2Translation(BOOL inVal) {if(inVal == FALSE) m_UCS2Translation = false;};
	inline bool getUCS2Translation() {return m_UCS2Translation;};

    int UTF8ToFromMultiByte( int direction,
                             DWORD dwFlags,
                             const char *inputStr,
                             int  inputSize,
                             char *outputStr,
                             int outputSize,
                             DWORD &lastError,
							 int *charCount = NULL );

    int CharsetToCharset( UConverter* convFrom, 
					UConverter* convTo,
                                       const char *inputStr,
                                       int  inputSize,
                                       char *outputStr,
                                       int outputSize,
                                       DWORD &lastError );

	int MultiByteToWideChar(UINT CodePage,  
				DWORD dwFlags,
  				LPCSTR lpMultiByteStr,
  				int cbMultiByte,
  				LPWSTR lpWideCharStr,
  				int cchWideChar,
  				DWORD &lastError );

    int WideCharToMultiByte(int CodePage,  
                            int dwFlags,
                            const UChar* lpWideCharStr,
                            int cchWideChar,
                            LPSTR lpMultiByteStr,
                            int cbMultiByte,
                            LPCSTR lpDefaultChar,
                            LPBOOL lpUsedDefaultChar,
                            DWORD &lastError );

	SQLRETURN WCharToUTF8(UChar *wst, int wstlen, char *st, int stlen, int *translen, 
							char* error, DWORD dwFlags=MB_ERR_INVALID_CHARS, int* reqLen=NULL);
	SQLRETURN UTF8ToWChar(char *st, int stlen, UChar *wst,  int wstlen, int *translen,
			       				char* error, DWORD dwFlags=MB_ERR_INVALID_CHARS, int* reqLen=NULL);

	SQLRETURN WCharToISO88591(UChar *wst, int wstlen, char *st, int stlen, int *translen, 
							char* error, DWORD dwFlags=MB_ERR_INVALID_CHARS, int* reqLen=NULL);
	SQLRETURN ISO88591ToWChar(char *st, int stlen, UChar *wst,  int wstlen, int *translen,
			       				char* error, DWORD dwFlags=MB_ERR_INVALID_CHARS, int* reqLen=NULL);

/*
	SQLRETURN UTF8ToISO88591(char *inst, int inlen, char *outst, int outlen, SQLINTEGER *translen, 
								char *errorMsg, DWORD dwFlags=MB_ERR_INVALID_CHARS);
	SQLRETURN ISO88591ToUTF8(char *inst, int inlen, char *outst, int outlen, SQLINTEGER *translen, 
										char *errorMsg, DWORD dwFlags=0);
*/
       //Translate to and from utf8/locale
	SQLRETURN TranslateUTF8(bool forward, char *inst, int inlen, char *outst, int outlen, SQLINTEGER *translen,
		       											char *errorMsg, int *charCount = NULL);

       //Translate to and from iso88951/locale
	SQLRETURN TranslateISO88591(bool forward, char *inst, int inlen, char *outst, int outlen, SQLINTEGER *translen, char *errorMsg, DWORD dwFlags=MB_ERR_INVALID_CHARS);

	SQLRETURN UTF8ToFromISO88591(bool forward, char *inst, int inlen, char *outst, int outlen, SQLINTEGER *translen,
		       							char *errorMsg, DWORD dwFlags=MB_ERR_INVALID_CHARS);

	SQLRETURN WCharToLocale(UChar *wst, int wstlen, char *st, int stlen, int *translen,
							char* error = NULL, char *replacementChar = NULL);
	SQLRETURN LocaleToWChar(char *st, int stlen, UChar *wst, int wstlen,  int *translen, 	
											char* error = NULL);                             
											

	//Returns lenth in bytes, if input arg is utf8/locale 
	SQLINTEGER FindStrLength(const char* str, SQLINTEGER strLen);
	
	
	SQLCHAR* AllocCharBufferHelper(	SQLCHAR* inputArg,
									SQLINTEGER inputArgLength, /*in */
									bool isWideCall,  /* in */
									SQLINTEGER* bufferLength );
	
	SQLRETURN InputArgToUTF8Helper(	SQLCHAR* inputArg,	 /*in */
				 				SQLINTEGER inputArgLength, /*in */
				 				SQLCHAR* u8_inputArg, /* in/out Allocated by Caller */
				 				SQLINTEGER u8_inputArgLength, /*in  length of u8_inputArg*/
				 				SQLINTEGER* u8_inputArgTransLength, /*in/out */
				 				bool isWideCall,  /* in */
				 				char* errorMsg /* out */	);
						 	
	SQLRETURN OutputArgFromUTF8Helper(	SQLCHAR* u8_outputArg,	 /*in */
						 		SQLINTEGER u8_outputArgLength, /*in */
						 		SQLCHAR* outputArg, /*in/out  App specified buffer*/
						 		SQLINTEGER outputArgLength, /*in  App specified buffer length, length of outputArg*/  	
						 		SQLINTEGER* outputArgTransLength, /*in/out The actual translated length */
								bool isWideCall,  /* in */
								char* errorMsg /* out */	);

	SQLRETURN InputArgToWCharHelper(SQLCHAR* inputArg, SQLINTEGER inputArgLength,
			UChar* dest, int destLen, int* translatedLen, char* errorMsgi, bool truncate = false);
								
	void NullTerminate(char* ValuePtr);
	
	SQLRETURN InArgTranslationHelper(SQLCHAR* arg, unsigned int argLen, char * dest,
		unsigned int destLen, int* transLen, char* errorMsg, bool internalData=false, int caseSensitivity=CASE_NONE, bool trucate=false);
                                        
	SQLRETURN OutArgTranslationHelper(SQLCHAR* arg, int argLen, char* dest, 
			int destLen, int *transLen, int *charCount, char *errorMsg, bool LengthInUChars = false);
	
	char* strcpyUTF8(char *dest, const char *src, size_t destSize, size_t copySize);
private:

        // converter will use the replacement character
	UConverter *cp_acp_replace ; 
	UConverter *cp_utf8_replace;
	UConverter *cp_utf16_replace;
	UConverter *cp_iso88591_replace;

        // converter will stop on conversion errors
	UConverter *cp_acp_error; 
	UConverter *cp_utf8_error;
	UConverter *cp_utf16_error;
	UConverter *cp_iso88591_error;

	bool m_stdConfiguration;
	bool bLocaleTranslation; 
	bool m_UCS2Translation;
 
};

#ifndef unixcli
/* ifndef unixcli */
typedef wchar_t UChar 
#endif 

#include <assert.h>

#endif /* ifndef __CHARSETCONVH */
