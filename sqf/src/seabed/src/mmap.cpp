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

#include <syscall.h>
#include <unistd.h>

#include <sys/mman.h>

#include "verslib.h"

VERS_LIB(libsbmmap)

//#define XX

// TODO: remove - temp!
extern "C" {
    void *mmap(void   *pp_start,
               size_t  pv_length,
               int     pv_prot,
               int     pv_flags,
               int     pv_fd,
               off_t   pv_offset) __THROW {
        void *lp_ret;

#ifdef XX
printf("Xmmap(%p, %ld, %d, %d(0x%x), %d, %ld)\n",
       pp_start, pv_length, pv_prot, pv_flags, pv_flags, pv_fd, pv_offset);
#endif
        lp_ret = mmap64(pp_start, pv_length, pv_prot, pv_flags, pv_fd, pv_offset);
#ifdef XX
printf("Xmmap ret=%p\n", lp_ret);
#endif
        return lp_ret;
    }
    void *mmap64(void       *pp_start,
                 size_t      pv_length,
                 int         pv_prot,
                 int         pv_flags,
                 int         pv_fd,
                 __off64_t   pv_offset) __THROW {
        static char *lp_next = (char *) (0x80000000 - 4096);
        void        *lp_ret;

#ifdef XX
printf("Xmmap64(%p, %ld, %d, %d(0x%x), %d, %lld)\n",
       pp_start, pv_length, pv_prot, pv_flags, pv_flags, pv_fd, (long long) pv_offset);
#endif
        if (pv_flags & MAP_32BIT) {
            pv_flags &= ~MAP_32BIT;
            pv_flags |= MAP_FIXED;
            lp_next -= pv_length;
            pp_start = lp_next;
#ifdef XX
printf("Xmmap64-new(%p, %ld, %d, %d(0x%x), %d, %lld)\n",
       pp_start, pv_length, pv_prot, pv_flags, pv_flags, pv_fd, (long long) pv_offset);
#endif
        }
        lp_ret = (void *) syscall(__NR_mmap, pp_start, pv_length, pv_prot, pv_flags, pv_fd, pv_offset);
#ifdef XX
printf("Xmmap64 ret=%p\n", lp_ret);
#endif
        return lp_ret;
    }
}
