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
 * File:         ProcessEnv.h
 * Description:  The class declaration for the process environment. i.e.
 *               current working directory and environment variables.
 *               
 *               
 * Created:      07/10/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#ifndef PROCESSENV_H
#define PROCESSENV_H

// ProcessEnv contains the methods for setting the runtime environment
// for certain process, including current working directory(chdir) and
// environment variables. Eventually whatever sqlci handles for environment
// setup should be set here. This is for arkcmp to set up the same run time
// environment as sqlci.

class ProcessEnv : public NABasicObject
{
public:

  ProcessEnv(CollHeap *heap) ;
  
  void cleanup();
  void setEnv(char** newenvs, Lng32 nEnvs);
  void addOrChangeEnv(char **newenvs, Lng32 nEnvs);
  void resetEnv(const char *envName);
  Int32 unsetEnv(char* env);  
  Int32 chdir(char* dir);
  void dumpEnvs();
  
  virtual ~ProcessEnv();
  
private:
  void removeEnv(char **newenvs, Lng32 nEnvs);

  CollHeap *heap_;
  // The following members are used to keep the environment variable info.
  NAArray<char*> envs_;
  // Copy Constructor Unimplemented, to prevent an accidental use of 
  // the default byte copy operation
  // Copy Constructor Unimplemented, to prevent an accidental use of 
  // the default byte copy operation
  ProcessEnv& operator=(const ProcessEnv&);
  ProcessEnv(const ProcessEnv&);
};

#endif
