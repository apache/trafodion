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
#ifndef EXP_TUPLE_DESC_H
#define EXP_TUPLE_DESC_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         <file>
 * Description:  
 *
 * Created:      5/30/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ExpAlignedFormat.h"

// Forward external references
class Attributes;
class ex_clause;

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ExpTupleDesc;

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for ExTupleDesc
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<ExpTupleDesc> ExpTupleDescPtr;
typedef NAVersionedObjectPtrArrayTempl<ExpTupleDescPtr> ExpTupleDescPtrPtr;
typedef NAVersionedObjectPtrTempl<Attributes> AttributesPtr;
typedef NAVersionedObjectPtrArrayTempl<AttributesPtr> AttributesPtrPtr;

#pragma warning ( disable : 4251 )


#define NEG_BIT_MASK 0x0080    //the first bit in a byte - used for finding
                               //out whether the negative bit in a number
                               //is flipped

// Size of each VOA array entry for SQLMX_FORMAT.
// The SQLMX_ALIGNED_FORMAT has its own constant in ExpAlignedFormat.h
static const UInt32 ExpVoaSize = sizeof(Int32);
static const UInt32 ExpOffsetMax = UINT_MAX;


////////////////////////////////////////////////////////////////////
//
// class ExpTupleDesc
//
////////////////////////////////////////////////////////////////////
class ExpTupleDesc : public NAVersionedObject
{
public:
  //
  // Format of data in tuple:
  //
  // PACKED_FORMAT:     records stored in tables created on SQL/MP.
  //                   Fields stored without any alignment.
  //                   If nullable field, null indicator bytes are
  //                   stored just before the actual data.
  //                   Null indicator length = 2.
  //                   If variable length field, the varchar length
  //                   bytes are stored just before the actual varchar data.
  //                   Varchar indicator length = 2.
  //                   Varchars are NOT stored in exploded format.
  //
  // SQLMX_KEY_FORMAT: 
  //                Fields aligned like PACKED_FORMAT plus (no VOA)
  //                varchars are blank padded to max len. Used to
  //                build key buffers for SQL/MX and SQL/MP tables.
  //
  // SQLARK_EXPLODED_FORMAT: 
  //                   All fields aligned on their appropriate byte boundary.
  //                   Int16 aligned on 2 byte, Int32 on 4 byte and Int64
  //                   on 8-byte boundary.
  //                   If nullable field, null indicator bytes are
  //                   stored just before the actual data.
  //                   Null indicator length = 2.
  //                   If variable length field, the varchar length
  //                   bytes are stored just before the actual varchar data.
  //                   Varchar indicator length = 2 or 4.
  //                   Varchars stored in exploded format(blankpadded
  //                   to max len). 
  // 
  // SQLMX_FORMAT:
  //                   Records stored in tables created within SQL/MX.
  //                   Fields are stored as follows (without any alignment):
  //                   all fixed fields stored first, then all variable
  //                   length fields.
  //                   There is an additional VariableOffsetArray that is
  //                   stored on disk.  This array is the # of variable
  //                   length fields + 1 (for the first fixed field).  Each
  //                   VOA entry is 4 bytes.
  // 
  // SQLMX_ALIGNED_FORMAT:
  //                   Records are padded if needed to 4 byte boundaries.
  //                   Fixed fields are re-arranged and grouped based on their
  //                   alignment boundaries.
  //                   Added fixed length columns follow the original fixed
  //                   fields and are in the logical order they were added.
  //                   Null bitmap at the beginning of record to record
  //                   null column values.
  //                   For more details see exp/ExpAlignedFormat.h
  // 
  enum TupleDataFormat 
  { UNINITIALIZED_FORMAT,
    OBSOLETE_SIMULATOR_FORMAT,
    PACKED_FORMAT, SQLMX_KEY_FORMAT, 
    SQLARK_EXPLODED_FORMAT,
    SQLMX_FORMAT,
    SQLMX_ALIGNED_FORMAT
  };

  enum TupleDescConstants
  {
    // Size of each VOA (variable offset array) entry for SQLMX_FORMAT records
    // data.  This includes the first fixed field offset (i.e., VOA[0])
    EXP_VOA_SIZE               = sizeof(UInt32)
    ,NULL_INDICATOR_LENGTH     = sizeof(Int16)
    ,VC_ACTUAL_LENGTH          = sizeof(UInt16)  // varchar actual length
    ,SQLMX_VC_ACTUAL_LENGTH    = sizeof(UInt32)  // SQLMX_FORMAT varchar data len
    ,KEY_NULL_INDICATOR_LENGTH = sizeof(Int16)   // Key null indicator length
  };

  // Format of tuple descriptor.
  // LONG_FORMAT: Contains information about the tuple and each attribute 
  //              that it contains.
  //              'this' class is followed by an array of size numAttrs_.
  //              The array is followed by numAttrs_ Attributes. Each array
  //              entry points to the corresponding Attribute. The array
  //              is needed because the size of all Attributes may not be
  //              the same. See file exp_attrs.h for details on Attributes
  //              class.
  //
  // SHORT_FORMAT: Only contains 'this' class.
  // 
  enum TupleDescFormat
  { SHORT_FORMAT, LONG_FORMAT
  };

private:
  enum flagsType
  {
    PACKED              = 0x0001,
    VAR_FIELD_PRESENT   = 0x0002,
    ADDED_COLUMN        = 0x0004,
    TDM_FLOAT_PRESENT   = 0x0008,
    KEY_SHIFT           = 0x0010,     // key fields may be shifted
                                      // thus pCode can not be relied on
    ADDED_FIXED_PRESENT = 0x0020
  };
  
  // Optional array of numAttrs_ Attributes pointers.
  // Each entry points to the corresponding Attributes structure.
  AttributesPtrPtr          attrs_;           // 00-07

  // number of attributes/fields/columns/entries in this tuple
  UInt32                    numAttrs_;        // 08-11

  // total length of the generated tuple descriptor. Includes
  // sizeof(this) + size of Attributes array + size of all
  // Attributes.
  UInt32                    tupleDescLength_; // 12-15

  // total max length of data in this tuple
  UInt32                    tupleDataLength_; // 16-19

  Int16 /*TupleDataFormat*/ tupleDataFormat_; // 20-21
  Int16 /*TupleDescFormat*/ tupleDescFormat_; // 22-23
  UInt32                    flags_;           // 24-27
  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char                      fillers_[4];      // 28-31

  Int16 packed()
  {
    return (Int16)(flags_ & PACKED);
  }
    
public:

  ExpTupleDesc(UInt32 num_attrs, 
	       Attributes ** attrs,
	       UInt32 tupleDataLength,
	       TupleDataFormat tdataF,
	       TupleDescFormat tdescF,
	       Space * space);


  ExpTupleDesc(UInt32 num_attrs, 
	       AttributesPtr * attrs,
	       UInt32 tupleDataLength,
	       TupleDataFormat tdataF,
	       TupleDescFormat tdescF);


  ExpTupleDesc();

  ~ExpTupleDesc();


  UInt32 numAttrs(){return numAttrs_;}


  /* Attributes** */ AttributesPtr * attrs(){return attrs_;}


  Attributes *  getAttr(UInt32 attr_num)   {return attrs_[attr_num];}


  virtual Long pack(void *);


  virtual Int32 unpack(void *, void * reallocator);

  ////////////////////////////////////////////////////////
  // Computes offsets of all attributes in the attrs array.
  // start_offset could be passed in optionally indicating
  // offsets computation to start there.
  ////////////////////////////////////////////////////////
  static Int16 computeOffsets(UInt32 num_attr,
			      Attributes ** attrs,
			      TupleDataFormat tf,
			      UInt32 & datalen,
                              UInt32 startOffset = 0,
			      UInt32 * rtnFlags = NULL,
			      TupleDataFormat * outTf = NULL,
                              ExpHdrInfo * hdrInfo = NULL,
                              UInt32 * headerSizePtr = NULL,
                              UInt32 * VarSizePtr = NULL);


  ////////////////////////////////////////////////////////
  // Computes offsets of a single attribute.
  // start_offset could be passed in optionally indicating
  // offsets computation to start there.
  ////////////////////////////////////////////////////////
  static Int16 computeOffsets(Attributes * attr,
			      TupleDataFormat tf,
			      UInt32     & datalen,
			      UInt32       startOffset = 0);
  

  ////////////////////////////////////////////////////////
  // Class method that centralizes SQLARK_EXPLODED_FORMAT tuple
  // format offset calculations.  Returns aligned column offset.
  ////////////////////////////////////////////////////////
  static UInt32 sqlarkExplodedOffsets(
    UInt32 offset,
    UInt32 length,
    Int16 dataType,
    NABoolean isNullable,
    UInt32 * nullIndicatorOffset = NULL,
    UInt32 * vcIndicatorOffset = NULL,
    UInt32 vcIndicatorSize = ExpTupleDesc::VC_ACTUAL_LENGTH);

  Int16 operator==(ExpTupleDesc * td);

  // assigns the atp and atp_index value to all attributes
  void assignAtpAndIndex(Int16 atp, Int16 atp_index);
  

  UInt32 tupleDescLength()         {return tupleDescLength_;}
    

  UInt32 tupleDataLength()         {return tupleDataLength_;}


  void setAddedField()
  { flags_ |= ADDED_COLUMN; }


  Int16 addedFieldPresent() 
  { return (Int16)(flags_ & ADDED_COLUMN); }

  void setAddedFixed()
  { flags_ |= ADDED_FIXED_PRESENT; }


  Int16 addedFixedPresent() 
  { return (Int16)(flags_ & ADDED_FIXED_PRESENT); }

  NABoolean variableFieldPresent()
  {  return( (flags_ & (UInt32)VAR_FIELD_PRESENT) != 0 ); }

  NABoolean tandemFloatPresent() 
  {  return( (flags_ & (UInt32)TDM_FLOAT_PRESENT) != 0 ); }

  // Check if key fields may be shifted due to an added column or not.
  NABoolean isKeyShifted()
  {  return( (flags_ & KEY_SHIFT) > 0 ); }

  void setFlags(UInt32 newFlags)   { flags_ = newFlags; }

  Int16 tupleFormat() { return tupleDataFormat_; };

  const char *tupleFormatStr() {
    switch(tupleDataFormat_) {
    case UNINITIALIZED_FORMAT:
      return "Uninitialized";
    case OBSOLETE_SIMULATOR_FORMAT:
      return "Simulator";
    case PACKED_FORMAT:
      return "Packed";
    case SQLMX_KEY_FORMAT:
      return "Key";
    case SQLARK_EXPLODED_FORMAT:
      return "Exploded";
    case SQLMX_FORMAT:
      return "SqlMX";
    case SQLMX_ALIGNED_FORMAT:
      return "Aligned";
    default:
      return "Unknown";
    }
  }

  NABoolean isSQLMXTable()
  {  return tupleDataFormat_ == (Int16) ExpTupleDesc::SQLMX_FORMAT; }

  NABoolean isSQLMXAlignedTable()
  {  return tupleDataFormat_ == (Int16)ExpTupleDesc::SQLMX_ALIGNED_FORMAT; }

  NABoolean isSQLPackedTable()
  {  return tupleDataFormat_ == (Int16) ExpTupleDesc::PACKED_FORMAT; }

  // computeFirstFixedOffset
  //   Called at compile time to compute the offset to the first fixed
  //   field.
  //   For SQLMX_FORMAT set it to the minimum that it can be - right
  //   after VOA[0].
  //   For SQLMX_ALIGNED_FORMAT see notes in ExpAlignedFormat.h
  static UInt32 computeFirstFixedOffset(Int32           ffAlignSize,
                                        UInt32          startOffset,
                                        UInt32          voaIdxOffset,
                                        TupleDataFormat tdf,
                                        UInt32          nullableFieldCount,
                                        ExpHdrInfo    * hdrInfo,
                                        UInt32        & headerSize,
                                        UInt32        & bitmapOffset)
                                        
  {
    if ( tdf == ExpTupleDesc::SQLMX_ALIGNED_FORMAT )
    {
      return ExpAlignedFormat::computeFirstFixedOffset( ffAlignSize,
                                                        startOffset,
                                                        voaIdxOffset,
                                                        nullableFieldCount,
                                                        hdrInfo,
                                                        headerSize,
                                                        bitmapOffset );
    }
    else
    {
      headerSize = ExpVoaSize;             // First fixed field offset

      if (hdrInfo != NULL)
      {
        hdrInfo->setValues(headerSize);    // first fixed field offset area
      }

      return voaIdxOffset + headerSize;
    }
  }

  static UInt32 read4( char *tempPtr )
  {
    // use 'unsigned char *' so that the value is not sign extended.
    UChar *dataPtr = (UChar *)tempPtr;

#ifdef NA_BIG_ENDIAN
    return (dataPtr[0] << 24) |
           (dataPtr[1] << 16) |
           (dataPtr[2] << 8) |
           (dataPtr[3]);
#else
    return (dataPtr[3] << 24) |
           (dataPtr[2] << 16) |
           (dataPtr[1] << 8) |
           (dataPtr[0]);
#endif
  }

  static UInt32 read2( char *tempPtr )
  {
    // use 'unsigned char *' so that the value is not sign extended.
    UChar *dataPtr = (UChar *)tempPtr;

#ifdef NA_BIG_ENDIAN
    return (unsigned short )((dataPtr[0] << 8) | dataPtr[1]);
#else
    return (unsigned short )((dataPtr[1] << 8) | dataPtr[0]);
#endif
  }

  static void write4( char *dataPtr,
                      UInt32 value )
  {
#ifdef NA_BIG_ENDIAN
    dataPtr[0] = (char)(value >> 24);
    dataPtr[1] = (char)(value >> 16);
    dataPtr[2] = (char)(value >> 8);
    dataPtr[3] = (char)value;
#else
    dataPtr[3] = (char)(value >> 24);
    dataPtr[2] = (char)(value >> 16);
    dataPtr[1] = (char)(value >> 8);
    dataPtr[0] = (char)value;
#endif
  }

  static void write2( char *dataPtr,
                      unsigned short value )
  {
#ifdef NA_BIG_ENDIAN
    dataPtr[0] = (char)(value >> 8);
    dataPtr[1] = (char)value;
#else
    dataPtr[1] = (char)(value >> 8);
    dataPtr[0] = (char)value;
#endif
  }

  // If filler bytes are needed to extend a record to a 4-byte multiple
  // then the bytes are stored specific to each data format.
  UInt32 getNumberFillerBytes( char          * dataPtr )
  {
    if ( isSQLMXAlignedTable() )
      return ((ExpAlignedFormat *)dataPtr)->getNumberFillerBytes();
    else
      return 0;  // no filler bytes
  }

  // If there were filler bytes added to the row length, subtract them out
  // and return the true length of the data.
  UInt32 getActualDataLength( char           * dataPtr,
                              UInt32           dataLen )
  {
    if ( isSQLMXAlignedTable() )
      return( dataLen - ((ExpAlignedFormat *)dataPtr)->getNumberFillerBytes());
    else
      return dataLen;  // no filler bytes
  }

  //
  // Get the size of each header and voa entry dependent on the table row
  // format.
  static UInt32 getVoaSize( TupleDataFormat tdf )
  {
    return (( tdf == SQLMX_ALIGNED_FORMAT )
            ? ExpAlignedFormat::OFFSET_SIZE : ExpVoaSize ); 
  }

  UInt32 getVoaSize()
  {
    return getVoaSize( (TupleDataFormat)tupleDataFormat_ );
  }

  UInt32 adjustDataLength( char              * dataPtr,
                           UInt32              currLength )
  {
    if ( isSQLMXAlignedTable() )
      return ExpAlignedFormat::adjustDataLength( dataPtr,
                                                 currLength,
                                                 ExpAlignedFormat::ALIGNMENT );
    else
      // no resizing needed since fields are packed
      return currLength;
  }

  // Determine which disk format we are working with and return the first
  // field offset based on the disk format.
  static UInt32 getFirstFixedOffset( char           * dataPtr,
                                     TupleDataFormat  tdf )
  {
    if ( tdf == ExpTupleDesc::SQLMX_ALIGNED_FORMAT )
      return ( ExpAlignedFormat::getFirstFixedOffset( dataPtr ) );
    else
      return ( *(UInt32 *)dataPtr );
  }

  // Set the first fixed field's offset.
  static void setFirstFixedOffset( char           * dataPtr,
                                   UInt32           firstFixedOffset,
                                   TupleDataFormat  tdf )
  {
    if ( tdf == ExpTupleDesc::SQLMX_ALIGNED_FORMAT )
      ExpAlignedFormat::setFirstFixedOffset( dataPtr, firstFixedOffset );
    else
      *((UInt32 *)dataPtr) = firstFixedOffset;
  }

  // Determine which disk format we are working with and return the first
  // field offset based on the disk format.
  UInt32 getFirstVariableOffset( char        * dataPtr,
                                 UInt32        firstFixedOffset )
  {
    UInt32 offset = 0;
    if ( variableFieldPresent() )
    {
      if ( isSQLMXAlignedTable() )
      {
        offset = ExpAlignedFormat::getFirstVariableOffset( dataPtr,
                                                           firstFixedOffset
                                                           );
      }
      else
      {
        // One would think this could not happen if variableFieldPresent() were
        // true, but that is not the case when there are added columns in the
        // table and the data row is a "short" row where no variable length
        // columns exist.
        if ( firstFixedOffset == ExpVoaSize )  // fixed fields only
          offset = 0;
        else   // variable fields present
          offset = *(UInt32 *)(dataPtr + ExpVoaSize);
      }
    }
    return offset;
  }

  static char getNullChar()
  {  return (char)'\377'; }

  static char getZeroChar()
  {  return (char)'\0'; }

  // Determine the data format we are working with and set the appropriate
  // null indicator correctly.
  // This assumes we are pointing to the null indicator bytes for MX format
  // or the bitmap itself for MX Aligned format.
  static void setNullValue( char             * dataPtr,
                            Int16              nullBitIndex,
                            TupleDataFormat    tdf )
  {
    if ( tdf == ExpTupleDesc::SQLMX_ALIGNED_FORMAT )
    {
      ExpAlignedFormat::setNullBit( dataPtr, nullBitIndex );
    }
    else
    {
      dataPtr[0] = '\377';
      dataPtr[1] = '\377';
    }
  }

  //
  // Set the null indicator.
  static void setNullValue( char             * dataPtr,
                            Int16              nullBitIndex )
  {
    if ( nullBitIndex >= 0 )
    {
      ExpAlignedFormat::setNullBit( dataPtr, nullBitIndex );
    }
    else
    {
      dataPtr[0] = '\377';
      dataPtr[1] = '\377';
    }
  }

  // Determine the data format we are working with and set the appropriate
  // null indicator correctly.
  static void clearNullValue( char           * dataPtr,
                              Int16            nullBitIndex,
                              TupleDataFormat  tdf )
  {
    if ( tdf == ExpTupleDesc::SQLMX_ALIGNED_FORMAT )
    {
      ExpAlignedFormat::clearNullBit( dataPtr, nullBitIndex );
    }
    else
    {
      dataPtr[0] = '\0';
      dataPtr[1] = '\0';
    }
  }

  // Determine the data format we are working with and set the appropriate
  // null indicator correctly.
  static void clearNullValue( char           * dataPtr,
                              Int16            nullBitIndex )
  {
    if ( nullBitIndex >= 0 )
    {
      ExpAlignedFormat::clearNullBit( dataPtr, nullBitIndex );
    }
    else
    {
      dataPtr[0] = '\0';
      dataPtr[1] = '\0';
    }
  }

  // Based on the data format, determine if the value is NULL or not.
  static NABoolean isNullValue( char          * nullDataPtr,
                                Int16           nullBitIdx,
                                TupleDataFormat tdf )
  {
    if ( tdf == ExpTupleDesc::SQLMX_ALIGNED_FORMAT )
    {
      return ( ExpAlignedFormat::isNullValue( nullDataPtr, nullBitIdx ) );
    }
    else
    {
      // NSK and NT have different byte orderings.
      return ((nullDataPtr)[1] & NEG_BIT_MASK); // sign bit is flipped
    }
  }

  // Null bit index is set to [0,n] for aligned format,
  // and all other formats it is initialized to -1.
  static NABoolean isNullValue( char          * nullDataPtr,
                                Int16           nullBitIdx )
  {
    if ( nullBitIdx >= 0 )
    {
      return ( ExpAlignedFormat::isNullValue( nullDataPtr, nullBitIdx ) );
    }
    else
    {
      return ((nullDataPtr)[0] & NEG_BIT_MASK);
    }
  }

  //
  // Method to determine if a variable field offset is valid or not.
  // If not valid then the offset must be read from the VOA entry.
  static
  NABoolean isValidVariableOffset(UInt32      offset)
  {  return (offset != ExpOffsetMax); }

  static void setVoaValue( char              * dataPtr,
                           UInt32              voaEntryOffset,
                           UInt32              voaEntryValue,
                           TupleDataFormat     tdf)
  {
    if (voaEntryOffset != ExpOffsetMax)
    {
      if ( tdf == SQLMX_ALIGNED_FORMAT )
      {
        ((ExpAlignedFormat *)dataPtr)->setVoaOffset(voaEntryOffset,
                                                    voaEntryValue);
      }
      else if ( tdf == SQLMX_FORMAT )
      {
        str_cpy_all(dataPtr + voaEntryOffset,(char *)&voaEntryValue,ExpVoaSize);
      }
      else
      {
        // should never be here
        assert( 0 );
      }
    }
  }

  // Set a variable length field's offset value in it's VOA entry.
  // The size of the voa entry is the same as the size of the variable
  // length fields "length" bytes.
  static void setVoaValue( char              * dataPtr,
                           UInt32              voaEntryOffset,
                           UInt32              voaEntryValue,
                           Int16               vcIndLen)
  {
    if (voaEntryOffset != ExpOffsetMax)
    {
      if ( vcIndLen == sizeof(Int16) )
      {
        ((ExpAlignedFormat *)dataPtr)->setVoaOffset(voaEntryOffset,
                                                    voaEntryValue);
      }
      else if ( vcIndLen == sizeof(Int32) )
      {
        str_cpy_all(dataPtr + voaEntryOffset,(char *)&voaEntryValue,ExpVoaSize);
      }
      else
      {
        // should never be here
        assert( 0 );
      }
    }
  }

  void setVoaValue( char                     * dataPtr,
                    UInt32                     voaEntryOffset,
                    UInt32                     voaEntryValue )
  {
    ExpTupleDesc::setVoaValue( dataPtr,
                               voaEntryOffset,
                               voaEntryValue,
                               getTupleDataFormat() );
  }

  // Get the offset value out of the VOA entry based on tuple data format.
  static UInt32 getVoaOffset( char           * dataPtr,
                              UInt32           voaEntryOffset,
                              TupleDataFormat  tdf )
  {
    UInt32 offset = 0;
    if (voaEntryOffset != ExpOffsetMax)
    {
      if ( SQLMX_ALIGNED_FORMAT == tdf )
      {
        offset = ((ExpAlignedFormat *)dataPtr)->getVoaEntry( voaEntryOffset );
      }
      else
      {
        offset = *(UInt32 *)(dataPtr + voaEntryOffset);
      }
    }
    return offset;
  }

  static
  UInt32 getVoaOffset( char   *dataPtr,
                       UInt32  voaEntryOffset,
                       Int32   voaSize )
  {
    UInt32 offset = 0;
    
    if (voaSize == ExpVoaSize)  // non-aligned format
    {
      offset = *(UInt32 *)(dataPtr + voaEntryOffset);
    }
    else
    {
      offset = (UInt32)((ExpAlignedFormat *)dataPtr)->getVoaEntry( voaEntryOffset );
    }

    return offset;
  }

  UInt32 getVoaOffset( char                  * dataPtr,
                      UInt32                   voaEntryOffset )
  {
    return ExpTupleDesc::getVoaOffset( dataPtr,
                                       voaEntryOffset,
                                       getTupleDataFormat() );
  }

  // Return the offset to the start of the variable field, 
  // i.e., at the start of the vcIndLen.
  // If the offset is -1, it must read the offset from the voa entry
  static
  UInt32 getVarOffset( char             * dataPtr,
                       UInt32             offset,
                       UInt32             voaEntryOffset,
                       UInt32             vcIndLen,
                       UInt32             nullIndLen
                     )
  {
    UInt32 varLenIndOffset;

    if (offset == ExpOffsetMax)
    {
      // indirect varchar
      varLenIndOffset = getVoaOffset( dataPtr, voaEntryOffset, vcIndLen )
                        + nullIndLen;
    }
    else
    {
      // offset is valid, go back vcIndLen bytes
      varLenIndOffset = offset - vcIndLen;
    }
    
    return varLenIndOffset;
  }

  // Compute the target varchar offset. 
  // For tgt in disk format, compute offset based on loop variable.
  // Update the VOA entry. 
  static
  Int32 getTgtVarOffset(  char   *dataPtr,
                          Int32  offset,
                          Int32  voaEntryOffset,
                          UInt32 vcIndLen,
                          UInt32 nullIndLen,
                          Int32  &varOffset,
                          UInt32 len)
  {
    char *tgt = dataPtr;
    Int32 tgtOffset = offset;
  
    if (voaEntryOffset > 0) // disk format
    {
      if ( tgtOffset >= 0 ) // If tgtOffset is valid ... use it.
      {
        tgtOffset -= vcIndLen;
        if ( voaEntryOffset >= 0 ) // if first varchar, setup loop variable?
          varOffset = tgtOffset - nullIndLen;  
      }
      else // indirect varchar, get offset from loop variable.
        tgtOffset = varOffset + nullIndLen;  // skip over the null indicator len

      setVoaValue(tgt, voaEntryOffset, varOffset, (UInt32)vcIndLen); 

      varOffset += len + nullIndLen + vcIndLen; 
    }
    else
    {
      tgtOffset -= vcIndLen;
    }

    return tgtOffset;
  }

  static 
  UInt32 getVarLength( char           * dataPtr,
                       Int32            vcIndLen )
  {
    UInt32 varLen = 0;

    if (vcIndLen == sizeof(Int16))
      varLen = read2(dataPtr);
    else
      varLen = read4(dataPtr);
    
    return varLen;
  }

  static void setVarLength( char             * dataPtr,
                            UInt32             varLength,
                            Int32              vcIndLen )
  {
    if (vcIndLen == sizeof(Int16))
      write2(dataPtr, (UInt16)varLength);
    else
      write4(dataPtr, varLength);
  }

  // Return the number of bytes used to store the length for variable fields
  static Int32 getVarLenBytes( TupleDataFormat tdf )
  {
    return( tdf == SQLMX_FORMAT
            ? ExpTupleDesc::SQLMX_VC_ACTUAL_LENGTH
            : ( tdf == SQLMX_ALIGNED_FORMAT
                ? (Int32) ExpAlignedFormat::VARIABLE_LEN_SIZE
                : (Int32) ExpTupleDesc::VC_ACTUAL_LENGTH ) );
  }

  // Return the number of bytes used to store the null indicator based on
  // the tuple data format.
  static Int32 getNullIndBytes(TupleDataFormat tdf)
  {
    return( tdf == SQLMX_ALIGNED_FORMAT
            ? (Int32) ExpAlignedFormat::NULL_IND_SIZE
            : (Int32) ExpTupleDesc::NULL_INDICATOR_LENGTH );
  }

  static
  UInt32 getVarLength( char * dataPtr, TupleDataFormat tdf )
  {
    if (tdf == ExpTupleDesc::SQLMX_ALIGNED_FORMAT ||
        tdf == ExpTupleDesc::SQLARK_EXPLODED_FORMAT)
      return getVarLength(dataPtr, sizeof(Int16));
    else
      return getVarLength(dataPtr, sizeof(Int32));
  }
                   
  TupleDataFormat getTupleDataFormat()
  {  return (TupleDataFormat)tupleDataFormat_; }

  static NABoolean isDiskFormat(TupleDataFormat tdf )
  {
    return ( ((tdf == SQLMX_ALIGNED_FORMAT) || (tdf == SQLMX_FORMAT))
             ? TRUE : FALSE );
  }

  static NABoolean isAlignedFormat(TupleDataFormat tdf)
  {  return ( SQLMX_ALIGNED_FORMAT == tdf ); }

  static NABoolean isHeaderClauseNeeded( TupleDataFormat tdf )
  {
    return ( tdf == SQLMX_ALIGNED_FORMAT );
  }


  void display(const char* title = NULL);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual Int16 getClassSize() { return (Int16)sizeof(*this); }
  // ---------------------------------------------------------------------

};

#pragma warning ( default : 4251 )

#endif
