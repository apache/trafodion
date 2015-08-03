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
**************************************************************************
*
* File:         MiscType.C
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

#include "MiscType.h"

// ***********************************************************************
//
//  SQLBoolean : The boolean data type
//
// ***********************************************************************
NAType * SQLBoolean::newCopy(CollHeap* h) const
{
  return new(h) SQLBoolean(unknownFlag_,h);
}

short SQLBoolean::getFSDatatype() const 
{
  return REC_BIN32_SIGNED;
}

NABoolean SQLBoolean::canBeSQLUnknown() const
{
  return unknownFlag_;
}

void SQLBoolean::setSQLUnknownFlag(NABoolean flag)
{
  unknownFlag_ = flag;
}

NAString SQLBoolean::getSimpleTypeName() const
{
  return "SQLBoolean";
}

// -- The external name for the type (text representation)

NAString SQLBoolean::getTypeSQLname(NABoolean) const
{
  if (unknownFlag_)
    return "BOOLEAN ALLOWS UNKNOWN";
  return "BOOLEAN NO UNKNOWNS";
}

// ***********************************************************************
//
// Type synthesis for binary operators
//
// ***********************************************************************

const NAType* SQLBoolean::synthesizeType(enum NATypeSynthRuleEnum synthRule,
                                         const NAType& operand1,
					 const NAType& operand2,
					 CollHeap* h,
					 UInt32 *flags) const
{
  //
  // If the second operand's type synthesis rules have higher precedence than
  // this operand's rules, use the second operand's rules.
  //
  if (operand2.getSynthesisPrecedence() > getSynthesisPrecedence())
    return operand2.synthesizeType(synthRule, operand1, operand2, h, flags);
  //
  // If either operand is not boolean, the expression is invalid.
  //
  if ((operand1.getTypeQualifier() != NA_BOOLEAN_TYPE) ||
      (operand2.getTypeQualifier() != NA_BOOLEAN_TYPE))
    return NULL;
  //
  // The generator can create CASE expressions that have boolean result
  // expressions.
  //
  if (synthRule == SYNTH_RULE_UNION)
    return new(h) SQLBoolean();
  return NULL;
} // synthesizeType()


// ---------------------------------------------------------------------
// A method which tells if a conversion error can occur when converting
// a value of this type to the target type.
// This method is a stub and so just returns true for now.
// ---------------------------------------------------------------------
NABoolean SQLBoolean::errorsCanOccur(const NAType& target, NABoolean lax) const
{
  return TRUE;
}


// ***********************************************************************
//
//  SQLRecord : The record data type
//
// ***********************************************************************
SQLRecord::SQLRecord(const NAType * elementType, const SQLRecord * restOfRecord,NAMemory * heap) :
   NAType(LiteralRecord,
          NA_RECORD_TYPE,
          elementType->getTotalSize() +
                (restOfRecord ? restOfRecord->getTotalSize()
                              : 0),               // dataStorageSize
          elementType->supportsSQLnull() |
                (restOfRecord ? restOfRecord->supportsSQLnull()
                              : FALSE),            // supportsSQLnull
          0,                                      // SQLnullHdrSize
          FALSE,                                  // variableLength
          0,                                      // lengthHeaderSize
          4,                                       // dataAlignment
          heap),
    elementType_(elementType),
    restOfRecord_(restOfRecord)
{
  if (restOfRecord)
    degree_ = restOfRecord->getDegree() + 1;
  else
    degree_ = 1;
}

NAType * SQLRecord::newCopy(CollHeap* h) const
{
  return new(h) SQLRecord(elementType_, restOfRecord_,h);
}

short SQLRecord::getFSDatatype() const 
{
  return REC_BYTE_F_ASCII;
}

const NAType * SQLRecord::getElementType() const
{
  return elementType_;
}

const SQLRecord * SQLRecord::getRestOfRecord() const
{
  return restOfRecord_;
}
Lng32 SQLRecord::getDegree() const
{
  return degree_;
}

NAString SQLRecord::getSimpleTypeName() const
{
  return "SQLRecord";
}

// -- The external name for the type (text representation)

NAString SQLRecord::getTypeSQLname(NABoolean terse) const
{
  if (terse)
    return "SQLRecord";

  NAString name = "SQLRecord(" + this->elementType_->getTypeSQLname(terse);

  const SQLRecord *p = this->restOfRecord_;

  while (p)
  {
    name += "," + p->elementType_->getTypeSQLname(terse);
    p = p->restOfRecord_;
  }
  name += ")";

  return name;
}

// ***********************************************************************
//
// Type synthesis for binary operators
//
// ***********************************************************************

const NAType* SQLRecord::synthesizeType(enum NATypeSynthRuleEnum synthRule,
					const NAType& operand1,
					const NAType& operand2,
					CollHeap* h,
					UInt32 *flags) const
{
  //
  // If the second operand's type synthesis rules have higher precedence than
  // this operand's rules, use the second operand's rules.
  //
  if (operand2.getSynthesisPrecedence() > getSynthesisPrecedence())
    return operand2.synthesizeType(synthRule, operand1, operand2, h, flags);
  //
  // If either operand is not boolean, the expression is invalid.
  //
  if ((operand1.getTypeQualifier() != NA_RECORD_TYPE) ||
      (operand2.getTypeQualifier() != NA_RECORD_TYPE))
    return NULL;

  // To be done
  return NULL;
} // synthesizeType()

// ---------------------------------------------------------------------
// Are the two types compatible for comparison or assignment?
// ---------------------------------------------------------------------
NABoolean SQLRecord::isCompatible (const NAType& other, UInt32 * flags) const
{
  return FALSE;
}

// ---------------------------------------------------------------------
// A method which tells if a conversion error can occur when converting
// a value of this type to the target type.
// This method is a stub and so just returns true for now.
// ---------------------------------------------------------------------
NABoolean SQLRecord::errorsCanOccur(const NAType& target, NABoolean lax) const
{
  return TRUE;
}

// ***********************************************************************
//
//  SQLRowset: the Rowset array data type. 
//             This is mainly for host variables of Rowset type. Other than
//             that, rowsets would be very much like arrays.
//
//  The name SQLArray is not used to avoid a future conflict with a real
//  database array type such as collection. In addition, we are trying to
//  avoid any confusion with scalar indexed host variables which are 
//  supported by SQL/MX.
// ***********************************************************************

SQLRowset::SQLRowset(NAType *elementType, Lng32 maxNumElements, 
                     Lng32 numElements,NAMemory * heap) : 
  NAType(LiteralRowset
         ,NA_ROWSET_TYPE
         ,elementType->getNominalSize()
         ,elementType->supportsSQLnull()
         ), // We may need to pass Data Alignment
  elementType_(elementType),
  maxNumElements_(maxNumElements),
  numElements_(numElements),
  useTotalSize_(0)
{
} // SQLRowset::SQLRowset()

NABoolean SQLRowset::isCompatible (const NAType& other, UInt32 * flags) const
{

  // Both types must be Rowsets of the same element type and dimensions
  if (NAType::isCompatible(other, flags)) { 
    if (other.getTypeQualifier() == NA_ROWSET_TYPE) { 
      const SQLRowset &otherRowset = *(const SQLRowset *)&other;
      if (elementType_->isCompatible(*otherRowset.getElementType()) &&
        maxNumElements_ == otherRowset.getMaxNumElements()) {
        return TRUE;
      }
    }
  }
  return FALSE;
} // SQLRowset::isCompatible ()

NABoolean SQLRowset::operator==(const NAType& other) const
{
  // Both types must be Rowsets of the same element type and dimensions
  if (isCompatible(other)) { 
    const SQLRowset &otherRowset = *(const SQLRowset *)&other;
    if (numElements_ == otherRowset.getNumElements()) {
      return TRUE;
    }
  }

  return FALSE;
} // SQLRowset::operator==()

NABoolean SQLRowset::errorsCanOccur(const NAType& target, NABoolean lax) const
{
  return TRUE;
}

NAString SQLRowset::getSimpleTypeName() const
{
  const NAString &sname = NAType::getTypeName();
  return sname;
}

NAString SQLRowset::getTypeSQLname(NABoolean terse) const
{
  NAString rName = getSimpleTypeName();
  if (!terse)
    {
      char size[20];

      sprintf(size, " %d ", maxNumElements_);
      rName += size;
      rName += elementType_->getTypeSQLname(terse);
    }
  return rName;
} // SQLRowset::getTypeSQLname()

short SQLRowset::getFSDatatype() const 
{
  //return REC_BYTE_F_ASCII;
  return elementType_->getFSDatatype();
}

void SQLRowset::print(FILE* ofd, const char* indent)
{
  NAType::print(ofd, indent);
  
#ifdef TRACING_ENABLED
  fprintf(ofd,"%s elementType as follows\n",
          indent);
  getElementType()->print(ofd, indent);
  fprintf(ofd,"%s maximum num of elements %d, actual num of elements %d\n",
          indent, maxNumElements_, numElements_);
#endif
} // SQLRowset::print()

NAType * SQLRowset::newCopy(CollHeap* h) const
{
  return new(h) SQLRowset(elementType_, maxNumElements_, numElements_,h);
}

NAType * SQLRowset::getElementType() const
{
  return elementType_;
}

// Both operands expected to be rowsets.
const NAType* SQLRowset::synthesizeType(enum NATypeSynthRuleEnum synthRule,
					const NAType& operand1,
					const NAType& operand2,
					CollHeap* h,
					UInt32 *flags) const
{
  // Both must be rowset types
  if ((operand1.getTypeQualifier() != NA_ROWSET_TYPE) ||
      (operand2.getTypeQualifier() != NA_ROWSET_TYPE)) {
    return NULL;
  }
  
  //
  // Check that the operands are compatible.
  //
  if (NOT operand1.isCompatible(operand2)) {
    // 4041 comparison between these two types is not allowed
    return NULL;
  }

  // Operations currently allowed in rowsets
  if (synthRule != SYNTH_RULE_UNION && synthRule != SYNTH_RULE_ADD &&
      synthRule != SYNTH_RULE_SUB && synthRule != SYNTH_RULE_MUL &&
      synthRule != SYNTH_RULE_DIV) {
    return NULL;
  }

  return new(h) SQLRowset(elementType_, maxNumElements_, numElements_,h);
}

Lng32 SQLRowset::getNumElements() const
{
  return numElements_;
}

Lng32 SQLRowset::setNumElements(Lng32 numElements)
{
  return numElements_ = numElements;
}

Lng32 SQLRowset::getMaxNumElements() const
{
  return maxNumElements_;
}

// ***********************************************************************
//
//  SQLUnknown : The unknown data type (for params and NULLs)
//
// ***********************************************************************

NAType * SQLUnknown::newCopy(CollHeap* h) const
{
  return new(h) SQLUnknown(*this,h);
}

// ---------------------------------------------------------------------
// A method which tells if a conversion error can occur when converting
// a value of this type to the target type.
// This method is a stub and so just returns true for now.
// ---------------------------------------------------------------------
NABoolean SQLUnknown::errorsCanOccur(const NAType& target, NABoolean lax) const
{
  return TRUE;
}
