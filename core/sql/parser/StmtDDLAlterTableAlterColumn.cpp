


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
     const NAString &columnName 
     , NAType * natype
     , CollHeap *heap)
     : StmtDDLAlterTableAlterColumn(DDL_ALTER_TABLE_ALTER_COLUMN_DATATYPE,
                                    columnName,
                                    NULL,
                                    heap)
{
  natype_ = natype->newCopy(heap);
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
	
