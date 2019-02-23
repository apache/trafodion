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
 * File:         ext_tuple_desc.cpp
 * Description:  
 *
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "dfs2rec.h"


#include "ComPackDefs.h"
#include "exp_stdh.h"


// Uncomment this line to see the offsets computed.
// #define LOG_OFFSETS
#if defined( LOG_OFFSETS )
#include <stdlib.h>
#endif

ExpTupleDesc::ExpTupleDesc(UInt32 num_attrs, 
			   Attributes ** attrs,
			   UInt32 tupleDataLength,
			   TupleDataFormat tdataF,
			   TupleDescFormat tdescF,
			   Space * space)
  : numAttrs_(num_attrs),
    tupleDataLength_(tupleDataLength),
    tupleDataFormat_(tdataF),
    tupleDescFormat_(tdescF),
    NAVersionedObject(-1)
{
  if ( space != NULL )
    {
      // remember current allocated space size. Used at the end of
      // generation to find out total tuple desc length allocated.
      tupleDescLength_ = space->getAllocatedSpaceSize();

      flags_ = 0;
      attrs_ = 0;
  
#ifndef _DEBUG
      if (tdescF == LONG_FORMAT)
#endif
	{
	  // allocate an array of num_attrs Attributes*. This array follows
	  // 'this' class.
	  attrs_ = (AttributesPtr *)
	    (space->allocateAlignedSpace(num_attrs * sizeof(AttributesPtr))); 
      
	  for (UInt32 i=0; i < num_attrs; i++)
	    { 
	      // make a new copy of input attributes. This new attr entry
	      // will follow the attribute array.
	      attrs_[i] = attrs[i]->newCopy(space);
	    }
	}
  
      // and now find out the total length of the generated descriptor
      tupleDescLength_ = sizeof(*this) +
	space->getAllocatedSpaceSize() - tupleDescLength_;
    }
  else
    {
      flags_ = 0;
      attrs_ = attrs;
      tupleDescLength_ = 0x0ffff;
    }

  str_pad(fillers_, sizeof(fillers_), '\0');
}

ExpTupleDesc::ExpTupleDesc(UInt32 num_attrs, 
			   AttributesPtr * attrs,
			   UInt32 tupleDataLength,
			   TupleDataFormat tdataF,
			   TupleDescFormat tdescF)

  : numAttrs_(num_attrs),
    tupleDataLength_(tupleDataLength),
    tupleDataFormat_(tdataF),
    tupleDescFormat_(tdescF),
    NAVersionedObject(-1)
{
  // special case where space is null, used in ex_rcb.cpp for late bind.

  flags_ = 0;
  attrs_ = attrs;
  tupleDescLength_ = 0x0ffff;
}

ExpTupleDesc::ExpTupleDesc() : 
        NAVersionedObject(-1)
{
}

ExpTupleDesc::~ExpTupleDesc()
{
  // if ((tupleDescFormat_ == LONG_FORMAT) && (attrs_))
  //   {
  //     for (Int32 i=0; i < (Int32) numAttrs_; i++)
  //       {
  //         delete attrs_[i];
  //       }
  //     
  //     delete[] attrs_;
  //   }
  // 
  // attrs_ = 0;
}

//
// Takes a list of fixed fields and destructively rearranges the fixed fields
// such that all the 8-byte fields come first, followed by the 4-byte, then
// 2-byte, and then all remaining fields.  This is only done for non-added
// columns.  The added columns follow the original fixed columns and are NOT
// ordered on their byte boundary, but put in the list in the order they
// were added.
static Int16 orderFixedFieldsByAlignment( Attributes    ** attrs,
                                          NAList<UInt32> * fixedFields)
{
  Int16  rc = 0;

  Attributes *attr;
  NAList<UInt32> align8(NULL,4);
  NAList<UInt32> align4(NULL,10);
  NAList<UInt32> align2(NULL,10);
  NAList<UInt32> align1(NULL,4);
  NAList<UInt32> addedCols(NULL,4); // on C++ heap, wil get dellocated when
                                    // we return

  UInt32 fieldIdx = 0;
  UInt32 numFixed = fixedFields->entries();
  Int32  alignSz;
  UInt32 i;

  for( i = 0; i < numFixed; i++ )
  {
    fixedFields->getFirst( fieldIdx );
    attr    = attrs[ fieldIdx ];

    if (attr->isAddedCol())   // an added column
    {
      addedCols.insert( fieldIdx );
      continue;
    }

    alignSz = attr->getDataAlignmentSize();

    if ( alignSz == 8 )
      align8.insert( fieldIdx );
    else if ( alignSz == 4 )
      align4.insert( fieldIdx );
    else if ( alignSz == 2 )
      align2.insert( fieldIdx );
    else
      align1.insert( fieldIdx );
  }

  fixedFields->clear();

  if ( align8.entries() > 0 )
    for( i = 0; i < align8.entries(); i++ )
    {
      fixedFields->insert( align8[ i ] );
    }

  // Add all the 4-byte attributes next.
  if ( align4.entries() > 0 )
    for( i = 0; i < align4.entries(); i++ )
    {
      fixedFields->insert( align4[ i ] );
    }

  // Add all the 2-byte attributes next.
  if ( align2.entries() > 0 )
    for( i = 0; i < align2.entries(); i++ )
    {
      fixedFields->insert( align2[ i ] );
    }

  // Add all the remaining attributes if any.
  if ( align1.entries() > 0 )
    for( i = 0; i < align1.entries(); i++ )
    {
      fixedFields->insert( align1[ i ] );
    }

  // Add all the added column attributes in their logical order.
  if ( addedCols.entries() > 0 )
    for( i = 0; i < addedCols.entries(); i++ )
    {
      fixedFields->insert( addedCols[ i ] );
    }

  return rc;
}


// input is a single attribute
Int16 ExpTupleDesc::computeOffsets(Attributes    * attr,
				   TupleDataFormat tf,
				   UInt32&         datalen,
				   UInt32          start_offset)
{
  ExpTupleDesc::computeOffsets(1, &attr, tf,
			       datalen, 
			       start_offset); // start here
  
  return 0;
}

// This code is the same for fixed fields and GuOutput fields.
static
void computeOffsetOfFixedField(Attributes **  attrs,
			       UInt32         fieldIdx,
                               UInt32         bitmapOffset,
			       UInt32&        fixedOffset,
			       UInt32&        firstField,
			       UInt32&        prevIdx,
                               UInt16&        nullBitIdx,
                               ExpHdrInfo *   hdrInfo = NULL,
			       NABoolean      alignFormat = FALSE)
{
  Attributes *field = attrs[fieldIdx];

  if (prevIdx != ExpOffsetMax)
    attrs[prevIdx]->setNextFieldIndex(fieldIdx);

  if (alignFormat)
    {
      if ((field->isAddedCol()) && (field->getDataAlignmentSize() == 8))
        {
          fixedOffset = ADJUST(fixedOffset, 4);
        }
      else
        {
          fixedOffset = ADJUST(fixedOffset, field->getDataAlignmentSize());
        }
    }

  if (firstField == ExpOffsetMax)
  {
    // Only the first fixed field will have a VOA offset set.
    // This is used when writing SQLMX_FORMAT so the proper offset gets
    // written there.
    firstField = fixedOffset;
    if(!alignFormat)
      field->setVoaOffset(0);

    if (hdrInfo != NULL)
      hdrInfo->setFirstFixedOffset( firstField );
  }

  // Relative offet is to the very start of the field - before the null
  // indicator offset.
  field->setRelOffset(fixedOffset - firstField);

  if (field->getNullFlag())
  {
    if (alignFormat)
    {
      // The SQLMX_ALIGNED_FORMAT has a null bitmap - 1 bit per nullable field.
      // Each nullable field has the same null indicator offset that
      // points to the start of the bitmap and a bit within
      // the null bitmap that is set if the field is null.
      field->setNullBitIndex( nullBitIdx++ );
      field->setNullIndOffset( bitmapOffset );
    }
    else
    {
      // The SQLMX_FORMAT has the 2 byte null indicator field at the start
      // of the fields data.
      field->setNullIndOffset(fixedOffset);
      fixedOffset += field->getNullIndicatorLength();
    }
  }

  // A varchar could be a fixed field if the forceFixed flag is set.
  if (field->getVCIndicatorLength() > 0)
    {
      field->setVCLenIndOffset(fixedOffset);
      fixedOffset += field->getVCIndicatorLength();
    }

  field->setOffset(fixedOffset);
  fixedOffset += field->getLength();
  prevIdx = fieldIdx;
}


// Static method
//   Input is an array of num_attrs Attributes.
Int16 ExpTupleDesc::computeOffsets(UInt32 num_attrs,        /* IN  */
				   Attributes ** attrs,     /* IN  */
				   TupleDataFormat tf,      /* IN  */
				   UInt32 & datalen,        /* OUT */
				   UInt32 startOffset,      /* IN  */
				   UInt32 * rtnFlags,       /* OUT */
				   TupleDataFormat * outTf, /* OUT */
                                   ExpHdrInfo * hdrInfo,    /* OUT */
                                   UInt32 * headerSizePtr,  /* OUT */
                                   UInt32 * varSizePtr)     /* OUT */
{
  // start at startOffset.
  UInt32 offset = startOffset;
  UInt32 attrStartOffset;

  // The first fixecd field needs to add in the pad bytes if the data length
  // is extended and for Aligned Format.
  // This only needs to be done if there are no variable length fields present
  // and there is a hdrInfo passed in.  Data rows with a variable length field
  // adjust the data len at runtime.
  NABoolean adjustFirstFixed = FALSE;

  datalen = 0;
  
  if (outTf)
    *outTf = tf;

  // Compute length of tuple and assign offset to attributes.
  switch (tf)
    {
      case PACKED_FORMAT:
      {
        Int16 varchar_seen = 0;
        for (UInt32 i=0; i < num_attrs; i++)
          {
            attrStartOffset = offset;

            // set format in attrs
            attrs[i]->setTupleFormat(tf);
	    attrs[i]->setDefaultFieldNum(i);

            if (attrs[i]->getNullFlag())
              attrs[i]->setNullIndicatorLength(NULL_INDICATOR_LENGTH); 
            if (attrs[i]->getVCIndicatorLength() > 0)
              attrs[i]->setVCIndicatorLength(VC_ACTUAL_LENGTH); 
	    
            // all fields except char/varchar/binary fields may need alignment
            // at runtime.
            if (NOT DFS2REC::isAnyCharacter(attrs[i]->getDatatype()))
              attrs[i]->needDataAlignment();
	    
            if (varchar_seen)
              {
                // this field follows a varchar field. Cannot
                // determine offset at compile time. Remember 
                // the position of this field (as a negative number
                // coz all positive values could be valid offsets).
                attrs[i]->setAddedCol();
             	attrs[i]->setOffset(ExpOffsetMax);
		attrs[i]->setRelOffset(i);

                // For rows that have varchars in them, set datalen
                // to be the max datalen. Set 'offset' variable to reflect this.
                // Offset is used later to find
                // out the total length.
                offset += attrs[i]->getStorageLength();
              }
            else
              {
                if (attrs[i]->getVCIndicatorLength() > 0)
                  {
		    *rtnFlags = ExpTupleDesc::VAR_FIELD_PRESENT;
                    varchar_seen = -1;
                  }
		
                if (attrs[i]->getNullFlag())
                  {
                    attrs[i]->setNullIndOffset((Int32)offset);		    
                    offset += attrs[i]->getNullIndicatorLength();   
                  }
		
                if (attrs[i]->getVCIndicatorLength() > 0)
                  {
                    attrs[i]->setVCLenIndOffset(offset);		    
                    offset += attrs[i]->getVCIndicatorLength();
                  } 
		
                attrs[i]->setOffset(offset);
		
                offset += attrs[i]->getLength();
            
		if (attrs[i]->getRowsetSize() > 0) {
	          if (!(attrs[i]->getUseTotalRowsetSize())) {
                    // The space used by a rowset consists of four bytes to hold
                    // the number of entries, plus the space used by each array
                    // entry times the number of elements
		   attrs[i]->setOffset(attrStartOffset);
                   offset = (attrStartOffset +
                            ((offset - attrStartOffset) * attrs[i]->getRowsetSize())
                            + sizeof(Int32));
		  } 
		}

              } // none of the previous fields is a varchar
          } // for
      }
      break;
      
      case SQLMX_KEY_FORMAT:
      {
	for (UInt32 i=0; i < num_attrs; i++)
        {
            attrStartOffset = offset;
            // set format in attrs
            attrs[i]->setTupleFormat(tf);
	    
            if (attrs[i]->getNullFlag())
              attrs[i]->setNullIndicatorLength( KEY_NULL_INDICATOR_LENGTH ); 
            if (attrs[i]->getVCIndicatorLength() > 0)
              attrs[i]->setVCIndicatorLength( VC_ACTUAL_LENGTH ); 
	    
            // all fields except char/varchar/binary fields may need alignment
            // at runtime.
            if (NOT DFS2REC::isAnyCharacter(attrs[i]->getDatatype()))
              attrs[i]->needDataAlignment();
	    
            if (attrs[i]->getNullFlag())
              {
                attrs[i]->setNullIndOffset(offset);	
                offset += attrs[i]->getNullIndicatorLength();   
              }

            if (attrs[i]->getVCIndicatorLength() > 0)
              {
                attrs[i]->setVCLenIndOffset(offset);		
                offset += attrs[i]->getVCIndicatorLength();
              } 

            attrs[i]->setOffset(offset);
		 
            offset += attrs[i]->getLength(); // returns max len for varchars

            if (attrs[i]->getRowsetSize() > 0) {
	       if (!(attrs[i]->getUseTotalRowsetSize())) {
                // The space used by a rowset consists of four bytes to hold the 
		// number of entries, plus the space used by each array entry
		// times the number of elements
		attrs[i]->setOffset(attrStartOffset);
                offset = (attrStartOffset +
                         ((offset - attrStartOffset) * attrs[i]->getRowsetSize())
                          + sizeof(Int32));
               }
            }
        }
      }
      break;
      
      case SQLARK_EXPLODED_FORMAT:
      {
        for (UInt32 i=0; i < num_attrs; i++)
          {
            attrStartOffset = offset;
            // set format in attrs
            attrs[i]->setTupleFormat(tf);

	    UInt32 nullIndicatorOffset = 0;
	    UInt32 vcIndicatorOffset = 0;

	    offset = ExpTupleDesc::sqlarkExplodedOffsets(offset,
		       (UInt32) attrs[i]->getDataAlignmentSize(),
                       attrs[i]->getDatatype(),
                       attrs[i]->getNullFlag(),
		       &nullIndicatorOffset,
		       &vcIndicatorOffset,
		       attrs[i]->getVCIndicatorLength() ?
		        attrs[i]->getVCIndicatorLength() :
		       	ExpTupleDesc::VC_ACTUAL_LENGTH);
	    
            if (attrs[i]->getNullFlag())
              {
                attrs[i]->setNullIndOffset((Int32)nullIndicatorOffset);
              }

	    // All datatypes, if not aligned and if we don't move
	    // them to an aligned buffer at runtime, will trap and then
	    // get aligned by the system.
	    // This trap & alignement doesn't work for IEEE float datatypes
	    // which *must* be aligned at runtime before doing any float
	    // operations on them.
	    // Any row which is of the format, SQLARK_EXPLODED_FORMAT, should
	    // always start on an 8-byte boundary. But some operators do
	    // not do this correctly and that causes float operations on
	    // fields inside of that row to fail.
	    // Set the needDataAlignment flag out here for ieee float
	    // datatypes.
	    // This is not done if this is a rowsets as values inside
	    // of a rowset are already aligned.
            if ((attrs[i]->getRowsetSize() == 0) &&
                ((attrs[i]->getDatatype() == REC_FLOAT32) ||
		 (attrs[i]->getDatatype() == REC_FLOAT64)))
              attrs[i]->needDataAlignment();

            if (attrs[i]->getVCIndicatorLength() > 0)
              {
                attrs[i]->setVCLenIndOffset(vcIndicatorOffset);
              }
	    
            attrs[i]->setOffset(offset);

            offset += attrs[i]->getLength();
	                
	    if (attrs[i]->getRowsetSize() > 0) {
	      if (!(attrs[i]->getUseTotalRowsetSize())) {

		UInt32 elementDataLen = 0; 
                // The space used by a rowset consists of four bytes to hold the 
		// number of entries, plus the space used by each array entry
		// times the number of elements
		attrs[i]->setOffset(attrStartOffset);
                
		// For rowsets, the nullIndOffset_ and vcLenIndOffset_ has to be
		// set to match the code in CliExpExchange.cpp for input and
                // output.
		// The offsets for rowset COBOL VARCHAR attributes is set as 
		// follows:
	  	//   - offset_ points to the start of the rowset info followed
		//     by four bytes of the rowset size
                //   - nullIndOffset_ (if nullIndicatorLength_ is not set to 0)
                //     points to (offset_+4). 
		//   - vcLenIndOffset_ (if vcIndicatorLength_ is not set to 0)
                //     points to (offset_+nullIndicatorLength_)
                //   - The first data value starts at
                //     (offset_+nullIndicatorLength_+vcIndicatorLength_) or
		//     (offset_+vcIndicatorLength_) depending on whether 
		//   - nullIndicatorLength_ is valid or not.
		// Note vcIndicatorLength_ is set to sizeof(Int16) for rowset
		// SQLVarChars.
		if (attrs[i]->getNullFlag()){
		  attrs[i]->setNullIndOffset(attrStartOffset+sizeof(Int32));
		  elementDataLen += attrs[i]->getNullIndicatorLength();
		  if (attrs[i]->getVCIndicatorLength() > 0){
		    attrs[i]->setVCLenIndOffset(attrStartOffset+
		      attrs[i]->getNullIndicatorLength()+sizeof(Int32)); 
		    elementDataLen += attrs[i]->getVCIndicatorLength();
		  }
		}
		else {
		  if (attrs[i]->getVCIndicatorLength() > 0){
		    attrs[i]->setVCLenIndOffset(attrStartOffset+sizeof(Int32)); 
		    elementDataLen += attrs[i]->getVCIndicatorLength();
		  }
		}
		elementDataLen += attrs[i]->getLength();
		offset = (attrStartOffset +
                         (elementDataLen * attrs[i]->getRowsetSize())
                          + sizeof(Int32));
	      }
            }
          }
      }
      break;

      case SQLMX_ALIGNED_FORMAT:
      case SQLMX_FORMAT:
      {
	// Loop through all attributes processing the variable length fields
	// first to determine how many VOA[] (variable offset array)
	// entries there are.  The first fixed field is directly after the VOA.
	// The first variable length field is "saved" so the actual offset can
	// be set since this offset is directly after all fixed field offsets.
	// All other variable length fields will be computed when writing the
	// data to disk and the respective VOA[i] will be written to disk too.

	// The fixed fields are processed after all variable length fields.
	// Fixed field offsets are relative offsets added to the VOA[0]
	// value read at runtime.  This is to support add column in a lazy
	// fashion (ie. added columns only exist when data is actually
	// inserted and not for old data).

        NABoolean alignedFormat = (tf == SQLMX_ALIGNED_FORMAT);
	UInt32 voaIdxOff   = startOffset;
        UInt32 varLen = 0;
	UInt32 i      = 0;
        UInt32 nullableCnt = 0;  // assign a bit for each nullable field
        Int16  nullIndLen  = (Int16)(alignedFormat ? 0 : NULL_INDICATOR_LENGTH);
        Int16  varIndLen   = (Int16)(alignedFormat
                                     ? (Int16) ExpAlignedFormat::OFFSET_SIZE
                                     : (Int16) SQLMX_VC_ACTUAL_LENGTH);
        UInt32 voaSz       = (alignedFormat
                              ? ExpAlignedFormat::OFFSET_SIZE
                              : ExpVoaSize);

	// Keep a handle on the first variable length field.
	Attributes *firstVarField = NULL;

	// Lists to accumulate fixed and variable fields so we can process
        // them in their respective groups.
        // GU attributes are materialized view columns that are projected out
        // during an update.
        // Variable length fields get their VOA offset set here too.
	NAList<UInt32> fixedColumns(NULL,10);
	NAList<UInt32> varFields(NULL,10);
	NAList<UInt32> guFields(NULL,3);

        NAList<UInt32> *fixedFields = &fixedColumns;

        *rtnFlags = 0;

	for ( i = 0; i < num_attrs; i++ )
        {
          // set format in attrs
          attrs[i]->setTupleFormat(tf);

          if (attrs[i]->getNullFlag())
          {
            // Aligned format has the null bitmap at the start of the data
            // thus the length is 0 for each field.
            // The null indicator offset will be the offset to the bitmap
            // within the data record.  The actual bit index within the bitmap
            // will be stored in a new field.
            attrs[i]->setNullIndicatorLength( nullIndLen );

            nullableCnt++;
          }

          // all fields except char/varchar/binary fields may need to be aligned
          // at runtime
          if (NOT DFS2REC::isAnyCharacter(attrs[i]->getDatatype()))
            attrs[i]->needDataAlignment();

          // Handle variable length fields. Some varchars (aggregates)
          // are treated as a fixed field if the forceFixed flag is
          // set.
          if ( attrs[i]->getVCIndicatorLength() > 0 && !attrs[i]->isForceFixed())
          {
            // Variable length fields get their VOA offset set here,
            // and the first variable field is saved since this variable
            // fields offset can be calculated and set, where as accessing
            // all other variable fields is done via their VOA offset.
            if (firstVarField == NULL)
            {
              firstVarField = attrs[i];
              *rtnFlags |= ExpTupleDesc::VAR_FIELD_PRESENT;
            }

            // Different size for Packed and Aligned formats ...
            attrs[i]->setVCIndicatorLength( varIndLen );
            attrs[i]->setVoaOffset(voaIdxOff);
            voaIdxOff += voaSz;

            // get the total storage space (Null and VC ind. len's)
            varLen += attrs[i]->getStorageLength();

            varFields.insert(i);
          }
          else if ( attrs[i]->getVCIndicatorLength() > 0 && attrs[i]->isForceFixed())
          {
            // Varchars that are forceFixed are treated as fixed fields.

            // Different size for Packed and Aligned formats ...
            attrs[i]->setVCIndicatorLength( varIndLen );

            fixedFields->insert(i);
          }
          else if (attrs[i]->isGuOutput())
            guFields.insert(i);
          else   // have a fixed field, add it to list to process next
            fixedFields->insert(i);

          if (attrs[i]->isAddedCol())
          {
            *rtnFlags |= ExpTupleDesc::ADDED_COLUMN;

            // If the added column is nullable and using Aligned format,
            // then there's the possibility that the key may have shifted
            // since the bitmap array may need to grow to accomodate this.
            // This is very conservative approach and we can fine-tune this
            // by counting the number of nullable columns and only when the
            // null bitmap array needs to grow set this flag.
            // OR if the added column is a varchar then the key may be shifted
            // for short rows.  This is true for both formats.
            // OR if this is the first fixed field and it is an added column
            // then the key was a varchar and thus it is shifted now.
            if ( (attrs[i]->getNullFlag() && alignedFormat) ||
                 (attrs[i]->getVCIndicatorLength() > 0)     ||
                 (fixedFields->entries() == 1) )
              *rtnFlags |= ExpTupleDesc::KEY_SHIFT;
          }
        }

	if (varSizePtr != NULL)
	{
	  *varSizePtr = varLen;
	}

        if ( alignedFormat )
        {
          // This call destructively rearranges the list of fixed fields.
          orderFixedFieldsByAlignment( attrs, fixedFields);

          // No variable fields present so adjust the first fixed for the
          // pad bytes.
          if ( (NULL != hdrInfo) &&  (varFields.entries() == 0) )
            adjustFirstFixed = TRUE;
        }

	// Next, set the offsets for the accumulated fixed length fields.
	// The offset values are absolute offsets from the beginning of the
	// record.
        // Also setup a relative offset for Add Column support.  The relative
	// offset is used to determine if there are missing added column values.
        // This relative offset is relative to VOA[0] (the first fixed offset).
	Attributes *field  = NULL;
        UInt32 firstField  = ExpOffsetMax;
        UInt32 fieldIdx    = 0;
        UInt32 prevIdx     = ExpOffsetMax;
        UInt16 nullBitIdx  = 0;
        UInt32 bitmapOffset= 0;  // bitmap offset value
        UInt32 hdrSz;            // size in bytes of header
	UInt32 fixedOffset;      // first fixed offset value
        Int32  ffAlignSize = (fixedFields->entries()
                              ? attrs[fixedFields->at(0)]->getDataAlignmentSize()
                              : -1);

        fixedOffset = computeFirstFixedOffset(ffAlignSize, startOffset,
                                              voaIdxOff, tf,
                                              nullableCnt, hdrInfo,
                                              hdrSz, bitmapOffset);
        if (headerSizePtr)
          *headerSizePtr = fixedOffset;

        // Set offsets for all the fixed fields ...
	while( fixedFields->entries() )
	{
	  fixedFields->getFirst( fieldIdx );
	  computeOffsetOfFixedField(attrs,
                                    fieldIdx, 
                                    bitmapOffset,
	                            fixedOffset,
                                    firstField,
                                    prevIdx,
                                    nullBitIdx,
                                    hdrInfo,
				    alignedFormat);
	}

        // Set offsets for all the variable length fields ...
	firstField = ExpOffsetMax;
	while( varFields.entries() )
        {
          varFields.getFirst( fieldIdx );
          field = attrs[fieldIdx];

          if (prevIdx != ExpOffsetMax)
            attrs[prevIdx]->setNextFieldIndex(fieldIdx);

          // bump each voa offset by the header size
          field->setVoaOffset( hdrSz + field->getVoaOffset() );

          // The first variable length field handled like a fixed field
          // since the offset can be computed at compile time.
          if (firstField == ExpOffsetMax)
          {
            UInt32 firstVarOffset;

            firstField = fieldIdx;

            // set the first variable length field's offset to a
            // valid offset (at the end of all fixed fields)
            firstVarOffset = fixedOffset;

            if (field->getNullFlag()  &&  (NOT alignedFormat) )
            {
              field->setNullIndOffset( firstVarOffset );
              firstVarOffset += field->getNullIndicatorLength();
            }

            field->setVCLenIndOffset(firstVarOffset);

            firstVarOffset += field->getVCIndicatorLength();
            field->setOffset(firstVarOffset);
          }

          if ( field->getNullFlag() && alignedFormat )
          {
            // Each nullable field in the aligned format
            // has the same null indicator offset that
            // points to the start of the bitmap and a bit within
            // the null bitmap that is set if the field is null.
            field->setNullBitIndex( nullBitIdx++ );
            field->setNullIndOffset( bitmapOffset );
          }

          prevIdx = fieldIdx;
        }

	// Repeat the fixed field treatment for GuOutput fields.
	while( guFields.entries() )
	{
	  guFields.getFirst( fieldIdx );
	  computeOffsetOfFixedField(attrs, fieldIdx, bitmapOffset, fixedOffset,
	                            firstField, prevIdx, nullBitIdx);
	}

	// add in the max length of exploded variable length fields
	// since we must allocate space for maximum field values
	offset = fixedOffset + varLen;

        // Align format may have to pad the record out 1 - 3 bytes to
        // ensure each record starts on a 4-byte boundary.
	if (alignedFormat)
        {
          if (adjustFirstFixed)
          {
            offset = hdrInfo->adjustFirstFixed( offset );
          }
          else
          {
            offset = ADJUST(offset, ExpAlignedFormat::ALIGNMENT);
          }
        }
        
        nullBitIdx  = 0;
	for ( i = 0; i < num_attrs; i++ )
        {
         if (attrs[i]->getNullFlag())
          {
           attrs[i]->setNullBitIndex(nullBitIdx++ );
          }          
        }

        if(fixedFields->entries() &&
          ((*rtnFlags & ADDED_COLUMN) > 0))
          *rtnFlags |= ExpTupleDesc::ADDED_FIXED_PRESENT;

#if defined( LOG_OFFSETS )
        fprintf(stderr, "RowLen: %d \n", offset);
        for(Int32 k = 0; k < (Int32)num_attrs; k++)
        {
          Attributes *attr = attrs[k];
          fprintf(stderr,
                  "  Attr(%d): dataType: %d nullable: %d variable: %d "
                  "offset: %d voaOff: %d align: %d\n",
                  k, attr->getDatatype(), attr->getNullFlag(),
                  (attr->getVCIndicatorLength() > 0 ? 1 : 0), attr->getOffset(),
                  attr->getVoaOffset(), attr->getDataAlignmentSize());
        }
#endif
      }
      break;

      default:
        break;
    } // switch tf
  
  datalen = offset - startOffset;

  return 0;
}

UInt32 ExpTupleDesc::sqlarkExplodedOffsets(
  UInt32 offset,
  UInt32 length,
  Int16 dataType,
  NABoolean isNullable,
  UInt32 * nullIndicatorOffset,
  UInt32 * vcIndicatorOffset,
  UInt32 vcIndicatorSize)
{
  if (isNullable)
    {
      offset = ADJUST(offset, ExpTupleDesc::NULL_INDICATOR_LENGTH);
      if (nullIndicatorOffset)
	*nullIndicatorOffset = offset;
      offset += ExpTupleDesc::NULL_INDICATOR_LENGTH;
    }

  if (DFS2REC::isAnyVarChar(dataType))
    {
      offset = ADJUST(offset, vcIndicatorSize);
      if (vcIndicatorOffset)
        *vcIndicatorOffset = offset;
      offset += vcIndicatorSize;
    }
	    
  // column value alignment
  if (DFS2REC::isNumeric(dataType))
    offset = ADJUST(offset, length);

  return offset;
}

Long ExpTupleDesc::pack(void * space)
{
  if (! packed())
    {
      if (attrs_) attrs_.pack(space, numAttrs_);
    }
  flags_ |= PACKED;
  return NAVersionedObject::pack(space);
}

Int32 ExpTupleDesc::unpack(void * base, void * reallocator)
{
  if (packed())
    {
      if (attrs_)
        if (attrs_.unpack(base, numAttrs_, reallocator)) return -1;

      flags_ &= ~PACKED;
    }
  return NAVersionedObject::unpack(base, reallocator);
}

Int16 ExpTupleDesc::operator==(ExpTupleDesc * other)
{
  if ((numAttrs_ != other->numAttrs_) ||
      (tupleDataFormat_ != other->tupleDataFormat_))
    return 0;

  for (UInt32 i = 0; i < numAttrs_; i++)
    {
      if (! (*getAttr(i) == *other->getAttr(i)))
	return 0;
    }

  return -1;
}

// assigns the atp and atp_index value to all attributes
void ExpTupleDesc::assignAtpAndIndex(Int16 atp, Int16 atp_index)
{
  // attributes must be unpacked before they could be referenced
  if ((attrs_) && (! packed()))
    {
      for (UInt32 i=0; i < numAttrs_; i++)
	{
	  attrs_[i]->setAtp(atp);
	  attrs_[i]->setAtpIndex(atp_index);
	}
    }
}

void ExpTupleDesc::display(const char* title)
{
   if (title)
      cout << title;
   else
      cout << "ExpTupleDesc::display()";

   cout << endl;

   UInt32 attrs = numAttrs();
   cout << "this=" << this << ", num of attrs=" << attrs << endl;

   for (Int32 j=0; j<attrs; j++) {
       Attributes* attr = getAttr(j);
       Int16 dt = attr->getDatatype();
       UInt32 len = attr->getLength();
       cout << j << "th attr: dt=" << dt << ", len=" << len << endl;
   }
}

