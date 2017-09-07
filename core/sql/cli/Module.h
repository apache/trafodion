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
#ifndef MODULE_H
#define MODULE_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         Module.h
 * Description:  
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ComVersionDefs.h"
// -----------------------------------------------------------------------

class Module : public ExGod {
  char * module_name_;
  Lng32 module_name_len_;
  char * path_name_;
  Lng32 path_name_len_;
  NAHeap * heap_;
  Int32 statementCount_;
  COM_VERSION version_;
  char * vproc_;
  
public:
  Module(const char * module_name, Lng32 module_name_len, 
         char * pathName, Lng32 pathNameLen, NAHeap *heap);
  ~Module();

  inline char * getModuleName() { return module_name_; };
  inline Lng32 getModuleNameLen() { return module_name_len_; };
  inline char * getPathName() { return path_name_; };
  inline Lng32 getPathNameLen() { return path_name_len_; };
  inline Int32 getStatementCount() { return statementCount_; }
  inline void setStatementCount(Int32 c) { statementCount_ = c; }
  void setVersion(COM_VERSION v) { version_ = v; }
  COM_VERSION getVersion() { return version_;}
  char * getVproc() { return vproc_;}
  void setVproc(char * v) { vproc_ = v;}
};

#endif
