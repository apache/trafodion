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

#ifndef _RANGE_H_
#define _RANGE_H_

#include "QRDescriptor.h"
#include "nawstring.h"
#include "NAType.h"
#include "NumericType.h"
#include "MiscType.h"
#include "QRLogger.h"

class ItemExpr;
typedef CollIndex QRValueId;
#define QR_NULL_VALUE_ID ((CollIndex)0)

/**
 * \file
 * Contains the as classes involved in specifying the values encompassed by a
 * range predicate.
 */

class RangeSpec;
class SubrangeBase;
template <class T>
class   Subrange;
class RangeWString;
class RangeString;
class IntervalType;

/**
  * Enumeration indicating the location of a start or end point of a subrange
  * relative to another subrange. Possible values are before the start of the
  * subrange, within the subrange, or after the subrange.
  */
enum RelativeLocation
  {
    rel_loc_before,
    rel_loc_within,
    rel_loc_after
  };

/**
 * The RangeStringComparison namespace contains functions used in the comparison
 * of both single- and multi-byte string constants used in ranges.
 */
namespace RangeStringComparison
{
  /**
   * Does a blank-padded comparison of two strings, returning a value <0 if the
   * first string is less, >0 if the first string is greater, and 0 if they are
   * equal.
   * 
   * @param STRTYPE Template parameter denoting the string type (RangeString or
   *                RangeWString).
   * @param CHARTYPE Template parameter denoting the character type (char or
   *                 NAWChar).
   * @param rngStr1 The first string in the comparison.
   * @param rngStr2 The second string in the comparison.
   * @param len1 Length of the first string.
   * @param len2 Length of the first string.
   * @param padChar Character to extend shorter string with for purposes of
   *                comparison.
   */
  template <class STRTYPE, class CHARTYPE>
     Int32 cmp(const STRTYPE& rngStr1, const STRTYPE& rngStr2,
                    size_t len1, size_t len2, const CHARTYPE padChar);

  Int32 cmp(const RangeWString& rngStr1, const RangeWString& rngStr2,
                    size_t len1, size_t len2, const wchar_t padChar);

  Int32 cmp(const RangeString& rngStr1, const RangeString& rngStr2,
                    size_t len1, size_t len2, const char padChar);

  // The following two overloads of rngStrncmp allow a single function name to
  // be used in the template function above, even though different functions
  // have to be called for single- and multi-byte strings.

  /**
   * Compares two single-byte character strings.
   * @param s1 First string.
   * @param s2 Other string.
   * @param len Max number of characters to compare.
   * @return <0 if s1<s2, >0 if s1>s2, 0 if s1=s2.
   */
  Int32 rngStrncmp(const char* s1, const char* s2, size_t len);

  /**
   * Compares two double-byte character strings.
   * @param s1 First string.
   * @param s2 Other string.
   * @param len Max number of characters to compare.
   * @return <0 if s1<s2, >0 if s1>s2, 0 if s1=s2.
   */
  Int32 rngStrncmp(const NAWchar* s1, const NAWchar* s2, size_t len);
};

/**
 * Subclass of \c NAString that redefines comparison operators to use
 * blank-padding of the shorter string. Blank-padded comparisons are needed to
 * emulate the string comparison semantics of SQL.
 */
class RangeString : public NAString
{
  public:
    RangeString(NAMemory* heap = NULL)
      : NAString(heap)
      {}

    static Int32 cmp(const RangeString& rngStr1, const RangeString& rngStr2)
      {
        return RangeStringComparison::cmp(rngStr1, rngStr2, 
                                          rngStr1.length(), rngStr2.length(),
                                          ' ');
      }

    RangeString& operator=(const RangeString& other)
      {
        *((NAString*)this) = other;
        return *this;
      }

    RangeString& operator=(const char* other)
      {
        *((NAString*)this) = other;
        return *this;
      }
};

// Have to define the comparison operators for RangeString as overloads of the
// nonmember operators, or the NAString versions (which are also defined this
// way) get invoked instead.

inline NABoolean operator==(const RangeString& s1, const RangeString& s2)
{ 
  return RangeString::cmp(s1, s2) == 0;
}

inline NABoolean operator!=(const RangeString& s1, const RangeString& s2)
{ 
  return RangeString::cmp(s1, s2) != 0;
}

inline NABoolean operator< (const RangeString& s1, const RangeString& s2)
{
  return RangeString::cmp(s1, s2) < 0;
}

inline NABoolean operator> (const RangeString& s1, const RangeString& s2)
{
  return RangeString::cmp(s1, s2) > 0;
}

inline NABoolean operator<=(const RangeString& s1, const RangeString& s2)
{ 
  return RangeString::cmp(s1, s2) <= 0;
}

inline NABoolean operator>=(const RangeString& s1, const RangeString& s2)
{
  return RangeString::cmp(s1, s2) >= 0;
}

/**
 * Subclass of \c NAWString that defines copy assignment and redefines comparison
 * operators to use blank-padding of the shorter string. \c NAWString does not
 * define a proper copy assignment operator, and assigning one string to another
 * can lead to serious pointer problems. Blank-padded comparisons are needed to
 * emulate the string comparison semantics of SQL.
 */
class RangeWString : public NAWString
{
  public:
    RangeWString(NAMemory* heap = NULL)
      : NAWString(heap)
      {}

    RangeWString(const NAWchar* nawChrPtr, size_t len, NAMemory* heap = NULL)
      : NAWString(nawChrPtr, len, heap)
      {}

    RangeWString& operator=(const RangeWString& other);

    RangeWString& operator=(const NAWchar* other)
      {
        *((NAWString*)this) = other;
        return *this;
      }

    static Int32 cmp(const RangeWString& rngStr1,
                   const RangeWString& rngStr2)
      {
        return RangeStringComparison::cmp(rngStr1, rngStr2,
                                          rngStr1.length(), rngStr2.length(),
                                          L' ');
      }
};

// Have to define the comparison operators for RangeWString as overloads of the
// nonmember operators, or the NAWString versions (which are also defined this
// way) get invoked instead.

inline NABoolean operator==(const RangeWString& s1, const RangeWString& s2)
{ 
  return RangeWString::cmp(s1, s2) == 0;
}

inline NABoolean operator!=(const RangeWString& s1, const RangeWString& s2)
{ 
  return RangeWString::cmp(s1, s2) != 0;
}

inline NABoolean operator< (const RangeWString& s1, const RangeWString& s2)
{
  return RangeWString::cmp(s1, s2) < 0;
}

inline NABoolean operator> (const RangeWString& s1, const RangeWString& s2)
{
  return RangeWString::cmp(s1, s2) > 0;
}

inline NABoolean operator<=(const RangeWString& s1, const RangeWString& s2)
{ 
  return RangeWString::cmp(s1, s2) <= 0;
}

inline NABoolean operator>=(const RangeWString& s1, const RangeWString& s2)
{
  return RangeWString::cmp(s1, s2) >= 0;
}

// This exists only to allow an array of pointers to template instances.
/**
 * Abstract class representing a subrange (interval) of a range. The primary
 * purpose of the class is to allow an array of pointers to subranges for
 * various types, since the Subrange class is a template.
 * @see Subrange
 */
class SubrangeBase : public NABasicObject
{
  public:
    static const Int64 MICROSECONDS_IN_DAY;
    static const Int64 SUBRANGE_DATE_MIN;
    static const Int64 SUBRANGE_DATE_MAX;
    static const Int64 SUBRANGE_TIME_MIN;
    static const Int64 SUBRANGE_TIME_MAX;
    static const Int64 SUBRANGE_TIMESTAMP_MIN;
    static const Int64 SUBRANGE_TIMESTAMP_MAX;

    /**
     * Determines the minimum and maximum values of a given exact numeric
     * or datetime type. The latter are converted to an exact numeric
     * representation for use with ranges.
     *
     * @param numType The type (exact numeric or datetime).
     * @param [out] typeMin The minimum value for the type.
     * @param [out] typeMax The maximum value for the type.
     * @param level Logging level to use for any failure.
     */
    static void getExactNumericMinMax(const NAType& type,
                                      Int64& typeMin,
                                      Int64& typeMax,
                                      logLevel level);

    /**
     * Creates a QRNumericVal from the value and type information (specifically,
     * the scale) passed.
     *
     * @param heap The heap to allocate the element on.
     * @param value The unscaled value of the numeric constant.
     * @param type Type information for the column the value is for.
     * @return Pointer to the created scalar element.
     */
    static QRScalarValuePtr createScalarValElem(CollHeap* heap,
                                                Int64 value,
                                                const NAType& type,
                                                const NAString& unparsedText);

    /**
     * Creates a QRFloatVal from the value passed. #type is not used, but
     * must be part of the signature, because this function is called from a
     * template that will be instantiated for different types, including exact
     * numeric, which requires the type argument.
     *
     * @param heap The heap to allocate the element on.
     * @param value The value to construct a scalar element for.
     * @param type Not used for this overload.
     * @return Pointer to the created scalar element.
     */
    static QRScalarValuePtr createScalarValElem(CollHeap* heap,
                                                double value,
                                                const NAType& type,
                                                const NAString& unparsedText);

    /**
     * Creates a QRStringVal from the value passed. #type is not used, but
     * must be part of the signature, because this function is called from a
     * template that will be instantiated for different types, including exact
     * numeric, which requires the type argument.
     *
     * @param heap The heap to allocate the element on.
     * @param value The value to construct a scalar element for.
     * @param type Not used for this overload.
     * @return Pointer to the created scalar element.
     */
    static QRScalarValuePtr createScalarValElem(CollHeap* heap,
                                                const NAString& value,
                                                const NAType& type,
                                                const NAString& unparsedText);

    /**
     * Creates a QRWStringVal from the Unicode string passed. #rangeColVid is
     * not used, but must be part of the signature, because this function is
     * called from a template that will be instantiated for different types,
     * including exact numeric, which requires the argument.
     *
     * @param heap The heap to allocate the element on.
     * @param value The double-byte character string to construct a scalar
     *              element for.
     * @param type Not used for this overload.
     * @return Pointer to the created scalar element.
     */
    static QRScalarValuePtr createScalarValElem(CollHeap* heap,
                                                const RangeWString& value,
                                                const NAType& type,
                                                const NAString& unparsedText);

    /**
     * Returns the numeric difference between the rangespec representations of
     * two successive values for the given type. This applies only to types
     * represented for the purpose of rangespec analysis as Int64 values; all
     * exact numeric types, date, time, timestamp, and interval types. The
     * step size of all numeric types is one, but the situation is more complex
     * for the other types.
     *
     * @param type The data type to find the internal step size for.
     * @param level The logging level to use for any failure.
     * @return Integer difference between two successive values of the type.
     */
    static Int64 getStepSize(const NAType* type, logLevel level);

    /**
     * Determines equality of two subranges. To be equal, the subranges must
     * have the same start and end values, and both the start value pair and
     * end vaue pair must agree as to whether or not they are inclusive.
     *
     * @param other The subrange being compared to this one.
     * @return \c TRUE if the two subranges are equal, otherwise \c FALSE.
     */
    virtual NABoolean operator==(const SubrangeBase& other) const = 0;

    NABoolean operator!=(const SubrangeBase& other) const
      {
        return !(*this == other);
      }

    /**
     * Determines equality of two subranges. This method is equivalent to the
     * use of the \c == operator.
     *
     * @param other The subrange being compared to this one.
     * @return \c TRUE if the two subranges are equal, otherwise \c FALSE.
     */
    NABoolean isEqual(const SubrangeBase& other) const
      {
        return *this == other;
      }

    /**
     * Returns a deep copy of this &SubrangeBase. Use this instead of the copy
     * ctor, which is protected.
     *
     * @param heap Heap to use for the new object.
     * @return Deep copy of this %SubrangeBase.
     */
    virtual SubrangeBase* clone(CollHeap* heap = NULL) const = 0;

    /**
     * Returns \c TRUE iff this subrange begins with the minimum value of the
     * passed type. In addition, the start must be inclusive, and must not be
     * equal to the end (i.e, not a single-point subrange), and the type must
     * be numeric.
     *
     * @param type The numeric data type.
     * @return \c TRUE if the subrange inclusively starts with the type's
     *         minimum value.
     */
    NABoolean isMinForType(const NAType& type);

    /**
     * Returns \c TRUE iff this subrange ends with the maximum value of the
     * passed type. In addition, the end must be inclusive, and must not be
     * equal to the start (i.e, not a single-point subrange), and the type must
     * be numeric.
     *
     * @param type The numeric data type.
     * @return \c TRUE if the subrange inclusively ends with the type's
     *         maximum value.
     */
    NABoolean isMaxForType(const NAType& type);

    virtual void write(ostream& os) const = 0;

    /**
     * Makes a copy of this subrange and places it in the subrange list of
     * the given RangeSpec. This accesses the %RangeSpec as a friend function
     * rather than returning a Subrange* to the %RangeSpec function that is
     * copying the subranges, because in that context the compiler can't know
     * which template instantiation to use when invoking RangeSpec::placeRange().
     *
     * @param range %RangeSpec to add a copy of this subrange to.
     * @param heap Heap to allocate the copy on.
     */
    virtual void copyToRangeSpec(RangeSpec* range,
                                 CollHeap* heap = NULL) const = 0;

    /**
    * Indicates whether the start of this subrange comes before the start of
    * the passed one.
    *
    * @param other The subrange being compared to this one.
    * @return \c TRUE if this subrange starts before the passed one, \c FALSE
    *         otherwise.
    */
    virtual NABoolean startsBefore(SubrangeBase* other) const = 0;

    /**
    * Indicates whether the start of this subrange comes after the end of
    * the passed one.
    *
    * @param other The subrange being compared to this one.
    * @return \c TRUE if this subrange starts after the end of the passed one,
    *         \c FALSE otherwise.
    */
    virtual NABoolean startsAfter(SubrangeBase* other) const = 0;

    /**
    * Indicates whether the end of this subrange comes before the start of
    * the passed one.
    *
    * @param other The subrange being compared to this one.
    * @return \c TRUE if this subrange ends before the start of the passed one,
    *         \c FALSE otherwise.
    */
    virtual NABoolean endsBefore(SubrangeBase* other) const = 0;

    /**
    * Indicates whether the end of this subrange comes after the end of the
    * passed one.
    *
    * @param other The subrange being compared to this one.
    * @return \c TRUE if the this subrange ends after the end of the passed one,
    *         \c FALSE otherwise.
    */
    virtual NABoolean endsAfter(SubrangeBase* other) const = 0;

    /**
     * Determines whether or not the last value of this subrange is the
     * immediate predecessor of the first value of the passed subrange.
     * This is automatically \c FALSE for all but integral types, including
     * fixed-numerics with nonzero scale, which are represented in a subrange
     * as an unscaled Int64 with an associated scale factor. This method is
     * used to determine when two neighboring subranges can be combined into
     * a single one.
     *
     * @param other A subrange that represents higher values than this one
     *              and may be adjacent.
     * @return \c TRUE iff this subrange is adjacent to the passed subrange on
     *         the upper end of this subrange.
     */
    virtual NABoolean adjacentTo(SubrangeBase* other) const = 0;

    /**
     * Compares this subrange to the passed one in terms of the relative
     * position of its start and end points.
     *
     * @param other The subrange this one is compared to.
     * @param startLoc Returns the position of this subrange's start relative
     *                 to the other subrange (before, within, or after).
     * @param startLoc Returns the position of this subrange's end relative
     *                 to the other subrange (before, within, or after).
     */
    virtual void compareTo(SubrangeBase* other,
                           RelativeLocation& startLoc,
                           RelativeLocation& endLoc) const = 0;

    /**
    * Extends this subrange on the low side to the starting point of the other
    * one if it is less.
    *
    * @param other The subrange to adopt the starting point of, if it is less
    *              than that of this one.
    */
    virtual void extendStart(SubrangeBase* other) = 0;

    /**
    * Extends this subrange on the high side to the ending point of the other
    * one if it is more.
    *
    * @param other The subrange to adopt the ending point of, if it is more
    *              than that of this one.
    */
    virtual void extendEnd(SubrangeBase* other) = 0;
    
    /**
     * Decreases the span of a subrange by moving the start point up to that of
     * the other subrange. If the starting point of the other subrange is the
     * same or comes before that of this subrange, this subrange is unaffected.
     *
     * @param other Subrange used to restrict the starting point of this one.
     */
    virtual void restrictStart(SubrangeBase* other) = 0;

    /**
     * Splits this subrange in two at the value that is the end point of the
     * other subrange. The end point of this subrange is modified, and a new
     * subrange is created that includes the remainder of the original. The end
     * of the other subrange is required to be within this subrange; however,
     * if it coincides with the end point of this one (including inclusivity),
     * no split will occur because there are no values that would be in the
     * new subrange. \c NULL is returned in this case.
     *
     * @param other The subrange that is used to split this one (using the
     *              value of its end point.
     * @param heap If a new subrange is created, allocate storage for it on
     *             this heap.
     * @return Pointer to the new subrange created, or \c NULL 
     */
    virtual SubrangeBase* splitAtEndOf(const SubrangeBase* other,
                                       CollHeap* heap) = 0;

    /**
    * Indicates whether this subrange consists of a single value, i.e., is
    * derived from an equality predicate.
    *
    * @return \c TRUE if this subrange consists of a single value, \c FALSE
    *         otherwise.
    */
    virtual NABoolean isSingleValue() const = 0;

     /**
     * Sets the specified value count (number of values in a subrange derived
     * from an IN list or disjunction of equality predicates) to 1 if it is a
     * single-point subrange on an exact numeric type.
     */
    virtual void initSpecifiedValueCount() = 0;

    Lng32 getSpecifiedValueCount() const
      {
        return specifiedValueCount_;
      }

    void setSpecifiedValueCount(Lng32 newVal)
      {
        specifiedValueCount_ = newVal;
      }

    virtual void makeStartInclusive(const NAType* type, NABoolean& overflowed) = 0;
    virtual void makeEndInclusive(const NAType* type, NABoolean& overflowed) = 0;

    /**
    * Returns a QRScalarValue element corresponding to the starting value of this
    * subrange.
    *
    * @param heap The heap to allocate the new element on.
    * @param type Type information for the column or expression the range is on.
    * @return Pointer to the scalar value element representing the start of this
    *         subrange.
    */
    virtual QRScalarValuePtr getStartScalarValElem(CollHeap* heap,
                                                   const NAType& type) const = 0;

    /**
    * Returns a QRScalarValue element corresponding to the ending value of this
    * subrange.
    *
    * @param heap The heap to allocate the new element on.
    * @param type Type information for the column or expression the range is on.
    * @return Pointer to the scalar value element representing the end of this
    *         subrange.
    */
    virtual QRScalarValuePtr getEndScalarValElem(CollHeap* heap,
                                                 const NAType& type) const = 0;

    /**
     * Determines whether this subrange starts within another one. \c TRUE is
     * returned iff the first value is contained in the other subrange.
     *
     * @param other Subrange to look for the start value in.
     * @return \c TRUE if the start of this subrange is contained in the other
     *         one, \c FALSE otherwise.
     */
    virtual NABoolean startsWithin(SubrangeBase* other) const = 0;

    /**
     * Determines whether this subrange ends within another one. \c TRUE is
     * returned iff the last value is contained in the other subrange.
     *
     * @param other Subrange to look for the end value in.
     * @return \c TRUE if the end of this subrange is contained in the other
     *         one, \c FALSE otherwise.
     */
    virtual NABoolean endsWithin(SubrangeBase* other) const = 0;

    /**
     * Returns the logging to level for any exception that occurs in an operation
     * on this Subrange.
     *
     * @return Logging level to use when reporting an exception.
     */
    virtual logLevel getLogLevel() const = 0;

    /**
     * Returns \c TRUE if this subrange covers all possible values of its type.
     * A RangeSpec that represents a predicate that is necessarily true will
     * consist of a single subrange, which will cover all values, and allow NULL
     * as a value. The NULL part is checked in RangeSpec::coversFullRange().
     *
     * @return \c TRUE if all values are included in this subrange, \c FALSE
     *         otherwise.
     * @see RangeSpec::coversFullRange()
     */
    NABoolean coversFullRange() const
      {
        return startIsMin_ && endIsMax_;
      }

    NABoolean startInclusive() const
      {
        return startInclusive_;
      }

    NABoolean endInclusive() const
      {
        return endInclusive_;
      }

    NABoolean startIsMin() const
      {
        return startIsMin_;
      }

    NABoolean endIsMax() const
      {
        return endIsMax_;
      }

    void setStartInclusive(NABoolean inclusive)
      {
        startInclusive_ = inclusive;
      }

    void setEndInclusive(NABoolean inclusive)
      {
        endInclusive_ = inclusive;
      }

    void setStartIsMin(NABoolean isMin)
      {
        startIsMin_ = isMin;
      }

    void setEndIsMax(NABoolean isMax)
      {
        endIsMax_ = isMax;
      }

    Int64 getStartAdjustment() const
      {
        return startAdjustment_;
      }

    void setStartAdjustment(Int64 adjustment)
      {
        startAdjustment_ = adjustment;
      }

    Int64 getEndAdjustment() const
      {
        return endAdjustment_;
      }

    void setEndAdjustment(Int64 adjustment)
      {
        endAdjustment_ = adjustment;
      }

    Int64 getTotalDistinctValues(CollHeap* heap, const NAType* type);

    /**
     * Writes a representation of the range to standard output for debugging
     * purposes in console mode on NT. Typically, the RangeSpec display method
     * will be used instead, to display all subranges of the range.
     */
    void display() const;

  protected:
    SubrangeBase()
      : startInclusive_(FALSE),
        endInclusive_(FALSE),
        startIsMin_(FALSE),
        endIsMax_(FALSE),
        specifiedValueCount_(0),
        startAdjustment_(0),
        endAdjustment_(0)
      {}

    /**
     * Creates a copy of the passed %SubrangeBase. The default copy ctor is
     * adequate, but we have to define it here to make it protected. The
     * clone() virtual function (which invokes this ctor) should be used
     * instead, to ensure correct duplication of the object.
     *
     * @param other The %SubrangeBase to make a copy of.
     */
    SubrangeBase(const SubrangeBase& other)
      : startInclusive_(other.startInclusive_),
        endInclusive_(other.endInclusive_),
        startIsMin_(other.startIsMin_),
        endIsMax_(other.endIsMax_),
        specifiedValueCount_(other.specifiedValueCount_),
        startAdjustment_(other.startAdjustment_),
        endAdjustment_(other.endAdjustment_)
      {}

    /**
    * \c TRUE if the start value is itself included in this subrange, as in the
    * case of using >= rather than >.
    */
    NABoolean startInclusive_;

    /**
    * \c TRUE if the end value is itself included in this subrange, as in the
    * case of using <= rather than <.
    */
    NABoolean endInclusive_;

    /**
    * If \c TRUE, the subrange is unbounded on the low side. In this case,
    * Subrange#start and Subrange#startInclusive_ should be ignored. At most
    * one subrange in the collection of subranges comprising a range should
    * have this quality.
    */
    NABoolean startIsMin_;

    /**
    * If \c TRUE, the subrange is unbounded on the high side. In this case,
    * Subrange#end and Subrange#endInclusive_ should be ignored. At most one
    * subrange in the collection of subranges comprising a range should have
    * this quality.
    */
    NABoolean endIsMax_;

    /**
    * This value will be nonzero only for exact numeric subranges constructed
    * from individual values, and represents the number of such values. For
    * example a in (5,6,7) will result in a value of 3, and a=5 or a=5 will
    * result in a value of 2, while the value will be 0 for a between 5 and 7.
    * This only affects the way a subrange is converted to a new ItemExpr in
    * #makeSubrangeItemExpr().
    */
    Lng32 specifiedValueCount_;

    Int64 startAdjustment_;
    Int64 endAdjustment_;

  private:
    // Copy assignment not defined.
    SubrangeBase& operator=(const SubrangeBase&);

};  // class SubrangeBase

/**
 * Class representing a continuous interval of values of a given type.
 * A subrange is specified by comparison operators applied to a column or
 * expression and a constant:
 * - < or <= is a subrange that is unbounded on the low side
 * - > or >= is a subrange that is unbounded on the high side
 * - a combination of </<= and >/>= (including the \c between predicate), is an interior subrange
 * - = is a single-value subrange (start and end value the same)
 * \n\n
 * Most of the content of this class is inherited from SubrangeBase, which
 * exists primarily to allow parameters which are collections of subranges
 * without regard to the underlying data type. This templated subclass contains
 * the start and end values, and definitions of the virtual functions that
 * use them.
 */
template <class T>
class Subrange : public SubrangeBase
{
  public:
    Subrange(logLevel ll)
      : logLevel_(ll)
      {}

    virtual NABoolean operator==(const SubrangeBase& other) const;

    /**
     * Returns a deep copy of this &Subrange. Use this instead of the copy
     * ctor, which is protected.
     *
     * @param heap Heap to use for the new object.
     * @return Deep copy of this subrange.
     */
    virtual Subrange* clone(CollHeap* heap = NULL) const
      {
        return new(heap) Subrange(*this);
      }

    /**
     * Displays the end points of the subrange.
     * @param os Output stream to write the display to.
     */
    virtual void write(ostream& os) const;

    virtual void copyToRangeSpec(RangeSpec* range,
                                 CollHeap* heap = NULL) const ;
    virtual NABoolean startsBefore(SubrangeBase* other) const;
    virtual NABoolean startsAfter(SubrangeBase* other) const;
    virtual NABoolean startsWithin(SubrangeBase* other) const;
    virtual NABoolean endsBefore(SubrangeBase* other) const;
    virtual NABoolean endsAfter(SubrangeBase* other) const;
    virtual NABoolean endsWithin(SubrangeBase* other) const;
    virtual NABoolean adjacentTo(SubrangeBase* other) const;
    virtual void compareTo(SubrangeBase* other,
                           RelativeLocation& startLoc,
                           RelativeLocation& endLoc) const;
    virtual void extendStart(SubrangeBase* other);
    virtual void extendEnd(SubrangeBase* other);
    virtual void restrictStart(SubrangeBase* other);
    virtual SubrangeBase* splitAtEndOf(const SubrangeBase* other,
                                       CollHeap* heap);

    virtual NABoolean isSingleValue() const
      {
        return !startIsMin_ && !endIsMax_ && start == end;
      }

    virtual void initSpecifiedValueCount();

    virtual QRScalarValuePtr getStartScalarValElem(CollHeap* heap,
                                                   const NAType& type) const
      {
        return createScalarValElem(heap, start, type, unparsedStart_);
      }

    virtual QRScalarValuePtr getEndScalarValElem(CollHeap* heap,
                                                 const NAType& type) const
      {
        return createScalarValElem(heap, end, type, unparsedEnd_);
      }

    void makeStartInclusive(const NAType* type, NABoolean& overflowed);
    void makeEndInclusive(const NAType* type, NABoolean& overflowed);

    void setUnparsedStart(const NAString& text)
    {
      unparsedStart_ = text;
    }

    void setUnparsedEnd(const NAString& text)
    {
      unparsedEnd_ = text;
    }

    virtual logLevel getLogLevel() const
      {
        return logLevel_;
      }

    /** The starting value of this subrange. */
    T start;

    /** The ending value of this subrange. */
    T end;

    NAString unparsedStart_;
    NAString unparsedEnd_;
  protected:
    /**
     * Creates a copy of the passed %Subrange. The default copy ctor is
     * adequate, but we have to define it here to make it protected. The
     * clone() virtual function (which invokes this ctor) should be used
     * instead, to ensure correct duplication of the object.
     *
     * @param other The %Subrange to make a copy of.
     */
    Subrange(const Subrange<T>& other)
      : SubrangeBase(other),
        start(other.start),
        end(other.end),
        unparsedStart_(other.unparsedStart_),
        unparsedEnd_(other.unparsedEnd_),
        logLevel_(other.logLevel_)
      {}

  private:
    // Copy assignment not defined.
    Subrange& operator=(const Subrange&);

    /**
     * This is the logging level to use for an exception that occurs while
     * manipulating this subrange. When used for the Normalizer it will be
     * LL_ERROR, and for MVQR it will be LL_MVQR_FAIL.
     */
    logLevel logLevel_;

};  // class Subrange


/**
 * A specification of the range constraints on a column or expression. The most
 * important part of the representation of a range is an array of value
 * intervals, represented by the Subrange class. This array is ordered by
 * value so that it is easier to determine if a value is in the range, or if
 * it subsumes/is subsumed by another range.
 *
 * @see Subrange
 */
class RangeSpec : public NABasicObject
{
    friend ostream& operator<<(ostream&, RangeSpec&);
    friend void Subrange<Int64>::copyToRangeSpec(RangeSpec*, CollHeap*) const;
    friend void Subrange<double>::copyToRangeSpec(RangeSpec*, CollHeap*) const;
    friend void Subrange<RangeString>::copyToRangeSpec(RangeSpec*, CollHeap*) const;
    friend void Subrange<RangeWString>::copyToRangeSpec(RangeSpec*, CollHeap*) const;

  public:
    RangeSpec(CollHeap* heap = NULL, logLevel ll = LL_MVQR_FAIL)
      : rangeColValueId_(QR_NULL_VALUE_ID),
        rangeJoinPredId_(QR_NULL_VALUE_ID),
        rangeExprText_(heap),
        mvqrHeap_(heap),
        nullIncluded_(FALSE),
        type_(NULL),
        isDumpMvMode_(FALSE),
        logLevel_(ll)
      {}

    /**
     * Constructs a RangeSpec from a QRRangePred object.
     * @param rangePred The QRRangePred object.
     * @param heap Heap to use for any allocations.
     * @param ll Logging level to use for any failure.
     */
    RangeSpec(QRRangePredPtr rangePred, CollHeap* heap = NULL, logLevel ll = LL_MVQR_FAIL);

    virtual ~RangeSpec();

    /**
     * Returns a deep copy of this range spec. Use this instead of the copy
     * ctor, which is protected.
     *
     * @param heap Heap to use for the new object. If \c NULL, this range spec's
     *             heap will be used for the new one. Note that this precludes
     *             cloning a %RangeSpec from a private heap on the system heap.
     * @return Deep copy of this range spec.
     */
    virtual RangeSpec* clone(CollHeap* heap = NULL) const
      {
        if (!heap)
          heap = mvqrHeap_;
        return new(heap) RangeSpec(*this, heap);
      }

    /**
     * Two RangeSpec objects are equal if they have the same type, and identify
     * the same range of values of that type. Since the subranges that comprise
     * this range of values is stored in a canonical form, the number of
     * subranges must be the same, and the subranges must be pairwise-equal.
     *
     * @param other The RangeSpec object to compare this one to.
     * @return \c TRUE iff the two ranges are identical.
     */
    NABoolean operator==(const RangeSpec& other) const;

    NABoolean operator!=(const RangeSpec& other) const
      {
        return !(*this == other);
      }

    /**
     * Determines equality of two ranges. This method is equivalent to the
     * use of the \c == operator.
     *
     * @param other The range being compared to this one.
     * @return \c TRUE if the two ranges are equal, otherwise \c FALSE.
     */
    NABoolean isEqual(const RangeSpec& other) const
      {
        return *this == other;
      }

    /**
     * Returns the ValueId of the column the range applies to. If the range is
     * on an expression instead of a single column, this will be \c NULL_VALUE_ID,
     * and if this object is an instance of OptRangeSpec,
     * #OptRangeSpec::getRangeExpr() can be called to get the \c ItemExpr* for
     * the expression.
     *
     * @return ValueId of the column the range is on.
     */
    QRValueId getRangeColValueId() const
      {
        return rangeColValueId_;
      }

    /**
     * Returns the id of the JoinPred this range references. All range conditions
     * on a member of an equality set are used to build a range that applies to
     * the JoinPred derived from that equality set. If this is the case, this
     * function will return the id of that JoinPred.
     *
     * @return Id of the JoinPred this range applies to, if any.
     */
    QRValueId getRangeJoinPredId() const
      {
        return rangeJoinPredId_;
      }

    /**
     * Returns the expression text for the expression the range applies to.
     * @return The expression text.
     */
    const NAString& getRangeExprText() const
      {
        return rangeExprText_;
      }

    /**
     * Returns type information for the column or expression or join pred the
     * range is on.
     * @return NAType for the range column/expression/join pred.
     */
    virtual const NAType* getType() const
      {
        // Type may be null for RangeSpecs used by qms; qms doesn't know or
        // care about type.
        return type_;
      }

    /**
     * Sets the type of this %RangeSpec. For the OptRangeSpec subclass, the
     * type is derived from the range item, so this method is overridden.
     * Since a %RangeSpec is instantiated from a descriptor, the type
     * designation must be read from the descriptor and used to find the
     * corresponding \c NAType.
     *
     * @param type The type of the item (col or expr) this range is for.
     * @see #type_ for an explanation of why this mutator is declared as \c const.
     */
    virtual void setType(const NAType* type)
      {
        type_ = type->newCopy(mvqrHeap_);
      }

    /**
     * Determines whether the type of the column/expression/join pred this
     * range applies to is compatible with that of the passed %RangeSpec.
     * Currently, the same types are required. This will likely be relaxed in
     * the future to use union-compatibility. An exception is that if either
     * type is not present (i.e., NULL), the function returns \c TRUE. QMS does
     * not know or care about the types and sets it to NULL.
     *
     * @param other %RangeSpec to compare type to.
     * @return \c TRUE iff the type this range is for is compatible with that
     *         of the other range.
     */
    NABoolean typeCompatible(const RangeSpec* other) const
      {
        const NAType *t1 = getType(), *t2 = other->getType();
        return !t1 || !t2 || *t1 == *t2;
      }

    /**
     * Determines whether the range of values specified by this %RangeSpec
     * subsumes that of the passed %RangeSpec. X subsumes Y if every value that
     * is part of Y is also part of X.
     *
     * @param other The possibly subsumed %RangeSpec.
     * @return \c TRUE if this %RangeSpec subsumes the other one, else \C FALSE.
     */
    NABoolean subsumes(const RangeSpec* other) const;

    /**
     * Records the ValueId of the column this range is on. Used only if the
     * range is on a simple column instead of a general expression.
     *
     * @param colVid ValueId for the column the range is on.
     */
    void setRangeColValueId(QRValueId colVid)
      {
        rangeColValueId_ = colVid;
      }

    /**
     * Sets the id of the \c &lt;JoinPred&gt; element this range is on. Used
     * only if the range is applied to the equality set represented by a join
     * predicate instead of a single column or expression.
     *
     * @param joinPredId Value id serving as the id of the \c &lt;JoinPred&gt;
     *                   this range applies to. The id is taken from the id of
     *                   an arbitrary (currently, the first in the sorted list)
     *                   member of the equality set.
     */
    void setRangeJoinPredId(QRValueId joinPredId)
      {
        rangeJoinPredId_ = joinPredId;
      }

    /**
     * Operates similarly to #addSubrange(ConstValue*,ConstValue*,NABoolean,NABoolean),
     * but gets the start and end values of the subrange from two pointers to
     * QRScalarValue objects.
     *
     * @param start Pointer to \c QRScalarValue object containing the starting
     *              value for the new subrange.
     * @param end Pointer to \c QRScalarValue object containing the ending
     *            value for the new subrange.
     * @param startInclusive \c TRUE if the start value is included in the subrange.
     * @param endInclusive \c TRUE if the end value is included in the subrange.
     */
    void addSubrange(QRScalarValuePtr startVal,
                     QRScalarValuePtr endVal,
                     NABoolean startInclusive,
                     NABoolean endInclusive);

    /**
     * Sets the indicator of whether or not NULL is included in this range.
     * @param nullIncluded TRUE iff NULL is part of the range.
     */
    void setNullIncluded(NABoolean nullIncluded)
      {
        nullIncluded_ = nullIncluded;
      }

    /**
     * Checks the indicator of whether or not NULL is included in this range.
     * @return TRUE iff NULL is part of the range.
     */
    NABoolean isNullIncluded() const
      {
        return nullIncluded_;
      }

    /**
     * Returns \c NULL for an object with most-specific-type of RangeSpec or
     * OptRangeSpec. Overridden in OptNormRangeSpec to return the item
     * expression from which the range was derived.
     *
     * @return NULL.
     */
    virtual const ItemExpr* getOriginalItemExpr() const
      {
        return NULL;
      }

    /**
     * Returns \c NULL for an object with most-specific-type of RangeSpec (or
     * OptRangeSpec). Overridden in OptRangeSpec to return the item expression the range is
     * based on in a form derived directly from this range object.
     *
     * @return NULL.
     */
    virtual ItemExpr* getRangeItemExpr() const
      {
        return NULL;
      }

    /**
     * Modifies this RangeSpec by intersection with another one to form the
     * conjunction of the two range specifications.
     *
     * @param other The RangeSpec to conjoin to this one.
     */
    virtual void intersectRange(RangeSpec* other);

    /**
     * Modifies this RangeSpec by union with another one to form the disjunction
     * of the two range specifications.
     *
     * @param other The RangeSpec to union with this one.
     */
    virtual void unionRange(RangeSpec* other);

    /**
     * Returns \c TRUE if the range covers all possible values of the range
     * item's type.
     *
     * @return \c TRUE if all values are included in the range, \c FALSE otherwise.
     */
    NABoolean coversFullRange() const
      {
        // If all values are represented, there will be a single contiguous
        // subrange. The \c NULL is accounted for here in the RangeSpec object
        // instead of the subrange, and must be considered as well.
        return (subranges_.entries() == 1
                  ?  nullIncluded_ && subranges_[0]->coversFullRange()
                  : FALSE);
      }

    /**
     * Returns \c TRUE if the rangespec represents a logical absurdity and is
     * therefore necessary false. For example a<1 AND a>1.
     * @return 
     */
    NABoolean isFalse() const
      {
        return !nullIncluded_ && subranges_.entries() == 0;
      }

    /**
     * Writes a representation of the values included in the range to the log file.
     */
    void log();

    /**
     * Writes the range representation to standard output, for debugging in
     * console mode on NT.
     */
    void display();

    // Return the total # of distinct values for all subranges.
    Int64 getTotalDistinctValues(CollHeap* heap);

    Int32 getTotalRanges() { return subranges_.entries(); };

  protected:
    /**
     * Creates a copy of the passed object. This copy ctor is protected so it
     * can only be used by the virtual object copier function clone(). Using a
     * virtual function to copy objects in a hierarchy prevents "slicing", or
     * failing to produce a copy of the most specific type when the copy is made
     * from a reference to a superclass.
     *
     * @param other The object to make the copy of.
     */
    RangeSpec(const RangeSpec& other, CollHeap* heap = NULL);

    template <class T>
    NABoolean adjacent(Subrange<T>* sub1, Subrange<T>* sub2)
    {
      QRTRACER("adjacent(Subrange<T>*,Subrange<T>*) -- returns FALSE");
      return FALSE;
    }

    // This specialization of the above template checks for consecutive values
    // in successive subranges for a fixed numeric type.
    NABoolean adjacent(Subrange<Int64>* sub1, Subrange<Int64>* sub2)
    {
      QRTRACER("adjacent(Subrange<Int64>*,Subrange<Int64>*)");
      return (sub1->end + 1 == sub2->start);
    }

    /**
     * Inserts a subrange into the array of subranges comprising this range
     * specification. Since the subranges must be in order of the values
     * they represent, the new subrange is inserted at the proper position.
     * If the subrange is on a fixed numeric type and is contiguous with the
     * subrange before or after it, they are combined to form a single subrange.
     * 
     * @param sub The subrange to add to the array.
     */
    template <class T>
    void placeSubrange(Subrange<T>* sub);

    /**
     * Denormalizes (i.e., converts from an open interval to one bounded by the
     * min or max value of the relevant data type) an inequality operator,
     * and adds the resulting subrange to this %RangeSpec.
     *
     * @param typeText Textual description of the data type of the col/expr the
     *                 range is on. This is the value of the \c sqlType attribute
     *                 of the \c Range element.
     * @param rangeOpType The specific inequality operator (<, <=, >, or >=).
     * @param value The value at the fixed end of the interval. The value that
     *              will be used at the other end is determined by the data type.
     */
    void addDenormalizedSubrange(const char* typeText,
                                 ElementType rangeOpType,
                                 QRScalarValuePtr value);

    /**
     * Splits the subrange at index \c subrangeInx in the RangeSpec's list of
     * subranges, using the end point of the passed subrange. This is used by
     * #intersectRange() to separate a subrange into a part that intersects the
     * other subrange and a part that doesn't. The original subrange is retained
     * with an altered end point, and the new subrange is added to the list
     * immediately following it.
     *
     * @param subrangeInx Index within this RangeSpec's subrange list of the
     *                    subrange to split.
     * @param otherSubrange Use the end point of this subrange to determine
     *                      where to split the subrange indicated by \c subrangeInx.
     *                      This end value must lie somewhere within the
     *                      subrange being split.
     */
    void splitSubrangeEnd(CollIndex subrangeInx, SubrangeBase* otherSubrange);

    /**
     * Called by addDenormalizedSubrange(), to parse interval type designations.
     * These are significantly more complicated than the other types, so a separate
     * function is used. On function entry, the leading "INTERVAL" keyword has 
     * already been scanned, and the text parameter points to the space following
     * it. The type specification is generated, so we can make some simplifying 
     * assumptions about its structure, e.g., all upper case, left paren follows
     * immediately after leading field, etc.
     *
     * @param text Interval type designation, without the initial INTERVAL keyword.
     * @return Pointer to an \c IntervalType designated by the text.
     */
    IntervalType* parseIntervalTypeText(const char* text) const;

    /**
     * Get the precision and scale from the passed string, which is either of the
     * form (prec,scale) or (prec). This is also used for interval types, in which
     * case the pair of values represent the leading field precision and fractional
     * seconds precision. The default scale (0) is not the same as the default
     * fractional seconds precision (6), so the scale parameter is not set if it
     * isn't specified in the string -- it must be set to the proper default by
     * the caller.
     *
     * @param openParen Ptr to the start of the prec/scale designation, which
     *                  must start with '('.
     * @param [out] prec  The precision specified.
     * @param  [out]scale The scale specified.
     */
    void getPrecScale(const char* openParen, Lng32& prec, Lng32& scale) const;

    /**
     * Returns the enum value corresponding to the given inverval field name.
     * @param name Name of the interval field (YEAR, HOUR, etc.).
     * @param len  Length of the interval field name.
     * @return The \c rec_datetime_field for the passed interval field name.
     */
    rec_datetime_field getIntervalFieldFromName(const char* name, Int32 len) const;

    void setDumpMvMode()
    {
      isDumpMvMode_ = TRUE;
    }

    NABoolean isDumpMvMode()
    {
      return isDumpMvMode_;
    }

    QRValueId rangeColValueId_;
    QRValueId rangeJoinPredId_;  // 0 unless range spec is for equality set
    NAString rangeExprText_; // should be set only by setRangeExpr()
    NAList<SubrangeBase*> subranges_;
    CollHeap* mvqrHeap_;
    NABoolean nullIncluded_;
    NABoolean isDumpMvMode_;

    /**
     * The logging level to use when an exception occurs involving a RangeSpec
     * object. LL_MVQR_FAIL is used in general, but the subclass used by the
     * Normalizer passes LL_ERROR to the superclass ctor, because a failure is
     * fatal to the query in that case, whereas for MVQR it only means the query
     * will not be rewritten.
     */
    logLevel logLevel_;

  private:
    // Copy assignment not defined.
    RangeSpec& operator=(const RangeSpec&);

    NAType* type_;
};  // class RangeSpec

inline ostream& operator<<(ostream& os, RangeSpec& rangeSpec)
{
  os << "RangeSpec contains " << rangeSpec.subranges_.entries() << " subranges:\n";
  for (CollIndex i=0; i < rangeSpec.subranges_.entries(); i++)
    {
      rangeSpec.subranges_[i]->write(os);
    }
  if (rangeSpec.nullIncluded_)
    os << "NULL is included\n";
  return os;
}

#endif /* _RANGE_H_ */
