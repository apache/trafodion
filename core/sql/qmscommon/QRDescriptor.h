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

#ifndef _QRDESCRIPTOR_H_
#define _QRDESCRIPTOR_H_

#include <stdlib.h>
#include <math.h>
#include "Platform.h"
#include "XMLUtil.h"
#include "QRSharedPtr.h"
#include "NABasicObject.h"
#include "NAString.h"
#include "Collections.h"
#include "OperTypeEnum.h"
#include "ComSmallDefs.h"
#include "str.h"
#include "charinfo.h"
#include "SharedPtrCollections.h"
#include "logmxevent.h"
#include "QRExprElement.h"

class ItemExpr;
class Scan;
class JBB;
class GroupByAgg;
typedef NAHashDictionary<const NAString, Scan> TableNameScanHash;

/**
 * \file
 * Contains class definitions for the entire Query Rewrite descriptor
 * hierarchy, which represents the three types of descriptor (Query, MV, and
 * Result) used. Each class represents an element in the XML rendering of the
 * descriptor. The abstract class QRElement is the root of the hierarchy, and
 * the concrete element classes inherit from it. Besides the hierarchy, this
 * file includes a class representing the XML document as a whole, and ancillary
 * classes for exceptions and strings of XML text.
 */

// itoa is a nonstandard function not available with c89 compiler. This define
// was borrowed from sqlutils\inc\ds.h.
#ifndef itoa
#define itoa(intval, bufptr, radix) \
(((Int32)radix == 10)?(sprintf(bufptr,"%u",intval),bufptr):NULL)
#endif

/////////////////////////////////////////////////////////////////
//                      Descriptor Classes                     //
/////////////////////////////////////////////////////////////////

class QRDescriptorException;
class XMLString;
class Visitor;
class   AggregateFinderVisitor;
class   ElementFinderVisitor;
class   StdDevFinderVisitor;
class RangeSpec;

class QRElement;
class   QRDescriptor;
class     QRQueryDescriptor;
class     QRMVDescriptor;
class   QRResultDescriptor;
class   QRElementList;
class     QRJBBCList;
class     QRJoinPred;
class     QRPrimaryGroupBy;
class     QRDependentGroupBy;
class     QRMinimalGroupBy;
class     QROutput;
class     QRKey;
class   QRQueryMisc;
class   QRForcedMVs;
class   QRJBB;
class   QRInfo;
class   QRHub;
class   QRExtraHub;
class   QRTable;
class   QROperator;
class   QRRangePred;
class   QRRangeOperator;
class     QROpEQ;
class     QROpBT;
class     QROpInequality;
class       QROpLS;
class       QROpLE;
class       QROpGT;
class       QROpGE;
class   QRNullVal;
class   QRGroupBy;
class   QRParameter;
class   QRExpr;
class   QRExplicitExpr;
class     QRColumn;
class     QRMVColumn;
class     QRBinaryOper;
class     QRUnaryOper;
class     QRFunction;
class     QRScalarValue;
class       QRNumericVal;
class       QRStringVal;
class       QRWStringVal;
class       QRFloatVal;
class   QRMVMisc;
class   QRJbbResult;
class   QRJbbSubset;
class   QRCandidate;
class   QRMVName;
template <class T> class   QRList;
class   QRVersion;
class   QRUpdate;
class   QRInclude;
class   QRPublishDescriptor;

#ifdef _MEMSHAREDPTR
typedef QRIntrusiveSharedPtr<Visitor> VisitorPtr;
typedef QRIntrusiveSharedPtr<AggregateFinderVisitor> AggregateFinderVisitorPtr;
typedef QRIntrusiveSharedPtr<ElementFinderVisitor> ElementFinderVisitorPtr;
typedef QRIntrusiveSharedPtr<StdDevFinderVisitor> StdDevFinderVisitorPtr;
typedef QRIntrusiveSharedPtr<QRElement> QRElementPtr;
typedef QRIntrusiveSharedPtr<QRDescriptor> QRDescriptorPtr;
typedef QRIntrusiveSharedPtr<QRQueryDescriptor> QRQueryDescriptorPtr;
typedef QRIntrusiveSharedPtr<QRMVDescriptor> QRMVDescriptorPtr;
typedef QRIntrusiveSharedPtr<QRResultDescriptor> QRResultDescriptorPtr;
typedef QRIntrusiveSharedPtr<QRQueryMisc> QRQueryMiscPtr;
typedef QRIntrusiveSharedPtr<QRJBB> QRJBBPtr;
typedef QRIntrusiveSharedPtr<QRInfo> QRInfoPtr;
typedef QRIntrusiveSharedPtr<QRHub> QRHubPtr;
typedef QRIntrusiveSharedPtr<QRExtraHub> QRExtraHubPtr;
typedef QRIntrusiveSharedPtr<QROutput> QROutputPtr;
typedef QRIntrusiveSharedPtr<QRJBBCList> QRJBBCListPtr;
typedef QRIntrusiveSharedPtr<QRTable> QRTablePtr;
typedef QRIntrusiveSharedPtr<QRKey> QRKeyPtr;
typedef QRIntrusiveSharedPtr<QRForcedMVs> QRForcedMVsPtr;
typedef QRIntrusiveSharedPtr<QROperator> QROperatorPtr;
typedef QRIntrusiveSharedPtr<QRJoinPred> QRJoinPredPtr;
typedef QRIntrusiveSharedPtr<QRRangePred> QRRangePredPtr;
typedef QRIntrusiveSharedPtr<QRRangeOperator> QRRangeOperatorPtr;
typedef QRIntrusiveSharedPtr<QROpInequality> QROpInequalityPtr;
typedef QRIntrusiveSharedPtr<QROpEQ> QROpEQPtr;
typedef QRIntrusiveSharedPtr<QROpLS> QROpLSPtr;
typedef QRIntrusiveSharedPtr<QROpLE> QROpLEPtr;
typedef QRIntrusiveSharedPtr<QROpGT> QROpGTPtr;
typedef QRIntrusiveSharedPtr<QROpGE> QROpGEPtr;
typedef QRIntrusiveSharedPtr<QROpBT> QROpBTPtr;
typedef QRIntrusiveSharedPtr<QRScalarValue> QRScalarValuePtr;
typedef QRIntrusiveSharedPtr<QRNumericVal> QRNumericValPtr;
typedef QRIntrusiveSharedPtr<QRStringVal> QRStringValPtr;
typedef QRIntrusiveSharedPtr<QRWStringVal> QRWStringValPtr;
typedef QRIntrusiveSharedPtr<QRFloatVal> QRFloatValPtr;
typedef QRIntrusiveSharedPtr<QRNullVal> QRNullValPtr;
typedef QRIntrusiveSharedPtr<QRGroupBy> QRGroupByPtr;
typedef QRIntrusiveSharedPtr<QRPrimaryGroupBy> QRPrimaryGroupByPtr;
typedef QRIntrusiveSharedPtr<QRDependentGroupBy> QRDependentGroupByPtr;
typedef QRIntrusiveSharedPtr<QRMinimalGroupBy> QRMinimalGroupByPtr;
typedef QRIntrusiveSharedPtr<QRColumn> QRColumnPtr;
typedef QRIntrusiveSharedPtr<QRExpr> QRExprPtr;
typedef QRIntrusiveSharedPtr<QRExplicitExpr> QRExplicitExprPtr;
typedef QRIntrusiveSharedPtr<QRBinaryOper> QRBinaryOperPtr;
typedef QRIntrusiveSharedPtr<QRUnaryOper> QRUnaryOperPtr;
typedef QRIntrusiveSharedPtr<QRFunction> QRFunctionPtr;
typedef QRIntrusiveSharedPtr<QRParameter> QRParameterPtr;
typedef QRIntrusiveSharedPtr<QRMVColumn> QRMVColumnPtr;
typedef QRIntrusiveSharedPtr<QRMVMisc> QRMVMiscPtr;
typedef QRIntrusiveSharedPtr<QRJbbResult> QRJbbResultPtr;
typedef QRIntrusiveSharedPtr<QRJbbSubset> QRJbbSubsetPtr;
typedef QRIntrusiveSharedPtr<QRCandidate> QRCandidatePtr;
typedef QRIntrusiveSharedPtr<QRMVName> QRMVNamePtr;
typedef QRIntrusiveSharedPtr<QRVersion> QRVersionPtr;
typedef QRIntrusiveSharedPtr<QRUpdate> QRUpdatePtr;
typedef QRIntrusiveSharedPtr<QRInclude> QRIncludePtr;
typedef QRIntrusiveSharedPtr<QRPublishDescriptor> QRPublishDescriptorPtr; 
typedef QRIntrusiveSharedPtr< QRList<QROutput> > QROutputListPtr;
typedef QRIntrusiveSharedPtr< QRList<QRJoinPred> > QRJoinPredListPtr;
typedef QRIntrusiveSharedPtr< QRList<QRRangePred> > QRRangePredListPtr;
typedef QRIntrusiveSharedPtr< QRList<QRExpr> > QRResidualPredListPtr;
typedef QRIntrusiveSharedPtr< QRList<QRTable> > QRTableListPtr;
typedef QRIntrusiveSharedPtr< QRList<QRCandidate> > QRCandidateListPtr;
#else
typedef Visitor* VisitorPtr;
typedef AggregateFinderVisitor* AggregateFinderVisitorPtr;
typedef ElementFinderVisitor* ElementFinderVisitorPtr;
typedef StdDevFinderVisitor* StdDevFinderVisitorPtr;
typedef QRElement* QRElementPtr;
typedef QRDescriptor* QRDescriptorPtr;
typedef QRQueryDescriptor* QRQueryDescriptorPtr;
typedef QRMVDescriptor* QRMVDescriptorPtr;
typedef QRResultDescriptor* QRResultDescriptorPtr;
typedef QRQueryMisc* QRQueryMiscPtr;
typedef QRJBB* QRJBBPtr;
typedef QRInfo* QRInfoPtr;
typedef QRHub* QRHubPtr;
typedef QRExtraHub* QRExtraHubPtr;
typedef QROutput* QROutputPtr;
typedef QRJBBCList* QRJBBCListPtr;
typedef QRTable* QRTablePtr;
typedef QRKey* QRKeyPtr;
typedef QRForcedMVs* QRForcedMVsPtr;
typedef QROperator* QROperatorPtr;
typedef QRJoinPred* QRJoinPredPtr;
typedef QRRangePred* QRRangePredPtr;
typedef QRRangeOperator* QRRangeOperatorPtr;
typedef QROpInequality* QROpInequalityPtr;
typedef QROpEQ* QROpEQPtr;
typedef QROpLS* QROpLSPtr;
typedef QROpLE* QROpLEPtr;
typedef QROpGT* QROpGTPtr;
typedef QROpGE* QROpGEPtr;
typedef QROpBT* QROpBTPtr;
typedef QRScalarValue* QRScalarValuePtr;
typedef QRNumericVal* QRNumericValPtr;
typedef QRStringVal* QRStringValPtr;
typedef QRWStringVal* QRWStringValPtr;
typedef QRFloatVal* QRFloatValPtr;
typedef QRNullVal* QRNullValPtr;
typedef QRGroupBy* QRGroupByPtr;
typedef QRPrimaryGroupBy* QRPrimaryGroupByPtr;
typedef QRDependentGroupBy* QRDependentGroupByPtr;
typedef QRMinimalGroupBy* QRMinimalGroupByPtr;
typedef QRColumn* QRColumnPtr;
typedef QRExpr* QRExprPtr;
typedef QRExplicitExpr* QRExplicitExprPtr;
typedef QRBinaryOper* QRBinaryOperPtr;
typedef QRUnaryOper* QRUnaryOperPtr;
typedef QRFunction* QRFunctionPtr;
typedef QRParameter* QRParameterPtr;
typedef QRMVColumn* QRMVColumnPtr;
typedef QRMVMisc* QRMVMiscPtr;
typedef QRJbbResult* QRJbbResultPtr;
typedef QRJbbSubset* QRJbbSubsetPtr;
typedef QRCandidate* QRCandidatePtr;
typedef QRMVName* QRMVNamePtr;
typedef QRVersion* QRVersionPtr;
typedef QRUpdate* QRUpdatePtr;
typedef QRInclude* QRIncludePtr;
typedef QRPublishDescriptor* QRPublishDescriptorPtr;
typedef QRList<QROutput>* QROutputListPtr;
typedef QRList<QRJoinPred>* QRJoinPredListPtr;
typedef QRList<QRRangePred>* QRRangePredListPtr;
typedef QRList<QRExpr>* QRResidualPredListPtr;
typedef QRList<QRTable>* QRTableListPtr;
typedef QRList<QRCandidate>* QRCandidateListPtr;
#endif

typedef NAPtrList<QRElementPtr>                                 ElementPtrList;
typedef NAPtrList<QRRangePredPtr>				RangePredPtrList;
typedef NAPtrList<QRExprPtr>				        ResidualPredPtrList;
typedef SharedPtrValueHash<const NAString, QRElement>		QRElementHash;
typedef SharedPtrValueHashIterator<const NAString, QRElement>	QRElementHashIterator;
typedef SharedPtrValueHash<const NAString, QRExplicitExpr>	subExpressionRewriteHash;
typedef SharedPtrValueHashIterator<const NAString, QRExplicitExpr>  subExpressionRewriteHashIterator;

enum MvqrRewriteLevel {
	MRL_OFF = 0,  // MVQR is OFF.
	MRL_FRESH,    // Only fresh MVs are considered (Immediate for now)
	MRL_STALE,    // Any enabled MV that is not over MV_AGE since last refresh
	MRL_OLD,      // Any MV (except UMVs) with ENABLE QUERY REWRITE.
	MRL_UMVS      // Any MV including UMVs with ENABLE QUERY REWRITE.
};

/**
 * Exception thrown when an error occurs in processing or generating an XML
 * message involving Query Rewrite descriptors.
 */
class QRDescriptorException : public QRException
{
  public:
    /**
     * Creates an exception with text consisting of the passed template filled in
     * with the values of the other arguments.
     *
     * @param[in] msgTemplate Template for construction of the full message;
     *                        contains printf-style placeholders for arguments,
     *                        passed as part of a variable argument list.
     * @param[in] ... Variable argument list, consisting of a value for each
     *                placeholder in the message template.
     */
    QRDescriptorException(const char *msgTemplate ...)
      : QRException()
    {
      qrBuildMessage(msgTemplate, msgBuffer_);
      SQLMXLoggingArea::logSQLMXAssertionFailureEvent(__FILE__, __LINE__, getMessage()); 
    }

    virtual ~QRDescriptorException()
    {}

}; //QRDescriptorException

class QRElementMapper : public XMLElementMapper
{
  public:
    virtual XMLElementPtr operator()(void *parser,
                                     char *elementName,
                                     AttributeList atts);
};

/**
 * Class Visitor implements the Visitor design pattern.
 * Visitor is a pure virtual class that defines the interface for concrete
 * Visitor classes. Each concrete class will implement an operation for which 
 * we need a tree walk of the Descriptor object tree.
 */
class Visitor  : public NAIntrusiveSharedPtrObject
{
public:
  Visitor(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap))
    : NAIntrusiveSharedPtrObject(ADD_MEMCHECK_ARGS_PASS(heap))
  {}

  // The possible results of a visit.
  enum VisitResult {
    VR_Continue, // Continue tree walking.
    VR_Skip,     // Continue tree walking, but skip this object's children.
    VR_Stop      // Stop tree walking and return.
  };

  /**
   * Visit the visitor object from the caller element node.
   * @param caller 
   * @return VR_Continue - Continue tree walking.
   *         VR_Skip     - Continue tree walking, but skip this object's children.
   *         VR_Stop     - Stop tree walking and return.
   */
  virtual VisitResult visit(QRElementPtr caller) = 0;
};

// This macro should be placed as the beginning of every treeWalk() method.
// It implements the correct behaviour for VR_Stop and VR_Skip.
// When visit() returns VR_Continue, execution will continue to sub-elements.
#define VISIT_THIS(visitor, thisObj)                      \
  Visitor::VisitResult result = visitor->visit(thisObj);  \
  if      (result == Visitor::VR_Stop) return TRUE;       \
  else if (result == Visitor::VR_Skip) return FALSE;

/**
 * Class that implements the interface specified by the Visitor abstract class
 * and is used to traverse the tree structure of an explicit expression to see
 * if that expression contains an aggregate function reference. Upon completion
 * of the traversal, which is carried out by passing a pointer to an instance of
 * this class to an element's treeWalk() function, the result (whether or not an
 * aggregate function reference was found) is available via the foundAggregate()
 * function.
 */
class AggregateFinderVisitor : public Visitor
{
  public: 
    AggregateFinderVisitor(ADD_MEMCHECK_ARGS_DECL(CollHeap* heap),
                           NABoolean findAll=FALSE)
      : Visitor(ADD_MEMCHECK_ARGS_PASS(heap)),
        foundAggregate_(FALSE), findAll_(findAll), aggregatesFound_(heap)
    {}

    virtual VisitResult visit(QRElementPtr caller);
  
    NABoolean foundAggregate() const
    {
      return foundAggregate_;
    }

    const ElementPtrList& getAggregatesFound() const
    {
      return aggregatesFound_;
    }

private:
  NABoolean foundAggregate_;
  NABoolean findAll_;
  ElementPtrList aggregatesFound_;
}; // class AggregateFinderVisitor

/**
 * Visitor class that looks for elements of any of the given types passed to
 * the ctor, and when found, adds them to a list. A reference to the list is
 * accessible via the #getElementsFound() member function.
 */
class ElementFinderVisitor : public Visitor
{
  public: 
    /**
     * Creates an element finder that looks for a single specific element type.
     * @param elemType The element type to look for.
     * @param useRefedElem If TRUE, when an element is found, add the referenced
     *                     element to the list instead of the element itself.
     * @param heap Heap to use.
     */
    ElementFinderVisitor(ElementType elemType,
                         NABoolean useRefedElem = TRUE,
                         ADD_MEMCHECK_ARGS_DECL(CollHeap* heap = 0))
      : Visitor(ADD_MEMCHECK_ARGS_PASS(heap)),
        targetTypes_(heap),
        elementsFound_(heap),
        useRefedElem_(useRefedElem)
    {
      targetTypes_.insert(elemType);
    }

    /**
     * Creates an element finder that looks for any of a list of element types.
     * @param targetTypes List of element types to search for.
     * @param useRefedElem If TRUE, when an element is found, add the referenced
     *                     element to the list instead of the element itself.
     * @param heap Heap to use.
     */
    ElementFinderVisitor(NAList<ElementType>& targetTypes,
                         NABoolean useRefedElem = TRUE,
                         ADD_MEMCHECK_ARGS_DECL(CollHeap* heap = 0))
      : Visitor(ADD_MEMCHECK_ARGS_PASS(heap)),
        targetTypes_(heap),
        elementsFound_(heap),
        useRefedElem_(useRefedElem)
    {
      targetTypes_.insert(targetTypes);
    }

    virtual VisitResult visit(QRElementPtr caller);
  
    const ElementPtrList& getElementsFound() const
    {
      return elementsFound_;
    }

private:
  NAList<ElementType> targetTypes_;
  ElementPtrList elementsFound_;
  NABoolean useRefedElem_;
}; // class ElementFinderVisitor

/**
 * Abstract superclass of all classes representing XML elements used in Query
 * Rewrite descriptors. Each instance of an element in a descriptor is represented
 * by an object, and the structure of the XML document is maintained by a link to
 * the parent of each object.
 */
class QRElement : public XMLElement
{
  public:
    // Keep the following enum and array of names in synch.
    /**
     * Enumeration of possible values for the \c result attribute.
     */
    enum ExprResult { FIRST_EXPR_RESULT,
                      Outside=FIRST_EXPR_RESULT, /**< outside the JBBSubset */
                      Provided,                  /**< applied by the candidate MV */
                      NotProvided,               /**< needs to be applied on the MV */
                      INVALID_EXPR_RESULT };
    static const char* const ExprResultNames[];

    /**
     * Maps the string values of a \c result attribute to the corresponding
     * enum value stored as part of the element object.
     *
     * @param[in] resultString Value of a \c result attribute.
     * @return ExprResult enum value corresponding to the attribute value.
     */
    static ExprResult encodeResult(const char* resultString);

    /**
    * Compares two instances of QRElement that define the #getSortName() function.
    * This can be used with an \c opt_qsort of an array that contains more than
    * one element type (as long as each of those element types defines 
    * \c getSortName().
    *
    * @param *p1 Pointer to the first object to compare.
    * @param *p2 Pointer to the second object to compare.
    * @return 0 if equal, -1 if first object is less than second, 1 if first
    *         object is greater than second.
    */
    static Int32 cmpQRElement(const void *p1, const void *p2);

    /**
     * Replaces any special characters occurring in the passed attribute value
     * with the appropriate XML entity reference. The delimiter used (' or ")
     * must be passed, and is treated as a special character in the string.
     * For example, the attribute value ab&cd'ef<, if intended to be delimited
     * with single quotes, will be changed to ab&amp;cd&apos;ef&lt;.
     *
     * @param str The attribute value, without the delimiters.
     * @param attrDelim The delimiter to be used for the attribute, either ' or ".
     * @return A reference to the passed string, the contents of which will have
     *         been modified if it contained any special characters.
     */
    static NAString& addEntityRefs(NAString& str, char attrDelim);

    virtual ~QRElement()
      {}

    NABoolean operator==(const QRElement& other) const
    {
      // Compare the address of the element.
      return this == &other;
    }

    /**
     * Indicates whether this class can be a member of a JBBCList.
     *
     * @return \c TRUE if the element represented by this class can be a join
     *         backbone child (JBBC), \c FALSE otherwise.
     */
    virtual NABoolean canBeJbbc()
      {
        return FALSE;  // overridden to be TRUE for Table, JBB, Operator
      }

    /**
     * Indicates whether this class can be contained in a <JoinPred> element.
     *
     * @return \c TRUE if the element represented by this class can be part of
     *         the equality set in a join predicate, \c FALSE otherwise.
     */
    virtual NABoolean canBeInJoinPred()
      {
        return FALSE;  // overridden to be TRUE for Column, MVColumn, Expr, JoinPred
      }

    /**
     * Indicates whether this class represents one of the elements that can be
     * the root element of a \c QRExpr.
     *
     * @return \c TRUE if the element represented by this class can be the root
     *         element of a \c QRExpr, \c FALSE otherwise.
     */
    virtual NABoolean canBeExprRoot() const
      {
        return FALSE;  // overridden to be TRUE for Function, BinaryOper,
                       //    and UnaryOper
      }

    /**
     * Indicates whether this class represents one of the elements that can be
     * an output item.
     *
     * @return \c TRUE if the element represented by this class can be an
     *         output item.
     */
    virtual NABoolean canBeOutputItem() const
      {
        return FALSE;  // overridden to be TRUE for Column, Expr, MVColumn
      }

    /**
     * Indicates whether this class represents one of the elements that can be
     * a <code>Group By</code> item.
     *
     * @return \c TRUE if the element represented by this class can be an
     *         <code>Group By</code> item.
     */
    virtual NABoolean canBeGroupByItem() const
      {
        return FALSE;  // overridden to be TRUE for Column, Expr, MVColumn
      }

    /**
     * Indicates whether an element belongs to the extra-hub as opposed to the
     * hub. This is relevant only to QTable, QRColumn, and QRExpr, and is
     * redefined by those subclasses. It is needed at the superclass level
     * because in some cases it must be applied to members of collections of
     * QRElements that are either columns or expressions.
     *
     * @return TRUE if part of the extra-hub, false otherwise.
     */
    virtual NABoolean isExtraHub()
    {
      return FALSE;
    }

    /**
     * Returns the string value used as the basis for comparing this element
     * with other elements when creating a sorted list; for example, when
     * ordering the components of a \c &lt;JoinPred&gt; element.
     *
     * @return The value used for placing this element within a sorted list of
     *         elements.
     */
    virtual const NAString& getSortName() const
      {
        throw QRDescriptorException("getSortName() is not defined for Element %s",
                                    getElementName());
      }

    void serializeBoolAttr(const char* attrName,
                           NABoolean attrVal,
                           XMLString& xml);

    void deserializeBoolAttr(const char* attrName,
                             const char* attrVal,
                             NABoolean& attr);

    /**
     * Returns the unique identifier for this element
     * @return The element's unique id.
     */
    const NAString& getID() const
      {
        return id_;
      }

    /**
     * Extracts the numeric part of this element's id and returns it as the
     * function value. An id consists of an initial single character designating
     * the element type, followed by an integral value that is unique within
     * elements of that type in the document.
     * @return Numeric part of the unique id.
     */
    NumericID getIDNum() const
      {
        return (id_.length() < 2 ? 0 : atoi(id_.data()+1));
      }

    /**
     * Return the first character of ID strings used by an element.
     * @return the first char of the ID for subclass elements.
     */
    virtual char getIDFirstChar()
      {
        assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                          FALSE, QRDescriptorException, 
                          "QRElement subclass must supply a definition for getIDFirstChar() "
                          "to use the id attribute.");
        return ' ';
      }

    /**
     * Returns the id of the element referenced by this element.
     * @return The referenced element's id.
     */
    const NAString& getRef() const
      {
        return ref_;
      }

    /**
     * Returns the numeric part of the id of the element referenced by this element.
     * @return The referenced element's numeric id.
     */
    NumericID getRefNum() const
      {
        return (ref_.length() < 2 ? 0 : atoi(ref_.data()+1));
      }

    /**
     * Returns the first character of the ref attribute if present,
     * blank otherwise.
     * @return The first char of the ref attribute.
     */
    char getRefFirstChar() const
      {
        return (ref_.length() < 2 ? ' ' : *ref_.data());
      }

    /**
     * Sets the unique id to be used for this element. The numeric
     * value passed is appended to the single character designating the element
     * type, to form the id.
     * @param idNum Numeric component of the id.
     */
    virtual void setID(NumericID idNum)
      {
        char idStr[13];
        idStr[0] = getIDFirstChar();
        str_itoa(idNum, idStr+1);
        id_ = idStr;
      }

    /**
     * Associates this object with an existing element.
     * This method is typically used for the result descriptor.
     * @param ref ID of the referenced element.
     */
    void setRef(const NAString& ref)
      {
        ref_ = ref;
      }

    /**
     * Associates this object with an existing element.
     * This method is typically used for linking references to the same 
     * element in the query\MV descriptor
     * @param ref Numeric part of the ID of the referenced element.
     */
    void setRefFromInt(NumericID refNum, char otherFirstChar = 0)
      {
        char idStr[13];
        idStr[0] = (otherFirstChar==0 ? getIDFirstChar() : otherFirstChar);
        str_itoa(refNum, idStr+1);
        ref_ = idStr;
      }

    /**
     * Handles the callback from the parser when the name of the element's 
     * character data is being parsed.
     * @param[in] parser Pointer to the instance of the parser being used.
     * @param[in] data   The character data.
     * @param[in] len    The length of the character data.
     */
    virtual void charData(void *parser, const char *data, Int32 len)
      {
        stripWhitespace(data, len);
        charData_.append(data, len);
      }

    virtual void serializeBody(XMLString& xml)
      {
        xml.appendCharData(charData_);
      }

    /**
     * The generic implementation of the tree walk using the Visitor design pattern.
     * Should be overridden by any element with sub-elements.
     * @param visitor 
     * @return TRUE if the tree walk can be stopped, FALSE otherwise.
     */
    virtual NABoolean treeWalk(VisitorPtr visitor)
    {
      VISIT_THIS(visitor, this);
      return FALSE;
    }

    /**
     * Get the element type as an enum.
     * Its virtual because its actually defined (as a pure virtual method) in XMLElement.
     * @return 
     */
    virtual enum ElementType getElementType() const
    {
      return eType_;
    }

    void setReferencedElement(QRElementPtr referenced)
    {
      referencedElement_ = referenced;
    }

    QRElementPtr getReferencedElement() 
    {
      return referencedElement_;
    }

    const QRElementPtr getReferencedElement() const
    {
      return referencedElement_;
    }

    #define THROW_DOWNCAST_EXCEPTION assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL, FALSE, QRLogicException, "Down cast to wrong element type.");

    // Methods for easy downcasting to specific classes.
    // Each method is overridden by its specific class, to return 'this'.
    virtual QRColumnPtr     downCastToQRColumn()     { THROW_DOWNCAST_EXCEPTION }
    virtual QRMVColumnPtr   downCastToQRMVColumn()   { THROW_DOWNCAST_EXCEPTION }
    virtual QRTablePtr      downCastToQRTable()      { THROW_DOWNCAST_EXCEPTION }
    virtual QRJoinPredPtr   downCastToQRJoinPred()   { THROW_DOWNCAST_EXCEPTION }
    virtual QRExprPtr       downCastToQRExpr()       { THROW_DOWNCAST_EXCEPTION }
    virtual QROutputPtr     downCastToQROutput()     { THROW_DOWNCAST_EXCEPTION }
    virtual QRRangePredPtr  downCastToQRRangePred()  { THROW_DOWNCAST_EXCEPTION }
    virtual QRFunctionPtr   downCastToQRFunction()   { THROW_DOWNCAST_EXCEPTION }

    virtual QRColumnPtr     getFirstColumn() { return NULL; }

    const QRElementPtr getParentElement() const
    {
      return static_cast<const QRElementPtr>(getParent());
    }

  protected:
    /**
     * Constructs an object representing an XML element used in the Query Rewrite
     * descriptor classes. If the element is not the outermost element in the
     * document, \c parent should be non-null. If dynamically allocated, which is
     * currently always the case for element objects in this code, the file and
     * line number where the constructor was invoked may be passed, and are used
     * for logging memory management messages, if memory checking is enabled (by
     * conditional compilation).
     *
     * @param[in] parent Pointer to the object for the element containing this one.
     * @param[in] fileName Name of the file from which this constructor was called.
     * @param[in] lineNumber Line number at which this constructor was called.
     */
    QRElement(ElementType eType, XMLElementPtr parent = NULL, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : XMLElement(parent, ADD_MEMCHECK_ARGS_PASS(heap))
       ,id_(heap)
       ,ref_(heap)
       ,charData_(heap)
       ,eType_(eType)
       ,referencedElement_(NULL)
      {
        referencedElement_ = this;
      }

    // This method must be redefined for element types that have element content.
    virtual void startElement(void *parser,
                              const char *elementName,
                              const char **atts)
      {
        throw QRDescriptorException("Element %s cannot contain element content",
                                    getElementName());
      }

    virtual void serializeAttrs(XMLString& xml);

    // The ID and ref data members.
    NAString id_;
    NAString ref_;
    NAString charData_;
    const ElementType eType_;
    QRElementPtr referencedElement_;

  private:
    // Copy construction/assignment not defined.
    QRElement(const QRElement&);
    QRElement& operator=(const QRElement&);
}; // QRElement

/**
 * Class representing a list of elements of a given type. The template argument
 * is the type of the item contained in the list. This class is used to represent
 * elements used as containers for a series of elements of a common type. For
 * example, <CandidateList>, which contains one or more <Candidate> elements,
 * would be represented by the templatate instantiation \c QRList<QRCandidate>.
 */
template <class T>
class QRList : public QRElement
{
  public:
    static const char elemName[];
    QRList()
     : QRElement(ET_List, NULL, NULL)
     {}

    QRList(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_List, NULL, ADD_MEMCHECK_ARGS_PASS(heap))
       ,list_(heap)
      {}

    /**
     * Creates an object representing an element that is an initially empty list
     * containing elements of some other type.
     *
     * @param[in] parent Pointer to parent of this object, the element that
     *                   immediately contains it.
     * @param[in] atts Array of attribute/value pairs of the list element.
     * @param[in] fileName Name of the file from which this constructor was called.
     * @param[in] lineNum Line number at which this constructor was called.
     */
    QRList(XMLElementPtr parent, AttributeList atts,
           ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_List, parent, ADD_MEMCHECK_ARGS_PASS(heap))
       ,list_(heap)
      {}

    /**
     * The destructor uses deletePtr on each item in the list. The definition
     * of the deletePtr macro depends on the memory management option the code
     * is compiled with.
     */
    virtual ~QRList()
      {
        for (CollIndex i = 0; i < list_.entries(); i++) 
          deletePtr(list_[i]);
      }

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters

    /**
     * Get the number of elements in the list.
     * @return The number of elements in the list.
     */
    CollIndex entries()
    {
      return list_.entries();
    }

    /**
     * Get element number i from the list.
     * @param i The index into the list of the element to get.
     * @return The pointer to the needed element.
     */
    PTR_TO_TYPE(T) operator[](CollIndex i)
    {
      return list_[i];
    }

    /**
     * Returns a reference to the list of elements contained by this object.
     *
     * @return Reference to list of elements.
     */
    const NAPtrList<PTR_TO_TYPE(T)>& getList() const
      {
        return list_;
      }

    /**
     * Indicates whether or not the list is empty. This is a convenience
     * method that is equivalent to <code>getList().isEmpty()</code>.
     * @return 
     */
    NABoolean isEmpty() const
      {
        return list_.isEmpty();
      }

    /**
     * Adds an item to this list.
     *
     * @param[in] item Pointer to the item to add to the list. 
     */
    void addItem(PTR_TO_TYPE(T) item)
      {
        list_.insert(item);
      }

    /**
     * Adds an item to this ordered list.
     *
     * @param[in] item Pointer to the item to add to the list. 
     */
    void addItemOrdered(PTR_TO_TYPE(T) item);

  protected:
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QRList(const QRList<T>&);
    QRList<T>& operator=(const QRList<T>&);

    // Attributes and contained elements.
    NAPtrList<PTR_TO_TYPE(T)> list_;
}; // QRList

/**
 * An abstract class representing a list of elements of any type. 
 * The elements' type can be restricted by subclassing classes.
 * \par
 * Attributes: none
 * \par
 * Subelements:
 *   A list of QRElement elements.
 */
class QRElementList : public QRElement
{
protected:
    QRElementList(ElementType type, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(type, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        list_(heap)
      {}

    QRElementList(ElementType type, XMLElementPtr parent, AttributeList atts,
               ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

public:
    virtual ~QRElementList();

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters

    /**
     * Returns the full list of elements.
     */
    const ElementPtrList& getList() const
    {
      return list_;
    }

    /**
     * Get element number i from the list.
     * @param i The index into the list
     * @return 
     */
    const QRElementPtr getElement(CollIndex i)
    {
      return list_[i];
    }

    /**
     * Adds an element to the list.
     */
    void addElement(QRElementPtr elem)
    {
      if (!isAllowed(elem))
        throw QRDescriptorException("<%s> is not allowed in <%s>",
                                    elem->getElementName(), getElementName());
      list_.insert(elem);
    }

    /**
     * Indicates whether or not this list is empty.
     * @return \c TRUE if the list is empty, \c FALSE otherwise.
     */
    NABoolean isEmpty() const
    {
      return list_.isEmpty();
    }

    CollIndex entries() const
    {
      return list_.entries();
    }

    void removeElement(QRElementPtr elem)
    {
      list_.remove(elem);
    }

protected:
    /**
     * Is this element allowed in this list.
     * This pure virtual method must be overridden by subclasses.
     * @return TRUE if the element is allowed.
     */
    virtual NABoolean isAllowed(QRElementPtr elem) = 0;

    void startItemExprElement(void *parser, const char *elementName, const char **atts);

  protected:
    virtual void serializeBody(XMLString& xml);

  private:
    // Copy construction/assignment not defined.
    QRElementList(const QRElementList&);
    QRElementList& operator=(const QRElementList&);

    // Attributes and contained elements.
    ElementPtrList list_;  // Element can be Table, JBB, or Operator
}; // QRElementList

/**
 * Abstract superclass of the descriptor classes that use a list of JBBs.
 * This includes the query and MV descriptor, but not the result descriptor,
 * which only references a JBB of the query descriptor.
 */
class QRDescriptor : public QRElement
{
  public:
    virtual ~QRDescriptor()
      {}

    virtual NABoolean treeWalk(VisitorPtr visitor);

    /**
     * Returns the list of JBBs contained in this descriptor.
     * @return The list of JBBs.
     */
    const NAPtrList<QRJBBPtr>& getJbbList() const
      {
        return jbbList_;
      }

    /**
     * Adds a JBB to the list for this MV.
     * @param jbb The JBB to add to the list.
     */
    void addJBB(QRJBBPtr jbb)
      {
        jbbList_.insert(jbb);
      }

  protected:
    QRDescriptor(ElementType eType, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap))
      : QRElement(eType, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        jbbList_(heap)
      {}

    // Added to accommodate QRMVDescriptor as a subelement (of <Publish>)
    QRDescriptor(ElementType eType, XMLElementPtr parent, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap))
      : QRElement(eType, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
        jbbList_(heap)
      {}

    NAPtrList<QRJBBPtr> jbbList_;

  private:
    // Copy construction/assignment not defined.
    QRDescriptor(const QRQueryDescriptor&);
    QRDescriptor& operator=(const QRQueryDescriptor&);
}; // QRDescriptor

/**
 * Miscellaneous information accompanying a query descriptor, defined in an XML
 * document by the <Misc> element within a <Query> element.
 *
 * \par
 * Attributes:
 *   - \b userID -- User ID associated with the query.
 *   - \b MVAge -- MV age in effect.
 *   - \b optLevel -- Optimization level in effect.
 *   - \b rewriteLevel -- Value of the MV_REWRITE_LEVEL CQD, which determines
 *                        how inclusive we are of MV candidates (higher number
 *                        results in more candidates).
 * \par
 * Subelements:
 *   - \b ForcedMVs -- Contains the names of MVs to force consideration of.
 */
class QRQueryMisc : public QRElement
{
  public:
    static const char elemName[];

    QRQueryMisc(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    QRQueryMisc(XMLElementPtr parent, AttributeList atts,
                ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));
	
    virtual ~QRQueryMisc();

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters

    /**
     * Returns the user ID associated with the query.
     * @return The user ID.
     */
    const NAString& getUserID() const
      {
        return userID_;
      }

    /**
     * Returns the MV age associated with the query.
     * @return The MV age.
     */
    const NAString& getMVAge() const
      {
        return mvAge_;
      }

    /**
     * Returns the optimization level in effect for the query.
     * @return The optimization level.
     */
    const NAString& getOptLevel() const
      {
        return optLevel_;
      }

    /**
     * Returns the rewrite level in effect.
     * @return The query rewrite level.
     */
    const MvqrRewriteLevel getRewriteLevel() const
      {
        return rewriteLevel_;
      }

    /**
     * Returns MVs listed in the \c FORCE_MV_REWRITE CQD. QMS will not
     * disqualify these MVs based on heuristic reasons.
     * @return List of MVs not to be disqualified for heuristic reasons.
     */
    QRForcedMVsPtr getForcedMVs() const
      {
        return forcedMVs_;
      }

    /**
     * Sets the user ID for the query.
     * @param id The user ID.
     */
    void setUserID(NAString& id)
      {
        userID_ = id;
      }

    /**
     * Sets the MV age associated with the query.
     * @param age The MV age.
     */
    void setMVAge(NAString& age)
      {
        mvAge_ = age;
      }

    /**
     * Sets the MV age associated with the query.
     * @param age The MV age.
     */
    void setMVAge(char* age)
      {
        mvAge_ = *age;
      }

    /**
     * Sets the optimization level to use for the query.
     * @param optLevel The optimization level.
     */
    void setOptLevel(NAString& optLevel)
      {
        optLevel_ = optLevel;
      }

    /**
     * Sets the rewrite level to use for the query. This comes from the
     * MV_REWRITE_LEVEL CQD.
     * @param rewriteLevel The query rewrite level.
     */
    void setRewriteLevel(MvqrRewriteLevel rewriteLevel)
      {
        rewriteLevel_ = rewriteLevel;
      }

    /**
     * Sets the list of MVs that QMS is not to disqualify.
     * @param forcedMVs The list of MVs.
     */
    void setForcedMVs(QRForcedMVsPtr forcedMVs)
      {
        forcedMVs_ = forcedMVs;
      }

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QRQueryMisc(const QRQueryMisc&);
    QRQueryMisc& operator=(const QRQueryMisc&);

    // Attributes and contained elements.

    NAString userID_;
    NAString mvAge_;
    NAString optLevel_;
    MvqrRewriteLevel rewriteLevel_;
    QRForcedMVsPtr forcedMVs_;
}; // QRQueryMisc

/**
 * Class representing a query descriptor, defined in an XML document by the
 * <Query> element. The query descriptor is created by the Analyzer, and
 * describes a query being compiled. It is used to search the matching data
 * structures of the QMS.
 * \par
 * Attributes:
 *   - \b options -- Can be used to specify certain options from the compiler.
 * \par
 * Subelements:
 *   - \b Version -- Version of the descriptor definition.
 *   - \b Misc -- Miscellaneous information pertaining to the descriptor.
 *   - \b JBB -- A join backbone from the query represented by this descriptor.
 */
class QRQueryDescriptor : public QRDescriptor
{
  public:
    static const char elemName[];

    QRQueryDescriptor(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRDescriptor(ET_QueryDescriptor, ADD_MEMCHECK_ARGS_PASS(heap)),
        version_(NULL),
        misc_(new(heap) QRQueryMisc(ADD_MEMCHECK_ARGS(heap))),
        options_(heap)
      {}

    /**
     * Creates an instance of QRQueryDescriptor that references itself as parent,
     * since it is always used as a document (outermost) element.
     *
     * @param[in] atts List of attribute/value pairs; should be empty for this
     *                 element.
     * @param[in] fileName Name of the file from which this constructor was called.
     * @param[in] lineNum Line number at which this constructor was called.
     */
    QRQueryDescriptor(AttributeList atts,
                      ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QRQueryDescriptor();

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters

    /**
     * Returns the version of this query descriptor.
     * @return Descriptor version object.
     */
    const QRVersionPtr getVersion() const
      {
        return version_;
      }

    /**
     * Returns the miscellaneous information for this query descriptor.
     * @return Miscellaneous info object.
     */
    const QRQueryMiscPtr getMisc() const
      {
        return misc_;
      }

    /**
     * Returns the string specifying options for this query.
     * @return Options string for the query.
     */
    const NAString& getOptions() const
      {
        return options_;
      }

    /**
     * Sets the version of this query descriptor.
     * @param version The version of the descriptor.
     */
    void setVersion(QRVersionPtr version)
      {
        version_ = version;
      }

    /**
     * Sets the miscellaneous information for this query descriptor.
     * @param misc The miscellaneous info.
     */
    void setMisc(QRQueryMiscPtr misc)
      {
        misc_ = misc;
      }

    /**
     * Sets the options from the compiler for this query.
     * @param options The options for this query.
     */
    void setOptions(const NAString& options)
      {
        options_ = options;
      }

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

    QRVersionPtr version_;
    QRQueryMiscPtr misc_;
    NAString options_;

  private:
    // Copy construction/assignment not defined.
    QRQueryDescriptor(const QRQueryDescriptor&);
    QRQueryDescriptor& operator=(const QRQueryDescriptor&);
}; // QRQueryDescriptor

/**
 * Class representing a join backbone, defined in an XML document by the <JBB>
 * element.
 * \par
 * Attributes:
 *   - \b ID -- Unique identifier for this JBB.
 *   - \b ref -- Unique identifier of a referenced JBB.
 * \par
 * Subelements:
 *   - \b Hub -- The JBB's hub.
 *   - \b ExtraHub -- The extra-hub part of the JBB.
 *   - \b OutputList -- Select list items from this JBB.
 */
class QRJBB : public QRElement
{
  public:
    static const char elemName[];

    QRJBB(NumericID idNum, CollIndex nodeId, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    QRJBB(NumericID idNum, JBB* jbb, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    QRJBB(XMLElementPtr parent, AttributeList atts,
          ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QRJBB();

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual void startElement(void *parser, const char *elementName, const char **atts);

    virtual NABoolean treeWalk(VisitorPtr visitor);

    /**
     * Redefines this virtual function to unconditionally return \c TRUE, since
     * a JBB can be a join backbone child (JBBC).
     *
     * @return \c TRUE, indicating that this element may be a JBBC.
     */
    virtual NABoolean canBeJbbc()
      {
        return TRUE;
      }

    // Getters/setters

    virtual char getIDFirstChar()
    {
      return 'B';
    }

    /**
     * Returns the hub of this JBB.
     * @return The JBB's hub.
     */
    QRHubPtr getHub() const
      {
        return hub_;
      }

    /**
     * Returns the extra-hub of this JBB.
     * @return The JBB's extra-hub.
     */
    QRExtraHubPtr getExtraHub() const
      {
        return extraHub_;
      }

    /**
     * Returns the select list items for this JBB.
     * @return Select list items of the JBB.
     */
    QROutputListPtr getOutputList() const
      {
        return outputList_;
      }

    /**
     * Sets the hub of this JBB
     * @param hub The JBB's hub.
     */
    void setHub(QRHubPtr hub)
      {
        hub_ = hub;
      }

    /**
     * Sets the extra-hub of this JBB.
     * @param xhub The JBB's extra-hub.
     */
    void setExtraHub(QRExtraHubPtr xhub)
      {
        extraHub_ = xhub;
      }


    /**
     * Returns the primary grouping columns/expressions for this hub. The primary
     * grouping columns are those not functionally dependent on other grouping
     * columns.
     * @return Primary grouping columns of the hub.
     */
    QRGroupByPtr getGroupBy() const
      {
        return groupBy_;
      }

    /**
     * Sets the primary grouping columns/expressions for this hub.
     * @param groupBy The set of primary grouping columns/expressions.
     */
    void setGroupBy(QRGroupByPtr groupBy)
      {
        groupBy_ = groupBy;
      }

    /**
     * Returns the pointer to the JBB corresponding to this QRJBB element.
     * @return Ptr to \c JBB this element represents.
     */
    JBB* getJBB() const
      {
        return actualJbbPtr_;
      }
      
    /**
     * Returns the node id associated with this object, or \c NULL_CA_ID if
     * there isn't one. The node id is stored only for a QRJBB that does not
     * correspond to an actual \c JBB instance, i.e., a "single-node JBB" that
     * we create ourselves.
     *
     * @return Node id for this object.
     */
    CollIndex getNodeId() const
      {
        return caNodeId_;
      }

    // The following functions are used to collect the list of tables that are
    // part of the JBB while we traverse the relexpr tree.

    void createTableArray(CollIndex maxEntries);

    void addTable(QRTablePtr table);

    QRTablePtr* getTableArray() const
      {
        return tableArray_;
      }

    CollIndex getTableCount() const
      {
        return tableCount_;
      }

    GroupByAgg* getGbExpr() const
      {
        return gbExpr_;
      }

    void setGbExpr(GroupByAgg* gb)
      {
        gbExpr_ = gb;
      }

  protected:
    virtual void serializeBody(XMLString& xml);

    QRHubPtr hub_;
    QRExtraHubPtr extraHub_;
    QROutputListPtr outputList_;
    QRGroupByPtr groupBy_;

    // Saving the following values allows us to delay processing predicates for
    // the JBB until all QRJBBs have been created. This is necessary because a
    // predicate may reference a table for which the corresponding element hasn't
    // been set up yet, and fail because it needs the range or residual pred
    // bitmap for that table.
    CollIndex caNodeId_;   // used for simulated (single-table) JBBs
    JBB* actualJbbPtr_;    // used for real (Analyzer) JBBs

  private:
    // Copy construction/assignment not defined.
    QRJBB(const QRJBB&);
    QRJBB& operator=(const QRJBB&);

    QRTablePtr* tableArray_;
    CollIndex tableCount_, maxTableEntries_;
    GroupByAgg* gbExpr_;
}; // QRJBB

/**
 * Class containing information on details of the matching process, defined in
 * an XML document by the <Info>. This is an optional item, and can be used for
 * such things as reasons why certain MVs were not chosen as candidates.
 * \par
 * Attributes: none
 * \par
 * Subelements: none
 * \par
 * Character data: The text of the informational message.
 */
class QRInfo : public QRElement
{
  public:
    static const char elemName[];

    QRInfo(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_Info, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        text_(charData_)
      {}

    QRInfo(XMLElementPtr parent, AttributeList atts,
          ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_Info, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
        text_(charData_)
    {}
	
    virtual ~QRInfo()
      {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

    // @ZX -- for use with NAHashDictionary
    Int32 operator==(const QRInfo &rhs) const
      {
        return text_ == rhs.text_;
      }

    // Getters/setters

    /**
     * Returns the text of this information item.
     * @return Information text.
     */
    const NAString& getText() const
      {
        return text_;
      }

    /**
     * Sets the text of this information item.
     * @param text Information text.
     */
    void setText(const NAString& text)
      {
        text_ = text;
      }

    virtual void toXML(XMLString& xml, NABoolean sameLine = FALSE, NABoolean noEndTag = FALSE)
    {
      // Put the content (the string value) directly between the start and end tags
      // with no spaces or newlines added.
      XMLElement::toXML(xml, TRUE);
    }

    /**
     * Redefines the QRElement version of \c charData to avoid stripping
     * whitespace from the character data. A StringVal is serialized so that
     * the content is directly between the start and end tags on the same line.
     * Any leading or trailing whitespace is therefore part of the string value.
     *
     * @param[in] parser Pointer to the instance of the parser being used.
     * @param[in] data   The character data.
     * @param[in] len    The length of the character data.
     */
    virtual void charData(void *parser, const char *data, Int32 len)
      {
        charData_.append(data, len);
      }

    /**
     * This redefinition simply writes the content of the string directly to
     * the serialized XML document, to avoid any addition of whitespace for
     * formatting, as would otherwise be done if \c xml is an instance of
     * XMLFormattedString.
     *
     * @param xml The XML text of the descriptor being written.
     */
    virtual void serializeBody(XMLString& xml)
      {
        xml.append("<![CDATA[").append(charData_).append("]]>");
      }

  private:
    // Copy construction/assignment not defined.
    QRInfo(const QRInfo&);
    QRInfo& operator=(const QRInfo&);

    NAString& text_;

    // Attributes and contained elements.
}; // QRInfo

/**
 * Class representing the hub of a JBB, defined in an XML document by the
 * <Hub> element. The <Hub> element includes the list of tables that are part
 * of the hub, the predicates between them, and optionally the primary
 * grouping columns.
 * \par
 * Attributes: none 
 * \par
 * Subelements:
 *   - \b JBBCList -- List of join backbone children comprising the hub. In
 *                    addition to tables, these could include nested JBBs and
 *                    operators such as left outer join, semijoin, or transpose.
 *   - \b GroupBy --  grouping columns.
 *   - \b JoinPredList -- List of equijoins linking the hub tables.
 *   - \b RangePredList -- List of range predicates on hub tables.
 *   - \b ResidualPredList -- List of residual predicates on hub tables.
 */
class QRHub : public QRElement
{
  public:
    static const char elemName[];

    QRHub(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    QRHub(XMLElementPtr parent, AttributeList atts,
          ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QRHub();

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters

    /**
     * Returns the list of join backbone children for this hub.
     * @return List of the hub's JBBCs.
     */
    QRJBBCListPtr getJbbcList() const
      {
        return jbbcList_;
      }

    /**
     * Returns the list of equijoin predicates used within this hub.
     * @return List of the hub's join predicates.
     */
    QRJoinPredListPtr getJoinPredList() const
      {
        return joinPredList_;
      }

    /**
     * Returns the list of range predicates used within this hub.
     * @return List of the hub's range predicates.
     */
    QRRangePredListPtr getRangePredList() const
      {
        return rangePredList_;
      }

    /**
     * Returns the list of residual predicates used within this hub.
     * @return List of the hub's residual predicates.
     */
    QRResidualPredListPtr getResidualPredList() const
      {
        return residualPredList_;
      }

  protected:
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QRHub(const QRHub&);
    QRHub& operator=(const QRHub&);

    QRJBBCListPtr jbbcList_;  // Element can be Table, JBB, or Operator
    QRJoinPredListPtr joinPredList_;
    QRRangePredListPtr rangePredList_;
    QRResidualPredListPtr residualPredList_;
}; // QRHub

/**
 * Class representing a list of join backbone children, defined in an XML document
 * by the <JBBCList> element. An instance of this class consists of a list of
 * element objects which are instances of QRTable, QRJBB, and QROperator.
 * \par
 * Attributes: none
 * \par
 * Subelements:
 *   - \b Table -- A table belonging to this JBBC list.
 *   - \b JBB -- A nested JBB belonging to this JBBC list.
 *   - \b Operator -- A relational operator, such as left outer join or semijoin,
 *                    belonging to this JBBC list.
 */
class QRJBBCList : public QRElementList
{
  public:
    static const char elemName[];

    QRJBBCList(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElementList(ET_JBBCList, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    QRJBBCList(XMLElementPtr parent, AttributeList atts,
               ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QRJBBCList()
    {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

    /**
     * Sets \c isKeyCovered to TRUE for each member of the JBBC list that is a
     * table. This is used when catman is setting up an MV descriptor for an
     * incremental MJV, where all the tables used in the MV are guaranteed to
     * have their clustering key covered.
     */
    void setIsKeyCovered();

  protected:
    virtual void startElement(void *parser, const char *elementName, const char **atts);

    /**
     * Is this element allowed as a JBBC?.
     * @return TRUE if the element is allowed as a JBBC.
     */
    virtual NABoolean isAllowed(QRElementPtr elem)
    {
      return elem->canBeJbbc();
    }

  private:
    // Copy construction/assignment not defined.
    QRJBBCList(const QRJBBCList&);
    QRJBBCList& operator=(const QRJBBCList&);

}; // QRJBBCList

/**
 * Class representing a table, defined in an XML document by the &lt;Table> element.
 * \par
 * Attributes:
 *   - \b id -- Unique identifier for this table.
 *   - \b TS -- Redefinition timestamp for this table.
 *   - \b reason -- For an extra-hub table, indicates why it is in the extra-hub.
 *   - \b IsAnMV -- Indicates whether or not the table is an MV.
 *   - \b isKeyCovered -- Indicates whether or not clustering index is covered
 *                        by the MV. Used only in MV descriptors, and defaults
 *                        to 0. If 1, the clustering index is covered and the
 *                        table can be used for back joins.
 *   - \b numCols -- The number of columns in the table.
 *   - \b rangeBits -- Bitmap of columns with range predicates.
 *   - \b residualBits -- Bitmap of columns with residual predicates.
 * \par
 * Subelements:
 *  - \b Key -- The primary key of the table.
 * \par
 * Character data: The fully-qualified name of the table.
 */
class QRTable : public QRElement
{
  public:
    static const char elemName[];

    QRTable(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    QRTable(XMLElementPtr parent, AttributeList atts,
            ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QRTable()
      {}

    virtual QRTablePtr downCastToQRTable()    
    { 
      return this; 
    }

    virtual const char *getElementName() const
      {
        return elemName;
      }

    /**
     * Redefines this virtual function to unconditionally return \c TRUE, since
     * a table can be a join backbone child (JBBC).
     *
     * @return \c TRUE, indicating that this element may be a JBBC.
     */
    virtual NABoolean canBeJbbc()
      {
        return TRUE;
      }

    virtual const NAString& getSortName() const
      {
        return tableName_;
      }

    // Getters/setters

    virtual char getIDFirstChar()
      {
        return 'T';
      }

    /**
     * Overrides the standard definition of this function to throw an exception.
     * This ensures that #setAndRegisterID() is not bypassed as the way to set
     * the id of a table.
     *
     * @see setAndRegisterID()
     * @param idNum Id of the table.
     */
    virtual void setID(NumericID idNum)
      {
        assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                          FALSE, QRDescriptorException, 
                          "setAndRegisterID() must be used instead of setID() "
                          "for element QRTable");
      }

    /**
     * Sets the id for this table element, and creates an entry in the hash
     * table that maps the ids of columns and tables to their element object.
     *
     * @param idNum The id of this table.
     * @param idHash The id-to-element hash table.
     */
    void setAndRegisterID(NumericID idNum, QRElementHash& idHash)
      {
        QRElement::setID(idNum);
        idHash.insert(&id_, this);
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    /**
     * Returns the redefinition timestamp for this table.
     * @return The redefinition timestamp.
     */
    const NAString& getTimestamp() const
      {
        return redefTimestamp_;
      }

    /**
     * Returns the reason this table is in the extra-hub. If it is a hub table,
     * the result is an empty string.
     * @return Reason for being an extra-hub table, or empty string if it isn't
     */
    const NAString& getExtraHubReason() const
      {
        return extraHubReason_;
      }

    /**
     * Returns an indication of whether or not this table is an MV.
     * @return \c true if the table is an MV, \c false otherwise.
     */
    const NABoolean isAnMV() const
      {
        return isAnMV_;
      }

    /**
     * Returns the fully-qualified name of this table.
     * @return The table name.
     */
    const NAString& getTableName() const
      {
        return tableName_;
      }

    /**
     * Returns an indication of whether or not the clustering key for this
     * table is covered by the MV (used in MV descriptor only).
     * @return \c true if the clustering key is covered, \c false otherwise.
     */
    const NABoolean isKeyCovered() const
      {
        return isKeyCovered_;
      }

    /**
     * Returns the number of columns in the table.
     * @return \c Number of columns in the table.
     */
    Int32 getNumCols() const
      {
	return numCols_;
      }

    /**
     * Returns the bitmap of columns with range predicates.
     * @return Reference to the range predicate bitmap.
     */
    const XMLBitmap& getRangeBits()
    {
      return rangeBits_;
    }

    /**
     * Returns the bitmap of columns with residual predicates.
     * @return Reference to the residual predicate bitmap.
     */
    const XMLBitmap& getResidualBits()
    {
      return residualBits_;
    }

    /**
     * Is this table the right child of a Left Outer Join?
     * @return 
     */
    const NABoolean hasLOJParent() const
    {
      return hasLOJParent_;
    }

    /**
     * Get the primary key of the table.
     * @return 
     */
    const QRKeyPtr getKey()
    {
      return key_;
    }

    /**
     * Sets the redefinition timestamp for this table.
     * @param ts The redefinition timestamp.
     */
    void setTimestamp(const NAString& ts)
      {
        redefTimestamp_ = ts;
      }

    /**
     * Sets the reason that this table is in the extra-hub.
     * @param reason String indicating the reason this is an extra-hub table.
     */
    void setExtraHubReason(NAString& reason)
      {
        extraHubReason_ = reason;
      }

    /**
     * Sets the flag indicating whether or not this table is an MV.
     * @param isMV \c true if this table is an MV, \c false otherwise.
     */
    void setIsAnMV(NABoolean isMV)
      {
        isAnMV_ = isMV;
      }

    /**
     * Sets the name of this table.
     * @param tableName Fully-qualified table name.
     */
    void setTableName(const NAString& tableName)
      {
        tableName_ = tableName;
      }

    /**
     * Sets the flag indicating whether or not the clustering index for this
     * table is covered by the MV.
     * @param keyCovered \c true if the clustering index is covered, \c false
     *                   otherwise.
     */
    void setIsKeyCovered(NABoolean keyCovered)
      {
        isKeyCovered_ = keyCovered;
      }

    /**
     * Sets the number of columns contained in the table, and sizes the range
     * and residual predicate bitmaps accordingly.
     * @param numCols Number of columns in the table.
     */
    void setNumCols(Int32 numCols)
      {
        numCols_ = numCols;
        rangeBits_.resize((numCols + BitsPerWord) / BitsPerWord);
        residualBits_.resize((numCols + BitsPerWord) / BitsPerWord);
      }

    void setExtraHub(NABoolean eh)
    {
      isExtraHub_ = eh;
    }

    virtual NABoolean isExtraHub()
    {
      return isExtraHub_;
    }

    /**
     * Sets the bitmap of columns with range predicates.
     * @param bits 
     */
    void setRangeBits(const XMLBitmap& bits)
    {
      rangeBits_ = bits;
    }

    /**
     * Sets the indicated bit in the range predicate bit map. Bits are set for
     * columns that have a range predicate.
     * @param index Index of the bit to set, corresponding to the column's
     *              position in the table.
     */
    void setRangeBit(CollIndex index)
    {
      rangeBits_ += index;
    }

    /**
     * Sets the bitmap of columns with residual predicates.
     * @param bits 
     */
    void setResidualBits(const XMLBitmap& bits)
    {
      residualBits_ = bits;
    }

    /**
     * Sets the indicated bit in the residual predicate bit map. Bits are set
     * for columns that have a residual predicate.
     * @param index Index of the bit to set, corresponding to the column's
     *              position in the table.
     */
    void setResidualBit(CollIndex index)
    {
      residualBits_ += index;
    }

    /**
     * Set this table as the right child of a left outer join.
     * @param hasLOJParent 
     */
    void setLOJParent(NABoolean hasLOJParent)
    {
      hasLOJParent_ = hasLOJParent;
    }

    /**
     * Set the primary key of the table.
     * @param key 
     */
    void setKey(QRKeyPtr key)
    {
      key_ = key;
    }

    void setCorrelationName(const NAString& name)
    {
      correlationName_ = name;
    }

    const NAString& getCorrelationName() const
    {
      return correlationName_;
    }

    void setJoinOrder(Int32 order)
    {
      joinOrder_ = order;
    }

    const Int32 getJoinOrder() const
    {
      return joinOrder_;
    }

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QRTable(const QRTable&);
    QRTable& operator=(const QRTable&);

    // Attributes and contained elements.
    NAString  redefTimestamp_;  //@ZX -- probably need to store as int64
    NAString  extraHubReason_;
    NABoolean isAnMV_;
    NAString& tableName_;
    NABoolean isKeyCovered_;
    Int32     numCols_;
    XMLBitmap rangeBits_;
    XMLBitmap residualBits_;
    NABoolean hasLOJParent_;
    QRKeyPtr  key_;
    NAString  correlationName_;
    Int32     joinOrder_;

    // Temp flag used during descriptor construction.
    NABoolean isExtraHub_;
}; // QRTable

/**
 * Class representing a list of columns, that is a unique key of a table.
 * \par
 * Attributes: none
 * \par
 * Subelements:
 *   - \b Column -- A column belonging to this key.
 */
class QRKey : public QRElementList
{
  public:
    static const char elemName[];

    QRKey(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElementList(ET_JBBCList, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    QRKey(XMLElementPtr parent, AttributeList atts,
               ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QRKey()
    {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

    // Getters/setters

  protected:
    virtual void startElement(void *parser, const char *elementName, const char **atts);

    /**
     * Is this element allowed as a Key? 
     * TRUE only for Column elements.
     * @return TRUE if the element is a Column.
     */
    virtual NABoolean isAllowed(QRElementPtr elem)
    {
      return elem->getElementType() == ET_Column;
    }

  private:
    // Copy construction/assignment not defined.
    QRKey(const QRKey&);
    QRKey& operator=(const QRKey&);

}; // QRKey

/**
 * Class representing a list of forced MVs, defined in an XML document by the
 * <ForcedMVs> element. Forced MVs are those that QMS is instructed not to
 * disregard for heuristic reasons. Forced MVs are determined by the
 * \c FORCE_MV_REWRITE CQD.
 * \par
 * Attributes: none
 * \par
 * Subelements:
 *   - \b Table -- Each of these gives the name of a forced MV.
 */
class QRForcedMVs : public QRElement
{
  public:
    static const char elemName[];

    QRForcedMVs(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_ForcedMVs, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        tableList_(heap)
      {}

    QRForcedMVs(XMLElementPtr parent, AttributeList atts,
                ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_ForcedMVs, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
        tableList_(heap)
      {}
	
    virtual ~QRForcedMVs();

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters

    /**
     * Returns the list of tables that are forced MVs.
     * @return List of forced MVs.
     */
    const NAPtrList<QRTablePtr>& getTableList() const
      {
        return tableList_;
      }
    
    /**
     * Adds an MV to the list of forced MVs.
     * @param tbl MV to add to the list.
     */
    void addTable(QRTablePtr tbl)
      {
        tableList_.insert(tbl);
      }

  protected:
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QRForcedMVs(const QRForcedMVs&);
    QRForcedMVs& operator=(const QRForcedMVs&);

    NAPtrList<QRTablePtr> tableList_;

    // Attributes and contained elements.
}; // QRForcedMVs

/**
 * Abstract class that is parent of any element that can be part of an explicit
 * expresson: Column, MVColumn, Function, BinaryOper, UnaryOper, and scalar values.
 */
class QRExplicitExpr : public QRElement
{
  public:
    virtual ~QRExplicitExpr()
      {}

    /**
      * Is this function an aggregate function?
      * @return 
      */
    virtual NABoolean isAnAggregate() const
    {
      return FALSE;
    }

    /**
     * Indicates whether or not an aggregate function reference is part of this
     * expression.
     *
     * @param heap Heap to use for the AggregateFinderVisitor instance used in
     *             the search.
     * @return \c TRUE iff one or more aggregate function references appear
     *         within the expression.
     */
    NABoolean containsAnAggregate(CollHeap* heap = NULL);

    /**
     * Create a text representation of the expression.
     * @param text String to append to
     */
    virtual void unparse(NAString& text, NABoolean useColumnName) = 0;

    virtual void getInputColumns(ElementPtrList& inputList, CollHeap* heap,
                         NABoolean useRefedElem = TRUE);

    virtual QRExplicitExprPtr deepCopy(subExpressionRewriteHash& subExpressions, CollHeap* heap) = 0;
    virtual QRExplicitExprPtr deepCopyAndSwitch(subExpressionRewriteHash& subExpressions, CollHeap* heap);

    ItemExpr* findItemExpr();
    virtual ItemExpr* toItemExpr(const NAString& mvName, CollHeap* heap,
                                 TableNameScanHash* scanhash) = 0;

  protected:
    QRExplicitExpr(ElementType eType, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(eType, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        containsAnAggregate_(AGGREGATE_UNKNOWN)
      {}

    QRExplicitExpr(ElementType eType, XMLElementPtr parent, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(eType, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
        containsAnAggregate_(AGGREGATE_UNKNOWN)
      {}

    // Copy Constructor
    QRExplicitExpr(const QRExplicitExpr& other, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(other.getElementType(), NULL, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    /**
     * Creates a subelement of one of the QRExplicitExpr subclasses that has
     * operands/parameters (QRFunction, QRBinaryOper, QRUnaryOper). This function
     * is called from startElement() for any of those classes when the name of a
     * contained element is parsed.
     *
     * @param[in] parser      Pointer to the instance of the parser being used.
     * @param[in] elementName Name of the element in the start tag.
     * @param[in] atts        Array of pointers to attribute name/value pairs.
     * @return Pointer to the created subelement.
     */
    QRExplicitExprPtr constructSubElement(void* parser,
                                          const char* elementName,
                                          const char** atts);

  private:
    /**
     * Enum used to indicate the presence or absence of an aggregate function
     * reference within this explicit expression. If a search of the expression
     * has been conducted, #containsAnAggregate will have an AGGREGATE_YES or
     * AGGREGATE_NO value, and no more searching is required. AGGREGATE_UNKNOWN
     * indicates that a previous search has not been performed.
     */
    enum AggregateStatus
      {
        AGGREGATE_NO  = FALSE,
        AGGREGATE_YES = TRUE,
        AGGREGATE_UNKNOWN
      };

    /**
     * Indicates presence or absence of an aggregate function reference in this
     * explicit expression.
     * @see AggregateStatus
     */
    enum AggregateStatus containsAnAggregate_;
       
    // assignment not defined.
    QRExplicitExpr& operator=(const QRExplicitExpr&);

}; // QRExplicitExpr

/**
 * Class representing a column, defined in an XML document by the <Column>
 * element.
 * \par
 * Attributes:
 *   - \b id -- Unique id assigned to this column.
 *   - \b tableId -- The id of the table this column belongs to.
 *   - \b colIndex -- The ordinal position of the column in the table.
 * \par
 * Subelements: none
 * \par
 * Character data: The unqualified name of the column.
 */
class QRColumn : public QRExplicitExpr
{
  public:
    static const char elemName[];

    QRColumn(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRExplicitExpr(ET_Column, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        tableId_(heap),
        fqColumnName_(charData_),
        columnName_(heap),
        isExtraHub_(FALSE),
        colIndex_(-1),
        isNullable_(1),
        vegrefVid_(0)
      {}

    QRColumn(XMLElementPtr parent, AttributeList atts,
             ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    // Copy Ctor.
    QRColumn(const QRColumn& other, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRExplicitExpr(other, ADD_MEMCHECK_ARGS_PASS(heap))
       ,tableId_(other.tableId_, heap)
       ,fqColumnName_(charData_)
       ,columnName_(other.columnName_, heap)
       ,colIndex_(other.colIndex_)
       ,isNullable_(other.isNullable_)
       ,isExtraHub_(other.isExtraHub_)
       ,vegrefVid_(other.vegrefVid_)
    {
      fqColumnName_ = other.fqColumnName_;
    }

    virtual ~QRColumn()
      {}

    virtual QRColumnPtr downCastToQRColumn() 
    { 
      return this; 
    }

    // Redefined to initialize the non-fully qualified column name.
    virtual void charData(void *parser, const char *data, Int32 len);

    virtual const char *getElementName() const
      {
        return elemName;
      }

    /**
     * Redefines this virtual function to unconditionally return \c TRUE, since
     * a column can be part of the equality set in a join predicate.
     *
     * @return \c TRUE.
     */
    virtual NABoolean canBeInJoinPred()
      {
        return TRUE;
      }

    /**
     * Redefines the default implementation of this virtual function to return
     * \c TRUE, since the element represented by this class may be an output item.
     *
     * @return \c TRUE, indicating that this element can be an output item.
     */
    virtual NABoolean canBeOutputItem() const
      {
        return TRUE;
      }

    /**
     * Redefines the default implementation of this virtual function to return
     * \c TRUE, since the element represented by this class may be a
     * <code>Group By</code> item.
     *
     * @return \c TRUE, indicating that this element can be a
     *         <code>Group By</code> item.
     */
    virtual NABoolean canBeGroupByItem() const
      {
        return TRUE;
      }

    virtual const NAString& getSortName() const
      {
        return fqColumnName_;
      }

    // Getters/setters

    virtual char getIDFirstChar()
    {
      return 'C';
    }

    /**
     * Overrides the standard definition of this function to throw an exception.
     * This ensures that #setAndRegisterID() is not bypassed as the way to set
     * the id of a column.
     *
     * @see setAndRegisterID()
     * @param idNum Id of the column.
     */
    virtual void setID(NumericID idNum)
      {
        assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                          FALSE, QRDescriptorException, 
                          "setAndRegisterID() must be used instead of setID() "
                          "for element QRColumn");
      }

    /**
     * Sets the id for this column element, and creates an entry in the hash
     * table that maps the ids of columns and tables to their element object.
     *
     * @param idNum The id of this column.
     * @param idHash The id-to-element hash table.
     */
    void setAndRegisterID(NumericID idNum, QRElementHash& idHash)
      {
        QRElement::setID(idNum);
        idHash.insert(&id_, this);
      }

    /**
     * Returns the unique identifier for the table this column belongs to.
     * @return The id of the owning table.
     */
    const NAString& getTableID() const
      {
        return tableId_;
      }

    /**
     * Returns the numeric part of the unique identifier for the table this
     * column belongs to.
     * @return Numeric part of the id of the owning table.
     */
    NumericID getTableIDNum() const
      {
        return (tableId_.length() < 2 ? 0 : atoi(tableId_.data()+1));
      }

    /**
     * Returns the fully-qualified name of this column.
     * @return The column's name.
     */
    const NAString& getFullyQualifiedColumnName() const
      {
        return fqColumnName_;
      }

    /**
     * Returns the name of this column.
     * @return The column's name.
     */
    const NAString& getColumnName() const
      {
        return columnName_;
      }

    /**
     * Returns the ordinal position of this column within the table.
     * @return The column's ordinal position.
     */
    Int32 getColIndex() const
      {
        return colIndex_;
      }

    /**
     * Is the column nullable.
     * @return 
     */
    NABoolean isNullable() const
      {
        return isNullable_;
      }

    /**
     * Associates this column with its owning table.
     * @param[in] tblId Unique numeric part of the id of the table owning this
     *                  column.
     */
    void setTableID(NumericID tblId)
      {
        char tblStr[13] = "T";
        str_itoa(tblId, tblStr+1);
        tableId_ = tblStr;
      }

    /**
     * Associates this column with its owning table.
     * @param[in] tblId Unique ID of the table owning this column.
     */
    void setTableID(const NAString& tblId)
      {
        tableId_ = tblId;
      }

    /**
     * Sets the fully-qualified name of this column. The simple column name,
     * which is stored separately, is also set.
     * @param[in] colName The column's name.
     */
    void setFullyQualifiedColumnName(const NAString& colName)
      {
        fqColumnName_ = colName;
        setColumnName_();
      }

    /**
     * Sets the ordinal position of the column in the table.
     * @param colIndex Ordinal position of the column in the table.
     */
    void setColIndex(Int32 colIndex)
      {
        colIndex_ = colIndex;
      }

    /**
     * Sets the isNullable_ attribute.
     * @param nullable 
     */
    void setNullable(NABoolean nullable)
      {
        isNullable_ = nullable;
      }

    /**
     * Associates this object with an existing \c Column or \c JoinPred.
     * @param ref Numeric part of the ID of the referenced element.
     * @param refsJoinPred \c TRUE iff the reference is to a \c JoinPred
     *                     rather than a \c Column.
     */
    void setRefFromInt(NumericID refNum, NABoolean refsJoinPred = FALSE)
      {
	if (refsJoinPred)
	  QRElement::setRefFromInt(refNum, 'J');
	else
	  QRElement::setRefFromInt(refNum);
      }

    void setExtraHub(NABoolean eh)
    {
      isExtraHub_ = eh;
    }

    virtual NABoolean isExtraHub()
    {
      return isExtraHub_;
    }

    CollIndex getVegrefId() const
    {
      return vegrefVid_;
    }

    void setVegrefId(CollIndex vid)
    {
      vegrefVid_ = vid;
    }

    // Override the default implementation of doing a tree-walk
    // with a simple case for a single QRColumn.
    virtual void getInputColumns(ElementPtrList& inputList, CollHeap* heap,
                                 NABoolean useRefedElem = TRUE)
    {
      inputList.insert(getReferencedElement());
    }

    /**
     * Create a text representation of the expression.
     * @param text String to append to
     */
    virtual void unparse(NAString& text, NABoolean useColumnName);

    virtual QRExplicitExprPtr deepCopy(subExpressionRewriteHash& subExpressions, CollHeap* heap)
    {
      // We get here for a QRColumn element that is not in the subExpressions
      // hash table, so something is wrong.
      throw QRDescriptorException("QRColumn element not handled.");
      //QRColumnPtr result = new (heap) QRColumn(*this, ADD_MEMCHECK_ARGS(heap));
      //return result;
    }

    virtual ItemExpr* toItemExpr(const NAString& mvName, CollHeap* heap,
                                 TableNameScanHash* scanhash);

    virtual QRColumnPtr getFirstColumn();

  protected:
    virtual void serializeAttrs(XMLString& xml);

  private:
    // Assignment not defined.
    QRColumn& operator=(const QRColumn&);

    // Helper function used to keep col name in synch with fully qualified name.
    void setColumnName_();

    // Attributes and contained elements.
    NAString  tableId_;
    NAString& fqColumnName_; // Fully qualified column name - from XML
    NAString  columnName_;   // Minimal column name - computed after parsing.
    Int32     colIndex_;
    NABoolean isNullable_;

    // Temp flag used during descriptor construction.
    NABoolean isExtraHub_;

    // Vegref column is in, if any. Needed for map value ids node if the column
    // is used in a function.
    CollIndex vegrefVid_;
}; // QRColumn

/**
 * Class representing a column of an MV, defined in an XML document by the
 * <MVColumn> element. MV columns are used in rewrite instructions.
 * \par
 * Attributes:
 *   - \b MV -- Name of the MV containing the column.
 *   - \b ref -- Id of the column this MV column is to replace.
 * \par
 * Subelements: none
 * \par
 * Character data: Unqualified name of the MV column.
 */
class QRMVColumn : public QRExplicitExpr
{
  public:
    static const char elemName[];

    QRMVColumn(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRExplicitExpr(ET_MVColumn, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        mv_(heap),
        mvColName_(charData_),
        aggForRewrite_(ITM_NOT)  // This means not initialized.
      {}

    QRMVColumn(XMLElementPtr parent, AttributeList atts,
               ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    // Copy Ctor
    QRMVColumn(const QRMVColumn& other, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRExplicitExpr(other, ADD_MEMCHECK_ARGS_PASS(heap)),
        mv_(other.mv_, heap),
        mvColName_(charData_),
        aggForRewrite_(ITM_NOT)  // This means not initialized.
      {}

    virtual ~QRMVColumn()
      {}

    virtual QRMVColumnPtr downCastToQRMVColumn()
    { 
      return this;
    }

    virtual const char *getElementName() const
      {
        return elemName;
      }

    /**
     * Redefines this virtual function to unconditionally return \c TRUE, since
     * an MV column can be part of the equality set in a join predicate.
     *
     * @return \c TRUE.
     */
    virtual NABoolean canBeInJoinPred()
      {
        return TRUE;
      }

    /**
     * Redefines the default implementation of this virtual function to return
     * \c TRUE, since the element represented by this class may be an output item.
     *
     * @return \c TRUE, indicating that this element can be an output item.
     */
    virtual NABoolean canBeOutputItem() const
      {
        return TRUE;
      }

    /**
     * Redefines the default implementation of this virtual function to return
     * \c TRUE, since the element represented by this class may be a
     * <code>Group By</code> item.
     *
     * @return \c TRUE, indicating that this element can be a
     *         <code>Group By</code> item.
     */
    virtual NABoolean canBeGroupByItem() const
      {
        return TRUE;
      }

    /**
     * Redefines the default implementation of this virtual function to return
     * \c TRUE, since the element represented by this class may appear as the
     * root element within a \c QRExpr element.
     *
     * @return \c TRUE, indicating that this element can be the root of an
     *         expression tree.
     */
    virtual NABoolean canBeExprRoot() const
      {
        return TRUE;
      }

    // Getters/setters

    /**
     * Returns the name of the MV owning this column.
     * @return Name of the column's MV.
     */
    const NAString& getMV() const
      {
        return mv_;
      }

    /**
     * Returns the unqualified name of this MV column.
     * @return The MV column name.
     */
    const NAString& getMVColName() const
      {
        return mvColName_;
      }

    /**
     * Sets the name of the MV owning this column.
     * @param mvName The MV name.
     */
    void setMV(const NAString& mvName)
      {
        mv_ = mvName;
      }

    /**
     * Sets the unqualified name of this MV column.
     * @param name Name of the MV column.
     */
    void setMVColName(const NAString& name)
      {
        mvColName_ = name;
      }

    /**
     * Create a text representation of the expression.
     * @param text String to append to
     */
    virtual void unparse(NAString& text, NABoolean useColumnName)
    {
      text += getMVColName();
    }

    virtual QRExplicitExprPtr deepCopy(subExpressionRewriteHash& subExpressions, CollHeap* heap)
    {
      QRMVColumnPtr result = new (heap) QRMVColumn(*this, ADD_MEMCHECK_ARGS(heap));
      return result;
    }

    virtual ItemExpr* toItemExpr(const NAString& mvName, CollHeap* heap,
                                 TableNameScanHash* scanhash);

    /**
     * Returns TRUE if the MV column references the given ValueId. This can be
     * either directly through the ref attribute, or via a ref to a joinpred that
     * contains the ValueId.
     *
     * @param vid Looking for a reference to this ValueId.
     * @return TRUE if the MV column references vid, FALSE otherwise.
     */
    NABoolean hasRefTo(CollIndex vid);

    /**
     * Gets the aggregate function that is to be applied to the MV column in
     * rewrite. When a rollup query is executed, the mv column representing an
     * aggregate function is itself the argument of an aggregate function to
     * effect the rollup.
     *
     * @return Aggregate function to use.
     */
    OperatorTypeEnum getAggForRewrite() const
      {
        return aggForRewrite_;
      }

    /**
     * Sets the aggregate function to be applied to the MV column when doing
     * the rewrite.
     *
     * @param agg The aggregate function to use to rewrite the query.
     */
    void setAggForRewrite(OperatorTypeEnum agg)
      {
       aggForRewrite_ = agg;
      }

  protected:
    virtual void serializeAttrs(XMLString& xml);

  private:
    // Assignment not defined.
    QRMVColumn& operator=(const QRMVColumn&);

    // Attributes and contained elements.

    NAString mv_;
    NAString& mvColName_;
    OperatorTypeEnum aggForRewrite_;
}; // QRMVColumn

/**
 * Class representing an operator, defined in an XML document by the <Operator>
 * element. The content of this element has not been defined yet.
 */
class QROperator : public QRElement
{
  public:
    static const char elemName[];

    QROperator(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_Operator, NULL, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    QROperator(XMLElementPtr parent, AttributeList atts,
               ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QROperator()
      {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters

  protected:
    virtual void serializeAttrs(XMLString& xml)
      {
        //assert(FALSE);
      }

    /**
     * Redefines this virtual function to unconditionally return \c TRUE, since
     * an Operator can be a join backbone child (JBBC).
     *
     * @return \c TRUE, indicating that this element may be a JBBC.
     */
    virtual NABoolean canBeJbbc()
      {
        return TRUE;
      }

  private:
    // Copy construction/assignment not defined.
    QROperator(const QROperator&);
    QROperator& operator=(const QROperator&);

    // Attributes and contained elements.
}; // QROperator

/**
 * Class representing a equijoin predicate, defined in an XML document by the
 * <JoinPred> element. A JoinPred defines a column equality set, the members of
 * which are known to be equal because of the join predicates they participate in.
 * \par
 * Attributes:
 *   - \b id -- Unique identifier of this join predicate.
 *   - \b ref -- Unique id of a join predicate referenced by this object.
 *   - \b result -- Matching result for this join predicate.
 * \par
 * Subelements:
 *   - \b Column -- A column included in the equality set of the join predicate.
 *   - \b MVColumn -- An MV column included in the equality set of the join predicate.
 *   - \b Expr -- An expression included in the equality set of the join predicate.
 */
class QRJoinPred : public QRElementList
{
  public:
    static const char elemName[];

    QRJoinPred(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElementList(ET_JoinPred, ADD_MEMCHECK_ARGS_PASS(heap))
       ,result_(INVALID_EXPR_RESULT)
      {}

    QRJoinPred(XMLElementPtr parent, AttributeList atts,
               ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QRJoinPred()
    {}

    virtual QRJoinPredPtr downCastToQRJoinPred() 
    { 
      return this; 
    }

    virtual const char *getElementName() const
      {
        return elemName;
      }

    /**
     * Redefines this virtual function to unconditionally return \c TRUE, since
     * a JoinPred can itself be part of the equality set in another JoinPred.
     * This occurs when an extra-hub JoinPred references the join predicate
     * composed of items from the same equality set.
     *
     * @see QRDescGenerator::addJoinPred(), where the link from the extra-hub
     *      JoinPred to the corresponding hub JoinPred is established.
     * @return \c TRUE.
     */
    virtual NABoolean canBeInJoinPred()
      {
        return TRUE;
      }

    // Getters/setters

    virtual char getIDFirstChar()
    {
      return 'J';
    }

    /**
     * Overrides the standard definition of this function to throw an exception.
     * This ensures that #setAndRegisterID() is not bypassed as the way to set
     * the id of a join pred.
     *
     * @see setAndRegisterID()
     * @param idNum Id of the JoinPred.
     */
    virtual void setID(NumericID idNum)
      {
        assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                          FALSE, QRDescriptorException, 
                          "setAndRegisterID() must be used instead of setID() "
                          "for element QRJoinPred");
      }

    /**
     * We get here only when, in a multi-JBB query, one join pred includes 
     * a reference to another one.
     * @return 
     */
    virtual const NAString& getSortName() const
    {
      return ((QRJoinPredPtr)this)->getFirstColumn()->getSortName();
    }

    /**
     * Sets the id for this JoinPred element, and creates an entry in the hash
     * table that maps ids to their element objects.
     *
     * @param idNum The id of this JoinPred.
     * @param idHash The id-to-element hash table.
     */
    void setAndRegisterID(NumericID idNum, QRElementHash& idHash)
      {
        QRElement::setID(idNum);
        idHash.insert(&id_, this);
      }

    /**
     * Returns the matching result for this join predicate.
     * @return The matching result.
     */
    const ExprResult getResult() const
      {
        return result_;
      }

    /**
     * Returns the list of equal columns and expressions for this join predicate.
     * @return Equality list for the join predicate.
     */
    const ElementPtrList& getEqualityList() const
      {
        return getList();
      }

    /**
     * Returns an element from the equality list.
     * @return Equality list for the join predicate.
     */
    const QRElementPtr getEqualityListElement(CollIndex i) const
      {
        return getList()[i];
      }

    /**
     * Sets the matching result for this join predicate.
     * @param result The matching result.
     */
    void setResult(ExprResult result)
      {
        result_ = result;
      }

    /**
     * Is this element a join predicate?
     */
    virtual NABoolean isAllowed(QRElementPtr elem)
      {
        return elem->canBeInJoinPred();
      }

    /**
     * Remove an element from the equality set.
     * @param elem The column or expression to remove
     */
    void removeItem(QRElementPtr elem)
      {
        removeElement(elem);
      }

    /**
     * Determines whether this joinpred consists entirely of columns that
     * reference it, and can therefore be removed. In certain rare cases, the
     * same equality set will be referenced in more than one JBB, and we only
     * want to generate a JoinPred for the first one.
     *
     * @return \c TRUE if this joinpred can be eliminated, \c FALSE otherwise.
     */
    NABoolean isRedundant() const;

    /**
     * Returns the first element in the JoinPred's equality set that is a simple
     * column, or NULL if there aren't any columns.
     *
     * @return First column in the equality set of the JoinPred.
     */
    virtual QRColumnPtr getFirstColumn();

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QRJoinPred(const QRJoinPred&);
    QRJoinPred& operator=(const QRJoinPred&);

    // Attributes and contained elements.
    ExprResult result_;
}; // QRJoinPred

/**
 * Class representing a range predicate, defined in an XML document by the
 * <RangePred> element. A range predicate consists of a column or expression
 * that is restricted by the range, and one or more of the following:
 *   - An OpEQ listing one or more discrete values included in the range.
 *   - An OpLS or OpLE representing a subrange without a lower bound.
 *   - An OpGT or OpGE representing a subrange without an upper bound.
 *   - One or more OpBT representing bounded interior subranges.
 * \par
 * Attributes:
 *   - \b id -- Unique identifier for this range predicate.
 *   - \b ref -- Unique id of a range predicate referenced by this one.
 *   - \b sqlType -- The SQL type of the column or expression the range is on.
 *   - \b result -- Matching result for this range predicate.
 *   - \b mustMatch -- Indicates whether the range predicate must be matched
 *                     exactly. Defaults to 0, but is set to 1 in cases where
 *                     it is not possible to provide a compensating predicate
 *                     on top of the MV.
 * \par
 * Subelements:
 *   - \b Column -- The column to which the range restrictions apply. Either
 *                  this or Expr is specified.
 *   - \b Expr -- The expression to which the range restrictions apply. Either
 *                this or Column is specified.
 *   - \b MVColumn -- An MV column used in the rewrite of this range predicate.
 *   - \b OpEQ -- Gives a list of allowed values.
 *   - \b OpLS -- Adds all values less than a specified value to the range.
 *   - \b OpLE -- Adds all values less than or equal to a specified value to the range.
 *   - \b OpGT -- Adds all values greater than a specified value to the range.
 *   - \b OpGE -- Adds all values greater than or equal to a specified value to the range.
 *   - \b OpBT -- Adds all values between two specified values to the range.
 */
class QRRangePred : public QRElement
{
  public:
    static const char elemName[];

    QRRangePred(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_RangePred, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        result_(INVALID_EXPR_RESULT),
        rangeItem_(NULL),
        opList_(heap),
        mustMatch_(FALSE),
        sqlType_(heap),
	rangeSpec_(NULL)
      {}

    QRRangePred(XMLElementPtr parent, AttributeList atts,
                ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QRRangePred();

    virtual QRRangePredPtr  downCastToQRRangePred()
    { 
      return this;
    }

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters

    virtual char getIDFirstChar()
    {
      return 'R';
    }

    /**
     * Returns the textual description of the SQL type of this range predicate.
     * @return The range predicate's type description.
     */
    const NAString& getSqlType() const
      {
        return sqlType_;
      }

    /**
     * Returns the matching result for this range predicate.
     * @return The matching result.
     */
    const ExprResult getResult() const
      {
        return result_;
      }

    /**
     * Returns the column or expression restricted by this range predicate.
     * @return The restricted column or expression.
     */
    QRElementPtr getRangeItem() const
      {
        return rangeItem_;
      }

    /**
     * Returns the list of operators defining this range predicate.
     * @return Operator list for this range predicate.
     */
    const NAPtrList<QRRangeOperatorPtr>& getOperatorList() const
      {
        return opList_;
      }

    /**
     * Returns the indicator of whether or not the range predicate must be
     * matched exactly by a qualifying MV candidate.
     * @return Value of the mustMatch attribute.
     */
    NABoolean getMustMatch() const
      {
        return mustMatch_;
      }

    /**
     * Sets the textual description of the SQL type of this range predicate.
     * @param[in] sqlType The range predicate's type description.
     */
    void setSqlType(const NAString& sqlType)
      {
        sqlType_ = sqlType;
      }

    /**
     * Sets the matching result for this range predicate.
     * @param result The matching result.
     */
    void setResult(ExprResult result)
      {
        result_ = result;
      }

    /**
     * Sets the column or expression that is restricted by this range predicate.
     * @param item The column or expression subject to the range predicate.
     */
    void setRangeItem(QRElementPtr item)
      {
        rangeItem_ = item;
      }

    /**
     * Adds a range operator to the list for this MV.
     * @param op The operator to add to the list.
     */
    void addOperator(QRRangeOperatorPtr op)
      {
        opList_.insert(op);
      }

    /**
     * Set the value indicating whether or not the range predicate must be
     * matched exactly by a qualifying MV candidate.
     * @param mustMatch Value for the mustMatch attribute.
     */
    void setMustMatch(NABoolean mustMatch)
      {
        mustMatch_ = mustMatch;
      }

    const RangeSpec* getRangeSpec(NAMemory* heap);

    NABoolean isSingleValue();

    Int32 getSize();

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QRRangePred(const QRRangePred&);
    QRRangePred& operator=(const QRRangePred&);

    ExprResult result_;
    QRElementPtr rangeItem_;
    NABoolean mustMatch_;
    NAString sqlType_;

    // List of operators contained by this range predicate. If derived from a
    // parsed XML document, the list will be in document order.
    NAPtrList<QRRangeOperatorPtr> opList_;

    // The pointer to the RangeSpec object is used only on the QMS side.
    RangeSpec* rangeSpec_;
}; // QRRangePred

/**
 * Abstract base class for all range predicate operators. At this point, the
 * class is used only to provide a common type that distinguishes these
 * operators from other element types, and does not define any behavior common
 * to them all.
 */
class QRRangeOperator : public QRElement
{
  public:
    virtual ~QRRangeOperator()
      {}

    virtual void unparse(NAString& text, const NAString& rangeItem) = 0;

    virtual Int32 getSize()
    {
      return 1;
    }

  protected:
    QRRangeOperator(ElementType eType, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(eType, NULL, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    QRRangeOperator(ElementType eType, QRElement* parent, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(eType, parent, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

  private:
    // Copy construction/assignment not defined.
    QRRangeOperator(const QRRangeOperator&);
    QRRangeOperator& operator=(const QRRangeOperator&);
}; // QRRangeOperator

/**
 * Class representing a list of enumerated values that are part of the range
 * specified by a range predicate. This is defined in an XML document by the
 * <OpEQ> element. The contained values must all be of the same type, which
 * must be the type of the column that is specified in the range predicate.
 * For any type, a <NullVal> element may also be contained, and indicates that
 * the null value is in the range.
 * \par
 * Attributes: none
 * \par
 * Subelements:
 *   - \b NumericVal -- Used if this object consists of a list of fixed-point
 *                      numbers.
 *   - \b StringVal -- Used if this object consists of a list of strings.
 *   - \b FloatVal -- Used if this object consists of a list of floating-point
 *                    numbers.
 *   - \b NullVal -- Indicates that the null value is an acceptable value.
 */
class QROpEQ : public QRRangeOperator
{
  public:
    static const char elemName[];

    QROpEQ(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRRangeOperator(ET_OpEQ, ADD_MEMCHECK_ARGS_PASS(heap)),
        valueList_(heap),
        nullVal_(NULL)
      {}

    QROpEQ(QRElement *parent, AttributeList atts,
           ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRRangeOperator(ET_OpEQ, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
        valueList_(heap),
        nullVal_(NULL)
      {
        if (*atts)
          throw QRDescriptorException("Invalid attribute specified for element %s: %s",
                                      elemName, *atts);
      }
	
    virtual ~QROpEQ();

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters

    /**
     * Returns list of the values specified by this object.
     * @return List of the values that are part of the overall range.
     */
    const NAPtrList<QRScalarValuePtr>& getValueList() const
      {
        return valueList_;
      }

    /**
     * Indicates whether or not the null value is considered part of the range.
     * This is the case as long as a pointer to a QRNullVal object is present.
     *
     * @return \c TRUE if \c NULL is part of the range, \c FALSE otherwise.
     */
    NABoolean includesNull() const
      {
        return nullVal_ != NULL;
      }

    /**
     * Adds a value to those that are part of the range.
     * @param value The value to add.
     */
    void addValue(QRScalarValuePtr value)
      {
        valueList_.insert(value);
      }

    /**
     * Sets the indicator of whether \c null is included in the range.
     * @param nullVal Pointer to NullVal object; non-null ptr indicates that
     *                \c NULL is included in the range.
     */
    void setNullVal(QRNullValPtr nullVal)
      {
        nullVal_ = nullVal;
      }

    virtual void unparse(NAString& text, const NAString& rangeItem);

    virtual Int32 getSize()
    {
      return valueList_.entries();
    }

  protected:
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QROpEQ(const QROpEQ&);
    QROpEQ& operator=(const QROpEQ&);

    // Attributes and contained elements.
    NAPtrList<QRScalarValuePtr> valueList_;
    QRNullValPtr nullVal_;
};  // QROpEQ

/**
 * Abstract superclass for the inequality operators used in range predicates
 * (<, <=, >, >=). The subclasses of this class all specify a subrange using
 * a single value, which is either the upper or lower inclusive or noninclusive
 * end point of a subrange that is unbounded on the other end.
 * \par
 * Attributes:
 *   - \b isNormalized -- \c TRUE iff the inequality was produced by converting
 *                        a lower or upper bound from a type constraint. When
 *                        deserializing to a RangeSpec object, the constraint
 *                        must be reflected in the range.
 */
class QROpInequality : public QRRangeOperator
{
  public:
    QROpInequality(ElementType eType, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRRangeOperator(eType, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    /**
     * Constructs the common part of an object representing any of the inequality
     * operators used with a range predicate. The value used to specify the range
     * is initially null, and is filled in by a subelement representing one of
     * the value types.
     *
     * @param eType Element type for the specific inequality operator.
     * @param parent Object representing the element that contains this one.
     * @param atts Attribute/value pairs occurring with this element.
     * @param elemName Name of the element. #getElementName can't be used in a
     *                 constructor because it is virtual.
     * @param fileName Name of file the constructor was invoked from.
     * @param lineNum Line number from which the constructor was invoked.
     */
    QROpInequality(ElementType eType, QRElement *parent,
                   AttributeList atts, const char *elemName,
                   ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));
	
    virtual ~QROpInequality();
    virtual const char *getElementName() const = 0;

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters

    /**
     * Returns the value associated with this subrange. The subrange is
     * open on one end, so it is fully specified by a single value.
     * @return Value for the subrange.
     */
    QRScalarValuePtr getValue() const
      {
        return value_;
      }

    /**
     * Sets the value associated with this subrange.
     * @param val The value used for the subrange.
     */
    void setValue(QRScalarValuePtr val)
      {
        value_ = val;
      }

    /**
     * Tells whether this inequality operator is "normalized", i.e., converted
     * from a closed interval derived from a type constraint.
     *
     * @return \c TRUE iff normalized.
     */
    NABoolean isNormalized() const
      {
        return isNormalized_;
      }

    /**
     * Sets or resets the normalization indicator. An inequality operator is
     * considered normalized if it was converted to its inequality from from a
     * closed interval based on a type constraint.
     *
     * @param isNormalized Indicates whether normalized or not.
     */
    void setNormalized(NABoolean isNormalized)
      {
        isNormalized_ = isNormalized;
      }

    virtual void unparse(NAString& text, const NAString& rangeItem);

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);
    virtual char* getOperatorSign() = 0;

  private:
    // Copy construction/assignment not defined.
    QROpInequality(const QROpInequality&);
    QROpInequality& operator=(const QROpInequality&);

    // Attributes and contained elements.
    QRScalarValuePtr value_;
    NABoolean isNormalized_;
}; // QROpInequality

/**
 * Class representing all values of a given type up to but not including a
 * specified value, as part of the overall range specified by a range predicate.
 * This is defined in an XML document by the <OpLS> element. The contained value,
 * which must be of the type of the column that is specified in the range
 * predicate, is the noninclusive upper bound of the range. The range predicate
 * may also include a high end specification using <OpGT> or <OpGE>, a list of
 * specific values using <OpEQ>, and intermediate ranges using <OpBT>.
 * \par
 * Attributes: none
 * \par
 * Subelements:
 *   - \b NumericVal -- Used if this comparison operator involves fixed-point
 *                      numbers.
 *   - \b StringVal -- Used if this comparison operator involves strings.
 *   - \b FloatVal -- Used if this comparison operator involves floating-point
 *                    numbers.
 */
class QROpLS : public QROpInequality
{
  public:
    static const char elemName[];

    QROpLS(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QROpInequality(ET_OpLS, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    QROpLS(QRElement *parent, AttributeList atts,
           ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QROpInequality(ET_OpLS, parent, atts, elemName, ADD_MEMCHECK_ARGS_PASS(heap))
      {}
	
    virtual ~QROpLS()
      {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

  protected:
    virtual char* getOperatorSign()
    {
    	static char sign[] = " < ";
      return sign;
    }

  private:
    // Copy construction/assignment not defined.
    QROpLS(const QROpLS&);
    QROpLS& operator=(const QROpLS&);
}; // QROpLS

/**
 * Class representing all values of a given type up to and including a
 * specified value, as part of the overall range specified by a range predicate.
 * This is defined in an XML document by the <OpLE> element. The contained value,
 * which must be of the type of the column that is specified in the range
 * predicate, is the inclusive upper bound of the low end of the range. The
 * range predicate may also include a high end specification using <OpGT> or
 * <OpGE>, a list of specific values using <OpEQ> and intermediate ranges
 * using <OpBT>.
 * \par
 * Attributes: none
 * \par
 * Subelements:
 *   - \b NumericVal -- Used if this comparison operator involves fixed-point
 *                      numbers.
 *   - \b StringVal -- Used if this comparison operator involves strings.
 *   - \b FloatVal -- Used if this comparison operator involves floating-point
 *                    numbers.
 */
class QROpLE : public QROpInequality
{
  public:
    static const char elemName[];

    QROpLE(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QROpInequality(ET_OpLE, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    QROpLE(QRElement *parent, AttributeList atts,
           ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QROpInequality(ET_OpLE, parent, atts, elemName, ADD_MEMCHECK_ARGS_PASS(heap))
      {}
	
    virtual ~QROpLE()
      {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

  protected:
    virtual char* getOperatorSign()
    {
    	static char sign[] = " <= ";
      return sign;
    }

  private:
    // Copy construction/assignment not defined.
    QROpLE(const QROpLE&);
    QROpLE& operator=(const QROpLE&);
}; // QROpLE

/**
 * Class representing all values of a given type down to but not including a
 * specified value, as part of the overall range specified by a range predicate.
 * This is defined in an XML document by the <OpGT> element. The contained value,
 * which must be of the type of the column that is specified in the range
 * predicate, is the noninclusive lower bound of the high end of the range. The
 * range predicate may also include a low end specification using <OpLS> or
 * <OpLE>, a list of specific values using <OpEQ>,and intermediate ranges
 * using <OpBT>.
 * \par
 * Attributes: none
 * \par
 * Subelements:
 *   - \b NumericVal -- Used if this comparison operator involves fixed-point
 *                      numbers.
 *   - \b StringVal -- Used if this comparison operator involves strings.
 *   - \b FloatVal -- Used if this comparison operator involves floating-point
 *                    numbers.
 */
class QROpGT : public QROpInequality
{
  public:
    static const char elemName[];

    QROpGT(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QROpInequality(ET_OpGT, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    QROpGT(QRElement *parent, AttributeList atts,
           ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QROpInequality(ET_OpGT, parent, atts, elemName, ADD_MEMCHECK_ARGS_PASS(heap))
      {}
	
    virtual ~QROpGT()
      {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

  protected:
    virtual char* getOperatorSign()
    {
    	static char sign[] = " > ";
      return sign;
    }

  private:
    // Copy construction/assignment not defined.
    QROpGT(const QROpGT&);
    QROpGT& operator=(const QROpGT&);
}; // QROpGT

/**
 * Class representing all values of a given type down to and including a
 * specified value, as part of the overall range specified by a range predicate.
 * This is defined in an XML document by the <OpGE> element. The contained value,
 * which must be of the type of the column that is specified in the range
 * predicate, is the inclusive lower bound of the high end of the range. The
 * range predicate may also include a low end specification using <OpLS> or
 * <OpLE>, a list of specific values using <OpEQ>, and intermediate ranges
 * using <OpBT>.
 * \par
 * Attributes: none
 * \par
 * Subelements:
 *   - \b NumericVal -- Used if this comparison operator involves fixed-point
 *                      numbers.
 *   - \b StringVal -- Used if this comparison operator involves strings.
 *   - \b FloatVal -- Used if this comparison operator involves floating-point
 *                    numbers.
 */
class QROpGE : public QROpInequality
{
  public:
    static const char elemName[];

    QROpGE(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QROpInequality(ET_OpGE, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    QROpGE(QRElement *parent, AttributeList atts,
                 ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QROpInequality(ET_OpGE, parent, atts, elemName, ADD_MEMCHECK_ARGS_PASS(heap))
      {}
	
    virtual ~QROpGE()
      {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

  protected:
    virtual char* getOperatorSign()
    {
    	static char sign[] = " >= ";
      return sign;
    }

  private:
    // Copy construction/assignment not defined.
    QROpGE(const QROpGE&);
    QROpGE& operator=(const QROpGE&);
}; // QROpGE

/**
 * Class representing a continuous range of values of a given type, as part of
 * the overall range specified by a range predicate. This is defined in an XML
 * document by the <OpBT> element. The contained values, which must be of the
 * type of the column that is specified in the range predicate, are the inclusive
 * lower and upper ends of a range of  lower bound of the high end of the range. The
 * range predicate may also include a low end specification using <OpLS> or
 * <OpLE>, a list of specific values using <OpEQ>, and intermediate ranges
 * using <OpBT>.
 * \par
 * Attributes:
 *   - \b startIsIncluded -- Indicates whether or not the start value is
 *                           included in the range of values represented.
 *   - \b endIsIncluded -- Indicates whether or not the end value is
 *                         included in the range of values represented.
 * \par
 * Subelements:
 *   - \b NumericVal -- Used if this object defines a range between two 
 *                      fixed-point numbers.
 *   - \b StringVal -- Used if this object defines a range between two strings.
 *   - \b FloatVal -- Used if this object defines a range between two
 *                    floating-point numbers.
 */
class QROpBT : public QRRangeOperator
{
  public:
    static const char elemName[];

    QROpBT(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRRangeOperator(ET_OpBT, ADD_MEMCHECK_ARGS_PASS(heap)),
        valueCount_(0), startValue_(NULL), endValue_(NULL),
        startIsIncluded_(TRUE), endIsIncluded_(TRUE)
      {}

    QROpBT(QRElement *parent, AttributeList atts,
           ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));
	
    virtual ~QROpBT();

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters

    /**
     * Returns the string representation of the value that is the low end of
     * the range.
     * @return The low value of the range.
     */
    QRScalarValuePtr getStartValue() const
      {
        return startValue_;
      }

    /**
     * Returns the string representation of the value that is the high end of
     * the range.
     * @return The high value of the range.
     */
    QRScalarValuePtr getEndValue() const
      {
        return endValue_;
      }

    /**
     * Indicates whether or not this interval is closed on the low end. i.e.,
     * whether the start value is included in the interval.
     * @return \c TRUE if the start value is included in the interval, \c FALSE
     *         otherwise.
     */
    NABoolean startIsIncluded() const
      {
        return startIsIncluded_;
      }

    /**
     * Indicates whether or not this interval is closed on the high end. i.e.,
     * whether the end value is included in the interval.
     * @return \c TRUE if the start value is included in the interval, \c FALSE
     *         otherwise.
     */
    NABoolean endIsIncluded() const
      {
        return endIsIncluded_;
      }

    /**
     * Sets the string representation of the low value of the range.
     * @param start The low value of the range.
     */
    void setStartValue(QRScalarValuePtr start)
      {
        startValue_ = start;
      }

    /**
     * Sets the string representation of the high value of the range.
     * @param end The high value of the range.
     */
    void setEndValue(QRScalarValuePtr end)
      {
        endValue_ = end;
      }

    /**
     * Set the boolean that indicates whether or not the starting value of this
     * interval is included in the interval.
     * @param included \c TRUE if this interval is to be inclusive of the start
     *                 value, \c FALSE otherwise.
     */
    void setStartIncluded(NABoolean included)
      {
        startIsIncluded_ = included;
      }

    /**
     * Set the boolean that indicates whether or not the ending value of this
     * interval is included in the interval.
     * @param included \c TRUE if this interval is to be inclusive of the end
     *                 value, \c FALSE otherwise.
     */
    void setEndIncluded(NABoolean included)
      {
        endIsIncluded_ = included;
      }

    virtual void unparse(NAString& text, const NAString& rangeItem);

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);
    virtual void endElement(void *parser, const char *elementName)
      {
        if (valueCount_ != 2)
          throw QRDescriptorException("Two values must be contained in %s",
                                      elementName);
        QRRangeOperator::endElement(parser, elementName);
      }

  private:
    // Copy construction/assignment not defined.
    QROpBT(const QROpBT&);
    QROpBT& operator=(const QROpBT&);

    Int32 valueCount_;

    // Attributes and contained elements.
    QRScalarValuePtr startValue_;
    QRScalarValuePtr endValue_;
    NABoolean startIsIncluded_;
    NABoolean endIsIncluded_;
};

/**
 * Abstract base class for all scalar values. This class supplies member
 * functions to get and set the string representation of the value regardless
 * of the underlying type. It also provides a common type to use for variables
 * that reference a scalar value of arbitrary type.
 */
class QRScalarValue : public QRExplicitExpr
{
  public:
    virtual ~QRScalarValue()
    {
    }

    /**
     * Returns the string representation of this value.
     * @return The value represented by this object. 
     */
    virtual const NAString& getValue() const
      {
        return stringRep_;
      }

    /**
     * Sets the string representation of the value represented by this object.
     * @param value The value represented by this object.
     */
    virtual void setValue(const NAString& value) = 0;

    /**
     * Create a text representation of the expression.
     * @param text String to append to
     */
    virtual void unparse(NAString& text, NABoolean useColumnName)
    {
      text += stringRep_;
    }

    virtual ItemExpr* toItemExpr(const NAString& mvName, CollHeap* heap,
                                 TableNameScanHash* scanhash);
    virtual void serializeAttrs(XMLString& xml);

    virtual char getIDFirstChar()
    {
      return 'S';
    }

    void setSql(const NAString& sql)
    {
      sql_ = sql;
    }

    const NAString& getSql() const
    {
      return sql_;
    }

  protected:
    QRScalarValue(ElementType eType, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRExplicitExpr(eType, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        stringRep_(charData_),
        sql_(heap)
      {}

    QRScalarValue(ElementType eType, QRElement *parent, AttributeList atts, 
                  ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    QRScalarValue(const QRScalarValue& other, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRExplicitExpr(other, ADD_MEMCHECK_ARGS_PASS(heap)),
        stringRep_(charData_),
        sql_(heap)
      {}

    /**
     * Sets the string representation of the value represented by this object.
     * @param value The value represented by this object.
     */
    void setStringRep(const NAString& value)
      {
        stringRep_ = value;
      }

    /** The string representation of the value. */
    NAString& stringRep_;

    /** 
     * Used for workload analysis for passing the scalar's unparsed text
     * from the compiler, to make sure its parseable.
     * It may be different from the stringRep_ for date/times, intervals,
     strings from specific character sets, etc.
     */
    NAString sql_;

  private:
    // Assignment not defined.
    QRScalarValue& operator=(const QRScalarValue&);
}; // QRScalarValue

/**
 * Class representing a literal fixed numeric value, defined in an XML document
 * by the <NumericVal> element.
 * \par
 * Attributes:
 *   - \b scale -- Scale of the value.
 * \par
 * Subelements: none
 * \par
 * Character data: Textual representation of the numeric value.
 */
class QRNumericVal : public QRScalarValue
{
  public:
    static const char elemName[];

    QRNumericVal(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRScalarValue(ET_NumericVal, ADD_MEMCHECK_ARGS_PASS(heap)),
        unscaledNumericVal_(0), numericScale_(0), scale_(heap)
      {}

    QRNumericVal(QRElement *parent, AttributeList atts,
                 ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    // Copy Ctor
    QRNumericVal(const QRNumericVal& other, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRScalarValue(ET_NumericVal, ADD_MEMCHECK_ARGS_PASS(heap)),
        unscaledNumericVal_(other.unscaledNumericVal_), 
	numericScale_(other.numericScale_), 
	scale_(other.scale_, heap)
      {
        setValue(other.getValue());
      }

    virtual ~QRNumericVal()
      {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

    // Getters/setters

    /**
     * Returns the unscaled value of this numeric value.
     * @return Unscaled integral representation of this value.
     * @see getNumericScale()
     */
    Int64 getUnscaledNumericVal() const
      {
        return unscaledNumericVal_;
      }

    /**
     * Returns the scale of this fixed-point numeric value.
     * @return Scale of the value.
     * @see getUnscaledNumericVal()
     */
    const Int32 getNumericScale() const
      {
        return numericScale_;
      }

    /**
     * Returns the string representation of the scale of this fixed-point
     * numeric value.
     * @return Scale of the value, as a string.
     */
    const NAString& getScale() const
      {
        return scale_;
      }

    /**
     * Sets the string representation of the scale of this fixed-point numeric
     * value. The integer scale value is set to match the string representation
     * passed in.
     * @param scale The scale of the value, as a string.
     */
    void setScale(const NAString& scale)
      {
        scale_ = scale;
        sscanf(scale.data(), "%d", &numericScale_);
      }

    virtual void setValue(const NAString& value);

    void setNumericVal(Int64 unscaledVal, Int32 scale);

    virtual QRExplicitExprPtr deepCopy(subExpressionRewriteHash& subExpressions, CollHeap* heap)
    {
      QRNumericValPtr result = new (heap) QRNumericVal(*this, ADD_MEMCHECK_ARGS(heap));
      return result;
    }

  protected:
    virtual void serializeAttrs(XMLString& xml)
      {
        QRScalarValue::serializeAttrs(xml);
        ((xml += "scale='") += scale_) +=  "\' ";
      }

    virtual void endElement(void *parser, const char *elementName)
      {
        // This has to be done after processing the character content of the
        // element, because that constitutes the string representation of the
        // value.
        synchNumericValue();
        QRScalarValue::endElement(parser, elementName);
      }

  private:
    // Assignment not defined.
    QRNumericVal& operator=(const QRNumericVal&);

    /**
     * Sets the numeric value for this element from the stored character
     * representation.
     */
    void synchNumericValue();

    // Attributes and contained elements.
    Int64 unscaledNumericVal_;
    Int32 numericScale_;
    NAString scale_;
    char buf[50];  // Used for converting value and scale to their string
                   //   representations with sprintf
}; // QRNumericVal

/**
 * Class representing a literal string value, defined in an XML document by the
 * <StringVal> element. The string value may be accessed using the inherited
 * getValue() function.
 * \par
 * Attributes: none
 * \par
 * Subelements: none
 * \par
 * Character data: The string value.
 */
class QRStringVal : public QRScalarValue
{
  public:
    static const char elemName[];

    QRStringVal(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRScalarValue(ET_StringVal, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    QRStringVal(NAString& val, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRScalarValue(ET_StringVal, ADD_MEMCHECK_ARGS_PASS(heap))
      {
        setStringRep(val);
      }

    QRStringVal(QRElement *parent, AttributeList atts,
                ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRScalarValue(ET_StringVal, parent, atts, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    // Copy Ctor
    QRStringVal(const QRStringVal& other, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRScalarValue(ET_StringVal, ADD_MEMCHECK_ARGS_PASS(heap))
      {
        setValue(other.getValue());
      }

    virtual ~QRStringVal()
      {}

    /**
     * Redefines the serialization of this element so that indentation and
     * separate lines for the tags can be avoided. Whitespace can't be stripped
     * from StringVal elements because it may be part of the value, so we have
     * to avoid introducing it in the body of the element.
     *
     * @param xml Text representation of the XML document being serialized.
     * @param sameLine This is ignored; TRUE is passed to the superclass version
     *                 so the whole element will be serialized as a single line.
     * @param dummy Required by interface but not used by this redefinition.
     */
    virtual void toXML(XMLString& xml, NABoolean sameLine = FALSE, NABoolean noEndTag = FALSE);

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual void setValue(const NAString& value)
      {
        setStringRep(value);
      }

    virtual QRExplicitExprPtr deepCopy(subExpressionRewriteHash& subExpressions, CollHeap* heap)
    {
      QRStringValPtr result = new (heap) QRStringVal(*this, ADD_MEMCHECK_ARGS(heap));
      return result;
    }

    /**
     * Redefines the QRElement version of \c charData to avoid stripping
     * whitespace from the character data. A StringVal is serialized so that
     * the content is directly between the start and end tags on the same line.
     * Any leading or trailing whitespace is therefore part of the string value.
     *
     * @param[in] parser Pointer to the instance of the parser being used.
     * @param[in] data   The character data.
     * @param[in] len    The length of the character data.
     */
    virtual void charData(void *parser, const char *data, Int32 len)
      {
        charData_.append(data, len);
      }

    /**
     * This redefinition simply writes the content of the string directly to
     * the serialized XML document, to avoid any addition of whitespace for
     * formatting, as would otherwise be done if \c xml is an instance of
     * XMLFormattedString.
     *
     * @param xml The XML text of the descriptor being written.
     */
    virtual void serializeBody(XMLString& xml)
      {
        xml.append("<![CDATA[").append(charData_).append("]]>");
      }

  private:
    // Assignment not defined.
    QRStringVal& operator=(const QRStringVal&);
};

/**
 * Class representing a Unicode literal string value, defined in an XML document
 * by the <WStringVal> element. The string value may be accessed using the function
 * getWideValue().
 * \par
 * Attributes: none
 * \par
 * Subelements: none
 * \par
 * Character data: The wide string value.
 */
class QRWStringVal : public QRScalarValue
{
  public:
    static const char elemName[];

    QRWStringVal(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRScalarValue(ET_WStringVal, ADD_MEMCHECK_ARGS_PASS(heap)),
        wideStringRep_(heap),
        danglingCharCount_(0)
      {}

    QRWStringVal(NAWString& val, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRScalarValue(ET_WStringVal, ADD_MEMCHECK_ARGS_PASS(heap)),
        wideStringRep_(heap),
        danglingCharCount_(0)
      {
        setWStringRep(val);
      }

    QRWStringVal(QRElement *parent, AttributeList atts,
                ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRScalarValue(ET_WStringVal, parent, atts, ADD_MEMCHECK_ARGS_PASS(heap)),
        danglingCharCount_(0)
      {}

    // Copy Ctor
    QRWStringVal(const QRWStringVal& other, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRScalarValue(ET_WStringVal, ADD_MEMCHECK_ARGS_PASS(heap)),
        danglingCharCount_(0)
      {
        setWideValue(other.getWideValue());
      }

    virtual ~QRWStringVal()
      {}

    /**
     * Redefines the serialization of this element so that indentation and
     * separate lines for the tags can be avoided. Whitespace can't be stripped
     * from WStringVal elements because it may be part of the value, so we have
     * to avoid introducing it in the body of the element.
     *
     * @param xml Text representation of the XML document being serialized.
     * @param sameLine This is ignored; TRUE is passed to the superclass version
     *                 so the whole element will be serialized as a single line.
     * @param dummy Required by interface but not used by this redefinition.
     */
    virtual void toXML(XMLString& xml, NABoolean sameLine = FALSE, NABoolean noEndTag = FALSE);

    virtual const char *getElementName() const
      {
        return elemName;
      }

    /**
     * Overrides the standard definition of this function to throw an exception.
     * This ensures that #setWideValue() is called instead.
     *
     * @see setWideValue()
     * @param value String value.
     */
    virtual void setValue(const NAString& value)
      {
        assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                          FALSE, QRDescriptorException, 
                          "setWideValue() must be used instead of setValue() "
                          "for element QRWStringVal");
      }

    void setWideValue(const NAWString& value)
      {
        setWStringRep(value);
      }

    virtual QRExplicitExprPtr deepCopy(subExpressionRewriteHash& subExpressions, CollHeap* heap)
    {
      QRWStringValPtr result = new (heap) QRWStringVal(*this, ADD_MEMCHECK_ARGS(heap));
      return result;
    }

    /**
     * Redefines the QRElement version of \c charData to avoid stripping
     * whitespace from the character data. A StringVal is serialized so that
     * the content is directly between the start and end tags on the same line.
     * Any leading or trailing whitespace is therefore part of the string value.
     *
     * This redefinition also decodes the 4-hexdigit sequences that represent
     * each Unicode character in the incoming data, and ensures that a character
     * representation that is split across buffers (two separate calls to
     * charData()) is handled correctly.
     *
     * @param[in] parser Pointer to the instance of the parser being used.
     * @param[in] data   The character data, a sequence of hex digits, each
     *                   4 representing the Unicode code point for a character.
     * @param[in] len    The length of the character data in bytes, not Unicode
     *                   characters.
     */
    virtual void charData(void *parser, const char *data, Int32 len);

    /**
     * This redefinition encodes each character in the string to hex, and writes
     * the resulting string of hex digits to the XML document. Each of the two
     * bytes of the Unicode code point is decoded separately, yielding 1 hex
     * digit for each nibble.
     *
     * @param xml The XML text of the descriptor being written.
     */
    virtual void serializeBody(XMLString& xml);

    /**
     * Overrides the standard definition of this function to throw an exception.
     * This ensures that #getWideValue() is called instead.
     *
     * @see getWideValue()
     * @return Empty string.
     */
    virtual const NAString& getValue() const
      {
        assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                          FALSE, QRDescriptorException, 
                          "getWideValue() must be used instead of getValue() "
                          "for element QRWStringVal");
      }

    const NAWString& getWideValue() const
      {
        return wideStringRep_;
      }

    /**
     * Overrides the standard definition of this function to throw an exception.
     * This ensures that #setWStringRep() is called instead.
     *
     * @see setWStringRep()
     * @param value String value.
     */
    virtual void setStringRep(const NAString& value)
      {
        assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                          FALSE, QRDescriptorException, 
                          "setWStringRep() must be used instead of setStringRep() "
                          "for element QRWStringVal");
      }

    /**
     * Sets the string representation of the value represented by this object.
     * @param value The value represented by this object.
     */
    void setWStringRep(const NAWString& value)
      {
        wideStringRep_ = value;
      }

    /** The string representation of the value. */
    NAWString wideStringRep_;

  private:
    // Assignment not defined.
    QRWStringVal& operator=(const QRWStringVal&);

    NAWchar decode_(const char* buf) const;

    /** 
     * The number of hex digits (0 to 3) left over after the buffer passed
     * to charData() has been processed. These are the first n hex digits in
     * the encoding of the next character, to be completed on the next call to
     * #charData().
     */
    short danglingCharCount_;

    /** Buffer holding the characters left over after a call to #charData(). */
    char danglingChars_[4];
};

/**
 * Class representing a literal floating point value, defined in an XML document
 * by the <FloatVal> element.
 * \par
 * Attributes: none
 * \par
 * Subelements: none
 * \par
 * Character data: Textual representation of the floating point value.
 */
class QRFloatVal : public QRScalarValue
{
  public:
    static const char elemName[];

    QRFloatVal(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRScalarValue(ET_FloatVal, ADD_MEMCHECK_ARGS_PASS(heap)),
        floatVal_(0)
      {}

    QRFloatVal(double val, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRScalarValue(ET_FloatVal, ADD_MEMCHECK_ARGS_PASS(heap)),
        floatVal_(val)
      {
        sprintf(buf, "%g", val);
        stringRep_ = buf;
      }

    QRFloatVal(QRElement *parent, AttributeList atts,
               ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRScalarValue(ET_FloatVal, parent, atts, ADD_MEMCHECK_ARGS_PASS(heap)),
        floatVal_(0)
      {}

    // Copy Ctor
    QRFloatVal(const QRFloatVal& other, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRScalarValue(ET_FloatVal, ADD_MEMCHECK_ARGS_PASS(heap)),
        floatVal_(other.floatVal_)
      {
        setValue(other.getValue());
      }

    virtual ~QRFloatVal()
      {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual void setValue(const NAString& value)
      {
        setStringRep(value);
        floatVal_ = atof(value.data());
      }

    double getFloatVal() const
      {
        return floatVal_;
      }

    void setFloatVal(double val)
      {
        floatVal_ = val;

        // Keep string representation in superclass consistent with new value.
        sprintf(buf, "%g", val);
        stringRep_ = buf;
      }

    virtual QRExplicitExprPtr deepCopy(subExpressionRewriteHash& subExpressions, CollHeap* heap)
    {
      QRFloatValPtr result = new (heap) QRFloatVal(*this, ADD_MEMCHECK_ARGS(heap));
      return result;
    }

  protected:
    virtual void charData(void *parser, const char *data, Int32 len)
      {
	QRScalarValue::charData(parser, data, len);
        floatVal_ = atof(stringRep_.data());
      }

  private:
    // Assignment not defined.
    QRFloatVal& operator=(const QRFloatVal&);

    double floatVal_;
    char buf[50];  // Used for converting value to its string rep with sprintf
};

/**
 * Class representing a null value, defined in an XML document by the <NullVal>
 * element. This is a simple marker element with no content or attributes. It
 * is made a class only so the code that tracks element start and end can keep
 * things straight.
 *
 * \par
 * Attributes: none
 * \par
 * Subelements: none
 * \par
 * Character data: none
 * 
 */
class QRNullVal : public QRExplicitExpr
{
  public:
    static const char elemName[];

    QRNullVal(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRExplicitExpr(ET_NullVal, NULL, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    QRNullVal(XMLElementPtr parent, AttributeList atts,
              ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRExplicitExpr(ET_NullVal, parent, ADD_MEMCHECK_ARGS_PASS(heap))
      {
        if (*atts)
          throw QRDescriptorException("Invalid attribute specified for element %s: %s",
                                      elemName, *atts);
      }

    // Copy Ctor
    QRNullVal(const QRNullVal& other, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRExplicitExpr(other, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    virtual ~QRNullVal()
      {}

    /**
     * Redefines the serialization of QRNullVal to use the abbreviated form for
     * an empty element.
     *
     * @param xml The XML string being written.
     * @param sameLine Required by interface, not relevant for this redefinition.
     * @param noEndTag Ignored; TRUE automatically passed to superclass
     *                 implementation of the function.
     */
    virtual void toXML(XMLString& xml, NABoolean sameLine = FALSE, NABoolean noEndTag = FALSE)
      {
        XMLElement::toXML(xml, sameLine, TRUE);
      }

    virtual const char *getElementName() const
      {
        return elemName;
      }

    /**
     * Create a text representation of the expression.
     * @param text String to append to.
     */
    virtual void unparse(NAString& text, NABoolean useColumnName)
    {
      text += "NULL";
    }

    virtual QRExplicitExprPtr deepCopy(subExpressionRewriteHash& subExpressions, CollHeap* heap)
    {
      QRNullValPtr result = new (heap) QRNullVal(*this, ADD_MEMCHECK_ARGS(heap));
      return result;
    }

    virtual ItemExpr* toItemExpr(const NAString& mvName, CollHeap* heap,
                                 TableNameScanHash* scanhash);

  private:
    // Copy assignment not defined.
    QRNullVal& operator=(const QRNullVal&);
};

/**
 * The list of primary GroupBy columns (or expressions).
 * \par
 * Attributes: none
 * \par
 * Subelements:
 *   - \b Column -- A column used with Group By.
 *   - \b MVColumn -- An MV column used with Group By.
 *   - \b Expr -- An expression used with Group By.
 */
class QRPrimaryGroupBy : public QRElementList
{
  public:
    static const char elemName[];

    QRPrimaryGroupBy(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElementList(ET_PrimaryGroupBy, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    QRPrimaryGroupBy(XMLElementPtr parent, AttributeList atts,
               ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QRPrimaryGroupBy()
    {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual char getIDFirstChar()
    {
      return 'G';
    }

  protected:
    virtual void startElement(void *parser, const char *elementName, const char **atts);

    /**
     * Is this element allowed as a GroupBy element?.
     * @return TRUE if the element is allowed as a GroupBy element.
     */
    virtual NABoolean isAllowed(QRElementPtr elem)
    {
      return elem->canBeGroupByItem();
    }

  private:
    // Copy construction/assignment not defined.
    QRPrimaryGroupBy(const QRPrimaryGroupBy&);
    QRPrimaryGroupBy& operator=(const QRPrimaryGroupBy&);

}; // QRPrimaryGroupBy

/**
 * The list of dependent GroupBy columns (or expressions).
 * \par
 * Attributes: none
 * \par
 * Subelements:
 *   - \b Column -- A column used with Group By.
 *   - \b MVColumn -- An MV column used with Group By.
 *   - \b Expr -- An expression used with Group By.
 */
class QRDependentGroupBy : public QRElementList
{
  public:
    static const char elemName[];

    QRDependentGroupBy(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElementList(ET_DependentGroupBy, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    QRDependentGroupBy(XMLElementPtr parent, AttributeList atts,
               ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QRDependentGroupBy()
    {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

    // Getters/setters
    virtual char getIDFirstChar()
    {
      return 'G';
    }

  protected:
    virtual void startElement(void *parser, const char *elementName, const char **atts);

    /**
     * Is this element allowed as a GroupBy element?.
     * @return TRUE if the element is allowed as a GroupBy element.
     */
    virtual NABoolean isAllowed(QRElementPtr elem)
    {
      return elem->canBeGroupByItem();
    }

  private:
    // Copy construction/assignment not defined.
    QRDependentGroupBy(const QRDependentGroupBy&);
    QRDependentGroupBy& operator=(const QRDependentGroupBy&);

}; // QRDependentGroupBy

/**
 * The list of minimal GroupBy columns (or expressions).
 * \par
 * Attributes: none
 * \par
 * Subelements:
 *   - \b Column -- A column used with Group By.
 *   - \b MVColumn -- An MV column used with Group By.
 *   - \b Expr -- An expression used with Group By.
 */
class QRMinimalGroupBy : public QRElementList
{
  public:
    static const char elemName[];

    QRMinimalGroupBy(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElementList(ET_DependentGroupBy, ADD_MEMCHECK_ARGS_PASS(heap))
      {}

    QRMinimalGroupBy(XMLElementPtr parent, AttributeList atts,
               ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QRMinimalGroupBy()
    {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

    // Getters/setters
    virtual char getIDFirstChar()
    {
      return 'G';
    }

  protected:
    virtual void startElement(void *parser, const char *elementName, const char **atts);

    /**
     * Is this element allowed as a GroupBy element?.
     * @return TRUE if the element is allowed as a GroupBy element.
     */
    virtual NABoolean isAllowed(QRElementPtr elem)
    {
      return elem->canBeGroupByItem();
    }

  private:
    // Copy construction/assignment not defined.
    QRMinimalGroupBy(const QRMinimalGroupBy&);
    QRMinimalGroupBy& operator=(const QRMinimalGroupBy&);

}; // QRMinimalGroupBy

/**
 * Class representing a list of grouping columns/expressions in either the hub
 * or the extra-hub, defined in an XML document by the <GroupBy> element.
 * \par
 * Attributes:
 *   - \b id -- Unique id for the grouping list.
 *   - \b ref -- Reference to another group's unique id.
 *   - \b result -- Matching result for the grouping list.
 * \par
 * Subelements:
 *   - \b Column -- A column used with Group By.
 *   - \b MVColumn -- An MV column used with Group By.
 *   - \b Expr -- An expression used with Group By.
 */
class QRGroupBy : public QRElement
{
  public:
    static const char elemName[];

    QRGroupBy(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_GroupBy, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        result_(INVALID_EXPR_RESULT),
        primary_  (new(heap) QRPrimaryGroupBy(ADD_MEMCHECK_ARGS(heap))),
        dependent_(new(heap) QRDependentGroupBy(ADD_MEMCHECK_ARGS(heap))),
        minimal_  (new(heap) QRMinimalGroupBy(ADD_MEMCHECK_ARGS(heap))),
        tableList_(new(heap) QRList<QRTable>(ADD_MEMCHECK_ARGS(heap)))
      {}

    QRGroupBy(XMLElementPtr parent, AttributeList atts,
              ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QRGroupBy();

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual char getIDFirstChar()
    {
      return 'G';
    }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters

    /**
     * Returns the matching result for this grouping list.
     * @return Matching result for the list as a whole.
     */
    ExprResult getResult() const
      {
        return result_;
      }

    /**
     * Sets the matching result for this grouping list.
     * @param result The matching result.
     */
    void setResult(ExprResult result)
      {
        result_ = result;
      }

    /**
     * Returns the list of primary grouping columns/expressions.
     * @return List of grouping items.
     */
    const QRPrimaryGroupByPtr getPrimaryList() const
      {
        return primary_;
      }

    /**
     * Returns the list of dependent grouping columns/expressions.
     * @return List of grouping items.
     */
    const QRDependentGroupByPtr getDependentList() const
      {
        return dependent_;
      }

    /**
     * Returns the list of minimal grouping columns/expressions.
     * @return List of grouping items.
     */
    const QRMinimalGroupByPtr getMinimalList() const
      {
        return minimal_;
      }

    /**
     * Returns the list of tables that are outside the hub.
     * @return List of extra-hub tables.
     */
    QRTableListPtr getTableList() const
      {
        return tableList_;
      }

    void addPrimaryGroupingElement(QRElementPtr elem)
    {
      primary_->addElement(elem);
    }

    void addDependentGroupingElement(QRElementPtr elem)
    {
      dependent_->addElement(elem);
    }

    void addMinimalGroupingElement(QRElementPtr elem)
    {
      minimal_->addElement(elem);
    }

    /**
     * Adds a table to the list of extra-hub tables.
     * @param tbl The table to be added to the list.
     */
    void addTable(QRTablePtr tbl)
    {
      tableList_->addItem(tbl);
    }

    NABoolean isEmpty()
    {
      return primary_== NULL || primary_->entries()==0;
    }

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QRGroupBy(const QRGroupBy&);
    QRGroupBy& operator=(const QRGroupBy&);

    // Attributes and contained elements.

    ExprResult result_;

    // The child elements of a GroupBy are the grouping items, which can be
    // either QRColumn or QRExpr. They are combined into a several lists of
    // QRElements because the order is important when contained in Hub.
    QRPrimaryGroupByPtr    primary_;
    QRDependentGroupByPtr  dependent_;
    QRMinimalGroupByPtr    minimal_;
    QRTableListPtr         tableList_;
}; // QRGroupBy



/**
 * Class representing an expression or column of the select list of a query, or
 * a list of MV columns used in the replacement of a select list expression; 
 * defined in an XML document by the <Output> element.
 * \par
 * Attributes:
 *   - \b id -- Unique id for this output item.
 *   - \b name -- Name of the MV column corresponding to the output expression
 *                (used in MV descriptor only).
 *   - \b ref -- Id of a column or expression referenced by this object.
 *   - \b result -- Matching result for this output item.
 * \par
 * Subelements:
 *   - \b Column -- An output item that is a column.
 *   - \b Expr -- An output item that is an expression.
 *   - \b MVColumn -- An instance of %QROutput may consist of one or more of
 *                    these when it provides rewrite instructions for a select
 *                    list expression.
 */
class QROutput : public QRElementList
{
  public:
    static const char elemName[];

    QROutput(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElementList(ET_Output, ADD_MEMCHECK_ARGS_PASS(heap)),
        name_(heap),
        result_(INVALID_EXPR_RESULT),
        colPos_(-1)
      {}

    QROutput(XMLElementPtr parent, AttributeList atts,
             ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QROutput()
    {}

    virtual QROutputPtr downCastToQROutput()   
    { 
      return this; 
    }

    virtual const char *getElementName() const
      {
        return elemName;
      }

    // Getters/setters

    virtual char getIDFirstChar()
    {
      return 'O';
    }

    /**
     * Returns the name of the MV column corresponding to this output expression.
     * Needed for MV descriptors only.
     * @return Name of the MV column for this output.
     */
    const NAString& getName() const
      {
        return name_;
      }

    /**
     * Returns the matching result for this output item.
     * @return Matching result for the output item.
     */
    ExprResult getResult() const
      {
        return result_;
      }

    /**
     * Return the output column, expression or MV column at the front of the
     * list. In all cases except a NotProvided output expression with partial
     * matching, there is only a single item in the list.
     * @return The output column, expression or MV column at the head of the list.
     */
    QRElementPtr getOutputItem() const
      {
        return (entries() > 0 ? getList()[0] : NULL);
      }

    /**
     * Returns the list of output columns/expressions. Except for certain cases
     * in a result descriptor (see #getOutputItem()), there will be only one item
     * in the list.
     * @return List of output items.
     */
    const ElementPtrList& getOutputItems() const
      {
        return getList();
      }

    /**
     * Returns the index (0 is the first one) of the MV column that derives its
     * value from this output expression. This is used only for MV descriptors.
     * The value will be -1 if the output has not been matched to something in
     * the MV column list. This is an error condition for an MV descriptor.
     * @return 0-based position of the output item in the MV's column list, or
     *         -1 if it has not been matched to an MV column.
     */
    Int32 getColPos() const
      {
        return colPos_;
      }

    /**
     * Sets the name of the MV column corresponding to this output expression.
     * Needed for MV descriptors only.
     * @param name Name of the output's MV column.
     */
    void setName(const NAString& name)
      {
        name_ = name;
      }

    /**
     * Sets the matching result for this output item.
     * @param result The matching result.
     */
    void setResult(ExprResult result)
      {
        result_ = result;
      }

    /**
     * Adds a column, expression, or MV column to the list of components of
     * this output item. This just calls addOutputItem(), and can be used to
     * make the calling code clearer in contexts where only a single item is used.
     *
     * @param elem A column or expression from the select list, or one of the
     *             MV columns needed to rewrite a select list expression.
     */
    void setOutputItem(QRElementPtr elem)
      {
        addElement(elem);
      }

    /**
     * Is this element allowed to be an Output item?
     * @param elem 
     * @return 
     */
    virtual NABoolean isAllowed(QRElementPtr elem)
    {
      return elem->canBeOutputItem();
    }

    /**
     * Sets the position (0 is first) of this output item in the column list of
     * the MV. This is relevant only for MV descriptors.
     * @param colPos The 0-based position of this output item within the MV's
     *               column list.
     */
    void setColPos(Int32 colPos)
      {
        colPos_ = colPos;
      }

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QROutput(const QROutput&);
    QROutput& operator=(const QROutput&);

    // Attributes and contained elements.

    NAString name_;
    ExprResult result_;
    Int32 colPos_;
}; // QROutput

/**
 * Class representing the extra-hub, defined in an XML document by the <ExtraHub>
 * element. The extra-hub consists of the tables outside the hub, which are
 * connected to the hub via cardinality-preserving equijoins, the join predicates
 * to the non-hub tables, and the grouping columns that functionally depend on
 * other columns in the group by list of the hub.
 * \par
 * Attributes: none
 * \par
 * Subelements:
 *   - \b TableList -- List of tables outside the hub.
 *   - \b JoinPredList -- Cardinality-preserving join predicates to the non-hub
 *                        tables.
 *   - \b GroupList -- Grouping columns that functionally depend on the hub's
 *                     grouping columns.
 */
class QRExtraHub : public QRElement
{
  public:
    static const char elemName[];

    QRExtraHub(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_ExtraHub, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        tableList_(new (heap) QRList<QRTable>(ADD_MEMCHECK_ARGS(heap))),
        joinPredList_(new (heap) QRList<QRJoinPred>(ADD_MEMCHECK_ARGS(heap)))
      {}

    QRExtraHub(XMLElementPtr parent, AttributeList atts,
               ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_ExtraHub, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
        tableList_(NULL),
        joinPredList_(NULL)
      {
        if (*atts)
          throw QRDescriptorException("Invalid attribute specified for element %s: %s", 
                                      elemName, *atts);
      }

    virtual ~QRExtraHub()
      {
        deletePtr(tableList_);
        deletePtr(joinPredList_);
      }

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters

    /**
     * Returns the list of tables that are outside the hub.
     * @return List of extra-hub tables.
     */
    QRTableListPtr getTableList() const
      {
        return tableList_;
      }

    /**
     * Returns the list of equijoin predicates linking the extra-hub tables to the hub.
     * @return List of join predicates for extra-hub tables.
     */
    QRJoinPredListPtr getJoinPredList() const
      {
        return joinPredList_;
      }

    /**
     * Adds a table to the list of extra-hub tables.
     * @param tbl The table to be added to the list.
     */
    void addTable(QRTablePtr tbl)
      {
        tableList_->addItem(tbl);
      }

  protected:
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QRExtraHub(const QRExtraHub&);
    QRExtraHub& operator=(const QRExtraHub&);

    // Attributes and contained elements.

    //reason_
    QRTableListPtr tableList_;
    QRJoinPredListPtr joinPredList_;
}; // QRExtraHub


/**
 * Class representing a binary operator, defined in an XML document by the
 * <BinaryOper> element. The operands are specified by subelements, which
 * can include other operators.
 * \par
 * Attributes:
 *   - \b id -- Unique subexpression id assigned to this operator.
 *   - \b op -- Name of this operation.
 * \par
 * Subelements:
 *   - \b Column -- Column operand used as one of this operator's operands.
 *   - \b MVColumn -- MV column used as one of this operator's operands.
 *   - \b BinaryOper -- Binary operator used as one of this operator's operands.
 *   - \b UnaryOper -- Unary operator used as one of this operator's operands.
 *   - \b Function -- Function invocation used as one of this operator's operands.
 */
class QRBinaryOper : public QRExplicitExpr
{
  public:
    static const char elemName[];

    QRBinaryOper(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRExplicitExpr(ET_BinaryOper, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        operator_(heap),
        firstOperand_(NULL),
        secondOperand_(NULL)
      {}

    QRBinaryOper(XMLElementPtr parent, AttributeList atts,
                 ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    // Copy Ctor
    QRBinaryOper(const QRBinaryOper& other, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRExplicitExpr(other, ADD_MEMCHECK_ARGS_PASS(heap)),
        operator_(other.operator_, heap),
        firstOperand_(NULL), 
        secondOperand_(NULL)
      {}

    virtual ~QRBinaryOper()
      {
        deletePtr(firstOperand_);
        deletePtr(secondOperand_);
      }

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    /**
     * Redefines the default implementation of this virtual function to return
     * \c TRUE, since the element represented by this class may appear as the
     * root element within a \c QRExpr element.
     *
     * @return \c TRUE, indicating that this element can be the root of an
     *         expression tree.
     */
    virtual NABoolean canBeExprRoot() const
      {
        return TRUE;
      }

    // Getters/setters

    virtual char getIDFirstChar()
    {
      return 'S';
    }

    /**
     * Returns the text representing the operator given by the \c op attribute.
     *
     * @return The operator's symbolic name.
     */
    const NAString& getOperator() const
      {
        return operator_;
      }

    /**
     * Returns the first of the two operands of this operator. This is the first
     * subelement in the serialization of this class. The order must be maintained
     * in case the operator is not commutative.
     *
     * @return First operand of the operator.
     */
    QRExplicitExprPtr getFirstOperand() const
      {
        return firstOperand_;
      }

    /**
     * Returns the second of the two operands of this operator. This is the second
     * subelement in the serialization of this class. The order must be maintained
     * in case the operator is not commutative.
     *
     * @return Second operand of the operator.
     */
    QRExplicitExprPtr getSecondOperand() const
      {
        return secondOperand_;
      }

    /**
     * Sets the name of this operation.
     * @param[in] opType Name of the operation.
     */
    void setOperator(const NAString& opType)
      {
        operator_ = opType;
      }

    /**
     * Sets the first operand of this binary operator.
     * @param[in] op Pointer to the element object representing this operator's
     *               first operand.
     */
    void setFirstOperand(QRExplicitExprPtr op)
      {
        firstOperand_ = op;
      }

    /**
     * Sets the second operand of this binary operator.
     * @param[in] op Pointer to the element object representing this operator's
     *               second operand.
     */
    void setSecondOperand(QRExplicitExprPtr op)
      {
        secondOperand_ = op;
      }

    /**
     * Create a text representation of the expression.
     * @param text String to append to
     */
    virtual void unparse(NAString& text, NABoolean useColumnName)
    {
      firstOperand_->unparse(text, useColumnName);
      text += ' ';
      text += operator_;
      text += ' ';
      secondOperand_->unparse(text, useColumnName);
    }

    virtual QRExplicitExprPtr deepCopy(subExpressionRewriteHash& subExpressions, CollHeap* heap)
    {
      QRBinaryOperPtr result = new (heap) QRBinaryOper(*this, ADD_MEMCHECK_ARGS(heap));
      result->firstOperand_  = firstOperand_->deepCopyAndSwitch(subExpressions, heap);
      result->secondOperand_ = secondOperand_->deepCopyAndSwitch(subExpressions, heap);
      return result;
    }

    virtual ItemExpr* toItemExpr(const NAString& mvName, CollHeap* heap,
                                 TableNameScanHash* scanhash);

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Assignment not defined.
    QRBinaryOper& operator=(const QRBinaryOper&);

    // Attributes and contained elements.

    NAString operator_;
    QRExplicitExprPtr firstOperand_;
    QRExplicitExprPtr secondOperand_;
}; // QRBinaryOper

/**
 * Class representing a unary operator, defined in an XML document by the
 * <UnaryOper> element. The single operand is specified by a subelement, which
 * can be another operator.
 * \par
 * Attributes:
 *   - \b id -- Unique subexpression id assigned to this operator.
 *   - \b op -- Name of this operation.
 * \par
 * Subelements:
 *   - \b Column -- Column operand used as this operator's operand.
 *   - \b MVColumn -- MV column used as this operator's operand.
 *   - \b BinaryOper -- Binary operator used as this operator's operand.
 *   - \b UnaryOper -- Unary operator used as this operator's operand.
 *   - \b Function -- Function invocation used as this operator's operand.
 */
class QRUnaryOper : public QRExplicitExpr
{
  public:
    static const char elemName[];

    QRUnaryOper(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRExplicitExpr(ET_UnaryOper, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        operator_(heap),
        operand_(NULL)
      {}

    QRUnaryOper(XMLElementPtr parent, AttributeList atts,
                ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));
	
    // Copy Ctor
    QRUnaryOper(const QRUnaryOper& other, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRExplicitExpr(other, ADD_MEMCHECK_ARGS_PASS(heap)),
        operator_(other.operator_, heap),
        operand_(NULL)
      {}

    virtual ~QRUnaryOper()
      {
        deletePtr(operand_);
      }

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    /**
     * Redefines the default implementation of this virtual function to return
     * \c TRUE, since the element represented by this class may appear as the
     * root element within a \c QRExpr element.
     *
     * @return \c TRUE, indicating that this element can be the root of an
     *         expression tree.
     */
    virtual NABoolean canBeExprRoot() const
      {
        return TRUE;
      }

    // Getters/setters

    virtual char getIDFirstChar()
    {
      return 'S';
    }

    /**
     * Returns the text representing the operator given by the \c op attribute.
     *
     * @return The operator's symbolic name.
     */
    const NAString& getOperator() const
      {
        return operator_;
      }

    /**
     * Returns the sole operand of this operator.
     *
     * @return The operand of this operator.
     */
    QRExplicitExprPtr getOperand() const
      {
        return operand_;
      }

    /**
     * Sets the name for this operation.
     * @param opType Name of the operation.
     */
    void setOperator(const NAString& opType)
      {
        operator_ = opType;
      }

    /**
     * Sets the operand of this unary operator.
     * @param op Pointer to the element object representing this operator's
     *           operand.
     */
    void setOperand(QRExplicitExprPtr op)
      {
        operand_ = op;
      }

    /**
     * Create a text representation of the expression.
     * @param text String to append to
     */
    virtual void unparse(NAString& text, NABoolean useColumnName)
    {
      text += operator_;
      text += ' ';
      operand_->unparse(text, useColumnName);
    }

    virtual QRExplicitExprPtr deepCopy(subExpressionRewriteHash& subExpressions, CollHeap* heap)
    {
      QRUnaryOperPtr result = new (heap) QRUnaryOper(*this, ADD_MEMCHECK_ARGS(heap));
      result->operand_  = operand_->deepCopyAndSwitch(subExpressions, heap);
      return result;
    }

    virtual ItemExpr* toItemExpr(const NAString& mvName, CollHeap* heap,
                                 TableNameScanHash* scanhash);

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Assignment not defined.
    QRUnaryOper& operator=(const QRUnaryOper&);

    // Attributes and contained elements.

    NAString operator_;
    QRExplicitExprPtr operand_;
}; // QRUnaryOper

/* Class for a \c &lt;Parameter&gt; element, a subelement of \c &lt;Function&gt;
 * representing a parameter of the function that is not explicitly supplied by
 * the user, but is instead supplied internally to support the way the function
 * is implemented. For example, the \c DAY function has an explicit argument of
 * type date or timestamp, but is implemented using the \c extract_odbc function,
 * which takes an additional parameter that tells which field to extract.
 * \par
 * Attributes:
 *   - \b name -- Name of the parameter.
 *   - \b value -- Value of the parameter.
 */
class QRParameter : public QRElement
{
  public:
    static const char elemName[];

    QRParameter(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_Parameter, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        paramName_(heap), paramValue_(heap)
      {}

    QRParameter(XMLElementPtr parent, AttributeList atts,
                ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));
	
    QRParameter(const QRParameter& other, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(other.getElementType(), NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        paramName_(other.paramName_),
        paramValue_(other.paramValue_)
      {}

    virtual ~QRParameter()
      {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

    /**
     * Redefines the default serialization method to omit the end tag for this
     * content-free element, using the abbreviated element format instead.
     *
     * @param xml Text representation of the XML document being serialized.
     * @param dummy1 Required by interface, value passed doesn't matter.
     * @param dummy2 Required by interface, value passed doesn't matter.
     */
    virtual void toXML(XMLString& xml, NABoolean sameLine = FALSE, NABoolean noEndTag = FALSE)
      {
        XMLElement::toXML(xml, sameLine, TRUE);
      }

    // Getters/setters
    
    const NAString& getName() const
      {
        return paramName_;
      }
      
    const NAString& getValue() const
      {
        return paramValue_;
      }
      
    void setName(const char* name)
      {
        paramName_ = name;
      }
      
    void setValue(const char* strVal)
      {
        paramValue_ = strVal;
      }
      
    void setValue(Int32 intVal)
      {
        char sbuf[20];
        sprintf(sbuf, "%d", intVal);
        paramValue_ = sbuf;
      }
      
  protected:
    virtual void serializeAttrs(XMLString& xml);

  private:
    // Copy assignment not defined.
    QRParameter& operator=(const QRParameter&);

    // Attributes
    NAString paramName_;
    NAString paramValue_;
}; // QRParameter

/**
 * Class representing a function invocation, defined in an XML document by the
 * <Function> element. The arguments are specified by subelements, which can be
 * columns or expressions rooted by an operator element or another function.
 * \par
 * Attributes:
 *   - \b id -- Unique subexpression identifier for this function invocation.
 *   - \b op -- The function name.
 * \par
 * Subelements:
 *   - \b Column -- An argument of this function that is a column.
 *   - \b MVColumn -- An argument of this function that is an MV column.
 *   - \b BinaryOper -- An argument of this function that is an expression
 *                      rooted by a binary operator.
 *   - \b UnaryOper -- An argument of this function that is an expression
 *                     rooted by a unary operator.
 *   - \b Function -- An argument of this function that is an expression
 *                    rooted by another function invocation.
 */
class QRFunction : public QRExplicitExpr
{
  public:
    static const char elemName[];

    QRFunction(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRExplicitExpr(ET_Function, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        function_(heap),
        arguments_(heap),
        hiddenParams_(heap),
	aggregateFunc_(AFT_NONE)
      {}

    QRFunction(XMLElementPtr parent, AttributeList atts,
               ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    // Copy Ctor
    QRFunction(const QRFunction& other, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

public:

    virtual QRFunctionPtr downCastToQRFunction()   
    { 
      return this;
    }

    enum AggregateFunctionType {
      AFT_NONE = 0,
      AFT_COUNTSTAR,
      AFT_COUNT,
      AFT_SUM,
      AFT_MIN,
      AFT_MAX,
      AFT_COUNT_DISTINCT,
      AFT_SUM_DISTINCT,
      AFT_COUNT_ON_GROUPING,
      AFT_SUM_ON_GROUPING,
      AFT_ONE_ROW,
      AFT_ONEROW,
      AFT_ONE_TRUE,
      AFT_ANY_TRUE,
      AFT_INVALID
    };

    virtual ~QRFunction();

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    /**
     * Redefines the default implementation of this virtual function to return
     * \c TRUE, since the element represented by this class may appear as the
     * root element within a \c QRExpr element.
     *
     * @return \c TRUE, indicating that this element can be the root of an
     *         expression tree.
     */
    virtual NABoolean canBeExprRoot() const
      {
        return TRUE;
      }

    // Getters/setters

    virtual char getIDFirstChar()
    {
      return 'S';
    }

    /**
     * The name of this function.
     * @return Name of the function.
     */
    const NAString& getFunctionName() const
      {
        return function_;
      }

    /**
     * Returns a list of all the function arguments.
     * @return List of function arguments.
     */
    const NAPtrList<QRExplicitExprPtr>& getArguments() const
      {
        return arguments_;
      }

    /**
     * Returns a list of the hidden (implicitly added) parameters.
     * @return List of hidden parameters.
     */
    const NAPtrList<QRParameterPtr>& getHiddenParams() const
      {
        return hiddenParams_;
      }

    /**
     * Sets the name of this function.
     * @param fnName Function name to use.
     */
    void setFunctionName(const NAString& fnName)
      {
        function_ = fnName;
      }

    /**
     * Adds an argument to the argument list for this function.
     * @param arg The argument to add to the list.
     */
    void addArgument(QRExplicitExprPtr arg)
      {
        arguments_.insert(arg);
      }

    /**
     * Adds an entry to the list of hidden parameters. These are parameters
     * not externally visible to the user, but supplied internally.
     * @param param The parameter to add to the list.
     */
    void addHiddenParam(QRParameterPtr param)
      {
        hiddenParams_.insert(param);
      }

      /**
       * Set the type of this aggregate function.
       * @param itemExprType the operator type of the corresponding ItemExpr node.
       * @param isDistinct Is this a DISTINCT aggregate function?
       */
      void setAggregateFunc(OperatorTypeEnum itemExprType, NABoolean isDistinct);

       /**
       * Set the type of this aggregate function directly.
       * @param aft the aggregate function type enum.
       */
      void setAggregateFunc(AggregateFunctionType aft)
      {
	aggregateFunc_ = aft;
      }

      /**
       * Get the aggregate function type
       * @return 
       */
      AggregateFunctionType getAggregateFunc()
      {
	return aggregateFunc_;
      }

      /**
       * Is this a DISTINCT aggregate function?
       * @return 
       */
      NABoolean isDistinctAggregate()
      {
	return (aggregateFunc_ == AFT_COUNT_DISTINCT || aggregateFunc_ == AFT_SUM_DISTINCT);
      }

      /**
       * Is this function an aggregate function?
       * @return 
       */
      virtual NABoolean isAnAggregate() const
      {
	return aggregateFunc_ != AFT_NONE;
      }

    /**
     * Create a text representation of the expression.
     * @param text String to append to
     */
    virtual void unparse(NAString& text, NABoolean useColumnName);

    virtual QRExplicitExprPtr deepCopy(subExpressionRewriteHash& subExpressions, CollHeap* heap)
    {
      QRFunctionPtr result = new (heap) QRFunction(*this, ADD_MEMCHECK_ARGS(heap));

      CollIndex maxEntries = arguments_.entries();
      for (CollIndex i=0; i<maxEntries; i++)
      {
	QRExplicitExprPtr arg = arguments_[i]->deepCopyAndSwitch(subExpressions, heap);
	result->addArgument(arg);
      }
      return result;
    }

    virtual ItemExpr* toItemExpr(const NAString& mvName, CollHeap* heap,
                                 TableNameScanHash* scanhash);

    NABoolean isCountStarEquivalent(CollHeap* heap);

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Assignment not defined.
    QRFunction& operator=(const QRFunction&);

    // Attributes and contained elements.

    NAString function_;
    NAPtrList<QRExplicitExprPtr> arguments_;
    NAPtrList<QRParameterPtr> hiddenParams_;
    AggregateFunctionType aggregateFunc_;
}; // QRFunction

/**
 * Class representing an expression, defined in an XML document by the <Expr> or
 * <Residual> element. An instance of this class describes an expression both as
 * an unstructured text string and as nested XML elements that reflect the tree
 * structure of the expression. The body of a serialized <Expr> contains the
 * tree structure, and the text and input columns are derived from this.
 * The expression text is normalized so as to ignore differences in input
 * column naming and thereby enhance matching.
 * \par
 * Attributes:
 *   - \b id -- Unique expression identifier for this expression.
 *   - \b ref -- Id of another expression referenced by this one.
 * \par
 * Subelements:
 *   - \b BinaryOper -- Option for root of the expression tree.
 *   - \b UnaryOper -- Option for root of the expression tree.
 *   - \b Function -- Option for root of the expression tree.
 * \par
 */
class QRExpr : public QRElement
{
  public:
    static const char elemName[];
    static const char residElemName[];

    QRExpr(NABoolean isResidual = FALSE,
           ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_Expr, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        exprRoot_(NULL),
	unparsedTextWithNames_(heap),
	unparsedTextNoNames_(heap),
	inputColumns_(heap),
        isResidual_(isResidual),
        isExtraHub_(FALSE),
        result_(INVALID_EXPR_RESULT),
        info_(NULL)
      {}

    QRExpr(XMLElementPtr parent, AttributeList atts,
           NABoolean isResidual = FALSE,
           ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QRExpr();

    virtual QRExprPtr downCastToQRExpr()     
      { 
        return this; 
      }

    virtual const char* getElementName() const
      {
        return (isResidual_ ? residElemName : elemName);
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    /**
     * Redefines this virtual function to return \c TRUE unless the expression
     * represents a residual predicate, since an expression can be part of the
     * equality set in a join predicate.
     *
     * @return \c FALSE for a residual predicate, TRUE for any other expression.
     */
    virtual NABoolean canBeInJoinPred()
      {
        return !isResidual_;
      }

    /**
     * Redefines the default implementation of this virtual function to return
     * \c TRUE unless the expression represents a residual predicate, since an
     * expression may be an output item.
     *
     * @return \c FALSE for a residual predicate, TRUE for any other expression.
     */
    virtual NABoolean canBeOutputItem() const
      {
        return !isResidual_;
      }

    /**
     * Redefines the default implementation of this virtual function to return
     * \c TRUE unless the expression represents a residual predicate, since an
     * expression may be a <code>Group By</code> item.
     *
     * @return \c FALSE for a residual predicate, TRUE for any other expression.
     */
    virtual NABoolean canBeGroupByItem() const
      {
        return !isResidual_;
      }

    // Getters/setters

    virtual char getIDFirstChar()
      {
        return (isResidual_ ? 'D' : 'X');
      }

    /**
     * Overrides the standard definition of this function to throw an exception.
     * This ensures that #setAndRegisterID() is not bypassed as the way to set
     * the id of an expression.
     *
     * @see setAndRegisterID()
     * @param idNum Id of the expression.
     */
    virtual void setID(NumericID idNum)
      {
        assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                          FALSE, QRDescriptorException, 
                          "setAndRegisterID() must be used instead of setID() "
                          "for element QRExpr");
      }

    /**
     * Sets the id for this expression element, and creates an entry in the hash
     * table that maps the ids to their element object.
     *
     * @param idNum The id of this expression.
     * @param idHash The id-to-element hash table.
     */
    void setAndRegisterID(NumericID idNum, QRElementHash& idHash)
      {
        QRElement::setID(idNum);
        idHash.insert(&id_, this);
      }

    /**
     * Returns the root of the tree structured version of this expression. This
     * could be an instance of QRBinaryOper, QRUnaryOper, or QRFunction.
     * @return Root of the expression tree.
     */
    QRExplicitExprPtr getExprRoot() const
      {
        return exprRoot_;
      }

    /**
     * Assigns the structured expression tree for this expression by setting
     * the pointer to a binary operator that is the root of the tree.
     * @param binOp Binary operator that is the root of the expression tree.
     */
    void setExprRoot(QRExplicitExprPtr elem)
      {
        if (!elem->canBeExprRoot())
          throw QRDescriptorException("Element %s cannot be the root of an expression tree",
                                      elem->getElementName());
        exprRoot_ = elem;
      }

    /**
     * Associates this object with an existing \c Expr or \c JoinPred.
     * @param ref Numeric part of the ID of the referenced element.
     * @param refsJoinPred \c TRUE iff the reference is to a \c JoinPred
     *                     rather than an \c Expr.
     */
    void setRefFromInt(NumericID refNum, NABoolean refsJoinPred = FALSE)
      {
        if (refsJoinPred)
          QRElement::setRefFromInt(refNum, 'J');
        else
          QRElement::setRefFromInt(refNum);
      }

    void setExtraHub(NABoolean eh)
    {
      isExtraHub_ = eh;
    }

    virtual NABoolean isExtraHub()
    {
      return isExtraHub_;
    }

    const NAString& getExprText(NABoolean useColumnName = FALSE);

    CharInfo::CharSet getExprTextCharSet()
    {
      return unparsedTextCharSet_;
    }

    virtual const ElementPtrList& getInputColumns(CollHeap* heap,
                                                  NABoolean useRefedElem = TRUE);

    /**
     * Returns the first input column of the expression.
     * @return the first input column of the expression.
     */
    virtual QRColumnPtr getFirstColumn();

    /**
     * Returns the matching result, only for a residual predicate.
     * @return The matching result.
     */
    ExprResult getResult() const
      {
        assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                          isResidual_, QRDescriptorException,
                          "getResult() only valid for expr used as residual pred.");
        return result_;
      }

    /**
     * Sets the matching result, only for a residual predicate.
     * @param result The matching result.
     */
    void setResult(ExprResult result)
      {
        assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                          isResidual_, QRDescriptorException,
                          "setResult() only valid for expr used as residual pred.");
        result_ = result;
      }

    virtual const NAString& getSortName() const
    {
      QRExprPtr nonConstThis = (QRExprPtr)this;
      return nonConstThis->getExprText(TRUE);
    }
    
    void setResidual(NABoolean isResidual)
    {
      isResidual_ = isResidual;
    }

    void setInfo(QRInfoPtr info)
    {
      info_ = info;
    }

    const QRInfoPtr getInfo() const
    {
      return info_;
    }

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QRExpr(const QRExpr&);
    QRExpr& operator=(const QRExpr&);

    // Attributes and contained elements.
    QRExplicitExprPtr exprRoot_;  // Element can be BinaryOper, Function, UnaryOper

    /** Cached expression text, obtained by unparsing #exprRoot_. */
    NAString unparsedTextWithNames_;
    NAString unparsedTextNoNames_;
    CharInfo::CharSet unparsedTextCharSet_;

    /** Cached list of input columns, obtained by traversing #exprRoot_ with the
     *  #ElementFinderVisitor.
     */
    ElementPtrList inputColumns_;

    /** Indicates whether the element this object represents is a <ResidualPred>
     *  or an ordinary <Expr>.
     */
    NABoolean isResidual_;

    // Temp flag used during descriptor construction.
    NABoolean isExtraHub_;

    ExprResult result_;

    /**
     * Used for workload analysis for passing the expression's unparsed text
     * from the compiler, to make sure its parseable.
     * Other unparsed text data members are generated internally, and 
     * therefore may not be parsable or loyale to the source.
     */
    QRInfoPtr info_;
}; // QRExpr

/**
 * Class representing any miscellaneous information related to an MV descriptor,
 * defined in an XML document by the <MVMisc> element.
 * \par
 * Attributes:
 *   - \b isolationLevel -- The MV isolation level.
 *   - \b isIncremental -- Is this an incremental MV?
 *   - \b isImmediate -- Is this an ON STATEMENT MV?
 * \par
 * Subelements: none yet
 */
class QRMVMisc : public QRElement
{
  public:
    static const char elemName[];

    QRMVMisc(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_MVMisc, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        isolationLevel_(0), // @ZX -- what should default be? 0 is DP2LockFlags::READ_COMMITTED
        isIncremental_(FALSE),
        isImmediate_(FALSE),
        isFromQuery_(FALSE),
	isUMV_(FALSE)
      {}

    QRMVMisc(XMLElementPtr parent, AttributeList atts,
             ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));
	
    virtual ~QRMVMisc()
      {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

    // Getters/setters

    /**
     * Returns the isolation level of the MV.
     * @return MV's isolation level.
     */
    Int32 getIsolationLevel() const
      {
        return isolationLevel_;
      }

    /**
     * Tells whether the MV is incremental or not.
     * @return Return indicator of whether the MV is incremental or not.
     */
    NABoolean isIncremental() const
      {
        return isIncremental_;
      }

    /**
     * Tells whether the MV is immediate (ON STATEMENT).
     * @return Return indicator of whether the MV is immediate or not.
     */
    NABoolean isImmediate() const
      {
        return isImmediate_;
      }

    /**
     * Tells whether the MV is user maintained.
     * @return Return indicator of whether the MV is REFRESH BY USER.
     */
    NABoolean isUMV() const
      {
        return isUMV_;
      }

    /**
     * Sets the isolation level of the MV.
     * @param isolationLevel The isolation level enum value.
     */
    void setIsolationLevel(Int32 isolationLevel)
      {
        isolationLevel_ = isolationLevel;
      }

    /**
     * Set the incremental update indicator for this MV descriptor.
     * @param incremental \c TRUE if this descriptor is for an incremental MV,
     *                    \c FALSE otherwise.
     */
    void setIsIncremental(NABoolean incremental)
      {
        isIncremental_ = incremental;
      }

    /**
     * Set the immediate attribute for this MV.
     * @param incremental \c TRUE if this descriptor is for an incremental MV,
     *                    \c FALSE otherwise.
     */
    void setIsImmediate(NABoolean immediate)
      {
        isImmediate_ = immediate;
      }

    /**
     * Set that this MV is REFRESH BY USER.
     * @param incremental \c TRUE if this descriptor is user maintained,
     *                    \c FALSE otherwise.
     */
    void setIsUMV(NABoolean umv)
      {
        isUMV_ = umv;
      }

    /**
     * Tells whether the MV descriptor is for a query in workload analysis mode.
     * @return 
     */
    NABoolean isFromQuery() const
      {
        return isFromQuery_;
      }

    /**
     * Set whether the MV descriptor is for a query in workload analysis mode.
     * @param incremental 
     */
    void setFromQuery(NABoolean isFromQuery)
      {
        isFromQuery_ = isFromQuery;
      }

  protected:
    virtual void serializeAttrs(XMLString& xml);

    virtual void serializeBody(XMLString& xml)
      {
      }

  private:
    // Copy construction/assignment not defined.
    QRMVMisc(const QRMVMisc&);
    QRMVMisc& operator=(const QRMVMisc&);

    // Attributes and contained elements.

    Int32     isolationLevel_; // Values are from enum DP2LockFlags::ConsistencyLevel in ComTransInfo.h
    NABoolean isIncremental_;
    NABoolean isImmediate_;
    NABoolean isFromQuery_;
    NABoolean isUMV_;
}; // QRMVMisc

/**
 * Class representing the version of a descriptor, defined in an XML document
 * by the <Version> element. The version used is the MV Query Rewrite OFV
 * (Object Feature Version).
 */
class QRVersion : public QRElement
{
  public:
    static const char elemName[];

    QRVersion(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_Version, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        versionString_(charData_)
      {}

    QRVersion(QRElement *parent, AttributeList atts,
              ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_Version, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
        versionString_(charData_)
      {
        if (*atts)
          throw QRDescriptorException("Invalid attribute specified for element %s: %s",
                                      elemName, *atts);
      }
	
    virtual ~QRVersion()
      {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

    // Getters/setters

    /**
     * Returns the version string for this descriptor.
     * @return The descriptor's version.
     */
    const NAString& getVersionString() const
      {
        return versionString_;
      }

    /**
     * Sets the version string for this descriptor.
     * @param version The descriptor's version.
     */
    void setVersionString(NAString& version)
      {
        versionString_ = version; 
      }

  private:
    // Copy construction/assignment not defined.
    QRVersion(const QRVersion&);
    QRVersion& operator=(const QRVersion&);

    // Attributes and contained elements.

    NAString& versionString_;
}; // QRVersion

/**
 * Class representing an MV descriptor, defined in an XML document by the
 * <MVDescriptor> element.
 * \par
 * Attributes: none
 * \par
 * Subelements:
 *   - \b Version -- Version of the descriptor definition.
 *   - \b Table -- Name of the MV.
 *   - \b Misc -- Miscellaneous information pertaining to the descriptor.
 *   - \b JBB -- A join backbone in the query that defines the MV.
 */
class QRMVDescriptor : public QRDescriptor
{
  public:
    QRMVDescriptor(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRDescriptor(ET_MVDescriptor, ADD_MEMCHECK_ARGS_PASS(heap)),
        version_(NULL),
        table_(NULL),
        // Misc must exist so MV creation code can set attributes.
        misc_(new(heap) QRMVMisc(ADD_MEMCHECK_ARGS(heap)))
      {}

    QRMVDescriptor(AttributeList atts, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    // Added to support QRMVDescriptor as a subelement (of <Publish>).
    QRMVDescriptor(XMLElementPtr parent, AttributeList atts, 
                   ADD_MEMCHECK_ARGS_DEF(NAMemory* heap))
      : QRDescriptor(ET_MVDescriptor, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
        version_(NULL),
        table_(NULL),
        misc_(NULL)
      {
        if (*atts)
          throw QRDescriptorException("<%s> should have no attributes; attribute %s specified", 
                                      getElementName(), *atts);
      }

    virtual ~QRMVDescriptor();

    static const char elemName[];

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    /**
     * Returns the version of this MV descriptor.
     * @return Descriptor version object.
     */
    const QRVersionPtr getVersion() const
      {
        return version_;
      }

    /**
     * Returns a pointer to the table object that gives the name of this MV.
     * @return Table instance with name of this MV.
     */
    const QRTablePtr getTable() const
      {
        return table_;
      }

    /**
     * Returns the miscellaneous information pertaining to this MV descriptor.
     * @return The MV's miscellaneous info.
     */
    QRMVMiscPtr getMisc() const
      {
        return misc_;
      }

    /**
     * Sets the version of this MV descriptor.
     * @param version The version of the descriptor.
     */
    void setVersion(QRVersionPtr version)
      {
        version_ = version;
      }

    /**
     * Sets the name of the MV this descriptor is for.
     * @param table Table object that gives the MV name.
     */
    void setTable(QRTablePtr table)
      {
        table_ = table;
      }

    /**
     * Sets the miscellaneous information for this MV descriptor.
     * @param misc The miscellaneous info object.
     */
    void setMisc(QRMVMiscPtr misc)
      {
        misc_ = misc;
      }

    void setMvName(const NAString& name, NAMemory* heap)
    {
      QRTablePtr tableElement = new(heap) QRTable(ADD_MEMCHECK_ARGS(heap));
      tableElement->setTableName(name);
      setTable(tableElement);
    }

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QRMVDescriptor(const QRMVDescriptor&);
    QRMVDescriptor& operator=(const QRMVDescriptor&);

    QRVersionPtr version_;
    QRTablePtr table_;
    QRMVMiscPtr misc_;
}; // QRMVDescriptor

/**
 * Class representing the name of an MV, defined in an XML document by the
 * <MVName> element. 
 * \par
 * Attributes:
 *   - \b id -- Unique identifier of the MV.
 *   - \b TS -- The MV's redefinition timestamp.
 * Subelements: none
 * \par
 * Character data: The fully-qualified name of the MV.
 */
class QRMVName : public QRElement
{
  public:
    static const char elemName[];

    QRMVName(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_MVName, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        timestamp_(heap),
        fqMVName_(charData_)
      {}

    QRMVName(XMLElementPtr parent, AttributeList atts,
             ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));
	
    virtual ~QRMVName()
      {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

    // Getters/setters

    virtual char getIDFirstChar()
    {
      return 'V';
    }

    /**
     * Returns the redefinition timestamp for the MV.
     * @return The view's redefinition timestamp.
     */
    const NAString& getTimestamp() const
      {
        return timestamp_;
      }

    /**
     * Returns the fully-qualified name of the MV.
     * @return The MV's name.
     */
    const NAString& getMVName() const
      {
        return fqMVName_;
      }

    /**
     * Sets the redefinition timestamp for the MV
     * @param ts The MV's redefinition timestamp.
     */
    void setTimestamp(const NAString& ts)
      {
        timestamp_ = ts;
      }

    /**
     * Sets the fully-qualified name of the MV.
     * @param name The MV name.
     */
    void setMVName(const NAString& name)
      {
        fqMVName_ = name;
      }

  protected:
    virtual void serializeAttrs(XMLString& xml);

  private:
    // Copy construction/assignment not defined.
    QRMVName(const QRMVName&);
    QRMVName& operator=(const QRMVName&);

    // Attributes and contained elements.

    NAString timestamp_;
    NAString& fqMVName_;
}; // QRMVName

/**
 * Class representing a candidate MV, defined in an XML document by the <Candidate>
 * element. A candidate MV includes its name, a list of tables needed for
 * back-joins, the extra predicates that need to be applied, and any extra
 * grouping columns and output expressions.
 * \par
 * Attributes:
 *   - \b isPreferredMatch -- Is this candidate MV a preferred (high-ranking) match?
 *   - \b statsOnly -- Is this a statistics-only MV?
 * \par
 * Subelements:
 *   - \b MVName -- Fully-qualified name of the MV.
 *   - \b TableList -- List of tables needed for back-joins.
 *   - \b JoinPredList -- List of extra join predicates needed for rewrite.
 *   - \b RangeList -- List of extra range predicates needed for rewrite.
 *   - \b ResidualList -- List of extra residual predicates needed for rewrite.
 *   - \b GroupBy -- Extra grouping columns needed for rewrite.
 *   - \b OutputList -- Extra output expressions needed for rewrite.
 */
class QRCandidate : public QRElement
{
  public:
    static const char elemName[];

    QRCandidate(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_Candidate, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        isPreferredMatch_(FALSE),
        isIndirectGroupBy_(FALSE),
        statsOnly_(FALSE),
        mvName_(NULL),
        tableList_(new (heap) QRList<QRTable>(ADD_MEMCHECK_ARGS(heap))),
        joinPredList_(new (heap) QRList<QRJoinPred>(ADD_MEMCHECK_ARGS(heap))),
        rangePredList_(new (heap) QRList<QRRangePred>(ADD_MEMCHECK_ARGS(heap))),
        residualPredList_(new (heap) QRList<QRExpr>(ADD_MEMCHECK_ARGS(heap))),
        groupBy_(NULL),
        outputList_(new (heap) QRList<QROutput>(ADD_MEMCHECK_ARGS(heap)))
      {}

    QRCandidate(XMLElementPtr parent, AttributeList atts,
                ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));
	
    virtual ~QRCandidate();

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters

    /**
     * Tells if this candidate MV is rated as a preferred match.
     * @return Value of the \c isPreferredMatch attribute.
     */
    NABoolean isPreferredMatch() const
      {
        return isPreferredMatch_;
      }

    /**
     * Tells if this candidate MV was matched as an indirect Group By.
     * @return Value of the \c isIndirectGroupBy attribute.
     */
    NABoolean isIndirectGroupBy() const
      {
        return isIndirectGroupBy_;
      }

    /**
     * Tells if this candidate MV may only be used for statistical views.
     * @return Value of the \c statsOnly attribute.
     */
    NABoolean isStatsOnly() const
      {
        return statsOnly_;
      }

    /**
     * Returns the fully-qualified name of this candidate MV.
     * @return Name of this candidate MV.
     */
    QRMVNamePtr getMVName() const
      {
        return mvName_;
      }

    /**
     * Returns the list of tables needed for back-joins with this candidate MV.
     * This list consists of all the tables included in the <TableList> element.
     * @return List of tables needed for back-joins.
     */
    QRTableListPtr getTableList() const
      {
        return tableList_;
      }

    /**
     * Returns the list of extra join predicates for this candidate MV. This
     * includes those needed for back-joins.
     * @return List containing the join predicates for this candidate MV.
     */
    QRJoinPredListPtr getJoinPredList() const
      {
        return joinPredList_;
      }

    /**
     * Returns the list of extra range predicates needed for this candidate MV. 
     * @return List containing all the range predicates for this candidate MV.
     */
    QRRangePredListPtr getRangePredList() const
      {
        return rangePredList_;
      }

    /**
     * Returns the list of residual predicates needed for this candidate MV. 
     * @return List containing all the residual predicates for this candidate MV.
     */
    QRResidualPredListPtr getResidualPredList() const
      {
        return residualPredList_;
      }

    /**
     * Returns the object representing the extra grouping columns/expressions
     * to be applied with this candidate MV.
     * @return Pointer to the object representing the <GroupBy> element.
     */
    QRGroupByPtr getGroupBy() const
      {
        return groupBy_;
      }

    /**
     * Returns the list of extra output items for this candidate.
     * @return Extra output items of the candidate.
     */
    QROutputListPtr getOutputList() const
      {
        return outputList_;
      }

    /**
     * Sets indicator of whether or not this candidate MV is considered a
     * preferred match.
     * @param[in] preferredMatch \c TRUE if this MV is a preferred match,
     *                           \c FALSE otherwise.
     */
    void setPreferredMatch(NABoolean preferredMatch)
      {
        isPreferredMatch_ = preferredMatch;
      }

    /**
     * Sets indicator of whether or not this candidate MV was matched as an
     * indirect Group By.
     * @param[in] indirectGroupBy \c TRUE if this MV was matched as an indirect
     *                            Group By, \c FALSE otherwise.
     */
    void setIndirectGroupBy(NABoolean indirectGroupBy)
      {
        isIndirectGroupBy_ = indirectGroupBy;
      }

    /**
     * Establishes this candidate MV as either a statistics-only or a general MV.
     * @param[in] statsOnly \c TRUE if this MV can only be used as a statistical
     *                      view, \c FALSE otherwise.
     */
    void setStatsOnly(NABoolean statsOnly)
      {
        statsOnly_ = statsOnly;
      }

    /**
     * Sets the name of this candidate MV.
     * @param[in] mvName The fully-qualified name of the MV.
     */
    void setMVName(QRMVNamePtr mvName)
      {
        mvName_ = mvName;
      }

    /**
     * Sets the extra grouping columns for this candidate MV.
     * @param[in] groupBy Object representing the MV candidate's extra grouping columns.
     */
    void setGroupBy(QRGroupByPtr groupBy)
      {
        groupBy_ = groupBy;
      }

    /**
     * Add a Table element for the back-join list.
     * @param table 
     */
    void addBackJoinTable(QRTablePtr table)
      {
	tableList_->addItem(table);
      }

    /**
     * Add a Join predicate
     * @param pred 
     */
    void addJoinPred(QRJoinPredPtr pred)
      {
	joinPredList_->addItem(pred);
      }

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QRCandidate(const QRCandidate&);
    QRCandidate& operator=(const QRCandidate&);

    // Attributes and contained elements.

    NABoolean isPreferredMatch_;
    NABoolean isIndirectGroupBy_;
    NABoolean statsOnly_;
    QRMVNamePtr mvName_;
    QRTableListPtr tableList_;
    QRJoinPredListPtr joinPredList_;
    QRRangePredListPtr rangePredList_;
    QRResidualPredListPtr residualPredList_;
    QRGroupByPtr groupBy_;
    QROutputListPtr outputList_;
}; // QRCandidate

/**
 * Class representing a subset of a join backbone, defined in an XML document
 * by the <JbbSubset> element. A JBBSubset includes a list of the tables that
 * comprise the subset, a list of the candidate MVs that can replace the subset,
 * and an optional Group By.
 * \par
 * Attributes:
 *   - \b hasGroupBy -- If 1, indicates that the JBB subset is a superset of the
 *                      query's hub, containing any extra-hub tables needed to
 *                      perform the Group By.
 * \par
 * Subelements:
 *   - \b TableList -- List of tables in the subset.
 *   - \b CandidateList -- List of MVs that are candidates to replace the subset.
 */
class QRJbbSubset : public QRElement
{
  public:
    static const char elemName[];

    QRJbbSubset(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_JbbSubset, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        tableList_(new (heap) QRList<QRTable>(ADD_MEMCHECK_ARGS(heap))),
        candidateList_(new (heap) QRList<QRCandidate>(ADD_MEMCHECK_ARGS(heap))),
        hasGroupBy_(FALSE)
      {}

    QRJbbSubset(XMLElementPtr parent, AttributeList atts,
                ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));
	
    virtual ~QRJbbSubset()
      {
        deletePtr(tableList_);
        deletePtr(candidateList_);
      }

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters

    /**
     * Returns the list of tables in this JBB subset.
     * @return List of the subset's tables.
     */
    QRTableListPtr getTableList() const
      {
        return tableList_;
      }

    /**
     * Sets the list of tables that comprise this JBB subset.
     * @param tblList List of tables in the subset.
     */
    void addTable(QRTablePtr tbl)
      {
        tableList_->addItem(tbl);
      }

    /**
     * Returns the list of candidate MVs to replace this JBB subset.
     * @return The MV candidate list.
     */
    QRCandidateListPtr getCandidateList() const
      {
        return candidateList_;
      }

    /**
     * Sets the list of candidate MV that could replace this JBB subset.
     * @param candidateList The list of candidates.
     */
    void addCandidate(QRCandidatePtr candidate)
      {
        candidateList_->addItem(candidate);
      }

    /**
     * Tells whether or not this JBB subset has a Group By object.
     * @return \c TRUE if a Group By is present, otherwise \c FALSE.
     */
    NABoolean hasGroupBy() const
      {
        return hasGroupBy_;
      }

    /**
     * Sets the JBB subset's Group By object.
     * @param groupBy The Group By object.
     */
    void setGroupBy(NABoolean groupBy)
      {
        hasGroupBy_ = groupBy;
      }

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QRJbbSubset(const QRJbbSubset&);
    QRJbbSubset& operator=(const QRJbbSubset&);

    // Attributes and contained elements.

    QRTableListPtr tableList_;
    QRCandidateListPtr candidateList_;
    NABoolean hasGroupBy_;
}; // QRJbbSubset

/**
 * Class representing the matching results for a join backbone, defined in an
 * XML document by the <JbbResult> element. A JBBResult lists, for a given JBB
 * (identified through the \c ref attribute), any subsets of that JBB with
 * candidate MVs.
 * \par
 * Attributes:
 *   - \b ref -- Unique id of the JBB for which this %JbbResult lists subsets
 *               with matching candidates.
 * \par
 * Subelements:
 *   - \b JBBSubset -- A subset of join backbone children from the referenced
 *                     JBB for which matching candidates are available.
 */
class QRJbbResult : public QRElement
{
  public:
    static const char elemName[];

    QRJbbResult(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_JbbResult, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        jbbSubsets_(heap),
        infoItems_(heap)
      {}

    QRJbbResult(XMLElementPtr parent, AttributeList atts,
                ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));
	
    virtual ~QRJbbResult();

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters

    /**
     * Returns the list of subsets of the referenced JBBs for which matching
     * candidates exist.
     * @return List of JBB subsets covered by this %JBBResult.
     */
    const NAPtrList<QRJbbSubsetPtr>& getJbbSubsets() const
      {
        return jbbSubsets_;
      }

    /**
     * Returns the list of informational messages accompanying this %JBBResult.
     * @return List of informational message objects.
     */
    const NAPtrList<QRInfoPtr>& getInfoItems() const
      {
        return infoItems_;
      }

    /**
     * Add a JBB subset to the list of those with matching candidates.
     * @param jbbSubset The JBB subset to add to the list.
     */
    void addJbbSubset(QRJbbSubsetPtr jbbSubset)
      {
        jbbSubsets_.insert(jbbSubset);
      }

    /**
     * Add an information item to the list for this %JBBResult.
     * @param info The information item to add to the list.
     */
    void addInfoItem(QRInfoPtr info)
      {
        infoItems_.insert(info);
      }

  protected:
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QRJbbResult(const QRJbbResult&);
    QRJbbResult& operator=(const QRJbbResult&);

    // Attributes and contained elements.

    NAPtrList<QRJbbSubsetPtr> jbbSubsets_;
    NAPtrList<QRInfoPtr> infoItems_;
}; // QRJbbResult

/**
 * Class representing a result descriptor, defined in an XML document by the
 * <Result> element. The result descriptor provides the matching results for a
 * query.
 * \par
 * Attributes: none
 * \par
 * Subelements:
 *   - \b JBBResult -- References a JBB and provides information on candidates
 *                     for each subset of that JBB that has a match.
 */
class QRResultDescriptor : public QRElement
{
  public:
    QRResultDescriptor(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_ResultDescriptor, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        version_(NULL),
        jbbResults_(heap)
      {}

    QRResultDescriptor(AttributeList atts, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_ResultDescriptor, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        version_(NULL),
        jbbResults_(heap)
      {
        // Set parent here so we don't have to use 'this' in initializer
        setParent(this);
        if (*atts)
          throw QRDescriptorException("<%s> should have no attributes; attribute %s specified", 
                                      getElementName(), *atts);
      }

    virtual ~QRResultDescriptor();

    static const char elemName[];

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    // Getters/setters for attributes/subelements

    /**
     * Returns the version of this result descriptor.
     * @return Descriptor version object.
     */
    const QRVersionPtr getVersion() const
      {
        return version_;
      }

    /**
     * Returns the list of JBB results that make up this result descriptor.
     * @return List of JBB results.
     */
    const NAPtrList<QRJbbResultPtr>& getJbbResults() const
      {
        return jbbResults_;
      }

    /**
     * Sets the version of this result descriptor.
     * @param version The version of the descriptor.
     */
    void setVersion(QRVersionPtr version)
      {
        version_ = version;
      }

    /**
     * Adds a JBB result to the list for this result descriptor.
     * @param jbbResult The JBB result to add.
     */
    void addJbbResult(QRJbbResultPtr jbbResult)
      {
        jbbResults_.insert(jbbResult);
      }

  protected:
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

  private:
    // Copy construction/assignment not defined.
    QRResultDescriptor(const QRResultDescriptor&);
    QRResultDescriptor& operator=(const QRResultDescriptor&);

    QRVersionPtr version_;
    NAPtrList<QRJbbResultPtr> jbbResults_;
}; // QRResultDescriptor


/**
 * Class corresponding to the <Publish> element, which encapsulates one or more
 * published actions regarding an MV. Each individual action is represented by
 * an <Update> element within <Publish>.
 * 
 * \par
 * Attributes:
 *   - \b TS -- Redefinition timestamp for the MV being published.
 * \par
 * Subelements:
 *   - \b Name -- Name of the published MV.
 *   - \b Update -- One or more of these for individual published actions.
 *   - \b MV -- MV descriptor, used for Create and Republish operations.
 *   - \b Include -- Specifies an included file for the command-line interface.
 */
class QRPublishDescriptor : public QRElement
{
  public:
    static const char elemName[];

    QRPublishDescriptor(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_PublishDescriptor, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        redefTimestamp_(heap),
        name_(NULL),
        updateList_(heap),
        includeFile_(NULL),
        mvDesc_(NULL),
	mvDescText_(NULL)
      {}

    /**
     * Creates an instance of QRPublishDescriptor that references itself as parent,
     * since it is always used as a document (outermost) element.
     *
     * @param[in] atts List of attribute/value pairs.
     * @param[in] fileName Name of the file from which this constructor was called.
     * @param[in] lineNum Line number at which this constructor was called.
     */
    QRPublishDescriptor(AttributeList atts,
                        ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QRPublishDescriptor();

    virtual const char *getElementName() const
      {
        return elemName;
      }

    virtual NABoolean treeWalk(VisitorPtr visitor);

    void initialize(ComPublishMVOperationType opType,
		    const NAString*	      redefTimestamp,
		    NAString* 	              mvDescText, 
		    const NAString*	      mvName,
		    NABoolean		      hasIgnoreChanges,
		    const NAString*	      refreshTimestamp,
		    const NAString*	      newName,
		    NAMemory*		      heap);

    // Getters/setters

    /**
     * Returns the redefinition timestamp for the MV that is the subject of
     * this publish message
     *
     * @return The MV's redefinition timestamp.
     */
    const NAString& getRedefTimestamp() const
      {
        return redefTimestamp_;
      }

    /**
     * Returns the name of the MV that is the subject of this publish message.
     * @return The MV's name.
     */
    const NAString& getName() const
      {
	if (name_)
          return name_->getMVName();
	else
	{
	  assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                            mvDesc_ != NULL, QRDescriptorException,
			    "In Publish descriptor, expecting <MV> When <MVName> not supplied.");
	  return mvDesc_->getTable()->getTableName();
	}
      }

    /**
     * Returns the list of Update operations contained in this Publish message.
     * @return The list of Updates.
     */
    const NAPtrList<QRUpdatePtr>& getUpdateList() const
      {
        return updateList_;
      }

    /**
     * Returns the object representing the include file in the content of the
     * message (command-line interface only).
     *
     * @return The name of the include file.
     */
    const QRIncludePtr getIncludeFile() const
      {
        return includeFile_;
      }

    /**
     * Returns the MV descriptor being published. The MV descriptor appears only
     * in conjunction with a Create or Republish of an MV.
     *
     * @return The MV descriptor used in this Publish message.
     */
    const QRMVDescriptorPtr getMV() const
      {
        return mvDesc_;
      }

    /**
     * Returns the MV descriptor being published as an NAString. The MV descriptor appears only
     * in conjunction with a Create or Republish of an MV.
     *
     * @return The MV descriptor used in this Publish message as an NAString.
     */
    const NAString* getMVText() const
      {
        return mvDescText_;
      }

    /**
     * Sets the redefinition timestamp of the MV this publish message is for.
     * @param redefTimestamp The published MV's redefinition timestamp.
     */
    void setRedefTimestamp(const NAString& redefTimestamp)
      {
        redefTimestamp_ = redefTimestamp;
      }

    /**
     * Sets the name of the MV this publish message is for.
     * @param name The published MV's name.
     */
    void setName(const QRMVNamePtr name)
      {
        name_ = name;
      }

    /**
     * Adds an Update operation to the list for this Publish message.
     * @param update The update to add to the list.
     */
    void addUpdate(QRUpdatePtr update)
      {
        updateList_.insert(update);
      }

    /**
     * Sets the include file used in the content of the message
     * (command-line interface only).
     *
     * @param Pointer to object representing the include file.
     */
    void setIncludeFile(QRIncludePtr includeFile)
      {
        includeFile_ = includeFile;
      }

    /**
     * Sets the MV descriptor used with this Publish message. This element is
     * used only for doing a Create or Republish on an MV.
     *
     * @param mvDesc Pointer to the MV descriptor.
     */
    void setMV(QRMVDescriptorPtr mvDesc)
      {
        mvDesc_ = mvDesc;
      }

    /**
     * Sets the MV descriptor string used with this Publish message. This element is
     * used only for doing a Create or Republish on an MV.
     *
     * @param mvDescText Pointer to the MV descriptor as character data.
     * @param heap Heap to use for allocation
     */
    void setMVText(NAString* mvDescText, NAMemory* heap)
      {
        if (mvDescText)
           mvDescText_ = new (heap) NAString (*mvDescText);
      }

  protected:
    QRUpdatePtr createUpdateForAlter(NABoolean hasIgnoreChanges, NAMemory* heap);
    QRUpdatePtr createUpdateForRefresh(const NAString* refreshTimestamp, NAMemory* heap);
    QRUpdatePtr createUpdateForRecompute(const NAString* refreshTimestamp, NAMemory* heap);
    QRUpdatePtr createUpdateForDrop(NAMemory* heap);
    QRUpdatePtr createUpdateForRename(const NAString* newName, NAMemory* heap);

  protected:
    virtual void serializeAttrs(XMLString& xml);
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);

    NAString redefTimestamp_;
    QRMVNamePtr name_;
    NAPtrList<QRUpdatePtr> updateList_;
    QRIncludePtr includeFile_;  //@ZXpub -- allow more than one?
    QRMVDescriptorPtr mvDesc_;
    NAString* mvDescText_; 

  private:
    // Copy construction/assignment not defined.
    QRPublishDescriptor(const QRPublishDescriptor&);
    QRPublishDescriptor& operator=(const QRPublishDescriptor&);
}; // QRPublishDescriptor

/**
 * Class corresponding to the <Update> element, which recurs within a <Publish>
 * for each operation to apply to the MV named by the publish message.
 *
 * 
 * \par
 * Attributes:
 *   - \b op -- 
 *   - \b TS -- 
 *   - \b hasIgnoreChanges -- 
 * \par
 * Subelements: none
 * \par
 * Character data: The new name of the MV (only for op='Rename').
 */
class QRUpdate : public QRElement
{
  public:
    // The order of these enum values must match the corresponding names in
    // UpdateTypeNames_.
    enum UpdateType { NON_INIT=0, ALTER, DEFAULT, DROP, RECOMPUTE, REFRESH, RENAME };

    // Remove????????
    //QRUpdate(XMLElementPtr parent, ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
    //  : QRElement(ET_Update, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
    //    type_(NON_INIT),
    //    timestamp_(heap),
    //    hasIgnoreChanges_(FALSE),
    //	newName_(charData_)
    //  {}

    QRUpdate(XMLElementPtr parent, 
             AttributeList atts,
             ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    /**
     * Creates an update descriptor to be used to publish an MV alter, drop,
     * or rename.
     *
     * @param operationType The MV operation that was performed.
     * @param heap Heap to use for any allocations.
     */
    QRUpdate(UpdateType operationType,
             ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0));

    virtual ~QRUpdate()
      {}

    static const char elemName[];

    virtual const char *getElementName() const
      {
        return elemName;
      }

    // Getters/setters for attributes/subelements
    UpdateType getUpdateType()
    {
      return type_;
    }

    void setUpdateType(UpdateType type) 
    {
      type_ = type;
    }

    const NAString& getRefreshTimestamp()
    {
      assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                        type_ == REFRESH || type_ == RECOMPUTE, QRDescriptorException,
			"Refresh timestamp can only be used in Refresh or Recompute Update elements.");
      return timestamp_;
    }

    void setRefreshTimestamp(const NAString& ts)
    {
      assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                        type_ == REFRESH || type_ == RECOMPUTE, QRDescriptorException,
			"Refresh timestamp can only be used in Refresh or Recompute Update elements.");
      timestamp_ = ts;
    }

    const NAString& getNewName()
    {
      assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                        type_ == RENAME, QRDescriptorException,
			"MV new name can only be used in Rename Update elements.");
      return newName_;
    }

    void setNewName(const NAString& name)
    {
      assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                        type_ == RENAME, QRDescriptorException,
			"MV new name can only be used in Rename Update elements.");
      newName_ = name;
    }

    const NABoolean getIgnoreChanges()
    {
      assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                        type_ == ALTER, QRDescriptorException,
			"Ignore changes flag can only be used in Alter Update elements.");
      return hasIgnoreChanges_;
    }

    void setIgnoreChanges(NABoolean ic)
    {
      assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                        type_ == ALTER, QRDescriptorException,
			"Ignore changes flag can only be used in Alter Update elements.");
      hasIgnoreChanges_ = ic;
    }

    void setDefaultValue(const NAString& name, const NAString& value)
    {
      assertLogAndThrow(CAT_SQL_COMP_QR_DESC_GEN, LL_MVQR_FAIL,
                        type_ == DEFAULT, QRDescriptorException,
			"Defaults can only be used in Default Update elements.");
      defaultName_  = name;
      defaultValue_ = value;
    }

    const NAString& getDefaultName()
    {
      return defaultName_;
    }

    const NAString& getDefaultValue()
    {
      return defaultValue_;
    }

  protected:
    virtual void serializeAttrs(XMLString& xml);

private:
  static const char* const UpdateTypeNames_[];

  UpdateType type_;
  NAString   timestamp_;
  NABoolean  hasIgnoreChanges_;
  NAString&  newName_;
  NAString   defaultName_;
  NAString   defaultValue_;
};  // class QRUpdate

/**
 * Class representing the deserialized form of an <Include> element, which is
 * used within the <Publish> element to give the name of an include file. This
 * is used only with the command-line interface to QMS.
 *
 * \par
 * Attributes: none
 * \par
 * Subelements: none
 * \par
 * Character data: The name of the include file.
 */
class QRInclude : public QRElement
{
  public:
    static const char elemName[];

    QRInclude(ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_Include, NULL, ADD_MEMCHECK_ARGS_PASS(heap)),
        fileName_(charData_)
      {}

    QRInclude(XMLElementPtr parent, AttributeList atts,
          ADD_MEMCHECK_ARGS_DECL(NAMemory* heap = 0))
      : QRElement(ET_Include, parent, ADD_MEMCHECK_ARGS_PASS(heap)),
        fileName_(charData_)
    {}
	
    virtual ~QRInclude()
      {}

    virtual const char *getElementName() const
      {
        return elemName;
      }

    // @ZX -- for use with NAHashDictionary
    Int32 operator==(const QRInclude &rhs) const
      {
        return fileName_ == rhs.fileName_;
      }

    // Getters/setters

    /**
     * Returns the name of the include file.
     * @return The file name.
     */
    const NAString& getFileName() const
      {
        return fileName_;
      }

    /**
     * Sets the name of the include file.
     * @param fileName The include file name.
     */
    void setFileName(const NAString& fileName)
      {
        fileName_ = fileName;
      }

  private:
    // Copy construction/assignment not defined.
    QRInclude(const QRInclude&);
    QRInclude& operator=(const QRInclude&);

    NAString& fileName_;
}; // QRInclude

#endif   /* _QRDESCRIPTOR_H_ */
