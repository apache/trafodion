/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
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
**********************************************************************/
/* -*-C++-*-
****************************************************************************
*
* File:         ComTdbSequenceGenerator.h
* Description:  
*
* Created:      3/4/2008
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef COMTDBSEQUENCEGENERATOR_H
#define COMTDBSEQUENCEGENERATOR_H

#include "ComTdb.h"
#include "ExpCriDesc.h"
#include "exp_attrs.h"

//
// Contents of this file
//
class ComSequenceGeneratorAttributes;
class ComTdbSequenceGenerator;
class ComTdbNextValueFor;


//
// Template instantiation to produce a 64-bit pointer emulator class
// for ComSequenceGeneratorAttributes;
//
typedef NAVersionedObjectPtrTempl<ComSequenceGeneratorAttributes> ComSequenceGeneratorAttributesPtr;


//
// class ComSequenceGeneratorAttributes
//

class ComSequenceGeneratorAttributes : public NAVersionedObject
{
public:

  ComSequenceGeneratorAttributes(Int64 sgStartValue,
				 Int64 sgIncrement,
				 Int64 sgMaxValue,
				 Int64 sgMinValue,
				 Int32 sgCycleOption,
				 UInt32 sgDataType)
    :NAVersionedObject(-1),
     sgStartValue_(sgStartValue),
     sgIncrement_(sgIncrement),
     sgMaxValue_(sgMaxValue),
     sgMinValue_(sgMinValue),
     sgCycleOption_(sgCycleOption),
     sgDataType_(sgDataType)  
  {
    flags_ = 0;
  }
    //
  // Redefine virtual functions required for versioning
  //
  ComSequenceGeneratorAttributes() : NAVersionedObject(-1)
  {
  }
  NA_EIDPROC virtual unsigned char getClassVersionID()
  {
    return 1;
  }
  NA_EIDPROC virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0, getClassVersionID());
  }
  NA_EIDPROC virtual short getClassSize()
  {
    return sizeof(ComSequenceGeneratorAttributes);
  }
  NA_EIDPROC virtual Long pack(void *);
  NA_EIDPROC virtual Lng32 unpack(void *, void *);

  // Accessor functions
  //


  NA_EIDPROC inline Int64 getStartValue() const;
 
  NA_EIDPROC inline Int64 getSGIncrement() const;

  NA_EIDPROC inline Int64 getSGMaxValue() const;

  NA_EIDPROC inline Int64 getSGMinValue() const;
  
  NA_EIDPROC inline UInt32 getSGDataType() const;

  NA_EIDPROC inline Int32 getSGCycleOption() const;
  
   protected:
  // Sequence generator Attributes
  Int64                     sgStartValue_;             //00-07
  Int64                     sgIncrement_;              //08-15
  Int64                     sgMaxValue_;               //16-23
  Int64                     sgMinValue_;               //24-31
  Int32                     sgCycleOption_;            //32-35
  UInt32                    sgDataType_;               //36-39
  UInt32                    flags_;                    //40-43
  char fillersComSequenceGeneratorAttributes_[36];     //44-79
};

// Inline Routines for ComSequenceGeneratorAttribute:

inline Int64
ComSequenceGeneratorAttributes::getStartValue() const
{
  return sgStartValue_;
}

inline Int64
ComSequenceGeneratorAttributes::getSGIncrement() const
{
  return sgIncrement_;
}

inline Int64
ComSequenceGeneratorAttributes::getSGMaxValue() const
{
  return sgMaxValue_;
}

inline Int64
ComSequenceGeneratorAttributes::getSGMinValue() const
{
  return sgMinValue_; 
}

inline UInt32
ComSequenceGeneratorAttributes::getSGDataType() const
{
  return sgDataType_;
}

inline Int32
ComSequenceGeneratorAttributes::getSGCycleOption() const
{
  return sgCycleOption_;
}

inline Long ComSequenceGeneratorAttributes::pack(void *space)
{
  return NAVersionedObject::pack(space);
}

inline Lng32 ComSequenceGeneratorAttributes::unpack(void *base, void *reallocator)
{
  return NAVersionedObject::unpack(base, reallocator);
}


//
// Task Definition Block for the Sequence Generator:
//

class ComTdbSequenceGenerator : public ComTdb
{
  friend class ExSequenceGeneratorTcb;

public:

  // Constructors

  // Default constructor (used in ComTdb::fixupVTblPtr() to extract
  // the virtual table after unpacking.

  ComTdbSequenceGenerator();

  // Constructor used by the generator.
  ComTdbSequenceGenerator(ComTdb * childTdb,
                          ex_cri_desc *criDescParentDown,
                          ex_cri_desc *criDescParentUp,
                          queue_index queueSizeDown,
                          queue_index queueSizeUp,
                          const unsigned short dstTuppIndex,
 			  ComSequenceGeneratorAttributes *sgAttributes,
		          const UInt32 sgCache,
		          const UInt32 sgCacheInitial,
		          const UInt32 sgCacheIncrement,
		          const UInt32 sgCacheMaximum,
			  const UInt32 sgCacheRetry,
                          Lng32 numBuffers,
                          ULng32 bufferSize);
  
  // Return a pointer to the child TBD of this SequenceGenerator TDB.
  //
  NA_EIDPROC
  inline ComTdb * getChildTdb() { return childTdb_; }

  // This always returns TRUE from now
  NA_EIDPROC
  Int32 orderedQueueProtocol() const;

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  NA_EIDPROC
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  NA_EIDPROC
  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ComTdb::populateImageVersionIDArray();
  }

  NA_EIDPROC
  virtual short getClassSize() { return (short)sizeof(ComTdbSequenceGenerator); }

  // Pack and Unpack routines
  NA_EIDPROC
  Long pack(void *);
  NA_EIDPROC
  Lng32 unpack(void *, void * reallocator);

  // For the GUI, Does nothing right now
  NA_EIDPROC
  void display() const;
  
  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  NA_EIDPROC virtual void displayContents(Space *space,ULng32 flag);

  // The index of the ATP being returned to the parent, that has the
  // new row (Sequence Value)
  NA_EIDPROC
  inline unsigned short getDstTuppIndex() const;

  // The index of the ATP being returned from the update, that has the
  // new row (Sequence Value)
  NA_EIDPROC
  inline unsigned short getSrcTuppIndex() const;

  NA_EIDPROC
  inline ULng32 getSrcOffset() const;

  NA_EIDPROC
  inline ComSequenceGeneratorAttributes *getSGAttributes() const;


  NA_EIDPROC
    inline UInt32 getSGCacheOverride() { return sgCache_; };

  NA_EIDPROC
    inline UInt32 getSGCacheInitial() { return sgCacheInitial_; };

  NA_EIDPROC
    inline UInt32 getSGCacheIncrement() { return sgCacheIncrement_; };

  NA_EIDPROC
    inline UInt32 getSGCacheMaximum() { return sgCacheMaximum_; };

  NA_EIDPROC
    inline UInt32 getSGCacheRetry() { return sgCacheRetry_; };

  NA_EIDPROC
    inline void setSGCacheOverride(UInt32 value) { sgCache_ = value; };

  NA_EIDPROC
    inline void setSGCacheInitial(UInt32 value) { sgCacheInitial_ = value; };

  NA_EIDPROC
    inline void setSGCacheIncrement(UInt32 value) { sgCacheIncrement_ = value; };

  NA_EIDPROC
    inline void setSGCacheMaximum(UInt32 value) { sgCacheMaximum_ = value; };

  NA_EIDPROC
    inline void setSGCacheRetry(UInt32 value) { sgCacheRetry_ = value; };

   // Virtual routines to provide a consistent interface to TDB's
  // 
  // return a pointer to the specifed (by position) child TDB.
  // SequenceGenerator has only one child.
  //
  NA_EIDPROC
  virtual const ComTdb *getChild(Int32 pos) const
  {
    if(pos == 0) 
      return childTdb_;
    return NULL;
  }

  // numChildren always returns 1 for ComTdbSequenceGenerator
  NA_EIDPROC
  virtual Int32 numChildren() const;

  NA_EIDPROC
  virtual const char *getNodeName() const { return "EX_SEQUENCE_GENERATOR"; };

  // numExpressions always returns 0 for ComTdbSequenceGenerator
  NA_EIDPROC
  virtual Int32 numExpressions() const;
  
  // The names of the expressions
  NA_EIDPROC
  virtual const char * getExpressionName(Int32) const;

  // The expressions thenselves
  NA_EIDPROC
  virtual ex_expr* getExpressionNode(Int32);

protected:

  // The child of this SequenceGenerator TDB.
  //
  ComTdbPtr childTdb_;                                           // 00-07

  // The index of the ATP being returned to the parent, that has the
  // new row (next Value from the sequence generator)
  UInt16 dstTuppIndex_;                                          // 08-09

  char fillersComTdbSequenceGenerator_1_[6];                     // 10-15

  ComSequenceGeneratorAttributesPtr sgAttributes_;               // 16-23

  // sequence generator cache size and calculations.
  // sgCache_ is a pure override.  If greater than zero,
  // then use this cache size.  Otherwise, use the
  // initial, increment and maximum to dynamically
  // calculate the cache blocksize.
  UInt32 sgCache_;                                               // 24-27
  UInt32 sgCacheInitial_;                                        // 28-31
  UInt32 sgCacheIncrement_;                                      // 32-35
  UInt32 sgCacheMaximum_;                                        // 36-39
  UInt32 sgCacheRetry_;                                          // 40-43

  char fillersComTdbSequenceGenerator_[20];                      // 44-63

};


// Inline Routines for ComTdbSequenceGenerator:

inline Int32
ComTdbSequenceGenerator::orderedQueueProtocol() const
{
  return -1; // returns true
};

inline unsigned short
ComTdbSequenceGenerator::getDstTuppIndex() const
{
  return dstTuppIndex_;
};
 
inline ComSequenceGeneratorAttributes *ComTdbSequenceGenerator::getSGAttributes() const
{
  return sgAttributes_;
}


// Class ComTdbNextValueFor


class ComTdbNextValueFor : public ComTdb
{
  friend class ExNextValueForTcb;

public:

  // Constructors

  // Default constructor (used in ComTdb::fixupVTblPtr() to extract
  // the virtual table after unpacking.

  ComTdbNextValueFor();

  // Constructor used by the generator.
  ComTdbNextValueFor(ComTdb * leftChildTdb,
		     ComTdb * rightChildTdb,
		     ex_cri_desc *criDescParentDown,
		     ex_cri_desc *criDescParentUp,
		     ex_cri_desc *workCriDesc,
		     queue_index queueSizeDown,
		     queue_index queueSizeUp,
		     const unsigned short nextValueReturnTuppIndex,
		     ComSequenceGeneratorAttributes *nvSGAttributes,
		     ex_expr *moveSGOutputExpr,
 		     Lng32 numBuffers,
		     ULng32 bufferSize);
  
  // Return a pointer to the child TBDs of this NextValueFor TDB.
  //
  NA_EIDPROC
  inline ComTdb * getLeftChildTdb() { return leftChildTdb_; }

  NA_EIDPROC
  inline ComTdb * getRightChildTdb() { return rightChildTdb_; }

   // This always returns TRUE from now
  NA_EIDPROC
  Int32 orderedQueueProtocol() const;

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  NA_EIDPROC
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  NA_EIDPROC
  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ComTdb::populateImageVersionIDArray();
  }

  NA_EIDPROC
  virtual short getClassSize() { return (short)sizeof(ComTdbSequenceGenerator); }

  // Pack and Unpack routines
  NA_EIDPROC
  Long pack(void *);
  NA_EIDPROC
  Lng32 unpack(void *, void * reallocator);

  // For the GUI, Does nothing right now
  NA_EIDPROC
  void display() const;

  // ---------------------------------------------------------------------
  // Used by the internal SHOWPLAN command to get attributes of a TDB.
  // ---------------------------------------------------------------------
  NA_EIDPROC virtual void displayContents(Space *space,ULng32 flag);


  // Virtual routines to provide a consistent interface to TDB's

  // return a pointer to the specifed (by position) child TDB.
  // NextValueFor has 2 children.
  //
  NA_EIDPROC
  virtual const ComTdb *getChild(Int32 pos) const;
 
  // numChildren always returns 1 for ComTdbSequenceGenerator
  NA_EIDPROC
  virtual Int32 numChildren() const;

  NA_EIDPROC
  virtual const char *getNodeName() const { return "EX_NEXT_VALUE_FOR"; };

  // numExpressions always returns 0 for ComTdbSequenceGenerator
  NA_EIDPROC
  virtual Int32 numExpressions() const;
  
  // The names of the expressions
  NA_EIDPROC
  virtual const char * getExpressionName(Int32) const;

  // The expressions thenselves
  NA_EIDPROC
  virtual ex_expr* getExpressionNode(Int32);

  NA_EIDPROC
  inline ex_expr * moveSGOutputExpr() const;

  NA_EIDPROC
  inline ex_cri_desc *getWorkCriDesc() const;

protected:

  // The children of NextValueFor
  //
  ComTdbPtr leftChildTdb_;                                 // 00-07
  ComTdbPtr rightChildTdb_;                                // 08-15
  ExExprPtr moveSGOutputExpr_;                             // 16-23
  ExCriDescPtr workCriDesc_;                               // 24-31
  ComSequenceGeneratorAttributesPtr nvSGAttributes_;       // 32-39
  // The index of the ATP being returned to the parent, that has the
  // new row with the next value.
  UInt16 nextValueReturnTuppIndex_;                        // 40-41
  char fillersComTdbNextValueFor_[14];                     // 42-55

};

inline Int32
ComTdbNextValueFor::orderedQueueProtocol() const
{
  return -1; // returns true
};

inline const ComTdb* ComTdbNextValueFor::getChild(Int32 pos) const {
  if (pos == 0)
    return leftChildTdb_;
  else if (pos == 1)
    return rightChildTdb_;
  else
    return NULL;
}

inline ex_expr *ComTdbNextValueFor::moveSGOutputExpr() const
{
  return moveSGOutputExpr_;
}

inline ex_cri_desc *ComTdbNextValueFor::getWorkCriDesc() const
{
  return workCriDesc_; 
}


#endif


