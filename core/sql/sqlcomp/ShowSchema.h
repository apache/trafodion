#ifndef SHOWSCHEMA_H
#define SHOWSCHEMA_H
/* -*-C++-*-
******************************************************************************
*
* File:         ShowSchema.h
*
* Description:  A lightweight means for a requester (SQLCI's ENV command)
*		to get arkcmp's current default catalog and schema
*		(in Ansi format).
*		A similar means for ExSqlComp to get arkcmp's initial defaults,
*		to be reissued if arkcmp crashes, to ensure identical context
*		of the restarted arkcmp.
*
*
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
*
*       Nomina si nescis perit et cognitio rerum.
*	[If you do not know the names, the knowledge of things is also lost.]
*	        -- Carolus Linnaeus
*
******************************************************************************
*/

#include "Platform.h"
#include "BaseTypes.h"
#include "ExpError.h"
#include "NAStringDef.h"

class ShowSchema {
public:

  static const char *ShowControlDefaultSchemaMagic()
  { return                       "**cat.sch**"; }	// ident internal-fmt

  static const char *ShowSchemaStmt()
  { return "SHOWCONTROL DEFAULT \"**cat.sch**\";"; }	// ident delimited

  static Lng32 DiagSqlCode()
  { return ABS(EXE_INFO_DEFAULT_CAT_SCH); }

  static NABoolean getDefaultCatAndSch(NAString &cat, NAString &sch);

  // When you PREPARE the ShowSchema::ShowSchemaStmt(),
  // Describe::bindNode() returns "error" ShowSchema::DiagSqlCode().
  //
  // The intent of function ShowSchema::getDefaultCatAndSch()
  // is to do
  //	EXEC SQL GET DIAGNOSTICS :cat = CATALOG_NAME, :sch = SCHEMA_NAME;
  // but it is not written yet.
  //
  // See sqlci Env command, SqlciEnv::specialError/specialHandler,
  // and HandleCLIError for another way to get the same information.

};

class GetControlDefaults {			// Genesis 10-981211-5986
public:

  static const char *GetExternalizedDefaultsMagic()
  { return                       "**extlzd.deflts**"; }	   // ident internal-fmt

  static const char *GetExternalizedDefaultsStmt()
  { return "SHOWCONTROL DEFAULT \"**extlzd.deflts**\";"; }  // ident delimited

  static Lng32 DiagSqlCode()
  { return ABS(EXE_INFO_CQD_NAME_VALUE_PAIRS); }

  // Handled like the preceding class.
  // Used by ExSqlComp.

};
// This is just for test
#endif // SHOWSCHEMA_H
