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
 * File:         CmpStoredProc.C
 * Description:  The implementation of the internal stored procedure related
 *               classes.
 *
 *
 * Created:      03/14/97
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#define SQLPARSERGLOBALS_FLAGS
#define SQLPARSERGLOBALS_NADEFAULTS

#include "CmpStoredProc.h"
#include "TrafDDLdesc.h"
#include "parser.h"
#include "str.h"

#include "ElemDDLColDef.h" 
#include "CmpErrors.h"
#include "CmpContext.h"
#include "CmpMessage.h"		// for CmpMessageISPRequest
#include "ComDiags.h"
#include "CmpDescribe.h"	// for sendAllControls

#include "CharType.h"
#include "NumericType.h"
#include "DatetimeType.h"
#include "DTICommonType.h"	// for DatetimeIntervalCommonType

#include "ItemColRef.h"		// for ConstValue
#include "ItemNAType.h"		// for NATypeToItem
#include "ItemOther.h"		// for ItemList

#include "NAExit.h"
#include "NAMemory.h"
#include "NAString.h"
#include "ParserMsg.h"
#include "StoredProcInterface.h"

#include <time.h>		// timestamp to generate a unique table name
#include <memory.h>		// for memset
#include "SqlParserGlobalsCmn.h"


// helper routines used in this cpp file

static const char emptyString[] = "";

inline static char *copyString(const NAString &s)	// cf. readRealArk.cpp
{
  char *c = new(CmpCommon::statementHeap()) char[s.length()+1];
  str_cpy_all(c, s.data(), s.length()+1);
  return c;
}

static void CmpSPERROR2Diags(const SP_ERROR_STRUCT* spError, 
                             ComDiagsArea* diags)
{
  if ( !diags )
    return;

  if (spError[0].error == arkcmpErrorISPMergeCatDiags)
  {
    // Special error message to indicate that errors should really be
    // merged in from the catalog manager diags area.
    // not supported, raise unsupported error instead
    *diags << DgSqlCode(-4222)
           << DgString0("CmpSPERROR2Diags");
    return;
  }

  for ( Int32 i=0; i< SP_MAX_ERROR_STRUCTS; i++)
  {
    const SP_ERROR_STRUCT* pSET = &(spError[i]);
    if (pSET->error)
    {
      *diags << DgSqlCode(pSET->error);
      if(pSET->error == -20067) { //IDS_UTILITIES_BADSYNTAX = 20067
	// If utilities parser returned syntax error for syntax based 
	// utilities, error IDS_UTILITIES_BADSYNTAX is returned by 
	// utilities code to mxmcp.   
	// pSET->optionalString[1] has the utilities command from command line.
	// pSET->optionalInteger[0] has the approximate position of error in
	// the syntax of the utilities command given by user in command line.
	// The approximate error position is returned by utilities parser.

	// Function  StoreSyntaxError(.....) takes the information to put a 
	// caret (^) in the approximate position of the command and the 
	// information is saved in diags.
 
        StoreSyntaxError(pSET->optionalString[1],  // const char *      input_str
                         pSET->optionalInteger[0], // Int32             input_pos
                         *diags,                   // ComDiagsArea &    diagsArea
                         0,                        // Int32             dgStrNum
                         CharInfo::UTF8,           // CharInfo::CharSet input_str_cs
                         CharInfo::UTF8);          // CharInfo::CharSet terminal_cs
      }
      else{
      if ( pSET->optionalString[0] && 
           pSET->optionalString[0] != (char *) ComDiags_UnInitialized_Int )
        *diags << DgString0(pSET->optionalString[0]);
      if ( pSET->optionalString[1] &&
           pSET->optionalString[1] != (char *)ComDiags_UnInitialized_Int )
        *diags << DgString1(pSET->optionalString[1]);
      if ( pSET->optionalString[2] &&
           pSET->optionalString[2] != (char *)ComDiags_UnInitialized_Int )
        *diags << DgString2(pSET->optionalString[2]);
      if ( pSET->optionalString[3] &&
           pSET->optionalString[3] != (char *)ComDiags_UnInitialized_Int )
        *diags << DgString3(pSET->optionalString[3]);
      if ( pSET->optionalString[4] &&
           pSET->optionalString[4] != (char *)ComDiags_UnInitialized_Int )
        *diags << DgString4(pSET->optionalString[4]);
      if ( pSET->optionalInteger[0] != ComDiags_UnInitialized_Int )
        *diags << DgInt0(pSET->optionalInteger[0]);
      if ( pSET->optionalInteger[1] != ComDiags_UnInitialized_Int )
        *diags << DgInt1(pSET->optionalInteger[1]);
      if ( pSET->optionalInteger[2] != ComDiags_UnInitialized_Int )
        *diags << DgInt2(pSET->optionalInteger[2]);
      if ( pSET->optionalInteger[3] != ComDiags_UnInitialized_Int )
        *diags << DgInt3(pSET->optionalInteger[3]);
      if ( pSET->optionalInteger[4] != ComDiags_UnInitialized_Int )
        *diags << DgInt4(pSET->optionalInteger[4]);
      }
    }
    else if ( i==0 )
    {
      // this is the case that ISP implementation does not return any
      // SP_ERROR_STRUCT info back, but does return with error status.
      *diags << DgSqlCode(arkcmpErrorISPNoSPError);
      break;
    }
    else
      // no more SP_ERROR_STRUCT.
      break;
  }
}

// -----------------------------------------------------------------------
// helper routines used by both CmpSPInputFormat and CmpSPOutputFormat
// -----------------------------------------------------------------------

// This routine converts a field description in SP_FIELDDESC_STRUCT 
// into an ElemDDLColDef. Error will be put into context
static ElemDDLColDef* SPFieldDesc2ElemDDLColDef(SP_FIELDDESC_STRUCT* fd,
                                                CmpContext* context)
{
  ElemDDLColDef* elemDDL=0;

  if (!fd)
    return 0;
      
  Parser parser(context);
  if ( !(elemDDL = parser.parseColumnDefinition(fd->COLUMN_DEF, strlen(fd->COLUMN_DEF),
                                                CharInfo::UTF8
                                                )))
    { 
      return 0; // the error is put into context_ diags by parser already
    }
  return elemDDL;  
}

// -----------------------------------------------------------------------
// Methods of CmpSPInputFormat
// -----------------------------------------------------------------------

CmpSPInputFormat::CmpSPInputFormat(CmpContext* context)
{
  context_ = context;
  inputType_ = 0;  
}

CmpSPInputFormat::~CmpSPInputFormat()
{
}

NABoolean CmpSPInputFormat::SetFormat(Lng32 ncols,
				      SP_FIELDDESC_STRUCT* fields)
{
  SP_FIELDDESC_STRUCT* fd = fields;
  ElemDDLColDef* elem=0;
  
  for (Int32 i=0; i < ncols; i++, fd++)
    {
      if ( !( elem = SPFieldDesc2ElemDDLColDef(fd, context_) ) )
	return FALSE;
      ItemExpr*  item = new((CollHeap*)context_->statementHeap())
	NATypeToItem((NAType*)elem->getColumnDataType());
      if (inputType_)
	inputType_ = new((CollHeap*)context_->statementHeap())
	  ItemList(inputType_, item);
      else
	inputType_ = item;      
    }
  return TRUE;  
}

// -----------------------------------------------------------------------
// Methods of CmpSPOutputFormat
// -----------------------------------------------------------------------

CmpSPOutputFormat::CmpSPOutputFormat(CmpContext* context)
{
  context_ = context;
}


NABoolean CmpSPOutputFormat::SetFormat(Lng32 nCols, 
                                       const char* tableName,
                                       SP_FIELDDESC_STRUCT* fields,
                                       Lng32 nKeys,
                                       SP_KEYDESC_STRUCT *keys)
{
  // To convert the SP_FIELDDESC_STRUCT into columns_desc,
  // 1. call the parser to parse the column definition.
  // 2. The ElemDDLColDef is used to generate columns_desc.
  // 3. Refer to catman/CatExecCreateTable.C/CatBuildColumnList 
  //    to convert ElemDDLColDefArray to CatColumnList.
  // 4. Refer to sqlcat/readRealArk.C/convertColumns to convert
  //    CatColumnList to columns_desc.
  // Note :  03/18/97. 
  // It is not a good practice to clone the conversion code. The
  // correct way to handle this is to call CatBuildColumnList and 
  // convertColumns since it will conforms to however catman processes
  // the DDL queries from StmtDDLCreateTable RelExpr. But in
  // CatBuildColumnList routine, the CatRWBaseTable is created which 
  // involves the SOL cache routines and it will check the security for
  // creating tables and a whole lot more logics deep in catman. 
  // copying the code is much easier. ( Ya, I'm copying
  // the code and blame it to someone else :-) ).
  // Since this is for the internally developed stored
  // procedures, once there are changes in the data types supported and
  // requires changes in CatBuildColumnList or convertColumns routines.
  // The following conversion code needs to be changed accordingly.
  // Currently (03/29/97), catman has routines ( internal usage only )
  // to generate the in memory virtual table structure without going
  // through SOL. It is used for index table ( i.e. with type E_INDEX )
  // Once this interface is externalized, it should be used here to 
  // go through catman code to get desc structures.

  nCols_ = nCols;
  // set up tableDesc_, allocate the storage from allocate routine,
  // so the delete of the tableDesc_ could be done correctly. 
  // As in sqlcat/desc.h TrafAllocateDDLdesc(), allocate routine gets 
  // the storage from HEAP ( i.e. CmpCommon::statementHeap() ). 
  // Likewise, copyString should be the same implementation as in 
  // readRealArk.cpp. It was externalized originally, but being put as
  // static function by someone. Currently the code copyString is cloned
  // in the beginning of the file. ( which will allocate the memory from
  // statementHeap ).

  TrafDesc* table_desc = TrafAllocateDDLdesc(DESC_TABLE_TYPE, NULL);
  tableDesc_ = table_desc;

  char* objName = copyString(NAString(tableName));
  table_desc->tableDesc()->tablename = objName;

  table_desc->tableDesc()->record_length = 0; // to be set later in generator.
  table_desc->tableDesc()->colcount = (Int32)nCols;

  TrafDesc * files_desc = TrafAllocateDDLdesc(DESC_FILES_TYPE, NULL);
  table_desc->tableDesc()->files_desc = files_desc;
  
  // populate the TrafColumnsDesc
  TrafDesc* prev_col_desc = 0;
  TrafDesc* first_col_desc =  0;
  for ( Int32 i=0; i < nCols; i++ )
    {
      TrafDesc* column_desc = TrafAllocateDDLdesc(DESC_COLUMNS_TYPE, NULL);
      if (prev_col_desc)
	 prev_col_desc->next = column_desc;
      else
	 first_col_desc = column_desc;
      prev_col_desc = column_desc;
      if ( !getColumnDesc( &(fields[i]), (column_desc->columnsDesc())))
	{
	  *(context_->diags()) << DgSqlCode(arkcmpErrorISPFieldDef);
	  tableDesc_ = 0; // since HEAP is from statement heap, it will
	  // be removed automatically at the end of statement.
	  return FALSE;
        }
      column_desc->columnsDesc()->colnumber = i;      
    }

  table_desc->tableDesc()->columns_desc = first_col_desc;

  // populate index_desc and key_desc
  TrafDesc* keys_desc = 0;
  if ( !getKeysDesc( nKeys, keys, keys_desc) )
     {
       *(context_->diags()) << DgSqlCode(arkcmpErrorISPFieldDef);
       tableDesc_ = 0;
       return FALSE;
     }

  TrafDesc * index_desc = TrafAllocateDDLdesc(DESC_INDEXES_TYPE, NULL);
  index_desc->indexesDesc()->tablename = objName;
  index_desc->indexesDesc()->indexname = objName;
  index_desc->indexesDesc()->keytag = 0; // primary index
  index_desc->indexesDesc()->record_length = 
    table_desc->tableDesc()->record_length;
  index_desc->indexesDesc()->colcount =
    table_desc->tableDesc()->colcount;
  index_desc->indexesDesc()->blocksize = 4096; // anything > 0

  TrafDesc * i_files_desc = TrafAllocateDDLdesc(DESC_FILES_TYPE, NULL);
  index_desc->indexesDesc()->files_desc = i_files_desc;

  index_desc->indexesDesc()->keys_desc  = keys_desc;
  table_desc->tableDesc()->indexes_desc = index_desc;       

  return TRUE;
}

CmpSPOutputFormat::~CmpSPOutputFormat() 
{
}

NABoolean CmpSPOutputFormat::getColumnDesc(SP_FIELDDESC_STRUCT* fDesc, 
					   TrafColumnsDesc* colsDesc )
{
  ElemDDLColDef* elemDDL;
  
  if (! (elemDDL=SPFieldDesc2ElemDDLColDef(fDesc, context_)) )
    return FALSE;
  else 
    {
      // convert ElemDDLColViewDef into CatColumn, then column_desc
      if ( !ElemDDLColDef2ColumnDescStruct
	   (elemDDL, tableDesc_->tableDesc()->tablename, colsDesc) )
	return FALSE;      
    }
  
  return TRUE;
}

NABoolean CmpSPOutputFormat::getKeysDesc(Lng32 nKeys, 
				  SP_KEYDESC_STRUCT* keys, 
				  TrafDesc* &keysDesc)
{
  // key is not supported yet in FCS ( Sep. 97 release ) needs to rework on this.
  TrafDesc* prev_key_desc = 0;
  TrafDesc* first_key_desc = 0;
  for ( Int32 i=0; i < nKeys; i++ )
  {
    TrafDesc* key_desc = TrafAllocateDDLdesc(DESC_KEYS_TYPE, NULL);
    if (prev_key_desc)
      prev_key_desc->next = key_desc;
    else
      first_key_desc = key_desc;
    prev_key_desc = key_desc;
    key_desc->keysDesc()->keyseqnumber = i;
    // TODO, find out the tablecolnumber from column name ????
    key_desc->keysDesc()->tablecolnumber = 0;
    key_desc->keysDesc()->setDescending(FALSE);
  }
  keysDesc = first_key_desc;
  return TRUE;
}

NABoolean CmpSPOutputFormat::ElemDDLColDef2ColumnDescStruct
  (ElemDDLColDef* elem,
  const char* tableName,
  TrafColumnsDesc* colDesc)
{
  // Just copy the pointer for this name --
  // no need to alloc + strcpy a la copyString
  //  colDesc->tablename = (char *)tableName;

  colDesc->colname = copyString(elem->getColumnName());

  // colDesc->colnumber, filled outside
  
  NAType* genericType = elem->getColumnDataType();  
  colDesc->datatype = genericType->getFSDatatype();
  colDesc->length = genericType->getNominalSize();
  colDesc->pictureText = (char *)emptyString;

  // The logic for converting from an NAType to column_desc is also
  // in the catman code -- readRealArk.cpp::convertColumn(). Any changes
  // there must be reflected here as well and vice versa.

  if ( genericType->getTypeQualifier() == NA_NUMERIC_TYPE )
    {
      NumericType & nt = *((NumericType*)genericType);
      colDesc->scale = nt.getScale();

      // if this is a float (real or double) datatype,
      // then get the precision. Otherwise, for other
      // numeric type, get the precision only if it
      // is not binary precision.
      if(nt.isExact() && nt.binaryPrecision() &&
       (nt.getFSDatatype() != REC_BPINT_UNSIGNED))
      {
        colDesc->precision = 0;
      }
      else
      {
        colDesc->precision = nt.getPrecision();
      }
    }
  else 
    {
      colDesc->scale = 0;
      colDesc->precision = 0;      
    }

  if ( genericType->getTypeQualifier() == NA_CHARACTER_TYPE )
    {
      CharType & charType = (CharType &) *genericType;
      colDesc->character_set	  = charType.getCharSet();
      colDesc->encoding_charset	  = charType.getEncodingCharSet();
      colDesc->collation_sequence = charType.getCollation();
      colDesc->setUpshifted(charType.isUpshifted());
    }

  if ( genericType->getTypeQualifier() == NA_DATETIME_TYPE ||
       genericType->getTypeQualifier() == NA_INTERVAL_TYPE )
  {
      DatetimeIntervalCommonType& dti = 
        (DatetimeIntervalCommonType& )*genericType;
      colDesc->datetimestart = dti.getStartField();
      colDesc->datetimeend = dti.getEndField();
      colDesc->datetimefractprec = dti.getFractionPrecision();
      colDesc->intervalleadingprec = dti.getLeadingPrecision();
  }
  
  // offset, to be done (do we need it?)

  colDesc->setNullable(NOT elem->isNotNullConstraintSpecified());
  
  colDesc->colclass = 'U';
  colDesc->setAdded(FALSE);
  
  // defaultclass, to be done? (not referenced, not needed)

  ConstValue *pDefVal = (ConstValue*)elem->getDefaultValueExpr();
  if (!pDefVal || pDefVal->isNull())
    colDesc->defaultvalue = NULL;
  else
    colDesc->defaultvalue = copyString(pDefVal->getConstStr());
  colDesc->colFlags = 0;
  
  return TRUE;
}

// -----------------------------------------------------------------------
// Methods of CmpSPExecDataItem
// -----------------------------------------------------------------------

CmpSPExecDataItem::CmpSPExecDataItem(ULng32 exprSize,
                                     void* expr,
                                     ULng32 dataSize,
                                     void* data,
                                     CmpContext* context)
{
  exprSize_ = exprSize;
  expr_ = expr;
  dataSize_ = dataSize;
  data_ = data;
  context_ = context;
  SPFuncsDiags_ = ComDiagsArea::allocate(context->heap());
}

CmpSPExecDataItem::~CmpSPExecDataItem()
{
  if (SPFuncsDiags_)
    SPFuncsDiags_->decrRefCount();
}

CollHeap* CmpSPExecDataItem::wHeap()
{
  return context_->heap();
}

// -----------------------------------------------------------------------
// Methods of CmpISPDataItemInput
// -----------------------------------------------------------------------

CmpSPExecDataItemInput::CmpSPExecDataItemInput(ULng32 exprSize,
                                               void* expr,
                                               ULng32 dataSize,
                                               void* data,
                                               CmpContext* context)
                                               : CmpSPExecDataItem(exprSize, expr, 
                                               dataSize, data, context)
{
  CMPASSERT(ExSPPrepareInputBuffer(data_) == 0);
  CMPASSERT(ExSPPosition(data_) == 0);
  currentRow_ = 0;
  rowLength_  = 0;
  control_ = 0; 
}

CmpSPExecDataItemInput::~CmpSPExecDataItemInput()
{
}

short CmpSPExecDataItemInput::next() 
{
  short status = 0;
  status = ExSPGetInputRow(data_, control_, currentRow_, rowLength_);
  CMPASSERT(status != -1);
  return status;
}

// -----------------------------------------------------------------------
// Methods of CmpISPDataItemReply
// -----------------------------------------------------------------------

CmpSPExecDataItemReply::CmpSPExecDataItemReply(ULng32 exprSize,
                                               void* expr,
                                               ULng32 dataRowSize,
                                               ULng32 dataTotalSize,
                                               CmpSPExecDataItemInput* inputData,
                                               CollHeap* outHeap,
                                               CmpContext* context)
: CmpSPExecDataItem(exprSize, expr, dataTotalSize, 0, context)
{

  // allocate the reply buffer
  outHeap_ = outHeap;
  // data_ has to come from outHeap, because it will be taken away to be sent back to
  // executor.
  data_ = new(outHeap) char[dataTotalSize]; 
  rowLength_ = dataRowSize; // passed in from executor
  rowBuffer_ = new(context->statementHeap()) char[rowLength_];
  memset(rowBuffer_, 0, rowLength_);
  CMPASSERT(ExSPInitReplyBuffer(data_, dataTotalSize) == 0);
  inputData_ = inputData;
  rowExist_ = FALSE;
  diagsExist_ = FALSE;
  EORExist_ = FALSE;
}

CmpSPExecDataItemReply::~CmpSPExecDataItemReply()
{
  NADELETEBASIC((char *)data_, outHeap_);
  NADELETEBASIC((char *)rowBuffer_, context_->statementHeap());
  diags_.clear(); // this is to avoid memory leak.
}

NABoolean CmpSPExecDataItemReply::prepare()
{
  return ( ExSPPrepareReplyBuffer(data_) == 0 );
}

NABoolean CmpSPExecDataItemReply::MoveDiags(const SP_ERROR_STRUCT* error, const SP_STATUS spStatus)
{
  // Copy the error information from SP_ERROR_STRUCT into diags_
  diagsExist_ = TRUE;
  EORExist_ = TRUE;
  diags_.clear();
  CmpSPERROR2Diags(error, &diags_);
  return TRUE;
}

NABoolean CmpSPExecDataItemReply::MergeDiags(ComDiagsArea* pDiags)
{
  if (pDiags == NULL ||
      pDiags->getNumber() <= 0)
    return FALSE;
  // merge the warning and error information from *pDiags into diags_
  diagsExist_ = TRUE;
  EORExist_ = TRUE;
  diags_.mergeAfter(*pDiags);
  return TRUE;
}

short CmpSPExecDataItemReply::AddARow()
{
  // set the rowExist_ flag to be TRUE
  // call the executor provided routines to move the data into buffer to be sent
  // back to executor. 
  // if moved in successfully, reset the rowExist_ flag.
  // otherwise, return with buffer full and wait for the next GetNext request from
  // executor with enough buffer size to fill the row.
  rowExist_ = TRUE;
  short status = 0;
  status = ExSPPutReplyRow(data(), 
    inputData_->control(),
    (char*)rowBuffer_,
    rowLength_, 0);
  
  if ( status == 1 ) // buffer is full
    return 1;
  memset(rowBuffer_, 0, rowLength_);
  rowExist_ = FALSE;
  return status;
}


short CmpSPExecDataItemReply::AddEOR()
{
  // if diagsExist_, sent back the diags info along with end of row indicator 
  // if moved in successfully, reset the diagsExist_ and EORExist_ flag.
  // if buffer is full, just return 1, wait for executor to send enough buffer.
  short status= 0;
  EORExist_ = TRUE;
  if (diagsExist_)
  {
    status = ExSPPutReplyRow(data(), inputData_->control(), 0, 0, 
      &diags_);
    if ( status == 1) // buffer is full
      return 1;
    diagsExist_ = FALSE;
    diags_.clear();
  }
  if (status != 2) // In case of warning we have already returned an EOD
   status = ExSPPutReplyRow(data(), inputData_->control(), 0, 0, 0);
  if ( status == 1 ) // buffer is full
    return 1;
  EORExist_ = FALSE;
  return status;
}

void CmpSPExecDataItemReply::allocateData(ULng32 size)
{
  NADELETEBASIC((char *)data_, outHeap_);
  data_ = new(outHeap_) char[size];
  CMPASSERT(ExSPInitReplyBuffer(data_, size) == 0);
}

// -----------------------------------------------------------------------
// Methods of CmpISPDataObject
// -----------------------------------------------------------------------

CmpISPDataObject::CmpISPDataObject(CmpMessageISPRequest* req,
                                   CmpInternalSP* isp,
                                   CollHeap* outheap,
                                   CmpContext* context)
: input_(req->inputExprSize(), (void*)req->inputExpr(), 
         req->inputDataSize(), (void*)req->inputData(), context),
  key_(req->keyExprSize(), (void*)req->keyExpr(),
       req->keyDataSize(), (void*)req->keyData(), context),
  output_(req->outputExprSize(), (void*)req->outputExpr(), 
          req->outputRowSize(), req->outputTotalSize(),&input_,outheap,context),
  outHeap_(outheap),
  context_(context)
{
  isp->SetCmpISPDataObject(this);
  ispRequest_ = req;
  CMPASSERT( ExSPUnpackIOExpr(input_.expr(),
                              output_.expr(),
                              context->statementHeap()) == 0 );
}

CmpISPDataObject::~CmpISPDataObject()
{
  if (ispRequest_)
    ispRequest_->decrRefCount();
}

// -----------------------------------------------------------------------
// Methods of CmpStoredProc
// -----------------------------------------------------------------------

CmpStoredProc::CmpStoredProc(const NAString& procName, CmpContext* cmpContext) 
  : procName_(procName)
{
  cmpContext_ = cmpContext;
}

CmpStoredProc::~CmpStoredProc()
{
}

NABoolean CmpStoredProc::InputFormat(Lng32, CmpSPInputFormat&)
{
  return FALSE;
}

NABoolean CmpStoredProc::OutputFormat(CmpSPOutputFormat&)
{
  return FALSE;
}

CmpStoredProc::ExecStatus CmpStoredProc::open(CmpSPDataObject&)
{
  return FAIL;
}

CmpStoredProc::ExecStatus CmpStoredProc::fetch(CmpSPDataObject&)
{
  return FAIL;
}

CmpStoredProc::ExecStatus CmpStoredProc::close(CmpSPDataObject&)
{
  return FAIL;
}

// -----------------------------------------------------------------------
// Methods of CmpISPFuncs
// -----------------------------------------------------------------------

NAList<CmpISPFuncs::ProcFuncsStruct> CmpISPFuncs::procFuncsArray_(NULL,256);

CmpISPFuncs::CmpISPFuncs()
{
}

CmpISPFuncs::~CmpISPFuncs()
{
}

NABoolean CmpISPFuncs::ValidPFuncs
(const ProcFuncsStruct& pFuncs) const
{
  return ( pFuncs.procName_ && 
	   pFuncs.compileFunc_ &&
	   pFuncs.inFormatFunc_ &&
	   pFuncs.outNumFormatFunc_ &&
	   pFuncs.outFormatFunc_ &&
	   pFuncs.procFunc_ );	   
}

Int32 CmpISPFuncs::RegFuncs(
   const char* procName, //null terminated 
   SP_COMPILE_FUNCPTR compileFunc, 
   SP_INPUTFORMAT_FUNCPTR inFormatFunc,
   SP_PARSE_FUNCPTR parseFunc,
   SP_NUM_OUTPUTFIELDS_FUNCPTR outNumFormatFunc,
   SP_OUTPUTFORMAT_FUNCPTR outFormatFunc,
   SP_PROCESS_FUNCPTR procFunc,
   SP_HANDLE spHandle,
   const char* version)
{
  if ( strcmp(version, CMPISPVERSION) != 0 )
  {
    ABORT ( "arkcmp: The ISP interface version is not compatible" );
    NAExit(1);
  }
  procFuncsArray_.insert(ProcFuncsStruct(procName,
    compileFunc,
    inFormatFunc,
    parseFunc,
    outNumFormatFunc,
    outFormatFunc,
    procFunc,
    spHandle));
  return 1;
}

// -----------------------------------------------------------------------
// Methods of CmpInternalSP
// -----------------------------------------------------------------------

CmpInternalSP::CmpInternalSP(const NAString& name, CmpContext* context)
: CmpStoredProc(name, context),
  compHandle_(0),
  procHandle_(0),
  state_(NONE),
  ispData_(0)
{
  for ( Int32 j=0; j < SP_MAX_ERROR_STRUCTS; j++ )
  {
    for ( Int32 i=0; i < SP_ERROR_MAX_OPTIONAL_STRINGS; i++ )
      spError_[j].optionalString[i] = new((CollHeap*)cmpContext()->statementHeap())
      char[SP_STRING_MAX_LENGTH];
  }
 initSP_ERROR_STRUCT();
}

CmpInternalSP::~CmpInternalSP()
{
  if ( state_ == COMPILE )
    {
      // needs to call the compileFunc_
      initSP_ERROR_STRUCT();
      if ( (*(procFuncs_.compileFunc_)) (SP_COMP_EXIT, &compHandle_,
        procFuncs_.spHandle_, &spError_[0]) !=SP_SUCCESS)
        appendDiags();
    }

  // if not being properly closed, could be an exception happened,
  // close the process properly 
  if ( state_ == PROCESS )
  {
    close(*ispData_);
    // When it gets here, there must be an assertion happened before. 
    // To pass back the error infomation from SP close interface routine,
    // the CoMDiags is moved into the context_ diags. 
    cmpContext()->diags()->mergeAfter(ispData_->output()->diags());
  }

  delete ispData_;

  for ( Int32 j=0; j < SP_MAX_ERROR_STRUCTS; j++ )
  {
    for (Int32 i=0; i < SP_ERROR_MAX_OPTIONAL_STRINGS; i++)
      NADELETEBASIC(spError_[j].optionalString[i], cmpContext()->statementHeap());  
  }
}

// helper routines used internally


static NAString timeStamp()
{
  time_t tp;
  tp = time(NULL);
  char str[256];
  sprintf(str, "%u", (UInt32)tp);  // take lower 4 bytes as unique value
  return NAString(str);
}

NAString CmpInternalSP::OutTableName()
{
  // To make the OutTableName unique, put in the timestamp
  // Note the timeStamp() above returns just lower 4 bytes of seconds since
  // 01/01/1970 00:00:00, so it may not be unique if we get here to soon.
  NAString tableName = "SPTableOut" + procName() + timeStamp();
  return tableName;
}

NABoolean CmpInternalSP::startCompileCycle()
{
  procFuncs_ = procFuncsLookupTable_[procName()];
  if (!procFuncsLookupTable_.ValidPFuncs(procFuncs_))
    {
       *(cmpContext()->diags()) << DgSqlCode(arkcmpErrorISPNotFound) 
	 << DgString0(procName().data());
       return FALSE;       
    }
  if ( state_ != COMPILE )
    {
      // needs to call the compileFunc_
      initSP_ERROR_STRUCT();
      if ( (*(procFuncs_.compileFunc_)) (SP_COMP_INIT, &compHandle_,
        procFuncs_.spHandle_, &spError_[0])!= SP_SUCCESS )
	{
	  appendDiags();
	  return FALSE;	  
	}      
    }
        
  state_ = COMPILE;  
  return TRUE;  
}


SP_FIELDDESC_STRUCT* CmpInternalSP::allocSP_FIELDDESC_STRUCT(Lng32 num)
{
  // The SP_FIELDDESC_STRUCT is allocated using new, not the overloaded one
  // because it is an array of struct to the SP interface function. So it can 
  // not be derived from NABasicObject. Since the caller is not supposed to 
  // jump out of the routine before it returns, it will not cause memory leak.

  SP_FIELDDESC_STRUCT* fd = 0;
  if (num) 
    {
      fd = new SP_FIELDDESC_STRUCT[num];
      memset(fd, 0, sizeof(SP_FIELDDESC_STRUCT) * (Int32)num);
    }
  return fd;  
}

void CmpInternalSP::deleteSP_FIELDDESC_STRUCT(SP_FIELDDESC_STRUCT* fd)
{
  delete[] fd;  
}

SP_KEYDESC_STRUCT* CmpInternalSP::allocSP_KEYDESC_STRUCT(Lng32 num)
{
  // The SP_KEYDESC_STRUCT is allocated using new, not the overloaded one
  // because it is an array of struct to the SP interface function. So it can 
  // not be derived from NABasicObject. Since the caller is not supposed to 
  // jump out of the routine before it returns, it will not cause memory leak.

  SP_KEYDESC_STRUCT* kd = 0;
  if (num)
    {      
      kd = new SP_KEYDESC_STRUCT[num];
      memset(kd, 0, sizeof(SP_KEYDESC_STRUCT) * (Int32)num);
    }
  return kd;  
}

void CmpInternalSP::deleteSP_KEYDESC_STRUCT(SP_KEYDESC_STRUCT* kd)
{
  delete[] kd;  
}

void CmpInternalSP::initSP_ERROR_STRUCT()
{
  for ( Int32 j=0; j<SP_MAX_ERROR_STRUCTS; j++ )
  {
    spError_[j].error = 0;
    for ( Int32 i=0; i<SP_ERROR_MAX_OPTIONAL_STRINGS; i++)
      *(spError_[j].optionalString[i]) = '\0';
    for ( Int32 k=0; k<SP_ERROR_MAX_OPTIONAL_INTS; k++ )
      spError_[j].optionalInteger[k] = ComDiags_UnInitialized_Int;
  }
}

void CmpInternalSP::appendDiags()
{
  CmpSPERROR2Diags(spError_, cmpContext()->diags());
}

// virtual methods for interface between arkcmp ( compiler ) and
// stored procedure implementation.

NABoolean CmpInternalSP::InputFormat(Lng32 num, CmpSPInputFormat& t) 
{
  NABoolean retStatus = TRUE;  
  
  if ( !startCompileCycle() )
    return FALSE;

  SP_FIELDDESC_STRUCT* fields = allocSP_FIELDDESC_STRUCT(num);
  initSP_ERROR_STRUCT();
  if ( (*(procFuncs_.inFormatFunc_)) (fields, num, 
    compHandle_, procFuncs_.spHandle_, &spError_[0])
    == SP_SUCCESS )
    {
      if (!(t.SetFormat(num, fields) ) )   
	  retStatus = FALSE;	  
    }
  else
    {
      retStatus = FALSE;      
      appendDiags();
    }
  if ( !retStatus )
    *(cmpContext()->diags()) << DgSqlCode(arkcmpErrorISPFieldDef);

  // It is not supposed to jump out of this function before this delete.
  // This has been agreed on the interface between arkcmp/server and ISP.
  deleteSP_FIELDDESC_STRUCT(fields);
  return retStatus;  
}

NABoolean CmpInternalSP::OutputFormat(CmpSPOutputFormat& t)
{
  NABoolean retStatus = TRUE;
  
  CMPASSERT(state_ == COMPILE);
  
  initSP_ERROR_STRUCT();
  Lng32 num;
  NAString outTableName = OutTableName();
  if ( (*(procFuncs_.outNumFormatFunc_))( &num, compHandle_, 
    procFuncs_.spHandle_, &spError_[0])
    == SP_SUCCESS )
    {	 
      if ( num == 0 )
      {
        // Create a dummy column if there is no output, since otherwise
        // NATable generated later on will break later in binder.
        SP_FIELDDESC_STRUCT fields;
        strcpy(fields.COLUMN_DEF, "Dummy1 int");
        if ( !(t.SetFormat(1, outTableName.data(), &fields, 0, 0) ) )
          retStatus = FALSE;
      }
      else
      {
        SP_FIELDDESC_STRUCT* fields = allocSP_FIELDDESC_STRUCT(num);
        SP_KEYDESC_STRUCT* keys = allocSP_KEYDESC_STRUCT(num);
        Lng32 nKeys = 0;
        initSP_ERROR_STRUCT();
        if ( (*(procFuncs_.outFormatFunc_)) 
          (fields, keys, &nKeys, compHandle_, procFuncs_.spHandle_, &spError_[0])
          == SP_SUCCESS )
        {
          if (!(t.SetFormat(num, outTableName.data(), fields, nKeys, keys ) ) ) 
          {
            retStatus = FALSE;
          }	  
        }
        else
        {	  
          appendDiags();
          retStatus = FALSE;	  
        }
      
        // It is not supposed to jump out of this function before this  
        // delete[]
        deleteSP_FIELDDESC_STRUCT(fields);      
        deleteSP_KEYDESC_STRUCT(keys);
      }
    }
  else
    {
      appendDiags();
      retStatus = FALSE;
    }  
  if ( !retStatus )
    *(cmpContext()->diags()) << DgSqlCode(arkcmpErrorISPFieldDef);
  return retStatus;
}

NABoolean CmpInternalSP::ParseInput(const NAString& param)
{
  CMPASSERT(state_ == COMPILE);
  if ( !(procFuncs_.parseFunc_) )
    {
      // an optional function to call
      return TRUE;
    }  

  // test code, for CMPASSERT, check SPUtil.cpp SP_ERROR_* 
  // for more details.
  // CMPASSERT( strcmp((char*)param.data(), "TestCMPASSERT") != 0 );

  initSP_ERROR_STRUCT();
  if ( (*(procFuncs_.parseFunc_))((char*)param.data(), compHandle_, 
    procFuncs_.spHandle_, &spError_[0]) != SP_SUCCESS ) 
    {      
      appendDiags();
      return FALSE;
    }    
  return TRUE;  
}

//CmpStoredProc::Status CmpInternalSP::open(CmpISPDataObject& data)
CmpStoredProc::ExecStatus CmpInternalSP::open(CmpISPDataObject& data)
{
  CMPASSERT(state_ == NONE);
  procFuncs_ = procFuncsLookupTable_[procName()];
  if (!procFuncsLookupTable_.ValidPFuncs(procFuncs_))
  {
    *(cmpContext()->diags()) << DgSqlCode(arkcmpErrorISPNotFound) 
      << DgString0(procName().data());
    return FAIL;       
  }

  initSP_ERROR_STRUCT();
  SP_STATUS spStatus =
      (*(procFuncs_.procFunc_))( SP_PROC_OPEN, 
				(SP_ROW_DATA)data.input(),
				CmpSPExtractFunc,
				(SP_ROW_DATA)data.output(), 
				CmpSPFormatFunc,
				(SP_KEY_VALUE)data.key(), 
				CmpSPKeyValueFunc,
				&procHandle_, 
				procFuncs_.spHandle_, 
				&spError_[0]);

  if (spStatus == SP_FAIL || spStatus == SP_SUCCESS_WARNING)
  {
    // Errors or warnings, go get them
    data.output()->MoveDiags(spError_, spStatus);
    if (spStatus == SP_FAIL)
      return FAIL;
  }
  state_ = PROCESS;
  return SUCCESS; 
}

CmpStoredProc::ExecStatus CmpInternalSP::fetch(CmpISPDataObject& data)
{
  CMPASSERT(state_ == PROCESS);

  // Need to send controls explicitly from the compiler space to the
  // cli context so ISP requests can perform CLI operations to extract
  // their values.
  // For now, only send controls for MODIFY, POPULATE INDEX, TRANSFORM,
  // PURGEDATA, MV_refresh and VALIDATE sp requests.
  // This also means RECOVER since these operations
  // can be perfomed through a RECOVER operation.
  //
  // VO, Versioning Light: Also send controls for sp_SchLevel
  //                       (UPGRADE and DOWNGRADE)
  if (procName() == "sp_partn" || 
      procName() == "sp_populate" ||
      procName() == "sp_purgedata" ||
      procName() == "sp_recover" ||
      procName() == "sp_transform" ||
      procName() == "sp_validate" || 
      procName() == "sp_purgedata" ||
      procName() == "sp_refresh" ||
      procName() == "sp_SchLevel")  
    {
      sendAllControls(FALSE, FALSE, TRUE);
      
      // set the parent qid for this session
      const char *parentQid = cmpContext()->sqlSession()->getParentQid();
      if (parentQid != NULL)
	{
	  setParentQidAtSession(cmpContext()->statementHeap(), parentQid);
	}
    }

  // Send sqlparser_flags
  if (Get_SqlParser_Flags(ALLOW_FUNNY_IDENTIFIER))
    sendParserFlag(ALLOW_FUNNY_IDENTIFIER);

  initSP_ERROR_STRUCT();
  SP_STATUS spStatus =
      (*(procFuncs_.procFunc_))( SP_PROC_FETCH, 
                                (SP_ROW_DATA)data.input(),
                                CmpSPExtractFunc,
                                (SP_ROW_DATA)data.output(), 
                                CmpSPFormatFunc,
                                (SP_KEY_VALUE)data.key(), 
                                CmpSPKeyValueFunc,
                                &procHandle_, 
                                procFuncs_.spHandle_, 
                                &spError_[0]);

  if (spStatus == SP_FAIL)
  {
    // Errors, go get them
    data.output()->MoveDiags(spError_, spStatus);
    if (CmpCommon::diags() != NULL)
    {
      data.output()->MergeDiags(CmpCommon::diags());
      CmpCommon::diags()->clear();
    }
    return FAIL;
  }

  if (CmpCommon::diags() != NULL)
  {
    data.output()->MergeDiags(CmpCommon::diags());
    CmpCommon::diags()->clear();
  }

  if ( spStatus == SP_MOREDATA)
  {
    return MOREDATA;
  }
  return SUCCESS;
}

CmpStoredProc::ExecStatus CmpInternalSP::close(CmpISPDataObject& data)
{
  CMPASSERT(state_ == PROCESS);

  initSP_ERROR_STRUCT();
  state_ = NONE;
  SP_STATUS spStatus =
    (*(procFuncs_.procFunc_))( SP_PROC_CLOSE, 
                              (SP_ROW_DATA)data.input(),
                              CmpSPExtractFunc,
                              (SP_ROW_DATA)data.output(), 
                              CmpSPFormatFunc,
                              (SP_KEY_VALUE)data.key(), 
                              CmpSPKeyValueFunc,
                              &procHandle_, 
                              procFuncs_.spHandle_, 
                              &spError_[0]);
  if (spStatus != SP_SUCCESS )
  {
    data.output()->MoveDiags(spError_, spStatus);
    return FAIL;
  }
  return SUCCESS;
} 
