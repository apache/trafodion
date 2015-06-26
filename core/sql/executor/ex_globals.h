/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2015 Hewlett-Packard Development Company, L.P.
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
#ifndef EX_GLOBALS_H
#define EX_GLOBALS_H

/* -*-C++-*-
******************************************************************************
*
* File:         ex_globals.h
* Description:  Base Class declaration for executor statement globals
*               
* Created:      In the precambrian era
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include <setjmp.h>

#include "Platform.h"
#include "ExCollections.h"
#include "Int64.h"
#include "FragDir.h"
#include "ex_god.h"

// forward references
class ExScheduler;
class ExExeStmtGlobals;
class ExMasterStmtGlobals;
class ExEidStmtGlobals;
class SqlSessionData;
class ExStatisticsArea;
class ex_tcb;
class ExMeasStmtCntrs;
class StatsGlobals;
class sql_buffer_pool;
class LOBglobals;
class SequenceValueGenerator;

/////////////////////////////////////////////////////////////
// class ex_globals
// An object of globals class is created before 'build'ing
// the tcb's from tdb's. This object is then passed to 
// the 'build' function of tdbs.
/////////////////////////////////////////////////////////////
class ex_globals : public ExGod
{
public:

  NA_EIDPROC ex_globals(short num_temps,
			short create_gui_sched = 0,
			Space    * space = NULL,
			CollHeap * heap  = NULL);

  // reallocate members
  NA_EIDPROC void reAllocate(short create_gui_sched = 0);

  // to be called instead of a destructor
  NA_EIDPROC virtual void deleteMe(NABoolean fatalError);

  // operator to delete memory allocated with ::operator new(ex_globals *)
  // NOTE: this of course does NOT call the destructor for the object to delete
  NA_EIDPROC void deleteMemory(void *mem);
  
  NA_EIDPROC inline ExScheduler * getScheduler()            { return sch_; }

  NA_EIDPROC inline void ** getTempsList()             { return tempList_; }
  NA_EIDPROC inline Lng32 getNumTemps() const           { return numTemps_; }
  NA_EIDPROC void setNumOfTemps(Lng32 numTemps);

  NA_EIDPROC inline void setSpace(Space * space)           {space_ = space;}
  NA_EIDPROC inline Space * getSpace()                      {return space_;}

  // return a pointer to default heap, which is the heap specified
  // as an argument to the constructor
  NA_EIDPROC inline CollHeap *getDefaultHeap()             { return heap_; }

  NA_EIDPROC virtual ExExeStmtGlobals * castToExExeStmtGlobals();
  NA_EIDPROC virtual ExEidStmtGlobals * castToExEidStmtGlobals();

  NA_EIDPROC inline void setStatsArea(ExStatisticsArea * statsArea)
    { statsArea_ = statsArea; }

  // returns stats area, if allocated AND if stats are enabled
  NA_EIDPROC ExStatisticsArea* getStatsArea() 
  { return (statsEnabled() ? statsArea_ : NULL); }

  // returns stats area, if it were allocated
  NA_EIDPROC ExStatisticsArea* getOrigStatsArea() 
  { return statsArea_; }

  NA_EIDPROC inline jmp_buf *getJmpBuf()             { return &longJmpTgt_; }
  NA_EIDPROC inline void setJmpInScope(NABoolean jmpInScope)
    { jmpInScope_ = jmpInScope; }
  NA_EIDPROC inline NABoolean IsJmpInScope() { return jmpInScope_; }

  NA_EIDPROC inline void setEventConsumed(UInt32 *eventConsumed)
    { eventConsumedAddr_ = eventConsumed; }

  NA_EIDPROC inline UInt32 *getEventConsumed()
					      { return eventConsumedAddr_; }

  NA_EIDPROC inline void registerTcb( ex_tcb *newTcb)  
    { tcbList_.insert(newTcb); }

  NA_EIDPROC void cleanupTcbs();

  NA_EIDPROC void testAllQueues();

  NA_EIDPROC ExMeasStmtCntrs* getMeasStmtCntrs();
 

  // get the fragment id, the number of instances for my fragment,
  // or get my own fragment instance number (partition number in DP2)
  virtual ExFragId getMyFragId() const = 0;
  virtual Lng32 getNumOfInstances() const = 0;
  virtual Lng32 getMyInstanceNumber() const = 0;

NA_EIDPROC 
  inline ULng32 getInjectErrorAtExpr() const 
                                        { return injectErrorAtExprFreq_; }
NA_EIDPROC 
  inline void setInjectErrorAtExpr(ULng32 cif) 
                                        { injectErrorAtExprFreq_ = cif; }
NA_EIDPROC 
  inline ULng32 getInjectErrorAtQueue() const 
                                        { return injectErrorAtQueueFreq_; }
NA_EIDPROC 
  inline void setInjectErrorAtQueue(ULng32 cif) 
                                        { injectErrorAtQueueFreq_ = cif; }

NA_EIDPROC
  const LIST(ex_tcb *) &tcbList() const { return tcbList_; }

NA_EIDPROC 
  NABoolean computeSpace(){return (flags_ & COMPUTE_SPACE) != 0;};
NA_EIDPROC
  void setComputeSpace(NABoolean v)
  { (v ? flags_ |= COMPUTE_SPACE : flags_ &= ~COMPUTE_SPACE); };

NA_EIDPROC 
  NABoolean measStmtEnabled(){return (flags_ & MEAS_STMT_ENABLED) != 0;};
NA_EIDPROC
  void setMeasStmtEnabled(NABoolean v)
  { (v ? flags_ |= MEAS_STMT_ENABLED : flags_ &= ~MEAS_STMT_ENABLED); };

NA_EIDPROC 
  NABoolean statsEnabled() {return (flags_ & STATS_ENABLED) != 0;};
NA_EIDPROC
  void setStatsEnabled(NABoolean v)
  { (v ? flags_ |= STATS_ENABLED : flags_ &= ~STATS_ENABLED); };

  // getStreamTimeout: return TRUE (FALSE) if the stream-timeout was set (was
  // not set). If set, the timeoutValue parameter would return that value
  NA_EIDPROC virtual NABoolean getStreamTimeout( Lng32 & timeoutValue ) = 0;

  UInt32 planVersion() {return planVersion_;};
  void setPlanVersion(UInt32 pv){planVersion_ = pv; };

  virtual StatsGlobals *getStatsGlobals() { return NULL;}
  virtual Long getSemId() { return 0;}
  virtual pid_t getPid() { return 0;}
  virtual pid_t getTid() { return 0;}
  virtual Lng32 myNodeNumber() { return 0; }

  inline sql_buffer_pool *getSharedPool() { return sharedPool_; }
  inline void setSharedPool(sql_buffer_pool *p) { sharedPool_ = p; }

  void *& lobGlobal();
  LOBglobals * lobGlobals() { return lobGlobals_; }
  
  void initLOBglobal();
  
  SequenceValueGenerator * seqGen();
  
  Int64 &rowNum() { return rowNum_; }

private:
  enum FlagsTypeEnum 
  {
    // SPACE_COMP_MODE: indicates that the build() phase is
    // being done to compute space requirement only.
    // Do not make any changes to the generated expressions, or TDBs.
    // (like assigning tempsArea, assigning generated pcode, etc)
    COMPUTE_SPACE = 0x0001,
    
    // indicates that measure is running and stmt measurement is enabled.
    MEAS_STMT_ENABLED = 0x0002,

    // indicates that stat collection at runtime is enabled
    STATS_ENABLED = 0x0004

  };

  // the schedule to which the tcb's are added.
  ExScheduler * sch_;
  
  // number of temporaries
  short numTemps_;
  
  // Array containing the address of the temp tables.
  // Temps could be a tuple, a btree or a hash table.
  void ** tempList_;

  // pointer to where executor space is to be allocated from.
  Space * space_;

  // default heap for dynamic allocation and deallocation of 
  // memory in executor
  CollHeap * heap_;

  // pointer to the statsArea (if statistics are collected)
  ExStatisticsArea * statsArea_;

  // for handling tcb-build-time errors, and memory alloc errors.
  jmp_buf  longJmpTgt_;
  NABoolean jmpInScope_;

  // for cleanup.
  LIST(ex_tcb *) tcbList_;

  // For testing error handling & cancel.  0 means don't test.  
  // Other values should be even-powers-of-2 and control frequency 
  // of cancel injection as the denominator in the fraction 
  // 1/cancelInjectionFreq_.  The fraction is used w/ a random
  // number.
  ULng32 injectErrorAtQueueFreq_;    // used by ex_queue::insert.
  ULng32 injectErrorAtExprFreq_;    // used by ex_expr::eval.

  ULng32 flags_;

  // pointer to LDONE consumed indicator for Nowait CLI
  // and LSIG consumed indicator of event driven IpcSetOfConnections::wait
  UInt32 *eventConsumedAddr_;

  // plan version of the tdb fragment for this statement.
  // The plan version is set during compile time for all the 'root'
  // operators (root, eid root, split bottom). It is then put in here
  // during the TCB build phase. It could then be used by all operators.
  // For usage, see the actual executor operators.
  UInt32                planVersion_;                     

  Int64 rowNum_;

  // pool shared by among PAs under PAPA
  sql_buffer_pool *sharedPool_;

  LOBglobals * lobGlobals_;

  // pointer passed to interface methods that store and retrieve lob data
  // from flatfile or hdfs filesystem.
  // This pointer is initialized once per tcb.
  // Contents are set and used only by methods behind the interface.
  // Executor code does not access the contents.
  //  void * lobGlob_;

};

#endif


