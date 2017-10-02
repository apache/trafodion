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
* File:         DgBaseType.cpp (previously part of /common/ComDiags.cpp)
* Description:  Implementation of DgSqlCode class and its subclasses.
*
* Created:      5/6/98
* Language:     C++
*
*
*
****************************************************************************
*/

#include "ComDiags.h"
#include "DgBaseType.h"

// no inline constructors for SRL's sake

DgSqlCode::DgSqlCode(Lng32 aSqlCode) : theSQLCODE_(aSqlCode) {}
DgSqlCode::DgSqlCode(Lng32 aSqlCode, ErrorOrWarning e) : theSQLCODE_(aSqlCode)
{
  if ((theSQLCODE_ > 0 && e == ERROR_) || (theSQLCODE_ < 0 && e == WARNING_))
    theSQLCODE_ = -theSQLCODE_;
}

DgCustomSQLState::DgCustomSQLState (const char * const cName)
 : theCharStr_(cName) {}
DgColumnName::DgColumnName (const char * const cName) : theCharStr_(cName) {}
DgCatalogName::DgCatalogName (const char * const cName) : theCharStr_(cName) {}
DgSchemaName::DgSchemaName (const char * const cName) : theCharStr_(cName) {}
DgTableName::DgTableName (const char * const cName) : theCharStr_(cName) {}

DgConstraintName::DgConstraintName (const char * const cName)
 : theCharStr_(cName) {}

DgConstraintCatalog::DgConstraintCatalog (const char * const cName)
 : theCharStr_(cName) {}

DgConstraintSchema::DgConstraintSchema (const char * const cName)
 : theCharStr_(cName) {}

DgRowNumber::DgRowNumber (Lng32 rownum) : theLong_(rownum) {}

DgTriggerCatalog::DgTriggerCatalog (const char * const cName)
 : theCharStr_(cName) {}

DgTriggerSchema::DgTriggerSchema (const char * const cName)
 : theCharStr_(cName) {}

DgTriggerName::DgTriggerName (const char * const cName)
 : theCharStr_(cName) {}

DgNskCode::DgNskCode(Lng32 l) : theLong_(l) {}

DgString0::DgString0 (const char * const str) : theCharStr_(str) {}
DgString1::DgString1 (const char * const str) : theCharStr_(str) {}
DgString2::DgString2 (const char * const str) : theCharStr_(str) {}
DgString3::DgString3 (const char * const str) : theCharStr_(str) {}
DgString4::DgString4 (const char * const str) : theCharStr_(str) {}

DgInt0::DgInt0(Lng32 l) : theLong_(l) {}
DgInt1::DgInt1(Lng32 l) : theLong_(l) {}
DgInt2::DgInt2(Lng32 l) : theLong_(l) {}
DgInt3::DgInt3(Lng32 l) : theLong_(l) {}
DgInt4::DgInt4(Lng32 l) : theLong_(l) {}

DGTYPE DgSqlCode::getTypeName() const { return DGSQLCODE; }
DGTYPE DgCustomSQLState::getTypeName() const { return DGCUSTOMSQLSTATE; }
DGTYPE DgColumnName::getTypeName() const { return DGCOLUMNNAME; }
DGTYPE DgCatalogName::getTypeName() const { return DGCATALOGNAME; }
DGTYPE DgSchemaName::getTypeName() const { return DGSCHEMANAME; }
DGTYPE DgTableName::getTypeName() const { return DGTABLENAME; }

DGTYPE DgConstraintCatalog::getTypeName() const
{ return DGCONSTRAINTCATALOG; }

DGTYPE DgConstraintSchema::getTypeName() const
{ return DGCONSTRAINTSCHEMA; }

DGTYPE DgConstraintName::getTypeName() const 
{ return DGCONSTRAINTNAME; }

DGTYPE DgRowNumber::getTypeName() const { return DGROWNUMBER; }

DGTYPE DgTriggerCatalog::getTypeName() const
{ return DGTRIGGERCATALOG; }

DGTYPE DgTriggerSchema::getTypeName() const
{ return DGTRIGGERSCHEMA; }

DGTYPE DgTriggerName::getTypeName() const 
{ return DGTRIGGERNAME; }

DGTYPE DgNskCode::getTypeName() const { return DGNSKCODE; }

DGTYPE DgString0::getTypeName() const { return DGSTRING0; }
DGTYPE DgString1::getTypeName() const { return DGSTRING1; }
DGTYPE DgString2::getTypeName() const { return DGSTRING2; }
DGTYPE DgString3::getTypeName() const { return DGSTRING3; }
DGTYPE DgString4::getTypeName() const { return DGSTRING4; }

DGTYPE DgInt0::getTypeName() const { return DGINT0; }
DGTYPE DgInt1::getTypeName() const { return DGINT1; }
DGTYPE DgInt2::getTypeName() const { return DGINT2; }
DGTYPE DgInt3::getTypeName() const { return DGINT3; }
DGTYPE DgInt4::getTypeName() const { return DGINT4; }

ComDiagsArea& operator<<(ComDiagsArea& d, const DgBase & dgObj)
{
  DGTYPE t;
  t = dgObj.getTypeName();
  switch(t)
    {
    case DGSQLCODE :
      {
	ComCondition * const cp = d.makeNewCondition();
      	  assert(cp != NULL);
	  cp->setSQLCODE(((DgSqlCode &)dgObj).getSQLCODE());
	  d.acceptNewCondition();
	return d;
      }
    case DGCUSTOMSQLSTATE :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setCustomSQLState(((DgCustomSQLState &)dgObj).getCharStr());
	return d;
      }
    case DGCOLUMNNAME :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setColumnName(((DgColumnName &)dgObj).getCharStr());
	return d;
      }
    case DGCATALOGNAME :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setCatalogName(((DgCatalogName &)dgObj).getCharStr());
	return d;
      }
    case DGSCHEMANAME :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setSchemaName(((DgSchemaName &)dgObj).getCharStr());
	return d;
      }
    case DGTABLENAME :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setTableName(((DgTableName &)dgObj).getCharStr());
	return d;
      }
    case DGCONSTRAINTCATALOG:
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setConstraintCatalog(((DgConstraintCatalog &)dgObj).getCharStr());
	return d;
      }
    case DGCONSTRAINTSCHEMA:
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setConstraintSchema(((DgConstraintSchema &)dgObj).getCharStr());
	return d;
      }
    case DGCONSTRAINTNAME:
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setConstraintName(((DgConstraintName &)dgObj).getCharStr());
	return d;
      }
    case DGROWNUMBER :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setRowNumber(((DgRowNumber&)dgObj).getLong());
	return d;
      }
    case DGTRIGGERCATALOG:
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setTriggerCatalog(((DgTriggerCatalog &)dgObj).getCharStr());
        return d;
      }
    case DGTRIGGERSCHEMA:
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setTriggerSchema(((DgTriggerSchema &)dgObj).getCharStr());
        return d;
      }
    case DGTRIGGERNAME:
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setTriggerName(((DgTriggerName &)dgObj).getCharStr());
        return d;
      }
    case DGNSKCODE :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setNskCode(((DgNskCode&)dgObj).getLong());
	return d;
      }
    case DGSTRING0 :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setOptionalString(0,((DgString0& )dgObj).getCharStr());
	return d;
      }
    case DGSTRING1 :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setOptionalString(1,((DgString1&)dgObj).getCharStr());
	return d;
      }
    case DGSTRING2 :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setOptionalString(2,((DgString2&)dgObj).getCharStr());
	return d;
      }
    case DGSTRING3 :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setOptionalString(3,((DgString3&)dgObj).getCharStr());
	return d;
      }
    case DGSTRING4 :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setOptionalString(4,((DgString4&)dgObj).getCharStr());
	return d;
      }

// Added for allowing DgWStringX
    case DGWSTRING0 :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setOptionalWString(0,((DgWString0& )dgObj).getWCharStr());
	return d;
      }
    case DGWSTRING1 :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setOptionalWString(1,((DgWString1&)dgObj).getWCharStr());
	return d;
      }
    case DGWSTRING2 :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setOptionalWString(2,((DgWString2&)dgObj).getWCharStr());
	return d;
      }
    case DGWSTRING3 :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setOptionalWString(3,((DgWString3&)dgObj).getWCharStr());
	return d;
      }
    case DGWSTRING4 :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setOptionalWString(4,((DgWString4&)dgObj).getWCharStr());
	return d;
      }
    
    case DGINT0 :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setOptionalInteger(0,((DgInt0&)dgObj).getLong());
	return d;
      }
    case DGINT1 :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setOptionalInteger(1,((DgInt1&)dgObj).getLong());
	return d;
      }
    case DGINT2 :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setOptionalInteger(2,((DgInt2&)dgObj).getLong());
	return d;
      }
    case DGINT3 :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setOptionalInteger(3,((DgInt3&)dgObj).getLong());
	return d;
      }
    case DGINT4 :
      {
	assert(d.getNumber() != 0);
	d[d.getNumber()].setOptionalInteger(4,((DgInt4&)dgObj).getLong());
	return d;
      }

    } // end switch.

  return d; 
} // end operator <<.

