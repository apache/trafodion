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
#ifndef NATYPE_H
#define NATYPE_H
/* -*-C++-*-
**************************************************************************
*
* File:         NAType.h
* Description:  The base class for all (abstract) data types
* Created:      4/27/94
* Language:     C++
*
*
*
*
**************************************************************************
*/

#include "BaseTypes.h"
#include "Collections.h"
#include "NAStringDef.h"
#include "SQLTypeDefs.h"
#include "dfs2rec.h"
#include "charinfo.h"
#include "ComSmallDefs.h"

// forward ref for methods implemented in ../optimizer/SynthType.cpp
class ItemExpr;

#define ALIGN(offset, alignment) \
  ((offset > 0) ? (((offset - 1)/alignment) + 1) * alignment : offset)

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class NAType;

// ***********************************************************************
// Literals for the maximum and minimum data values for numeric data types
// with precision. These values are used to supply low and high values for
// keys in determining partition ranges etc.
// ***********************************************************************
#define recBin16sMax	077777
#define recBin16sMin	0100000
#define recBin16uMax	0177777
#define recBin16uMin	00

#define recBin32sMax	017777777777
#define recBin32sMin	020000000000
#define recBin32uMax	037777777777
#define recBin32uMin	00

#define recBin64sMax	0777777777777777777777LL
#define recBin64sMin	01000000000000000000000LL

#define recFloat32Min	037777777777
#define recFloat32Max	017777777777

#define recFloat64Min	01777777777777777777777LL
#define recFloat64Max	0777777777777777777777LL

// ***********************************************************************
//
// Enumerated lists for built-in data types that are supported.
//
// ***********************************************************************
enum NABuiltInTypeEnum
{
  NA_BOOLEAN_TYPE
, NA_CHARACTER_TYPE
, NA_DATETIME_TYPE
, NA_INTERVAL_TYPE
, NA_NUMERIC_TYPE
, NA_UNKNOWN_TYPE
, NA_USER_SUPPLIED_TYPE
, NA_RECORD_TYPE
, NA_ROWSET_TYPE
, NA_LOB_TYPE
};

// ***********************************************************************
//
// Enumeration of different type synthesis methods that are supported.
// Each item in the list corresponds to some operation that is defined
// for the datatype.
//
// ***********************************************************************
enum NATypeSynthRuleEnum
{
  SYNTH_RULE_UNION
, SYNTH_RULE_PASS_THRU_NUM
, SYNTH_RULE_ADD
, SYNTH_RULE_SUB
, SYNTH_RULE_MUL
, SYNTH_RULE_DIV
, SYNTH_RULE_EXP
, SYNTH_RULE_CONCAT
};

// ***********************************************************************
// Minimum and maximum constants
// ***********************************************************************
enum NATypeExtrema
{
  MAX_NUMERIC_PRECISION	= MAX_HARDWARE_SUPPORTED_SIGNED_NUMERIC_PRECISION
};

enum NATypeDefaultVals
{
  DEFAULT_CHARACTER_LENGTH = 32
};

// ***********************************************************************
//
//  NAType : An abstract data type (ADT)
//
// ***********************************************************************
class NAType : public NABasicObject
{
public:

  // copy ctor
  NAType (const NAType & rhs, NAMemory * h) ;

  // ---------------------------------------------------------------------
  // Constructor function (storage size and alignment needs to be
  // specified for data field only, null indicator and variable len
  // field are handled automatically)
  // ---------------------------------------------------------------------
  NAType (NAMemory *h, const NAString &   adtName,
          NABuiltInTypeEnum  ev,
          Lng32               dataStorageSize,
          NABoolean          nullable = FALSE,
          Lng32               SQLnullHdrSize = 0,
          NABoolean          variableLength = FALSE,
          Lng32               lengthHeaderSize = 0,
          Lng32               dataAlignment = 1
        );

  // ---------------------------------------------------------------------
  // Methods for comparing if two ADT definitions are equal.
  // ---------------------------------------------------------------------
  virtual NABoolean operator==(const NAType& other) const;

  // Identical as NAType::operator==() except null attribute is not compared.
  virtual NABoolean equalIgnoreNull(const NAType& other) const;

  // Identical as NAType::operator==() except length attribute is not compared.
  virtual NABoolean equalIgnoreLength(const NAType& other) const;

  // Checks NATypes to see if they are equal just like the operator==
  // except for the SQLnullFlag_  flags. For the SQLnullFlag_ it
  // checks to see if it's physical.
  NABoolean equalPhysical(const NAType& other) const;

  // ---------------------------------------------------------------------
  // Are the two types compatible?
  // ---------------------------------------------------------------------
  virtual NABoolean isCompatible(const NAType& other, UInt32 * flags = NULL) const
				  { return qualifier_ == other.qualifier_; }

  // ---------------------------------------------------------------------
  // Are the two types comparable (a more stringent test)?
  // ---------------------------------------------------------------------
  enum { EmitErrNever = FALSE, EmitErrAlways = TRUE, EmitErrIfAnyChar = 991 };
  #ifndef CLI_SRL
  virtual NABoolean isComparable(const NAType &other,
                                 ItemExpr *parentOp,
				 Int32 emitErr = EmitErrAlways,
                                 UInt32 * flags = NULL) const;
  #endif

  // ---------------------------------------------------------------------
  // Can a conversion error occur when converting from this datatype
  // to a given target datatype?
  // ---------------------------------------------------------------------
  virtual NABoolean errorsCanOccur(const NAType& target,
                                   NABoolean lax=TRUE) const = 0;

  // ---------------------------------------------------------------------
  // Get the enum of the type.
  // ---------------------------------------------------------------------
  NABuiltInTypeEnum getTypeQualifier() const { return qualifier_; }

  // ---------------------------------------------------------------------
  // Get the textual description of the type.
  // ---------------------------------------------------------------------
  const NAString& getTypeName() const { return typeName_; }

  void getTypeSQLnull(NAString& ns, NABoolean ignore = FALSE) const;

  virtual NAString getTypeSQLname(NABoolean terse = FALSE) const;

  // For data type like BYTEINT, getTypeName() returns "SMALLINT"
  // and getDisplayDataType() returns "BYTEINT"
  const NAString& getDisplayDataType() const { return displayDataType_; }
  void setDisplayDataType(const NAString& dt) { displayDataType_ = dt; }

  // ---------------------------------------------------------------------
  // Get a simpler textual description of the type.
  // ---------------------------------------------------------------------
  virtual NAString getSimpleTypeName() const { return getTypeSQLname(); }

  // ---------------------------------------------------------------------
  // Get the filesystem datatype value (from DFS2REC) for this type.
  // Used by expressions, filesystem and dp2.
  // ---------------------------------------------------------------------
  virtual short getFSDatatype() const;

  virtual Lng32 getPrecision() const;

  virtual Lng32 getMagnitude() const;

  virtual Lng32 getScale() const;

  // the expressions code overlays precision and max chars, ...
  virtual Lng32 getPrecisionOrMaxNumChars() const;

  // ...as well as scale and charset
  virtual Lng32 getScaleOrCharset() const;

  virtual CharInfo::CharSet getCharSet() const;	
  //    { return CharInfo::UnknownCharSet; };
 
  // The following method is redefined for the SQLMPDatetime and SQLInterval classes.
  virtual NABoolean isSupportedType()  const {return TRUE;};

  virtual NABoolean isSkewBusterSupportedType() const;

  // ---------------------------------------------------------------------
  // The nominal size is number of BYTES required for storing the
  // actual bit pattern that represents the data.
  // The total size includes any overheads that may be associated
  // for representing certain kinds of data, e.g., null values or
  // variable length data (total size >= nominal size).
  // ---------------------------------------------------------------------
  Lng32 getNominalSize() const	{ return dataStorageSize_; }
  void setNominalSize(Int32 newSize) { dataStorageSize_ = newSize; }

  NABoolean isValid() const	{ return dataStorageSize_ > 0; }

  void makeInvalid()		{ dataStorageSize_ = 0; }

  virtual Lng32 getTotalSize() const;

  virtual Lng32 getTotalAlignedSize() const;

  // ---------------------------------------------------------------------
  // Check if this ADT allows SQL null values.
  // If physical nulls are not supported, then logical nulls are not either
  // (if physicalNulls is False, then logicalNulls is ignored).
  // ---------------------------------------------------------------------
  enum SupportsSQLnull
  { ALLOWS_NULLS, NOT_NULL_DROPPABLE, NOT_NULL_NOT_DROPPABLE };

  NABoolean supportsSQLnullLogical() const
  { return SQLnullFlag_ == ALLOWS_NULLS; }

  NABoolean supportsSQLnullPhysical() const
  { return SQLnullFlag_ != NOT_NULL_NOT_DROPPABLE; }

  NABoolean supportsSQLnull() const
  { return supportsSQLnullPhysical(); }		// for historical reasons

  void setNullable(NABoolean physicalNulls, NABoolean logicalNulls = TRUE);

  void setNullable(const NAType &src)
  { setNullable(src.supportsSQLnullPhysical(), src.supportsSQLnullLogical()); }

  // temporarily set SQLnullFlag_ to ALLOWS_NULLS
  void setSQLnullFlag() { SQLnullFlag_ = ALLOWS_NULLS; }

  // reset SQLnullFlag_ to NOT_NULL_NOT_DROPPABLE
  void resetSQLnullFlag() { SQLnullFlag_ = NOT_NULL_NOT_DROPPABLE; }

  Lng32 getSQLnullHdrSize() const { return SQLnullHdrSize_; }

  void resetSQLnullHdrSize() { SQLnullHdrSize_= 0; }

  // ---------------------------------------------------------------------
  // Check if this ADT allows values to be of variable length.
  // ---------------------------------------------------------------------
  NABoolean isVaryingLen() const { return varLenFlag_; }

  Lng32 getVarLenHdrSize() const { return lengthHdrSize_; }

  // ---------------------------------------------------------------------
  // Get the alignments, the total size of the prefix (null indicator +
  // variable length field) including filler bytes, and the size of the
  // type if it is an array element
  // ---------------------------------------------------------------------
  Lng32 getTotalAlignment() const { return totalAlignment_; }
  Lng32 getDataAlignment() const { return dataAlignment_; }
  Lng32 getPrefixSize() const;
  Lng32 getPrefixSizeWithAlignment() const ;
  Lng32 getArrayElementSize() const;

  // ---------------------------------------------------------------------
  // Methods that return the binary form of the minimum and the maximum
  // representable values.
  // ---------------------------------------------------------------------
  virtual void minRepresentableValue(void*, Lng32*,
				     NAString ** stringLiteral = NULL,
				     CollHeap* h=0) const;
  virtual void maxRepresentableValue(void*, Lng32*,
				     NAString ** stringLiteral = NULL,
				     CollHeap * h=0) const;
  inline void minMaxRepresentableValue(void* buf,
                                       Lng32* bufLen,
                                       NABoolean isMax,
                                       NAString ** stringLiteral = NULL,
                                       CollHeap * h=0) const
  { if (isMax) maxRepresentableValue(buf, bufLen, stringLiteral, h);
    else       minRepresentableValue(buf, bufLen, stringLiteral, h); }

  virtual double getMinValue() const { return 0; };
  virtual double getMaxValue() const { return 0; };

  // ---------------------------------------------------------------------
  // create an SQL literal (in UTF-8) from a binary buffer
  // returns whether it was able to create a valid SQL literal
  // This assumes "buf" to be in the format of an already decoded
  // key value. That's similar to SQL/ARK exploded format, except
  // that there are no fillers for alignment, NULL indicator. Var
  // len indicator and value appear adjacent to each other, unaligned.
  // ---------------------------------------------------------------------
  virtual NABoolean createSQLLiteral(const char * buf,       // in
                                     NAString *&sqlLiteral,  // out
                                     NABoolean &isNull,      // out
                                     CollHeap *h) const;     // in/out

  // compute the largest smaller value than v
  virtual double computeLSV(double v) const 
  { return (roundTripConversionToDouble())? v-1 : v; }

  virtual NAString* convertToString(double v, NAMemory * h=0) const;

  virtual NABoolean computeNextKeyValue(NAString &stringLiteral) const;

  // ---------------------------------------------------------------------
  //  Method that returns the encoded form (in floating point) for a
  //  given value.  This value will then be used by the optimizer
  //  for estimations and selectivity computation. NOTE: this is
  //  NOT the key encoding used in DP2 to access rows in key-sequenced
  //  tables.
  // ---------------------------------------------------------------------
  virtual double encode (void*) const { return -1; }

  // --------------------------------------------------------------------
  // Compute the length of the key encoding for this type.
  //
  // NOTE: don't confuse the encoding of types into floating point
  // values (approximate, used for histogram statistics and other
  // heuristical decisions in the optimizer) and the key encoding
  // (an order-preserving, non-reducing encoding into an unsigned
  // array of bytes).
  // --------------------------------------------------------------------
  virtual Lng32 getEncodedKeyLength() const;

  // --------------------------------------------------------------------
  // Methods that returns TRUE if encoding for comparison is needed.
  // Datatypes are encoded as a string of chars so they could be
  // compared using string comparison (memcmp). Used to compare keys
  // and other places where the caller doesn't want to be concerned
  // about the datatype.
  // Note that this function has nothing to do with the 'encode'
  // method.
  // --------------------------------------------------------------------
  virtual NABoolean isEncodingNeeded() const { return FALSE; }

  // ---------------------------------------------------------------------
  // A function for synthesizing a nullable type from this type.
  // ---------------------------------------------------------------------
  const NAType* synthesizeNullableType(NAMemory * h) const;

  // enums for the 'flags' parameter to synthesizeType method.
  enum SynthesizeTypeFlags
  {
    // indicates that the max precision of an exact numeric bi arith
    // operation is to be limited to MAX_NUMERIC_PRECISION.
    LIMIT_MAX_NUMERIC_PRECISION = 0x0001,

    // indicates that special1 mode is on. Special handling of
    // syntax, expressions is done in some cases.
    // See other parts of code for what all is done in this case.
    MODE_SPECIAL_1 = 0x0002,

    // result with scale is requested to be rounded.
    ROUND_RESULT = 0x0008,

    // if result of arith operation is actually rounded.
    RESULT_ROUNDED = 0x0010,

    // if the result of union synthesis should always be binary 
    MAKE_UNION_RESULT_BINARY = 0x0020,

    // if the result of IfThenElse or Union synthesis should be a varchar
    MAKE_RESULT_VARCHAR = 0x0040,

    // indicates that special4 mode is on. Special handling of
    // syntax, expressions is done in some cases.
    // See other parts of code for what all is done in this case.
    MODE_SPECIAL_4 = 0x0080,

    // enable incompatibe operations.
    ALLOW_INCOMP_OPER = 0x0100
  };

  // ---------------------------------------------------------------------
  // A virtual function for synthesizing the type of a binary operator.
  // ---------------------------------------------------------------------
  virtual const NAType* synthesizeType(enum NATypeSynthRuleEnum synthRule,
                                       const NAType& operand1,
                                       const NAType& operand2,
				       NAMemory * h,
				       UInt32 *flags = NULL) const;

  // ---------------------------------------------------------------------
  // A virtual function for synthesizing the type of a ternary operator.
  // ---------------------------------------------------------------------
  virtual const NAType* synthesizeTernary(enum NATypeSynthRuleEnum synthRule,
                                          const NAType& operand1,
                                          const NAType& operand2,
                                          const NAType& operand3,
					  NAMemory * h=0) const;

  // ---------------------------------------------------------------------
  // The following table contains the order of precedence for the type
  // synthesis rules.  Precedence is listed from lowest to highest.  For
  // example, if a binary operator has operands of type SQLNumeric
  // and SQLFloat, the result type will be computed by
  // SQLFloat::synthesizeType, because SQLFloat has higher precedence.
  // ---------------------------------------------------------------------
  enum SynthesisPrecedence {
    SYNTH_PREC_DEFAULT
  , SYNTH_PREC_BIG_NUM
  , SYNTH_PREC_FLOAT
  , SYNTH_PREC_INTERVAL		// > Numeric (ANSI 6.15, multiplication)
  , SYNTH_PREC_DATETIME		// > Interval (ANSI 6.14, addition)
  , SYNTH_PREC_BINARY
  };

  virtual NAType::SynthesisPrecedence getSynthesisPrecedence() const
  {
    return SYNTH_PREC_DEFAULT;
  }

  //is this type numeric or non-numeric
  virtual NABoolean isNumeric() const;
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

  // --------------------------------------------------------------------
  // If this datatype is supported externally only, then this function
  // returns TRUE. External datatypes are the one that could be declared
  // in an application, but they are not supported internally (that is,
  // it cannot be used in a CREATE TABLE statement to create a column).
  // An example is the ANSI VARCHAR datatype, which is a null terminated
  // string and supported in programs only. Such datatype values are
  // converted to an equivalent internal datatype when inputing the
  // value into the executor. ANSI VARCHAR is converted to SQL VARCHAR
  // (which is length bytes followed by data). Similarly, at output
  // time, the internal datatype value is converted to the external
  // datatype.
  // ---------------------------------------------------------------------
  virtual NABoolean isExternalType() const {return FALSE;};

  //is this type a blob/clob
  virtual NABoolean isLob() const {return FALSE;};
  

  // returns the equivalent internal datatype
  virtual NAType * equivalentType(NAMemory * h=0) const {return NULL;};

  // --------------------------------------------------------------------
  // If this is a software datatype like Large Decimal or Big Num, this
  // routine returns TRUE. Currently (Jan '03) an exact numeric literal 
  // that is greater than 18 digits is not supported in both R2 And R 1.8.X.
  // Internally we make these numeric values to be software datatypes
  // (large decimal or big num)but that is only done for temporary results. 
  // User entered numeric values, either as literals or params or hostvars 
  // should not exceed 18 digits. Internally if a user defined variable is
  // converted to these software datatypes, the optimizer converts it to
  // its closest equivalent type. The routine closestEquivalentExternalType()
  // returns this type. For example, if a parameter is coerced to either 
  // big num or large decimal, then the optimizer actually sets it to a
  // SQLNumeric with maximum precision.
  // 
  // --------------------------------------------------------------------
  virtual NAType * closestEquivalentExternalType (NAMemory* heap=0) const 
                               {return this->newCopy(heap);}

  // ---------------------------------------------------------------------
  // Print function for debugging
  // ---------------------------------------------------------------------
  virtual void print(FILE* ofd = stdout, const char* indent = DEFAULT_INDENT);

  // ---------------------------------------------------------------------
  // Display function for debugging
  // ---------------------------------------------------------------------
  void display();

  // ---------------------------------------------------------------------
  // A method for generating the hash key.
  // SQL builtin types should return getTypeSQLName()
  // ---------------------------------------------------------------------
  virtual NAString *getKey(NAMemory * h=0) const;

  // ---------------------------------------------------------------------
  // A pure virtual function to return a copy of the type.
  // ---------------------------------------------------------------------
  virtual NAType *newCopy(NAMemory * h=0) const = 0;

  // Gets the length that a given data type would use in the display tool
  virtual Lng32 getDisplayLength(Lng32 datatype,
 		        Lng32 length,
		        Lng32 precision,
		        Lng32 scale,
                        Lng32 heading_len) const;

  virtual Lng32  getDisplayLength() const;

  // A helper function.
  // This method returns a text representation of the datatype
  // based on the datatype information input to this method/
  static short convertTypeToText(char * text,	/* OUTPUT */
				 Lng32 fs_datatype,  // all other vars: INPUT
				 Lng32 length,
				 Lng32 precision,
				 Lng32 scale,
				 rec_datetime_field datetimestart,
				 rec_datetime_field datetimeend,
				 short datetimefractprec,
				 short intervalleadingprec,
				 short upshift,
				 short caseinsensitive,
                                 CharInfo::CharSet charSet,
                                 CharInfo::Collation collation,
                                 const char * displaydatatype,
				 short displayCaseSpecific = 0);

  short getMyTypeAsText(NAString * outputStr,  // output
			NABoolean addNullability = TRUE,
                        NABoolean addCollation = TRUE) const;

  short getMyTypeAsHiveText(NAString * outputStr) const;  // output

  // used for query caching
  Lng32 getSize() const;
  Lng32 hashKey() const;
  NABoolean amSafeToHash() const;

  // 
  // used for skew buster. For SQLNumeric data type, we do not want to
  // compute hash in FrequentValue objects, since 200.00 and 200.000 will have different
  // hash values. Yet they are equal. Since hash values are used to determine the 
  // equal-ness of two FrequentValues, we cannot compute hash for SQLNumeric when the 
  // intended use of the hash is to store in FrequentValue objects.  
  //
  NABoolean useHashInFrequentValue() const;

  NABoolean useHashRepresentation() const;

  virtual NABoolean roundTripConversionToDouble() const { return FALSE; };

  // used during expr generation to indicate if conversion to/from otherFsType
  // is supported by expr evaluator.
  virtual NABoolean expConvSupported
  (const NAType &otherNAType) const { return TRUE; }

  static NAType* getNATypeForHive(const char* hiveType, NAMemory* heap);

protected:

  // ---------------------------------------------------------------------
  // Each data type has a characteristic size, expressed in bytes, for
  // representing a value. It does not include any extra headers that
  // may be required for indicating that a value is null or of a
  // variable length.
  // ---------------------------------------------------------------------
  Lng32 dataStorageSize_;

  // ---------------------------------------------------------------------
  // A flag that indicates whether the ADT supports SQL null values.
  // ---------------------------------------------------------------------
  SupportsSQLnull SQLnullFlag_;

  Lng32 SQLnullHdrSize_;		// physical attribute

  // ---------------------------------------------------------------------
  // A flag that indicates that dataStorageSize_ is the maximum number of
  // bytes of storage required for representing an instance of this type.
  // The actual size of the storage used is stored together with each
  // value instance.
  // ---------------------------------------------------------------------
  NABoolean varLenFlag_;

  Lng32 lengthHdrSize_;

  // ---------------------------------------------------------------------
  // A number that indicates how the data needs to be aligned. The
  // data alignment shows how the actual data needs to be aligned, the
  // total alignment is the combination of the alignments of null
  // indicator, var length, and data field. It has one of the values of
  // 1, 2, 4, or 8. A value of 1 means the field needs no alignment.
  // ---------------------------------------------------------------------
  Lng32 totalAlignment_;
  Lng32 dataAlignment_;

private:

  NAType();	// default ctor, not implemented, not callable

  // ---------------------------------------------------------------------
  // Each ADT has a name.
  // ---------------------------------------------------------------------
  NAString typeName_;

  // ---------------------------------------------------------------------
  // The generic class to which the data type belongs.
  // ---------------------------------------------------------------------
  NABuiltInTypeEnum qualifier_;

  // ---------------------------------------------------------------------
  // For data type like BYTEINT which maps to SMALLINT
  // ---------------------------------------------------------------------
  NAString displayDataType_;

}; // class NAType


#endif /* NATYPE_H */
