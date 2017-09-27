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
#ifndef PACKEDCOLDESC_H
#define PACKEDCOLDESC_H
/* -*-C++-*-
******************************************************************************
*
* File:         PackedColDesc.h
* Description:  Class definitions for the classes PackedColDesc,
*               PackedColDescList, PackedAPDesc, PackedAPDescList,
*               and PackedTableDesc.
* Created:      6/17/97
* Language:     C++
*
*
*
*
******************************************************************************
*/
// exclude this whole file from coverage since this code is related to vertical
// partitioning and that feature is not active anymore
#include "Collections.h"
#include "NAType.h"
#include "NATable.h"

// The classes defined in this file are used to describe the way in which
// a table is packed.   The diagram below illustrates how the strutures are
// related.  The PackedTableDesc constructor can be used to construct these
// structures given an NATable.
//
// |------------------|
// |PackedTableDesc   |
// |------------------|
// |callFromCatMan_   |
// |packedAPDescList_ |->-------------------------------------- ...
// |------------------|   |                  |              |       
//                     |---------------|  |------------| |------------|
//                     |PackedAPDesc   |  |PackedAPDesc| |PackedAPDesc|
//                     |---------------|  |------------| |------------|
//                     |keySize_       |
//                     |packingFactor_ |
//                     |cols_          |->-----------------------------....
//                     |---------------|   |                     |
//                                      |-------------------| |-------------|
//                                      |PackedColDesc      | |PackedColDesc|
//                                      |-------------------| |-------------|
//                                      |position_          |
//                                      |dataOffset_        |
//                                      |dataSize_          |
//                                      |totalSize_         |
//                                      |nullBitmapPresent_ |
//                                      |type_              |
//                                      |-------------------|
//

// Class PackedColDesc ---------------------------------------------------
// This class object is used to describe how a column of a packed table
// is layed out.  The layout of a packed table is illustrated in the 
// following diagram:
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
// to the way inserts into packed VP Tables is done.  The complete packed
// row is buffered in DP2 and the split into the VP's.  Having this buffer
// bigger than 32K may cause problems.
//
// The data members of this class describe how a column of a packed
// table is layed out.  The data members are:
//
//   long position_: The ordinal position of the column in the table.
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
//   NAType *type_: A pointer to the type of the packed data.
//
class PackedColDesc : public NABasicObject
{
public:
  
  // The constructor.  Intialize the PackedColDesc to be empty with the
  // exception of the position and the type of the column to be packed.
  // It is up to the user to ensure that the packing information is
  // properly set.
  //
// warning elimination (removed "inline") 
  PackedColDesc(Lng32 position,
                       const NAType *type)
    : position_(position),
      dataOffset_(0),
      dataSize_(0),
      totalSize_(0),
      nullBitmapPresent_(FALSE),
      type_(type)
  {
  };

  // Generate the packing info for this column given a packing factor.
  //
  void generatePackingInfo(Lng32 packingFactor);

  // Determine the size of this packed col given a packing factor.
  //
  Lng32 determinePackedColSize(Lng32 packingFactor) const;

  // Accessor methods of data memners
  //
  Lng32 getPosition() const { return position_;};
  Lng32 getDataOffset() const { return dataOffset_;};
  Lng32 getDataSize() const { return dataSize_;};
  Lng32 getTotalSize() const { return totalSize_;};
  NABoolean isNullBitmapPresent() const { return nullBitmapPresent_;};
  const NAType *getType() const { return type_; };

private:

  // The ordinal position of the column in the table.
  //
  Lng32 position_;

  // The offset in bytes to the start of the DATA field.
  //
  Lng32 dataOffset_;

  // The size in bits of a single data item.
  //
  Lng32 dataSize_;

  // The total size in bytes of the packed column.  This includes the
  // size of the NUM_ROWS field, the size of the NULL_BITMAP field and
  // the size of the DATA field.
  //
  Lng32 totalSize_;

  // A boolean flag indicating if there is a NULL_BITMAP field present
  // in this packed column.
  //
  NABoolean nullBitmapPresent_;

  // A pointer to the type of the packed data.
  //
  const NAType *type_;
};
  
// A List of PackedColDesc objects.
//
class PackedColDescList : public LIST(PackedColDesc *)
{
public:

  PackedColDescList(Lng32 numElements = 0)
    : LIST(PackedColDesc *)(CmpCommon::statementHeap(), numElements) {};
    
};


// class PackedAPDesc ------------------------------------------------
// This class object is used to describe how a particular access path
// (read vertical partition) is 'packed'. It is typically accessed
// through the PackedTableDesc class. The data members are:
//
//   long keySize_:  The size of the key for this access path.  The 
//   key size should be the same for each access path of a table.
//   Currently, the only supported key size for packed tables is 8.
//
//   long packingFactor_: The packing factor for this access path.
//   Currently, the packing factor must be the same for all access
//   paths of a table.  The packing factor is determined based on
//   two limiting factors.  First the packed row from each access path
//   (vertical partition) must fit within MaxPackedAPSize (currently
//   4000 bytes).  Secondly, the sum of the sizes of the packed rows
//   from all access paths for this table must fit within MaxPackedTableSize
//   (currently 32000 bytes).  These two limits are somewhat arbitrary.  The
//   first limits a row of an access path to fit within a block.  The second
//   is needed due to the way inserts into packed VP Tables is done.  The
//   complete packed row is buffered in DP2 and the split into the VP's. 
//   Having this buffer bigger than 32K may cause problems.
//
//   PackedColDescList cols_: A list of PackedColDesc's describing how each
//   of the columns of this AP is packed.
//
class PackedAPDesc : public NABasicObject
{
public:

  // Constructor - create an empty PackedAPDesc.
  //

  PackedAPDesc(Lng32 keySize, Lng32 packingFactor = 0)
        : keySize_(keySize),
          packingScheme_(0),
          packingFactor_(packingFactor)
  {};



  // The size of the key for this AP.
  //
  Lng32 getKeySize() const { return keySize_; };

  // Add a PackedColDesc for a column.
  //
  void addColumn(const NAType *type, Lng32 position, CollHeap *h);

  // return a reference to the list of packed columns for this AP.
  //
  PackedColDescList &getAPColumns() { return cols_; };

  // return a read only reference to the list of packed columns for this AP.
  //
  const PackedColDescList &getAPColumns() const { return cols_; };

  // Determine the max. packing factor for this AP, given a max rec. size.
  // The actual packing factor may be limited by another AP for this table.
  //
  Lng32 determinePackingFactor(Lng32 maxPackedRecLen) const;

  // Determine the size of this packed AP given a packing factor.
  //
  Lng32 determinePackedAPSize(Lng32 packingFactor) const;

  // Generate the packing information given a packing factor for all
  // columns of this AP.
  //
  void generatePackingInfo(Lng32 packingFactor);

  // Get the packing factor for this AP.
  //
  Lng32 getPackingFactor() const { return packingFactor_; };

  // Get the packing scheme for this AP.
  //
  Lng32 getPackingScheme() const { return packingScheme_; };

  // Get the packing information for the column with the given
  // value of position.  Position is the position of this column
  // in the base table.
  //
  PackedColDesc *getPackingInfoForColumn(Lng32 position);

private:

  // The size of the key for this access path.  The key size should
  // be the same for each access path of a table. Currently, the only
  // supported key size for packed tables is 8.
  //
  Lng32 keySize_;

  // The packing factor for this access path.  Currently, the packing 
  // factor must be the same for all access paths of a table.  The
  // packing factor is determined based on two limiting factors.
  // First the packed row from each access path (vertical partition)
  // must fit within MaxPackedAPSize (currently 4000 bytes).  Secondly,
  // the sum of the sizes of the packed rows from all access paths for
  // this table must fit within MaxPackedTableSize (currently 32000 bytes).
  // These two limits are somewhat arbitrary.  The first limits a row of an
  // access path to fit within a block.  The second is needed due to the way
  // inserts into packed VP Tables is done.  The complete packed row is
  // buffered in DP2 and the split into the VP's.  Having this buffer 
  // bigger than 32K may cause problems.
  //
  Lng32 packingFactor_;
  Lng32 packingScheme_;


  // A list of PackedColDesc's describing how each of the columns of
  // this AP is packed.
  //
  PackedColDescList cols_;
};


// A list of PackedApDesc objects.
//
class PackedAPDescList : public LIST(PackedAPDesc *)
{
public:
  PackedAPDescList(CollHeap *h = 0)
    : LIST(PackedAPDesc *)(h) {};
  PackedAPDescList(Lng32 numElements, CollHeap *h = 0)
    : LIST(PackedAPDesc *)(h, numElements) {};
};

// class PackedTableDesc ------------------------------------------------
// This class is used to describe how a table is packed. A PackedTableDesc
// is constructed in two places. In catman, if the table is vertically
// partitioned, a PackedTableDesc is constructed to determine how (and if)
// this table is to be packed. In the binder, a PackedTableDesc is
// constructed if the table being accessed is packed. The PackedTableDesc
// is in this case constructed given a NATable.  In both cases the packing
// information is determined implicitly by the generatePackingInfo()
// methods of PackedTableDesc, PackedAPDesc, and PackedColDesc. The packing
// information is NOT stored in the catalog. So if the algorithm used to
// determine the packing information changes, the packed tables store on
// disk may have to be repopulated or some sort of versioning control needs
// to be put in place.  The data members of the class are:
//
//   PackedAPDescList packedAPDescList_: A list of PackedAPDesc objects
//   describing how the access paths (read vertical partitions) of this
//   table are packed.
//
//   NABoolean callFromCatMan_: A flag indicating that this instance of
//   a PackedTableDesc was constructed from within catman.  This flag is
//   used to be able to turn packing off through the use of an environmental
//   variable ("PACKING_OFF").
//
//
class PackedTableDesc : public NABasicObject
{
public:

  // The default constructor.  This version of the constructor
  // is called only from catman. (see description of callFromCatMan_ above)
  //
  PackedTableDesc()
    { callFromCatMan_ = TRUE; };

  // Constructor called from the binder.  Constructs and populates a
  // PackedTableDesc given a NATable.  The result will indicate if the
  // table can be packed and if so, will describe how each of the columns
  // of each of the access paths (read vertical partitions) of this table 
  // is packed.
  //
  PackedTableDesc(const NATable *naTable, CollHeap *h);

  // Used to add an AP to this PackedTableDesc.
  //
  void addAP(PackedAPDesc *apDesc) 
    {
      packedAPDescList_.insert(apDesc);
    };

  // Generate the packing information for this table.
  //
  void generatePackingInfo();

  // Get the packing information for the column with the given
  // value of position.  Position is the position of this column
  // in the base table.
  //
  PackedColDesc *getPackingInfoForColumn(Lng32 position);

  // Get the packing factor for this table.  For now the packing factor
  // must be the same for all AP's of a table, so return the packing
  // factor of the first AP.
  //
  Lng32 getPackingFactor() { return packedAPDescList_[0]->getPackingFactor();};

  // return a reference to the list of PackedAPDesc objects for this table.
  //
  const PackedAPDescList &getAPDescList() const { return packedAPDescList_; };

private:
  // A list of PackedAPDesc objects describing how the access paths
  // (read vertical partitions) of this table are packed.
  //
  PackedAPDescList packedAPDescList_;

  // A flag indicating that this instance of a PackedTableDesc was
  // constructed from within catman.  This flag is used to be able to turn
  // packing off through the use of an environmental variable ("PACKING_OFF").
  //
  NABoolean callFromCatMan_;
};

#endif
