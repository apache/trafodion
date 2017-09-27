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
* File:         Cost.C
* Description:  Physical Properties and Cost
* Created:      11/12/96
* Language:     C++
*
* Purpose:	 Simple Cost Vector Reduction
*
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------

#include "Sqlcomp.h"
#include "GroupAttr.h"
#include "Cost.h"
#include "DefaultConstants.h"
#include "SQLCLIdev.h"
#include "CmpContext.h"
#include "CostScalar.h"
#include "CmpContext.h"
#include "SQLCLIdev.h"
#include "opt.h"
#include "CompException.h"
#include <math.h>


//<pb>

extern Cost* rollUpUnaryNonBlocking(const Cost& ,
                                    const Cost& ,
                                    const ReqdPhysicalProperty* const);

// -----------------------------------------------------------------------
// global display methods (to be invoked from Objectcenter)
// -----------------------------------------------------------------------
// excluded for coverage because it's a debug code
void displayCost (const Cost & cost)
{
  cost.print();
}

//<pb>

// -----------------------------------------------------------------------
// friend functions involving objects of class SimpleCostVector
// -----------------------------------------------------------------------
//<pb>
//==============================================================================
//  Operator for traditional vector addition.  Each component of the returned
// vector is the sum of the corresponding components of two specified vectors.
//
// Note: this operation makes the number of probes in the result
//       vector equal to that of the first specified vector.
//
// Input:
//  v1  -- First specified vector.
//
//  v2  -- Second specified vector.
//
// Output:
//  none
//
// Return:
//  Sum of v1 and v2 using traditional vector addition.
//
//==============================================================================
SimpleCostVector
operator+(const SimpleCostVector &v1, const SimpleCostVector &v2)
{

  //---------------------------------
  //  Initialize result to vector v1.
  //---------------------------------
  SimpleCostVector resultVector = v1;

  //---------------------------------------------------------------
  //  Add each component of v2 to result's corresponding component.
  //---------------------------------------------------------------
  for (Lng32 vecIdx = 0; vecIdx < COUNT_OF_SIMPLE_COST_COUNTERS; vecIdx++)
    {
      resultVector.counter_[vecIdx] += v2.counter_[vecIdx];
    }

  //------------------------------------------------
  //  Set result's number of probes to that of v1's.
  //------------------------------------------------
  resultVector.setNumProbes(v1.getNumProbes());

  return resultVector;

} // operator+
//<pb>
//==============================================================================
//  Operator for traditional vector subtraction.  Each component of the returned
// vector is the difference between the corresponding components of two
// specified vectors.
//
// Note: this operation makes the number of probes in the result
//       vector equal to that of the first specified vector.
//
// Input:
//  v1  -- First specified vector.
//
//  v2  -- Second specified vector.
//
// Output:
//  none
//
// Return:
//  v1 minus v2 using traditional vector addition.
//
//==============================================================================
SimpleCostVector
operator-(const SimpleCostVector &v1, const SimpleCostVector &v2)
{

  //---------------------------------
  //  Initialize result to vector v1.
  //---------------------------------
  SimpleCostVector resultVector = v1;

  //----------------------------------------------------------------------
  //  Subtract each component of v2 from v1's corresponding component,
  // but make sure no resulting component has a negative value.
  //----------------------------------------------------------------------
  for (Lng32 vecIdx = 0; vecIdx < COUNT_OF_SIMPLE_COST_COUNTERS; vecIdx++)
    {
      resultVector.counter_[vecIdx] -= v2.counter_[vecIdx];
      //if ( resultVector.counter_[vecIdx] < csZero )
      //  {
      //    resultVector.counter_[vecIdx] = csZero;
      //  }
      (resultVector.counter_[vecIdx]).minCsZero();
    }

  //------------------------------------------------
  //  Set result's number of probes to that of v1's.
  //------------------------------------------------
  resultVector.setNumProbes(v1.getNumProbes());

  return resultVector;

} // operator-
//<pb>
//==============================================================================
//  Multiply a specified vector's components by a specified scalar.
//
// Note:  this operation leaves the normal memory, persistent memory and number
//       of probes unaffected.
//
// Input:
//  vector  -- Specified vector.
//
//  scalar  -- Specified scalar.
//
// Output:
//  none
//
// Return:
//  Specified vector multiplied by a specified scalar.
//
//==============================================================================
SimpleCostVector
operator*(const SimpleCostVector &vector, const CostScalar &scalar)
{

  //----------------------------------------
  //  Initialize result to specified vector.
  //----------------------------------------
  SimpleCostVector resultVector = vector;

  //--------------------------------------------------
  //  Multiply each component by the specified scalar.
  //--------------------------------------------------
  for (Lng32 vecIdx = 0; vecIdx < COUNT_OF_SIMPLE_COST_COUNTERS; vecIdx++)
    {
      resultVector.counter_[vecIdx] *= scalar;
    }

  //-------------------------------------------------------------------------
  //  Leave normal memory, persistent memory and number of probes unaffected.
  //-------------------------------------------------------------------------
  /*j as of 04/12/01 Normal & Persistent Memory not used for costing
      resultVector.setNormalMemory    ( vector.getNormalMemory()     );
      resultVector.setPersistentMemory( vector.getPersistentMemory() );
  j*/
  resultVector.setNumProbes       ( vector.getNumProbes()        );

  return resultVector;

} // operator*
//<pb>
//==============================================================================
//  Divide a specified vector's components by a specified scalar.
//
// Note:  this operation leaves the normal memory, persistent memory and number
//       of probes unaffected.
//
// Input:
//  vector  -- Specified vector numerator.
//
//  scalar  -- Specified scalar divisor.
//
// Output:
//  none
//
// Return:
//  Specified vector divided by a specified scalar.
//
//==============================================================================
SimpleCostVector
operator/(const SimpleCostVector &vector, const CostScalar &scalar)
{

  //----------------------
  //  No division by zero.
  //----------------------
  CMPASSERT( scalar.isGreaterThanZero() /* > csZero */ );

  //----------------------------------------
  //  Initialize result to specified vector.
  //----------------------------------------
  SimpleCostVector resultVector = vector;

  //------------------------------------------------
  //  Divide each component by the specified scalar.
  //------------------------------------------------
  for (Lng32 vecIdx = 0; vecIdx < COUNT_OF_SIMPLE_COST_COUNTERS; vecIdx++)
    {
      resultVector.counter_[vecIdx] /= scalar;
    }

  //-------------------------------------------------------------------------
  //  Leave normal memory, persistent memory and number of probes unaffected.
  //-------------------------------------------------------------------------
  /*j as of 04/12/01 Normal & Persistent Memory not used for costing
      resultVector.setNormalMemory    ( vector.getNormalMemory()     );
      resultVector.setPersistentMemory( vector.getPersistentMemory() );
  j*/
  resultVector.setNumProbes       ( vector.getNumProbes()        );

  return resultVector;

} // operator/
//<pb>
//==============================================================================
//  Add two specified vectors using blocking vector addition.
//
//  Blocking vector addition has the property that the sum of the elapsed times
// of the two specified vectors equals the elapsed time of their sum.  This is
// achieved by adding an appropriate amount to the idle time component of the
// result.
//
//  Elapsed time computations depend on a performance goal as specified in
// required physical properties.
//
// Note: this operation makes the number of probes in the result
//       vector equal to that of the first specified vector.
//
// Input:
//  v1  -- First specified vector.
//
//  v2  -- Second specified vector.
//
//  rpp -- Required physical properties containing specified performance goal.
//
// Output:
//  none
//
// Return:
//  Sum of v1 and v2 using blocking vector addition.
//
//==============================================================================
SimpleCostVector
blockingAdd(const SimpleCostVector &v1,
            const SimpleCostVector &v2,
            const ReqdPhysicalProperty* const rpp)
{

  //--------------------------------------------------------------------
  //  Initialize result to simple vector sum.  The Idle time component
  // will be adjusted below.
  //--------------------------------------------------------------------
  SimpleCostVector resultVector = v1 + v2;

  //-------------------------------------------------------------------------
  //  Make result vector's elapsed time equal to the sum of the elapsed times
  // of the two specified vectors.
  //-------------------------------------------------------------------------

  CostScalar elapsedTimeV1     = v1.getElapsedTime(rpp);
  CostScalar elapsedTimeV2     = v2.getElapsedTime(rpp);
  CostScalar elapsedTimeResult = resultVector.getElapsedTime(rpp);
  resultVector.addToIdleTime(elapsedTimeV1 + elapsedTimeV2 - elapsedTimeResult);

  return resultVector;

} // blockingAdd
//<pb>
//============================================================================
//  Unary case.  Add two specified vectors using overlapped vector addition.
//
//  Overlapped vector addition takes into account that in some cases we can
// overlap (at least partially) I/O and messaging.  Thus, the CPU, Normal
// Memory, Persistent Memory and Temporary Disk Space components get added as in
// normal vector addition, but the I/O and message related components are
// calculated using the following general formula:
//
//     result[C] = MAX(v1[C],v2[C]) + FF[C]*MIN(v1[C],v2[C])
//
//   In the above formula, FF[C] is a fractional fudge factor indicating the
// degree of overlap for component C.  Thus, for a complete overlap, FF[C] == 0.
// For no overlap (i.e. simple vector addition) FF[C] == 1.
//
//   Also, for unary overlapped addition, the idle time of one vector cannot be reduced
// by the active time of the other.
//
// Note: this operation makes the number of probes in the result
//       vector equal to that of the first specified vector.
//
// Input:
//  v1  -- First specified vector.
//
//  v2  -- Second specified vector.
//
// Output:
//  none
//
// Return:
//  Sum of v1 and v2 using overlapped vector addition.
//
//==============================================================================
SimpleCostVector
overlapAddUnary(const SimpleCostVector &v1, const SimpleCostVector &v2)
{

  //--------------------------------------------------------------------
  //  Initialize result to simple vector sum.  IO, Message and Idle time
  // components will be adjusted below.
  //--------------------------------------------------------------------
  SimpleCostVector resultVector = v1 + v2;

  //----------------------------------------------------
  // Obtain fudge factors for IO and Message components.
  //----------------------------------------------------
  double ff_IO	=
            CostPrimitives::getBasicCostFactor(MSCF_OV_IO);
  double ff_MSG =
            CostPrimitives::getBasicCostFactor(MSCF_OV_MSG);


  //------------------------
  //  Compute overlapped IO.
  //------------------------

  if ( v1.getIOTime() > v2.getIOTime() )
    resultVector.setIOTime (v1.getIOTime() +
				CostScalar (ff_IO) * v2.getIOTime());
  else
    resultVector.setIOTime (v2.getIOTime() +
				CostScalar (ff_IO) * v1.getIOTime());


  //-------------------------------
  //  Compute overlapped Messaging.
  //-------------------------------

  if ( v1.getMessageTime() > v2.getMessageTime() )
    resultVector.setMSGTime ( v1.getMessageTime() +
			 CostScalar (ff_MSG) * v2.getMessageTime() );
  else
    resultVector.setMSGTime ( v2.getMessageTime() +
			 CostScalar (ff_MSG) * v1.getMessageTime() );


  //---------------------------------------------------------------
  //  Set new idle time to sum of each vector's idle time.
  //---------------------------------------------------------------
  resultVector.setIdleTime(v1.getIdleTime() + v2.getIdleTime());


  return resultVector;

} // overlapAddUnary
//<pb>
//============================================================================
//  Add two specified vectors using overlapped vector addition.
//
//  Overlapped vector addition takes into account that in some cases we can
// overlap (at least partially) I/O and messaging.  Thus, the CPU, Normal
// Memory, Persistent Memory and Temporary Disk Space components get added as in
// normal vector addition, but the I/O and message related components are
// calculated using the following general formula:
//
//     result[C] = MAX(v1[C],v2[C]) + FF[C]*MIN(v1[C],v2[C])
//
//   In the above formula, FF[C] is a fractional fudge factor indicating the
// degree of overlap for component C.  Thus, for a complete overlap, FF[C] == 0.
// For no overlap (i.e. simple vector addition) FF[C] == 1.
//
//   Also, for overlapped addition, the idle time of one vector can be reduced
// by the active time of the other (and vice versa).  Idle time, however, can't
// become negative.
//
// Note: this operation makes the number of probes in the result
//       vector equal to that of the first specified vector.
//
// Input:
//  v1  -- First specified vector.
//
//  v2  -- Second specified vector.
//
// Output:
//  none
//
// Return:
//  Sum of v1 and v2 using overlapped vector addition.
//
//==============================================================================
SimpleCostVector
overlapAdd(const SimpleCostVector &v1, const SimpleCostVector &v2)
{

  //--------------------------------------------------------------------
  //  Initialize result to simple vector sum.  IO, Message and Idle time
  // components will be adjusted below.
  //--------------------------------------------------------------------
  SimpleCostVector resultVector = v1 + v2;

  //----------------------------------------------------
  // Obtain fudge factors for IO and Message components.
  //----------------------------------------------------
  double ff_IO         =
            CostPrimitives::getBasicCostFactor(MSCF_OV_IO);
  double ff_MSG =
            CostPrimitives::getBasicCostFactor(MSCF_OV_MSG);


  //------------------------
  //  Compute overlapped IO.
  //------------------------

  if ( v1.getIOTime() > v2.getIOTime() )
    resultVector.setIOTime (v1.getIOTime() +
			      CostScalar (ff_IO) * v2.getIOTime());
  else
    resultVector.setIOTime (v2.getIOTime() +
			      CostScalar (ff_IO) * v1.getIOTime());


  //-------------------------------
  //  Compute overlapped Messaging.
  //-------------------------------

  if ( v1.getMessageTime() > v2.getMessageTime() )
    resultVector.setMSGTime ( v1.getMessageTime() +
			CostScalar (ff_MSG) * v2.getMessageTime() );
  else
    resultVector.setMSGTime ( v2.getMessageTime() +
			CostScalar (ff_MSG) * v1.getMessageTime() );


  double minIdleTime = v1.getIdleTime().value();
  double v2IdleTime = v2.getIdleTime().value();
  double elapsedTimeDif =
      v1.getElapsedTime(*(CURRSTMT_OPTDEFAULTS->getDefaultPerformanceGoal())).value() -
      v2.getElapsedTime(*(CURRSTMT_OPTDEFAULTS->getDefaultPerformanceGoal())).value();
  if ( minIdleTime < v2IdleTime )
  {
      if ( elapsedTimeDif > 0.0 )
        // try to reduce max idle time which is v2IdleTime
        minIdleTime = MAXOF(minIdleTime, v2IdleTime - elapsedTimeDif);
      else
        // no space in v1-v2 to spread idle time difference
        minIdleTime = v2IdleTime;
  }
  else
  // v1 v2 change places, v1 has bigger idle time
  {
      if ( elapsedTimeDif < 0.0 )
        // try to spread idle time difference over v2-v1 vector
        minIdleTime = MAXOF(v2IdleTime, minIdleTime + elapsedTimeDif);
      else
      ; //no space in v2-v1 to spread idle time difference
  }

  resultVector.setIdleTime(minIdleTime);
/*
  //--------------------------------------------------------------------------
  //  Make sure the following equality holds:
  //
  //     ElapsedTime(result) >= MAX( ElapsedTime(v1), ElapsedTime(v2) )
  //
  //  In other words, don't let result vector have a smaller elapsed time than
  // either input vector.
  //--------------------------------------------------------------------------
  CostScalar lowerBoundTime
                        = MAXOF( v1.getElapsedTime(*DefaultPerformanceGoal),
                                 v2.getElapsedTime(*DefaultPerformanceGoal) );

  CostScalar resultTime = resultVector.getElapsedTime(*DefaultPerformanceGoal);

  if (resultTime < lowerBoundTime)
    {
      resultVector.addToIdleTime(lowerBoundTime - resultTime);
    }

  //----------------------------------------------------------------------------
  //  At this point, one certainly expects the result vector's elapsed time to
  // exceed the lower bound, but due to the vagueries of floating point
  // arithmetic, the result vector's elapsed time could still be just a tiny bit
  // below the lower bound.  By adding back double the difference, no matter how
  // flaky the floating point arithmetic is, we can guarantee the loop below
  // eventually terminates.
  //----------------------------------------------------------------------------
  resultTime = resultVector.getElapsedTime(*DefaultPerformanceGoal);
  while ((resultTime != lowerBoundTime) && (resultTime < lowerBoundTime))
    {
      resultVector.addToIdleTime( (lowerBoundTime - resultTime) * csTwo );
      resultTime = resultVector.getElapsedTime(*DefaultPerformanceGoal);
    }
*/
  return resultVector;

} // overlapAdd
//<pb>
//==============================================================================
//  Of two specified vectors, return the one having the smallest elapsed time.
//
// Note:  If neccessary, callers of this routine must assure proper
//       normalization of the two specified vectors
//
// Input:
//  v1  -- First specified vector.
//
//  v2  -- Second specified vector.
//
//  rpp -- Required physical properties containing specified performance goal.
//
// Output:
//  none
//
// Return:
//  Vector having smallest elapsed time.
//
//==============================================================================
SimpleCostVector
etMINOF(const SimpleCostVector &v1,
        const SimpleCostVector &v2,
        const ReqdPhysicalProperty* const rpp)
{

  if (v1.getElapsedTime(rpp) < v2.getElapsedTime(rpp)) {
    return v1;
  }
  else {
    return v2;
  }

} //etMINOF
//<pb>
//==============================================================================
//  Of two specified vectors, return the one having the largest elapsed time.
//
// Note:  If neccessary, callers of this routine must assure proper
//       normalization of the two specified vectors
//
// Input:
//  v1  -- First specified vector.
//
//  v2  -- Second specified vector.
//
//  rpp -- Required physical properties containing specified performance goal.
//
// Output:
//  none
//
// Return:
//  Vector having largest elapsed time.
//
//==============================================================================
SimpleCostVector
etMAXOF(const SimpleCostVector &v1,
        const SimpleCostVector &v2,
        const ReqdPhysicalProperty* const rpp)
{

  if (v1.getElapsedTime(rpp) > v2.getElapsedTime(rpp)) {
    return v1;
  }
  else {
    return v2;
  }

} //etMAXOF
//<pb>
//==============================================================================
//  Form a new vector out of two specified vectors such that each component
// of the resulting vector has the minimum value of the two corresponding
// components in the specified vectors.  For IO and Message components, choose
// from the vector having the minimum IO time and Message time respectively.
//
// Note: this operation makes the number of probes in the result
//       vector equal to that of the first specified vector.
//
// Input:
//  v1  -- First specified vector.
//
//  v2  -- Second specified vector.
//
// Output:
//  none
//
// Return:
//  Vector with minimum values in each component.
//
//==============================================================================
SimpleCostVector
vecMINOF(const SimpleCostVector &v1, const SimpleCostVector &v2)
{

  SimpleCostVector resultVector;

  //-----------------------------------------------------------------
  // Choose minimum for all components except
  // NUM_PROBES which is the last component of vector
  // so repeating the loop < "COUNT_OF_SIMPLE_COST_COUNTERS - 1" times
  //------------------------------------------------------------------

  for(short index=0;index<COUNT_OF_SIMPLE_COST_COUNTERS - 1;index++)
    resultVector.counter_[index] = MINOF(v1.counter_[index],
                                          v2.counter_[index] );

  //---------------------------------------------------------------
  //  By convention, set result's number of probes to that of v1's.
  //---------------------------------------------------------------
  resultVector.setNumProbes( v1.getNumProbes() );

  return resultVector;

} //vecMINOF
//<pb>
//==============================================================================
//  Form a new vector out of two specified vectors such that each component
// of the resulting vector has the maximum value of the two corresponding
// components in the specified vectors.  For IO and Message components, choose
// from the vector having the maximum IO time and Message time respectively.
//
// Note: this operation makes the number of probes in the result
//       vector equal to that of the first specified vector.
//
// Input:
//  v1  -- First specified vector.
//
//  v2  -- Second specified vector.
//
// Output:
//  none
//
// Return:
//  Vector with maximum values in each component.
//
//==============================================================================
SimpleCostVector
vecMAXOF(const SimpleCostVector &v1, const SimpleCostVector &v2)
{

  SimpleCostVector resultVector;

  //-----------------------------------------------------------------
  // Choose maximum for all components except
  // NUM_PROBES which is the last component of vector
  // so repeating the loop < "COUNT_OF_SIMPLE_COST_COUNTERS - 1" times
  //------------------------------------------------------------------

  for(short index=0;index<COUNT_OF_SIMPLE_COST_COUNTERS - 1;index++)
    resultVector.counter_[index] = MAXOF(v1.counter_[index],
                                          v2.counter_[index] );

  //---------------------------------------------------------------
  //  By convention, set result's number of probes to that of v1's.
  //---------------------------------------------------------------
  resultVector.setNumProbes( v1.getNumProbes() );

  return resultVector;

} //vecMAXOF
//<pb>
//==============================================================================
//  Determine if first specified vector is a lower bound for second specified
// vector (i.e. each component of first vector is less than or equal to the
// second vector's corresponding component).
//
// Input:
//  v1  -- First specified vector.
//
//  v2  -- Second specified vector.
//
// Output:
//  none
//
// Return:
//  True if v1 is a lower bound for v2; false otherwise.
//
//==============================================================================
NABoolean
isLowerBound(const SimpleCostVector &v1, const SimpleCostVector &v2)
{

  //---------------------------------------------------------------
  //  Verify that each component of v1 is less than or equal to the
  // corresponding component of v2.
  //---------------------------------------------------------------
  for (Lng32 vecIdx = 0; vecIdx < COUNT_OF_SIMPLE_COST_COUNTERS; vecIdx++)
    {
      if (v1.counter_[vecIdx] > v2.counter_[vecIdx])
        {
          return FALSE;
        }
    }

  return TRUE;

} // isLowerBound()
//<pb>

// -----------------------------------------------------------------------
// methods for class SimpleCostVector used in SCM.
// -----------------------------------------------------------------------

SimpleCostVector::SimpleCostVector(
                                   const CostScalar & CPUTime,
                                   const CostScalar & IOTime,
                                   const CostScalar & MSGTime,
                                   const CostScalar & idleTime,
                                   const CostScalar & tcProc,
                                   const CostScalar & tcProd,
                                   const CostScalar & tcSent,
                                   const CostScalar & ioRand,
                                   const CostScalar & ioSeq,
                                   const CostScalar & numProbes)

{
  counter_[CPU_TIME]                = CPUTime;
  counter_[IO_TIME]                 = IOTime;
  counter_[MSG_TIME]                = MSGTime;
  counter_[IDLE_TIME]               = idleTime;
  counter_[TC_PROC]                 = tcProc;
  counter_[TC_PROD]                 = tcProd;
  counter_[TC_SENT]                 = tcSent;
  counter_[IO_RAND]                 = ioRand;
  counter_[IO_SEQ]                  = ioSeq;
  counter_[NUM_PROBES]              = numProbes;
}

// -----------------------------------------------------------------------
// methods for class SimpleCostVector used in OCM.
// -----------------------------------------------------------------------

SimpleCostVector::SimpleCostVector(
				   const CostScalar & CPUTime,
				   const CostScalar & IOTime,
				   const CostScalar & MSGTime,
				   const CostScalar & idleTime,
				   const CostScalar & numProbes)

{
  counter_[CPU_TIME]                = CPUTime;
  counter_[IO_TIME]                 = IOTime;
  counter_[MSG_TIME]                = MSGTime;
  counter_[IDLE_TIME]               = idleTime;
  counter_[TC_PROC]                 = csZero;
  counter_[TC_PROD]                 = csZero;
  counter_[TC_SENT]                 = csZero;
  counter_[IO_RAND]                 = csZero;
  counter_[IO_SEQ]                  = csZero;
  counter_[NUM_PROBES]              = numProbes;
}

Lng32 SimpleCostVector::entries() const
{
  return COUNT_OF_SIMPLE_COST_COUNTERS;
}
//<pb>
CostScalar SimpleCostVector::operator[] (Lng32 ix) const
{
  CMPASSERT((ix >= 0) AND (ix < COUNT_OF_SIMPLE_COST_COUNTERS));
  return counter_[ix];
}

SimpleCostVector::SimpleCostVector(const SimpleCostVector &other)
{
  for (Lng32 i = 0; i < COUNT_OF_SIMPLE_COST_COUNTERS; i++)
     counter_[i] = other.counter_[i];
}

SimpleCostVector & SimpleCostVector::operator = (const SimpleCostVector &other)
{
  for (Lng32 i = 0; i < COUNT_OF_SIMPLE_COST_COUNTERS; i++)
     counter_[i] = other.counter_[i];

  return *this;
}
// Simple addition of vectors; does not affect the number of probes, and they
// must be the same in both vectors

SimpleCostVector & SimpleCostVector::operator += (const SimpleCostVector &other)
{

  *this = *this + other;
  return *this;
}
//<pb>

// Simple substraction of vectors; does not affect the number of probes, and
// they must be the same in both vectors; all negatives in the result are set
// to zero

SimpleCostVector & SimpleCostVector::operator -= (const SimpleCostVector
                                                                 &other)
{

  *this = *this - other;
  return *this;
}
//<pb>

// Simple multiplication of vector by an scalar; does not affect memory
// components or the number of probes

SimpleCostVector & SimpleCostVector::operator *= (const CostScalar &other)
{

  *this = *this * other;
  return *this;
}
//<pb>

// Simple division of vector by an scalar; does not affect memory
// components or the number of probes

SimpleCostVector & SimpleCostVector::operator /= (const CostScalar &other)
{

  *this = *this / other;
  return *this;
}
//<pb>

// prints the private members of this class
/*
void SimpleCostVector::print(FILE* ofd) const
{
  fprintf(ofd, "CPU cost: %g \n CPU time: %g \n Temporary disk usage: %g \n Idle time: %g \n IO time: %g \n Kilobytes of local messages: %g \n Kilobytes of remote messages: %g \n Time spent processing messages: %g \n Normal memory: %g \n Kilobytes of I/O transfered: %g \n Number of local messages: %g \n Number of probes: %g \n Number of remote messages: %g \n Number of seeks: %g \n Amount of memory persisting, in KB: %g \n",
  getCPUCost().value(), getCPUTime().value(), getDiskUsage().value(),
  getIdleTime().value(), getIOTime().value(),
  getKBLocalMessages().value(), getKBRemoteMessages().value(),
  getMessageTime().value(), getNormalMemory().value(), getNumKBytes().value(),
  getNumLocalMessages().value(), getNumProbes().value(),
  getNumRemoteMessages().value(), getNumSeeks().value(),
  getPersistentMemory().value());
}
*/

// excluded for coverage because it's a debug code
void SimpleCostVector::print(FILE* pfp) const
{
  fprintf(pfp,"CPUTime=%g\n",counter_[CPU_TIME].value());
  fprintf(pfp,"IOTime=%g\n",counter_[IO_TIME].value());
  fprintf(pfp,"MSGTime=%g\n",counter_[MSG_TIME].value());
  fprintf(pfp,"idleTime=%g\n",counter_[IDLE_TIME].value());
  fprintf(pfp,"tuple processed=%g\n",counter_[TC_PROC].value());
  fprintf(pfp,"tuple produced=%g\n",counter_[TC_PROD].value());
  fprintf(pfp,"tuple sent=%g\n",counter_[TC_SENT].value());
  fprintf(pfp,"IO rand=%g\n",counter_[IO_RAND].value());
  fprintf(pfp,"IO seq=%g\n",counter_[IO_SEQ].value());
  fprintf(pfp,"num Probes=%g\n",counter_[NUM_PROBES].value());
/*
  for (Lng32 i = 0; i < COUNT_OF_SIMPLE_COST_COUNTERS; i++)
    fprintf(pfp,"%g,",counter_[i].value());
*/
  fprintf(pfp,"\n");
}

//<pb>

//<pb>
// Return a string reporting the details of the cost vector in terms of
// its individual components, based on the cost model in use
// The argument ownline indicates if the cost components need to be in their
// own lines.
// The argument prefix optionally gives a prefix string to be printed
// This gets called only when internal CQD EXPLAIN_DETAIL_COST_FOR_CALIBRATION
// is ON for debugging purpose. So, exclude for coverage
const NAString SimpleCostVector::getDetailDesc(const DefaultToken ownline,
					       const char *prefix) const
{
  NAString dtlDesc(CmpCommon::statementHeap());
  // Declare detail so that we never have a string larger than this.
  char detail[400];
  char separator[80];


  // When details need to be on their own lines use new line as separator
  if (ownline == DF_ON)
  {
    snprintf(separator, sizeof(separator), "\n%s", prefix);
  }
  else
  {
    separator[0] = '\0';
  }

  // printing all the cost scalars of a simple cost vector
  if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
  {
    snprintf(detail, sizeof(detail),
	    "%sTC_PROC: %g %sTC_PROD: %g %sTC_SENT: %g %sIO_SEQ: %g %sIO_RAND: %g",
	    prefix,
	    counter_[TC_PROC].value(), separator,
	    counter_[TC_PROD].value(), separator,
	    counter_[TC_SENT].value(), separator,
	    counter_[IO_SEQ].value(), separator,
	    counter_[IO_RAND].value());
  }
  else
  {
    snprintf(detail, sizeof(detail),
	    "%sCPU_TIME: %g %sIO_TIME: %g %sMSG_TIME: %g %sIDLE_TIME: %g %sPROBES: %g",
	    prefix,
	    MINOF(counter_[CPU_TIME], 1e32).value(),  separator,
	    MINOF(counter_[IO_TIME], 1e32).value(), separator,
	    counter_[MSG_TIME].value(), separator,
	    counter_[IDLE_TIME].value(), separator,
	    counter_[NUM_PROBES].value());
  }
  dtlDesc = detail;
  return dtlDesc;
} // SimpleCostVector::getDetailDesc()

//<pb>
// NCM specific method.
CostScalar SimpleCostVector::getElapsedTime() const
{
  // assert if called by OCM .
  DCMPASSERT(CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON);

  //Moved from scmComputeTotalCost() method.
  CostScalar seqIOWeight, randIOWeight;
  seqIOWeight = ActiveSchemaDB()->getDefaults().getAsDouble(NCM_SEQ_IO_WEIGHT);
  randIOWeight = ActiveSchemaDB()->getDefaults().getAsDouble(NCM_RAND_IO_WEIGHT);
  CostScalar tuplesProcessed = getTcProc();
  CostScalar tuplesProduced = getTcProd();
  CostScalar tuplesSent = getTcSent();

  CostScalar seqIOs = getIoSeq();
  CostScalar randIOs = getIoRand();

  return (((tuplesProcessed + tuplesProduced) + tuplesSent) +
          ((seqIOs * seqIOWeight) + (randIOs * randIOWeight)));

}
// Finds elapsed time in vector

CostScalar SimpleCostVector::getElapsedTime(const ReqdPhysicalProperty* const
                                                                   rpp) const
{
  const CostWeight* cw;
  const PerformanceGoal *goal;
  if (rpp) {
    goal = rpp->getPerformanceGoal();
    cw = rpp->getCostWeight();
  }
  else {
    goal = CURRSTMT_OPTDEFAULTS->getDefaultPerformanceGoal();
    cw = CURRSTMT_OPTDEFAULTS->getDefaultCostWeight();
  }

  return getElapsedTime(*goal,cw);
}
//<pb>

// Finds elapsed time in vector. Kind can be last row, first row or total.

CostScalar SimpleCostVector::getElapsedTime(const PerformanceGoal& goal,
                                    const CostWeight* const vectorWeight) const
{
  CMPASSERT(goal.isOptimizeForFirstRow() || goal.isOptimizeForLastRow() ||
			goal.isOptimizeForResourceConsumption());

  CostScalar et = csZero;

  const CostScalar & message = getMessageTime();
  const CostScalar & cpu     = getCPUTime();
  const CostScalar & io      = getIOTime();

  if (goal.isOptimizeForResourceConsumption())
  {
    CostWeight *vecWeight;
    if (vectorWeight == NULL)
      vecWeight = CURRSTMT_OPTDEFAULTS->getDefaultCostWeight();
    else
      vecWeight = (CostWeight*) vectorWeight;
    et = vecWeight->convertToElapsedTime(*this);
  }

  else if ((goal.isOptimizeForFirstRow() || goal.isOptimizeForLastRow()) &&
           (CmpCommon::getDefault(TOTAL_RESOURCE_COSTING) == DF_OFF))
    {
      const CostScalar & maxOfIOAndCpuAndMessage =
        MAXOF(io, MAXOF(message,cpu));

      et = maxOfIOAndCpuAndMessage + getIdleTime();
    }
  else // additive resource costing
    {

      // For now, just add the various resource units (converted
      // to time) together. In the future, may want to consider
      // waiting each component by the appropriate cost weight.

      et = message + cpu + io;
    }

  return et;
}


// This routine has the effect of setting the number of probes
// to the input factor and scaling all other members accordingly
// with the exception of the memory components. The intention
// is to be able to do operations between the resulting vector
// and another vector whose number of probes is factor.

const SimpleCostVector& SimpleCostVector::normalize(const CostScalar & factor)
{
  CMPASSERT( factor.isGreaterThanZero() /* > csZero */ );

  //-------------------------------------------------------------------
  //  Normalization does not affect memory values, so save them off and
  // restore them later.
  //-------------------------------------------------------------------
/*j  const CostScalar normalMemory     = getNormalMemory();
  const CostScalar persistantMemory = getPersistentMemory();
j*/

  //---------------------------------------------
  //  Normalize each component to the new factor.
  //---------------------------------------------
  const CostScalar numProbes = getNumProbes() / factor;
  for (Lng32 vecIdx = 0; vecIdx < COUNT_OF_SIMPLE_COST_COUNTERS; vecIdx++)
  {
    counter_[vecIdx] *= numProbes;
  }

  //-----------------------------------------------------------------------
  //  Restore original memory values, and set number of probes to specified
  // factor.
  //-----------------------------------------------------------------------
  /*j setNormalMemory(normalMemory);
  setPersistentMemory(persistantMemory);
  j*/
  setNumProbes(factor);

  return *this;
}
//<pb>
//==============================================================================
//  Return a copy of this vector normalized to a specified factor.  Do not
// change this vector.
//
// Input:
//  factor -- specified factor to which this vector should be normalized.
//
// Output:
//  none
//
// Return:
//  normalized cost.
//
//==============================================================================
SimpleCostVector
SimpleCostVector::getNormalizedVersion(const CostScalar & factor) const
{

  SimpleCostVector normVec(*this);
  return normVec.normalize(factor);

} // SimpleCostVector::getNormalizedVersion
//<pb>

//==============================================================================
//  Add a specified vector to this vector using overlapped vector addition.
//
//  Overlapped vector addition takes into account that in some cases we can
// overlap (at least partially) I/O and messaging.  Thus, the CPU, Normal
// Memory, Persistent Memory and Temporary Disk Space components get added as in
// normal vector addition, but the I/O and message related components are
// calculated using the following general formula:
//
//     result[C] = MAX(v1[C],v2[C]) + FF[C]*MIN(v1[C],v2[C])
//
//   In the above formula, FF[C] is a fractional fudge factor indicating the
// degree of overlap for component C.  Thus, for a complete overlap, FF[C] == 0.
// For no overlap (i.e. simple vector addition) FF[C] == 1.
//
//   Also, for overlapped addition, the idle time of one vector can be reduced
// by the active time of the other (and vice versa).  Idle time, however, can't
// become negative.
//
// Note: this operation makes the number of probes in the result
//       vector equal to that of the first specified vector.
//
// Input:
//  other -- other vector whose value will be accumulated into this vector
//            using overlapped addition.
//
// Output:
//  none
//
// Return:
//  Accumulated vector using overlapped addition.
//
//==============================================================================
const SimpleCostVector&
SimpleCostVector::overlappedAdd(const SimpleCostVector& other)
{

  *this = overlapAddUnary(*this,other);
  return *this;

} // SimpleCostVector::overlappedAdd
//<pb>
//==============================================================================
//  Add  a specified number of copies of this vector using overlapped vector
// addition .
//
// Note:  this operation leaves the normal memory, persistent memory and number
//       of probes unaffected.
//
// Input:
//  times -- specified number of copies of this vector to add.
//
// Output:
//  none
//
// Return:
//  Result of adding vector to itself a repeated number of times.
//
//==============================================================================
const SimpleCostVector&
SimpleCostVector::repeatedOverlappedAdd(const Lng32 times)
{

  //---------------------------------------------------------------------------
  //  Number of times must be positive.  If it equals 1, we have nothing to do,
  // so return immediately.
  //---------------------------------------------------------------------------
  if (times == 1)
    {
      return *this;
    }
  CMPASSERT(times >= 1);

  //-------------------------------------------------------------------------
  //  For idle time, assume that only two instances of this vector overlap at
  // any given time.  In other words, determine the remaining idle time when
  // this vector is added to itself once, and allot that idle time to all
  // pairs of instances of this vector.
  //-------------------------------------------------------------------------
  SimpleCostVector temp = *this;
  temp.overlappedAdd(*this);
  counter_[IDLE_TIME] = temp.getIdleTime() * (times -1);

  //-------------------------------------------------------------------------
  //  CPU and disk usage not overlappable, so use traditional multiplication.
  //-------------------------------------------------------------------------
  counter_[CPU_TIME]   *= times;
  //j counter_[DISK_USAGE] *= times;

  //-------------------------------------------------------------------------
  //  Normal and persistent memory not overlappable, so use traditional
  //  multiplication. This was added to use repeatedOverlappedAdd instead of
  //  multiple calls for overlappedAdd with the same cast vector. SP 09/06/00
  //-------------------------------------------------------------------------
  /*j counter_[NORMAL_MEMORY]     *= times;
  counter_[PERSISTENT_MEMORY] *= times;
  j*/

  //----------------------------------------------------
  // Obtain fudge factors for IO and Message components.
  //----------------------------------------------------
  double ff_IO         =
            CostPrimitives::getBasicCostFactor(MSCF_OV_IO);
  double ff_MSG         =
            CostPrimitives::getBasicCostFactor(MSCF_OV_MSG);

  //---------------------------------------------------------------------------
  //  Calculate repeated overlapped addition for the IO and Message components.
  //
  //  Recall the formula for overlapped addition:
  //
  //     result[C] = MAX(v1[C],v2[C]) + FF[C]*MIN(v1[C],v2[C])
  //
  //  When v1 == v2, this reduces to:
  //
  //     result[C] = v1[C] + FF[C]*v1[C].
  //
  //  For "n" repeated overlapped additions, this becomes:
  //
  //       result[C] = v1[C] + FF[C]*v1[C]*(n - 1).
  //
  //---------------------------------------------------------------------------
  counter_[IO_TIME]
           += (counter_[IO_TIME]  * ff_IO  * (times - 1) );
  counter_[MSG_TIME]
           += (counter_[MSG_TIME] * ff_MSG * (times - 1) );

  return *this;

} //SimpleCostVector::repeatedOverlappedAdd()
//<pb>
// -----------------------------------------------------------------------
// scaleUpByNumProbes() scales up a simple cost vector by its number of
// probes. Memory and disk space are considered to be recyclable over
// probes, and hence not scaled up by this method.
// -----------------------------------------------------------------------
const SimpleCostVector& SimpleCostVector::scaleUpByNumProbes()
{
  const CostScalar & numProbes = counter_[NUM_PROBES];

  // Multiply all the components of the vector except the "NumProbes"
  // normal memory, persitent memory & diskusage, as Numprobes in
  // the last component and we don't have Normal, persistent memory
  // and diskusage currently 04/2001 repeat loop
  // "count_of_simple_cost_counters-1" times AND LEAVING PROBES

  // Check for overflow. If overflow initialize counter_[i] by DBL_MAX
  // The problem occurs only on NSK

//  short overflow=0;    //to check for overflow


  for( Lng32 i = 0; i < COUNT_OF_SIMPLE_COST_COUNTERS - 1; i++ )
  {
    counter_[i] *= numProbes;
  }

  return *this;
}
//<pb>

// -----------------------------------------------------------------------
// scaleUpByCountOfCPUs() scales up a simple cost vector by the number
// of CPUs. This is typically used to convert a simple cost vector which
// tracks resource usage on a CPU to another simple cost vector which
// tracks total resource usage over all CPUs. All components except the
// no of probes are scaled up.
// -----------------------------------------------------------------------
const SimpleCostVector& SimpleCostVector::scaleUpByCountOfCPUs(
                                                   const Lng32 countOfCPUs)
{
  return scaleByValue( countOfCPUs );
}
//<pb>
//==============================================================================
//  Scale (up or down) this vector by a specified scalar value but leaving
// number of probes unaffected.
//
// Input:
//  scalar  -- Specified scalar.
//
// Output:
//  none
//
// Return:
//  This vector appropriately scaled.
//
//==============================================================================
SimpleCostVector&
SimpleCostVector::scaleByValue(const CostScalar &scalar)
{

  //--------------------------------------------------
  //  Multiply each component by the specified scalar
  //  except the NUMPROBES which is the last component
  // so just run the loop to "count_of_simple_cost_counters-1" times
  //--------------------------------------------------
  for (Lng32 vecIdx = 0; vecIdx < COUNT_OF_SIMPLE_COST_COUNTERS - 1; vecIdx++)
    {
      counter_[vecIdx] *= scalar;
    }

  return *this;

} // SimpleCostVector::scaleByValue()
//<pb>

//-------------------------------------------------------------------------
// SimpleCostVector::setToValue()
//--------------------------------------------------------------------------
void SimpleCostVector::setToValue(const CostScalar &scalar)
{

   //--------------------------------------------------
   //  set each component to the specified scalar
   //  except the NUMPROBES which is the last component
   //--------------------------------------------------
  for (Lng32 vecIdx = 0; vecIdx < COUNT_OF_SIMPLE_COST_COUNTERS - 1; vecIdx++)
  {
    counter_[vecIdx] = scalar;
  }

   return;
}
//==============================================================================
//  Ensure no component of this vector has no component that exceeds the
// corresponding component of a specified upper bound vector.
//
// Input:
//  upperBoundVector -- specified vector which serves as an upper bound.
//
// Output:
//  none
//
// Return:
//  This vector appropriately bounded.
//
//==============================================================================
const SimpleCostVector&
SimpleCostVector::enforceUpperBound(const SimpleCostVector& upperBoundVector)
{

  //--------------------------------------------------------------------------
  //  If any component of this vector exceeds the corresponding component of a
  // specified upper bound vector, reduce this vector's component to the upper
  // bound vector's corresponding component.
  //--------------------------------------------------------------------------
  for (Lng32 vecIdx = 0; vecIdx < COUNT_OF_SIMPLE_COST_COUNTERS; vecIdx++)
    {
      if (counter_[vecIdx] > upperBoundVector.counter_[vecIdx])
        {
          counter_[vecIdx] = upperBoundVector.counter_[vecIdx];
        }
    }

  return *this;

} // SimpleCostVector::enforceUpperBound()

//<pb>
// Resets all components to zero

const SimpleCostVector& SimpleCostVector::reset()
{
  for (Lng32 i = 0; i < COUNT_OF_SIMPLE_COST_COUNTERS; i++) {
     counter_[i] = csZero;
  }

  // But Number of probes to be atleast one.
  setNumProbes(csOne);

  return *this;

}
//<pb>
//==============================================================================
//  Indicates whether or not this object is a zero-vector.
//
//  Note:  number of probes is not taken into account when determining if this
//        is a zero vector.
//
// Input:
//  none
//
// Output:
//  none
//
// Return:
//  TRUE if this is a zero vector; FALSE otherwise.
//
//==============================================================================
// This method is no longer used, isZeroVectorWithProbes is used instead.
// So hide it from coverage
NABoolean
SimpleCostVector::isZeroVector() const
{
  Lng32 vecIdx;
  //-----------------------------------------------------------------------
  //  Check all components (except number of probes).  If any are non-zero,
  // report that this is not a zero vector.
  //-----------------------------------------------------------------------
  for (vecIdx = 0; vecIdx < NUM_PROBES; vecIdx++)
    {
      if ( (counter_[vecIdx]).isGreaterThanZero() /* != csZero */)
        {
          return FALSE;
        }
    }

  //------------------------------------------------------------
  //  All relevant components have a zero value, so return TRUE.
  //------------------------------------------------------------
  return TRUE;

} // SimpleCostVector::isZero

// This method needs to be used instead of isZeroVector for checking
// blocking components of Cost Object: cpbcTotal and cpbc1.
// Otherwise we can have a case when after normalizing SimpleCostVector
// with huge number of probes it could look like zero vector - all
// components are less that COSTSCALAR EPSILON - but in fact it is not.
// This might cause inconsistency in costing.
// See Genesis case : 10-031021-5359

NABoolean
SimpleCostVector::isZeroVectorWithProbes() const
{
  Lng32 vecIdx;
  const CostScalar numProbes = getNumProbes();

  //-----------------------------------------------------------------------
  //  Check all components (except number of probes).  If any are non-zero,
  // report that this is not a zero vector.
  //-----------------------------------------------------------------------
  if ( numProbes.isGreaterThanOne() )
  {
    for (vecIdx = 0; vecIdx < NUM_PROBES; vecIdx++)
    {
      if ( (counter_[vecIdx]*numProbes).isGreaterThanZero() /* != csZero */)
        {
          return FALSE;
        }
    }
  }
  else
  // here we avoid unnecessary CostScalar multiplications
  {
    for (vecIdx = 0; vecIdx < NUM_PROBES; vecIdx++)
    {
      if ( (counter_[vecIdx]).isGreaterThanZero() /* != csZero */)
        {
          return FALSE;
        }
    }
  }
  //------------------------------------------------------------
  //  All relevant components have a zero value, so return TRUE.
  //------------------------------------------------------------
  return TRUE;

} // SimpleCostVector::isZeroVectorWithProbes

//<pb>
// -----------------------------------------------------------------------
// methods for class Cost
// -----------------------------------------------------------------------

//==============================================================================
//  Cost Constructor.
//
// Input:
//  currentProcessFirstRowCost -- resources necessary to produce operator's
//                                 first row once blocking activity completes.
//
//  currentProcessLastRowCost  -- resources necessary to produce operator's
//                                 last row once blocking activity completes.
//
//  currentProcessBlockingCost -- resources necessary for blocking activity.
//
//  countOfCPUs                -- number of CPUs for executing this operator.
//
//  planFragmentsPerCPU        -- number of plan fragments per CPU.
//
// Output:
//  none
//
// Return:
//  none.
//
//==============================================================================
Cost::Cost(const SimpleCostVector* currentProcessFirstRowCost,
           const SimpleCostVector* currentProcessLastRowCost,
           const SimpleCostVector* currentProcessBlockingCost,
           const Lng32              countOfCPUs,
           const Lng32              planFragmentsPerCPU)
   : cpScmlr_(),
    cpScmDbg_(),
    cpbc1_(),
    cpbcTotal_(),
//jo    opfr_(),
//jo    oplr_(),
    countOfCPUs_(countOfCPUs),
    planFragmentsPerCPU_(planFragmentsPerCPU),
    priority_()
{

  //-----------------------------------------------
  // First Row and Last Row Cost MUST be supplied!!
  //-----------------------------------------------
  CMPASSERT(currentProcessFirstRowCost != NULL);
  CMPASSERT(currentProcessLastRowCost != NULL);

  //-----------------------------------------------
  // Checks must come first before using them!
  //-----------------------------------------------
  totalCost_ = *currentProcessLastRowCost;
  cpfr_	     = *currentProcessFirstRowCost;
  cplr_	     = *currentProcessLastRowCost;

#ifndef NDEBUG
  //--------------------------------------------------------------------------
  //  Ensure no component of first row exceeds corresponding component of last
  // row.
  //--------------------------------------------------------------------------
  CMPASSERT(isLowerBound(cpfr_,cplr_));
#endif

  //---------------------------------------------------------------------------
  //  Since cost vectors supplied to the Cost constructor are on a "per stream"
  // basis, we convert them to a "per CPU" basis.  We use overlapped addition
  // for first row and last row vectors because independent streams can overlap
  // IO and message costs.  For total cost, however, we use simple vector
  // addition to reflect actual resources used.
  // NOTE! Calls to overlappedAdd function were replaced by repeatedOverlappedAdd
  // to save time in case of multiple plan fragments per CPU.
  //---------------------------------------------------------------------------

  if ( planFragmentsPerCPU > 1 )
  {
    totalCost_ = totalCost_ * planFragmentsPerCPU;
    // NOTE!!! This multiplication operator * above will not multiply
    // the memory and persistent memory of the cost vector.
    // This needs to be done because if CPU has several plan fragments
    // then every fragment will require its own memory and persistent
    // memory. That's why we need to adjust them explicitly.

    /*j Memory not used currently 05/09/01
    totalCost_.setNormalMemory(totalCost_.getNormalMemory()
                               * planFragmentsPerCPU);
    totalCost_.setPersistentMemory(totalCost_.getPersistentMemory()
                                   * planFragmentsPerCPU);
    j*/

    // In the future this case needs further investigation.

    cpfr_.repeatedOverlappedAdd(planFragmentsPerCPU);
    cplr_.repeatedOverlappedAdd(planFragmentsPerCPU);
  }
  //----------------------------------------
  //  See if cost has any blocking activity.
  //----------------------------------------
  if(currentProcessBlockingCost != NULL)
    {

      //------------------------------------------------------------------------
      //  Cost has blocking activity.  Store blocking vector in cpbc1_ and in
      // a temporary vector used for reflecting blocking activity in total cost.
      //------------------------------------------------------------------------
      SimpleCostVector totalBlockingCost(*currentProcessBlockingCost);
      cpbc1_ = *currentProcessBlockingCost;

      //------------------------------------------------------------------
      //  Ensure that last row vector and blocking vector have same number
      // of probes.
      //------------------------------------------------------------------
      CCMPASSERT(cplr_.getNumProbes() == cpbc1_.getNumProbes());

      //---------------------------------------------------------------------
      //  As with first row and last row vectors, we need to convert the
      // blocking vector from a "per stream" basis to a "per CPU" basis.  For
      // total cost, use simple vector addition to reflect actual resources
      // used.
      //---------------------------------------------------------------------

      if ( planFragmentsPerCPU > 1 )
      {
        totalBlockingCost = totalBlockingCost * planFragmentsPerCPU;
        // NOTE!!! This multiplication operator * above will not multiply
        // the memory and persistent memory of the cost vector.
        // This needs to be done because if CPU has several plan fragments
        // then every fragment will require its own memory and persistent
        // memory. That's why we need to adjust them explicitly.

        /*j not used currently 05/2001
	totalBlockingCost.setNormalMemory(totalBlockingCost.getNormalMemory()
                                          * planFragmentsPerCPU);
        totalBlockingCost.setPersistentMemory(totalBlockingCost.getPersistentMemory()
                                          * planFragmentsPerCPU);
        j*/

	cpbc1_.repeatedOverlappedAdd(planFragmentsPerCPU);
      }
      //---------------------------------------------------------------------
      //  During preliminary costing when a cost object is initially created,
      // cpbcTotal_ equals cpbc1_ by convention.
      //---------------------------------------------------------------------
      cpbcTotal_ = cpbc1_;

      //---------------------------------------------------------------------
      //  Since blocking cost vectors reflect average usage per probe, we
      // need to scale up this vector by number of probes before accumulating
      // it in total cost.
      //
      //  Note that scaling up by number of probes does not affect memory or
      // temporary disk space used since we assume these can be reused for
      // every probe.
      // --------------------------------------------------------------------
      totalBlockingCost.scaleUpByNumProbes();
      totalCost_ += totalBlockingCost;

    }
  else
    {

      //--------------------------------------------------------------------
      //  No blocking vector supplied, so cpbc1_ and cpbcTotal_ will be zero
      // vectors.  For consistency, set their number of probes to that of
      // the supplied last row vector.
      //--------------------------------------------------------------------
      cpbc1_.setNumProbes(cplr_.getNumProbes());
      cpbcTotal_.setNumProbes(cplr_.getNumProbes());

    }

  //---------------------------------------------------------------------
  //  Since total cost reflects all resources used, we scale it up by the
  // number of CPUs executing this query.
  //---------------------------------------------------------------------
  totalCost_.scaleUpByCountOfCPUs(countOfCPUs);

} //  Cost Constructor.

//============================================================================
//  Cost Constructor used in SCM code.
//
// Input:
//  currentProcessScmCost -- resources used in SCM.
//
// Output:
//  none
//
// Return:
//  none.
//
//============================================================================
Cost::Cost(const SimpleCostVector* currentProcessScmCost)
  : cpScmDbg_(),
    cpfr_(),
    cplr_(),
    cpbc1_(),
    cpbcTotal_(),
    countOfCPUs_(1),
    planFragmentsPerCPU_(1),
    priority_()
{

  //-----------------------------------------------
  // SCM Cost MUST be supplied!!
  //-----------------------------------------------
  CMPASSERT(currentProcessScmCost != NULL);

  //-----------------------------------------------
  // Checks must come first before using them!
  //-----------------------------------------------
  cpScmlr_      = *currentProcessScmCost;
} //  Cost Constructor for SCM.

//============================================================================
//  Cost Constructor used in SCM code.
//
// Inputs:
//  currentProcessScmCost -- resources used in SCM.
//  currentOpDebugInfo -- debug information to be used internally only.
//
// Output:
//  none
//
// Return:
//  none.
//
//============================================================================
Cost::Cost(const SimpleCostVector* currentProcessScmCost,
	   const SimpleCostVector* currentOpDebugInfo)
  : cpfr_(),
    cplr_(),
    cpbc1_(),
    cpbcTotal_(),
    countOfCPUs_(1),
    planFragmentsPerCPU_(1),
    priority_()
{

  //-----------------------------------------------
  // SCM Cost MUST be supplied!!
  //-----------------------------------------------
  CMPASSERT(currentProcessScmCost != NULL);
  CMPASSERT(currentOpDebugInfo != NULL);

  //-----------------------------------------------
  // Checks must come first before using them!
  //-----------------------------------------------
  cpScmlr_      = *currentProcessScmCost;
  cpScmDbg_      = *currentOpDebugInfo;
} //  Cost Constructor for SCM.

//<pb>
///==============================================================================
//  Effectively a virtual constructor for a Cost object.
//
// Input:
//  none
//
// Output:
//  none
//
// Return:
//  none.
//
//==============================================================================
Cost*
Cost::duplicate()
{

  Cost* costPtr = new (CmpCommon::statementHeap()) Cost(*this);
  return costPtr;

}
//<pb>
//==============================================================================
//  Cost destructor.
//
// Input:
//  none
//
// Output:
//  none
//
// Return:
//  none.
//
//==============================================================================
Cost::~Cost()
{

} // Cost Destructor
//<pb>

// -----------------------------------------------------------------------
// Returns the cost vector "representative of" a performance goal. Read
// comments below.
// -----------------------------------------------------------------------
const SimpleCostVector& Cost::getCostVector(
                                   const PerformanceGoal* const pfg) const
{

  // ---------------------------------------------------------------------
  // It is doubtful whether we should support this method in the new Cost
  // object. For FR elapsed time optimization, we should optimize the sum
  // ET(cpfr_)+ET(cpbcTotal_). For LR elapsed time optimization, we should
  // optimize ET(cplr_)+ET(cpbcTotal_)*NumOfProbes. Thus, in those cases,
  // we don't have a representative vector whose ET() is to be optimized,
  // but rather, we have an expression made up of ET()'s of two vectors.
  // This method, however, gives the false impression that the returned
  // vector is the one we should optimize for the given performance goal.
  // ---------------------------------------------------------------------
   // SIMPLE_COST_MODEL check should be the first condition.
  if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
    return cpScmlr_;
  else if(pfg->isOptimizeForLastRow())
    return cplr_;
  else if(pfg->isOptimizeForFirstRow())
    return cpfr_;
  else // if(pfg->isOptimizeForResourceConsumption())
    return cplr_;
}
//<pb>

// -----------------------------------------------------------------------
// Compares two Cost objects according to the performance goal and cost
// weights if applicable, returns MORE if this > other, SAME if they are
// equal and LESS otherwise.
// -----------------------------------------------------------------------
COMPARE_RESULT Cost::compareCosts(
                              const Cost & other,
                              const ReqdPhysicalProperty* const rpp) const
{

  // Higher priority plans always win the comparison and considered "cheaper"
  if (priority_ > other.priority_)
    return LESS;
  if (priority_ < other.priority_)
    return MORE;

  // if SIMPLE_COST_MODEL call scmCompareCosts() function.
  if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
  {
    return scmCompareCosts(other, rpp);
  }

  // Plans with equal priorities proceed to real cost comparison

  // Fix for coverity cid #1373: pointer pfg is unused.
  // const PerformanceGoal* pfg;
  // pfg = (rpp ? rpp->getPerformanceGoal() : DefaultPerformanceGoal);

  const CostWeight * rcw;
  rcw = (rpp ? rpp->getCostWeight() : CURRSTMT_OPTDEFAULTS->getDefaultCostWeight());

  // ---------------------------------------------------------------------
  // If optimized for resource consumption, the set of cost weights
  // specified in rpp are used. The weights are implcitly used when the
  // elapsed time method is called.
  // ---------------------------------------------------------------------
  /*if(pfg->isOptimizeForResourceConsumption())
  {
    return
     rcw->compareCostVectors(getCostVector(pfg),other.getCostVector(pfg));
  }*/

  // ---------------------------------------------------------------------
  // We are optimizing for elapsed time
  // ---------------------------------------------------------------------
  ElapsedTime et1 (convertToElapsedTime(rpp) * priority_.riskPremium());
  ElapsedTime et2 (other.convertToElapsedTime(rpp) *
                   other.priority_.riskPremium());

  if ( CURRSTMT_OPTDEFAULTS->OPHuseCompCostThreshold() )
  {
	  double ratio = et1.getValue()/et2.getValue();
	  if ( ratio < 0.999999 )
		  return LESS;
	  else if (ratio > 1.000001)
		  return MORE;
  }
  else if (et1 > et2) return MORE;
  else if (et1 < et2) return LESS;

  // If the elapsed time cost is the same, then look at individual components
  // of last row cost as a tie breaker before trying total resources.
  if ( CmpCommon::getDefault(COMP_BOOL_95) == DF_OFF )
  {
    CostScalar cpu_io_msg_1 = getCplr().getCPUTime() + getCplr().getIOTime()
                            + getCplr().getMessageTime();
    CostScalar cpu_io_msg_2 = other.getCplr().getCPUTime()
                            + other.getCplr().getIOTime()
                            + other.getCplr().getMessageTime();
    if (cpu_io_msg_1 > cpu_io_msg_2)
      return MORE;
    else if (cpu_io_msg_1 < cpu_io_msg_2)
      return LESS;
  }

    // If the individual components of last row cost is the same,
    // then look at total cost (i.e. resources) as a tie-breaker.
    // In order to compare total resources, first convert it to
    // an elapsed time unit.
    CostScalar tc_et1 = getTotalCost().getElapsedTime
                          (*(CURRSTMT_OPTDEFAULTS->getResourcePerformanceGoal()),rcw);
    CostScalar tc_et2 = other.getTotalCost().getElapsedTime
                          (*(CURRSTMT_OPTDEFAULTS->getResourcePerformanceGoal()),rcw);
    if (tc_et1 > tc_et2)
      return MORE;
    else if (tc_et1 < tc_et2)
      return LESS;
    else
      return SAME;


} // Cost::compareCosts()
//<pb>

// -----------------------------------------------------------------------
// Elapsed time is just loosely interpreted as a single value representing
// the cost of the operation. It is used by Cascades to determine which
// plan costs less. In reality, it might be the real FR/LR elapsed time of
// the operation or it might be just a weighted sum of resource usage.
// -----------------------------------------------------------------------
ElapsedTime Cost::convertToElapsedTime(
                              const ReqdPhysicalProperty* const rpp) const
{

  // if SIMPLE_COST_MODEL call scmComputeTotalCost() function.
  if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
  {
    return scmComputeTotalCost();
  }
  const PerformanceGoal* pfg;
  pfg = (rpp ? rpp->getPerformanceGoal() : CURRSTMT_OPTDEFAULTS->getDefaultPerformanceGoal());

  // ---------------------------------------------------------------------
  // Assume the given set of cost weights in rpp converts the Cost object
  // into "ElapsedTime". Instead of taking totalCost_ for estimating
  // "elapsed time", we consider the last row cost and the blocking cost
  // even for the resource consumption goal
  // ---------------------------------------------------------------------

  /*if(pfg->isOptimizeForResourceConsumption())
  {
    CMPASSERT(rpp);
    return rpp->getCostWeight()->convertToElapsedTime(totalCost_);
  }*/


  // ---------------------------------------------------------------------
  // Use the built-in mechanism to convert the object into elapsed time.
  // The elapsed time of a Cost object is made up of two parts. The first
  // part is either the elapsed time of cpfr_ or cplr_ depending on the
  // performance goal. It represents the resource consumption in the
  // current process above the last seen blocking operator. The second
  // part is the elapsed time below the blocking operator. Total elapsed
  // time is just the algebraic sum of the two, since activities above the
  // blocking operator do not overlap with those below it.
  // ---------------------------------------------------------------------
  ElapsedTime et ( csZero );
  CostScalar etBlock = cpbcTotal_.getElapsedTime(*pfg);

  // FirstRow optimization is a disabled feature, hide from coverage
  if(pfg->isOptimizeForFirstRow())
  {
    // Assume we get our first row in the first probe.
    const CostScalar etFR = cpfr_.getElapsedTime(*pfg) + etBlock;
    et = ElapsedTime( etFR );
  }
  else if(pfg->isOptimizeForLastRow())
  {
    // cpbcTotal_ is a per-probe average cost.
    const CostScalar etLR = cplr_.getElapsedTime(*pfg);

    CostScalar etLRPlusBlk;

    etLRPlusBlk = etLR + etBlock * cpbcTotal_.getNumProbes();
    et = ElapsedTime( etLRPlusBlk );
  }
  // total resource consumption is a disabled feature, hide from coverage
  else if (pfg->isOptimizeForResourceConsumption())
  {
    // We use LR cost and BK costs for resource consumption goal as well
    // The difference is that each component may get a different weight.
    // As before cpbcTotal_ is a per-probe average cost.
    // totalCost_ is not being used
    const CostScalar etLR = cplr_.getElapsedTime(*pfg);

    CostScalar etLRPlusBlk;

    etLRPlusBlk = etLR + etBlock * cpbcTotal_.getNumProbes();
    et = ElapsedTime( etLRPlusBlk );
  }
  return et;

} // Cost::convertToElapsedTime()
//<pb>

// This method is to be used only for displaying total cost information
// in the context of explain, visual debugger, etc. This is NOT to be used
// for computing and/or comparing plan cost information. In the context of NCM,
// internal costs used during plan computation use very different units than
// the total cost displayed externally to the user, and so these cannot be
// used interchangably.
ElapsedTime Cost::displayTotalCost (const ReqdPhysicalProperty* const rpp) const
{
  if ((CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_OFF) ||
      (CmpCommon::getDefault(EXPLAIN_DISPLAY_FORMAT) == DF_INTERNAL))
  {
    return convertToElapsedTime(rpp);
  }
  else
  {
    double cpu, io, msg, idle, seqIOs, randIOs, total;
    Lng32 probes;
    getExternalCostAttr(cpu, io, msg, idle, seqIOs, randIOs, total, probes);
    return total;
  }
}


// -----------------------------------------------------------------------
// The cost detail description consists of the individual components of the
// simple cost vector depending on the cost model in effect. This is expected
// to be used by callers like Explain etc. to display the cost details.
// -----------------------------------------------------------------------
const NAString Cost::getDetailDesc() const
{
  NAString dtlDesc(CmpCommon::statementHeap());
  // Declare line so that we never have a string larger than this.
  char line[400];
  double cpu, io, msg, idle, seqIOs, randIOs, total;
  Lng32 probes;
  getExternalCostAttr(cpu, io, msg, idle, seqIOs, randIOs, total, probes);

  if ((CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_OFF) ||
      (CmpCommon::getDefault(EXPLAIN_DISPLAY_FORMAT) == DF_EXTERNAL))
  {
    sprintf(line,
            "CPU_TIME: %g IO_TIME: %g MSG_TIME: %g IDLE_TIME: %g PROBES: %d",
            cpu, io, msg, idle, probes);
  }
  else
  {
    DCMPASSERT(CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON);
    if (CmpCommon::getDefault(EXPLAIN_DISPLAY_FORMAT) == DF_EXTERNAL_DETAILED)
    {
      sprintf(line,
              "CPU_TIME: %g IO_TIME: %g MSG_TIME: %g IDLE_TIME: %g PROBES: %d IO_SEQ: %g IO_RAND: %g",
              cpu, io, msg, idle, probes, seqIOs, randIOs);
    }
    else
    { // DF_INTERNAL, print details in NCM internal format.
      double tcProc, tcProd, tcSent, ioRand, ioSeq;
      getScmCostAttr(tcProc, tcProd, tcSent, ioRand, ioSeq, probes);
      sprintf(line,
              "TC_PROC: %g TC_PROD: %g TC_SENT: %g IO_SEQ: %g IO_RAND: %g ",
              tcProc, tcProd, tcSent, ioSeq, ioRand);

      NABoolean scmDebugOn = (CmpCommon::getDefault(NCM_PRINT_ROWSIZE) == DF_ON);
      if (scmDebugOn == TRUE)
      {
        double input0RowSize, input1RowSize, outputRowSize, probeRowSize;
        char rowsizeInfo[400];
        getScmDebugAttr(input0RowSize, input1RowSize, outputRowSize, probeRowSize);
        sprintf(rowsizeInfo,
                " I0RS: %g I1RS: %g O_RS: %g P_RS: %g",
                input0RowSize, input1RowSize, outputRowSize, probeRowSize);

        // Does strcat truncate??
        strcat(line, rowsizeInfo);
      }
    }
  }
  dtlDesc = line;
  return dtlDesc;
} // Cost::getDetailDesc()
//<pb>

// -----------------------------------------------------------------------
// This method returns cost information to WMS (and possibly other callers).
// -----------------------------------------------------------------------
void Cost::getExternalCostAttr(double &cpuTime, double &ioTime,
                               double &msgTime, double &idleTime,
                               double &numSeqIOs, double &numRandIOs,
                               double &totalTime, Lng32 &probes) const
{
  // Depending on the cost model in effect get the detailed description for
  // the corresponding cost vector
  if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
  {
    double tcProc, tcProd, tcSent;
    getScmCostAttr(tcProc, tcProd, tcSent, numRandIOs, numSeqIOs, probes);

    const double cpu_ff    = CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();
    const double randIO_ff = CURRSTMT_OPTDEFAULTS->getTimePerSeek();
    const double seqIO_ff  = CURRSTMT_OPTDEFAULTS->getTimePerSeqKb();

    double randIOTime, seqIOTime;
    double blockSizeInKB = 32.0;  // better way to get this from CQD?

    // compute cpu resources: total rows * copy cost per row
    cpuTime =  (tcProc + tcProd) *
                CostPrimitives::getBasicCostFactor(CPUCOST_COPY_ROW_PER_BYTE);
    // convert resources to time
    cpuTime *= cpu_ff;

    // compute IO times
    randIOTime = randIO_ff * numRandIOs;
    seqIOTime = seqIO_ff * numSeqIOs * blockSizeInKB;

    // compute msg resources
    msgTime = tcSent *
              CostPrimitives::getBasicCostFactor(CPUCOST_EXCHANGE_COST_PER_BYTE);
    // convert msg resources to time
    msgTime *= cpu_ff;

    // final tuning if needed.
    double mapCpuFactor =
      ActiveSchemaDB()->getDefaults().getAsDouble(NCM_MAP_CPU_FACTOR);
    double mapRandIoFacor =
      ActiveSchemaDB()->getDefaults().getAsDouble(NCM_MAP_RANDIO_FACTOR);
    double seqIOFactor =
      ActiveSchemaDB()->getDefaults().getAsDouble(NCM_MAP_SEQIO_FACTOR);
    double mapMsgFactor =
      ActiveSchemaDB()->getDefaults().getAsDouble(NCM_MAP_MSG_FACTOR);

    cpuTime *= mapCpuFactor;
    randIOTime *= mapRandIoFacor;
    seqIOTime *= seqIOFactor;
    msgTime *= mapMsgFactor;

    ioTime = randIOTime + seqIOTime;

    totalTime = cpuTime + ioTime + msgTime;
    idleTime = 0;
  }
  else
  {
    getOcmCostAttr(cpuTime, ioTime, msgTime, idleTime, probes);
    totalTime = MINOF(convertToElapsedTime(), 1e32).getValue();
    numSeqIOs = numRandIOs = 0;
  }
} // Cost::getExternalCostAttr()

// -----------------------------------------------------------------------
// The cost detail description consists of the individual components of the
// simple cost vector depending on the cost model in effect. This is expected
// to be used by callers like Explain etc. to display the cost details.
// -----------------------------------------------------------------------
void Cost::getOcmCostAttr(double &cpu, double &io,
                          double &msg, double &idleTime,
                          Lng32 &probes) const
{
  cpu =   MINOF(cplr_.getCPUTime(), 1e32).getValue()
        + MINOF(cpbcTotal_.getCPUTime(), 1e32).getValue();

  io =    MINOF(cplr_.getIOTime(), 1e32).getValue()
       +  MINOF(cpbcTotal_.getIOTime(), 1e32).getValue();

  msg =   cplr_.getMessageTime().getValue()
        + cpbcTotal_.getMessageTime().getValue();

  idleTime = MAXOF( cplr_.getIdleTime().getValue(),
                    cpbcTotal_.getIdleTime().getValue());

  CostScalar tmpProbe = cplr_.getNumProbes();
  probes =  Lng32(tmpProbe.getValue());
}

// -----------------------------------------------------------------------
// The cost detail description consists of the individual components of the
// simple cost vector depending on the cost model in effect. This is expected
// to be used by callers like Explain etc. to display the cost details.
// -----------------------------------------------------------------------
// called only when internal debugging CQD NCM_PRINT_ROWSIZE is ON.
// So hide from coverage
void Cost::getScmCostAttr(double &tcProc, double &tcProd,
                          double &tcSent, double &ioRand,
                          double &ioSeq, Lng32 &probes) const
{
  tcProc = cpScmlr_.getTcProc().getValue();

  tcProd = cpScmlr_.getTcProd().getValue();

  tcSent = cpScmlr_.getTcSent().getValue();

  ioRand = cpScmlr_.getIoRand().getValue();

  ioSeq = cpScmlr_.getIoSeq().getValue();

  CostScalar tmpProbe = cpScmlr_.getNumProbes();
  probes = Lng32(tmpProbe.getValue());
}

// -----------------------------------------------------------------------
// The cost detail description consists of the individual components of the
// simple cost vector depending on the cost model in effect. This is expected
// to be used by callers like Explain etc. to display the cost details.
// -----------------------------------------------------------------------
void Cost::getScmDebugAttr(double &dbg1, double &dbg2,
			   double &dbg3, double &dbg4) const
{
  dbg1 = cpScmDbg_.getTcProc().getValue();

  dbg2 = cpScmDbg_.getTcProd().getValue();

  dbg3 = cpScmDbg_.getTcSent().getValue();

  dbg4 = cpScmDbg_.getIoRand().getValue();
}

//<pb>
// -----------------------------------------------------------------------
// Compares two Cost objects in the simple cost model
// returns MORE if this > other, SAME if they are equal, and LESS otherwise.
// rpp is unused for now, but may be used later if we add different goals to the simple cost model.
// -----------------------------------------------------------------------
COMPARE_RESULT Cost::scmCompareCosts(
                              const Cost & other,
                              const ReqdPhysicalProperty* const rpp) const
{
  // Plans with equal priorities proceed to real cost comparison.
  // We are optimizing based on the total cost represented by the cost objects.
  CostScalar cs1 = scmComputeTotalCost() * priority_.riskPremium();
  CostScalar cs2 = other.scmComputeTotalCost() * other.priority_.riskPremium();

  if (cs1 > cs2)
    return MORE;
  else if (cs1 < cs2)
    return LESS;
  else
    return SAME;
} // Cost::scmCompareCosts()

//<pb>
// -----------------------------------------------------------------------
// The total cost represented by the cpScmlr SimpleCostVector of "this" Cost object
// is the sum of tuples processed, tuples produced, tuples sent,
// and weighted sequential IOs and random IOs.
// rpp is unused for now.
// -----------------------------------------------------------------------
CostScalar Cost::scmComputeTotalCost(
                              const ReqdPhysicalProperty* const rpp) const
{
  return cpScmlr_.getElapsedTime();
} // Cost::scmComputeTotalCost()

//<pb>
//==============================================================================
//  Use this Cost object to produce a corresponding CostLimit object using
// specified required physical properties for elapsed time conversions.
//
// Input:
//  rpp  -- specified required physical properties.
//
// Output:
//  none
//
// Return:
//  Pointer to newly created CostLimit object.
//
//==============================================================================
CostLimit*
Cost::convertToCostLimit(const ReqdPhysicalProperty* const rpp) const
{

  // Fix for coverity cid #1511 : pointer pfg is not used.
  // const PerformanceGoal* pfg;
  // pfg = (rpp ? rpp->getPerformanceGoal() : DefaultPerformanceGoal);

  //----------------------------------------------------------------------
  // If optimized for resource consumption, the set of cost weights
  // specified in rpp are used. CostLimit is computed based on lr_ and blocking
  // cost for resource consumption as well.
  //----------------------------------------------------------------------
  /* if(pfg->isOptimizeForResourceConsumption())
  {
    const CostWeight* rcw;
    rcw = (rpp ? rpp->getCostWeight() : DefaultCostWeight);
    return rcw->convertToCostLimit(totalCost_);
  }*/

  //----------------------------------------------------------------------
  //  Else, we are optimizing for elapsed time (either FR or LR).  The new
  // CostLimit contains an upper limit equal to the elapsed time of this
  // cost object.  The CostLimit's accumulated ancestor and sibling costs
  // are initially set to be empty.
  //----------------------------------------------------------------------
  ElapsedTime et (convertToElapsedTime(rpp));
  Cost emptyAncestorCost;
  Cost emptySiblingCost;
  if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
  {
    return new(CmpCommon::statementHeap())
      ScmElapsedTimeCostLimit(et, getPlanPriority(),
                              &emptyAncestorCost, &emptySiblingCost);
  }
  else
  {
    return new(CmpCommon::statementHeap())
      ElapsedTimeCostLimit(et, getPlanPriority(),
                           &emptyAncestorCost, &emptySiblingCost);
  }

} // Cost::convertToCostLimit()
//<pb>

// -----------------------------------------------------------------------
// Some operators on all component vectors. Calls the same overloaded
// operation for all the vectors. Make sure you know what you are doing
// when you use them. Really doubtful whether they should be supported at
// all.
// -----------------------------------------------------------------------
Cost & Cost::operator += (const Cost &other)
{

  // ---------------------------------------------------------------------
  // Take special care when using this function in the new version Cost
  // object. This method simply adds the current process vectors up using
  // the vector addition provided by CostVector. When combining cost
  // objects, the contexts (such as the operators to which cost objects
  // belong) under which they are combined should be considered and we
  // should have operator specific implementations.
  // ---------------------------------------------------------------------
  totalCost_ += other.totalCost_;
  cplr_	     += other.cplr_;
  cpScmlr_   += other.cpScmlr_;
  cpfr_	     += other.cpfr_;
  cpbc1_     += other.cpbc1_;
  cpbcTotal_ += other.cpbcTotal_;
//jo  opfr_	     += other.opfr_;
//jo  oplr_	     += other.oplr_;
  priority_ += other.priority_;
  return *this;
}
//<pb>

//==============================================================================
//  Merge a sibling cost vector with this cost vector.
//
// Input:
//  otherChildCost  -- specified other child cost object.
//
// Output:
//  none
//
// Return:
//  This cost object after being merged with its sibling.
//
//==============================================================================
void
Cost::mergeOtherChildCost(const Cost& otherChildCost)
{

  // merge sibling cost for NCM
  if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
    cpScmlr_ += otherChildCost.cpScmlr_;
  else
  {
    //---------------------------------------------------------------------------
    //  Add corresponding vectors of a new style cost object using overlapped
    // addition (except for total cost which always uses simple vector addition).
    // Remember to normalize other blocking vector's to the number of probes
    // for this cost object's blocking vectors.
    //---------------------------------------------------------------------------

    totalCost_ += otherChildCost.totalCost_;

    cplr_.overlappedAdd(otherChildCost.cplr_);
    cpfr_.overlappedAdd(otherChildCost.cpfr_);

    CostScalar numProbes = cpbc1_.getNumProbes();
    cpbc1_.overlappedAdd(otherChildCost.cpbc1_.getNormalizedVersion(numProbes));
    numProbes = cpbcTotal_.getNumProbes();
    cpbcTotal_.overlappedAdd(
               otherChildCost.cpbcTotal_.getNormalizedVersion(numProbes));

    //jo  opfr_.overlappedAdd(otherChildCost.opfr_);
    //jo  oplr_.overlappedAdd(otherChildCost.oplr_);
  }

  priority_.combine(otherChildCost.priority_);

} // Cost::mergeOtherChildCost
//<pb>

//<pb>
//==============================================================================
//  Default implementation to produce a priority for an entire subtree
//
// Output:
//  Plan Priority
//
// Input:
//  op         -- specified physical operator.
//
//  myContext  -- context associated with specified physical operator
//
//  childCost  -- cost of operator child subtree
//========================================================================

// For use with leaf operators
PlanPriority
Cost::computePlanPriority( RelExpr* op,
                           const Context* myContext)
{
  priority_ = op->computeOperatorPriority(myContext);
  return priority_;
}

// For use with unary operators
PlanPriority
Cost::computePlanPriority( RelExpr* op,
                           const Context* myContext,
                           const Cost* childCost)
{

  priority_ = op->computeOperatorPriority(myContext);
  priority_.rollUpUnary(childCost->getPlanPriority());
  return priority_;
}

// For use with binary operators
PlanPriority
Cost::computePlanPriority( RelExpr* op,
                           const Context* myContext,
                           const Cost* child0Cost,
                           const Cost* child1Cost,
                           PlanWorkSpace *pws,
                           Lng32 planNumber)
{

  priority_ = op->computeOperatorPriority(myContext, pws, planNumber);
  priority_.rollUpBinary(child0Cost->getPlanPriority(),child1Cost->getPlanPriority());
  return priority_;
}


// excluded for coverage because it's a debug code
void Cost::print(FILE * pfp, const char * , const char *) const
{
  if (CmpCommon::getDefault(SIMPLE_COST_MODEL) == DF_ON)
  {
    fprintf(pfp,"SCM Last Row Cost information\n");
    cpScmlr_.print(pfp);
  }
  else
  {
    fprintf(pfp,"First Row Cost information\n");
    cpfr_.print(pfp);
    fprintf(pfp,"Last Row Cost information\n");
    cplr_.print(pfp);
    fprintf(pfp,"Blocking Cost information\n");
    cpbc1_.print(pfp);
  }
  fprintf(pfp,"preference : %d\n", priority_.getLevel()); //sathya
  return;
}

void Cost::display() const                                        
{ 
  print(); 
}


//<pb>
//==============================================================================
//  HashJoinCost Constructor.
//
// Input:
//  currentProcessFirstRowCost -- resources necessary to produce operator's
//                                 first row once blocking activity completes.
//
//  currentProcessLastRowCost  -- resources necessary to produce operator's
//                                 last row once blocking activity completes.
//
//  currentProcessBlockingCost -- resources necessary for blocking activity.
//
//  countOfCPUs                -- number of CPUs for executing this hash join.
//
//  planFragmentsPerCPU        -- number of plan fragments per CPU.
//
//  stage2WorkFractionForFR -- percentage of stage 2 work necessary to
//                                 produce hash join's first row.
//
//  stage3WorkFractionForFR -- percentage of stage 3 work necessary to
//                                 produce hash join's first row.
//
//  stage2Cost              -- resources necessary for stage 2 activity.
//
//  stage3Cost              -- resources necessary for stage 3 activity.
//
//
// Output:
//  none
//
// Return:
//  none.
//
//==============================================================================
HashJoinCost::HashJoinCost(const SimpleCostVector* currentProcessFirstRowCost,
                           const SimpleCostVector* currentProcessLastRowCost,
                           const SimpleCostVector* currentProcessBlockingCost,
                           const Lng32              countOfCPUs,
                           const Lng32              planFragmentsPerCPU,
                           const CostScalar &      stage2WorkFractionForFR,
                           const CostScalar &      stage3WorkFractionForFR,
                           const SimpleCostVector* stage2Cost,
                           const SimpleCostVector* stage3Cost,
                           const SimpleCostVector* stage1BkCost,
                           const SimpleCostVector* stage2BkCost,
                           const SimpleCostVector* stage3BkCost)

: Cost(currentProcessFirstRowCost,
       currentProcessLastRowCost,
       currentProcessBlockingCost,
       countOfCPUs,
       planFragmentsPerCPU),
  stage2WorkFractionForFR_(stage2WorkFractionForFR),
  stage3WorkFractionForFR_(stage3WorkFractionForFR),
  stage2Cost_(*stage2Cost),
  stage3Cost_(*stage3Cost),
  stage1BkCost_(),
  stage2BkCost_(),
  stage3BkCost_()
{

  if (stage1BkCost != NULL)
    stage1BkCost_ = *stage1BkCost;

  if (stage2BkCost != NULL)
    stage2BkCost_ = *stage2BkCost;

  if (stage3BkCost != NULL)
    stage3BkCost_ = *stage3BkCost;

  //----------------------------------------------------------------------------
  //  Ensure that both stage two and stage three work fractions for first row
  // are between zero and 1.  Also, ensure that the stage two work fraction
  // for first row is greater than or equal to the stage 3 work fraction for
  // first row.
  //----------------------------------------------------------------------------
  CMPASSERT( stage2WorkFractionForFR.isGreaterOrEqualThanZero() /* >= csZero */ );
  CMPASSERT( NOT stage2WorkFractionForFR.isGreaterThanOne() /* <= csOne */ );
  CMPASSERT( stage3WorkFractionForFR.isGreaterOrEqualThanZero() /* >= csZero */ );
  CMPASSERT( stage2WorkFractionForFR >= stage3WorkFractionForFR );

  //-------------------------------------------------------------------------
  //  Since cost vectors supplied to this constructor are on a "per stream"
  // basis, we convert them to a "per CPU" basis.  We use overlapped addition
  // because independent streams can overlap IO and message costs.
  //-------------------------------------------------------------------------
  for (Lng32 fragIdx = 1; fragIdx < planFragmentsPerCPU; fragIdx++)
    {
      stage2Cost_.overlappedAdd(*stage2Cost);
      stage3Cost_.overlappedAdd(*stage3Cost);

      if (stage1BkCost != NULL)
        stage1BkCost_.overlappedAdd(*stage1BkCost);

      if (stage2BkCost != NULL)
        stage2BkCost_.overlappedAdd(*stage2BkCost);

      if (stage3BkCost != NULL)
        stage3BkCost_.overlappedAdd(*stage3BkCost);
    }

} // HashJoinCost constructor.
//<pb>
//==============================================================================
//  Effectively a virtual constructor for a hash join cost object.
//
// Input:
//  none
//
// Output:
//  none
//
// Return:
//  none.
//
//==============================================================================
Cost*
HashJoinCost::duplicate()
{

  Cost* costPtr = new (CmpCommon::statementHeap()) HashJoinCost(*this);
  return costPtr;

} // HashJoinCost::duplicate()
//<pb>
//==============================================================================
//  HashJoinCost destructor.
//
// Input:
//  none
//
// Output:
//  none
//
// Return:
//  none.
//
//==============================================================================
HashJoinCost::~HashJoinCost()
{
} // HashJoinCost destructor.
//<pb>
//==============================================================================
//  HashGroupByCost Constructor.
//
// Input:
//  currentProcessFirstRowCost -- resources necessary to produce operator's
//                                 first row once blocking activity completes.
//
//  currentProcessLastRowCost  -- resources necessary to produce operator's
//                                 last row once blocking activity completes.
//
//  currentProcessBlockingCost -- resources necessary for blocking activity.
//
//  countOfCPUs                -- number of CPUs for executing this hash join.
//
//  planFragmentsPerCPU        -- number of plan fragments per CPU.
//
//  groupingFactor             -- percentage of groups which fit in memory.
//
// Output:
//  none
//
// Return:
//  none.
//
//==============================================================================
HashGroupByCost::HashGroupByCost(
                             const SimpleCostVector* currentProcessFirstRowCost,
                             const SimpleCostVector* currentProcessLastRowCost,
                             const SimpleCostVector* currentProcessBlockingCost,
                             const Lng32              countOfCPUs,
                             const Lng32              planFragmentsPerCPU,
                             const CostScalar &      groupingFactor)

: Cost(currentProcessFirstRowCost,
       currentProcessLastRowCost,
       currentProcessBlockingCost,
       countOfCPUs,
       planFragmentsPerCPU),
  groupingFactor_(groupingFactor)
{

  //--------------------------------------------------------------
  //  Ensure that grouping factor is between zero and 1 inclusive.
  //--------------------------------------------------------------
  CMPASSERT( groupingFactor.isGreaterOrEqualThanZero() /* >= csZero */ );
  CMPASSERT( NOT groupingFactor.isGreaterThanOne() /* <= csOne */ );

} // HashGroupByCost constructor.
//<pb>
//==============================================================================
//  Effectively a virtual constructor for a Hash GroupBy cost object.
//
// Input:
//  none
//
// Output:
//  none
//
// Return:
//  none.
//
//==============================================================================
Cost*
HashGroupByCost::duplicate()
{

  Cost* costPtr = new (CmpCommon::statementHeap()) HashGroupByCost(*this);
  return costPtr;

} // HashGroupByCost::duplicate()
//<pb>
//==============================================================================
//  HashGroupByCost destructor.
//
// Input:
//  none
//
// Output:
//  none
//
// Return:
//  none.
//
//==============================================================================
HashGroupByCost::~HashGroupByCost()
{
} // HashGroupByCost destructor.
//<pb>
// ***********************************************************************

// -----------------------------------------------------------------------
// methods for class CostWeight
// -----------------------------------------------------------------------
ResourceConsumptionWeight* CostWeight::castToResourceConsumptionWeight() const
{
  return NULL; // sorry, wrong number
}
//<pb>

// ***********************************************************************

// -----------------------------------------------------------------------
// methods for class ResourceConsumptionWeight
// -----------------------------------------------------------------------

// the old constructor is kept for reference
/*ResourceConsumptionWeight::ResourceConsumptionWeight(
                        const CostScalar & cpuWeight,
			const CostScalar & numSeeksWeight,
			const CostScalar & numKBytesWeight,
			const CostScalar & normalMemoryWeight,
			const CostScalar & persistentMemoryWeight,
			const CostScalar & numLocalMessagesWeight,
			const CostScalar & kbLocalMessagesWeight,
			const CostScalar & numRemoteMessagesWeight,
			const CostScalar & kbRemoteMessagesWeight,
			const CostScalar & diskUsageWeight,
			const CostScalar & idleTimeWeight,
			const CostScalar & numProbesWeight)

{
  weighFactors_[CPU_TIME] = cpuWeight;

  weighFactors_[IO_TIME]  = MAXOF(numSeeksWeight, numKBytesWeight);

  weighFactors_[MSG_TIME] =
		    MAXOF(
		      MAXOF(numLocalMessagesWeight, kbLocalMessagesWeight),
		      MAXOF(numRemoteMessagesWeight, kbRemoteMessagesWeight)
		    );

  weighFactors_[IDLE_TIME]           = idleTimeWeight;
  weighFactors_[TC_PROC] = csZero;
  weighFactors_[TC_PROD] = csZero;
  weighFactors_[TC_SENT] = csZero;
  weighFactors_[IO_RAND] = csZero;
  weighFactors_[IO_SEQ] = csZero;
  weighFactors_[NUM_PROBES]          = numProbesWeight;
} // ResourceConsumptionWeight::ResourceConsumptionWeight
*/

ResourceConsumptionWeight::ResourceConsumptionWeight(
                        const CostScalar & cpuWeight,
			const CostScalar & IOWeight,
			const CostScalar & MsgWeight,
			const CostScalar & idleTimeWeight,
			const CostScalar & numProbesWeight)

{
  weighFactors_[CPU_TIME] = cpuWeight;

  weighFactors_[IO_TIME]  = IOWeight;

  weighFactors_[MSG_TIME] = MsgWeight;

  weighFactors_[IDLE_TIME] = idleTimeWeight;

  weighFactors_[TC_PROC] = csZero;

  weighFactors_[TC_PROD] = csZero;

  weighFactors_[TC_SENT] = csZero;

  weighFactors_[IO_RAND] = csZero;

  weighFactors_[IO_SEQ] = csZero;

  weighFactors_[NUM_PROBES]  = numProbesWeight;


} // ResourceConsumptionWeight::ResourceConsumptionWeight
//<pb>

// -----------------------------------------------------------------------
// Type-safe pointer cast
// -----------------------------------------------------------------------
ResourceConsumptionWeight*
ResourceConsumptionWeight::castToResourceConsumptionWeight() const
{
  return (ResourceConsumptionWeight*)this;
}

// -----------------------------------------------------------------------
//  ResourceConsumptionWeight::entries()
// -----------------------------------------------------------------------
Lng32 ResourceConsumptionWeight::entries() const
{
  return COUNT_OF_SIMPLE_COST_COUNTERS;
} // ResourceConsumptionWeight::entries()

// -----------------------------------------------------------------------
//  ResourceConsumptionWeight::convertToElapsedTime()
// -----------------------------------------------------------------------
ElapsedTime ResourceConsumptionWeight::convertToElapsedTime
                                      (const CostVector& cv) const
{
  CMPASSERT(cv.entries() == entries());

  ElapsedTime result( csZero );

  for (Lng32 i = 0; i < entries(); i++)
    result += cv[i] * weighFactors_[i];

  return result;
}; // ResourceConsumptionWeight::convertToElapsedTime()

// -----------------------------------------------------------------------
//  ResourceConsumptionWeight::convertToFloatingPointValue()
// -----------------------------------------------------------------------
double ResourceConsumptionWeight::convertToFloatingPointValue
                                 (const CostVector& cv) const
{
  return convertToElapsedTime(cv).value();
}; // ResourceConsumptionWeight::convertToFloatingPointValue()
//<pb>
//==============================================================================
//  Use this ResourceConsumptionWeight object to produce a corresponding
// CostLimit object using a specified cost vector for elapsed time calculations.
//
// Input:
//  cv  -- specified Cost vector.
//
// Output:
//  none
//
// Return:
//  Pointer to newly created CostLimit object.
//
//==============================================================================
CostLimit*
ResourceConsumptionWeight::convertToCostLimit (const CostVector& cv) const
{

  //----------------------------------------------------------------------
  //  The new CostLimit contains an upper limit equal to the elapsed time
  // of the specified cost vector.  The CostLimit's accumulated ancestor
  // and sibling costs are initially set to be empty.
  //----------------------------------------------------------------------
  Cost emptyAncestorCost;
  Cost emptySiblingCost;
  PlanPriority* normalPriority = new(CmpCommon::statementHeap()) PlanPriority();
  // coverity flags this as resource leak, since we cleanup
  // statement heap later we fix it here using annotations.
  // coverity[leaked_storage]
  return new(CmpCommon::statementHeap())
               ElapsedTimeCostLimit(convertToElapsedTime(cv),
                                    *normalPriority,
                                    &emptyAncestorCost,
                                    &emptySiblingCost);

}; // ResourceConsumptionWeight::convertToCostLimit()
//<pb>
// -----------------------------------------------------------------------
// ResourceConsumptionWeight::compareCostVectors()
// -----------------------------------------------------------------------
COMPARE_RESULT ResourceConsumptionWeight::compareCostVectors
                                         (const CostVector& cv1,
			                  const CostVector& cv2) const
{
  ElapsedTime abs1(convertToElapsedTime(cv1));
  ElapsedTime abs2(convertToElapsedTime(cv2));

  if ( abs1 == abs2 )
    return SAME;
  else if ( abs1 < abs2 )
    return LESS;
  else
    return MORE;

}; // ResourceConsumptionWeight::compareCostVectors()
//<pb>

// Provides access to a specific entry
CostScalar ResourceConsumptionWeight::operator[] (Lng32 ix) const
{
  CMPASSERT((ix >= 0) AND (ix < COUNT_OF_SIMPLE_COST_COUNTERS));
  return weighFactors_[ix];
}

// ***********************************************************************

// -----------------------------------------------------------------------
// methods for class CostLimit
// -----------------------------------------------------------------------
ElapsedTimeCostLimit* CostLimit::castToElapsedTimeCostLimit() const
{
  return NULL; // sorry, wrong number
}
//<pb>

// ***********************************************************************

// -----------------------------------------------------------------------
// methods for class ElpasedTimeCostLimit
// -----------------------------------------------------------------------
ElapsedTimeCostLimit* ElapsedTimeCostLimit::castToElapsedTimeCostLimit() const
{
  return (ElapsedTimeCostLimit*)this;
}
//<pb>
//==============================================================================
//  Convert this cost limit to a single value which represents the elapsed time
// remaining after the elapsed time of the associated Cost is subtracted from
// the initial elapsed time limit.  Use specified required physical properties
// for elapsed time conversion.
//
// Input:
//  rpp   -- specified required physical properties.
//
// Output:
//  none
//
// Return:
//  remaining elapsed time in the form of a double.
//
//==============================================================================
double
ElapsedTimeCostLimit::getValue(const ReqdPhysicalProperty* const rpp) const
{
  //-------------------------------------------------------------------------
  //  Roll up accumulated sibling cost into accumulated ancestor cost using
  // the most conservative (i.e. non-blocking) unary roll-up strategy.
  //-------------------------------------------------------------------------
  Cost* rollUpCost   = rollUpUnaryNonBlocking(*ancestorCost_,
                                              *otherKinCost_,
                                              rpp);
  double newLimitVal = rollUpCost->convertToElapsedTime(rpp).value();
  delete rollUpCost;

  //--------------------------------------------------------------------
  //  Remaining elapsed time is upper limit less the calculated limit of
  // ancestors and siblings.
  //--------------------------------------------------------------------
  return upperLimit_.value() - newLimitVal;

} // ElapsedTimeCostLimit::getValue

//<pb>
//==============================================================================
//   Compare this cost limit with a specified cost limit using specified
// required physical properties for any elapsed time conversions.  Comparison
// is based on associated value of each cost limit.
//
// Input:
//  other -- specified cost limit with which to compare.
//
//  rpp   -- specified required physical properties.
//
// Output:
//  none
//
// Return:
//  Result of comparison.
//
//==============================================================================
COMPARE_RESULT
ElapsedTimeCostLimit::compareCostLimits
                             (CostLimit* other,
                              const ReqdPhysicalProperty* const rpp)
{
  // other == NULL means infinite time, thus return LESS in this case:
  if (other==NULL)
  {
    return LESS;
  }
  else
  {
    CMPASSERT(other->castToElapsedTimeCostLimit());

    // Higher priority plans always win the comparison and considered "cheaper"
    // Similarly a costLimit with higher priority limit should be considered
    // a stronger (i.e. lower) limit than one with low priority
    if (CmpCommon::getDefault(COMP_BOOL_193) == DF_OFF)
    {
      if (priorityLimit_ > other->priorityLimit())
        return LESS;
      if (priorityLimit_ < other->priorityLimit())
        return MORE;
    }

    // try to use cached values if possible
    double otherValue = other->getCachedValue();
    if ( otherValue == 0.0 OR
       NOT CURRSTMT_OPTDEFAULTS->OPHuseCachedElapsedTime())
    {
      // This shouldn't happen, just in case.
      otherValue = other->getValue(rpp) *
        other->priorityLimit().riskPremium().value();
      other->setCachedValue(otherValue);
    }

    if ( cachedValue_ == 0.0 OR
       NOT CURRSTMT_OPTDEFAULTS->OPHuseCachedElapsedTime())
    {
      // This may happen if the current cost limit gets modified
      cachedValue_ = getValue(rpp) * priorityLimit().riskPremium().value();
    }

    if (cachedValue_ > otherValue)
      {
        return MORE;
      }
    else
      {
        if (cachedValue_ < otherValue)
          {
            return LESS;
          }
        else
          {
            return SAME;
          }
      }
  } // other != NULL

} // ElapsedTimeCostLimit::compareCostLimits()
//<pb>
//==============================================================================
//  Compare this CostLimit with a Cost object by first converting the Cost
// object to a CostLimit and then comparing the two CostLimit objects using
// specified required physical properties for elapsed time calculations.
//
// Input:
//  other -- cost object with which to compare.
//
//  rpp   -- specified required physical properties.
//
// Output:
//  none
//
// Return:
//  Result of comparison.
//
//==============================================================================
COMPARE_RESULT
ElapsedTimeCostLimit::compareWithCost (const Cost & other,
	                               const ReqdPhysicalProperty* const rpp)
{


  // Higher priority plans always win the comparison and considered "cheaper"
  // Similarly a costLimit with higher priority limit should be considered
  // a stronger (i.e. lower) limit than one with low priority
  if (CmpCommon::getDefault(COMP_BOOL_193) == DF_OFF)
  {
    // xxx We only consider the demotion levels of priority limit for this type
    // of cost based pruning. This is because demotions levels are monotonically
    // decreasing with upper limit of zero.
    Lng32 demotionLimit = priorityLimit_.getDemotionLevel()
                         - ancestorCost_->getPlanPriority().getDemotionLevel()
                         - otherKinCost_->getPlanPriority().getDemotionLevel();

    if (demotionLimit > other.getPlanPriority().getDemotionLevel())
      return LESS;
    if (demotionLimit < other.getPlanPriority().getDemotionLevel())
      return MORE;

    /*
    PlanPriority tmpPriorityLimit =
      priorityLimit_ - (ancestorCost_->getPlanPriority() + otherKinCost_->getPlanPriority());
    if (tmpPriorityLimit > other.getPlanPriority())
      return LESS;
    if (tmpPriorityLimit < other.getPlanPriority())
      return MORE;
    */
  }

  // Trying to be less aggressive. First, roll-up other cost into cost limit
  // then call getValue() if result is negative - return LESS and so on.
  // for conservative CL use CQD OPH_USE_CONSERVATIVE_COST_LIMIT 'ON';
  // Note that here we cannot use cachedValue.
  if ( CURRSTMT_OPTDEFAULTS->OPHuseConservativeCL() )
  {
    CostLimit * costLimit = copy();
    costLimit->ancestorAccum(other, rpp);
    double diff = costLimit->getValue(rpp);
    // do we want to delete temporary costLimit object here?
    // delete costLimit;
    if ( diff < -0.000001 )
      return LESS;
    else if ( diff > 0.000001 )
      return MORE;
    else return SAME;
  }

  //---------------------------------------------------------------------------
  // Determine remaining time of cost limit and elapsed time of specified cost.
  //---------------------------------------------------------------------------

  if (cachedValue_ == 0.0 OR
      NOT CURRSTMT_OPTDEFAULTS->OPHuseCachedElapsedTime())
    cachedValue_ = getValue(rpp) * priorityLimit().riskPremium().value();
  double costValue = other.convertToElapsedTime(rpp).value() *
    ((Cost&)other).planPriority().riskPremium().value();

  if ( CURRSTMT_OPTDEFAULTS->OPHuseCompCostThreshold() )
  {
	  double ratio = costValue/cachedValue_;
	  if ( ratio < 0.999999 )
		  return MORE;
	  else if (ratio > 1.000001)
		  return LESS;
	  else
		  return SAME;
  }
  //---------------------------------------------------------------------------
  //  Compare remaining time of cost limit with elapsed time of specified cost.
  //---------------------------------------------------------------------------
  if (cachedValue_ == costValue )
    {
      return SAME;
    }
  else
    {
      if (cachedValue_ < costValue )
        {
          return LESS;
        }
      else
        {
          return MORE;
        }
    }

}

COMPARE_RESULT
ElapsedTimeCostLimit::compareWithPlanCost (CascadesPlan* plan,
	                               const ReqdPhysicalProperty* const rpp)
{
  // Cannot use cached elapsed time if
  // CQD OPH_USE_CONSERVATIVE_COST_LIMIT 'ON' need to roll up plan cost
  // into cost limit in this case
  if ( CURRSTMT_OPTDEFAULTS->OPHuseConservativeCL() )
    return compareWithCost(*plan->getRollUpCost(),rpp);

  // Higher priority plans always win the comparison and considered "cheaper"
  // Similarly a costLimit with higher priority limit should be considered
  // a stronger (i.e. lower) limit than one with low priority
  if (CmpCommon::getDefault(COMP_BOOL_193) == DF_OFF)
  {
    // xxx We only consider the demotion levels of priority limit for this type
    // of cost based pruning. This is because demotions levels are monotonically
    // decreasing with upper limit of zero.
    Lng32 demotionLimit = priorityLimit_.getDemotionLevel()
                         - ancestorCost_->getPlanPriority().getDemotionLevel()
                         - otherKinCost_->getPlanPriority().getDemotionLevel();

    if (demotionLimit > plan->getRollUpCost()->getPlanPriority().getDemotionLevel())
      return LESS;
    if (demotionLimit < plan->getRollUpCost()->getPlanPriority().getDemotionLevel())
      return MORE;

    /*
    PlanPriority tmpPriorityLimit =
      priorityLimit_ - (ancestorCost_->getPlanPriority() + otherKinCost_->getPlanPriority());
    if (tmpPriorityLimit > plan->getRollUpCost()->getPlanPriority())
      return LESS;
    if (tmpPriorityLimit < plan->getRollUpCost()->getPlanPriority())
      return MORE;
    */
  }

  ElapsedTime planElapsedTime = plan->getPlanElapsedTime();
  if ( planElapsedTime.isZero() OR
       NOT CURRSTMT_OPTDEFAULTS->OPHuseCachedElapsedTime())
  {
    // Using this plan for comparison for the first time. Compute
    // plan elapsed time and save in plan
    planElapsedTime = plan->getRollUpCost()->convertToElapsedTime(rpp);
    plan->setPlanElapsedTime(planElapsedTime);
  }

  // check and if necessary compute cachedValue
  if ( cachedValue_ == 0.0 OR
       NOT CURRSTMT_OPTDEFAULTS->OPHuseCachedElapsedTime())
    cachedValue_ = getValue(rpp);

  //---------------------------------------------------------------------------
  // Determine remaining time of cost limit and elapsed time of specified cost.
  //---------------------------------------------------------------------------
  double  costValue      = planElapsedTime.value();

  if ( CURRSTMT_OPTDEFAULTS->OPHuseCompCostThreshold() )
  {
	  double ratio = costValue/cachedValue_;
	  if ( ratio < 0.999999 )
		  return MORE;
	  else if (ratio > 1.000001)
		  return LESS;
	  else
		  return SAME;
  }
  //---------------------------------------------------------------------------
  //  Compare remaining time of cost limit with elapsed time of specified cost.
  //---------------------------------------------------------------------------
  if (cachedValue_ == costValue )
    {
      return SAME;
    }
  else
    {
      if (cachedValue_ < costValue )
        {
          return LESS;
        }
      else
        {
          return MORE;
        }
    }

}
//<pb>

//<pb>
//==============================================================================
//  Accumulate a specified cost into this cost limit's ancestor costs using
// specified required physical properties.
//
// Input:
//  otherCost -- specified Cost object.
//
//  rpp       -- specified required physical properties.
//
// Output:
//  none
//
// Return:
//  none
//
//==============================================================================
void
ElapsedTimeCostLimit::ancestorAccum(const Cost& otherCost,
                                    const ReqdPhysicalProperty* const rpp)
{

  //---------------------------------------------------
  //  Extract number of probes for later normalization.
  //---------------------------------------------------
  const CostScalar ancestorProbes = ancestorCost_->getCplr().getNumProbes();
  const CostScalar otherNumProbes = otherCost.getCplr().getNumProbes();

  //--------------------------------------------------------------------------
  //  This temporary cost object will ultimately contain the newly accumulated
  // cost.  Create it initially empty.
  //--------------------------------------------------------------------------
  Cost* tempCost = new (CmpCommon::statementHeap()) Cost();
//<pb>
  //---------------------------------------------------------------------------
  //  Accumulation follows the same basic strategy as cost roll-up.  We can
  // encounter four cases:
  //
  // 1) Neither specified cost nor cost accumulated so far is blocking.
  // 2) Specified cost is non-blocking but cost accumulated so far is blocking.
  // 3) Specified cost is blocking but cost accumulated so far is non-blocking.
  // 4) Both specified cost and cost accumulated so far are blocking.
  //---------------------------------------------------------------------------
  if ( otherCost.getCpbcTotal().isZeroVectorWithProbes() )
    {
      if ( ancestorCost_->getCpbcTotal().isZeroVectorWithProbes() )
        {

          //--------------------------------------------------------------------
          // Case 1.
          //
          //  Since neither cost is blocking, we need not concern ourselves with
          // blocking vectors.  The first row and last row vectors are added as
          // in non-blocking unary roll-up.
          //--------------------------------------------------------------------
          tempCost->cpfr() = blockingAdd( ancestorCost_->getCpfr(),
                                          otherCost.getCpfr(),
                                          rpp                      );
          tempCost->cplr() = overlapAdd(  ancestorCost_->getCplr(),
                                          otherCost.getCplr()
                                          - otherCost.getCpfr()
                                         )
                              + otherCost.getCpfr();
        }
      else
        {
//<pb>
          //--------------------------------------------------------------------
          // Case 2.
          //
          //  Since the accumulated cost is blocking, its first and last row
          // vectors do not change.  The specified cost's last row vector gets
          // added (after being converted to an average cost) to the accumulated
          // cost's blocking vectors using overlapped addition similar to
          // blocking unary roll-up.
          //--------------------------------------------------------------------
          tempCost->cpfr()      = ancestorCost_->getCpfr();
          tempCost->cplr()      = ancestorCost_->getCplr();
          tempCost->cpbc1()     =
                              overlapAdd( ancestorCost_->getCpbc1(),
                                          otherCost.getCplr() / otherNumProbes);
          tempCost->cpbcTotal() =
                              overlapAdd( ancestorCost_->getCpbcTotal(),
                                          otherCost.getCplr() / otherNumProbes);
        }
    }
  else
    {
      if ( ancestorCost_->getCpbcTotal().isZeroVectorWithProbes() )
        {

          //-------------------------------------------------------------------
          // Case 3.
          //
          //   Since accumulated cost is non-blocking, we simply add first and
          // last row cost vectors as in case 1 above, and the blocking vectors
          // come directly from the specified cost object (which is blocking).
          // These blocking vectors are normalized to the number of probes of
          // the accumulated cost.
          //-------------------------------------------------------------------
          tempCost->cpfr()      = blockingAdd( ancestorCost_->getCpfr(),
                                               otherCost.getCpfr(),
                                               rpp                      );
          tempCost->cplr()      = overlapAdd( ancestorCost_->getCplr(),
                                              otherCost.getCplr()
                                               - otherCost.getCpfr() )
                                   + otherCost.getCpfr();
          tempCost->cpbc1()     =
                  otherCost.getCpbc1().getNormalizedVersion(ancestorProbes);
          tempCost->cpbcTotal() =
                  otherCost.getCpbcTotal().getNormalizedVersion(ancestorProbes);
        }
      else
        {
//<pb>
          //------------------------------------------------------------------
          // Case 4.
          //
          //  Since the accumulated cost is blocking, the first and last row
          // cost vectors come directly from the accumulated cost object as in
          // case 2 above.
          //
          //  Since the specified cost is blocking, the first blocking
          // cost vector comes directly from the specified cost.  It is
          // normalized to the number of probes of the accumulated cost.
          //
          //  The final total blocking cost vector needs to include the total
          // blocking cost of both the specified cost and the accumulated cost
          // as well as the last row cost vector (converted to an average cost
          // vector) of the specified cost.  The total blocking vector of the
          // specified cost must be converted to the number of probes of the
          // accumulated cost.
          //------------------------------------------------------------------
          tempCost->cpfr()      = ancestorCost_->getCpfr();
          tempCost->cplr()      = ancestorCost_->getCplr();
          tempCost->cpbc1()     =
                      otherCost.getCpbc1().getNormalizedVersion(ancestorProbes);
          tempCost->cpbcTotal() =
            blockingAdd(
                  overlapAdd(ancestorCost_->getCpbcTotal(),
                             otherCost.getCplr() / otherNumProbes),
                  otherCost.getCpbcTotal().getNormalizedVersion(ancestorProbes),
                               rpp
                       );
        }
    }

  //---------------------------------------------------------------------------
  //  As in standard roll-up, total cost accumulation always uses simple vector
  // addition.
  //---------------------------------------------------------------------------
  tempCost->totalCost() = ancestorCost_->getTotalCost() + otherCost.getTotalCost();


  //---------------------------------------------------------------------------
  //  As in standard roll-up, priority will be combined. I rather use the mmethod
  //  combine() here over the + operator, but its OK now since they do the same thing
  //  and we have no reverse method for combine()
  //---------------------------------------------------------------------------
  tempCost->planPriority() = ancestorCost_->getPlanPriority() + otherCost.getPlanPriority();

  //-------------------------------------------------------------------------
  //  Delete old accumulated cost and replace it with newly accumulated cost.
  //-------------------------------------------------------------------------
  delete ancestorCost_;

  ancestorCost_ = tempCost;

  // invalidate cachedValue;
  cachedValue_ = 0.0;

} //ElapsedTimeCostLimit::ancestorAccum()
//<pb>
//==============================================================================
//  Accumulate a specified cost into this cost limit's other kin cost.
//
// Input:
//  otherCost -- specified Cost object.
//
// Output:
//  none
//
// Return:
//  none
//
//==============================================================================
void
ElapsedTimeCostLimit::otherKinAccum(const Cost& otherCost)
{

  otherKinCost_->mergeOtherChildCost(otherCost);
  // Note: no need to accumilate priority of otherKins here since
  // the mergeOtherChildCost method does that already

  // invalidate cachedValue;
  cachedValue_ = 0.0;

} // ElapsedTimeCostLimit::otherKinAccum()
//<pb>
//==============================================================================
//  Potentially reduce upper limit of this CostLimit based on the cost of a
// more promising plan.  Use specified required physical properties for
// elapsed time conversions.
//
// Input:
//  otherCost -- specified Cost object associated with more promising plan.
//
//  rpp       -- specified required physical properties.
//
// Output:
//  none
//
// Return:
//  none
//
//==============================================================================
void
ElapsedTimeCostLimit::tryToReduce(const Cost& otherCost,
                                  const ReqdPhysicalProperty* const rpp)
{

  //-------------------------------------------------------------------------
  // Calculate new potential priority limit as the sum of the priorities of
  // the new plan and the siblings and ancestors.
  //-------------------------------------------------------------------------
  NABoolean priorityLimitIncreased = FALSE;
  PlanPriority newPriorityLimit =
    otherCost.getPlanPriority() +
    ancestorCost_->getPlanPriority() +
    otherKinCost_->getPlanPriority();
  if (newPriorityLimit > priorityLimit_ AND // we have higher priority option (winner)
      CmpCommon::getDefault(COMP_BOOL_193) == DF_OFF)
  {
    priorityLimit_ = newPriorityLimit;
    priorityLimitIncreased = TRUE;
  }

  //-------------------------------------------------------------------------
  //  Calculate new potential upper limit as the sum of the elapsed time of
  // the cost of the new plan and the elapsed time of the combined costs of
  // siblings and ancestors accumulated so far.
  //-------------------------------------------------------------------------
  double newLimitVal =   otherCost.convertToElapsedTime(rpp).value()
                       + upperLimit_.value()
                       - getValue(rpp);

  //------------------------------------------------------------------------
  //  If newly calculated limit is smaller than old upper limit, make it the
  // official upper limit.
  //------------------------------------------------------------------------
  if (newLimitVal < upperLimit_.value() OR
      priorityLimitIncreased) // need to compute upperLimit based on new winner
    {
      upperLimit_ = ElapsedTime(newLimitVal);
    }

  // invalidate cachedValue;
  cachedValue_ = 0.0;

} //ElapsedTimeCostLimit::tryToReduce()
//<pb>
//==============================================================================
//  Unilaterally reduce upper limit by a specified elapsed time without going
// below zero.
//
// Input:
//  timeReduction -- specified elapsed time reduction.
//
// Output:
//  none
//
// Return:
//  none
//
//==============================================================================
void
ElapsedTimeCostLimit::unilaterallyReduce(const ElapsedTime & timeReduction)
{
  // This method is used for changing elapsed time limit for child of blocking
  // binary operators. It should not change priority limit

  upperLimit_ -= timeReduction;
  //if ( upperLimit_.getValue() < csZero.getValue() )
  //  {
  //    upperLimit_ = csZero;
  //  }
  upperLimit_.minCsZero();

  // invalidate cachedValue;
  cachedValue_ = 0.0;

} //ElapsedTimeCostLimit::unilaterallyReduce()
//<pb>

// ***********************************************************************
// methods for class ElpasedTimeCostLimit
// -----------------------------------------------------------------------
//ScmElapsedTimeCostLimit*
//  ScmElapsedTimeCostLimit::castToElapsedTimeCostLimit() const
//{
//  return (ScmElapsedTimeCostLimit*)this;
//}

//=============================================================================
//  Convert this cost limit to a single value which represents the elapsed time
// remaining after the elapsed time of the associated Cost is subtracted from
// the initial elapsed time limit.  Use specified required physical properties
// for elapsed time conversion.
//
// Input:
//  rpp   -- specified required physical properties.
//
// Output:
//  none
//
// Return:
//  remaining elapsed time in the form of a double.
//
//=============================================================================
double
ScmElapsedTimeCostLimit::getValue(const ReqdPhysicalProperty* const rpp) const
{
  //-----------------------
  //  Create an empty cost.
  //-----------------------
  Cost* rollUp = new STMTHEAP Cost();

  //-------------------------------------------------------------------------
  //  Roll up accumulated sibling cost into accumulated ancestor cost
  //-------------------------------------------------------------------------
  rollUp->cpScmlr() = ancestorCost_->getScmCplr() + otherKinCost_->getScmCplr();
  CostScalar ncmCostLimitFF =
              ActiveSchemaDB()->getDefaults().getAsDouble(NCM_COSTLIMIT_FACTOR);
  rollUp->cpScmlr() = rollUp->cpScmlr().scaleByValue(ncmCostLimitFF);
  double newLimitVal = rollUp->convertToElapsedTime(rpp).value();
  delete rollUp;

  //--------------------------------------------------------------------
  //  Remaining elapsed time is upper limit less the calculated limit of
  // ancestors and siblings.
  //--------------------------------------------------------------------
  return upperLimit_.value() - newLimitVal;

} // ElapsedTimeCostLimit::getValue

//============================================================================
// Compare this cost limit with a specified cost limit using specified
// required physical properties for any elapsed time conversions.  Comparison
// is based on associated value of each cost limit.
//
// Input:
//  other -- specified cost limit with which to compare.
//
//  rpp   -- specified required physical properties.
//
// Output:
//  none
//
// Return:
//  Result of comparison.
//
//============================================================================
COMPARE_RESULT
ScmElapsedTimeCostLimit::compareCostLimits
                             (CostLimit* other,
                              const ReqdPhysicalProperty* const rpp)
{
  // other == NULL means infinite time, thus return LESS in this case:
  if (other==NULL)
  {
    return LESS;
  }
  else
  {
    // CMPASSERT(other->castToScmElapsedTimeCostLimit());
    // Higher priority plans always win the comparison and considered "cheaper"
    // Similarly a costLimit with higher priority limit should be considered
    // a stronger (i.e. lower) limit than one with low priority
    if (CmpCommon::getDefault(COMP_BOOL_193) == DF_OFF)
    {
      if (priorityLimit_ > other->priorityLimit())
        return LESS;
      if (priorityLimit_ < other->priorityLimit())
        return MORE;
    }

    // get my value and other value.
    double myValue = getValue(rpp) * priorityLimit().riskPremium().value();
    double otherValue = other->getValue(rpp) *
                        other->priorityLimit().riskPremium().value();

    if (myValue > otherValue)
    {
      return MORE;
    }
    else
    {
      if (myValue < otherValue)
      {
        return LESS;
      }
      else
      {
        return SAME;
      }
    }
  } // other != NULL

} // ScmElapsedTimeCostLimit::compareCostLimits()

//=============================================================================
// Compare this CostLimit with a Cost object by first converting the Cost
// object to a CostLimit and then comparing the two CostLimit objects using
// specified required physical properties for elapsed time calculations.
//
// Input:
//  other -- cost object with which to compare.
//
//  rpp   -- specified required physical properties.
//
// Output:
//  none
//
// Return:
//  Result of comparison.
//
//=============================================================================
COMPARE_RESULT
ScmElapsedTimeCostLimit::compareWithCost (const Cost & other,
                                          const ReqdPhysicalProperty* const rpp)
{
  // Higher priority plans always win the comparison and considered "cheaper"
  // Similarly a costLimit with higher priority limit should be considered
  // a stronger (i.e. lower) limit than one with low priority
  if (CmpCommon::getDefault(COMP_BOOL_193) == DF_OFF)
  {
    // xxx We only consider the demotion levels of priority limit for this type
    // of cost based pruning. This is because demotions levels are monotonically
    // decreasing with upper limit of zero.
    Lng32 demotionLimit = priorityLimit_.getDemotionLevel()
                         - ancestorCost_->getPlanPriority().getDemotionLevel()
                         - otherKinCost_->getPlanPriority().getDemotionLevel();

    if (demotionLimit > other.getPlanPriority().getDemotionLevel())
      return LESS;
    if (demotionLimit < other.getPlanPriority().getDemotionLevel())
      return MORE;
  }
  // Trying to be less aggressive. First, roll-up other cost into cost limit
  // then call getValue() if result is negative - return LESS and so on.
  // for conservative CL use CQD OPH_USE_CONSERVATIVE_COST_LIMIT 'ON';
  // Note that here we cannot use cachedValue.
  if ( CURRSTMT_OPTDEFAULTS->OPHuseConservativeCL() )
  {
    CostLimit * costLimit = copy();
    costLimit->ancestorAccum(other, rpp);
    double diff = costLimit->getValue(rpp);
    delete costLimit;

    if ( diff < -0.000001 )
      return LESS;
    else if ( diff > 0.000001 )
      return MORE;
    else return SAME;
  }

  return compareCostLimits(other.convertToCostLimit(rpp), rpp);
}

COMPARE_RESULT
ScmElapsedTimeCostLimit::compareWithPlanCost (CascadesPlan* plan,
                                       const ReqdPhysicalProperty* const rpp)
{
  return compareWithCost(*plan->getRollUpCost(),rpp);
}

//============================================================================
//  Accumulate a specified cost into this cost limit's ancestor costs using
// specified required physical properties.
//
// Input:
//  otherCost -- specified Cost object.
//
//  rpp       -- specified required physical properties.
//
// Output:
//  none
//
// Return:
//  none
//
//============================================================================
void
ScmElapsedTimeCostLimit::ancestorAccum(const Cost& otherCost,
                                    const ReqdPhysicalProperty* const rpp)
{
  //--------------------------------------------------------------------------
  //  This temporary cost object will ultimately contain the newly accumulated
  // cost.  Create it initially empty.
  //--------------------------------------------------------------------------
  Cost* tempCost = new (CmpCommon::statementHeap()) Cost();
  tempCost->cpScmlr() = ancestorCost_->getScmCplr() + otherCost.getScmCplr();
  //-----------------------------------------------------
  //  As in standard roll-up, priority will be combined.
  tempCost->planPriority() = ancestorCost_->getPlanPriority() +
                             otherCost.getPlanPriority();
  //-------------------------------------------------------------------------
  //  Delete old accumulated cost and replace it with newly accumulated cost.
  //-------------------------------------------------------------------------
  delete ancestorCost_;

  ancestorCost_ = tempCost;

} //ScmElapsedTimeCostLimit::ancestorAccum()

//=========================================================================
//  Accumulate a specified cost into this cost limit's other kin cost.
//
// Input:
//  otherCost -- specified Cost object.
//
// Output:
//  none
//
// Return:
//  none
//
//=========================================================================
void
ScmElapsedTimeCostLimit::otherKinAccum(const Cost& otherCost)
{
  //--------------------------------------------------------------------------
  //  This temporary cost object will ultimately contain the newly accumulated
  // cost.  Create it initially empty.
  //--------------------------------------------------------------------------
  Cost* tempCost = new (CmpCommon::statementHeap()) Cost();
  tempCost->cpScmlr() = otherKinCost_->getScmCplr() + otherCost.getScmCplr();
  //-----------------------------------------------------
  //  As in standard roll-up, priority will be combined.
  tempCost->planPriority() = otherKinCost_->getPlanPriority() +
                             otherCost.getPlanPriority();
  //-------------------------------------------------------------------------
  //  Delete old accumulated cost and replace it with newly accumulated cost.
  //-------------------------------------------------------------------------
  delete otherKinCost_;

  otherKinCost_ = tempCost;

} // ScmElapsedTimeCostLimit::otherKinAccum()

//===========================================================================
//  Potentially reduce upper limit of this CostLimit based on the cost of a
// more promising plan.  Use specified required physical properties for
// elapsed time conversions.
//
// Input:
//  otherCost -- specified Cost object associated with more promising plan.
//
//  rpp       -- specified required physical properties.
//
// Output:
//  none
//
// Return:
//  none
//
//===========================================================================
void
ScmElapsedTimeCostLimit::tryToReduce(const Cost& otherCost,
                                  const ReqdPhysicalProperty* const rpp)
{

  //-------------------------------------------------------------------------
  // Calculate new potential priority limit as the sum of the priorities of
  // the new plan and the siblings and ancestors.
  //-------------------------------------------------------------------------
  NABoolean priorityLimitIncreased = FALSE;
  PlanPriority newPriorityLimit =
    otherCost.getPlanPriority() +
    ancestorCost_->getPlanPriority() +
    otherKinCost_->getPlanPriority();
  if (newPriorityLimit > priorityLimit_ AND // we have higher priority option
      CmpCommon::getDefault(COMP_BOOL_193) == DF_OFF)
  {
    priorityLimit_ = newPriorityLimit;
    priorityLimitIncreased = TRUE;
  }

  //-------------------------------------------------------------------------
  //  Calculate new potential upper limit as the sum of the elapsed time of
  // the cost of the new plan and the elapsed time of the combined costs of
  // siblings and ancestors accumulated so far.
  //-------------------------------------------------------------------------
  double newLimitVal;
  if (ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_95) == 0)
  {
    newLimitVal = otherCost.convertToElapsedTime(rpp).value()
                       + ancestorCost_->convertToElapsedTime(rpp).value()
                       + otherKinCost_->convertToElapsedTime(rpp).value();
  }
  else
  {
    newLimitVal = otherCost.convertToElapsedTime(rpp).value()
                       + ancestorCost_->convertToElapsedTime(rpp).value()
                       + otherKinCost_->convertToElapsedTime(rpp).value();
  }

  //------------------------------------------------------------------------
  //  If newly calculated limit is smaller than old upper limit, make it the
  // official upper limit.
  //------------------------------------------------------------------------
  if (newLimitVal < upperLimit_.value() OR
      priorityLimitIncreased) // need to compute upperLimit based on new winner
    {
      upperLimit_ = ElapsedTime(newLimitVal);
    }

  // invalidate cachedValue;
  cachedValue_ = 0.0;

} //ScmElapsedTimeCostLimit::tryToReduce()

//============================================================================
//  Unilaterally reduce upper limit by a specified elapsed time without going
// below zero.
//
// Input:
//  timeReduction -- specified elapsed time reduction.
//
// Output:
//  none
//
// Return:
//  none
//
//============================================================================
void
ScmElapsedTimeCostLimit::unilaterallyReduce(const ElapsedTime & timeReduction)
{
  // This method is used for changing elapsed time limit for child of blocking
  // binary operators. It should not change priority limit

  upperLimit_ -= timeReduction;
  upperLimit_.minCsZero();

  // invalidate cachedValue;
  cachedValue_ = 0.0;
} //ElapsedTimeCostLimit::unilaterallyReduce()

// ---------------------------------------------------------------------------
// methods for class CostPrimitives
// ---------------------------------------------------------------------------

double CostPrimitives::cpuCostForCopySet(const ValueIdSet & vidset)
{
  return
       (getBasicCostFactor(CPUCOST_COPY_SIMPLE_DATA_TYPE) * vidset.entries());
}

double CostPrimitives::cpuCostForCopyRow(Lng32 byteCount)
{
  return (getBasicCostFactor(CPUCOST_COPY_ROW_OVERHEAD) +
                   getBasicCostFactor(CPUCOST_COPY_ROW_PER_BYTE) * byteCount);
}

double CostPrimitives::cpuCostForCompare(const ValueIdSet & vidset)
{
  const double entries = vidset.entries();

  if (entries != 0.0 )
  {
      const double compareCost =
	(getBasicCostFactor(CPUCOST_COMPARE_SIMPLE_DATA_TYPE) * entries);
      const double length = (vidset.isEmpty() ? 0 : vidset.getRowLength());
      const double extraLength = MAXOF(8.0, length / entries / 2.0);
      return  (compareCost + compareCost * ((extraLength - 8.0) / 8.0));
  }
  else
  {
    return 0.0;
  }

}

double CostPrimitives::cpuCostForLikeCompare(const ValueId & vid)
{
  CMPASSERT(vid.getItemExpr()->getOperatorType() == ITM_LIKE);
  ItemExpr * child0 = vid.getItemExpr()->getChild(0)->castToItemExpr();
  Lng32 child0length = child0->getValueId().getType().getNominalSize();
  return (getBasicCostFactor(CPUCOST_LIKE_COMPARE_OVERHEAD) +
            getBasicCostFactor(CPUCOST_LIKE_COMPARE_PER_BYTE) * child0length);
}

double CostPrimitives::cpuCostForEvalArithExpr(const ValueId & vid)
{
  CMPASSERT(vid.getItemExpr() != NULL); // Just to suppress warnings.
  return                            getBasicCostFactor(CPUCOST_EVAL_ARITH_OP);
}

double CostPrimitives::cpuCostForEvalFunc(const ValueId & vid)
{
  return                        getBasicCostFactor(CPUCOST_EVAL_FUNC_DEFAULT);
}
//<pb>

double CostPrimitives::cpuCostForEvalPred(const ValueIdSet & vidset)
{
  return
       (getBasicCostFactor(CPUCOST_EVAL_SIMPLE_PREDICATE) * vidset.entries());
}

double CostPrimitives::cpuCostForEvalExpr(const ValueId & vid)
{
  return                            getBasicCostFactor(CPUCOST_EVAL_ARITH_OP);
}

double CostPrimitives::cpuCostForEncode(const ValueIdSet & vidset)
{
  return
        (getBasicCostFactor(CPUCOST_ENCODE_PER_BYTE) * vidset.getRowLength());
}

double CostPrimitives::cpuCostForHash(const ValueIdSet & vidset)
{
  return (getBasicCostFactor(CPUCOST_HASH_PER_KEY) * vidset.entries() +
           getBasicCostFactor(CPUCOST_HASH_PER_BYTE) * vidset.getRowLength());
}

double CostPrimitives::cpuCostForAggrRow(const ValueIdSet & vidset)
{
  return       (getBasicCostFactor(CPUCOST_EVAL_ARITH_OP) * vidset.entries());
}
//<pb>

//  The parameter mscf is one of the factors listed in the table
// defaultDefaults in sqlcomp/NADefaults.C; getBasicCostFactor returns
// the associated value listed in that table. To add a factor, include
// it in sqlcomp/DefaultConstants.h and include that file where the
// factor will be used. Also, list the factor and its value in the
// defaultDefaults table.
double CostPrimitives::getBasicCostFactor(Lng32 id)
{
  
  if (id == MSCF_OV_MSG ||
      id == MSCF_OV_IO)
    return 1.0;
  return CmpCommon::getDefaultNumeric((DefaultConstants)id);
}
// eof
