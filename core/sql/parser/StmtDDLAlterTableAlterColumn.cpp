/**********************************************************************
// @@@ START COPYRIGHT @@@
//
//// Licensed to the Apache Software Foundation (ASF) under one
//// or more contributor license agreements.  See the NOTICE file
//// distributed with this work for additional information
//// regarding copyright ownership.  The ASF licenses this file
//// to you under the Apache License, Version 2.0 (the
//// "License"); you may not use this file except in compliance
//// with the License.  You may obtain a copy of the License at
////
////   http://www.apache.org/licenses/LICENSE-2.0
////
//// Unless required by applicable law or agreed to in writing,
//// software distributed under the License is distributed on an
//// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
//// KIND, either express or implied.  See the License for the
//// specific language governing permissions and limitations
//// under the License.
////
//// @@@ END COPYRIGHT @@@
//**********************************************************************/


//----------------------------------------------------------------------------
// CLASS StmtDDLAlterTableAlterColumn
//----------------------------------------------------------------------------
StmtDDLAlterTableAlterColumn::StmtDDLAlterTableAlterColumn(
     OperatorTypeEnum operatorType
     , const NAString &columnName 
     , ElemDDLNode *pColDefault
     , CollHeap *heap)
     : StmtDDLAlterTable(operatorType,
                         QualifiedName(PARSERHEAP()) /*no table name*/,
                         pColDefault),
       columnName_(columnName, heap)
{
}

//
// Virtual destructor
//

StmtDDLAlterTableAlterColumn::~StmtDDLAlterTableAlterColumn()
{}

const NAString
StmtDDLAlterTableAlterColumn::getText() const
{
  return "StmtDDLAlterTableAlterColumn" ;
}
	

//----------------------------------------------------------------------------
// CLASS StmtDDLAlterTableAlterColumnDatatype
//----------------------------------------------------------------------------
StmtDDLAlterTableAlterColumnDatatype::StmtDDLAlterTableAlterColumnDatatype(
     ElemDDLNode * pColumnToAlter
     ,CollHeap    * heap)
     : StmtDDLAlterTableAlterColumn(DDL_ALTER_TABLE_ALTER_COLUMN_DATATYPE,
                                    NAString(""),
                                    NULL,
                                    heap),
       pColumnToAlter_(pColumnToAlter)
{
  ElemDDLColDef *pColDef = pColumnToAlter->castToElemDDLColDef();
  if (pColDef NEQ NULL)
    {
      getColDefArray().insert(pColDef);
    }
  else
    *SqlParser_Diags << DgSqlCode(-1001);
}

//
// Virtual destructor
//

StmtDDLAlterTableAlterColumnDatatype::~StmtDDLAlterTableAlterColumnDatatype()
{}

//
// Cast function: to provide the safe castdown to the current object
//

StmtDDLAlterTableAlterColumnDatatype *
StmtDDLAlterTableAlterColumnDatatype::castToStmtDDLAlterTableAlterColumnDatatype()
{
  return this;
}

const NAString
StmtDDLAlterTableAlterColumnDatatype::getText() const
{
  return "StmtDDLAlterTableAlterColumnDatatype" ;
}
	
//----------------------------------------------------------------------------
// CLASS StmtDDLAlterTableAlterColumnRename
//----------------------------------------------------------------------------
StmtDDLAlterTableAlterColumnRename::StmtDDLAlterTableAlterColumnRename( 
     const NAString &columnName,
     const NAString &renamedColumnName,
     CollHeap *heap)
    : StmtDDLAlterTableAlterColumn(DDL_ALTER_TABLE_ALTER_COLUMN_RENAME, 
                                   columnName,
                                   NULL,
                                   heap),
      renamedColumnName_(renamedColumnName)
{
}

//
// Virtual destructor
//

StmtDDLAlterTableAlterColumnRename::~StmtDDLAlterTableAlterColumnRename()
{}

//
// Cast function: to provide the safe castdown to the current object
//

StmtDDLAlterTableAlterColumnRename *
StmtDDLAlterTableAlterColumnRename::castToStmtDDLAlterTableAlterColumnRename()
{
  return this;
}

const NAString
StmtDDLAlterTableAlterColumnRename::getText() const
{
  return "StmtDDLAlterTableAlterColumnRename" ;
}
	

//----------------------------------------------------------------------------
// CLASS StmtDDLAlterTableAlterColumnDefaultValue
//----------------------------------------------------------------------------
StmtDDLAlterTableAlterColumnDefaultValue::StmtDDLAlterTableAlterColumnDefaultValue( 
     const NAString &columnName 
     , ElemDDLNode *pColDefault
     , CollHeap *heap)
    : StmtDDLAlterTableAlterColumn(DDL_ALTER_TABLE_ALTER_COLUMN_DEFAULT_VALUE, 
                                   columnName,
                                   pColDefault,
                                   heap)
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
	
