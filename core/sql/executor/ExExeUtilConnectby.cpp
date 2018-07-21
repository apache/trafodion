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
 *****************************************************************************
 *
 * File:         ExExeUtilConnectby.cpp
 * Description:  
 *               
 *               
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include <iostream>
using std::cerr;
using std::endl;

#include <fstream>
using std::ofstream;

#include <stdio.h>

#include "ComCextdecs.h"
#include  "cli_stdh.h"
#include  "ex_stdh.h"
#include  "sql_id.h"
#include  "ex_transaction.h"
#include  "ComTdb.h"
#include  "ex_tcb.h"
#include  "ComSqlId.h"

#include  "ExExeUtil.h"
#include  "ex_exe_stmt_globals.h"
#include  "exp_expr.h"
#include  "exp_clause_derived.h"
#include  "ComRtUtils.h"
#include  "ExStats.h"
#include  "ExpLOB.h"
#include "ExpLOBenums.h"
#include  "ExpLOBinterface.h"
#include  "ExpLOBexternal.h"
#include  "str.h"
#include "ExpHbaseInterface.h"
#include "ExHbaseAccess.h"
#include "ExpErrorEnums.h"
#include "HdfsClient_JNI.h"


class connectByStackItem
{
public:
  connectByStackItem() {}
  ~connectByStackItem() {}
  char * seedValue;
  int len;
  int level;
  int type;
};

class rootItem
{
public:
  rootItem() {}
  ~rootItem() {}
  int rootId;
  Queue * qptr;
};

ExExeUtilConnectbyTcb::ExExeUtilConnectbyTcb(
    const ComTdbExeUtilConnectby & exe_util_tdb,
    ex_globals * glob)
: ExExeUtilTcb( exe_util_tdb, NULL, glob),
  step_(INITIAL_)
{
  Space * space = (glob ? glob->getSpace() : 0);
  CollHeap * heap = (glob ? glob->getDefaultHeap() : 0);
  qparent_.down->allocatePstate(this);
  pool_->get_free_tuple(tuppData_, exe_util_tdb.tupleLen_);
  data_ = tuppData_.getDataPointer();
  //pool_->get_free_tuple(workAtp_->getTupp(2), 0);
}

ex_tcb_private_state *  ExExeUtilConnectbyTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilConnectbyTdbState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

short ExExeUtilConnectbyTcb::emitRows(Queue *q, ExpTupleDesc * tDesc)
{
  short retcode = 0, rc =0;
  char * ptr;
  Lng32   len;

  q->position();
#if 1         
    for (Lng32 idx = 0; idx < q->numEntries(); idx++)
    {
      OutputInfo * vi = (OutputInfo*)q->getNext();

      if (vi == NULL) break;
	  for (UInt32 i = 1; i < tDesc->numAttrs() ; i++)
	  {
	   // OutputInfo * vi = (OutputInfo*)q->getNext();

	    char * src = (char*)vi->get(i);
	    Attributes * attr = tDesc->getAttr(i);
	    short srcType;
	    Lng32 srcLen;
	    short valIsNull = 0;
            if(attr->getDatatype() < REC_MAX_CHARACTER)
              srcType = REC_BYTE_F_ASCII;
            else
              srcType = attr->getDatatype(); 
	    srcLen = attr->getLength();
            if(src == NULL) valIsNull = 1;
#if 1
	    if (attr->getNullFlag())
	      {
		// target is nullable
		if (attr->getNullIndicatorLength() == 2)
		  {
		    // set the 2 byte NULL indicator to -1
		    *(short *) (&data_[attr->getNullIndOffset()]) =
		      valIsNull;
		  }
		else
		  {
		    ex_assert(attr->getNullIndicatorLength() == 4,
			      "NULL indicator must be 2 or 4 bytes");
		    *(Lng32 *) (&data_[attr->getNullIndOffset()]) =
		      valIsNull;
		  }
	      }
	    else
	      ex_assert(!valIsNull,
			"NULL source for NOT NULL stats column");
#endif
	    if (!valIsNull)
	      {

#if 1
		if (
		    ::convDoIt(src, srcLen, srcType, 0, 0,
			       &data_[attr->getOffset()], 
			       attr->getLength(),
			       attr->getDatatype(),0,0,
			       0, 0, NULL) != ex_expr::EXPR_OK)
		  {
		    ex_assert(
			 0,
			 "Error from ExStatsTcb::work::convDoIt.");
		  }
#endif
	      }
	  }
#endif

      retcode = moveRowToUpQueue(data_, exeUtilTdb().tupleLen_, &rc, FALSE);
    }

  return retcode;
}

short ExExeUtilConnectbyTcb::emitOneRow(ExpTupleDesc * tDesc, int level)
{
  short retcode = 0, rc = 0;
  char * ptr;
  Lng32   len;  
  ex_expr::exp_return_type evalRetCode = ex_expr::EXPR_OK;
  UInt32 rowLen = exeUtilTdb().outputRowlen_; //TODO
  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();

  cliInterface()->getPtrAndLen(1, ptr, len); //start from second column
  //setup ATP
  workAtp_->getTupp(exeUtilTdb().sourceDataTuppIndex_)
    .setDataPointer(data_);
  workAtp_->getTupp(2) 
    .setDataPointer(ptr);

  evalRetCode = (exeUtilTdb().outputExpr())->eval(pentry_down->getAtp(), workAtp_, NULL, 
                                exeUtilTdb().tupleLen_, &rowLen);
  retcode = moveRowToUpQueue(data_, exeUtilTdb().tupleLen_, &rc, FALSE);
  return retcode;
}

short ExExeUtilConnectbyTcb::emitRow(ExpTupleDesc * tDesc, int level)
{
  short retcode = 0, rc =0;
  char * ptr;
  Lng32   len;
  short nullInd = 0;
  short *ind ;
  ind = &nullInd;

  for (UInt32 i = 1; i < tDesc->numAttrs() ; i++) 
  {
    cliInterface()->getPtrAndLen(i+1, ptr, len,&ind);
    char * src = ptr;
    Attributes * attr = tDesc->getAttr(i);
    short srcType = 0;
    Lng32 srcLen;
    short valIsNull = 0;
    srcType = attr->getDatatype(); 
    srcLen = len;
    if (len == 0  )  valIsNull = -1;

    if (attr->getNullFlag())
      {
	// target is nullable
	if (attr->getNullIndicatorLength() == 2)
	  {
	    // set the 2 byte NULL indicator to -1
	    *(short *) (&data_[attr->getNullIndOffset()]) =
	      valIsNull;
	  }
	else
	  {
	    ex_assert(attr->getNullIndicatorLength() == 4,
		      "NULL indicator must be 2 or 4 bytes");
	    *(Lng32 *) (&data_[attr->getNullIndOffset()]) =
	      valIsNull;
	  }
      }
    else
      ex_assert(!valIsNull,
		"NULL source for NOT NULL stats column");
    UInt32 vcActualLen = 0;

    if (!valIsNull)
      { 
        if (attr->getVCIndicatorLength() > 0)
        {
	  ::convDoIt(src, srcLen, srcType, 0, 0,
		       &data_[attr->getOffset()], 
		       attr->getLength(),
		       attr->getDatatype(),0,0,
		       (char*) &vcActualLen, sizeof(vcActualLen), NULL);
          attr->setVarLength(vcActualLen,&data_[attr->getVCLenIndOffset()]);
        }
        else
	  ::convDoIt(src, srcLen, srcType, 0, 0,
		       &data_[attr->getOffset()], 
		       attr->getLength(),
		       attr->getDatatype(),0,0,
		       NULL, 0, NULL);
	
      }  
  }
#if 0
  short srcType = REC_BIN32_UNSIGNED;
  Lng32 srcLen = 4;
  int src = level;
  Attributes * attr = tDesc->getAttr(tDesc->numAttrs() - 1);
  if (
   ::convDoIt((char*)&src, srcLen, srcType, 0, 0,
	       &data_[attr->getOffset()], 
	       attr->getLength(),
	       attr->getDatatype(),0,0,
	       0, 0, NULL) != ex_expr::EXPR_OK)
  {
    ex_assert(
	 0,
	 "Error from ExStatsTcb::work::convDoIt.");
  }
#endif
  retcode = moveRowToUpQueue(data_, exeUtilTdb().tupleLen_, &rc, FALSE);
  return retcode;
}

void releaseCurrentQueue(Queue * q, CollHeap * h)
{
  for(int i = 0; i < q->numEntries(); i++)
  {
     connectByStackItem *entry = (connectByStackItem *)q->get(i);
     NADELETEBASIC(entry->seedValue,h);
  }
}

short haveDupSeed(Queue * q , connectByStackItem *it, int len, int level)
{
  
  for(int i = 0; i < q->numEntries(); i++)
  {
     connectByStackItem *entry = (connectByStackItem *)q->get(i);
     if(memcmp(entry->seedValue,it->seedValue, len) == 0)
     {
       if(entry->level < level)
          return 1;
       else
          return -1;
     }
  }
  return 0;
}

Queue * getCurrQueue(int id, Queue *q)
{
  for(int i = 0; i < q->numEntries(); i++)
  {
     rootItem * ri = (rootItem*) q->get(i);
     if(ri->rootId == id) return ri->qptr;
  }
  return NULL;
}
 
short ExExeUtilConnectbyTcb::work()
{
  short retcode = 0; 
  char q1[2048]; //TODO: the max len of supported query len
  void* uppderid = NULL;
  char * ptr;
  Lng32   len;
  short rc;
  NAString nq11, nq21;	    
  memset(q1, 0, sizeof(q1));
  int matchRowNum = 0;

  int currLevel = 1;
  int resultSize = 0;
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ex_queue_entry * up_entry = qparent_.up->getTailEntry();
  ex_tcb_private_state * pstate = pentry_down->pstate;
  char * stmtStr = exeUtilTdb().oriQuery_;
  if(stmtStr && exeUtilTdb().hasStartWith_ == TRUE)
    {
      NAString nq1=stmtStr;
      UInt32 pos = nq1.index(" from ", 0, NAString::ignoreCase);
      sprintf(q1,"SELECT %s , * , 0 FROM %s WHERE %s ;" ,(exeUtilTdb().parentColName_).data() , (exeUtilTdb().connTableName_).data() ,(exeUtilTdb().startWithExprString_).data() );
      nq11 = q1;
    }
  else
    {
      NAString nq1=stmtStr;
      UInt32 pos = nq1.index(" from ", 0, NAString::ignoreCase);
      sprintf(q1,"SELECT %s , * , 0 FROM %s WHERE %s is null ;" ,(exeUtilTdb().parentColName_).data(), (exeUtilTdb().connTableName_).data(), (exeUtilTdb().childColName_).data());
      nq11 = q1;
    }   

  ExpTupleDesc * tDesc = exeUtilTdb().workCriDesc_->getTupleDescriptor( exeUtilTdb().sourceDataTuppIndex_);

  Queue * seedQueue = new(getHeap()) Queue(getHeap()) ;
  Queue * currQueue = NULL ;
  Queue * tmpQueue = new(getHeap()) Queue(getHeap());;
  tupp p;
  Lng32 fsDatatype = 0;
  Lng32 length = 0;
  Lng32 indOffset = 0;
  Lng32 varOffset = 0;
  int currRootId = 0;
  
  while (1)
    {
     ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
     switch (step_) 
     {
       case INITIAL_:

         step_ = EVAL_START_WITH_;
       break;
       case EVAL_START_WITH_:
        {
           short rc = 0;
          int rootId = 0;

         //get the stmt
         retcode = cliInterface()->fetchRowsPrologue(nq11.data(), FALSE, FALSE);
         while ((rc >= 0) && 
	   (rc != 100))
         {
           rc = cliInterface()->fetch();
           if (rc < 0)
	   {
	     cliInterface()->fetchRowsEpilogue(0, TRUE);
             return rc;
            }

           if (rc == 100)
	    continue;
 
           connectByStackItem *it = new connectByStackItem();
           cliInterface()->getPtrAndLen(1, ptr, len);
           cliInterface()->getAttributes(1, FALSE,
			fsDatatype, length,
			&indOffset, &varOffset);
           currQueue = new(getHeap()) Queue(getHeap()) ;
           rootItem * ri= new(getHeap()) rootItem();
           ri->rootId = rootId;
           rootId++;
           ri->qptr = currQueue;
           seedQueue->insert(ri);

           
           char *tmp = new(getHeap()) char[len]; 
           memcpy(tmp,ptr,len);
           it->seedValue = tmp; 
           it->level = currLevel;
           it->type = fsDatatype;
           it->len = len;
           if(haveDupSeed(currQueue, it, len, currLevel) == 0) {
             emitRow(tDesc, currLevel); matchRowNum++;
             currQueue->insert(it);
           }
         }
         cliInterface()->fetchRowsEpilogue(0, FALSE);
         step_ = DO_CONNECT_BY_;
        }
       break;
      case DO_CONNECT_BY_:
       {
       int seedNum = currQueue->numEntries();
       int loopDetected = 0;
       
       currQueue->position();
       resultSize = 0;

       for(int i=0; i< seedNum; i++)  {
         
         sprintf(q1,"SELECT %s , *, %d FROM %s WHERE " ,(exeUtilTdb().parentColName_).data(), currLevel,(exeUtilTdb().connTableName_).data());
         nq21 = q1;
         nq21.append((exeUtilTdb().childColName_).data()); 
         nq21.append(" in ( ");
         int sybnum = 0;

         for(int batchIdx = 0; batchIdx < 10 && i < seedNum; batchIdx ++)
         {
           connectByStackItem * vi = (connectByStackItem*)currQueue->get(i);
           int tmpLevel = vi->level; 
           i++;
           if( tmpLevel == currLevel) {
             sybnum++;
             uppderid = ((connectByStackItem*)vi)->seedValue;
             char tmpbuf[128];
             char tmpbuf1[128];
             memset(tmpbuf,0,128);
             memset(tmpbuf1,0,128);
             if(vi->type >= REC_MIN_NUMERIC && vi->type <= REC_MAX_NUMERIC)
                 sprintf(tmpbuf, "%d ", *(int*)uppderid);
             else
             {
                 strcpy(tmpbuf, " '");	
                 strncpy(tmpbuf1,(char*)uppderid, vi->len);
                 strcat(tmpbuf,tmpbuf1);
                 strcat(tmpbuf,"'");
             }

             nq21.append(tmpbuf);
             if( i == seedNum || batchIdx ==10) continue;
             else
               nq21.append(" , ");
           } //if( tmpLevel == currLevel)

         }//for(int batchIdx = 0; batchIdx < 10 && i < seedNum; batchIdx ++)

         nq21.append(" );");

         if(sybnum == 0 ) //end
         {
           step_ = NEXT_LEVEL_;
            break;
         }
         retcode = cliInterface()->fetchRowsPrologue(nq21.data(), FALSE, FALSE);

         rc = 0;
         while ((rc >= 0) && 
	    (rc != 100))
         {
           rc = cliInterface()->fetch();
           if (rc < 0)
	   {
             cliInterface()->fetchRowsEpilogue(0, TRUE);
             return rc;
           }
           if (rc == 100)
	       continue;
           resultSize++;
           cliInterface()->getPtrAndLen(1, ptr, len);
           connectByStackItem *it = new connectByStackItem();
           char *tmp = new(getHeap()) char[len]; 
           memcpy(tmp,ptr,len);
           it->seedValue = tmp; 
           it->level = currLevel + 1;
           it->type = fsDatatype;
           it->len = len;
           short rc1 = haveDupSeed(currQueue, it, len, currLevel) ; //loop detection
           if(rc1 == 1) 
           {
               loopDetected = 1;
               ComDiagsArea * diags = getDiagsArea();
              if(diags == NULL)
              {
                setDiagsArea(ComDiagsArea::allocate(getHeap()));
                diags = getDiagsArea();              
              }
              *diags << DgSqlCode(-8041);
               step_ = ERROR_;
               break;
           }

           if(rc1 == 0) {
             emitRow(tDesc, currLevel+1); matchRowNum++;
             tmpQueue->insert(it);
           }//if(haveDupSeed(currQueue, it, len, currLevel) == 0)
          }// while ((rc >= 0) 
          cliInterface()->fetchRowsEpilogue(0, FALSE);
        }//for(int i=0; i< seedNum; i++)
        //insert tmpQueue into currQueue
        for(int i = 0; i < tmpQueue->numEntries(); i++)
          currQueue->insert(tmpQueue->get(i));
         NADELETE(tmpQueue, Queue, getHeap());
         tmpQueue = new(getHeap()) Queue(getHeap());

         if( loopDetected == 1)
         {
             step_ = ERROR_;
             break;
         }
	 if(resultSize == 0)
             step_ = NEXT_ROOT_;
         else
             step_ = NEXT_LEVEL_;
       }
       break;
       case NEXT_LEVEL_:
         {
            currLevel++;
            step_ = DO_CONNECT_BY_;
         }
       break;
       case NEXT_ROOT_:
         {
            currRootId++;
            currLevel =  1;
            currQueue = getCurrQueue(currRootId, seedQueue);
            if(currQueue == NULL) step_ = DONE_;
            else
              step_ = DO_CONNECT_BY_;
         }
       break;
       case ERROR_:
	  {

	    //if (qparent_.up->isFull())
	      //return WORK_OK;
            if (handleError())
              return WORK_OK;

	    //getDiagsArea()->clear();

	    step_ = DONE_;
#if 0
	    // Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	    
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    
	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_SQLERROR;
            ComDiagsArea * diagsArea = NULL;
            ExRaiseSqlError(getHeap(), &diagsArea, 
                                    (ExeErrorCode)(8041));
            up_entry->setDiagsArea(diagsArea);
	    //ComDiagsArea *diagsArea = up_entry->getDiagsArea();
	    
	    if (diagsArea == NULL)
	      diagsArea = 
		ComDiagsArea::allocate(this->getGlobals()->getDefaultHeap());
            else
              diagsArea->incrRefCount (); // setDiagsArea call below will decr ref count

	    if (getDiagsArea())
	      diagsArea->mergeAfter(*getDiagsArea());

	    // insert into parent
	    qparent_.up->insert();
#endif	    
	    step_ = DONE_;
	  }
	break;
       case DONE_:
         if (qparent_.up->isFull())
           return WORK_OK;

         step_ = INITIAL_;
	 // Return EOF.
	 up_entry = qparent_.up->getTailEntry();
	 up_entry->upState.parentIndex = 
	   pentry_down->downState.parentIndex;
	    
	 up_entry->upState.setMatchNo(matchRowNum);
	 up_entry->upState.status = ex_queue::Q_NO_DATA;
	    
	 // insert into parent
	 qparent_.up->insert();
	  
	 qparent_.down->removeHead();

         //release the currentQueue
         for(int i = 0; i< currRootId ; i++)
         {
           currQueue = getCurrQueue( i, seedQueue);
           releaseCurrentQueue(currQueue, getHeap());
           NADELETE(currQueue, Queue, getHeap());
         }
         NADELETE(seedQueue, Queue, getHeap());
         return WORK_OK;
       break;
     }
    }

  return WORK_OK;
}

ex_tcb * ExExeUtilConnectbyTdb::build(ex_globals * glob )
{
  ExExeUtilConnectbyTcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilConnectbyTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

ExExeUtilConnectbyTdbState::ExExeUtilConnectbyTdbState()
{

}
ExExeUtilConnectbyTdbState::~ExExeUtilConnectbyTdbState()
{}

