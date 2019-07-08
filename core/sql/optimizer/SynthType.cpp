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
* File:         SynthType.C
* Description:  Methods for synthesizing a type
* Created:      3/15/95
* Language:     C++
*
*
*
*
******************************************************************************
*/

#define   SQLPARSERGLOBALS_NADEFAULTS

#include "Sqlcomp.h"
#include "AllItemExpr.h"
#include "BindWA.h"
#include "CmpStatement.h"
#include "CmpErrors.h"
#include "ComSqlId.h"
#include "OptimizerSimulator.h"
#include "exp_datetime.h"

#include "ComSSL.h"
// For TRIGGERS_STATUS_VECTOR_SIZE and SIZEOF_UNIQUE_EXECUTE_ID
#include "Triggers.h"
#include "TriggerEnable.h"
#ifndef NDEBUG
  static Int32 NCHAR_DEBUG = -1;	// note that, for perf, we call getenv only once
#endif

#include "SqlParserGlobalsCmn.h"
//#define getDefaultCharSet CharInfo::getCharSetEnum(ActiveSchemaDB()->getDefaults().getValue(DEFAULT_CHARSET))
#define getDefaultCharSet SqlParser_DEFAULT_CHARSET

// -----------------------------------------------------------------------
// utility functions -- cosmetics of error message arguments
// -----------------------------------------------------------------------

// These just shorten error messages by removing irrelevant junk
static void shortenTypeSQLname(NAString &typStr)
{
  size_t i = typStr.index('(');
  if (i && i != NA_NPOS)
    typStr.remove(i);			// remove from lparen onward
  i = typStr.index(' ');
  if (i && i != NA_NPOS)
    typStr.remove(i);			// remove from space onward
}

static void shortenTypeSQLname(const NAType &op,
			       NABuiltInTypeEnum typEnum,
			       NAString &typStr)
{
  if (op.getTypeQualifier() == typEnum) {
    size_t i = typStr.index(')');
    if (i != NA_NPOS)
      i = typStr.index(" ", i);		// find space AFTER the parens
    else
      i = typStr.index(' ');		// find first space (no parens)
    if (i && i != NA_NPOS)
      typStr.remove(i);			// remove from space onward
  }
}

void emitDyadicTypeSQLnameMsg(Lng32 sqlCode,
			      const NAType &op1,
			      const NAType &op2,
			      const char *str1 = NULL,
			      const char *str2 = NULL,
			      ComDiagsArea * diagsArea = NULL,
                              const Lng32 int1 = -999999)
{
  NAString tsn1(op1.closestEquivalentExternalType( HEAP )->getTypeSQLname(TRUE /*terse*/));
  NAString tsn2(op2.closestEquivalentExternalType( HEAP )->getTypeSQLname(TRUE /*terse*/));
  NABoolean charShorten = TRUE, numShorten = TRUE;

  if (op1.getTypeQualifier() == NA_CHARACTER_TYPE &&
      op2.getTypeQualifier() == NA_CHARACTER_TYPE) {

    const CharType &ct1 = (CharType&)op1;
    const CharType &ct2 = (CharType&)op2;
    NABoolean csDiff = (ct1.getCharSet()      != ct2.getCharSet() ||
    			ct1.getCharSet()      == CharInfo::UnknownCharSet);
    NABoolean coDiff = (ct1.getCollation()    != ct2.getCollation() ||
    			ct1.getCollation()    == CharInfo::UNKNOWN_COLLATION);

    if (csDiff)
      charShorten = FALSE;	// leave the CHARACTER SET info as is in tsnX
    else if (coDiff) {		// add COLLATE info to the type-texts
      tsn1 += NAString(" ") + ct1.getCollateClause(ct1.getCollation());
      tsn2 += NAString(" ") + ct2.getCollateClause(ct2.getCollation());
      charShorten = FALSE;
    }
    // else, do do the charShorten thing

    numShorten = FALSE;		// both args are character: shorten is a no-op
  }
  else if (op1.getTypeQualifier() == NA_NUMERIC_TYPE &&
	   op2.getTypeQualifier() == NA_NUMERIC_TYPE) {
    if (!str1 || strcmp(str1, "||"))	// If a CONCAT, we do shorten; else:
      numShorten = FALSE;	// do not shorten, we need distinguishing info
    charShorten = FALSE;	// both args are numeric: shorten is a no-op
  }

  if (charShorten) {		// "CHAR(8) CHARACTER SET UNICODE" => "CHAR(8)"
    shortenTypeSQLname(op1, NA_CHARACTER_TYPE, tsn1);
    shortenTypeSQLname(op2, NA_CHARACTER_TYPE, tsn2);
  }
  if (numShorten) {		// "NUMERIC(8, 2) SIGNED" => "NUMERIC(8, 2)"
    shortenTypeSQLname(op1, NA_NUMERIC_TYPE, tsn1);
    shortenTypeSQLname(op2, NA_NUMERIC_TYPE, tsn2);
  }

  ComDiagsArea * da = (diagsArea ? diagsArea : CmpCommon::diags());
  *da << DgSqlCode(sqlCode);
  switch (sqlCode) {
    case -4034:
      CMPASSERT(str1);
      *da
	<< DgString0(tsn1)
	<< DgString1(str1)
	<< DgString2(tsn2)
	<< DgString3(str2 ? str2 : "");
      break;
    case -4039:
      CMPASSERT(str1);
      *da
	<< DgColumnName(str1)
	<< DgString0(tsn1)
	<< DgString1(tsn2);
      break;
    case arkcmpErrorISPWrongDataType:    // -19016
      CMPASSERT(str1);
      CMPASSERT(int1 != -999999);
      *da
        << DgString0(str1)
        << DgString1(tsn1)
        << DgInt0(int1)
        << DgString2(tsn2);
      break;
    default:
      *da
	<< DgString0(tsn1)
	<< DgString1(tsn2);
  }

} // emitDyadicTypeSQLnameMsg


// -----------------------------------------------------------------------
// helper functions for *Type::isComparable() methods
// -----------------------------------------------------------------------

static inline NABoolean involvesEQorNE(const OperatorType &opType)
{
  return opType.match(ITM_WILDCARD_EQ_NE) ||
         OperatorTypeEnum(opType) == ITM_LIKE ||
         OperatorTypeEnum(opType) == ITM_LIKE_DOUBLEBYTE ||
         OperatorTypeEnum(opType) == ITM_POSITION ||
         OperatorTypeEnum(opType) == ITM_REPLACE ||
         OperatorTypeEnum(opType) == ITM_REPLACE_UNICODE ||
         OperatorTypeEnum(opType) == ITM_TRIM ||	  // == ' ' (space char)
         OperatorTypeEnum(opType) == ITM_VALUEIDUNION;	  // when DISTINCT flag
}

static ItemExpr * propagateCoAndCoToItem(ItemExpr *ie,
					 CharInfo::Collation    co,
					 CharInfo::Coercibility ce)
{
  // The special check is here for isAUserSuppliedInput()
  // because we cannot directly mutate the type of a
  // ConstValue, HostVar, DynamicParam, or AnsiUSERFunction
  // because their bindNode's call ItemExpr::bindUserInput()
  // which makes multiple refs to an input all map to
  // (all share) the SAME valueId.
  //
  // Here we only want to mutate THIS ref's type,
  //   or, equivalently, CAST a leaf type to the new type --
  //   where leaves are basic items the Binder does lookup on --
  //   ColRefs (Base/IndexColumns), ConstValues, HostVars, DynamicParams.
  //   In these lookups we do NOT want to change the leaf's original ValueId
  //   NOR its original type -- hence, we CAST.
  //   It is safe not to do an additional bindNode (we have no bindWA anyway!)
  //   because we KNOW the new type is okay!

  CharType *ct = (CharType *)&ie->getValueId().getType();
  CMPASSERT(ct->getTypeQualifier() == NA_CHARACTER_TYPE);
  if (ct->getCollation() != co) {

    #ifndef NDEBUG
      if (NCHAR_DEBUG > 0)
	cerr << "CMP--:\t"
	     << ct->getCollation() << ',' <<  ct->getCoercibility() << ' '
	     << co << ',' << ce
	     << '\t' << ie->getText()
	     << '\t' << ie->getValueId();

      CMPASSERT(co != CharInfo::UNKNOWN_COLLATION);	// sanity check
      CMPASSERT(ce != CharInfo::NO_COLLATING_SEQUENCE);	// sanity check
    #endif

    ct = (CharType *)ct->newCopy(HEAP);
    ct->setCoAndCo(co, ce);

    // add a cast to the right type
    // (since the child may be shared by other ItemExprs we can't
    // simply change it, but we might someday be able to do an
    // optimization similar to what ICAT does)
    ie = new HEAP Cast(ie, ct);
    ie->synthTypeAndValueId();

    #ifndef NDEBUG
      if (NCHAR_DEBUG > 0)
	cerr << '\t' << ie->getValueId()
	     << endl;
    #endif

  } // collation of operand overridden

  return ie;
}

static void propagateCoAndCoToChildren(ItemExpr *parentOp,
				       CharInfo::Collation    co,
				       CharInfo::Coercibility ce)
{
  // Propagate the new co & ce to the immediate operands --
  // yes, a shallow propagation is what we want.

  #ifndef NDEBUG
    // Just double-check that Cast::synthType() is NOT calling here...
    // If it did, we would rewrite its child and the Cast would get optimized
    // away as a no-op.
    CMPASSERT(!parentOp->getOperator().match(ITM_ANY_CAST));
  #endif

  for (Int32 i = 0; i < parentOp->getArity(); i++) {
    ItemExpr *ie = parentOp->child(i);
    if (ie &&
        ie->getValueId().getType().getTypeQualifier() == NA_CHARACTER_TYPE)
      parentOp->child(i) = propagateCoAndCoToItem(ie, co, ce);
  }
}

static Int32 getNumCHARACTERArgs(ItemExpr *parentOp)
{
  Int32 n = 0;
  for (Int32 i = 0; i < parentOp->getArity(); i++) {
    ItemExpr *ie = parentOp->child(i);
    if (ie &&
        ie->getValueId().getType().getTypeQualifier() == NA_CHARACTER_TYPE)
      n++;
  }
  return n;
}

// -----------------------------------------------------------------------
// The virtual NAType::isComparable() methods -- implemented here rather than
// ../common/*Type.cpp because they're called only from here, and here
// we've embedded stuff like ../optimizer/ItemExpr methods and
// the ../arkcmp/CmpCommon global-diags-area.
// -----------------------------------------------------------------------

NABoolean NAType::isComparable(const NAType &other,
			       ItemExpr *parentOp,
			       Int32 emitErr,
                               UInt32 * flags) const
{
  #ifndef NDEBUG
    CMPASSERT(parentOp);	//## reserved for future errmsg 4034 w/ unparse,
  #endif			// for CoAndCo propagation and for errmsgs!

    if (isCompatible(other, flags))
      return TRUE;
    NAString defVal;
    NABoolean charsetInference =
      (CmpCommon::getDefault(INFER_CHARSET, defVal) == DF_ON);
    if(charsetInference  &&
       getTypeQualifier()       == NA_CHARACTER_TYPE &&
       other.getTypeQualifier() == NA_CHARACTER_TYPE){  // do not reject matches of UNKNOWN_CHARSET with UNKNOWN_CHARSET
      CharType *ct = (CharType *)this;
      if (ct->isCompatibleAllowUnknownCharset(other))
        return TRUE;
    }
    
    if (emitErr == EmitErrIfAnyChar)
    if (getTypeQualifier()       != NA_CHARACTER_TYPE &&
        other.getTypeQualifier() != NA_CHARACTER_TYPE)
      emitErr = FALSE;

  if (emitErr) {
    // 4041 Type $1 cannot be compared with type $2.
    //10-070228-2913 -Begin

    //When MODE_SPECIAL_1 'ON', UNICODE CHARSET, and UPPER
    //function is involved data type is converted to VARCHAR(dataStorageSize).
    //dataSotragesize = getMaxCharLen() * bytesPerChar.
    //see in Upper::synthesizeType() method.
    //When generating error condition we convert it back to original size.
    if ( getTypeQualifier() == NA_CHARACTER_TYPE)
    {
      CharType &ct1 = (CharType&)*this;
      if((ct1.isCaseinsensitive()) &&
         (ct1.getCharSet() == CharInfo::UNICODE) &&
         (parentOp->child(0)->castToItemExpr()->getOperatorType() == ITM_UPPER))
      {
         ct1.setDataStorageSize(ct1.getDataStorageSize()/3);
      }
      //10-070228-2913 -End
    }

    emitDyadicTypeSQLnameMsg(-4041, *this, other);
  }
  return FALSE;
}

NABoolean CharType::isComparable(const NAType &otherNA,
				 ItemExpr *parentOp,
				 Int32 emitErr,
                                 UInt32 * flags) const
{
  if (NOT NAType::isComparable(otherNA, parentOp, emitErr, flags))
    return FALSE;

  const CharType &other = (const CharType &)otherNA;

  CharInfo::Collation    co;
  CharInfo::Coercibility ce;
  computeCoAndCo(other, co, ce);
  NABoolean cmpOK = (co != CharInfo::UNKNOWN_COLLATION);

  if (emitErr) emitErr = +1;			// for fall-thru msg suppression
  if (cmpOK) {

    // a "mini-cache" to avoid proc call, for perf
    static THREAD_P CharInfo::Collation cachedCO = CharInfo::UNKNOWN_COLLATION;
    static THREAD_P Int32         cachedFlags = CollationInfo::ALL_NEGATIVE_SYNTAX_FLAGS;

    if (cachedCO != co) {				// use the mini-cache
      cachedCO = co;
      cachedFlags = CharInfo::getCollationFlags(co);
    }

    if (involvesEQorNE(parentOp->getOperator())) {
      if (cachedFlags & CollationInfo::EQ_NE_CMP_ILLEGAL)
	cmpOK = FALSE;
    }
    else if (cachedFlags & CollationInfo::ORDERED_CMP_ILLEGAL)
      cmpOK = FALSE;

    if (!cmpOK && emitErr > 0) {
      // 4044 Collation $0~String0 does not support the $1~String1 operator.
      *CmpCommon::diags() << DgSqlCode(-4044)
        << DgString0(CharInfo::getCollationName(co))
        << DgString1(parentOp->getTextUpper());

      emitErr = -1;		// We fall thru but do not also emit error 4041.
    }
  } // additional collation flag checks

  if (cmpOK)
    propagateCoAndCoToChildren(parentOp, co, ce);	// type-synth/propagate!
  else {
    if (emitErr > 0)
      // 4041 Type $1 cannot be compared with type $2.
      emitDyadicTypeSQLnameMsg(-4041, *this, other);

//  if (emitErr)		// +1 OR -1
//    //## also emit errmsg 4034 w/ unparse?
  }

  #ifndef NDEBUG
    if (NCHAR_DEBUG < 0) NCHAR_DEBUG = getenv("NCHAR_DEBUG") ? +1 : 0;
    if (NCHAR_DEBUG > 0) {
      NAString p(CmpCommon::statementHeap());
      parentOp->unparse(p);
      NAString s(getTypeSQLname(TRUE /*terse*/));
      s += NAString(" ") + getCollateClause(getCollation());
      cerr << "CMP" << (cmpOK ? "==" : "<>") << ":\t"
           << (Int32)parentOp->getOperatorType() << '\t'
	   << p << endl
	   << s << '\t' << getCoercibilityText(getCoercibility())
	   << endl;

      s = other.getTypeSQLname(TRUE /*terse*/);
      s += NAString(" ") + other.getCollateClause(other.getCollation());
      cerr << s << '\t' << other.getCoercibilityText(other.getCoercibility())
           << endl;

      cerr << CharInfo::getCollationName(co) << '\t'
           << CharType::getCoercibilityText(ce)
	   << endl;
      if (!cmpOK)
        cerr << endl;
    }
  #endif

  return cmpOK;
}

NABoolean SQLBinaryString::isComparable(const NAType &otherNA,
                                      ItemExpr *parentOp,
                                      Int32 emitErr,
                                      UInt32 * flags) const
{
  if (NOT NAType::isComparable(otherNA, parentOp, emitErr, flags))
    return FALSE;

  return TRUE;
}

// -----------------------------------------------------------------------
// additional, miscellaneous helper functions
// -----------------------------------------------------------------------

// Called by BiRelat and QuantifiedComp comparison predicates.
static NABoolean synthItemExprLists(ItemExprList &exprList1,
				    ItemExprList &exprList2,
				    NABoolean allowIncompatibleComparison,
				    NABoolean &allowsUnknown,
				    ItemExpr *parentOp)
{
  if (exprList1.entries() != exprList2.entries()) {
    // 4042 The operands of a comparison predicate must be of equal degree.
    *CmpCommon::diags() << DgSqlCode(-4042);
    return FALSE;
  }

  NABoolean ODBC = (CmpCommon::getDefault(ODBC_PROCESS) == DF_ON);
  NABoolean JDBC = (CmpCommon::getDefault(JDBC_PROCESS) == DF_ON);
  allowsUnknown = FALSE;
  for (CollIndex i = 0; i < exprList1.entries(); i++) {
    //
    // Type cast any params.
    //
    ValueId vid1 = exprList1[i]->getValueId();
    ValueId vid2 = exprList2[i]->getValueId();

    NABoolean dummy;
    if (vid1.getType().getTypeQualifier() != NA_UNKNOWN_TYPE &&
	vid2.getType().getTypeQualifier() == NA_UNKNOWN_TYPE &&
	vid2.getItemExpr()->getOperatorType() == ITM_CONSTANT &&
	(vid2.getItemExpr()->castToConstValue(dummy))->isNull())
      {
	vid2.coerceType(vid1.getType());
      }
    else
    if (vid2.getType().getTypeQualifier() != NA_UNKNOWN_TYPE &&
	vid1.getType().getTypeQualifier() == NA_UNKNOWN_TYPE &&
	vid1.getItemExpr()->getOperatorType() == ITM_CONSTANT &&
	(vid1.getItemExpr()->castToConstValue(dummy))->isNull())
      {
	vid1.coerceType(vid2.getType());
      }

    // if this query is being processed for ODBC, then type cast param
    // to have the same type as the other side of birelat op. Otherwise,
    // give param the default type if the other side is an exact numeric.
    if ((NOT ODBC) && (NOT JDBC)) {
      // give param the default type if the other side is an exact numeric.
      if (vid1.getType().getTypeQualifier() == NA_UNKNOWN_TYPE &&
	  vid2.getType().getTypeQualifier() == NA_NUMERIC_TYPE) {
	// if op1 is a param with unknown type and
	// op2 is an exact numeric, type cast op1 to the default numeric type
	const NumericType& op2 = (NumericType&)vid2.getType();
	if (op2.isExact())
	  vid1.coerceType(NA_NUMERIC_TYPE);
      }
      else if (vid2.getType().getTypeQualifier() == NA_UNKNOWN_TYPE &&
	       vid1.getType().getTypeQualifier() == NA_NUMERIC_TYPE) {
	const NumericType& op1 = (NumericType&)vid1.getType();
	if (op1.isExact())
	  vid2.coerceType(NA_NUMERIC_TYPE);
      };

      vid1.coerceType(vid2.getType(), NA_NUMERIC_TYPE);
    }
    else {
      // coerce to default character type(VARCHAR(32)) for ODBC.
      vid1.coerceType(vid2.getType(), NA_CHARACTER_TYPE);
    }

    vid2.coerceType(vid1.getType());
    //
    // Check that the operands are comparable.
    //
    const NAType *operand1 = &vid1.getType();
    const NAType *operand2 = &vid2.getType();

    NABoolean DoCompatibilityTest = TRUE;

    NAString defVal;

    if ( operand1->getTypeQualifier() == NA_CHARACTER_TYPE &&
         operand2->getTypeQualifier() == NA_CHARACTER_TYPE
       )
    {
      if ( CmpCommon::getDefault(ALLOW_IMPLICIT_CHAR_CASTING) == DF_ON )
      {
          DoCompatibilityTest = FALSE;
          //
          // NOTE: The Generator has code to throw in a Translate node if an
          // incompatible character set comparison is attempted.
          //
      }
      const CharType *charOp1 = (CharType*)&(vid1.getType());
      const CharType *charOp2 = (CharType*)&(vid2.getType());

      NABoolean charsetInference =
       (CmpCommon::getDefault(INFER_CHARSET, defVal) == DF_ON);

      if ( charsetInference ) {

         const CharType* desiredType =
           CharType::findPushDownCharType(getDefaultCharSet,
                                      charOp1, charOp2, 0);

         if ( desiredType )
         {
	   // just push down the charset field. All other fields are
	   // meaningless.
            vid1.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);
            vid2.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);

         }
      }

      // get the newly pushed-down/relaxed types
      operand1 = &vid1.getType();
      operand2 = &vid2.getType();

      if ( DoCompatibilityTest && NOT operand1->isCompatible(*operand2) ) {
         // 4041 comparison between these two types is not allowed
         emitDyadicTypeSQLnameMsg(-4041, *operand1, *operand2);
         return FALSE;
      }
    }

    // binary datatypes can only be compared with binary datatypes
    if (((DFS2REC::isBinaryString(operand1->getFSDatatype())) &&
         (NOT DFS2REC::isBinaryString(operand2->getFSDatatype()))) ||
        ((DFS2REC::isBinaryString(operand2->getFSDatatype())) &&
         (NOT DFS2REC::isBinaryString(operand1->getFSDatatype()))))
      {
        emitDyadicTypeSQLnameMsg(-4041, *operand1, *operand2);

        return FALSE;
      }
            
    allowsUnknown = allowsUnknown OR
                    operand1->supportsSQLnullLogical() OR
                    operand2->supportsSQLnullLogical();

    if (allowIncompatibleComparison)
      {
	// incompatible conversion is only allowed between:
	// 1. char and numeric types.
	// 2. char literal and date types

	// Or for special_1 mode:
	// 3. DATE and numeric.  Date is an interval from year 1900.
	// 4. interval and numeric.

	// Check if this is char and numeric comparison
	if (((operand1->getTypeQualifier() == NA_CHARACTER_TYPE) &&
	     (operand2->getTypeQualifier() == NA_NUMERIC_TYPE) &&
	     ((((CharType*)operand1)->getCharSet() == CharInfo::ISO88591) ||
              (((CharType*)operand1)->getCharSet() == CharInfo::UTF8))) ||
	    ((operand1->getTypeQualifier() == NA_NUMERIC_TYPE) &&
	     (operand2->getTypeQualifier() == NA_CHARACTER_TYPE) &&
	     ((((CharType*)operand2)->getCharSet() == CharInfo::ISO88591) ||
              (((CharType*)operand2)->getCharSet() == CharInfo::UTF8))))
	  {
	    return TRUE;
	  }

	// Check if this is char and date comparison
	if (((operand1->getTypeQualifier() == NA_CHARACTER_TYPE) &&
	     (operand2->getTypeQualifier() == NA_DATETIME_TYPE) &&
	     ((((CharType*)operand1)->getCharSet() == CharInfo::ISO88591) ||
              (((CharType*)operand1)->getCharSet() == CharInfo::UTF8))) ||
	    ((operand1->getTypeQualifier() == NA_DATETIME_TYPE) &&
	     (operand2->getTypeQualifier() == NA_CHARACTER_TYPE) &&
	     ((((CharType*)operand2)->getCharSet() == CharInfo::ISO88591) ||
              (((CharType*)operand2)->getCharSet() == CharInfo::UTF8))))

	  {
	    return TRUE;
	  }

	if (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON)
	  {
	    // Check if this is numeric literal and date comparison
	    if (((operand1->getTypeQualifier() == NA_NUMERIC_TYPE) &&
		 (operand2->getTypeQualifier() == NA_DATETIME_TYPE)) ||
		((operand1->getTypeQualifier() == NA_DATETIME_TYPE) &&
		 (operand2->getTypeQualifier() == NA_NUMERIC_TYPE)))
	      {
		NumericType *numOper;
		DatetimeType *dtOper;
		if (operand1->getTypeQualifier() == NA_NUMERIC_TYPE)
		  {
		    numOper = &(NumericType&)vid1.getType();
		    dtOper = &(DatetimeType&)vid2.getType();
		  }
		else
		  {
		    numOper = &(NumericType&)vid2.getType();
		    dtOper = &(DatetimeType&)vid1.getType();
		  }

		// make sure it is DATE to exact NUMERIC with scale
		// of 0 comparison.
		if ((numOper->isExact()) &&
		    (numOper->getScale() == 0) &&
		    (dtOper->getPrecision() == SQLDTCODE_DATE))
		  return TRUE;
	      }

	    // Check if this is numeric literal and interval comparison
	    if (((operand1->getTypeQualifier() == NA_NUMERIC_TYPE) &&
		 (operand2->getTypeQualifier() == NA_INTERVAL_TYPE)   &&
		 (vid1.getItemExpr()->getOperatorType() == ITM_CONSTANT)) ||
		((operand1->getTypeQualifier() == NA_INTERVAL_TYPE) &&
		 (operand2->getTypeQualifier() == NA_NUMERIC_TYPE) &&
		 (vid2.getItemExpr()->getOperatorType() == ITM_CONSTANT)))
	      {
		IntervalType* interval;
		const NumericType*  numeric;
		if (operand1->getTypeQualifier() == NA_NUMERIC_TYPE)
		  {
		    numeric  = &(NumericType&)vid1.getType();
		    interval = &(IntervalType&)vid2.getType();
		  }
		else
		  {
		    numeric  = &(NumericType&)vid2.getType();
		    interval = &(IntervalType&)vid1.getType();
		  }

		// make sure it is exact NUMERIC with scale
		// of 0 comparison.
		if ((numeric->isExact()) &&
		    (numeric->getScale() == 0) &&
		    (interval->getFractionPrecision() == 0))
		  return TRUE;
	      }
	  }
      }

    UInt32 flags = 0;
    if (CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON)
      {
        flags |= NAType::ALLOW_INCOMP_OPER;
      }
    
    //## errmsg 4034 w/ unparse?
    if ( DoCompatibilityTest && 
         NOT operand1->isComparable(*operand2, parentOp,
                                    NAType::EmitErrAlways, &flags) )
      return FALSE; 
  }
  return TRUE;			// success

} // synthItemExprLists

static const NAType *synthAvgSum(const NAType& operand,
                                 NABoolean inScalarGroup)
{

  NABoolean aggNeedsToBeNullable;
  aggNeedsToBeNullable = operand.supportsSQLnullPhysical() || inScalarGroup;

  switch (operand.getTypeQualifier()) {
  case NA_NUMERIC_TYPE:
    {
      NumericType * const type = (NumericType * const ) &operand;
      if(type->isExact())
	{
	  /////////////////////////////////////////////////////////////////////
	  // Rules to compute datatype, precision and scale of SUM/AVG for
	  // EXACT numerics.
	  //
	  // Precision and Datatype computation:
	  //
	  // If precision is less than 19, make it LargeInt (Int64).
	  //
	  // If precision is > 19, make result precision = operand precision
	  // + 10 and result to be BigNum datatype.
	  //
	  // Scale computation:
	  // Result scale is always equal to operand's scale.
	  //
	  // Result is also signed if operand is signed.
	  //
	  /////////////////////////////////////////////////////////////////////

	  Lng32 precision = (type->getMagnitude() + 9) / 10 + type->getScale();
	  Lng32 scale = type->getScale();
	  NABoolean isARealBigNum = FALSE;
	  if (type->isBigNum())
	    {
	      // make the max precision to be the precision of operand + 10.
	      // Just a nice, round number.
	      //
	      // It could also be always made to 
	      // be MAX_NUMERIC_PRECISION_ALLOWED. But that would mean that
	      // the result is always NUMERIC(128) which may be too much
	      // for all aggregates. We can think about it.
	      precision = MINOF(precision + 10,
				(Lng32)CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED));

	      isARealBigNum = ((SQLBigNum*)type)->isARealBigNum();
	    }
	  else
	    {
	      NABoolean limitPrecision =
		(CmpCommon::getDefault(LIMIT_MAX_NUMERIC_PRECISION) == DF_ON);
	      
	      if (precision <= MAX_NUMERIC_PRECISION)
		precision = MAX_NUMERIC_PRECISION;
	      
	      if (limitPrecision)
		{
		  if (precision > MAX_NUMERIC_PRECISION)
		    {
		      precision = MAX_NUMERIC_PRECISION;
		    }
		} else {

                    if ( precision >= MAX_NUMERIC_PRECISION + 1 )
                        precision = MINOF(precision + 10,
                                          (Lng32)CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED));
                }
	    }

	  if ((NOT type->isBigNum()) &&
	      (precision <= MAX_NUMERIC_PRECISION))
	    {
	      if (scale > 0)
		return new HEAP
		  SQLNumeric(HEAP, 8, // length = 8 bytes
			     precision,
			     scale,
                             (precision > 9 ? TRUE : type->isSigned()),
			     aggNeedsToBeNullable);

	      else
		return new HEAP SQLLargeInt(HEAP, TRUE, aggNeedsToBeNullable);

	    }
	  else
            {
	      return new HEAP
	        SQLBigNum(HEAP, precision,
			  scale,
			  isARealBigNum,
			  type->isSigned(),
			  aggNeedsToBeNullable);
            }
	}
      else
	{
	  return new HEAP SQLDoublePrecision(HEAP, aggNeedsToBeNullable);
	}
    }
  break;

  case NA_INTERVAL_TYPE:
    {
      IntervalType * const type = (IntervalType * const ) &operand;
      if (type->isSupportedType())
      {
       return new HEAP
         SQLInterval(HEAP, aggNeedsToBeNullable,
                     type->getStartField(),
                     type->computeLeadingPrecision(type->getStartField(),
                                                   MAX_NUMERIC_PRECISION,
                                                   type->getEndField(),
                                                   type->getFractionPrecision()),
                     type->getEndField(),
                     type->getFractionPrecision());
      }
      // else fall through to error
    }
  break;
  }

  // 4038 The operand of an AVG or SUM function must be numeric or interval.
  *CmpCommon::diags() << DgSqlCode(-4038);
  return NULL;
}

//
// getFirstKnownCharSet() - get CharSet of first vid that has one.
//
CharInfo::CharSet getFirstKnownCharSet( ValueId vid1, ValueId vid2, ValueId vid3)
{
  CharInfo::CharSet first_cs = CharInfo::ISO88591;  // Default to ISO88591

  if ( CmpCommon::getDefault(ALLOW_IMPLICIT_CHAR_CASTING) == DF_ON )
  {
     const NAType *otyp1 = &(vid1.getType());
     const NAType *otyp2 = &(vid2.getType());
     const NAType *otyp3 = &(vid3.getType());
     if (otyp1->getTypeQualifier() == NA_CHARACTER_TYPE)
        first_cs = ((CharType *)otyp1)->getCharSet();
     else if (otyp2->getTypeQualifier() == NA_CHARACTER_TYPE)
        first_cs = ((CharType *)otyp2)->getCharSet();
     else if (otyp3->getTypeQualifier() == NA_CHARACTER_TYPE)
        first_cs = ((CharType *)otyp3)->getCharSet();
  }
  return first_cs;
}

// -----------------------------------------------------------------------
// member functions for class ItemExpr
// -----------------------------------------------------------------------

const NAType *ItemExpr::synthTypeWithCollateClause(BindWA *bindWA,
						   const NAType *type)
{
  // First, call the VIRTUAL method, if we were not passed a type to use
  if (!type)			   // e.g. ColReference::bindNode passes a type
    type = synthesizeType();

  if (type && collateClause()) {
    CharInfo::Collation    co = collateClause()->collation_;
    CharInfo::Coercibility ce = collateClause()->coercibility_;
    collateClause() = NULL;
    CMPASSERT(ce == CharInfo::EXPLICIT);	// else, Parser screwed up?

    const ColumnDescList *cols = NULL;
    if (isASubquery())
      cols = ((Subquery *)this)->getRETDesc()->getColumnList();

    if (type->getTypeQualifier() != NA_CHARACTER_TYPE) {
      // 4034 The operation (operand COLLATE coll-name) is not allowed.
      NAString optext(CmpCommon::statementHeap());
      unparse(optext, DEFAULT_PHASE, USER_FORMAT_DELUXE);
      if (isASubquery()) {
	// Cosmetics:  convert 'SCAN C.S.T' to '(SELECT a,b FROM C.S.T)'
        NAString x(optext, CmpCommon::statementHeap());
	x.remove(5);	//offset of "SCAN "
	x.toUpper();
	if (x == "SCAN " || x == "SCAN(") {
	  x = NAString("(SELECT ") + cols->getColumnDescListAsString() +
	      " FROM ";
	  optext.remove(0,5);
	  optext.prepend(x);
	  optext += ")";
	}
      }
      *CmpCommon::diags() << DgSqlCode(-4034)
        << DgString0(optext)
	<< DgString1("COLLATE")
	<< DgString2(CharInfo::getCollationName(co));

      // 4073 COLLATE may not appear after a $string0 type expression.
      NAString typnam(type->getTypeSQLname(TRUE/*terse*/));
      shortenTypeSQLname(typnam);
      if (typnam.length() == 9) {	//len("SQLRecord")
	// Cosmetics:  convert 'SQLRecord' to 'NON-SCALAR'
        NAString x(typnam, CmpCommon::statementHeap());
	x.toUpper();
	if (x == "SQLRECORD") typnam="NON-SCALAR";
      }
      *CmpCommon::diags() << DgSqlCode(-4073)
        << DgString0(typnam);

      type = NULL;
    }
    else if (co == CharInfo::UNKNOWN_COLLATION &&
    	     bindWA->getCurrentScope()->context()->inOrderBy()) {
      // 3169 $0~string0 is not a known collation.
      *CmpCommon::diags() << DgSqlCode(-3169)
        << DgString0(CharInfo::getCollationName(co));
      type = NULL;
    }
    else {
      // Consider
      //	('a' COLLATE AAA || 'b') COLLATE BBB
      // 'a' is AAA/EXPLICIT, so the concat within the parens is too --
      // but ANSI 6.13 SR 4(a)(i) says that outside the parens we override
      // the inner EXPLICIT, so the expression outside the parens
      // is BBB/EXPLICIT.  No computeCoAndCo() needed, simply setCoAndCo().
      //
      type = type->newCopy(HEAP);
      CharType *ct = (CharType *)type;
      ct->setCoAndCo(co, ce);
      if (!ct->isCharSetAndCollationComboOK()) {
        // 3179 Collation $0 is not defined on character set $1.
	*CmpCommon::diags() << DgSqlCode(-3179)
	  << DgString0(CharInfo::getCollationName(ct->getCollation()))
	  << DgString1(CharInfo::getCharSetName(ct->getCharSet()));
	type = NULL;
      }

      if (isASubquery()) {
	CMPASSERT(cols->entries() == 1);
	ColumnDesc *col = cols->at(0);
	ItemExpr *ie = propagateCoAndCoToItem(
			 col->getValueId().getItemExpr(), co, ce);
	col->setValueId(ie->getValueId());
      }
    }
  }

  if (!type) bindWA->setErrStatus();
  return type;
}

// This is the virtual method for ItemExpr's that do not define their own
const NAType *ItemExpr::synthesizeType()
{
  if (getArity() > 0)
    return &child(0)->castToItemExpr()->getValueId().getType();
  return new HEAP SQLUnknown(HEAP);
}

// Propagate type information down the ItemExpr tree.
// Called by coerceType().  The default implementation
// does nothing.  Currently is only redefined by ValueIdUnion
// to propagate the desired type to the sources of the ValueIdUnion.
//
const NAType *
ItemExpr::pushDownType(NAType& desiredType,
                       enum NABuiltInTypeEnum defaultQualifier)
{
   for(CollIndex i = 0; i < getArity(); i++) {
     child(i) -> getValueId().coerceType(desiredType, defaultQualifier);
   }
   
   return (NAType *)synthesizeType();
   //return &desiredType;
}

const NAType *
Cast::pushDownType(NAType& desiredType,
		   enum NABuiltInTypeEnum defaultQualifier)
{
   for(CollIndex i = 0; i < getArity(); i++) {
      child(i) -> getValueId().coerceType(desiredType, defaultQualifier);
   }

   if (getType()->getTypeQualifier() == NA_UNKNOWN_TYPE &&
       desiredType.getTypeQualifier() != NA_UNKNOWN_TYPE)
   {
     type_ = desiredType.newCopy(HEAP);
   }

   return (NAType *)synthesizeType();
}

void ItemExpr::coerceChildType(NAType& desiredType,
                   enum NABuiltInTypeEnum defaultQualifier)
{
   for(CollIndex i = 0; i < getArity(); i++) {
     child(i) -> getValueId().coerceType(desiredType, defaultQualifier);
   }
}

// -----------------------------------------------------------------------
// member functions for class BuiltinFunction.
// This methods is for those functions which are not defined as a
// derived class or do not have a derived synthesizeType method.
// This method should not be called from any derived class's
// synthesizeType method.
// -----------------------------------------------------------------------
const NAType *BuiltinFunction::synthesizeType()
{
  NAType * retType = NULL;
  switch (getOperatorType())
    {
    case ITM_CONVERTTOBITS:
      {
	ValueId vid1 = child(0)->getValueId();

	// untyped param operands are typed as Int32 Unsigned.
	SQLInt si(NULL, FALSE);
	vid1.coerceType(si, NA_NUMERIC_TYPE);

	const NAType &typ1 = vid1.getType();

	// one byte of display size for each bit.
	// 8 bits per byte.
	Int32 maxLength = typ1.getNominalSize() * 8;
	
	if ( typ1.getTypeQualifier() == NA_CHARACTER_TYPE &&
	     typ1.isVaryingLen() == TRUE )
	  retType = new HEAP
	    SQLVarChar(HEAP, maxLength, typ1.supportsSQLnull());
	else
	  retType = new HEAP
	    SQLChar(HEAP, maxLength, typ1.supportsSQLnull());
      }
    break;
    case ITM_SHA1:
      {
        // type cast any params
        ValueId vid1 = child(0)->getValueId();
        SQLChar c1(NULL, ComSqlId::MAX_QUERY_ID_LEN);
        vid1.coerceType(c1, NA_CHARACTER_TYPE);
        //input type must be string
        const NAType &typ1 = child(0)->getValueId().getType();

        if (typ1.getTypeQualifier() != NA_CHARACTER_TYPE)
          {
	    *CmpCommon::diags() << DgSqlCode(-4067) << DgString0("SHA");
	    return NULL;
          }

        retType = new HEAP
           SQLChar(HEAP, 128, FALSE);
	if (typ1.supportsSQLnull())
	  {
	    retType->setNullable(TRUE);
	  }
      }
    break;

    case ITM_SHA2_256:
    case ITM_SHA2_224:
    case ITM_SHA2_384:
    case ITM_SHA2_512:
      {
        ValueId vid1 = child(0)->getValueId();
        SQLChar c1(NULL, ComSqlId::MAX_QUERY_ID_LEN);
        vid1.coerceType(c1, NA_CHARACTER_TYPE);

        const NAType &typ1 = child(0)->getValueId().getType();

        if (typ1.getTypeQualifier() != NA_CHARACTER_TYPE)
        {
          *CmpCommon::diags() << DgSqlCode(-4067) << DgString0("SHA2");
          return NULL;
        }

        Lng32 resultLen = 0;
        switch (getOperatorType()) {
        case ITM_SHA2_224:
            resultLen = (224 * 2) / 8;
            break;
        case ITM_SHA2_256:
            resultLen = (256 * 2) / 8;
            break;
        case ITM_SHA2_384:
            resultLen = (384 * 2) / 8;
            break;
        case ITM_SHA2_512:
            resultLen = (512 * 2) / 8;
            break;
        default:
            break;
        }
        retType = new HEAP
          SQLChar(HEAP, resultLen, typ1.supportsSQLnull());
      }
    break;

    case ITM_MD5:
      {
        // type cast any params
        ValueId vid1 = child(0)->getValueId();
        SQLChar c1(NULL, ComSqlId::MAX_QUERY_ID_LEN);
        vid1.coerceType(c1, NA_CHARACTER_TYPE);
        //input type must be string
        const NAType &typ1 = child(0)->getValueId().getType();

        if (typ1.getTypeQualifier() != NA_CHARACTER_TYPE)
          {
	    *CmpCommon::diags() << DgSqlCode(-4067) << DgString0("MD5");
	    return NULL;
          }

        retType = new HEAP
           SQLChar(HEAP, 32, FALSE);
	if (typ1.supportsSQLnull())
	  {
	    retType->setNullable(TRUE);
	  }
      }
    break;
    case ITM_CRC32:
      {
        const NAType &typ1 = child(0)->getValueId().getType();
        retType = new HEAP
           SQLInt(HEAP, FALSE, FALSE); //unsigned int
        if (typ1.supportsSQLnull())
          {
            retType->setNullable(TRUE);
          }
      } 
    break;
    case ITM_ISIPV4:
    case ITM_ISIPV6:
      {
        // type cast any params
        ValueId vid1 = child(0)->getValueId();
        SQLChar c1(NULL, ComSqlId::MAX_QUERY_ID_LEN);
        vid1.coerceType(c1, NA_CHARACTER_TYPE);
        //input type must be string
        const NAType &typ1 = child(0)->getValueId().getType();

        if (typ1.getTypeQualifier() != NA_CHARACTER_TYPE)
          {
	    *CmpCommon::diags() << DgSqlCode(-4067) << DgString0("IS_IP");
	    return NULL;
          }
        retType = new HEAP
           SQLSmall(HEAP, TRUE, FALSE);
	if (typ1.supportsSQLnull())
	  {
	    retType->setNullable(TRUE);
	  }
      }
    break;

    case ITM_INET_ATON:
      {
        // type cast any params
        ValueId vid1 = child(0)->getValueId();
        SQLChar c1(NULL, ComSqlId::MAX_QUERY_ID_LEN);
        vid1.coerceType(c1, NA_CHARACTER_TYPE);

        //input type must be string
        const NAType &typ1 = child(0)->getValueId().getType();

        if (typ1.getTypeQualifier() != NA_CHARACTER_TYPE)
          {
	    *CmpCommon::diags() << DgSqlCode(-4067) << DgString0("INET_ATON");
	    return NULL;
          }
        retType = new HEAP
           SQLInt(HEAP, FALSE, FALSE);
	if (typ1.supportsSQLnull())
	  {
	    retType->setNullable(TRUE);
	  }
      }
    break;
    case ITM_INET_NTOA:
      {
	// type cast any params
	ValueId vid = child(0)->getValueId();
	vid.coerceType(NA_NUMERIC_TYPE);

	const NAType &typ1 = child(0)->getValueId().getType();
	if (typ1.getTypeQualifier() != NA_NUMERIC_TYPE)
	  {
	    *CmpCommon::diags() << DgSqlCode(-4045) << DgString0("INET_NTOA");
	    return NULL;
	  }
        const NumericType &ntyp1 = (NumericType &) typ1;
        if (NOT ntyp1.isExact() || ntyp1.getScale() != 0)
	  {
	    *CmpCommon::diags() << DgSqlCode(-4046) << DgString0("INET_NTOA");
	    return NULL;
	  }

	retType = new HEAP
	  SQLVarChar(HEAP, 15, FALSE);

	if (typ1.supportsSQLnull())
	  {
	    retType->setNullable(TRUE);
          }
      }
    break;
    case ITM_NULLIFZERO:
      {
	// type cast any params
	ValueId vid = child(0)->getValueId();
	vid.coerceType(NA_NUMERIC_TYPE);

	const NAType &typ1 = child(0)->getValueId().getType();
	if (typ1.getTypeQualifier() != NA_NUMERIC_TYPE)
	  {
	    // 4045 nullifzero function is only defined for numeric types.
	    *CmpCommon::diags() << DgSqlCode(-4045) << DgString0(getTextUpper());
	    return NULL;
	  }

	// returned type is the same as child's type but always nullable.
	retType = typ1.newCopy(HEAP);
	if (NOT typ1.supportsSQLnull())
	  {
	    retType->setNullable(TRUE);
	  }
      }
    break;

    case ITM_NVL:
      {
	// type cast any params
	ValueId vid1 = child(0)->getValueId();
	vid1.coerceType(NA_NUMERIC_TYPE);

        const NAType &typ1 = vid1.getType();
	ValueId vid2 = child(1)->getValueId();
	vid2.coerceType(typ1);

	
	const NAType &typ2 = vid2.getType();

	//
	// Synthesize the result.
	//
       UInt32 flags =
              ((CmpCommon::getDefault(LIMIT_MAX_NUMERIC_PRECISION) == DF_ON)
               ? NAType::LIMIT_MAX_NUMERIC_PRECISION : 0);

	retType = (NAType*)typ1.synthesizeType(SYNTH_RULE_UNION,
					       typ1,
					       typ2,
					       HEAP,
                                            &flags);
	if (retType == NULL)
	  {
	    // 4049 CASE can't have result types that are mixed
	    emitDyadicTypeSQLnameMsg(-4049, typ1, typ2);
	    return NULL;
	  }

	if (NOT typ1.supportsSQLnull())
	  {
	    retType = typ1.newCopy(HEAP);
	  }
      }
    break;

    case ITM_JSONOBJECTFIELDTEXT:
    {
        ValueId vid1 = child(0)->getValueId();
        ValueId vid2 = child(1)->getValueId();

        // untyped param operands are typed as CHAR
        vid2.coerceType(NA_CHARACTER_TYPE);

        const NAType &typ1 = vid1.getType();
        const NAType &typ2 = vid2.getType();

        if ((typ1.getTypeQualifier() != NA_CHARACTER_TYPE) ||
            (typ2.getTypeQualifier() != NA_CHARACTER_TYPE))
        {
            // 4043 The operand of a $0~String0 function must be character.
            *CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextUpper());
            return NULL;
        }

        retType = new HEAP
        SQLVarChar(HEAP, typ1.getNominalSize(), typ1.supportsSQLnull());
    }
    break;

    case ITM_QUERYID_EXTRACT:
      {
	// type cast any params
	ValueId vid1 = child(0)->getValueId();
	SQLChar c1(NULL, ComSqlId::MAX_QUERY_ID_LEN);
	vid1.coerceType(c1, NA_CHARACTER_TYPE);

	ValueId vid2 = child(1)->getValueId();
	SQLChar c2(NULL, 40, FALSE);
	vid2.coerceType(c2, NA_CHARACTER_TYPE);

	const CharType &typ1 = (CharType&)child(0)->getValueId().getType();
	if (typ1.getTypeQualifier() != NA_CHARACTER_TYPE)
	  {
	    // 4043 The operand of a $0~String0 function must be character.
	    *CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextUpper());
	    return NULL;
	  }

	const CharType &typ2 = (CharType&)child(1)->getValueId().getType();
	if (typ2.getTypeQualifier() != NA_CHARACTER_TYPE)
	  {
	    // 4043 The operand of a $0~String0 function must be character.
	    *CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextUpper());
	    return NULL;
	  }

	retType = new HEAP
	  SQLVarChar(HEAP, ComSqlId::MAX_QUERY_ID_LEN,
                     (typ1.supportsSQLnull() || typ2.supportsSQLnull()),
                     FALSE, // not upshifted
                     FALSE, // not case-insensitive
                     CharInfo::ISO88591);
      }
    break;

    case ITM_TOKENSTR:
      {
	// tokenstr('token', 'string');
	// First param is a quoted_string and is typed as char during
	// parsing phase.
	ValueId vid1 = child(0)->getValueId();
	ValueId vid2 = child(1)->getValueId();

	// untyped param operands are typed as CHAR
	vid2.coerceType(NA_CHARACTER_TYPE);

	const NAType &typ1 = vid1.getType();
	const NAType &typ2 = vid2.getType();

	if ((typ1.getTypeQualifier() != NA_CHARACTER_TYPE) ||
	    (typ2.getTypeQualifier() != NA_CHARACTER_TYPE))
	  {
	    // 4043 The operand of a $0~String0 function must be character.
	    *CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextUpper());
	    return NULL;
	  }

	retType = new HEAP
	  SQLVarChar(HEAP, typ2.getNominalSize(), typ2.supportsSQLnull());
      }
    break;

    case ITM_REVERSE:
      {
	// reserve(<value>);
	ValueId vid1 = child(0)->getValueId();

	// untyped param operands are typed as CHAR
	vid1.coerceType(NA_CHARACTER_TYPE);

	const NAType &typ1 = vid1.getType();

	if (typ1.getTypeQualifier() != NA_CHARACTER_TYPE)
	  {
	    // 4043 The operand of a $0~String0 function must be character.
	    *CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextUpper());
	    return NULL;
	  }

        // return type same as child type
        retType = typ1.newCopy(HEAP);
      }
    break;

    case ITM_UNIQUE_ID:
      {
        //please check the ExFunctionUniqueId::eval if the size is changed
	retType = new HEAP SQLChar(HEAP, 36, FALSE);
      }
    case ITM_UNIQUE_ID_SYS_GUID:
      {
        //please check the ExFunctionUniqueId::eval if the size is changed
	retType = new HEAP SQLChar(HEAP, 16, FALSE);
      }
      break;
    case ITM_UNIQUE_SHORT_ID:
      {
        //please check the ExFunctionUniqueId::eval if the size is changed
	retType = new HEAP SQLChar(HEAP, 21, FALSE);
      }
      break;

    case ITM_SOUNDEX:
      {
          // type cast any params
          ValueId vid1 = child(0)->getValueId();
          SQLChar c1(NULL, ComSqlId::MAX_QUERY_ID_LEN);
          vid1.coerceType(c1, NA_CHARACTER_TYPE);
          
          //input type must be string
          const NAType &typ1 = vid1.getType();

          if (typ1.getTypeQualifier() != NA_CHARACTER_TYPE)
          {
              *CmpCommon::diags() << DgSqlCode(-4067) << DgString0("SOUNDEX");
              return NULL;
          }

          retType = new HEAP SQLChar(HEAP, 4, FALSE);
          if (typ1.supportsSQLnull())
          {
              retType->setNullable(TRUE);
          }
      }
      break;

    case ITM_AES_ENCRYPT:
    case ITM_AES_DECRYPT:
      {
        const NAType &typ1 = child(0)->getValueId().getType();
        const NAType &typ2 = child(1)->getValueId().getType();

        if (typ1.getTypeQualifier() != NA_CHARACTER_TYPE ||
                typ2.getTypeQualifier() != NA_CHARACTER_TYPE)
        {
          *CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextUpper());
          return NULL;
        }

        if (getArity() == 3)
        {
          // check the optional init_vector argument
          const NAType &typ3 = child(0)->getValueId().getType();
          if (typ3.getTypeQualifier() != NA_CHARACTER_TYPE)
          {
            *CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextUpper());
          }
        }

        Int32 source_len = typ1.getNominalSize();

        Int32 maxLength = source_len;

        // the origin string is short than encrypted string, so for descrypt process,
        // the length of source string is enough.
        // When encrypting a string, we need a formula to calculate the length
        if (getOperatorType() == ITM_AES_ENCRYPT)
        {
          // the length of crypt_str can be calculated by
          // block_size * (trunc(string_length / block_size) + 1)
          //
          // the block_size should be get using EVP_CIPHER_block_size(), but in some Algorithm
          // type, it return 1 in OpenSSL 1.0.1e . So using EVP_MAX_BLOCK_LENGTH instead of it,
          // which can make sure longer then block size.
          //Int32 aes_mode = CmpCommon::getDefaultNumeric(BLOCK_ENCRYPTION_MODE);
          //size_t block_size = EVP_CIPHER_block_size(aes_algorithm_type[aes_mode]);

          Int32 block_size = EVP_MAX_IV_LENGTH;
          if (block_size > 1) {
            maxLength = block_size * (source_len / block_size) + block_size;
          }
        }

        retType = new HEAP
            SQLVarChar(HEAP, maxLength, TRUE);
      }
      break;

    case ITM_ENCODE_BASE64:
      {
        const NAType &typ1 = child(0)->getValueId().getType();

        Int32 source_len = typ1.getNominalSize();

        Int32 maxLength = str_encoded_len_base64(source_len);

        retType = new HEAP SQLVarChar(HEAP, maxLength, TRUE);
      }
      break;

    case ITM_DECODE_BASE64:
      {
        const NAType &typ1 = child(0)->getValueId().getType();

        if (typ1.getTypeQualifier() != NA_CHARACTER_TYPE)
        {
          *CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextUpper());
          return NULL;
        }

        Int32 source_len = typ1.getNominalSize();

        Int32 maxLength = str_decoded_len_base64(source_len);

        retType = new HEAP SQLVarChar(HEAP, maxLength, TRUE);
      }
      break;

    default:
      {
	retType = (NAType *)ItemExpr::synthesizeType();
      }

    } // switch

  return retType;
}

// -----------------------------------------------------------------------
// member functions for class UDFunction.
// -----------------------------------------------------------------------
const NAType *UDFunction::synthesizeType()
{
  const NAType * retType = NULL;
  ValueId outVarId;

  // We assosiate the type of the UDFunction ItemExpr to that of the
  // first output of the Function. If the function has more than one output
  // that gets hadled when we flatten the MVF out and use the ValueIdProxies
  // to represent those outputs. See bindRowValues().
 
  if (udfDesc_ != NULL) 
  {
    outVarId = udfDesc_->getOutputColumnList()[0];
    const NAType &funcType = outVarId.getType();
    retType = &funcType;
  }

  return retType;
}

// -----------------------------------------------------------------------
// member functions for class Abs
// -----------------------------------------------------------------------

const NAType *Abs::synthesizeType()
{
  // The expression is ABS(<value>)
  // The result is the absolute value of the operand.

  // type cast any params
  ValueId vid = child(0)->getValueId();
  SQLDoublePrecision dp(NULL, TRUE);
  vid.coerceType(dp, NA_NUMERIC_TYPE);

  const NAType &typ1 = child(0)->getValueId().getType();
  if (typ1.getTypeQualifier() != NA_NUMERIC_TYPE)
    {
      // 4045 Absolute function is only defined for numeric types.
      *CmpCommon::diags() << DgSqlCode(-4045) << DgString0(getTextUpper());
      return NULL;
    }

  // now it's safe to cast the types to numeric type
  const NumericType &ntyp1 = (NumericType &) typ1;
  NAType *result = NULL;

  if (ntyp1.isExact())
    {
      Lng32 precision = (ntyp1.getMagnitude() + 9) / 10 + ntyp1.getScale();

      if (precision <= MAX_NUMERIC_PRECISION)
        {
          Int32 length;
          if (precision < 5)
            length = 2;
          else if (precision < 10)
            length = 4;
          else
            length = 8;

          result = new HEAP SQLNumeric(HEAP, length,
                                       precision,
                                       ntyp1.getScale(),
                                       ntyp1.isSigned());
        }
      else if (NOT ntyp1.isBigNum() && (ntyp1.getScale()==0) ) // this must be LargeInt
        result = new HEAP SQLLargeInt(HEAP, ntyp1.isSigned());
      else
        result = new HEAP SQLDoublePrecision(HEAP);
    }
  else
    {
      result = new HEAP SQLDoublePrecision(HEAP);
    }

  if (ntyp1.supportsSQLnullLogical()) result->setNullable(TRUE);

  return result;
}

// -----------------------------------------------------------------------
// member functions for class CodeVal
// -----------------------------------------------------------------------

const NAType *CodeVal::synthesizeType()
{
  // The expression is ASCII(<value>)/CODE_VALUE(<value>)
  // The result is the ASCII or UNICODE value of the first character in <value>.

  //
  // Type cast any params.
  //
  ValueId vid1 = child(0)->getValueId();
  vid1.coerceType(NA_CHARACTER_TYPE);

  const CharType &typ1 = (CharType&)child(0)->getValueId().getType();
  if (typ1.getTypeQualifier() != NA_CHARACTER_TYPE)
    {
      // 4043 The operand of a $0~String0 function must be character.
      *CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextUpper());
      return NULL;
    }

  switch (getOperatorType())
  {
    case ITM_NCHAR_MP_CODE_VALUE:
       if ( CharInfo::is_NCHAR_MP(typ1.getCharSet()) != TRUE )
       {
         // 4000: internal binder error. This should not happen because we set the
         // operator type according to the charset of the operand in NO_OPERATOR_TYPE
         // case first. If we get to here, then type code/operand has been changed
         // such that they do no match.
         *CmpCommon::diags() << DgSqlCode(-4000);
         return NULL;
       }
       break;

    case ITM_UNICODE_CODE_VALUE:
       if ( typ1.getCharSet() != CharInfo::UNICODE  ) {
         // 4000: internal binder error. This should not happen because we set the
         // operator type according to the charset of the operand in NO_OPERATOR_TYPE
         // case first. If we get to here, then type code/operand has been changed
         // such that they do no match.
         *CmpCommon::diags() << DgSqlCode(-4000);
         return NULL;
       }
       break;

    case ITM_ASCII:
       {
        CharInfo::CharSet cs = typ1.getCharSet();
        if ( CharInfo::maxBytesPerChar( cs ) != 1 ) {
         if ( cs == CharInfo::UNICODE ) {
         // 4106 The character set for the operand of string function
         // ascii/code_value must be $1~String1.
         *CmpCommon::diags() << DgSqlCode(-4106)  <<
              DgString0(getTextUpper()) <<
              DgString1(SQLCHARSETSTRING_ISO88591);

          return NULL;
         }
        }
       }
       break;
    case ITM_CODE_VALUE:
      // Before R2.4, code_value and ASCII functions returned the same result.
      // In R2.4, code_value will return the code value of the first
      // character. ASCII will return an error if the first character in the
      // buffer is not an ASCII character, for example, SJIS or UTF8
      // character. Add one case for code_value.
      
      break;

    case NO_OPERATOR_TYPE:
     {
       switch ( typ1.getCharSet() )
       {
          case CharInfo::KANJI_MP:
          case CharInfo::KSC5601_MP:
            setOperatorType(ITM_NCHAR_MP_CODE_VALUE);
            break;

          case CharInfo::UNICODE:
            setOperatorType(ITM_UNICODE_CODE_VALUE);
            break;

          case CharInfo::ISO88591:
          default:
            setOperatorType(ITM_CODE_VALUE);
            break;
       }
       break;
       // fall through
     }
    default:
      // Character set $0~string0 is not supported for function $1~string1
      *CmpCommon::diags() << DgSqlCode(-3403)  <<
           DgString0(getTextUpper()) <<
           DgString1(CharInfo::getCharSetName(typ1.getCharSet()));
      return NULL;
  }

  NAType *result = new (HEAP) SQLInt(HEAP, FALSE, typ1.supportsSQLnullLogical());

  return result;
}

// -----------------------------------------------------------------------
// member functions for class Aggregate
// -----------------------------------------------------------------------

const NAType *Aggregate::synthesizeType()
{
  const NAType *result;
  switch (getEffectiveOperatorType()) {
  case ITM_COUNT:
  case ITM_COUNT_NONULL:
    result = new HEAP
      SQLLargeInt(HEAP, TRUE /* 'long long' on NSK can't be unsigned */,
		  FALSE /*not null*/);
    break;
  case ITM_AVG:
  case ITM_SUM: {
    ValueId vid = child(0)->getValueId();
    vid.coerceType(NA_NUMERIC_TYPE);

    const NAType& operand = child(0)->castToItemExpr()->getValueId().getType();
    
    // If Top of a split aggregate, use the data type of the child
    // aggregate.
    if(topPartOfAggr()) {

      // If this is in a scalar groupby, it can potentially return NULL.
      // Make sure that the type is nullable.
      if (inScalarGroupBy())
        result = operand.synthesizeNullableType(HEAP);
      else
        result = operand.newCopy(HEAP);

    } else {
      result = synthAvgSum(operand, inScalarGroupBy());
    }
    break;
  }
  case ITM_MAX:
  case ITM_MIN: {
    ValueId vid = child(0)->getValueId();
    vid.coerceType(NA_CHARACTER_TYPE);

    const NAType& operand = child(0)->castToItemExpr()->getValueId().getType();

    if ( operand.getTypeQualifier() == NA_CHARACTER_TYPE ) {

       if (CmpCommon::wantCharSetInference()) {

         const CharType *charOp = (CharType*)&(vid.getType());

         const CharType* desiredType =
           CharType::findPushDownCharType(getDefaultCharSet, charOp, 0);

         if ( desiredType ) {
           // just push down the charset field. All other fields are
           // meaningless.
            vid.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);
         }
       }
    }

    if (!operand.isSupportedType())
    {
      emitDyadicTypeSQLnameMsg(-4041, operand, operand);
      return NULL;
    }

    if (inScalarGroupBy())
      result = operand.synthesizeNullableType(HEAP);
    else
      result = operand.newCopy(HEAP);
    break;
  }

  case ITM_GROUPING:
    {
	  // grouping result is an unsigned int (32 bit)
      result = new HEAP
        SQLInt(HEAP, FALSE /*unsigned*/,
               FALSE /*not null*/);
    }
  break;

  case ITM_ONE_ROW:
  case ITM_ONEROW:  
  {
    const NAType& operand = child(0)->castToItemExpr()->getValueId().getType();
    result = operand.synthesizeNullableType(HEAP);
    break;
  }
  case ITM_ONE_TRUE:
  case ITM_ANY_TRUE_MAX:
  case ITM_ANY_TRUE:
  {
    const SQLBooleanRelat& operand = (const SQLBooleanRelat &)
                   child(0)->castToItemExpr()->getValueId().getType();

    // The argument of a ONE/ANY TRUE must be of type SQLBoolean
    CMPASSERT(operand.getTypeQualifier() == NA_BOOLEAN_TYPE);

    result = new HEAP SQLBooleanRelat(HEAP, operand.canBeSQLUnknown());
    break;
  }
  default:
    result = ItemExpr::synthesizeType();
    break;
  }
  return result;
}

// -----------------------------------------------------------------------
// member functions for class AggMinMax
// -----------------------------------------------------------------------
const NAType *AggrMinMax::synthesizeType()
{
  const NAType *result;

  const NAType& operand = child(0)->castToItemExpr()->getValueId().getType();
  result = operand.newCopy(HEAP);

  return result;
}

// -----------------------------------------------------------------------
// member functions for class AggGrouping
// -----------------------------------------------------------------------
const NAType *AggrGrouping::synthesizeType()
{
  // result unsigned 32 bit integer
  const NAType *result = new HEAP SQLInt(HEAP, FALSE, FALSE);

  return result;
}

// -----------------------------------------------------------------------
// member functions for class PivotGroup
// -----------------------------------------------------------------------
const NAType *PivotGroup::synthesizeType()
{
  //for Character type, need to consider the charset
  //the output charset should be same as input child 0
  const NAType &operand = child(0)->getValueId().getType();
  if(operand.getTypeQualifier() == NA_CHARACTER_TYPE)
  {
    CharType & origType = (CharType &) operand;
    return new HEAP SQLVarChar(HEAP, maxLen_, TRUE, origType.isUpshifted(), FALSE, operand.getCharSet());
  }
  else
    return new HEAP SQLVarChar(HEAP, maxLen_, TRUE);
}

// -----------------------------------------------------------------------
// member functions for class AnsiUSERFunction
// -----------------------------------------------------------------------

static const Lng32 MAX_NT_DOMAIN_NAME_LEN = 30;
static const Lng32 MAX_NT_USERNAME_LEN	 = 20;
//the ldap username needs to fit into this field, so make them equal
static const Lng32 OPT_MAX_USERNAME_LEN	 = ComSqlId::MAX_LDAP_USER_NAME_LEN+1;

const NAType *AnsiUSERFunction::synthesizeType()
{
  return new HEAP SQLVarChar(HEAP, OPT_MAX_USERNAME_LEN, FALSE);
}

const NAType *MonadicUSERFunction::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid = child(0)->getValueId();

  SQLInt si(NULL, TRUE);
  vid.coerceType(si, NA_NUMERIC_TYPE);

  //
  // Check that the operands are compatible.
  //
  const NAType& typ1 = vid.getType();

  if (typ1.getTypeQualifier() != NA_NUMERIC_TYPE) {
    // 4043 The operand of a USER function must be character.
    *CmpCommon::diags() << DgSqlCode(-4045) << DgString0(getTextUpper());
    return NULL;
  }

  // now it's safe to cast the types to numeric type
  const NumericType &ntyp1 = (NumericType &) typ1;
  if (NOT ntyp1.isExact())
    {
      // 4046 USER function is only defined for exact numeric types.
      *CmpCommon::diags() << DgSqlCode(-4046) << DgString0(getTextUpper());
      return NULL;
    }

  if (ntyp1.getScale() != 0)
    {
      // 4047 Arguments of USER function must have a scale of 0.
      *CmpCommon::diags() << DgSqlCode(-4047) << DgString0(getTextUpper());
      return NULL;
    }

  //
  // Return the result.
  //
  return new HEAP SQLVarChar(HEAP, OPT_MAX_USERNAME_LEN, typ1.supportsSQLnullLogical());
}

const NAType *MonadicUSERIDFunction::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid = child(0)->getValueId();
  vid.coerceType(NA_CHARACTER_TYPE);
  //
  // Check that the operands are compatible.
  //
  const NAType& operand = vid.getType();
  if (operand.getTypeQualifier() != NA_CHARACTER_TYPE) {
    // 4043 The operand of a USERID function must be character.
    *CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextUpper());
    return NULL;
  }

  //
  // Return the result.
  //
  return new HEAP SQLVarChar(HEAP, OPT_MAX_USERNAME_LEN, operand.supportsSQLnullLogical());
}

// -----------------------------------------------------------------------
// member functions for class Assign
// -----------------------------------------------------------------------

const NAType *Assign::doSynthesizeType(ValueId & targetId, ValueId & sourceId)
{
  NABoolean ODBC = (CmpCommon::getDefault(ODBC_PROCESS) == DF_ON);
  NABoolean JDBC = (CmpCommon::getDefault(JDBC_PROCESS) == DF_ON);
  NABoolean isSourceNullConst = FALSE;
  NABoolean forceSourceParamToBeNullable = 
    (CmpCommon::getDefault(COMP_BOOL_173) == DF_ON);

  //
  // Type cast any params.
  //
  targetId = child(0)->castToItemExpr()->getValueId();
  sourceId = child(1)->castToItemExpr()->getValueId();
  
  const NAType& targetType = targetId.getType();

  NABoolean sourceIsUntypedParam = 
    (sourceId.getType().getTypeQualifier() == NA_UNKNOWN_TYPE);
 
  // Charset inference.
  const NAType& sourceType = sourceId.getType();
  targetId.coerceType(sourceType);

  sourceId.coerceType(targetType);

  // if this param is the source of an insert/update stmt coming in
  // from odbc/jdbc interface and is not nullable, then make it nullable
  // if the user has asked for it.
  if ((NOT sourceId.getType().supportsSQLnull()) &&
      (ODBC || JDBC) && (forceSourceParamToBeNullable) &&
      (sourceIsUntypedParam))
    {
      NAType &sourceType = (NAType&)(sourceId.getType());
      sourceType.setNullable(TRUE);

      // Propagate (pushDowntype()) this type to the children of this valueid
      // in case one of the children could not be typed.
      //
      const NAType* synthesizedNewType =
	sourceId.getItemExpr()->pushDownType(sourceType);
      
      sourceId.changeType(synthesizedNewType);
    }

  //
  // Check that the operands are compatible.
  //
  if (NOT targetId.getType().isCompatible(sourceId.getType())) {
    if ((CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON) &&
        (sourceId.getType().getTypeQualifier() != NA_RECORD_TYPE ) &&
        ((child(1)->getOperatorType() != ITM_CONSTANT) ||
	 (NOT ((ConstValue *) child(1).getPtr() )->isNull())))
      {
	// target type is not the same as source type.
	// Assignment allowed in special_1 mode.
	// bindNode will add an explicit CAST node.
	  // All supported incompatible conversions will be handled by CAST.
	return &targetType;
      }

    return NULL;
  }

  //
  // Return the result.
  //
  return &targetType;
}
const NAType *Assign::synthesizeType(const char * str1, const Lng32 int1)
{
  ValueId targetId, sourceId;
  const NAType * result = doSynthesizeType(targetId, sourceId);
  if (result == NULL)
  {
    emitDyadicTypeSQLnameMsg(arkcmpErrorISPWrongDataType,
                             targetId.getType(),
                             sourceId.getType(),
                             str1,
                             NULL,        // No str2 value
                             NULL,        // Default diags area
                             int1);
    return NULL;
  }

  //
  // Return the result.
  //
  return result;
}

const NAType *Assign::synthesizeType()
{
  ValueId targetId, sourceId;
  const NAType * result = doSynthesizeType(targetId, sourceId);
  if (result == NULL)
  {
    emitDyadicTypeSQLnameMsg(-4039,
                             targetId.getType(),
                             sourceId.getType(),
                             ToAnsiIdentifier(targetId.getNAColumn()->getColName()));
    return NULL;
  }
  //
  // Return the result.
  //
  return result;
}

// -----------------------------------------------------------------------
// member functions for class BaseColumn
// -----------------------------------------------------------------------

const NAType *BaseColumn::synthesizeType()
{
  return &getType();
}

// -----------------------------------------------------------------------
// member functions for class IndexColumn
// -----------------------------------------------------------------------

const NAType * IndexColumn::synthesizeType()
{
  return &indexColDefinition_.getType();
}

// -----------------------------------------------------------------------
// member functions for class Between
// -----------------------------------------------------------------------

const NAType *Between::synthesizeType()
{
  ItemExprList exprList1(child(0).getPtr(), HEAP);
  ItemExprList exprList2(child(1).getPtr(), HEAP);
  ItemExprList exprList3(child(2).getPtr(), HEAP);
  if (exprList1.entries() != exprList2.entries() OR
      exprList1.entries() != exprList3.entries()) {
    // 4040 The operands of a between predicate must be of equal degree.
    *CmpCommon::diags() << DgSqlCode(-4040);
    return NULL;
  }

  NABoolean allowsUnknown = FALSE;

  NABoolean allowIncompatibleComparison =
    ((CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON) &&
     (child(0)->castToItemExpr()->getOperatorType() != ITM_ONE_ROW) &&
     (child(1)->castToItemExpr()->getOperatorType() != ITM_ONE_ROW) &&
     (child(2)->castToItemExpr()->getOperatorType() != ITM_ONE_ROW) &&
     (child(0)->castToItemExpr()->getOperatorType() != ITM_ONEROW) &&
     (child(1)->castToItemExpr()->getOperatorType() != ITM_ONEROW) &&
     (child(2)->castToItemExpr()->getOperatorType() != ITM_ONEROW));
  
  for (CollIndex i = 0; i < exprList1.entries(); i++) {
    //
    // Type cast any params.
    //
    ValueId vid1 = exprList1[i]->getValueId();
    ValueId vid2 = exprList2[i]->getValueId();
    ValueId vid3 = exprList3[i]->getValueId();
    vid1.coerceType(vid2.getType());
    vid1.coerceType(vid3.getType(), NA_NUMERIC_TYPE);
    vid2.coerceType(vid1.getType());
    vid3.coerceType(vid1.getType());
    //
    // Check that the operands are comparable.
    //
    const NAType& op1 = vid1.getType();
    const NAType& op2 = vid2.getType();
    const NAType& op3 = vid3.getType();

    NABoolean compareOp2 = TRUE;
    NABoolean compareOp3 = TRUE;

    if (allowIncompatibleComparison)
    {
      if(((op1.getTypeQualifier() == NA_DATETIME_TYPE) &&
          (op2.getTypeQualifier() == NA_CHARACTER_TYPE) &&
          (vid2.getItemExpr()->getOperatorType() == ITM_CONSTANT)) ||
          ((op2.getTypeQualifier() == NA_DATETIME_TYPE) &&
	   (op1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
	   (vid1.getItemExpr()->getOperatorType() == ITM_CONSTANT)))
        compareOp2 = FALSE;

      if(((op1.getTypeQualifier() == NA_DATETIME_TYPE) &&
          (op3.getTypeQualifier() == NA_CHARACTER_TYPE) &&
          (vid3.getItemExpr()->getOperatorType() == ITM_CONSTANT)) ||
          ((op3.getTypeQualifier() == NA_DATETIME_TYPE) &&
	   (op1.getTypeQualifier() == NA_CHARACTER_TYPE) &&
	   (vid1.getItemExpr()->getOperatorType() == ITM_CONSTANT)))
        compareOp3 = FALSE;
    }

    if (op1.getTypeQualifier() == NA_CHARACTER_TYPE &&
        op2.getTypeQualifier() == NA_CHARACTER_TYPE &&
        op3.getTypeQualifier() == NA_CHARACTER_TYPE)
      {
        if ( CmpCommon::getDefault(ALLOW_IMPLICIT_CHAR_CASTING) == DF_ON )
          {
            compareOp2 = FALSE;
            compareOp3 = FALSE;
          }
      }

    if ((compareOp2) && (NOT op1.isComparable(op2, this)))  //## errmsg 4034 w/ unparse?
      return FALSE;
    if ((compareOp3) && (NOT op1.isComparable(op3, this)))  //## errmsg 4034 w/ unparse?
      return FALSE;

    // If any of the operands is nullable the result could be unknown
    allowsUnknown = allowsUnknown OR
                    op1.supportsSQLnullLogical() OR
                    op2.supportsSQLnullLogical() OR
                    op3.supportsSQLnullLogical();
  }
  //
  // Return the result.
  //
  return new HEAP SQLBooleanRelat(HEAP, allowsUnknown);
}

// -----------------------------------------------------------------------
// member functions for class Overlaps 
// -----------------------------------------------------------------------

const NAType *Overlaps::synthesizeType()
{
  const NAType &type1 = child(0)->getValueId().getType();
  const NAType &type2 = child(1)->getValueId().getType();
  const NAType &type3 = child(2)->getValueId().getType();
  const NAType &type4 = child(3)->getValueId().getType();

  //Syntax Rules:
  // ......
  //2) The declared types of the first field of <row value predicand 1> 
  //   and the first field of <row value predicand2> shall both be datetime
  //   data types and these data types shall be comparable.
  //3) The declared type of the second field of each <row value predicand> 
  //   shall be a datetime data type or INTERVAL.
  if (type1.getTypeQualifier() != NA_DATETIME_TYPE)
  {
    *CmpCommon::diags() << DgSqlCode(-4497) << DgString0("first")
                                            << DgString1("overlaps part1")
                                            << DgString2("datetime");
    return NULL;
  } 

  if ((type2.getTypeQualifier() != NA_DATETIME_TYPE)
      && (type2.getTypeQualifier() != NA_INTERVAL_TYPE))
  {
    *CmpCommon::diags() << DgSqlCode(-4497) << DgString0("second")
                                            << DgString1("overlaps part1")
                                            << DgString2("datetime or interval");
    return NULL;
  } 

  if (type3.getTypeQualifier() != NA_DATETIME_TYPE)
  {
    *CmpCommon::diags() << DgSqlCode(-4497) << DgString0("first")
                                            << DgString1("overlaps part2")
                                            << DgString2("datetime");
    return NULL;
  } 

  if ((type4.getTypeQualifier() != NA_DATETIME_TYPE)
      && (type4.getTypeQualifier() != NA_INTERVAL_TYPE))
  {
    *CmpCommon::diags() << DgSqlCode(-4497) << DgString0("second")
                                            << DgString1("overlaps part2")
                                            << DgString2("datetime or interval");
    return NULL;
  } 

  UInt32 allowIncompOper = NAType::ALLOW_INCOMP_OPER;
  if (NOT type1.isCompatible(type2, &allowIncompOper))
  {
    emitDyadicTypeSQLnameMsg(-4041, type1, type2);
    return NULL;
  }
  if (NOT type1.isCompatible(type3, &allowIncompOper))
  {
    emitDyadicTypeSQLnameMsg(-4041, type1, type3);
    return NULL;
  }

  if (NOT type3.isCompatible(type4, &allowIncompOper))
  {
    emitDyadicTypeSQLnameMsg(-4041, type3, type4);
    return NULL;
  }

  return new HEAP SQLBooleanRelat(HEAP, TRUE);
}


// -----------------------------------------------------------------------
// member functions for class BiArith
// -----------------------------------------------------------------------

const NAType *BiArith::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid1 = child(0)->getValueId();
  ValueId vid2 = child(1)->getValueId();

  if (vid1.getType().getTypeQualifier() == NA_UNKNOWN_TYPE &&
      vid2.getType().getTypeQualifier() == NA_NUMERIC_TYPE) {
    // if op1 is a param with unknown type and op2
    // is an exact numeric, type cast op1 to the default
    // numeric type
    const NumericType& op2 = (NumericType&)vid2.getType();
    if (op2.isExact())
      vid1.coerceType(NA_NUMERIC_TYPE);
  }
  else if (vid2.getType().getTypeQualifier() == NA_UNKNOWN_TYPE &&
	   vid1.getType().getTypeQualifier() == NA_NUMERIC_TYPE) {
    // if op2 is a param with unknown type and op1
    // is an exact numeric, type cast op2 to the default
    // numeric type
    const NumericType& op1 = (NumericType&)vid1.getType();
    if (op1.isExact())
      vid2.coerceType(NA_NUMERIC_TYPE);
  };

  vid1.coerceType(vid2.getType(), NA_NUMERIC_TYPE);
  vid2.coerceType(vid1.getType());

  UInt32 flags =
    ((CmpCommon::getDefault(LIMIT_MAX_NUMERIC_PRECISION) == DF_ON)
     ? NAType::LIMIT_MAX_NUMERIC_PRECISION : 0);

  if (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON)
    {
      flags |= NAType::MODE_SPECIAL_1;
    }

  if (CmpCommon::getDefault(MODE_SPECIAL_4) == DF_ON)
    {
      flags |= NAType::MODE_SPECIAL_4;
    }

  NABoolean limitPrecision = 
    ((flags & NAType::LIMIT_MAX_NUMERIC_PRECISION) != 0);

  //
  // Synthesize the result.
  //
  const NAType& operand1 = vid1.getType();
  const NAType& operand2 = vid2.getType();
  const NAType *result;
  switch (getOperatorType()) {
  case ITM_PLUS:
    result = operand1.synthesizeType(SYNTH_RULE_ADD, operand1, operand2, HEAP,
				     &flags);
    break;
  case ITM_MINUS:
    result = (getIntervalQualifier() == NULL)
           ? operand1.synthesizeType(SYNTH_RULE_SUB, operand1, operand2, HEAP,
				     &flags)
           : operand1.synthesizeTernary(SYNTH_RULE_SUB,
                                        operand1,
                                        operand2,
                                        *getIntervalQualifier(),
					HEAP);
    break;
  case ITM_TIMES:
    result = operand1.synthesizeType(SYNTH_RULE_MUL, operand1, operand2, HEAP,
				     &flags);
    break;
  case ITM_DIVIDE:
    {
      // if roundingMode is already set in this node, use it.
      // ignoreSpecialRounding() == TRUE  indicates rounding disabled, in
      // which case follow default roundingMode = 0.

      short roundingMode = getRoundingMode();
      if (roundingMode == 0 && ! ignoreSpecialRounding() )
	roundingMode = (short)CmpCommon::getDefaultLong(ROUNDING_MODE);

      if (roundingMode != 0)
	{
	  flags |= NAType::ROUND_RESULT;

	  // also limit precision, if rounding is to be done.
	  // Rounding is only supported using division rounding mechanism
	  // for exact and simple (no BigNums) numerics.
	  flags |= NAType::LIMIT_MAX_NUMERIC_PRECISION;
	}

      result = 
	operand1.synthesizeType(SYNTH_RULE_DIV, operand1, operand2, HEAP,
				&flags);
      if ((roundingMode != 0) && (result) && 
	  ((flags & NAType::RESULT_ROUNDED) != 0))
	{
	  // if rounding was requested and done, set that info in
	  // the BiArith node.
	  setRoundingMode(roundingMode);
	}
      else
	{
	  setRoundingMode(0);
	}
    }
    break;
  case ITM_EXPONENT:
    result = operand1.synthesizeType(SYNTH_RULE_EXP, operand1, operand2, HEAP,
				     &flags);
    break;
  default:
    result = ItemExpr::synthesizeType();
    break;
  }
  if (!result) {
    if (operand1.getTypeQualifier() == NA_RECORD_TYPE ||
        operand2.getTypeQualifier() == NA_RECORD_TYPE) {
      // 4020 arith operation not allowed on row-value-constructor.
      *CmpCommon::diags() << DgSqlCode(-4020);
    }
    else {
      const char *intervalQ;
      if (getIntervalQualifier())
	intervalQ = getIntervalQualifier()->getTypeSQLname(TRUE /*terse*/);
      else
	intervalQ = "";
      // 4034 The operation (~op1 ~operator ~op2) ~iq is not allowed.
      emitDyadicTypeSQLnameMsg(-4034, operand1, operand2,
			       getTextUpper(), intervalQ);
    }
  }
  return result;
}

// -----------------------------------------------------------------------
// member functions for class UnArith
// -----------------------------------------------------------------------

const NAType *UnArith::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid1 = child(0)->getValueId();

  if (vid1.getType().getTypeQualifier() == NA_UNKNOWN_TYPE)
    {
      vid1.coerceType(NA_BOOLEAN_TYPE);
    }
  
  const NAType& operand1 = vid1.getType();
  if (operand1.getFSDatatype() != REC_BOOLEAN)
    {
      *CmpCommon::diags() << DgSqlCode(-4034)
                          << DgString0("!")
                          << DgString1(child(0)->getText())
                          << DgString2("");
      
      return NULL;
    }

  NAType * retType = new HEAP SQLBooleanNative(HEAP, operand1.supportsSQLnull());

  return retType;
}

// -----------------------------------------------------------------------
// member functions for class BiLogic
// -----------------------------------------------------------------------

const NAType *BiLogic::synthesizeType()
{
  const SQLBooleanRelat& operand0 = (SQLBooleanRelat&) child(0).getValueId().getType();
  const SQLBooleanRelat& operand1 = (SQLBooleanRelat&) child(1).getValueId().getType();

  NABoolean allowsUnknown = operand0.canBeSQLUnknown() OR
                            operand1.canBeSQLUnknown();

  return new HEAP SQLBooleanRelat(HEAP, allowsUnknown);
}

// -----------------------------------------------------------------------
// member functions for class BiRelat
// -----------------------------------------------------------------------
const NAType *BiRelat::synthesizeType()
{
  ItemExpr *ie1 = child(0);
  ItemExpr *ie2 = child(1);

  if (ie1->getOperatorType() == ITM_ONE_ROW) ie1 = ie1->child(0);
  if (ie2->getOperatorType() == ITM_ONE_ROW) ie2 = ie2->child(0);
  ItemExprList exprList1(ie1, HEAP);
  ItemExprList exprList2(ie2, HEAP);

  // in some cases, we allow comparisons between 'incompatible' datatypes.
  // This is allowed if CQD is set, and it is a single valued scaler
  // predicate (a <op> b), and the comparison is done between a char/varhar
  // and numeric type.
  // In these conditions, the char type is converted to numeric by putting
  // a CAST node on top of it.
  // This incompatible comparison is not allowed if the statement is a DDL
  NABoolean allowIncompatibleComparison = FALSE;

  if ((CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON) &&
      (child(0)->castToItemExpr()->getOperatorType() != ITM_ONE_ROW) &&
      (child(1)->castToItemExpr()->getOperatorType() != ITM_ONE_ROW) &&
      (exprList1.entries() == 1) &&
      (exprList2.entries() == 1))
    allowIncompatibleComparison = TRUE;
  
  NABoolean allowsUnknown;
  if (!synthItemExprLists(exprList1, exprList2, allowIncompatibleComparison,
			  allowsUnknown, this))
    return NULL;
  return new HEAP SQLBooleanRelat(HEAP, allowsUnknown);
}

// -----------------------------------------------------------------------
// member functions for class BoolResult
// -----------------------------------------------------------------------

const NAType *BoolResult::synthesizeType()
{
  return new HEAP SQLBooleanRelat(HEAP, getOperatorType() == ITM_RETURN_NULL);
}

// -----------------------------------------------------------------------
// member functions for class BoolVal
// -----------------------------------------------------------------------

const NAType *BoolVal::synthesizeType()
{
  return new HEAP SQLBooleanRelat(HEAP, getOperatorType() == ITM_RETURN_NULL);
}

//------------------------------------------------------------------
// member functions for class RaiseError
//------------------------------------------------------------------
const NAType *RaiseError::synthesizeType()
{
  // -- Triggers
  if (getArity() == 1)
    {  // Verify the string expression is of character type.
      if (child(0)->getValueId().getType().getTypeQualifier() != NA_CHARACTER_TYPE)
	{
          //  parameter 3 must be of type string.
          *CmpCommon::diags() << DgSqlCode(-3185);
          return NULL;
	}
    }

  if (type_)
    return type_;

  return new HEAP SQLBooleanRelat(FALSE);	// can be overridden in IfThenElse
}

// -----------------------------------------------------------------------
// member functions for class IfThenElse
// -----------------------------------------------------------------------

const NAType *IfThenElse::synthesizeType()
{
  //
  // The ELSE clause may be a NULL pointer if this is part of a CASE statement
  // created by the generator.
  //
  ValueId thenId = child(1)->getValueId();
  if (child(2).getPtr() == NULL)
    return &thenId.getType();
  ValueId elseId = child(2)->getValueId();
  //
  // Type cast any params.
  //
  thenId.coerceType(elseId.getType(), NA_NUMERIC_TYPE);
  elseId.coerceType(thenId.getType());

  // infer the charset if unknown.
  if ( thenId.getType().getTypeQualifier() == NA_CHARACTER_TYPE &&
       elseId.getType().getTypeQualifier() == NA_CHARACTER_TYPE
     )
  {
    const CharType *thenCharType = (CharType*)&thenId.getType();
    const CharType *elseCharType = (CharType*)&elseId.getType();

    if (CmpCommon::wantCharSetInference()) {
      const CharType* desiredType =
        CharType::findPushDownCharType(getDefaultCharSet,
                                       thenCharType, elseCharType, 0);

      if ( desiredType ) {
        // just push down the charset field. All other fields are
        // meaningless.
        thenId.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);
        elseId.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);
      }
    }
  }

  //
  // Synthesize the result.
  //
  UInt32 flags =
    ((CmpCommon::getDefault(LIMIT_MAX_NUMERIC_PRECISION) == DF_ON)
     ? NAType::LIMIT_MAX_NUMERIC_PRECISION : 0);

  if (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON)
    {
      flags |= NAType::MODE_SPECIAL_1;
    }

  if (CmpCommon::getDefault(TYPE_UNIONED_CHAR_AS_VARCHAR) == DF_ON)
  {
    flags |= NAType::MAKE_RESULT_VARCHAR;
  }

  const NAType& thenType = thenId.getType();
  const NAType& elseType = elseId.getType();

  const NAType *result = thenType.synthesizeType(SYNTH_RULE_UNION,
                                                 thenType,
                                                 elseType,
						 HEAP,
                                     &flags);
  if (result == NULL) {

    // Ignore the RaiseError's type and pass thru the other operand's type
    if (thenId.getItemExpr()->getOperatorType() == ITM_RAISE_ERROR)
      return &elseType;
    if (elseId.getItemExpr()->getOperatorType() == ITM_RAISE_ERROR)
      return &thenType;

    // 4049 CASE can't have result types that are mixed
    emitDyadicTypeSQLnameMsg(-4049, thenType, elseType);
  }
  return result;
}

// -----------------------------------------------------------------------
// member functions for class Cast
// -----------------------------------------------------------------------

// Exact numeric can be cast to a single-field interval, and vice versa.
// In special_1 mode, numerics can be cast to multi-field intervals.
static NABoolean numericCastIsCompatible(const NAType &src, const NAType &tgt)
{
  if (src.getTypeQualifier() == NA_NUMERIC_TYPE &&
      tgt.getTypeQualifier() == NA_INTERVAL_TYPE &&
      tgt.isSupportedType()) 
    {
      if (CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON)
        return TRUE;
      
      NumericType&  numeric  = (NumericType&)src;
      IntervalType& interval = (IntervalType&)tgt;
      if (numeric.isExact())
        {
          if (CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON)
            return TRUE;
          else if (interval.getStartField() == interval.getEndField())
            return TRUE;
        }
    }
  //check for numeric to date conversion
  else if ((CmpCommon::getDefault(MODE_SPECIAL_1) == DF_ON) &&
	   (tgt.getTypeQualifier() == NA_DATETIME_TYPE) &&
	   (src.getTypeQualifier() == NA_NUMERIC_TYPE))
    {
      DatetimeType &dtType = (DatetimeType&)tgt;
      NumericType &numeric   = (NumericType&)src;
      if ((dtType.getPrecision() == SQLDTCODE_DATE) &&
	  (numeric.isExact()) &&
	  (NOT numeric.isBigNum()) &&
	  (numeric.getScale() == 0))
	{
	  return TRUE;
	}
    }

  return FALSE;
}

// Begin_Fix 10-040114-2431
// 02/18/2004
// Added as part of above mentioned fix
// synthesizeType for Narrow ensures that
// if we match the nullability of the child
// if that is required. This is done by setting
// Cast::matchChildType_ flag, if it is not already
// set. If Cast::matchChildType_ is not set, we set
// it and then unset it after Cast::synthesizeType().
// Setting Cast::matchChildType_ does more than just
// matching my child's nullability (please see class
// Cast in ItemFunc.h), therefore if it is not set
// initially we just unset it after calling
// Cast::synthesizeType().
const NAType *Narrow::synthesizeType()
{
  //check if we Cast::matchChildType_ is set
  NABoolean matchChildType = Cast::matchChildType();

  //if Cast::matchChildType_ is not set and we
  //want to force our nullability to be the same
  //as the child's nullability, set Cast::matchChildType_
  if ((!matchChildType) && (matchChildNullability_))
  {
    //setting this flag will force our nullability
    //to be the same as the child's nullability
    Cast::setMatchChildType(TRUE);
  }

  //call Cast::synthesizeType() to do the real type synthesis
  const NAType * result = Cast::synthesizeType();

  //if Cast::matchChildType_ was not initially set
  //then just unset it again.
  if ((!matchChildType) && (matchChildNullability_))
  {
    Cast::setMatchChildType(FALSE);
  }

  return result;
}
// End_Fix 10-040114-2431

const NAType *Cast::synthesizeType()
{
  //
  // Type cast any params.
  // Assert that we are bound, or created by Generator, so we have type info.
  //
  ValueId vid = child(0)->getValueId();
  CMPASSERT(vid != NULL_VALUE_ID);
  NABoolean untypedParam =
    ((child(0)->getOperatorType() == ITM_DYN_PARAM) &&
     (vid.getType().getTypeQualifier() == NA_UNKNOWN_TYPE));
  NAType * result = NULL;
  NABoolean typeChanged = FALSE;
  NABoolean sensitiveChanged = FALSE;
  NABoolean charsetChanged = FALSE;
  vid.coerceType(*getType());
  if (untypedParam)
    {
      // an untyped param is being typed using CAST.
      if (vid.getType().supportsSQLnull() != getType()->supportsSQLnull())
	{
	  // Set the null attribute to be the same as that of the cast node.
	  NAType * newType = vid.getType().newCopy(HEAP);
	  newType->setNullable(getType()->supportsSQLnull());
	  vid.changeType(newType);
	}

      // mark this cast node so code for it is not generated at code
      // generation time.
      setMatchChildType(TRUE);
    }

  // Fix for CR 10-010426-2464: If its child supports NULL but itself
  // does not AND the node's nullability is changable (i.e. not specified
  // explicitly in the application), set it same as its child.
  // NOTE: the new copy is necessary only because that the type_ is a const
  // member. If the const is ever removed, setNullable can be called
  // directly
  else if (vid.getType().supportsSQLnull() != getType()->supportsSQLnull())
    {
      if (matchChildType())
	// NOT NULL phrase not specified
	{
	  result = getType()->newCopy(HEAP);
	  result->setNullable(vid.getType());
	  typeChanged = TRUE;
	}
    }

  const NAType &src = vid.getType();
  const NAType &tgt = (typeChanged)? *result: *getType();

  NABuiltInTypeEnum srcQual = src.getTypeQualifier();
  NABuiltInTypeEnum tgtQual = tgt.getTypeQualifier();

  if ((DFS2REC::isCharacterString(src.getFSDatatype())) &&
      (DFS2REC::isCharacterString(tgt.getFSDatatype())))
    {
      const CharType &cSrc = (CharType&)src;
      CharType &cTgt       = (CharType&)tgt;
      if (cSrc.isCaseinsensitive() && (NOT cTgt.isCaseinsensitive()))
         sensitiveChanged = TRUE;
      if ( cSrc.getCharSet() != CharInfo::UnknownCharSet &&
         cTgt.getCharSet() == CharInfo::UnknownCharSet)
         charsetChanged = TRUE;
         if (sensitiveChanged || charsetChanged) 	
         {
          result = tgt.newCopy(HEAP);
          typeChanged = TRUE;
          if (sensitiveChanged)
           ((CharType*)result)->setCaseinsensitive(TRUE);
          if (charsetChanged)
           ((CharType*)result)->setCharSet(cSrc.getCharSet());
         }
    }

  const NAType &res = (typeChanged)? *result: *getType();
  //
  // The NULL constant can be cast to any type.
  //
  if (getExpr()->getOperatorType() == ITM_CONSTANT)
    if (((ConstValue*)getExpr())->isNull())
      return (typeChanged)? result: getType();
  //
  // See the chart in ANSI 6.10, a rather symmetrical piece of work.
  // Currently the "M" (Maybe) general subrules are being interpreted
  // as "Y" (Yes, legally castable).
  // Also, the Bitstring datatypes are not currently supported.
  // Internally, we use SQLBooleans for some predicate results (=ANY, e.g.).
  //
  // The diagonal of compatible types is fine.
  // Character types can be cast from or to with impunity.
  // Numeric can be cast to our internal SQLBoolean.
  // Exact numeric can be cast to a single-field interval, and vice versa.
  // Timestamp can be cast to time or date; date or time can cast to timestamp.
  //

  NABoolean legal = FALSE;

  // If both operands are char, they must be compatible (i.e., same charset);
  // they do NOT have to be comparable (i.e., collation/coercibility ok)!
  //
  // The result type takes the charset from the target, and:
  // - if target is a standard Ansi data type -- i.e., no COLLATE-clause --
  //   then DEFAULT collation and COERCIBLE coercibility are used,
  //   per Ansi 6.10 SR 8;
  // - if target is a Tandem-extension data type declaration --
  //   i.e., with an explicit COLLATE-clause, such as
  //		CAST(a AS CHAR(n) COLLATE SJIS)		--
  //   then the specified collation and coercibility (EXPLICIT) are used.
  //
  // Note that both of these come "for free":
  // - if no COLLATE-clause was specified,
  //   SqlParser.y and the CharType-ctor-defaults will give
  //   DEFAULT/COERCIBLE to the unadorned data type;
  // - if a COLLATE-clause was specified by user
  //   or if we are doing internal-expr casts --
  //     e.g., if our caller is
  //     propagateCoAndCoToXXX(), or CodeGen, or ColReference::bindNode --
  //   we simply use that collate/coerc.
  //
  // In fact, if these DIDN'T come for free, we would break INTERNAL casts:
  //	if (srcQual == NA_CHARACTER_TYPE && tgtQ == NA_CHARACTER_TYPE)[
  //	  if (((const CharType&)src).getCharSet() == tgt.getCharSet())[
  //	    CharType* newType = (CharType*)(tgtCT.newCopy(HEAP));
  //	    newType->setCoAndCo(CharInfo::DefaultCollation, COERCIBLE);
  //	    return newType;
  //
  //	But if in future we support Ansi "domains",
  // 	then need to revisit this, per Ansi 6.10 SR 1a + 8.
  //
  // In other words, a) use isCompatible(), not isComparable(),
  // and b) just pass the tgt's collation/coercibility along!
  //
  if (DFS2REC::isBinaryString(tgt.getFSDatatype()))
    legal = TRUE;
  else if (DFS2REC::isBinaryString(src.getFSDatatype()))
    legal = TRUE;
  else if ((srcQual == NA_LOB_TYPE) && (tgtQual != NA_LOB_TYPE))
    legal = FALSE;
  else if (charsetChanged && src.isCompatible(res))
    legal = TRUE;
  else if (src.isCompatible(tgt))
    legal = TRUE;
  else if (srcQual == NA_CHARACTER_TYPE || tgtQual == NA_CHARACTER_TYPE)
  {
    legal = (srcQual != tgtQual);  // if BOTH are CHAR: isCompatible() failed

    // disable casting KANJI/KSC5601 from/to any other data types. Same behavior as MP.
    if ( (srcQual == NA_CHARACTER_TYPE && CharInfo::is_NCHAR_MP(((const CharType&)src).getCharSet())) ||
         (tgtQual == NA_CHARACTER_TYPE && CharInfo::is_NCHAR_MP(((const CharType&)tgt).getCharSet()))
       )
       legal = FALSE;
// if BOTH are CHAR: make legal if both unknown charset
    if ( (srcQual == NA_CHARACTER_TYPE && (((const CharType&)src).getCharSet())==CharInfo::UnknownCharSet) &&
         (tgtQual == NA_CHARACTER_TYPE && (((const CharType&)tgt).getCharSet())==CharInfo::UnknownCharSet)
       )
       legal = TRUE;  

    if ( srcQual == tgtQual ) // if BOTH are CHAR
    {
       if ( CmpCommon::getDefault(ALLOW_IMPLICIT_CHAR_CASTING) == DF_ON )
       {
          legal = TRUE;
          //
          // NOTE: The Generator has code to throw in a Translate node if an
          // incompatible character set comparison is attempted.
          //
       }
    }
  }
  else if (srcQual == NA_NUMERIC_TYPE)
    legal = numericCastIsCompatible(src, tgt);
  else if (srcQual == NA_INTERVAL_TYPE)
    legal = numericCastIsCompatible(tgt, src);
  else if (srcQual == NA_DATETIME_TYPE && tgtQual == NA_NUMERIC_TYPE)
    {
      legal =
	(((DatetimeType&)src).getSubtype() == DatetimeType::SUBTYPE_SQLDate);
    }
  else if (srcQual == NA_DATETIME_TYPE && tgtQual == NA_DATETIME_TYPE) {
    legal =
      ((DatetimeType&)src).getSubtype() == DatetimeType::SUBTYPE_SQLTimestamp
				        ||
      ((DatetimeType&)tgt).getSubtype() == DatetimeType::SUBTYPE_SQLTimestamp
                                           ||
      ((DatetimeType&)tgt).fieldsOverlap((DatetimeType &)src);
     }

  if (!src.isSupportedType() || !tgt.isSupportedType())
    {
      if (src == tgt)
	{
	  legal = TRUE;
	}
      else
	{
	  legal = FALSE;
	}
    }
  if (legal)
    return (typeChanged)? result: getType();

  // 4035 can't cast type from src to tgt
  emitDyadicTypeSQLnameMsg(-4035, src, tgt);
  return NULL;
}

const NAType *CastConvert::synthesizeType()
{
  const NAType * type = Cast::synthesizeType();
  if (type == NULL)
    return NULL;

  NABuiltInTypeEnum qual = type->getTypeQualifier();
  if (qual != NA_CHARACTER_TYPE)
    return type;

  // return a char type that is large enough to hold the ascii
  // representation of the operand.
  const NAType &childType =
    child(0)->castToItemExpr()->getValueId().getType();
  Lng32 maxLength = childType.getDisplayLength(childType.getFSDatatype(),
                                              childType.getNominalSize(),
                                              childType.getPrecision(),
                                              childType.getScale(),
                                              0);
  CharType * origType = (CharType *)getType();
  if (DFS2REC::isAnyVarChar(origType->getFSDatatype()) == FALSE)
    type = new HEAP
      SQLChar(HEAP, maxLength, childType.supportsSQLnull(),
	      origType->isUpshifted());
  else
    type = new HEAP
      SQLVarChar(HEAP, maxLength, childType.supportsSQLnull(),
		 origType->isUpshifted());

  return type;
}

const NAType *CastType::synthesizeType()
{
  if (getType())
    return getType();

  ValueId childVid = child(0)->getValueId();
  NAType *newType = childVid.getType().newCopy(HEAP);
  if (makeNullable_ && (NOT newType->supportsSQLnull()))
    {
      newType->setNullable(TRUE);
    }
  changeType(newType);

  return getType();
}

// -----------------------------------------------------------------------
// member functions for class CharFunc
// -----------------------------------------------------------------------

const NAType *CharFunc::synthesizeType()
{
  // The expression is CHAR(<num>) or UNICODE_CHAR(<num>) or NCHAR(<num>)
  // The result is the character that has the
  // ASCII or UNICODE or <NATIONAL_CHARSET> code of <num>.

  //
  // Type cast any params.
  //
  SQLInt nType(NULL, FALSE);
  ValueId vid1 = child(0)->getValueId();
  vid1.coerceType(nType, NA_NUMERIC_TYPE);

  const NAType &typ1 = child(0)->getValueId().getType();
  if (typ1.getTypeQualifier() != NA_NUMERIC_TYPE)
    {
      // 4045 Operand must be numeric.
      *CmpCommon::diags() << DgSqlCode(-4045) << DgString0(getTextUpper());
      return NULL;
    }

  // now it's safe to cast the type to numeric type
  const NumericType &ntyp1 = (NumericType &) typ1;

  if (! ntyp1.isExact())
    {
      // 4046 Operand must be exact.
      *CmpCommon::diags() << DgSqlCode(-4046) << DgString0(getTextUpper());
      return NULL;
    }

  if (typ1.getScale() != 0)
    {
      // 4047 Operand must be not have scale.
      *CmpCommon::diags() << DgSqlCode(-4047) << DgString0(getTextUpper());
      return NULL;
    }

  CharInfo::CharSet cs_to_use    = charSet_ ;


  CharType *result;

  if(charSet_ == CharInfo::UCS2 || charSet_ < 0)  // UCS2, kanji and KSC5601_MP
    result = new (HEAP) SQLChar ( HEAP, 1,
                                  typ1.supportsSQLnullLogical(),
                                  FALSE/*not upshift*/,
                                  FALSE/*case sensitive*/,
                                  FALSE/*not varchar*/,
                                  charSet_);
  else
    result = new (HEAP) SQLVarChar(HEAP, CharInfo::maxBytesPerChar( cs_to_use )
                                  , typ1.supportsSQLnullLogical()
                                  , FALSE /*not upshift*/
                                  , FALSE /*case sensitive*/
                                  , cs_to_use
                                  , CharInfo::DefaultCollation
                                  , CharInfo::COERCIBLE
                                  );

  return result;
}

// -----------------------------------------------------------------------
// member functions for class ConvertHex
// -----------------------------------------------------------------------

const NAType *ConvertHex::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid = child(0)->getValueId();
  vid.coerceType(NA_CHARACTER_TYPE);
  //
  // Check that the operands are compatible.
  //
  const NAType* operand = &vid.getType();
  if (getOperatorType() == ITM_CONVERTFROMHEX)
    {
      if (operand->getTypeQualifier() != NA_CHARACTER_TYPE) {
	// 4043 The operand of an ConvertHex function must be character.
	*CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextUpper());
	return NULL;
      }

      const CharType* charType = (CharType*)operand;
      if ( charType->getCharSet() == CharInfo::UnknownCharSet ) {
        const CharType* desiredType =
            CharType::desiredCharType(CharInfo::ISO88591);
        vid.coerceType(*desiredType, NA_CHARACTER_TYPE);
        operand = &vid.getType();
      }

      // operand's size must be an even number since two hex characters make
      // up one result byte.
      const CharType* chartype1 = (CharType*)operand;
      if (NOT chartype1->sizeIsEven()) {
	*CmpCommon::diags() << DgSqlCode(-4068) << DgString0(getTextUpper());
	return NULL;
      }
    }

  Int32 maxLength;
  if (getOperatorType() == ITM_CONVERTTOHEX)
    maxLength = operand->getNominalSize() * 2;
  else
    maxLength = operand->getNominalSize() / 2;

  NAType * type;

  if ( operand -> getTypeQualifier() == NA_CHARACTER_TYPE &&
       ( (operand -> isVaryingLen() == TRUE) || 
       ( (const CharType*)operand)->getCharSet()==CharInfo::UTF8 )
     )
    type = new HEAP
      SQLVarChar(HEAP, maxLength, operand->supportsSQLnull());
  else
    type = new HEAP
      SQLChar(HEAP, maxLength, operand->supportsSQLnull());

  //
  // Return the result.
  //
  return type;
}

// -----------------------------------------------------------------------
// member functions for class CharLength
// -----------------------------------------------------------------------

const NAType *CharLength::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid = child(0)->getValueId();
  vid.coerceType(NA_CHARACTER_TYPE);
  //
  // Check that the operands are compatible.
  //
  const NAType& operand = vid.getType();
  if (operand.getTypeQualifier() != NA_CHARACTER_TYPE) {
    // 4043 The operand of a CHAR_LENGTH function must be character.
    *CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextForError());
    return NULL;
  }

  const CharType* charOperand = (CharType*)&(vid.getType());

  NAString defVal;
  NABoolean charsetInference =
      (CmpCommon::getDefault(INFER_CHARSET, defVal) == DF_ON);
  if(charsetInference)
     {
       const CharType* desiredType =
          CharType::findPushDownCharType(getDefaultCharSet, charOperand, 0);

       if ( desiredType )
       {
           // just push down the charset field. All other fields are
           // ignored.

           //coerceChildType((NAType&)*desiredType, NA_CHARACTER_TYPE);
           vid.coerceType(*desiredType, NA_CHARACTER_TYPE);

            // get the newly pushed-down types
           charOperand = (CharType*)&(vid.getType());
       }
     }

  if ( charOperand -> getCharSet() == CharInfo::UnknownCharSet ) {
      *CmpCommon::diags() << DgSqlCode(-4127);
      return NULL;
  }

  //
  // Return the result.
  //
  return new HEAP
    SQLInt(HEAP, FALSE  // unsigned
	   ,operand.supportsSQLnullLogical()
	   );
}

// -----------------------------------------------------------------------
// member functions for class Concat
// -----------------------------------------------------------------------

const NAType *Concat::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid1 = child(0)->getValueId();
  ValueId vid2 = child(1)->getValueId();
// these first two extra calls handle any parameters
// operands must be gotten twice because they could change types.

  CharInfo::CharSet new_cs = getFirstKnownCharSet(vid1, vid2, vid2);

  // If vid not aleady of NA_CHARACTER_TYPE, make varchar(255) of character set = new_cs
  vid1.coerceType(NA_CHARACTER_TYPE, new_cs);
  vid2.coerceType(NA_CHARACTER_TYPE, new_cs);

  vid1.coerceType(vid2.getType(), NA_CHARACTER_TYPE);
  vid2.coerceType(vid1.getType());
  //
  // Synthesize the result.
  //
  const NAType* operand1 = &vid1.getType();
  const NAType* operand2 = &vid2.getType();

  NABoolean isCaseInsensitive = FALSE;

  if ( operand1 -> getTypeQualifier() == NA_CHARACTER_TYPE &&
       operand2 -> getTypeQualifier() == NA_CHARACTER_TYPE
     )
  {
    const CharType *op1 = (CharType *)operand1;
    const CharType *op2 = (CharType *)operand2;

    if (CmpCommon::wantCharSetInference())  {

       const CharType* desiredType =
         CharType::findPushDownCharType(getDefaultCharSet, op1, op2, 0);

       if ( desiredType )
       {
         // just push down the charset field. All other fields are
         // meaningless.
          vid1.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);
          vid2.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);
       }
    }

    if (op1->isCaseinsensitive() || op2->isCaseinsensitive())
    {
      isCaseInsensitive = TRUE;
    }
  }

  const NAType *result = operand1->synthesizeType(SYNTH_RULE_CONCAT,
                                                 *operand1,
                                                 *operand2,
						 HEAP);
  if (result == NULL)
    {
      // 4034 The operation (~op1 ~operator ~op2) is not allowed.
      emitDyadicTypeSQLnameMsg(-4034, *operand1, *operand2, getTextUpper());
      return result;
    }

  if ((result->getTypeQualifier() == NA_CHARACTER_TYPE) &&
      (isCaseInsensitive))
    {
      CharType *ct = (CharType *)result;
      ct->setCaseinsensitive(TRUE);
    }

  return result;
}

// -----------------------------------------------------------------------
// member functions for class ConstValue
// -----------------------------------------------------------------------

const NAType * ConstValue::synthesizeType()
{
  return getType();
}

// -----------------------------------------------------------------------
// member functions for class ConvertTimestamp
// -----------------------------------------------------------------------

const NAType *ConvertTimestamp::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid = child(0)->getValueId();
  SQLLargeInt largeintType(NULL);
  vid.coerceType(largeintType);
  //
  // Check that the operands are compatible.
  //
  const NAType& operand = vid.getType();
  if (operand.getTypeQualifier() != NA_NUMERIC_TYPE OR
      NOT ((NumericType&) operand).isExact()) {
    // 4070 The operand of a CONVERTTIMESTAMP function must be exact numeric.
    *CmpCommon::diags() << DgSqlCode(-4070) << DgString0(getTextUpper());
    return NULL;
  }
  //
  // Return the result.
  //
  return new HEAP SQLTimestamp (HEAP, operand.supportsSQLnullLogical(),
                               SQLTimestamp::DEFAULT_FRACTION_PRECISION);

}

// -----------------------------------------------------------------------
// member functions for class SleepFunction 
// -----------------------------------------------------------------------
const NAType *SleepFunction::synthesizeType()
{
    return  new HEAP SQLInt(HEAP, TRUE, TRUE);
}

// -----------------------------------------------------------------------
// member functions for class UnixTimestamp
// -----------------------------------------------------------------------
const NAType *UnixTimestamp::synthesizeType()
{
  return new HEAP SQLLargeInt(HEAP, FALSE,FALSE);
}

// -----------------------------------------------------------------------
// member functions for class CurrentTimestamp
// -----------------------------------------------------------------------

const NAType *CurrentTimestamp::synthesizeType()
{
  return new HEAP SQLTimestamp (HEAP, FALSE,
                                SQLTimestamp::DEFAULT_FRACTION_PRECISION);
}

// -----------------------------------------------------------------------
// member functions for class InternalTimestamp
// -----------------------------------------------------------------------

const NAType *InternalTimestamp::synthesizeType()
{
  return new HEAP SQLTimestamp(HEAP, FALSE);
}

// -----------------------------------------------------------------------
// member functions for class CurrentTimestampRunning
// -----------------------------------------------------------------------

const NAType *CurrentTimestampRunning::synthesizeType()
{
  return new HEAP SQLTimestamp(HEAP, FALSE);
}

// -----------------------------------------------------------------------
// member functions for class DateFormat
// -----------------------------------------------------------------------
const NAType *DateFormat::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid = child(0)->getValueId();
  SQLTimestamp timestampType(NULL);
  vid.coerceType(timestampType);
  //
  // Check that the operands are compatible.
  //
  if (!vid.getType().isSupportedType()) {
    // 4071 The operand of a DATEFORMAT function must be a datetime.
    *CmpCommon::diags() << DgSqlCode(-4071) << DgString0(getTextUpper());
    return NULL;
  }

  if ((getDateFormat() == TIME_FORMAT_STR) &&
      ((vid.getType().getTypeQualifier() != NA_NUMERIC_TYPE) &&
       (vid.getType().getTypeQualifier() != NA_CHARACTER_TYPE) &&
       (vid.getType().getTypeQualifier() != NA_DATETIME_TYPE))) {
    // 4071 The operand of a DATEFORMAT function must be a datetime.
    *CmpCommon::diags() << DgSqlCode(-4071) << DgString0(getTextUpper());
    return NULL;
  }

  Lng32 length = 0;
  NABoolean formatAsDate = FALSE;
  NABoolean formatAsTimestamp = FALSE;
  NABoolean formatAsTime = FALSE;

  if (vid.getType().getTypeQualifier() == NA_DATETIME_TYPE)
    {
      const DatetimeType& operand = (DatetimeType &)vid.getType();
      Lng32 frmt = ExpDatetime::getDatetimeFormat(formatStr_.data());

      if (wasDateformat_)
        {
	  length = operand.getDisplayLength();
          if(operand.containsField(REC_DATE_HOUR) && 
             ((frmt == ExpDatetime::DATETIME_FORMAT_USA) ||
              (frmt == ExpDatetime::DATETIME_FORMAT_TS7)))
	    length += 3; // add 3 for a blank and "am" or "pm"
        }
      else
        {
          length = ExpDatetime::getDatetimeFormatMaxLen(frmt);
        }
    }
  else if (vid.getType().getTypeQualifier() == NA_CHARACTER_TYPE)
    {
      length = formatStr_.length();
      if (getDateFormat() == DATE_FORMAT_STR)
	{
	  formatAsDate = TRUE;
	}
      else if (getDateFormat() == TIMESTAMP_FORMAT_STR)
	{
	  formatAsTimestamp = TRUE;
	}
      else if (getDateFormat() == TIME_FORMAT_STR)
	{
	  formatAsTime = TRUE;
	}
    }
  else if (vid.getType().getTypeQualifier() == NA_NUMERIC_TYPE)
    {
      length = formatStr_.length();
    }

  if (formatAsDate)
    return new HEAP SQLDate(HEAP, vid.getType().supportsSQLnullLogical());
  else if (formatAsTimestamp)
    return new HEAP SQLTimestamp(HEAP, vid.getType().supportsSQLnullLogical());
  else if (formatAsTime)
    return new HEAP SQLTime(HEAP, vid.getType().supportsSQLnullLogical());
  else
    return new HEAP SQLChar(HEAP, length, vid.getType().supportsSQLnullLogical());
}

// -----------------------------------------------------------------------
// member functions for class DayOfWeek
// -----------------------------------------------------------------------

const NAType *DayOfWeek::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid = child(0)->getValueId();
  SQLTimestamp timestampType(NULL);
  vid.coerceType(timestampType);
  //
  // Check that the operand contains a DAY field
  //
  const NAType& operand = vid.getType();
  if ((operand.getTypeQualifier() != NA_DATETIME_TYPE) ||
      (!((DatetimeType&) operand).containsField (REC_DATE_YEAR)) ||
      (!((DatetimeType&) operand).containsField (REC_DATE_MONTH)) ||
      (!((DatetimeType&) operand).containsField (REC_DATE_DAY)))  {
    // Need to reword:
    // 4072 The operand of function DAYOFWEEK must be a Datetime containing a DAY field.
    *CmpCommon::diags() << DgSqlCode(-4072) << DgString0(getTextUpper()) << DgString1("YEAR, MONTH and DAY");
    return NULL;
  }
  //
  // Return the result.
  //
  const Int16 DisAmbiguate = 0; // added for 64bit project
  return new HEAP SQLNumeric(HEAP, FALSE, 1, 0, DisAmbiguate,
                             operand.supportsSQLnullLogical());
}

// -----------------------------------------------------------------------
// member functions for class DynamicParam
// -----------------------------------------------------------------------

const NAType *DynamicParam::synthesizeType()
{
  // dynamic params are always nullable.
  return new HEAP SQLUnknown(HEAP, TRUE);
}


const NAType *ExplodeVarchar::synthesizeType()
{
  return getType();
}

const NAType *Format::synthesizeType()
{
  NAType * retType = NULL;

  retType = (NAType *)ItemExpr::synthesizeType();

  return retType;
}

// -----------------------------------------------------------------------
// member functions for class RoutineParam
// -----------------------------------------------------------------------

const NAType *RoutineParam::synthesizeType()
{
  return getType();
}


// -----------------------------------------------------------------------
// member functions for class Function -- a catchall for those funx which
// don't have their own virtual synthType()
// -----------------------------------------------------------------------

const NAType *Function::synthesizeType()
{
  // Function derives directly from ItemExpr, so safe to do this
  const NAType *result = ItemExpr::synthesizeType();

  return result;
}

// -----------------------------------------------------------------------
// member functions for class Hash
// -----------------------------------------------------------------------

const NAType *Hash::synthesizeType()
{
  // result of hash function is always a non-nullable, unsigned 32 bit integer
  return new HEAP SQLInt(HEAP, FALSE, FALSE);
}

// -----------------------------------------------------------------------
// member functions for class HashComb
// -----------------------------------------------------------------------

NABoolean HashCommon::areChildrenExactNumeric(Lng32 left, Lng32 right)
{
  const NAType &typ1 = child(left)->getValueId().getType();
  const NAType &typ2 = child(right)->getValueId().getType();

  if (typ1.getTypeQualifier() != NA_NUMERIC_TYPE ||
      typ2.getTypeQualifier() != NA_NUMERIC_TYPE)
    return FALSE;

  // now it's safe to cast the types to numeric type
  const NumericType &ntyp1 = (NumericType &) typ1;
  const NumericType &ntyp2 = (NumericType &) typ2;

  return (ntyp1.isExact() AND ntyp2.isExact() AND
          ntyp1.binaryPrecision() AND ntyp2.binaryPrecision() AND
	  ntyp1.getPrecision() == ntyp2.getPrecision() AND
	  ntyp1.isUnsigned() AND ntyp2.isUnsigned());
}

const NAType *HashComb::synthesizeType()
{
  // Both dividend and divisor must be exact numeric with scale 0.
  // The result has values from 0 to <divisor> - 1 and therefore
  // can always fit into the data type of the divisor.

  // HashComb is an internal operator and errors are fatal
  CMPASSERT(areChildrenExactNumeric(0, 1));

  // result of hashcomb function is always a non-nullable,
  // unsigned 32 bit integer
  return new HEAP SQLInt(HEAP, FALSE, FALSE);
}

const NAType *HiveHashComb::synthesizeType()
{
  // Both dividend and divisor must be exact numeric with scale 0.
  // The result has values from 0 to <divisor> - 1 and therefore
  // can always fit into the data type of the divisor.

  // HashComb is an internal operator and errors are fatal
  CMPASSERT(areChildrenExactNumeric(0, 1));

  // result of hashcomb function is always a non-nullable,
  // unsigned 32 bit integer
  return new HEAP SQLInt(HEAP, FALSE, FALSE);
}

// -----------------------------------------------------------------------
// member functions for class HashDistHash
// Hash Function used by Hash Partitioning. This function cannot change
// once Hash Partitioning is released!  Defined for all data types,
// returns a 32 bit non-nullable hash value for the data item.
// -----------------------------------------------------------------------

const NAType *HashDistPartHash::synthesizeType()
{
  // result of hash function is always a non-nullable, unsigned 32 bit integer
  return new HEAP SQLInt(HEAP, FALSE, FALSE);
}

// -----------------------------------------------------------------------
// member functions for class HiveHash
// -----------------------------------------------------------------------
const NAType *HiveHash::synthesizeType()
{
  // result of hivehash function is always a non-nullable, unsigned 32 bit integer
  return new HEAP SQLInt(HEAP, FALSE, FALSE);
}

// -----------------------------------------------------------------------
// member functions for class HashDistHashComb
// This function is used to combine two hash values to produce a new
// hash value. Used by Hash Partitioning. This function cannot change
// once Hash Partitioning is released!  Defined for all data types,
// returns a 32 bit non-nullable hash value for the data item.
// -----------------------------------------------------------------------

const NAType *HashDistPartHashComb::synthesizeType()
{
  // Both dividend and divisor must be exact numeric with scale 0.
  // The result has values from 0 to <divisor> - 1 and therefore
  // can always fit into the data type of the divisor.

  const NAType &typ1 = child(0)->getValueId().getType();
  const NAType &typ2 = child(1)->getValueId().getType();

  // HashDistHashComb is an internal operator and errors are fatal
  CMPASSERT(typ1.getTypeQualifier() == NA_NUMERIC_TYPE AND
	    typ2.getTypeQualifier() == NA_NUMERIC_TYPE);

  // now it's safe to cast the types to numeric type
  const NumericType &ntyp1 = (NumericType &) typ1;
  const NumericType &ntyp2 = (NumericType &) typ2;

  // Make sure both operands are SQLInt.
  //
  CMPASSERT(ntyp1.getFSDatatype() == REC_BIN32_UNSIGNED AND
            ntyp2.getFSDatatype() == REC_BIN32_UNSIGNED AND
            ntyp1.isAnyUnsignedInt() AND
            ntyp2.isAnyUnsignedInt() AND
	    ntyp1.getPrecision() == SQL_UINT_PRECISION AND
            ntyp2.getPrecision() == SQL_UINT_PRECISION);

  // result of hashcomb function is always a non-nullable,
  // unsigned 32 bit integer
  return new HEAP SQLInt(HEAP, FALSE, FALSE);
}

// -----------------------------------------------------------------------
// member functions for class ReplaceNull
// -----------------------------------------------------------------------

const NAType *ReplaceNull::synthesizeType()
{
  // result of ReplaceNull is always the same as the second argument
  // except it is non nullable.
  ValueId childId = child(1)->getValueId();
  NAType *newType = childId.getType().newCopy(HEAP);
  //newType->setNullable(FALSE);
  return newType;
}

// -----------------------------------------------------------------------
// member functions for class JulianTimestamp
// -----------------------------------------------------------------------

const NAType *JulianTimestamp::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid = child(0)->getValueId();
  SQLTimestamp timestampType(NULL);
  vid.coerceType(timestampType);
  //
  // Check that the operands are compatible.
  //
  const NAType& operand = vid.getType();
  if (operand.getTypeQualifier() != NA_DATETIME_TYPE) {
    // 4071 The operand of a JULIANTIMESTAMP function must be a datetime.
    *CmpCommon::diags() << DgSqlCode(-4071) << DgString0(getTextUpper());
    return NULL;
  }
  //
  // Return the result.
  //
  return new HEAP SQLLargeInt(HEAP, TRUE, operand.supportsSQLnullLogical());
}

// -----------------------------------------------------------------------
// member functions for class StatementExecutionCount
// -----------------------------------------------------------------------
const NAType * StatementExecutionCount::synthesizeType()
{
  return new HEAP SQLLargeInt(HEAP, TRUE,FALSE);
}

// -----------------------------------------------------------------------
// member functions for class CurrentTransId
// -----------------------------------------------------------------------
const NAType * CurrentTransId::synthesizeType()
{
  return new HEAP SQLLargeInt(HEAP, TRUE,FALSE);
}

// -----------------------------------------------------------------------
// member functions for class BitOperFunc
// -----------------------------------------------------------------------
const NAType *BitOperFunc::synthesizeType()
{
  NABoolean nullable = FALSE;
  for (Int32 i = 0; i < getArity(); i++)
    {
      // type cast any params
      ValueId vid = child(i)->getValueId();
      // untyped param operands are typed as Int32 Unsigned.
      SQLInt dp(NULL, FALSE);
      vid.coerceType(dp, NA_NUMERIC_TYPE);

      const NAType &typ = vid.getType();

      if (typ.getTypeQualifier() != NA_NUMERIC_TYPE)
	{
	  // 4045 operand must be numeric.
	  // 4052 2nd operand must be numeric.
	  // 4059 1st operand must be numeric.
	  if (getArity() == 1)
	    *CmpCommon::diags()
    	      << DgSqlCode(-4045) << DgString0(getTextUpper());
	  else
	    {
	      if (i == 0)
	      *CmpCommon::diags()
		<< DgSqlCode(-4059) << DgString0(getTextUpper());
	      else if (i == 1)
		*CmpCommon::diags()
		  << DgSqlCode(-4052) << DgString0(getTextUpper());
	      else
		*CmpCommon::diags()
		  << DgSqlCode(-4053) << DgString0(getTextUpper());
	    }

	  return NULL;
	}

      if (typ.supportsSQLnullLogical())
	nullable = TRUE;
    }

  const NAType *result = NULL;

  switch (getOperatorType())
    {
    case ITM_BITAND:
    case ITM_BITOR:
    case ITM_BITXOR:
      {
	CMPASSERT(getArity() == 2);

	// now it's safe to cast the types to numeric type
	const NumericType &ntyp1 = (NumericType &) child(0)->getValueId().getType();
	const NumericType &ntyp2 = (NumericType &) child(1)->getValueId().getType();

	if (NOT ntyp1.isExact() OR NOT ntyp2.isExact() OR
	    ntyp1.isBigNum() OR ntyp2.isBigNum())
	  {
	    // 4046 BIT operation is only defined for exact numeric types.
	    *CmpCommon::diags() << DgSqlCode(-4046) << DgString0(getTextUpper());
	    return NULL;
	  }
	
	if (ntyp1.getScale() != 0 OR
	    ntyp2.getScale() != 0)
	  {
	    // 4047 Arguments of BIT operation must both have a scale of 0.
	    *CmpCommon::diags() << DgSqlCode(-4047) << DgString0(getTextUpper());
	    return NULL;
	  }
	
	UInt32 flags = NAType::MAKE_UNION_RESULT_BINARY;

	result = ntyp1.synthesizeType(
	     SYNTH_RULE_UNION,
	     ntyp1,
	     ntyp2,
	     HEAP,
	     &flags);
      }
    break;

    case ITM_BITNOT:
      {
	CMPASSERT(getArity() == 1);

	// now it's safe to cast the types to numeric type
	const NumericType &ntyp1 = (NumericType &) child(0)->getValueId().getType();

	if (NOT ntyp1.isExact() OR ntyp1.isBigNum())
	  {
	    // 4046 BIT operation is only defined for exact numeric types.
	    *CmpCommon::diags() << DgSqlCode(-4046) << DgString0(getTextUpper());
	    return NULL;
	  }
	
	if (ntyp1.getScale() != 0)
	  {
	    // 4047 Arguments of BIT operation must both have a scale of 0.
	    *CmpCommon::diags() << DgSqlCode(-4047) << DgString0(getTextUpper());
	    return NULL;
	  }

	// result of BITNOT is the same type as the operand, if the
	// operand is binary.
	// If operand is decimal, then convert it to equivalent binary.
	NAType * result1 = NULL;
	if (ntyp1.binaryPrecision())
	  result1 = (NumericType*)ntyp1.newCopy(HEAP);
	else
	  {
	    const Int16 DisAmbiguate = 0;
	    result1 = new HEAP SQLNumeric(HEAP, NOT ntyp1.isUnsigned(),
					  ntyp1.getPrecision(),
					  ntyp1.getScale(),
					  DisAmbiguate); // added for 64bit proj.
	  }

	result1->setNullable(nullable);
	result = result1;
      }
    break;

    case ITM_BITEXTRACT:
      {
	CMPASSERT(getArity() == 3);

	// now it's safe to cast the types to numeric type
	const NumericType &ntyp1 = (NumericType &) child(0)->getValueId().getType();
	const NumericType &ntyp2 = (NumericType &) child(1)->getValueId().getType();
	const NumericType &ntyp3 = (NumericType &) child(2)->getValueId().getType();

	if (ntyp1.isBigNum() ||
	    ntyp1.isDecimal())
	  {
	    // 4046 BIT operation is only defined for exact numeric types.
	    *CmpCommon::diags() << DgSqlCode(-4046) << DgString0(getTextUpper());
	    return NULL;
	  }
	  
	if ((NOT ntyp2.isExact()) ||
	    (NOT ntyp3.isExact()))
	  {
	    // 4046 BIT operation is only defined for exact numeric types.
	    *CmpCommon::diags() << DgSqlCode(-4046) << DgString0(getTextUpper());
	    return NULL;
	  }
	
	if ((ntyp2.getScale() != 0) ||
	    (ntyp3.getScale() != 0))
	  {
	    // 4047 Arguments of BIT operation must both have a scale of 0.
	    *CmpCommon::diags() << DgSqlCode(-4047) << DgString0(getTextUpper());
	    return NULL;
	  }

	// result can contain as many bits as the length of the operand.
	// Make the result an Int32 or Int64.
	NAType * result1 = NULL;
	if (ntyp1.getNominalSize() <= 9)
	  result = new HEAP SQLInt(HEAP, TRUE, nullable);
	else
	  result = new HEAP SQLLargeInt(HEAP, TRUE, nullable);
      }
    break;

    default:
      {
	// 4000 Internal Error. This function not supported.
	*CmpCommon::diags() << DgSqlCode(-4000);
	result = NULL;
      }
      break;
    }

  return result;
}

// -----------------------------------------------------------------------
// member functions for class MathFunc
// -----------------------------------------------------------------------
	
NAType* MathFunc::findReturnTypeForFloorCeil(NABoolean nullable)
{
  assert (getArity() == 1);

  for (Int32 i = 0; i < getArity(); i++) {
      ValueId vid = child(i)->getValueId();
      const NAType &typ = vid.getType();
      
      if (typ.getTypeQualifier() != NA_NUMERIC_TYPE)
        break;

      const NumericType& nuTyp = (NumericType&)(typ);

      // If the child is a SQL integer, just return the same type
      // for the function.
      if ( nuTyp.isInteger() ) 
         return nuTyp.newCopy(HEAP);

      // Here we only return a modified data type for SQLDecimal 
      // or SQLNumeric. For all others, we return SQLDoublePrecision.
      if ( !nuTyp.decimalPrecision() )
        break;

      switch ( nuTyp.getFSDatatype() ) {

        // for SQLDecimal
        case REC_DECIMAL_UNSIGNED:
        case REC_DECIMAL_LSE:
          return new HEAP SQLDecimal(HEAP, nuTyp.getNominalSize()
                                    ,0
                                    ,!nuTyp.isUnsigned()
                                    ,nuTyp.supportsSQLnull()
                                    );
          break;

        // for SQLNumeric
        case REC_BIN8_UNSIGNED:
        case REC_BIN16_UNSIGNED:
        case REC_BIN32_UNSIGNED:
        case REC_BIN64_UNSIGNED:
        case REC_BIN8_SIGNED:
        case REC_BIN16_SIGNED:
        case REC_BIN32_SIGNED:
        case REC_BIN64_SIGNED:
           return new HEAP SQLNumeric(HEAP, nuTyp.getNominalSize()
                                     ,nuTyp.getPrecision()
                                     ,0
                                     ,!nuTyp.isUnsigned()
                                     ,nuTyp.supportsSQLnull()
                                     );

        default: 
          break;
    }
  }
  return new HEAP SQLDoublePrecision(HEAP, nullable);
}

const NAType *MathFunc::synthesizeType()
{
  CMPASSERT(getArity() <= 2);

  NABoolean nullable = FALSE;

  for (Int32 i = 0; i < getArity(); i++)
    {
      // type cast any params
      ValueId vid = child(i)->getValueId();

      SQLDoublePrecision dp(NULL, TRUE);
      vid.coerceType(dp, NA_NUMERIC_TYPE);

      const NAType &typ = vid.getType();

      if (typ.getTypeQualifier() != NA_NUMERIC_TYPE)
	{
	  // 4045 operand must be numeric.
	  // 4052 2nd operand must be numeric.
	  // 4059 1st operand must be numeric.
	  if (getArity() == 1)
	    *CmpCommon::diags()
    	      << DgSqlCode(-4045) << DgString0(getTextUpper());
	  else
	    *CmpCommon::diags()
	      << DgSqlCode(i ? -4052 : -4059) << DgString0(getTextUpper());

	  return NULL;
	}

      if (typ.supportsSQLnullLogical())
	nullable = TRUE;
    }

  const NAType *result = NULL;

  switch (getOperatorType())
    {
    case ITM_ABS:
    case ITM_ACOS:
    case ITM_ASIN:
    case ITM_ATAN:
    case ITM_ATAN2:
    case ITM_COS:
    case ITM_COSH:
    case ITM_DEGREES:
    case ITM_EXP:
    case ITM_EXPONENT:
    case ITM_LOG:
    case ITM_LOG10:
    case ITM_LOG2:
    case ITM_PI:
    case ITM_POWER:
    case ITM_RADIANS:
    case ITM_SCALE_TRUNC:
    case ITM_SIN:
    case ITM_SINH:
    case ITM_SQRT:
    case ITM_TAN:
    case ITM_TANH:
      {
	result = new HEAP SQLDoublePrecision(HEAP, nullable);
      }
      break;

    case ITM_ROUND:
      {
        // if the first operand of ROUND is BigNum, then return
        // the BigNum type; otherwise return DOUBLE PRECISION
        ValueId vid0 = child(0)->getValueId();
        const NAType &typ0 = vid0.getType();
        if (((const NumericType &)typ0).isBigNum())
          {
            const SQLBigNum & btyp0 = (const SQLBigNum &)typ0;
            Lng32 precision = btyp0.getPrecision();
            if (precision < CmpCommon::getDefaultNumeric(MAX_NUMERIC_PRECISION_ALLOWED))
              precision++;  // increase precision when we can since rounding up might add a digit
            result = new HEAP SQLBigNum(HEAP, 
                                        precision,
                                        btyp0.getScale(),
	                                btyp0.isARealBigNum(),
	                                !btyp0.isUnsigned(), 
	                                btyp0.supportsSQLnull());
          }
        else
          {
            result = new HEAP SQLDoublePrecision(HEAP, nullable);
          }
      }
      break;

    case ITM_FLOOR:
    case ITM_CEIL:
      {
	result = findReturnTypeForFloorCeil(nullable);
      }
      break;

    default:
      {
	// 4000 Internal Error. This function not supported.
	*CmpCommon::diags() << DgSqlCode(-4000);
	result = NULL;
      }
      break;
    }

  return result;
}

// -----------------------------------------------------------------------
// member functions for class Modulus
// -----------------------------------------------------------------------

const NAType *Modulus::synthesizeType()
{
  // The expression is <dividend> mod <divisor>.
  // Both dividend and divisor must be exact numeric with scale 0.
  // The result has values from 0 to <divisor> - 1 and therefore
  // can always fit into the data type of the divisor.

  //
  // Type cast any params.
  //
  ValueId vid1 = child(0)->getValueId();
  ValueId vid2 = child(1)->getValueId();
  SQLInt si(NULL, TRUE);
  vid1.coerceType(si, NA_NUMERIC_TYPE);
  vid2.coerceType(si, NA_NUMERIC_TYPE);

  const NAType &typ1 = child(0)->getValueId().getType();
  const NAType &typ2 = child(1)->getValueId().getType();

  if (typ1.getTypeQualifier() != NA_NUMERIC_TYPE OR
      typ2.getTypeQualifier() != NA_NUMERIC_TYPE)
    {
      // 4046 Modulus function is only defined for exact numeric types.
      *CmpCommon::diags() << DgSqlCode(-4046) << DgString0(getTextUpper());
      return NULL;
    }

  // now it's safe to cast the types to numeric type
  const NumericType &ntyp1 = (NumericType &) typ1;
  const NumericType &ntyp2 = (NumericType &) typ2;

  if (NOT ntyp1.isExact() OR NOT ntyp2.isExact())
    {
      // 4046 Modulus function is only defined for exact numeric types.
      *CmpCommon::diags() << DgSqlCode(-4046) << DgString0(getTextUpper());
      return NULL;
    }

  if (ntyp1.getScale() != 0 OR
      ntyp2.getScale() != 0)
    {
      // 4047 Arguments of modulus function must both have a scale of 0.
      *CmpCommon::diags() << DgSqlCode(-4047) << DgString0(getTextUpper());
      return NULL;
    }

  if (ntyp1.decimalPrecision() && ntyp1.getPrecision() > MAX_NUMERIC_PRECISION)
    {
      // 3037: precision of dividend cannot exceed 18.
      *CmpCommon::diags() << DgSqlCode(-3037) << DgString0(child(0)->getTextUpper());
      return NULL;
    }

  if (ntyp2.decimalPrecision() && ntyp2.getPrecision() > MAX_NUMERIC_PRECISION)
    {
      // 3037: precision of divisor cannot exceed 18.
      *CmpCommon::diags() << DgSqlCode(-3037) << DgString0(child(1)->getTextUpper());
      return NULL;
    }

  NumericType * result = (NumericType*)ntyp2.newCopy(HEAP);
  result->setNullable(typ1.supportsSQLnullLogical() ||
		      typ2.supportsSQLnullLogical());
  if (ntyp1.isUnsigned())
    result->makeUnsigned();
  else
    result->makeSigned();

  return result;
}

// -----------------------------------------------------------------------
// member functions for class Repeat
// -----------------------------------------------------------------------

const NAType *Repeat::synthesizeType()
{
  // The expression is REPEAT(<value1>, <value2>)
  // The result is string <value1> repeated <value2> times.

  //
  // Type cast any params.
  //
  ValueId vid1 = child(0)->getValueId();
  ValueId vid2 = child(1)->getValueId();
  vid1.coerceType(NA_CHARACTER_TYPE);

  const SQLInt t(FALSE);
  vid2.coerceType(t, NA_NUMERIC_TYPE);

  const NAType &typ1 = child(0)->getValueId().getType();
  if (typ1.getTypeQualifier() != NA_CHARACTER_TYPE)
    {
      // 4051 Operand 1 must be character.
      *CmpCommon::diags() << DgSqlCode(-4051) << DgString0(getTextUpper());
      return NULL;
    }

  const NAType &typ2 = child(1)->getValueId().getType();
  if (typ2.getTypeQualifier() != NA_NUMERIC_TYPE)
    {
      // 4052 Operand 2 must be numeric.
      *CmpCommon::diags() << DgSqlCode(-4052) << DgString0(getTextUpper());
      return NULL;
    }

  const CharType &ctyp1 = (CharType &) typ1;

  // now it's safe to cast the type to numeric type
  const NumericType &ntyp2 = (NumericType &) typ2;

  if (ntyp2.getScale() != 0)
    {
      // 4047 Operand must be not have scale.
      *CmpCommon::diags() << DgSqlCode(-4047) << DgString0(getTextUpper());
      return NULL;
    }

  if (! ntyp2.isExact())
    {
      // 4046 Operand 2 must be exact.
      *CmpCommon::diags() << DgSqlCode(-4046) << DgString0(getTextUpper());
      return NULL;
    }

  Int64 size_in_bytes;
  Int64 size_in_chars;

  Int32 maxCharColLen = CmpCommon::getDefaultNumeric(TRAF_MAX_CHARACTER_COL_LENGTH);

  // figure out the max length of result.
  NABoolean negate;
  if (maxLengthWasExplicitlySet_)
    {
      // cap max len at traf_max_character_col_length
      size_in_bytes = 
        MINOF(maxCharColLen, getMaxLength());
      size_in_chars = 
        size_in_bytes / CharInfo::minBytesPerChar(ctyp1.getCharSet());
    }
  else if ((child(1)->getOperatorType() == ITM_CONSTANT) &&
      (child(1)->castToConstValue(negate)))
    {
      ConstValue * cv = child(1)->castToConstValue(negate);
      Int64 repeatCount;

      if (! cv->isNull())
	{
	  if (cv->canGetExactNumericValue()) {
	    repeatCount = cv->getExactNumericValue();
	    if (repeatCount <= 0)
	      repeatCount = 1;
	  } else {
	    // 4116 The 2nd operand of REPEAT(o1, o2) is invalid
	    *CmpCommon::diags() << DgSqlCode(-4116) << DgString0(getTextUpper());
	    return NULL;
	  }
	}
      else
	{
	  repeatCount = 1;
	}

      size_in_bytes = typ1.getNominalSize() * repeatCount;
      size_in_chars = ctyp1.getStrCharLimit() * repeatCount;
      // check size limit only for fixed character type
      if ( ! typ1.isVaryingLen() ) {
         if ( size_in_bytes > maxCharColLen ) {
	    *CmpCommon::diags() << DgSqlCode(-4129)
                                << DgString0(getTextUpper());
            return NULL;
         }
       } else // varchar. The nominal size of the result is
              // the min of (size, maxCharColLen).
         {
            size_in_bytes = MINOF(maxCharColLen, size_in_bytes);
            size_in_chars = size_in_bytes / CharInfo::minBytesPerChar(ctyp1.getCharSet());
         }
    }
  else if (getMaxLength() > -1)
    {
      size_in_bytes = MINOF(maxCharColLen, 
                            getMaxLength() * typ1.getNominalSize());
      size_in_chars = size_in_bytes / CharInfo::minBytesPerChar(ctyp1.getCharSet());
    }
  else
    {
      // Assign some arbitrary max result size since we can't
      // figure out the actual max size.
      size_in_bytes = maxCharColLen;
      size_in_chars = size_in_bytes / CharInfo::minBytesPerChar(ctyp1.getCharSet());
    }

  NAType *result = NULL;
  if (DFS2REC::isCharacterString(typ1.getFSDatatype()))
    result = new (HEAP) SQLVarChar(
         HEAP,
         CharLenInfo((Lng32)size_in_chars, (Lng32)size_in_bytes),
         (typ1.supportsSQLnullLogical() ||
          typ2.supportsSQLnullLogical()),
         ctyp1.isUpshifted(),
         ctyp1.isCaseinsensitive(),
         ctyp1.getCharSet(),
         ctyp1.getCollation(),
         ctyp1.getCoercibility());
  else
    result = new HEAP SQLBinaryString(
         HEAP,
         size_in_bytes,
         (typ1.supportsSQLnullLogical() ||
          typ2.supportsSQLnullLogical()),
         TRUE);
  return result;
}

// -----------------------------------------------------------------------
// member functions for class Replace
// -----------------------------------------------------------------------

const NAType *Replace::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid1 = child(0)->getValueId();
  ValueId vid2 = child(1)->getValueId();
  ValueId vid3 = child(2)->getValueId();

  CharInfo::CharSet new_cs = getFirstKnownCharSet(vid1, vid2, vid3);

  vid1.coerceType(NA_CHARACTER_TYPE, new_cs);
  vid2.coerceType(NA_CHARACTER_TYPE, new_cs);
  vid3.coerceType(NA_CHARACTER_TYPE, new_cs);

  const NAType *typ1 = &(child(0)->getValueId().getType());
  const NAType *typ2 = &(child(1)->getValueId().getType());
  const NAType *typ3 = &(child(2)->getValueId().getType());


  /* Soln-10-050426-7137 begin */

  NAString defVal;
  NABoolean charsetInference =
      (CmpCommon::getDefault(INFER_CHARSET, defVal) == DF_ON);

  if(charsetInference)
  {
    const CharType *replaceSource = (CharType *)typ1;
    const CharType *replaceChar= (CharType *)typ2;
    const CharType *replacingChar = (CharType *)typ3;
    const CharType* desiredType =
       CharType::findPushDownCharType(getDefaultCharSet,
                                   replaceSource, replaceChar, replacingChar, 0);

     if ( desiredType )
     {
        // push down charset and re-synthesize
        vid1.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);
        vid2.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);
        vid3.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);

        // get the newly pushed-down types
        typ1 = (CharType*)&vid1.getType();
        typ2 = (CharType*)&vid2.getType();
        typ3 = (CharType*)&vid3.getType();
     }

  }


  /* Soln-10-050426-7137 end */

  // typ3 does not need to be comparable, only compatible!
  //
  if (typ1->getTypeQualifier() != NA_CHARACTER_TYPE OR
      NOT typ1->isCompatible(*typ3)) {
    // 4064 The operands of a $0 function must be compatible character types.
    //      ##Should say "The FIRST and THIRD operands must be compatible..."
    *CmpCommon::diags() << DgSqlCode(-4064) << DgString0(getTextForError());;
    return NULL;
  }
  if (NOT typ1->isComparable(*typ2, this)) {
    // 4063 The operands of a $0 function must be comparable character types.
    //      ##Should say "The FIRST and SECOND operands must be comparable..."
    *CmpCommon::diags() << DgSqlCode(-4063) << DgString0(getTextForError());
    return NULL;
  }

  const CharType *ctyp1 = (CharType *)typ1;
  Lng32 minLength_in_bytes = ctyp1->getDataStorageSize();
  Lng32 minLength_in_chars = ctyp1->getStrCharLimit();

  Lng32 ctype2Length_in_bytes = ((CharType *)typ2)->getDataStorageSize();
  Lng32 ctype3Length_in_bytes = ((CharType *)typ3)->getDataStorageSize();
  Lng32 ctype2Length_in_chars = ((CharType *)typ2)->getStrCharLimit();
  Lng32 ctype3Length_in_chars = ((CharType *)typ3)->getStrCharLimit();

  if ( ctype2Length_in_bytes == 0 ) {
    *CmpCommon::diags() << DgSqlCode(-8428) << DgString0(getTextForError());
    return NULL;
  }

  // Fix for CR 10-000724-1369
  // figure out result size.

  Lng32 size_in_bytes = minLength_in_bytes;;
  Lng32 size_in_chars = minLength_in_chars;
  
  // NOTE: We are trying to find the MAX result size!

  if ( ((CharType *)typ2)->isVaryingLen() )
  {
     ctype2Length_in_chars = 1; // Use *minimum* possible length 
     ctype2Length_in_bytes = 1; // Use *minimum* possible length 
  }

  if ( ctyp1->getCharSet() == CharInfo::UNICODE )
  {
     if (ctype2Length_in_chars < ctype3Length_in_chars)
     {
       Int32 maxOccurrences = size_in_chars / ctype2Length_in_chars;
       Int32 remainder = size_in_chars - (maxOccurrences * ctype2Length_in_chars);
       size_in_chars = maxOccurrences * ctype3Length_in_chars + remainder;
       size_in_bytes = size_in_chars * ctyp1->getBytesPerChar();
     }
  }
  else
  {
     if (ctype2Length_in_chars < ctype3Length_in_chars)
     {
       Int32 maxOccurrences = size_in_chars / ctype2Length_in_chars;
       Int32 remainder = size_in_chars - (maxOccurrences * ctype2Length_in_chars);
       size_in_chars = maxOccurrences * ctype3Length_in_chars + remainder;
     }
     if (ctype2Length_in_bytes < ctype3Length_in_bytes)
     {
       Int32 maxOccurrences = size_in_bytes / ctype2Length_in_bytes;
       Int32 remainder = size_in_bytes - (maxOccurrences * ctype2Length_in_bytes);
       size_in_bytes = maxOccurrences * ctype3Length_in_bytes + remainder;
     }
  }

  Int32 maxLenInBytes = CmpCommon::getDefaultNumeric(TRAF_MAX_CHARACTER_COL_LENGTH);
  if (size_in_bytes > maxLenInBytes)
    {
      size_in_bytes = maxLenInBytes;
      size_in_chars = size_in_bytes / CharInfo::minBytesPerChar(ctyp1->getCharSet());
    }
 
  CharLenInfo CLInfo( size_in_chars, size_in_bytes );

  NAType *result = NULL;
  if (DFS2REC::isCharacterString(typ1->getFSDatatype()))
    result =
      new (HEAP) SQLVarChar(HEAP, CLInfo,
                            (typ1->supportsSQLnullLogical() ||
                             typ2->supportsSQLnullLogical() ||
                             typ3->supportsSQLnullLogical()),
                            ctyp1->isUpshifted(),
                            ctyp1->isCaseinsensitive(),
                            ctyp1->getCharSet(),
                            ctyp1->getCollation(),
                            ctyp1->getCoercibility());
  else
    result = new HEAP SQLBinaryString(
         HEAP, size_in_bytes,
         (typ1->supportsSQLnullLogical() ||
          typ2->supportsSQLnullLogical() ||
          typ3->supportsSQLnullLogical()),
         TRUE);

  return result;
}

// -----------------------------------------------------------------------
// member functions for class HashDistrib
// -----------------------------------------------------------------------

const NAType *HashDistrib::synthesizeType()
{
  // Both operands (hash of the partitioning keys and number of partitions)
  // must be exact numeric with scale 0. The result has values from 0 to
  // <number of partitions> - 1 and therefore can always fit into the data
  // type of the number of partitions.

  const NAType &typ1 = child(0)->getValueId().getType();
  const NAType &typ2 = child(1)->getValueId().getType();

  if (typ1.getTypeQualifier() != NA_NUMERIC_TYPE OR
      typ2.getTypeQualifier() != NA_NUMERIC_TYPE)
    {
      // 4045 Progressive Distribution function is only defined
      // for numeric types.
      *CmpCommon::diags() << DgSqlCode(-4045);
      return NULL;
    }

  // now it's safe to cast the types to numeric type
  const NumericType &ntyp1 = (NumericType &) typ1;
  const NumericType &ntyp2 = (NumericType &) typ2;

  if (NOT ntyp1.isExact() OR NOT ntyp2.isExact())
    {
      // 4046 Progessive Distribution function is only defined for
      // exact numeric types.
      *CmpCommon::diags() << DgSqlCode(-4046);
      return NULL;
    }

  if (ntyp1.getScale() != 0 OR
      ntyp2.getScale() != 0)
    {
      // 4047 Arguments of Progessive Distribution function must both
      // have a scale of 0.
      *CmpCommon::diags() << DgSqlCode(-4047);
      return NULL;
    }

  NAType *result = typ2.newCopy(HEAP);

  // the only thing the LHS contributes is that the result may become nullable
  if (typ1.supportsSQLnullLogical()) result->setNullable(TRUE);

  return result;
}

const NAType *ProgDistribKey::synthesizeType()
{
  // return: Large Int.
  return new HEAP SQLLargeInt(HEAP, TRUE, FALSE);
}

// -----------------------------------------------------------------------
// member functions for class PAGroup
// -----------------------------------------------------------------------

const NAType *PAGroup::synthesizeType()
{
  // Both operands (pre-grouped number of partitions and number of groups)
  // must be exact numeric with scale 0. The result has values from 0 to
  // <number of groups> - 1 and therefore can always fit into the data
  // type of the number of groups.

  const NAType &typ1 = child(0)->getValueId().getType();
  const NAType &typ2 = child(1)->getValueId().getType();
  const NAType &typ3 = child(2)->getValueId().getType();

  if (typ1.getTypeQualifier() != NA_NUMERIC_TYPE OR
      typ2.getTypeQualifier() != NA_NUMERIC_TYPE OR
      typ3.getTypeQualifier() != NA_NUMERIC_TYPE)
    {
      // 4045 PA Group function is only defined
      // for numeric types.
      *CmpCommon::diags() << DgSqlCode(-4045);
      return NULL;
    }

  // now it's safe to cast the types to numeric type
  const NumericType &ntyp1 = (NumericType &) typ1;
  const NumericType &ntyp2 = (NumericType &) typ2;
  const NumericType &ntyp3 = (NumericType &) typ3;

  if (NOT ntyp1.isExact() OR NOT ntyp2.isExact() OR NOT ntyp3.isExact())
    {
      // 4046 PA Group function is only defined for
      // exact numeric types.
      *CmpCommon::diags() << DgSqlCode(-4046);
      return NULL;
    }

  if (ntyp1.getScale() != 0 OR ntyp2.getScale() != 0 OR ntyp3.getScale() != 0)
    {
      // 4047 Arguments of the PA Group function must both
      // have a scale of 0.
      *CmpCommon::diags() << DgSqlCode(-4047);
      return NULL;
    }

  NAType *result = typ1.newCopy(HEAP);

  return result;
}

// -----------------------------------------------------------------------
// member functions for class Encode
// -----------------------------------------------------------------------

const NAType *CompEncode::synthesizeType()
{
  ValueId vid = child(0)->getValueId();
  const NAType &src = vid.getType();

  // result of encode function is a non-nullable fixed char.
  // Result is not nullable
  // because null values are encoded too.
  // Length of encode function is equal to length_ field, if it
  // is set to a positive number. Otherwise, it is equal to the
  // total size of operand.

  Lng32 keyLength = 0;
  NABoolean supportsSQLnull = FALSE;
  if (regularNullability_)
    {
      // should not be common for CompEncode, preserve nullability of child
      keyLength = src.getNominalSize();
      supportsSQLnull = src.supportsSQLnull();
    }
  else
    {
      if (length_ < 0)
        // common case for encode, include prefix fields but leave
        // out the var len header (if any), which is not order-preserving
        keyLength = src.getTotalSize() - src.getVarLenHdrSize();
      else
        keyLength = length_;
    }

  if (src.getTypeQualifier() != NA_CHARACTER_TYPE)
    {
      return new HEAP SQLChar(HEAP, keyLength, supportsSQLnull);
    }
  else
    {
      const CharType &cSrc = (CharType&)src;
      CharInfo::Collation collation = cSrc.getCollation();

      // set casesensitivity of encoding based on child's type.
      // This may get overwritten by the caller (for example, to
      // build a key for a predicate of the form:  where keycol = 'val',
      // both sides of the predicate must be caseinsensitive.
      if (cSrc.isCaseinsensitive())
        {
          setCaseinsensitiveEncode(TRUE);
        }

      setEncodedCollation(cSrc.getCollation());

      if (CollationInfo::isSystemCollation(collation))
	{	
	  keyLength = 
	    CompEncode::getEncodedLength( collation,
					  collationType_,
					  child(0)->getValueId().getType().getNominalSize(),
					  cSrc.supportsSQLnull());
	  
	  switch (collationType_)
	    {
	    case CollationInfo::Sort:
	      {
		// in this case the encode is non nullable if not regularNullability
		return new HEAP SQLChar(HEAP, keyLength, 
					supportsSQLnull); 
	      }
	    case CollationInfo::Compare:
	      {
		return new HEAP SQLChar(HEAP, keyLength, 
					cSrc.supportsSQLnull());
	      }
	    case CollationInfo::Search:
	      {
		return new HEAP SQLVarChar(HEAP, keyLength,   
					   cSrc.supportsSQLnull());
	      }
	    default:
	      {
		CMPASSERT(0);
		return NULL;
	      }
	    }
	}
      else
	{
	  return new HEAP SQLChar(HEAP, keyLength, supportsSQLnull);
	}
    }
}

const NAType *CompDecode::synthesizeType()
{
  if (unencodedType_)
    return unencodedType_;
  else
    return CompEncode::synthesizeType();
}

// -----------------------------------------------------------------------
// member functions for class Extract
// -----------------------------------------------------------------------

const NAType *Extract::synthesizeType()
{
  // Assert that we are bound, or created by Generator, so we have type info.
  ValueId vid = child(0)->getValueId();
  CMPASSERT(vid != NULL_VALUE_ID);
  const DatetimeIntervalCommonType &dti =
       (DatetimeIntervalCommonType &)vid.getType();

  NABuiltInTypeEnum type = dti.getTypeQualifier();
  if (type != NA_DATETIME_TYPE && type != NA_INTERVAL_TYPE) {
    // 4036 The source field must be of DateTime or Interval type.
    *CmpCommon::diags() << DgSqlCode(-4036);

    if (getFieldFunction())
      {
	// 4062 The preceding error actually occurred in function $0~String0.
	*CmpCommon::diags() << DgSqlCode(-4062) << DgString0(dti.getFieldName(getExtractField()));
      }

    return NULL;
  }
  if ( type != NA_DATETIME_TYPE )
    {
      enum rec_datetime_field eField = getExtractField();
      NAString sErr;
      if ( REC_DATE_WEEK == eField
          || REC_DATE_DOW == eField
          || REC_DATE_DOY == eField
          || REC_DATE_WOM == eField
          || REC_DATE_CENTURY == eField)
        sErr = dti.getFieldName(eField);
      if (sErr.length() > 0)
        {
          *CmpCommon::diags() << DgSqlCode(-4496) << DgString0(sErr);
          return NULL;
        }
    }
  // ANSI 6.6 SR 3a.
  enum rec_datetime_field extractStartField = getExtractField();
  enum rec_datetime_field extractEndField = extractStartField;

  if (extractStartField > REC_DATE_MAX_SINGLE_FIELD)
    {
      // YEARQUARTER, YEARMONTH, or YEARWEEK
      extractStartField = REC_DATE_YEAR;

      if (extractEndField > REC_DATE_YEARMONTH_EXTRACT)
        extractEndField = REC_DATE_DAY; // extracting week requires the day
      else
        extractEndField = REC_DATE_MONTH; // months/quarters need only the month

      if (type == NA_INTERVAL_TYPE)  // YEARQUARTER etc. are not supported on intervals
        {
          *CmpCommon::diags() << DgSqlCode(-4037)
            << DgString0(dti.getFieldName(getExtractField()))
            << DgString1(dti.getTypeSQLname(TRUE /*terse*/));
          return NULL;
        }
    }

  if ( !(extractStartField >=REC_DATE_CENTURY && extractStartField<=REC_DATE_WOM) )
    {
      if (dti.getStartField() > extractStartField ||
          dti.getEndField()   < extractEndField ||
          !dti.isSupportedType()) {
        // 4037 cannot extract field from type
        *CmpCommon::diags() << DgSqlCode(-4037)
           << DgString0(dti.getFieldName(getExtractField()))
           << DgString1(dti.getTypeSQLname(TRUE /*terse*/));

        return NULL;
      }
    }
  // ANSI 6.6 SR 4.  Precision is implementation-defined:
  // EXTRACT(YEAR       from datetime):  result precision is 4 + scale
  // EXTRACT(other      from datetime):  result precision is 2 + scale
  // EXTRACT(startfield from interval):  result precision is leading prec + scal
  // EXTRACT(other      from interval):  result precision is 2 + scale
  // where scale is 0 if extract field is not SECOND, else at least fract prec.
  //
  Lng32 prec, scale = 0;
  if (type == NA_INTERVAL_TYPE && getExtractField() == dti.getStartField())
    prec = dti.getLeadingPrecision();
  else if (getExtractField() == REC_DATE_YEAR)
    prec = 4;					// YEAR field can be 9999 max
  else if (getExtractField() == REC_DATE_YEARQUARTER_EXTRACT ||
           getExtractField() == REC_DATE_YEARQUARTER_D_EXTRACT)
    prec = 5;					// YEARQUARTER is yyyyq
  else if (getExtractField() == REC_DATE_YEARMONTH_EXTRACT ||
           getExtractField() == REC_DATE_YEARMONTH_D_EXTRACT)
    prec = 6;					// YEARMONTH is yyyymm
  else if (getExtractField() == REC_DATE_YEARWEEK_EXTRACT ||
           getExtractField() == REC_DATE_YEARWEEK_D_EXTRACT)
    prec = 6;					// YEARMWEEK is yyyyww
  else if (getExtractField() == REC_DATE_DECADE ||
           getExtractField() == REC_DATE_DOY)
    prec = 3;
  else if (getExtractField() == REC_DATE_QUARTER ||
           getExtractField() == REC_DATE_DOW)
    prec = 1;
  else if (getExtractField() == REC_DATE_EPOCH)
    prec = 10;
  else
    prec = 2;					// else max of 12, 31, 24, 59
  if (getExtractField() == REC_DATE_SECOND) {
    prec  += dti.getFractionPrecision();
    scale += dti.getFractionPrecision();
  }
  if (getExtractField() == REC_DATE_EPOCH)
    {
      prec  += dti.getFractionPrecision();
      scale += dti.getFractionPrecision();
    }
  NABoolean bNegValue = FALSE;
  if (getExtractField() == REC_DATE_DECADE
      || getExtractField() == REC_DATE_QUARTER
      || getExtractField() == REC_DATE_EPOCH )
    bNegValue = TRUE;
  const Int16 disAmbiguate = 0; // added for 64bit project
  return new HEAP
    SQLNumeric(HEAP, (type == NA_INTERVAL_TYPE) || bNegValue, /*allowNegValues*/
	       prec,
	       scale,
               disAmbiguate,
               dti.supportsSQLnull());
}

// -----------------------------------------------------------------------
// member functions for class Increment
// -----------------------------------------------------------------------

const NAType *Increment::synthesizeType()
{
  // It should get the type of its child
  return &child(0)->getValueId().getType();
}

// -----------------------------------------------------------------------
// member functions for class Decrement
// -----------------------------------------------------------------------

const NAType *Decrement::synthesizeType()
{
  return &child(0)->getValueId().getType();
}

// -----------------------------------------------------------------------
// member functions for class TriRelational
// -----------------------------------------------------------------------

const NAType *TriRelational::synthesizeType()
{
  ItemExprList exprList1(child(0).getPtr(), HEAP);
  ItemExprList exprList2(child(1).getPtr(), HEAP);
  if (exprList1.entries() != exprList2.entries()) {
    // 4042 The operands of a comparison predicate must be of equal degree.
    *CmpCommon::diags() << DgSqlCode(-4042);
    return NULL;
  }
  NABoolean allowsUnknown = FALSE;
  for (CollIndex i = 0; i < exprList1.entries(); i++) {
    //
    // Type cast any params.
    //
    ValueId vid1 = exprList1[i]->getValueId();
    ValueId vid2 = exprList2[i]->getValueId();
    vid1.coerceType(vid2.getType(), NA_NUMERIC_TYPE);
    vid2.coerceType(vid1.getType());
    //
    // Check that the operands are compatible.
    //
    const NAType& operand1 = vid1.getType();
    const NAType& operand2 = vid2.getType();
    if (NOT operand1.isCompatible(operand2)) {
      // 4041 comparison between these two types is not allowed
      emitDyadicTypeSQLnameMsg(-4041, operand1, operand2);
      return NULL;
    }

    allowsUnknown = allowsUnknown OR
                   operand1.supportsSQLnullLogical() OR
                   operand2.supportsSQLnullLogical();
  }
  //
  // is the third operand a boolean?
  //
  if (child(2)->getValueId().getType().getTypeQualifier() != NA_BOOLEAN_TYPE) {
    // 4048 third arg of ternary comparison must be boolean
    *CmpCommon::diags() << DgSqlCode(-4048) <<
      DgString0(child(2)->getValueId().getType().getTypeSQLname(TRUE /*terse*/));
    return NULL;
  }
  //
  // Return the result.
  //
  return new HEAP SQLBooleanRelat(HEAP, allowsUnknown);
}

// -----------------------------------------------------------------------
// member functions for class RangeLookup
// -----------------------------------------------------------------------

const NAType *RangeLookup::synthesizeType()
{
  // the result is a signed 32 bit number
  return new HEAP SQLInt(HEAP, TRUE,FALSE);
}

// -----------------------------------------------------------------------
// member functions for class HostVar
// -----------------------------------------------------------------------

const NAType *HostVar::synthesizeType()
{
  return getType();
}

// -----------------------------------------------------------------------
// member functions for class InverseOrder
// -----------------------------------------------------------------------

const NAType *InverseOrder::synthesizeType()
{
  return &child(0)->getValueId().getType();
}

// member functions for class Like
// -----------------------------------------------------------------------

const NAType *PatternMatchingFunction::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid1 = child(0)->getValueId();
  ValueId vid2 = child(1)->getValueId();
  ValueId vid3;
  CharType * cType1 = 0;
  CharType * cType2 = 0;

  // if either side of LIKE was an untyped param, then assign it
  // the same casesensitive attr, collation, and character set as the other side.
  if (	vid1.getType().getTypeQualifier() != NA_UNKNOWN_TYPE  &&
        vid2.getType().getTypeQualifier() == NA_UNKNOWN_TYPE)
    {
      vid1.coerceType(NA_CHARACTER_TYPE);
      cType1 = (CharType*)&vid1.getType();
      vid2.coerceType(NA_CHARACTER_TYPE, cType1->getCharSet());     
      cType2 = (CharType*)&vid2.getType();
      cType2->setCollation(cType1->getCollation());
      cType2->setCaseinsensitive(cType1->isCaseinsensitive());
    }
  else if (	vid2.getType().getTypeQualifier() != NA_UNKNOWN_TYPE  &&
        vid1.getType().getTypeQualifier() == NA_UNKNOWN_TYPE)
    {
      vid2.coerceType(NA_CHARACTER_TYPE);
      cType2 = (CharType*)&vid2.getType();
      vid1.coerceType(NA_CHARACTER_TYPE, cType2->getCharSet());     
      cType1 = (CharType*)&vid1.getType();
      cType1->setCollation(cType2->getCollation());
      cType1->setCaseinsensitive(cType2->isCaseinsensitive());
    }
  else
    {
      vid1.coerceType(NA_CHARACTER_TYPE);
      vid2.coerceType(NA_CHARACTER_TYPE);
    }

  const NAType *typ1 = &vid1.getType();
  const NAType *typ2 = &vid2.getType();
  const NAType *typ3 = NULL;

  if (getArity() > 2) {   // Escape clause was specified
    vid3 = child(2)->getValueId();
    const SQLChar charType(NULL, 1);
    vid3.coerceType(charType);
    typ3 = &vid3.getType();
  }

  // 2/13/98: make sure like pattern and source types are comparable.
  const NAType& operand1 = vid1.getType();
  const NAType& operand2 = vid2.getType();
  const CharType *likeSource = (CharType*)&operand1;
  const CharType *likePat = (CharType*)&operand2;
  const CharType *escapeChar = ( getArity() > 2 ) ?
                (CharType*)&(vid3.getType()) : 0;

  NAString defVal;
  NABoolean charsetInference =
      (CmpCommon::getDefault(INFER_CHARSET, defVal) == DF_ON);

  if(charsetInference)
  {
     const CharType* desiredType =
       CharType::findPushDownCharType(getDefaultCharSet,
                                   likeSource, likePat, escapeChar, 0);

     if ( desiredType )
     {
        // push down charset and re-synthesize
        vid1.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);
        vid2.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);
        if ( getArity() > 2 )
           vid3.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);

        // get the newly pushed-down types
        typ1 = &vid1.getType();
        typ2 = &vid2.getType();
        typ3 = ( getArity() > 2 ) ? &vid3.getType() : 0;
     }

  }

  // Check that the operands are comparable.
  //
  if (NOT typ1->isComparable(*typ2, this, NAType::EmitErrIfAnyChar) OR
     (typ3 AND
      NOT typ1->isComparable(*typ3, this, NAType::EmitErrIfAnyChar)) OR
      typ1->getTypeQualifier() != NA_CHARACTER_TYPE) {
    // 4050 The operands of a LIKE predicate must be comparable character types.
    *CmpCommon::diags() << DgSqlCode(-4050) << DgString0("LIKE");
    return NULL;
  }

  // If any of the arguments can be nullable then LIKE can evaluate to Unknown
  NABoolean allowsUnknown = typ1->supportsSQLnull() OR
                            typ2->supportsSQLnull() OR
			   (typ3 AND
			    typ3->supportsSQLnull());

  return new HEAP SQLBooleanRelat(HEAP, allowsUnknown);
}

// -----------------------------------------------------------------------
// member functions for classes Lower and Upper`
// -----------------------------------------------------------------------

const NAType *Lower::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid = child(0)->getValueId();
  vid.coerceType(NA_CHARACTER_TYPE);
  //
  // Check that the operands are compatible.
  //
  const NAType& operand = vid.getType();
  if (NOT DFS2REC::isCharacterString(operand.getFSDatatype())) {
    // 4043 The operand of a LOWER function must be character.
    *CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextForError());
    return NULL;
  }

  CharType *ct = (CharType *)&operand;

  if ( CharInfo::is_NCHAR_MP(ct->getCharSet()) ) {
    // 3217: Character set KANJI/KSC5601 is not allowed in the LOWER function.
    *CmpCommon::diags() << DgSqlCode(-3217)
                        << DgString0(CharInfo::getCharSetName(ct->getCharSet()))
                        << DgString1("LOWER");
  }

  if ((ct->isUpshifted()) ||
      (ct->isCaseinsensitive())) {
    ct = (CharType *)ct->newCopy(HEAP);
    if (ct->isUpshifted())
      ct->setUpshifted(FALSE);
    if (ct->isCaseinsensitive())
      ct->setCaseinsensitive(TRUE);
  }
  //
  // For UTF8 strings, we must make the TYPE be a VARCHAR because there are certain
  // UCS2 characters (e.g. 0x0130) where the value of LOWER is actually fewer bytes in
  // length than the original character!
  //
  if (ct->getCharSet() == CharInfo::UTF8) {
    // NOTE: See comment near end of Upper::synthesizeType() for reason we don't multiply by 3 here.
    ct =  new (HEAP) SQLVarChar(HEAP, CharLenInfo(ct->getStrCharLimit(), (ct->getDataStorageSize()))
				,ct->supportsSQLnull()
				,ct->isUpshifted()
				,ct->isCaseinsensitive()
				,ct->getCharSet()
				,ct->getCollation()
				,ct->getCoercibility()
				);
  }

  return ct;
}

const NAType *Upper::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid = child(0)->getValueId();
  vid.coerceType(NA_CHARACTER_TYPE);
  //
  // Check that the operands are compatible.
  //
  const NAType& operand = vid.getType();

  if (NOT DFS2REC::isCharacterString(operand.getFSDatatype())) {
    // 4043 The operand of an UPPER function must be character.
    *CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextForError());
    return NULL;
  }

  CharType *ct = (CharType *)&operand;

  if ( CharInfo::is_NCHAR_MP(ct->getCharSet()) ) {
    *CmpCommon::diags() << DgSqlCode(-3217)
                        << DgString0(CharInfo::getCharSetName(ct->getCharSet()))
                        << DgString1("UPPER");
  }

  if (NOT ct->isUpshifted()) {
    ct = (CharType *)ct->newCopy(HEAP);
    ct->setUpshifted(TRUE);
  }

  if (ct->getCharSet() == CharInfo::UNICODE) {
    ct =  new (HEAP) SQLVarChar(HEAP, 3*(ct->getStrCharLimit())
				,ct->supportsSQLnull()
				,ct->isUpshifted()
				,ct->isCaseinsensitive()
				,ct->getCharSet()
				,ct->getCollation()
				,ct->getCoercibility()
				);
  }

  if (ct->getCharSet() == CharInfo::UTF8) {
  //
  // NOTE: For some UCS2 characters, the UPPER function can produce *three* UCS2 characters
  // and for that reason, the UPPER function provides for 3 times as much output as the
  // input string is long.  HOWEVER, such is never the case for the LOWER function.
  //
    ct =  new (HEAP) SQLVarChar(HEAP, CharLenInfo(3*ct->getStrCharLimit(), 3*(ct->getDataStorageSize()))
				,ct->supportsSQLnull()
				,ct->isUpshifted()
				,ct->isCaseinsensitive()
				,ct->getCharSet()
				,ct->getCollation()
				,ct->getCoercibility()
				);
  }

  return ct;
}

// -----------------------------------------------------------------------
// member functions for class NATypeToItem
// -----------------------------------------------------------------------
const NAType * NATypeToItem::synthesizeType()
{
  return natype_pointer;
};

const NAType*
NATypeToItem::pushDownType(NAType& newType,
                       enum NABuiltInTypeEnum defaultQualifier)
{
   return &newType;
}

// -----------------------------------------------------------------------
// member functions for class OctetLength
// -----------------------------------------------------------------------

const NAType *OctetLength::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid = child(0)->getValueId();
  vid.coerceType(NA_CHARACTER_TYPE);
  //
  // Check that the operands are compatible.
  //
  const NAType& operand = vid.getType();
  if (operand.getTypeQualifier() != NA_CHARACTER_TYPE) {
    // 4043 The operand of an OCTET_LENGTH function must be character.
    *CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextUpper());
    return NULL;
  }

  const CharType* charOperand = (CharType*)&(vid.getType());

  NAString defVal;
  NABoolean charsetInference =
      (CmpCommon::getDefault(INFER_CHARSET, defVal) == DF_ON);

  if(charsetInference)
 {
    const CharType* desiredType =
       CharType::findPushDownCharType(getDefaultCharSet, charOperand, 0);

    if ( desiredType )
    {
      // push down charset and re-synthesize
      vid.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);

      // get the newly pushed-down types
      charOperand = (CharType*)&(vid.getType());
    }
 }

  if ( charOperand -> getCharSet() == CharInfo::UnknownCharSet ) {
     *CmpCommon::diags() << DgSqlCode(-4127);
     return NULL;
  }

  //
  // Return the result.
  //
  return new HEAP SQLInt(HEAP, FALSE  // unsigned
		     ,operand.supportsSQLnullLogical()
		     );
}

// -----------------------------------------------------------------------
// member functions for class PositionFunc
// -----------------------------------------------------------------------

const NAType *PositionFunc::synthesizeType()
{

  //
  // Type cast any params.
  //
  ValueId vid1 = child(0)->getValueId();
  ValueId vid2 = child(1)->getValueId();

  CharInfo::CharSet new_cs = getFirstKnownCharSet(vid1, vid2, vid2);
  vid1.coerceType(NA_CHARACTER_TYPE, new_cs);
  vid2.coerceType(NA_CHARACTER_TYPE, new_cs);

  const NAType *operand1 = &vid1.getType();
  const NAType *operand2 = &vid2.getType();
  const NAType *operand3 = NULL;
  const NAType *operand4 = NULL;


  //
  // Check that the operands are comparable.
  // ##Hmm, Ansi 6.6 does NOT say they need to be comparable,
  // ##just compatible (same char repertoire, i.e. same charset),
  // ##but that must be a mistake (we need string =ity testing
  // ##to do POSITION, so the collations will need to be the same)...
  //
/*
  if (NOT operand1.isComparable(operand2, this, NAType::EmitErrIfAnyChar) ||
      operand1.getTypeQualifier() != NA_CHARACTER_TYPE ||
      operand2.getTypeQualifier() != NA_CHARACTER_TYPE) {
    // 4063 The operands of a $0 function must be comparable character types.
    *CmpCommon::diags() << DgSqlCode(-4063) << DgString0(getTextForError());
    return NULL;
  }
*/


  if (operand1->getTypeQualifier() != NA_CHARACTER_TYPE OR
      operand2->getTypeQualifier() != NA_CHARACTER_TYPE) {
    // 4063 The operands of a POSITION function must be character.
    *CmpCommon::diags() << DgSqlCode(-4063) << DgString0(getTextForError());
    return NULL;
  }


// 1/5/98: make sure position pattern and source types are comparable.
  const CharType *posPat = (CharType*)operand1;
  const CharType *posSource = (CharType*)operand2;


  NAString defVal;
  NABoolean charsetInference =
      (CmpCommon::getDefault(INFER_CHARSET, defVal) == DF_ON);

  if(charsetInference)
  {
    // 9/24/98: charset inference
    const CharType* desiredType =
      CharType::findPushDownCharType(getDefaultCharSet,
                                  posPat, posSource, 0);

    if ( desiredType )
    {
      // push down charset and re-synthesize
      vid1.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);
      vid2.coerceType((NAType&)*desiredType, NA_CHARACTER_TYPE);

      // get the newly pushed-down types
/*
      posPat = (CharType*)&(vid1.getType());
      posSource = (CharType*)&(vid2.getType());
*/
      operand1 = &vid1.getType();
      operand2 = &vid2.getType();

    }
  }

/*
  if ( ! (posPat->isComparable(*posSource, TRUE)) ) { // } 
*/
  if ( ! (operand1->isComparable(*operand2, this, NAType::EmitErrIfAnyChar)) ) {
    // 4063 The operands of a POSITION function must be character.
    *CmpCommon::diags() << DgSqlCode(-4063) << DgString0(getTextForError());
    return NULL;
  }


  //
  // Return the result.
  //
  return new HEAP
    SQLInt(HEAP, FALSE,  // unsigned
	   operand1->supportsSQLnullLogical() ||
	   operand2->supportsSQLnullLogical()
	  );
}

// -----------------------------------------------------------------------
// member functions for class Substring
// -----------------------------------------------------------------------

const NAType *Substring::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid1 = child(0)->getValueId();
  ValueId vid2 = child(1)->getValueId();
  vid1.coerceType(NA_CHARACTER_TYPE);

  SQLInt si(NULL);
  vid2.coerceType(si, NA_NUMERIC_TYPE);
  if (getArity() == 3)
    {
      ValueId vid3 = child(2)->getValueId();
      vid3.coerceType(si, NA_NUMERIC_TYPE);
    }

  const NAType *operand1 = &child(0)->getValueId().getType();
  const NAType *operand2 = &child(1)->getValueId().getType();
  const NAType *operand3 = NULL;

  if (getArity() == 3)
    {
      operand3 = &child(2)->getValueId().getType();
    }

  if ((operand1->getTypeQualifier() != NA_CHARACTER_TYPE) &&
      (operand1->getFSDatatype() != REC_CLOB)) {
    // 4051 The first operand of a SUBSTRING function must be character.
    *CmpCommon::diags() << DgSqlCode(-4051) << DgString0(getTextUpper());
    return NULL;
  }
  if (operand2->getTypeQualifier() != NA_NUMERIC_TYPE) {
    // 4052 The second operand of a SUBSTRING function must be numeric.
    *CmpCommon::diags() << DgSqlCode(-4052) << DgString0(getTextUpper());
    return NULL;
  }
  if (((NumericType*)operand2)->getScale() != 0) {
    // 4047 The second operand of a SUBSTRING function must have a scale of 0.
    *CmpCommon::diags() << DgSqlCode(-4047) << DgString0(getTextUpper());
    return NULL;
  }
  if (operand3) {
      if (operand3->getTypeQualifier() != NA_NUMERIC_TYPE) {
	// 4053 The third operand of a SUBSTRING function must be numeric.
	*CmpCommon::diags() << DgSqlCode(-4053) << DgString0(getTextUpper());
	return NULL;
      }

      if (((NumericType*)operand3)->getScale() != 0) {
	// 4047 The third operand of a SUBSTR function must have a scale of 0.
	*CmpCommon::diags() << DgSqlCode(-4047) << DgString0(getTextUpper());
	return NULL;
      }
  }

  CharInfo::CharSet op1_cs = operand1->getCharSet();

  const CharType *charOperand = (CharType *) operand1;
  Lng32 maxLength_bytes = charOperand->getDataStorageSize();
  Lng32 maxLength_chars = charOperand->getPrecisionOrMaxNumChars();
  if ( maxLength_chars <= 0 ) // If unlimited
       maxLength_chars = maxLength_bytes / CharInfo::minBytesPerChar(op1_cs);

  NABoolean negate;
  Lng32 pos = 0;
  {
    // The position arg is allowed to be negative (see Ansi 6.7 GR 1).
    ConstValue *cv = child(1)->castToConstValue(negate);

    // adjust the max length for the result only if it is a positive start position.
    // solu 10-030603-6815.
    if (cv && negate == FALSE) {
      if (cv->canGetExactNumericValue()) {
	Int64 pos64 = cv->getExactNumericValue();
	if (pos64 <= MINOF(maxLength_chars,INT_MAX)) {
	  pos = int64ToInt32(pos64);
	  if ((pos-1) > 0) {
             maxLength_chars -= (pos-1);	  // shorten max
             maxLength_bytes -= (pos-1) * CharInfo::minBytesPerChar(op1_cs);
             if ( maxLength_bytes > charOperand->getDataStorageSize() )
                  maxLength_bytes = charOperand->getDataStorageSize() ;
	  }
	} 	// value is in bounds
      }	  	// can get exact numeric
    }	  	// constant pos op
  }	  	// position operand

  NABoolean resultIsFixedChar = FALSE;
  Lng32 length = 0;
  Int64 length64 = 0;
  if (operand3) {
    ConstValue *cv = child(2)->castToConstValue(negate);
    if (cv) {

      if (negate) {
	// 8403 The length arg of a SUBSTRING function cannot be less than zero.
	*CmpCommon::diags() << DgSqlCode(-8403);
	return NULL;
      }

      if (cv->canGetExactNumericValue()) {
	length64 = cv->getExactNumericValue();

	if (length64 <= INT_MAX) {
	  length = int64ToInt32(length64);

          if (maxLength_chars > length) {
              maxLength_chars = length;
              maxLength_bytes = MINOF(maxLength_bytes, length * CharInfo::maxBytesPerChar(op1_cs));
          }
	} 	// value is in bounds
      }	  	// can get exact numeric
    }	  	// constant length op
  }	  	// length operand specified

  if (operand1->getFSDatatype() == REC_CLOB)
    {
      return new HEAP
	SQLClob(HEAP, maxLength_bytes, Lob_Invalid_Storage,
		operand1->supportsSQLnull() OR
		operand2->supportsSQLnull() OR
		((operand3 != NULL) AND operand3->supportsSQLnull()));
    }
  else if (DFS2REC::isCharacterString(operand1->getFSDatatype()))
    {
      return new HEAP
        SQLVarChar(HEAP, CharLenInfo(maxLength_chars, maxLength_bytes), // OLD: maxLength
                   operand1->supportsSQLnull() OR
                   operand2->supportsSQLnull() OR
                   ((operand3 != NULL) AND operand3->supportsSQLnull())
                   ,charOperand->isUpshifted()
                   ,charOperand->isCaseinsensitive()
                   ,operand1->getCharSet()
                   ,charOperand->getCollation()
                   ,charOperand->getCoercibility()
                   );
    }
  else
    {
      return new HEAP
	SQLBinaryString(HEAP, maxLength_bytes,
                      operand1->supportsSQLnull() OR
                      operand2->supportsSQLnull() OR
                      ((operand3 != NULL) AND operand3->supportsSQLnull()),
                      TRUE);
    }
}

// -----------------------------------------------------------------------
// member functions for class Trim
// -----------------------------------------------------------------------

const NAType *Trim::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid1 = child(0)->getValueId();
  ValueId vid2 = child(1)->getValueId();

  CharInfo::CharSet new_cs = getFirstKnownCharSet(vid1, vid2, vid2);
  vid1.coerceType(NA_CHARACTER_TYPE, new_cs);
  vid2.coerceType(NA_CHARACTER_TYPE, new_cs);

  if (vid1.getType().getTypeQualifier() != NA_CHARACTER_TYPE ||
      vid2.getType().getTypeQualifier() != NA_CHARACTER_TYPE)
  {
    //4133: Both trim character and source have to be CHARACTER typed.
    *CmpCommon::diags() << DgSqlCode(-4133);
    return NULL;
  }

  //
  // Check that the operands are compatible.
  //
  const CharType *trimChar   = (const CharType *)&vid1.getType();
  const CharType *trimSource = (const CharType *)&vid2.getType();

    // charset inference
  if ( trimChar->getCharSet() == CharInfo::UnknownCharSet &&
       trimSource->getCharSet() != CharInfo::UnknownCharSet
     )
  {
     // Special case for MP NCHAR when the default trim character is
     // a single single-byte character like ' '. Here we prepend a space
     // character to the local string copy (locale_string) inside 
     // the constant value holding the sinlge byte trim character. During 
     // invocation of vid1.coerceType(), the newly fabricated double-byte 
     // trim character will be instantiated in the constant value object. 
     if (CharInfo::is_NCHAR_MP(trimSource->getCharSet()) &&
         vid1.getItemExpr()-> getOperator() == ITM_CONSTANT)
     {
        ConstValue* trimCharValue = (ConstValue*)vid1.getItemExpr();
        if ( trimCharValue -> getStorageSize() == 1) {
          trimCharValue->getLocaleString()->prepend(' ');
        }
     }


     vid1.coerceType(*trimSource, NA_CHARACTER_TYPE);
     trimChar = (CharType*)&vid1.getType();

  } else
  if ( trimChar->getCharSet() != CharInfo::UnknownCharSet &&
       trimSource->getCharSet() == CharInfo::UnknownCharSet
     )
  {
     vid2.coerceType(*trimChar, NA_CHARACTER_TYPE);
     trimSource = (CharType*)&vid2.getType();
  } else
  if ( trimChar->getCharSet() == CharInfo::UnknownCharSet &&
       trimSource->getCharSet() == CharInfo::UnknownCharSet
     )
  {
     const CharType* desiredType =
       CharType::findPushDownCharType(getDefaultCharSet, 0);

     vid1.coerceType(*desiredType, NA_CHARACTER_TYPE);
     trimChar = (CharType*)&vid1.getType();
     vid2.coerceType(*trimChar, NA_CHARACTER_TYPE);
     trimSource = (CharType*)&vid2.getType();
  }

  if (NOT trimChar->isComparable(*trimSource, this, NAType::EmitErrIfAnyChar)) {
    // Per Ansi 6.7 SR 6(f), trim source and trim char must be comparable.
    //
    // 4063 The operands of a $0 function must be comparable character types.
    *CmpCommon::diags() << DgSqlCode(-4063) << DgString0(getTextForError());
    return NULL;
  }

  // Per Ansi 6.7 SR 6(g,h), the result
  // takes the collation and coercibility of the trim source.
  //
  Int32 size = trimSource->getDataStorageSize();

  if (DFS2REC::isBinaryString(vid2.getType().getFSDatatype()))
    {
      return new HEAP
	SQLBinaryString(HEAP, size,
                        trimChar->supportsSQLnull() OR trimSource->supportsSQLnull(),
                        TRUE);
    }
  else
    {
      return new HEAP
        SQLVarChar(HEAP, CharLenInfo(trimSource->getStrCharLimit(), size )
                   ,trimChar->supportsSQLnull() OR trimSource->supportsSQLnull()
                   ,trimSource->isUpshifted()
                   ,trimSource->isCaseinsensitive()
                   ,trimSource->getCharSet()
                   ,trimSource->getCollation()
                   ,trimSource->getCoercibility()
                   );
    }
}

// -----------------------------------------------------------------------
// member functions for class UnLogic
// -----------------------------------------------------------------------

const NAType *UnLogic::synthesizeType()
{
  NABoolean allowsUnknown = FALSE;

  // All Unary Ops evaluate to TRUE/FALSE except NOT which can also
  // evaluate to UNKNOWN
  switch(getOperatorType())
  {
    case ITM_NOT:
    {
      CMPASSERT(child(0).getValueId().getType().getTypeQualifier() == NA_BOOLEAN_TYPE);
      const SQLBooleanRelat& operand0 = (SQLBooleanRelat &) child(0).getValueId().getType();
      allowsUnknown = operand0.canBeSQLUnknown();
      break;
    }

    case ITM_IS_UNKNOWN:
    case ITM_IS_NOT_UNKNOWN:
    case ITM_IS_FALSE:
    case ITM_IS_TRUE:
      CMPASSERT(child(0).getValueId().getType().getTypeQualifier() == NA_BOOLEAN_TYPE);
      // Falling throuuuuuuuuuu
    case ITM_IS_NULL:
    case ITM_IS_NOT_NULL:
     allowsUnknown = FALSE;
     break;

    default:
      CMPASSERT(0); // Case not handled
      break;
  }

  return new HEAP SQLBooleanRelat(HEAP, allowsUnknown);
}

// -----------------------------------------------------------------------
// member functions for class Translate
// -----------------------------------------------------------------------
const NAType *Translate::synthesizeType()
{
  //
  // Type cast any params.
  //
  ValueId vid = child(0)->getValueId();
  vid.coerceType(NA_CHARACTER_TYPE);
  //
  // Check that the operands are compatible.
  //

  const NAType& operand = vid.getType();
  if (NOT DFS2REC::isCharacterString(operand.getFSDatatype())) {
    // 4043 The operand of TRANSLATE function must be character.
    *CmpCommon::diags() << DgSqlCode(-4043) << DgString0(getTextUpper());
    return NULL;
  }

  const CharType *translateSource = (CharType*)&operand;

  // pushdown ISO88591 if the charset is unknown at this time.
  switch ( getTranslateMapTableId() ) {
     case ISO88591_TO_UNICODE:
     case SJIS_TO_UCS2:
     case UTF8_TO_UCS2: case UTF8_TO_ISO88591: case UTF8_TO_SJIS:
     case SJIS_TO_UNICODE:
     case ISO88591_TO_UTF8:
     case SJIS_TO_UTF8:

        if ( translateSource->getCharSet() == CharInfo::UnknownCharSet)
        {
           CharInfo::CharSet assumedSrcCS = CharInfo::ISO88591;
           switch (getTranslateMapTableId())
           {
           case SJIS_TO_UCS2: case SJIS_TO_UNICODE: case SJIS_TO_UTF8:
             assumedSrcCS = CharInfo::SJIS; break;
           case UTF8_TO_UCS2: case UTF8_TO_ISO88591: case UTF8_TO_SJIS:
             assumedSrcCS = CharInfo::UTF8; break;
           default:
             break;
           }
           vid.coerceType(*CharType::desiredCharType(assumedSrcCS),
                          NA_CHARACTER_TYPE
                         );

           translateSource = (CharType*)&vid.getType();
        }
        break;
     case UCS2_TO_SJIS:
     case UCS2_TO_UTF8:
     case UNICODE_TO_SJIS:
     case UNICODE_TO_ISO88591:
     case KANJI_MP_TO_ISO88591:
     case KSC5601_MP_TO_ISO88591:
        if ( translateSource->getCharSet() == CharInfo::UnknownCharSet )
        {
           vid.coerceType(*CharType::desiredCharType(CharInfo::UNICODE),
                          NA_CHARACTER_TYPE
                          );
           translateSource = (CharType*)&vid.getType();
        }
        break;
  }

  CharInfo::CharSet charsetTarget = CharInfo::UnknownCharSet;
  NAString err4106arg(CmpCommon::statementHeap());

  switch (getTranslateMapTableId()) {
     case ISO88591_TO_UNICODE:
     case SJIS_TO_UCS2:
     case UTF8_TO_UCS2:
     case SJIS_TO_UNICODE:
        if (translateSource->getCharSet() == CharInfo::ISO88591
            || translateSource->getCharSet() == CharInfo::UTF8
            || translateSource->getCharSet() == CharInfo::SJIS
            )
        {
          charsetTarget = CharInfo::UNICODE;
        }
	else
	  switch (getTranslateMapTableId()) {
             case UTF8_TO_UCS2:
	          err4106arg = SQLCHARSETSTRING_UTF8; break;
             case SJIS_TO_UCS2:
             case SJIS_TO_UNICODE:
	          err4106arg = SQLCHARSETSTRING_SJIS; break;
             case ISO88591_TO_UNICODE:
             default:
	          err4106arg = SQLCHARSETSTRING_ISO88591; break;
	  }
	break;

     case UNICODE_TO_SJIS:
       if (translateSource->getCharSet() == CharInfo::UNICODE)
         charsetTarget = CharInfo::SJIS;
       else
         err4106arg = SQLCHARSETSTRING_UNICODE;
       break;
       
     case UNICODE_TO_ISO88591:
       if (translateSource->getCharSet() == CharInfo::UNICODE)
         charsetTarget = CharInfo::ISO88591;
       else
         err4106arg = SQLCHARSETSTRING_UNICODE;
       break;

     case UCS2_TO_SJIS:
       if (translateSource->getCharSet() == CharInfo::UNICODE)
         charsetTarget = CharInfo::SJIS;
       else
         err4106arg = SQLCHARSETSTRING_UNICODE;
       break;

     case UCS2_TO_UTF8:
       if (translateSource->getCharSet() == CharInfo::UNICODE)
         charsetTarget = CharInfo::UTF8;
       else
         err4106arg = SQLCHARSETSTRING_UNICODE;
       break;

     case KANJI_MP_TO_ISO88591:
        if (translateSource->getCharSet() == CharInfo::KANJI_MP)
          charsetTarget = CharInfo::ISO88591;
	else
	  err4106arg = SQLCHARSETSTRING_KANJI;
	break;

     case KSC5601_MP_TO_ISO88591:
        if (translateSource->getCharSet() == CharInfo::KSC5601_MP)
          charsetTarget = CharInfo::ISO88591;
	else
	  err4106arg = SQLCHARSETSTRING_KSC5601;
	break;

     case UTF8_TO_SJIS:
       if ( (translateSource->getCharSet() == CharInfo::UTF8) ||
            (translateSource->getCharSet() == CharInfo::ISO88591) )
         charsetTarget = CharInfo::SJIS;
       else
         err4106arg = SQLCHARSETSTRING_UTF8;
       break;

     case GBK_TO_UTF8:
       if (translateSource->getCharSet() == CharInfo::GBK )
         charsetTarget = CharInfo::UTF8;
       else
         err4106arg = SQLCHARSETSTRING_GBK;
       break;

     case ISO88591_TO_UTF8:
       if (translateSource->getCharSet() == CharInfo::ISO88591)
       {
         charsetTarget = CharInfo::UTF8;
       }
       else
         err4106arg = SQLCHARSETSTRING_ISO88591;
       break;

     case UTF8_TO_ISO88591:
       if ( (translateSource->getCharSet() == CharInfo::UTF8) ||
            (translateSource->getCharSet() == CharInfo::ISO88591) )
         charsetTarget = CharInfo::ISO88591;
       else
         err4106arg = SQLCHARSETSTRING_UTF8;
       break;

      default:
        // 4105 Unknown translation
        *CmpCommon::diags() << DgSqlCode(-4105);
        return NULL;
  }


  if (charsetTarget != CharInfo::UnknownCharSet)
    {
      Lng32 resultLen =
        CharInfo::getMaxConvertedLenInBytes(translateSource->getCharSet(),
                                            translateSource->getNominalSize(),
                                            charsetTarget);

      return new HEAP SQLVarChar(HEAP, CharLenInfo(translateSource->getStrCharLimit(), resultLen),
                                 TRUE, FALSE, 
                                 translateSource->isCaseinsensitive(),
                                 charsetTarget,
                                 CharInfo::DefaultCollation,
                                 CharInfo::IMPLICIT);	// ANSI 6.7 SR 5b
    }

  *CmpCommon::diags() << DgSqlCode(-4106)
    << DgString0(getTextUpper()) << DgString1(err4106arg);

  return NULL;
}

// -----------------------------------------------------------------------
// member functions for class ValueIdUnion
// -----------------------------------------------------------------------

const NAType *ValueIdUnion::synthesizeType()
{
  const NAType *result = NULL;

  CollIndex i = 0;

  // if this is the case of insert values list tuples, then
  // isTrueUnion() will not be set and isCastTo() will be set.
  // Last entry of sources_ valueidlist will be set to target valueId,
  // it is set in method TupleList::bindNode.
  // Validate that each source entry is compatible with target type.
  if ((NOT isTrueUnion()) &&
      (isCastTo()))
    {
      result = &getSource(entries()-1).getType();
      const NAType& opR = *result;
      for (i = 0; i < entries()-1; i++) 
        {
          getSource(i).coerceType(*result);
          ValueId vidI = getSource(i);
          
          const NAType& opI = vidI.getType();

          if ((NOT opR.isCompatible(opI)) &&
              (CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_OFF))
            {
              // 4055 The select lists or tuples must have compatible data types.
              emitDyadicTypeSQLnameMsg(-4055, opR, opI);
              return NULL;
            }
        } // for
      return result;
    }
  
  for (i = 0; i < entries(); i++) {
    result = &getSource(i).getType();
    if (result->getTypeQualifier() != NA_UNKNOWN_TYPE)
      break;
  }

  CMPASSERT(result);

  if (result->getTypeQualifier() == NA_UNKNOWN_TYPE)
    return result->newCopy(HEAP);

  CollIndex r = i;		// the r'th source was the first non-unknown

  for (i = 0; i < entries(); i++) {
    if (i != r) {		// r'th source started it all, we did it already
      getSource(i).coerceType(*result);
      ValueId vidi = getSource(i);

      const NAType& opR = *result;		// save operand BEFORE synth
      const NAType& opI = vidi.getType();
      UInt32 flags =
              ((CmpCommon::getDefault(LIMIT_MAX_NUMERIC_PRECISION) == DF_ON)
               ? NAType::LIMIT_MAX_NUMERIC_PRECISION : 0);
      if (CmpCommon::getDefault(TYPE_UNIONED_CHAR_AS_VARCHAR) == DF_ON)
        flags |= NAType::MAKE_RESULT_VARCHAR;

      if (CmpCommon::getDefault(MODE_SPECIAL_4) == DF_ON)
        {
          flags |= NAType::MODE_SPECIAL_4;
        }
      
      result = result->synthesizeType(SYNTH_RULE_UNION, opR, opI, HEAP, &flags);


      if (!result) {
	// 4055 The select lists or tuples must have compatible data types.
	emitDyadicTypeSQLnameMsg(-4055, opR, opI);
	//## Here, also emit errmsg 4034 w/ unparse?
	return NULL;
      }
      else if (getUnionFlags() == Union::UNION_DISTINCT) {
        if (NOT opR.isComparable(opI, this)) {
	  // 4134 The operation (x UNION y) is not allowed. Try UNION ALL.
	  *CmpCommon::diags() << DgSqlCode(-4134)
			      << DgString0(getText(USER_FORMAT_DELUXE));
	  return NULL;
	}
      }
    }
  }

  return result;
}

// ValueIdUnion::pushDownType() -----------------------------------
// Propagate type information down the ItemExpr tree.  This method
// is called by coerceType(). It will attempt to coerce (a recursive call)
// the type of each member of the ValueIdUnion to the desired type.
// This only has an effect when none of the members of the ValueIdUnion
// could be typed bottom up.  An example query that illustrates this
// is:
//      Select NULL from t1
//      Union all
//      Select NULL from t2
//      Union all
//      Select c    from t3;
//
// This results in a tree of ValueIdUnion nodes:
//      ValueIdUnion(c (ValueIdUnion(Null, Null)));
//
// The inner ValueIdUnion node can not be typed bottom up, but
// the outer ValueIdUnion node will attempt to coerce the type of
// the inner node. This will in turn (through pushDownType) coerce
// the types of the members (NULLs) of the inner ValueIdUnion node
// and re-synthesize the Type of the inner ValueIdUnion node.
//
//
const NAType *
ValueIdUnion::pushDownType(NAType& desiredType,
                           enum NABuiltInTypeEnum defaultQualifier)
{
  for(CollIndex i = 0; i < entries(); i++) {
    getSource(i).coerceType(desiredType, defaultQualifier);
  }

  return (NAType *)synthesizeType();

}


const NAType *
RowsetArrayScan::pushDownType(NAType& desiredType,
                           enum NABuiltInTypeEnum defaultQualifier)
{
  elemType_ = &desiredType;
  const NAType *newType = child(0)->pushDownType(desiredType, desiredType.getTypeQualifier());
  child(0)->getValueId().changeType(newType);
  //BEGIN 10-050523-8022
  //When datatype has constraint NOT_NULL_DROPPABLE , getting the
  //null indicator from
  //supportsSQLnullogical was leading to truncation of host data in
  //ExRowsetArrayScan::eval function of file exp_function.cpp.
  elemNullInd_ = desiredType.supportsSQLnull();
  //End 10-050523-8022
  return &desiredType;
}

const NAType *
HostVar::pushDownType(NAType& desiredType,
                      enum NABuiltInTypeEnum defaultQualifier)
{
  // If this is a rowset host var, we need to propagate the desired type into it
  if (varType_->getTypeQualifier() == NA_ROWSET_TYPE) {
    SQLRowset *rw1 = (SQLRowset *) varType_;
    SQLRowset *rw2 = new HEAP SQLRowset(HEAP, &desiredType, rw1->getMaxNumElements(),
                                                      rw1->getNumElements());
    NAType *tempType = &desiredType;
    rw2->setNullable(*tempType);
    varType_ = rw2;
    return rw2;
  }

  return &desiredType;
}


// -----------------------------------------------------------------------
// member functions for class ValueIdProxy
// -----------------------------------------------------------------------
const NAType *ValueIdProxy::synthesizeType()
{
  const NAType *proxyType = &getOutputId().getType();
  return proxyType->newCopy(HEAP);
}

// Propagate type information down the node we are Proxy for.
// Called by coerceType().  
//
const NAType *
ValueIdProxy::pushDownType(NAType& desiredType,
                       enum NABuiltInTypeEnum defaultQualifier)
{
   outputValueId_.coerceType(desiredType, defaultQualifier);
   return (NAType *)synthesizeType();
}

// -----------------------------------------------------------------------
// member functions for class VEGPredicate
// -----------------------------------------------------------------------

const NAType *VEGPredicate::synthesizeType()
{
  return new HEAP SQLBooleanRelat(HEAP);
}

// -----------------------------------------------------------------------
// member functions for class VEGReference
// $$$ WORK REMAINING TO BE DONE:
// $$$ compute the intersection of the datatypes
// $$$ of the members of the VEG and assign it as the type
// $$$ for the VEGReference.
// -----------------------------------------------------------------------

const NAType *VEGReference::synthesizeType()
{
  if (getVEG()->seenBefore())
    return NULL;

  getVEG()->markAsSeenBefore();

  NAType *type = NULL;
  if (NOT getVEG()->getAllValues().isEmpty())
  {
    // return the type of any one expression from the VEG.
    ValueId exprId;

    const ValueIdSet & vegValues = getVEG()->getAllValues();

    for (exprId = vegValues.init();
                  vegValues.next(exprId);
                  vegValues.advance(exprId))
    {
      if (exprId.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE)
      {
        // Saw a VEGReference inside the VEG which is not type synthesized
        // yet. Drive its synthesis if it's not a VEGReference to a VEG
        // which we are in the process of type synthesizing. In that case,
        // we could ignore the VEGRef, since (1) we want to avoid infinite
        // recursion; (2) it will get its type after the completion of an
        // earlier call to synthesizeType().
        //
        if (exprId.getValueDesc()->getDomainDesc() == NULL)
        {
          VEGReference *vegref = (VEGReference *)(exprId.getItemExpr());
          if (NOT vegref->getVEG()->seenBefore())
          {
            vegref->synthTypeAndValueId(FALSE);

            // Remember the first non-null type seen.
            if (type == NULL) type = (NAType *) &(exprId.getType());
          }
        }
        else
        {
          // Remember the first non-null type seen.
          if (type == NULL) type = (NAType *) &(exprId.getType());
        }
      }
      else
      {
        // Remember the first non-null type seen.
        if (type == NULL) type = (NAType *) &(exprId.getType());
      }
    }
  }
  else
  {
    type = new HEAP SQLUnknown(HEAP);
  }

  getVEG()->markAsNotSeenBefore();
  CMPASSERT(type);
  return type;
}

const NAType *ScalarVariance::synthesizeType()
{
  return new HEAP SQLDoublePrecision(HEAP, TRUE); // Variance is always Nullable
}

// UnPackCol::synthesizeType() --------------------------------
// The type of the UnPackCol is the type of the original unpacked
// column.  This type is store within the UnPackCol node.
//
const NAType *UnPackCol::synthesizeType()
{
  // The type of the original unpacked column.
  //
  return getType();
}

// RowsetArrayScan::synthesizeType() --------------------------------
// The type of the RowsetArrayScan is the type of the original unpacked
// element. This type is store within the RowsetArrayScan node.
//
const NAType *RowsetArrayScan::synthesizeType()
{
  // The element type
  return getType();
}

const NAType *RowsetArrayInto::synthesizeType()
{
  // The element type
  return getType();
}

const NAType *RandomNum::synthesizeType()
{
  NAType *result = NULL;

  if (getArity() == 1)
    {
      //
      // Type cast any params.
      //
      SQLInt nType(NULL, FALSE);
      ValueId vid = child(0)->getValueId();
      vid.coerceType(nType, NA_NUMERIC_TYPE);

      const NAType& operand = vid.getType();
      if (operand.getTypeQualifier() != NA_NUMERIC_TYPE) {
	// 4045 The operand of a Random function must be numeric.
	*CmpCommon::diags() << DgSqlCode(-4045) << DgString0(getTextUpper());
	return NULL;
      }

      // now it's safe to cast the types to numeric type
      const NumericType &ntyp1 = (NumericType &) operand;

      if (NOT ntyp1.isExact())
	{
	  // 4070 Random function is only defined for exact numeric types.
	  *CmpCommon::diags() << DgSqlCode(-4070) << DgString0(getTextUpper());
	  return NULL;
	}

      if (ntyp1.getScale() != 0)
	{
	  // 4047 Arguments of random function must have a scale of 0.
	  *CmpCommon::diags() << DgSqlCode(-4047) << DgString0(getTextUpper());
	  return NULL;
	}

     // return: int unsigned 
      result = (NAType *) new HEAP SQLInt(HEAP, FALSE, ntyp1.supportsSQLnullLogical());
    }
  else
    {
      // return: int unsigned not null 
      result = (NAType *) new HEAP SQLInt(HEAP, FALSE,FALSE);
    }

  return result;
}

const NAType *Mask::synthesizeType()
{
  // The expression is <op1> Mask <op2>.
  // Both operands must be exact numeric with scale 0.
  // The result can always fit into the data type of the first child.

  const NAType &typ1 = child(0)->getValueId().getType();
  const NAType &typ2 = child(1)->getValueId().getType();

  // Mask is an internal operator and errors are fatal
  CMPASSERT(typ1.getTypeQualifier() == NA_NUMERIC_TYPE AND
	    typ2.getTypeQualifier() == NA_NUMERIC_TYPE);

  // now it's safe to cast the types to numeric type
  const NumericType &ntyp1 = (NumericType &) typ1;
  const NumericType &ntyp2 = (NumericType &) typ2;

  // for now make sure both operands basically have the same binary
  // type, but this may be changed in the future
  CMPASSERT(ntyp1.isExact() AND ntyp2.isExact() AND
	    ntyp1.getScale() == 0 AND ntyp2.getScale() == 0 AND
	    ntyp1.getPrecision() == ntyp2.getPrecision() AND
            ntyp1.binaryPrecision() AND
            ntyp2.binaryPrecision());

  const NAType *result = typ1.newCopy(HEAP);

  return result;
}

const NAType *Shift::synthesizeType()
{
  // The expression is <op1> Shift <op2>.
  // Both operands must be exact numeric with scale 0.
  // The result can always fit into the data type of the first child.

  const NAType &typ1 = child(0)->getValueId().getType();
  const NAType &typ2 = child(1)->getValueId().getType();

  // Mask is an internal operator and errors are fatal
  CMPASSERT(typ1.getTypeQualifier() == NA_NUMERIC_TYPE AND
	    typ2.getTypeQualifier() == NA_NUMERIC_TYPE);

  // now it's safe to cast the types to numeric type
  const NumericType &ntyp1 = (NumericType &) typ1;
  const NumericType &ntyp2 = (NumericType &) typ2;

  // for now make sure both operands basically have the same binary
  // type, but this may be changed in the future
  CMPASSERT(ntyp1.isExact() AND ntyp2.isExact() AND
	    ntyp1.getScale() == 0 AND ntyp2.getScale() == 0 AND
            ntyp1.binaryPrecision() AND
            ntyp2.binaryPrecision());

  const NAType *result = typ1.newCopy(HEAP);

  return result;
}

// -----------------------------------------------------------------------
// member functions for class PackFunc
// -----------------------------------------------------------------------

const NAType* PackFunc::synthesizeType()
{
  // ---------------------------------------------------------------------
  // If format information is valid, type is already available. Otherwise,
  // compute type information for the result of the PackFunc from the type
  // information of its operand.
  // ---------------------------------------------------------------------
  if(NOT isFormatInfoValid_)
  {
    // Type of column to be packed.
    const NAType* columnType = &child(0)->getValueId().getType();
    deriveFormatInfoFromUnpackType(columnType);
  }
  return type_;
}

const NAType * ZZZBinderFunction::synthesizeType()
{
  // the synthesizeType method is needed only when we process an item
  // expression at DDL time. For DML the function gets transformed into
  // another function in the binder before we reach type synthesis
  switch (getOperatorType())
    {
    case ITM_DATEDIFF_YEAR:
    case ITM_DATEDIFF_MONTH:
    case ITM_DATEDIFF_QUARTER:
    case ITM_DATEDIFF_WEEK:
      return new HEAP SQLInt(HEAP, TRUE,
                             child(0)->getValueId().getType().supportsSQLnull() ||
                             child(1)->getValueId().getType().supportsSQLnull());
      
    case ITM_LEFT:
      {
        // make a temporary transformation for synthesizing the right type
        Substring *temp = new HEAP Substring(child(0).getPtr(),
                                             new HEAP ConstValue((Lng32) 1, (NAMemory *) HEAP),
                                             child(1));
        temp->synthTypeAndValueId();
        return temp->getValueId().getType().newCopy(HEAP);
      }

    case ITM_YEARWEEK:
    case ITM_YEARWEEKD:
      return new HEAP SQLNumeric(HEAP, 4,
                                 6,
                                 0,
                                 TRUE,
                                 child(0)->getValueId().getType().supportsSQLnull());

    default:
      // use the parent class implementation by default
      return BuiltinFunction::synthesizeType();
    }
}

const NAType *Subquery::synthesizeType()
{
  return new HEAP SQLBooleanRelat(HEAP);
}

const NAType *RowSubquery::synthesizeType()
{
  const NAType *rowType = &getSubquery()->selectList()->getValueId().getType();
  return rowType->newCopy(HEAP);
}

// Propagate type information down the compExpr of the RowSubquery
// Called by coerceType().  We only change type if the selectList of 
// the RowSubquery is of degree 1 and the returned value in the select
// list is of unknown or character type.
//
const NAType *
RowSubquery::pushDownType(NAType& desiredType,
                       enum NABuiltInTypeEnum defaultQualifier)
{
   
   // In the case where the select list of the rowSubquery contains
   // a dynamic parameter, we need to change its type..
   if ( getDegree() == 1 )
   {
     RelRoot *sq_root = (RelRoot *) getSubquery();
     ValueId outVid = sq_root->compExpr()[0];
     if ( outVid.getType().getTypeQualifier() == NA_UNKNOWN_TYPE ||
          outVid.getType().getTypeQualifier() == NA_CHARACTER_TYPE
        )
       outVid.coerceType(desiredType, defaultQualifier);
   }
   
   return (NAType *)synthesizeType();
}

const NAType *Exists::synthesizeType()
{
  // EXISTS predicate can never evaluate to Unknown
  return new HEAP SQLBooleanRelat(FALSE);
}

const NAType *QuantifiedComp::synthesizeType()
{
  // Genesis 10-980305-3294
  ItemExprList exprList1(child(0).getPtr(), HEAP);
  ItemExprList exprList2(getSubquery()->selectList(), HEAP);
  NABoolean allowsUnknown;
  NABoolean allowIncompatibleComparison = FALSE;
  if (CmpCommon::getDefault(ALLOW_INCOMPATIBLE_OPERATIONS) == DF_ON)
    allowIncompatibleComparison = TRUE; 
  if (!synthItemExprLists(exprList1, exprList2, allowIncompatibleComparison,
			  allowsUnknown, this))
    return NULL;
  return new HEAP SQLBooleanRelat(HEAP, allowsUnknown);
}

// MV,
const NAType *GenericUpdateOutputFunction::synthesizeType()
{
  const NAType *type = NULL;

  if( getOperator().match(ITM_JULIANTIMESTAMP) )
  {
    //
    // Type cast any params.
    //
    ValueId vid = child(0)->getValueId();
    SQLTimestamp timestampType(NULL);
    vid.coerceType(timestampType);
    //
    // Check that the operands are compatible.
    //
    const NAType& operand = vid.getType();
    if (operand.getTypeQualifier() != NA_DATETIME_TYPE) {
      // 4071 The operand of a JULIANTIMESTAMP function must be a datetime.
      *CmpCommon::diags() << DgSqlCode(-4071) << DgString0(getTextUpper());
      return NULL;
    }
  
    type = new HEAP SQLLargeInt(HEAP, TRUE, operand.supportsSQLnullLogical());
  }
  else
  {
    type = new HEAP SQLInt(HEAP, TRUE, FALSE);
  }

  return type;
}

//++Triggers,

const NAType *UniqueExecuteId::synthesizeType()
{
  return new HEAP SQLChar(HEAP, SIZEOF_UNIQUE_EXECUTE_ID, FALSE);
}


const NAType *GetTriggersStatus::synthesizeType()
{
  return new HEAP SQLChar(HEAP, TRIGGERS_STATUS_VECTOR_SIZE, FALSE);
}

const NAType *GetBitValueAt::synthesizeType()
{
  const NAType *operand1 = &child(0)->getValueId().getType();
  const NAType *operand2 = &child(1)->getValueId().getType();

  if (operand1->getTypeQualifier() != NA_CHARACTER_TYPE) {
    // 4051 Operand 1 must be character.
    *CmpCommon::diags() << DgSqlCode(-4051) << DgString0(getTextUpper());
    return NULL;
  }
  if (operand2->getTypeQualifier() != NA_NUMERIC_TYPE) {
    // 4052 Operand 2 must be numeric.
    *CmpCommon::diags() << DgSqlCode(-4052) << DgString0(getTextUpper());
    return NULL;
  }
  return new HEAP SQLInt(HEAP, FALSE, FALSE);
}

//--Triggers,

const NAType *IsBitwiseAndTrue::synthesizeType()
{
  const NAType *operand1 = &child(0)->getValueId().getType();
  const NAType *operand2 = &child(1)->getValueId().getType();

  if (operand1->getTypeQualifier() != NA_CHARACTER_TYPE) {
    // 4051 Operand 1 must be character.
    *CmpCommon::diags() << DgSqlCode(-4051) << DgString0(getTextUpper());
    return NULL;
  }

  if (operand1->getTypeQualifier() != NA_CHARACTER_TYPE) {
    // 4051 Operand 1 must be character.
    *CmpCommon::diags() << DgSqlCode(-4051) << DgString0(getTextUpper());
    return NULL;
  }

  return new HEAP SQLBooleanRelat(FALSE);
}

//--MV


const NAType *ItemList::synthesizeType()
{
  const NAType * elementType = &child(0)->getValueId().getType();

  SQLRecord *restOfRecord;

  if (child(1)->getOperatorType() == ITM_ITEM_LIST)
  {
    restOfRecord = (SQLRecord *)&child(1)->getValueId().getType();
    CMPASSERT(restOfRecord->getTypeQualifier() == NA_RECORD_TYPE);
  }
  else
  {
    restOfRecord = new HEAP SQLRecord(HEAP, &child(1)->getValueId().getType(),NULL);
  }

  return new HEAP SQLRecord(HEAP, elementType,restOfRecord);
}
// -----------------------------------------------------------------------
// member functions for class ItmSeqOffset
// -----------------------------------------------------------------------

const NAType *ItmSeqOffset::synthesizeType()
{
// Verify that child 1 is numeric.
// Return the type of child 0.

  const NAType &operand1 = child(0)->getValueId().getType();

  if (getArity() > 1)  {
    const NAType &operand2 = child(1)->getValueId().getType();

    if (operand2.getTypeQualifier() != NA_NUMERIC_TYPE) {
    // The second operand of an OFFSET function must be numeric.
    *CmpCommon::diags() << DgSqlCode(-4052) << DgString0(getTextUpper());
    return NULL;
    }
  }
  if (getArity() > 2)  {
    const NAType &operand3 = child(2)->getValueId().getType();

    if (operand3.getTypeQualifier() != NA_NUMERIC_TYPE) {
    // The third operand of an OFFSET function must be numeric.
    *CmpCommon::diags() << DgSqlCode(-4053) << DgString0(getTextUpper());
    return NULL;
    }
   }

 NAType *result = operand1.newCopy(HEAP);

 if(nullRowIsZero()) {
   result->setNullable(FALSE);
   CMPASSERT(result->getTypeQualifier() == NA_NUMERIC_TYPE ||
             result->getTypeQualifier() == NA_INTERVAL_TYPE);
 } else {
   result->setNullable(TRUE);
 }

 if (isOLAP())
 {
   result->setNullable(TRUE);
 }
 return result;
}


const NAType *ItmLagOlapFunction::synthesizeType()
{
// Return the type of child 0.
  const NAType &operand1 = child(0)->getValueId().getType();

  if (getArity() > 1)  {
    const NAType &operand2 = child(1)->getValueId().getType();

    if (operand2.getTypeQualifier() != NA_NUMERIC_TYPE) {
    // The second operand of an OFFSET function must be numeric.
    *CmpCommon::diags() << DgSqlCode(-4052) << DgString0(getTextUpper());
    return NULL;
    }

    // check the default expression which should have the same type as the
    // child(0).
    if (getArity() > 2)  {
        const NAType& typeForOp0 = child(0)->castToItemExpr()->getValueId().getType();
        const NAType& typeForDefault = child(2)->castToItemExpr()->getValueId().getType();
     
        UInt32 flags =
            ((CmpCommon::getDefault(LIMIT_MAX_NUMERIC_PRECISION) == DF_ON)
              ? NAType::LIMIT_MAX_NUMERIC_PRECISION : 0);
  
         if ( !(typeForOp0 == typeForDefault) ) {
          
                NATypeSynthRuleEnum rule = SYNTH_RULE_ADD;
          
                if ( typeForOp0.getTypeQualifier() == NA_CHARACTER_TYPE )
                  rule = SYNTH_RULE_CONCAT;
          
                const NAType* resultType = typeForOp0.synthesizeType(rule,
                                              typeForOp0,
                                              typeForDefault,
                                              HEAP, &flags);
          
                if ( !resultType ) {
                   *CmpCommon::diags() << DgSqlCode(-4141) << DgString0("LAG");
                   return NULL;
                }
          
                // Set the null attribute of the default value to TRUE.
                NAType* newType = typeForDefault.newCopy(HEAP);
                newType->setNullable(TRUE);
                ValueId vid2 = child(2)->castToItemExpr()->getValueId();
                vid2.changeType(newType);
         }
     }
  }

 NAType *result = operand1.newCopy(HEAP);

 result->setNullable(TRUE);

 return result;
}

// -----------------------------------------------------------------------
// member functions for class ItmSeqDiff1
// -----------------------------------------------------------------------

const NAType *ItmSeqDiff1::synthesizeType()
{
// Verify that children are numeric.
// Return the result type of child(0) - child(0).

 for (Int32 i = 0; i < getArity(); i++) {
   const NAType &operand = child(i)->getValueId().getType();
   NABuiltInTypeEnum  opType = operand.getTypeQualifier() ;
   if ((opType != NA_NUMERIC_TYPE &&
        opType != NA_DATETIME_TYPE &&
        opType != NA_INTERVAL_TYPE) ||
        !operand.isSupportedType())  {
    if (i == 0) {
     // The first operand of a DIFF1 function must be numeric.
     *CmpCommon::diags() << DgSqlCode(-4059) << DgString0(getTextUpper());
    }
    else {
     // The second operand of a DIFF1 function must be numeric.
     *CmpCommon::diags() << DgSqlCode(-4052) << DgString0(getTextUpper());
    }
   return NULL;
   }   // if not numeric
 }

 const NAType &operand1 = child(0)->getValueId().getType();
 const NAType *result1 = operand1.synthesizeType(SYNTH_RULE_SUB, operand1, operand1, HEAP);
 NAType       *result;
 if (getArity()==2)        // will be transformed into: DIFF1(child(0)) / DIFF1(child(1))
 {
  const NAType &operand2 = child(1)->getValueId().getType();
  const NAType *result2 = operand2.synthesizeType(SYNTH_RULE_SUB, operand2, operand2, HEAP);
  if (result2->getTypeQualifier() == NA_INTERVAL_TYPE)
  {
    result2 = new HEAP SQLLargeInt(HEAP, TRUE, FALSE );
  }
  result = (NAType *)result2->synthesizeType(SYNTH_RULE_DIV, *result1, *result2, HEAP);
 }
 else
 {
  result = (NAType *)result1;
 }

 result->setNullable(TRUE);
 return result;
}
// -----------------------------------------------------------------------
// member functions for class ItmSeqDiff2
// -----------------------------------------------------------------------

const NAType *ItmSeqDiff2::synthesizeType()
{
// Verify that children are numeric.
// Return the result type of child(0) - child(0).

 for (Int32 i = 0; i < getArity(); i++) {
   const NAType &operand = child(i)->getValueId().getType();
   NABuiltInTypeEnum  opType = operand.getTypeQualifier() ;
   if ((opType != NA_NUMERIC_TYPE &&
        opType != NA_DATETIME_TYPE &&
        opType != NA_INTERVAL_TYPE ) ||
        !operand.isSupportedType()) {
    if (i == 0) {
     // The first operand of a DIFF1 function must be numeric.
     *CmpCommon::diags() << DgSqlCode(-4059) << DgString0(getTextUpper());
    }
    else {
     // The second operand of a DIFF1 function must be numeric.
     *CmpCommon::diags() << DgSqlCode(-4052) << DgString0(getTextUpper());
    }
   return NULL;
   } // if not numeric
 }
 const NAType &operand1 = child(0)->getValueId().getType();
 const NAType *result1 = operand1.synthesizeType(SYNTH_RULE_SUB, operand1, operand1, HEAP);
 NAType       *result  = (NAType *)result1->synthesizeType(SYNTH_RULE_SUB, *result1, *result1, HEAP);

 if (getArity()==2)        // will be transformed into: DIFF2(child(0)) / DIFF1(child(1))
 {
  const NAType &operand2 = child(1)->getValueId().getType();
  const NAType *result2 = operand2.synthesizeType(SYNTH_RULE_SUB, operand2, operand2, HEAP);
  if (result2->getTypeQualifier() == NA_INTERVAL_TYPE)
  {
    result2 = new HEAP SQLLargeInt(HEAP, TRUE, FALSE );
  }
  result = (NAType *)result2->synthesizeType(SYNTH_RULE_DIV, *result, *result2, HEAP);
 }

 result->setNullable(TRUE);
 return result;
}

// -----------------------------------------------------------------------
// member functions for class ItmSeqRunningFunction
// -----------------------------------------------------------------------


const NAType *ItmSeqRunningFunction::synthesizeType()
{
  const NAType *result = NULL;

  if ((getOperatorType() == ITM_RUNNING_COUNT) ||
      (getOperatorType() == ITM_RUNNING_RANK)  ||
      (getOperatorType() == ITM_RUNNING_DRANK)  ||
      (getOperatorType() == ITM_RUNNING_CHANGE)) {
    result = new HEAP
      SQLLargeInt(HEAP, TRUE /* 'long long' on NSK can't be unsigned */,
		  FALSE /*not null*/);
  }
  else {
    const NAType& operand = child(0)->castToItemExpr()->getValueId().getType();

    switch (getOperatorType())  {
     case ITM_RUNNING_AVG:  {         // needs to mimic what will happen after transformation
       const NAType *operand1 = synthAvgSum(operand, FALSE);
       const NAType *newInt =  new HEAP SQLLargeInt(HEAP, TRUE,FALSE);
       if (operand1){
        result = operand1->synthesizeType(SYNTH_RULE_DIV, *operand1, *newInt, HEAP);
       }
     }
       break;

     case ITM_RUNNING_SUM:
       result = synthAvgSum(operand, FALSE);
       break;

     case ITM_LAST_NOT_NULL:
     case ITM_RUNNING_MAX:
     case ITM_RUNNING_MIN:
       result = operand.newCopy(HEAP);
       break;

     case ITM_RUNNING_SDEV:
     case ITM_RUNNING_VARIANCE:
       result = new HEAP SQLDoublePrecision(HEAP, TRUE); // See ScalarVariance::synthesizeType()
       break;

     default:
       CMPASSERT("Unknown running function in synthesizeType().");
       break;
   }     //  end switch getOperatorType()
   if (result){
    ((NAType *)result)->setNullable(TRUE);
   }
  }  // end else not RUNNINGCOUNT

  return result;
}



const NAType *ItmSeqOlapFunction::synthesizeType()
{
  const NAType *result = NULL;
 
  if (getOperatorType() == ITM_OLAP_COUNT) 
  {
    result = new HEAP
              SQLLargeInt(HEAP, TRUE /* 'long long' on NSK can't be unsigned */,
		          TRUE /* null*/);
  }
  else
  if (/*(getOperatorType() == ITM_OLAP_COUNT) || */ //-- causes runtime error: ERROR[8421] NULL cannot be assigned to a NOT NULL column.
      (getOperatorType() == ITM_OLAP_RANK)  ||
      (getOperatorType() == ITM_OLAP_DRANK)) 
  {
    result = new HEAP
              SQLLargeInt(HEAP, TRUE /* 'long long' on NSK can't be unsigned */,
		          FALSE /*not null*/);
  }
  else 
  {
    const NAType& operand = child(0)->castToItemExpr()->getValueId().getType();

    switch (getOperatorType())  
    {
     case ITM_OLAP_AVG:  
     {         // needs to mimic what will happen after transformation
       const NAType *operand1 = synthAvgSum(operand, FALSE);
       const NAType *newInt =  new HEAP SQLLargeInt(HEAP, TRUE, TRUE /*FALSE*/);
       if (operand1)
       {
        result = operand1->synthesizeType(SYNTH_RULE_DIV, *operand1, *newInt, HEAP);
       }
     }
     break;

     case ITM_OLAP_SUM:
       result = synthAvgSum(operand, FALSE);
       break;

     case ITM_OLAP_MAX:
     case ITM_OLAP_MIN:
       result = operand.newCopy(HEAP);
       break;

     case ITM_OLAP_SDEV:
     case ITM_OLAP_VARIANCE:
       result = new HEAP SQLDoublePrecision(HEAP, TRUE); 
       break;

     default:
       CMPASSERT("Unknown running function in synthesizeType().");
       break;
    }     //  end switch getOperatorType()
    if (result)
    {
     ((NAType *)result)->setNullable(TRUE);
    }
  }  // end else not RUNNINGCOUNT

  return result;
}

// -----------------------------------------------------------------------
// member functions for class ItmSeqRowsSince
// -----------------------------------------------------------------------
const NAType *ItmSeqRowsSince::synthesizeType()
{
  if (getArity() == 2)
  {
    const NAType &operand2 = child(1)->getValueId().getType();
    if (operand2.getTypeQualifier() != NA_NUMERIC_TYPE) {
    // The second operand of a ROWS SINCE function must be numeric.
    *CmpCommon::diags() << DgSqlCode(-4052) << DgString0(getTextUpper());
    return NULL;
    }
  }
  return  new HEAP
    SQLInt(HEAP, TRUE /* 'long long' on NSK can't be unsigned */,
	   TRUE /* nullable */);
}

// -----------------------------------------------------------------------
// member functions for class ItmSeqMovingFunction
// -----------------------------------------------------------------------

const NAType *ItmSeqMovingFunction::synthesizeType()
{
  const NAType *result=NULL;

  //
  // Verify that moving window sizes are numeric values
  //
  NABoolean child2isOK = TRUE;
  const NAType &operand1 = child(1)->getValueId().getType();

  if (operand1.getTypeQualifier() != NA_NUMERIC_TYPE)  {
   // The second operand of a MOVING sequence function must be numeric.
   *CmpCommon::diags() << DgSqlCode(-4052) << DgString0(getTextUpper());
   return NULL;
  }

  if (getArity() > 2 )                      // check child(2) type
  {
    const NAType &operand2 = child(2)->getValueId().getType();

    if (operand2.getTypeQualifier() != NA_NUMERIC_TYPE)  {
    // The third operand of a MOVING sequence function must be numeric.
    *CmpCommon::diags() << DgSqlCode(-4053) << DgString0(getTextUpper());
    return NULL;
    }
  }

  if ((getOperatorType() == ITM_MOVING_COUNT) ||
      (getOperatorType() == ITM_MOVING_RANK)  ||
      (getOperatorType() == ITM_MOVING_DRANK)) {
    result = new HEAP
      SQLLargeInt(HEAP, TRUE /* 'long long' on NSK can't be unsigned */,
		  FALSE /*not null*/);
  }
  else  {
   const NAType& operand = child(0)->castToItemExpr()->getValueId().getType();

   switch (getOperatorType())  {
    case ITM_MOVING_AVG: {          // needs to mimic what will happen after transformation
       const NAType *operand1 = synthAvgSum(operand, FALSE);
       const NAType *newInt =  new HEAP SQLLargeInt(HEAP, TRUE,FALSE);
       if (operand1) {
       result = operand1->synthesizeType(SYNTH_RULE_DIV, *operand1, *newInt, HEAP);
       }
    }
       break;

    case ITM_MOVING_SUM:
       result = synthAvgSum(operand, FALSE);
       break;

    case ITM_MOVING_MAX:
    case ITM_MOVING_MIN:
       result = operand.newCopy(HEAP);
       break;

    case ITM_MOVING_SDEV:
    case ITM_MOVING_VARIANCE:
       result = new HEAP SQLDoublePrecision(HEAP, TRUE); // See ScalarVariance::synthesizeType()
       break;

    default:
       CMPASSERT("Unknown moving function in synthesizeType().");
       break;
   }   // end switch getOperatorType()
   if (result)  {
    ((NAType *)result)->setNullable(TRUE);
   }
  }  // end else not MOVING_COUNT
  return result;

}
// -----------------------------------------------------------------------
// member functions for class ItmSeqThisFunction
// -----------------------------------------------------------------------

const NAType *ItmSeqThisFunction::synthesizeType()
{
// Return the type of child

 const NAType &operand = child(0)->getValueId().getType();
 NAType *result = operand.newCopy(HEAP);
 result->setNullable(TRUE);
 return result;
}

// -----------------------------------------------------------------------
// member functions for class ItmScalarMinMax
// -----------------------------------------------------------------------

const NAType *ItmScalarMinMax::synthesizeType()
{
  // The expression is SCALAR_MIN(<val1>, <val2>) or SCALAR_MAX(<val1>, <val2>)
  // The result is the min or max value of the operands.

  ValueId valId1 = child(0)->getValueId();
  ValueId valId2 = child(1)->getValueId();
  //
  // Type cast any params.
  //
  valId1.coerceType(valId2.getType(), NA_NUMERIC_TYPE);
  valId2.coerceType(valId1.getType());
  //
  // Synthesize the result.
  //
  const NAType& op1 = valId1.getType();
  const NAType& op2 = valId2.getType();
  UInt32 flags =
            ((CmpCommon::getDefault(LIMIT_MAX_NUMERIC_PRECISION) == DF_ON)
             ? NAType::LIMIT_MAX_NUMERIC_PRECISION : 0);

  const NAType *result =  op1.synthesizeType(SYNTH_RULE_UNION,
					     op1,
					     op2,
					     HEAP,
                                          &flags);
  if (result == NULL) {
    // 4041 Type $1 cannot be compared with type $2.
    emitDyadicTypeSQLnameMsg(-4041, op1, op2);
	return NULL;
  }

  if (result->getTypeQualifier() == NA_CHARACTER_TYPE) {
    CharType *ct = (CharType *)result;
    propagateCoAndCoToChildren(this, ct->getCollation(), ct->getCoercibility());
  }

  return result;
}

// -----------------------------------------------------------------------
// member functions for class ItmSeqNotTHISFunction
// -----------------------------------------------------------------------

const NAType *ItmSeqNotTHISFunction::synthesizeType()
{
// Return the type of child

 const NAType &operand = child(0)->getValueId().getType();
 NAType *result = operand.newCopy(HEAP);
 result->setNullable(TRUE);
 return result;
}

const NAType * HbaseColumnLookup::synthesizeType()
{
  NAType * type = NULL;

  if (naType_)
    type = (NAType*)naType_;
  else
    type = new HEAP SQLVarChar(HEAP, 100000);

  return type;
}

const NAType * HbaseColumnsDisplay::synthesizeType()
{
  NAType * type = new HEAP SQLVarChar(HEAP, displayWidth_);

  return type;
}

const NAType * HbaseColumnCreate::synthesizeType()
{
  NAType * type = NULL;

  if (resultType_)
    type = (NAType*)resultType_;
  else
    type = new HEAP SQLVarChar(HEAP, 100000);

  return type;
}

const NAType * SequenceValue::synthesizeType()
{
  NAType * type = NULL;

  type = new HEAP SQLLargeInt(HEAP, TRUE, FALSE);

  return type;
}

const NAType * HbaseTimestamp::synthesizeType()
{
  NAType * type = NULL;

  type = new HEAP SQLLargeInt(HEAP, TRUE,  
                              col_->getValueId().getType().supportsSQLnull());

  return type;
}

const NAType * HbaseVersion::synthesizeType()
{
  NAType * type = NULL;

  type = new HEAP SQLLargeInt(HEAP, TRUE, FALSE); 

  return type;
}

const NAType * RowNumFunc::synthesizeType()
{
  NAType * type = NULL;

  type = new HEAP SQLLargeInt(HEAP, TRUE, FALSE);

  return type;
}

const NAType *LOBoper::synthesizeType()
{
  // Return blob or clob type
  
  NAType *result = new HEAP SQLBlob(HEAP, ((Int64) CmpCommon::getDefaultNumeric(LOB_MAX_SIZE)*1024*1024));

  if (child(0))
    {
      ValueId vid1 = child(0)->getValueId();
      const NAType &typ1 = (NAType&)vid1.getType();
      
      if (typ1.getFSDatatype() == REC_BLOB)
	{
	  result = new HEAP SQLBlob(HEAP, ((Int64) CmpCommon::getDefaultNumeric(LOB_MAX_SIZE)*1024*1024), Lob_Invalid_Storage,
				    typ1.supportsSQLnull());
	}
      else if (typ1.getFSDatatype() == REC_CLOB)
	{
	  result = new HEAP SQLClob(HEAP, ((Int64) CmpCommon::getDefaultNumeric(LOB_MAX_SIZE)*1024*1024), Lob_Invalid_Storage,
				    typ1.supportsSQLnull());
	}
    } 
  return result;
}

const NAType *LOBinsert::synthesizeType()
{
  // Return blob type
  ValueId vid1;
  const NAType *typ1 = NULL;
  if (child(0))
    {
      vid1 = child(0)->getValueId();
      typ1 = &vid1.getType();     
    }

  if ((obj_ == STRING_) ||
      (obj_ == FILE_) ||
      (obj_ == EXTERNAL_) ||
      (obj_ == LOAD_))
    {

        if (typ1 && typ1->getTypeQualifier() != NA_CHARACTER_TYPE)
	{          
          if (!lobAsVarchar())
            {
              // 4221 The operand of a $0~String0 function must be character.
              *CmpCommon::diags() << DgSqlCode(-4221) << DgString0("LOBINSERT")
                                  << DgString1("CHARACTER");
              return NULL;
            }
	}
    }
  else if (obj_ == LOB_)
    {
      if (typ1 && typ1->getTypeQualifier() != NA_LOB_TYPE)
	{
	  // 4043 The operand of a $0~String0 function must be blob
	  *CmpCommon::diags() << DgSqlCode(-4221) << DgString0("LOBINSERT")
			      << DgString1("LOB");
	  return NULL;
	}
    }
  else if (obj_ == BUFFER_)
    {
     if (typ1 && typ1->getTypeQualifier() != NA_NUMERIC_TYPE)
	{
	  // 4043 The operand of a $0~String0 function must be blob
	  *CmpCommon::diags() << DgSqlCode(-4221) << DgString0("LOBINSERT")
			      << DgString1("LARGEINT");
	  return NULL;
	} 
    }
  else if(obj_ == EMPTY_LOB_)
    {
    }
  else 
    {
      // 4221 The operand of a $0~String0 function must be character.
      *CmpCommon::diags() << DgSqlCode(-4221) << DgString0("LOBINSERT")
			  << DgString1("BLOB");
      return NULL;
    }
  
    

  NAType * result = NULL;
  if (lobFsType() == REC_BLOB)
    {
      result = new HEAP SQLBlob(HEAP, lobSize(), Lob_Invalid_Storage,
				(obj_ ==EMPTY_LOB_) ? FALSE:typ1->supportsSQLnull());
    }
  else if (lobFsType() == REC_CLOB)
    {
      result = new HEAP SQLClob(HEAP, lobSize(), Lob_Invalid_Storage,
				(obj_ == EMPTY_LOB_)? FALSE:typ1->supportsSQLnull());
    }
    
  return result;
 }

const NAType *LOBupdate::synthesizeType()
{
  // Return blob type

  ValueId vid1,vid2 ;
  const NAType *typ1 = NULL;
  const NAType *typ2 = NULL;

  if(child(0))
    {
      vid1= child(0)->getValueId();
      typ1 = &vid1.getType();
    }

  if(child(1))
    {
      vid2 = child(1)->getValueId();
      typ2 = &vid2.getType();
    }

 

  if ((obj_ == STRING_) ||
      (obj_ == FILE_) ||
      (obj_ == EXTERNAL_))
    {
     
       if (typ1->getTypeQualifier() != NA_CHARACTER_TYPE)
	{
          if(!lobAsVarchar())
            {
              // 4221 The operand of a $0~String0 function must be character.
              *CmpCommon::diags() << DgSqlCode(-4221) << DgString0("LOBUPDATE")
                                  << DgString1("CHARACTER");
              return NULL;
            }
	}
    }
  else if (obj_ == LOB_)
    {
      if (typ1->getTypeQualifier() != NA_LOB_TYPE)
	{
            
          // 4043 The operand of a $0~String0 function must be blob
          *CmpCommon::diags() << DgSqlCode(-4221) << DgString0("LOBUPDATE")
			      << DgString1("LOB");
          return NULL;
            
	}
    }
  else if (obj_ == BUFFER_)
    {
     if (typ1->getTypeQualifier() != NA_NUMERIC_TYPE)
	{
	  // 4043 The operand of a $0~String0 function must be blob
	  *CmpCommon::diags() << DgSqlCode(-4221) << DgString0("LOBUPDATE")
			      << DgString1("LARGEINT");
	  return NULL;
	} 
      ValueId vid3 = child(2)->getValueId();
      const  NAType &typ3 = (NAType&)vid3.getType();
      if (typ3.getTypeQualifier() != NA_NUMERIC_TYPE)
	{
	  // 4043 The operand of a $0~String0 function must be blob
	  *CmpCommon::diags() << DgSqlCode(-4221) << DgString0("LOBUPDATE")
			      << DgString1("LARGEINT");
	  return NULL;
	} 
    }
  else if (obj_ == EMPTY_LOB_)
    {
    }
  else 
    {
      // 4221 The operand of a $0~String0 function must be character.
      *CmpCommon::diags() << DgSqlCode(-4221) << DgString0("LOBUPDATE")
			  << DgString1("BLOB");
      return NULL;
    }

  NAType * result = NULL;
  if (typ2->getFSDatatype() == REC_BLOB)
    {
      SQLBlob &blob = (SQLBlob&)*typ2;
      result = new HEAP SQLBlob(HEAP, blob.getLobLength(), Lob_Invalid_Storage,
				(obj_ ==EMPTY_LOB_) ? FALSE:typ2->supportsSQLnull());
    }
  else if (typ2->getFSDatatype() == REC_CLOB)
    {
      SQLClob &clob = (SQLClob&)*typ2;

      result = new HEAP SQLClob(HEAP, clob.getLobLength(), Lob_Invalid_Storage,
				(obj_ ==EMPTY_LOB_) ? FALSE:typ2->supportsSQLnull());
    }
    
  return result;
 }

const NAType *LOBconvertHandle::synthesizeType()
{
  // Return blob type

  ValueId vid1 = child(0)->getValueId();
  const NAType &typ1 = (NAType&)vid1.getType();

  NAType *result = NULL;

  if (obj_ == STRING_)
    {
      if (typ1.getTypeQualifier() != NA_LOB_TYPE)
	{
	  // 4043 The operand of a $0~String0 function must be character.
	  *CmpCommon::diags() << DgSqlCode(-4221) << DgString0("LOBCONVERTHANDLE")
			      << DgString1("LOB");
	  return NULL;
	}
      
      if (typ1.getFSDatatype() == REC_BLOB)
	{
	  SQLBlob& op1 = (SQLBlob&)vid1.getType();
	  
	  result = new HEAP SQLBlob(HEAP, op1.getLobLength(), Lob_Invalid_Storage,
				    typ1.supportsSQLnull(), FALSE, 
				    TRUE);
	}
      else if (typ1.getFSDatatype() == REC_CLOB)
	{
	  SQLClob& op1 = (SQLClob&)vid1.getType();
	  
	  result = new HEAP SQLClob(HEAP, op1.getLobLength(), Lob_Invalid_Storage,
				    typ1.supportsSQLnull(), FALSE, 
				    TRUE);
	}
	
      return result;
    }
  else if (obj_ == LOB_)
    {
     if (typ1.getTypeQualifier() != NA_CHARACTER_TYPE)
	{
	  // 4221 The operand of a $0~String0 function must be character.
	  *CmpCommon::diags() << DgSqlCode(-4221) << DgString0("LOBCONVERTHANDLE")
			      << DgString1("CHARACTER");
	  return NULL;
	}
      
     result = new HEAP SQLBlob(HEAP, ((Int64) CmpCommon::getDefaultNumeric(LOB_MAX_SIZE)*1024*1024), Lob_Invalid_Storage, typ1.supportsSQLnull(), FALSE, 
					FALSE);
      return result;
    }
  else
    {
      *CmpCommon::diags() << DgSqlCode(-4221) << DgString0("LOBCONVERTHANDLE")
			  << DgString1("CHARACTER");
      return NULL;
    }
}

const NAType *LOBconvert::synthesizeType()
{
  // Return blob type

  ValueId vid1 = child(0)->getValueId();
  const NAType &typ1 = (NAType&)vid1.getType();

  if (obj_ == STRING_ || obj_ == FILE_)
    {
      if (typ1.getTypeQualifier() != NA_LOB_TYPE)
	{
	  // 4043 The operand of a $0~String0 function must be blob
	  *CmpCommon::diags() << DgSqlCode(-4221) << DgString0("LOBCONVERT")
			      << DgString1("LOB");
	  return NULL;
	}
      
      SQLlob& op1 = (SQLlob&)vid1.getType();

      Lng32 tgtSize = MINOF((Lng32)op1.getLobLength(), tgtSize_);


      NAType *result = new HEAP SQLVarChar(HEAP, tgtSize,
					   typ1.supportsSQLnull());
      return result;
    }
  else 
    {
      // 4221 The operand of a $0~String0 function must be character.
      *CmpCommon::diags() << DgSqlCode(-4221) << DgString0("LOBCONVERT")
			  << DgString1("LOB");
      return NULL;
    }
}

const NAType *LOBextract::synthesizeType()
{
  // Return blob type

  ValueId vid1 = child(0)->getValueId();
  const NAType &typ1 = (NAType&)vid1.getType();

  if (typ1.getTypeQualifier() != NA_LOB_TYPE)
    {
      // 4043 The operand of a $0~String0 function must be blob
      *CmpCommon::diags() << DgSqlCode(-4221) << DgString0("LOBEXTRACT")
			  << DgString1("LOB");
      return NULL;
    }
  
  SQLlob& op1 = (SQLlob&)vid1.getType();
  
  Lng32 tgtSize = MINOF((Lng32)op1.getLobLength(), tgtSize_);

  
  NAType *result = new HEAP SQLVarChar(HEAP, tgtSize,
				       typ1.supportsSQLnull());
  return result;
}

const NAType * ItmLeadOlapFunction::synthesizeType()
{
   // check the type of the offset operand, if present
   if (getArity() > 1)  {
      const NAType &operand2 = child(1)->getValueId().getType();

      NABoolean isInteger = FALSE;
      if (operand2.getTypeQualifier() == NA_NUMERIC_TYPE ) {

         const NumericType& nt = (NumericType&)operand2;
         if ( nt.isInteger() ) 
           isInteger = TRUE;
      }
            
         // The second operand of a LEAD function must be of integer type.
     if ( !isInteger ) {
         *CmpCommon::diags() << DgSqlCode(-4140) << DgString0(getTextUpper());
         return NULL;
     }

     // check the value of the offset expression constant
     NABoolean offsetOK = FALSE;
     Int64 value = 0;
  
     if ( getArity() > 1 )
     {
        ValueId vid1 = child(1)->getValueId();
        ItemExpr *offsetExpr = vid1.getItemExpr();
  
        if (offsetExpr) {
           if ( offsetExpr->getOperatorType() == ITM_CONSTANT ) 
           {
              ConstValue* cv = (ConstValue*)offsetExpr;
              if ( cv->canGetExactNumericValue() )
              {
                 value = cv->getExactNumericValue();
                 if ( value >= 0 ) {
                    offsetOK = TRUE;
                    offset_ = (Int32)value;
                 }
              }
           } else 
             offsetOK = TRUE; // delay making the decision. It coud be a row subquery
       }
  
     } else { 
  
        if ( offset_ >= 0 )
          offsetOK = TRUE;
     }
  
     if ( !offsetOK ) {
  
        *CmpCommon::diags() << DgSqlCode(-4249) << DgString0("LEAD");
        return NULL;
     }
   }

   // check the default expression which should have the same type as the
   // child(0).
   if (getArity() > 2)  {
      const NAType& typeForOp0 = child(0)->castToItemExpr()->getValueId().getType();
      const NAType& typeForDefault = child(2)->castToItemExpr()->getValueId().getType();

      UInt32 flags =
          ((CmpCommon::getDefault(LIMIT_MAX_NUMERIC_PRECISION) == DF_ON)
            ? NAType::LIMIT_MAX_NUMERIC_PRECISION : 0);

      if ( !(typeForOp0 == typeForDefault) ) {

         NATypeSynthRuleEnum rule = SYNTH_RULE_ADD;

         if ( typeForOp0.getTypeQualifier() == NA_CHARACTER_TYPE )
           rule = SYNTH_RULE_CONCAT;

         const NAType* resultType = typeForOp0.synthesizeType(rule,
                                       typeForOp0,
                                       typeForDefault,
                                       HEAP, &flags);

         if ( !resultType ) {
            *CmpCommon::diags() << DgSqlCode(-4141) << DgString0("LEAD");
            return NULL;
         }

         // Set the null attribute of the default value to TRUE.
         NAType* newType = typeForDefault.newCopy(HEAP);
         newType->setNullable(TRUE);
         ValueId vid2 = child(2)->castToItemExpr()->getValueId();
         vid2.changeType(newType);
      }
   }
    
   // the type of the LEAD() is the type of the 1st argument.
   const NAType& operand = child(0)->castToItemExpr()->getValueId().getType();
   NAType* result = operand.newCopy(HEAP);

   // LEAD can return NULL.
   result->setNullable(TRUE);

   return result;
}

const NAType * SplitPart::synthesizeType()
{
  ValueId vid1 = child(0)->getValueId(); 
  ValueId vid2 = child(1)->getValueId();
  ValueId vid3 = child(2)->getValueId();
  vid1.coerceType(NA_CHARACTER_TYPE);
  vid2.coerceType(NA_CHARACTER_TYPE);
  SQLInt si(NULL);
  vid3.coerceType(NA_NUMERIC_TYPE);

  const NAType *operand1 = &child(0)->getValueId().getType();
  const NAType *operand2 = &child(1)->getValueId().getType();
  const NAType *operand3 = &child(2)->getValueId().getType();

  if ((operand1->getTypeQualifier() != NA_CHARACTER_TYPE) 
      && (operand1->getFSDatatype() != REC_CLOB))
  {
    //4051 The first operand of a split_part function must be character.
    *CmpCommon::diags()<<DgSqlCode(-4051) << DgString0(getTextUpper());
    return NULL;
  }
  if ((operand2->getTypeQualifier() != NA_CHARACTER_TYPE)
      && (operand1->getFSDatatype() != REC_CLOB))
  {
    //4497 The second operand of a split_part function must be character.
    *CmpCommon::diags()<<DgSqlCode(-4497) << DgString0("second")
                                          << DgString1(getTextUpper())
                                          << DgString2("character");
    return NULL;
  }

  if (operand3->getTypeQualifier() != NA_NUMERIC_TYPE)
  {
    //4053 The third operand of a split_part function must be numeric.
    *CmpCommon::diags() << DgSqlCode(-4053) << DgString0(getTextUpper());
    return NULL;
  }

  const CharType *charOperand = (CharType *)operand1; 
  Lng32 maxLength_bytes = charOperand->getDataStorageSize();
  Lng32 maxLength_chars = charOperand->getPrecisionOrMaxNumChars();
  CharInfo::CharSet op1_cs = operand1->getCharSet();
  if (maxLength_chars <= 0) //if unlimited
    maxLength_chars = maxLength_bytes/CharInfo::minBytesPerChar(op1_cs);

  if (operand1->getFSDatatype() == REC_CLOB)
  {
    return new HEAP SQLClob(HEAP
                            , maxLength_bytes
                            , Lob_Invalid_Storage
                            , operand1->supportsSQLnull()
                                OR operand2->supportsSQLnull()
                                OR operand3->supportsSQLnull()
                           );
  } 

  return new HEAP SQLVarChar(HEAP
                             , CharLenInfo(maxLength_chars, maxLength_bytes)
                             , operand1->supportsSQLnull()
                                 OR operand2->supportsSQLnull()
                                 OR operand3->supportsSQLnull()
                             , charOperand->isUpshifted()
                             , charOperand->isCaseinsensitive()
                             , operand1->getCharSet()
                             , charOperand->getCollation()
                             , charOperand->getCoercibility()
                             );

}
