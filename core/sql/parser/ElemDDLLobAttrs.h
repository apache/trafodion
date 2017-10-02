#ifndef ELEMDDLLOBATTRS_H
#define ELEMDDLLOBATTRS_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLLobAttrs.h
 * Description:  column LOB attrs
 *
 *               
 * Created:      
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
 *
 *
 *****************************************************************************
 */


#include "ElemDDLNode.h"
#include "ExpLOBenums.h"

class ElemDDLLobAttrs : public ElemDDLNode
{

public:

 ElemDDLLobAttrs(LobsStorage storage)
   : ElemDDLNode(ELM_LOBATTRS),
    storage_(storage)
    {
      
    }
  
  
  // virtual destructor
  virtual ~ElemDDLLobAttrs(){}
  
  // cast
  virtual ElemDDLLobAttrs* castToElemDDLLobAttrs(){return this;}
  
  // methods for tracing
  //	virtual const NAString displayLabel2() const;
  //	virtual const NAString getText() const;
  
 LobsStorage getLobStorage() { return storage_; }
  
 private:
  
  LobsStorage storage_;
  
}; // class ElemDDLElemDDLLobAttrsAttribute

// this class is used if 'serialized' option is specified for a column.
// If it is specified, then column values stored in seabase/hbase are
// encoded and serialized. This allows hbase filter predicates to be
// passed down to hbase during scan and other operations.
class ElemDDLSeabaseSerialized : public ElemDDLNode
{

public:

 ElemDDLSeabaseSerialized(NABoolean serialized)
   : ElemDDLNode(ELM_SEABASE_SERIALIZED),
    serialized_(serialized)
    {
    }
  
  // virtual destructor
  virtual ~ElemDDLSeabaseSerialized(){}
  
  // cast
  virtual ElemDDLSeabaseSerialized* castToElemDDLSeabaseSerialized(){return this;}
  
  ComBoolean serialized() { return serialized_;};
 private:
  
  ComBoolean serialized_;
}; // class ElemDDLSeabaseSerialized



#endif // ELEMDDLLOBATTRS_H
