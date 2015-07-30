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
  #ifndef NULL
    #ifdef __cplusplus
      #define NULL 0
    #else
      #define NULL ((void *)0)
    #endif
  #endif
  #include "platform.h"
  #include "nsk/nskport.h"
  #include "fs/feerrors.h"
_declspec(dllexport) PCHAR NSKGetRegKeyServerWarePath()
{ return NULL; }

_declspec(dllexport) PCHAR NSKGetRegKeyConfigPath()
{ return NULL; }

_declspec(dllexport) int_16 SECURITY_PSB_GET_(
        int_16         item,
        void          *value_,
        int_16         max_len,
        int_16        *value_len_arg,
        int_16         pin)    // PIN identifies target process, unused on NT
{ return FEBADPARMVALUE; }

extern "C" _declspec(dllexport) short FILE_GETINFOLISTBYNAME_
  (char           *name,        
   short           length_a,   
   short          *itemlist,    
   short           numberitems,
   short          *result,    
   short           resultmax,
   short          *result_len,  
   short          *erroritem)
{
  return 0;
}

extern "C" _declspec(dllexport) _int64   COMPUTETIMESTAMP (short   *date_n_time, //IN
                                                 short   *error)       //OUT OPTIONAL
{
  return 0;
}
;

extern "C" _declspec(dllexport) int INTERPRETTIMESTAMP (_int64 juliantimestamp, //IN
                                                        short  *date_n_time)    //OUT
{
  return 0;
}
;

extern "C" _declspec(dllexport) _int64 JULIANTIMESTAMP (short type,
                                                        short *tuid,
                                                        short *error,
                                                        short node)
{
   return 0;
}
;
