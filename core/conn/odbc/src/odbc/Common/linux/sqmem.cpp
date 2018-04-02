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
#define SQMem_C

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stddef.h>

#include "sqmem.h"
#include "feerrors.h"

DllExport
void SQBugCheck(const char * pchar, const char * ptypeName, int typeValue)
{
   printf("SQMem error: %s, value %s = %d.\n", 
          pchar, ptypeName, typeValue);
   exit(-1);
}

DllExport
void SQBugCheck(const char * pchar)
{
   printf("SQMem error: %s.\n", pchar);
   exit(-1);
}


DllExport
long SQInsert(LIST_ENTRY *pwhere, LIST_ENTRY *pitem)
{
   if((pwhere->Flink->Blink != pwhere) || (pwhere->Blink->Flink != pwhere))
      SQBugCheck("SQINSERT");

   pitem->Flink = pwhere->Flink;
   pitem->Blink = pwhere->Flink->Blink;
   pwhere->Flink = pitem;
   pitem->Flink->Blink = pitem;

   return NO_ERROR;
}

DllExport
long SQDelete(LIST_ENTRY *pitem)
{
   if((pitem->Flink->Blink != pitem) ||
         (pitem->Blink->Flink != pitem))
      SQBugCheck("SQDELETE");

   pitem->Blink->Flink = pitem->Flink;
   pitem->Flink->Blink = pitem->Blink;

   return NO_ERROR;

}

//#define ALIGNMENT 16
//#define ALIGNMENT_MASK 0xFFFFFFF0L
#define TAG_OVERHEAD sizeof(long)
#define MIN_BLOCKSIZE 32
#define POOL_EYE_CATCHER 0x1234567L

//
// PoolPtr must be 16 byte aligned
//


DllExport
void * PoolDefine(void * PoolPtr, int PoolSize, short * error)
{
   PPOOL_HEADER pph;

   // take care of alignment

   //pph = (PPOOL_HEADER)(((int) PoolPtr + (ALIGNMENT - 1)) & ALIGNMENT_MASK);
   pph = (PPOOL_HEADER) PoolPtr;
   PoolSize = PoolSize - ((long)pph - (long)PoolPtr);

   memset(pph, 0, sizeof(POOL_HEADER));
   pph->eyecatcher = POOL_EYE_CATCHER;
   pph->freelist.Flink =
   pph->freelist.Blink = &(pph->freelist);
   pph->curhigh =
      pph->maxhigh =
         pph->poolstart = &(pph[1]);
   pph->poolend = (void *) ((long)pph + PoolSize);
   pph->poolsize = (long) ((long) pph->poolend - (long) pph->poolstart);

   *error = NO_ERROR;

   return pph;
}

DllExport
short PoolCheck(PPOOL_HEADER pph)

{
   long size;
   int i,j;
   LIST_ENTRY *plist;
   PBLOCK_HEAD pblock;
   PBLOCK_HEAD pnext;

   if(pph->eyecatcher != POOL_EYE_CATCHER)
         SQBugCheck("POOLCHECK_HEADER", (char *) "i" , pph->eyecatcher);

   // check the free list

   i = 0;
   plist = pph->freelist.Flink;
   while((plist != &(pph->freelist)) || (i > (int) pph->curfrag))
   {
      i++;
      pblock = (PBLOCK_HEAD) ((long) plist - TAG_OVERHEAD);
      pnext = (PBLOCK_HEAD) ((long) pblock + abs(pblock->tag));

      // the check below is performed in case freelist
      // goes outside of pool area.

      if((void *) pblock < pph->poolstart ||
         (void *) pnext > pph->curhigh     ||
         abs(pblock->tag) < MIN_BLOCKSIZE)
               SQBugCheck("POOLCHECK_HD");

      if (pnext[-1].endtag != pblock->tag)
         SQBugCheck("POOLCHECK_TAG");
      if (pblock->tag < 0)
         SQBugCheck("POOLCHECK_POS", "Tag", pblock->tag);
      plist = plist->Flink;
   }
                           // freelist count correct?
   if (i != (int) pph->curfrag)
      SQBugCheck("POOLCHECK_FREELIST", (char *) "i" , i);

                        // pool empty?
   if(pph->curhigh == pph->poolstart)
   {
      if(pph->curalloc != 0 ||
         pph->curfrag != 0)
         SQBugCheck("POOLCHECK_COUNT");
         else
         return 0;

   }
   // check blocks

   pblock = (PBLOCK_HEAD) pph->poolstart;
   i = j = 0;

   while ((void *) pblock < pph->curhigh)
   {
      if((size = abs(pblock->tag)) < MIN_BLOCKSIZE)
         SQBugCheck("POOLCHECK_BLOCK", (char *) "i" , pblock->tag);
      pnext = (PBLOCK_HEAD) (((long) pblock) + size);
      if (pblock->tag != pnext[-1].endtag)
         SQBugCheck("POOLCHECK_BLKTAG");
      if (pblock->tag < 0)
         i = i + size;
      else
         j = j + size;
      pblock = pnext;
   }

   if (i != pph->curalloc ||
      j != ((long) pph->curhigh - (long) pph->poolstart - i))
      SQBugCheck("POOLCHECK_ALLOC");

   return 0;
}

DllExport
void * PoolGetSpace(PPOOL_HEADER pph, int size, short *error)
{
   int curfrag;
   int new_size;
   PBLOCK_HEAD pblock;
   PBLOCK_HEAD pnext;

#ifdef _DEBUG
   if(PoolCheck(pph))
      SQBugCheck("POOLGETSPACE_COR1");
#endif

   *error = NO_ERROR;

   //size = ((size + (2 * TAG_OVERHEAD)) + ALIGNMENT - 1) & ALIGNMENT_MASK;
   size = (size + (2 * TAG_OVERHEAD));
   int to = TAG_OVERHEAD; //JdR testing only

   if (MIN_BLOCKSIZE > size)
      size = MIN_BLOCKSIZE;

   pph->gets ++;

   // if items on freelist, check there first.

   if((curfrag = pph->curfrag) != 0)
   {
      pblock = (PBLOCK_HEAD) (((long) &(pph->freelist)) - TAG_OVERHEAD);
      do
      {
         pblock = (PBLOCK_HEAD) ((long) pblock->list.Flink - TAG_OVERHEAD);
                        //(int is unsigned, long is signed)
      } while (((int)pblock->tag < size) && (curfrag--) > 0);

                        // terminated loop with block?
   if (curfrag > 0)
   {                  // is block large enough to split?
      if ((new_size = pblock->tag - size) >= MIN_BLOCKSIZE)
      {
                        // keep first part of block in freelist
         pnext = (PBLOCK_HEAD) ((long) pblock + new_size);
         pblock->tag = pnext[-1].endtag = new_size;
         pblock = pnext;
      } else
      {                // remove entry from freelist
         size = pblock->tag;
         SQDelete((LIST_ENTRY *) &(pblock->list));
         pph->curfrag--;
      }

      if ((pph->curalloc = pph->curalloc + size) > pph->maxalloc)
         pph->maxalloc = pph->curalloc;
      pnext = (PBLOCK_HEAD) ((long) pblock + size);
      pblock->tag = pnext[-1].endtag = -(long) size;

#ifdef _DEBUG
      if(PoolCheck(pph))
         SQBugCheck("POOLGETSPACE_COR2");
#endif
         return (void *) &(pblock->data);
      } // found entry on freelist
   } // items on freelist
                              // room in pool?
   if (((long) pph->poolend - (long) (pblock = (PBLOCK_HEAD)pph->curhigh)) >= size)
   {
      pnext = (PBLOCK_HEAD)((long) pblock + size);

      if ((pph->curhigh = (void *) pnext) > pph->maxhigh)
         pph->maxhigh = pph->curhigh;
      if ((pph->curalloc = pph->curalloc + size) > pph->maxalloc)
         pph->maxalloc = pph->curalloc;
      pblock->tag = pnext[-1].endtag = -(long) size;

#ifdef _DEBUG
      if(PoolCheck(pph))
         SQBugCheck("POOLGETSPACE_COR3");
#endif
      return &(pblock->data);

   } else
   {            // no room in pool
      pph->fails++;
      pph->failsize = size;

#ifdef _DEBUG
      if(PoolCheck(pph))
         SQBugCheck("POOLGETSPACE_COR4");
#endif
      *error = FENOPOOLSPACE;
      return NULL;
   }
}

DllExport
long PoolPutSpace(PPOOL_HEADER pph, void * pelement)
{
   PBLOCK_HEAD pblock;
   PBLOCK_HEAD pnext;

   LIST_ENTRY *pprevlink;
   int free_size;
   long prev_tag;
   long next_tag;

#ifdef _DEBUG
   if(PoolCheck(pph))
      SQBugCheck("POOLPUTSPACE_COR1");
#endif

   pph->puts++;

   pblock = (PBLOCK_HEAD)((long) pelement - TAG_OVERHEAD);
   pnext = (PBLOCK_HEAD) ((long) pblock  + (free_size = abs(pblock->tag)));

   if ((void *)pblock < pph->poolstart || (void *) pnext > pph->curhigh)
      SQBugCheck("POOLPUTSPACE_HEAD");
   if ((free_size < MIN_BLOCKSIZE) ||
      (pnext[-1].endtag != pblock->tag) ||
       pblock->tag >= 0)
       SQBugCheck("POOLPUTSPACE_BLK");
                                // check block at beginning of pool
   if ((void *) pblock == pph->poolstart)
      prev_tag = 0;
   else
      prev_tag = pblock[-1].endtag;
                                // check for block at end of pool
   if (pnext == (PBLOCK_HEAD) pph->curhigh)
      next_tag = 0;
   else
      next_tag = pnext->tag;

   pph->curalloc = pph->curalloc - free_size;

   if(next_tag == 0)  // no next block, so just lower curhigh
   {
      if (prev_tag >= MIN_BLOCKSIZE)  // prev block free?
      {                               // delete it from freelist
         pblock = (PBLOCK_HEAD) ((long) pblock - prev_tag);
         if (SQDelete(&(pblock->list)))
            SQBugCheck("POOLPUTSPACE_LIST");
         pph->curfrag--;
      }
      pph->curhigh = (void *)pblock;

#ifdef _DEBUG
      if(PoolCheck(pph))
         SQBugCheck("POOLPUTSPACE_COR2");
#endif
      return NO_ERROR;
   }

   // it's not an end block.

   if (prev_tag >= MIN_BLOCKSIZE)  // prev free too?
   {
      pblock = (PBLOCK_HEAD) ((long) pblock - prev_tag);
      free_size = free_size + prev_tag;
                                     // next free too?
      if(next_tag >= MIN_BLOCKSIZE)
      {
         free_size = free_size + next_tag;
         if(SQDelete (&(pnext->list)))
            SQBugCheck("POOLPUTSPACE_LIST2");
         pph->curfrag --;
      }
   } else
   {   //prev is not free
      if(next_tag >= MIN_BLOCKSIZE) // next free?
      {
         free_size = free_size + next_tag;
                       // replace next with pblock in list
         pblock->list.Flink = pnext->list.Flink;
         pblock->list.Blink = pnext->list.Blink;

         pnext->list.Blink->Flink =
         pnext->list.Flink->Blink = &pblock->list;
      } else
      {  // next is not free
         pprevlink = pph->freelist.Blink;
               // follow list from back to find insertion point
         while (pprevlink != &(pph->freelist) && (long) pprevlink > (long) pblock)
            pprevlink = pprevlink->Blink;

         if (SQInsert(pprevlink, &(pblock->list)))
            SQBugCheck("POOLPUTSPACE_INSERT");

         if((pph->curfrag++) > pph->maxfrag)
            pph->maxfrag = pph->curfrag;
      }

   } //prev is not free

   // assign block tags.
   //note: there is never a free block at end of pool.
   pnext = (PBLOCK_HEAD) ((long) pblock + free_size);
   pblock->tag = pnext[-1].endtag = free_size;

#ifdef _DEBUG
   if(PoolCheck(pph))
      SQBugCheck("POOLPUTSPACE_COR3");
#endif
   return 0;
}

// fire this up if problems are encountered

DllExport
void TestPool(void * membase, int length)
{
   long j, index;
   short error;
   long pass;
   void * pool_ptrs[256 + 1];
   PPOOL_HEADER pph;

   pass = 1;


   while (true)
   {
      for(index=0; index<=256; index++)
         pool_ptrs[index] = NULL;

      membase = PoolDefine(membase, length, &error);
      if(error != NO_ERROR)
         SQBugCheck("TESTPOOL_PD", "PoolDefine error", error);

      pph = (PPOOL_HEADER)membase;

      for(j=0; j <= 31777; j++)
      {
         index = rand() / ((RAND_MAX)/ 256);
         if(pool_ptrs[ index ] != NULL)
         {
            error = PoolPutSpace((PPOOL_HEADER)membase, pool_ptrs[ index ]);
            if(error)
               SQBugCheck("TESTPOOL_PPS", "PoolPutSpace error", error);
            pool_ptrs[ index ] = NULL;
         } else
         {
            pool_ptrs[ index ] = PoolGetSpace((PPOOL_HEADER)membase,
                                              (int) rand(), &error);
            if (error != NO_ERROR)
            {
               SQBugCheck("TESTPOOL_PGS", "PoolGetSpace error", error);
               SQBugCheck("TESTPOOL_PGS", "index", index);
            }
         }

      }

      for(index = 0;index <= 255;index++)
      {
         if(pool_ptrs[index] != NULL)
         {
            error = PoolPutSpace((PPOOL_HEADER)membase, pool_ptrs[index]);
            if(error)
               SQBugCheck("TESTPOOL_POOLPUTSPACE", "error", error);
         }
      }

      printf("pooltest pass %ld \n", pass++);
   }
}

