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
#ifndef RECORD_H
#define RECORD_H

/* -*-C++-*-
******************************************************************************
*
* File:         Record.h
* RCS:          $Id: Record.h,v 1.2.16.2 1998/07/08 21:47:13  Exp $
*                               
* Description:  This class represents the Record. The actual data is stored
*               in a char* with member fuctions to retrieve and store this
*               data.
*                              
* Created:	    05/20/96
* Modified:     $ $Date: 1998/07/08 21:47:13 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
******************************************************************************
*/

#include "CommonStructs.h"
#include "Const.h"
#include "ScratchSpace.h"
#include "NABasicObject.h"
#include "SortError.h"

class Record   {
  public :

    Record();
    Record(ULng32 size, NABoolean doNotallocRec, CollHeap* heap);
    ~Record(void);

    void initialize(ULng32 recsize, NABoolean doNotallocRec, 
                    CollHeap* heap, SortError* sorterror);

    char* extractKey(ULng32 keylen, Int16 offset);

    NABoolean setRecord(void *rec, ULng32 reclen);
    NABoolean getRecord(void *rec, ULng32 reclen) const;

    NABoolean setRecordTupp(void* rec, ULng32 reclen, void* tupp);
    NABoolean getRecordTupp(void*& rec,ULng32 reclen,void*& tupp)const;
 
    RESULT getFromScr(SortMergeNode* sortMergeNode, 
                     ULng32 reclen,
                     SortScratchSpace* scratch,
                     ULng32 &actRecLen,
                     //ULng32 keySize,
                     NABoolean waited = FALSE,
                     Int16 numberOfBytesForRecordSize = 0);
   
    //void putToFile(ofstream& to);  
    RESULT putToScr(ULng32 run,
                  ULng32 reclen,
                  SortScratchSpace* scratch, NABoolean waited = FALSE);

    void releaseTupp(void);
    ULng32 getRecSize()
    {
      return recSize_;
    }

  private :
    char* rec_;          // Pointer to the data which constitutes the actual
                         // record data.
    ULng32 recSize_;
    void* tupp_;
    
    NABoolean allocatedRec_;
    SortError* sortError_;
    CollHeap* heap_;
};

#endif




















