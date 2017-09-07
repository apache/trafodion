// **********************************************************************
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
// **********************************************************************


#include "QRDescriptor.h"
#include "QRSharedPtr.h"
#include "Range.h"
// Element names
const char QRQueryDescriptor::elemName[] = "Query";
const char QRMVDescriptor::elemName[] = "MV";
const char QRResultDescriptor::elemName[] = "Result";
const char QRPublishDescriptor::elemName[] = "Publish";
const char QRQueryMisc::elemName[] = "Misc";
const char QRJBB::elemName[] = "JBB";
const char QRInfo::elemName[] = "Info";
const char QRHub::elemName[] = "Hub";
const char QRJBBCList::elemName[] = "JBBCList";
const char QRTable::elemName[] = "Table";
const char QRForcedMVs::elemName[] = "ForcedMVs";
template<> const char QRList<QRTable>::elemName[] = "TableList";
const char QROperator::elemName[] = "Operator";
const char QRJoinPred::elemName[] = "JoinPred";
template<> const char QRList<QRJoinPred>::elemName[] = "JoinPredList";
const char QRRangePred::elemName[] = "Range";
template<> const char QRList<QRRangePred>::elemName[] = "RangePredList";
template<> const char QRList<QRExpr>::elemName[] = "ResidualPredList";
const char QROpEQ::elemName[] = "OpEQ";
const char QROpLS::elemName[] = "OpLS";
const char QROpLE::elemName[] = "OpLE";
const char QROpGT::elemName[] = "OpGT";
const char QROpGE::elemName[] = "OpGE";
const char QROpBT::elemName[] = "OpBT";
const char QRNumericVal::elemName[] = "NumericVal";
const char QRStringVal::elemName[] = "StringVal";
const char QRWStringVal::elemName[] = "WStringVal";
const char QRFloatVal::elemName[] = "FloatVal";
const char QRNullVal::elemName[] = "NullVal";
const char QRGroupBy::elemName[] = "GroupBy";
const char QROutput::elemName[] = "Output";
template<> const char QRList<QROutput>::elemName[] = "OutputList";
const char QRExtraHub::elemName[] = "ExtraHub";
const char QRColumn::elemName[] = "Column";
const char QRExpr::elemName[] = "Expr";
const char QRExpr::residElemName[] = "Residual";  // 2 elem names map to QRExpr
const char QRBinaryOper::elemName[] = "BinaryOper";
const char QRUnaryOper::elemName[] = "UnaryOper";
const char QRFunction::elemName[] = "Function";
const char QRParameter::elemName[] = "Parameter";
const char QRMVColumn::elemName[] = "MVColumn";
const char QRMVMisc::elemName[] = "Misc";
const char QRJbbResult::elemName[] = "JbbResult";
const char QRJbbSubset::elemName[] = "JbbSubset";
const char QRCandidate::elemName[] = "Candidate";
template<> const char QRList<QRCandidate>::elemName[] = "CandidateList";
const char QRMVName::elemName[] = "MVName";
const char QRVersion::elemName[] = "Version";
const char QRUpdate::elemName[] = "Update";
const char QRInclude::elemName[] = "Include";
const char QRPrimaryGroupBy::elemName[] = "Primary";
const char QRDependentGroupBy::elemName[] = "Dependent";
const char QRMinimalGroupBy::elemName[] = "Minimal";
const char QRKey::elemName[] = "Key";

////////////////////////////////////////////////////
//           Member function definitions
////////////////////////////////////////////////////

//
// AggregateFinderVisitor
//

Visitor::VisitResult AggregateFinderVisitor::visit(QRElementPtr caller)
{
  if (caller->getElementType() == ET_Function &&
      caller->downCastToQRFunction()->isAnAggregate())
    {
      foundAggregate_ = TRUE;
      if (findAll_) 
        {
          aggregatesFound_.insert(caller->getReferencedElement());
          return VR_Continue;
        }
      return VR_Stop;
    }

  return VR_Continue;
}
 
Visitor::VisitResult ElementFinderVisitor::visit(QRElementPtr caller)
{
  ElementType callerElemType = caller->getElementType();
  for (CollIndex i=0; i<targetTypes_.entries(); i++)
    {
      if (callerElemType == targetTypes_[i])
        {
          elementsFound_.insert(useRefedElem_ ? caller->getReferencedElement()
                                              : caller);
          return VR_Continue;
        }
    }

  return VR_Continue;
}
 

//
// QRElement
//

const char* const QRElement::ExprResultNames[] = { "Outside", "Provided", "NotProvided" };

QRElement::ExprResult QRElement::encodeResult(const char* resultString)
{
  for (Int32 i=FIRST_EXPR_RESULT; i<INVALID_EXPR_RESULT; i++)
    {
      if (!strcmp(resultString, ExprResultNames[i]))
        return (ExprResult)i;
    }
  throw QRDescriptorException("Invalid value for 'result' attribute -- %s",  // LCOV_EXCL_LINE :rfi
                              resultString);
}

Int32 QRElement::cmpQRElement(const void *p1, const void *p2)
{
  QRElementPtr t1 = *((QRElementPtr*)p1);
  QRElementPtr t2 = *((QRElementPtr*)p2);
  return t1->getReferencedElement()->getSortName().compareTo(t2->getReferencedElement()->getSortName());
}

NAString& QRElement::addEntityRefs(NAString& str, char attrDelim)
{
  const char* specialChars;
  if (attrDelim == '"')
    specialChars = "&<\"";
  else if (attrDelim == '\'')
    specialChars = "&<'";
  else
    assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_ERROR,
                      FALSE, QRDescriptorException,
                      "addEntityRefs: attribute delimiter must be ' or \"");

  const char* data = str.data();
  if (!strpbrk(data, specialChars))
    return str;

  Int32 inx = str.length() - 1;
  while (inx >= 0)
    {
      switch (data[inx])
        {
          case '&':
            str.replace(inx, 1, "&amp;");
            break;
          case '<':
            str.replace(inx, 1, "&lt;");
            break;
          case '"':
            if (attrDelim == '"')
              str.replace(inx, 1, "&quot;");
            break;
          case '\'':
            if (attrDelim == '\'')
              str.replace(inx, 1, "&apos;");
            break;
          default:
            break;
        }
      inx--;
    }
  return str;
}

void QRElement::serializeAttrs(XMLString& xml)
{
  if (id_.length() > 0)
    xml.append("id='").append(id_).append("' ");
  if (ref_.length() > 0)
    xml.append("ref='").append(ref_).append("' ");
}

void QRElement::serializeBoolAttr(const char* attrName,
                                  NABoolean attrVal,
                                  XMLString& xml)
{
  xml.append(attrName)
     .append("='")
     .append(attrVal ? '1' : '0')
     .append("' ");
}

void QRElement::deserializeBoolAttr(const char* attrName,
                                    const char* attrVal,
                                    NABoolean& attr)
{
  if (!strcmp(attrVal, "0"))
    attr = FALSE;
  else if (!strcmp(attrVal, "1"))
    attr = TRUE;
  else
    throw QRDescriptorException("Value of %s attribute must be either 0 or 1",  // LCOV_EXCL_LINE :rfi
                                attrName);
}


//
// QRElementMapper
//

XMLElementPtr QRElementMapper::operator()(void *parser,
                                          char *elementName,
                                          AttributeList atts)
{
  XMLElementPtr elemPtr = NULL;

  if (!strcmp(elementName, QRQueryDescriptor::elemName))
    elemPtr = new (XMLPARSEHEAP) QRQueryDescriptor(atts, ADD_MEMCHECK_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRMVDescriptor::elemName))
    elemPtr = new (XMLPARSEHEAP) QRMVDescriptor(atts, ADD_MEMCHECK_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRResultDescriptor::elemName))
    elemPtr = new (XMLPARSEHEAP) QRResultDescriptor(atts, ADD_MEMCHECK_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRPublishDescriptor::elemName))
    elemPtr = new (XMLPARSEHEAP) QRPublishDescriptor(atts, ADD_MEMCHECK_ARGS(XMLPARSEHEAP));
      
  return elemPtr;
}

//
// QRElementList
//

QRElementList::QRElementList(ElementType type, XMLElementPtr parent, AttributeList atts,
                             ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(type, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    list_(heap)
{
}

QRElementList::~QRElementList()
{
  for (CollIndex i = 0; i < list_.entries(); i++) 
    deletePtr(list_[i]);
}

void QRElementList::serializeBody(XMLString& xml)
{
  for (CollIndex i = 0; i < list_.entries(); i++)
    list_[i]->toXML(xml);
}

NABoolean QRElementList::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  for (CollIndex i = 0; i < list_.entries(); i++)
    if (list_[i]->treeWalk(visitor))
      return TRUE;

  return FALSE;
}

void QRElementList::startItemExprElement(void *parser, const char *elementName, const char **atts)
{
  QRElementPtr elem = NULL;

  if (!strcmp(elementName, QRColumn::elemName))
    elem = new (XMLPARSEHEAP) QRColumn(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRMVColumn::elemName))
    elem = new (XMLPARSEHEAP) QRMVColumn(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRExpr::elemName))
    elem = new (XMLPARSEHEAP) QRExpr(this, atts, FALSE, ADD_MEMCHECK_ARGS(XMLPARSEHEAP));
  else
    throw QRDescriptorException("<%s> cannot contain <%s>",  // LCOV_EXCL_LINE :rfi
                                getElementName(), elementName);

  addElement(elem);
  XMLDocument::setCurrentElement(parser, elem);
}


//
// QRDescriptor
//

NABoolean QRDescriptor::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  for (CollIndex i=0; i<jbbList_.entries(); i++)
    if (jbbList_[i]->treeWalk(visitor))
      return TRUE;

  return FALSE;
}

//
// QRQueryMisc
//

QRQueryMisc::QRQueryMisc(ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_QueryMisc, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
    userID_(heap),
    mvAge_(heap),
    optLevel_(heap),
    rewriteLevel_(MRL_OFF),
    forcedMVs_(NULL)
{
}

QRQueryMisc::QRQueryMisc(XMLElementPtr parent, AttributeList atts,
                         ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_QueryMisc, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    userID_(heap),
    mvAge_(heap),
    optLevel_(heap),
    rewriteLevel_(MRL_OFF),
    forcedMVs_(NULL)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  const char *attrValue;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      attrValue = iter.getValue();
      if (!strcmp(attrName, "userID"))
        userID_ = attrValue;
      else if (!strcmp(attrName, "MVAge"))
        mvAge_ = attrValue;
      else if (!strcmp(attrName, "optLevel"))
        optLevel_ = attrValue;
      else if (!strcmp(attrName, "rewriteLevel"))
        rewriteLevel_ = (MvqrRewriteLevel)atoi(attrValue);
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",
                                    elemName, attrName);
    }
}

QRQueryMisc::~QRQueryMisc()
{
  deletePtr(forcedMVs_);
}

NABoolean QRQueryMisc::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (forcedMVs_ && forcedMVs_->treeWalk(visitor))
    return TRUE;

  return FALSE;
}

void QRQueryMisc::startElement(void *parser, const char *elementName, const char **atts)
{
  // LCOV_EXCL_START :cnu
  if (!strcmp(elementName, QRForcedMVs::elemName))
    {
      forcedMVs_ = new (XMLPARSEHEAP) QRForcedMVs(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, forcedMVs_);
    }
  // LCOV_EXCL_STOP
  else
    {
      throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                  elemName, elementName);
    }
}

void QRQueryMisc::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  if (userID_.length() > 0)
    xml.append("userID='").append(userID_).append("\' ");
  if (mvAge_.length() > 0)
    xml.append("MVAge='").append(mvAge_).append("\' ");
  if (optLevel_.length() > 0)
    xml.append("optLevel='").append(optLevel_).append("\' ");
  if (rewriteLevel_ != MRL_OFF)
  {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "rewriteLevel='%d' ", (Int32)rewriteLevel_);
    xml.append(buffer);
  }
}

void QRQueryMisc::serializeBody(XMLString& xml)
{
  if (forcedMVs_)
    forcedMVs_->toXML(xml);
}


//
// QRQueryDescriptor
//

QRQueryDescriptor::QRQueryDescriptor(AttributeList atts,
                                     ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRDescriptor(ET_QueryDescriptor, ADD_MEMCHECK_ARGS_PASS(heap)),
    version_(NULL),
    misc_(NULL),
    options_(heap)
{
  // Set parent here so we don't have to use 'this' in initializer.
  setParent(this);

  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      if (!strcmp(attrName, "options"))
        options_ = iter.getValue();
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",
                                    elemName, attrName);
    }
}

QRQueryDescriptor::~QRQueryDescriptor()
{
  deletePtr(version_);
  deletePtr(misc_);
  for (CollIndex i = 0; i < jbbList_.entries(); i++) 
    deletePtr(jbbList_[i]);
}

NABoolean QRQueryDescriptor::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (version_ && version_->treeWalk(visitor))
    return TRUE;
  if (misc_ && misc_->treeWalk(visitor))
    return TRUE;
  for (CollIndex i = 0; i < jbbList_.entries(); i++) 
    if (jbbList_[i]->treeWalk(visitor))
      return TRUE;

  return FALSE;
}

void QRQueryDescriptor::startElement(void *parser, const char *elementName, const char **atts)
{
  if (!strcmp(elementName, QRVersion::elemName))
    {
      version_ = new (XMLPARSEHEAP) QRVersion(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, version_);
    }
  else if (!strcmp(elementName, QRQueryMisc::elemName))
    {
      misc_ = new (XMLPARSEHEAP) QRQueryMisc(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, misc_);
    }
  else if (!strcmp(elementName, QRJBB::elemName))
    {
      QRJBBPtr jbb = new (XMLPARSEHEAP) QRJBB(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      addJBB(jbb);
      XMLDocument::setCurrentElement(parser, jbb);
    }
  else
    {
      throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                  elemName, elementName);
    }
}

void QRQueryDescriptor::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  if (options_.length() > 0)
    xml.append("options='").append(options_).append("\' ");
}

void QRQueryDescriptor::serializeBody(XMLString& xml)
{
  if (version_)
    version_->toXML(xml);
  if (misc_)
    misc_->toXML(xml);
  for (CollIndex i = 0; i < jbbList_.entries(); i++)
    jbbList_[i]->toXML(xml);
}


//
// QRJBB
//

QRJBB::QRJBB(NumericID idNum,
             JBB* jbb,
             ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_JBB, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
    hub_(new(heap) QRHub(ADD_MEMCHECK_ARGS(heap))),
    extraHub_(new(heap) QRExtraHub(ADD_MEMCHECK_ARGS(heap))),
    outputList_(new (heap) QRList<QROutput>(ADD_MEMCHECK_ARGS(heap))),
    groupBy_(NULL),
    caNodeId_(0),
    actualJbbPtr_(jbb),
    tableArray_(NULL),
    tableCount_(0),
    maxTableEntries_(0),
    gbExpr_(NULL)
{
  setID(idNum);
}

QRJBB::QRJBB(NumericID idNum,
             CollIndex nodeId,
             ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_JBB, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
    hub_(new(heap) QRHub(ADD_MEMCHECK_ARGS(heap))),
    extraHub_(new(heap) QRExtraHub(ADD_MEMCHECK_ARGS(heap))),
    outputList_(new (heap) QRList<QROutput>(ADD_MEMCHECK_ARGS(heap))),
    groupBy_(NULL),
    caNodeId_(nodeId),
    actualJbbPtr_(NULL),
    tableArray_(NULL),
    tableCount_(0),
    maxTableEntries_(0),
    gbExpr_(NULL)
{
  setID(idNum);
}

QRJBB::QRJBB(XMLElementPtr parent, AttributeList atts,
             ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_JBB, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    hub_(NULL),
    extraHub_(NULL),
    outputList_(NULL),
    groupBy_(NULL),
    tableArray_(NULL),
    tableCount_(0),
    maxTableEntries_(0),
    gbExpr_(NULL)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      if (!strcmp(attrName, "id"))
        id_ = iter.getValue();
      else if (!strcmp(attrName, "ref"))
        ref_ = iter.getValue();
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    elemName, attrName);
    }
}

QRJBB::~QRJBB()
{
  deletePtr(hub_);
  deletePtr(extraHub_);
  deletePtr(outputList_);
  deletePtr(groupBy_);
}

NABoolean QRJBB::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (hub_ && hub_->treeWalk(visitor))
    return TRUE;
  if (extraHub_ && extraHub_->treeWalk(visitor))
    return TRUE;
  if (outputList_ && outputList_->treeWalk(visitor))
    return TRUE;
  if (groupBy_ && groupBy_->treeWalk(visitor))
    return TRUE;

  return FALSE;
}

void QRJBB::serializeBody(XMLString& xml)
{
  // A JBB that is merely a link to another JBB has no body.
  if (ref_.length() > 0)
    return;

  if (hub_)
    hub_->toXML(xml);
  if (extraHub_)
    extraHub_->toXML(xml);
  if (outputList_ && !outputList_->isEmpty())
    outputList_->toXML(xml);
  if (groupBy_)
    groupBy_->toXML(xml);
}

void QRJBB::startElement(void *parser, const char *elementName, const char **atts)
{
  if (!strcmp(elementName, QRHub::elemName))
    {
      hub_ = new (XMLPARSEHEAP) QRHub(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, hub_);
    }
  else if (!strcmp(elementName, QRExtraHub::elemName))
    {
      extraHub_ = new (XMLPARSEHEAP) QRExtraHub(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, extraHub_);
    }
  else if (!strcmp(elementName, QRList<QROutput>::elemName))
    {
      outputList_ = new (XMLPARSEHEAP) QRList<QROutput>(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, outputList_);
    }
  else if (!strcmp(elementName, QRGroupBy::elemName))
    {
      groupBy_ = new (XMLPARSEHEAP) QRGroupBy(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, groupBy_);
    }
  else
    {
      throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                  elemName, elementName);
    }
}

// LCOV_EXCL_START :cnu
void QRJBB::createTableArray(CollIndex maxEntries)
{
  assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                     !tableArray_, QRDescriptorException,
		     "tableArray_ already created for jbb %d", getIDNum());
  maxTableEntries_ = maxEntries;
  tableArray_ = new QRTablePtr[maxEntries];
}
// LCOV_EXCL_STOP

void QRJBB::addTable(QRTablePtr table)
{
  assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                     tableCount_ < maxTableEntries_, QRDescriptorException,
		     "Found more tables than in jbbc list for jbb %d", getIDNum());
  tableArray_[tableCount_++] = table;
}


//
// QRList
//

template <class T>
void QRList<T>::serializeBody(XMLString& xml)
{
  for (CollIndex i = 0; i < list_.entries(); i++)
    list_[i]->toXML(xml);
}

template <class T>
NABoolean QRList<T>::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  for (CollIndex i = 0; i < list_.entries(); i++) 
    if (list_[i]->treeWalk(visitor))
      return TRUE;

  return FALSE;
}

template <class T>
void QRList<T>::startElement(void *parser, const char *elementName, const char **atts)
{
  if (!strcmp(elementName, T::elemName))
    {
      PTR_TO_TYPE(T) listElem = new (XMLPARSEHEAP) T(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      list_.insert(listElem);
      XMLDocument::setCurrentElement(parser, listElem);
    }
  else
    {
      throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                  elemName, elementName);
    }
}

// This template specialization is necessary because QRExpr is associated with two
// different elements, and when used as a list, is a list of the secondary one
// (<Residual>). So in this specialization, we are looking for residElemName,
// instead of elemName as is the case for all other classes.
template<> void QRList<QRExpr>::startElement(void *parser, const char *elementName, const char **atts)
{
  if (!strcmp(elementName, QRExpr::residElemName))
    {
      PTR_TO_TYPE(QRExpr) listElem = 
              new (XMLPARSEHEAP) QRExpr(this, atts, TRUE, ADD_MEMCHECK_ARGS(XMLPARSEHEAP));
      list_.insert(listElem);
      XMLDocument::setCurrentElement(parser, listElem);
    }
  else
    {
      throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                  elemName, elementName);
    }
}

// For some reason, the generic definition does not work, so I used a specific definition for QRRangePred.
//template<class T> 
//void QRList<T>::addItemOrdered(PTR_TO_TYPE(T) item)
template<> void QRList<QRRangePred>::addItemOrdered(PTR_TO_TYPE(QRRangePred) item)
{
  CollIndex maxEntries = entries();
  if (maxEntries==0)
  {
    list_.insert(item);
  }
  else
  {
    CollIndex pos = 0;
    const NAString& newID = item->getID();
    while (pos<maxEntries && newID > list_[pos]->getID())
      pos++;

    list_.insertAt(pos, item);
  }
}

//
// QRHub
//

QRHub::QRHub(ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_Hub, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
    jbbcList_(new(heap) QRJBBCList(ADD_MEMCHECK_ARGS(heap))),
    joinPredList_(new(heap) QRList<QRJoinPred>(ADD_MEMCHECK_ARGS(heap))),
    rangePredList_(new(heap) QRList<QRRangePred>(ADD_MEMCHECK_ARGS(heap))),
    residualPredList_(new(heap) QRList<QRExpr>(ADD_MEMCHECK_ARGS(heap)))
{}

QRHub::QRHub(XMLElementPtr parent, AttributeList atts,
             ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_Hub, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    jbbcList_(NULL),
    joinPredList_(NULL),
    rangePredList_(NULL),
    residualPredList_(NULL)
{
  if (*atts)
    throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                elemName, *atts);
}

QRHub::~QRHub()
{
  deletePtr(jbbcList_);
  deletePtr(joinPredList_);
  deletePtr(rangePredList_);
  deletePtr(residualPredList_);
}

void QRHub::serializeBody(XMLString& xml)
{
  if (jbbcList_ && !jbbcList_->isEmpty())
    jbbcList_->toXML(xml);
  if (joinPredList_ && !joinPredList_->isEmpty())
    joinPredList_->toXML(xml);
  if (rangePredList_ && !rangePredList_->isEmpty())
    rangePredList_->toXML(xml);
  if (residualPredList_ && !residualPredList_->isEmpty())
    residualPredList_->toXML(xml);
}

NABoolean QRHub::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (jbbcList_ && !jbbcList_->isEmpty() && jbbcList_->treeWalk(visitor))
    return TRUE;
  if (joinPredList_ && !joinPredList_->isEmpty() && joinPredList_->treeWalk(visitor))
    return TRUE;
  if (rangePredList_ && !rangePredList_->isEmpty() && rangePredList_->treeWalk(visitor))
    return TRUE;
  if (residualPredList_ && !residualPredList_->isEmpty() && residualPredList_->treeWalk(visitor))
    return TRUE;

  return FALSE;
}

void QRHub::startElement(void *parser, const char *elementName, const char **atts)
{
  if (!strcmp(elementName, QRJBBCList::elemName))
    {
      jbbcList_ = new (XMLPARSEHEAP) QRJBBCList(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, jbbcList_);
    }
  else if (!strcmp(elementName, QRList<QRJoinPred>::elemName))
    {
      joinPredList_ = new (XMLPARSEHEAP) QRList<QRJoinPred>(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, joinPredList_);
    }
  else if (!strcmp(elementName, QRList<QRRangePred>::elemName))
    {
      rangePredList_ = new (XMLPARSEHEAP) QRList<QRRangePred>(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, rangePredList_);
    }
  else if (!strcmp(elementName, QRList<QRExpr>::elemName))
    {
      residualPredList_ = new (XMLPARSEHEAP) QRList<QRExpr>(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, residualPredList_);
    }
  else
    {
      throw QRDescriptorException("<%s> cannot contain <%s>",  // LCOV_EXCL_LINE :rfi
                                  elemName, elementName);
    }
}

//
// QRJBBCList
//
QRJBBCList::QRJBBCList(XMLElementPtr parent, AttributeList atts,
                       ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElementList(ET_JBBCList, parent, NULL, ADD_MEMCHECK_ARGS_PASS(heap))
{
  if (*atts)
    throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                elemName, *atts);
}

void QRJBBCList::startElement(void *parser, const char *elementName, const char **atts)
{
  QRElementPtr elem;

  if (!strcmp(elementName, QRTable::elemName))
    elem = new (XMLPARSEHEAP) QRTable(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRJBB::elemName))
    elem = new (XMLPARSEHEAP) QRJBB(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QROperator::elemName))
    elem = new (XMLPARSEHEAP) QROperator(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else
    throw QRDescriptorException("<%s> cannot contain <%s>",  // LCOV_EXCL_LINE :rfi
                                elemName, elementName);

  addElement(elem);
  XMLDocument::setCurrentElement(parser, elem);
}

void QRJBBCList::setIsKeyCovered()
{
  const ElementPtrList& elemList = getList();
  for (CollIndex i=0; i<elemList.entries(); i++)
    {
      if (elemList[i]->getElementType() == ET_Table)
        static_cast<QRTablePtr>(elemList[i])->setIsKeyCovered(TRUE);
    }
}

//
// QRTable
//

QRTable::QRTable(ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_Table, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
    redefTimestamp_(heap),
    extraHubReason_(heap),
    isAnMV_(FALSE),
    tableName_(charData_),
    isExtraHub_(FALSE),
    isKeyCovered_(FALSE),
    numCols_(-1),
    rangeBits_(heap),
    residualBits_(heap),
    hasLOJParent_(FALSE),
    key_(NULL),
    correlationName_(heap),
    joinOrder_(1)
{
}

QRTable::QRTable(XMLElementPtr parent, AttributeList atts,
                 ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_Table, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    redefTimestamp_(heap),
    extraHubReason_(heap),
    isAnMV_(false),
    tableName_(charData_),
    isExtraHub_(FALSE),
    isKeyCovered_(FALSE),
    numCols_(-1),
    rangeBits_(heap),
    residualBits_(heap),
    hasLOJParent_(FALSE),
    key_(NULL),
    correlationName_(heap),
    joinOrder_(1)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  const char *attrVal;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      attrVal = iter.getValue();
      if (!strcmp(attrName, "id"))
        id_ = attrVal;
      else if (!strcmp(attrName, "ref"))
        ref_ = attrVal;
      else if (!strcmp(attrName, "TS"))
        redefTimestamp_ = attrVal;  //@ZX -- probably need to convert to INT64
      else if (!strcmp(attrName, "reason"))
        extraHubReason_ = attrVal;
      else if (!strcmp(attrName, "isAnMV"))
        deserializeBoolAttr(attrName, attrVal, isAnMV_);
      else if (!strcmp(attrName, "isKeyCovered"))
        deserializeBoolAttr(attrName, attrVal, isKeyCovered_);
      else if (!strcmp(attrName, "numCols"))
        setNumCols(atoi(attrVal));  // must call fn, so bitmaps are resized
      else if (!strcmp(attrName, "rangeBits"))
        rangeBits_.initFromHexString(attrVal, numCols_);
      else if (!strcmp(attrName, "residualBits"))
        residualBits_.initFromHexString(attrVal, numCols_);
      else if (!strcmp(attrName, "hasLOJParent"))
        deserializeBoolAttr(attrName, attrVal, hasLOJParent_);
      else if (!strcmp(attrName, "corr"))
        correlationName_ = attrVal;
      else if (!strcmp(attrName, "joinOrder"))
        joinOrder_ = atoi(attrVal);
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",
                                    elemName, attrName);
    }
}

void QRTable::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  if (redefTimestamp_.length() > 0)
    xml.append("TS='").append(redefTimestamp_).append("' ");
  if (extraHubReason_.length() > 0)
    xml.append("reason='").append(extraHubReason_).append("' ");
  if (isAnMV_)  // 0 is default
    xml.append("isAnMV='1' ");
  if (isKeyCovered_)  // 0 is default
    xml.append("isKeyCovered='1' ");
  if (numCols_ != -1)
  {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "numCols='%d' ", numCols_);
    xml.append(buffer);
  }
  if (hasLOJParent_)  // 0 is default
    xml.append("hasLOJParent='1' ");
  if (!rangeBits_.isEmpty())
  {
    xml.append("rangeBits='");
    rangeBits_.toXML(xml);
    xml.append("' ");
  }
  if (!residualBits_.isEmpty())
  {
    xml.append("residualBits='");
    residualBits_.toXML(xml);
    xml.append("' ");
  }
  if (correlationName_ != "")
    xml.append("corr='").append(correlationName_).append("' ");    
  if (joinOrder_ > 1)
  {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "joinOrder='%d' ", joinOrder_);
    xml.append(buffer);
  }
}

void QRTable::serializeBody(XMLString& xml)
{
  QRElement::serializeBody(xml);
  if (key_)
    key_->toXML(xml);
}

void QRTable::startElement(void *parser, const char *elementName, const char **atts)
{
  QRElementPtr elem;

  if (!strcmp(elementName, QRKey::elemName))
  {
    QRKeyPtr key = new (XMLPARSEHEAP) QRKey(CHILD_ELEM_ARGS(XMLPARSEHEAP));
    setKey(key);
    elem = key;
  }
  else
    throw QRDescriptorException("<%s> cannot contain <%s>",  // LCOV_EXCL_LINE :rfi
                                elemName, elementName);

  XMLDocument::setCurrentElement(parser, elem);
}

NABoolean QRTable::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (key_ && key_->treeWalk(visitor))
    return TRUE;

  return FALSE;
}

//
// QRKey
//
QRKey::QRKey(XMLElementPtr parent, AttributeList atts,
                       ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElementList(ET_Key, parent, NULL, ADD_MEMCHECK_ARGS_PASS(heap))
{
  if (*atts)
    throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                elemName, *atts);
}

void QRKey::startElement(void *parser, const char *elementName, const char **atts)
{
  QRElementPtr elem;

  if (!strcmp(elementName, QRColumn::elemName))
    elem = new (XMLPARSEHEAP) QRColumn(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else
    throw QRDescriptorException("<%s> cannot contain <%s>",  // LCOV_EXCL_LINE :rfi
                                elemName, elementName);

  addElement(elem);
  XMLDocument::setCurrentElement(parser, elem);
}

//
// QRForcedMVs
//

// LCOV_EXCL_START :cnu Entire QRForcedMVs is currently not used.
QRForcedMVs::~QRForcedMVs()
{
  CollIndex i;
  for (i = 0; i < tableList_.entries(); i++) 
    deletePtr(tableList_[i]);
}

NABoolean QRForcedMVs::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  for (CollIndex i = 0; i < tableList_.entries(); i++) 
    if (tableList_[i]->treeWalk(visitor))
      return TRUE;

  return FALSE;
}

void QRForcedMVs::startElement(void *parser, const char *elementName, const char **atts)
{
  QRTablePtr table;
  if (!strcmp(elementName, QRTable::elemName))
    {
      table = new (XMLPARSEHEAP) QRTable(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, table);
      addTable(table);
    }
  else
    {
      throw QRDescriptorException("Element %s cannot contain element %s",
                                  elemName, elementName);
    }
}

void QRForcedMVs::serializeBody(XMLString& xml)
{
  CollIndex i;
  for (i = 0; i < tableList_.entries(); i++) 
    tableList_[i]->toXML(xml);
}
// LCOV_EXCL_STOP


//
// QROperator
//

QROperator::QROperator(XMLElementPtr parent, AttributeList atts,
                       ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_Operator, parent, ADD_MEMCHECK_ARGS_PASS(heap))
{
  // Atts of Operator element have not been defined yet
}

NABoolean QROperator::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  // Add subelements here...

  return FALSE;
}


//
// QRJoinPred
//

QRJoinPred::QRJoinPred(XMLElementPtr parent, AttributeList atts,
                       ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElementList(ET_JoinPred, parent, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
    result_(INVALID_EXPR_RESULT)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      if (!strcmp(attrName, "id"))
        id_ = iter.getValue();
      else if (!strcmp(attrName, "ref"))
        ref_ = iter.getValue();
      else if (!strcmp(attrName, "result"))
        result_ = encodeResult(iter.getValue());
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    elemName, attrName);
    }
}

void QRJoinPred::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  if (result_ != INVALID_EXPR_RESULT)
    xml.append("result='").append(ExprResultNames[result_]).append( "' ");
}

void QRJoinPred::startElement(void *parser, const char *elementName, const char **atts)
{
  QRElementPtr elem;

  // A QRJoinPred can also have a QRJoinPred as a child element.
  if (!strcmp(elementName, QRJoinPred::elemName))
  {
    elem = new (XMLPARSEHEAP) QRJoinPred(CHILD_ELEM_ARGS(XMLPARSEHEAP));

    addElement(elem);
    XMLDocument::setCurrentElement(parser, elem);
  }
  else
  {
    QRElementList::startItemExprElement(parser, elementName, atts);
  }
}

NABoolean QRJoinPred::isRedundant() const
{
  const ElementPtrList& list = getList();
  const NAString& joinPredId = getID();
  for (CollIndex i=0; i<list.entries(); i++)
    {
      if (joinPredId != list[i]->getRef())
        return FALSE;
    }

  return TRUE;  // all contained eq set members referenced the containing joinpred
}

QRColumnPtr QRJoinPred::getFirstColumn()
{
  const ElementPtrList& eqList = getEqualityList();
  for (CollIndex i=0; i<eqList.entries(); i++)
    {
      const QRElementPtr elem = eqList[i]->getReferencedElement();
      // Return as soon as we find the first one.
      if (elem->getElementType() == ET_Column)
        return elem->downCastToQRColumn();
    }

  // Found no column in equality list, return NULL.
  return NULL;
}


//
// QRRangePred
//

QRRangePred::QRRangePred(XMLElementPtr parent, AttributeList atts,
                         ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_RangePred, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    result_(INVALID_EXPR_RESULT),
    rangeItem_(NULL),
    opList_(heap),
    mustMatch_(FALSE),
    sqlType_(heap),
    rangeSpec_(NULL)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      if (!strcmp(attrName, "id"))
        id_ = iter.getValue();
      else if (!strcmp(attrName, "ref"))
        ref_ = iter.getValue();
      else if (!strcmp(attrName, "sqlType"))
        sqlType_ = iter.getValue();
      else if (!strcmp(attrName, "result"))
        result_ = encodeResult(iter.getValue());
      else if (!strcmp(attrName, "mustMatch"))
        deserializeBoolAttr(attrName, iter.getValue(), mustMatch_);
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    elemName, attrName);
    }
}

QRRangePred::~QRRangePred()
{
  deletePtr(rangeItem_);
  for (CollIndex i = 0; i < opList_.entries(); i++) 
    deletePtr(opList_[i]);

  // RangeSpec is not a SharedPtr derivative.
  if (rangeSpec_ != NULL)
    delete rangeSpec_;
}

void QRRangePred::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  if (sqlType_.length() > 0)
    xml.append("sqlType='").append(sqlType_).append("' ");
  if (result_ != INVALID_EXPR_RESULT)
    xml.append("result='").append(ExprResultNames[result_]).append("' ");
  if (mustMatch_)
    serializeBoolAttr("mustMatch", mustMatch_, xml);
}

void QRRangePred::serializeBody(XMLString& xml)
{
  if (rangeItem_)
    rangeItem_->toXML(xml);
    if (opList_.entries() == 0)            // contains no operators
    {
      // Don't add the comment to result descriptor range preds that reference
      // query preds, because they are always empty.
      if (result_ == INVALID_EXPR_RESULT ||  // not in a result descriptor
          ref_ == "")                        // or in a constructed range pred       
      {
        // Add comment to <Range> element explaining that when empty it means
        // IS NOT NULL is specified or implied, but no other conditions.
        xml.indent();
        xml.append("<!-- empty Range element indicates IS NOT NULL -->");
        xml.endLine();
      }
    }
  else
    for (CollIndex i = 0; i < opList_.entries(); i++) 
      opList_[i]->toXML(xml);
}

NABoolean QRRangePred::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (rangeItem_ && rangeItem_->treeWalk(visitor))
    return TRUE;
  for (CollIndex i = 0; i < opList_.entries(); i++) 
    if (opList_[i]->treeWalk(visitor))
      return TRUE;

  return FALSE;
}

void QRRangePred::startElement(void *parser, const char *elementName, const char **atts)
{
  QRRangeOperatorPtr op;

  if (!strcmp(elementName, QRColumn::elemName))
    {
      setRangeItem(new (XMLPARSEHEAP) QRColumn(CHILD_ELEM_ARGS(XMLPARSEHEAP)));
      XMLDocument::setCurrentElement(parser, rangeItem_);
    }
  else if (!strcmp(elementName, QRMVColumn::elemName))
    {
      setRangeItem(new (XMLPARSEHEAP) QRMVColumn(CHILD_ELEM_ARGS(XMLPARSEHEAP)));
      XMLDocument::setCurrentElement(parser, rangeItem_);
    }
  else if (!strcmp(elementName, QRExpr::elemName))
    {
      setRangeItem(new (XMLPARSEHEAP) QRExpr(this, atts, FALSE, ADD_MEMCHECK_ARGS(XMLPARSEHEAP)));
      XMLDocument::setCurrentElement(parser, rangeItem_);
    }
  else if (!strcmp(elementName, QROpEQ::elemName))
    {
      op = new (XMLPARSEHEAP) QROpEQ(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      addOperator(op);
      XMLDocument::setCurrentElement(parser, op);
    }
  else if (!strcmp(elementName, QROpLS::elemName))
    {
      op = new (XMLPARSEHEAP) QROpLS(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      addOperator(op);
      XMLDocument::setCurrentElement(parser, op);
    }
  else if (!strcmp(elementName, QROpLE::elemName))
    {
      op = new (XMLPARSEHEAP) QROpLE(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      addOperator(op);
      XMLDocument::setCurrentElement(parser, op);
    }
  else if (!strcmp(elementName, QROpGT::elemName))
    {
      op = new (XMLPARSEHEAP) QROpGT(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      addOperator(op);
      XMLDocument::setCurrentElement(parser, op);
    }
  else if (!strcmp(elementName, QROpGE::elemName))
    {
      op = new (XMLPARSEHEAP) QROpGE(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      addOperator(op);
      XMLDocument::setCurrentElement(parser, op);
    }
  else if (!strcmp(elementName, QROpBT::elemName))
    {
      op = new (XMLPARSEHEAP) QROpBT(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      addOperator(op);
      XMLDocument::setCurrentElement(parser, op);
    }
  else
    throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                elemName, elementName);
} // QRRangePred::startElement

const RangeSpec* QRRangePred::getRangeSpec(NAMemory* heap)
{
  if (rangeSpec_ == NULL)
    rangeSpec_ = new (heap) RangeSpec(this, heap);

  return rangeSpec_;
}

NABoolean QRRangePred::isSingleValue()
{
  if (opList_.entries() != 1)
    return FALSE;

  QRRangeOperatorPtr op=opList_[0];
  if (op->getElementType() != ET_OpEQ)
    return FALSE;

  QROpEQPtr eqOp = static_cast<QROpEQPtr>(op);
  return (eqOp->getValueList().entries() == 1 && !eqOp->includesNull());
}

Int32 QRRangePred::getSize()
{
  Int32 result = 0;
  CollIndex maxEntries = opList_.entries();
  for (CollIndex i=0; i<maxEntries; i++)
    result += opList_[i]->getSize();

  return result;
}

//
// QROpEQ
//

QROpEQ::~QROpEQ()
{
  for (CollIndex i = 0; i < valueList_.entries(); i++) 
    deletePtr(valueList_[i]);

  deletePtr(nullVal_);
}

void QROpEQ::serializeBody(XMLString& xml)
{
  if (nullVal_)
    {
      xml.indent();
      xml.append('<').append(nullVal_->getElementName()).append("/>");
      xml.endLine();
    }

  for (CollIndex i = 0; i < valueList_.entries(); i++)
    valueList_[i]->toXML(xml);
}

NABoolean QROpEQ::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  for (CollIndex i = 0; i < valueList_.entries(); i++)
    if (valueList_[i]->treeWalk(visitor))
      return TRUE;

  return FALSE;
}

void QROpEQ::startElement(void *parser, const char *elementName, const char **atts)
{
  // Note the early return below when the element is <NullVal>
  QRScalarValuePtr valElem = NULL;
  if (!strcmp(elementName, QRNumericVal::elemName))
    valElem = new (XMLPARSEHEAP) QRNumericVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRStringVal::elemName))
    valElem = new (XMLPARSEHEAP) QRStringVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRWStringVal::elemName))
    valElem = new (XMLPARSEHEAP) QRWStringVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRFloatVal::elemName))
    valElem = new (XMLPARSEHEAP) QRFloatVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRNullVal::elemName))
    {
      nullVal_ = new (XMLPARSEHEAP) QRNullVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, nullVal_);
      return;
    }
  else
    throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                elemName, elementName);
  if (valElem)
    {
      addValue(valElem);                                
      XMLDocument::setCurrentElement(parser, valElem);
    }
}

void QROpEQ::unparse(NAString& text, const NAString& rangeItem)
{
  CollIndex numValues = valueList_.entries();
  if (numValues == 1)
  {
    // A single value - use the "<range-item> = <literal>" notation.
    text.append(rangeItem);
    text.append(" = ");
    text.append(valueList_[0]->getSql());
  }
  else if (numValues > 1)
  {
    // Multiple values - use the "<range-item> IN ( <literal> [, <literal> ] )" 
    text.append(rangeItem);
    text.append(" IN ( ");
    for (CollIndex i=0; i<numValues; i++)
    {
      text.append(valueList_[i]->getSql());
      if (i<numValues-1)
        text.append(", ");
    }
    text.append(" ) ");
  }

  if (nullVal_)
  {
    // Add an optional "[OR ] <range-item> IS NULL"
    if (numValues > 0)
      text.append(" OR ");

    text.append(rangeItem);
    text.append("IS NULL");
  }
}

//
// QROpInequality
//

QROpInequality::QROpInequality(ElementType eType, QRElement *parent, 
                               AttributeList atts, const char *elemName,
                               ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRRangeOperator(eType, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    value_(NULL),
    isNormalized_(FALSE)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  const char *attrVal;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      attrVal = iter.getValue();
      if (!strcmp(attrName, "isNormalized"))
        deserializeBoolAttr(attrName, attrVal, isNormalized_);
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    elemName, attrName);
    }
}

QROpInequality::~QROpInequality()
{
  deletePtr(value_);
}

void QROpInequality::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  if (isNormalized_)
    serializeBoolAttr("isNormalized", isNormalized_, xml);
}

void QROpInequality::serializeBody(XMLString& xml)
{
  value_->toXML(xml);
}

NABoolean QROpInequality::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);
  	
  if (value_->treeWalk(visitor))
    return TRUE;

  return FALSE;
}

void QROpInequality::startElement(void *parser, const char *elementName, const char **atts)
{
  if (!strcmp(elementName, QRNumericVal::elemName))
    value_ = new (XMLPARSEHEAP) QRNumericVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRStringVal::elemName))
    value_ = new (XMLPARSEHEAP) QRStringVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRWStringVal::elemName))
    value_ = new (XMLPARSEHEAP) QRWStringVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRFloatVal::elemName))
    value_ = new (XMLPARSEHEAP) QRFloatVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else
    throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                getElementName(), elementName);
  XMLDocument::setCurrentElement(parser, value_);
}

void QROpInequality::unparse(NAString& text, const NAString& rangeItem)
{
  // The text looks like this: "<range-item> <operator-sign> <literal>"
  text.append(rangeItem);
  text.append(getOperatorSign());
  text.append(value_->getSql());
}

//
// QROpBT
//

QROpBT::QROpBT(QRElement *parent, AttributeList atts,
               ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRRangeOperator(ET_OpBT, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    valueCount_(0), startValue_(NULL), endValue_(NULL),
    startIsIncluded_(TRUE), endIsIncluded_(TRUE)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  const char *attrVal;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      attrVal = iter.getValue();
      if (!strcmp(attrName, "startIsIncluded"))
        deserializeBoolAttr(attrName, attrVal, startIsIncluded_);
      else if (!strcmp(attrName, "endIsIncluded"))
        deserializeBoolAttr(attrName, attrVal, endIsIncluded_);
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    elemName, attrName);
    }
}

QROpBT::~QROpBT()
{
  deletePtr(startValue_);
  deletePtr(endValue_);
}

void QROpBT::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  serializeBoolAttr("startIsIncluded", startIsIncluded_, xml);
  serializeBoolAttr("endIsIncluded", endIsIncluded_, xml);
}

void QROpBT::serializeBody(XMLString& xml)
{
  if (startValue_)
    startValue_->toXML(xml);
  if (endValue_)
    endValue_->toXML(xml);
}

NABoolean QROpBT::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (startValue_ && startValue_->treeWalk(visitor))
    return TRUE;
  if (endValue_ && endValue_->treeWalk(visitor))
    return TRUE;

  return FALSE;
}

void QROpBT::startElement(void *parser, const char *elementName, const char **atts)
{
  valueCount_++;
  if (valueCount_ > 2)
    throw QRDescriptorException("Only 2 values can be contained in %s", elemName);  // LCOV_EXCL_LINE :rfi

  //QRElementPtr& val = (valueCount_ == 1 ? startValue_ : endValue_);
  QRScalarValuePtr& val = (valueCount_ == 1 ? startValue_ : endValue_);
  if (!strcmp(elementName, QRNumericVal::elemName))
    val = new (XMLPARSEHEAP) QRNumericVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRStringVal::elemName))
    val = new (XMLPARSEHEAP) QRStringVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRWStringVal::elemName))
    val = new (XMLPARSEHEAP) QRWStringVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRFloatVal::elemName))
    val = new (XMLPARSEHEAP) QRFloatVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else
    throw QRDescriptorException("Element %s cannot contain element %s",
                                elemName, elementName);
  XMLDocument::setCurrentElement(parser, val);
}

void QROpBT::unparse(NAString& text, const NAString& rangeItem)
{
  if (startIsIncluded_ && endIsIncluded_)
  {
    // If both ends are included, use "<range-item> BETWEEN <literal> AND <literal>"
    text.append(rangeItem);
    text.append(" BETWEEN ");
    text.append(startValue_->getSql());
    text.append(" AND ");
    text.append(endValue_->getSql());
  }
  else
  {
    // Otherwise use: "(<range-item> <operator> <literal> AND <range-item> <operator> <literal>)"
    text.append("(");
    text.append(rangeItem);
    text.append((startIsIncluded_ ? " >= " : " > "));
    text.append(startValue_->getSql());
    text.append(" AND ");
    text.append(rangeItem);
    text.append((endIsIncluded_ ? " <= " : " < "));
    text.append(endValue_->getSql());
    text.append(")");
  }
}

//
// QRScalarValue
//

QRScalarValue::QRScalarValue(ElementType eType, QRElement *parent, AttributeList atts,
                             ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRExplicitExpr(eType, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    stringRep_(charData_),
    sql_(heap)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      if (!strcmp(attrName, "id"))
        id_ = iter.getValue();
      if (!strcmp(attrName, "ref"))
        ref_ = iter.getValue();
      if (!strcmp(attrName, "sql"))
        sql_ = iter.getValue();
    }
}

void QRScalarValue::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  if (sql_ != "")
    xml.append("sql=\"").append(addEntityRefs(sql_, '"')).append("\" ");
}

//
// QRNumericVal
//

QRNumericVal::QRNumericVal(QRElement *parent, AttributeList atts,
                           ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRScalarValue(ET_NumericVal, parent, atts, ADD_MEMCHECK_ARGS_PASS(heap)),
    unscaledNumericVal_(0), numericScale_(0), scale_(heap)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      if (!strcmp(attrName, "scale"))
        scale_ = iter.getValue();
    }
}

void QRNumericVal::setValue(const NAString& value)
{
  // synchNumericValue() uses the string representation of the value, so set
  // that first.
  setStringRep(value);
  synchNumericValue();
}

void QRNumericVal::setNumericVal(Int64 unscaledVal, Int32 scale)
{
  unscaledNumericVal_ = unscaledVal;
  numericScale_ = scale;

  // Keep string representations of unscaled value and scale consistent with
  // the new value.
  snprintf(buf, 
          sizeof(buf),
          "%0*ld",
          scale + (unscaledVal < 0),  // add one for sign if negative
          unscaledVal);
  stringRep_ = buf;
  if (scale > 0)
    stringRep_.insert(stringRep_.length() - scale, ".");

  snprintf(buf, sizeof(buf), "%d", scale);
  scale_ = buf;
}

void QRNumericVal::synchNumericValue()
{
  // Store the scale and the unscaled numeric value.
  // @ZX -- This needs sorting out. It uses the scale attribute rather than the
  //        position of the decimal point in the string representation of the
  //        constant. The scale attribute is redundant with the scaled
  //        representation of the value that is the content of QRNumericVal, and
  //        this needs to be resolved.
  if (strlen(scale_.data()) > 0)
    sscanf(scale_.data(), "%d", &numericScale_);

  Int64 fractionalPart = 0;
  sscanf(stringRep_.data(),
          PFLL "." PFLL,
          &unscaledNumericVal_, &fractionalPart);
  unscaledNumericVal_ *= (Int64)pow(10, numericScale_);
  unscaledNumericVal_ += fractionalPart;
}


//
// QRStringVal
//

void QRStringVal::toXML(XMLString& xml, NABoolean dummy1, NABoolean dummy2)
{
  // Put the content (the string value) directly between the start and end tags
  // with no spaces or newlines added. No whitespace will be stripped from
  // StringVal's content when deserialized, allowing for leading and trailing
  // whitespace that is part of the actual string value.
  XMLElement::toXML(xml, TRUE);
}


//
// QRWStringVal
//

void QRWStringVal::toXML(XMLString& xml, NABoolean dummy1, NABoolean dummy2)
{
  // Put the content (the string value) directly between the start and end tags
  // with no spaces or newlines added.
  XMLElement::toXML(xml, TRUE);
}

// Restore the actual code point for a Unicode character encoded as a sequence
// of 4 hex digits. Get the 4-bit value for each hex digit and shift it into
// position, taking endianness into account.
NAWchar QRWStringVal::decode_(const char* buf) const
{
#ifdef NA_LITTLE_ENDIAN
  const char* low  = buf;
  const char* high = buf+2;
#else
  const char* low  = buf+2;
  const char* high = buf;
#endif
  NAWchar val = (isdigit(*high) ? *high-48 : *high-55) << 12;
  val += (isdigit(*(high+1)) ? *(high+1)-48 : *(high+1)-55) << 8;
  val += (isdigit(*low) ? *low-48 : *low-55) << 4;
  val += (isdigit(*(low+1)) ? *(low+1)-48 : *(low+1)-55);
  return val;
}

// XML parsers have a good deal of latitude in how they break up character data
// into calls to this function. The problem for us is that it takes a sequence
// of 4 characters of input to decode the sequence into a Unicode character. If
// our buffer runs out in the middle of a char's representation, we have to hold
// on to the initial part, and put it together with the remaining hex digits
// for that character in the next call.
void QRWStringVal::charData(void *parser, const char *data, Int32 len)
{
  Int32 startOfst = 0;
  NAWchar wch;
  // if danglingCharCount_ is nonzero, we're holding a partial representation
  // from the previous call.
  if (danglingCharCount_)
    {
      // We don't handle the case where the representation of a single char
      // spans 3 calls to this function.
      assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                        len >= 4-danglingCharCount_, 
                        QRDescriptorException,
                        "charData() received buffer that does not complete dangling char");
      // Copy the remainder to our local buffer and decode.
      memcpy(danglingChars_ + danglingCharCount_,
             data,
             4 - danglingCharCount_);
      wch = decode_(danglingChars_);
      wideStringRep_.append(&wch, 1);
      startOfst = (4 - danglingCharCount_); // update starting point in passed buffer
    }

  // Decode each 4-byte sequence in the buffer, and append to the wide char
  // string.
  for (Int32 i=startOfst; i<len-3; i+=4)
    {
      wch = decode_(data+i);
      wideStringRep_.append(&wch, 1);
    }

  // If there are chars left over (max 3), save them in separate buffer for when
  // we get the rest in the next call.
  danglingCharCount_ = (len - startOfst) % 4;
  if (danglingCharCount_)
    memcpy(danglingChars_, data + len - danglingCharCount_, danglingCharCount_);
}

// This array maps the value of a byte to the hex digit pair that represents the
// same value in hex.
static const char* hexArray[] =
  {
    "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", "0A", "0B", "0C", "0D", "0E", "0F",
    "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "1A", "1B", "1C", "1D", "1E", "1F",
    "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", "2A", "2B", "2C", "2D", "2E", "2F",
    "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", "3A", "3B", "3C", "3D", "3E", "3F",
    "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", "4A", "4B", "4C", "4D", "4E", "4F",
    "50", "51", "52", "53", "54", "55", "56", "57", "58", "59", "5A", "5B", "5C", "5D", "5E", "5F",
    "60", "61", "62", "63", "64", "65", "66", "67", "68", "69", "6A", "6B", "6C", "6D", "6E", "6F",
    "70", "71", "72", "73", "74", "75", "76", "77", "78", "79", "7A", "7B", "7C", "7D", "7E", "7F",
    "80", "81", "82", "83", "84", "85", "86", "87", "88", "89", "8A", "8B", "8C", "8D", "8E", "8F",
    "90", "91", "92", "93", "94", "95", "96", "97", "98", "99", "9A", "9B", "9C", "9D", "9E", "9F",
    "A0", "A1", "A2", "A3", "A4", "A5", "A6", "A7", "A8", "A9", "AA", "AB", "AC", "AD", "AE", "AF",
    "B0", "B1", "B2", "B3", "B4", "B5", "B6", "B7", "B8", "B9", "BA", "BB", "BC", "BD", "BE", "BF",
    "C0", "C1", "C2", "C3", "C4", "C5", "C6", "C7", "C8", "C9", "CA", "CB", "CC", "CD", "CE", "CF",
    "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7", "D8", "D9", "DA", "DB", "DC", "DD", "DE", "DF",
    "E0", "E1", "E2", "E3", "E4", "E5", "E6", "E7", "E8", "E9", "EA", "EB", "EC", "ED", "EE", "EF",
    "F0", "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "FA", "FB", "FC", "FD", "FE", "FF",
  };

// For each of the double-byte characters that make up the wide string value,
// encode it in 4 hex digits and append them to the XML document.
void QRWStringVal::serializeBody(XMLString& xml)
{
  // chPtr MUST be unsigned, or value when used as an index could be negative.
  const unsigned char* chPtr = (const unsigned char*)wideStringRep_.data();
  size_t wchLen = wideStringRep_.length();
  for (size_t i=0; i<wchLen; i++)
    {
      xml.append(hexArray[*chPtr++]);
      xml.append(hexArray[*chPtr++]);
    }
}


//
// QRGroupBy
//

QRGroupBy::QRGroupBy(XMLElementPtr parent, AttributeList atts,
                     ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_GroupBy, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    result_(INVALID_EXPR_RESULT),
    primary_(NULL),
    dependent_(NULL),
    minimal_(NULL),
    tableList_(NULL)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
  {
    iter.next();
    attrName = iter.getName();
    if (!strcmp(attrName, "id"))
      id_ = iter.getValue();
    else if (!strcmp(attrName, "ref"))
      ref_ = iter.getValue();
    else if (!strcmp(attrName, "result"))
      result_ = encodeResult(iter.getValue());
    else
      throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                  elemName, attrName);
  }
}

QRGroupBy::~QRGroupBy()
{
  deletePtr(primary_);
  deletePtr(dependent_);
  deletePtr(minimal_);
  deletePtr(tableList_);
}

void QRGroupBy::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  if (result_ != INVALID_EXPR_RESULT)
    xml.append("result='").append(ExprResultNames[result_]).append("' ");
}

void QRGroupBy::serializeBody(XMLString& xml)
{
  if (primary_ && !primary_->isEmpty())
    primary_->toXML(xml);

  if (dependent_ && !dependent_->isEmpty())
    dependent_->toXML(xml);

  if (minimal_ && !minimal_->isEmpty())
    minimal_->toXML(xml);

  if (tableList_ && !tableList_->isEmpty())
    tableList_->toXML(xml);
}

NABoolean QRGroupBy::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (primary_ && primary_->treeWalk(visitor))
    return TRUE;

  if (dependent_ && dependent_->treeWalk(visitor))
    return TRUE;

  if (minimal_ && minimal_->treeWalk(visitor))
    return TRUE;

  if (tableList_ && tableList_->treeWalk(visitor))
    return TRUE;

  return FALSE;
}

void QRGroupBy::startElement(void *parser, const char *elementName, const char **atts)
{
  QRElementPtr elem = NULL;
  if      (!strcmp(elementName, QRPrimaryGroupBy::elemName))
  {
    primary_ = new (XMLPARSEHEAP) QRPrimaryGroupBy(CHILD_ELEM_ARGS(XMLPARSEHEAP));
    elem = primary_;
  }
  // LCOV_EXCL_START :cnu Dependent/Minimal GB not implemented yet.
  else if (!strcmp(elementName, QRDependentGroupBy::elemName))
  {
    dependent_ = new (XMLPARSEHEAP) QRDependentGroupBy(CHILD_ELEM_ARGS(XMLPARSEHEAP));
    elem = dependent_;
  }
  else if (!strcmp(elementName, QRMinimalGroupBy::elemName))
  {
    minimal_ = new (XMLPARSEHEAP) QRMinimalGroupBy(CHILD_ELEM_ARGS(XMLPARSEHEAP));
    elem = minimal_;
  }
  // LCOV_EXCL_STOP
  else if (!strcmp(elementName, QRList<QRTable>::elemName))
  {
    tableList_ = new (XMLPARSEHEAP) QRList<QRTable>(CHILD_ELEM_ARGS(XMLPARSEHEAP));
    elem = tableList_;
  }
  else
  {
    throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                elemName, elementName);
  }

  XMLDocument::setCurrentElement(parser, elem);
}


//
// QRPrimaryGroupBy
//
QRPrimaryGroupBy::QRPrimaryGroupBy(XMLElementPtr parent, AttributeList atts,
                       ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElementList(ET_PrimaryGroupBy, parent, NULL, ADD_MEMCHECK_ARGS_PASS(heap))
{
  if (*atts)
    throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                elemName, *atts);
}

void QRPrimaryGroupBy::startElement(void *parser, const char *elementName, const char **atts)
{
  QRElementList::startItemExprElement(parser, elementName, atts);
}

// LCOV_EXCL_START :cnu Dependent GB not implemented yet.
//
// QRDependentGroupBy
//
QRDependentGroupBy::QRDependentGroupBy(XMLElementPtr parent, AttributeList atts,
                       ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElementList(ET_DependentGroupBy, parent, NULL, ADD_MEMCHECK_ARGS_PASS(heap))
{
  if (*atts)
    throw QRDescriptorException("Invalid attribute specified for element %s: %s",
                                elemName, *atts);
}

void QRDependentGroupBy::startElement(void *parser, const char *elementName, const char **atts)
{
  QRElementList::startItemExprElement(parser, elementName, atts);
}
// LCOV_EXCL_STOP

// LCOV_EXCL_START :cnu Minimal GB not implemented yet.
//
// QRMinimalGroupBy
//
QRMinimalGroupBy::QRMinimalGroupBy(XMLElementPtr parent, AttributeList atts,
                       ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElementList(ET_MinimalGroupBy, parent, NULL, ADD_MEMCHECK_ARGS_PASS(heap))
{
  if (*atts)
    throw QRDescriptorException("Invalid attribute specified for element %s: %s",
                                elemName, *atts);
}

void QRMinimalGroupBy::startElement(void *parser, const char *elementName, const char **atts)
{
  QRElementList::startItemExprElement(parser, elementName, atts);
}
// LCOV_EXCL_STOP



//
// QRColumn
//

QRColumn::QRColumn(XMLElementPtr parent, AttributeList atts,
                   ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRExplicitExpr(ET_Column, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    tableId_(heap),
    fqColumnName_(charData_),
    columnName_(heap),
    isExtraHub_(FALSE),
    colIndex_(-1),
    isNullable_(1),
    vegrefVid_(0)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  const char *attrValue;
  while (iter.hasNext())
    {
      iter.next();
      attrName  = iter.getName();
      attrValue = iter.getValue();
      if (!strcmp(attrName, "id"))
        id_ = attrValue;
      else if (!strcmp(attrName, "ref"))
        ref_ = attrValue;
      else if (!strcmp(attrName, "tableId"))
        tableId_ = attrValue;
      else if (!strcmp(attrName, "colIndex"))
        colIndex_ = atoi(attrValue);
      else if (!strcmp(attrName, "isNullable"))
        deserializeBoolAttr(attrName, attrValue, isNullable_);
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    elemName, attrName);
    }
}


// Redefined to initialize the non-fully qualified column name.
void QRColumn::charData(void *parser, const char *data, Int32 len)
{
  // Call the superclass first to initialize the fully qualified column name.
  QRElement::charData(parser, data, len);

  // Extract the col name from the fully qualified version and set it.
  setColumnName_();
}

void QRColumn::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  if (tableId_.length() > 0)
    xml.append("tableId='").append(tableId_).append("' ");
  if (colIndex_ != -1)
  {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "colIndex='%d' ", colIndex_);
    xml.append(buffer);
  }
  if (isNullable_ == FALSE)
    xml.append("isNullable='0' ");
}

void QRColumn::setColumnName_()
{
  if (fqColumnName_.length() > 0) // QRColumn with ref attr won't include name
    {
      // Find the position of the column name after the last dot
      UInt32 pos = fqColumnName_.last('.')+1;
      UInt32 length = fqColumnName_.length();

      // Set the column name.
      columnName_ = fqColumnName_(pos, length - pos);
    }
}

void QRColumn::unparse(NAString& text, NABoolean useColumnName)
{
  if (!useColumnName)
    text.append("<col>"); //"%s";
  else
    text.append(getReferencedElement()->getSortName());
}

QRColumnPtr QRColumn::getFirstColumn()
{
  if (getReferencedElement() == this)
    return this;
  else
    return getReferencedElement()->getFirstColumn();
}

//
// QRExpr
//

QRExpr::QRExpr(XMLElementPtr parent, AttributeList atts, NABoolean isResidual,
               ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_Expr, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    exprRoot_(NULL),
    unparsedTextWithNames_(heap),
    unparsedTextNoNames_(heap),
    unparsedTextCharSet_(CharInfo::ISO88591),
    inputColumns_(heap),
    isResidual_(isResidual),
    isExtraHub_(FALSE),
    result_(INVALID_EXPR_RESULT),
    info_(NULL)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      if (!strcmp(attrName, "id"))
        id_ = iter.getValue();
      else if (!strcmp(attrName, "ref"))
        ref_ = iter.getValue();
      else if (!strcmp(attrName, "result"))
        {
          assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                            isResidual_, QRDescriptorException,
                            "'result' attribute specified for non-residual expr.");
          result_ = encodeResult(iter.getValue());
        }
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    getElementName(), //@ZXresid -- virtual fn in ctor
                                    attrName);
    }
}

QRExpr::~QRExpr()
{
  deletePtr(exprRoot_);
  inputColumns_.clear();
  deletePtr(info_);
}

void QRExpr::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  if (result_ != INVALID_EXPR_RESULT)
    xml.append("result='").append(ExprResultNames[result_]).append("' ");
}

void QRExpr::serializeBody(XMLString& xml)
{
  if (exprRoot_)
    exprRoot_->toXML(xml);
  if (info_)
    info_->toXML(xml);
}

NABoolean QRExpr::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (exprRoot_ && exprRoot_->treeWalk(visitor))
    return TRUE;

  return FALSE;
}

void QRExpr::startElement(void *parser, const char *elementName, const char **atts)
{
  if (!strcmp(elementName, QRBinaryOper::elemName))
    {
      exprRoot_ = new (XMLPARSEHEAP) QRBinaryOper(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, exprRoot_);
    }
  else if (!strcmp(elementName, QRFunction::elemName))
    {
      exprRoot_ = new (XMLPARSEHEAP) QRFunction(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, exprRoot_);
    }
  else if (!strcmp(elementName, QRUnaryOper::elemName))
    {
      exprRoot_ = new (XMLPARSEHEAP) QRUnaryOper(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, exprRoot_);
    }
  else if (!strcmp(elementName, QRMVColumn::elemName))
    {
      exprRoot_ = new (XMLPARSEHEAP) QRMVColumn(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, exprRoot_);
    }
  else if (!strcmp(elementName, QRInfo::elemName))
    {
      info_ = new (XMLPARSEHEAP) QRInfo(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, info_);
    }
  else
    {
      throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                  getElementName(), elementName);
    }
}

const NAString& QRExpr::getExprText(NABoolean useColumnName)
{
  NAString& unparsedText = useColumnName ? unparsedTextWithNames_ : unparsedTextNoNames_;
 
  if (unparsedText == "")
  {
    assertLogAndThrow(CAT_SQL_COMP_QR_COMMON, LL_MVQR_FAIL,
                      exprRoot_, QRDescriptorException,
                      "QRExpr has null exprRoot_ and no unparsed text");
    exprRoot_->unparse(unparsedText, useColumnName);
  }

  return unparsedText;
}

const ElementPtrList& QRExpr::getInputColumns(CollHeap* heap,
                                              NABoolean useRefedElem)
{
  if (inputColumns_.entries() == 0)
  {
    assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                      exprRoot_, QRDescriptorException,
                      "QRExpr has null exprRoot_ and no input columns");
    exprRoot_->getInputColumns(inputColumns_, heap, useRefedElem);
  }

  return inputColumns_;
}

QRColumnPtr QRExpr::getFirstColumn()
{
  const ElementPtrList& eqList = getInputColumns(NULL);
  if (eqList.entries() == 0)
    return NULL;
  else
    return eqList[0]->getReferencedElement()->getFirstColumn();
}

//
// QRExplicitExpr
//

NABoolean QRExplicitExpr::containsAnAggregate(CollHeap* heap)
{
  // Return cached value if we've done this already.
  if (containsAnAggregate_ != AGGREGATE_UNKNOWN)
    return (NABoolean)containsAnAggregate_;

  AggregateFinderVisitorPtr aggVisitor 
          = new(heap) AggregateFinderVisitor(ADD_MEMCHECK_ARGS(heap));
  treeWalk(aggVisitor);
  NABoolean returnValue = aggVisitor->foundAggregate();
  containsAnAggregate_ = (enum AggregateStatus)returnValue;  // cache the result
  deletePtr(aggVisitor);
  return returnValue;
}

QRExplicitExprPtr QRExplicitExpr::deepCopyAndSwitch(subExpressionRewriteHash& subExpressions, 
                                                    CollHeap* heap)
{
  const NAString* id = &getID();
  if (*id == "")
    id = &getRef();

  if(subExpressions.contains(id))
  {
    const QRExplicitExprPtr mvCol = subExpressions.getFirstValue(id);
    subExpressions.remove(id);
    return mvCol;
  }
  else
  {
    QRExplicitExprPtr dup = deepCopy(subExpressions, heap);
    dup->setRef(*id);
    return dup;
  }
}

QRExplicitExprPtr QRExplicitExpr::constructSubElement(void *parser,
                                                      const char *elementName,
                                                      const char **atts)
{
  QRExplicitExprPtr elem;
  if (!strcmp(elementName, QRColumn::elemName))
    elem = new (XMLPARSEHEAP) QRColumn(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRMVColumn::elemName))
    elem = new (XMLPARSEHEAP) QRMVColumn(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRBinaryOper::elemName))
    elem = new (XMLPARSEHEAP) QRBinaryOper(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRFunction::elemName))
    elem = new (XMLPARSEHEAP) QRFunction(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRUnaryOper::elemName))
    elem = new (XMLPARSEHEAP) QRUnaryOper(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRStringVal::elemName))
    elem = new (XMLPARSEHEAP) QRStringVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRWStringVal::elemName))
    elem = new (XMLPARSEHEAP) QRWStringVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRNumericVal::elemName))
    elem = new (XMLPARSEHEAP) QRNumericVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRFloatVal::elemName))
    elem = new (XMLPARSEHEAP) QRFloatVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else if (!strcmp(elementName, QRNullVal::elemName))
    elem = new (XMLPARSEHEAP) QRNullVal(CHILD_ELEM_ARGS(XMLPARSEHEAP));
  else
    {
      throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                  getElementName(), elementName);
    }

  return elem;
}

void QRExplicitExpr::getInputColumns(ElementPtrList& inputList, CollHeap* heap,
                                     NABoolean useRefedElem)
{
  NAList<ElementType> elemTypes(heap);
  elemTypes.insert(ET_Column);
  elemTypes.insert(ET_MVColumn);
  ElementFinderVisitorPtr visitor = 
    new(heap) ElementFinderVisitor(elemTypes, useRefedElem,
                                   ADD_MEMCHECK_ARGS(heap));
  treeWalk(visitor);
  inputList = visitor->getElementsFound();
  deletePtr(visitor);
}

//
// QRBinaryOper
//

QRBinaryOper::QRBinaryOper(XMLElementPtr parent, AttributeList atts,
                           ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRExplicitExpr(ET_BinaryOper, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    operator_(heap),
    firstOperand_(NULL),
    secondOperand_(NULL)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      if (!strcmp(attrName, "id"))
        id_ = iter.getValue();
      else if (!strcmp(attrName, "ref"))
        ref_ = iter.getValue();
      else if (!strcmp(attrName, "op"))
        operator_ = iter.getValue();
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    elemName, attrName);
    }
}

void QRBinaryOper::serializeAttrs(XMLString& xml)
{
  QRExplicitExpr::serializeAttrs(xml);

  if (operator_.length() > 0)
    {
      const char* opText = operator_.data();
      xml.append("op='");
      if (*opText == '<')
        {
          // Use entity reference in place of '<' to avoid illegal attribute.
          xml.append("&lt;");
          opText++;
        }
      xml.append(opText).append("' ");
    }
}

void QRBinaryOper::serializeBody(XMLString& xml)
{
  if (firstOperand_)
    firstOperand_->toXML(xml);
  if (secondOperand_)
    secondOperand_->toXML(xml);
}

NABoolean QRBinaryOper::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (firstOperand_ && firstOperand_->treeWalk(visitor))
    return TRUE;
  if (secondOperand_ && secondOperand_->treeWalk(visitor))
    return TRUE;

  return FALSE;
}

void QRBinaryOper::startElement(void* parser, const char* elementName, const char** atts)
{
  QRExplicitExprPtr elem = constructSubElement(parser, elementName, atts);
  if (!firstOperand_)
    firstOperand_ = elem;
  else if (!secondOperand_)
    secondOperand_ = elem;
  else
    throw QRDescriptorException("More than two operands given for binary operator "   // LCOV_EXCL_LINE :rfi
                                "with id=%s", id_.data());
                                
  XMLDocument::setCurrentElement(parser, elem);
}


//
// QRUnaryOper
//

QRUnaryOper::QRUnaryOper(XMLElementPtr parent, AttributeList atts,
                         ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRExplicitExpr(ET_UnaryOper, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    operator_(heap),
    operand_(NULL)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      if (!strcmp(attrName, "id"))
        id_ = iter.getValue();
      else if (!strcmp(attrName, "ref"))
        ref_ = iter.getValue();
      else if (!strcmp(attrName, "op"))
        operator_ = iter.getValue();
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    elemName, attrName);
    }
}

void QRUnaryOper::serializeAttrs(XMLString& xml)
{
  QRExplicitExpr::serializeAttrs(xml);

  if (operator_.length() > 0)
    xml.append("op='").append(operator_.data()).append("' ");
}

void QRUnaryOper::serializeBody(XMLString& xml)
{
  if (operand_)
    operand_->toXML(xml);
}

NABoolean QRUnaryOper::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (operand_ && operand_->treeWalk(visitor))
    return TRUE;

  return FALSE;
}

void QRUnaryOper::startElement(void *parser, const char *elementName, const char **atts)
{
  QRExplicitExprPtr elem = constructSubElement(parser, elementName, atts);
  if (!operand_)
    operand_ = elem;
  else
    throw QRDescriptorException("More than one operand given for unary operator "   // LCOV_EXCL_LINE :rfi
                                "with id=%s", id_.data());
                                
  XMLDocument::setCurrentElement(parser, elem);
}

//
// QRParameter
//

QRParameter::QRParameter(XMLElementPtr parent, AttributeList atts,
                         ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_Parameter, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    paramName_(heap), paramValue_(heap)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      if (!strcmp(attrName, "name"))
        paramName_ = iter.getValue();
      else if (!strcmp(attrName, "value"))
        paramValue_ = iter.getValue();
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    elemName, attrName);
    }
}

void QRParameter::serializeAttrs(XMLString& xml)
{
  assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                    paramName_.length() > 0, QRDescriptorException,
                    "'name' attribute not specified for %s element.", elemName);
  assertLogAndThrow1(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                    paramValue_.length() > 0, QRDescriptorException,
                    "'value' attribute not specified for %s element.", elemName);
  xml.append("name='").append(paramName_).append("\' ");
  xml.append("value='").append(paramValue_).append("\' ");
}

//
// QRFunction
//

QRFunction::QRFunction(XMLElementPtr parent, AttributeList atts,
                       ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRExplicitExpr(ET_Function, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    function_(heap),
    arguments_(heap),
    hiddenParams_(heap),
    aggregateFunc_(AFT_NONE)
{
  XMLAttributeIterator iter(atts);
  const char* attrName;
  const char* attrVal;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      attrVal  = iter.getValue();
      if (!strcmp(attrName, "id"))
        id_ = attrVal;
      else if (!strcmp(attrName, "ref"))
        ref_ = iter.getValue();
      else if (!strcmp(attrName, "op"))
        function_ = attrVal;
      else if (!strcmp(attrName, "aggregateFunc"))
        aggregateFunc_ = (AggregateFunctionType)atoi(iter.getValue());
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    getElementName(), attrName);
    }
}

QRFunction::QRFunction(const QRFunction& other,
                       ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRExplicitExpr(other, ADD_MEMCHECK_ARGS_PASS(heap)),
    function_(other.function_, heap),
    arguments_(heap),
    hiddenParams_(heap),
    aggregateFunc_(other.aggregateFunc_)
{
  for (CollIndex i=0; i<other.hiddenParams_.entries(); i++)
    addHiddenParam(new(heap) QRParameter(*other.hiddenParams_[i]));
}

QRFunction::~QRFunction()
{
  for (CollIndex i = 0; i < arguments_.entries(); i++) 
    deletePtr(arguments_[i]);

  for (CollIndex i = 0; i < hiddenParams_.entries(); i++)
    deletePtr(hiddenParams_[i]);
}

void QRFunction::serializeAttrs(XMLString& xml)
{
  QRExplicitExpr::serializeAttrs(xml);

  if (function_.length() > 0)
    xml.append("op='").append(function_.data()).append("' ");
  if (aggregateFunc_ != AFT_NONE)  // Optional attr, AF_NONE is the default.
  {
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "aggregateFunc='%d' ", (Int32)aggregateFunc_);
    xml.append(buffer);
  }
}

void QRFunction::serializeBody(XMLString& xml)
{
  for (CollIndex i = 0; i < arguments_.entries(); i++)
    arguments_[i]->toXML(xml);

  for (CollIndex i = 0; i < hiddenParams_.entries(); i++)
    hiddenParams_[i]->toXML(xml, FALSE, TRUE);
}

NABoolean QRFunction::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  for (CollIndex i = 0; i < arguments_.entries(); i++)
    if (arguments_[i]->treeWalk(visitor))
    	return TRUE;

  for (CollIndex i = 0; i < hiddenParams_.entries(); i++)
    if (hiddenParams_[i]->treeWalk(visitor))
    	return TRUE;

  return FALSE;
}

void QRFunction::startElement(void *parser, const char *elementName, const char **atts)
{
 if (!strcmp(elementName, QRParameter::elemName))
    {
      QRParameterPtr param =
                new (XMLPARSEHEAP) QRParameter(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      addHiddenParam(param);
      XMLDocument::setCurrentElement(parser, param);
    }
  else
    {
      QRExplicitExprPtr elem = constructSubElement(parser, elementName, atts);
      addArgument(elem);                                
      XMLDocument::setCurrentElement(parser, elem);
    }
}

void QRFunction::setAggregateFunc(OperatorTypeEnum itemExprType, NABoolean isDistinct)
{
  if (isDistinct)
  {
    // ItemExpr::getText() for distinct aggregate functions returns a text 
    // that includes a ValueID. This breaks matching, so we need to override
    // the function name.
    NAString name;
    switch (itemExprType)
    {
      case ITM_COUNT_NONULL: 
	aggregateFunc_ = AFT_COUNT_DISTINCT;
        name = "count distinct";
	break;

      case ITM_SUM: 
	aggregateFunc_ = AFT_SUM_DISTINCT;
        name = "sum distinct";
	break;

      default:
	throw QRDescriptorException("Unsupported distinct aggregate function.");
    }
    setFunctionName(name);
  }
  else
  {
    switch (itemExprType)
    {
      case ITM_COUNT_NONULL: 
	aggregateFunc_ = AFT_COUNT;
	break;

      case ITM_COUNT:
	aggregateFunc_ = AFT_COUNTSTAR;
	break;

      case ITM_SUM: 
	aggregateFunc_ = AFT_SUM;
	break;

      case ITM_MIN: 
	aggregateFunc_ = AFT_MIN;
	break;

      case ITM_MAX: 
	aggregateFunc_ = AFT_MAX;
	break;

      case ITM_ONE_ROW:
	aggregateFunc_ = AFT_ONE_ROW;
	break;

      case ITM_ONEROW:
	aggregateFunc_ = AFT_ONEROW;
	break;

      case ITM_ONE_TRUE:
	aggregateFunc_ = AFT_ONE_TRUE;
	break;

      case ITM_ANY_TRUE:
	aggregateFunc_ = AFT_ANY_TRUE;
	break;

      // AVG is not listed here because by now it will get translated to SUM/COUNT.
      // STDDEV/VARIANCE get translated to ScalarVariance(SUM*SUM, SUM, COUNT)
      default:
	throw QRDescriptorException("Unsupported aggregate function.");
    }
  }
}

NABoolean QRFunction::isCountStarEquivalent(CollHeap* heap)
{
  // If its a COUNT(*), than its really easy.
  if (aggregateFunc_ == AFT_COUNTSTAR)
    return TRUE;
  // If its not a COUNT, forget it.
  else if (aggregateFunc_ != AFT_COUNT)
    return FALSE;

  // OK, this is a COUNT.
  // Now check that all the input columns are not nullable.
  ElementPtrList inputList(heap);
  getInputColumns(inputList, heap);

  NABoolean allInputsAreNotNullable = TRUE;
  for (CollIndex i=0; i<inputList.entries(); i++)
  {
    QRElementPtr input = inputList[i]->getReferencedElement();
    if (input->getElementType() != ET_Column)
    {
      // This is a join pred. Don't bother for now.
      allInputsAreNotNullable = FALSE;
      break;
    }
    QRColumnPtr inputCol = input->downCastToQRColumn();
    if (inputCol->isNullable())
    {
      allInputsAreNotNullable = FALSE;
      break;
    }
  }

  return allInputsAreNotNullable;
}

void QRFunction::unparse(NAString& text, NABoolean useColumnName)
{
  if (function_ == "cast")
  {
    // Skip Cast functions.
    arguments_[0]->unparse(text, useColumnName);
  }
  else
  {
    text.append(function_ + "[");

    // Do the parameters inside square brackets.
    for (CollIndex j=0; j<hiddenParams_.entries(); j++)
    {
      QRParameterPtr param = hiddenParams_[j];
      text.append(param->getName() + "=" + param->getValue() + " ");
    }

    // And then the argumants inside parenthesis.
    text.append("](");
    CollIndex maxEntries = arguments_.entries();
    for (CollIndex i=0; i<maxEntries; i++)
    {
      arguments_[i]->unparse(text, useColumnName);
      if (i != maxEntries-1)
	text.append(", ");
    }
    text.append(")");
  }
}

//
// QRMVColumn
//

QRMVColumn::QRMVColumn(XMLElementPtr parent, AttributeList atts,
                       ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRExplicitExpr(ET_MVColumn, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    mv_(heap),
    mvColName_(charData_),
    aggForRewrite_(ITM_NOT)  // This means not initialized.
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      if (!strcmp(attrName, "MV"))
        mv_ = iter.getValue();
      else if (!strcmp(attrName, "ref"))
        ref_ = iter.getValue();
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",
                                    elemName, attrName);
    }
}

void QRMVColumn::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  if (mv_.length() > 0)
    xml.append("MV='").append(mv_).append("' ");
}

NABoolean QRMVColumn::hasRefTo(CollIndex vid)
{
  NumericID refNum = getRefNum();
  char firstChar = getRefFirstChar();

  switch (firstChar)
    {
      case 'C':
        return vid == refNum;

      case 'J':
        {
          QRJoinPredPtr joinPred = getReferencedElement()->downCastToQRJoinPred();
          const ElementPtrList& eqList = joinPred->getEqualityList();
          for (CollIndex i=0; i<eqList.entries(); i++)
            {
              if (vid == eqList[i]->getReferencedElement()->getIDNum())
                return TRUE;
            }
          return FALSE;
        }

      // Default includes ' ', returned by getRefFirstChar() when there is no ref.
      default:
        return FALSE;
    }
}


//
// QROutput
//

QROutput::QROutput(XMLElementPtr parent, AttributeList atts,
                   ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElementList(ET_Output, parent, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
    name_(heap),
    result_(INVALID_EXPR_RESULT),
    colPos_(-1)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      if (!strcmp(attrName, "id"))
        id_ = iter.getValue();
      else if (!strcmp(attrName, "name"))
        name_ = iter.getValue();
      else if (!strcmp(attrName, "ref"))
        ref_ = iter.getValue();
      else if (!strcmp(attrName, "result"))
        result_ = encodeResult(iter.getValue());
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    elemName, attrName);
    }
}

void QROutput::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  if (name_.length() > 0)
    xml.append("name='").append(name_).append("' ");
  if (result_ != INVALID_EXPR_RESULT)
    xml.append("result='").append(ExprResultNames[result_]).append("' ");
}

void QROutput::startElement(void *parser, const char *elementName, const char **atts)
{
  QRElementList::startItemExprElement(parser, elementName, atts);
}


//
// QRExtraHub
//

void QRExtraHub::serializeBody(XMLString& xml)
{
  if (tableList_ && !tableList_->isEmpty())
    tableList_->toXML(xml);
  if (joinPredList_ && !joinPredList_->isEmpty())
    joinPredList_->toXML(xml);
}

NABoolean QRExtraHub::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (tableList_ && !tableList_->isEmpty() && tableList_->treeWalk(visitor))
    return TRUE;
  if (joinPredList_ && !joinPredList_->isEmpty() && joinPredList_->treeWalk(visitor))
    return TRUE;

  return FALSE;
}

void QRExtraHub::startElement(void *parser, const char *elementName, const char **atts)
{
  if (!strcmp(elementName, QRList<QRTable>::elemName))
    {
      tableList_ = new (XMLPARSEHEAP) QRList<QRTable>(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, tableList_);
    }
  else if (!strcmp(elementName, QRList<QRJoinPred>::elemName))
    {
      joinPredList_ = new (XMLPARSEHEAP) QRList<QRJoinPred>(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, joinPredList_);
    }
  else
    {
      throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                  elemName, elementName);
    }
}


//
// QRMVMisc
//

QRMVMisc::QRMVMisc(XMLElementPtr parent, AttributeList atts,
                   ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_MVMisc, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    isolationLevel_(0), // @ZX -- what should default be?
    isIncremental_(FALSE),
    isImmediate_(FALSE),
    isFromQuery_(FALSE),
    isUMV_(FALSE)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      if (!strcmp(attrName, "isolationLevel"))
	isolationLevel_ = atoi(iter.getValue());
      else if (!strcmp(attrName, "isIncremental"))
        deserializeBoolAttr(attrName, iter.getValue(), isIncremental_);
      else if (!strcmp(attrName, "isImmediate"))
        deserializeBoolAttr(attrName, iter.getValue(), isImmediate_);
      else if (!strcmp(attrName, "isFromQuery"))
        deserializeBoolAttr(attrName, iter.getValue(), isFromQuery_);
      else if (!strcmp(attrName, "isUMV"))
        deserializeBoolAttr(attrName, iter.getValue(), isUMV_);
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    elemName, attrName);
    }
}

void QRMVMisc::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  // Isolation level matching not implemented.
  //char buffer[20];
  //sprintf(buffer, "isolationLevel='%d' ", isolationLevel_);
  //xml.append(buffer);

  serializeBoolAttr("isIncremental", isIncremental_, xml);
  if (isImmediate_)
    serializeBoolAttr("isImmediate", isImmediate_, xml);
  if (isFromQuery_)
    serializeBoolAttr("isFromQuery", isFromQuery_, xml);
  if (isUMV_)
    serializeBoolAttr("isUMV", isUMV_, xml);
}

//
// QRMVDescriptor
//

QRMVDescriptor::QRMVDescriptor(AttributeList atts, 
			       ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRDescriptor(ET_MVDescriptor, ADD_MEMCHECK_ARGS_PASS(heap)),
    version_(NULL),
    table_(NULL),
    misc_(NULL) //,
    //jbbList_(heap)
{
  // Set parent here so we don't have to use 'this' in initializer
  setParent(this);
  if (*atts)
    throw QRDescriptorException("<%s> should have no attributes; attribute %s specified",  // LCOV_EXCL_LINE :rfi
                                getElementName(), *atts);
}

QRMVDescriptor::~QRMVDescriptor()
{
  deletePtr(version_);
  deletePtr(table_);
  deletePtr(misc_);
  for (CollIndex i = 0; i < jbbList_.entries(); i++) 
    deletePtr(jbbList_[i]);
}

void QRMVDescriptor::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);
}

void QRMVDescriptor::serializeBody(XMLString& xml)
{
  if (version_)
    version_->toXML(xml);
  if (table_)
    table_->toXML(xml);
  if (misc_)
    misc_->toXML(xml);

  for (CollIndex i = 0; i < jbbList_.entries(); i++)
    jbbList_[i]->toXML(xml);
}

NABoolean QRMVDescriptor::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (version_ && version_->treeWalk(visitor))
    return TRUE;
  if (table_ && table_->treeWalk(visitor))
    return TRUE;
  if (misc_ && misc_->treeWalk(visitor))
    return TRUE;

  for (CollIndex i = 0; i < jbbList_.entries(); i++)
    if (jbbList_[i]->treeWalk(visitor))
      return TRUE;

  return FALSE;
}

void QRMVDescriptor::startElement(void *parser, const char *elementName, const char **atts)
{
  if (!strcmp(elementName, QRVersion::elemName))
    {
      version_ = new (XMLPARSEHEAP) QRVersion(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, version_);
    }
  else if (!strcmp(elementName, QRTable::elemName))
    {
      table_ = new (XMLPARSEHEAP) QRTable(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, table_);
    }
  else if (!strcmp(elementName, QRMVMisc::elemName))
    {
      misc_ = new (XMLPARSEHEAP) QRMVMisc(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, misc_);
    }
  else if (!strcmp(elementName, QRJBB::elemName))
    {
      QRJBBPtr jbb = new (XMLPARSEHEAP) QRJBB(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      addJBB(jbb);
      XMLDocument::setCurrentElement(parser, jbb);
    }
  else
    {
      throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                  elemName, elementName);
    }
}


//
// QRMVName
//

QRMVName::QRMVName(XMLElementPtr parent, AttributeList atts,
                   ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_MVName, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    timestamp_(heap),
    fqMVName_(charData_)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      if (!strcmp(attrName, "id"))
        id_ = iter.getValue();
      else if (!strcmp(attrName, "TS"))
        timestamp_ = iter.getValue();
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    elemName, attrName);
    }
}

void QRMVName::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  if (timestamp_.length() > 0)
    xml.append("TS='").append(timestamp_).append("\' ");
}


//
// QRCandidate
//

QRCandidate::QRCandidate(XMLElementPtr parent, AttributeList atts,
                         ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_Candidate, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    isPreferredMatch_(FALSE),
    isIndirectGroupBy_(FALSE),
    statsOnly_(FALSE),
    mvName_(NULL),
    tableList_(NULL),
    joinPredList_(NULL),
    rangePredList_(NULL),
    residualPredList_(NULL),
    groupBy_(NULL),
    outputList_(NULL)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  const char *attrVal;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      attrVal = iter.getValue();
      if (!strcmp(attrName, "statsOnly"))
        deserializeBoolAttr(attrName, attrVal, statsOnly_);
      else if (!strcmp(attrName, "isPreferredMatch"))
        deserializeBoolAttr(attrName, attrVal, isPreferredMatch_);
      else if (!strcmp(attrName, "isIndirectGroupBy"))
        deserializeBoolAttr(attrName, attrVal, isIndirectGroupBy_);
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    elemName, attrName);
    }
}

QRCandidate::~QRCandidate()
{
  deletePtr(mvName_);
  deletePtr(tableList_);
  deletePtr(joinPredList_);
  deletePtr(rangePredList_);
  deletePtr(residualPredList_);
  deletePtr(groupBy_);
  deletePtr(outputList_);
}

void QRCandidate::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  serializeBoolAttr("isPreferredMatch", isPreferredMatch_, xml);
  if (isIndirectGroupBy_)
    serializeBoolAttr("isIndirectGroupBy", isIndirectGroupBy_, xml);
  serializeBoolAttr("statsOnly", statsOnly_, xml);
}

void QRCandidate::serializeBody(XMLString& xml)
{
  if (mvName_)
    mvName_->toXML(xml);
  if (tableList_ && !tableList_->isEmpty())
    tableList_->toXML(xml);
  if (joinPredList_ && !joinPredList_->isEmpty())
    joinPredList_->toXML(xml);
  if (rangePredList_ && !rangePredList_->isEmpty())
    rangePredList_->toXML(xml);
  if (residualPredList_ && !residualPredList_->isEmpty())
    residualPredList_->toXML(xml);
  if (groupBy_)
    groupBy_->toXML(xml);
  if (outputList_ && !outputList_->isEmpty())
    outputList_->toXML(xml);
}

NABoolean QRCandidate::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (mvName_ && mvName_->treeWalk(visitor))
    return TRUE;
  if (tableList_ && !tableList_->isEmpty() && tableList_->treeWalk(visitor))
    return TRUE;
  if (joinPredList_ && !joinPredList_->isEmpty() && joinPredList_->treeWalk(visitor))
    return TRUE;
  if (rangePredList_ && !rangePredList_->isEmpty() && rangePredList_->treeWalk(visitor))
    return TRUE;
  if (residualPredList_ && !residualPredList_->isEmpty() && residualPredList_->treeWalk(visitor))
    return TRUE;
  if (groupBy_ && groupBy_->treeWalk(visitor))
    return TRUE;
  if (outputList_ && !outputList_->isEmpty() && outputList_->treeWalk(visitor))
    return TRUE;

  return FALSE;
}

void QRCandidate::startElement(void *parser, const char *elementName, const char **atts)
{
  if (!strcmp(elementName, QRMVName::elemName))
    {
      mvName_ = new (XMLPARSEHEAP) QRMVName(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, mvName_);
    }
  else if (!strcmp(elementName, QRList<QRTable>::elemName))
    {
      tableList_ = new (XMLPARSEHEAP) QRList<QRTable>(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, tableList_);
    }
  else if (!strcmp(elementName, QRList<QRJoinPred>::elemName))
    {
      joinPredList_ = new (XMLPARSEHEAP) QRList<QRJoinPred>(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, joinPredList_);
    }
  else if (!strcmp(elementName, QRList<QRRangePred>::elemName))
    {
      rangePredList_ = new (XMLPARSEHEAP) QRList<QRRangePred>(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, rangePredList_);
    }
  else if (!strcmp(elementName, QRList<QRExpr>::elemName))
    {
      residualPredList_ = new (XMLPARSEHEAP) QRList<QRExpr>(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, residualPredList_);
    }
  else if (!strcmp(elementName, QRGroupBy::elemName))
    {
      groupBy_ = new (XMLPARSEHEAP) QRGroupBy(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, groupBy_);
    }
  else if (!strcmp(elementName, QRList<QROutput>::elemName))
    {
      outputList_ = new (XMLPARSEHEAP) QRList<QROutput>(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, outputList_);
    }
  else
    {
      throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                  elemName, elementName);
    }
} // QRCandidate::startElement


//
// QRJbbSubset
//

QRJbbSubset::QRJbbSubset(XMLElementPtr parent, AttributeList atts,
			 ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
      : QRElement(ET_JbbSubset, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
        tableList_(NULL),
        candidateList_(NULL),
        hasGroupBy_(FALSE)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  const char *attrVal;
  while (iter.hasNext())
  {
    iter.next();
    attrName = iter.getName();
    attrVal = iter.getValue();
    if (!strcmp(attrName, "hasGroupby"))
      deserializeBoolAttr(attrName, attrVal, hasGroupBy_);
    else if (!strcmp(attrName, "ref"))
      ref_ = iter.getValue();
    else
      throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                  elemName, attrName);
  }
}

void QRJbbSubset::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  serializeBoolAttr("hasGroupby", hasGroupBy_, xml);
}

void QRJbbSubset::serializeBody(XMLString& xml)
{
  if (tableList_ && !tableList_->isEmpty())
    tableList_->toXML(xml);
  if (candidateList_ && !candidateList_->isEmpty())
    candidateList_->toXML(xml);
}

NABoolean QRJbbSubset::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (tableList_ && !tableList_->isEmpty() && tableList_->treeWalk(visitor))
    return TRUE;
  if (candidateList_ && !candidateList_->isEmpty() && candidateList_->treeWalk(visitor))
    return TRUE;

  return FALSE;
}

void QRJbbSubset::startElement(void *parser, const char *elementName, const char **atts)
{
  if (!strcmp(elementName, QRList<QRTable>::elemName))
    {
      tableList_ = new (XMLPARSEHEAP) QRList<QRTable>(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, tableList_);
    }
  else if (!strcmp(elementName, QRList<QRCandidate>::elemName))
    {
      candidateList_ = new (XMLPARSEHEAP) QRList<QRCandidate>(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, candidateList_);
    }
  else
    {
      throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                  elemName, elementName);
    }
}


//
// QRJbbResult
//

QRJbbResult::QRJbbResult(XMLElementPtr parent, AttributeList atts,
                         ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_JbbResult, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    jbbSubsets_(heap),
    infoItems_(heap)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      if (!strcmp(attrName, "ref"))
        ref_ = iter.getValue();
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    elemName, attrName);
    }
}

QRJbbResult::~QRJbbResult()
{
  CollIndex i;
  for (i = 0; i < jbbSubsets_.entries(); i++) 
    deletePtr(jbbSubsets_[i]);
  for (i = 0; i < infoItems_.entries(); i++) 
    deletePtr(infoItems_[i]);
}

void QRJbbResult::serializeBody(XMLString& xml)
{
  CollIndex i;
  for (i = 0; i < jbbSubsets_.entries(); i++)
    jbbSubsets_[i]->toXML(xml);
  for (i = 0; i < infoItems_.entries(); i++)
    infoItems_[i]->toXML(xml);
}

NABoolean QRJbbResult::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  CollIndex i;
  for (i = 0; i < jbbSubsets_.entries(); i++)
    if (jbbSubsets_[i]->treeWalk(visitor))
      return TRUE;
  for (i = 0; i < infoItems_.entries(); i++)
    if (infoItems_[i]->treeWalk(visitor))
      return TRUE;

  return FALSE;
}

void QRJbbResult::startElement(void *parser, const char *elementName, const char **atts)
{
  if (!strcmp(elementName, QRJbbSubset::elemName))
    {
      QRJbbSubsetPtr jbbSubset = new (XMLPARSEHEAP) QRJbbSubset(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      addJbbSubset(jbbSubset);
      XMLDocument::setCurrentElement(parser, jbbSubset);
    }
  else if (!strcmp(elementName, QRInfo::elemName))
    {
      QRInfoPtr info = new (XMLPARSEHEAP) QRInfo(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      addInfoItem(info);
      XMLDocument::setCurrentElement(parser, info);
    }
  else
    throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                elemName, elementName);
}


//
// QRResultDescriptor
//

QRResultDescriptor::~QRResultDescriptor()
{
  deletePtr(version_);
  for (CollIndex i = 0; i < jbbResults_.entries(); i++) 
    deletePtr(jbbResults_[i]);
}

void QRResultDescriptor::serializeBody(XMLString& xml)
{
  if (version_)
    version_->toXML(xml);
  for (CollIndex i = 0; i < jbbResults_.entries(); i++)
    jbbResults_[i]->toXML(xml);
}

NABoolean QRResultDescriptor::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (version_ && version_->treeWalk(visitor))
    return TRUE;
  for (CollIndex i = 0; i < jbbResults_.entries(); i++)
    if (jbbResults_[i]->treeWalk(visitor))
      return TRUE;

  return FALSE;
}

void QRResultDescriptor::startElement(void *parser, const char *elementName, const char **atts)
{
  if (!strcmp(elementName, QRVersion::elemName))
    {
      version_ = new (XMLPARSEHEAP) QRVersion(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, version_);
    }
  else if (!strcmp(elementName, QRJbbResult::elemName))
    {
      QRJbbResultPtr jbbResult = new (XMLPARSEHEAP) QRJbbResult(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      addJbbResult(jbbResult);
      XMLDocument::setCurrentElement(parser, jbbResult);
    }
  else
    {
      throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                  elemName, elementName);
    }
}


//
// QRPublishDescriptor
//

QRPublishDescriptor::QRPublishDescriptor(AttributeList atts,
                                         ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_PublishDescriptor, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
    redefTimestamp_(heap),
    name_(NULL),
    updateList_(heap),
    includeFile_(NULL),
    mvDesc_(NULL),
    mvDescText_(NULL)
{
  // Set parent here so we don't have to use 'this' in initializer.
  setParent(this);

  XMLAttributeIterator iter(atts);
  const char *attrName;
  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      if (!strcmp(attrName, "TS"))
        redefTimestamp_ = iter.getValue();
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",  // LCOV_EXCL_LINE :rfi
                                    elemName, attrName);
    }
}

QRPublishDescriptor::~QRPublishDescriptor()
{
  deletePtr(name_);
  deletePtr(includeFile_);
  deletePtr(mvDesc_);

  if (mvDescText_)
    delete mvDescText_;

  for (CollIndex i=0; i<updateList_.entries(); i++)
    deletePtr(updateList_[i]);
}

NABoolean QRPublishDescriptor::treeWalk(VisitorPtr visitor)
{
  VISIT_THIS(visitor, this);

  if (name_ && name_->treeWalk(visitor))
    return TRUE;

  for (CollIndex i=0; i<updateList_.entries(); i++)
    if (updateList_[i]->treeWalk(visitor))
      return TRUE;

  if (includeFile_ && includeFile_->treeWalk(visitor))
    return TRUE;

  if (mvDesc_ && mvDesc_->treeWalk(visitor))
    return TRUE;

  return FALSE;
}

void QRPublishDescriptor::startElement(void *parser,
                                       const char *elementName,
                                       const char **atts)
{
  if (!strcmp(elementName, QRUpdate::elemName))
    {
      QRUpdatePtr update = new (XMLPARSEHEAP) QRUpdate(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      addUpdate(update);
      XMLDocument::setCurrentElement(parser, update);
    }
  else if (!strcmp(elementName, QRMVName::elemName))
    {
      name_ = new (XMLPARSEHEAP) QRMVName(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, name_);
    }
  else if (!strcmp(elementName, QRInclude::elemName))
    {
      includeFile_ = new (XMLPARSEHEAP) QRInclude(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, includeFile_);
    }
  else if (!strcmp(elementName, QRMVDescriptor::elemName))
    {
      mvDesc_ = new (XMLPARSEHEAP) QRMVDescriptor(CHILD_ELEM_ARGS(XMLPARSEHEAP));
      XMLDocument::setCurrentElement(parser, mvDesc_);
    }
  else
    {
      throw QRDescriptorException("Element %s cannot contain element %s",  // LCOV_EXCL_LINE :rfi
                                  elemName, elementName);
    }
}

void QRPublishDescriptor::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  if (redefTimestamp_.length() > 0)
    xml.append("TS='").append(redefTimestamp_).append("\' ");
}

void QRPublishDescriptor::serializeBody(XMLString& xml)
{
  if (name_)
    name_->toXML(xml);

  for (CollIndex i=0; i<updateList_.entries(); i++)
    updateList_[i]->toXML(xml);

  if (includeFile_)
    includeFile_->toXML(xml);

  if (mvDesc_)
    mvDesc_->toXML(xml);

  if (mvDescText_)
  {
    xml.incrementLevel();
    xml.indent();
    xml.append(mvDescText_->data());
    xml.endLine();
    xml.decrementLevel();
  }
}

void QRPublishDescriptor::initialize(ComPublishMVOperationType opType,
				     const NAString*	       redefTimestamp,
				     NAString* 	               mvDescText, 
				     const NAString*	       mvName,
				     NABoolean		       hasIgnoreChanges,
				     const NAString*	       refreshTimestamp,
				     const NAString*	       newName,
				     NAMemory*		       heap)
{
  setRedefTimestamp(*redefTimestamp);

  QRMVNamePtr nameElement = new(heap) QRMVName(ADD_MEMCHECK_ARGS(heap));
  nameElement->setMVName(*mvName);
  setName(nameElement);

  switch(opType)
  {
    case COM_PUBLISH_MV_CREATE_AND_REFRESH:
      addUpdate(createUpdateForRefresh(refreshTimestamp, heap));
      // Fall through to CREATE.
    case COM_PUBLISH_MV_CREATE:
      if (hasIgnoreChanges)
        addUpdate(createUpdateForAlter(hasIgnoreChanges, heap));
      setMVText(mvDescText, heap);
      break;

    case COM_PUBLISH_MV_RENAME:
      addUpdate(createUpdateForRename(newName, heap));
      break;

    case COM_PUBLISH_MV_ALTER_IGNORE_CHANGES:
      addUpdate(createUpdateForAlter(hasIgnoreChanges, heap));
      break;

    case COM_PUBLISH_MV_TOUCH:
      // Nothing to do here.
      break;

    case COM_PUBLISH_MV_DROP:
      addUpdate(createUpdateForDrop(heap));
      break;

    case COM_PUBLISH_MV_REPUBLISH:
      setMVText(mvDescText, heap);
      addUpdate(createUpdateForAlter(hasIgnoreChanges, heap));
      addUpdate(createUpdateForRefresh(refreshTimestamp, heap));
      break;

    case COM_PUBLISH_MV_REFRESH:
      addUpdate(createUpdateForRefresh(refreshTimestamp, heap));
      break;

    case COM_PUBLISH_MV_REFRESH_RECOMPUTE:
      addUpdate(createUpdateForRecompute(refreshTimestamp, heap));
      break;

    case COM_PUBLISH_MV_UNKNOWN:
    default:
      assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,  // LCOV_EXCL_LINE :rfi
                        FALSE, QRDescriptorException,
			"Unhandled MV operation type");
      break;
  }
}

QRUpdatePtr QRPublishDescriptor::createUpdateForAlter(NABoolean hasIgnoreChanges, NAMemory* heap)
{
  QRUpdatePtr update = new (heap) QRUpdate(QRUpdate::ALTER, ADD_MEMCHECK_ARGS(heap));
  update->setIgnoreChanges(hasIgnoreChanges);
  return update;
}

QRUpdatePtr QRPublishDescriptor::createUpdateForRefresh(const NAString* refreshTimestamp, NAMemory* heap)
{
  QRUpdatePtr update = new (heap) QRUpdate(QRUpdate::REFRESH, ADD_MEMCHECK_ARGS(heap));
  update->setRefreshTimestamp(*refreshTimestamp);
  return update;
}

QRUpdatePtr QRPublishDescriptor::createUpdateForRecompute(const NAString* refreshTimestamp, NAMemory* heap)
{
  QRUpdatePtr update = new (heap) QRUpdate(QRUpdate::RECOMPUTE, ADD_MEMCHECK_ARGS(heap));
  update->setRefreshTimestamp(*refreshTimestamp);
  return update;
}

QRUpdatePtr QRPublishDescriptor::createUpdateForDrop(NAMemory* heap)
{
  QRUpdatePtr update = new (heap) QRUpdate(QRUpdate::DROP, ADD_MEMCHECK_ARGS(heap));
  return update;
}

QRUpdatePtr QRPublishDescriptor::createUpdateForRename(const NAString* newName, NAMemory* heap)
{
  QRUpdatePtr update = new (heap) QRUpdate(QRUpdate::RENAME, ADD_MEMCHECK_ARGS(heap));
  update->setNewName(*newName);
  return update;
}

//
// QRUpdate
//

// The order of these names must match that of the UpdateType enum.
const char* const QRUpdate::UpdateTypeNames_[] = 
  { "Non-Init", "Alter", "Default", "Drop", "Recompute", "Refresh", "Rename" };

QRUpdate::QRUpdate(XMLElementPtr parent,
                   AttributeList atts,
                   ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_Update, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    type_(NON_INIT),
    timestamp_(heap),
    hasIgnoreChanges_(FALSE),
    newName_(charData_),
    defaultName_(heap),
    defaultValue_(heap)
{
  XMLAttributeIterator iter(atts);
  const char *attrName;
  const char *attrVal;

  while (iter.hasNext())
    {
      iter.next();
      attrName = iter.getName();
      attrVal = iter.getValue();

      if (!strcmp(attrName, "TS")) // refresh timestamp; only for op=Refresh
        timestamp_ = attrVal;  //@ZX -- probably need to convert to INT64
      else if (!strcmp(attrName, "attr")) // default attribute name for op=Default
        defaultName_ = attrVal;   
      else if (!strcmp(attrName, "value")) // default attribute value for op=Default
        defaultValue_ = attrVal;  
      else if (!strcmp(attrName, "op"))
      {
	if      (!strcmp(attrVal, "Refresh"))
          type_ = REFRESH;
        else if (!strcmp(attrVal, "Recompute"))
          type_ = RECOMPUTE;
        else if (!strcmp(attrVal, "Drop"))
          type_ = DROP;
        else if (!strcmp(attrVal, "Alter"))
          type_ = ALTER;
        else if (!strcmp(attrVal, "Rename"))
          type_ = RENAME;
        else if (!strcmp(attrVal, "Default"))
          type_ = DEFAULT;
        else
          throw QRDescriptorException("Invalid Update type: %s", attrVal);
      }
      else if (!strcmp(attrName, "hasIgnoreChanges"))
        deserializeBoolAttr(attrName, attrVal, hasIgnoreChanges_);
      else
        throw QRDescriptorException("Invalid attribute specified for element %s: %s",
                                    elemName, attrName);
    }
}

QRUpdate::QRUpdate(UpdateType updateType,
                   ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
  : QRElement(ET_Update, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
    type_(updateType),
    timestamp_(heap),
    hasIgnoreChanges_(FALSE),
    newName_(charData_)
{
}

void QRUpdate::serializeAttrs(XMLString& xml)
{
  QRElement::serializeAttrs(xml);

  assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                    type_ != NON_INIT, QRDescriptorException,
                    "Op type has not been set for <Update>");
  xml.append("op='").append(UpdateTypeNames_[type_]).append("' ");

  switch (type_)
  {
    case ALTER:
      serializeBoolAttr("hasIgnoreChanges", hasIgnoreChanges_, xml);
      break;

    case REFRESH:
    case RECOMPUTE:
      xml.append("TS='").append(timestamp_).append("' ");
      break;

    case DEFAULT:
      assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                        defaultName_.length() > 0, QRDescriptorException,
                        "Attribute name not available for default operation");
      assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                        defaultValue_.length() > 0, QRDescriptorException,
                        "Attribute value not available for default operation");

      xml.append("attr='").append(defaultName_).append("' ");
      xml.append("value='").append(defaultValue_).append( "' ");
      break;

    case DROP:
      // Nothing to do here.
      break;
  }
}
