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

#include <stdio.h>

#include <sys/time.h>

#include "tutilp.h"


void print_elapsed(const char     *p_msg,
                   struct timeval *p_t_elapsed) {
    printf("elapsed%s=%ld.%06ld sec\n",
           p_msg,
           p_t_elapsed->tv_sec,
           p_t_elapsed->tv_usec);
}

void print_rate(bool            p_bm,
                const char     *p_prefix,
                int             p_msgcnt,
                int             p_dsize,
                struct timeval *p_t_elapsed,
                double          p_busy) {
    double msgs = (double) p_msgcnt;
    double ds = (double) p_dsize;
    double mb = (double) (p_msgcnt * ds) / 1000000;
    double sec = ((double) p_t_elapsed->tv_sec * 1000000.0 +
                  (double) p_t_elapsed->tv_usec) / 1000000.0;
    if (p_bm)
        printf("%s%d\t%1.2f\t%1.1f\t%1.1f\n",
               p_prefix, p_dsize, mb/sec, msgs/sec, p_busy);
    else
        printf("%sstats=%ddsize|%1.2fMB|%1.0fmsgs|%1.3fsec rate=%1.2fMB/s|%1.1fmsgs/s client-cpu-busy=%%%1.1f\n",
               p_prefix,
               p_dsize,
               mb,
               msgs,
               sec,
               mb/sec,
               msgs/sec,
               p_busy);
}

void print_server_busy(bool        p_bm,
                       const char *p_prefix,
                       double      p_busy) {
    if (p_bm)
        printf("%s%1.1f\n", p_prefix, p_busy);
    else
        printf("%sserver-cpu-busy=%%%1.1f\n", p_prefix, p_busy);
}
