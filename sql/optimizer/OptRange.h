/**********************************************************************
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
**********************************************************************/

#ifndef _OPTRANGE_H_
#define _OPTRANGE_H_

#include "Range.h"
#include "QRDescriptor.h"
#include "nawstring.h"
#include "QRLogger.h"

/**
 * \file
 * Contains an extension of the RangeSpec class used in identifying range
 * predicates when building query or MV descriptors.
 */

// Classes defined in this file.
class OptRangeSpec;
class   OptNormRangeSpec;
class RangeInfo;

// Other forward declarations.
class QRDescGenerator;

/**
 * \file
 * Contains the class definition for the class used to generate a query
 * descriptor from Analyzer structures.
 */
 
/**
 * Extends RangeSpec to provide capabilities needed for MVQR descriptor
 * generation.
 */
class OptRangeSpec : public RangeSpec
{
  public:
    OptRangeSpec(QRDescGenerator* descGenerator, CollHeap* heap = NULL, logLevel ll = LL_MVQR_FAIL);

    /**
     * Creates a copy of the passed object. This copy ctor is used by the
     * virtual object copier function clone(), and may also be invoked directly
     * without fear of "slicing" (failing to produce a copy of the most specific
     * type when the copy is made from a reference to a superclass), since this
     * class has no subclasses. If %OptRangeSpec ever sprouts a subclass, this
     * ctor should probably be made protected.
     *
     * @param other The object to make the copy of.
     */
    OptRangeSpec(const OptRangeSpec& other, CollHeap* heap = NULL);

    virtual ~OptRangeSpec()
      {}

    virtual NABoolean forNormalizer() const
      {
        return FALSE;  // overridden by OptNormRangeSpec
      }

    /**
     * Returns a deep copy of this range spec. Use this instead of the copy
     * ctor, which is protected.
     *
     * @param heap Heap to use for the new object. If \c NULL (the default),
     *             the heap used for this (the source) object will be used for
     *             the new one. Note that this precludes cloning an %OptRangeSpec
     *             from a private heap on the system heap.
     * @return Deep copy of this range spec.
     */
    virtual OptRangeSpec* clone(CollHeap* heap = NULL) const
      {
        if (!heap)
          heap = mvqrHeap_;
        return new(heap) OptRangeSpec(*this, heap);
      }

    /**
     * Creates an instance of OptRangeSpec if a range can be derived from #predExpr.
     * This is generally the case when the predicate is a disjunction of
     * comparisons of a single column or expression to literals, such as
     * <code>c=10 or c=20 or c between 50 and 60</code>. If the predicate is
     * not of a form suitable for deriving the allowable range of values for
     * a column or expression, this function returns \c NULL, and no %OptRangeSpec
     * is instantiated.
     *
     * @param descGenerator The QRDescGenerator currently being produced.
     * @param predExpr The predicate to evaluate for creating the range.
     * @param heap The heap to allocate from.
     * @param createForNormalizer If \c TRUE, instantiate an OptNormRangeSpec
     *                            instead of an OptRangeSpec.
     * @return Pointer to a new %OptRangeSpec object if one is created.
     */
    static OptRangeSpec* createRangeSpec(QRDescGenerator* descGenerator,
                                         ItemExpr* predExpr,
                                         CollHeap* heap,
                                         NABoolean createForNormalizer = FALSE);

    void setID(ValueId vid)
      {
        vid_ = vid;
      }

    ValueId getID() const
      {
        return vid_;
      }

    /**
     * Generates a \c &lt;Range&gt; element from this range specification.
     * @return Pointer to the QRRangePred created.
     */
    QRRangePredPtr createRangeElem();

    ValueId getBaseCol(const ValueIdSet& vegMembers);
    ValueId getBaseCol(const ValueId vegRefVid);

    /**
     * Creates a subrange and adds it to this range specification, taking into
     * account possible overlap with existing subranges. If the subrange would
     * be wholly included in an existing one, it is ignored. If it extends an
     * existing subrange, they are merged to a single one. If a new subrange is
     * created, #placeSubrange is called to place it at the proper location in
     * the array.
     *
     * @param start Pointer to \c ConstValue object containing the starting
     *              value for the new subrange.
     * @param end Pointer to \c ConstValue object containing the ending
     *            value for the new subrange.
     * @param startInclusive \c TRUE if the start value is included in the
     *                       subrange. Use \c TRUE for < or <=; although it has
     *                       no effect on the use of the range, this will cause
     *                       the external display format of the range to
     *                       indicate an interval that is closed on the lower end.
     * @param endInclusive \c TRUE if the end value is included in the subrange.
     *                     Use \c TRUE for > or >=; although it has no effect on
     *                     the use of the range, this will cause the external
     *                     display format of the range to indicate an interval
     *                     that is closed on the upper end.
     * @see #placeSubrange
     */
    void addSubrange(ConstValue* start, ConstValue* end,
                     NABoolean startInclusive, NABoolean endInclusive);

    /**
     * Returns the subtree of a \c CheckConstraint item expression that
     * represents the actual predicate the constraint is based on.
     *
     * @param checkConstraint The overall check constraint item expression.
     * @return The constraint predicate item expression.
     */
    ItemExpr* getCheckConstraintPred(ItemExpr* checkConstraint);

    /**
     * Applies any check constraints or the constraint implied by a data type
     * to this range spec. This is done by intersecting the range conditions
     * corresponding to those constraints to this range spec.
     *
     * @param descGen The QRDescGenerator to use to create the range specs to
     *                intersect with this one.
     */
    void addConstraints(QRDescGenerator* descGen);

    /**
     * Finds the check constraint that apply to the column this range spec is
     * on, and intersects any ranges that can be derived from these constraints
     * with this range spec.
     *
     * @param descGen QRDescGenerator used to create range specs for the
     *                constraints.
     * @param colValId Column this range spec is on.
     */
    void intersectCheckConstraints(QRDescGenerator* descGen, ValueId colValId);

    /**
     * Creates a %RangeSpec for the value constraint implied by range column's
     * type, and intersects it with this range spec.
     *
     * @param descGen QRDescGenerator used to create range specs for the type
     *                constraint.
     * @param colValId Column this range spec is on.
     */
    void intersectTypeConstraint(QRDescGenerator* descGen, ValueId colValId);

    /**
     * Generates the QRElement (either a column or expression) that is the item
     * this range spec applies to.
     * @return The element representing the subject of the range.
     */
    QRElementPtr genRangeItem();

    /**
     * Returns pointer to the item expression the range applies to. If the range
     * is for a column rather than an expression, the ValueId of the vegref (if
     * the column is part of an equality set), or the ValueId of the column
     * itself (if not part of an eq set) is used to get the \c ItemExpr* to return.
     *
     * @return Pointer to item expression the range is on.
     */
    ItemExpr* getRangeExpr() const;

    QRValueId getRangeExprValueId() const
      {
        return (rangeExpr_ ? rangeExpr_->getValueId() : NULL_VALUE_ID);
      }

    /**
     * Records the item expression this range applies to. Used only if the range
     * is on an expression instead of a simple column.
     *
     * @param itemExpr Pointer to the item expression the range is on.
     */
    void setRangeExpr(ItemExpr* itemExpr)
      {
        rangeExpr_ = itemExpr;
        itemExpr->unparse(rangeExprText_, OPTIMIZER_PHASE, MVINFO_FORMAT);
      }

    /**
     * Returns an \c ItemExpression which is the simplified version of the
     * original expression. It is derived directly from this range object and
     * therefore may differ from the original by having consecutive ORed values
     * expressed as a subrange, adjacent or overlapping subranges coalesced, etc.
     * The expression is recalculated on demand rather than being cached. The
     * returned pointer is to a newly allocated \c ItemExpression which is
     * owned by the caller.
     *
     * @param normWA Pointer to Normalizer work area. Not used in this
     *               implementation of the virtual function.
     * @return Pointer to an \c ItemExpression representing the simplified
     *         range condition.
     */
    virtual ItemExpr* getRangeItemExpr(NormWA* normWA = NULL) const;

    /**
     * Asserts that the other range is on the same column or expression as this
     * one, and then calls the parent version of this function to do the actual
     * range intersection.
     *
     * @param other The RangeSpec to conjoin to this one.
     */
    virtual void intersectRange(OptRangeSpec* other);

    /**
     * Asserts that the other range is on the same column or expression as this
     * one, and then calls the parent version of this function to do the actual
     * range union.
     *
     * @param other The RangeSpec to union with this one.
     */
    virtual void unionRange(OptRangeSpec* other);

    /**
     * Indicates whether or not the column or expression the range is for has
     * been established.
     *
     * @return TRUE iff the subject column or expression has been set.
     */
    NABoolean rangeSubjectIsSet()
      {
        return rangeColValueId_ != NULL_VALUE_ID  || rangeExpr_ != NULL;
      }

    /**
     * Set the column or expression that the range applies to so that it is
     * the same as \c otherRange.
     *
     * @param otherRange The range spec to set this range spec's subject
     *                   column or exrpession from.
     */
    void setRangeSubject(OptRangeSpec* otherRange)
      {
        assertLogAndThrow(CAT_SQL_COMP_RANGE, logLevel_,
                          !rangeSubjectIsSet(), QRLogicException,
                          "setRangeSubject(): range already has a subject");
        rangeColValueId_ = otherRange->rangeColValueId_;
        rangeExpr_ = otherRange->rangeExpr_;
        setType(otherRange->getType());
      }

    /**
     * Indicates whether or not this range spec has the same subject column
     * or expression as that of the argument range spec.
     *
     * @param otherRange The range spec to compare to this one.
     * @return TRUE iff \c otherRange applies to the same column or expression
     *         as this one.
     */
    NABoolean sameRangeSubject(OptRangeSpec* otherRange)
      {
        return rangeSubjectIsSet() &&
               rangeColValueId_ == otherRange->rangeColValueId_ &&
               rangeExpr_ == otherRange->rangeExpr_;
      }

    /**
     * Adds all columns used in this range specification to the ValueIdSet
     * that indicates the columns used in the specified descriptor generator.
     *
     * @param descGen Descriptor generator to add the columns to the hash
     *                table of.
     */
    void addColumnsUsed(const QRDescGenerator* descGen);

  protected:
    /**
     * Evaluate a predicate in tree form to derive the contents of this range
     * specification.
     *
     * @param predExpr Predicate to build the range specification from.
     * @return \c TRUE if a range predicate could be derived from \c predExpr,
     *         \c FALSE otherwise.
     */
    NABoolean buildRange(ItemExpr* predExpr);

    /**
     * Generates a left-linear OR backbone of equality predicates corresponding
     * to the values within this subrange. This is an alternative to an AND of
     * >= and <= predicates used when the subrange originated as an IN list or a
     * sequence of equality predicates.
     *
     * @param subrange The subrange to generate the ORed equality predicates for.
     * @param subrangeItem The \c ItemExpr the subrange applies to.
     * @return Item expression for the OR backbone.
     */
    ItemExpr* makeSubrangeORBackbone(SubrangeBase* subrange,
                                     ItemExpr* subrangeItem) const;

    /**
     * Creates and returns an \c ItemExpr representing the specified subrange.
     * Ideally, this would be a SubrangeBase member function, but subrange is
     * defined in qmscommon, and can not depend on \c ItemExpr.
     *
     * @param subrange The subrange do derive the \c ItemExpr from.
     * @param subrangeItem The column or expression the subrange applies to.
     * @return The \c ItemExpr representing the subrange.
     */
    ItemExpr* makeSubrangeItemExpr(SubrangeBase* subrange,
                                   ItemExpr* subrangeItem) const;

    /**
     * Given an item expression subtree with a comparison operator at its root,
     * returns a pointer to a constant value item expression which is one of its
     * operands, or \c NULL if a constant is not involved. If this is the first
     * such expression considered in evaluating a predicate for a range, the
     * column used in the expression is set as the column the range is on.
     * Otherwise, the column used must correspond to the column previously
     * established as the range column. It is assumed that the expression has
     * been converted to a normal form in which the constant, if present, is
     * the second operand. If the wrong column is used, or if there is no
     * constant, \c NULL is returned, effectively eliminating #predExpr as a
     * source of a range constraint.
     *
     * @param predExpr Pointer to the item expression subtree with a comparison
     *                 operator at the root.
     * @param constInx Index of the presumed constant. This defaults to one, but
     *                 two will be passed when called for the upper value of a
     *                 BETWEEN operator.
     * @return Pointer to the item expression for the constant value, or \c NULL
     *         if there is not one, or if the column compared is not the right one.
     */
    ConstValue* getConstOperand(ItemExpr* predExpr, Lng32 constInx = 1);

    ConstValue* reconstituteInt64Value(NAType* type, Int64 val) const;
    ConstValue* reconstituteDoubleValue(NAType* type, Float64 val) const;

  private:
    // Copy assignment not defined.
    OptRangeSpec& operator=(const OptRangeSpec&);

    QRDescGenerator* descGenerator_;

    /** The \c expression the range applies to. \c NULL if the range is on a
        simple column. */
    ItemExpr* rangeExpr_;

    ValueId vid_;
    NABoolean isIntersection_;

};  // class OptRangeSpec

/**
 * Extends OptRangeSpec to provide capabilities needed for the Normalizer's use
 * of range specifications.
 */
class OptNormRangeSpec : public OptRangeSpec
{
  public:
    OptNormRangeSpec(QRDescGenerator* descGenerator, CollHeap* heap = NULL)
      : OptRangeSpec(descGenerator, heap, LL_ERROR),
        originalItemExpr_(NULL)
      {}

    virtual ~OptNormRangeSpec()
      {}

    virtual NABoolean forNormalizer() const
      {
        return TRUE;
      }

    /**
     * Returns the item expression from which the range was generated. This
     * allows the original expression to be used in lieu of the range, if
     * desired.
     *
     * @return Pointer to the original \c ItemExpr the range spec was derived
     *         from.
     */
    virtual const ItemExpr* getOriginalItemExpr() const
      {
        return originalItemExpr_;
      }

    virtual void setOriginalItemExpr(ItemExpr* itemExpr)
      {
        originalItemExpr_ = itemExpr;
      }

    /**
     * Calls the OptRangeSpec version of this function to get the ItemExpression
     * which is the simplified version of the original expression. It then uses
     * that expression to replace the expression associated with the value id
     * of the original item expression for the column or expression the range
     * is on.
     *
     * @param normWA Pointer to Normalizer work area. Used to normalize the
     *               returned expression.
     * @return Pointer to an \c ItemExpression representing the simplified
     *         range condition.
     */
    virtual ItemExpr* getRangeItemExpr(NormWA* normWA = NULL) const;

    /**
     * Builds the item expression tree that represents the original form of the
     * range, and then calls the parent version of this function to do the
     * actual range intersection.
     *
     * @param other The RangeSpec to conjoin to this one.
     */
    virtual void intersectRange(OptNormRangeSpec* other);

    /**
     * Builds the item expression tree that represents the original form of the
     * range, and then calls the parent version of this function to do the
     * actual range union.
     *
     * @param other The RangeSpec to union with this one.
     */
    virtual void unionRange(OptNormRangeSpec* other);

  private:
    // Copy ctor/assignment not defined.
    OptNormRangeSpec(const OptNormRangeSpec&);
    OptNormRangeSpec& operator=(const OptNormRangeSpec&);

    /** The \c ItemExpr used to build this range spec. */
    ItemExpr* originalItemExpr_;
};

/**
 * Instances of this class are used as the hash table value in hash tables
 * associating ValueIds (of columns) or text (of expressions) with a range.
 * Lookup is necessary because an existing range may need to be modified when
 * an additional predicate on the column or expression is encountered.
 * %RangeInfo pairs the range specification and the range predicate list it
 * belongs to, so that when all range information has been acquired, we can
 * iterate over the hash table contents and insert each range specification
 * into its owning range predicate list in the query descriptor.
 */
class RangeInfo : public NABasicObject
{
  public:
    RangeInfo(OptRangeSpec* range, QRJBBPtr jbbElem)
        : rangeSpec_(range), jbbElem_(jbbElem)
      {
        owningList_ = jbbElem->getHub()->getRangePredList();
      }
      
    virtual ~RangeInfo()
      {
        delete rangeSpec_;
      }
      
    // Required by NAHashDictionaryIterator
    Int32 operator==(const RangeInfo& other);

    OptRangeSpec* getRangeSpec() const
      {
        return rangeSpec_;
      }

    const QRRangePredListPtr getOwningList() const
      {
        return owningList_;
      }

    const QRJBBPtr getJbbElem() const
      {
        return jbbElem_;
      }

  private:
    /** Range specification for a column or expression. */
    OptRangeSpec* rangeSpec_;

    /** 
     * JBB element owning the list of range preds. This is saved so we can
     * access the residual pred list if a range predicate becomes a logical
     * absurdity and must be represented as a FALSE condition.
     */
    QRJBBPtr jbbElem_;

    /** Range predicate list the range belongs to. */
    QRRangePredListPtr owningList_;
}; // class RangeInfo

#endif /* _OPTRANGE_H_ */
