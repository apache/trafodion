//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef __SB_INT_OPTS_H_
#define __SB_INT_OPTS_H_

#define USE_NOWAIT_OPEN 1               // can't take out - due to SQL dependency
#define USE_NEW_START_NOWAIT 1          // can't take out - due to SQL dependency
#define USE_SB_UTRACE_API               // micro trace-api
#define USE_SB_NEW_RI                   // receive-info

#ifdef SQ_PHANDLE_VERIFIER
  #ifndef USE_PHAN_VERIFIER
    #define USE_PHAN_VERIFIER
  #endif
#endif

// post-process defines
#ifdef USE_SB_INLINE
#  define SB_INLINE inline
#else
#  define SB_INLINE
#endif
#ifdef USE_SB_MAP_STATS
#  ifndef USE_SB_NO_SL
#    define USE_SB_NO_SL
#  endif
#endif

#endif // !__SB_INT_OPTS_H_
