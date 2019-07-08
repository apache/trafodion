/* -*-C++-*-
/**********************************************************************
*
* File:         OptimizerSimulator.cpp
* Description:  This file is the source file for Optimizer Simulator
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

#include "OptimizerSimulator.h"
#include "NADefaults.h"
#include "CmpContext.h"
#include "CompException.h"
#include "SchemaDB.h"
#include "NATable.h"
#include "ObjectNames.h"
#include "NAClusterInfo.h"
#include "ControlDB.h"
#include "RelControl.h"
#include "CmpStatement.h"
#include "QCache.h"
#include <errno.h>
#include "ComCextdecs.h" 
#include "opt_error.h"
#include "ComRtUtils.h"
#include <cstdlib>
#include <sys/stat.h>
#include <string.h>
#include <dirent.h>
#include <cstdarg>
#include "HBaseClient_JNI.h"

#include "vproc.h"
#include "CmpSeabaseDDL.h"
#include "ExExeUtilCli.h"
#include "ComUser.h"
#include "HDFSHook.h"

#include "Globals.h"
#include "CmpContext.h"
#include "Context.h"

extern THREAD_P NAClusterInfo *gpClusterInfo;
extern const WordAsBits SingleBitArray[];

// Define PATH_MAX, FILENAME_MAX, LINE_MAX for both NT and NSK.
// _MAX_PATH and _MAX_FNAME are defined in stdlib.h that is
// included above.
#define OSIM_PATHMAX PATH_MAX
#define OSIM_FNAMEMAX FILENAME_MAX
#define OSIM_LINEMAX 4096
#define OSIM_HIVE_TABLE_STATS_DIR "/hive_table_stats"
//#define HIVE_CRAETE_TABLE_SQL "/hive_create_table.sql"
//#define HIVE_TABLE_LIST "/hive_table_list.txt"
//the hdfs dir path should start from /bulkload
#define UNLOAD_HDFS_DIR "/user/trafodion/bulkload/osim_capture"
//tags for histograms file path
#define TAG_ALL_HISTOGRAMS "all_histograms"
#define TAG_HISTOGRAM_ENTRY "histogram_entry"
#define TAG_FULL_PATH "fullpath"
#define TAG_USER_NAME "username"
#define TAG_PID "pid"
#define TAG_CATALOG "catalog"
#define TAG_SCHEMA "schema"
#define TAG_TABLE "table"
#define TAG_HISTOGRAM "histogram"
//tags for hive table stats
#define TAG_HHDFSFILESTATS "hhdfs_file_stats"
#define TAG_HHDFSBUCKETSTATS "hhdfs_bucket_stats"
#define TAG_HHDFSLISTPARTSTATS "hhdfs_list_part_stats"
#define TAG_HHDFSTABLESTATS "hhdfs_table_stats"

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
const char OsimAllHistograms::elemName[] = TAG_ALL_HISTOGRAMS;
const char OsimHistogramEntry::elemName[] = TAG_HISTOGRAM_ENTRY;
const char OsimHHDFSFileStats::elemName[] = TAG_HHDFSFILESTATS;
const char OsimHHDFSBucketStats::elemName[] = TAG_HHDFSBUCKETSTATS;
const char OsimHHDFSListPartitionStats::elemName[] = TAG_HHDFSLISTPARTSTATS;
const char OsimHHDFSTableStats::elemName[] = TAG_HHDFSTABLESTATS;

const char* OptimizerSimulator::logFileNames_[NUM_OF_LOGS]= {
  "ESTIMATED_ROWS.txt" ,
  "NODE_AND_CLUSTER_NUMBERS.txt",
  "NAClusterInfo.txt",
  "MYSYSTEMNUMBER.txt",
  "VIEWS.txt" ,
  "VIEWDDLS.txt",
  "TABLES.txt",
  "CREATE_SCHEMA_DDLS.txt",
  "CREATE_TABLE_DDLS.txt" ,
  "SYNONYMS.txt",
  "SYNONYMDDLS.txt",
  "CQDS.txt" ,
  "QUERIES.txt",
  "VERSIONS.txt",
  "CaptureSysType.txt",
  "HISTOGRAM_PATHS.xml",
  "HIVE_HISTOGRAM_PATHS.xml",
  "HIVE_CREATE_TABLE.sql",
  "HIVE_CREATE_EXTERNAL_TABLE.sql",
  "HIVE_TABLE_LIST.txt",
  "HHDFS_MASTER_HOST_LIST.txt"
};

static ULng32 intHashFunc(const Int32& Int)
{
  return (ULng32)Int;
}

static NABoolean isFileExists(const char *filename, NABoolean & isDir)
{
  struct stat sb;
  Int32 rVal = stat(filename, &sb);
  isDir = FALSE;
  if(S_ISDIR(sb.st_mode))
      isDir = TRUE;
  return rVal != -1;
}

void OsimAllHistograms::startElement(void * parser, const char * elementName, const char * * atts)
{
    OsimHistogramEntry* entry = NULL;
    if(!strcmp(elementName, TAG_HISTOGRAM_ENTRY)){
        entry = new (XMLPARSEHEAP) OsimHistogramEntry(this, XMLPARSEHEAP);
        XMLDocument::setCurrentElement(parser, entry);
        list_.insert(entry);
    }
    else
       raiseOsimException("Errors Parsing hitograms file.");
}

void OsimAllHistograms::serializeBody(XMLString& xml)
{//format on my own,  vialote the use of XMLElement.
    for(CollIndex i = 0; i < list_.entries(); i++)
    {
        xml.append("    ").append('<').append(TAG_HISTOGRAM_ENTRY).append('>');
        xml.endLine();
        list_[i]->serializeBody(xml);
        xml.append("    ").append("</").append(TAG_HISTOGRAM_ENTRY).append('>');
        xml.endLine();
    }
}

void OsimAllHistograms::addEntry( const char* fullpath,
                                                                const char* username,
                                                                const char* pid,
                                                                const char* cat,
                                                                const char* sch,
                                                                const char* table,
                                                                const char* histogram)
{
        OsimHistogramEntry * en = new (STMTHEAP) OsimHistogramEntry(this, STMTHEAP);
        en->getFullPath() = fullpath;
        en->getUserName() = username;
        en->getPID() = pid;
        en->getCatalog() = cat;
        en->getSchema() = sch;
        en->getTable() = table;
        en->getHistogram() = histogram;
        list_.insert(en);
}

void OsimHistogramEntry::charData(void *parser, const char *data, Int32 len)
{
     if(!currentTag_.compareTo(TAG_FULL_PATH) )
         fullPath_.append(data, len);
     else if(!currentTag_.compareTo(TAG_USER_NAME) )
         userName_.append(data, len);
     else if(!currentTag_.compareTo(TAG_PID) )
         pid_.append(data, len);
     else if(!currentTag_.compareTo(TAG_CATALOG) )
         catalog_.append(data, len);
     else if(!currentTag_.compareTo(TAG_SCHEMA) )
         schema_.append(data, len);
     else if(!currentTag_.compareTo(TAG_TABLE) )
          table_.append(data, len);
     else if(!currentTag_.compareTo(TAG_HISTOGRAM) )
          histogram_.append(data, len);
}

void OsimHistogramEntry::startElement(void * parser, const char * elementName, const char * * atts)
{
    currentTag_ = elementName;
}

void OsimHistogramEntry::endElement(void * parser, const char * elementName)
{
    if(!strcmp(getElementName(), elementName))
    {
       XMLElement::endElement(parser, elementName);
    }
    currentTag_="";
 }

void OsimHistogramEntry::serializeBody(XMLString& xml)
{
   xml.append("        ");
   xml.append('<').append(TAG_FULL_PATH).append('>');
   xml.appendCharData(fullPath_);
   xml.append("</").append(TAG_FULL_PATH).append('>');
   xml.endLine();
   
   xml.append("        ");
   xml.append('<').append(TAG_USER_NAME).append('>');
   xml.appendCharData(userName_);
   xml.append("</").append(TAG_USER_NAME).append('>');
   xml.endLine();

   xml.append("        ");
   xml.append('<').append(TAG_PID).append('>');
   xml.appendCharData(pid_);
   xml.append("</").append(TAG_PID).append('>');
   xml.endLine();

   xml.append("        ");
   xml.append('<').append(TAG_CATALOG).append('>');
   xml.appendCharData(catalog_);
   xml.append("</").append(TAG_CATALOG).append('>');
   xml.endLine();

   xml.append("        ");
   xml.append('<').append(TAG_SCHEMA).append('>');
   xml.appendCharData(schema_);
   xml.append("</").append(TAG_SCHEMA).append('>');
   xml.endLine();

   xml.append("        ");
   xml.append('<').append(TAG_TABLE).append('>');
   xml.appendCharData(table_);
   xml.append("</").append(TAG_TABLE).append('>');
   xml.endLine();

   xml.append("        ");
   xml.append('<').append(TAG_HISTOGRAM).append('>');
   xml.appendCharData(histogram_);
   xml.append("</").append(TAG_HISTOGRAM).append('>');
   xml.endLine();
   
}

XMLElementPtr OsimElementMapper::operator()(void *parser,
                                          char *elementName,
                                          AttributeList atts)
{
  XMLElementPtr elemPtr = NULL;
  //atts is not used here
  if (!strcmp( elementName, "all_histograms"))
    elemPtr = new (XMLPARSEHEAP) OsimAllHistograms(XMLPARSEHEAP);
  return elemPtr;
}

/////////////////////////////////////////////////////////////////////////
OptimizerSimulator::OptimizerSimulator(CollHeap *heap)
:osimLogLocalDir_(heap),
 osimMode_(OptimizerSimulator::OFF),
 hashDict_getEstimatedRows_(NULL),
 hashDict_Views_(NULL),
 hashDict_Tables_(NULL),
 hashDict_Synonyms_(NULL),
 hashDict_HiveTables_(NULL),
 nodeNum_(-1),
 clusterNum_(-1),
 captureSysType_(OSIM_LINUX),
 mySystemNumber_(-1),
 capturedNodeAndClusterNum_(FALSE),
 capturedInitialData_(FALSE),
 hashDictionariesInitialized_(FALSE),
 clusterInfoInitialized_(FALSE),
 tablesBeforeActionInitilized_(FALSE),
 viewsBeforeActionInitilized_(FALSE),
 CLIInitialized_(FALSE),
 cmpSBD_(NULL),
 cliInterface_(NULL),
 queue_(NULL),
 sysCallsDisabled_(0),
 forceLoad_(FALSE),
 heap_(heap)
{
  for (OsimLog sc=FIRST_LOG; sc<NUM_OF_LOGS; sc = OsimLog(sc+1))
  {
    logFilePaths_[sc]=NULL;
    writeLogStreams_[sc]=NULL;
  }
}

// Print OSIM error message
void OSIM_errorMessage(const char *errMsg)
{
  if(CURRCONTEXT_OPTSIMULATOR)
      CURRCONTEXT_OPTSIMULATOR->errorMessage(errMsg);
}

void OptimizerSimulator::errorMessage(const char *errMsg)
{
  // ERROR message
  *CmpCommon::diags() << DgSqlCode(-OSIM_ERRORORWARNING)
                      << DgString0(errMsg);
}

// Print OSIM warning message
void OSIM_warningMessage(const char *errMsg)
{
  if(CURRCONTEXT_OPTSIMULATOR)
      CURRCONTEXT_OPTSIMULATOR->warningMessage(errMsg);
}

void OptimizerSimulator::warningMessage(const char *errMsg)
{
  *CmpCommon::diags() << DgSqlCode(OSIM_ERRORORWARNING)
                      << DgString0(errMsg);
}

void OptimizerSimulator::debugMessage(const char* format, ...)
{
    char* debugLog = getenv("OSIM_DEBUG_LOG");
    FILE *stream = stdout;
    if(debugLog) stream = fopen(debugLog,"a+");
    va_list argptr;
    fprintf(stream, "[OSIM]");
    va_start(argptr, format);
    vfprintf(stream, format, argptr);
    va_end(argptr);
    if(debugLog) fclose(stream);
}

void OptimizerSimulator::dumpVersions()
{
    //dump version info
    NAString cmd = "sqvers -u > ";
    cmd += logFilePaths_[VERSIONSFILE];
    system(cmd.data()); //dump versions
}

void OptimizerSimulator::dumpHHDFSMasterHostList()
{
    (*writeLogStreams_[HHDFS_MASTER_HOST_LIST])
           << "HasVirtualSQNodes" 
           << " "
           << HHDFSMasterHostList::hasVirtualSQNodes() << endl;
    (*writeLogStreams_[HHDFS_MASTER_HOST_LIST])
           << "NumSQNodes" 
           << " "
           << HHDFSMasterHostList::getNumSQNodes() << endl;
    NAString hostNameList;
    for(Int32 i = 0; i < HHDFSMasterHostList::entries(); i++)
    {  
        hostNameList += HHDFSMasterHostList::getHostName(i);
        hostNameList += '|'; //delimiter
    }
    if(hostNameList.length() > 0)
    { 
        //if there's any hostname, 
        //hostNameList would be like "node0|node1|node2|",
        //this is to replace '|'.
        hostNameList[hostNameList.length()-1] = '\n';
        (*writeLogStreams_[HHDFS_MASTER_HOST_LIST]) << hostNameList.data();
    }
}

void raiseOsimException(const char* fmt, ...)
{
    char * buffer;
    va_list args ;
    va_start(args, fmt);
    buffer = NAString::buildBuffer(fmt, args);
    va_end(args);
    //throw anyway null buffer will be handled inside constructor of
    //OsimLogException, empty string will be issued.
    OsimLogException(buffer, __FILE__, __LINE__).throwException();
}

NABoolean OptimizerSimulator::setOsimModeAndLogDir(osimMode targetMode, const char * localDir)
{
  try{
      if(targetMode == UNLOAD)
      {
         setOsimMode(targetMode);
         setOsimLogdir(localDir);
         initLogFilePaths();
         setOsimMode(OFF);
         dropObjects();
         cleanup();
         return TRUE;
      }

      switch(osimMode_)
      {
        case OFF:
              switch(targetMode)
              {
                  case CAPTURE: // OFF --> CAPTURE
                      setOsimLogdir(localDir);
                      setOsimMode(targetMode);//mode must be set before initialize
                      NADefaults::updateSystemParameters(TRUE);
                      createLogDir();
                      initHashDictionaries();
                      initLogFilePaths();
                      setClusterInfoInitialized(TRUE);
                      break;
                  case LOAD: //OFF --> LOAD
                      setOsimMode(targetMode);
                      setOsimLogdir(localDir);
                      initHashDictionaries();
                      initLogFilePaths();
                      loadDDLs();
                      loadHistograms(logFilePaths_[HISTOGRAM_PATHS], FALSE);
                      loadHiveDDLs();
                      loadHistograms(logFilePaths_[HIVE_HISTOGRAM_PATHS], TRUE);
                      break;
                  case SIMULATE: //OFF-->SIMU
                      setOsimMode(targetMode);
                      setOsimLogdir(localDir);
                      initLogFilePaths();
                      initHashDictionaries();
                      readSysCallLogfiles();
                      //reinitialize NAClusterInfoLinux and CQDs
                      NADefaults::updateSystemParameters(TRUE);
                      //apply cqds
                      readAndSetCQDs();
                      setClusterInfoInitialized(TRUE);
                      break;
              } 
              break;
        case CAPTURE:
            if(targetMode == OFF) //CAPURE --> OFF only
            {  
                //call dumpHHDFSMasterHostList() first
                //otherwise context-switch will reset
                //HHDFSMasterHostList::hasVirtualSQNodes_
                //HHDFSMasterHostList::numSQNodes_
                dumpHHDFSMasterHostList();
                dumpHiveTableDDLs();
                dumpHiveHistograms();
                dumpHistograms();
                dumpVersions();
                setOsimMode(targetMode);
                cleanup();//NOTE: osimMode_ is set OFF in cleanup()
            }
            else
                warningMessage("Mode transition is not allowed.");
            break;
        case LOAD:
            if(targetMode == SIMULATE)//LOAD --> SIMU only
            {
                setOsimMode(targetMode);
                readSysCallLogfiles();
                NADefaults::updateSystemParameters(TRUE);
                //apply CQDs
                readAndSetCQDs();
                setClusterInfoInitialized(TRUE);
            }
            else
                warningMessage("Mode transition other than LOAD to SIMULATE is not allowed.");
            break;
        default :
            warningMessage("Mode transition is not allowed.");
            break;
      }
  }
  catch(OsimLogException & e)
  {
      cleanup();
      //move err string from exception object to diagnostic area
      errorMessage(e.getErrMessage());
      return FALSE;
  }
  catch(...)
  {
      cleanup();
      errorMessage("Unknown OSIM error.");
      return FALSE;
  }
  return TRUE;
}

void OptimizerSimulator::dumpDDLs(const QualifiedName & qualifiedName)
{
    short retcode;
    Queue * outQueue = NULL;
    NAString query(STMTHEAP);
    debugMessage("Dumping DDL for %s\n", qualifiedName.getQualifiedNameAsAnsiString().data());
        
    query = "SHOWDDL " + qualifiedName.getQualifiedNameAsAnsiString();
        
    retcode = fetchAllRowsFromMetaContext(outQueue, query.data());
    if (retcode < 0 || retcode == 100/*rows not found*/) {
           cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
           raiseOsimException("Errors Dumping Table DDL.");
    }

    if(outQueue)
    {
        ofstream * createSchema = writeLogStreams_[CREATE_SCHEMA_DDLS];
    
        ofstream * createTable = writeLogStreams_[CREATE_TABLE_DDLS];
            
        //Dump a "create schema ..." to schema ddl file for every table.
        
        //This comment line will be printed during loading, ';' must be omitted
        (*createSchema) << "--" <<"CREATE SCHEMA IF NOT EXISTS " 
                                                << qualifiedName.getCatalogName() 
                                                << "." << qualifiedName.getSchemaName() <<endl;
    
        (*createSchema) << "CREATE SCHEMA IF NOT EXISTS " 
                                   << qualifiedName.getCatalogName() 
                                   <<"."<< qualifiedName.getSchemaName() 
                                   << ";" << endl;
        
        // skippingSystemGeneratedIndex is set to TRUE to avoid generating redundant
        // DDL for system-generated indexes
        NABoolean skippingSystemGeneratedIndex = FALSE;
        outQueue->position();//rewind
        for (int i = 0; i < outQueue->numEntries(); i++) {
            OutputInfo * vi = (OutputInfo*)outQueue->getNext();
            char * ptr = vi->get(0);
            if (strcmp(ptr,"\n-- The following index is a system created index --") == 0)
              skippingSystemGeneratedIndex = TRUE;

            if (!skippingSystemGeneratedIndex)
            {
                // skip heading newline, and add a comment line
                // for the DDL text upto the first trailing '\n'
                Int32 ix = 0;
                for(; ptr[ix]=='\n'; ix++);
                if( strstr(ptr, "CREATE TABLE") ||
                    strstr(ptr, "CREATE INDEX") ||
                    strstr(ptr, "CREATE UNIQUE INDEX") ||
                    strstr(ptr, "ALTER TABLE")  )

                {
                  (*createTable) << "--";
                  char* x = ptr+ix;
                  while ( (*x) && *x != '\n' ) {
                    (*createTable) << *x;
                    x++;
                  } 
                  (*createTable) << endl;
                }

                //output ddl  
                (*createTable) << ptr << endl;
            }

            if (skippingSystemGeneratedIndex && (strcmp(ptr,";") == 0)) // at end of DDL to be skipped?
              skippingSystemGeneratedIndex = FALSE;
        }
    }
}

void OptimizerSimulator::dumpHistograms()
{
    short retcode;
    const QualifiedName* name = NULL;
    Int64*  tableUID = NULL;
    NAString query(STMTHEAP);

    NAHashDictionaryIterator<const QualifiedName, Int64> iterator(*hashDict_Tables_);

    OsimAllHistograms* histoInfoList = new (STMTHEAP) OsimAllHistograms(STMTHEAP);
    NAString fullPath(STMTHEAP);
    //enumerate captured table names and tableUIDs in hash table
    for(iterator.getNext(name, tableUID); name && tableUID; iterator.getNext(name, tableUID))
    {
        //check if this table_uid is in TRAFODION."_HIVESTATS_".SB_HISTOGRAMS,
        //if not, we consider this table has no histogram data.
        Queue * outQueue = NULL;
        query = "SELECT TABLE_UID FROM TRAFODION.";
        query += name->getSchemaName();
        query += ".SB_HISTOGRAMS WHERE TABLE_UID = ";
        query += std::to_string((long long)(*tableUID)).c_str();
        retcode = fetchAllRowsFromMetaContext(outQueue, query.data());
        if(retcode < 0 || outQueue && outQueue->entries() == 0)
            continue;

        debugMessage("Dumping histograms for %s\n", name->getQualifiedNameAsAnsiString().data());
        //dump histograms data to hdfs
        query =   "UNLOAD WITH NULL_STRING '\\N' INTO ";
        query +=  "'" UNLOAD_HDFS_DIR"/";
        query +=  ComUser::getCurrentUsername();
        query +=  "/";
        query +=  std::to_string((long long unsigned int)(getpid())).c_str();
        query += "/";
        query += name->getQualifiedNameAsAnsiString();
        query += ".SB_HISTOGRAMS'";
        query += " SELECT TABLE_UID"                        
                    ", HISTOGRAM_ID"      
                    ", COL_POSITION"  
                    ", COLUMN_NUMBER" 
                    ", COLCOUNT"
                    ", INTERVAL_COUNT"
                    ", ROWCOUNT"
                    ", TOTAL_UEC"
                    ", STATS_TIME"
                    ", TRANSLATE(LOW_VALUE USING UCS2TOUTF8)"
                    ", TRANSLATE(HIGH_VALUE USING UCS2TOUTF8)"
                    ", READ_TIME"
                    ", READ_COUNT"
                    ", SAMPLE_SECS" 
                    ", COL_SECS"
                    ", SAMPLE_PERCENT"
                    ", CV,REASON, V1, V2, V3, V4"
                    ", TRANSLATE(V5 USING UCS2TOUTF8)"
                    ", TRANSLATE(V6 USING UCS2TOUTF8)"
                    " FROM ";
        query += name->getCatalogName();
        query += ".";
        if(name->getSchemaName()[0]=='_')
        {
            query += "\"";
            query += name->getSchemaName();
            query += "\"";
        }
        else
            query += name->getSchemaName();
        query += ".SB_HISTOGRAMS WHERE TABLE_UID = ";
        query += std::to_string((long long)(*tableUID)).c_str();
                            
        retcode = executeFromMetaContext(query.data());
        
        if(retcode >= 0)
        {    
            fullPath = osimLogLocalDir_;
            fullPath += "/";
            fullPath += ComUser::getCurrentUsername();
            fullPath += "/";
            fullPath += std::to_string((long long unsigned int)(getpid())).c_str();
            fullPath += "/";
            fullPath += name->getQualifiedNameAsAnsiString();
            fullPath += ".SB_HISTOGRAMS";
            histoInfoList->addEntry( fullPath.data(),
                                       ComUser::getCurrentUsername(),
                                       std::to_string((long long unsigned int)(getpid())).c_str(),
                                       name->getCatalogName().data(),
                                       name->getSchemaName().data(),
                                       name->getObjectName().data(),
                                       "SB_HISTOGRAMS");
        }
        //ignore -4082, 
        //which means histogram tables are not exist,
        //i.e. update stats hasn't been done for any table.
        else if(retcode < 0 && -4082 != retcode)
        {
           cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
           raiseOsimException("Unload histogram data error: %d", retcode);
        }
        
        query =  "UNLOAD WITH NULL_STRING '\\N' INTO ";
        query += "'" UNLOAD_HDFS_DIR"/";
        query += ComUser::getCurrentUsername();
        query += "/";
        query += std::to_string((long long unsigned int)(getpid())).c_str();
        query += "/";
        query += name->getQualifiedNameAsAnsiString();
        query += ".SB_HISTOGRAM_INTERVALS'";
        query += " SELECT TABLE_UID"                        
                    ", HISTOGRAM_ID"      
                    ", INTERVAL_NUMBER"  
                    ", INTERVAL_ROWCOUNT" 
                    ", INTERVAL_UEC"
                    ", TRANSLATE(INTERVAL_BOUNDARY USING UCS2TOUTF8)"
                    ", STD_DEV_OF_FREQ"
                    ", V1, V2, V3, V4"
                    ", TRANSLATE(V5 USING UCS2TOUTF8)"
                    ", TRANSLATE(V6 USING UCS2TOUTF8)"
                    " FROM ";
        query += name->getCatalogName();
        query += ".";
        if(name->getSchemaName()[0]=='_')
        {
            query += "\"";
            query += name->getSchemaName();
            query += "\"";
        }
        else
            query += name->getSchemaName();
        query += ".SB_HISTOGRAM_INTERVALS WHERE TABLE_UID = ";
        query += std::to_string((long long)(*tableUID)).c_str();
         
        retcode = executeFromMetaContext(query.data());

        if(retcode >= 0)
        {            
            fullPath = osimLogLocalDir_;
            fullPath += "/";
            fullPath += ComUser::getCurrentUsername();
            fullPath += "/";
            fullPath += std::to_string((long long unsigned int)(getpid())).c_str();
            fullPath += "/";
            fullPath += name->getQualifiedNameAsAnsiString();
            fullPath += ".SB_HISTOGRAM_INTERVALS";
            histoInfoList->addEntry( fullPath.data(),
                                       ComUser::getCurrentUsername(),
                                       std::to_string((long long unsigned int)(getpid())).c_str(),
                                       name->getCatalogName().data(),
                                       name->getSchemaName().data(),
                                       name->getObjectName().data(),
                                       "SB_HISTOGRAM_INTERVALS");
        }
        //ignore -4082, 
        //which means histogram tables are not exist,
        //i.e. update stats hasn't been done for any table.
        else if(retcode < 0 && -4082 != retcode)
        {
           cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
           raiseOsimException("Unload histogram data error: %d", retcode);
        }
    
    }

    //Do not use XMLFormatString as we do format ourself
    XMLString* xmltext = new (STMTHEAP) XMLString(STMTHEAP);
    histoInfoList->toXML(*xmltext);
    (*writeLogStreams_[HISTOGRAM_PATHS]) << xmltext->data() << endl;
    NADELETE(xmltext, XMLString, STMTHEAP);
    //copy histograms data from hdfs to osim directory.
    histogramHDFSToLocal();
}

void OptimizerSimulator::dumpHiveHistograms()
{
    short retcode;
    const QualifiedName* name = NULL;
    Int64*  tableUID = NULL;
    NAString query(STMTHEAP);

    NAHashDictionaryIterator<const QualifiedName, Int64> iterator(*hashDict_HiveTables_);

    OsimAllHistograms* histoInfoList = new (STMTHEAP) OsimAllHistograms(STMTHEAP);
    NAString fullPath(STMTHEAP);
    //enumerate captured table names and tableUIDs in hash table
    for(iterator.getNext(name, tableUID); name && tableUID; iterator.getNext(name, tableUID))
    {
        //check if this table_uid is in TRAFODION."_HIVESTATS_".SB_HISTOGRAMS,
        //if not, we consider this table has no histogram data.
        Queue * outQueue = NULL;
        query = "SELECT TABLE_UID FROM TRAFODION.\"_HIVESTATS_\".SB_HISTOGRAMS WHERE TABLE_UID = ";
        query += std::to_string((long long)(*tableUID)).c_str();
        retcode = fetchAllRowsFromMetaContext(outQueue, query.data());
        if(retcode < 0 || outQueue && outQueue->entries() == 0)
            continue;
        
        debugMessage("Dumping histograms for %s\n", name->getQualifiedNameAsAnsiString().data());
        
        //dump histograms data to hdfs
        query =   "UNLOAD WITH NULL_STRING '\\N' INTO ";
        query +=  "'" UNLOAD_HDFS_DIR"/";
        query +=  ComUser::getCurrentUsername();
        query +=  "/";
        query +=  std::to_string((long long unsigned int)(getpid())).c_str();
        query += "/";
        query += name->getQualifiedNameAsAnsiString();
        query += ".SB_HISTOGRAMS'";
        query += " SELECT TABLE_UID"                        
                    ", HISTOGRAM_ID"      
                    ", COL_POSITION"  
                    ", COLUMN_NUMBER" 
                    ", COLCOUNT"
                    ", INTERVAL_COUNT"
                    ", ROWCOUNT"
                    ", TOTAL_UEC"
                    ", STATS_TIME"
                    ", TRANSLATE(LOW_VALUE USING UCS2TOUTF8)"
                    ", TRANSLATE(HIGH_VALUE USING UCS2TOUTF8)"
                    ", READ_TIME"
                    ", READ_COUNT"
                    ", SAMPLE_SECS" 
                    ", COL_SECS"
                    ", SAMPLE_PERCENT"
                    ", CV,REASON, V1, V2, V3, V4"
                    ", TRANSLATE(V5 USING UCS2TOUTF8)"
                    ", TRANSLATE(V6 USING UCS2TOUTF8)"
                    " FROM TRAFODION.\"_HIVESTATS_\".SB_HISTOGRAMS WHERE TABLE_UID = ";
        query += std::to_string((long long)(*tableUID)).c_str();
                            
        retcode = executeFromMetaContext(query.data());
        //succeed
        if(retcode >= 0)
        {    
            fullPath = osimLogLocalDir_;
            fullPath += "/";
            fullPath += ComUser::getCurrentUsername();
            fullPath += "/";
            fullPath += std::to_string((long long unsigned int)(getpid())).c_str();
            fullPath += "/";
            fullPath += name->getQualifiedNameAsAnsiString();
            fullPath += ".SB_HISTOGRAMS";
            histoInfoList->addEntry( fullPath.data(),
                                       ComUser::getCurrentUsername(),
                                       std::to_string((long long unsigned int)(getpid())).c_str(),
                                       name->getCatalogName().data(),
                                       name->getSchemaName().data(),
                                       name->getObjectName().data(),
                                       "SB_HISTOGRAMS");
        }
        //ignore -4082, 
        //which means histogram tables are not exist,
        //i.e. update stats hasn't been done for any table.
        else if(retcode < 0 && -4082 != retcode)
        {
           cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
           raiseOsimException("Unload histogram data error: %d", retcode);
        }
        
        query =  "UNLOAD WITH NULL_STRING '\\N' INTO ";
        query += "'" UNLOAD_HDFS_DIR"/";
        query += ComUser::getCurrentUsername();
        query += "/";
        query += std::to_string((long long unsigned int)(getpid())).c_str();
        query += "/";
        query += name->getQualifiedNameAsAnsiString();
        query += ".SB_HISTOGRAM_INTERVALS'";
        query += " SELECT TABLE_UID"                        
                    ", HISTOGRAM_ID"      
                    ", INTERVAL_NUMBER"  
                    ", INTERVAL_ROWCOUNT" 
                    ", INTERVAL_UEC"
                    ", TRANSLATE(INTERVAL_BOUNDARY USING UCS2TOUTF8)"
                    ", STD_DEV_OF_FREQ"
                    ", V1, V2, V3, V4"
                    ", TRANSLATE(V5 USING UCS2TOUTF8)"
                    ", TRANSLATE(V6 USING UCS2TOUTF8)"
                    " FROM TRAFODION.\"_HIVESTATS_\".SB_HISTOGRAM_INTERVALS WHERE TABLE_UID = ";
        query += std::to_string((long long)(*tableUID)).c_str();
         
        retcode = executeFromMetaContext(query.data());

        if(retcode >= 0)
        {            
            fullPath = osimLogLocalDir_;
            fullPath += "/";
            fullPath += ComUser::getCurrentUsername();
            fullPath += "/";
            fullPath += std::to_string((long long unsigned int)(getpid())).c_str();
            fullPath += "/";
            fullPath += name->getQualifiedNameAsAnsiString();
            fullPath += ".SB_HISTOGRAM_INTERVALS";
            histoInfoList->addEntry( fullPath.data(),
                                       ComUser::getCurrentUsername(),
                                       std::to_string((long long unsigned int)(getpid())).c_str(),
                                       name->getCatalogName().data(),
                                       name->getSchemaName().data(),
                                       name->getObjectName().data(),
                                       "SB_HISTOGRAM_INTERVALS");
        }
        //ignore -4082, 
        //which means histogram tables are not exist,
        //i.e. update stats hasn't been done for any table.
        else if(retcode < 0 && -4082 != retcode)
        {
           cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
           raiseOsimException("Unload histogram data error: %d", retcode);
        }
    
    }

    //Do not use XMLFormatString as we do format ourself
    XMLString* xmltext = new (STMTHEAP) XMLString(STMTHEAP);
    histoInfoList->toXML(*xmltext);
    (*writeLogStreams_[HIVE_HISTOGRAM_PATHS]) << xmltext->data() << endl;
    NADELETE(xmltext, XMLString, STMTHEAP);
    //copy histograms data from hdfs to osim directory.
    histogramHDFSToLocal();
}

void OptimizerSimulator::dropObjects()
{
   short retcode;
   ifstream tables(logFilePaths_[TABLESFILE]);
   if(!tables.good())
       raiseOsimException("Error open %s", logFilePaths_[TABLESFILE]);

   std::string stdQualTblNm;//get qualified table name from file
   NAString query(STMTHEAP);
   while(tables.good())
   {
      //read one line
      std::getline(tables, stdQualTblNm);
      // eofbit is not set until an attempt is made to read beyond EOF.
      // Exit the loop if there was no data to read above.
      if(!tables.good())
         break;
      //if table name is in existance
      query = "DROP TABLE IF EXISTS ";
      query += stdQualTblNm.c_str();
      query += " CASCADE;";
      debugMessage("%s\n", query.data());
      retcode = executeFromMetaContext(query.data());
      if(retcode < 0)
      {
          cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
          raiseOsimException("Drop Table %s error: %d", stdQualTblNm.c_str(), retcode);
      }
   }

   std::string str;
   Int64 uid;
   const QualifiedName * qualName = NULL;
   Int64 * tableUID;
   NAString trafName;
   std::ifstream hiveTableListFile(logFilePaths_[HIVE_TABLE_LIST]);
  //we only need one loop, no need to populate hashDict_HiveTables_
   while(hiveTableListFile.good())
   {
         // read tableName and uid from the file
         hiveTableListFile >> str >> uid;
         // eofbit is not set until an attempt is made to read beyond EOF.
         // Exit the loop if there was no data to read above.
         if(!hiveTableListFile.good())
           break;
         NAString name = str.c_str();
         qualName = new (heap_) QualifiedName(name,3);
         trafName = ComConvertNativeNameToTrafName(qualName->getCatalogName(),
                                                   qualName->getSchemaName(),
                                                   qualName->getObjectName());
          QualifiedName qualTrafName(trafName,3);
          //drop external table
          NAString dropStmt = "DROP TABLE IF EXISTS ";
          dropStmt += trafName;
          debugMessage("%s\n", dropStmt.data());                                                               
          retcode = executeFromMetaContext(dropStmt.data());
          if(retcode < 0)
          {
              cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
              raiseOsimException("drop external table: %d", retcode);
          }
          //unregister hive table
          NAString unregisterStmt = "UNREGISTER HIVE TABLE IF EXISTS ";
          unregisterStmt += name;
          debugMessage("%s\n", unregisterStmt.data());
          retcode = executeFromMetaContext(unregisterStmt.data());
          if(retcode < 0)
          {
              //suppress errors for now, even with IF EXISTS this will
              //give an error if the Hive table does not exist
              //cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
              //raiseOsimException("unregister hive table: %d", retcode);
          }
          //drop hive table
          NAString hiveSchemaName;
          qualName->getHiveSchemaName(hiveSchemaName);
          dropStmt = "DROP TABLE IF EXISTS ";
          dropStmt += hiveSchemaName;
          dropStmt +=  '.';
          dropStmt += qualName->getObjectName();
          debugMessage("%s\n", dropStmt.data());
          execHiveSQL(dropStmt.data());//drop hive table
   }
}

void OptimizerSimulator::loadDDLs()
{
    debugMessage("loading tables and views ...\n");
    short retcode;

    //If force option is present, 
    //drop tables with same names, otherwise rollback
    //if(isForceLoad())
    //    dropObjects();
    //else
    //    checkDuplicateNames();
    dropObjects();

    NAString statement(STMTHEAP);
    NAString comment(STMTHEAP);
    statement.capacity(4096);
    comment.capacity(4096);

    //Step 1:
    //Fetch and execute "create schema ..." from schema ddl file.
    debugMessage("Step 1 Create Schemas:\n");
    ifstream createSchemas(logFilePaths_[CREATE_SCHEMA_DDLS]);
    if(!createSchemas.good())
    {
        raiseOsimException("Error open %s", logFilePaths_[CREATE_SCHEMA_DDLS]);
    }    
    while(readStmt(createSchemas, statement, comment))
    {
        if(comment.length() > 0)
            debugMessage("%s\n", comment.data());
        if(statement.length() > 0)
            retcode = executeFromMetaContext(statement.data());
        //ignore error of creating schema, which might already exist.
    }
    
    //Step 2:
    //Fetch and execute "create table ... "  from table ddl file.
    debugMessage("Step 2 Create Tables:\n");
    ifstream createTables(logFilePaths_[CREATE_TABLE_DDLS]);
    if(!createTables.good())
    {
        raiseOsimException("Error open %s", logFilePaths_[CREATE_TABLE_DDLS]);
    }    
    while(readStmt(createTables, statement, comment))
    {
        if(comment.length() > 0)
            debugMessage("%s\n", comment.data());
        if(statement.length() > 0){
            retcode = executeFromMetaContext(statement.data());
            if(retcode < 0)
            {
                cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
                raiseOsimException("Create Table Error: %d", retcode);
            }
        }
    }

    //Step 3:
    //Fetch and execute "create view ..." from view ddl file.
    debugMessage("Step 3 Create Views:\n");
    ifstream createViews(logFilePaths_[VIEWDDLS]);
    if(!createViews.good())
    {
        raiseOsimException("Error open %s", logFilePaths_[VIEWDDLS]);
    }  

    while(readStmt(createViews, statement, comment))
    {
        if(comment.length() > 0)
            debugMessage("%s\n", comment.data());
        if(statement.length() > 0){
            retcode = executeFromMetaContext(statement.data());
            if(retcode < 0)
            {
                cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
                raiseOsimException("Create View Error: %d %s", retcode, statement.data());
            }
        }
    }
}

static const char* extractAsComment(const char* header, const NAString & stmt)
{
    NAString tmp;
    int begin = stmt.index(header);
    if(begin > -1)
    {
        int end = stmt.index('\n', begin);
        if(end > begin)
          end -= 1;
        else
          end = stmt.length()-1;

        stmt.extract(begin, end, tmp);
        return tmp.data();
    }
    return NULL;
}

void OptimizerSimulator::loadHiveDDLs()
{
    debugMessage("creating hive tables ...\n");
    short retcode;
    NAString statement(STMTHEAP);
    NAString comment(STMTHEAP);
    statement.capacity(4096);
    comment.capacity(4096);
    std::ifstream hiveCreateTableSql(logFilePaths_[HIVE_CREATE_TABLE]);
    std::ifstream hiveTableListFile(logFilePaths_[HIVE_TABLE_LIST]);
    std::ifstream hiveCreateExternalTableSql(logFilePaths_[HIVE_CREATE_EXTERNAL_TABLE]);
    if(!hiveTableListFile.good() || !hiveCreateTableSql.good())
       return;

    //read hive sql file and create hive table
    std::string str;
    Int64 uid;
    const QualifiedName * qualName = NULL;
    Int64 * tableUID;
    int counter = 0;
    NAString trafName;

    while(hiveTableListFile.good())
    {
          // read tableName and uid from the file
          hiveTableListFile >> str >> uid;
          // eofbit is not set until an attempt is made to read beyond EOF.
          // Exit the loop if there was no data to read above.
          if(!hiveTableListFile.good())
            break;
          NAString name = str.c_str();
          qualName = new (heap_) QualifiedName(name,3);
          tableUID = new Int64(uid);
          hashDict_HiveTables_->insert(qualName, tableUID);
    }

    NAHashDictionaryIterator<const QualifiedName, Int64> iterator(*hashDict_HiveTables_);
    //create hive schema and trafodion external schema and 
    //drop external tables and hive tables with same names
    for(iterator.getNext(qualName, tableUID); qualName && tableUID; iterator.getNext(qualName, tableUID))
    {
        trafName = ComConvertNativeNameToTrafName(
                                                                        qualName->getCatalogName(),
                                                                        qualName->getSchemaName(),
                                                                        qualName->getObjectName());

        QualifiedName qualTrafName(trafName,3);
         //create external table schema
         NAString create_ext_schema = "CREATE SCHEMA IF NOT EXISTS ";
         create_ext_schema += qualTrafName.getCatalogName();
         create_ext_schema += ".\"";
         create_ext_schema += qualTrafName.getSchemaName();
         create_ext_schema += "\" AUTHORIZATION DB__ROOT";
         retcode = executeFromMetaContext(create_ext_schema);
         if(retcode < 0) {
             cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
             raiseOsimException("create hive external schema: %d %s", retcode, statement.data());
         }
         //drop external table
         NAString dropStmt = "DROP TABLE IF EXISTS ";
         dropStmt += trafName;
         retcode = executeFromMetaContext(dropStmt.data());
         if(retcode < 0)
         {
             cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
             raiseOsimException("drop external table: %d", retcode);
         }

         //create hive schema
         NAString hiveSchemaName;
         qualName->getHiveSchemaName(hiveSchemaName);
         NAString create_hive_schema = "CREATE SCHEMA IF NOT EXISTS ";
         create_hive_schema += hiveSchemaName;
         execHiveSQL(create_hive_schema.data());
         //drop hive table
         dropStmt = "DROP TABLE IF EXISTS ";
         dropStmt += hiveSchemaName;
         dropStmt +=  '.';
         dropStmt += qualName->getObjectName();
         execHiveSQL(dropStmt.data());//drop hive table
    }
    //create hive table
   debugMessage("Begin creating hive tables\n");

    while(readHiveStmt(hiveCreateTableSql, statement, comment))
    {   
        if(statement.length() > 0)
        {
            debugMessage("%s\n", extractAsComment("CREATE TABLE", statement));
            execHiveSQL(statement.data());//create hive table
            debugMessage("done\n");
        }
    }
            
   debugMessage("Begin creating hive external tables\n");

    //create external table
    while(readHiveStmt(hiveCreateExternalTableSql, statement, comment))
   {
        if(statement.length() > 0) {
            // this could be a create external table or just a register table
            // if this Hive table just has stats but no external table
            const char *stmtText = extractAsComment("CREATE EXTERNAL TABLE", statement);

            if (!stmtText)
              stmtText = extractAsComment("REGISTER  HIVE TABLE", statement);
            debugMessage("%s\n", stmtText);

            retcode = executeFromMetaContext(statement.data()); //create hive external table
            if(retcode < 0)
            {
                cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
                raiseOsimException("Create hive external table error:  %d", retcode);
            }
            debugMessage("done\n");
        }
   }
}

//============================================================================
// This method writes the information related to the NAClusterInfo class to a
// logfile called "NAClusterInfo.txt".
//============================================================================
void NAClusterInfo::captureNAClusterInfo(ofstream & naclfile)
{
  CollIndex i, ci;
  char filepath[OSIM_PATHMAX];
  char filename[OSIM_FNAMEMAX];

  // We don't capture data members that are computed during the compilation of
  // a query. These include:
  //
  // * smpCount_;
  // * tableToClusterMap_;
  // * activeClusters_;
  //

  naclfile << "localCluster_: " << LOCAL_CLUSTER << endl
           << "localSMP_: " << localSMP_ << endl;
  // number of clusters and their CPU (node) ids
  // (right now there is always 1 cluster)
  naclfile << "clusterToCPUMap_: 1 :" << endl;

  // Write the header line for the table.
  naclfile << "  ";
  naclfile.width(10); 
  naclfile << "clusterNum" << "  ";
  naclfile << "cpuList" << endl;

  naclfile << "  ";
  naclfile.width(10); naclfile << LOCAL_CLUSTER << "  ";
  naclfile << cpuArray_.entries() << " : ";
  for (ci=0; ci<cpuArray_.entries(); ci++)
    {
      naclfile.width(3); naclfile << cpuArray_[ci] << " ";
    }
  naclfile << endl;

  Int32 * nodeID = NULL;
  NAString* nodeName = NULL;
  NAHashDictionaryIterator<Int32, NAString> nodeNameAndIDIter (*nodeIdToNodeNameMap_);
  naclfile << "nodeIdAndNodeNameMap: " << nodeNameAndIDIter.entries() << endl;
  for(nodeNameAndIDIter.getNext(nodeID, nodeName); nodeID && nodeName; nodeNameAndIDIter.getNext(nodeID, nodeName))
  {
      naclfile << *nodeID << " " << nodeName->data() << endl;
  }

  // Now save the OS-specific information to the NAClusterInfo.txt file
  captureOSInfo(naclfile);
}

//============================================================================
// This method reads the information needed for NAClusterInfo class from
// a logfile called "NAClusterInfo.txt" and then populates the variables
// accordigly.
//============================================================================
void NAClusterInfo::simulateNAClusterInfo()
{
  Int32 i, ci;
  char var[256];

  const char* filepath = CURRCONTEXT_OPTSIMULATOR->getLogFilePath(OptimizerSimulator::NACLUSTERINFO);

  cpuArray_.clear();

  ifstream naclfile(filepath);

  if(!naclfile.good())
  {
    raiseOsimException("Unable to open %s file for reading data.", filepath);
  }
  
  while(naclfile.good())
  {
    // Read the variable name from the file.
    naclfile.getline(var, sizeof(var), ':');
    if(!strcmp(var, "localCluster_"))
    {
      int dummyLocalCluster;

      naclfile >> dummyLocalCluster; naclfile.ignore(OSIM_LINEMAX, '\n');
    }
    else if (!strcmp(var, "localSMP_"))
    {
      naclfile >> localSMP_; naclfile.ignore(OSIM_LINEMAX, '\n');
    }
    else if (!strcmp(var, "clusterToCPUMap_"))
    {
      Int32 numClusters, clusterNum, cpuArray_entries, cpuNum;

      naclfile >> numClusters; naclfile.ignore(OSIM_LINEMAX, '\n');
      // we don't support multiple clusters at this time
      CMPASSERT(numClusters <= 1);
      if(numClusters > 0)
      {
        // Read and ignore the header line.
        naclfile.ignore(OSIM_LINEMAX, '\n');
        for (i=0; i<numClusters; i++)
        {
          naclfile >> clusterNum;
          naclfile >> cpuArray_entries; naclfile.ignore(OSIM_LINEMAX, ':');
          for (ci=0; ci<cpuArray_entries; ci++)
          {
            naclfile >> cpuNum;
            cpuArray_.insertAt(ci, cpuNum);
          }
          naclfile.ignore(OSIM_LINEMAX, '\n');
        }
      }
    }
    else if(!strcmp(var, "nodeIdAndNodeNameMap"))
    {
      Int32 id_name_entries;
      Int32 nodeId;
      char nodeName[256];
      nodeIdToNodeNameMap_ = new(heap_) NAHashDictionary<Int32, NAString>
                                                          (&intHashFunc, 101,TRUE,heap_);
                                                          
      nodeNameToNodeIdMap_ = new(heap_) NAHashDictionary<NAString, Int32>
                                                          (&NAString::hash, 101,TRUE,heap_);
      naclfile >> id_name_entries;
      naclfile.ignore(OSIM_LINEMAX, '\n');
      for(i = 0; i < id_name_entries; i++)
      {
          naclfile >> nodeId >> nodeName;
          naclfile.ignore(OSIM_LINEMAX, '\n');
          
          //populate clusterId<=>clusterName map from file
          Int32 * key_nodeId = new Int32(nodeId);
          NAString * val_nodeName = new (heap_) NAString(nodeName, heap_);
          Int32 * retId = nodeIdToNodeNameMap_->insert(key_nodeId, val_nodeName);
          //CMPASSERT(retId);
          
          NAString * key_nodeName = new (heap_) NAString(nodeName, heap_);
          Int32 * val_nodeId = new Int32(nodeId);
          NAString * retName = nodeNameToNodeIdMap_->insert(key_nodeName, val_nodeId);
          //some node names are like g4t3024:0, g4t3024:1
          //I don't know why we need to remove strings after ':' or '.' in node name,
          //but if string after ':' or '.' is removed, same node names correspond to different node ids,
          //this can cause problems here
          //CMPASSERT(retName);
      }
    }
    else
    {
      // This variable will either be read in simulateNAClusterInfoNSK()
      // method of NAClusterInfoNSK class or is not the one that we want
      // to read here in this method. So discard it and continue.
      naclfile.ignore(OSIM_LINEMAX, '\n');
      while (naclfile.peek() == ' ')
      {
        // The main variables are listed at the beginning of a line
        // with additional information indented. If one or more spaces
        // are seen at the beginning of the line upon the entry to this
        // while loop, it is because of that additional information.
        // So, ignore this line since the variable is being ignored.
        naclfile.ignore(OSIM_LINEMAX, '\n');
      }
    }
  }
}

void NAClusterInfoLinux::simulateNAClusterInfoLinux()
{
  char var[256];
  
  const char* filepath = CURRCONTEXT_OPTSIMULATOR->getLogFilePath(OptimizerSimulator::NACLUSTERINFO);

  ifstream nacllinuxfile(filepath);

  if(!nacllinuxfile.good())
  {
        raiseOsimException("Unable to open %s file for reading data.", filepath);
  }

  while(nacllinuxfile.good())
  {
    // Read the variable name from the file
    nacllinuxfile.getline(var, sizeof(var), ':');
    if(!strcmp(var, "frequency_"))
    {
      nacllinuxfile >> frequency_; nacllinuxfile.ignore(OSIM_LINEMAX, '\n');
    }
    else if (!strcmp(var, "iorate_"))
    {
      nacllinuxfile >> iorate_; nacllinuxfile.ignore(OSIM_LINEMAX, '\n');
    }
    else if (!strcmp(var, "seekTime_"))
    {
      nacllinuxfile >> seekTime_; nacllinuxfile.ignore(OSIM_LINEMAX, '\n');
    }
    else if (!strcmp(var, "pageSize_"))
    {
      nacllinuxfile >> pageSize_; nacllinuxfile.ignore(OSIM_LINEMAX, '\n');
    }
    else if (!strcmp(var, "totalMemoryAvailable_"))
    {
      nacllinuxfile >> totalMemoryAvailable_; nacllinuxfile.ignore(OSIM_LINEMAX, '\n');
    }
    else if (!strcmp(var, "numCPUcoresPerNode_"))
    {
      nacllinuxfile >> numCPUcoresPerNode_; nacllinuxfile.ignore(OSIM_LINEMAX, '\n');
    }
    else
    {
      // This variable either may have been read in simulateNAClusterInfo()
      // method of NAClusterInfo class or is not the one that we want to
      // read here in this method. So discard it.
      nacllinuxfile.ignore(OSIM_LINEMAX, '\n');
      while (nacllinuxfile.peek() == ' ')
      {
        // The main variables are listed at the beginning of a line
        // with additional information indented. If one or more spaces
        // are seen at the beginning of the line upon the entry to this
        // while loop, it is because of that additional information.
        // So, ignore this line since the variable is being ignored.
        nacllinuxfile.ignore(OSIM_LINEMAX, '\n');
      }
    }
  }
}

//use local table UID to replace UID in captured histogram file,
//then read from files to histogram.
NABoolean OptimizerSimulator::massageTableUID(OsimHistogramEntry* entry, NAHashDictionary<NAString, QualifiedName> * modifiedPathList, NABoolean isHive)
{
  int retcode;
  NAString tmp = osimLogLocalDir_ + '/';
  tmp += entry->getUserName() + '/';
  tmp += entry->getPID() + '/';
  tmp += entry->getCatalog() + '.';
  tmp += entry->getSchema() + '.';
  tmp += entry->getTable() + '.';
  tmp += entry->getHistogram();
  const char* fullPath = tmp.data();
  
  const char* catalog = entry->getCatalog();
  const char* schema = entry->getSchema();
  const char* table = entry->getTable();
  const char* histogramTableName = entry->getHistogram();
  
  NAString * UIDModifiedPath = new (STMTHEAP) NAString(STMTHEAP);
  //qualifiedName is used to create histogram tables
  QualifiedName* qualifiedName;

  qualifiedName = new (STMTHEAP) QualifiedName(histogramTableName, schema, catalog, STMTHEAP);

  Int64 tableUID;
  //in _MD_.OBJECTS, schema of hive table is _HV_HIVE_, catalog is TRAFODION
  if(isHive)
  {
      NAString trafName;
      trafName = ComConvertNativeNameToTrafName(
                                                                        entry->getCatalog(),
                                                                        entry->getSchema(),
                                                                        entry->getTable());
      QualifiedName qname (trafName, 3);
      tableUID = getTableUID(qname.getCatalogName(), qname.getSchemaName(), qname.getObjectName());
  }
  else
      tableUID = getTableUID(catalog, schema, table);

  if(tableUID < 0)
  {
        raiseOsimException("Get Table UID Error: %d", tableUID);
  }
  NAString dataPath(STMTHEAP);
  //get text file path within the dir
  DIR * histogramDir = opendir(fullPath);
  if(!histogramDir)
  {
        raiseOsimException("Error open %s", fullPath);
  } 
  struct dirent * dataPathInfo = readdir(histogramDir);
  while(dataPathInfo != NULL)
  {
      if(dataPathInfo->d_name[0] != '.' && dataPathInfo->d_type != DT_DIR)
      {//there should be only one
          dataPath = fullPath;
          dataPath += '/';
          dataPath += dataPathInfo->d_name;
          break;
      }
      dataPathInfo = readdir(histogramDir);
  }
  closedir(histogramDir);
  
  *UIDModifiedPath = osimLogLocalDir_;
  *UIDModifiedPath += '/';
  *UIDModifiedPath += catalog;
  *UIDModifiedPath += '.';
  *UIDModifiedPath += schema;
  *UIDModifiedPath += '.';
  *UIDModifiedPath += histogramTableName;
  *UIDModifiedPath += ".modified";

  //pass modified file and qualified histogram table name out 
  if(!modifiedPathList->contains(UIDModifiedPath))
  {
      unlink(UIDModifiedPath->data());
      modifiedPathList->insert(UIDModifiedPath, qualifiedName);
  }

  //open append
  std::ifstream infile (dataPath.data(), std::ifstream::binary);
  if(!infile.good())
  {
        raiseOsimException("Error open %s", dataPath.data());
  } 
  std::ofstream outfile (UIDModifiedPath->data(), std::ofstream::binary|std::ofstream::app);
  //update table UID between files
  NAString uidstr;
  NAList<NAString> fields(STMTHEAP);
  NAString oneLine(STMTHEAP);
  uidstr.format("%ld", tableUID);
  while(oneLine.readLine(infile) > 0)
  {
    oneLine.split('|', fields);
    //dumped fields of sb_histograms or sb_histogram_intervals
    //should have at least 3 or more.
    if(fields.entries() > 3)
    {
        //replace table uid column 
        //with valid table uid in target instance.
        fields[0] = uidstr;

        //replace V5, V6 with string "empty" if they are null
        NAString & V5 = fields[fields.entries() - 2];
        NAString & V6 = fields[fields.entries() - 1];
        
        if(V5.length() == 0)
            V5 = "empty";

        if(V6.strip(NAString::trailing, '\n').length() == 0)
            V6 = "empty";

        //then output the modified oneLine
        for(CollIndex i = 0; i < fields.entries() - 1; i++)
        {
            outfile << fields[i] << '|';
        }
        outfile << V6 << endl;
    }
    else
        raiseOsimException("Invalid format of histogram data.");
  }

  return TRUE;
}

void OptimizerSimulator::execHiveSQL(const char* hiveSQL)
{
    if (HiveClient_JNI::executeHiveSQL(hiveSQL) != HVC_OK)
    {
        NAString error("Error running hive SQL. ");
        const char * jniErrorStr = GetCliGlobals()->getJniErrorStr();
        if (jniErrorStr)
          error += jniErrorStr;
        raiseOsimException(error.data());
    }
}

short OptimizerSimulator::loadHistogramsTable(NAString* modifiedPath, QualifiedName * qualifiedName, unsigned int bufLen, NABoolean isHive)
{
    debugMessage("loading %s\n", qualifiedName->getQualifiedNameAsString().data());
    short retcode;

    NAString cmd(STMTHEAP);
    //drop hive sb_histogram table
    cmd = "drop table "+qualifiedName->getCatalogName()+"_"+qualifiedName->getSchemaName()+"_"+qualifiedName->getObjectName();
    execHiveSQL(cmd.data());
    //create hive sb_histogram table
    cmd =      "create table " + qualifiedName->getCatalogName()+"_"+qualifiedName->getSchemaName()+"_"+qualifiedName->getObjectName();
    cmd +=     " (table_uid              bigint"
                      ",histogram_id             int"
                      ",col_position              int"
                      ",column_number           int"
                      ",colcount                  int"
                      ",interval_count       smallint"
                      ",rowcount              bigint"
                      ",total_uec              bigint"
                      ",stats_time           timestamp"
                      ",low_value               string"
                      ",high_value              string"
                      ",read_time          timestamp"
                      ",read_count           smallint"
                      ",sample_secs            bigint"
                      ",col_secs                bigint"
                      ",sample_percent       smallint"
                      ",cv                      double"
                      ",reason                  string"
                      ",v1                      bigint"
                      ",v2                      bigint"
                      ",v3                      bigint"
                      ",v4                      bigint"
                      ",v5                      string"
                      ",v6                      string"
                      " ) row format delimited fields terminated by '|' "
                      "tblproperties ('serialization.null.format' = '\\N')";
    execHiveSQL(cmd.data());
    //populate hive table with table UID modified file
    cmd =       "load data local inpath '" + *modifiedPath + "' into table ";
    cmd +=      qualifiedName->getCatalogName()+"_"+qualifiedName->getSchemaName()+"_"+qualifiedName->getObjectName();
    execHiveSQL(cmd.data());
    
    //create sb_histograms
    cmd =      "CREATE TABLE IF NOT EXISTS ";
    if(isHive)
        cmd += "TRAFODION.\"_HIVESTATS_\"." + qualifiedName->getObjectName();
    else
        cmd += qualifiedName->getQualifiedNameAsString();
    cmd +=      " (  TABLE_UID      LARGEINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , HISTOGRAM_ID   INT UNSIGNED NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , COL_POSITION   INT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , COLUMN_NUMBER  INT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , COLCOUNT       INT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , INTERVAL_COUNT SMALLINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , ROWCOUNT       LARGEINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , TOTAL_UEC      LARGEINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , STATS_TIME     TIMESTAMP(0) NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , LOW_VALUE      VARCHAR(250) CHARACTER SET UCS2 COLLATE DEFAULT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , HIGH_VALUE     VARCHAR(250) CHARACTER SET UCS2 COLLATE DEFAULT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , READ_TIME      TIMESTAMP(0) NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , READ_COUNT     SMALLINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , SAMPLE_SECS    LARGEINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , COL_SECS       LARGEINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , SAMPLE_PERCENT SMALLINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , CV             FLOAT(54) NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , REASON         CHAR(1) CHARACTER SET ISO88591 COLLATE DEFAULT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , V1             LARGEINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , V2             LARGEINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , V3             LARGEINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , V4             LARGEINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , V5             VARCHAR(250) CHARACTER SET UCS2 COLLATE DEFAULT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , V6             VARCHAR(250) CHARACTER SET UCS2 COLLATE DEFAULT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                          "  , constraint " HBASE_HIST_PK" primary key"
                          "  (TABLE_UID ASC, HISTOGRAM_ID ASC, COL_POSITION ASC)"
                          " )";
    retcode = executeFromMetaContext(cmd.data());
    if(retcode < 0)
    {
        cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
        raiseOsimException("Load histogram data error:  %d", retcode);
    }
    
    cmd = "upsert using load into ";
    if(isHive)
        cmd += "TRAFODION.\"_HIVESTATS_\"." + qualifiedName->getObjectName();
    else
        cmd += qualifiedName->getQualifiedNameAsString();
    //from hive table to trafodion table
    cmd +=" select TABLE_UID, HISTOGRAM_ID, COL_POSITION, COLUMN_NUMBER, "
                             "COLCOUNT, INTERVAL_COUNT, ROWCOUNT, TOTAL_UEC, STATS_TIME, "
                             "CAST(LOW_VALUE AS VARCHAR(250) CHARACTER SET UCS2 NOT NULL), "
                             "CAST(HIGH_VALUE AS VARCHAR(250) CHARACTER SET UCS2 NOT NULL), "
                             "READ_TIME, READ_COUNT, SAMPLE_SECS, COL_SECS, SAMPLE_PERCENT, "
                             "CV, REASON, V1, V2, V3, V4, V5, V6 from hive.hive.";
    cmd +=qualifiedName->getCatalogName()+"_"+qualifiedName->getSchemaName()+"_"+qualifiedName->getObjectName(); 
    retcode = executeFromMetaContext(cmd.data());
    
    if(retcode < 0)
    {
        cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
        raiseOsimException("Load histogram data error:  %d", retcode);
    }
    
    return retcode;
}

short OptimizerSimulator::loadHistogramIntervalsTable(NAString* modifiedPath, QualifiedName * qualifiedName, unsigned int bufLen, NABoolean isHive)
{
    debugMessage("loading %s\n", qualifiedName->getQualifiedNameAsString().data());
    short retcode;

    NAString cmd(STMTHEAP);
    //drop hive histogram table
    cmd = "drop table "+qualifiedName->getCatalogName()+"_"+qualifiedName->getSchemaName()+"_"+qualifiedName->getObjectName();
    execHiveSQL(cmd.data());
    //create hive histogram table
    cmd  =    "create table " + qualifiedName->getCatalogName()+"_"+qualifiedName->getSchemaName()+"_"+qualifiedName->getObjectName();
    cmd +=    " (  table_uid              bigint"
                   ",histogram_id             int"
                   ",interval_number          int"
                   ",interval_rowcount       bigint"
                   ",interval_uec            bigint"
                   ",interval_boundary      string"
                   ",std_dev_of_freq       int"
                   ",v1                      bigint"
                   ",v2                      bigint"
                   ",v3                      bigint"
                   ",v4                      bigint"
                   ",v5                      string"
                   ",v6                      string"
                   " ) row format delimited fields terminated by '|' "
                   "tblproperties ('serialization.null.format' = '\\N')";
    execHiveSQL(cmd.data());
    
    cmd =     "load data local inpath '" + *modifiedPath + "' into table ";
    cmd +=   qualifiedName->getCatalogName()+"_"+qualifiedName->getSchemaName()+"_"+qualifiedName->getObjectName();
    execHiveSQL(cmd.data());
    
    //create sb_histogram_intervals
    cmd =      "CREATE TABLE IF NOT EXISTS ";
    if(isHive)
        cmd +="TRAFODION.\"_HIVESTATS_\"." + qualifiedName->getObjectName();
    else
        cmd +=  qualifiedName->getQualifiedNameAsString();
    cmd +=           " (  TABLE_UID         LARGEINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                       "  , HISTOGRAM_ID      INT UNSIGNED NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                       "  , INTERVAL_NUMBER   SMALLINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                       "  , INTERVAL_ROWCOUNT LARGEINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                       "  , INTERVAL_UEC      LARGEINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                       "  , INTERVAL_BOUNDARY VARCHAR(250) CHARACTER SET UCS2 COLLATE DEFAULT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                       "  , STD_DEV_OF_FREQ   NUMERIC(12, 3) NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                       "  , V1                LARGEINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                       "  , V2                LARGEINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                       "  , V3                LARGEINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                       "  , V4                LARGEINT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                       "  , V5                VARCHAR(250) CHARACTER SET UCS2 COLLATE DEFAULT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                       "  , V6                VARCHAR(250) CHARACTER SET UCS2 COLLATE DEFAULT NO DEFAULT NOT NULL NOT DROPPABLE NOT SERIALIZED"
                       "  , constraint " HBASE_HISTINT_PK" primary key"
                       "     (TABLE_UID ASC, HISTOGRAM_ID ASC, INTERVAL_NUMBER ASC)"
                       " )";

    retcode = executeFromMetaContext(cmd.data());
    if(retcode < 0)
    {
        cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
        raiseOsimException("Load histogram data error:  %d", retcode);
    }
    //from hive table to trafodion table.
    cmd = "upsert using load into ";
    if(isHive)
        cmd +="TRAFODION.\"_HIVESTATS_\"." + qualifiedName->getObjectName();
    else
        cmd +=qualifiedName->getQualifiedNameAsString();
    //from hive table to trafodion table
    cmd += " select * from hive.hive.";
    cmd += qualifiedName->getCatalogName()+"_"+qualifiedName->getSchemaName()+"_"+qualifiedName->getObjectName();
    retcode = executeFromMetaContext(cmd.data());
    if(retcode < 0)
    {
        cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
        raiseOsimException("Load histogram data error:  %d", retcode);
    }
    return retcode;
}

void OptimizerSimulator::loadHistograms(const char* histogramPath, NABoolean isHive)
{
    debugMessage("loading histograms ...\n");

    OsimElementMapper om;
    OsimAllHistograms * allHistograms = NULL;
    XMLDocument doc(STMTHEAP, om);
    std::ifstream s (histogramPath, std::ifstream::binary);
    if(!s.good())
    {
        raiseOsimException("Error open %s", histogramPath);
    }   
    char * txt = new (STMTHEAP) char[1024];
    s.read(txt, 1024);
    while(s.gcount() > 0)
    {
       if(s.gcount() < 1024)
       {
           allHistograms = (OsimAllHistograms *)doc.parse(txt, s.gcount(), 1);
           break;
       }
       else
          allHistograms = (OsimAllHistograms *)doc.parse(txt, s.gcount(), 0);
       s.read(txt, 1024);
   }
   if(!allHistograms)
   {
        raiseOsimException("Error parsing %s", histogramPath);
   } 
   NAHashDictionary<NAString, QualifiedName> * modifiedPathDict = 
                                        new(STMTHEAP) NAHashDictionary<NAString, QualifiedName>
                                                           (&NAString::hash, 101, TRUE, STMTHEAP);
                              
   for(CollIndex i = 0; i < allHistograms->getHistograms().entries(); i++)
   {  
       OsimHistogramEntry * en = (allHistograms->getHistograms())[i];
       massageTableUID(en, modifiedPathDict, isHive);
   }
       
   //do load
   NAHashDictionaryIterator<NAString, QualifiedName> iterator(*modifiedPathDict);
   //create _HIVESTATS_ schema
   if(isHive && iterator.entries() > 0)
   {
       //create hive stats schema
       const char * create_stats_schema = "CREATE SCHEMA IF NOT EXISTS "
                                                                 "TRAFODION.\"_HIVESTATS_\" AUTHORIZATION DB__ROOT";
       short retcode = executeFromMetaContext(create_stats_schema);
       if(retcode < 0)
       {
            cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
            raiseOsimException("create hive stats schema: return code %d", retcode);
       }
   }
   NAString* modifiedPath = NULL;
   QualifiedName* qualifiedName = NULL;
   Queue * dummyQueue = NULL;
   for (iterator.getNext(modifiedPath, qualifiedName); modifiedPath && qualifiedName; iterator.getNext(modifiedPath, qualifiedName))
   {
       if(qualifiedName->getObjectName().compareTo("SB_HISTOGRAMS", NAString::ignoreCase) == 0)
       {
           loadHistogramsTable(modifiedPath, qualifiedName, OSIM_LINEMAX, isHive);
       }
       else if(qualifiedName->getObjectName().compareTo("SB_HISTOGRAM_INTERVALS", NAString::ignoreCase) == 0)
       {
           loadHistogramIntervalsTable(modifiedPath, qualifiedName, OSIM_LINEMAX, isHive);
       }
       unlink(modifiedPath->data());
   }
}



void OptimizerSimulator::initializeCLI()
{
   if(!CLIInitialized_)
   {   
       cmpSBD_ = new (STMTHEAP) CmpSeabaseDDL(STMTHEAP);
       cliInterface_ = new (STMTHEAP) ExeCliInterface(STMTHEAP);
       queue_ = NULL;
       CLIInitialized_ = TRUE;
   }
}

void OptimizerSimulator::readAndSetCQDs()
{
   initializeCLI();
   NABoolean isDir;
   if(!isFileExists(logFilePaths_[CQD_DEFAULTSFILE], isDir))
   {
         raiseOsimException("Unable to open %s file for reading data.", logFilePaths_[CQD_DEFAULTSFILE]);
   }

   ifstream inLogfile(logFilePaths_[CQD_DEFAULTSFILE]);
   if(!inLogfile.good())
   {
        raiseOsimException("Error open %s", logFilePaths_[CQD_DEFAULTSFILE]);
   } 

   Lng32 retcode;
   std::string cqd;
   while(inLogfile.good())
   {
       //read one line
       std::getline(inLogfile, cqd);
       // eofbit is not set until an attempt is made to read beyond EOF.
       // Exit the loop if there was no data to read above.
       if(!inLogfile.good())
         break;
       retcode = cliInterface_->executeImmediate(cqd.c_str());
       if(retcode < 0)
       {
           cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
           raiseOsimException("Error Setting CQD: %s", cqd.c_str());
       }
   }
}

Int64 OptimizerSimulator::getTableUID(const char * catName, const char * schName, const char * objName)
{
   initializeCLI();
   
   Int64 retcode;
   if (cmpSBD_->switchCompiler(CmpContextInfo::CMPCONTEXT_TYPE_META)) {
       raiseOsimException("Errors Switch Context.");
   }
   
   retcode = cmpSBD_->getObjectUID(cliInterface_, catName, schName, objName, "BT");

   cmpSBD_->switchBackCompiler();

   return retcode;
}

short OptimizerSimulator::fetchAllRowsFromMetaContext(Queue * &q, const char* query)
{   
   initializeCLI();
   
   short retcode;
   if (cmpSBD_->switchCompiler(CmpContextInfo::CMPCONTEXT_TYPE_META)) {
       raiseOsimException("Errors Switch Context.");
   }

   retcode = cliInterface_->fetchAllRows(queue_, query, 0, FALSE, FALSE, TRUE);
   //retrieve idag area runing the query above,
   //if there's any error, we can get the detail.
   cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());

   cmpSBD_->switchBackCompiler();
   
   q = queue_;

   return retcode;
   
}

short OptimizerSimulator::executeFromMetaContext(const char* query)
{
    Queue* dummy = NULL;
    return fetchAllRowsFromMetaContext(dummy, query);
}

//Get a complete SQL statement and a line of comment in front of the SQL statement
NABoolean OptimizerSimulator::readStmt(ifstream & DDLFile, NAString & stmt, NAString & comment)
{
     char a = ' ';
     long index = 0;
     stmt = "";
     comment = "";
     enum readState
     {
          PROBABLY_COMMENT, CONSUME, EAT_CHAR, EOSTMT, EOFILE
     };
     readState state = EAT_CHAR;
     while(1)
     {
        switch(state)
        {
            case EAT_CHAR:
                DDLFile.get(a);
                if(DDLFile.eof())
                    state = EOFILE;
                else if(a == '-')
                    state = PROBABLY_COMMENT;
                else if(a == ';') //end of statement
                    state = EOSTMT;
                else
                    stmt += a;
                break;
            case PROBABLY_COMMENT :
            {
                char b = ' ';
                DDLFile.get(b);
                if( b == '-' )
                    state = CONSUME;
                else //not comment
                {
                    stmt += a;
                    stmt += b;
                    state = EAT_CHAR;
                }
                break;
            }                
            case CONSUME:
                //comment line, eat up rest of the line
                while(DDLFile.get(a))
                {
                    if(a == '\n'){
                        state = EAT_CHAR;
                        break;
                    }
                    else if(DDLFile.eof()){
                        state = EOFILE;
                        break;
                    }
                    else
                       comment += a;
                }
                break;
            case EOSTMT:
                return TRUE;
            case EOFILE:
                return FALSE;
        }
     }
}

//Get a complete SQL statement and a line of comment in front of the SQL statement
NABoolean OptimizerSimulator::readHiveStmt(ifstream & DDLFile, NAString & stmt, NAString & comment)
{
     char a = ' ';
     long index = 0;
     stmt = "";
     comment = "";
     enum readState
     {
          PROBABLY_COMMENT, CONSUME, EAT_CHAR, EOSTMT, EOFILE
     };
     readState state = EAT_CHAR;
     while(1)
     {
        switch(state)
        {
            case EAT_CHAR:
                DDLFile.get(a);
                if(DDLFile.eof())
                    state = EOFILE;
                else if(a == '/')
                    state = PROBABLY_COMMENT;
                else if(a == ';') //end of statement
                    state = EOSTMT;
                else
                    stmt += a;
                break;
            case PROBABLY_COMMENT :
            {
                char b = ' ';
                DDLFile.get(b);
                if( b == '*' )
                    state = CONSUME;
                else //not comment
                {
                    stmt += a;
                    stmt += b;
                    state = EAT_CHAR;
                }
                break;
            }                
            case CONSUME:
                //comment line, eat up rest of the line
                while(DDLFile.get(a))
                {
                    if(a == '*'){
                        char b = ' ';
                        DDLFile.get(b);
                        if(b == '/') //end of comment
                            state = EAT_CHAR;
                        else
                            comment += b;
                        break;
                    }
                    else if(DDLFile.eof()){
                        state = EOFILE;
                        break;
                    }
                    else
                       comment += a;
                }
                break;
            case EOSTMT:
                return TRUE;
            case EOFILE:
                return FALSE;
        }
     }
}


void OptimizerSimulator::histogramHDFSToLocal()
{
    Int32 status;
    struct hdfsBuilder * srcBld = hdfsNewBuilder();
    //build locfs handle
    hdfsBuilderSetNameNode(srcBld, NULL);
    hdfsBuilderSetNameNodePort(srcBld, 0);
    hdfsFS locfs = hdfsBuilderConnect(srcBld);
    //build hdfs handle
    struct hdfsBuilder * dstBld = hdfsNewBuilder();
    hdfsBuilderSetNameNode(dstBld, "default");
    hdfsBuilderSetNameNodePort(dstBld, 0);
    hdfsFS hdfs = hdfsBuilderConnect(dstBld);

    //copy file from hdfs to local one by one
    int numEntries = 0;
    NAString src(STMTHEAP);
    NAString dst(STMTHEAP);
    
    src = UNLOAD_HDFS_DIR"/";
    src += ComUser::getCurrentUsername();
    src += '/';
    src += std::to_string((long long unsigned int)(getpid())).c_str();
    src += '/';

    hdfsFileInfo * info = hdfsListDirectory(hdfs, src.data(), &numEntries);

    for(int i = 0; i < numEntries; i++)
    {
        char * p = strstr(info[i].mName, UNLOAD_HDFS_DIR"/");

        p += strlen(UNLOAD_HDFS_DIR"/");
        
        src = UNLOAD_HDFS_DIR"/";
        src += p;
        
        dst = osimLogLocalDir_ + '/';
        dst += p;

        status = hdfsCopy(hdfs, src.data(), locfs, dst.data());
        if(status != 0)
        {
            raiseOsimException("Error getting histogram data from %s to %s", src.data(), dst.data());
        }
    }
        
    if( hdfsDisconnect(locfs) != 0 ||
         hdfsDisconnect(hdfs) != 0)
    {
            raiseOsimException("Error getting histogram data, disconneting");
    }
}

void OptimizerSimulator::removeHDFSCacheDirectory()
{    
    //build hdfs handle
    struct hdfsBuilder * hdfsBld = hdfsNewBuilder();
    hdfsBuilderSetNameNode(hdfsBld, "default");
    hdfsBuilderSetNameNodePort(hdfsBld, 0);
    hdfsFS hdfs = hdfsBuilderConnect(hdfsBld);    

    //it's ok to fail as this directory may not exist.
    hdfsDelete(hdfs, UNLOAD_HDFS_DIR, 1);
    
    hdfsDisconnect(hdfs);
}

void OptimizerSimulator::createLogDir()
{
  removeHDFSCacheDirectory();
  //create local dir
  Int32 rval = mkdir(osimLogLocalDir_.data(), S_IRWXU | S_IRWXG );

  Int32 error = errno;

  if (rval != 0)
    switch (error)
    {
      case EACCES:
        {
               raiseOsimException("Could not create directory %s, permission denied.", osimLogLocalDir_.data());
        }
        break;
      case ENOENT:
        {
              raiseOsimException("Could not create directory %s, a component of the path does not exist.", osimLogLocalDir_.data());
        }
        break;
      case EROFS:
        {
              raiseOsimException("Could not create directory %s, read-only filesystem.", osimLogLocalDir_.data());
        }
        break;
      case ENOTDIR:
        {
             raiseOsimException("Could not create directory %s, a component of the path is not a directory.", osimLogLocalDir_.data());
        }
        break;
      default:
        {
             raiseOsimException("Could not create %s, errno is %d", osimLogLocalDir_.data(), error);
        }
        break;
    }
    rval = mkdir(hiveTableStatsDir_.data(), S_IRWXU | S_IRWXG );
}

void OptimizerSimulator::readSysCallLogfiles()
{
  readLogfile_MYSYSTEMNUMBER();
  readLogfile_getEstimatedRows();
  readLogFile_getNodeAndClusterNumbers();
  readLogFile_captureSysType();
}

void OptimizerSimulator::setOsimLogdir(const char *localdir) {
  if (localdir) {
      osimLogLocalDir_ = localdir;
      hiveTableStatsDir_ = osimLogLocalDir_ + OSIM_HIVE_TABLE_STATS_DIR;
  }
}

void OptimizerSimulator::initHashDictionaries()
{
  // Initialize hash dictionary variables for all the system calls.
  if(!hashDictionariesInitialized_)
  {

    hashDict_getEstimatedRows_ = new(heap_) NAHashDictionary<NAString, double>
                                           (&NAString::hash, 101, TRUE, heap_);

    hashDict_Views_ = new(heap_) NAHashDictionary<const QualifiedName, Int64>
                                           (&QualifiedName::hash, 101, TRUE, heap_);
                                           
    hashDict_Tables_ = new(heap_) NAHashDictionary<const QualifiedName, Int64>
                                           (&QualifiedName::hash, 101, TRUE, heap_);
                                           
    hashDict_Synonyms_ = new(heap_) NAHashDictionary<const QualifiedName, Int32>
                                           (&QualifiedName::hash, 101, TRUE, heap_);

    hashDict_HiveTables_ = new(heap_) NAHashDictionary<const QualifiedName, Int64>
                                           (&QualifiedName::hash, 101, TRUE, heap_);
                                            
    hashDictionariesInitialized_ = TRUE;
  }

}

void OptimizerSimulator::createLogFilepath(OsimLog sc)
{
  // Allocate memory for file pathname:
  // dirname + '/' + syscallname + ".txt" + '\0'
  size_t pathLen = osimLogLocalDir_.length()+1+strlen(logFileNames_[sc])+4+1;
  logFilePaths_[sc] = new (heap_) char[pathLen];
  // Construct an absolute pathname for the file.
  strcpy(logFilePaths_[sc], osimLogLocalDir_.data());
  strcat(logFilePaths_[sc], "/");
  strcat(logFilePaths_[sc], logFileNames_[sc]);
}

void OptimizerSimulator::openWriteLogStreams(OsimLog sc)
{
        NABoolean isDir;
        if(isFileExists(logFilePaths_[sc],isDir))
        {
           raiseOsimException("The target log file %s already exists. "
                            "Delete this and other existing log files before "
                            "running the OSIM in CAPTURE mode.", logFilePaths_[sc]);
        }
        // Create the file and write header lines to it.
        writeLogStreams_[sc] = new (heap_) ofstream(logFilePaths_[sc],ios::app);
}

// Initialize the log files if OSIM is running under either CAPTURE
// or SIMULATE mode. If the OSIM is not running under CAPTURE mode,
// add the header lines to the file. Just set the file name variables
// to NULL if OSIM is not running(OFF).
void OptimizerSimulator::initLogFilePaths()
{
  for (OsimLog sc=FIRST_LOG; sc<NUM_OF_LOGS; sc = OsimLog(sc+1))
  {
    switch (osimMode_)
    {
      case OFF:
            // OFF mode indicates no log files needed.
            logFilePaths_[sc] = NULL;
            break;
      case CAPTURE:
            createLogFilepath(sc);
            openWriteLogStreams(sc);
            break;
      case LOAD:
      case UNLOAD:
      case SIMULATE:
            createLogFilepath(sc);
            break;;
    }
  }
}

// BEGIN *********** System Call: MYSYSTEMNUMBER() *************
//
void OptimizerSimulator::capture_MYSYSTEMNUMBER(short sysNum)
{
  if (mySystemNumber_ == -1)
  {
    // Open file in append mode.
    ofstream * outLogfile = writeLogStreams_[MYSYSTEMNUMBER];

    // Write data at the end of the file.
    Int32 origWidth = (*outLogfile).width();
    (*outLogfile) << "  ";
    (*outLogfile).width(10); (*outLogfile) << sysNum << endl;
    (*outLogfile).width(origWidth);
    mySystemNumber_ = sysNum;
  }
}

void OptimizerSimulator::readLogfile_MYSYSTEMNUMBER()
{
  short sysNum;
  NABoolean isDir;

  if(!isFileExists(logFilePaths_[MYSYSTEMNUMBER],isDir))
    raiseOsimException("Unable to open %s file for reading data.", logFilePaths_[MYSYSTEMNUMBER]);

  ifstream inLogfile(logFilePaths_[MYSYSTEMNUMBER]);

  if(inLogfile.good())
  {
    // read sysNum and errSysName from the file
    inLogfile >> sysNum;

    // eofbit is not set until an attempt is made to read beyond EOF.
    // Exit the loop if there was no data to read above.
    if(!inLogfile.good())
    {
      mySystemNumber_ = -1;
    }
    else{
      mySystemNumber_ = sysNum;
    }
  }
  else
      raiseOsimException("Unable to open %s, bad handle.", logFilePaths_[MYSYSTEMNUMBER]);
}

short OptimizerSimulator::simulate_MYSYSTEMNUMBER()
{
  return mySystemNumber_;
}

short OSIM_MYSYSTEMNUMBER()
{
  short sysNum = 0;
  OptimizerSimulator::osimMode mode = OptimizerSimulator::OFF;

  if(CURRCONTEXT_OPTSIMULATOR && 
      !CURRCONTEXT_OPTSIMULATOR->isCallDisabled(7))
    mode = CURRCONTEXT_OPTSIMULATOR->getOsimMode();

  // Check for OSIM mode
  switch (mode)
  {
    case OptimizerSimulator::OFF:
    case OptimizerSimulator::LOAD:
    case OptimizerSimulator::CAPTURE:
      sysNum = MYSYSTEMNUMBER();
      if(mode == OptimizerSimulator::CAPTURE)
          CURRCONTEXT_OPTSIMULATOR->capture_MYSYSTEMNUMBER(sysNum);
      break;
    case OptimizerSimulator::SIMULATE:
      sysNum = CURRCONTEXT_OPTSIMULATOR->simulate_MYSYSTEMNUMBER();
      break;
    default:
      // The OSIM must run under OFF (normal), CAPTURE or SIMULATE mode.
      raiseOsimException("An invalid OSIM mode is encountered - The valid mode is OFF, CAPTURE or SIMULATE");
      break;
  }
  return sysNum;
}
// END ************* System Call: MYSYSTEMNUMBER() *************

void OptimizerSimulator::capture_getEstimatedRows(const char *tableName, double estRows)
{
  NAString *key_tableName = new (heap_) NAString(tableName, heap_);
  double *val_estRows = new double(estRows);

  if (hashDict_getEstimatedRows_->contains(key_tableName))
  {
    double *chkValue = hashDict_getEstimatedRows_->getFirstValue(key_tableName);
    if (*chkValue != estRows)
      // A given key should always have the same value.
      CMPASSERT(FALSE);
  }
  else
  {
    NAString *check = hashDict_getEstimatedRows_->insert(key_tableName,
                                                        val_estRows);
    // Open file in append mode.
    ofstream * outLogfile = writeLogStreams_[ESTIMATED_ROWS];
    Int32 origWidth = (*outLogfile).width();
    // Write data at the end of the file.
    (*outLogfile) << "  ";
    (*outLogfile).width(36); (*outLogfile) << tableName << "  ";
    (*outLogfile).width(36); (*outLogfile) << estRows << endl;
    (*outLogfile).width(origWidth);
  }
}

void OSIM_restoreHHDFSMasterHostList()
{
  if(CURRCONTEXT_OPTSIMULATOR && 
     CURRCONTEXT_OPTSIMULATOR->getOsimMode() == OptimizerSimulator::SIMULATE)
         CURRCONTEXT_OPTSIMULATOR->restoreHHDFSMasterHostList();
}

void OptimizerSimulator::restoreHHDFSMasterHostList()
{
    NABoolean isDir;
    if(!isFileExists(logFilePaths_[HHDFS_MASTER_HOST_LIST],isDir))
      raiseOsimException("Unable to open %s file for reading data.", logFilePaths_[HHDFS_MASTER_HOST_LIST]);
    
    ifstream inLogfile(logFilePaths_[HHDFS_MASTER_HOST_LIST]);
    
    if(inLogfile.good())
    {
        std::string name;
        std::string value;
        //read HHDFSMasterHostList::hasVirtualSQNodes_;
        inLogfile >> name >> value;
        CmpCommon::context()->setHasVirtualSQNodes(std::atoi(value.c_str()));
        //read HHDFSMasterHostList::numSQNodes_;
        inLogfile >> name >> value;
        CmpCommon::context()->setNumSQNodes(std::atoi(value.c_str()));
        inLogfile >> value;
        if(value.length() > 0){
            LIST(NAString) hosts(STMTHEAP);
            NAString line = value.c_str();
            line.split('|', hosts);
            for(Int32 i = 0; i < hosts.entries(); i++)
                HHDFSMasterHostList::getHostNumInternal(hosts[i].data());
        }
    }
    else
        raiseOsimException("Unable to open %s, bad handle.", logFilePaths_[HHDFS_MASTER_HOST_LIST]);
}

void OptimizerSimulator::readLogfile_getEstimatedRows()
{
  char tableName[ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+1];
  double estRows;
  NABoolean isDir;

  if(!isFileExists(logFilePaths_[ESTIMATED_ROWS],isDir))
    raiseOsimException("Unable to open %s file for reading data.", logFilePaths_[ESTIMATED_ROWS]);

  ifstream inLogfile(logFilePaths_[ESTIMATED_ROWS]);

  while(inLogfile.good())
  {
    // read tableName and estRows from the file
    inLogfile >> tableName >> estRows;
    // eofbit is not set until an attempt is made to read beyond EOF.
    // Exit the loop if there was no data to read above.
    if(!inLogfile.good())
      break;
    NAString *key_tableName = new (heap_) NAString(tableName, heap_);
    double *val_estRows = new double(estRows);
    NAString *check = hashDict_getEstimatedRows_->insert(key_tableName,
                                                        val_estRows);
  }
}


double OptimizerSimulator::simulate_getEstimatedRows(const char *tableName)
{
  NAString key_tableName(tableName, heap_);
  if (hashDict_getEstimatedRows_->contains(&key_tableName))
  {
    double *val_estRows = hashDict_getEstimatedRows_->getFirstValue(&key_tableName);
    return *(val_estRows);
  }

  return -1;
}

void OptimizerSimulator::capture_getNodeAndClusterNumbers(short& nodeNum, Int32& clusterNum)
{
    if (capturedNodeAndClusterNum_)
      return;

    nodeNum_ = nodeNum;
    clusterNum_ = clusterNum;

    capturedNodeAndClusterNum_ = TRUE;
}


void OptimizerSimulator::log_getNodeAndClusterNumbers()
{
    // Open file in append mode.
    ofstream * outLogfile = writeLogStreams_[NODE_AND_CLUSTER_NUMBERS];
    Int32 origWidth = (*outLogfile).width();
    // Write data at the end of the file.
    (*outLogfile) << "  ";
    (*outLogfile).width(8); (*outLogfile) << nodeNum_ << "  ";
    (*outLogfile).width(12); (*outLogfile) << clusterNum_ << endl;
    (*outLogfile).width(origWidth);
}


void OptimizerSimulator::readLogFile_getNodeAndClusterNumbers()
{
  short nodeNum;
  Int32 clusterNum;
  NABoolean isDir;

  if(!isFileExists(logFilePaths_[NODE_AND_CLUSTER_NUMBERS],isDir))
    raiseOsimException("Unable to open %s file for reading data.", logFilePaths_[NODE_AND_CLUSTER_NUMBERS]);

  ifstream inLogfile(logFilePaths_[NODE_AND_CLUSTER_NUMBERS]);

  if(inLogfile.good())
  {
    // read nodeNum and clusterNum from the file
    inLogfile >> nodeNum >> clusterNum;
    // eofbit is not set until an attempt is made to read beyond EOF.
    // Exit if there was no data to read above.
    if(!inLogfile.good())
    {
      nodeNum_ = -1;
      clusterNum_ = -1;
    }
    else{
      nodeNum_ = nodeNum;
      clusterNum_ = clusterNum;
    }
  }

}


void OptimizerSimulator::simulate_getNodeAndClusterNumbers(short& nodeNum, Int32& clusterNum)
{
  nodeNum = nodeNum_;
  clusterNum = clusterNum_;
}


void  OSIM_getNodeAndClusterNumbers(short& nodeNum, Int32& clusterNum){

  OptimizerSimulator::osimMode mode = OptimizerSimulator::OFF;

  if(CURRCONTEXT_OPTSIMULATOR && 
     !CURRCONTEXT_OPTSIMULATOR->isCallDisabled(10))
    mode = CURRCONTEXT_OPTSIMULATOR->getOsimMode();

  // Check for OSIM mode
  switch (mode)
  {
    case OptimizerSimulator::OFF:
    case OptimizerSimulator::LOAD:
    case OptimizerSimulator::CAPTURE:
      NADefaults::getNodeAndClusterNumbers(nodeNum, clusterNum);
      if(mode == OptimizerSimulator::CAPTURE)
          CURRCONTEXT_OPTSIMULATOR->capture_getNodeAndClusterNumbers(nodeNum, clusterNum);
      break;
    case OptimizerSimulator::SIMULATE:
      CURRCONTEXT_OPTSIMULATOR->simulate_getNodeAndClusterNumbers(nodeNum, clusterNum);
      break;
    default:
      // The OSIM must run under OFF (normal), CAPTURE or SIMULATE mode.
      raiseOsimException("Invalid OSIM mode - It must be OFF or CAPTURE or SIMULATE.");
      break;
  }

}

void OptimizerSimulator::capture_CQDs()
{
  NAString cqd(STMTHEAP);
  ofstream * cqdDefaultsLogfile =
               writeLogStreams_[CQD_DEFAULTSFILE];

  // send all externalized CQDs.
  NADefaults &defs = CmpCommon::context()->getSchemaDB()->getDefaults();

  for (UInt32 i = 0; i < defs.numDefaultAttributes(); i++)
  {
    const char *attrName = defs.lookupAttrName (i);
    const char *val=defs.getValue(i);
    
    cqd =   "CONTROL QUERY DEFAULT ";
    cqd += attrName;
    cqd += " ";
    cqd += "'";
    cqd += val;
    cqd += "'";
    cqd += ";";
    DefaultConstants attrEnum = NADefaults::lookupAttrName(attrName);
    switch(defs.getProvenance(attrEnum))
    {
        case NADefaults::SET_BY_CQD:
        case NADefaults::DERIVED:
        case NADefaults::READ_FROM_SQL_TABLE:
        case NADefaults::COMPUTED:
        //case NADefaults::UNINITIALIZED:
        //case NADefaults::INIT_DEFAULT_DEFAULTS:
        //case NADefaults::IMMUTABLE:
            (*cqdDefaultsLogfile) << cqd.data() << endl;
            break;
    }
  }
}

void OSIM_captureTableOrView(NATable * naTab)
{
  if(CURRCONTEXT_OPTSIMULATOR && 
     CURRCONTEXT_OPTSIMULATOR->getOsimMode() == OptimizerSimulator::CAPTURE)
    CURRCONTEXT_OPTSIMULATOR->capture_TableOrView(naTab);
}

void OptimizerSimulator::capture_TableOrView(NATable * naTab)
{
  if(naTab->isHiveTable()
     ||naTab->isHistogramTable()
     ||ComIsTrafodionReservedSchemaName(naTab->getTableName().getSchemaName()))
      return;
  const char * viewText = naTab->getViewText();
  const QualifiedName objQualifiedName = naTab->getTableName();
  const NAString nastrQualifiedName = objQualifiedName.getQualifiedNameAsString();
  
  // Handle Synonym first
  if(naTab->getIsSynonymTranslationDone())
  {
    NAString synRefName = naTab->getSynonymReferenceName();

    if(!hashDict_Synonyms_->contains(&objQualifiedName))
    {      
      ofstream * synonymListFile = writeLogStreams_[SYNONYMSFILE];
      (*synonymListFile) << objQualifiedName.getQualifiedNameAsAnsiString().data() <<endl;

      ofstream * synonymLogfile = writeLogStreams_[SYNONYMDDLS];
      
      (*synonymLogfile) << "create catalog " << objQualifiedName.getCatalogName().data()
                  << ";" << endl;
      (*synonymLogfile) << "create schema " << objQualifiedName.getCatalogName().data()
                  << "." << objQualifiedName.getSchemaName().data() << ";"
                  << endl;
      (*synonymLogfile) << "create synonym " 
                         << objQualifiedName.getQualifiedNameAsAnsiString().data() 
                         << " for " << synRefName << ";" << endl;


      QualifiedName * synonymName = new (heap_) QualifiedName(objQualifiedName, heap_);
      Int32 * dummy = new Int32(0);
      hashDict_Synonyms_->insert(synonymName, dummy);
    }

  }

  if (viewText)
  {
    // * if viewText not already written out then write out viewText
    if(!hashDict_Views_->contains(&objQualifiedName))
    {
      // Open file in append mode.
      ofstream * viewsListFile = writeLogStreams_[VIEWSFILE];
      (*viewsListFile) << objQualifiedName.getQualifiedNameAsAnsiString().data() <<endl;
      
      ofstream * viewLogfile = writeLogStreams_[VIEWDDLS];

      (*viewLogfile) << viewText <<endl;

      // insert viewName into hash table
      // this is used to check if the view has already
      // been written out to disk
      QualifiedName * viewName = new (heap_) QualifiedName(objQualifiedName, heap_);
      Int64 * dummy = new Int64(naTab->objectUid().get_value());
      hashDict_Views_->insert(viewName, dummy);
    }
  }
  else if (naTab->getSpecialType() == ExtendedQualName::NORMAL_TABLE)
  {
    // handle base tables
    // if table not already captured then:
    if(!hashDict_Tables_->contains(&objQualifiedName))
    {
        //tables referred by this table should also be write out.
        //recursively call myself until no referred table.
        const AbstractRIConstraintList &refList = naTab->getRefConstraints();
        BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
        for (Int32 i = 0; i < refList.entries(); i++)
        {
              AbstractRIConstraint *ariConstr = refList[i];

              if (ariConstr->getOperatorType() != ITM_REF_CONSTRAINT)
                  continue;
                        
              RefConstraint * refConstr = (RefConstraint*)ariConstr;
              const ComplementaryRIConstraint &uniqueConstraintReferencedByMe
                    = refConstr->getUniqueConstraintReferencedByMe();

              NATable *otherNaTable = NULL;
              CorrName otherCN(uniqueConstraintReferencedByMe.getTableName());
              otherNaTable = bindWA.getNATable(otherCN);
              if (otherNaTable == NULL || bindWA.errStatus())
                   raiseOsimException("Errors Dumping Table DDL.");
              capture_TableOrView(otherNaTable);
        }
        //end capture referred tables
    
      // Open file in append mode.
      ofstream * tablesListFile = writeLogStreams_[TABLESFILE];
      (*tablesListFile) << objQualifiedName.getQualifiedNameAsAnsiString().data() <<endl;

      //insert tableName into hash table for later use
      QualifiedName * tableName = new (heap_) QualifiedName(objQualifiedName, heap_);
      //save table uid to get historgram data on leaving.
      Int64 * tableUID = new Int64(naTab->objectUid().get_value());
      hashDict_Tables_->insert(tableName, tableUID);
      dumpDDLs(objQualifiedName);
    }
  }
}

void OptimizerSimulator::captureQueryText(const char * query)
{
  // Open file in append mode.
  ofstream * outLogfile = writeLogStreams_[QUERIESFILE];

  //(*outLogfile) << "--BeginQuery" << endl;
  (*outLogfile) << query ;
  Int32 queryStrLen = strlen(query);
  // put in a semi-colon at end of query if it is missing
  if (query[queryStrLen]!= ';')
    (*outLogfile) << ";";
  (*outLogfile) << endl;
  //(*outLogfile) << "--EndQuery" << endl;
}

void OptimizerSimulator::captureQueryShape(const char * shape)
{
  // Open file in append mode.
  ofstream * outLogfile = writeLogStreams_[QUERIESFILE];

  (*outLogfile) << "--QueryShape: " << shape << ";" << endl;

}

void OSIM_captureQueryShape(const char * shape)
{
  if(CURRCONTEXT_OPTSIMULATOR)
      CURRCONTEXT_OPTSIMULATOR->captureQueryShape(shape);
}

//every time each query
void OptimizerSimulator::capturePrologue()
{
  if (osimMode_ == OptimizerSimulator::CAPTURE)
  {    
    if (!capturedInitialData_)
    {
      capture_CQDs();
      gpClusterInfo->initializeForOSIMCapture();
      gpClusterInfo->captureNAClusterInfo(*writeLogStreams_[NACLUSTERINFO]);
      //captureVPROC();

      // Write the system type to a file.
      captureSysType();

      //log_REMOTEPROCESSORSTATUS();
      log_getNodeAndClusterNumbers();
      ControlDB * cdb = ActiveControlDB();

      if (cdb->getRequiredShape())
      {
        const char * requiredShape =
          cdb->getRequiredShape()->getShapeText().data();
        captureQueryText(requiredShape);
      }
      capturedInitialData_ = TRUE;
    }

    const char * queryText = CmpCommon::context()->statement()->userSqlText();
    captureQueryText(queryText);
  }
}

void  OSIM_capturePrologue()
{
  if(CURRCONTEXT_OPTSIMULATOR)
      CURRCONTEXT_OPTSIMULATOR->capturePrologue();
}

void OptimizerSimulator::cleanup()
{
  mySystemNumber_ = -1;
  capturedInitialData_ = FALSE;
  //usingCaptureHint_ = FALSE;
  
  osimMode_ = OptimizerSimulator::OFF;

  // delete file names
  for (OsimLog sc=FIRST_LOG; sc<NUM_OF_LOGS; sc = OsimLog(sc+1))
  {
    if (logFilePaths_[sc])
    {
      NADELETEBASIC(logFilePaths_[sc],heap_); 
      logFilePaths_[sc]=NULL;
    }
    
    if(writeLogStreams_[sc])
    {
      writeLogStreams_[sc]->close();
      NADELETE(writeLogStreams_[sc], ofstream, heap_); 
      writeLogStreams_[sc]=NULL;
    }
  }//for

  if(hashDict_getEstimatedRows_)
      hashDict_getEstimatedRows_->clear(TRUE);
  if(hashDict_Views_)
      hashDict_Views_->clear(TRUE);
  if(hashDict_Tables_)
      hashDict_Tables_->clear(TRUE);
  if(hashDict_Synonyms_)
      hashDict_Synonyms_->clear(TRUE);
  if(hashDict_HiveTables_)
      hashDict_HiveTables_->clear(TRUE);
}

void OptimizerSimulator::cleanupSimulator()
{
  cleanup();
  //clear out QueryCache
  CURRENTQCACHE->makeEmpty();
  //clear out NATableCache
  CmpCommon::context()->schemaDB_->getNATableDB()->setCachingOFF();
  CmpCommon::context()->schemaDB_->getNATableDB()->setCachingON();
  //clear out HistogramCache
  if(CURRCONTEXT_HISTCACHE)
    CURRCONTEXT_HISTCACHE->invalidateCache();
}

void OptimizerSimulator::cleanupAfterStatement()
{
  CLIInitialized_ = FALSE;
};

NABoolean OptimizerSimulator::isCallDisabled(ULng32 callBitPosition)
{
  if(callBitPosition > 32)
    return FALSE;

  ULng32 bitMask = SingleBitArray[callBitPosition];

  if(bitMask & sysCallsDisabled_)
    return TRUE;

  return FALSE;
}

void OptimizerSimulator::captureSysType()
{
  const char *sysType = "LINUX";

  ofstream* outLogfile= writeLogStreams_[CAPTURE_SYS_TYPE];
  (*outLogfile) << sysType << endl;
}

OptimizerSimulator::sysType OptimizerSimulator::getCaptureSysType()
{
  return captureSysType_;
}

void OptimizerSimulator::readLogFile_captureSysType()
{
  // This is not an error.  If the file doesn't exist, assume that
  // the captured system type is NSK.
  NABoolean isDir;
  if(!isFileExists(logFilePaths_[CAPTURE_SYS_TYPE],isDir))
  {
    captureSysType_ = OSIM_UNKNOWN_SYSTYPE;
    raiseOsimException("Unable to open %s file for reading data.", logFilePaths_[CAPTURE_SYS_TYPE]);
  }

  ifstream inLogfile(logFilePaths_[CAPTURE_SYS_TYPE]);

  char captureSysTypeString[64];
  inLogfile >> captureSysTypeString;

  if (strncmp(captureSysTypeString,"LINUX",5) == 0)
    captureSysType_ = OSIM_LINUX;
  else 
    CMPASSERT(0); // Something is wrong with the log file.
}

NABoolean OptimizerSimulator::runningSimulation()
{
  return getOsimMode() == OptimizerSimulator::SIMULATE;
}

NABoolean OptimizerSimulator::runningInCaptureMode()
{
  return getOsimMode() == OptimizerSimulator::CAPTURE;
}

NABoolean OptimizerSimulator::runningInLoadMode()
{
  return getOsimMode() == OptimizerSimulator::LOAD;
}

NABoolean OSIM_ClusterInfoInitialized()
{
  return (CURRCONTEXT_OPTSIMULATOR && 
           CURRCONTEXT_OPTSIMULATOR->isClusterInfoInitialized());
}

NABoolean OSIM_runningSimulation()
{
  return (CURRCONTEXT_OPTSIMULATOR &&
           CURRCONTEXT_OPTSIMULATOR->runningSimulation());
}

NABoolean OSIM_runningLoadEmbedded()
{
  const LIST(CmpContext *)& cmpContextsInUse = 
       GetCliGlobals()->currContext()->getCmpContextsInUse();

  // search backwards from the most current CmpContext.
  for (Int32 i=cmpContextsInUse.entries()-1; i>=0; i-- ) {
     CmpContext* cmpContext = cmpContextsInUse[i];
     OptimizerSimulator* simulator = cmpContext->getOptimizerSimulator();
     if ( simulator && simulator->runningInLoadMode())
       return TRUE;
  }
  return FALSE;
}

NABoolean OSIM_runningLoad()
{
  return (CURRCONTEXT_OPTSIMULATOR &&
           CURRCONTEXT_OPTSIMULATOR->runningInLoadMode());
}

NABoolean OSIM_runningInCaptureMode()
{
  return (CURRCONTEXT_OPTSIMULATOR &&
           CURRCONTEXT_OPTSIMULATOR->runningInCaptureMode());
}

NABoolean OSIM_ustatIsDisabled()
{
  return (CURRCONTEXT_OPTSIMULATOR && 
           CURRCONTEXT_OPTSIMULATOR->isCallDisabled(12));
}

void OptimizerSimulator::captureHiveTableStats(HHDFSTableStats* tablestats, const NATable* naTab)
{
     const QualifiedName & qualName = naTab->getTableName();
     if(!hashDict_HiveTables_->contains(&qualName))
     {
         XMLFormattedString * xmltext = new (STMTHEAP) XMLFormattedString(STMTHEAP);
         
         OsimHHDFSStatsBase* root = tablestats->osimSnapShot(STMTHEAP);

          if(!root)
              raiseOsimException("Save capture hive table stasts error");

         root->toXML(*xmltext);
         
         NAString ofile = hiveTableStatsDir_ + '/' +qualName.getQualifiedNameAsAnsiString();
         
         ofstream myfile(ofile.data());
         
         myfile<< *xmltext << endl;
         
         NADELETE(xmltext, XMLFormattedString, STMTHEAP);

         QualifiedName * tableName = new (heap_) QualifiedName(qualName, heap_);

         Int64 * tableUID = new Int64(naTab->objectUid().get_value());

         hashDict_HiveTables_->insert(tableName, tableUID);

         (*writeLogStreams_[HIVE_TABLE_LIST]) << qualName.getQualifiedNameAsAnsiString().data() << ' ' << *tableUID << endl;
     }
}

void OSIM_captureHiveTableStats(HHDFSTableStats* tablestats, const NATable* naTab)
{
  if(CURRCONTEXT_OPTSIMULATOR && 
     CURRCONTEXT_OPTSIMULATOR->getOsimMode() == OptimizerSimulator::CAPTURE)
    CURRCONTEXT_OPTSIMULATOR->captureHiveTableStats(tablestats, naTab);
}

void OptimizerSimulator::dumpHiveTableDDLs()
{
    //const QualifiedName & qualifiedName;
    Int64 * tableUID = NULL;
    const QualifiedName* qualifiedName = NULL;
    NAHashDictionaryIterator<const QualifiedName, Int64> iterator(*hashDict_HiveTables_);
    NAString query(STMTHEAP);
    //turn on HIVE_USE_EXT_TABLE_ATTRS, 
    //if the hive table has corresponding trafodion external table,
    //the external table will also be exported.
    executeFromMetaContext("CQD HIVE_USE_EXT_TABLE_ATTRS 'ON';");
    for(iterator.getNext(qualifiedName, tableUID); qualifiedName && tableUID; iterator.getNext(qualifiedName, tableUID))
    {
        short retcode;
        Queue * outQueue = NULL;
        CMPASSERT(qualifiedName->isHive())
        debugMessage("Dumping DDL for %s\n", qualifiedName->getQualifiedNameAsAnsiString().data());
        query = "SHOWDDL " + qualifiedName->getQualifiedNameAsAnsiString();
        retcode = fetchAllRowsFromMetaContext(outQueue, query.data());
        if (retcode < 0 || retcode == 100/*rows not found*/) {
           cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
           raiseOsimException("Errors Dumping Table DDL.");
        }

        if(outQueue)
        {
            outQueue->position();//rewind
            NABoolean inExtDDL = FALSE;
            for (int i = 0; i < outQueue->numEntries(); i++) {
                OutputInfo * vi = (OutputInfo*)outQueue->getNext();
                char * ptr = vi->get(0);
                //write "CREATE EXTERNAL TABLE" and "REGISTER" DDL to another file.
                if(strstr(ptr, "CREATE EXTERNAL TABLE") ||
                   strstr(ptr, "REGISTER /*INTERNAL*/ HIVE TABLE"))
                    inExtDDL = TRUE;
                if(inExtDDL){
                    (*writeLogStreams_[HIVE_CREATE_EXTERNAL_TABLE]) << ptr << endl;
                    if(strstr(ptr, ";"))
                        inExtDDL = FALSE;
                }
                else
                    (*writeLogStreams_[HIVE_CREATE_TABLE])<< ptr << endl;
            }
        }
    }
}

XMLElementPtr OsimHHDFSStatsMapper::operator()(void *parser,  char *elementName, AttributeList atts)
{
  XMLElementPtr elemPtr = NULL;
  //atts is not used here
  if (!strcmp( elementName, TAG_HHDFSTABLESTATS)){
    OsimHHDFSTableStats * root = new (heap_) OsimHHDFSTableStats(NULL, NULL, heap_);
    HHDFSTableStats * hhstats = new (heap_) HHDFSTableStats(heap_);
    root->restoreHHDFSStats(hhstats , atts);
    elemPtr = root;
  }
  return elemPtr;
}

HHDFSTableStats * OSIM_restoreHiveTableStats(const QualifiedName & qualName, NAMemory* heap, hive_tbl_desc* hvt_desc)
{
  if(CURRCONTEXT_OPTSIMULATOR && 
     CURRCONTEXT_OPTSIMULATOR->getOsimMode() == OptimizerSimulator::SIMULATE)
    return CURRCONTEXT_OPTSIMULATOR->restoreHiveTableStats(qualName, heap, hvt_desc);

  return NULL;
}

HHDFSTableStats * OptimizerSimulator::restoreHiveTableStats(const QualifiedName & qualName, NAMemory* heap, hive_tbl_desc* hvt_desc)
{
     const unsigned int bufSize = 1024;
     OsimHHDFSTableStats* hhTableStats = 0;
     OsimHHDFSStatsMapper om(heap);
     XMLDocument doc(STMTHEAP, om);
     NAString path = hiveTableStatsDir_ + '/' + qualName.getQualifiedNameAsAnsiString();
     std::ifstream s (path, std::ifstream::binary);
     if(!s.good())
         raiseOsimException("Errors open %s. This table is not referred to in the capture mode.", path.data());
     char * txt = new (STMTHEAP) char[bufSize];
     s.read(txt, bufSize);
     while(s.gcount() > 0)
     {
            if(s.gcount() < bufSize)
            {
                hhTableStats = (OsimHHDFSTableStats *)doc.parse(txt, s.gcount(), 1);
                break;
            }
            else
               hhTableStats = (OsimHHDFSTableStats *)doc.parse(txt, s.gcount(), 0);

            s.read(txt, 1024);
    }
    HHDFSTableStats* hiveHDFSTableStats = (HHDFSTableStats *)(hhTableStats->getHHStats());
    hiveHDFSTableStats->setPortOverride(CmpCommon::getDefaultLong(HIVE_LIB_HDFS_PORT_OVERRIDE));
    //may be some date should come from debugging configuration.
    //struct hive_sd_desc *hsd = hvt_desc->getSDs();
    //hiveHDFSTableStats->tableDir_ = hsd->location_;
    //hiveHDFSTableStats->numOfPartCols_ = hvt_desc->getNumOfPartCols();
    //hiveHDFSTableStats->recordTerminator_ = hsd->getRecordTerminator();
    //hiveHDFSTableStats->fieldTerminator_ = hsd->getFieldTerminator() ;
    //hiveHDFSTableStats->currHdfsHost_ = hdfsHost;
    //hiveHDFSTableStats->currHdfsPort_ = hdfsPort;
    struct hive_sd_desc *hsd = hvt_desc->getSDs();

    hiveHDFSTableStats->tableDir_ = hsd->location_;
    hiveHDFSTableStats->validationJTimestamp_ = JULIANTIMESTAMP();

    // for debugging
    NAString logFile = 
      ActiveSchemaDB()->getDefaults().getValue(HIVE_HDFS_STATS_LOG_FILE);

    if (logFile.length())
      {
        FILE *ofd = fopen(logFile, "a");
        if (ofd){
          hiveHDFSTableStats->print(ofd);
          fclose(ofd);
        }
      }

    return hiveHDFSTableStats;
}

NABoolean OsimHHDFSStatsBase::restoreHHDFSStats(HHDFSStatsBase* hhstats, const char ** atts)
{
    if(!hhstats)
        return FALSE;
    else
        setHHStats(hhstats);

    XMLAttributeIterator iter(atts);
    const char *attrName;
    const char *attrValue;

    while (iter.hasNext())
    {
        iter.next();
        attrName = iter.getName();
        attrValue = iter.getValue();
        if(!setValue(hhstats, attrName, attrValue))
            return FALSE;
    }

    return TRUE;
}

void OsimHHDFSStatsBase::serializeBody(XMLString & xml)
{
     xml.incrementLevel();
     for(Int32 i = 0; i < statsList_.entries(); i++)
     {
         statsList_[i]->toXML(xml);
     }
     xml.decrementLevel();
}

void OsimHHDFSStatsBase::serializeAttrs(XMLString & xml)
{
    if(NULL == mirror_) 
        return;
    HHDFSStatsBase* hhstats = (HHDFSStatsBase*)mirror_;
    xml.append("position='").append(std::to_string((long long)(getPosition())).c_str()).append("' ");
    xml.append("numBlocks='").append(std::to_string((long long)(hhstats->numBlocks_)).c_str()).append("' ");
    xml.append("numFiles='").append(std::to_string((long long)(hhstats->numFiles_)).c_str()).append("' ");
    xml.append("totalRows='").append(std::to_string((long long)(hhstats->totalRows_)).c_str()).append("' ");
    xml.append("numStripes='").append(std::to_string((long long)(hhstats->numStripes_)).c_str()).append("' ");
    xml.append("totalStringLengths='").append(std::to_string((long long)(hhstats->totalStringLengths_)).c_str()).append("' ");
    xml.append("totalSize='").append(std::to_string((long long)(hhstats->totalSize_)).c_str()).append("' ");
    xml.append("modificationTS='").append(std::to_string((long long)(hhstats->modificationTS_)).c_str()).append("' ");
    xml.append("sampledBytes='").append(std::to_string((long long)(hhstats->sampledBytes_)).c_str()).append("' ");
    xml.append("sampledRows='").append(std::to_string((long long)(hhstats->sampledRows_)).c_str()).append("' ");
}

void OsimHHDFSFileStats::serializeAttrs(XMLString & xml)
{
    if(NULL == mirror_) 
        return;

     OsimHHDFSStatsBase::serializeAttrs(xml);

     HHDFSFileStats* hhstats = (HHDFSFileStats*)mirror_;
     if(hhstats->fileName_.length() > 0)
          xml.append("fileName='").append(hhstats->fileName_).append("' ");

     xml.append("replication='").append(std::to_string((long long)(hhstats->replication_)).c_str()).append("' ");

     xml.append("blockSize='").append(std::to_string((long long)(hhstats->blockSize_)).c_str()).append("' ");
     
     if(hhstats->replication_*hhstats->numBlocks_ > 0 && hhstats->blockHosts_)
     {
         xml.append("blockHosts='");
         for(Int64 i = 0; i < hhstats->replication_*hhstats->numBlocks_; i++)
         {
             xml.append(std::to_string((long long)(hhstats->blockHosts_)[i]).c_str()).append('|');
         }
         xml[xml.length()-1]='\'';
         xml.append(' ');
     }
     else
         xml.append("blockHosts='no_entry' ");

     xml.append("\n");

}

void OsimHHDFSBucketStats::serializeAttrs(XMLString & xml)
{
    if(NULL == mirror_) 
        return;

     OsimHHDFSStatsBase::serializeAttrs(xml);

     HHDFSBucketStats* hhstats = (HHDFSBucketStats*)mirror_;

     xml.append("scount='").append(std::to_string((long long)(hhstats->scount_)).c_str()).append("' ");

}

void OsimHHDFSListPartitionStats::serializeAttrs(XMLString & xml)
{
    if(NULL == mirror_) 
        return;

     OsimHHDFSStatsBase::serializeAttrs(xml);

     HHDFSListPartitionStats* hhstats = (HHDFSListPartitionStats*)mirror_;

     if(hhstats->partitionDir_.length() > 0)
         xml.append("partitionDir='").append(hhstats->partitionDir_).append("' ");
     if(hhstats->partitionKeyValues_.length() > 0)
        xml.append("partitionKeyValues='").append(hhstats->partitionKeyValues_).append("' ");

     xml.append("partIndex='").append(std::to_string((long long)(hhstats->partIndex_)).c_str()).append("' ");
   
     xml.append("defaultBucketIdx='").append(std::to_string((long long)(hhstats->defaultBucketIdx_)).c_str()).append("' ");

     xml.append("recordTerminator='").append(std::to_string((long long)(hhstats->recordTerminator_)).c_str()).append("' ");

}

void OsimHHDFSTableStats::serializeAttrs(XMLString & xml)
{
    if(NULL == mirror_) 
        return;

    OsimHHDFSStatsBase::serializeAttrs(xml);

    HHDFSTableStats* hhstats = (HHDFSTableStats*)mirror_;

     if(hhstats->tableDir_.length() > 0)
         xml.append("tableDir='").append(hhstats->tableDir_).append("' ");

     xml.append("numOfPartCols='").append(std::to_string((long long)(hhstats->numOfPartCols_)).c_str()).append("' ");

     xml.append("totalNumPartitions='").append(std::to_string((long long)(hhstats->totalNumPartitions_)).c_str()).append("' ");
   
     xml.append("recordTerminator='").append(std::to_string((long long)(hhstats->recordTerminator_)).c_str()).append("' ");

     xml.append("fieldTerminator='").append(std::to_string((long long)(hhstats->fieldTerminator_)).c_str()).append("' ");

     xml.append("validationJTimestamp='").append(std::to_string((long long)(hhstats->validationJTimestamp_)).c_str()).append("' ");

     xml.append("hiveStatsSize='").append(std::to_string((long long)(hhstats->hiveStatsSize_)).c_str()).append("' ");

     xml.append("type='").append(std::to_string((long long)(hhstats->type_)).c_str()).append("' ");
}

NABoolean OsimHHDFSStatsBase::setValue(HHDFSStatsBase* hhstats, const char *attrName, const char *attrValue)
{      
      if(NULL == hhstats)
          return FALSE;

      if (!strcmp(attrName, "position"))
        setPosition(std::atol(attrValue));
      else if (!strcmp(attrName, "numBlocks"))
        hhstats->numBlocks_= std::atol(attrValue);
      else if (!strcmp(attrName, "numFiles"))
        hhstats->numFiles_ = std::atol(attrValue);
      else if (!strcmp(attrName, "totalRows"))
        hhstats->totalRows_= std::atol(attrValue);
      else if (!strcmp(attrName, "numStripes"))
        hhstats->numStripes_= std::atol(attrValue);
      else if (!strcmp(attrName, "totalStringLengths"))
        hhstats->totalStringLengths_= std::atol(attrValue);
      else if (!strcmp(attrName, "totalSize"))
        hhstats->totalSize_= std::atol(attrValue);
      else if (!strcmp(attrName, "modificationTS"))
        hhstats->modificationTS_ = (time_t)std::atol(attrValue);
      else if (!strcmp(attrName, "sampledBytes"))
        hhstats->sampledBytes_ = std::atol(attrValue);
      else if (!strcmp(attrName, "sampledRows"))
        hhstats->sampledRows_ = std::atol(attrValue);
      else
        return FALSE;

      return TRUE;
}

NABoolean OsimHHDFSFileStats::setValue(HHDFSStatsBase* stats, const char *attrName, const char *attrValue)
{
    if(NULL == stats)
        return FALSE;
    
    if(OsimHHDFSStatsBase::setValue(stats, attrName, attrValue))
       return TRUE;
    
    HHDFSFileStats* hhstats = dynamic_cast<HHDFSFileStats*>(stats);
    assert(hhstats);
    
    if (!strcmp(attrName, "fileName"))
          hhstats->fileName_= attrValue;
    else if (!strcmp(attrName, "replication"))
          hhstats->replication_= std::atoi(attrValue);
    else if (!strcmp(attrName, "blockSize"))
          hhstats->blockSize_= std::atoi(attrValue);
    else if (!strcmp(attrName, "blockHosts"))
    {
          if(strcmp(attrValue, "no_entry"))
          {
              NAString values = attrValue;
              LIST(NAString) valueList(STMTHEAP);
              values.split('|', valueList);
              hhstats->blockHosts_ = new (hhstats->heap_) HostId[valueList.entries()];
              for(Int32 i = 0; i < valueList.entries(); i++)
              {   
                  if(valueList[i].length() > 0)
                      hhstats->blockHosts_[i] = std::atoi(valueList[i].data());
              }
          }
    }
    else
      return FALSE;
    
    return TRUE;
}


NABoolean OsimHHDFSBucketStats::setValue(HHDFSStatsBase* stats, const char *attrName, const char *attrValue)
{
    if(NULL == stats)
        return FALSE;
    
    if(OsimHHDFSStatsBase::setValue(stats, attrName, attrValue))
       return TRUE;
    
    HHDFSBucketStats* hhstats = dynamic_cast<HHDFSBucketStats*>(stats);
    assert(hhstats);
    
    if (!strcmp(attrName, "scount"))
      hhstats->scount_= std::atoi(attrValue);
    else
      return FALSE;
    
    return TRUE;
}

NABoolean OsimHHDFSListPartitionStats::setValue(HHDFSStatsBase* stats, const char *attrName, const char *attrValue)
{      
      if(NULL == stats)
          return FALSE;

      if(OsimHHDFSStatsBase::setValue(stats, attrName, attrValue))
         return TRUE;

      HHDFSListPartitionStats* hhstats = dynamic_cast<HHDFSListPartitionStats*>(stats);
      assert(hhstats);

      if (!strcmp(attrName, "partitionDir"))
        hhstats->partitionDir_= attrValue;
      else if (!strcmp(attrName, "partitionKeyValues"))
        hhstats->partitionKeyValues_ = attrValue;
      else if (!strcmp(attrName, "partIndex"))
        hhstats->partIndex_= std::atoi(attrValue);
      else if (!strcmp(attrName, "defaultBucketIdx"))
        hhstats->defaultBucketIdx_= std::atoi(attrValue);
      else if (!strcmp(attrName, "recordTerminator"))
        hhstats->recordTerminator_ = std::atoi(attrValue);
      else
        return FALSE;

      return TRUE;
}

NABoolean OsimHHDFSTableStats::setValue(HHDFSStatsBase* stats, const char *attrName, const char *attrValue)
{
      if(NULL == stats)
          return FALSE;
      
      if(OsimHHDFSStatsBase::setValue(stats, attrName, attrValue))
         return TRUE;

      HHDFSTableStats * hhstats = dynamic_cast<HHDFSTableStats*>(stats);
      assert(hhstats);
      if (!strcmp(attrName, "tableDir"))
        hhstats->tableDir_ = attrValue;
      else if (!strcmp(attrName, "numOfPartCols"))
        hhstats->numOfPartCols_ = std::atoi(attrValue);
      else if (!strcmp(attrName, "totalNumPartitions"))
        hhstats->totalNumPartitions_= std::atoi(attrValue);
      else if (!strcmp(attrName, "recordTerminator"))
        hhstats->recordTerminator_ = std::atoi(attrValue);
      else if (!strcmp(attrName, "fieldTerminator"))
        hhstats->fieldTerminator_ = std::atoi(attrValue);
      else if (!strcmp(attrName, "validationJTimestamp"))
        hhstats->validationJTimestamp_ = std::atol(attrValue);
      else if (!strcmp(attrName, "hiveStatsSize"))
        hhstats->hiveStatsSize_ = std::atol(attrValue);
      else if (!strcmp(attrName, "type"))
        hhstats->type_= (HHDFSTableStats::FileType)std::atoi(attrValue);
      else
        return FALSE;

      return TRUE;
}

void OsimHHDFSTableStats::startElement(void *parser, const char *elementName, const char **atts)
{
    if(!strcmp(elementName, TAG_HHDFSLISTPARTSTATS)){
        OsimHHDFSListPartitionStats* entry = new (heap_) OsimHHDFSListPartitionStats(this, NULL, heap_);
        HHDFSListPartitionStats* hhstats = new (heap_) HHDFSListPartitionStats(heap_, mirror_->getTable());
        entry->restoreHHDFSStats(hhstats, atts);
        Int32 pos = entry->getPosition();
        addEntry(entry, pos);
        //put hhstats into the same position as capture time,
        //positions are ascendant, because they are capture this way,
        //"gaps" are filled with "un-used" entries
        ((HHDFSTableStats*)mirror_)->insertAt(pos, hhstats);

        XMLDocument::setCurrentElement(parser, entry);
    }
    else
       raiseOsimException("Errors Parsing stats file.");
}

void OsimHHDFSListPartitionStats::startElement(void *parser, const char *elementName, const char **atts)
{
    if(!strcmp(elementName, TAG_HHDFSBUCKETSTATS)){
        OsimHHDFSBucketStats* entry = new (heap_) OsimHHDFSBucketStats(this, NULL, heap_);
        HHDFSBucketStats* hhstats = new (heap_) HHDFSBucketStats(heap_, mirror_->getTable());
        entry->restoreHHDFSStats(hhstats, atts);
        Int32 pos = entry->getPosition();
        addEntry(entry, pos);
        ((HHDFSListPartitionStats*)mirror_)->insertAt(pos, hhstats);

        XMLDocument::setCurrentElement(parser, entry);
    }
    else
       raiseOsimException("Errors Parsing stats file.");
}

void OsimHHDFSBucketStats::startElement(void *parser, const char *elementName, const char **atts)
{
    if(!strcmp(elementName, TAG_HHDFSFILESTATS)){
        OsimHHDFSFileStats* entry = new (heap_) OsimHHDFSFileStats(this, NULL, heap_);
        HHDFSFileStats* hhstats = new (heap_) HHDFSFileStats(heap_, mirror_->getTable());
        entry->restoreHHDFSStats(hhstats, atts);
        Int32 pos = entry->getPosition();
        addEntry(entry, pos);
        ((HHDFSBucketStats*)mirror_)->insertAt(pos, hhstats);

        XMLDocument::setCurrentElement(parser, entry);
    }
    else
       raiseOsimException("Errors Parsing stats file.");
}

void OsimHHDFSStatsBase::endElement(void *parser, const char *elementName)
{
    XMLDocument::setCurrentElement(parser, getParent());
}

