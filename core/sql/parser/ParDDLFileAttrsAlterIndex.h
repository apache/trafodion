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
#ifndef PARDDLFILEATTRSALTERINDEX_H
#define PARDDLFILEATTRSALTERINDEX_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ParDDLFileAttrsAlterIndex.h
 * Description:  class to contain all legal file attributes associating
 *               with the DDL statement Alter Index ... Attribute(s) --
 *               The parser constructs a parse node for each file
 *               attribute specified in a DDL statement.  Collecting all
 *               file attributes to a single object (node) helps the user
 *               to access the file attribute information easier.  The
 *               user does not need to traverse the parse sub-tree to look
 *               for each file attribute parse node associating with the
 *               DDL statement.  Default values will be assigned to file
 *               attributes that are not specified in the DDL statement.
 *
 *               Class ParDDLFileAttrsAlterIndex does not represent a
 *               parse node.  The class StmtDDLAlterIndexAttribute
 *               representing an Alter Index ... Attribute(s) parse node
 *               contains (has a has-a relationship with) the class
 *               ParDDLFileAttrsAlterIndex.
 *
 *
 * Created:      1/31/96
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
class ParDDLFileAttrsAlterIndex;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// class to contain all file attributes associating with a DDL statement
// -----------------------------------------------------------------------
class ParDDLFileAttrsAlterIndex : public ParDDLFileAttrs
{

public:

  // default constructor
  ParDDLFileAttrsAlterIndex(
       ParDDLFileAttrs::fileAttrsNodeTypeEnum fileAttrsNodeType
       = ParDDLFileAttrs::FILE_ATTRS_ALTER_INDEX);

  // copy constructor
  ParDDLFileAttrsAlterIndex(
       const ParDDLFileAttrsAlterIndex & alterIndexFileAttributes);

  // virtual destructor
  virtual ~ParDDLFileAttrsAlterIndex();

  // assignment operator
  ParDDLFileAttrsAlterIndex & operator = (
       const ParDDLFileAttrsAlterIndex &rhs);

  //
  // accessors
  //

  inline ComSInt16 getExtentsToAllocate() const;

  // Returns the number of extents specified in the ALLOCATE
  // phrase; returns the default values when the ALLOCATE
  // phrase does not appear.

  inline NABoolean getIsAuditCompress() const;

       // Returns TRUE when the AuditCompress phrase appears; returns
       // FALSE when the No AuditCompress phrase appears. The return
       // value has no meanings when neither the AuditCompress nor No
       // AuditCompress phrase appears.

  inline NABoolean getIsBuffered() const;

       // Returns TRUE when the Buffered phrase appears; returns
       // FALSE when the No Buffered phrase appears. The return
       // value has no meanings when neither the Buffered nor No
       // Buffered phrase appears.

  inline ComCompressionType getCompressionType() const;

      // Returns the value appearing in the specified CompressionType
      // phrase.

  inline NABoolean getIsClearOnPurge() const;

       // Returns TRUE when the ClearOnPurge phrase appears; returns
       // FALSE when the No ClearOnPurge phrase appears. The return
       // value has no meanings when neither the ClearOnPurge nor No
       // ClearOnPurge phrase appears.

  inline ULng32 getMaxSize() const;

       // Returns the value appearing in the specified MaxSize phrase.
       // The return value has no meanings when the MaxSize phrase
       // does not appear.
       
  inline ComUnits getMaxSizeUnit() const;

       // Returns the size increment (unit) appearing in the specified
       // MaxSize phrase.  If specified MaxSize phrase does not include
       // a size unit, returns the default size increment.  If the
       // specified MaxSize includes the keyword Unbounded or if the
       // MaxSize phrase does not appear, the return value does not
       // have any meanings.

  NAString getMaxSizeUnitAsNAString() const;

       // Same as getMaxSizeUnit() except that the returned size unit
       // is in string format so it can be used for tracing purposes.

  inline ULng32 getPriExt() const;

       // Returns the value appearing in the specified EXTENT phrase.
       // The return value has no meanings when the EXTENT phrase
       // does not appear.
 
  inline ULng32 getSecExt() const;

       // Returns the value appearing in the specified EXTENT phrase.
       // The return value has no meanings when the EXTENT phrase
       // does not appear.

  inline ULng32 getMaxExt() const;

       // Returns the value appearing in the specified MaxExtent phrase.
       // The return value has no meanings when the MaxExtent phrase
       // does not appear.

  inline NABoolean isAllocateSpecified() const;

       // Returns TRUE if the Allocate phrase appears;
       // returns FALSE otherwise.

  inline NABoolean isAuditCompress() const;

       // Same as getIsAuditCompress()

  inline NABoolean isAuditCompressSpecified() const;

       // Returns TRUE if the AuditCompress or No AuditCompress
       // phrase appears; returns FALSE otherwise.

  inline NABoolean isBuffered() const;

       // Same as getIsBuffered()

  inline NABoolean isBufferedSpecified() const;

       // Returns TRUE if the Buffered or No Buffered phrase
       // appears; returns FALSE if neither the Buffered nor
       // No Buffered phrase appears.

  inline NABoolean isClearOnPurge() const;

       // Same as getIsClearOnPurge()

  inline NABoolean isClearOnPurgeSpecified() const;

       // Returns TRUE if the [No] ClearOnPurge phrase appears;
       // returns FALSE if neither the ClearOnPurge nor No
       // ClearOnPurge phrase appears.

  inline NABoolean isDeallocateSpecified() const;

       // Returns TRUE if the Deallocate phrase appears;
       // returns FALSE otherwise.

  inline NABoolean isMaxSizeSpecified() const;

       // Returns TRUE if the MaxSize phrase appears;
       // returns FALSE otherwise.

  inline NABoolean isMaxSizeUnbounded() const;

       // Returns TRUE if the MaxSize Unbounded phrase appears;
       // returns FALSE otherwise.  The return value does not
       // any meaning when the MaxSize phrase does not appear.

  inline NABoolean isExtentSpecified() const;

       // Returns TRUE if the Extent phrase appears;
       // returns FALSE otherwise.

  inline NABoolean isMaxExtentSpecified() const;

       // Returns TRUE if the MaxExtent phrase appears;
       // returns FALSE otherwise.

  inline NABoolean isNoLabelUpdateSpecified() const;
      // Returns TRUE is the 'NO LABEL UPDATE' phrase appears;
      // returns FALSE otherwise.  // mutators

  inline NABoolean isNoLabelUpdate() const;

  inline NABoolean isCompressionTypeSpecified() const;
      // Returns TRUE if the COMPRESSION TYPE phrase appears;
      // returns FALSE otherwise.

  // mutators
  void setFileAttr(ElemDDLFileAttr * pFileAttrParseNode);
  void copy(const ParDDLFileAttrsAlterIndex & alterIndexFileAttributes);

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

  // ALLOCATE
  //
  //   controls amount of disk space allocated
  //
  NABoolean       isAllocateSpec_;
  ComSInt16       extentsToAllocate_;
  
  // [ NO ] AUDITCOMPRESS
  NABoolean       isAuditCompressSpec_;
  NABoolean       isAuditCompress_;

  // [ NO ] BUFFERED
  NABoolean       isBufferedSpec_;
  NABoolean       isBuffered_;

  // [ NO ] CLEARONPURGE
  NABoolean       isClearOnPurgeSpec_;
  NABoolean       isClearOnPurge_;

  // DEALLOCATE
  NABoolean       isDeallocateSpec_;
  
  // MAXSIZE
  NABoolean       isMaxSizeSpec_;
  NABoolean       isMaxSizeUnbounded_;
  ULng32   maxSize_;
  ComUnits        maxSizeUnit_;

  // EXTENT
  NABoolean       isExtentSpec_;
  ULng32   priExt_;
  ULng32   secExt_;

  // MAXEXTENT
  NABoolean       isMaxExtentSpec_;
  ULng32   maxExt_;

  // NO LABEL UPDATE
  NABoolean       isNoLabelUpdateSpec_;
  NABoolean       noLabelUpdate_;

  // COMPRESSION TYPE
  NABoolean          isCompressionTypeSpec_;
  ComCompressionType compressionType_;
}; // class ParDDLFileAttrsAlterIndex

// -----------------------------------------------------------------------
// definitions of inline methods for class ParDDLFileAttrsAlterIndex
// -----------------------------------------------------------------------

//
// accessors
//

inline ComSInt16
ParDDLFileAttrsAlterIndex::getExtentsToAllocate() const
{
  return extentsToAllocate_;
}

inline NABoolean
ParDDLFileAttrsAlterIndex::getIsAuditCompress() const
{
  return isAuditCompress_;
}

inline NABoolean
ParDDLFileAttrsAlterIndex::getIsBuffered() const
{
  return isBuffered_;
}

inline NABoolean
ParDDLFileAttrsAlterIndex::getIsClearOnPurge() const
{
  return isClearOnPurge_;
}

inline ULng32
ParDDLFileAttrsAlterIndex::getMaxSize() const
{
  return maxSize_;
}

inline ComUnits
ParDDLFileAttrsAlterIndex::getMaxSizeUnit() const
{
  return maxSizeUnit_;
}
  
inline ULng32
ParDDLFileAttrsAlterIndex::getPriExt() const
{
  return priExt_;
}

inline ULng32
ParDDLFileAttrsAlterIndex::getSecExt() const
{
  return secExt_;
}

inline ULng32
ParDDLFileAttrsAlterIndex::getMaxExt() const
{
  return maxExt_;
}

// is the Allocate phrase specified?
inline NABoolean
ParDDLFileAttrsAlterIndex::isAllocateSpecified() const
{
  return isAllocateSpec_;
}

inline NABoolean
ParDDLFileAttrsAlterIndex::isAuditCompress() const
{
  return getIsAuditCompress();
}

// is the [No] AuditCompress phrase specified?
inline NABoolean
ParDDLFileAttrsAlterIndex::isAuditCompressSpecified() const
{
  return isAuditCompressSpec_;
}

inline NABoolean
ParDDLFileAttrsAlterIndex::isBuffered() const
{
  return getIsBuffered();
}

// is the [No] Buffered phrase specified?
inline NABoolean
ParDDLFileAttrsAlterIndex::isBufferedSpecified() const
{
  return isBufferedSpec_;
}

inline NABoolean
ParDDLFileAttrsAlterIndex::isClearOnPurge() const
{
  return getIsClearOnPurge();
}

// is the [No] ClearOnPurge phrase specified?
inline NABoolean
ParDDLFileAttrsAlterIndex::isClearOnPurgeSpecified() const
{
  return isClearOnPurgeSpec_;
}

inline NABoolean
ParDDLFileAttrsAlterIndex::isCompressionTypeSpecified() const
{
  return isCompressionTypeSpec_;
}

inline ComCompressionType
ParDDLFileAttrsAlterIndex::getCompressionType() const
{
  return compressionType_;
}

// is the Deallocate phrase specified?
inline NABoolean
ParDDLFileAttrsAlterIndex::isDeallocateSpecified() const
{
  return isDeallocateSpec_;
}

// is the MaxSize phrase specified?
inline NABoolean
ParDDLFileAttrsAlterIndex::isMaxSizeSpecified() const
{
  return isMaxSizeSpec_;
}

inline NABoolean
ParDDLFileAttrsAlterIndex::isMaxSizeUnbounded() const
{
  return isMaxSizeUnbounded_;
}

// is the Extent phrase specified?
inline NABoolean
ParDDLFileAttrsAlterIndex::isExtentSpecified() const
{
  return isExtentSpec_;
}

// is the MaxExtents phrase specified?
inline NABoolean
ParDDLFileAttrsAlterIndex::isMaxExtentSpecified() const
{
  return isMaxExtentSpec_;
}
#endif // PARDDLFILEATTRSALTERINDEX_H
