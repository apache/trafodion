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
#ifndef EXP_CLAUSE_H
#define EXP_CLAUSE_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         <file>
 * Description:  
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ExpError.h"
#include "exp_attrs.h"

#include "Collections.h"
#include "ComSpace.h"
#include "Int64.h"
#include "OperTypeEnum.h"
#include "ExpAtp.h"
#include "exp_expr.h"

// Forware external declaractions
//
class PCILink;

#define ADJUST(offset, alignment) ((offset > 0) ?    \
				   (((offset - 1)/alignment) + 1) * alignment : \
				   offset);

class ex_globals;
typedef NABasicPtrTempl<ex_globals>   ExGlobalsPtr; 

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ex_clause;


/////////////////////////////////////////////////
// Class ex_clause
/////////////////////////////////////////////////
class ex_clause : public NAVersionedObject {
public:
  // Possible types of clauses
  // (these codes must remain invariant across future versions)
  enum clause_type  {
    CLAUSE_ANCHOR            =-1,
    AGGREGATE_TYPE           = 1,
    ARITH_TYPE               = 2,
    BOOL_RESULT_TYPE         = 3,
    BOOL_TYPE                = 4,
    BRANCH_TYPE              = 5,
    COMP_TYPE                = 6,
    CONV_TYPE                = 7,
    FUNCTION_TYPE            = 8,
    INOUT_TYPE               = 9,
    LIKE_TYPE                =10,
    NOOP_TYPE                =11,
    UN_LOGIC_TYPE            =12,
    ARITH_SUM_TYPE           =13,
    ARITH_COUNT_TYPE         =14,
    MATH_FUNCTION_TYPE       =15,
    // we use most of the above enumerated constants as ex_clause classID's.
    // we also use the following enumerated constants as classID's of 
    // ex_clause derived classes.
    // NAVersionedObject::driveUnpack relies on classID to find and set the 
    // unpacked object's virtual function table pointer; so, classID must
    // be a  "flattened" unique class id. That is, no two ex_clause-derived 
    // classes can have the same classID.
    LIKE_CLAUSE_CHAR_ID      =16,
    LIKE_CLAUSE_DOUBLEBYTE_ID   =17,
    FUNC_ASCII_ID            =18,
    FUNC_CHAR_ID             =19,
    FUNC_CHAR_LEN_ID         =20,
    FUNC_CHAR_LEN_DOUBLEBYTE_ID =21,
    FUNC_CVT_HEX_ID          =22,
    FUNC_OCT_LEN_ID          =23,
    FUNC_POSITION_ID         =24,
    FUNC_POSITION_DOUBLEBYTE_ID =25,
    FUNC_CONCAT_ID           =26,
    FUNC_REPEAT_ID           =27,
    FUNC_REPLACE_ID          =28,
    FUNC_SUBSTR_ID           =29,
    FUNC_SUBSTR_DOUBLEBYTE_ID=30,
    FUNC_TRIM_ID             =31,
    FUNC_TRANSLATE_ID        =32,
    FUNC_TRIM_DOUBLEBYTE_ID  =33,
    FUNC_LOWER_ID            =34,
    FUNC_UPPER_ID            =35,
    FUNC_UPPER_UNICODE_ID    =36,
    FUNC_LOWER_UNICODE_ID    =37,
    FUNC_CURRENT_TIMESTAMP_ID=38,
    FUNC_ENCODE_ID           =39,
    FUNC_EXPLODE_VARCHAR_ID  =40,
    FUNC_HASH_ID             =41,
    FUNC_HASHCOMB_ID         =42,
    FUNC_BITMUX_ID           =43,
    FUNC_REPLACE_NULL_ID     =44,
    FUNC_MOD_ID              =45,
    FUNC_MASK_ID             =47,
    FUNC_ABS_ID              =48,
    FUNC_BOOL_ID             =49,
    FUNC_CONVERTTIMESTAMP_ID =50,
    FUNC_DATEFORMAT_ID       =51,
    FUNC_DAYOFWEEK_ID        =52,
    FUNC_EXTRACT_ID          =53,
    FUNC_JULIANTIMESTAMP_ID  =54,
    FUNC_EXEC_COUNT_ID       =55,
    FUNC_CURR_TRANSID_ID     =56,
    FUNC_USER_ID             =57,
    FUNC_ANSI_USER_ID        =58,
    FUNC_VARIANCE_ID         =59,
    FUNC_STDDEV_ID           =60,
    FUNC_RAISE_ERROR_ID      =61,
    FUNC_RANDOMNUM_ID        =62,
    FUNC_HDPHASH_ID          =63,
    FUNC_HDPHASHCOMB_ID      =64,
    FUNC_PROGDISTRIB_ID      =65,
    FUNC_UNPACKCOL_ID        =66,
    FUNC_PACK_ID             =67,
    FUNC_RANGE_LOOKUP_ID     =68,
    FUNC_MATH_ID             =69,
    AGGR_ONE_ROW_ID          =70,
    AGGR_ANY_TRUE_MAX_ID     =71,
    AGGR_MIN_MAX_ID          =72,
    FUNC_OFFSET_ID           =73,
    FUNC_RAND_SELECTION_ID   =74,
    FUNC_ROWSETARRAY_SCAN_ID =75,
    FUNC_ROWSETARRAY_ROW_ID  =76,
    FUNC_ROWSETARRAY_INTO_ID =77,
    FUNC_UNIQUE_EXECUTE_ID_ID=78,
    FUNC_INTERNALTIMESTAMP_ID=79,
    FUNC_GET_TRIGGERS_STATUS_ID=80,
    FUNC_GET_BIT_VALUE_AT_ID =81,
    FUNC_PROGDISTKEY_ID      =82,
    FUNC_PAGROUP_ID          =83,
    FUNC_SHIFT_ID            =84,
    FUNC_GENERICUPDATEOUTPUT_ID=85,
    FUNC_IS_BITWISE_AND_TRUE =86,
    FUNC_NULLIFZERO          =87,
    FUNC_NVL                 =88,
    FUNC_HASH2_DISTRIB_ID    =89,
    FUNC_HEADER              =92,
    FUNC_QUERYID_EXTRACT     =93,
    FUNC_BIT_OPER_ID         =94,
    FUNC_TOKENSTR_ID         =95,
    LOB_TYPE                 =96,
    LOB_INSERT               =97,
    LOB_SELECT               =98,
    LOB_DELETE               =99,
    LOB_UPDATE               =100,
    LOB_CONVERT              =101,
    LOB_CONVERTHANDLE        =102,
    LOB_LOAD                 =103,
    LOB_FUNC_TYPE            =104,
    LOB_FUNC_SUBSTR          =105,
    FUNC_HIVEHASH_ID         =106,
    FUNC_HIVEHASHCOMB_ID     =107,
    FUNC_UNIQUE_ID           = 108,
    FUNC_HBASE_COLUMN_LOOKUP = 109,
    FUNC_HBASE_COLUMNS_DISPLAY   = 110,
    FUNC_HBASE_COLUMN_CREATE = 111,
    FUNC_CAST_TYPE           = 112,
    FUNC_SEQUENCE_VALUE      = 113,
    FUNC_PIVOT_GROUP         = 114,
    FUNC_ROWNUM              = 115,
    FUNC_HBASE_TIMESTAMP     = 116,
    FUNC_HBASE_VERSION       = 117,
    FUNC_ISIP_ID             = 118,
    FUNC_INETATON_ID         = 119,
    FUNC_INETNTOA_ID         = 120,
    AGGR_GROUPING_ID         = 121,
    FUNC_CRC32_ID            = 122,
    FUNC_MD5_ID              = 123,
    FUNC_SHA1_ID             = 124,
    FUNC_SHA2_ID             = 125,
    FUNC_SOUNDEX_ID          = 126,
    REGEXP_CLAUSE_CHAR_ID    = 127,
    FUNC_JSON_ID             = 128,
    FUNC_AES_ENCRYPT         = 129,
    FUNC_AES_DECRYPT         = 130,
    FUNC_REVERSE_ID          = 131,
    FUNC_BASE64_ENC_DEC      = 132,
    FUNC_SLEEP_ID            = 133,
    FUNC_UNIX_TIMESTAMP_ID   = 134,
    FUNC_SPLIT_PART_ID       = 135
  };

  // max number of operands (including result) in a clause.
  // Result occupies the first position(index = 0) in the
  // Attributes array 'op'. See exp_expr::eval in file
  // exp_eval.C for details on use of these enums.
  enum ClauseConstants {
    MAX_OPERANDS         = 5,
    MAX_TEMP_OPERAND_LEN = 64,
    NULL_HASH            = 666654765,
    SQL_INT32_MAX        = 2147483647
  };
  
  // Construction, Destruction
  //
  ex_clause() : NAVersionedObject(CLAUSE_ANCHOR), 
    nextClause_(NULL), op_(NULL) {};
  ex_clause(clause_type type_,
		       OperatorTypeEnum operType_,
		       short num_operands_,
		       Attributes ** op_,
		       Space * space_);
  ~ex_clause();

  // Bookkeeping Accessors
  //
  void generateClause(char *pt);
  inline clause_type getType();
  inline void setType(clause_type t);

  inline OperatorTypeEnum getOperType()
  {return (OperatorTypeEnum)operType_;};

  // PCode Methods
  //
  // getPCILink - returns the link to the PCI List for this clause
  //
  // setPCIList - sets the link to the PCI List for this clause
  //
  // pCodeGenerate - virtual functions used to generate the PCode for 
  // a clause. If not redefined, the default implementations generates
  // PCode to set up for and call the clause->eval() method.
  //
  inline PCILink *getPCIList() { return pciLink_; };
  inline void setPCIList(PCILink *pciLink) {
    pciLink_ = pciLink; };
  virtual ex_expr::exp_return_type pCodeGenerate(Space *space,
							    UInt32 flags);

  // Null Processing Methods
  //
  // isNullInNullOut - Pure virtual function which returns TRUE if the clause
  // semantics are any NULL input produces a NULL result.
  //
  // isNullRelevant - Pure virtual function which returns TRUE if there
  // is processing for NULLs other than in eval.
  //
  // isAnyInputNullable - Returns TRUE if any of the input operands are
  // nullable.
  //
  // isAnyOutputNullable - Returns TRUE if any of the output operands are
  // nullable.
  //
  // isAnyOperandNullable - Returns TRUE is any of the input or output
  // operands are nullable.
  //
  // useProcessNulls - Returns TRUE is processNulls should be called
  // for this clause.
  //
  // setSpecialNulls - Sets flags to indicate that NULL should compare as
  // bigger than maximum value (for sorting)
  //
  // isSpecialNulls - Return TRUE if NULL should compare bigger than maximum
  // value
  //
  virtual Int32 isNullInNullOut() const 
    { return 1;}; // must be redefined in derived classes
  virtual Int32 isNullRelevant() const
    { return 1;}; // must be redefined in derived classes
  inline Int32 isAnyInputNullable() {
    return flags_ & ANY_INPUT_NULLABLE; };

  inline Int32 isAnyOutputNullable() {
    return flags_ & ANY_OUTPUT_NULLABLE; };
 
  inline Int32 isAnyOperandNullable() { 
    return flags_ & (ANY_INPUT_NULLABLE | ANY_OUTPUT_NULLABLE); };

  inline Int32 useProcessNulls() {
    return flags_ & USE_PROCESS_NULLS; 
  };

  void setProcessNulls() {
    if(isNullRelevant() && (flags_ & (ANY_INPUT_NULLABLE|ANY_OUTPUT_NULLABLE)))
      flags_ |= USE_PROCESS_NULLS;
  };
  void setSpecialNulls() { flags_ |= SPECIAL_NULLS; };
  Int32 isSpecialNulls()   { return (flags_ & SPECIAL_NULLS) != 0; };

  NABoolean noPCodeAvailable() { return (flags_ & NO_PCODE_AVAILABLE) != 0; };
  void setNoPCodeAvailable(NABoolean v) 
  {
    (v ? flags_ |= NO_PCODE_AVAILABLE : flags_ &= ~NO_PCODE_AVAILABLE);
  };

  NABoolean handleIndirectVC() { return (flags_ & HANDLE_INDIRECT_VC) != 0; };
  void setHandleIndirectVC(NABoolean v) 
  {
    (v ? flags_ |= HANDLE_INDIRECT_VC : flags_ &= ~HANDLE_INDIRECT_VC);
  };

  NABoolean forInsertUpdate() { return (flags_ & FOR_INSERT_UPDATE) != 0; };
  void setForInsertUpdate(NABoolean v) 
  {
    (v ? flags_ |= FOR_INSERT_UPDATE : flags_ &= ~FOR_INSERT_UPDATE);
  };

  inline Int32 getAllFlags() { return flags_ ; };

  //
  // SPECIAL TRUNCATION FLAG 
  //
  // this flag is set if a character string casting is done when a larger
  // string is assigned to a smaller string size
  //
  // The following two methods are used only in ex_conv_clause objects

  void setCheckTruncationFlag()
                                    {flags_ |= CHECK_STRING_TRUNCATION;};
  Int32 getCheckTruncationFlag()
                                    {return (flags_ & CHECK_STRING_TRUNCATION); }

  void setNoTruncationWarningsFlag()
                                    {flags_ |= NO_STRING_TRUNCATION_WARNINGS; };
  Int32 getNoTruncationWarningsFlag()
                                    { return (flags_ & NO_STRING_TRUNCATION_WARNINGS); }

    // Branch handling
  //
  // isBranchingClause - Virtual function which returns TRUE if the clause
  // may change the flow on control (ie. branch)
  //
  // isBranchTarget - Returns TRUE is this clause is the target
  // of a branch clause.
  //
  // getNumberBranchTargets - Returns the number of branch clauses
  // which target this clause.
  //
  // addBranchTarget - Increments the number of branch clauses
  // which target this clause.
  //
  virtual Int32 isBranchingClause() const { return 0; };
  Int32 isBranchTarget() const { return numberBranchTargets_ != 0; };
  Int32 getNumberBranchTargets() { return numberBranchTargets_; }; 
  void setNumberBranchTargets(Int32 num) { numberBranchTargets_ = num; };
  void addBranchTarget() { numberBranchTargets_++; };

    
  unsigned short &clauseNum() { return clauseNum_; }

  // Operand Accessors
  //
  inline short getNumOperands();
  inline Attributes *getOperand(short operand_num);
  inline AttributesPtr *getOperand();
  inline void setOperand(AttributesPtrPtr& attr_);

  // Clause List Accessors
  //
  inline ex_clause *getNextClause();
  inline void setNextClause(ex_clause *clause_);
  inline void setNextPackedClause(Int64 offset);
  inline void setNext(Lng32 next_clause_offset);
  inline void setLastClause();

  // Packing, Unpacking, and Fixup
  //
  virtual Long pack(void *);
  virtual Lng32 unpack(void *, void * reallocator);
  Long packClause(void *, Lng32 size);
  Lng32 unpackClause(void *base, void * reallocator);

  virtual Lng32 initClause(){return 0;};

  // SpaceCompOnly: if TRUE, then compute space requirement only.
  //                Do not make any changes to the generated expressions,
  //                (like assigning tempsArea, assigning generated pcode, etc).
  virtual ex_expr::exp_return_type fixup(Space * space = 0,
						    CollHeap * exHeap = 0,
						    char * constants_area = 0,
						    char * temps_area = 0,
						    char * persistentArea = 0,
						    short = 0,
						    NABoolean spaceCompOnly = FALSE);
  
  // Execution
  //
  // isEvalRelevant - Returns true if eval() does any work. The eval()
  // methods for some clauses do nothing but serve as placeholders. These
  // clauses can be identified using this routine.
  //
  // eval - Actually does the work based on the pointers in the op_data
  // vector. The op_data vector has the following layout:
  //
  // 0 - pointer to result null indicator
  // 1 - TRUE if 1st arg is not NULL
  // 2 - TRUE if 2nd arg is not NULL
  // MAX_OPERANDS-1 - TRUE if MAX_OPERANDS-2(th) arg is not NULL
  //
  // MAX_OPERANDS+0 - pointer to result varchar length (NULL for non-varchar)
  // MAX_OPERANDS+1 - pointer to 1st arg varchar length (NULL for non-varchar)
  // MAX_OPERANDS+2 - pointer to 2nd arg varchar length (NULL for non-varchar)
  // 2*MAX_OPERANDS-1 - pointer to 2*MAX_OPERANDS-2(th) arg
  //
  // 2*MAX_OPERANDS+0 - pointer to result attribute
  // 2*MAX_OPERANDS+1 - pointer to 1st arg attribute (maybe null for NULL attr)
  // 2*MAX_OPERANDS+2 - pointer to 2nd arg attribute (maybe null for NULL attr)
  // 3*MAX_OPERANDS-1 - pointer to 3*MAX_OPERANDS-2(th) arg
  //
  // The processNulls method gets passed &(op_data[0]) so that addressing
  // the null indicators is done with op_data[0], op_data[1], etc.
  // The method returns EXPR_ERROR for attempt to assign a NULL to a nonnullable
  // column, EXPR_NULL for NULL successfully processed (assigned to a nullable
  // column), EXPR_OK for no error and no null -- caller must continue eval.
  //
  // The eval method gets passed &(op_data[2*MAX_OPERANDS+0]) so that
  // addressing the attributes is done with op_data[0], op_data[1], etc. The
  // null and varchar information is accessed using negative offsets:
  // op_data[-2*MAX_OPERANDS], etc.
  //
  //
  virtual Int32 isEvalRelevant() const
  { return 1;}; // must be redefined in derived classes
  virtual ex_expr::exp_return_type processNulls(char *op_data[],
							   CollHeap * = 0,
							   ComDiagsArea ** = 0);
  virtual ex_expr::exp_return_type eval(char *op_data[],
						   CollHeap * = 0,
						   ComDiagsArea ** = 0);
 
  // Display
  //
  virtual void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);
  
  void displayContents(Space * space, const char * displayStr, 
                                  Int32 clauseNum, char * constsArea, 
                                  UInt32 clauseFlags,
                                  Int16 instruction = -1,
                                  const char * instrText = NULL);

  static void clearVOA(Attributes *attr, atp_struct *atp)
  {
    CollIndex atpIdx = attr->getAtpIndex();
    
    if (attr->isSQLMXDiskFormat() && (attr->getVCIndicatorLength() > 0))
      str_pad((atp->getTupp(atpIdx)).getDataPointer(), sizeof(Int32), 0);
  }


  static void setRowLength(Attributes * attr, 
				      atp_struct * atp,
				      UInt32     * rowLen,
				      UInt32       newLength)
  {
    if (attr->isSQLMXAlignedFormat())
    {
      *rowLen = ExpAlignedFormat::adjustDataLength(
                             atp->getTupp(attr->getAtpIndex()).getDataPointer(),
                             newLength,
                             ExpAlignedFormat::ALIGNMENT
                             );
    }
    else
      *rowLen = newLength;
  }


  static void setVoaData(char *dataPtr, UInt32 voaOffset, UInt32 value)
   {
     assert(dataPtr);
     if (voaOffset != ExpOffsetMax)
       str_cpy_all(dataPtr + voaOffset, (char *)&value, ExpVoaSize);
   }
  
  static void evalSetRowLength(Attributes *attr, 
					  atp_struct *atp,
					  UInt32     *rowLen,
					  UInt32      newLength)
  {
    // Only adjust the length if using aligned row format.
    // If the row lengths are equal, then use the computed at codegen it is
    // adjusted already for alignment purposes.
    if (attr && attr->isSQLMXAlignedFormat() && (*rowLen != newLength))
    {
      *rowLen = ExpAlignedFormat::adjustDataLength(
                      (char*)atp->getTupp(attr->getAtpIndex()).getDataPointer(),
                      newLength,
                      ExpAlignedFormat::ALIGNMENT
                      );
    }
    else
      *rowLen = newLength;
  }

  void copyOperands(ex_clause* clause, Space* space);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  virtual char *findVTblPtr(short classID);
  // ---------------------------------------------------------------------

  void setExeGlobals(ex_globals * glob)
  {
    globals_ = glob;
  }

  void setTcb(const ex_tcb *tcb)
  {
    tcb_ = (ex_tcb *)tcb;
  }

  Int16 getInstrArrayIndex() { return instrArrayIndex_; }
  void setInstrArrayIndex(Int16 index) { instrArrayIndex_ = index; }

protected:
  ex_globals * getExeGlobals() 
  {
    return globals_;
  }
  
  ex_tcb *getTcb()
  {
    return tcb_;
  }

private:  
  ExClausePtr                nextClause_;          // 00-07
  PCILink                   *pciLink_;             // 08-15
  AttributesPtrPtr           op_;                  // 16-23
  UInt16                     clauseNum_;           // 24-25
  UInt16                     numberBranchTargets_; // 26-27
  UInt32                     flags_;               // 28-31
  enum flags_type {
    SPECIAL_NULLS = 0x0001,       // Compare NULLs as values
    ANY_INPUT_NULLABLE = 0x0002,  // Nullable Input
    ANY_OUTPUT_NULLABLE = 0x0004, // Nullable Ouput
    USE_PROCESS_NULLS = 0x0008,   // Use processNulls
    CHECK_STRING_TRUNCATION = 0x0010,  // check for string truncation
    NO_PCODE_AVAILABLE = 0x0020,
    NO_STRING_TRUNCATION_WARNINGS = 0x0040,
    HANDLE_INDIRECT_VC = 0x0080,
    FOR_INSERT_UPDATE  = 0x0100
  };
  Int16                      numOperands_;         // 32-33
  Int16 /*OperatorTypeEnum*/ operType_;            // 34-35
  Int16 /*clause_type*/      clauseType_;          // 36-37

  // index into static clause array struct containing details about
  // this clause (datatypes, operation, etc) if derived clause has
  // this array defined.
  // See derived clauses for details.
  Int16 instrArrayIndex_;                       // 48-49

  // this field is set at runtime when this clause is fixed up.
  // It is used in eval for cases where an expression needs to access 
  // executor globals.
  // Not valid for EID expressions.
  // Only used for SQ.
  ExGlobalsPtr globals_;                           // 40-47
 
  ex_tcb *tcb_;                              // 48-55
  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char                       fillers_[8];          // 56-63
};


inline ex_clause::clause_type ex_clause::getType(){
  
  return (clause_type)clauseType_;
};

inline void ex_clause::setType(ex_clause::clause_type t){
  clauseType_ = t;
};

inline void ex_clause::setLastClause(){
  nextClause_ = 0;
};

inline short ex_clause::getNumOperands(){
  return numOperands_;
};

inline Attributes *ex_clause::getOperand(short operand_num){
  return op_[operand_num];
};

inline AttributesPtr *ex_clause::getOperand(){
  return op_;
};

inline void ex_clause::setOperand(AttributesPtrPtr& attr)
{
  op_ = attr;
};

inline void ex_clause::setNextClause(ex_clause *clause_){
  nextClause_ = clause_;
};

inline ex_clause *ex_clause::getNextClause(){
  return nextClause_;
};

inline void ex_clause::setNextPackedClause(Int64 offset){
  nextClause_ = offset;
};

// functions to compare two strings
Int32 charStringCompareWithPad(char* in_s1, Int32 length1, 
                                          char* in_s2, Int32 length2, 
                                          char space);

Int32 wcharStringCompareWithPad(NAWchar* s1, Int32 length1, 
                                           NAWchar* s2, Int32 length2, 
                                           NAWchar space);
#endif
