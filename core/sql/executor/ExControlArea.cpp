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
/* -*-C++-*-
****************************************************************************
*
* File:         ExControlArea.cpp
* Description:  see header file
*
* Created:      5/6/98
* Language:     C++
*
****************************************************************************
*/

#include "ex_transaction.h"


ExControlEntry::ExControlEntry(CollHeap * heap,
			       ControlQueryType cqt, 
			       Int32 reset,
			       char * sqlText, Int32 lenX, Int16 sqlTextCharSet,
			       char * value1,  Int32 len1,
			       char * value2,  Int32 len2,
			       char * value3,  Int32 len3,
                               Int16 actionType,
				 ResendType resendType,
                                 NABoolean nonResettable)
     : heap_(heap), cqt_(cqt), reset_(reset),
       sqlText_(sqlText), value1_(value1), value2_(value2), value3_(value3),
       lenX_(lenX),       len1_(len1),     len2_(len2),     len3_(len3),
       sqlTextCharSet_(sqlTextCharSet),
       actionType_(actionType),
	 resendType_(resendType),
         nonResettable_(nonResettable)
{
  numValues_ = 0;
  if (value1) numValues_++;
  if (value2) numValues_++;
  if (value3) numValues_++;
}

ExControlEntry::~ExControlEntry()
{
  NADELETEBASIC(sqlText_, heap_);
  for (Int32 i = 0; i < this->getNumValues(); i++)
      NADELETEBASIC(this->getValue(i+1), heap_) ;
      
}

char * ExControlEntry::getValue(Int32 i)
{
  if (i == 1)
    return value1_;
  else if (i == 2)
    return value2_;
  else if (i == 3)
    return value3_;
  else
    return NULL;
}

Int32 ExControlEntry::getLen(Int32 i)
{
  if (i == 1)
    return len1_;
  else if (i == 2)
    return len2_;
  else if (i == 3)
    return len3_;
  else
    return 0;
}

Int32 ExControlEntry::match(ControlQueryType cqt,
			  const char * value1,
			  const char * value2,
			  Int32 reset)
{
  if (cqt_ != cqt)    return FALSE;
  if (reset_ > reset) return FALSE;
  if (cqt_ == SHAPE_) return TRUE;
  Int32 v1 = !value1 || !*value1 || !str_cmp_ne(value1_, value1);
  if (cqt_ == TABLE_)
    {
      Int32 v2 = !value2 || !*value2 || !str_cmp_ne(value2_, value2);
      v1 = v1 && v2;
    }
  return v1;
}


ExControlEntry::ResendType ExControlEntry::getResendType() { return resendType_; }


ExControlArea::ExControlArea(ContextCli *context, CollHeap *heap)
     : context_(context), heap_(heap)
{
  controlList_ = new(heap_) Queue(heap_);
  resetAllQueueEntry_ = NULL;
  sysDefResetQueueEntry_ = NULL;
}

ExControlArea::~ExControlArea()
{
  if (controlList_)
    {
      controlList_->position();
      ExControlEntry * e;
      while ((e = (ExControlEntry *)controlList_->getNext()) != NULL)
	{
	  NADELETE(e, ExControlEntry,heap_);
	}
      NADELETE(controlList_, Queue, heap_);
      controlList_ = NULL;
    }
}

void ExControlArea::addControl(ControlQueryType type, 
			       Int32 reset,
			       const char * sqlText, Int32 lenX,
			       const char * value1,  Int32 len1,
			       const char * value2,  Int32 len2,
			       const char * value3,  Int32 len3,
                               Int16 actionType,
                               ExControlEntry::ResendType resendType,
                               NABoolean isNonResettable)
{
   NABoolean addToList = TRUE;
   Queue *q = controlList_;
   ExControlEntry *e;

   if (reset == -1)
   {
   // CQD * RESET
   // postiion back to earlier CQD * RESET RESET
   // and remove all entries below it
      addToList = FALSE;
      // overloading the resetAllQueueEntry_ to denote that CQD * RESET was
      // issued 
      resetAllQueueEntry_ = (void *)1L;
      if (sysDefResetQueueEntry_ == NULL)
         q->position();
      else
      {
         q->position(sysDefResetQueueEntry_);
         q->getNext(); // Leave CQD * RESET RESET entry
      }
      while ((e = (ExControlEntry *)q->getNext()) != NULL)
      { 
         if  (e->type() == DEFAULT_)
         {
             if (! e->isNonResettable())
             {
                q->remove(NULL);
                NADELETE(e, ExControlEntry, heap_);
             }
         }
      }
   }
   else
   if (reset == -2)
   {
      if (sysDefResetQueueEntry_ != NULL)
      { 
         // Remove the CQD * RESET RESET entry
         q->position(sysDefResetQueueEntry_);
         e = (ExControlEntry *)q->getNext();
         q->remove(NULL);
         NADELETE(e, ExControlEntry, heap_);
      }
   }
   else
   if (reset == 1)
   {
      // Remove a CQD entry where attributes match
      // Loop to remove all matching entries for all types
      addToList = FALSE;
      if (type ==  DEFAULT_ && sysDefResetQueueEntry_ != NULL)
      {
         q->position(sysDefResetQueueEntry_);
         q->getNext();
      }
      else
         q->position();
      while ((e = (ExControlEntry *)q->getNext()) != NULL)
      {
        if (e->getActionType() != ComTdbControl::HOLD_ 
              && e->match(type, value1, value2))
        {
           if (! e->isNonResettable())
           {
              q->remove(NULL);
              NADELETE(e, ExControlEntry, heap_);
           }
           // Exactly one match for these, so stop iterating now:
           //   CQD attr RESET;
           //   CQT tbl attr RESET;
           if (len1)
              if (type != TABLE_ || len2)
                 break;
        }
      }         
   }
   else
   if ( actionType == ComTdbControl::RESTORE_)
   {
      // Remove the matching hold CQD
      addToList = FALSE;
      if (sysDefResetQueueEntry_ != NULL)
      {
         q->position(sysDefResetQueueEntry_);
         q->getNext();
      }
      else
         q->position();
      while ((e = (ExControlEntry *)q->getNext()) != NULL)
      {
        if (e->getActionType() == ComTdbControl::HOLD_ 
              && e->match(type, value1, value2))
        {
           q->remove(NULL);
           NADELETE(e, ExControlEntry, heap_);
           break;
        }
      }         
   }
   else
   if ( actionType == ComTdbControl::HOLD_)
      addToList = TRUE;
   else
   {
      addToList = TRUE;
      // If CQD * RESET RESET is already issued afer CQD * RESET is issued
      // we need to go from the system defaults
      // Otherwise go from begining and eliminate duplicates while establishing the ODBC conection
      if (type == DEFAULT_  && sysDefResetQueueEntry_ != NULL && 
                 resetAllQueueEntry_ == NULL)
      {
         q->position(sysDefResetQueueEntry_);
         q->getNext();
      }
      else
         q->position();
      while ((e = (ExControlEntry *)q->getNext()) != NULL)
      {
         if (e->getActionType() != ComTdbControl::HOLD_
                && e->match(type, value1, value2))
         {
            q->remove(NULL);
            NADELETE(e, ExControlEntry, heap_);
            break;
         }
      }
   }
   if (addToList)
   {
      char * sX = NULL;
      char * v1 = NULL;
      char * v2 = NULL;
      char * v3 = NULL;
        
      if (sqlText && lenX)
      {
         sX = new(heap_) char[lenX + 1];
         str_cpy_all(sX, (char *)sqlText, lenX);
         sX[lenX] = 0;
      }
      if (value1 && len1)
      {
          v1 = new(heap_) char[len1 + 1];
          str_cpy_all(v1, (char *)value1, len1);
          v1[len1] = 0;
      }
      if (value2 && len2)
      {
         v2 = new(heap_) char[len2 + 1];
         str_cpy_all(v2, (char *)value2, len2);
         v2[len2] = 0;
      }
      if (value3 && len3)
      {
         v3 = new(heap_) char[len3 + 1];
         str_cpy_all(v3, (char *)value3, len3);
         v3[len3] = 0;
      }
        
      e = new(heap_)
           ExControlEntry(heap_, type, reset, sX, lenX,
                   SQLCHARSETCODE_UTF8
                   , v1, len1, v2, len2, v3, len3, 
                   actionType, resendType, isNonResettable);
      if (reset == -2)
      {
         controlList_->insert(e, 0, &sysDefResetQueueEntry_);
         resetAllQueueEntry_ = NULL;
      }
      else
         controlList_->insert(e);
  }   
}

const char *ExControlArea::getText(ControlQueryType cqt)
{
  switch (cqt)
    {
    case SHAPE_:
      return "CONTROL QUERY SHAPE";
    case DEFAULT_:
      return "CONTROL QUERY DEFAULT";
    case TABLE_:
      return "CONTROL TABLE";
    case CONTROL_SESSION_:
      return "CONTROL SESSION";
    case SESSION_DEFAULT_:
      return "SET SESSION DEFAULT";
    }

  // error
  return NULL;
}

