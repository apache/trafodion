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
#ifndef PARDDLFILEATTRSCREATEINDEX_H
#define PARDDLFILEATTRSCREATEINDEX_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ParDDLFileAttrsCreateIndex.h
 * Description:  class to contain all legal file attributes associating
 *               with the DDL statement Create Index -- The parser
 *               constructs a parse node for each file attribute specified
 *               in a DDL statement.  Collecting all file attributes
 *               to a single object (node) helps the user to access
 *               the file attribute information easier.  The user does
 *               not need to traverse the parse sub-tree to look for
 *               each file attribute parse node associating with the
 *               DDL statement.  Default values will be assigned to file
 *               attributes that are not specified in the DDL statement.
 *
 *               Class ParDDLFileAttrsCreateIndex does not represent a
 *               parse node.  The class StmtDDLCreateIndex representing
 *               a Create Index parse node contains (has a has-a
 *               relationship with) the class ParDDLFileAttrsCreateIndex.
 *
 *
 * Created:      9/29/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLFileAttrMaxSize.h"
#include "ParDDLFileAttrs.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ParDDLFileAttrsCreateIndex;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// class to contain all file attributes associating with a DDL statement
// -----------------------------------------------------------------------
class ParDDLFileAttrsCreateIndex : public ParDDLFileAttrs
{

public:

  // default constructor
  ParDDLFileAttrsCreateIndex(
       ParDDLFileAttrs::fileAttrsNodeTypeEnum fileAttrsNodeType
       = ParDDLFileAttrs::FILE_ATTRS_CREATE_INDEX);

  // copy constructor
  ParDDLFileAttrsCreateIndex(
       const ParDDLFileAttrsCreateIndex & createIndexFileAttributes);

  // virtual destructor
  virtual ~ParDDLFileAttrsCreateIndex();

  // assignment operator
  ParDDLFileAttrsCreateIndex & operator = (
       const ParDDLFileAttrsCreateIndex &rhs);

  //
  // accessors
  //

  inline ComSInt16 getExtentsToAllocate() const;
  
  // Returns the number of extents specified in the ALLOCATE
  // phrase; returns the default values when the ALLOCATE
  // phrase does not appear.

  inline NABoolean getIsAuditCompress() const;

        // Returns TRUE (the default value) when the [No] AuditCompress
        // phrase is not specified.

  inline ULng32 getBlockSize() const;

        // Returns 4096 (the default value) when the BlockSize
        // phrase is not specified.

  inline NABoolean getIsBuffered() const;

        // For Create Index statements, the parser can not determine
        // the default value when the [No] Buffered phrase does not
        // appear; therefore the return value does not have any meaning
        // when the Buffered phrase is not specified.
        //
        // For Create Table statements, the parser can determine the
        // default value during the construction of the Create Table
        // parse node.

  inline NABoolean getIsClearOnPurge() const;

        // For Create Index statements, the parser cannot determine
        // the default value when the [No] ClearOnPurge phrase does not
        // appear; therefore the return value does not have any meaning
        // when the ClearOnPurge phrase is not specified.
        //
        // For Create Table statements, the default value is FALSE (no
        // erasure).

  inline ComCompressionType getCompressionType() const;

        // Returns compression type (SOFTWARE, HARDWARE, etc) when the
        // COMRPESSION TYPE phrase is specified

  inline NABoolean getIsDCompress() const;

        // For Create Index statements, the parser cannot determine
        // the default value when the [No] DCompress phrase does not 
        // appear; therefore the return value does not have any meaning
        // when the [No] DCompress phrase is not specified.
        //
        // For Create Table statements, the default value is FALSE (no
        // compression).

  inline NABoolean getIsICompress() const;

        // For Create Index statements, the parser cannot determine
        // the default value when the [No] ICompress phrase does not 
        // appear; therefore the return value does not have any meaning
        // when the neither the ICompress nor No ICompress phrase is
        // not specified.
        //
        // For Create Table statements, the default value is FALSE (no
        // compression).

  inline unsigned short getLockLength() const;

        // Returns the unsigned number appearing in the specified
        // LockLength phrase.  Returns 0 (the default value) when
        // the LockLength phrase is not specified.

  inline ULng32 getMaxSize() const;

        // Returns the value specified in the MaxSize clause.
        // If MaxSize clause does not appears, returns the default value.
        // If MaxSize Unbounded clause appears, the returned (default)
        // value has no meaning.
       
  inline ComUnits getMaxSizeUnit() const;

        // Returns the size increment (unit) specified in the MaxSize
        // clause.  If MaxSize clause or size unit do not appear, returns
        // the default size increment.  If MaxSize Unbounded clause
        // appears, the returned (default) size unit has not meaning. 

  NAString getMaxSizeUnitAsNAString() const;

        // Same as getMaxSizeUnit() except that the returned size unit
        // is in string format so it can be used for tracing purposes.

  inline ULng32 getPriExt() const;

        // Returns the value for the primary extent size specified in the Extent
	// clause.

  inline ULng32 getSecExt() const;

        // Returns the value for the secondary extent size specified in the Extent
	// clause.

  inline ULng32 getMaxExt() const;

        // Returns the value specified in the MaxExtents clause.

  inline Int64 getUID() const;

        // Returns the value specified in the UID clause.

  inline ElemDDLFileAttrRowFormat::ERowFormat getRowFormat() const;

        // Returns the value specified in the Row Format phrase;
        // returns ElemDDLFileAttrRowFormat::eUNSPECIFIED otherwise.

  inline NABoolean isAllocateSpecified() const;

        // Returns TRUE if the Allocate phrase appears;
        // returns FALSE otherwise.

  inline NABoolean isAuditCompress() const;

        // Same as getIsAuditCompress()

  inline NABoolean isAuditCompressSpecified() const;

        // Returns TRUE if AuditCompress phrase appears,
        // returns FALSE otherwise.

  inline NABoolean isBlockSizeSpecified() const;

        // Returns TRUE if the BlockSize phrase appears;
        // returns FALSE otherwise.

  inline NABoolean isBuffered() const;

        // Same as getIsBuffered()

  inline NABoolean isBufferedSpecified() const;

        // Returns TRUE if the [No] Buffered phrase appears;
        // returns FALSE otherwise.

  inline NABoolean isClearOnPurge() const;

        // Same as getIsClearOnPurge()

  inline NABoolean isClearOnPurgeSpecified() const;

        // Returns TRUE if the [No] ClearOnPurge phrase appears;
        // returns FALSE otherwise.

  inline NABoolean isDCompress() const;

        // Same as getIsDCompress()

  inline NABoolean isDCompressSpecified() const;

        // Returns TRUE if the DCompress phrase appears;
        // returns FALSE otherwise.

  inline NABoolean isICompress() const;

        // Same as getIsICompress()

  inline NABoolean isICompressSpecified() const;

        // Returns TRUE if the ICompress phrase appears;
        // returns FALSE otherwise.

  inline NABoolean isLockLengthSpecified() const;

        // Returns TRUE if the LockLength phrase appears;
        // returns FALSE otherwise.

  inline NABoolean isMaxSizeSpecified() const;

        // Returns TRUE if the MaxSize phrase appears;
        // returns FALSE otherwise.

  inline NABoolean isMaxSizeUnbounded() const;

        // Returns TRUE if the MaxSize Unbounded phrase appears;
        // returns FALSE otherwise.

  inline NABoolean isExtentSpecified() const;

        // Returns TRUE if the Extent phrase appears;
        // returns FALSE otherwise

  inline NABoolean isMaxExtentSpecified() const;

        // Returns TRUE if the MaxExtents phrase appears;
        // returns FALSE otherwise

  inline NABoolean isUIDSpecified() const;

        // Returns TRUE if UID phrase appears;
        // returns FALSE otherwise.

  inline NABoolean isRowFormatSpecified() const;

        // Returns TRUE if Row Format phrase appears;
        // returns FALSE otherwise.

  inline NABoolean isCompressionTypeSpecified() const;

        // Returns TRUE if the Compression Type phrase appears;
        // returns FALSE otherwise

  // mutators
  void setFileAttr(ElemDDLFileAttr * pFileAttrParseNode);
  inline void setIsBuffered(NABoolean setting);
  void copy(const ParDDLFileAttrsCreateIndex & createIndexFileAttributes);

  // trace
  NATraceList getDetailInfo() const;

private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  void initializeDataMembers();
  void resetAllIsSpecDataMembers();

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  // The flags is...Spec_ shows whether the corresponding file
  // attributes were specified in the DDL statement or not.  They
  // are used by the parser to look for duplicate clauses.

  // ALLOCATE controls amount of disk space allocated, in extents.
  // Default is to allocate space as needed.
  NABoolean       isAllocateSpec_;
  ComSInt16       extentsToAllocate_;
  
  // [ NO ] AUDITCOMPRESS
  NABoolean       isAuditCompressSpec_;
  NABoolean       isAuditCompress_;

  // BLOCKSIZE
  NABoolean       isBlockSizeSpec_;
  ULng32   blockSize_;  // in bytes

  // [ NO ] BUFFERED
  NABoolean       isBufferedSpec_;
  NABoolean       isBuffered_;

  // [ NO ] CLEARONPURGE
  NABoolean       isClearOnPurgeSpec_;
  NABoolean       isClearOnPurge_;

  // [ NO ] DCOMPRESS
  NABoolean       isDCompressSpec_;
  NABoolean       isDCompress_;

  // [ NO ] ICOMPRESS
  NABoolean       isICompressSpec_;
  NABoolean       isICompress_;

  // LOCKLENGTH
  NABoolean       isLockLengthSpec_;
  unsigned short  lockLength_;

  // MAXSIZE
  NABoolean       isMaxSizeSpec_;
  NABoolean       isMaxSizeUnbounded_;
  ULng32   maxSize_;
  ComUnits        maxSizeUnit_;

  // EXTENT
  NABoolean       isExtentSpec_;
  ULng32   priExt_;
  ULng32   secExt_;

  // MAXEXTENTS
  NABoolean       isMaxExtentSpec_;
  ULng32   maxExt_;

  // UID
  NABoolean       isUIDSpec_;  
  Int64		  UID_;

  // { ALIGNED | PACKED } FORMAT
  NABoolean       isRowFormatSpec_;
  ElemDDLFileAttrRowFormat::ERowFormat eRowFormat_;

  // COMPRESSION
  NABoolean          isCompressionTypeSpec_;
  ComCompressionType compressionType_;

}; // class ParDDLFileAttrsCreateIndex

// -----------------------------------------------------------------------
// definitions of inline methods for class ParDDLFileAttrsCreateIndex
// -----------------------------------------------------------------------

//
// accessors
//

inline ComSInt16
ParDDLFileAttrsCreateIndex::getExtentsToAllocate() const
{
  return extentsToAllocate_;
}

inline ULng32
ParDDLFileAttrsCreateIndex::getBlockSize() const
{
  return blockSize_;
}

inline NABoolean
ParDDLFileAttrsCreateIndex::getIsAuditCompress() const
{
  return isAuditCompress_;
}

inline NABoolean
ParDDLFileAttrsCreateIndex::getIsBuffered() const
{
  return isBuffered_;
}

inline NABoolean
ParDDLFileAttrsCreateIndex::getIsClearOnPurge() const
{
  return isClearOnPurge_;
}

inline ComCompressionType
ParDDLFileAttrsCreateIndex::getCompressionType() const
{
  return compressionType_;
}

inline NABoolean
ParDDLFileAttrsCreateIndex::getIsDCompress() const
{
  return isDCompress_;
}

inline NABoolean
ParDDLFileAttrsCreateIndex::getIsICompress() const
{
  return isICompress_;
}

inline unsigned short
ParDDLFileAttrsCreateIndex::getLockLength() const
{
  return lockLength_;
}

inline ULng32
ParDDLFileAttrsCreateIndex::getMaxSize() const
{
  return maxSize_;
}

inline ComUnits
ParDDLFileAttrsCreateIndex::getMaxSizeUnit() const
{
  return maxSizeUnit_;
}
  
inline ULng32
ParDDLFileAttrsCreateIndex::getPriExt() const
{
  return priExt_;
}

inline ULng32
ParDDLFileAttrsCreateIndex::getSecExt() const
{
  return secExt_;
}


inline ULng32
ParDDLFileAttrsCreateIndex::getMaxExt() const
{
  return maxExt_;
}

inline Int64
ParDDLFileAttrsCreateIndex::getUID() const
{
  return UID_;
}

inline ElemDDLFileAttrRowFormat::ERowFormat
ParDDLFileAttrsCreateIndex::getRowFormat() const
{
  return eRowFormat_;
}

// is Allocate phrase specified?
inline NABoolean
ParDDLFileAttrsCreateIndex::isAllocateSpecified() const
{
  return isAllocateSpec_;
}

inline NABoolean
ParDDLFileAttrsCreateIndex::isAuditCompress() const
{
  return getIsAuditCompress();
}

// is the [No] AuditCompress phrase specified?
inline NABoolean
ParDDLFileAttrsCreateIndex::isAuditCompressSpecified() const
{
  return isAuditCompressSpec_;
}

// is the BlockSize phrase specified?
inline NABoolean
ParDDLFileAttrsCreateIndex::isBlockSizeSpecified() const
{
  return isBlockSizeSpec_;
}

inline NABoolean
ParDDLFileAttrsCreateIndex::isBuffered() const
{
  return getIsBuffered();
}

// is the [No] Buffered phrase specified?
inline NABoolean
ParDDLFileAttrsCreateIndex::isBufferedSpecified() const
{
  return isBufferedSpec_;
}

inline NABoolean
ParDDLFileAttrsCreateIndex::isClearOnPurge() const
{
  return getIsClearOnPurge();
}

// is the [No] ClearOnPurge phrase specified?
inline NABoolean
ParDDLFileAttrsCreateIndex::isClearOnPurgeSpecified() const
{
  return isClearOnPurgeSpec_;
}

// is the compression phrase specified?
inline NABoolean
ParDDLFileAttrsCreateIndex::isCompressionTypeSpecified() const
{
  return isCompressionTypeSpec_;
}

inline NABoolean
ParDDLFileAttrsCreateIndex::isDCompress() const
{
  return getIsDCompress();
}

// is the DCompress phrase specified?
inline NABoolean
ParDDLFileAttrsCreateIndex::isDCompressSpecified() const
{
  return isDCompressSpec_;
}

inline NABoolean
ParDDLFileAttrsCreateIndex::isICompress() const
{
  return getIsICompress();
}

// is the ICompress phrase specified?
inline NABoolean
ParDDLFileAttrsCreateIndex::isICompressSpecified() const
{
  return isICompressSpec_;
}

// is the LockLength phrase specified?
inline NABoolean
ParDDLFileAttrsCreateIndex::isLockLengthSpecified() const
{
  return isLockLengthSpec_;
}

// is the MaxSize phrase specified?
inline NABoolean
ParDDLFileAttrsCreateIndex::isMaxSizeSpecified() const
{
  return isMaxSizeSpec_;
}

inline NABoolean
ParDDLFileAttrsCreateIndex::isMaxSizeUnbounded() const
{
  return isMaxSizeUnbounded_;
}

// is the Extent phrase specified?
inline NABoolean
ParDDLFileAttrsCreateIndex::isExtentSpecified() const
{
  return isExtentSpec_;
}

// is the MaxExtent phrase specified?
inline NABoolean
ParDDLFileAttrsCreateIndex::isMaxExtentSpecified() const
{
  return isMaxExtentSpec_;
}

// is the UID phrase specified?
inline NABoolean
ParDDLFileAttrsCreateIndex::isUIDSpecified() const
{
  return isUIDSpec_;
}

// is the Row Format phrase specified?
inline NABoolean
ParDDLFileAttrsCreateIndex::isRowFormatSpecified() const
{
  return isRowFormatSpec_;
}

//
// mutator
//

inline void
ParDDLFileAttrsCreateIndex::setIsBuffered(NABoolean setting)
{
  isBuffered_ = setting;
}
#endif // PARDDLFILEATTRSCREATEINDEX_H
