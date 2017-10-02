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
#ifndef COSTSCALAR_H
#define COSTSCALAR_H
/* -*-C++-*-
 **************************************************************************
 *
 * File:         CostScalar.h
 * Description:  A numeric class, to be protected from under/overflow
 * Created:      04/15/97
 * Language:     C++
 *
 *
 *
 **************************************************************************
 */

#include "BaseTypes.h"
#include "CmpCommon.h"
#include <float.h>


// -----------------------------------------------------------------------
// since these are doubles, we need to define non-zero equality within an
// EPSILON value; note that this EPSILON value should be equal (roughly)
// to whatever is written in /../include/float.h
//NB: DBL_EPSILON is ~2.22e-16
// -----------------------------------------------------------------------
#define COSTSCALAR_EPSILON       (1e-10) /* had problems with DBL_EPSILON -- too precise! */
#define MINUS_COSTSCALAR_EPSILON (-1e-10)
#define COSTSCALAR_MICRO_EPSILON (1e-70) /* on NSK, DBL_MIN == e-77 */
#define COSTSCALAR_MAX           (1e70)  /* give ourselves a little room ... */
// -----------------------------------------------------------------------
// NB: the above values are a KLUDGE -- if we're too close to the boundary
// values, sometimes we get unexpected results (e.g., two numbers which
// look identical are judged not-equal; e.g., FPU overflow).
// -----------------------------------------------------------------------

// these two are used to control when absolute value of CostScalar arithmetics 
// result goes out of range [COSTSCALAR_MICRO_EPSILON,COSTSCALAR_MAX]
// when exponent of dpv_ attribute belongs to the range [-230,230]  we can 
// guarantee that dpv_ belongs to the range above. In fact, 1e70 is 
// approximately equal to 2^232.5 In case we don't want any CostScalar
// to be in the above range we have to put overflow check not only in *,/
// but also in +,- CostScalar operations, so we want to have some slack
#define CS_MAX_BIN_EXP (230)
#define CS_MIN_BIN_EXP (-230)


#define _ABSOLUTE_VALUE_(x) ((x) >= 0 ? x : ((x) * -1))

#define _IS_ZERO_(x)         (_ABSOLUTE_VALUE_(x) < COSTSCALAR_EPSILON)
#define _IS_EXACTLY_ZERO_(x) (_ABSOLUTE_VALUE_(x) < COSTSCALAR_MICRO_EPSILON)

// -----------------------------------------------------------------------
// these are the flags indicating values that we never want to allow :
// not-a-number ("quiet" or not) and infinity (positive or negative)
// -----------------------------------------------------------------------
#define BAD_VALUE_FLAGS   (_FPCLASS_SNAN | _FPCLASS_QNAN | _FPCLASS_NINF | _FPCLASS_PINF)
#define COSTSCALAR_IS_INFINITE_OR_NaN    (_fpclass(dpv_) & BAD_VALUE_FLAGS)



// -----------------------------------------------------------------------
// Minimum of one macro.  This macro assumes that the parameter passed in
// is of type CostScalar.  The idea is not to create temporary CostScalar
// object for the numeric literal 1 every time the MIN_ONE macro is called,
// hence improve performance at runtime (compiling a SQL query).
// -----------------------------------------------------------------------
#ifndef MIN_ONE_CS
//#define MIN_ONE_CS( X )	  MAXOF( X, csOne )
#define MIN_ONE_CS( X )  ( ((CostScalar)X).minCsOne() )
#endif

#ifndef MIN_ZERO_CS
//#define MIN_ZERO_CS( X )  ( ( (X) < csZero ) ? (X) = csZero : (X) )
#define MIN_ZERO_CS( X )  ( ((CostScalar)X).minCsZero() ) 
#endif

#ifndef MIN_ZERO
#define MIN_ZERO( X )  ( ( (X) < 0 ) ? (X) = 0 : (X) )
#endif

// -----------------------------------------------------------------------
//
// These two variables (stored in CostScalar.cpp) determine whether
// or not we give assertion failures in case of div-by-zero stupidities.
//
//   USER_WANTS_DIVZERO_FAILURES : if the user has set the DIVZERO_ENV_VAR,
//                                 then s/he wants to get these assertions
//
//   ASSERTIONS_ALWAYS_ON        : at certain times we always want these assertions to fail
//
// -----------------------------------------------------------------------

#define DIVZERO_ENV_VAR "SQL_WANT_DIVZERO_FAILURES"


extern THREAD_P NABoolean USER_WANTS_DIVZERO_FAILURES ;
extern THREAD_P NABoolean ASSERTIONS_ALWAYS_ON ;       


#ifndef NDEBUG 
// On NSK using Tandem floating point format, this macro expands to "if (0)",
// hence we define the macro to expand to no-op.
#define ASSERT_IS_A_VALID_COSTSCALAR
#else
#define ASSERT_IS_A_VALID_COSTSCALAR
#endif // NDEBUG

#include <math.h>

// -----------------------------------------------------------------------
// The following classes are defined in this file.
// -----------------------------------------------------------------------

class CostScalar;
//<pb>

// -----------------------------------------------------------------------
// Externally defined constants.
// -----------------------------------------------------------------------

// Commonly used CostScalar objects.
extern const CostScalar csZero;
extern const CostScalar csOne;
extern const CostScalar csTwo;
extern const CostScalar csMinusOne;
extern const CostScalar csOneKiloBytes;

// -----------------------------------------------------------------------
// A simple shell around a 'double' that is introduced for preventing
// overflow conditions.
// -----------------------------------------------------------------------
class CostScalar
{

public:
  // -----------------------------------------------------------------------
  // constructors (no need for a destructor, it's just a double-wrapper)
  // -----------------------------------------------------------------------
  
  CostScalar() : dpv_(0.0)                                              {}

  CostScalar(const double dpv) : dpv_(dpv)                              
  { ASSERT_IS_A_VALID_COSTSCALAR ; }

  // Copy constructor.
  CostScalar(const CostScalar &other) : dpv_(other.dpv_)
  { ASSERT_IS_A_VALID_COSTSCALAR ; }

  // ----------------------------------------------------------------------
  // overloaded operators
  // ----------------------------------------------------------------------

  // assignment
  inline CostScalar & operator = (const CostScalar &other)
     { dpv_ = other.dpv_ ; ASSERT_IS_A_VALID_COSTSCALAR ; return *this ; }

  // ----------------------------------------------------------------------
  // CostScalar arithmetic
  // ----------------------------------------------------------------------

  // op + : simple
  inline CostScalar operator + (const CostScalar &other) const
  { return dpv_ + other.dpv_; }

  // op - : lots of code seems to depend on case where if x==x, x-x==0
  inline CostScalar operator - (const CostScalar &other) const
  { 
    if ( *this == other ) 
      return 0 ; 
    else 
      return dpv_ - other.dpv_; 
  }

  // op *, implementation in CostScalar.cpp
  CostScalar operator * (const CostScalar &other) const;
  // op /, implementation in CostScalar.cpp
  CostScalar operator / (const CostScalar &other) const;

  // op += : simple
  inline CostScalar & operator += (const CostScalar &other) 
  { 
    dpv_ = dpv_ + other.dpv_ ; 
    return *this ; 
  }
  // op -= : fairly simple ; use op - above
  inline CostScalar & operator -= (const CostScalar &other)
  { 
    *this = *this - other ; 
    return *this ; 
  }
  // op *= : delegate to op *
  inline CostScalar & operator *= (const CostScalar &other)
  {
    *this = *this * other;
    return *this;
  }
  // op /= : delegate to op /
  inline CostScalar & operator /= (const CostScalar &other)
  { 
    *this = *this / other ; 
    return *this ; 
  }

  // ----------------------------------------------------------------------
  // comparison of CostScalar objects 
  // ----------------------------------------------------------------------

  // op == : needs to be somewhat complicated ... sigh ...
  inline NABoolean operator == (const CostScalar &other) const
  {
    return ( ( _ABSOLUTE_VALUE_(dpv_ - other.dpv_) / 
                (_ABSOLUTE_VALUE_(dpv_)+_ABSOLUTE_VALUE_(other.dpv_)+1) )
             < COSTSCALAR_EPSILON ) ;
  }

  // $$$ We have to make sure we don't allow two CostScalar objects to be
  // $$$ both == and < each other!

  // op < : see note above
  inline NABoolean operator <  (const CostScalar &other) const
             { return ( NOT (*this == other) AND (dpv_ < other.dpv_) ) ; }
  //                    ^^^^^^^^^^^^^^^^^^^^
  //                      this is essential

  // op > : same as op < 
  inline NABoolean operator >  (const CostScalar &other) const
             { return ( NOT (*this == other) AND (dpv_ > other.dpv_) ) ; }
  //                    ^^^^^^^^^^^^^^^^^^^^
  //                      this is essential

  // op != : simple
  inline NABoolean operator != (const CostScalar &other) const
                                         { return NOT (*this == other) ; }
  // op <= : don't use op< (avoid 2nd call to op==)
  inline NABoolean operator <= (const CostScalar &other) const
                      { return (dpv_ < other.dpv_) OR (*this == other) ; }
  // op >= : don't use op> (avoid 2nd call to op==)
  inline NABoolean operator >= (const CostScalar &other) const
                      { return (dpv_ > other.dpv_) OR (*this == other) ; }

  // ----------------------------------------------------------------------
  // overloaded ops (++,--) that we probably don't need
  // ----------------------------------------------------------------------

  // prefix
  inline CostScalar & operator ++ ()             { dpv_++; return *this; }
  inline CostScalar & operator -- ()             { dpv_--; return *this; }

  // postfix
  inline CostScalar operator ++ (Int32)
                         { CostScalar temp (*this); ++dpv_; return temp; }
  inline CostScalar operator -- (Int32)
                         { CostScalar temp (*this); --dpv_; return temp; }

  // ----------------------------------------------------------------------
  // other useful methods
  // ----------------------------------------------------------------------

  // isZero : is THIS *essentially* zero? (+/- COSTSCALAR_EPSILON)
  //inline NABoolean isZero () const          { return (_IS_ZERO_(dpv_)) ; }
  inline NABoolean isZero () const          
  { 
      return ( (dpv_ >= MINUS_COSTSCALAR_EPSILON) AND
               (dpv_ <= COSTSCALAR_EPSILON) ); 
  }

  // isExactlyZero : is THIS *exactly* zero? (+/- COSTSCALAR_MICRO_EPSILON) 
  // --> we hate to require that every bit be exactly the same, so this
  // function requires the CostScalar object be very very very very small
  // in order to return TRUE
  inline NABoolean isExactlyZero () const { return (_IS_EXACTLY_ZERO_(dpv_)) ; }

  // round : round to zero if the value is *essentially* equal to zero
  inline void roundIfZero ()                        { if (isZero()) dpv_ = 0 ; }

  // roundExactly : round to zero if the value is *exactly* equal to zero
  inline void roundIfExactlyZero ()          { if (isExactlyZero()) dpv_ = 0 ; }

  // round the value to closes integer.
  inline CostScalar & round ()
  {
    CostScalar highValue = getCeiling();
    if ((dpv_ + 0.5) < highValue.getValue())   // very roundabout way of rounding
      *this = getFloor();
    else
      *this = highValue;
    return *this;
  }

  // getCeiling : ceiling of 12.34 = 13.0, assumes non-negative CostScalar
  CostScalar getCeiling() const
  {
    // use ANSI c-runtime function defined in <math.h>:
    return CostScalar(ceil(getValue()));
  }

  // getFloor : floor of 12.34 = 12.0, assumes non-negative CostScalar
  CostScalar getFloor() const
  {
    // use ANSI c-runtime function defined in <math.h>:
    return CostScalar(floor(getValue()));
  }

  // getValue, value : getting at the underlying double value explicitly
  inline double getValue() const                   { return dpv_ ; }
  inline double value() const                      { return dpv_ ; }
  inline const double toDouble() const             { return dpv_ ; }

  inline long toLong() const
  { return (dpv_ > LONG_MAX ? LONG_MAX : (dpv_ < LONG_MIN ? LONG_MIN : (long) dpv_)); }

  // Many functions above are expensive because of using overloaded op==
  // The purpose of functions below is to make comparison with csOne,csZero
  // and computing min/max function between CoastScalar and csOne/csZero 
  // more efficient. These operations make a big chunk of CostScalar
  // operations. This needs further improvement and cleanup.

  // isGreaterThanZero uses COSTSCALAR_EPSILON to decide if *this > csZero
  inline NABoolean isGreaterThanZero() const
  { 
      return (dpv_ > COSTSCALAR_EPSILON); 
  }
  inline NABoolean isGreaterOrEqualThanZero() const
  { 
      return (dpv_ >= MINUS_COSTSCALAR_EPSILON); 
  }
  inline NABoolean isLessThanZero() const
  { 
      return (dpv_ < MINUS_COSTSCALAR_EPSILON); 
  }
  
  // isGreaterThanOne uses hardcoded constant that could be replaced later
  inline NABoolean isGreaterThanOne() const 
  { 
      return (dpv_ > 1.000001); 
  }
  // isLessThanOne uses hardcoded constant that could be replaced later
  inline NABoolean isLessThanOne() const
  { 
      return (dpv_ < .999999); 
  }

  // These are analogs to MIN_ONE_CS, MIN_ZERO_CS macros but it will 
  // sideeffect the object it is applied to. The main application is
  // - for temporary variables. I think that these macros can be replaced
  // (with carefull consideration) with much more efficient ones like
  // #define MIN_ONE_CS(X) (X).minOneCs(). In this case we won't have to 
  // change lots of code in costmethod.cpp and other files. It is a
  // subject of a separate cleanup/performance project.
  // In general, the intention is to replace, wherever possible, 
  // comparison between CostScalar objects with these less expensive calls.

  inline CostScalar& minCsOne() 
  {
       if ( dpv_ < 1.0 )
         dpv_ = 1.0;
       return *this;
  }
  inline CostScalar& maxCsOne() 
  {
       if ( dpv_ > 1.0 )
         dpv_ = 1.0;
       return *this;
  }
  inline CostScalar& minCsZero() 
  {
       if ( dpv_ < 0.0 )
         dpv_ = 0.0;
       return *this;
  }
   inline CostScalar& maxCsZero() 
  {
       if ( dpv_ > 0.0 )
         dpv_ = 0.0;
       return *this;
  }


  // These counters are not expensive if we done't expect lots of 
  // overflows/underflows. In case we do we want to measure them
  // and their effect not only on debug but also on release compiler
  static inline void initOvflwCount(Lng32 cnt)
  {
      ovflwCount_ = cnt;
  }
  static inline Lng32 ovflwCount()
  {
      return ovflwCount_;
  }
  static inline void initUdflwCount(Lng32 cnt)
  {
      udflwCount_ = cnt;
  }
  static inline Lng32 udflwCount()
  {
      return udflwCount_;
  }

private:

  double dpv_;

  static THREAD_P Int32 ovflwCount_;
  static THREAD_P Int32 udflwCount_;

}; // class CostScalar
//<pb>
  
// -----------------------------------------------------------------------
// A representation for elapsed time.
// -----------------------------------------------------------------------

typedef CostScalar ElapsedTime ;

#endif /* COSTSCALAR_H */






