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

/*
 * File:         ComExtractInfo.h
 * Description:  NAVersionedObject subclasses to store information in plans
 *               specific to parallel extract producer and consumer queries.
 * Created:      May 2007
 * Language:     C++
 *
 */

#ifndef COMEXTRACTINFO_H
#define COMEXTRACTINFO_H

#include "NAVersionedObject.h"

// -----------------------------------------------------------------------
// A class for information related to parallel extract producer
// queries. The top-level split top and split bottom TDBs for a
// producer query will each have a pointer to an instance of this
// class.
// -----------------------------------------------------------------------
class ComExtractProducerInfo : public NAVersionedObject
{
public:
  // Redefine virtual functions required for versioning
  ComExtractProducerInfo() : NAVersionedObject(-1)  {}
  virtual unsigned char getClassVersionID() { return 1; }
  virtual void populateImageVersionIDArray()
  { setImageVersionID(0,getClassVersionID()); }
  virtual short getClassSize()
  { return (short) sizeof(ComExtractProducerInfo); }
  Long pack(void *space);
  Lng32 unpack(void *base, void *reallocator);

  // Accessor functions
  const char *getSecurityKey() const { return securityKey_; }
  
  // Mutator functions
  void setSecurityKey(char *s) { securityKey_ = s; }
  
protected:
  NABasicPtr     securityKey_;       // 00-07
  char           filler_[56];        // 08-63
};

// -----------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for ComExtractProducerInfo
// -----------------------------------------------------------------------
typedef
  NAVersionedObjectPtrTempl<ComExtractProducerInfo> ComExtractProducerInfoPtr;

// -----------------------------------------------------------------------
// A class for information related to parallel extract consumer
// queries. The send top TDB for a consumer query will have a pointer
// to an instance of this class.
// -----------------------------------------------------------------------
class ComExtractConsumerInfo : public NAVersionedObject
{
public:
  // Redefine virtual functions required for versioning
  ComExtractConsumerInfo() : NAVersionedObject(-1)  {}
  virtual unsigned char getClassVersionID() { return 1; }
  virtual void populateImageVersionIDArray()
  { setImageVersionID(0,getClassVersionID()); }
  virtual short getClassSize()
  { return (short) sizeof(ComExtractConsumerInfo); }
  Long pack(void *space);
  Lng32 unpack(void *base, void *reallocator);

  // Accessor functions
  const char *getEspPhandle() const { return espPhandle_; }
  const char *getSecurityKey() const { return securityKey_; }
  
  // Mutator functions
  void setEspPhandle(char *s) { espPhandle_ = s; }
  void setSecurityKey(char *s) { securityKey_ = s; }
  
protected:
  NABasicPtr     espPhandle_;        // 00-07
  NABasicPtr     securityKey_;       // 08-15
  char           filler_[48];        // 16-63
};

// -----------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for ComExtractConsumerInfo
// -----------------------------------------------------------------------
typedef
  NAVersionedObjectPtrTempl<ComExtractConsumerInfo> ComExtractConsumerInfoPtr;

#endif // COMEXTRACTINFO_H
