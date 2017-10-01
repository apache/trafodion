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
 *****************************************************************************
 *
 * File:         GenMapTable.C
 * Description:  Methods to handle the map table
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


#include "GenMapTable.h"
#include "ComOptIncludes.h"
#include "dfs2rec.h"
#include "Generator.h"
#include "GenExpGenerator.h"

//////////////////////////////////////////////////////////////////
// class MapInfo
/////////////////////////////////////////////////////////////////
void MapInfo::set(const ValueId & valueId, Attributes * attr, CollHeap * heap)
{
  ValueId l_val_id = value_id = valueId;

  const ItemExpr *iePtr = l_val_id.getValueDesc()->getItemExpr();
  const OperatorTypeEnum operatorType = iePtr->getOperatorType();

  if ( operatorType == ITM_VALUEIDREF )
    {
      l_val_id = ((ValueIdRef *)iePtr)->isDerivedFrom();
    }
  else if ( operatorType == ITM_INDEXCOLUMN )
    {
      l_val_id = ((IndexColumn *)iePtr)->getDefinition();
    }
  
  // get the data attributes(datatype, length, etc) for this value id.
  attr_ = ExpGenerator::convertNATypeToAttributes( l_val_id.getType(),
						   heap
						   );
  
  // remember the address(buffer location at runtime)
  // attributes for this value id, if attr is passed in.
  if (attr)
    attr_->copyLocationAttrs(attr);
  
};

short MapInfo::isOffsetAssigned()
  {
    if (attr_->getOffset() >= 0)
      return -1;
    else
      return 0;
  };

 
//////////////////////////////////////////////////////////////////
// class MapTable
/////////////////////////////////////////////////////////////////

// ---------------------------------------------------------------------
// addMapEntry()
//
// Add an map info object into this map table.
// Return the pointer to the map info added.
// ---------------------------------------------------------------------

MapInfo * MapTable::addMapInfoToThis(const ValueId & value_id, 
				     Attributes * attr)
{
  const CollIndex valueId = value_id;
  const Int32 whichMap      = valueId / bitsPerUnit;

  // --------------------------------------------------
  // Do we need to allocate more memory for the bitmaps?
  // --------------------------------------------------
  if ( whichMap >= vidBitMapArraySize_ )
    {
      // Double the size needed.
      const Int32 newBitMapArraySize = ((vidBitMapArraySize_ == 0) 
				      ? (MAXOF(initMTBAsize, 2 * whichMap))
				      : (2 * whichMap));
      
      // How many bytes is needed for the bitmap array and the integer array.
      const Int32 bytesToAllocate = 
	newBitMapArraySize * (sizeof(MTBitmapUnit) + sizeof(short));
      
      // Allocate memory from statement heap, which is always defined when
      // compiling a SQL statement.
      char * buffer = new(collHeap()) char[ bytesToAllocate ];
      
      // It is necessary to zero out memory for the bitmap array and
      // initialize the integer array to zero.
      memset( buffer, 0, bytesToAllocate );
      
      // Assign the allocated memory to the data members.
      MTBitmapUnit * newVidBitMapArray    = (MTBitmapUnit *) &buffer[ 0 ];
      short        * newVidsInBitMapArray = 
	(short *) &buffer[ newBitMapArraySize * sizeof(MTBitmapUnit) ];
      
      if (vidBitMapArraySize_ > 0)
	{
	  // Copy the existing info to the new location.
	  memcpy(
	       newVidBitMapArray,
	       vidBitMapArray_,
	       vidBitMapArraySize_ * sizeof(MTBitmapUnit)
	       );
	  memcpy(
	       newVidsInBitMapArray,
	       vidsInBitMapArray_,
	       vidBitMapArraySize_ * sizeof(short)
	       );
	  
	  // Deallocate the value id bitmap array is sufficient.
	  // Can you figure out why?
	  NADELETEBASIC( vidBitMapArray_, collHeap() );
	}
      
      // Assign the new bitmaps to the map table.
      vidBitMapArray_     = newVidBitMapArray;
      vidsInBitMapArray_  = newVidsInBitMapArray;
      vidBitMapArraySize_ = newBitMapArraySize;
    }
  
  // --------------------------------------------------
  // Do we need to allocate memory for the map info?
  // --------------------------------------------------
  if ( totalVids_ >= mapInfoPtrArraySize_ )
    {
      Int32 newMapInfoPtrArraySize = 
	((mapInfoPtrArraySize_ == 0)
	 ? initMTMIPAsize
	 : mapInfoPtrArraySize_ + mapInfoPtrArrayStepSize);
      
      MapInfo ** newMapInfoPtrArray = 
	new(collHeap()) MapInfo * [ newMapInfoPtrArraySize ];
      
#ifdef _DEBUG
      // Initialize the map info array to repeating sequence of 1011.
      // Make it eye-catching and if we ever try dereferencing
      // an odd pointer, it'll crash.
      
      for ( Int32 i = 0; i < newMapInfoPtrArraySize; i++ )
	{
	  newMapInfoPtrArray[i] = (MapInfo *)0xBBBBBBBB;
	}
#endif
      
      if (mapInfoPtrArraySize_ > 0)
	{
	  // Copy the existing info to the new location.
	  memcpy( 
	       newMapInfoPtrArray, 
	       mapInfoPtrArray_, 
	       mapInfoPtrArraySize_ * sizeof(MapInfo *)
	       );
	  
	  // Deallocate memory for old map info array.
	  NADELETEBASIC( mapInfoPtrArray_, collHeap() );
	}
      
      // Assign the new map info array to the map table.
      mapInfoPtrArray_     = newMapInfoPtrArray;
      mapInfoPtrArraySize_ = newMapInfoPtrArraySize;
    }
  
  // --------------------------------------------------
  // Get the index into the map info array.
  // --------------------------------------------------
#ifdef _DEBUG
  MTBitmapUnit bits = getBits( valueId, whichMap );
#endif
  
  Int32 index = getIndexIntoMapInfoPtrArray( whichMap, 
                                           getBits( valueId, whichMap ) 
					   );
  
  // --------------------------------------------------
  // Move map infos around if necessary.
  // --------------------------------------------------
  Int32 mapInfosLeftToMove;
  if ( (mapInfosLeftToMove = totalVids_ - index) > 0 )
    {
      // Some map infos are behind me in the map info array.  Move them.
      // This will happen when value ids are not inserted in ascending order.
      
      // Let's move 8 pointers at a time.
#define moveXMapInfoPtrsAtATime 8
      MapInfo * tmpMapInfoPtrArray[ moveXMapInfoPtrsAtATime ];
      
      Int32 end = totalVids_;
      while ( mapInfosLeftToMove > 0 )
	{
	  const Int32 ptrsToMove = ( mapInfosLeftToMove > moveXMapInfoPtrsAtATime )
	    ? moveXMapInfoPtrsAtATime 
	    : mapInfosLeftToMove;
	  
	  const Int32 bytesToMove = sizeof(MapInfo *) * ptrsToMove;
	  
	  // Copy to a local array.
	  memcpy( 
	       tmpMapInfoPtrArray, 
	       &mapInfoPtrArray_[end - ptrsToMove],
	       bytesToMove
	       );
	  
	  // Copy back to the original array.
	  memcpy( 
	       &mapInfoPtrArray_[end + 1 - ptrsToMove], 
	       tmpMapInfoPtrArray, 
	       bytesToMove 
	       );
	  
	  end -= moveXMapInfoPtrsAtATime;
	  mapInfosLeftToMove -= moveXMapInfoPtrsAtATime;
	}
    }
  
  // --------------------------------------------------
  // Finally, the real deal.
  // --------------------------------------------------
  
  // Flip the bit corresponding to the value id 'ON'
  // by left shifting to the correct bit position.
  *(vidBitMapArray_ + whichMap) |= 
    (0x1 << (bitsPerUnitMinus1 - (valueId % bitsPerUnit)));
  
  // Allocate a new MapInfo entry and insert it into the map info array.
  Int32 containerIndex = totalVids_ % MICAsize;
  if (containerIndex == 0)
    {
      MapInfoContainer * mic = new(collHeap()) MapInfoContainer(collHeap());
      if (firstMapInfoContainer_ == NULL)
	{
	  firstMapInfoContainer_ = mic;
	  lastMapInfoContainer_ = mic;
	}
      else
	{
	  lastMapInfoContainer_->next_ = mic;
	  lastMapInfoContainer_ = mic;
	}
    }
  
  MapInfo * mapEntry =&(lastMapInfoContainer_->mapInfoArray_[containerIndex]);
  
  //  MapInfo * mapEntry = new(collHeap()) MapInfo();
  mapEntry->set(value_id, attr, collHeap());
  *(mapInfoPtrArray_ + index) = mapEntry;
  
  // Update counters.
  (*(vidsInBitMapArray_ + whichMap))++;
  totalVids_++;
  
  
#ifdef _DEBUG
  if (getenv("IM_DEBUG")) 
    cerr << "addMapEntry:      " << this << " " << value_id << "\t" 
	 << attr << "\t me=" << mapEntry << endl;
  
  // Sanity checks.
  Int32 numVids = 0;
  for ( Int32 i = 0; i < vidBitMapArraySize_; i++ )
    {
      numVids += vidsInBitMapArray_[i];
    }
  CMPASSERT( totalVids_ == numVids );
#endif
  
  // Return the map info.
  return mapEntry;
  
}


// ---------------------------------------------------------------------
// getMapEntry()
//
// Get a pointer to a MapInfo that corresponds to the value id.
// Return NULL if not found.
// ---------------------------------------------------------------------

MapInfo * MapTable::getMapInfoFromThis(const ValueId & value_id)
{
  // --------------------------------------------------
  // Find the bits in the bitmap that we're interested in.
  // --------------------------------------------------
  const CollIndex valueId = value_id;
  const Int32 whichMap = valueId / bitsPerUnit;
  
  // Range check.
  if ( whichMap >= vidBitMapArraySize_ )
    {
      // Value Id not found.  Out of bitmap range.
      return NULL;
    }
  
  MTBitmapUnit bits = getBits( valueId, whichMap );
  
  if ( 0 == (bits & 0x1) )
    {
      // Value Id not found.  It is not in this map table.
      return NULL;
    }
  
  
  // --------------------------------------------------
  // Return the map info pointer.
  // --------------------------------------------------
  
#ifdef _DEBUG
  
  Int32       index   = getIndexIntoMapInfoPtrArray( whichMap, bits );
  MapInfo * mapInfo = mapInfoPtrArray_[index];
  return mapInfo;
  
#else
  
  return *(mapInfoPtrArray_ + getIndexIntoMapInfoPtrArray( whichMap, bits ));
  
#endif // _DEBUG
  
}  

void MapTable::setAllAtp(short Atp)
{
  MapTable *me = this;
  
  do
    {
      for ( Int32 i = 0; i < me->totalVids_; i++ )
	{
#ifdef _DEBUG
	  MapInfo *info = *(me->mapInfoPtrArray_ + i);
	  Attributes *attr = info->getAttr();
#else
	  Attributes *attr = (*(me->mapInfoPtrArray_ + i))->getAttr();
#endif
	  
	  if ( (attr != NULL) && (attr->getAtpIndex() > 1) )
	    {
	      // Only change the atp if the ATP index is greater than 1.
	      // This excludes constants, temps, and persistents.
	      //
	      attr->setAtp( Atp );
	    }
	}
      
      me = me->next_;
    } 
  while ( me != NULL );
  
}

void MapTable::resetCodeGen()
{
  MapTable *me = this;
  
  do
    {
      for ( Int32 i = 0; i < me->totalVids_; i++ )
	{
	  MapInfo *info = *(me->mapInfoPtrArray_ + i);
          info->resetCodeGenerated();
	}
      
      me = me->next_;
    } 
  while ( me != NULL );
  
}

void MapTable::shiftAtpIndex(short shiftIndex)
{
  MapTable *me = this;
  
  do
    {
      for ( Int32 i = 0; i < me->totalVids_; i++ )
	{
#ifdef _DEBUG
	  MapInfo *info = *(me->mapInfoPtrArray_ + i);
	  Attributes *attr = info->getAttr();
#else
	  Attributes *attr = (*(me->mapInfoPtrArray_ + i))->getAttr();
#endif
	  
	  if ( (attr != NULL) && (attr->getAtpIndex() > 1) )
	    {
	      // Only change the atp if the original ATP index is greater 
	      // than 1. This excludes constants, temps, and persistents.
	      //
	      attr->setAtpIndex( attr->getAtpIndex() + shiftIndex );
	    }
	}
      
      me = me ->next_;
    }
  while ( me != NULL );
}
// ---------------------------------------------------------------------
// getIndexIntoMapInfoPtrArray()
//
// Get the index into the map info array.
// Inputs: whichMap -- the bitmap the value id is in
//         bits     -- the relevant bits that we're interested in
// ---------------------------------------------------------------------

Int32 MapTable::getIndexIntoMapInfoPtrArray( 
  const Int32 whichMap, 
  MTBitmapUnit inBits 
  ) const
{
#ifdef _DEBUG
  DCMPASSERT( whichMap >= 0 AND whichMap < vidBitMapArraySize_ );
#endif
  // --------------------------------------------------
  // It is the sum of the number of value ids that is
  // smaller than me in this map table.
  // --------------------------------------------------
  Int32 arrayIndex = 0;

  // --------------------------------------------------
  // First, add up the number of value ids in the bitmaps before me.
  // --------------------------------------------------
  for ( Int32 i = 0; i < whichMap; i++ )
    {
      arrayIndex += *(vidsInBitMapArray_ + i);
    }
  
  // --------------------------------------------------
  // Next, add up the number of value ids in the same bitmap as me.
  // --------------------------------------------------
  
#ifdef _DEBUG
  
  // This is done differently than the release version.
  // See the explanation below.
  
  for ( Int32 k = 0; k < bitsPerUnit && inBits != 0; k++ )
    {
      // Right shift 1 bit.
      // If the least significant bit is 'ON', add index by 1.
      inBits >>= 1;
      arrayIndex += (inBits & 0x1);
    }
  
  // By this time, all the bits should have been shifted and
  // none of the bits should be 'ON'.
  CMPASSERT( 0 == (inBits & 0x1) );
  
#else
  
  // Right shift 1 bit.  Keep looping as long as one of the bits is 'ON'.
  while ( 0 != (inBits >>= 1) )
    {
      // If the least significant bit is 'ON', add index by 1.
      arrayIndex += (inBits & 0x1);
    }
  
#endif // _DEBUG
  
  // Sanity check.
#ifdef _DEBUG
  DCMPASSERT( arrayIndex >= 0 AND arrayIndex <= totalVids_ );
#endif
  
  return arrayIndex;
}

#ifdef _DEBUG

void MapTable::print()
{
  printToFile(stdout);
}
void MapTable::printToFile(FILE *f)
{
  // header for each map table
  fprintf(f, "Map Table: %p\n", this);

  // print each map info (value id, unparsed text, attributes, if present)
  for ( Int32 i = 0; i < totalVids_; i++ )
    {
      MapInfo *info = mapInfoPtrArray_[i];

      Attributes *attr = info->getAttr();
      NAString unparsed;

      // ValueId <xx> (generated) (<unparsed>):
      fprintf(f, "  ValueId %d", (CollIndex) info->getValueId());
      if (info->isCodeGenerated())
	fprintf(f, " (generated)");
      else
        fprintf(f, "            ");
      info->getValueId().getItemExpr()->unparse(unparsed);
      fprintf(f, " (%s):\n", unparsed.data());

      if (attr)
	{
	  // atp = <atp>, atpindex = <atpindex>, offset = <offset>
	  // type = <fs type>, null = <nullable>
	  fprintf(f, "    atp = %2d", attr->getAtp());
	  fprintf(f, ", atpindex = %2d", attr->getAtpIndex());
	  fprintf(f, ", offset = %3d", attr->getOffset());
	  fprintf(f, ", fstype = %3d", attr->getDatatype());
          fprintf(f, ", tupleFormat = %2d", attr->getTupleFormat());
	  fprintf(f, ", %s\n", (attr->getNullFlag() ? 
				     "Nullable" : "Not nullable"));
	}
    }

  if (next_)
    next_->printToFile(f);
  else
    fprintf(f, "--------------------------------------------------------\n");
}

#endif



