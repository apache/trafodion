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
#ifndef _CLI_MSG_OBJ_H_
#define _CLI_MSG_OBJ_H_

//forwarded class declarations
class ComplexObject;

class CtrlStmtComplexObject : public ComplexObject {
public:
  CtrlStmtComplexObject(NAMemory * heap, char * sqlText, Int16 sqlTextCharSet,
			 CtrlStmtComplexObject *ctrlObj);
  CtrlStmtComplexObject(char * sqlText, Int16 sqlTextCharSet,
			 CtrlStmtComplexObject *ctrlObj);
  CtrlStmtComplexObject(NAMemory *heap);
  CtrlStmtComplexObject();
  virtual ~CtrlStmtComplexObject();
  virtual void freeSubObjects();
  virtual void sharedOperationSequence(MessageOperator *msgOp,
				       InputContainer *input,
				       OutputContainer *output);
  char * getSqlText();
  Int16 getSqlTextCharSet();
  CtrlStmtComplexObject * getCtrlStmt();
  void dump();
private:
  char * sqlText_;
  Int16 sqlTextCharSet_;
  CtrlStmtComplexObject *ctrlObj_;
};

class TransAttrComplexObject: public ComplexObject {
public:
  TransAttrComplexObject(NAMemory * heap, TransMode::AccessMode mode, 
    TransMode::IsolationLevel isoLv, Lng32 diagSize, 
    TransMode::RollbackMode rollbackMode, Lng32 autoabortInterval );
  TransAttrComplexObject(TransMode::AccessMode mode, 
    TransMode::IsolationLevel isoLv, Lng32 diagSize,
    TransMode::RollbackMode rollbackMode, Lng32 autoabortInterval );
  TransAttrComplexObject(NAMemory * heap);
  TransAttrComplexObject();
  virtual ~TransAttrComplexObject();
  virtual void freeSubObjects();
  virtual void sharedOperationSequence(MessageOperator *msgOp,
    InputContainer *input,
    OutputContainer *output);
  TransMode::AccessMode getAccessMode();
  TransMode::IsolationLevel getIsolationLevel();
  Lng32 getDiagSize();
  TransMode::RollbackMode getRollbackMode();
  Lng32 getAutoabortInterval();
  void setAccessMode(TransMode::AccessMode mode);
  void setRollbackMode(TransMode::RollbackMode rollbackMode);
  void setIsolationLevel(TransMode::IsolationLevel isoLv);
  void setDiagSize(Lng32 diagSize);
  void setAutoabortInterval(Lng32 autoabortInterval);
  void dump();
private:
  TransMode::AccessMode mode_;
  TransMode::IsolationLevel isoLv_;
  Lng32 diagSize_;
  TransMode::RollbackMode rollbackMode_;
  Lng32 autoabortInterval_;


};

// Stack allocated factory object.
// Should used singleton pattern. However, NSK security does not allow globals.
class CliComplexObjectFactory : public ComplexObjectFactory{
public:
  virtual ComplexObject *manufacture(NAMemory *heap, ComplexObjectType objType);
};
#endif
