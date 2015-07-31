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
/**********************************************************************/
#ifndef ENCODEDKEYVALUE_H
#define ENCODEDKEYVALUE_H
/* -*-C++-*-
/**************************************************************************
*
* File:         EncodedKeyValue.h
* Description:  Functions to compute binary encoded keys that can be written 
                to disk for a given set of desc_structs.
* Origin:       Copy of existing code from GenRFork.cpp
* Created:      10/30/2013
* Language:     C++
*
*************************************************************************
*/

#include "Platform.h"
#include "desc.h"
#include "NAString.h"
#include "Generator.h"


NAString * getMinMaxValue(desc_struct * column,
                          desc_struct * key,
                          NABoolean highKey,
                          CollHeap * h);

NAString ** createInArrayForLowOrHighKeys(desc_struct   * column_descs,
					  desc_struct   * key_descs,
					  Lng32 numKeys,
					  NABoolean highKey,
                                          NABoolean isIndex,
                                          CollHeap * h);


ItemExpr * buildEncodeTree(desc_struct * column,
                           desc_struct * key,
                           NAString * dataBuffer, //IN:contains original value
                           Generator * generator,
                           ComDiagsArea * diagsArea);

short encodeKeyValues(desc_struct   * column_descs,
		      desc_struct   * key_descs,
		      NAString      * inValuesArray[],          // INPUT
                      NABoolean isIndex,
		      char * encodedKeyBuffer,                  // OUTPUT
                      CollHeap * h,
		      ComDiagsArea * diagsArea);





#endif /* ENCODEDKEYVALUE_H */
