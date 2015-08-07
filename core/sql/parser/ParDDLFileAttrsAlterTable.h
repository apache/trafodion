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
#ifndef PARDDLFILEATTRSALTERTABLE_H
#define PARDDLFILEATTRSALTERTABLE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ParDDLFileAttrsAlterTable.h
 * Description:  class to contain all legal file attributes associating
 *               with the DDL statement Alter Table ... Attribute(s) --
 *               The parser constructs a parse node for each file
 *               attribute specified in a DDL statement.  Collecting all
 *               file attributes to a single object (node) helps the user
 *               to access the file attribute information easier.  The
 *               user does not need to traverse the parse sub-tree to look
 *               for each file attribute parse node associating with the
 *               DDL statement.  Default values will be assigned to file
 *               attributes that are not specified in the DDL statement.
 *
 *               Class ParDDLFileAttrsAlterTable does not represent a
 *               parse node.  The class StmtDDLAlterTableAttribute
 *               representing an Alter Table ... Attribute(s) parse node
 *               contains (has a has-a relationship with) the class
 *               ParDDLFileAttrsAlterTable.
 *
 *
 * Created:      1/12/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLFileAttrMaxSize.h"
#include "ParDDLFileAttrs.h"
#include "ElemDDLFileAttrRangeLog.h"
#include "ElemDDLFileAttrMvsAllowed.h"
#include "ElemDDLFileAttrMvAudit.h"
#include "ElemDDLFileAttrMaxExtents.h"
#include "ComASSERT.h"


// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ParDDLFileAttrsAlterTable;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// class to contain all file attributes associating with a DDL statement
// -----------------------------------------------------------------------
class ParDDLFileAttrsAlterTable : public ParDDLFileAttrs
{

public:

  // default constructor
  ParDDLFileAttrsAlterTable(
       ParDDLFileAttrs::fileAttrsNodeTypeEnum fileAttrsNodeType
       = ParDDLFileAttrs::FILE_ATTRS_ALTER_TABLE);

  // copy constructor
  ParDDLFileAttrsAlterTable(
       const ParDDLFileAttrsAlterTable & alterTableFileAttributes);

  // virtual destructor
  virtual ~ParDDLFileAttrsAlterTable();

  // assignment operator
  ParDDLFileAttrsAlterTable & operator = (
       const ParDDLFileAttrsAlterTable &rhs);

  //
  // accessors
  //

  inline ComSInt16 getExtentsToAllocate() const;
  // Returns the number of extents specified in the ALLOCATE
  // phrase; returns the default values when the ALLOCATE
  // phrase does not appear.

  inline NABoolean getIsAudit() const;

       // Returns TRUE when the Audit phrase appears; returns FALSE
       // when the No Audit phrase appears. The return value has no
       // meanings when neither the Audit nor No Audit phrase
       // appears.

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

  inline NABoolean getIsClearOnPurge() const;

       // Returns TRUE when the ClearOnPurge phrase appears; returns
       // FALSE when the No ClearOnPurge phrase appears. The return
       // value has no meanings when neither the ClearOnPurge nor No
       // ClearOnPurge phrase appears.

  inline ComCompressionType getCompressionType() const;
     // Returns the value of COMPRESSION TYPE

  inline unsigned short getLockLength() const;

       // Returns the unsigned number appearing in the specified
       // LockLength phrase.  The return value has no meanings when
       // the LockLength phrase is not specified.

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

  inline ULng32 getMaxExt() const;
		
       // Returns the maxext phrase.

  inline NABoolean isAllocateSpecified() const;

       // Returns TRUE if the Allocate phrase appears;
       // returns FALSE otherwise.

  inline NABoolean isAuditCompress() const;

       // Same as getIsAuditCompress()

  inline NABoolean isAuditCompressSpecified() const;

       // Returns TRUE if the AuditCompress or No AuditCompress
       // phrase appears; returns FALSE otherwise.

  inline NABoolean isAuditSpecified() const;

       // returns TRUE if the Audit or No Audit phrase appears;
       // returns FALSE otherwise.

  inline NABoolean isAudited() const;

       // same as getIsAudit()

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

  inline NABoolean isCompressionTypeSpecified() const;

       // Returns TRUE if the Compression phrase appears;
       // returns FALSE otherwise.

  inline NABoolean isDeallocateSpecified() const;

       // Returns TRUE if the Deallocate phrase appears;
       // returns FALSE otherwise.

  inline NABoolean isLockLengthSpecified() const;

       // Returns TRUE if the LockLength phrase appears;
       // returns FALSE otherwise.

  inline NABoolean isMaxSizeSpecified() const;

       // Returns TRUE if the MaxSize phrase appears;
       // returns FALSE otherwise.

  inline NABoolean isMaxSizeUnbounded() const;

       // Returns TRUE if the MaxSize Unbounded phrase appears;
       // returns FALSE otherwise.  The return value does not
       // any meaning when the MaxSize phrase does not appear.


//++ MV
  
  inline ComRangeLogType getRangelogType() const;

  inline NABoolean isRangeLogSpecified() const;

  inline NABoolean isLockOnRefresh() const;

  inline NABoolean isLockOnRefreshSpecified() const;


  NABoolean isRangeLog() const
  {
	// XXXXXXXXMVSXXXXX BITMAP
	  printf("depricated rangelog attribute\n");
	  return TRUE;
  }

  inline NABoolean isInsertLog() const;

  inline NABoolean isInsertLogSpecified() const;

  inline ComMvsAllowed getMvsAllowedType() const;

  inline NABoolean isMvsAllowedSpecified() const;

//-- MV


  inline NABoolean isExtentSpecified() const;
      // Returns TRUE is the Extent phrase appears;
      // returns FALSE otherwise.

  inline NABoolean isMaxExtentSpecified() const;
      // Returns TRUE is the MaxExtents phrase appears;
      // returns FALSE otherwise.  // mutators

  inline NABoolean isNoLabelUpdateSpecified() const;
      // Returns TRUE is the 'NO LABEL UPDATE' phrase appears;
      // returns FALSE otherwise.  // mutators

  inline NABoolean isNoLabelUpdate() const;

  void setFileAttr(ElemDDLFileAttr * pFileAttrParseNode);
  void copy(const ParDDLFileAttrsAlterTable & alterTableFileAttributes);

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
  
  // [ NO ] AUDIT
  //
  //   controls TMF auditing
  //
  NABoolean       isAuditSpec_;
  NABoolean       isAudit_;

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

    // MAXEXTENT
  NABoolean       isMaxExtentSpec_;
  ULng32   maxExt_;

  // NO LABEL UPDATE
  NABoolean       isNoLabelUpdateSpec_;
  NABoolean       noLabelUpdate_;

//++ MV
  
  // RANGELOG
  NABoolean		  isRangeLogSpec_;
  ComRangeLogType rangelogType_;	

  // [NO] LOCKONREFRESH
  NABoolean       isLockOnRefreshSpec_;
  NABoolean       isLockOnRefresh_;

  // [NO] INSERTLOG
  NABoolean       isInsertLogSpec_;
  NABoolean       isInsertLog_;

  // MVS ALLOWED
  NABoolean			isMvsAllowedSpec_;
  ComMvsAllowed		mvsAllowedType_;
  
  // COMPRESSION TYPE { SOFTWARE | HARDWARE | NONE }
  ComCompressionType  compressionType_;
  NABoolean           isCompressionTypeSpec_;
//-- MV



}; // class ParDDLFileAttrsAlterTable

// -----------------------------------------------------------------------
// definitions of inline methods for class ParDDLFileAttrsAlterTable
// -----------------------------------------------------------------------

//
// accessors
//

inline ComSInt16
ParDDLFileAttrsAlterTable::getExtentsToAllocate() const
{
  return extentsToAllocate_;
}

inline NABoolean
ParDDLFileAttrsAlterTable::getIsAudit() const
{
  return isAudit_;
}

inline NABoolean
ParDDLFileAttrsAlterTable::getIsAuditCompress() const
{
  return isAuditCompress_;
}

inline NABoolean
ParDDLFileAttrsAlterTable::getIsBuffered() const
{
  return isBuffered_;
}

inline NABoolean
ParDDLFileAttrsAlterTable::getIsClearOnPurge() const
{
  return isClearOnPurge_;
}

inline unsigned short
ParDDLFileAttrsAlterTable::getLockLength() const
{
  return lockLength_;
}

inline ULng32
ParDDLFileAttrsAlterTable::getMaxSize() const
{
  return maxSize_;
}

inline ComUnits
ParDDLFileAttrsAlterTable::getMaxSizeUnit() const
{
  return maxSizeUnit_;
}
  
inline ULng32
ParDDLFileAttrsAlterTable::getMaxExt() const
{
  return maxExt_;
}

// is the Allocate phrase specified?
inline NABoolean
ParDDLFileAttrsAlterTable::isAllocateSpecified() const
{
  return isAllocateSpec_;
}

inline NABoolean
ParDDLFileAttrsAlterTable::isAuditCompress() const
{
  return getIsAuditCompress();
}

// is the [No] AuditCompress phrase specified?
inline NABoolean
ParDDLFileAttrsAlterTable::isAuditCompressSpecified() const
{
  return isAuditCompressSpec_;
}

// is the [No] Audit phrase appeared?
inline NABoolean
ParDDLFileAttrsAlterTable::isAuditSpecified() const
{
  return isAuditSpec_;
}

inline NABoolean
ParDDLFileAttrsAlterTable::isAudited() const
{
  return getIsAudit();
}

inline NABoolean
ParDDLFileAttrsAlterTable::isBuffered() const
{
  return getIsBuffered();
}

// is the [No] Buffered phrase specified?
inline NABoolean
ParDDLFileAttrsAlterTable::isBufferedSpecified() const
{
  return isBufferedSpec_;
}

inline NABoolean
ParDDLFileAttrsAlterTable::isClearOnPurge() const
{
  return getIsClearOnPurge();
}

// is the [No] ClearOnPurge phrase specified?
inline NABoolean
ParDDLFileAttrsAlterTable::isClearOnPurgeSpecified() const
{
  return isClearOnPurgeSpec_;
}

// is the Deallocate phrase specified?
inline NABoolean
ParDDLFileAttrsAlterTable::isDeallocateSpecified() const
{
  return isDeallocateSpec_;
}

// is the LockLength phrase specified?
inline NABoolean
ParDDLFileAttrsAlterTable::isLockLengthSpecified() const
{
  return isLockLengthSpec_;
}

// is the MaxSize phrase specified?
inline NABoolean
ParDDLFileAttrsAlterTable::isMaxSizeSpecified() const
{
  return isMaxSizeSpec_;
}


inline NABoolean
ParDDLFileAttrsAlterTable::isMaxSizeUnbounded() const
{
  return isMaxSizeUnbounded_;
}

inline ComCompressionType
ParDDLFileAttrsAlterTable::getCompressionType() const
{
  return compressionType_;
}

// is the Compression phrase specified?
inline NABoolean
ParDDLFileAttrsAlterTable::isCompressionTypeSpecified() const
{
  return isCompressionTypeSpec_;
}

//----------------------------------------------------------------------------
//++ MV
inline ComRangeLogType 
ParDDLFileAttrsAlterTable::getRangelogType() const
{
	return rangelogType_;
}

inline NABoolean 
ParDDLFileAttrsAlterTable::isRangeLogSpecified() const
{
	return isRangeLogSpec_;
}

inline NABoolean 
ParDDLFileAttrsAlterTable::isLockOnRefresh() const
{
	return isLockOnRefresh_;
}

inline NABoolean 
ParDDLFileAttrsAlterTable::isLockOnRefreshSpecified() const
{
	return isLockOnRefreshSpec_;

}


inline NABoolean 
ParDDLFileAttrsAlterTable::isInsertLog() const
{
	return isInsertLog_;
}

inline NABoolean 
ParDDLFileAttrsAlterTable::isInsertLogSpecified() const
{
	return isInsertLogSpec_;
}

inline ComMvsAllowed 
ParDDLFileAttrsAlterTable::getMvsAllowedType() const
{
	return mvsAllowedType_;	
}

inline NABoolean 
ParDDLFileAttrsAlterTable::isMvsAllowedSpecified() const
{
	return isMvsAllowedSpec_;
}
//-- MV
//----------------------------------------------------------------------------


// is the Extent phrase specified?
inline NABoolean
ParDDLFileAttrsAlterTable::isExtentSpecified() const
{
  return isExtentSpec_;
}

// is the MaxExtent phrase specified?
inline NABoolean
ParDDLFileAttrsAlterTable::isMaxExtentSpecified() const
{
  return isMaxExtentSpec_;
}

// is the MaxExtent phrase specified?
inline NABoolean
ParDDLFileAttrsAlterTable::isNoLabelUpdateSpecified() const
{
  return isNoLabelUpdateSpec_;
}

inline NABoolean
ParDDLFileAttrsAlterTable::isNoLabelUpdate() const
{
  return noLabelUpdate_;
}

#endif 

// PARDDLFILEATTRSALTERTABLE_H
