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
******************************************************************************
*
* File:         ValueDesc.C
* Description:  Value Descriptor
*
* Created:      11/16/1994
* Language:     C++
*
*
******************************************************************************
*/

#define   SQLPARSERGLOBALS_FLAGS	// must precede all #include's
#define   SQLPARSERGLOBALS_NADEFAULTS

#include "ValueDesc.h"
#include "Sqlcomp.h"
#include "BindWA.h"
#include "NormWA.h"
#include "GroupAttr.h"
#include "AllItemExpr.h"
#include "RelJoin.h"
#include "CmpStatement.h"
#include "Generator.h"
#include "GenExpGenerator.h"
#include "ExpCriDesc.h"
#include "ExpAtp.h"
#include "exp_dp2_expr.h"
#include "exp_clause_derived.h"
#include "SQLCLIdev.h"
#include "GroupAttr.h"
#include "BindWA.h"
#include "ItemOther.h"
#include "ComDefs.h"
#include "OptimizerSimulator.h"
//////////////////////////////
#include "Analyzer.h"
//////////////////////////////
//
#include "NATable.h"
#include "EncodedKeyValue.h"
#include "TrafDDLdesc.h"

#include "SqlParserGlobals.h"		// must be last #include

#define TEXT_DISPLAY_LENGTH 1001

// ***********************************************************************
// A function having an external linkage to allow display() to
// be called on a tree object. This is a workaround for bugs/missing
// functionality in ObjectCenter that cause display() to become
// an undefined symbol.
// ***********************************************************************

void displayMap(const ValueIdMap & map)
{
  map.getTopValues().display();
  map.getBottomValues().display();
}

void displaySet(const ValueIdSet & vs)
{
  vs.display();
}

void displayList(const ValueIdList & vl)
{
  vl.display();
}

void DisplayVid(CollIndex id)		// useful when debugging in MSDEV on NT
{
  ValueId vid(id);
  NAString unp(CmpCommon::statementHeap());
  vid.getItemExpr()->unparse(unp);
  cerr << "[" << id << "]" << unp << endl;
}

// ***********************************************************************
// Methods for class ValueId
// ***********************************************************************

ValueId::ValueId(ValueDesc *vdesc)
{
  ActiveSchemaDB()->insertValueDesc(vdesc);
  id_ = vdesc->getValueId();
}

ValueDesc* ValueId::getValueDesc() const
{
  ValueDesc *vdesc = (ActiveSchemaDB()->getValueDescArray())[id_];
  // The if() is for easier debugging, to break on the assertion.
  if (vdesc == NULL)
    CMPASSERT(vdesc);
  return vdesc;
}

ItemExpr* ValueId::getItemExpr() const
{
  return getValueDesc()->getItemExpr();
}

void ValueId::replaceItemExpr(ItemExpr * iePtr)
{
  iePtr->setValueId(id_);
  getValueDesc()->replaceItemExpr(iePtr);
}

const NAType& ValueId::getType() const
{
  return getValueDesc()->getDomainDesc()->getType();
}

// return TRUE iff i'm a string literal with unknown char set
NABoolean ValueId::inferableCharType()
{
  if (getType().getTypeQualifier() != NA_CHARACTER_TYPE ||
      getItemExpr()->getOperatorType() != ITM_CONSTANT) {
    return FALSE;
  }
  else {
    return ((CharType*)(&getType()))->getCharSet() == CharInfo::UnknownCharSet;
  }
}

// return TRUE iff i'm a char type with known char set
NABoolean ValueId::hasKnownCharSet(const CharType **ctp)
{
  if (getType().getTypeQualifier() != NA_CHARACTER_TYPE) {
    return FALSE;
  }
  else {
    *ctp = (const CharType*)(&getType());
    return ((CharType*)(&getType()))->getCharSet() != CharInfo::UnknownCharSet;
  }
}

void ValueId::changeType(const NAType *type)
{
  getValueDesc()->getDomainDesc()->changeType(type);
}

// ----------------------------------------------------------------------
// returns TRUE if the valueId is a veg_ref referring columns from more
// than one tables
// ----------------------------------------------------------------------

NABoolean ValueId::isInvolvedInJoinAndConst() const
{

  ValueIdSet tempSet(*this);

  if (!tempSet.referencesAConstExpr())
    return FALSE;

  ValueIdSet columns;

  getItemExpr()->findAll( ITM_BASECOLUMN, columns, TRUE, TRUE );

  // Now get all the base tables in this VEG_reference

  // see if this column is being joined to a column of another table
  SET(TableDesc *) * tablesJoined = columns.getAllTables();
  if (tablesJoined->entries() > 1)
    return TRUE;
  else
    return FALSE;
}

////////////////////////////////////////////
ColAnalysis* ValueId::colAnalysis() const
{
  ItemExpr *ie = getItemExpr();
  switch (ie->getOperatorType())
  {
    case ITM_BASECOLUMN:
      return QueryAnalysis::Instance()->getColAnalysis(id_);
    case ITM_INDEXCOLUMN:
      return QueryAnalysis::Instance()->getColAnalysis(((IndexColumn *)ie)->getDefinition());
    default:
      return NULL;
//    if (okIfNotColumn)
//      return NULL;
//    CMPASSERT("ColAnalysis call by non column");
  }
//  return NULL;
}

ColAnalysis* 
ValueId::baseColAnalysis(NABoolean *isaConstant, ValueId &vid) const 
{ 
  vid = getBaseColumn(isaConstant);
  if (isaConstant AND *isaConstant)
    return NULL;
  else
    return vid.colAnalysis(); 
}

// should be inlined
PredAnalysis* ValueId::predAnalysis() const
{
  return QueryAnalysis::Instance()->getPredAnalysis(id_);
}
////////////////////////////////////////////

void ValueId::coerceType(enum NABuiltInTypeEnum desiredQualifier,
                         enum CharInfo::CharSet charSet)
{
  //
  // If the ValueId has already been typed or the desired type is unknown,
  // do nothing.
  //
  const NAType& originalType = getType();
  if (originalType.getTypeQualifier() != NA_UNKNOWN_TYPE ||
      desiredQualifier == NA_UNKNOWN_TYPE)
    return;
  // If charset is unknown then take the default charset
  if(charSet==CharInfo::UnknownCharSet)
    charSet = CharInfo::DefaultCharSet;
  //
  // Create the desired type, retaining the original null attribute,
  // with any UPSHIFT set to FALSE.
  //
  NAType *desiredType = NULL;
  switch (desiredQualifier) {
  case NA_BOOLEAN_TYPE:
    desiredType = new STMTHEAP SQLBooleanNative(STMTHEAP, originalType.supportsSQLnull());
    break;
  case NA_CHARACTER_TYPE:
    {
      Lng32 len = CmpCommon::getDefaultNumeric(VARCHAR_PARAM_DEFAULT_SIZE);

      desiredType = new STMTHEAP
	SQLVarChar(STMTHEAP, len, //DEFAULT_CHARACTER_LENGTH,
		   originalType.supportsSQLnull(),
		   FALSE/*isUpShifted*/,
		   FALSE/*isCaseinsensitive*/,
		   charSet,
		   CharInfo::DefaultCollation,
		   CharInfo::COERCIBLE);
    }
    break;
  case NA_NUMERIC_TYPE:
    {
      Int16 DisAmbiguate = 0;
      desiredType = new STMTHEAP
      SQLNumeric(STMTHEAP, TRUE,   		// signed
                 MAX_NUMERIC_PRECISION,	// precision
                 6,    			// scale
                 DisAmbiguate, // added for 64bit proj.
                 originalType.supportsSQLnull());
    }
    break;
  default:
    ABORT("Internal error in ValueId::coerceType()");
  }

  // Propagate (pushDowntype()) this type to the children of this valueid
  // in case one of the children could not be typed.
  //
  const NAType *newType = getItemExpr()->pushDownType(*desiredType);
  changeType(newType);
}

void ValueId::coerceType(const NAType& desiredType,
                         enum NABuiltInTypeEnum defaultQualifier)
{
  //
  // If the ValueId has already been typed, do nothing.
  //
  const NAType& originalType = getType();
  NABoolean NotaCharType = TRUE;

  NAType *newType = NULL;

  // If the original is of CHARACTER type and the charset field is
  // known, return.
  if ( originalType.getTypeQualifier() == NA_CHARACTER_TYPE )
  {
     if ( ((CharType&)originalType).getCharSet() == CharInfo::ISO88591 &&
          desiredType.getTypeQualifier() == NA_CHARACTER_TYPE &&
          ((CharType&)originalType).getEncodingCharSet() != ((CharType&)desiredType).getEncodingCharSet()) {
         NotaCharType = FALSE;
         newType = originalType.newCopy(STMTHEAP);
         ((CharType*)newType) -> setEncodingCharSet(((CharType&)desiredType).getEncodingCharSet()); // for now ...
         if ( getItemExpr()->getOperatorType() == ITM_CONSTANT ) {
            ((ConstValue*)getItemExpr())->setRebindNeeded(TRUE);
         }
     } else
     // experimenting code end
     if ( ((CharType&)originalType).getCharSet() != CharInfo::UnknownCharSet ) {
       return;
     } else {
         NotaCharType = FALSE;
         if ( getItemExpr()->getOperatorType() == ITM_CONSTANT ) {
            ((ConstValue*)getItemExpr())->setRebindNeeded(TRUE);
         }
     }


     // CharSet is unknown but the desired type is not CHARACTER, assume the
     // charset is ISo88591
     if ( desiredType.getTypeQualifier() != NA_CHARACTER_TYPE ) {
       newType = originalType.newCopy(STMTHEAP);
       ((CharType*)newType) -> setCharSet(CharInfo::ISO88591);
     }

  } else {
     if ( originalType.getTypeQualifier() != NA_UNKNOWN_TYPE )
       return;

     //
     // If the desired type is unknown, set the ValueId's type according to the
     // default qualifier.
     //
     if (desiredType.getTypeQualifier() == NA_UNKNOWN_TYPE) {
        coerceType(defaultQualifier);
        return;
     }
  }

  // Copy the desired type, retaining the original null attribute,
  // but dropping any UPSHIFT attribute.
  //
  if (NotaCharType &&          // We do not want to force a characters storage values to be float
     (desiredType.getFSDatatype() == REC_FLOAT32 ||
      desiredType.getFSDatatype() == REC_FLOAT64 ))
    {
      if (desiredType.getFSDatatype() == REC_FLOAT32)
        newType = new STMTHEAP
          SQLReal(STMTHEAP, desiredType.supportsSQLnull(),
                  desiredType.getPrecision());
      else
        // ieee double, tandem real and tandem double are all
        // cast as IEEE double. Tandem real is cast as ieee double as
        // it won't 'fit' into ieee real.
        newType = new STMTHEAP
          SQLDoublePrecision(STMTHEAP, desiredType.supportsSQLnull(),
                             desiredType.getPrecision());
    }
  else {
     if ( newType == NULL )
       {
         // if param default is OFF, type tinyint as smallint.
         // This is needed until all callers/drivers have full support to
         // handle IO of tinyint datatypes.
	 if (((desiredType.getFSDatatype() == REC_BIN8_SIGNED) ||
              (desiredType.getFSDatatype() == REC_BIN8_UNSIGNED)) &&
             ((CmpCommon::getDefault(TRAF_TINYINT_SUPPORT) == DF_OFF) ||
              (CmpCommon::getDefault(TRAF_TINYINT_INPUT_PARAMS) == DF_OFF)))
	   {
             const NumericType &numType = (NumericType&)desiredType; 
 
             NABoolean isSigned = numType.isSigned();
             if (numType.getScale() == 0)
               newType = new (STMTHEAP)
                 SQLSmall(STMTHEAP, isSigned, desiredType.supportsSQLnull());
             else
               newType = new (STMTHEAP)
                 SQLNumeric(STMTHEAP, sizeof(short), 
                            numType.getPrecision(), 
                            numType.getScale(),
                            isSigned, 
                            desiredType.supportsSQLnull());
               
	   } // TinyInt
	 else if ((desiredType.getFSDatatype() == REC_BIN64_UNSIGNED) &&
                  (CmpCommon::getDefault(TRAF_LARGEINT_UNSIGNED_IO) == DF_OFF))
           {
             NumericType &nTyp = (NumericType &)desiredType;
             if (CmpCommon::getDefault(BIGNUM_IO) == DF_OFF)
               {
		 Int16 DisAmbiguate = 0;
                 newType = new (STMTHEAP)
                   SQLLargeInt(STMTHEAP, nTyp.getScale(),
                               DisAmbiguate,
                               TRUE,
                               nTyp.supportsSQLnull());
               }
             else
               {
                 newType = new (STMTHEAP)
                   SQLBigNum(STMTHEAP, MAX_HARDWARE_SUPPORTED_UNSIGNED_NUMERIC_PRECISION,
                             nTyp.getScale(),
                             FALSE,
                             FALSE,
                             nTyp.supportsSQLnull());
               }
           }
	 else if ((desiredType.getFSDatatype() == REC_BOOLEAN) &&
                  (CmpCommon::getDefault(TRAF_BOOLEAN_IO) == DF_OFF))
           {
             newType = new (STMTHEAP)
               SQLVarChar(STMTHEAP, SQL_BOOLEAN_DISPLAY_SIZE, 
                          desiredType.supportsSQLnull());
           }
	 else if ((DFS2REC::isBinaryString(desiredType.getFSDatatype())) &&
                  (CmpCommon::getDefault(TRAF_BINARY_INPUT) == DF_OFF))
	   {
             if (desiredType.getFSDatatype() == REC_BINARY_STRING)
               newType = new (STMTHEAP) 
                 SQLChar(STMTHEAP,
                         desiredType.getNominalSize(), 
                         desiredType.supportsSQLnull());
             else
               newType = new (STMTHEAP) 
                 SQLVarChar(STMTHEAP,
                            desiredType.getNominalSize(), 
                            desiredType.supportsSQLnull());               
	   } // Binary String
	 else if (DFS2REC::isBigNum(desiredType.getFSDatatype()))
	   {
	     // If bignum IO is not enabled or
	     // if max numeric precision allowed is what is supported in
	     // hardware, do what was previously being done: generate
	     // equivalent SQLNumeric class.
	     // Can't do this inside of closestEquivalentExternalType method
	     // since CmpCommon::getDefault method is not accessible from it.

	     NABoolean bignumIO = FALSE;
	     if (CmpCommon::getDefault(BIGNUM_IO) == DF_ON)
	       bignumIO = TRUE; // explicitely set to ON
	     else if (CmpCommon::getDefault(BIGNUM_IO) == DF_OFF)
	       bignumIO = FALSE; // explicitely set to OFF
	     else if (CmpCommon::getDefault(BIGNUM_IO) == DF_SYSTEM)
	       {
		 if (((SQLBigNum&)desiredType).isARealBigNum())
		   bignumIO = TRUE;
	       }
	     if (CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED) ==
		   MAX_HARDWARE_SUPPORTED_SIGNED_NUMERIC_PRECISION)
	       bignumIO = FALSE;

	     if (bignumIO)
	       {
		 Lng32 precision = 
		   MINOF(desiredType.getPrecision(), (Lng32)
			 CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED));
		 Lng32 scale     = MINOF(desiredType.getScale(), precision);
		 newType = new (STMTHEAP)
		   SQLBigNum(STMTHEAP, precision, scale, 
			     ((SQLBigNum&)desiredType).isARealBigNum(),
			     ((NumericType&)desiredType).isSigned(),
			     desiredType.supportsSQLnull());
	       }
	     else
	       {
		 Lng32 precision = MINOF(desiredType.getPrecision(), MAX_NUMERIC_PRECISION);
		 Lng32 scale     = MINOF(desiredType.getScale(), precision);
		 NABoolean isSigned = ((NumericType&)desiredType).isSigned();
		 if ((precision > MAX_HARDWARE_SUPPORTED_UNSIGNED_NUMERIC_PRECISION) &&
		     (precision <= MAX_NUMERIC_PRECISION))
		   isSigned = TRUE;
		   
		 Int16 DisAmbiguate = 0;
		 newType = new (STMTHEAP)
		   SQLNumeric(STMTHEAP, isSigned,
			      precision,
			      scale,
			      DisAmbiguate, // added for 64bit proj.
			      desiredType.supportsSQLnull());
	       }

	   } // BigNum
	 else
	   newType = desiredType.closestEquivalentExternalType(STMTHEAP);
       }
  }

  NAType *tempType = originalType.newCopy(STMTHEAP);

  // In rowsets, we preserve the nullability of the type
  if (getItemExpr()->origOpType() == ITM_ROWSETARRAY_SCAN) {
    tempType->setNullable(*newType);
  }

  // If param is being typed from odbc or jdbc, preserve the
  // nullability of the type.
  NABoolean ODBC = (CmpCommon::getDefault(ODBC_PROCESS) == DF_ON);
  NABoolean JDBC = (CmpCommon::getDefault(JDBC_PROCESS) == DF_ON);
  if ((ODBC || JDBC) &&
      //      (CmpCommon::getDefault(GENERATE_LEAN_EXPR) == DF_ON)) {
      ((CmpCommon::getDefaultLong(QUERY_OPTIMIZATION_OPTIONS) & QO_PARAM_NULLABILITY_OPT) != 0)) {
        if ((getItemExpr()->origOpType()) == ITM_DYN_PARAM)
	  tempType->setNullable(*newType);
  }

  newType->setNullable(*tempType);

  if (newType->getTypeQualifier() == NA_CHARACTER_TYPE) {
    CharType *ct = (CharType *)newType;
    ct->setUpshifted(FALSE);
  }

  // Propagate (pushDowntype()) this type to the children of this valueid
  // in case one of the children could not be typed.
  //
  const NAType* synthesizedNewType =
	getItemExpr()->pushDownType(*newType, defaultQualifier);

  changeType(synthesizedNewType);
}


NAColumn*
ValueId::getNAColumn(NABoolean okIfNotColumn) const
{
  ItemExpr *ie = getItemExpr();
  switch (ie->getOperatorType()) {
  case ITM_BASECOLUMN:
    return ((BaseColumn *)ie)->getNAColumn();
  case ITM_INDEXCOLUMN:
    return ((IndexColumn *)ie)->getNAColumn();
  case ITM_UNPACKCOL:
  case ITM_NOTCOVERED:
    return ie->child(0).getValueId().getNAColumn(okIfNotColumn);
  case ITM_INSTANTIATE_NULL:
    return (*ie)[0].getNAColumn(okIfNotColumn);
  default:
    if (okIfNotColumn)
      return NULL;
    CMPASSERT(okIfNotColumn);
  }
  return NULL;
}


NABoolean ValueId::isColumnWithNonNullNonCurrentDefault() const{
  NAColumn * nac = NULL;
  ItemExpr *ck = getItemExpr();
  if ( ck == NULL )
     return FALSE;
  switch (ck->getOperatorType()){
  case ITM_BASECOLUMN:
      nac = ((BaseColumn*)ck)->getNAColumn();
      break;
  case ITM_INDEXCOLUMN:
      nac = ((IndexColumn*)ck)->getNAColumn();
      break;
  default:
      break;
  }
  if (nac &&  nac->getDefaultValue() && nac->getDefaultClass()!=COM_NULL_DEFAULT && nac->getDefaultClass()!=COM_CURRENT_DEFAULT && nac->getDefaultClass()!=COM_CURRENT_UT_DEFAULT)
      return TRUE;
  else
      return FALSE;
}


// Since we *can* have an INSTANTIATE_NULL inside a VEG_REFERENCE, a loop
// was required for the function below.
//
// Genesis case # 10-981118-9063 was the impetus for this change; basically,
// three left joins caused the previous version of this function to have an
// assertion failure.

BaseColumn *
ValueId::castToBaseColumn(NABoolean *isaConstant) const
{
  ItemExpr *ie = getItemExpr();

  // loop until we RETURN from this function or its a NULL expression
  while (ie)
    {
      ValueId vid = NULL_VALUE_ID;

      switch (ie->getOperatorType())
        {
        case ITM_BASECOLUMN:
          return (BaseColumn *)ie;

	case ITM_INDEXCOLUMN:
	  return (BaseColumn *) ((IndexColumn *) ie)->
            getDefinition().getItemExpr();

        case ITM_INSTANTIATE_NULL:
		case ITM_UNPACKCOL:
		  ie = (*ie)[0] ;
		  break;

        case ITM_VALUEIDUNION:
		  // In case of ValueiId Union, get the child, which should be a
		  // base column.
		  vid = ((ValueIdUnion *)ie)->getSource(0);
		  if (vid == NULL_VALUE_ID)
		  {
			ie = NULL;
			break;
		  }

          ie = vid.getItemExpr();
          break;

        case ITM_ROWSETARRAY_SCAN:
          // $$$ mar: no good way to do this easily, so for now we punt
          // $$$ --> post-FCS, do something better
	  //
	  // $$$ ste: return 80 yards for a touch down.
	  // $$$ ---> score: ste 6, mar 0
          return NULL ;
          // ie = ((ValueIdUnion *)ie)->getSource(0) ;
          break;

        case ITM_VEG_REFERENCE:
          {
            ValueId kid;
            ((VEGReference*)ie)->getVEG()->getAllValues().getFirst(kid);
            ie = kid.getItemExpr();
            break;
          }

        default:
          if (ie->getArity() > 0)
            {
              ie = (*ie)[0];
              break;
            }
          else
            {
              if (isaConstant)
                {
                  *isaConstant = ie->doesExprEvaluateToConstant
                    (FALSE,TRUE);
                }
              return NULL;
            }
        }
    } // end of while
	return NULL;
}

ValueId
ValueId::getBaseColumn(NABoolean *isaConstant) const
{
  BaseColumn *bc = castToBaseColumn(isaConstant);
  return bc==NULL ? NULL_VALUE_ID : bc->getValueId();
}

ValueId ValueId::nullInstantiate(BindWA *bindWAPtr, NABoolean forceCast) const
{
  const NAType &sourceType = getType();

  // For Joins, the Normalizer requires the cast to null-instantiation
  // (InstantiateNull is a subclass of Cast),
  // so Binder here passes forceCast as TRUE.
  if (sourceType.supportsSQLnull())
    if (!forceCast)
      return *this;
//##	// It would be nice not to put an InstNull on top of an InstNull.
//##	// Unfortunately, TEST014 q15 fails when we do so --
//##	// should investigate why and fix that!
//##    else if (getItemExpr()->getOperatorType() == ITM_INSTANTIATE_NULL)
//##      return *this;
  const NAType *resultType = sourceType.synthesizeNullableType(STMTHEAP);
  InstantiateNull *instNull = new (bindWAPtr->wHeap())
				  InstantiateNull(getItemExpr(), resultType);
  instNull->bindNode(bindWAPtr);
  CMPASSERT(NOT bindWAPtr->errStatus());
  return instNull->getValueId();
};

// -----------------------------------------------------------------------
// get the value id's of all ValueIdUnion nodes which exists as sub expr
// of the item expr which has this valueid.
// Used in MergeUnion::preCodeGen() to minimize the set of outputs the
// operator should produce.
// -----------------------------------------------------------------------
void ValueId::getSubExprRootedByVidUnion(ValueIdSet & vs)
{
  ItemExpr* thisItemExpr = getItemExpr();
  if ( thisItemExpr->getOperatorType() == ITM_VALUEIDUNION )
    vs.insert( *this );
  else
    for ( Lng32 i = 0; i < thisItemExpr->getArity(); i++ )
      thisItemExpr->child(i).getValueId().getSubExprRootedByVidUnion( vs );
}

// -----------------------------------------------------------------------
// replace any BaseColumn (of the given column name) in of this value
// expression with the given expression.
// used in Insert::bindNode() to move constraints from the target table
// to the source table.
// ----------------------------------------------------------------------
void ValueId::replaceBaseColWithExpr(const NAString& colName,
				     const ValueId & vid)
{
  ItemExpr* thisItemExpr = getItemExpr();
  for( Lng32 i = 0; i < thisItemExpr->getArity(); i++ )
  {
    ValueId childValueId = thisItemExpr->child(i).getValueId();
    ItemExpr* childItemExpr = childValueId.getItemExpr();

    if( childItemExpr->getOperatorType() == ITM_BASECOLUMN )
    {
      if( ((BaseColumn*)childItemExpr)->getColName() == colName )
        thisItemExpr->setChild( i, vid.getItemExpr() );
    }
    childValueId.replaceBaseColWithExpr( colName, vid );
  }
}

// ---------------------------------------------------
// Method for applying normalizeNode() to a ValueId.
// ---------------------------------------------------
NABoolean ValueId::normalizeNode(NormWA & normWARef)
{
  ItemExpr * nePtr;              // normalized expr.
  NABoolean transformed = FALSE;
  ItemExpr * iePtr = getItemExpr(); // original expr.
  // Transform the Item Expression.
  nePtr = iePtr->getReplacementExpr()->normalizeNode( normWARef );
  // If the original expression was transformed, update the id
  if ( nePtr->castToItemExpr() != iePtr ||
       nePtr->getValueId() != *this) 
    {
      transformed = TRUE;
      id_ = nePtr->castToItemExpr()->getValueId();
    }
  
  return transformed;
}

// ***********************************************************************
// Methods for class ValueIdList
// (normalizer-related methods further below)
// ***********************************************************************

THREAD_P ObjectCounter (*ValueIdList::counter_)(0);

ValueIdList::ValueIdList(const ValueIdSet &vs, CollHeap *h) :
NAList<ValueId>(h, vs.entries())
{
  CollIndex index = 0;
  for (ValueId vid = vs.init(); vs.next(vid); vs.advance(vid))
    insertAt(index++,vid);
  CMPASSERT(index == vs.entries());
  (*counter_).incrementCounter();
}

// Initialization of list assumes array is densely populated or full
ValueIdList::ValueIdList(const ValueIdArray &va, CollHeap *h) :
NAList<ValueId>(h, va.entries())
{
  for (CollIndex i = 0; i < va.entries(); i++)
    if (va.used(i))
      insert(va[i]);
  (*counter_).incrementCounter();
}

void ValueIdList::insertSet(const ValueIdSet &other)
{
  CollIndex index = entries();
  for (ValueId vid = other.init(); other.next(vid); other.advance(vid))
    insertAt(index++,vid);
}

Lng32 ValueIdList::getRowLength() const
{
  Lng32 result = 0;

  for (CollIndex i = 0; i < entries(); i++)
    {
      result += at(i).getType().getTotalSize();
    }

  return result;
}

Lng32 ValueIdList::getRowLengthOfNumericCols() const
{
  Lng32 result = 0;

  for (CollIndex i = 0; i < entries(); i++)
    {
     if (at(i).getType().isNumeric()) 
      result += at(i).getType().getTotalSize();
    }

  return result;
}

// -----------------------------------------------------------------------
// Calculate the space needed in an sql_buffer for Mdam values.
// These values are: high, low, non-null high, non-null low, and current.
// There are five per key column.  Data in sql_buffer is eight-byte
// alligned.
// -----------------------------------------------------------------------
Lng32 ValueIdList::getMdamSqlBufferSpace() const
{
  Lng32 result = 0;

  for (CollIndex i = 0; i < entries(); i++)
    {
      result += 5 * (ROUND8(at(i).getType().getEncodedKeyLength())
                      + sizeof(tupp_descriptor));
    }

  return result;
}


// -----------------------------------------------------------------------
// prefixCoveredInSet()
// Is a prefix of this list covered in the provided set?  If so,
// return the number of covered elements.  Otherwise, return 0.
// -----------------------------------------------------------------------
Int32 ValueIdList::prefixCoveredInSet (const ValueIdSet& vidSet) const
{
  Int32 i = 0;
  ValueId simpVid;
  ValueId noInverseVid;

  // if the set's empty, clearly it doesn't cover anything!
  if ( vidSet.entries() == 0 ) return 0 ;

  while (i < (Int32)entries())
  {
    // simplify the expression
    // e.g. remove any INVERSE nodes on top,
    // and simplify any bi-arith ops involving constants to their
    // base value.
    simpVid = (*this)[i].getItemExpr()->simplifyOrderExpr()->getValueId();
    // Also compute the value id that results when the only
    // simplification that is done is to remove any INVERSE nodes -
    // i.e. if the valueId is a bi-arith op or is a bi-arith op that
    // is the child of an INVERSE node, it will remain.
    noInverseVid = (*this)[i].getItemExpr()->removeInverseOrder()->getValueId();

    // Stop unless we find the completely simplified or partially
    // simplified version of the expression in the provided set.
    if (NOT (vidSet.contains (simpVid) OR vidSet.contains(noInverseVid)) )
    {
         return i;
    }
    else
      i++;
  }

  return i;
}

// -----------------------------------------------------------------------
// allMembersCoveredInSet()
// Are all members of this list are covered in the provided set?
// If exactMatch is set: 
//    If all the members of this list are covered in the provided set AND 
//    if they are exact match, return TRUE. Otherwise, return FALSE.
// If exactMatch is not set: 
//    If all the members of this list are covered in the provided set, 
//    return TRUE. Otherwise, return FALSE.
// -----------------------------------------------------------------------
NABoolean ValueIdList::allMembersCoveredInSet(const ValueIdSet& vidSet,
                                              NABoolean exactMatch) const
{
  // Get the length of the covered prefix
  CollIndex numCovered = prefixCoveredInSet(vidSet);

  // Check if all the members of this list are covered.
  if (numCovered == entries())
  {
    // All members of this list are covered.
    // If they are exact match, return TRUE.
    if (entries() == vidSet.entries())
      return TRUE;
    else
    { 
      // The provided set has additional memebers.
      // If exactMatch is set, return FALSE. Otherwise, return TRUE.
      if (exactMatch)
        return FALSE;
      else
        return TRUE;
    }
  }
  // All members of this list are NOT covered, so return FALSE.
  return FALSE;
}

// ---------------------------------------------------------------------
// removeUncoveredSuffix()
// Check whether a prefix of this list is covered by the provided set.
// For the remaining suffix, remove those items from this list.
// ---------------------------------------------------------------------
void ValueIdList::removeUncoveredSuffix (const ValueIdSet& vidSet)
{
  Int32 index = prefixCoveredInSet(vidSet);	// last covered id in this list
  Int32 i = (Int32)entries() - 1;			// index of last key column

  // Remove all entries following the index
  while (i >= index)
    removeAt(i--);
}

// ---------------------------------------------------------------------
// findNJEquiJoinColumns()
// Determine the maximum prefix of this list that is covered
// by either a constant, a parameter/host variable, or an equi-join
// column.  Then return the list that is composed of just the equi-join
// columns. Also returns in an output parameter the columns
// of this list that were not covered.
// Input: child0Outputs, child1Inputs
// Output: uncoveredCols
// Return value: a list representing the equijoin columns found
// ---------------------------------------------------------------------
ValueIdList ValueIdList::findNJEquiJoinCols(
                           const ValueIdSet & child0Outputs,
                           const ValueIdSet & child1Inputs,
                           ValueIdList & uncoveredCols) const
{
  ValueIdList equiJoinCols;
  NABoolean isChild0Output,isChild1Input;
  GroupAttributes emptyGA;
  ValueIdSet dummy;
  CollIndex i = 0;
  CollIndex j = 0;

  // Start off assuming that all columns are not covered
  uncoveredCols = *this;

  while ( i < entries() )
  {
    // simplify the expression
    // e.g. remove any INVERSE nodes on top (i.e. DESC)
    const ValueId & noInverseVid =
      (*this)[i].getItemExpr()->removeInverseOrder()->getValueId();

    // Is the value a constant or produced by child0?
    isChild0Output = emptyGA.covers( noInverseVid, child0Outputs, dummy );
    // Is the value a parameter/host variable or produced by
    // child0 or the parent?
    isChild1Input = child1Inputs.contains( noInverseVid );

    // If the value is produced by child0 and it is also an input
    // value, then it must be an equi-join column.
    if ( isChild0Output AND isChild1Input )
    {
      equiJoinCols.insert( (*this)[i] );
      uncoveredCols.removeAt(j);
      j--;
    }
    // Otherwise, check to see if the value is a constant or a
    // parameter/host variable.
    else if ( isChild0Output OR isChild1Input )
    {
      uncoveredCols.removeAt(j);
      j--;
    }
    // Otherwise, the column is not covered and we must stop now.
    else
    {
      return equiJoinCols;
    }
    // advance loop
    i++;
    j++;
  } // end while more entries in the list

  return equiJoinCols;
} // findNJEquiJoinCols()

// -----------------------------------------------------------------------
// complifyAndCheckPrefixCovered()
// Is a prefix of this list covered in the unsimplified provided set?  If so,
// return the number of covered elements.  Otherwise, if an element
// fails the check, check to see if it is in the simplified version of
// the provided set. If the element is found there, then remove the
// original version from the list and replace it with the unsimplified
// version of that element from the provided set, and keep going.
// Otherwise, return the length of the prefix seen so far. If there
// were no matches, then we will return 0.
// -----------------------------------------------------------------------
Int32 ValueIdList::complifyAndCheckPrefixCovered (const ValueIdSet& vidSet,
                                                  const GroupAttributes *ga)
{
  // if the set's empty, clearly it doesn't cover anything!
  if ( vidSet.entries() == 0 ) return 0 ;

  Int32 i = 0;
  ValueId thisVid;

  // Set up an empty GA so we can test for coverage by the provided set.
  ValueIdSet referencedInputs;
  GroupAttributes emptyGA;
  // The provided set will be the char. outputs that provide coverage
  emptyGA.setCharacteristicOutputs(vidSet);

  while (i < (Int32)entries())
  {
    // Get the current expression
    thisVid = (*this)[i];

    // Stop unless the provided set covers the current value id expression,
    // or unless we are able to do a "complify" operation.
    if (NOT emptyGA.covers(thisVid,
                           vidSet,
                           referencedInputs))
    {
      // Prepare to see if we can do a "complify" operation.
      // First, simplify the expression
      // e.g. remove any INVERSE nodes on top,
      // and simplify any bi-arith ops involving constants to their
      // base value.
      OrderComparison order1,order2;
      const ValueId & simpThisVid =
	(*this)[i].getItemExpr()->simplifyOrderExpr(&order1)->getValueId();
      // compute the simplified version of the provided set
      ValueIdSet simpleVidSet = vidSet.simplifyOrderExpr();

      // The current expression was not covered by the unsimplified version
      // of the provided set. So, see if the simplified version of the current
      // expression is in the simplified version of the provided set.
      //
      // Note that the typical usage of this method is to check if a sort key
      // is covered by the group attributes characteristic outputs. We need
      // to use the simplified version of the current expression because
      // inverse nodes (for DESC orders) will never be in the group attributes,
      // so we must strip them off. Also note that the simplifyOrderExpr
      // method does not yet handle all the simplifications that we would
      // like. For example, it does not simplify CAST expressions.  It also
      // does not simplify expressions involving multiplication, because
      // multiplying a column by a negative number reverses the ordering of
      // the column. This is an example of why we test for strict containment
      // here with only very basic simplifications allowed: since the "complify"
      // expression is actually substituting a different expression into the
      // sort key, we must ensure that the new expression is in fact completely
      // equivalent to the old. In some cases, such as CAST, we could
      // probably be more liberal, but in other cases, such as multiplication,
      // we cannot.
      if (simpleVidSet.contains(simpThisVid))
      {
        // Need to replace the current entry with one that is based on
        // the value id of the unsimplified version in the provided set.

        // Compute the corresponding value id in the provided set
        ValueId newThisVid;
        for (ValueId vid = vidSet.init();
                           vidSet.next(vid);
                           vidSet.advance(vid))
        {
          const ValueId & simpOtherVid =
	    vid.getItemExpr()->simplifyOrderExpr(&order2)->getValueId();
          if (simpOtherVid == simpThisVid)
          {
            newThisVid = vid;
            break;
          }
        }

        if (order1==order2)
        {
          // No inverse function to worry about - just replace the
          // current valid with the one from the provided set.
          (*this)[i] = newThisVid;
        }
        else
        {
          // Need to add an inverse operator on top of the provided
          // set expression.
          ItemExpr *inverseCol;
          inverseCol = new(CmpCommon::statementHeap())
                         InverseOrder(newThisVid.getItemExpr());
          inverseCol->synthTypeAndValueId();
          (*this)[i] = inverseCol->getValueId();
        }
      } // end if the simp. set contains the simp. expresssion
      else if (ga && ga->canEliminateOrderColumnBasedOnEqualsPred(thisVid))
      {
        // if this column is constrained to a single value, then
        // simply remove it from "this"
        removeAt(i--);
      }
      else
        return i;
    }
    i++;
  } // end while

  return i;
} // complifyAndCheckPrefixCovered

// ---------------------------------------------------------------------
// complifyAndRemoveUncoveredSuffix()
// Check whether a prefix of this list is covered by the provided set.
// This might involve replacing some elements of the list with their
// corresponding versions in the provided set.
// For the remaining suffix, remove those items from this list.
// ---------------------------------------------------------------------
void ValueIdList::complifyAndRemoveUncoveredSuffix (const ValueIdSet& vidSet,
                                                    const GroupAttributes *ga)
{
  // last covered id in this list
  Int32 index = complifyAndCheckPrefixCovered(vidSet, ga);
  Int32 i = (Int32)entries() - 1;  // index of last key column

  // Remove all entries following the index
  while (i >= index)
    removeAt(i--);
}

// ---------------------------------------------------------------------
// simplifyOrderExpr()
//
//  Returns a version of the list that has all expressions involving
// columns and constants or columns and inverse nodes simplified to be
// the column only.
// ---------------------------------------------------------------------
ValueIdList ValueIdList::simplifyOrderExpr() const
{
  CollIndex i;
  ValueId svid;
  ValueIdList simpleSortCols;

  for (i = 0; i < entries(); i++)
  {
    svid = (*this)[i].getItemExpr()->simplifyOrderExpr()->getValueId();
    simpleSortCols.insert(svid);
  }

  return simpleSortCols;
}

// ---------------------------------------------------------------------
// removeInverseOrder()
//
//  Returns a version of the list that has all expressions involving
// columns and inverse nodes simplified to be the column only.
// ---------------------------------------------------------------------
ValueIdList ValueIdList::removeInverseOrder() const
{
  CollIndex i;
  ValueId svid;
  ValueIdList noInverseSortCols;

  for (i = 0; i < entries(); i++)
  {
    svid = (*this)[i].getItemExpr()->removeInverseOrder()->getValueId();
    noInverseSortCols.insert(svid);
  }

  return noInverseSortCols;
}

// -----------------------------------------------------------------------
// Method for applying normalizeNode() to a ValueIdList.
// -----------------------------------------------------------------------
NABoolean ValueIdList::normalizeNode(NormWA & normWARef)
{
  ItemExpr * nePtr;              // normalized expr.
  NABoolean transformed = FALSE;

  for ( CollIndex index = 0; index < entries(); index++ )
  {
    ItemExpr * iePtr = at(index).getItemExpr(); // original expr.
    // Transform the Item Expression.
    nePtr = iePtr->getReplacementExpr()->normalizeNode( normWARef );
    // If the original expression was transformed, update the list
    if ( nePtr->castToItemExpr() != iePtr ||
	 nePtr->getValueId() != at(index) )
    {
      transformed = TRUE;
      at(index) = nePtr->castToItemExpr()->getValueId();
    }
  } // loop over expressions list

  return transformed;

} // ValueIdList::normalizeNode()

// -----------------------------------------------------------------------
// Method for rebuilding a predicate tree from a ValueIdList.
// -----------------------------------------------------------------------
ItemExpr * ValueIdList::rebuildExprTree
                           (OperatorTypeEnum op,
                            NABoolean redriveTypeSynthesisFlag,
			    NABoolean createFinalizeResultNode) const
{
  ItemExpr * iePtr;
  ItemExpr * leftPtr = NULL;
  // ---------------------------------------------------------------------
  // Iterate over the predicate factors in the given predicate tree,
  // constructing a left-linear backbone of the specified node type.
  // ---------------------------------------------------------------------
  for (CollIndex ix = 0; ix < entries(); ix++)
    {
      iePtr = at(ix).getItemExpr();
      if (leftPtr)
	switch (op)
	  {
	  case ITM_AND:
	  case ITM_OR:
	    leftPtr = new STMTHEAP BiLogic(op,leftPtr,iePtr);
	    break;

	  case ITM_ITEM_LIST:
	    leftPtr = new STMTHEAP ItemList(leftPtr,iePtr);
	    break;

	  default:
	    ABORT("Can't do backbones with this node type");
	  }
      else
        leftPtr = iePtr;
    } // loop over predTree

  // -----------------------------------------------------------------
  // Create a BoolResult node on top of the ANDed tree, if asked for.
  // Used by generator to finalize results.
  // -----------------------------------------------------------------
  if (leftPtr && createFinalizeResultNode)
    {
      CMPASSERT(op == ITM_AND);
      leftPtr = new STMTHEAP BoolResult(leftPtr);
    }

  // ---------------------------------------------------------------------
  // Synthesize the data type for the operators in the tree and allocate
  // their ValueIds.
  // ---------------------------------------------------------------------
  if (leftPtr)
    leftPtr->synthTypeAndValueId(redriveTypeSynthesisFlag);

  return leftPtr;

} // ValueIdList::rebuildExprTree()

// -----------------------------------------------------------------------
// ValueIdList::replaceVEGExpressions()
//
// This method is used by the code generator for rewriting
// expressions that belong to a ValueIdList. A reference to
// a VEG, i.e., a ValueId Equality Group is replaced with
// an expression that belongs the VEG as well as to the
// set of availableValues that is supplied.
// -----------------------------------------------------------------------
void ValueIdList::replaceVEGExpressions
                     (const ValueIdSet & availableValues,
                      const ValueIdSet & inputValues,
                      NABoolean thisIsAKeyPredicate,
                      VEGRewritePairs * lookup,
                      NABoolean replicateExpression,
                      const ValueIdSet * outputExpr,
                      const ValueIdSet * joinInputAndPotentialOutput,
                      const IndexDesc * iDesc,
                      const GroupAttributes * left_ga,
                      const GroupAttributes * right_ga
)
{
  // ---------------------------------------------------------------------
  // Iterate over the predicate factors in the given predicate tree.
  // ---------------------------------------------------------------------
  NABoolean savedReplicateExpression = replicateExpression ;
  for (CollIndex ix = 0; ix < entries(); ix++)
    {
      // -----------------------------------------------------------------
      // Walk through the item expression tree and replace any
      // VEGPredicates or VEGReferences that are found.
      // -----------------------------------------------------------------
      ValueId exprId = at(ix);
      if (outputExpr && outputExpr->containsTheGivenValue(exprId))
        replicateExpression = FALSE;

      ItemExpr * iePtr = NULL;
      if(CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON &&
         exprId.getItemExpr()->getOperatorType() == ITM_RANGE_SPEC_FUNC)
      {
        iePtr = exprId.getItemExpr()->child(1)->replaceVEGExpressions
                                                   (availableValues,
                                                    inputValues,
                                                    thisIsAKeyPredicate,
                                                    lookup,
                                                    replicateExpression,
                                                    joinInputAndPotentialOutput,
                                                    iDesc,
                                                    left_ga,
                                                    right_ga);
      }
      else
      {

        iePtr = exprId.getItemExpr()->replaceVEGExpressions
                                                 (availableValues,
                                                  inputValues,
                                                  thisIsAKeyPredicate,
                                                  lookup,
                                                  replicateExpression,
                                                  joinInputAndPotentialOutput,
                                                  iDesc,
                                                  left_ga,
                                                  right_ga);
      }

      replicateExpression = savedReplicateExpression;
      if (iePtr) // wild card expansion was successful
	{
	  if (!iePtr->nodeIsBound() || iePtr->isPreCodeGenNATypeChanged())
	    // redrive type synthesis for both self and child nodes
	    iePtr->synthTypeAndValueId(TRUE, TRUE);
	  if (iePtr->getValueId() != exprId)
	    at(ix) = iePtr->getValueId();
	}
      else               // discard element from the list
        {
	  removeAt(ix);
          ix--;
        }

    } // loop over expression list

} // ValueIdList::replaceVEGExpressions()

OrderComparison ValueIdList::satisfiesReqdOrder(const ValueIdList & reqdOrder,
                                                GroupAttributes *ga,
                                                const ValueIdSet *preds) const
{
  NAUnsigned numReqdEntries = reqdOrder.entries();
  OrderComparison allCols = SAME_ORDER; // sort order comparison of all columns together
  OrderComparison oneCol;  // one column compared with its counterpart

  CollIndex thisIx = 0;
  CollIndex reqdIx = 0;

  ValueIdList tempThis(*this);

  // if group attributes are provided, remove any constant expressions,
  // since those should not appear in the requirements and since they
  // are not relevant to the ordering
  if (ga)
    tempThis.removeCoveredExprs(ga->getCharacteristicInputs());

  CollIndex numActEntries = tempThis.entries();

  // we assume that reqdOrder went through a RequirementGenerator
  // and is therefore already optimized (e.g. no duplicate columns)

  // is the actual sort key at least as long as the required one?
  if (numActEntries < numReqdEntries)
     return DIFFERENT_ORDER;

  for (CollIndex reqdIx = 0; reqdIx < numReqdEntries; reqdIx++)
  {
     NABoolean done = FALSE;
 
     // compare the next required order column with one or more
     // ValueIds in tempThis
     while (!done)
     {
       // make sure we have enough columns in "tempThis"
       if (thisIx >= numActEntries)
         return DIFFERENT_ORDER;

       // compare the next column of tempThis with the required order
       oneCol = reqdOrder[reqdIx].getItemExpr()->sameOrder(
            tempThis[thisIx].getItemExpr());

       done = TRUE;

       // If we didn't find the column, then try to find an equals
       // predicate that equates the column to a constant, so that
       // we can skip over it. This typically happens with computed
       // columns such as salt or division that don't have associated
       // VEGs, which are handled by tempThis.removeCoveredExprs() above.
       if (oneCol == DIFFERENT_ORDER &&
           ga &&
           (numActEntries-thisIx) > (numReqdEntries-reqdIx) && // extra cols in this
           ga->tryToEliminateOrderColumnBasedOnEqualsPred(tempThis[thisIx],
                                                          preds))
       {
         // this is a predicate of the form col = const
         // and col is our current ValueId in tempThis,
         // therefore skip over it and try again
         thisIx++;
         done = FALSE;
       }
     }


     if (reqdIx == 0)
        allCols = oneCol;
     else
        allCols = combineOrderComparisons(allCols,oneCol);

     if (allCols == DIFFERENT_ORDER)
        return allCols;

     thisIx++;
  }
  return allCols;
}

NABoolean ValueIdList::satisfiesReqdArrangement(
     const ValueIdSet &reqdArrangement,
     GroupAttributes * ga,
     const ValueIdSet *preds) const
{
  // ---------------------------------------------------------------------
  // check requirement for arrangement of data (check whether the required
  // arranged columns form a prefix of the sort key or whether the
  // table contains at most one row, in which case it satisfies any
  // required ordering)
  // ---------------------------------------------------------------------
  ValueIdSet arrCols(reqdArrangement);
  NABoolean found = TRUE;
  CollIndex startCol = 0;

  // if group attributes are provided, remove any constant expressions,
  // since those should not appear in the requirements and since they
  // are not relevant to the ordering
  if (ga)
    arrCols.removeCoveredExprs(ga->getCharacteristicInputs());

  // walk along the sort key, as long as all columns in the sort
  // keys are part of the required set of arranged columns

  for (CollIndex i = startCol; i < entries() AND found; i++)
    {
      found = FALSE;

      // try a shortcut first: is the value id a required column?
      if (reqdArrangement.contains((*this)[i]))
	{
	  // yes, the sort key column is one of the columns passed in
	  // through "reqdArrangement", this required arrangement col.
	  // is satisfied
	  found = TRUE;
	  arrCols -= (*this)[i];
	}
      else
	{
	  // do it the long way: is the sort key colum in the same order
	  // as one of the columns that are required for the arrangement?

	  ItemExpr *sortKeyCol = (*this)[i].getItemExpr();

	  for (ValueId x = reqdArrangement.init();
	       reqdArrangement.next(x);
	       reqdArrangement.advance(x))
	    {
	      if (sortKeyCol->sameOrder(x.getItemExpr()) != DIFFERENT_ORDER)
		{
		  // yes, the sort key column has the same order as or the
		  // inverse order of the value x that was passed in
		  // through "reqdArrangement", this required arrangement col.
		  // is satisfied
		  found = TRUE;
		  arrCols -= x;
		}
	    }

          if ((NOT found) && ga)
            {
              // if this column is constrained to a single value,
              // we can skip over it and continue (we set found to
              // true but don't remove anything from arrCols)
              found = ga->tryToEliminateOrderColumnBasedOnEqualsPred(
                   (*this)[i],
                   preds);
            }
	}
    }

  // if there are leftover columns from the required arrangement that
  // haven't been satisfied yet, return FALSE

  return (arrCols.isEmpty());
}

// -----------------------------------------------------------------------
// Method for applying transformNode() to a ValueIdList.
// -----------------------------------------------------------------------
NABoolean ValueIdList::transformNode(NormWA & normWARef,
                                     ExprGroupId & introduceSemiJoinHere,
				     const ValueIdSet & externalInputs)
{
  NABoolean transformed = FALSE;
  RelExpr * saveOrigPtr = introduceSemiJoinHere;

  for (CollIndex index = 0; index < entries(); index++)
    {
      ItemExpr * iePtr = at(index).getItemExpr(); // original expr.
      ExprValueId nePtr(iePtr);                   // normalized expr.
      // Transform the Item Expression.
      iePtr->getReplacementExpr()->transformNode(normWARef, nePtr,
                           introduceSemiJoinHere, externalInputs);
      // If the original expression was transformed, update the list
      if (nePtr != (const ItemExpr *)iePtr)
	{
	  at(index) = nePtr->getValueId();
	}
      // if the original expression contains an ITM_ONE_ROW aggregate as output,
      // replace it with its child (or children)

      if ( iePtr->getReplacementExpr()->getOperatorType() == ITM_ONE_ROW )
      {
        ItemExpr* ReplacedExpr =
            iePtr->getReplacementExpr()->transformOneRowAggregate( normWARef );

	// redrive type synthesis for both self and child nodes
        ReplacedExpr->synthTypeAndValueId( TRUE, TRUE );

        // For now (as of 11/17/98) assume arity of childIE is at most one
        CMPASSERT(ReplacedExpr->child(0)->castToItemExpr()->getArity() <= 1 );

        at(index) = ReplacedExpr->child(0)->castToItemExpr()->getValueId();
      }
    } // loop over expressions list

  // if a subquery was transformed, set transformed flag
  if (introduceSemiJoinHere != (const RelExpr *)saveOrigPtr)
    transformed = TRUE;

  return transformed;

} // ValueIdList::transformNode()

// ------------------------------------------------------------------------
// ValueIdList::removeCoveredExprs()
//
// Removes from the valueid set those expressions that are
// covered by the available values
// ------------------------------------------------------------------------

void ValueIdList::removeCoveredExprs(const ValueIdSet & newExternalInputs)
{
  NABoolean coverFlag;
  ValueIdSet referencedInputs;
  GroupAttributes emptyGA;

  emptyGA.setCharacteristicOutputs(newExternalInputs);
  for (CollIndex ix = 0; ix < entries(); ix++)
    {
      ValueId exprId = at(ix);
      if (exprId.getItemExpr()->getOperatorType() == ITM_VEG_PREDICATE)
        {
          VEG * vegPtr = ((VEGPredicate *)(exprId.getItemExpr()))->getVEG();
          coverFlag = emptyGA.covers(vegPtr->getVEGReference()->getValueId(),
                                     newExternalInputs,
                                     referencedInputs);
        }
      else
        coverFlag = emptyGA.covers(exprId,
                                   newExternalInputs,
                                   referencedInputs);
      if (coverFlag)
        {
	  removeAt(ix);
          ix--;
        }
    } // for
} // ValueIdList::removeCoveredExprs

// ------------------------------------------------------------------------
// ValueIdList::removeUnCoveredExprs()
//
// Removes from the valueid list those expressions that are NOT covered by
// the available values.
//
// When the first uncovered element is found, that and all subsequent list
// elements are removed.
// ------------------------------------------------------------------------

void ValueIdList::removeUnCoveredExprs(const ValueIdSet & newExternalInputs)
{
  NABoolean coverFlag;
  ValueIdSet referencedInputs;
  GroupAttributes emptyGA;

  emptyGA.setCharacteristicOutputs(newExternalInputs);
  for (CollIndex ix = 0; ix < entries(); ix++)
    {
      ValueId exprId = at(ix);
      if (exprId.getItemExpr()->getOperatorType() == ITM_VEG_PREDICATE)
        {
          VEG * vegPtr = ((VEGPredicate *)(exprId.getItemExpr()))->getVEG();
          coverFlag = emptyGA.covers(vegPtr->getVEGReference()->getValueId(),
                                     newExternalInputs,
                                     referencedInputs);
        }
      else
        coverFlag = emptyGA.covers(exprId,
                                   newExternalInputs,
                                   referencedInputs);
      if (!coverFlag)
        {
          CollIndex entriesToRemove = entries()-ix ;
          while (entriesToRemove > 0)
            {
              removeAt (entries()-1) ; // remove the last elt, n times
              entriesToRemove-- ;
            }
        }
    } // for
} // ValueIdList::removeUnCoveredExprs

// ---------------------------------------------------------------------
// removeDuplicateValueIds()
// This method removes duplicate value ids(vid) from the ValueIdList object.
// First get the last vid and walks through the list comparing this last vid
// with the rest of vids. If the previous vid is same as the next one then,
// remove the last one.
// ---------------------------------------------------------------------
void ValueIdList::removeDuplicateValueIds()
{
  CollIndex entry = entries()-1;
  for (CollIndex last = entry; last > 0; last--)
  {
    // Get the last vid from the List.
    // simplify the expression
    // e.g. remove any INVERSE nodes on top (i.e. DESC)
    ValueId  prev =
      (*this)[last].getItemExpr()->removeInverseOrder()->getValueId();
    // walk through the list comparing next vid with the prev vid.
    for (Int32 start = 0; start < (Int32)last; start++)
    {
      // Get the next value id from the List.
      // simplify the expression
      // e.g. remove any INVERSE nodes on top (i.e. DESC)
      ValueId  next =
      (*this)[start].getItemExpr()->removeInverseOrder()->getValueId();
      // If there is a duplicate vid, then remove the last one.
      if (next == prev)
      {
        removeAt(last);
        break;
      }
    } // for start
  } // for last
} // removeDuplicateValueIds()


// ---------------------------------------------------------------------
// substituteValueIds()
// This method replaces the vid with the replacementVid
// ---------------------------------------------------------------------
void ValueIdList::substituteValueIds(const ValueId vid, const ValueId replacementVid)
{
  for (CollIndex i=0; i <  entries(); i++)
  {
    // walk through the list comparing next cvid with vid.
    ValueId  cvid = (*this)[i];
    
    if (cvid == vid)
    {
       // found one to replace, replace it.
        removeAt(i);
        insertAt(i, replacementVid);
    }
  } // for i
} // substituteValueIds()

// -----------------------------------------------------------------------
// ValueIdList::removeSubqueryOrIsolatedUDFunctionPredicates()
//
// This method walks through the ItemExprs that belong to this set,
// collects the ValueIds of the subqueries or UDFs in the list. It also
// collects any ValueIdProxies associated with the said subquiery or UDF.
// -----------------------------------------------------------------------
void ValueIdList::removeSubqueryOrIsolatedUDFunctionPredicates(ValueIdSet & subqueryExpr)
{
  
  for (CollIndex ix = 0; ix < entries(); ix++)
    {
      ValueId exprId = at(ix);

      if (exprId.getItemExpr()->containsValueIdProxySibling(subqueryExpr) ||
          exprId.getItemExpr()->containsSubquery() ||
          exprId.getItemExpr()->containsIsolatedUDFunction())
          
        {
          subqueryExpr.addElement(exprId);
          removeAt(ix);
          ix--;
        
        }
    }
} // ValueIdList::removeSubqueryOrIsolatedUDFunctionPredicates()

void ValueIdList::unparse(NAString &result,
			  PhaseEnum phase,
			  UnparseFormatEnum form,
			  TableDesc * tabId) const
{
  //result += "(";
  for (CollIndex i = 0; i < entries(); i++)
    {
      if (i > 0) result += ", ";

      if ( form == ERROR_MSG_FORMAT ) // ERROR_MSG_FORMAT : don't print vid's
        {
          at(i).getItemExpr()->unparse(result,phase,EXPLAIN_FORMAT,tabId);
        }
      else
        {
	//never print vid's
       /*   char vidbuf[TEXT_DISPLAY_LENGTH];
          sprintf(vidbuf, "[%u]", (CollIndex)at(i));
          result += vidbuf;
	  */

          at(i).getItemExpr()->unparse(result,phase,form,tabId);
        }
    }
  //result += ")";


} // ValueIdList::unparse

// -----------------------------------------------------------------------
// ValueIdList::replaceOperandsOfInstantiateNull()
// This method is used by the code generator for replacing the
// operands of an ITM_INSTANTIATE_NULL with a value that belongs
// to availableValues.
// -----------------------------------------------------------------------
void ValueIdList::replaceOperandsOfInstantiateNull
                      (const ValueIdSet & availableValues,
                       const ValueIdSet & inputValues)
{
 for (CollIndex j = 0; j < entries(); j++)
    {
      at(j).getItemExpr()->replaceOperandsOfInstantiateNull(availableValues,inputValues);
    }
} // ValueIdList::replaceOperandsOfInstantiateNull()

void ValueIdList::print(FILE* ofd, const char* indent, const char* title,
                        CollHeap *c, char *buf) const
{
  BUMP_INDENT(indent);
  Space * space = (Space *)c;
  char mybuf[1000];

  snprintf(mybuf, sizeof(mybuf), "%s%s\n",NEW_INDENT,title);
  PRINTIT(ofd, c, space, buf, mybuf);

  for (CollIndex j = 0; j < entries(); j++)
  {
    Int32 i = (Int32)((CollIndex) (at(j))); // valueid as an integer
    NAString unparsed(CmpCommon::statementHeap());

    if (at(j).getItemExpr())
      at(j).getItemExpr()->unparse(unparsed); // expression as ascii string

    snprintf(mybuf, sizeof(mybuf), "%4d: %s\n",i,(const char *) unparsed);
    PRINTIT(ofd, c, space, buf, mybuf);
  }
} // ValueIdList::print()

void ValueIdList::display() const	// To be called from the debugger
{
  print();
}

// ***********************************************************************
// Methods for class ValueIdSet
// ***********************************************************************

THREAD_P ObjectCounter (*ValueIdSet::counter_)(0);

ValueIdSet::ValueIdSet() :
   ClusteredBitmap(CmpCommon::statementHeap())
{
  (*counter_).incrementCounter();
}

ValueIdSet::ValueIdSet(const ValueIdSet &other) :
   ClusteredBitmap(other, CmpCommon::statementHeap())
{
  (*counter_).incrementCounter();
}

ValueIdSet::ValueIdSet(const ValueId& vid) :
   ClusteredBitmap(CmpCommon::statementHeap())
{
  addElement(vid.id_);
  (*counter_).incrementCounter();
}

ValueIdSet::ValueIdSet(const ValueIdList &other) :
   ClusteredBitmap(CmpCommon::statementHeap())
{
  for (CollIndex i = 0; i < other.entries(); i++)
    {
      *this += other[i];
    }
  (*counter_).incrementCounter();
}

ValueIdSet::ValueIdSet(ValueDescArray *arr) :
  ClusteredBitmap(CmpCommon::statementHeap())
{
  (*counter_).incrementCounter();
}

NABoolean ValueIdSet::operator == (const ValueIdSet & other) const
{
  if (entries() != other.entries()) return FALSE;
  for (ValueId vid = init(); next(vid); advance(vid))
  {
    if (other.contains(vid) == FALSE)
      return FALSE;
  }
  return TRUE;
}

void ValueIdSet::getFirst(ValueId & v) const
{
  v = init();
  if (NOT next(v))
    v = NULL_VALUE_ID;
}
//------------------------------------------------------------------------
// ValueIdSet::convertToBaseids()
// Method that synthesizes a valueIdset that is comprised of the basecolumn
// valueids of this valueidset. So the ids in this set are expected to be
// for Columns.
//------------------------------------------------------------------------
ValueIdSet ValueIdSet::convertToBaseIds() const
{
  ValueIdSet result;
  for(ValueId vid=init(); next(vid); advance(vid))
  {
    // Since we can call this on any ValueIdSet, make sure we actually have
    // an IndexColumn.
    DCMPASSERT(vid.getItemExpr()->getOperatorType() == ITM_INDEXCOLUMN);
    result.insert(((BaseColumn *)(((IndexColumn *)vid.getItemExpr())->
			  getDefinition().getItemExpr()))->getValueId());
  }
  return result;
}

// --------------------------------------------------------------------
// ValueIdSet::getMinOrigUecOfJoiningCols
// Out of all columns in the set, return the one with minimum UEC
// --------------------------------------------------------------------

CostScalar ValueIdSet::getMinOrigUecOfJoiningCols()
{
  BaseColumn * result = NULL;
  CostScalar minUec = COSTSCALAR_MAX;

  for(ValueId vid= init(); next(vid); advance(vid))
  {
    // Find the base column for the input column:

    ItemExpr *inputColumnIEPtr = vid.getItemExpr();

    if (inputColumnIEPtr == NULL)
      continue;

    ValueId baseColumnForInputColumn = NULL_VALUE_ID;
    switch ( inputColumnIEPtr->getOperatorType() )
      {
      case ITM_VEG_REFERENCE:
	{
	  //  The inputColumn is a VEG reference
	  //  Loop through the columns until you find it:
	  ValueIdSet columnSet;

	  inputColumnIEPtr->findAll(ITM_BASECOLUMN, columnSet, TRUE, TRUE);
	  if (!columnSet.isEmpty() )
	    columnSet.getFirst(baseColumnForInputColumn);
	  break;
	}

	case ITM_INSTANTIATE_NULL:
	case ITM_BASECOLUMN:
	  //  The inputColumn is a base column.
	  baseColumnForInputColumn = vid;
	  break;

	case ITM_INDEXCOLUMN:
	  //  Get the base column for the index column:
	  {
	    const BaseColumn *bcIEPtrForIndexColumn =
	      (BaseColumn *) ((IndexColumn *) inputColumnIEPtr)->
	      getDefinition().getItemExpr();

	    baseColumnForInputColumn = bcIEPtrForIndexColumn->getValueId();
	    break;
	  }

	default:
	  // The value id in inputColumns *must* be either an index column or
	  // a base table column; anything else, when we reach here, is an
	  // internal error.
	  break;
	} // switch on type of input column

    if (baseColumnForInputColumn != NULL_VALUE_ID)
      result = (BaseColumn *)(vid.getItemExpr() );

    if (result)
    {
      ColStatDescList & colStatsList = result->getTableDesc()->tableColStats();

      CollIndex index;
      NABoolean found = colStatsList.getColStatDescIndexForColumn( index, baseColumnForInputColumn );

      if (found)
      {
	CostScalar uec = colStatsList[index]->getColStats()->getTotalUec();
	if (uec < minUec)
	{
	  minUec = uec;
	}
      }
    }
  }

  return minUec;
}

NABoolean ValueIdSet::coversFirstN(const ValueIdList &other, Int32 N) const
{
  Int32 i, limit = N <= (Int32)other.entries() ? N : (Int32)other.entries();
  for (i = 0; i < limit; i++) {
    // if other is an indexcolumn, compare against its definition,
    // ie, compare against what column it indexes
    if (other[i].getItemExpr()->getOperatorType() == ITM_INDEXCOLUMN) {
      IndexColumn *ixCol = (IndexColumn*)(other[i].getItemExpr());
      if (!contains(ixCol->getDefinition())) { return FALSE; }
    }
    else if (!contains(other[i])) { return FALSE; }
  }
  return TRUE; // this covers first N elements of other
}

// how many prefix columns of other are found in this predicate?
Int32 ValueIdSet::prefixMatchesOf(const ValueIdList &other) const
{
  Int32 result = 0; // assume no prefix match until proven otherwise
  ValueId vId;

  for (CollIndex i = 0; i <  other.entries(); i++)
    {
      if (referencesTheGivenValue(other[i], vId))
        result++;
      else
	return result; // quit on first prefix mismatch
    }
  // all elements of other are found in this
  return result;
}

NABoolean ValueIdSet::isDensePrefix(const ValueIdList &other) const
{
  ValueIdSet notContainedInThis(*this);
  NABoolean found = TRUE;

  for (CollIndex i = 0; i <  other.entries() AND found; i++)
    {
      if (contains(other[i]))
	notContainedInThis -= other[i];
      else
	found = FALSE;
    }

  // if all value ids of this set are contained in the prefix, return TRUE
  return notContainedInThis.isEmpty();
}

Lng32 ValueIdSet::getRowLength() const
{
  Lng32 result = 0;

  for (ValueId x = init(); next(x); advance(x))
    {
      result += x.getType().getTotalSize();
    }

  return result;
}

Lng32 ValueIdSet::getRowLengthOfNumericCols() const
{
  Lng32 result = 0;

  for (ValueId x = init(); next(x); advance(x))
    {
      if (x.getType().isNumeric())
      	result += x.getType().getTotalSize();
    }

  return result;
}


// ***********************************************************************
// normalizer-related methods for class ValueIdSet
// ***********************************************************************

// -----------------------------------------------------------------------
// ValueIdSet::accumulateReferencedValues()
//
// This method accumulates those values that are members of the
// referencedSet and are referenced either individually or in a
// VEGReference in the referencingSet, if the optional parameters
// allow it. The original contents of this ValueIdSet are augmented.
// -----------------------------------------------------------------------
void ValueIdSet::accumulateReferencedValues(const ValueIdSet & referencedSet,
                                  const ValueIdSet & referencingSet,
                                  NABoolean doNotDigInsideVegRefs,
                                  NABoolean doNotDigInsideInstNulls)
{
  for (ValueId referencedExpr = referencedSet.init();
       referencedSet.next(referencedExpr);
       referencedSet.advance(referencedExpr))
    {
      for (ValueId referencingExpr = referencingSet.init();
	   referencingSet.next(referencingExpr);
	   referencingSet.advance(referencingExpr))
	{
          if (referencingExpr.getItemExpr()
               ->referencesTheGivenValue(
                   referencedExpr,
                   doNotDigInsideVegRefs,
                   doNotDigInsideInstNulls))
	    addElement(referencedExpr);
	} // referencingExpr loop
    } // referencedExpr loop

} // ValueIdSet::accumulateReferencedValues()

// -----------------------------------------------------------------------
// Walk through an ItemExpr tree and gather the ValueIds of those
// expressions that behave as if they are "leaves" for the sake of
// the coverage test, e.g., expressions that have no children, or
// aggregate functions, or instantiate null. These are usually values
// that are produced in one "scope" and referenced above that "scope"
// in the dataflow tree for the query.
// -----------------------------------------------------------------------
void ValueIdSet::getLeafValuesForCoverTest(ValueIdSet & leafValueIds, 
                                           const GroupAttributes& coveringGA,
                                           const ValueIdSet & newExternalInputs) const
{
  for (ValueId exprId = init(); next(exprId); advance(exprId) )
    {
      exprId.getItemExpr()->getLeafValuesForCoverTest(leafValueIds, coveringGA, 
	newExternalInputs);
    }
} // ValueIdSet::getLeafValuesForCoverTest()

// -----------------------------------------------------------------------
// ValueIdSet::referencesOneValueFromTheSet()
//
// Check whether an expression contained in this ValueIdSet
// references an expression that is a member of the otherSet.
// -----------------------------------------------------------------------
NABoolean ValueIdSet::referencesOneValueFromTheSet
                         (const ValueIdSet & otherSet) const
{
  for (ValueId myExpr = init(); next(myExpr); advance(myExpr))
    {
      for (ValueId otherExpr = otherSet.init();
	   otherSet.next(otherExpr);
	   otherSet.advance(otherExpr))
	{
	  if (myExpr.getItemExpr()->referencesTheGivenValue(otherExpr))
	    return TRUE;
	}
    }
  return FALSE;
} // ValueIdSet::referencesOneValueFromTheSet()

// -----------------------------------------------------------------------
// ValueIdSet::referencesAllValuesFromMe()
//
// Check whether all expressions contained in this ValueIdSet
// references an expression that is a member of the otherSet.
// -----------------------------------------------------------------------
NABoolean ValueIdSet::referencesAllValuesFromMe
		      (const ValueIdSet & otherSet) const
{
   ValueId id;
   for (ValueId myExpr = init(); next(myExpr); advance(myExpr))
    {
      if (NOT(otherSet.referencesTheGivenValue(myExpr, id)))
	return FALSE;
    }
  return TRUE;
} // ValueIdSet::referencesAllValuesFromMe()

// -----------------------------------------------------------------------
// ValueIdSet::referencesTheGivenValue()
//
// Check whether any expression contained in this ValueIdSet
// references the given value.
// -----------------------------------------------------------------------
NABoolean ValueIdSet::referencesTheGivenValue(const ValueId & theValue,
                                    ValueId & exprId,
                                    NABoolean doNotDigInsideVegRefs,
                                    NABoolean doNotDigInsideInstNulls) const
{
  for (ValueId vid = init(); next(vid); advance(vid))
    {
      if (vid.getItemExpr()->referencesTheGivenValue(
                               theValue,
                               doNotDigInsideVegRefs,
                               doNotDigInsideInstNulls))
      {
	exprId = vid;
	return TRUE;
      }
    }
  return FALSE;
} // ValueIdSet::referencesTheGivenValue()


// -----------------------------------------------------------------------
// ValueIdSet::containsTheGivenValue()
//
// Check whether any expression contained in this ValueIdSet
// references the given value.
// -----------------------------------------------------------------------
NABoolean ValueIdSet::containsTheGivenValue( const ValueId & theValue ) const
{
  for ( ValueId vid = init(); next(vid); advance(vid) )
  {
    if ( vid.getItemExpr()->containsTheGivenValue( theValue ) )
    {
      return TRUE;
    }
  }

  return FALSE;
} // ValueIdSet::containsTheGivenValue()

// return true iff set has RandomNum expr
NABoolean ValueIdSet::hasRandom() const
{
  for ( ValueId vid = init(); next(vid); advance(vid) )
  {
    if ( vid.getItemExpr()->getOperatorType() == ITM_RANDOMNUM )
    {
      return TRUE;
    }
  }
  return FALSE;
}

// -----------------------------------------------------------------------
// ValueIdSet::membersCoveredInSet()
//
// Check whether any of the members of this ValueIdSet are contained
// in the provided ValueIdSet.  Return the number of members found.
// -----------------------------------------------------------------------
Int32 ValueIdSet::membersCoveredInSet (const ValueIdSet& vidSet, NABoolean lookBelowInstantiateNull) const
{
  NABoolean coverFlag = FALSE;
  ItemExpr * memberExpr = NULL;
  ValueId memberValId;

  Int32 membersFound = 0;

  for ( ValueId vid = init(); next(vid); advance(vid) )
  {
    // If any member of this set contains the INVERSE function (for
    // specifying DESC ordering), examine the valueid of the child of
    // the inverse expression for coverage.
    memberExpr = vid.getItemExpr();
    if ((memberExpr->getOperatorType() == ITM_INVERSE) ||
	(lookBelowInstantiateNull && memberExpr->getOperatorType() == ITM_INSTANTIATE_NULL))
      memberValId = ((ItemExpr *)memberExpr->child(0))->getValueId();
    else
      memberValId = vid;

    if ( vidSet.contains( memberValId ) )
      membersFound++;
  }

  return (membersFound);
}

//-------------------------------------------------------
//removeCoveredVidSet()
//Input: other set
//output: reduced *this set by other set
//Constraints: will remove the ids as long as
//other set contains a reference or the id itself.
//---------------------------------------------------------
void ValueIdSet::removeCoveredVIdSet(const ValueIdSet & otherSet)
{
  for ( ValueId id = otherSet.init(); otherSet.next(id); otherSet.advance(id) )
  {
    if ( id.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE )
    {
      const ValueIdSet & insideSet =
	((VEGReference *)(id.getItemExpr()))->getVEG()->getAllValues();
      remove( insideSet );
    }
    else
    {
      remove( id );
    }
  }
}

// -----------------------------------------------------------------------
// ValueIdSet::removeUnReferencedVEGPreds()
//
// Check whether a VEGPred contained in this ValueIdSet references
// references a value that is a member of the otherSet and remove it
// othewise.
// -----------------------------------------------------------------------
void ValueIdSet::removeUnReferencedVEGPreds
                         (const ValueIdSet & otherSet)
{
  for (ValueId myExpr = init(); next(myExpr); advance(myExpr))
    {
      if (myExpr.getItemExpr()->getOperatorType() == ITM_VEG_PREDICATE)
        {
          ValueIdSet predSet;
          predSet += myExpr;
          // If any of the vegMembers is provided by the available values
          // we want to keep the predicate
          if(NOT predSet.referencesOneValueFromTheSet(otherSet))
	    subtractElement(myExpr); // delete original expression from set
        }
    }
} // ValueIdSet::removeUnReferencedVEGPreds()

// -----------------------------------------------------------------------
// ValueIdSet::removeSubqueryOrIsolatedUDFunctionPredicates()
//
// This method walks through the ItemExprs that belong to this set,
// collects the ValueIds of the expressions at their leaves and
// returns them.
// -----------------------------------------------------------------------
void ValueIdSet::removeSubqueryOrIsolatedUDFunctionPredicates(ValueIdSet & subqueryExpr)
{
  for (ValueId exprId = init(); next(exprId); advance(exprId))
    {
      if (exprId.getItemExpr()->containsSubquery() ||
         (exprId.getItemExpr()->containsIsolatedUDFunction()))
        {
          subqueryExpr.addElement(exprId);
	  subtractElement(exprId); // delete original expression from set
        }
    }
} // ValueIdSet::removeSubqueryOrIsolatedUDFunctionPredicates()

// -----------------------------------------------------------------------
// ValueIdSet::transformNode()
//
// Method for applying transformNode() to a ValueIdSet.
// If the ValueIdSet represents a predicate tree, then the caller is
// expected to set movePredicate to TRUE. Whenever movePredicate is
// is set and a subquery is transformed, the transformed predicate
// is moved to the SemiJoin or Join that is introduced by the
// subquery transformation.
// -----------------------------------------------------------------------
NABoolean ValueIdSet::transformNode(NormWA & normWARef,
                                    ExprGroupId & introduceSemiJoinHere,
                                    const ValueIdSet & externalInputs,
				    const NABoolean movePredicates,
				    const NABoolean postJoinPredicates)
{
  // ---------------------------------------------------------------------
  // Save the contents of introduceSemiJoinHere in order to be able to
  // detect whether a subquery transformation was performed while
  // processing one of the elements of this set.
  // ---------------------------------------------------------------------
  RelExpr * saveOrigPtr = introduceSemiJoinHere;
  // ---------------------------------------------------------------------
  // newJoin is allocated for referencing a SemiJoin or a Join that is
  // introduced by a subquery transformation.
  // Set its initial value to be the contents of introduceSemiJoinHere.
  // ---------------------------------------------------------------------
  RelExpr * newJoin = (RelExpr *) introduceSemiJoinHere.getPtr();
  // ---------------------------------------------------------------------
  // newExpr will contain the ValueIds of new expressions that are
  // introduced by the normalizing transformations that are performed
  // during the transformation phase of query normalization.
  // ---------------------------------------------------------------------
  ValueIdSet newExpr;
  // ---------------------------------------------------------------------
  // Iterate over all the values (scalar expressions) in this set.
  // ---------------------------------------------------------------------
  for (ValueId exprId = init(); next(exprId); advance(exprId))
    {
      ItemExpr *iePtr = exprId.getItemExpr();  // original expr.
      ExprValueId nePtr(iePtr);                // normalized expr.
      // -----------------------------------------------------------------
      // Transform the Item Expression.
      // -----------------------------------------------------------------
      iePtr->getReplacementExpr()->transformNode(normWARef, nePtr,
      						 introduceSemiJoinHere,
			   			 externalInputs);
      // -----------------------------------------------------------------
      // If the original expression was transformed ..
      // -----------------------------------------------------------------
      if (nePtr != (const ItemExpr *)iePtr)
	{
	  // -------------------------------------------------------------
	  // Delete the original expression from this set.
	  // -------------------------------------------------------------
	  subtractElement(exprId);
	  // -------------------------------------------------------------
	  // If the original expression was not eliminated by transformation,
          // then if its replacement is an unconditional True, eliminate it now
	  // otherwise, accumulate the replacement in the set newExpr.
	  // -------------------------------------------------------------
	  if (nePtr.getPtr())
	  {
	    if (nePtr.getPtr()->getOperatorType() == ITM_RETURN_TRUE &&
		nePtr.getPtr()->getArity() == 0)
	      nePtr = NULL;
	    else
        {
              // Make sure selectivity hint information is carried over correctly.
              if(iePtr->isSelectivitySetUsingHint())
              {
                nePtr.getPtr()->setSelectivitySetUsingHint();
                nePtr.getPtr()->setSelectivityFactor(iePtr->getSelectivityFactor());
              }

                // See if UDF_SUBQ_IN_AGGS_AND_GBYS is enabled. It is
                // enabled if the default is ON, or if the default is
                // SYSTEM and ALLOW_UDF is ON.
                NABoolean udfSubqInAggGrby_Enabled = FALSE;
                DefaultToken udfSubqTok =
                  CmpCommon::getDefault(UDF_SUBQ_IN_AGGS_AND_GBYS);
                if ((udfSubqTok == DF_ON) ||
                    (udfSubqTok == DF_SYSTEM))
                  udfSubqInAggGrby_Enabled = TRUE;

                if (!udfSubqInAggGrby_Enabled)
                  nePtr->convertToValueIdSet(newExpr);
                else
                  // Since we can have UDFs or Subqueries in the group by
                  // we now can get an ITM_ITEM_LIST back from a transformation
                  // of a MVF or multi-degree subqueries. So we have to 
                  // flatten any such itemExpr 
                  nePtr->convertToValueIdSet(newExpr, NULL, 
                                             ITM_AND, TRUE, TRUE/*flattenlist*/);
        }
       }
      }
      // -----------------------------------------------------------------
      // If a subquery transformation has introduced a new Join ...
      // -----------------------------------------------------------------
      if (introduceSemiJoinHere.getPtr() != newJoin)
        {
	  // -------------------------------------------------------------
	  // Get addressability to the new Join node.
	  // -------------------------------------------------------------
          newJoin = (RelExpr *) introduceSemiJoinHere.getPtr();
	  // -------------------------------------------------------------
          // Note that it is possible to encounter a subquery in each
	  // scalar expression that is processed per iteration of the loop.
          // Hence, introduceSemiJoinHere can contain a different
          // SemiJoin or a Join per iteration of the loop. Therefore, the
	  // movement of predicates must be done inside the loop.
	  // -------------------------------------------------------------
          if (movePredicates)
            {               // this ValueIdSet represents a predicate tree
	      // ---------------------------------------------------------
	      // If the subquery predicate is buried in a complex
	      // expression, such as T1.x = 10 OR EXISTS ( ... ), then
	      // the predicate that replaces the subquery cannot be moved
	      // independently. The entire expression in which it is
	      // contained must be moved. Delete the original expression
	      //  from this set.
	      // ---------------------------------------------------------
              subtractElement(exprId);

              if (nePtr.getPtr())
                {                    // if the subquery was not eliminated
		  // -----------------------------------------------------
                  // Remove the expression that replaces the subquery
                  // predicate (*nePtr) from newExpr because it will be
		  // evaluated as a member of another expression tree.
		  // -----------------------------------------------------
                  newExpr -= nePtr->getValueId();
		  // -----------------------------------------------------
                  // Add it to newJoin (the SemiJoin or Join) node that
                  // was introduced by the subquery transformation.
		  // -----------------------------------------------------
		  ValueIdSet transformedExpr;
		  nePtr->convertToValueIdSet(transformedExpr,
					     NULL, ITM_AND, FALSE);

                  //--------------------------------------------------------
                  // Generator does not know how to transmit
                  // ONE_ROW of columns yet from a child to its parent. So do not
                  // ask for it. One Row check is already performed at the
                  // concerned GroupBy node.
                  //--------------------------------------------------------

                  transformedExpr.replaceOneRowbyList( normWARef );

                  if (newJoin->getOperator().match(REL_ANY_SEMIJOIN))
                    ((Join *)newJoin)->joinPred() += transformedExpr;	  // ON clause
                  else
                    newJoin->selectionPred() += transformedExpr;
		  // -----------------------------------------------------
		  // Compute the additional set of external inputs that
		  // should be supplied to newJoin so that the expression
		  // that replaces the subquery predicate (*nePtr) can
		  // be evaluated.
		  // -----------------------------------------------------
                  ValueIdSet neededValues(externalInputs);
                  ValueIdSet outputValues;
                  newJoin->getPotentialOutputValues(outputValues);
                  neededValues -= outputValues;
		  // -----------------------------------------------------
                  // neededValues contains the external inputs. Remove
                  // all those values from it that are not referenced
		  // by the expression that replaces the subquery
		  // predicate (*nePtr).
		  // -----------------------------------------------------
                  transformedExpr.weedOutUnreferenced(neededValues);
                  newJoin->getGroupAttr()->addCharacteristicInputs(neededValues);
                }                    // if the subquery was not eliminated
            }               // this ValueIdSet represents a predicate tree
          else
            {
	      // ---------------------------------------------------------
              // If we are not moving predicates, newJoin should not be
              // a SemiJoin. This is because a SemiJoin can only be
	      // introduced by the transformation of a subquery predicate.
	      // ---------------------------------------------------------
              CMPASSERT(NOT newJoin->getOperator().match(REL_ANY_SEMIJOIN));
            }
        }                      // if (introduceSemiJoinHere != newJoin)
      // -----------------------------------------------------------------
      // If the given predicates are post-join predicates, i.e., they
      // are evaluated after the join is performed, check whether a
      // left join can be converted to an inner join by the transformed
      // predicate. The check initiates the transformation of a left
      // join to an inner join as a side effect.
      // -----------------------------------------------------------------
      if (postJoinPredicates)
	{
	ValueIdSet outerReferences;
	newJoin->getGroupAttr()->getCharacteristicInputs().
					getOuterReferences(outerReferences);
	if (nePtr != (const ItemExpr *)iePtr)
	  {
	    if (nePtr.getPtr()) // transformation hasn't eliminated it
	      nePtr->predicateEliminatesNullAugmentedRows(normWARef, outerReferences);
	  }
	else
	  iePtr->predicateEliminatesNullAugmentedRows(normWARef, outerReferences);
	}
      // ---------------------------------------------------------------------
      // try conditionally eliminate LJ even for join preds.  The code
      // in the else clause pretty much resembles the code in predicateElim-
      // inateNullAugmentRows(). The only difference is due to the nature of
      // join predicate, we need to check for whether the instantiate null
      // value involved comes from the right before transforming the associated
      // left join into an inner join. The characteristics outputs from the
      // right child is needed to determine this. Ideally, this additional
      // parameter could be passed in predicateEliminateNullAugmentedRows()
      // so that the condition could be determined there. However, for now,
      // I don't have time to recompile a bunch of stuffs after changing a
      // header file. So,... promise to move this code into there after FCS.
      // ---------------------------------------------------------------------
      else
      {
        if (saveOrigPtr)
        {
          const ValueIdSet & outputs = saveOrigPtr->getGroupAttr()->
                                       getCharacteristicOutputs();
          ItemExpr *ie;
          if (nePtr != (const ItemExpr *)iePtr)
          {
            if (nePtr.getPtr()) // transformation hasn't eliminated it
              ie = nePtr.getPtr();
            else
              ie = NULL;
          }
          else ie = iePtr;

          if (ie)
          {
            // emulate predicateEliminatesNullAugmentedRows() in BiRelat
            //

            NABoolean isFOJ = FALSE;
            if (normWARef.getCurrentOwnerExpr())
              isFOJ = (normWARef.getCurrentOwnerExpr()
                        ->getOperatorType() == REL_FULL_JOIN);

            if ( (NOT normWARef.walkingAnExprTree()) AND     
                 (NOT isFOJ) AND
                 (ie->getOperatorType() == ITM_EQUAL OR
                  ie->getOperatorType() == ITM_NOT_EQUAL OR
                  ie->getOperatorType() == ITM_LESS OR
                  ie->getOperatorType() == ITM_LESS_EQ OR
                  ie->getOperatorType() == ITM_GREATER OR
                  ie->getOperatorType() == ITM_GREATER_EQ) )
            {
              if (ie->child(0)->getOperatorType() == ITM_INSTANTIATE_NULL)
              {
                InstantiateNull *inst =
                       (InstantiateNull *)(ie->child(0)->castToItemExpr());
                if (NOT inst->NoCheckforLeftToInnerJoin)
                {
                  // Only for values coming from right hand side.
                  //
                  if (outputs.contains(ie->child(0)->getValueId()))
                  {
                    ie->child(0) = ie->child(0)->
                          initiateLeftToInnerJoinTransformation(normWARef);
                  }
                }
              }
              if (ie->child(1)->getOperatorType() == ITM_INSTANTIATE_NULL)
              {
                InstantiateNull *inst =
                       (InstantiateNull *)(ie->child(1)->castToItemExpr());
                if (NOT inst->NoCheckforLeftToInnerJoin)
                {
                  // Only for values coming from right hand side.
                  //
                  if (outputs.contains(ie->child(1)->getValueId()))
                  {
                    ie->child(1) = ie->child(1)->
                          initiateLeftToInnerJoinTransformation(normWARef);
                  }
                }
              }
            } // end of emulation of predicateEliminatesNullAugmentedRows()
          } // if(ie)
        } // endif(saveOrigPtr)
      }   // endif(postJoinPredicates)

    } // loop over given expressions

  addSet(newExpr); // add transformed expressions to the given set

  // Indicate whether a subquery was transformed to the caller.
  return (introduceSemiJoinHere != (const RelExpr *)saveOrigPtr);

} // ValueIdSet::transformNode()

// -----------------------------------------------------------------------
// Method for applying normalizeNode() to a ValueIdSet.
// -----------------------------------------------------------------------
NABoolean ValueIdSet::normalizeNode(NormWA & normWARef)
{
  NABoolean transformed = FALSE;
  ValueIdSet newExpr;
  ItemExpr * nePtr;                      // normalized expr.
  for (ValueId exprId = init(); next(exprId); advance(exprId) )
    {
      ItemExpr *iePtr = exprId.getItemExpr();  // original expr.
      // Transform the Item Expression.
      nePtr = iePtr->getReplacementExpr()->normalizeNode(normWARef);
      // If the original expression was transformed, remember transformed ids
      if ( nePtr != iePtr ||
           nePtr->getValueId() != exprId )
	{
	  transformed = TRUE;

	  // Need to set selectivity correctly when JOIN predicate (t1.a = t2.a) is eliminated:
	  // select * from t1, t2 where t1.a = t2.a <<+ selectivity 10e-3 >> and t1.a = 3;
	  if(iePtr->isSelectivitySetUsingHint() &&
	     nePtr->getOperatorType() == ITM_VEG_PREDICATE )
	  {
	    VEG * predVEG = ((VEGPredicate *)nePtr)->getVEG();
	    const ValueIdSet & VEGGroup = predVEG->getAllValues();
	    ItemExpr *constant = NULL;
	    if(VEGGroup.referencesAConstValue(&constant) ||
	       VEGGroup.referencesAConstExpr(&constant))
	    {
              ValueIdSet cols;
	      nePtr->findAll(ITM_BASECOLUMN, cols, TRUE, TRUE);

	      CollIndex tblCount = cols.getAllTables()->entries();
	      if(tblCount > 0)
	      {
		// The following line is to make sure selectivity pushed down to Scan will result
		// in cumulative selectivity equal to that specifed originally by the user.In the 
		// example above, the selectivity pushed down will be pow(sel, 1/2) = sqrt(0.01) = 0.1.
		double selFactor = pow(iePtr->getSelectivityFactor(), ((double)1/tblCount));
		nePtr->setSelectivitySetUsingHint();
		nePtr->setSelectivityFactor(selFactor);
	      }
	    }
	  }	    
	  subtractElement(exprId); // delete original expression from set
	  // accumulate new expressions in another set
	  if (nePtr)
	    nePtr->convertToValueIdSet(newExpr);
	}
    } // loop over given expressions

  addSet(newExpr); // add transformed expressions to the given set

  return transformed;
} // ValueIdSet::normalizeNode

// -----------------------------------------------------------------------
// lookForVEGReferences()
// Accumulate ValueIds of VEGReferences, if any, in vs.
// -----------------------------------------------------------------------
void ValueIdSet::lookForVEGReferences(ValueIdSet & VEGRefSet) const
{
  for (ValueId vid = init(); next(vid); advance(vid))
    {
      if (vid.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE)
        VEGRefSet += vid;
    }
} // ValueIdSet::lookForVEGReferences

// *this is a set of (join) equality predicates.
// grcols is a set of group by columns.
// if c is in grcols and c is an opd of an equality predicate then
// add the equality's other opd to grcols.
void ValueIdSet::introduceMissingVEGRefs(ValueIdSet & grcols) const
{
  // iterate over the equality predicates
  for (ValueId vid = init(); next(vid); advance(vid))
    {
      if (vid.getItemExpr()->getOperatorType() == ITM_EQUAL)
        { 
          BiRelat *eq = (BiRelat*)vid.getItemExpr();
          ValueId v0, v1;
          if (eq->child(0)->getOperatorType() == ITM_VEG_REFERENCE)
            v0 = eq->child(0)->getValueId();
          if (eq->child(1)->getOperatorType() == ITM_VEG_REFERENCE)
            v1 = eq->child(1)->getValueId();
          // if left child is in grcols then add right child to grcols
          if (grcols.containsTheGivenValue(v0) AND v1 != NULL_VALUE_ID)
            {
              grcols += v1;
            }
          // if right child is in grcols then add left child to grcols
          else if (grcols.containsTheGivenValue(v1) AND v0 != NULL_VALUE_ID)
            {
              grcols += v0;
            }
        }
    }
} 

// -----------------------------------------------------------------------
// lookForVEGPredicates()
// Accumulate ValueIds of VEGPredicates, if any, in vs.
// -----------------------------------------------------------------------
void ValueIdSet::lookForVEGPredicates(ValueIdSet & VEGPredSet) const
{
  for (ValueId vid = init(); next(vid); advance(vid))
    {
      if (vid.getItemExpr()->getOperatorType() == ITM_VEG_PREDICATE)
        VEGPredSet += vid;
    }
} // ValueIdSet::lookForVEGPredicates

// return TRUE iff this has no equality predicates at all
// put in joinCols the simple base columns being equijoined
// given "a-1=b and c=d+1", joinCols should get "b,c"
NABoolean ValueIdSet::hasNoEquiPredicates(ValueIdSet& joinCols) const
{
  NABoolean result = TRUE; // until proven otherwise
  if (entries() <= 0)
    return result; // has no predicates at all

  for (ValueId vid = init(); next(vid); advance(vid))
    {
      ItemExpr *expr = vid.getItemExpr();
      switch (expr->getOperatorType()) 
        { 
        case ITM_VEG_PREDICATE:
          // given equi-predicate "a=b", add "a,b" to joinCols
          expr->findAll(ITM_BASECOLUMN, joinCols, TRUE, TRUE);
          result = FALSE; // has at least one equality predicate
          break;
        case ITM_EQUAL:
          // given equi-predicate "a=b+1", add "a" to joinCols
          if (expr->child(0)->getOperatorType() == ITM_VEG_REFERENCE)
            expr->child(0)->findAll(ITM_BASECOLUMN, joinCols, TRUE, TRUE);
          // given equi-predicate "a-1=b", add "b" to joinCols
          if (expr->child(1)->getOperatorType() == ITM_VEG_REFERENCE)
            expr->child(1)->findAll(ITM_BASECOLUMN, joinCols, TRUE, TRUE);
          result = FALSE; // has at least one equality predicate
          break;
        default:
          continue; // keep looking
        }
    }
  return result;
} // ValueIdSet::isAllEquiPredicates

NABoolean ValueIdSet::containsAsEquiLocalPred(ValueId x) const
{
  NABoolean result = FALSE; // until proven otherwise
  if (entries() <= 0)
    return result; // has no predicates at all

  ItemExpr* constExpr = NULL;

  NABoolean found = FALSE;

  for (ValueId vid = init(); next(vid); advance(vid))
    {
      ItemExpr *expr = vid.getItemExpr();
      switch (expr->getOperatorType())
        {
        case ITM_VEG_PREDICATE:
          {
           ValueIdSet vegMembers =
               ((VEGPredicate*)expr)->getVEG()->getAllValues();

           return (vegMembers.contains(x) &&
                   vegMembers.referencesAConstExpr(&constExpr));
          }
          break;

        case ITM_EQUAL:

          // consider equi-predicate "a=1", or "1=a" 
          for (CollIndex i=0; i<2; i++ ) {
             if ( expr->child(i)->getOperatorType() == ITM_VEG_REFERENCE ) 
             {
               ItemExpr* childExpr = expr->child(i);
               found = ((VEGReference*)(childExpr))->getVEG()
                                   ->getAllValues().contains(x);
   
   
             } else
                 found = (expr->child(i) == x);
   
             if (found) {
   
               CollIndex j = (i==0) ? 1 : 0;
               ValueIdSet vset(expr->child(j));
               if ( vset.entries() == 1 &&
                    vset.referencesAConstExpr(&constExpr) == TRUE )
                  return TRUE;

             }
          }

          break;

        default:
          return FALSE;
        }
    }
  return result;

} // ValueIdSet::containsAsEquiPred()

NABoolean ValueIdSet::containsAsRangeLocalPred(ValueId x) const
{
  NABoolean result = FALSE; // until proven otherwise
  if (entries() <= 0)
    return result; // has no predicates at all
  ItemExpr* constExpr = NULL;
  NABoolean found = FALSE;

  for (ValueId vid = init(); next(vid); advance(vid))
  {
    ItemExpr *expr = vid.getItemExpr();
    switch (expr->getOperatorType())
    {      
    case ITM_LESS:
    case ITM_LESS_EQ:
    case ITM_LESS_OR_LE:
    case ITM_GREATER:
    case ITM_GREATER_EQ:
    case ITM_GREATER_OR_GE:
      
      // consider range-predicate "a<1", "1>a", "a>1" or "1<xa" 
      for (CollIndex i=0; i<2; i++ ) {
        if ( expr->child(i)->getOperatorType() == ITM_VEG_REFERENCE ) 
        {
          ItemExpr* childExpr = expr->child(i);
          found = ((VEGReference*)(childExpr))->getVEG()
            ->getAllValues().contains(x);
        } else
          found = (expr->child(i) == x);
        
        if (found) {
          CollIndex j = (i==0) ? 1 : 0;
          ValueIdSet vset(expr->child(j));
          if ( vset.entries() == 1 &&
               vset.referencesAConstExpr(&constExpr) == TRUE )
            return TRUE;
        }
      }      
      break;
      
    default:
      return FALSE;
    }
  }
  return result;

} // ValueIdSet::containsAsRangeLocalPred()


void ValueIdSet::insertList(const ValueIdList &other)
{
  for (CollIndex i = 0; i < other.entries(); i++)
    insert(other[i]);
}

// -----------------------------------------------------------------------
// isCovered()
// Returns TRUE only if ALL the members of the set are covered.
// -----------------------------------------------------------------------
NABoolean ValueIdSet::isCovered(const ValueIdSet & newExternalInputs,
		                const GroupAttributes & coveringGA,
		                ValueIdSet & referencedInputs,
		                ValueIdSet & coveredSubExpr,
				ValueIdSet & unCoveredExpr) const
{
  NABoolean coverFlag, exprIsCovered = TRUE;

  for (ValueId exprId = init(); next(exprId); advance(exprId))
    {
      // Perform the coverage test for each ItemExpr.
      // This set is covered if EVERY ItemExpr that belongs to it isCovered.
      coverFlag = coveringGA.covers(exprId,
				    newExternalInputs,
				    referencedInputs,
				    &coveredSubExpr,
				    &unCoveredExpr);
      if (coverFlag)
	coveredSubExpr += exprId;
      exprIsCovered &= coverFlag;
    } // for

  return exprIsCovered;

} // ValueIdSet::isCovered()

// ------------------------------------------------------------------------
// ValueIdSet::removeCoveredExprs()
//
// Removes from the valueid set those expressions that are
// covered by the available values
// ------------------------------------------------------------------------

Int32 ValueIdSet::removeCoveredExprs(const ValueIdSet & newExternalInputs, 
                                     ValueIdSet* usedInputs)
{
  Int32 result = 0;
  NABoolean coverFlag;
  ValueIdSet referencedInputs;
  GroupAttributes emptyGA;

  emptyGA.setCharacteristicOutputs(newExternalInputs);
  for (ValueId exprId = init(); next(exprId); advance(exprId))
    {
      if (exprId.getItemExpr()->getOperatorType() == ITM_VEG_PREDICATE)
        {
          VEG * vegPtr = ((VEGPredicate *)(exprId.getItemExpr()))->getVEG();
          coverFlag = emptyGA.covers(vegPtr->getVEGReference()->getValueId(),
                                     newExternalInputs,
                                     referencedInputs);
        }
      else
        coverFlag = emptyGA.covers(exprId,
                                   newExternalInputs,
                                   referencedInputs);
      if (coverFlag) {
	subtractElement(exprId);
        result++;
        if (usedInputs)
          *usedInputs += referencedInputs ;
      }
    } // for

  return result;
} // ValueIdSet::removeCoveredExprs

// ------------------------------------------------------------------------
// ValueIdSet::removeUnCoveredExprs()
//
// Removes from the valueid set those expressions that are
// NOT covered by the available values
// ------------------------------------------------------------------------

Int32 ValueIdSet::removeUnCoveredExprs(const ValueIdSet & newExternalInputs)
{
  Int32 result = 0;
  NABoolean coverFlag;
  ValueIdSet referencedInputs;
  GroupAttributes emptyGA;

  emptyGA.setCharacteristicOutputs(newExternalInputs);
  for (ValueId exprId = init(); next(exprId); advance(exprId))
    {
      if (exprId.getItemExpr()->getOperatorType() == ITM_VEG_PREDICATE)
        {
          VEG * vegPtr = ((VEGPredicate *)(exprId.getItemExpr()))->getVEG();
          coverFlag = emptyGA.covers(vegPtr->getVEGReference()->getValueId(),
                                     newExternalInputs,
                                     referencedInputs);
        }
      else
        coverFlag = emptyGA.covers(exprId,
                                   newExternalInputs,
                                   referencedInputs);
      if (!coverFlag)
        {
          subtractElement(exprId);
          result++;
        }
    } // for

  return result;
} // ValueIdSet::removeUnCoveredExprs

// ---------------------------------------------------------------------
// simplifyOrderExpr()
//
//  Returns a version of the set that has all expressions involving
// columns and constants or columns and inverse nodes simplified to be
// the column only.
// ---------------------------------------------------------------------
ValueIdSet ValueIdSet::simplifyOrderExpr() const
{
  ValueIdSet simpleVidSet;
  for (ValueId vid = init();
                     next(vid);
                     advance(vid))
  {
    simpleVidSet +=
      vid.getItemExpr()->simplifyOrderExpr()->getValueId();
  }

  return simpleVidSet;
}

// ---------------------------------------------------------------------
// removeInverseOrder()
//
//  Returns a version of the set that has all expressions involving
// columns and inverse nodes simplified to be the column only.
// ---------------------------------------------------------------------
ValueIdSet ValueIdSet::removeInverseOrder() const
{
  ValueIdSet noInverseVidSet;
  for (ValueId vid = init();
                     next(vid);
                     advance(vid))
  {
    noInverseVidSet +=
      vid.getItemExpr()->removeInverseOrder()->getValueId();
  }

  return noInverseVidSet;
}

// ---------------------------------------------------------------------
// findCommonSubexpressions()
//
// Find common subexpression between "this" and "other". Remove all
// value ids that are not common subexpressions from "this" (so that
// "this" contains the common subexpressions). Optionally, remove
// the common subexpressions from "other" (note that those don't need
// to have the same value ids, just be equivalent).
// ---------------------------------------------------------------------
void ValueIdSet::findCommonSubexpressions(ValueIdSet &other,
					  NABoolean removeCommonExprFromOther)
{
  ValueIdList otherList;
  CollIndex otherNumEntries;
  LIST(HashValue) otherHashValues(STMTHEAP);
  ValueId vid;
  CollIndex j;
  HashValue hashVal;

  otherList.insertSet(other);
  otherNumEntries = otherList.entries();

  // This is an n**2 (really n * m) algorithm, but for realistic cases
  // it should be good enough. To avoid performance problems, exit
  // this method for very large sets (suggested to use a hash
  // table of the hash values if this ever becomes a problem)
  if (entries() * otherNumEntries > 2000)
    {
      clear();
      return;
    }

  // make hash values for the other set
  for (j=0; j<otherNumEntries; j++)
    {
      otherHashValues.insert(otherList[j].getItemExpr()->treeHash());
    }

  // Go through my set and hash each member. If we find it, do a precise
  // comparison using duplicateMatch. Otherwise, delete the member.
  for (vid=init(); next(vid); advance(vid))
    {
      NABoolean found = FALSE;

      hashVal = vid.getItemExpr()->treeHash();
      for (j=0; j<otherNumEntries; j++)
	{
	  if (otherHashValues[j] == hashVal)
	    if (vid.getItemExpr()->duplicateMatch(
		     *(otherList[j]).getItemExpr()))
	      {
		found = TRUE;
		break;
	      }
	}

      if (found)
	{
	  // found a common subexpression
	  if (removeCommonExprFromOther)
	    other -= otherList[j];
	}
      else
	{
	  // didn't find a common subexpression, remove element from my set
	  *this -= vid;
	}
    }
}

// ---------------------------------------------------------------------
// Build predicate like the originals in the set, that substitues the
// given column reference with a computed column and the corresponding 
// computed column expression.
//
// For example: 
// predicate is : A = 2
// computed Column is: SQR
// computed Column expr is: POWER(A, 2)
// Then this method will return a ValueIdSet that contains
// a predicate that looks like this:
//    SQR = POWER(VEG(A,2),2)
// ---------------------------------------------------------------------
ValueIdSet ValueIdSet::createMirrorPreds(ValueId &computedCol,
                                         ValueIdSet underlyingCols)
{
  ValueIdSet newComputedPreds;
  ItemExpr *iePtr = computedCol.getItemExpr();

  CMPASSERT( iePtr->getOperatorType() == ITM_BASECOLUMN );

  ItemExpr *compExpr = ((BaseColumn *) iePtr)->getComputedColumnExpr().getItemExpr();
  ItemExpr *prevPred = NULL;

  for (ValueId kpv = init(); next(kpv); advance(kpv))  
   {
     ItemExpr *piePtr = kpv.getItemExpr();
     switch (piePtr->getOperatorType()) 
       {
         case ITM_VEG_PREDICATE:
         case ITM_EQUAL:
         case ITM_LESS:
         case ITM_LESS_EQ:
         case ITM_LESS_OR_LE:
         case ITM_GREATER:
         case ITM_GREATER_EQ:
         case ITM_GREATER_OR_GE:
           {
             ItemExpr * newPred = piePtr->createMirrorPred(iePtr, compExpr, underlyingCols);
             if (newPred != NULL)
               {
                 // look for the following pattern, which is common
                 // when a range of values falls within one division
                 // col1 >= const1 AND col1 <= const1
                 // and transform this into the single predicate col1 = const1
                 if (prevPred)
                   {
                     OperatorTypeEnum prevOp = prevPred->getOperatorType();
                     OperatorTypeEnum newOp = newPred->getOperatorType();
                     NABoolean neg1, neg2;
                     ConstValue *prevConst = prevPred->child(1)->castToConstValue(neg1);
                     ConstValue *newConst = newPred->child(1)->castToConstValue(neg2);

                     if ((prevOp == ITM_GREATER_EQ && newOp  == ITM_LESS_EQ ||
                          newOp  == ITM_GREATER_EQ && prevOp == ITM_LESS_EQ) &&
                         prevPred->child(0) == newPred->child(0) &&
                         prevConst && newConst &&
                         prevConst->duplicateMatch(*newConst) && neg1 == neg2 &&
                         static_cast<BiRelat *>(prevPred)->getSpecialNulls() ==
                         static_cast<BiRelat *>(newPred)->getSpecialNulls())
                       {
                         // make the <= or >= predicate of the pattern into an = predicate
                         BiRelat *newEqualPred = new(CmpCommon::statementHeap())
                           BiRelat(ITM_EQUAL,
                                   newPred->child(0),
                                   newPred->child(1));
                         newEqualPred->setSpecialNulls(
                              static_cast<BiRelat *>(newPred)->getSpecialNulls());
                         newEqualPred->synthTypeAndValueId();

                         newComputedPreds -= prevPred->getValueId();
                         newPred = newEqualPred;
                       }
                   }

                 newComputedPreds += newPred->getValueId();
                 prevPred = newPred;
               }
             break;
           }
         case ITM_ASSIGN:
         default:
            // XXX Don't expect this case to ever happen
           CMPASSERT(0);
           break;
       }
   }
   return newComputedPreds;
}

// -----------------------------------------------------------------------
// Method for rebuilding a predicate tree from a ValueIdSet.
// -----------------------------------------------------------------------
ItemExpr * ValueIdSet::rebuildExprTree
                           (OperatorTypeEnum op,
                            NABoolean redriveTypeSynthesisFlag,
			    NABoolean createFinalizeResultNode) const
{
  ValueIdList vidList;
  vidList.insertSet(*this);
  return vidList.rebuildExprTree(op,
				 redriveTypeSynthesisFlag,
				 createFinalizeResultNode);

} // ValueIdSet::rebuildExprTree()

// -----------------------------------------------------------------------
// ValueIdSet::replaceVEGExpressionsAndCopy()
// The wildcard expansion is limited to the VEGReferences that are
// found in this set. There is no attempt to traverse ItemExpr
// trees in order to replace VEGReferences that are not at the
// root.
// -----------------------------------------------------------------------
void ValueIdSet::replaceVEGExpressionsAndCopy
                    (const ValueIdSet & setContainingWildCards)
{
  clear();
  for (ValueId vid = setContainingWildCards.init();
       setContainingWildCards.next(vid);
       setContainingWildCards.advance(vid))
    {
      if (vid.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE)
        {
          // For an expression that is a VegRef, we invoke
          // VEG::getAndExpandAllValues(). That routine traverses
          // expression tree only for VegReferences at root.
          // (case 10-001201-9972: changed the following to prevent an
          // infinite loop)

          ValueIdSet recurse;

          ((VEGReference *)(vid.getItemExpr()))->
             getVEG()->getAndExpandAllValues(recurse);

	  addSet(recurse);
        }
      else
	addElement(vid);
    }
} // ValueIdSet::replaceVEGExpressionsAndCopy()

// -----------------------------------------------------------------------
// ValueIdSet::getUnReferencedVEGmembers()
//   Called only from preCodeGen. Gets all the VEG members that
//   have not been bound yet.
// -----------------------------------------------------------------------
void ValueIdSet::getUnReferencedVEGmembers
                    (const ValueIdSet & setContainingWildCards)
{
  clear();
  for (ValueId vid = setContainingWildCards.init();
       setContainingWildCards.next(vid);
       setContainingWildCards.advance(vid))
    {
      if (vid.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE)
        {
          VEGReference * vegRef = ((VEGReference *)(vid.getItemExpr()));

          ValueIdSet unReferencedValues;
          unReferencedValues.replaceVEGExpressionsAndCopy(vegRef->getVEG()->getAllValues());
          unReferencedValues -= vegRef->getVEG()->getReferencedValues();

          addSet(unReferencedValues);
        }
      else
        addElement(vid);
    }
} // ValueIdSet::getUnReferencedVEGmembers()

// -----------------------------------------------------------------------
// ValueIdSet::replaceVEGExpressions()
//
// This method is used by the code generator for rewriting
// expressions that belong to a ValueIdSet. A reference to
// a VEG, i.e., a ValueId Equality Group is replaced with
// an expression that belongs the VEG as well as to the
// set of availableValues that is supplied.
// -----------------------------------------------------------------------
void ValueIdSet::replaceVEGExpressions
                    (const ValueIdSet & availableValues,
                     const ValueIdSet & inputValues,
                     NABoolean thisIsAKeyPredicate,
                     VEGRewritePairs * lookup,
                     NABoolean replicateExpression,
                     const ValueIdSet * outputExpr,
                     const ValueIdSet * joinInputAndPotentialOutput,
                     const IndexDesc * iDesc,
                     const GroupAttributes * left_ga,
                     const GroupAttributes * right_ga)
{
  ValueIdSet newExpr;
  ItemExpr * iePtr;
  NABoolean savedReplicateExpression = replicateExpression ;
  // ---------------------------------------------------------------------
  // Iterate over the predicate factors in the given predicate tree.
  // ---------------------------------------------------------------------
  for (ValueId exprId = init(); next(exprId); advance(exprId))
    {
      if (outputExpr && outputExpr->containsTheGivenValue(exprId))
        replicateExpression = FALSE;
      // -----------------------------------------------------------------
      // Walk through the item expression tree and replace any
      // VEGPredicates or VEGReferences that are found.
      // -----------------------------------------------------------------
      // Modifying for core/TEST029 since copyTopNode() not available for VEG_Reference
      if(CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_ON &&
	 exprId.getItemExpr()->getOperatorType() == ITM_RANGE_SPEC_FUNC)
      {
	iePtr = exprId.getItemExpr()->child(1)->replaceVEGExpressions(availableValues,
                                                                inputValues,
                                                                thisIsAKeyPredicate,
                                                                lookup,
                                                                replicateExpression,
                                                                joinInputAndPotentialOutput,
                                                                iDesc,
                                                                left_ga,
                                                                right_ga);
      }
      else
      {
	iePtr = exprId.getItemExpr()->replaceVEGExpressions(availableValues,
                                                      inputValues,
                                                      thisIsAKeyPredicate,
                                                      lookup,
                                                      replicateExpression,
                                                      joinInputAndPotentialOutput,
                                                      iDesc,
                                                      left_ga,
                                                      right_ga);
      }

      replicateExpression = savedReplicateExpression;

      if (iePtr)			      // expression was not discarded
	{
	  if(!iePtr->nodeIsBound() || iePtr->isPreCodeGenNATypeChanged())
	    // redrive type synthesis for both self and child nodes
	    iePtr->synthTypeAndValueId(TRUE, TRUE);
          if (iePtr != exprId.getItemExpr())  // a replacement was done
	    {
	      subtractElement(exprId);        // remove existing ValueId
          //insert new expression(s)
          if (iePtr->getOperatorType() == ITM_AND)
              //The replacement of a RangeSpec could be an AND, convert ANDed predicates into additional values in newExpr.
              iePtr->convertToValueIdSet(newExpr, NULL, ITM_AND, FALSE, FALSE);
          else
              newExpr += iePtr->getValueId(); // replace with a new one
	    }
	}
      else // delete the ValueId of the VEGPredicate/VEGReference from the set
	subtractElement(exprId);
    } // loop over predTree

  addSet(newExpr); // add the replacement expressions

} // ValueIdSet::replaceVEGExpressions()

  // --------------------------------------------------------------------
  // ValueIdSet::getVEGesWithMultipleConsts()
  // This method accumulates those VEGes that have more than one constant
  // references in them
  // --------------------------------------------------------------------

  void ValueIdSet::getVEGesWithMultipleConsts(ValueIdSet & keyPredsToBeEvaluated)
  {
     for (ValueId exprId = init(); next(exprId); advance(exprId))
     {
       ItemExpr * vegItemExpr = exprId.getItemExpr();
       if ((vegItemExpr->getOperatorType() == ITM_VEG_PREDICATE)
	 || (vegItemExpr->getOperatorType() == ITM_VEG_REFERENCE))
       {
	 const VEG * predVEG = ((VEGPredicate *)vegItemExpr)->getVEG();
        // Now, get all members of the VEG group
        const ValueIdSet & VEGGroup = predVEG->getAllValues();
        if (VEGGroup.referencesConstExprCount() > 1)
	  keyPredsToBeEvaluated += exprId;
       }

     }
  }

// -----------------------------------------------------------------------
// ValueIdSet::replaceOperandsOfInstantiateNull()
// This method is used by the code generator for replacing the
// operands of an ITM_INSTANTIATE_NULL with a value that belongs
// to availableValues.
// -----------------------------------------------------------------------
void ValueIdSet::replaceOperandsOfInstantiateNull
                      (const ValueIdSet & availableValues,
                       const ValueIdSet & inputValues)
{
  for (ValueId exprId = init(); next(exprId); advance(exprId))
    {
      exprId.getItemExpr()->replaceOperandsOfInstantiateNull
	                      (availableValues,inputValues);
    }
} // ValueIdSet::replaceOperandsOfInstantiateNull()

// -----------------------------------------------------------------------
// weedOutUnreferenced(ValueIdSet & other)
// For each ValueId in other check whether it is referenced anywhere
// within an ItemExpr whose ValueId belongs to this set.
// -----------------------------------------------------------------------
void ValueIdSet::weedOutUnreferenced(ValueIdSet & other) const
{
  ValueIdSet unrefSet;
  NABoolean notFound;

  // loop over other
  for ( ValueId otherId = other.init();
	other.next(otherId);
	other.advance(otherId) )
  {
    notFound = TRUE;

    const ValueId & otherId2 =
      otherId.getItemExpr()->getReplacementExpr()->getValueId();

    // loop over this
    for ( ValueId exprId = init(); next(exprId); advance(exprId) )
    {
      if ( exprId.getItemExpr()->referencesTheGivenValue(otherId2) )
      {
	notFound = FALSE;
	break;
      }
    } // iterator on this set
    if ( notFound )         // accumulate the ValueIds that are
      unrefSet += otherId;  // not referenced
  } // iterator on other

  // Delete unreferenced values from other.
  other -= unrefSet;

} // ValueIdSet::weedOutUnreferenced()

void ValueIdSet::weedOutNonEquiPreds()
{
  for (ValueId v = init(); next(v); advance(v))
    {
      if (v.getItemExpr()->getOperatorType() != ITM_VEG_PREDICATE &&
          v.getItemExpr()->getOperatorType() != ITM_EQUAL)
        *this -= v;
    }
}

// -----------------------------------------------------------------------
//  referencesAConstValue
//
//  Does this set references a constant either directly (or indirectly
//  thru a VEGReference)?
// -----------------------------------------------------------------------
NABoolean ValueIdSet::referencesAConstValue(ItemExpr ** constant) const
{
  for (ValueId id = init(); next(id); advance(id))
    {
      const ItemExpr *pred = id.getItemExpr();
      if (pred->getOperatorType() == ITM_CONSTANT)
	{
	  *constant = id.getItemExpr();
	  return TRUE;
	}
      else if (pred->getOperatorType() == ITM_VEG_REFERENCE)
      {
	const VEG * predVEG = ((VEGPredicate *)pred)->getVEG();

	if (predVEG->seenBefore())
	  return FALSE;

	predVEG->markAsSeenBefore();

        // Now, get all members of the VEG group
	ItemExpr *constant2;
        const ValueIdSet & VEGGroup = predVEG->getAllValues();
	if (VEGGroup.referencesAConstValue(&constant2))
	{
	  *constant = constant2;
	  predVEG->markAsNotSeenBefore();
	  return TRUE;
	}
	predVEG->markAsNotSeenBefore();
      }
    }
  return FALSE;
} // ValueIdSet::referencesAConstValue()

// -----------------------------------------------------------------------
//  referenceAConstExpr
//
//  This method is invoked to determine whether any element of the
//  valueidset contains a "constant expression", i.e. a constant,
//  host variable or a parameter.
// -----------------------------------------------------------------------
NABoolean ValueIdSet::referencesAConstExpr(ItemExpr** constExprPtr) const
{
  for (ValueId id = init(); next(id); advance(id))
    {
      const ItemExpr *pred = id.getItemExpr();

      //
	  //check if the item expr is a non-strict constant
	  //a strict constant is somethine like cos(1)
	  //where as cos(?p) can be considered a constant
	  //in the non-strict definition since it remains
      //constant for a given execution of a query
      if (pred->doesExprEvaluateToConstant(FALSE,TRUE))
	{
          if ( constExprPtr )
             *constExprPtr = id.getItemExpr();

	  return TRUE;
	}
      else if (pred->getOperatorType() == ITM_VEG_REFERENCE)
        {
          const VEG * predVEG = ((VEGPredicate *)pred)->getVEG();

	  // To avoid infinite loop for cases where a VEG references itself.
	  // get to each element of the VEG, only if it has not been seen
	  // before
	  if (predVEG->seenBefore() )
	    return FALSE;

	  predVEG->markAsSeenBefore();

	  // Now, get all members of the VEG group
	  const ValueIdSet & VEGGroup = predVEG->getAllValues();
	  if (VEGGroup.referencesAConstExpr(constExprPtr))
	    {
	      predVEG->markAsNotSeenBefore();
	      return TRUE;
	    }
	  predVEG->markAsNotSeenBefore();
        }
    }
  return FALSE;
} // referencesAConstExpr()

NABoolean ValueIdSet::doAllExprsEvaluateToConstant(NABoolean isStrict, 
                                                   NABoolean considerVEG) const
{
  for (ValueId id = init(); next(id); advance(id))
    {
      const ItemExpr *pred = id.getItemExpr();
      if (!pred->doesExprEvaluateToConstant(isStrict,considerVEG))
        return FALSE;
    }
    return TRUE ;
} // doAllExprsEvaluateToConstant()

void ValueIdSet::removeConstExprReferences(NABoolean considerExpressions)
{
  for (ValueId id = init(); next(id); advance(id))
  {
    const ItemExpr *expr = id.getItemExpr();

    // we want to consider hostvars and parameters are consts
    NABoolean strict = FALSE;

    // we want to dig inside VEGREFS to see if there is an
    // equality with a constant
    NABoolean considerVEG = TRUE;

    if (expr->doesExprEvaluateToConstant(strict,considerVEG))
	  {
      subtractElement(id);
	  }
    else if (expr->getOperatorType() == ITM_VEG_REFERENCE)
    {
      const VEG * exprVEG = ((VEGReference *)expr)->getVEG();

      // Now, get all members of the VEG group, only if this VEG has not
	    // been seen before. This is to avoid infinite looping for cases
	    // where a VEG references itself
	    if (!exprVEG->seenBefore())
	    {
	      exprVEG->markAsSeenBefore();
        const ValueIdSet & VEGGroup = exprVEG->getAllValues();
        if (VEGGroup.referencesAConstExpr())
	      {
		      subtractElement(id);
	      }
	      exprVEG->markAsNotSeenBefore();
	    }
    }
  }
} // removeConstExprReferences()

// -----------------------------------------------------------------------
//  referenceConstExprCount
//
//  This method is invoked to determine how many elements of the
//  valueidset contains a "constant expression", i.e. a constant,
//  host variable or a parameter.
// -----------------------------------------------------------------------
Int32 ValueIdSet::referencesConstExprCount() const
{
  Int32 count = 0;
  for (ValueId id = init(); next(id); advance(id))
    {
      const ItemExpr *pred = id.getItemExpr();

    //
	//check if the item expr is a non-strict constant
	//a strict constant is somethine like cos(1)
	//where as cos(?p) can be considered a constant
	//in the non-strict definition since it remains
    //constant for a given execution of a query
    if (pred->doesExprEvaluateToConstant(FALSE,TRUE))
	{
	  count = count + 1;
	}
      else if (pred->getOperatorType() == ITM_VEG_REFERENCE)
        {
          const VEG * predVEG = ((VEGPredicate *)pred)->getVEG();
          // Now, get all members of the VEG group, only if this VEG has not
	  // been seen before. This is to avoid infinite looping for cases
	  // where a VEG references itself
	  if (!predVEG->seenBefore())
	  {
	    predVEG->markAsSeenBefore();
	    const ValueIdSet & VEGGroup = predVEG->getAllValues();
	    count = count + VEGGroup.referencesConstExprCount();
	    predVEG->markAsNotSeenBefore();
	  }
        }
    }
  return count;
} // referencesConstExprCount()


// -----------------------------------------------------------------------
// referenceAHostvariableorParam
//
// Checks if the valueidset is a veg predicate and if it contains a
// hostvariable, or param.
// referencesAConstValue() returns TRUE if the VEGGroup contains a constant
// referencesAConstExpr() returns TRUE if the VEGGroup contains a constant
// or a hostvar/dynamic parameter
// Thus, if referencesAConstExpr returns TRUE, but referencesAConstValue
// returns FALSE, then there's a hostvar, not a constant.
// -----------------------------------------------------------------------
// The patch is added to take care of the case when predicate is an equality
// predicate with column or vegref on the left side and CAST of dynamic
// parameter on the right side (TPCC Q7)

NABoolean ValueIdSet::referencesAHostvariableorParam() const
{
  for( ValueId id = init(); next(id); advance(id))
    {
       const ItemExpr * pred = id.getItemExpr();

       if ( pred->getOperatorType() == ITM_VEG_PREDICATE )
         {
           VEG * predVEG = ((VEGPredicate *)pred)->getVEG();

	   // Now, get all members of the VEG group, only if this VEG has not
	   // been seen before. This is to avoid infinite looping for cases
	   // where a VEG references itself
	   if (predVEG->seenBefore())
	    return FALSE;

	   predVEG->markAsSeenBefore();

	   const ValueIdSet & VEGGroup = predVEG->getAllValues();

	   // See if the VEG group contains a constant.
	   // NOTE:  A VEG group should not contain more than 1 constant; but if
	   //        that should happen, referencesAConstValue will return the first
	   //        constant found.

	   ItemExpr *constant = NULL;
	   NABoolean containsConstant  = VEGGroup.referencesAConstValue( &constant );
	   NABoolean containsConstExpr = VEGGroup.referencesAConstExpr();
	   NABoolean containsHostvar   = ( containsConstExpr && !containsConstant );

	   predVEG->markAsNotSeenBefore();
	   if(containsHostvar)
	     return TRUE;
         }

       if ( pred->getOperatorType() == ITM_EQUAL )
         {
           const ItemExpr *leftExpr  = pred->child(0);
           const ItemExpr *rightExpr = pred->child(1);

           const ValueId &leftChildVid  = leftExpr->getValueId();
           const ValueId &rightChildVid = rightExpr->getValueId();

          //check if the item expr is a non-strict constant
          //a strict constant is somethine like cos(1), CAST(1)
          //where as cos(?p), CAST(?p) can be considered a constant
          //in the non-strict definition since it remains
          //constant for a given execution of a query
          if ( leftExpr->doesExprEvaluateToConstant(FALSE) OR
	       rightExpr->doesExprEvaluateToConstant(FALSE) )
	  {
            return TRUE;
          }
       }
    }

  return FALSE;
}

ValueIdSet
ValueIdSet::replaceRangeSpecRefs() const
{
  // If not using rangespec, just return the original set.
  if (CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) != DF_ON)
    return *this;

  ValueIdSet derangedPreds;
  ValueIdSet vs;
  ItemExpr* pred;

  // For each pred, it is a RangespecRef, add its expansion to the new set.
  // Otherwise, just add the pred itself to the new set.
  for (ValueId vid = init(); next(vid); advance(vid))
  {
    pred = vid.getItemExpr();
    if (pred->getOperatorType() == ITM_RANGE_SPEC_FUNC)
    {
      vs.clear();
      (static_cast<RangeSpecRef*>(pred))->getValueIdSetForReconsItemExpr(vs);
      derangedPreds += vs;
    }
    else
      // Add the predicate to the new set.
      derangedPreds += vid;
  }

  return derangedPreds;
}

ValueIdSet
ValueIdSet::removePredsWithHostVars() const
{
  ValueIdSet localPreds;
  localPreds.clear();
  
  // check if any predicate in this set is a host variable
  for( ValueId id = init(); next(id); advance(id))
  {
    if (id.getItemExpr()->referencesAHostVar())
    {
      // continue to evaluate next pred
      continue;
    }
    // else add the predicate to the new set
    localPreds.insert(id);
  }
  return localPreds;
}

// Return TRUE if any itemExpr is of Bignum type.
NABoolean ValueIdSet::referencesBignumNumericDataType() const
{
  for (ValueId id = init(); next(id); advance(id))
    {
      const ItemExpr *pred = id.getItemExpr();

        
      if (pred->getOperatorType() == ITM_VEG_REFERENCE)
        {
          const VEG * predVEG = ((VEGReference *)pred)->getVEG();
          // Now, get all members of the VEG group, only if this VEG has not
          // been seen before. This is to avoid infinite looping for cases
          // where a VEG references itself
          if (!predVEG->seenBefore())
          {
            predVEG->markAsSeenBefore();
            const ValueIdSet & VEGGroup = predVEG->getAllValues();
            NABoolean result = VEGGroup.referencesBignumNumericDataType();
            predVEG->markAsNotSeenBefore();
            if ( result == TRUE ) return TRUE;
          }
        }
      else if (pred->getOperatorType() == ITM_VEG_PREDICATE)
        {
          const VEG * predVEG = ((VEGPredicate *)pred)->getVEG();
          // Now, get all members of the VEG group, only if this VEG has not
          // been seen before. This is to avoid infinite looping for cases
          // where a VEG references itself
          if (!predVEG->seenBefore())
          {
            predVEG->markAsSeenBefore();
            const ValueIdSet & VEGGroup = predVEG->getAllValues();
            NABoolean result = VEGGroup.referencesBignumNumericDataType();
            predVEG->markAsNotSeenBefore();
            if ( result == TRUE ) return TRUE;
          }
        }
       else 
        {
          const NAType * type = &(pred->getValueId().getType());

          if ( (type->getTypeQualifier() == NA_NUMERIC_TYPE &&
               ((const NumericType *)type)->isBigNum()) ) 
          {
              return TRUE;
          }

        }
    }
  return FALSE;
}
// ---------------------------------------------------------------------
// Remove any predicate with rolling columns from the
// set of predicates
// ---------------------------------------------------------------------
ValueIdSet 
ValueIdSet::removePredsWithRollingCols(TableDesc *tdesc) const
{
  ValueIdSet localPreds;
  localPreds.clear();
  const TableAnalysis * ta = tdesc->getTableAnalysis();

  // it is not a table, so don't bother to call fetchCount
  if (!ta)
    return localPreds;

  // get all user columns of this table
  ValueIdSet usedCols = ta->getUsedCols();

  // traverse thru all predicates
  for( ValueId id = init(); next(id); advance(id))
  {
    // if the predicate is an equality or a range predicate
    // check to see if it is on a date column and if the constant
    // it is being compared to is greater than the value in the 
    // histogram
    ItemExpr * pred = id.getItemExpr();
    OperatorTypeEnum op = pred->getOperatorType();

    // if it is not an equality or range pred, insert for evaluation by fetchCount
    if (!((op ==  ITM_VEG_PREDICATE ) ||
        (op ==  ITM_EQUAL) ||
        (op ==  ITM_NOT_EQUAL) ||
        (op ==  ITM_LESS) ||
        (op ==  ITM_LESS_EQ) ||
        (op ==  ITM_GREATER) ||
        (op ==  ITM_GREATER_EQ) ||
        (op == ITM_OR) ||
        (op == ITM_AND)))
    {
      localPreds.insert(id);
      continue;
    }

    // if the type of the predicate is month(col), dayofmonth(col) etc
    // then use fetchCount
    if ( (op != ITM_VEG_PREDICATE) &&
         ((pred->child(0)->getOperatorType() == ITM_EXTRACT_ODBC) ||
         (pred->child(0)->getOperatorType() == ITM_EXTRACT)) )
    {
      localPreds.insert(id);
      continue;
    }

    // get the column from the predicate
    ValueIdSet predCols;
    pred->findAll(ITM_BASECOLUMN, predCols, TRUE, TRUE);

    // from this column set extract columns of this table
    predCols = predCols.intersect(usedCols);
    if (predCols.entries() != 1)
    {
      // if there are more than one column from this table in the
      // predicate, use the predicate for fetchCount
      localPreds.insert(id);
      continue;
    }

    // there is only one column from this table in the predicate
    // check if it is date type
    ValueId colId;
    predCols.getFirst(colId);
    if (colId == NULL_VALUE_ID)
    {
      localPreds.insert(id);
      continue;
    }

    if (colId.getType().getTypeQualifier() != NA_DATETIME_TYPE )
    {
      localPreds.insert(id);
      continue;
    }
    
    // it is date type, get the constant value from the predicate 
    ValueIdSet leafValues;
    pred->findAll(ITM_CONSTANT, leafValues, TRUE, TRUE);
    
    if(leafValues.entries() == 0)
    {
      // there is no constant in the predicate, insert the predicate for CTS
      localPreds.insert(id);
      continue;
    }
  
    // get the histogram for the column. If the histogram is fake
    // insert the predicate for evaluation
    ColStatsSharedPtr colStats = tdesc->getTableColStats().getColStatsPtrForColumn(colId);

    if(colStats == NULL)
    {
      // No histograms fetched for the predicate column, it could be a DDL query, 
      // continue. 
      continue;
    }
  
    if (colStats->isFakeHistogram())
    {
      // histogram is fake, so no need to comapre the values. Use fetchCount
      localPreds.insert(id);
      continue;
    }

    // for OR and AND, there could be more than one date values
    // go thru each date value, and if even one of them lies outside 
    // histogram range, do not use fetchcount on that predicate

    NABoolean insertPred = TRUE;
    for (ValueId cid = leafValues.init();
                       leafValues.next(cid);
                       leafValues.advance(cid))
    {
      // from all the leaf values extract the constant value
      ItemExpr *constant = NULL;
      ValueIdSet cidSet(cid);
      cidSet.referencesAConstValue( &constant );

      // compare the constant value with the min and the max values of the histogram
      NABoolean neg = FALSE;
      ConstValue * cv = NULL;
      
      if (constant)
        cv = constant->castToConstValue(neg);
      else
        insertPred = FALSE;

      // get the encoded value of the constant if any
      EncodedValue val (UNINIT_ENCODEDVALUE) ;

      if (cv != NULL)
      {
        // get the encoded format for the constant
        val = EncodedValue (cv, neg);
 
        // valid constant, compare the value with the histogram range
        // if within the range use fetchCount, else use the rolling 
        // column adjustment done by the histogram
        if ((val < colStats->getMinValue()) ||
          (val > colStats->getMaxValue()))
        {
          insertPred = FALSE;
          break;
        }
      }
    } // for all leafValues;
    if (insertPred)
    {
      localPreds.insert(id);
    }
  }
  return localPreds;
}

// -----------------------------------------------------------------------
// getColumnsForHistogram
//
// returns columns which are referred in the expressions containing constants,
// constant expressions or host variables
// --------------------------------------------------------------------------

ValueIdSet ValueIdSet::getColumnsForHistogram() const
{
  ValueIdSet result;
  result.clear();

  // traverse through all ValueIds, looking for VEG preds
  for (ValueId id = init(); next(id); advance(id))
  {
    VEG * predVEG = NULL;
    ItemExpr * expr = id.getItemExpr();

    // Presently we are considering only columns in vegPredicates
    // There could be cases where we have a Cast over a column.
    // Example, Cast(col1) = Cast(col2). In this case, the predicate is
    // not transformed into a VEG_PREDICATE, instead it remains as an
    // equality predicate. If we find cases where histograms for these
    // columns are not being picked up during synthEstLogProp. We should
    // revisit this code to add condition for ITM_EQUAL

    if ( expr->getOperatorType() == ITM_VEG_PREDICATE )
      predVEG = ((VEGPredicate *)expr)->getVEG();
    else
      continue ; // it is not a VEG predicate, so continue

    ValueIdSet columns;
    expr->findAll( ITM_BASECOLUMN, columns, TRUE, TRUE );

    // Now get all the base tables in this predicate

    // see if this column is being joined to a column of another table
    SET(TableDesc *) * tablesJoined = columns.getAllTables();
    if (tablesJoined->entries() > 1)
    {
      ValueId vegREF = predVEG->getVEGReference()->getValueId();
      result.insert(vegREF);
    }
  }

  return result;
}



void ValueIdSet::getConstants(ValueIdSet & addConstantsToThisSet, NABoolean includeCacheParams) const
{
  for (ValueId id = init(); next(id); advance(id))
    {
      const ItemExpr *pred = id.getItemExpr();
      if (pred->getOperatorType() == ITM_CONSTANT)
        addConstantsToThisSet += id;
      else
      if (includeCacheParams == TRUE && pred->getOperatorType() == ITM_CACHE_PARAM )
        addConstantsToThisSet += id;
    }
}

void ValueIdSet::getConstantExprs(ValueIdSet & addConstantExprsToThisSet, 
				  NABoolean isStrict) const
{
  for (ValueId id = init(); next(id); advance(id))
    {
      ItemExpr *ie = id.getItemExpr();

      //
      //check if the item expr is a non-strict constant
      //a strict constant is somethine like cos(1)
      //where as cos(?p) can be considered a constant
      //in the non-strict definition since it remains
      //constant for a given execution of a query
      // if isStrict we look only for constants and not parameters and hostvars
      // However we still look for constants inside a VEG
      if (ie->doesExprEvaluateToConstant(isStrict,TRUE) OR
          ((ie->previousHostVar() == TRUE) && (NOT isStrict)))
        addConstantExprsToThisSet += id;
    }
}

// -----------------------------------------------------------------------
//  Get Outer References
// -----------------------------------------------------------------------
void ValueIdSet::getOuterReferences(ValueIdSet & OuterRefs) const
{
  NABoolean doNotInsert ;
  for (ValueId id = init(); next(id); advance(id))
    {
      const ItemExpr *pred = id.getItemExpr();
      doNotInsert = FALSE ;
      if (pred->getOperatorType() == ITM_VEG_REFERENCE)


      {
	ItemExpr *constant = NULL;
	const ValueIdSet & VEGGroup =((VEGReference *)pred)->getVEG()->getAllValues();
	if (VEGGroup.referencesAConstExpr(&constant))
	  doNotInsert = TRUE ;
      }
      else if (pred->isAUserSuppliedInput())
	doNotInsert = TRUE ;

      if (NOT doNotInsert)
      {
	  OuterRefs.insert (id);
      }
  }
} // ValueIdSet::getOuterReferences()

// -----------------------------------------------------------------------
// categorizePredicates:
// Group the predicates in the following categories, to be applied
// in the following order:
//
// (1) BiLogic: AND, or OR
// (2) Local Predicates - those that do not contain outer references
//     a) VEG equality predicates
//     b) other non-VEG predicates supported by synthesis (for now,
//        these include class BiRelat predicates, and some UnLogic, only)
// (3) Non-local predicates - those that do contain outer references
//     a) VEG predicates
//     b) other non-VEG class BiRelat and UnLogic predicates
// -----------------------------------------------------------------------

void ValueIdSet::categorizePredicates (const ValueIdSet & outerReferences,
				       ValueIdSet & EqLocalPreds,
				       ValueIdSet & OtherLocalPreds,
				       ValueIdSet & EqNonLocalPreds,
				       ValueIdSet & OtherNonLocalPreds,
				       ValueIdSet & BiLogicPreds,
				       ValueIdSet & DefaultPreds ) const
{

  for (ValueId id = init(); next(id); advance(id))
    {
      const ItemExpr *pred = id.getItemExpr();

      if (pred->synthSupportedOp() && // does the operator support synthesis??
	  ((0 > pred->getSelectivityFactor()) || //++MV does this operator has an assigned selectivity factor
	   ((ItemExpr *)pred)->isSelectivitySetUsingHint())) // Unless selectivity is specified by user
	{
	  OperatorTypeEnum op = pred->getOperatorType();
	  if (op == ITM_VEG_PREDICATE) // Equality (transitive closure) predicate?
	    {
	      if (outerReferences.entries() > 0)
		{
		  if (pred->referencesOneValueFrom(outerReferences))
		    EqNonLocalPreds.insert (id);
		  else
		    EqLocalPreds.insert (id);
		}
	      else
		EqLocalPreds.insert (id);
	    }
	  else if ((op == ITM_LESS) || (op == ITM_LESS_EQ) ||
		   (op == ITM_GREATER) || (op == ITM_GREATER_EQ) ||
		   (op == ITM_EQUAL) || (op == ITM_NOT_EQUAL) ||
		   (op == ITM_IS_NULL) || (op == ITM_IS_NOT_NULL) ||
		   (op == ITM_IS_UNKNOWN) || (op == ITM_IS_NOT_UNKNOWN))
	    {
	      if (outerReferences.entries () > 0)
		{
		  if (pred->referencesOneValueFrom(outerReferences))
		    OtherNonLocalPreds.insert (id);
		  else
		    OtherLocalPreds.insert (id);
		}
	      else
		OtherLocalPreds.insert (id);
	    }
	  else if ((op == ITM_OR) || (op == ITM_AND))
	    {
	      BiLogicPreds.insert (id);
	    }
	  else
    {
	    CCMPASSERT ("Unsupported predicate found");
      continue;
    }

	} // endif (pred->synthSupportedOp())

      else // operator not supported by synthesis

	DefaultPreds.insert (id);

    } // end for loop over setOfPredicates

}    // categorizePredicates

// This method returns TRUE if there are any predicates for which the 
// cardinality cannot be accurately estimated by the optimizer
// Conditions are:
// 1. Any predicate with a hos variable or a dynamic parameter
// 2. Any predicate for which the histograms cannot be directly used. These include
//    Case expressions, LIKE predicates, or any other predicate
// 3. For more than one range predicate
// 4. For one range and one or more equality predicate
// 5. Any Bilogic pred
// 6. Equality predicaes 
//    a. Any equality on an expression
//    b. Equality on more than one column of the same table. T1.a = T1.b
//    c. More than one equality predicate, and multi col UEC does not exist for that set of columns
//       Also the columns do not constitute unique index

NABoolean 
ValueIdSet::predsReqFetchCnt(TableDesc * tdesc)
{
  // categorize all predicates so it is to determine if the
  // predicates are complex or not

  // if there are no predicates, return FALSE
  if (entries() == 0)
    return FALSE;

  // check to see if any of the predicates contain a hostVar or dynamic
  // parameter

  ItemExpr *constant = NULL;
  NABoolean containsConstant  = referencesAConstValue( &constant );
  NABoolean containsConstExpr = referencesAConstExpr();
  NABoolean containsHostvar   = ( containsConstExpr && !containsConstant );

  // if they do, return FALSE, we do not want to call fetchCount with
  // host variables or dynamic parameters
  if (containsHostvar)
    return TRUE;
  
  ValueIdSet eqLocalPreds, otherLocalPreds, eqNonLocalPreds,
             otherNonLocalPreds, biLogicPreds, defaultPreds;

  // We will not concern ourselves with outer refernces at this time
  ValueIdSet outerRefs;
  outerRefs.clear();

  categorizePredicates (outerRefs, eqLocalPreds,
                        otherLocalPreds, eqNonLocalPreds,
			otherNonLocalPreds, biLogicPreds,
			defaultPreds);

  // Complex predicates are defined as:
  // 1. Predicates for which the optimizer will use default selectivity
  // 2. Bi-logic preds (AND / OR)
  // 3. More than one range predicates
  if ( ((otherLocalPreds.entries() > 0) && (eqLocalPreds.entries() > 0)) || 
       (otherLocalPreds.entries() > 1) ||
       (biLogicPreds.entries() > 0) ||
       (defaultPreds.entries() > 0) ||
       (eqNonLocalPreds.entries() > 0) ||
       (otherNonLocalPreds.entries() > 0))
    return TRUE;
  
  // for range predicates, use fetchcount:
  // 1. If there is one range predicate but it is on an expression
  // example a+2 > 10
  if (otherLocalPreds.entries() == 1) 
  {
    ValueId pred;
    otherLocalPreds.getFirst(pred);
    // if predicate is an expression such as a + 2 > 10
    // use fetchCount
    if (pred.anExpression()) 
      return TRUE;

    // if predicate involves more than one column from the same
    // table such as a > b use fetchCount
    ValueIdSet colsOfThisTableInPred;

    if (pred.moreThanOneColFromSameTabOrCharDate(tdesc, colsOfThisTableInPred)) 
      return TRUE;

    // for all other cases return FALSE
    return FALSE;
  }

  if (eqLocalPreds.entries() > 0)
  {
    // check if any predicate is an equality on more
    // than one column, such as a = b where both a and b
    // belong to the same table
    // During this process also collect base columns from
    // all predicate to check for correlation and multi-column 
    // stats later
    ValueIdSet cols;
    for(ValueId predId = eqLocalPreds.init();eqLocalPreds.next(predId);eqLocalPreds.advance(predId))
    {
      ValueIdSet colsOfThisTableInPred;
      // There are one or less columns from this table participating
      // in the predicates, so no need to use fetchCount
      if (predId.moreThanOneColFromSameTabOrCharDate(tdesc, colsOfThisTableInPred))
        return TRUE;

      cols.insert(colsOfThisTableInPred);
    }

    // if the columns constitute unique index, no need to use fetchCount
    if (cols.doColumnsConstituteUniqueIndex(tdesc))
      return FALSE;

    // There are more than one columns from this table, see if multi-column
    // stats exists for this set of cols, if not use fetchCount
    const MultiColumnUecList* uecList = tdesc->getTableColStats().getUecList();

    if ( uecList ) {
      CostScalar mcUec = uecList->lookup(cols);
      if (mcUec <= 0)
        return TRUE;
    }
  }
    if (entries() > (ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_INT_43))
      return TRUE;
    else
      return FALSE;
  }

NABoolean
ValueId::anExpression()
{
  ItemExpr * expr = getItemExpr();
  if (!expr) 
    return FALSE;

  for (Int32 i=0; i<expr->getArity(); i++)
  {
    // predicate is an expression
    if (expr->child(i)->getArity() >= 1)
      return TRUE;
  }
  return FALSE;  
}


NABoolean
ValueId::moreThanOneColFromSameTabOrCharDate(TableDesc * tdesc,
                                     ValueIdSet & colsOfThisTable)
{
  // if the predicate is like a > b such that both a and b 
  // belong to same table, then also we want to call fetchCount
  // optimizer will use default selectivity of 33%
  ValueIdSet predCol;
  ValueIdSet predSet(*this);

  predSet.findAllReferencedBaseCols(predCol);

  // if there is only one column in the predicate
  // check for only character columns;
  NABoolean checkForCharCol = FALSE;
  if (predCol.entries() <= 1)
    checkForCharCol = TRUE;

  if (tdesc && tdesc->getTableAnalysis() )
  {
    ValueIdSet usedCols = tdesc->getTableAnalysis()->getUsedCols();
    colsOfThisTable = predCol.intersect(usedCols);
    if (colsOfThisTable.entries() > 1)
      return TRUE;
    if (checkForCharCol || (colsOfThisTable.entries() == 1))
    {
      ValueId colId;
      colsOfThisTable.getFirst(colId);
      if (colId == NULL_VALUE_ID)
        return FALSE;

      // get column type and return TRUE if date time type
      if (colId.getType().getTypeQualifier() == NA_DATETIME_TYPE )
          return TRUE;
      // if the intervals of the histograms had to be merged due 
      // to conflicting interval boundaries use 
      // fetchCount. 
      ColStatsSharedPtr colStats = tdesc->getTableColStats().getColStatsPtrForColumn(colId);
      if (colStats == NULL)
        return FALSE;
      if (colStats->isColWithBndryConflict())
        return TRUE;
    }
  }
  return FALSE;
}

NABoolean ValueId::isDivisioningColumn() const
{
   ItemExpr *ck = getItemExpr();

   if ( ck == NULL )
      return FALSE;

   if (ck->getOperatorType() == ITM_INDEXCOLUMN)
      ck = ((IndexColumn *) ck)->getDefinition().getItemExpr();

   if (ck->getOperatorType() == ITM_BASECOLUMN)
      return (((BaseColumn *) ck)->getNAColumn()->isDivisioningColumn());
   else 
      return FALSE;
}

NABoolean ValueId::isSaltColumn() const
{
   ItemExpr *ck = getItemExpr();

   if ( ck == NULL )
      return FALSE;

   if (ck->getOperatorType() == ITM_INDEXCOLUMN)
      ck = ((IndexColumn *) ck)->getDefinition().getItemExpr();

   if (ck->getOperatorType() == ITM_BASECOLUMN)
     return (((BaseColumn *) ck)->getNAColumn()->isSaltColumn());
   else
     return FALSE;
}

// -----------------------------------------------------------------------
// getReferencedPredicates:
// This method goes through the ValueIdSet pointed to by the this pointer.
// All those valueids that are referenced by any member
// of the valueIdSet that is the first argument to this method are inserted
// into the ValueIdSet that is the second argument. This method can be used
// get all the predicates in the selection predicate of a node that contain
// outer references. Method returns TRUE if any predicate is found that 
// references a valueid from the first set.
//
// The default behavior of the routine is to skip any expression that 
// evaluates to a constant(see ItemExpr::referencesOneValueFrom()). If 
// you want those expressions to be searched as well, set searchConstExpr
// to TRUE.
// -----------------------------------------------------------------------

NABoolean ValueIdSet::getReferencedPredicates (const ValueIdSet & outerReferences,
					       ValueIdSet & nonLocalPreds) const
{
  NABoolean found = FALSE;
  if (outerReferences.entries() > 0)
  {
    for (ValueId id = init(); next(id); advance(id))
      {
	const ItemExpr *pred = id.getItemExpr();
	if (pred->referencesOneValueFrom(outerReferences))
	{
	  nonLocalPreds.insert (id);
	  found = TRUE ;
	}
      }
  }
  return found;
}

// ---------------------------------------------------------------------
// ValueIdSet::getAllTables()
// This method will get all tables whose columns are included in the set
// We assume there are all base columns. Hence use all base columns in this
// set.
// ----------------------------------------------------------------------

SET(TableDesc *) * ValueIdSet::getAllTables()
{

  SET(TableDesc *) * tableSet = new STMTHEAP SET(TableDesc *)(STMTHEAP);

  for (ValueId column = init(); next(column); advance(column) )
  {
      const ItemExpr * colExpr = column.getItemExpr() ;
      if (colExpr->getOperatorType() != ITM_BASECOLUMN) continue;

      TableDesc * tableDescForCol = ((BaseColumn *)colExpr)->getTableDesc();

      tableSet->insert(tableDescForCol);
    }
  return tableSet;
}

// ---------------------------------------------------------------------
// ValueIdSet::doColumnsConstituteUniqueIndex(const NATable * table)
// Returns a boolean to indicate if these columns constitute a primary
// key or unique index.
// ---------------------------------------------------------------------
NABoolean ValueIdSet::doColumnsConstituteUniqueIndex(TableDesc * table, NABoolean considerStats)
{
  NABoolean colsConstUniqueIndex = FALSE;

  ValueIdSet primaryKeyColumns = table->getPrimaryKeyColumns();

  if ((primaryKeyColumns.entries() > 0) &&
      (contains(primaryKeyColumns)) )
    colsConstUniqueIndex = TRUE;
  else
  {
    // Complete primary key is not covered by the joining columns. See if
    // if any unique index is covered

    const LIST(IndexDesc *) &uniqueIndexes = table->getUniqueIndexes();
    for (CollIndex i = 0; i < uniqueIndexes.entries(); i++)
    {
      IndexDesc * index = uniqueIndexes[i];
          ValueIdSet indexColumns(index->getIndexKey());
    }
  }

  // if the column set is not unique based on semantics, check to see if it is unique based on
  // its statistics. That is if any column in the set is unique, or has UEC equal to rowcount
  // then we say that the column set is unique
  // Do that only if it is being tested for cardinality estimation. 
  if ((colsConstUniqueIndex == FALSE) && considerStats)
  {
    // get the rowcount from the table colstats
    ColStatDescList colStats = table->tableColStats();
    CostScalar rc = colStats[0]->getColStats()->getRowcount();
    for (ValueId vid = init(); next (vid); advance (vid))
    {
      ColStatsSharedPtr cs = colStats.getColStatsPtrForColumn(vid);
      if (cs)
      {
        if (cs->getUecBeforePreds() == rc)
        {
          colsConstUniqueIndex = TRUE;
          break;
        }
      }
    }
  }
  return colsConstUniqueIndex;
}

// -----------------------------------------------------------------------
// containsSubquery()
// determine if any of the vids correspond to a subquery, in this vs.
// -----------------------------------------------------------------------
NABoolean ValueIdSet::containsSubquery() const
{
  for (ValueId vid = init(); next(vid); advance(vid))
    {
      if (vid.getItemExpr()->containsSubquery())
        return TRUE;
    }
  return FALSE;
} // ValueIdSet::containsSubquery

// -----------------------------------------------------------------------
// containsUDF()
// determine if any of the vids correspond to a UDF, in this vs.
// -----------------------------------------------------------------------
ItemExpr *ValueIdSet::containsUDF() const
{
  for (ValueId vid = init(); next(vid); advance(vid))
    {
      if (vid.getItemExpr()->containsUDF())
        return vid.getItemExpr()->containsUDF();
    }
  return 0;
} // ValueIdSet::containsUDF

// -----------------------------------------------------------------------
// containsIsolatedUDFunction()
// determine if any of the vids correspond to an Isolated UDFunction,in this vs.
// -----------------------------------------------------------------------
NABoolean ValueIdSet::containsIsolatedUDFunction() const
{
  for (ValueId vid = init(); next(vid); advance(vid))
    {
      if (vid.getItemExpr()->containsIsolatedUDFunction())
        return TRUE;
    }
  return FALSE;
} // ValueIdSet::containsIsolatedUDFunction

// -----------------------------------------------------------------------
// containsCount()
// determine if any of the vids correspond to a count, in this vs.
// -----------------------------------------------------------------------
NABoolean ValueIdSet::containsCount() const
{
  for (ValueId vid = init(); next(vid); advance(vid))
    {
    if ((vid.getItemExpr()->origOpType() == ITM_COUNT) ||
        (vid.getItemExpr()->origOpType() == ITM_COUNT_STAR__ORIGINALLY)) 
        return TRUE;
    }
  return FALSE;
} // ValueIdSet::containsCount

// -----------------------------------------------------------------------
// containsOneTrue()
// determine if any of the vids correspond to a oneTrue, in this vs.
// -----------------------------------------------------------------------
NABoolean ValueIdSet::containsOneTrue(ValueId &refOneTrue ) const
{

   for ( ValueId vid = init(); next(vid); advance(vid))
   {
      if (vid.getItemExpr()->isAnAggregate() && 
         (vid.getItemExpr()->getOperatorType() == ITM_ONE_TRUE))
      {
         refOneTrue = vid;
         return TRUE;
      }
   }
   return FALSE;
} // ValueIdSet::containsOneTrue


// -----------------------------------------------------------------------
// containsAnyTrue()
// determine if any of the vids correspond to a anyTrue, in this vs.
// -----------------------------------------------------------------------
NABoolean ValueIdSet::containsAnyTrue(ValueId &refAnyTrue ) const
{

   for ( ValueId vid = init(); next(vid); advance(vid))
   {
      if (vid.getItemExpr()->isAnAggregate() && 
         (vid.getItemExpr()->getOperatorType() == ITM_ANY_TRUE))
      {
         refAnyTrue = vid;
         return TRUE;
      }
   }
   return FALSE;
} // ValueIdSet::containsAnyTrue

NABoolean ValueIdSet::containsFalseConstant(ValueId &falseConstant ) const
{

   for ( ValueId vid = init(); next(vid); advance(vid))
   {
     OperatorTypeEnum OpType = vid.getItemExpr()->getOperatorType();
     
     if( OpType == ITM_RETURN_FALSE )
     {
       falseConstant = vid;
       return TRUE;
     }
     else if ( OpType == ITM_CONSTANT )
     {
       NABoolean negate;
       ConstValue *cv = vid.getItemExpr()->castToConstValue(negate);
       if( cv && cv->isAFalseConstant())
       {
         falseConstant = vid;
         return TRUE;
       }
     }
   }
   return FALSE;

} // ValueIdSet::containsFalseConstant

// -----------------------------------------------------------------------
// Methods used for debugging and display.
// -----------------------------------------------------------------------
void ValueIdSet::unparse(NAString &result,
			 PhaseEnum phase,
			 UnparseFormatEnum form,
			 TableDesc * tabId) const
{
  NABoolean first = TRUE;
  // MVs -- 

  NAString connectorText;

  if ((form == MVINFO_FORMAT) || (form == QUERY_FORMAT))
    connectorText = " AND ";
  else
    connectorText = " , ";

  result += "(";
  for (ValueId x = init(); next(x); advance(x))
    {
      if (first)
	first = FALSE;
      else
	result += connectorText;

      if (form == ERROR_MSG_FORMAT) // ERROR_MSG_FORMAT : don't print vid's
        {
          x.getItemExpr()->unparse(result,phase,EXPLAIN_FORMAT,tabId);
        }
      else if ((form == MVINFO_FORMAT) ||
	       (form == QUERY_FORMAT)) 
        {
	  // MVINFO_FORMAT or QUERY_FORMAT: don't print vid's
          x.getItemExpr()->unparse(result,phase,form,tabId);
        }
      else
        {
          char vidbuf[TEXT_DISPLAY_LENGTH];
          sprintf(vidbuf, "[%u]", (CollIndex)x);
          result += vidbuf;

          x.getItemExpr()->unparse(result,phase,form,tabId);
        }
    }
  result += ")";
} // ValueIdSet::unparse

void ValueIdSet::print(FILE* ofd, const char* indent, const char* title,
                       CollHeap *c, char *buf) const
{
  BUMP_INDENT(indent);
  Space * space = (Space *)c;
  char mybuf[1000];

  snprintf(mybuf, sizeof(mybuf), "%s%s\n",NEW_INDENT,title);
  PRINTIT(ofd, c, space, buf, mybuf);

  for (ValueId x = init(); next(x); advance(x))
  {
    Int32 i = (Int32)((CollIndex) x); // valueid as an integer
    NAString unparsed(CmpCommon::statementHeap());

    if (x.getItemExpr())
      x.getItemExpr()->unparse(unparsed); // expression as ascii string

    snprintf(mybuf, sizeof(mybuf),"%4d: %s\n",i,(const char *) unparsed);
    PRINTIT(ofd, c, space, buf, mybuf);
  }
} // ValueIdSet::print()

void ValueIdSet::display() const	// To be called from the debugger
{
  print();
}

ex_expr::exp_return_type ValueIdList::evalAtCompileTime
(short addConvNodes, // (IN) : 1 to add conv nodes, 0 otherwise
 ExpTupleDesc::TupleDataFormat tf, // (IN) : tuple format of resulting expr(s)
 char* resultBuffer, // (INOUT): tuple buffer of resulting expr(s)
 ULng32 resultBufferLength, // (IN): length of the result buffer
 Lng32 *length,       // (OUT) : length of 1st result expr
 Lng32 *offset,        // (OUT) : offset of 1st result expr
 ComDiagsArea *diagsArea
) const
{
  // set up binder/generator stuff so expressions could be generated.
  BindWA       bindWA(ActiveSchemaDB(), CmpCommon::context());
  Generator    generator(CmpCommon::context());
  ExpGenerator expGen(&generator);
  generator.appendAtEnd();

  // Do not perform pcode optimizations for expressions being evaluated at
  // compile-time.  This is mainly being done to address a native expressions
  // bug.  However, there is no need to optimize in this case.
  Int16 savedPCodeMode = (Int16) expGen.getPCodeMode();
  expGen.setPCodeMode(savedPCodeMode & ~(ex_expr::PCODE_LLO));

  generator.setBindWA(&bindWA);
  generator.setExpGenerator(&expGen);
  FragmentDir * compFragDir = generator.getFragmentDir();

  // create the fragment (independent code space) for this expression
  CollIndex myFragmentId = compFragDir->pushFragment(FragmentDir::MASTER);

  // space where compile-time expr(s) will be generated
  Space * space = generator.getSpace();

  // allocate a work cri desc to encode expr(s). It has
  // 3 entries: 0, for consts. 1, for temps. 2, for the encoded expr(s).
  ex_cri_desc * workCriDesc = new(space) ex_cri_desc(3, space);
  short keyAtpIndex   = 2;  // where the encoded expr(s) will be built

  ULng32 encodedKeyLen;
  ex_expr * keExpr = 0;
  ValueIdList tempValueIdList;

  // call ExpGenerator::generateContiguousMoveExpr to create an ex_expr
  // to "assign" expr(s) into the resultBuffer
  expGen.generateContiguousMoveExpr
    (*this, addConvNodes, 0, keyAtpIndex, tf, encodedKeyLen, &keExpr,
     NULL, ExpTupleDesc::SHORT_FORMAT, NULL, &tempValueIdList,
     0, NULL, TRUE /*disable const folding*/);

  if(encodedKeyLen > resultBufferLength)
  {
    generator.removeAll();
    return ex_expr::EXPR_ERROR;
  }

  // create a DP2 expression and initialize it with the expr(s)
  ExpDP2Expr * keyEncodeExpr = new(space) ExpDP2Expr(keExpr,
                                                     workCriDesc,
                                                     space);
  keyEncodeExpr->getExpr()->fixup(0,0,0,space,STMTHEAP, FALSE, NULL);

  atp_struct * workAtp = keyEncodeExpr->getWorkAtp();
  workAtp->getTupp(keyAtpIndex).setDataPointer(resultBuffer);

  // set the diagsArea for the caller to get warnings
  if (workAtp->getDiagsArea() != diagsArea)
     workAtp->setDiagsArea(diagsArea);
  // evaluate the expr(s) into resultBuffer
  ex_expr::exp_return_type rc =
    keyEncodeExpr->getExpr()->eval(workAtp, 0, STMTHEAP);

  // constant folding caller (ValueIdList::evaluateTree) wants to get the
  // length, offset of 1st expr because it assumes it has only one element.
  // other callers (CacheData::backpatchConstParams) are not interested in
  // length, offset and these two parameters default to NULL addresses.
  if (rc != ex_expr::EXPR_ERROR && (length || offset)) {
    const ValueId & tempValueId =
      tempValueIdList[0].getItemExpr()->getValueId();
    Attributes *attr = generator.getMapInfo(tempValueId)->getAttr();
    if (length) {
      if (attr->getVCIndicatorLength() > 0) {
        char temp[8];
        str_cpy_all(temp, resultBuffer + attr->getVCLenIndOffset(),
                    attr->getVCIndicatorLength());
        if (attr->getVCIndicatorLength() == sizeof(short))
          *length = *(short *)temp;
        else
          *length = *(Lng32 *)temp;
      }
      else {
        *length = attr->getLength();
      }
    }
    if (offset) *offset = attr->getOffset();
  }

  // Restore pcode mode.
  expGen.setPCodeMode(savedPCodeMode);

  generator.removeAll();
  return rc;

}

// Used by constant folding. Calls the executor evaluator. The parameters are:
// 1.- The root of the expression tree to evaluate. It is assumed that it
// contains constants only.
// 2.- encodedKeyBuffer, which is a pointer to the result.
// 3.- The length of the result
// 4.- The position in which the result starts
short ValueIdList::evaluateTree( const ItemExpr * root,
				 char * encodedKeyBuffer,
                                 ULng32 encodedKeyLength,
				 Lng32 *length,
				 Lng32 *offset,
				 ComDiagsArea *diagsArea)
{
  // Let's start with a list of size 4 rather than resizing continuously (?)
  ValueIdList encodedValueIdList(4);
  encodedValueIdList.insert(root->getValueId());

  if (encodedValueIdList.evalAtCompileTime
      (1, ExpTupleDesc::SQLMX_KEY_FORMAT, encodedKeyBuffer, encodedKeyLength,
       length, offset, diagsArea) == ex_expr::EXPR_ERROR)
    return -1;
  else
    return 0;
}

// This function assumes that the tree rooted at ch contains constants only.
// Parent is the parent of ch and childNumber is the number of ch. The function
// computes the value represented by the subtree rooted at ch and puts
// the new value in the position of ch.	Used by constant folding.
Lng32 ValueIdList::evaluateConstantTree( const ValueId &parent,
				        const ValueId & ch,
					Int32 childNumber,
					ItemExpr ** outItemExpr,
					ComDiagsArea *diagsArea)
{
  // eval requires CASE to be IF_THEN_ELSE's parent
  if( (ch.getItemExpr()->getOperatorType() == ITM_IF_THEN_ELSE) &&
      (parent.getItemExpr()->getOperatorType() != ITM_CASE) )
      return 0;

  #define RESULT_SIZE 1000
  Lng32 length;
  char value[RESULT_SIZE];

  Lng32 offset;

  if (outItemExpr)
    *outItemExpr = NULL;

  // Compute the value represented by the subtree rooted at ch
  Int32 error = 0;
  error = evaluateTree(ch.getItemExpr(), value, RESULT_SIZE, &length,
		       &offset, diagsArea);
  if(error)
    return error;

  const NAType & type = ch.getType();

  NAString text(CmpCommon::statementHeap());
  ItemExpr *item;

  if ( type.getTypeQualifier() != NA_CHARACTER_TYPE ) {

    Int32 index = 0;
  
    value[length + offset] = 0;
  
    // See how many positions the result will take in the display
    // long t = NAType::getDisplayLength(type.getFSDatatype(),
    Lng32 t = type.getDisplayLength(type.getFSDatatype(),
  		    length,
  		    type.getPrecision(),
  		    type.getScale(),
  		    0);
  
  
    char *result = new  char[t + 1];
    CMPASSERT( result != NULL );
    memset( result, ' ', t );
  
    // Get the ASCII representation
    ex_expr::exp_return_type retcode =
      convDoIt(value + offset,
               length,
               (short)type.getFSDatatype(),
               type.getPrecisionOrMaxNumChars(),
               type.getScaleOrCharset(),
               result,
               t,                          // target length
               REC_BYTE_F_ASCII,           // target type
               0,                          // no char limit
               SQLCHARSETCODE_ISO88591,    // ISO 8859-1
               NULL,                       // no vc length
               0,                          // not a varchar
               CmpCommon::statementHeap(), // heap
               0,                          // don't pass a diags area
               CONV_UNKNOWN_LEFTPAD,       // left pad with zeroes
               NULL,                       // no conv error flag
               CONV_ALLOW_INVALID_CODE_VALUE); // non ISO characters are lost,
                                               // see Bugzilla 2938
  
    // CMPASSERT(retcode == ex_expr::EXPR_OK); //Called by constant folding, no need to panic
    if ( retcode != ex_expr::EXPR_OK )
       return (-1);
  
    result[t] = 0;
  
    // Create a new node containing the result
    text = result;

    delete [] result;

    item = new (CmpCommon::statementHeap())
      SystemLiteral((NAType *) &type, value, offset + length, &text);

  } else {

  // for CHARACTER typed result, do not convert to ASCII format, because
  // the result contained in 'value' is in the right format already.
  //
  // Besides, we will have inconsistent result if the conversion did 
  // happen for UCS2 'ab': 
  //       SystemLiteral.text_ is 'ab' and 
  //       type.Unicode=UCS2.
  //
  // The correct representation should be: 
  //       stemLiteral.text_ is 'a\0b\0' and
  //       type.Unicode=UCS2.

   
    const CharType& ctype = (CharType&)type;

    NAString strval(value+offset, length);
    item = new (CmpCommon::statementHeap())
      SystemLiteral(strval, ctype.getCharSet(),ctype.getCollation(), 
                            ctype.getCoercibility());
  }

  item->synthTypeAndValueId();

  // Replace the subtree just computed with the node containing the result
  if (childNumber >= 0)
    parent.getItemExpr()->setChild(childNumber,item);

  if (outItemExpr)
    *outItemExpr = item;

  return error;

}

////////////////////////////////////////////////////////////////////////////
// Tells us if some sort of algebraic simplification can be done in a tree.
// Used by constant folding. The parameters are:
// 1.- Points to the root of the tree in question
// 2.- Parent of node pointed to by parameter 1
// 3.- The index of one of the childs of parameter 1. If it is a constant,
// then some sort of simplification may be done
// 4.- The index of the node represented by parameter 1
// 5.- Indicates which of the children of the node pointed by parameter 1
//     will not occupy its place after the simplification is done
////////////////////////////////////////////////////////////////////////////
NABoolean ValueIdList::canSimplify(ItemExpr *itemExpr,
				   const ValueId &parent,
				   Int32 i,
				   Int32 childNumber,
				   Int32 &moved)
{
  char value[4];
  Int32 length;
  ItemExpr *item;
  NAString tempStr(CmpCommon::statementHeap());

  if (itemExpr->child(i)->getOperatorType() != ITM_CONSTANT) {
    return FALSE;
  }

  ItemExpr *tempItem1 = itemExpr->child(i);
  ConstValue *c = (ConstValue *) tempItem1;

  short type = c->getType()->getFSDatatype();
  if (!((type >= REC_MIN_NUMERIC && type <= REC_MAX_NUMERIC) ||
	tempItem1->getValueId().getType().getTypeQualifier() == NA_BOOLEAN_TYPE)) {
    return FALSE;
  }

  if ((type >= REC_MIN_NUMERIC && type <= REC_MAX_NUMERIC)&&
    (tempItem1->getValueId().getType().getTypeQualifier() != NA_BOOLEAN_TYPE))
    {
      // must be exact numeric with scale of zero and max
      // precision is less than 10 (so it is a long and atoi can
      // convert that value).
      NumericType *ntyp = (NumericType *) c->getType();
      if ((! ntyp->isExact()) ||
	  (ntyp->getScale() != 0) ||
	  (ntyp->getPrecision() >= 10))
	return FALSE;
    }

  NAString strValue(c->getText(), CmpCommon::statementHeap());
  Int32 val;
  val = atoi(strValue);

  // We simplify the following cases:
  // <expr> AND FALSE => FALSE; FALSE AND <expr> => FALSE
  // <expr> AND TRUE  => expr;  TRUE  AND <expr> => <expr>
  // <expr> OR  TRUE  => TRUE;  <expr> OR TRUE   => TRUE
  // <expr> OR FALSE  => expr;  FALSE  OR <expr> => <expr>
  // NOT FALSE => TRUE
  // NOT TRUE  => FALSE
  // <expr> + 0 => <expr>; 0 + <expr> => <expr>
  // <expr> * 0 => 0; 0 * <expr> => 0
  // <expr> * 1 => <expr>; 1 * <expr> => <expr>
  // <expr> - 0 => <expr>
  // <expr> / 1 => <expr>

  // If we have an OR such that one of its operands is TRUE, replace the operator
  // with TRUE
  if ((itemExpr->getOperatorType() == ITM_OR) &&
      (val == 1)) {
    value[0]  = 1; value[1] = 0; value[2] = 0; value[3] = 0;
    length = 4;
    tempStr = "1";
    item = new (CmpCommon::statementHeap())
      SystemLiteral(itemExpr->synthesizeType(), value, length, &tempStr);

    item->synthTypeAndValueId();
    parent.getItemExpr()->setChild(childNumber,item);

    moved = i;
    return TRUE;
  }

  // We have an OR such that one of its operands is FALSE, then we may replace
  // the OR with its other child
  if ((itemExpr->getOperatorType() == ITM_OR) &&
      (val == 0)) {

    parent.getItemExpr()->setChild(childNumber,itemExpr->getChild(1 - i));
    moved = 1 - i;
    return TRUE;
  }

  // If we have an AND operator such that one of its operands is FALSE, replace
  // the operator with FALSE
  if ((itemExpr->getOperatorType() == ITM_AND) &&
      (val == 0)) {

    value[0]  = 0; value[1] = 0; value[2] = 0; value[3] = 0;
    length = 4;
    tempStr = "0";
    item = new (CmpCommon::statementHeap())
      SystemLiteral(itemExpr->synthesizeType(), value, length, &tempStr);
    item->synthTypeAndValueId();
    parent.getItemExpr()->setChild(childNumber,item);
    moved = i;
    return TRUE;
  }

  // We have an AND operator such that one of its operators is TRUE. We may replace
  // the operator with its other child
  if ((itemExpr->getOperatorType() == ITM_AND) &&
      (val == 1)) {

    parent.getItemExpr()->setChild(childNumber,itemExpr->getChild(1 - i));
    moved = 1 - i;
    return TRUE;
  }

  // We have a NOT operator with TRUE as the child. Replace the operator with
  // a FALSE node
  if ((itemExpr->getOperatorType() == ITM_NOT) &&
      (val == 1)) {

    value[0]  = 0; value[1] = 0; value[2] = 0; value[3] = 0;
    length = 4;
    tempStr = "0";
    item = new (CmpCommon::statementHeap())
      SystemLiteral(itemExpr->synthesizeType(), value, length, &tempStr);
    item->synthTypeAndValueId();
    parent.getItemExpr()->setChild(childNumber,item);
    moved = i;

    return TRUE;
  }

  // We have a NOT operator with FALSE as the child. Replace the operator with
  // a TRUE node
  if ((itemExpr->getOperatorType() == ITM_NOT) &&
      (val == 0)) {

    value[0]  = 1; value[1] = 0; value[2] = 0; value[3] = 0;
    length = 4;
    tempStr = "1";
    item = new (CmpCommon::statementHeap())
      SystemLiteral(itemExpr->synthesizeType(), value, length, &tempStr);
    item->synthTypeAndValueId();
    parent.getItemExpr()->setChild(childNumber,item);
    moved = i;

    return TRUE;
  }

  // We have a + such that one of its operands is 0, then we may replace
  // the + with its other child
  if ((itemExpr->getOperatorType() == ITM_PLUS) &&
      (val == 0)) {

    parent.getItemExpr()->setChild(childNumber,itemExpr->getChild(1 - i));
    moved = 1 - i;
    return TRUE;
  }

  // We have a * operator with 0 as a child. Replace the operator with
  // a 0 node
  if ((itemExpr->getOperatorType() == ITM_TIMES) &&
      (val == 0)) {

    item = new (CmpCommon::statementHeap()) SystemLiteral(0);
    item->synthTypeAndValueId();
    parent.getItemExpr()->setChild(childNumber,item);
    moved = i;

    return TRUE;
  }

  // We have an * operator such that one of its operators is 1. We may replace
  // the operator with its other child
  if ((itemExpr->getOperatorType() == ITM_TIMES) &&
      (val == 1)) {

    parent.getItemExpr()->setChild(childNumber,itemExpr->getChild(1 - i));
    moved = 1 - i;
    return TRUE;
  }

  // We have the case <expr> - 0
  if (itemExpr->getOperatorType() == ITM_MINUS) {

    if (i != 1)
      return FALSE;

    if (val == 0) {
      parent.getItemExpr()->setChild(childNumber,itemExpr->getChild(0));
      moved = 0;
      return TRUE;
    }
  }

  // We have the case <expr> / 1
  if (itemExpr->getOperatorType() == ITM_DIVIDE) {

    if (i != 1)
      return FALSE;

    if (val == 1) {
      parent.getItemExpr()->setChild(childNumber,itemExpr->getChild(0));
      moved = 0;
      return TRUE;
    }
    else {
      // do not change division to multiplication, if this div is
      // to downscale the result and rounding is to be done. Div to
      // round off the value does special processing which cannot be
      // replaced by a multiplication.
      if ((((BiArith*)itemExpr)->getDivToDownscale()) &&
	  (((BiArith*)itemExpr)->getRoundingMode() > 0))
	return FALSE;

      // see if we can replace "<expr> / val" by "<expr> * <newval>"
      // where <newval> is equal to "1 / <val>".
      Int64 temp = val;
      Int64 numerator = 1;
      Lng32 scale = 0;
      while (temp > 0)
	{
	  temp = temp / 10;
	  numerator = numerator * 10;
	  scale++;
	}
      if (numerator == 1)
	return FALSE;

      Int64 div = numerator / val;
      if ( div * val != numerator)
	return FALSE;

      NumericType * ntyp = new (CmpCommon::statementHeap())
	SQLNumeric(CmpCommon::statementHeap(), 4, scale, scale, TRUE, FALSE);
      Lng32 cval = (Lng32)div;
      char cvalStr[20];
      cvalStr[0] = '.';
      str_itoa(cval, &cvalStr[1]);
      NAString nas(cvalStr, CmpCommon::statementHeap());
      item = new (CmpCommon::statementHeap())
	SystemLiteral(ntyp, &cval, 4, &nas);
      item->synthTypeAndValueId();

      BiArith * ba =  new (CmpCommon::statementHeap())
	BiArith(ITM_TIMES, itemExpr->getChild(0)->castToItemExpr(), item);
      ba->synthTypeAndValueId();

      parent.getItemExpr()->setChild(childNumber,ba);

      moved = 0;
      return TRUE;
    }
  }

  return FALSE;
}

// Evaluate all constant subexpressions of the tree whose root is ch; after
// this routine is called, all subtrees that contain constants only will
// be replaced by the corresponding equivalent constant.
// Parent is the parent of ch and childNumber is the number of ch.
//
// RETURN:
//    TRUE, if 'ch' contains all constants
//    FALSE, if 'ch' doesn't contain all constants
//    -1, if error during const expr evaluation
//
//    If evalAllConsts is TRUE and 'ch' contains an all const expr, then that
//    expr is evaluated. If outAllConstsItemExpr is passed in, then on return
//    it contains a ConstValue(SystemLiteral) node representing the result of
//    the const expression.
Int32 ValueIdList::evaluateExpr( const ValueId & parent,
			       const ValueId & ch,
			       Int32 childNumber,
			       NABoolean simplifyExpr,
			       NABoolean evalAllConsts,
			       ItemExpr ** outAllConstsItemExpr,
			       ComDiagsArea *diagsArea)
{
  ItemExpr *itemExpr;

  itemExpr = ch.getItemExpr();
  Int32 nc = itemExpr->getArity();
  Int32 op = itemExpr->getOperatorType();

  if (outAllConstsItemExpr)
    *outAllConstsItemExpr = itemExpr;

  // No aggregate or sequence functions since their arguments are
  // represented as constants when they are not. Ditto for translations.
  // Null-instantiated constants aren't constant either. Should
  // really do a cover test here. Since it handles this issue now
  // and in the future.
  if (itemExpr->isAnAggregate() ||
      itemExpr->isASequenceFunction() ||
      (op == ITM_INSTANTIATE_NULL) ||
      (op == ITM_TRANSLATE) ||
      (op == ITM_ROWSETARRAY_INTO)) {
    return FALSE;
  }

  if (itemExpr->constFoldingDisabled())
    {
      ItemExpr *outExpr = NULL;

      for (Int32 i = 0; i < nc; i++)
	{
	  Int32 rc =
	    evaluateExpr(ch,itemExpr->child(i)->getValueId(),i,
			 simplifyExpr, evalAllConsts, &outExpr,
			 diagsArea);
	  if ((rc < 0) ||
	      (diagsArea && diagsArea->getNumber() > 0))
  	    return -1;
	  else if ((outExpr) &&
		   (evalAllConsts))
	    {
	      itemExpr->child(i) = outExpr;
	    }
	}
      return FALSE;
    }

  // If the tree has no subtrees, we just say if it is a constant or not.
  if (nc == 0) {
    if (itemExpr->getOperatorType() != ITM_CONSTANT) {
      return FALSE;
    }
    else {
      ConstValue *c = (ConstValue *) itemExpr;
      if (c->isNull()) {  // not dealing with NULLs
        return FALSE;
      }

      return TRUE;
    }
  }

  // Assume by now that the tree rooted at ch contains constants only
  Int32 allConstants = TRUE;

  const Int32 constantTreeArrayDim = 2;
  Int32 constantTreeArray[constantTreeArrayDim];
                             //  You probably don't like constants, but
                             // no tree should have more than 2 children.

  Int32 *constantTree = 0;

  if ( nc > constantTreeArrayDim ) {
    constantTree = new (CmpCommon::statementHeap()) Int32[nc];
  } else
    constantTree = constantTreeArray;

  Lng32 i = 0;
  for (; i < nc; i++) {

    // We call this routine recursively; therefore after this call the
    // subtree rooted at child number i will not contain subtrees that
    // contain constants only, since all of them would have been evaluated;
    // We store in an array a boolean value that tells us if this subtree
    // contains constants only, or not
    constantTree[i] = evaluateExpr(ch,itemExpr->child(i)->getValueId(),i,
				   simplifyExpr, FALSE, NULL, diagsArea);
    if (constantTree[i] < 0) {
      Int32 x = constantTree[i];
      if ( nc > constantTreeArrayDim )
	NADELETEBASIC(constantTree, CmpCommon::statementHeap());
      return x;
    }

    // The variable allConstants tells us if the tree rooted at ch consists
    // of constants only so far
    allConstants = (allConstants && constantTree[i]);

  }

  // If we only contain constants, we indicate so and we are done
  if (allConstants) {
    if ( nc > constantTreeArrayDim )
      NADELETEBASIC(constantTree, CmpCommon::statementHeap());

    if (evalAllConsts)
      {
	ValueId dummy;

        Lng32 error = evaluateConstantTree( dummy, itemExpr->getValueId(), -1,
					   outAllConstsItemExpr, diagsArea );
	if (error) // -1
	  return error;
      }

    return TRUE;
  }

  // Otherwise, we limit ourselves to evaluate those subtrees that contain
  // constants only. We indicate that the tree rooted at ch does not only
  // contain constants
  for (i = 0; i < nc; i++) {

    if (constantTree[i]) {

      // Evaluate the tree since we know it only contains constants
      if (itemExpr->child(i)->getArity() != 0) {
        Int32 error = 0;
        error = evaluateConstantTree(ch,itemExpr->child(i)->getValueId(),i,NULL,
				     diagsArea);
        if (error) {
          if ( nc > constantTreeArrayDim )
             NADELETEBASIC(constantTree, CmpCommon::statementHeap());
          return error;
        }
      }

      if (simplifyExpr) {
	Int32 moved;
	// Find out if some sort of simplification can be done; if
	// so, we do it and let know the parent of this subtree
	// whether the result contains all constants or not.
	if (canSimplify(itemExpr, parent, i, childNumber, moved)) {
	  if ( nc > constantTreeArrayDim )
	    NADELETEBASIC(constantTree, CmpCommon::statementHeap());
	  return (moved == i);
	}
      }
    }
  }

  if ( nc > constantTreeArrayDim )
    NADELETEBASIC(constantTree, CmpCommon::statementHeap());

  return FALSE;

}
NABoolean ValueIdList::hasVarChars() const
{
   ValueIdList list = *this;

   for (CollIndex i = 0; i < list.entries(); i++)
   {
     ValueId vid = list[i];

     if (vid.getType().getVarLenHdrSize()>0)
     {
       return TRUE;
     }
   }
   return FALSE;
}

// Simplify subexpressions that contain constants only; for
// example, if the valueIdList looks like x < 3 + 4
// then the list returns as x < 7 and the function result will be FALSE.
// If we see "1=1 and 2=2",
// then the list returns as "1." and the function result will be TRUE.
// If error, the list returned will be empty!
NABoolean ValueIdList::constantFolding()
{
  ItemExpr *tempItem1;
  ConstValue *c;
  Int32 constValue;

  ValueIdList list = *this;
  clear();

  // visit each subtree
  ItemExpr *item = NULL;

  // Tells if the whole expression is TRUE
  NABoolean allTrue = TRUE;

  for (CollIndex i = 0; i < list.entries(); i++)
  {
    const ValueId & vid = list[i];

    // If this constant changes to 1, then we have a TRUE boolean constant as result;
    // if it changes to 0, we have a FALSE value. Otherwise we have any other expression.
    constValue = -1;

    // We create a temporary constant to serve as a dummy tree root
    item = new (CmpCommon::statementHeap()) SystemLiteral();
    item->synthTypeAndValueId();
    item->setChild(0,vid.getItemExpr());

    if ((vid != NULL_VALUE_ID) && (vid.getItemExpr()->getOperatorType() == ITM_OR))
    {
      ValueIdList eqList;
      NABoolean status = vid.getItemExpr()->convertToValueIdList(eqList,NULL, ITM_OR);

      if(!status && eqList.entries() > CmpCommon::getDefaultNumeric(MAX_EXPRS_USED_FOR_CONST_FOLDING))
      {
        // if the number of branches in this OR predicate are more than max_exprs_used_for_constant_folding
        // we will not try to simplify the expression. Add the original expression to the list and continue
        // to evaluate next predicate
        insert(vid);
        // also since we have not done constant folding we don;t know if that expression evaluates to TRUE or not
        // so to be on the safe side we will make allTrue = FALSE
        allTrue = FALSE;
        continue;
      }
    }


    Int32 allConstants;

    // We evaluate the tree rooted at item. If it consists of constants only,
    // evaluateExpr will let us know; otherwise all subtrees that contain only
    // constants will be evaluated
    const ValueId & tempVal = item->getValueId();
    allConstants = evaluateExpr( tempVal, vid, 0 );
    if (allConstants < 0) {
      clear();
      return FALSE;
    }

    // If we only have constants, do the evaluation
    if (allConstants) {

      if (item->child(0)->getArity() > 0) {
        Int32 error = 0;
        const ValueId & tempVal1 = item->getValueId();
        error = evaluateConstantTree( tempVal1, vid, 0, NULL );
        if (error) {
	  clear();
	  return FALSE;
        }
      }

      tempItem1 = item->child(0);
      c = (ConstValue *) tempItem1;

      constValue = atoi(c->getText());

      if (constValue == 0) {			// the FALSE value
	clear();
        insert(item->child(0)->getValueId());
	return FALSE;
      }

      if (constValue != 1) {
        allTrue = FALSE;
      }

    }
    else {
      allTrue = FALSE;
    }

    // Get the simplified tree inserted in resulting list; unless it is a TRUE
    // node.
    if (constValue != 1) {
      insert(item->child(0)->getValueId());
    }

  }

  // If the whole expression evaluates to TRUE, return TRUE
  if (allTrue) {
    clear();
    insert(item->child(0)->getValueId());
  }

  return allTrue;
}

// ---------------------------------------------------------------------
// findCommonElements()
//
// Find common elements between "this" and "other". Remove all
// value ids that are not common elements from "this" (so that
// "this" contains the common subexpressions).
// ---------------------------------------------------------------------
void ValueIdList::findCommonElements(const ValueIdSet &other)
{
  NAList<CollIndex> listCopy(CmpCommon::statementHeap());
  listCopy.clear();
  CollIndex i;

  for (i = 0; i < entries() ; i++)
  {
    // copy the index of the element that is not contained in the "other" set
    // this should be removed later
    if (!other.contains((*this)[i]) )
      listCopy.insert(i);
  }
  // now go and remove all the elements indexed by i. Start from the back of the
  // list. If all elements of this exist in other, then return.

  for (i =  listCopy.entries(); i > 0 ;i--)
    removeAt(listCopy[i-1]);
}

// ---------------------------------------------------------------------
// findCommonElementsFromList()
//
// Find common elements between "this" and "other".
// ---------------------------------------------------------------------
ValueIdList ValueIdList::findCommonElementsFromList(const ValueIdList &other)
{
  ValueIdList listCopy;
  listCopy.clear();
  CollIndex i, j;

  // We use "other' as the main loop, as it is important to maintain the order
  // of the other in "this"
  for (i = 0; i < other.entries() ; i++)
  {
    ValueId currentOtherId = other[i];

    // Find all occurences of "other" in "this"

    for (j = 0; j < entries(); j++)
    {
      // copy the index of the element that is not contained in the "other" set
      // this should be removed later
      if ( (*this)[j] ==  currentOtherId )
	listCopy.insert(currentOtherId);
    }
  }

  return listCopy;
}

// ***********************************************************************
// Methods for class ValueIdMap
// ***********************************************************************

ValueIdMap::ValueIdMap(const ValueIdSet &identity)
{
  for (ValueId x = identity.init();
       identity.next(x);
       identity.advance(x))
    {
      topValues_.insert(x);
      bottomValues_.insert(x);
    }
}

NABoolean ValueIdMap::operator == (const ValueIdMap &other) const
{
  // shortcut: identical lists of value ids
  if (topValues_ == other.topValues_ AND
      bottomValues_ == other.bottomValues_)
    return TRUE;

  return FALSE; // for now
}

void ValueIdMap::remapTopValue(const ValueId &newTopValue,
			       const ValueId &bottomValue)
{
  CollIndex ix = bottomValues_.index(bottomValue);

  if (ix != NULL_COLL_INDEX)
    topValues_[ix] = newTopValue;
  else
    addMapEntry(newTopValue,bottomValue);
}

void ValueIdMap::remapBottomValue(const ValueId &topValue,
			          const ValueId &newBottomValue)
{
  CollIndex ix = topValues_.index(topValue);

  if (ix != NULL_COLL_INDEX)
    bottomValues_[ix] = newBottomValue;
  else
    addMapEntry(topValue,newBottomValue);
}

void ValueIdMap::addMapEntry(const ValueId &newTopValue,
			     const ValueId &newBottomValue)
{
  topValues_.insert(newTopValue);
  bottomValues_.insert(newBottomValue);
}

void ValueIdMap::mapValueIdUp(ValueId &topValue,
			      const ValueId &bottomValue) const
{
  CollIndex ix = bottomValues_.index(bottomValue);

  if (ix != NULL_COLL_INDEX)
    topValue = topValues_[ix];
  else
    topValue = bottomValue;
}

void ValueIdMap::mapValueIdUpWithIndex(ValueId &topValue,
			      const ValueId &bottomValue, CollIndex i) const
{
  CollIndex currEntry = i;
  CollIndex result    = i;
  NABoolean found = FALSE;
  CollIndex bottomEntries = bottomValues_.entries();

  while (currEntry != NULL_COLL_INDEX)
  {
    // get the next used entry of bottomValues_ from the index currEntry_
    // This methos is the const version of usedEntry
    if (bottomValues_.constEntry(currEntry) == bottomValue)
	{
	currEntry = result;
	found = TRUE;
	break;
	}
      else
	{
	  // advance to the next entry
	  result++;
	  currEntry = bottomValues_.getUsage(currEntry);
	}
    }

  if (found)
    topValue = topValues_[currEntry];
  else
    topValue = bottomValue;
}


void ValueIdMap::mapValueIdDown(const ValueId &topValue,
				ValueId &bottomValue) const
{
  CollIndex ix = topValues_.index(topValue);

  if (ix != NULL_COLL_INDEX)
  {
    bottomValue = bottomValues_[ix];
	return;
  }
  //++ Triggers - 
  // If a generic update has a trigger,
  // topValues_ might contain VEG_REFERENCEs
  for (ix=0; ix < topValues_.entries() ; ix++)
  {
    ItemExpr *pred = topValues_[ix].getItemExpr();

	if (pred->getOperatorType() == ITM_VEG_REFERENCE)
	{
	  const ValueIdSet & VEGGroup =((VEGReference *)pred)->getVEG()->getAllValues();
	  if (VEGGroup.contains(topValue))
	  {
		bottomValue = bottomValues_[ix];
		return;
	  }
	}
  }
  //-- Triggers - 
  bottomValue = topValue;
}

void ValueIdMap::mapValueIdListUp(ValueIdList &topValues,
				  const ValueIdList &bottomValues) const
{
  CMPASSERT(topValues.entries() == 0);

  for (CollIndex i = 0; i < bottomValues.entries(); i++)
    {
      ValueId u;
      mapValueIdUp(u,bottomValues[i]);
      topValues.insert(u);
    }
}

void ValueIdMap::mapValueIdListDown(const ValueIdList &topValues,
				    ValueIdList &bottomValues) const
{
  CMPASSERT(bottomValues.entries() == 0);

  for (CollIndex i = 0; i < topValues.entries(); i++)
    {
      ValueId l;
      mapValueIdDown(topValues[i],l);
      bottomValues.insert(l);
    }
}

void ValueIdMap::mapValueIdSetUp(ValueIdSet &topValues,
				 const ValueIdSet &bottomValues) const
{
  CMPASSERT(topValues.isEmpty());

  for (ValueId l = bottomValues.init();
       bottomValues.next(l); bottomValues.advance(l))
    {
      ValueId u;
      mapValueIdUp(u,l);
      topValues.insert(u);
    }
}

void ValueIdMap::mapValueIdSetDown(const ValueIdSet &topValues,
				   ValueIdSet &bottomValues) const
{
  CMPASSERT(bottomValues.isEmpty());

  for (ValueId u = topValues.init();
       topValues.next(u); topValues.advance(u))
    {
      ValueId l;
      mapValueIdDown(u,l);
      bottomValues.insert(l);
    }
}

void ValueIdMap::rewriteValueIdUp(ValueId &topValue,
				  const ValueId &bottomValue)
{
  topValue = bottomValue.getItemExpr()->mapAndRewrite(*this,FALSE);
}

void ValueIdMap::rewriteValueIdDown(const ValueId &topValue,
				    ValueId &bottomValue)
{
  bottomValue = topValue.getItemExpr()->mapAndRewrite(*this,TRUE);
}

void ValueIdMap::rewriteValueIdUpWithIndex(ValueId &topValue,
				  const ValueId &bottomValue, CollIndex i)
{
  topValue = bottomValue.getItemExpr()->mapAndRewriteWithIndx(*this,i);
}

void ValueIdMap::rewriteValueIdListUp(ValueIdList &topValues,
				      const ValueIdList &bottomValues)
{
  CMPASSERT(topValues.entries() == 0);

  for (CollIndex i = 0; i < bottomValues.entries(); i++)
    {
      ValueId u;
      rewriteValueIdUp(u,bottomValues[i]);
      topValues.insert(u);
    }
}

void ValueIdMap::rewriteValueIdListUpWithIndex(ValueIdList &topValues,
				      const ValueIdList &bottomValues)
{
  CMPASSERT(topValues.entries() == 0);

  for (CollIndex i = 0; i < bottomValues.entries(); i++)
    {
      ValueId u;
      rewriteValueIdUpWithIndex(u,bottomValues[i],i);
      topValues.insert(u);
    }
}

void ValueIdMap::rewriteValueIdListDown(const ValueIdList &topValues,
					ValueIdList &bottomValues)
{
  CMPASSERT(bottomValues.entries() == 0);

  for (CollIndex i = 0; i < topValues.entries(); i++)
    {
      ValueId l;
      rewriteValueIdDown(topValues[i],l);
      bottomValues.insert(l);
    }
}

void ValueIdMap::rewriteValueIdSetUp(ValueIdSet &topValues,
				     const ValueIdSet &bottomValues)
{
  CMPASSERT(topValues.isEmpty());

  for (ValueId l = bottomValues.init();
       bottomValues.next(l); bottomValues.advance(l))
    {
      ValueId u;
      rewriteValueIdUp(u,l);
      topValues.insert(u);
    }
}

void ValueIdMap::rewriteValueIdSetDown(const ValueIdSet &topValues,
				       ValueIdSet &bottomValues)
{
  CMPASSERT(bottomValues.isEmpty());

  for (ValueId u = topValues.init(); topValues.next(u); topValues.advance(u))
    {
      ValueId l;
      rewriteValueIdDown(u,l);
      bottomValues.insert(l);
    }
}

void ValueIdMap::flipSides()
{
  ValueIdList flipList = topValues_;
  topValues_ = bottomValues_;
  bottomValues_ = flipList;
}

void ValueIdMap::augmentForVEG(NABoolean addVEGPreds,
                               NABoolean addVEGRefs,
                               NABoolean compareConstants,
                               const ValueIdSet *topInputsToCheck,
                               const ValueIdSet *bottomInputsToCheck,
                               ValueIdSet *vegRefsWithDifferentConstants,
                               ValueIdSet *vegRefsWithDifferentInputs)
{
  // If a ValueIdMap maps one VEGReference x to another VEGReference y,
  // we may want to be able to map the corresponding VEGPredicates as
  // well. This method enables that by finding such pairs of VEGReferences
  // and augmenting the map with their VEGPredicates. The method can also
  // augment the map from VEGPredicates to VEGReferences.

  // NOTE: Before using this method, make sure it is applicable in
  // your case. What it does is somewhat questionable. We may have
  // a VEG(a,b,1) in the top values and a VEG(c,d,2) in the bottom
  // values. Replacing one VEGPred into another may or may not be
  // what we want. Furthermore, a,b,c may be local values, d may
  // be a characteristic input. Again, it is questionable whether
  // the rewrite is what's desired.

  // The method allows to restrict the rewrite somewhat:
  // - compareConstants requires top and bottom VEGPreds to
  //   have the same constant (or no constants at all)
  // - top/bottom inputs to check can be used to exclude
  //   VEGPreds that differ in the way they use inputs.

  // There are other issues that still may go wrong with this
  // method, maybe with predicates like COL1=COL2.

  CollIndex ne = topValues_.entries();

  for (CollIndex i=0; i<ne; i++)
    {
      ItemExpr *t = topValues_[i].getItemExpr();
      ItemExpr *b = bottomValues_[i].getItemExpr();
      OperatorTypeEnum to = t->getOperatorType();
      OperatorTypeEnum bo = b->getOperatorType();

      if (addVEGPreds &&
          to == ITM_VEG_REFERENCE &&
          bo == ITM_VEG_REFERENCE)
        {
          VEG *vegT = static_cast<VEGReference *>(t)->getVEG();
          VEG *vegB = static_cast<VEGReference *>(b)->getVEG();
          ValueId topPred(vegT->getVEGPredicate()->getValueId());

          if (! topValues_.contains(topPred))
            {
              NABoolean ok = TRUE;
              ValueId constT = vegT->getAConstant(TRUE);
              ValueId constB = vegB->getAConstant(TRUE);

              // check whether constants match
              // (or are both NULL_VALUE_ID)
              if (compareConstants)
                ok = (constT == constB);

              if (!ok && vegRefsWithDifferentConstants)
                *vegRefsWithDifferentConstants +=
                  vegT->getVEGReference()->getValueId();

              if (ok &&
                  ((topInputsToCheck && topInputsToCheck->entries() > 0) ||
                   (bottomInputsToCheck && bottomInputsToCheck->entries() > 0)))
                {
                  ValueIdSet topInputs;
                  ValueIdSet bottomInputs;

                  if (topInputsToCheck)
                    topInputs = *topInputsToCheck;
                  if (bottomInputsToCheck)
                    bottomInputs = *bottomInputsToCheck;

                  topInputs.intersectSet(vegT->getAllValues());
                  bottomInputs.intersectSet(vegB->getAllValues());
                  // if the caller provided inputs to check, we only
                  // rewrite VEGPreds if their VEGies refer to the
                  // same inputs (or if they don't refer to any inputs)
                  ok = (topInputs == bottomInputs);

                  if (!ok && vegRefsWithDifferentInputs)
                    *vegRefsWithDifferentInputs +=
                      vegT->getVEGReference()->getValueId();
                }

              if (ok)
                {
                  topValues_.insert(topPred);
                  bottomValues_.insert(vegB->getVEGPredicate()->getValueId());
                }
            }
        }

      if (addVEGRefs &&
          to == ITM_VEG_PREDICATE &&
          bo == ITM_VEG_PREDICATE)
        {
          ValueId topRef(static_cast<VEGPredicate *>(t)->
                         getVEG()->getVEGReference()->getValueId());

          if (! topValues_.contains(topRef))
            {
              topValues_.insert(topRef);
              bottomValues_.insert(
                   static_cast<VEGPredicate *>(b)->
                      getVEG()->getVEGReference()->getValueId());
            }
        }
    }
}

NABoolean ValueIdMap::normalizeNode(NormWA & normWARef)
{
  NABoolean t1,t2;

  // Transform both lists
  t1 = topValues_.normalizeNode(normWARef);
  t2 = bottomValues_.normalizeNode(normWARef);

  return (t1 || t2);
}

// Remove unused entries from the Map if they are no longer required
// based on the require values.
//
void ValueIdMap::removeUnusedEntries(const ValueIdSet &requiredValues, NABoolean matchWithTopValues)
{
  if (matchWithTopValues)
  {
    for (CollIndex ix=0; ix < topValues_.entries() ; ix++)
    {
      if(!requiredValues.contains(topValues_[ix])) 
      {
	topValues_.removeAt(ix);
	bottomValues_.removeAt(ix);
	ix--;
      }
    }
  }
  else
  {
    for (CollIndex ix=0; ix < bottomValues_.entries() ; ix++)
    {
      if(!requiredValues.contains(bottomValues_[ix])) 
      {
	topValues_.removeAt(ix);
	bottomValues_.removeAt(ix);
	ix--;
      }
    }
  }
}

// ***********************************************************************
// Constructor for ValueDesc
//
// Use the ActiveSchemaDB() consistently in ALL methods for ValueDescs
// and ValueIds.
// ***********************************************************************

ValueDesc::ValueDesc(ItemExpr *expr)
         : exprPtr_(expr), domId_(NULL)
{
  ActiveSchemaDB()->insertValueDesc(this);
}

// static ctor
ValueId ValueDesc::create(ItemExpr *expr, const NAType *type, CollHeap *h)
{
  ValueDesc *vdesc = new (h) ValueDesc(expr);
  vdesc->setDomainDesc(new (h) DomainDesc(ActiveSchemaDB(), *type));
  return vdesc->getValueId();
}

// ***********************************************************************
// Methods for class ValueDescArray
// ***********************************************************************
void ValueDescArray::print(FILE* ofd, const char* indent, const char* title,
			   NABoolean dontDisplayErrors) const
{
#ifndef NDEBUG
  BUMP_INDENT(indent);
  fprintf(ofd, "%s%s %p (%d entries)\n",
	       NEW_INDENT, title, this, entries());

  for (CollIndex i = 0; i < entries(); i++)
    {
      ValueDesc *vd = at(i);
      if (vd && used(i))
	{
	  Int32 otyp = 0;
          NAString unparsed(CmpCommon::statementHeap());
	  ItemExpr *ie = vd->getItemExpr();
	  if (ie)
	    {
	      CollIndex ivid = (CollIndex)vd->getValueId();
	      if (i != ivid)
		{
		  //fprintf(ofd, "%4d != %d: ERROR\n", i,
		  otyp = -1;
		  char cvid[TEXT_DISPLAY_LENGTH];
		  sprintf(cvid, "ERROR: vid %d", ivid);
		  unparsed = cvid;
		}
	      else
		{
		  otyp = ie->getOperatorType();
		  ie->unparse(unparsed);
		}
	    }
	  if (otyp > 0 || !dontDisplayErrors)
	    fprintf(ofd, "%4d %p %4d: %s\n", i, ie, otyp, unparsed.data());
	}
    }
#endif
} // ValueDescArray::print()

void ValueDescArray::display(NABoolean dontDisplayErrors) const
{
  print(stdout, DEFAULT_INDENT, "ValueDescArray", dontDisplayErrors);
}

/*static*/ void ValueDescArray::Display(NABoolean dontDisplayErrors)
{
  ActiveSchemaDB()->getValueDescArray().print(
    stdout, DEFAULT_INDENT, "Global ValueDescArray", dontDisplayErrors);
}

//--------------------------------------------------------
// Replace aggregates of type ITM_ONE_ROW(I,J,K) with (I,J,K);
// Generator does not know how to transmit
// ONE_ROW of columns from a child to parent. So do not
// ask for it. One Row check would be performed at the
// concerned GroupBy node.
//
// If we have a expression tree where ITM_ONE_ROW node is an
// interior node with arity one, replace it as well.
//--------------------------------------------------------
void ValueIdSet::replaceOneRowbyList( NormWA & normref )
{

  ValueId exprId;
  ValueIdSet replaceSet;
  for (exprId = init(); next(exprId); advance(exprId))
  {
    ItemExpr *thisIE = exprId.getItemExpr();
    if ( (thisIE->getArity() >= 1) &&
         (thisIE->containsOneRowAggregate()) )

    {
      ItemExpr *tfm = NULL;
      tfm = thisIE->transformOneRowAggregate( normref );
      CMPASSERT( tfm );

      exprId.replaceItemExpr(tfm);
      // redrive type synthesis for both self and child nodes
      tfm->synthTypeAndValueId(TRUE, TRUE);

      tfm = tfm->transformMultiValuePredicate(FALSE);
      if (tfm) // transform was needed
      {
        exprId.replaceItemExpr(tfm);
	// redrive type synthesis for both self and child nodes
        tfm->synthTypeAndValueId(TRUE, TRUE);
        tfm->convertToValueIdSet(replaceSet, NULL, ITM_AND, FALSE);
        subtractElement(exprId);
      }
      else
      {
        // this itemexpression tree contains a onerow aggregate node, yet no
        // transformation was done. Remove the aggregate node from the tree so
        // that the relational expression won't ask for a onerow valueid.
        tfm =  thisIE->removeOneRowAggregate( thisIE, normref );
        CMPASSERT( tfm );
        exprId.replaceItemExpr(tfm);
        tfm->convertToValueIdSet(replaceSet, NULL, ITM_AND, FALSE);
        subtractElement(exprId);
      }
    } // if contains oneRow aggregate.

    // Even though some scalar aggregates do not contain the node
    // ITM_ONE_ROW, they implicitly contain them. There is special logic
    // in Subquery::transformNode to identify them. If we are seeing one
    // of those here, they need to be retransformed.

    // Go through the ItemExpression Tree, anchored by the Variable
    // 'ThisIE' and retransform it; case Id: 10-990122-0552

    if ( thisIE->markPathToUnTransformedNode() )

      // Returns TRUE if any node in the whole tree anchored by thisIE
      // contains an untransformed node.
      //
      // Marks the entire path, from this node to the untransformed node, as
      // untransformed.
      //
      // I believe that this modification of what was originally coded
      // handles the case when the item expression tree is deep -- that
      // is, when there is one or more nodes betwen thisIE and the
      // currently untransformed node.  See his comment below:
      //
      // $$$ This fix is temporary as it would only work if the item
      // $$$ expression tree is shallow. Need to do special processing if the
      // $$$ tree is deep.  .... and I have a deadline to keep -  1/29/99

      {
        CMPASSERT( ! thisIE->nodeIsTransformed() );
      }

    // If we don't mark the entire path as untransformed (from the
    // untransformed child to thisIE), when it comes time to transforming
    // thisIE, we won't walk down the tree to the untransformed node.
    // Look at the beginning of all ::transformNode() methods -- the first
    // thing that each one does is check to see if it's already
    // transformed.  If so, returns, without looking at its children.
    // Thus, if there are one or more nodes marked "transformed" between
    // thisIE (marked "UNtransformed") and the untransformed child node,
    // we won't transform that child node when we call
    // thisIE->transformNode().
    //
    // There is a general assumption in all other transformation code: if
    // any node in the tree is transformed, then its children have been
    // transformed already. 

  } // for

  addSet(replaceSet);
} // ValueIdSet::replaceOneRowbyList()

// is this list cacheable after this phase?
NABoolean ValueIdList::isCacheableExpr(CacheWA& cwa)
{
  CollIndex ix, limit = entries();
  for (ix = 0; ix < limit; ix++) {
    if (!((*this)[ix].getItemExpr()->isCacheableExpr(cwa))) {
      return FALSE;
    }
  }
  return TRUE;
}

ValueId ValueIdList::extractVEGRefForEquiPredicate(ValueId x) const
{
   ItemExpr* ie = x.getItemExpr();
   switch ( ie->getOperatorType() )
   {
      case ITM_EQUAL:
       {
         for (CollIndex i = 0; i < 2; i++)
          if ( contains(ie->child(i)->getValueId()) )
             return ie->child(i)->getValueId();

          break;
       }

      case ITM_VEG_PREDICATE:
      {
        VEG* veg = ((VEGPredicate*)ie)->getVEG();

        // Simple case first: the veg predicate refers to a VEG
        // that appears in the ValueIdList of this. 
        for (CollIndex i = 0; i < entries(); i++)
        {
           ItemExpr* iei = at(i).getItemExpr();

           if (iei->getOperatorType() == ITM_INSTANTIATE_NULL)
              iei=iei->child(0)->castToItemExpr();

           if ( iei->getOperatorType() == ITM_VEG_REFERENCE &&
              ((VEGReference*)iei)->getVEG() == veg )
           {
               return at(i);
           }
       }

       // Next we drill down the VEG predicate to see if there is a
       // VEG reference that appears in ValueIdList of this. We return 
       // that VEG reference when the lookup succeeds.
       //
       // The following loop is necessary to deal with a VEG predicate of form
       // D.A = VEGRef(T.A)
       // ValueIdList of this contains {VEGRef(T.A), VEGRef(T.B))
       // The MC join predicate is
       //  T.A = D.A and T.B = D.B
       const ValueIdList vegMembers(veg->getAllValues());

       for (CollIndex i=0; i<vegMembers.entries(); i++) {
          // we will be getting D.A, VegRef(T.A)
          ItemExpr* iei = vegMembers[i].getItemExpr();

          if (iei->getOperatorType() == ITM_INSTANTIATE_NULL)
             iei=iei->child(0)->castToItemExpr();

          if ( iei->getOperatorType() == ITM_VEG_REFERENCE &&
               this->contains(vegMembers[i])
             )
            return vegMembers[i];
          
       }

       break;
         
     }

     default:
       break;
   }

   return  NULL_VALUE_ID;
}

// change literals of a ValueIdList into ConstantParameters
void ValueIdList::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  // Iterate over all the values (scalar expressions) in this list.
  CollIndex ix, limit = entries();
  for (ix = 0; ix < limit; ix++) {
    ItemExpr *iePtr = at(ix).getItemExpr(); // original expr
    // apply normalizeForCache on the item expr
    ItemExpr *nePtr = iePtr->normalizeForCache(cwa, bindWA);
    // if normalizeForCache changed the original expr, update the list
    if (nePtr->castToItemExpr() != iePtr || nePtr->getValueId() != at(ix)) {
      at(ix) = nePtr->castToItemExpr()->getValueId();
    }
  }
}

ConstValue* ValueIdList::getConstant(ItemExpr* ie)
{
   switch (ie->getOperatorType()) 
     {
     case ITM_CACHE_PARAM:
       {
	 ConstantParameter*cp = (ConstantParameter*)ie;
	 return cp->getConstVal();
       }
       break;
       
     case ITM_CONSTANT:
       return (ConstValue*)ie;
       break;
       
     default:
       {
	 if ( ie->getOperatorType() == ITM_VEG_REFERENCE )
	   {
	     ValueIdSet result;
	     ie->findAll(ITM_CONSTANT, result, TRUE, FALSE);
	     
	     if ( result.entries() == 0 ) 
	       ie->findAll(ITM_CACHE_PARAM, result, TRUE, FALSE);
	     
	     if ( result.entries() == 1 ) 
	       {
		 for (ValueId vid= result.init();  result.next(vid); result.advance(vid)) 
		   {
		     ConstValue * constVal = getConstant(vid.getItemExpr());
		     return constVal;
		   } // for
	       } // if
	   } // if
       } // default
       break;
     } // switch
   
   return NULL;
}

void ValueIdList::convertToTextKey(const ValueIdList& keyList, NAString& result)
{
  result.resize(0);

  CollIndex i = 0;
  for (i = 0; i < entries(); i++) 
  {
     ItemExpr* ie = at(i).getItemExpr();
 
     ConstValue* constVal = getConstant(ie);
   
     if ( constVal ) {
       const NAType &type = keyList.at(i).getType();
       const NAType *constType = constVal->getType();

       NAString val = *constVal->getRawText();
       short len = 0;

       ///////////////////////////////////////////////////////////////////////
       // Value Layout:
       //    2 bytes total length     |  2 bytes null value, if nullable | data
       //    (null indicator length(2 bytes) + data length)   |  
       ////////////////////////////////////////////////////////////////////////
       if (type.supportsSQLnull())
	 {
	   len = 2;
	 }

       if (constVal->isNull())
	 {
	   if (type.supportsSQLnull())
	     {
	       result.append((char*)&len, sizeof(short));

	       short nullVal = -1;
	       result.append((char*)&nullVal, sizeof(short));
	     } 
	 }       
       else if ((val == "<min>") ||
		(val == "<max>"))
	 {
	   Lng32 bufLen = 
	     (type.isVaryingLen() ? 
	      (type.getNominalSize() + type.getVarLenHdrSize()) :
	      type.getNominalSize());
	   char * buf = new(CmpCommon::statementHeap()) char[bufLen+10];

	   NAString *mmVal = NULL;
	   if (val == "<min>")
	     {
	       type.minRepresentableValue(buf, &bufLen, &mmVal, 
					  CmpCommon::statementHeap());
	       
	       if ((type.getTypeQualifier() == NA_DATETIME_TYPE) ||
		   (type.getTypeQualifier() == NA_INTERVAL_TYPE))
		 {
		   // string value returned is of the form: DATE '2013-06-20' or
		   // INTERVAL '10' YEAR.
		   // Extract the actual string from the returned value.
		   Lng32 start = mmVal->index("'");
		   if (start > 0)
		     {
		       Lng32 end = mmVal->index("'", start+1);
		       if (end > 0)
			 {
			   *mmVal = (*mmVal)(start+1, (end-start-1));

			    if (type.getTypeQualifier() == NA_INTERVAL_TYPE)
			      {
				// prepend '-' to the output
				(*mmVal).prepend('-', 1);
			      }
			 }
		     }
		 }
               else if (type.getTypeQualifier() == NA_CHARACTER_TYPE)
                 {
                   // string literal looks like _ISO88591'abc' and is
                   // in UTF-8, what we want here is the actual character
                   // values in the type's character set
                   *mmVal = NAString(buf, bufLen);
                 }
	     }
	   else
	     {
	       type.maxRepresentableValue(buf, &bufLen, &mmVal, 
					  CmpCommon::statementHeap());

	       if ((type.getTypeQualifier() == NA_DATETIME_TYPE) ||
		   (type.getTypeQualifier() == NA_INTERVAL_TYPE))
		 {
		   // string value returned is of the form: DATE '2013-06-20' or
		   // INTERVAL '10' YEAR.
		   // Extract the actual string from the returned value.
		   Lng32 start = mmVal->index("'");
		   if (start > 0)
		     {
		       Lng32 end = mmVal->index("'", start+1);
		       if (end > 0)
			 {
			   *mmVal = (*mmVal)(start+1, (end-start-1));
			 }
		     }
		 }
               else if (type.getTypeQualifier() == NA_CHARACTER_TYPE)
                 {
                   // string literal looks like _ISO88591'abc' and is
                   // in UTF-8, what we want here is the actual character
                   // values in the type's character set
                   *mmVal = NAString(buf, bufLen);
                 }
	     }
	   
	   short vLen = 0;
	   if (type.isVaryingLen())
	     {
	       vLen = *(short*)mmVal->data();
	     }
	   else
	     {
	       vLen = mmVal->length();
	     }

	   len += vLen;
	   result.append((char*)&len, sizeof(short));
	   if (type.supportsSQLnull())
	     {
	       short nullVal = 0;
	       result.append((char*)&nullVal, sizeof(short));
	     }

	   if (type.isVaryingLen())
	     result.append(&mmVal->data()[type.getVarLenHdrSize()], vLen);
	   else
	     result.append(mmVal->data(), vLen);
	 }
       else
	 {
           short vLen = val.length();

	   if (constType->getTypeQualifier() == NA_INTERVAL_TYPE)
	     {
	       // In some code paths, the text may have "INTERVAL 'xxx' <qualifier>"
	       // junk around it so we have to strip that off. (Example: An equality
	       // predicate when query caching has been turned off via 
	       // CQD QUERY_CACHE '0'. Another example happens with BETWEEN, whether 
	       // or not query caching is turned off. See JIRA TRAFODION-3088 for
	       // that example.)
	       Lng32 start = val.index("'");
	       Lng32 minus = val.index("-");
	       if (start > 0)
	         {
	           Lng32 end = val.index("'", start+1);
	           if (end > 0)
	             {
	               val = val(start+1, (end-start-1));
	               if ((minus > 0) && (minus < start))  // '-' before the string part
	                 {
	                   // prepend '-' to the output
	                   val.prepend('-', 1);
	                 }
	               vLen = val.length();		         
	             }
	         }             
	     }
	   else if ((constType->getTypeQualifier()  == NA_NUMERIC_TYPE) &&
	       (((NumericType*)constType)->isExact()) &&
               (NOT ((NumericType*)constType)->isBigNum()) &&
	       (constType->getScale() > 0))
	     {
               // See how many positions the result will take in the display
               Lng32 t = constType->getDisplayLength(constType->getFSDatatype(),
                                               constType->getNominalSize(),
                                               constType->getPrecision(),
                                               constType->getScale(),
                                               0);

               char strval[t+1];
               memset( strval, ' ', t );

               // Get the ASCII representation
               ex_expr::exp_return_type retcode =
                 convDoIt((char*)constVal->getConstValue(),
                          constVal->getStorageSize(),
                          (short)constType->getFSDatatype(),
                          constType->getPrecision(),
                          constType->getScale(),
                          strval,
                          t,                          // target length
                          REC_BYTE_F_ASCII,           // target type
                          0,                          // no char limit
                          SQLCHARSETCODE_ISO88591,    // ISO 8859-1
                          NULL,                       // no vc length
                          0);                         // not a varchar

               if ( retcode == ex_expr::EXPR_OK )
                 {
                   strval[t] = 0;
                   val = strval;
                   val = val.strip(NAString::trailing, ' ');
                 }

	       vLen = val.length();
	     } // exact numeric

	   len += vLen;
	   
	   result.append((char*)&len, sizeof(short));

	   if (type.supportsSQLnull())
	     {
	       short nullVal = 0;
	       result.append((char*)&nullVal, sizeof(short));
	     }
	   
	   result.append(val.data(), vLen);
	 }
     } else
        return;
  } // for
}

Int32 ValueIdList::countConstantsAsPrefixes()
{
  Int32 ct = 0;
  for (CollIndex i = 0; i < entries(); i++)
  {
     ItemExpr* ie = at(i).getItemExpr();

     switch (ie->getOperatorType()) {
       case ITM_CACHE_PARAM:
       case ITM_CONSTANT:
         ct++;
         continue;

       case ITM_VEG_REFERENCE:
       case ITM_VEG_PREDICATE:
         {
         ValueIdSet result;
         ie->findAll(ITM_CONSTANT, result, TRUE, FALSE);
         ie->findAll(ITM_CACHE_PARAM, result, TRUE, FALSE);

         ct += result.entries();
         }
         continue;

       default:
         break;
     }
  }
  
  return ct;
}

// change literals of a ValueIdSet into ConstantParameters
void ValueIdSet::normalizeForCache(CacheWA& cwa, BindWA& bindWA)
{
  NABoolean changed = FALSE;
  ValueIdSet newExpr;
  // Iterate over all the values (scalar expressions) in this set.
  for (ValueId exprId = init(); next(exprId); advance(exprId)) {
    ItemExpr *iePtr = exprId.getItemExpr();  // original expr.
    // apply normalizeForCache on the item expr
    ItemExpr *nePtr = iePtr->normalizeForCache(cwa, bindWA);
    // if normalizeForCache changed the original expr, update the set
    if (nePtr != iePtr || nePtr->getValueId() != exprId) {
	  changed = TRUE;
	  subtractElement(exprId); // delete original expression from set
	  // accumulate new expressions in another set
	  if (nePtr) {
	    nePtr->convertToValueIdSet(newExpr);
      }
    }
  }
  if (changed) {
    addSet(newExpr); // add normalized expressions to the given set
  }
}

//---------------------------------------------------------------------------
//Check whether the Valudidset contains any CAST expressions && and replace
// it with the original expression
//---------------------------------------------------------------------------
void ValueIdSet::replaceCastExprWithOriginal(const ValueIdSet &originalOutputs,
                                             const RelExpr * parent)
{
  ValueId vid, vid1;
  RelExpr *child = parent->child(0);
  ValueIdSet childOutputs(child->getGroupAttr()->getCharacteristicOutputs());

  for (vid = originalOutputs.init();
       originalOutputs.next(vid);
       originalOutputs.advance(vid) )
  {
    if (vid.getItemExpr()->getOperatorType() == ITM_CAST)
    {
      Cast *castExpr = (Cast *)vid.getItemExpr();

      // can the child produce the valueid vid1?
      vid1 = castExpr->getExpr()->getValueId();

      if  (childOutputs.contains(vid1))
      {
        subtractElement(vid);
        addElement(vid1);
      }
      // for case statement, do not produce any expressions downwards
      // compute the case statement at the root
      else if (castExpr->getExpr()->getOperatorType() == ITM_CASE)
      {
        ValueIdSet coveredValues, emptySet;
        const GroupAttributes emptyGA;
        castExpr->getExpr()->getLeafValuesForCoverTest(coveredValues, emptyGA, emptySet);
        // take an intersection of child outputs and covered values
        // and request from child those ooutputs
        coveredValues.intersectSet(childOutputs);
	//10-070201-2242 -Begin
	//Replace the cast itemExpr only if the child can
	//provide ALL values to evaluate it.
	if(!coveredValues.isEmpty())
	{
	  NABoolean found = TRUE;
	  for(ValueId v = coveredValues.init();
	      coveredValues.next(v);
	      coveredValues.advance(v))
	  {
      	    if(!childOutputs.contains(v))
	      {
		   found = FALSE;
		   break;
	      }
	  }
	  if(found)
	  {
            subtractElement(vid);
            addSet(coveredValues);
	  }
	//10-070201-2242-End
      }
      }
    }
  }
} // ValueIdSet::replaceCastExprWithOriginal()

//-----------------------------------------------------------------------
// Check whether the ValueIdSet contains expressions of the form
// InstNull(Cast(aggregate))) and modify them to aggregate. This is done
// if the left child of the argument joinExpr can potentially produce
// the aggregate; typically the valueid set represents output values of
// a join operator. This is called from Join::pushdownCoveredExpr().
// Input: joinExpression
// Potentially sideeffects the this object
//-----------------------------------------------------------------------
void ValueIdSet::replaceInstnullCastAggregateWithAggregateInLeftJoins
                                            ( RelExpr *joinExpr)
{
  if (joinExpr->getOperatorType() != REL_LEFT_JOIN)
    return;

  ValueId vid;
  for (vid = init();
          next(vid);
          advance(vid) )
  {
     ItemExpr *ie= vid.getItemExpr();
     if (ie->getOperatorType() != ITM_INSTANTIATE_NULL)
       continue;
     if (ie->child(0)->getOperatorType() != ITM_CAST)
       continue;
     Cast *castIE = (Cast *)ie->child(0)->castToItemExpr();
     if (castIE->getExpr()->isAnAggregate())
     {
       // can the right child produce the aggregate?
       ValueIdSet outputs;
       joinExpr->child(1)->getPotentialOutputValues(outputs);
       if (outputs.contains(castIE->getExpr()->getValueId()))

       {
         subtractElement(vid);
         addElement(castIE->getExpr()->getValueId());
       }

     }
  }
} // ValueIdSet::replaceInstnullCastAggregateWithAggregateInLeftJoins

// -----------------------------------------------------------------------
// This method is a slight modification of intersectSet. Here instead of
// modifying this operand it returns the modified set
// -----------------------------------------------------------------------
ValueIdSet ValueIdSet::intersect(const ValueIdSet & v) const
{
  ValueIdSet thisCopy(*this);
  thisCopy.intersectSet(v);
  return thisCopy;
}

ValueIdSet& ValueIdSet::intersectSetDeep(const ValueIdSet & v)
{
  for (ValueId tgt = init(); next(tgt); advance(tgt))
    {
      NABoolean found = FALSE;
      for (ValueId vid = v.init(); v.next(vid) && !found; v.advance(vid))
        {
          if (vid.getItemExpr()->referencesTheGivenValue(tgt,FALSE,FALSE))
            found = TRUE; // keep it
        }
      if (!found)
        subtractElement(tgt); // remove unreferenced element
    }
  return *this;
}


// --------------------------------------------------------------------
// return true iff ValueIdSet has predicates that guarantee
// that opd is not nullable
// --------------------------------------------------------------------
NABoolean ValueIdSet::isNotNullable(const ValueId& opd)
{
  // if opd's type is not nullable then return TRUE
  if (!opd.getType().supportsSQLnullLogical()) {
    return TRUE;
  }
  // opd's type is nullable.

  // search ValueIdSet for a predicate conjunct
  // that can guarantee that opd is not null
  for (ValueId vid = init(); next(vid); advance(vid) ) {
    // predicates are implicitly connected by AND
    ItemExpr *iePtr = vid.getItemExpr();
    switch (iePtr->getOperatorType()) {
    case ITM_IS_NOT_NULL:
      if (iePtr->child(0)->containsTheGivenValue( opd )) {
        // found "opd IS NOT NULL" predicate conjunct
        return TRUE; // opd is not null
      }
      break; // keep looking
    case ITM_EQUAL:
    case ITM_NOT_EQUAL:
    case ITM_LESS:
    case ITM_LESS_EQ:
    case ITM_GREATER:
    case ITM_GREATER_EQ:
      if (iePtr->child(0)->containsTheGivenValue( opd ) ||
          iePtr->child(1)->containsTheGivenValue( opd )) {
        // found "opd compop expr" predicate conjunct
        return TRUE; // opd is not null
      }
      break; // keep looking
    case ITM_IS_NULL:
      if (iePtr->child(0)->containsTheGivenValue( opd )) {
        // found "opd IS NULL" predicate conjunct
        return FALSE; // opd is null
      }
      break;
    default:
      break; // keep looking
    }
  }
  return FALSE;
}

void ValueIdList::addMember(ItemExpr* x) 
{ 
   insert(x->getValueId()); 
}

void ValueIdSet::addMember(ItemExpr* x) 
{ 
   (*this) += (x->getValueId()); 
}

Lng32 ValueIdList::findPrefixLength(const ValueIdSet& x) const
{
  CollIndex count = this->entries();
  Lng32 ct = 0;
  for (CollIndex i=0; i<count; i++)
    {
      if ( x.contains((*this)[i]) )
         ct++;
      else
         break;
    }
  return ct;
}

// -----------------------------------------------------------------------
// replace any ColReference (of the given column name) in of this value
// expression with the given expression.
// used in ValueId::computeEncodedKey() to assign key values into the
// salt/DivisionByto expression.
// ----------------------------------------------------------------------
void ValueId::replaceColReferenceWithExpr(const NAString& colName,
                                          const ValueId & vid)
{
  ItemExpr* thisItemExpr = getItemExpr();
  for( Lng32 i = 0; i < thisItemExpr->getArity(); i++ )
  {
    ValueId childValueId = thisItemExpr->child(i).getValueId();
    ItemExpr* childItemExpr = childValueId.getItemExpr();

    if( childItemExpr->getOperatorType() == ITM_REFERENCE)
    {
      if( ((ColReference*)childItemExpr)->getColRefNameObj().getColName() == colName )
        thisItemExpr->setChild( i, vid.getItemExpr() );
    }
    childValueId.replaceColReferenceWithExpr( colName, vid );
  }
}


char*
ValueIdList::computeEncodedKey(const TableDesc* tDesc, NABoolean isMaxKey, 
                               char*& encodedKeyBuffer, Int32& keyBufLen) const
{
   const NATable*  naTable = tDesc->getNATable();


   CollIndex count = entries();
   NAString** inputStrings = new (STMTHEAP) NAStringPtr[count];

   for (Int32 j=0; j<count; j++ ) 
       inputStrings[j] = NULL;

   for (Int32 j=0; j<count; j++ ) {

      ValueId vid = (*this)[j];
      ItemExpr* ie = vid.getItemExpr();

      if ( ie->getOperatorType() != ITM_CONSTANT ) {
          
          ConstValue* value = NULL;
          if ( ie->doesExprEvaluateToConstant(TRUE, TRUE) ) {
             ValueIdSet availableValues;
             // do a simple VEG replacement with no available values
             // and no inputs, all VEGies should have constants in
             // them and should be replaced with those
             ie = ie->replaceVEGExpressions(availableValues, availableValues);
             if (ie->getOperatorType() == ITM_CONSTANT)
               value = static_cast<ConstValue *>(ie);
             else
               value = ie->evaluate(STMTHEAP);

             if ( !value )
                return NULL;
          } else
             return NULL;

          inputStrings[j] = new (STMTHEAP) NAString(value->getConstStr(FALSE));
      } else  {
         // no need to prefix with charset prefix.
         inputStrings[j] = new (STMTHEAP) NAString(((ConstValue*) ie)->getConstStr(FALSE));

         if ( *inputStrings[j] == "<min>" ||  *inputStrings[j] == "<max>" )
            inputStrings[j] = NULL;
      }
   }

   const NAFileSet * naf = naTable->getClusteringIndex();
   const TrafDesc * tableDesc = naTable->getTableDesc();
   TrafDesc * colDescs = tableDesc->tableDesc()->columns_desc;
   TrafDesc * keyDescs = (TrafDesc*)naf->getKeysDesc();

   // cast away const since the method may compute and store the length
   keyBufLen = ((NAFileSet*)naf)->getEncodedKeyLength(); 

   if ( (count > 0) && (inputStrings[0]) && 
        ( naTable->isHbaseCellTable() || naTable->isHbaseRowTable() ) ) { 
      // the encoded key for Native Hbase table is a null-terminated string ('<key>')
      NAString key;
      key.append("(");

      size_t idx = inputStrings[0]->index("_ISO88591");
      if ( idx == 0 )
         key.append(inputStrings[0]->remove(0, 9));
      else
         key.append(*inputStrings[0]);

      key.append(")");

      keyBufLen = inputStrings[0]->length() + 5; // extra 4 bytes for (,', ', ), and one byte for null. 

      if (!encodedKeyBuffer )
         encodedKeyBuffer = new (STMTHEAP) char[keyBufLen];

      memcpy(encodedKeyBuffer, key.data(), key.length());
      encodedKeyBuffer[key.length()] = NULL;

      NADELETEARRAY(inputStrings, count, NAStringPtr, STMTHEAP);

      return encodedKeyBuffer;

   } else {
      keyBufLen = ((NAFileSet*)naf)->getEncodedKeyLength(); 

      if (!encodedKeyBuffer )
         encodedKeyBuffer = new (STMTHEAP) char[keyBufLen];

      short ok = encodeKeyValues(colDescs, keyDescs,
                      inputStrings,    // INPUT
                      FALSE,           // not isIndex
                      isMaxKey,        
                      encodedKeyBuffer,// OUTPUT
                      STMTHEAP, CmpCommon::diags());

      NADELETEARRAY(inputStrings, count, NAStringPtr, STMTHEAP);

      return ( ok == 0 ) ? encodedKeyBuffer : NULL;
   }
}

void ValueIdSet::findAllOpType(OperatorTypeEnum type, ValueIdSet & result) const
{
    for(ValueId valId = init(); next(valId); advance(valId)) {
  
      ItemExpr *itmExpr = valId.getItemExpr();
      if(itmExpr->getOperatorType() == type) 
         result += valId;
    }
}

void ValueIdSet::findAllChildren(ValueIdSet & result) const
{   
    for(ValueId valId = init(); next(valId); advance(valId)) {
      
      ItemExpr *itmExpr = valId.getItemExpr();

      for ( Lng32 i = 0; i < itmExpr->getArity(); i++ )
         result += itmExpr->child(i).getValueId();
    }
}

void ValueIdSet::addOlapLeadFuncs(const ValueIdSet& input, ValueIdSet& result)
{
    for(ValueId valId = init(); next(valId); advance(valId)) {
       ItemExpr *itmExpr = valId.getItemExpr();
       if ( itmExpr->getOperatorType() == ITM_OLAP_LEAD ) {

          ItmLeadOlapFunction* me = (ItmLeadOlapFunction*)(itmExpr);  

          for(ValueId j= input.init(); input.next(j); input.advance(j)) {
             ItemExpr* child = j.getItemExpr();
             ItmLeadOlapFunction* lead = 
                 new (STMTHEAP) ItmLeadOlapFunction(child, me->getOffset());
             lead->synthTypeAndValueId();
             result.insert(lead->getValueId());
         }
      }
    }
}

