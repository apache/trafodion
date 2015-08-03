/**********************************************************************
 *
 * File:         CmpMemoryMonitor.cpp
 * Description:  The memory monitor records and logs memory usage for
 *               different phases in compiler/optimizer.
 * 
 * Created:      June 2007
 * Language:     C++
 * 
 *
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

#include "CmpMemoryMonitor.h"
#include "NADefaults.h"
#include "SchemaDB.h"

// Global pointer to the CmpMemoryMonitor that encapsulates of
// Memory Usage Information in SQL/MX compiler/optimizer.
THREAD_P CmpMemoryMonitor *cmpMemMonitor = NULL;

// Constructor for MemoryUsage
MemoryUsage::MemoryUsage(const char *phaseName, CollHeap *heap)
{
  phaseName_ = new(heap) char[strlen(phaseName)+1];
  strcpy(phaseName_, phaseName);
  stmtHBegin_ = stmtHEnd_ = stmtHAllocSize_ = stmtHHighWaterMark_ = 0;
  cxtHBegin_ = cxtHEnd_ = cxtHAllocSize_ = cxtHHighWaterMark_ = 0;
  heap_ = heap;
}

// Another constructor for MemoryUsage
MemoryUsage::MemoryUsage(const char *phaseName,
                         size_t stmtHBegin, size_t stmtHEnd,
                         Int32 stmtHAllocSize, size_t stmtHHighWaterMark,
                         size_t cxtHBegin, size_t cxtHEnd, 
                         Int32 cxtHAllocSize, size_t cxtHHighWaterMark,
                         CollHeap *heap)
{
  phaseName_ = new(heap) char[strlen(phaseName)+1];
  strcpy(phaseName_, phaseName);
  stmtHBegin_ = stmtHBegin;
  stmtHEnd_ = stmtHEnd;
  stmtHAllocSize_ = stmtHAllocSize;
  stmtHHighWaterMark_ = stmtHHighWaterMark;
  cxtHBegin_ = cxtHBegin;
  cxtHEnd_ = cxtHEnd;
  cxtHAllocSize_ = cxtHAllocSize;
  cxtHHighWaterMark_ = cxtHHighWaterMark;
  heap_ = heap;
}

// Update the phase name with additional info.
// There are case (such the optimizer task count) that needs to updated
// to reflect the correct values that embedded in the phase name.
void MemoryUsage::updatePhaseName(char *updatedPhaseName)
{
  if (updatedPhaseName)
  {
    NADELETEBASIC(phaseName_, heap_);
    phaseName_ = new(heap_) char[strlen(updatedPhaseName)+1]; 
    strcpy(phaseName_, updatedPhaseName);
  }
}

// Record the memory usage information at the beginning of the phase.
void MemoryUsage::enter(CollHeap *otherHeap)
{
  if (otherHeap)
  {
    // otherHeap is passed hesre in a special case for NATable when 
    // it uses its own heap instead of statement heap.
    stmtHBegin_ = otherHeap->getAllocSize();
    stmtHHighWaterMark_ = otherHeap->getHighWaterMark();
  }
  else
  {
    stmtHBegin_ = CmpCommon::statementHeap()->getAllocSize();
    stmtHHighWaterMark_ = CmpCommon::statementHeap()->getHighWaterMark();
  }
  cxtHBegin_ = CmpCommon::contextHeap()->getAllocSize();
  cxtHHighWaterMark_ = CmpCommon::contextHeap()->getHighWaterMark();
}

// Record the memory usage information at the end of the phase and 
// compute the difference to get actual allocations.
void MemoryUsage::exit(CollHeap *otherHeap)
{
  if (otherHeap)
  {
    // otherHeap is passed hesre in a special case for NATable when 
    // it uses its own heap instead of statement heap.
    stmtHEnd_ = otherHeap->getAllocSize();
    stmtHHighWaterMark_ = otherHeap->getHighWaterMark();
  }
  else
  {
    stmtHEnd_ = CmpCommon::statementHeap()->getAllocSize();
    stmtHHighWaterMark_ = CmpCommon::statementHeap()->getHighWaterMark();
  }
  cxtHEnd_ = CmpCommon::contextHeap()->getAllocSize();
  cxtHHighWaterMark_ = CmpCommon::contextHeap()->getHighWaterMark();

  // Compute allocations during a phase.
  stmtHAllocSize_ = (Int32)stmtHEnd_ - (Int32)stmtHBegin_;
  cxtHAllocSize_  = (Int32)cxtHEnd_ - (Int32)cxtHBegin_;
}

// Log memory usage for a phase.
void MemoryUsage::logMemoryUsage(ofstream *logFilestream)
{
  if (logFilestream)
  {
    if (strlen(phaseName_) > 23)
    {
      (*logFilestream) << phaseName_ << ": " << endl;
      (*logFilestream).width(23); (*logFilestream) << "" << "  ";
    }
    else
    {
     (*logFilestream).width(23); (*logFilestream) << phaseName_ << ": ";
    }
    (*logFilestream).width(11); (*logFilestream) << stmtHAllocSize_ << " ";
    (*logFilestream).width(15); (*logFilestream) << stmtHHighWaterMark_ << " ";
    (*logFilestream).width(11); (*logFilestream) << cxtHAllocSize_ << " ";
    (*logFilestream).width(15); (*logFilestream) << cxtHHighWaterMark_ << endl;
  }
  else 
  {
    // If output logfile is not specified, use std out.
    cout.width(23); cout << phaseName_ << ": ";
    cout.width(11); cout << stmtHAllocSize_ << " ";
    cout.width(15); cout << stmtHHighWaterMark_ << " ";
    cout.width(11); cout << cxtHAllocSize_ << " ";
    cout.width(15); cout << cxtHHighWaterMark_ << endl;
  }
}

// This operator is a must since we are using NAHashDictionary.
NABoolean MemoryUsage::operator==(const MemoryUsage& mu)
{
  if (strcmp(phaseName_, mu.phaseName_)             ||
      stmtHBegin_ != mu.stmtHBegin_                 ||
      stmtHEnd_  != mu.stmtHEnd_                    ||
      stmtHAllocSize_ != mu.stmtHAllocSize_         ||
      stmtHHighWaterMark_ != mu.stmtHHighWaterMark_ ||
      cxtHBegin_ != mu.cxtHBegin_                   ||
      cxtHEnd_ != mu.cxtHEnd_                       ||
      cxtHAllocSize_ != mu.cxtHAllocSize_           ||
      cxtHHighWaterMark_ != mu.cxtHHighWaterMark_)
    return FALSE;
  return TRUE;
}

// Constructor
CmpMemoryMonitor::CmpMemoryMonitor(CollHeap *heap)
{
  isMemMonitor_ = FALSE;
  isMemMonitorInDetail_ = FALSE;
  isLogInstantly_ = FALSE;
  logFilename_ = NULL;
  logFilestream_ = NULL;
  queryText_ = NULL;
  hd_MemoryUsage_ = NULL;
  memUsageList_ = NULL;
  heap_ = heap;
}

// Hash fucntion used for storing memory usage information.
ULng32 cmmHashFunc_NAString(const NAString& str)
{
  return (ULng32) NAString::hash(str);
}

void CmpMemoryMonitor::setIsMemMonitor(NABoolean isMemMonitor)
{
  isMemMonitor_ = isMemMonitor;
  if (isMemMonitor_ && !hd_MemoryUsage_)
    // Create a new NAHashDictionary if it hasn't been already created.
    hd_MemoryUsage_ = new(heap_) NAHashDictionary<NAString, MemoryUsage>
                                 (&cmmHashFunc_NAString, 101, TRUE, heap_);
  if (isMemMonitor_ && !memUsageList_)
    // Create a new NAList if it hasn't been already created.
    memUsageList_ = new(heap_) NAList<MemoryUsage *>(heap_);
}

NABoolean CmpMemoryMonitor::getIsMemMonitor()
{
  return isMemMonitor_;
}

void CmpMemoryMonitor::setIsMemMonitorInDetail(NABoolean isMemMonitorInDetail)
{
  if (isMemMonitorInDetail)
    // if isMemMonitorInDetail is being set then set isMemMonitor_ as well.
    isMemMonitor_ = isMemMonitorInDetail;
  isMemMonitorInDetail_ = isMemMonitorInDetail;
  if (isMemMonitor_ && !hd_MemoryUsage_)
    // Create a new NAHashDictionary if it hasn't been already created.
    hd_MemoryUsage_ = new(heap_) NAHashDictionary<NAString, MemoryUsage>
                                 (&cmmHashFunc_NAString, 101, TRUE, heap_);
  if (isMemMonitor_ && !memUsageList_)
    // Create a new NAList if it hasn't been already created.
    memUsageList_ = new(heap_) NAList<MemoryUsage *>(heap_);
}

NABoolean CmpMemoryMonitor::getIsMemMonitorInDetail()
{
  return isMemMonitorInDetail_;
}

void CmpMemoryMonitor::setLogFilename(const char *logFilename)
{
  if(logFilename) 
  {
    // Clean up file name and stream since we are changing the 
    // output filename.
    fileCleanUp();

    setIsMemMonitor(TRUE);

    logFilename_ = new(heap_) char[strlen(logFilename)+1];
    strcpy(logFilename_, logFilename);
    // Initialize the log file and write appropriate headers.
    ofstream outLogFile(logFilename_);
    outLogFile.width(19); outLogFile << "";
    outLogFile << "Memory Usage Information for SQL/MX Compiler" << endl;
    outLogFile.width(23); outLogFile << "" << "  ";
    outLogFile.width(11); outLogFile << "StmtH Alloc" << " ";
    outLogFile.width(15); outLogFile << "StmtH HgWtrMark" << " ";
    outLogFile.width(11); outLogFile << "CxtH Alloc" << " ";
    outLogFile.width(15); outLogFile << "CxtH HgWtrMark" << endl;
    outLogFile.close();

    // Now open the file stream to append memery usage data as we get it.
    logFilestream_ = new(heap_) ofstream(logFilename_, ios::app);
  }
}

char*  CmpMemoryMonitor::getLogFilename()
{
  return logFilename_;
}

void CmpMemoryMonitor::setIsLogInstantly(NABoolean isLogInstantly)
{
  isLogInstantly_ = isLogInstantly;
}

NABoolean CmpMemoryMonitor::getIsLogInstantly()
{
  return isLogInstantly_;
}

// Capture Query Text of a query that is being monitored for 
// memory usage.
void CmpMemoryMonitor::setQueryText(char *queryText)
{
  queryText_ = new(heap_) char[strlen(queryText)+1];
  strcpy(queryText_, queryText);
  if (isLogInstantly_)
  {
    if (logFilestream_)
      (*logFilestream_) << "Query: \n" << queryText_ << endl << endl;
    else
      cout << "Query: \n" << queryText_ << endl << endl;
  }
}

char* CmpMemoryMonitor::getQueryText()
{
  return queryText_;
}

// Cleanup file information when another filename is being used.
void CmpMemoryMonitor::fileCleanUp()
{
  // Delete memory allocated for the filename.
  if (logFilename_)
  {
    NADELETEBASIC(logFilename_, heap_); 
    logFilename_ = NULL;
  }
  // Delete memory allocated for the output file stream and call
  // the destructor for ofstream.
  if (logFilestream_)
  {
    logFilestream_->close();
    NADELETE(logFilestream_, ofstream, heap_); 
    logFilestream_ = NULL;
  }
}

// Cleanup after each statement
void CmpMemoryMonitor::cleanupPerStatement()
{
  // Delete queryText_
  if (queryText_)
  {
    NADELETEBASIC(queryText_, heap_); 
    queryText_ = NULL;
  }
  // Clear NAHashDictionary  hd_MemoryUsage_.
  if(hd_MemoryUsage_)
    hd_MemoryUsage_->clear();

  // Clear NAList muiList_.
  if(memUsageList_)
    memUsageList_->clear();
}

// Log memory usge for all phases at the end of monitoring 
// memory usage.
void CmpMemoryMonitor::logMemoryUsageAll()
{
  // If isLogInstantly_ is set, the information is already logged.
  if (!isLogInstantly_)
  {
    if (queryText_)
    {
      if (logFilestream_)
        (*logFilestream_) << "Query: \n" << queryText_ << endl << endl;
      else
        cout << "Query: \n" << queryText_ << endl;
    }
    for (CollIndex i=0; i<memUsageList_->entries(); i++)
      (*memUsageList_)[i]->logMemoryUsage(logFilestream_);
  }
}

// Record memory usage for a given phase at the start.
void CmpMemoryMonitor::enter(const char *phaseName, 
                             CollHeap *otherHeap)
{
  NAString *key_phaseName   = new(heap_) NAString(phaseName, heap_);
  MemoryUsage *val_memUsage = new(heap_) MemoryUsage(phaseName, heap_);
  val_memUsage->enter(otherHeap);
  // Insert MemoryUsage into list. This list will be used while
  // writing to the file or std out to keep the order of the phases
  // as they were encountered.
  memUsageList_->insert(val_memUsage);
  if (!hd_MemoryUsage_->contains(key_phaseName))
    // Insert MemoryUsage into hash dictionary. This hash dictionary
    // will be used to search for a memory usage for a given phase to
    // update memory usage information. This will very helpful if 
    // memory is monitored more often (such as for each optimizer task 
    // or after every few optimizer tasks).
    NAString *check = hd_MemoryUsage_->insert(key_phaseName, val_memUsage);
}

// Record memory usage for a given phase at the end.
void CmpMemoryMonitor::exit(const char *phaseName, 
                            CollHeap *otherHeap,
                            char *updatedPhaseName) 
{
  NAString key_phaseName(phaseName, heap_);
  MemoryUsage *val_memUsage = hd_MemoryUsage_->getFirstValue(&key_phaseName); 
  if (val_memUsage)
  {
    if(updatedPhaseName)
      val_memUsage->updatePhaseName(updatedPhaseName);
    val_memUsage->exit(otherHeap);
    if (isLogInstantly_)
      val_memUsage->logMemoryUsage(logFilestream_);
  }
}

// Record query text.
void MonitorMemoryUsage_QueryText(char *queryText)
{
  if (cmpMemMonitor->getIsMemMonitor())
    cmpMemMonitor->setQueryText(queryText);
}

// Wrapper function to record memory usage at the beginning of the
// phase using CmpMemoryMonitor.
void MonitorMemoryUsage_Enter(const char *phaseName, 
                              CollHeap *otherHeap, 
                              NABoolean isDetail)
{
  // Log memory usage information only when the query text is set. 
  // If the query text is not set, it may be due to one of the 
  // memory monitoring CQDs that set isMemMoniotor_ caused the
  // recording of memory usage information. So we discard that info.
  if (cmpMemMonitor->getQueryText())
  {
    if(isDetail)
    {
      // isDetail_ is set, so record memory usage info.
      if (cmpMemMonitor->getIsMemMonitorInDetail())
        cmpMemMonitor->enter(phaseName, otherHeap);
    }
    else
    {
      if (cmpMemMonitor->getIsMemMonitor())
        cmpMemMonitor->enter(phaseName, otherHeap);
    }
  }
}

// Wrapper function to record memory usage at the end of the phase using
// CmpMemoryMonitor.
void MonitorMemoryUsage_Exit(const char *phaseName, 
                             CollHeap *otherHeap, 
                             char *updatedPhaseName, 
                             NABoolean isDetail)
{
  // Log memory usage information only when the query text is set. 
  // If the query text is not set, it may be due to one of the 
  // memory monitoring CQDs that set isMemMoniotor_ caused the
  // recording of memory usage information. So we discard that info.
  if (cmpMemMonitor->getQueryText())
  {
    if(isDetail)
    {
      // isDetail_ is set, so record memory usage info.
      if (cmpMemMonitor->getIsMemMonitorInDetail())
        cmpMemMonitor->exit(phaseName, otherHeap, updatedPhaseName);
    }
    else
    {
      if (cmpMemMonitor->getIsMemMonitor())
        cmpMemMonitor->exit(phaseName, otherHeap, updatedPhaseName);
    }
  }
}

// Log all memory usage information.
void MonitorMemoryUsage_LogAll()
{
  // Log memory usage information only when the query text is set. 
  // If the query text is not set, it may be due to one of the 
  // memory monitoring CQDs that set isMemMoniotor_ caused the
  // recording of memory usage information. So we discard that info.
  if (cmpMemMonitor->getQueryText())
  {
    if (cmpMemMonitor->getIsMemMonitor())
      cmpMemMonitor->logMemoryUsageAll();
  }
}
