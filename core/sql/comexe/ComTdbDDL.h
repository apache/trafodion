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
* File:         ComTdbDDL.h
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef COMTDBDDL_H
#define COMTDBDDL_H

#include "ComTdb.h"

////////////////////////////////////////////////////////////////////
// class ComTdbGenericUtil
////////////////////////////////////////////////////////////////////
class ComTdbGenericUtil : public ComTdb
{
  friend class ExDDLTcb;
  friend class ExDDLPrivateState;

public:
  ComTdbGenericUtil()
  : ComTdb(ComTdb::ex_DDL, eye_DDL)
  {};
  
  ComTdbGenericUtil(char * query,
		    ULng32 querylen,
		    Int16 querycharset,
		    char * objectName,
		    ULng32 objectNameLen,
		    ex_expr * input_expr,
		    ULng32 input_rowlen,
		    ex_expr * output_expr,
		    ULng32 output_rowlen,
		    ex_cri_desc * work_cri_desc,
		    const unsigned short work_atp_index,
		    ex_cri_desc * given_cri_desc,
		    ex_cri_desc * returned_cri_desc,
		    queue_index down,
		    queue_index up,
		    Lng32 num_buffers,
		    ULng32 buffer_size
		    );
  
  ~ComTdbGenericUtil();

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(1,getClassVersionID());
    ComTdb::populateImageVersionIDArray();
  }

  virtual short getClassSize()        { return (short)sizeof(ComTdbGenericUtil); }

  virtual Long pack(void *);
  virtual Lng32 unpack(void *, void * reallocator);
  
  Int32 orderedQueueProtocol() const;

  ex_expr * inputExpr() { return inputExpr_; }
  ex_expr * outputExpr() { return outputExpr_; }

  virtual Int32 numChildren() const { return 0; };
  virtual Int32 numExpressions() const { return 2; };
  virtual const ComTdb *getChild(Int32 child) const { return NULL; };
  virtual ex_expr* getExpressionNode(Int32 pos) 
  {
    switch (pos)
      {
      case 0: return (ex_expr*)inputExpr_.getPointer();
      case 1: return (ex_expr*)outputExpr_.getPointer();
      default: return NULL;
      }
  }
  
  virtual const char * getExpressionName(Int32 pos) const 
  {
    switch (pos)
      {
      case 0:return "inputExpr_";
      case 1:return "outputExpr_";
      default:return NULL;
      } // switch 
  }
  
  char * getQuery() { return query_; }
  UInt32 getQueryLen() { return queryLen_;}
  Int16  getQueryCharSet() { return queryCharSet_; }
  char * getObjectName() { return objectName_; }
  UInt32 getObjectNameLen() { return objectNameLen_;}

  UInt32 tuppIndex()    { return tuppIndex_; }
  UInt32 outputRowlen() { return outputRowlen_; }
 
  UInt32 workAtpIndex()    { return workAtpIndex_; }

protected:
  ExCriDescPtr workCriDesc_;           // 00-07
  NABasicPtr query_;                   // 08-15
  NABasicPtr objectName_;              // 16-23
  ExExprPtr inputExpr_;                // 24-31
  ExExprPtr outputExpr_;               // 32-39
  UInt32 queryLen_;                    // 40-43
  UInt32 objectNameLen_;               // 44-47
  UInt32 inputRowlen_;                 // 48-51
  UInt32 outputRowlen_;                // 52-55
  UInt32 flags_;                       // 56-59
  UInt32 tuppIndex_;                   // 60-63
  UInt16 workAtpIndex_;                // 64-65
  Int16  queryCharSet_;                // 66-67
  char fillersComTdbGenericUtil_[12];  // 68-79
};

////////////////////////////////////////////////////////////////////
// class ComTdbDDL
////////////////////////////////////////////////////////////////////
static const ComTdbVirtTableColumnInfo ddlVirtTableColumnInfo[] =
{
  { "DDL_OUTPUT",       0, COM_USER_COLUMN, REC_BYTE_V_ASCII,     2000, FALSE, SQLCHARSETCODE_UTF8 , 0, 0, 0, 0, 0, 0, 0, COM_NO_DEFAULT, "",NULL,NULL, COM_UNKNOWN_DIRECTION_LIT, 0}
};

class ComTdbDDL : public ComTdbGenericUtil
{
  friend class ExDDLTcb;
  friend class ExDDLwithStatusTcb;
  friend class ExDDLPrivateState;

public:
  ComTdbDDL()
  : ComTdbGenericUtil()
  {}

  ComTdbDDL(char * ddl_query,
	   ULng32 ddl_querylen,
	   Int16 ddl_querycharset,
	   char * schemaName,
	   ULng32 schemaNameLen,
	   ex_expr * input_expr,
	   ULng32 input_rowlen,
	   ex_expr * output_expr,
	   ULng32 output_rowlen,
	   ex_cri_desc * work_cri_desc,
	   const unsigned short work_atp_index,
           ex_cri_desc * given_cri_desc,
	   ex_cri_desc * returned_cri_desc,
           queue_index down,
           queue_index up,
           Lng32 num_buffers,
           ULng32 buffer_size
           );
  
  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize()        { return (short)sizeof(ComTdbDDL); }

  virtual const char *getNodeName() const { return "EX_DDL"; };

  static Int32 getVirtTableNumCols()
  {
    return sizeof(ddlVirtTableColumnInfo)/sizeof(ComTdbVirtTableColumnInfo);
  }

  static ComTdbVirtTableColumnInfo * getVirtTableColumnInfo()
  {
    return (ComTdbVirtTableColumnInfo*)ddlVirtTableColumnInfo;
  }

  static Int32 getVirtTableNumKeys()
  {
    return 0;
  }

  static ComTdbVirtTableKeyInfo * getVirtTableKeyInfo()
  {
    return NULL;
  }

  NABoolean createTable()
    { return (flags_ & CREATE_TABLE_DDL) != 0; }
  void setCreateTable(NABoolean v)
    { (v ? flags_ |= CREATE_TABLE_DDL : flags_ &= ~CREATE_TABLE_DDL); }

  NABoolean createIndex()
    { return (flags_ & CREATE_INDEX_DDL) != 0; }
  void setCreateIndex(NABoolean v)
    { (v ? flags_ |= CREATE_INDEX_DDL : flags_ &= ~CREATE_INDEX_DDL); }

  NABoolean createMV()
    { return (flags_ & CREATE_MV_DDL) != 0; }
  void setCreateMV(NABoolean v)
    { (v ? flags_ |= CREATE_MV_DDL : flags_ &= ~CREATE_MV_DDL); }

 NABoolean hbaseDDL()
    { return (flags_ & HBASE_DDL) != 0; }
  void setHbaseDDL(NABoolean v)
    { (v ? flags_ |= HBASE_DDL : flags_ &= ~HBASE_DDL); }

 NABoolean hbaseDDLNoUserXn()
    { return (flags_ & HBASE_DDL_NO_USER_XN) != 0; }
  void setHbaseDDLNoUserXn(NABoolean v)
    { (v ? flags_ |= HBASE_DDL_NO_USER_XN : flags_ &= ~HBASE_DDL_NO_USER_XN); }

 NABoolean returnStatus()
    { return (flags_ & RETURN_STATUS) != 0; }
  void setReturnStatus(NABoolean v)
    { (v ? flags_ |= RETURN_STATUS : flags_ &= ~RETURN_STATUS); }

protected:
  enum Flags 
  { 
    CREATE_TABLE_DDL         = 0x0002,
    CREATE_INDEX_DDL         = 0x0004,
    CREATE_MV_DDL               = 0x0008,
    HBASE_DDL                       = 0x0010,
    HBASE_DDL_NO_USER_XN  = 0x0020,
    RETURN_STATUS               = 0x0040
  };
 
  UInt16 flags_;                       // 00-01
  char fillersComTdbDDL_[30];          // 02-31
};

class ComTdbDDLwithStatus : public ComTdbDDL
{
  friend class ExDDLTcb;
  friend class ExDDLwithStatusTcb;
  friend class ExDDLPrivateState;

public:
  ComTdbDDLwithStatus()
  : ComTdbDDL()
  {}

  ComTdbDDLwithStatus(char * ddl_query,
	   ULng32 ddl_querylen,
	   Int16 ddl_querycharset,
	   char * schemaName,
	   ULng32 schemaNameLen,
	   ex_expr * input_expr,
	   ULng32 input_rowlen,
	   ex_expr * output_expr,
	   ULng32 output_rowlen,
	   ex_cri_desc * work_cri_desc,
	   const unsigned short work_atp_index,
           ex_cri_desc * given_cri_desc,
	   ex_cri_desc * returned_cri_desc,
           queue_index down,
           queue_index up,
           Lng32 num_buffers,
           ULng32 buffer_size
           );
  
  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize()        { return (short)sizeof(ComTdbDDLwithStatus); }

  virtual const char *getNodeName() const { return "EX_DDL_WITH_STATUS"; };

  void setGetMDVersion(NABoolean v)
  {(v ? flags2_ |= GET_MD_VERSION : flags2_ &= ~GET_MD_VERSION); }
  NABoolean getMDVersion() { return (flags2_ & GET_MD_VERSION) != 0;}

  void setGetSWVersion(NABoolean v)
  {(v ? flags2_ |= GET_SW_VERSION : flags2_ &= ~GET_SW_VERSION); }
  NABoolean getSWVersion() { return (flags2_ & GET_SW_VERSION) != 0;}

  void setMDupgrade(NABoolean v)
  {(v ? flags2_ |= MD_UPGRADE : flags2_ &= ~MD_UPGRADE); }
  NABoolean getMDupgrade() { return (flags2_ & MD_UPGRADE) != 0;}

  void setMDcleanup(NABoolean v)
  {(v ? flags2_ |= MD_CLEANUP : flags2_ &= ~MD_CLEANUP); }
  NABoolean getMDcleanup() { return (flags2_ & MD_CLEANUP) != 0;}

  void setCheckOnly(NABoolean v)
  {(v ? flags2_ |= CHECK_ONLY : flags2_ &= ~CHECK_ONLY); }
  NABoolean getCheckOnly() { return (flags2_ & CHECK_ONLY) != 0;}

  void setReturnDetails(NABoolean v)
  {(v ? flags2_ |= RETURN_DETAILS : flags2_ &= ~RETURN_DETAILS); }
  NABoolean getReturnDetails() { return (flags2_ & RETURN_DETAILS) != 0;}

  void setInitTraf(NABoolean v)
  {(v ? flags2_ |= INIT_TRAF : flags2_ &= ~INIT_TRAF); }
  NABoolean getInitTraf() { return (flags2_ & INIT_TRAF) != 0;}

protected:
  enum Flags 
  { 
    GET_MD_VERSION          = 0x0001,
    GET_SW_VERSION          = 0x0002,
    MD_UPGRADE              = 0x0004,
    MD_CLEANUP              = 0x0008,
    CHECK_ONLY              = 0x0010,
    RETURN_DETAILS          = 0x0020,
    INIT_TRAF               = 0x0040
  };
 
  UInt16 flags2_;                       // 00-01
  char fillersComTdbDDL_[30];          // 02-31

};

////////////////////////////////////////////////////////////////////
// class ComTdbProcessVolatileTable
////////////////////////////////////////////////////////////////////
class ComTdbProcessVolatileTable : public ComTdbDDL
{
  friend class ExProcessVolatileTableTcb;
  friend class ExProcessVolatileTablePrivateState;

public:
  ComTdbProcessVolatileTable()
  : ComTdbDDL()
  {}

  ComTdbProcessVolatileTable(char * ddl_query,
			     ULng32 ddl_querylen,
			     Int16 ddl_querycharset,
			     char * volTabName,
			     ULng32 volTabNameLen,
			     NABoolean isCreate,
			     NABoolean isTable,
			     NABoolean isIndex,
			     NABoolean isSchema,
			     char * schemaName,
			     ULng32 schemaNameLen,
			     ex_expr * input_expr,
			     ULng32 input_rowlen,
			     ex_expr * output_expr,
			     ULng32 output_rowlen,
			     ex_cri_desc * work_cri_desc,
			     const unsigned short work_atp_index,
			     ex_cri_desc * given_cri_desc,
			     ex_cri_desc * returned_cri_desc,
			     queue_index down,
			     queue_index up,
			     Lng32 num_buffers,
			     ULng32 buffer_size
			     );
  
  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() 
  { return (short)sizeof(ComTdbProcessVolatileTable); }

  virtual const char *getNodeName() const { return "EX_VOLATILE_TABLE"; };

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  void setIsCreate(NABoolean v)
  {(v ? flags_ |= IS_CREATE_ 
    : flags_ &= ~IS_CREATE_); };
  NABoolean isCreate() 
  { return (flags_ & IS_CREATE_) != 0; };

  void setIsTable(NABoolean v)
  {(v ? flags_ |= IS_TABLE_ 
    : flags_ &= ~IS_TABLE_); };
  NABoolean isTable() 
  { return (flags_ & IS_TABLE_) != 0; };

  void setIsIndex(NABoolean v)
  {(v ? flags_ |= IS_INDEX_ 
    : flags_ &= ~IS_INDEX_); };
  NABoolean isIndex() 
  { return (flags_ & IS_INDEX_) != 0; };

  void setIsSchema(NABoolean v)
  {(v ? flags_ |= IS_SCHEMA_ 
    : flags_ &= ~IS_SCHEMA_); };
  NABoolean isSchema() 
  { return (flags_ & IS_SCHEMA_) != 0; };

protected:
  enum
  {
    IS_CREATE_ = 0x0001,
    IS_TABLE_  = 0x0002,
    IS_INDEX_  = 0x0004,
    IS_SCHEMA_ = 0x0008
  };

  NABasicPtr volTabName_;              // 00-07
  UInt32 volTabNameLen_;               // 08-11
  UInt32 flags_;                       // 12-15
  char fillersComTdbPVT_[32];          // 16-47
};

////////////////////////////////////////////////////////////////////
// class ComTdbProcessInMemoryTable
////////////////////////////////////////////////////////////////////
class ComTdbProcessInMemoryTable : public ComTdbDDL
{
  friend class ExProcessInMemoryTableTcb;
  friend class ExProcessInMemoryTablePrivateState;

public:
  ComTdbProcessInMemoryTable()
  : ComTdbDDL()
  {}

  ComTdbProcessInMemoryTable(char * ddl_query,
			     ULng32 ddl_querylen,
			     Int16 ddl_querycharset,
			     char * objName,
			     ULng32 objNameLen,
			     NABoolean isCreate,
			     NABoolean isVolatile,
			     NABoolean isTable,
			     NABoolean isIndex,
			     NABoolean isMV,
			     char * schemaName,
			     ULng32 schemaNameLen,
			     ex_expr * input_expr,
			     ULng32 input_rowlen,
			     ex_expr * output_expr,
			     ULng32 output_rowlen,
			     ex_cri_desc * work_cri_desc,
			     const unsigned short work_atp_index,
			     ex_cri_desc * given_cri_desc,
			     ex_cri_desc * returned_cri_desc,
			     queue_index down,
			     queue_index up,
			     Lng32 num_buffers,
			     ULng32 buffer_size
			     );
  
  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize() 
  { return (short)sizeof(ComTdbProcessInMemoryTable); }

  virtual const char *getNodeName() const { return "EX_INMEMORY_TABLE"; };

  Long pack(void *);
  Lng32 unpack(void *, void * reallocator);

  void setIsCreate(NABoolean v)
  {(v ? flags_ |= IS_CREATE_ 
    : flags_ &= ~IS_CREATE_); };
  NABoolean isCreate() 
  { return (flags_ & IS_CREATE_) != 0; };

  void setIsVolatile(NABoolean v)
  {(v ? flags_ |= IS_VOLATILE_ 
    : flags_ &= ~IS_VOLATILE_); };
  NABoolean isVolatile() 
  { return (flags_ & IS_VOLATILE_) != 0; };

  void setIsTable(NABoolean v)
  {(v ? flags_ |= IS_TABLE_ 
    : flags_ &= ~IS_TABLE_); };
  NABoolean isTable() 
  { return (flags_ & IS_TABLE_) != 0; };

  void setIsIndex(NABoolean v)
  {(v ? flags_ |= IS_INDEX_ 
    : flags_ &= ~IS_INDEX_); };
  NABoolean isIndex() 
  { return (flags_ & IS_INDEX_) != 0; };

  void setIsMV(NABoolean v)
  {(v ? flags_ |= IS_MV_ 
    : flags_ &= ~IS_MV_); };
  NABoolean isMV() 
  { return (flags_ & IS_MV_) != 0; };

protected:
  enum
  {
    IS_CREATE_   = 0x0001,
    IS_VOLATILE_ = 0x0002,
    IS_TABLE_    = 0x0004,
    IS_INDEX_    = 0x0008,
    IS_MV_       = 0x0010
  };

  NABasicPtr objName_;                 // 00-07
  UInt32 objNameLen_;                  // 08-11
  UInt32 flags_;                       // 12-15
  char fillersComTdbIMT_[32];          // 16-47
};


////////////////////////////////////////////////////////////////////
// classes ComTdbDescribe, ExDescribeTcb, ExDescribePrivateState
////////////////////////////////////////////////////////////////////
class ComTdbDescribe : public ComTdbDDL
{
  friend class ExDescribeTcb;

public:
  enum DescribeType
  {
    INVOKE_ = 0, SHORT_, LONG_, PLAN_, LABEL_, LEAKS_, SHAPE_, SHOWSTATS_, TRANSACTION_, ENVVARS_
  };
  
  ComTdbDescribe()
  : ComTdbDDL() {};
  
  ComTdbDescribe(char * query,
		ULng32 querylen,
		Int16 querycharset,
		ex_expr * input_expr,
		ULng32 input_rowlen,
		ex_expr * output_expr,
		ULng32 output_rowlen,
		ex_cri_desc * work_cri_desc,
		const unsigned short work_atp_index,
		DescribeType type,
		ULng32 flags,
                ex_cri_desc * given_cri_desc,
		ex_cri_desc * returned_cri_desc,
                queue_index down,
                queue_index up,
                Lng32 num_buffers,
                Lng32 buffer_size);

  // ---------------------------------------------------------------------
  // Redefine virtual functions required for Versioning.
  //----------------------------------------------------------------------
  virtual short getClassSize()   { return (short)sizeof(ComTdbDescribe); }
  
  virtual const char *getNodeName() const { return "EX_DESCRIBE"; };

protected:

  Int16 type_;                         // 00-01
  char fillersComTdbDescribe2_[2];     // 02-03
  UInt32 flags_;                       // 04-07
  char fillersComTdbDescribe_[32];     // 08-39

};


#endif

