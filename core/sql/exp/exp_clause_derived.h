/************************************************************************
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
 *****************************************************************************
 *
 * File:         <file>
 * RCS:          $Id: exp_clause_derived.cpp,v 1.44 1998/07/20 07:24:44  Exp $
 * Description:  
 *
 *
 * Created:      7/10/95
 * Modified:     $ $Date: 1998/07/20 07:24:44 $ (GMT)
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */
#ifndef EXP_CLAUSE_DERIVED_H
#define EXP_CLAUSE_DERIVED_H

#include <sys/types.h>
#include <regex.h>
#include "exp_clause.h"
#include "exp_like.h"
#include <byteswap.h>
#include "NAStringDef.h"


#define instrAndText(a) a, #a

/////////////////////////////////////////
// Class ex_aggregate_clause            //
/////////////////////////////////////////
class  ex_aggregate_clause : public ex_clause {
public:
  // Construction
  //
  ex_aggregate_clause(){};
  ex_aggregate_clause(OperatorTypeEnum oper_type,
				 short num_operands,
				 Attributes ** attr,
				 Space * space);


  // Null Semantics
  //
  Int32 isNullInNullOut() const { return 0; };
  Int32 isNullRelevant() const { return 0; };


  // Execution
  //
  Int32 isEvalRelevant() const { return 1; };
  virtual ex_expr::exp_return_type init();
  virtual ex_expr::exp_return_type eval(char *op_data[],
						   CollHeap * = 0,
						   ComDiagsArea** = 0);  

  // Fixup
  //
  Long pack(void *);
  virtual ex_expr::exp_return_type 
    pCodeGenerate(Space *space, UInt32 flags);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------
};

class  ex_aggr_one_row_clause : public ex_aggregate_clause {
public:	
  // Construction
  //
  ex_aggr_one_row_clause(){};
  ex_aggr_one_row_clause(OperatorTypeEnum oper_type,
				    short num_operands,
				    Attributes ** attr,
				    Space * space)
    : ex_aggregate_clause(oper_type, num_operands, attr, space) {};

 
  // Execution
  //
  ex_expr::exp_return_type init();
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0);  

  // Fixup
  //
  ex_expr::exp_return_type 
       pCodeGenerate(Space *space, UInt32 flags);  

  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
                                  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(2,getClassVersionID());
    ex_aggregate_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------
  
private:  
  // set to TRUE, if one row has been processed.
  UInt16           oneRowProcessed_;     // 00-01
  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char             fillers_[14];          // 02-15

};

class  ex_aggr_any_true_max_clause : public ex_aggregate_clause {
public:	
  // Construction
  //
  ex_aggr_any_true_max_clause(){};
  ex_aggr_any_true_max_clause(OperatorTypeEnum oper_type,
					 short num_operands,
					 Attributes ** attr,
					 Space * space)
    : ex_aggregate_clause(oper_type, num_operands, attr, space) {};
    
 
  // Execution
  //
  ex_expr::exp_return_type init();
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0);  

  // Fixup
  //
  ex_expr::exp_return_type 
       pCodeGenerate(Space *space, UInt32 flags);

  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
                                  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(2,getClassVersionID());
    ex_aggregate_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

private:  
  // set to TRUE, is a null value is seen while processing this
  // aggregate.
  UInt16           nullSeen_;            // 00-01
  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char             fillers_[14];          // 02-15

};

class  ex_aggr_min_max_clause : public ex_aggregate_clause {
public:	
  // Construction
  //
  ex_aggr_min_max_clause(){};
  ex_aggr_min_max_clause(OperatorTypeEnum oper_type,
				    short num_operands,
				    Attributes ** attr,
				    Space * space)
    : ex_aggregate_clause(oper_type, num_operands, attr, space) {};

 
  // Execution
  //
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0);  

  // Fixup
  //
  ex_expr::exp_return_type 
       pCodeGenerate(Space *space, UInt32 flags);

  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(2,getClassVersionID());
    ex_aggregate_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }

private:  
  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char             fillers_[16];          // 00-15

  // ---------------------------------------------------------------------
};

class  ExFunctionGrouping : public ex_aggregate_clause {
public:
  ExFunctionGrouping(OperatorTypeEnum oper_type,
                     short num_operands,
                     Attributes ** attr,
                     Int16 rollupGroupIndex,
                     Space * space)
    : ex_aggregate_clause(oper_type, num_operands, attr, space),
    rollupGroupIndex_(rollupGroupIndex),
    rollupNull_(0)
      {};
  
  ExFunctionGrouping(){};

  ex_expr::exp_return_type init();
  ex_expr::exp_return_type eval(char *op_data[], 
                                CollHeap* = 0, 
                                ComDiagsArea** = 0);  

  virtual short getClassSize() { return (short)sizeof(*this); }

  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
                                  Int32 clauseNum, char * constsArea);

  Int16 getRollupGroupIndex() { return rollupGroupIndex_; }
  void setRollupNull(short v) {  rollupNull_ = v; }

private:
  Int16 rollupGroupIndex_;
  Int16 rollupNull_;
  
  char fillers_[60];
  // ---------------------------------------------------------------------
};

class  ex_pivot_group_clause : public ex_aggregate_clause {
public:	
  // Construction
  //
  ex_pivot_group_clause(){};
  ex_pivot_group_clause(OperatorTypeEnum oper_type,
                                   short num_operands,
                                   Attributes ** attr,
                                   char * delim,
                                   Lng32 maxLen,
                                   NABoolean isOrderBy,
                                   Space * space)
    : ex_aggregate_clause(oper_type, num_operands, attr, space),
    maxLen_(maxLen)
    {
      setOrderBy(isOrderBy);
      strcpy(delim_, delim);
    }

 
  // Execution
  //
  ex_expr::exp_return_type init();
  ex_expr::exp_return_type eval(char *op_data[],
                                CollHeap * = 0,
                                ComDiagsArea ** = 0);

  Int32 isNullInNullOut() const { return 0; };
  Int32 isNullRelevant() const { return 1; };
  
  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
                                  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(2,getClassVersionID());
    ex_aggregate_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }

  NABoolean orderBy()   
  { return (flags_ & ORDER_BY) != 0;}
  void setOrderBy(NABoolean v)      
  { (v ? flags_ |= ORDER_BY : flags_ &= ~ORDER_BY); }

  NABoolean ovflWarn()   
  { return (flags_ & OVFL_WARN) != 0;}
  void setOvflWarn(NABoolean v)      
  { (v ? flags_ |= OVFL_WARN : flags_ &= ~OVFL_WARN); }

private:  
  enum
  {
    ORDER_BY   = 0x0001,
    OVFL_WARN = 0x0002
  };

  Lng32 currPos_;
  Lng32 currTgtLen_;
  char delim_[112];
  Lng32 maxLen_;
  UInt32 flags_;

  // ---------------------------------------------------------------------
};
  
/////////////////////////////////////////
// Class arith_clause                  //
/////////////////////////////////////////

// (these codes must remain invariant across future versions)
enum ArithInstruction { 
  ADD_BIN16S_BIN16S_BIN16S      =0, 
  ADD_BIN16S_BIN16S_BIN32S      =1, 
  ADD_BIN16S_BIN32S_BIN32S      =2, 
  ADD_BIN32S_BIN16S_BIN32S      =3, 
  ADD_BIN32S_BIN32S_BIN32S      =4,
  ADD_BIN64S_BIN64S_BIN64S      =5,
  ADD_BIN32S_BIN64S_BIN64S      =6, 
  ADD_BIN64S_BIN32S_BIN64S      =7,
  ADD_BIN16U_BIN16U_BIN16U      =8, 
  ADD_BIN16U_BIN16U_BIN32U      =9, 
  ADD_BIN16U_BIN32U_BIN32U      =10, 
  ADD_BIN32U_BIN16U_BIN32U      =11, 
  ADD_BIN32U_BIN32U_BIN32U      =12,
  ADD_BPINTU_BIN64S_BIN64S      =13, 
  ADD_BIN64S_BPINTU_BIN64S      =14, 
  ADD_BIN32U_BIN64S_BIN64S      =15, 
  ADD_BIN64S_BIN32U_BIN64S      =16,

  ADD_FLOAT32_FLOAT32_FLOAT32   =17, 
  ADD_FLOAT64_FLOAT64_FLOAT64   =18,
  ADD_DATETIME_INTERVAL_DATETIME=19, 
  ADD_INTERVAL_DATETIME_DATETIME=20,
    
  SUB_BIN16S_BIN16S_BIN16S      =21, 
  SUB_BIN16S_BIN16S_BIN32S      =22, 
  SUB_BIN16S_BIN32S_BIN32S      =23, 
  SUB_BIN32S_BIN16S_BIN32S      =24, 
  SUB_BIN32S_BIN32S_BIN32S      =25, 
  SUB_BIN64S_BIN64S_BIN64S      =26,
  SUB_BIN16U_BIN16U_BIN16U      =27, 
  SUB_BIN16U_BIN16U_BIN32U      =28, 
  SUB_BIN16U_BIN32U_BIN32U      =29, 
  SUB_BIN32U_BIN16U_BIN32U      =30, 
  SUB_BIN32U_BIN32U_BIN32U      =31,
  SUB_FLOAT32_FLOAT32_FLOAT32   =32, 
  SUB_FLOAT64_FLOAT64_FLOAT64   =33,
  SUB_DATETIME_INTERVAL_DATETIME=34, 
  SUB_DATETIME_DATETIME_INTERVAL=35,
    
  MUL_BIN16S_BIN16S_BIN16S      =36, 
  MUL_BIN16S_BIN16S_BIN32S      =37, 
  MUL_BIN16S_BIN32S_BIN32S      =38, 
  MUL_BIN32S_BIN16S_BIN32S      =39, 
  MUL_BIN32S_BIN32S_BIN32S      =40, 
  MUL_BIN64S_BIN64S_BIN64S      =41,
  MUL_BIN16U_BIN16U_BIN16U      =42, 
  MUL_BIN16U_BIN16U_BIN32U      =43, 
  MUL_BIN16U_BIN32U_BIN32U      =44, 
  MUL_BIN32U_BIN16U_BIN32U      =45, 
  MUL_BIN32U_BIN32U_BIN32U      =46,
  MUL_FLOAT32_FLOAT32_FLOAT32   =47, 
  MUL_FLOAT64_FLOAT64_FLOAT64   =48,
    
  DIV_BIN16S_BIN16S_BIN16S      =49, 
  DIV_BIN16S_BIN16S_BIN32S      =50, 
  DIV_BIN16S_BIN32S_BIN32S      =51, 
  DIV_BIN32S_BIN16S_BIN32S      =52, 
  DIV_BIN32S_BIN32S_BIN32S      =53, 
  DIV_BIN64S_BIN64S_BIN64S      =54,
  DIV_BIN16U_BIN16U_BIN16U      =55, 
  DIV_BIN16U_BIN16U_BIN32U      =56, 
  DIV_BIN16U_BIN32U_BIN32U      =57, 
  DIV_BIN32U_BIN16U_BIN32U      =58, 
  DIV_BIN32U_BIN32U_BIN32U      =59,
  DIV_FLOAT64_FLOAT64_FLOAT64   =60,
    
  ADD_COMPLEX                   =61, 
  SUB_COMPLEX                   =62, 
  MUL_COMPLEX                   =63, 
  DIV_COMPLEX                   =64,
  ARITH_NOT_SUPPORTED           =65,
  MUL_BIN16S_BIN32S_BIN64S      =66,
  MUL_BIN32S_BIN16S_BIN64S      =67,
  MUL_BIN32S_BIN32S_BIN64S      =68,

  DIV_BIN64S_BIN64S_BIN64S_ROUND=69,

  NEGATE_BOOLEAN                =70
};

class  ex_arith_clause : public ex_clause {

  typedef struct {
    OperatorTypeEnum op;
    short type_op1; // left operand
    short type_op2; // right operand
    short type_op0; // result
    ArithInstruction instruction;
    const char * instrStr;
  } ArithInstrStruct;
  
public:

  // Construction
  //
  ex_arith_clause() 
      { setAugmentedAssignOperation(TRUE); }

  ex_arith_clause(OperatorTypeEnum oper_type,
			     Attributes ** attr,
			     Space * space,
			     short arithRoundingMode,
			     NABoolean divToDownscale);
  ex_arith_clause(clause_type type,
			     OperatorTypeEnum oper_type,
			     Attributes ** attr,
			     Space * space);

 
  // Accessors
  //
  ArithInstruction getInstruction()
  {
    if (getInstrArrayIndex() >= 0)
      return getInstruction(getInstrArrayIndex());
    else
      return ARITH_NOT_SUPPORTED;
  };
  void setInstruction();
  void setInstruction(OperatorTypeEnum op,
                      Attributes * attr1,
                      Attributes * attr2,
                      Attributes * result);
  
  short isArithSupported(OperatorTypeEnum op,
                         Attributes * attr1,
                         Attributes * attr2,
                         Attributes * result);

  static const ArithInstrStruct arithInstrInfo[];
  static const char * getInstructionStr(Lng32 index) 
  { return arithInstrInfo[index].instrStr;}
  static const ArithInstruction getInstruction(Lng32 index) 
  { return arithInstrInfo[index].instruction;}

  Lng32 findIndexIntoInstrArray(ArithInstruction ci);

  // Null Semantics
  //
  Int32 isNullInNullOut() const { return 1; };
  Int32 isNullRelevant() const { return 1; };

  // Execution
  //
  Int32 isEvalRelevant() const { return 1; };
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0);  

  ex_expr::exp_return_type evalUnsupportedOperations(
       char *op_data[],
       CollHeap * heap,
       ComDiagsArea ** diagsArea);  

  // Fixup
  //
  Long pack(void *);
  ex_expr::exp_return_type pCodeGenerate(Space *space,
						    UInt32 flags);

  // pcode for unary arith operators, like NEGATE.
  ex_expr::exp_return_type unaryArithPCodeGenerate
    (Space *space, UInt32 flags);

  ex_expr::exp_return_type fixup(Space * space = 0,
					    CollHeap * exHeap = 0,
					    char * constants_area = 0,
					    char * temps_area = 0,
					    char * persistentArea = 0,
					    short fixupConstsAndTemps = 0,
					    NABoolean spaceCompOnly = FALSE);

  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------
  
private:
  enum
  {
    DIV_TO_DOWNSCALE = 0x01,
    ALLOW_AUGMENTED_ASSIGN_OPERATION = 0x02
  };

  char filler[4];             // 00-03

  char arithRoundingMode_;               // 04-04
  char flags_;                           // 05-05
 
  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char             fillers_[18];          // 06-23

  const ArithInstrStruct * getMatchingRow(OperatorTypeEnum op,
						    short datatype1,
						    short datatype2,
						    short resulttype);

  const ArithInstruction computeCaseIndex(OperatorTypeEnum op,
						     Attributes * attr1,
						     Attributes * attr2,
						     Attributes * result);

  NABoolean getDivToDownscale()   
  { return (flags_ & DIV_TO_DOWNSCALE) != 0;}
  void setDivToDownscale(NABoolean v)      
  { (v ? flags_ |= DIV_TO_DOWNSCALE : flags_ &= ~DIV_TO_DOWNSCALE); }
public:
  NABoolean isAugmentedAssignOperation()
  { return (flags_ & ALLOW_AUGMENTED_ASSIGN_OPERATION) != 0;}

  void setAugmentedAssignOperation(NABoolean v) 
  { (v ? flags_ |= ALLOW_AUGMENTED_ASSIGN_OPERATION : flags_ &= ~ALLOW_AUGMENTED_ASSIGN_OPERATION); }
  
};

class  ex_arith_sum_clause : public ex_arith_clause {
public:
  ex_arith_sum_clause() {};
  ex_arith_sum_clause(OperatorTypeEnum oper_type,
				 Attributes ** attr,
				 Space * space);


  ex_expr::exp_return_type pCodeGenerate(Space *space, 
						    UInt32 flags);

  // Null Semantics
  //
  Int32 isNullInNullOut() const { return 0; };
  ex_expr::exp_return_type processNulls(char *op_data[],
  						   CollHeap * = 0,
						   ComDiagsArea ** = 0);
  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }

  // ---------------------------------------------------------------------
  
private:   

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char             fillers_[8];          // 00-07

};

class  ex_arith_count_clause : public ex_arith_clause {
public:
  ex_arith_count_clause() {};
  ex_arith_count_clause(OperatorTypeEnum oper_type,
				   Attributes ** attr,
				   Space * space);


  ex_expr::exp_return_type pCodeGenerate(Space *space
						    , UInt32 flags);

  // Null Semantics
  //
  Int32 isNullInNullOut() const { return 0; };
  ex_expr::exp_return_type processNulls(char *op_data[],
  						   CollHeap * = 0,
						   ComDiagsArea ** = 0);
  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(2,getClassVersionID());
    ex_arith_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------
  
private:   

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char             fillers_[8];          // 00-07

};

/////////////////////////////////////////
// Class bool_clause                   //
/////////////////////////////////////////
class  ex_bool_clause : public ex_clause {
public:
  // Construction
  //
  ex_bool_clause(){};
  ex_bool_clause(OperatorTypeEnum oper_type,
			    Attributes ** attr,
			    Space * space);


  // Null Semantics
  //
  Int32 isNullInNullOut() const { return 0; };
  Int32 isNullRelevant() const { return 0; };

  // Execution
  //
  Int32 isEvalRelevant() const { return 1; };
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0);  

  // Fixup
  //
  Long pack(void *);
  ex_expr::exp_return_type 
    pCodeGenerate(Space *space, UInt32 flags);

  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }

private:
  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char             fillers_[8];          // 00-07

  // ---------------------------------------------------------------------
};

////////////////////////////////////////////////
// Class bool_result_clause
///////////////////////////////////////////////
class  bool_result_clause : public ex_clause {
public:
  // Construction
  //
  bool_result_clause(){};
  bool_result_clause(OperatorTypeEnum oper_type, Attributes ** attr,
				Space * space);


  // Null Semantics
  //
  Int32 isNullInNullOut() const { return 0; };
  Int32 isNullRelevant() const { return 0; };

  // Execution
  //
  Int32 isEvalRelevant() const { return 1; };
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0);

  // Fixup
  //
  Long pack(void *);
  ex_expr::exp_return_type 
    pCodeGenerate(Space *space, UInt32 flags);

  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }

private:
  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char             fillers_[8];          // 00-07

  // ---------------------------------------------------------------------
};

/////////////////////////////////////////
// Class branch_clause                 //
/////////////////////////////////////////
class  ex_branch_clause : public ex_clause {
public:
  // Construction
  //
  ex_branch_clause() : saved_next_clause(NULL), branch_clause(NULL)
    {};
  ex_branch_clause(OperatorTypeEnum oper_type,
			      Attributes ** attr,
			      Space * space);

  ex_branch_clause(OperatorTypeEnum oper_type,
			      Space * space);


  // Accessors
  //
  //inline void set_branch_clause(ex_clause*);
  inline void set_branch_clause(ex_clause *clause_)
  {
    branch_clause = clause_;
  };
  //inline void set_saved_next(ex_clause*);
  inline void set_saved_next(ex_clause *clause_)
  {
    saved_next_clause = clause_;
  };
  inline ex_clause * get_branch_clause()
  {
    return branch_clause;
  };
  //inline ex_clause * get_saved_next();
  inline ex_clause * get_saved_next()
  {
    return saved_next_clause;
  };

  // This is a branching clause
  //
  Int32 isBranchingClause() const { return 1; };

  // Null Semantics
  //
  Int32 isNullInNullOut() const { return 0; };
  Int32 isNullRelevant() const { return 0; };

  // Execution
  //
  Int32 isEvalRelevant() const { return 1; };
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0);

  // Fixup
  //
  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);
  ex_expr::exp_return_type fixup(Space * space = 0,
                                            CollHeap * exHeap = 0,
                                            char * constants_area = 0,
                                            char * temps_area = 0,
                                            char * persistentArea = 0,
                                            short = 0,
					    NABoolean spaceCompOnly = FALSE);
  ex_expr::exp_return_type 
    pCodeGenerate(Space *space, UInt32 flags);

  void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

private:   
  ExClausePtr      saved_next_clause;    // 00-07
  ExClausePtr      branch_clause;        // 08-15

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char             fillers_[8];          // 16-23

};

//inline void ex_branch_clause::set_branch_clause(ex_clause *clause_){
//  branch_clause = clause_;
//};

//inline void ex_branch_clause::set_saved_next(ex_clause *clause_){
//  saved_next_clause = clause_;
//};

//inline ex_clause * ex_branch_clause::get_branch_clause(){
//  return branch_clause;
//};

//inline ex_clause * ex_branch_clause::get_saved_next(){
//  return saved_next_clause;
//};

#define myDefine(a) #a

/////////////////////////////////////////
// Class comp_clause
/////////////////////////////////////////
// (these codes must remain invariant across future versions)
enum CompInstruction { 
  EQ_BIN16S_BIN16S    =0,
  EQ_BIN16S_BIN32S    =1, 
  EQ_BIN16S_BIN16U    =2, 
  EQ_BIN16S_BIN32U    =3,
  NE_BIN16S_BIN16S    =4, 
  NE_BIN16S_BIN32S    =5, 
  NE_BIN16S_BIN16U    =6, 
  NE_BIN16S_BIN32U    =7,
  LT_BIN16S_BIN16S    =8, 
  LT_BIN16S_BIN32S    =9, 
  LT_BIN16S_BIN16U    =10, 
  LT_BIN16S_BIN32U    =11,
  LE_BIN16S_BIN16S    =12, 
  LE_BIN16S_BIN32S    =13, 
  LE_BIN16S_BIN16U    =14, 
  LE_BIN16S_BIN32U    =15,
  GT_BIN16S_BIN16S    =16, 
  GT_BIN16S_BIN32S    =17, 
  GT_BIN16S_BIN16U    =18, 
  GT_BIN16S_BIN32U    =19,
  GE_BIN16S_BIN16S    =20, 
  GE_BIN16S_BIN32S    =21, 
  GE_BIN16S_BIN16U    =22, 
  GE_BIN16S_BIN32U    =23,
    
  EQ_BIN32S_BIN16S    =24, 
  EQ_BIN32S_BIN32S    =25, 
  EQ_BIN32S_BIN16U    =26, 
  EQ_BIN32S_BIN32U    =27,
  NE_BIN32S_BIN16S    =28, 
  NE_BIN32S_BIN32S    =29, 
  NE_BIN32S_BIN16U    =30, 
  NE_BIN32S_BIN32U    =31,
  LT_BIN32S_BIN16S    =32, 
  LT_BIN32S_BIN32S    =33, 
  LT_BIN32S_BIN16U    =34, 
  LT_BIN32S_BIN32U    =35,
  LE_BIN32S_BIN16S    =36, 
  LE_BIN32S_BIN32S    =37, 
  LE_BIN32S_BIN16U    =38, 
  LE_BIN32S_BIN32U    =39,
  GT_BIN32S_BIN16S    =40, 
  GT_BIN32S_BIN32S    =41, 
  GT_BIN32S_BIN16U    =42, 
  GT_BIN32S_BIN32U    =43,
  GE_BIN32S_BIN16S    =44, 
  GE_BIN32S_BIN32S    =45, 
  GE_BIN32S_BIN16U    =46, 
  GE_BIN32S_BIN32U    =47,
    
  EQ_BIN64S_BIN64S    =48,
  NE_BIN64S_BIN64S    =49,
  LT_BIN64S_BIN64S    =50,
  LE_BIN64S_BIN64S    =51,
  GT_BIN64S_BIN64S    =52,
  GE_BIN64S_BIN64S    =53,
    
  EQ_BIN16U_BIN16S    =54, 
  EQ_BIN16U_BIN32S    =55, 
  EQ_BIN16U_BIN16U    =56, 
  EQ_BIN16U_BIN32U    =57,
  NE_BIN16U_BIN16S    =58, 
  NE_BIN16U_BIN32S    =59, 
  NE_BIN16U_BIN16U    =60, 
  NE_BIN16U_BIN32U    =61,
  LT_BIN16U_BIN16S    =62, 
  LT_BIN16U_BIN32S    =63, 
  LT_BIN16U_BIN16U    =64, 
  LT_BIN16U_BIN32U    =65,
  LE_BIN16U_BIN16S    =66, 
  LE_BIN16U_BIN32S    =67, 
  LE_BIN16U_BIN16U    =68, 
  LE_BIN16U_BIN32U    =69,
  GT_BIN16U_BIN16S    =70, 
  GT_BIN16U_BIN32S    =71, 
  GT_BIN16U_BIN16U    =72, 
  GT_BIN16U_BIN32U    =73,
  GE_BIN16U_BIN16S    =74, 
  GE_BIN16U_BIN32S    =75, 
  GE_BIN16U_BIN16U    =76, 
  GE_BIN16U_BIN32U    =77,
    
  EQ_BIN32U_BIN16S    =78, 
  EQ_BIN32U_BIN32S    =79, 
  EQ_BIN32U_BIN16U    =80, 
  EQ_BIN32U_BIN32U    =81,
  NE_BIN32U_BIN16S    =82, 
  NE_BIN32U_BIN32S    =83, 
  NE_BIN32U_BIN16U    =84, 
  NE_BIN32U_BIN32U    =85,
  LT_BIN32U_BIN16S    =86, 
  LT_BIN32U_BIN32S    =87, 
  LT_BIN32U_BIN16U    =88, 
  LT_BIN32U_BIN32U    =89,
  LE_BIN32U_BIN16S    =90, 
  LE_BIN32U_BIN32S    =91, 
  LE_BIN32U_BIN16U    =92, 
  LE_BIN32U_BIN32U    =93,
  GT_BIN32U_BIN16S    =94, 
  GT_BIN32U_BIN32S    =95, 
  GT_BIN32U_BIN16U    =96, 
  GT_BIN32U_BIN32U    =97,
  GE_BIN32U_BIN16S    =98, 
  GE_BIN32U_BIN32S    =99, 
  GE_BIN32U_BIN16U    =100, 
  GE_BIN32U_BIN32U    =101,
    
  EQ_DECU_DECU        =102, 
  NE_DECU_DECU        =103, 
  LT_DECU_DECU        =104, 
  LE_DECU_DECU        =105, 
  GT_DECU_DECU        =106,
  GE_DECU_DECU        =107,
    
  EQ_DECS_DECS        =108, 
  NE_DECS_DECS        =109, 
  LT_DECS_DECS        =110, 
  LE_DECS_DECS        =111, 
  GT_DECS_DECS        =112,
  GE_DECS_DECS        =113,
    
  EQ_FLOAT32_FLOAT32  =114,
  NE_FLOAT32_FLOAT32  =115,
  LT_FLOAT32_FLOAT32  =116,
  LE_FLOAT32_FLOAT32  =117,
  GT_FLOAT32_FLOAT32  =118,
  GE_FLOAT32_FLOAT32  =119,
    
  EQ_FLOAT64_FLOAT64  =120,
  NE_FLOAT64_FLOAT64  =121,
  LT_FLOAT64_FLOAT64  =122,
  LE_FLOAT64_FLOAT64  =123,
  GT_FLOAT64_FLOAT64  =124,
  GE_FLOAT64_FLOAT64  =125,
    
  EQ_DATETIME_DATETIME=126,
  NE_DATETIME_DATETIME=127,
  LT_DATETIME_DATETIME=128,
  LE_DATETIME_DATETIME=129,
  GT_DATETIME_DATETIME=130,
  GE_DATETIME_DATETIME=131,
    
  EQ_ASCII_F_F        =132, 
  NE_ASCII_F_F        =133, 
  LT_ASCII_F_F        =134, 
  LE_ASCII_F_F        =135, 
  GT_ASCII_F_F        =136,
  GE_ASCII_F_F        =137,
    
  EQ_UNICODE_F_F      =138, 
  NE_UNICODE_F_F      =139, 
  LT_UNICODE_F_F      =140, 
  LE_UNICODE_F_F      =141, 
  GT_UNICODE_F_F      =142, 
  GE_UNICODE_F_F      =143,
  
  ASCII_COMP          =144,   
  UNICODE_COMP        =145,
    
  COMP_COMPLEX        =146,

  EQ_ASCII_COMP       =147,
  NE_ASCII_COMP       =148,
  LT_ASCII_COMP       =149,
  LE_ASCII_COMP       =150,
  GT_ASCII_COMP       =151,
  GE_ASCII_COMP       =152,

  EQ_BLOB        =153, 
  NE_BLOB        =154, 
  LT_BLOB        =155, 
  LE_BLOB        =156, 
  GT_BLOB        =157,
  GE_BLOB        =158,

  EQ_BIN64U_BIN64U    =159,
  EQ_BIN64U_BIN64S    =160,
  EQ_BIN64S_BIN64U    =161,
  NE_BIN64U_BIN64U    =162,
  NE_BIN64U_BIN64S    =163,
  NE_BIN64S_BIN64U    =164,
  LT_BIN64U_BIN64U    =165,
  LT_BIN64U_BIN64S    =166,
  LT_BIN64S_BIN64U    =167,
  LE_BIN64U_BIN64U    =168,
  LE_BIN64U_BIN64S    =169,
  LE_BIN64S_BIN64U    =170,
  GT_BIN64U_BIN64U    =171,
  GT_BIN64U_BIN64S    =172,
  GT_BIN64S_BIN64U    =173,
  GE_BIN64U_BIN64U    =174,
  GE_BIN64U_BIN64S    =175,
  GE_BIN64S_BIN64U    =176,
    
  EQ_BOOL_BOOL        =177,
  NE_BOOL_BOOL        =178,

  // tinyint operations
  EQ_BIN8S_BIN8S      =179,
  EQ_BIN8U_BIN8U      =180,
  NE_BIN8S_BIN8S      =181,
  NE_BIN8U_BIN8U      =182,
  LT_BIN8S_BIN8S      =183,
  LT_BIN8U_BIN8U      =184,
  LE_BIN8S_BIN8S      =185,
  LE_BIN8U_BIN8U      =186,
  GT_BIN8S_BIN8S      =187,
  GT_BIN8U_BIN8U      =188,
  GE_BIN8S_BIN8S      =189,
  GE_BIN8U_BIN8U      =190,

  // comparison between sql binary/varbinary datatypes
  BINARY_COMP         =191,

  COMP_NOT_SUPPORTED  =192
};

class  ex_comp_clause : public ex_clause {

  typedef struct {
    OperatorTypeEnum op;
    short type_op1; // left operand
    short type_op2; // right operand
    CompInstruction instruction;
    const char * instrStr;
  } CompInstrStruct;
  
public:

  // Construction
  //
  ex_comp_clause(): flags_(0)
  {  };
  ex_comp_clause(OperatorTypeEnum oper_type,
			    Attributes ** attr,
			    Space * space,
			    ULng32 flags_);


  // Accessors
  //
  inline CompInstruction getInstruction()
  {
    if (getInstrArrayIndex() >= 0)
      return getInstruction(getInstrArrayIndex());
    else
      return COMP_NOT_SUPPORTED;
  };

  void setInstruction();
  void setInstruction(OperatorTypeEnum op,
				 Attributes * attr1,
				 Attributes * attr2);

  short isComparisonSupported(OperatorTypeEnum op,
					 Attributes * attr1,
					 Attributes * attr2);

  static const CompInstrStruct compInstrInfo[];
  static const char * getInstructionStr(Lng32 index) 
  { return compInstrInfo[index].instrStr;}
  static const CompInstruction getInstruction(Lng32 index) 
  { return compInstrInfo[index].instruction;}

  Lng32 findIndexIntoInstrArray(CompInstruction ci);

  void setRollupColumnNum(Int16 v) {rollupColumnNum_ = v;}
  Int16 getRollupColumnNum() { return rollupColumnNum_; }

  // Null Semantics
  //
  Int32 isNullInNullOut() const { return 0; };
  Int32 isNullRelevant() const { return 1; };
  ex_expr::exp_return_type processNulls(char *op_data[],
  						   CollHeap * = 0,
						   ComDiagsArea ** = 0);

  // Execution
  //
  Int32 isEvalRelevant() const { return 1; };
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0); 

  ex_expr::exp_return_type evalUnsupportedOperations(
       char *op_data[],
       CollHeap * heap,
       ComDiagsArea ** diagsArea);  

  // Fixup
  //
  Long pack(void *);
  ex_expr::exp_return_type 
    pCodeGenerate(Space *space, UInt32 flags);
  ex_expr::exp_return_type fixup(Space * space = 0,
					    CollHeap * exHeap = 0,
					    char * constants_area = 0,
					    char * temps_area = 0,
					    char * persistentArea = 0,
					    short fixupConstsAndTemps = 0,
					    NABoolean spaceCompOnly = FALSE);

  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }

   NABoolean getCollationEncodeComp() 
  {
    return ((flags_ & COLLATION_ENCODE_COMP) != 0);
  }
   void setCollationEncodeComp(NABoolean v) 
  {
    (v) ? flags_ |= COLLATION_ENCODE_COMP: flags_ &= ~COLLATION_ENCODE_COMP;
  }



  // ---------------------------------------------------------------------

private:  

  enum
  {
    COLLATION_ENCODE_COMP          = 0x0001
  };

  Int32            filler0;           // 00-03
  Int16		   flags_; //04-05		   

  // see optimizer/ItemLog.h, class BiRelat
  Int16 rollupColumnNum_;   //06-07

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char             fillers_[24];          // 08-31
  ex_expr::exp_return_type processResult(Int32 compare_code, Lng32* result,
					      CollHeap *heap,
					      ComDiagsArea** diagsArea);
  
  const CompInstrStruct * getMatchingRow(OperatorTypeEnum op,
						   short datatype1,
						   short datatype2);

  const CompInstruction computeCaseIndex(OperatorTypeEnum op,
						    Attributes * attr1,
						    Attributes * attr2);
};

  
/////////////////////////////////////////
// Class ex_conv_clause
/////////////////////////////////////////
enum ConvInstruction {
  // (these codes must remain invariant across future versions)
  CONV_BIN16S_BPINTU                   =0,  
  CONV_BIN16S_BIN16S                   =1,    
  CONV_BIN16S_BIN16U                   =2,   
  CONV_BIN16S_BIN32S                   =3,  
  CONV_BIN16S_BIN32U                   =4,
  CONV_BIN16S_BIN64S                   =5,   
  CONV_BIN16S_DECS                     =6,
  CONV_BIN16S_DECU                     =7,
  CONV_BIN16S_FLOAT32                  =8,  
  CONV_BIN16S_FLOAT64                  =10,
  CONV_BIN16S_ASCII                    =13, 
  CONV_BPINTU_BPINTU                   =14,

  CONV_BIN16U_BPINTU                   =15, 
  CONV_BIN16U_BIN16S                   =16, 
  CONV_BIN16U_BIN16U                   =17,   
  CONV_BIN16U_BIN32S                   =18, 
  CONV_BIN16U_BIN32U                   =19, 
  CONV_BIN16U_BIN64S                   =20,   
  CONV_BIN16U_DECS                     =21, 
  CONV_BIN16U_FLOAT32                  =22,
  CONV_BIN16U_FLOAT64                  =23, 
  CONV_BIN16U_ASCII                    =25,

  CONV_BIN32S_BPINTU                   =26, 
  CONV_BIN32S_BIN16S                   =27, 
  CONV_BIN32S_BIN16U                   =28,   
  CONV_BIN32S_BIN32S                   =29, 
  CONV_BIN32S_BIN32U                   =30, 
  CONV_BIN32S_BIN64S                   =31,   
  CONV_BIN32S_DECS                     =32, 
  CONV_BIN32S_DECU                     =33, 
  CONV_BIN32S_FLOAT32                  =34,  
  CONV_BIN32S_FLOAT64                  =35, 
  CONV_BIN32S_ASCII                    =38,

  CONV_BIN32U_BPINTU                   =39, 
  CONV_BIN32U_BIN16S                   =40, 
  CONV_BIN32U_BIN16U                   =41,   
  CONV_BIN32U_BIN32S                   =42, 
  CONV_BIN32U_BIN32U                   =43, 
  CONV_BIN32U_BIN64S                   =44,   
  CONV_BIN32U_DECS                     =45, 
  CONV_BIN32U_FLOAT32                  =46,
  CONV_BIN32U_FLOAT64                  =47, 
  CONV_BIN32U_ASCII                    =49,

  CONV_BIN64S_BPINTU                   =50, 
  CONV_BIN64S_BIN16S                   =51, 
  CONV_BIN64S_BIN16U                   =52,   
  CONV_BIN64S_BIN32S                   =53, 
  CONV_BIN64S_BIN32U                   =54, 
  CONV_BIN64S_BIN64S                   =55, 
  CONV_BIN64S_DECS                     =56, 
  CONV_BIN64S_DECU                     =57, 
  CONV_BIN64S_FLOAT32                  =58,  
  CONV_BIN64S_FLOAT64                  =59, 
  CONV_BIN64S_ASCII                    =62,
 
  // all conversions from DECU are handled by the appropriate
  // CONV_DECS_* (compare exp_fixup.cpp)
  // conversions to BIN16S, BIN16U are handled
  // by CONV_DECS_BIN32S and CONV_DECS_BIN32U respectively
  // conversions to BPINT are hadled by CONV_DECS_BIN32U

  CONV_DECS_BIN32S                     =63,
  CONV_DECS_BIN32U                     =64, 
  CONV_DECS_BIN64S                     =65, 
  CONV_DECS_DECS                       =66,
  CONV_DECS_DECU                       =67, 
  CONV_DECS_FLOAT32                    =68, 
  CONV_DECS_FLOAT64                    =69,
  CONV_DECS_ASCII                      =70,

  CONV_FLOAT32_BPINTU                  =71, 
  CONV_FLOAT32_BIN16S                  =72, 
  CONV_FLOAT32_BIN16U                  =73,  
  CONV_FLOAT32_BIN32S                  =74, 
  CONV_FLOAT32_BIN32U                  =75, 
  CONV_FLOAT32_BIN64S                  =76,  
  CONV_FLOAT32_DECS                    =77, 
  CONV_FLOAT32_DECU                    =78, 
  CONV_FLOAT32_FLOAT32                 =79, 
  CONV_FLOAT32_FLOAT64                 =80, 
  CONV_FLOAT32_ASCII                   =81,

  CONV_FLOAT64_BPINTU                  =82, 
  CONV_FLOAT64_BIN16S                  =83, 
  CONV_FLOAT64_BIN16U                  =84,  
  CONV_FLOAT64_BIN32S                  =85, 
  CONV_FLOAT64_BIN32U                  =86, 
  CONV_FLOAT64_BIN64S                  =87,  
  CONV_FLOAT64_DECS                    =88, 
  CONV_FLOAT64_DECU                    =89, 
  CONV_FLOAT64_FLOAT32                 =90, 
  CONV_FLOAT64_FLOAT64                 =91, 
  CONV_FLOAT64_ASCII                   =92,

  CONV_DECLS_DECLS                     =104, 
  CONV_DECLS_ASCII                     =105,

  // interval conversions

  // The representation of an interval depends only on the end field;
  // conversions between interval datatypes with the same end field
  // are done via the numeric cases.  When end fields differ, conversion
  // implies multiplication or division by a constant (e.g. converting
  // years to months implies multiplication by 12).  Note that the
  // Generator ordinarily generates arithmetic operators to do this
  // scaling; the exception is in key building where we need to catch
  // overflow and truncations from this scaling operation.  It's simpler
  // to catch it as part of convDoIt() processing.

  CONV_INTERVALY_INTERVALMO            =106,
  
  CONV_INTERVALMO_INTERVALY            =107,

  CONV_INTERVALD_INTERVALH             =108,  
  CONV_INTERVALD_INTERVALM             =109,  
  CONV_INTERVALD_INTERVALS             =110,

  CONV_INTERVALH_INTERVALD             =111,  
  CONV_INTERVALH_INTERVALM             =112,  
  CONV_INTERVALH_INTERVALS             =113,

  CONV_INTERVALM_INTERVALD             =114,  
  CONV_INTERVALM_INTERVALH             =115,  
  CONV_INTERVALM_INTERVALS             =116,

  CONV_INTERVALS_INTERVALD             =117,  
  CONV_INTERVALS_INTERVALH             =118,  
  CONV_INTERVALS_INTERVALM             =119,
  CONV_INTERVALS_INTERVALS_DIV         =120, // second(n) to second(m), n > m
  CONV_INTERVALS_INTERVALS_MULT        =121, // second(n) to second(m), m > n
  CONV_INTERVAL_ASCII                  =122,

  // datetime conversions

  CONV_DATETIME_DATETIME               =123, 
  CONV_DATETIME_ASCII                  =124,

  // character string conversions

  CONV_ASCII_BIN16S                    =125, 
  CONV_ASCII_BIN16U                    =126,    
  CONV_ASCII_BIN32S                    =127, 
  CONV_ASCII_BIN32U                    =128, 
  CONV_ASCII_BIN64S                    =129,
  CONV_ASCII_DEC                       =130,
  CONV_ASCII_FLOAT32                   =131, 
  CONV_ASCII_FLOAT64                   =132,
  CONV_ASCII_DATETIME                  =134,
  CONV_ASCII_INTERVAL                  =135,
  CONV_ASCII_F_F                       =136, 
  CONV_ASCII_F_V                       =137, 
  CONV_ASCII_V_F                       =138,
  CONV_ASCII_V_V                       =139,

// unicode related conversion constants

  CONV_UNICODE_BIN16S                  =140, 
  CONV_UNICODE_BIN16U                  =141,    
  CONV_UNICODE_BIN32S                  =142, 
  CONV_UNICODE_BIN32U                  =143,    
  CONV_UNICODE_BIN64S                  =144,
  CONV_UNICODE_DEC                     =145,
  CONV_UNICODE_FLOAT32                 =146, 
  CONV_UNICODE_FLOAT64                 =147,
  CONV_UNICODE_DATETIME                =149,
  CONV_UNICODE_INTERVAL                =150,

  CONV_BIN16S_UNICODE                  =151, 
  CONV_BIN16U_UNICODE                  =152,    
  CONV_BIN32S_UNICODE                  =153, 
  CONV_BIN32U_UNICODE                  =154,    
  CONV_BIN64S_UNICODE                  =155,
  CONV_DECS_UNICODE                    =156,
  CONV_FLOAT32_UNICODE                 =157, 
  CONV_FLOAT64_UNICODE                 =158,
  CONV_DATETIME_UNICODE                =160,
  CONV_INTERVAL_UNICODE                =161,

  CONV_UNICODE_F_F                     =162, 
  CONV_UNICODE_F_V                     =163, 
  CONV_UNICODE_V_F                     =164,
  CONV_UNICODE_V_V                     =165,
  CONV_UNICODE_F_ASCII_F               =166,
  CONV_UNICODE_V_ASCII_F               =167,
  CONV_UNICODE_F_ASCII_V               =168,
  CONV_UNICODE_V_ASCII_V               =169,

  CONV_UNICODE_F_SJIS_F                =170,
  CONV_UNICODE_V_SJIS_F                =171,
  CONV_UNICODE_F_SJIS_V                =172,
  CONV_UNICODE_V_SJIS_V                =173,

  CONV_SJIS_F_UNICODE_F                =174,
  CONV_SJIS_V_UNICODE_F                =175,
  CONV_SJIS_F_UNICODE_V                =176,
  CONV_SJIS_V_UNICODE_V                =177,

  CONV_UNICODE_F_MBYTE_LOCALE_F        =178, // to multi-byte locale (fixed)
  CONV_UNICODE_F_SBYTE_LOCALE_F        =179, // to single-byte locale (fixed)
  CONV_UNICODE_V_MBYTE_LOCALE_F        =180, // to multi-byte locale (v)
  CONV_UNICODE_V_SBYTE_LOCALE_F        =181, // to single-byte locale (v)

// 6/26/98: added from unicode to ansi varnchar, 
// from ansi varnchar to varnchar, and ansi varnchar to ansi varnchar

  CONV_UNICODE_TO_ANSI_V_UNICODE       =182, 
  CONV_ANSI_V_UNICODE_TO_UNICODE_V     =183, 
  CONV_ANSI_V_UNICODE_TO_ANSI_V_UNICODE=184,

  // convert from ASCII(SQL internal) to ANSI V (ANSI VARCHAR --
  // null terminated string)

  CONV_ASCII_UNICODE_F                 =185, 
  CONV_ASCII_UNICODE_V                 =186, 

  // convert from ASCII(SQL internal) to ANSI V (ANSI VARCHAR --
  // null terminated string)

  CONV_ASCII_TO_ANSI_V                 =187,

  CONV_ANSI_V_TO_ASCII_F               =188,
  CONV_ANSI_V_TO_ASCII_V               =189,
  CONV_ANSI_V_TO_ANSI_V                =190,
    
  CONV_COMPLEX_TO_COMPLEX              =191, 
  CONV_SIMPLE_TO_COMPLEX               =192, 
  CONV_COMPLEX_TO_SIMPLE               =193,

  CONV_NOT_SUPPORTED                   =194, 
  CONV_UNKNOWN_LEFTPAD                 =195, 
  CONV_UNKNOWN                         =196,

  CONV_UNICODE_F_ANSI_V                =197,
  CONV_UNICODE_V_ANSI_V                =198,

  // Big Num related conversions

  CONV_BIN16S_BIGNUMU                  =199,
  CONV_BIN16S_BIGNUM                   =200,
  CONV_BIN16U_BIGNUM                   =201,
  CONV_BIN32S_BIGNUMU                  =202,
  CONV_BIN32S_BIGNUM                   =203,
  CONV_BIN32U_BIGNUM                   =204,
  CONV_BIN64S_BIGNUMU                  =205,
  CONV_BIN64S_BIGNUM                   =206,
  CONV_ASCII_BIGNUM                    =207,
  CONV_UNICODE_BIGNUM                  =208,

  CONV_BIGNUM_BIN16S                   =209,
  CONV_BIGNUM_BIN16U                   =210,
  CONV_BIGNUM_BIN32S                   =211,
  CONV_BIGNUM_BIN32U                   =212,
  CONV_BIGNUM_BIN64S                   =213,
  CONV_BIGNUM_DECS                     =214,
  CONV_BIGNUM_DECU                     =215,
  CONV_BIGNUM_FLOAT32                  =216,
  CONV_BIGNUM_FLOAT64                  =217,
  CONV_BIGNUM_BIGNUM                   =218,
  CONV_BIGNUM_ASCII                    =219,
  CONV_BIGNUM_UNICODE                  =220,

  // Conversions needed for char-type matching rule relaxation
  CONV_ASCII_TO_ANSI_V_UNICODE       =222, 
  CONV_ANSI_V_UNICODE_TO_ASCII_V     =223,
  CONV_ANSI_V_UNICODE_TO_ASCII_F     =224,
  CONV_ANSI_V_UNICODE_TO_UNICODE_F   =225,

  // conversion between tandem and IEEE floats. These are the ONLY supported
  // conversions involving tdm floats. Any other conversion(like int to 
  // tdm_float32, etc) should be first converted to IEEE equivalent and then
  // from IEEE to tdm.

  CONV_FLOAT32TDM_FLOAT32IEEE          =226,
  CONV_FLOAT32TDM_FLOAT64IEEE          =227,
  CONV_FLOAT32TDM_FLOAT32TDM           =228,
  CONV_FLOAT32TDM_FLOAT64TDM           =229,

  CONV_FLOAT64TDM_FLOAT32IEEE          =230,
  CONV_FLOAT64TDM_FLOAT64IEEE          =231,
  CONV_FLOAT64TDM_FLOAT32TDM           =232,
  CONV_FLOAT64TDM_FLOAT64TDM           =233,

  CONV_FLOAT32IEEE_FLOAT32TDM          =234,
  CONV_FLOAT32IEEE_FLOAT64TDM          =235,
  CONV_FLOAT64IEEE_FLOAT32TDM          =236,
  CONV_FLOAT64IEEE_FLOAT64TDM          =237,

  CONV_FLOAT32TDM_ASCII                =238,
  CONV_FLOAT64TDM_ASCII                =239,
  CONV_ASCII_FLOAT32TDM                =240,
  CONV_ASCII_FLOAT64TDM                =241,
  CONV_DECLS_DECU                      =242,

  CONV_BIN64S_DATETIME                 =243,
  CONV_DATETIME_BIN64S                 =244,

  CONV_UCS2_F_SJIS_V                   =245,
  CONV_UCS2_F_UTF8_V                   =246,
  CONV_SJIS_F_UCS2_V                   =247,
  CONV_UTF8_F_UCS2_V                   =248,

  CONV_BLOB_BLOB                       =249,
  CONV_BLOB_ASCII_F                    =250,

  CONV_GBK_F_UTF8_V                    =251,

  // TINYINT conversions
  CONV_BIN8S_BIN8S                     =252,
  CONV_BIN8U_BIN8U                     =253,
  CONV_BIN8S_BIN16S                    =254,
  CONV_BIN8U_BIN16U                    =255,
  CONV_BIN16S_BIN8S                    =256,
  CONV_BIN16U_BIN8U                    =257,
  CONV_BIN16U_BIN8S                    =258,
  CONV_BIN16S_BIN8U                    =259,
  CONV_BIN8U_BIN16S                    =260,
  CONV_BIN8S_BIN32S                    =261,
  CONV_BIN8U_BIN32U                    =262,
  CONV_BIN8S_BIN64S                    =263,
  CONV_BIN8U_BIN64U                    =264,
  CONV_BIN8S_ASCII                     =265,
  CONV_BIN8U_ASCII                     =266,
  CONV_ASCII_BIN8S                     =267,
  CONV_ASCII_BIN8U                     =268,
  
  // boolean conversions
  CONV_BOOL_BOOL                       =269,
  CONV_BOOL_ASCII                      =270,
  CONV_ASCII_BOOL                      =271,

  // unsigned largeint related conversions
  CONV_BIN64S_BIN64U                   =272,
  CONV_BIN64U_BIN64U                   =273,
  CONV_FLOAT32_BIN64U                  =274, 
  CONV_FLOAT64_BIN64U                  =275,
  CONV_BIN64U_BIN64S                   =276,
  CONV_BIN64U_BIGNUM                   =277,
  CONV_BIGNUM_BIN64U                   =278,
  CONV_BIN64U_FLOAT32                  =279,
  CONV_BIN64U_FLOAT64                  =280,
  CONV_BIN64U_ASCII                    =281,
  CONV_ASCII_BIN64U                    =282,

  // more tinyint conversions not handled above. 
  // At runtime, expression code will handle them based on source datatype.
  CONV_NUMERIC_BIN8S                    =283,
  CONV_NUMERIC_BIN8U                    =284,

  CONV_BIN8S_BIN8U                     =285,
  CONV_BIN8U_BIN8S                     =286,

  CONV_BINARY_TO_BINARY                =287,
  CONV_BINARY_TO_VARBINARY             =288,
  CONV_VARBINARY_TO_BINARY             =289,
  CONV_VARBINARY_TO_VARBINARY          =290,

  CONV_BINARY_TO_OTHER                 =291,
  CONV_VARBINARY_TO_OTHER              =292,
  CONV_OTHER_TO_BINARY                 =293,
  CONV_OTHER_TO_VARBINARY              =294
};

class  ex_conv_clause : public ex_clause {

  typedef struct {
    short type_op1; // left operand
    short type_op2; // right operand
    ConvInstruction instruction;
    const char * instrStr;
  } ConvInstrStruct;
  
public:

  // Construction
  //
  ex_conv_clause()
  {
    lastVOAoffset_ = 0;
    lastVcIndicatorLength_ = 0;
    lastNullIndicatorLength_ = 0;
  };
  ex_conv_clause(OperatorTypeEnum oper_type, 
			    Attributes ** attr, 
			    Space * space,
                            short num_operands = 2, 
			    NABoolean checkTruncErr = FALSE,
                            NABoolean reverseDataErrorConversionFlag = FALSE,
                            NABoolean noStringTruncWarnings = FALSE,
                            NABoolean convertToNullWhenErrorFlag = FALSE);


  // Values used for dataConvErrorFlag.
  enum ConvResult {CONV_RESULT_OK = 0,
		   CONV_RESULT_ROUNDED_DOWN = -1,
		   CONV_RESULT_ROUNDED_DOWN_TO_MAX = -2,
		   CONV_RESULT_ROUNDED_UP = 1,
		   CONV_RESULT_ROUNDED_UP_TO_MIN = 2,
		   CONV_RESULT_FAILED = 3,
		   CONV_RESULT_ROUNDED = 4,
                  }
  ;

  // Accessors
  // 
  void setInstruction();

  // Null Semantics
  //
  Int32 isNullInNullOut() const ;

  Int32 isNullRelevant() const { return 1; };
  ex_expr::exp_return_type processNulls(char *op_data[],
  						   CollHeap * = 0,
						   ComDiagsArea ** = 0);

  // Execution
  // 
  Int32 isEvalRelevant() const { return 1; };
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0);  

  // case index get and set
  ConvInstruction getInstruction()
  {
    if (getInstrArrayIndex() >= 0)
      return getInstruction(getInstrArrayIndex());
    else
      return CONV_NOT_SUPPORTED;
  };

  ConvInstruction findInstruction(short sourceType, Lng32 sourceLen,
                                             short targetType, Lng32 targetLen,
                                             Lng32 scaleDifference);

  NABoolean isConversionSupported(short sourceType, Lng32 srcLen, 
                                  short targetType, Lng32 tgtLen);

  static const ConvInstrStruct convInstrInfo[];
  static const char * getInstructionStr(Lng32 index) 
  { return convInstrInfo[index].instrStr;}
  static const ConvInstruction getInstruction(Lng32 index) 
  { return convInstrInfo[index].instruction;}

  static bool  sv_instrOffsetIndexPopulated;
  static short sv_MaxOpTypeValue;
  static int   *sv_convIndexSparse;
  static void  populateInstrOffsetIndex();
  static int   getInstrOffset(short pv_op1);

  Lng32 findIndexIntoInstrArray(ConvInstruction ci);

  NABoolean treatAllSpacesAsZero()
    { return ((flags_ & TREAT_ALL_SPACES_AS_ZERO) != 0); };

  void setTreatAllSpacesAsZero(NABoolean v)
    { (v) ? flags_ |= TREAT_ALL_SPACES_AS_ZERO : flags_ &= ~TREAT_ALL_SPACES_AS_ZERO; }

  NABoolean allowSignInInterval()
    { return ((flags_ & ALLOW_SIGN_IN_INTERVAL) != 0); };

  void setAllowSignInInterval(NABoolean v)
    { (v) ? flags_ |= ALLOW_SIGN_IN_INTERVAL : flags_ &= ~ALLOW_SIGN_IN_INTERVAL; }

  NABoolean noDatetimeValidation()
    { return ((flags_ & NO_DATETIME_VALIDATION) != 0); };

  void setNoDatetimeValidation(NABoolean v)
    { (v) ? flags_ |= NO_DATETIME_VALIDATION : flags_ &= ~NO_DATETIME_VALIDATION; }

  NABoolean srcIsVarcharPtr()   { return (flags_ & SRC_IS_VARCHAR_PTR) != 0; }
  void setSrcIsVarcharPtr(NABoolean v)
  { (v ? flags_ |= SRC_IS_VARCHAR_PTR : flags_ &= ~SRC_IS_VARCHAR_PTR); }
 
  // Fixup
  //
  Long pack(void *);
  ex_expr::exp_return_type 
    pCodeGenerate(Space *space, UInt32 flags);
  ex_expr::exp_return_type fixup(Space * space = 0,
					    CollHeap * exHeap = 0,
					    char * constants_area = 0,
					    char * temps_area = 0,
					    char * persistentArea = 0,
					    short fixupConstsAndTemps = 0,
					    NABoolean spaceCompOnly = FALSE);

  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
				  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

  void setLastVOAoffset( UInt32 v)
  {
    lastVOAoffset_ = v;
  }

  UInt32 getLastVOAoffset()
  {
    return lastVOAoffset_;
  }

  void setLastVcIndicatorLength( Int16 v)
  {
	  lastVcIndicatorLength_ = v;
  }
  void setLastNullIndicatorLength( Int16 v)
  {
    lastNullIndicatorLength_ = v;
  }

  void setComputedLength( UInt32 v)
  {
    computedLength_ = v;
  }
  UInt32 getComputedLength( )
  {
    return computedLength_;
  }
  void setAlignment( Int16 v)
  {
    alignment_ = v;
  }
  Int16 getAlignment()
  {
    return alignment_;
  }
private:
  enum flags_type 
    {
      REVERSE_DATA_ERROR_CONVERSION_FLAG = 0x0001,  // Reverse data error conversion flag
      
      // when converting string to numeric, if input string contains only
      // spaces or is a null string(length of zero), then the result becomes
      // zero(numeric value of 0).
      TREAT_ALL_SPACES_AS_ZERO           = 0x0002,
      ALLOW_SIGN_IN_INTERVAL             = 0x0004,
      NO_DATETIME_VALIDATION             = 0x0008,
       
      // source is a varchar value which is a pointer to the actual data.
      SRC_IS_VARCHAR_PTR                     = 0x0010,
      // when convert into error, suppress error, move null into convert target
      CONV_TO_NULL_WHEN_ERROR                = 0x0020
    };

  char        filler_[2];           // 00-01

  // Flags
  UInt16      flags_;               // 02-03
  UInt32      lastVOAoffset_;        //
  Int16       lastVcIndicatorLength_;  // 08-09
  Int16       lastNullIndicatorLength_;// 10-11
  UInt32      computedLength_;
  Int16       alignment_;

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char             fillers_[14];          // 04-31
};

inline Int32 ex_conv_clause::isNullInNullOut() const {
  return ((((ex_conv_clause *)this)->getNumOperands() > 2) ? 0 : 1 );
}

// the following function is used by ex_conv_clause::eval. In order to
// allow conversions from outside of an ex_conv_clause, this function
// is not a method of ex_conv_clause
enum ConvDoItFlags
{
  // while doing conversion, the string target will be blankpadded to the left.
  CONV_LEFT_PAD = 0x0001,

  // while converting ascii to interval, the sign('-', '+') is allowed in
  // the source string.
  CONV_ALLOW_SIGN_IN_INTERVAL = 0x0002,

  // allow invalid code values in the target (e.g, converting from Unicode to ascii)
  // If this bit is set, replace the invalid value with a replacement character.
  // By default (bit not set) an invalid code value results in an error.
  CONV_ALLOW_INVALID_CODE_VALUE = 0x0004,

  // Enable datetime validation when calling convDatetimeDatetime()
  // this is used in FCHECK support
  CONV_ENABLE_DATETIME_VALIDATION = 0x0008,

  // when converting string to numeric, if input string contains only
  // spaces or is a null string(length of zero), then the result becomes
  // zero(numeric value of 0).
  CONV_TREAT_ALL_SPACES_AS_ZERO = 0x0010,

  // invalid date/time/timestamps are allowed.
  // currently used for queries coming in from mariaquest.
  CONV_NO_DATETIME_VALIDATION = 0x0020,

  //Raising an expression error involves the use of convdoit to convert the
  //operands into strings. This flag ensure there is no infinite looping
  //between convdoit and ExRaiseDetailSqlError.
  //Setting the higher bits and allowing lowers ones for convdoit clauses.
  CONV_CONTROL_LOOPING = 0x0040,

  //The following flag is set when performing intermediate conversions so
  //that if the conversion fails, error report would specify it as a failure
  //of an intermediate conversion.
  CONV_INTERMEDIATE_CONVERSION = 0x0080,

  // during CAST from string to timestamp, a date value is extended with
  // zeroed out time part. This flag, if set, disables it.
  // Used when a TIMESTAMP literal is being created which requires the value
  // to exactly match the specified type. 
  CONV_NO_HADOOP_DATE_FIX  = 0x0010
};

// helper function for convDoIt and ex_conv_clause::pCodeGenerate:

// for conversions from REC_BYTE_F_ASCII, REC_BYTE_V_ASCII, or
// REC_BYTE_V_ANSI to one of these three types: Does the conversion
// involve charset conversions, check for partial characters, or check
// for max. number of characters?
inline int requiresNoConvOrVal(Lng32 sourceLen, Lng32 sourcePrecision, Lng32 sourceScale,
                               Lng32 targetLen, Lng32 targetPrecision, Lng32 targetScale,
                               ConvInstruction index)
{
  return (
       // ISO chars are ok - treat UNKNOWN as ISO88591 if the other operand is ISO88591 or unknown
       ((sourceScale == SQLCHARSETCODE_ISO88591 || sourceScale == SQLCHARSETCODE_UNKNOWN) &&
        (targetScale == SQLCHARSETCODE_ISO88591 || targetScale == SQLCHARSETCODE_UNKNOWN))
       
       ||
       
       (// source and target charsets are the same, and...
        targetScale == sourceScale &&

        // ...we do not need to check for partial characters 
        //    (multi-byte chars copied to a shorter target buffer), and...
        (targetLen >= sourceLen ||
         !(CharInfo::isVariableWidthMultiByteCharSet((CharInfo::CharSet) targetScale))) &&

        // ...we do not need to enforce a max. char limit
        //    (target precision is not set or no possibility of exceeding its limit)
        (targetPrecision == 0 ||
         ((targetPrecision >= sourcePrecision && sourcePrecision > 0 ||
           targetPrecision >= sourceLen) && index != CONV_ASCII_F_V)
        )));
}

ex_expr::exp_return_type
convDoIt(char * source,
         Lng32 sourceLen,
	 short sourceType,
	 Lng32 sourcePrecision,
	 Lng32 sourceScale,
	 char * target,
	 Lng32 targetLen,
	 short targetType,
	 Lng32 targetPrecision,
	 Lng32 scale,
         char * varCharLen,      // NULL if not a varChar
         Lng32 varCharLenSize,    // 0 if not a varChar
	 CollHeap *heap = 0,
	 ComDiagsArea** diagsArea = 0,
	 ConvInstruction index = CONV_UNKNOWN,
         Lng32 * dataConversionErrorFlag = 0,
	 ULng32 flags = 0);

ex_expr::exp_return_type
scaleDoIt(char *operand,              // ptr to operand
          Lng32 operandLen,           // len of operand
          Lng32 operandType,          // FS2 type of operand (current representation)
          Lng32 operandCurrScale,     // scale of operand,
                                      // could be charset if converted from char
          Lng32 newScale,             // scale the operand should have
          Lng32 typeOfOldScale,       // type of previous representation of operand,
                                      // to determine what scale really means
          CollHeap * heap);

ex_expr::exp_return_type
convAsciiToInt64(Int64 &target,
		 Lng32 targetScale,
		 char *source,
		 Lng32 sourceLen,
		 CollHeap *heap,
		 ComDiagsArea** diagsArea,
		 ULng32 flags);

ex_expr::exp_return_type 
convAsciiToFloat64(char * target,
		   char *source,
		   Lng32 sourceLen,
		   CollHeap *heap,
		   ComDiagsArea** diagsArea,
		   ULng32 flags);

inline void swapBytes(void *ptr, UInt32 size)
{
   switch (size)
   {
     case 2:
        *(UInt16 *)ptr = bswap_16(*(UInt16 *)ptr);
        break;
     case 4:
        *(UInt32 *)ptr = bswap_32(*(UInt32 *)ptr);
        break;
     case 8:
        *(Int64 *)ptr = bswap_64(*(Int64 *)ptr);
        break;
   }
}

ex_expr::exp_return_type swapBytes(Attributes *attr,
                                   void *ptr);
/////////////////////////////////////////
// Class ex_function_clause            //
/////////////////////////////////////////
class  ex_function_clause : public ex_clause {

public:
  // Construction
  //
  ex_function_clause(){};
  ex_function_clause(OperatorTypeEnum oper_type,
				short num_operands,
				Attributes ** attr,
				Space * space);


  // Null Semantics
  //
  Int32 isNullInNullOut() const { return 1; };
  Int32 isNullRelevant() const { return 1; };

  // Execution
  //
  Int32 isEvalRelevant() const { return 1; };

  // Fixup
  //
  Long pack(void *);
  ex_expr::exp_return_type 
    pCodeGenerate(Space *space, UInt32 flags);

  NABoolean derivedFunction()
    { return ((flags_ & DERIVED_FUNCTION) != 0); };

  void setDerivedFunction(NABoolean v)
    { (v) ? flags_ |= DERIVED_FUNCTION: flags_ &= ~DERIVED_FUNCTION; }

  NABoolean caseInsensitiveOperation()
    { return ((flags_ & CI_OPERATION) != 0); };

  void setCaseInsensitiveOperation(NABoolean v)
    { (v) ? flags_ |= CI_OPERATION: flags_ &= ~CI_OPERATION; }


  OperatorTypeEnum origFunctionOperType()
                       { return (OperatorTypeEnum)origFunctionOperType_; }
  void setOrigFunctionOperType(OperatorTypeEnum type)
                                  { origFunctionOperType_ = (Int16)type; }


  
  // Display
  //
  virtual void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

private:
  // there are times when a function is represented in terms of other
  // functions (See optimizer/BindItemExpr.cpp, class ZZZBinderFunction).
  // The next two fields are used to report errors for these cases.
  // The flags_, if sets to DERIVED_FUNCTION, indicates that this is a function
  // that was used to represent another function. origFunctionOperType_
  // contains the oper type for the original function. This field is valid
  // only if the DERIVED_FUNCTION bit is set in the flags_ field.
  enum
  {
    DERIVED_FUNCTION = 0x00000001,

    // if the function need to do caseinsnsitive operation. Used
    // by REPLACE string function.
    CI_OPERATION = 0x00000002
  };

  Int32 flags_;      // 00-03
  Int16 /*OperatorTypeEnum*/ origFunctionOperType_; // 04-05
  
  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char                       fillers_[10];           // 06-15
};

/////////////////////////////////////////
// Class ex_inout_clause
/////////////////////////////////////////
class  ex_inout_clause : public ex_clause {
public:
  // Consruction
  //
  ex_inout_clause() { flags_ = 0; };
  ex_inout_clause(OperatorTypeEnum oper_type, Attributes ** attr,
			     Space * space);


  // Accessors
  //
  inline char * getName(){ return name; };
  inline void setName(char * name_){ name = name_; };

  inline char * getHeading(){ return heading_; };
  inline void setHeading(char * heading){ heading_ = heading; };

  inline char * getTableName(){ return table_name_; };
  inline void setTableName(char * tn){ table_name_ = tn; };

  inline char * getSchemaName(){ return schema_name_; };
  inline void setSchemaName(char * sn){ schema_name_ = sn; };

  inline char * getCatalogName(){ return catalog_name_; };
  inline void setCatalogName(char * cn){ catalog_name_ = cn; };

  //  inline ex_conv_clause* &getConvClause(){ return convHVClause_; };

  // Null Semantics
  //
  Int32 isNullInNullOut() const { return 0; };
  Int32 isNullRelevant() const { return 0; };

  // Execution
  //
  Int32 isEvalRelevant() const { return 0; };
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0);  

  // Fixup
  //
  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);
  ex_expr::exp_return_type 
    pCodeGenerate(Space *space, UInt32 flags);

  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

  short getParamMode() { return paramMode_; }
  
  short getParamIdx() { return paramIdx_; }

  short getOrdPos() { return ordPos_; }

  void setParamMode (short paramMode) { paramMode_ = paramMode; }
  
  void setParamIdx(short paramIdx) { paramIdx_ = paramIdx; }

  void setOrdPos(short ordPos) { ordPos_ = ordPos; }

  NABoolean excludeFromBulkMove()
  {return (flags_ & EXCLUDE_FROM_BULK_MOVE) != 0;}
  void setExcludeFromBulkMove(NABoolean v)
  { (v ? flags_ |= EXCLUDE_FROM_BULK_MOVE : v &= ~EXCLUDE_FROM_BULK_MOVE);}

private:
  enum
  {
    // if set, this clause is not part of bulk move at runtime.
    // Set at runtime and used for rowwise rowset input.
    EXCLUDE_FROM_BULK_MOVE = 0x0001
  };

  NABasicPtr /* char* */ name;             // 00-07
  NABasicPtr /* char* */ heading_;         // 08-15
  NABasicPtr /* char */ table_name_;       // 16-23
  NABasicPtr /* char */ schema_name_;      // 24-31
  NABasicPtr /* char */ catalog_name_;     // 32-39

  UInt16  flags_;                          // 40-41
  char    filler_1[6];                     // 42-47


  //
  // UDR parameter info
  //
  short paramMode_;                        // 48-49
  short paramIdx_;                         // 50-51
  short ordPos_;                           // 52-53
  short udrFiller_;                        // 54-55

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char                       fillers_[16];           // 56-71
  
};

/////////////////////////////////////////
// Class noop_clause                   //
/////////////////////////////////////////
class  ex_noop_clause : public ex_clause {

public:
  // Construction
  //
  ex_noop_clause();


  // Null Semantics
  //
  Int32 isNullInNullOut() const { return 0; };
  Int32 isNullRelevant() const { return 0; };

  // Execution
  //
  Int32 isEvalRelevant() const { return 0; };
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0);  

  // Fixup
  //
  Long pack(void *);
  ex_expr::exp_return_type 
    pCodeGenerate(Space *space, UInt32 flags);

  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

private:   

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char                       fillers_[8];           // 00-07
 
};



/////////////////////////////////////////
// Class un_logic_clause               //
/////////////////////////////////////////
class  ex_unlogic_clause : public ex_clause {

public:
  // Construction
  //
  ex_unlogic_clause(){};
  ex_unlogic_clause(OperatorTypeEnum oper_type, 
			       Attributes ** attr, Space * space);


  // Null Semantics
  //
  Int32 isNullInNullOut() const { return 0; };
  Int32 isNullRelevant() const { return 0; };

  // Execution
  //
  Int32 isEvalRelevant() const { return 1; };
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0);  

  // Fixup
  //
  Long pack(void *);
  ex_expr::exp_return_type 
    pCodeGenerate(Space *space, UInt32 flags);

  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }

private:

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char                       fillers_[8];           // 00-07

  // ---------------------------------------------------------------------
};


class  ExRegexpClauseBase : public ex_clause {

public:
  // Construction
  //
  ExRegexpClauseBase() 
  {
    setCollation(CharInfo::DefaultCollation);
  }
  ExRegexpClauseBase(OperatorTypeEnum oper_type,
			    short num_operands,
			    Attributes ** attr,
			    Space * space) :
		ex_clause(ex_clause::LIKE_TYPE, oper_type,
                          num_operands, attr, space
                         ) 
  {
    setCollation(CharInfo::DefaultCollation);
  }


  // Null Semantics
  //
  Int32 isNullInNullOut() const { return 0; };
  Int32 isNullRelevant() const { return 1; };
  ex_expr::exp_return_type processNulls(char *op_data[],
  						   CollHeap * = 0,
						   ComDiagsArea ** = 0);

  // Execution
  //
  Int32 isEvalRelevant() const { return 1; };
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0) = 0;

  // Fixup
  //
  Long pack (void *) = 0;
  
  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea)
    {};


  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }

  inline char* getPatternStr()
  {
    return patternStr_;
  }

  inline void setPatternStr(char* pat)
  {
    patternStr_ = pat;
  }

  // Flags used in pcode implementation of like clauses
  enum
  {
    LIKE_HEAD = 0x01,  // Check for pattern at beginning of string only
    LIKE_TAIL = 0x02,  // Check for pattern at end of string only
    END       = 0xFF   // Last possible flag for use in pcode implementation
  };

  // ---------------------------------------------------------------------
protected:
  
  inline CharInfo::Collation getCollation()
  {
    return (CharInfo::Collation) collation_;
  }

  inline void setCollation(CharInfo::Collation v)
  {
    collation_ = (Int16) v;
  }

private:
union {
  LikePattern     *pattern_;          // 00-07
  char            *patternStr_;
};

  Int16 collation_; //08-09

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char                       fillers_[14];           // 10-23

};


// Added for unicode like function
class  ExRegexpClauseChar : public ExRegexpClauseBase {

public:
  // Construction
  //
  ExRegexpClauseChar() { rpattern_ = ""; };
  ~ExRegexpClauseChar() { if(rpattern_ != "") regfree(&reg); };
  ExRegexpClauseChar(OperatorTypeEnum oper_type, 
			    short num_operands,
			    Attributes ** attr,
			    Space * space);


  // Execution
  //
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0);

  // Fixup
  //
  Long pack (void *);

  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(2,getClassVersionID());
    ExRegexpClauseBase::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

  regex_t reg;

  NAString rpattern_; //previous pattern

private:

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char                       fillers_[8];           // 00-07

};
//////////////////////////////////
// Class like_clause            //
//////////////////////////////////

class  ex_like_clause_base : public ex_clause {

public:
  // Construction
  //
  ex_like_clause_base() 
  {
    setCollation(CharInfo::DefaultCollation);
  }
  ex_like_clause_base(OperatorTypeEnum oper_type,
			    short num_operands,
			    Attributes ** attr,
			    Space * space) :
		ex_clause(ex_clause::LIKE_TYPE, oper_type,
                          num_operands, attr, space
                         ) 
  {
    setCollation(CharInfo::DefaultCollation);
  }


  // Null Semantics
  //
  Int32 isNullInNullOut() const { return 0; };
  Int32 isNullRelevant() const { return 1; };
  ex_expr::exp_return_type processNulls(char *op_data[],
  						   CollHeap * = 0,
						   ComDiagsArea ** = 0);

  // Execution
  //
  Int32 isEvalRelevant() const { return 1; };
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0) = 0;

  // Fixup
  //
  Long pack (void *) = 0;
  ex_expr::exp_return_type pCodeGenerate(Space *space, UInt32 flags);

  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea)
    {};


  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_clause::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }

  inline char* getPatternStr()
  {
    return patternStr_;
  }

  inline void setPatternStr(char* pat)
  {
    patternStr_ = pat;
  }

  // Flags used in pcode implementation of like clauses
  enum
  {
    LIKE_HEAD = 0x01,  // Check for pattern at beginning of string only
    LIKE_TAIL = 0x02,  // Check for pattern at end of string only
    END       = 0xFF   // Last possible flag for use in pcode implementation
  };

  // ---------------------------------------------------------------------
protected:
  
  inline CharInfo::Collation getCollation()
  {
    return (CharInfo::Collation) collation_;
  }

  inline void setCollation(CharInfo::Collation v)
  {
    collation_ = (Int16) v;
  }

private:
union {
  LikePattern     *pattern_;          // 00-07
  char            *patternStr_;
};

  Int16 collation_; //08-09

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char                       fillers_[14];           // 10-23

};

// Added for unicode like function
class  ex_like_clause_char : public ex_like_clause_base {

public:
  // Construction
  //
  ex_like_clause_char() {};
  ex_like_clause_char(OperatorTypeEnum oper_type, 
			    short num_operands,
			    Attributes ** attr,
			    Space * space);


  // Execution
  //
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0);

  // Fixup
  //
  Long pack (void *);

  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
					  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(2,getClassVersionID());
    ex_like_clause_base::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

private:

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char                       fillers_[8];           // 00-07

};

class  ex_like_clause_doublebyte: public ex_like_clause_base {

public:
  // Construction
  //
  ex_like_clause_doublebyte() {};
  ex_like_clause_doublebyte(OperatorTypeEnum oper_type, 
			    short num_operands,
			    Attributes ** attr,
			    Space * space);


  // Execution
  //
  ex_expr::exp_return_type eval(char *op_data[],
					   CollHeap * = 0,
					   ComDiagsArea ** = 0);

  // Fixup
  //
  Long pack (void *);

  // Display
  //
  void displayContents(Space * space, const char * displayStr, 
				  Int32 clauseNum, char * constsArea);

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  };

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(2,getClassVersionID());
    ex_like_clause_base::populateImageVersionIDArray();
  };

  virtual short getClassSize() { return (short)sizeof(*this); };
  // ---------------------------------------------------------------------

private:
  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char                       fillers_[8];           // 00-07

};


#endif
















