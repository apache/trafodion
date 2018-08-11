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
  currLevel_ = 0;
  resultSize_ = 0;
  currQueue_ = new(getHeap()) Queue(getHeap())  ;
  thisQueue_ = NULL;
  prevQueue_ = NULL;
  tmpPrevQueue_ = NULL;
  currRootId_ = 0;
  connBatchSize_ = CONNECT_BY_DEFAULT_BATCH_SIZE;
  seedQueue_ = new(getHeap()) Queue(getHeap()) ;
  for( int i = 0; i< CONNECT_BY_MAX_LEVEL_SIZE; i++)
  {
    currArray[i] = NULL;
  }
}

ex_tcb_private_state *  ExExeUtilConnectbyTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilConnectbyTdbState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

short ExExeUtilConnectbyTcb::emitRow(ExpTupleDesc * tDesc, int level, int isleaf, int iscycle, connectByStackItem *vi )
{
  short retcode = 0, rc =0;
  char * ptr;
  Lng32   len;
  short nullind=0;
  short *pn = &nullind;
  short **ind = &pn;
  UInt32 vcActualLen = 0;

  for (UInt32 i = 2; i < tDesc->numAttrs() - 2 ; i++)  //bypass first two columns and ignore the last three system columns
  {
    cliInterface()->getPtrAndLen(i+1, ptr, len,ind);
    
    char * src = ptr;
    Attributes * attr = tDesc->getAttr(i-1);
    short srcType = 0;
    Lng32 srcLen;
    short valIsNull = 0;
    srcType = attr->getDatatype(); 
    srcLen = len;
    if (((char*)*ind)[0] == -1) valIsNull = -1;

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


  short srcType = REC_BIN32_UNSIGNED;
  Lng32 srcLen = 4;
  int src = isleaf;
  Attributes * attr = tDesc->getAttr(tDesc->numAttrs() - 2);
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

  src = iscycle;
  attr = tDesc->getAttr(tDesc->numAttrs() - 3);
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

  //apply the expression
  ex_expr::exp_return_type evalRetCode = ex_expr::EXPR_TRUE;
  if(exeUtilTdb().scanExpr_ )
  {
    //evalRetCode = evalScanExpr((char*)data_,  exeUtilTdb().tupleLen_, FALSE);
    workAtp_->getTupp(exeUtilTdb().workAtpIndex())
	.setDataPointer((char*)data_);
    evalRetCode =
	exeUtilTdb().scanExpr_->eval(workAtp_, NULL);
  }

  if (evalRetCode == ex_expr::EXPR_TRUE)
  {

  char pathBuf[CONNECT_BY_MAX_PATH_SIZE];
  memset(pathBuf, 0, CONNECT_BY_MAX_PATH_SIZE);
  char tmpbuf1[256];
  Lng32 pathlength = 0;
  Lng32 dlen = strlen(exeUtilTdb().delimiter_.data());
  Queue *pathq =  new(getHeap()) Queue(getHeap()) ;;
  if (exeUtilTdb().hasPath_ == TRUE) {
    pathq->insert(vi->pathItem);
    pathlength = vi->pathLen;
    for(int ii = level ; ii > 0; ii--)
    {
      Queue *tq = getCurrentQueue(ii-1);
      connectByStackItem * p = (connectByStackItem*)tq->get(vi->parentId - 1);
      if(p == NULL) abort();
      pathq->insert(p->pathItem);
      pathlength += p->pathLen + dlen;
      vi = p;
    }
    for(int pi = pathq->entries(); pi > 0; pi--)
    {
      if(pi-1 == 0)
        sprintf(tmpbuf1,"%s",(char*)(pathq->get(pi-1)));
      else
        sprintf(tmpbuf1,"%s%s",(char*)(pathq->get(pi-1)) ,exeUtilTdb().delimiter_.data());
      strcat(pathBuf,tmpbuf1);
    }
    NADELETE(pathq, Queue, getHeap());
  }

  attr = tDesc->getAttr(tDesc->numAttrs() - 1);

  if(pathlength > CONNECT_BY_MAX_PATH_SIZE) pathlength = CONNECT_BY_MAX_PATH_SIZE;

  if (
   ::convDoIt(pathBuf, pathlength, REC_BYTE_V_ASCII, 0, 0,
              &data_[attr->getOffset()], 
              attr->getLength(),
              attr->getDatatype(),0,0,
              (char*) &vcActualLen, sizeof(vcActualLen), NULL) != ex_expr::EXPR_OK)
  {
    ex_assert(
        0,
        "Error from ExStatsTcb::work::convDoIt.");
  }
  attr->setVarLength(vcActualLen,&data_[attr->getVCLenIndOffset()]);
  retcode = moveRowToUpQueue(data_, exeUtilTdb().tupleLen_, &rc, FALSE);

}
  return retcode;
}


short ExExeUtilConnectbyTcb::emitPrevRow(ExpTupleDesc * tDesc, int level, int isleaf, int iscycle,  Queue * pq, int idx)
{
  short retcode = 0, rc =0;
  connectByStackItem *vi = NULL;
  char * ptr;
  Lng32   len;
  short nullInd = 0;
  short *ind = &nullInd ;
  short **indadd = &ind;
  UInt32 vcActualLen = 0;
  //get the item
  Queue * thatone;
  int pos=0, doffset=0, lastpos = 0;
  for(int i1 = 0; i1< pq->numEntries(); i1++)
  {
    lastpos = pos;
    pos+= ((Queue *)pq->get(i1))->numEntries();
    if(idx < pos) 
    {
      thatone = (Queue *)pq->get(i1);
      doffset = idx - lastpos;
      break;
    }
  }
  if(level > 1)
    vi = (connectByStackItem *)currArray[level - 1]->get(doffset);
  OutputInfo * ti = (OutputInfo *)thatone->get(doffset);
  for (UInt32 i = 2; i < tDesc->numAttrs() - 2 ; i++) 
  {
    ti->get(i, ptr, len);
    char * src = ptr;
    Attributes * attr = tDesc->getAttr(i-1);
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


  short srcType = REC_BIN32_UNSIGNED;
  Lng32 srcLen = 4;
  int src = isleaf;
  Attributes * attr = tDesc->getAttr(tDesc->numAttrs() - 2);
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

  src = iscycle;
  attr = tDesc->getAttr(tDesc->numAttrs() - 3);
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

  //apply the expression
  ex_expr::exp_return_type evalRetCode = ex_expr::EXPR_TRUE;
  if(exeUtilTdb().scanExpr_ )
  {
    //evalRetCode = evalScanExpr((char*)data_,  exeUtilTdb().tupleLen_, FALSE);
    workAtp_->getTupp(exeUtilTdb().workAtpIndex())
	.setDataPointer((char*)data_);
    evalRetCode =
	exeUtilTdb().scanExpr_->eval(workAtp_, NULL);
  }

  if (evalRetCode == ex_expr::EXPR_TRUE)
{

  char pathBuf[CONNECT_BY_MAX_PATH_SIZE];
  memset(pathBuf, 0, CONNECT_BY_MAX_PATH_SIZE);
  char tmpbuf1[256];
  Lng32 pathlength = 0;
  Lng32 dlen = strlen(exeUtilTdb().delimiter_.data());
  Queue *pathq =  new(getHeap()) Queue(getHeap()) ;;
  if (exeUtilTdb().hasPath_ == TRUE) {
    ti->get(2, ptr, len);
    pathq->insert(ptr);
    pathlength = len;
    for(int ii = level - 1 ; ii > 0; ii--)
    {
      Queue *tq = getCurrentQueue(ii-1);
      connectByStackItem * p = (connectByStackItem*)tq->get(vi->parentId - 1);
      if(p == NULL) abort();
      pathq->insert(p->pathItem);
      pathlength += p->pathLen + dlen;
      vi = p;
    }
    for(int pi = pathq->entries(); pi > 0; pi--)
    {
      if(pi-1 == 0)
        sprintf(tmpbuf1,"%s",(char*)(pathq->get(pi-1)));
      else
        sprintf(tmpbuf1,"%s%s",(char*)(pathq->get(pi-1)) ,exeUtilTdb().delimiter_.data());
      strcat(pathBuf,tmpbuf1);
    }
    NADELETE(pathq, Queue, getHeap());
  }

  attr = tDesc->getAttr(tDesc->numAttrs() - 1);

  if(pathlength > CONNECT_BY_MAX_PATH_SIZE) pathlength = CONNECT_BY_MAX_PATH_SIZE;

  if (
   ::convDoIt(pathBuf, pathlength, REC_BYTE_V_ASCII, 0, 0,
              &data_[attr->getOffset()], 
              attr->getLength(),
              attr->getDatatype(),0,0,
              (char*) &vcActualLen, sizeof(vcActualLen), NULL) != ex_expr::EXPR_OK)
  {
    ex_assert(
        0,
        "Error from ExStatsTcb::work::convDoIt.");
  }
  attr->setVarLength(vcActualLen,&data_[attr->getVCLenIndOffset()]);
  retcode = moveRowToUpQueue(data_, exeUtilTdb().tupleLen_, &rc, FALSE);

}
  return retcode;
}

void releaseCurrentQueue(Queue * q, CollHeap * h)
{
  for(int i = 0; i < q->numEntries(); i++)
  {
     connectByStackItem *entry = (connectByStackItem *)q->get(i);
     NADELETEBASIC(entry->seedValue,h);
     NADELETEBASIC(entry->pathItem,h);
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

short ExExeUtilConnectbyTcb::checkDuplicate(connectByStackItem *it, int len, int level)
{
  short rc = 0;
  for(int i = 0; i < level -1; i++)
  {
    Queue * q = currArray[i];
    if(q == NULL) continue;
    rc = haveDupSeed( q, it, len, level);
    if( rc == -1 || rc == 1) return rc;
  }
  return rc;
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
  char q1[CONNECT_BY_MAX_SQL_TEXT_SIZE]; //TODO: the max len of supported query len
  void* uppderid = NULL;
  char * ptr, *ptr1;
  Lng32   len, len1;
  short rc;
  NAString nq11, nq21;	    
  memset(q1, 0, sizeof(q1));
  int matchRowNum = 0;

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
  
  if(exeUtilTdb().hasStartWith_ == TRUE)
    {
      sprintf(q1,"SELECT %s , cast (%s as VARCHAR(3000) ), * , cast( 0 as INT) FROM %s WHERE %s ;" ,(exeUtilTdb().parentColName_).data() , (exeUtilTdb().pathColName_).data(), (exeUtilTdb().connTableName_).data() ,(exeUtilTdb().startWithExprString_).data() );
      nq11 = q1;
    }
  else
    {
      sprintf(q1,"SELECT %s ,  cast (%s as VARCHAR(3000) ) , * , cast(0 as INT)  FROM %s WHERE %s is null ;" ,(exeUtilTdb().parentColName_).data(), (exeUtilTdb().pathColName_).data(), (exeUtilTdb().connTableName_).data(), (exeUtilTdb().childColName_).data());
      nq11 = q1;
    }   

  ExpTupleDesc * tDesc = exeUtilTdb().workCriDesc_->getTupleDescriptor( exeUtilTdb().sourceDataTuppIndex_);

  Lng32 fsDatatype = 0;
  Lng32 length = 0;
  Lng32 indOffset = 0;
  Lng32 varOffset = 0;

  if (exeUtilTdb().hasPath_ == TRUE || exeUtilTdb().hasIsLeaf_ == TRUE) 
  {
    connBatchSize_ = 1;
  }

  if(exeUtilTdb().hasIsLeaf_ == TRUE && prevQueue_ == NULL)
    prevQueue_ = new(getHeap()) Queue(getHeap()) ;
  
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
         
         if( exeUtilTdb().hasIsLeaf_ == TRUE)
         {
            Queue * rootRow = new(getHeap()) Queue(getHeap()) ;  
            rc = cliInterface()->fetchAllRows(rootRow, nq11.data(), 0, FALSE, FALSE, TRUE);
            if (rc < 0)
            {
              NADELETE(rootRow, Queue, getHeap());
              return WORK_BAD_ERROR;
            }
            //populate the seedQueue and currArray
            rootRow->position();

            for(int i1 = 0; i1 < rootRow->numEntries(); i1 ++)
            {
              OutputInfo *vi = (OutputInfo*)rootRow->getNext();
              vi->get(0, ptr, len, fsDatatype, NULL, NULL);
              connectByStackItem *it = new connectByStackItem();
              
              char *tmp = new(getHeap()) char[len];
              memcpy(tmp,ptr,len);
              it->seedValue = tmp; 
              it->level = currLevel_;
              it->type = fsDatatype;
              it->len = len;
              it->parentId = -1;
              if (exeUtilTdb().hasPath_ == TRUE) 
              {
                 vi->get(1, ptr1, len1);
                 char *tmp1 = new(getHeap()) char[len1+1];  
                 memset(tmp1,0, len1+1);
                 memcpy(tmp1,ptr1,len1);
                 it->pathLen = len1;
                 it->pathItem = tmp1;
               }
               if(checkDuplicate( it, len, currLevel_) == 0) {
                 matchRowNum++;
                 Queue* cq = new(getHeap()) Queue(getHeap()) ;
                 rootItem * ri= new(getHeap()) rootItem();
                 ri->rootId = rootId;
                 rootId++;
                 cq->insert(it);
                 ri->qptr = cq;
                 seedQueue_->insert(ri);   
               }

               //populate prevQueue
               prevQueue_->insert(rootRow);
            } //for(int i1 = 0; i1 < rootRow->numEntries(); i1 ++)
         }
         else
         {
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

	 
		   char *tmp = new(getHeap()) char[len];
		   memcpy(tmp,ptr,len);
		   it->seedValue = tmp; 
		   it->level = currLevel_;
		   it->type = fsDatatype;
		   it->len = len;
		   it->parentId = -1;
		   if (exeUtilTdb().hasPath_ == TRUE) 
		   {
		     cliInterface()->getPtrAndLen(2, ptr1, len1);
		     char *tmp1 = new(getHeap()) char[len1+1];  
                     memset(tmp1,0,len1+1);
		     memcpy(tmp1,ptr1,len1);
		     it->pathLen = len1;
		     it->pathItem = tmp1;
		   }     
		   if(checkDuplicate( it, len, currLevel_) == 0) {
		     emitRow(tDesc, currLevel_, 0, 0, it); 
		     matchRowNum++;
                     Queue* cq = new(getHeap()) Queue(getHeap()) ;
                     rootItem * ri= new(getHeap()) rootItem();
                     cq->insert(it);
                     ri->rootId = rootId;
                     rootId++;
                     ri->qptr = cq;
                     seedQueue_->insert(ri); //TO BE REMOVED
		   }

		 }
           cliInterface()->fetchRowsEpilogue(0, FALSE);
         }
         currQueue_ = getCurrQueue(currRootId_, seedQueue_);
         currArray[0] = currQueue_;  

         if( matchRowNum > exeUtilTdb().maxSize_ )
         {
           ComDiagsArea * diags = getDiagsArea();
           if(diags == NULL)
           {
              setDiagsArea(ComDiagsArea::allocate(getHeap()));
              diags = getDiagsArea();              
           }
           *diags << DgSqlCode(-8039);

           step_ = ERROR_;
         }
         else
           step_ = NEXT_LEVEL_;
        }
       break;
      case DO_CONNECT_BY_:
       {
       currQueue_ = getCurrentQueue(currLevel_ - 1);
       thisQueue_ = getCurrentQueue(currLevel_);
       if(exeUtilTdb().hasIsLeaf_ == TRUE) 
         tmpPrevQueue_ = new(getHeap()) Queue(getHeap()) ;
       Lng32 seedNum = currQueue_->numEntries();
       Int8 loopDetected = 0;
       
       currQueue_->position();
       resultSize_ = 0;

       for(int i=0; i< seedNum; )  {
         
         sprintf(q1,"SELECT %s ,cast (%s as VARCHAR(3000) ) , *, cast( %d as INT) FROM %s WHERE " ,
                    (exeUtilTdb().parentColName_).data(), (exeUtilTdb().pathColName_).data(), currLevel_,(exeUtilTdb().connTableName_).data() );
         nq21 = q1;
         nq21.append((exeUtilTdb().childColName_).data()); 
         nq21.append(" in ( ");
         Lng32 sybnum = 0;

         for(int batchIdx = 0; batchIdx < connBatchSize_ && i < seedNum; )
         {
           connectByStackItem * vi = (connectByStackItem*)currQueue_->get(i);
           Lng32 tmpLevel = vi->level; 
           i++;
           if( tmpLevel == currLevel_ - 1) {
             sybnum++;
             uppderid = ((connectByStackItem*)vi)->seedValue;
             batchIdx++;
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
             if( i == seedNum || batchIdx == connBatchSize_) continue;
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
             if(exeUtilTdb().hasIsLeaf_ == TRUE)
             {
		    Queue * allrows = new(getHeap()) Queue(getHeap()) ;  //TODO DELETE
		    rc = cliInterface()->fetchAllRows(allrows, nq21.data(), 0, FALSE, FALSE, TRUE);
		    if (rc < 0)
		    {
		      NADELETE(allrows, Queue, getHeap());
		      return WORK_BAD_ERROR;
		    }

                    allrows->position();
                    if(allrows->numEntries() == 0) //no child
                    {
                      //emit parent
                      emitPrevRow(tDesc, currLevel_, 1, 0,prevQueue_, i-1);
                    }
                    else
                    {
                      //emit parent
                      emitPrevRow(tDesc, currLevel_, 0, 0,prevQueue_, i-1);
                    }
                    tmpPrevQueue_->insert(allrows); //TODO: delete it

		    for(int i1 = 0; i1 < allrows->numEntries(); i1 ++)
		    {
                      resultSize_++;
		      OutputInfo *vi = (OutputInfo*)allrows->getNext();
                      vi->get(0, ptr, len);
		      connectByStackItem *it = new connectByStackItem();
		      char *tmp = new(getHeap()) char[len];
	   	      memcpy(tmp,ptr,len);
		      it->seedValue = tmp; 
  		      it->level = currLevel_ ;
		      it->type = fsDatatype;
		      it->len = len;
		      it->parentId = i;
		      if (exeUtilTdb().hasPath_ == TRUE) 
		      {
		        vi->get(1, ptr1, len1);
		        char *tmp1 = new(getHeap()) char[len1+1];  
                        memset(tmp1,0,len1+1);
		        memcpy(tmp1,ptr1,len1);
		        it->pathLen = len1;
		        it->pathItem = tmp1;
		      }     
		      short rc1 = checkDuplicate(it, len, currLevel_) ; //loop detection

		      if(rc1 == 0) {
		        thisQueue_->insert(it);
                        matchRowNum++;
		      }//if(rc1== 0)
		      else if(rc1 == 1) 
		      {
		        loopDetected = 1;
		        if(exeUtilTdb().noCycle_ == FALSE) {
		          ComDiagsArea * diags = getDiagsArea();
		          if(diags == NULL)
		          {
		          setDiagsArea(ComDiagsArea::allocate(getHeap()));
		          diags = getDiagsArea();              
		          }
		          *diags << DgSqlCode(-8037);
		          step_ = ERROR_;
		         }
  		     }
                    }
             }
             else
             {
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
		   resultSize_++;
		   cliInterface()->getPtrAndLen(1, ptr, len);
		   connectByStackItem *it = new connectByStackItem();
		   char *tmp = new(getHeap()) char[len];
		   memcpy(tmp,ptr,len);
		   it->seedValue = tmp; 
		   it->level = currLevel_ ;
		   it->type = fsDatatype;
		   it->len = len;
		   it->parentId = i;
		   if (exeUtilTdb().hasPath_ == TRUE) 
		   {
		     cliInterface()->getPtrAndLen(2, ptr1, len1);
		     char *tmp1 = new(getHeap()) char[len1+1];  
		     memset(tmp1,0,len1+1);
		     memcpy(tmp1,ptr1,len1);
		     it->pathLen = len1;
		     it->pathItem = tmp1;
		   }     
		   short rc1 = checkDuplicate(it, len, currLevel_) ; //loop detection

		   if(rc1 == 0) {
		     thisQueue_->insert(it);
		     emitRow(tDesc, currLevel_, 0, 0,it); 
		     matchRowNum++;
		   }//if(rc1== 0)
		   else if(rc1 == 1) 
		   {
		      loopDetected = 1;
		      if(exeUtilTdb().noCycle_ == FALSE) {
		        ComDiagsArea * diags = getDiagsArea();
		        if(diags == NULL)
		        {
		          setDiagsArea(ComDiagsArea::allocate(getHeap()));
		          diags = getDiagsArea();              
		        }
		        *diags << DgSqlCode(-8037);
		        step_ = ERROR_;
		       }
		      else
		      {
		         emitRow(tDesc, currLevel_, 0, 1, it); 
		      }
		   }
		  }// while ((rc >= 0) 
		  cliInterface()->fetchRowsEpilogue(0, FALSE);
          }

        }//for(int i=0; i< seedNum; i++)
         //this level is done
         if(exeUtilTdb().hasIsLeaf_ == TRUE)
         {
            //release prevQueue_
            for(int i = 0 ; i < prevQueue_->numEntries(); i ++)
              NADELETE((Queue*)prevQueue_->get(i), Queue, getHeap());
            NADELETE(prevQueue_, Queue, getHeap());
            prevQueue_ = tmpPrevQueue_;
         }

         if( loopDetected == 1)
         {
             if( exeUtilTdb().noCycle_ == TRUE)
             {
               step_ = NEXT_ROOT_;
             }
             else
               step_ = ERROR_;
             break;
         }
	 if(resultSize_ == 0)
             step_ = NEXT_ROOT_;
         else
             step_ = NEXT_LEVEL_;
       }
       break;
       case NEXT_LEVEL_:
         {
           Queue* currQueue = new(getHeap()) Queue(getHeap()) ;
           currLevel_++;
           currArray[currLevel_] = currQueue;

           if( currLevel_ > exeUtilTdb().maxDeep_ )
           {
             ComDiagsArea * diags = getDiagsArea();
             if(diags == NULL)
             {
               setDiagsArea(ComDiagsArea::allocate(getHeap()));
               diags = getDiagsArea();              
             }
             *diags << DgSqlCode(-8038);

             step_ = ERROR_;
           }
           else
             step_ = DO_CONNECT_BY_;
         }
       break;
       case NEXT_ROOT_:
         {
            currRootId_++;
            currLevel_ = 1;
            currQueue_ = getCurrQueue(currRootId_, seedQueue_);
            //clear currArray
            for(int i =0; i< CONNECT_BY_MAX_LEVEL_SIZE; i++)
            {
               if(currArray[i] != NULL) //comehere
                 NADELETE(currArray[i], Queue, getHeap());
            }
            if(currQueue_ == NULL) 
              step_ = DONE_;
            else
              step_ = DO_CONNECT_BY_;
         }
       break;
       case ERROR_:
	  {

	    if (qparent_.up->isFull())
	      return WORK_OK;
            if (handleError())
              return WORK_OK;
   
	    step_ = DONE_;
	  }
	break;
       case DONE_:
         if (qparent_.up->isFull())
           return WORK_OK;

         retcode = handleDone();
	 if (retcode == 1)
	      return WORK_OK;

         NADELETE(seedQueue_, Queue, getHeap());
	 step_ = INITIAL_;
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

