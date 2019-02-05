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
#ifndef EXP_LOB_EXPR_H
#define EXP_LOB_EXPR_H


/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ExpLOB.h
 * Description:  
 *               
 *               
 * Created:      11/30/2012
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "exp_clause.h"
#include "ExpLOBenums.h"

class ContextCli;
class ExLobGlobals;


class ExLobInMemoryDescChunksEntry;


/////////////////////////////////////////
// Class ExpLOBoper                    //
/////////////////////////////////////////
class ExpLOBoper : public ex_clause {

public:
  // Construction
  //
  ExpLOBoper();
  ExpLOBoper(OperatorTypeEnum oper_type,
	     short num_operands,
	     Attributes ** attr,
	     Space * space);

  virtual ex_expr::exp_return_type pCodeGenerate(Space *space, UInt32 f);

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

  // LOB hdfs name format for permanent lobs.
  // Length:  30 bytes
  // 
  //     LOBP_<objectUid>_<LOBnum>
  //          <---20----> <--4--->
  static char * ExpGetLOBname(Int64 uid, Lng32 lobNum, 
			      char * outBuf, Lng32 outBufLen);

  static char * ExpGetLOBDescName(Lng32 schNameLen, char * schName, 
				  Int64 uid, Lng32 lobNum, 
				  char * outBuf, Lng32 outBufLen);

  static char * ExpGetLOBDescHandleObjNamePrefix(Int64 uid, 
                                                             char * outBuf, Lng32 outBufLen);

  static char * ExpGetLOBDescHandleName(Lng32 schNameLen, char * schName, 
					Int64 uid, Lng32 lobNum, 
					char * outBuf, Lng32 outBufLen);
  
  static char * ExpGetLOBDescChunksName(Lng32 schNameLen, char * schName, 
					Int64 uid, Lng32 lobNum, 
					char * outBuf, Lng32 outBufLen);


  static Lng32 ExpGetLOBnumFromDescName(char * descName, Lng32 descNameLen);
  
  static char * ExpGetLOBMDName(Lng32 schNameLen, char * schName,
				Int64 uid,  
				char * outBuf, Lng32 outBufLen);
  static void calculateNewOffsets(ExLobInMemoryDescChunksEntry *dcArray, Lng32 numEntries);
  static Lng32 compactLobDataFile(ExLobGlobals *lobGlob, ExLobInMemoryDescChunksEntry *dcArray, Int32 numEntries, char *tgtLobName, Int64 lobMaxChunkSize, NAHeap *lobHeap, ContextCli *currContext,char *hdfsServer, Int32 hdfsPort,char *lobLocation);
  static Int32 restoreLobDataFile(ExLobGlobals *lobGlob, char *lobName, NAHeap *lobHeap, ContextCli *currContext,char *hdfsServer, Int32 hdfsPort,char *lobLocation );
  static Int32 purgeBackupLobDataFile(ExLobGlobals *lobGlob,char *lobName, NAHeap *lobHeap, ContextCli *currContext, char *hdfsServer, Int32 hdfsPort, char *lobLocation);

  static Lng32 createLOB(ExLobGlobals * lobGlob, ContextCli *currContext,
			 char * lobLoc, Int32 hdfsPort, char *hdfsServer,
			 Int64 uid, Lng32 lobNum, Int64 lobMAxSize);

  static Lng32 dropLOB(ExLobGlobals * lobGlob, ContextCli *currContext, 
		       char * lobLoc,Int32 hdfsPort, char *hdfsServer,
		       Int64 uid, Lng32 lobNum);

  static Lng32 purgedataLOB(ExLobGlobals * lobGlob, 
			    char * lobLob,
			    Int64 uid, Lng32 lobNum);

  static Lng32 initLOBglobal(ExLobGlobals *& lobGlob, NAHeap *heap, ContextCli *currContext,char *server, Int32 port, NABoolean isHiveRead = FALSE);
  static ExLobGlobals *initLOBglobal(NAHeap *parentHeap, ContextCli *currContext, NABoolean useLibHdfs, NABoolean isHiveRead = FALSE);
  static void deleteLOBglobal(ExLobGlobals *lobGlob, NAHeap *parentHeap);
  static void genLobLockId(Int64 objUid,Int32 lobNum, char *llid);

  // Extracts values from the LOB handle stored at ptr
  static Lng32 extractFromLOBhandle(Int16 *flags,
				    Lng32 *lobType,
				    Lng32 *lobNum,
				    Int64 *uid, 
				    Int64 *descSyskey, 
				    Int64 *descPartnKey,
				    short *schNameLen,
				    char * schName,
				    char * ptrToLobHandle,
				    Lng32 handleLen = 0);

  // Generates LOB handle that is stored in the SQL row.
  // LOB handle max len:  512 bytes
  // <flags><LOBType><LOBnum><objectUid><LOBlen><descKey><descTS><chunkNum><schNameLen><schName>
  // <--4--><--4----><--4---><----8----><---8--><---8---><--8---><----2---><---2------><--vc--->
  static void genLOBhandle(Int64 uid, 
			   Lng32 lobNum,
			   Int32 lobType,
			   Int64 descKey, 
			   Int64 descTS,
			   Lng32 flags,
			   short schNameLen,
			   char  * schName,
			   Lng32 &handleLen,
			   char * ptr);

  static void updLOBhandle(Int64 descSyskey, 
			   Lng32 flags,                       
			   char * ptr);

  static Lng32 genLOBhandleFromHandleString(char * lobHandleString,
					    Lng32 lobHandleStringLen,
					    char * lobHandle,
					    Lng32 &lobHandleLen);
  
  short &lobNum() {return lobNum_; }

  LobsStorage lobStorageType() { return (LobsStorage)lobStorageType_; }
  void setLobStorageType(LobsStorage v) { lobStorageType_ = (short)v; };

  char * lobStorageLocation() { return lobStorageLocation_; }
  void setLobStorageLocation(char * loc) { strcpy(lobStorageLocation_, loc); }

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  void setDescSchNameLen(short v) { descSchNameLen_ = v; }
  
  virtual Lng32 initClause();
  void setLobMaxSize(Int64 maxsize) { lobMaxSize_ = maxsize;}
  Int64 getLobMaxSize() { return lobMaxSize_;}
  void setLobSize(Int64 lobsize) { lobSize_ = lobsize;}
  Int64 getLobSize() { return lobSize_;}
  void setLobMaxChunkMemSize(Int64 maxsize) { lobMaxChunkMemSize_ = maxsize;}
  Int64 getLobMaxChunkMemSize() { return lobMaxChunkMemSize_;}
  void setLobGCLimit(Int64 gclimit) { lobGCLimit_ = gclimit;}
  Int64 getLobGCLimit() { return lobGCLimit_;}
  void setLobHdfsServer(char *hdfsServer)
  {strcpy(lobHdfsServer_,hdfsServer);}
  void setLobHdfsPort(Int32 hdfsPort)
  {lobHdfsPort_ = hdfsPort;}
 protected:
  typedef enum
  {
    DO_NOTHING_,
    CHECK_STATUS_,
    START_LOB_OPER_
  } LobOperStatus;

  static Lng32 extractFromLOBstring(Int64 &uid, 
				    Int32 &lobNum,
				    Int64 &descPartnKey,
				    Int64 &descSyskey,
				    Int16 &flags,
				    Int32 &lobType,
				    short &schNameLen,
				    char * schName,
				    char * handle,
				    Int32 handleLen);
  
  void createLOBhandleString(Int16 flags,
			     Int32 lobType,
			     Int64 uid, 
			     Lng32 lobNum,
			     Int64 descKey, 
			     Int64 descTS,
			     Int16 schNameLen,
			     char * schName,
			     char * ptrToLobHandle);

  struct LOBHandle
  {
    
    Int32 flags_;
    Int32  lobType_;
    Lng32 lobNum_;
    Int64 objUID_;
    Int64 descSyskey_;
    Int64 descPartnkey_;
    short schNameLen_;
    char  schName_;
   
  };

  Lng32 checkLobOperStatus();
  
protected:
  char * descSchName() { return descSchName_; }

  char * getLobHdfsServer() { return (strlen(lobHdfsServer_) == 0 ? NULL : lobHdfsServer_); }
  Lng32 getLobHdfsPort() { return lobHdfsPort_; }
 

  short flags_;      // 00-02

  short lobNum_;
  
  short lobStorageType_;

  short lobHandleLenSaved_;

  // identifier returned by ExLobsOper during a nowaited operation. 
  // Used to check status of the request.
  Int64 requestTag_;

  char lobHandleSaved_[LOB_HANDLE_LEN];

  char outLobHandle_[LOB_HANDLE_LEN];
  Int32 outHandleLen_;

  char blackBox_[MAX_BLACK_BOX_LEN];
  Int64 blackBoxLen_;

  char lobStorageLocation_[MAX_LOB_FILE_NAME_LEN];

  char lobHdfsServer_[256];
  Lng32 lobHdfsPort_;

  short descSchNameLen_;
  char  descSchName_[ComAnsiNamePart::MAX_IDENTIFIER_EXT_LEN+1];
  Int64 lobSize_;
  Int64 lobMaxSize_;
  Int64 lobMaxChunkMemSize_;
  Int64 lobGCLimit_;
  //  NABasicPtr lobStorageLocation_;
}
;

class ExpLOBiud : public ExpLOBoper {
 public:
  ExpLOBiud(OperatorTypeEnum oper_type,
	    Lng32 numAttrs,
	    Attributes ** attr, 
	    Int64 objectUID,
	    short descSchNameLen,
	    char * descSchName,
	    Space * space);
  ExpLOBiud();

  
  ex_expr::exp_return_type insertDesc(char *op_data[],
                                       char *lobData,
                                       Int64 lobLen,
                                       Int64 lobHdfsDataOffset,
                                       char *&result,
                                       CollHeap*h,
                                       ComDiagsArea** diagsArea);

  ex_expr::exp_return_type insertData(Lng32 handleLen,
                                                char *&handle,
                                                char *lobData,
                                                Int64 lobLen,
                                                Int64 &hdfsDataOffset,
                                                CollHeap*h,
                                                ComDiagsArea** diagsArea);

  NABoolean isAppend()
  {
    return ((liudFlags_ & IS_APPEND) != 0);
  };

  inline void setIsAppend(NABoolean v)
  {
    (v) ? liudFlags_ |= IS_APPEND: liudFlags_ &= ~IS_APPEND;
  };

  NABoolean fromString()
  {
    return ((liudFlags_ & FROM_STRING) != 0);
  };

  inline void setFromString(NABoolean v)
  {
    (v) ? liudFlags_ |= FROM_STRING: liudFlags_ &= ~FROM_STRING;
  };
  NABoolean fromBuffer()
  {
    return ((liudFlags_ & FROM_BUFFER) != 0);
  };
  inline void setFromBuffer(NABoolean v)
  {
    (v) ? liudFlags_ |= FROM_BUFFER: liudFlags_ &= ~FROM_BUFFER;
  };
  inline void setFromEmpty(NABoolean v)
  {
    (v) ? liudFlags_ |= FROM_EMPTY: liudFlags_ &= ~FROM_EMPTY;
  };
  NABoolean fromEmpty()
  {
    return ((liudFlags_ & FROM_EMPTY) != 0);
  };
 
  NABoolean fromFile()
  {
    return ((liudFlags_ & FROM_FILE) != 0);
  };

  inline void setFromFile(NABoolean v)
  {
    (v) ? liudFlags_ |= FROM_FILE: liudFlags_ &= ~FROM_FILE;
  };

  NABoolean fromLoad()
  {
    return ((liudFlags_ & FROM_LOAD) != 0);
  };

  inline void setFromLoad(NABoolean v)
  {
    (v) ? liudFlags_ |= FROM_LOAD: liudFlags_ &= ~FROM_LOAD;
  };

  NABoolean fromLob()
  {
    return ((liudFlags_ & FROM_LOB) != 0);
  };

  inline void setFromLob(NABoolean v)
  {
    (v) ? liudFlags_ |= FROM_LOB: liudFlags_ &= ~FROM_LOB;
  };
  NABoolean fromLobExternal()
  {
    return ((liudFlags_ & FROM_LOB_EXTERNAL) != 0);
  };

  inline void setFromLobExternal(NABoolean v)
  {
    (v) ? liudFlags_ |= FROM_LOB_EXTERNAL: liudFlags_ &= ~FROM_LOB_EXTERNAL;
  };

  NABoolean fromExternal()
  {
    return ((liudFlags_ & FROM_EXTERNAL) != 0);
  };

  inline void setFromExternal(NABoolean v)
  {
    (v) ? liudFlags_ |= FROM_EXTERNAL: liudFlags_ &= ~FROM_EXTERNAL;
  };
 
 NABoolean lobLocking()
  {
    return ((liudFlags_ & LOB_LOCKING) != 0);
  };

  inline void setLobLocking(NABoolean v)
  {
    (v) ? liudFlags_ |= LOB_LOCKING: liudFlags_ &= ~LOB_LOCKING;
  };

 protected:
  Int64 objectUID_;

  enum
  {
    IS_APPEND          = 0x0001,
    FROM_STRING        = 0x0002,
    FROM_FILE          = 0x0004,
    FROM_LOAD          = 0x0008,
    FROM_LOB           = 0x0010,
    FROM_EXTERNAL      = 0x0020,
    FROM_BUFFER        = 0x0040,
    FROM_EMPTY         = 0x0080,
    FROM_LOB_EXTERNAL  = 0x0100,
    LOB_LOCKING        = 0x0200
  };

  Lng32 liudFlags_;
  char filler1_[4];
};

class ExpLOBinsert : public ExpLOBiud {
public:
  ExpLOBinsert(OperatorTypeEnum oper_type,
	       Lng32 numAttrs,
	       Attributes ** attr, 
	       Int64 objectUID,
	       short descSchNameLen,
	       char * descSchName,
	       Space * space);
  ExpLOBinsert();


  virtual ex_expr::exp_return_type eval(char *op_data[],
					CollHeap*,
					ComDiagsArea** diagsArea = 0);
  
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
    setImageVersionID(2,getClassVersionID());
    ExpLOBoper::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------
  
 private:
 
  Lng32 liFlags_;
  char filler1_[4];
 
};

class ExpLOBdelete : public ExpLOBiud {
public:
  ExpLOBdelete(OperatorTypeEnum oper_type,
	       Attributes ** attr, 
	       Space * space);
  ExpLOBdelete();

  virtual ex_expr::exp_return_type eval(char *op_data[],
					CollHeap*,
					ComDiagsArea** diagsArea = 0);
  
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
    setImageVersionID(2,getClassVersionID());
    ExpLOBoper::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

 private:

  Lng32 ldFlags_;
  char  filler1_[4];
};

class ExpLOBupdate : public ExpLOBiud {
public:
  ExpLOBupdate(OperatorTypeEnum oper_type,
	       Lng32 numAttrs,
	       Attributes ** attr, 
	       Int64 objectUID,
	       short descSchNameLen,
	       char * descSchName,
	       Space * space);
  ExpLOBupdate();

  // Null Semantics
  //
  Int32 isNullInNullOut() const { return 0; };
  Int32 isNullRelevant() const { return 1; };

  virtual ex_expr::exp_return_type processNulls(char *op_data[], CollHeap *heap,
						ComDiagsArea **diagsArea);
  
  virtual ex_expr::exp_return_type eval(char *op_data[],
					CollHeap*,
					ComDiagsArea** diagsArea = 0);

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
    setImageVersionID(2,getClassVersionID());
    ExpLOBoper::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------


 private:
  Lng32 luFlags_;
  short nullValue_;
  char  filler1_[2];
};

class ExpLOBselect : public ExpLOBoper {
public:
  ExpLOBselect(OperatorTypeEnum oper_type,
	       Attributes ** attr, 
	       Space * space);
  ExpLOBselect();

  virtual ex_expr::exp_return_type eval(char *op_data[],
					CollHeap*,
					ComDiagsArea** diagsArea = 0);
  
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
    setImageVersionID(2,getClassVersionID());
    ExpLOBoper::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------
   NABoolean toLob()
  {
    return ((lsFlags_ & TO_LOB) != 0);
  };

  inline void setToLob(NABoolean v)
  {
    (v) ? lsFlags_ |= TO_LOB: lsFlags_ &= ~TO_LOB;
  };
  NABoolean toFile()
  {
    return ((lsFlags_ & TO_FILE) != 0);
  };

  inline void setToFile(NABoolean v)
  {
    (v) ? lsFlags_ |= TO_FILE: lsFlags_ &= ~TO_FILE;
  };
  void setTgtFile(char *tgtFile)
  {
    strcpy(tgtFile_,tgtFile);
  }
  char *getTgtFile()
  {
    return tgtFile_;
  }
  void setTgtLocation(char *tgtLoc)
  {
    strcpy(tgtLocation_,tgtLoc);
  }
  char *getTgtLocation()
  {
    return tgtLocation_;
  }
  
 private:
   enum
  {
    
    TO_LOB             = 0x0001,
    TO_FILE            = 0x0002
  };
 
 
  Lng32 lsFlags_;
  char  filler1_[4];
  char tgtLocation_[512];
  char tgtFile_[512];
};

class ExpLOBconvert : public ExpLOBoper {
public:
  ExpLOBconvert(OperatorTypeEnum oper_type,
		Attributes ** attr, 
		Space * space);
  ExpLOBconvert();

  virtual ex_expr::exp_return_type eval(char *op_data[],
					CollHeap*,
					ComDiagsArea** diagsArea = 0);
  
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
    setImageVersionID(2,getClassVersionID());
    ExpLOBoper::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

  NABoolean toString()
  {
    return ((lcFlags_ & TO_STRING) != 0);
  };

  inline void setToString(NABoolean v)
  {
    (v) ? lcFlags_ |= TO_STRING: lcFlags_ &= ~TO_STRING;
  };

  NABoolean toLob()
  {
    return ((lcFlags_ & TO_LOB) != 0);
  };

  inline void setToLob(NABoolean v)
  {
    (v) ? lcFlags_ |= TO_LOB: lcFlags_ &= ~TO_LOB;
  };
  NABoolean toFile()
  {
    return ((lcFlags_ & TO_FILE) != 0);
  };

  inline void setToFile(NABoolean v)
  {
    (v) ? lcFlags_ |= TO_FILE: lcFlags_ &= ~TO_FILE;
  };
   void setConvertSize(Int64 size)
  {
    convertSize_ = size;
  }
  Int64 getConvertSize()
  {
    return convertSize_;
  }
  void setTgtFile(char *tgtFile)
  {
    tgtFileName_ = tgtFile;
  }
  char *getTgtFile()
  {
    return tgtFileName_;
  }

private:
  enum
  { 
    TO_STRING          = 0x0001,
    TO_LOB             = 0x0002,
    TO_FILE            = 0x0004
  };
 

  Lng32 lcFlags_;
  char  filler1_[4];
  Int64 convertSize_;
  char * tgtFileName_;
};

class ExpLOBconvertHandle : public ExpLOBoper {
public:
  ExpLOBconvertHandle(OperatorTypeEnum oper_type,
		      Attributes ** attr, 
		      Space * space);
  ExpLOBconvertHandle();

  virtual ex_expr::exp_return_type eval(char *op_data[],
					CollHeap*,
					ComDiagsArea** diagsArea = 0);
  
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
    setImageVersionID(2,getClassVersionID());
    ExpLOBoper::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
  // ---------------------------------------------------------------------

  NABoolean toString()
  {
    return ((lchFlags_ & TO_STRING) != 0);
  };

  inline void setToString(NABoolean v)
  {
    (v) ? lchFlags_ |= TO_STRING: lchFlags_ &= ~TO_STRING;
  };

  NABoolean toLob()
  {
    return ((lchFlags_ & TO_LOB) != 0);
  };

  inline void setToLob(NABoolean v)
  {
    (v) ? lchFlags_ |= TO_LOB: lchFlags_ &= ~TO_LOB;
  };
 private:
  enum
  {
    TO_STRING          = 0x0001,
    TO_LOB             = 0x0002
  };


  Lng32 lchFlags_;
  char  filler1_[4];
};

	   
/////////////////////////////////////////
// Class ExpLOBfunction                //
/////////////////////////////////////////
class ExpLOBfunction : public ExpLOBoper {

public:
  // Construction
  //
  ExpLOBfunction();
  ExpLOBfunction(OperatorTypeEnum oper_type,
		 short num_operands,
		 Attributes ** attr,
		 Space * space);

 protected:

 private:
  Int64 funcFlags_;
}
;

/////////////////////////////////////////
// Class ExpLOBfuncSubstring                
/////////////////////////////////////////
class ExpLOBfuncSubstring : public ExpLOBfunction {

public:
  // Construction
  //
  ExpLOBfuncSubstring();
  ExpLOBfuncSubstring(OperatorTypeEnum oper_type,
		      short num_operands,
		      Attributes ** attr,
		      Space * space);

  // Display
  //
  virtual void displayContents(Space * space, const char * displayStr, 
			       Int32 clauseNum, char * constsArea);

  virtual ex_expr::exp_return_type eval(char *op_data[], CollHeap*, 
					ComDiagsArea** = 0); 
  
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
    ExpLOBoper::populateImageVersionIDArray();
  }

  virtual short getClassSize() { return (short)sizeof(*this); }
 
 private:
}
;

#endif
