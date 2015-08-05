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
#ifndef ELEMDDLFILEATTRRANGELOG_H
#define ELEMDDLFILEATTRRANGELOG_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLFileAttrRangeLog.h
* Description:  class for Range log File Attribute (parse node)
*               elements in DDL statements
*
*               
* Created:      11/21/99
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "ElemDDLFileAttr.h"
#include "ComSmallDefs.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLFileAttrRangeLog;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// Range-Log File Attribute (parse node) elements in DDL statements
// -----------------------------------------------------------------------

class ElemDDLFileAttrRangeLog : public ElemDDLFileAttr
{

public:

  
  // default constructor
  ElemDDLFileAttrRangeLog(ComRangeLogType rangelogType = COM_NO_RANGELOG)
    : ElemDDLFileAttr(ELM_FILE_ATTR_RANGE_LOG_ELEM),
      rangelogType_(rangelogType)
  {
  }

  // virtual destructor
  virtual ~ElemDDLFileAttrRangeLog();

  // cast
  virtual ElemDDLFileAttrRangeLog * castToElemDDLFileAttrRangeLog();

  // accessor
  ComRangeLogType
  getRangelogType() const
  {
    return rangelogType_;
  }

  // member functions for tracing
  virtual const NAString getText() const;
  virtual const NAString displayLabel1() const;
  static NAString getRangeLogTypeAsNAString(ComRangeLogType type);

  // method for building text
  virtual NAString getSyntax() const;


private:

  ComRangeLogType rangelogType_;

}; // class ElemDDLFileAttrRangeLog

#endif // ELEMDDLFILEATTRRANGELOG_H
