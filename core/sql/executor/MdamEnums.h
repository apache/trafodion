#ifndef MDAMENUMS_H
#define MDAMENUMS_H
 
/* -*-C++-*-
********************************************************************************
*
* File:         MdamEnums.h
* Description:  MDAM Enums
*               
*               
* Created:      11/18/96
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
********************************************************************************
*/

// -----------------------------------------------------------------------------


// *****************************************************************************
// MdamEnums is a dummy class that contains Mdam-related enums.
// *****************************************************************************

class MdamEnums
{

public:
  
  enum MdamOrder {MDAM_LESS = -1, MDAM_EQUAL = 0, MDAM_GREATER = 1};

  enum MdamInclusion {MDAM_EXCLUDED = 0, MDAM_INCLUDED = 1};

  enum MdamEndPointType {MDAM_BEGIN = 0, MDAM_END = 1};

}; // end class MdamEnums


#endif /* MDAMENUMS_H */
