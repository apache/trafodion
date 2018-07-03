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
 *****************************************************************************
 *
 * File:         GenRelDCL.cpp
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

#define   SQLPARSERGLOBALS_FLAGS
#include "ComOptIncludes.h"
#include "ComQueue.h"
#include "ControlDB.h"
#include "GroupAttr.h"
#include "NATable.h"
#include "RelDCL.h"
#include "RelControl.h"

#include "Generator.h"
#include "GenExpGenerator.h"

#include "ExpCriDesc.h"
#include "ComTdb.h"
#include "ComTdbControl.h"
#include "ComTdbTransaction.h"

#include "ComTdbTimeout.h"  
#include "LateBindInfo.h"  


#include "SqlParserGlobals.h"   // Parser Flags

/////////////////////////////////////////////////////////////////////
//
// Contents:
//    
//   Control*::codeGen()
//   RelLock::codeGen()
//   RelTransaction::codeGen()
//
//////////////////////////////////////////////////////////////////////

short ControlAbstractClass::codeGen(Generator * generator)
{
  // No code needed if static compile and static-only statement.
  if (alterArkcmpEnvNow()) {
    if (!generator->explainDisabled()) generator->setExplainTuple(NULL);
    return 0;				// don't need any runtime code
  }

  Space * space = generator->getSpace();

  ControlQueryType cqt = SESSION_DEFAULT_;
  switch (getOperatorType()) {
    case REL_CONTROL_QUERY_DEFAULT:	cqt = DEFAULT_; break;
    case REL_CONTROL_QUERY_SHAPE:	cqt = SHAPE_;   break;
    case REL_CONTROL_SESSION:		cqt = CONTROL_SESSION_; break;
    case REL_CONTROL_TABLE:		cqt = TABLE_;   break;
    case REL_SET_SESSION_DEFAULT:	cqt = SESSION_DEFAULT_;   break;
    default:				GenAssert(FALSE, "cqt is unknown");
  }

  // trim leading and trailing spaces from token_ and value_
  token_ = token_.strip(NAString::both);
  value_ = value_.strip(NAString::both);

  // and upcase them
  token_.toUpper();
  if ((cqt == DEFAULT_) ||
      (cqt == TABLE_))
    {
      if ((value_ == "ON") ||
	  (value_ == "ENABLE") ||
	  (value_ == "TRUE"))
	value_ = "ON";
      else if ((value_ == "OFF") ||
	       (value_ == "DISABLE") ||
	       (value_ == "FALSE"))
	value_ = "OFF";
    }

  Int16 reset;
  if (cqt == DEFAULT_)
     reset = ((token_ == "") ? -reset_ : reset_);
  else
     reset = reset_;

  // if this is a SET SCHEMA stmt for a Hive schema, construct the value
  // as fully qualified schema name (cat.sch).
  // This string will be used at runtime to set this schema in Hive
  // if the schema exists.
  // See ExControlTcb::work for details.
  NABoolean isHiveSetSchema = FALSE;
  if ((cqt == DEFAULT_) && (dynamic()) && (token_ == "SCHEMA") && (reset == 0))
    {
      ComSchemaName csn(value_);
      NAString catName(csn.getCatalogNamePart().getInternalName());
      if (catName.isNull())
        catName = CmpCommon::getDefaultString(CATALOG);
      if (catName == HIVE_SYSTEM_CATALOG)
        {
          value_ = HIVE_SYSTEM_CATALOG;
          value_ += ".";
          value_ += csn.getSchemaNamePart().getInternalName();
          isHiveSetSchema = TRUE;
        }
    }

  // We need txt/tok/val stuff if in [1] a dynamic compile (EXEC SQL PREPARE),
  // OR [2] a dynamic statement even in a static compile.
  //
  // [1] is for Executor to maintain its ControlDB, to give to arkcmp
  //     if arkcmp crashes.
  // [2] is for SET CAT/SCH to work in embedded.

  char *v[] = { NULL, NULL, NULL };
  if (cqt != SHAPE_) {			// CQS has 0 args; CT has 3; others 2
    size_t i = 0;
    if (cqt == TABLE_)
      v[i++] = convertNAString(((ControlTable *)this)->
			  getTableName().getExposedNameAsAnsiString(), space);
    v[i++] = convertNAString(token_, space);
    v[i++] = convertNAString(value_, space);
  }

  ComTdbControl * control_tdb = new(space) 
    ComTdbControl(cqt,
		  reset,
		  convertNAString(sqlText_, space), (Int16)sqlTextCharSet_,
		  v[0],
		  v[1],
		  v[2],
		  (ex_cri_desc *)(generator->getCriDesc(Generator::DOWN)),
		  (ex_cri_desc *)(generator->getCriDesc(Generator::DOWN)),
		  (queue_index)4,	// ## Should be 2: this tmp-fixes error
		  (queue_index)4,	// ## -8816 if ExHandleArkcmpErrors in
		  1, 1024);		// ## ExControlTcb::work().
  generator->initTdbFields(control_tdb);
  if (cqt == DEFAULT_)
  {
     NABoolean nonResettable = 
        ActiveSchemaDB()->getDefaults().isNonResetableAttribute(v[0]);
     control_tdb->setNonResettable(nonResettable);
     control_tdb->setControlActionType(((ControlQueryDefault *)this)->getHoldOrRestoreCQD());

     if (dynamic()) // dynamic() is true for SET stmts
       {
         control_tdb->setIsSetStmt(TRUE);
         control_tdb->setIsHiveSetSchema(isHiveSetSchema);
       }
  }
  // no tupps are returned 
  generator->setCriDesc((ex_cri_desc *)(generator->getCriDesc(Generator::DOWN)),
			Generator::UP);
  generator->setGenObj(this, control_tdb);

  if (!generator->explainDisabled())
    generator->setExplainTuple(addExplainInfo(control_tdb, 0, 0, generator));

  return 0;
}

/////////////////////////////////////////////////////////
//
// RelLock::codeGen()
//
/////////////////////////////////////////////////////////
short RelLock::codeGen(Generator * generator)
{
  GenAssert(FALSE, "Lock not supported");
  return 0;
}

/////////////////////////////////////////////////////////
//
// RelTransaction::codeGen()
//
/////////////////////////////////////////////////////////
short RelTransaction::codeGen(Generator * generator)
{
  ExpGenerator * exp_gen = generator->getExpGenerator();
  Space * space = generator->getSpace();

  TransMode * mode = NULL;
  if (mode_)
  {
    mode = new(space) TransMode();
    *mode = *mode_;
  }
  
  ex_expr * diag_area_size_expr = 0;

  short workAtp = 1; // second atp 
  short workAtpIndex = 2;
  ex_cri_desc * work_cri_desc = new(space) ex_cri_desc(3, space);

  if (diagAreaSizeExpr_)
    {
      // generate expression to compute the diagnostic area size.
      // At runtime, the size is computed and moved to a temporary
      // location. Create that node.
      ItemExpr * daSize = 
	new(generator->wHeap()) Cast (diagAreaSizeExpr_, 
	new(generator->wHeap()) SQLInt(generator->wHeap(), TRUE, FALSE));
      daSize->setConstFoldingDisabled(TRUE);      
      
      daSize->bindNode(generator->getBindWA());
      
      // add location of diag area size to map table.
      Attributes * map_attr 
	= (generator->addMapInfo(daSize->getValueId(), 0))->getAttr();
      map_attr->setAtp(workAtp);
      map_attr->setAtpIndex(workAtpIndex);

      ULng32 len;
      ExpTupleDesc::computeOffsets(map_attr, 
				   ExpTupleDesc::SQLARK_EXPLODED_FORMAT, len);

      // generate expression
      exp_gen->generateArithExpr(daSize->getValueId(), ex_expr::exp_ARITH_EXPR,
				 &diag_area_size_expr);
    }

  ComTdbTransaction * trans_tdb = new(space) 
    ComTdbTransaction(type_, mode,
	       diag_area_size_expr,
	       work_cri_desc,
	       (ex_cri_desc *)(generator->getCriDesc(Generator::DOWN)),
	       (ex_cri_desc *)(generator->getCriDesc(Generator::DOWN)),
	       (queue_index)getDefault(GEN_TRAN_SIZE_DOWN),
	       (queue_index)getDefault(GEN_TRAN_SIZE_UP),
	       getDefault(GEN_TRAN_NUM_BUFFERS), 
	       getDefault(GEN_TRAN_BUFFER_SIZE));
  generator->initTdbFields(trans_tdb);

  // SET TRANSACTION is allowed within a transaction, if this query
  // is issued from odbc/jdbc/java.
  if ((CmpCommon::getDefault(ODBC_PROCESS) == DF_ON) ||
      (CmpCommon::getDefault(JDBC_PROCESS) == DF_ON))
    trans_tdb->setSetAllowedInXn(TRUE);

  // no tupps are returned 
  generator->setCriDesc((ex_cri_desc *)(generator->getCriDesc(Generator::DOWN)),
			Generator::UP);
  generator->setGenObj(this, trans_tdb);
  // begin work and set transaction statements don't 
  // need a read_write transaction but we set this flag
  // so that root_tdb::transmode gets whatever settings
  // are current in CmpCommon::Transmode. In other words
  // setting this flag implies that SQL/MX will not start
  // a READ ONLY transaction for this statement, unless
  // explicitly requested by the user.
  generator->setNeedsReadWriteTransaction(TRUE);
  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
       addExplainInfo(trans_tdb, 0, 0, generator));
  }

  // no aqr for trasaction control stmts (begin/commit/rollback/set)
  generator->setAqrEnabled(FALSE);

  return 0;
}


/////////////////////////////////////////////////////////
//
//  RelSetTimeout::codeGen()
//
/////////////////////////////////////////////////////////
short RelSetTimeout::codeGen(Generator * generator)
{
  ExpGenerator * exp_gen = generator->getExpGenerator();
  MapTable * map_table = generator->getMapTable();
  Space * space = generator->getSpace();
  ex_expr * timeout_value_expr = 0;

  short workAtp = 1; // second atp 
  short workAtpIndex = 2;
  ex_cri_desc * work_cri_desc = new(space) ex_cri_desc(3, space);

  //
  //     Extract the table-name
  //
  LateNameInfo* lateNameInfo = new(generator->wHeap()) LateNameInfo();
  
  const HostVar *hv = userTableName_.getPrototype() ;
  if ( hv != NULL)  {   // This is a host var
    char * varName;
    GenAssert(hv->getName().data(), "Hostvar pointer must have name");

    lateNameInfo->setEnvVar(hv->isEnvVar());

    varName = convertNAString(hv->getName(), generator->wHeap());
    strcpy(lateNameInfo->variableName(), varName);
    
    char * prototypeValue =  convertNAString(hv->getPrototypeValue(),
					     generator->wHeap());
    lateNameInfo->setCompileTimeName(prototypeValue, space);
    lateNameInfo->setLastUsedName(prototypeValue, space);
    strcpy(lateNameInfo->resolvedPhyName(), prototypeValue);
    lateNameInfo->setVariable(1);
  } // end of host-var
  else if ( isForAllTables_ ) { // a "*" was specified for a table name
    strcpy( lateNameInfo->resolvedPhyName(), "*" );  // special mark
    // Mark compile-time-name as alreay qualified (for RelRoot::codeGen() )
    strcpy(lateNameInfo->compileTimeAnsiName(), "\\DUMMY" ); // trick it !!
  }
  else {  // a specific table name was given; copy physical name
    GenAssert( physicalFileName_[0] == '\\' , 
	       "Full Guardian table name expected.");
    strcpy( lateNameInfo->resolvedPhyName(), physicalFileName_ );
    strcpy( lateNameInfo->compileTimeAnsiName(), physicalFileName_ );
  }

  lateNameInfo->setNameSpace(COM_TABLE_NAME);
  lateNameInfo->setInputListIndex(-1);
  generator->addLateNameInfo(lateNameInfo);
 
  //
  //  If needed, generate expression to compute the timeout value.
  // 
  if (timeoutValueExpr_)
    {
      // At runtime, the value is computed and moved to a temporary
      // location. Create that node.
      ItemExpr * toVal = 
	new(generator->wHeap()) Cast(timeoutValueExpr_, 
				     new(generator->wHeap()) 
				     SQLInt(generator->wHeap(), TRUE, FALSE));
      toVal->setConstFoldingDisabled(TRUE);      
      
      toVal->bindNode(generator->getBindWA());
  
      // add location of timeout value to map table.
      Attributes * map_attr 
	= (generator->addMapInfo(toVal->getValueId(), 0))->getAttr();
      map_attr->setAtp(workAtp);
      map_attr->setAtpIndex(workAtpIndex);

      ULng32 len;
      ExpTupleDesc::computeOffsets(map_attr, 
				   ExpTupleDesc::SQLARK_EXPLODED_FORMAT, len);

      // generate expression
      exp_gen->generateArithExpr(toVal->getValueId(), ex_expr::exp_ARITH_EXPR,
				 &timeout_value_expr);
    }
  
  ComTdbTimeout * timeout_tdb = new(space) 
    ComTdbTimeout( timeout_value_expr,
		   work_cri_desc,
		   (ex_cri_desc *)(generator->getCriDesc(Generator::DOWN)),
		   (ex_cri_desc *)(generator->getCriDesc(Generator::DOWN)),
		   (queue_index)getDefault(GEN_TIMEOUT_SIZE_DOWN),
		   (queue_index)getDefault(GEN_TIMEOUT_SIZE_UP),
		   getDefault(GEN_TIMEOUT_NUM_BUFFERS),
		   getDefault(GEN_TIMEOUT_BUFFER_SIZE));
  
  timeout_tdb->setStream( isStream_ );  // set the flags in the TCB
  timeout_tdb->setReset( isReset_ );

  // no tupps are returned 
  generator->setCriDesc((ex_cri_desc *)
			(generator->getCriDesc(Generator::DOWN)),
			Generator::UP);
  generator->setGenObj(this, timeout_tdb);
  if(!generator->explainDisabled()) {
    generator->setExplainTuple(
			       addExplainInfo(timeout_tdb, 0, 0, generator));
  }
  return 0;
}

