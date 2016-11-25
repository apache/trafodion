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
* File:         ExpSqlTupp.h (previously /executor/sql_tupp.h)
* Description:
*
* Created:      5/6/98
* Language:     C++
*
*
*
****************************************************************************
*/

#ifndef EXP_SQL_TUPP_H
#define EXP_SQL_TUPP_H

#include "SqlExpDllDefines.h"
#include "BaseTypes.h"

// Local defines for assertions
//
//#define ExSqlTuppAssert(a,b) ex_assert(a,b)
#define ExSqlTuppAssert(a,b) 

//forward reference
class tupp_descriptor;
class ex_queue;

#define NA_DEBUG_TUPP 1
//
// tupp is simply a pointer to a tupp descriptor.
// we want to control the copying of pointers to tupp descriptors
//
// In the executor and expression code no variables of class tuppDescriptor *
// should be used. Only tupp should be used.
//
class SQLEXP_LIB_FUNC  tupp
{	
  tupp_descriptor *	tuppDescPointer;
  
public:
NA_EIDPROC
   ~tupp();				// destructor
NA_EIDPROC
   tupp();				// constructor
NA_EIDPROC
  tupp(const tupp_descriptor * source); // constructor
NA_EIDPROC
  tupp(const tupp & source); // constructor
  
NA_EIDPROC 
  inline void init();			// Initialize a newly allocated tupp
NA_EIDPROC 
  inline void	operator=(const tupp	& source); 
NA_EIDPROC 
  inline void	operator=(const tupp_descriptor * source); 
NA_EIDPROC 
  inline void	release();		// Release the tuple this tupp refers to
NA_EIDPROC 
  inline char *	getDataPointer() const;
NA_EIDPROC 
  inline void   setDataPointer(char *dp);
NA_EIDPROC 
  inline tupp_descriptor* get_tupp_descriptor();  
NA_EIDPROC 
  inline unsigned short getRefCount() const;
NA_EIDPROC
  inline ULng32   getAllocatedSize() const;

NA_EIDPROC
  Long pack(void * space);
NA_EIDPROC
  Lng32 unpack(Lng32 base);
NA_EIDPROC
  NABoolean isAllocated(){return (tuppDescPointer ? TRUE : FALSE);};

NA_EIDPROC
  void display();

};

#pragma nowarn(1506)   // warning elimination 
class SQLEXP_LIB_FUNC  tupp_descriptor
{

#ifdef COMING_FROM_NATIVE_EXPR_SOURCE_FILE
       // Native Expression code needs to be able to calculate the address
       // of the tupleAddress_ pointer.  However, doing so is not allowed
       // by C++ if the following data is declared 'private' as we want
       // to do.  So, we declare it 'public' ONLY WHEN this header file
       // is #included by Native Expression code and 'private' otherwise.
       // The Native Expression code will access the tupp_descriptor
       // ONLY for reading -- absolutely no modifying!
public:

#else
private:

#endif // COMING_FROM_NATIVE_EXPR_SOURCE_FILE

friend class tupp;

  enum TuppType {DATA_TUPP = 0, CONTROL_TUPP = 1, 
		 DIAGS_TUPP = 2, STATS_TUPP = 3, 
		 INVALID_DATA_VSBB_TUPP = 4};

  
  union
  {
    unsigned short referenceCount_;

    // These flags are overloaded with referenceCount so as to keep
    // the size of this class as small as possible. These flags indicate
    // the 'kind' of tuple descriptor that is moved to sql_buffer. Since these
    // flags are only valid while communicating between exe & dp2, or
    // exe & esp, they could be overloaded with referenceCount__
    // as that field is not in use during communication.
    // non-atomic inserts will also be using the flags_ data member in EID code
    // for VSBB & possibly sidetree inserts. The idea is that since these tuple
    // are "bufInbuf" their referenceCount value will not be needed.
    unsigned short flags_;
  };

  unsigned short filler_;
  ULng32 allocatedSize_;

  union
  {
    tupp_descriptor * relocatedAddress_; //addr where record is being copied
    
    struct
    {
      unsigned short nextTDIOffset_;
      unsigned short prevTDIOffset_;
    } tdiOffset_;
  };
  
  union
    {
      Lng32   offset_;
      char * tupleAddress_;
    };

protected:
NA_EIDPROC
  unsigned short& nextTDIOffset() { return tdiOffset_.nextTDIOffset_; }
NA_EIDPROC
  unsigned short& prevTDIOffset() { return tdiOffset_.prevTDIOffset_; }

public:
NA_EIDPROC 
   tupp_descriptor(); //  construct and initiliaze a descriptor
NA_EIDPROC 
  inline void	init();		// initialize a newly allocated descriptor
NA_EIDPROC 
  inline void	init(ULng32 allocatedSize, tupp_descriptor * relocatedAddress, char *tupleAddress);
NA_EIDPROC 
  inline unsigned short getReferenceCount() const;
NA_EIDPROC void setReferenceCount(unsigned short ref_count)
  {
    referenceCount_ = ref_count;
  }

  // resets the flags that were used ONLY while the tupp_desc are
  // being in transit between PA and EID. Once they are received,
  // and the flags are looked at, these must be initialized to zero
  // as they are overloaded with the reference count for this tupp.
NA_EIDPROC void resetCommFlags()
  {
    flags_ = 0;
  }

NA_EIDPROC 
  inline char *getTupleAddress() const;
NA_EIDPROC 
  inline Lng32 getTupleOffset() const
    {
      return offset_;
    }
NA_EIDPROC 
  inline void setTupleOffset(Lng32 offset)
    {
      offset_ = offset;
    }
NA_EIDPROC 
  inline void setTupleAddress(char * tuple_address)
    {
      tupleAddress_ = tuple_address;
    }
NA_EIDPROC 
  inline void setRelocatedAddress(tupp_descriptor * relocated_address)
    {
      relocatedAddress_ = relocated_address;
    }
  
NA_EIDPROC 
  inline ULng32 getAllocatedSize(){return allocatedSize_;};

NA_EIDPROC
  inline void setAllocatedSize(Lng32 size) 
    {
      allocatedSize_ = size;
    }


////////////////////////////////////////////////////////////////////
// Data is sent between exe & dp2, and exe & esp, as control data
// and data data. The control data (see ex_io_control.h) includes
// information about queue state.
////////////////////////////////////////////////////////////////////
NA_EIDPROC
  NABoolean isControlTuple()
  {
    return (flags_ == CONTROL_TUPP);
  };

NA_EIDPROC
  NABoolean isDataTuple()
  {
    return (flags_ == DATA_TUPP);
  };

NA_EIDPROC
  NABoolean isDiagsTuple()
  {
    return (flags_ == DIAGS_TUPP);
  };

NA_EIDPROC
  NABoolean isStatsTuple()
  {
    return (flags_ == STATS_TUPP);
  };

NA_EIDPROC
  NABoolean isInvalidTuple()
  {
    return (flags_ == INVALID_DATA_VSBB_TUPP);
  };

NA_EIDPROC
 void setControlTupleType()
  {
    flags_ = CONTROL_TUPP;
  };
  
NA_EIDPROC
 void setDataTupleType()
  {
    flags_ = DATA_TUPP;
  };
  
NA_EIDPROC
 void setDiagsTupleType()
  {
    flags_ = DIAGS_TUPP;
  };
  
NA_EIDPROC
 void setStatsTupleType()
  {
    flags_ = STATS_TUPP;
  };
NA_EIDPROC
 void setInvalidTupleType()
  {
    flags_ = INVALID_DATA_VSBB_TUPP;
  };


};
#pragma warn(1506)   // warning elimination 


//
// Inline procedures
//

inline void tupp::release()
{
  // set the tuple desc pointer to null and decrement the reference count
  if (tuppDescPointer)
    {
      
      ExSqlTuppAssert(tuppDescPointer->referenceCount_ > 0 , 
		      "Releasing a free tuple descriptor");
      tuppDescPointer->referenceCount_--;
      tuppDescPointer = (tupp_descriptor *) 0;
    }
}

inline void tupp::init()
{
  // Init should be called to initialize a newly allocated tupp
  // If the tupp was created with a constructor there is no need to call
  // it. But if the tupp was allocated by creating space for it in another
  // data structure without calling it's constructor then it must be initialized
  
  // assert tuppDesc_pointer points to garbage and not a valid tuple descriptor
  tuppDescPointer = (tupp_descriptor *) 0;

  
};

inline void tupp::operator=(const tupp_descriptor *tp)
{
  // First release the target tupp
  release();

  tuppDescPointer = (tupp_descriptor*)tp;
  if(tuppDescPointer) tuppDescPointer->referenceCount_++;
  
  // cast away the const
  //register tupp_descriptor *td = (tupp_descriptor *) tp;
  
  // if the source is a null pointer then we are done
  //  if (td != NULL)
  //    {
  //      
  //      // Follow relocation chain, if any
  //      while (td->relocatedAddress_)
  //	td = td->relocatedAddress_;
  //      
  //      // set the tuple desc pointer in the tupp struct to the value provided
  //      // and increment the reference count
  //      tuppDescPointer = td;
  //      td->referenceCount_++;
  //    }
}

inline void tupp::operator=(const tupp & source)
{
  // Copying X=X should be a no-op. Do only if they are different.
  if(this != &source)
    {
      
      // first release the target
      release();
      
      // if the source is a null pointer then we are done. 
      if (source.tuppDescPointer)
	{
	  ExSqlTuppAssert(source.tuppDescPointer->referenceCount_ > 0, 
			  "Copying a free tuple descriptor");
	  
	  // if the source has been relocated then modify the pointer in the source tupp
	  //	  while (source.tuppDescPointer->relocatedAddress_)
	  //	    {
	  //	      source.tuppDescPointer->referenceCount_--;
	  //	      ((tupp &) source).tuppDescPointer = source.tuppDescPointer->relocatedAddress_;
	  //	      source.tuppDescPointer->referenceCount_++;
	  //	    };
	  
	  // increment the reference count of the tuple descriptor pointed to	
	  source.tuppDescPointer->referenceCount_++;
	  this->tuppDescPointer = source.tuppDescPointer;
	}
    }
}

inline char * tupp::getDataPointer() const
{
  if(tuppDescPointer) return tuppDescPointer->getTupleAddress();
  return NULL;

  //  if (tuppDescPointer != NULL)
  //    {
  //      // cast away the const
  //      register tupp_descriptor *td = (tupp_descriptor *)tuppDescPointer;
  //      
  //      // Follow relocation chain, if any, and relocate.
  //      while (td->relocatedAddress_)
  //	{
  //	  // this tuppDesc has been relocated. Make
  //	  // it point to the relocated address.
  //	  // Decrement the reference count for this
  //	  // tuppDesc and increment it for the relocated
  //	  // tuppDesc.
  //	  td->referenceCount_--;
  //	  td = td->relocatedAddress_;
  //	  td->referenceCount_++;
  //	}
  //      return td->getTupleAddress();
  //    }
  //  else
  //    return NULL;
};

inline unsigned short tupp::getRefCount() const
{
#pragma nowarn(1506)   // warning elimination 
  return tuppDescPointer ? tuppDescPointer->getReferenceCount() : 0;
#pragma warn(1506)  // warning elimination 
};

inline ULng32 tupp::getAllocatedSize() const
{
  return tuppDescPointer->allocatedSize_;
}

inline void tupp::setDataPointer(char *dp)
{
  tuppDescPointer->tupleAddress_ = dp;
};

inline tupp_descriptor* tupp::get_tupp_descriptor()
{
  return tuppDescPointer;
}


/////////////////////////////////////////////////
// class tupp_decriptor
/////////////////////////////////////////////////
inline	unsigned short tupp_descriptor::getReferenceCount() const
{
  return referenceCount_;
}

inline char *tupp_descriptor::getTupleAddress() const
{
  return tupleAddress_;
};

inline void tupp_descriptor::init(ULng32 allocatedSize,
				  tupp_descriptor *relocatedAddress, char *tupleAddress)
{
  referenceCount_ = 0;
  allocatedSize_  = allocatedSize;
  
  relocatedAddress_ = relocatedAddress;
  tupleAddress_     = tupleAddress;

  filler_ = 0;
};

inline void tupp_descriptor::init()
{
  referenceCount_ = 0;
  allocatedSize_  = 0;
  
  relocatedAddress_ = 0;
  tupleAddress_     = 0;

  filler_ = 0;
};

#endif

