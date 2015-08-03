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
****************************************************************************
*
* File:         UdrFormalParamInfo.h
* Description:  Metadata for UDR parameters
* Created:      10/10/2000
* Language:     C++
*
****************************************************************************
*/

#ifndef UDR_FORMAL_PARAM_INFO_H
#define UDR_FORMAL_PARAM_INFO_H

#include "NAVersionedObject.h"
#include "ComSmallDefs.h"

//
// Contents of this file
//
class UdrFormalParamInfo;

//
// Template instantiation to produce a 64-bit pointer emulator class
// for UdrFormalParamInfo
//
typedef NAVersionedObjectPtrTempl<UdrFormalParamInfo> UdrFormalParamInfoPtr;

//
// Template instantiation to produce a 64-bit pointer emulator class
// for UdrFormalParamInfoPtr
//
typedef NAVersionedObjectPtrArrayTempl<UdrFormalParamInfoPtr> UdrFormalParamInfoPtrPtr;

//
// class UdrFormalParamInfo
//
// Each instance of this class describes a single UDR formal parameter
//
class UdrFormalParamInfo : public NAVersionedObject
{
public:

  UdrFormalParamInfo(Int16 type, Int16 precision, Int16 scale, Int16 flags,
                     Int16 encodingCharSet, Int16 collation, char *paramName);
  
  // Redefine virtual functions required for versioning
  UdrFormalParamInfo() : NAVersionedObject(-1)
  {}
  virtual unsigned char getClassVersionID()
  { return 1; }
  virtual void populateImageVersionIDArray()
  { setImageVersionID(0, getClassVersionID()); }
  virtual short getClassSize()
  { return sizeof(UdrFormalParamInfo); }

  virtual Long pack(void *);
  virtual Lng32 unpack(void *, void *);

  // Accessor functions
  inline Int16 getFlags() const { return flags_; }
  inline Int16 getType() const { return type_; }
  inline Int16 getPrecision() const { return precision_; }
  inline Int16 getScale() const { return scale_; }
  inline Int16 getEncodingCharSet() const { return encodingCharSet_; }
  inline Int16 getCollation() const { return collation_; }
  inline char *getParamName() const { return paramName_; }
  inline NABoolean isIn() const { return (flags_ & UDR_PARAM_IN); }
  inline NABoolean isOut() const { return (flags_ & UDR_PARAM_OUT); }
  inline NABoolean isInOut() const { return isIn() && isOut(); }
  inline NABoolean isNullable() const { return (flags_ & UDR_PARAM_NULLABLE); }
  inline NABoolean isLmObjType() const
  { return (flags_ & UDR_PARAM_LM_OBJ_TYPE); }

protected:

  // Fields in this class describe a UDR formal parameter. They do not
  // necessarily describe how an actual data value will be represented in
  // the message that gets sent to the UDR server.
  // 
  // The SQL datatype is an FS datatype from common/dfs2rec.h. The 
  // precision field is either numeric precision or a qualifier for
  // datetime types. The scale field is always numeric scale.
  //
  // The enumeration for flags is ComUdrParamFlags in ComSmallDefs.h.
  //
  // encodingCharSet_ and collation_ apply only to character types and
  // are Int16 cast'ed values of CharInfo::CharSet and
  // CharInfo::Collation.

  Int16 flags_;                          // 00-01
  Int16 type_;                           // 02-03
  Int16 precision_;                      // 04-05
  Int16 scale_;                          // 06-07
  Int16 encodingCharSet_;                // 08-09
  Int16 collation_;                      // 10-11
  char fillersUdrFormalParamInfo1_[4];   // 12-15
  NABasicPtr paramName_;                 // 16-23
  char fillersUdrFormalParamInfo2_[40];  // 24-63
};

#endif // UDR_FORMAL_PARAM_INFO_H

