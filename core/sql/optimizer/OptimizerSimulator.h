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
// (C) Copyright 2003-2015 Hewlett-Packard Development Company, L.P.
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
// Define PATH_MAX, FILENAME_MAX, LINE_MAX for both NT and NSK.
  // _MAX_PATH and _MAX_FNAME are defined in stdlib.h that is
  // included above.
  #define OSIM_PATHMAX PATH_MAX
  #define OSIM_FNAMEMAX FILENAME_MAX
  #define OSIM_LINEMAX 4096
  
class OsimAllHistograms;
class OsimHistogramEntry;

#define TAG_ALL_HISTOGRAMS "all_histograms"
#define TAG_HISTOGRAM_ENTRY "histogram_entry"
#define TAG_FULL_PATH "fullpath"
#define TAG_USER_NAME "username"
#define TAG_PID "pid"
#define TAG_CATALOG "catalog"
#define TAG_SCHEMA "schema"
#define TAG_TABLE "table"
#define TAG_HISTOGRAM "histogram"
//<TAG_ALL_HISTOGRAMS> 
//  <TAG_HISTOGRAM_ENTRY>
//    <TAG_FULL_PATH>/opt/home/xx/xxx </TAG_FULL_PATH>
//    <TAG_USER_NAME>root</TAG_USER_NAME>
//    <TAG_PID>12345</TAG_PID>
//    <TAG_CATALOG>trafodion</TAG_CATALOG>
//    <TAG_SCHEMA>seabase</TAG_SCHEMA>
//    <TAG_TABLE>order042</TAG_TABLE>
//    <TAG_HISTOGRAM>sb_histogram_interval</TAG_HISTOGRAM>
//  </TAG_HISTOGRAM_ENTRY>
// ...
//</TAG_ALL_HISTOGRAMS>

class OsimAllHistograms : public XMLElement
{
public:
    static const char elemName[];
    OsimAllHistograms(NAMemory * heap = 0)
      : XMLElement(NULL, heap)
      , list_(heap)
    {
        setParent(this);
    }

    virtual const char *getElementName() const
    { return elemName; }
    
    virtual ElementType getElementType() const 
    { return ElementType::ET_List;    }
    
    NAList<OsimHistogramEntry*> & getEntries() { return list_; }

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

    NAList<OsimHistogramEntry*> list_;

private:
    // Copy construction/assignment not defined.
    OsimAllHistograms(const OsimAllHistograms&);
    OsimAllHistograms& operator=(const OsimAllHistograms&);
};

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
    NAString &  getCatalog() {  return catalog_;  }
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
    // Copy construction/assignment not defined.
    OsimHistogramEntry(const OsimHistogramEntry&);
    OsimHistogramEntry& operator=(const OsimHistogramEntry&);
};

class OsimElementMapper : public XMLElementMapper
{
  public:
    virtual XMLElementPtr operator()(void *parser,
                                     char *elementName,
                                     AttributeList atts);
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

    enum sysCall {
      FIRST_SYSCALL,
      ESTIMATED_ROWS = FIRST_SYSCALL,
      NODE_AND_CLUSTER_NUMBERS,
      NACLUSTERINFO,
      NODENAME_TO_NODENUMBER,
      NODENUMBER_TO_NODENAME,
      MYSYSTEMNUMBER,
      VIEWSFILE,
      VIEWDDLS,
      TABLESFILE,
      CREATE_SCHEMA_DDLS,
      CREATE_TABLE_DDLS,
      CREATE_INDEX_DDLS,
      ALTER_TABLE_DDLS,
      SYNONYMSFILE,
      SYNONYMDDLS,
      CQD_DEFAULTSFILE,
      QUERIESFILE,
      VERSIONSFILE,
      CAPTURE_SYS_TYPE,
      HISTOGRAM_PATHS,
      NUM_OF_SYSCALLS
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
    void setOsimLogdir(const char *localdir) {
      if (localdir) {
          osimLogLocalDir_ = localdir;
      }
    }
    const char* getOsimLogdir() const { return osimLogLocalDir_.isNull()? NULL : osimLogLocalDir_.data(); }
    void initHashDictionaries();
    void setLogFilepath(sysCall sc);
    void openAndAddHeaderToLogfile(sysCall sc);
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

    NABoolean readStmt(ifstream & DDLFile,  NAString & stmt, NAString & comment);
    NABoolean massageTableUID(OsimHistogramEntry* entry, NAHashDictionary<NAString, QualifiedName> * modifiedPathList);
    NABoolean isCallDisabled(ULng32 callBitPosition);

    NABoolean runningSimulation();
    NABoolean runningInCaptureMode();

    NAHashDictionary<const QualifiedName, Int64> *getHashDictTables() 
      {  return hashDict_Tables_; }

    NAHashDictionary<const QualifiedName, Int64> *getHashDictViews() 
      {  return hashDict_Views_; }

    void readSysCallLogfiles();
    
    void capture_NODENUMBER_TO_NODENAME(short error, 
                                                                                                                Int32 nodeNumber,
                                                                                                                char *nodeNameStr,
                                                                                                                short maxLen,
                                                                                                                short *actualLen);
    void readLogfile_NODENUMBER_TO_NODENAME();
    short simulate_NODENUMBER_TO_NODENAME(Int32 nodeNumber,
                                                                                                                   char *nodeName,
                                                                                                                   short maxLen,
                                                                                                                   short *actualLen);
    
    

    void capture_NODENAME_TO_NODENUMBER(short error,
                                                                                                               const char *fileName,
                                                                                                               short nodeNameLen,
                                                                                                               Int32 *nodeNumber);
    void readLogfile_NODENAME_TO_NODENUMBER();
    short simulate_NODENAME_TO_NODENUMBER(const char *fileName,
                                                                                                                    short nodeNameLen,
                                                                                                                    Int32 *nodeNumber);
    
    void capture_getEstimatedRows(const char *tableName, double estRows);
    void readLogfile_getEstimatedRows();
    double simulate_getEstimatedRows(const char *tableName);
    
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

    NABoolean isSysParamsInitialized();
    void setSysParamsInitialized(NABoolean b);
    // File pathnames for log files that contain system call data.
    const char * getLogFilePath (sysCall index) const
    {  return sysCallLogFilePath_[index]; }

    void setForceLoad(NABoolean b) { forceLoad_ = b; }
    NABoolean isForceLoad() const { return forceLoad_; }
    
  private:
    
    void readAndSetCQDs();
    void dumpHistograms();
    void dumpDDLs(const QualifiedName & qualifiedName);
    void initializeCLI();
    void loadHistograms();
    short loadHistogramsTable(NAString* modifiedPath, QualifiedName * qualifiedName, unsigned int bufLen);
    short loadHistogramIntervalsTable(NAString* modifiedPath, QualifiedName * qualifiedName, unsigned int bufLen);
    Int64 getTableUID(const char * catName, const char * schName, const char * objName);
    short fetchAllRowsFromMetaTable(Queue * &q, const char* query);
    short executeInMetaContext(const char* query);
    void loadDDLs();
    void histogramHDFSToLocal();
    void removeHDFSCacheDirectory();
    void createLogDir();
    void checkDuplicateNames();
    void dropObjects();
    void dumpVersions();
    void saveTablesBeforeAction();
    void saveViewsBeforeAction();
    void execHiveSQL(const char* hiveSQL);
    // This is the directory OSIM uses to read/write log files.
    NAString osimLogLocalDir_;    //OSIM dir in local disk, used during capture and simu mode.
    // This is the mode under which OSIM is running (default is OFF).
    // It is set by an environment variable OSIM_MODE.
    osimMode osimMode_;
    
    // System call names.
    static const char *sysCallLogFileName_[NUM_OF_SYSCALLS];
    
    char *sysCallLogFilePath_[NUM_OF_SYSCALLS];

    ofstream* writeSysCallStream_[NUM_OF_SYSCALLS];

    NAHashDictionary<NAString, NAString> *hashDict_NODENAME_TO_NODENUMBER_;
    
    NAHashDictionary<Int32, NAString> *hashDict_NODENUMBER_TO_NODENAME_;
    
    NAHashDictionary<NAString, double> *hashDict_getEstimatedRows_;

    NAHashDictionary<const QualifiedName, Int64> *hashDict_Views_;
    NAHashDictionary<const QualifiedName, Int64> *hashDict_Tables_;
    NAHashDictionary<const QualifiedName, Int32> *hashDict_Synonyms_;
    NAHashDictionary<NAString, Int32> * hashDict_TablesBeforeAction_;
    NAHashDictionary<NAString, Int32> * hashDict_ViewsBeforeAction_;
    
    short nodeNum_;
    Int32 clusterNum_;
    short mySystemNumber_;
    sysType captureSysType_;
    NABoolean capturedNodeAndClusterNum_;
    NABoolean capturedInitialData_;
    //NABoolean usingCaptureHint_;
    NABoolean hashDictionariesInitialized_;
    NABoolean sysParamsInitialized_;
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
    HiveClient_JNI* hiveClient_;
    CollHeap *heap_;
};

// System call wrappers.
  short OSIM_NODENAME_TO_NODENUMBER(const char *nodeName,
                                     short nodeNameLen,
                                     Int32 *nodeNumber);
  short OSIM_NODENUMBER_TO_NODENAME(Int32 nodeNumber,
                                     char *nodeName,
                                     short maxLen,
                                     short *actualLen);
  

  short OSIM_MYSYSTEMNUMBER();
  void  OSIM_getNodeAndClusterNumbers(short& nodeNum, Int32& clusterNum);
  void  OSIM_captureTableOrView(NATable * naTab);
  void  OSIM_capturePrologue();
  void  OSIM_captureQueryText(const char * query);
  void  OSIM_captureQueryShape(const char * shape);
  void  OSIM_captureEstimatedRows(const char *tableName, double estRows);
  double OSIM_simulateEstimatedRows(const char *tableName);
  // errorMessage and warningMessage wrappers.
  void OSIM_errorMessage(const char *msg);
  void OSIM_warningMessage(const char *msg);

  NABoolean OSIM_runningSimulation();
  NABoolean OSIM_runningInCaptureMode();
  NABoolean OSIM_ustatIsDisabled();
  NABoolean OSIM_isNTbehavior();
  NABoolean OSIM_isNSKbehavior();
  NABoolean OSIM_isLinuxbehavior();
  NABoolean OSIM_SysParamsInitialized();
#endif
