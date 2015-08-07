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

#include "sqtypes.h"
#include <stdio.h>
#include <string.h>
#define SECLIBAPI

DLLEXPORT int_16/*usr*//*prv*/TRANSIDTOTEXT
(fixed_0        TransID,     /* IN : Required. */
 char          *Text,        /* OUT: Required. */
 int_16         TextByteLen, /* IN : Required.  Range: >2. */
 int_16        *BytesUsed)   /**/
/* This procedure converts an internal format transid to its external ascii*/
/* form.  The 'TextByteLen' parameter indicates how many bytes are in 'Text'.*/
/* The 'BytesUsed' parameter returns the number of bytes used in the 'Text'*/
/* parameter.  All output parameters are undefined unless FEOK is returned.*/
/**/
/* The TransID is formatted as follows:*/
/**/
/*       \<system-name>(<crash-count>).<cpu>.<sequence>*/
/**/
/* If the system number cannot be converted to a system name:*/
/**/
/*       \<system-number>(<crash-count>).<cpu>.<sequence>*/
/**/
/* If the internal format TransID contains a crashcount of zero, the*/
/* TransID is formatted as follows:*/
/**/
/*       \<system-name>.<cpu>.<sequence>*/
/**/
/* If the system number cannot be converted to a system name:*/
/**/
/*       \<system-number>.<cpu>.<sequence>*/
/**/
/* In any of the four cases above if the system is not named the TransID*/
/* is formatted as follows:*/
/**/
/*       <cpu>.<sequence>*/
/**/
/* Errors returned:*/
/**/
/*        -2          = 'Text' is not large enough.*/
/*        -1          = Internal TransID is invalid.*/
/*        FEOK        = Successful.*/
/*        FEMISSPARM  = A required parameter is missing.*/
/*        FEBOUNDSERR = A reference parameter is invalid.*/

/* OUT: Required.*/

{
  /* NO check - hope that buffer's big enough */
  sprintf(Text, "%llx", TransID);
  *BytesUsed = strlen(Text);
  if (*BytesUsed > TextByteLen)
    return -2; /* but we already went beyond! */
  return 0;
}
