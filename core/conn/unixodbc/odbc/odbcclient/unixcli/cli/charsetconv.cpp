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


#include "charsetconv.h"


/*
 *
 * class constructor
 *
 * Parameters:
 *
 * (in)  CodePage : Name of the coded character set table.
 *  
 *  Use the standard IANA names:
 *  See: http://www.iana.org/assignments/character-sets
 *
 *  -or-
 *
 *  Use any of the ICU internal converter names, or any of the aliases it supports
 *  See: http://demo.icu-project.org/icu-bin/convexp for the list of internal
 *  converter names
 *                
 *
 *  The charset should be specified in the MXODSN/odbc.ini or as part of the connection string
 * 
 */

 //Static members initialization
 int ICUConverter::m_AppUnicodeType = -1 ;
 
 int ICUConverter::m_AppType = -1 ;
 
 ICUConverter::ICUConverter()
 {
	 UErrorCode Uerr = U_ZERO_ERROR;
	 
	//UTF-8 converter
	cp_utf8_replace = ucnv_open("utf8", &Uerr);
	assert(U_SUCCESS(Uerr));

	cp_utf8_error = ucnv_open("utf8", &Uerr);
	assert(U_SUCCESS(Uerr));

    ucnv_setToUCallBack( cp_utf8_error,
                         UCNV_TO_U_CALLBACK_STOP,
                         NULL,
                         NULL,
                         NULL,
                         &Uerr);

	assert(U_SUCCESS(Uerr));

   /*
    * enable the fallback translations
    */
    ucnv_setFallback(cp_utf8_error,TRUE);
    ucnv_setFallback(cp_utf8_replace,TRUE);

	//(Active) Code Page converter
	cp_acp_replace  = ucnv_open("ISO-8859-1", &Uerr);
	assert(U_SUCCESS(Uerr));

	cp_acp_error  = ucnv_open("ISO-8859-1", &Uerr);
	assert(U_SUCCESS(Uerr));

	ucnv_setToUCallBack( cp_acp_error,
                         UCNV_TO_U_CALLBACK_STOP,
                          NULL,
                          NULL,
                          NULL,
                         &Uerr);
	assert(U_SUCCESS(Uerr));
   /*
    * enable the fallback translations
    */
	ucnv_setFallback(cp_acp_error,TRUE);
	ucnv_setFallback(cp_acp_replace,TRUE);

	//ISO88591 Code Page converter
	cp_iso88591_replace  = ucnv_open("ISO-8859-1", &Uerr);
	assert(U_SUCCESS(Uerr));

	cp_iso88591_error  = ucnv_open("ISO-8859-1", &Uerr);
	assert(U_SUCCESS(Uerr));

	ucnv_setToUCallBack( cp_iso88591_error,
                         UCNV_TO_U_CALLBACK_STOP,
                          NULL,
                          NULL,
                          NULL,
                         &Uerr);
	assert(U_SUCCESS(Uerr));
   /*
    * enable the fallback translations
    */
	ucnv_setFallback(cp_iso88591_error,TRUE);
	ucnv_setFallback(cp_iso88591_replace,TRUE);
	
// set m_UCS2Translation to true by default
	m_UCS2Translation = true ;
       m_iso88591Translation = false;
 }
 
 
void ICUConverter::setClientLocale(const char *cp) 
{
	UErrorCode Uerr = U_ZERO_ERROR;
       // if ClientCharSet is "ISO-8859-1" , we already have the converter
       if(stricmp(cp,"ISO-8859-1") != 0)
       {
         //close the existing "ISO-8859-1" converters and create new one
	 ucnv_close(cp_acp_replace);
	 ucnv_close(cp_acp_error);

	 //(Active) Code Page converter
	 cp_acp_replace  = ucnv_open(cp, &Uerr);
	 assert(U_SUCCESS(Uerr));

	 cp_acp_error  = ucnv_open(cp, &Uerr);
	 assert(U_SUCCESS(Uerr));

         ucnv_setToUCallBack( cp_acp_error,
                         UCNV_TO_U_CALLBACK_STOP,
                          NULL,
                          NULL,
                          NULL,
                         &Uerr);
	 assert(U_SUCCESS(Uerr));

      /*
      * enable the fallback translations
      */
       ucnv_setFallback(cp_acp_error,TRUE);
       ucnv_setFallback(cp_acp_replace,TRUE);
    }
  //Do we need to translate iso88591 data that goes to iso88591 columns ?
  if (ucnv_compareNames(ucnv_getName(cp_acp_error, &Uerr), ucnv_getName(cp_iso88591_error, &Uerr)) == 0)
	m_iso88591Translation = false;
  else
 	m_iso88591Translation = true;
}
/* ICUConverter::setICUConverter() */

/*
 *
 * class destructor
 * 
 */

ICUConverter::~ICUConverter() 
{
	ucnv_close(cp_acp_replace);
	ucnv_close(cp_acp_error);

	ucnv_close(cp_utf8_replace);
	ucnv_close(cp_utf8_error);

	ucnv_close(cp_iso88591_replace);
	ucnv_close(cp_iso88591_error);
	
} /* ICUConverter::~ICUConverter() */

/*
 *
 * set the replacement character for conversion from utf16 to the client code page
 *
 *  for conversions to utf16 the only replacement possible is U+fffd and that too only 
 *  if the callback has not been set to UCNV_TO_U_CALLBACK_STOP.
 *  if the callback has been set to UCNV_TO_U_CALLBACK_STOP, conversion will stop with an error
 *
 * Parameters:
 * (in) replacementchar: the substitution character byte sequence we want set
 * (in) size           : the number of bytes in subChars
 *
 * return: true if the set was successful the error status code. false otherwise
           (typically U_INDEX_OUTOFBOUNDS_ERROR is set if size is bigger than the maximum number of bytes allowed in subchars)

 *  The substitution is specified as a string of 1-4 bytes, and may contain NULL bytes. The replacement must represent a single character. 
 *  The caller needs to know the byte sequence of a valid character in the converter's charset.
 */

bool ICUConverter::setReplacementChar(char *replacementchar,int size, bool IsoMappingConv)
{

	UErrorCode Uerr = U_ZERO_ERROR;

	if(!IsoMappingConv)
	{
		ucnv_setSubstChars(cp_acp_replace,replacementchar,size,&Uerr);
	    	if(U_FAILURE(Uerr))
		   return false;
	}
	else //IsoMapping Converter
	{
		ucnv_setSubstChars(cp_iso88591_replace,replacementchar,size,&Uerr);
    		if(U_FAILURE(Uerr))
			return false;
	}
    	return true;

} /* ICUConverter::setReplacementChar() */


/*
 * UTF8ToFromMultiByte()
 * 
 * Parameters:
 *
 * (in)  direction      : 1 for conversion from UTF8 to Code Page --or--  0 for conversion from Code Page to UTF8
 * (in)  dwFlags        : either 0 or MB_ERR_INVALID_CHARS
 * (in)  inputStr       : input utf-8 string
 * (in)  inputSize      : length of the input string (in bytes)
 * (out) outputStr      : output character string
 * (in)  outputSize     : size in bytes of the output character string buffer
 * (out) lastError      : returns last error (instead of the GetLastError() call on windows)
 * (out) charCount		: returns the number of characters that been translated.
 *
 * Returns: 0 on error (lastError is set) otherwise returns the output string length
 *
 */
int ICUConverter::UTF8ToFromMultiByte( int direction,
                                       DWORD dwFlags,
                                       const char *inputStr,
                                       int  inputSize,
                                       char *outputStr,
                                       int outputSize,
                                       DWORD &lastError,
									   int *charCount )
{

    UConverter *convFrom, *convTo, *convToU16;
    UErrorCode Uerr = U_ZERO_ERROR;
    int32_t len = 0;
    char *target = outputStr;

    lastError = 0;

    if(direction == 1)
	{
	   // conversion from UTF8 to code page
       if(dwFlags == MB_ERR_INVALID_CHARS)
       {
          convFrom = cp_utf8_error;
          convTo = cp_acp_error;
       }
       else
       {
          convFrom = cp_utf8_replace;
          convTo = cp_acp_replace;
       }
	}
	else
	{
	   // conversion from code page to UTF8
       if(dwFlags == MB_ERR_INVALID_CHARS)
       {
	      convFrom = cp_acp_error;
          convTo = cp_utf8_error;
       }
       else
       {
          convFrom = cp_acp_replace;
          convTo = cp_utf8_replace;
       }
	}
	convToU16 = convTo;

	// if source charset is same as destination charset, not need to translate - simply do copy
	if (ucnv_compareNames(ucnv_getName(convFrom, &Uerr), ucnv_getName(convTo, &Uerr)) == 0)
	{
		if (inputSize >= outputSize)
		{
			// A result would not fit in the supplied buffer
			lastError = ERROR_INSUFFICIENT_BUFFER;
			return 0; // len=0 now, should we return required buffer size?
		}
		memcpy(outputStr, inputStr, inputSize);
		outputStr[inputSize]=0;
        if (charCount)
        {
            *charCount = inputSize;
        }
		return inputSize;
	}
	
    ucnv_convertEx( convTo,
                    convFrom,
                   &target,
                    outputStr + outputSize,
                   &inputStr,
                    inputStr + inputSize,
                    NULL,NULL,NULL,NULL,
                    TRUE,TRUE, &Uerr);

	if(Uerr != U_ZERO_ERROR) {
		switch(Uerr)
		{
			case U_STRING_NOT_TERMINATED_WARNING:
				//	An output string could not be NUL-terminated because output length==destCapacity (ignore)
			case U_BUFFER_OVERFLOW_ERROR:
				// A result would not fit in the supplied buffer
				lastError = ERROR_INSUFFICIENT_BUFFER;
				break;

			case U_INVALID_CHAR_FOUND:
				// Character conversion: Unmappable input sequence (Invalid character)
			case U_ILLEGAL_CHAR_FOUND:
				// Character conversion: Illegal input sequence/combination of input units
			case U_TRUNCATED_CHAR_FOUND:
				// Character conversion: Incomplete input sequence (example just one part of surrogate pair is in the input sequence)
				lastError = ERROR_NO_UNICODE_TRANSLATION;
				break;

			default:
				lastError = Uerr;
		}
	}
	else
		len = (int32_t)(target-outputStr);

	if (charCount && (len != 0))
	{
		*charCount = ucnv_toUChars(convToU16, NULL, 0, outputStr, -1, &Uerr);
	}

    return len;

} // ICUConverter::UTF8ToFromMultiByte()


/*
 * CharsetToCharset()
 *
 * Note: This function can NOT be used for
 * 	locale <-> UTF-16
 * 	UTF-8  <-> UTF-16
 * 
 * Parameters:
 *
 * (in)  convFrom       : UConverter for the source
 * (in)  convTo	       :  UConverter for the target
 * (in)  dwFlags        : either 0 or MB_ERR_INVALID_CHARS
 * (in)  inputStr       : input utf-8 string
 * (in)  inputSize      : length of the input string (in bytes)
 * (out) outputStr      : output character string
 * (in)  outputSize     : size in bytes of the output character string buffer
 * (out) lastError      : returns last error (instead of the GetLastError() call on windows)
 *
 * Returns: 0 on error (lastError is set) otherwise returns the output string length
 *
 */
int ICUConverter::CharsetToCharset( UConverter *convFrom, 
					UConverter *convTo,
                                       const char *inputStr,
                                       int  inputSize,
                                       char *outputStr,
                                       int outputSize,
                                       DWORD &lastError )
{

    UErrorCode Uerr = U_ZERO_ERROR;
    int32_t len = 0;
    char *target = outputStr;

    lastError = 0;

	// if source charset is same as destination charset, not need to translate - simply do copy
	if (ucnv_compareNames(ucnv_getName(convFrom, &Uerr), ucnv_getName(convTo, &Uerr)) == 0)
	{
		if (inputSize >= outputSize)
		{
			// A result would not fit in the supplied buffer
			lastError = ERROR_INSUFFICIENT_BUFFER;
			return 0; // len=0 now, should we return required buffer size?
		}
		memcpy(outputStr, inputStr, inputSize);
		outputStr[inputSize]=0;
		return inputSize;
	}
	
    ucnv_convertEx( convTo,
                    convFrom,
                   &target,
                    outputStr + outputSize,
                   &inputStr,
                    inputStr + inputSize,
                    NULL,NULL,NULL,NULL,
                    TRUE,TRUE, &Uerr);

	if(U_FAILURE(Uerr)) {
		switch(Uerr)
		{
			case U_STRING_NOT_TERMINATED_WARNING:
			// 	An output string could not be NUL-terminated because output length==destCapacity (ignore)
			break;

			case U_BUFFER_OVERFLOW_ERROR:
			// A result would not fit in the supplied buffer
			lastError = ERROR_INSUFFICIENT_BUFFER;
			break;

			case U_INVALID_CHAR_FOUND:
			// Character conversion: Unmappable input sequence (Invalid character)
			case U_ILLEGAL_CHAR_FOUND:
			// Character conversion: Illegal input sequence/combination of input units
			case U_TRUNCATED_CHAR_FOUND:
			// Character conversion: Incomplete input sequence (example just one part of surrogate pair is in the input sequence)
			lastError = ERROR_NO_UNICODE_TRANSLATION;
			break;

			default:
			lastError = Uerr;
		}
	}
	else
	   len = (int32_t)(target-outputStr);

    return len;

} // ICUConverter::UTF8ToFromMultiByte()


/*
 *
 * MultiByteToWideChar() 
 *
 * Parameters:
 *
 * (in)  CodePage       : Either CP_ACP(for conversion to client locale) or CP_UTF8 (for conversion to UTF8) or CP_ISO88591
 * (in)  dwFlags        : this is either MB_ERR_INVALID_CHARS (to stop conversion with an error when invalid chars are detected)
 *                        or 0 - to continue with the conversion (in which case the default replacement character is used U+fffd
 * (in ) lpMultiByteStr : input character string
 * (in)  cbMultiByte    : size in bytes of the input character string buffer
 * (out) lpWideCharStr  : output utf-16 string
 * (in)  cchWideChar    : length of the output widechar buffer (note: not in bytes - but number of wide chars ex: from wstrlen()/u_strlen())
 * (out) lastError      : returns last error (instead of the GetLastError() call on windows)
 *
 * Returns:
 * the length of the output string, not counting the terminating NUL; 
 * if the length is greater than destination Capacity it will return the actual lenght of the buffer that is needed
 *
 * Notable difference from the windows version:
 *
 * (1) When  ERROR_INSUFFICIENT_BUFFER is returned, the return value contains the length of the actual Wide Char buffer needed 
 *     on windows you would need to call MultiByteToWideChar() with cchWideChar = 0, to get this information.
 *     ->> so do check for lastError and don't depend on the return value = 0 to test for errors
 *
 * (2) The wide character buffer will automatically be null terminated (if space is available in the buffer)
 *     (the return value does not consider this null that the conversion routine adds)
 *
 * (3) lastError is an additional parameter which returns the error
 *
 *
 */
int ICUConverter::MultiByteToWideChar(	UINT CodePage,  
                                        DWORD dwFlags,
  					                    LPCSTR lpMultiByteStr,
  					                    int cbMultiByte,
  					                    LPWSTR lpWideCharStr,
  					                    int cchWideChar,
  					                    DWORD &lastError )
{
	UConverter *conv;
	UErrorCode Uerr = U_ZERO_ERROR;
	int32_t len;

	lastError = 0;

	if(CodePage == CP_ACP) 
        {
	   if(dwFlags == MB_ERR_INVALID_CHARS)
	      conv = cp_acp_error;
           else
              conv = cp_acp_replace;
        }
	else if (CodePage == CP_UTF8)
	{
	   if(dwFlags == MB_ERR_INVALID_CHARS)
		conv = cp_utf8_error;
            else
                conv = cp_utf8_replace;
	}
	else if (CodePage == CP_ISO88591)
	{
	   if(dwFlags == MB_ERR_INVALID_CHARS)
		conv = cp_iso88591_error;
            else
                conv = cp_iso88591_replace;
	}
	else
	    // we don't need anything besides the above two
	    assert(0);

	len = ucnv_toUChars(conv, (UChar*)lpWideCharStr, cchWideChar, lpMultiByteStr, cbMultiByte, &Uerr);

	if(U_FAILURE(Uerr)) {
		switch(Uerr)
		{
			case U_STRING_NOT_TERMINATED_WARNING:
			// 	An output string could not be NUL-terminated because output length==destCapacity (ignore)
			break;

			case U_BUFFER_OVERFLOW_ERROR:
			// A result would not fit in the supplied buffer
			lastError = ERROR_INSUFFICIENT_BUFFER;
			break;

			case U_INVALID_CHAR_FOUND:
			// Character conversion: Unmappable input sequence (Invalid character)
			case U_ILLEGAL_CHAR_FOUND:
			// Character conversion: Illegal input sequence/combination of input units
			case U_TRUNCATED_CHAR_FOUND:
			// Character conversion: Incomplete input sequence (example just one part of surrogate pair is in the input sequence)
			lastError = ERROR_NO_UNICODE_TRANSLATION;
			break;

			default:
			lastError = Uerr;
		}
	}

	return len; // does not count the terminating null character

} // ICUConverter::MultiByteToWideChar()

/*
 *
 * WideCharToMultiByte()
 *
 * Parameters:
 *
 * (in)  CodePage       : Either CP_ACP(for conversion to client locale) or CP_UTF8 (for conversion to UTF8)
 * (in)  dwFlags        : this is either MB_ERR_INVALID_CHARS (to stop conversion with an error when invalid chars are detected)
 *                        or 0 - to continue with the conversion (in which case the default replacement character is used U+fffd
 * (in)  lpWideCharStr  : input utf-16 string
 * (in)  cchWideChar    : length of the input string (note: not in bytes - but number of wide chars ex: from wstrlen()/u_strlen()
 * (out) lpMultiByteStr : output character string
 * (in)  cbMultiByte    : size in bytes of the output character string buffer
 * (in)  lpDefaultChar  : unused - call the setReplacementChar() method to set it
 * (out) lpUsedDefaultChar :unused (did not see it being used in the windows driver)
 * (out) lastError      : returns last error (instead of the GetLastError() call on windows)
 *
 *
 * Notable difference from the windows version:
 *
 * (1) When  ERROR_INSUFFICIENT_BUFFER is returned, the return value contains the length of the actual Wide Char buffer needed 
 *     on windows you would need to call MultiByteToWideChar() with cchWideChar = 0, to get this information.
 *
 * (2) The wide character buffer will automatically be null terminated (if space is available in the buffer)
 *     (the return value does not consider this null that the conversion routine it adds)
 *
 * (3) lastError is an additional parameter
 *
 *
 */
int ICUConverter::WideCharToMultiByte(	INT CodePage,  
                                        INT dwFlags,
                                        const UChar* lpWideCharStr,
                                        int cchWideChar,
                                        LPSTR lpMultiByteStr,
                                        int cbMultiByte,
                                        LPCSTR lpDefaultChar,
                                        LPBOOL lpUsedDefaultChar,
                                        DWORD &lastError )
{
	UConverter *conv;
	UErrorCode Uerr = U_ZERO_ERROR;
	int32_t len;

	lastError = 0;

        if(CodePage == CP_ACP)
        {
           if(dwFlags == MB_ERR_INVALID_CHARS)
              conv = cp_acp_error;
           else
              conv = cp_acp_replace;
        }
        else if (CodePage == CP_UTF8)
        {  
           if(dwFlags == MB_ERR_INVALID_CHARS)
                conv = cp_utf8_error;
            else
                conv = cp_utf8_replace;
        }
        else if (CodePage == CP_ISO88591)
        {  
           if(dwFlags == MB_ERR_INVALID_CHARS)
                conv = cp_iso88591_error;
            else
                conv = cp_iso88591_replace;
        }
        else
            // we don't need anything besides the above two
            assert(0);


   assert(CodePage == CP_ACP || CodePage == CP_UTF8 || CodePage == CP_ISO88591);
#if defined (MXHPUXIA) || (MXHPUXPA)
	if((long)lpWideCharStr % 2 == 1) //Odd boundaried UChar* pointer, could cause a SIGBUS
	{
		char tmp_AlignedBuffer[TMP_ALINGNED_BUFSIZE];
		//copy the buffer
		memcpy(tmp_AlignedBuffer, lpWideCharStr, cchWideChar*2);
		len = ucnv_fromUChars(conv, lpMultiByteStr, cbMultiByte,(const UChar*)tmp_AlignedBuffer, cchWideChar, &Uerr);
		if(U_FAILURE(Uerr)) 
		{
			switch(Uerr)
			{
				case U_STRING_NOT_TERMINATED_WARNING:
					// 	An output string could not be NUL-terminated because output length==destCapacity (ignore)
				break;

				case U_BUFFER_OVERFLOW_ERROR:
				// A result would not fit in the supplied buffer
				lastError = ERROR_INSUFFICIENT_BUFFER;
				break;

				case U_INVALID_CHAR_FOUND:
				// Character conversion: Unmappable input sequence (Invalid character)
				case U_ILLEGAL_CHAR_FOUND:
				// Character conversion: Illegal input sequence/combination of input units
				case U_TRUNCATED_CHAR_FOUND:
				// Character conversion: Incomplete input sequence (example just one part of surrogate pair is in the input sequence)
				lastError = ERROR_NO_UNICODE_TRANSLATION;
				break;
				default:
				lastError = Uerr;
			}
		}
		return len; // come out, we are done
	}
	else
		len = ucnv_fromUChars(conv, lpMultiByteStr, cbMultiByte,lpWideCharStr, cchWideChar, &Uerr);
#else		
   len = ucnv_fromUChars(conv, lpMultiByteStr, cbMultiByte,lpWideCharStr, cchWideChar, &Uerr);
#endif
	if(U_FAILURE(Uerr)) {
		switch(Uerr)
		{
			case U_STRING_NOT_TERMINATED_WARNING:
			// 	An output string could not be NUL-terminated because output length==destCapacity (ignore)
			break;

			case U_BUFFER_OVERFLOW_ERROR:
			// A result would not fit in the supplied buffer
			lastError = ERROR_INSUFFICIENT_BUFFER;
			break;

			case U_INVALID_CHAR_FOUND:
			// Character conversion: Unmappable input sequence (Invalid character)
			case U_ILLEGAL_CHAR_FOUND:
			// Character conversion: Illegal input sequence/combination of input units
			case U_TRUNCATED_CHAR_FOUND:
			// Character conversion: Incomplete input sequence (example just one part of surrogate pair is in the input sequence)
			lastError = ERROR_NO_UNICODE_TRANSLATION;
			break;

			default:
			lastError = Uerr;
		}
	}

	return len; // does not count the terminating null character

} // ICUConverter::WideCharToMultiByte()


#ifdef __TEST_ICUCONVERTER

/*
 *  Runs a few MultiByteToWideChar tests
 */

void runMultiByteToWideCharTest(void)
{
    
    ICUConverter *iconv = new ICUConverter("UTF8");

    int  len; // return value from MultiByteToWideChar
    DWORD lastError; // return error from MultiByteToWideChar
   

    /*
     * invalid input sequence
     */

    char fromStr1[] = {0xf5,0xf7,0xf8,0xfb,0xfc,0xfd};
    int  fromStrLen1 = 6;

    UChar wideChar1[100];
    int wideCharLen1;

    len = -1;
    lastError = -1;
    u_memset(wideChar1,0x9,sizeof(wideChar1));
    wideCharLen1 = sizeof(wideChar1)/sizeof(UChar);

    len = iconv->MultiByteToWideChar(CP_UTF8, 
                                    MB_ERR_INVALID_CHARS,
  				    fromStr1,
  				    fromStrLen1,
  				    wideChar1,
  				    wideCharLen1,
  				    lastError );
    assert(len == 0);
    assert(lastError == ERROR_NO_UNICODE_TRANSLATION);

    /*
     * test replacement char
     */

    char fromStr2[] = {0xf5,0xf7,0xf8,0xfb,0xfc,0xfd,0x0};
    int  fromStrLen2 = 7;

    UChar wideChar2[100];
    int wideCharLen2;

    len = -1;
    lastError = -1;
    u_memset(wideChar2,0x9,sizeof(wideChar2));
    wideCharLen2 = sizeof(wideChar2)/sizeof(UChar);

    len = iconv->MultiByteToWideChar(CP_UTF8, 
                                     0,
  				                     fromStr2,
  				                     fromStrLen2,
  				                     wideChar2,
  				                     wideCharLen2,
  				                     lastError );

    UChar refStr2[] = {0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x0};

    assert(len == 7); // since fromStrLen2 is set to strlen +1
    assert(lastError == 0);
    assert(u_strcmp(wideChar2,refStr2) == 0);

    /*
     * test buffer overflow
     */

    char fromStr3[100] = "overflow";
    int  fromStrLen3 = strlen(fromStr3);

    UChar wideChar3[3];
    int wideCharLen3;

    len = -1;
    lastError = -1;
    u_memset(wideChar3,0x9,sizeof(wideChar3));
    wideCharLen3 = sizeof(wideChar3)/sizeof(UChar);

    len = iconv->MultiByteToWideChar(CP_UTF8, 
                                     0,
  				                     fromStr3,
  				                     fromStrLen3,
  				                     wideChar3,
  				                     wideCharLen3,
  				                     lastError );

    UChar refStr3[] = {0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0xfffd, 0x0};

    assert(len == 8); // different from windows - returns the length needed
    assert(lastError == ERROR_INSUFFICIENT_BUFFER);
    
    delete iconv;

} // runMultiByteToWideCharTest()

/*
 *  Runs a few runWideCharToMultiByteTest tests
 */
void runWideCharToMultiByteTest(void)
{
    
    ICUConverter *iconv = new ICUConverter("latin1");

    int  len; // return value from MultiByteToWideChar
    DWORD lastError; // return error from MultiByteToWideChar
   

    /*
     * conversion error sequence
     */

    UChar wideChar1[] = {0x346C,0x34D8,0x1EDD,0x0000};
    int   wideCharLen1;

    char toString1[100];
    int toStringLen1 = sizeof(toString1);

    len = -1;
    lastError = -1;

    memset(toString1,'9',sizeof(toString1));
    wideCharLen1 = u_strlen(wideChar1);

    char replace[] = "?";
    iconv->setReplacementChar(replace,strlen(replace));

    len = iconv->WideCharToMultiByte(CP_ACP,  
                                     0, // dwFlags,
                                     wideChar1,
                                     wideCharLen1,
                                     toString1,
                                     toStringLen1,
                                     NULL,
                                     NULL,
                                     lastError);


    assert(len == 3);
    assert(lastError == 0);

    char refstr1[] = "???";
    assert(strcmp(toString1,refstr1) == 0);


    /*
     * buffer overflow test 
     */

    UChar wideChar2[] = {'o','v','e','r','f','l','o','w',0x0000 };
    int   wideCharLen2;

    char toString2[4];
    int toStringLen2 = sizeof(toString2);

    len = -1;
    lastError = -1;

    memset(toString2,'9',sizeof(toString2));
    wideCharLen2 = u_strlen(wideChar2);

    len = iconv->WideCharToMultiByte(CP_ACP,  
                                     0, // dwFlags,
                                     wideChar2,
                                     wideCharLen2,
                                     toString2,
                                     toStringLen2,
                                     NULL,
                                     NULL,
                                     lastError);


    assert(len == 8); // different from windows - returns the length needed
    assert(lastError == ERROR_INSUFFICIENT_BUFFER);

    delete iconv;
    
} // runWideCharToMultiByteTest()

/*
 *  Runs runUTF8ToISOTest
 */
void runUTF8ToISOTest(void)
{
   time_t t1,t2;
   int  len; // return value from conversion function
   DWORD lastError; // return error from the ICU call
   unsigned char toString1[1000];
   int toStringLen1 = sizeof(toString1);
   int iterations;
 

   unsigned char  oneByteUTF8[] = {0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0x1c,0x1d,0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2a,0x2b,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3a,0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5a,0x5b,0x5c,0x5d,0x5e,0x5f,0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6a,0x6b,0x6c,0x6d,0x6e,0x6f,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7a,0x7b,0x7c,0x7d,0x7e,0x7f };

   unsigned char twoByteUTF8[] = {0xc2,0x80,0xc2,0x81,0xc2,0x82,0xc2,0x83,0xc2,0x84,0xc2,0x85,0xc2,0x86,0xc2,0x87,0xc2,0x88,0xc2,0x89,0xc2,0x8a,0xc2,0x8b,0xc2,0x8c,0xc2,0x8d,0xc2,0x8e,0xc2,0x8f,0xc2,0x90,0xc2,0x91,0xc2,0x92,0xc2,0x93,0xc2,0x94,0xc2,0x95,0xc2,0x96,0xc2,0x97,0xc2,0x98,0xc2,0x99,0xc2,0x9a,0xc2,0x9b,0xc2,0x9c,0xc2,0x9d,0xc2,0x9e,0xc2,0x9f,0xc2,0xa0,0xc2,0xa1,0xc2,0xa2,0xc2,0xa3,0xc2,0xa4,0xc2,0xa5,0xc2,0xa6,0xc2,0xa7,0xc2,0xa8,0xc2,0xa9,0xc2,0xaa,0xc2,0xab,0xc2,0xac,0xc2,0xad,0xc2,0xae,0xc2,0xaf,0xc2,0xb0,0xc2,0xb1,0xc2,0xb2,0xc2,0xb3,0xc2,0xb4,0xc2,0xb5,0xc2,0xb6,0xc2,0xb7,0xc2,0xb8,0xc2,0xb9,0xc2,0xba,0xc2,0xbb,0xc2,0xbc,0xc2,0xbd,0xc2,0xbe,0xc2,0xbf,0xc3,0x80,0xc3,0x81,0xc3,0x82,0xc3,0x83,0xc3,0x84,0xc3,0x85,0xc3,0x86,0xc3,0x87,0xc3,0x88,0xc3,0x89,0xc3,0x8a,0xc3,0x8b,0xc3,0x8c,0xc3,0x8d,0xc3,0x8e,0xc3,0x8f,0xc3,0x90,0xc3,0x91,0xc3,0x92,0xc3,0x93,0xc3,0x94,0xc3,0x95,0xc3,0x96,0xc3,0x97,0xc3,0x98,0xc3,0x99,0xc3,0x9a,0xc3,0x9b,0xc3,0x9c,0xc3,0x9d,0xc3,0x9e,0xc3,0x9f,0xc3,0xa0,0xc3,0xa1,0xc3,0xa2,0xc3,0xa3,0xc3,0xa4,0xc3,0xa5,0xc3,0xa6,0xc3,0xa7,0xc3,0xa8,0xc3,0xa9,0xc3,0xaa,0xc3,0xab,0xc3,0xac,0xc3,0xad,0xc3,0xae,0xc3,0xaf,0xc3,0xb0,0xc3,0xb1,0xc3,0xb2,0xc3,0xb3,0xc3,0xb4,0xc3,0xb5,0xc3,0xb6,0xc3,0xb7,0xc3,0xb8,0xc3,0xb9,0xc3,0xba,0xc3,0xbb,0xc3,0xbc,0xc3,0xbd,0xc3,0xbe,0xc3,0xbf};

  unsigned char refOutPutforTwoByte[] = {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8a,0x8b,0x8c,0x8d,0x8e,0x8f,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9a,0x9b,0x9c,0x9d,0x9e,0x9f,0xa0,0xa1,0xa2,0xa3,0xa4,0xa5,0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2,0xb3,0xb4,0xb5,0xb6,0xb7,0xb8,0xb9,0xba,0xbb,0xbc,0xbd,0xbe,0xbf,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8,0xc9,0xca,0xcb,0xcc,0xcd,0xce,0xcf,0xd0,0xd1,0xd2,0xd3,0xd4,0xd5,0xd6,0xd7,0xd8,0xd9,0xda,0xdb,0xdc,0xdd,0xde,0xdf,0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6,0xe7,0xe8,0xe9,0xea,0xeb,0xec,0xed,0xee,0xef,0xf0,0xf1,0xf2,0xf3,0xf4,0xf5,0xf6,0xf7,0xf8,0xf9,0xfa,0xfb,0xfc,0xfd,0xfe,0xff};
   


   ICUConverter *iconv = new ICUConverter("latin1");
   char replace[] = "?";
   iconv->setReplacementChar(replace,strlen(replace));



t1 = time(NULL);

for(iterations = 0; iterations < 1000000; iterations++) // repeat all the conversions
{
       //convert the whole string
       len = -1;
       lastError = -1;
       memset(toString1,'9',sizeof(toString1));
       len = iconv->UTF8ToFromMultiByte(0, // from utf-8 to client code page 
                                        0, // dwFlags,
                                        (const char*)oneByteUTF8,
                                        sizeof(oneByteUTF8),
                                        (char*)toString1,
                                        toStringLen1,
                                        lastError);
      assert(len == 128 && lastError == 0);
      assert(memcmp(toString1,oneByteUTF8,sizeof(oneByteUTF8)) == 0);

//       printf("onebyte iteration = %d, len = %d, lastError = %d\n",iterations,len,lastError);
    
       len = -1;
       lastError = -1;
       memset(toString1,'9',sizeof(toString1));
       len = iconv->UTF8ToFromMultiByte( 0, // from utf-8 to client code page
                                         0, // dwFlags,
                                         (const char*)twoByteUTF8,
                                         sizeof(twoByteUTF8),
                                        (char*)toString1,
                                         toStringLen1,
                                        lastError);

//       printf("twobyte iteration = %d, len = %d, lastError = %d\n",iterations,len,lastError);
      assert(len == 128 && lastError == 0);
      assert(memcmp(toString1,refOutPutforTwoByte,sizeof(refOutPutforTwoByte)) == 0);

} // end for iterations

   t2 = time(NULL);

   printf("%d iterations, conversions took %f seconds\n",iterations,difftime(t2,t1));

} // runUTF8ToISOTest()

main() {
   //runMultiByteToWideCharTest();
   //runWideCharToMultiByteTest();
   // runUTF8ToISOTest();
}

#endif /* __TEST_ICUCONVERTER */

/*
Conversion routine from UChar to UTF8

UChar* wst  	->	UCharString
int wstlen		->	UCharString length
char *st		->	Buffer for UTF8 string
int stlen		->	Length for *st
int *translen	-> 	translated length (actual length of the ut8 string)
char* error		-> 	Error, if any
DWORD dwFlags	-> 	flag detemines the behavior (error or replacement string)
int* reqLen		-> 	NULL by default, but if the caller provides space, the length of required buffer
					will returned in case a buffer overflow

*/

SQLRETURN ICUConverter::WCharToUTF8(UChar* wst, int wstlen, char *st, int stlen, int *translen, char* error, DWORD dwFlags, int* reqLen)
{
	int len;
	SQLRETURN rc = SQL_SUCCESS;
	DWORD lastError;
	error[0] ='\0';
	if(translen != NULL)
		*translen = 0;
	if (st != NULL && stlen > 0)
	{
		if (wst != NULL)
		{
			len = wstlen;
			if(len < 0)
			{
				if (len == SQL_NTS)
					len = u_strlen((const UChar *)wst);
				else // Invalid length, return SQL_ERROR
				{
					strcpy(error,"WCharToUTF8: Invalid Length");
					return SQL_ERROR;
				}
			}
#ifndef unixcli
			if((len != 0) && (*translen=(int)WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wst, len, st, stlen, NULL, NULL)) == 0)
			{
				switch (GetLastError())
#else
			if(len != 0)
			{
				*translen=(int)WideCharToMultiByte(CP_UTF8, dwFlags, wst, len, st, stlen, NULL, NULL, lastError);
				if(lastError == U_ZERO_ERROR && *translen >= stlen) lastError = ERROR_INSUFFICIENT_BUFFER;	
				if(lastError != U_ZERO_ERROR)
				{
					switch (lastError)
#endif
					{
						case ERROR_INSUFFICIENT_BUFFER:
						{
							if(reqLen != NULL) // if caller is interested to know the required buffer size
								*reqLen = *translen;
							st[stlen-1] ='\0';
							*translen = stlen-1 ;
							strcpy(error,"WCharToUTF8: Insufficient Buffer "); 
							return SQL_SUCCESS_WITH_INFO;
						}
						break;
//Fill in the actual UChar Errors here!
						default:
							strcpy(error,"WCharToUTF8: Unknown Translation Error");
							return SQL_ERROR;
							break;
					}
				}
			}
			else
			{
				*translen = 0;
			}
		}
	}
	else if(st != NULL)
			*st = '\0';
	return rc;
}

SQLRETURN  ICUConverter::UTF8ToWChar(char *st, int stlen, UChar *wst, int wstlen, int *translen, char *error, DWORD dwFlags, int* reqLen)
{
	int len;
	SQLRETURN rc = SQL_SUCCESS;
	DWORD lastError;
	error[0] ='\0';
	if(translen != NULL)
		*translen = 0;
	if (wst != NULL && wstlen > 0)
	{
		if (st != NULL)
		{
			len = stlen;
			if (len < 0)
			{
				if(len == SQL_NTS)
					len = strlen((const char *)st);
				else
				{
					strcpy(error, "UTF8ToWChar: Invalid String Length"); 
					return SQL_ERROR;
				}
			}
			if (len !=0)
			{
				*translen=(int)MultiByteToWideChar(CP_UTF8, (DWORD)dwFlags, (const  char*)st, (int)len, (LPWSTR)wst, wstlen, lastError); 
				 if(lastError == U_ZERO_ERROR && *translen >= wstlen) lastError = ERROR_INSUFFICIENT_BUFFER;
				 if(lastError != U_ZERO_ERROR)
				 {
					switch (lastError)
					{
						case ERROR_INSUFFICIENT_BUFFER:
						{
							strcpy(error,"UTF8ToWChar: Insufficient Buffer ");
							wst[wstlen-1] = UCharNull;
							if(reqLen != NULL) // if caller is interested to know the required buffer size
								*reqLen = *translen;
							*translen = wstlen-1;
							rc = SQL_SUCCESS_WITH_INFO;
						}
						break;
//Fill in the actual UChar Errors here!
						case ERROR_NO_UNICODE_TRANSLATION:
							strcpy(error,"UTF8ToWChar: No Unicode Translation");
							rc = SQL_ERROR;
							break;
						default:
							strcpy(error,"UTF8ToWChar: Unknown Translation Error");
							rc = SQL_ERROR;
							break;
					}
				}
			}
		}
		else
			*wst = UCharNull;
	}
	else if(wst != NULL)
			*wst = UCharNull;
	return rc;
} 
 
//Start
SQLRETURN ICUConverter::WCharToISO88591(UChar* wst, int wstlen, char *st, int stlen, int *translen, char* error, DWORD dwFlags, int* reqLen)
{
	int len;
	SQLRETURN rc = SQL_SUCCESS;
	DWORD lastError;
	error[0] ='\0';
	if(translen != NULL)
		*translen = 0;
	if (st != NULL && stlen > 0)
	{
		if (wst != NULL)
		{
			len = wstlen;
			if(len < 0)
			{
				if (len == SQL_NTS)
					len = u_strlen((const UChar *)wst);
				else // Invalid length, return SQL_ERROR
				{
					strcpy(error,"WCharToUTF8: Invalid Length");
					return SQL_ERROR;
				}
			}
#ifndef unixcli
			if((len != 0) && (*translen=(int)WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wst, len, st, stlen, NULL, NULL)) == 0)
			{
				switch (GetLastError())
#else
			if(len != 0)
			{
				*translen=(int)WideCharToMultiByte(CP_UTF8, dwFlags, wst, len, st, stlen, NULL, NULL, lastError);
				if(lastError == U_ZERO_ERROR && *translen >= stlen) lastError = ERROR_INSUFFICIENT_BUFFER;	
				if(lastError != U_ZERO_ERROR)
				{
					switch (lastError)
#endif
					{
						case ERROR_INSUFFICIENT_BUFFER:
						{
							if(reqLen != NULL) // if caller is interested to know the required buffer size
								*reqLen = *translen;
							st[stlen-1] ='\0';
							*translen = stlen-1 ;
							strcpy(error,"WCharToUTF8: Insufficient Buffer "); 
							return SQL_SUCCESS_WITH_INFO;
						}
						break;
//Fill in the actual UChar Errors here!
						default:
							strcpy(error,"WCharToUTF8: Unknown Translation Error");
							return SQL_ERROR;
							break;
					}
				}
			}
			else
			{
				*translen = 0;
			}
		}
	}
	else if(st != NULL)
			*st = '\0';
	return rc;
}

SQLRETURN  ICUConverter::ISO88591ToWChar(char *st, int stlen, UChar *wst, int wstlen, int *translen, char *error, DWORD dwFlags, int* reqLen)
{
	int len;
	SQLRETURN rc = SQL_SUCCESS;
	DWORD lastError;
	error[0] ='\0';
	if(translen != NULL)
		*translen = 0;
	if (wst != NULL && wstlen > 0)
	{
		if (st != NULL)
		{
			len = stlen;
			if (len < 0)
			{
				if(len == SQL_NTS)
					len = strlen((const char *)st);
				else
				{
					strcpy(error, "UTF8ToWChar: Invalid String Length"); 
					return SQL_ERROR;
				}
			}
			if (len !=0)
			{
				 *translen=(int)MultiByteToWideChar(CP_ISO88591, (DWORD)dwFlags, (const  char*)st, (int)len, (LPWSTR)wst, wstlen, lastError); 
				 if(lastError == U_ZERO_ERROR && *translen >= wstlen) lastError = ERROR_INSUFFICIENT_BUFFER;
				 if(lastError != U_ZERO_ERROR)
				 {
					switch (lastError)
					{
						case ERROR_INSUFFICIENT_BUFFER:
						{
							strcpy(error,"UTF8ToWChar: Insufficient Buffer ");
							wst[wstlen-1] = UCharNull;
							if(reqLen != NULL) // if caller is interested to know the required buffer size
								*reqLen = *translen;
							*translen = wstlen-1;
							rc = SQL_SUCCESS_WITH_INFO;
						}
						break;
//Fill in the actual UChar Errors here!
						case ERROR_NO_UNICODE_TRANSLATION:
							strcpy(error,"UTF8ToWChar: No Unicode Translation");
							rc = SQL_ERROR;
							break;
						default:
							strcpy(error,"UTF8ToWChar: Unknown Translation Error");
							rc = SQL_ERROR;
							break;
					}
				}
			}
		}
		else
			*wst = UCharNull;
	}
	else if(wst != NULL)
			*wst = UCharNull;
	return rc;
}
//ENd


// This routine translate a given from UTF8 to ISO88591 and ISO88591 to UTF8
// Note that errorMsg should have a size MAX_TRANSLATE_ERROR_MSG_LEN
SQLRETURN ICUConverter::UTF8ToFromISO88591(bool forward, char *inst, int inlen, char *outst, int outlen, SQLINTEGER *translen, char *errorMsg, DWORD dwFlags)
{
	SQLRETURN rc = SQL_SUCCESS;
	int len;
	UConverter *convFrom, *convTo;
	DWORD lastError;

	errorMsg[0] ='\0';
	if(translen != NULL)
		*translen = 0;
	
	if(forward) //UTF8 to ISO88591
	{
		if(dwFlags == MB_ERR_INVALID_CHARS)
		{
	    	  convFrom = cp_utf8_error;
	      	convTo = cp_iso88591_error;
		}
        else
		{
	      convFrom = cp_utf8_replace;
	      convTo = cp_iso88591_replace;
        }
    }
    else  //ISO88591 To UTF8
    {
		if(dwFlags == MB_ERR_INVALID_CHARS)
		{
			convTo = cp_utf8_error;
			convFrom = cp_iso88591_error;
		}
		else
		{
			convTo = cp_utf8_replace;
			convFrom = cp_iso88591_replace;
		}
    }

	if (outst != NULL && outlen > 0)
	{
		if (inst != NULL && inlen > 0)
		{
			*translen = CharsetToCharset( convFrom, 
												convTo,
												inst,
												inlen,
												outst,
												outlen,
												lastError );
			if(lastError == U_ZERO_ERROR && *translen >= outlen) lastError = ERROR_INSUFFICIENT_BUFFER;									
			if(lastError != U_ZERO_ERROR)
			{
		   		switch (lastError)
				{
					case ERROR_INSUFFICIENT_BUFFER:
					{
						outst[outlen-1] ='\0';
						*translen = outlen-1 ;
						strcpy(errorMsg,"UTF8ToFromISO88591: Insufficient Buffer "); 
						return SQL_SUCCESS_WITH_INFO;
					}
					break;
//Fill in the actual UChar Errors here!
					default:
						strcpy(errorMsg,"UTF8ToFromISO88591: Unknown Translation Error");
					break;
				}
				*translen = 0;
				return SQL_ERROR;
			}
			
		}
		else
		{
			*outst = '\0';
			strcpy(errorMsg,"UTF8ToFromISO88591: In string is NULL ");  
			return SQL_ERROR;
		}
	}
	else
	{
		if(outst != NULL)
			*outst = '\0';
		strcpy(errorMsg,"UTF8ToFromISO88591: Out string is NULL ");
		return SQL_ERROR;
	}
	return rc ;
}

// The fuction translates a string between UTF-8 and Client Locale. 
// If the first argument is TRUE, the function converts the given string from UTF-8 to Locale.
// If the first argument is FALSE, the function converts the given string from Locale to UTF-8
SQLRETURN ICUConverter::TranslateUTF8(bool forward, char *inst, int inlen, char *outst, int outlen, SQLINTEGER *translen, char *errorMsg, int *charCount)
{
	SQLRETURN rc = SQL_SUCCESS;
	DWORD lastError;

	if(translen != NULL)
		*translen = 0;
	errorMsg[0] ='\0';
	
	if (outst != NULL && outlen > 0)
	{
		if (inst != NULL && inlen > 0)
		{
			//To Improve, directly call charsetTocharset here!!
			*translen=UTF8ToFromMultiByte(forward, MB_ERR_INVALID_CHARS, inst, inlen, outst, outlen, lastError, charCount);
			if(lastError == U_ZERO_ERROR && *translen >= outlen) lastError = ERROR_INSUFFICIENT_BUFFER;
			if(lastError != U_ZERO_ERROR)
			{
				switch (lastError)
				{
					case ERROR_INSUFFICIENT_BUFFER:
						{
							outst[outlen-1] ='\0';
							*translen = outlen-1 ;
							strcpy(errorMsg,"TranslateUTF8: Insufficient Buffer ");  
							return SQL_SUCCESS_WITH_INFO;
						}
						break;
					case ERROR_NO_UNICODE_TRANSLATION:
						strcpy(errorMsg,"TranslateUTF8: No Unicode Translation");
						break;
					default:
						strcpy(errorMsg,"TranslateUTF8: Unknown Translation Error");
						break;
				}
				*translen = 0;
				return SQL_ERROR;
			}
			outst[*translen] = '\0';
		}
		else
			*outst = '\0';
	}
	else
	{
		if(outst != NULL)
			*outst = '\0';
		strcpy(errorMsg,"TranslateUTF8: Out string is NULL ");
		return SQL_ERROR;
	}
	return rc ;	
}

// The fuction translates a string between iso88591  and Client Locale. 
// If the first argument is true, the function converts the given string from ISO88591  to Locale.
// If the first argument is FALSE, the function converts the given string from Locale to ISO88591
SQLRETURN ICUConverter::TranslateISO88591(bool forward, char *inst, int inlen, char *outst, int outlen, SQLINTEGER *translen, char *errorMsg, DWORD dwFlags)
{
	
    SQLRETURN rc = SQL_SUCCESS;
    UConverter *convFrom, *convTo;
    UErrorCode Uerr = U_ZERO_ERROR;
    int32_t len = 0;
    char *target = outst;

    DWORD lastError = 0;

    if(forward)
	{
	   // conversion from iso88591 to code page
		if(dwFlags == MB_ERR_INVALID_CHARS)
       		{
		          convFrom = cp_iso88591_error;
		          convTo = cp_acp_error;
       		}
	       else
       		{
		          convFrom = cp_iso88591_replace;
		          convTo = cp_acp_replace;
       		}
	}
	else
	{
	   // conversion from code page to iso88591
	       if(dwFlags == MB_ERR_INVALID_CHARS)
       		{
		      convFrom = cp_acp_error;
			convTo = cp_iso88591_error;
       		}
	       else
       		{
		          convFrom = cp_acp_replace;
		          convTo = cp_iso88591_replace;
       		}
	}

	// if source charset is same as destination charset, not need to translate - simply do copy
	if (ucnv_compareNames(ucnv_getName(convFrom, &Uerr), ucnv_getName(convTo, &Uerr)) == 0)
	{
		if (inlen >= outlen)
		{
			// A result would not fit in the supplied buffer, truncate
			strncpy(outst, inst, outlen - 1);
			outst[outlen - 1] = '\0' ;
			*translen = outlen - 1;
			return SQL_SUCCESS_WITH_INFO; 
		}
		memcpy(outst, inst, inlen);
		outst[inlen]=0;
		*translen = inlen;
		return rc;
	}
	
    ucnv_convertEx( convTo,
                    convFrom,
                   &target,
                    outst + outlen,
                   (const char**)(&inst),
                    inst + inlen,
                    NULL,NULL,NULL,NULL,
                    TRUE,TRUE, &Uerr);

	if(U_FAILURE(Uerr)) 
	{
		switch(Uerr)
		{
			case U_BUFFER_OVERFLOW_ERROR:
			{
				// A result would not fit in the supplied buffer
				if(!errorMsg)
					strcpy(errorMsg,"TranslateISO88591: Insufficient Buffer "); 
				rc = SQL_SUCCESS_WITH_INFO;
				break;
			}
			default:
				return SQL_ERROR;
		}
	}
	else
	{
		if(target-outst == 0)
		{
			return SQL_ERROR;
		}
		else if(Uerr == U_STRING_NOT_TERMINATED_WARNING)
		{
			// 	An output string could not be NUL-terminated because output length==destCapacity (ignore)
				if(!errorMsg)
					strcpy(errorMsg,"TranslateISO88591: Insufficient Buffer. A result exactly fills the target buffer. Truncate the ending character and replace with NUL terminator."); 
				rc = SQL_SUCCESS_WITH_INFO;
		}
		else
		{
			rc = SQL_SUCCESS;
		}
	}

	*translen = (int32_t)(target-outst);
	outst[*translen] = '\0' ;
	
	return rc;
}

//The following function translates WChar to Locale
SQLRETURN ICUConverter::WCharToLocale(UChar *wst, int wstlen, char *st, int stlen, int *translen, char* error, char *replacementChar)
{
	int len;
	SQLRETURN rc = SQL_SUCCESS;
	DWORD lastError;

	error[0] ='\0';
	if(translen != NULL)
		*translen = 0;
	if (st != NULL && stlen > 0)
	{
		memset(st, '\0',stlen); 
		if (wst != NULL)
		{
			len = wstlen;
			if (len < 0)
			{
				if(len == SQL_NTS)
					len = u_strlen((const UChar *)wst);
				else
				{
					if(!error)
					{
						strcpy(error, "WCharToLocale: Invalid Length for in WChar string"); 
						return SQL_ERROR;
					}
				}
			}
			if (len !=0)
			{
				if(m_UCS2Translation)
				{
					*translen=(int)WideCharToMultiByte(CP_ACP, 0, (const UChar*)wst, len, st, stlen, (LPCSTR)replacementChar, NULL, lastError);
					if(lastError == U_ZERO_ERROR && *translen >= stlen) lastError = ERROR_INSUFFICIENT_BUFFER;
					if (lastError != U_ZERO_ERROR)
					{
						switch (lastError)
						{
							case ERROR_INSUFFICIENT_BUFFER:
							{
								st[stlen-1] ='\0';
								*translen = strlen(st);
								if(!error)
									strcpy(error,"WCharToLocale: Insufficient Buffer "); 
								return SQL_SUCCESS_WITH_INFO;
							}
							break;
							default:
								if(!error)
								strcpy(error,"WCharToLocale: Unknown Translation Error");
								return SQL_ERROR;			
							break;
						}
					}
				}
				else
				{
					if(stlen > wstlen*2)  // have anough space
					{
						memcpy((void*)st, (void*)wst, wstlen*2);
						st[wstlen*2] = '\0';
						*translen = wstlen*2;
					}
					else
					{
						memcpy(st, (char*)wst, stlen-1);
						st[stlen-1] = '\0';
						*translen = stlen-1;
						if(!error)
							strcpy(error,"WCharToLocale: Insufficient Buffer "); 
						return SQL_SUCCESS_WITH_INFO;
					}
				}
			}
			else
				*st = '\0';
		}
		else
			*st = '\0';
	}
	else
		if(st != NULL)
			*st = '\0';
	return rc;
}

//The following function translates Locale to WChar
SQLRETURN ICUConverter::LocaleToWChar(char *st, int stlen, UChar *wst, int wstlen, int *translen, char* error)
{
	int len;
	error[0] = '\0';
	SQLRETURN rc = SQL_SUCCESS;
	DWORD lastError;

	if(translen != NULL)
		*translen = 0;
	if (wst != NULL && wstlen > 0)
	{
		if (st != NULL)
		{
			len = stlen;
			if (len < 0)
			{
				if (len == SQL_NTS)
					len = strlen((const char *)st);
				else
				{
					if(!error)
					{
						strcpy(error, "LocaleToWChar: Invalid Length for in Locale string"); 
						return SQL_ERROR;
					}
				}
			}
			if (len != 0)
			{
				if(m_UCS2Translation)
				{
					*translen=(int)MultiByteToWideChar(CP_ACP, 0, st, len, (LPWSTR)wst, wstlen, lastError);
					if(lastError == U_ZERO_ERROR && *translen >= wstlen) lastError = ERROR_INSUFFICIENT_BUFFER;
					if (lastError != U_ZERO_ERROR)
					{
						switch (lastError)
						{
							case ERROR_INSUFFICIENT_BUFFER:
							{
								wst[wstlen-1] = UCharNull;
								*translen = wstlen-1 ;
								if(!error)
									strcpy(error,"LocaleToWChar: Insufficient Buffer "); 
								return SQL_SUCCESS_WITH_INFO;
							}
							break;
							default:
								if(!error)
									strcpy(error,"LocaleToWChar: Unknown Translation Error");
							break;
						}
						*translen = 0;
						return SQL_ERROR;
					}
				}
				else
				{
					if(wstlen > len/2) //we have enough space!
					{
						memcpy((void*)wst, (void*)st, len);
						wst[len/2] = UCharNull;
						*translen = len;
					}
					else
					{
						memcpy((char*)wst, st, (wstlen - 1)*2);
						wst[wstlen-1] = UCharNull;
						*translen = wstlen-1;
						if(!error)
							strcpy(error,"LocaleToWChar: Insufficient Buffer "); 
						return SQL_SUCCESS_WITH_INFO;
					}
						
				}
			}
			else
				*wst = UCharNull;
		}
		else
			*wst = UCharNull;
	}
	else
	if (wst != NULL)
		*wst = UCharNull;
	
	return rc;
}

/*
//The following function translates WChar to IsoMapping
SQLRETURN ICUConverter::WCharToIsoMapping(UChar *wst, int wstlen, char *st, int stlen, int *translen, char* error, DWORD dwFlags)
{
	int len;
	SQLRETURN rc = SQL_SUCCESS;
	UErrorCode lastError = U_ZERO_ERROR;
	UConverter* conv;
	
	if(dwFlags == MB_ERR_INVALID_CHARS)
	      conv = cp_IsoMapping_error;
    else
	     conv = cp_IsoMapping_replace;

	error[0] ='\0';
	if(translen != NULL)
		*translen = 0;
	if (st != NULL && stlen > 0)
	{
		memset(st, '\0',stlen); 
		if (wst != NULL)
		{
			len = wstlen;
			if (len < 0)
			{
				if(len == SQL_NTS)
					len = u_strlen((const UChar *)wst);
				else
				{
					if(!error)
					{
						strcpy(error, "WCharToLocale: Invalid Length for in WChar string"); 
						return SQL_ERROR;
					}
				}
			}

			if (len != 0)
			{
				*translen = ucnv_fromUChars(conv, st, stlen, wst, len, &lastError);
				if(lastError == U_ZERO_ERROR && *translen >= len) lastError = U_BUFFER_OVERFLOW_ERROR;
				if (lastError != U_ZERO_ERROR)
				{
					switch(lastError)
					{
						case U_BUFFER_OVERFLOW_ERROR:
							// A result would not fit in the supplied buffer
							st[stlen-1] ='\0';
							*translen = strlen(st);
							if(!error)
								strcpy(error,"WCharToIsoMapping: Insufficient Buffer "); 
							return SQL_SUCCESS_WITH_INFO;					
							break;
					case U_STRING_NOT_TERMINATED_WARNING:
					case U_INVALID_CHAR_FOUND:
					case U_ILLEGAL_CHAR_FOUND:
					case U_TRUNCATED_CHAR_FOUND:
					default:
						if(!error)
							strcpy(error,"WCharToIsoMapping: Unknown Translation Error");
						*translen = 0;
						return SQL_ERROR;							
						break;
					}
				}				
			}
			else
				*st = '\0';
		}
		else
			*st = '\0';
	}
	else
		if(st != NULL)
			*st = '\0';
	return rc;
}

//The following function translates IsoMapping to WChar
//
//
//
SQLRETURN ICUConverter::IsoMappingToWChar(char *st, int stlen, UChar *wst, int wstlen, int *translen, char* error, DWORD dwFlags)
{
	int len;
	error[0] = '\0';
	SQLRETURN rc = SQL_SUCCESS;
	UErrorCode lastError = U_ZERO_ERROR;
	UConverter* conv;
	
	if(m_ISOMapping == -1)  //Wait!!  we have not made a connection, yet!!
	{// so (best option) is to convert from utf8 to utf16.
		if(dwFlags == MB_ERR_INVALID_CHARS)
	      conv = cp_utf8_error;
		else
			conv = cp_utf8_replace;
	}
	else
	{
		if(dwFlags == MB_ERR_INVALID_CHARS)
		      conv = cp_IsoMapping_error;
    	else
	    	 conv = cp_IsoMapping_replace;
    }
	if(translen != NULL)
		*translen = 0;
	if (wst != NULL && wstlen > 0)
	{
		if (st != NULL)
		{
			len = stlen;
			if (len < 0)
			{
				if (len == SQL_NTS)
					len = strlen((const char *)st);
				else
				{
					if(!error)
					{
						strcpy(error, "IsoMappingToWChar: Invalid Length for in Locale string"); 
						return SQL_ERROR;
					}
				}
			}
			
			if(len != 0)
			{
				*translen = ucnv_toUChars(conv, wst, wstlen, st, len, &lastError);
				if(lastError == U_ZERO_ERROR && *translen >= len) lastError = U_BUFFER_OVERFLOW_ERROR;
				if (lastError != U_ZERO_ERROR)
				{
					switch(lastError)
					{
						case U_BUFFER_OVERFLOW_ERROR:
							// A result would not fit in the supplied buffer
							wst[wstlen-1] = UCharNull;
							*translen = wstlen-1 ;
							if(!error)
								strcpy(error,"IsoMappingToWChar: Insufficient Buffer "); 
							rc = SQL_SUCCESS_WITH_INFO;
							break;						
						case U_STRING_NOT_TERMINATED_WARNING:
						case U_INVALID_CHAR_FOUND:
						case U_ILLEGAL_CHAR_FOUND:
						case U_TRUNCATED_CHAR_FOUND:
						default:
					// Character conversion: Incomplete input sequence (example just one part of surrogate pair is in the input sequence)
							if(!error)
								strcpy(error,"IsoMappingToWChar: Unknown Translation Error");		
							*wst = UCharNull;
							rc = SQL_ERROR;										
						break;
					}
				}	

			}
			else
				*wst = UCharNull;
		} //if (st != NULL)
		else
			*wst = UCharNull;
	}
	else
		if (wst != NULL)
			*wst = UCharNull;
	return rc;
}

*/

/*

	Helper routines

*/

// returns the length always in bytes
SQLINTEGER ICUConverter::FindStrLength(const char* inStr, SQLINTEGER strLen)
{
	int actualLen;
	if(inStr == NULL) return 0;
		if((m_AppUnicodeType == APP_UNICODE_TYPE_UTF16) && (strLen == SQL_NTS))
		{
			if(m_AppType == APP_TYPE_UNICODE){
				actualLen = u_strlen((UChar*)inStr);
				if(actualLen == 0)	return 0;
				return actualLen * 2;
			}
			else goto mbcs_nts;
		}
		else if((m_AppUnicodeType == APP_UNICODE_TYPE_UTF16) && (strLen != SQL_NTS))
		{
			if(m_AppType == APP_TYPE_UNICODE){
				actualLen = u_strlen((UChar*)inStr);
				if(actualLen == 0)	return 0;
				return strLen * 2;
			}
			else goto mbcs_with_len;
		}
		else if((m_AppUnicodeType != APP_UNICODE_TYPE_UTF16) && (strLen == SQL_NTS))
		{
mbcs_nts:
			actualLen = strlen(inStr);
			if(actualLen == 0)	return 0;
			return actualLen;
		}
		else if((m_AppUnicodeType != APP_UNICODE_TYPE_UTF16) && (strLen != SQL_NTS))
		{ // For UTF8 and Multibyte character sets, non-fixed width, need to determine the
			// number of bytes within the indicated number of characters.
mbcs_with_len:
			if(*inStr== 0)	return 0;

			int actualLen = strlen(inStr);
			if(strLen >= actualLen)
			{
				return actualLen;
			}
			else
			{
				// For now, we convert the source string into UTF16 and intercept the leading
				// charcaters indicated by strLen, and convert it back to the souce character
				// set, and then calculate the length in bytes. This is not a efficient, but
				// only the simplest solution we have right now.
				DWORD lastError;
				UINT CodePage;
				UConverter *ucnv;
				UErrorCode Uerr = U_ZERO_ERROR;	
				int transLen; 

				if(m_AppType == APP_TYPE_ANSI)
					ucnv = cp_acp_replace;
				else // UTF8 Translation
					ucnv = cp_utf8_replace;

				int destULen = actualLen + 1;
				UChar *tmpUStr = new UChar[destULen];
				transLen = ucnv_toUChars(ucnv, tmpUStr, destULen, inStr, -1, &Uerr);

				int destMBcsLen = UCNV_GET_MAX_BYTES_FOR_STRING(strLen, ucnv_getMaxCharSize(ucnv));
				char *tmpMBcs = new char[destMBcsLen+1];
				transLen = ucnv_fromUChars(ucnv, tmpMBcs, destMBcsLen+1, tmpUStr, strLen, &Uerr);
				actualLen = strlen(tmpMBcs);

				delete[] tmpUStr;
				delete[] tmpMBcs;

				return actualLen;
			}
		}
}


SQLCHAR* ICUConverter::AllocCharBufferHelper(	SQLCHAR* inputArg,
		SQLINTEGER inputArgLength, /*in */
		bool isWideCall,  /* in */
		SQLINTEGER* allocBufferLength )
{
	SQLCHAR* bufferPtr = NULL;
	SQLINTEGER u16_len =0;
	SQLINTEGER u8_len = 0;
	// utf8 or locale
	if ((isWideCall) && (m_AppUnicodeType == APP_UNICODE_TYPE_UTF8))
	{
		if(inputArgLength == SQL_NTS)
			u8_len = strlen((const char*)inputArg);
		else
			u8_len = inputArgLength;
		bufferPtr = (SQLCHAR*)malloc(u8_len +1);
		*allocBufferLength = u8_len + 1;
	}
	else /* a utf16 char or a locale char can endup being a 4 bytes utf8 char */
	{
		if(inputArgLength == SQL_NTS)
			u16_len = u_strlen((UChar*)inputArg);
		else
			u16_len = inputArgLength;
		bufferPtr = (SQLCHAR*)malloc(u16_len*4);
		*allocBufferLength = u16_len*4;
	}
	return (SQLCHAR*)bufferPtr;
}


SQLRETURN ICUConverter::InputArgToUTF8Helper(	SQLCHAR* inputArg,	 /*in */
						 				SQLINTEGER inputArgLength, /*in */
						 				SQLCHAR* u8_inputArg, /* in/out Allocated by Caller */
						 				SQLINTEGER u8_inputArgLength, /*in  length of u8_inputArg*/
						 				SQLINTEGER* u8_inputArgTransLength, /*in/out */
						 				bool isWideCall,  /* in */
						 				char* errorMsg /* out */ )
{
	SQLRETURN rc = SQL_SUCCESS;
	*errorMsg = '\0';
	DWORD convError;
	SQLINTEGER u8_len =0;
	if(isWideCall)
	{
		if (m_AppUnicodeType  == APP_UNICODE_TYPE_UTF8) //most probable
		{
			if(inputArgLength != 0 && u8_inputArg != NULL)
			{
				strncpy((char*)u8_inputArg, (char*)inputArg, inputArgLength);
				u8_inputArg[inputArgLength] = '\0';
				*u8_inputArgTransLength = inputArgLength;
			}
			else
				*u8_inputArgTransLength = 0;
		}
		else if (m_AppUnicodeType  == APP_UNICODE_TYPE_UTF16)
    	{
/*			char* tempInputArg=new char[inputArgLength+2];
			memcpy(tempInputArg,inputArg,inputArgLength);
			tempInputArg[inputArgLength]=0;
			tempInputArg[inputArgLength+1]=0;
*/			rc = WCharToUTF8((UChar*)inputArg, inputArgLength/2, (char*)u8_inputArg, u8_inputArgLength, u8_inputArgTransLength, errorMsg);
//			delete tempInputArg;
		}
	}	
	else if (!isWideCall) // ANSI application, convert from locale to UTF8
	{
		rc = TranslateUTF8(false, (char*)inputArg, inputArgLength, (char*)u8_inputArg, u8_inputArgLength, u8_inputArgTransLength, errorMsg);
	}
	else // Shouldn't be here!
	{
		//throw an error!
		strcpy(errorMsg, "InputArgToUTF8Helper : Unknown application type");
		return SQL_ERROR;
	}
	return rc;
}

SQLRETURN ICUConverter::OutputArgFromUTF8Helper(	SQLCHAR* u8_outputArg,	 /*in */
									 		SQLINTEGER u8_outputArgLength, /*in */
									 		SQLCHAR* outputArg, /*in/out  App specified buffer*/
									 		SQLINTEGER outputArgLength, /*in  App specified buffer length, length of outputArg*/  	
									 		SQLINTEGER* outputArgTransLength, /*in/out The actual translated length */
						 					bool isWideCall,  /* in */
						 					char* errorMsg /* out */
						 				)
{
	SQLRETURN rc = SQL_SUCCESS;
	SQLINTEGER u16_len =0;
	SQLINTEGER u8_len = 0;
	*errorMsg = '\0';
	if(isWideCall)
	{
		if(m_AppUnicodeType  == APP_UNICODE_TYPE_UTF8) //most probable
		{
			if(u8_outputArgLength > 0 && outputArg != NULL)
			{
				if(outputArgLength >  u8_outputArgLength) //data truncation
				{
					strcpy((char*)outputArg,(char*)u8_outputArg);
					*outputArgTransLength=strlen((char*)outputArg);
				}
				else
				{
					strncpy((char*)outputArg, (char*)u8_outputArg,outputArgLength -1);
					outputArg[outputArgLength -1] = '\0';
					*outputArgTransLength = outputArgLength -1 ;
					strcpy((char*)errorMsg, "OutputArgFromUTF8Helper : Insufficient Buffer");
					rc = SQL_SUCCESS_WITH_INFO;
				}
			}
			else
				*outputArgTransLength = 0;
		}
		else if(m_AppUnicodeType  == APP_UNICODE_TYPE_UTF16)
		{
			rc=UTF8ToWChar((char*)u8_outputArg, u8_outputArgLength, (UChar*)outputArg, outputArgLength, outputArgTransLength, errorMsg);
		}
	}
	else if (!isWideCall) // ANSI application, convert from UTF8 to Locale
	{
		rc = TranslateUTF8(true, (char*)u8_outputArg, u8_outputArgLength, (char*)outputArg, outputArgLength, outputArgTransLength, errorMsg);
	}
	else // Shouldn't be here!
	{
		strcpy(errorMsg, "OutputArgFromUTF8Helper: Unknown application type");
		rc = SQL_ERROR;
	}
	return rc;
} 

SQLRETURN ICUConverter::InputArgToWCharHelper(	SQLCHAR* inputArg,	 /*in */
						 				SQLINTEGER inputArgLength, /*in */
						 				UChar* dest, /* out Allocated by Caller */
						 				int destLen, /*in  length of output buffers, in bytes*/
						 				int* translatedLen, /*out actual output buffers length.*/
						 				char* errorMsg, /* out */
										bool truncate /*in determine truncate or not when given buffer is not sufficient*/)
{
	SQLRETURN rc = SQL_SUCCESS;
	*errorMsg = '\0';
	DWORD convError;

	SQLINTEGER u8_len =0;

	if(inputArg == NULL)
	{
		strcpy(errorMsg, "InputArgToWCharHelper: NULL input argument. Internal error.");
		if(dest != NULL)
			u_memset(dest,0, destLen);
		return SQL_ERROR;
	}

	if(m_AppType == APP_TYPE_ANSI) // App is Locale, translate locale characters to UTF16
	{
		if(rc = LocaleToWChar((char*)inputArg, inputArgLength, dest, destLen, translatedLen, errorMsg) != SQL_SUCCESS)
		{
			if (rc == SQL_ERROR || !truncate)
			{
				if (rc == SQL_SUCCESS_WITH_INFO)
					strcpy((char *)errorMsg, "InputArgToWCharHelper: Internal error - Insufficient buffer");
				else
					strcat((char *)errorMsg, ": InputArgToWCharHelper: Internal error");
				return SQL_ERROR;
			}
			else
			{
				strcpy((char *)errorMsg, "InArgTranslationHelper: Internal warning - Truncation");
				rc = SQL_SUCCESS_WITH_INFO;
			}
		}
	}
	else	// App is UNICODE
	{
		if(m_AppUnicodeType == APP_UNICODE_TYPE_UTF8)	// App is UTF8, translate UTF8 to UTF16
		{
			if(rc = UTF8ToWChar((char*)inputArg, inputArgLength, (UChar*)dest, destLen, translatedLen, errorMsg) != SQL_SUCCESS)
			{		
				if (rc == SQL_ERROR || !truncate)
				{
					if (rc == SQL_SUCCESS_WITH_INFO)
						strcpy((char *)errorMsg, "InputArgToWCharHelper: Internal error - Insufficient buffer");
					else
						strcat((char *)errorMsg, ": InputArgToWCharHelper: Internal error");
					return SQL_ERROR;
				}
				else
				{
					strcpy((char *)errorMsg, "InputArgToWCharHelper: Internal warning - UTF8ToWchar Truncation");
					rc = SQL_SUCCESS_WITH_INFO;						
				}
			}
		}
		else	// App is UTF16, directly copy.
		{
			if(destLen > inputArgLength)
			{
				u_strcpy(dest, (const UChar*)inputArg);
				*translatedLen = inputArgLength;
			}
			else if (truncate)
			{
				u_strncpy(dest, (const UChar*)inputArg, destLen);
				*translatedLen = destLen;
				rc = SQL_SUCCESS_WITH_INFO;
			}
			else
			{
				strcpy(errorMsg, "InputArgToWCharHelper: Insufficient buffer Error");
				rc = SQL_ERROR;
			}
		}
	}
		
	return rc;
}

void ICUConverter::NullTerminate(char* ValuePtr)
{
	if(ValuePtr != NULL)
	{
		if(m_AppUnicodeType == APP_UNICODE_TYPE_UTF16)
			*((UChar*)ValuePtr) = UCharNull;
		else
			*((char *)ValuePtr) = '\0';
	}
}



// InArgTranslationHelper - translates a given string to UTF8
// Input arguments
// SQLCHAR* arg			- input argument  - char buffer (ANSI/UTF8)	or WChar (UTF16) buffer
// unsigned int argLen	- input argument length - No of bytes ALWAYS!
// char * dest			- Target string
// unsigned int destLen - Target string length - No. of bytes ALWAYS
// InternalData			-  true, if the input argument was kept internally in client locale or ISOMapping, false by default
// caseSensitivity      - Used by Catalog APIs, can have values CASE_NONE,  CASE_NO, CASE_YES
// Function returns either ERROR or SQL_SUCCESs. SQL_SUCCESS_WITH_INFO is unwanted!

SQLRETURN ICUConverter::InArgTranslationHelper(SQLCHAR* arg, unsigned int argLen, char * dest, unsigned int destLen,
										int* transLen, char* errorMsg, bool internalData, int caseSensitivity, bool truncate)
{
	SQLRETURN	rc = SQL_SUCCESS;
	*transLen = 0;
	bool isCase = false;

	errorMsg[0] = '\0';
	
	if(argLen > 0 && (arg != NULL) && destLen >0)
	{
		if(internalData) // input is an internally kept 
		{ 
			// Just make a copy
			if(destLen > argLen)
			{
				strcpy(dest, (const char *)arg);
				*transLen = argLen;
			}
			else if (truncate)
			{
				strncpy(dest, (const char *)arg, destLen-1);
				dest[destLen-1] = '\0';
				*transLen = destLen-1;
				rc = SQL_SUCCESS_WITH_INFO;
			}
			else
			{
				strcpy(errorMsg, "InArgTranslationHelper: InternalData Insufficient buffer Error");
				rc = SQL_ERROR;
			}
		}
		else //Locale/UTF8/UTF16
		{
			if(m_AppType == APP_TYPE_ANSI)
			{
				//Locale to UTF8
				if((rc = TranslateUTF8(FALSE, (char*)arg, argLen, dest, destLen, transLen, errorMsg)) != SQL_SUCCESS)
				{
					if (rc == SQL_ERROR || !truncate)
					{
						if (rc == SQL_SUCCESS_WITH_INFO)
							strcpy((char *)errorMsg, "InArgTranslationHelper: Internal error - Insufficient buffer");
						else
							strcat((char *)errorMsg, ": InArgTranslationHelper: Internal error");
						return SQL_ERROR;
					}
					else
					{
						strcpy((char *)errorMsg, "InArgTranslationHelper: Internal warning - Truncation");
						rc = SQL_SUCCESS_WITH_INFO;
					}
				}	
			}
			else  // m_AppType is Unicode
			{ 
				if(m_AppUnicodeType == APP_UNICODE_TYPE_UTF8) 
				{
				// Just make a copy
					if(destLen > argLen)
					{
						strcpy(dest, (const char *)arg);
						*transLen = argLen;
					}
					else if (truncate)
					{
						 strncpy(dest, (const char *)arg, destLen-1);
						 dest[destLen-1] = '\0';
						 *transLen = destLen-1;
						  rc = SQL_SUCCESS_WITH_INFO;
 					}
					else
 					{
						 strcpy(errorMsg, "InArgTranslationHelper: InternalData Insufficient buffer Error");
						 rc = SQL_ERROR;
					}

				}
				else
				{//utf16 to //WCharToUTF8
					if((rc = WCharToUTF8((UChar*)arg, argLen, dest, destLen, transLen, (char *)errorMsg)) != SQL_SUCCESS)
					{
						if (rc == SQL_ERROR || !truncate)
						{
							strcpy((char *)errorMsg, "CHandle::InArgTranslationHelper: No Connection WCharToUTF8 Error");
							return SQL_ERROR;
						}
						else
						{
							strcpy((char *)errorMsg, "CHandle::InArgTranslationHelper: No Connection WCharToUTF8 Truncation");
							rc = SQL_SUCCESS_WITH_INFO;						
						}
					}
				}
			}
		}
		// Check whether we need to do any upshift (case) operation
		// Now only for catalog APIs
		if(caseSensitivity != CASE_NONE) 
		{//Catalog API args
			if(isCase == CASE_NO)
			{	//	Catalog AP args are NOT case insensitive by default, so they get upshifted.
		 		//	But don't do the upshift, if they are delimited by a pair of "s.
				if ((arg[0] == '"') && (arg[*transLen-1] == '"'))
					isCase = true;
				if(!isCase) // if no "s are specified
					_strupr(dest);
			} 		
		}
	}
	else if(dest != NULL)
			*dest = '\0';
	return rc;
}

// OutArgTranslationHelper - translates a given string to WChar/UTF8/Locale from UTF8
// Input arguments
// SQLHANDLE InputHandle - Connect Handle
// SQLCHAR* arg			- input argument
// unsigned int argLen	- input argument length in bytes(Always)
// wchar_t * dest		- Target string
// unsigned int destLen - Target string length in bytes (Always)
SQLRETURN ICUConverter::OutArgTranslationHelper(SQLCHAR* arg, int argLen, char * dest, int destLen,
										 int *transLen, int *charCount, char *errorMsg, bool LengthInUChars)
{
	SQLRETURN	rc = SQL_SUCCESS;
	UErrorCode Uerr = U_ZERO_ERROR;
	*transLen = 0;
	
	errorMsg[0] = '\0';

	if(destLen == SQL_NTS)
	{
		if (m_AppUnicodeType == APP_UNICODE_TYPE_UTF16)
			destLen = u_strlen((UChar*)dest) *2;
		else
			destLen = strlen((char*)dest);
	}	
	if(argLen > 0 && (arg != NULL) && destLen >0)
	{
		if(m_AppType == APP_TYPE_ANSI) //UTF8 to Locale
		{ 
		 	if((rc = TranslateUTF8(TRUE, (char*)arg, argLen, dest, destLen, transLen, errorMsg, charCount)) == SQL_ERROR)
		 	{
		 		if (errorMsg[0] == '\0')
						strcpy((char *)errorMsg, "OutArgTranslationHelper: UTF8ToLocale  Internal error");
				else
					strcat((char *)errorMsg, ": OutArgTranslationHelper: Translation error");
			}
		}
		else
		{
			 if(m_AppUnicodeType == APP_UNICODE_TYPE_UTF8)
			{
				if(destLen > argLen)
				{
					strcpy((char*)dest, (char*)arg);
					*transLen = argLen;
				}
				else
				{
					strncpy((char*)dest, (char*)arg, destLen-1);
					*((char*)dest + destLen-1) = '\0'; 
					*transLen =  destLen-1;
					rc = SQL_SUCCESS_WITH_INFO;
				}
				if(charCount != NULL)
				{
					*charCount = ucnv_toUChars(cp_utf8_replace, NULL, 0, dest, -1, &Uerr);
				}
			}
			else // Else app type is UTF16
			{
				if((rc = UTF8ToWChar((char*)arg, argLen, (UChar*)dest, destLen-1, transLen, errorMsg)) == SQL_ERROR)
				{
					// utf8 to utf16
					if (errorMsg[0] == '\0')
						strcpy((char *)errorMsg, "CHandle::OutArgTranslationHelper: UTF8ToWChar Error");
					else
						strcat((char *)errorMsg, ": OutArgTranslationHelper: Translation error");
				}
				if(! LengthInUChars)
				{
						*transLen = *transLen * 2; // In bytes
				}
				if(charCount != NULL)
				{
					*charCount = *transLen;
				}
			}
		}
	}
	else if(dest != NULL)
			NullTerminate(dest);
	return rc;
}

char* ICUConverter::strcpyUTF8(char *dest, const char *src, size_t destSize, size_t copySize)
{
        char c;
        size_t len;

        if (copySize == 0)
                len = strlen(src);
        else
                len = copySize;

        if (len >= destSize)
                len = destSize-1; // truncation

        while (len > 0)
        {
                c = src[len-1];
                if (c < 0x80 || c > 0xbf)
                        break;
                len--; // in second, third, or fourth byte of a multi-byte sequence
        }
        strncpy((char*)dest, (const char*)src, len);
        dest[len] = 0;

        return dest;
}

