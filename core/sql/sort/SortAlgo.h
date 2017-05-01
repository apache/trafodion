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
#ifndef SORTALGO_H
#define SORTALGO_H

/* -*-C++-*-
******************************************************************************
*
* File:         SortAlgo.h
* RCS:          $Id: SortAlgo.h,v 1.2.16.2 1998/05/26 22:32:37  Exp $
*                               
* Description:  This class represents the generic SortAlgorithm base class.
*               Specific  algorithms are implemented as subclasses of this 
*               class. Note that SortAlgorithm is a virtual base class since
*               it contains pure virtual functions. 
*                              
* Created:	    05/22/96
* Modified:     $ $Date: 1998/05/26 22:32:37 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
******************************************************************************
*/

#include "str.h"
#include "CommonStructs.h"
#include "ScratchSpace.h"
#include <fstream>
#include "Const.h"
#include "NABasicObject.h"

class SortAlgo : public NABasicObject 
{

  public :

   SortAlgo(ULng32 runsize, ULng32 recsize, 
            NABoolean doNotallocRec, ULng32 keysize, 
            SortScratchSpace* scratch, Lng32 explainNodeId, ExBMOStats *bmoStats);
   ~SortAlgo() {}; 
     
   //------------------------------------------------------------
   // Note that sort is implemented as a pure virtual function.
   //------------------------------------------------------------

   virtual Lng32 sortSend(void *rec, ULng32 len, void* tupp) = 0;

   virtual Lng32 sortClientOutOfMem(void) = 0;

   virtual Lng32 sortSendEnd(void) = 0;

   virtual Lng32 sortReceive(void *rec, ULng32& len) = 0;
   virtual Lng32 sortReceive(void*& rec,ULng32& len,void*& tupp)=0;
   virtual UInt32 getOverheadPerRecord(void) = 0;
   //-----------------------------------------------------------
   // Since the compare routine is independent of the sort 
   // algorithm it can be a member of this base class.
   //-----------------------------------------------------------
   short compare(char* key1, char* key2)  ;
   virtual Lng32  generateInterRuns() = 0;
   ULng32 getNumOfCompares() const;
   SortScratchSpace* getScratch() const;
   Lng32 getRunSize() const;
   void setExternalSort(void) { internalSort_ = FALSE_L; }
   NABoolean isInternalSort(void) { return internalSort_; }
  
  
  protected :  
 
   ULng32 numCompares_;
   ULng32 runSize_;
   ULng32 recSize_;
   ULng32 keySize_;
   SortScratchSpace *scratch_;
   NABoolean sendNotDone_;
   NABoolean internalSort_;
   NABoolean doNotallocRec_;
   Lng32 explainNodeId_;
   ExBMOStats *bmoStats_;

};





#endif







