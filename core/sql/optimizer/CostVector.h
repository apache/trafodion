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
#ifndef COSTVECTOR_HDR
#define COSTVECTOR_HDR
/* -*-C++-*-
**************************************************************************
*
* File:         CostVector.h
* Description:  Cost
* Created:      11/12/96
* Language:     C++
*
* Purpose:	 Split from Cost.h to remove header file dependency loop
*
*
*
**************************************************************************
*/

// -----------------------------------------------------------------------

#include "NADefaults.h"
#include "BaseTypes.h"
#include "NAType.h"
#include "ValueDesc.h"
#include "CostScalar.h"
#include "opt.h"
#include "CmpStatement.h"



// -----------------------------------------------------------------------
// Forward references.
// -----------------------------------------------------------------------
class NAType;
class ValueId;
class ValueIdSet;
class ValueIdList;
class ItemExpr;
class PerformanceGoal;
class CostWeight;

// -----------------------------------------------------------------------
// The following classes are defined in this file.
// -----------------------------------------------------------------------

class CostVector;
class SimpleCostVector;


// -----------------------------------------------------------------------
// A vector that contains various resource consumption and elapsed
// time metrics (base class).
// -----------------------------------------------------------------------
class CostVector : public NABasicObject
{
public:

  virtual ~CostVector() {}

  // A virtual method that returns the number of entries.
  virtual Lng32 entries() const = 0;

  // A virtual method that provides access to a specific entry.
  virtual CostScalar operator[] (Lng32 ix) const = 0;

  // A virtual method that provides a string describing individual components
  virtual const NAString getDetailDesc(const DefaultToken ownline, const char *prefix = "") const = 0;

  // A virtual copy constructor.
  virtual CostVector * copy() const = 0;

}; // class CostVector
//<pb>
// -----------------------------------------------------------------------
// The following enum gives a name and a number to each of the
// entities that are measured for computing the cost of a plan.
//
/*j Before 04/12/2001 SimpleCostVector had 12 components as of now it
   has been reduced to FIVE components for better performance. And the
   enum has been changed as following.
   On 10-16-2006: In order to implement SCM, this enum has been extended
   to have FIVE new components.

  CPU_TIME: Total time of CPU = CPU_COST * FudgeFactor(CPU)

  IO_TIME: Total time of IO =
		  NUM_SEEKS * FF(NUM_SEEKS) + NUM_BYTES * FF(NUM_BYTES);

  MSG_TIME: Total time for messages =
		  NUM_LOCAL_MESSAGES * FF(NUM_LOCAL_MESSAGES) +
		  KB_LOCAL_MESAGES * FF(KB_LOCAL_MESAGES) +
		  NUM_REMOTE_MESSAGES * FF(NUM_REMOTE_MESSAGES) +
		  KB_REMOTE_MESSAGES * FF(KB_REMOTE_MESSAGES);

  IDLE_TIME : The time for which an operator is expected to not
  perform any useful work. Refered to as blk_idlt in the Cost Estimation
  document.

  // Begin SCM
  TC_PROC : Represents total number of tuples processed.

  TC_PROD : Represents total number of tuples produced.

  TC_SENT : Represents total number of tuples sent to parent.

  IO_RAND : Total number of random IOs.

  IO_SEQ : Total number of sequential IOs.
  // End SCM

  NUM_PROBES: Number of times that the operator is run, should be the last one.

  As we are not using Normal, Persitent Memory and Disk Usage those
  components have been eliminated from the vector for now.
  The below are the 12 components which were used previously if a need
  arises where we need to have more specific cost then we might change
  back to using one of these components and that is the reason these line
  have not been deleted. May be we will need it in future.
j*/
// CPU_COST:
//    A count of CPU instructions that are expected to be executed.
//    A value of 1 means 1000 machine instructions.
// NUM_SEEKS:
//    Number of random I/Os and positionings for sequential reads
// NUM_KBYTES:
//    Kilobytes of I/O transferred
// note: NUM_SEEKS and NUM_BYTES do not incorporate the effect of
// cache
// NORMAL_MEMORY:
//    Amount of memory, in KB, used to hold working buffers and hash tables
// PERSISTENT_MEMORY
//    Amount of memory, in KB, which persists after operation completion
// NUM_LOCAL_MESSAGES:
//    Number of local messages
// KB_LOCAL_MESAGES:
//    Kilobytes of local messages
// NUM_REMOTE_MESSAGES:
//    Number of remote messages
// KB_REMOTE_MESSAGES:
//    Kilobytes of remote messages
// IDLE_TIME:
//    The time for which an operator is expected to not perform
//    any useful work. Refered to as blk_idlt in the Cost Estimation document.
// DISK_USAGE:
//    Disk space utilization, in 1KB units, for creating intermediate
//    results of processing that are transient in nature.
// NUM_PROBES:
//    Number of times that the operator is run
// -----------------------------------------------------------------------
enum SimpleCostCountersEnum
  {

    CPU_TIME	  = 0,	// total time taken by cpu
    IO_TIME	  = 1,	// total time taken by IO
    MSG_TIME	  = 2,	// total time taken by messages
    IDLE_TIME	  = 3,	// waiting time
    TC_PROC       = 4,  // total number of tuples processed
    TC_PROD       = 5,  // total number of tuples produced
    TC_SENT       = 6,  // total number of tuples exchanged
    IO_RAND       = 7,  // total number of random IOs.
    IO_SEQ        = 8,  // total number of sequential IOs.
    NUM_PROBES    = 9,  // number of times the plan is run
    COUNT_OF_SIMPLE_COST_COUNTERS = 10  // $$$$ KEEP AS THE FINAL MEMBER

   /*j
    CPU_COST      = 0, // A count of CPU instructions that are expected to be
                       // executed. A value of 1 means 1000 machine
                       // instructions.
    NUM_SEEKS     = 1, // number of random io's and positioning for sequential
	               // reads.
    NUM_KBYTES    = 2, // kilobytes of I/O transfered
    NORMAL_MEMORY = 3, // amount of memory, in KB, used for buffers and tables
    PERSISTENT_MEMORY   = 4,   // amount of memory, in KB, persisting after
                               // operation
    NUM_LOCAL_MESSAGES  = 5,   // number of local messages
    KB_LOCAL_MESSAGES   = 6,   // Kilobytes of local messages
    NUM_REMOTE_MESSAGES = 7,   // Number of remote messages
    KB_REMOTE_MESSAGES  = 8,   // Kilobytes of remote messages
    IDLE_TIME           = 9,   // Time not doing useful work
    DISK_USAGE          = 10,  // Temporary disk space in KB units
    NUM_PROBES          = 11,  // number of times the plan is run
    COUNT_OF_SIMPLE_COST_COUNTERS = 12
  j*/                               // $$$$ KEEP AS THE FINAL MEMBER

 };
 //<pb>
// -----------------------------------------------------------------------
// A cost vector, implemented with CostScalar objects
// -----------------------------------------------------------------------
class SimpleCostVector : public CostVector
{

  //-------------------------------
  // Friend operators and functions
  //-------------------------------
  friend SimpleCostVector operator+  (const SimpleCostVector &v1,
                                      const SimpleCostVector &v2);
  friend SimpleCostVector operator-  (const SimpleCostVector &v1,
                                      const SimpleCostVector &v2);
  friend SimpleCostVector operator*  (const SimpleCostVector &vector,
                                      const CostScalar &scalar);
  friend SimpleCostVector operator/  (const SimpleCostVector &vector,
                                      const CostScalar &scalar);
  friend SimpleCostVector blockingAdd(const SimpleCostVector &v1,
                                      const SimpleCostVector &v2,
                                      const ReqdPhysicalProperty* const rpp);
  friend SimpleCostVector overlapAdd (const SimpleCostVector &v1,
                                      const SimpleCostVector &v2);
  friend SimpleCostVector overlapAddUnary (const SimpleCostVector &v1,
                                      const SimpleCostVector &v2);

  friend SimpleCostVector etMINOF    (const SimpleCostVector &v1,
                                      const SimpleCostVector &v2,
                                      const ReqdPhysicalProperty* const rpp);
  friend SimpleCostVector etMAXOF    (const SimpleCostVector &v1,
                                      const SimpleCostVector &v2,
                                      const ReqdPhysicalProperty* const rpp);
  friend SimpleCostVector vecMINOF   (const SimpleCostVector &v1,
                                      const SimpleCostVector &v2);
  friend SimpleCostVector vecMAXOF   (const SimpleCostVector &v1,
                                      const SimpleCostVector &v2);
  friend NABoolean isLowerBound      (const SimpleCostVector &v1,
                                      const SimpleCostVector &v2);

public:

  // ---------------------------------------------------------------------
  // Constructors (simple, specify costs, copy constructor)
  // Used by OCM.
  // ---------------------------------------------------------------------

  SimpleCostVector(const CostScalar & CPUTime           = csZero,
		   const CostScalar & IOTime            = csZero,
		   const CostScalar & MSGTime           = csZero,
		   const CostScalar & idleTime          = csZero,
		   const CostScalar & numProbes         = csOne
                  );

/*j
		   const CostScalar & cpuCost            = csZero,
		   const CostScalar & numSeeks           = csZero,
		   const CostScalar & numKBytes          = csZero,
		   const CostScalar & normalMemory       = csZero,
		   const CostScalar & persistentMemory   = csZero,
		   const CostScalar & numLocalMessages   = csZero,
		   const CostScalar & kbLocalMessages    = csZero,
		   const CostScalar & numRemoteMessages  = csZero,
		   const CostScalar & kbRemoteMessages   = csZero,
		   const CostScalar & idleTime           = csZero,
		   const CostScalar & diskUsage          = csZero,
		   const CostScalar & numProbes          = csOne
j*/
  // Used by SCM
  SimpleCostVector(const CostScalar & CPUTime,
                   const CostScalar & IOTime,
                   const CostScalar & MSGTime,
                   const CostScalar & idleTime,
                   const CostScalar & tcProc,
                   const CostScalar & tcProd,
                   const CostScalar & tcSent,
                   const CostScalar & ioRand,
                   const CostScalar & ioSeq,
                   const CostScalar & numProbes
                   );

  SimpleCostVector(const SimpleCostVector &other);

  virtual CostVector* copy() const
    { return new(CmpCommon::statementHeap()) SimpleCostVector(*this); }

  // ---------------------------------------------------------------------
  // Destructor
  // ---------------------------------------------------------------------
  ~SimpleCostVector() {}

  // ---------------------------------------------------------------------
  // Arithmetic operators for a SimpleCostVector. They are vector-oriented,
  // as opossed to blockingAdd, overlappedAdd and repeatedOverlappedAdd.
  // Please see below.
  // ---------------------------------------------------------------------

  SimpleCostVector & operator =  (const SimpleCostVector &other);

  // Simple addition of vectors; does not affect the number of probes, and they
  // must be the same in both vectors

  SimpleCostVector & operator += (const SimpleCostVector &other);

  // Simple substraction of vectors; does not affect the number of probes,
  // and they must be the same in both vectors; all negatives in the result
  // are set to zero

  SimpleCostVector & operator -= (const SimpleCostVector &other);

  // Simple multiplication of vector by an scalar; does not affect memory
  // components or the number of probes

  SimpleCostVector & operator *= (const CostScalar &other);

  // Simple division of vector by an scalar; does not affect memory
  // components or the number of probes

  SimpleCostVector & operator /= (const CostScalar &other);
  // ---------------------------------------------------------------------
  // Operators to measure costs in execution tree.
  // ---------------------------------------------------------------------

  // -- Overlapped addition of two operators. They are assumed to be executing
  //    concurrently in the same process.

  const SimpleCostVector& overlappedAdd(const SimpleCostVector & other);

  // -- Repeated overlapped addition. It does not apply to memory, either
  //    normal or persistent.

  const SimpleCostVector& repeatedOverlappedAdd(const Lng32 times);

  // ---------------------------------------------------------------------
  // scaleUpByNumProbes() scales up a simple cost vector by its number of
  // probes. Memory and disk space are considered to be recyclable over
  // probes, and hence not scaled up by this method.
  // ---------------------------------------------------------------------
  const SimpleCostVector& scaleUpByNumProbes();

  // ---------------------------------------------------------------------
  // scaleUpByCountOfCPUs() scales up a simple cost vector by the number
  // of CPUs. This is typically used to convert a simple cost vector which
  // tracks resource usage on a CPU to another simple cost vector which
  // tracks total resource usage over all CPUs. All components except the
  // no of probes are scaled up.
  // ---------------------------------------------------------------------
  const SimpleCostVector& scaleUpByCountOfCPUs(const Lng32 countOfCPUs);

  // ---------------------------------------------------------------------
  // scaleByValue() scales the vector by a specified scalar leaving only
  // number of probes unaffected.
  // ---------------------------------------------------------------------

  SimpleCostVector& scaleByValue(const CostScalar &scalar);
  //----------------------------------------------------------------------
  // sets each element of vector to given value; number of probes is not
  // changed
  //----------------------------------------------------------------------
  void setToValue(const CostScalar &);

  // -- Sets to zero all components

  const SimpleCostVector& reset();

  const SimpleCostVector&
    enforceUpperBound(const SimpleCostVector& upperBoundVector);

  // ---------------------------------------------------------------------
  // Miscellaneous functions.
  // ---------------------------------------------------------------------

  // This routine has the effect of setting the number of probes
  // to the input factor and scaling all other members accordingly
  // with the exception of the memory components. The intention
  // is to be able to do operations between the resulting vector
  // and another vector whose number of probes is factor.

  const SimpleCostVector& normalize(const CostScalar & factor);

  SimpleCostVector getNormalizedVersion(const CostScalar & factor) const;

  // Indicate whether or not this vector is a zero vector.
  NABoolean isZeroVector() const;

  NABoolean isZeroVectorWithProbes() const;

  // ---------------------------------------------------------------------
  // Accessor functions for the elements of the SimpleCostVector
  // ---------------------------------------------------------------------

  CostScalar getCounter(Lng32 index) const
    {
      CMPASSERT((index >= 0) AND (index < COUNT_OF_SIMPLE_COST_COUNTERS));
      return counter_[index];
    }

  // -- cpu time

  inline const CostScalar& getCPUTime() const
			      { return counter_[CPU_TIME]; }

  // -- total time spent

  CostScalar getElapsedTime(const ReqdPhysicalProperty* const rpp) const;

  // -- total time spent

  CostScalar getElapsedTime(const PerformanceGoal& goal,
                 const CostWeight* const vectorWeight = NULL) const;

  // NCM specific method.
  CostScalar getElapsedTime() const;

  // -- time spent doing input/output operations

  inline const CostScalar& getIOTime() const
				      { return counter_[IO_TIME]; }

  // -- time spent processing messages

  inline const CostScalar& getMessageTime() const
				      { return counter_[MSG_TIME]; }

  // -- time spent without doing useful work

  inline const CostScalar& getIdleTime() const
                                      { return counter_[IDLE_TIME]; }
  // For SCM
  // get tuples processed
  inline const CostScalar& getTcProc() const
                                      { return counter_[TC_PROC]; }
  // get tuples produced
  inline const CostScalar& getTcProd() const
                                      { return counter_[TC_PROD]; }

  // get tuples sent
  inline const CostScalar& getTcSent() const
                                      { return counter_[TC_SENT]; }

  // get Random Ios
  inline const CostScalar& getIoRand() const
                                      { return counter_[IO_RAND]; }

  // get sequential Ios
  inline const CostScalar& getIoSeq() const
                                      { return counter_[IO_SEQ]; }

  // -- number of times the plan is run

  inline const CostScalar& getNumProbes() const
                                      { return counter_[NUM_PROBES]; }

  virtual const NAString getDetailDesc(const DefaultToken ownline, const char *prefix = "") const;

/*

/*j  // -- temporary disk space

  inline const CostScalar& getDiskUsage() const
                                      { return counter_[DISK_USAGE]; }


  // -- normal memory
  inline const CostScalar& getNormalMemory() const
				      { return counter_[NORMAL_MEMORY];}

  // -- amount of memory persisting after operator has been used

  inline const CostScalar& getPersistentMemory() const
                                      { return counter_[PERSISTENT_MEMORY]; }

j*/
  // ---------------------------------------------------------------------
  // Mutator functions for the elements of the SimpleCostVector
  // ---------------------------------------------------------------------


//////////////////// SETTING CPU TIME ////////////////////////

  // -- cpu cost

  inline void setCPUTime(const CostScalar & value)
  {
    counter_[CPU_TIME] = value; }

  // set the cpu time by converting the instructions passed into time
  inline  void setInstrToCPUTime(const CostScalar & value)
  {
    counter_[CPU_TIME] = value * CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();
  }

  // add to cpu time by converting the instructions passed into time
  inline  void addInstrToCPUTime(const CostScalar & value)
  {
    counter_[CPU_TIME] += value * CURRSTMT_OPTDEFAULTS->getTimePerCPUInstructions();
  }


/////////////////// SETTING IO TIME   /////////////////////////

  // -- Elapsed IO Calculated by adding cost of seeks & Kbytes transfer

  inline void setIOTime(const CostScalar & value)
  { counter_[IO_TIME] = value; }

  inline  void addSeeksToIOTime(const CostScalar & value)
  {
    counter_[IO_TIME] += value * CURRSTMT_OPTDEFAULTS->getTimePerSeek();
  }

  inline  void addKBytesToIOTime(const CostScalar & value)
  {
    counter_[IO_TIME] += value * CURRSTMT_OPTDEFAULTS->getTimePerSeqKb();
  }


/////////////// SETTING MESSAGES TIME /////////////////
// Calcuating Elapsed MSG by adding the num of Local messages,
// kbytes of local messages, Remote messages & kbytes for remote msgs.

  inline void setMSGTime(const CostScalar & value)
  {
    counter_[MSG_TIME] = value;
  }

  inline  void addNumLocalToMSGTime(const CostScalar & value)
  {
    counter_[MSG_TIME] += value * CURRSTMT_OPTDEFAULTS->getTimePerLocalMsg();
  }

  inline  void addKBLocalToMSGTime(const CostScalar & value)
  {
    counter_[MSG_TIME] += value * CURRSTMT_OPTDEFAULTS->getTimePerKbOfLocalMsg();
  }

  inline  void addNumRemoteToMSGTime(const CostScalar & value)
  {
    counter_[MSG_TIME] += value * CURRSTMT_OPTDEFAULTS->getTimePerRemoteMsg();
  }

  inline  void addKBRemoteToMSGTime(const CostScalar & value)
  {     counter_[MSG_TIME] += value * CURRSTMT_OPTDEFAULTS->getTimePerKbOfRemoteMsg();
  }



///////////// SETTING IDLE TIME /////////////////////////////////


  // -- time spent without doing useful work

  inline  void setIdleTime(const CostScalar & value)
                                      { counter_[IDLE_TIME] = value; }
  inline  void addToIdleTime(const CostScalar & value)
                                      { counter_[IDLE_TIME] += value; }

inline  void addToCpuTime(const CostScalar & value)
                                      { counter_[CPU_TIME] += value; }
////////////////// SET METHODS FOR SCM /////////////////////////////

  inline void setTcProc(const CostScalar & value)
  {
    counter_[TC_PROC] = value;
  }

  inline void setTcProd(const CostScalar & value)
  {
    counter_[TC_PROD] = value;
  }

  inline void setTcSent(const CostScalar & value)
  {
    counter_[TC_SENT] = value;
  }

  inline void setIoRand(const CostScalar & value)
  {
    counter_[IO_RAND] = value;
  }

  inline void setIoSeq(const CostScalar & value)
  {
    counter_[IO_SEQ] = value;
  }

//////////////////// SETTING NUMBER OF PROBES /////////////////////

  // -- number of times the plan is run

  inline  void setNumProbes(const CostScalar & value)
                                      { counter_[NUM_PROBES] = value; }
  inline  void addToNumProbes(const CostScalar & value)
                                      { counter_[NUM_PROBES] += value; }


////////////////////////////// END //////////////////////////////////

/*j

  inline  void setDiskUsage(const CostScalar & value)
   { counter_[DISK_USAGE] = value; }
  inline  void addToDiskUsage(const CostScalar & value)
   { counter_[DISK_USAGE] += value; }

  // -- amount of memory to hold buffers and tables

  inline  void setNormalMemory(const CostScalar & value)
                                      { counter_[NORMAL_MEMORY] = value; }
  inline  void addToNormalMemory(const CostScalar & value)
                                      { counter_[NORMAL_MEMORY] += value; }

  // -- amount of memory persisting after operator has been used

  inline  void setPersistentMemory(const CostScalar & value)
                                      { counter_[PERSISTENT_MEMORY] = value; }
  inline  void addToPersistentMemory(const CostScalar & value)
                                      { counter_[PERSISTENT_MEMORY] += value; }

j*/

  // -- prints all members of this class
  void print(FILE* pfp = stdout) const;

  // ---------------------------------------------------------------------
  // Overloaded indexing operator for accessing individual elements.
  // ---------------------------------------------------------------------
  virtual Lng32 entries() const;

  virtual CostScalar operator[] (Lng32 ix) const;

private:

  CostScalar counter_[COUNT_OF_SIMPLE_COST_COUNTERS];

}; // class SimpleCostVector

//----------------------------------------------------------------------
//  Needed for passing pointers to SimpleCostVector objects as reference
// parameters.
//----------------------------------------------------------------------
typedef SimpleCostVector* CostVecPtr;
#endif /* COSTVECTOR_HDR */
