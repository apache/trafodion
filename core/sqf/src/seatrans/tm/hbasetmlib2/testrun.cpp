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

#include<iostream>
#include <sys/types.h>
#include <sys/time.h>

#include "hbasetm.h"

// To avoid pulling in other .so
int tm_log_write(int pv_event_type, posix_sqlog_severity_t pv_severity, char *pp_string) {return 0;}

// Define here because we don't have the TM.
timeval gv_startTime;

using namespace std;


int main () {

   short lv_retcode = 0;
   long  lv_txid = 20500;
   short lv_dtmid = 0;
   char *errStr = NULL;
   int   errStrLen = 0;
   long ctrlPtNum = 0;
   CHbaseTM *lp_myHbaseTM = 0;

   lp_myHbaseTM = new CHbaseTM();
   printf("Created the CHbaseTM object \n");

   lp_myHbaseTM->initConnection(lv_dtmid);
   printf("After lp_myHbaseTM->initConnection \n");

   lp_myHbaseTM->beginTransaction(&lv_txid);
   printf("After lp_myHbaseTM->beginTransaction, transactionId = %ld \n", lv_txid);

   lv_retcode = lp_myHbaseTM->abortTransaction(lv_txid);
   printf("After lp_myHbaseTM->abortTransaction(transactionId = %ld), retcode = %d \n", lv_txid, lv_retcode);

   lv_txid++;
   lp_myHbaseTM->beginTransaction(&lv_txid);
   printf("After lp_myHbaseTM->beginTransaction, transactionId = %ld \n", lv_txid);

   lv_retcode = lp_myHbaseTM->prepareCommit(lv_txid, errStr, errStrLen);
   printf("After lp_myHbaseTM->prepareCommit(transactionId = %ld), retcode = %d \n", lv_txid, lv_retcode);

   lv_retcode = lp_myHbaseTM->participatingRegions(lv_txid);
   printf("After lp_myHbaseTM->participatingRegions(transactionId = %ld), retcode = %d \n", lv_txid, lv_retcode);

   lv_retcode = lp_myHbaseTM->doCommit(lv_txid);
   printf("After lp_myHbaseTM->doCommit(transactionId = %ld), retcode = %d \n", lv_txid, lv_retcode);

   lv_txid++;
   lp_myHbaseTM->beginTransaction(&lv_txid);
   printf("After lp_myHbaseTM->beginTransaction, transactionId = %ld \n", lv_txid);

   lv_retcode = lp_myHbaseTM->abortTransaction(lv_txid);
   printf("After lp_myHbaseTM->abortTransaction(transactionId = %ld), retcode = %d \n", lv_txid, lv_retcode);

   lv_txid++;
   lp_myHbaseTM->beginTransaction(&lv_txid);
   printf("After lp_myHbaseTM->beginTransaction, transactionId = %ld \n", lv_txid);

   lv_retcode = lp_myHbaseTM->abortTransaction(lv_txid);
   printf("After lp_myHbaseTM->abortTransaction(transactionId = %ld), retcode = %d \n", lv_txid, lv_retcode);

   lv_txid++;
   lp_myHbaseTM->beginTransaction(&lv_txid);
   printf("After lp_myHbaseTM->beginTransaction, transactionId = %ld \n", lv_txid);

   lv_retcode = lp_myHbaseTM->abortTransaction(lv_txid);
   printf("After lp_myHbaseTM->abortTransaction(transactionId = %ld), retcode = %d \n", lv_txid, lv_retcode);

   lv_txid++;
   lp_myHbaseTM->beginTransaction(&lv_txid);
   printf("After lp_myHbaseTM->beginTransaction, transactionId = %ld \n", lv_txid);

   lv_retcode = lp_myHbaseTM->abortTransaction(lv_txid);
   printf("After lp_myHbaseTM->abortTransaction(transactionId = %ld), retcode = %d \n", lv_txid, lv_retcode);

   lv_txid++;
   lp_myHbaseTM->beginTransaction(&lv_txid);
   printf("After lp_myHbaseTM->beginTransaction, transactionId = %ld \n", lv_txid);

   lv_retcode = lp_myHbaseTM->abortTransaction(lv_txid);
   printf("After lp_myHbaseTM->abortTransaction(transactionId = %ld), retcode = %d \n", lv_txid, lv_retcode);

   lv_txid++;
   lp_myHbaseTM->beginTransaction(&lv_txid);
   printf("After lp_myHbaseTM->beginTransaction, transactionId = %ld \n", lv_txid);

   lv_retcode = lp_myHbaseTM->abortTransaction(lv_txid);
   printf("After lp_myHbaseTM->abortTransaction(transactionId = %ld), retcode = %d \n", lv_txid, lv_retcode);

   lv_txid++;
   lp_myHbaseTM->beginTransaction(&lv_txid);
   printf("After lp_myHbaseTM->beginTransaction, transactionId = %ld \n", lv_txid);

   lv_retcode = lp_myHbaseTM->abortTransaction(lv_txid);
   printf("After lp_myHbaseTM->abortTransaction(transactionId = %ld), retcode = %d \n", lv_txid, lv_retcode);

   lv_txid++;
   lp_myHbaseTM->beginTransaction(&lv_txid);
   printf("After lp_myHbaseTM->beginTransaction, transactionId = %ld \n", lv_txid);

   lv_retcode = lp_myHbaseTM->abortTransaction(lv_txid);
   printf("After lp_myHbaseTM->abortTransaction(transactionId = %ld), retcode = %d \n", lv_txid, lv_retcode);

   lv_txid++;
   lp_myHbaseTM->beginTransaction(&lv_txid);
   printf("After lp_myHbaseTM->beginTransaction, transactionId = %ld \n", lv_txid);

   lv_retcode = lp_myHbaseTM->prepareCommit(lv_txid, errStr, errStrLen);
   printf("After lp_myHbaseTM->prepareCommit(transactionId = %ld), retcode = %d \n", lv_txid, lv_retcode);

   lv_retcode = lp_myHbaseTM->participatingRegions(lv_txid);
   printf("After lp_myHbaseTM->participatingRegions(transactionId = %ld), retcode = %d \n", lv_txid, lv_retcode);

   lv_retcode = lp_myHbaseTM->doCommit(lv_txid);
   printf("After lp_myHbaseTM->doCommit(transactionId = %ld), retcode = %d \n", lv_txid, lv_retcode);

   ctrlPtNum = lp_myHbaseTM->addControlPoint();
   printf("After lp_myHbaseTM->addControlPoint, (transactionId = %ld),  ctrlPtNum = %ld \n", lv_txid, ctrlPtNum);

   delete lp_myHbaseTM;
   
   return 0;
}
