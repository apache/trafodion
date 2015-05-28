/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#ifndef LMJNI_H
#define LMJNI_H

  // including jni.h with these symbols doesn't work properly
  // save and undefine
  #define _ZWIN32    WIN32
  #define _Z_WIN32    _WIN32
  #define _Z__WIN32__ __WIN32__
  #undef WIN32
  #undef _WIN32
  #undef __WIN32__

#include <jni.h> 

  // restore
  #define WIN32    _ZWIN32
  #define _WIN32   _Z_WIN32
  #define __WIN32__ _Z__WIN32__

#endif // LMJNI_H

