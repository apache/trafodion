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

#ifndef __TSTATS_H_
#define __TSTATS_H_

#ifndef __TUTILP_H_
#error "include 'tutilp.h' instead"
#endif

extern void print_elapsed(const char *p_msg, struct timeval *p_t_elapsed);
extern void print_rate(bool            p_bm,
                       const char     *p_prefix,
                       int             p_msgcnt,
                       int             p_dsize,
                       struct timeval *p_t_elapsed,
                       double          p_busy);
extern void print_server_busy(bool        p_bm,
                              const char *p_prefix,
                              double      p_busy);

#endif // !__TSTATS_H_
