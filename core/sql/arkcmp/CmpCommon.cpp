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
 * File:         CmpCommon.C
 * Description:  The implementation of CmpCommon class, which includes the
 *               static functions to get the contents of the CmpContext
 *               class (containing the global info for arkcmp).
 *
 *
 * Created:      09/05/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include <stdio.h>  // NT_PORT ( bd 10/24/96 ) define NULL
#include <iostream>

#include "Platform.h"

#include "CmpCommon.h"
#include "CmpContext.h"
#include "CmpErrors.h"
#include "CmpStatement.h"
#include "ErrorMessage.h"

#include "logmxevent.h"

#include "NADefaults.h"
#include "NewDel.h"
#include "SchemaDB.h"
#include "ControlDB.h"
#include "CompException.h"

#include "NAInternalError.h"

THREAD_P CmpContext* cmpCurrentContext=0;
THREAD_P jmp_buf CmpInternalErrorJmpBuf;

//
// Two jmp_buf pointers that the compiler will longjmp from. 
// These two pointers must be assigned to the addresss of the 
// jmp buffer when setjmp() is called when entering into the
// compiler. 
//
THREAD_P jmp_buf* CmpInternalErrorJmpBufPtr;
THREAD_P jmp_buf* ExportJmpBufPtr;

ComDiagsArea* CmpCommon::diags()
{
  return (cmpCurrentContext? cmpCurrentContext->diags() : 0);
}

CmpContext* CmpCommon::context()
{
  return cmpCurrentContext;
}

CmpStatement* CmpCommon::statement()
{
  return (cmpCurrentContext ? cmpCurrentContext->statement() : 0) ;
}

CollHeap* CmpCommon::contextHeap()
{
  return (cmpCurrentContext ? cmpCurrentContext->heap() : 0 );
}

NAHeap* CmpCommon::statementHeap()
{
  return (statement() ? statement()->heap() : 0);
}

TransMode * CmpCommon::transMode()
{
  TransMode& transMode = cmpCurrentContext->getTransMode();

  if (transMode.isolationLevel() == TransMode::IL_NOT_SPECIFIED_ &&
      ActiveSchemaDB())
    {
      // First time in, *after* CmpCommon/CmpContext/SchemaDB has been init'd
      // (Genesis 10-990224-2929); cf. SchemaDB::initPerStatement for more info.
      // Get current NADefaults setting of ISOLATION_LEVEL --
      // if invalid, use Ansi default of serializable -- then set our global.
      ActiveSchemaDB()->getDefaults().
        getIsolationLevel(transMode.isolationLevel());
      CMPASSERT(transMode.isolationLevel() != TransMode::IL_NOT_SPECIFIED_);
    }
  return &(cmpCurrentContext->getTransMode());
}

const NAString * CmpCommon::getControlSessionValue(const NAString &token)
{
  return ActiveControlDB()->getControlSessionValue(token);
}

// *********** About Error Handling in CmpAssertInternal ****************
// 1. TFDS must be done here to get a complete statck trace,
//    It is only enabled with a special control query default.
//    See the implemenation of makeTFDSCall().
// 3. Debugging support must be done here to give a complete stack trace.
// 3. Logging of EMS event is done in the outermost catch handler.
// 4. Adding Error/Warning to ComDiagsArea is done in the outermost catch
//    handler.
// **********************************************************************
void CmpAssertInternal(const char* condition, const char* file, Int32 num)
{
  NAInternalError::throwAssertException(condition, file, num);

  CmpInternalException(condition, file, num).throwException();
}

void CmpAbortInternal(const char* msg, const char* file, Int32 num)
{
  NAInternalError::throwFatalException(msg, file, num);

  throw EHBreakException(file, num);
}

void CmpCommon::dumpDiags(ostream& outStream, NABoolean newline)
{
  NADumpDiags(outStream, diags(), newline);
}

// The following functions allow access to the defaults table
// in sqlcomp/NADefaults.cpp; given the id number of the default, it
// returns the value
Lng32 CmpCommon::getDefaultLong(DefaultConstants id)
{
  return ActiveSchemaDB()->getDefaults().getAsLong(id);
}

double CmpCommon::getDefaultNumeric(DefaultConstants id)
{
  float result;

  if (!ActiveSchemaDB()->getDefaults().getFloat(id, result))
    { CMPASSERT("CmpCommon::getDefaultNumeric()" == NULL); }

  return result;
}

NAString CmpCommon::getDefaultString(DefaultConstants id)
{
  return ActiveSchemaDB()->getDefaults().getString(id);
}

DefaultToken CmpCommon::getDefault(DefaultConstants id,
				   Int32 errOrWarn)
{
  return ActiveSchemaDB()->getDefaults().getToken(id, errOrWarn);
}

DefaultToken CmpCommon::getDefault(DefaultConstants id,
				   NAString &result,
				   Int32 errOrWarn)
{
  return ActiveSchemaDB()->getDefaults().token(id, result, FALSE, errOrWarn);
}

NABoolean CmpCommon::wantCharSetInference()
{
  NAString defVal;
  return getDefault(INFER_CHARSET, defVal) == DF_ON;
}

void CmpCommon::applyDefaults(ComObjectName &nam)
{
  CMPASSERT(nam.isValid());
  if (nam.getCatalogNamePart().isEmpty()) {

    NAString dcat, dsch;
    ActiveSchemaDB()->getDefaults().getCatalogAndSchema(dcat, dsch);

    if (nam.getSchemaNamePart().isEmpty())
      nam.setSchemaNamePart(ComAnsiNamePart(dsch,CmpCommon::statementHeap()));
    nam.setCatalogNamePart(ComAnsiNamePart(dcat,CmpCommon::statementHeap()));
  }
}

