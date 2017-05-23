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
#include "hdfs.h"
#include "CmpSeabaseDDL.h"
#include "ExExeUtilCli.h"
#include "ComUser.h"

extern THREAD_P NAClusterInfo *gpClusterInfo;

extern const WordAsBits SingleBitArray[];

//the dir path should start from /bulkload
#define UNLOAD_HDFS_DIR "/user/trafodion/bulkload/osim_capture"

static ULng32 hashFunc_int(const Int32& Int)
{
  return (ULng32)Int;
}

static NABoolean fileExists(const char *filename, NABoolean & isDir)
{
  struct stat sb;
  Int32 rVal = stat(filename, &sb);
  isDir = FALSE;
  if(S_ISDIR(sb.st_mode))
      isDir = TRUE;
  return rVal != -1;
}


const char OsimAllHistograms::elemName[] = TAG_ALL_HISTOGRAMS;
const char OsimHistogramEntry::elemName[] = TAG_HISTOGRAM_ENTRY;

void OsimAllHistograms::startElement(void * parser, const char * elementName, const char * * atts)
{
    OsimHistogramEntry* entry = NULL;
    if(!strcmp(elementName, TAG_HISTOGRAM_ENTRY)){
        entry = new (XMLPARSEHEAP) OsimHistogramEntry(this, XMLPARSEHEAP);
        XMLDocument::setCurrentElement(parser, entry);
        list_.insert(entry);
    }
    else
       OsimLogException("Errors Parsing hitograms file.", __FILE__, __LINE__).throwException();
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


const char* OptimizerSimulator::sysCallLogFileName_[NUM_OF_SYSCALLS]= {
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
  "captureSysType.txt",
  "HISTOGRAM_PATHS.xml"
};

OptimizerSimulator::OptimizerSimulator(CollHeap *heap)
:osimLogLocalDir_(heap),
 osimMode_(OptimizerSimulator::OFF),
 hashDict_getEstimatedRows_(NULL),
 hashDict_Views_(NULL),
 hashDict_Tables_(NULL),
 hashDict_Synonyms_(NULL),
 hashDict_TablesBeforeAction_(NULL),
 hashDict_ViewsBeforeAction_(NULL),
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
  for (sysCall sc=FIRST_SYSCALL; sc<NUM_OF_SYSCALLS; sc = sysCall(sc+1))
  {
    sysCallLogFilePath_[sc]=NULL;
    writeSysCallStream_[sc]=NULL;
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
  // WARNING message
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
    cmd += sysCallLogFilePath_[VERSIONSFILE];
    system(cmd.data()); //dump versions
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
                      //record all qualified table names before running query,
                      //except meta tables and histogram tables in any schema
                      saveTablesBeforeStart();
                      saveViewsBeforeStart();
                      setClusterInfoInitialized(TRUE);
                      break;
                  case LOAD: //OFF --> LOAD
                      setOsimMode(targetMode);
                      setOsimLogdir(localDir);
                      initHashDictionaries();
                      initLogFilePaths();
                      saveTablesBeforeStart();
                      saveViewsBeforeStart();
                      loadDDLs();
                      loadHistograms();
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
                dumpHistograms();
                dumpVersions();
                setOsimMode(targetMode);
                cleanup();//NOTE: osimMode_ is set OFF in cleanup()
            }
            else
                errorMessage("Mode transition is not allowed.");
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
                errorMessage("Mode transition rather than LOAD to SIMULATE is not allowed.");
            break;
        default :
            errorMessage("Mode transition is not allowed.");
            break;
      }
  }
  catch(OsimLogException & e)
  {
      cleanup();
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
           CmpCommon::diags()->mergeAfter(*(cliInterface_->getDiagsArea()));
           OsimLogException("Errors Dumping Table DDL.", __FILE__, __LINE__).throwException();
    }
    outQueue->position();
    
    ofstream * createSchema = writeSysCallStream_[CREATE_SCHEMA_DDLS];

    ofstream * createTable = writeSysCallStream_[CREATE_TABLE_DDLS];
        
    //Dump a "create schema ..." to schema ddl file for every table.
    
    //This comment line will be printed during loading, ';' must be omitted
    (*createSchema) << "--" <<"CREATE SCHEMA IF NOT EXISTS " 
                                            << qualifiedName.getCatalogName() 
                                            << "." << qualifiedName.getSchemaName() <<endl;

    (*createSchema) << "CREATE SCHEMA IF NOT EXISTS " 
                               << qualifiedName.getCatalogName() 
                               <<"."<< qualifiedName.getSchemaName() 
                               << ";" << endl;

    for (int i = 0; i < outQueue->numEntries(); i++) {
        OutputInfo * vi = (OutputInfo*)outQueue->getNext();
        char * ptr = vi->get(0);
        //skip heading newline, and add a comment line
        Int32 ix = 0;
        for(; ptr[ix]=='\n'; ix++);
        if( strstr(ptr, "CREATE TABLE") ||
            strstr(ptr, "CREATE INDEX") ||
            strstr(ptr, "CREATE UNIQUE INDEX") ||
            strstr(ptr, "ALTER TABLE")  )
            (*createTable) << "--" << ptr+ix << endl;
        //output ddl    
        (*createTable) << ptr << endl;
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
        
        debugMessage("Dumping histograms for %s\n", name->getQualifiedNameAsAnsiString().data());
        
        //dump histograms data to hdfs
        query =   "UNLOAD WITH NULL_STRING '\\N' INTO ";
        query +=  "'"UNLOAD_HDFS_DIR"/";
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
           CmpCommon::diags()->mergeAfter(*(cliInterface_->getDiagsArea()));
           NAString errMsg;
           errMsg = "Unload histogram data error: ";
           errMsg += std::to_string((long long)(retcode)).c_str();
           OsimLogException(errMsg.data(),  __FILE__, __LINE__).throwException();
        }
        
        query =  "UNLOAD WITH NULL_STRING '\\N' INTO ";
        query += "'"UNLOAD_HDFS_DIR"/";
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
           CmpCommon::diags()->mergeAfter(*(cliInterface_->getDiagsArea()));
           NAString errMsg;
           errMsg = "Unload histogram data error: ";
           errMsg += std::to_string((long long)(retcode)).c_str();
           OsimLogException(errMsg.data(),  __FILE__, __LINE__).throwException();
        }
    
    }

    //Do not user XMLFormatString as we do format ourself
    XMLString* xmltext = new (STMTHEAP) XMLString(STMTHEAP);
    histoInfoList->toXML(*xmltext);
    (*writeSysCallStream_[HISTOGRAM_PATHS]) << xmltext->data() << endl;
    NADELETE(xmltext, XMLString, STMTHEAP);
    //copy histograms data from hdfs to osim directory.
    histogramHDFSToLocal();
}

void OptimizerSimulator::dropObjects()
{
   short retcode;
   ifstream tables(sysCallLogFilePath_[TABLESFILE]);
   if(!tables.good())
   {
       NAString errMsg = "Error open ";
       errMsg += sysCallLogFilePath_[CREATE_TABLE_DDLS];
       OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
   }
   //ignore first 2 lines
   tables.ignore(OSIM_LINEMAX, '\n');
   tables.ignore(OSIM_LINEMAX, '\n');
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
      debugMessage("DELETING %s ...\n", stdQualTblNm.c_str());
      retcode = executeFromMetaContext(query.data());
      if(retcode < 0)
      {
          CmpCommon::diags()->mergeAfter(*(cliInterface_->getDiagsArea()));
          NAString errMsg = "Drop Table " ;
          errMsg += stdQualTblNm.c_str();
          errMsg += " Error: ";
          errMsg += std::to_string((long long)(retcode)).c_str();
          OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
      }
   }
}

void OptimizerSimulator::checkDuplicateNames()
{
   //Get captured qualied table names
   //and compare each with the names in hash dictionary.
   ifstream tables(sysCallLogFilePath_[TABLESFILE]);
   if(!tables.good())
   {
       NAString errMsg = "Error open ";
       errMsg += sysCallLogFilePath_[CREATE_TABLE_DDLS];
       OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
   }
   //ignore first 2 lines
   tables.ignore(OSIM_LINEMAX, '\n');
   tables.ignore(OSIM_LINEMAX, '\n');
   std::string stdQualTblNm;
   NAString naQualTblNm(STMTHEAP);
   while(tables.good())
   {
      //get one qualified table name from file
      std::getline(tables, stdQualTblNm);

      // Exit the loop if there was no data to read.
      // eofbit is not set until an attempt is made to read beyond EOF.
      if(!tables.good())
         break;
         
      //Check if table name is in existance
      naQualTblNm = stdQualTblNm.c_str();
      if(hashDict_TablesBeforeAction_->contains(&naQualTblNm))
      {
          NAString errMsg = "Object " + naQualTblNm + " already exists.";
          OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
      }
   }
}

void OptimizerSimulator::saveViewsBeforeStart()
{
   if(viewsBeforeActionInitilized_)
       return;
   //Ask _MD_.OBJECTS for a list of all qualified view names.
   initializeCLI();
   
   short retcode;
   if (cmpSBD_->switchCompiler(CmpContextInfo::CMPCONTEXT_TYPE_META)) {
       OsimLogException("Errors Switch Context.", __FILE__, __LINE__).throwException();
   }
   
   char * ptr = NULL;
   Lng32 len = 0;
                                                           
   retcode = cliInterface_->fetchRowsPrologue("select catalog_name, schema_name, object_name "
                              " from TRAFODION.\""SEABASE_MD_SCHEMA"\"."SEABASE_OBJECTS
                              " where object_type = 'VI' "
                                           "and schema_name <> '_MD_' " 
                                           "and schema_name <> '_REPOS_' "
                                           "and schema_name <> '_PRIVMGR_MD_' "
                                           "and object_name <> 'SB_HISTOGRAMS' "
                                           "and object_name <> 'SB_HISTOGRAM_INTERVALS'; ");
   if (retcode < 0)
   {
      cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
      NAString errMsg = "Get existing views, error ";
      errMsg += std::to_string((long long)(retcode)).c_str();
      OsimLogException(errMsg.data(),  __FILE__, __LINE__).throwException();
   }
   
   while(1)
   {
       retcode = cliInterface_->fetch();
       
       if (retcode < 0)
       {
          cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
          NAString errMsg = "Get existing views, error ";
          errMsg += std::to_string((long long)(retcode)).c_str();
          OsimLogException(errMsg.data(),  __FILE__, __LINE__).throwException();
       }
       if (retcode == 100) //no more data
           break;
           
       NAString * qualifiedName = new (heap_) NAString(heap_);
       Int32 * dummy = new Int32(0);
       //append catalog name
       cliInterface_->getPtrAndLen(1,ptr,len);
       qualifiedName->append(ptr, len);
       qualifiedName->append('.');
       
       //append schema name
       cliInterface_->getPtrAndLen(2,ptr,len);
       qualifiedName->append(ptr, len);
       qualifiedName->append('.');
       
       //append table name
       cliInterface_->getPtrAndLen(3,ptr,len);
       qualifiedName->append(ptr, len);

       hashDict_ViewsBeforeAction_->insert(qualifiedName, dummy);

   }

   //end fetch
   retcode = cliInterface_->fetchRowsEpilogue(NULL);
   
   cmpSBD_->switchBackCompiler();

   viewsBeforeActionInitilized_ = TRUE;
}

void OptimizerSimulator::saveTablesBeforeStart()
{
   if(tablesBeforeActionInitilized_)
       return;
   //Ask _MD_.OBJECTS for a list of all qualified table names.
   initializeCLI();
   
   short retcode;
   if (cmpSBD_->switchCompiler(CmpContextInfo::CMPCONTEXT_TYPE_META)) {
       OsimLogException("Errors Switch Context.", __FILE__, __LINE__).throwException();
   }
   
   char * ptr = NULL;
   Lng32 len = 0;
                                                           
   retcode = cliInterface_->fetchRowsPrologue("select catalog_name, schema_name, object_name "
                              " from TRAFODION.\""SEABASE_MD_SCHEMA"\"."SEABASE_OBJECTS
                              " where object_type = 'BT' "
                                           "and schema_name <> '_MD_' " 
                                           "and schema_name <> '_REPOS_' "
                                           "and schema_name <> '_PRIVMGR_MD_' "
                                           "and object_name <> 'SB_HISTOGRAMS' "
                                           "and object_name <> 'SB_HISTOGRAM_INTERVALS'; ");
   if (retcode < 0)
   {
      cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
      NAString errMsg = "Get existing tables, error ";
      errMsg += std::to_string((long long)(retcode)).c_str();
      OsimLogException(errMsg.data(),  __FILE__, __LINE__).throwException();
   }
   
   while(1)
   {
       retcode = cliInterface_->fetch();
       
       if (retcode < 0)
       {
          cliInterface_->retrieveSQLDiagnostics(CmpCommon::diags());
          NAString errMsg = "Get existing tables, error ";
          errMsg += std::to_string((long long)(retcode)).c_str();
          OsimLogException(errMsg.data(),  __FILE__, __LINE__).throwException();
       }
       if (retcode == 100) //no more data
           break;
           
       NAString * qualifiedName = new (heap_) NAString(heap_);
       Int32 * dummy = new Int32(0);
       //append catalog name
       cliInterface_->getPtrAndLen(1,ptr,len);
       qualifiedName->append(ptr, len);
       qualifiedName->append('.');
       
       //append schema name
       cliInterface_->getPtrAndLen(2,ptr,len);
       qualifiedName->append(ptr, len);
       qualifiedName->append('.');
       
       //append table name
       cliInterface_->getPtrAndLen(3,ptr,len);
       qualifiedName->append(ptr, len);

       hashDict_TablesBeforeAction_->insert(qualifiedName, dummy);

   }

   //end fetch
   retcode = cliInterface_->fetchRowsEpilogue(NULL);
   
   cmpSBD_->switchBackCompiler();

   tablesBeforeActionInitilized_ = TRUE;
}

void OptimizerSimulator::loadDDLs()
{
    debugMessage("loading tables and views ...\n");
    short retcode;

    //If force option is present, 
    //drop tables with same names, otherwise rollback
    if(isForceLoad())
        dropObjects();
    else
        checkDuplicateNames();
    
    NAString statement(STMTHEAP);
    NAString comment(STMTHEAP);
    statement.capacity(4096);
    comment.capacity(4096);

    //Step 1 :
    //Fetch and execute "create schema ..." from schema ddl file.
    debugMessage("Step 1 Create Schemas:\n");
    ifstream createSchemas(sysCallLogFilePath_[CREATE_SCHEMA_DDLS]);
    if(!createSchemas.good())
    {
        NAString errMsg = "Error open ";
        errMsg += sysCallLogFilePath_[CREATE_SCHEMA_DDLS];
        OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
    }    
    //ignore first 2 lines
    createSchemas.ignore(OSIM_LINEMAX, '\n');
    createSchemas.ignore(OSIM_LINEMAX, '\n');
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
    ifstream createTables(sysCallLogFilePath_[CREATE_TABLE_DDLS]);
    if(!createTables.good())
    {
        NAString errMsg = "Error open ";
        errMsg += sysCallLogFilePath_[CREATE_TABLE_DDLS];
        OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
    }    
    //ignore first 2 lines
    createTables.ignore(OSIM_LINEMAX, '\n');
    createTables.ignore(OSIM_LINEMAX, '\n');
    while(readStmt(createTables, statement, comment))
    {
        if(comment.length() > 0)
            debugMessage("%s\n", comment.data());
        if(statement.length() > 0){
            retcode = executeFromMetaContext(statement.data());
            if(retcode < 0)
            {
                CmpCommon::diags()->mergeAfter(*(cliInterface_->getDiagsArea()));
                NAString errMsg = "Create Table Error: " ;
                errMsg += std::to_string((long long)(retcode)).c_str();
                OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
            }
        }
    }

    //Step 3:
    //Fetch and execute "create view ..." from view ddl file.
    debugMessage("Step 3 Create Views:");
    ifstream createViews(sysCallLogFilePath_[VIEWDDLS]);
    if(!createViews.good())
    {
        NAString errMsg = "Error open ";
        errMsg += sysCallLogFilePath_[VIEWDDLS];
        OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
    }  
    //ignore first 2 lines
    createViews.ignore(OSIM_LINEMAX, '\n');
    createViews.ignore(OSIM_LINEMAX, '\n');
    while(readStmt(createViews, statement, comment))
    {
        if(comment.length() > 0)
            debugMessage("%s\n", comment.data());
        if(statement.length() > 0){
            retcode = executeFromMetaContext(statement.data());
            if(retcode < 0)
            {
                CmpCommon::diags()->mergeAfter(*(cliInterface_->getDiagsArea()));
                NAString errMsg = "Create View Error: " ;
                errMsg += std::to_string((long long)(retcode)).c_str();
                errMsg += statement;
                OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
            }
        }
    }
}

NABoolean OptimizerSimulator::massageTableUID(OsimHistogramEntry* entry, NAHashDictionary<NAString, QualifiedName> * modifiedPathList)
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

  QualifiedName* qualifiedName = new (STMTHEAP) QualifiedName(histogramTableName, schema, catalog, STMTHEAP);
  
  Int64 tableUID = getTableUID(catalog, schema, table);
  if(tableUID < 0)
  {
        NAString errMsg = "Get Table UID Error: " ;
        errMsg += std::to_string((long long)(tableUID)).c_str();
        OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
  }
  NAString dataPath(STMTHEAP);
  //get text file path within the dir
  DIR * histogramDir = opendir(fullPath);
  if(!histogramDir)
  {
        NAString errMsg = "Error open ";
        errMsg += fullPath;
        OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
  } 
  struct dirent * dataPathInfo = readdir(histogramDir);
  while(dataPathInfo != NULL)
  {
      if(dataPathInfo->d_name[0] != '.')
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
        NAString errMsg = "Error open ";
        errMsg += dataPath;
        OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
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
        OsimLogException("Invalid format of histogram data.", __FILE__, __LINE__).throwException();
  }
#if 0
  enum workState { WRITEUID, READUID, RESTOFLINE };
  workState state = READUID;
  char a = ' ';
  char uidstr[256];
  snprintf(uidstr, 256, "%ld", tableUID);
  while(1)
  {
    switch(state)
    {
      case READUID :
        infile.get (a);
        if(infile.eof())
          return TRUE;
        else if('|' == a)
          state = WRITEUID;
        break;

      case WRITEUID :
        outfile.write(uidstr, strlen(uidstr));
        outfile.put(a);
        state = RESTOFLINE;
        break;
  
      case RESTOFLINE :
        infile.get (a);
        if(infile.eof())
          return TRUE;
        else if('\n' == a){
          outfile.put(a);
          state = READUID;
        }
        else if('\0' == a)
           continue;
        else 
          outfile.put(a);
        break;      
    }//switch
  }//while
#endif
  return TRUE;
}

void OptimizerSimulator::execHiveSQL(const char* hiveSQL)
{
  HiveClient_JNI *hiveClient = CmpCommon::context()->getHiveClient();

  if (hiveClient == NULL)
    {
      NAString errMsg;
      errMsg = "Error initialize hive client.";
      OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
    }
  else
    {
      if (!CmpCommon::context()->execHiveSQL(hiveSQL))
        {
          NAString errMsg;
          errMsg = "Error running hive SQL.";
          OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
        }
    }
}

short OptimizerSimulator::loadHistogramsTable(NAString* modifiedPath, QualifiedName * qualifiedName, unsigned int bufLen)
{
    debugMessage("loading %s\n", qualifiedName->getQualifiedNameAsString().data());
    short retcode;

    NAString cmd(STMTHEAP);
    
    cmd = "drop table "+qualifiedName->getCatalogName()+"_"+qualifiedName->getSchemaName()+"_"+qualifiedName->getObjectName();
    execHiveSQL(cmd.data());
    
    cmd =      "create table " + qualifiedName->getCatalogName()+"_"+qualifiedName->getSchemaName()+"_"+qualifiedName->getObjectName();
    cmd +=      " (    table_uid              bigint"
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
    
    cmd =       "load data local inpath '" + *modifiedPath + "' into table ";
    cmd +=      qualifiedName->getCatalogName()+"_"+qualifiedName->getSchemaName()+"_"+qualifiedName->getObjectName();
    execHiveSQL(cmd.data());
    
    //create sb_histograms
    cmd =      "CREATE TABLE IF NOT EXISTS " + qualifiedName->getQualifiedNameAsString();
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
                          "  , constraint "HBASE_HIST_PK" primary key"
                          "  (TABLE_UID ASC, HISTOGRAM_ID ASC, COL_POSITION ASC)"
                          " )";
    retcode = executeFromMetaContext(cmd.data());
    if(retcode < 0)
    {
        CmpCommon::diags()->mergeAfter(*(cliInterface_->getDiagsArea()));
        NAString errMsg = "Load histogram data error:  " ;
        errMsg += std::to_string((long long)(retcode)).c_str();
        OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
    }
    
    cmd = "upsert using load into " + qualifiedName->getQualifiedNameAsString() + " select * from hive.hive.";
    cmd += qualifiedName->getCatalogName()+"_"+qualifiedName->getSchemaName()+"_"+qualifiedName->getObjectName(); 
    retcode = executeFromMetaContext(cmd.data());
    
    if(retcode < 0)
    {
        CmpCommon::diags()->mergeAfter(*(cliInterface_->getDiagsArea()));
        NAString errMsg = "Load histogram data error:  " ;
        errMsg += std::to_string((long long)(retcode)).c_str();
        OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
    }
    
    return retcode;
}

short OptimizerSimulator::loadHistogramIntervalsTable(NAString* modifiedPath, QualifiedName * qualifiedName, unsigned int bufLen)
{
    debugMessage("loading %s\n", qualifiedName->getQualifiedNameAsString().data());
    short retcode;

    NAString cmd(STMTHEAP);
    
    cmd = "drop table "+qualifiedName->getCatalogName()+"_"+qualifiedName->getSchemaName()+"_"+qualifiedName->getObjectName();
    execHiveSQL(cmd.data());
    
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
    cmd =      "CREATE TABLE IF NOT EXISTS " + qualifiedName->getQualifiedNameAsString();
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
                       "  , constraint "HBASE_HISTINT_PK" primary key"
                       "     (TABLE_UID ASC, HISTOGRAM_ID ASC, INTERVAL_NUMBER ASC)"
                       " )";

    retcode = executeFromMetaContext(cmd.data());
    if(retcode < 0)
    {
        CmpCommon::diags()->mergeAfter(*(cliInterface_->getDiagsArea()));
        NAString errMsg = "Load histogram data error:  " ;
        errMsg += std::to_string((long long)(retcode)).c_str();
        OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
    }
    
    cmd = "upsert using load into " + qualifiedName->getQualifiedNameAsString() + " select * from hive.hive.";
    cmd += qualifiedName->getCatalogName()+"_"+qualifiedName->getSchemaName()+"_"+qualifiedName->getObjectName();
    retcode = executeFromMetaContext(cmd.data());
    if(retcode < 0)
    {
        CmpCommon::diags()->mergeAfter(*(cliInterface_->getDiagsArea()));
        NAString errMsg = "Load histogram data error:  " ;
        errMsg += std::to_string((long long)(retcode)).c_str();
        OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
    }
    
    return retcode;
}

void OptimizerSimulator::loadHistograms()
{
    debugMessage("loading histograms ...\n");

    OsimElementMapper om;
    OsimAllHistograms * allHistograms = NULL;
    XMLDocument doc(STMTHEAP, om);
    std::ifstream s (sysCallLogFilePath_[HISTOGRAM_PATHS], std::ifstream::binary);
    if(!s.good())
    {
        NAString errMsg = "Error open ";
        errMsg += sysCallLogFilePath_[HISTOGRAM_PATHS];
        OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
    }   
    char * txt = new (STMTHEAP) char[1024];
    s.ignore(OSIM_LINEMAX, '\n');
    s.ignore(OSIM_LINEMAX, '\n');
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
        NAString errMsg = "Error parsing ";
        errMsg += sysCallLogFilePath_[HISTOGRAM_PATHS];
        OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
   } 
   NAHashDictionary<NAString, QualifiedName> * modifiedPathDict = 
                                        new(STMTHEAP) NAHashDictionary<NAString, QualifiedName>
                                                           (&NAString::hash, 101, TRUE, STMTHEAP);
                              
   for(CollIndex i = 0; i < allHistograms->getEntries().entries(); i++)
   {  
       OsimHistogramEntry * en = (allHistograms->getEntries())[i];
       massageTableUID(en, modifiedPathDict);
   }

   //do load
   NAHashDictionaryIterator<NAString, QualifiedName> iterator(*modifiedPathDict);
   NAString* modifiedPath = NULL;
   QualifiedName* qualifiedName = NULL;
   Queue * dummyQueue = NULL;
   iterator.getNext(modifiedPath, qualifiedName) ; 
   while ( modifiedPath && qualifiedName )
   {
       if(qualifiedName->getObjectName().compareTo("SB_HISTOGRAMS", NAString::ignoreCase) == 0)
       {
           loadHistogramsTable(modifiedPath, qualifiedName, OSIM_LINEMAX);
       }
       else if(qualifiedName->getObjectName().compareTo("SB_HISTOGRAM_INTERVALS", NAString::ignoreCase) == 0)
       {
           loadHistogramIntervalsTable(modifiedPath, qualifiedName, OSIM_LINEMAX);
       }
       unlink(modifiedPath->data());
       iterator.getNext(modifiedPath, qualifiedName);
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
   if(!fileExists(sysCallLogFilePath_[CQD_DEFAULTSFILE], isDir))
   {
     char errMsg[38+OSIM_PATHMAX+1]; // Error errMsg below + filename + '\0'
     snprintf(errMsg, sizeof(errMsg), "Unable to open %s file for reading data.",
                       sysCallLogFilePath_[CQD_DEFAULTSFILE]);
     OsimLogException(errMsg, __FILE__, __LINE__).throwException();
   }

   ifstream inLogfile(sysCallLogFilePath_[CQD_DEFAULTSFILE]);
   if(!inLogfile.good())
   {
        NAString errMsg = "Error open ";
        errMsg += sysCallLogFilePath_[CQD_DEFAULTSFILE];
        OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
   } 
   // Read and ignore the top 2 header lines.
   inLogfile.ignore(OSIM_LINEMAX, '\n');
   inLogfile.ignore(OSIM_LINEMAX, '\n');
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
           NAString errMsg = "Error Setting CQD: ";
           errMsg += cqd.c_str();
           OsimLogException(errMsg.data(), __FILE__, __LINE__).throwException();
       }
   }
}

Int64 OptimizerSimulator::getTableUID(const char * catName, const char * schName, const char * objName)
{
   initializeCLI();
   
   Int64 retcode;
   if (cmpSBD_->switchCompiler(CmpContextInfo::CMPCONTEXT_TYPE_META)) {
       OsimLogException("Errors Switch Context.", __FILE__, __LINE__).throwException();
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
       OsimLogException("Errors Switch Context.", __FILE__, __LINE__).throwException();
   }

   retcode = cliInterface_->fetchAllRows(queue_, query, 0, FALSE, FALSE, TRUE);
   //retrieve idag area runing the query above,
   //if there's any error, we can get the detail.
   cliInterface_->retrieveSQLDiagnostics(0);

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
            NAString errMsg;
            errMsg = "Error getting histogram data from ";
            errMsg += src + " to " + dst;
            OsimLogException(errMsg, __FILE__, __LINE__).throwException();
        }
    }
        
    if( hdfsDisconnect(locfs) != 0 ||
         hdfsDisconnect(hdfs) != 0)
    {
            NAString errMsg;
            errMsg = "Error getting histogram data, disconneting";
            OsimLogException(errMsg, __FILE__, __LINE__).throwException();
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
          char errMsg[37+OSIM_PATHMAX+1];
          snprintf (errMsg, sizeof(errMsg), 
                    "Could not create directory %s, permission denied.", 
                    osimLogLocalDir_.data());
          OsimLogException(errMsg, __FILE__, __LINE__).throwException();
        }
        break;
      case ENOENT:
        {
          char errMsg[58+OSIM_PATHMAX+1];
          snprintf (errMsg, sizeof(errMsg), 
                   "Could not create directory %s, a component of the path does not exist.",
                   osimLogLocalDir_.data());
 
          OsimLogException(errMsg, __FILE__, __LINE__).throwException();
        }
        break;
      case EROFS:
        {
          char errMsg[40+OSIM_PATHMAX+1];
          snprintf (errMsg, sizeof(errMsg), 
                    "Could not create directory %s, read-only filesystem.", 
                    osimLogLocalDir_.data());
          OsimLogException(errMsg, __FILE__, __LINE__).throwException();
        }
        break;
      case ENOTDIR:
        {
          char errMsg[62+OSIM_PATHMAX+1];
          snprintf (errMsg, sizeof(errMsg),
                   "Could not create directory %s, a component of the path is not a directory.",
                   osimLogLocalDir_.data());
          OsimLogException(errMsg, __FILE__, __LINE__).throwException();
        }
        break;
      default:
        {
          char errMsg[58+OSIM_PATHMAX+1];
          snprintf (errMsg, sizeof(errMsg),
                   "Could not create %s, errno is %d",
                   osimLogLocalDir_.data(), error);
          OsimLogException(errMsg, __FILE__, __LINE__).throwException();
        }
        break;
    }
}

void OptimizerSimulator::readSysCallLogfiles()
{
  readLogfile_MYSYSTEMNUMBER();
  readLogfile_getEstimatedRows();
  readLogFile_getNodeAndClusterNumbers();
  readLogFile_captureSysType();
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

    hashDict_TablesBeforeAction_   = new (heap_) NAHashDictionary<NAString, Int32>
                                            (&NAString::hash, 101, TRUE, heap_);

    hashDict_ViewsBeforeAction_= new (heap_) NAHashDictionary<NAString, Int32>
                                            (&NAString::hash, 101, TRUE, heap_);
                                            
    hashDictionariesInitialized_ = TRUE;
  }

}

void OptimizerSimulator::setLogFilepath(sysCall sc)
{
  // Allocate memory for file pathname:
  // dirname + '/' + syscallname + ".txt" + '\0'
  size_t pathLen = osimLogLocalDir_.length()+1+strlen(sysCallLogFileName_[sc])+4+1;
  sysCallLogFilePath_[sc] = new (heap_) char[pathLen];
  // Construct an absolute pathname for the file.
  strcpy(sysCallLogFilePath_[sc], osimLogLocalDir_.data());
  strcat(sysCallLogFilePath_[sc], "/");
  strcat(sysCallLogFilePath_[sc], sysCallLogFileName_[sc]);
}

void OptimizerSimulator::openAndAddHeaderToLogfile(sysCall sc)
{
  NABoolean isDir;
  if(fileExists(sysCallLogFilePath_[sc],isDir))
  {
    char errMsg1[118+OSIM_PATHMAX+1]; // Error errMsg below + filename + '\0'
    sprintf(errMsg1, "The target log file %s already exists. "
                      "Delete this and other existing log files before "
                      "running the OSIM in CAPTURE mode.", sysCallLogFilePath_[sc]);
    OsimLogException(errMsg1, __FILE__, __LINE__).throwException();
  }

  // Create the file and write header lines to it.
  writeSysCallStream_[sc] = new (heap_) ofstream(sysCallLogFilePath_[sc],ios::app);
  
  *writeSysCallStream_[sc] << "--" << sysCallLogFileName_[sc] << ":" << endl;
  
  // Indent the output.
  *writeSysCallStream_[sc] << "  " << endl;
}

// Initialize the log files if OSIM is running under either CAPTURE
// or SIMULATE mode. If the OSIM is not running under CAPTURE mode,
// add the header lines to the file. Just set the file name variables
// to NULL if OSIM is not running(OFF).
void OptimizerSimulator::initLogFilePaths()
{
  for (sysCall sc=FIRST_SYSCALL; sc<NUM_OF_SYSCALLS; sc = sysCall(sc+1))
  {
    switch (osimMode_)
    {
      case OFF:
        // OFF mode indicates no log files needed.
        sysCallLogFilePath_[sc] = NULL;
        break;
      case CAPTURE:
        // Set log file path.
        setLogFilepath(sc);
        // Add header to the log file.
        openAndAddHeaderToLogfile(sc);
        break;
      case LOAD:
      case UNLOAD:
      case SIMULATE:
        // Set log file path.
        setLogFilepath(sc);
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
    ofstream * outLogfile = writeSysCallStream_[MYSYSTEMNUMBER];

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

  if(!fileExists(sysCallLogFilePath_[MYSYSTEMNUMBER],isDir))
  {
    char errMsg[38+OSIM_PATHMAX+1]; // Error errMsg below + filename + '\0'
    snprintf(errMsg, sizeof(errMsg), "Unable to open %s file for reading data.",
                       sysCallLogFilePath_[MYSYSTEMNUMBER]);
    OsimLogException(errMsg, __FILE__, __LINE__).throwException();
  }

  ifstream inLogfile(sysCallLogFilePath_[MYSYSTEMNUMBER]);

  // Read and ignore the top 2 header lines.
  inLogfile.ignore(OSIM_LINEMAX, '\n');
  inLogfile.ignore(OSIM_LINEMAX, '\n');

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
      OSIM_errorMessage("Invalid OSIM mode - It must be OFF or CAPTURE or SIMULATE.");
      break;
  }
  return sysNum;
}
// END ************* System Call: MYSYSTEMNUMBER() *************


// BEGIN *********** System Call: getEstimatedRows() ****************
//
void OSIM_captureEstimatedRows(const char *tableName, double estRows)
{
    if(CURRCONTEXT_OPTSIMULATOR)
        CURRCONTEXT_OPTSIMULATOR->capture_getEstimatedRows(tableName, estRows);
}

double OSIM_simulateEstimatedRows(const char *tableName)
{
    return CURRCONTEXT_OPTSIMULATOR ? 
            CURRCONTEXT_OPTSIMULATOR->simulate_getEstimatedRows(tableName) : 
            -1;
}

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
    ofstream * outLogfile = writeSysCallStream_[ESTIMATED_ROWS];
    Int32 origWidth = (*outLogfile).width();
    // Write data at the end of the file.
    (*outLogfile) << "  ";
    (*outLogfile).width(36); (*outLogfile) << tableName << "  ";
    (*outLogfile).width(36); (*outLogfile) << estRows << endl;
    (*outLogfile).width(origWidth);
  }
}


void OptimizerSimulator::readLogfile_getEstimatedRows()
{
  char tableName[ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES+1];
  double estRows;
  NABoolean isDir;

  if(!fileExists(sysCallLogFilePath_[ESTIMATED_ROWS],isDir))
  {
    char errMsg[38+OSIM_PATHMAX+1]; // Error errMsg below + filename + '\0'
    snprintf(errMsg, sizeof(errMsg), "Unable to open %s file for reading data.",
                       sysCallLogFilePath_[ESTIMATED_ROWS]);
    OsimLogException(errMsg, __FILE__, __LINE__).throwException();
  }

  ifstream inLogfile(sysCallLogFilePath_[ESTIMATED_ROWS]);

  // Read and ignore the top 2 header lines.
  inLogfile.ignore(OSIM_LINEMAX, '\n');
  inLogfile.ignore(OSIM_LINEMAX, '\n');

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
    ofstream * outLogfile = writeSysCallStream_[NODE_AND_CLUSTER_NUMBERS];
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

  if(!fileExists(sysCallLogFilePath_[NODE_AND_CLUSTER_NUMBERS],isDir))
  {
    char errMsg[38+OSIM_PATHMAX+1]; // Error errMsg below + filename + '\0'
    snprintf(errMsg, sizeof(errMsg), "Unable to open %s file for reading data.",
                       sysCallLogFilePath_[NODE_AND_CLUSTER_NUMBERS]);
    OsimLogException(errMsg, __FILE__, __LINE__).throwException();
  }

  ifstream inLogfile(sysCallLogFilePath_[NODE_AND_CLUSTER_NUMBERS]);

  // Read and ignore the top 2 header lines.
  inLogfile.ignore(OSIM_LINEMAX, '\n');
  inLogfile.ignore(OSIM_LINEMAX, '\n');

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
      OSIM_errorMessage("Invalid OSIM mode - It must be OFF or CAPTURE or SIMULATE.");
      break;
  }

}

void OptimizerSimulator::capture_CQDs()
{
  NAString cqd(STMTHEAP);
  ofstream * cqdDefaultsLogfile =
               writeSysCallStream_[CQD_DEFAULTSFILE];

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
  const char * viewText = naTab->getViewText();
  const QualifiedName objQualifiedName = naTab->getTableName();
  const NAString nastrQualifiedName = objQualifiedName.getQualifiedNameAsString();
  
  // Handle Synonym first
  if(naTab->getIsSynonymTranslationDone())
  {
    NAString synRefName = naTab->getSynonymReferenceName();

    if(!hashDict_Synonyms_->contains(&objQualifiedName))
    {      
      ofstream * synonymListFile = writeSysCallStream_[SYNONYMSFILE];
      (*synonymListFile) << objQualifiedName.getQualifiedNameAsAnsiString().data() <<endl;

      ofstream * synonymLogfile = writeSysCallStream_[SYNONYMDDLS];
      
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
    if(!hashDict_Views_->contains(&objQualifiedName) &&
         hashDict_ViewsBeforeAction_->contains(&nastrQualifiedName)
    )
    {
      // Open file in append mode.
      ofstream * viewsListFile = writeSysCallStream_[VIEWSFILE];
      (*viewsListFile) << objQualifiedName.getQualifiedNameAsAnsiString().data() <<endl;
      
      ofstream * viewLogfile = writeSysCallStream_[VIEWDDLS];

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
          {
               OsimLogException("Errors Dumping Table DDL.", __FILE__, __LINE__).throwException();
          }
          
          capture_TableOrView(otherNaTable);
    }
    //end capture referred tables
    
    if(!hashDict_Tables_->contains(&objQualifiedName) && 
         //and only capture tables exist before capture command is issued.
         hashDict_TablesBeforeAction_->contains(&nastrQualifiedName) 
      )
    {
      // Open file in append mode.
      ofstream * tablesListFile = writeSysCallStream_[TABLESFILE];
      (*tablesListFile) << objQualifiedName.getQualifiedNameAsAnsiString().data() <<endl;

      // insert tableName into hash table
      // this is used to check if the table has already
      // been written out to disk
      QualifiedName * tableName = new (heap_) QualifiedName(objQualifiedName, heap_);
      //save table uid for dump historgram data when done
      Int64 * tableUID = new Int64(naTab->objectUid().get_value());
      hashDict_Tables_->insert(tableName, tableUID);
      dumpDDLs(objQualifiedName);
    }
  }
}

void OptimizerSimulator::captureQueryText(const char * query)
{
  // Open file in append mode.
  ofstream * outLogfile = writeSysCallStream_[QUERIESFILE];

  //(*outLogfile) << "--BeginQuery" << endl;
  (*outLogfile) << query ;
  Int32 queryStrLen = strlen(query);
  // put in a semi-colon at end of query if it is missing
  if (query[queryStrLen]!= ';')
    (*outLogfile) << ";";
  (*outLogfile) << endl;
  //(*outLogfile) << "--EndQuery" << endl;
}

void OSIM_captureQueryText(const char * query)
{
  if(CURRCONTEXT_OPTSIMULATOR)
      CURRCONTEXT_OPTSIMULATOR->captureQueryText(query);
}

void OptimizerSimulator::captureQueryShape(const char * shape)
{
  // Open file in append mode.
  ofstream * outLogfile = writeSysCallStream_[QUERIESFILE];

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
      gpClusterInfo->captureNAClusterInfo(*writeSysCallStream_[NACLUSTERINFO]);
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
  for (sysCall sc=FIRST_SYSCALL; sc<NUM_OF_SYSCALLS; sc = sysCall(sc+1))
  {
    if (sysCallLogFilePath_[sc])
    {
      NADELETEBASIC(sysCallLogFilePath_[sc],heap_); 
      sysCallLogFilePath_[sc]=NULL;
    }
    
    if(writeSysCallStream_[sc])
    {
      writeSysCallStream_[sc]->close();
      NADELETE(writeSysCallStream_[sc], ofstream, heap_); 
      writeSysCallStream_[sc]=NULL;
    }
    
  }

  if(hashDict_getEstimatedRows_)
      hashDict_getEstimatedRows_->clear(TRUE);
  if(hashDict_Views_)
      hashDict_Views_->clear(TRUE);
  if(hashDict_Tables_)
      hashDict_Tables_->clear(TRUE);
  if(hashDict_Synonyms_)
      hashDict_Synonyms_->clear(TRUE);
  
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
  //nodeNum_ = -1;
  //clusterNum_ = -1;
  //capturedNodeAndClusterNum_ = FALSE;
  //hdfsDisconnect(fs_);
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

  ofstream* outLogfile= writeSysCallStream_[CAPTURE_SYS_TYPE];
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
  if(!fileExists(sysCallLogFilePath_[CAPTURE_SYS_TYPE],isDir))
  {
    captureSysType_ = OSIM_UNKNOWN_SYSTYPE;
    char errMsg[38+OSIM_PATHMAX+1]; // Error errMsg below + filename + '\0'
    snprintf(errMsg, sizeof(errMsg), "Unable to open %s file for reading data.",
                       sysCallLogFilePath_[CAPTURE_SYS_TYPE]);
    OsimLogException(errMsg, __FILE__, __LINE__).throwException();
  }

  ifstream inLogfile(sysCallLogFilePath_[CAPTURE_SYS_TYPE]);

  // Read and ignore the top 2 header lines.
  inLogfile.ignore(OSIM_LINEMAX, '\n');
  inLogfile.ignore(OSIM_LINEMAX, '\n');

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
