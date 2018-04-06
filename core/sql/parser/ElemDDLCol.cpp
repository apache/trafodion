/* -*-C++-*-
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
 *****************************************************************************
 *
 * File:         ElemDDLCol.C
 * Description:  methods for classes relating to columns.
 *
 * Created:      9/21/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "AllElemDDLCol.h"
#include "ComASSERT.h"
#include "ComDiags.h"
#include "ComOperators.h"
#include "ElemDDLConstraintAttrDroppable.h"
#include "ElemDDLConstraintPK.h"
#include "ElemDDLConstraintRI.h"
#include "ElemDDLConstraintUnique.h"
#include "ItemColRef.h"
#include "ElemDDLLoggable.h"
#include "ElemDDLLobAttrs.h"
#include "DatetimeType.h"
#include "IntervalType.h"
#include "csconvert.h"

extern NABoolean getCharSetInferenceSetting(NAString& defval);

// -----------------------------------------------------------------------
// methods for class ElemDDLColDef
// -----------------------------------------------------------------------

// constructor
ElemDDLColDef::ElemDDLColDef(
     const NAString *columnFamily,
     const NAString *columnName,
     NAType * pColumnDataType,
     ElemDDLNode * pColAttrList,
     CollHeap * heap)
: ElemDDLNode(ELM_COL_DEF_ELEM),
  columnName_(*columnName, heap),
  columnDataType_(pColumnDataType),
  defaultClauseStatus_(DEFAULT_CLAUSE_NOT_SPEC),
  isNewAdjustedDefaultConstValueNode_(FALSE),
  pDefault_(NULL),
  isHeadingSpec_(FALSE),
  heading_(heap),
  columnClass_(COM_USER_COLUMN),
  isNotNullSpec_(FALSE),
  isNotNullNondroppable_(FALSE),
  isLoggableSpec_(FALSE),
  isLoggable_(TRUE),
  pConstraintNotNull_(NULL),
  isPrimaryKeySpec_(FALSE),
  pConstraintPK_(NULL),
  columnConstraintArray_(heap),
  primaryKeyColRefArray_(heap),
  direction_(COM_OUTPUT_COLUMN),
  pSGOptions_(NULL),
  pSGLocation_(NULL),
  isDivisionColumn_(FALSE),
  divisionColumnSeqNum_(-1),
  isLobAttrsSpec_(FALSE),
  lobStorage_(Lob_HDFS_File),
  isSeabaseSerializedSpec_(FALSE),
  seabaseSerialized_(FALSE),
  isColDefaultSpec_(FALSE)
{
  //  ComASSERT(pColumnDataType NEQ NULL);

  if (columnFamily)
    columnFamily_ = *columnFamily;

  if (pColumnDataType NEQ NULL)
  {
    const NAString dataTypeName = pColumnDataType->getTypeName();

    // Create table with data type DATETIME not supported. Must check for DATE,
    // TIME, and TIMESTAMP as well since DATETIME might be converted into these,
    if(
      dataTypeName == "DATETIME" ||
      dataTypeName == "DATE" ||
      dataTypeName == "TIME" ||
      dataTypeName == "TIMESTAMP"
      )
    {
      // Check flag to see if DATETIME originally specified
      if( ((DatetimeIntervalCommonType *)pColumnDataType)->
	  getDTIFlag(DatetimeIntervalCommonType::UNSUPPORTED_DDL_DATA_TYPE))
      {
	// Only put error into diags if it doesn't already contain it
	if(!SqlParser_Diags->contains(-3195))
        {
	  *SqlParser_Diags << DgSqlCode(-3195) 
			   << DgString0("DATETIME");
	}
	return;
      }
    }
    // Create table with data type INTERVAL with FRACTION field(s) not supported
    else if (dataTypeName == "INTERVAL")
    {
      // Check flag to see if FRACTION was originally specified
      if( ((DatetimeIntervalCommonType *)pColumnDataType)->
	  getDTIFlag(DatetimeIntervalCommonType::UNSUPPORTED_DDL_DATA_TYPE))
      {
	// Only put error into diags if it doesn't already contain it
	if(!SqlParser_Diags->contains(-3195))
	{
	  *SqlParser_Diags << DgSqlCode(-3195)
			   << DgString0("INTERVAL with FRACTION field(s)");
	}
	return;
      }

      // Check to see if interval second is specified with leading 
      // precision of 0
      if(!((SQLInterval *)pColumnDataType)->isSupportedType())
      {
	// Only put error into diags if it doesn't already contain it
        if(!SqlParser_Diags->contains(-3195))
        {
	  *SqlParser_Diags << DgSqlCode(-3195)
			   << DgString0("INTERVAL SECOND with leading precision 0");
	}
	return;
      }
    }
  }

  setChild(INDEX_ELEM_DDL_COL_ATTR_LIST, pColAttrList);

  // initialize data member pDefault_

  ComBoolean isIdentityColumn = FALSE;

  //
  // Traverse the list of column attributes to check for duplicate
  // HEADING clause and duplicate NOT NULL column constraint definition
  //

  if (pColAttrList NEQ NULL)
  {
    for (CollIndex index = 0; index < pColAttrList->entries(); index++)
    {
      setColumnAttribute((*pColAttrList)[index]);
    }
  }

  // At this point we will know if the user
  // has specified NOT NULL NOT DROPPABLE for IDENTITY
  // column. If not specified, then automatically add
  // it. 
  if (pSGOptions_) //isIdentityColumn
    {
      // if NOT NULL not specified, then specify it here.
      if(NOT getIsConstraintNotNullSpecified())
	isNotNullSpec_ = TRUE;
      
      // [NOT] DROPPABLE is the only attribute for NOT NULL.
      if (pConstraintNotNull_)
	{
	  // if DROPPABLE was specified explicity then raise an error.
	  if (pConstraintNotNull_->isDroppableSpecifiedExplicitly())
	    {
	      *SqlParser_Diags << DgSqlCode(-3413)
			       << DgColumnName(ToAnsiIdentifier(getColumnName()));
	      return;
	    }
          else
            {
              // add the NOT DROPPABLE attribute to the NOT NULL .
              pConstraintNotNull_->setConstraintAttributes
                (new (PARSERHEAP()) ElemDDLConstraintAttrDroppable(FALSE)); 
            }
	}
      else
	{
	  // by default NOT NULLs are NOT DROPPABLEs as well in SQL/MX
	  pConstraintNotNull_ = new (PARSERHEAP()) ElemDDLConstraintNotNull(PARSERHEAP());
	  pConstraintNotNull_->setConstraintAttributes
	    (new (PARSERHEAP()) ElemDDLConstraintAttrDroppable(FALSE)); 
	}
    } //if isIdentityColumn
  
  //
  // All column attributes has been checked and saved.
  // If there exists a NOT NULL NONDROPPABLE constraint
  // associating with the currently defined column, makes
  // sure that the associating NAType (data type) parse
  // node does not allow null values.
  //

  if (getIsConstraintNotNullSpecified() AND
      NOT getConstraintNotNull()->isDroppable())
  {
    isNotNullNondroppable_ = TRUE; 
    if (columnDataType_)
      columnDataType_->setNullable(FALSE);
  }
  
}  // ElemDDLColDef()

// virtual destructor
ElemDDLColDef::~ElemDDLColDef()
{
  // delete data structures owned by the node
  delete columnDataType_;

  // Don't need to delete pDefault_ unless the
  // CatMan code created a new ConstValue parse
  // node to represent the adjusted new default
  // value.  The original ConstValue parse node
  // of the specified default value'll be deleted
  // when the children in the sub-tree are deleted.
  if (isNewAdjustedDefaultConstValueNode_)
    delete pDefault_;

  // delete all children
  for (Int32 i = 0; i < getArity(); i++)
  {
    delete getChild(i);
  }
}

// cast
ElemDDLColDef *
ElemDDLColDef::castToElemDDLColDef()
{
  return this;
}

//
// accessors
//

// get the degree of this node
Int32
ElemDDLColDef::getArity() const
{
  return MAX_ELEM_DDL_COL_DEF_ARITY;
}

ExprNode *
ElemDDLColDef::getChild(Lng32 index)
{ 
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

//
// mutators
//

void
ElemDDLColDef::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());
  if (pChildNode NEQ NULL)
  {
    ComASSERT(pChildNode->castToElemDDLNode() NEQ NULL);
    children_[index] = pChildNode->castToElemDDLNode();
  }
  else
    children_[index] = NULL;
}

void
ElemDDLColDef::setDefaultAttribute(ElemDDLNode * pColDefaultNode)
{
  ElemDDLColDefault * pColDefault = NULL;
  ComBoolean isIdentityColumn = FALSE;

  NAType * pColumnDataType = columnDataType_;

  if (pColDefaultNode NEQ NULL)
    {
      ComASSERT(pColDefaultNode->castToElemDDLColDefault() NEQ NULL);
      pColDefault = pColDefaultNode->castToElemDDLColDefault();
    }

  if (pColDefault NEQ NULL)
    {
      switch (pColDefault->getColumnDefaultType())
        {
        case ElemDDLColDefault::COL_NO_DEFAULT:
          defaultClauseStatus_ = NO_DEFAULT_CLAUSE_SPEC;
          break;
        case ElemDDLColDefault::COL_DEFAULT:
          {
            defaultClauseStatus_ = DEFAULT_CLAUSE_SPEC;

            if (pColDefault->getSGOptions())
              {
                isIdentityColumn = TRUE;
                pSGOptions_ = pColDefault->getSGOptions();
                pSGLocation_ = pColDefault->getSGLocation();
              }
            else
              {
                ComASSERT(pColDefault->getDefaultValueExpr() NEQ NULL);
                pDefault_ = pColDefault->getDefaultValueExpr();
              }
            
            // The cast ItemExpr to ConstValue for (ConstValue *)pDefault_; 
            // statement below sets arbitary value for the isNULL_. 
            // Bypass these checks for ID column (basically ITM_IDENTITY).
            ConstValue *cvDef = (ConstValue *)pDefault_;
            if ((cvDef && !cvDef->isNull()) && (!isIdentityColumn))
              {
                const NAType *cvTyp = cvDef->getType();
                NABoolean isAnErrorAlreadyIssued = FALSE;
                
                if ( cvTyp->getTypeQualifier() == NA_CHARACTER_TYPE )
                  {
                    CharInfo::CharSet defaultValueCS = ((const CharType *)cvTyp)->getCharSet();
                    // Always check for INFER_CHARSET setting before the ICAT setting.
                    NAString inferCharSetFlag;
                    if (getCharSetInferenceSetting(inferCharSetFlag) == TRUE &&
                        NOT cvDef->isStrLitWithCharSetPrefixSpecified())
                      {
                        if (pColumnDataType->getTypeQualifier() == NA_CHARACTER_TYPE
                            && ((const CharType *)pColumnDataType)->getCharSet() == CharInfo::UCS2
                            && SqlParser_DEFAULT_CHARSET == CharInfo::UCS2
                            && defaultValueCS == CharInfo::ISO88591
                            )
                          {
                            *SqlParser_Diags << DgSqlCode(-1186)
                                             << DgColumnName(ToAnsiIdentifier(getColumnName()))
                                             << DgString0(pColumnDataType->getTypeSQLname(TRUE/*terse*/))
                                             << DgString1(cvTyp->getTypeSQLname(TRUE/*terse*/));
                            isAnErrorAlreadyIssued = TRUE;
                          }
                        else
                          {
                            cvTyp = cvDef -> pushDownType(*columnDataType_, NA_CHARACTER_TYPE);
                          }
                      }
                    else if (CmpCommon::getDefault(ALLOW_IMPLICIT_CHAR_CASTING) == DF_ON &&
                             NOT cvDef->isStrLitWithCharSetPrefixSpecified() &&
                             cvTyp->getTypeQualifier() == NA_CHARACTER_TYPE &&
                             SqlParser_DEFAULT_CHARSET == CharInfo::ISO88591 &&
                             defaultValueCS == CharInfo::UnknownCharSet)
                      {
                        cvTyp = cvDef -> pushDownType(*columnDataType_, NA_CHARACTER_TYPE);
                      }
                    
                  } // column default value has character data type
                
                if (NOT isAnErrorAlreadyIssued &&
                    pColumnDataType->getTypeQualifier() == NA_CHARACTER_TYPE &&
                    cvTyp->getTypeQualifier() == NA_CHARACTER_TYPE &&
                    (
                         CmpCommon::getDefault(ALLOW_IMPLICIT_CHAR_CASTING) == DF_ON ||
                         NOT cvDef->isStrLitWithCharSetPrefixSpecified()))
                  {
                    const CharType *cdCharType = (const CharType *)pColumnDataType;
                    const CharType *cvCharType = (const CharType *)cvTyp;
                    CharInfo::CharSet cdCharSet = cdCharType->getCharSet(); // cd = column definition
                    CharInfo::CharSet cvCharSet = cvCharType->getCharSet(); // cv = constant value
                    if (cvCharSet == CharInfo::ISO88591)  // default value is a _ISO88591 str lit
                      {
                        
                      }
                    else if ( (cvCharSet == CharInfo::UNICODE ||  // default value is a _UCS2 string literal
                               cvCharSet == CharInfo::UTF8)   &&  // or a _UTF8 string literal
                              cdCharSet != cvCharSet )
                      {
                        //
                        // Check to see if all characters in the specified column default
                        // string literal value can be successfully converted/translated
                        // to the actual character set of the column.
                        //
                        char buf[2032];  // the output buffer - should be big enough
                        buf[0] = '\0';
                        enum cnv_charset eCnvCS = convertCharsetEnum( cdCharSet );
                        const char * pInStr = cvDef->getRawText()->data();
                        Int32 inStrLen = cvDef->getRawText()->length();
                        char * p1stUnstranslatedChar = NULL;
                        UInt32 outStrLenInBytes = 0;
                        unsigned charCount = 0;  // number of characters translated/converted
                        Int32 cnvErrStatus = 0;
                        char *pSubstitutionChar = NULL;
                        Int32 convFlags = 0;
                        
                        if ( cvCharSet == CharInfo::UNICODE )
                          {
                            cnvErrStatus =
                              UTF16ToLocale
                              ( cnv_version1            // in  - const enum cnv_version version
                                , pInStr                  // in  - const char *in_bufr
                                , inStrLen                // in  - const int in_len
                                , buf                     // out - const char *out_bufr
                                , 2016                    // in  - const int out_len
                                , eCnvCS                  // in  - enum cnv_charset charset
                                , p1stUnstranslatedChar   // out - char * & first_untranslated_char
                                , &outStrLenInBytes       // out - unsigned int *output_data_len_p
                                , convFlags               // in  - const int cnv_flags
                                , (Int32)TRUE               // in  - const int addNullAtEnd_flag
                                , (Int32)FALSE              // in  - const int allow_invalids
                                , &charCount              // out - unsigned int * translated_char_cnt_p
                                , pSubstitutionChar       // in  - const char *substitution_char
                                );
                          }
                        else // cvCharSet must be CharInfo::UTF8
                          {
                            cnvErrStatus =
                              UTF8ToLocale
                              ( cnv_version1            // in  - const enum cnv_version version
                                , pInStr                  // in  - const char *in_bufr
                                , inStrLen                // in  - const int in_len
                                , buf                     // out - const char *out_bufr
                                , 2016                    // in  - const int out_len
                                , eCnvCS                  // in  - enum cnv_charset charset
                                , p1stUnstranslatedChar   // out - char * & first_untranslated_char
                                , &outStrLenInBytes       // out - unsigned int *output_data_len_p
                                , (Int32)TRUE               // in  - const int addNullAtEnd_flag
                                , (Int32)FALSE              // in  - const int allow_invalids
                                , &charCount              // out - unsigned int * translated_char_cnt_p
                                , pSubstitutionChar       // in  - const char *substitution_char
                                );
                          }
                        switch (cnvErrStatus)
                          {
                          case 0: // success
                          case CNV_ERR_NOINPUT: // an empty input string will get this error code
                            {
                              ConstValue *pMBStrLitConstValue ;
                              // convert the string literal saved in cvDef (column default value)
                              // from UNICODE (e.g. UTF16) to the column character data type
                              if ( cdCharSet != CharInfo::UNICODE)
                                {
                                  NAString mbs2(buf, PARSERHEAP());  // note that buf is NULL terminated
                                  pMBStrLitConstValue =
                                    new(PARSERHEAP()) ConstValue ( mbs2
                                                                   , cdCharSet // use this for str lit prefix
                                                                   , CharInfo::DefaultCollation
                                                                   , CharInfo::COERCIBLE
                                                                   , PARSERHEAP()
                                                                   );
                                }
                              else
                                {
                                  NAWString mbs2((NAWchar*)buf, PARSERHEAP());  // note that buf is NULL terminated
                                  pMBStrLitConstValue = 
                                    new(PARSERHEAP()) ConstValue ( mbs2
                                                                   , cdCharSet // use this for str lit prefix
                                                                   , CharInfo::DefaultCollation
                                                                   , CharInfo::COERCIBLE
                                                                   , PARSERHEAP()
                                                                   );
                                }
                              delete pDefault_; // deallocate the old ConstValue object
                              cvDef = NULL;     // do not use cvDef anymore
                              pDefault_ = pMBStrLitConstValue;
                              pColDefault->setDefaultValueExpr(pDefault_);
                            }
                            break;
                          case CNV_ERR_INVALID_CHAR:
                            {
                              // 1401 ==  CAT_UNABLE_TO_CONVERT_COLUMN_DEFAULT_VALUE_TO_CHARSET
                              *SqlParser_Diags << DgSqlCode(-1401)
                                               << DgColumnName(ToAnsiIdentifier(getColumnName()))
                                               << DgString0(CharInfo::getCharSetName(cdCharSet));
                            }
                            break;
                          case CNV_ERR_BUFFER_OVERRUN: // output buffer not big enough
                          case CNV_ERR_INVALID_CS:
                          default:
                            CMPABORT_MSG("Parser internal logic error");
                            break;
                          } // switch
                      }
                    else if(!pColumnDataType->isCompatible(*cvTyp))
                      {
                        if (NOT isAnErrorAlreadyIssued)
                          {
                            *SqlParser_Diags << DgSqlCode(-1186)
                                             << DgColumnName(ToAnsiIdentifier(getColumnName()))
                                             << DgString0(pColumnDataType->getTypeSQLname(TRUE/*terse*/))
                                             << DgString1(cvTyp->getTypeSQLname(TRUE/*terse*/));
                            isAnErrorAlreadyIssued = TRUE;
                          }
                      }
                  } // column has character data type
                else
                  // if interval data type, the default value must have the same
                  // interval qualifier as the column.
                  if (NOT isAnErrorAlreadyIssued &&
                      (!pColumnDataType->isCompatible(*cvTyp) ||
                       (pColumnDataType->getTypeQualifier() == NA_INTERVAL_TYPE &&
                        pColumnDataType->getFSDatatype() != cvTyp->getFSDatatype())))
                    {
                      *SqlParser_Diags << DgSqlCode(-1186)
                                       << DgColumnName(ToAnsiIdentifier(getColumnName()))
                                       << DgString0(pColumnDataType->getTypeSQLname(TRUE/*terse*/))
                                       << DgString1(cvTyp->getTypeSQLname(TRUE/*terse*/));
                      isAnErrorAlreadyIssued = TRUE;
                    }
              }
          }
          break;
        case ElemDDLColDefault::COL_FUNCTION_DEFAULT:
          {
            defaultClauseStatus_ = DEFAULT_CLAUSE_SPEC;
            defaultExprString_= pColDefault->getDefaultExprString();
    
            ComASSERT(pColDefault->getDefaultValueExpr() NEQ NULL);
            pDefault_ = pColDefault->getDefaultValueExpr();
            
            ItemExpr *itr = pDefault_; 
            NABoolean valid = TRUE;
            //Only support to_char(cast (currenttimestamp ))
            if( pDefault_->getOperatorType() != ITM_DATEFORMAT)
              valid = FALSE;
            else 
            {
              //next should be CAST
              itr = pDefault_->child(0)->castToItemExpr();
              if(itr->getOperatorType() != ITM_CAST)
                valid = FALSE;
              else
              {
                itr = itr->child(0)->castToItemExpr();
                if(itr->getOperatorType() !=  ITM_CURRENT_TIMESTAMP)
                 valid = FALSE;
              }
            }
            if( valid == FALSE )
            {
              *SqlParser_Diags << DgSqlCode(-1084)
                               << DgColumnName(ToAnsiIdentifier(getColumnName()));
            }
          }
          break;
        case ElemDDLColDefault::COL_COMPUTED_DEFAULT:
          {
            defaultClauseStatus_ = DEFAULT_CLAUSE_SPEC;
            computedDefaultExpr_ = pColDefault->getComputedDefaultExpr();
          }
          break;
        default:
          CMPABORT_MSG("Parser internal logic error");
          break;
        }
    }

}

void
ElemDDLColDef::setColumnAttribute(ElemDDLNode * pColAttr)
{
  switch(pColAttr->getOperatorType())
  {
  case ELM_COL_HEADING_ELEM :
    if (isHeadingSpec_)
    {
       // Duplicate HEADING clauses in column definition.
      *SqlParser_Diags << DgSqlCode(-3051)
                       << DgColumnName(ToAnsiIdentifier(getColumnName()));
    }
    ComASSERT(pColAttr->castToElemDDLColHeading() NEQ NULL);
    heading_ = pColAttr->castToElemDDLColHeading()->getColumnHeading();
    isHeadingSpec_ = TRUE;

    // Report an error if heading_ is too long.
    if (heading_.length() > ElemDDLColHeading::maxHeadingLength)
    {
      *SqlParser_Diags << DgSqlCode(-3132)
                       << DgColumnName(ToAnsiIdentifier(getColumnName())); 
    }
    break;

  case ELM_CONSTRAINT_CHECK_ELEM :
    ComASSERT(pColAttr->castToElemDDLConstraintCheck() NEQ NULL);
    columnConstraintArray_.insert(pColAttr->castToElemDDLConstraint());
    break;

  case ELM_CONSTRAINT_NOT_NULL_ELEM :
    ComASSERT(pColAttr->castToElemDDLConstraintNotNull() NEQ NULL);
    if (isNotNullSpec_)
    {
      // Duplicate NOT NULL clauses in column definition.
      *SqlParser_Diags << DgSqlCode(-3052)
                       << DgString0("NOT NULL")
                       << DgColumnName(ToAnsiIdentifier(getColumnName()));
    }
    isNotNullSpec_ = TRUE;
    pConstraintNotNull_ = pColAttr->castToElemDDLConstraintNotNull();

    if (NOT pConstraintNotNull_->isDroppable())
      {
	isNotNullNondroppable_ = TRUE;    
	if (columnDataType_)
	  columnDataType_->setNullable(FALSE);
      }

    // Note that we do not insert pConstraintNotNull_ into
    // columnConstraintArray_ even though Not Null constraint is
    // also a column constraint.  The user can use the accessors
    // getIsConstraintNotNullSpecified and getConstraintNotNull
    // instead.

    break;


  case ELM_LOGGABLE:
	  ComASSERT( NULL NEQ pColAttr->castToElemDDLLoggable())
		if(TRUE == isLoggableSpec_)
		{
			// Duplicate LOGGABLE in column definition.
			*SqlParser_Diags << DgSqlCode(-12064)
						   << DgColumnName(ToAnsiIdentifier(getColumnName()));
		}

		isLoggableSpec_ = TRUE;
		isLoggable_ = pColAttr->castToElemDDLLoggable()->getIsLoggable();
		break;


  case ELM_CONSTRAINT_PRIMARY_KEY_COLUMN_ELEM :
    {
      ComASSERT(pColAttr->castToElemDDLConstraintPKColumn() NEQ NULL);
      ComASSERT(pColAttr->castToElemDDLConstraintPKColumn()
                        ->getConstraintKind()
             EQU ElemDDLConstraint::COLUMN_CONSTRAINT_DEF);
      if (isPrimaryKeySpec_)
      {
	// Duplicate PRIMARY KEY clauses in column definition.
	*SqlParser_Diags << DgSqlCode(-3053)
                         << DgColumnName(ToAnsiIdentifier(getColumnName()));
      }
      isPrimaryKeySpec_ = TRUE;
      ElemDDLConstraintPKColumn * pColPKConstraint =
                                  pColAttr->castToElemDDLConstraintPKColumn();
      //
      // Copies the pointer to the parse node representing the column
      // primary key constraint to pConstraintPK_ so the user (caller)
      // can access the information easier.  Note that this pointer is
      // not inserted into columnConstraintArray_ because primary key
      // constraint is special.  (There can only be one primary key
      // constraint associating with a table.)  The user (caller) can
      // use method getIsConstraintPKSpecified() and getConstraintPK()
      // to get the primary key constraint information.
      //
      pConstraintPK_ = pColPKConstraint;

      // The column name is not specified in the column primary key
      // constraint definition.  To make the user (caller) to access
      // to this information easier, creates a parse node containing
      // the column name.
      
      ComASSERT(pColPKConstraint->getColumnRefList() EQU NULL);
      ElemDDLColRef * pColRef = new(PARSERHEAP())
	ElemDDLColRef(getColumnName(),
		      pColPKConstraint->
		      getColumnOrdering(),
                      PARSERHEAP());
      pColPKConstraint->setColumnRefList(pColRef);
      primaryKeyColRefArray_.insert(pColRef);
    }
    break;

  case ELM_CONSTRAINT_REFERENTIAL_INTEGRITY_ELEM :
    {
      ComASSERT(pColAttr->castToElemDDLConstraintRI() NEQ NULL);
      ComASSERT(pColAttr->castToElemDDLConstraintRI()->getConstraintKind()
                EQU ElemDDLConstraint::COLUMN_CONSTRAINT_DEF);
      columnConstraintArray_.insert(pColAttr->castToElemDDLConstraint());
      //
      // The column name is not specified in the column referential
      // integrity constraint definition.  To make the user (caller)
      // to access to this information easier, creates a parse node
      // containing the column name.
      //
      ElemDDLConstraintRI * pColRIConstraint =
                            pColAttr->castToElemDDLConstraintRI();
      ComASSERT(pColRIConstraint->getReferencingColumnNameList() EQU NULL);
      ElemDDLColName * pColName = new(PARSERHEAP())
	ElemDDLColName(getColumnName());
      pColRIConstraint->setReferencingColumnNameList(pColName);
    }
    break;

  case ELM_CONSTRAINT_UNIQUE_ELEM :
    {
      ComASSERT(pColAttr->castToElemDDLConstraintUnique() NEQ NULL);
      ComASSERT(pColAttr->castToElemDDLConstraintUnique()->getConstraintKind()
                EQU ElemDDLConstraint::COLUMN_CONSTRAINT_DEF);
      columnConstraintArray_.insert(pColAttr->castToElemDDLConstraint());
      //
      // The column name is not specified in the column unique
      // constraint definition.  To make the user (caller) to access
      // to this information easier, creates a parse node containing
      // the column name.
      //
      ElemDDLConstraintUnique * pColUniqueConstraint =
                                pColAttr->castToElemDDLConstraintUnique();
      ComASSERT(pColUniqueConstraint->getColumnRefList() EQU NULL);
      ElemDDLColRef * pColRef = new(PARSERHEAP())
	ElemDDLColRef(getColumnName(),
		      COM_ASCENDING_ORDER,
		      PARSERHEAP());
      pColUniqueConstraint->setColumnRefList(pColRef);
    }
    break;

  case ELM_LOBATTRS:
    {
      ComASSERT( NULL NEQ pColAttr->castToElemDDLLobAttrs())
	if(TRUE == isLobAttrsSpec_)
	  {
	    // Duplicate LOB attrs in column definition.
            *SqlParser_Diags << DgSqlCode(-3052)
                             << DgString0("LOB")
                             << DgColumnName(ToAnsiIdentifier(getColumnName()));
	  }
      
      isLobAttrsSpec_ = TRUE;
      lobStorage_ = pColAttr->castToElemDDLLobAttrs()->getLobStorage();
    }
    break;

  case ELM_SEABASE_SERIALIZED:
    {
      ComASSERT( NULL NEQ pColAttr->castToElemDDLSeabaseSerialized())
	if(TRUE == isSeabaseSerializedSpec_)
	  {
	    // Duplicate SERIALIZED attrs in column definition.
            *SqlParser_Diags << DgSqlCode(-3052)
                             << DgString0("SERIALIZED")
                             << DgColumnName(ToAnsiIdentifier(getColumnName()));
	  }
      
      isSeabaseSerializedSpec_ = TRUE;
      seabaseSerialized_ =  pColAttr->castToElemDDLSeabaseSerialized()->serialized();
    }
    break;

  case ELM_COL_DEFAULT_ELEM:
    {
      ComASSERT( NULL NEQ pColAttr->castToElemDDLColDefault());
	if(TRUE == isColDefaultSpec_)
	  {
	    // Duplicate DEFAULT attrs in column definition.
	    *SqlParser_Diags << DgSqlCode(-3052)
                             << DgString0("DEFAULT")
                             << DgColumnName(ToAnsiIdentifier(getColumnName()));
	  }
      
      isColDefaultSpec_ = TRUE;
      setDefaultAttribute(pColAttr->castToElemDDLColDefault());
    }
    break;

  default :
    ABORT("internal logic error");
    break;
    
  }  // switch
}

void
ElemDDLColDef::setDefaultValueExpr(ItemExpr *pNewDefValNode)
{
  if (pDefault_ EQU pNewDefValNode)
    return;
  // Do not remove the original Default Value ConstValue parse
  // node because it is part of the children sub-tree.  The
  // entire children sub-treebe will be removed later by the
  // destructor.
  isNewAdjustedDefaultConstValueNode_ = TRUE;
  pDefault_ = pNewDefValNode;
}

NAString ElemDDLColDef::getColDefAsText() const
{
  NAString text;

  text = getColumnName();

  text += " ";

  if (getColumnDataType())
    {
      getColumnDataType()->getMyTypeAsText(&text, FALSE);
    }

  text += " ";

  if (getDefaultClauseStatus() == NO_DEFAULT_CLAUSE_SPEC)
    {
      text += "NO DEFAULT ";
    }
  else if (getDefaultValueExpr() NEQ NULL)
    {
      if (getDefaultValueExpr()->getOperatorType() == ITM_IDENTITY)
	{
	  text += "GENERATED BY DEFAULT AS IDENTITY ";
	}
      else
	{
	  text += "DEFAULT ";
	  text += getDefaultValueExpr()->getText();
	}
    }

  text += " ";

  if (getIsConstraintNotNullSpecified())
    {
      text += "NOT NULL NOT DROPPABLE";
    }

  text += " ";

  if (getIsConstraintPKSpecified())
    {
      text += "PRIMARY KEY ";
    }

  for (CollIndex i = 0; i < columnConstraintArray_.entries(); i++)
    {
      ElemDDLNode * pColAttr = columnConstraintArray_[i];
      if ((pColAttr) && (pColAttr->castToElemDDLConstraintUnique()) &&
	  (pColAttr->castToElemDDLConstraintUnique()->getConstraintKind()
	   EQU ElemDDLConstraint::COLUMN_CONSTRAINT_DEF))
	{
	  text += "UNIQUE ";
	}
    }

  return text;
}

void
ElemDDLColDef::setSGOptions(ElemDDLSGOptions *pSGOptions)
{
  pSGOptions_ = pSGOptions;
}

//
// Helper methods
//

//
// methods for tracing
//

const NAString
ElemDDLColDef::displayLabel1() const
{
  return NAString("Column name: ") + getColumnName();
}

const NAString
ElemDDLColDef::displayLabel2() const
{
  return NAString("Data type:   ") + getColumnDataType()->getSimpleTypeName();
}

NATraceList
ElemDDLColDef::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailTextList.append(displayLabel1());  // column name
  detailTextList.append(displayLabel2());  // column data type

  //
  // default value for column
  //
  
  if (getDefaultValueExpr() NEQ NULL)
  {
    detailText = "Default value: ";
    detailText += getDefaultValueExpr()->getText();
    detailTextList.append(detailText);
  }
  else
  {
    detailTextList.append("No default value.");
  }

  //
  // heading information for column
  //

  if (NOT getHeading().isNull())
  {
    detailText = "Heading: ";
    detailText += getHeading();
    detailTextList.append(detailText);
  }
  else
  {
    detailTextList.append("No heading.");
  }

  //
  // not null (column) constraint information
  //

  detailText = "notnull? ";
  detailText += YesNo(getIsConstraintNotNullSpecified());
  detailTextList.append(detailText);

  //
  // primary key column constraint information
  //

  detailText = "prikey?  ";
  detailText += YesNo(getIsConstraintPKSpecified());
  detailTextList.append(detailText);
  
  //
  // information about other column constraint definitions
  //

  const ElemDDLConstraintArray & consList = getConstraintArray();
  CollIndex nbrConstraints = consList.entries();

  if (nbrConstraints EQU 0)
  {
    detailTextList.append("No column constraints.");
  }
  else
  {
    detailText = "Column Constraints list [";
    detailText += LongToNAString((Lng32)nbrConstraints);
    detailText += " element(s)]:";
    detailTextList.append(detailText);

    for (CollIndex i = 0; i < nbrConstraints; i++)
    {
      ElemDDLConstraint * cons = consList[i];

      detailText = "[column constraint ";
      detailText += LongToNAString((Lng32)i);
      detailText += "]";
      detailTextList.append(detailText);

      NATraceList constraintDetailTextList = cons->getDetailInfo();

      for (CollIndex j = 0; j <  constraintDetailTextList.entries(); j++)
      {
        detailTextList.append(NAString("    ") + constraintDetailTextList[j]);
      }
    }
  }

  return detailTextList;

} // ElemDDLColDef::getDetailInfo()

const NAString
ElemDDLColDef::getText() const
{
  return "ElemDDLColDef";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLColDefArray
// -----------------------------------------------------------------------

// constructor
ElemDDLColDefArray::ElemDDLColDefArray(CollHeap *heap)
  : LIST(ElemDDLColDef *)(heap)
{
}
                             
// virtual destructor
ElemDDLColDefArray::~ElemDDLColDefArray()
{
}


// See if this columnName is in a ElemDDLColDefArray.  Returns the index,
// -1 if not found.
Int32 
ElemDDLColDefArray::getColumnIndex(const NAString & internalColumnName)
{
  Int32 thisEntryCount = this->entries();

  for(Int32 i = 0; i < thisEntryCount; i++)
  { 
    if ((*this)[i]->getColumnName() == internalColumnName)
      return i;
  }
  return -1;
}

// -----------------------------------------------------------------------
// methods for class ElemDDLColDefault
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLColDefault::~ElemDDLColDefault()
{
  // delete all children
  for (Int32 i = 0; i < getArity(); i++)
  {
    delete getChild(i);
  }
}

// cast
ElemDDLColDefault *
ElemDDLColDefault::castToElemDDLColDefault()
{
  return this;
}

//
// accessors
//

Int32
ElemDDLColDefault::getArity() const
{
  return MAX_ELEM_DDL_COL_DEFAULT_ARITY;
}

ExprNode *
ElemDDLColDefault::getChild(Lng32 index)
{
  ComASSERT(index EQU INDEX_DEFAULT_VALUE_EXPR);
  return defaultValueExpr_;
}

// mutator
void
ElemDDLColDefault:: setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index EQU INDEX_DEFAULT_VALUE_EXPR);
  if (pChildNode NEQ NULL)
  {
    ComASSERT(pChildNode->castToItemExpr() NEQ NULL);
    defaultValueExpr_ = pChildNode->castToItemExpr();
  }
  else
  {
    defaultValueExpr_ = NULL;
  }
}

//
// methods for tracing
//

const NAString
ElemDDLColDefault::displayLabel1() const
{
  switch (getColumnDefaultType())
  {
  case COL_DEFAULT :
  case COL_FUNCTION_DEFAULT :
    return NAString("Type: Default");
  case COL_NO_DEFAULT :
    return NAString("Type: No Default");
  default :
    ABORT("internal logic error");
    return NAString();
  }
}

const NAString
ElemDDLColDefault::displayLabel2() const
{
  if ((getColumnDefaultType() EQU COL_DEFAULT)
      || getColumnDefaultType() EQU COL_FUNCTION_DEFAULT)
  {
    return (NAString("Default value: ") +
            getDefaultValueExpr()->getText());
  }
  else
  {
    return NAString();
  }
}

const NAString
ElemDDLColDefault::getText() const
{
  return "ElemDDLColDefault";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLColHeading
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLColHeading::~ElemDDLColHeading()
{
}

// cast
ElemDDLColHeading *
ElemDDLColHeading::castToElemDDLColHeading()
{
  return this;
}

// methods for tracing

const NAString
ElemDDLColHeading::displayLabel1() const
{
  switch (getColumnHeadingType())
  {
  case COL_HEADING :
    return NAString("Type: Heading");
  case COL_NO_HEADING :
    return NAString("Type: No Heading");
  default :
    ABORT("internal logic error");
    return NAString();
  }
}

const NAString
ElemDDLColHeading::displayLabel2() const
{
  switch (getColumnHeadingType())
  {
  case COL_HEADING :
    return NAString("Heading: ") + getColumnHeading();
  case COL_NO_HEADING :
    return NAString();
  default :
    ABORT("internal logic error");
    return NAString();
  }
}

const NAString
ElemDDLColHeading::getText() const
{
  return "ElemDDLColHeading";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLColName
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLColName::~ElemDDLColName()
{
}

// cast virtual function
ElemDDLColName *
ElemDDLColName::castToElemDDLColName()
{
  return this;
}

// methods for tracing

const NAString
ElemDDLColName::getText() const
{
  return "ElemDDLColName";
}

const NAString
ElemDDLColName::displayLabel1() const
{
  return NAString("Column name: ") + getColumnName();
}

// -----------------------------------------------------------------------
// methods for class ElemDDLColNameArray
// -----------------------------------------------------------------------

// constructor
ElemDDLColNameArray::ElemDDLColNameArray(CollHeap *heap)
  : LIST(ElemDDLColName *)(heap)
{
}

// virtual destructor
ElemDDLColNameArray::~ElemDDLColNameArray()
{
}

// -----------------------------------------------------------------------
// methods for class ElemDDLColRef
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLColRef::~ElemDDLColRef()
{
}

// cast virtual function
ElemDDLColRef *
ElemDDLColRef::castToElemDDLColRef()
{
  return this;
}

// accessor
NAString
ElemDDLColRef::getColumnOrderingAsNAString() const
{
  switch(getColumnOrdering())
  {
  case COM_ASCENDING_ORDER :
    return NAString("Ascending");
  case COM_DESCENDING_ORDER :
    return NAString("Descending");
  default : 
    ABORT("internal logic error");
    return NAString();
  }
}

//
// methods for tracing
//

const NAString
ElemDDLColRef::getText() const
{
  return "ElemDDLColRef";
}

const NAString
ElemDDLColRef::displayLabel1() const
{
  return NAString("Column name:   ") + getColumnName();
}

const NAString
ElemDDLColRef::displayLabel2() const
{
  return NAString("Order:         ") + getColumnOrderingAsNAString();
}

// -----------------------------------------------------------------------
// methods for class ElemDDLColRefArray
// -----------------------------------------------------------------------

// constructor
ElemDDLColRefArray::ElemDDLColRefArray(CollHeap *heap)
  : LIST(ElemDDLColRef *)(heap)
{
}

// virtual destructor
ElemDDLColRefArray::~ElemDDLColRefArray()
{
}

// see if this ElemDDLColRefArray has other ElemDDLColRefArray
// as a prefix.
ComBoolean
ElemDDLColRefArray::hasPrefix(ElemDDLColRefArray &other)
{
  Int32 otherEntryCount = other.entries();
  Int32 thisEntryCount = this->entries();

  // See if this array has atleast as many entries as the
  // other.
  if (thisEntryCount < otherEntryCount) return FALSE;

  Int32 i = 0;
  for(; i < otherEntryCount; i++)
  {
    if ((*this)[i]->getColumnName() != other[i]->getColumnName())
       break;
    if ((*this)[i]->getColumnOrdering() != other[i]->getColumnOrdering())
       break;
  }
  if (i == otherEntryCount)
     return TRUE;
  else
     return FALSE;
}


// See if this columnName is in a ElemDDLColRefArray.  Returns the index,
// -1 if not found.
Int32 
ElemDDLColRefArray::getColumnIndex(const NAString & columnName)
{
  Int32 thisEntryCount = this->entries();

  for(Int32 i = 0; i < thisEntryCount; i++)
  { 
    if ((*this)[i]->getColumnName() == columnName)
      return i;
  }
  return -1;
}


// see if this ElemDDLColRefArray contains other ElemDDLColRefArray.
// The columns need not be in the same order.
ComBoolean 
ElemDDLColRefArray::contains(ElemDDLColRefArray &other
                            , Int32 &firstUnmatchedEntry)
{
  Int32 i, j;
  Int32 otherEntryCount = other.entries();
  Int32 thisEntryCount = this->entries();

  firstUnmatchedEntry = -1;

  // See if this array has atleast as many entries as the
  // other.
  if (thisEntryCount < otherEntryCount) return FALSE;

  for(i = 0; i < otherEntryCount; i++)
  { 
    for(j = 0; j < thisEntryCount; j++)
    {
      if ((*this)[j]->getColumnName() == other[i]->getColumnName())
         break;
    }
    // if we are past the last element in this array, we did not match
    // a column name from the other array.
    if (j == thisEntryCount)
    {
        firstUnmatchedEntry = i;
        return FALSE;
    }
  }
  // all columns of other array have matching column names in this array.
  return TRUE;
}

// see if the ElemDDLColRefArray matches the other ElemDDLColRefArray.
// The columns need not be in the same order.
ComBoolean 
ElemDDLColRefArray::matches(ElemDDLColRefArray &other)
{
  Int32 junk;
  Int32 otherEntryCount = other.entries();
  Int32 thisEntryCount = this->entries();

  if (otherEntryCount != thisEntryCount)
  {
    return FALSE;
  }

  if ( ! this->contains(other, junk))
  {
    return FALSE;
  }

  if ( ! other.contains(*this, junk))
  {
    return FALSE;
  }

  return TRUE;
}

// see if this ElemDDLColRefArray has the ElemDDLColRef as an entry.
ComBoolean 
ElemDDLColRefArray::hasEntry(ElemDDLColRef &colRef)
{
  Int32 j;
  Int32 thisEntryCount;

  thisEntryCount = this->entries();

    for(j = 0; j < thisEntryCount; j++)
    {
      if ((*this)[j]->getColumnName() == colRef.getColumnName())
         break;
    }
    // if we are past the last element in this array, we did not match
    // the column name.
    if (j == thisEntryCount)
       return FALSE;
    else
       return TRUE;
}

// Compare column names and their order with other.
ComBoolean
ElemDDLColRefArray::operator == (ElemDDLColRefArray & other)
{
  if (this->entries() != other.entries())
    return FALSE;
  for (int k = 0; k < this->entries(); k++)
  {
    if ((*this)[k]->getColumnName() != other[k]->getColumnName())
    {
      return FALSE;
    }
  }
  return TRUE;
}

// Compare column names and their order with other.
ComBoolean
ElemDDLColRefArray::operator != (ElemDDLColRefArray & other)
{
  return !((*this) == other);
}
// -----------------------------------------------------------------------
// methods for class ElemDDLColViewDef
// -----------------------------------------------------------------------

// constructor
ElemDDLColViewDef::ElemDDLColViewDef(const NAString & columnName,
                                     ElemDDLNode * pColAttrList)
: ElemDDLNode(ELM_COL_VIEW_DEF_ELEM),
  columnName_(columnName, PARSERHEAP()),
  heading_(PARSERHEAP()),
  isHeadingSpec_(FALSE)
{
  setChild(INDEX_ELEM_DDL_COL_ATTR_LIST, pColAttrList);

  // Traverse the list of column attributes to check for
  // duplicate HEADING clause

  if (pColAttrList NEQ NULL)
  {
    for (CollIndex index = 0; index < pColAttrList->entries(); index++)
    {
      setColumnAttribute((*pColAttrList)[index]);
    }
  }
}  // ElemDDLColViewDef()

// virtual destructor
ElemDDLColViewDef::~ElemDDLColViewDef()
{
  // delete all children
  for (Int32 i = 0; i < getArity(); i++)
  {
    delete getChild(i);
  }
}

// cast
ElemDDLColViewDef *
ElemDDLColViewDef::castToElemDDLColViewDef()
{
  return this;
}

//
// accessors
//

// get the degree of this node
Int32
ElemDDLColViewDef::getArity() const
{
  return MAX_ELEM_DDL_COL_VIEW_DEF_ARITY;
}

ExprNode *
ElemDDLColViewDef::getChild(Lng32 index)
{ 
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

//
// mutators
//

void
ElemDDLColViewDef::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());
  if (pChildNode NEQ NULL)
  {
    ComASSERT(pChildNode->castToElemDDLNode() NEQ NULL);
    children_[index] = pChildNode->castToElemDDLNode();
  }
  else
  {
    children_[index] = NULL;
  }
}

void
ElemDDLColViewDef::setColumnAttribute(ElemDDLNode * pColAttr)
{ 
  switch(pColAttr->getOperatorType())
  {
  case ELM_COL_HEADING_ELEM :
    if (isHeadingSpec_)
    {

      // Duplicate HEADING clauses in column definition.
      *SqlParser_Diags << DgSqlCode(-3051)
                       << DgColumnName(ToAnsiIdentifier(getColumnName()));
    }
    ComASSERT(pColAttr->castToElemDDLColHeading() NEQ NULL);
    heading_ = pColAttr->castToElemDDLColHeading()->getColumnHeading();
    isHeadingSpec_ = TRUE;
    // Report an error if heading_ is too long.
    if (heading_.length() > ElemDDLColHeading::maxHeadingLength)
    {
      *SqlParser_Diags << DgSqlCode(-3132)
                       << DgColumnName(ToAnsiIdentifier(getColumnName()));
    }
    break;

  default :
    ABORT("internal logic error");
    break;
    
  }  // switch
}

//
// methods for tracing
//

const NAString
ElemDDLColViewDef::displayLabel1() const
{
  return NAString("Column name: ") + getColumnName();
}

const NAString
ElemDDLColViewDef::displayLabel2() const
{
  if (isHeadingSpecified())
  {
    return NAString("Heading:   ") + getHeading();
  }
  else
  {
    return NAString("Heading not spec.");
  }
}

NATraceList
ElemDDLColViewDef::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailTextList.append(displayLabel1());  // column name
  detailTextList.append(displayLabel2());  // heading

  return detailTextList;

} // ElemDDLColViewDef::getDetailInfo()

const NAString
ElemDDLColViewDef::getText() const
{
  return "ElemDDLColViewDef";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLColViewDefArray
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLColViewDefArray::~ElemDDLColViewDefArray()
{
}

// Returns -1 if the colDefParseNodeArray does not contain any division columns.
// All division columns (if exist) are at the end of the list.
ComSInt32 ElemDDLColDefArray::getIndexToFirstDivCol() const
{
  ElemDDLColDefArray * ncThis = const_cast<ElemDDLColDefArray *>(this);
  ComSInt32 idxToDivCol = -1;
  ElemDDLColDef * pColDefParseNode = NULL;
  ComSInt32 i = ncThis->entries() - 1;
  while (i >= 0)
  {
    pColDefParseNode = (*ncThis)[i];
    if (pColDefParseNode->isDivisionColumn())
      idxToDivCol = i;
    else
      break;
    i--;
  }
  return idxToDivCol;
}

// -----------------------------------------------------------------------
// methods for class ElemProxyColDef
// -----------------------------------------------------------------------

ElemProxyColDef::ElemProxyColDef(QualifiedName *tableName,
                                 NAString &colName,
                                 NAType *type,
                                 ElemDDLNode *colAttrs,
                                 CollHeap *heap)
     : ElemDDLColDef(NULL, &colName, type, colAttrs, heap),
       tableName_(tableName)
{
}

ElemProxyColDef::~ElemProxyColDef()
{
}

//
// End of File
//
