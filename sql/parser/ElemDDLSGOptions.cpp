/* -*-C++-*-
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
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
 *****************************************************************************
 *
 * File:         ElemDDLSGOptions.C
 * Description:  methods for class ElemDDLSGOption and any classes
 *               derived from class ElemDDLSGOption.
 *
 * Created:      4/22/08
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComDiags.h"
#include "ElemDDLSGOptions.h"
#include "ElemDDLSGOption.h"
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#include "SqlParserGlobals.h"
#include "NAString.h"

// -----------------------------------------------------------------------
// methods for class ElemDDLSGOptions
// -----------------------------------------------------------------------

//
// constructors
//

ElemDDLSGOptions::ElemDDLSGOptions(OperatorTypeEnum operType)
 : ElemDDLNode(operType),
   ieType_(SG_UNKNOWN),
   cdType_(CD_UNKNOWN)
{
  setChild(INDEX_SG_OPT_LIST, NULL);

  initializeDataMembers();
}

ElemDDLSGOptions::ElemDDLSGOptions()
 : ElemDDLNode(ELM_SG_OPTS_DEFAULT_ELEM),
   ieType_(SG_UNKNOWN),
   cdType_(CD_UNKNOWN)
{
  setChild(INDEX_SG_OPT_LIST, NULL);

  initializeDataMembers();
}

ElemDDLSGOptions::ElemDDLSGOptions(Int32 ieType,
				   ElemDDLNode * pSGOptList)
	: ElemDDLNode(ELM_SG_OPTS_DEFAULT_ELEM)
	{
	  setChild(INDEX_SG_OPT_LIST, pSGOptList);

	  if (ieType == 1)
	    ieType_= SG_INTERNAL;
	  else if (ieType == 2)
	    ieType_ = SG_EXTERNAL;
	  else if (ieType == 3)
	    ieType_ = SG_INTERNAL_COMPUTED;
	  else
	    ieType_ = SG_UNKNOWN;

	  initializeDataMembers();
	}

// Virtual destructor
ElemDDLSGOptions::~ElemDDLSGOptions()
{
  // delete all children
  for (Int32 i = 0; i < getArity(); i++)
  {
    delete getChild(i);
  }
}

// cast virtual function
ElemDDLSGOptions *
ElemDDLSGOptions::castToElemDDLSGOptions()
{
  return this;
}

//
// accessors
//

// get the degree of this node
Int32
ElemDDLSGOptions::getArity() const
{
  return MAX_ELEM_DDL_SG_OPTS_ARITY;
}

ExprNode *
ElemDDLSGOptions::getChild(Lng32 index)
{ 
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

//
// mutators
//

void
ElemDDLSGOptions::initializeDataMembers()
{
  //
  // Sequence generator options
  //

  isStartValueSpec_      = FALSE;
  isIncrementSpec_       = FALSE;
  isMinValueSpec_        = FALSE;
  isMaxValueSpec_        = FALSE;
  isCycleSpec_           = FALSE;
  isCacheSpec_           = FALSE;
  isDatatypeSpec_       = FALSE;
  
  isNoMinValue_        = FALSE;
  isNoMaxValue_        = FALSE;
  cycle_               = FALSE;
  cache_               = 0;
  isNoCache_        = FALSE;

  startValue_      = 0;
  increment_       = 0;
  minValue_        = 0;
  maxValue_        = 0;

  fsDataType_  = COM_UNKNOWN_FSDT;

  //
  // Traverse the parse sub-tree containing the list of SG
  // options.  For each option found, set the corresponding
  // data member in this class.  Also check for duplicate clauses.
  //

  if (getChild(INDEX_SG_OPT_LIST) NEQ NULL)
  {
    ElemDDLNode * pSGOptList = getChild(INDEX_SG_OPT_LIST)
                                       ->castToElemDDLNode();
    numOptions_ = pSGOptList->entries();

    for (CollIndex i = 0; i < pSGOptList->entries(); i++)
    {
      setSGOpt((*pSGOptList)[i]);
    }
  }

} // ElemDDLSGOptions::initializeDataMembers()

void
ElemDDLSGOptions::setChild(Lng32 index, ExprNode * pChildNode)
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

//
// Set private data members corresponding to the SG options
// specified in a file option or load option phrases in a SG
// clause.  This method also looks for duplicate phrases.
//
void
ElemDDLSGOptions::setSGOpt(ElemDDLNode * pSGOpt)
{
  switch(pSGOpt->getOperatorType())
  {
    
 
    case ELM_SG_OPT_START_VALUE_ELEM :
    if (isStartValueSpec_)
    {
      // cout << "*** Error *** Duplicate options in sg option list.
      if (ieType_ == SG_INTERNAL)
        *SqlParser_Diags << DgSqlCode(-3427)
                         << DgString0("START WITH")
		         << DgString1("IDENTITY column");
      else
        *SqlParser_Diags << DgSqlCode(-3427)
                         << DgString0("START WITH")
		         << DgString1("sequence generator");
    }
    isStartValueSpec_ = TRUE;
    startValue_ =
      pSGOpt->castToElemDDLSGOptionStartValue()->getValue();
    break;

    case ELM_SG_OPT_INCREMENT_ELEM :
    if (isIncrementSpec_)
    {
      // cout << "*** Error *** Duplicate options in sg option list.
      if (ieType_ == SG_INTERNAL)
        *SqlParser_Diags << DgSqlCode(-3427)
                         << DgString0("INCREMENT BY")
		         << DgString1("IDENTITY column");
      else
        *SqlParser_Diags << DgSqlCode(-3427)
                         << DgString0("INCREMENT BY")
		         << DgString1("sequence generator");
    }
    isIncrementSpec_ = TRUE;
    increment_ =
      pSGOpt->castToElemDDLSGOptionIncrement()->getValue();
    break;

    case ELM_SG_OPT_MIN_VALUE_ELEM :
    if (isMinValueSpec_)
    {
      // cout << "*** Error *** Duplicate options in sg option list.
      if (ieType_ == SG_INTERNAL)
        *SqlParser_Diags << DgSqlCode(-3427)
                         << DgString0("MINVALUE")
		         << DgString1("IDENTITY column");
      else
        *SqlParser_Diags << DgSqlCode(-3427)
                         << DgString0("MINVALUE")
		         << DgString1("sequence generator");
    }
    isMinValueSpec_ = TRUE;
    minValue_ =
      pSGOpt->castToElemDDLSGOptionMinValue()->getValue();
    isNoMinValue_ =
      pSGOpt->castToElemDDLSGOptionMinValue()->isNoMinValue();
    break;

    case ELM_SG_OPT_MAX_VALUE_ELEM :
    if (isMaxValueSpec_)
    {
      // cout << "*** Error *** Duplicate options in sg option list.
      if (ieType_ == SG_INTERNAL)
        *SqlParser_Diags << DgSqlCode(-3427)
                         << DgString0("MAXVALUE")
		         << DgString1("IDENTITY column");
      else
        *SqlParser_Diags << DgSqlCode(-3427)
                         << DgString0("MAXVALUE")
		         << DgString1("sequence generator");
    }
    isMaxValueSpec_ = TRUE;
    maxValue_ =
      pSGOpt->castToElemDDLSGOptionMaxValue()->getValue();
    isNoMaxValue_ =
      pSGOpt->castToElemDDLSGOptionMaxValue()->isNoMaxValue();
    break;

    case ELM_SG_OPT_CYCLE_OPTION_ELEM :
    if (isCycleSpec_)
    {
      // cout << "*** Error *** Duplicate options in sg option list.
      if (ieType_ == SG_INTERNAL)
        *SqlParser_Diags << DgSqlCode(-3427)
                         << DgString0("CYCLE")
		         << DgString1("IDENTITY column");
      else
        *SqlParser_Diags << DgSqlCode(-3427)
                         << DgString0("CYCLE")
		         << DgString1("sequence generator");
    }
    isCycleSpec_ = TRUE;
    cycle_ =
      pSGOpt->castToElemDDLSGOptionCycleOption()->getValue();
    break;

    case ELM_SG_OPT_CACHE_OPTION_ELEM :
    if (isCacheSpec_)
    {
      // cout << "*** Error *** Duplicate options in sg option list.
      if (ieType_ == SG_INTERNAL)
        *SqlParser_Diags << DgSqlCode(-3427)
                         << DgString0("CACHE")
		         << DgString1("IDENTITY column");
      else
        *SqlParser_Diags << DgSqlCode(-3427)
                         << DgString0("CACHE")
		         << DgString1("sequence generator");
    }
    isCacheSpec_ = TRUE;
    cache_ =
      pSGOpt->castToElemDDLSGOptionCacheOption()->getCacheSize();
    isNoCache_ =
      pSGOpt->castToElemDDLSGOptionCacheOption()->isNoCache();
    break;

    case ELM_SG_OPT_DATATYPE_ELEM :
    if (isDatatypeSpec_)
    {
      // cout << "*** Error *** Duplicate options in sg option list.
      if (ieType_ == SG_INTERNAL)
        *SqlParser_Diags << DgSqlCode(-3427)
                         << DgString0("DATATYPE")
		         << DgString1("IDENTITY column");
      else
        *SqlParser_Diags << DgSqlCode(-3427)
                         << DgString0("DATATYPE")
		         << DgString1("sequence generator");
    }
    isDatatypeSpec_ = TRUE;
    fsDataType_ =
      pSGOpt->castToElemDDLSGOptionDatatype()->getDatatype();
    break;

  default :
    ABORT("internal logic error");
    break;
  }
}  // ElemDDLSGOptions::setSGOpt()

void ElemDDLSGOptions::setCDType(Int32 cdType)
{
  if (cdType == 1)
    cdType_= CD_GENERATED_BY_DEFAULT;
  else if (cdType == 2)
    cdType_ = CD_GENERATED_ALWAYS;
  else
    cdType_ = CD_UNKNOWN;
}

//
// method for binding
//

ExprNode *
ElemDDLSGOptions::bindNode(BindWA * /*pBindWA*/)
{

  markAsBound();
  return this;
}

// queryType:  0, create sequence.  1, alter sequence.  2, IDENTITY col.
short ElemDDLSGOptions::validate(short queryType)
{
  char queryTypeStr[40];

  if (queryType == 0)
    strcpy(queryTypeStr, "CREATE SEQUENCE");
  else if (queryType == 1)
    strcpy(queryTypeStr, "ALTER SEQUENCE");
  else
    strcpy(queryTypeStr, "IDENTITY column");

  Int64 minValue = 0;
  Int64 startValue = 0;
  Int64 increment = 0;
  Int64 maxValue = LONG_MAX - 1;

  NAString dtStr;
  if (fsDataType_ != COM_UNKNOWN_FSDT)
    {
      switch (fsDataType_)
        {
        case COM_UNSIGNED_BIN16_FSDT:
          maxValue = USHRT_MAX;
          dtStr = COM_SMALLINT_UNSIGNED_SDT_LIT;
          break;
        case COM_UNSIGNED_BIN32_FSDT:
          maxValue = UINT_MAX;
          dtStr = COM_INTEGER_UNSIGNED_SDT_LIT;
          break;
        case COM_SIGNED_BIN64_FSDT:
          maxValue = LONG_MAX - 1;
          dtStr =  COM_LARGEINT_SIGNED_SDT_LIT;
          break;
        default:
          *CmpCommon::diags() << DgSqlCode(-1510);
          return -1;
        }
    }

  if (queryType == 1) // alter
    {
      if ((isMinValueSpecified()|| isStartValueSpecified()))
        {
          *CmpCommon::diags() << DgSqlCode(-1592)
                              << DgString0(queryTypeStr);
          return -1;
        }
      
      if ((isMinValueSpecified()) && (isNoMinValue()))
        minValue = 1LL;
      else
        minValue = getMinValue();
      startValue = getStartValue();
      increment = getIncrement();

      maxValue = getMaxValue();
    }
  else
    {
      minValue = ((isMinValueSpecified() && (NOT isNoMinValue())) ? 
                  getMinValue() : 1LL); 
      startValue = (isStartValueSpecified() ? getStartValue() : minValue);
      increment = (isIncrementSpecified() ? getIncrement() : 1LL);
    } //else

  if (isMaxValueSpecified() && (NOT isNoMaxValue()))
    {
      if ((fsDataType_ != COM_UNKNOWN_FSDT) &&
          (getMaxValue() > maxValue))
        {
          *CmpCommon::diags() << DgSqlCode(-1576)
                              << DgString0("MAXVALUE")
                              << DgString1(dtStr);
          
          return -1;
        }

      maxValue = getMaxValue();
    }

  if (minValue == 0)
    {
      *CmpCommon::diags() << DgSqlCode(-1571)
			  << DgString0("MINVALUE")
			  << DgString1(queryTypeStr);
      
      return -1;
    }

  if (minValue < 0)
    {
      *CmpCommon::diags() << DgSqlCode(-1572)
			  << DgString0("MINVALUE")
			  << DgString1(queryTypeStr);
      
      return -1;
    }

  if (maxValue == 0)
    {
      *CmpCommon::diags() << DgSqlCode(-1571)
			  << DgString0("MAXVALUE")
			  << DgString1(queryTypeStr);
      
      return -1;
    }

  if (maxValue < 0)
    {
      *CmpCommon::diags() << DgSqlCode(-1572)
			  << DgString0("MAXVALUE")
			  << DgString1(queryTypeStr);
      
      return -1;
    }

  if (increment == 0)
    {
      *CmpCommon::diags() << DgSqlCode(-1571)
			  << DgString0("INCREMENT BY")
			  << DgString1(queryTypeStr);
      
      return -1;
    }

  if (increment < 0)
    {
      *CmpCommon::diags() << DgSqlCode(-1572)
			  << DgString0("INCREMENT BY")
			  << DgString1(queryTypeStr);  
      return -1;
    }

  if (startValue < 0)
    {
      *CmpCommon::diags() << DgSqlCode(-1572)
			  << DgString0("START WITH")
			  << DgString1(queryTypeStr);
      
      return -1;
    }

  if (maxValue <= minValue)
    {
      *CmpCommon::diags() << DgSqlCode(-1570)
			  << DgString0(queryTypeStr);      
      return -1;
    }

  if (startValue > maxValue)
    {
      *CmpCommon::diags() << DgSqlCode(-1573)
			  << DgString0(queryTypeStr);      
      return -1;
    }

  if (startValue < minValue)
    {
      *CmpCommon::diags() << DgSqlCode(-1573)
			  << DgString0(queryTypeStr);      
      return -1;
    }

  if (increment > (maxValue - minValue))
    {
      *CmpCommon::diags() << DgSqlCode(-1575)
			  << DgString0(queryTypeStr);      
      return -1;
    }

  Int64 cache = 0;
  double fMaxVal = maxValue;
  double fMinVal = MAXOF(startValue, minValue);
  Int64 minVal = MAXOF(startValue, minValue);
  double fRangeOfVals = ceil((fMaxVal-fMinVal+1)/increment);
  Int64 rangeOfVals = (maxValue-minVal+1);
  if (isCacheSpecified())
    cache = getCache();
  else
    {
      if (increment == 1)
	cache = MINOF(rangeOfVals, 25);
      else
	cache = MINOF(fRangeOfVals, 25);
    }

  if (NOT isNoCache())
    {
      if ((cache <= 1) ||
	  (cache > (increment == 1 ? rangeOfVals : fRangeOfVals)))
	{
	  *CmpCommon::diags() << DgSqlCode(-1577)
			      << DgString0(queryTypeStr);	  
	  return -1;
	}
    }
  
  if (increment == 1)
    cache = MINOF(rangeOfVals, cache);
  else
    cache = MINOF(fRangeOfVals, cache);

  setStartValue(startValue);
  setIncrement(increment);
  setMinValue(minValue);
  setMaxValue(maxValue);
  if (NOT isCacheSpecified())
    setCache(cache);

  return 0;
}

short ElemDDLSGOptions::genSGA(SequenceGeneratorAttributes &sga)
{
  sga.setSGStartValue(getStartValue());
  sga.setSGIncrement(getIncrement());
  sga.setSGMinValue(getMinValue());
  sga.setSGMaxValue(getMaxValue());

  sga.setSGCache(getCache());
  sga.setSGCycleOption(isCycle());

  sga.setSGFSDataType(getFSDataType());

  return 0;
}

short ElemDDLSGOptions::importSGA(const SequenceGeneratorAttributes *sga)
{
  initializeDataMembers();

  setFSDataType(sga->getSGFSDataType());
  setStartValue(sga->getSGStartValue());
  setIncrement(sga->getSGIncrement());
  setMinValue(sga->getSGMinValue());
  setMaxValue(sga->getSGMaxValue());

  setCache(sga->getSGCache());
  setCycle(sga->getSGCycleOption());

  return 0;
}

short ElemDDLSGOptions::importSGO(const ElemDDLSGOptions *sgo)
{
  if (sgo->isStartValueSpecified())
    {
      setStartValueSpec(TRUE);
      setStartValue(sgo->getStartValue());
    }

  if (sgo->isIncrementSpecified())
    {
      setIncrementSpec(TRUE);
      setIncrement(sgo->getIncrement());
    }

  if (sgo->isMinValueSpecified())
    {
      setMinValueSpec(TRUE);
      
      if (sgo->isNoMinValue())
        setNoMinValue(TRUE);
      else
        setMinValue(sgo->getMinValue());
    }

  if (sgo->isMaxValueSpecified())
    {
      setMaxValueSpec(TRUE);

      if (sgo->isNoMaxValue())
        setNoMaxValue(TRUE);
      else
        setMaxValue(sgo->getMaxValue());
    }

  if (sgo->isCacheSpecified())
    {
      setCacheSpec(TRUE);
      setCache(sgo->getCache());
    }

  if (sgo->isCycleSpecified())
    {
      setCycleSpec(TRUE);
      setCycle(sgo->isCycle());
    }

  return 0;
}

//
// methods for tracing
//

NATraceList
ElemDDLSGOptions::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailTextList.append("Sequence Generator Options:");
  detailText = "    SG Type:      ";

  if (isInternalSG())
    detailText = "INTERNAL ";
  else if (isExternalSG())
    detailText = "EXTERNAL ";
  else
    detailText = "UNKNOWN ";
  detailTextList.append(detailText);

  detailText = "    Start Value specified?   ";
  detailText += YesNo(isStartValueSpecified());
  detailTextList.append(detailText);

  detailText = "    Start Value:      ";
  detailText += Int64ToNAString(getStartValue());
  detailTextList.append(detailText);

  detailText = "    Increment specified?   ";
  detailText += YesNo(isIncrementSpecified());
  detailTextList.append(detailText);

  detailText = "    Increment:      ";
  detailText += Int64ToNAString(getIncrement());
  detailTextList.append(detailText);

  detailText = "    MaxValue specified?   ";
  detailText += YesNo(isMaxValueSpecified());
  detailTextList.append(detailText);

  if (isNoMaxValue())
  {
    detailText = "    Max Value:  NO MAXVAL      ";
    detailTextList.append(detailText);
  }
  else
  {
    detailText = "    Max Value:      ";
    detailText += Int64ToNAString(getMaxValue());
    detailTextList.append(detailText);
  }

  detailText = "    MinValue specified?   ";
  detailText += YesNo(isMinValueSpecified());
  detailTextList.append(detailText);

  if (isNoMinValue())
  {
    detailText = "    Min Value:  NO MINVAL      ";
    detailTextList.append(detailText);
  }
  else
  {
    detailText = "    Min Value:      ";
    detailText += Int64ToNAString(getMinValue());
    detailTextList.append(detailText);
  }

  detailText = "    Cycle specified?   ";
  detailText += YesNo(isCycleSpecified());
  detailTextList.append(detailText);

  if (isNoCycle())
  {
    detailText = "    Cycle Option:  NO CYCLE      ";
    detailTextList.append(detailText);
  }
  else
  {
    detailText = "    Cycle Option:  CYCLE      ";
    detailTextList.append(detailText);
  }

  detailText = "    Cache specified?   ";
  detailText += YesNo(isCacheSpecified());
  detailTextList.append(detailText);

  if (isNoCache())
  {
    detailText = "    Cache Option:  NO CACHE      ";
    detailTextList.append(detailText);
  }
  else
  {
    detailText = "    Cache Option:  CACHE      ";
    detailTextList.append(detailText);
  }

  return detailTextList;
}

const NAString
ElemDDLSGOptions::getText() const
{
  return "ElemDDLSGOptions";
}

// method for building text
// virtual 
NAString ElemDDLSGOptions::getSyntax() const
{
  ElemDDLSGOptions *ncThis = (ElemDDLSGOptions*)this;
  
  
  NAString syntax = "Sequence Generator Options: ";  

  if (NULL != ncThis->getChild(INDEX_SG_OPT_LIST))
  {
    ElemDDLNode * pSGOptList = 
	    ncThis->getChild(INDEX_SG_OPT_LIST)->castToElemDDLNode();

    for (CollIndex i = 0; i < pSGOptList->entries(); i++)
    {
      syntax += (*pSGOptList)[i]->getSyntax();
      syntax += " ";
    }
  }
  return syntax;
}

//
// End of File
//
