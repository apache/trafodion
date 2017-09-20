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
******************************************************************************
*
* File:         PackedColDesc.cpp
* Description:  All the methods of PackedAPDesc PackedTableDesc
*
* Created:      6/27/97
* Language:     C++
*
*
******************************************************************************
*/

// exclude this whole file from coverage since this code is related to vertical
// partitioning and that feature is not active anymore

// -----------------------------------------------------------------------
// This file contains all the methods for the class PackedAPDesc
// (Packed Access Path Descriptor) and PackedTableDesc. (Packed
// Table Descriptor).  These classes are used during table creation
// (catman) and binding, to determine how the table should be packed.
// Currently, no packing information is stored in the catalog.  Only
// a flag indicating that the table is packed.  Therefore, the binder
// must determine the packing information at run time in the same way
// that was done at create time.
//
#include "PackedColDesc.h"
#include "NumericType.h"

// The layout of a packed table is illustrated in the following diagram:
//
//
//   Original Row Definition
//   -----------------------
//     SYSKEY        Col1          Col2          Col3
//     LARGEINT      INT           CHAR(3)       CHAR(2)
//     Not Nullable  Nullable      Not Nullable  Nullable
//
//   Packed Row Definition for a 'packing factor' of 5
//   -------------------------------------------------
//     SYSKEY        Col1_Packed   Col2_Packed   Col3_Packed
//     LARGEINT      CHAR(25)      CHAR(19)      CHAR(15)
//     Not Nullable  Not Nullable  Not Nullable  Not Nullable
//
//   Each Char field in the packed row contains the following sub-fields.
//   --------------------------------------------------------------------
//     NUMROWS       NULL_BITMAP   DATA
//     INT           CHAR(N)       CHAR(M)
//     Not Nullable  Not Nullable  NotNullable
//
//   These fields are not defined (ie. the system just sees the packed columns
//   as CHAR columns.) and the UnPackCol ItemExpr is used to extract the
//   information.
//
//   NUMROWS - This contains the number of actual values packed into a
//   packed column.  This value should be between 1 and the packing factor.
//   The value of NUMROWS should be the same in each packed column of a
//   packed row.  If this value is less than the packing factor, the values
//   are packed into the lower bytes of the char field.
//
//   NULL_BITMAP - This field contains enough bytes to hold a bit for each
//   Null indicator ('packing factor' bits).
//
//   DATA - This field contains enough bytes to hold the packed values.
//   (datasize * 'packing factor')
//
//   In the above example, with a packing factor of 5, the first column
//   is a nullable int.  The data size for an int is 4 bytes, so the
//   packed column will need:
//
//           4 bytes for the NUMROWS field
//           1 byte for 5 bits of NULL_BITMAP
//          20 bytes for 5 ints ( 5 * 4)
//          --
//          25 total bytes required.
//
// The packing factor is determined based on two limiting factors.
// First the packed row from each access path (vertical partition) must
// fit within MaxPackedAPSize (currently 4000 bytes).  Secondly, the
// sum of the sizes of the packed rows from all access paths for this
// table must fit within MaxPackedTableSize (currently 32000 bytes).
// These two limits are somewhat arbitrary.  The first limits a row
// of an access path to fit within a block.  The second is needed due
// to the way inserts into packed VP Tables are done.  The complete
// packed row is buffered in DP2 and the split into the VP's.  Having
// this buffer bigger than 32K may cause problems.
//

// PackedColDesc::determinePackedColSize() -------------------------------
// Determine the size in bytes of a packed column for the column
// given a packingFactor.
//
Lng32
PackedColDesc::determinePackedColSize(Lng32 packingFactor) const
{
#pragma nowarn(1026)   // warning elimination 
  const Int32 BitsPerByte = 8;
#pragma warn(1026)  // warning elimination 

  const NAType *colType = getType();

  // Variable length columns cannot be packed.
  //
  CMPASSERT(NOT DFS2REC::isAnyVarChar(colType->getFSDatatype()));

  Lng32 nullBitMapSize = (colType->supportsSQLnull()
                         ? ((packingFactor-1)/BitsPerByte)+1
                         : 0);

  // The size of this column in bits.
  //
  Lng32 dataSizeInBits;

  if((colType->getTypeQualifier() == NA_NUMERIC_TYPE) &&
     ((NumericType *)colType)->binaryPrecision() &&
     ((NumericType *)colType)->isUnsigned()) {

    // If the column is a bit precision integer, get the
    // number of bits used by this column.  When we pack we
    // also compress the bit precision integers.
    //
    dataSizeInBits = ((NumericType *)colType)->getPrecision();
      
  } else {
      
    dataSizeInBits = colType->getNominalSize() * BitsPerByte;
  }

  // Total size of the DATA field.
  //
  Lng32 totalDataSize = (((dataSizeInBits * packingFactor)-1)/BitsPerByte)+1;

  // Total size of this packed column given the packing factor.
  // SQL_INT_SIZE is for the NUM_ROWS field.
  //
  return SQL_INT_SIZE + nullBitMapSize + totalDataSize;
}

// PackedColDesc::generatePackingInfo()---------------------------------
// Generate the packing information (dataOffset_, dataSize_, totalSize_, 
// and nullBitmapPresent_) for this column given a packing factor.
// This packing information is:
//
//   long dataOffset_: The offset in bytes to the start of the DATA field.
//   In the above example, the dataOffset_ would be 5 (4 bytes for the NUMROWS
//   field plus 1 byte for the NULL_BITMAP field).
//
//   long dataSize_: The size in bits of a single data item. In the above
//   example, the dataSize_ would be 32 bits (4 bytes).
//
//   long totalSize_: The total size in bytes of the packed column.  This
//   includes the size of the NUM_ROWS field, the size of the NULL_BITMAP
//   field and the size of the DATA field.  In the above example, totalSize_
//   would be 25 (4 + 1 + 20).
//
//   NABoolean nullBitmapPresent_: A boolean flag indicating if there is
//   a NULL_BITMAP field present in this packed column.
//
//
void
PackedColDesc::generatePackingInfo(Lng32 packingFactor)
{

  CMPASSERT(packingFactor > 1);

#pragma nowarn(1026)   // warning elimination 
  const Int32 BitsPerByte = 8;
#pragma warn(1026)  // warning elimination 

  const NAType *colType = getType();
      
        
  Lng32 nullBitMapSize = (colType->supportsSQLnull()
                         ? ((packingFactor-1)/BitsPerByte)+1
                         : 0);

  Lng32 dataSize;
      
  if((colType->getTypeQualifier() == NA_NUMERIC_TYPE) &&
     ((NumericType *)colType)->binaryPrecision() &&
     ((NumericType *)colType)->isUnsigned()) {

    // If the column is a bit precision integer, get the
    // number of bits used by this column.  When we pack we
    // also compress the bit precision integers.
    //
    dataSize = ((NumericType *)colType)->getPrecision();
      
  } else {
    
    dataSize = colType->getNominalSize() * BitsPerByte;
  }

  Lng32 totalDataSize = (((dataSize * packingFactor)-1)/BitsPerByte)+1;

  dataOffset_ = SQL_INT_SIZE + nullBitMapSize;

  dataSize_ = dataSize;

  totalSize_ = SQL_INT_SIZE + nullBitMapSize + totalDataSize;

  nullBitmapPresent_ = colType->supportsSQLnull();
}

// PackedAPDesc::determinePackingFactor() ------------------------
// Determine the maximum packing factor for this AP limited by the
// given maxPackedRecLen.
//
Lng32
PackedAPDesc::determinePackingFactor(Lng32 maxPackedRecLen) const
{

#pragma nowarn(1026)   // warning elimination 
  const Int32 BitsPerByte = 8;
#pragma warn(1026)  // warning elimination 

  PackedColDescList apCols = getAPColumns();

  // Number of columns requiring a null bitmap.
  //
  Lng32 numNullFlags = 0;

  // Size of the key for this AP.
  //
  Lng32 keySizeInBytes = getKeySize();

  // Size of the data for all columns of this AP.
  //
  Lng32 dataSizeInBits = 0;

  // Number of user columns in this AP. (SYSKEY is handled separately.)
  //
  CollIndex numUserColumns = apCols.entries();

  for(CollIndex c = 0; c < numUserColumns; c++) {
    
    const NAType *colType = apCols[c]->getType();

    // Variable length columns can not be packed at this time.
    // If any column of this AP is variable length, then the AP
    // cannot be packed.
    //
    if(colType->isVaryingLen())

      // Cannot pack this column.
      //
      return 0;

    // Does this column require a null bitmap.
    //
    numNullFlags += (colType->supportsSQLnull() ? 1 : 0);

    // If the column is a bit precision integer, get the
    // number of bits used by this column.  When we pack we
    // also compress the bit precision integers.
    //
    if((colType->getTypeQualifier() == NA_NUMERIC_TYPE) &&
       ((NumericType *)colType)->binaryPrecision() &&
       ((NumericType *)colType)->isUnsigned()) {

      dataSizeInBits += ((NumericType *)colType)->getPrecision();
      
    } else {

      // Calculate how many bits required to store this column.
      //
      dataSizeInBits += colType->getNominalSize() * BitsPerByte;
    }
  }

  // For now we only support packing with SYSKEYs of 8 bytes.
  //
  CMPASSERT(keySizeInBytes == 8);

  // Calculate the packing factor.  This does not take into account
  // rounding each packed field to the closest byte, but this should
  // be close enough.
  // The keySizeinBytes is for the SYSKEY (one per AP).
  // The (SQL_INT_SIZE * numUserColumns) is for the NUM_ROWS fields.
  //
#pragma nowarn(1506)   // warning elimination 
  Lng32 packingFactor =
    ((maxPackedRecLen - keySizeInBytes - (SQL_INT_SIZE * numUserColumns))
     * BitsPerByte) /
    (numNullFlags + dataSizeInBits);
#pragma warn(1506)  // warning elimination 

  return packingFactor;
}

// PackedAPDesc::determinePackedAPSize() -------------------------------
// Determine the size in bytes of all the packed columns of this AP
// given a packing factor.
//
Lng32
PackedAPDesc::determinePackedAPSize(Lng32 packingFactor) const
{

  Lng32 packedAPSize = 0;

  PackedColDescList apCols = getAPColumns();
  CollIndex numUserColumns = apCols.entries();

  for(CollIndex c = 0; c < numUserColumns; c++) {

    packedAPSize += apCols[c]->determinePackedColSize(packingFactor);
  }

  return packedAPSize;
}


// PackedAPDesc::generatePackingInfo() ---------------------------------
// Generate the packing information for this AP and all its
// columns given a packing factor.  After this call the packing
// info can be retrieved for a given column of the base table.
//
void
PackedAPDesc::generatePackingInfo(Lng32 packingFactor)
{

  PackedColDescList apCols = getAPColumns();

  if(packingFactor > 1) {
    packingFactor_ = packingFactor;
  
    for (CollIndex i = 0; i < apCols.entries(); i++) {
      apCols[i]->generatePackingInfo(packingFactor);
    }
  } else {
    // Not Packed.
    //
    packingFactor_ = 0;
  }
}


// PackedAPDesc::getPackingInfoForColumn() --------------------------------
// Retrieve the packing information for a column given the columns
// ordinal position in the base table.
//
PackedColDesc *
PackedAPDesc::getPackingInfoForColumn(Lng32 position)
{
  PackedColDesc *packingInfo;

  for(CollIndex i = 0; i < cols_.entries(); i++) {
    packingInfo = cols_[i];
    if(packingInfo->getPosition() == position)
      return packingInfo;
  }
  return (PackedColDesc *)NULL;
}

// PackedAPDesc::addColumn() ------------------------------------------
// Add a column (PackedColDesc) to this PackedAPDesc.
//
void 
PackedAPDesc::addColumn(const NAType *type, Lng32 position, CollHeap *h)
{
  PackedColDesc *packedColDesc = new (h) PackedColDesc(position, type);

  cols_.insert(packedColDesc);
  
}


// PackedTableDesc::PackedTableDesc() ------------------------------------
// Constructor called from the binder.  Constructs and populates a
// PackedTableDesc given a NATable.  The result will indicate if the
// table can be packed and if so, will describe how each of the columns
// of each of the access paths (read vertical partitions) of this table 
// is packed.
//
PackedTableDesc::PackedTableDesc(const NATable *naTable, CollHeap *h)
{
  // This constructor is only called from the binder when naTable is
  // already available.
  //
  callFromCatMan_ = FALSE;

  // For now, only vertically partitioned tables are considered for
  // packing.
  //
  if(naTable->getVerticalPartitionList().entries() > 0) {

    // For each VP of the table,
    //
    //   - determine the size of the key.  For now the keysize must
    //     be 8, the size of a SYSKEY.
    //
    //   - add a PackedAPDesc to this PackedTableDesc which
    //     contains a PackedColDesc for each user column of the VP
    //
    NAFileSetList vpList = naTable->getVerticalPartitionList();
  
    for(CollIndex i = 0; i < vpList.entries(); i++) {
      NAColumnArray vpCols = vpList[i]->getAllColumns();

      Lng32 keySize = 0;

      CollIndex j = 0;
      for(j = 0; j < vpCols.entries(); j++) {
        const NAType *colType = vpCols[j]->getType();

        if(vpCols[j]->isClusteringKey()) {
          keySize += colType->getNominalSize();
        }
      }

      PackedAPDesc *apDesc = new(h) PackedAPDesc(keySize);
      for(j = 0; j < vpCols.entries(); j++) {
        const NAType *colType = vpCols[j]->getType();

        if(!vpCols[j]->isClusteringKey()) {

          apDesc->addColumn(colType, vpCols[j]->getPosition(), h);
        }
      }

      addAP(apDesc);
    }

    // Generate the Packing information.
    //
    generatePackingInfo();
  }
}


// PackedTableDesc::generatePackingInfo() ---------------------------------
// Generate the packing information of each column of each AP of this
// table.  If this table can be packed, the resulting packing information
// will be (for each column):
//
//   long dataOffset_: The offset in bytes to the start of the DATA field.
//   In the above example, the dataOffset_ would be 5 (4 bytes for the NUMROWS
//   field plus 1 byte for the NULL_BITMAP field).
//
//   long dataSize_: The size in bits of a single data item. In the above
//   example, the dataSize_ would be 32 bits (4 bytes).
//
//   long totalSize_: The total size in bytes of the packed column.  This
//   includes the size of the NUM_ROWS field, the size of the NULL_BITMAP
//   field and the size of the DATA field.  In the above example, totalSize_
//   would be 25 (4 + 1 + 20).
//
//   NABoolean nullBitmapPresent_: A boolean flag indicating if there is
//   a NULL_BITMAP field present in this packed column.


void
PackedTableDesc::generatePackingInfo()
{

  // The packing factor is determined based on two limiting factors.
  // First the packed row from each access path (vertical partition)
  // must fit within MaxPackedAPSize (currently 4000 bytes).  Secondly,
  // the sum of the sizes of the packed rows from all access paths for
  // this table must fit within MaxPackedTableSize (currently 32000 bytes).
  // These two limits are somewhat arbitrary.  The first limits a row of an
  // access path to fit within a block.  The second is needed due to the way
  // inserts into packed VP Tables are done.  The complete packed row is
  // buffered in DP2 and then split into the VP's.  Having this buffer 
  // bigger than 32K may cause problems.
  //
  // The maximum size of any packed AP (which could have more than
  // one column.
  //
#pragma nowarn(1026)   // warning elimination 
  const Int32 MaxPackedAPSize = 4000;
#pragma warn(1026)  // warning elimination 

  // The maximum size of all the packed AP's of this table.
  //
#pragma nowarn(1026)   // warning elimination 
  const Int32 MaxPackedTableSize = 24000;
#pragma warn(1026)  // warning elimination 

  // The current minimum packing factor.  This will be the packing
  // factor limited by MaxPackedAPSize for the worst case AP.
  // Initialize to the max possible packing factor.
  //
  Lng32 minPackingFactor = MaxPackedAPSize * 8;
    
  // IF packing is turned off. Check to see if we're creating a new table.
  // Only check when we are creating a new table.  If we are scanning
  // then the table is already packed and we cannot do anything about
  // it.  If a table that could have been packed is create with
  // "PACKING_OFF" set, then the packed flag in the catalog will not
  // be set and we will not reach this code when scanning the table.
  //
  if(getenv("PACKING_OFF") != NULL AND callFromCatMan_) {
    // If we are, don't create a packed table.
    //
    minPackingFactor = 0;

  }  else {
    // For each AP determine its packing factor given MaxPackedAPSize.
    // Keep track of the minimum. This is the limiting AP.
    //
    CollIndex i = 0;
    for (i = 0; i < packedAPDescList_.entries(); i++) {

      Lng32 packingFactor = 
        packedAPDescList_[i]->determinePackingFactor(MaxPackedAPSize);

      minPackingFactor = (packingFactor < minPackingFactor)
        ? packingFactor
        : minPackingFactor;
    }


    if(minPackingFactor > 1) {
      // Determine the size of all the packed AP's given the packing
      // factor limited by MaxPackedAPSize.
      //
      Lng32 packedTableSize = 0;
      for (i = 0; i < packedAPDescList_.entries(); i++) {
        
        packedTableSize +=
          packedAPDescList_[i]->determinePackedAPSize(minPackingFactor);
      }
      
      // If the total size is larger than the max allowed, adjust the
      // packing factor by the same ratio.
      //
      if(packedTableSize > MaxPackedTableSize) {

        float packingFactorAdj = MaxPackedTableSize/(float)packedTableSize;
#pragma nowarn(1506)   // warning elimination 
        minPackingFactor = (Int32)(minPackingFactor * packingFactorAdj) - 1;
#pragma warn(1506)  // warning elimination 
      }
    }
  }

  // Now that a suitable packing factor has been determined
  // generate the packing information for each column of each
  // AP of this table.
  //
  for (CollIndex i = 0; i < packedAPDescList_.entries(); i++) {
    packedAPDescList_[i]->generatePackingInfo(minPackingFactor);
  }

}

// PackedTableDesc::getPackingInfoForColumn()------------------------------
// Retrieve the packing information for a column given its
// ordinal postion in the base table.
//
PackedColDesc *
PackedTableDesc::getPackingInfoForColumn(Lng32 position)
{
  PackedColDesc *packingInfo = (PackedColDesc *)NULL;

  for(CollIndex i = 0; i < packedAPDescList_.entries(); i++) {
    packingInfo = packedAPDescList_[i]->getPackingInfoForColumn(position);
    if(packingInfo)
      return packingInfo;
  }
  return (PackedColDesc *)packingInfo;
}

