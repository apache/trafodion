// **********************************************************************
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
// **********************************************************************

#include "ComTdbHdfsScan.h"
#include "ComTdbCommon.h"

// Dummy constructor for "unpack" routines.
ComTdbHdfsScan::ComTdbHdfsScan():
 ComTdb(ComTdb::ex_HDFS_SCAN, eye_HDFS_SCAN)
{};

// Constructor

ComTdbHdfsScan::ComTdbHdfsScan(
                               char * tableName,
                               short type,
                               ex_expr * select_pred,
                               ex_expr * move_expr,
                               ex_expr * convert_expr,
                               ex_expr * move_convert_expr,
                               UInt16 convertSkipListSize,
                               Int16 * convertSkipList,
                               char * hostName,
                               tPort port,
                               Queue * hdfsFileInfoList,
                               Queue * hdfsFileRangeBeginList,
                               Queue * hdfsFileRangeNumList,
                               char recordDelimiter,
                               char columnDelimiter,
                               char * nullFormat,
                               Int64 hdfsBufSize,
                               UInt32 rangeTailIOSize,
                               Int64 hdfsSqlMaxRecLen,
                               Int64 outputRowLength,
                               Int64 asciiRowLen,
                               Int64 moveColsRowLen,
                               const unsigned short tuppIndex,
                               const unsigned short asciiTuppIndex,
                               const unsigned short workAtpIndex,
                               const unsigned short moveColsTuppIndex,
                               const unsigned short origTuppIndex,
                               ex_cri_desc * work_cri_desc,
                               ex_cri_desc * given_cri_desc,
                               ex_cri_desc * returned_cri_desc,
                               queue_index down,
                               queue_index up,
                               Cardinality estimatedRowCount,
                               Int32  numBuffers,
                               UInt32  bufferSize,
                               char * errCountTable,
                               char * loggingLocation,
                               char * errCountId,

                               char * hdfsRootDir,
                               Int64  modTSforDir,
                               Lng32  numOfPartCols,
                               Queue * hdfsDirsToCheck

                               )
: ComTdb( ComTdb::ex_HDFS_SCAN,
            eye_HDFS_SCAN,
            estimatedRowCount,
            given_cri_desc,
            returned_cri_desc,
            down,
            up,
            numBuffers,        // num_buffers - we use numInnerTuples_ instead.
            bufferSize),       // buffer_size - we use numInnerTuples_ instead.
  tableName_(tableName),
  type_(type),
  selectPred_(select_pred),
  moveExpr_(move_expr),
  convertExpr_(convert_expr),
  moveColsConvertExpr_(move_convert_expr),
  convertSkipListSize_(convertSkipListSize),
  convertSkipList_(convertSkipList),
  hostName_(hostName),
  port_(port),
  hdfsFileInfoList_(hdfsFileInfoList),
  hdfsFileRangeBeginList_(hdfsFileRangeBeginList),
  hdfsFileRangeNumList_(hdfsFileRangeNumList),
  recordDelimiter_(recordDelimiter),
  columnDelimiter_(columnDelimiter),
  nullFormat_(nullFormat),
  hdfsBufSize_(hdfsBufSize),
  rangeTailIOSize_(rangeTailIOSize),
  hdfsSqlMaxRecLen_(hdfsSqlMaxRecLen),
  outputRowLength_(outputRowLength),
  asciiRowLen_(asciiRowLen),
  moveExprColsRowLength_(moveColsRowLen),
  origTuppIndex_(origTuppIndex),
  tuppIndex_(tuppIndex),
  asciiTuppIndex_(asciiTuppIndex),
  workAtpIndex_(workAtpIndex),
  moveExprColsTuppIndex_(moveColsTuppIndex),
  workCriDesc_(work_cri_desc),
  flags_(0),
  errCountTable_(errCountTable),
  loggingLocation_(loggingLocation),
  errCountRowId_(errCountId),
  hdfsRootDir_(hdfsRootDir),
  modTSforDir_(modTSforDir),
  numOfPartCols_(numOfPartCols),
  hdfsIoByteArraySizeInKB_(0),
  hdfsDirsToCheck_(hdfsDirsToCheck)
{};

ComTdbHdfsScan::~ComTdbHdfsScan()
{};

void ComTdbHdfsScan::display() const 
{};

Long ComTdbHdfsScan::pack(void * space)
{
  tableName_.pack(space);
  selectPred_.pack(space);
  moveExpr_.pack(space);
  convertExpr_.pack(space);
  moveColsConvertExpr_.pack(space);
  convertSkipList_.pack(space);
  hostName_.pack(space);
  workCriDesc_.pack(space);

  // pack elements in hdfsFileInfoList
  getHdfsFileInfoList()->position();
  for (Lng32 i = 0; i < getHdfsFileInfoList()->numEntries(); i++)
    {
      HdfsFileInfo * hdf = (HdfsFileInfo*)getHdfsFileInfoList()->getNext();
      
      hdf->fileName_.pack(space);
    }

  hdfsFileInfoList_.pack(space);
  hdfsFileRangeBeginList_.pack(space);
  hdfsFileRangeNumList_.pack(space);

  nullFormat_.pack(space);

  errCountTable_.pack(space);
  loggingLocation_.pack(space);
  errCountRowId_.pack(space);

  hdfsRootDir_.pack(space);
  hdfsDirsToCheck_.pack(space);

  return ComTdb::pack(space);
}

Lng32 ComTdbHdfsScan::unpack(void * base, void * reallocator)
{
  if (tableName_.unpack(base)) return -1;
  if(selectPred_.unpack(base, reallocator)) return -1;
  if(moveExpr_.unpack(base, reallocator)) return -1;
  if(convertExpr_.unpack(base, reallocator)) return -1;
  if(moveColsConvertExpr_.unpack(base, reallocator)) return -1;
  if(convertSkipList_.unpack(base)) return -1;
  if(hostName_.unpack(base)) return -1;
  if(workCriDesc_.unpack(base, reallocator)) return -1;

  if (hdfsFileInfoList_.unpack(base, reallocator)) return -1;

  // unpack elements in hdfsFileInfoList
  getHdfsFileInfoList()->position();
  for (Lng32 i = 0; i < getHdfsFileInfoList()->numEntries(); i++)
    {
      HdfsFileInfo * hdf = (HdfsFileInfo*)getHdfsFileInfoList()->getNext();
      
      if (hdf->fileName_.unpack(base)) return -1;
    }

  if (hdfsFileRangeBeginList_.unpack(base, reallocator)) return -1;
  if (hdfsFileRangeNumList_.unpack(base, reallocator)) return -1;

  if (nullFormat_.unpack(base)) return -1;

  if (errCountTable_.unpack(base)) return -1;
  if (loggingLocation_.unpack(base)) return -1;
  if (errCountRowId_.unpack(base)) return -1;

  if (hdfsRootDir_.unpack(base)) return -1;
  if (hdfsDirsToCheck_.unpack(base, reallocator)) return -1;

  return ComTdb::unpack(base, reallocator);
}

void ComTdbHdfsScan::displayContents(Space * space,ULng32 flag)
{
  ComTdb::displayContents(space,flag & 0xFFFFFFFE);

  if(flag & 0x00000008)
    {
      char buf[16384];

      str_sprintf(buf, "\nFor ComTdbHdfsScan :");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "tableName_ = %s", (char*)tableName_);
     space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      if (hostName_)
        str_sprintf(buf, "hostName_ = %s, port_ = %d", 
                       (char *) hostName_, port_);
      else
        str_sprintf(buf, "hostName_ = NULL (empty scan)");
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "recordDelimiter_ = %d, columnDelimiter_ = %d", 
                         recordDelimiter_ ,  columnDelimiter_ );
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "hdfsBufSize_ = %ld, rangeTailIOSize_ = %d, "
                       "hdfsSqlMaxRecLen_ = %ld",
                        hdfsBufSize_ ,  rangeTailIOSize_, hdfsSqlMaxRecLen_ );
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "tuppIndex_ = %d, workAtpIndex_ = %d",
                          tuppIndex_ ,      workAtpIndex_ );
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "asciiTuppIndex_ = %d, asciiRowLen_ = %ld",
                          asciiTuppIndex_ ,      asciiRowLen_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "moveExprColsTuppIndex_ = %d, moveExprColsRowLength_ = %ld",
                          moveExprColsTuppIndex_ ,      moveExprColsRowLength_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      if (convertSkipListSize_)
        {
          // only the first element of the skip list is printed for now
          str_sprintf(buf, "convertSkipListSize_ = %d, convertSkipList_ = %d",
                      convertSkipListSize_ ,      convertSkipList_[0]);
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
        }

      str_sprintf(buf, "outputRowLength_ = %ld", outputRowLength_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      str_sprintf(buf, "Flag = %x",flags_);
      space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

      Queue *hdfsFileInfoList = hdfsFileInfoList_;
      if (hdfsFileInfoList)
        {
          UInt32 dataElems = hdfsFileInfoList->numEntries();
          str_sprintf(buf, "\nNumber of ranges to scan: %d",
                      (Int32) dataElems);
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

          Lng32 currInstNum = -1;
          Lng32 currEntryNum = 0;
          Lng32 startOfNextInst = 0;
          HdfsFileInfo * hdfo;

          hdfsFileInfoList->position();
          while ((hdfo = (HdfsFileInfo*)hdfsFileInfoList->getNext()) != NULL)
            {
              if (currEntryNum >= startOfNextInst)
                {
                  currInstNum++;
                  startOfNextInst = 
                    *((Lng32 *) getHdfsFileRangeBeginList()->get(currInstNum)) +
                    *((Lng32 *) getHdfsFileRangeNumList()->get(currInstNum));
                }
	      currEntryNum++;
            }

         Int32 numESPs = currInstNum+1;
         str_sprintf(buf, "Number of esps to scan:    %d\n",
                      numESPs);
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

	  currInstNum = -1;
          currEntryNum = 0;
          startOfNextInst = 0;

          Int64 bytesPerESP[4096];
          Int64 totalBytes = 0;

          for (Int32 i=0; i<4096; i++) 
	    bytesPerESP[i] = 0;

          if (isOrcFile())
            str_sprintf(
                        buf, 
                        "%6s %6s %12s %12s  %s",
                        "  Esp#", "Range#", " StartRowNum", "     NumRows", "    FileName");
          else
            str_sprintf(
                        buf, 
                        "%6s %6s %12s %12s  %s",
                        "  Esp#", "Range#", " StartOffset", "   BytesRead", "    FileName");

           space->allocateAndCopyToAlignedSpace(buf, str_len(buf),
                                                   sizeof(short));
   
	  str_sprintf(
		      buf, "====== ====== ============ ============  ==============================\n");
           space->allocateAndCopyToAlignedSpace(buf, str_len(buf),
                                                   sizeof(short));

          hdfsFileInfoList->position();
          while ((hdfo = (HdfsFileInfo*)hdfsFileInfoList->getNext()) != NULL)
            {
              if (currEntryNum >= startOfNextInst)
                {
                  currInstNum++;
                  startOfNextInst = 
                    *((Lng32 *) getHdfsFileRangeBeginList()->get(currInstNum)) +
                    *((Lng32 *) getHdfsFileRangeNumList()->get(currInstNum));
                }

	      NABoolean printStr = FALSE;
	      NABoolean isLocal = FALSE;
	      char splitInfo[200];
	      strcpy(splitInfo, " ");
              if (hdfo->fileIsLocal()) //hdfo->getFlags() & HdfsFileInfo::HDFSFILEFLAGS_LOCAL)
		{
		  isLocal = TRUE;
		  strcpy(splitInfo, "(local");
		}

	      if ((hdfo->fileIsSplitBegin()) ||
		  (hdfo->fileIsSplitEnd()))
		{
		  if (isLocal)
		    strcat(splitInfo, ", ");
		  else
		    strcpy(splitInfo, "(");

		  printStr = TRUE;
		  strcat(splitInfo, "split_");
		  if (hdfo->fileIsSplitBegin())
		    {
		      strcat(splitInfo, "b");
		    
		      if (hdfo->fileIsSplitEnd())
			strcat(splitInfo, "/");
		    }

		  if (hdfo->fileIsSplitEnd())
		    strcat(splitInfo, "e");

		  strcat(splitInfo, ")");
		}
	      else if (isLocal)
		strcat(splitInfo, ")");

              if (isSequenceFile())
                {
                  if ( isLocal ||
                       (hdfo->fileIsSplitBegin()) ||
                       (hdfo->fileIsSplitEnd()))
                     strcat(splitInfo, ", ");
                     
                  strcat(splitInfo, "seq");
                }
                
	      // filename is fully qualified addr and looks like this:
	      // hdfs://....com/hive/warehouse/so_dtl_f_test/000030_0
	      // To make it little easier to read, if it starts with "hdfs://", then pick
	      // the last 2 parts, or maybe 3 parts at some point.
	      Lng32 i = 0;
	      if (str_cmp(hdfo->fileName(), "hdfs://", strlen("hdfs://")) == 0)
		{
		  i = strlen(hdfo->fileName()) - 1;
		  Lng32 numParts = 2;
		  NABoolean done = FALSE;
		  while (NOT done)
		    {
		      if ((i > 0) && (numParts > 0) && (hdfo->fileName()[i] != '/'))
			i--;
		      else
			{
			  if (i <= 0)
			    done = TRUE;
			  else 
			    {
			      numParts--;
			      if (numParts == 0)
				{
				  i++;
				  done = TRUE;
				}
			      else
				i--;
			    } // else
			} // else
		    } // while
		} // if

              str_sprintf(
                   buf, 
		   "%6d %6d %12ld %12ld  %s %s",
                   currInstNum,
                   currEntryNum,
                   hdfo->getStartOffset(),
                   hdfo->getBytesToRead(),
                   &hdfo->fileName()[i],
		   splitInfo);

              space->allocateAndCopyToAlignedSpace(buf, str_len(buf),
                                                   sizeof(short));
              bytesPerESP[currInstNum] += (hdfo->getBytesToRead() >= 0 ? hdfo->getBytesToRead() : 0);
              totalBytes += (hdfo->getBytesToRead() >= 0 ? hdfo->getBytesToRead() : 0);
              currEntryNum++;
            }

          str_sprintf(buf, "\nSummary of bytes read per ESP (%ld = 100 percent):\n",
                      totalBytes);
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf),
                                               sizeof(short));

          if (totalBytes == 0)
            totalBytes = 1; // avoid divide by zero below

          numESPs = currInstNum+1;

          for (Int32 e=0; e<numESPs; e++)
            {
              str_sprintf(buf,"ESP %4d reads %18ld bytes (%4ld percent of avg)",
                          e,
                          bytesPerESP[e],
                          (Int32) 100 * bytesPerESP[e] * numESPs / totalBytes);
              space->allocateAndCopyToAlignedSpace(buf, str_len(buf),
                                                   sizeof(short));
            }
        }

      if (hdfsRootDir_)
        {
          str_sprintf(buf, "hdfsRootDir: %s", hdfsRootDir());
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

          str_sprintf(buf, "modTSforDir_ = %ld, numOfPartCols_ = %d",
                      modTSforDir_, numOfPartCols_);
          space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));

          if (hdfsDirsToCheck())
            {
              hdfsDirsToCheck()->position();
              char * dir = NULL;
              while ((dir = (char*)hdfsDirsToCheck()->getNext()) != NULL)
                {
                  str_sprintf(buf, "Dir Name: %s", dir);
                  space->allocateAndCopyToAlignedSpace(buf, str_len(buf), sizeof(short));
                }
            }

        }

    }

  if(flag & 0x00000001)
    {
      displayExpression(space,flag);
      displayChildren(space,flag);
    }
}

Int32 ComTdbHdfsScan::orderedQueueProtocol() const
{
  return 1;
}

///////////////////////////////////////////////////////////////
// ComTdbOrcAggr
///////////////////////////////////////////////////////////////

// Dummy constructor for "unpack" routines.
ComTdbOrcFastAggr::ComTdbOrcFastAggr():
 ComTdbHdfsScan()
{
  setNodeType(ComTdb::ex_ORC_AGGR);
  setEyeCatcher(eye_ORC_AGGR);
};

// Constructor
ComTdbOrcFastAggr::ComTdbOrcFastAggr(
                                     char * tableName,
                                     OrcAggrType type,
                                     Queue * hdfsFileInfoList,
                                     Queue * hdfsFileRangeBeginList,
                                     Queue * hdfsFileRangeNumList,
                                     ex_expr * proj_expr,
                                     Int64 projRowLen,
                                     const unsigned short projTuppIndex,
                                     const unsigned short returnedTuppIndex,
                                     ex_cri_desc * work_cri_desc,
                                     ex_cri_desc * given_cri_desc,
                                     ex_cri_desc * returned_cri_desc,
                                     queue_index down,
                                     queue_index up,
                                     Int32  numBuffers,
                                     UInt32  bufferSize
                                     )
  : ComTdbHdfsScan( 
                   tableName,
                   type,
                   NULL, NULL,
                   proj_expr,
                   NULL,
                   0, NULL, NULL, 0,
                   hdfsFileInfoList,
                   hdfsFileRangeBeginList,
                   hdfsFileRangeNumList,
                   0, 0, NULL, 0, 0, 0,
                   projRowLen, 
                   0, 0,
                   returnedTuppIndex,
                   0, 
                   projTuppIndex, 
                   0, 0,
                   work_cri_desc,
                   given_cri_desc,
                   returned_cri_desc,
                   down,
                   up,
                   0,
                   numBuffers,        // num_buffers - we use numInnerTuples_ instead.
                   bufferSize),       // buffer_size - we use numInnerTuples_ instead.
    type_(type)
{
  setNodeType(ComTdb::ex_ORC_AGGR);
  setEyeCatcher(eye_ORC_AGGR);
}

ComTdbOrcFastAggr::~ComTdbOrcFastAggr()
{
}

