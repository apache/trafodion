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
#ifndef ELEMDDLFILEATTRDCOMPRESS_H
#define ELEMDDLFILEATTRDCOMPRESS_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLFileAttrDCompress.h
* Description:  class for D-Compress File Attribute (parse node) elements in
*               DDL statements
*
*               
* Created:      4/21/95
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "ElemDDLFileAttr.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLFileAttrDCompress;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// D-Compress File Attribute (parse node) elements in DDL statements
// -----------------------------------------------------------------------

class ElemDDLFileAttrDCompress : public ElemDDLFileAttr
{

public:

  // default constructor
  ElemDDLFileAttrDCompress(NABoolean dCompressSpec = TRUE)
                          : ElemDDLFileAttr(ELM_FILE_ATTR_D_COMPRESS_ELEM),
                            isDCompress_(dCompressSpec)
  {
  }

  // virtual destructor
  virtual ~ElemDDLFileAttrDCompress();

  // cast
  virtual ElemDDLFileAttrDCompress * castToElemDDLFileAttrDCompress();

  // accessor
  const NABoolean
  getIsDCompress() const
  {
    return isDCompress_;
  }

  // member functions for tracing
  virtual const NAString getText() const;
  virtual const NAString displayLabel1() const;

  // method for building text
  virtual NAString getSyntax() const;


private:

  NABoolean isDCompress_;

}; // class ElemDDLFileAttrDCompress

#endif /* ELEMDDLFILEATTRDCOMPRESS_H */
