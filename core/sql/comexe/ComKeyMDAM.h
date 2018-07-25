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
* File:         ComKeyMDAM.h
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef COMKEYMDAM_H
#define COMKEYMDAM_H

#include "ComKeyRange.h"
#include "exp_expr.h"
#include "MdamEnums.h"

class atp_struct;
class ex_tcb;

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for MdamPred
// ---------------------------------------------------------------------
class MdamPred;
typedef NAVersionedObjectPtrTempl<MdamPred> MdamPredPtr;

///////////////////////////////////////////////////////////
// Class MdamPred
///////////////////////////////////////////////////////////
class MdamPred : public NAVersionedObject
{
public:
  enum MdamPredType { MDAM_EQ, 
          MDAM_LT, MDAM_LE, 
          MDAM_GT, MDAM_GE,
          MDAM_BETWEEN,
          MDAM_ISNULL, MDAM_ISNULL_DESC, MDAM_ISNOTNULL,
          MDAM_RETURN_FALSE};

private:

  // expression that computes comparison value -- absent for IS NULL and
  // IS NOT NULL predicates
  ExExprPtr value_;                // 00-07

  // expression that computes 2nd comparison value, used only if predType
  // is MDAM_BETWEEN.
  ExExprPtr value2_;               // 08-15

  // a link field (really should be in some collection instead of here)
  MdamPredPtr next_;               // 16-23

  // to implement OR-groups, i.e. collections of predicates within one
  // disjunct that are OR'd together instead of AND'd:  the next field
  // is TRUE if this is the first predicate in its OR group.  Note that
  // if a predicate is not OR'd with any other predicate, it forms an
  // OR-group by itself and this field would be set to TRUE.
  Int32 firstInOrGroup_;           // 24-27
  
  UInt32 disjunctNumber_;          // 28-31
  Int16 predType_;                 // 32-33

  // These are used only with MDAM_BETWEEN, to indicate inclusivity of the endpoints.
  Int16 val1Inclusive_;            // 34-35
  Int16 val2Inclusive_;            // 36-37

  // This is set to 1 for an MDAM_BETWEEN on a descending key column, which
  // requires the endpoints to swap places in the generated interval.
  Int16 reverseEndpoints_;         // 38-39

  // private method used by public methods getValue() and getValue2()
  ex_expr::exp_return_type getValue_(ExExprPtr value,
                                                atp_struct * atp0,
                                                atp_struct * atp1);

public:

  MdamPred()  // for use by unpack only
       : NAVersionedObject(-1)
  { }

  MdamPred(Lng32 disjunctNumber,
                         MdamPredType pred_type,
                         ex_expr *value,
                         ex_expr *value2 = NULL,
                         Int16 val1Inclusive = 1,
                         Int16 val2Inclusive = 1,
                         Int16 reverse = 0) 
       : disjunctNumber_(disjunctNumber),
         predType_(pred_type),
         value_(value),
         value2_(value2),
         val1Inclusive_(val1Inclusive),
         val2Inclusive_(val2Inclusive),
         reverseEndpoints_(reverse),
         next_(0),
         firstInOrGroup_(TRUE),
         NAVersionedObject(-1)
  { }

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize() { return (short)sizeof(MdamPred); }

  ~MdamPred() { };

  // methods used during Generation only
  void setOr()
  { firstInOrGroup_ = FALSE; };

  Long pack(void *);
  Lng32 unpack(void *,void * reallocator);

  ex_expr::exp_return_type fixup(Lng32 base, unsigned short,
                                            const ex_tcb *tcb,
                                            Space * space,
                                            CollHeap *heap);

  void setNext(MdamPred *next) { next_ = next; };
  MdamPred *getNext() { return next_; };
  NABoolean firstInOrGroup() { return firstInOrGroup_; };
  Lng32 getDisjunctNumber() { return disjunctNumber_; };

  MdamPredType getPredType()
  { return (enum MdamPredType)predType_; };

  NABoolean reverseEndpoints()
  { return (NABoolean)reverseEndpoints_; }

  // Indicates whether the start of an MDAM_BETWEEN interval is inclusive.
  MdamEnums::MdamInclusion getStartInclusion()
    { 
      return val1Inclusive_ 
               ? MdamEnums::MDAM_INCLUDED
               : MdamEnums::MDAM_EXCLUDED; 
    }

  // Indicates whether the end of an MDAM_BETWEEN interval is inclusive.
  MdamEnums::MdamInclusion getEndInclusion()
    { 
      return val2Inclusive_ 
               ? MdamEnums::MDAM_INCLUDED
               : MdamEnums::MDAM_EXCLUDED; 
    }

  // This function computes a transformed predicate type based on
  // predType_ and dataConvErrorFlag.  
  MdamPredType getTransformedPredType
                                     (Lng32 dataConvErrorFlag,
                                      Lng32 dataConvErrorFlag2,
                                      MdamEnums::MdamInclusion& startInclusion,
                                      MdamEnums::MdamInclusion& endInclusion);

  // Get the value used in this predicate. For MDAM_BETWEEN, which has two values,
  // this is the start value.
  ex_expr::exp_return_type getValue(atp_struct* atp0, atp_struct* atp1)
  { return getValue_(value_, atp0, atp1); }

  // Used only for MDAM_BETWEEN, this gets the end value.
  ex_expr::exp_return_type getValue2(atp_struct* atp0, atp_struct* atp1)
  { return getValue_(value2_, atp0, atp1); }
};


// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for MdamColumnGen
// ---------------------------------------------------------------------
class MdamColumnGen;
typedef NAVersionedObjectPtrTempl<MdamColumnGen> MdamColumnGenPtr;

///////////////////////////////////////////////////////////
// Class MdamColumnGen
///////////////////////////////////////////////////////////
class MdamColumnGen : public NAVersionedObject
{
private:

  // expressions used to capture encoded versions of extremal values

  // get lowest value
  ExExprPtr loExpr_;                  // 00-07

  // get highest value
  ExExprPtr hiExpr_;                  // 08-15

  // get lowest non-null value
  ExExprPtr nonNullLoExpr_;           // 16-23

  // get highest non-null value
  ExExprPtr nonNullHiExpr_;           // 24-31

  // pointer to first predicate for this column
  // (would be better to implement this using a collections datatype,
  // but none of them support pack()/unpack() right now...)
  //
  MdamPredPtr preds_;                 // 32-39
  MdamPredPtr lastPred_;              // 40-47

  // pointers to other MdamColumnGen's (implementing a doubly linked list)
  //
  MdamColumnGenPtr previous_;         // 48-55
  MdamColumnGenPtr next_;             // 56-63

  // length of this column in key buffer
  UInt32 columnLength_;               // 64-67

  // offset of this column in key buffer
  UInt32 keyBufferOffset_;            // 68-71

  // true for sparse algorithm, false for dense
  Int32 useSparseProbes_;             // 72-75

  char fillersMdamColumnGen_[20];     // 76-95

public: 

  MdamColumnGen() // for use by unpack only
       : NAVersionedObject(-1)
  { preds_ = NULL; };

  MdamColumnGen(MdamColumnGen *previous,
                           ULng32 columnLength,
                           ULng32 keyBufferOffset,
                           NABoolean useSparseProbes,
                           ex_expr *loExpr,
                           ex_expr *hiExpr,
                           ex_expr *nonNullLoExpr,
                           ex_expr *nonNullHiExpr)
       : columnLength_(columnLength),
         keyBufferOffset_(keyBufferOffset),
   useSparseProbes_(useSparseProbes),
   loExpr_(loExpr),
   hiExpr_(hiExpr),
   nonNullLoExpr_(nonNullLoExpr),
   nonNullHiExpr_(nonNullHiExpr),
   preds_(0),
   lastPred_(0),
   previous_(previous),
   next_(0),
   NAVersionedObject(-1)
  {
    if (previous)
      {
        previous->setNext(this);
      }
  };

  ~MdamColumnGen();

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()
                                      { return (short)sizeof(MdamColumnGen); }

  ex_expr::exp_return_type fixup(Lng32 base, unsigned short mode,
                                            Space * space,
                                            CollHeap *heap, const ex_tcb *tcb);

  MdamColumnGen *getNext()
  { return next_; };
  void setNext(MdamColumnGen *next)
  { next_ = next; };

  // the following function is used in the Generator to obtain the
  // maximum disjunct number
  Lng32 getLastDisjunctNumber()
  {
    Lng32 rc = -1;
    
    if (lastPred_)
      rc = lastPred_->getDisjunctNumber();
    
    return rc;
  };

  // the following mutator functions are used only at Generation time
  MdamPred *getLastPred() 
  { return lastPred_; };

  void setLastPred(MdamPred *last) 
  { 
    if (!preds_)
      preds_ = last;
    lastPred_ = last;
  };

  void setDenseProbes()
  { useSparseProbes_ = FALSE; };

  // the following functions are used at MdamColumn constructor time
  ex_expr *getHiExpr()
  { return hiExpr_; };
  ex_expr *getLoExpr()
  { return loExpr_; };
  ex_expr *getNonNullHiExpr()
  { return nonNullHiExpr_; };
  ex_expr *getNonNullLoExpr()
  { return nonNullLoExpr_; };

  // the following function is used at MdamColumn constructor time
  // and during tree traversal
  ULng32 getLength()
  { return columnLength_; };

  // the following function is used during tree traversal
  ULng32 getOffset()
  { return keyBufferOffset_; };
  NABoolean useSparseProbes()
  { return useSparseProbes_; };

  // the following function used when building an Mdam network
  MdamPred *getFirstPred()
  { return preds_; };

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

};

///////////////////////////////////////////////////////////
// Class keyMdamGen
///////////////////////////////////////////////////////////
class keyMdamGen : public keyRangeGen
{
   // most significant key column
   MdamColumnGenPtr first_;                 // 00-07

   // least significant key column
   MdamColumnGenPtr last_;                  // 08-15

   Int32 maxDisjunctNumber_;                // 16-19

   // The next two fields give upper bounds on storage requirements
   // for an Mdam network.

   // max number of MdamInterval's required
   Int32 maxMdamIntervals_;                 // 20-23

   // max number of MdamRefListEntry's required
   Int32 maxMdamRefs_;                      // 24-27

   // max number of MdamRefListEntry's required to build the stop lists.
   Int32 maxMdamRefsForStopLists_;          // 28-31

   // For reverse scans, we complement the encoded key values in the
   // Mdam network (this is done via the encode expressions created by
   // the Generator) so that we can treat forward scans and reverse
   // scans in the same way. But the scan operators require the values
   // to be uncomplemented.  So, we need to know whether to
   // uncomplement them before returning.  The following member tells us.
   //
   Int32 complementKeysBeforeReturning_;    // 32-35

   // used for evaluating expressions that give values in key predicates
   UInt16 valueAtpIndex_;                   // 36-37

   char fillerskeyMdamGen_[18];             // 38-55

public:

   keyMdamGen()  // default constructor needed for UNPACK
       { first_ = 0; last_ = 0; };  

   keyMdamGen(ULng32 keyLen,
                         ex_cri_desc * workCriDesc,
                         unsigned short keyValuesAtpIndex,
                         unsigned short excludeFlagAtpIndex,
                         unsigned short dataConvErrorFlagAtpIndex,
                         unsigned short valueAtpIndex,
                         MdamColumnGen * first,
                         MdamColumnGen * last,
                         NABoolean complementKeysBeforeReturning,
                         CollHeap * heap);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    keyRangeGen::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(keyMdamGen); }

   ~keyMdamGen();

   MdamColumnGen *getFirst() const
  { return first_; };

   unsigned short getValueAtpIndex() const
  { return valueAtpIndex_; };

   Lng32 getMaxDisjunctNumber() const
  { return maxDisjunctNumber_; };

   Lng32 getMaxMdamIntervals() const
  { return maxMdamIntervals_; };

   Lng32 getMaxMdamRefs() const
  { return maxMdamRefs_; };

   Lng32 getMaxMdamRefsForStopLists() const
  { return maxMdamRefsForStopLists_; };

   NABoolean complementKeysBeforeReturning() const
  { return complementKeysBeforeReturning_; };

   virtual Long pack(void * space);
   virtual Lng32 unpack(void * base, void * reallocator);
   virtual ex_expr* getExpressionNode(Int32 pos);

  virtual keyMdamGen * castToKeyMdamGen() { return this; }

private:
   NABoolean isBlank(char *text);
};



#endif
