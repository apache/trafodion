/* -*-C++-*- */
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
 * File:         StmtDDLCreate.C
 * Description:  Methods for classes representing DDL Create Statements
 *
 *               Also contains definitions of non-inline methods of
 *               classes relating to view usages.
 *
 *
 * Created:      3/9/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#define   SQLPARSERGLOBALS_FLAGS	// must precede all #include's

#include <stdlib.h>
#ifndef NDEBUG
#include <iostream>
#endif
#include "AllElemDDLPartition.h"
#include "AllElemDDLParam.h"
#include "AllElemDDLUdr.h"
#include "AllStmtDDLCreate.h"
#include "BaseTypes.h"
#include "ComDiags.h"
#include "ComOperators.h"
#include "ComMisc.h"
#include "ComDistribution.h"
#include "ElemDDLConstraintCheck.h"
#include "ElemDDLConstraintPK.h"
#include "ElemDDLConstraintRI.h"
#include "ElemDDLConstraintUnique.h"
#include "ElemDDLFileAttrClause.h"
#include "ElemDDLGrantee.h"
#include "ElemDDLLibClientFilename.h"
#include "ElemDDLLibClientName.h"
#include "ElemDDLList.h"
#include "ElemDDLLocation.h"
#include "ElemDDLParallelExec.h"
#include "ElemDDLPartitionList.h"
#include "ElemDDLSchemaName.h"
#include "ElemDDLStoreOptions.h"
#include "ElemDDLWithCheckOption.h"
#include "ElemDDLIndexPopulateOption.h"
#include "ItemConstValueArray.h"
#include "StmtDDLAddConstraintPK.h"
#include "StmtDDLCreateLibrary.h"
#include "StmtDDLCreateSynonym.h"
#include "ElemDDLMVFileAttrClause.h"
#include "StmtDDLCreateExceptionTable.h"
#include "StmtDDLCommentOn.h"
#include "MVInfo.h"

#include "NumericType.h"

#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif

#include "SqlParserGlobals.h"	// must be last #include


// -----------------------------------------------------------------------
// ComUudfParamKind translation
// -----------------------------------------------------------------------

// Keep getting the folowing error on NSK platform when i define
// the following definitions in w:/common/ComMisc.cpp so i move
// them to here.
// *** RLD ERROR ***: Unresolved Text Symbo
// ComGetUudfParamKindAsLit__F16ComUudfParamKindR8NAString
// in file /G/fc0001/kdl0208d/mxcmp.

static
const literalAndEnumStruct ComUudfParamKindXlateArray [] =
{
  {COM_UUDF_PARAM_OMITTED,               COM_UUDF_PARAM_OMITTED_LIT},
  {COM_UUDF_PARAM_ACTION,                COM_UUDF_PARAM_ACTION_LIT},
  {COM_UUDF_PARAM_SAS_FORMAT,            COM_UUDF_PARAM_SAS_FORMAT_LIT},
  {COM_UUDF_PARAM_SAS_LOCALE,            COM_UUDF_PARAM_SAS_LOCALE_LIT},
  {COM_UUDF_PARAM_SAS_MODEL_INPUT_TABLE, COM_UUDF_PARAM_SAS_MODEL_INPUT_TABLE_LIT}
};

// define the enum-to-literal function
static
void ComGetUudfParamKindAsLit ( const ComUudfParamKind pv_enumValue  // in
                              , NAString &pr_literal                 // out
                              )
{
  NABoolean lv_found;
  char la_lit[100];
  enumToLiteral ( ComUudfParamKindXlateArray
                , occurs(ComUudfParamKindXlateArray)
                , pv_enumValue
                , la_lit
                , lv_found
                );
  ComASSERT(lv_found);
  pr_literal = la_lit;
}

// -----------------------------------------------------------------------
// definitions of file-scope function ParIsTracingViewUsages()
// -----------------------------------------------------------------------

NABoolean ParIsTracingViewUsages()
{
  return (getenv("SQLMX_TRACE_VIEW_USAGES") NEQ NULL);
}

// -----------------------------------------------------------------------
// definitions of non-inline methods for class ParViewTableColsUsage
// -----------------------------------------------------------------------

//
// default constructor
//


//
// initialize constructor
//

ParViewTableColsUsage::ParViewTableColsUsage(const ColRefName &colName, CollHeap * h)
  : tableName_(colName.getCorrNameObj().getQualifiedNameObj(), h),
    columnName_(colName.getColName(), h)
{}


//
// virtual destructor
//

ParViewTableColsUsage::~ParViewTableColsUsage()
{
}

//
// operator
//

NABoolean
ParViewTableColsUsage::operator==(const ParViewTableColsUsage &rhs) const
{
  if (this EQU &rhs)
  {
    return TRUE;
  }
  return (getColumnName()    EQU rhs.getColumnName() AND
          getTableQualName() EQU rhs.getTableQualName());
}

// -----------------------------------------------------------------------
// definitions of non-inline methods for class ParViewTableColsUsageList
// -----------------------------------------------------------------------

//
// constructor
//

ParViewTableColsUsageList::ParViewTableColsUsageList(CollHeap *heap)
  : LIST(ParViewTableColsUsage *)(heap),
    heap_(heap)
{
}

//
// virtual destructor
//

ParViewTableColsUsageList::~ParViewTableColsUsageList()
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    delete &operator[](i);
//    NADELETE(&operator[](i), ParViewTableColsUsage, heap_);
  }
}

//
// accessor
//

ParViewTableColsUsage * const
ParViewTableColsUsageList::find(const ColRefName &colName)
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    if (operator[](i).getColumnName()    EQU colName.getColName() AND
        operator[](i).getTableQualName() EQU colName.getCorrNameObj().
        getQualifiedNameObj())
    {
      return &operator[](i);
    }
  }
  return NULL;
}

//
// mutator
//

NABoolean
ParViewTableColsUsageList::insert(const ColRefName &colName)
{
  ParViewTableColsUsage * pCu = find(colName);
  if (pCu EQU NULL)  // not found
  {
    // ok to insert
    pCu = new(heap_) ParViewTableColsUsage(colName);
    CMPASSERT(pCu NEQ NULL);
    LIST(ParViewTableColsUsage *)::insert(pCu);
#ifndef NDEBUG
    if (ParIsTracingViewUsages())
    {
      NAString traceStr = "v-tcu: " +
                          colName.getCorrNameObj().getQualifiedNameObj().
                          getQualifiedNameAsAnsiString() +
                          "." + ToAnsiIdentifier(colName.getColName());
      cout << traceStr << endl;
//      *SqlParser_Diags << DgSqlCode(3066)  // kludge to print trace message
//        << DgString0(traceStr);
    }
#endif
    return TRUE;   // inserted successfully
  }
  else  // found
  {
    return FALSE;  // does nothing
  }
}

// -----------------------------------------------------------------------
// Definitions of non-inline methods of class ParViewColTablesUsage
// -----------------------------------------------------------------------

//
// constructors
//

ParViewColTablesUsage::ParViewColTablesUsage(
        const CollIndex usingViewColumnNumber,
        const ExtendedQualName &usedObjectName,
        CollHeap * h)
  : usingViewColumnNumber_(usingViewColumnNumber),
    usedObjectName_(usedObjectName, h)
{}

//
// virtual destructor
//

ParViewColTablesUsage::~ParViewColTablesUsage()
{
}

//
// operator
//

NABoolean
ParViewColTablesUsage::operator==(const ParViewColTablesUsage &rhs) const
{
  if (this EQU &rhs)
  {
    return TRUE;
  }
  return (getUsingViewColumnNumber() EQU rhs.getUsingViewColumnNumber() AND
          getUsedObjectName()  EQU rhs.getUsedObjectName());
}

// -----------------------------------------------------------------------
// Definitions of non-inline methods of class ParViewColTablesUsageList
// -----------------------------------------------------------------------

//
// constructor
//

ParViewColTablesUsageList::ParViewColTablesUsageList(CollHeap *heap)
  : LIST(ParViewColTablesUsage *)(heap),
    heap_(heap)
{
}

//
// virtual destructor
//

ParViewColTablesUsageList::~ParViewColTablesUsageList()
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    delete &operator[](i);
//    NADELETE(&operator[](i), ParViewColTablesUsage, heap_);
  }
}

//
// accessor
//

ParViewColTablesUsage * const
ParViewColTablesUsageList::find(const CollIndex usingViewColNum)
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    if (operator[](i).getUsingViewColumnNumber() EQU usingViewColNum)
    {
      return &operator[](i);
    }
  }
  return NULL;
}

const ParViewColTablesUsage * const
ParViewColTablesUsageList::find(const CollIndex usingViewColNum) const
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    if (operator[](i).getUsingViewColumnNumber() EQU usingViewColNum)
    {
      return &operator[](i);
    }
  }
  return NULL;
}

//
// mutator
//

NABoolean
ParViewColTablesUsageList::insert(const CollIndex usingViewColumnNumber,
                                  const ExtendedQualName &usedObjectName)
{
  ParViewColTablesUsage *vctu = find(usingViewColumnNumber);
  if (vctu EQU NULL)  // not found
  {
    // ok to insert
    vctu = new(heap_) ParViewColTablesUsage(usingViewColumnNumber,
                                               usedObjectName);
    CMPASSERT(vctu NEQ NULL);
    LIST(ParViewColTablesUsage *)::insert(vctu);
#ifndef NDEBUG
    if (ParIsTracingViewUsages())
    {
      NAString traceStr = "vct-u: " +
                          LongToNAString((Lng32)usingViewColumnNumber) +
                          "   " +
                          usedObjectName.getQualifiedNameObj().getQualifiedNameAsAnsiString();
      cout << traceStr << endl;
//      *SqlParser_Diags << DgSqlCode(3066)  // kludge to print trace message
//        << DgString0(traceStr);
    }
#endif
    return TRUE;
  }
  return FALSE;  // does nothing
}

// -----------------------------------------------------------------------
// Definitions of non-inline methods of class ParViewColTableColsUsage
// -----------------------------------------------------------------------

//
// constructors
//

ParViewColTableColsUsage::ParViewColTableColsUsage(CollHeap * h)
  : usingViewColumnNumber_(0),
    usedObjectColumnName_(h)
{}

ParViewColTableColsUsage::ParViewColTableColsUsage(
        const CollIndex usingViewColumnNumber,
        const ColRefName &usedObjectColumnName,
        CollHeap * h)
  : usingViewColumnNumber_(usingViewColumnNumber),
    usedObjectColumnName_(usedObjectColumnName, h)
{}

//
// virtual destructor
//

ParViewColTableColsUsage::~ParViewColTableColsUsage()
{
}

//
// operator
//

NABoolean
ParViewColTableColsUsage::operator==(const ParViewColTableColsUsage &rhs) const
{
  if (this EQU &rhs)
  {
    return TRUE;
  }
  return (getUsingViewColumnNumber() EQU rhs.getUsingViewColumnNumber() AND
          getUsedObjectColumnName()  EQU rhs.getUsedObjectColumnName());
}

// -----------------------------------------------------------------------
// Definitions of non-inline methods of class ParViewColTableColsUsageList
// -----------------------------------------------------------------------

//
// constructor
//

ParViewColTableColsUsageList::ParViewColTableColsUsageList(CollHeap *heap)
  : LIST(ParViewColTableColsUsage *)(heap),
    heap_(heap)
{
}

//
// virtual destructor
//

ParViewColTableColsUsageList::~ParViewColTableColsUsageList()
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    delete &operator[](i);
//    NADELETE(&operator[](i), ParViewColTableColsUsage, heap_);
  }
}

//
// accessors
//

ParViewColTableColsUsage * const
ParViewColTableColsUsageList::find(const CollIndex usingViewColNum)
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    if (operator[](i).getUsingViewColumnNumber() EQU usingViewColNum)
    {
      return &operator[](i);
    }
  }
  return NULL;
}

ParViewColTableColsUsage * const
ParViewColTableColsUsageList::find(const CollIndex usingViewColNum,
                                   const ColRefName &usedColRefName)
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    ParViewColTableColsUsage *const u = &(operator[](i));
    const ColRefName &uColRefName = u->getUsedObjectColumnName();
    if (u->getUsingViewColumnNumber() EQU usingViewColNum AND
        uColRefName.getCorrNameObj().getQualifiedNameObj() EQU
          usedColRefName.getCorrNameObj().getQualifiedNameObj() AND
        uColRefName.getColName() EQU usedColRefName.getColName())
    {
      return u;
    }
  }
  return NULL;
}

const CollIndex
ParViewColTableColsUsageList::getIndex(const CollIndex usingViewColNum) const
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    if (operator[](i).getUsingViewColumnNumber() EQU usingViewColNum)
    {
      return i;
    }
  }
  return NULL_COLL_INDEX;
}

const CollIndex
ParViewColTableColsUsageList::getIndexOfNextElem(const CollIndex
                                                 usingViewColNum) const
{
  for (CollIndex i = 0; i < entries(); i++)
  {
    if (operator[](i).getUsingViewColumnNumber() > usingViewColNum)
    {
      return i;
    }
  }
  return NULL_COLL_INDEX;
}

//
// mutators
//

void
ParViewColTableColsUsageList::clear()
{

  ParViewColTableColsUsage *temp1;

  for (CollIndex i = 0; i < entries(); i++)
  {
    //    NADELETE(&operator[](i), ParViewColTableColsUsage, heap_);


    //#define NADELETE(p,C,h)  \

    temp1 = &operator[](i);
    if (temp1) 
       {  
       if (heap_)
	   { 
           delete temp1;
//           (temp1)->~ParViewColTableColsUsage();
//           (heap_)->deallocateMemory((void*)temp1);
           }
        else
	   {
           (delete (ParViewColTableColsUsage *)temp1);
           }
       }       

#if 0
  (void) (!(&operator[](i)) 
          || 
  	  (
           (
            (h) ? 
             ((&operator[](i))->~ParViewColTableColsUsage(), (heap_)->deallocateMemory((void*)&operator[](i)))
            : 
             (delete (void*)&operator[](i))
           ), 
           0
          )
         )

#endif // 0
  }
  LIST(ParViewColTableColsUsage *)::clear();
}

NABoolean
ParViewColTableColsUsageList::insert(const CollIndex usingViewColumnNumber,
                                     const ColRefName &usedObjectColumnName)
{
  ParViewColTableColsUsage *vctcu = find(usingViewColumnNumber);
  if (vctcu EQU NULL OR  // not found
      find(usingViewColumnNumber,
           usedObjectColumnName) EQU NULL)
  {
    // ok to insert
    ParViewColTableColsUsage *u = new(heap_)
      ParViewColTableColsUsage(usingViewColumnNumber, usedObjectColumnName);
    CMPASSERT(u NEQ NULL);
    if (vctcu NEQ NULL)  // more than 1 elem. with same usingViewColumnNumber
    {
      LIST(ParViewColTableColsUsage *)::insertAt(
        getIndex(usingViewColumnNumber), u);
    }
    else
    {
      CollIndex iOfNextElem(getIndexOfNextElem(usingViewColumnNumber));
      if (iOfNextElem EQU NULL_COLL_INDEX)  // usingViewColumnNumber is biggest
      {
        LIST(ParViewColTableColsUsage *)::insert(u);  // append to end of list
      }
      else
      {
        LIST(ParViewColTableColsUsage *)::insertAt(iOfNextElem, u);
      }
    }
#ifndef NDEBUG
    if (ParIsTracingViewUsages())
    {
      NAString traceStr = "vctcu: " +
                          LongToNAString((Lng32)usingViewColumnNumber) +
                          "   " +
                          usedObjectColumnName.getCorrNameObj().
                          getQualifiedNameObj().getQualifiedNameAsAnsiString()
                          + "." +
                          ToAnsiIdentifier(usedObjectColumnName.getColName());
      cout << traceStr << endl;
//      *SqlParser_Diags << DgSqlCode(3066)  // kludge to print trace message
//        << DgString0(traceStr);
    }
#endif
    return TRUE;
  }
  return FALSE;
}

// -----------------------------------------------------------------------
// Definitions of non-inline methods of class ParViewUsages
// -----------------------------------------------------------------------

//
// constructor
//
ParViewUsages::ParViewUsages(CollHeap *heap)
: curViewColNum_(NULL_COLL_INDEX),
  isItmColRefInColInRowVals_(FALSE),
  isViewSurelyNotUpdatable_(FALSE),
  usedTableNameList_(heap),
  usedColRefList_(heap),
  viewColTablesUsageList_(heap),
  viewColTableColsUsageList_(heap)
{
}

//
// virtual destructor
//
ParViewUsages::~ParViewUsages()
{
}



const ParViewColTableColsUsageList &
ParViewUsages::getViewColTableColsUsageList() const
{
  return viewColTableColsUsageList_;
}

ParViewColTableColsUsageList &
ParViewUsages::getViewColTableColsUsageList()
{
  return viewColTableColsUsageList_;
}




// -----------------------------------------------------------------------
// Methods for class StmtDDLCreateSynonym
// -----------------------------------------------------------------------

//
// Constructor
//

StmtDDLCreateSynonym::StmtDDLCreateSynonym(const QualifiedName & synonymName,
                                           const QualifiedName & objectReference,
                                           ElemDDLNode *pOwner)
  : StmtDDLNode(DDL_CREATE_SYNONYM),
    synonymName_(synonymName, PARSERHEAP()),
    objectReference_(objectReference, PARSERHEAP())
{
  if (pOwner)
  {
    pOwner_ = pOwner->castToElemDDLGrantee();
    ComASSERT(pOwner_ NEQ NULL);
  }
  else
    pOwner_ = NULL;
}

//
// Virtual Destructor
// garbage collection is being done automatically by the NAString Class
//

StmtDDLCreateSynonym::~StmtDDLCreateSynonym()
{
  if (pOwner_)
    delete pOwner_;
}

//
// cast
//

StmtDDLCreateSynonym *
StmtDDLCreateSynonym::castToStmtDDLCreateSynonym()
{
   return this;
}

//
// mutators
//

//
// for tracing
//

const NAString
StmtDDLCreateSynonym::displayLabel1() const
{
   return NAString("Synonym name: " ) + getSynonymName();
}

const NAString
StmtDDLCreateSynonym::displayLabel2() const
{
   return NAString("Object Reference: ") + getObjectReference();
}

const NAString
StmtDDLCreateSynonym::getText() const
{
   return "StmtDDLCreateSynonym";
}

// -----------------------------------------------------------------------
// Methods for class StmtDDLCreateExceptionTable
// -----------------------------------------------------------------------

//
// Constructor
//

StmtDDLCreateExceptionTable::StmtDDLCreateExceptionTable(const QualifiedName & exceptionName,
                                                    const QualifiedName & objectReference)
  : StmtDDLNode(DDL_CREATE_EXCEPTION_TABLE),
    exceptionName_(exceptionName, PARSERHEAP()),
    objectReference_(objectReference, PARSERHEAP())
{
}

//
// Virtual Destructor
// garbage collection is being done automatically by the NAString Class
//

StmtDDLCreateExceptionTable::~StmtDDLCreateExceptionTable()
{}

//
// cast
//

StmtDDLCreateExceptionTable *
StmtDDLCreateExceptionTable::castToStmtDDLCreateExceptionTable()
{
   return this;
}

//
// for tracing
//

const NAString
StmtDDLCreateExceptionTable::displayLabel1() const
{
   return NAString("Exception table name: " ) + getExceptionName();
}

const NAString
StmtDDLCreateExceptionTable::displayLabel2() const
{
   return NAString("Object Reference: ") + getObjectReference();
}

const NAString
StmtDDLCreateExceptionTable::getText() const
{
   return "StmtDDLCreateExceptionTable";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLCreateCatalog
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLCreateCatalog::StmtDDLCreateCatalog(const NAString & aCatalogName,
                                           ElemDDLNode * pAttributeList)
  : StmtDDLNode(DDL_CREATE_CATALOG),
    catalogName_(aCatalogName, PARSERHEAP()),
    isLocationClauseSpec_(FALSE),
    locationName_(PARSERHEAP()),
    locationNameInputFormat_(ComLocationName::INPUT_NOT_SPECIFIED),
    locationNameType_(ElemDDLLocation::LOCATION_DEFAULT_NAME_TYPE),
    pLocationNode_(NULL),
    attributeList_(pAttributeList)
{

  //
  // Traverse the Create Catalog Attribute List sub-tree tp
  // look for specified attributes.  Set the corresponding data
  // members accordingly.  Also check for duplicate clauses.
  //

  if (pAttributeList NEQ NULL)
  {
    for (CollIndex i = 0; i < pAttributeList->entries(); i++)
    {
      setAttribute((*pAttributeList)[i]);
    }
  }

} // StmtDDLCreateCatalog::StmtDDLCreateCatalog()

//
// virtual destructor
//
StmtDDLCreateCatalog::~StmtDDLCreateCatalog()
{
  // delete all children
  for (Int32 i = 0; i < getArity(); i++)
  {
    delete getChild(i);
  }
}

//
// cast
//
StmtDDLCreateCatalog *
StmtDDLCreateCatalog::castToStmtDDLCreateCatalog()
{
  return this;
}

//
// accessors
//

Int32
StmtDDLCreateCatalog::getArity() const
{
  return MAX_STMT_DDL_CREATE_CATALOG_ARITY;
}

ExprNode *
StmtDDLCreateCatalog::getChild(Lng32 index)
{
  ComASSERT(index EQU INDEX_CREATE_CATALOG_ATTRIBUTE_LIST);
  return attributeList_;
}

//
// mutators
//

void
StmtDDLCreateCatalog::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index EQU INDEX_CREATE_CATALOG_ATTRIBUTE_LIST);
  if (pChildNode NEQ NULL)
  {
    attributeList_ = pChildNode->castToElemDDLNode();
  }
  else
  {
    attributeList_ = NULL;
  }
}

//
// Get the information in the parse node pointed by parameter
// pAttrNode.  Update the corresponding data members (in this
// class) accordingly.  Also check for duplicate clauses.
//
void
StmtDDLCreateCatalog::setAttribute(ElemDDLNode * pAttrNode)
{
  ComASSERT(pAttrNode NEQ NULL);

  //
  // Currently, the Create Catalog statement accepts only
  // one optional clause, the Location clause.
  //

  if (pAttrNode->castToElemDDLLocation() NEQ NULL)
  {
    //
    // Location clause was specified
    //

    if (isLocationClauseSpec_)
    {
      // Duplicate LOCATION clauses.
      *SqlParser_Diags << DgSqlCode(-3098);
    }

    //
    // Does not need to check the syntax of the specified location
    // name.  The constructor of class ElemDDLLocation has already
    // done that.
    //
    // Copies the information about the location name to this
    // node for easier access.
    //

    ElemDDLLocation * pLocation = pAttrNode->castToElemDDLLocation();
    ComASSERT(pLocation NEQ NULL);

    if (NOT pLocation->getPartitionName().isNull())
     *SqlParser_Diags << DgSqlCode(-3405);

    isLocationClauseSpec_    = TRUE;
    locationName_            = pLocation->getLocationName();
    locationNameInputFormat_ = pLocation->getLocationNameInputFormat();
    locationNameType_        = pLocation->getLocationNameType();
    pLocationNode_           = pLocation;
  }
  else
  {
    NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
  }
}

//
// methods for tracing
//

const NAString
StmtDDLCreateCatalog::displayLabel1() const
{
  return NAString("Catalog name: ") + getCatalogName();
}

const NAString
StmtDDLCreateCatalog::displayLabel2() const
{
  if (NOT getLocationName().isNull())
  {
    return NAString("Location name: ") + getLocationName();
  }
  else
  {
    return NAString("Location name not specified.");
  }
}

const NAString
StmtDDLCreateCatalog::displayLabel3() const
{
  if (NOT getLocationName().isNull())
  {
    ElemDDLLocation location(getLocationNameType(), getLocationName());
    return (NAString("Location name type: ") +
            location.getLocationNameTypeAsNAString());
  }
  else
  {
    return NAString();
  }
}

const NAString
StmtDDLCreateCatalog::getText() const
{
  return "StmtDDLCreateCatalog";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLCreateComponentPrivilege
// -----------------------------------------------------------------------

//
// constructor
//

StmtDDLCreateComponentPrivilege::StmtDDLCreateComponentPrivilege(
   const NAString & aComponentPrivilegeName,
   const NAString & aComponentPrivilegeAbbreviation,
   const NAString & aComponentName,
   const NABoolean isSystem,
   const NAString & aComponentPrivilegeDetailInformation,
   CollHeap       * heap) // default is PARSERHEAP()
  : StmtDDLNode(DDL_CREATE_COMPONENT_PRIVILEGE),
    componentPrivilegeName_(aComponentPrivilegeName, heap),
    componentPrivilegeAbbreviation_(aComponentPrivilegeAbbreviation, heap),
    componentName_(aComponentName, heap),
    isSystem_(isSystem),
    componentPrivilegeDetailInformation_(aComponentPrivilegeDetailInformation, heap) 
{
}

//
// virtual destructor
//

StmtDDLCreateComponentPrivilege::~StmtDDLCreateComponentPrivilege()
{
}

//
// virtual safe cast-down function
//

StmtDDLCreateComponentPrivilege *
StmtDDLCreateComponentPrivilege::castToStmtDDLCreateComponentPrivilege()
{
  return this;
}

//
// methods for tracing
//


const NAString
StmtDDLCreateComponentPrivilege::displayLabel1() const
{
  NAString aLabel("Component privilege name: ");
  aLabel += getComponentPrivilegeName();
  aLabel += " - Component privilege abbreviation: ";
  aLabel += getComponentPrivilegeAbbreviation();
  return aLabel;
}

const NAString
StmtDDLCreateComponentPrivilege::displayLabel2() const
{
  NAString aLabel("Component name: ");
  aLabel += getComponentName();
  if (isSystem_)
  {
     aLabel += " (SYSTEM)";
  }
  aLabel += " - Component privilege detail information: ";
  aLabel += getComponentPrivilegeDetailInformation();
  return aLabel;
}

const NAString
StmtDDLCreateComponentPrivilege::getText() const
{
  return "StmtDDLCreateComponentPrivilege";
}


//----------------------------------------------------------------------------
// MV - RG (refresh groups)
// ---------------------------------------------------------------------------
// methods for class StmtDDLCreateMvRGroup  - refresh groups
// ---------------------------------------------------------------------------



  // initialize constructor
StmtDDLCreateMvRGroup::StmtDDLCreateMvRGroup(
										const QualifiedName & mvRGroupName,
										CollHeap    * heap			)
		:StmtDDLNode(DDL_CREATE_MV_REFRESH_GROUP),
            mvRGroupQualName_(mvRGroupName, heap) //,
//			mvRGroupName_(mvRGroupQualName_.getQualifiedNameAsAnsiString())
{
	// XXXXXXXXXMVSXXXXXXXXXXXXXXX
}

  // virtual destructor
StmtDDLCreateMvRGroup::~StmtDDLCreateMvRGroup()
{



}

// cast
StmtDDLCreateMvRGroup * 
StmtDDLCreateMvRGroup::castToStmtDDLCreateMvRGroup()
{
	return this;


}


const NAString
StmtDDLCreateMvRGroup::displayLabel1() const
{

  return NAString("MV name: ") + getMvRGroupName();
}

const NAString
StmtDDLCreateMvRGroup::getText() const
{

  return "StmtDDLCreateMvRGroup";


}


/**********************************************************/
/****     B E G I N                                    ****/
/**********************************************************/

// -----------------------------------------------------------------------
// methods for class StmtDDLCreateTrigger
// -----------------------------------------------------------------------

// Constructor
StmtDDLCreateTrigger::StmtDDLCreateTrigger(
		       const QualifiedName & aTriggerName,
		       ParNameLocList * nameLocList,
		       NABoolean isAfter,
		       NABoolean hasRowOrTableInRefClause,
		       ComOperation iud_event,
		       ElemDDLNode * columnList,
		       const QualifiedName & aTableName,
		       NAString  * oldName,
		       NAString  * newName,
		       NABoolean isStatement,
                       ElemDDLNode * pOwner,
		       CollHeap    * heap)
        : StmtDDLNode(DDL_CREATE_TRIGGER),
	  triggerQualName_(aTriggerName, heap),
          nameLocListPtr_(nameLocList),  // copy ptr
          startPos_(nameLocList->getTextStartPosition()),
          endPos_(0),  // will be set during parsing by  ParSetTextEndPos()
	  isAfter_(isAfter),
	  hasCSInAction_(FALSE),
	  hasRowORTableInRefClause_(hasRowOrTableInRefClause),
	  iudEvent_(iud_event),
	  tableQualName_(aTableName, heap),
	  isStatement_(isStatement),
	  oldName_(oldName),
	  newName_(newName),
	  pActionExpression_(NULL),  // Initialize to NULL, set it later
	  pColumnList_(NULL),
          columnRefArray_(heap)
{
  setChild(TRIGGER_OPTIONAL_UPDATE_COLUMN_LIST, columnList);
  setChild(TRIGGER_OPTIONAL_OWNER, pOwner);
  // set the other child (action tree) later !!
}

// "patch" the create trigger node -- add the action tree
// (done after the ctor for StmtDDLCreateTrigger -- cause the parsing of the
//  action uses the StmtDDLCreateTrigger object to get the referencing info)
void StmtDDLCreateTrigger::setAction( RelExpr * actionExpression )
{ 
  setChild(TRIGGER_ACTION_EXPRESSION, actionExpression ); 
}

//
// virtual destructor
//
StmtDDLCreateTrigger::~StmtDDLCreateTrigger()
{
  // delete all children
  for (Int32 i = 0; i < getArity(); i++)
  {
    delete getChild(i);
  }
  // delete the name loc list
  if ( nameLocListPtr_ NEQ NULL) delete nameLocListPtr_ ;
  // delete the old/new strings
  if ( oldName_ ) delete oldName_ ;
  if ( newName_ ) delete newName_ ;
}

//
// cast virtual function
//
StmtDDLCreateTrigger *
StmtDDLCreateTrigger::castToStmtDDLCreateTrigger()
{
  return this;
}

//
// accessors
//

Int32
StmtDDLCreateTrigger::getArity() const
{
  return MAX_STMT_DDL_CREATE_TRIGGER_ARITY;
}

ExprNode *
StmtDDLCreateTrigger::getChild(Lng32 index)
{
  ComASSERT(index >= 0 AND index < getArity());
  switch (index)
  {
  case TRIGGER_OPTIONAL_UPDATE_COLUMN_LIST :
    return pColumnList_;
  case TRIGGER_ACTION_EXPRESSION :
    return pActionExpression_ ;
  case TRIGGER_OPTIONAL_OWNER :
    return pOwner_;
  default :   // can't be 'cause index was checked ...
    ABORT("internal logic error"); 
    break ; 
  }
  return NULL;  // for the compiler's peace of mind; can't be anyway ...
}

//
// mutators
//

void
StmtDDLCreateTrigger::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());
  switch (index) {
  case TRIGGER_OPTIONAL_UPDATE_COLUMN_LIST :
    if (pChildNode NEQ NULL) {
      pColumnList_ = pChildNode->castToElemDDLNode();
      ComASSERT(pColumnList_ NEQ NULL);
    }
    else pColumnList_ = NULL;
    break;

  case TRIGGER_ACTION_EXPRESSION :
    if (pChildNode NEQ NULL)  {
      pActionExpression_ = pChildNode->castToRelExpr();
      ComASSERT(pActionExpression_ NEQ NULL);
    }
    else  pActionExpression_ = NULL;
    break;

  case TRIGGER_OPTIONAL_OWNER:
    if (pChildNode NEQ NULL) { 
      ElemDDLNode *pTemp = pChildNode->castToElemDDLNode();
      ComASSERT(pTemp NEQ NULL);
      ElemDDLGrantee *pOwner = pTemp->castToElemDDLGrantee();
      ComASSERT(pOwner NEQ NULL);
      pOwner_ = pOwner;
    }
    else pOwner_ = NULL;
    break;
     
  default :  // can't be 'cause index was checked ...
    ABORT("internal logic error");
  }
}

//
// collects information in the parse sub-tree and copy/move them
// to the current parse node.
//
void
StmtDDLCreateTrigger::synthesize()
{
  ExprNode * columnList = getChild(TRIGGER_OPTIONAL_UPDATE_COLUMN_LIST) ;

  if ( columnList NEQ NULL ) {  // In case there are (update) columns defined

    ComASSERT( getIUDEvent() EQU COM_UPDATE ) ;  // only update can have cols

    ElemDDLNode *pColumnList = columnList->castToElemDDLNode();

    ComASSERT( pColumnList NEQ NULL ) ;

    //
    // Initialize columnRefArray_ so the user can access
    // the information in the Column Reference parse nodes
    // easier.
    //
    
    CollIndex i;
    CollIndex nbrColList = pColumnList->entries();
    for (i = 0; i < nbrColList; i++) {
      ElemDDLColRef *nextElem = (*pColumnList)[i]->castToElemDDLColRef() ;
      ComASSERT( nextElem NEQ NULL);
      columnRefArray_.insert( nextElem );
    }
  } // if

} // StmtDDLCreateTrigger::synthesize()

//
// methods for tracing
//

const NAString
StmtDDLCreateTrigger::displayLabel1() const
{
  return NAString("Trigger name: ") + getTriggerName();
}

const NAString
StmtDDLCreateTrigger::displayLabel2() const
{
  return NAString("Table name: ") + getTableName();
}

NATraceList
StmtDDLCreateTrigger::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "Trigger name: ";
  detailText += getTriggerName();
  detailTextList.append(detailText);

  detailText = "Table name: ";
  detailText += getTableName();
  detailTextList.append(detailText);

  CollIndex            i, nbrCols;
  ElemDDLColRefArray   colsList;
  ElemDDLColRef      * col;

  colsList = getColRefArray();
  nbrCols = colsList.entries();

  if (nbrCols EQU 0)
  {
    detailTextList.append("No columns.");
  }
  else
  {
    detailText = "Column list [";
    detailText += LongToNAString((Lng32)nbrCols);
    detailText += " column(s)]:";
    detailTextList.append(detailText);

    for (i = 0; i < nbrCols; i++)
    {
      col = colsList[i];

      detailText = "[column ";
      detailText += LongToNAString((Lng32) i);
      detailText += "]";
      detailTextList.append(detailText);

      detailTextList.append("    ", col->getDetailInfo());
    }
  } // else (nbrCols EQU 0) of column references

  //
  // miscellaneous options
  //

  detailText = "is After?         ";
  detailText += YesNo(isAfter());
  detailTextList.append(detailText);
  detailText = "is Statement?         ";
  detailText += YesNo(isStatement());
  detailTextList.append(detailText);

  return detailTextList;

} // StmtDDLCreateTrigger::getDetailInfo()

const NAString
StmtDDLCreateTrigger::getText() const
{
  return "StmtDDLCreateTrigger";
}

/**********************************************************/
/****     E N D                                        ****/
/**********************************************************/

// -----------------------------------------------------------------------
// methods for class StmtDDLCreateIndex
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLCreateIndex::StmtDDLCreateIndex(NABoolean isUnique,
                                       const NAString & anIndexName,
                                       const QualifiedName & aTableName,
                                       ElemDDLNode * pColumnList,
                                       NABoolean posIgnore,
                                       ElemDDLNode * pOptionList,
                                       CollHeap    * heap)
        : StmtDDLNode(DDL_CREATE_INDEX),
          isUnique_(isUnique),
          indexName_(anIndexName, heap),
          indexQualName_(heap),
          origTableQualName_(heap),
          tableQualName_(aTableName, heap),
          locationName_(heap),
          partitionName_(heap),
          isLocationClauseSpec_(FALSE),
          locationNameType_(ElemDDLLocation::LOCATION_DEFAULT_NAME_TYPE),
          partitioningScheme_ (COM_NO_PARTITIONING),
          guardianLocation_(heap),
          isPartitionClauseSpec_(FALSE),
          isPartitionByClauseSpec_(FALSE),
          isDivisionClauseSpec_(FALSE),
          pDivisionClauseParseNode_(NULL),
          isHbaseOptionsSpec_(FALSE),
	  pHbaseOptionsParseNode_(NULL),
          pSaltOptions_(NULL),
          isParallelExec_(FALSE),
          configFileName_(heap),
          isParallelExecutionClauseSpec_(FALSE),
          isAttributeClauseSpec_(FALSE),
          pPrimaryPartition_(NULL),
          columnRefArray_(heap),
          partitionArray_(heap),
          partitionKeyColRefArray_(heap),
          isPopulated_(TRUE),              // default is always populated.
          isNoPopulated_(FALSE),
	  populateCount_(0),
          noPopulateCount_(0),
          isIndexOnGhostTable_(FALSE),
          posIgnore_(posIgnore),
	  isNumRowsSpecified_(FALSE),
	  numRows_(-1)
{
  setChild(INDEX_COLUMN_REF_LIST, pColumnList);
  setChild(INDEX_OPTION_LIST, pOptionList);

}

//
// virtual destructor
//
StmtDDLCreateIndex::~StmtDDLCreateIndex()
{
  // Delete the Primary Partition parse node.  This parse node
  // is not part of the parse tree.  The former was created
  // during the construction of the Create Index parse node
  // (this object).

  delete pPrimaryPartition_;

  // delete all children
  for (Int32 i = 0; i < getArity(); i++)
  {
    delete getChild(i);
  }
}

//
// cast virtual function
//
StmtDDLCreateIndex *
StmtDDLCreateIndex::castToStmtDDLCreateIndex()
{
  return this;
}

//
// accessors
//

Int32
StmtDDLCreateIndex::getArity() const
{
  return MAX_STMT_DDL_CREATE_INDEX_ARITY;
}

ExprNode *
StmtDDLCreateIndex::getChild(Lng32 index)
{
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

const NAString
StmtDDLCreateIndex::getTableName() const
{
  return tableQualName_.getQualifiedNameAsAnsiString();
}

//
// mutators
//

void
StmtDDLCreateIndex::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());
  if (pChildNode EQU NULL)
  {
    children_[index] = NULL;
  }
  else
  {
    children_[index] = pChildNode->castToElemDDLNode();
  }
}

//
// Copies information in the specified file attributes clause
// to the data member fileAttributes_.
//
void
StmtDDLCreateIndex::setFileAttributes(ElemDDLFileAttrClause * pFileAttrClause)
{
  ComASSERT(pFileAttrClause NEQ NULL);

  if (isAttributeClauseSpec_)
  {
    // Duplicate file ATTRIBUTE(S) clauses.
    *SqlParser_Diags << DgSqlCode(-3099);
  }
  isAttributeClauseSpec_ = TRUE;

  ElemDDLNode * pFileAttrs = pFileAttrClause->getFileAttrDefBody();
  ComASSERT(pFileAttrs NEQ NULL);

  for (CollIndex i = 0; i < pFileAttrs->entries(); i++)
  {
    getFileAttributes().setFileAttr((*pFileAttrs)[i]->castToElemDDLFileAttr());
  }

} // StmtDDLCreateIndex::setFileAttributes()

void
StmtDDLCreateIndex::setIndexOption(ElemDDLNode * pIndexOption)
{
  switch (pIndexOption->getOperatorType())
  {
  case ELM_FILE_ATTR_CLAUSE_ELEM :
    setFileAttributes(pIndexOption->castToElemDDLFileAttrClause());
    break;

  case ELM_LOCATION_ELEM :
    //
    // Currently, Location clause only contains one location element
    // (represented by class ElemDDLLocation); therefore, there is no
    // need to define class ElemDDLLocationClause to check for duplicate
    // Location clauses.
    //
    ComASSERT(pIndexOption->castToElemDDLLocation() NEQ NULL);
    if (isLocationClauseSpec_)
    {
      // Duplicate LOCATION clauses.
      *SqlParser_Diags << DgSqlCode(-3098);
    }
    isLocationClauseSpec_ = TRUE;
    {
      ElemDDLLocation * pLocation = pIndexOption->castToElemDDLLocation();
      locationName_     = pLocation->getLocationName();
      locationNameType_ = pLocation->getLocationNameType();
	  partitionName_    = pLocation->getPartitionName();
    }
    break;
  case ELM_WITH_POPULATE_OPTION_ELEM :
    {
      ElemDDLIndexPopulateOption * pPopulateIndex = pIndexOption->castToElemDDLIndexPopulateOption();
      //10-030303-4626-begin
      populateCount_ += pPopulateIndex->getPopulateOptionCount();
      noPopulateCount_ += pPopulateIndex->getNoPopulateOptionCount();
      if((populateCount_ >= 1) && (noPopulateCount_ >= 1))
      {
      //10-030303-4626-end
        // Error message. Populate and no populate can't coexist in the same clause.
        *SqlParser_Diags << DgSqlCode (-3155);
      }
      else
      {
	//10-030303-4626-begin
	if((populateCount_ > 1) || (noPopulateCount_ > 1))
	{
		//  Error message. Mutiple Populate OR No Populate
		//  can't exist in the same clause.
	       	*SqlParser_Diags << DgSqlCode (-3183) << DgString0(" [NO] POPULATE ");
	}
	else
	{
		// No Syntax errors.
		isNoPopulated_ = pPopulateIndex->getNoPopulateOption();
		isPopulated_   = pPopulateIndex->getPopulateOption();
	}
	//10-030303-4626-end
      }
    }
    break;
  case ELM_PARTITION_CLAUSE_ELEM :
    setPartitions(pIndexOption->castToElemDDLPartitionClause());
    break;

  case ELM_DIVISION_CLAUSE_ELEM :
    ComASSERT(pIndexOption->castToElemDDLDivisionClause() NEQ NULL);
    if (isDivisionClauseSpecified())
    {
      // Error 3183 - Duplicate $0~string0 clauses were specified.
      *SqlParser_Diags << DgSqlCode(-3183)
                       << DgString0("DIVISION");
    }
    pDivisionClauseParseNode_ =
      pIndexOption->castToElemDDLDivisionClause();
    isDivisionClauseSpec_ = TRUE;
    break;

  case ELM_PARALLEL_EXEC_ELEM :
    {
      ElemDDLParallelExec * pParallelExec =
        pIndexOption->castToElemDDLParallelExec();
      ComASSERT(pParallelExec NEQ NULL);
      if (isParallelExecutionClauseSpecified())
      {
        // Duplicate PARALLEL EXECUTION clauses.
        *SqlParser_Diags << DgSqlCode(-3102); 
      }
      isParallelExecutionClauseSpec_ = TRUE;
      configFileName_ = pParallelExec->getConfigFileName();
      isParallelExec_ = pParallelExec->isParallelExecEnabled();
    }
    break;

  case ELM_FILE_ATTR_POS_TABLE_SIZE_ELEM:
    {
      if (pIndexOption->castToElemDDLFileAttrPOSTableSize()->getNumRows() != -1) 
	{
	  if (isNumRowsSpecified_)
	    {
	      // Duplicate phrases.
	      *SqlParser_Diags << DgSqlCode(-3407);
	    }
	  numRows_ = 
	    pIndexOption->castToElemDDLFileAttrPOSTableSize()->getNumRows();
	  isNumRowsSpecified_ = TRUE;
	}

    }
    break;
    
  case ELM_HBASE_OPTIONS_ELEM:
    {
      if (isHbaseOptionsSpecified())
        {
          // Error 3183 - Duplicate $0~string0 clauses were specified.
          *SqlParser_Diags << DgSqlCode(-3183)
                           << DgString0("HBASE_OPTIONS");
        }
      pHbaseOptionsParseNode_ =
        pIndexOption->castToElemDDLHbaseOptions();
      isHbaseOptionsSpec_ = TRUE;
    }
    break ;
  case ELM_SALT_OPTIONS_ELEM:
    {
      if (pSaltOptions_)
        // Error 3183 - Duplicate $0~string0 clauses were specified.
        *SqlParser_Diags << DgSqlCode(-3183) << DgString0("SALT");
      else
        pSaltOptions_ = pIndexOption->castToElemDDLSaltOptionsClause();
    }
    break;
  default :
    NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
    break;
  }
}

//
// Copies the information in the specified Partition clause
// to this object.
//
void
StmtDDLCreateIndex::setPartitions(ElemDDLPartitionClause * pPartitionClause)
{
  ComASSERT(pPartitionClause NEQ NULL);

  if (isPartitionSpecified())
  {
    // Duplicate PARTITION clauses.
    *SqlParser_Diags << DgSqlCode(-3103);
  }
  isPartitionClauseSpec_ = TRUE;

  //
  // Initializes pPartitions to point to the parse node representing
  // the definition of a partition (class ElemDDLPartition) or
  // a list of partition definitions (class ElemDDLPartitionList).
  //
  ElemDDLNode * pPartitions = pPartitionClause->getPartitionDefBody();

  //
  // Copy the partitioning type from the partition clause
  //
  partitioningScheme_ = pPartitionClause->getPartitionType();

  //
  // Constructs the primary partition node and then inserts
  // its pointer at the beginning of the partitionArray_.
  //
  // Note that the partition clause does not include the
  // definition of the primary partition.
  //
  // The kind of the primary partition node must be the
  // same as that of the secondary partition node.
  //
  // Note that for indexes with only a single partition,
  // pPartitions contains the NULL pointer value.
  //

  if (pPartitions EQU NULL)
    setPrimaryPartition(NULL);
  else
    // (*pPartitions)[0] points to the parse node
    // representing the first secondary partition.
    setPrimaryPartition((*pPartitions)[0]);

  //
  // Scans the list of the definitions of the secondary
  // partition.  Copies the information to the partitionArray_.
  //

  if (pPartitions NEQ NULL)
    for (CollIndex i = 0; i < pPartitions->entries(); i++)
    {
      setSecondaryPartition((*pPartitions)[i]->castToElemDDLPartition());
    }

  //
  // Initializes pPartitionByOption to point to the parse node
  // representing the definition of partition by columns (class
  // ElemDDLPartitionByOpt).
  //
  ElemDDLNode *pPartitionByOption = pPartitionClause->getPartitionByOption();

  if(pPartitionByOption NEQ NULL)
  {
    // If the Partition By Option is specified, set the flag and copy
    // the pointers to column parse nodes to partitionKeyColRefArray_.
    if(pPartitionByOption->castToElemDDLPartitionByOpt() NEQ NULL)
    {
      isPartitionByClauseSpec_ = TRUE;

      switch (pPartitionByOption->getOperatorType())
      {
      case ELM_PARTITION_BY_COLUMN_LIST_ELEM :
        ComASSERT(pPartitionByOption->castToElemDDLPartitionByColumnList()
                                                                     NEQ NULL);
                {
          //
          // Copies array of pointers pointing to Column parse nodes to
          // PartitionKeyColRefArray_ so the user of this object can access
          // the information easier.
          //
          ElemDDLNode * pPartitionByCols = pPartitionByOption->
              castToElemDDLPartitionByColumnList()->getPartitionKeyColumnList();
          NABoolean columnOrderingSpecified = FALSE;
          for (CollIndex index = 0; index < pPartitionByCols->entries(); index++)
          {
            ComASSERT((*pPartitionByCols)[index]->castToElemDDLColRef() NEQ NULL);
            // Report error if ordering was specified for any column.
            if ((*pPartitionByCols)[index]->castToElemDDLColRef()->isColumnOrderingSpecified())
            {
              // It is not allowed to specify ordering for columns of a partitioning key clause.
              // Report an error only once for the column list - not for each offending column.
              if (NOT columnOrderingSpecified)
              {
                *SqlParser_Diags << DgSqlCode(-3199)
                  << DgString0((*pPartitionByCols)[index]->castToElemDDLColRef()->getColumnName());
                columnOrderingSpecified = TRUE;
              }
            }
            partitionKeyColRefArray_.insert(
                         (*pPartitionByCols)[index]->castToElemDDLColRef());
          }
                }
        break;

      default :
        NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
        break;
      }  // switch
    }
  } // if(pPartitionByOption NEQ NULL)

}

//
// Constructs the primary partition node and then inserts its pointer
// at the beginning of the partitionArray_.  The kind of the primary
// partition must match that of the secondary partition[s].
//
// This method is only invoked when the partition clause appears.
//
void
StmtDDLCreateIndex::setPrimaryPartition(ElemDDLNode * pFirstSecondaryPartition)
{
  if (pFirstSecondaryPartition EQU NULL)  // single partition
  {
    if (getPartitioningScheme() EQU COM_HASH_V1_PARTITIONING ||
        getPartitioningScheme() EQU COM_HASH_V2_PARTITIONING)
      // The HASH PARTITION or HASH2 PARTITION clause was specified.
      pPrimaryPartition_ = new(PARSERHEAP())ElemDDLPartitionSystem();
    else
      pPrimaryPartition_ = new(PARSERHEAP())ElemDDLPartitionRange();
  }
  else
  switch (pFirstSecondaryPartition->getOperatorType())
  {
  case ELM_PARTITION_RANGE_ELEM :
    pPrimaryPartition_ = new(PARSERHEAP()) ElemDDLPartitionRange();
    break;
  case ELM_PARTITION_SYSTEM_ELEM :
    // We use ElemDDLPartitionSystem parse node for HASH partitions.
    pPrimaryPartition_ = new(PARSERHEAP()) ElemDDLPartitionSystem();
    break;
  //
  // Other future partitioning schemes
  //
  default :
    NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
    break;
  }
  partitionArray_.insert(pPrimaryPartition_);
}

void
StmtDDLCreateIndex::setSecondaryPartition(ElemDDLPartition * pPartition)
{
  ComASSERT(pPartition NEQ NULL);

  //
  // Inserts the specified secondary partition node to the end
  // of the partitionArray_.  Note that secondary partitions
  // are defined in the partition clause.
  //

  switch (pPartition->getOperatorType())
  {
  case ELM_PARTITION_RANGE_ELEM :
  case ELM_PARTITION_SYSTEM_ELEM :
    //
    // Note that class ElemDDLPartitionRange is derived
    // from class ElemDDLPartitionSystem, and the latter
    // is derived from class ElemDDLPartition.
    //
    ComASSERT(pPartition->castToElemDDLPartitionSystem() NEQ NULL);
    if (pPartition->castToElemDDLPartitionSystem()->getOption()
      NEQ ElemDDLPartition::ADD_OPTION)
    {
      // Only ADD option allowed in PARTITION clause in CREATE TABLE statement.
      *SqlParser_Diags << DgSqlCode(-3104);
    }
    partitionArray_.insert(pPartition->castToElemDDLPartition());
    break;

  //
  // Other future partitioning schemes; e.g., hash partitioning
  //

  default :
    NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
    break;

  } // switch (pPartition->getOperatorType())

} // StmtDDLCreateIndex::setSecondaryPartition()

//
// collects information in the parse sub-tree and copy/move them
// to the current parse node.
//
void
StmtDDLCreateIndex::synthesize()
{
  indexQualName_ = QualifiedName(getIndexName(),
				 tableQualName_.getSchemaName(),
				 tableQualName_.getCatalogName(),
				 PARSERHEAP());

  if (isVolatile())
    indexQualName_.setIsVolatile(TRUE);

  ComASSERT(getChild(INDEX_COLUMN_REF_LIST) NEQ NULL);

  ElemDDLNode *pColumnList =
    getChild(INDEX_COLUMN_REF_LIST)->castToElemDDLNode();

  ComASSERT(pColumnList NEQ NULL);

  //
  // Initialize columnRefArray_ so the user can access
  // the information in the Column Reference parse nodes
  // easier.
  //

  CollIndex i;
  CollIndex nbrColList = pColumnList->entries();
  for (i = 0; i < nbrColList; i++)
  {
    ComASSERT((*pColumnList)[i]->castToElemDDLColRef() NEQ NULL);
    columnRefArray_.insert((*pColumnList)[i]->castToElemDDLColRef());
  }

  //
  // Collect information in the Index option parse nodes
  // if they exist.  An index option can be a file attribute
  // or a load option.
  //

  if (getChild(INDEX_OPTION_LIST) NEQ NULL)
  {
    ElemDDLNode *pOptionList =
      getChild(INDEX_OPTION_LIST)->castToElemDDLNode();

    ComASSERT(pOptionList NEQ NULL);

    if (pOptionList->castToElemDDLOptionList() NEQ NULL)
    {
      CollIndex nbrOptList = pOptionList->entries();
      for (i = 0; i < nbrOptList; i++)
      {
        setIndexOption((*pOptionList)[i]);
      }
    }
    else
    {
      //
      // only a single option (which may be an array of file
      // attributes) encountered.
      //
      setIndexOption(pOptionList);
    }
  }

  //
  // If partition clause does not appear, the partitionArray_
  // is still empty; constructs the primary partition node and
  // then inserts its pointer to the array.
  //

  if (partitionArray_.entries() EQU 0)
  {
    pPrimaryPartition_ = new(PARSERHEAP()) ElemDDLPartitionRange();
    partitionArray_.insert(pPrimaryPartition_);
  }

  //
  // The information specified in the location clause and
  // load option clauses (e.g., dslack and islack) is for
  // the primary partition, copy it to the primary partition
  // node.
  //

  // the pRangePart is empty if first key option is ommitted.
  // Shall we assert the pRangePart NEQ NULL ? Internal error
  // will occur if the user does not specified the first key 
  // in the create index with partition clause.

  // Note that all of the following methods are defined on the ElemDDLPartitionSystem object.
  ElemDDLPartitionSystem * pSystemPart =
    pPrimaryPartition_->castToElemDDLPartitionSystem();

  // ComASSERT(pSystemPart NEQ NULL);

  //
  // location
  //

  if (isLocationSpecified())
  {
    pSystemPart->setLocationName(getLocationName());
    pSystemPart->setLocationNameType(getLocationNameType());
	pSystemPart->setPartitionName(getPartitionName());
  }

  //
  // file attributes
  //

  const ParDDLFileAttrsCreateIndex & fileAttrs = getFileAttributes();
  if (fileAttrs.isMaxSizeSpecified())
  {
    pSystemPart->setIsMaxSizeSpecified(fileAttrs.isMaxSizeSpecified());
    pSystemPart->setIsMaxSizeUnbounded(fileAttrs.isMaxSizeUnbounded());
    pSystemPart->setMaxSize           (fileAttrs.getMaxSize());
    pSystemPart->setMaxSizeUnit       (fileAttrs.getMaxSizeUnit());
  }

  if (fileAttrs.isExtentSpecified())
  {
    pSystemPart->setIsExtentSpecified(fileAttrs.isExtentSpecified());
    pSystemPart->setPriExt           (fileAttrs.getPriExt());
    pSystemPart->setSecExt           (fileAttrs.getSecExt());
  }

  if (fileAttrs.isMaxExtentSpecified())
  {
    pSystemPart->setIsMaxExtentSpecified(fileAttrs.isMaxExtentSpecified());
    pSystemPart->setMaxExt           (fileAttrs.getMaxExt());
  }

  //
  // Check to see if the index is a range partitioned index.
  // If it is then the FIRST-KEY option is required.
  // 

  if (partitionArray_.entries() > 1 ) // PARTITION clause specified
    {
      for (CollIndex i = 0; i < getPartitionArray().entries(); i++)
        if (getPartitionArray()[i]->castToElemDDLPartitionRange() EQU NULL)  
	  { // FIRST KEY phrase(s) not specified in PARTITION clause
            if  (partitioningScheme_ NEQ COM_HASH_V1_PARTITIONING &&  // Range partition requires FIRST KEY
                 partitioningScheme_ NEQ COM_HASH_V2_PARTITIONING)
            {
              *SqlParser_Diags << DgSqlCode(-3139);
              break;
            }
          }
        else
          {
           // No FIRST KEY allowed with HASH PARTITION
           if  (partitioningScheme_ EQU COM_HASH_V1_PARTITIONING ||
                partitioningScheme_ EQU COM_HASH_V2_PARTITIONING)
            {
              *SqlParser_Diags << DgSqlCode(-3153);
              break;
            }
          }
    } 

} // StmtDDLCreateIndex::synthesize()

//
// methods for tracing
//

const NAString
StmtDDLCreateIndex::displayLabel1() const
{
  return NAString("Index name: ") + getIndexName();
}

const NAString
StmtDDLCreateIndex::displayLabel2() const
{
  return NAString("Table name: ") + getTableName();
}

NATraceList
StmtDDLCreateIndex::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "Index name: ";
  detailText += getIndexName();
  detailTextList.append(detailText);

  detailText = "Table name: ";
  detailText += getTableName();
  detailTextList.append(detailText);

  CollIndex i, nbrCols;
  const ElemDDLColRefArray & colsList = getColRefArray();
  ElemDDLColRef * col;

  nbrCols = colsList.entries();

  if (nbrCols EQU 0)
  {
    detailTextList.append("No columns.");
  }
  else
  {
    detailText = "Column list [";
    detailText += LongToNAString((Lng32)nbrCols);
    detailText += " column(s)]:";
    detailTextList.append(detailText);

    for (i = 0; i < nbrCols; i++)
    {
      col = colsList[i];

      detailText = "[column ";
      detailText += LongToNAString((Lng32) i);
      detailText += "]";
      detailTextList.append(detailText);

      detailTextList.append("    ", col->getDetailInfo());
    }
  } // else (nbrCols EQU 0) of column references

  //
  // miscellaneous options
  //

  detailText = "is unique?         ";
  detailText += YesNo(isUniqueSpecified());
  detailTextList.append(detailText);

  //
  // file attributes
  //

  detailTextList.append("File attributes: ");

  ParDDLFileAttrsCreateIndex fileAttribs = getFileAttributes();
  detailTextList.append("    ", fileAttribs.getDetailInfo());

  //
  // load options
  //

  detailTextList.append("Load options: ");

  detailText = "    par exec spec? ";
  detailText += YesNo(isParallelExecutionClauseSpecified());
  detailTextList.append(detailText);

  detailText = "    par exec on?   ";
  detailText += YesNo(isParallelExecutionEnabled());
  detailTextList.append(detailText);

  if (isParallelExecutionEnabled())
  {
    detailText = "    config file:   ";
    detailText += getParallelExecConfigFileName();
    detailTextList.append(detailText);
  }

  //
  // partition information
  //

  CollIndex nbrParts;
  const ElemDDLPartitionArray & partsList = getPartitionArray();
  ElemDDLPartitionSystem * part;

  nbrParts = partsList.entries();
  if (nbrParts EQU 0)
  {
    detailTextList.append("No partitions.");
  }
  else
  {
    detailText = "Partition list [";
    detailText += LongToNAString((Lng32)nbrParts);
    detailText += " partition(s)]: ";
    detailTextList.append(detailText);

    for (i = 0; i < nbrParts; i++)
    {
      part = partsList[i]->castToElemDDLPartitionSystem();

      if (part EQU NULL)
      {
        //
        // Note that class ElemDDLPartitionRange is
        // derived from class ElemDDLPartitionSystem
        //
        // Only support range and system partitioning.
        *SqlParser_Diags << DgSqlCode(-3105) ;

        NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
      }

      detailText = "[partition ";
      detailText += LongToNAString((Lng32)i);
      detailText += "]";
      detailTextList.append(detailText);

      detailTextList.append("    ", part->getDetailInfo());
    } //for (j = 0; j < nbrParts; j++)
  } // else (nbrParts NEQ 0)

  return detailTextList;

} // StmtDDLCreateIndex::getDetailInfo()

const NAString
StmtDDLCreateIndex::getText() const
{
  return "StmtDDLCreateIndex";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLPopulateIndex
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLPopulateIndex::StmtDDLPopulateIndex(NABoolean populateAll,
                                           NABoolean populateAllUnique,
                                           const NAString & anIndexName,
                                           const QualifiedName & aTableName,
                                           CollHeap    * heap)
        : StmtDDLNode(DDL_POPULATE_INDEX),
          populateAll_(populateAll),
          populateAllUnique_(populateAllUnique),
          indexName_(anIndexName, heap),
          indexQualName_(heap),
          origTableQualName_(heap),
          tableQualName_(aTableName, heap)
{
}

//
// virtual destructor
//
StmtDDLPopulateIndex::~StmtDDLPopulateIndex()
{
}

//
// cast virtual function
//
StmtDDLPopulateIndex *
StmtDDLPopulateIndex::castToStmtDDLPopulateIndex()
{
  return this;
}

//
// accessors
//

Int32
StmtDDLPopulateIndex::getArity() const
{
  return 0; 
}

ExprNode *
StmtDDLPopulateIndex::getChild(Lng32 index)
{
  return NULL;
}

const NAString
StmtDDLPopulateIndex::getTableName() const
{
  return tableQualName_.getQualifiedNameAsAnsiString();
}

//
// collects information in the parse sub-tree and copy/move them
// to the current parse node.
//
void
StmtDDLPopulateIndex::synthesize()
{
  indexQualName_ = QualifiedName(getIndexName(),
				 tableQualName_.getSchemaName(),
				 tableQualName_.getCatalogName(),
				 PARSERHEAP());

} // StmtDDLPopulateIndex::synthesize()

//
// methods for tracing
//

const NAString
StmtDDLPopulateIndex::displayLabel1() const
{
  return NAString("Index name: ") + getIndexName();
}

const NAString
StmtDDLPopulateIndex::displayLabel2() const
{
  return NAString("Table name: ") + getTableName();
}

const NAString
StmtDDLPopulateIndex::getText() const
{
  return "StmtDDLPopulateIndex";
}

// -----------------------------------------------------------------------
// Methods for class StmtDDLCreateLibrary
// -----------------------------------------------------------------------

//
// Constructor
//

StmtDDLCreateLibrary::StmtDDLCreateLibrary(
   NABoolean             isSystem,
   const QualifiedName & libraryName,
   const NAString      & libraryFilename,
   ElemDDLNode         * clientName,
   ElemDDLNode         * clientFilename,
   ElemDDLNode         * pOwner,
   CollHeap            * heap)  
  : StmtDDLNode(DDL_CREATE_LIBRARY),
    isSystem_(isSystem),
    libraryName_(libraryName,heap),
    fileName_(libraryFilename),
    clientName_("",heap),
    clientFilename_("",heap),
    pOwner_(NULL)
    
{

ElemDDLLibClientName *clientNameNode = NULL;
ElemDDLLibClientFilename *clientFilenameNode = NULL;

   if (clientName != NULL)
   {
      clientNameNode = clientName->castToElemDDLLibClientName();

      if (clientNameNode != NULL)
         clientName_ = clientNameNode->getClientName();
   }
   
   if (clientFilename != NULL)
   {      
      clientFilenameNode = clientFilename->castToElemDDLLibClientFilename();


      if (clientFilenameNode != NULL)
         clientFilename_ = clientFilenameNode->getFilename();
   }
   
  if (pOwner)
  {
    pOwner_ = pOwner->castToElemDDLGrantee();
    ComASSERT(pOwner_ NEQ NULL);
  }
  else
    pOwner_ = NULL;

}

//
// Virtual Destructor
// garbage collection is being done automatically by the NAString Class
//

StmtDDLCreateLibrary::~StmtDDLCreateLibrary()
{
  if (pOwner_)
    delete pOwner_;
}

//
// cast
//

StmtDDLCreateLibrary *
StmtDDLCreateLibrary::castToStmtDDLCreateLibrary()
{
   return this;
}

//
// mutators
//

//
// collects information in the parse sub-tree and copy/move them
// to the current parse node.
//
void
StmtDDLCreateLibrary::synthesize()
{
}

//
// for tracing
//

const NAString
StmtDDLCreateLibrary::displayLabel1() const
{
   return NAString("Library name: " ) + getLibraryName();
}

const NAString
StmtDDLCreateLibrary::displayLabel2() const
{
   return NAString("Library filename: ") + getLibraryName();  
}

const NAString
StmtDDLCreateLibrary::getText() const
{
   return "StmtDDLCreateLibrary";
}




// -----------------------------------------------------------------------
// methods for class StmtDDLCreateRoutine
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLCreateRoutine::StmtDDLCreateRoutine(const QualifiedName & aRoutineName,
                                           const QualifiedName & anActionName,
                                           ElemDDLNode         * pParamList,
                                           ElemDDLNode         * pReturnsList,
                                           ElemDDLNode         * pPassThroughParamList,
                                           ElemDDLNode         * pOptionList,
                                           ComRoutineType        rType,
                                           CollHeap            * heap)
        : StmtDDLNode(DDL_CREATE_ROUTINE),
          routineQualName_(aRoutineName, heap),
          actionQualName_(anActionName, heap),
          routineType_(rType),
          isLocationClauseSpec_(FALSE),
          locationName_(heap),
          locationNameType_(ElemDDLLocation::LOCATION_DEFAULT_NAME_TYPE),
          guardianLocation_(heap),
          languageType_(COM_UNKNOWN_ROUTINE_LANGUAGE),
          languageTypeSpecified_(FALSE),
          deterministic_(FALSE),
          deterministicSpecified_(FALSE),
          sqlAccess_(COM_CONTAINS_SQL),
          sqlAccessSpecified_(FALSE),
          callOnNull_(TRUE),
          callOnNullSpecified_(FALSE),
          isolate_(TRUE),
          isolateSpecified_(FALSE),
          paramStyle_(COM_STYLE_GENERAL),
          paramStyleSpecified_(FALSE),
          transactionAttributes_(COM_TRANSACTION_REQUIRED),
          transactionAttributesSpecified_(FALSE),
          finalCall_(FALSE),
          finalCallSpecified_(FALSE),
          parallelism_(COM_ROUTINE_NO_PARALLELISM),
          parallelismSpecified_(FALSE),
          maxResults_(0),
          maxResultSetsSpecified_(FALSE),
          stateAreaSize_(0),
          stateAreaSizeSpecified_(FALSE),
          udrAttributes_("", heap),
          udrAttributesSpecified_(FALSE),
          libraryName_("", heap),
          libraryNameSpecified_(FALSE),
          externalPath_("", heap),
          externalPathSpecified_(FALSE),
          externalFile_("", heap),
          externalName_("", heap),
          externalNameSpecified_(FALSE),
          pOwner_(NULL),
          javaClassName_("", heap),
          javaMethodName_("", heap),
          javaSignature_("", heap),
          paramStyleVersion_(1),
          paramStyleVersionSpecified_(FALSE),
          initialCpuCost_(-1),
          initialCpuCostSpecified_(FALSE),
          initialIoCost_(-1),
          initialIoCostSpecified_(FALSE),
          initialMsgCost_(-1),
          initialMsgCostSpecified_(FALSE),
          normalCpuCost_(-1),
          normalCpuCostSpecified_(FALSE),
          normalIoCost_(-1),
          normalIoCostSpecified_(FALSE),
          normalMsgCost_(-1),
          normalMsgCostSpecified_(FALSE),
          isUniversal_(FALSE),
          isUniversalSpecified_(FALSE),
          canCollapse_(FALSE),
          canCollapseSpecified_(FALSE),
          userVersion_("", heap),
          userVersionSpecified_(FALSE),
          externalSecurity_(COM_ROUTINE_EXTERNAL_SECURITY_INVOKER),
          externalSecuritySpecified_(FALSE),
          actionPosition_(-1),
          actionPositionSpecified_(FALSE),
          executionMode_(COM_ROUTINE_SAFE_EXECUTION),
          executionModeSpecified_(FALSE),
          uudfParamKindList_(heap),
          uudfParamKindListSpecified_(FALSE),
          uniqueOutputValues_(heap),
          numberOfUniqueOutputValuesSpecified_(FALSE),
          specialAttributesText_("", heap),
          specialAttributesSpecified_(FALSE),
          firstReturnedParamPosWithinParamArray_(-1)
{
  setChild(INDEX_ROUTINE_PARAM_LIST, pParamList);
  setChild(INDEX_ROUTINE_RETURNS_LIST, pReturnsList);
  setChild(INDEX_ROUTINE_PASSTHROUGH_LIST, pPassThroughParamList);
  setChild(INDEX_ROUTINE_OPTION_LIST, pOptionList);
  setChild(INDEX_ROUTINE_UUDF_PARAM_KIND_LIST, NULL);

  if (routineType_ NEQ COM_PROCEDURE_TYPE) // Different default values for UDF
  {
    paramStyleVersion_ = 1;
    deterministic_ = FALSE;
    sqlAccess_ = COM_NO_SQL;
    transactionAttributes_ = COM_UNKNOWN_ROUTINE_TRANSACTION_ATTRIBUTE;
    finalCall_ = TRUE;
    parallelism_ = COM_ROUTINE_ANY_PARALLELISM;
    stateAreaSize_ = 0; // NO STATE AREA
  }
}

//
// virtual destructor
//
StmtDDLCreateRoutine::~StmtDDLCreateRoutine()
{
  // delete all children
  for (Int32 i = 0; i < getArity(); i++)
  {
    delete getChild(i);
  }

  if (pOwner_)
    delete pOwner_;
}

//
// cast virtual function
//
StmtDDLCreateRoutine *
StmtDDLCreateRoutine::castToStmtDDLCreateRoutine()
{
  return this;
}

//
// accessors
//

Int32
StmtDDLCreateRoutine::getArity() const
{
  return MAX_STMT_DDL_CREATE_ROUTINE_ARITY;
}

ExprNode *
StmtDDLCreateRoutine::getChild(Lng32 index)
{
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

//
// mutators
//

void
StmtDDLCreateRoutine::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());
  if (pChildNode EQU NULL)
  {
    children_[index] = NULL;
  }
  else
  {
    children_[index] = pChildNode->castToElemDDLNode();
  }
}


//
// collects information in the parse sub-tree and copy/move them
// to the current parse node.
//
void
StmtDDLCreateRoutine::synthesize()
{
  switch (getRoutineType())
  {
  case COM_PROCEDURE_TYPE:
    routineQualName_.setObjectNameSpace(COM_TABLE_NAME);
    break;
  case COM_ACTION_UDF_TYPE:
    routineQualName_.setObjectNameSpace(COM_UDF_NAME);
    actionQualName_.setObjectNameSpace(COM_UUDF_ACTION_NAME);
    break;
  case COM_TABLE_UDF_TYPE:
    routineQualName_.setObjectNameSpace(COM_TABLE_NAME);
    break;
  default:
    //
    // Note that getRoutineType() returns COM_UNKNOWN_ROUTINE_TYPE for the
    // ALTER FUNCTION statement case.  The catman/CatExecAlterRoutine layer
    // will look up the information from the metadata tables to determine
    // whether the function is a scalar and universal function.
    //
    routineQualName_.setObjectNameSpace(COM_UDF_NAME);
    break;
  }

  if (getChild(INDEX_ROUTINE_PARAM_LIST) NEQ NULL)
  {
    if (getRoutineType() EQU COM_UNIVERSAL_UDF_TYPE)
    {
      *SqlParser_Diags << DgSqlCode(-3260);
      // Do not have to exit/return here.
      // Continue so we can do as much checking as possible.
    }
    else
    {
      ElemDDLNode *pParamList =
        getChild(INDEX_ROUTINE_PARAM_LIST)->castToElemDDLNode();

      CollIndex i0;
      CollIndex nbrParamList = pParamList->entries();
      ElemDDLParamDef * pParamDef0 = NULL;
      for (i0 = 0; i0 < nbrParamList; i0++)
      {
        pParamDef0 = (*pParamList)[i0]->castToElemDDLParamDef();
        ComASSERT(pParamDef0 NEQ NULL);
        if ((getRoutineType() == COM_PROCEDURE_TYPE) AND
            (pParamDef0->getParamDataType()->getTypeQualifier() == NA_NUMERIC_TYPE))
        {
          if (((NumericType *)pParamDef0->getParamDataType())->isUnsigned())
          {
            *SqlParser_Diags << DgSqlCode(-3208);
            return;
          }
        }
        if ((getRoutineType() == COM_SCALAR_UDF_TYPE) AND
            ((pParamDef0->getParamDirection() == COM_OUTPUT_PARAM) OR
             (pParamDef0->getParamDirection() == COM_INOUT_PARAM)))
        {
          *SqlParser_Diags << DgSqlCode(-15001);
          return;
        }
        paramArray_.insert(pParamDef0);
      } // for
    } // getRoutineType() NEQ COM_UNIVERSAL_UDF_TYPE
  }  // end ROUTINE_PARAM_LIST not null

  if (getChild(INDEX_ROUTINE_RETURNS_LIST) NEQ NULL)
  {
    if (getRoutineType() == COM_PROCEDURE_TYPE)
    {
      *SqlParser_Diags << DgSqlCode(-3261);
      // Do not have to exit/return here.
      // Continue so we can do as much checking as possible.
    }
    else // NOT an SPJ
    {
      // Note that we append the returned parameters - specified in the
      // RETURN[S] clause - to the end of the formal parameter list.
      // Keep track of the position where the RETURNed formal parameter
      // list starts to mark the boundary.
      firstReturnedParamPosWithinParamArray_ = paramArray_.entries();

      ElemDDLNode *pReturnsList =
        getChild(INDEX_ROUTINE_RETURNS_LIST)->castToElemDDLNode();

      NABoolean isOnlyOutParamModeSupportedMsgAlreadyIssued2 = FALSE;
      CollIndex i2;
      CollIndex nbrReturnsList = pReturnsList->entries();
      ElemDDLParamDef* pParamDef2 = NULL;
      for (i2 = 0; i2 < nbrReturnsList; i2++)
      {
        pParamDef2 = (*pReturnsList)[i2]->castToElemDDLParamDef();
        ComASSERT(pParamDef2 NEQ NULL);

        paramArray_.insert(pParamDef2);
        if (NOT isOnlyOutParamModeSupportedMsgAlreadyIssued2 AND
            pParamDef2->getParamDirection() NEQ COM_OUTPUT_PARAM)
        {
          *SqlParser_Diags << DgSqlCode(-3262);
          isOnlyOutParamModeSupportedMsgAlreadyIssued2 = TRUE;
        }
      } // for (i2 = 0; i2 < nbrReturnsList; i2++)
    } // NOT an SPJ
  }  // end INDEX_ROUTINE_RETURNS_LIST not null

  if (getChild(INDEX_ROUTINE_PASSTHROUGH_LIST) NEQ NULL)
  {
    if (getRoutineType() == COM_PROCEDURE_TYPE)
    {
      *SqlParser_Diags << DgSqlCode(-3263);
      return;
    }
    ElemDDLNode *pPassThroughList =
      getChild(INDEX_ROUTINE_PASSTHROUGH_LIST)->castToElemDDLNode();

    CollIndex i4 = 0;
    CollIndex nbrPassThroughList = pPassThroughList->entries();
    ElemDDLPassThroughParamDef * passThroughDef = NULL;
    for (i4 = 0; i4 < nbrPassThroughList; i4++)
    {
      passThroughDef = (*pPassThroughList)[i4]->castToElemDDLPassThroughParamDef();
      ComASSERT(passThroughDef NEQ NULL);
      passThroughParamArray_.insert(passThroughDef); // [ ADD ] PASS THROUGH INPUTS
    } // for (i4 = 0; i4 < nbrPassThroughList; i4++)

  }  // end INDEX_ROUTINE_PASSTHROUGH_LIST not null

  if (getChild(INDEX_ROUTINE_OPTION_LIST) NEQ NULL)
  {
    ElemDDLNode *pOptionList =
      getChild(INDEX_ROUTINE_OPTION_LIST)->castToElemDDLNode();
    ComASSERT(pOptionList NEQ NULL);
    if (pOptionList->castToElemDDLOptionList() NEQ NULL)
    {
      for (CollIndex i6 = 0; i6 < pOptionList->entries(); i6++)
      {
        setRoutineOption((*pOptionList)[i6]);
      }
    }
    else
    {
      //
      // pOptionList points to a single Create Routine option
      //
      setRoutineOption(pOptionList);
    }
  } // if (pOptionList NEQ NULL)

  //
  // More semantic checks
  //

  if ( NOT isStmtDDLAlterRoutineParseNode() AND
       NOT externalNameSpecified_ AND getRoutineType() NEQ COM_ACTION_UDF_TYPE )
  {
     *SqlParser_Diags << DgSqlCode(-3205);
     if (getRoutineType() == COM_PROCEDURE_TYPE) // keep the old behavior to avoid regression
       return;
  }

  // ---------------------------------------------------------------------
  // SPJ
  // ---------------------------------------------------------------------

  if (getRoutineType() == COM_PROCEDURE_TYPE)
  {

    if ( ! externalPathSpecified_ && ! libraryNameSpecified_)
    {
      *SqlParser_Diags << DgSqlCode(-3201);
      return;
    }
    if ( ! languageTypeSpecified_ )
    {
      *SqlParser_Diags << DgSqlCode(-3203);
      return;
    }

    if (stateAreaSizeSpecified_ || parallelismSpecified_ || finalCallSpecified_)
    {
      *SqlParser_Diags << DgSqlCode(-15001);
      return;
    }

    if ( (maxResults_ < 0) || ( maxResults_ > 255) )
    {
      *SqlParser_Diags << DgSqlCode(-3219);
      return;
    }

    // For LANGUAGE JAVA case,
    // parse the specified External Name, which is of the form:
    //   <class-name>.<method-name> [ <signature> ]

    // Trim any leading and trailing blanks
    externalName_ = externalName_.strip(NAString::both);

    size_t namSize = externalName_.length() - 1;
    if ( namSize < 2 )
    {
      *SqlParser_Diags << DgSqlCode(-3204);
      return;
    }

    size_t pos = externalName_.first('(');  // Find first left paren
    if ( pos NEQ NA_NPOS )
    {
      // Found left paren.  So a signature was specified.
      javaSignature_ = externalName_(pos, (externalName_.length()-pos));
      externalName_.remove (pos);
      externalName_ = externalName_.strip(); // Strip trailing blanks

      namSize = externalName_.length() - 1;  // Remaining name
      if ( namSize < 2 )
      {
        *SqlParser_Diags << DgSqlCode(-3204);
        return;
      }
    }

    // Find the method-name portion.
    if ((pos=externalName_.last('.')) EQU NA_NPOS)
    {
      *SqlParser_Diags << DgSqlCode(-3204); // Badly formed
      return;
    }
    else
    {
      javaMethodName_ = externalName_(pos+1, (externalName_.length()-pos-1));
      javaClassName_ = externalName_.remove (pos);
    };

    if ( ( javaMethodName_.length() < 1 ) ||
         ( javaClassName_.length() < 1 ) )
    {
      *SqlParser_Diags << DgSqlCode(-3204); // Badly formed
      return;
    }

  } // end if (getRoutineType() == COM_PROCEDURE_TYPE)

  // ---------------------------------------------------------------------
  // UDF or (routine) action
  // ---------------------------------------------------------------------

  if (getRoutineType() NEQ COM_PROCEDURE_TYPE) // is a function or routine action
  {
    // Note that getRoutineType() returns COM_UNKNOWN_ROUTINE_TYPE for the
    // ALTER FUNCTION statement case.  The catman/CatExecAlterRoutine layer
    // will look up the information from the metadata tables to determine
    // whether the function is a scalar and universal function.

    NABoolean isErrorFound = FALSE;

    if (NOT isStmtDDLAlterRoutineParseNode() // registration of a routine
        AND getRoutineType() EQU COM_UNIVERSAL_UDF_TYPE)
    {
      ElemDDLNode *pUudfParamKindListParseTree =
        getChild(INDEX_ROUTINE_UUDF_PARAM_KIND_LIST)->castToElemDDLNode();

      if (pUudfParamKindListParseTree NEQ NULL)
      {
        for (CollIndex i7 = 0; i7 < pUudfParamKindListParseTree->entries(); i7++)
        {
          ElemDDLUudfParamDef *pUudfParamKind =
            (*pUudfParamKindListParseTree)[i7]->castToElemDDLUudfParamDef();
          ComUudfParamKind uudfParamKind = pUudfParamKind->getUudfParamKind();
          switch (uudfParamKind)
          {
          case COM_UUDF_PARAM_OMITTED:
            break;
          case COM_UUDF_PARAM_ACTION:
            if (actionPositionSpecified_)
            {
              if (NOT isErrorFound)
              {
                // 3183 ZZZZZ 99999 BEGINNER MAJOR DBADMIN Duplicate $0~string0 clauses were specified.
                // *** ERROR[3183] Duplicate ACTION clauses were specified.
                *SqlParser_Diags << DgSqlCode(-3183)
                                 << DgString0("ACTION");
                isErrorFound = TRUE;
              }
            }
            actionPosition_ = i7 + 1;
            actionPositionSpecified_ = TRUE;
            break;
          default:
            {
              for (CollIndex i9 = 0; i9 < uudfParamKindList_.entries(); i9++)
              {
                if (NOT isErrorFound AND
                    uudfParamKindList_[i9] EQU uudfParamKind)
                {
                  NAString paramKindName;
                  ComGetUudfParamKindAsLit ( uudfParamKind  // in
                                           , paramKindName  // out
                                           );
                  // 3183 ZZZZZ 99999 BEGINNER MAJOR DBADMIN Duplicate $0~string0 clauses were specified.
                  // *** ERROR[3183] Duplicate <param kind> clauses were specified.
                  *SqlParser_Diags << DgSqlCode(-3183)
                                   << DgString0(paramKindName.data());
                  isErrorFound = TRUE;
                }
              }
              if (uudfParamKind EQU COM_UUDF_PARAM_SAS_FORMAT)
              {
                if (NOT isErrorFound AND
                    actionPositionSpecified_)
                {
                  *SqlParser_Diags << DgSqlCode(-3264);
                  isErrorFound = TRUE;
                }
                actionPosition_ = i7 + 1;
              }
            } // default
            break;
          } // switch
          uudfParamKindList_.insert(pUudfParamKind->getUudfParamKind());
        } // for (CollIndex i7 = 0; ... )

        if (NOT isErrorFound)
        {
          // Manufacture and insert new entries to the (empty) formal
          // parameter definition list for the universal fucnction.

          ComASSERT(paramArray_.entries() EQU 0);

          ElemDDLParamDef  *  pParamDef2 = NULL;
          ElemDDLParamName * pParamName2 = NULL;
          NAType *       pParamDataType2 = NULL;

          for (CollIndex i2 = 0; i2 < uudfParamKindList_.entries(); i2++)
          {
            switch (uudfParamKindList_[i2])
            {
            case COM_UUDF_PARAM_ACTION:
              pParamName2     = new(STMTHEAP) ElemDDLParamName("ACTION");
              pParamDataType2 = new(STMTHEAP) SQLVarChar (STMTHEAP, 1024  // long maxLength
                                                         , FALSE // NABoolean allowSQLnull
                                                         );
              break;
            case COM_UUDF_PARAM_SAS_FORMAT:
              pParamName2     = new(STMTHEAP) ElemDDLParamName("SAS_FORMAT");
              pParamDataType2 = new(STMTHEAP) SQLVarChar (STMTHEAP, 1024  // long maxLength
                                                         , FALSE // NABoolean allowSQLnull
                                                         );
              break;
            case COM_UUDF_PARAM_SAS_LOCALE:
              pParamName2     = new(STMTHEAP) ElemDDLParamName("SAS_LOCALE");
              pParamDataType2 = new(STMTHEAP) SQLInt (STMTHEAP,  TRUE  // NABoolean allowNegValues
                                                     , FALSE // NABoolean allowSQLnull
                                                     );
              break;
            case COM_UUDF_PARAM_SAS_MODEL_INPUT_TABLE:
              pParamName2     = new(STMTHEAP) ElemDDLParamName("SAS_MODEL_INPUT_TABLE");
              pParamDataType2 = new(STMTHEAP) SQLVarChar (STMTHEAP, 1024  // long maxLength
                                                         , FALSE // NABoolean allowSQLnull
                                                         );
              break;
            default:
            case COM_UUDF_PARAM_OMITTED:           // any dummy data type would do
              pParamName2     = NULL;
              pParamDataType2 = new(STMTHEAP) SQLChar (STMTHEAP, 1     // long maxLength
                                                      , FALSE // NABoolean allowSQLnull
                                                      );
              break;
            } // switch
            pParamDef2 = new(STMTHEAP) ElemDDLParamDef ( pParamDataType2
                                                       , pParamName2 // optional_param_name
                                                       , COM_INPUT_PARAM
                                                       , STMTHEAP
                                                       );
            paramArray_.insert(pParamDef2);
          } // for (CollIndex i2 = 0; ...
        } // if no error found
      } // if (pUudfParamKindListParseTree NEQ NULL)
    } // if is the registration of a universal function

    if (externalNameSpecified_)
    {
      // Trim any leading and trailing blanks
      externalName_ = externalName_.strip(NAString::both);
      if (externalName_.isNull())
      {
        *SqlParser_Diags << DgSqlCode(-3204); // Badly formed
        isErrorFound = TRUE;
      }
    }
    if (externalPathSpecified_ AND getRoutineType() EQU COM_ACTION_UDF_TYPE)
    {
      *SqlParser_Diags << DgSqlCode(-3265);
      isErrorFound = TRUE;
    }
    if (isLocationClauseSpec_ AND ( isStmtDDLAlterRoutineParseNode() OR getRoutineType() EQU COM_ACTION_UDF_TYPE ))
    {
      *SqlParser_Diags << DgSqlCode(-3279);
      isErrorFound = TRUE;
    }
    if (isolateSpecified_)
    {
      *SqlParser_Diags << DgSqlCode(-3266);
      isErrorFound = TRUE;
    }
    if (transactionAttributesSpecified_)
    {
      *SqlParser_Diags << DgSqlCode(-3267);
      isErrorFound = TRUE;
    }
    if (maxResultSetsSpecified_)
    {
      *SqlParser_Diags << DgSqlCode(-3268);
      isErrorFound = TRUE;
    }
    if (languageTypeSpecified_)
    {
      if (isStmtDDLAlterRoutineParseNode() OR getRoutineType() EQU COM_ACTION_UDF_TYPE)
      {
        *SqlParser_Diags << DgSqlCode(-3277);
        isErrorFound = TRUE;
      }
      else if (getRoutineType() NEQ COM_TABLE_UDF_TYPE AND languageType_ EQU COM_LANGUAGE_JAVA)
      {
        *SqlParser_Diags << DgSqlCode(-3269);
        isErrorFound = TRUE;
      }
    }
    if (paramStyleSpecified_)
    {
      if (isStmtDDLAlterRoutineParseNode() OR getRoutineType() EQU COM_ACTION_UDF_TYPE  )
      {
        *SqlParser_Diags << DgSqlCode(-3278);
        isErrorFound = TRUE;
      }
      else
      {
        switch (paramStyle_)
        {
        case COM_STYLE_JAVA_CALL:
          *SqlParser_Diags << DgSqlCode(-3270);
          isErrorFound = TRUE;
          break;
        case COM_STYLE_SQL:
          if (getRoutineType() EQU COM_TABLE_UDF_TYPE OR
              getRoutineType() EQU COM_ACTION_UDF_TYPE OR
              getRoutineType() EQU COM_UNIVERSAL_UDF_TYPE)
          {
            *SqlParser_Diags << DgSqlCode(-3280);
            isErrorFound = TRUE;
          }
          break;
        case COM_STYLE_SQLROW:
          if (getRoutineType() EQU COM_SCALAR_UDF_TYPE)
          {
            *SqlParser_Diags << DgSqlCode(-3281);
            isErrorFound = TRUE;
          }
          break;
        default:
          break;
        } // switch
      }
    } // if (paramStyleSpecified_)

    if ( sqlAccessSpecified_ AND sqlAccess_ NEQ COM_NO_SQL)
    {
      *SqlParser_Diags << DgSqlCode(-3271);
      isErrorFound = TRUE;
    }

    if (getRoutineType() EQU COM_TABLE_UDF_TYPE &&
        (finalCallSpecified_ OR stateAreaSizeSpecified_))
    {
      *SqlParser_Diags << DgSqlCode(-3287);
      if (finalCallSpecified_)
        *SqlParser_Diags << DgString0("FINAL CALL");
      else
        *SqlParser_Diags << DgString0("STATE AREA SIZE");
      isErrorFound = TRUE;
    }

    if ( (stateAreaSize_ < 0) || ( stateAreaSize_ > 16000) )
    {
      *SqlParser_Diags << DgSqlCode(-3272);
      isErrorFound = TRUE;
    }

    if (finalCallSpecified_)
    {
      if (getRoutineType() EQU COM_ACTION_UDF_TYPE)
      {
        *SqlParser_Diags << DgSqlCode(-3273);
        isErrorFound = TRUE;
      }
      else if (finalCall_ EQU FALSE)
      {
        *SqlParser_Diags << DgSqlCode(-3274);
        isErrorFound = TRUE;
      }
    }

    if (numberOfUniqueOutputValuesSpecified_ AND
        NOT isStmtDDLAlterRoutineParseNode()) // registration of a routine
    {
      if (NOT isReturnClauseSpecified() AND uniqueOutputValues_.entries() > 0)
      {
        *SqlParser_Diags << DgSqlCode(-3275);
        isErrorFound = TRUE;
      }
      if (getRoutineType() NEQ COM_UNIVERSAL_UDF_TYPE)
      {
        size_t numberOfOutputParams = getParamArray().entries()
          - getFirstReturnedParamPosWithinParamArray();
        if (numberOfOutputParams < uniqueOutputValues_.entries())
        {
          *SqlParser_Diags << DgSqlCode(-3276);
          isErrorFound = TRUE;
        }
      }
    } // if (numberOfUniqueOutputValuesSpecified_ AND registration of a routine)

    if (isErrorFound)
      return; // play it safe

  } // if (getRoutineType() NEQ COM_PROCEDURE_TYPE)

} // StmtDDLCreateRoutine::synthesize()


void
StmtDDLCreateRoutine::setRoutineOption(ElemDDLNode * pRoutineOption)
{
  ComASSERT(pRoutineOption NEQ NULL);

  switch (pRoutineOption->getOperatorType())
  {

  case ELM_LOCATION_ELEM :
    {
      if (isLocationClauseSpec_)
      {
        // Duplicate LOCATION clauses.
        *SqlParser_Diags << DgSqlCode(-3098);
      }

    isLocationClauseSpec_ = TRUE;

    ElemDDLLocation * pLocation = pRoutineOption->castToElemDDLLocation();
    if (NOT pLocation->getPartitionName().isNull())
        *SqlParser_Diags << DgSqlCode(-3405);
    locationName_     = pLocation->getLocationName();
    locationNameType_ = pLocation->getLocationNameType();
    }
    break;

  case ELM_UDR_DETERMINISTIC :
    {
      if (deterministicSpecified_)
      {
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("[NOT] DETERMINISTIC");
      }

      ElemDDLUdrDeterministic * pDeterministic = pRoutineOption->
                                           castToElemDDLUdrDeterministic();
      deterministic_ = pDeterministic->getDeterministic();
      deterministicSpecified_ = TRUE;
    }
    break;

  case ELM_UDR_EXTERNAL_NAME :
    {
      if (externalNameSpecified_)
      {
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("EXTERNAL NAME");
      }

      ElemDDLUdrExternalName * pExternalName = pRoutineOption->
                                         castToElemDDLUdrExternalName();
      externalName_ = pExternalName->getExternalName();
      externalNameSpecified_ = TRUE;
    }
    break;

  case ELM_UDR_EXTERNAL_PATH :
    {
      if (externalPathSpecified_)
      {
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("EXTERNAL PATH");
      }
      if (libraryNameSpecified_)
      {
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("LIBRARY");
      }

      ElemDDLUdrExternalPath * pExternalPath = pRoutineOption->
                                         castToElemDDLUdrExternalPath();
      externalPath_ = pExternalPath->getExternalPath();
      externalPathSpecified_ = TRUE;
    }
    break;

  case ELM_UDR_ISOLATE :
    {
      if (isolateSpecified_)
      {
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("[NO] ISOLATE");
      }

      ElemDDLUdrIsolate * pIsolate = pRoutineOption->castToElemDDLUdrIsolate();
      isolate_ = pIsolate->getIsolate();
      isolateSpecified_ = TRUE;
    }
    break;

  case ELM_UDR_LANGUAGE :
    {
      if (languageTypeSpecified_)
      {
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("LANGUAGE");
      }

      ElemDDLUdrLanguage * pLanguage = pRoutineOption->
                                 castToElemDDLUdrLanguage();
      languageType_ = pLanguage->getLanguage();
      languageTypeSpecified_ = TRUE;
    }
    break;

  case ELM_UDR_LIBRARY :
    {
      if (libraryNameSpecified_)
      {
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("LIBRARY");
      }
      if (externalPathSpecified_)
      {
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("EXTERNAL PATH");
      }

      ElemDDLUdrLibrary * pLibrary = pRoutineOption->
                                         castToElemDDLUdrLibrary();
                                         
      BindWA *pBindWA = new(STMTHEAP) BindWA(ActiveSchemaDB(), 
                                             CmpCommon::context(), 
                                             TRUE/*inDDL*/);
      pLibrary->bindNode(pBindWA);
      
      libraryName_ = pLibrary->getLibraryName();
      libraryNameSpecified_ = TRUE;
      delete pBindWA;
    }
    break;

  case ELM_UDR_MAX_RESULTS :
    {
      if (maxResultSetsSpecified_)
      {
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("DYNAMIC RESULT SETS");
      }

      ElemDDLUdrMaxResults * pMaxResults = pRoutineOption->
                                     castToElemDDLUdrMaxResults();
      maxResults_ = (ComSInt32)pMaxResults->getMaxResults();
      maxResultSetsSpecified_ = TRUE;
    }
    break;

  case ELM_UDR_PARAM_STYLE :
    {
      if (paramStyleSpecified_)
      {
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("PARAMETER STYLE");
      }

      ElemDDLUdrParamStyle * pParamStyle = pRoutineOption->
                                     castToElemDDLUdrParamStyle();
      paramStyle_ = pParamStyle->getParamStyle();
      paramStyleSpecified_ = TRUE;
    }
    break;

  case ELM_UDR_SQL_ACCESS :
    {
      if (sqlAccessSpecified_)
      {
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("SQL access mode");
      }

      ElemDDLUdrSqlAccess * pSqlAccess = pRoutineOption->
                                   castToElemDDLUdrSqlAccess();
      sqlAccess_ = pSqlAccess->getSqlAccess();
      sqlAccessSpecified_ = TRUE;
    }
    break;

  case ELM_UDR_TRANSACTION_ATTRIBUTES :
    {
      if (transactionAttributesSpecified_)
      {
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("transaction attributes");
      }

      ElemDDLUdrTransaction * pTransactionAttributes = pRoutineOption->
                                              castToElemDDLUdrTransaction();
      transactionAttributes_ = pTransactionAttributes->
                                              getTransactionAttributes();
    }
    break;

  case ELM_UDR_EXTERNAL_SECURITY :
    {
      if (externalSecuritySpecified_)
      {
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("EXTERNAL SECURITY");
      }

      ElemDDLUdrExternalSecurity * pExternalSecurity = pRoutineOption->
                                         castToElemDDLUdrExternalSecurity();
      externalSecurity_ = pExternalSecurity->getExternalSecurity();
      externalSecuritySpecified_ = TRUE;
    }
    break;

  case ELM_UDF_EXECUTION_MODE :
    {
      if (executionModeSpecified_)
      {
        // 3183 ZZZZZ 99999 BEGINNER MAJOR DBADMIN Duplicate $0~string0 clauses were specified.
        // *** ERROR[3183] Duplicate EXECUTION MODE clauses were specified.
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("EXECUTION MODE");
      }

      ElemDDLUdfExecutionMode * pExecMode = pRoutineOption->castToElemDDLUdfExecutionMode();
      executionMode_ = pExecMode->getExecutionMode();
      executionModeSpecified_ = TRUE;
    }
    break;

  case ELM_UDF_FINAL_CALL :
    {
      if (finalCallSpecified_)
      {
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("[NO] FINAL CALL");
      }

      ElemDDLUdfFinalCall * pFinalCall = pRoutineOption->castToElemDDLUdfFinalCall();
      finalCall_ = pFinalCall->getFinalCall();
      finalCallSpecified_ = TRUE;
    }
    break;

  case ELM_UDF_PARALLELISM :
    {
      if (parallelismSpecified_)
      {
        // 3183 ZZZZZ 99999 BEGINNER MAJOR DBADMIN Duplicate $0~string0 clauses were specified.
        // *** ERROR[3183] Duplicate PARALLELISM clauses were specified.
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("PARALLELISM");
      }

      ElemDDLUdfParallelism * pParallelism = pRoutineOption->castToElemDDLUdfParallelism();
      parallelism_ = pParallelism->getParallelism();
      parallelismSpecified_ = TRUE;
    }
    break;

  case ELM_UDF_SPECIAL_ATTRIBUTES :
    {
      if (specialAttributesSpecified_)
      {
        // 3183 ZZZZZ 99999 BEGINNER MAJOR DBADMIN Duplicate $0~string0 clauses were specified.
        // *** ERROR[3183] Duplicate ATTRIBUTES clauses were specified.
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("ATTRIBUTES");
      }

      ElemDDLUdfSpecialAttributes * pUdfSpecialAttrs = pRoutineOption->
                                    castToElemDDLUdfSpecialAttributes();
      specialAttributesText_ = pUdfSpecialAttrs->getSpecialAttributesText();
      specialAttributesSpecified_ = TRUE;
    }
    break;

  case ELM_UDF_STATE_AREA_SIZE :
    {
      if (stateAreaSizeSpecified_)
      {
        // 3183 ZZZZZ 99999 BEGINNER MAJOR DBADMIN Duplicate $0~string0 clauses were specified.
        // *** ERROR[3183] Duplicate STATE AREA clauses were specified.
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("STATE AREA");
      }

      ElemDDLUdfStateAreaSize * pStateAreaSize = pRoutineOption->
                                     castToElemDDLUdfStateAreaSize();
      stateAreaSize_ = (ComSInt32)pStateAreaSize->getStateAreaSize();
      stateAreaSizeSpecified_ = TRUE;
    }
    break;

  case ELM_UDF_OPTIMIZATION_HINT :
    {
      ElemDDLUdfOptimizationHint * pOptHint = pRoutineOption->
        castToElemDDLUdfOptimizationHint();
      NAString udfAttrName;
      switch (pOptHint->getOptimizationKind())
      {
      case COM_UDF_INITIAL_CPU_COST :
        if (initialCpuCostSpecified_)
          udfAttrName = "INITIAL CPU COST";
        initialCpuCost_ = pOptHint->getCost();
        initialCpuCostSpecified_ = TRUE;
        break;
      case COM_UDF_INITIAL_IO_COST :
        if (initialIoCostSpecified_)
          udfAttrName = "INITIAL IO COST";
        initialIoCost_ = pOptHint->getCost();
        initialIoCostSpecified_ = TRUE;
        break;
      case COM_UDF_INITIAL_MESSAGE_COST :
        if (initialMsgCostSpecified_)
          udfAttrName = "INITIAL MESSAGE COST";
        initialMsgCost_ = pOptHint->getCost();
        initialMsgCostSpecified_ = TRUE;
        break;
      case COM_UDF_NORMAL_CPU_COST :
        if (normalCpuCostSpecified_)
          udfAttrName = "NORMAL CPU COST";
        normalCpuCost_ = pOptHint->getCost();
        normalCpuCostSpecified_ = TRUE;
        break;
      case COM_UDF_NORMAL_IO_COST :
        if (normalIoCostSpecified_)
          udfAttrName = "NORMAL IO COST";
        normalIoCost_ = pOptHint->getCost();
        normalIoCostSpecified_ = TRUE;
        break;
      case COM_UDF_NORMAL_MESSAGE_COST :
        if (normalMsgCostSpecified_)
          udfAttrName = "NORMAL MESSAGE COST";
        normalMsgCost_ = pOptHint->getCost();
        normalMsgCostSpecified_ = TRUE;
        break;
      case COM_UDF_NUMBER_OF_UNIQUE_OUTPUT_VALUES :
        if (numberOfUniqueOutputValuesSpecified_ )
          udfAttrName = "NUMBER OF UNIQUE OUTPUT VALUES";
        uniqueOutputValues_ = pOptHint->getUniqueOutputValues();
        numberOfUniqueOutputValuesSpecified_ = TRUE;
        break;
      default :
        {
          NAAbort(__FILE__, __LINE__, "internal logic error");
        }
        break;
      } // switch
      if (NOT udfAttrName.isNull())
      {
        // 3183 ZZZZZ 99999 BEGINNER MAJOR DBADMIN Duplicate $0~string0 clauses were specified.
        // *** ERROR[3183] Duplicate <attribute name> clauses were specified.
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0(udfAttrName.data());
      }
    }
    break;

  case ELM_UDF_VERSION_TAG :
    {
      if (userVersionSpecified_)
      {
        // 3183 ZZZZZ 99999 BEGINNER MAJOR DBADMIN Duplicate $0~string0 clauses were specified.
        // *** ERROR[3183] Duplicate VERSION TAG clauses were specified.
        *SqlParser_Diags << DgSqlCode(-3183)
                         << DgString0("VERSION TAG");
      }

      ElemDDLUdfVersionTag * pUdfVesionTag = pRoutineOption->castToElemDDLUdfVersionTag();
      userVersion_ = pUdfVesionTag->getVersionTag();
      userVersionSpecified_ = TRUE;
    }
    break;

  default :
    {
      NAAbort(__FILE__, __LINE__, "internal logic error");
    }
    break;

  } // switch

} // StmtDDLRoutineTable::setRoutineOption()

//
// methods for tracing
//

const NAString
StmtDDLCreateRoutine::displayLabel1() const
{
  return NAString("Routine name: ") + getRoutineName();
}

const NAString
StmtDDLCreateRoutine::displayLabel2() const
{
  return NAString("Table name: ") + getRoutineName();
}

NATraceList
StmtDDLCreateRoutine::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailText = "Routine name: ";
  detailText += getRoutineName();
  detailTextList.append(detailText);

  CollIndex i, nbrParams;
  const ElemDDLParamDefArray & paramsList = getParamArray();
  ElemDDLParamDef * param;

  nbrParams = paramsList.entries();

  if (nbrParams EQU 0)
  {
    detailTextList.append("No params.");
  }
  else
  {
    detailText = "Param list [";
    detailText += LongToNAString((Lng32)nbrParams);
    detailText += " param(s)]:";
    detailTextList.append(detailText);

    for (i = 0; i < nbrParams; i++)
    {
      param = paramsList[i];

      detailText = "[param ";
      detailText += LongToNAString((Lng32) i);
      detailText += "]";
      detailTextList.append(detailText);

      detailTextList.append("    ", param->getDetailInfo());
    }
  } // else (nbrParams EQU 0) of params

  return detailTextList;

} // StmtDDLCreateRoutine::getDetailInfo()

const NAString
StmtDDLCreateRoutine::getText() const
{
  return "StmtDDLCreateRoutine";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLCreateSchema
// -----------------------------------------------------------------------

//
// initialize constructor
//
StmtDDLCreateSchema::StmtDDLCreateSchema(
     const ElemDDLSchemaName & aSchemaNameParseNode,
     ComSchemaClass            schemaClass,
     CharType*                 pCharType,
     CollHeap                * heap)

: StmtDDLNode(DDL_CREATE_SCHEMA),
  schemaName_(heap),
  schemaQualName_(aSchemaNameParseNode.getSchemaName(), heap),
  authorizationID_(aSchemaNameParseNode.getAuthorizationID(), heap),
  schemaClass_(schemaClass),
  pCharType_(pCharType),
  createIfNotExists_(FALSE)
{
}

//
// virtual destructor
//
StmtDDLCreateSchema::~StmtDDLCreateSchema()
{
  delete pCharType_;
}

//
// cast virtual function
//
StmtDDLCreateSchema *
StmtDDLCreateSchema::castToStmtDDLCreateSchema()
{
  return this;
}

//
// accessors
//

//
// collects information in the parse sub-tree and copy/move them
// to the current parse node.
//
void
StmtDDLCreateSchema::synthesize()
{
  if (schemaQualName_.getCatalogName().isNull())
  {
    schemaName_ = ToAnsiIdentifier(schemaQualName_.getSchemaName());
  }
  else
  {
    schemaName_ = ToAnsiIdentifier(schemaQualName_.getCatalogName()) + "." +
      ToAnsiIdentifier(schemaQualName_.getSchemaName());
  }

  // If the schema name specified is reserved name, users cannot create them.
  // They can only be created internally.
  if ((! Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)) &&
      (ComIsTrafodionReservedSchemaName(schemaQualName_.getSchemaName())))
    {
      // error.
      *SqlParser_Diags << DgSqlCode(-1430)
                       << DgSchemaName(schemaName_);
      
    }

} // StmtDDLCreateSchema::synthesize()


//
// methods for tracing
//

const NAString
StmtDDLCreateSchema::displayLabel1() const
{
  return NAString("Schema name: ") + getSchemaName();
}

const NAString
StmtDDLCreateSchema::displayLabel2() const
{
  if (NOT getAuthorizationID().isNull())
  {
    return NAString("Authorization ID: ") + getAuthorizationID();
  }
  else
  {
    return NAString("Authorization identifier not specified.");
  }
}

const NAString
StmtDDLCreateSchema::getText() const
{
  return "StmtDDLCreateSchema";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLCreateSequence
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLCreateSequence::StmtDDLCreateSequence(const QualifiedName & seqQualName,
					     ElemDDLSGOptions * pSGOptions,
					     ComBoolean alter,
					     CollHeap    * heap)
  : StmtDDLNode(DDL_CREATE_SEQUENCE),
    seqQualName_(seqQualName, heap),
    pSGOptions_(pSGOptions),
    alter_(alter)
{

}

StmtDDLCreateSequence::~StmtDDLCreateSequence()
{
}
  
//
// cast virtual function
//
StmtDDLCreateSequence *
StmtDDLCreateSequence::castToStmtDDLCreateSequence()
{
  return this;
}

//
// accessors
//

Int32
StmtDDLCreateSequence::getArity() const
{
  return 0;
}

ExprNode *
StmtDDLCreateSequence::getChild(Lng32 index)
{
  return NULL;
}

//
// methods for tracing
//

const NAString
StmtDDLCreateSequence::displayLabel1() const
{
  return NAString("Sequence name: ") + seqQualName_.getQualifiedNameAsAnsiString();
}

NATraceList
StmtDDLCreateSequence::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  //
  // table name
  //

  detailTextList.append(displayLabel1());
  
  return detailTextList;
}

const NAString
StmtDDLCreateSequence::getText() const
{
  return "StmtDDLCreateSequence";
}

// method for collecting information
void StmtDDLCreateSequence::synthesize()
{
}


// -----------------------------------------------------------------------
// methods for class StmtDDLCreateTable
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLCreateTable::StmtDDLCreateTable(const QualifiedName & aTableQualName,
                                       ElemDDLNode * pTableDefBody,
                                       ElemDDLNode * pCreateTableAttrList,
				       ElemDDLNode * pInsertColumnsList,
				       RelExpr	   * queryExpression,
                                       CollHeap    * heap)
        : StmtDDLNode(DDL_CREATE_TABLE),
          origTableQualName_(heap),
          tableQualName_(aTableQualName, heap),
          isLocationClauseSpec_(FALSE),
          locationName_(heap),
          locationNameType_(ElemDDLLocation::LOCATION_DEFAULT_NAME_TYPE),
	  partitionName_(heap),
          isLikeClauseSpec_(FALSE),
          likeSourceTableCorrName_("", heap),
          guardianLocation_(heap),
          isPrimaryKeyClauseSpec_(FALSE),
          pAddConstraintPK_(NULL),
          isAttributeClauseSpec_(FALSE),
	  isMVFileAttributeClauseSpec_(FALSE),
          isDivisionByClauseSpec_(FALSE),
          pDivisionByClauseParseNode_(NULL),
	  isHbaseOptionsSpec_(FALSE),
	  pHbaseOptionsParseNode_(NULL),
          pSaltOptions_(NULL),
          isStoreByClauseSpec_(FALSE),
          isUniqueStoreByKeylist_(FALSE),
          isUniqueStoreByPrimaryKey_(FALSE),
          storeOption_(COM_UNKNOWN_STORE_OPTION),
          isRoundRobinPartitioningSpecified_(FALSE),
          isHashV1PartitionSpec_(FALSE),
          isHashV2PartitionSpec_(FALSE),
          isPartitionClauseSpec_(FALSE),
          isPartitionByClauseSpec_(FALSE),
          pPrimaryPartition_(NULL),
          columnDefArray_(heap),
          primaryKeyColRefArray_(heap),
          partitionArray_(heap),
          partitionKeyColRefArray_(heap),
          keyColRefArray_(heap),
          addConstraintCheckArray_(heap),
          addConstraintRIArray_(heap),
          addConstraintUniqueArray_(heap),
          addConstraintArray_(heap),
	  tableType_(ExtendedQualName::NORMAL_TABLE), //++ MV
	  isSpecialTypeSpecified_(FALSE),  //++ MV
	  isPOSNumPartnsSpecified_(FALSE),
	  isPOSInitialTableSizeSpecified_(FALSE),
	  isPOSMaxTableSizeSpecified_(FALSE),
          isPOSDiskPoolSpecified_(FALSE),
          isPOSIgnoreSpecified_(FALSE),
	  isNumRowsSpecified_(FALSE),
	  isIndexLevelsSpecified_(FALSE),
	  isPartnEOFSpecified_(FALSE),
	  posNumPartns_(-1),
	  posInitialTableSize_(-1),
	  posMaxTableSize_(-1),
          posDiskPool_(0),
          posNumDiskPools_(0),
          posIgnore_(FALSE),
	  numRows_(-1),
	  indexLevels_(-1),
	  partnEOF_(-1),
	  eInsertMode_(COM_REGULAR_TABLE_INSERT_MODE),
	  pInsertColumnsList_(pInsertColumnsList),
	  pQueryExpression_(queryExpression),
	  startOfCreateTableQuery_(0),
	  startOfCreateTableAsAttrList_(0),
	  ctaColumnsAreRenamed_(FALSE),
	  loadIfExists_(FALSE),
	  noLoad_(FALSE),
	  deleteData_(FALSE),
          isTableFeatureSpecified_(FALSE),
	  isDroppable_(TRUE),
          isInsertOnly_(FALSE),
	  pSGOptions_(NULL),
	  createIfNotExists_(FALSE),
          mapToHbaseTable_(FALSE),
          hbaseDataFormat_(FALSE)
{
  setChild(INDEX_TABLE_DEFINITION, pTableDefBody);
  setChild(INDEX_ATTRIBUTE_LIST, pCreateTableAttrList);
}

//
// virtual destructor
//
StmtDDLCreateTable::~StmtDDLCreateTable()
{
  // Delete the Primary Partition parse node.  This parse node
  // is not part of the parse tree.  The former was created
  // during the construction of the Create Table parse node
  // (this object).

  delete pPrimaryPartition_;

  // Delete the kludge parse nodes derived from class
  // StmtDDLAddConstraint.  For more information, please read
  // the contents of the header file StmtDDLCreateTable.h.

  StmtDDLAddConstraint * pAddConstraint;
  while (addConstraintArray_.getFirst(pAddConstraint))
  {
    delete pAddConstraint;
  }

  // Delete all children

  for (Int32 i = 0; i < getArity(); i++)
  {
    delete getChild(i);
  }
}

//
// cast virtual function
//
StmtDDLCreateTable *
StmtDDLCreateTable::castToStmtDDLCreateTable()
{
  return this;
}

//
// accessors
//

Int32
StmtDDLCreateTable::getArity() const
{
  return MAX_STMT_DDL_CREATE_TABLE_ARITY;
}

ExprNode *
StmtDDLCreateTable::getChild(Lng32 index)
{
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

ComPartitioningScheme
StmtDDLCreateTable::getPartitioningScheme() const
{
  if (getIsHashV1PartitionSpecified()) // HASH PARTITION clause specified
    return COM_HASH_V1_PARTITIONING;
  if (getIsHashV2PartitionSpecified()) // HASH2 PARTITION clause specified
    return COM_HASH_V2_PARTITIONING;

  ComASSERT(pPrimaryPartition_ NEQ NULL);
  switch (pPrimaryPartition_->getOperatorType())
  {
  case ELM_PARTITION_SINGLE_ELEM :
    //
    // Note that COM_NO_PARTITIONING and
    // COM_SINGLE_PARTITIONING defined in
    // ComSmallDefs.h have the same meaning.
    //
    return COM_SINGLE_PARTITIONING;
  case ELM_PARTITION_RANGE_ELEM :
    return COM_RANGE_PARTITIONING;
  case ELM_PARTITION_SYSTEM_ELEM :
    return COM_SYSTEM_PARTITIONING;
  default :
    ABORT("internal logic error");
    return COM_UNKNOWN_PARTITIONING;    // any dummy value would do
  }
}

//
// mutators
//

//
// Computes the default primary partitioning scheme and then
// creates the Primary Partition parse node.
//
void
StmtDDLCreateTable::computeDefaultPrimaryPartition()
{
  //
  // This method should not be called unless
  // the Partition clause does not appear.
  //

  //
  // Computes the default partitioning scheme.
  // There are four (4) possibilities:
  //
  // 1a.  If the STORE BY NONDROPPABLE PRIMARY KEY clause appears, or
  // 1b.  if the STORE BY clause does not appear and the PRIMARY KEY
  //      clause with the NONDROPPABLE option is specified, then
  // uses the range partitioning scheme.
  //
  if (/*1a*/ (isStoreBySpecified() AND
              getStoreOption() EQU COM_NONDROPPABLE_PK_STORE_OPTION) OR
      /*1b*/ (NOT isStoreBySpecified() AND
              getIsConstraintPKSpecified() AND
              NOT getAddConstraintPK()->isDroppable()))
  {
    pPrimaryPartition_ = new(PARSERHEAP()) ElemDDLPartitionRange();
    pPrimaryPartition_->setTheSpecialCase1aOr1bFlag(TRUE);
  }
  //
  // 2.  If the STORE BY clause does not appear and the PRIMARY KEY
  //     clause with the NONDROPPABLE option is not specified, then
  //     we can not partition the table at all (forever)!  There can
  //     only exist one single partition, the primary partition.
  //
  else if (NOT isStoreBySpecified() AND
           (NOT getIsConstraintPKSpecified() OR
            getAddConstraintPK()->isDroppable()))
  {
    pPrimaryPartition_ = new(PARSERHEAP()) ElemDDLPartitionSingle();
  }
  //
  // 3.  If the STORE BY <key-column-list> clause appears (whether
  //     the PRIMARY KEY appears or not), uses the range partitioning
  //     scheme.
  //
  else if (isStoreBySpecified() AND
           getStoreOption() EQU COM_KEY_COLUMN_LIST_STORE_OPTION)
  {
    pPrimaryPartition_ = new(PARSERHEAP()) ElemDDLPartitionRange();
  }
  //
  // 4.  If the STORE BY ENTRY ORDER clause appears, uses the
  //     system partitioning scheme.
  //
  else if (isStoreBySpecified() AND
           getStoreOption() EQU COM_ENTRY_ORDER_STORE_OPTION)
  {
    pPrimaryPartition_ = new(PARSERHEAP()) ElemDDLPartitionSystem();
  }
  //
  // I intentionally structured the nested IF code to reflect the
  // descriptions on pages 103 and 104 of the version 2.0 of the
  // "SQL/Ark DDL Language" external specification, dated 15
  // November 1995.  The code could have been simplified, but I
  // decided that it would be easier to maintain the code if I
  // structured the code this way.
  //
  // I was not sure whether I should have issued error messages
  // for the cases not covered in the external specification or
  // not.  For now I decided to use the system partitioning scheme
  // for these cases.
  //                             Kenneth Luu   2/1/96
  //
  else
  {
    pPrimaryPartition_ = new(PARSERHEAP()) ElemDDLPartitionSystem();
  }

  partitionArray_.insert(pPrimaryPartition_);
}

void
StmtDDLCreateTable::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());
  if (pChildNode EQU NULL)
  {
    children_[index] = NULL;
  }
  else
  {
    children_[index] = pChildNode->castToElemDDLNode();
  }
}

void
StmtDDLCreateTable::setConstraint(ElemDDLNode * pElement)
{
  switch (pElement->getOperatorType())
  {
  case ELM_CONSTRAINT_CHECK_ELEM :
    {
      ComASSERT(pElement->castToElemDDLConstraintCheck() NEQ NULL);

      // Create a kludge parse node.  For more information, please
      // read the contents of the head file StmtDDLCreateTable.h

      StmtDDLAddConstraintCheck * pAddConstraintCheck =
        new(PARSERHEAP())
	  StmtDDLAddConstraintCheck(getTableNameAsQualifiedName(), pElement);

      pAddConstraintCheck->setIsParseSubTreeDestroyedByDestructor(FALSE);
      addConstraintArray_.insert(pAddConstraintCheck);

      addConstraintCheckArray_.insert(pAddConstraintCheck);
    }
    break;

  case ELM_CONSTRAINT_PRIMARY_KEY_ELEM :
  case ELM_CONSTRAINT_PRIMARY_KEY_COLUMN_ELEM :
    {
      ComASSERT(pElement->castToElemDDLConstraintPK() NEQ NULL);
      if (isPrimaryKeyClauseSpec_)
      {
        // Duplicate PRIMARY KEY clauses.
        *SqlParser_Diags << DgSqlCode(-3106);
      }
      isPrimaryKeyClauseSpec_ = TRUE;

      ElemDDLNode * pColRefList = pElement->castToElemDDLConstraintPK()
        ->getColumnRefList();
      ComASSERT(pColRefList NEQ NULL);

      for (CollIndex index = 0; index < pColRefList->entries(); index++)
      {
        primaryKeyColRefArray_.insert(
             (*pColRefList)[index]->castToElemDDLColRef());
      }

      // Create a kludge parse node.  For more information, please
      // read the contents of the head file StmtDDLCreateTable.h

      pAddConstraintPK_ = new(PARSERHEAP())
	StmtDDLAddConstraintPK(getTableNameAsQualifiedName(), pElement);

      pAddConstraintPK_->setIsParseSubTreeDestroyedByDestructor(FALSE);
      addConstraintArray_.insert(pAddConstraintPK_);
    }
    break;

  case ELM_CONSTRAINT_REFERENTIAL_INTEGRITY_ELEM :
    {
      ComASSERT(pElement->castToElemDDLConstraintRI() NEQ NULL);

      // Create a kludge parse node.  For more information, please
      // read the contents of the head file StmtDDLCreateTable.h

      StmtDDLAddConstraintRI * pAddConstraintRI = new(PARSERHEAP())
	  StmtDDLAddConstraintRI(getTableNameAsQualifiedName(), pElement);

      pAddConstraintRI->setIsParseSubTreeDestroyedByDestructor(FALSE);
      addConstraintArray_.insert(pAddConstraintRI);

      addConstraintRIArray_.insert(pAddConstraintRI);
    }
    break;

   case ELM_CONSTRAINT_UNIQUE_ELEM :
    {
      ComASSERT(pElement->castToElemDDLConstraintUnique() NEQ NULL);

      // Create a kludge parse node.  For more information, please
      // read the contents of the head file StmtDDLCreateTable.h

      StmtDDLAddConstraintUnique * pAddConstraintUnique =
        new(PARSERHEAP())
	  StmtDDLAddConstraintUnique(getTableNameAsQualifiedName(),
				     pElement);

      pAddConstraintUnique->setIsParseSubTreeDestroyedByDestructor(FALSE);
      addConstraintArray_.insert(pAddConstraintUnique);

      addConstraintUniqueArray_.insert(pAddConstraintUnique);
    }
    break;

  default :
    NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
    break;
  }
} // StmtDDLCreateTable::setConstraint()

void
StmtDDLCreateTable_visitTableDefElement(ElemDDLNode * pCreateTableNode,
                                        CollIndex /* index */,
                                        ElemDDLNode * pElement);
//
// collects information in the parse sub-tree and copy/move them
// to the current parse node.
//
void
StmtDDLCreateTable::synthesize()
{
  ElemDDLNode *pTableDefBody = NULL;

  if (! pQueryExpression_)
    {
      ComASSERT(getChild(INDEX_TABLE_DEFINITION) NEQ NULL);
    }

  if (getChild(INDEX_TABLE_DEFINITION) NEQ NULL)
    {
      pTableDefBody =
	getChild(INDEX_TABLE_DEFINITION)->castToElemDDLNode();
      
      ComASSERT(pTableDefBody NEQ NULL);
    }
  
  ElemDDLNode *pCreateTableAttrList = NULL;
  if (getChild(INDEX_ATTRIBUTE_LIST) NEQ NULL)
    {
      pCreateTableAttrList = 
	getChild(INDEX_ATTRIBUTE_LIST)->castToElemDDLNode();
      ComASSERT(pCreateTableAttrList NEQ NULL);
    }

  ElemDDLStoreOptKeyColumnList * sbkcl = NULL; //store by keycolumnlist
  ElemDDLStoreOptNondroppablePK * sbpk = NULL; //store by primary key
  ElemDDLPartitionClause * pbc         = NULL; //partition by clause
  NABoolean isDuplicateDivisionByClauseErrorMsgAlreadyIssued = FALSE;
  if (pCreateTableAttrList NEQ NULL)
    {
      if (pCreateTableAttrList->castToElemDDLOptionList() NEQ NULL)
	{
	  for (CollIndex i = 0; i < pCreateTableAttrList->entries(); i++)
	    {
	      ElemDDLNode * pTableOption = (*pCreateTableAttrList)[i];
	      
	      if (pTableOption->castToElemDDLStoreOptKeyColumnList() 
		  NEQ NULL)
		{
		  sbkcl = 
		    pTableOption->castToElemDDLStoreOptKeyColumnList();
		}
	      if (pTableOption->castToElemDDLStoreOptNondroppablePK() 
		  NEQ NULL)
		{
		  sbpk = 
		    pTableOption->castToElemDDLStoreOptNondroppablePK();
		}
	      if (pTableOption->castToElemDDLPartitionClause() 
		  NEQ NULL)
		{
		  pbc = 
		    pTableOption->castToElemDDLPartitionClause();
		}
	      else if (pTableOption->castToElemDDLDivisionClause() 
		       NEQ NULL) // The user specified the DIVISION BY clause
		{
		  if (isDivisionByClauseSpec_ AND NOT isDuplicateDivisionByClauseErrorMsgAlreadyIssued)
		    {
		      // Error 3183 - Duplicate $0~string0 clauses were specified.
		      *SqlParser_Diags << DgSqlCode(-3183)
		                       << DgString0("DIVISION BY");
		      isDuplicateDivisionByClauseErrorMsgAlreadyIssued = TRUE;
		    }
		  pDivisionByClauseParseNode_ =
		    pTableOption->castToElemDDLDivisionClause();
		  isDivisionByClauseSpec_ = TRUE;
		}
	      else if (pTableOption->castToElemDDLHbaseOptions() NEQ NULL)
		{
		  if (isHbaseOptionsSpecified())
		    {
		      // Error 3183 - Duplicate $0~string0 clauses were specified.
		      *SqlParser_Diags << DgSqlCode(-3183)
				       << DgString0("HBASE_OPTIONS");
		    }
		  pHbaseOptionsParseNode_ =
		    pTableOption->castToElemDDLHbaseOptions();
		  isHbaseOptionsSpec_ = TRUE;
		}
	    }
	}
      else
	{
	  if (pCreateTableAttrList->castToElemDDLStoreOptKeyColumnList() 
	      NEQ NULL)
	    sbkcl = 
	      pCreateTableAttrList->castToElemDDLStoreOptKeyColumnList();
	  if (pCreateTableAttrList->castToElemDDLStoreOptNondroppablePK() 
	      NEQ NULL)
	    sbpk = 
	      pCreateTableAttrList->castToElemDDLStoreOptNondroppablePK();
	  if (pCreateTableAttrList->castToElemDDLPartitionClause() 
	      NEQ NULL)
	    pbc = 
	      pCreateTableAttrList->castToElemDDLPartitionClause();
	  else if (pCreateTableAttrList->castToElemDDLDivisionClause()
	           NEQ NULL) // The user specified the DIVISION BY clause
	    {
	      pDivisionByClauseParseNode_ =
		pCreateTableAttrList->castToElemDDLDivisionClause();
	      isDivisionByClauseSpec_ = TRUE;
	    }
	  else if (pCreateTableAttrList->castToElemDDLHbaseOptions() NEQ NULL)
	    {
	      pHbaseOptionsParseNode_ =
		pCreateTableAttrList->castToElemDDLHbaseOptions();
	      isHbaseOptionsSpec_ = TRUE;
	    }
	}
    } // some table attr specified

  isUniqueStoreByKeylist_ = FALSE;  // default
  isUniqueStoreByPrimaryKey_ = FALSE;  // default
  NABoolean isPkeyStoreByKeylist = FALSE;

  // if the user specified UNIQUE PRIMARY KEY set the flag.
  if ( (sbpk != NULL) &&
       (sbpk->isUniqueStoreByPrimaryKey()))
    {
      isUniqueStoreByPrimaryKey_ = TRUE;
    }

  // if the user specified STORE BY UNIQUE (col-list) then set flag.
  if ( (sbkcl != NULL) &&
       (sbkcl->getKeyColumnList()->entries() > 0))
    {
      // User specified a STORE BY (col-list) clause; did they specify
      // the UNIQUE keyword too?
      isUniqueStoreByKeylist_ = sbkcl->isUniqueStoreByKeylist();

      // did they specify a primary key clause with column list
      isPkeyStoreByKeylist = sbkcl->isPkeyStoreByKeylist();
    }

  NABoolean userSpecifiedPKey = FALSE;
  NABoolean nullablePKeySpecified = FALSE;
  if ((CmpCommon::getDefault(TRAF_MAKE_PKEY_COLUMNS_NOT_NULL) == DF_ON) ||
      (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON) ||
      (isVolatile()) ||
      ((isPkeyStoreByKeylist) && pTableDefBody && 
       (pTableDefBody->castToElemDDLLikeCreateTable() == NULL)))
    {
      NABoolean addPrimaryKeyClause = FALSE;

      ElemDDLNode * keyColsList = NULL;
      ElemDDLColDef * col = NULL;
      ElemDDLColDef * volTabSbyCol = NULL;
      Lng32 volTabSbyColPos = -1;
      ElemDDLNode * currListElem = pTableDefBody;
      ElemDDLNode * currElem = NULL;
      while (currListElem)
	{
	  if (currListElem->getOperatorType() EQU ELM_ELEM_LIST)
	    {
	      currElem = (ElemDDLNode*)currListElem->getChild(1);
	      currListElem = (ElemDDLNode*)currListElem->getChild(0);
	    }
	  else
	    {
	      currElem = currListElem;
	      currListElem = NULL;
	    }
	  
	  if (currElem->getOperatorType() EQU ELM_COL_DEF_ELEM)
	    {
	      col = (ElemDDLColDef *)currElem;
	      if (col->getConstraintPK())
		userSpecifiedPKey = TRUE;

	      // Hierarchy of the datatypes that will be used to determine
	      // the keys of a volatile table. Hierarchy is low to high 
	      // (first entry is the lowest)
	      static const Lng32 volTabSbyHier[] =
	      {
		REC_BYTE_V_ASCII,

		REC_INT_SECOND,
		REC_INT_MINUTE_SECOND,
		REC_INT_MINUTE,
		REC_INT_HOUR_SECOND,
		REC_INT_DAY_SECOND,
		REC_INT_DAY,
		REC_INT_HOUR,
		REC_INT_DAY_HOUR,
		REC_INT_HOUR_MINUTE,
		REC_INT_DAY_MINUTE,
		REC_INT_YEAR,
		REC_INT_MONTH,
		REC_INT_YEAR_MONTH,

		REC_DATETIME,

		REC_BYTE_F_ASCII,

		REC_NUM_BIG_SIGNED,
		REC_NUM_BIG_UNSIGNED,

		REC_DECIMAL_LSE,
		REC_DECIMAL_UNSIGNED,

		REC_BIN16_SIGNED,
		REC_BIN16_UNSIGNED,
		REC_BIN32_SIGNED,
		REC_BIN32_UNSIGNED,
		REC_BIN64_UNSIGNED,
		REC_BIN64_SIGNED
	      };

	      // find the column on which to store/hash based on the
	      // hierarchy specified in the volTabSbyHier array.
	      // Do this only if there are no user specified pkey, store by,
	      // or partition by clauses.
	      // Within that hierarchy, non-nullable values have preference
	      // over nullable values.
	      // If a column is found, then it will be used later in this
	      // method to create a storeBy clause of the volatile table
	      // so that the table created as partitioned table.
	      // If no usable columns are found,
	      // then the table will be created as non-partitioned.
	      if ((CmpCommon::getDefault(VOLATILE_TABLE_FIND_SUITABLE_KEY) != DF_OFF) &&
		  (NOT userSpecifiedPKey) &&
		  ((sbkcl == NULL) && (sbpk == NULL) && (pbc == NULL)))
		{
		  NABoolean found = FALSE;
		  Lng32 j = 0;
		  Lng32 maxSize = sizeof(volTabSbyHier) / sizeof(Lng32); 
		  for (j = 0; ((NOT found) && (j < maxSize)); j++)
		    {
		      if (col->getColumnDataType() &&
		          col->getColumnDataType()->getFSDatatype() ==
			   volTabSbyHier[j])
			found = TRUE;
		    }

		  if (found) 
		    {
		      if (volTabSbyCol == NULL)
			{
			  volTabSbyColPos = j;
			  volTabSbyCol = col;
			}
		      else
			{
			  // choose the current col if it has a higher
			  // hierarchy than the saved column.
			  // If they have the same hierarchy then pick the
			  // non-nullable column.
			  // If they have the same hierarchy and same
			  // nullability, then pick the current col.
			  if ((j > volTabSbyColPos) ||
			      ((j == volTabSbyColPos) &&
			       ((col->isNotNullConstraintSpecified() ==
				 volTabSbyCol->isNotNullConstraintSpecified()) ||
				(col->isNotNullConstraintSpecified()))))
			    {
			      volTabSbyColPos = j;
			      volTabSbyCol = col;
			    }
			}
		    }
		}		  
	    }
	  else
	    {
	      if (currElem->getOperatorType() EQU 
		  ELM_CONSTRAINT_PRIMARY_KEY_ELEM)
		{
		  userSpecifiedPKey = TRUE;

		  keyColsList = 
		    currElem->castToElemDDLConstraintPK()->getColumnRefList();

                  nullablePKeySpecified = 
                    currElem->castToElemDDLConstraintPK()->isNullableSpecified();
		}
	    }
	} // while

      if (userSpecifiedPKey)
	{
	  volTabSbyCol = NULL;

	  if (isPkeyStoreByKeylist)
	    {
	      // cannot specify 'primary key' as a column AND a table attr.
	      // Return error.
	      *SqlParser_Diags << DgSqlCode(-3106);
	      return;
	    }
	}

      // if no user specified pkey or store by clause has been specified
      // for a volatile table, then make the first column the pkey.
      if ((isVolatile()) &&
	  (sbkcl == NULL) &&
	  (NOT userSpecifiedPKey) &&
	  (col != NULL) &&
	  (CmpCommon::getDefault(VOLATILE_TABLE_FIND_SUITABLE_KEY) == DF_OFF))
	{
	  // make the storeby list the primary key
	  ElemDDLConstraintPKColumn * pk = 
	    new (PARSERHEAP()) ElemDDLConstraintPKColumn();
	  pk->setConstraintAttributes(NULL);
	  col->setColumnAttribute(pk);

	  ElemDDLNode * pColAttrList = NULL;
	  if (col->getChild(ElemDDLColDef::INDEX_ELEM_DDL_COL_ATTR_LIST))
	    pColAttrList =
	      col->getChild(ElemDDLColDef::INDEX_ELEM_DDL_COL_ATTR_LIST)->castToElemDDLNode();
	  
	  ElemDDLNode * newColAttrList = NULL;
	  if (pColAttrList)
	    newColAttrList = 
	      new (PARSERHEAP()) ElemDDLList(pColAttrList, pk);
	  else
	    newColAttrList = pk;
	  col->setChild(ElemDDLColDef::INDEX_ELEM_DDL_COL_ATTR_LIST, 
                        newColAttrList);

	  userSpecifiedPKey = TRUE;
	}

      if ((sbkcl) &&
	  (pTableDefBody) &&
	  (NOT userSpecifiedPKey) &&
	  (sbkcl->getKeyColumnList()->entries() > 0))
	{
	  keyColsList = sbkcl->getKeyColumnList();
	  
	  if (sbkcl->isUniqueStoreBy())
	    {
	      // make the storeby list the primary key
	      ElemDDLConstraintPK * pk = 
		new (PARSERHEAP()) ElemDDLConstraintPK
                (NULL, sbkcl->getSerializedOption());
	      pk->setColumnRefList(keyColsList);
	      pk->setConstraintKind(ElemDDLConstraint::TABLE_CONSTRAINT_DEF);
	      pk->setConstraintAttributes(NULL);

	      pTableDefBody = new (PARSERHEAP())
		ElemDDLList(pTableDefBody, pk);
	    }
	}


      // if no user specified pkey or store by clause or partition by clause
      // has been specified for a volatile table, and the CQD is OFF,
      // then pick the first column based on the hierarchy specified
      // in array volTabSbyType, and add the following clause: 
      //
      //    store by (<first-col>)
      //
      if ((isVolatile()) &&
	  (sbkcl == NULL) &&
	  (NOT userSpecifiedPKey) &&
	  (volTabSbyCol != NULL) &&
	  (pbc == NULL) &&
	  (CmpCommon::getDefault(VOLATILE_TABLE_FIND_SUITABLE_KEY) != DF_OFF))
	{
	  sbkcl = new (PARSERHEAP())
	    ElemDDLStoreOptKeyColumnList(
		 new (PARSERHEAP()) 
		 ElemDDLColRef(volTabSbyCol->getColumnName(), 
			       COM_UNKNOWN_ORDER, PARSERHEAP()));
		 
	  if (pCreateTableAttrList)
	    {
	      pCreateTableAttrList = new (PARSERHEAP())
		ElemDDLOptionList(pCreateTableAttrList, sbkcl);
	    }
	  else
	    pCreateTableAttrList = sbkcl;
	  setChild(INDEX_ATTRIBUTE_LIST, pCreateTableAttrList);

	  userSpecifiedPKey = TRUE;
	}

      // loop over all cols and make nullable columns which are part
      // of pkey specification, not-null-non-droppable.
      // For volatile tables, do this only if cqd 
      // VOLATILE_TABLE_FIND_SUITABLE_KEY is set to OFF.
      if (((NOT isVolatile()) &&
           (CmpCommon::getDefault(TRAF_MAKE_PKEY_COLUMNS_NOT_NULL) == DF_ON) &&
           (NOT nullablePKeySpecified)) ||
          ((isVolatile()) && 
           (CmpCommon::getDefault(VOLATILE_TABLE_FIND_SUITABLE_KEY) == DF_OFF)))
	{
	  currListElem = pTableDefBody;
	  while (currListElem)
	    {
	      if (currListElem->getOperatorType() EQU ELM_ELEM_LIST)
		{
		  currElem = (ElemDDLNode*)currListElem->getChild(1);
		  currListElem = (ElemDDLNode*)currListElem->getChild(0);
		}
	      else
		{
		  currElem = currListElem;
		  currListElem = NULL;
		}
	      
	      if (currElem->getOperatorType() EQU ELM_COL_DEF_ELEM)
		{
		  col = (ElemDDLColDef *)currElem;
		}
	      else
		{
		  continue;
		}
	      
	      // if 'col' is nullable and part of keyColsList, make
	      // 'col' not nullable.
	      if (NOT col->isNotNullConstraintSpecified())
		{
		  NABoolean makeThisColNNND = FALSE;
		  
		  if (col->isPrimaryKeyConstraintSpecified())
                    {
                      makeThisColNNND = TRUE;

                      if (col->getConstraintPK() && 
                          col->getConstraintPK()->isNullableSpecified())
                        makeThisColNNND = FALSE;
                    }
		  else if (keyColsList NEQ NULL)
		    {
		      // See if this col is in pkey or store by clause.
		      for (CollIndex index = 0; index < keyColsList->entries(); 
			   index++)
			{
			  ComASSERT((*keyColsList)[index]->castToElemDDLColRef() NEQ NULL);
			  ElemDDLColRef * kCol = 
			    (*keyColsList)[index]->castToElemDDLColRef();
			  
			  if (col->getColumnName() == kCol->getColumnName())
			    {
			      // make col non-nullable
			      makeThisColNNND = TRUE;
			    }
			} // for
		    } // else
		  
		  // soln 10-090312-9983.  If dataType is NULL then create table AS will set it later.		  		  
		  if (makeThisColNNND && col->getColumnDataType())
		    {
		      ElemDDLConstraintNotNull * nn =
			new (PARSERHEAP()) ElemDDLConstraintNotNull(PARSERHEAP());
		      ElemDDLConstraintAttrDroppable * nd = 
			new (PARSERHEAP())
			ElemDDLConstraintAttrDroppable(FALSE);
		      
		      nn->setConstraintAttributes(nd);
		      col->setColumnAttribute(nn);
		      
		    }
		} // col is nullable
	      
	    } // while
	}
    } //
  
  // ---------------------------------------------------------------------
  // traverse the Table Definition parse sub-tree
  // ---------------------------------------------------------------------

  // If pTableDefBody points to a parse node representing LIKE clause
  // (instead of a list of column and table constraint definitions),
  // gather the LIKE options specified in the LIKE clause and store
  // them in data member likeOptions_.  Also save the source table
  // name in data member likeSourceTableCorrName_.
  if (pTableDefBody)
    {
      if (pTableDefBody->castToElemDDLLikeCreateTable() NEQ NULL)
	{
	  
	  if (isLikeClauseSpec_)
	    {
	      // Duplicate LIKE clauses.
	      *SqlParser_Diags << DgSqlCode(-3107);
	    }
	  isLikeClauseSpec_ = TRUE;
	  
	  likeSourceTableCorrName_ = pTableDefBody->castToElemDDLLikeCreateTable()
	    ->getDDLLikeNameAsCorrName();
	  likeOptions_ = pTableDefBody->castToElemDDLLikeCreateTable()
	    ->getLikeOptions();

          if ((NOT isExternal()) &&
              (pTableDefBody->castToElemDDLLikeCreateTable()->forExtTable()))
            {
              *SqlParser_Diags << DgSqlCode(-3242)
                               << DgString0("'for' clause can only be specified when creating an 'external' table.");
              return;
            }
          else if ((isExternal()) &&
                   (NOT pTableDefBody->castToElemDDLLikeCreateTable()->forExtTable()))
            {
              *SqlParser_Diags << DgSqlCode(-3242)
                               << DgString0("'like' clause cannot be specified when creating an external table.");
              return;
            }
	}
      else
	{
	  // pTableDefBody points to a (left linear tree) list of
	  // column and/or table constraint definitions
	  
	  // Traverse the parse sub-tree containing the list of column
	  // and table constraint definitions.
	  //
	  // Update the data member columnDefArray_ to contain pointers
	  // pointing to Column Definition parse nodes.  Storing the
	  // pointers in columnDefArray_ allows the user to access to
	  // Column Definition parse nodes more efficiently.  The friend
	  // function StmtDDLCreateTable_visitTableDefElement does the
	  // insertion.
	  //
	  // For each column and table constraint definition (except for
	  // Not Null constraint), add them to the corresponding contraint
	  // list.  For more information, please read the contents of the
	  // header file StmtDDLCreateTable.h.
	  
	  pTableDefBody->traverseList(
	       this,
	       StmtDDLCreateTable_visitTableDefElement);
	}
    }

  // ---------------------------------------------------------------------
  // traverse the Create Table Attribute List sub-tree
  // ---------------------------------------------------------------------

  // Traverse the Create Table Attribute List sub-tree to extract the
  // information about the specified file attributes.  Store this
  // information in data member fileAttributes_.

  if (pCreateTableAttrList NEQ NULL)
  {
    if (pCreateTableAttrList->castToElemDDLOptionList() NEQ NULL)
    {
      for (CollIndex i = 0; i < pCreateTableAttrList->entries(); i++)
      {
        setTableOption((*pCreateTableAttrList)[i]);
      }
    }
    else
    {
      //
      // pCreateTableAttrList points to a single Create Table
      // option (e.g., a partition clause, a file attributes
      // clause, a location clause, etc.
      //
      setTableOption(pCreateTableAttrList);
    }
  } // if (pCreateTableAttrList NEQ NULL)

  // Don't allow volatile tables to be created as not droppable
  if (isVolatile() && !isDroppable()) 
    *SqlParser_Diags << DgSqlCode(-1286); // volatile table must be droppable

  // ---------------------------------------------------------------------
  // Updates information about file attributes if necessary
  // ---------------------------------------------------------------------

  //
  // If Buffered phrase does not appeared, sets its default value
  // depending on whether the table is audited or not.
  //
  getFileAttributes().setDefaultValueForBuffered();

  // if default column family is not specified, then set it to traf default
  getFileAttributes().setDefaultValueForColFam();

  // ---------------------------------------------------------------------
  // Creates or updates parse node representing primary partition
  // ---------------------------------------------------------------------

  //
  // If partition clause does not appear, the partitionArray_
  // is still empty; constructs the primary partition node and
  // then inserts its pointer to the array.
  //

  if (partitionArray_.entries() EQU 0)
  {
    //
    // The following method should only be invoked after all
    // information in the parse has been collected and copied
    // to the Create Table (root) parse node.
    //
    computeDefaultPrimaryPartition();
  }
  ComASSERT(pPrimaryPartition_ NEQ NULL);

  // ---------------------------------------------------------------------
  // Temporary code--Currently we do not allow multiple partitions
  // for entry-sequenced user tables.
  // ---------------------------------------------------------------------

  if (getStoreOption() EQU COM_ENTRY_ORDER_STORE_OPTION AND
      isPartitionSpecified())
  {
    // *** ERROR[3121] Partitioned entry-sequenced tables
    //                   currently not allowed.
    // I (Huy) will need to change this to an error instead. 
    // Otherwise execution will keep go, and executor spit
    // out weird error message. Remove the minus in front of
    // the 3121 to get a warning , and remove the whole line
    // to support the entry-sequence partition table.

     *SqlParser_Diags << DgSqlCode(-3121);
  }


  // I (Huy) added the code to check if the table is 
  // Key-Sequence is specified, 
  // Then first key is not allowed.

  /**********
  if ( getIsStoreBySpecified() && getIsLikeOptionSpecified()) 
    {
      *SqlParser_Diags << DgSqlCode(-3156);
      return;
    }
  **********/
    
  if (getStoreOption() NEQ COM_ENTRY_ORDER_STORE_OPTION AND 
      getStoreOption() NEQ COM_RELATIVE_ORDER_STORE_OPTION)
    {  
       // is key-sequenced by default it is a key-sequence table.
       // unless the store by option is specified.

     /*****************
     if ((Get_SqlParser_Flags(ALLOW_FUNNY_TABLE_CREATE)) || 
	 (getIsLikeOptionSpecified())  
	)
	 {
           for (CollIndex i = 0; i < getPartitionArray().entries(); i++)
	     if (getPartitionArray()[i]->castToElemDDLPartitionRange() NEQ NULL)
	       {
		 // this is the case that the table is key-sequence and 
		 // the partition scheme is vertical, then the first key is 
		 // not allowed.
		 *SqlParser_Diags << DgSqlCode(-3146);
		 break;
	       }
	 }
     else 
       { 
     *****************/
	 if (partitionArray_.entries() > 1) // PARTITION clause specified
	   {
	     for (CollIndex i = 0; i < getPartitionArray().entries(); i++)
	       {
		 if (getPartitionArray()[i]->castToElemDDLPartitionRange() EQU NULL)
		   
		   {  // Neither FIRST KEY phrase nor HASH specified in PARTITION clause
		     if (getIsHashV1PartitionSpecified() EQU FALSE &&
			 getIsHashV2PartitionSpecified() EQU FALSE)
		       {
			 *SqlParser_Diags << DgSqlCode(-3138);
			 break;
		       }
		   }
		 else // FIRST KEY WAS SPECIFIED--cannot specify HASH
		   {
		     if (getIsHashV1PartitionSpecified() ||
			 getIsHashV2PartitionSpecified())
		       {
			 *SqlParser_Diags << DgSqlCode(-3153);
			 break;
		       }
		   }
	       }
	   }
       //} // else 
    } // if is key-sequenced

  // The information specified in the location clause and
  // load option clauses (e.g., dslack and islack) is for
  // the primary partition, copies it to the primary partition
  // node.
  //

  //
  // Note that class ElemDDLPartitionSystem is the base class
  // for both derived classes ElemDDLPartitionSingle and
  // ElemDDLPartitionRange.
  //
  ElemDDLPartitionSystem * pSysPart =
    pPrimaryPartition_->castToElemDDLPartitionSystem();
  if (pSysPart EQU NULL)
  {
    ABORT("internal logic error");
  }
  else
  {
    //
    // location
    //

    if (isLocationSpecified())
    {
      pSysPart->setLocationName(getLocationName());
      pSysPart->setLocationNameType(getLocationNameType());
	  pSysPart->setPartitionName(getPartitionName());
    }

    //
    // file attributes
    //

    const ParDDLFileAttrsCreateIndex & fileAttrs = getFileAttributes();
    if (fileAttrs.isMaxSizeSpecified())
    {
      pSysPart->setIsMaxSizeSpecified(fileAttrs.isMaxSizeSpecified());
      pSysPart->setIsMaxSizeUnbounded(fileAttrs.isMaxSizeUnbounded());
      pSysPart->setMaxSize           (fileAttrs.getMaxSize());
      pSysPart->setMaxSizeUnit       (fileAttrs.getMaxSizeUnit());
    }
    
    if (fileAttrs.isExtentSpecified())
    { 
      pSysPart->setIsExtentSpecified(fileAttrs.isExtentSpecified());
      pSysPart->setPriExt           (fileAttrs.getPriExt());
      pSysPart->setSecExt           (fileAttrs.getSecExt());
    }

    if (fileAttrs.isMaxExtentSpecified())
    {
      pSysPart->setIsMaxExtentSpecified(fileAttrs.isMaxExtentSpecified());
      pSysPart->setMaxExt           (fileAttrs.getMaxExt());
    }

  } // else (pSysPart NEQ NULL)

  if (isStoreBySpecified() AND
      getStoreOption() EQU COM_NONDROPPABLE_PK_STORE_OPTION)
  {
    if (getIsConstraintPKSpecified())
    {
      if (isUniqueStoreByPrimaryKey_)
      {
        // STORE BY UNIQUE PRIMARY KEY clause appears in the CREATE TABLE
        // definition.  Do not give error 3065 because of UNIQUE.
      }
      else
      if (getAddConstraintPK()->isDroppableSpecifiedExplicitly())
      {
        // The primary key constraint cannot be droppable when the STORE BY
        // PRIMARY KEY clause appears in the CREATE TABLE definition.  Please
        // change the definition of the primary key constraint to make it
        // NOT DROPPABLE.
        *SqlParser_Diags << DgSqlCode(-3065);
      }
      else
      {
        // Forces the primary key constraint to be non-droppable
        getAddConstraintPK()->setDroppableFlag(FALSE);
      }
    }

    else
      // "store by primary key" specified, but no primary key exists
      // Error 3188 unless MODE_SPECIAL_1.
      if (!getIsLikeOptionSpecified())
        if (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_OFF)
        {
          *SqlParser_Diags << DgSqlCode(-3188);
          return;
        }
  }

  // check and RI constraints are not allowed on volatile tables
  if ((isVolatile()) &&
      ((getAddConstraintCheckArray().entries() > 0) ||         // no check constraint
       (getAddConstraintRIArray().entries() > 0)))             // no RI (FK) constraint
    {
      *SqlParser_Diags << DgSqlCode(-1283);
    }

  // if volatile table and pos extensions are not specified in
  // the ddl, then get number of partitions info from CQD and set it.
  // The default is 4 local partitions. The default is 100 Mb per partition.
  // Also set the max table size from temp table CQD, if that cqd exists.
  if ((isVolatile()) &&
      (CmpCommon::getDefault(POS) != DF_OFF))
    {
      // num partitions is not explicitly specified, get it from defaults.
      if (NOT isPOSNumPartnsSpecified())
	{
	  if (CmpCommon::getDefault(POS_NUM_OF_TEMP_TABLE_PARTNS, 0) == DF_SYSTEM)
	    // set to 4 local partitions.
	    posNumPartns_ = -4; // the default, 4 local partitions
	  else
	    {
	      posNumPartns_ = ActiveSchemaDB()->getDefaults().
		getAsLong(POS_NUM_OF_TEMP_TABLE_PARTNS);
	      
	      if (posNumPartns_ == 1)
		posNumPartns_ = 0;
	      
	      // negative number from this cqd indicates all(local + remote)
	      // segment partitions.
	      // positive number indicates local partitions.
	      // Negate it so +ve number can indicate all partitions and
	      // -ve number can indicate local partitions.
	      posNumPartns_ = - posNumPartns_;
	    }
	  
	  isPOSNumPartnsSpecified_ = TRUE;
	}

      // if max table size if not explicitly specified, get it from defaults.
      if (NOT isPOSMaxTableSizeSpecified())
	{
	  NAString str;
	  char tempStr[1000];
	  char *value;
	  const char *sep = " ,";
	  
	  CmpCommon::getDefault(POS_TEMP_TABLE_SIZE, str, 0);
	  strcpy(tempStr, str.data());
	  value = strtok(tempStr, sep);
	  if (value != NULL)
	    {
	      double tableSize = 0;
	      
	      sscanf(value, "%lf", &tableSize);
	      
	      posMaxTableSize_ = (Lng32)tableSize;
	      isPOSMaxTableSizeSpecified_ = TRUE;
	    }
	}
    }

  if (isPOSDiskPoolSpecified_)
  {
    if (posNumDiskPools_ != 0 )
    {
      if (posDiskPool_)  
      {
        if ((posDiskPool_ != MAX_COMSINT32) &&
            (posDiskPool_ > posNumDiskPools_))
        {
          *SqlParser_Diags << DgSqlCode(-3417) 
                           << DgInt0(posDiskPool_) 
                           << DgInt1(posNumDiskPools_);
        }
      }
      else
      {
        if (ActiveSchemaDB()->getDefaults().getAsLong(POS_DISK_POOL) >
                          posNumDiskPools_)
        {
          *SqlParser_Diags << DgSqlCode(-3417)
                           << DgInt0(ActiveSchemaDB()->getDefaults()
                                                 .getAsLong(POS_DISK_POOL))
                           << DgInt1(posNumDiskPools_);
        }
      }
    }
    else if ((ActiveSchemaDB()->getDefaults().getAsLong(POS_NUM_DISK_POOLS) > 0) &&
             ((posDiskPool_ != MAX_COMSINT32) &&
              (posDiskPool_ > ActiveSchemaDB()->
                           getDefaults().getAsLong(POS_NUM_DISK_POOLS))))
    {
      *SqlParser_Diags << DgSqlCode(-3417)
                       << DgInt0(posDiskPool_)
                       << DgInt1(ActiveSchemaDB()->
                                getDefaults().getAsLong(POS_NUM_DISK_POOLS));
    }

  }
        
  if (isPOSDiskPoolSpecified_ &&
      isPOSNumPartnsSpecified_ &&
      posNumPartns_ < 0)
  {
    *SqlParser_Diags << DgSqlCode(-3418);
  }

/*
  if ((isPOSDiskPoolSpecified_ ||
       (CmpCommon::getDefault(POS) == DF_DISK_POOL)) &&
      ((isPOSNumPartnsSpecified_ &&
        posNumPartns_ > 0)           ||
       ((CmpCommon::getDefault(POS_NUM_OF_PARTNS, 0) != DF_SYSTEM) &&
        (ActiveSchemaDB()->getDefaults().getAsULong(POS_NUM_OF_PARTNS) >0))) &&
      ((posNumDiskPools_ > 1) ||
       (ActiveSchemaDB()->getDefaults().getAsULong(POS_NUM_DISK_POOLS) > 1)))
  {
    *SqlParser_Diags << DgSqlCode(-3417);
  }
*/

} // StmtDDLCreateTable::synthesize()

// Parameter pCreateTableNode points to the Create Table
//   parse node.
// Parameter pElement points to either a column or table
//   constraint definition parse node in the left linear
//   tree list.  This tree is a sub-tree in the Create
//   Table parse node.
// Parameter index contains the index of the parse node
//   pointed by pElement in the (left linear tree) list.
//
void
StmtDDLCreateTable_visitTableDefElement(ElemDDLNode * pCreateTableNode,
                                        CollIndex /* index */,
                                        ElemDDLNode * pElement)
{
  ComASSERT(pCreateTableNode NEQ NULL AND
            pCreateTableNode->castToStmtDDLCreateTable() NEQ NULL AND
            pElement NEQ NULL);

  StmtDDLCreateTable * pCreateTable =
    pCreateTableNode->castToStmtDDLCreateTable();

  if (pElement->castToElemDDLLikeCreateTable() NEQ NULL)
    {
      pCreateTable->likeSourceTableCorrName_ = 
        pElement->castToElemDDLLikeCreateTable()
        ->getDDLLikeNameAsCorrName();
      pCreateTable->likeOptions_ = 
        pElement->castToElemDDLLikeCreateTable()
        ->getLikeOptions();

      if ((NOT pCreateTable->isExternal()) &&
          (pElement->castToElemDDLLikeCreateTable()->forExtTable()))
        {
          *SqlParser_Diags << DgSqlCode(-3242)
                           << DgString0("'for' clause can only be specified when creating an 'external' table.");
          return;
        }
      else if ((pCreateTable->isExternal()) &&
               (NOT pElement->castToElemDDLLikeCreateTable()->forExtTable()))
        {
          *SqlParser_Diags << DgSqlCode(-3242)
                           << DgString0("'like' clause cannot be specified when creating an external table.");
          return;
        }
    }
  else if (pElement->castToElemDDLConstraint() NEQ NULL)
  {
    //
    // table constraint definition
    //
    pCreateTable->setConstraint(pElement);
  }
  else if (pElement->castToElemDDLColDef() NEQ NULL)
  {
    ElemDDLColDef * pColDef = pElement->castToElemDDLColDef();

    if (pColDef->getColumnDataType() == NULL)
    {
      //      if ((CmpCommon::getDefault(COMP_BOOL_207) == DF_OFF) ||
      if (pCreateTable->getQueryExpression() == NULL)
      {
	*SqlParser_Diags << DgSqlCode(-1174)
			 << DgString0(pColDef->getColumnName());
	return;
      }
      else
      {
	pCreateTable->setCTAcolumnsAreRenamed(TRUE);
      }
    }

    pCreateTable->getColDefArray().insert(pColDef);
    if (pColDef->getIsConstraintPKSpecified())
    {
      pCreateTable->setConstraint(pColDef->getConstraintPK());
    }

    //
    // For each column constraint definition (except for
    // not null and primary key constraints), creates
    // a corresponding table constraint definition and
    // then insert the newly create parse node to the
    // appropriate table constraint array.  This arrangement
    // helps the processing of constraint definitions in
    // create table statement.
    //
    for (CollIndex i = 0; i < pColDef->getConstraintArray().entries(); i++)
    {
      pCreateTable->setConstraint(pColDef->getConstraintArray()[i]);
    }
  }
  else
  {
    NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
  }
} // StmtDDLCreateTable_visitTableDefElement()

//
// Copies the information in the specified secondary partition
// (pointed to by pElement) to the Create Table parse node
// pointed to by pCreateTableNode.
//
void
StmtDDLCreateTable_visitPartitionElement(ElemDDLNode * pCreateTableNode,
                                         CollIndex /* index */,
                                         ElemDDLNode * pElement)
{
  ComASSERT(pCreateTableNode NEQ NULL  AND  pElement NEQ NULL);

  StmtDDLCreateTable * pCreateTable =
    pCreateTableNode->castToStmtDDLCreateTable();
  ComASSERT(pCreateTable NEQ NULL);

  ElemDDLPartition * pPartition = pElement->castToElemDDLPartition();
  ComASSERT(pPartition NEQ NULL);

  //
  // Inserts the specified secondary partition node to the end
  // of the partitionArray_ of the Create Table parse node.
  // Note that secondary partitions are defined in the partition
  // clause.
  //

  switch (pPartition->getOperatorType())
  {
  case ELM_PARTITION_RANGE_ELEM :
  case ELM_PARTITION_SYSTEM_ELEM :
    //
    // Note that class ElemDDLPartitionRange is derived
    // from class ElemDDLPartitionSystem, and the latter
    // is derived from class ElemDDLPartition.
    //
    ComASSERT(pPartition->castToElemDDLPartitionSystem() NEQ NULL);
    if (pPartition->castToElemDDLPartitionSystem()->getOption()
      NEQ ElemDDLPartition::ADD_OPTION)
    {
      // Only ADD option allowed in PARTITION clause in CREATE TABLE statement.
      *SqlParser_Diags << DgSqlCode(-3104);
    }
    pCreateTable->partitionArray_.insert(pPartition);
    break;

  //
  // Other future partitioning schemes; e.g., hash partitioning
  //

  default :
    NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
    break;

  } // switch (pPartition->getOperatorType())

} // StmtDDLCreateTable_visitPartitionElement()

//
// Copies the information in the specified Partition clause
// (pointed to by pPartitionClause) to this object.
//
void
StmtDDLCreateTable::setPartitions(ElemDDLPartitionClause * pPartitionClause)
{
  ComASSERT(pPartitionClause NEQ NULL);

  if (  (isPartitionSpecified()) || 
        ((isPOSNumPartnsSpecified()) && (posNumPartns_ == 0)) )
  {
    // Duplicate PARTITION clauses.  It is OK to have a partn spec (e.g.
    // "hash2 partitioning") and a partitioning count (e.g. 
    // "number of partitions 2")   But, if "no partitions" is specified 
    // (number of partitions 0) then we don't allow any other partn spec
    *SqlParser_Diags << DgSqlCode(-3103)
                     << DgString0("PARTITION");
  }

  isPartitionClauseSpec_ = TRUE;

  //
  // Initializes pPartitions to point to the parse node representing
  // the definition of a partition (class ElemDDLPartition) or
  // a list of partition definitions (class ElemDDLPartitionList).
  //
  ElemDDLNode * pPartitions = pPartitionClause->getPartitionDefBody();

  //
  // Copy the partitioning type from the partition clause
  //

  if (pPartitionClause->getPartitionType() EQU COM_HASH_V1_PARTITIONING)
  {
    isHashV1PartitionSpec_ = TRUE;
  }
  else if (pPartitionClause->getPartitionType() EQU COM_HASH_V2_PARTITIONING)
  {
    isHashV2PartitionSpec_ = TRUE;
  }
 
  //
  // Constructs the primary partition node and then inserts
  // its pointer at the beginning of the partitionArray_.
  // Note that the partition clause does not include the
  // definition of the primary partition.
  //
  // The kind of the primary partition node must be the
  // same as that of the secondary partition node.
  //
  if (pPartitions EQU NULL)
    setPrimaryPartition(NULL);
  else
    // (*pPartitions)[0] points to the parse node
    // representing the first secondary partition.
    setPrimaryPartition((*pPartitions)[0]);

  //
  // Inserts the secondary partition nodes to the end
  // of the partitionArray_.  Note that secondary partitions
  // are defined in the partition clause.
  //
  if (pPartitions NEQ NULL)
    pPartitions->traverseList(this, StmtDDLCreateTable_visitPartitionElement);

  //
  // Initializes pPartitionByOption to point to the parse node 
  // representing the definition of partition by columns (class
  // ElemDDLPartitionByOpt).
  //
  ElemDDLNode *pPartitionByOption = pPartitionClause->getPartitionByOption();
  
  if(pPartitionByOption NEQ NULL)
  {
    // If the Partition By Option is specified, set the flag and copy 
    // the pointers to column parse nodes to partitionKeyColRefArray_.
    if(pPartitionByOption->castToElemDDLPartitionByOpt() NEQ NULL)
    {
      isPartitionByClauseSpec_ = TRUE;

      switch (pPartitionByOption->getOperatorType())
      {
      case ELM_PARTITION_BY_COLUMN_LIST_ELEM :
        ComASSERT(pPartitionByOption->castToElemDDLPartitionByColumnList() 
                                                                     NEQ NULL);
		{
          // Once we have other types of partition by
          // partitionByOption_ = COM_COLUMN_LIST_PARTITION_BY_OPTION;
          //
          // Copies array of pointers pointing to Column parse nodes to
          // PartitionKeyColRefArray_ so the user of this object can access
          // the information easier.
          //
          ElemDDLNode * pPartitionByCols = pPartitionByOption->
              castToElemDDLPartitionByColumnList()->getPartitionKeyColumnList();
          NABoolean columnOrderingSpecified = FALSE;
          for (CollIndex index = 0; index < pPartitionByCols->entries(); index++)
          {
            ComASSERT((*pPartitionByCols)[index]->castToElemDDLColRef() NEQ NULL);
            // Report error if ordering was specified for any column.
            if ((*pPartitionByCols)[index]->castToElemDDLColRef()->isColumnOrderingSpecified())
            {
              // It is not allowed to specify ordering for columns of a partitioning key clause.
              // Report an error only once for the column list - not for each offending column.
              if (NOT columnOrderingSpecified)
              {
                *SqlParser_Diags << DgSqlCode(-3199)
                  << DgString0((*pPartitionByCols)[index]->castToElemDDLColRef()->getColumnName());
                columnOrderingSpecified = TRUE;
              }
            }
            partitionKeyColRefArray_.insert(
                         (*pPartitionByCols)[index]->castToElemDDLColRef());
          }
		}
        break;

      default :
        NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
        break;
      }  // switch
    }
  } // if(pPartitionByOption NEQ NULL)


} // StmtDDLCreateTable::setPartitions()

//
// Copies the information in the specified file attributes clause
// to the data member fileAttributes_.
//
void
StmtDDLCreateTable::setFileAttributes(ElemDDLFileAttrClause * pFileAttrClause)
{
  ComASSERT(pFileAttrClause NEQ NULL);

  if (isAttributeClauseSpec_)
  {
    // Duplicate file ATTRIBUTE(S) clauses.
    *SqlParser_Diags << DgSqlCode(-3099);
  }
  isAttributeClauseSpec_ = TRUE;

  ElemDDLNode * pFileAttrs = pFileAttrClause->getFileAttrDefBody();
  ComASSERT(pFileAttrs NEQ NULL);

  for (CollIndex i = 0; i < pFileAttrs->entries(); i++)
  {
    getFileAttributes().setFileAttr((*pFileAttrs)[i]->castToElemDDLFileAttr());
  }

} // StmtDDLCreateTable::setFileAttributes()


void
StmtDDLCreateTable::setMVFileAttributes(ElemDDLMVFileAttrClause * pMVFileAttrClause)
{
  ComASSERT(pMVFileAttrClause NEQ NULL);

  if (isMVFileAttributeClauseSpec_)
  {
    // Duplicate file ATTRIBUTE(S) clauses.
    *SqlParser_Diags << DgSqlCode(-3099);
  }
  isMVFileAttributeClauseSpec_ = TRUE;

  ElemDDLNode * pMVFileAttrs = pMVFileAttrClause->getFileAttrDefBody();
  ComASSERT(pMVFileAttrs NEQ NULL);

  for (CollIndex i = 0; i < pMVFileAttrs->entries(); i++)
  {
    getFileAttributes().setFileAttr((*pMVFileAttrs)[i]->castToElemDDLFileAttr());
  }

} // StmtDDLCreateTable::setFileAttributes()


//
// Constructs the primary partition node and then inserts its pointer
// at the beginning of the partitionArray_.  The kind of the primary
// partition must match that of the secondary partition[s].
//
// This method is only invoked when the partition clause appears.
//
void
StmtDDLCreateTable::setPrimaryPartition(ElemDDLNode * pFirstSecondaryPartition)
{
  if (pFirstSecondaryPartition EQU NULL)  // single partition
  {
    if (getIsHashV1PartitionSpecified() || getIsHashV2PartitionSpecified())
      // The HASH PARTITION or HASH2 PARTITION clause was specified.
      pPrimaryPartition_ = new(PARSERHEAP())ElemDDLPartitionSystem();
    else
      pPrimaryPartition_ = new(PARSERHEAP())ElemDDLPartitionRange();
  }
  else
  switch (pFirstSecondaryPartition->getOperatorType())
  {
  case ELM_PARTITION_RANGE_ELEM :
    pPrimaryPartition_ = new(PARSERHEAP()) ElemDDLPartitionRange();
    break;
  case ELM_PARTITION_SYSTEM_ELEM :
    // We use ElemDDLPartitionSystem parse node for HASH partitions.
    pPrimaryPartition_ = new(PARSERHEAP()) ElemDDLPartitionSystem();
    break;
  //
  // Other future partitioning schemes
  //
  default :
    NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
    break;
  }
  partitionArray_.insert(pPrimaryPartition_);

} // StmtDDLCreateTable::setPrimaryPartition()

void
StmtDDLCreateTable::setTableOption(ElemDDLNode * pTableOption)
{
  ComASSERT(pTableOption NEQ NULL);

  if (pTableOption->castToElemDDLStoreOpt() NEQ NULL)
  {
    if (isStoreByClauseSpec_)
    {
      // Duplicate STORE BY clauses.
      *SqlParser_Diags << DgSqlCode(-3109);
    }
    isStoreByClauseSpec_ = TRUE;

    switch (pTableOption->getOperatorType())
    {
    case ELM_STORE_OPT_DEFAULT_ELEM :
      ComASSERT(pTableOption->castToElemDDLStoreOptDefault() NEQ NULL);
      storeOption_ = COM_UNKNOWN_STORE_OPTION;  // for now
      break;

    case ELM_STORE_OPT_ENTRY_ORDER_ELEM :
      ComASSERT(pTableOption->castToElemDDLStoreOptEntryOrder() NEQ NULL);
      storeOption_ = COM_ENTRY_ORDER_STORE_OPTION;
      break;

    case ELM_STORE_OPT_KEY_COLUMN_LIST_ELEM :
      ComASSERT(pTableOption->castToElemDDLStoreOptKeyColumnList() NEQ NULL);
      storeOption_ = COM_KEY_COLUMN_LIST_STORE_OPTION;
      {
        //
        // Copies array of pointers pointing to Key Column parse nodes
        // keyColumnArray_ so the user of this object can access the
        // information easier.
        //
        ElemDDLNode * pKeyCols = pTableOption->
                castToElemDDLStoreOptKeyColumnList()->getKeyColumnList();
        for (CollIndex index = 0; index < pKeyCols->entries(); index++)
        {
          ComASSERT((*pKeyCols)[index]->castToElemDDLColRef() NEQ NULL);
          keyColRefArray_.insert((*pKeyCols)[index]->castToElemDDLColRef());
        }
      }
      break;

    case ELM_STORE_OPT_NONDROPPABLE_PRIMARY_KEY_ELEM :
      ComASSERT(pTableOption->castToElemDDLStoreOptNondroppablePK() NEQ NULL);
      storeOption_ = COM_NONDROPPABLE_PK_STORE_OPTION;
      break;

    default :
      NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
      break;

    } // switch

    return;   // exist this method

  } // if (pTableOption->castToElemDDLStoreOpt() NEQ NULL)

  switch (pTableOption->getOperatorType())
  {
  case ELM_FILE_ATTR_CLAUSE_ELEM :
    // no needs to check for duplication - the syntax only allows
    // a single file ATTRIBUTES clause.
    //
    setFileAttributes(pTableOption->castToElemDDLFileAttrClause());
    break;
  
  // this is a special MV extension to file attributes
  case ELM_MV_FILE_ATTR_CLAUSE_ELEM :
	setMVFileAttributes(pTableOption->castToElemDDLMVFileAttrClause());
	break;

  case ELM_TABLE_FEATURE_ELEM :
    setTableFeature(pTableOption->castToElemDDLTableFeature());
    break;

  case ELM_LOCATION_ELEM :
    ComASSERT(pTableOption->castToElemDDLLocation() NEQ NULL);
    if (isLocationClauseSpec_)
    {
      // Duplicate LOCATION clauses.
      *SqlParser_Diags << DgSqlCode(-3098);
    }
    isLocationClauseSpec_ = TRUE;
    {
      ElemDDLLocation * pLocation = pTableOption->castToElemDDLLocation();
      locationName_     = pLocation->getLocationName();
      locationNameType_ = pLocation->getLocationNameType();
	  partitionName_    = pLocation->getPartitionName();
    }
    break;

  case ELM_PARTITION_CLAUSE_ELEM :
    // no needs to check for duplication - the syntax only allows
    // a single PARTITION clause.
    //
    setPartitions(pTableOption->castToElemDDLPartitionClause());
    break;

  case ELM_FILE_ATTR_POS_NUM_PARTNS_ELEM:
    if (isPOSNumPartnsSpecified_)
      {
	// Duplicate phrases.
        
        if ((posNumPartns_ == 0) &&
            (pTableOption->castToElemDDLFileAttrPOSNumPartns()
                                                 ->getPOSNumPartns() == 0))
        {
	  *SqlParser_Diags << DgSqlCode(-3421);
        }
        else
        {
          *SqlParser_Diags << DgSqlCode(-3423);
        }
      }
    
    posNumPartns_ = pTableOption->castToElemDDLFileAttrPOSNumPartns()
                                                      ->getPOSNumPartns();

    if ( (isPartitionSpecified())  && (posNumPartns_ == 0) )
    {
      // Conflicting PARTITION clauses.  It is OK to have a partn spec (e.g.
      // "hash2 partitioning") and a partitioning count (e.g. 
      // "number of partitions 2")   But, if "no partitions" is specified 
      // (number of partitions 0) then we don't allow any other partn spec
      *SqlParser_Diags << DgSqlCode(-3103)
                       << DgString0("PARTITION");
    }

    isPOSNumPartnsSpecified_ = TRUE;    


    break;

  case ELM_FILE_ATTR_POS_DISK_POOL_ELEM:
    {
      if (isPOSDiskPoolSpecified_)
        *SqlParser_Diags << DgSqlCode(-3422);

      posDiskPool_ = pTableOption->castToElemDDLFileAttrPOSDiskPool()
                                                        ->getPOSDiskPool();
      posNumDiskPools_ = pTableOption->castToElemDDLFileAttrPOSDiskPool()
                                                    ->getPOSNumDiskPools();

      isPOSDiskPoolSpecified_ = TRUE;
    }
    break;

  case ELM_FILE_ATTR_POS_TABLE_SIZE_ELEM:
    {
      if (pTableOption->castToElemDDLFileAttrPOSTableSize()
                                   ->getPOSInitialTableSize() != -1)
	{
	  if (isPOSInitialTableSizeSpecified_)
	    {
	      // Duplicate TABLE SIZE phrases.
	      *SqlParser_Diags << DgSqlCode(-3407);
	    }
	  posInitialTableSize_ = 
               pTableOption->castToElemDDLFileAttrPOSTableSize()
                                                ->getPOSInitialTableSize();
	  isPOSInitialTableSizeSpecified_ = TRUE;
	}

      if (pTableOption->castToElemDDLFileAttrPOSTableSize()
                                  ->getPOSMaxTableSize() != -1) 
	{
	  if (isPOSMaxTableSizeSpecified_)
	    {
	      // Duplicate TABLE SIZE phrases.
	      *SqlParser_Diags << DgSqlCode(-3407);
	    }
	  posMaxTableSize_ = 
	    pTableOption->castToElemDDLFileAttrPOSTableSize()
                                             ->getPOSMaxTableSize();
	  isPOSMaxTableSizeSpecified_ = TRUE;
	}

      if (pTableOption->castToElemDDLFileAttrPOSTableSize()->getNumRows() != -1) 
	{
	  if (isNumRowsSpecified_)
	    {
	      // Duplicate phrases.
	      *SqlParser_Diags << DgSqlCode(-3407);
	    }
	  numRows_ = 
	    pTableOption->castToElemDDLFileAttrPOSTableSize()->getNumRows();
	  isNumRowsSpecified_ = TRUE;
	}

      if (pTableOption->castToElemDDLFileAttrPOSTableSize()->getIndexLevels() != -1) 
	{
	  // only allowed with InMem table definitions
	  if (NOT isInMemoryObjectDefn())
	    {
	      *SqlParser_Diags << DgSqlCode(-15001);
	      return;
	    }

	  if (isIndexLevelsSpecified_)
	    {
	      // Duplicate phrases.
	      *SqlParser_Diags << DgSqlCode(-3407);
	    }

	  indexLevels_ = 
	    pTableOption->castToElemDDLFileAttrPOSTableSize()->getIndexLevels();
	  isIndexLevelsSpecified_ = TRUE;
	}

      if (pTableOption->castToElemDDLFileAttrPOSTableSize()->getPartnEOF() != -1) 
	{
	  // only allowed with InMem table definitions
	  if (NOT isInMemoryObjectDefn())
	    {
	      *SqlParser_Diags << DgSqlCode(-15001);
	      return;
	    }

	  if (isPartnEOFSpecified_)
	    {
	      // Duplicate phrases.
	      *SqlParser_Diags << DgSqlCode(-3407);
	    }

	  partnEOF_ = 
	    pTableOption->castToElemDDLFileAttrPOSTableSize()->getPartnEOF();
	  isPartnEOFSpecified_ = TRUE;
	}

    }
    break;

  case ELM_FILE_ATTR_POS_IGNORE_ELEM:
    {
      if (isPOSIgnoreSpecified_)
        *SqlParser_Diags << DgSqlCode(-3090);

      posIgnore_ = 
        pTableOption->castToElemDDLFileAttrPOSIgnore()->getIgnorePOS();

      isPOSIgnoreSpecified_ = TRUE;
    }
    break;
  case ELM_DIVISION_CLAUSE_ELEM:
  case ELM_HBASE_OPTIONS_ELEM:
    // Do nothing here.
    // We already performed the initialization in StmtDDLCreateTable::synthesize().
    break;
  case ELM_SALT_OPTIONS_ELEM:
    {
      if (pSaltOptions_)
        // Error 3183 - Duplicate $0~string0 clauses were specified.
        *SqlParser_Diags << DgSqlCode(-3183) << DgString0("SALT");
      else
        pSaltOptions_ = pTableOption->castToElemDDLSaltOptionsClause();
    }
    break;
  default :
    NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
    break;

  } // switch

} // StmtDDLCreateTable::setTableOption()


//
// methods for tracing
//

const NAString
StmtDDLCreateTable::displayLabel1() const
{
  return NAString("Table name: ") + getTableName();
}

NATraceList
StmtDDLCreateTable::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  //
  // table name
  //

  detailTextList.append(displayLabel1());

  //
  // Like clause
  //

  if (getIsLikeOptionSpecified())
  {
    //
    //  ParDDLLikeOptsCreateTable  from getLikeOptions()
    //
    detailTextList.append("LIKE option specified.");
    //
    // Does nothing for now because Like clause is not
    // support in the first release.  Exits this method.
    //
    return detailTextList;
  }

  //
  // Display information about column definitions
  //

  const ElemDDLColDefArray & colsList = getColDefArray();
  CollIndex i, nbrCols = colsList.entries();

  if (nbrCols EQU 0)
  {
    //
    // list of column definitions is required
    // unless Like clause appears.
    //
    detailTextList.append("No columns.");
  }
  else
  {
    detailText = "Column list [";
    detailText += LongToNAString((Lng32)nbrCols);
    detailText += " column(s)]:";
    detailTextList.append(detailText);

    ElemDDLColDef * col;
    for (i = 0; i < nbrCols; i++)
    {
      col = colsList[i];

      detailText = "[column ";
      detailText += LongToNAString((Lng32) i);
      detailText += "]";
      detailTextList.append(detailText);

      detailTextList.append("    ", col->getDetailInfo());
    }
  } // else (nbrCols NEQ 0) of column definitions

  //
  // check constraints
  //

  const StmtDDLAddConstraintCheckArray & checkConstraintList
    = getAddConstraintCheckArray();
  CollIndex nbrConstraints = checkConstraintList.entries();

  if (nbrConstraints EQU 0)
  {
    detailTextList.append("No check constraints.");
  }
  else
  {
    detailText = "Check constraint list [";
    detailText += LongToNAString((Lng32)nbrConstraints);
    detailText += " element(s)]:";
    detailTextList.append(detailText);

    StmtDDLAddConstraintCheck * checkConstraint;
    for (i = 0; i < nbrConstraints; i++)
    {
      checkConstraint = checkConstraintList[i];

      detailText = "[check constraint ";
      detailText += LongToNAString((Lng32) i);
      detailText += "]";
      detailTextList.append(detailText);

      detailTextList.append("    ", checkConstraint->getDetailInfo());
    }
  } // else (nbrConstraints NEQ 0) of check constraints

  //
  // primary key (column or table) constraint
  //

  detailText = "is primary key constraint specified? ";
  detailText += YesNo(getIsConstraintPKSpecified());
  detailTextList.append(detailText);

  if (getIsConstraintPKSpecified())
  {
    const ElemDDLColRefArray & keysList = getPrimaryKeyColRefArray();
    nbrCols = keysList.entries();
    if (nbrCols EQU 0)
    {
      detailTextList.append("No primary key list.");
    }
    else
    {
      detailText = "Primary key column list [";
      detailText += LongToNAString((Lng32)nbrCols);
      detailText += " element(s)]:";
      detailTextList.append(detailText);

      ElemDDLColRef * keyCol;
      for (i = 0; i < nbrCols; i++)
      {
        keyCol = keysList[i];

        detailText = "[primary key column ";
        detailText += LongToNAString((Lng32)i);
        detailText += "]";
        detailTextList.append(detailText);

        detailTextList.append("    ", keyCol->getDetailInfo());
      } //  for (i = 0; i < nbrCols; i++)
    } // else ((nbrCols EQU 0)
  } // if (getIsConstraintPKSpecified())

  //
  // referential integrity constraints
  //

  const StmtDDLAddConstraintRIArray & rIConstraintList
    = getAddConstraintRIArray();
  nbrConstraints = rIConstraintList.entries();

  if (nbrConstraints EQU 0)
  {
    detailTextList.append("No referential integrity constraints.");
  }
  else
  {
    detailText = "Referential integrity constraint list [";
    detailText += LongToNAString((Lng32)nbrConstraints);
    detailText += " element(s)]:";
    detailTextList.append(detailText);

    StmtDDLAddConstraintRI * rIConstraint;
    for (i = 0; i < nbrConstraints; i++)
    {
      rIConstraint = rIConstraintList[i];

      detailText = "[referential integrity constraint ";
      detailText += LongToNAString((Lng32) i);
      detailText += "]";
      detailTextList.append(detailText);

      detailTextList.append("    ", rIConstraint->getDetailInfo());
    }
  } // else (nbrConstraints NEQ 0) of referential integrity constraints

  //
  // unique constraints
  //

  const StmtDDLAddConstraintUniqueArray & uniqueConstraintList
    = getAddConstraintUniqueArray();
  nbrConstraints = uniqueConstraintList.entries();

  if (nbrConstraints EQU 0)
  {
    detailTextList.append("No unique constraints.");
  }
  else
  {
    detailText = "Unique constraint list [";
    detailText += LongToNAString((Lng32)nbrConstraints);
    detailText += " element(s)]:";
    detailTextList.append(detailText);

    StmtDDLAddConstraintUnique * uniqueConstraint;
    for (i = 0; i < nbrConstraints; i++)
    {
      uniqueConstraint = uniqueConstraintList[i];

      detailText = "[unique constraint ";
      detailText += LongToNAString((Lng32) i);
      detailText += "]";
      detailTextList.append(detailText);

      detailTextList.append("    ", uniqueConstraint->getDetailInfo());
    }
  } // else (nbrConstraints NEQ 0) of unique constraints

  //
  // Store By clause
  //

  if (isStoreBySpecified())
  {
    switch (getStoreOption())
    {
    case COM_ENTRY_ORDER_STORE_OPTION :
      detailTextList.append("Entry Order store option specified");
      break;

    case COM_KEY_COLUMN_LIST_STORE_OPTION :
      detailTextList.append("Key Column List store option specified");
      {
        const ElemDDLColRefArray & keyCols = getKeyColumnArray();
        nbrCols = keyCols.entries();
        ComASSERT(nbrCols NEQ 0);
        detailText = "key column list in Store By clause[";
        detailText += LongToNAString((Lng32)nbrCols);
        detailText += " element(s)]:";
        detailTextList.append(detailText);

        ElemDDLColRef * keyCol;
        for (i = 0; i < nbrCols; i++)
        {
          keyCol = keyCols[i];

          detailText = "[key column ";
          detailText += LongToNAString((Lng32)i);
          detailText += "]";
          detailTextList.append(detailText);

          detailTextList.append("    ", keyCol->getDetailInfo());
        } // for (i = 0; i < nbrCols; i++)
      }
      break;

    case COM_NONDROPPABLE_PK_STORE_OPTION :
      detailTextList.append("Nondroppable Primary Key store option specified");
      break;

    default :
      NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
      break;

    } // switch
  } // if (isStoreBySpecified())
  else
  {
    detailTextList.append("Store By clause not specified.");
  } // else (NOT isStoreBySpecified())

  //
  // file attributes
  //

  detailTextList.append("File attributes: ");

  ParDDLFileAttrsCreateTable fileAttribs  = getFileAttributes();
  detailTextList.append("    ", fileAttribs.getDetailInfo());

  //
  // partitions
  //

  CollIndex nbrParts;
  const ElemDDLPartitionArray & partsList = getPartitionArray();
  ElemDDLPartitionSystem * part;

  nbrParts = partsList.entries();
  if (nbrParts EQU 0)
  {
    detailTextList.append("No partitions.");
  }
  else
  {
    detailText = "Partition list [";
    detailText += LongToNAString((Lng32)nbrParts);
    detailText += " element(s)]: ";
    detailTextList.append(detailText);

    for (i = 0; i < nbrParts; i++)
    {
      part = partsList[i]->castToElemDDLPartitionSystem();

      if (part EQU NULL)
      {
        //
        // Note that class ElemDDLPartitionRange is
        // derived from class ElemDDLPartitionSystem
        //
        // Only support range and system partitioning.
        *SqlParser_Diags << DgSqlCode(-3105);

        NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
      }

      detailText = "[partition ";
      detailText += LongToNAString((Lng32)i);
      detailText += "]";
      detailTextList.append(detailText);

      detailTextList.append("    ", part->getDetailInfo());
    } //for (j = 0; j < nbrParts; j++)
  } // else (nbrParts NEQ 0)

  return detailTextList;

} // StmtDDLCreateTable::getDetailInfo()

const NAString
StmtDDLCreateTable::getText() const
{
  return "StmtDDLCreateTable";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLCreateTableArray
// -----------------------------------------------------------------------

// virtual destructor
StmtDDLCreateTableArray::~StmtDDLCreateTableArray()
{
}

// -----------------------------------------------------------------------
// methods for class StmtDDLCreateHbaseTable
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLCreateHbaseTable::StmtDDLCreateHbaseTable(const QualifiedName & aTableQualName,
						 ConstStringList * csl,
						 ElemDDLHbaseOptions *	hbaseOptions,	      
						 CollHeap    * heap)
        : StmtDDLNode(DDL_CREATE_HBASE_TABLE),
          origTableQualName_(heap),
          tableQualName_(aTableQualName, heap),
	  csl_(csl),
	  pHbaseOptionsParseNode_(hbaseOptions)
{

}

StmtDDLCreateHbaseTable::~StmtDDLCreateHbaseTable()
{
}

//
// cast virtual function
//
StmtDDLCreateHbaseTable *
StmtDDLCreateHbaseTable::castToStmtDDLCreateHbaseTable()
{
  return this;
}

const NAString
StmtDDLCreateHbaseTable::displayLabel1() const
{
  return NAString("Table name: ") + getTableName();
}

NATraceList
StmtDDLCreateHbaseTable::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  //
  // table name
  //

  detailTextList.append(displayLabel1());

  return detailTextList;

} // StmtDDLCreateHbaseTable::getDetailInfo()

const NAString
StmtDDLCreateHbaseTable::getText() const
{
  return "StmtDDLCreateHbaseTable";
}


// -----------------------------------------------------------------------
// methods for class StmtDDLCreateMV
// -----------------------------------------------------------------------
// 
//  Internal helper routine
// 



//
// constructor
//
StmtDDLCreateMV::StmtDDLCreateMV( const QualifiedName	&MVQualName,
				  const ParNameLocList	&nameLocList,
				  ElemDDLNode		*optionalMVColumnList,
				  ComMVRefreshType	 refreshType,
				  ElemDDLNode		*pAttributeTableLists, 
				  //ComMVStatus            mvStatus,
                                  MvInitializationType   mvInitType,
				  ComBoolean		 isRewriteEnabled,
				  ElemDDLNode		*optionalFileOptionsClause,
				  RelExpr		*queryExpression,
                                  ElemDDLNode           *pOptionalOwner,
				  CollHeap		*heap)
  : MVQualName_(MVQualName, heap),
    refreshType_(refreshType),
    columnDefArray_(heap),
    nameLocList_(nameLocList, heap),
    startPos_(nameLocList.getTextStartPosition()),
    endPos_(0), // set later by ParSetTextEndPos()
    theMVInfo_(NULL),     
    viewUsages_(heap),
    isAttrClauseSpecified_(FALSE),
    isLocationClauseSpecified_(FALSE), 
    isPartitionDefinitionSpecified_(FALSE), 
    isPartitionByClauseSpecified_(FALSE), 
    isDivisionByClauseSpecified_(FALSE),
    pDivisionByClauseParseNode_(NULL),
    isStoreByClauseSpecified_(FALSE), 
    isMVAttributeClauseSpecified_(FALSE),
    mvAuditType_(COM_MV_AUDIT),
    commitEachNRows_(0),
    pMVColumnList_(optionalMVColumnList),
    pFileOptions_(optionalFileOptionsClause),
    pStoreByOption_(NULL),
    pQueryExpression_(queryExpression),
    pAttributeTableLists_(pAttributeTableLists),
    pIgnoreChangesList_(0),
    StmtDDLNode(DDL_CREATE_MV),
    partitionKeyColRefArray_(heap),
    udfList_(heap)
{
  // MV file options are a subset of Table's file options.
  // Since we use the same parsing rule as a Table's, we need to check that all file 
  // options are allowed.
  ensureCorrectMVFileOptions();
  extractMvAttributesFromFileOptions();

  ComMVStatus mvStatus = COM_MVSTATUS_UNKNOWN;
  switch (mvInitType)
  {
    case MVINIT_ON_CREATE: 
      mvStatus = COM_MVSTATUS_INITIALIZED;
      break;
    case MVINIT_ON_REFRESH: 
      mvStatus = COM_MVSTATUS_NOT_INITIALIZED;
      break;
    case MVINIT_NO_INIT: 
    case MVINIT_BY_USER: 
      mvStatus = COM_MVSTATUS_NO_INITIALIZATION;
      break;
  }

  theMVInfo_ = new(heap) 
  MVInfoForDDL(MVQualName_.getQualifiedNameAsAnsiString(),
	       refreshType,
	       isRewriteEnabled,
	       mvStatus,
	       nameLocList.getInputStringPtr(),
	       nameLocList.getInputStringCharSet(),
	       pStoreByOption_,
	       heap );

  // set up owner info
  if (pOptionalOwner)
  {
    ElemDDLNode *pTemp = pOptionalOwner;
    ComASSERT(pTemp NEQ NULL);
    pOwner_ = pTemp->castToElemDDLGrantee();
    ComASSERT(pOwner_ NEQ NULL);
  }
}

//
// virtual destructor
//
StmtDDLCreateMV::~StmtDDLCreateMV()
{
  // delete all children
  for (Int32 i = 0; i < getArity(); i++)
  {
    delete getChild(i);
  }
}

//
// cast
//
StmtDDLCreateMV *
StmtDDLCreateMV::castToStmtDDLCreateMV()
{
  return this;
}

//
// accessors
//

Int32
StmtDDLCreateMV::getArity() const
{
  return MAX_STMT_DDL_CREATE_MV_ARITY;
}

ExprNode *
StmtDDLCreateMV::getChild(Lng32 index)
{
  switch (index)
  {
  case INDEX_MV_COLUMN_LIST :
    return pMVColumnList_;
  case INDEX_FILE_OPTIONS_CLAUSE :
    return pFileOptions_;
  case INDEX_QUERY_EXPRESSION :
    return pQueryExpression_;
  default :
    ABORT("internal logic error");
    return 0;
  }
}

//
// mutators
//

void
StmtDDLCreateMV::incrCurViewColNum()
{
  if (isProcessingViewColList())
  {
    setCurViewColNum(getCurViewColNum()+(CollIndex)1);
  }
  else
  {
    setCurViewColNum((CollIndex)0);
  }
}

void
StmtDDLCreateMV::setChild(Lng32 index, ExprNode * pChildNode)
{
  switch (index)
  {
  case INDEX_MV_COLUMN_LIST :
    if (pChildNode NEQ NULL)
    {
      pMVColumnList_ = pChildNode->castToElemDDLNode();
      ComASSERT(pMVColumnList_ NEQ NULL);
    }
    else
    {
      pMVColumnList_ = NULL;
    }
    break;

  case INDEX_FILE_OPTIONS_CLAUSE :
    if (pChildNode NEQ NULL)
    {
      pFileOptions_ = pChildNode->castToElemDDLNode();
      ComASSERT(pFileOptions_ NEQ NULL);
    }
    else
    {
      pFileOptions_ = NULL;
    }
    break;

  case INDEX_QUERY_EXPRESSION :
    if (pChildNode NEQ NULL)
    {
      pQueryExpression_ = pChildNode->castToRelExpr();
      ComASSERT(pQueryExpression_ NEQ NULL);
    }
    else
    {
      pQueryExpression_ = NULL;
    }
    break;

  default :
    ABORT("internal logic error");
  }
}

void
StmtDDLCreateMV::synthesize()
{
	//
	// Collects pointers to column name parser nodes for easier access
	//
	
	if (pMVColumnList_ NEQ NULL)
	{
		for (Int32 i = 0; i < (Int32)pMVColumnList_->entries(); i++)
		{
			ComASSERT((*pMVColumnList_)[i] NEQ NULL);
			columnDefArray_.insert((*pMVColumnList_)[i]->
                             castToElemDDLColViewDef());
		}
	}

	processAttributeTableLists();

} // StmtDDLCreateMV::synthesize()

NABoolean
StmtDDLCreateMV::statementHasAttributeTableLists()
{
	return (NULL != pAttributeTableLists_);
}


void
StmtDDLCreateMV::processAttributeTableLists()
{
	if(!statementHasAttributeTableLists())

	{
		return;
	}
	
	if (COM_ON_REQUEST != refreshType_)
	{
		// no changes clause for recompute mvs
		*SqlParser_Diags << DgSqlCode(-12043); 
	}

	NABoolean ignoreChangesListSpecified = FALSE;

	for ( CollIndex i = 0 ; i < pAttributeTableLists_->entries() ; i++)
	{
		
		ElemDDLCreateMVOneAttributeTableList * pAttributeList = 
			(*pAttributeTableLists_)[i]->
								castToElemDDLCreateMVOneAttributeTableList();		
		
		ComASSERT(NULL NEQ pAttributeList);

		if (COM_IGNORE_CHANGES == pAttributeList->getType())
		{
			if (ignoreChangesListSpecified)
			{
				// Duplicate IGNORE CHANGES clause.
				*SqlParser_Diags << DgSqlCode(-12044); 
			}
			else
			{
				ignoreChangesListSpecified = TRUE;
			}

			pIgnoreChangesList_ = pAttributeList;
		}
		else
		{
			// Fatal Error in attribute table lists - list type unknown.
			*SqlParser_Diags << DgSqlCode(-12046); 
		}
	}
}


//----------------------------------------------------------------------------
void
StmtDDLCreateMV::ensureCorrectMVFileOptions()
{

  if (getChild(INDEX_FILE_OPTIONS_CLAUSE) NEQ NULL)
  {
    ElemDDLNode *pCreateTableAttrList =
	    getChild(INDEX_FILE_OPTIONS_CLAUSE)->castToElemDDLNode();

    ComASSERT(pCreateTableAttrList NEQ NULL);
  
    if (pCreateTableAttrList->castToElemDDLOptionList() NEQ NULL)
    {
      for (CollIndex i = 0; i < pCreateTableAttrList->entries(); i++)
      {
	checkFileOption((*pCreateTableAttrList)[i]);
      }
    }
    else
    {
      // pCreateTableAttrList points to a single Create Table
      // option (e.g., a partition clause, a file attributes
      // clause, a location clause, etc.
      checkFileOption(pCreateTableAttrList);
    }
  } // if (pCreateTableAttrList NEQ NULL)

  return;
}


//----------------------------------------------------------------------------
void
StmtDDLCreateMV::checkFileOption(ElemDDLNode * pTableOption)
{
  ComASSERT(pTableOption NEQ NULL);

  // STORE BY
  if (pTableOption->castToElemDDLStoreOpt() NEQ NULL)
  {
    checkStoreByClause(pTableOption->castToElemDDLStoreOpt());
    pStoreByOption_ = pTableOption->castToElemDDLStoreOpt()
				      ->castToElemDDLStoreOptKeyColumnList();
  }
  //ATTRIBUTES
  else if (pTableOption->castToElemDDLFileAttrClause() NEQ NULL)
  {
    checkFileAttributeClause(pTableOption->castToElemDDLFileAttrClause());
  }
  // LOCATION
  else if (pTableOption->castToElemDDLLocation() NEQ NULL)
  {
    checkLocationClause(pTableOption->castToElemDDLLocation());
  }
  // DIVISION BY
  else if (pTableOption->castToElemDDLDivisionClause() NEQ NULL)
  {
    checkDivisionByClause(pTableOption->castToElemDDLDivisionClause());
  }
  // PARTITION
  else if (pTableOption->castToElemDDLPartitionClause() NEQ NULL)
  {
    checkPartitionDefinitionclause(
				pTableOption->castToElemDDLPartitionClause());

  }
  // MVATTRIBUTES
  else if (pTableOption->castToElemDDLMVFileAttrClause() NEQ NULL)
  {
    checkMVFileAttributeClause(pTableOption->castToElemDDLMVFileAttrClause());

  }
   
  else // error
  {
    switch (pTableOption->getOperatorType())
    {
      case ELM_FILE_ATTR_POS_NUM_PARTNS_ELEM:
      case ELM_FILE_ATTR_POS_TABLE_SIZE_ELEM:
      case ELM_FILE_ATTR_POS_DISK_POOL_ELEM:
      {
        *SqlParser_Diags << DgSqlCode (-3424);
        break;
      }
      default :
        NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");

    }
  }

} // checkFileOption 


//----------------------------------------------------------------------------
void
StmtDDLCreateMV::checkStoreByClause(ElemDDLStoreOpt *pTableOption)
{
	if (isStoreByClauseSpecified_)
	{
	  // Duplicate STORE BY clauses.
	  *SqlParser_Diags << DgSqlCode(-3109);
	}
	
	isStoreByClauseSpecified_ = TRUE;
	
	
	switch (pTableOption->getOperatorType())
    {
    // ALLOWED

    case ELM_STORE_OPT_KEY_COLUMN_LIST_ELEM :
		ComASSERT(pTableOption->castToElemDDLStoreOptKeyColumnList() NEQ NULL);
		break;
		
	// NOT ALLOWED
	case ELM_STORE_OPT_DEFAULT_ELEM :
	  *SqlParser_Diags << DgSqlCode(-12040); 
		break;

	case ELM_STORE_OPT_ENTRY_ORDER_ELEM:
	  *SqlParser_Diags << DgSqlCode(-12041); 
	  break;

	case ELM_STORE_OPT_NONDROPPABLE_PRIMARY_KEY_ELEM:
	  *SqlParser_Diags << DgSqlCode(-12042); 
	  break;
    
    default :
      NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
      break;

    } // switch

    return;   // exist this method

} // checkStoreByClause

//----------------------------------------------------------------------------
void
StmtDDLCreateMV::checkFileAttributeClause(ElemDDLFileAttrClause *pTableOption)
{
  if (isAttrClauseSpecified_)
  {
    // Duplicate file ATTRIBUTE(S) clauses.
    *SqlParser_Diags << DgSqlCode(-3099);
  }
  isAttrClauseSpecified_ = TRUE;

  ElemDDLNode * pFileAttrs = pTableOption->getFileAttrDefBody();
  ComASSERT(pFileAttrs NEQ NULL);

  for (CollIndex i = 0; i < pFileAttrs->entries(); i++)
  {
    checkFileAttribute((*pFileAttrs)[i]->castToElemDDLFileAttr());
  }
}


//----------------------------------------------------------------------------
void 
StmtDDLCreateMV::checkFileAttribute(ElemDDLFileAttr * pFileAttr)
{
  
	switch (pFileAttr->getOperatorType())
	{

	// ALLOWED
	case ELM_FILE_ATTR_AUDIT_COMPRESS_ELEM	:
	case ELM_FILE_ATTR_BLOCK_SIZE_ELEM	:
	case ELM_FILE_ATTR_BUFFERED_ELEM	:
	case ELM_FILE_ATTR_CLEAR_ON_PURGE_ELEM	:
        case ELM_FILE_ATTR_COMPRESSION_ELEM     :
	case ELM_FILE_ATTR_D_COMPRESS_ELEM	:
	case ELM_FILE_ATTR_I_COMPRESS_ELEM	:
	case ELM_FILE_ATTR_MAX_SIZE_ELEM	:
	case ELM_FILE_ATTR_LOCK_ON_REFRESH_ELEM	:
	case ELM_FILE_ATTR_MVS_ALLOWED_ELEM	:	
        case ELM_FILE_ATTR_EXTENT_ELEM          :
        case ELM_FILE_ATTR_MAXEXTENTS_ELEM      :
        case ELM_FILE_ATTR_ROW_FORMAT_ELEM      :
        case ELM_FILE_ATTR_OWNER_ELEM           :
		break;	

	// NOT ALLOWED
	case ELM_FILE_ATTR_ALLOCATE_ELEM       :
		*SqlParser_Diags << DgSqlCode(-12047); 
		break;

	case ELM_FILE_ATTR_AUDIT_ELEM :
		*SqlParser_Diags << DgSqlCode(-12050); 
		break;

	case ELM_FILE_ATTR_DEALLOCATE_ELEM : 
		*SqlParser_Diags << DgSqlCode(-12051); 
		break;

	case ELM_FILE_ATTR_RANGE_LOG_ELEM:
		*SqlParser_Diags << DgSqlCode(-12062); 
		break;

	case ELM_FILE_ATTR_INSERT_LOG_ELEM:
		*SqlParser_Diags << DgSqlCode(-12063); 
		break;

	case ELM_FILE_ATTR_MV_COMMIT_EACH_ELEM: // cannot be
	case ELM_FILE_ATTR_MVAUDIT_ELEM: // cannot be
	default :
		NAAbort("StmtDDLCreate.CPP", __LINE__, "internal logic error");
		break;
	}



}

//----------------------------------------------------------------------------
void
StmtDDLCreateMV::checkLocationClause(ElemDDLLocation *pTableOption)
{
    if (isLocationClauseSpecified_)
    {
      // Duplicate LOCATION clauses.
      *SqlParser_Diags << DgSqlCode(-3098);
    }
    isLocationClauseSpecified_ = TRUE;

    if (NOT pTableOption->getPartitionName().isNull())
        *SqlParser_Diags << DgSqlCode(-3405);
}

//----------------------------------------------------------------------------
void StmtDDLCreateMV::checkDivisionByClause(ElemDDLDivisionClause *pDivisionByParseNode)
{
  if (isDivisionByClauseSpecified_)
  {
    // Error 3183 - Duplicate $0~string0 clauses were specified.
    *SqlParser_Diags << DgSqlCode(-3183)
                     << DgString0("DIVISION BY");
  }
  pDivisionByClauseParseNode_ = pDivisionByParseNode;
  isDivisionByClauseSpecified_ = TRUE;
}

//----------------------------------------------------------------------------
void
StmtDDLCreateMV::checkPartitionDefinitionclause(
					 ElemDDLPartitionClause *pTableOption)
{
  if (isPartitionDefinitionSpecified_ || isPartitionByClauseSpecified_)
  {
    // Duplicate PARTITION clauses.
    *SqlParser_Diags << DgSqlCode(-3103)
                     << DgString0("PARTITION");
  }
  isPartitionDefinitionSpecified_ = TRUE;

  //
  // Initializes pPartitionByOption to point to the parse node
  // representing the definition of partition by columns (class
  // ElemDDLPartitionByOpt).
  //

  ElemDDLNode *pPartitionByOption = pTableOption->getPartitionByOption();

  if(pPartitionByOption NEQ NULL)
  {
    // If the Partition By Option is specified, set the flag and copy
    // the pointers to column parse nodes to partitionKeyColRefArray_.
    if(pPartitionByOption->castToElemDDLPartitionByOpt() NEQ NULL)
    {
      isPartitionByClauseSpecified_ = TRUE;

      switch (pPartitionByOption->getOperatorType())
      {
      case ELM_PARTITION_BY_COLUMN_LIST_ELEM :
        ComASSERT(pPartitionByOption->castToElemDDLPartitionByColumnList()
                                                                     NEQ NULL);
                {
          //
          // Copies array of pointers pointing to Column parse nodes to
          // PartitionKeyColRefArray_ so the user of this object can access
          // the information easier.
          //
          ElemDDLNode * pPartitionByCols = pPartitionByOption->
              castToElemDDLPartitionByColumnList()->getPartitionKeyColumnList();
          NABoolean columnOrderingSpecified = FALSE;
          for (CollIndex index = 0; index < pPartitionByCols->entries(); index++)
          {
            ComASSERT((*pPartitionByCols)[index]->castToElemDDLColRef() NEQ NULL);
            // Report error if ordering was specified for any column.
            if ((*pPartitionByCols)[index]->castToElemDDLColRef()->isColumnOrderingSpecified())
            {
              // It is not allowed to specify ordering for columns of a partitioning key clause.
              // Report an error only once for the column list - not for each offending column.
              if (NOT columnOrderingSpecified)
              {
                *SqlParser_Diags << DgSqlCode(-3199)
                  << DgString0((*pPartitionByCols)[index]->castToElemDDLColRef()->getColumnName());
                columnOrderingSpecified = TRUE;
              }
            }
            partitionKeyColRefArray_.insert(
                         (*pPartitionByCols)[index]->castToElemDDLColRef());
          }
                }
        break;

      default :
        NAAbort("StmtDDLCreate.C", __LINE__, "internal logic error");
        break;
      }  // switch
    }
  } // if(pPartitionByOption NEQ NULL)

}

//----------------------------------------------------------------------------
void
StmtDDLCreateMV::checkMVFileAttributeClause(
					ElemDDLMVFileAttrClause *pTableOption)
{
  if(TRUE == isMVAttributeClauseSpecified_)
  {
    // Duplicate MVATTRIBUTES clauses.
    *SqlParser_Diags << DgSqlCode(-12060);
  }
  isMVAttributeClauseSpecified_ = TRUE;
}


//----------------------------------------------------------------------------
void StmtDDLCreateMV::extractMvAttributesFromFileOptions()
{
	if(FALSE == isMVAttributeClauseSpecified_ || 
		NULL == getChild(INDEX_FILE_OPTIONS_CLAUSE) )
	{
		return;
	}

	ElemDDLNode *pCreateTableAttrList =
					getChild(INDEX_FILE_OPTIONS_CLAUSE)->castToElemDDLNode();

	ComASSERT(pCreateTableAttrList NEQ NULL);

	ElemDDLMVFileAttrClause* pAttribs = NULL;
		
	if (pCreateTableAttrList->castToElemDDLOptionList() NEQ NULL)
	{
		for (CollIndex i = 0; i < pCreateTableAttrList->entries(); i++)
		{
			if (NULL != 
				((*pCreateTableAttrList)[i])->castToElemDDLMVFileAttrClause())
			{
				pAttribs = ((*pCreateTableAttrList)[i])->
											castToElemDDLMVFileAttrClause();
			}
		}
	}
	else
	{
		// pCreateTableAttrList points to a single Create Table
		// option. this has to be the MVATTRIBUTE CLAUSE.
		pAttribs = pCreateTableAttrList->castToElemDDLMVFileAttrClause();
		ComASSERT(NULL != pAttribs);
	}


	ComASSERT(NULL != pAttribs);
	ElemDDLNode * pFileAttrs = pAttribs ->getFileAttrDefBody();
	ComASSERT(pFileAttrs NEQ NULL);

	NABoolean mvAuditSpecified(FALSE);
	NABoolean commitEachSpecified(FALSE);
	
	for (CollIndex j = 0; j < pFileAttrs->entries(); j++)
	{
		ElemDDLFileAttr* pAttr = 
				(*pFileAttrs)[j]->castToElemDDLFileAttr();

		switch (pAttr->getOperatorType())
		{
		case ELM_FILE_ATTR_MVAUDIT_ELEM:
			if(TRUE == mvAuditSpecified)
			{
				*SqlParser_Diags << DgSqlCode(-12061);
			}
	
			mvAuditType_ = 
				(pAttr->castToElemDDLFileAttrMvAudit())
				->getMvAuditType();

			mvAuditSpecified = TRUE;
			break;

		case ELM_FILE_ATTR_MV_COMMIT_EACH_ELEM:

			if(TRUE == commitEachSpecified)
			{
				*SqlParser_Diags << DgSqlCode(-12059);
			}

			commitEachNRows_ = 
				(pAttr->castToElemDDLFileAttrMVCommitEach())
											->getNRows();
			commitEachSpecified = TRUE;
			break;
		}
	}
	return;
} // StmtDDLCreateMV::extractMvAttributesFromFileOptions



//
// methods for tracing
//

const NAString
StmtDDLCreateMV::displayLabel1() const
{
  return NAString("MV name: ") + getMVName();
}

NATraceList
StmtDDLCreateMV::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailTextList.append(displayLabel1());    // display name of defined view

  //
  // displays list of column names
  //

  CollIndex i, nbrCols;
  const ElemDDLColViewDefArray & colsList = getMVColDefArray();
  ElemDDLColViewDef * col;

  nbrCols = colsList.entries();

  if (nbrCols EQU 0)
  {
    detailTextList.append("No columns.");
  }
  else
  {
    detailText = "Column list [";
    detailText += LongToNAString((Lng32)nbrCols);
    detailText += " column(s)]:";
    detailTextList.append(detailText);

    for (i = 0; i < nbrCols; i++)
    {
      col = colsList[i];

      detailText = "[column ";
      detailText += LongToNAString((Lng32) i);
      detailText += "]";
      detailTextList.append(detailText);

      detailTextList.append("    ", col->getDetailInfo());
    }
  } // else (nbrCols EQU 0) of column references


  return detailTextList;
}

const NAString
StmtDDLCreateMV::getText() const
{
  return "StmtDDLCreateMV";
}


// -----------------------------------------------------------------------
// methods for class StmtDDLCreateView
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLCreateView::StmtDDLCreateView(const QualifiedName & viewQualName,
                                     const ParNameLocList & nameLocList,
                                     ElemDDLNode * optionalViewColumnList,
                                     ElemDDLNode * optionalLocationClause,
                                     RelExpr     * queryExpression,
                                     ElemDDLNode * optionalWithCheckOption,
                                     ComCreateViewBehavior  createViewBehavior,
                                     ElemDDLNode *pOwner,                                    
                                     CollHeap    * heap)
        : StmtDDLNode(DDL_CREATE_VIEW),
          viewQualName_(viewQualName, heap),
	  isUpdatable_(FALSE),
	  isInsertable_(FALSE),
          nameLocList_(nameLocList, heap),
          startPos_(nameLocList.getTextStartPosition()),
          endPos_(0),  // will be set later by function ParSetTextEndPos()
          isLocationClauseSpec_(FALSE),
          locationName_(heap),
          locationNameType_(ElemDDLLocation::LOCATION_DEFAULT_NAME_TYPE),
          createViewBehavior_(createViewBehavior),
          checkOptionLevel_(COM_UNKNOWN_LEVEL),
          pViewColumnList_(optionalViewColumnList),
          pLocationClause_(optionalLocationClause),
          pQueryExpression_(queryExpression),
          pWithCheckOption_(optionalWithCheckOption),
          columnDefArray_(heap),
          viewUsages_(heap),
	  udfList_(heap),
          createIfNotExists_(FALSE)
{
  setChild(INDEX_VIEW_OWNER, pOwner);
}

//
// virtual destructor
//
StmtDDLCreateView::~StmtDDLCreateView()
{
  // delete all children
  for (Int32 i = 0; i < getArity(); i++)
  {
    delete getChild(i);
  }
}

//
// cast
//
StmtDDLCreateView *
StmtDDLCreateView::castToStmtDDLCreateView()
{
  return this;
}

//
// accessors
//

Int32
StmtDDLCreateView::getArity() const
{
  return MAX_STMT_DDL_CREATE_VIEW_ARITY;
}

ExprNode *
StmtDDLCreateView::getChild(Lng32 index)
{
  switch (index)
  {
  case INDEX_VIEW_COLUMN_LIST :
    return pViewColumnList_;
  case INDEX_LOCATION_CLAUSE :
    return pLocationClause_;
  case INDEX_QUERY_EXPRESSION :
    return pQueryExpression_;
  case INDEX_WITH_CHECK_OPTION :
    return pWithCheckOption_;
  case INDEX_VIEW_OWNER :
    return pOwner_;
  default :
    ABORT("internal logic error");
    return 0;
  }
}

//
// mutators
//

void
StmtDDLCreateView::incrCurViewColNum()
{
  if (isProcessingViewColList())
  {
    setCurViewColNum(getCurViewColNum()+(CollIndex)1);
  }
  else
  {
    setCurViewColNum((CollIndex)0);
  }
}

void
StmtDDLCreateView::setChild(Lng32 index, ExprNode * pChildNode)
{
  switch (index)
  {
  case INDEX_VIEW_COLUMN_LIST :
    if (pChildNode NEQ NULL)
    {
      pViewColumnList_ = pChildNode->castToElemDDLNode();
      ComASSERT(pViewColumnList_ NEQ NULL);
    }
    else
    {
      pViewColumnList_ = NULL;
    }
    break;

  case INDEX_LOCATION_CLAUSE :
    if (pChildNode NEQ NULL)
    {
      pLocationClause_ = pChildNode->castToElemDDLNode();
      ComASSERT(pLocationClause_ NEQ NULL);
    }
    else
    {
      pLocationClause_ = NULL;
    }
    break;

  case INDEX_QUERY_EXPRESSION :
    if (pChildNode NEQ NULL)
    {
      pQueryExpression_ = pChildNode->castToRelExpr();
      ComASSERT(pQueryExpression_ NEQ NULL);
    }
    else
    {
      pQueryExpression_ = NULL;
    }
    break;

  case INDEX_WITH_CHECK_OPTION :
    if (pChildNode NEQ NULL)
    {
      pWithCheckOption_ = pChildNode->castToElemDDLNode();
      ComASSERT(pWithCheckOption_ NEQ NULL);
    }
    else
    {
      pWithCheckOption_ = NULL;
    }
    break;

  case INDEX_VIEW_OWNER :
    if (pChildNode NEQ NULL)
    {
      ElemDDLNode *pTemp = pChildNode->castToElemDDLNode();
      ComASSERT(pTemp NEQ NULL);
      pOwner_ = pTemp->castToElemDDLGrantee();
      ComASSERT(pOwner_ NEQ NULL);
    }
    else
    {
      pOwner_ = NULL;
    }
    break;

  default :
    ABORT("internal logic error");
  }
}

void
StmtDDLCreateView::synthesize()
{
  //
  // Collects pointers to column name parser nodes for easier access
  //

  if (pViewColumnList_ NEQ NULL)
  {
    for (Int32 i = 0; i < (Int32)pViewColumnList_->entries(); i++)
    {
      ComASSERT((*pViewColumnList_)[i] NEQ NULL);
      columnDefArray_.insert((*pViewColumnList_)[i]->
                             castToElemDDLColViewDef());
    }
  }

  //
  // Collects information from the Location clause if it appears
  //

  if (pLocationClause_ NEQ NULL)
  {    
    isLocationClauseSpec_ = TRUE;
    ElemDDLLocation * pLoc = pLocationClause_->castToElemDDLLocation();
    ComASSERT(pLoc NEQ NULL);
    if (NOT pLoc->getPartitionName().isNull())
        *SqlParser_Diags << DgSqlCode(-3405);
    locationName_     = pLoc->getLocationName();
    locationNameType_ = pLoc->getLocationNameType();
  }

  //
  // Collects information from With Check Option parser node if it exists
  //

  if (pWithCheckOption_ NEQ NULL)
  {
    ElemDDLWithCheckOption * pWCkOpt = pWithCheckOption_->
                                       castToElemDDLWithCheckOption();
    ComASSERT(pWCkOpt NEQ NULL);
    checkOptionLevel_ = pWCkOpt->getLevel();
  }
} // StmtDDLCreateView::synthesize()

//
// methods for tracing
//

const NAString
StmtDDLCreateView::displayLabel1() const
{
  return NAString("View name: ") + getViewName();
}

const NAString
StmtDDLCreateView::displayLabel2() const
{
  return NAString("is With Ck Opt spec? ") +
         YesNo(isWithCheckOptionSpecified());
}

const NAString
StmtDDLCreateView::displayLabel3() const
{
  if (isWithCheckOptionSpecified())
  {
    ElemDDLWithCheckOption * pWCkOpt = pWithCheckOption_->
                                       castToElemDDLWithCheckOption();
    ComASSERT(pWCkOpt NEQ NULL);
    return NAString("check option level: ") + pWCkOpt->getLevelAsNAString();
  }
  else
  {
    return NAString();
  }
}

NATraceList
StmtDDLCreateView::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailTextList.append(displayLabel1());    // display name of defined view
  detailTextList.append(displayLabel2());    // is With Check Option specified?
  if (NOT displayLabel3().isNull())
  {
    detailTextList.append(displayLabel3());  // display Check Option level
  }

  //
  // displays list of column names
  //

  CollIndex i, nbrCols;
  const ElemDDLColViewDefArray & colsList = getViewColDefArray();
  ElemDDLColViewDef * col;

  nbrCols = colsList.entries();

  if (nbrCols EQU 0)
  {
    detailTextList.append("No columns.");
  }
  else
  {
    detailText = "Column list [";
    detailText += LongToNAString((Lng32)nbrCols);
    detailText += " column(s)]:";
    detailTextList.append(detailText);

    for (i = 0; i < nbrCols; i++)
    {
      col = colsList[i];

      detailText = "[column ";
      detailText += LongToNAString((Lng32) i);
      detailText += "]";
      detailTextList.append(detailText);

      detailTextList.append("    ", col->getDetailInfo());
    }
  } // else (nbrCols EQU 0) of column references


  return detailTextList;
}

const NAString
StmtDDLCreateView::getText() const
{
  return "StmtDDLCreateView";
}

/*****************************************************

// -----------------------------------------------------------------------
// non-inline methods for class ViewUsages
// -----------------------------------------------------------------------

//
// virtual destructor
//
ViewUsages::~ViewUsages()
{
}

//
// accessors
//

const QualifiedName *
ViewUsages::getUsedTableNamePtr(const QualifiedName &tableName) const
{
  for (CollIndex i = 0; i < usedTableNameList_.entries(); i++)
  {
    if (usedTableNameList_[i] EQU tableName)
    {
      return &usedTableNameList_[i];
    }
  }
  return NULL;
}

//
// mutators
//

void
ViewUsages::insertUsedTableName(const QualifiedName &tableName)
{
  const QualifiedName *pTableName = getUsedTableNamePtr(tableName);
  if (pTableName EQU NULL)  // not found
  {
    // ok to insert
    usedTableNameList_.insert(tableName);
  }
}

*****************************************************/




// -----------------------------------------------------------------------
// Methods for class StmtDDLCommentOn
// -----------------------------------------------------------------------

//
// Constructor
//


StmtDDLCommentOn::StmtDDLCommentOn(COMMENT_ON_TYPES objType, const QualifiedName & objName, const NAString & commentStr, CollHeap * heap)
  : StmtDDLNode(DDL_COMMENT_ON),
    type_(objType),
    objectName_(objName, heap),
    comment_(commentStr),
    colRef_(NULL),
    isViewCol_(FALSE),
    colNum_(0)
{
      
}


StmtDDLCommentOn::StmtDDLCommentOn(COMMENT_ON_TYPES objType, const QualifiedName & objName, const NAString & commentStr, ColReference  * colRef, CollHeap * heap)
  : StmtDDLNode(DDL_COMMENT_ON),
    type_(objType),
    objectName_(objName, heap),
    colRef_(colRef),
    comment_(commentStr),
    isViewCol_(FALSE),
    colNum_(0)
{
      
}


StmtDDLCommentOn::~StmtDDLCommentOn()
{

}

StmtDDLCommentOn  * StmtDDLCommentOn::castToStmtDDLCommentOn()
{
  return this;
}

/* add escape quote for single quote */
NAString StmtDDLCommentOn::getCommentEscaped()
{
    NAString ret(comment_);
    int idx = ret.length() - 1;

    for (; idx >= 0; idx--)
    {
        if (ret[idx] == '\'')
            ret.insert(idx, "'");
    }

    return ret;
}


//
// End of File
//

