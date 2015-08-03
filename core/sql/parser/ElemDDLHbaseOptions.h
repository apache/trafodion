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
#ifndef ELEMDDLHBASEOPTIONS_H
#define ELEMDDLHBASEOPTIONS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLHbaseoptions.h
 * Description:  Describes table options: [NOT] DROPPABLE & INSERT_ONLY
 *
 *               
 * Created:      04/02/2012
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLNode.h"
#include "ExpHbaseDefs.h"

class ElemDDLHbaseOptions : public ElemDDLNode
{
  
 public:

  ElemDDLHbaseOptions(NAList<HbaseCreateOption*> * hbaseOptions,
		      CollHeap * heap)
   : ElemDDLNode(ELM_HBASE_OPTIONS_ELEM)
    {
      for (CollIndex i = 0; i < hbaseOptions->entries(); i++)
	{
	  HbaseCreateOption * hbo = new(heap) 
	    HbaseCreateOption(((*hbaseOptions)[i])->key(), ((*hbaseOptions)[i])->val());
						    
	  hbaseOptions_.insert(hbo);
	}
    }
  
  // virtual destructor
  virtual ~ElemDDLHbaseOptions(){}
  
  // cast
  virtual ElemDDLHbaseOptions* castToElemDDLHbaseOptions()
  {
    return this;
  }
  
  NAList<HbaseCreateOption*> &getHbaseOptions() {return hbaseOptions_;}
  
  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString getText() const;
  
 private:
  
  NAList<HbaseCreateOption*> hbaseOptions_;
  
}; // class ElemDDLHbaseOptions



#endif // ELEMDDLHBASEOPTIONS_H
