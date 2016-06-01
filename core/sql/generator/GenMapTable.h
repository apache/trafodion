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
#ifndef GEN_MAPTABLE_H
#define GEN_MAPTABLE_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         GenMaptable.h
 * Description:  The map table. Used to keep track of value ids and
 *               their address where this value will be found at runtime.
 *               
 *               
 * Created:      4/15/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "AllItemExpr.h"
#include "ItemExprList.h"
#include "exp_attrs.h"

/////////////////////////////////////////////////////////////////
// class MapInfo
//   Contains information about a value id, its type attributes
//   and its buffer attributes. 
//   
//   Type attributes for a value id contain datatype, length
//   etc. See exp_attrs.h for detail on type attributes.
//
//   Buffer attributes for a value id define where that
//   value will be available at runtime. See Executor Internal
//   spec and/or files ex_queue.h and sql_tupp.h in executor
//   directory.
/////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////
// Important note :
// The design of MapInfo and MapTable cannot
// be declared as a stack variables. They will always be created
// from the dynamic memory ( i.e. new ). So the CollHeap* in
// NABasicObject can be used for furthur memory allocation.
//
/////////////////////////////////////////////////////////////////


class MapInfo // : public NABasicObject
{
  ValueId value_id;
  Attributes * attr_; // contains type and buffer attributes.
  ULng32 flags;

  enum {CODE_GENERATED = 0x0001, MARKED = 0x0002};

public:
  // MapInfo(const ValueId & value_id_, Attributes * attr);

  MapInfo() 
  {
    init();
  }

  void init()
  {
    flags = 0;

#ifdef _DEBUG
    // Initialize the attribute pointer to repeating sequence of 1011.
    // Make it eye-catching and if we ever try dereferencing
    // an odd pointer, it'll crash.
    attr_ = (Attributes *) 0xBBBBBBBB;
#endif
  }

  // ~MapInfo() {}
  
  inline ValueId getValueId(){return value_id;};
  
  inline Attributes * getAttr(){return attr_;};

  // returns 1, if code has been generated for this value id.
  inline ULng32 isCodeGenerated()
  {
    return flags & CODE_GENERATED;
  }

  // remembers that code has been generated.
  inline void codeGenerated(){flags |= CODE_GENERATED;};
  inline void resetCodeGenerated() { flags &= ~CODE_GENERATED; };

  inline void setMark(){flags |= MARKED;};
  inline void clearMark(){flags &= ~MARKED;};
  inline ULng32 marked() { return flags & MARKED; };

  short isOffsetAssigned();

  // Set the value id and attributes for this map info.
  void set( const ValueId & valueId, Attributes * attr, CollHeap * heap );

};


class MapInfoContainer
{
#define MICAsize 8  // Map Info Container Array size

public:
  MapInfoContainer(CollHeap * heap)
  {
    mapInfoArray_ = (MapInfo*)(new(heap) char[sizeof(MapInfo) * MICAsize]);
    for (Int32 i = 0; i < MICAsize; i++)
      {
	mapInfoArray_[i].init();
      }

    next_ = NULL;
  }

  MapInfoContainer * next_;
  MapInfo * mapInfoArray_;
};

/////////////////////////////////////////////////////////////////////
// class MapTable
//
// Any value id that is to be used in an expression is added to
// 'the map table' and is retrieved when code is being generated
// for the expression which uses that value id. With each value
// id, 'the map table' stores its type and buffer attributes (see,
// class MapInfo).
//
// 'the map table' is really a list of class MapTable's. To
// retrieve attributes of a value id, the list of map tables is
// searched until it is found. Attributes are retrieved by calling
// getMapInfo. An assertion is raised if it is not found.
//
// There are also times when we need to know if a value is available
// in a given map table. In this case, we don't want to raise an
// assertion. This could happen when values are being added to the
// map table. In this case, getMapInfoAsIs is called
//
// Value ids are normally added to the last map table in the list
// by calling addMapInfo. However, if it is to be added to a particular
// map table, then addMapInfoToThis should be called.
//
/////////////////////////////////////////////////////////////////////

class MapTable : public NABasicObject
{
  // ---------------------------------------------------------------------
  // The map table contains the following:
  // 1. an array of value id bitmaps
  // 2. an array of integers showing the number of value ids in each bitmap.
  // 3. an array of pointers to map info.
  // 4. a cache for the < value id, pointer to map info> tuple last accessed.
  // 5. some helpful counters
  //
  // The value id bitmap is shown as follows:
  //
  // value id:   0   1   2   3   4   5    ...    31
  //           -------------------------------------
  //           |   |   |   |   |   |   |  ...  |   |
  //           -------------------------------------
  //      bit:   0   1   2   3   4   5    ...    31
  //
  // Example:
  // Suppose we have the value ids { 57, 60, 61, 64, 91 } in this map table:
  //  value id bitmap array = < 0x0, 0x4c, 0x8000 0010, 0x0 >
  //          integer array = < 0,   3,    2,           0   >
  // map info pointer array = < 0x4def2380, 0x4def26f0, ... >
  // correspond to value id = < 57,         60,         ... >
  //                  cache = < 57, 0x4def2380 >
  //        total value ids = 5
  // ---------------------------------------------------------------------

private:
  friend class Generator;

  // ---------------------------------------------------------------------
  // The unit here is an unsigned integer (assume it is a 32-bit integer).
  // If we use a different unit, then change the defines below.
  // NOTE: Do NOT use a signed data type!
  // For explanation, see the comments in getIndexIntoMapInfoPtrArray()
  // function.
  // ---------------------------------------------------------------------
  typedef UInt32 MTBitmapUnit;  // prefix MT stands for map table
  #define bitsPerUnit	      32
  #define bitsPerUnitMinus1   31

  // ---------------------------------------------------------------------
  // Bitmap for value ids in the map table.
  // ---------------------------------------------------------------------
  #define initMTBAsize 8              // initial map table bitmap array size
  Int32            vidBitMapArraySize_; // size of value id bitmap array
  MTBitmapUnit * vidBitMapArray_;     // array of value id bitmaps
  short        * vidsInBitMapArray_;  // array of number of value ids in bitmap
  Int32            totalVids_;          // total number of value ids in map table
                                      // it is the sum of vidsInBitMapArray_

  // ---------------------------------------------------------------------
  // Map infos in the map table.
  // ---------------------------------------------------------------------
  #define initMTMIPAsize 8            // initial map table map info array size
  #define mapInfoPtrArrayStepSize 8   // increase the map info array size by 8
  Int32        mapInfoPtrArraySize_;    // size of map info array ptr
  MapInfo ** mapInfoPtrArray_;        // array of map infos ptrs

  // ---------------------------------------------------------------------
  // List of MapInfoContainer's
  // ---------------------------------------------------------------------
  MapInfoContainer * firstMapInfoContainer_;
  MapInfoContainer * lastMapInfoContainer_;

  // ---------------------------------------------------------------------
  // Next map table.
  // ---------------------------------------------------------------------
  MapTable * next_;

  // ---------------------------------------------------------------------
  // Previous map table.
  // ---------------------------------------------------------------------
  MapTable * prev_;
  
  // ---------------------------------------------------------------------
  // getBits()
  //
  // Get the bits that we are interested in.
  // ---------------------------------------------------------------------
  inline MTBitmapUnit getBits( const CollIndex valueId, const Int32 whichMap );

  // ---------------------------------------------------------------------
  // getIndexIntoMapInfoPtrArray()
  //
  // Get the index into the map info array.
  // Inputs: whichMap -- the bitmap the value id is in
  //         inBits   -- the relevant bits that we're interested in
  // ---------------------------------------------------------------------
  Int32 getIndexIntoMapInfoPtrArray( const Int32    whichMap, 
                                          MTBitmapUnit inBits    ) const;

public:


  void setAllAtp(short Atp);
  void resetCodeGen();

  void shiftAtpIndex(short shiftIndex);

#ifdef _DEBUG
  // for debugging
  void print();
  void printToFile(FILE *f = stdout);
#endif

protected:
  // ---------------------------------------------------------------------
  // Constructor.
  // ---------------------------------------------------------------------
  MapTable() :
       vidBitMapArraySize_(0)
    , totalVids_( 0 )
    , mapInfoPtrArraySize_(0)
    , next_( NULL )
    , prev_( NULL )
    , firstMapInfoContainer_(0)
    , lastMapInfoContainer_(0)
  {
  }
  
  // ---------------------------------------------------------------------
  // Destructor.
  // ---------------------------------------------------------------------
  ~MapTable() 
  {
    if (vidBitMapArraySize_ > 0)
      NADELETEBASIC(vidBitMapArray_, collHeap());
    if (mapInfoPtrArraySize_ > 0)
      NADELETEBASIC(mapInfoPtrArray_, collHeap());
    
    MapInfoContainer * curr = firstMapInfoContainer_;
    while (curr)
      {
	MapInfoContainer * next = curr->next_;
	NADELETEBASIC(curr->mapInfoArray_, collHeap());
	NADELETEBASIC(curr, collHeap());
	curr = next;
      }
  }

  // adds to 'this' map table
  MapInfo * addMapInfoToThis(const ValueId &value_id,
			     Attributes * attr);
  
  // searches for value_id in the list of map tables
  // starting at 'this'. Returns MapInfo, if found.
  // Raises assertion, if not found.
  MapInfo * getMapInfoFromThis(const ValueId & value_id);

  MapTable* &next() {return next_;};
  MapTable* &prev() {return prev_;};

  Int32 getTotalVids() { return totalVids_;};
};


// ---------------------------------------------------------------------
// getBits()
//
// Get the bits that we are interested in.
// ---------------------------------------------------------------------

MapTable::MTBitmapUnit MapTable::getBits( const CollIndex valueId, const Int32 whichMap )
{
  // Get the bitmap and right shift away the bits that we don't care about.
  
#ifdef _DEBUG
  
  MTBitmapUnit bitmap = *(vidBitMapArray_ + whichMap);
  MTBitmapUnit bits   = bitmap >> (bitsPerUnitMinus1 - (valueId % bitsPerUnit));
  return bits;
  
#else
  
  return ( *(vidBitMapArray_ + whichMap) 
           >> (bitsPerUnitMinus1 - (valueId % bitsPerUnit))
	   );
  
#endif
}

#endif /* GEN_MAPTABLE_H */

