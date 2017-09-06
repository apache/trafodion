//-------------------------------------------------------------------------
//
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
//
//  (06/24/2013)
//  This program is not a participant in general Linux SQL build, unless env
//  SQ_COVERAGE or SQ_COVERAGE_OPTIMIZER is specified, see
//  nskgmake/Makerules.linux. To build it by default, add "memtest" to the
//  EXECUTABLE list iin Makerules.linux.
//  Once built, it can be run standalone. However, it would also be invoked
//  by regress/core/TESTMEM and increase the regression runtime substantially.
//
//-------------------------------------------------------------------------
#include "NAMemory.h"
#include <fstream>
#include <unistd.h>
#include <ios>
#include <iostream>
#include <string>

#include <math.h>
#include <stdlib.h>
#include <sys/time.h>

typedef void* voidPtr;

void memtest_vers2_print() {};

#include "seabed/ms.h"
#include "seabed/fs.h"
extern void my_mpi_fclose();
#include "SCMVersHelp.h"
DEFINE_DOVERS(memtest)


extern "C"
{
Int32 sq_fs_dllmain();
}

void process_mem_usage(long& vm_usage, long& resident_set)
{
   using std::ios_base;
   using std::ifstream;
   using std::string;

   vm_usage     = 0.0;
   resident_set = 0.0;

   // dummy vars for leading entries in stat that we don't care about
   //
   string pid, comm, state, ppid, pgrp, session, tty_nr;
   string tpgid, flags, minflt, cminflt, majflt, cmajflt;
   string utime, stime, cutime, cstime, priority, nice;
   string O, itrealvalue, starttime;

   // the two fields we want
   //
   unsigned long vsize;
   long rss;

   // 'file' stat seems to give the most reliable results
   //
   ifstream stat_stream("/proc/self/stat",ios_base::in);

   stat_stream >> pid >> comm >> state >> ppid >> pgrp >> session >> tty_nr
               >> tpgid >> flags >> minflt >> cminflt >> majflt >> cmajflt
               >> utime >> stime >> cutime >> cstime >> priority >> nice
               >> O >> itrealvalue >> starttime >> vsize >> rss;
               // don't care about the rest

   stat_stream.close();

   long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024;
        // in case x86-64 is configured to use 2MB pages
   vm_usage     = vsize / 1024;
   resident_set = rss * page_size_kb;
}

int main(int argc, char** argv)
{

  dovers(argc, argv);
  try
  {
    file_init_attach(&argc, &argv, TRUE, (char *)"");
    sq_fs_dllmain();
    file_mon_process_startup(true);
    msg_debug_hook("memtest", "memtest.hook");
    atexit(my_mpi_fclose);
  }
  catch (...)
  {
    exit(1);
  }

   long vmSize, resSize;
   timeval startTm, endTm;
   int counts = 0;
   int failedCnt = 0;
   NASegGlobals segGlobals;

   NAHeap hp("Simulated global exec memory", 0,0,0,0,0,&segGlobals, 28);

   #define ONE_G (1024 * 1024 * 1024L)

   #define  MAX_ALLOCATES 200000
   voidPtr hps[MAX_ALLOCATES];

   process_mem_usage(vmSize, resSize);
   printf("memtest: before testing: vmSize %ldKB resSize %ldKB\n",
          vmSize, resSize);
   printf("memtest: phace I tests (random memory de/allocations):\n");
   voidPtr x = hp.allocateHeapMemory(size_t(568041472), FALSE);
   if (x != NULL)
     {
       process_mem_usage(vmSize, resSize);
       printf("memtest: after 500MB alloc: vmSize %ldKB resSize %ldKB\n",
              vmSize, resSize);
       hp.deallocateHeapMemory(x);
       process_mem_usage(vmSize, resSize);
       printf("memtest: after 500MB de-alloc: vmSize %ldKB resSize %ldKB\n",
              vmSize, resSize);
     }
   else
     printf("memtest: failed to allocate a 500MB chunk!\n");

   printf("memtest: continue random memory (de)allocation for size less than 1MB\n");
   gettimeofday(&startTm, 0);
   srand(time(NULL));

   for ( int i=0; i<MAX_ALLOCATES; i++ ) {
     int sz = rand() % 1000000;
     hps[i] = hp.allocateHeapMemory(size_t(sz), FALSE);
     if (hps[i] == NULL)
       failedCnt++;
     else if (sz > 500000)
       counts++;
     if ( rand() % 1 ) {
        int index = rand() % i;
        if ( hps[index] ) {
          hp.deallocateHeapMemory(hps[index]);
          hps[index] = NULL;
       }
     }
    }

   gettimeofday(&endTm, 0);
   printf("\n");
   printf("memtest: larger than 500KB: %d; failed %d\n", counts, failedCnt);
   process_mem_usage(vmSize, resSize);
   printf("memtest: before cleanup: vmSize %ldKB resSize %ldKB\n",
              vmSize, resSize);
   printf("memtest: time spent for 200K allocations: %ld sec\n", (endTm.tv_sec - startTm.tv_sec));

   for ( int i=0; i<MAX_ALLOCATES; i++ )
     if ( hps[i] )
       hp.deallocateHeapMemory(hps[i]);
   gettimeofday(&endTm, 0);

   hp.reInitialize();

   printf("memtest: phase I tests spent: %ld sec\n", (endTm.tv_sec - startTm.tv_sec));
   process_mem_usage(vmSize, resSize);
   printf("memtest: after phase I cleanup: vmSize %ldKB resSize %ldKB\n",
              vmSize, resSize);
   printf("memtest: phase II tests (aligned fragments (de)allocate):\n");

   srand(time(NULL));

   for ( int i=0; i<MAX_ALLOCATES; i++ ) {
     int sz = 4096;
     hps[i] = hp.allocateAlignedHeapMemory(size_t(sz), size_t(512), FALSE);
     if ( rand() % 1 ) {
        int index = rand() % i;
        if ( hps[index] ) {
          hp.deallocateHeapMemory(hps[index]);
          hps[index] = NULL;
       }
     }
    }

   for ( int i=0; i<MAX_ALLOCATES; i++ )
     if ( hps[i] )
       hp.deallocateHeapMemory(hps[i]);

   hp.reInitialize();

   printf("memtest: phase II tests done.\n");
   process_mem_usage(vmSize, resSize);
   printf("memtest: after phase II cleanup: vmSize %ldKB resSize %ldKB\n",
              vmSize, resSize);

   printf("memtest: phase III tests (larger than 2GB chunk (de)allocate):\n");

   for ( int i=1; i<12; i++)
     {
       Int32 j = 1 << i;
       printf("memtest: to allocate %d GB chunk...\n", j);
       voidPtr z = hp.allocateHeapMemory(size_t(j * ONE_G), FALSE);
       if (z != NULL) {
           memcpy((char *)z, "beginning", (size_t)9);
           memcpy(((char *)z + j * ONE_G - 6), "ending", (size_t)6);
         }
       else {
           printf("memtest: failed to allocate a %d GB chunk!\n", j);
           process_mem_usage(vmSize, resSize);
           printf("memtest: current usage: vmSize %ldKB resSize %ldKB\n",
                  vmSize, resSize);
           break;
         }
     }

   hp.reInitialize();
   sleep(10);
   process_mem_usage(vmSize, resSize);
   printf("memtest: after all de-alloc: vmSize %ldKB resSize %ldKB\n",
              vmSize, resSize);
   printf("memtest: phase III tests done.\n");
   return 0;
}
