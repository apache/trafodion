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

#ifndef EXP_ALIGNED_FORMAT_H
#define EXP_ALIGNED_FORMAT_H


//
// Class used at codegen time to record header information to later
// generate an ExHeaderClause from.
// This is not stored on disk and only used dynamically.
class ExpHdrInfo
{
private:
  UInt16 adminSz_;             // complete admin size (hdr + VOA + bitmap + pad)
  UInt16 bitmapEntryOffset_;   // offset to write bitmap offset to
  UInt16 bitmapOffset_;        // bitmap offset value
  UInt16 firstFixedOffset_;    // first fixed field offset
  UInt16 startOffset_;         // offset into data record where first fixed
                               // offset is written and all other offsets
  UInt16 unused_;

public:
  ExpHdrInfo()
    : adminSz_(0),
      bitmapEntryOffset_(0),
      bitmapOffset_(0),
      firstFixedOffset_(0)
  { };

  // Used by packed format to clear the first fixed offset area.
  void setValues(UInt32                 adminSize)
  {
    setValues(0, adminSize, 0, 0);
  }

  // Used by aligned format to clear up to the first fixed field.  Entire VOA
  // is cleared along with null bitmap area (if any).
  void setValues(UInt32                 startOffset,
                 UInt32                 adminSize,
                 UInt32                 bitmapEntryOffset,
                 UInt32                 bitmapOffset)
  {
    startOffset_ = (UInt16)startOffset,
    adminSz_ = (UInt16)adminSize;
    bitmapEntryOffset_ = (UInt16)bitmapEntryOffset;
    bitmapOffset_ = (UInt16)bitmapOffset;
  }

  void setFirstFixedOffset(UInt32       firstFixedOff)
  {  firstFixedOffset_ = (UInt16)firstFixedOff; }

  // The admin size is the entire width of the area that will be cleared
  // before any data columns are written to the data record.
  // For packed format this is just the first fixed field offset area, 
  // but for aligned format this encompasses the entire area up to the first
  // fixed field.
  UInt16 getAdminSize()
  {  return adminSz_; }

  UInt16 getBitmapOffset()
  {  return bitmapOffset_; }

  UInt16 getBitmapEntryOffset()
  {  return bitmapEntryOffset_; }

  UInt16 getFirstFixedOffset()
  {  return firstFixedOffset_; }

  // If there are only fixed fields, then adjust the first fixed offset
  // to include the pad bytes added at the end (if any).
  UInt32 adjustFirstFixed(UInt32 dataLen) ;

  UInt16 getStartOffset()
  {  return startOffset_; }

};


//
// Class to encapsulate the SQLMX_ALIGNED_FORMAT record header.
// The SQLMX_ALIGNED_FORMAT differs from the SQLMX_FORMAT in that there is
// a 4-byte offset to a null bitmap vector.  The null bitmap vector has a
// bit for every nullable column in the table.  If no nullable columns exist
// in the table, the offset is 0 and not bitmap is present in the record.
// The null bitmap offset value is directly after the first fixed field
// offset.
// The null bitmap vector itself can be 0, 4, 8, 12, ... to accomodate all
// the nullable columns.
//
// Aligned format
//   -----------------------------------------------------------------------
//   | FF | BO | VO1 | .. | VOn | bitmap | fixed | addFixed | var | addVar |
//   -----+----+-----+----+-----+--------+-------+----------+-----+---------
//
//   FF       = first fixed field offset, 4 bytes
//              the hi 2 bits of FF used to record # of bytes the record is
//              extended to ensure record is a 4-byte multiple
//   BO       = null bitmap offset (may be 0 if no nulls), 4 bytes
//   VO1, VOn = offsets to variable fields within record, 4 bytes each
//   bitmap   = 4 bytes (or a multiple of 4 bytes) for null bitmap
//              1 bit per nullable field
//              may not be there if no nullable columns
//   fixed    = original fixed fields re-arranged with all 8, 4, 2, 1 byte
//              alignment needs grouped and aligned correctly
//   addFixed = any added fixed fields, aligned on proper byte boundaries
//   var      = original variable length fields, first variable length field
//              aligned on a 4-byte boundary for length value
//   addVar   = all added variable fields packed together
//
//   Padding if needed:
//      - after bitmap & before first fixed field
//      - between last fixed field and any added fixed fields
//      - between any added fixed fields to align correctly
//      - between last added fixed or last fixed field and first variable
//        field to align variable fields 2 byte length correctly
//      - at the end of the data record to ensure record length a 4 byte
//        multiple
//
class ExpAlignedFormat
{
private:

  UInt32  firstFixed_;
  UInt32  bitmapOffset_;

public:

  enum ExpAlignedFormatConstants {
    NULL_IND_SIZE       = 0x00000000,
    VARIABLE_LEN_SIZE   = 0x00000004,     // variable length size
    OFFSET_SIZE         = 0x00000004,     // all offsets within a record
    ALIGNMENT           = 0x00000004,     // record alignment size
    BITS_PER_BYTE       = 0x00000008,
    PAD_SHIFT_BITS      = 0x0000000E,     // shift pad bytes by 14 bits
    OFFSET_MASK         = 0x00003FFF,     // 16383 max first fixed offset
    MAX_OFFSET          = OFFSET_MASK,
    PAD_BYTE_MASK       = 0x0000C000      // mask the hi 2 bits
  };


  ////////////////////////////////////////////////////////////////////////
  //      Class methods
  ////////////////////////////////////////////////////////////////////////

  //
  // Compute the number of bytes needed for the null bitmap.
  // This size is a multiple of 4-byte words.
  static UInt32 getNeededBitmapSize( UInt32     nullableColumnCount )
  {
    UInt32 bitmapSz = 0;
    if ( nullableColumnCount > 0 )
    {
      // divide by 2 ^ 5 add 1 multiply by 4 = number 4 bytes needed
      bitmapSz = ((nullableColumnCount >> 5) + 1) * ALIGNMENT;
    }
    return bitmapSz;
  }

  //
  // Get the size of the header - first fixed offset, bitmap offset.
  static UInt32 getHdrSize()
  {  return 2 * OFFSET_SIZE; }

  //
  // Compute the offset where the first fixed field resides based
  // on the number of nullable columns.
  // This method used at code generation time.
  static UInt32 computeFirstFixedOffset( Int32        ffAlignSize,
                                         UInt32       startOffset,
                                         UInt32       voaIdxOffset,
                                         UInt32       nullableColCount,
                                         ExpHdrInfo * hdrInfo,
                                         UInt32     & hdrSize,
                                         UInt32     & bitmapOffset )
  {
    UInt32 bitmapSize = 0;
    UInt32 hdrEntryOffset = startOffset + OFFSET_SIZE;
    UInt32 firstFixedOffset;

    hdrSize = getHdrSize();   // first fixed offset, bitmap offset

    // voaIdxOffset was initialized as startOffset then incremented from there
    // for each voa entry
    firstFixedOffset = voaIdxOffset + hdrSize;

    if ( nullableColCount > 0 )
    {
      // Determine the number of bytes needed to store the bitmap.
      bitmapSize = getNeededBitmapSize( nullableColCount );
      bitmapOffset = voaIdxOffset + hdrSize;

      firstFixedOffset += bitmapSize;
    }

    if (ffAlignSize > 1)
    {
      // Now determine the first fixed offset based on the alignment size
      // of the first fixed field.  Padding may have to be added to get the
      // correct alignment and this will be added immediately following
      // the bitmap offset value field.
      //
      //   | FF | BO | VO1 | ... | VOn | bitmap | pad | F1 | .... |
      //   --------------------------------------------------------
      //
      // Then the header size will be adjusted as well as the bitmap offset
      // value.
      // The first fixed alignment size may be -1 when no fixed fields reside
      // in the table.

      Int32 pad = (Int32)(firstFixedOffset % ffAlignSize);

      if (pad > 0)
      {
        pad = ffAlignSize - pad;

        firstFixedOffset += pad;
      }
    }

    if ( hdrInfo != NULL )
    {
      hdrInfo->setValues( startOffset,
                          firstFixedOffset - startOffset, // overall admin size
                          hdrEntryOffset,
                          bitmapOffset );
    }

    return firstFixedOffset;
  }

  //
  // Get the offset to the first fixed field.
  static UInt32 getFirstFixedOffset( char      *dataPtr )
  {
    return ((ExpAlignedFormat *)dataPtr)->getFirstFixedOffset();
  }

  //
  // Get the offset to the first fixed field.
  UInt32 getFirstFixedOffset()
  {
    return ( firstFixed_ & OFFSET_MASK );
  }

  //
  // Set the offset to the first fixed field.
  static void setFirstFixedOffset( char        *dataPtr,
                                   UInt32       firstFixedOffset )
  {
    ((ExpAlignedFormat *)dataPtr)->firstFixed_ = (UInt16)firstFixedOffset;
  }

  //
  // Set the offset to the first fixed field.
  void setFirstFixedOffset( UInt32              firstFixedOffset )
  {
    firstFixed_ = (UInt16)firstFixedOffset;
  }

  //
  // Get the offset to the first variable length field.
  static UInt32 getFirstVariableOffset( char   *dataPtr,
                                        UInt32  firstFixedOffset )
  {
    return(((ExpAlignedFormat *)dataPtr)->getFirstVariableOffset(firstFixedOffset));
  }

  //
  // Get the offset to the first variable length field.
  UInt32 getFirstVariableOffset( UInt32         firstFixedOffset )
  {
    // Jump past the first fixed offset and the bitmap offset.
    UInt32 hdrSz = getHdrSize();

    if ( ( bitmapOffset_ == hdrSz ) ||
         ( (bitmapOffset_ == 0) && (firstFixedOffset == hdrSz) ) )
      return 0;
    else
      return ( *((UInt16 *)(((char *)this) + hdrSz)) );
  }

  //
  // Get the offset at the specified VOA entry
  UInt32 getVoaEntry( UInt32                    voaOffset )
  {
    return( *((UInt16 *)((char *)this + voaOffset)) );
  }

  //
  // Set the variable offset entry.
  void setVoaOffset( UInt32                     voaOffset,
                     UInt32                     voaEntryValue )
  {
    *((UInt16 *)(((char *)this) + voaOffset)) = (UInt16)voaEntryValue;
  }

  static void incrVoaOffset( UInt32            &voaOffset )
  {
    voaOffset += OFFSET_SIZE;
  }

  //
  // Set the offset to the bitmap.
  static void setBitmapOffset( char            *dataPtr,
                               UInt32           offset )
  {
    ((ExpAlignedFormat *)dataPtr)->bitmapOffset_ = (UInt16)offset;
  }

  //
  // Set the offset to the bitmap.
  void setBitmapOffset( UInt32                  offset )
  {
    bitmapOffset_ = (UInt16)offset;
  }

  //
  // Get the current bitmap offset from the data record.
  static UInt32 getBitmapOffset( char          *dataPtr )
  {
    return ( ((ExpAlignedFormat *)dataPtr)->getBitmapOffset() );
  }

  //
  // Get the current bitmap offset from the data record.
  UInt32 getBitmapOffset()
  {
    return ( bitmapOffset_ );
  }

  NABoolean isValidBitmapOffset()
  {
    return( ((bitmapOffset_ > MAX_OFFSET) ||
             ( (bitmapOffset_ < (getHdrSize()) && bitmapOffset_ != 0) ) ||
             ( (getFirstFixedOffset() > 0) &&
               (bitmapOffset_ > getFirstFixedOffset()) ) )
            ? FALSE : TRUE );
  }

  UInt32 getBitmapEntryOffset()
  {
    return( OFFSET_SIZE );
  }

  //
  // Check if a specific column value is null or not.  The bit index
  // represents the nullable column.
  static NABoolean isNullValue( char           *bitmap,
                                UInt16          bitIndex )
  {
    UInt32 mask = ((UInt32)0x1 << (7 - (bitIndex & 7)));
    return ( ( ((UInt32)(bitmap[(bitIndex >> 3)])) & mask ) != 0 );
  }

  //
  // Set the specified null bit if bitmap exists.
  static void setNullValue( char               *dataPtr,
                            UInt32              bitIdx )
  {
    setNullBit( ((ExpAlignedFormat *)dataPtr)->nullBitmap(), bitIdx );
  }

  //
  // Set the specific bit index corresponding to the null column value.
  static void setNullBit( char                 *bitmap,
                          UInt32                bitIdx )
  {
    // (bitIdx & 7) is same as (bitIdx % 8)
    if ( bitmap )
      bitmap[ (bitIdx >> 3) ] |= ((UInt32)0x1 << (7 - (bitIdx & 7)));
  }

  //
  // Clear the specific bit index corresponding to the column value.
  static void clearNullBit( char               *bitmap,
                            UInt32              bitIdx )
  {
    if ( bitmap )
      bitmap[ (bitIdx >> 3) ] &= ~((UInt32)0x1 << (7 - (bitIdx & 7)));
  }

  static UInt32 getVarLength( char             *dataPtr )
  {
    UInt16 len;
    str_cpy_all( (char *)&len, dataPtr, sizeof(len) );
    return len;
  }

  static void setVarLength( char               *dataPtr,
                            UInt32              varLength )
  {
    str_cpy_all( dataPtr, (char*)&varLength, VARIABLE_LEN_SIZE );
  }

  //
  // Return the size in bytes of the header plus the variable field
  // array size.
  NABoolean isVOAComplete( char               * dataPtr,
                           UInt32               totalVariableFields,
                           UInt32               firstFixedOffset,
                           UInt32               firstVariableOffset,
                           UInt32             & voaSize,
                           UInt32             & endVoaOffset,
                           UInt32             & numVarFields )
  {
    NABoolean status = TRUE;
    ExpAlignedFormat *alignedFmt = (ExpAlignedFormat *)dataPtr;

    // Have 2 options here to find the end of the VOA:
    //  1) use the bitmap offset to determine end of VOA
    //  2) use first fixed offset if > 0, otherwise use first variable offset
    //     to determine end of VOA

    UInt32 hdrSz = getHdrSize();

    // Compute the maximum VOA plus header size (FF | BO).
    voaSize = (totalVariableFields * OFFSET_SIZE) + hdrSz;

    endVoaOffset = hdrSz;
    numVarFields = 0;

    // If there is a bitmap, then the bitmap offset is the end of the entire
    // VOA.
    // Otherwise it must be calculated walking back from the first field
    // since there may be padded after the bitmap to ensure the first field
    // is aligned correctly.
    if ( alignedFmt->bitmapOffset_ > 0 )
      endVoaOffset = alignedFmt->bitmapOffset_;
    else if ( firstFixedOffset > hdrSz )
    {
      UInt32 endVoa = firstFixedOffset - OFFSET_SIZE;
      while( endVoa >= hdrSz )
      {
        if ( *(UInt16 *)(dataPtr + endVoa) > 0 )
        break;

        endVoa -= OFFSET_SIZE;
      }
      endVoaOffset = endVoa + OFFSET_SIZE;
    }
    else
      endVoaOffset = firstVariableOffset;
      
    if ( totalVariableFields > 0 )
    {
      if ( firstVariableOffset == 0 )
        status = FALSE;
      else 
      {
        // Now see if the current VOA plus header size is equal to the max.
        // If so then the VOA is complete, otherwise there are missing variable
        // length columns.
        if ( endVoaOffset < voaSize )
        {
          status = FALSE;
        }
      }

      numVarFields = (endVoaOffset - hdrSz) / OFFSET_SIZE;
    }

    return status;
  }

  //
  // Get information all in one shot.
  void getRecordFormatInfo( UInt32              dataLen,
                            UInt32              firstFixedOffset,
                            UInt32              firstVariableOffset,
                            UInt32              numberNullFields,
                            UInt32            & firstVoaOffset,
                            UInt32            & endVoaOffset,
                            UInt32            & fixedFieldBlockSize,
                            UInt32            & bitmapOffset,
                            UInt32            & bitmapSize,
                            UInt32            & bitmapNeededSize )
  {
    // set the output parameters
    firstVoaOffset = getHdrSize();     // jump past | FF | BO |
    bitmapNeededSize = getNeededBitmapSize( numberNullFields );
    bitmapSize = getBitmapSize();
    bitmapOffset = bitmapOffset_;

    if (bitmapSize > bitmapNeededSize)
      bitmapSize = bitmapNeededSize;

    if ( firstFixedOffset == 0 )  // no fixed fields in data record
    {
      fixedFieldBlockSize = 0;
    }
    else if ( (firstVariableOffset == 0) ||
              (firstVoaOffset + bitmapSize) == firstFixedOffset )
    {
      fixedFieldBlockSize = dataLen - firstFixedOffset;
    }
    else
    {
      fixedFieldBlockSize = firstVariableOffset - firstFixedOffset;
    }

    endVoaOffset = (bitmapOffset_ > 0 ? bitmapOffset_
                     : (firstFixedOffset > 0
                        ? firstFixedOffset : firstVariableOffset));
  }

  //
  // Return the size of the bitmap (and any padding before the first fixed
  // field) in bytes.  Padding may be added after the null bitmap to align
  // the first fixed field correctly.
  static UInt32 getBitmapSize( char            *dataPtr )
  {
    return ((ExpAlignedFormat *)dataPtr)->getBitmapSize();
  }

  UInt32 getBitmapSize()
  {
    UInt32 bitmapSz = 0;
    UInt32 ff = getFirstFixedOffset();

    if ( bitmapOffset_ > 0 )
    {
      // The first fixed field offset may be 0 if there are no
      // fixed fields in the record.
      // If no fixed fields, then use the offset to the first variable
      // field to compute the size of the bitmap.
      if ( ff > 0 )
        bitmapSz = ff - bitmapOffset_;
      else
      {
        // read the first variable offset value
        //
        // | FF=0 | BO | VO1 | ..... | VOn | bitmap | V1   | ... | Vn |
        // ------------------------------------------------------------

        UInt32 firstVar = getFirstVariableOffset( ff );
        bitmapSz = ( firstVar - bitmapOffset_ );
      }

      bitmapSz = (bitmapSz / ALIGNMENT) * ALIGNMENT;
    }
    return bitmapSz;
  }

  //
  // Adjust the current length to have it be a multiple of the alignment size.
  // The added bytes are stored on the hi 2 bits of the first fixed field
  // offset.
  // Return the new length.
  static UInt16 adjustDataLength( UInt16        firstFixed,
                                  UInt32        dataLen,
                                  UInt32       &newLen )
  {
    UInt16 adjFirstFixed = firstFixed;

    newLen = ( ( ( ( dataLen - 1 ) / ExpAlignedFormat::ALIGNMENT ) + 1 )
               * ExpAlignedFormat::ALIGNMENT );

    if ( newLen > dataLen )
    {
      // Clear the hi 2 bits ...
      adjFirstFixed &= OFFSET_MASK;

      // Set the hi 2 bits ...
      adjFirstFixed |= ((newLen - dataLen) << PAD_SHIFT_BITS);
    }

    return adjFirstFixed;
  }

  //
  // Adjust the current length to have it be a multiple of the alignment size.
  // The added bytes are stored on the hi 2 bits of the first fixed field
  // offset.
  // Return the new length.
  static UInt32 adjustDataLength( char         *dataPtr,
                                  UInt32        currLen,
                                  Int32         alignmentSize,
                                  NABoolean     updateHeader = TRUE )
  /* for CIF update header should be FALSE since hash join and hash group by
   * use the header to store length info-- may be we need to change this in hash join and hash group
   * if turns out to be risky
   */
  {
    UInt32 newLen = currLen;

    if ( currLen > 0 )
    {
      UInt16 adjFF = adjustDataLength(((ExpAlignedFormat *)dataPtr)->firstFixed_,
                                      currLen,
                                      newLen);


      if ( newLen > currLen )
      {
        if (updateHeader)
        {
          ((ExpAlignedFormat *)dataPtr)->firstFixed_ = adjFF;
        }

        // Clear the pad bytes at the end of the data row.
        str_pad( dataPtr + currLen, newLen - currLen, '\0' );
      }
    }

    return newLen;
  }

  //
  // CTOR
  //   Constructor to clear the header information.
  ExpAlignedFormat()
    : firstFixed_(0),
      bitmapOffset_(0)
  { }

  //
  // Given an offset and an alignment boundary adjust the offset correctly.
  static UInt32 adjustOffset( UInt32            offset,
                              Int32             alignmentSize )
  {
    return( (offset > 0
             ? (((offset - 1) / alignmentSize) + 1) * alignmentSize
             : offset) );
  }

  //
  // Return the number of pad or filler bytes [0,7] needed to
  // extend the record to a proper boundary.  Need to ensure that every
  // record starts on a 4-byte boundary so we don't incur alignment traps.
  UInt32 getNumberFillerBytes()
  {
    return ( (firstFixed_ & PAD_BYTE_MASK) >> PAD_SHIFT_BITS );
  }

  //
  // Return the actual length minus any pad bytes added to round the row
  // size up to a 4-byte boundary.
  UInt32 getActualLength( UInt32                dataLen )
  {
    Int32 padBytes = ((firstFixed_ & PAD_BYTE_MASK) >> PAD_SHIFT_BITS);
    return( dataLen - padBytes );
  }

  void setNullValue( UInt32                     bitIdx )
  {
    setNullBit( nullBitmap(), bitIdx );
  }

  void clearNullValue( UInt32                   bitIdx )
  {
    clearNullBit( nullBitmap(), bitIdx );
  }

  NABoolean isNullValue( UInt32                 bitIdx )
  {
    Int32 mask = (1 << (7 - (bitIdx & 7)));
    char *bitmap = nullBitmap();
    NABoolean rtnValue = FALSE;

    if ( bitmap != NULL )
      rtnValue = ((bitmap[(bitIdx >> 3)]) & mask);

    return rtnValue;
  }

private:

  ////////////////////////////////////////////////////////////////////////
  //      Instance private methods
  ////////////////////////////////////////////////////////////////////////
  
  char *nullBitmap()
  {
    return ( bitmapOffset_ > 0 
             ? ((char *)this) + bitmapOffset_
             : NULL );
  }

};


inline UInt32 ExpHdrInfo::adjustFirstFixed(UInt32          dataLen)
{
  UInt32 newLen = dataLen;

  firstFixedOffset_ = ExpAlignedFormat::adjustDataLength( firstFixedOffset_,
                                                          dataLen,
                                                          newLen );
  return newLen;
}

#endif // EXP_ALIGNED_FORMAT_H
