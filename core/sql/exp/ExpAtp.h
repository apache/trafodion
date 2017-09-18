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
* File:         ExpAtp.h (previously executor/ex_atp.h)
* Description:  ATP structure (array of tuple pointers)
*               An ATP represents a pointer to one row as it flows through
*               the executor. The row does not have to be contiguous.
* Created:      5/6/98
* Language:     C++
*
*
*
****************************************************************************
*/

#ifndef EXP_ATP_H
#define EXP_ATP_H

#include "SqlExpDllDefines.h"
// Local defines for assertions
//
//#define ExAtpAssert(a,b) ex_assert(a,b)
#define ExAtpAssert(a,b) 

// Includes
//
#include "ComDiags.h"
#include "ExpCriDesc.h"
#include "ExpSqlTupp.h"

// forward declarations
class ex_queue_entry;
class ExStatisticsArea;

//////////////////////////////////////////////////////////////////////////////
// CRI - Composite row instance
//

// Composite row instance is an array of pointers to records
// fragments

//
// An atp is an array of tupp's. Each tupp represents
// a pointer to a tuple.
//
// The first member of the array is actually a pointer to the composite row
// descriptor.

#pragma warning ( disable : 4251 )

class atp_struct
{
 public:

    inline ex_cri_desc* getCriDesc() const;
    inline tupp &       getTupp(Lng32 i);
    inline tupp &       getTuppForNativeExpr(long i);
    inline void         setCriDesc(ex_cri_desc *p);
  // defined in ex_queue.h
    inline void         copyAtp(const ex_queue_entry *from);
    inline void         copyAtp(atp_struct *from);
    inline void         copyPartialAtp(atp_struct * from);
    inline void         copyPartialAtp(atp_struct * from, 
				       short first_tupp,
				       short last_tupp);
    inline void         copyPartialAtp(atp_struct * from, 
				       short my_first_tupp,
				       short from_first_tupp,
				       short last_tupp);
    inline unsigned short  numTuples() const;
    inline void         release();   // de-initialize
    inline void releasePartialAtp(short from_first_tupp,
				  short last_tupp) ;

  // 
  // Methods for manipulating diagnostics area.
  //
    inline ComDiagsArea *getDiagsArea() const;
    inline void setDiagsArea(ComDiagsArea* diagsArea);
    inline void initDiagsArea(ComDiagsArea* diagsArea);
    Long pack(void * space);
    Lng32 unpack(Lng32 base);
  
  // ****  information for GUI  *** -------------
    inline void set_tag(Int32 tag);
    inline Int32 get_tag() const;
    inline void unset_tag();

  // The passed-in cri will be used if it is not NULL. Otherwise
  // the cri associated with the atp is used.
  void display(const char* title = "", ex_cri_desc* cri = NULL);
  //
  // print the content of a data field, based on the data type dt
  void print(char* title, Int16 dt, char* ptr, UInt32 len);
  //---------------------------------------------
   
  //  inline              ~atp_struct();	// destructor

 private:

  Int32          tagged_;       // TRUE or FALSE;
  ex_cri_desc  *criDesc_;     // descriptor (num tupps, etc.)
  ComDiagsArea *diagsArea_;   // diagnostics area
  // the following data member is just a filler. Statistics areas are not passed
  // thru atps anymore. Pulling the data meber out would cause a reload of
  // the database, because atps are stored on disk for some dp2 expressions.
  ExStatisticsArea* statsArea_;
  tupp         tuppArray_[1]; // array of tuple pointers--the actual length of
                              // this array is determined by the amount of space
                              // allocated for an atp struct instance.  e.g., if
			      // sizeof(atp_struct) + 2*sizeof(tupp) bytes of memory
			      // are allocated, then the length is 3
};                  

#pragma warning ( default : 4251 )

// Constructor of an Atp given a cri descriptor.  Can't make it a simple 
// constructor since we need to allocate a variable length array.  In
// other words, the length of an ATP object is not statically known,
// but rather determined dynamically by the number of tuple pointers in 
// the ATP.
// TBD: move to exp/ExpAtp.h

atp_struct * allocateAtp (Lng32 numTuples, CollHeap * space);

atp_struct * allocateAtp (ex_cri_desc * criDesc, CollHeap * space);

// Create an atp inside a pre-allocated buffer instead of allocating it from
// a space object. Used by ExpDP2Expr.
atp_struct * createAtpInBuffer (ex_cri_desc * criDesc, char * & buf);


void deallocateAtp (atp_struct * atp, CollHeap * space);

atp_struct * allocateAtpArray (ex_cri_desc * criDesc,
			       Lng32 cnt,
			       Int32 *atpSize,
			       CollHeap * space,
                               NABoolean failureIsFatal=FALSE);

void deallocateAtpArray(atp_struct **atp, CollHeap * space);

//
// inline procedures for atp_struct
//
inline ex_cri_desc * atp_struct::getCriDesc() const
{
  return criDesc_;
}

inline tupp & atp_struct::getTupp(Lng32 i)
{
#ifdef _DEBUG
  assert(i < criDesc_->noTuples());
#endif
  return tuppArray_[i];
}

//
// Used only when native expressions are generated.  Does not have assert.
//
inline tupp & atp_struct::getTuppForNativeExpr(long i)
{
  return tuppArray_[i];
}

inline void atp_struct::setCriDesc(ex_cri_desc *p)
{
  criDesc_ = p;
};

inline unsigned short atp_struct::numTuples() const
{
  return getCriDesc()->noTuples();
};

inline ComDiagsArea *atp_struct::getDiagsArea() const
{
  return diagsArea_;
}

inline void atp_struct::setDiagsArea(ComDiagsArea* diagsArea)
{
  if (diagsArea_)
    diagsArea_->decrRefCount();

  diagsArea_ = diagsArea;
}

inline void atp_struct::initDiagsArea(ComDiagsArea* diagsArea)
{
  diagsArea_ = diagsArea;
}

inline void atp_struct::set_tag(Int32 tag) 
{ 
  tagged_ = (short) tag; 
}

inline Int32 atp_struct::get_tag() const 
{ 
  return (Int32) tagged_; 
}

inline void atp_struct::unset_tag() 
{ 
  tagged_ = 0; /* FALSE */ 
}

inline void atp_struct::copyAtp(atp_struct *from)
{
  // copy the tupps from one atp to the other.
  // release those tupps in the target atp that are not being copied
  
  Lng32 i = numTuples();
  Lng32 j = from->numTuples();
  
  // release the entries past the number of tuples in the source if any
  while(i > j)
  {
    i--;
    this->getTupp(i).release(); // zero tupp
  }

  // copy the remaining entries
  while (i>0)
  {
    i--;
    this->getTupp(i) = from->getTupp(i); //copy tupp
  }

  set_tag(from->get_tag());

  if (from->getDiagsArea())
    from->getDiagsArea()->incrRefCount();

  setDiagsArea(from->getDiagsArea());
}

// copies tupps first_tupp thru last_tupp from 'from'(source)
// to 'this' (target). tupp numbers are zero based.
inline void atp_struct::copyPartialAtp(atp_struct * from, 
				       short first_tupp,
				       short last_tupp)
{
  for (short i = first_tupp; i <= last_tupp; i++)
  {
    this->getTupp(i) = from->getTupp(i); //copy tupp
  }
  set_tag(from->get_tag());      
}

// copies tupps first_tupp thru last_tupp from 'from'(source)
// to 'this' (target). Copies starting at my_first_tupp.
// tupp numbers are zero based.
inline void atp_struct::copyPartialAtp(atp_struct * from, 
				       short my_first_tupp,
				       short from_first_tupp,
				       short last_tupp)
{
  short j = my_first_tupp;
  
  for (short i = from_first_tupp; i <= last_tupp; i++, j++)
  {
    this->getTupp(j) = from->getTupp(i); //copy tupp
  }
}

inline void atp_struct::copyPartialAtp(atp_struct * from)
{
#pragma nowarn(1506)   // warning elimination 
  copyPartialAtp(from, 0, from->numTuples()-1);
#pragma warn(1506)  // warning elimination 
}

// De-initialize atp. deallocation is done elsewhere
// (probably done incorrectly)
inline void atp_struct::release()
{
  ExAtpAssert(criDesc_, "atp_struct::release() has no criDesc");

  // Release each tupp.
  //
  for(Int32 i=0; i<criDesc_->noTuples(); i++)
  {
    tuppArray_[i].release();
  }

  // null-out diags area pointer; setDiagsArea releases existing
  // reference to diags area if it is non-null
  setDiagsArea(0);
}

inline void atp_struct::releasePartialAtp(short from_first_tupp,
 					  short last_tupp)
{
  // Release each tupp.
  //
  for(Int32 i=from_first_tupp; i<=last_tupp; i++)
  {
    tuppArray_[i].release();
  }
  
  // null-out diags area pointer; setDiagsArea releases existing
  // reference to diags area if it is non-null
  setDiagsArea(0);
}

#endif
