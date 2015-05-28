/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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

#ifndef PCODEEXPRCACHE_H
#define PCODEEXPRCACHE_H

#include "BaseTypes.h"
#include "NABasicObject.h"
#include "CmpCommon.h"
#include "Collections.h"
#include "NAString.h"
#include "ItemColRef.h"
#include "BindWA.h"
#include "ExpPCodeOptimizations.h"
#include "ExpPCodeInstruction.h"

typedef NAHeap NABoundedHeap;
class NAHeap ;

#define OPT_PCC_DEBUG 1 /* Set to 1 if PCEC Logging needed, 0 otherwise */

Int64 computeTimeUsed( timeval begTime ) ;

class PCECacheEntry {        // Doubly-linked PCode Expr Cache Entry
public:

  // Accessors
  UInt32 getOptConstsLen()        const { return constsLenO_ ; }
  UInt32 getUnOptConstsLen()      const { return constsLenU_ ; }
  UInt32 getNEConstsLen()         const { return constsLenN_ ; }
  UInt32 getTempsAreaLen()        const { return tempsLenO_  ; }
  UInt32 getOptPClen()            const { return pcLenO_     ; }
  UInt32 getUnOptPClen()          const { return pcLenU_     ; }
  UInt64 getPCEHits()             const { return hits_       ; }

#if OPT_PCC_DEBUG==1
  Int64  getNEgenTime()            const { return NEgenTime_   ; }
  Int64  getOptTime()              const { return optTime_     ; }
  UInt64 getUniqCtr()              const { return myUniqueCtr_ ; }
#endif /* OPT_PCC_DEBUG==1 */

  char          * getConstsArea()     const { return constsPtr_  ; }
  PCodeBinary   * getOptPCptr()       const { return pCodeO_ ; }
  PCodeBinary   * getUnOptPCptr()     const { return pCodeU_ ; }
  PCECacheEntry * getNextInCrOrder()  const { return pnextInCreateOrder_  ; }
  PCECacheEntry * getPrevInCrOrder()  const { return pprevInCreateOrder_  ; }
  PCECacheEntry * getNextInMRUOrder() const { return pnextMRU_  ; }
  PCECacheEntry * getPrevInMRUOrder() const { return pprevMRU_  ; }

  UInt32 getTotalMemorySize()     const
  {
     return ( (pcLenU_ + pcLenO_ ) * sizeof(PCodeBinary) +
                 constsLenN_ + sizeof( PCECacheEntry ) ) ;
  }

  // Various methods that change the PCEC entry
  void setPrevInCrOrder(PCECacheEntry* prevp)  { pprevInCreateOrder_ = prevp ; }
  void setNextInCrOrder(PCECacheEntry* nextp)  { pnextInCreateOrder_ = nextp ; }
  void setPrevInMRUOrder(PCECacheEntry* prevp) { pprevMRU_ = prevp ; }
  void setNextInMRUOrder(PCECacheEntry* nextp) { pnextMRU_ = nextp ; }

  void   addToOptTime( Int64 totAddTime ) { optTime_ += totAddTime ; }
  UInt64 incrPCEHits()                    { return ++hits_         ; }

  NABoolean matches( PCodeBinary * unOptPCodePtr, char * unOptConstantsArea,
                     UInt32 unOptPCodeLen, UInt32 unOptConstsLen, UInt32 NEflag );

  // constructor
  PCECacheEntry( NAHeap        * heap
               , PCodeBinary   * unOptimizedPCode
               , PCodeBinary   * optimizedPCode
               , char          * constsPtr
               , UInt32          unOptPClen
               , UInt32          optPClen
               , UInt32          constsLenU
               , UInt32          constsLenO
               , UInt32          constsLenN
               , UInt32          tempsLenO
#if OPT_PCC_DEBUG==1
               , UInt64          optTime
               , UInt64          NEgenTime
               , UInt64          myUniqCtr
#endif /* OPT_PCC_DEBUG==1 */
               )
  : heap_              ( heap ) ,
    pnextInCreateOrder_  (NULL) ,
    pprevInCreateOrder_  (NULL) ,
    pnextMRU_            (NULL) ,
    pprevMRU_            (NULL) ,
    constsPtr_           (NULL) ,
    pcLenU_      ( unOptPClen ) ,
    pcLenO_      ( optPClen   ) ,
    constsLenU_  ( constsLenU ) ,
    constsLenO_  ( constsLenO ) ,
    constsLenN_  ( constsLenN ) ,
    tempsLenO_   ( tempsLenO  ) ,
#if OPT_PCC_DEBUG==1
    optTime_     ( optTime    ) ,
    NEgenTime_   ( NEgenTime  ) ,
    myUniqueCtr_ ( myUniqCtr  ) ,
#endif /* OPT_PCC_DEBUG==1 */
    hits_   ( 0 )
    {
       pCodeU_ = new(heap) PCodeBinary[ unOptPClen ];
       pCodeO_ = new(heap) PCodeBinary[ optPClen ];

       memcpy( pCodeU_, unOptimizedPCode, sizeof(PCodeBinary) * unOptPClen );
       memcpy( pCodeO_, optimizedPCode,   sizeof(PCodeBinary) * optPClen   );

       if ( constsLenN > 0 )
       {
          constsPtr_ = new(heap) char[ constsLenN ];
          memcpy( constsPtr_ , constsPtr , constsLenN   );
       }
    };


  // destructor
  ~PCECacheEntry()
   {
     NADELETEBASIC( pCodeU_ , heap_ );
     NADELETEBASIC( pCodeO_ , heap_ );
     if ( constsLenN_ > 0 )
       NADELETEBASIC( constsPtr_ , heap_ );
   };

private:
  NAHeap        * heap_       ;  // heap to use for memory allocations
  PCECacheEntry * pnextInCreateOrder_      ;
  PCECacheEntry * pprevInCreateOrder_      ;
  PCECacheEntry * pnextMRU_   ;
  PCECacheEntry * pprevMRU_   ;
  PCodeBinary   * pCodeU_     ;  // Ptr    to unoptimized PCode byte stream
  PCodeBinary   * pCodeO_     ;  // Ptr    to optimized   PCode byte stream
  char          * constsPtr_  ;  // Ptr to cached constants area
  UInt64          hits_       ;  // number of hits for this cache entry
  UInt32          pcLenU_     ;  // Length of unoptimized PCode byte stream
  UInt32          pcLenO_     ;  // Length of optimized   PCode byte stream
  UInt32          constsLenU_ ;  // Length of ConstantsArea for unoptimized PC
  UInt32          constsLenO_ ;  // Length of ConstantsArea for   optimized PC
  UInt32          constsLenN_ ;  // Length of ConstantsArea with Native Expr.
  UInt32          tempsLenO_  ;  // Length of TempsArea needed
#if OPT_PCC_DEBUG==1
  UInt64          myUniqueCtr_ ;
  Int64           optTime_    ; // Will incl time to Add to the cache
  Int64           NEgenTime_  ;
#endif /* OPT_PCC_DEBUG==1 */
};

class OptPCodeCache : public NABasicObject { // Anchor for PCode Expr Cache
 public:

  // Accessors
  NAHeap * getCacheHeapPtr() const { return heap_   ; }
  UInt64 getNumLookups() const { return numLookups_ ; }
  UInt64 getNumHits()    const { return numHits_    ; }
  UInt64 getMaxHits()    const { return maxHits_    ; }
  UInt64 getNumNEHits()  const { return numNEHits_  ; }
  UInt64 getMaxHitsDel() const { return maxHitsDel_ ; }
  UInt32 getNumEntries() const { return numEntries_ ; }
  UInt32 getCurrSize()   const { return currSize_   ; }
  UInt32 getMaxSize()    const { return maxSize_    ; }

#if OPT_PCC_DEBUG==1
  UInt64 getUniqFileNameTime() { return fileNameTime_ ; } ;
  UInt32 getUniqFileNamePid()  { return fileNamePid_ ;  } ;
#endif /* OPT_PCC_DEBUG==1 */

  NABoolean getPCECLoggingEnabled() { return (PCECLoggingEnabled_ > 0) ; } ;
  void      setPCECLoggingEnabled( Int32 enabVal ) { PCECLoggingEnabled_ = enabVal ; } ;

  void addPCodeExpr( PCECacheEntry * newPCEntry
#if OPT_PCC_DEBUG==1
                   , UInt64          NEgenTime
                   , timeval         begAdd
                   , char     *      sqlStmt
#endif // OPT_PCC_DEBUG==1
                   ) ;

  PCECacheEntry * findPCodeExprInCache( PCodeBinary * unOptPCodePtr
                                      , char   * unOptConstantsArea
                                      , UInt32   optFlags
                                      , UInt32   unOptPCodeLen
                                      , UInt32   unOptConstsLen
#if OPT_PCC_DEBUG==1
                                      , char   * sqlStmt
#endif // OPT_PCC_DEBUG==1
                                      ) ;

  void  clearStats() ;
  void  resizeCache( Int32 newsiz )  ;
  void  printPCodeExprCacheStats();

#if OPT_PCC_DEBUG==1
  UInt64 genNewUniqCtrVal() { return ++uniqueCtr_ ; }
  void   addToTotalSavedTime( Int64 totSavedTime )   { totalSavedTime_  += totSavedTime ; }
  void   addToTotalSearchTime( Int64 totSearchTime ) { totalSearchTime_ += totSearchTime ; }

  const  OptPCodeCache * getThisPtr() const { return this  ; }  // Strictly for debug
  char * getPCDlogDirPath()           const { return logDirPath_ ; } ;

  void   setPCDlogDirPath( NAString * logDirPth ) ;

#endif /* OPT_PCC_DEBUG==1 */

  //constructor
  OptPCodeCache()
    : heap_( new CTXTHEAP NABoundedHeap
            ("optPCode cache heap", (NAHeap *)CTXTHEAP, 0, 0) )
    , createOrderHead_  (NULL)
    , createOrderTail_  (NULL)
    , MRUHead_          (NULL)
    , MRUTail_          (NULL)
    , lastMatchedEntry_ (NULL)
    , numLookups_ ( 0 )
    , numSrchd_   ( 0 )
    , totSrchd_   ( 0 )
    , numHits_    ( 0 )
    , numNEHits_  ( 0 )
    , maxHits_    ( 0 )
    , maxHitsDel_ ( 0 )
    , totByCfC_   ( 0 )
    , numEntries_ ( 0 )
    , currSize_   ( 0 )
    , maxOptPCodeSize_( 0 )
    , PCECLoggingEnabled_( 0 )
    , PCECHeaderWritten_ ( 0 )
#if OPT_PCC_DEBUG==1
    , totalSavedTime_ ( 0 )
    , totalSearchTime_( 0 )
    , totalOptTime_   ( 0 )
    , totalNEgenTime_ ( 0 )
    , uniqueCtr_      ( 0 )
    , fileNameTime_   ( -1 )
    , fileNamePid_    ( -1 )
    , logDirPath_   ( NULL )
#endif /* OPT_PCC_DEBUG==1 */

    {
       setInitialMaxCacheSize() ;
#if OPT_PCC_DEBUG==1
       genUniqFileNamePart() ;
#endif /* OPT_PCC_DEBUG==1 */
    } ;

  // destructor
  ~OptPCodeCache()
   { resizeCache(0) ;
#if OPT_PCC_DEBUG==1
     if ( logDirPath_ )
        NADELETEBASIC( logDirPath_ , heap_ );
     logDirPath_ = NULL ;
#endif /* OPT_PCC_DEBUG==1 */
   };

#if OPT_PCC_DEBUG==1
private:
  void genUniqFileNamePart();
  void throwOutExcessCacheEntries() ;

  void addToCreateOrderList( PCECacheEntry* PCEtoAdd ) ;
  void removeFromCreateOrderList( PCECacheEntry* PCEtoRem ) ;

  void addToMRUList( PCECacheEntry* PCEtoAdd ) ;
  void removeFromMRUList( PCECacheEntry* PCEtoRem ) ;

  void  setInitialMaxCacheSize()
  {
     maxSize_  = 2000000 ;
     const char * envPCCsize = getenv("PCEC_CACHE_SIZE");
     if (envPCCsize)
        maxSize_ = atoi( envPCCsize );
  } ;

  void logPCCEvent( Int32 eventType, PCECacheEntry * PCEptr, char * sqlStmt );

#endif /* OPT_PCC_DEBUG==1 */

 private:
  NAHeap        * heap_ ;    // heap to use for memory allocations
  PCECacheEntry * createOrderHead_  ;
  PCECacheEntry * createOrderTail_  ;
  PCECacheEntry * MRUHead_          ;
  PCECacheEntry * MRUTail_          ;
  PCECacheEntry * lastMatchedEntry_ ;
  UInt64          numLookups_ ; // Number of searches done
  UInt64          totSrchd_   ; // Number of Entries examined in all searches
  UInt64          numSrchd_   ; // Number of Entries examined in successful searches
  UInt64          numHits_    ; // Number of cache hits (total)
  UInt64          numNEHits_  ; // Number of cache hits where we use Native Expr.
  UInt64          maxHits_    ; // Max hits of any cache entry
  UInt64          maxHitsDel_ ; // Max hits of any kicked out entry
  UInt64          totByCfC_   ; // Total bytes copied from Cache
  UInt32          numEntries_ ; // Number of cache entries
  UInt32          currSize_   ; // Current total size of cached byte streams
  UInt32          maxSize_    ; // Maximum total size allowed (see CQD)
  UInt32     maxOptPCodeSize_ ; // Maximum optimized PCode byte stream length
  Int32   PCECLoggingEnabled_ ;
  Int32   PCECHeaderWritten_  ;
#if OPT_PCC_DEBUG==1
  Int64       totalSavedTime_ ;
  Int64       totalSearchTime_;
  Int64       totalOptTime_   ;
  Int64       totalNEgenTime_ ;
  UInt64      uniqueCtr_      ;
  UInt64      fileNameTime_   ;
  UInt32      fileNamePid_    ;
  char      * logDirPath_     ;
#endif /* OPT_PCC_DEBUG==1 */
};

#endif /* PCODEEXPRCACHE_H */
