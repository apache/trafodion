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
 * File:         StmtDDLDrop.C
 * Description:  definitions of methods associating with DDL Drop
 *               statements
 *               
 *               
 * Created:      11/11/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "AllStmtDDLDrop.h"

#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"

// -----------------------------------------------------------------------
// methods for class StmtDDLDropCatalog
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLDropCatalog::StmtDDLDropCatalog(const NAString & catalogName, ComDropBehavior dropBehavior)
        : StmtDDLNode(DDL_DROP_CATALOG),
          catalogName_(catalogName, PARSERHEAP()), 
          dropBehavior_(dropBehavior)
{
}

//
// virtual destructor
//
StmtDDLDropCatalog::~StmtDDLDropCatalog()
{
}

//
// cast
//
StmtDDLDropCatalog *
StmtDDLDropCatalog::castToStmtDDLDropCatalog()
{
  return this;
}

//
// methods for tracing
//

const NAString
StmtDDLDropCatalog::displayLabel1() const
{
  return NAString("Catalog name: ") + getCatalogName();
}

const NAString
StmtDDLDropCatalog::getText() const
{
  return "StmtDDLDropCatalog";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLDropComponentPrivilege
// -----------------------------------------------------------------------

//
// constructor
//

StmtDDLDropComponentPrivilege::StmtDDLDropComponentPrivilege(
   const NAString & aComponentPrivilegeName,
   const NAString & aComponentName,
   ComDropBehavior dropBehavior, 
   CollHeap       * heap) // default is PARSERHEAP()
  : StmtDDLNode(DDL_DROP_COMPONENT_PRIVILEGE),
    componentPrivilegeName_(aComponentPrivilegeName, heap),
    dropBehavior_(dropBehavior),
    componentName_(aComponentName, heap)
{
}

//
// virtual destructor
//

StmtDDLDropComponentPrivilege::~StmtDDLDropComponentPrivilege()
{
}

//
// virtual safe cast-down function
//

StmtDDLDropComponentPrivilege *
StmtDDLDropComponentPrivilege::castToStmtDDLDropComponentPrivilege()
{
  return this;
}

//
// methods for tracing
//


const NAString
StmtDDLDropComponentPrivilege::displayLabel1() const
{
  NAString aLabel("Component privilege name: ");
  aLabel += getComponentPrivilegeName();
  return aLabel;
}

const NAString
StmtDDLDropComponentPrivilege::displayLabel2() const
{
  NAString aLabel("Component name: ");
  aLabel += getComponentName();
  aLabel += " Drop behavior: ";
  if (dropBehavior_ == COM_CASCADE_DROP_BEHAVIOR)
     aLabel += "CASCADE";
  else
     aLabel += "RESTRICT";
      
  return aLabel;
}

const NAString
StmtDDLDropComponentPrivilege::getText() const
{
  return "StmtDDLDropComponentPrivilege";
}


//----------------------------------------------------------------------------
// MV - RG

// methods for class StmtDDLDropMvRGroup  - refresh groups
// ---------------------------------------------------------------------------



  // initialize constructor
StmtDDLDropMvRGroup::StmtDDLDropMvRGroup(const QualifiedName & mvGroupName)
		:StmtDDLNode(DDL_DROP_MV_REFRESH_GROUP),
            mvRGroupQualName_(mvGroupName, PARSERHEAP()) 
{
	// XXXXXXXXXMVSXXXXXXXXXXXXXXX
}

StmtDDLDropMvRGroup::~StmtDDLDropMvRGroup()
{



}

StmtDDLDropMvRGroup * 
StmtDDLDropMvRGroup::castToStmtDDLDropMvRGroup()
{
	return this;


}


const NAString
StmtDDLDropMvRGroup::displayLabel1() const
{

  return NAString("MV name: ") + getMvRGroupName();
}

const NAString
StmtDDLDropMvRGroup::getText() const
{

  return "StmtDDLDropMvRGroup";


}

// -----------------------------------------------------------------------
// methods for class StmtDDLDropTrigger
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLDropTrigger::StmtDDLDropTrigger(const QualifiedName & triggerQualName,
                                       NABoolean cleanupSpec,
				       NABoolean validateSpec,
				       NAString * pLogFile)
        : StmtDDLNode(DDL_DROP_TRIGGER),
          triggerQualName_(triggerQualName, PARSERHEAP()),
	  isCleanupSpec_(cleanupSpec),
	  isValidateSpec_(validateSpec),
	  pLogFile_(pLogFile)
{}

//
// virtual destructor
//
StmtDDLDropTrigger::~StmtDDLDropTrigger()
{
  if (pLogFile_)
    delete pLogFile_;
}

//
// cast
//
StmtDDLDropTrigger *
StmtDDLDropTrigger::castToStmtDDLDropTrigger()
{
  return this;
}

//
// methods for tracing
//

const NAString
StmtDDLDropTrigger::displayLabel1() const
{
  return NAString("Trigger name: ") + getTriggerName();
}

const NAString
StmtDDLDropTrigger::getText() const
{
  return "StmtDDLDropTrigger";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLDropIndex
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLDropIndex::StmtDDLDropIndex(const QualifiedName & indexName,
                                   ComDropBehavior dropBehavior,
                                   NABoolean cleanupSpec,
				   NABoolean validateSpec,
				   NAString * pLogFile)
        : StmtDDLNode(DDL_DROP_INDEX),
          origIndexQualName_(PARSERHEAP()),
          indexQualName_(indexName, PARSERHEAP()),
          dropBehavior_(dropBehavior),
	  isCleanupSpec_(cleanupSpec),
	  isValidateSpec_(validateSpec),
	  pLogFile_(pLogFile)
{}

//
// virtual destructor
//
StmtDDLDropIndex::~StmtDDLDropIndex()
{
  if (pLogFile_)
    delete pLogFile_;
}

void
StmtDDLDropIndex::synthesize()
{
}

//
// cast
//
StmtDDLDropIndex *
StmtDDLDropIndex::castToStmtDDLDropIndex()
{
  return this;
}

//
// methods for tracing
//

const NAString
StmtDDLDropIndex::displayLabel1() const
{
  return NAString("Index name: ") + getIndexName();
}

const NAString
StmtDDLDropIndex::displayLabel2() const
{
  NAString label2("Drop behavior: ");
  switch (getDropBehavior())
  {
  case COM_CASCADE_DROP_BEHAVIOR :
    return label2 + "Cascade";

  case COM_RESTRICT_DROP_BEHAVIOR :
    return label2 + "Restrict";

  default :
    NAAbort("StmtDDLDrop.C", __LINE__, "internal logic error");
    return NAString();
  }
}

const NAString
StmtDDLDropIndex::getText() const
{
  return "StmtDDLDropIndex";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLDropLibrary
// -----------------------------------------------------------------------

//
// constructor
//

StmtDDLDropLibrary::StmtDDLDropLibrary(
     const QualifiedName & libraryName,
     ComDropBehavior       dropBehavior) 
  : StmtDDLNode(DDL_DROP_LIBRARY),
    libraryName_(libraryName, PARSERHEAP()),
    dropBehavior_(dropBehavior)
{

}

//
// virtual destructor
//

StmtDDLDropLibrary::~StmtDDLDropLibrary()
{}

//
// safe cast
//

StmtDDLDropLibrary *
StmtDDLDropLibrary::castToStmtDDLDropLibrary()
{
  return this;
} 

//
// methods for tracing
//

const NAString
StmtDDLDropLibrary::displayLabel1() const
{
  return NAString("Library name: ") + getLibraryName();
}

const NAString
StmtDDLDropLibrary::getText() const
{
  return "StmtDDLDropLibrary";
}





// -----------------------------------------------------------------------
// methods for class StmtDDLDropModule
// -----------------------------------------------------------------------

//
// constructor
//

StmtDDLDropModule::StmtDDLDropModule(const QualifiedName & modulename)
  : StmtDDLNode(DDL_DROP_MODULE),
    moduleName_(PARSERHEAP()),
    moduleQualName_(modulename, PARSERHEAP())
{
  moduleName_ = moduleQualName_.getQualifiedNameAsAnsiString();
}

//
// virtual destructor
//

StmtDDLDropModule::~StmtDDLDropModule()
{}

//
// safe cast
//

StmtDDLDropModule *
StmtDDLDropModule::castToStmtDDLDropModule()
{
  return this;
} 

//
// methods for tracing
//

const NAString
StmtDDLDropModule::displayLabel1() const
{
  return NAString("Module name: ") + getModuleName();
}

const NAString
StmtDDLDropModule::getText() const
{
  return "StmtDDLDropModule";
}



// -----------------------------------------------------------------------
// methods for class StmtDDLDropRoutine
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLDropRoutine::StmtDDLDropRoutine(ComRoutineType        routineType,
                                       const QualifiedName & routineName,
                                       const QualifiedName & routineActionName,
                                       ComDropBehavior       dropBehavior,
                                       NABoolean             cleanupSpec,
                                       NABoolean             validateSpec,
                                       NAString *            pLogFile,
                                       CollHeap *            heap)
  : StmtDDLNode(DDL_DROP_ROUTINE),
    routineType_(routineType),
    routineQualName_(routineName, heap),
    routineActionQualName_(routineActionName, heap),
    dropBehavior_(dropBehavior),
    isCleanupSpec_(cleanupSpec),
    isValidateSpec_(validateSpec),
    pLogFile_(pLogFile)
{
}


//
// virtual destructor
//
StmtDDLDropRoutine::~StmtDDLDropRoutine()
{
  if (pLogFile_)
    delete pLogFile_;
}

//
// cast
//
StmtDDLDropRoutine *
StmtDDLDropRoutine::castToStmtDDLDropRoutine()
{
  return this;
}

//
// methods for tracing
//

const NAString
StmtDDLDropRoutine::displayLabel1() const
{
  return NAString("Routine name: ") + getRoutineName();
}

const NAString
StmtDDLDropRoutine::displayLabel2() const
{
  NAString label2("Drop behavior: ");
  switch (getDropBehavior())
  {
  case COM_CASCADE_DROP_BEHAVIOR :
    return label2 + "Cascade";

  case COM_RESTRICT_DROP_BEHAVIOR :
    return label2 + "Restrict";

  default :
    NAAbort("StmtDDLDrop.C", __LINE__, "internal logic error");
    return NAString();
  }
}

const NAString
StmtDDLDropRoutine::getText() const
{
  return "StmtDDLDropRoutine";
}


// -----------------------------------------------------------------------
// methods for class StmtDDLDropSchema
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLDropSchema::StmtDDLDropSchema(//const SchemaName & schemaName,
				     const ElemDDLSchemaName & aSchemaNameParseNode,
                                     ComDropBehavior dropBehavior,
                                     ComBoolean cleanupMode,
                                     ComBoolean dropObjectsOnly)
        : StmtDDLNode(DDL_DROP_SCHEMA),
          schemaQualName_(aSchemaNameParseNode.getSchemaName(), PARSERHEAP()),
          dropBehavior_(dropBehavior),
          cleanupMode_(cleanupMode),
          dropObjectsOnly_(dropObjectsOnly),
          dropIfExists_(FALSE),
          schemaName_(PARSERHEAP())
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

  // If the schema name specified is reserved name, users cannot drop them.
  // They can only be dropped internally.
  if ((! Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)) &&
      (ComIsTrafodionReservedSchemaName(schemaQualName_.getSchemaName())) &&
      (!ComIsTrafodionExternalSchemaName(schemaQualName_.getSchemaName())))
    {
      // error.
      *SqlParser_Diags << DgSqlCode(-1430)
                       << DgSchemaName(schemaName_);
      
    }

}

//
// virtual destructor
//
StmtDDLDropSchema::~StmtDDLDropSchema()
{
}

//
// cast
//
StmtDDLDropSchema *
StmtDDLDropSchema::castToStmtDDLDropSchema()
{
  return this;
}

//
// methods for tracing
//

const NAString
StmtDDLDropSchema::displayLabel1() const
{
  return NAString("Schema name: ") + getSchemaName();
}

const NAString
StmtDDLDropSchema::displayLabel2() const
{
  NAString label2("Drop behavior: ");
  switch (getDropBehavior())
  {
  case COM_CASCADE_DROP_BEHAVIOR :
    return label2 + "Cascade";

  case COM_RESTRICT_DROP_BEHAVIOR :
    return label2 + "Restrict";

  default :
    NAAbort("StmtDDLDrop.C", __LINE__, "internal logic error");
    return NAString();
  }
}

const NAString
StmtDDLDropSchema::getText() const
{
  return "StmtDDLDropSchema";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLDropSequence
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLDropSequence::StmtDDLDropSequence(const QualifiedName & seqQualName,
                                       ElemDDLNode * pSequenceOptionList,
                                       CollHeap    * heap)
  : StmtDDLNode(DDL_DROP_SEQUENCE),
    seqQualName_(seqQualName, heap)
{

}

StmtDDLDropSequence::~StmtDDLDropSequence()
{
}
  
//
// cast virtual function
//
StmtDDLDropSequence *
StmtDDLDropSequence::castToStmtDDLDropSequence()
{
  return this;
}

//
// accessors
//

Int32
StmtDDLDropSequence::getArity() const
{
  return 0;
}

ExprNode *
StmtDDLDropSequence::getChild(Lng32 index)
{
  return NULL;
}

//
// methods for tracing
//

const NAString
StmtDDLDropSequence::displayLabel1() const
{
  return NAString("Sequence name: ") + seqQualName_.getQualifiedNameAsAnsiString();
}

NATraceList
StmtDDLDropSequence::getDetailInfo() const
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
StmtDDLDropSequence::getText() const
{
  return "StmtDDLDropSequence";
}

// method for collecting information
void StmtDDLDropSequence::synthesize()
{
}

// -----------------------------------------------------------------------
// methods for class StmtDDLDropSQL
// -----------------------------------------------------------------------

// 
// constructor
//

StmtDDLDropSQL::StmtDDLDropSQL(ComDropBehavior dropBehavior)
  : StmtDDLNode(DDL_DROP_SQL), 
    dropBehavior_(dropBehavior)
{}

// 
// virtual destructor
//

StmtDDLDropSQL::~StmtDDLDropSQL()
{}

StmtDDLDropSQL *
StmtDDLDropSQL::castToStmtDDLDropSQL()
{
  return this;
}

// 
// for tracing
//
const NAString
StmtDDLDropSQL::getText() const
{
  return "StmtDDLDropSQL";
}


// -----------------------------------------------------------------------
// methods for class StmtDDLDropTable
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLDropTable::StmtDDLDropTable(const QualifiedName & tableQualName,
                                   ComDropBehavior dropBehavior)
        : StmtDDLNode(DDL_DROP_TABLE),
          origTableQualName_(PARSERHEAP()),
          tableQualName_(tableQualName, PARSERHEAP()),
          dropBehavior_(dropBehavior),
	  tableType_(ExtendedQualName::NORMAL_TABLE), //++ MV
	  isSpecialTypeSpecified_(FALSE),  //++ MV
	  isCleanupSpec_(FALSE),
	  isValidateSpec_(FALSE),
	  pLogFile_(NULL),
	  dropIfExists_(FALSE)
{
}

//
// constructor for CLEANUP
//
StmtDDLDropTable::StmtDDLDropTable(const QualifiedName & tableQualName,
                                   ComDropBehavior dropBehavior,
                                   NABoolean cleanupSpec,
				   NABoolean validateSpec,
                                   NAString *pLogFile)
        : StmtDDLNode(DDL_DROP_TABLE),
          origTableQualName_(PARSERHEAP()),
          tableQualName_(tableQualName, PARSERHEAP()),
          dropBehavior_(dropBehavior),
	  tableType_(ExtendedQualName::NORMAL_TABLE), //++ MV
  	  isSpecialTypeSpecified_(FALSE),  //++ MV
	  isCleanupSpec_(cleanupSpec),
	  isValidateSpec_(validateSpec),
	  pLogFile_(pLogFile),
	  dropIfExists_(FALSE)
{
}

//
// virtual destructor
//
StmtDDLDropTable::~StmtDDLDropTable()
{
  if (pLogFile_)
    delete pLogFile_;
}

void
StmtDDLDropTable::synthesize()
{
}

//
// cast
//
StmtDDLDropTable *
StmtDDLDropTable::castToStmtDDLDropTable()
{
  return this;
}

//
// methods for tracing
//

const NAString
StmtDDLDropTable::displayLabel1() const
{
  return NAString("Table name: ") + getTableName();
}

const NAString
StmtDDLDropTable::displayLabel2() const
{
  NAString label2("Drop behavior: ");
  switch (getDropBehavior())
  {
  case COM_CASCADE_DROP_BEHAVIOR :
    return label2 + "Cascade";

  case COM_RESTRICT_DROP_BEHAVIOR :
    return label2 + "Restrict";

  default :
    NAAbort("StmtDDLDrop.C", __LINE__, "internal logic error");
    return NAString();
  }
}

const NAString
StmtDDLDropTable::getText() const
{
  return "StmtDDLDropTable";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLDropHbaseTable
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLDropHbaseTable::StmtDDLDropHbaseTable(const QualifiedName & tableQualName)
        : StmtDDLNode(DDL_DROP_HBASE_TABLE),
          origTableQualName_(PARSERHEAP()),
          tableQualName_(tableQualName, PARSERHEAP())
{
}

//
// virtual destructor
//
StmtDDLDropHbaseTable::~StmtDDLDropHbaseTable()
{
}

//
// cast
//
StmtDDLDropHbaseTable *
StmtDDLDropHbaseTable::castToStmtDDLDropHbaseTable()
{
  return this;
}

//
// methods for tracing
//

const NAString
StmtDDLDropHbaseTable::displayLabel1() const
{
  return NAString("Table name: ") + getTableName();
}

const NAString
StmtDDLDropHbaseTable::displayLabel2() const
{
  return NAString();
}

const NAString
StmtDDLDropHbaseTable::getText() const
{
  return "StmtDDLDropHbaseTable";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLDropView
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLDropView::StmtDDLDropView(const QualifiedName & viewQualName,
                                 ComDropBehavior dropBehavior,
                                 NABoolean cleanupSpec,
				 NABoolean validateSpec,
				 NAString * pLogFile)
        : StmtDDLNode(DDL_DROP_VIEW),
          viewQualName_(viewQualName, PARSERHEAP()),
          dropBehavior_(dropBehavior),
	  isCleanupSpec_(cleanupSpec),
	  isValidateSpec_(validateSpec),
	  pLogFile_(pLogFile),
          dropIfExists_(FALSE)
{
}

//
// virtual destructor
//
StmtDDLDropView::~StmtDDLDropView()
{
  if (pLogFile_)
    delete pLogFile_;
}

//
// cast
//
StmtDDLDropView *
StmtDDLDropView::castToStmtDDLDropView()
{
  return this;
}

//
// methods for tracing
//

const NAString
StmtDDLDropView::displayLabel1() const
{
  return NAString("View name: ") + getViewName();
}

const NAString
StmtDDLDropView::displayLabel2() const
{
  NAString label2("Drop behavior: ");
  switch (getDropBehavior())
  {
  case COM_CASCADE_DROP_BEHAVIOR :
    return label2 + "Cascade";

  case COM_RESTRICT_DROP_BEHAVIOR :
    return label2 + "Restrict";

  default :
    NAAbort("StmtDDLDrop.C", __LINE__, "internal logic error");
    return NAString();
  }
}

const NAString
StmtDDLDropView::getText() const
{
  return "StmtDDLDropView";
}

// -----------------------------------------------------------------------
// methods for class StmtDDLDropMV
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLDropMV::StmtDDLDropMV(const QualifiedName & MVQualName,
                                 ComDropBehavior dropBehavior,
                                 NABoolean cleanupSpec,
				 NABoolean validateSpec,
				 NAString * pLogFile)
        : StmtDDLNode(DDL_DROP_MV),
          MVQualName_(MVQualName, PARSERHEAP()),
          dropBehavior_(dropBehavior),
	  isCleanupSpec_(cleanupSpec),
	  isValidateSpec_(validateSpec),
	  pLogFile_(pLogFile)
{
}

//
// virtual destructor
//
StmtDDLDropMV::~StmtDDLDropMV()
{
  if (pLogFile_)
    delete pLogFile_;
}

//
// cast
//
StmtDDLDropMV *
StmtDDLDropMV::castToStmtDDLDropMV()
{
  return this;
}

//
// methods for tracing
//

const NAString
StmtDDLDropMV::displayLabel1() const
{
  return NAString("Materialized View name: ") + getMVName();
}

const NAString
StmtDDLDropMV::displayLabel2() const
{
  NAString label2("Drop behavior: ");
  switch (getDropBehavior())
  {
  case COM_CASCADE_DROP_BEHAVIOR :
    return label2 + "Cascade";

  case COM_RESTRICT_DROP_BEHAVIOR :
    return label2 + "Restrict";

  default :
    NAAbort("StmtDDLDrop.C", __LINE__, "internal logic error");
    return NAString();
  }
}

const NAString
StmtDDLDropMV::getText() const
{
  return "StmtDDLDropMV";
}

//-----------------------------------------------------------------------
// methods for class StmtDDLDropSynonym
//-----------------------------------------------------------------------

//
// constructor
//

StmtDDLDropSynonym::StmtDDLDropSynonym(const QualifiedName & synonymName)
  : StmtDDLNode (DDL_DROP_SYNONYM),
    synonymName_(synonymName, PARSERHEAP())
{
}

//
// Virtual destructor
//

StmtDDLDropSynonym::~StmtDDLDropSynonym()
{}

//
// cast
//

StmtDDLDropSynonym *
StmtDDLDropSynonym::castToStmtDDLDropSynonym()
{
  return this;
}

//
// for tracing
//

const NAString
StmtDDLDropSynonym::displayLabel1() const
{
  return NAString ("Synonym name: ") + getSynonymName();
}

const NAString
StmtDDLDropSynonym::getText() const
{
  return "StmtDropSynonym";
}

//-----------------------------------------------------------------------
// methods for class StmtDDLDropExceptionTable
//-----------------------------------------------------------------------

//
// constructor
//

StmtDDLDropExceptionTable::StmtDDLDropExceptionTable(const QualifiedName & exceptionName,
                                                     const QualifiedName & objectReference,
                                                           ComDropBehavior dropBehavior,
                                                           NABoolean cleanupSpec,
	                                                   NAString * pLogFile)
  : StmtDDLNode (DDL_DROP_EXCEPTION_TABLE),
    exceptionName_(exceptionName, PARSERHEAP()),
    objectReference_(objectReference, PARSERHEAP()),
    dropBehavior_(dropBehavior),
    isCleanupSpec_(cleanupSpec),
    dropType_(COM_DROP_SINGLE),
    pLogFile_(pLogFile)
{
}


StmtDDLDropExceptionTable::StmtDDLDropExceptionTable(const QualifiedName & objectReference,
                                                           ComDropBehavior dropBehavior,
                                                           NABoolean cleanupSpec,
	                                                   NAString * pLogFile)
  : StmtDDLNode (DDL_DROP_EXCEPTION_TABLE),
    exceptionName_(NULL),
    objectReference_(objectReference, PARSERHEAP()),
    dropBehavior_(dropBehavior),
    isCleanupSpec_(cleanupSpec),
    dropType_(COM_DROP_ALL),
    pLogFile_(pLogFile)
{
}


//
// Virtual destructor
//

StmtDDLDropExceptionTable::~StmtDDLDropExceptionTable()
{}

//
// cast
//

StmtDDLDropExceptionTable *
StmtDDLDropExceptionTable::castToStmtDDLDropExceptionTable()
{
  return this;
}

//
// for tracing
//

const NAString
StmtDDLDropExceptionTable::displayLabel1() const
{
  return NAString ("Exception table name: ") + getExceptionName();
}

const NAString
StmtDDLDropExceptionTable::displayLabel2() const
{
  return NAString ("Table name: ") + getObjectReference();
}

const NAString
StmtDDLDropExceptionTable::getText() const
{
  return "StmtDropExceptionTable";
}
  
//
// End of File
//
