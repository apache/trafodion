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

#include "HDFSHook.h"
#include "hiveHook.h"
#include "CmpCommon.h"
#include "SchemaDB.h"
#include "ComCextdecs.h"

// for DNS name resolution
#include <netdb.h>
#include "HdfsClient_JNI.h"
#include "Globals.h"
#include "Context.h"
// Initialize static variables
THREAD_P CollIndex HHDFSMasterHostList::numSQNodes_(0);
THREAD_P NABoolean HHDFSMasterHostList::hasVirtualSQNodes_(FALSE);

HHDFSMasterHostList::~HHDFSMasterHostList()
{
}

// translate a host name to a number (add host if needed)
HostId HHDFSMasterHostList::getHostNum(const char *hostName)
{
  if (getHosts()->entries() == 0)
    initializeWithSeaQuestNodes();

  return getHostNumInternal(hostName);
}

// translate a host name to a number (add host if needed)
HostId HHDFSMasterHostList::getHostNumInternal(const char *hostName)
{
  for (CollIndex i=0; i<getHosts()->entries(); i++)
    if (strcmp(hostName,getHosts()->at(i)) == 0)
      return i;

  char *hostCopy = new char[strlen(hostName)+1];

  strcpy(hostCopy, hostName);
  getHosts()->insertAt(getHosts()->entries(),hostCopy);
  return getHosts()->entries() - 1;
}

// get host name from host number
const char * HHDFSMasterHostList::getHostName(HostId hostNum)
{
  return getHosts()->at(hostNum);
}

NABoolean HHDFSMasterHostList::initializeWithSeaQuestNodes()
{
  NABoolean result = FALSE;
  FILE *pp;
  NAString fakeNodeNames =
    ActiveSchemaDB()->getDefaults().getValue(HIVE_USE_FAKE_SQ_NODE_NAMES);

  if (fakeNodeNames.length() <= 1)
    {
      // execute the command "sqshell -c node" and open a pipe to the output of this command.
      pp = popen("sqshell -c node", "r");
      if (pp != NULL)
        {
          // we want to add all the nodes returned by sqshell such that their HostIds
          // assigned here in class HHDFSMasterHostList matches their SeaQuest host number
          HostId nextHostId = getHosts()->entries();

          while (1)
            {
              char *line;
              char buf[1000];
              line = fgets(buf, sizeof buf, pp);
              if (line == NULL)
                {
                  // if we inserted anything without encountering an error, consider that success
                  numSQNodes_ = getHosts()->entries();
                  result = (numSQNodes_ > 0);
                  break;
                }
              char *nodeNum = strstr(line, "Node[");
              if (nodeNum)
                {
                  nodeNum += 5; // skip the matched text
                  int nodeId = atoi(nodeNum);
                  if (nodeId != nextHostId)
                    break; // out-of-sequence host ids are not supported

                  char *nodeName = strstr(nodeNum, "=");
                  if (nodeName == NULL)
                    break; // expecting "=" sign in the sqshell output
                  nodeName++;
                  char *nodeEnd = strstr(nodeName, ",");
                  if (nodeEnd == NULL)
                    break; // couldn't find the end of the node name
                  *nodeEnd = 0;

                  // resolve the found name to make it a fully qualified DNS name,
                  // like HDFS also uses it
                  struct hostent * h = gethostbyname(nodeName);
                  if (h)
                    nodeName = h->h_name;

                  HostId checkId = getHostNumInternal(nodeName);
                  if (checkId != nodeId)
                    if (checkId > nodeId)
                      break; // something must have gone wrong, this should not happen
                    else
                      {
                        // checkId < nodeId, this can happen if we have duplicate
                        // node ids. In this case, insert a dummy node to take up the
                        // number, so we stay in sync
                        sprintf(buf, "dummy.node.%d.nosite.com", nodeId);
                        HostId checkId2 = getHostNumInternal(buf);
                        if (checkId2 != nodeId)
                          break; // again, not expecting to get here
                        // remember that we mave multiple SQ nodes
                        // on the same physical node
                        hasVirtualSQNodes_ = TRUE;
                      }
                  nextHostId++;
                }
            }
          pclose(pp);
        }
    }
  else
    {
      // seed the host name list with fake SQ node names from the CQD insted
      const char delim = ',';
      const char *nodeStart = fakeNodeNames;
      const char *nodeEnd;

      do
        {
          // this is debug code, no error check, no blanks in this string!!!
          char buf[500];

          nodeEnd = strchrnul(nodeStart, delim);
          strncpy(buf, nodeStart, nodeEnd-nodeStart);
          getHostNumInternal(buf);
          nodeStart = nodeEnd+1;
        }
      while (*nodeEnd != 0);
      
      numSQNodes_ = getHosts()->entries();
      result = (numSQNodes_ > 0);
    }
  return result;
}

void HHDFSDiags::recordError(const char *errMsg,
                             const char *errLoc)
{
  // don't overwrite the original error
  if (success_)
    {
      success_ = FALSE;
      errMsg_ = errMsg;
      if (errLoc)
        errLoc_ = errLoc;
    }
}

void HHDFSStatsBase::add(const HHDFSStatsBase *o)
{
  numBlocks_ += o->numBlocks_;
  numFiles_ += o->numFiles_; 
  totalSize_ += o->totalSize_;
  if (o->modificationTS_ > modificationTS_)
    modificationTS_ = o->modificationTS_ ;
  sampledBytes_ += o->sampledBytes_;
  sampledRows_ += o->sampledRows_;
}

void HHDFSStatsBase::subtract(const HHDFSStatsBase *o)
{
  numBlocks_ -= o->numBlocks_;
  numFiles_ -= o->numFiles_; 
  totalSize_ -= o->totalSize_;
  sampledBytes_ -= o->sampledBytes_;
  sampledRows_ -= o->sampledRows_;
}

Int64 HHDFSStatsBase::getEstimatedRowCount() const
{
   return  ( getTotalSize() / getEstimatedRecordLength() );
}

Int64 HHDFSStatsBase::getEstimatedRecordLength() const
{
  return MAXOF(sampledBytes_ / (sampledRows_ ? sampledRows_ : 1), 1);
}

Int64 HHDFSStatsBase::getEstimatedBlockSize() const
{
  return MAXOF(totalSize_ / (numBlocks_ ? numBlocks_ : 1), 32768);
}

void HHDFSStatsBase::print(FILE *ofd, const char *msg)
{
  fprintf(ofd,"File stats at %s level:\n", msg);
  fprintf(ofd," ++ numBlocks:    %ld\n",  numBlocks_);
  fprintf(ofd," ++ numFiles:     %ld\n",  numFiles_);
  fprintf(ofd," ++ totalSize:    %ld\n",  totalSize_);
  fprintf(ofd," ++ sampledBytes: %ld\n",  sampledBytes_);
  fprintf(ofd," ++ sampledRows:  %ld\n",  sampledRows_);
}

HHDFSFileStats::~HHDFSFileStats()
{
  if (blockHosts_)
    NADELETEBASIC(blockHosts_, heap_);
}

static void sortHostArray(HostId *blockHosts,
                          Int32 numBlocks,
                          Int32 replication, 
                          const NAString &randomizer)
{
  // the hdfsGetHosts() call randomizes the hosts for 1st, 2nd and 3rd replica etc.
  // for each call, probably to get more even access patterns. This makes it hard
  // to debug the placement algorithm, since almost no 2 query plans are alike.
  // Replace the random method of hdfsGetHosts with a pseudo-random one,
  // based on the file name. With no randomization we would put a bigger load
  // on hosts with a lower id.

  // we have replication * numBlocks entries in blockHosts, with entry
  // (r * numBlocks + b) being the rth replica of block #b.

  if (replication > 1 && replication <= 10)
    {
      UInt32 rshift = (UInt32) randomizer.hash();

      for (Int32 b=0; b<numBlocks; b++)
        {
          // a sorted array of HostIds for a given block
          HostId s[10];

          // insert the first v
          s[0]=blockHosts[b];
          for (Int32 r=1; r<replication; r++)
            {
              HostId newVal = blockHosts[r*numBlocks + b];

              // replication is a small number, bubblesort of s will do...
              for (Int32 x=0; x<r; x++)
                if (newVal < s[x])
                  {
                    // shift the larger values up by 1
                    for (Int32 y=r; y>x; y--)
                      s[y] = s[y-1];
                    // then insert the new value
                    s[x] = newVal;
                    break;
                  }
                else if (x == r-1)
                  // new value is the largest, insert at end
                  s[r] = newVal;
            } // for each replica host of a block

          // now move sorted values in s back to blockHosts,
          // but shift them by rshift mod replication
          for (Int32 m=0; m<replication; m++)
            blockHosts[m*numBlocks + b] = s[((UInt32) m + rshift + (UInt32) b) % replication];

        } // for each block b
    } // replication between 2 and 10
} // sortHostArray

void HHDFSFileStats::populate(hdfsFS fs, hdfsFileInfo *fileInfo, 
                              Int32& samples,
                              HHDFSDiags &diags,
                              NABoolean doEstimation,
                              char recordTerminator)
{
  // copy fields from fileInfo
  fileName_       = fileInfo->mName;
  replication_    = (Int32) fileInfo->mReplication;
  totalSize_      = (Int64) fileInfo->mSize;
  blockSize_      = (Int64) fileInfo->mBlockSize;
  modificationTS_ = fileInfo->mLastMod;
  numFiles_       = 1;

  Int64 sampleBufferSize = MINOF(blockSize_, 65536);
  NABoolean sortHosts = (CmpCommon::getDefault(HIVE_SORT_HDFS_HOSTS) == DF_ON);

  sampleBufferSize = MINOF(sampleBufferSize,totalSize_/10);

  if (doEstimation && sampleBufferSize > 100) {

     // 
     // Open the hdfs file to estimate record length. Read one block at
     // a time searching for <s> instances of record separators. Stop reading 
     // when either <s> instances have been found or a partial number of
     // instances have and we have exhausted all data content in the block.
     // We will keep reading if the current block does not contain 
     // any instance of the record separator.
     hdfsFile file = 
                 hdfsOpenFile(fs, fileInfo->mName, 
                              O_RDONLY, 
                              sampleBufferSize, // buffer size
                              0, // replication, take the default size 
                              fileInfo->mBlockSize // blocksize 
                              ); 
     if ( file != NULL ) {
        tOffset offset = 0;
        tSize bufLen = sampleBufferSize;
        char* buffer = new (heap_) char[bufLen+1];

        buffer[bufLen] = 0; // extra null at the end to protect strchr()
                            // to run over the buffer.

        NABoolean sampleDone = FALSE;

        Int32 totalSamples = 10;
        Int32 totalLen = 0;
        Int32 recordPrefixLen = 0;
   
        while (!sampleDone) {
   
           tSize szRead = hdfsPread(fs, file, offset, buffer, bufLen);
           if ( szRead <= 0 )
              break;

           CMPASSERT(szRead <= bufLen);

            char* pos = NULL;
   
             //if (isSequenceFile && offset==0 && memcmp(buffer, "SEQ6", 4) == 0)
             //   isSequenceFile_ = TRUE;
   
           char* start = buffer;
   

           for (Int32 i=0; i<totalSamples; i++ ) {
   
              if ( (pos=strchr(start, recordTerminator)) ) {
   
                 totalLen += pos - start + 1 + recordPrefixLen;
                 samples++;
   
                 start = pos+1;
   
                 if ( start > buffer + szRead ) {
                    sampleDone = TRUE;
                    break;
                 }

                 recordPrefixLen = 0;

              } else {
                 recordPrefixLen += szRead - (start - buffer + 1);
                 break;
              }
          }
          if ( samples > 0 )
             break;
          else
             offset += szRead;
        }
        NADELETEBASIC(buffer, heap_);
        if ( samples > 0 ) {
           sampledBytes_ += totalLen;
           sampledRows_  += samples;
        }
        hdfsCloseFile(fs, file);
     } else {
       diags.recordError(NAString("Unable to open HDFS file ") + fileInfo->mName,
                         "HHDFSFileStats::populate");
     }
  }
  if (blockSize_)
    {
      numBlocks_ = totalSize_ / blockSize_;
      if (totalSize_ % blockSize_ > 0)
        numBlocks_++; // partial block at the end
    }
  else
    {
      diags.recordError(NAString("Could not determine block size of HDFS file ") + fileInfo->mName,
                        "HHDFSFileStats::populate");
    }
  if ( totalSize_ > 0 && diags.isSuccess())
    {

      blockHosts_ = new(heap_) HostId[replication_*numBlocks_];

      // walk through blocks and record their locations
      tOffset o = 0;
      Int64 blockNum;
      for (blockNum=0; blockNum < numBlocks_ && diags.isSuccess(); blockNum++)
        {
          char*** blockHostNames = hdfsGetHosts(fs,
                                                fileInfo->mName, 
                                                o,
                                                fileInfo->mBlockSize);

          o += blockSize_;

          if (blockHostNames == NULL)
            {
              diags.recordError(NAString("Could not determine host of blocks for HDFS file ") + fileInfo->mName,
                                "HHDFSFileStats::populate");
            }
          else
            {
              char **h = *blockHostNames;
              HostId hostId;

              for (Int32 r=0; r<replication_; r++)
                {
                  if (h[r])
                    hostId = HHDFSMasterHostList::getHostNum(h[r]);
                  else
                    hostId = HHDFSMasterHostList::InvalidHostId;
                  blockHosts_[r*numBlocks_+blockNum] = hostId;
                }
              if (sortHosts)
                sortHostArray(blockHosts_,
                              (Int32) numBlocks_,
                              replication_,
                              getFileName());
            }
          hdfsFreeHosts(blockHostNames);
        }
    }
}

void HHDFSFileStats::print(FILE *ofd)
{
  fprintf(ofd,"-----------------------------------\n");
  fprintf(ofd,">>>> File:       %s\n", fileName_.data());
  fprintf(ofd,"  replication:   %d\n", replication_);
  fprintf(ofd,"  block size:    %ld\n", blockSize_);
  fprintf(ofd,"  mod timestamp: %d\n", (Int32) modificationTS_);
  fprintf(ofd,"\n");
  fprintf(ofd,"            host for replica\n");
  fprintf(ofd,"  block #     1    2    3    4\n");
  fprintf(ofd,"  --------- ---- ---- ---- ----\n");
  for (Int32 b=0; b<numBlocks_; b++)
    fprintf(ofd,"  %9d %4d %4d %4d %4d\n",
            b,
            getHostId(0, b),
            (replication_ >= 2 ? getHostId(1, b) : -1),
            (replication_ >= 3 ? getHostId(2, b) : -1),
            (replication_ >= 4 ? getHostId(3, b) : -1));
  HHDFSStatsBase::print(ofd, "file");
}

HHDFSBucketStats::~HHDFSBucketStats()
{
  for (CollIndex i=0; i<fileStatsList_.entries(); i++)
    delete fileStatsList_[i];
}

void HHDFSBucketStats::addFile(hdfsFS fs, hdfsFileInfo *fileInfo, 
                               HHDFSDiags &diags,
                               NABoolean doEstimate, 
                               char recordTerminator,
                               CollIndex pos)
{
  HHDFSFileStats *fileStats = new(heap_) HHDFSFileStats(heap_, getTable());

  if ( scount_ > 10 )
    doEstimate = FALSE;

  Int32 sampledRecords = 0;

  fileStats->populate(fs, fileInfo, sampledRecords, diags,
                      doEstimate, recordTerminator);

  if (diags.isSuccess())
    {
      if ( sampledRecords > 0 )
        scount_++;

      if (pos == NULL_COLL_INDEX)
        fileStatsList_.insert(fileStats);
      else
        fileStatsList_.insertAt(pos, fileStats);
      add(fileStats);
    }
}

void HHDFSBucketStats::removeAt(CollIndex i)
{
  HHDFSFileStats *e = fileStatsList_[i];
  subtract(e);
  fileStatsList_.removeAt(i);
  delete e;
}

void HHDFSBucketStats::print(FILE *ofd)
{
  for (CollIndex f=0; f<fileStatsList_.entries(); f++)
    fileStatsList_[f]->print(ofd);
  HHDFSStatsBase::print(ofd, "bucket");
}

OsimHHDFSStatsBase* HHDFSBucketStats::osimSnapShot(NAMemory * heap)
{
    OsimHHDFSBucketStats* stats = new(heap) OsimHHDFSBucketStats(NULL, this, heap);
    
    for(Int32 i = 0; i < fileStatsList_.getUsedLength(); i++){
            //"gaps" are not added, but record the position
            if(fileStatsList_.getUsage(i) != UNUSED_COLL_ENTRY)
                stats->addEntry(fileStatsList_[i]->osimSnapShot(heap), i);
    }
    return stats;
}


HHDFSListPartitionStats::~HHDFSListPartitionStats()
{
  for (CollIndex b=0; b<=defaultBucketIdx_; b++)
    if (bucketStatsList_.used(b))
      delete bucketStatsList_[b];
}

void HHDFSListPartitionStats::populate(hdfsFS fs,
                                       const NAString &dir,
                                       Int32 numOfBuckets,
                                       HHDFSDiags &diags,
                                       NABoolean doEstimation,
                                       char recordTerminator)
{
  int numFiles = 0;

  // remember parameters
  partitionDir_     = dir;
  defaultBucketIdx_ = (numOfBuckets >= 1) ? numOfBuckets : 0;
  doEstimation_     = doEstimation;
  recordTerminator_ = recordTerminator;

  // to avoid a crash, due to lacking permissions, check the directory
  // itself first
  hdfsFileInfo *dirInfo = hdfsGetPathInfo(fs, dir.data());
  
  if (!dirInfo)
    {
      diags.recordError(NAString("Could not access HDFS directory ") + dir,
                        "HHDFSListPartitionStats::populate");
    }
  else
    {
      dirInfo_ = *dirInfo;

      // list all the files in this directory, they all belong
      // to this partition and either belong to a specific bucket
      // or to the default bucket
      hdfsFileInfo *fileInfos = hdfsListDirectory(fs,
                                                  dir.data(),
                                                  &numFiles);

      // populate partition stats
      for (int f=0; f<numFiles && diags.isSuccess(); f++)
        if (fileInfos[f].mKind == kObjectKindFile)
          {
            // the default (unbucketed) bucket number is
            // defaultBucketIdx_
            Int32 bucketNum = determineBucketNum(fileInfos[f].mName);
            HHDFSBucketStats *bucketStats = NULL;

            if (! bucketStatsList_.used(bucketNum))
              {
                bucketStats = new(heap_) HHDFSBucketStats(heap_, getTable());
                bucketStatsList_.insertAt(bucketNum, bucketStats);
              }
            else
              bucketStats = bucketStatsList_[bucketNum];

            bucketStats->addFile(fs, &fileInfos[f], diags, doEstimation, recordTerminator);
          }

      hdfsFreeFileInfo(fileInfos, numFiles);
      hdfsFreeFileInfo(dirInfo,1);

      // aggregate statistics over all buckets
      for (Int32 b=0; b<=defaultBucketIdx_; b++)
        if (bucketStatsList_.used(b))
          add(bucketStatsList_[b]);
    }
}

NABoolean HHDFSListPartitionStats::validateAndRefresh(hdfsFS fs, HHDFSDiags &diags, NABoolean refresh)
{
  NABoolean result = TRUE;

  // assume we get the files sorted by file name
  int numFiles = 0;
  Int32 lastBucketNum = -1;
  ARRAY(Int32) fileNumInBucket(HEAP, getLastValidBucketIndx()+1);
  HHDFSBucketStats *bucketStats = NULL;

  for (CollIndex i=0; i<=getLastValidBucketIndx(); i++)
    fileNumInBucket.insertAt(i, (Int32) -1);

  // to avoid a crash, due to lacking permissions, check the directory
  // itself first
  hdfsFileInfo *dirInfo = hdfsGetPathInfo(fs, partitionDir_.data());

  if (!dirInfo)
    // don't set diags, let caller re-read the entire stats
    return FALSE;

  // list directory contents and compare with cached statistics
  hdfsFileInfo *fileInfos = hdfsListDirectory(fs,
                                              partitionDir_.data(),
                                              &numFiles);
  CMPASSERT(fileInfos || numFiles == 0);

  // populate partition stats
  for (int f=0; f<numFiles && result; f++)
    if (fileInfos[f].mKind == kObjectKindFile)
      {
        Int32 bucketNum = determineBucketNum(fileInfos[f].mName);

        if (bucketNum != lastBucketNum)
          {
            if (! bucketStatsList_.used(bucketNum))
              {
                // first file for a new bucket got added
                if (!refresh)
                  return FALSE;
                bucketStats = new(heap_) HHDFSBucketStats(heap_, getTable());
                bucketStatsList_.insertAt(bucketNum, bucketStats);
              }
            else
              bucketStats = bucketStatsList_[bucketNum];
            lastBucketNum = bucketNum;
          }

        // file stats for an existing file, or NULL
        // for a new file
        HHDFSFileStats *fileStats = NULL;
        // position in bucketStats of the file (existing or new)
        fileNumInBucket[bucketNum] = fileNumInBucket[bucketNum] + 1;

        if (fileNumInBucket[bucketNum] < bucketStats->entries())
          fileStats = (*bucketStats)[fileNumInBucket[bucketNum]];
        // else this is a new file, indicated by fileStats==NULL

        if (fileStats &&
            fileStats->getFileName() == fileInfos[f].mName)
          {
            // file still exists, check modification timestamp
            if (fileStats->getModificationTS() !=
                fileInfos[f].mLastMod ||
                fileStats->getTotalSize() !=
                (Int64) fileInfos[f].mSize)
              {
                if (refresh)
                  {
                    // redo this file, it changed
                    subtract(fileStats);
                    bucketStats->removeAt(fileNumInBucket[bucketNum]);
                    fileStats = NULL;
                  }
                else
                  result = FALSE;
              }
            // else this file is unchanged from last time
          } // file name matches
        else
          {
            if (refresh)
              {
                if (fileStats)
                  {
                    // We are looking at a file in the directory, fileInfos[f]
                    // and at a file stats entry, with names that do not match.
                    // This could be because a new file got inserted or because
                    // the file of our file stats entry got deleted or both.
                    // We can only refresh this object in the first case, if
                    // a file got deleted we will return FALSE and not refresh.

                    // check whether fileStats got deleted,
                    // search for fileStats->getFileName() in the directory
                    int f2;
                    for (f2=f+1; f2<numFiles; f2++)
                      if (fileStats->getFileName() == fileInfos[f2].mName)
                        break;

                    if (f2<numFiles)
                      {
                        // file fileInfos[f] got added, don't consume
                        // a FileStats entry, instead add it below
                        fileStats = NULL;
                      }
                    else
                      {
                        // file fileStats->getFileName() got deleted,
                        // it's gone from the HDFS directory,
                        // give up and redo the whole thing
                        result = FALSE;
                      }
                  }
                // else file was inserted (fileStats is NULL)
              }
            else
              result = FALSE;
          } // file names for HHDFSFileStats and directory don't match

        if (result && !fileStats)
          {
            // add this file
            bucketStats->addFile(fs,
                                 &fileInfos[f],
                                 diags,
                                 doEstimation_,
                                 recordTerminator_,
                                 fileNumInBucket[bucketNum]);
            if (!diags.isSuccess())
              {
                result = FALSE;
              }
            else
              add((*bucketStats)[fileNumInBucket[bucketNum]]);
          }
      } // loop over actual files in the directory

  hdfsFreeFileInfo(fileInfos, numFiles);
  hdfsFreeFileInfo(dirInfo,1);
  // check for file stats that we did not visit at the end of each bucket
  for (CollIndex i=0; i<=getLastValidBucketIndx() && result; i++)
    if (bucketStatsList_.used(i) &&
        bucketStatsList_[i]->entries() != fileNumInBucket[i] + 1)
      result = FALSE; // some files got deleted at the end

  return result;
}

Int32 HHDFSListPartitionStats::determineBucketNum(const char *fileName)
{
  Int32 result = 0;
  HHDFSBucketStats *bucketStats;

  // determine bucket number (from file name for bucketed tables)
  if (defaultBucketIdx_ <= 1)
    return 0;

  // figure out name from file prefix bb..bb_*
  const char *mark = fileName + strlen(fileName) - 1;

  // search backwards for the last slash in the name or the start
  while (*mark != '/' && mark != fileName)
    mark--;

  if (*mark == '/')
    mark++;

  // go forward, expect digits, followed by an underscore
  while (*mark >= '0' && *mark <= '9' && result < defaultBucketIdx_)
    {
      result = result*10 + (*mark - '0');
      mark++;
    }

  // we should see an underscore as a separator
  if (*mark != '_' || result > defaultBucketIdx_)
    {
      // this file has no valid bucket number encoded in its name
      // use an artificial bucket number "defaultBucketIdx_" in this case
      result = defaultBucketIdx_;
    }

  return result;
}

void HHDFSListPartitionStats::print(FILE *ofd)
{
  fprintf(ofd,"------------- Partition %s\n", partitionDir_.data());
  fprintf(ofd," num of buckets: %d\n", defaultBucketIdx_);

  for (CollIndex b=0; b<=defaultBucketIdx_; b++)
    if (bucketStatsList_.used(b))
      {
        fprintf(ofd,"---- statistics for bucket %d:\n", b);
        bucketStatsList_[b]->print(ofd);
      }
  HHDFSStatsBase::print(ofd, "partition");
}

OsimHHDFSStatsBase* HHDFSListPartitionStats::osimSnapShot(NAMemory * heap)
{
    OsimHHDFSListPartitionStats* stats = new(heap) OsimHHDFSListPartitionStats(NULL, this, heap);

    for(Int32 i = 0; i < bucketStatsList_.getUsedLength(); i++)
    {
        //"gaps" are not added, but record the position
        if(bucketStatsList_.getUsage(i) != UNUSED_COLL_ENTRY)
            stats->addEntry(bucketStatsList_[i]->osimSnapShot(heap), i);
    }
    return stats;
}

HHDFSTableStats::~HHDFSTableStats()
{
  for (int p=0; p<totalNumPartitions_; p++)
    delete listPartitionStatsList_[p];
}

NABoolean HHDFSTableStats::populate(struct hive_tbl_desc *htd)
{
  // here is the basic outline how this works:
  //
  // 1. Walk SD descriptors of the table, one for the table
  //    itself and one for each partition. Each one represents
  //    one HDFS directory with files for the table.
  // 2. For each list partition directory (or the directory for
  //    an unpartitioned table):
  //     3. Walk through every file. For every file:
  //         4. Determine bucket number (0 if file is not bucketed)
  //         5. Add file to its bucket
  //         6. Walk through blocks of file. For every block:
  //             7. Get host list for this block and add it
  //         9. Get file stats
  //     10. Aggregate file stats for all files and buckets
  // 11. Aggregate bucket stats for all buckets of the partition
  // 12. Aggregate partition stats for all partitions of the table

  struct hive_sd_desc *hsd = htd->getSDs();
  if (hsd == NULL)
    return TRUE; // nothing need to be done

  diags_.reset();
  tableDir_ = hsd->location_;
  numOfPartCols_ = htd->getNumOfPartCols();
  recordTerminator_ = hsd->getRecordTerminator();
  fieldTerminator_ = hsd->getFieldTerminator() ;
  nullFormat_ = hsd->getNullFormat();
  NAString hdfsHost;
  Int32 hdfsPort = -1;
  NAString tableDir;

  if (hsd) {
     if (hsd->isTextFile())
        type_ = TEXT_;
     else if (hsd->isSequenceFile())
        type_ = SEQUENCE_;
     else if (hsd->isOrcFile())
        type_ = ORC_;
     else
        type_ = UNKNOWN_;
  }
  // split table URL into host, port and filename
  if (! splitLocation(hsd->location_,
                hdfsHost,
                hdfsPort,
                tableDir,
                diags_,
                hdfsPortOverride_)) 
      return FALSE;
  if (! connectHDFS(hdfsHost, hdfsPort)) 
     return FALSE; // diags_ is set
  // put back fully qualified URI
  tableDir = hsd->location_;
  computeModificationTSmsec();
  if (diags_.isSuccess()) {
     modificationTSInMillisec_ = htd->setRedeftime(modificationTSInMillisec_);
     while (hsd && diags_.isSuccess()) {
        // visit the directory
        processDirectory(hsd->location_, hsd->buckets_, 
                       hsd->isTrulyText(), 
                       hsd->getRecordTerminator());

        hsd = hsd->next_;
     }
  }

  disconnectHDFS();
  validationJTimestamp_ = JULIANTIMESTAMP();

  return diags_.isSuccess();
}

NABoolean HHDFSTableStats::validateAndRefresh(Int64 expirationJTimestamp, NABoolean refresh)
{
  NABoolean result = TRUE;
  // initial heap allocation size
  Int32 initialSize = heap_->getAllocSize();

  diags_.reset();

  // check if the stats needs to be fetched within a specified time interval
  // when not requested to refresh
  if (! refresh && (expirationJTimestamp == -1 ||
      (expirationJTimestamp > 0 &&
       validationJTimestamp_ < expirationJTimestamp)))
    return result; // consider the stats still valid

  // if partitions get added or deleted, that gets
  // caught in the Hive metadata, so no need to check for
  // that here
  for (int p=0; p<totalNumPartitions_ && result && diags_.isSuccess(); p++)
    {
      HHDFSListPartitionStats *partStats = listPartitionStatsList_[p];
      NAString hdfsHost;
      Int32 hdfsPort;
      NAString partDir;

      result = splitLocation(partStats->getDirName(),
                             hdfsHost,
                             hdfsPort, 
                             partDir,
                             diags_,
                             hdfsPortOverride_);
      if (! result)
        break;

      if (! connectHDFS(hdfsHost, hdfsPort))
        return FALSE;

      subtract(partStats);
      result = partStats->validateAndRefresh(fs_, diags_, refresh);
      if (result)
        add(partStats);
    }

  disconnectHDFS();
  validationJTimestamp_ = JULIANTIMESTAMP();
  // account for the heap used by stats. Heap released during
  // stats refresh will also be included
  hiveStatsSize_ += (heap_->getAllocSize() - initialSize);

  return result;
}

NABoolean HHDFSTableStats::splitLocation(const char *tableLocation,
                                         NAString &hdfsHost,
                                         Int32 &hdfsPort,
                                         NAString &tableDir,
                                         HHDFSDiags &diags,
                                         int hdfsPortOverride)
{
  const char *hostMark = NULL;
  const char *portMark = NULL;
  const char *dirMark  = NULL;
  const char *fileSysTypeTok = NULL;

  // The only two filesysTypes supported are hdfs: and maprfs:
  // One of these two tokens must appear at the the start of tableLocation

  // hdfs://localhost:35000/hive/tpcds/customer
  if (fileSysTypeTok = strstr(tableLocation, "hdfs:"))
    tableLocation = fileSysTypeTok + 5; 
  // maprfs:/user/hive/warehouse/f301c7af0-2955-4b02-8df0-3ed531b9abb/select
  else if (fileSysTypeTok = strstr(tableLocation, "maprfs:"))
    tableLocation = fileSysTypeTok + 7; 
  else
    {
      diags.recordError(NAString("Expected hdfs: or maprfs: in the HDFS URI ") + tableLocation,
                        "HHDFSTableStats::splitLocation");
      return FALSE;
    }

  
  // The characters that  come after "//" is the hostName.
  // "//" has to be at the start of the string (after hdfs: or maprfs:)
  if ((hostMark = strstr(tableLocation, "//"))&&
      (hostMark == tableLocation))
    {
      hostMark = hostMark + 2;
      
      dirMark = strchr(hostMark, '/');
      if (dirMark == NULL)
        {
          diags.recordError(NAString("Could not find slash in HDFS directory name ") + tableLocation,
                            "HHDFSTableStats::splitLocation");
          return FALSE;
        }

      // if there is a hostName there could be a hostPort too.
      // It is not not an error if there is a hostName but no hostPort
      // for example  hdfs://localhost/hive/tpcds/customer is valid
      portMark = strchr(hostMark, ':');
      if (portMark && (portMark < dirMark))
        portMark = portMark +1 ;
      else
        portMark = NULL; 
    }
  else // no host location, for example maprfs:/user/hive/warehouse/
    {
      hostMark = NULL;
      portMark = NULL;
      if (*tableLocation != '/') 
        {
          diags.recordError(NAString("Expected a maprfs:/<filename> URI: ") + tableLocation,
                            "HHDFSTableStats::splitLocation");
          return FALSE;
        }
      dirMark = tableLocation;
    }

  
  if (hostMark)
    hdfsHost    = NAString(hostMark, (portMark ? portMark-hostMark-1
                                      : dirMark-hostMark));
  else
    hdfsHost = NAString("default");

  if (hdfsPortOverride > -1)
    hdfsPort    = hdfsPortOverride;
  else
    if (portMark)
      hdfsPort  = atoi(portMark);
    else
      hdfsPort  = 0;
  tableDir      = NAString(dirMark);
  return TRUE;
}

void HHDFSTableStats::processDirectory(const NAString &dir, Int32 numOfBuckets, 
                                       NABoolean doEstimate, char recordTerminator)
{
  HHDFSListPartitionStats *partStats = new(heap_)
    HHDFSListPartitionStats(heap_, this);
  partStats->populate(fs_, dir, numOfBuckets, diags_, doEstimate, recordTerminator);

  if (diags_.isSuccess())
    {
      listPartitionStatsList_.insertAt(listPartitionStatsList_.entries(), partStats);
      totalNumPartitions_++;
      // aggregate stats
      add(partStats);

      if (partStats->dirInfo()->mLastMod > modificationTS_)
        modificationTS_ = partStats->dirInfo()->mLastMod;
    }
}

Int32 HHDFSTableStats::getNumOfConsistentBuckets() const
{
  Int32 result = 0;

  // go through all partitions and chck whether they have
  // the same # of buckets and have no files w/o enforced bucketing
  for (Int32 i=0; i<listPartitionStatsList_.entries(); i++)
    {
      Int32 b = listPartitionStatsList_[i]->getLastValidBucketIndx();

      if (result == 0)
        result = b;
      if (b <= 1 || b != result)
        return 1; // some partition not bucketed or different from others
      if ((*listPartitionStatsList_[i])[b] != NULL)
        return 1; // this partition has files that are not bucketed at all
                  // and are therefore assigned to the exception bucket # b
    }
  // everything is consistent, with multiple buckets
  return result;
}

void HHDFSTableStats::setupForStatement()
{
}

void HHDFSTableStats::resetAfterStatement()
{
}

void HHDFSTableStats::print(FILE *ofd)
{
  fprintf(ofd,"====================================================================\n");
  fprintf(ofd,"HDFS file stats for directory %s\n", tableDir_.data());
  fprintf(ofd,"  number of part cols: %d\n", numOfPartCols_);
  fprintf(ofd,"  total number of partns: %d\n", totalNumPartitions_);
  fprintf(ofd,"  Record Terminator: %d\n", recordTerminator_);
  fprintf(ofd,"  Field Terminator: %d\n", fieldTerminator_);
  
  for (CollIndex p=0; p<listPartitionStatsList_.entries(); p++)
    listPartitionStatsList_[p]->print(ofd);
  HHDFSStatsBase::print(ofd, "table");
  fprintf(ofd,"\n");
  fprintf(ofd,"Host id to host name table:\n");
  CollIndex numHosts = HHDFSMasterHostList::entries();
  for (HostId h=0; h<numHosts; h++)
    fprintf(ofd, "   %4d: %s\n", h, HHDFSMasterHostList::getHostName(h));
  
  fprintf(ofd,"\n");
  fprintf(ofd,"end of HDFS file stats for directory %s\n", tableDir_.data());
  fprintf(ofd,"====================================================================\n");
}


NABoolean HHDFSTableStats::connectHDFS(const NAString &host, Int32 port)
{
  NABoolean result = TRUE;

  // establish connection to HDFS . Conect to the connection cached in the context.
 
  
  fs_ = ((GetCliGlobals()->currContext())->getHdfsServerConnection((char *)host.data(),port));
     
      
      if (fs_ == NULL)
        {
          NAString errMsg("hdfsConnect to ");
          errMsg += host;
          errMsg += ":";
          errMsg += port;
          errMsg += " failed";
          diags_.recordError(errMsg, "HHDFSTableStats::connectHDFS");
          result = FALSE;
        }
      currHdfsHost_ = host;
      currHdfsPort_ = port;
      //  }
  return result;
}

void HHDFSTableStats::disconnectHDFS()
{
  // No op. The disconnect happens at the context level wehn the session 
  // is dropped or the thread exits.
}

void HHDFSTableStats::computeModificationTSmsec()
{
      HDFS_Client_RetCode rc;

      // get a millisecond-resolution timestamp via JNI
      rc = HdfsClient::getHiveTableMaxModificationTs(
               modificationTSInMillisec_,
               tableDir_.data(),
               numOfPartCols_);
      // check for errors and timestamp mismatches
      if (rc != HDFS_CLIENT_OK || modificationTSInMillisec_ <= 0)
        {
          NAString errMsg;

          errMsg.format("Error %d when reading msec timestamp for HDFS URL %s",
                        rc,
                        tableDir_.data());
          diags_.recordError(errMsg, "HHDFSTableStats::computeModificationTSmsec");
          modificationTSInMillisec_ = -1;
        }
}

OsimHHDFSStatsBase* HHDFSTableStats::osimSnapShot(NAMemory * heap)
{
    OsimHHDFSTableStats* stats = new(heap) OsimHHDFSTableStats(NULL, this, heap);

    for(Int32 i = 0; i < listPartitionStatsList_.getUsedLength(); i++)
    {
        //"gaps" are not added, but record the position
        if(listPartitionStatsList_.getUsage(i) != UNUSED_COLL_ENTRY)
            stats->addEntry(listPartitionStatsList_[i]->osimSnapShot(heap), i);
    }
    return stats;
}

OsimHHDFSStatsBase* HHDFSFileStats::osimSnapShot(NAMemory * heap)
{
    OsimHHDFSFileStats* stats = new(heap) OsimHHDFSFileStats(NULL, this, heap);

    return stats;
}



