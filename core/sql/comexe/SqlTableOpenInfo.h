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
****************************************************************************
*
* File:         SqlTableOpenInfo.h
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef SQLTABLEOPENINFO_H
#define SQLTABLEOPENINFO_H

#include "Int64.h"
#include "NABoolean.h"
#include "ComPackDefs.h"
#include "NAVersionedObject.h"
#include "PrivMgrDefs.h"

// #include "BaseTypes.h"
// --------------------------------------------------------------------------
// Class SqlTableOpenInfo
// --------------------------------------------------------------------------
class SqlTableOpenInfo : public NAVersionedObject
{
public:

  enum AccessMode
  {
    READ_,
    WRITE_,
    READWRITE_
  };

  struct AccessFlags
  {
    UInt32 select_: 1;
    UInt32 insert_: 1;
    UInt32 update_: 1;
    UInt32 delete_: 1;
  };

  void init()
  {
    openFlags_           = 0;
    otherFlags_          = FILLER_IS_INITED;

    // Default settings
    setShareOpens(TRUE);
    setExecutorOpen(TRUE);
    setNoWaited(TRUE);
    setLockWait(TRUE);
    setValidateTimestamp(TRUE);
    setCheckSecurity(FALSE);
    setUtilityOpen(FALSE);

    maxNowaitRequests_   = 15;
    accessMode_ 	 = READ_;
    redefTime_ 		 = -1;
    numParts_ 		 = -1;
    numIndexes_          = -1;

    fileName_ 		 = (NABasicPtr) NULL ;
    ansiName_ 		 = (NABasicPtr) NULL ;
    corrName_ 		 = (NABasicPtr) NULL ;
    columnListCount_ 	 = 0;
    updateColumnList_ 	 = (Int16Ptr) NULL;
    partNameArray_       = (NABasicPtr) NULL ;

    timeoutVal_          = 6000;

    utilityOpenId_       = -1;

    numPartsInPartNameArray_
                         = 0;

    priority_            = 3;
    clearAllAccessFlags();
    clearFiller();
  }

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize()   { return (short)sizeof(SqlTableOpenInfo); }

  SqlTableOpenInfo() : NAVersionedObject(-1)                     { init(); }

  // No destructor, and no "delete ansiName_;" etc. in setAnsiName() methods,
  // for we will assume that all allocations come from a heap
  
  NABoolean shareOpens()      { return (openFlags_ & SHARE_OPENS)    != 0; }
  NABoolean executorOpen()    { return (openFlags_ & EXECUTOR_OPEN)  != 0; }
  NABoolean openAllParts()    { return (openFlags_ & OPEN_ALL_PARTS) != 0; }
  NABoolean isUtilityOpen()   { return (openFlags_ & UTILITY_OPEN) != 0;   }
  NABoolean distributeOpens() { return (openFlags_ & DISTRIBUTE_OPENS) != 0; }
  NABoolean reuseOpens()      { return (openFlags_ & REUSE_OPENS) != 0; }
  NABoolean reuseOpensForWrite()      { return (openFlags_ & REUSE_OPENS_FOR_WRITE) != 0; }
  NABoolean readProtectedOpen() { return (openFlags_ & READ_PROTECTED_OPEN) != 0;}
  NABoolean insertToNonAudited() { return (openFlags_ & INSERT_TO_NONAUDITED_OPEN) != 0;}

  NABoolean randomPartarrayGeneration() { return (openFlags_ & RANDOM_PARTARRAY_GENERATION) != 0;}

  NABoolean locationSpecified() { return (otherFlags_ & LOCATION)    != 0; }
  NABoolean isView()            { return (otherFlags_ & IS_MX_VIEW)  != 0; }
  NABoolean validateTimestamp() { return (otherFlags_ & VALIDATE_TS) != 0; }
  NABoolean noWaited()          { return (otherFlags_ & NO_WAIT)     != 0; }
  NABoolean lockWait()          { return (otherFlags_ & LOCK_WAIT)   != 0; }
  NABoolean checkSecurity()     { return (otherFlags_ & SECURITY)    != 0; }
  
  NABoolean subjectTable() const{ return (otherFlags_ & SUBJECT_TABLE) != 0; }
  NABoolean isAudited()         { return (otherFlags_ & AUDITED)     != 0; }
  NABoolean isIndex()           { return (otherFlags_ & IS_MX_INDEX) != 0; }
  NABoolean specialTable() { return (otherFlags_ & SPECIAL_TABLE) != 0; }
  NABoolean isUDRSurrogate()    { return (otherFlags_ & IS_UDR_SURROGATE) != 0; }
  NABoolean isReverseScan()    { return (otherFlags_ & REVERSE_SCAN) != 0; }
  NABoolean isNSAOperation()   { return (otherFlags_ & IS_NSA_OPERATION) != 0; }
  NABoolean isInsertOfUpdateCK() { return (otherFlags_ & INSERT_OF_UPDATE_CK) != 0; }
  NABoolean isMXMetadataTable()
  { return (otherFlags_ & IS_MX_METADATA_TABLE) != 0; }

  NABoolean isValidateViewsAtOpenTime()     
  { return (otherFlags_ & VALIDATE_VIEWS_AT_OPEN_TIME)    != 0; }

  NABoolean validateNumIndexesAtOpenTime()     
  { return (otherFlags_ & VALIDATE_NUM_INDEXES_AT_OPEN_TIME)    != 0; }

  NABoolean isHbase()     
  { return (otherFlags_ & IS_HBASE)    != 0; }
  
  void setShareOpens(NABoolean v)      
           { (v ? openFlags_ |= SHARE_OPENS : openFlags_ &= ~SHARE_OPENS); }
  void setExecutorOpen(NABoolean v)    
       { (v ? openFlags_ |= EXECUTOR_OPEN : openFlags_ &= ~EXECUTOR_OPEN); }
  void setOpenAllParts(NABoolean v)    
     { (v ? openFlags_ |= OPEN_ALL_PARTS : openFlags_ &= ~OPEN_ALL_PARTS); }
  void setUtilityOpen(NABoolean v)            
     { (v ? openFlags_ |= UTILITY_OPEN : openFlags_ &= ~UTILITY_OPEN); }
  void setDistributeOpens(NABoolean v) 	      
               { (v ? openFlags_ |= DISTRIBUTE_OPENS : openFlags_ &= ~DISTRIBUTE_OPENS); }
  void setReuseOpens(NABoolean v) 	      
               { (v ? openFlags_ |= REUSE_OPENS : openFlags_ &= ~REUSE_OPENS); }
  void setReuseOpensForWrite(NABoolean v) 	      
               { (v ? openFlags_ |= REUSE_OPENS_FOR_WRITE : openFlags_ &= ~REUSE_OPENS_FOR_WRITE); }
  void setReadProtectedOpen(NABoolean v) 	      
               { (v ? openFlags_ |= READ_PROTECTED_OPEN : openFlags_ &= ~READ_PROTECTED_OPEN); }
  void setInsertToNonAuditedOpen(NABoolean v) 	      
               { (v ? openFlags_ |= INSERT_TO_NONAUDITED_OPEN : openFlags_ &= ~INSERT_TO_NONAUDITED_OPEN); }

  void setRandomPartarrayGeneration(NABoolean v) 	      
               { (v ? openFlags_ |= RANDOM_PARTARRAY_GENERATION : openFlags_ &= ~RANDOM_PARTARRAY_GENERATION); }

  void setLocationSpecified(NABoolean v) 
               { (v ? otherFlags_ |= LOCATION : otherFlags_ &= ~LOCATION); }
  void setIsView(NABoolean v)            
                 { (v ? otherFlags_ |= IS_MX_VIEW : otherFlags_ &= ~IS_MX_VIEW); }
  void setValidateTimestamp(NABoolean v) 
         { (v ? otherFlags_ |= VALIDATE_TS : otherFlags_ &= ~VALIDATE_TS); }
  void setNoWaited(NABoolean v)          
                   { (v ? otherFlags_ |= NO_WAIT : otherFlags_ &= ~NO_WAIT); }
  void setLockWait(NABoolean v)          
             { (v ? otherFlags_ |= LOCK_WAIT : otherFlags_ &= ~LOCK_WAIT); }
  void setCheckSecurity(NABoolean v)     
               { (v ? otherFlags_ |= SECURITY : otherFlags_ &= ~SECURITY); }
  void setIsAudited(NABoolean v)     
               { (v ? otherFlags_ |= AUDITED : otherFlags_ &= ~AUDITED); }
  void setIsIndex(NABoolean v)            
                 { (v ? otherFlags_ |= IS_MX_INDEX : otherFlags_ &= ~IS_MX_INDEX); }
  void setSpecialTable(NABoolean v)            
     { (v ? otherFlags_ |= SPECIAL_TABLE : otherFlags_ &= ~SPECIAL_TABLE); }
  void setIsUDRSurrogate(NABoolean v)
     { (v ? otherFlags_ |= IS_UDR_SURROGATE : otherFlags_ &= ~IS_UDR_SURROGATE); }
  void setIsReverseScan(NABoolean v)
     { (v ? otherFlags_ |= REVERSE_SCAN : otherFlags_ &= ~REVERSE_SCAN); }
  void setIsNSAOperation(NABoolean v)
     { (v ? otherFlags_ |= IS_NSA_OPERATION : otherFlags_ &= ~IS_NSA_OPERATION); }
  void setIsMXMetadataTable(NABoolean v)
     { (v ? otherFlags_ |= IS_MX_METADATA_TABLE : otherFlags_ &= ~IS_MX_METADATA_TABLE); }
 void setIsInsertOfUpdateCK(NABoolean v)
     { (v ? otherFlags_ |= INSERT_OF_UPDATE_CK : otherFlags_ &= ~INSERT_OF_UPDATE_CK); }

  inline 
  void setSubjectTable(NABoolean v)
	{ (v ? otherFlags_ |= SUBJECT_TABLE : otherFlags_ &= ~SUBJECT_TABLE); }
  void setValidateViewsAtOpenTime(NABoolean v)     
  { (v ? otherFlags_ |= VALIDATE_VIEWS_AT_OPEN_TIME :
     otherFlags_ &= ~VALIDATE_VIEWS_AT_OPEN_TIME); }

  void setValidateNumIndexesAtOpenTime(NABoolean v)     
  { (v ? otherFlags_ |= VALIDATE_NUM_INDEXES_AT_OPEN_TIME :
     otherFlags_ &= ~VALIDATE_NUM_INDEXES_AT_OPEN_TIME); }

  void setIsHbase(NABoolean v)     
  { (v ? otherFlags_ |= IS_HBASE : otherFlags_ &= ~IS_HBASE); }

  short maxNowaitRequests()                   { return maxNowaitRequests_; }
  void setMaxNowaitRequests(short numreq)   { maxNowaitRequests_ = numreq; }
  short numParts()                                     { return numParts_; }
  void setNumParts(short numparts)                 { numParts_ = numparts; }

  AccessMode accessMode()              { return (AccessMode)(accessMode_); }
  void setAccessMode(AccessMode am)                    { accessMode_ = am; }
  
  UInt32 getSelectAccess() const      { return accessFlags_.select_; }
  UInt32 getInsertAccess() const      { return accessFlags_.insert_; }
  UInt32 getUpdateAccess() const      { return accessFlags_.update_; }
  UInt32 getDeleteAccess() const      { return accessFlags_.delete_; }
  UInt32 getPrivAccess(PrivType which)
  {
    if (which == SELECT_PRIV)
      return getSelectAccess();
    else if (which == INSERT_PRIV)
      return getInsertAccess();
    else if (which == UPDATE_PRIV)
      return getUpdateAccess();
    else if (which == DELETE_PRIV)
      return getDeleteAccess();
    else
      return -1;
  }

  AccessFlags  getAccessFlags()  const      { return accessFlags_; }

  void setSelectAccess(UInt32 flag = 1)
                                    { accessFlags_.select_ = flag ? 1 : 0; }
  void setInsertAccess(UInt32 flag = 1)
                                    { accessFlags_.insert_ = flag ? 1 : 0; }
  void setUpdateAccess(UInt32 flag = 1)
                                    { accessFlags_.update_ = flag ? 1 : 0; }
  void setDeleteAccess(UInt32 flag = 1)
                                    { accessFlags_.delete_ = flag ? 1 : 0; }
  
  void clearAllAccessFlags()
  {
    accessFlags_.select_ = 0;
    accessFlags_.insert_ = 0;
    accessFlags_.update_ = 0;
    accessFlags_.delete_ = 0;
  }

  const char *ansiName()                  { return ansiName_.getPointer(); }
  char *nonConstAnsiName()                { return ansiName_.getPointer(); }
  void setAnsiName(char *name)                         { ansiName_ = name; }
  char *fileName()                        { return fileName_.getPointer(); }
  void setFileName(char *name)                         { fileName_ = name; }
  char *corrName()                        { return corrName_.getPointer(); }
  void setCorrName(char *name)                         { corrName_ = name; }

  
  char *getPartNameArray()  
           { return partNameArray_.getPointer();  }
  void setPartNameArray (char *buf)    { partNameArray_ = buf; }

  void setNumEntriesInPartNameArray (unsigned short numEntries) 
                                  { numPartsInPartNameArray_ = numEntries; }

  unsigned short getNumEntriesInPartNameArray (void)
                                        { return numPartsInPartNameArray_; }
  short getColumnListCount() const              { return columnListCount_; } 
  void setColumnListCount(short count)         { columnListCount_ = count; }
  void incColumnListCount()                          { columnListCount_++; }
  void decColumnListCount()                          { columnListCount_--; }

  // short *&getRefToColumnList()              { return updateColumnList_; }
  short *getColumnList()          { return updateColumnList_.getPointer(); }
  void setColumnList(short *colList)        { updateColumnList_ = colList; }

  // short &updateColumnList(short ix)     { return updateColumnList_[ix]; }
  short getUpdateColumn(short ix)     { return updateColumnList_[(Int32)ix]; }
  void setUpdateColumn(short ix, short col)
                                       { updateColumnList_[(Int32)ix] = col; }

  // short &updateColumnList(CollIndex ix) { return updateColumnList_[ix]; }
  short getUpdateColumn(CollIndex ix) { return updateColumnList_[(Int32)ix]; }
  void setUpdateColumn(CollIndex ix, short col)
                                       { updateColumnList_[(Int32)ix] = col; }

  // long &timeoutVal()                              { return timeoutVal_; }
  Lng32 getTimeoutVal()                               { return timeoutVal_; }
  void setTimeoutVal(Lng32 timeoutVal)          { timeoutVal_ = timeoutVal; }

  // short &priority()                                 { return priority_; }
  short getPriority()                                  { return priority_; }
  void setPriority(short priority)                 { priority_ = priority; }

  // timestamp value when table schema was modified 
  Int64 &redefTime()                                  { return redefTime_; }
  Int64 getRedefTime()                                { return redefTime_; }
  void setRedefTime(const Int64 &redefTime)      { redefTime_ = redefTime; }
 
  // security vector to monitor changes   
  Int32 getSecurityVector()                      { return securityVector_; }
  void setSecurityVector(const Int32 &secVec)  { securityVector_ = secVec; }

  // owner 
  Int16 getOwner()                                        { return owner_; }
  void setOwner(const Int16 &owner)                      { owner_ = owner; }

  Int16 getNumIndexes() {return numIndexes_;}
  void setNumIndexes(const Int16 numIndexes) {numIndexes_ = numIndexes;}

  // Utility Open Id
  Int64 getUtilityOpenId()                      { return utilityOpenId_; }
  void setUtilityOpenId(const Int64 &openId)    {utilityOpenId_ = openId;}

  void clearFiller()
  {
    memset(fillersSqlTableOpenInfo2_, 0, sizeof fillersSqlTableOpenInfo2_);
    memset(fillersSqlTableOpenInfo_,  0, sizeof fillersSqlTableOpenInfo_ );
  }

  Long pack(void *space)
  {
    ansiName_.pack(space);
    fileName_.pack(space);
    corrName_.pack(space);
    updateColumnList_.pack(space);
    partNameArray_.pack(space);
    return NAVersionedObject::pack(space);
  }

  Lng32 unpack(void *base, void * reallocator)
  {
    if ((otherFlags_ & FILLER_IS_INITED) == 0)
    {
      // This is a version-compatible way to fix a problem in plans 
      // written by the pre-R2 FCS compiler: the filler fields were not 
      // initialized to zeroes.  We only support plans from the R1.8 compiler,
      // so only offsets that were filler in R1.8 need to be repaired here.
      //
      numPartsInPartNameArray_ = 0;
      utilityOpenId_ = 0;
      partNameArray_ = (NABasicPtr) NULL ;
      clearFiller();
    }   

    if(updateColumnList_.unpack(base)) return -1;
    if(fileName_.unpack(base)) return -1;
    if(ansiName_.unpack(base)) return -1;
    if(corrName_.unpack(base)) return -1;
    if(partNameArray_.unpack(base)) return -1;
    return NAVersionedObject::unpack(base, reallocator);
  }

private:

  enum OpenFlags 
  {
    SHARE_OPENS = 0x0001, EXECUTOR_OPEN = 0x0002,
    OPEN_ALL_PARTS = 0x0004, SQL_SECURED = 0x0008,
    PRIV_OPEN = 0x0010, LICENSED_PROC = 0x0020,     
    UTILITY_OPEN = 0x0040,

    // Normally, first open is always done on the root partition. This
    // could cause heavy traffic on the root datavol and result in it
    // running out of resources(error 35, 30).
    // If this flag is set, then open is done on one of the partitions
    // from the partNameArray, if this array was generated. 
    // This would distribute opens among multiple datavols.
    // See runtime code for the actual algorith used to pick a partition.
    DISTRIBUTE_OPENS = 0x0080,

    // if set, tables are not closed at the end of a query. This allows
    // the same open to be reused for the next query which accesses that
    // table.
    // If the table is shared opened by multiple openers from the same
    // process, then the share count is decremented until it reaches 1.
    // At that time, the last open is preserved so it could be reused.
    //
    // Tables are closed if the process is exited, or if user id changes.
    REUSE_OPENS = 0x0100,

    // sets exclusion mode to ARK_DML_PROTECTED. This does a read protected
    // open which prevents any write access to the table.
    READ_PROTECTED_OPEN = 0x0400,

    // this flagis set when CQD USTAT_INSERT_TO_NONAUDITED_TABLE is DF_ON.
    // this tells the executor that inserting to a nonaudited table is ok
    // this causes the force open flag to be set for DP2.
    INSERT_TO_NONAUDITED_OPEN = 0x0800,

    // this flag is a temporary flag to tell the executor whether to keep 
    // current behavior as far as picking which partition of the table to 
    // open, or not.
    RANDOM_PARTARRAY_GENERATION = 0x1000,
    REUSE_OPENS_FOR_WRITE = 0x2000
  };

  /////////////////////////////////////////////////////////////////////
  // OtherFlags:
  //
  // SECURITY: this is set, if security check is to be done before
  //           opening the table to access data. If set, security
  //           check is done with a non-priv open, and if successful,
  //           the table is closed and reopened with priv open.
  /////////////////////////////////////////////////////////////////////
  enum OtherFlags
  {
    LOCATION       = 0x0001,
    IS_MX_VIEW     = 0x0002, 
    VALIDATE_TS    = 0x0004,
    NO_WAIT        = 0x0008,
    LOCK_WAIT      = 0x0010,
    SECURITY       = 0x0020, 
    AUDITED        = 0x0040,
    IS_MX_INDEX    = 0x0080,
    SPECIAL_TABLE =  0x0400, 
    IS_UDR_SURROGATE = 0x0800,
    SUBJECT_TABLE  =   0x01000, // triggers
    FILLER_IS_INITED = 0x2000,
    REVERSE_SCAN   = 0x4000,
    IS_NSA_OPERATION = 0x8000,
    VALIDATE_NUM_INDEXES_AT_OPEN_TIME = 0x10000,
    VALIDATE_VIEWS_AT_OPEN_TIME = 0x20000,
    IS_MX_METADATA_TABLE = 0x40000,
    INSERT_OF_UPDATE_CK = 0x80000,
    IS_HBASE                      = 0x100000
  };
	
  AccessFlags accessFlags_;                                         // 00-03
  UInt32 openFlags_;                                                // 04-07
  UInt32 otherFlags_;                                               // 08-11

  Int16 maxNowaitRequests_;                                         // 12-13

  // number of horizontal partitions
  Int16 numParts_;                                                  // 14-15
  
  // physical file name and the ansi name of the table or view
  NABasicPtr ansiName_;                                             // 16-23
  NABasicPtr fileName_;                                             // 24-31
  NABasicPtr corrName_;                                             // 32-39

  Int16Ptr updateColumnList_;                                       // 40-47
  Int16 columnListCount_;                                           // 48-49

  Int16 accessMode_;                                                // 50-51
  Int16 priority_;                                                  // 52-53

  // if this is an sql/mp base table which has secondary indexes, then
  // the next field indicates the number of indexes.
  // This field is used at runtime to validate if a new index with 
  // NO INVALIDATE option was created at runtime. An index created with
  // NO INV option does not change the timestamp of the base table which
  // causes incorrect index maintanence to be done.
  // To validate that, we compare this field with the actual number of
  // indexes at open time, if there is no timestamp mismatch. 
  // If the number at runtime is greater than numIndexes_, then an index
  // was created with NO INV option.
  Int16 numIndexes_;                                                // 54-55

  // timestamp value when table schema was modified 
  Int64 redefTime_;                                                 // 56-63

  // timeout value used while accessing this table. 
  Int32 timeoutVal_;                                                // 64-67

  // securityVector_ and owner_ for use with UDRs

  // guardian security vector                          
  Int32 securityVector_;                                            // 68-71

  // owner
  Int16 owner_;                                                     // 72-73 
  Int16 numPartsInPartNameArray_;                                   // 74-75
  char fillersSqlTableOpenInfo2_[4];                                // 76-79
  // utility open id, if this open is part of a utility operation.
  Int64 utilityOpenId_;                                             // 80-87
  NABasicPtr partNameArray_;                                        // 88-95
  char fillersSqlTableOpenInfo_[16];                               // 96-111

};

// --------------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for SqlTableOpenInfo
// --------------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<SqlTableOpenInfo> SqlTableOpenInfoPtr;

// --------------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for SqlTableOpenInfoPtr
// --------------------------------------------------------------------------
typedef
  NAVersionedObjectPtrArrayTempl<SqlTableOpenInfoPtr> SqlTableOpenInfoPtrPtr;


#endif
