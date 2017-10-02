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

  virtual DGTYPE getTypeName() const = 0;

}; 



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
  DgSqlCode   (Lng32   aSqlCode);
  DgSqlCode   (Lng32   aSqlCode, ErrorOrWarning e);

  Lng32        getSQLCODE() const {return theSQLCODE_; }

  DGTYPE      getTypeName() const ;
private:
  Lng32        theSQLCODE_;
};


class DgColumnName : public DgBase
{
public:

  DgColumnName  (const char * const);

  const char * getCharStr  () const {return theCharStr_; };

  DGTYPE      getTypeName() const;
private:
  const char * const theCharStr_;
};


class DgCustomSQLState : public DgBase
{
public:

  DgCustomSQLState  (const char * const);

  const char * getCharStr  () const {return theCharStr_; };

  DGTYPE      getTypeName() const;
private:
  const char * const theCharStr_;
};


class DgCatalogName : public DgBase
{
public:

  DgCatalogName  (const char * const);

  const char * getCharStr  () const {return theCharStr_; };

  DGTYPE      getTypeName() const;
private:
  const char * const theCharStr_;
};


class DgSchemaName : public DgBase
{
public:

  DgSchemaName  (const char * const);

  const char * getCharStr  () const {return theCharStr_; };

  DGTYPE      getTypeName() const;
private:
  const char * const theCharStr_;
};


class DgTableName : public DgBase
{
public:

  DgTableName  (const char * const);

  const char * getCharStr  () const {return theCharStr_; };

  DGTYPE      getTypeName() const;
private:
  const char * const theCharStr_;
};


class DgConstraintCatalog : public DgBase
{
public:

  DgConstraintCatalog  (const char * const);

  const char * getCharStr  () const {return theCharStr_; };

  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


class DgConstraintSchema : public DgBase
{
public:

  DgConstraintSchema  (const char * const);

  const char * getCharStr  () const {return theCharStr_; };

  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


class DgConstraintName : public DgBase
{
public:

  DgConstraintName  (const char * const);

  const char * getCharStr  () const {return theCharStr_; };

  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


class DgRowNumber : public DgBase
{ 
public:

  DgRowNumber  (Lng32);

  Lng32 getLong  () const {return theLong_; };

  DGTYPE      getTypeName() const;
private:
  Lng32 theLong_;
};


class DgTriggerCatalog : public DgBase
{
public:
  DgTriggerCatalog  (const char * const);
  const char * getCharStr  () const {return theCharStr_; };
  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


class DgTriggerSchema : public DgBase
{
public:
  DgTriggerSchema  (const char * const);
  const char * getCharStr  () const {return theCharStr_; };
  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


class DgTriggerName : public DgBase
{
public:
  DgTriggerName  (const char * const);
  const char * getCharStr  () const {return theCharStr_; };
  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


class DgNskCode : public DgBase
{ 
public:

  DgNskCode  (Lng32);

  Lng32 getLong  () const {return theLong_; };

  DGTYPE      getTypeName() const;
private:
  Lng32 theLong_;
};


class DgString0 : public DgBase
{ 
public:

  DgString0  (const char * const);

  const char * getCharStr  () const {return theCharStr_; };

  DGTYPE      getTypeName() const;
private:
  const char * const theCharStr_;
};


class DgString1 : public DgBase 
{ 
public:

  DgString1  (const char * const);

  const char * getCharStr  () const {return theCharStr_; };

  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


class DgString2 : public DgBase
{ 
public:

  DgString2  (const char * const);

  const char * getCharStr  () const {return theCharStr_; };

  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


class DgString3 : public DgBase
{ 
public:

  DgString3  (const char * const);

  const char * getCharStr  () const {return theCharStr_; };

  DGTYPE      getTypeName() const;
private:
  const char * const theCharStr_;
};


class DgString4 : public DgBase
{ 
public:

  DgString4  (const char * const);

  const char * getCharStr  () const {return theCharStr_; };

  DGTYPE      getTypeName() const ;
private:
  const char * const theCharStr_;
};


// Added to allow NCHAR error messages.
class DgWString0 : public DgBase
{ 
public:

  DgWString0  (const NAWchar* const x) : theWCharStr_(x) {};

  const NAWchar* getWCharStr  () const {return theWCharStr_; };

  DGTYPE      getTypeName() const { return DGWSTRING0; };
private:
  const NAWchar* const theWCharStr_;
};


class DgWString1 : public DgBase 
{ 
public:

  DgWString1  (const NAWchar* const x) : theWCharStr_(x) {};

  const NAWchar* getWCharStr  () const {return theWCharStr_; };

  DGTYPE      getTypeName() const { return DGWSTRING1; };
private:
  const NAWchar* const theWCharStr_;
};


class DgWString2 : public DgBase
{ 
public:

  DgWString2  (const NAWchar* const x) : theWCharStr_(x) {};

  const NAWchar* getWCharStr  () const {return theWCharStr_; };

  DGTYPE      getTypeName() const  { return DGWSTRING2; };
private:
  const NAWchar* const theWCharStr_;
};


class DgWString3 : public DgBase
{ 
public:

  DgWString3  (const NAWchar* const x) : theWCharStr_(x) {};

  const NAWchar* getWCharStr  () const {return theWCharStr_; };

  DGTYPE      getTypeName() const { return DGWSTRING3; };
private:
  const NAWchar* const theWCharStr_;
};


class DgWString4 : public DgBase
{ 
public:

  DgWString4  (const NAWchar* const x) : theWCharStr_(x) {};

  const NAWchar* getWCharStr  () const {return theWCharStr_; };

  DGTYPE      getTypeName() const { return DGWSTRING4; };
private:
  const NAWchar* const theWCharStr_;
};


class DgInt0 : public DgBase
{ 
public:

  DgInt0  (Lng32);

  Lng32 getLong  () const {return theLong_; };

  DGTYPE      getTypeName() const;
private:
  Lng32 theLong_;
};


class DgInt1 : public DgBase
{ 
public:

  DgInt1  (Lng32);

  Lng32 getLong  () const {return theLong_; };

  DGTYPE      getTypeName() const ;
private:
  Lng32 theLong_;
};


class DgInt2 : public DgBase
{ 
public:

  DgInt2  (Lng32);

  Lng32 getLong  () const {return theLong_; };

  DGTYPE      getTypeName() const ;
private:
  Lng32 theLong_;
};


class DgInt3 : public DgBase
{ 
public:

  DgInt3  (Lng32);

  Lng32 getLong  () const {return theLong_; };

  DGTYPE      getTypeName() const ;
private:
  Lng32 theLong_;
};


class DgInt4 : public DgBase
{ 
public:

  DgInt4  (Lng32);

  Lng32 getLong  () const {return theLong_; };

  DGTYPE      getTypeName() const ;
private:
  Lng32 theLong_;
};


#endif // DGBASETYPE_H
