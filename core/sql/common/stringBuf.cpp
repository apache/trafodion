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

#include "stringBuf.h"

// Explicit function template argument lists are not supported yet 
// in expression contexts. Use non-template argument instead.
NAWcharBuf* checkSpace(CollHeap* heap, Int32 sourceLen, NAWcharBuf*& target, NABoolean addNullAtEnd)
{
  Int32 olen = sourceLen;
  if (addNullAtEnd) olen++;

  NAWcharBuf* finalTarget = target;

#ifndef MODULE_DEBUG
// SQL/MX 
   if ( target ) {
      
      if ( target -> getBufSize() < olen ) 
         finalTarget = NULL;

   } else {
      if ( heap )
           finalTarget = new (heap) stringBuf<NAWchar>(olen, heap);
        else
           finalTarget = new stringBuf<NAWchar>(olen);
   }

#else // MODULE_DEBUG
   if ( target ) {
      if ( target -> getBufSize() < olen ) {
          finalTarget = NULL;
      } 
   } else
      finalTarget = new stringBuf<NAWchar>(olen);
#endif // MODULE_DEBUG

   // side-effect the pass-in target buffer if addNullAtEnd is TRUE.
   if ( addNullAtEnd && target && target->getBufSize() > 0 )
   {
     (target->data())[0] = NULL;
     // keep existing behavior for now // target->setStrLen(0);
   }

   return finalTarget;
}

charBuf* checkSpace(CollHeap* heap, Int32 sourceLen, charBuf*& target, NABoolean addNullAtEnd)
{
  Int32 olen = sourceLen;
  if (addNullAtEnd) olen++;

  charBuf* finalTarget = target;

#ifndef MODULE_DEBUG
// SQL/MX 
   if ( target ) {

     if ( target -> getBufSize() < olen ) 
       finalTarget = NULL;

   } else {
      if ( heap )
           finalTarget = new (heap) charBuf(olen, heap);
        else
           finalTarget = new charBuf(olen);
   }
#else // MODULE_DEBUG
   if ( target ) {
      if ( target -> getBufSize() < olen ) {
          finalTarget = NULL;
      } 
   } else
      finalTarget = new charBuf(olen);
#endif // MODULE_DEBUG

   // side-effect the pass-in target buffer if addNullAtEnd is TRUE.
   if ( addNullAtEnd && target && target->getBufSize() > 0 )
   {
     (target->data())[0] = NULL;
     // keep existing behavior for now // target->setStrLen(0);
   }

   return finalTarget;
}


