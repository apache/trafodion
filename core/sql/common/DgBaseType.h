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
#ifndef DGBASETYPE_H
#define DGBASETYPE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         DgBaseType.h
 * Description:  Types of condition information that can be added to a
 *               ComCondition object in the SQL Diagnostics area.
 *               See file /export/ComDiagsArea.h for details.
 * Created:      10/06/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "NAWinNT.h"  // to get NAWchar

class ComDiagsArea;


enum DGTYPE 
{
  DGSQLCODE,
  DGCUSTOMSQLSTATE,
  DGCOLUMNNAME,
  DGCATALOGNAME,
  DGSCHEMANAME,
  DGTABLENAME,
  DGCONSTRAINTCATALOG,
  DGCONSTRAINTSCHEMA,
  DGCONSTRAINTNAME,
  DGROWNUMBER,
  DGTRIGGERCATALOG,
  DGTRIGGERSCHEMA,
  DGTRIGGERNAME,
  DGNSKCODE,
  DGSTRING0,
  DGSTRING1,
  DGSTRING2,
  DGSTRING3,
  DGSTRING4,
  DGINT0,
  DGINT1,
  DGINT2,
  DGINT3,
  DGINT4,
  DGWSTRING0,
  DGWSTRING1,
  DGWSTRING2,
  DGWSTRING3,
  DGWSTRING4
};

// Due to the Catalog Manager need. I (Huy) introduced a new base class call DgBase
// and all other Dg classes will be derived from the base class.


class DgBase
{
public:
NA_EIDPROC 
  virtual DGTYPE getTypeName() const = 0;

}; 


NA_EIDPROC 
ComDiagsArea& operator<<(ComDiagsArea&, const DgBase &);


// The following are classes defined solely for the purpose
// of creating manipulators for streaming into a ComDiagsArea class.
//
// Note that these "data insertion" operators do nothing in the case
// that an overflow of conditions would occur (more than the length
// limit due to a new DgSqlCode going in), or, if areMore() is true.
// This functionality supports the concept of length limit of a ComDiagsArea.

class DgSqlCode : public DgBase
{
public:
  enum ErrorOrWarning { ERROR_ = -1, WARNING_ = +1 };
NA_EIDPROC
  DgSqlCode   (Lng32   aSqlCode);
NA_EIDPROC
  DgSqlCode   (Lng32   aSqlCode, ErrorOrWarning e);
NA_EIDPROC 
  Lng32        getSQLCODE() const {return theSQLCODE_; }
NA_EIDPROC 
  DGTYPE      getTypeName() const ;
private:
  Lng32        theSQLCODE_;
};


class DgColumnName : public DgBase
{
public:
NA_EIDPROC 
  DgColumnName  (const char * const);
NA_EIDPROC 
  const char * getCharStr  () const {return theCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const;
private:
  const char * const theCharStr_;
};


class DgCustomSQLState : public DgBase
{
public:
NA_EIDPROC 
  DgCustomSQLState  (const char * const);
NA_EIDPROC 
  const char * getCharStr  () const {return theCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const;
private:
  const char * const theCharStr_;
};


class DgCatalogName : public DgBase
{
public:
NA_EIDPROC 
  DgCatalogName  (const char * const);
NA_EIDPROC 
  const char * getCharStr  () const {return theCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const;
private:
  const char * const theCharStr_;
};


class DgSchemaName : public DgBase
{
public:
NA_EIDPROC 
  DgSchemaName  (const char * const);
NA_EIDPROC 
  const char * getCharStr  () const {return theCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const;
private:
  const char * const theCharStr_;
};


class DgTableName : public DgBase
{
public:
NA_EIDPROC 
  DgTableName  (const char * const);
NA_EIDPROC 
  const char * getCharStr  () const {return theCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const;
private:
  const char * const theCharStr_;
};


class DgConstraintCatalog : public DgBase
{
public:
NA_EIDPROC 
  DgConstraintCatalog  (const char * const);
NA_EIDPROC 
  const char * getCharStr  () const {return theCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


class DgConstraintSchema : public DgBase
{
public:
NA_EIDPROC 
  DgConstraintSchema  (const char * const);
NA_EIDPROC 
  const char * getCharStr  () const {return theCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


class DgConstraintName : public DgBase
{
public:
NA_EIDPROC 
  DgConstraintName  (const char * const);
NA_EIDPROC 
  const char * getCharStr  () const {return theCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


class DgRowNumber : public DgBase
{ 
public:
NA_EIDPROC 
  DgRowNumber  (Lng32);
NA_EIDPROC 
  Lng32 getLong  () const {return theLong_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const;
private:
  Lng32 theLong_;
};


class DgTriggerCatalog : public DgBase
{
public:
NA_EIDPROC
  DgTriggerCatalog  (const char * const);
NA_EIDPROC
  const char * getCharStr  () const {return theCharStr_; };
NA_EIDPROC
  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


class DgTriggerSchema : public DgBase
{
public:
NA_EIDPROC
  DgTriggerSchema  (const char * const);
NA_EIDPROC
  const char * getCharStr  () const {return theCharStr_; };
NA_EIDPROC
  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


class DgTriggerName : public DgBase
{
public:
NA_EIDPROC
  DgTriggerName  (const char * const);
NA_EIDPROC
  const char * getCharStr  () const {return theCharStr_; };
NA_EIDPROC
  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


class DgNskCode : public DgBase
{ 
public:
NA_EIDPROC 
  DgNskCode  (Lng32);
NA_EIDPROC 
  Lng32 getLong  () const {return theLong_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const;
private:
  Lng32 theLong_;
};


class DgString0 : public DgBase
{ 
public:
NA_EIDPROC 
  DgString0  (const char * const);
NA_EIDPROC 
  const char * getCharStr  () const {return theCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const;
private:
  const char * const theCharStr_;
};


class DgString1 : public DgBase 
{ 
public:
NA_EIDPROC 
  DgString1  (const char * const);
NA_EIDPROC 
  const char * getCharStr  () const {return theCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


class DgString2 : public DgBase
{ 
public:
NA_EIDPROC 
  DgString2  (const char * const);
NA_EIDPROC 
  const char * getCharStr  () const {return theCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


class DgString3 : public DgBase
{ 
public:
NA_EIDPROC 
  DgString3  (const char * const);
NA_EIDPROC 
  const char * getCharStr  () const {return theCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const;
private:
  const char * const theCharStr_;
};


class DgString4 : public DgBase
{ 
public:
NA_EIDPROC 
  DgString4  (const char * const);
NA_EIDPROC 
  const char * getCharStr  () const {return theCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


// Added to allow NCHAR error messages.
class DgWString0 : public DgBase
{ 
public:
NA_EIDPROC 
  DgWString0  (const NAWchar* const x) : theWCharStr_(x) {};
NA_EIDPROC 
  const NAWchar* getWCharStr  () const {return theWCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const { return DGWSTRING0; };
private:
  const NAWchar* const theWCharStr_;
};


class DgWString1 : public DgBase 
{ 
public:
NA_EIDPROC 
  DgWString1  (const NAWchar* const x) : theWCharStr_(x) {};
NA_EIDPROC 
  const NAWchar* getWCharStr  () const {return theWCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const { return DGWSTRING1; };
private:
  const NAWchar* const theWCharStr_;
};


class DgWString2 : public DgBase
{ 
public:
NA_EIDPROC 
  DgWString2  (const NAWchar* const x) : theWCharStr_(x) {};
NA_EIDPROC 
  const NAWchar* getWCharStr  () const {return theWCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const  { return DGWSTRING2; };
private:
  const NAWchar* const theWCharStr_;
};


class DgWString3 : public DgBase
{ 
public:
NA_EIDPROC 
  DgWString3  (const NAWchar* const x) : theWCharStr_(x) {};
NA_EIDPROC 
  const NAWchar* getWCharStr  () const {return theWCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const { return DGWSTRING3; };
private:
  const NAWchar* const theWCharStr_;
};


class DgWString4 : public DgBase
{ 
public:
NA_EIDPROC 
  DgWString4  (const NAWchar* const x) : theWCharStr_(x) {};
NA_EIDPROC 
  const NAWchar* getWCharStr  () const {return theWCharStr_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const { return DGWSTRING4; };
private:
  const NAWchar* const theWCharStr_;
};


class DgInt0 : public DgBase
{ 
public:
NA_EIDPROC 
  DgInt0  (Lng32);
NA_EIDPROC 
  Lng32 getLong  () const {return theLong_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const;
private:
  Lng32 theLong_;
};


class DgInt1 : public DgBase
{ 
public:
NA_EIDPROC 
  DgInt1  (Lng32);
NA_EIDPROC 
  Lng32 getLong  () const {return theLong_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const ;
private:
  Lng32 theLong_;
};


class DgInt2 : public DgBase
{ 
public:
NA_EIDPROC 
  DgInt2  (Lng32);
NA_EIDPROC 
  Lng32 getLong  () const {return theLong_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const ;
private:
  Lng32 theLong_;
};


class DgInt3 : public DgBase
{ 
public:
NA_EIDPROC 
  DgInt3  (Lng32);
NA_EIDPROC 
  Lng32 getLong  () const {return theLong_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const ;
private:
  Lng32 theLong_;
};


class DgInt4 : public DgBase
{ 
public:
NA_EIDPROC 
  DgInt4  (Lng32);
NA_EIDPROC 
  Lng32 getLong  () const {return theLong_; };
NA_EIDPROC 
  DGTYPE      getTypeName() const ;
private:
  Lng32 theLong_;
};


#endif // DGBASETYPE_H
