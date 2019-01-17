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
#ifndef QUERYCACHE__H
#define QUERYCACHE__H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         QCache.h
 * Description:  
 *               
 *               
 * Created:      07/31/2000
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Collections.h"
#include "NAString.h"
#include "ItemColRef.h"
#include "BindWA.h"
#include "DefaultConstants.h"
#include "CmpMessage.h"
#include "ComSysUtils.h"
#include "Generator.h"
#include "CmpMain.h"
#include "QueryCacheSt.h"

class Key;
class CacheKey;
class TextKey;
class CData;
class CacheData;
class TextData;
struct KeyDataPair;
class CompilerEnv;
struct CacheEntry;
class NAType;
class QueryCache;
class ConstantParameter;
class ConstantParameters;
class SelParameters;
class HQCParseKey;
class HQCCacheKey;
class HQCCacheEntry;
class HQCCacheData;

#define QCache CmpQCache
typedef NAArray<CacheEntry*> TextPtrArray;
typedef NAHashDictionary<CacheKey,CacheEntry> CacheHashTbl;
typedef NAHashDictionaryIterator<CacheKey,CacheEntry> CacheHashTblIterator;
typedef NAHashDictionary<TextKey,CacheEntry> TextHashTbl;
typedef NAHashDictionary<HQCCacheKey, HQCCacheData> HQCHashTbl; //hash table for Hybrid Query Cache
typedef NAHashDictionaryIterator<HQCCacheKey, HQCCacheData> HQCHashTblItor; //hash table Iterator for Hybrid Query Cache

typedef NAHeap NABoundedHeap;
typedef CacheData*  CacheDataPtr;
typedef CacheEntry* CacheEntryPtr;
typedef TextData*   TextDataPtr;

ULng32 getDefaultInK(const Int32& key);

class CQDefault : public NABasicObject {
  friend class CQDefaultSet;
  friend class CompilerEnv;
  NAString attr; // a control query default attribute
  NAString value;// a CQD value

 public:
  // constructor
  CQDefault(const NAString& a, const NAString& v, NAHeap *h) 
    : attr(a,h), value(v,h) {}

  // copy constructor
  CQDefault(const CQDefault& s, NAHeap* h) 
    : attr(s.attr,h), value(s.value,h) {}

  // destructor 
  virtual ~CQDefault() {} // implicitly calls data members' destructors

  // equality comparison used by CompilerEnv::isEqual
  NABoolean isEqual(const CQDefault &o) const 
    { return attr == o.attr && value == o.value; }

  // return byte size of this CQDefault
  ULng32 getByteSize() const 
    { return sizeof(*this) + 
             attr.getAllocatedSize() + value.getAllocatedSize(); }
};
typedef CQDefault* CQDefPtr;

class CQDefaultSet : public NABasicObject {
  friend class CompilerEnv;
  CQDefPtr* CQDarray; // array of pointers to control query default settings
  Int32       nEntries; // number of elements in CQDarray
  Int32       arrSiz;   // allocated size of CQDarray
  NAHeap*   heap;     // heap used to allocate CQD settings

 public:
  // constructor for control query default settings
  CQDefaultSet(Int32 n, NAHeap *h);

  // constructor for control query default settings
  CQDefaultSet(const CQDefaultSet& s, NAHeap* h);

  // destructor frees all control query default settings
  virtual ~CQDefaultSet();

  // add a control query default to CQDarray
  void addCQD(CQDefPtr cqd);

  // return byte size of this CQDefaultSet
  ULng32 getByteSize() const;

  // comparison method for sorting & searching CQDarray
  static Int32 Compare(const void *d1, const void *d2); 
};

class CtrlTblOpt : public NABasicObject {
  friend class CtrlTblSet;
  friend class CompilerEnv;
  NAString tblNam; // table name or '*'
  NAString attr;   // a control table attribute
  NAString value;  // a control table setting

 public:
  // constructor
  CtrlTblOpt(const NAString& t, const NAString& a, const NAString& v, 
             NAHeap *h) : tblNam(t,h), attr(a,h), value(v,h) {}

  // copy constructor
  CtrlTblOpt(const CtrlTblOpt& s, NAHeap* h)
    : tblNam(s.tblNam,h), attr(s.attr,h), value(s.value,h) {}

  // destructor 
  virtual ~CtrlTblOpt() {} // implicitly calls data members' destructors

  // equality comparison used by CompilerEnv::isEqual
  NABoolean isEqual(const CtrlTblOpt &o) const 
    { return tblNam == o.tblNam && attr == o.attr && value == o.value; }

  // return byte size of this CtrlTblOpt
  ULng32 getByteSize() const 
    { return sizeof(*this) + tblNam.getAllocatedSize() + 
             attr.getAllocatedSize() + value.getAllocatedSize(); 
    }
};
typedef CtrlTblOpt* CtrlTblPtr;

class CtrlTblSet : public NABasicObject {
  friend class CompilerEnv;
  CtrlTblPtr* CTarray;  // array of pointers to control table settings
  Int32         nEntries; // number of elements in CTarray
  Int32         arrSiz;   // allocated size of CTarray
  NAHeap*     heap;     // heap used to allocate control table settings

 public:
  // constructor for control table settings
  CtrlTblSet(Int32 n, NAHeap *h);

  // copy constructor for control table settings
  CtrlTblSet(const CtrlTblSet& s, NAHeap *h); 

  // destructor frees all control table settings
  virtual ~CtrlTblSet();

  // add a control table setting to CTarray
  void addCT(CtrlTblPtr ct);

  // return byte size of this CtrlTblSet
  ULng32 getByteSize() const;

  // comparison method for sorting & searching CTarray
  static Int32 Compare(const void *t1, const void *t2); 
};


// CompilerEnv represents changes to the query compiler's environment that
// can affect query plan quality and correctness. These include:
//   1) optimization level and other "important" control query defaults
//   2) control table settings
//   3) query statement attributes
//   4) sql parser flags
// We don't represent set defines and set envvars as part of CompilerEnv
// because 
//   1) set define cannot change during the lifetime of an mxcmp -- you
//      have to kill and restart mxcmp for a new set define to stick
//   2) set envvar is undocumented and should never be used by customers.

class CompilerEnv : public NABasicObject {
 public:
  // constructor captures query compiler environment for current query
  CompilerEnv(NAHeap *h, CmpPhase phase, const QryStmtAttributeSet& attrs);

  // copy constructor transfers memory ownership to heap h
  CompilerEnv(const CompilerEnv &s, NAHeap *h);

  // free our allocated memory 
  virtual ~CompilerEnv();

  // return byte size of this CompilerEnv
  ULng32 getSize() const;

  // returns TRUE if cached plan's environment is better or as good as other's
  NABoolean isEqual(const CompilerEnv &other, CmpPhase phase) const;

  // compute hash address of this CompilerEnv
  ULng32 hashKey() const;

  // am I safe to hash?
  NABoolean amSafeToHash() const;

  enum OptLevel { 
    OPT_MINIMUM = 1, OPT_MEDIUM_LOW, OPT_MEDIUM, OPT_MAXIMUM, OPT_UNDEFINED
  };

  // convert DefaultToken optimization level into OptLevel
  static OptLevel defToken2OptLevel(DefaultToken tok);

  Int32 getOptLvl() const { return optLvl_; }
  const char *getCatalog() const { return cat_.data(); }
  const char *getSchema() const { return schema_.data(); }

 private:
  OptLevel      optLvl_;
  NAString      cat_;    // catalog name
  NAString      schema_; // schema name
  CQDefaultSet *CQDset_; // control query default settings
  CtrlTblSet   *CTset_;  // control table settings
  NAHeap       *heap_;   // heap to use for memory allocations
  TransMode    *tmode_;  // NULL or context-wide TransMode for preparser entry

  QryStmtAttributeSet attrs_; // has statement attributes & sqlParserFlags
  // used by regress/MVS/TESTMV700. 2 queries that differ in their context 
  // sqlParserFlags must have 2 cache entries.

  // Set compiler environment's optimization level
  void setOptimizationLevel(DefaultToken optLvl);

  Int32 CQDcnt() const { return CQDset_ ? CQDset_->nEntries : 0; }
  Int32 CTcnt() const { return CTset_ ? CTset_->nEntries : 0; }
};

// ParameterTypeList is used to represent the type signature of a cache entry
class ParameterTypeList : public LIST(ParamType) 
{
 public:
  // create type list from ConstantParameters
  ParameterTypeList(ConstantParameters *p, NAHeap *h);

  // return a copy of list s using heap h
  ParameterTypeList(const ParameterTypeList& s, NAHeap *h);

  // free our allocated memory 
  virtual ~ParameterTypeList();

  // return our contribution to CacheKey::hashKey
  ULng32 hashKey() const;
  NABoolean amSafeToHash() const;

  // return our contribution to CacheKey::getSize & CacheData::getSize
  ULng32 getSize() const;

  // return true if two ParameterTypeLists are equal
  NABoolean operator==(const ParameterTypeList& other) const;

  // deposit parameter types string form into parameterTypes
  void depositParameterTypes(NAString*) const;

 private:
  NAHeap *heap_; // NULL or heap used for this class
};

// SelParamTypeList is used to represent the type signature of a cache entry
class SelParamTypeList : public LIST(SelParamType) {
 public:
  // create type list from SelParameters
  SelParamTypeList(SelParameters *p, NAHeap *h);

  // return a copy of list s using heap h
  SelParamTypeList(const SelParamTypeList& s, NAHeap *h);

  // free our allocated memory 
  virtual ~SelParamTypeList();

  // return our contribution to CacheKey::hashKey
  ULng32 hashKey() const;
  NABoolean amSafeToHash() const;

  // return our contribution to CacheKey::getSize & CacheData::getSize
  ULng32 getSize() const;

  // return true if two SelParamTypeLists are equal on both types 
  // and selectivities
  NABoolean operator==(const SelParamTypeList& other) const;

  // This compares param's types only
  NABoolean compareParamTypes(const SelParamTypeList& other) const;

  // This compares param's selectivities only
  NABoolean compareSelectivities(const SelParamTypeList& other) const;

  // deposit parameter types string form into parameterTypes
  void depositParameterTypes(NAString*) const;

 private:
  NAHeap *heap_; // NULL or heap used for this class
};

// Key is the base class for CacheKey and TextKey
class Key : public NABasicObject {
 public:
  // Constructor
  Key(CmpPhase phase, CompilerEnv* e, NAHeap *h);

  // copy constructor
  Key(const Key &s, NAHeap *h);

  // Destructor
  virtual ~Key();

  // return hash value of a Key
  ULng32 hashKey() const; 

  // am I safe to hash?
  NABoolean amSafeToHash() const;

  // equality operator
  NABoolean isEqual(const Key &other) const;

  // identify myself 
  enum KeyType { KEY, CACHEKEY, TEXTKEY, HYBRIDKEY };
  virtual KeyType am() = 0;
  ULng32 hash() const {return hashKey(); }


  // accessors
  CmpPhase getCmpPhase() const { return phase_; }
  Int32 getOptLvl() const;
  const char *getCatalog() const;
  const char *getSchema() const;
  NAHeap* heap() const { return heap_; }
  CompilerEnv* env() const { return env_; }
  virtual Int32 getNofParameters() const { return 0; }
  virtual const char *getParameterTypes(NAString*) const { return ""; }
  virtual const char *getReqdShape() const { return ""; }
  virtual TransMode::IsolationLevel getIsoLvl() 
    { return TransMode::IL_NOT_SPECIFIED_; }
  virtual TransMode::AccessMode getAccessMode() 
    { return TransMode::AM_NOT_SPECIFIED_; }

  virtual TransMode::IsolationLevel getIsoLvlForUpdates() 
    { return TransMode::IL_NOT_SPECIFIED_; }

  virtual TransMode::AutoCommit getAutoCommit() 
    { return TransMode::AC_NOT_SPECIFIED_; }
  virtual Int16 getFlags() { return 0; }
  virtual TransMode::RollbackMode getRollbackMode() 
    { return TransMode::ROLLBACK_MODE_NOT_SPECIFIED_; }

  // return byte size of this Key
  virtual ULng32 getSize() const;

 protected:
  // key has several components
  CmpPhase     phase_; // enum CompilerPhase
  CompilerEnv *env_;   // compiler environment
  NAHeap      *heap_;  // heap used by stmt_
};

struct HQCDParamPair
{
  HQCDParamPair(){}
  HQCDParamPair(NAString & ori, NAString & nor)
  : original_(ori), normalized_(nor)
  {}
  HQCDParamPair(const char* cori, const char* cnor)
  : original_(cori), normalized_(cnor)
  {}
  NAString original_;
  NAString normalized_;
};

class hqcTerm : public NABasicObject {
public:
  virtual NABoolean isLookupHistRequired() const  = 0;
};

struct HistIntRangeForHQC
{
  HistIntRangeForHQC(const EncodedValue & hiBound, const EncodedValue & loBound, NABoolean hiinc, NABoolean loinc, NAHeap* h)
      : hiBound_(hiBound, h)
      , loBound_(loBound, h)
      , hiInclusive_(hiinc)
      , loInclusive_(loinc)
      {}
  HistIntRangeForHQC(HistIntRangeForHQC& other, NAHeap* h)
      : hiBound_(other.hiBound_, h)
      , loBound_(other.loBound_, h)
      , hiInclusive_(other.hiInclusive_)
      , loInclusive_(other.loInclusive_)
      {}
  //if this range contains a specific EncodedValue
  NABoolean constains(const EncodedValue & value) const;

  EncodedValue hiBound_;
  EncodedValue loBound_;
  NABoolean hiInclusive_;
  NABoolean loInclusive_;
};

class hqcConstant : public hqcTerm {
public:
    enum Flags {

      IS_LOOKUP_HISTOGRAM_REQUIRED = 0x1,

      IS_PARAMETERIZED  = 0x4,

      IS_PROCESSED = 0x8
    };
      
    hqcConstant(ConstValue* cv, Int32 index, NAHeap* h);

    hqcConstant(const hqcConstant & other, NAHeap* h);

    ~hqcConstant();

    virtual NABoolean isLookupHistRequired() const { return flags_ & IS_LOOKUP_HISTOGRAM_REQUIRED; }

    void setIsLookupHistRequired( NABoolean b ) { b ? flags_ |= IS_LOOKUP_HISTOGRAM_REQUIRED : flags_ &= ~IS_LOOKUP_HISTOGRAM_REQUIRED; }

    ConstValue* & getConstValue(){ return constValue_; }
    
    Int32 & getIndex()  { return index_; }

    HistIntRangeForHQC * getRange() { return histRange_; }
    
    void addRange(const EncodedValue & hiBound, const EncodedValue & loBound, NABoolean hiinc, NABoolean loinc);

    NABoolean isApproximatelyEqualTo(hqcConstant & other) ;

    void setIsParameterized(NABoolean b)
    {
        b ? flags_ |= IS_PARAMETERIZED : flags_ &= ~IS_PARAMETERIZED;
    }

    NABoolean isParameterized()  const { return flags_ & IS_PARAMETERIZED; }
    
    void setProcessed() { flags_ |= IS_PROCESSED; }

    NABoolean isProcessed()  const { return flags_ & IS_PROCESSED; }
    
    const NAType* getSQCType() const { return SQCType_; }

    void setSQCType(NAType* t) { SQCType_ = t; }

    ConstValue * getBinderRetConstVal() const 
      { return binderRetConstVal_; }

    void setBinderRetConstVal(ConstValue * v)
      { binderRetConstVal_ = v; }
    
protected:
  
  ConstValue* constValue_;

  //binder returned ConstValue
  ConstValue* binderRetConstVal_;
  
  NAType *SQCType_; //normalized type;

  HistIntRangeForHQC * histRange_;
  
  Int32 index_;

  NAHeap* heap_;

  UInt32 flags_;
  //disable assignment operator
  hqcConstant & operator=(const hqcConstant & other);
  //disable standard copy constructor
  hqcConstant(const hqcConstant & other);

};

class hqcDynParam : public hqcTerm {
public:
    hqcDynParam(DynamicParam* dm, Int32 idx, const NAString& normalizedName, const NAString& originalName)
      : dynParam_(dm)
      , normalizedName_(normalizedName)
      , originalName_(originalName)
      , index_(idx)
    {}
    hqcDynParam(const hqcDynParam & other)
      : dynParam_(other.dynParam_)
      , normalizedName_(other.normalizedName_.data())
      , originalName_(other.originalName_.data())
      , index_(other.index_)
    {}
     virtual NABoolean isLookupHistRequired()  const { return FALSE; }

      DynamicParam* & getDynParam() { return dynParam_;}

      NAString & getNormalizedName() { return normalizedName_;}
      NAString & getOriginalName() { return originalName_;}
      Int32 & getIndex() {return index_;}
      
protected:
  DynamicParam* dynParam_;
  NAString normalizedName_;
  NAString originalName_;
  Int32 index_;
};

class HQCParams : public NABasicObject
{
 public:
   HQCParams (NAHeap* h);

   // copy constructor
   HQCParams (const HQCParams & other, NAHeap* h);

   ~HQCParams ();

    const NAList <hqcConstant*> & getConstantList() const { return ConstantList_;}
    NAList <hqcConstant*> & getConstantList() { return ConstantList_;}
    
    const NAList <hqcDynParam*> & getDynParamList() const { return DynParamList_;}
    NAList <hqcDynParam*> & getDynParamList() { return DynParamList_;}

    const NAList <NAString> & getNPLiterals() const { return NPLiterals_;}
    NAList <NAString> & getNPLiterals() { return NPLiterals_; }

    const NAList <hqcConstant* > * getTmpList() const { return tmpList_;}
    NAList <hqcConstant* > * getTmpList() { return tmpList_; }
    
   // Compare whether each constant in this falls within the correspoinding
   // histogram interval boundaries specified in other.
   NABoolean isApproximatelyEqualTo(HQCParams& other) ;

 private:
   NAHeap      *heap_;
   NAList <hqcConstant * > ConstantList_;
   NAList <hqcDynParam * > DynParamList_;
   NAList <NAString> NPLiterals_;
   NAList <hqcConstant* > * tmpList_;
   //disable operator= for this class
   HQCParams & operator=(const HQCParams & other);
   //disable standard copy constructor
   HQCParams (const HQCParams & other);
};

// the HQC key that is stored in the HQC cache
class HQCCacheKey : public Key 
{
 public:
       HQCCacheKey (CompilerEnv* e, NAHeap* h);
       
       HQCCacheKey(const HQCParseKey & hkey, NAHeap* h);

	     //copy constructor
       HQCCacheKey(const HQCCacheKey & other, NAHeap* h)
          : Key(other, h)
          , keyText_(other.keyText_.data(), h)
          , reqdShape_(other.reqdShape_, h)
          , numConsts_(other.numConsts_)
          , numDynParams_(other.numDynParams_)
        {}

       ~HQCCacheKey() {}
       const char* getText() const {return keyText_.data(); }
       NAString& getKey() {return keyText_;}
       ULng32 hashKey() const
       {
          ULng32 hval = Key::hashKey() + keyText_.hash();
          return hval;
       }

       // only compare the parent class and the keyText_.
       virtual NABoolean operator==(const HQCCacheKey &other) const ;
       
       //identify as HYBRIDKEY
       virtual KeyType am() { return HYBRIDKEY; } 

       virtual const char *getReqdShape() const { return reqdShape_.data(); }

       Int32 getNumConsts() const { return numConsts_; }
       
 protected:
       NAString  keyText_;
       NAString  reqdShape_;    // control query shape or empty string 

       // number of constants,
       // its value valid only in a compile cycle,
       //not valid in static senario, i.e. one HQCCacheKey maps to multiple HQCCacheEntry in hashtable
       Int32     numConsts_;    
       Int32     numDynParams_; // number of dynamic parameters 
       //disable standard copy constructor
       HQCCacheKey(const HQCCacheKey& other);
       //disable operator=() for this class
       HQCCacheKey & operator=(const HQCCacheKey & other); 
};

// HQC key that is used during binding to decide on HQC cacheability
class HQCParseKey : public HQCCacheKey 
{
 public:
       HQCParseKey(CompilerEnv* e, NAHeap* h);
	
	     //copy constructor
	    HQCParseKey(const HQCParseKey& other, NAHeap* h)
            : HQCCacheKey(other, h)
            , params_ (other.params_, h)
            , isCacheable_ (other.isCacheable_)
            , nOfTokens_(other.nOfTokens_)
            , isStringNormalized_(other.isStringNormalized_)
            , paramStart_(other.paramStart_)
	    , HQCDynParamMap_(other.HQCDynParamMap_, h)
	     {}
	
       ~HQCParseKey() {}
	
       const HQCParams & getParams () const { return params_; };
       
       HQCParams & getParams() { return params_; }

       // Compare whether each constant in this falls within the correspoinding
       // histogram interval boundaries specified in other.
       NABoolean isApproximatelyEqualTo(const HQCParseKey& other) const;

       NABoolean isCacheable() const { return isCacheable_;}
       void setIsCacheable(NABoolean b) { isCacheable_ = b;}

       void verifyCacheability(CacheKey* ckey);
    
       void addTokenToNormalizedString(Int32 & tokCod);

       inline NAString & getNormalizedQueryString()
       {
           if(keyText_.isNull())
	           return keyText_;
           if(!isStringNormalized_)
           {
               keyText_.toUpper8859_1();
               isStringNormalized_ = TRUE;
           }
           return keyText_;
       }

       void collectItem4HQC(ItemExpr* itm);

       void collectBinderRetConstVal4HQC(ConstValue* origin, ConstValue* after);

       void bindConstant2SQC(BaseColumn* base, ConstantParameter* cParameter, LIST(Int32)& hqcConstPos);
       
       void FixupForUnaryNegate(BiArith* itm);

       
  private:
       HQCParams params_;
       LIST(HQCDParamPair) HQCDynParamMap_;
   
       NABoolean isCacheable_;
       Lng32 nOfTokens_;
       NABoolean isStringNormalized_;
       Int32 paramStart_;
       //disable standard copy constructor
       HQCParseKey(const HQCParseKey& other);
};

// HQC individual values stored with an HQC key. A given HQC key
// can be associated with a list of values
class HQCCacheEntry : public NABasicObject
{
 public:
       HQCCacheEntry (NAHeap* h);

       HQCCacheEntry (const HQCParams& params, CacheKey* sqcCacheKey, NAHeap* h);

        //copy constructor
        HQCCacheEntry(const HQCCacheEntry& other, NAHeap* h)
            : heap_(h)
            , params_(other.params_ ? new (h) HQCParams (*(other.params_), h) : NULL)
            , sqcCacheKey_(other.sqcCacheKey_)
            , numHits_(other.numHits_)   
        {}

       virtual ~HQCCacheEntry();

       void incrementNumHits() { numHits_++; }
       Int32 getNumHits() const { return numHits_; }

       HQCParams* getParams () const { return params_; }
       CacheKey*  getSQCKey()  const { return sqcCacheKey_; }

       // only compare the parent class and the keyText_.
       virtual NABoolean operator==(const HQCCacheEntry &other) const ;

 private:
       NAHeap      *heap_;
       HQCParams   *params_;
       CacheKey    *sqcCacheKey_;
       Int32       numHits_;
       //disable standard copy constructor
       HQCCacheEntry(const HQCCacheEntry& other);
};

// HQC list of values that is stored in HQC with every HQC key
class HQCCacheData : public LIST (HQCCacheEntry*)
{
 public:
   HQCCacheData (NAHeap* h, CollIndex maxEntries=5)
      : LIST(HQCCacheEntry*) (h)
      , heap_(h)
      , maxEntries_ (maxEntries)
  {}

  HQCCacheData (const HQCCacheData &otherList , NAHeap* h)
      : LIST(HQCCacheEntry*)(otherList, h)
      , heap_(h)
      , maxEntries_(otherList.maxEntries_)
  {}

  ~HQCCacheData () {}

  NABoolean isFull () { return (maxEntries_ == entries ()); }

  NABoolean addOrReplace ();

  CacheKey* findMatchingSQCKey (HQCParams& param);

  NABoolean removeEntry (CacheKey* sqcCacheKey);

 private:
   NAHeap* heap_;
   Int32 maxEntries_;
   //disable standard copy constructor
   HQCCacheData (const HQCCacheData &otherList);
};


class HybridQCache : public NABasicObject
{
public:
  HybridQCache(QueryCache & qc, ULng32 nOfBuckets);

  ~HybridQCache();
  
  //this method and above should be called in pair.
  NABoolean addEntry(HQCParseKey* hkey, CacheKey* ckey);
  
  NABoolean lookUp(HQCParseKey * hkey, CacheKey* & ckey);

  NABoolean delEntryWithCacheKey(CacheKey* key);
  
  void clear();

  ostream* getHQCLogFile() { return HQCLogFile_; };

  void initLogging();
  
  void invalidateLogging();

  HybridQCache* resizeCache(ULng32 numBuckets);

  void setMaxEntries(ULng32 v) { maxValuesPerKey_ = v; }

  ULng32 getMaxEntriesPerKey() const { return maxValuesPerKey_; }

  ULng32 getNumBuckets() const { return hashTbl_->getNumBuckets(); }

  ULng32 getEntries() const { return hashTbl_->entries(); }
  
  ULng32 getNumSQCKeys() const;

  //return a iterator pointing to begin.
  HQCHashTblItor begin() { HQCHashTblItor iterator(*hashTbl_); return iterator; }

  NABoolean isPlanNoAQROrHiveAccess() const { return planNoAQROrHiveAccess_; }

  void setPlanNoAQROrHiveAccess( NABoolean b ) { planNoAQROrHiveAccess_ = b; }

  HQCParseKey * getCurrKey() const { return currentKey_; }

  void setCurrKey( HQCParseKey* key ) { currentKey_ = key; }

  void collectBinderRetConstVal4HQC(ConstValue* origin, ConstValue* after)
    { if ( currentKey_ ) currentKey_->collectBinderRetConstVal4HQC(origin, after); }
  
private:
  QueryCache & querycache_; 
  NAHeap* heap_;
  HQCHashTbl* hashTbl_;
  ULng32  maxValuesPerKey_;
  HQCParseKey * currentKey_;
  ofstream* HQCLogFile_;
  //if the plan is found in cache but AQR disabled, or accessing Hive
  //set true, do not use the plan and  do not lookup cache after bind, 
  //and do not add to cache again
  NABoolean planNoAQROrHiveAccess_;
    
  HybridQCache(const HybridQCache & other);
  HybridQCache & operator=(const HybridQCache & other);
  //remove a <HQCCacheKey*, HQCCacheData*> pair 
  //in HQC hash table.
  void deCache(HQCCacheKey * hkey)
  {  //hkey must point to internal HQCParseKey
      hashTbl_->remove(hkey);
      NADELETE(hkey, HQCCacheKey, heap_);
  }
  
};

struct HybridQueryCacheStats {
  ULng32 nHKeys;
  ULng32 nSKeys;
  ULng32 nMaxValuesPerKey;
  ULng32 nHashTableBuckets;
};

struct HybridQueryCacheDetails 
{
    Int64     planId;
    NAString hkeyTxt;
    NAString skeyTxt;
    ULng32 nHits;
    ULng32 nOfPConst;
    NAString PConst;
    ULng32 nOfNPConst;
    NAString NPConst;
};


// CacheKey is the key used for searching the "after parser" & "after binder" 
// stages of the cache.
class CacheKey : public Key {
 public:
  // Constructor
  CacheKey(NAString &stmt, CmpPhase phase, CompilerEnv* e,
           const ParameterTypeList& p, 
           const SelParamTypeList& s, NAHeap *h, NAString &cqs,
           TransMode::IsolationLevel l, TransMode::AccessMode m,
           TransMode::IsolationLevel lForUpdates, 
           TransMode::AutoCommit a, Int16 f, TransMode::RollbackMode r,
           LIST(NATable*) &tables, NABoolean useView);

  // copy constructor
  CacheKey(CacheKey &s, NAHeap *h);

  // Destructor
  virtual ~CacheKey();

  // identify myself 
  virtual KeyType am() { return CACHEKEY; };

  // return hash value of a CacheKey
  ULng32 hashKey() const; 

  // am I safe to hash?
  NABoolean amSafeToHash() const;

  // equality operator
  NABoolean operator ==(const CacheKey &other) const;

  // is string s found in stmt_?
  NABoolean contains(const NAString& s) const { return stmt_.contains(s); }
  NABoolean contains(const char *s) const { return stmt_.contains(s); }

  // accessors
  NABoolean useView() const { return useView_; }
  virtual const char *getReqdShape() const { return reqdShape_.data(); }
  const char *getText() const { return stmt_.data(); }

  virtual Int32 getNofParameters() const 
    { return Int32(actuals_.entries()+sels_.entries()); }

  virtual const char *getParameterTypes(NAString*) const;

  // return byte size of this CacheKey
  virtual ULng32 getSize() const;

  virtual TransMode::IsolationLevel getIsoLvl() { return isoLvl_; }
  virtual TransMode::AccessMode getAccessMode() { return accMode_; }

  virtual TransMode::IsolationLevel getIsoLvlForUpdates() { return isoLvlIDU_; }

  virtual TransMode::AutoCommit getAutoCommit() { return autoCmt_; }
  virtual Int16 getFlags() { return flags_; }
  virtual TransMode::RollbackMode getRollbackMode() { return rbackMode_; }

  // mutate function
  void setCompareSelectivity(NABoolean x) { compareSelectivity_ = x; };

  // update referenced tables' histograms' timestamps
  void updateStatsTimes( LIST(NATable*) &tables );

  void setPlanId(Int64 id) { planId_ = id; }

  Int64 getPlanId() const { return planId_; }
  
 private:
  // key has several components
  NAString     stmt_;  // normalized query statement text
  ParameterTypeList actuals_; // list of Constant Parameter types
  SelParamTypeList  sels_;    // list of SelParameter types
  NAString     reqdShape_; // control query shape or empty string

  TransMode::IsolationLevel isoLvl_;   // tx isolation level
  TransMode::AccessMode     accMode_;  // tx access mode
  TransMode::IsolationLevel isoLvlIDU_;   // tx isolation level for updates

  TransMode::AutoCommit     autoCmt_;  // tx auto-commit
  Int16                     flags_;    // tx flags
  TransMode::RollbackMode   rbackMode_;// tx rollback mode

  NABoolean compareSelectivity_; // whether to compare the selectivity 
                                 // for sels_

  // list of referenced tables' histograms' timestamp
  // used for "passive invalidation" of oached plans 
  LIST(Int64)           updateStatsTime_; 

  NABoolean             useView_; 
  // must be set to true for cacheable query that references a view.
  // used to suppress text caching of such a query to avoid subverting
  // revocation of privilege on referenced view(s).
  //keep planId in SQC Cache key
  //planID can serve as connection between HQC entries virtual table and SQC entries virtual table.
  Int64 planId_;
};

// TextKey is the key used for searching the "pre parser" stage of the cache.
class TextKey : public Key {
 public:
  // Constructor
  TextKey(const char *sText, CompilerEnv* e, NAHeap *h, Lng32 cSet);

  // copy constructor
  TextKey(TextKey &s, NAHeap *h);

  // Destructor
  virtual ~TextKey();

  // identify myself 
  virtual KeyType am() { return TEXTKEY; };

  // return hash value of a TextKey
  ULng32 hashKey() const; 

  // equality operator
  NABoolean operator ==(const TextKey &other) const;

  // accessors
  const char *getText() const { return sText_.data(); }

  // am I safe to hash?
  NABoolean amSafeToHash() const;

  // return byte size of this TextKey
  virtual ULng32 getSize() const;

 private:
  // key has one component
  NAString     sText_; // sql statement text for pre-parser cache lookups
  Lng32         charset_; // character set of sql statement text
};

class Plan {

public:

  Plan(char* plan, ULng32 planLen, Int64 planId, NAHeap    *h) :
    plan_(plan), planL_(planLen), planId_(planId), heap_(h), refCount_(0),
    visits_(0) {};

  Plan(Generator* gen, Int64 planId, NAHeap    *h) :
    plan_((char*)gen), planL_(0), planId_(planId), heap_(h), refCount_(0),
    visits_(0) {};

  Plan(Plan& p, NAHeap* h);

  ~Plan() 
  {  
     if (planL_ > 0) NADELETEBASIC(plan_,heap_); 
  };

  // how many times this plan is reused (shared) in the template cache.
  // The ref. count is increased whenever the plan is shared by another
  // and decreased when an entry is bumped out of the cache. 
  virtual Int32 getRefCount() { return refCount_; };
  virtual void incRefCount() { refCount_++; };
  virtual void decRefCount() { refCount_--; };

  virtual Int64 getId() const { return planId_; }
  virtual char *getPlan() const { return plan_; }
  virtual ULng32 getPlanLen() const 
    {return (planL_ == 0) ? ((Generator *)plan_)->getFinalObjLength() : planL_;}

  virtual NABoolean inGenerator() const  { return planL_ == 0; }

  // the total size of the Plan object plus the actual compiled plan
  virtual ULng32 getSize() const { return getPlanLen() + sizeof(*this); };
  

  Int32 getVisits() { return visits_; };
  void visitOnce() { visits_++; };
  void resetVisits() { visits_ = 0; };

private:
  char *plan_;  // compiled query plan in packed form
  ULng32 planL_; // byte length of plan_
  Int64             planId_;  // from Generator
  NAHeap*   heap_;    // heap used by dynamic allocs

  Int32 refCount_; // reference count

  Int32 visits_;  // # of visits. Used by QCache::canFit()
};


// CData is the base class for CacheData and TextData
class CData : public NABasicObject {
 public:
  // Constructor
  CData
   (NAHeap *h); // (IN) : heap to use for dynamic allocs

  // copy constructor
  CData
  (CData  &s,  // (IN) : assignment source
   NAHeap *h); // (IN) : heap to use for copying s

  // Destructor
  virtual ~CData();

  // accessors 
  virtual const char *getOrigStmt() const { return NULL; }
  virtual Plan *getPlan() const { return NULL; }

  ULng32 getHits() const { return hits_; }
  NAHeap* heap() const { return heap_; }

  ULng32 getCompTime() const { return (ULng32)compTime_; }
  ULng32 getAvgHitTime() const 
    { return hits_ ? (ULng32)(cumHitTime_/hits_) : 0; } 


  // mutators
  void incHits() { hits_++; }


  // tally hit time
  virtual void addHitTime(TimeVal& begTime);

  // compute this entry's compile time (in msec)
  void setCompTime(TimeVal& begTime);

  // return elapsed msec since begTime
  static Int64 timeSince(TimeVal& begTime);

  // return byte size of this CacheData
  virtual ULng32 getSize() const { return 0; }

 protected:
  // data
  NAHeap*   heap_;    // heap used by dynamic allocs

 private:
  // usage
  ULng32 hits_;     // number of hits for this entry
  Int64     compTime_;  // msec to compile this entry
  Int64     cumHitTime_;// cum. time of hits for this entry

};

class CacheData : public CData {
 public:
  // Constructor
  CacheData
   (Generator *plan,           // (IN) : access to sql query's compiled plan 
    const ParameterTypeList& f,// (IN) : list of formal ParameterTypes
    const SelParamTypeList& s, // (IN) : list of formal SelParamTypes
    LIST(Int32) hqcListOfConstParamPos, // (IN) : list of positions of formal params
    LIST(Int32) hqcListOfSelParamPos,   // (IN) : list of positions of sel params
    LIST(Int32) hqcListOfAllConstPos,   // (IN) : list of positions of all params
    Int64 planId,              // (IN) : id from generator
    const char *text,          // (IN) : original sql statement text
    Lng32  cs,                  // (IN) : character set of sql statement text
    NAHeap *h);                // (IN) : heap to use for formals_

  // copy constructor
  CacheData
  (CacheData &s,  // (IN) : assignment source
   NAHeap    *h,
   NABoolean copyPlan = TRUE
  ); // (IN) : heap to use for copying s

  // Destructor
  virtual ~CacheData();

  // accessors 

  virtual Plan* getPlan() const { return plan_; }
  virtual void setPlan(Plan* plan) { plan_ = plan; }

  virtual const char *getOrigStmt() const { return origStmt_; }


  // return byte size of this CacheData
  virtual ULng32 getSize() const; 

  virtual const ParameterTypeList& getParamTypeList() 
  { return formals_; }


  // return byte size of this CacheData's preparser entries
  ULng32 getSizeOfPreParserEntries() const; 

  // This function copies the contents in listOfConstantParameters into 
  // a field of the plan_ called parameterBuffer. To do this, it unpacks
  // parameterBuffer and then generates and evals backpatch expression(s)
  // to do the copy.
  NABoolean backpatchParams
    (const ConstantParameters &listOfConstantParameters, 
     const SelParameters& listOfSelParameters,
     const LIST(Int32)& listOfConstParamPositionsInSql,
     const LIST(Int32)& listOfSelParamPositionsInSql,
     BindWA &bindWA, char* &params, ULng32 &paramSize);

  // HQC backpatch
  NABoolean backpatchParams
    (LIST(hqcConstant *) &listOfConstantParameters,
     LIST(hqcDynParam *) &listOfDynamicParameters,
     BindWA &bindWA, char* &params, ULng32 &parameterBufferSize);

  // copies actuals_ into this CacheData's plan_
  NABoolean backpatchPreParserParams(char *actuals, ULng32 actLen); 

  // allocate and copy plan
  void allocNcopyPlan(NAHeap *h, char **plan, ULng32 *pLen);

  // add backpointer to given preparser cache entry
  void addTextEntry(CacheEntry *entry);

  // return array of backpointers to preparser entries
  TextPtrArray& PreParserEntries() { return textentries_; }

 private:
  // data
  Plan *plan_;  // compiled query plan 

  ParameterTypeList formals_; // list of ParameterTypes
  SelParamTypeList  fSels_;   // list of SelParamTypes
  LIST(Int32) hqcListOfConstParamPos_;  // list of positions of formal params
  LIST(Int32) hqcListOfSelParamPos_;    // list of positions of sel params
  LIST(Int32) hqcListOfConstPos_;       // list of positions of all params
  const char       *origStmt_;// original sql text
  Lng32              charset_; // character set of original sql text
  TextPtrArray      textentries_; 
  // back pointers to preparser instances of this postparser cache entry

  // helper method to unpack parameter buffer part of plan_
  NABoolean unpackParms(NABasicPtr &params, ULng32 &parmSz);

};

struct KeyDataPair {
  Key   *first_;
  CData *second_;
  KeyDataPair(Key* a, CData* b) : first_(a), second_(b) {}
  KeyDataPair() : first_(NULL), second_(NULL) {}
  ~KeyDataPair();
};

struct CacheEntry : public NABasicObject {
  CacheEntry *next_;
  CacheEntry *prev_;
  KeyDataPair data_;

  CacheEntry() : next_(0), prev_(0), data_() {}
  CacheEntry(const KeyDataPair& x) : next_(0), prev_(0), data_(x) {}
  CacheEntry(Key *k, CData *d, CacheEntry *n, CacheEntry *p)
    : next_(n), prev_(p), data_(k,d) {}
  virtual ~CacheEntry() {}
  // operator "==" is required by NAHashDictionary to avoid a compilation
  // error but it never does a "data1==data2" comparison.
  NABoolean operator==(const CacheEntry& other) const { return FALSE; }
};

class TextData : public CData {
 public:
  // Constructor
  TextData
    (NAHeap             *h, // (IN) : heap to use 
     char               *p, // (IN) : actual parameters
     ULng32       l, // (IN) : len of actual parameters
     CacheEntry        *e);// (IN) : postparser cache entry

  // Destructor
  virtual ~TextData();

  // accessors 
  virtual const char *getOrigStmt() const 
    { return PostParserEntry()->getOrigStmt(); }

  virtual Plan *getPlan() const
    { return PostParserEntry()->getPlan(); }

  CacheData *PostParserEntry() const 
    { return (CacheData*)(entry_->data_.second_); }
  CacheEntry *PostParserNode() const { return entry_; }

  // return byte size of this TextData
  virtual ULng32 getSize() const; 

  // copies the contents of actuals_ into a field of post parser entry's
  // plan_ called parameterBuffer. 
  NABoolean backpatchParams();

  // allocate and copy plan
  void allocNcopyPlan(NAHeap *h, char **plan, ULng32 *pLen)
    { PostParserEntry()->allocNcopyPlan(h, plan, pLen); }

  // tally hit time
  virtual void addHitTime(TimeVal& begTime);

  void setIndexInTextEntries(CollIndex x) { indexInTextEntries_ = x; };
  CollIndex getIndexInTextEntries() { return indexInTextEntries_; };


 private:
  // data
  NAHeap        *heap_;    // Needed by TextData::~TextData destructor
  char          *actuals_; // list of actual constant parameters
  ULng32  actLen_;  // length of actuals_
  CacheEntry   *entry_;   // pointer to postparser cache entry
  CollIndex    indexInTextEntries_;
};

class LRUList : public NABasicObject {
 private:
  ULng32 length_; // number of elements in list
  CacheEntry *anchor_; // anchor of doubly linked list
  NAHeap      *heap_;   // used to allocate nodes

 public:
  struct iterator;

 protected:
  // node allocator
  CacheEntry* getNode() { return new(heap_) CacheEntry(); }
  // node deallocator
  void putNode(CacheEntry* p) { 
    NADELETE(p,CacheEntry,heap_); // implicitly calls p.data_.~KeyDataPair()
  }

  // transfer elements [first,last) from list x to this list at pos
  void transfer(iterator pos, iterator first, iterator last, LRUList& x)
    {
      if (this != &x) {
        insert(pos, first, last);
        x.erase(first, last);
      }
      else {
        last.node_->prev_->next_ = pos.node_;
        first.node_->prev_->next_ = last.node_;
        pos.node_->prev_->next_ = first.node_;
        CacheEntry *tmp = pos.node_->prev_;
        pos.node_->prev_ = last.node_->prev_;
        last.node_->prev_ = first.node_->prev_;
        first.node_->prev_ = tmp;
      }
    }

 public:
  // create an empty list
  LRUList(NAHeap *h) : heap_(h), length_(0)
    {
      anchor_ = getNode();
      anchor_->next_ = anchor_;
      anchor_->prev_ = anchor_;
    }

  NABoolean empty() const { return length_ == 0; }
  ULng32 size() const { return length_; }

  struct iterator {
    CacheEntry *node_;
    iterator(CacheEntry *x) : node_(x) {}
    iterator() {}
    NABoolean operator==(const iterator& x) const { return node_ == x.node_; }
    NABoolean operator!=(const iterator& x) const { return node_ != x.node_; }
    KeyDataPair& operator*() const { return node_->data_; }
    iterator& operator++() { node_ = node_->next_; return *this; }
    iterator operator++(Int32) { iterator tmp = *this; ++*this; return tmp; }
    iterator& operator--() { node_ = node_->prev_; return *this; }
    iterator operator--(Int32) { iterator tmp = *this; --*this; return tmp; }
  }; // end of class iterator

  // iterators
  iterator begin() { return anchor_->next_; }
  iterator end() { return anchor_; }
  
  // insert element x at position
  iterator insert(iterator position, KeyDataPair& x)
    { 
      CacheEntry *tmp = getNode();
      tmp->data_ = x;
      tmp->next_ = position.node_;
      tmp->prev_ = position.node_->prev_;
      position.node_->prev_->next_ = tmp;
      position.node_->prev_ = tmp;
      ++length_;
      return tmp;
    }

  // inserts at iterator position pos a copy of all elements [first,last)
  void insert(iterator pos, iterator first, iterator last)
    { while (first != last) insert(pos, *first++); }

  // inserts a copy of element x at the beginning
  iterator pushFront(KeyDataPair& x) { return insert(begin(), x); }

  // removes the element at iterator position pos and 
  // returns the position of the next element
  iterator erase(iterator pos)
    {
      if (pos == end()) return end();
      iterator tmp = (iterator)(pos.node_->next_);
      pos.node_->prev_->next_ = pos.node_->next_;
      pos.node_->next_->prev_ = pos.node_->prev_;
      putNode(pos.node_);
      --length_;
      return tmp;
    }

  // removes all elements of the range [first,last) and
  // returns the position of the next element
  iterator erase(iterator first, iterator last)
    {
      iterator tmp = end();
      while (first != last) tmp = erase(first++);
      return tmp;
    }

  // removes all elements (makes the container empty)
  void clear() { erase(begin(), end()); }

  // moves the element at c2pos in c2 in front of pos of this list
  // (this and c2 may be identical)
  void splice(iterator pos, LRUList& c2, iterator c2pos)
    {
      iterator k = c2pos;
      if (k != pos && ++k != pos) {
        iterator j = c2pos;
        transfer(pos, c2pos, ++j, c2);
      }
    }
}; // end of class LRUList

// The mxcmp cache has 3 stages: before PARSE, after PARSE, after BIND
const Int32 N_STAGES=3;

// QCache is a helper class that implements the mxcmp query cache
class QCache : public NABasicObject {
 public:
  // Constructor
  QCache
    (QueryCache & qc,      // (IN) : reference to its wrapper
     ULng32 maxSize,       // (IN) : maximum heap size in bytes
     ULng32 maxVictims=40, // (IN) : max # of victims replaceable by new entry
     ULng32 avgPlanSz=93070);// (IN) : average plan size in bytes

  virtual ~QCache(); // destructor

  void makeEmpty();

  // try to add a new postparser (and preparser) entry into the cache
  CacheKey* addEntry
    (TextKey            *tkey,   // (IN) : preparser key
     CacheKey           *stmt,   // (IN) : postparser key
     CacheData          *plan,   // (IN) : sql statement's compiled plan
     TimeVal&            begT,   // (IN) : time at start of this compile
     char               *params, // (IN) : parameters for preparser entry
     ULng32       parmSz);// (IN) : len of params for preparser entry
  // requires: tkey,stmt are the keys of a cachable query
  // modifies: cache
  // effects : tries to add a new entry into the cache. may replace up to n 
  //           least recently used entries to try to make room. may return
  //           without adding stmt into the cache if there's still no room
  //           after decaching n LRU entries (where n=maxVictims). 
  //           Otherwise, allocates and copies stmt and plan into cache.
  //           The returned cache key is the copy created on CmpContext.

  // make room for and add a new preparser entry into the cache
  void addPreParserEntry
    (TextKey            *tkey,   // (IN) : a cachable sql statement
     char               *params, // (IN) : actual parameters
     ULng32       parmSz, // (IN) : len of actual parameters
     CacheEntry        *entry,  // (IN) : postparser cache entry
     TimeVal            &begT);  // (IN) : time at start of this compile
  // requires: tkey is the preparser key of a cachable query q
  //           entry is the postparser entry of cachable query q
  // modifies: cache
  // effects : tries to add a new entry into the cache. may replace
  //           least recently used preparser entry to try to make room.
  //           allocates and adds preparser entry into cache.

  // look-up sql query in the cache
  NABoolean lookUp
    (CacheKey      *stmt,  // (IN) : a cachable sql statement
     CacheDataPtr  &data,  // (OUT): stmt's template cache data
     CacheEntryPtr &entry, // (OUT): stmt's template cache entry
     CmpPhase       phase);// (IN) : current compiler phase
  // requires: stmt is a cachable query
  // modifies: lruQ_, data, entry, cache stats
  // effects : search cache for the given sql query
  //           if found then 
  //             return TRUE & pointer to compiled plan of the sql query
  //           else
  //             return FALSE

  // look-up sql query in the cache
  NABoolean lookUp
    (TextKey     *stmt, // (IN) : a cachable sql statement
     TextDataPtr &data);// (OUT): stmt's text cache entry
  // requires: stmt is a cachable query
  // modifies: lruQ_, data
  // effects : search cache for the given sql query
  //           if found then 
  //             data = pointer to compiled plan of the sql query; return TRUE
  //           else
  //             return FALSE

  // decache a sql query
  void deCache
    (CacheKey  *stmt); // (IN) : sql statement to be de-cached
  // requires: stmt is a cachable
  // modifies: cache
  // effects : if stmt is in-cache then remove it from cache

  // decache all entries that match a sql query
  void deCacheAll
    (CacheKey  *stmt, // (IN) : sql statement to be de-cached
     RelExpr   *qry); // (IN) : sql query
  // requires: stmt is a cachable
  // modifies: cache
  // effects : if stmt is in-cache then remove all matching entries 
  //           from cache. This is used by recompiles to make sure all
  //           outdated cached entries that match a query are decached.

  // decache all entries that match a sql query
  void deCacheAll
    (TextKey   *stmt, // (IN) : sql statement to be de-cached
     RelExpr   *qry); // (IN) : sql query
  // requires: stmt is a cachable
  // modifies: cache
  // effects : if stmt is in-cache then remove all matching entries 
  //           from cache. This is used by recompiles to make sure all
  //           outdated cached entries that match a query are decached.

  // decache a preparser cache entry
  void deCachePreParserEntry
    (TextKey  *stmt); // (IN) : key of a preparser cache entry
  // requires: stmt is the key of a preparser cache entry
  // modifies: cache
  // effects : if stmt is in-cache then remove its entry from cache

  // return average template plan size
  ULng32 avgPlanSize();

  // return average text entry size
  ULng32 avgTextEntrySize();

  // return an iterator positioned at beginning of query cache's LRU list
  LRUList::iterator begin() { return clruQ_.begin(); }

  // return an iterator positioned at beginning of preparser cache's LRU list
  LRUList::iterator beginPre() { return tlruQ_.begin(); }

  // return an iterator positioned at end of preparser cache's LRU list
  LRUList::iterator endPre() { return tlruQ_.end(); }

  // return TRUE iff cache has no entries
  NABoolean empty() const { return clruQ_.empty() && tlruQ_.empty(); }

  // current size (in bytes) of query cache
  ULng32 byteSize() const { return heap_->getAllocSize(); }

  // set heap upper limit
  void setHeapUpperLimit (size_t newUpperLimit) { heap_->setUpperLimit (newUpperLimit); }

  // high water mark (in bytes) of query cache
  ULng32 highWaterMark() const { return heap_->getHighWaterMark(); }

  // high water mark over some interval
  ULng32 intervalWaterMark() const { return heap_->getIntervalWaterMark(); }

  // return an iterator positioned at end of query cache's LRU list
  LRUList::iterator end() { return clruQ_.end(); }

  void incNOfCompiles(IpcMessageObjType op);
  void incNOfLookups() { nOfLookups_++; }
  void incNOfRetries() { nOfRetries_++; }  

  NABoolean isCachingOn() const;
  NABoolean isPreparserCachingOn(NABoolean duringAddEntry = FALSE) const;

  // maximum size (in bytes) of query cache
  ULng32 maxSize() const { return maxSiz_; }

  // maximum entries that can be displaced
  ULng32 maxVictims() const { return limit_; }

  ULng32 nOfCompiles() const { return nOfCompiles_; }
  ULng32 nOfLookups() const { return nOfLookups_; }
  ULng32 nOfRecompiles() const { return nOfRecompiles_; }
  ULng32 nOfRetries() const { return nOfRetries_; }

  ULng32 nOfCacheableCompiles(CmpPhase stage) const;

  ULng32 nOfCacheHits(CmpPhase stage) const;

  void resetIntervalWaterMark() { heap_->resetIntervalWaterMark(); }

  ULng32 nOfCacheableButTooLarge() const { return nOfCacheableButTooLarge_; }
  ULng32 nOfDisplacedEntries() const { return nOfDisplacedEntries_; }
  ULng32 nOfDisplacedTextEntries() const 
    { return nOfDisplacedPreParserEntries_; }
  
  // current number of cache entries
  ULng32 nOfPostParserEntries() const { return clruQ_.size(); }
  ULng32 nOfPreParserEntries() const { return tlruQ_.size(); }

  // current number of cached plans 
  ULng32 nOfPlans();

  // resize the query cache
  QCache* resizeCache
    (ULng32 maxSize,       // (IN) : maximum heap size in bytes
     ULng32 maxVictims=40);// (IN) : max # of victims replaceable by new entry  

  // set average plan size
  void setAvgPlanSz(ULng32 s) { if (clruQ_.size() <= 0) planSz_ = s; }

  // set limit on entries that can be displaced
  void setMaxVictims(ULng32 v) { limit_ = v; }

  // return maximum number of preparser entries
  static ULng32 maxPreParserEntries(ULng32 maxByteSz, ULng32 avgEntrySz);

  void sanityCheck(Int32 x);

  // reset counters
  void clearStats();

  // free Query Cache entries with specified QI Security Key
  void free_entries_with_QI_keys( Int32 NumSiKeys, SQL_QIKEY * pSiKeyArray );

  Lng32 getNumBuckets() { return cache_->getNumBuckets(); }

 private:
  // decache a postparser cache entry
  void deCachePostParserEntry
    (CacheEntry *entry); // (IN) : a postparser cache entry
  // requires: entry is a postparser cache entry
  // modifies: cache
  // effects : decache entry

  // decache a preparser cache entry
  void deCachePreParserEntry
    (CacheEntry *entry); // (IN) : a preparser cache entry
  // requires: entry is a preparser cache entry
  // modifies: cache
  // effects : decache entry

  // decache an array of preparser entries
  void deCachePreParserEntries
    (TextPtrArray& entries); // (IN) : an array of preparser entries
  // requires: entries is an array of preparser entries
  // modifies: cache
  // effects : decache entries
  QueryCache & querycache_; //reference to wapper object
  ULng32 limit_; // maximum number of victims replaceable
  NABoundedHeap* heap_;  // heap for cache entries
  ULng32 maxSiz_;// maximum byte size of query cache
  ULng32 planSz_;// average template plan size in bytes
  ULng32 tEntSz_;// average text entry size in bytes

  CacheHashTbl* cache_; // CacheKey hash table of cache entries
  TextHashTbl*  tHash_; // TextKey  hash table of cache entries
  LRUList       clruQ_; // queue of LRU postparser entries
  LRUList       tlruQ_; // queue of LRU preparser entries

  ULng32 nOfCompiles_; // cummulative number of compilation requests (include queries, cqd, invalid queries, ...)
  ULng32 nOfLookups_; // cummulative number of query cache loopups = cache hits (text and template) + cache misses (template cache insert attempts)
  ULng32 nOfRecompiles_; 
  ULng32 nOfCacheableCompiles_[N_STAGES];
  ULng32 nOfCacheHits_[N_STAGES];
  ULng32 nOfCacheableButTooLarge_;
  ULng32 nOfDisplacedEntries_;
  ULng32 nOfDisplacedPreParserEntries_;
  ULng32 nOfRetries_;
  ULng32 totalHashTblSize_; // the total space taken by 
                           // the text and template hash table.

  // return TRUE iff cache can accommodate a new entry of given size
  NABoolean canFit(ULng32 size);

  // unconditionally cache new postparser (and preparser) entry
  CacheKey* addEntry(TextKey *tkey, 
                KeyDataPair& newEntry, TimeVal& begTime,
                char *params, ULng32 parmSz, NABoolean sharePlan);

  // return free bytes left before we hit maxSiz_
  ULng32 getFreeSize() 
    { ULng32 a = heap_->getAllocSize(); return maxSiz_>a ? maxSiz_-a : 0; }

  // return bytes that can be freed by evicting this postparser cache entry
  ULng32 getSizeOfPostParserEntry(KeyDataPair& entry);

  // free enough LRU entries to increase free space to desired value
  NABoolean freeLRUentries(ULng32 newFreeBytes, ULng32 maxVictims);

  // free LRU preparser entry 
  void freeLRUPreParserEntry();

  void incNOfCacheableCompiles(CmpPhase stage);
  void incNOfCacheHits(CmpPhase stage);
  void incNOfCacheableButTooLarge();
  void incNOfDisplacedEntries(ULng32 howMany=1);
  void incNOfDisplacedPreParserEntries(ULng32 howMany=1);
};

// QCacheStats is the structure interface for supplying the QueryCache
// fields needed by the CompilationStats & Compiler Tracking feature.
// We have to restrict the interface to only those data members used
// by CompilationStats because we were getting mxcmp saveabends from 
// trying to compute avgPlanSize (which is not even used by the
// CompilationStats & Compiler Tracking feature).
struct QCacheStats {
  ULng32 currentSize;  // current cache size in bytes
  ULng32 nRecompiles;  // cum. count of recompiles
  ULng32 nCacheableP;  // cum. count of cacheable after parse
  ULng32 nCacheableB;  // cum. count of cacheable after bind
  ULng32 nCacheHitsP;  // cum. count of cache hits after parse
  ULng32 nCacheHitsB;  // cum. count of cache hits after bind
  ULng32 nCacheHitsT;  // cum. count of cache hits (all phases)
  ULng32 nCacheHitsPP; // cum. count of cache hits before parse
  ULng32 nLookups;     // cum. count of query cache lookups
};

// QueryCacheStats is the structure interface for supplying the QueryCache
// fields needed by Javier's Virtual Tables for Query Plan Caching Statistics
struct QueryCacheStats {
  ULng32 avgPlanSize;  // average template plan size
  ULng32 maxSize;      // maximum cache size in bytes
  ULng32 highWaterMark;// high water mark of cache size in bytes
  ULng32 maxVictims;   // maximum entries that can be displaced
  ULng32 nEntries;     // current number of template cache entries
  ULng32 nPlans;       // current number of compiled plans in the template cache 
  ULng32 nCompiles;    // cum. count of sqlcomp calls
  ULng32 nRetries;     // cum. count of successful compiler retries

  QCacheStats s;   

  ULng32 nTooLarge;    // cum. count of cacheable-but-too-large
  ULng32 nDisplaced;   // cum. count of displaced template cache entries
  Int32   optimLvl;     // current optimization level
  ULng32 envID;        // current environment ID

  ULng32 avgTEntSize;  // average text entry size
  ULng32 nTextEntries; // current number of text cache entries
  ULng32 nDispTEnts;   // cum. count of displaced text cache entries
  ULng32 intervalWaterMark; // high water mark resettable on interval
};

// QueryCacheDetails is the structure interface for supplying the 
// QueryCacheEntries fields needed by Javier's Virtual Tables for 
// Query Plan Caching Statistics
struct QueryCacheDetails {
  Int64 planId;       // plan id from generator
  const char *qryTxt; // original sql statement text
  ULng32 entrySize;    // size in bytes of this cache entry, excluding plan size
  ULng32 planLength;   // size in bytes of the plan
  ULng32 nOfHits;      // total hits for this cache entry
  CmpPhase phase;     // compiler phase of this cache entry
  Int32 optLvl;         // optimization level of this entry
  ULng32 envID;        // environment ID of this entry
  const char *catalog;// catalog name for this entry
  const char *schema; // schema name for this entry
  Int32 nParams;        // number of parameters of this entry
  const char *paramTypes;// parameter types of this entry
  ULng32 compTime;   // compile time in msec
  ULng32 avgHitTime; // avg hit time in msec
  const char *reqdShape;// control query shape or empty string
  TransMode::IsolationLevel isoLvl;   // tx isolation level
  TransMode::IsolationLevel isoLvlForUpdates;   // tx isolation level
  TransMode::AccessMode     accMode;  // tx access mode
  TransMode::AutoCommit     autoCmt;  // tx auto-commit
  Int16                     flags;    // tx flags
  TransMode::RollbackMode   rbackMode;// tx rollback mode
};

// QueryCache encapsulates a singleton cache of compiled query plans.
// It is a singleton cache in the sense that there is only one cache 
// instance that is shared by one or more CmpMain instances. (There 
// is one CmpMain instance per query compilation). The lifetime of
// this shared singleton cache starts with the first query compilation
// and ends when the cache is resized to zero (via control query default
// query_cache) or when mxcmp dies.
class QueryCache {
 public:

  QueryCache 
  (ULng32 maxSize = 16384, // (IN) : maximum heap size in bytes
   ULng32 maxVictims=40,   // (IN) : max # of victims replaceable by new entry  
   ULng32 avgPlanSz=93070);// (IN) : average plan size in bytes

  // add a new postparser entry into the cache
  CacheKey* addEntry
    (TextKey            *tkey,   // (IN) : preparser key
     CacheKey           *stmt,   // (IN) : postparser key
     CacheData          *plan,   // (IN) : sql statement's compiled plan
     TimeVal            &begT,   // (IN) : time at start of this compile
     char               *params, // (IN) : parameters for preparser entry
     ULng32       parmSz);// (IN) : len of params for preparser entry

  // add a new preparser entry into the cache
  void addPreParserEntry
    (TextKey            *tkey,   // (IN) : a cachable sql statement
     char               *actuals,// (IN) : actual parameters
     ULng32       actLen, // (IN) : len of actuals
     CacheEntry        *entry,  // (IN) : postparser cache entry
     TimeVal            &begT);  // (IN) : time at start of this compile

  // return an iterator positioned at beginning of query cache's LRU list
  LRUList::iterator begin();

  // return an iterator positioned at beginning of preparser cache's LRU list
  LRUList::iterator beginPre();

  void sanityCheck(Int32 x)
  {
    { if (cache_) cache_->sanityCheck(x); }
  }

  // set heap upper limit
  void setHeapUpperLimit (size_t newUpperLimit) { if (cache_) cache_->setHeapUpperLimit (newUpperLimit); }

  // decache all entries that match a sql query
  void deCacheAll
    (CacheKey  *stmt, // (IN) : sql statement to be de-cached
     RelExpr   *qry)  // (IN) : sql query
    { if (cache_) cache_->deCacheAll(stmt, qry); }

  // decache all entries that match a sql query
  void deCacheAll
    (TextKey   *stmt, // (IN) : sql statement to be de-cached
     RelExpr   *qry)  // (IN) : sql query
    { if (cache_) cache_->deCacheAll(stmt, qry); }

  // decache a preparser cache entry
  void deCachePreParserEntry
    (TextKey  *stmt) // (IN) : key of a preparser cache entry
  { if (cache_) cache_->deCachePreParserEntry(stmt); }

  // return an iterator positioned at end of query cache's LRU list
  LRUList::iterator end();

  // return an iterator positioned at end of preparser cache's LRU list
  LRUList::iterator endPre();

  // return TRUE iff cache has no entries
  NABoolean empty() { return cache_ ? cache_->empty() : TRUE; }

  // free all memory allocated to cache
  void finalize(const char *staticOrDynamic);

  // get query cache statistics
  void getCacheStats
    (QueryCacheStats &stats); // (OUT): query cache statistics

  //get statistics for Hybrid Query Cache
  void getHQCStats(HybridQueryCacheStats & stats);

  void getHQCEntryDetails(HQCCacheKey* hkey, HQCCacheEntry* entry, HybridQueryCacheDetails & details);
  
  // get query cache statistics used by CompilationStats
  // return FALSE if cache size is zero
  NABoolean getCompilationCacheStats
    (QCacheStats &stats); // (OUT): query cache statistics

  // get details of this query cache entry
  void getEntryDetails
    (LRUList::iterator i,         // (IN) : query cache iterator entry
     QueryCacheDetails &details); // (OUT): cache entry's detailed statistics

  // increment number of compiles
  void incNOfCompiles(IpcMessageObjType op) 
    { if (cache_) cache_->incNOfCompiles(op); }

  // increment number of compiler retries
  void incNOfRetries() { if (cache_) cache_->incNOfRetries(); }

  void resetIntervalWaterMark() 
    { if (cache_) cache_->resetIntervalWaterMark(); }

  // is query caching on?
  NABoolean isCachingOn() 
    { return cache_ ? cache_->isCachingOn() : FALSE; }

  // is pre-parser caching on?
  NABoolean isPreparserCachingOn() 
    { return cache_ ? cache_->isPreparserCachingOn() : FALSE; }

  // look-up sql query in the cache
  NABoolean lookUp
    (CacheKey      *stmt,  // (IN) : a cachable sql statement
     CacheDataPtr  &data,  // (OUT): stmt's template cache data
     CacheEntryPtr &entry, // (OUT): stmt's template cache entry
     CmpPhase       phase) // (IN) : current compiler phase
    { return cache_ ? cache_->lookUp(stmt,data,entry, phase) : FALSE; }

  // look-up sql query in the cache
  NABoolean lookUp
    (TextKey     *stmt, // (IN) : a cachable sql statement
     TextDataPtr &data) // (OUT): stmt's text cache entry
    { return cache_ ? cache_->lookUp(stmt,data) : FALSE; }

  // shrink query cache to zero entries
  void makeEmpty() { if (cache_) cache_->makeEmpty(); }

  // reconfigure cache to have new maxSize and new maxVictims
  void resizeCache
  (ULng32 maxSize,         // (IN) : maximum heap size in bytes
   ULng32 maxVictims=40,   // (IN) : max # of victims replaceable by new entry  
   ULng32 avgPlanSz=93070);// (IN) : average plan size in bytes

  // set average plan size
  void setAvgPlanSz(ULng32 s) { if (cache_) cache_->setAvgPlanSz(s); }

  // set limit on entries that can be displaced
  void setMaxVictims(ULng32 v) { if (cache_) cache_->setMaxVictims(v); }

  // set limit on entries per key in hqc
  void setHQCMaxValuesPerKey(Int32 v) { if (hqc_) hqc_->setMaxEntries(v); }


  // reset counters
  void clearStats() { if (cache_) cache_->clearStats(); }

  // free (remove) entires in the Query Cache that have a particular SQL_QIKEY
  void free_entries_with_QI_keys( Int32 NumSiKeys, SQL_QIKEY * pSiKeyEntry );
  void setQCache(QCache *qCache);

  NABoolean HQCAddEntry(HQCParseKey* hkey, CacheKey* ckey)
   {  return (hqc_->addEntry(hkey, ckey));  }

  ostream* getHQCLogFile(){  hqc_->initLogging(); return hqc_->getHQCLogFile(); }

  void invalidateHQCLogging() { hqc_->invalidateLogging(); }
  
  NABoolean HQCLookUp(
  	 HQCParseKey      *hkey,  // (IN) : a cachable sql statement
     CacheKey* & ckey)  // (OUT): stmt's template cache data
  { return hqc_->lookUp(hkey, ckey); }

  HybridQCache* getHQC() { return hqc_;}

 private:

  QCache *cache_; // cache of compiled plans on CmpContext
  
  HybridQCache* hqc_;
  
  void shutdownCache();

  // constant parameter types in string form.
  // used by QueryCache::getEntryDetails(),
  // ParameterTypeList::getParameterTypes()
  // and freed by QueryCache::finalize().
  NAString *parameterTypes_;
};

//Classes below, when xxx::getNext() is called time after time by xxx::sp_Process(), 
//should be able to iterator all querycache instances of different CmpContexts, in embedded compiler.
class QueryCacheStatsISPIterator : public ISPIterator
{
public:
  QueryCacheStatsISPIterator(SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, 
                                             SP_ERROR_STRUCT* error, const NAArray<CmpContextInfo*> & ctxs, CollHeap * h);
  //if currCacheIndex_ is set 0, currQCache_ is not used and should always be NULL
  NABoolean getNext(QueryCacheStats & stats);
protected:
  QueryCache* currQCache_;
};

class QueryCacheEntriesISPIterator : public ISPIterator
{
public:
  QueryCacheEntriesISPIterator(SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, 
                                               SP_ERROR_STRUCT* error, const NAArray<CmpContextInfo*> & ctxs, CollHeap * h);
  
  NABoolean getNext(QueryCacheDetails & details);
  Int32 & counter() { return counter_; }
protected:
  Int32 counter_;
  QueryCache* currQCache_;
  LRUList::iterator SQCIterator_;
};

class HybridQueryCacheStatsISPIterator : public ISPIterator
{
public:
    HybridQueryCacheStatsISPIterator(SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, 
                                                          SP_ERROR_STRUCT* error, const NAArray<CmpContextInfo*> & ctxs, CollHeap * h);

    NABoolean getNext(HybridQueryCacheStats & stats);
protected:
  QueryCache* currQCache_;
};

class HybridQueryCacheEntriesISPIterator : public ISPIterator
{
public:
    HybridQueryCacheEntriesISPIterator(SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, 
                                                            SP_ERROR_STRUCT* error, const NAArray<CmpContextInfo*> & ctxs, CollHeap * h);

    ~HybridQueryCacheEntriesISPIterator();

    NABoolean getNext(HybridQueryCacheDetails & details);
protected:
    Int32 currEntryIndex_;
    Int32 currEntriesPerKey_;
    HQCCacheKey* currHKeyPtr_;
    HQCCacheData* currValueList_;
    HQCHashTblItor* HQCIterator_;
    LRUList::iterator SQCIterator_;
    QueryCache* currQCache_;
};

class QueryCacheDeleter : public ISPIterator
{
public:
  QueryCacheDeleter(SP_ROW_DATA  inputData, SP_EXTRACT_FUNCPTR  eFunc, 
                                             SP_ERROR_STRUCT* error, const NAArray<CmpContextInfo*> & ctxs, CollHeap * h);
  void doDelete();
protected:
  QueryCache* currQCache_;
};

#endif // QUERYCACHE__H
