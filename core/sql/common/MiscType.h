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
#ifndef MISCTYPE_H
#define MISCTYPE_H
/* -*-C++-*-
**************************************************************************
*
* File:         MiscType.h
* Description:  Miscellaneous types
* Created:      4/12/95
* Language:     C++
*
*
*
*
**************************************************************************
*/

// -----------------------------------------------------------------------

#include <limits.h>
#include "BaseTypes.h"
#include "NAType.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class SQLBoolean;
class SQLRowset;
class SQLUnknown;

//-----------------------------------------------------------------------
static NAString LiteralBoolean("BOOLEAN");
static NAString LiteralRecord("RECORD");
static NAString LiteralUnknown("UNKNOWN");
static NAString LiteralRowset("ROWSET");
//-----------------------------------------------------------------------

// ***********************************************************************
//
//  SQLBooleanBase : The boolean data type
//
// ***********************************************************************
class SQLBooleanBase : public NAType
{
public:

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  SQLBooleanBase(NAMemory *heap, NABoolean allowSQLnull,
             NABoolean isRelat);

  // ---------------------------------------------------------------------
  // A method which tells if a conversion error can occur when converting
  // a value of this type to the target type.
  // ---------------------------------------------------------------------
  NABoolean errorsCanOccur (const NAType& target, NABoolean lax=TRUE) const;
 
private:
}; // class SQLBooleanBase

// ***********************************************************************
//
//  SQLBooleanRelat : The boolean data type used in relational operators
//
// ***********************************************************************
class SQLBooleanRelat : public SQLBooleanBase
{
public:

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  SQLBooleanRelat(NAMemory *heap, NABoolean sqlUnknownFlag = TRUE);
  
  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(NAMemory* h=0) const;

  virtual short getFSDatatype() const { return REC_BIN32_SIGNED; }

  // ---------------------------------------------------------------------
  // Get the external/SQL name of the Type.
  // ---------------------------------------------------------------------
  virtual NAString getTypeSQLname(NABoolean terse = FALSE) const
  {return "BOOLEAN_RELAT";}

  // ---------------------------------------------------------------------
  // A virtual function for synthesizing the type of a binary operator.
  // ---------------------------------------------------------------------
  virtual const NAType* synthesizeType(enum NATypeSynthRuleEnum synthRule,
                                       const NAType& operand1,
                                       const NAType& operand2,
				       CollHeap* h,
				       UInt32 *flags = NULL) const;
  
  // ---------------- Methods not inherited from NAType ----------------

  NABoolean canBeSQLUnknown() const { return sqlUnknownFlag_;}

  private:
  NABoolean sqlUnknownFlag_;
}; // class SQLBooleanRelat

// ***********************************************************************
//
//  SQLBooleanNative : The boolean data type used as a column datatype
//
// ***********************************************************************
class SQLBooleanNative : public SQLBooleanBase
{
public:

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  SQLBooleanNative(NAMemory *heap, NABoolean allowSQLnull);
  
  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(NAMemory* h=0) const;

  virtual short getFSDatatype() const { return REC_BOOLEAN; }

  // ---------------------------------------------------------------------
  // Get the external/SQL name of the Type.
  // ---------------------------------------------------------------------
  virtual NAString getTypeSQLname(NABoolean terse = FALSE) const
  {return "BOOLEAN";}

  // ---------------------------------------------------------------------
  // A virtual function for synthesizing the type of a binary operator.
  // ---------------------------------------------------------------------
  virtual const NAType* synthesizeType(enum NATypeSynthRuleEnum synthRule,
                                       const NAType& operand1,
                                       const NAType& operand2,
				       CollHeap* h,
				       UInt32 *flags = NULL) const;
  
  virtual void minRepresentableValue(void* bufPtr, Lng32* bufLen,
                                     NAString ** stringLiteral,
				     CollHeap* h) const;

  virtual void maxRepresentableValue(void* bufPtr, Lng32* bufLen,
                                     NAString ** stringLiteral,
				     CollHeap* h) const;

  // ---------------- Methods not inherited from NAType ----------------

  private:
}; // class SQLBooleanNative

// ***********************************************************************
//
//  SQLRowset: the Rowset array data type. 
//             This is mainly for host variables of Rowset type. Other than
//             that, rowsets would be very much like arrays.
//
//  The name SQLArray is not used to avoid a future conflict with a real
//  database array type such as collection.
// ***********************************************************************

class SQLRowset : public NAType
{
public:
  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  SQLRowset(NAMemory *heap, NAType *elementType, Lng32 maxNumElements, 
            Lng32 numElements);
  
  // ---------------------------------------------------------------------
  // Are the two types compatible?
  // ---------------------------------------------------------------------
  virtual NABoolean isCompatible(const NAType& other, UInt32 * flags = NULL) const;
  virtual NABoolean operator==(const NAType& other) const;

  // ---------------------------------------------------------------------
  // Can a conversion error occur when converting from this datatype
  // to a given target datatype?
  // ---------------------------------------------------------------------
  virtual NABoolean errorsCanOccur (const NAType& target, 
                                    NABoolean lax=TRUE) const;

  virtual NAString getSimpleTypeName() const;
  virtual NAString getTypeSQLname(NABoolean terse = FALSE) const;

  // ---------------------------------------------------------------------
  // Get the filesystem datatype value (from DFS2REC) for this type.
  // ---------------------------------------------------------------------

  virtual short getFSDatatype() const;

  // -------------------------------------------------------------------------
  // Operations on 'simple' type can usually be supported by the
  // underlying hardware (like, smallint, long, etc). 

  // Operations on 'complex' type are done in software as they involve
  // more complicated operations (like datetime, large decimal, etc).
  // Used by expression generator to decide which of the two methods to
  // use to generate code.
  // -------------------------------------------------------------------------
  virtual NABoolean isSimpleType()  const {return TRUE;};
  virtual NABoolean isComplexType() const {return FALSE;};

  // ---------------------------------------------------------------------
  // Print function for debugging
  // ---------------------------------------------------------------------
  virtual void print(FILE* ofd = stdout, const char* indent = DEFAULT_INDENT);

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const;

  virtual const NAType* synthesizeType(enum NATypeSynthRuleEnum synthRule,
                                       const NAType& operand1,
                                       const NAType& operand2,
				       CollHeap* h,
				       UInt32 *flags = NULL) const;


  // ---------------- Methods not inherited from NAType ----------------
  // Accesor methods
  // ---------------------------------------------------------------------
  NAType* getElementType() const;
  Lng32    getNumElements() const;
  Lng32    setNumElements(Lng32 numElements);
  Lng32    getMaxNumElements() const;  
  NABoolean &useTotalSize() 
  {
    return useTotalSize_;
  }

  private:
  NAType *elementType_;        // The type of each array element
  const Lng32  maxNumElements_; // Maximum number of elements as specified
                               // in the host variable declaration
  Lng32  numElements_;          // Number of elements in use always starting
                               // from zero.
  NABoolean useTotalSize_;     // Used at generation time to determine
                               // if the whole rowset size is to be
                               // generated or part of it
}; // class SQLRowset

// ***********************************************************************
//
//  SQLRecord : The record data type
//
// ***********************************************************************
class SQLRecord : public NAType
{
public:

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  SQLRecord(NAMemory *heap, const NAType * elementType,
            const SQLRecord * restOfRecord);

  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap * h=0) const;

  short getFSDatatype() const;

  // ---------------------------------------------------------------------
  // Get a simpler textual description of the type.  
  // ---------------------------------------------------------------------
  virtual NAString getSimpleTypeName() const;

  // ---------------------------------------------------------------------
  // Get the external/SQL name of the Type.
  // ---------------------------------------------------------------------
  virtual NAString getTypeSQLname(NABoolean terse = FALSE) const;

  // ---------------------------------------------------------------------
  // A virtual function for synthesizing the type of a binary operator.
  // ---------------------------------------------------------------------
  virtual const NAType* synthesizeType(enum NATypeSynthRuleEnum synthRule,
                                       const NAType& operand1,
                                       const NAType& operand2,
				       CollHeap* h,
				       UInt32 *flags = NULL) const;

  // ---------------------------------------------------------------------
  // Are the two types compatible?
  // ---------------------------------------------------------------------
  virtual NABoolean isCompatible(const NAType& other, UInt32 * flags = NULL) const;

  // ---------------------------------------------------------------------
  // A method which tells if a conversion error can occur when converting
  // a value of this type to the target type.
  // ---------------------------------------------------------------------
  NABoolean errorsCanOccur (const NAType& target, NABoolean lax=TRUE) const;

  // ---------------- Methods not inherited from NAType ----------------

  // ---------------------------------------------------------------------
  // Accesor methods
  // ---------------------------------------------------------------------
  const NAType * getElementType() const;
  const SQLRecord * getRestOfRecord() const;
  Lng32 getDegree() const;

  private:
  const NAType * elementType_;
  const SQLRecord * restOfRecord_;
  Lng32  degree_;          // number of types in the record

}; // class SQLRecord

// ***********************************************************************
//
//  SQLUnknown : The unknown data type (for params and NULLs)
//
// ***********************************************************************
class SQLUnknown : public NAType
{
public:

  // ---------------------------------------------------------------------
  // Constructor functions
  // ---------------------------------------------------------------------
  SQLUnknown(NAMemory *heap, NABoolean supportsSQLnull = FALSE)
  : NAType(heap, LiteralUnknown, NA_UNKNOWN_TYPE, 2, supportsSQLnull, SQL_NULL_HDR_SIZE,FALSE,0,1) {}
  // copy ctor
  SQLUnknown(const SQLUnknown &unknown,NAMemory * heap=0)
	  :NAType(unknown,heap)
  {}
  // ---------------------------------------------------------------------
  // A virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(CollHeap* h=0) const;

  // ---------------------------------------------------------------------
  // A method which tells if a conversion error can occur when converting
  // a value of this type to the target type.
  // ---------------------------------------------------------------------
  NABoolean errorsCanOccur (const NAType& target, NABoolean lax=TRUE) const;

}; // class SQLUnknown

#endif /* MISCTYPE_H */




