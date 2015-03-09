/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
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

#include "SqlExpDllDefines.h"
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

class SQLEXP_LIB_FUNC  NExDbgInfo {
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

#pragma warning ( disable : 4251 )

class SQLEXP_LIB_FUNC  ex_expr : public NAVersionedObject
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
  NA_EIDPROC ex_expr(exp_node_type type = exp_ANCHOR)
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

  NA_EIDPROC ~ex_expr() {}

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  NA_EIDPROC virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  NA_EIDPROC virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  NA_EIDPROC virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

  NA_EIDPROC exp_node_type getType()
  {
    return (exp_node_type)getClassID();
  };

  // Accessors for Heap
  //
  NA_EIDPROC inline void setHeap(CollHeap * heap)     { heap_ = heap; }
  NA_EIDPROC inline CollHeap *getHeap()               { return heap_; }
  
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
  NA_EIDPROC virtual void displayContents(ComSpace * space, short mode,
					  const char * displayStr,ULng32 flag = 0x00000006);

  // Accessors for the clause linked list
  //
  NA_EIDPROC ex_clause *getClauses(){
    return clauses_.getPointer();
  };

  NA_EIDPROC void setClauses(ex_clause * clause)
  {
    clauses_ = clause;
  };
  
  NA_EIDPROC void              linkClause(ex_clause*);

  // Accessors for the constants and temps areas and sizes
  //
  NA_EIDPROC Lng32 getConstsLength()   { return constantsAreaLength_; };
  NA_EIDPROC  void setConstsLength(Lng32 constants_area_length)
                                             {constantsAreaLength_ = constants_area_length;}

  NA_EIDPROC inline char *getConstantsArea()
  {
    return constantsArea_;
  }
  
  NA_EIDPROC char *getMyConstantsArea()
  {
    return constantsArea_;
  }
  
  NA_EIDPROC  void setConstantsArea(char * constants_area)
  {constantsArea_ = constants_area;};
  
  NA_EIDPROC inline char *getTempsArea()     
  {return tempsArea_;}
  NA_EIDPROC char *getMyTempsArea()     
  {return tempsArea_;}
  NA_EIDPROC  void setTempsArea(char * temps_area)
  {tempsArea_ = temps_area;};

  NA_EIDPROC Lng32 getTempsLength()
  {
    return tempsAreaLength_;
  };
  
  NA_EIDPROC void setTempsLength(Lng32 tempsLength){
    tempsAreaLength_ = tempsLength;
  };

  NA_EIDPROC void setLength(Lng32 length){
    length_ = length;
  };
  
  NA_EIDPROC Lng32 getLength(){
    return length_;
  };
  
  // Packing, unpacking and fixup
  //
  // computeSpaceOnly: if TRUE, then compute space requirement only.
  //                Do not make any changes to the generated expressions,
  //                (like assigning tempsArea, assigning generated pcode, etc).
  NA_EIDPROC virtual exp_return_type fixup(Lng32, unsigned short,
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
      return evalPCode(pCode, atp1, atp2, rowLen);
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
  NA_EIDPROC exp_return_type 
    evalPCodeAligned(PCodeBinary*pCode,
                     atp_struct*,
                     atp_struct*,
                     ULng32 *rowlen);

  NA_EIDPROC exp_return_type 
    evalPCode(PCodeBinary*pCode,
	      atp_struct*,
	      atp_struct*,
	      ULng32 *rowlen);

  // Inlined implementation of various expressions.  The number after "evalFast"
  // in the routine name specifies the opcode of the dominant pcode instruction
  // being inlined.

  //Moves
  static NA_EIDPROC exp_return_type
  evalFast4(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast202(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast203(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);


  //Compares
  static NA_EIDPROC exp_return_type
  evalFast33(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast36(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast37(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast40(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast117(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast130(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast131(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast132(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast133(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast162(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast262(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast275(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  // Hash
  static NA_EIDPROC exp_return_type
  evalFast94(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast170(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast171(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast173(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);

  static NA_EIDPROC exp_return_type
  evalFast223(PCodeBinary*,atp_struct*,atp_struct*,ex_expr*);


  NA_EIDPROC exp_return_type
    reportOverflowError(atp_struct*,
                        PCodeBinary* pCodeOpcode,
                        PCodeBinary* pCode,
                        Long* stack);

  NA_EIDPROC exp_return_type 
    evalClauses(ex_clause *, 
		atp_struct *,
		atp_struct *,
		Lng32 datalen,
		ULng32 *rowLen,
		short *lastFldIndex,
		char * fetchedDataPtr);
  
  NA_EIDPROC virtual char *findVTblPtr(short classID);

  NA_EIDPROC virtual Long pack(void *);
  NA_EIDPROC virtual Lng32 unpack(void *, void * reallocator);
  NA_EIDPROC void packStuff(void *);

  NA_EIDPROC void makeLeanCopyAll(ComSpace * space);
  NA_EIDPROC void makeLeanCopy(ex_expr_lean * other, ComSpace * space);

  NA_EIDPROC inline Long packExpr(void * space) {
    return ex_expr::pack(space);
  }

  NA_EIDPROC inline Lng32 unpackExpr(void * base, void * reallocator) {
    return ex_expr::unpack(base, reallocator);
  }

  // called to initialized clauses before evaluating the expr.
  // Currently only called to initialize LOB related expr clauses.
  Lng32 initExpr();

  // Accessors and Methods for PCode
  //
  NA_EIDPROC PCodeBinary *getPCodeBinary() 
  { 
    return (pCode_.getPointer() ? pCode_.getPointer()->getPCodeBinary() : NULL);
  };

  NA_EIDPROC void setPCodeLean(PCodeBinary *pCode)
  {
    pCodeLean_ = pCode;
  }

  NA_EIDPROC PCode *getPCodeObject()  
  { 
    return pCodeObject_; 
  }
  
  NA_EIDPROC PCodeSegment * getPCodeSegment()
  { return pCode_.getPointer(); }

  NA_EIDPROC Int32 getPCodeSize()
  {
    return pCode_.getPointer()->getPCodeSegmentSize();
  }

  NA_EIDPROC virtual void setPCodeBinary(PCodeBinary *pCode)  
                                    { pCode_.getPointer()->setPCodeBinary(pCode); }
  NA_EIDPROC void setPCodeObject(PCode *pCodeObject)
                                    {pCodeObject_ = pCodeObject; }
  NA_EIDPROC Int16 getPCodeMode()
                                    { return pCodeMode_ ; }
  NA_EIDPROC void setPCodeMode(Int16 mode)
                                    { pCodeMode_ = mode; }
  NA_EIDPROC UInt32 getPCodeOptFlags()
                                    { return pCodeOptFlags_; }
  NA_EIDPROC void setPCodeOptFlags(UInt32 flags)
                                    { pCodeOptFlags_ = flags; }
  NA_EIDPROC UInt32 getPCodeMaxOptBrCnt()
                                    { return pCodeMaxOptBrCnt_; }
  NA_EIDPROC UInt32 getPCodeMaxOptInCnt()
                                    { return pCodeMaxOptInCnt_; }
  NA_EIDPROC void setPCodeMaxOptBrCnt(UInt32 cnt)
                                    { pCodeMaxOptBrCnt_ = cnt; }
  NA_EIDPROC void setPCodeMaxOptInCnt(UInt32 cnt)
                                    { pCodeMaxOptInCnt_ = cnt; }
  NA_EIDPROC void setEvalPtr( NABoolean isSamePrimary );

  NA_EIDPROC void setEvalPtr(evalPtrType e) { evalPtr_ = e; }

  NA_EIDPROC void setNExDbgInfo(NExDbgInfo * NExDbgPtr)
                            { NExDbgInfo_ =  NExDbgPtr; }

  NA_EIDPROC NExDbgInfo * getNExDbgInfoPtr() { return NExDbgInfo_ ; }

  NA_EIDPROC Long getEvalPtrOffset() { return evalPtrOff_ ; }

  NA_EIDPROC exp_return_type pCodeGenerate(ComSpace *space, CollHeap *heap,
					   UInt32 flags);
  NA_EIDPROC void pCodePrint();

  // Takes pointer out of PCodeBinary sequences
  // NA_EIDPROC Long getPCodeBinaryAsPtr(PCodeBinary *pcode, Int32 idx)
  //   {
  //     return *(Long*)&pcode[idx];
  //   }

  // Adds pointer to PCodeBinary sequences and returns # of PCodeBinary occupied
  NA_EIDPROC Int32 setPtrAsPCodeBinary(PCodeBinary *pcode, Int32 idx, Long ptr)
    {
      *(Long*)&pcode[idx] = ptr;
      return (sizeof(ptr)/sizeof(PCodeBinary));
    }


  // Accessors/Mutators and Methods for persisent expression storage.
  //
  NA_EIDPROC inline char * getPersistentArea() 
  { 
    return persistentArea_;
  }

  NA_EIDPROC char * getMyPersistentArea() 
  { 
    return persistentArea_;
  }

  NA_EIDPROC char *getPersistentData(Int32 offset =0);
  NA_EIDPROC char* &persistentArea() { return persistentArea_.pointer(); }
  NA_EIDPROC char* &persistentInitializationArea() 
  { return persistentInitializationArea_.pointer(); }
#ifdef NA_64BIT
  // dg64 - Match ref type
  NA_EIDPROC Int32 &persistentLength() { return persistentLength_; }
#else
  NA_EIDPROC Lng32 &persistentLength() { return persistentLength_; }
#endif
  NA_EIDPROC void initializePersistentData() { 
    str_cpy_all(persistentArea_, 
		persistentInitializationArea_,
		persistentLength_);
  }

  NA_EIDPROC void addPCodeMode(Int16 mode)
                                    { pCodeMode_ |= mode; }

  NA_EIDPROC NABoolean pCodeSpecialFields() {
    return (pCodeMode_ & PCODE_SPECIAL_FIELDS) != 0;
  };

  NA_EIDPROC void setFixupConstsAndTemps(short v) {
    if (v != 0) flags_ |= FIXUP_CONSTS_AND_TEMPS;
    else flags_ &= ~FIXUP_CONSTS_AND_TEMPS;
  };
  NA_EIDPROC NABoolean getFixupConstsAndTemps() {
    return ((flags_ & FIXUP_CONSTS_AND_TEMPS) != 0);
  };

  NA_EIDPROC NABoolean generateNoPCode() {
    return ((flags_ & GENERATE_NO_PCODE) != 0);
  }

  NA_EIDPROC void setGeneratePcode(short v) {
    if (v != 0) flags_ |= GENERATE_NO_PCODE;
    else flags_ &= ~GENERATE_NO_PCODE;
  }

  NA_EIDPROC NABoolean getPCodeGenCompile() {
    return ((flags_ & PCODE_GEN_COMPILE) != 0);
  }

  NA_EIDPROC void setPCodeGenCompile (short v) {
    if (v != 0) 
      flags_ |= PCODE_GEN_COMPILE;
    else flags_ &= ~PCODE_GEN_COMPILE;
  }

  NA_EIDPROC void setPCodeNative(NABoolean v) {
    if (v) flags_ |= PCODE_EVAL_NATIVE;
    else flags_ &= ~PCODE_EVAL_NATIVE;
  };

  NA_EIDPROC NABoolean getPCodeNative() {
    return ((flags_ & PCODE_EVAL_NATIVE) != 0);
  };

  NABoolean getNEInShowplan() {
    return ((flags_ & PCODE_NE_IN_SHOWPLAN) != 0);
  };

  void setNEInShowplan(NABoolean v) {
   ( v ? flags_ |= PCODE_NE_IN_SHOWPLAN :
         flags_ &= ~PCODE_NE_IN_SHOWPLAN );
  };

  NA_EIDPROC void setPCodeMoveFastpath(NABoolean v) {
    if (v) flags_ |= PCODE_MOVE_FASTPATH;
    else flags_ &= ~PCODE_MOVE_FASTPATH;
  };
  NA_EIDPROC NABoolean getPCodeMoveFastpath() {
    return ((flags_ & PCODE_MOVE_FASTPATH) != 0);
  };

  NA_EIDPROC void setForInsertUpdate(NABoolean value)
  {
    if (value)
      flags_ |= FOR_INSERT_UPDATE;
    else
      flags_ &= ~FOR_INSERT_UPDATE;
  };
  NA_EIDPROC NABoolean forInsertUpdate() {
    return ((flags_ & FOR_INSERT_UPDATE) != 0);
  };

  NA_EIDPROC void setPCodeEvalAligned(NABoolean value)
  {
    if (value)
      flags_ |= PCODE_EVAL_ALIGNED;
    else
      flags_ &= ~PCODE_EVAL_ALIGNED;
  };
  NA_EIDPROC NABoolean usePCodeEvalAligned() {
    return ((flags_ & PCODE_EVAL_ALIGNED) != 0);
  };

  NA_EIDPROC void setHandleIndirectVC(NABoolean value) {
    if (value) 
      flags_ |= HANDLE_INDIRECT_VC;
    else 
      flags_ &= ~HANDLE_INDIRECT_VC;
  };

  NA_EIDPROC NABoolean handleIndirectVC() {
    return ((flags_ & HANDLE_INDIRECT_VC) != 0);
  };

  static NABoolean forShowplan(UInt32 f) {return ((f & FOR_SHOWPLAN) != 0);}
  static void setForShowplan(UInt32 &f, NABoolean v)
  { ( v ? f |= FOR_SHOWPLAN : f &= ~FOR_SHOWPLAN );}

  static NABoolean downrevCompile(UInt32 f) {return ((f & DOWNREV_COMPILE) != 0);}
  static NABoolean downrevCompileR2FCS(UInt32 f) {return ((f & DOWNREV_COMPILE) != 0);}
  static void setDownrevCompile(UInt32 &f, NABoolean v)
  { ( v ? f |= DOWNREV_COMPILE : f &= ~DOWNREV_COMPILE );}

  static void setDownrevCompileR2FCS(UInt32 &f, NABoolean v)
  { ( v ? f |= DOWNREV_COMPILE : f &= ~DOWNREV_COMPILE );}

  static NABoolean downrevCompileRR(UInt32 f) {return ((f & DOWNREV_COMPILE_RR) != 0);}
  static void setDownrevCompileRR(UInt32 &f, NABoolean v)
  { ( v ? f |= DOWNREV_COMPILE_RR : f &= ~DOWNREV_COMPILE_RR );}
  static NABoolean notValidateFloat64(UInt32 f)
  { return ((f & NOT_VALIDATE_FLOAT64) != 0); }
  static void setNotValidateFloat64(UInt32 &f, NABoolean v)
  { ( v ? f |= NOT_VALIDATE_FLOAT64 : f &= ~NOT_VALIDATE_FLOAT64 );}

  NA_EIDPROC
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

    // expressions compatible with pre-R2.1(052005) and post R1.8
    // release(R2 FCS) need to be generated
    DOWNREV_COMPILE = 0x0002,

    // expressions compatible with R2.1(roadrunner) need to be generated
    DOWNREV_COMPILE_RR = 0x0004,

    // tell clause pcodegenerate() to generate PCode instruction
    // to validate value range for float64 to float64 assigments
    NOT_VALIDATE_FLOAT64 = 0x0008
  };

  exp_eye_catcher         eyeCatcher_;                           // 00-03

  // internal flags
  UInt32                  flags_;                                // 04-07

  // heap will be set by the executor?
  CollHeap               *heap_;
#ifndef NA_64BIT
  char                    fillerHeap_[4];
#endif                                                           // 08-15 

  union                                                          // 16-19
  {
    PCodeBinary * pCodeLean_;
    PCode * pCodeObject_;
  };

#ifndef NA_64BIT
  char                    fillerPCodeObject_[4];                 // 20-23
#endif                                                           

  PCodeSegmentPtr         pCode_;                                // 24-31

  ExClausePtr             clauses_;                              // 32-39

  // For error injection testing.
  const ex_tcb           *myTcb_;
#ifndef NA_64BIT
  char                    fillerMyTcb_[4];
#endif                                                           // 40-47 

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

#ifndef NA_64BIT
  char                    fillerEvalPtr_[4];
#endif                                                           // 116-119 

  UInt32                  pCodeMaxOptBrCnt_;                     // 120-123
  UInt32                  pCodeMaxOptInCnt_;                     // 124-127

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char                   fillers_[8];                           // 128-135

  NA_EIDPROC void fixupNextClause();

};


//////////////////////////////////////////////////
// class AggrExpr
//////////////////////////////////////////////////
#pragma nowarn(1319)  // warning elimination 
class SQLEXP_LIB_FUNC  AggrExpr : public ex_expr {

  enum Flags {
    ONE_ROW_AGGR = 0x0001
  };

  ExExprPtr initExpr_;          // 00-07
  ExExprPtr perrecExpr_;        // 08-15
  ExExprPtr finalNullExpr_;     // 16-23
  ExExprPtr finalExpr_;         // 24-31
  UInt32    flags_;             // 32-35

  // ---------------------------------------------------------------------
  // Fillers for potential future extensions without changing class size.
  // When a new member is added, size of this filler should be reduced so
  // that the size of the object remains the same (and is modulo 8).
  // ---------------------------------------------------------------------
  char                   fillers_[12];  // 36-47

public:
NA_EIDPROC
  AggrExpr() : ex_expr(ex_expr::exp_AGGR), flags_(0) {}

NA_EIDPROC
  AggrExpr( ex_expr * init_expr, 
	    ex_expr * perrec_expr, 
	    ex_expr * final_expr, 
	    ex_expr * final_null_expr
	  )
    : ex_expr(ex_expr::exp_AGGR), 
      initExpr_(init_expr),
      perrecExpr_(perrec_expr), 
      finalExpr_(final_expr),
      finalNullExpr_(final_null_expr),
      flags_(0)
  {}

NA_EIDPROC ~AggrExpr() {}

NA_EIDPROC
  exp_return_type initializeAggr(atp_struct *);
NA_EIDPROC
  exp_return_type finalizeAggr(atp_struct *);
NA_EIDPROC
  exp_return_type finalizeNullAggr(atp_struct *);
NA_EIDPROC
  Long pack(void *);
NA_EIDPROC
  Lng32 unpack(void *, void * reallocator);
NA_EIDPROC
  virtual exp_return_type fixup(Lng32, unsigned short, const ex_tcb * tcb,
				ComSpace * space = 0,
				CollHeap * exHeap = 0,
				NABoolean computeSpaceOnly = FALSE,
				ex_globals * glob = FALSE);
NA_EIDPROC
  virtual void displayContents(ComSpace * space, short mode, const char * displayStr,
                               ULng32 flag);


  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  NA_EIDPROC virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  NA_EIDPROC virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_expr::populateImageVersionIDArray();
  }

  NA_EIDPROC virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

  NA_EIDPROC void       setOneRowAggr()  { flags_ |= ONE_ROW_AGGR; }
  NA_EIDPROC NABoolean  isOneRowAggr()
                 { return( (flags_ & ONE_ROW_AGGR) != 0); }
};
#pragma warn(1319)  // warning elimination 

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
    NO_DATETIME_VALIDATION = 0x0008

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

  SQLEXP_LIB_FUNC InputOutputExpr() 
    : ex_expr(ex_expr::exp_INPUT_OUTPUT),
      flags_(0), isCall_(0),
      numEntries_(-1)
  {}

  SQLEXP_LIB_FUNC InputOutputExpr(Generator * generator);
  NA_EIDPROC
  SQLEXP_LIB_FUNC ~InputOutputExpr() {}

  ex_expr::exp_return_type describeInput(void*, void*, UInt32);
  ex_expr::exp_return_type describeOutput(void*, UInt32);

  ex_expr::exp_return_type addDescInfoIntoStaticDesc(Descriptor* desc,
						     NABoolean isInput);


  Lng32 getMaxParamIdx();

  SQLEXP_LIB_FUNC ex_expr::exp_return_type inputSingleRowValue
  (atp_struct*,
   void*,
   char*,
   CollHeap *exHeap,
   UInt32 flags);
  
  SQLEXP_LIB_FUNC ex_expr::exp_return_type inputRowwiseRowsetValues
  (atp_struct*,
   void*,
   void*,
   NABoolean tolerateNonFatalError,  
   CollHeap *exHeap,
   UInt32 flags);

  SQLEXP_LIB_FUNC ex_expr::exp_return_type inputValues
  (atp_struct *atp,
   void * inputDesc_,
   NABoolean tolerateNonFatalError,
   CollHeap * heap,
   UInt32 flags);

  SQLEXP_LIB_FUNC ex_expr::exp_return_type outputValues(atp_struct*,
							void*, 
                                                        CollHeap *exHeap,
							UInt32 flags);

  SQLEXP_LIB_FUNC  Lng32 getCompiledOutputRowsetSize(atp_struct *);

  SQLEXP_LIB_FUNC virtual Long pack(void *);

  SQLEXP_LIB_FUNC  Lng32 getNumEntries(){
    return numEntries_;
  };
  
  SQLEXP_LIB_FUNC  void setNumEntries(Lng32 entries){
    numEntries_ = entries;
  };

  SQLEXP_LIB_FUNC NABoolean noEval() 
  { return (flags_ & NO_EVAL) != 0; };

  SQLEXP_LIB_FUNC void setNoEval(NABoolean v)      
  { (v ? flags_ |= NO_EVAL : flags_ &= ~NO_EVAL); }

  SQLEXP_LIB_FUNC NABoolean skipTypeCheck() 
  { return (flags_ & SKIP_TYPE_CHECK) != 0; };

  SQLEXP_LIB_FUNC void setSkipTypeCheck(NABoolean v)      
  { (v ? flags_ |= SKIP_TYPE_CHECK : flags_ &= ~SKIP_TYPE_CHECK); }

  SQLEXP_LIB_FUNC NABoolean restrictedSkipTypeCheck() 
  { return (flags_ & RESTRICTED_SKIP_TYPE_CHECK) != 0; };

  SQLEXP_LIB_FUNC void setRestrictedSkipTypeCheck(NABoolean v)      
  { (v ? flags_ |= RESTRICTED_SKIP_TYPE_CHECK : flags_ &= ~RESTRICTED_SKIP_TYPE_CHECK); }

  SQLEXP_LIB_FUNC NABoolean isDBTR(UInt32 flags) 
  { return (flags & IS_DBTR_) != 0; };

  SQLEXP_LIB_FUNC void setIsDBTR(UInt32 &flags, NABoolean v)      
  { (v ? flags |= IS_DBTR_ : flags &= ~IS_DBTR_); }

  SQLEXP_LIB_FUNC NABoolean isODBC(UInt32 flags) 
  { return (flags & IS_ODBC_) != 0; };

  SQLEXP_LIB_FUNC void setIsODBC(UInt32 &flags, NABoolean v)      
  { (v ? flags |= IS_ODBC_ : flags &= ~IS_ODBC_); }

  SQLEXP_LIB_FUNC NABoolean isRWRS(UInt32 flags) 
  { return (flags & IS_RWRS_) != 0; };

  SQLEXP_LIB_FUNC void setIsRWRS(UInt32 &flags, NABoolean v)      
  { (v ? flags |= IS_RWRS_ : flags &= ~IS_RWRS_); }

  SQLEXP_LIB_FUNC NABoolean isInternalFormatIO(UInt32 flags) 
  { return (flags & INTERNAL_FORMAT_IO_) != 0; };

  SQLEXP_LIB_FUNC void setInternalFormatIO(UInt32 &flags, NABoolean v)
  { (v ? flags |= INTERNAL_FORMAT_IO_ : flags &= ~INTERNAL_FORMAT_IO_); }

  SQLEXP_LIB_FUNC NABoolean noDatetimeValidation()
    { return ((flags_ & NO_DATETIME_VALIDATION) != 0); };

  SQLEXP_LIB_FUNC void setNoDatetimeValidation(NABoolean v)
    { (v) ? flags_ |= NO_DATETIME_VALIDATION : flags_ &= ~NO_DATETIME_VALIDATION; }

  // ---------------------------------------------------------------------
  // Redefinition of methods inherited from NAVersionedObject.
  // ---------------------------------------------------------------------
  SQLEXP_LIB_FUNC virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  SQLEXP_LIB_FUNC virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ex_expr::populateImageVersionIDArray();
  }

  SQLEXP_LIB_FUNC virtual short getClassSize() { return (short)sizeof(*this); }

  SQLEXP_LIB_FUNC void setupBulkMoveInfo(void * desc_, CollHeap * heap,
					 NABoolean isInputDesc,
					 UInt32 flags);
  SQLEXP_LIB_FUNC ex_expr::exp_return_type doBulkMove(atp_struct * atp, 
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
  static SQLEXP_LIB_FUNC NABoolean areCompatible(short source, short target)
  {
    if ((((source >= REC_MIN_NUMERIC) && (source <= REC_MAX_NUMERIC)) &&
	 ((target >= REC_MIN_NUMERIC) && (target <= REC_MAX_NUMERIC))) ||
	(((source >= REC_MIN_CHARACTER) && (source <= REC_MAX_CHARACTER)) &&
	 ((target >= REC_MIN_CHARACTER) && (target <= REC_MAX_CHARACTER))) ||
	((source == REC_DATETIME) && (target == REC_DATETIME)) ||
	(((source >= REC_MIN_INTERVAL) && (source <= REC_MAX_INTERVAL)) &&
	 ((target >= REC_MIN_INTERVAL) && (target <= REC_MAX_INTERVAL))) ||
	((source == REC_BLOB) && (target == REC_BLOB)) ||
	((source == REC_CLOB) && (target == REC_CLOB)))
      return TRUE;
    else
      return FALSE;
  }
	
  static SQLEXP_LIB_FUNC NABoolean implicitConversionSupported(short source, short target)
  {
    if ((((source >= REC_MIN_CHARACTER) && (source <= REC_MAX_CHARACTER)) &&
	 ((target == REC_DATETIME) || ((target >= REC_MIN_BINARY) && (target <= REC_MAX_BINARY)))) ||
	(((target >= REC_MIN_CHARACTER) && (target <= REC_MAX_CHARACTER)) &&
	 ((source == REC_DATETIME) || ((source >= REC_MIN_BINARY) && (source <= REC_MAX_BINARY)))))
      return TRUE;
    else
      return FALSE;
  }

  SQLEXP_LIB_FUNC NABoolean isExactNumeric(short type)
  {
    if ((type >= REC_MIN_BINARY) && (type <= REC_MAX_BINARY) ||
	(type >= REC_MIN_DECIMAL) && (type <= REC_MAX_DECIMAL))
      return TRUE;
    else
      return FALSE;
  }
  
  // ---------------------------------------------------------------------

};


class SQLEXP_LIB_FUNC  ex_expr_lean : public ex_expr
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
  NA_EIDPROC virtual exp_return_type fixup(Lng32, unsigned short,
                                           const ex_tcb * tcb,
					   ComSpace * space = 0,
					   CollHeap * exHeap = 0,
					   NABoolean computeSpaceOnly = FALSE);

  NA_EIDPROC virtual Long pack(void *);
  NA_EIDPROC virtual Lng32 unpack(void *, void * reallocator);

  void convAddrToOffsetInPCode(void * space);
  void convOffsetToAddrInPCode(void* base);

  NA_EIDPROC virtual void displayContents(ComSpace * space, short mode,
					  const char * displayStr,
					  ULng32 flag = 0x00000006);

  //  NA_EIDPROC Int32 *getMyPCode() 
  //  { 
  //    return &pCodeLean_[1];
  //  }

  NA_EIDPROC Int32 getPCodeSize()
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


#pragma warning ( default : 4251 )
 
///////////////////////////////////////////////////
// class Target
///////////////////////////////////////////////////
template <class t> class Target {
public:
	t operator=(t source)
	{
		t *targetBuffer;
#if defined( NA_SHADOWCALLS )
		targetBuffer = (t *)SqlCliSp_GetBufPtr((char *)this, false);
#else
                targetBuffer = (t *)this;
#endif
		*targetBuffer = source;
#if defined( NA_SHADOWCALLS )
		SqlCliSp_SetWriteLength((char *)this, sizeof(source));
#endif
		return source;
	}
};


#endif

