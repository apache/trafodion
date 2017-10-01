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
 * File:         RelStoredProc.cpp
 * Description:  The implementation for stored procedure related RelExprs.
 *
 * Created:      03/12/97
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// Contents: implementation for RelInternalSP

#include "RelStoredProc.h"  // definition of RelStoredProc::RelExpr
#include "CmpStoredProc.h"  // interface to the SP routines.
#include "BindWA.h"         // for binder related stuffs.
#include "CmpStatement.h"

// the following includes are for OptLogRelExpr and OptPhysRelExpr related
// methods, once those methods are moved to the appropriate files, the
// following include need to be removed

#include "Sqlcomp.h"
#include "GroupAttr.h"
#include "AllRelExpr.h"
#include "AllItemExpr.h"
#include "opt.h"
#include "PhyProp.h"
#include "Cost.h"
#include "EstLogProp.h"
#include <math.h>

#include "ComDistribution.h"

RelInternalSP::RelInternalSP(const NAString & procName, ItemExpr *params,
			     OperatorTypeEnum otype, CollHeap* oHeap, 
			     UInt16 arkcmpInfo)
  : TableValuedFunction(params, otype, oHeap),
    procName_(procName, oHeap), 
    outTableName_(oHeap),
    arkcmpInfo_(arkcmpInfo)
{}

RelInternalSP::RelInternalSP(const RelInternalSP & other, CollHeap * h)
  : TableValuedFunction(other,h),
    procName_(other.procName_, h), 
    outTableName_(other.outTableName_, h),
    procTypesTree_(other.procTypesTree_),
    procTypes_(other.procTypes_),
    arkcmpInfo_(other.arkcmpInfo_)
{
}

RelInternalSP::~RelInternalSP()
{
}

Int32 RelInternalSP::getArity() const 
{
  return 0;  
}

// -----------------------------------------------------------------------
// A virtual method for computing output values that an operator can
// produce potentially.
// -----------------------------------------------------------------------
void RelInternalSP::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues.clear();
  outputValues.insertList( getTableDesc()->getColumnList() );
} // RelInternalSP::getPotentialOutputValues()

void RelInternalSP::addLocalExpr(LIST(ExprNode *) &xlist,
				 LIST(NAString) &llist) const
{
  if (!procTypesTree_ || ! procTypes_.isEmpty())
    {
      if ( procTypes_.isEmpty() )
	xlist.insert(procTypesTree_);
      else
	xlist.insert(procTypes_.rebuildExprTree());
      llist.insert("procedure_parameter_types");      
    }
  TableValuedFunction::addLocalExpr(xlist,llist);
}

HashValue RelInternalSP::topHash()
{
  return TableValuedFunction::topHash();
}

NABoolean RelInternalSP::duplicateMatch(const RelExpr & other) const
{
  if ( !TableValuedFunction::duplicateMatch(other))
    return FALSE;
  RelInternalSP &o = (RelInternalSP &) other;

  if (! (procName_ == o.procName_) )
    return FALSE;

  return TRUE;
}

RelExpr *RelInternalSP::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelInternalSP* result;

  if (!derivedNode)
    result = new(outHeap) RelInternalSP("", 0, REL_INTERNALSP, outHeap);
  else
    result = (RelInternalSP *) derivedNode;

  result->procName_ = procName_;
  result->arkcmpInfo_ = arkcmpInfo_;

  return TableValuedFunction::copyTopNode(result, outHeap);
}

const NAString RelInternalSP::getText() const 
{
  NAString name("InternalSP ");
  name += procName_;
  return name;
}

NABoolean RelInternalSP::isQueryCacheVirtualTable() const 
{
    //for query cache virtual table ISPs, 
    //and they have 2 parameters and following ISP names.
    return ( getProcAllParamsVids().entries() == 2 && 
    ( procName_.compareTo("QUERYCACHE", NAString::ignoreCase)==0  || 
      procName_.compareTo("QUERYCACHEENTRIES", NAString::ignoreCase)==0 ||
      procName_.compareTo("QUERYCACHEDELETE", NAString::ignoreCase)==0 ));
}

NABoolean RelInternalSP::isHybridQueryCacheVirtualTable() const 
{
    //for query hybrid cache virtual table ISPs, 
    //and they have 2 parameters and following ISP names.
    return ( getProcAllParamsVids().entries() == 2 && 
    ( procName_.compareTo("HYBRIDQUERYCACHE", NAString::ignoreCase)==0  || 
      procName_.compareTo("HYBRIDQUERYCACHEENTRIES", NAString::ignoreCase)==0 ));
}

NABoolean RelInternalSP::isNATableCacheVirtualTable() const 
{
    //for query cache virtual table ISPs, 
    //and they have 2 parameters and following ISP names.
    return ( getProcAllParamsVids().entries() == 2 && 
    ( procName_.compareTo("NATABLECACHE", NAString::ignoreCase)==0  || 
      procName_.compareTo("NATABLECACHEENTRIES", NAString::ignoreCase)==0 ));
}

NABoolean RelInternalSP::isNARoutineCacheVirtualTable() const 
{
    //for query hybrid cache virtual table ISPs, 
    //and they have 2 parameters and following ISP names.
    return ( getProcAllParamsVids().entries() == 2 && 
    ( procName_.compareTo("NAROUTINECACHE", NAString::ignoreCase)==0  || 
      procName_.compareTo("NAROUTINECACHEENTRIES", NAString::ignoreCase)==0 ));
}

RelExpr* RelInternalSP::bindNode(BindWA *bindWA)
{

  // In this routine, it should not do a longjmp out of this routine, 
  // because the destructor of CmpInternalSP needs to be called to
  // end the connection with internal stored procedures. 

  if (nodeIsBound())
    return this;

  // Bind the children nodes
  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;

  CmpInternalSP* cmpInternalSP = new(bindWA->wHeap())
	  CmpInternalSP(procName_, bindWA->currentCmpContext());
  CMPASSERT(cmpInternalSP);

  bindWA->currentCmpContext()->statement()->setStoredProc(cmpInternalSP);

  outTableName_ = cmpInternalSP->OutTableName();
  CorrName outCorrName(outTableName_);
  outCorrName.setSpecialType(ExtendedQualName::ISP_TABLE);
  SchemaDB* schemaDB = bindWA->getSchemaDB();

  // bind the input parameters

  if (getProcAllParamsTree())
    {
      // convert the input parameters into ValueIdList
      ((ItemExpr *)getProcAllParamsTree())->
	convertToValueIdList(getProcAllParamsVids(),bindWA,ITM_ITEM_LIST);
      if (bindWA->errStatus()) return NULL;
	  
      Lng32 nParams = getProcAllParamsVids().entries();
      CmpSPInputFormat inputFormat(bindWA->currentCmpContext());
      if ( !cmpInternalSP->InputFormat(nParams, inputFormat) )
	// error, the info should be put into currentCmpContext()->diags()
	//already.
	{
	  bindWA->setErrStatus();	  
	  bindWA->currentCmpContext()->statement()->setStoredProc(0);
	  return 0;
	}      

       //This is for (hybrid)query cache ISP, like querycache('user'|'meta'|'all', 'remote'|'local')
       //the first param specifying instance(USER, META) to get,
       //the second specifying location the ISP will be executed, first param should be extracted in sp_process()
       //local -- within local process
       //remote  -- if running in embedded compiler exec ISP locally,
       // if in remote compiler send to remote arkcmp
      if( isQueryCacheVirtualTable() || isHybridQueryCacheVirtualTable() ||
          isNATableCacheVirtualTable() || isNARoutineCacheVirtualTable() )
      {
          //extract the location parameter
          const NAString locationParam = getProcAllParamsVids()[1].getItemExpr()->getText();
          if(locationParam.compareTo("'local'", NAString::ignoreCase)==0)
          {
              //set execute in local process
             arkcmpInfo_ |= executeInLocalProcess;
          }
          else//if the second param is not 'local', take as 'remote'
          {
              //clear execute in local bit
             arkcmpInfo_ &= ~executeInLocalProcess;
          }

          //if this location string is different from previously set one, error
          const NAString preLoc = bindWA->getISPExecLocation();
          if(preLoc.isNull()) { //set first time
              bindWA->setISPExecLocation(locationParam);
          }
          //preceding string is not identical to here now
          else if(preLoc.compareTo(locationParam, NAString::ignoreCase)!=0)
          {
             bindWA->setErrStatus();
             //"The location $0~string0 for $1~string1 does not match with another location $2~string2 specified. All location specifications must be identical.", 
             *(bindWA->currentCmpContext()->diags()) 
               << DgSqlCode(-1197) 
               << DgString0(locationParam.data())
               << DgString1(procName_.data())
               << DgString2(preLoc.data());
             return NULL;
          }
      }
      else 
      {//for other ISPs, without location parameter, 
       //which is default situation, executing in local process, 
       //see ExStoredProcTcb::work()  
          arkcmpInfo_ |= executeInLocalProcess;
      }
      
      // get the ItemExpr for input paramters.
      procTypesTree_ = inputFormat.itemExpr();
      procTypesTree_->convertToValueIdList(procTypes_,bindWA,ITM_ITEM_LIST);
      if (bindWA->errStatus()) return NULL;

      // traverse the procType_ itemExpr* list to check whether the
      // target datatype is compatible with the source getProcAllParamsTree()
      // entered in the query
      for ( Int32 i=0; i < nParams; i++ )
	{
	  //   Cannot be done with a stack-allocated tmpAssign
	  //   because ItemExpr destructor will delete children,
	  //   which we (and parent) are still referencing!
	  Assign *tmpAssign = new(bindWA->wHeap())
	    Assign(procTypes_[i].getItemExpr(),		// target
		   getProcAllParamsVids()[i].getItemExpr());	// source
	  const NAType *targetType = tmpAssign->synthesizeType(cmpInternalSP->procName(),
                                                               i+1);
	  if (!targetType) {
	    bindWA->setErrStatus();
	    bindWA->currentCmpContext()->statement()->setStoredProc(0);
	    return 0;
	  }
	}
    }
  else 
    {
      CmpSPInputFormat inputFormat(bindWA->currentCmpContext());
      if ( !cmpInternalSP->InputFormat(0, inputFormat) )
	{
	  bindWA->currentCmpContext()->statement()->setStoredProc(0);
	  bindWA->setErrStatus();	  
	  return 0;
	}      
    }      

  NABoolean b=TRUE;
  ConstValue* c;
  Int32 newInputSize = 0;

  if (getProcAllParamsVids().entries() > 0)
  {
    if (!getSuppressDefaultSchema())
      {

	Int32 inputSize;
	NAString defaultSchema = schemaDB->getDefaultSchema().getSchemaNameAsAnsiString();
	c = getProcAllParamsVids()[0].getItemExpr()->castToConstValue(b);
	if (c)
	  {
	    inputSize    = c->getRawText()->length();
	    newInputSize = inputSize + defaultSchema.length() + 2;
	  };
	if (newInputSize)
	  {
	    char* newInput = new(bindWA->wHeap()) char[newInputSize];
	    newInput[newInputSize - 1] = '\0';

	    str_cpy (newInput, c->getRawText()->data(), inputSize);
	    newInput[inputSize] = '$';

	    str_cpy (&newInput[inputSize+1], defaultSchema.data(), defaultSchema.length());

	    NAType *newType   = new (bindWA->wHeap()) SQLChar (bindWA->wHeap(), newInputSize, FALSE);
	    ItemExpr *newItem = new (bindWA->wHeap()) ConstValue(newType, newInput, newInputSize );
	    newItem->bindNode(bindWA);
	    getProcAllParamsVids()[0] = newItem->getValueId();
	    procTypes()[0].changeType (newType);
        // "newInput" is freed by bindWA->wHeap's destructor.
        // "newInput" cannot be safely freed here because newItem 
        // indirectly points to "newInput".
        // coverity[leaked_storage]
	  };
      }
    // call the SP parsing routine if the first parameter is a constant
    c = getProcAllParamsVids()[0].getItemExpr()->castToConstValue(b);
    if (c)
      {
	const NAString* constString = c->getRawText();
	if ( ! cmpInternalSP->ParseInput(constString->data()) )
	  {
	    bindWA->setErrStatus();	      
	    bindWA->currentCmpContext()->statement()->setStoredProc(0);
	    return 0;	        
	  }	  
      }
  };



  NATable *outNaTable = schemaDB->getNATableDB()->
    get(&(outCorrName.getExtendedQualNameObj()));
  if ( !outNaTable )
    {
      CmpSPOutputFormat outFormat(bindWA->currentCmpContext());
      if ( !cmpInternalSP->OutputFormat(outFormat) )
        return 0;      
      TrafDesc* outTableDesc = outFormat.tableDesc();      
      
      // the outTableDesc is expected to be deleteed in NATable::NATable
      if (outTableDesc)
	outNaTable = bindWA->getNATable(outCorrName, TRUE, outTableDesc);
      else
	outNaTable = 0;      
      
      if (bindWA->errStatus())
	{
	  bindWA->currentCmpContext()->statement()->setStoredProc(0);
	  return 0;      
	}      
    }

  if ( outNaTable )
  {
    // allocate a TableDesc and attach it to the RelInternalSP node
    setTableDesc(bindWA->createTableDesc(outNaTable, outCorrName));
    if (bindWA->errStatus())
    {
      bindWA->currentCmpContext()->statement()->setStoredProc(0);
      return this;
    }

    // Allocate and RETDesc and attach it to the RelStoredProc node
    // and the BindScope.

    setRETDesc(new(bindWA->wHeap()) RETDesc(bindWA, getTableDesc()));
  }

  // Bind the base class
  RelExpr* boundExpr = bindSelf(bindWA);
  if (bindWA->errStatus())
  {
    bindWA->currentCmpContext()->statement()->setStoredProc(0);
    return boundExpr;
  }

  // Assign the set of columns that belong to the virtual SP table
  // as the output values that can be produced by this node.

  if ( outNaTable )
    getGroupAttr()->addCharacteristicOutputs(getTableDesc()->getColumnList());

  bindWA->currentCmpContext()->statement()->setStoredProc(0);
  return boundExpr;
} // RelInternalSP::bindNode


// -----------------------------------------------------------------------
// methods might go into OptLogRelExpr.C
// -----------------------------------------------------------------------
void RelInternalSP::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp) == TRUE) 
    return;

  // Create a new Output Log Property with cardinality of 10 for now.
  EstLogPropSharedPtr myEstProps(new(STMTHEAP) EstLogProp((float)10.0));

  getGroupAttr()->addInputOutputLogProp(inputEstLogProp, myEstProps);
  
} // RelInternalSP::synthEstLogProp

void RelInternalSP::synthLogProp(NormWA * normWAPtr)
{
  // Check to see whether this GA has already been associated
  // with a logExpr for synthesis.  If so, no need to resynthesize 
  // for this equivalent log. expression.
  if (getGroupAttr()->existsLogExprForSynthesis()) return;

  RelExpr::synthLogProp(normWAPtr);

} // RelInternalSP::synthLogProp()

// -----------------------------------------------------------------------
// Methods should go into NormRelExpr.C
// -----------------------------------------------------------------------

void RelInternalSP::transformNode(NormWA & normWARef,
				  ExprGroupId & locationOfPointerToMe)
{
  TableValuedFunction::transformNode(normWARef,
				       locationOfPointerToMe);

  getProcAllParamsVids().transformNode(normWARef, locationOfPointerToMe,
			     getGroupAttr()->getCharacteristicInputs());
}

// This is needed because the RelRoot::transformNode will do pullPred
// which calls recomputeOuterReferences, since the getProcAllParamsVids() are
// not predicate, they will be removed from characteristicInput.

void RelInternalSP::recomputeOuterReferences()
{
  TableValuedFunction::recomputeOuterReferences();

  // Insert the input parameters into characteristicInputs
  //getGroupAttr()->addCharacteristicInputs(getProcAllParamsVids());   
}

void RelInternalSP::pushdownCoveredExpr(const ValueIdSet & outputExpr,
					const ValueIdSet & newExternalInputs,
					ValueIdSet & predicatesOnParent,
					const ValueIdSet * setOfValuesReqdByParent,
					Lng32 childIndex
					)
{

  TableValuedFunction::pushdownCoveredExpr(outputExpr,
    newExternalInputs,
    predicatesOnParent,
    setOfValuesReqdByParent,
    childIndex);
  return;

} // RelInternalSP::pushdownCoveredExpr

RelExpr* RelInternalSP::normalizeNode(NormWA & normWARef)
{
  TableValuedFunction::normalizeNode(normWARef);  
  return this;
}

////////////////////////////////////////////////////////////////////
// class RelUtilInternalSP
////////////////////////////////////////////////////////////////////
RelExpr* RelUtilInternalSP::bindNode(BindWA *bindWA)
{
  RelExpr * boundExpr = RelInternalSP::bindNode(bindWA);
  if (bindWA->errStatus()) 
    return this;

  // this node doesn't return any data.
  // Allocate an empty RETDesc and attach it to this.
  ((RelInternalSP*)boundExpr)->setRETDesc(
       new (bindWA->wHeap()) RETDesc(bindWA));

  return boundExpr;
}

void RelUtilInternalSP::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  // does not return any data
  outputValues.clear();
} // RelUtilInternalSP::getPotentialOutputValues()

RelExpr *RelUtilInternalSP::copyTopNode(RelExpr *derivedNode, CollHeap* outHeap)
{
  RelUtilInternalSP* result;

  if (!derivedNode)
    result = new(outHeap) RelUtilInternalSP("", 0, REL_UTIL_INTERNALSP, outHeap);
  else
    result = (RelUtilInternalSP *) derivedNode;

  return RelInternalSP::copyTopNode(result, outHeap);
}

// ---------------------------------------------------------------------
// Standalone function to determine the requirements of built-in functions
// activated through the TDMISP mechanism. Called from the parser.

// Table of flag values and built-in function names. Contains an entry per
// TDMISP built-in function that will have non-default flags values:
//
//   SpImport:        don't append default cat.sch to input parameter
//   SpCatApiRequest: don't append default cat.sch
//                    execute in same arkcmp
//                    requiresTMFTransaction
// 
// Future TDMISP built-in functions can be added to this list, with their 
// relevant flag value settings. Example:
//  ...
//  , { RelInternalSP::executeInSameArkcmp | RelInternalSP::requiresTMFTransaction, "SpFutureFunc" }

const literalAndEnumStruct SPFlagsValues [] =
{
  { RelInternalSP::suppressDefaultSchema, "SpImport" }
, { RelInternalSP::suppressDefaultSchema | RelInternalSP::executeInSameArkcmp | RelInternalSP::requiresTMFTransaction, "SpCatApiRequest" }
};


UInt16 getTdmispArkcmpInfo (const char * actualSP)
{
  NABoolean found;
  // Lookup the function flag values. Note that literalToEnum will
  // return 0 if the literal is not found - no need to explicitly
  // check for that since we would want to return zero in that case
  // anyway.
  return (UInt16) literalToEnum ( SPFlagsValues
                                , occurs(SPFlagsValues)
                                , actualSP
                                , found);
}
