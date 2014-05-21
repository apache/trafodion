/**********************************************************************
//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2009-2014 Hewlett-Packard Development Company, L.P.
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
**************************************************************************
*
* File:         UdfDllInteraction.cpp
* Description:  Methods for a udf RelExpr to interact with a dll
* Created:      3/01/10
* Language:     C++
*
*************************************************************************
*/

#include "UdfDllInteraction.h"
#include "NumericType.h"
#include "CharType.h"
#include "DatetimeType.h"
#include "ItemOther.h"
#include "LmError.h"


// ValidateScalarInput function declaration
typedef Int32 (*valScalarInp_func) (  
  SQLUDR_CHAR         **in_data,
  SQLUDR_INT16        *null_in_data,
  SQLUDR_TMUDFINFO    *tmudfInfo,                                     
  SQLUDR_PARAM        *input_values,                                  
  SQLUDR_CHAR          sqlstate[6],                                     
  SQLUDR_CHAR          msgtext[256],                                      
  SQLUDR_STATEAREA    *statearea) ;

SQLUDR_INT32 SQLUDR_INVOKE_VALSCALARINP(valScalarInp_func   func_ptr,
                                        SQLUDR_CHAR         **in_data,  
                                        SQLUDR_INT16        *null_in_data,
                                        SQLUDR_TMUDFINFO    *tmudfInfo,                                     
                                        SQLUDR_PARAM        *input_values,                                  
                                        SQLUDR_CHAR          sqlstate[6],                                     
                                        SQLUDR_CHAR          msgtext[256],                                      
                                        SQLUDR_STATEAREA    *statearea)
{
  return func_ptr(in_data, null_in_data, tmudfInfo, input_values, 
                  sqlstate, msgtext, statearea);
}

// DescribeMaxOutputs function declaration
typedef Int32 (*descMaxOutputs_func) (
  SQLUDR_CHAR         **in_data,
  SQLUDR_INT16        *null_in_data,
  SQLUDR_TMUDFINFO    *tmudfInfo,                                     
  SQLUDR_UINT32       *num_return_values,  
  SQLUDR_PARAM        *return_values,                                         
  SQLUDR_CHAR         sqlstate[6],                                     
  SQLUDR_CHAR         msgtext[256],                                      
  SQLUDR_STATEAREA    *statearea) ;

SQLUDR_INT32 SQLUDR_INVOKE_DESCMAXOUTPUTS(descMaxOutputs_func   func_ptr,
					  SQLUDR_CHAR           **in_data,  
                                          SQLUDR_INT16          *null_in_data,
                                          SQLUDR_TMUDFINFO      *tmudfInfo,                                     
                                          SQLUDR_UINT32         *num_return_values,  
					  SQLUDR_PARAM          *return_values,                                  
                                          SQLUDR_CHAR           sqlstate[6],                                     
                                          SQLUDR_CHAR           msgtext[256],                                      
                                          SQLUDR_STATEAREA      *statearea)
{
  return func_ptr( in_data, null_in_data, tmudfInfo, num_return_values,
		   return_values, sqlstate, msgtext, statearea);
}

// -----------------------------------------------------------------------
// methods for class TMUDFDllInteraction
// -----------------------------------------------------------------------
//THREAD_P SQLUDR_CHAR * TMUDFDllInteraction::host_data_ = NULL ;

TMUDFDllInteraction::TMUDFDllInteraction(TableMappingUDF * tmudfNode)
:   dllPtr_(NULL),
    validateScalarInputsPtr_(NULL),
    describeMaxOutputsPtr_(NULL),
    describeInputsAndOutputsPtr_(NULL),
    describeInputPartitionAndOrderPtr_(NULL),
    predicatePushDownPtr_(NULL),
    cardinalityPtr_(NULL),
    constraintsPtr_(NULL),
    costPtr_(NULL),
    degreeOfParallelismPtr_(NULL),
    generateInputPartitionAndOrderPtr_(NULL),
    describeOutputOrderPtr_(NULL),
    stateArea_(NULL),
    tmudfInfo_(NULL),
    inData_(NULL),
    nullInData_(NULL)
    {

/*
      // Allocate (static) host_data_ if it's not already allocated.
      // This will never be deleted and stays until the process dies.
      if (TMUDFDllInteraction::host_data_ == NULL)
      {
        TMUDFDllInteraction::host_data_ = (SQLUDR_CHAR *) 
          new (CmpCommon::contextHeap()) 
          char[SQLUDR_STATEAREA_BUFFER_SIZE + 128];
        CMPASSERT(TMUDFDllInteraction::host_data_);
        memset((char *)TMUDFDllInteraction::host_data_, 0,
              SQLUDR_STATEAREA_BUFFER_SIZE + 128);
      }
*/


      // Allocate the SQLUDR_STATEAREA structure
      stateArea_ = (SQLUDR_STATEAREA *) new (CmpCommon::statementHeap()) 
                   char[sizeof(SQLUDR_STATEAREA) + 32];
      CMPASSERT(stateArea_);
      memset(stateArea_, 0, sizeof(SQLUDR_STATEAREA) + 32);
      stateArea_->version = SQLUDR_STATEAREA_CURRENT_VERSION;
  
      stateArea_->host_data.data = 
            CmpCommon::context()->getTMFUDF_DLL_InterfaceHostDataBuffer();

      stateArea_->host_data.length = 
            CmpCommon::context()->getTMFUDF_DLL_InterfaceHostDataBufferLen();

      stateArea_->stmt_data.data = (SQLUDR_CHAR *)
                                    new (CmpCommon::statementHeap()) 
                                    char[SQLUDR_STATEAREA_BUFFER_SIZE + 128];
      CMPASSERT(stateArea_->stmt_data.data);
      memset((char *)stateArea_->stmt_data.data, 0,
            SQLUDR_STATEAREA_BUFFER_SIZE + 128);
      stateArea_->stmt_data.length = SQLUDR_STATEAREA_BUFFER_SIZE;

      // Allocate SQLUDR_TMUDFINFO structure
      tmudfInfo_ = (SQLUDR_TMUDFINFO *) new (CmpCommon::statementHeap()) 
                   char[sizeof(SQLUDR_TMUDFINFO) + 32];
      CMPASSERT(tmudfInfo_);
      memset(tmudfInfo_, 0, sizeof(SQLUDR_TMUDFINFO) + 32);

      tmudfInfo_->version = SQLUDR_TMUDFINFO_CURRENT_VERSION;
      tmudfInfo_->sql_version = (SQLUDR_UINT16) ComVersion_GetMXV();
      
      NAString name(tmudfNode->getRoutineName().getQualifiedNameAsAnsiString());
      tmudfInfo_->routine_name = new (CmpCommon::statementHeap()) 
                                  char[name.length()+1];
      strncpy(tmudfInfo_->routine_name, name.data(), name.length()+1); 

      // pass through inputs currently not made available to dll
      // during compiler interaction.
      tmudfInfo_->num_pass_through = 0;
      tmudfInfo_->pass_through = NULL;

      // scalar inputs
      tmudfInfo_->num_inputs = 0;
      tmudfInfo_->inputs = NULL;

      // table inputs
      tmudfInfo_->num_table_inputs = (SQLUDR_UINT32) tmudfNode->getArity();
      tmudfInfo_->table_inputs = new (CmpCommon::statementHeap())
                                  SQLUDR_TABLE_PARAM[tmudfNode->getArity()];
      for (Int32 i = 0; i < tmudfNode->getArity(); i++)
      {
        SQLUDR_TABLE_PARAM *childInfo = &(tmudfInfo_->table_inputs[i]);
        TableMappingUDFChildInfo * cInfo = tmudfNode->getChildInfo(i);
        childInfo->table_name =  new (CmpCommon::statementHeap()) 
              char[cInfo->getInputTabName().length()+1];
        strncpy(childInfo->table_name, 
                cInfo->getInputTabName().data(), 
                cInfo->getInputTabName().length()+1); 

        childInfo->num_params = cInfo->getOutputs().entries();
        childInfo->params = new (CmpCommon::statementHeap())
                     SQLUDR_PARAM[childInfo->num_params];
        CMPASSERT(childInfo->params);
        memset(childInfo->params, 0, 
          childInfo->num_params * sizeof(SQLUDR_PARAM));
        for (Int32 j = 0; j < (Int32) childInfo->num_params; j++)
        {
          SQLUDR_PARAM * inpParam = &childInfo->params[j];
          const NAType *p = cInfo->getInputTabCols()[j]->getType();
          setParamInfo(inpParam, p, cInfo->getInputTabCols()[j]->getColName());
        }
      }

      // output_info
      NAString correlationName = 
        tmudfNode->getUserTableName().getExposedNameAsAnsiString();
      tmudfInfo_->output_info.table_name = new (CmpCommon::statementHeap()) 
                                            char[correlationName.length()+1];
      strncpy(tmudfInfo_->output_info.table_name,
              correlationName.data(), correlationName.length()+1);
    }

void 
TMUDFDllInteraction::setParamInfo(SQLUDR_PARAM * inpParam, const NAType * p, 
                                  const NAString & name)
{
  inpParam->version = SQLUDR_PARAM_CURRENT_VERSION;
  inpParam->datatype = (SQLUDR_INT16)  
    getAnsiTypeFromFSType(p->getFSDatatype());
  
  inpParam->data_len = (SQLUDR_UINT32)p->getNominalSize();


  if (name.length() == 0)
    inpParam->name = NULL;
  else
  {
    inpParam->name = new (CmpCommon::statementHeap())char[name.length()+1];
    strncpy(inpParam->name, name.data(),name.length()+1);
  }

  // Union u1 contains:
  // * character_set
  // * datetime_code
  // * interval_code
  // * scale
  if (p->getTypeQualifier() == NA_CHARACTER_TYPE)
  {
    inpParam->u1.character_set =
      (SQLUDR_INT16) ((const CharType*)p)->getCharSet();
  }
  else if (p->getTypeQualifier() == NA_DATETIME_TYPE)
  {
    // For datetime types: ANSI type will be SQLTYPECODE_DATETIME and
    // SQLUDR_PARAM stores a code to indicate date, time, or
    // timestamp.
    inpParam->u1.datetime_code =
      (SQLUDR_INT16) ((const DatetimeType*)p)->getPrecision();
  }
  else if (p->getTypeQualifier() == NA_INTERVAL_TYPE)
  {
    // For interval types: ANSI type will be SQLTYPECODE_INTERVAL and
    // SQLUDR_PARAM stores a code to indicate start and end fields.
    inpParam->u1.interval_code =
      (SQLUDR_INT16) getIntervalCode(p->getFSDatatype());
  }
  else
  {
    inpParam->u1.scale = (SQLUDR_INT16) p->getScale();
  }

  // Union u2 contains:
  // * precision
  // * collation
  if ((p->getTypeQualifier() == NA_DATETIME_TYPE)&&
    ((((const DatetimeType*)p)->getPrecision() == REC_DTCODE_TIME) ||
    (((const DatetimeType*)p)->getPrecision() == REC_DTCODE_TIMESTAMP)))
  {
    inpParam->u2.precision = (SQLUDR_INT16) p->getScale();
  }
  else if (p->getTypeQualifier() == NA_CHARACTER_TYPE)
  {
    SQLUDR_COLLATION collation = SQLUDR_COLLATION_UNKNOWN;
    switch (((const CharType*)p)->getCollation())
    {
      case CharInfo::DefaultCollation:
        collation = SQLUDR_COLLATION_DEFAULT;
        break;
      case CharInfo::CZECH_COLLATION:
        collation = SQLUDR_COLLATION_CZECH;
        break;
      case CharInfo::CZECH_COLLATION_CI:
        collation = SQLUDR_COLLATION_CZECH_CI;
        break;
      case CharInfo::SJIS_COLLATION:
        collation = SQLUDR_COLLATION_SJIS;
        break;
    }
    inpParam->u2.collation = (SQLUDR_INT16) collation;
  }
  else
  {
    inpParam->u2.precision = (SQLUDR_INT16) p->getPrecision();
  }


  inpParam->ind_offset = p->supportsSQLnullLogical() ? 0 : -1;

}

void TMUDFDllInteraction::setScalarInputParamInfo(const ValueIdList & vids)
{
  tmudfInfo_->num_inputs = vids.entries();

  tmudfInfo_->inputs = NULL;
  if (tmudfInfo_->num_inputs > 0)
  {
    tmudfInfo_->inputs = new (CmpCommon::statementHeap())
      SQLUDR_PARAM[vids.entries()];
    CMPASSERT(tmudfInfo_->inputs);
    memset(tmudfInfo_->inputs, 0, 
           tmudfInfo_->num_inputs * sizeof(SQLUDR_PARAM));
    for (Int32 j = 0; j < (Int32) tmudfInfo_->num_inputs; j++)
    {
      SQLUDR_PARAM * inpParam = &tmudfInfo_->inputs[j];
      const NAType *p = &(vids[j].getType());
      NAString name;
      if (vids[j].getItemExpr()->getOperatorType() == ITM_RENAME_COL)
      {
        RenameCol *ie = (RenameCol *) vids[j].getItemExpr();
        name  = ie->getNewColRefName()->getCorrNameObj().getCorrNameAsString();
      }
      setParamInfo(inpParam, p, (const NAString &)name);
    }
  }
}

void TMUDFDllInteraction::setScalarInputValues(const ValueIdList & vids)
{
  Int32 numInputs = vids.entries();

  if (numInputs < 1)
    return;

  inData_ = new (CmpCommon::statementHeap()) char* [numInputs];
  nullInData_ = new (CmpCommon::statementHeap()) short [numInputs];
  NABoolean negate ;
  ConstValue * cv;
  Lng32 size;
  for (Int32 i = 0; i < numInputs; i++)
  {
    inData_[i] = NULL;
    nullInData_[i] = 0; // initialize everything to not null

    cv = vids[i].getItemExpr()->castToConstValue(negate);
    if (cv)
    {
      if (! cv->isNull())
      {
        size = vids[i].getType().getNominalSize();
        inData_[i] = new (CmpCommon::statementHeap()) char[size+1];
        switch (vids[i].getType().getFSDatatype())
        {
        case COM_SIGNED_BIN16_FSDT:
          memcpy(inData_[i], (short *) cv->getConstValue(), size);
          break;

        case COM_UNSIGNED_BIN16_FSDT:
          memcpy(inData_[i], (unsigned short *) cv->getConstValue(), size);
           break;

        case COM_SIGNED_BIN32_FSDT:
          memcpy(inData_[i], (Int32 *) cv->getConstValue(), size);
          break;

        case COM_UNSIGNED_BIN32_FSDT:
          memcpy(inData_[i], (UInt32 *) cv->getConstValue(), size);
          break;

        case COM_SIGNED_BIN64_FSDT:
          memcpy(inData_[i], (unsigned short *) cv->getConstValue(), size);
          break;

        case COM_FCHAR_FSDT:
        case COM_VCHAR_FSDT:
          memcpy(inData_[i], (char *) cv->getConstValue(), size);
          *(inData_[i]+size) = 0;
          break;
          
        default:
          CMPASSERT(0);
        }
      }
      else 
        nullInData_[i] = -1; // is a null value
    } // item is a is a constant
  }
}

SQLUDR_PARAM * TMUDFDllInteraction::copyParams(Int32 num, SQLUDR_PARAM * src)
{
  if (num < 1)
    return NULL;

  SQLUDR_PARAM * result = new (CmpCommon::statementHeap()) SQLUDR_PARAM[num];
  CMPASSERT(result);
  for(Int32 i=0; i<num; i++)
  {
    result[i] = src[i]; // copy all non ptr members
    if (src[i].name != NULL)
    {
      result[i].name = new (CmpCommon::statementHeap()) 
                        char[strlen(src[i].name)];
      CMPASSERT(result[i].name);
      strncpy(result[i].name, src[i].name, strlen(src[i].name)+1);
    }
  }
  return result;

}

// Create a NAType from the Param returned by DLL
NAType *UDRCreateNAType ( SQLUDR_PARAM * col
                          , BindWA    *bindWA
                          , CollHeap *heap )
{
		  
  NAType *newType = NULL;
  DataType datatype = (DataType) getFSTypeFromANSIType(col->datatype);
  
  switch (datatype)
  {
    
    case REC_BPINT_UNSIGNED :
      newType = new (heap) SQLBPInt ( col->u2.precision
				      , (col->ind_offset == -1) ? FALSE : TRUE
				      , FALSE
				      , heap
				    );
      break;
    
    case REC_BIN16_SIGNED:
      if (col->u2.precision > 0)
	newType = new (heap) SQLNumeric ( TRUE
					  , col->u2.precision
					  , col->u1.scale
					  , (col->ind_offset == -1) ? FALSE : TRUE
					);
      else
	newType = new (heap) SQLSmall ( TRUE
					, (col->ind_offset == -1) ? FALSE : TRUE
					, heap
				      );
      break;
    case REC_BIN16_UNSIGNED:
      if (col->u2.precision > 0)
	newType = new (heap) SQLNumeric ( FALSE
					  , col->u2.precision
					  , col->u1.scale
					  , (col->ind_offset == -1) ? FALSE : TRUE
					);
      else
	newType = new (heap) SQLSmall ( FALSE // allow neg values
					, (col->ind_offset == -1) ? FALSE : TRUE
					, heap
				      );
      break;
    case REC_BIN32_SIGNED:
		if (col->u2.precision > 0)
	newType = new (heap) SQLNumeric ( TRUE
					  , col->u2.precision
					  , col->u1.scale
					  , (col->ind_offset == -1) ? FALSE : TRUE
					);
      else
	newType = new (heap) SQLInt ( TRUE
					, (col->ind_offset == -1) ? FALSE : TRUE
					, heap
				      );
      break;
    case REC_BIN32_UNSIGNED:
      if (col->u2.precision > 0)
	newType = new (heap) SQLNumeric ( FALSE
					  , col->u2.precision
					  , col->u1.scale
					  , (col->ind_offset == -1) ? FALSE : TRUE
					);
      else
	newType = new (heap) SQLInt ( FALSE // allow neg values
					, (col->ind_offset == -1) ? FALSE : TRUE
					, heap
				      );
      break;
    case REC_BIN64_SIGNED:
		if (col->u2.precision > 0)
	newType = new (heap) SQLNumeric ( TRUE
					  , col->u2.precision
					  , col->u1.scale
					  , (col->ind_offset == -1) ? FALSE : TRUE
					);
      else
	newType = new (heap) SQLLargeInt ( TRUE
					, (col->ind_offset == -1) ? FALSE : TRUE
					, heap
				      );
      break;
    case REC_NUM_BIG_UNSIGNED:
        newType = new (heap) SQLBigNum ( col->u2.precision
				       , col->u1.scale
				       , TRUE
				       , FALSE
				       , (col->ind_offset == -1) ? FALSE : TRUE
				       , heap
				     );
      break;
    case REC_NUM_BIG_SIGNED:
        newType = new (heap) SQLBigNum ( col->u2.precision
				       , col->u1.scale
				       , TRUE
				       , TRUE
				       , (col->ind_offset == -1) ? FALSE : TRUE
				       , heap
				     );
      break;
    case REC_DECIMAL_UNSIGNED:
      newType = new (heap) SQLDecimal ( col->u2.precision // precision is length?
					, col->u1.scale
					, FALSE
					, (col->ind_offset == -1) ? FALSE : TRUE
					, heap
				      );
      break;
    case REC_DECIMAL_LSE:
      newType = new (heap) SQLDecimal ( col->u2.precision // precision is length?
					, col->u1.scale
					, TRUE
					, (col->ind_offset == -1) ? FALSE : TRUE
					, heap
				      );
      break;
    case REC_TDM_FLOAT32:
      newType = new (heap) SQLRealTdm ( (col->ind_offset == -1) ? FALSE : TRUE
					, heap
					, col->u2.precision
				      );
      break;

    case REC_TDM_FLOAT64:
      newType = new (heap) SQLDoublePrecisionTdm ( 
							(col->ind_offset == -1) ? FALSE : TRUE
						   , heap
						   , col->u2.precision
						 );
      break;

    case REC_FLOAT32:
      newType = new (heap) SQLReal ( (col->ind_offset == -1) ? FALSE : TRUE
				     , heap
				     , col->u2.precision
				   );
      break;

    case REC_FLOAT64:
      newType = new (heap) SQLDoublePrecision ( 
						(col->ind_offset == -1) ? FALSE : TRUE
						, heap
						, col->u2.precision
					      );
      break;

    case REC_BYTE_F_ASCII:
      newType = new (heap) SQLChar ( col->data_len
                                   , (col->ind_offset == -1) ? FALSE : TRUE
                                   , FALSE // not upshifted
								   , FALSE // not caseinsensitive
                                   , FALSE
								   , (CharInfo::CharSet) col->u1.character_set
                                   , (CharInfo::Collation)col->u2.collation
                                   , CharInfo::COERCIBLE
                                   , CharInfo::UnknownCharSet // encoding?
                                   );

      break;

    case REC_BYTE_F_DOUBLE:
		newType = new (heap) SQLChar ( col->data_len/SQL_DBCHAR_SIZE
                                   , (col->ind_offset == -1) ? FALSE : TRUE
                                   , FALSE // not upshifted
								   , FALSE // not caseinsensitive
                                   , FALSE
								   , (CharInfo::CharSet) col->u1.character_set
                                   , (CharInfo::Collation)col->u2.collation
                                   , CharInfo::COERCIBLE
                                   , CharInfo::UnknownCharSet // encoding?
                                   );
      break;

    case REC_BYTE_V_ASCII:
      newType = new (heap) SQLVarChar ( col->data_len
                                   , (col->ind_offset == -1) ? FALSE : TRUE
                                   , FALSE // not upshifted
								   , FALSE // not caseinsensitive
								   , (CharInfo::CharSet) col->u1.character_set
                                   , (CharInfo::Collation)col->u2.collation
                                   , CharInfo::COERCIBLE
                                   , CharInfo::UnknownCharSet // encoding?
                                   );
      break;

    case REC_BYTE_V_DOUBLE:
      newType = new (heap) SQLVarChar ( col->data_len/SQL_DBCHAR_SIZE
                                   , (col->ind_offset == -1) ? FALSE : TRUE
                                   , FALSE // not upshifted
								   , FALSE // not caseinsensitive
								   , (CharInfo::CharSet) col->u1.character_set
                                   , (CharInfo::Collation)col->u2.collation
                                   , CharInfo::COERCIBLE
                                   , CharInfo::UnknownCharSet // encoding?
                                   );
      break;

    case REC_BYTE_V_ASCII_LONG:
      newType = new (heap) SQLLongVarChar ( col->data_len
                                   , (col->ind_offset == -1) ? FALSE : TRUE
                                   , FALSE // not upshifted
								   , FALSE // not caseinsensitive
                                   , FALSE
								   , (CharInfo::CharSet) col->u1.character_set
                                   , (CharInfo::Collation)col->u2.collation
                                   , CharInfo::COERCIBLE
                                   , CharInfo::UnknownCharSet // encoding?
                                   );
      break;

    case REC_DATETIME:
	  if (col->u1.datetime_code == SQLDTCODE_DATE)
	  {
		  	newType = new (heap)  SQLDate(
				(col->ind_offset == -1) ? FALSE : TRUE, 
					heap) ;
	  }
	  else if (col->u1.datetime_code == SQLDTCODE_TIME)
	  {
		  	newType = new (heap)  SQLTime(
				(col->ind_offset == -1) ? FALSE : TRUE, 
					col->u2.precision,
					heap) ;
	  }
	  else if (col->u1.datetime_code == SQLDTCODE_TIMESTAMP)
	  {
		 newType = new (heap)  SQLTimestamp(
				(col->ind_offset == -1) ? FALSE : TRUE, 
					col->u2.precision,
					heap) ;
	  }
	  else
      {
		// 4030 Column is an unsupported combination of datetime fields
		*CmpCommon::diags() << DgSqlCode(-4030)
					<< DgTableName("") ;
		bindWA->setErrStatus();
		return NULL;
      }
      break;
     
    default:
      *CmpCommon::diags() << DgSqlCode(-4308)
			  << DgInt0(datatype);
	  bindWA->setErrStatus();
      return NULL;   
  } // end switch (column_desc->datatype)

  // catch all
  if ( NULL == newType )
  {
    if ( bindWA )
    {
      bindWA->setErrStatus ();
    }
  }
    
  return newType;
} // UDRCreateNAType


SQLUDR_PARAM * TMUDFDllInteraction::createEmptyParams(Int32 num, Int32 nameLen)
{
  SQLUDR_PARAM * result = new (CmpCommon::statementHeap()) SQLUDR_PARAM[num];
  for (Int32 i=0; i < num; i++)
  {
    result[i].version = SQLUDR_PARAM_CURRENT_VERSION;
    result[i].datatype = 0;
    result[i].data_len = 0;
    result[i].name = new (CmpCommon::statementHeap())char[nameLen];
    memset(result[i].name, '\0',nameLen);
    result[i].u1.character_set = 0;
    result[i].u2.precision = 0;
    result[i].ind_offset = 0;
  }
  return result;
}

NABoolean TMUDFDllInteraction::ValidateScalarInputs(
                                TableMappingUDF * TMUDFNode, BindWA * bindWA)
{ 
  if (validateScalarInputsPtr_ == NULL)
    return TRUE;

  // Initialize SQLSTATE to all '0' characters and add a null terminator
  str_pad(sqlState_, SQLUDR_SQLSTATE_SIZE - 1, '0');
  sqlState_[SQLUDR_SQLSTATE_SIZE - 1] = 0;
  
  // Initialize SQL text to all zero bytes
  str_pad(msgText_, SQLUDR_MSGTEXT_SIZE, '\0');

  SQLUDR_PARAM  *input_values = copyParams(tmudfInfo_->num_inputs,
                                            tmudfInfo_->inputs) ;

  // Call the function
  Int32 retValue = SQLUDR_INVOKE_VALSCALARINP(
                        (valScalarInp_func)   validateScalarInputsPtr_,
                                              inData_,  
                                              nullInData_,
                                              tmudfInfo_,                                     
                                              input_values,                                  
                                              sqlState_,                                     
                                              msgText_,                                      
                                              stateArea_);
  


  // Check the return value from routine execution
  if (retValue != SQLUDR_SUCCESS)
  {
    sqlState_[SQLUDR_SQLSTATE_SIZE - 1] = 0;
    processReturnStatus(retValue, CmpCommon::diags(), 
      TMUDFNode->getUserTableName().getExposedNameAsAnsiString().data());
    bindWA->setErrStatus();
    return FALSE; 
  }

  NAHeap* h = CmpCommon::statementHeap();
  NAColumnArray * inpColArray = new (h) NAColumnArray(h);
  for (Int32 i=0; i < (Int32) tmudfInfo_->num_inputs; i++)
  {
    if ((!input_values) || !(&(input_values[i])))
    {
      retValue = SQLUDR_ERROR;
      sprintf(msgText_, 
	      "Returned parameter input_value[%d] is null after a call to ValidateScalarInputs",i);
      processReturnStatus(retValue, CmpCommon::diags(), 
      TMUDFNode->getUserTableName().getExposedNameAsAnsiString().data());
      bindWA->setErrStatus();
      return FALSE; 
    }

    char * paramName = input_values[i].name ;
    if (paramName == NULL)
    {
      NAString nameStr(h);
      char val[4];
      str_itoa(i,val);
      nameStr = TMUDFNode->getUserTableName().getExposedNameAsAnsiString().data();
      nameStr += "_scalarInput_";
      nameStr += val;
      paramName = (char *) nameStr.data();
    }


    NAType * inpType = UDRCreateNAType(&input_values[i], bindWA, h);
    NAColumn * inpParam = new (h) NAColumn(paramName,i,inpType,h);
    inpColArray->insert(inpParam);

    // copy info from output param to tmudfInfo
    tmudfInfo_->inputs[i] = input_values[i]; // copy all non ptr members
    if (tmudfInfo_->inputs[i].name != NULL)
    {
      delete tmudfInfo_->inputs[i].name;
    }
    tmudfInfo_->inputs[i].name = new (h) char[strlen(paramName)+1];
    strncpy(tmudfInfo_->inputs[i].name, paramName, strlen(paramName)+1);
  }
  TMUDFNode->setScalarInputParams(inpColArray);
  return TRUE;
}

NABoolean TMUDFDllInteraction::DescribeMaxOutputs(
                                TableMappingUDF * TMUDFNode, BindWA * bindWA)
{   
  if (describeMaxOutputsPtr_ == NULL)
    return TRUE;

  // Initialize SQLSTATE to all '0' characters and add a null terminator
  str_pad(sqlState_, SQLUDR_SQLSTATE_SIZE - 1, '0');
  sqlState_[SQLUDR_SQLSTATE_SIZE - 1] = 0;
  
  // Initialize SQL text to all zero bytes
  str_pad(msgText_, SQLUDR_MSGTEXT_SIZE, '\0');

  const Int32 OutputParamLimit = 100;
  const Int32 ParamNameLimit   = 128;

  SQLUDR_PARAM  *return_values = createEmptyParams(OutputParamLimit,
						    ParamNameLimit) ;
  SQLUDR_UINT32 num_return_values = 0;

  // Call the function
  Int32 retValue = SQLUDR_INVOKE_DESCMAXOUTPUTS(
                        (descMaxOutputs_func) describeMaxOutputsPtr_,
                                              inData_,  
                                              nullInData_,
                                              tmudfInfo_,
					      &num_return_values,
                                              return_values,                                  
                                              sqlState_,                                     
                                              msgText_,                                      
                                              stateArea_);
  


  // Check the return value from routine execution
  if (retValue != SQLUDR_SUCCESS)
  {
    sqlState_[SQLUDR_SQLSTATE_SIZE - 1] = 0;
    processReturnStatus(retValue, CmpCommon::diags(), 
      TMUDFNode->getUserTableName().getExposedNameAsAnsiString().data());
    bindWA->setErrStatus();
    return FALSE; 
  }

  NAHeap* h = CmpCommon::statementHeap();
  NAColumnArray * outColArray = new (h) NAColumnArray(h);
  for (Int32 i=0; i < (Int32) num_return_values; i++)
  {
    if ((!return_values) || !(&(return_values[i])))
    {
      retValue = SQLUDR_ERROR;
      sprintf(msgText_, 
	      "Returned parameter return_values[%d] is null after a call to DescribeMaxOutputs",i);
      processReturnStatus(retValue, CmpCommon::diags(), 
      TMUDFNode->getUserTableName().getExposedNameAsAnsiString().data());
      bindWA->setErrStatus();
      return FALSE; 
    }

    char * paramName = return_values[i].name ;
    if (paramName == NULL)
    {
      NAString nameStr(h);
      char val[4];
      str_itoa(i,val);
      nameStr = TMUDFNode->getUserTableName().getExposedNameAsAnsiString().data();
      nameStr += "_output_";
      nameStr += val;
      paramName = (char *) nameStr.data();
    }


    NAType * outType = UDRCreateNAType(&return_values[i], bindWA, h);
    NAColumn * outParam = new (h) NAColumn(paramName,i,outType,h);
    outColArray->insert(outParam);
  }
  TMUDFNode->setOutputParams(outColArray);
  return TRUE;
}

NABoolean TMUDFDllInteraction::DescribeInputsAndOutputs(
                                TableMappingUDF * TMUDFNode)
{ return FALSE; }

NABoolean TMUDFDllInteraction::DescribeInputPartitionAndOrder(
                                TableMappingUDF * TMUDFNode)
{ return FALSE; }

NABoolean TMUDFDllInteraction::PredicatePushDown(TableMappingUDF * TMUDFNode)
{ return FALSE; }

NABoolean TMUDFDllInteraction::Cardinality(TableMappingUDF * TMUDFNode)
{ return FALSE; }

NABoolean TMUDFDllInteraction::Constraints(TableMappingUDF * TMUDFNode)
{ return FALSE; }

NABoolean TMUDFDllInteraction::Cost(TableMappingUDF * TMUDFNode)
{ return FALSE; }

NABoolean TMUDFDllInteraction::DegreeOfParallelism(TableMappingUDF * TMUDFNode)
{ return FALSE; }

NABoolean TMUDFDllInteraction::GenerateInputPartitionAndOrder(
                                                  TableMappingUDF * TMUDFNode)
{ return FALSE; }

NABoolean TMUDFDllInteraction::DescribeOutputOrder(TableMappingUDF * TMUDFNode)
{ return FALSE; }


void TMUDFDllInteraction::setFunctionPtrs(const NAString& entryName)
{
  if (dllPtr_ == NULL)
    return ;

  NAString f1;
  f1 = entryName + "_ValidateScalarInputs" ;
  validateScalarInputsPtr_ = getRoutinePtr(dllPtr_, f1.data());

  NAString f2;
  f2 = entryName + "_DescribeMaxOutputs" ;
  describeMaxOutputsPtr_ = getRoutinePtr(dllPtr_, f2.data());

  NAString f3;
  f3 = entryName + "_DescribeInputsAndOutputs" ;
  describeInputsAndOutputsPtr_ = getRoutinePtr(dllPtr_, f3.data());

  NAString f4;
  f4 = entryName + "_DescribeInputPartitionAndOrder" ;
  describeInputPartitionAndOrderPtr_ = getRoutinePtr(dllPtr_, f4.data());

  NAString f5;
  f5 = entryName + "_PredicatePushDown" ;
  predicatePushDownPtr_ = getRoutinePtr(dllPtr_, f5.data());

  NAString f6;
  f6 = entryName + "_Cardinality" ;
  cardinalityPtr_ = getRoutinePtr(dllPtr_, f6.data());

  NAString f7;
  f7 = entryName + "_Constraints" ;
  constraintsPtr_ = getRoutinePtr(dllPtr_, f7.data());

  NAString f8;
  f8 = entryName + "_Cost" ;
  costPtr_ = getRoutinePtr(dllPtr_, f8.data());

  NAString f9;
  f9 = entryName + "_DegreeOfParallelism" ;
  degreeOfParallelismPtr_ = getRoutinePtr(dllPtr_, f9.data());

  NAString f10;
  f10 = entryName + "_GenerateInputPartitionAndOrder" ;
  generateInputPartitionAndOrderPtr_ = getRoutinePtr(dllPtr_, f10.data());

  NAString f11;
  f11 = entryName + "_describeOutputOrder" ;
  describeOutputOrderPtr_= getRoutinePtr(dllPtr_, f11.data());

}


void TMUDFDllInteraction::processReturnStatus(ComSInt32 retcode, 
                                              ComDiagsArea *diags,
                                              const char* routineName)
{
  char *returnMsgText = NULL, *noMsgText = NULL;

  if (retcode == SQLUDR_SUCCESS)
    return ;

  if (msgText_[0] != '\0')
  {
    returnMsgText = msgText_;
  }
  else
  {
    const char *text =
      "No SQL message text was provided by user-defined function ";
    ComUInt32 msgLen = str_len(text) + str_len(routineName);
    noMsgText = new (CmpCommon::statementHeap()) char[msgLen + 1];
    sprintf(noMsgText, "%s%s", text, routineName);

    returnMsgText = noMsgText;
  }

  // Check the returned SQLSTATE value and raise appropriate
  // SQL code. Valid SQLSTATE values begin with "38" except "38000"
  if ((strncmp(sqlState_, "38", 2) == 0) &&
      (strncmp(sqlState_, "38000", 5) != 0))
  {
    Int32 sqlCode = (retcode == SQLUDR_ERROR) ?
      -LME_CUSTOM_ERROR : LME_CUSTOM_WARNING;

    *diags << DgSqlCode(sqlCode)
           << DgString0(returnMsgText)
	   << DgString1(sqlState_);
    *diags << DgCustomSQLState(sqlState_);
  }
  else
  {
    Int32 sqlCode = (retcode == SQLUDR_ERROR) ? -LME_UDF_ERROR : LME_UDF_WARNING;

    *diags << DgSqlCode(sqlCode)
	   << DgString0(routineName)
           << DgString1(sqlState_)
           << DgString2(returnMsgText);
  }  
  return ;
}
