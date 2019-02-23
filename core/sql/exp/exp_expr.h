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
#ifndef EXP_EXPR_H
#define EXP_EXPR_H

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

// -----------------------------------------------------------------------

#include "NAVersionedObject.h" // for NAVersionedObject
#include "ExpAtp.h"
#include "ExpPCode.h"
#include "dfs2rec.h"

// -----------------------------------------------------------------------
// Classes defined in this file
// -----------------------------------------------------------------------
class ex_expr;
class AggrExpr;
class InputOutputExpr;

/////////////////////////////////////////////////////////////
// Forward references
/////////////////////////////////////////////////////////////
class atp_struct;
class ex_cri_desc;
class tupp;
class Generator;
class NodeInfo; // for GUI
class Attributes;
class Descriptor;
class ex_conv_clause;
class PCode;
class ex_tcb;
class ex_expr_lean;

class ex_globals;

typedef ex_expr ex_expr_base;

/////////////////////////////////////////////////////////////////
// defines
/////////////////////////////////////////////////////////////////
#define NEG_BIT_MASK 0x0080    //the first bit in a byte - used for finding
                               //out whether the negative bit in a number
                               //is flipped

#define ExpAssert(p, msg) if (!(p)) { NAAssert(msg, __FILE__ , __LINE__ ); };

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for ExExpr
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<ex_expr> ExExprPtr;
typedef NAVersionedObjectPtrTempl<ex_expr_lean> ExExprLeanPtr;
typedef NAVersionedObjectPtrTempl<ex_expr_base> ExExprBasePtr;
typedef NAVersionedObjectPtrTempl<PCodeSegment> PCodeSegmentPtr;

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for ExExprPtr
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrArrayTempl<ExExprPtr> ExExprPtrPtr;

// ---------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for ExClausePtr
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<ex_clause> ExClausePtr;


/////////////////////////////////////////////////////////////
// Class ex_expr
/////////////////////////////////////////////////////////////
struct exp_eye_catcher {
  char name_[4];
};

class  NExDbgInfo {
private:

   char       * NExLogPath_     ;  // Ptr to Native Expr. Log Pathname
   char       * NExStmtSrc_     ;  // Ptr to orig. SQL statement source
   NABoolean    NExStmtPrinted_ ;  // Whether SQL statement has been printed
   Int32        NExDbgLvl_      ;  // Native Expr. Debug Level

public:
   NExDbgInfo() :
      NExLogPath_( NULL ),
      NExStmtSrc_( NULL ),
      NExStmtPrinted_( FALSE ),
      NExDbgLvl_(0)
  { }

  ~NExDbgInfo() {}

  char * getNExLogPath()                     { return NExLogPath_  ; }
  void   setNExLogPath( char * lgPth )       { NExLogPath_ = lgPth ; }

  char * getNExStmtSrc()                     { return NExStmtSrc_    ; }
  void   setNExStmtSrc( char * stmtSrc )     { NExStmtSrc_ = stmtSrc ; }

  NABoolean getNExStmtPrinted()              { return NExStmtPrinted_  ; }
  void  setNExStmtPrinted( NABoolean prntd ) { NExStmtPrinted_ = prntd ; }

  Int32 getNExDbgLvl()                       { return NExDbgLvl_ ; }
  void  setNExDbgLvl( Int32 Lvl )            { NExDbgLvl_ = Lvl ; }
};


class  ex_expr : public NAVersionedObject
{
  friend class NodeInfo;
  //----------------------------------------------------------------------
  // GSH : The following tdm_sqlcmdbg Dialog box class needs to access 
  // private member variables. 
  //----------------------------------------------------------------------
  friend class CDlgExprList;

public:
  // The possible types of expressions
  // (these codes must remain invariant across future versions)
  enum exp_node_type {
    exp_ANCHOR      =-1,
    exp_AGGR        = 1, // expression to evaluate aggregates
    exp_ARITH_EXPR  = 2, // expression results in an arith value
    exp_DP2_EXPR    = 3, // expression evaluated by DP2
    exp_INPUT_OUTPUT= 4, // expression to input/output values from/to caller
    exp_SCAN_PRED   = 5, // expression applied in executor/fs2
    exp_LEAN_EXPR   = 6,
    exp_last	    = 7  // not used
  };

  // Return type for expression methods
  //
  enum exp_return_type {
    EXPR_OK, EXPR_ERROR, EXPR_TRUE, EXPR_FALSE, EXPR_NULL, EXPR_PREEMPT
  };

  typedef exp_return_type (*evalPtrType)
    (PCodeBinary*, atp_struct*, atp_struct*, ex_expr*);

  // Read/Write mode for SQLMX disk format
  enum exp_mode {
    exp_UNKNOWN,
    exp_READ,
    exp_WRITE
  };

  // The ExpressionMode enumeration specifies the type of PCODE to generate
  // at expression fixup time.  Also, config's for error injection at 
  // fixup time.
  // PCODE_NONE - no PCODE
  // PCODE_ON - generate PCODE instead of clauses
  // PCODE_EVAL - simple PCODE_EVAL for every clause
  // PCODE_OPTIMIZE - PCODE optimization
  // PCODE_SPECIAL_FIELD - generate PCode for special fields
  // INJECT_ERROR - for testing error handling.
  // INJECT_WARNING - for testing error handling.

  enum ExpressionMode {
    PCODE_NONE=0x0000, PCODE_ON=0x0001, PCODE_EVAL=0x0002, 
    PCODE_OPTIMIZE=0x0004,
    PCODE_SPECIAL_FIELDS=0x0008,
    PCODE_NO_LEANER_EXPR=0x0010,
    PCODE_LLO=0x0020,

    // add new flags here
    INJECT_ERROR=0x2000,
    INJECT_WARNING=0x4000
  };

  // Construction/Destruction
  //
  ex_expr(exp_node_type type = exp_ANCHOR)
    : NAVersionedObject(type), 
      pCodeMaxOptBrCnt_(9999999),
      pCodeMaxOptInCnt_(9999999),
      warningInjection_   (0),   // Zero misc member vars for
      errorInjection_     (0),   // the sake of precaution.
      constantsAreaLength_(0),   // Note: There is an assumption
      persistentLength_   (0),   // that if these *Length vars are 0
      tempsAreaLength_    (0),   // the associated ptrs are meaningless.
      pCodeOptFlags_      (0),   //
      pCodeMode_          (0),   //
      length_             (0),   //
      flags_(0)
  {
    // Initialize eyeCatcher_ here in this form instead of 
    // in the initialization list because we don't want to copy the NULL terminator.

    eyeCatcher_.name_[0] = 'E';
    eyeCatcher_.name_[1] = 'X';
    eyeCatcher_.name_[2] = 'P';
    eyeCatcher_.name_[3] = 'R';

    // Initialize native expr offset to 0.
    evalPtrOff_ = 0;
    NExDbgInfo_ = NULL;
  }

  ~ex_expr() {}

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
  // ---------------------------------------------------------------------

  exp_node_type getType()
  {
    return (exp_node_type)getClassID();
  };

  // Accessors for Heap
  //
  inline void setHeap(CollHeap * heap)     { heap_ = heap; }
  inline CollHeap *getHeap()               { return heap_; }
  
  ex_expr_lean * castToExExprLean()
  {
    return (ex_expr_lean*)this;
  }

  ex_expr * castToExExpr()
  {
    return this;
  }

  // Accessors for interogation
  //
  virtual void displayContents(ComSpace * space, short mode,
					  const char * displayStr,ULng32 flag = 0x00000006);

  // Accessors for the clause linked list
  //
  ex_clause *getClauses(){
    return clauses_.getPointer();
  };

  void setClauses(ex_clause * clause)
  {
    clauses_ = clause;
  };
  
  void              linkClause(ex_clause*);

  // Accessors for the constants and temps areas and sizes
  //
  Lng32 getConstsLength()   { return constantsAreaLength_; };
   void setConstsLength(Lng32 constants_area_length)
                                             {constantsAreaLength_ = constants_area_length;}

  inline char *getConstantsArea()
  {
    return constantsArea_;
  }
  
  char *getMyConstantsArea()
  {
    return constantsArea_;
  }
  
   void setConstantsArea(char * constants_area)
  {constantsArea_ = constants_area;};
  
  inline char *getTempsArea()     
  {return tempsArea_;}
  char *getMyTempsArea()     
  {return tempsArea_;}
   void setTempsArea(char * temps_area)
  {tempsArea_ = temps_area;};

  Lng32 getTempsLength()
  {
    return tempsAreaLength_;
  };
  
  void setTempsLength(Lng32 tempsLength){
    tempsAreaLength_ = tempsLength;
  };

  void setLength(Lng32 length){
    length_ = length;
  };
  
  Lng32 getLength(){
    return length_;
  };
  
  // Packing, unpacking and fixup
  //
  // computeSpaceOnly: if TRUE, then compute space requirement only.
  //                Do not make any changes to the generated expressions,
  //                (like assigning tempsArea, assigning generated pcode, etc).
  virtual exp_return_type fixup(Lng32, unsigned short,
                                           const ex_tcb * tcb,
					   ComSpace * space,
					   CollHeap * exHeap,
					   NABoolean computeSpaceOnly,
					   ex_globals * glob);

  ////////////////////////////////////////////////////////////////////
  // eval() -- evaluates the expression.
  // fields rowLen, lastFldIndex and fetchedDataPtr are used for
  // update processing. See processUpdate() in ex_dp2_oper.cpp and
  // exp/exp_eval.cpp for details.
  ////////////////////////////////////////////////////////////////////
  inline exp_return_type 
       eval(atp_struct*atp1,
	    atp_struct*atp2,
	    CollHeap*exHeap = 0,
	    Lng32 datalen = -1,
	    ULng32 *rowLen = NULL,
            short *lastFldIndex = 0,
	    char *fetchedDataPtr = NULL)
  {
    // If there is no PCODE, evaluate the clauses using the standard
    // expression evaluator loop. For now, this will always be the case
    // for resource fork expressions.
    //
    PCodeBinary *pCode = getPCodeBinary();

    if (evalPtr_)
      return (*evalPtr_)(pCode, atp1, atp2, this);
    else if (pCode != NULL)
      return evalPCode(pCode, atp1, atp2, datalen, rowLen);
    else
      return evalClauses(getClauses(), atp1, atp2, datalen, rowLen, 
			 lastFldIndex, fetchedDataPtr);
  }

  ////////////////////////////////////////////////////////////////////
  // eval() -- evaluates the expression.
  // fields rowLen, lastFldIndex and fetchedDataPtr are used for
  // update processing. See processUpdate() in ex_dp2_oper.cpp and
  // exp/exp_eval.cpp for details.
  ////////////////////////////////////////////////////////////////////
  exp_return_type 
    evalPCodeAligned(PCodeBinary*pCode,
                     atp_struct*,
                     atp_struct*,
                     ULng32 *rowlen);

  exp_return_type 
    evalPCode(PCodeBinary*pCode,
	      atp_struct*,
	      atp_struct*,
              Lng32 datalen,
	      ULng32 *rowlen);

  // Inlined implementation of various expressions.  The number after "evalFast"
  // in the routine name specifies the opcode of the dominant pcode instruction
  // being inlined.

  //Moves
  static exp_return_type
  evalFast4(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast202(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast203(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);


  //Compares
  static exp_return_type
  evalFast33(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast36(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast37(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast40(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast117(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast130(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast131(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast132(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast133(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast162(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast262(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast275(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  // Hash
  static exp_return_type
  evalFast94(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast170(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast171(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast173(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static exp_return_type
  evalFast223(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);


  exp_return_type
    reportOverflowError(atp_struct*,
                        PCodeBinary* pCodeOpcode,
                        PCodeBinary* pCode,
                        Long* stack);

  exp_return_type 
    evalClauses(ex_clause *, 
		atp_struct *,
		atp_struct *,
		Lng32 datalen,
		ULng32 *rowLen,
		short *lastFldIndex,
		char * fetchedDataPtr);
  
  virtual char *findVTblPtr(short classID);

  virtual Long pack(void *);
  virtual Lng32 unpack(void *, void * reallocator);
  void packStuff(void *);

  void makeLeanCopyAll(ComSpace * space);
  void makeLeanCopy(ex_expr_lean * other, ComSpace * space);

  inline Long packExpr(void * space) {
    return ex_expr::pack(space);
  }

  inline Lng32 unpackExpr(void * base, void * reallocator) {
    return ex_expr::unpack(base, reallocator);
  }

  // called to initialized clauses before evaluating the expr.
  // Currently only called to initialize LOB related expr clauses.
  Lng32 initExpr();

  // Accessors and Methods for PCode
  //
  PCodeBinary *getPCodeBinary() 
  { 
    return (pCode_.getPointer() ? pCode_.getPointer()->getPCodeBinary() : NULL);
  };

  void setPCodeLean(PCodeBinary *pCode)
  {
    pCodeLean_ = pCode;
  }

  PCode *getPCodeObject()  
  { 
    return pCodeObject_; 
  }
  
  PCodeSegment * getPCodeSegment()
  { return pCode_.getPointer(); }

  Int32 getPCodeSize()
  {
    return pCode_.getPointer()->getPCodeSegmentSize();
  }

  virtual void setPCodeBinary(PCodeBinary *pCode)  
                                    { pCode_.getPointer()->setPCodeBinary(pCode); }
  void setPCodeObject(PCode *pCodeObject)
                                    {pCodeObject_ = pCodeObject; }
  Int16 getPCodeMode()
                                    { return pCodeMode_ ; }
  void setPCodeMode(Int16 mode)
                                    { pCodeMode_ = mode; }
  UInt32 getPCodeOptFlags()
                                    { return pCodeOptFlags_; }
  void setPCodeOptFlags(UInt32 flags)
                                    { pCodeOptFlags_ = flags; }
  UInt32 getPCodeMaxOptBrCnt()
                                    { return pCodeMaxOptBrCnt_; }
  UInt32 getPCodeMaxOptInCnt()
                                    { return pCodeMaxOptInCnt_; }
  void setPCodeMaxOptBrCnt(UInt32 cnt)
                                    { pCodeMaxOptBrCnt_ = cnt; }
  void setPCodeMaxOptInCnt(UInt32 cnt)
                                    { pCodeMaxOptInCnt_ = cnt; }
  void setEvalPtr( NABoolean isSamePrimary );

  void setEvalPtr(evalPtrType e) { evalPtr_ = e; }

  void setNExDbgInfo(NExDbgInfo * NExDbgPtr)
                            { NExDbgInfo_ =  NExDbgPtr; }

  NExDbgInfo * getNExDbgInfoPtr() { return NExDbgInfo_ ; }

  Long getEvalPtrOffset() { return evalPtrOff_ ; }

  exp_return_type pCodeGenerate(ComSpace *space, CollHeap *heap,
					   UInt32 flags);
  void pCodePrint();

  // Takes pointer out of PCodeBinary sequences
  // Long getPCodeBinaryAsPtr(PCodeBinary *pcode, Int32 idx)
  //   {
  //     return *(Long*)&pcode[idx];
  //   }

  // Adds pointer to PCodeBinary sequences and returns # of PCodeBinary occupied
  Int32 setPtrAsPCodeBinary(PCodeBinary *pcode, Int32 idx, Long ptr)
    {
      *(Long*)&pcode[idx] = ptr;
      return (sizeof(ptr)/sizeof(PCodeBinary));
    }


  // Accessors/Mutators and Methods for persisent expression storage.
  //
  inline char * getPersistentArea() 
  { 
    return persistentArea_;
  }

  char * getMyPersistentArea() 
  { 
    return persistentArea_;
  }

  char *getPersistentData(Int32 offset =0);
  char* &persistentArea() { return persistentArea_.pointer(); }
  char* &persistentInitializationArea() 
  { return persistentInitializationArea_.pointer(); }
  Int32 &persistentLength() { return persistentLength_; }
  void initializePersistentData() { 
    str_cpy_all(persistentArea_, 
		persistentInitializationArea_,
		persistentLength_);
  }

  void addPCodeMode(Int16 mode)
                                    { pCodeMode_ |= mode; }

  NABoolean pCodeSpecialFields() {
    return (pCodeMode_ & PCODE_SPECIAL_FIELDS) != 0;
  };

  void setFixupConstsAndTemps(short v) {
    if (v != 0) flags_ |= FIXUP_CONSTS_AND_TEMPS;
    else flags_ &= ~FIXUP_CONSTS_AND_TEMPS;
  };
  NABoolean getFixupConstsAndTemps() {
    return ((flags_ & FIXUP_CONSTS_AND_TEMPS) != 0);
  };

  NABoolean generateNoPCode() {
    return ((flags_ & GENERATE_NO_PCODE) != 0);
  }

  void setGeneratePcode(short v) {
    if (v != 0) flags_ |= GENERATE_NO_PCODE;
    else flags_ &= ~GENERATE_NO_PCODE;
  }

  NABoolean getPCodeGenCompile() {
    return ((flags_ & PCODE_GEN_COMPILE) != 0);
  }

  void setPCodeGenCompile (short v) {
    if (v != 0) 
      flags_ |= PCODE_GEN_COMPILE;
    else flags_ &= ~PCODE_GEN_COMPILE;
  }

  void setPCodeNative(NABoolean v) {
    if (v) flags_ |= PCODE_EVAL_NATIVE;
    else flags_ &= ~PCODE_EVAL_NATIVE;
  };

  NABoolean getPCodeNative() {
    return ((flags_ & PCODE_EVAL_NATIVE) != 0);
  };

  NABoolean getNEInShowplan() {
    return ((flags_ & PCODE_NE_IN_SHOWPLAN) != 0);
  };

  void setNEInShowplan(NABoolean v) {
   ( v ? flags_ |= PCODE_NE_IN_SHOWPLAN :
         flags_ &= ~PCODE_NE_IN_SHOWPLAN );
  };

  void setPCodeMoveFastpath(NABoolean v) {
    if (v) flags_ |= PCODE_MOVE_FASTPATH;
    else flags_ &= ~PCODE_MOVE_FASTPATH;
  };
  NABoolean getPCodeMoveFastpath() {
    return ((flags_ & PCODE_MOVE_FASTPATH) != 0);
  };

  void setForInsertUpdate(NABoolean value)
  {
    if (value)
      flags_ |= FOR_INSERT_UPDATE;
    else
      flags_ &= ~FOR_INSERT_UPDATE;
  };
  NABoolean forInsertUpdate() {
    return ((flags_ & FOR_INSERT_UPDATE) != 0);
  };

  void setPCodeEvalAligned(NABoolean value)
  {
    if (value)
      flags_ |= PCODE_EVAL_ALIGNED;
    else
      flags_ &= ~PCODE_EVAL_ALIGNED;
  };
  NABoolean usePCodeEvalAligned() {
    return ((flags_ & PCODE_EVAL_ALIGNED) != 0);
  };

  void setHandleIndirectVC(NABoolean value) {
    if (value) 
      flags_ |= HANDLE_INDIRECT_VC;
    else 
      flags_ &= ~HANDLE_INDIRECT_VC;
  };

  NABoolean handleIndirectVC() {
    return ((flags_ & HANDLE_INDIRECT_VC) != 0);
  };

  static NABoolean forShowplan(UInt32 f) {return ((f & FOR_SHOWPLAN) != 0);}
  static void setForShowplan(UInt32 &f, NABoolean v)
  { ( v ? f |= FOR_SHOWPLAN : f &= ~FOR_SHOWPLAN );}

  static NABoolean notValidateFloat64(UInt32 f)
  { return ((f & NOT_VALIDATE_FLOAT64) != 0); }
  static void setNotValidateFloat64(UInt32 &f, NABoolean v)
  { ( v ? f |= NOT_VALIDATE_FLOAT64 : f &= ~NOT_VALIDATE_FLOAT64 );}

    static int formatARow2(const char** srcFldsPtr,
                          const int* srcFldsLength,
                          const int * srcFieldsConvIndex,
                          char* formattedRow,
                          int& formattedRowLength,
                          int numAttrs,
                          AttributesPtr * attrs,
                          ExpTupleDesc *tupleDesc,
                          UInt16 firstFixedOffset,
                          UInt16 bitmapEntryOffset,
                          UInt16 bitmapOffset,
                          NABoolean sysKeyTable,
                          CollHeap *heap,
                          ComDiagsArea **diags);
						  
#ifdef COMING_FROM_NATIVE_EXPR_SOURCE_FILE
       // Native Expression code needs to be able to calculate the address
       // of the constantsArea_ and tempsArea_ pointers.  However, doing so
       // is not allowed by C++ if the following data is declared 'protected'
       // as we want to do.  So, we declare it 'public' ONLY WHEN this header
       // file is #included by Native Expression code and 'protected' otherwise.
       // The Native Expression code will access the ex_expr object
       // ONLY for reading -- absolutely no modifying!
public:

#else
protected:

#endif // COMING_FROM_NATIVE_EXPR_SOURCE_FILE
  // Bookkeeping
  //
  enum Flags
  {
    // this flag, if set, indicates that the 'address' of
    // constants and temps could be computed once at fixup
    // time and assigned to the corresponding 'offset'
    // fields in Attributes. To be set if we know for sure
    // that the expression will not be shared among many
    // TCBs at runtime.
    FIXUP_CONSTS_AND_TEMPS = 0x0001,

    // set, if this expression has been fixed up.
    // No longer used
    FIXED_UP = 0x0002,

    // set if this expression wants no PCODE
    GENERATE_NO_PCODE = 0x0004,

    // set if pcode was generated at compile time
    PCODE_GEN_COMPILE = 0x0008,

    // do pcode move fastpath optimization.
    // In this case, we move source to target directly without going
    // through expr eval at runtime.
    // Done if there is only one pcode instruction, MOVE_MBIN8_MBIN8_IBIN32S.
    PCODE_MOVE_FASTPATH = 0x0010,

    // This expression is for an insert or an update to ensure that the
    // first fixed field is cleared.  If there are no fixed fields this value
    // remains 0.
    // See usage in evalClauses to clear VOA[0] (the first fixed offset).
    FOR_INSERT_UPDATE = 0x0020,

    // If set, then call pcodeEval method that is compiled with refaligned 8.
    // Assumes everything is aligned.
    PCODE_EVAL_ALIGNED = 0x0040,

    // this flag is used to indicate that pcode can be generated if this expression
    // involves varchars. 
    HANDLE_INDIRECT_VC = 0x0080,

    // If set, native code was generated for this expression.  The function
    // pointer evalPtr_ points to an offset in the constants area of the expr
    // where the code resides.
    PCODE_EVAL_NATIVE = 0x0100,

    // If set, SHOWPLAN will print native code
    PCODE_NE_IN_SHOWPLAN = 0x0200
  };
  
  // flags passed to exp and clause generation methods.
  enum GenFlags {
    // expression being generated for SHOWPLAN request
    FOR_SHOWPLAN = 0x0001,

    // tell clause pcodegenerate() to generate PCode instruction
    // to validate value range for float64 to float64 assigments
    NOT_VALIDATE_FLOAT64 = 0x0002
  };

  exp_eye_catcher         eyeCatcher_;                           // 00-03

  // internal flags
  UInt32                  flags_;                                // 04-07

  // heap will be set by the executor?
  CollHeap               *heap_;

  union                                                          // 16-19
  {
    PCodeBinary * pCodeLean_;
    PCode * pCodeObject_;
  };

  PCodeSegmentPtr         pCode_;                                // 24-31

  ExClausePtr             clauses_;                              // 32-39

  // For error injection testing.
  const ex_tcb           *myTcb_;

  Int32                   errorInjection_;                       // 48-51
  Int32                   warningInjection_;                     // 52-55

  // Pointer and length of the constants area
  NABasicPtr /* char* */  constantsArea_;                        // 56-63

  // Pointer and length of the temps area
  NABasicPtr /* char* */  tempsArea_;                            // 64-71

  Int32                   constantsAreaLength_;                  // 72-75
  Int32                   tempsAreaLength_;                      // 76-79

  Int32                   length_;                               // 80-83

  // Persistent Area
  Int32                   persistentLength_;                     // 84-87
  NABasicPtr /* char* */  persistentArea_;                       // 88-95
  NABasicPtr /* char* */  persistentInitializationArea_;         // 96-103

  // Set the PCode mode that is used when generating the PCode
  // during compilation so if it must be regenerated at runtime
  // the same mode is used.
  Int16                   pCodeMode_;                            // 104-105

  // Filler to get back onto a 4-byte boundary
  Int16                   fillerPCodeOptFlags_;                  // 106-107

  // Set flags used for advanced low-level pcode optimizations
  UInt32                  pCodeOptFlags_;                        // 108-111

  // Fast eval pointer
  union
  {
    evalPtrType           evalPtr_;                              // 112-115
    long                  evalPtrOff_;
    NExDbgInfo         *  NExDbgInfo_ ;
  };

  UInt32                  pCodeMaxOptBrCnt_;                     // 120-123
  UInt32                  pCodeMaxOptInCnt_;                     // 124-127

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char                   fillers_[8];                           // 128-135

  void fixupNextClause();

};


//////////////////////////////////////////////////
// class AggrExpr
//////////////////////////////////////////////////
class  AggrExpr : public ex_expr {

  enum Flags {
    ONE_ROW_AGGR = 0x0001
  };

  ExExprPtr initExpr_;          // 00-07
  ExExprPtr perrecExpr_;        // 08-15
  ExExprPtr finalNullExpr_;     // 16-23
  ExExprPtr finalExpr_;         // 24-31
  ExExprPtr groupingExpr_;      // 32-39
  UInt32    flags_;             // 40-43

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char                   fillers_[12];  // 44-55

public:
  AggrExpr() : ex_expr(ex_expr::exp_AGGR), flags_(0) {}

  AggrExpr( ex_expr * init_expr, 
	    ex_expr * perrec_expr, 
	    ex_expr * final_expr, 
	    ex_expr * final_null_expr,
            ex_expr * grouping_expr
	  )
  : ex_expr(ex_expr::exp_AGGR), 
  initExpr_(init_expr),
  perrecExpr_(perrec_expr), 
  finalExpr_(final_expr),
  finalNullExpr_(final_null_expr),
  groupingExpr_(grouping_expr),
  flags_(0)
    {}
 
~AggrExpr() {}

 ex_expr * perrecExpr()   { return perrecExpr_;}
 ex_expr * groupingExpr() { return groupingExpr_;}

  exp_return_type initializeAggr(atp_struct *);
  exp_return_type finalizeAggr(atp_struct *);
  exp_return_type finalizeNullAggr(atp_struct *);

 exp_return_type evalGroupingForNull(Int16 startEntry, Int16 endEntry);

  virtual Long pack(void *);
  virtual Lng32 unpack(void *, void * reallocator);
  virtual exp_return_type fixup(Lng32, unsigned short, const ex_tcb * tcb,
				ComSpace * space = 0,
				CollHeap * exHeap = 0,
				NABoolean computeSpaceOnly = FALSE,
				ex_globals * glob = NULL);
  virtual void displayContents(ComSpace * space, short mode, const char * displayStr,
                               ULng32 flag);


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
    ex_expr::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

  void       setOneRowAggr()  { flags_ |= ONE_ROW_AGGR; }
  NABoolean  isOneRowAggr()
                 { return( (flags_ & ONE_ROW_AGGR) != 0); }
};

// -----------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for InputOutputExpr
// -----------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<InputOutputExpr> InputOutputExprPtr;


///////////////////////////////////////////////////
// class InputOutputExpr
///////////////////////////////////////////////////
class InputOutputExpr : public ex_expr {
  enum Flags
  {
    // do not evaluate io expr at runtime, it only contains inout_clauses
    // which evaluate to a NO_OP.
    NO_EVAL = 0x0001,

    // skip type compatibility check at runtime between the user hvar
    // and the input/output value. 
    // Skipping the check will make all conversions
    // (if they are possible) go through. So a user hvar(char,
    // for example) could be converted to an integer input value.
    // Or vica versa. This type check is currently skipped for
    // ODBC users only. See Generator/GenExpGenerator.cpp.
    SKIP_TYPE_CHECK = 0x0002,

    // skip type check for certain restricted externalized conversions
    // only. Right now, we only support datetime&binary to/from string.
    // This flag is valid is previous flag is set.
    RESTRICTED_SKIP_TYPE_CHECK = 0x0004,

    // invalid date/time/timestamps are allowed.
    // currently used for queries coming in from mariaquest.
    NO_DATETIME_VALIDATION = 0x0008,

    // for compatibility with older versions (Trafodion 2.2.0 or
    // earlier), suppress errors for a target assignment and
    // warnings for a retrieval assignment where we need to
    // check whether a UTF-8 string has more characters than
    // the target data types allows
    SUPPRESS_CHAR_LENGTH_LIMIT_CHECKS = 0x0010

  };

  enum DescribeFlags
  {
    IS_ODBC_ = 0x0001,
    IS_DBTR_ = 0x0002,
    IS_RWRS_ = 0x0004,
    INTERNAL_FORMAT_IO_ = 0x0008
  };

  // number of input or output entries
  Int32           numEntries_;           // 00-03

  UInt32 flags_;                         // 04-07

  UInt32 isCall_;			 // 08-11 

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char            fillersExExpr_[20];    // 12-31

public:

   void setCall(NABoolean flag) { isCall_ = (UInt32)flag; }
   NABoolean isCall() { return (NABoolean)isCall_; }

  InputOutputExpr() 
    : ex_expr(ex_expr::exp_INPUT_OUTPUT),
      flags_(0), isCall_(0),
      numEntries_(-1)
  {}

  InputOutputExpr(Generator * generator);
  ~InputOutputExpr() {}

  ex_expr::exp_return_type describeInput(void*, void*, UInt32);
  ex_expr::exp_return_type describeOutput(void*, UInt32);

  ex_expr::exp_return_type addDescInfoIntoStaticDesc(Descriptor* desc,
						     NABoolean isInput);


  Lng32 getMaxParamIdx();

  ex_expr::exp_return_type inputSingleRowValue
  (atp_struct*,
   void*,
   char*,
   CollHeap *exHeap,
   UInt32 flags);
  
  ex_expr::exp_return_type inputRowwiseRowsetValues
  (atp_struct*,
   void*,
   void*,
   NABoolean tolerateNonFatalError,  
   CollHeap *exHeap,
   UInt32 flags);

  ex_expr::exp_return_type inputValues
  (atp_struct *atp,
   void * inputDesc_,
   NABoolean tolerateNonFatalError,
   CollHeap * heap,
   UInt32 flags);

  ex_expr::exp_return_type outputValues(atp_struct*,
							void*, 
                                                        CollHeap *exHeap,
							UInt32 flags);

   Lng32 getCompiledOutputRowsetSize(atp_struct *);

  virtual Long pack(void *);

   Lng32 getNumEntries(){
    return numEntries_;
  };
  
   void setNumEntries(Lng32 entries){
    numEntries_ = entries;
  };

  NABoolean noEval() 
  { return (flags_ & NO_EVAL) != 0; };

  void setNoEval(NABoolean v)      
  { (v ? flags_ |= NO_EVAL : flags_ &= ~NO_EVAL); }

  NABoolean skipTypeCheck() 
  { return (flags_ & SKIP_TYPE_CHECK) != 0; };

  void setSkipTypeCheck(NABoolean v)      
  { (v ? flags_ |= SKIP_TYPE_CHECK : flags_ &= ~SKIP_TYPE_CHECK); }

  NABoolean restrictedSkipTypeCheck() 
  { return (flags_ & RESTRICTED_SKIP_TYPE_CHECK) != 0; };

  void setRestrictedSkipTypeCheck(NABoolean v)      
  { (v ? flags_ |= RESTRICTED_SKIP_TYPE_CHECK : flags_ &= ~RESTRICTED_SKIP_TYPE_CHECK); }

  NABoolean suppressCharLimitCheck()
  { return (flags_ & SUPPRESS_CHAR_LENGTH_LIMIT_CHECKS); }

  void setSuppressCharLimitCheck(NABoolean v)
  { (v ? flags_ |= SUPPRESS_CHAR_LENGTH_LIMIT_CHECKS
       : flags_ &= ~SUPPRESS_CHAR_LENGTH_LIMIT_CHECKS); }

  NABoolean isDBTR(UInt32 flags) 
  { return (flags & IS_DBTR_) != 0; };

  void setIsDBTR(UInt32 &flags, NABoolean v)      
  { (v ? flags |= IS_DBTR_ : flags &= ~IS_DBTR_); }

  NABoolean isODBC(UInt32 flags) 
  { return (flags & IS_ODBC_) != 0; };

  void setIsODBC(UInt32 &flags, NABoolean v)      
  { (v ? flags |= IS_ODBC_ : flags &= ~IS_ODBC_); }

  NABoolean isRWRS(UInt32 flags) 
  { return (flags & IS_RWRS_) != 0; };

  void setIsRWRS(UInt32 &flags, NABoolean v)      
  { (v ? flags |= IS_RWRS_ : flags &= ~IS_RWRS_); }

  NABoolean isInternalFormatIO(UInt32 flags) 
  { return (flags & INTERNAL_FORMAT_IO_) != 0; };

  void setInternalFormatIO(UInt32 &flags, NABoolean v)
  { (v ? flags |= INTERNAL_FORMAT_IO_ : flags &= ~INTERNAL_FORMAT_IO_); }

  NABoolean noDatetimeValidation()
    { return ((flags_ & NO_DATETIME_VALIDATION) != 0); };

  void setNoDatetimeValidation(NABoolean v)
    { (v) ? flags_ |= NO_DATETIME_VALIDATION : flags_ &= ~NO_DATETIME_VALIDATION; }

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
    ex_expr::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }

  void setupBulkMoveInfo(void * desc_, CollHeap * heap,
					 NABoolean isInputDesc,
					 UInt32 flags);
  ex_expr::exp_return_type doBulkMove(atp_struct * atp, 
						      void * desc_, 
						      char * tgtRowPtr,
						      NABoolean isInputDesc);

  /////////////////////////////////////////////////////////////////////
  // return TRUE, if source and target are compatible types.
  // Compatible types are: char/varchar & char/varchar
  //                       numeric & numeric
  //                       datetime & datetime
  //                       interval & interval
  //        For datetime and interval types, a detailed compatibility
  //        (matching qualifiers) is done later at conversion time.
  /////////////////////////////////////////////////////////////////////
  static NABoolean areCompatible(short source, short target)
  {
    if ((((source >= REC_MIN_NUMERIC) && (source <= REC_MAX_NUMERIC)) &&
	 ((target >= REC_MIN_NUMERIC) && (target <= REC_MAX_NUMERIC))) ||
        ((DFS2REC::isCharacterString(source)) && 
         (DFS2REC::isCharacterString(target))) ||
        ((DFS2REC::isBinaryString(source)) && 
         (DFS2REC::isBinaryString(target))) ||
	((source == REC_DATETIME) && (target == REC_DATETIME)) ||
	(((source >= REC_MIN_INTERVAL) && (source <= REC_MAX_INTERVAL)) &&
	 ((target >= REC_MIN_INTERVAL) && (target <= REC_MAX_INTERVAL))) ||
	((source == REC_BLOB) && (target == REC_BLOB)) ||
	((source == REC_CLOB) && (target == REC_CLOB)) ||
        ((source == REC_BOOLEAN) && (target == REC_BOOLEAN)))
      return TRUE;
    else
      return FALSE;
  }
	
  static NABoolean implicitConversionSupported(short source, short target)
  {
    if ((((source >= REC_MIN_CHARACTER) && (source <= REC_MAX_CHARACTER)) &&
	 ((target == REC_DATETIME) || ((target >= REC_MIN_BINARY_NUMERIC) && (target <= REC_MAX_BINARY_NUMERIC)))) ||
	(((target >= REC_MIN_CHARACTER) && (target <= REC_MAX_CHARACTER)) &&
	 ((source == REC_DATETIME) || ((source >= REC_MIN_BINARY_NUMERIC) && (source <= REC_MAX_BINARY_NUMERIC)))))
      return TRUE;
    else
      return FALSE;
  }

  NABoolean isExactNumeric(short type)
  {
    if ((type >= REC_MIN_BINARY_NUMERIC) && (type <= REC_MAX_BINARY_NUMERIC) ||
	(type >= REC_MIN_DECIMAL) && (type <= REC_MAX_DECIMAL))
      return TRUE;
    else
      return FALSE;
  }
  
  // ---------------------------------------------------------------------

};


class  ex_expr_lean : public ex_expr
{
  friend class ex_expr;
  //  Int32                   pCodeSize_;         

public:
  ex_expr_lean()
       : ex_expr(exp_LEAN_EXPR)
  {
  };

  // Packing, unpacking and fixup
  //
  // computeSpaceOnly: if TRUE, then compute space requirement only.
  //                Do not make any changes to the generated expressions,
  //                (like assigning tempsArea, assigning generated pcode, etc).
  virtual exp_return_type fixup(Lng32, unsigned short,
                                           const ex_tcb * tcb,
					   ComSpace * space = 0,
					   CollHeap * exHeap = 0,
					   NABoolean computeSpaceOnly = FALSE);

  virtual Long pack(void *);
  virtual Lng32 unpack(void *, void * reallocator);

  void convAddrToOffsetInPCode(void * space);
  void convOffsetToAddrInPCode(void* base);

  virtual void displayContents(ComSpace * space, short mode,
					  const char * displayStr,
					  ULng32 flag = 0x00000006);

  //  Int32 *getMyPCode() 
  //  { 
  //    return &pCodeLean_[1];
  //  }

  Int32 getPCodeSize()
  {
    return pCodeLean_[0];
    //    return pCodeSize_;
  }

  //  void setPCodeSize(Int32 s) 
  //  {
  //    pCodeLean_[0] = s;
    //    pCodeSize_ = s;
  //  }

  //  void setPCode(Int32 *pCode)  
  //  { 
  //    setPCodeLean(pCode);
  //  }

};


 
///////////////////////////////////////////////////
// class Target
///////////////////////////////////////////////////
template <class t> class Target {
public:
	t operator=(t source)
	{
          t *targetBuffer;
          targetBuffer = (t *)this;
          *targetBuffer = source;
          return source;
	}
};


#endif

