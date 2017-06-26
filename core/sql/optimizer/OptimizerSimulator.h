/* -*-C++-*-
***********************************************************************
*
* File:         OptimizerSimulator.h
* Description:  This file is the header file for Optimizer Simulator
*               component (OSIM).
*
* Created:      12/2006
* Language:     C++
*
*
**********************************************************************/
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

#ifndef __OPTIMIZERSIMULATOR_H
#define __OPTIMIZERSIMULATOR_H

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <fstream>
#include <CmpCommon.h>
#include "BaseTypes.h"
#include "CollHeap.h"
#include "XMLUtil.h"
// forward declaration to allow usage of NATable *
class NATable;
class HiveClient_JNI;
class ExeCliInterface; 
class CmpSeabaseDDL;
class Queue;

class OsimAllHistograms;
class OsimHistogramEntry;
class HHDFSStatsBase;
class HHDFSTableStats;
struct hive_tbl_desc;

class OsimAllHistograms : public XMLElement
{
public:
    static const char elemName[];
    OsimAllHistograms(NAMemory * heap = 0)
      : XMLElement(NULL, heap)
      , list_(heap)
    { setParent(this); }

    virtual const char *getElementName() const
    { return elemName; }
    
    virtual ElementType getElementType() const 
    { return ElementType::ET_List;    }
    
    NAList<OsimHistogramEntry*> & getHistograms() { return list_; }

    void addEntry( const char* fullpath,
                            const char* username,
                            const char* pid,
                            const char* cat,
                            const char* sch,
                            const char* table,
                            const char* histogram);

protected:
    virtual void serializeBody(XMLString& xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);
private:
    //disable copy constructor
    OsimAllHistograms(const OsimAllHistograms&);
   //disable assign operator
    OsimAllHistograms& operator=(const OsimAllHistograms&);
    NAList<OsimHistogramEntry*> list_;
};

//represent one histogram file xxx.histograms or xxx.sb_histogram_interval
class OsimHistogramEntry : public XMLElement
{
    friend class OsimAllHistograms;
public:
    static const char elemName[];
    OsimHistogramEntry(XMLElementPtr parent, NAMemory * heap)
      : XMLElement(parent, heap)
      , currentTag_(heap)
      , fullPath_(heap)
      , userName_(heap)
      , pid_(heap)
      , catalog_(heap)
      , schema_(heap)
      , table_(heap)
      , histogram_(heap)
    {}
    
    virtual const char *getElementName() const
    { return elemName; }
    virtual ElementType getElementType() const 
    { return ElementType::ET_Table;    }
    
    NAString & getFullPath() {  return fullPath_;  }
    NAString & getUserName() {  return userName_;  }
    NAString & getPID() {  return pid_;  }
    NAString & getCatalog() {  return catalog_;  }
    NAString & getSchema() {  return schema_;  }
    NAString &  getTable() {  return table_;  }
    NAString & getHistogram() {  return histogram_;  }
protected:
    virtual void serializeBody(XMLString& xml);
    virtual void endElement(void * parser, const char * elementName);
    virtual void charData(void *parser, const char *data, Int32 len);
    virtual void startElement(void *parser, const char *elementName, const char **atts);
private:
    NAString currentTag_;
    NAString fullPath_;
    NAString userName_;
    NAString pid_;
    NAString catalog_;
    NAString schema_;
    NAString table_;
    NAString histogram_;
    //disable copy constructor and assignment.
    OsimHistogramEntry(const OsimHistogramEntry&);
    OsimHistogramEntry& operator=(const OsimHistogramEntry&);
};
//for reading histogram files path
class OsimElementMapper : public XMLElementMapper
{
  public:
    virtual XMLElementPtr operator()(void *parser, char *elementName, AttributeList atts);
};
//for reading hive table stats file path
class OsimHHDFSStatsMapper : public XMLElementMapper
{
  public:
    OsimHHDFSStatsMapper(NAMemory* h) : heap_(h){}
    virtual XMLElementPtr operator()(void *parser, char *elementName, AttributeList atts);
    NAMemory* heap_;
};

class OsimHHDFSStatsBase : public XMLElement
{
public:
        OsimHHDFSStatsBase(XMLElement* parent, HHDFSStatsBase* mirror, NAMemory * heap = 0)
        : XMLElement(parent, heap)
        , statsList_(heap)
        , mirror_(mirror)
        , pos_(-1)
        , heap_(heap)
        {}

        ~OsimHHDFSStatsBase(){
            for ( CollIndex i = 0; i < statsList_.entries(); i++){
              NADELETE(statsList_[i], OsimHHDFSStatsBase, heap_);
            }
        }

        virtual ElementType getElementType() const 
	{ return ElementType::ET_List;	  }
		
        virtual void addEntry(OsimHHDFSStatsBase* statsBase, Int32 pos){ statsBase->setPosition(pos); statsList_.insert(statsBase); }
        void setHHStats(HHDFSStatsBase* st) { mirror_ = st; }
	HHDFSStatsBase * getHHStats() { return mirror_; }
	void setPosition(Int32 p) { pos_ = p; }
	Int32 getPosition() const {  return pos_; }	
	virtual NABoolean restoreHHDFSStats(HHDFSStatsBase* hhstats, const char ** atts);
	virtual NABoolean setValue(HHDFSStatsBase* hhstats, const char *attrName, const char *attrValue);
	
protected:
	virtual void serializeBody(XMLString& xml);
	virtual void serializeAttrs(XMLString & xml);
	virtual void startElement(void *parser, const char *elementName, const char **atts){}
	virtual void endElement(void * parser, const char * elementName);

        LIST(OsimHHDFSStatsBase*)  statsList_;
	HHDFSStatsBase* mirror_;
        Int32 pos_;//position of this object in partition/bucket/file list
	NAMemory* heap_;
};

class  OsimHHDFSFileStats : public OsimHHDFSStatsBase
{
public:
	static const char elemName[];
	OsimHHDFSFileStats(XMLElement* parent, HHDFSStatsBase* mirror, NAMemory * heap = 0)
         : OsimHHDFSStatsBase(parent, mirror, heap)
	{}

	virtual const char *getElementName() const
	{ return elemName; }

        virtual NABoolean setValue(HHDFSStatsBase* stats, const char *attrName, const char *attrValue);

protected:
	virtual void serializeAttrs(XMLString & xml);
};

class  OsimHHDFSBucketStats : public OsimHHDFSStatsBase
{
public:
	static const char elemName[];
	OsimHHDFSBucketStats(XMLElement* parent, HHDFSStatsBase* mirror, NAMemory * heap = 0)
         : OsimHHDFSStatsBase(parent, mirror, heap)
	  {}

	virtual const char *getElementName() const
	{ return elemName; }
	
	virtual NABoolean setValue(HHDFSStatsBase* stats, const char *attrName, const char *attrValue);
	
protected:
	virtual void serializeAttrs(XMLString & xml);
	virtual void startElement(void *parser, const char *elementName, const char **atts);
};

class  OsimHHDFSListPartitionStats : public OsimHHDFSStatsBase
{
public:
	static const char elemName[];
	OsimHHDFSListPartitionStats(XMLElement* parent, HHDFSStatsBase* mirror, NAMemory * heap = 0)
          : OsimHHDFSStatsBase(parent, mirror, heap)
         {}

	virtual const char *getElementName() const
	{ return elemName; }
	
        virtual NABoolean setValue(HHDFSStatsBase* stats, const char *attrName, const char *attrValue);

protected:
       virtual void serializeAttrs(XMLString & xml);
	virtual void startElement(void *parser, const char *elementName, const char **atts);

};

class  OsimHHDFSTableStats : public OsimHHDFSStatsBase
{
public:
        static const char elemName[];
        OsimHHDFSTableStats(XMLElement* parent, HHDFSStatsBase * mirror, NAMemory * heap = 0)
          : OsimHHDFSStatsBase(parent, mirror, heap)
	{setParent(this);}
	
        virtual const char *getElementName() const
        { return elemName; }

	virtual NABoolean setValue(HHDFSStatsBase* hhstats, const char *attrName, const char *attrValue);
	
protected:
    virtual void serializeAttrs(XMLString & xml);
    virtual void startElement(void *parser, const char *elementName, const char **atts);
	
};

// This class initializes OSIM and implements capture and simulation
// OSIM is a single-threaded singleton.
class OptimizerSimulator : public NABasicObject
{
  public:
    // The default OSIM mode is OFF and is set to either CAPTURE or
    // SIMULATE by an environment variable OSIM_MODE.
    enum osimMode {
      OFF,
      CAPTURE,
      SIMULATE,
      LOAD,
      UNLOAD
    };

    enum OsimLog {
      FIRST_LOG,
      ESTIMATED_ROWS = FIRST_LOG,
      NODE_AND_CLUSTER_NUMBERS,
      NACLUSTERINFO,
      MYSYSTEMNUMBER,
      VIEWSFILE,
      VIEWDDLS,
      TABLESFILE,
      CREATE_SCHEMA_DDLS,
      CREATE_TABLE_DDLS,
      SYNONYMSFILE,
      SYNONYMDDLS,
      CQD_DEFAULTSFILE,
      QUERIESFILE,
      VERSIONSFILE,
      CAPTURE_SYS_TYPE,
      HISTOGRAM_PATHS,
      HIVE_HISTOGRAM_PATHS,
      HIVE_CREATE_TABLE,
      HIVE_CREATE_EXTERNAL_TABLE,
      HIVE_TABLE_LIST,
      HHDFS_MASTER_HOST_LIST,
      NUM_OF_LOGS
    };

    // sysType indicates the type of system that captured the queries
    enum sysType {
      OSIM_UNKNOWN_SYSTYPE,
      OSIM_NSK,
      OSIM_WINNT,
      OSIM_LINUX
    };
    // Constructor
    OptimizerSimulator(CollHeap *heap);
    // Accessor methods to get and set osimMode_.
    void setOsimMode(osimMode mode) { osimMode_ = mode; }
    osimMode getOsimMode() { return osimMode_; }

    // Accessor methods to get and set osimLogHDFSDir_.
    void setOsimLogdir(const char *localdir);
    const char* getOsimLogdir() const { return osimLogLocalDir_.isNull()? NULL : osimLogLocalDir_.data(); }
    void initHashDictionaries();
    void createLogFilepath(OsimLog sc);
    void openWriteLogStreams(OsimLog sc);
    void initLogFilePaths();

    void capture_CQDs();
    void capture_TableOrView(NATable * naTab);

    void capturePrologue();
    void captureQueryText(const char * query);
    void captureQueryShape(const char * shape);

    void cleanup();
    void cleanupSimulator();
    void cleanupAfterStatement();

    void errorMessage(const char *msg);
    void warningMessage(const char *msg);
    void debugMessage(const char* format, ...);
    
    NABoolean setOsimModeAndLogDir(osimMode mode, const char * localdir);
    NABoolean readHiveStmt(ifstream & DDLFile, NAString & stmt, NAString & comment);
    NABoolean readStmt(ifstream & DDLFile,  NAString & stmt, NAString & comment);
    NABoolean massageTableUID(OsimHistogramEntry* entry, NAHashDictionary<NAString, QualifiedName> * modifiedPathList, NABoolean isHive);
    NABoolean isCallDisabled(ULng32 callBitPosition);

    NABoolean runningSimulation();
    NABoolean runningInCaptureMode();
    NABoolean runningInLoadMode();

    NAHashDictionary<const QualifiedName, Int64> *getHashDictTables() 
      {  return hashDict_Tables_; }

    NAHashDictionary<const QualifiedName, Int64> *getHashDictViews() 
      {  return hashDict_Views_; }

    NAHashDictionary<const QualifiedName, Int64> *getHashDictHiveTables() 
      {  return hashDict_HiveTables_; }
	
    void readSysCallLogfiles();
    
    void capture_getEstimatedRows(const char *tableName, double estRows);
    void readLogfile_getEstimatedRows();
    double simulate_getEstimatedRows(const char *tableName);
    void restoreHHDFSMasterHostList();
   void simulate_getNodeAndClusterNumbers(short& nodeNum, Int32& clusterNum);
   void readLogFile_getNodeAndClusterNumbers();
   void capture_getNodeAndClusterNumbers(short& nodeNum, Int32& clusterNum);
   void log_getNodeAndClusterNumbers();
    
    void readLogFile_captureSysType();
    void captureSysType();
    sysType getCaptureSysType();
    
    void capture_MYSYSTEMNUMBER(short sysNum);
    void readLogfile_MYSYSTEMNUMBER();
    short simulate_MYSYSTEMNUMBER();

    NABoolean isClusterInfoInitialized() { return clusterInfoInitialized_; }

    void setClusterInfoInitialized(NABoolean b) { clusterInfoInitialized_ = b; }

    // File pathnames for log files that contain system call data.
    const char * getLogFilePath (OsimLog index) const
    {  return logFilePaths_[index]; }

    void setForceLoad(NABoolean b) { forceLoad_ = b; }
    NABoolean isForceLoad() const { return forceLoad_; }
    HHDFSTableStats * restoreHiveTableStats(const QualifiedName & qualName, NAMemory* heap, hive_tbl_desc* hvt_desc);
    void captureHiveTableStats(HHDFSTableStats* tablestats, const NATable* naTab);
  private:
    void readAndSetCQDs();
    void dumpHistograms();
    void dumpDDLs(const QualifiedName & qualifiedName);
    void dumpHiveTableDDLs();
    void dumpHiveHistograms();
    void initializeCLI();
    void loadHistograms(const char* histogramPath, NABoolean isHive);
    short loadHistogramsTable(NAString* modifiedPath, QualifiedName * qualifiedName, unsigned int bufLen, NABoolean isHive);
    short loadHistogramIntervalsTable(NAString* modifiedPath, QualifiedName * qualifiedName, unsigned int bufLen, NABoolean isHive);
    Int64 getTableUID(const char * catName, const char * schName, const char * objName);
    short fetchAllRowsFromMetaContext(Queue * &q, const char* query);
    short executeFromMetaContext(const char* query);
    void loadDDLs();
    void loadHiveDDLs();
    void histogramHDFSToLocal();
    void removeHDFSCacheDirectory();
    void createLogDir();
    void dropObjects();
    void dumpVersions();
    void dumpHHDFSMasterHostList();
    void execHiveSQL(const char* hiveSQL);
    
    // This is the directory OSIM uses to read/write log files.
    NAString osimLogLocalDir_;    //OSIM dir in local disk, used during capture and simu mode.
    NAString hiveTableStatsDir_;
    // This is the mode under which OSIM is running (default is OFF).
    // It is set by an environment variable OSIM_MODE.
    osimMode osimMode_;
    
    // System call names.
    static const char *logFileNames_[NUM_OF_LOGS];
    
    char *logFilePaths_[NUM_OF_LOGS];

    ofstream* writeLogStreams_[NUM_OF_LOGS];
    
    NAHashDictionary<NAString, double> *hashDict_getEstimatedRows_;
    NAHashDictionary<const QualifiedName, Int64> *hashDict_Views_;
    NAHashDictionary<const QualifiedName, Int64> *hashDict_Tables_;
    NAHashDictionary<const QualifiedName, Int32> *hashDict_Synonyms_;
    NAHashDictionary<const QualifiedName, Int64> *hashDict_HiveTables_;
    
    short nodeNum_;
    Int32 clusterNum_;
    short mySystemNumber_;
    sysType captureSysType_;
    NABoolean capturedNodeAndClusterNum_;
    NABoolean capturedInitialData_;
    NABoolean hashDictionariesInitialized_;
    NABoolean clusterInfoInitialized_;
    NABoolean tablesBeforeActionInitilized_;
    NABoolean viewsBeforeActionInitilized_;
    NABoolean CLIInitialized_;
    CmpSeabaseDDL * cmpSBD_ ;
    ExeCliInterface * cliInterface_ ;
    Queue * queue_;
    //for debugging
    ULng32 sysCallsDisabled_;
    //in respond to force option of osim load, 
    //e.g. osim load from '/xxx/xxx/osim-dir', force
    //if true, when loading osim tables/views/indexes
    //existing objects with same qualified name 
    //will be droped first
    NABoolean forceLoad_;
    CollHeap *heap_;
};

// System call wrappers.
  void OSIM_restoreHHDFSMasterHostList();
  short OSIM_MYSYSTEMNUMBER();
  void  OSIM_getNodeAndClusterNumbers(short& nodeNum, Int32& clusterNum);
  void  OSIM_captureTableOrView(NATable * naTab);
  void  OSIM_captureHiveTableStats(HHDFSTableStats* tablestats, const NATable * naTab);
  void  OSIM_capturePrologue();
  void  OSIM_captureQueryShape(const char * shape);
  // errorMessage and warningMessage wrappers.
  void OSIM_errorMessage(const char *msg);
  void OSIM_warningMessage(const char *msg);
  HHDFSTableStats * OSIM_restoreHiveTableStats(const QualifiedName & qualName, NAMemory* heap, hive_tbl_desc* hvt_desc);

  // Check this, the parent and all ancestor CmpContext
  // to find out if one of them is running in Load mode.
  NABoolean OSIM_runningLoadEmbedded();

  NABoolean OSIM_runningSimulation();
  NABoolean OSIM_runningLoad();
  NABoolean OSIM_runningInCaptureMode();
  NABoolean OSIM_ustatIsDisabled();
  NABoolean OSIM_ClusterInfoInitialized();
  void raiseOsimException(const char* fmt, ...);
#endif
