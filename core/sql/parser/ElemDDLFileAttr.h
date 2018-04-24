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
#ifndef ELEMDDLFILEATTR_H
#define ELEMDDLFILEATTR_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLFileAttr.h
 * Description:  base class for all File Attribute parse nodes in DDL
 *               statements
 *
 *               
 * Created:      5/30/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ComUnits.h"
#include "ElemDDLNode.h"
#include "ComASSERT.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLFileAttr;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// base class for all File Attribute parse nodes in DDL statements
// -----------------------------------------------------------------------

class ElemDDLFileAttr : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLFileAttr(OperatorTypeEnum operType = ELM_ANY_FILE_ATTR_ELEM)
  : ElemDDLNode(operType)
  { }

  // virtual destructor
  virtual ~ElemDDLFileAttr();

  // cast
  virtual ElemDDLFileAttr * castToElemDDLFileAttr();

  // given a size unit enumerated constant,
  // return an appropriate NAString.
  NAString convertSizeUnitToNAString(ComUnits sizeUnit) const;

  // methods for tracing
  virtual const NAString getText() const;

  // method for building text
  virtual NAString getSyntax() const { ComASSERT(FALSE); return "";}


private:


}; // class ElemDDLFileAttr


// class ElemDDLFileAttrCompression
class ElemDDLFileAttrCompression : public ElemDDLFileAttr
{

public:

  // default constructor
  ElemDDLFileAttrCompression(ComCompressionType compressionSpec)
                          : ElemDDLFileAttr(ELM_FILE_ATTR_COMPRESSION_ELEM),
                            compressionType_(compressionSpec)
  {
  }

  // virtual destructor
  virtual ~ElemDDLFileAttrCompression();

  // cast
  virtual ElemDDLFileAttrCompression * castToElemDDLFileAttrCompression();

  // accessor
  const ComCompressionType
  getCompressionType() const
  {
    return compressionType_;
  }

private:

  ComCompressionType compressionType_;

}; // class ElemDDLFileAttrCompression

class ElemDDLFileAttrUID : public ElemDDLFileAttr
{

public:

  ElemDDLFileAttrUID(Int64 UID);

  // virtual destructor
  virtual ~ElemDDLFileAttrUID();

  // cast
  virtual ElemDDLFileAttrUID * castToElemDDLFileAttrUID();

  // accessors
  inline Int64 getUID() const { return UID_; }

private:

  Int64 UID_;
  
}; // class ElemDDLFileAttrUID

class ElemDDLFileAttrRowFormat : public ElemDDLFileAttr
{

public:

  enum ERowFormat { eUNSPECIFIED, ePACKED, eALIGNED, eHBASE };

public:

  ElemDDLFileAttrRowFormat(ERowFormat rowFormat);

  // virtual destructor
  virtual ~ElemDDLFileAttrRowFormat();

  // cast
  virtual ElemDDLFileAttrRowFormat * castToElemDDLFileAttrRowFormat();

  // accessors
  inline ERowFormat getRowFormat() const { return eRowFormat_; }

  // method for building text
  virtual NAString getSyntax() const;

private:

  ERowFormat eRowFormat_;
  
}; // class ElemDDLFileAttrRowFormat

class ElemDDLFileAttrSuperTable: public ElemDDLFileAttr
{

public:

  ElemDDLFileAttrSuperTable(NAString &tblNm)
       : ElemDDLFileAttr(ELM_FILE_ATTR_SUPER_TABLE_ELEM),
         superTable_(tblNm)
  {
  }

  // virtual destructor
  virtual ~ElemDDLFileAttrSuperTable()
  {
  };

  // cast
  virtual ElemDDLFileAttrSuperTable* castToElemDDLFileAttrSuperTable()
  {
    return this;
  }

  // accessors
  NAString &getSuperTable()
  {
    return superTable_;
  }

  // method for building text
  virtual NAString getSyntax() const
  {
   return "";
  }

private:

  NAString superTable_;

}; // class ElemDDLFileAttrSuperTable



class ElemDDLFileAttrColFam : public ElemDDLFileAttr
{

public:

  ElemDDLFileAttrColFam(NAString &colFam)
       : ElemDDLFileAttr(ELM_FILE_ATTR_COL_FAM_ELEM),
         colFam_(colFam)
  {
  }

  // virtual destructor
  virtual ~ElemDDLFileAttrColFam()
  {
  };

  // cast
  virtual ElemDDLFileAttrColFam * castToElemDDLFileAttrColFam()
  {
    return this; 
  }

  // accessors
  NAString &getColFam() 
  { 
    return colFam_; 
  }

  // method for building text
  virtual NAString getSyntax() const
  {
    return "";
  }

private:

  NAString colFam_;
  
}; // class ElemDDLFileAttrColFam

#endif // ELEMDDLFILEATTR_H
