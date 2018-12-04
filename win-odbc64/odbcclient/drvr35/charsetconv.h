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
#ifndef CHARSETCONV_H
#define CHARSETCONV_H

#include "windows.h"
#include <sql.h>
#include <sqlext.h>
#include "drvrglobal.h"

SQLRETURN WCharToUTF8(wchar_t *wst, int wstlen, char *st, int stlen, int *translen, char* error);
SQLRETURN UTF8ToWChar(char *st, int stlen, wchar_t *wst,  int wstlen, int *translen, char* error);

// If the first argument is TRUE, the function converts the given string from UTF-8 to Locale.
// If the first argument is FALSE, the function converts the given string from Locale to UTF-8
SQLRETURN TranslateUTF8(bool forward, const char *inst, int inlen, char *outst, int outlen, int *translen, char *errorMsg);

SQLRETURN WCharToLocale(wchar_t *wst, int wstlen, char *st, int stlen, int *translen, char* error = NULL, char *replacementChar = NULL);
SQLRETURN LocaleToWChar(char *st, int stlen, wchar_t *wst, int wstlen,  int *translen, char* error = NULL);

// some helper routines
bool isUTF8(const char *str);
char* strcpyUTF8(char *dest, const char *src, size_t destSize, int copySize=SQL_NTS);

#endif //CHARSETCONV_H
