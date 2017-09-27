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
 * File:         ElemDDLPartition.C
 * Description:  methods for classes representing parse node elements
 *               specified in Partition clauses in DDL statements,
 *               including parse node elements representing
 *               Partition clauses.
 *
 * Created:      9/29/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComUnits.h"
#include "AllElemDDLPartition.h"
#include "BaseTypes.h"
#include "ComASSERT.h"
#include "ComDiags.h"
#include "ComOperators.h"
#include "ComLocationNames.h"
#include "ElemDDLKeyValue.h"
#include "ElemDDLLocation.h"
#include "ElemDDLPartitionList.h"
#include "ItemConstValueArray.h"
#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"


// -----------------------------------------------------------------------
// methods for class ElemDDLPartition
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPartition::~ElemDDLPartition()
{
}

// cast
ElemDDLPartition *
ElemDDLPartition::castToElemDDLPartition()
{
  return this;
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPartitionArray
//
//   Note that class ElemDDLPartitionArray is not derived from class
//   ElemDDLPartition.  The former represents an array of pointers
//   pointing to ElemDDLPartition parse node.
// -----------------------------------------------------------------------

// constructor
ElemDDLPartitionArray::ElemDDLPartitionArray(CollHeap *heap)
  : LIST(ElemDDLPartition *)(heap)
{
}

// virtual destructor
ElemDDLPartitionArray::~ElemDDLPartitionArray()
{
}

//----------------------------------------------------------------------------
// methods for class ElemDDLPartitionClause
//
//   Note that class ElemDDLPartitionClause is not derived from class
//   ElemDDLPartition.  The former is derived from class ElemDDLNode.
//----------------------------------------------------------------------------

// virtual destructor
ElemDDLPartitionClause::~ElemDDLPartitionClause()
{
}

// cast virtual function
ElemDDLPartitionClause *
ElemDDLPartitionClause::castToElemDDLPartitionClause()
{
  return this;
}

//
// accessors
//

// get the degree of this node
Int32
ElemDDLPartitionClause::getArity() const
{
  return MAX_ELEM_DDL_PARTITION_CLAUSE_ARITY;
}

ExprNode *
ElemDDLPartitionClause::getChild(Lng32 index)
{ 
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

//
// mutators
//

void
ElemDDLPartitionClause::setChild(Lng32 index, ExprNode * pChildNode)
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
// methods for tracing
//

const NAString
ElemDDLPartitionClause::getText() const
{
  return "ElemDDLPartitionClause";
}


// method for building text
//virtual 
NAString ElemDDLPartitionClause::getSyntax() const
{
  NAString syntax = "";
  
  switch(partitionType_)
  {
  case COM_SYSTEM_PARTITIONING:
    // this is the default - no syntax
    break;
  case COM_RANGE_PARTITIONING:
    syntax += "RANGE ";
    break;
  case COM_HASH_V1_PARTITIONING:
    syntax += "HASH ";
    break;
  case COM_HASH_V2_PARTITIONING:
    syntax += "HASH2 ";
    break;
  default:
    ComASSERT(FALSE);
  }

  syntax += "PARTITION ";


  ElemDDLNode * pElemDDL = getPartitionByOption();

  if(NULL != pElemDDL)
  {
    ElemDDLPartitionByColumnList * pPartitionByColList = 
			      pElemDDL->castToElemDDLPartitionByColumnList();

    ComASSERT(NULL != pPartitionByColList);

    syntax += "BY ";
    syntax += "(";
    syntax += pPartitionByColList->getSyntax();
    syntax += ") ";

  }

  pElemDDL = getPartitionDefBody();
  if(NULL != pElemDDL)
  {
    // pElemDDL dynamic type returned from the parser can be either an 
    // ElemDDLPartitionList or an ElemDDLPartitionRange
    ElemDDLPartitionList * pPartitionList = 
				      pElemDDL->castToElemDDLPartitionList();

    ElemDDLPartitionRange *pRange = pElemDDL->castToElemDDLPartitionRange();

    //++ MV
    syntax += "(";
    if(NULL != pPartitionList) 
    {
      syntax += pPartitionList->getSyntax();
    } 
    else 
    {
      ComASSERT(NULL != pRange);
      syntax += pRange->getSyntax();
    }
    syntax += ") ";
    //-- MV
  }


  return syntax;
} // getSyntax()


// -----------------------------------------------------------------------
// methods for class ElemDDLPartitionSystem
// -----------------------------------------------------------------------

//
// constructors
//

ElemDDLPartitionSystem::ElemDDLPartitionSystem()
: ElemDDLPartition(ELM_PARTITION_SYSTEM_ELEM),
  option_(ADD_OPTION),
  locationName_(PARSERHEAP()),
  guardianLocation_(PARSERHEAP()),
  partitionName_(PARSERHEAP())
{
  setChild(INDEX_LOCATION, NULL);
  setChild(INDEX_PARTITION_ATTR_LIST, NULL);

  initializeDataMembers();
}

ElemDDLPartitionSystem::ElemDDLPartitionSystem(
     ElemDDLPartition::optionEnum option,
     ElemDDLNode * pLocation,
     ElemDDLNode * pPartitionAttrList)
: ElemDDLPartition(ELM_PARTITION_SYSTEM_ELEM),
  option_(option),
  locationName_(PARSERHEAP()),
  guardianLocation_(PARSERHEAP()),
  partitionName_(PARSERHEAP())
{
  setChild(INDEX_LOCATION, pLocation);
  setChild(INDEX_PARTITION_ATTR_LIST, pPartitionAttrList);

  initializeDataMembers();
}

	ElemDDLPartitionSystem::ElemDDLPartitionSystem(
		 OperatorTypeEnum operType,
		 ElemDDLPartition::optionEnum option,
		 ElemDDLNode * pLocation,
		 ElemDDLNode * pPartitionAttrList)
	: ElemDDLPartition(operType),
	  option_(option),
	  locationName_(PARSERHEAP()),
	  guardianLocation_(PARSERHEAP()),
	  partitionName_(PARSERHEAP())
	{
	  setChild(INDEX_LOCATION, pLocation);
	  setChild(INDEX_PARTITION_ATTR_LIST, pPartitionAttrList);

	  initializeDataMembers();
	}

// Virtual destructor
ElemDDLPartitionSystem::~ElemDDLPartitionSystem()
{
  // delete all children
  for (Int32 i = 0; i < getArity(); i++)
  {
    delete getChild(i);
  }
}

// cast virtual function
ElemDDLPartitionSystem *
ElemDDLPartitionSystem::castToElemDDLPartitionSystem()
{
  return this;
}

//
// accessors
//

// get the degree of this node
Int32
ElemDDLPartitionSystem::getArity() const
{
  return MAX_ELEM_DDL_PARTITION_SYSTEM_ARITY;
}

ExprNode *
ElemDDLPartitionSystem::getChild(Lng32 index)
{ 
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

NAString
ElemDDLPartitionSystem::getOptionAsNAString() const
{
  switch (getOption())
  {
  case ADD_OPTION :
    return NAString("ADD");

  case DROP_OPTION :
    return NAString("DROP");
      
  default :
    ABORT("internal logic error");
    return NAString();
  }
}

//
// mutators
//

void
ElemDDLPartitionSystem::initializeDataMembers()
{
  //
  // file attributes
  //

  isMaxSizeSpec_        = FALSE;
  isMaxSizeUnbounded_   = FALSE;
  ParSetDefaultMaxSize(maxSize_, maxSizeUnit_);

  isExtentSpec_         = FALSE;
  ParSetDefaultExtents(priExt_, secExt_);

  isMaxExtentSpec_         = FALSE;
  ParSetDefaultMaxExtents(maxExt_);

  //
  // location
  //

  if (getChild(INDEX_LOCATION) EQU NULL)
  {
    locationNameType_ = ElemDDLLocation::LOCATION_DEFAULT_NAME_TYPE;
  }
  else
  {
    ElemDDLLocation * pLocation = getChild(INDEX_LOCATION)->
      castToElemDDLNode()->castToElemDDLLocation();
    locationName_ = pLocation->getLocationName();
    locationNameType_ = pLocation->getLocationNameType();
	partitionName_ = pLocation->getPartitionName();
  }

  //
  // Traverse the parse sub-tree containing the list of partition
  // attributes.  For each attribute found, set the corresponding
  // data member in this class.  Also check for duplicate clauses.
  //

  if (getChild(INDEX_PARTITION_ATTR_LIST) NEQ NULL)
  {
    ElemDDLNode * pPartitionAttrList = getChild(INDEX_PARTITION_ATTR_LIST)
                                       ->castToElemDDLNode();
    for (CollIndex i = 0; i < pPartitionAttrList->entries(); i++)
    {
      setPartitionAttr((*pPartitionAttrList)[i]);
    }
  }

} // ElemDDLPartitionSystem::initializeDataMembers()

void
ElemDDLPartitionSystem::setChild(Lng32 index, ExprNode * pChildNode)
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
// Set private data members corresponding to the partition attributes
// specified in a file attribute or load option phrases in a PARTITION
// clause.  This method also looks for duplicate phrases.
//
void
ElemDDLPartitionSystem::setPartitionAttr(ElemDDLNode * pPartitionAttr)
{
  switch(pPartitionAttr->getOperatorType())
  {
    
    case ELM_FILE_ATTR_EXTENT_ELEM :
    if (isExtentSpec_)
    {
      // Duplicate EXTENT phrases.
      *SqlParser_Diags << DgSqlCode(-3062);
    }
    isExtentSpec_ = TRUE;
    ComASSERT(pPartitionAttr->castToElemDDLFileAttrExtents() NEQ NULL);
    {
      ElemDDLFileAttrExtents * pExtents =
        pPartitionAttr->castToElemDDLFileAttrExtents();
      priExt_        = pExtents->getPriExtents();
      secExt_        = pExtents->getSecExtents();
    }
    break;

    case ELM_FILE_ATTR_MAXEXTENTS_ELEM :
    if (isMaxExtentSpec_)
    {
      // Duplicate MAXEXTENTS phrases.
      *SqlParser_Diags << DgSqlCode(-3062);
    }
    isMaxExtentSpec_ = TRUE;
    ComASSERT(pPartitionAttr->castToElemDDLFileAttrMaxExtents() NEQ NULL);
    {
      ElemDDLFileAttrMaxExtents * pMaxExtents =
        pPartitionAttr->castToElemDDLFileAttrMaxExtents();
	  // error checking for limits when we specify the MAXEXTENTS clause
#pragma nowarn(1506)   // warning elimination 
	  Lng32 maxext = pMaxExtents->getMaxExtents();
#pragma warn(1506)  // warning elimination 
	  if ((maxext <= 0) || (maxext > COM_MAX_MAXEXTENTS))
      {
		*SqlParser_Diags << DgSqlCode(-3191);
      }
	  else
	  {
		maxExt_            = pMaxExtents->getMaxExtents();
	  }
    }
	break;
  default :
    ABORT("internal logic error");
    break;
  }
}  // ElemDDLPartitionSystem::setPartitionAttr()

//
// method for binding
//

ExprNode *
ElemDDLPartitionSystem::bindNode(BindWA * /*pBindWA*/)
{
  //
  // location
  //
  ComLocationName defaultLocName;  // empty object

  if (getLocationName().isNull())
    //
    // location clause not specified (only allowed for primary partition)
    //
    guardianLocation_ =
      defaultLocName.getGuardianFullyQualifiedName();
  else  // LOCATION clause was specified
  {
    ComLocationName locName;         // empty object
    switch (getLocationNameType())
    {
    case ElemDDLLocation::LOCATION_GUARDIAN_NAME :
      locName.copy(getLocationName(),
                   ComLocationName::GUARDIAN_LOCATION_NAME_FORMAT);
      if (NOT locName.isValid())
      {
        // Illegal location name format.
        *SqlParser_Diags << DgSqlCode(-3061) << DgString0(getLocationName());
        guardianLocation_ =
          defaultLocName.getGuardianFullyQualifiedName();
      }
      else  // valid location
        guardianLocation_ =
          locName.getGuardianFullyQualifiedName();
      break;

    case ElemDDLLocation::LOCATION_OSS_NAME :
      locName.copy(getLocationName(),
                   ComLocationName::OSS_LOCATION_NAME_FORMAT);
      if (NOT locName.isValid())
      {
        // Illegal location name format.
        *SqlParser_Diags << DgSqlCode(-3061) << DgString0(getLocationName());
        guardianLocation_ =
          defaultLocName.getGuardianFullyQualifiedName();
      }
      else  // valid location
        guardianLocation_ =
          locName.getGuardianFullyQualifiedName();
      break;

#if 0
      //
      // Currently we do not handle this case
      // The grammar productions don't accept this syntax.
      // So comment out the following code for now.
      //
    case ElemDDLLocation::LOCATION_ENVIRONMENT_VARIABLE :
      {
        NAString envVarName(getLocationName());
        //
        // if the specified OSS environment variable has the
        // dollar sign prefix, removes it.
        //
        if (envVarName[(size_t) 0] EQU '$')
        {
          envVarName = envVarName(1/*startpos*/, envVarName.length() - 1);
        }
        const char * pEnvVarValue = getenv((const char *)envVarName);
        NAString locationName;
        if (pEnvVarValue NEQ NULL)
        {
          locationName = pEnvVarValue;
        }
        if (locationName.isNull())
        {
          guardianLocation_ = defaultLocName.
                              getGuardianFullyQualifiedName();
        }
        else
        {
          guardianLocationName = locationName;
          if (guardianLocationName.isValid())
          {
            guardianLocation_ = (guardianLocationName.
                                 getGuardianFullyQualifiedName());
            
          }
          else
          {
            ossLocationName = locationName;
            if (ossLocationName.isValid())
            {
              guardianLocation_ = (ossLocationName.
                                   getGuardianFullyQualifiedName());
            }
            else
            {
              // OSS environment variable $1~string1 contains illegal
              // location name $0~string0.
              *SqlParser_Diags << DgSqlCode(-3061) 
		<< DgString0(locationName)
	        << DgString1(envVarName)
		;
            }
          }
        }
      }
      break;
#endif // 0

    default :
      NAAbort("ElemDDLPartition.C", __LINE__, "internal logic error");
      break;
    }
  }

  markAsBound();
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLPartitionSystem::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;

  detailTextList.append(displayLabel1());  // add or drop
  detailTextList.append(displayLabel2());  // location name
  detailTextList.append(displayLabel3());  // location name type

  //
  // file attributes for this partition
  //

  detailTextList.append("File attributes:");

  detailText = "    max size spec? ";
  detailText += YesNo(isMaxSizeSpecified());
  detailTextList.append(detailText);

  detailText = "    maxsizunbound? ";
  detailText += YesNo(isMaxSizeUnbounded());
  detailTextList.append(detailText);

  detailText = "    max size:      ";
  detailText += LongToNAString((Lng32)getMaxSize());
  detailTextList.append(detailText);

#pragma nowarn(1506)   // warning elimination 
  ElemDDLFileAttrMaxSize maxSizeFileAttr(getMaxSize(), 
                                         getMaxSizeUnit());
#pragma warn(1506)  // warning elimination

  detailText = "    max size unit: ";
  detailText += maxSizeFileAttr.getMaxSizeUnitAsNAString();;
  detailTextList.append(detailText);

  return detailTextList;
}

const NAString
ElemDDLPartitionSystem::getText() const
{
  return "ElemDDLPartitionSystem";
}

const NAString
ElemDDLPartitionSystem::displayLabel1() const
{
  return NAString("Add or Drop: ") + getOptionAsNAString();
}

const NAString
ElemDDLPartitionSystem::displayLabel2() const
{
  if (NOT getLocationName().isNull())
  {
    return NAString("Location name: ") + getLocationName();
  }
  else
  {
    return NAString("Location name not specified.");
  }
}

const NAString
ElemDDLPartitionSystem::displayLabel3() const
{
  if (NOT getLocationName().isNull())
  {
    return NAString("Partition name: ") + getPartitionName();
  }
  else
  {
    return NAString("Partition name not specified.");
  }
}


// method for building text
// virtual 
NAString ElemDDLPartitionSystem::getSyntax() const
{
  ElemDDLPartitionSystem *ncThis = (ElemDDLPartitionSystem*)this;
  
  
  NAString syntax = getOptionAsNAString();
  syntax += " ";
  

  syntax += getLocationNode()->getSyntax();
  syntax += " ";


  if (NULL != ncThis->getChild(INDEX_PARTITION_ATTR_LIST))
  {
    ElemDDLNode * pPartitionAttrList = 
	    ncThis->getChild(INDEX_PARTITION_ATTR_LIST)->castToElemDDLNode();

    for (CollIndex i = 0; i < pPartitionAttrList->entries(); i++)
    {
      syntax += (*pPartitionAttrList)[i]->getSyntax();
      syntax += " ";
    }
  }
  return syntax;
}


// -----------------------------------------------------------------------
// methods for class ElemDDLPartitionSingle
// -----------------------------------------------------------------------

//
// default constructor
//

ElemDDLPartitionSingle::ElemDDLPartitionSingle()
: ElemDDLPartitionSystem(ELM_PARTITION_SINGLE_ELEM,
                         ADD_OPTION,
                         NULL /*location*/,
                         NULL /*partition option list*/)
{
}

// virtual destructor
ElemDDLPartitionSingle::~ElemDDLPartitionSingle()
{
  //
  // Does not delete any child parse node.
  // The destructor of the base class ElemDDLPartitionSystem
  // does the deletion(s).
  //
}

// cast
ElemDDLPartitionSingle *
ElemDDLPartitionSingle::castToElemDDLPartitionSingle()
{
  return this;
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPartitionRange
// -----------------------------------------------------------------------

//
// constructors
//

ElemDDLPartitionRange::ElemDDLPartitionRange(CollHeap * heap)
: ElemDDLPartitionSystem(ELM_PARTITION_RANGE_ELEM,
                         ADD_OPTION,
                         NULL /*location*/,
                         NULL /*partition option list*/),
  keyValueArray_(heap)
{
  setChild(INDEX_KEY_VALUE_LIST, NULL);
}

ElemDDLPartitionRange::ElemDDLPartitionRange(
     ElemDDLPartition::optionEnum option,
     ElemDDLNode * pKeyValueList,
     ElemDDLNode * pLocation,
     ElemDDLNode * pPartitionOptionList,
     CollHeap    * heap)
: ElemDDLPartitionSystem(ELM_PARTITION_RANGE_ELEM,
                         option,
                         pLocation,
                         pPartitionOptionList),
  keyValueArray_(heap)
{

  setChild(INDEX_KEY_VALUE_LIST, pKeyValueList);

  // Traverses the parse sub-tree containing the list of key values.
  // Copies pointers to these key values to keyValueArray_ for
  // easier access.

  for (CollIndex i = 0; i < pKeyValueList->entries(); i++)
  {
    ComASSERT((*pKeyValueList)[i]->castToElemDDLKeyValue() NEQ NULL);
    keyValueArray_.insert((*pKeyValueList)[i]->castToElemDDLKeyValue()
                          ->getKeyValue());
  }

}

// virtual destructor
ElemDDLPartitionRange::~ElemDDLPartitionRange()
{
  //
  // Only deletes the child parse node(s) added specifically
  // for this class.  The destructor of the base class
  // ElemDDLPartitionSystem does the deletion(s) of the other
  // child parse node(s).
  //
  for (Int32 i = ElemDDLPartitionSystem::getArity(); i < getArity(); i++)
  {
    delete getChild(i);
  }
}

// cast
ElemDDLPartitionRange *
ElemDDLPartitionRange::castToElemDDLPartitionRange()
{
  return this;
}

//
// accessors
//

// get the degree of this node
Int32
ElemDDLPartitionRange::getArity() const
{
  return MAX_ELEM_DDL_PARTITION_RANGE_ARITY;
}

ExprNode *
ElemDDLPartitionRange::getChild(Lng32 index)
{ 
  ComASSERT(index >= 0 AND index < getArity());

  //
  // Note that class ElemDDLPartitionRange is
  // class from class ElemDDLPartitionSystem.
  // For more information, please read the
  // descriptions in the head file.
  //
  if (index >= ElemDDLPartitionSystem::getArity())
  {
    ComASSERT(index EQU INDEX_KEY_VALUE_LIST);
    return pKeyValueList_;
  }
  else
  {
    return children_[index];
  }
}

//
// mutator
//

void
ElemDDLPartitionRange::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());

  ElemDDLNode * pElemDDLNode = NULL;
  if (pChildNode NEQ NULL)
  {
    ComASSERT(pChildNode->castToElemDDLNode() NEQ NULL);
    pElemDDLNode = pChildNode->castToElemDDLNode();
  }

  //
  // Note that class ElemDDLPartitionRange is
  // class from class ElemDDLPartitionSystem.
  // For more information, please read the
  // descriptions in the head file.
  //
  if (index >= ElemDDLPartitionSystem::getArity())
  {
    pKeyValueList_ = pElemDDLNode;
  }
  else
  {
    children_[index] = pElemDDLNode;
  }
}

//
// methods for tracing
//

NATraceList
ElemDDLPartitionRange::getDetailInfo() const
{
  //
  // Note that class ElemDDLPartitionRange is derived
  // from class ElemDDLPartitionSystem.
  //
  NAString        detailText;
  NATraceList detailTextList = ElemDDLPartitionSystem::getDetailInfo();

  const ItemConstValueArray & keyValues = getKeyValueArray();

  if (keyValues.entries() NEQ 0)
  {
    detailText = "Key value list [";
    detailText += LongToNAString((Lng32)keyValues.entries());
    detailText += " key value(s)]:";
    detailTextList.append(detailText);
  }
  else
  {
    //
    // only primary (range) partition node is
    // allowed not to contain a list of key values.
    //
    detailText = "Key value not specified.";
    detailTextList.append(detailText);
  }
  for (CollIndex j = 0; j < keyValues.entries(); j++)
  {
    ConstValue * keyVal = keyValues[j];
    
    detailText = "  [key value ";
    detailText += LongToNAString((Lng32)j);
    detailText += "]";
    detailTextList.append(detailText);
    
    detailText = "    Key value:      ";
    detailText += keyVal->getText();
    detailTextList.append(detailText);
    
    detailText = "    Key value type: ";
    detailText += keyVal->getType()->getTypeSQLname();
    detailTextList.append(detailText);
  }

  return detailTextList;

} // ElemDDLPartitionRange::getDetailInfo()

const NAString
ElemDDLPartitionRange::getText() const
{
  return "ElemDDLPartitionRange";
}

const NAString
ElemDDLPartitionRange::displayLabel1() const
{
  if (getLocationName().length() NEQ 0)
    return NAString("Location name: ") + getLocationName();
  else
    return NAString("Location name not specified.");
}

const NAString
ElemDDLPartitionRange::displayLabel2() const
{
  ElemDDLLocation location(getLocationNameType(), getLocationName());
  return (NAString("Location name type: ") +
          location.getLocationNameTypeAsNAString());
}


// method for building text
// virtual 
NAString ElemDDLPartitionRange::getSyntax() const
{
  ElemDDLPartitionRange* ncThis = (ElemDDLPartitionRange*)this;
  
  
  NAString syntax = getOptionAsNAString();
  syntax += " ";
  
  syntax += "FIRST KEY ";

  ElemDDLNode * pKeyValueList = 
		 ncThis->getChild(INDEX_KEY_VALUE_LIST)->castToElemDDLNode();
  
  syntax += "( ";

  syntax += pKeyValueList->getSyntax();

  syntax += ") ";

  syntax += getLocationNode()->getSyntax();
  syntax += " ";


  if (NULL != ncThis->getChild(INDEX_PARTITION_ATTR_LIST))
  {
    ElemDDLNode * pPartitionAttrList = 
	    ncThis->getChild(INDEX_PARTITION_ATTR_LIST)->castToElemDDLNode();

    for (CollIndex i = 0; i < pPartitionAttrList->entries(); i++)
    {
      syntax += (*pPartitionAttrList)[i]->getSyntax();
      syntax += " ";
    }
  }
  return syntax;
}


// -----------------------------------------------------------------------
// methods for class ElemDDLPartitionByOpt
// -----------------------------------------------------------------------

// constructor
ElemDDLPartitionByOpt::ElemDDLPartitionByOpt(OperatorTypeEnum operatorType)
: ElemDDLNode(operatorType)
{
}

// virtual destructor
ElemDDLPartitionByOpt::~ElemDDLPartitionByOpt()
{
}

// casting
ElemDDLPartitionByOpt *
ElemDDLPartitionByOpt::castToElemDDLPartitionByOpt()
{
  return this;
}

//
// methods for tracing
//

NATraceList
ElemDDLPartitionByOpt::getDetailInfo() const
{
  NATraceList detailTextList;

  //
  // Note that the invoked displayLabel1() is a method of
  // a class derived from class ElemDDLPartitionByOpt
  //
  detailTextList.append(displayLabel1());

  return detailTextList;
}

const NAString
ElemDDLPartitionByOpt::getText() const
{
  ABORT("internal logic error");
  return "ElemDDLPartitionByOpt";
}


//----------------------------------------------------------------------------
// methods for class ElemDDLPartitionByColumnList
//----------------------------------------------------------------------------

//
// constructor
//

ElemDDLPartitionByColumnList::ElemDDLPartitionByColumnList(
     ElemDDLNode * partitionKeyColumnList,
     CollHeap    * heap)
: ElemDDLPartitionByOpt(ELM_PARTITION_BY_COLUMN_LIST_ELEM),
  partitionKeyColumnArray_(heap)
{
  setChild(INDEX_PARTITION_KEY_COLUMN_LIST, partitionKeyColumnList);

  //
  // copies pointers to parse nodes representing
  // column names (appearing in a partition by option)
  // to partitionKeyColumnArray_ so the user can access
  // this information easier.
  //

  ComASSERT(partitionKeyColumnList NEQ NULL);
  for (CollIndex i = 0; i < partitionKeyColumnList->entries(); i++)
  {
    partitionKeyColumnArray_.insert((*partitionKeyColumnList)[i]->
						castToElemDDLColRef());
  }
}

// virtual destructor
ElemDDLPartitionByColumnList::~ElemDDLPartitionByColumnList()
{
}

// casting
ElemDDLPartitionByColumnList *
ElemDDLPartitionByColumnList::castToElemDDLPartitionByColumnList()
{
  return this;
}

//
// accessors
//

// get the degree of this node
Int32
ElemDDLPartitionByColumnList::getArity() const
{
  return MAX_ELEM_DDL_PARTITION_BY_COLUMN_LIST_ARITY;
}

ExprNode *
ElemDDLPartitionByColumnList::getChild(Lng32 index)
{ 
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

//
// mutator
//

void
ElemDDLPartitionByColumnList::setChild(Lng32 index, ExprNode * pChildNode)
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
// methods for tracing
//

const NAString
ElemDDLPartitionByColumnList::displayLabel1() const
{
  return NAString("Partition by column list option");
}

NATraceList
ElemDDLPartitionByColumnList::getDetailInfo() const
{
  NAString        detailText;
  NATraceList detailTextList;
  ElemDDLNode   * pPartitionKeyColumnList = getPartitionKeyColumnList();

  //
  // kind of store option
  //

  detailTextList.append(displayLabel1());

  //
  // column name list
  //

  if (pPartitionKeyColumnList EQU NULL)
  {
    detailTextList.append("No column name list.");
    return detailTextList;
  }

  detailText = "Column Name List [";
  detailText += LongToNAString((Lng32)pPartitionKeyColumnList->entries());
  detailText += " element(s)]:";
  detailTextList.append(detailText);

  for (CollIndex i = 0; i < pPartitionKeyColumnList->entries(); i++)
  {
    detailText = "[column ";
    detailText += LongToNAString((Lng32)i);
    detailText += "]";
    detailTextList.append(detailText);

    detailTextList.append("    ", (*pPartitionKeyColumnList)[i]
                                                         ->getDetailInfo());
  }
  return detailTextList;
}

const NAString
ElemDDLPartitionByColumnList::getText() const
{
  return "ElemDDLPartitionByColumnList";
}


// method for building text
//virtual 
NAString ElemDDLPartitionByColumnList::getSyntax() const
{
  
  const ElemDDLColRefArray & colRefArray = getPartitionKeyColumnArray();

  NAString syntax = (colRefArray[0])->getColumnName();
  
  
//ElemDDLColRef * pGroupByColumnRef =    
    
  // if you didn't notice - we start with i=1  
  for (CollIndex i = 1; i < colRefArray.entries(); i++)
  {
    syntax += ", ";
    syntax += (colRefArray[i])->getColumnName();
    
  }
  return syntax;

} // getSyntax



//
// End of File
//
