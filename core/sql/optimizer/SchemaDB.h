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
#ifndef SCHEMADB_H
#define SCHEMADB_H
/* -*-C++-*-
*************************************************************************
*
* File:         SchemaDB.h
* Description:  Schema Information
* Created:      4/27/94
* Language:     C++
*
*
*************************************************************************
*/


#include "NAAssert.h"	 // required after including a RogueWave file!

#include "BaseTypes.h"
#include "charinfo.h"
#include "ColumnDesc.h"
#include "ComTransInfo.h"
#include "CmpContext.h"
#include "DomainDesc.h"
#include "NADefaults.h"
#include "NARoutineDB.h"
#include "NATable.h"
#include "NARoutine.h"
#include "TableDesc.h"
#include "ValueDesc.h"

// triggers -- eliezer
class TriggerDB;
class NARoutineDB;

// ----------------------------------------------------------------------
// contents of this file
// ----------------------------------------------------------------------
class SchemaDB;
class NodeToCpuVolMap;
class NodeToCpuVolMapDB;
class POSRule;
class POSInfo;

// ***********************************************************************
// Information derived from the Information Schema tables (IST's) is
// stored in SchemaDB.
//
// The SchemaDB contains two "databases" that span the compilation 
// of multiple SQL statments. They are
//
// 1) The NATableDB - it contains the physical schema information
//                    for an SQL table or table-valued stored procedure
//                    corresponding to a given qualified (ANSI/NSK/UNIX) name.
// 2) The FilesetDB - it contains the description regarding each fileset 
//                    that corresponds to a NATable in the NATableDB.
//
// The SchemaDB also contains three "statement tables". Each statement
// table contains information that is relevant for the compilation of 
// a single SQL statement. The statement tables are reallocated on a
// per statement basis.
// 
// Important note : The SchemaDB is designed to last across statements.
// For the fields need to be initialized/reset at each statement, they
// need to be done in
//  void initPerStatement();
//  void cleanupPerStatement();
// See notes below about CmpContext...!
// ***********************************************************************
class SchemaDB : public NABasicObject
{
public:

  // --------------------------------------------------------------------
  // Constructor function
  // --------------------------------------------------------------------
  SchemaDB(ReadTableDef *rtd) ;

  // copy ctor
  SchemaDB(const SchemaDB & orig) ; // not written

  // --------------------------------------------------------------------
  // Destructor function
  // --------------------------------------------------------------------
  ~SchemaDB();
  
  // --------------------------------------------------------------------
  // Accessor functions
  // --------------------------------------------------------------------
  NATableDB       * getNATableDB()          { return &tableDB_; }
  ReadTableDef	  * getReadTableDef()       { return readTableDef_; }
  ValueDescArray  & getValueDescArray()     { return valueDArray_; }
  NADefaults      & getDefaults()           { return defaults_; }
  NARoutineDB     * getNARoutineDB()        { return &routineDB_; }
  NARoutineDB     * getNARoutineActionDB()  { return &actionRoutineDB_; }

  //   Flags:  	  The ON/ENABLE bit		The OFF value
  enum Flags	{ REFRESH_CACHE		= 0x1,  NO_REFRESH_CACHE	= 0,
  		  APPLY_NAMETYPE_RULES	= 0x2,  FORCE_ANSI_NAMETYPE	= 0
		};

  const SchemaName& getDefaultSchema(UInt32 flags =
				     NO_REFRESH_CACHE | FORCE_ANSI_NAMETYPE);
  // triggers -- eliezer
  TriggerDB		  * getTriggerDB();
  RefConstraintList *getRIs(QualifiedName &subjectTable, 
		ComOperation operation);

  // --------------------------------------------------------------------
  // Mutator functions
  // --------------------------------------------------------------------

  // --------------------------------------------------------------------
  // The SchemaDB maintains a set of internal tables. They must be 
  // re-initialized explicitly per statement -- this used to be via a
  // call to createStmtTables, which is no longer useful, just a noop --
  // now **ONLY** CmpContext should manage the init/cleanupPerStatement
  // calls, which is the current mechanism!
  //
  // This is also a convenient spot to hide transaction ends and begins;
  // see ReadTableDef.[hC] for fuller discussion.
  // --------------------------------------------------------------------
  void createStmtTables();      // no longer used (formerly required)
  void dropStmtTables();        // no longer used
  NABoolean endTransaction();   // implicitly called (idempotent)

  // --------------------------------------------------------------------
  // Methods for addimg new entries to the "statement tables".
  // --------------------------------------------------------------------
  void insertValueDesc(ValueDesc *vdesc) { valueDArray_.insert(vdesc); }
  void insertDomainDesc(DomainDesc *ddesc) { domainDList_.insert(ddesc); }
  
  // -----------------------------------------------------------------------
  // Methods to initialize and cleanup the statement wide members.
  // For the members need to be initialized/reset for each statement,
  // the initialization code has to be put into the following routines.
  // SchemaDB is designed to stay across statements. So the constructor
  // does not get called for each statement.
  //
  // See notes above about CmpContext, which should be the **only** caller!
  // -----------------------------------------------------------------------
  void initPerStatement(NABoolean lightweight = FALSE);		// before stmt
  void cleanupPerStatement();					// after stmt

  NodeToCpuVolMapDB *getNodeToCpuVolMapDB();

  Lng32 getCurrentDiskPool() { return currentDiskPool_; }

  void setCurrentDiskPool(Lng32 diskPool) 
                                      { currentDiskPool_ = diskPool; }

  void incCurrentDiskPool() { currentDiskPool_++; }

  float getHbaseBlockCacheFrac();
  
private:  

  // --------------------------------------------------------------------
  // A hash table that uses the qualified tablename as the hash key.
  // This is a collection of NATables that persists across the 
  // compilation of all SQL statements. It is deallocated when the 
  // SQL compiler terminates processing. For now, (11/20/96) tableDB_
  // needs to be cleanup up for each statement, because it is caching
  // the information without checking the timestamp for reload if 
  // necessary. After integrated with catman ReadTableDef code, it
  // will only be reloaded if necessary.
  // --------------------------------------------------------------------
  NATableDB tableDB_;

  // --------------------------------------------------------------------
  // A collection of ValueDesc (value descriptors).
  // A column can have one or more ValueDescs allocated for it.  
  // This collection is rebuilt for each SQL statement. It is reset
  // at the end of compilation for each statement.
  // --------------------------------------------------------------------
  ValueDescArray valueDArray_;

  // --------------------------------------------------------------------
  // A collection of DomainDesc (domain descriptors).
  // There is one Domain descriptor per column that is referenced in 
  // the query. This collection is rebuilt for each SQL statement.
  // --------------------------------------------------------------------
  DomainDescList domainDList_;

  // --------------------------------------------------------------------
  // ReadTableDef server
  // --------------------------------------------------------------------
  ReadTableDef *readTableDef_;

  // --------------------------------------------------------------------
  // In-memory table of defaults values.
  // --------------------------------------------------------------------
  NADefaults defaults_;

  // --------------------------------------------------------------------------
  // The default catalog and schema might come from the NADefaults
  // or be overridden via Ansi rules (the MODULE stmt, CREATE SCHEMA stmt).
  //
  // They might also be supplanted by the MPLOC, if NAMETYPE NSK is in effect
  // (this is of course a Tandem extension).
  //
  // The Ansi override/defaulting occurs in BindWA, not here.
  //
  // The Tandem NAMETYPE NSK defaulting occurs in method getDefaultSchema().
  // Note that defaultSchema_ always contains the ANSI schema,
  // so a call to the get method will by default get the ANSI schema --
  // only by passing the flag bit APPLY_NAMETYPE_RULES might you get 
  // the NAMETYPE NSK version of the schema (which is to say the MPLOC version).
  //
  //   This is sufficiently complicated logic, that the default schema appears
  //   in this class, as opposed to class SqlParser_NADefaults.
  //   Most other of our NADefaults follow no Ansi-required standard
  //   and generally are fixed/global.
  //   Those ones can be gotten directly from NADefaults,
  //   or (for better performance) from the "precompiled" SqlParser_NADefaults
  //   globals -- see NADefaults and SqlParserGlobalsCmn.h.
  // --------------------------------------------------------------------------
  SchemaName defaultSchema_;			// always ANSI (not MPLOC)

  // triggers -- eliezer
  // created on demand
  TriggerDB *triggerDB_;

  // Hash lookup of (UDR) NARoutine objects
  NARoutineDB routineDB_;
  NARoutineDB actionRoutineDB_;

  // Hash lookup by Node name
  NodeToCpuVolMapDB *nodeToCpuVolMapDB_;

  Lng32 currentDiskPool_;
  float hbaseBlockCacheFrac_;

}; // class SchemaDB

void InitSchemaDB();

inline SchemaDB *ActiveSchemaDB()  { return CmpCommon::context()->schemaDB_; }
inline SchemaDB *ActiveSchemaDB_Safe()
{ return CmpCommon::context() ? CmpCommon::context()->schemaDB_ : NULL; }

inline double getDefaultAsDouble(const Int32& key)
{
  return ActiveSchemaDB()->getDefaults().getAsDouble(key);
}

inline Lng32 getDefaultAsLong(const Int32& key)
{
  return ActiveSchemaDB()->getDefaults().getAsLong(key);
}


#define NODE_INIT_SIZE  16
#define CPU_INIT_SIZE  16

class CpuToVolMap : public LIST(NAString *)
{
  public:
    CpuToVolMap(Lng32 cpu, NAMemory *h)
               : LIST(NAString *)(h), cpu_(cpu)
    {};

    Lng32 getCpu() const {return cpu_;}

  private:
    Lng32 cpu_;
};


class NodeToCpuVolMap : public ARRAY(CpuToVolMap *)
{
  public:
    NodeToCpuVolMap(const NAString &nodeName, NAMemory *h)
        : ARRAY(CpuToVolMap *)(h, CPU_INIT_SIZE),
          nodeName_(nodeName, h),
	  totalNumOfVols_(0)
    { };

    NodeToCpuVolMap(const NodeToCpuVolMap &orig, CollHeap *h)
        : ARRAY(CpuToVolMap *)(orig, h),
	  totalNumOfVols_(0)
    {};


  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------
  const NAString& getNodeName() const  { return nodeName_; }

  // ---------------------------------------------------------------------
  // The following methods are required by the NAKeyLookup
  // ---------------------------------------------------------------------
   NABoolean operator==(const NodeToCpuVolMap& other) const
                                        { return this == &other; }

   const NAString *getKey() const { return &nodeName_; }

   short getTotalNumOfVols() const { return totalNumOfVols_; }

   void setTotalNumOfVols(const Lng32 numVols) 
    { totalNumOfVols_ = (short) numVols; }


  private:
    NAString nodeName_;
    short totalNumOfVols_;
};

class NodeToCpuVolMapDB : public NAKeyLookup<NAString, NodeToCpuVolMap>
{
  public:
    NodeToCpuVolMapDB(NAMemory *h) :
                      NAKeyLookup<NAString, NodeToCpuVolMap>
                          (NODE_INIT_SIZE,
                           NAKeyLookupEnums::KEY_INSIDE_VALUE,
                           h),
                      heap_(h),
                      sizeOfDiskVol_(0)
        {};

    NodeToCpuVolMapDB(const NodeToCpuVolMapDB &orig, NAMemory *h)
                        : NAKeyLookup<NAString, NodeToCpuVolMap> (orig, h)
        {};

    void clearAndDestroy();

    void buildVolumeNameCache(const LIST(NAString *) &nodeNames);
    Lng32 getTotalNumOfVols(NAString * nodeName);
    void estimateAndSetDiskSize(double diskSize);

    void setSizeOfDiskVol(Lng32 sizeOfDiskVol)
                   { sizeOfDiskVol_ = sizeOfDiskVol; }
    Lng32 getSizeOfDiskVol() { return sizeOfDiskVol_; }

  private:
    NAMemory *heap_;
    Lng32 sizeOfDiskVol_;
};


class VolumeInfo : public NABasicObject
{
  public:
    VolumeInfo(const NAString &volName, NAMemory *h)
      : volumeName_(volName, h),
        includedVol_(FALSE),
        usedCnt_(0)
    { }

    const NAString &getVolName() const { return volumeName_; }
    NABoolean isIncludedVol() const { return includedVol_; }
    void setIncludedVol(NABoolean included) { includedVol_ = included; }
    short getUsedCnt() const {return usedCnt_; }
    void incUsedCnt() { usedCnt_++; }
    void setUsedCnt(short cnt) { usedCnt_ = cnt; }


  private:
    NAString volumeName_;
    NABoolean includedVol_;
    short usedCnt_;
};

class VolumeList : public LIST(VolumeInfo *)
{
  public:
    VolumeList(Lng32 cpu, NAMemory *h)
               : LIST(VolumeInfo *)(h),
                 cpu_(cpu),
                 includedCpu_(FALSE),
                 numOfIncludedVols_(0),
                 currentVolIndex_(0),
                 totalNumOfVols_(0)
    { };

    VolumeList(const VolumeList &orig, NAMemory *h)
        : LIST(VolumeInfo *)(orig, h),
          cpu_(orig.cpu_),
          includedCpu_(orig.includedCpu_),
          numOfIncludedVols_(orig.numOfIncludedVols_),
          currentVolIndex_(orig.currentVolIndex_),
          totalNumOfVols_(0)
    { };

    VolumeList(const CpuToVolMap &orig, NAMemory *h);

    void insertVolInSortedOrder(VolumeInfo *vol);

    Lng32 getCpu() const 
        {return cpu_;}
    NABoolean isIncludedCpu() const 
        { return includedCpu_; }
    void setIncludedCpu(NABoolean included) 
        { includedCpu_ = included; }
    Lng32 getNumOfIncludedVols() const 
        { return numOfIncludedVols_; }
    void incNumOfIncludedVols() 
        { numOfIncludedVols_++; }
    void incTotalNumOfVols() { totalNumOfVols_++; }
    void setTotalNumOfVols(Lng32 numOfVols) 
        { totalNumOfVols_ = numOfVols; }
    Lng32 getTotalNumOfVols()
        { return totalNumOfVols_; }
    ULng32 getCurrVolIndex() const 
        { return currentVolIndex_; }
    void incCurrVolIndex()
        { currentVolIndex_ = (++currentVolIndex_  %  entries()); }

  private:
    Lng32 cpu_;
    NABoolean includedCpu_;
    short numOfIncludedVols_;
    Lng32 totalNumOfVols_;
    ULng32 currentVolIndex_;
};


class CpuArray : public ARRAY(VolumeList *)
{
  public:
    CpuArray(const NAString &nodeName, NAMemory *h)
        : ARRAY(VolumeList *)(h, CPU_INIT_SIZE),
          nodeName_(nodeName, h),
          currentCpuIndex_(-1)
    { };

    CpuArray(const CpuArray &orig, CollHeap *h)
        : ARRAY(VolumeList *)(orig, h),
          nodeName_(orig.nodeName_, h),
          currentCpuIndex_(orig.currentCpuIndex_)
    {};

    CpuArray(const NodeToCpuVolMap *node, NAMemory *h);

    void setCurrCpuIndex(Lng32 index)
        { currentCpuIndex_ = index; }
    Lng32 getCurrCpuIndex() const
        { return currentCpuIndex_; }
    void incCurrCpuIndex()
        { currentCpuIndex_ = (Lng32) (++currentCpuIndex_  %  getUsedLength()); }
    void setTotalNumOfVols(Lng32 numOfVols)
        { totalNumOfVols_ = numOfVols; }
    Lng32 getTotalNumOfVols()
        { return totalNumOfVols_; }

  // ---------------------------------------------------------------------
  // Accessor functions
  // ---------------------------------------------------------------------
  const NAString& getNodeName() const  { return nodeName_; }

  // ---------------------------------------------------------------------
  // The following methods are required by the NAKeyLookup
  // ---------------------------------------------------------------------
   NABoolean operator==(const CpuArray& other) const
                                        { return this == &other; }

   const NAString *getKey() const { return &nodeName_; }


  private:
    NAString nodeName_;
    Lng32 currentCpuIndex_;
    Lng32 totalNumOfVols_;
};


class NodeInfoDB : public NAKeyLookup<NAString, CpuArray>
{
  public:
    NodeInfoDB(NAMemory *h) :
                      NAKeyLookup<NAString, CpuArray>
                          (NODE_INIT_SIZE,
                           NAKeyLookupEnums::KEY_INSIDE_VALUE,
                           h),
                      totalNumOfCpus_(0),
                      totalNumOfVols_(0),
                      nodeNames_(h),
                      heap_(h)
    { };

    NodeInfoDB(const NodeInfoDB &orig, NAMemory *h)
                        : NAKeyLookup<NAString, CpuArray> (orig, h),
                          heap_(h),
                          totalNumOfVols_(orig.totalNumOfVols_),
                          totalNumOfCpus_(orig.totalNumOfCpus_),
                          nodeNames_(orig.nodeNames_, h)
    { };

    NodeInfoDB(const NodeToCpuVolMapDB *nodeDB,
               const LIST(NAString *) &qualifyingNodeNames,
               NAMemory *h);

    void clearAndDestroy();  // not written

    void incNumOfCpus() { totalNumOfCpus_++; }
    Lng32 getNumOfCpus() const { return totalNumOfCpus_; }

    void incTotalNumOfVols() { totalNumOfVols_++; }
    Lng32 getTotalNumOfVols();

    const LIST(NAString *) &getNodeNames() const { return nodeNames_; }

    void insertNodeName(NAString *nodeName)
    {
      NAString *name = new (heap_) NAString(*nodeName, heap_);
      nodeNames_.insert(name);
    }


    void insertNodeNameInSortedOrder(NAString *nodeName);


  private:
    NAMemory *heap_;

    // total number of qualifying cpus in all the qualifying nodes of
    // this statement
    Lng32 totalNumOfCpus_;

    // total number of qualifying volumes in all the qualifying nodes of
    // this statement
    Lng32 totalNumOfVols_;

    // list of qualifying node names for this statement
    LIST(NAString *) nodeNames_;

    Lng32 sizeOfDiskVol_;

};


#endif /* SCHEMADB_H */



