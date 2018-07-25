#ifndef DEFINE_H
#define DEFINE_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         Define.h
 * RCS:          $Id: Define.h,v 1.3 1997/06/20 23:39:45  Exp $
 * Description:  
 *               
 *               
 * Created:      4/15/95
 * Modified:     $ $Date: 1997/06/20 23:39:45 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
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

class SqlciEnv;

class Envvar 
{
  char * name;
  char * value;
  char * env_str;
  
public:
  Envvar(const char * name_, const char * value_);
  ~Envvar();

  const char * getName() const		{return name;};
  void setName(const char * name_);

  const char * getValue() const		{return value;};
  void setValue(const char * value_);

  short contains(const char * value) const;

  Int32 set();
  Int32 reset();
};

#endif
