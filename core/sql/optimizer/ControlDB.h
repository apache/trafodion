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
#ifndef CONTROL_DB_H
#define CONTROL_DB_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ControlDB.h
 * Description:  Repository for CONTROL statements
 *               (the ControlDB retains info during compilation of multiple
 *               statements, similar to SchemaDB)
 * Created:      6/12/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "RelControl.h"

#include <iosfwd>
using namespace std;

class ControlDB;
class ControlQueryDefault;
class ControlQueryShape;
class ControlTableOptions;

// -----------------------------------------------------------------------
// Global methods to support a ControlDB that
// survives multiple SQL statement compilations
// -----------------------------------------------------------------------

// return a pointer to the currently active control DB
ControlDB * ActiveControlDB();

class ControlTableOptions : public NABasicObject
{
public:
  enum TokenConsts {
    IF_LOCKED, MDAM, NOWAIT, PRIORITY, PRIORITY_DELTA, SIMILARITY_CHECK, TABLELOCK, TIMEOUT
  };

  struct CTTokens {
    const char * token_;
    TokenConsts const_;
  };

  ControlTableOptions();
  ControlTableOptions(const NAString &tableName);
  ~ControlTableOptions();

  const NAString &tableName() const { return tableName_; }

  void remove(const NAString &token);

  CollIndex numEntries() const { return tokens_->entries(); }

  const NAString &getToken(CollIndex index);
  const NAString &getValue(CollIndex index);
  const NAString *getValue(const NAString &token);
  void addTokenAndValue(const NAString &token, const NAString &value);

private:

  NAString tableName_;
  LIST(NAString *) *tokens_;
  LIST(NAString *) *values_;
};

class ControlSessionOption : public NABasicObject
{
public:
  ControlSessionOption();
  ~ControlSessionOption();

  const NAString &getToken() { return *token_; }
  const NAString &getValue() { return *value_; }
  const NAString *getValue(const NAString &token);
  void addTokenAndValue(const NAString &token, const NAString &value);

private:

  NAString * token_;
  NAString * value_;
};

// -----------------------------------------------------------------------
// Class to hold information about CONTROL statements.
//
// This is a prototype for now, handling only the CONTROL QUERY SHAPE
// statement (only one such statement can be active at a time for now).
// Note that this table lives in the SQLCOMP memory at this time,
// ultimately it will also have to be stored into the module file
// (for recompiles). I guess it may be a good idea to store the source
// text in the module file, since that takes care of versioning and
// data representation problems (we don't want to store RelExpr's
// in the module file).
// -----------------------------------------------------------------------

// To be used for Control Query Shape identifiers in SqlParser.y.
// A numeric ConstValue will be generated for those particular identifiers.
enum CQSIdentifierEnum
{
  _SPARSE_,
  _DENSE_,
  _SYSTEM_ 
};

// Helper function for CQS in SqlParser.y.
class ExprNodePtrList : public LIST(ExprNode *)
{
public:
  ExprNodePtrList(CollHeap* h) : LIST(ExprNode *)(h) {}
};

// Global function called by SqlParser.y.
ExprNode *DecodeShapeSyntax(const NAString &fname,
			    ExprNodePtrList *args,
			    ComDiagsArea *diags,
			    CollHeap *heap);

class ControlDB : public NABasicObject
{
public:
  
  ControlDB(); 
  ~ControlDB();

  void initPerStatement() { outStream_ = NULL; }

  ostream *&outStream() { return outStream_; }

  void setRequiredShape(ControlQueryShape *shape);
  ControlQueryShape * getRequiredShape() const { return requiredShape_; }
  NABoolean requiredShapeWasOnceNonNull() const
    { return requiredShapeWasOnceNonNull_; }

  LIST(ControlQueryDefault *) &getCQDList() { return cqdList_; }
  void setControlDefault(ControlQueryDefault *def);

  LIST(ControlTableOptions *) &getCTList()  { return *ctList_; }
  LIST(ControlSessionOption *) &getCSList()  { return csList_; }

  ControlTableOptions * getControlTableOptions(const NAString &tableName);

  NABoolean validate(ControlTable *ct);
  NABoolean setControlTableValue(ControlTable *ct);
  const NAString * getControlTableValue(const NAString &tableName,
  				        const NAString &token);
  const NAString * getControlTableValue(const NAString *tableName,
  				        const NAString &token)
  { return getControlTableValue(*tableName, token); }

  NABoolean validate(ControlSession *cs);
  NABoolean setControlSessionValue(ControlSession *cs);
  const NAString * getControlSessionValue(const NAString &token);

  Lng32 packedLengthControlTableOptions();
  Lng32 packControlTableOptionsToBuffer(char * buffer);
  Lng32 unpackControlTableOptionsFromBuffer(char * buffer);
  NABoolean isSameCTO(char * buffer, Lng32 bufLen);
  Lng32 saveCurrentCTO();
  Lng32 restoreCurrentCTO();

  Lng32 packedLengthControlQueryShape();
  Lng32 packControlQueryShapeToBuffer(char * buffer);
  Lng32 unpackControlQueryShapeFromBuffer(char * buffer);
  NABoolean isSameCQS(char * buffer, Lng32 bufLen);
  Lng32 saveCurrentCQS();
  Lng32 restoreCurrentCQS();
  
private:
  enum RQOsetting { RQO_MIN, RQO_SYS, RQO_HIGH, RQO_MAX, RQO_RESET };
  void do_one_RQO_CQD(const char *attrName,
                      const char *attrValue,
                      NABoolean reset);
  void doRobustQueryOptimizationCQDs(RQOsetting set);
  void resetRobustQueryOptimizationCQDs();
  void setRobustQueryOptimizationCQDs();

  void do_one_CQD(const char *attrName,
                  const char *attrValue,
                  NABoolean reset,
                  NADefaults::Provenance masterOrigin);

  void do_one_MVQR_CQD(const char *attrName,
                       const char *attrValue,
                       NABoolean reset);
  void doMVQRCQDs(Lng32 level);
  void resetMVQRCQDs();
  void setMVQRCQDs();

  void setHistogramCacheState();

  ControlTableOptions * getControlTableOption(const NAString &tableName,
					      CollIndex &index);
  void resetTableValue(const NAString &tableName, const NAString &token);
  CollIndex isValidToken(const NAString &token);

  void resetSessionValue(const NAString &token);
  ControlSessionOption * getControlSessionOption(const NAString &token,
  						 CollIndex &index);

  ControlQueryShape *requiredShape_;
  NABoolean requiredShapeWasOnceNonNull_;

  // used to save the current CQS during auto recomp, if a CQS
  // is passed in.
  // Restored once the auto recomp is done.
  ControlQueryShape *savedRequiredShape_;

  ostream *outStream_;

  // list of all the control defaults that were added by
  // issuing control default stmt.
  LIST(ControlQueryDefault*) cqdList_;

  // list of CONTROL TABLE values that were added.
  // One entry per table. Each entry is a list of control table values
  // that were specified for that table.
  LIST(ControlTableOptions*) * ctList_;

  LIST(ControlTableOptions*) * savedCtList_;

  // list of CONTROL SESSION values that were added.
  LIST(ControlSessionOption*) csList_;
};

#endif /* CONTROL_DB_H */

