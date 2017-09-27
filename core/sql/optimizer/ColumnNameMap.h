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
#ifndef COLUMNNAMEMAP_H
#define COLUMNNAMEMAP_H
/* -*-C++-*-
**************************************************************************
*
* File:         ColumnNameMap.h        
* Description:  A name map for a Column
* Created:      4/27/94
* Language:     C++
*
*
*
*
*	Fast bind, fast find;
*	A proverb never stale in thrifty mind.
*		-- Shylock, Merchant of Venice, II:v
*
**************************************************************************
*/

// -----------------------------------------------------------------------

#include "ColumnDesc.h"
#include "TableDesc.h"
#include "ValueDesc.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ColumnNameMap;
class XCNM;

// ***********************************************************************
//
// ColumnNameMap : A column name map.
//
// A ColumnNameMap associates a column name  with its ValueId.
// A ColumnNameMap is transient and is used only during the
// name binding phase.
//
// The XCNM (Exposed Column Name Map) cache is a hash table of
// ColumnNameMaps that permits an associative lookup for column
// names that are qualified by an exposed table or correlation name.
// There is one entry in the XCNM for each column of every table 
// whose name is exposed in a given scope.
// 
// ***********************************************************************
#pragma nowarn(1506)   // warning elimination 
class ColumnNameMap : public NABasicObject
{
public:
  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  ColumnNameMap(const NAString& colRefName, 	// simple name of input value
                const ValueId& valId,
                CollHeap * h=0)
              : colRefName_(colRefName,h),
                valId_(valId),
                columnDesc_(NULL),
		duplicateFlag_(FALSE),
                qualColAmbiguousFlag_(FALSE) 
  {}

  ColumnNameMap(const NAString& colRefName, 	// simple name of column
                ColumnDesc *columnDesc,
                CollHeap * h=0,
		NABoolean dup = FALSE,
		NABoolean qualColAmbi = FALSE)
              : colRefName_(colRefName, h),
                valId_(columnDesc->getValueId()),
                columnDesc_(columnDesc),
		duplicateFlag_(dup),
		qualColAmbiguousFlag_(qualColAmbi)
  {}
    
  ColumnNameMap(const ColRefName& colRefName,  // colRefName including corrName
                ColumnDesc *columnDesc,
                CollHeap * h=0,
		NABoolean dup = FALSE,
		NABoolean qualColAmbi = FALSE)
              : colRefName_(colRefName.getColName(), 
			    colRefName.getCorrNameObj(), h),
                valId_(columnDesc->getValueId()),
                columnDesc_(columnDesc),
		duplicateFlag_(dup),
		qualColAmbiguousFlag_(qualColAmbi)
  {}

  // copy ctor
  ColumnNameMap (const ColumnNameMap & cnm, CollHeap * h=0) 
       : colRefName_(cnm.colRefName_, h),
         valId_(cnm.valId_),
         columnDesc_(cnm.columnDesc_),
         duplicateFlag_(cnm.duplicateFlag_),
	 qualColAmbiguousFlag_(cnm.qualColAmbiguousFlag_)
  {}

  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------
  const ColRefName& getColRefNameObj() const 	{ return colRefName_; }

  ColumnDesc *getColumnDesc() const 		{ return columnDesc_; }

  ValueId getValueId() const 			{ return valId_; }

  NABoolean isDuplicate() const 		{ return duplicateFlag_; }
  void setDuplicateFlag() 			{ duplicateFlag_ = TRUE; }

  NABoolean isQualifiedColumnAmbiguous()        
  { return qualColAmbiguousFlag_; }
  
  void setQualifiedColumnAmbiguousFlag()        
  {  qualColAmbiguousFlag_ = TRUE; }

  // ---------------------------------------------------------------------
  // Display/print, for debugging.
  // ---------------------------------------------------------------------
  void print(FILE* ofd = stdout,
	     const char* indent = DEFAULT_INDENT,
             const char* title = "ColumnNameMap")
  {
    if (strcmp(title, "")) {
      BUMP_INDENT(indent);
      fprintf(ofd,"%s%s %s",NEW_INDENT,title,NEW_INDENT);
    }
    colRefName_.print(ofd, indent, "", TRUE /* brief */);
    fprintf(ofd," %u%s",
      CollIndex(valId_),
      isDuplicate()? " dup" : "");
    if (columnDesc_) {
      if (columnDesc_->getColRefNameObj() != colRefName_) {
	fprintf(ofd, " ");
	columnDesc_->print(ofd, indent, "");
      }
      CMPASSERT(columnDesc_->getValueId() == valId_);
    } else
      fprintf(ofd," (input value)");
    if (strcmp(title, "")) fprintf(ofd,"\n");
  } // print()

  void display() { print(); }

  // ---------------------------------------------------------------------
  // The following methods are required for each descriptor.
  // ---------------------------------------------------------------------
  const ColRefName* getKey() const	{ return &colRefName_; }

  NABoolean operator==(const ColumnNameMap& other) const
					{ return this == &other; }

private:
  // ---------------------------------------------------------------------
  // Column name
  // ---------------------------------------------------------------------
  ColRefName colRefName_;
  
  // ---------------------------------------------------------------------
  // Column descriptor
  // ---------------------------------------------------------------------
  ColumnDesc *columnDesc_;

  // ---------------------------------------------------------------------
  // Its Value Identifier
  // ---------------------------------------------------------------------
  ValueId valId_;

  // ---------------------------------------------------------------------
  // Duplicate flag - TRUE if there is more than one column with this name
  // in scope.  This must be in this class, not in ColumnDesc;
  // otherwise you'd get false duplicate conditions in joins and unions.
  // ---------------------------------------------------------------------
  NABoolean duplicateFlag_;

  NABoolean qualColAmbiguousFlag_;

}; // class ColumnNameMap
#pragma warn(1506)  // warning elimination 

// ***********************************************************************
// Implementation for inline functions
// ***********************************************************************

// ***********************************************************************
// XCNM
//
// The Exposed Column Name Map for name lookup
// ***********************************************************************
// -- Initial size of the XCNM cache
#define XCNM_INIT_SIZE    61

class XCNM : public NAKeyLookup<ColRefName,ColumnNameMap>
{
public:
  XCNM(CollHeap* h/*=0*/) : 
    NAKeyLookup<ColRefName,ColumnNameMap> (XCNM_INIT_SIZE, 
					   NAKeyLookupEnums::KEY_INSIDE_VALUE,
					   h)
    {}

  // copy ctor
  XCNM (const XCNM & orig, CollHeap * h=0) :
       NAKeyLookup<ColRefName,ColumnNameMap> (orig, h) 
  {}
  
  ~XCNM() { clearAndDestroy(); }
private:
}; // class XCNM


#endif  /* COLUMNNAMEMAP_H */
