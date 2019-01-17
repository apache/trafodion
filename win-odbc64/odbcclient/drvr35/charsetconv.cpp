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

SQLRETURN WCharToUTF8(wchar_t *wst, int wstlen, char *st, int stlen, int *translen, char* error)
{
	int len;
	SQLRETURN rc = SQL_SUCCESS;
	if (NULL != error)
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
					len = wcslen((const wchar_t *)wst);
				else // Invalid length, return SQL_ERROR
				{
					if (NULL != error)
						strcpy(error,"WCharToUTF8: Invalid Length");
					return SQL_ERROR;
				}
			}
			if (stlen == 1) // no room for translation, just null terminator
			{
				if (NULL != error)
					strcpy(error,"WCharToUTF8: Insufficient Buffer "); 
				rc = SQL_SUCCESS_WITH_INFO;  
			}
			else if((len != 0) && (*translen=(int)WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wst, len, st, stlen-1, NULL, NULL)) == 0)
			{
				switch (GetLastError())
				{
				case ERROR_INSUFFICIENT_BUFFER:
					{
						char *temp = new char[len*4+1];
						if ((*translen=(int)WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wst, len, temp, len*4, NULL, NULL)) == 0)
						{
							strcpy(error,"WCharToUTF8: Unknown Translation Error");
							*translen = 0;
							delete[] temp;
							return SQL_ERROR;
						}
						strcpyUTF8(st, (const char*)temp, stlen);
						*translen = strlen(st);
						if (NULL != error)
							strcpy(error,"WCharToUTF8: Insufficient Buffer "); 
						delete[] temp;
						return SQL_SUCCESS_WITH_INFO;
					}
					break;
				case ERROR_INVALID_FLAGS:
					if (NULL != error)
						strcpy(error,"WCharToUTF8: Invalid Flags");
					break;
				case ERROR_INVALID_PARAMETER:
					if (NULL != error)
						strcpy(error,"WCharToUTF8: Invalid parameter");
					break;
				default:
					if (NULL != error)
						strcpy(error,"WCharToUTF8: Unknown Translation Error");
					break;
				}
				*translen = 0;
				return SQL_ERROR;
			}
			if(*translen == stlen) // no space for null termintation, adjust the length
			{
				*translen = *translen - 1;
				rc = SQL_SUCCESS_WITH_INFO;
			}
			st[*translen] = '\0';
		}
		else
		{
			*translen = 0;
		}
	}
	else if(st != NULL)
			*st = '\0';
	return rc;
}

SQLRETURN UTF8ToWChar(char *st, int stlen, wchar_t *wst, int wstlen, int *translen, char *error)
{
	short len;
	SQLRETURN rc = SQL_SUCCESS;
	if (NULL != error)
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
					if (NULL != error)
						strcpy(error, "UTF8ToWChar: Invalid String Length"); 
					return SQL_ERROR;
				}
			}
			if (wstlen == 1) // no room for translation, just null terminator
			{
				if (NULL != error)
					strcpy(error,"UTF8ToWChar: Insufficient Buffer ");
				rc = SQL_SUCCESS_WITH_INFO; 
			}
			else if ((len !=0) && (*translen=(int)MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, st, len,(LPWSTR)wst, wstlen-1)) == 0)
			{
				switch (GetLastError())
				{
					case ERROR_INSUFFICIENT_BUFFER:
					{
						wchar_t *temp = new wchar_t[len*2+1];
						if ((*translen=(int)MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, st, len,(LPWSTR)temp, len*2)) == 0)
						{
							strcpy(error,"UTF8ToWChar: Unknown Translation Error");
							*translen = 0;
							delete[] temp;
							return SQL_ERROR;
						}
						wcsncpy(wst, (const wchar_t*)temp, wstlen-1);
						wst[wstlen-1] = L'\0';
						*translen = wcslen(wst);
						if (NULL != error)
							strcpy(error,"UTF8ToWChar: Insufficient Buffer ");
						delete[] temp;
						return SQL_SUCCESS_WITH_INFO;
					}
					break;
					case ERROR_INVALID_FLAGS:
						if (NULL != error)
							strcpy(error,"UTF8ToWChar: Invalid Flags");
						break;
					case ERROR_INVALID_PARAMETER:
						if (NULL != error)	
							strcpy(error,"UTF8ToWChar: Invalid parameter");
						break;
					case ERROR_NO_UNICODE_TRANSLATION:
						if (NULL != error)
							strcpy(error,"UTF8ToWChar: No Unicode Translation");
						break;
					default:
						if (NULL != error)
							strcpy(error,"UTF8ToWChar: Unknown Translation Error");
						break;
				}
				*translen = 0;
				return SQL_ERROR;
			}
			if(*translen == wstlen) // no space for null termintation, adjust the length
			{
				*translen = *translen - 1;
				rc = SQL_SUCCESS_WITH_INFO;
			}
			wst[*translen]= L'\0'; 
		}
		else
			*wst = L'\0';
	}
	else if(wst != NULL)
		*wst = L'\0';
	return rc;
}

// The fuction translates a string between UTF-8 and Client Locale. 
// If the first argument is TRUE, the function converts the given string from UTF-8 to Locale.
// If the first argument is FALSE, the function converts the given string from Locale to UTF-8
SQLRETURN TranslateUTF8(bool forward, const char *inst, int inlen, char *outst, int outlen, int *translen, char *errorMsg)
{
	SQLRETURN rc = SQL_SUCCESS;
	int len;
	wchar_t *wst = NULL;
	UINT codePage1, codePage2;

	if(translen != NULL)
		*translen = 0;
	if (NULL != errorMsg)
		errorMsg[0] ='\0';
	if (forward)
	{
		codePage1 = CP_UTF8;
		codePage2 = CP_ACP;
	}
	else
	{
		codePage1 = CP_ACP;
		codePage2 = CP_UTF8;
	}
	if (outst != NULL && outlen > 0)
	{
		if (inst != NULL && inlen > 0)
		{
			len = inlen*4+1;
			wst = new wchar_t [len];
			if ((*translen=(int)MultiByteToWideChar(codePage1, MB_ERR_INVALID_CHARS, inst, inlen,(LPWSTR)wst, len)) == 0)
			{
				switch (GetLastError())
				{
					case ERROR_INSUFFICIENT_BUFFER:
						if (NULL != errorMsg)
							strcpy(errorMsg,"TranslateUTF8: Insufficient Buffer ");  
						break;
					case ERROR_INVALID_FLAGS:
						if (NULL != errorMsg)
							strcpy(errorMsg,"TranslateUTF8: Invalid Flags");
						break;
					case ERROR_INVALID_PARAMETER:
						if (NULL != errorMsg)
							strcpy(errorMsg,"TranslateUTF8: Invalid parameter");
						break;
					case ERROR_NO_UNICODE_TRANSLATION:
						if (NULL != errorMsg)
							strcpy(errorMsg,"TranslateUTF8: No Unicode Translation");
						break;
					default:
						if (NULL != errorMsg)
							strcpy(errorMsg,"TranslateUTF8: Unknown Translation Error");
						break;
				}
				if (wst != NULL) delete [] wst;
				*translen = 0;
				return SQL_ERROR;
			}
			len = *translen;
			wst[len]= L'\0';

			if (outlen == 1) // no room for translation, just null terminator
			{
				if (NULL != errorMsg)
					strcpy(errorMsg,"TranslateUTF8: Insufficient Buffer "); 
				rc = SQL_SUCCESS_WITH_INFO; 
			}
			else if ((*translen=(int)WideCharToMultiByte(codePage2, 0, (LPCWSTR)wst, len, outst, outlen-1, NULL, NULL)) == 0)
			{
				switch (GetLastError())
				{
					case ERROR_INSUFFICIENT_BUFFER:
					{
						outst[outlen-1] ='\0';
						if (NULL != errorMsg)
							strcpy(errorMsg,"TranslateUTF8: Insufficient Buffer "); 
						if (wst != NULL) 
						{
							delete [] wst;
							wst = NULL;
						}
						rc = SQL_SUCCESS_WITH_INFO;
					}
					break;
					case ERROR_INVALID_FLAGS:
						if (NULL != errorMsg)
							strcpy(errorMsg,"TranslateUTF8: Invalid Flags");
						break;
					case ERROR_INVALID_PARAMETER:
						if (NULL != errorMsg)
							strcpy(errorMsg,"TranslateUTF8: Invalid parameter");
						break;
					default:
						if (NULL != errorMsg)
							strcpy(errorMsg,"TranslateUTF8: Unknown Translation Error");
						break;
				}
				if (wst != NULL) delete [] wst;
				*translen = 0;
				return SQL_ERROR;
			}
			if(*translen == outlen) // no space for null termintation, adjust the length
			{
				*translen = *translen - 1;
				rc = SQL_SUCCESS_WITH_INFO;
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
		if (NULL != errorMsg)
			strcpy(errorMsg,"TranslateUTF8: Out string is NULL ");
		return SQL_ERROR;
	}
	if (wst != NULL) delete [] wst;
	return rc ;
}

//The following function translates WChar to Locale
SQLRETURN WCharToLocale(wchar_t *wst, int wstlen, char *st, int stlen, int *translen, char* error, char *replacementChar)
{
	int len;
	SQLRETURN rc = SQL_SUCCESS;
	if(NULL != error)
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
					len = wcslen((const wchar_t *)wst);
				else
				{
					if(NULL != error)
						strcpy(error, "WCharToLocale: Invalid Length for in WChar string"); 
					return SQL_ERROR;
				}
			}
			if (stlen == 1) // no room for translation, just null terminator
			{
				if(NULL != error)
					strcpy(error,"WCharToLocale: Insufficient Buffer "); 
				rc = SQL_SUCCESS_WITH_INFO; 
			}
			else if ((len !=0) && (*translen=(int)WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)wst, len, st, stlen-1, (LPCSTR)replacementChar, NULL)) == 0)
			{
				switch (GetLastError())
				{
					case ERROR_INSUFFICIENT_BUFFER:
					{
						char *temp = new char[len*4+1];
						if ((*translen=(int)WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)wst, len, temp, len*4, (LPCSTR)replacementChar, NULL)) == 0)
						{
							if(NULL != error)
								strcpy(error,"WCharToLocale: Unknown Translation Error");
							*translen = 0;
							delete[] temp;
							return SQL_ERROR;
						}
						strncpy(st, (const char*)temp, stlen-1);
						st[stlen-1] ='\0';
						*translen = strlen(st);
						if(NULL != error)
							strcpy(error,"WCharToLocale: Insufficient Buffer "); 
						delete[] temp;
						return SQL_SUCCESS_WITH_INFO;
					}
					break;
					case ERROR_INVALID_FLAGS:
						if(NULL != error)
							strcpy(error,"WCharToLocale: Invalid Flags");
						break;
					case ERROR_INVALID_PARAMETER:
						if(NULL != error)
							strcpy(error,"WCharToLocale: Invalid parameter");
						break;
					default:
						if(NULL != error)
							strcpy(error,"WCharToLocale: Unknown Translation Error");
						break;
				}
				*translen = 0;
				return SQL_ERROR;
			}
			if(*translen == stlen) // no space for null termintation, adjust the length
			{
				*translen = *translen - 1;
				rc = SQL_SUCCESS_WITH_INFO;
			}
			st[*translen] = '\0';
		}
	}
	else
		if(st != NULL)
			*st = '\0';
	return rc;
}

//The following function translates Locale to WChar
SQLRETURN LocaleToWChar(char *st, int stlen, wchar_t *wst, int wstlen, int *translen, char* error)
{
	int len;
	if(NULL != error)
		error[0] = '\0';
	SQLRETURN rc = SQL_SUCCESS;
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
					if(NULL != error)
						strcpy(error, "LocaleToWChar: Invalid Length for in Locale string"); 
					return SQL_ERROR;
				}
			}
			if (wstlen == 1) // no room for translation, just null-terminator
			{
				if(NULL != error)
					strcpy(error,"LocaleToWChar: Insufficient Buffer ");
				rc = SQL_SUCCESS_WITH_INFO; 
			}
			else if ((len != 0) && (*translen=(int)MultiByteToWideChar(CP_ACP, 0, st, len, (LPWSTR)wst, wstlen-1)) == 0)
			{
				switch (GetLastError())
				{
					case ERROR_INSUFFICIENT_BUFFER:
					{
						wchar_t *temp = new wchar_t[len*2+1];
						if ((*translen=(int)MultiByteToWideChar(CP_ACP, 0, st, len, (LPWSTR)temp, len*2)) == 0)
						{
							strcpy(error,"LocaleToWChar: Unknown Translation Error");
							*translen = 0;
							delete[] temp;
							return SQL_ERROR;
						}
						wcsncpy(wst, (const wchar_t*)temp, wstlen-1);
						wst[wstlen-1] =L'\0';
						*translen = wcslen(wst);
						if(NULL != error)
							strcpy(error,"LocaleToWChar: Insufficient Buffer "); 
						delete[]  temp;
						return SQL_SUCCESS_WITH_INFO;
					}
					break;
					case ERROR_INVALID_FLAGS:
						if(NULL != error)
							strcpy(error,"LocaleToWChar: Invalid Flags");
						break;
					case ERROR_INVALID_PARAMETER:
						if(NULL != error)
							strcpy(error,"LocaleToWChar: Invalid parameter");
						break;
					default:
						if(NULL != error)
							strcpy(error,"LocaleToWChar: Unknown Translation Error");
						break;
				}
				*translen = 0;
				return SQL_ERROR;
			}
			if(*translen == wstlen) // no space for null termintation, adjust the length
			{
				*translen = *translen - 1;
				rc = SQL_SUCCESS_WITH_INFO;
			}
			wst[*translen] = L'\0';
		}
		else
			*wst = L'\0';
	}
	else
	if (wst != NULL)
		*wst = L'\0';
	
	return rc;
}

bool isUTF8(const char *str)
{
	char c; 
	unsigned short byte = 1;
	size_t len = strlen(str);

	for (size_t i=0; i<len; i++)
	{
		c = str[i];

		if (c >= 0x00 && c < 0x80 && byte == 1) // ascii
			continue;
		else if (c >= 0x80 && c < 0xc0 && byte > 1) // second, third, or fourth byte of a multi-byte sequence 
			byte--;
		else if (c == 0xc0 || c == 0xc1) // overlong encoding
			return false;
		else if (c >= 0xc2 && c < 0xe0 && byte == 1) // start of 2-byte sequence
			byte = 2;
		else if (c >= 0xe0 && c < 0xf0 && byte == 1) // start of 3-byte sequence
			byte = 3;
		else if (c >= 0xf0 && c < 0xf5 && byte == 1) // start of 4-byte sequence
			byte = 4;
		else
			return false;
	}
	return true;
}

// copies src to dest
// if dest is not big enough, src is truncated up to size of dest
// when truncating UTF8 string, it will not truncate in the middle of multi-byte sequence
// always null-terminated

char* strcpyUTF8(char *dest, const char *src, size_t destSize, int copySize)
{
	char c;
	size_t len;
	
	if(copySize != 0)
	{
		if (copySize == SQL_NTS)
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
	}
	else if(dest != 0)
			*dest = '\0';
		return dest;
}
