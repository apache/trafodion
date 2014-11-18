/* -*-C++-*- */
/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1995-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLAlter.C
 * Description:  contains definitions of non-inline methods for classes
 *               representing DDL Alter Statements
 *
 *               Also contains definitions of non-inline methods for classes
 *               relating to check constraint usages
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


#include "AllStmtDDLAlterTable.h"
#include "StmtDDLAlterTableToggleConstraint.h"
#include "BaseTypes.h"
#include "ComASSERT.h"
#include "ComOperators.h"
#include "ElemDDLConstraint.h"
#include "ElemDDLConstraintCheck.h"
#include "ElemDDLFileAttrClause.h"
#include "ElemDDLLibClientFilename.h"
#include "ElemDDLLibClientName.h"
#include "StmtDDLAlterIndexAttribute.h"
#include "ElemDDLConstraintPK.h"
#include "AllStmtDDLAlter.h" // MV - RG
#include "ElemDDLQualName.h" // MV - RG
#include "StmtDDLAlterTrigger.h"   
#include "AllElemDDLFileAttr.h"
#include "StmtDDLAlterAuditConfig.h"
#include "StmtDDLAlterCatalog.h"
#include "StmtDDLAlterLibrary.h"
#include "StmtDDLAlterSynonym.h"
#include "StmtDDLAlterTableDisableIndex.h"
#include "StmtDDLAlterTableEnableIndex.h"
#include "StmtDDLAlterTableAlterColumnDefaultValue.h"

// -----------------------------------------------------------------------
// definitions of non-inline methods for class ParCheckConstraintColUsage
// -----------------------------------------------------------------------

//
// default constructor
//

ParCheckConstraintColUsage::ParCheckConstraintColUsage(CollHeap * h)
  : isInSelectList_(FALSE),
    tableName_(h), columnName_(h)
{
}

//
// initialize constructor
//

ParCheckConstraintColUsage::ParCheckConstraintColUsage
( const ColRefName &colName,
  const NABoolean isInSelectList,
  CollHeap * h)
  : tableName_(colName.getCorrNameObj().getQualifiedNameObj(), h),
    columnName_(colName.getColName(), h),
    isInSelectList_(isInSelectList)
{
}


//
// virtual destructor
//

ParCheckConstraintColUsage::~ParCheckConstraintColUsage()
{
}

//
// operator
//

NABoolean
ParCheckConstraintColUsage::operator==(const ParCheckConstraintColUsage &rhs)
     const
{
  if (this EQU &rhs)
  {
    return TRUE;
  }
  return (getColumnName()    EQU rhs.getColumnName()    AND
          getTableQualName() EQU rhs.getTableQualName() AND
          isInSelectList()   EQU rhs.isInSelectList());
}

// -----------------------------------------------------------------------
// definitions of non-inline methods for class ParCheckConstraintColUsageList
// -----------------------------------------------------------------------

//
// constructor
//

ParCheckConstraintColUsageList::ParCheckConstraintColUsageList
 (const ParCheckConstraintColUsageList &other,
  CollHeap *heap)
: LIST(ParCheckConstraintColUsage *)(other, heap),
  heap_(heap)
{
}

//
// virtual destructor
//

ParCheckConstraintColUsageList::~ParCheckConstraintColUsageList()
{
  for (CollIndex i = 0; i < entries(); i++)
  {
//KSKSKS
    delete &operator[](i);
//    NADELETE(&operator[](i), ParCheckConstraintColUsage, heap_);
//KSKSKS
  }
}

//
// operator
//

ParCheckConstraintColUsageList &
ParCheckConstraintColUsageList::operator=
 (const ParCheckConstraintColUsageList &rhs)
{
  if (this EQU &rhs) return *this;
  clear();
  copy(rhs);
  return *this;
}

//
// accessor
//

ParCheckConstraintColUsage * const
ParCheckConstraintColUsageList::find(const ColRefName &colName)
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
// mutators
//

void
ParCheckConstraintColUsageList::clear()
{
  for (CollIndex i = 0; i < entries(); i++)
  {
//KSKSKS
    delete &operator[](i);
//    NADELETE(&operator[](i), ParCheckConstraintColUsage, heap_);
//KSKSKS
  }
  LIST(ParCheckConstraintColUsage *)::clear();
  // leave data member heap_ alone (it's only set once, by the constructor).
}

void
ParCheckConstraintColUsageList::copy(const ParCheckConstraintColUsageList &rhs)
{
  // DO NOT set the heap_ field.
  // It's already been set by the constructor
  // heap_ = rhs.heap_;

  for (CollIndex i = 0; i < rhs.entries(); i++)
  {
    CorrName corrName(rhs[i].getTableQualName());
    ColRefName colRefName(rhs[i].getColumnName(), corrName);
    insert(colRefName, rhs[i].isInSelectList());
  }
}

void
ParCheckConstraintColUsageList::insert(const ColRefName &colName,
                                       const NABoolean isInSelectList)
{
  ParCheckConstraintColUsage * const pCu = find(colName);
  if (pCu EQU NULL)  // not found
  {
    // ok to insert
    ParCheckConstraintColUsage *cu = new(heap_)
      ParCheckConstraintColUsage(colName, isInSelectList, heap_);
    LIST(ParCheckConstraintColUsage *)::insert(cu);
  }
  else  // found
  {
    if (NOT pCu->isInSelectList())
    {
      pCu->setIsInSelectList(isInSelectList);
    }
  }
}

// -----------------------------------------------------------------------
// definitions of non-inline methods for class ParCheckConstraintUsages
// -----------------------------------------------------------------------

//
// virtual destructor
//

ParCheckConstraintUsages::~ParCheckConstraintUsages()
{
}



// -----------------------------------------------------------------------
// methods for class StmtDDLAlterAuditConfig
// -----------------------------------------------------------------------

//
// constructor
//
// constructor used for ALTER AUDIT CONFIG
StmtDDLAlterAuditConfig::StmtDDLAlterAuditConfig(
   const NAString        & logName,
   const NAString        & columns,
   const NAString        & values)  
 : StmtDDLNode(DDL_ALTER_AUDIT_CONFIG),
   logName_(logName),
   columns_(columns),
   values_(values)
    
{

} 

// virtual destructor
StmtDDLAlterAuditConfig::~StmtDDLAlterAuditConfig()
{
}

// virtual cast
StmtDDLAlterAuditConfig *
StmtDDLAlterAuditConfig::castToStmtDDLAlterAuditConfig()
{
   return this;
}


//
// methods for tracing
//

const NAString
StmtDDLAlterAuditConfig::displayLabel1() const
{
   return NAString("Log name: ") + getLogName();
}

const NAString
StmtDDLAlterAuditConfig::displayLabel2() const
{

   return NAString("Columns: ") + getColumns() + 
          NAString("Values: ") + getValues();

}


const NAString
StmtDDLAlterAuditConfig::getText() const
{
  return "StmtDDLAlterAuditConfig";
}





// -----------------------------------------------------------------------
// Methods for class StmtDDLAlterCatalog
// -----------------------------------------------------------------------

// This constructor is used by: 
//    ALTER CATALOG <cat> <enable/disable> SCHEMA <schema name> 
StmtDDLAlterCatalog::StmtDDLAlterCatalog(const NAString & catalogName,
		    NABoolean isEnable,                    
                    const ElemDDLSchemaName & aSchemaNameParseNode,
		    CollHeap    * heap)

: StmtDDLNode(DDL_ALTER_CATALOG),
  catalogName_(catalogName, heap),
  schemaName_(heap),
  schemaQualName_(aSchemaNameParseNode.getSchemaName(), heap),
  enableStatus_(isEnable),
  disableEnableCreates_(FALSE),
  disableEnableAllCreates_(FALSE),
  isAllSchemaPrivileges_(FALSE)
{
}

// This constructor is used by:
//    ALTER CATALOG <cat> <enable/disable> ALL SCHEMA and
//    ALTER CATALOG <cat> <enable/disable> CREATE
StmtDDLAlterCatalog::StmtDDLAlterCatalog(const NAString & catalogName,
		    NABoolean isEnable,
                    NABoolean disableEnableCreates)
: StmtDDLNode(DDL_ALTER_CATALOG),
  catalogName_(catalogName, PARSERHEAP()),
  schemaName_(PARSERHEAP()),
  schemaQualName_(PARSERHEAP()),
  enableStatus_(isEnable)
{
  if (disableEnableCreates)
  {
    isAllSchemaPrivileges_ = FALSE;
    disableEnableCreates_ = TRUE;
    disableEnableAllCreates_ = FALSE;
  }
  else
  {
    isAllSchemaPrivileges_ = TRUE;
    disableEnableCreates_ = FALSE;
    disableEnableAllCreates_ = FALSE;
  }
}

// This constructure is used by:
//    ALTER ALL CATALOG <enable/disable> CREATE and
//    ALTER ALL CATALOGS <enable/disable> CREATE
StmtDDLAlterCatalog::StmtDDLAlterCatalog(NABoolean isEnable)
: StmtDDLNode(DDL_ALTER_CATALOG),
  catalogName_(PARSERHEAP()),
  schemaName_(PARSERHEAP()),
  schemaQualName_(PARSERHEAP()),
  enableStatus_(isEnable),
  isAllSchemaPrivileges_(FALSE),
  disableEnableCreates_(FALSE),
  disableEnableAllCreates_(TRUE)
{
}

// This constructor is used by: 
//    ALTER CATALOG <cat> <enable/disable> CREATE IN SCHEMA <schema name> 
StmtDDLAlterCatalog::StmtDDLAlterCatalog(const NAString & catalogName,
		    NABoolean isEnable,         
		    NABoolean disableEnableCreates,           
                    const ElemDDLSchemaName & aSchemaNameParseNode,
		    CollHeap    * heap)

: StmtDDLNode(DDL_ALTER_CATALOG),
  catalogName_(catalogName, heap),
  schemaName_(heap),
  schemaQualName_(aSchemaNameParseNode.getSchemaName(), heap),
  enableStatus_(isEnable),
  disableEnableCreates_(disableEnableCreates),
  disableEnableAllCreates_(FALSE),
  isAllSchemaPrivileges_(FALSE)
{
}

StmtDDLAlterCatalog::~StmtDDLAlterCatalog()
{}

// cast

StmtDDLAlterCatalog *
StmtDDLAlterCatalog::castToStmtDDLAlterCatalog()
{
   return this;
}

// for tracing
const NAString
StmtDDLAlterCatalog::displayLabel1() const
{
   return NAString("Catalog name: " ) + getCatalogName();
}

Int32
StmtDDLAlterCatalog::getArity() const
{
  return MAX_STMT_DDL_ALTER_CATALOG_ARITY;
}

const NAString
StmtDDLAlterCatalog::getText() const
{
   return "StmtDDLAlterCatalog";
}

void 
StmtDDLAlterCatalog::setSchemaName(const ElemDDLSchemaName & aSchemaName)
{
  schemaQualName_ = aSchemaName.getSchemaName();
}
void 
StmtDDLAlterCatalog::setAllPrivileges(NABoolean isAll)
{
  isAllSchemaPrivileges_ = isAll;
}


// -----------------------------------------------------------------------
// Methods for class StmtDDLAlterSynonym
// -----------------------------------------------------------------------

//
// Constructor
//

StmtDDLAlterSynonym::StmtDDLAlterSynonym(const QualifiedName & synonymName,
                                         const QualifiedName & objectReference)
: StmtDDLNode(DDL_ALTER_SYNONYM),
  synonymName_(synonymName, PARSERHEAP()),
  objectReference_(objectReference, PARSERHEAP())
{
}

//
// Virtual Destructor
// garbage collection is being done automatically by the NAString Class
//

StmtDDLAlterSynonym::~StmtDDLAlterSynonym()
{}

// cast

StmtDDLAlterSynonym *
StmtDDLAlterSynonym::castToStmtDDLAlterSynonym()
{
   return this;
}

// for tracing

const NAString
StmtDDLAlterSynonym::displayLabel1() const
{
   return NAString("Synonym name: " ) + getSynonymName();
}


const NAString
StmtDDLAlterSynonym::displayLabel2() const
{
   return NAString("Object reference: ") + getObjectReference();
}

const NAString
StmtDDLAlterSynonym::getText() const
{
   return "StmtDDLAlterSynonym";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAlterIndex
// -----------------------------------------------------------------------

StmtDDLAlterIndex::StmtDDLAlterIndex()
: StmtDDLNode(DDL_ANY_ALTER_INDEX_STMT),
  alterIndexAction_(NULL),
  indexName_(PARSERHEAP()),
  indexQualName_(PARSERHEAP())
{
}

StmtDDLAlterIndex::StmtDDLAlterIndex(OperatorTypeEnum operatorType)
: StmtDDLNode(operatorType),
  alterIndexAction_(NULL),
  indexName_(PARSERHEAP()),
  indexQualName_(PARSERHEAP())
{
}

StmtDDLAlterIndex::StmtDDLAlterIndex(OperatorTypeEnum operatorType,
                                     ElemDDLNode * pAlterIndexAction)
: StmtDDLNode(operatorType),
  alterIndexAction_(pAlterIndexAction),
  indexName_(PARSERHEAP()),
  indexQualName_(PARSERHEAP())
{
}

StmtDDLAlterIndex::StmtDDLAlterIndex(OperatorTypeEnum operatorType,
                                     const QualifiedName & indexName,
                                     ElemDDLNode * pAlterIndexAction)
: StmtDDLNode(operatorType),
  indexName_(PARSERHEAP()),
  indexQualName_(indexName, PARSERHEAP()),
  alterIndexAction_(pAlterIndexAction)
{
  indexName_ = indexQualName_.getQualifiedNameAsAnsiString();
}


// virtual destructor
StmtDDLAlterIndex::~StmtDDLAlterIndex()
{
  // delete all child parse nodes

  for (Int32 i = 0; i < getArity(); i++)
  {
    delete getChild(i);
  }
}

// cast virtual function
StmtDDLAlterIndex *
StmtDDLAlterIndex::castToStmtDDLAlterIndex()
{
  return this;
}

//
// accessors
//

Int32
StmtDDLAlterIndex::getArity() const
{
  return MAX_STMT_DDL_ALTER_INDEX_ARITY;
}

ExprNode *
StmtDDLAlterIndex::getChild(Lng32 index) 
{
  ComASSERT(index EQU INDEX_ALTER_INDEX_ACTION);
  return alterIndexAction_;
}

//
// mutators
//

void
StmtDDLAlterIndex::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index EQU INDEX_ALTER_INDEX_ACTION);
  if (pChildNode NEQ NULL)
  {
    ComASSERT(pChildNode->castToElemDDLNode() NEQ NULL);
    alterIndexAction_ = pChildNode->castToElemDDLNode(); 
  }
  else
    alterIndexAction_ = NULL;
}

void
StmtDDLAlterIndex::setIndexName(const QualifiedName &indexName)
{
  indexQualName_ = indexName;
}

//
// methods for tracing
//

const NAString
StmtDDLAlterIndex::getText() const
{
  return "StmtDDLAlterIndex";
}

const NAString
StmtDDLAlterIndex::displayLabel1() const
{
  return NAString("Index name: ") + getIndexName();
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAlterIndexAttribute
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLAlterIndexAttribute::StmtDDLAlterIndexAttribute(
     ElemDDLNode * pFileAttrNode)
: StmtDDLAlterIndex(DDL_ALTER_INDEX_ATTRIBUTE,
                    pFileAttrNode)
{

  // Traverse the File Attribute List parse sub-tree to extract the
  // information about the specified file attributes.  Store this
  // information in data member fileAttributes_.

  ComASSERT(getAlterIndexAction() NEQ NULL);
  ElemDDLFileAttrClause * pFileAttrClause = getAlterIndexAction()->
                                            castToElemDDLFileAttrClause();
  ComASSERT(pFileAttrClause NEQ NULL);
  ElemDDLNode * pFileAttrList = pFileAttrClause->getFileAttrDefBody();
  ComASSERT(pFileAttrList NEQ NULL);
  for (CollIndex i = 0; i < pFileAttrList->entries(); i++)
  {
    fileAttributes_.setFileAttr((*pFileAttrList)[i]->castToElemDDLFileAttr());
  }
}

// virtual destructor
StmtDDLAlterIndexAttribute::~StmtDDLAlterIndexAttribute()
{
}

// cast virtual function
StmtDDLAlterIndexAttribute *
StmtDDLAlterIndexAttribute::castToStmtDDLAlterIndexAttribute()
{
  return this;
}

//
// methods for tracing
//

NATraceList
StmtDDLAlterIndexAttribute::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;
  
  //
  // index name
  //

  detailTextList.append(displayLabel1());

  //
  // file attributes
  //

  detailTextList.append("File attributes: ");

  const ParDDLFileAttrsAlterIndex fileAttribs = getFileAttributes();
  detailTextList.append("    ", fileAttribs.getDetailInfo());

  return detailTextList;
}

const NAString
StmtDDLAlterIndexAttribute::getText() const
{
  return "StmtDDLAlterIndexAttribute";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAlterTable
// -----------------------------------------------------------------------

//
// constructors
//

StmtDDLAlterTable::StmtDDLAlterTable()
: StmtDDLNode(DDL_ANY_ALTER_TABLE_STMT),
  tableQualName_(PARSERHEAP()),
  alterTableAction_(NULL),
  isParseSubTreeDestroyedByDestructor_(TRUE),
  isDroppable_(TRUE),
  isOnline_(TRUE),
  forPurgedata_(FALSE)
{}

StmtDDLAlterTable::StmtDDLAlterTable(OperatorTypeEnum operatorType)
: StmtDDLNode(operatorType),
  tableQualName_(PARSERHEAP()),
  alterTableAction_(NULL),
  isParseSubTreeDestroyedByDestructor_(TRUE),
  isDroppable_(TRUE),
  isOnline_(TRUE),
  forPurgedata_(FALSE)
{}

StmtDDLAlterTable::StmtDDLAlterTable(OperatorTypeEnum operatorType,
                                     ElemDDLNode * pAlterTableAction)
: StmtDDLNode(operatorType),
  tableQualName_(PARSERHEAP()),
  alterTableAction_(pAlterTableAction),
  isParseSubTreeDestroyedByDestructor_(TRUE),
  isDroppable_(TRUE),
  isOnline_(TRUE),
  forPurgedata_(FALSE)
{}

StmtDDLAlterTable::StmtDDLAlterTable(OperatorTypeEnum operatorType,
                                     const QualifiedName & tableQualName,
                                     ElemDDLNode * pAlterTableAction)
: StmtDDLNode(operatorType),
  tableQualName_(tableQualName, PARSERHEAP()),
  alterTableAction_(pAlterTableAction),
  isParseSubTreeDestroyedByDestructor_(TRUE),
  isDroppable_(TRUE),
  isOnline_(TRUE),
  forPurgedata_(FALSE)
{}


// virtual destructor
StmtDDLAlterTable::~StmtDDLAlterTable()
{
  //
  // delete all children if the flag isParseSubTreeDestroyedByDestructor_
  // is set.  For more detail information about this flag, please read
  // the comments about this flag in the header file StmtDDLAlterTable.h.
  //
  if (isParseSubTreeDestroyedByDestructor_)
  {
    for (Int32 i = 0; i < getArity(); i++)
    {
      delete getChild(i);
    }
  }
}

// cast virtual function
StmtDDLAlterTable *
StmtDDLAlterTable::castToStmtDDLAlterTable()
{
  return this;
}

//
// accessors
//

Int32
StmtDDLAlterTable::getArity() const
{
  return MAX_STMT_DDL_ALTER_TABLE_ARITY;
}

ExprNode *
StmtDDLAlterTable::getChild(Lng32 index) 
{
  ComASSERT(index EQU INDEX_ALTER_TABLE_ACTION);
  return alterTableAction_;
}

//
// mutators
//

void
StmtDDLAlterTable::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index EQU INDEX_ALTER_TABLE_ACTION);
  if (pChildNode NEQ NULL)
  {
    ComASSERT(pChildNode->castToElemDDLNode() NEQ NULL);
    alterTableAction_ = pChildNode->castToElemDDLNode(); 
  }
  else
    alterTableAction_ = NULL;
}

void
StmtDDLAlterTable::setTableName(const QualifiedName &tableQualName)
{
  tableQualName_ = tableQualName;
}

//
// methods for tracing
//

const NAString
StmtDDLAlterTable::getText() const
{
  return "StmtDDLAlterTable";
}

const NAString
StmtDDLAlterTable::displayLabel1() const
{
  return NAString("Table name: ") + getTableName();
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAddConstraint
// -----------------------------------------------------------------------

// virtual destructor
StmtDDLAddConstraint::~StmtDDLAddConstraint()
{
}

// cast virtual function
StmtDDLAddConstraint *
StmtDDLAddConstraint::castToStmtDDLAddConstraint()
{
  return this;
}

//
// accessors
//

const NAString
StmtDDLAddConstraint::getConstraintName() const
{
  ComASSERT(getAlterTableAction() NEQ NULL);
  ElemDDLConstraint * pConstraint
    = getAlterTableAction()->castToElemDDLConstraint();
  ComASSERT(pConstraint NEQ NULL);

  return pConstraint->getConstraintName();
}

const QualifiedName &
StmtDDLAddConstraint::getConstraintNameAsQualifiedName() const
{
  return ((StmtDDLAddConstraint *)this)->getConstraintNameAsQualifiedName();
}

QualifiedName &
StmtDDLAddConstraint::getConstraintNameAsQualifiedName()
{
  ComASSERT(getAlterTableAction() NEQ NULL);
  ElemDDLConstraint * pConstraint
    = getAlterTableAction()->castToElemDDLConstraint();
  ComASSERT(pConstraint NEQ NULL);

  return pConstraint->getConstraintNameAsQualifiedName();
}

NABoolean
StmtDDLAddConstraint::isDeferrable() const
{
  ComASSERT(getAlterTableAction() NEQ NULL);
  ElemDDLConstraint * pConstraint
    = getAlterTableAction()->castToElemDDLConstraint();
  ComASSERT(pConstraint NEQ NULL);

  return pConstraint->isDeferrable();
}

NABoolean
StmtDDLAddConstraint::isDroppable() const
{
  ComASSERT(getAlterTableAction() NEQ NULL);
  ElemDDLConstraint * pConstraint
    = getAlterTableAction()->castToElemDDLConstraint();
  ComASSERT(pConstraint NEQ NULL);

  StmtDDLAddConstraint *ncThis = (StmtDDLAddConstraint *)this;
  StmtDDLAddConstraintPK *pk = ncThis->castToStmtDDLAddConstraintPK();
  if (pk NEQ NULL AND pk->isAlwaysDroppable())
  {
    // Primary Key constrainst created in the ALTER TABLE <table-name>
    // ADD CONSTRAINT statement specified by the user is always droppable.
    return TRUE;
  }
  else
  {
    return pConstraint->isDroppable();
  }
}

NABoolean
StmtDDLAddConstraint::isDroppableSpecifiedExplicitly() const
{
  ComASSERT(getAlterTableAction() NEQ NULL);
  ElemDDLConstraint * pConstraint
    = getAlterTableAction()->castToElemDDLConstraint();
  ComASSERT(pConstraint NEQ NULL);

  return pConstraint->isDroppableSpecifiedExplicitly();
}

NABoolean
StmtDDLAddConstraint::isNotDroppableSpecifiedExplicitly() const
{
  ComASSERT(getAlterTableAction() NEQ NULL);
  ElemDDLConstraint * pConstraint
    = getAlterTableAction()->castToElemDDLConstraint();
  ComASSERT(pConstraint NEQ NULL);

  return pConstraint->isNotDroppableSpecifiedExplicitly();
}

//
// mutators
//

void
StmtDDLAddConstraint::setDroppableFlag(const NABoolean setting)
{
  ComASSERT(getAlterTableAction() NEQ NULL);
  ElemDDLConstraint * pConstraint
    = getAlterTableAction()->castToElemDDLConstraint();
  ComASSERT(pConstraint NEQ NULL);

  pConstraint->setDroppableFlag(setting);
}

//
// methods for tracing
//

NATraceList
StmtDDLAddConstraint::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  ElemDDLConstraint * pConstraint = getConstraint();
  ComASSERT(pConstraint NEQ NULL);
  detailTextList.append(pConstraint->getDetailInfo());

  return detailTextList;

} // StmtDDLAddConstraint::getDetailInfo()

const NAString
StmtDDLAddConstraint::getText() const
{
  return "StmtDDLAddConstraint";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAddConstraintArray
// -----------------------------------------------------------------------

// virtual destructor
StmtDDLAddConstraintArray::~StmtDDLAddConstraintArray()
{
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAddConstraintCheck
// -----------------------------------------------------------------------

//
// constructors
//

StmtDDLAddConstraintCheck::StmtDDLAddConstraintCheck(
     ElemDDLNode * pElemDDLConstraintCheck)
: StmtDDLAddConstraint(DDL_ALTER_TABLE_ADD_CONSTRAINT_CHECK,
                       pElemDDLConstraintCheck)
{
  init(pElemDDLConstraintCheck);
}

StmtDDLAddConstraintCheck::StmtDDLAddConstraintCheck(
     const QualifiedName & tableQualName,
     ElemDDLNode * pElemDDLConstraintCheck)
: StmtDDLAddConstraint(DDL_ALTER_TABLE_ADD_CONSTRAINT_CHECK,
                       tableQualName,
                       pElemDDLConstraintCheck)
{
  init(pElemDDLConstraintCheck);
}

void
StmtDDLAddConstraintCheck::init(ElemDDLNode * pElemDDLConstraintCheck)
{
  ComASSERT(pElemDDLConstraintCheck NEQ NULL AND
            pElemDDLConstraintCheck->castToElemDDLConstraintCheck() NEQ NULL);
  ElemDDLConstraintCheck *pCkCnstrnt =
    pElemDDLConstraintCheck->castToElemDDLConstraintCheck();
  endPos_ = pCkCnstrnt->getEndPosition();
  startPos_ = pCkCnstrnt->getStartPosition();
  nameLocList_ = pCkCnstrnt->getNameLocList();
}
  
// virtual destructor
StmtDDLAddConstraintCheck::~StmtDDLAddConstraintCheck()
{
}

// cast virtual function
StmtDDLAddConstraintCheck *
StmtDDLAddConstraintCheck::castToStmtDDLAddConstraintCheck()
{
  return this;
}

//
// accessors
//

ItemExpr *
StmtDDLAddConstraintCheck::getSearchCondition() const
{
  ElemDDLConstraintCheck *pCkCnstrnt =
    getAlterTableAction()->castToElemDDLConstraintCheck();
  ComASSERT(pCkCnstrnt NEQ NULL);
  return pCkCnstrnt->getSearchCondition();
}

//
// methods for tracing
//

const NAString
StmtDDLAddConstraintCheck::getText() const
{
  return "StmtDDLAddConstraintCheck";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAddConstraintCheckArray
// -----------------------------------------------------------------------

// virtual destructor
StmtDDLAddConstraintCheckArray::~StmtDDLAddConstraintCheckArray()
{
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAddConstraintPK
// -----------------------------------------------------------------------

// constructors
StmtDDLAddConstraintPK::StmtDDLAddConstraintPK(
     ElemDDLNode * pElemDDLConstraintPK,
     const NABoolean isAlwaysDroppable)
: StmtDDLAddConstraintUnique(DDL_ALTER_TABLE_ADD_CONSTRAINT_PRIMARY_KEY,
                             pElemDDLConstraintPK),
  isAlwaysDroppable_(isAlwaysDroppable)
{
  if (isAlwaysDroppable AND isNotDroppableSpecifiedExplicitly())
  {
    // Primary key constraint defined in ALTER TABLE statement is always
    // droppable.  Please remove the NOT DROPPABLE clause.
    // Note that for the ALTER TABLE ADD COLUMN statement, this check
    // cannot be made in the constructor, since this statement uses 
    // the code for CREATE TABLE to parse its column definition.  
    // The test for this error was therefore put into function
    // CatAlterTableAddColumn.
    *SqlParser_Diags << DgSqlCode(-3067);
  }
}
StmtDDLAddConstraintPK::StmtDDLAddConstraintPK(
     const QualifiedName & tableQualName,
     ElemDDLNode * pElemDDLConstraintPK,
     const NABoolean isAlwaysDroppable)
: StmtDDLAddConstraintUnique(DDL_ALTER_TABLE_ADD_CONSTRAINT_PRIMARY_KEY,
                             tableQualName,
                             pElemDDLConstraintPK),
  isAlwaysDroppable_(isAlwaysDroppable)
{
  if (isAlwaysDroppable AND isNotDroppableSpecifiedExplicitly())
  {
    // Primary key constraint defined in ALTER TABLE statement is always
    // droppable.  Please remove the NOT DROPPABLE clause.
    // Note that for the ALTER TABLE ADD COLUMN statement, this check
    // cannot be made in the constructor, since this statement uses 
    // the code for CREATE TABLE to parse its column definition.  
    // The test for this error was therefore put into function
    // CatAlterTableAddColumn.
    *SqlParser_Diags << DgSqlCode(-3067);
  }
}

// virtual destructor
StmtDDLAddConstraintPK::~StmtDDLAddConstraintPK()
{
}

// cast virtual function
StmtDDLAddConstraintPK *
StmtDDLAddConstraintPK::castToStmtDDLAddConstraintPK()
{
  return this;
}

// methods for tracing

const NAString
StmtDDLAddConstraintPK::getText() const
{
  return "StmtDDLAddConstraintPK";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAddConstraintRI
// -----------------------------------------------------------------------

// virtual destructor
StmtDDLAddConstraintRI::~StmtDDLAddConstraintRI()
{
}

// cast virtual function
StmtDDLAddConstraintRI *
StmtDDLAddConstraintRI::castToStmtDDLAddConstraintRI()
{
  return this;
}

//
// accessors
//

ComRCDeleteRule
StmtDDLAddConstraintRI::getDeleteRule() const
{
  return getConstraint()->castToElemDDLConstraintRI()->getDeleteRule();
}

ComRCMatchOption
StmtDDLAddConstraintRI::getMatchType() const
{
  return getConstraint()->castToElemDDLConstraintRI()->getMatchType();
}

ElemDDLColNameArray & 
StmtDDLAddConstraintRI::getReferencedColumns()
{
  return getConstraint()->castToElemDDLConstraintRI()->
    getReferencedColumns();
}

const ElemDDLColNameArray & 
StmtDDLAddConstraintRI::getReferencedColumns() const
{
  return getConstraint()->castToElemDDLConstraintRI()->
    getReferencedColumns();
}

NAString
StmtDDLAddConstraintRI::getReferencedTableName() const
{
  return getConstraint()->castToElemDDLConstraintRI()->
    getReferencedTableName();
}

const ElemDDLColNameArray & 
StmtDDLAddConstraintRI::getReferencingColumns() const
{
  return getConstraint()->castToElemDDLConstraintRI()->
    getReferencingColumns();
}

ElemDDLColNameArray & 
StmtDDLAddConstraintRI::getReferencingColumns()
{
  return getConstraint()->castToElemDDLConstraintRI()->
    getReferencingColumns();
}

ComRCUpdateRule
StmtDDLAddConstraintRI::getUpdateRule() const
{
  return getConstraint()->castToElemDDLConstraintRI()->getUpdateRule();
}

NABoolean
StmtDDLAddConstraintRI::isDeleteRuleSpecified() const
{
  return getConstraint()->castToElemDDLConstraintRI()->
    isDeleteRuleSpecified();
}

NABoolean
StmtDDLAddConstraintRI::isUpdateRuleSpecified() const
{
  return getConstraint()->castToElemDDLConstraintRI()->
    isUpdateRuleSpecified();
}

// methods for tracing

const NAString
StmtDDLAddConstraintRI::getText() const
{
  return "StmtDDLAddConstraintRI";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAddConstraintRIArray
// -----------------------------------------------------------------------

// virtual destructor
StmtDDLAddConstraintRIArray::~StmtDDLAddConstraintRIArray()
{
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAddConstraintUnique
// -----------------------------------------------------------------------

// virtual destructor
StmtDDLAddConstraintUnique::~StmtDDLAddConstraintUnique()
{
}

// cast virtual function
StmtDDLAddConstraintUnique *
StmtDDLAddConstraintUnique::castToStmtDDLAddConstraintUnique()
{
  return this;
}

// methods for tracing

const NAString
StmtDDLAddConstraintUnique::getText() const
{
  return "StmtDDLAddConstraintUnique";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAddConstraintUniqueArray
// -----------------------------------------------------------------------

// virtual destructor
StmtDDLAddConstraintUniqueArray::~StmtDDLAddConstraintUniqueArray()
{
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAlterTableAttribute
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLAlterTableAttribute::StmtDDLAlterTableAttribute(
     ElemDDLNode * pFileAttrNode)
: StmtDDLAlterTable(DDL_ALTER_TABLE_ATTRIBUTE,
                    pFileAttrNode)
{

  // Traverse the File Attribute List parse sub-tree to extract the
  // information about the specified file attributes.  Store this
  // information in data member fileAttributes_.

  ComASSERT(getAlterTableAction() NEQ NULL);
  ElemDDLFileAttrClause * pFileAttrClause = getAlterTableAction()->
                                            castToElemDDLFileAttrClause();
  ComASSERT(pFileAttrClause NEQ NULL);
  ElemDDLNode * pFileAttrList = pFileAttrClause->getFileAttrDefBody();
  ComASSERT(pFileAttrList NEQ NULL);
  for (CollIndex i = 0; i < pFileAttrList->entries(); i++)
  {
    fileAttributes_.setFileAttr((*pFileAttrList)[i]->castToElemDDLFileAttr());
  }
}

// virtual destructor
StmtDDLAlterTableAttribute::~StmtDDLAlterTableAttribute()
{
}

// cast virtual function
StmtDDLAlterTableAttribute *
StmtDDLAlterTableAttribute::castToStmtDDLAlterTableAttribute()
{
  return this;
}

//
// methods for tracing
//

NATraceList
StmtDDLAlterTableAttribute::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;
  
  //
  // table name
  //

  detailTextList.append(displayLabel1());

  //
  // file attributes
  //

  detailTextList.append("File attributes: ");

  const ParDDLFileAttrsAlterTable fileAttribs = getFileAttributes();
  detailTextList.append("    ", fileAttribs.getDetailInfo());

  return detailTextList;
}

const NAString
StmtDDLAlterTableAttribute::getText() const
{
  return "StmtDDLAlterTableAttribute";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAlterTableColumn
// -----------------------------------------------------------------------

// virtual destructor
StmtDDLAlterTableColumn::~StmtDDLAlterTableColumn()
{
}

// cast virtual function
StmtDDLAlterTableColumn *
StmtDDLAlterTableColumn::castToStmtDDLAlterTableColumn()
{
  return this;
}

//
// methods for tracing
//

const NAString
StmtDDLAlterTableColumn::getText() const
{
  return "StmtDDLAlterTableColumn";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAlterTableMove
// -----------------------------------------------------------------------

// virtual destructor
StmtDDLAlterTableMove::~StmtDDLAlterTableMove()
{
}

// cast virtual function
StmtDDLAlterTableMove *
StmtDDLAlterTableMove::castToStmtDDLAlterTableMove()
{
  return this;
}

//
// methods for tracing
//

const NAString
StmtDDLAlterTableMove::getText() const
{
  return "StmtDDLAlterTableMove";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAlterTablePartition
// -----------------------------------------------------------------------

// virtual destructor
StmtDDLAlterTablePartition::~StmtDDLAlterTablePartition()
{
}

// cast virtual function
StmtDDLAlterTablePartition *
StmtDDLAlterTablePartition::castToStmtDDLAlterTablePartition()
{
  return this;
}

//
// methods for tracing
//

const NAString
StmtDDLAlterTablePartition::getText() const
{
  return "StmtDDLAlterTablePartition";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAlterTableRename
// -----------------------------------------------------------------------

// virtual destructor
StmtDDLAlterTableRename::~StmtDDLAlterTableRename()
{
}

// cast virtual function
StmtDDLAlterTableRename *
StmtDDLAlterTableRename::castToStmtDDLAlterTableRename()
{
  return this;
}

//
// methods for tracing
//

const NAString
StmtDDLAlterTableRename::getText() const
{
  return "StmtDDLAlterTableRename";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAlterTableNamespace
// -----------------------------------------------------------------------

// virtual destructor
StmtDDLAlterTableNamespace::~StmtDDLAlterTableNamespace()
{
}

// cast virtual function
StmtDDLAlterTableNamespace *
StmtDDLAlterTableNamespace::castToStmtDDLAlterTableNamespace()
{
  return this;
}

//
// methods for tracing
//

const NAString
StmtDDLAlterTableNamespace::getText() const
{
  return "StmtDDLAlterTableNamespace";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAlterTableSetConstraint
// -----------------------------------------------------------------------

// virtual destructor
StmtDDLAlterTableSetConstraint::~StmtDDLAlterTableSetConstraint()
{
}

// cast virtual function
StmtDDLAlterTableSetConstraint *
StmtDDLAlterTableSetConstraint::castToStmtDDLAlterTableSetConstraint()
{
  return this;
}

//
// methods for tracing
//

const NAString
StmtDDLAlterTableSetConstraint::getText() const
{
  return "StmtDDLAlterTableSetConstraint";
}

// --------------------------------------------------------------------
// methods for class StmtDDLAlterTableDisableIndex
// --------------------------------------------------------------------

//
// constructor
//

StmtDDLAlterTableDisableIndex::StmtDDLAlterTableDisableIndex
    (NAString & indexName, NABoolean allIndexes)
  : StmtDDLAlterTable(DDL_ALTER_TABLE_DISABLE_INDEX),
    indexName_(indexName, PARSERHEAP()),
    allIndexes_(allIndexes) 
{
}

//
// Virtual destructor
//

StmtDDLAlterTableDisableIndex::~StmtDDLAlterTableDisableIndex()
{
}

//
// Cast function: to provide the safe castdown to the current object
//

StmtDDLAlterTableDisableIndex *
StmtDDLAlterTableDisableIndex::castToStmtDDLAlterTableDisableIndex()
{
  return this;
}

//
// accessors
//

//
// for tracing
//

const NAString
StmtDDLAlterTableDisableIndex::displayLabel2() const
{
  return NAString ("Index name: ") + getIndexName();
}

const NAString
StmtDDLAlterTableDisableIndex::getText() const
{
  return "StmtAlterTableDisableIndex" ;
}

// --------------------------------------------------------------------
// methods for class StmtDDLAlterTableEnableIndex
// --------------------------------------------------------------------

//
// constructor
//

StmtDDLAlterTableEnableIndex::StmtDDLAlterTableEnableIndex
    (NAString & indexName, NABoolean allIndexes)
  : StmtDDLAlterTable(DDL_ALTER_TABLE_ENABLE_INDEX),
    indexName_(indexName, PARSERHEAP()),
    allIndexes_(allIndexes) 
{
}

//
// Virtual destructor
//

StmtDDLAlterTableEnableIndex::~StmtDDLAlterTableEnableIndex()
{
}

//
// Cast function: to provide the safe castdown to the current object
//

StmtDDLAlterTableEnableIndex *
StmtDDLAlterTableEnableIndex::castToStmtDDLAlterTableEnableIndex()
{
  return this;
}

//
// accessors
//

//
// for tracing
//

const NAString
StmtDDLAlterTableEnableIndex::displayLabel2() const
{
  return NAString ("Index name: ") + getIndexName();
}

const NAString
StmtDDLAlterTableEnableIndex::getText() const
{
  return "StmtAlterTableEnableIndex" ;
}

// --------------------------------------------------------------------
// methods for class StmtDDLAlterTableToggleConstraint
// --------------------------------------------------------------------

//
// constructor
//

StmtDDLAlterTableToggleConstraint::StmtDDLAlterTableToggleConstraint
  (const QualifiedName & constraintQualifiedName
         , NABoolean allConstraints
         , NABoolean setDisabled
         , NABoolean validateConstraint)
  : StmtDDLAlterTable(DDL_ALTER_TABLE_TOGGLE_CONSTRAINT),
    constraintQualName_(constraintQualifiedName, PARSERHEAP()),
    allConstraints_(allConstraints),
    setDisabled_(setDisabled),
    validateConstraint_(validateConstraint)
{
}


//
// Virtual destructor
//

StmtDDLAlterTableToggleConstraint::~StmtDDLAlterTableToggleConstraint()
{
}

//
// Cast function: to provide the safe castdown to the current object
//

StmtDDLAlterTableToggleConstraint *
StmtDDLAlterTableToggleConstraint::castToStmtDDLAlterTableToggleConstraint()
{
  return this;
}

//
// accessors
//
const NAString
StmtDDLAlterTableToggleConstraint::getConstraintName() const
{
  return constraintQualName_.getQualifiedNameAsAnsiString();
}

//
// for tracing
//

const NAString
StmtDDLAlterTableToggleConstraint::displayLabel2() const
{
  return NAString ("Constraint name: ") + getConstraintName();
}

const NAString
StmtDDLAlterTableToggleConstraint::getText() const
{
  return "StmtAlterTableToggleConstraint" ;
}

// -----------------------------------------------------------------------
// methods for class StmtDDLDropConstraint
// -----------------------------------------------------------------------

// 
// constructor
// 

StmtDDLDropConstraint::StmtDDLDropConstraint(const QualifiedName &
                                             constraintQualifiedName,
                                             ComDropBehavior dropbehavior)

  : StmtDDLAlterTable(DDL_ALTER_TABLE_DROP_CONSTRAINT),
    constraintQualName_(constraintQualifiedName, PARSERHEAP()),
    dropBehavior_(dropbehavior)
{
}

//
// Virtual destructor 
//

StmtDDLDropConstraint::~StmtDDLDropConstraint()
{}

//
// Cast function: to provide the safe castdown to the current object 
//

StmtDDLDropConstraint * 
StmtDDLDropConstraint::castToStmtDDLDropConstraint()
{
  return this;
}

//
// accessors
//

const NAString
StmtDDLDropConstraint::getConstraintName() const
{
  return constraintQualName_.getQualifiedNameAsAnsiString();
}

// 
// for tracing
//

const NAString 
StmtDDLDropConstraint::displayLabel2() const
{
  return NAString ("Constraint name: ") + getConstraintName();
}

const NAString
StmtDDLDropConstraint::getText() const
{
  return "StmtAlterTableDropContraint" ;
}

const NAString 
StmtDDLDropConstraint::displayLabel3() const
{
  NAString label2("Drop Behavior: ");
  switch (getDropBehavior())
    {
    case COM_CASCADE_DROP_BEHAVIOR :
      return label2 + "Cascade" ;
    case COM_RESTRICT_DROP_BEHAVIOR :
      return label2 + "Restrict" ;
    default : 
      NAAbort("StmtDDLAlter.C", __LINE__ , "Internal logic error");
      return NAString();
    }
}

//----------------------------------------------------------------------------
// CLASS StmtDDLAlterTableAlterColumnDefaultValue
//----------------------------------------------------------------------------
StmtDDLAlterTableAlterColumnDefaultValue::StmtDDLAlterTableAlterColumnDefaultValue( const NAString &columnName 
                                                                                  , ElemDDLNode *pColDefault
                                                                                  , CollHeap *heap)
    : StmtDDLAlterTable(DDL_ALTER_TABLE_ALTER_COLUMN_DEFAULT_VALUE, 
			QualifiedName(PARSERHEAP()) /*no table name*/,
		 	pColDefault /* alterTableAction_ */),	
                        columnName_(columnName, heap)
{


}

//
// Virtual destructor
//

StmtDDLAlterTableAlterColumnDefaultValue::~StmtDDLAlterTableAlterColumnDefaultValue()
{}

//
// Cast function: to provide the safe castdown to the current object
//

StmtDDLAlterTableAlterColumnDefaultValue *
StmtDDLAlterTableAlterColumnDefaultValue::castToStmtDDLAlterTableAlterColumnDefaultValue()
{
  return this;
}

const NAString
StmtDDLAlterTableAlterColumnDefaultValue::getText() const
{
  return "StmtDDLAlterTableAlterColumnDefaultValue" ;
}
	
//----------------------------------------------------------------------------
// CLASS StmtDDLAlterTableAlterColumnLoggable
//----------------------------------------------------------------------------

StmtDDLAlterTableAlterColumnLoggable::StmtDDLAlterTableAlterColumnLoggable
										(ElemDDLNode * pColumnDefinition,
										NABoolean loggableVal
										,CollHeap    * heap)
	: StmtDDLAlterTable(DDL_ALTER_TABLE_ALTER_COLUMN_LOGGABLE,
						QualifiedName(PARSERHEAP()) /*no table name*/,
						pColumnDefinition),
        columnName_(heap),
	loggable_(loggableVal)
{



}


StmtDDLAlterTableAlterColumnLoggable::StmtDDLAlterTableAlterColumnLoggable
										(NAString columnName,
										NABoolean loggableVal
										,CollHeap    * heap)
	: StmtDDLAlterTable(DDL_ALTER_TABLE_ALTER_COLUMN_LOGGABLE),
	columnName_(columnName, heap),
	loggable_(loggableVal)
{


}

StmtDDLAlterTableAlterColumnLoggable::~StmtDDLAlterTableAlterColumnLoggable()
{



}


StmtDDLAlterTableAlterColumnLoggable * 
StmtDDLAlterTableAlterColumnLoggable::
								castToStmtDDLAlterTableAlterColumnLoggable()
{
	return this;

}

//----------------------------------------------------------------------------
// CLASS StmtDDLAlterTableAlterColumnSetSGOption
//----------------------------------------------------------------------------
StmtDDLAlterTableAlterColumnSetSGOption::StmtDDLAlterTableAlterColumnSetSGOption( const NAString &columnName 
                                                                                  , ElemDDLSGOptions *pSGOptions
                                                                                  , CollHeap *heap)
    : StmtDDLAlterTable(DDL_ALTER_TABLE_ALTER_COLUMN_SET_SG_OPTION, 
			QualifiedName(PARSERHEAP()) /*no table name*/,
			NULL),	
 			columnName_(columnName, heap),
			pSGOptions_(pSGOptions)
{


}

//
// Virtual destructor
//

StmtDDLAlterTableAlterColumnSetSGOption::~StmtDDLAlterTableAlterColumnSetSGOption()
{
  if (pSGOptions_)
    delete pSGOptions_;

  if (columnName_)
    delete columnName_;
}

//
// Cast function: to provide the safe castdown to the current object
//

StmtDDLAlterTableAlterColumnSetSGOption *
StmtDDLAlterTableAlterColumnSetSGOption::castToStmtDDLAlterTableAlterColumnSetSGOption()
{
  return this;
}

const NAString
StmtDDLAlterTableAlterColumnSetSGOption::getText() const
{
  return "StmtDDLAlterTableAlterColumnSetSGOption" ;
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAlterTableAddColumn
// -----------------------------------------------------------------------

// constructor
StmtDDLAlterTableAddColumn::StmtDDLAlterTableAddColumn(
      ElemDDLNode * pColumnDefinition
     ,CollHeap    * heap )
: StmtDDLAlterTable(DDL_ALTER_TABLE_ADD_COLUMN,
                    QualifiedName(PARSERHEAP()) /*no table name*/,
                    pColumnDefinition)
  ,pColumnToAdd_(pColumnDefinition)
  ,columnDefArray_(heap)
  ,addConstraintCheckArray_(heap)
  ,addConstraintRIArray_(heap)
  ,addConstraintUniqueArray_(heap)
  ,addConstraintArray_(heap)
  ,pAddConstraintPK_(NULL)
  ,addIfNotExists_(FALSE)
{
}

// virtual destructor
StmtDDLAlterTableAddColumn::~StmtDDLAlterTableAddColumn()
{
  // Delete the kludge parse nodes derived from class
  // StmtDDLAddConstraint.  For more information, please read
  // the contents of the header file StmtDDLAlterTableAddColumn.h.

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

// cast virtual function
StmtDDLAlterTableAddColumn *
StmtDDLAlterTableAddColumn::castToStmtDDLAlterTableAddColumn()
{
  return this;
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAlterTableDropColumn
// -----------------------------------------------------------------------

// constructor
StmtDDLAlterTableDropColumn::StmtDDLAlterTableDropColumn(
							 NAString & colName
							 ,CollHeap    * heap )
  : StmtDDLAlterTable(DDL_ALTER_TABLE_DROP_COLUMN,
		      QualifiedName(PARSERHEAP()) /*no table name*/,
		      NULL) 
  ,colName_(colName)
  ,dropIfExists_(FALSE)
{
}

// virtual destructor
StmtDDLAlterTableDropColumn::~StmtDDLAlterTableDropColumn()
{
}

// cast virtual function
StmtDDLAlterTableDropColumn *
StmtDDLAlterTableDropColumn::castToStmtDDLAlterTableDropColumn()
{
  return this;
}

void
StmtDDLAlterTableAddColumn_visit(ElemDDLNode * pAltTabAddColNode,
                                        CollIndex /* index */,
                                        ElemDDLNode * pElement);
//
// Collect information in the parse sub-tree and copy it
// to the current parse node.
//
void
StmtDDLAlterTableAddColumn::synthesize()
{
  ElemDDLNode *theColumn = getColToAdd();
  if (theColumn NEQ NULL)
  {
    // If pConstraint is not NULL, it points to a (left linear tree) list
    // of column constraint definitions. Traverse this parse sub-tree.
    // For each column constraint definition (except for Not Null
    // constraint), add it to the contraint list corresponding to its
    // constraint type (Check, RI, or Unique).

    theColumn->traverseList(
         this,
         StmtDDLAlterTableAddColumn_visit);
  }
}  //StmtDDLAlterTableAddColumn::synthesize()

//------------------------------------------------------------------
// StmtDDLAlterTableAddColumn_visit
//
// Parameter pAltTabAddColNode points to the Alter Table Add Column
//   parse node.
// Parameter pElement points to a column constraint definition
//   parse node in the left linear tree list.
//     This tree is a sub-tree in the Alter Table Add Column parse node.
// Parameter index contains the index of the parse node
//   pointed by pElement in the (left linear tree) list.
//------------------------------------------------------------------
void
StmtDDLAlterTableAddColumn_visit(ElemDDLNode * pAltTabAddColNode,
                                        CollIndex /* index */,
                                        ElemDDLNode * pElement)
{
  ComASSERT(pAltTabAddColNode NEQ NULL AND
    pAltTabAddColNode->castToStmtDDLAlterTableAddColumn() NEQ NULL AND
    pElement NEQ NULL);

  StmtDDLAlterTableAddColumn * pAltTabAddCol =
    pAltTabAddColNode->castToStmtDDLAlterTableAddColumn();
  if (pElement->castToElemDDLColDef() NEQ NULL)
  {
    ElemDDLColDef *pColDef = pElement->castToElemDDLColDef();
    pAltTabAddCol->getColDefArray().insert(pColDef);
    if(pColDef->getIsConstraintPKSpecified())
    {
      pAltTabAddCol->setConstraint(pColDef->getConstraintPK());
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
      pAltTabAddCol->setConstraint(pColDef->getConstraintArray()[i]);
    }
  }
  else
    *SqlParser_Diags << DgSqlCode(-1001);

} // StmtDDLAlterTableAddColumn_visit()

void
StmtDDLAlterTableAddColumn::setConstraint(ElemDDLNode * pElement)
{
  switch (pElement->getOperatorType())
  {
  case ELM_CONSTRAINT_CHECK_ELEM :
    {
      ComASSERT(pElement->castToElemDDLConstraintCheck() NEQ NULL);

      StmtDDLAddConstraintCheck * pAddConstraintCheck =
        new(PARSERHEAP())
	  StmtDDLAddConstraintCheck(getTableNameAsQualifiedName(), pElement);

      pAddConstraintCheck->setIsParseSubTreeDestroyedByDestructor(FALSE);
      addConstraintArray_.insert(pAddConstraintCheck);

      addConstraintCheckArray_.insert(pAddConstraintCheck);
    }
    break;

  case ELM_CONSTRAINT_REFERENTIAL_INTEGRITY_ELEM :
    {
      ComASSERT(pElement->castToElemDDLConstraintRI() NEQ NULL);

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

      StmtDDLAddConstraintUnique * pAddConstraintUnique =
        new(PARSERHEAP())
	  StmtDDLAddConstraintUnique(getTableNameAsQualifiedName(),
				     pElement);

      pAddConstraintUnique->setIsParseSubTreeDestroyedByDestructor(FALSE);
      addConstraintArray_.insert(pAddConstraintUnique);

      addConstraintUniqueArray_.insert(pAddConstraintUnique);
    }
    break;

  case ELM_CONSTRAINT_PRIMARY_KEY_ELEM :
  case ELM_CONSTRAINT_PRIMARY_KEY_COLUMN_ELEM :
    {
      pAddConstraintPK_ = new(PARSERHEAP())
	StmtDDLAddConstraintPK(getTableNameAsQualifiedName(), pElement);

      pAddConstraintPK_->setIsParseSubTreeDestroyedByDestructor(FALSE);
      addConstraintArray_.insert(pAddConstraintPK_);
    }
    break;

  default :
    NAAbort("StmtDDLAlter.C", __LINE__, "internal logic error");
    break;
  }
} // StmtDDLAlterTableAddColumn::setConstraint()

//
// methods for tracing
//

const NAString
StmtDDLAlterTableAddColumn::getText() const
{
  return "StmtDDLAlterTableAddColumn";
}





// -----------------------------------------------------------------------
// methods for class StmtDDLAlterLibrary
// -----------------------------------------------------------------------

//
// constructor
//
// constructor used for ALTER LIBRARY
StmtDDLAlterLibrary::StmtDDLAlterLibrary(
   const QualifiedName   & libraryName,
   const NAString        & fileName,
   ElemDDLNode           * clientName,
   ElemDDLNode           * clientFilename,
   CollHeap              * heap)  
 : StmtDDLNode(DDL_ALTER_LIBRARY),
   libraryName_(libraryName,heap),
   fileName_(fileName),
   clientName_("",heap),
   clientFilename_("",heap)
    
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

} 

// virtual destructor
StmtDDLAlterLibrary::~StmtDDLAlterLibrary()
{
}

// virtual cast
StmtDDLAlterLibrary *
StmtDDLAlterLibrary::castToStmtDDLAlterLibrary()
{
   return this;
}


//
// methods for tracing
//

const NAString
StmtDDLAlterLibrary::displayLabel1() const
{
   return NAString("Library name: ") + getLibraryName();
}

const NAString
StmtDDLAlterLibrary::displayLabel2() const
{

   return NAString("Filename: ") + getFilename();

}


const NAString
StmtDDLAlterLibrary::getText() const
{
  return "StmtDDLAlterLibrary";
}





//----------------------------------------------------------------------------
//++ MVS


StmtDDLAlterMV::StmtDDLAlterMV(QualifiedName & mvName, 
							   ComBoolean rewriteEnableStatus) :
			StmtDDLNode(DDL_ALTER_MV),
			alterType_(REWRITE), 
			MVQualName_(mvName, PARSERHEAP()),
			newName_("", PARSERHEAP()),
			isCascade_(FALSE),
			pFileAttrClause_(NULL),
			pMVFileAttrClause_(NULL),
			rewriteEnableStatus_(rewriteEnableStatus),
			pAuditCompress_(NULL),
			pClearOnPurge_(NULL),
                        pCompressionType_(NULL),
			pMaxExtents_(NULL),
			pMvAudit_(NULL),
			pCommitEach_(NULL),
			pLockOnRefresh_(NULL),
			pMvsAllowed_(NULL),
			numOfAttributes_(0),
			isFirstAttribute_(FALSE),
			attributesListString_("", PARSERHEAP()),
                        pIgnoreChangesList_(NULL)
{



}
StmtDDLAlterMV::StmtDDLAlterMV(QualifiedName & mvName, 
                               ElemDDLNode * fileAttributeClause) :
			StmtDDLNode(DDL_ALTER_MV),
			alterType_(BT_ATTRIBUTES), 
			MVQualName_(mvName, PARSERHEAP()),
			newName_("", PARSERHEAP()),
			isCascade_(FALSE),
			rewriteEnableStatus_(FALSE),
			pAuditCompress_(NULL),
			pClearOnPurge_(NULL),
                        pCompressionType_(NULL),
			pMaxExtents_(NULL),
			pMvAudit_(NULL),
			pCommitEach_(NULL),
			pLockOnRefresh_(NULL),
			pMvsAllowed_(NULL),
			numOfAttributes_(0),
			isFirstAttribute_(TRUE),
			attributesListString_("", PARSERHEAP()),
                        pIgnoreChangesList_(NULL)
{
	pFileAttrClause_ = fileAttributeClause->castToElemDDLFileAttrClause();

	pMVFileAttrClause_ = fileAttributeClause->castToElemDDLMVFileAttrClause();

	if ((NULL == pFileAttrClause_ && NULL == pMVFileAttrClause_) || 
		(NULL != pFileAttrClause_ && NULL != pMVFileAttrClause_))
	{
		ComASSERT(FALSE);		
	}

	if(NULL != pFileAttrClause_)
	{
		alterType_ = BT_ATTRIBUTES;
	}
	else // pMVFileAttrClause_
	{
		alterType_ = MV_ATTRIBUTES;
	}
		
}



StmtDDLAlterMV::StmtDDLAlterMV(QualifiedName & mvName,
                               const NAString & newName,
                               ComBoolean isCascade) :
			StmtDDLNode(DDL_ALTER_MV),
			alterType_(RENAME), 
			MVQualName_(mvName, PARSERHEAP()),
			newName_(newName, PARSERHEAP()),
			isCascade_(isCascade),
			pFileAttrClause_(NULL),
			pMVFileAttrClause_(NULL),
			rewriteEnableStatus_(FALSE),
			pAuditCompress_(NULL),
			pClearOnPurge_(NULL),
                        pCompressionType_(NULL),
			pMvAudit_(NULL),
			pCommitEach_(NULL),
			pLockOnRefresh_(NULL),
			pMvsAllowed_(NULL),
			numOfAttributes_(0),
			isFirstAttribute_(FALSE),
			attributesListString_("", PARSERHEAP()),
                        pIgnoreChangesList_(NULL)
{

}

StmtDDLAlterMV::StmtDDLAlterMV(QualifiedName & mvName,
                   ElemDDLNode *ignoreChangesList,
                   AlterType alterType) :
                        StmtDDLNode(DDL_ALTER_MV),
                        alterType_(alterType),
                        MVQualName_(mvName, PARSERHEAP()),
                        newName_("", PARSERHEAP()),
                        isCascade_(FALSE),
                        pFileAttrClause_(NULL),
                        pMVFileAttrClause_(NULL),
                        rewriteEnableStatus_(FALSE),
                        pAuditCompress_(NULL),
                        pClearOnPurge_(NULL),
                        pCompressionType_(NULL),
                        pMvAudit_(NULL),
                        pCommitEach_(NULL),
                        pLockOnRefresh_(NULL),
                        pMvsAllowed_(NULL),
                        numOfAttributes_(0),
                        isFirstAttribute_(FALSE),
                        attributesListString_("", PARSERHEAP()),
                        pIgnoreChangesList_(NULL)
{
  pIgnoreChangesList_ = ignoreChangesList
                            ->castToElemDDLCreateMVOneAttributeTableList();
}


void StmtDDLAlterMV::synthesize()
{
	if (NULL != pFileAttrClause_)
	{
		processFileAttributeClause(BT_ATTRIBUTES);
	}

	if (NULL != pMVFileAttrClause_)
	{
		processFileAttributeClause(MV_ATTRIBUTES);
	}
}



//----------------------------------------------------------------------------
void
StmtDDLAlterMV::processFileAttributeClause(AlterType type) 
{
	ElemDDLNode * pFileAttrs = NULL;

	switch(type)
	{
	case BT_ATTRIBUTES:
		pFileAttrs = pFileAttrClause_->getFileAttrDefBody();
		break;
	case MV_ATTRIBUTES:
		pFileAttrs = pMVFileAttrClause_->getFileAttrDefBody();
		break;
	}
	
	ComASSERT(pFileAttrs NEQ NULL);

	for (CollIndex i = 0; i < pFileAttrs->entries(); i++)
	{
		checkFileAttribute((*pFileAttrs)[i]->castToElemDDLFileAttr());
	}

	attributesListString_ += getAuditCompressString();
	attributesListString_ += getClearOnPurgeString();
        attributesListString_ += getCompressionTypeString();
	attributesListString_ += getMaxExtentsString();
	attributesListString_ += getLockOnRefreshString();
	attributesListString_ += getMvsAllowedString();

}






//----------------------------------------------------------------------------
void 
StmtDDLAlterMV::checkFileAttribute(ElemDDLFileAttr * pFileAttr) 
{
        ULng32 maxext = 0;

	switch (pFileAttr->getOperatorType())
	{

	// ALLOWED
	case ELM_FILE_ATTR_AUDIT_COMPRESS_ELEM	:
		if(NULL != pAuditCompress_)
		{
			*SqlParser_Diags << DgSqlCode(-12053); 
		}

		pAuditCompress_ = pFileAttr->castToElemDDLFileAttrAuditCompress();
		ComASSERT(NULL != pAuditCompress_);

		numOfAttributes_++;
		break;
	
	case ELM_FILE_ATTR_CLEAR_ON_PURGE_ELEM	:
		if(NULL != pClearOnPurge_)
		{
			*SqlParser_Diags << DgSqlCode(-12053); 
		}

		pClearOnPurge_ = pFileAttr->castToElemDDLFileAttrClearOnPurge();
		numOfAttributes_++;
		ComASSERT(NULL != pClearOnPurge_);	
		break;

        case ELM_FILE_ATTR_COMPRESSION_ELEM :
                *SqlParser_Diags << DgSqlCode(-12049);
                break;

        case ELM_FILE_ATTR_MAXEXTENTS_ELEM :
		if(NULL != pMaxExtents_)
		{
			*SqlParser_Diags << DgSqlCode(-12053); 
		}
                pMaxExtents_ = pFileAttr->castToElemDDLFileAttrMaxExtents();
                numOfAttributes_++;
                ComASSERT(NULL != pMaxExtents_);

                maxext = pMaxExtents_->getMaxExtents();
                if ((maxext <= 0) || (maxext > COM_MAX_MAXEXTENTS))
                {
                       *SqlParser_Diags << DgSqlCode(-3191);
                }
		break;



	case ELM_FILE_ATTR_MV_COMMIT_EACH_ELEM	:
		if(NULL != pCommitEach_)
		{
			*SqlParser_Diags << DgSqlCode(-12059); 
		}

		pCommitEach_ = pFileAttr->castToElemDDLFileAttrMVCommitEach();
		numOfAttributes_++;
		ComASSERT(NULL != pCommitEach_);	
		break;


	case ELM_FILE_ATTR_MVAUDIT_ELEM	:
		if(NULL != pMvAudit_)
		{
			*SqlParser_Diags << DgSqlCode(-3082); 
		}

		pMvAudit_ = pFileAttr->castToElemDDLFileAttrMvAudit();
		numOfAttributes_++;
		ComASSERT(NULL != pMvAudit_);	
		break;

						
	case ELM_FILE_ATTR_LOCK_ON_REFRESH_ELEM:
		if (NULL != pLockOnRefresh_)
		{
		  // Duplicate [NO]LOCKONREFRESH phrases.
		  *SqlParser_Diags << DgSqlCode(-12055);
		}
		pLockOnRefresh_ = pFileAttr->castToElemDDLFileAttrLockOnRefresh();
		ComASSERT(NULL != pLockOnRefresh_ );	
		break;


	case ELM_FILE_ATTR_MVS_ALLOWED_ELEM:
		if (NULL != pMvsAllowed_)
		{
		  // Duplicate MVS ALLOWED phrases.
		  *SqlParser_Diags << DgSqlCode(-12058);
		}
		pMvsAllowed_ = pFileAttr->castToElemDDLFileAttrMvsAllowed();
		ComASSERT(NULL != pMvsAllowed_);	
		break;


	
	// NOT ALLOWED
	case ELM_FILE_ATTR_BLOCK_SIZE_ELEM		:
	case ELM_FILE_ATTR_BUFFERED_ELEM		:
	case ELM_FILE_ATTR_D_COMPRESS_ELEM		:
	case ELM_FILE_ATTR_I_COMPRESS_ELEM		:
	case ELM_FILE_ATTR_MAX_SIZE_ELEM		:
	case ELM_FILE_ATTR_ALLOCATE_ELEM		:
	case ELM_FILE_ATTR_LOCK_LENGTH_ELEM		:
	case ELM_FILE_ATTR_AUDIT_ELEM			:
	case ELM_FILE_ATTR_DEALLOCATE_ELEM		:
	case ELM_FILE_ATTR_RANGE_LOG_ELEM		:
	case ELM_FILE_ATTR_INSERT_LOG_ELEM		:


		*SqlParser_Diags << DgSqlCode(-12052); 
		break;

	default :
		NAAbort("StmtDDLAlter.CPP", __LINE__, "internal logic error");
		break;
	}



} // StmtDDLAlterMV::checkFileAttribute

const NAString StmtDDLAlterMV::getAuditCompressString() 
{
	NAString text;

	if (NULL != pAuditCompress_)
	{
		if (FALSE == isFirstAttribute_)
		{
			text += ", ";
		}
		else
		{
			isFirstAttribute_ = FALSE; // for the next one
		}
		
		if ( FALSE == pAuditCompress_->getIsAuditCompress())
		{
			text = "NO ";
		}

		text += "AUDITCOMPRESS ";
	}

	return text;
}

const NAString StmtDDLAlterMV::getClearOnPurgeString() 
{
	NAString text;
	
	if (NULL != pClearOnPurge_) // no attribute
	{
		
		if (FALSE == isFirstAttribute_)
		{
			text += ", ";
		}
		else
		{
			isFirstAttribute_ = FALSE; // for the next one
		}

		if ( FALSE == pClearOnPurge_->getIsClearOnPurge())
		{
			text = "NO ";
		}

		text += "CLEARONPURGE ";
	}

	return text;
}

const NAString StmtDDLAlterMV::getCompressionTypeString()
{
        NAString text;

        if (NULL != pCompressionType_) // no attribute
        {

                if (FALSE == isFirstAttribute_)
                {
                        text += ", ";
                }
                else
                {
                        isFirstAttribute_ = FALSE; // for the next one
                }

                switch (pCompressionType_->getCompressionType())
                {
                    case COM_HARDWARE_COMPRESSION:
                      text = "COMPRESSION TYPE HARDWARE ";
                      break;

                    case COM_SOFTWARE_COMPRESSION:
                       text = "COMPRESSION TYPE SOFTWARE ";
                       break;

                    default:
                       text = "COMPRESSION TYPE NONE ";
                       break;
                }
        }

        return text;
}

const NAString StmtDDLAlterMV::getMaxExtentsString()
{
        NAString text;

        if (NULL != pMaxExtents_) // no attribute
        {

                if (FALSE == isFirstAttribute_)
                {
                        text += ", ";
                }
                else
                {
                        isFirstAttribute_ = FALSE; // for the next one
                }

                text = "MAXEXTENTS ";
                text += LongToNAString(pMaxExtents_->getMaxExtents());
        }

        return text;
}

const NAString StmtDDLAlterMV::getLockOnRefreshString()
{
	NAString text;
	
	if (NULL != pLockOnRefresh_) // no attribute
	{
		
		if (FALSE == isFirstAttribute_)
		{
			text += ", ";
		}
		else
		{
			isFirstAttribute_ = FALSE; // for the next one
		}

		if ( FALSE == pLockOnRefresh_->isLockOnRefresh())
		{
			text = "NO ";
		}

		text += "LOCKONREFRESH ";
	}

	return text;

}

const NAString StmtDDLAlterMV::getMvsAllowedString()
{
	NAString text;
	
	if (NULL != pMvsAllowed_) // no attribute
	{
		
		if (FALSE == isFirstAttribute_)
		{
			text += ", ";
		}
		else
		{
			isFirstAttribute_ = FALSE; // for the next one
		}

		text += ElemDDLFileAttrMvsAllowed::getMvsAllowedTypeAsNAString(
										pMvsAllowed_->getMvsAllowedType());

		text += " MVS ALLOWED ";
	}

	return text;
}




//----------------------------------------------------------------------------
// MV - RG

StmtDDLAlterMvRGroup::StmtDDLAlterMvRGroup(const QualifiedName & mvRGroupName,
						alterMvGroupType action,
						ElemDDLNode * pMVList)
	:StmtDDLNode(DDL_ALTER_MV_REFRESH_GROUP),
		mvRGroupQualName_(mvRGroupName, PARSERHEAP()),
		pMVList_(pMVList),
		action_(action),
		listIndex_(0)
{



}

StmtDDLAlterMvRGroup::~StmtDDLAlterMvRGroup()
{

 // should i release anything? probably

}
StmtDDLAlterMvRGroup * 
StmtDDLAlterMvRGroup::castToStmtDDLAlterMvRGroup()
{
	return this;


}

QualifiedName & 
StmtDDLAlterMvRGroup::getFirstMvInList()
{
	listIndex_ = 0;
	return getNextMvInList();
}
ComBoolean	
StmtDDLAlterMvRGroup::listHasMoreMVs() const
{
	return (listIndex_ <  pMVList_->entries()) ? TRUE : FALSE ;
}

QualifiedName & 
StmtDDLAlterMvRGroup::getNextMvInList()
{
	ComASSERT(TRUE == listHasMoreMVs());	

	return ((*pMVList_)[listIndex_++]->
	castToElemDDLQualName())->getQualifiedName();
}

StmtDDLAlterMvRGroup::alterMvGroupType 
StmtDDLAlterMvRGroup::getAction() const
{

	return action_;

}

const NAString 
StmtDDLAlterMvRGroup::displayLabel1() const
{

  return NAString("MV name: ") + getMvRGroupName();
	

}
const NAString 
StmtDDLAlterMvRGroup::getText() const
{
  return "StmtDDLAlterMvRGroup";
}


//-- MVS
//----------------------------------------------------------------------------




//
// Virtual destructor 
//

StmtDDLAlterTrigger::~StmtDDLAlterTrigger() {}

// cast virtual function
StmtDDLAlterTrigger *
StmtDDLAlterTrigger::castToStmtDDLAlterTrigger()
{
  return this;
}


//
// methods for tracing
//

const NAString
StmtDDLAlterTrigger::getText() const
{
  return "StmtDDLAlterTrigger";
}

const NAString
StmtDDLAlterTrigger::displayLabel1() const
{
  return NAString("TriggerOrTable name: ") + getTriggerOrTableName();
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAlterView
// -----------------------------------------------------------------------

// constructor
StmtDDLAlterView::StmtDDLAlterView(QualifiedName & viewName,
                                   const NAString & newName) :
  StmtDDLNode(DDL_ALTER_VIEW),
  alterType_(RENAME),
  viewQualName_(viewName, PARSERHEAP()),
  newName_(newName, PARSERHEAP()),
  cascade_(FALSE)
{

}

// constructor
StmtDDLAlterView::StmtDDLAlterView(QualifiedName & viewName, 
				   const NABoolean  cascade) :
  StmtDDLNode(DDL_ALTER_VIEW),
  alterType_(COMPILE),
  viewQualName_(viewName, PARSERHEAP()),
  cascade_(cascade)
{

}

// virtual destructor 
StmtDDLAlterView::~StmtDDLAlterView() {}

// cast virtual function
StmtDDLAlterView *
StmtDDLAlterView::castToStmtDDLAlterView()
{
  return this;
}

//
// methods for tracing
//

// virtual
const NAString
StmtDDLAlterView::displayLabel1() const
{  
  return NAString("View name: ") + getViewName();  
}
  
// virtual
const NAString
StmtDDLAlterView::getText() const
{
  return "StmtDDLAlterView";      
}

//
// accessors
//

QualifiedName  &
StmtDDLAlterView::getViewNameAsQualifiedName()
{
  return viewQualName_;
}

const QualifiedName &
StmtDDLAlterView::getViewNameAsQualifiedName() const
{
  return viewQualName_;
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAlterRoutine
// -----------------------------------------------------------------------

// constructor
StmtDDLAlterRoutine::StmtDDLAlterRoutine(ComAnsiNameSpace      eNameSpace,
                                         const QualifiedName & aRoutineName,
                                         const QualifiedName & anActionName,
                                         ComRoutineType        eRoutineType,
                                         ElemDDLNode         * pAlterPassThroughParamParseTree,
                                         ElemDDLNode         * pAddPassThroughParamParseTree,
                                         ElemDDLNode         * pRoutineAttributesParseTree,
                                         CollHeap            * heap)
  : StmtDDLCreateRoutine(aRoutineName,
                         anActionName,
                         NULL,                          // in - ElemDDLNode * pParamList
                         NULL,                          // in - ElemDDLNode * pReturnsList
                         pAddPassThroughParamParseTree, // in - ElemDDLNode * pPassThroughParamList
                         pRoutineAttributesParseTree,   // in - ElemDDLNode * pOptionList
                         eRoutineType,                  // in - ComRoutinetype
                         heap),                         // in - CollHeap * heap);
    nameSpace_(eNameSpace),
    alterPassThroughInputsParseTree_(pAlterPassThroughParamParseTree)
{
  setOperatorType(DDL_ALTER_ROUTINE); // overwrite the setting done by StmtDDLCreateRoutine() call
}

// virtual destructor
StmtDDLAlterRoutine::~StmtDDLAlterRoutine()
{
}

// virtual cast
StmtDDLAlterRoutine *
StmtDDLAlterRoutine::castToStmtDDLAlterRoutine()
{
  return this;
}

//
// helper
//

void StmtDDLAlterRoutine::synthesize()
{
  ElemDDLNode *pPassThroughList = getAlterPassThroughInputsParseTree();
  if (pPassThroughList NEQ NULL)
  {
    CollIndex i4 = 0;
    CollIndex nbrPassThroughList = pPassThroughList->entries();
    ElemDDLPassThroughParamDef * passThroughDef = NULL;
    for (i4 = 0; i4 < nbrPassThroughList; i4++)
    {
      passThroughDef = (*pPassThroughList)[i4]->castToElemDDLPassThroughParamDef();
      ComASSERT(passThroughDef NEQ NULL);
      alterPassThroughParamArray_.insert(passThroughDef); // ALTER PASS THROUGH INPUTS
    } // for (i4 = 0; i4 < nbrPassThroughList; i4++)
  }

  StmtDDLCreateRoutine::synthesize();

} // StmtDDLAlterRoutine::synthesize()

//
// virtual tracing functions
//

const NAString StmtDDLAlterRoutine::displayLabel1(void) const
{
  return NAString("Routine name: ") + getRoutineName();
}

const NAString StmtDDLAlterRoutine::displayLabel2(void) const
{
  NAString actionName(getActionNameAsAnsiString());
  if (NOT actionName.isNull())
    return "Routine action name: " + actionName;
  else
    return "No routine action name";
}

NATraceList StmtDDLAlterRoutine::getDetailInfo(void) const
{
  NAString        detailText;
  NATraceList detailTextList;

  //
  // routine name and routine action name
  //

  detailTextList.append(displayLabel1());
  detailTextList.append(displayLabel2());

  return detailTextList;
}

const NAString StmtDDLAlterRoutine::getText(void) const
{
  return "StmtDDLAlterKRoutine";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLAlterUser
// -----------------------------------------------------------------------

//
// constructor
//
// constructor used for ALTER USER
StmtDDLAlterUser::StmtDDLAlterUser(
   const NAString        & databaseUsername,
   AlterUserCmdSubType     cmdSubType,
   const NAString        * pExternalName,
   NABoolean               isValidUser,
   CollHeap              * heap) 
 : StmtDDLNode(DDL_ALTER_USER),
   databaseUserName_(databaseUsername,heap),
   alterUserCmdSubType_(cmdSubType),
   isValidUser_(isValidUser)

{

   if (pExternalName == NULL)
      externalUserName_ = ComString("",heap);
   else
   {
     NAString userName(*pExternalName,heap);
     externalUserName_ = userName;
   }
   
} 

// virtual destructor
StmtDDLAlterUser::~StmtDDLAlterUser()
{
}

// virtual cast
StmtDDLAlterUser *
StmtDDLAlterUser::castToStmtDDLAlterUser()
{
   return this;
}


//
// methods for tracing
//

const NAString
StmtDDLAlterUser::displayLabel1() const
{
   return NAString("Database username: ") + getDatabaseUsername();
}

const NAString
StmtDDLAlterUser::displayLabel2() const
{

   if (NOT getExternalUsername().isNull())
      return NAString("External username: ") + getExternalUsername();

   return NAString("External username not specified.");
   
}


const NAString
StmtDDLAlterUser::getText() const
{
  return "StmtDDLAlterUser";
}



//
// End of File
//
