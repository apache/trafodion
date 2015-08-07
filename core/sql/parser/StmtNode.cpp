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
******************************************************************************
*
* File:         StmtNode.C
* Description:  Stmt nodes
* Created:      3/6/95
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "charinfo.h"
#include "ComOptIncludes.h"
#include "SchemaDB.h"
#include "StmtNode.h"


void action::setActionLabel(MBD_ACTION  newAction, const NAString &newLabel)
{
  theAction = newAction;
  theLabel = newLabel;
}

// -----------------------------------------------------------------------
// methods for class StmtNode 
// -----------------------------------------------------------------------


Int32 StmtNode::getArity() const { return 0; }

StmtNode * StmtNode::castToStatementExpr()
{
  return this;
}

const StmtNode * StmtNode::castToStatementExpr() const
{
  return this;
}

RelExpr * StmtNode::getQueryExpression() const { return NULL; }
  
ExprNode * StmtNode::getChild(Lng32 /* index */)
{
  assert(0 == 1);
  return NULL;
} // StmtNode::getChild()

void StmtNode::setChild(Lng32 /* index */, ExprNode * /* newChild */)
{
  assert(0 == 1);
} // StmtNode::setChild()

// -----------------------------------------------------------------------
// methods for class StmtQuery
// -----------------------------------------------------------------------
  
Int32 StmtQuery::getArity() const { return 1; }

RelExpr * StmtQuery::getQueryExpression() const { return queryExpr_; }
  
ExprNode * StmtQuery::getChild(Lng32 index)
{
  assert(index == 0);
  return getQueryExpression();
} // StmtNode::getChild()

void StmtQuery::setChild(Lng32 index, ExprNode * newChild)
{
  assert(index == 0);
  assert(newChild->castToRelExpr());
  queryExpr_ = newChild->castToRelExpr();
} // StmtQuery::setChild()

const NAString StmtQuery::getText() const
{
     return "StmtQuery";
} // StmtQuery::getText()

void StmtQuery::print(FILE * f,
		     const char * prefix,
		     const char *) const
{
  // print operator text
  fprintf(f,"%sExprNode(%s)\n", prefix, (const char *) getText());
}

// -----------------------------------------------------------------------
// methods for class StmtModule
// -----------------------------------------------------------------------

// Apply defaults to self, and then (ANSI 12.1 SR 3) apply self to SchemaDB.
NABoolean StmtModule::applyDefaults(NABoolean wantR18behavior)
{
  NABoolean err = FALSE;

  if (charSet().isNull())
    charSet() = CharInfo::getCharSetName(CharInfo::DefaultCharSet);

  if (CharInfo::isCharSetSupported(charSet())) {
    // Get charset name in canonical format (the name of the enum of the name).
    charSet() = CharInfo::getCharSetName(CharInfo::getCharSetEnum(charSet()));
  }
  else {
    *CmpCommon::diags() << DgSqlCode(-3010) << DgString0(charSet());
    err = TRUE;
  }

  if (!CharInfo::isModuleCharSetSupported(CharInfo::getCharSetEnum(charSet()))) 
  {
    *CmpCommon::diags() << DgSqlCode(-3404) << DgString0(charSet());
    err = TRUE;
  }

  // Here we're using internal-format names
  if (name().getCatalogName().isNull()) {

    // Must be an Ansi name, not an MPLOC.
    const SchemaName& defcs =
      ActiveSchemaDB()->getDefaultSchema(
        SchemaDB::REFRESH_CACHE | SchemaDB::FORCE_ANSI_NAMETYPE);

    if (name().getSchemaName().isNull()) {
      if (name().getObjectName().isNull()) {
        name().setObjectName("SQLMX_DEFAULT_MODULE_");
      }
      name().setSchemaName(defcs.getSchemaName());
    }
    name().setCatalogName(defcs.getCatalogName());

  }

  if (wantR18behavior) {
    // And now we use external-format names, for the ANSI 12.1 SR 3 stuff.
    NAString catName(name().getCatalogNameAsAnsiString());
    if (!ActiveSchemaDB()->getDefaults().setCatalog(catName))
      err = TRUE;
  
    NAString schName(name().getUnqualifiedSchemaNameAsAnsiString());
    if (!ActiveSchemaDB()->getDefaults().setSchema(schName))
      err = TRUE;
  }
  else { // want R2 (correct) behavior
    // We used to take the catalog & schema of the module directive and apply
    // them above as the default catalog & schema. This was a misguided
    // attempt to "use external-format names, for the ANSI 12.1 SR 3 stuff"
    // in the SQL92 std. 
    // 
    // In SQL99, this has been clarified in section 13.1 of ISO/IEC FDIS 
    // 9075-2:1999 (aka the 1999 Foundation doc) where syntax rules 3 & 4
    // specify that:
    // 
    // "If the explicit or implicit <schema name> does not specify a <catalog
    //  name>, then an implementation-defined <catalog name> is implicit."
    // 
    // "The implicit or explicit <catalog name> is the implicit <catalog name>
    //  for all unqualified <schema name>s in the <SQL-client module 
    //  definition>."
    //
    // Three observations may be worth pointing out here:
    // 1) SQL/MX client modules do not have a <module authorization clause> as
    //    specified by the SQL99 std.
    // 2) Even if (in the future) SQL/MX tries to conform to the SQL99 std for 
    //    client module definition(s), the SQL99 std itself (syntax rule 3) 
    //    allows us to use "an implementation-defined <catalog name>" to 
    //    implicitly qualify unqualified <schema name>s.
    // 3) This current "implementation is a deviation from ANSI in
    //    that the module name is a 3-part name. This deviation and the use of
    //    cat/sch name to qualify unqualified SQL objects in the module are
    //    also 'valid' implementation of the 2 syntax rules 13.1, rules 3 & 4
    //    of ANSI."
    // Similar logic can be applied to using an implementation-defined
    // <schema name> to implicitly qualify unqualified table names, etc.
    // 
    // In other words, our technique of using CQD default CATALOG & SCHEMA
    // settings to qualify unqualified table names, view names, etc is allowed
    // for by the SQL99 std.
    // 
    // Deleting the old code that was here is part of the fix to genesis
    // cases 10-030725-8215, 10-030730-8326, 10-030826-0792.
  }
  return err;
}

NABoolean StmtModule::unparse(NAString &result, NABoolean wantR18behavior)
{
  NABoolean err = applyDefaults(wantR18behavior);

  result += NAString("MODULE ") + 
            name().getQualifiedNameAsAnsiNTFilenameString() +
  	    " NAMES ARE " + charSet() + ";";

  return err;
}

NABoolean StmtModule::unparseSimple(NAString &result, 
                                    NABoolean wantR18behavior)
{
  NABoolean err = applyDefaults(wantR18behavior); 
  // assume caller will handle err

  result += NAString("MODULE ") + 
            name().getQualifiedNameAsAnsiString() +
  	    " NAMES ARE " + charSet() + ";";

  return err;
}
  
void StmtModule::applyModuleCatalogSchema(const NAString& cat, 
                                          const NAString& sch)
{
  // apply moduleCatalog if necessary
  if (name().getCatalogName().isNull()) {
    if (!cat.isNull())
      name().setCatalogName(cat);
  }

  // apply moduleSchema if necessary
  if (name().getSchemaName().isNull()) {
    if (!sch.isNull())
      name().setSchemaName(sch);
  }

  // module catalog & schema must be non-null now. Otherwise, a fatal
  // exception can occur in mxsqlc/mxsqlco in
  //   mod_def::setModule() --> mod_def::outputModule()
  //   --> StmtModule::unparseSimple() --> 
  //   QualifiedName::getQualifiedNameAsAnsiString() -->
  //   QualifiedName::getQualifiedNameAsString() -->
  //   CMPASSERT(NOT getSchemaName().isNull());
  // as reported in genesis case 10-030708-8735.
  if (name().getCatalogName().isNull() || name().getSchemaName().isNull()) {
    const SchemaName& defcs =
      ActiveSchemaDB()->getDefaultSchema(
        SchemaDB::REFRESH_CACHE | SchemaDB::FORCE_ANSI_NAMETYPE);
    if (name().getCatalogName().isNull())
      name().setCatalogName(defcs.getCatalogName());
    if (name().getSchemaName().isNull())
      name().setSchemaName(defcs.getSchemaName());
  }
}

// -----------------------------------------------------------------------
// methods for ...
// -----------------------------------------------------------------------
  
/* The constructor for StmtAllocStaticDesc                                  */
/* just takes its arguments and initializes the                             */
/* private data members from them.  Rather straightforward.                 */

StmtAllocStaticDesc::StmtAllocStaticDesc( NamePlusDesc* namePlusDescPtr,
                        StmtOrCursEnum theTag, 
                        NAString* theName) : StmtNode(STM_ALLOC_STATIC_DESC),
     mainDescriptor(namePlusDescPtr),
     entityTag(theTag),
     entityName( theTag == NAME_ABSENT ? NULL : theName)
{ }


NamePlusDesc::NamePlusDesc(NAString*   descName, 
              NABoolean   isInputFlag, 
              DescTypeList* theArray) :
   descriptorName(descName), isInput(isInputFlag), descriptorArray(theArray)
   { }


StmtDeclStatCurs::StmtDeclStatCurs(NAString* new_curs_name,
                                   RelExpr*  new_curs_spec,
				   NABoolean holdable		// QSTUFF
                                   ) :
    StmtNode(STM_DECL_STATCURS, holdable), 
    cursorName(new_curs_name),
    cursorSpec(new_curs_spec)
{ }

StmtDeclDynCurs::StmtDeclDynCurs(NAString* new_curs_name,
                                 NAString* new_stmt_name,
				 NABoolean holdable		// QSTUFF
                                 ) :
    StmtNode(STM_DECL_DYNCURS, holdable), 
    cursorName(new_curs_name),
    stmtName(new_stmt_name)
{ }

designation::designation(designator_tag newTag)
{
  assert(newTag != DT_DESCRIPTOR);
  theTag = newTag;
  descName = NULL;
}

designation::designation()
{
  
  descName = NULL;
  theTag = DT_NULL;
}

designation::designation(NAString* newDescName) 
{
  if (newDescName == NULL) 
    {
      descName = NULL;
      theTag = DT_DESC_VIA_HV;
    }
  else 
    {
      descName = newDescName;
      theTag = DT_DESCRIPTOR;
    }
}



StaticDescItem::StaticDescItem(ConstValue* newLiteral)
{
  assert(newLiteral != NULL);
  literalPointer = newLiteral;
  dataTypePointer = NULL;
}

StaticDescItem::StaticDescItem(HostVarExprType *dataPtr)
{
   assert(dataPtr!=NULL);
   literalPointer = NULL;
   dataTypePointer = dataPtr;
}


StaticDescItem::~StaticDescItem()
{
   delete literalPointer;
   delete dataTypePointer;
}

NABoolean    StaticDescItem::isDataTypePointer      () const
{
   return (dataTypePointer!=NULL);
}


NABoolean    StaticDescItem::isLiteralPointer       () const
{
   return (literalPointer != NULL);
}

ConstValue      *StaticDescItem::getConstValuePtr       ()
{
   ConstValue  *returnValue = literalPointer;
   assert(returnValue!=NULL);
   literalPointer = NULL;
   return returnValue;
}

HostVarExprType *StaticDescItem::getDataTypePtr         ()
{
   HostVarExprType *returnValue = dataTypePointer;
   assert(returnValue != NULL);
   dataTypePointer = NULL;
   return returnValue;
}

StmtDescBase::~StmtDescBase()
{
    delete descriptorName;
}

