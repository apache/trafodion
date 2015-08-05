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

#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include <windows.h>
#include "org_trafodion_ci_WCIUtils.h"


JNIEXPORT void JNICALL Java_org_trafodion_ci_WCIUtils_cls
   (JNIEnv *, jobject)
{
   system("cls");
   return;
}

JNIEXPORT jint JNICALL Java_org_trafodion_ci_WCIUtils_getConsoleMode
   (JNIEnv *env, jobject ob)
{
   DWORD mode;
   HANDLE hConsole = GetStdHandle (STD_INPUT_HANDLE);

   if (hConsole == INVALID_HANDLE_VALUE)
      return -1;

   if (!GetConsoleMode (hConsole, &mode))
      return -1;

   // CloseHandle (hConsole);
   //printf ("JNI get mode=%d\n", mode);

   return mode;
}


JNIEXPORT void JNICALL Java_org_trafodion_ci_WCIUtils_setConsoleMode
   (JNIEnv *env, jobject ob, jint mode)
{
   DWORD m = (DWORD)mode;
   HANDLE hConsole = GetStdHandle (STD_INPUT_HANDLE);

   if (hConsole == INVALID_HANDLE_VALUE)
      return;

   //printf ("JNI set mode=%d\n", m);

   SetConsoleMode (hConsole, m);

   // CloseHandle (hConsole);
}
