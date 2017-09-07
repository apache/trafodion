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
#ifndef EXP_INTERVAL_H
#define EXP_INTERVAL_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         exp_interval.h
 * Description:  Interval Type
 *
 *
 * Created:      8/21/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ExpError.h"
#include "exp_attrs.h"
#include "dfs2rec.h"

class ExpInterval : public SimpleType {

public:

static short getIntervalStartField(Lng32 fsDatatype,
				   rec_datetime_field &startField);

static short getIntervalEndField(Lng32 fsDatatype,
				 rec_datetime_field &endField);
  
static Lng32 getStorageSize(rec_datetime_field startField,
			   UInt32 leadingPrecision,
			   rec_datetime_field endField,
			   UInt32 fractionPrecision = 0);
  
static Lng32 getDisplaySize(Lng32 fsDatatype,
			   short leadingPrecision,
			   short fractionPrecision);
  
};

#endif
