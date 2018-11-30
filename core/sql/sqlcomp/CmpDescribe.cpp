//******************************************************************************
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
/**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         Describe.C
 * Description:
 *
 * Created:      4/15/95
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *****************************************************************************
 */

#define   SQLPARSERGLOBALS_FLAGS        // must precede all #include's
#define   SQLPARSERGLOBALS_NADEFAULTS

#include "Platform.h"

#include <ctype.h>
#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <time.h>

#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"  // must be the last #include.

#include "CmpCommon.h"
#include "CmpContext.h"
#include "CmpMain.h"
#include "CmpErrors.h"
#include "ComDiags.h"
#include "ComObjectName.h"
#include "ComOperators.h"
#include "ComSpace.h"
#include "ComTdbRoot.h"
#include "ComSmallDefs.h"
#include "ComUser.h"
#include "ControlDB.h"
#include "DatetimeType.h"
#include "FragDir.h"
#include "HeapLog.h"
#include "parser.h"
#include "RelControl.h"
#include "RelExpr.h"
#include "RelExeUtil.h"
#include "RelMisc.h"
#include "RelRoutine.h"
#include "RelScan.h"
#include "sql_id.h"
#include "Sqlcomp.h"
#include "ItemOther.h"
#include "NARoutine.h"
#include "LmJavaSignature.h"
#include "QueryText.h"
#include "wstr.h"
#include "SqlParserGlobals.h"
#include "hs_update.h"
#include "csconvert.h"
#include "charinfo.h"
#include "CmpDescribe.h"
#include "CmpDDLCatErrorCodes.h"

#include "CmpSeabaseDDL.h"
#include "CmpSeabaseDDLauth.h"
#include "HDFSHook.h"
#include "PrivMgrCommands.h"
#include "PrivMgrComponentPrivileges.h"

#include "Analyzer.h"
#include "ComSqlId.h"
#include "ExExeUtilCli.h"
#include "TrafDDLdesc.h"

#define CM_SIM_NAME_LEN 32


enum PrivTarget {
   PRIV_SCHEMA = 2,
   PRIV_TABLE,
   PRIV_PROCEDURE,
   PRIV_LIBRARY};

enum Display_Schema_Object {
   SCHEMA_TABLE,
   SCHEMA_TABLE_INDEX,
   SCHEMA_TABLE_VIEW,
   SCHEMA_TABLE_MV,
   SCHEMA_TABLE_TRIGGER,
   SCHEMA_TABLE_SYNONYM,
   SCHEMA_TABLE_STORED_PROCEDURE,
   SCHEMA_TABLE_TABLE_MAPPING_FUNCTION,
   SCHEMA_TABLE_SCALAR_FUNCTION,
   SCHEMA_TABLE_UNIVERSAL_FUNCTION,
   SCHEMA_TABLE_ROUTINE_ACTION
   };

extern "C"
{
  Lng32 SQLCLI_GetRootTdb_Internal( /*IN*/ CliGlobals * cliGlobals,
                                         /*INOUT*/ char * roottdb_ptr,
                                         /*IN*/ Int32 roottdb_len,
                                         /*INOUT*/ char * srcstr_ptr,
                                         /*IN*/ Int32 srcstr_len,
                                         /*IN*/ SQLSTMT_ID *statement_id);

  Lng32 SQLCLI_GetRootTdbSize_Internal(/*IN*/ CliGlobals * cliGlobals,
                                            /*INOUT*/ Lng32 * root_tdb_size,
                                            /*INOUT*/ Lng32* srcstr_size,
                                            /*IN*/ SQLSTMT_ID *statement_id);
}

extern CliGlobals * GetCliGlobals();

void outputLine(Space &space, NAString &outputText, size_t indent,
                       size_t indentDeltaOfLineFollowingLineBreak = 2,
                       NABoolean commentOut = FALSE,
                       const char *testSchema = NULL);

void outputLine(Space &space, const char *buf, size_t indent,
                       size_t indentDeltaOfLineFollowingLineBreak = 2,
                       NABoolean commentOut = FALSE,
                       const char *testSchema = NULL);

static size_t indexLastNewline(const NAString & text,
                               size_t startPos,
                               size_t maxLen);


static bool CmpDescribeLibrary(
   const CorrName   & corrName,
   char           * & outbuf,
   ULng32           & outbuflen,
   CollHeap         * heap);

static short CmpDescribeRoutine(
     const CorrName   & corrName,
     char           * & outbuf,
     ULng32           & outbuflen,
     CollHeap         * heap);
  
static short CmpDescribePlan(
   const char   *query,
   ULng32 flags,
   char         *&outbuf,
   ULng32 &outbuflen,
   NAMemory      *heap);

static short CmpDescribeShape(
   const char    *query,
   char         *&outbuf,
   ULng32 &outbuflen,
   NAMemory      *h);

static short CmpDescribeTransaction(
   char         *&outbuf,
   ULng32 &outbuflen,
   NAMemory      *h);

short CmpDescribeSequence ( 
                             const CorrName  &dtName,
                             char* &outbuf,
                             ULng32 &outbuflen,
                             CollHeap *heap,
                             Space *inSpace);

bool CmpDescribeIsAuthorized( 
   SQLOperation operation = SQLOperation::UNKNOWN,
   PrivMgrUserPrivs *privs = NULL,
   ComObjectType objectType = COM_UNKNOWN_OBJECT);

// The real Ark catalog manager returns all object names as three-part
// identifiers, properly delimited where necessary.
// Only simple column names are returned undelimited;
// only they should use this procedure.
#define ANSI_ID(name)   ToAnsiIdentifier(name).data()

// ## This only applies to CatSim; remove it!
#define CONSTRAINT_IS_NAMED(constrdesc) \
          (constrdesc->constrnts_desc.constrntname && \
          *constrdesc->constrnts_desc.constrntname)

// Define a shorter synonym.
#define SpacePrefix     CmpDescribeSpaceCountPrefix

static short CmpDescribePlan    (const char *query,
                                                             ULng32 flags,
                          char *&outbuf,
                          ULng32 &outbuflen,
                          NAMemory *h);


// DZC - outputToStream does nothing for SHOWDDL because outStream() returns NULL
// needed for SHOWCONTROL though, so they must be left in. 
static void outputToStream(const char *buf, NABoolean commentOut = FALSE)
{
  if (ActiveControlDB()->outStream())
    *(ActiveControlDB()->outStream()) << (commentOut ? "--" : "") << buf << endl;
}

/*
static void outputToStreamNoNewline(const char *buf, NABoolean commentOut = FALSE)
{
  if (ActiveControlDB()->outStream())
    *(ActiveControlDB()->outStream()) << (commentOut ? "--" : "") << buf;
}
*/

void outputShortLine(Space &space, const char *buf, NABoolean commentOut = FALSE)
{
  // DZC - if commentOut = TRUE, prepend buf with "--"
  NAString outputString(buf);
  if(commentOut)
    outputString.prepend("--");
  space.outputbuf_ = TRUE;
  space.allocateAndCopyToAlignedSpace(outputString, outputString.length(), SpacePrefix);
  outputToStream(buf ? buf : "", commentOut); //DZC - this does nothing
}


void outputColumnLine(Space &space, NAString pretty, 
                      Int32 &colcount, NABoolean isForMV = FALSE,
                      NABoolean isDivisionColumn = FALSE)
{
  // Must linebreak before prepending comma+space, else long colname won't
  // display correctly.
  if (isDivisionColumn)
    LineBreakSqlText(pretty, FALSE/*showddlView*/, 79/*maxlen*/,
                     6/*pfxlen*/, 4/*pfxinitlen*/, ' '/*pfxchar*/,
                     NULL/*testSchema*/, isDivisionColumn/*commentOut*/);
  else
  LineBreakSqlText(pretty, FALSE, 79, 6, 4);    // initial indent 4, subseq 6
  if (colcount++)                               // if not first line,
    pretty[(size_t)2] = ',';                    // change "    " to "  , "
  space.outputbuf_ = TRUE;
  space.allocateAndCopyToAlignedSpace(pretty, pretty.length(), SpacePrefix);
  if (isDivisionColumn)
  {
    // Do not prepend the leading "--" comment prefix because we have
    // already done that in the LineBreakSqlText() call.
    outputToStream(pretty, FALSE/*commentOut*/);
  }
  else
  outputToStream(pretty, isForMV);
}

void outputLine(Space &space, const char *buf, size_t indent,
                size_t indentDeltaOfLineFollowingLineBreak,
                NABoolean commentOut,
                const char *testSchema)
{
  NAString pretty(buf);
  outputLine(space, pretty, indent, indentDeltaOfLineFollowingLineBreak, 
             commentOut, testSchema);
}

// NAstring version of outputLine so there are fewer conversions
void outputLine(Space &space, NAString &outputText, size_t indent,
                size_t indentDeltaOfLineFollowingLineBreak,
                NABoolean commentOut, 
                const char *testSchema)
{
  if(commentOut)
    outputText.prepend("--");

  LineBreakSqlText(outputText, FALSE, 79,
                   indent + indentDeltaOfLineFollowingLineBreak, 0, ' ',
                   testSchema);
  space.outputbuf_ = TRUE;
  space.allocateAndCopyToAlignedSpace(outputText, outputText.length(), 
    SpacePrefix);
  outputToStream(outputText, commentOut);
}

// To output lines with more than 3000 characters
void outputLongLine(Space &space, NAString & outputText, size_t indent,
                    const char *testSchema = NULL, NABoolean commentOut = FALSE)
{
  if (commentOut)
    LineBreakSqlText(outputText, TRUE, 79, indent+2, 0, ' ', testSchema, commentOut);
  else
  LineBreakSqlText(outputText, TRUE, 79, indent+2, 0, ' ', testSchema);

  // Can only print out 3000 characters at a time due to generator limitation
  // This limitation only applies to views because view DDL is printed out as
  // a continuous string of text (vs. other DDL which is outputted piecemeal).
  // This code assumes that there must be at least one space in every 3000
  // chars of text.

  const size_t CHUNK_SIZE = 3000;
  size_t textLen = outputText.length();
  space.outputbuf_ = TRUE;

  // If there's only one chunk, print it
  if (textLen <= CHUNK_SIZE) 
  {
    space.allocateAndCopyToAlignedSpace(outputText, textLen, SpacePrefix);
  }
  else
  { 
    // Output multiple chunks
    size_t textStartPos = 0;
    size_t finalTextStartPos = textLen - CHUNK_SIZE;
    size_t chunkLen = 0;
    NABoolean isFinalChunk = FALSE;
    do
    {
      isFinalChunk = (textStartPos >= finalTextStartPos);
      // Final chunk may be smaller than CHUNK_SIZE
      chunkLen = (isFinalChunk) ? (textLen - textStartPos) : CHUNK_SIZE;
      if (!isFinalChunk)
      {
        // Use the last newline in this chunk as the boundary between this
        // chunk and the next chunk.
        chunkLen = indexLastNewline(outputText, textStartPos, chunkLen);
        CMPASSERT(chunkLen != NA_NPOS);
      }
      space.allocateAndCopyToAlignedSpace(outputText(textStartPos, chunkLen).data(),
                                          chunkLen, SpacePrefix);
      textStartPos += chunkLen + 1;  // + 1 skips over newline
    } while (!isFinalChunk);
  }
}


// Return index of closing right quote that matches the left quote
// text(startPos).  Return NA_NPOS if closing right quote is not found.
// Searches text(startPos+1) through text(endPos-1).
static size_t skipQuotedText(const NAString & text,
                             size_t startPos,
                             size_t endPos)
{
  size_t closeQuotePos = NA_NPOS;

  char quoteChar = text(startPos);

  for (size_t pos = startPos + 1; pos < endPos; pos++)
  {
    char c = text(pos);
    if (c == quoteChar)
    {
      if ( (pos != (endPos - 1))
           && (text(pos + 1) == quoteChar) )
        pos++;  // skip embedded quote
      else 
      {
        closeQuotePos = pos;
        break;
      }
    }
  }

  return closeQuotePos;
}

// Return the offset from startPos of the last unquoted newline that occurs in
// the substring text(startPos, maxLen).  Return NA_NPOS if the substring
// contains fewer than maxLen characters (and therefore doesn't need to be
// broken up) or we can't find an unquoted newline.
//
static size_t indexLastNewline(const NAString & text,
                               size_t startPos,
                               size_t maxLen)
{
  size_t newlinePos = NA_NPOS;

  size_t endPos = startPos + maxLen;
  if (text.length() <= endPos)
    return NA_NPOS;

  // search text(startPos) through text(endPos - 1)
  for (size_t pos = startPos; pos < endPos; pos++)
  {
    char c = text[pos];
    if ((c == '"') OR (c == '\''))
    {
      pos = skipQuotedText(text, pos, endPos);
      // If a quoted string was split across the chunk, we won't find the 
      // right-hand quote; break out of loop and return the offset of last  
      // found (if any) newline character.
      if (pos == NA_NPOS)
         break;
    }
    else 
       if (c == '\n')
       {
         newlinePos = pos;
       }
  }

  // Convert newlinePos to offset from startPos
  if (newlinePos != NA_NPOS)
    newlinePos = (newlinePos - startPos);

  return newlinePos;
}

static Int32 displayDefaultValue(const char * defVal, const char * colName,
                                 NAString &displayableDefVal)
{
  displayableDefVal.append(defVal);
  return 0;
}

static short CmpDescribeShowQryStats(
   const char    *query,
   char         *&outbuf,
   ULng32        &outbuflen,
   NAMemory      *heap)
{
  // make sure we have a showstats query to process
  short rc = -1; 
  if (!query) return rc;

  // Skip leading blanks
  const char *cc = query;
  while(isSpace8859_1(*cc)) cc++;

  // scan "showstats" token
  char lowertok[10];
  const char *ctok=cc;
  while (isAlpha8859_1(*cc)) cc++;
  strncpy(lowertok, ctok, 9);
  lowertok[9] = '\0';
  if (stricmp(lowertok, "showstats")) return rc;

  // skip blanks
  while(isSpace8859_1(*cc)) cc++;

  // scan "for" token
  ctok = cc;
  while (isAlpha8859_1(*cc)) cc++;
  strncpy(lowertok, ctok, 3);
  lowertok[3] = '\0';
  if (stricmp(lowertok, "for")) return rc;

  // skip blanks
  while(isSpace8859_1(*cc)) cc++;

  // scan "query" token
  ctok = cc;
  while (isAlpha8859_1(*cc)) cc++;
  strncpy(lowertok, ctok, 5);
  lowertok[5] = '\0';
  if (stricmp(lowertok, "query")) return rc;

  // skip blanks
  while(isSpace8859_1(*cc)) cc++;

  // a "showstats for query <q>" does its work by:
  //   1) at start, CmpCommon::context()->setShowQueryStats()
  //   2) sqlcomp -- analyzer & cardinality estimation code will consult
  //      CmpCommon::context()->showQueryStats() to do their part in
  //      showing internal histogram stuff
  //   3) at end, CmpCommon::context()->resetShowQueryStats()
  CmpCommon::context()->setShowQueryStats();

  // prepare this query.
  char * qTree = NULL;
  ULng32 dummyLen;

  CmpMain sqlcomp;
  CmpMain::CompilerPhase phase = CmpMain::ANALYSIS;

  // "showstats for query <q>" will cause QueryAnalysis::analyzeThis() to
  // call QueryAnalysis::setHistogramsToDisplay() which will set
  // QueryAnalysis::Instance()->statsToDisplay_ which will be displayed
  // by QueryAnalysis::Instance()->showQueryStats() below.
  QueryText qText((char*)cc, SQLCHARSETCODE_ISO88591);
  CmpMain::ReturnStatus rs = 
    sqlcomp.sqlcomp(qText, 0, &qTree, &dummyLen, heap, phase);
  CmpCommon::context()->resetShowQueryStats();
  if (rs)
    {
      return rc;
    }

  Space space;
  char buf[10000];
  QueryAnalysis::Instance()->showQueryStats(query, &space, buf);
  outputShortLine(space, buf);

  outbuflen = space.getAllocatedSpaceSize();
  outbuf = new (heap) char[outbuflen];
  space.makeContiguous(outbuf, outbuflen);
  return 0;
}

// Returns -1, if error.
short CmpDescribe(const char *query, const RelExpr *queryExpr,
                  char* &outbuf, ULng32 &outbuflen,
                  CollHeap *heap)
{
  short rc = 0;  // assume success

  // save the current parserflags setting

  ULng32 savedParserFlags;
  SQL_EXEC_GetParserFlagsForExSqlComp_Internal(savedParserFlags);

  // OK, folks, we are about to locally change a global variable, so any returns 
  // must insure that the global variable gets reset back to its original value
  // (saved above in "savedParserFlags"). So, if you are tempted to code a
  // "return", don't do it. Set the rc instead and goto the "finally" label.

  Set_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL);
  SQL_EXEC_SetParserFlagsForExSqlComp_Internal(INTERNAL_QUERY_FROM_EXEUTIL);

 // add an exception handler around all the SQL specific code
 try
 {
  // Display triggers using this object
  NABoolean showUsingTriggers = !!getenv("SQLMX_SHOW_USING_TRIGGERS");

  NABoolean showUsingMVs = !!getenv("SQLMX_SHOW_USING_MVS"); 
  // MV
  NABoolean showUnderlyingBT = !!getenv("SQLMX_SHOW_MVS_UNDERLYING_BASE_TABLE");

  Lng32 replShowddl = (Lng32) CmpCommon::getDefaultNumeric(SHOWDDL_FOR_REPLICATE);
  NABoolean logFormat = (CmpCommon::getDefault(SHOWDDL_DISPLAY_FORMAT) == DF_LOG);
  Lng32 replIOVersion = (Lng32) CmpCommon::getDefaultNumeric(REPLICATE_IO_VERSION);
  // Init SHOWDDL MX-format table regression test support for schema
  // names longer than 'SCH'.
  const char *testSchema = NULL;
  NAString testSchemaName;
  if (CmpCommon::context()->getSqlmxRegress() == 1)
  {
    const char *env = getenv("TEST_SCHEMA_NAME");
    if (env) 
    {
      const size_t SCH_LEN= 3;  // 'SCH' schema length
      testSchemaName = env;
      if (testSchemaName.length() > SCH_LEN)
        testSchema = testSchemaName.data();
    }
  }

  char *buf = NULL;
  Space space;

  // emit an initial newline
  outputShortLine(space, " ");

  // initialize the returned values
  outbuf = NULL;
  outbuflen = 0;

  CMPASSERT(queryExpr->getOperatorType() == REL_ROOT);
  Describe *d = (Describe *)(queryExpr->child(0)->castToRelExpr());
  CMPASSERT(d->getOperatorType() == REL_DESCRIBE);   

  if (d->getIsSchema())
    {
      if (!CmpDescribeIsAuthorized())
        {
          rc = -1;
          goto finally;
        }
      NAString schemaText;
      std::vector<std::string> outlines;
      QualifiedName objQualName(d->getDescribedTableName().getQualifiedNameObj(),
                                STMTHEAP);

      NABoolean isHiveRegistered = FALSE;
      if (objQualName.isHive())
        {
          CorrName cn(SEABASE_SCHEMA_OBJECTNAME, STMTHEAP,
                      objQualName.getSchemaName(),
                      objQualName.getCatalogName());
          cn.setSpecialType(ExtendedQualName::SCHEMA_TABLE);
          
          BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
          NATable *naTable = bindWA.getNATable(cn); 
          if ((naTable == NULL) || (bindWA.errStatus()))
            {
              CmpCommon::diags()->clear();

              *CmpCommon::diags() << DgSqlCode(-CAT_SCHEMA_DOES_NOT_EXIST_ERROR)
                                  << DgString0(objQualName.getCatalogName())
                                  << DgString1(objQualName.getSchemaName());

              rc = -1;
              goto finally;
            }
                                         
          isHiveRegistered = naTable->isRegistered();
        }

      if (!CmpSeabaseDDL::describeSchema(objQualName.getCatalogName(),
                                         objQualName.getSchemaName(),
                                         isHiveRegistered,
                                         outlines))
                                         //schemaText))
        {
          rc = -1;
          goto finally;
        }
        
      for(int i = 0; i < outlines.size(); i++)
         outputLine(space, outlines[i].c_str(), 0);
 
      outbuflen = space.getAllocatedSpaceSize();
      outbuf = new (heap) char[outbuflen];
      space.makeContiguous(outbuf, outbuflen);

      goto finally;  // we are done and rc is already 0
    }
  
  // If SHOWDDL USER, go get description and return
  if (d->getIsUser())
    {
      NAString userText;
      CmpSeabaseDDLuser userInfo;
      if (!userInfo.describe(d->getAuthIDName(), userText))
        {
          rc = -1;
          goto finally;
        }

      outputLine(space, userText,0);
      outbuflen = space.getAllocatedSpaceSize();
      outbuf = new (heap) char[outbuflen];
      space.makeContiguous(outbuf, outbuflen);

      goto finally;  // we are done and rc is already 0
    }

  // If SHOWDDL ROLE, go get description and return
  if (d->getIsRole())
  {
      NAString roleText;
      CmpSeabaseDDLrole roleInfo(ActiveSchemaDB()->getDefaults().getValue(SEABASE_CATALOG));
      if (!roleInfo.describe(d->getAuthIDName(),roleText))
        {
          rc = -1;
          goto finally;
        }

      outputLine(space, roleText,0);
      outbuflen = space.getAllocatedSpaceSize();
      outbuf = new (heap) char[outbuflen];
      space.makeContiguous(outbuf, outbuflen);

      goto finally;  // we are done and rc is already 0
   }

  // If SHOWDDL COMPONENT, go get description and return
  if (d->getIsComponent())
    {
      if (!CmpDescribeIsAuthorized(SQLOperation::MANAGE_COMPONENTS))
        {
          rc = -1;
          goto finally;
        }
       std::vector<std::string> outlines;
       std::string privMDLoc = ActiveSchemaDB()->getDefaults().getValue(SEABASE_CATALOG);
       privMDLoc += ".\"";
       privMDLoc += SEABASE_PRIVMGR_SCHEMA;
       privMDLoc += "\"";
       PrivMgrCommands privMgrInterface(privMDLoc,CmpCommon::diags());

       //get description in REGISTER, CREATE, GRANT statements.
       CmpSeabaseDDL cmpSBD((NAHeap*)heap);
       if (cmpSBD.switchCompiler())
         {
           *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);
           rc = -1;
           goto finally;
         }

       if(!privMgrInterface.describeComponents(d->getComponentName().data(), outlines))
         {
           *CmpCommon::diags() << DgSqlCode(-CAT_TABLE_DOES_NOT_EXIST_ERROR)
                        << DgTableName(d->getComponentName().data());
           cmpSBD.switchBackCompiler();
           rc = -1;
           goto finally;
         }
       cmpSBD.switchBackCompiler();
           
       for(int i = 0; i < outlines.size(); i++)
         outputLine(space, outlines[i].c_str(), 0);
       outbuflen = space.getAllocatedSpaceSize();
       outbuf = new (heap) char[outbuflen];
       space.makeContiguous(outbuf, outbuflen);
       goto finally;  // we are done and rc is already 0
    }

  if (d->getDescribedTableName().getQualifiedNameObj().getObjectNameSpace() == COM_LIBRARY_NAME)
    {
      if (!CmpDescribeLibrary(d->getDescribedTableName(),outbuf,outbuflen,heap))
        {
          rc = -1;
          goto finally;
        }
         
      goto finally;  // we are done and rc is already 0
    }

  if (d->getDescribedTableName().getQualifiedNameObj().getObjectNameSpace() == 
      COM_UDF_NAME)
    {
      if (!CmpDescribeRoutine(d->getDescribedTableName(),outbuf,outbuflen,heap))
        {
          rc = -1;
          goto finally;
        }
         
      goto finally;  // we are done and rc is already 0
    }

  ExtendedQualName::SpecialTableType tType =
    d->getDescribedTableName().getSpecialType();
  // Don't reveal exception table (special table syntax)
  if ( (CmpCommon::getDefault(IS_DB_TRANSPORTER) == DF_OFF) &&
       (tType == ExtendedQualName::EXCEPTION_TABLE) )
      tType = ExtendedQualName::NORMAL_TABLE;

  NAString upshiftedLocName(d->getDescribedTableName().getLocationName());
  upshiftedLocName.toUpper();
  NAString *pLocName = NULL;
  if (d->getDescribedTableName().isLocationNameSpecified())
    pLocName = &upshiftedLocName;

  if (d->getFormat() == Describe::PLAN_)
    {
      // Return runtime plan.
      // This command (SHOWPLAN) is for internal debugging use only.

      CMPASSERT(query);

      // Skip leading blanks
      for ( ; *query == ' '; query++) ;

      // Skip the SHOWPLAN token & option (if specified)(length to be determined) in front of the input query.
      Int32 i=8;
      while(query[i] == ' ') i++;
      if(str_cmp(query+i,"option",6) == 0 || str_cmp(query+i,"OPTION",6) == 0)
      {
                        while(query[i] != '\'' && query[i] != '\0') i++;
         i++;
                        while(query[i] != '\'' && query[i] != '\0') i++;
         i++;
      }
      rc = CmpDescribePlan(&query[i], d->getFlags(), outbuf, outbuflen, heap);
      goto finally;  // we are done
    }

  if (d->getFormat() == Describe::SHAPE_)
    {
      // Return CONTROL QUERY SHAPE statement for this query.

      CMPASSERT(query);

      // Skip leading blanks
      for ( ; *query == ' '; query++) ;

      // Skip the SHOWSHAPE token (length 9) in front of the input query.
      rc = CmpDescribeShape(&query[9], outbuf, outbuflen, heap);
      goto finally;  // we are done
    }

  if (d->getFormat() == Describe::SHOWQRYSTATS_)
    {
      // show histogram for root of this query.
      rc = CmpDescribeShowQryStats(query, outbuf, outbuflen, heap);
      goto finally;  // we are done
    }

  if (d->getIsControl())
    {
      rc = CmpDescribeControl(d, outbuf, outbuflen, heap);
      goto finally;  // we are done
    }
 
  if (d->getFormat() == Describe::LEAKS_)
    {
#ifdef NA_DEBUG_HEAPLOG
      if ((d->getFlags()) & LeakDescribe::FLAG_ARKCMP)
        outbuflen = HeapLogRoot::getPackSize();
      else
        outbuflen = 8;
      HEAPLOG_OFF();
      outbuf = (char *)heap->allocateMemory(outbuflen);
      HEAPLOG_ON();
      HeapLogRoot::pack(outbuf, d->getFlags());
#endif
      goto finally;  // we are done and rc is already 0
    }

  if (d->getFormat() == Describe::SHOWSTATS_)
    {
      rc = (short)ShowStats(query, outbuf, outbuflen, heap);
      goto finally;  // we are done
    }

  if (d->getFormat() == Describe::TRANSACTION_)
    {
      rc = CmpDescribeTransaction(outbuf, outbuflen, heap);
      goto finally;  // we are done
    }
    
    
  // Start SHOWDDL code

  // check if this is a hive table. If so, describe using hive info from NATable.
  // For now, schemaName of HIVE indicates a hive table.
  // Need to fix that at a later time when multiple hive schemas are supported.
  if (((d->getFormat() == Describe::INVOKE_) ||
       (d->getFormat() == Describe::SHOWDDL_)) &&
      (d->getDescribedTableName().isHive()) &&
      (!d->getDescribedTableName().isSpecialTable()))
    {
      rc = 
        CmpDescribeHiveTable(d->getDescribedTableName(), 
                             (d->getFormat() == Describe::INVOKE_ ? 1 : 2),
                             outbuf, outbuflen, heap,
                             d->getIsDetail());
      goto finally;  // we are done
    }

  if (d->getLabelAnsiNameSpace() == COM_SEQUENCE_GENERATOR_NAME)
    {
      rc = 
        CmpDescribeSequence(d->getDescribedTableName(), 
                            outbuf, outbuflen, heap, NULL);
      goto finally;  // we are done
    }

  // check if this is an hbase/seabase table. If so, describe using info from NATable.
  if (((d->getFormat() == Describe::INVOKE_) ||
       (d->getFormat() == Describe::SHOWDDL_)) &&
      ((d->getDescribedTableName().isHbase()) ||
       (d->getDescribedTableName().isSeabase())))
    {
      rc = 
        CmpDescribeSeabaseTable(d->getDescribedTableName(), 
                                (d->getFormat() == Describe::INVOKE_ ? 1 : 2),
                                outbuf, outbuflen, heap, NULL, NULL, TRUE,
                                FALSE, FALSE, FALSE, FALSE, UINT_MAX, FALSE,
                                NULL, 0, NULL, NULL, NULL,
                                d->getIsDetail());
      goto finally;  // we are done
    }

  TrafDesc *tabledesc = NULL;
  if ( ExtendedQualName::isDescribableTableType(tType) )
  {
    *CmpCommon::diags() << DgSqlCode(-4222)
                        << DgString0("DESCRIBE");
  }
  else  // special virtual table
    {
      HiveMDaccessFunc hiveMDF;
      const NAString& virtualTableName =
        d->getDescribedTableName().getQualifiedNameObj().getObjectName();
      if (strncmp(virtualTableName.data(), "HIVEMD_", 7) == 0)
        {
          NAString mdType = virtualTableName;
          mdType = mdType.strip(NAString::trailing, '_');
          mdType = mdType(7, mdType.length()-7);

          hiveMDF.setMDtype(mdType.data());

          tabledesc = hiveMDF.createVirtualTableDesc();
        }
      else
        {
          // not supported
          *CmpCommon::diags() << DgSqlCode(-3131);
          rc = -1;
          goto finally;
        }
    }
  
  
  if (!tabledesc)
    {
      rc = -1;
      goto finally;
    }

  NAString tableName(tabledesc->tableDesc()->tablename) ;
  
  NABoolean external = d->getIsExternal();
  //strip catalog off of table name
  if (external) 
  {
    ComObjectName externName(tableName, COM_TABLE_NAME);
    if (logFormat || (replShowddl != 0))
    {
       if ((replIOVersion >= 5 && replIOVersion <= 10) ||
           (replIOVersion >= 14))
         tableName = externName.getObjectNamePartAsAnsiString();
       else
         tableName = externName.getExternalName(FALSE, TRUE);
    }
    else
      tableName = externName.getExternalName(FALSE, TRUE);
  }
  
  NABoolean isVolatile =
                  FALSE;
  NABoolean isInMemoryObjectDefn = 
                  FALSE;


  if(isVolatile)
        Set_SqlParser_Flags(ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);

  // Allocate enough for a big view or constraint definition
  const Int32 LOCAL_BIGBUF_SIZE = 30000;
  buf = new (CmpCommon::statementHeap()) char[LOCAL_BIGBUF_SIZE];
  CMPASSERT(buf);

  TrafDesc *viewdesc = tabledesc->tableDesc()->views_desc;

  if (d->getFormat() == Describe::INVOKE_)
    {
      time_t tp;
      time(&tp);
      
      if (viewdesc)
        sprintf(buf, "-- Definition of view %s\n"
                     "-- Definition current %s",
                     tableName.data(), ctime(&tp));
      else if ( tType == ExtendedQualName::TRIGTEMP_TABLE )
        sprintf(buf, "-- Definition of table (TEMP_TABLE %s)\n"
                     "-- Definition current  %s",
                     tableName.data(), ctime(&tp));
      else if ( tType == ExtendedQualName::EXCEPTION_TABLE )
        sprintf(buf, "-- Definition of table (EXCEPTION_TABLE %s)\n"
                     "-- Definition current  %s",
                     tableName.data(), ctime(&tp));
      else if ( tType == ExtendedQualName::IUD_LOG_TABLE )
        sprintf(buf, "-- Definition of table (IUD_LOG_TABLE %s)\n"
                     "-- Definition current  %s",
                     tableName.data(), ctime(&tp));
      else if ( tType == ExtendedQualName::RANGE_LOG_TABLE )
        sprintf(buf, "-- Definition of table (RANGE_LOG_TABLE %s)\n"
                     "-- Definition current  %s",
                     tableName.data(), ctime(&tp));
      

      else
      {
        if (isVolatile)
          {
            ComObjectName cn(tableName.data());
            sprintf(buf,  "-- Definition of %svolatile table %s\n"
                    "-- Definition current  %s",
                    (isInMemoryObjectDefn ? "InMemory " : ""),
                    cn.getObjectNamePartAsAnsiString().data(),
                    ctime(&tp));
          }
        else if (isInMemoryObjectDefn)
          {
            sprintf(buf,  "-- Definition of InMemory table %s\n"
                    "-- Definition current  %s",
                    tableName.data(), ctime(&tp));
          }
        else
          {
            sprintf(buf,  "-- Definition of table %s\n"
                    "-- Definition current  %s",
                    tableName.data(), ctime(&tp));
          }
      }

      outputShortLine(space, buf);

      outputShortLine(space, "  ( ");
      NABoolean displayAddedCols = FALSE; // dummy variable
      // this may be a problem when describing virtual tables like EXPLAIN
      *CmpCommon::diags() << DgSqlCode(-4222)
                          << DgString0("INVOKE for TABLE");
      outputShortLine(space, "  )");
      
      outbuflen = space.getAllocatedSpaceSize();
      outbuf = new (heap) char[outbuflen];
      space.makeContiguous(outbuf, outbuflen);

      NADELETEBASIC(buf, CmpCommon::statementHeap());
      goto finally;  // we are done and rc is already 0
    }

   
   // This is done for SHOWDDL only. Not for SHOWLABEL
  if(logFormat)
    outputShortLine(space, "--> BT ");
  sprintf(buf, "CREATE TABLE %s",
          tableName.data());

  outputLine(space, buf, 0, 2, FALSE, testSchema);

  const NABoolean commentOut = 
    FALSE;
  const NABoolean mustShowBT =
    TRUE;
  if(mustShowBT)
  {
    outputShortLine(space, "  (", commentOut);
  }

  NABoolean displayAddedCols = FALSE;

  // this may be a problem when describing virtual tables like EXPLAIN
  *CmpCommon::diags() << DgSqlCode(-4222)
                      << DgString0("SHOWDDL for TABLE");

  Int32 pkey_colcount = 0;
  
  // Flag for additional alter table statements for constraints
  NABoolean displayAlterTable = FALSE; 
  // Flag for whether primary key belongs in a ALTER TABLE statement
  NABoolean alterTablePrimaryKey = FALSE;

  buf[0] = '\0';

  if(mustShowBT)
  {
    outputShortLine(space, "  ;", commentOut);
  }
  
  
  outbuflen = space.getAllocatedSpaceSize();
  outbuf = new (heap) char[outbuflen];
  space.makeContiguous(outbuf, outbuflen);

  Reset_SqlParser_Flags(ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);

  NADELETEBASIC(buf, CmpCommon::statementHeap());

 }  // end of try block

 // exception handling
 catch(...)
 {
    // Check diags area, if it doesn't contain an error, add an
    // internal exception
    if (CmpCommon::diags()->getNumber() == 0)
    {
      *CmpCommon::diags() << DgSqlCode(-2006);
      *CmpCommon::diags() << DgString0("Unknown error returned while retrieving metadata");
      *CmpCommon::diags() << DgString1(__FILE__);
      *CmpCommon::diags() << DgInt0(__LINE__);
    }
    rc = -1;
 }

finally:

  // Restore parser flags settings to what they originally were
  Assign_SqlParser_Flags(savedParserFlags);
  SQL_EXEC_AssignParserFlagsForExSqlComp_Internal(savedParserFlags);
  
  return rc;

} // SHOWDDL


short exeImmedCQD(const char *cqd, NABoolean hold)
{
  Int32 retcode = 0;

  ExeCliInterface cliInterface(CmpCommon::statementHeap());
  if (hold)
    retcode = cliInterface.holdAndSetCQD(cqd, "ON");
  else
    retcode = cliInterface.restoreCQD(cqd);
  if (retcode < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      return retcode;
    }
      
  return 0;
}

short exeImmedOneStmt(const char *controlStmt)
{
  Int32 retcode = 0;

  ExeCliInterface cliInterface(CmpCommon::statementHeap());
  retcode = cliInterface.executeImmediate(controlStmt);
  if (retcode < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return retcode;
    }
      
  return 0;
}
  
short sendAllControls(NABoolean copyCQS,
                      NABoolean sendAllCQDs,
                      NABoolean sendUserCQDs,
                      enum COM_VERSION versionOfCmplrRcvCntrlInfo,
                      NABoolean sendUserCSs,
                      CmpContext* prevContext)
{
  // -----------------------------------------------------------------------
  //  Given a SQL query, this procedure invokes the appropriate
  //  CLI calls to prepare and execute the statement.
  // -----------------------------------------------------------------------
  Int32 retcode = 0;

  char *buf =  new (CmpCommon::statementHeap()) char[15000];


  if (copyCQS) 
    {
      if (ActiveControlDB()->getRequiredShape())
        {
          retcode = 
            exeImmedOneStmt(ActiveControlDB()->getRequiredShape()->getShapeText());
          if (retcode)
            return (short)retcode;
        }
      else if (ActiveControlDB()->requiredShapeWasOnceNonNull())
        {
          // Genesis solution 10-040528-6511: If we possibly
          // sent a CQS down once before, we need to send a
          // CQS CUT to get rid of it if it is no longer 
          // active.
          strcpy(buf,"CONTROL QUERY SHAPE CUT;");
          retcode = exeImmedOneStmt(buf);       
            if (retcode)
              return((short)retcode);
        }
    }

  CollIndex i;
  ControlDB *cdb = NULL;

  // if prevContext is defined, get the controlDB from the previous context so the 
  // defaults are copied from the previous cmp context to the new cmp context. This is only 
  // required for embedded compilers where a SWITCH is taking place
  if (prevContext)
     cdb = prevContext->getControlDB();
  else
     cdb = ActiveControlDB();

  if ((sendAllCQDs) ||
      (sendUserCQDs))
    {
      // send all externalized CQDs.
      NADefaults &defs = CmpCommon::context()->schemaDB_->getDefaults();
      
      for (i = 0; i < defs.numDefaultAttributes(); i++)
        {
          const char *attrName, *val;
          if (defs.getCurrentDefaultsAttrNameAndValue(i, attrName, val,
                                                      sendUserCQDs))
            {
              if (NOT defs.isReadonlyAttribute(attrName))
                {
                  if (strcmp(attrName, "ISO_MAPPING") == 0)
                  {
                    // ISO_MAPPING is a read-only default attribute --
                    // But for internal development and testing purpose,
                    // we sometimes want to change the setting within
                    // an MXCI session. Make sure that we send the CQD
                    // COMP_BOOL_58 'ON' before sending CQD ISO_MAPPING
                    // so that the child/receiving MXCMP does not reject
                    // the CQD ISO_MAPPING request.
                    sprintf(buf, "CONTROL QUERY DEFAULT COMP_BOOL_58 'ON';");
                    exeImmedOneStmt(buf);
                  }

                  if (strcmp(attrName, "TERMINAL_CHARSET") == 0 &&
                      versionOfCmplrRcvCntrlInfo < COM_VERS_2300)
                  {
                    sprintf(buf, "CONTROL QUERY DEFAULT TERMINAL_CHARSET 'ISO88591';");
                  }
                  else
                  {
                    if (strcmp(attrName, "REPLICATE_IO_VERSION") == 0)
                    {
                       ULng32 originalParserFlags = Get_SqlParser_Flags(0xFFFFFFFF);
                       SQL_EXEC_SetParserFlagsForExSqlComp_Internal (originalParserFlags);
                    }
                    NAString quotedString;
                    ToQuotedString(quotedString, NAString(val), TRUE);
                    sprintf(buf, "CONTROL QUERY DEFAULT %s %s;", attrName, quotedString.data());
                  }     
                  retcode = exeImmedOneStmt(buf);       
                  if (retcode)
                    return((short)retcode);
                }
            }
        }
    }

  if (NOT sendUserCQDs && prevContext)
    {
      // Send only to different context (prevContext != NULL)
      // and send the CQDs that were entered by user and were not 'reset reset'
      for (i = 0; i < cdb->getCQDList().entries(); i++)
        {
          ControlQueryDefault * cqd = cdb->getCQDList()[i];
          NAString quotedString;
          ToQuotedString (quotedString, cqd->getValue());

          if (strcmp(cqd->getToken().data(), "TERMINAL_CHARSET") == 0 &&
              versionOfCmplrRcvCntrlInfo < COM_VERS_2300)
          {
            sprintf(buf, "CONTROL QUERY DEFAULT TERMINAL_CHARSET 'ISO88591';");
          }
          else
          {
             if (strcmp(cqd->getToken().data(), "REPLICATE_IO_VERSION") == 0)
             {
                ULng32 originalParserFlags = Get_SqlParser_Flags(0xFFFFFFFF);
                SQL_EXEC_SetParserFlagsForExSqlComp_Internal (originalParserFlags);
             }
             sprintf(buf, "CONTROL QUERY DEFAULT %s %s;",
                  cqd->getToken().data(),
                  quotedString.data());
          }
          retcode = exeImmedOneStmt(buf);       
          if (retcode)
            return((short)retcode);
        }
    }

  for (i = 0; i < cdb->getCTList().entries(); i++)
    {
      ControlTableOptions *cto = cdb->getCTList()[i];

      for (CollIndex j = 0; j < cto->numEntries(); j++)
        {
          sprintf(buf, "CONTROL TABLE %s %s '%s';",
                  cto->tableName().data(),
                  cto->getToken(j).data(),
                  cto->getValue(j).data());
          retcode = exeImmedOneStmt(buf);       
          if (retcode)
            return((short)retcode);
        } // j
    } // i


  // pass on the parserflags to the new compiler
  // used to enable SHOWPLAN for MV INTERNAL REFRESH
  if (CmpCommon::getDefault(MV_ENABLE_INTERNAL_REFRESH_SHOWPLAN) == DF_ON)
  {
     ULng32 originalParserFlags = Get_SqlParser_Flags(0xFFFFFFFF);
     SQL_EXEC_SetParserFlagsForExSqlComp_Internal (originalParserFlags);
  }

  if (sendUserCSs)
    {
      for (i = 0; i < cdb->getCSList().entries(); i++)
        {
          ControlSessionOption *cso = cdb->getCSList()[i];
          
          sprintf(buf, "CONTROL SESSION '%s' '%s';",
                  cso->getToken().data(),
                  cso->getValue().data());
          retcode = exeImmedOneStmt(buf);       
          if (retcode)
            return((short)retcode);
        }
    }

  if (CmpCommon::context()->sqlSession()->volatileSchemaInUse())
    {
      // sendAllControls is called by mxcmp when it prepares internal
      // queries. Tablenames used in these queries may already be fully
      // qualified with volatile schema name.
      // Set the flag to indicate that an error should not be returned
      // if volatile schema name has been explicitely specified.
      SQL_EXEC_SetParserFlagsForExSqlComp_Internal
        (ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);  
    }
  else
    {
      SQL_EXEC_ResetParserFlagsForExSqlComp_Internal
        (ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);  
    }

  return 0; 
}

void sendParserFlag (ULng32 flag)
{
  SQL_EXEC_SetParserFlagsForExSqlComp_Internal (flag);
}

short setParentQidAtSession(NAHeap *heap, const char *parentQid)
{
  Int32 retcode = 0;

  short len = (short)strlen(parentQid);
  char *srcBuf = new (heap) char[len+50];
  strcpy(srcBuf, "SET SESSION DEFAULT PARENT_QID '");
  strcat(srcBuf, parentQid);
  strcat(srcBuf, "'");
  retcode = exeImmedOneStmt(srcBuf);
  NADELETEBASIC(srcBuf, heap);
  return (short) retcode;
}


/*
static long describeError(long retcode)
{
  *CmpCommon::diags() << DgSqlCode(retcode);
  return -1;
}
*/

/////////////////////////////////////////////////////////////////
//
// flags:
//        0x00000001   display all expressions
//        0x00000002   display pcode expressions
//        0x00000004   display clause expressions
//        0x00000008   display TDBs
//        0x00000010   do not regenerate pcode for showplan.
//        0x00000010   do downrev compile for R2. Used when pcode is
//                     regenerated during display
//        0x00000020   do downrev compile for RR. Used when pcode is
//                     regenerated during display
/////////////////////////////////////////////////////////////////
static short CmpGetPlan(SQLSTMT_ID &stmt_id, 
                 ULng32 flags,
                        Space &space, 
                        CollHeap * heap,
                        char* &rootTdbBuf, Lng32 &rootTdbSize,
                        char* &srcStrBuf, Lng32 &srcStrSize)
{
  Lng32 retcode = 0;
  ULong stmtHandle = (ULong)stmt_id.handle;

  // Now get the generated code to describe the plan.
  // do not advance to next statement yet, if all stmts are to be retrieved.
  if ((stmt_id.handle) && (stmt_id.name_mode != stmt_handle) &&
      ((stmtHandle & 0x0001) != 0))
    stmt_id.handle = (void *)(stmtHandle & ~0x0004);
  retcode = SQLCLI_GetRootTdbSize_Internal(GetCliGlobals(),
                                           &rootTdbSize, 
                                           &srcStrSize, 
                                           &stmt_id);

  if (retcode)
    return ((retcode < 0) ? -1 : (short)retcode);

  rootTdbBuf = new (heap) char[rootTdbSize];
  srcStrBuf  = new (heap) char[srcStrSize+1];
  // now advance to the next statement
  stmtHandle = (ULong)stmt_id.handle;
  if ((stmt_id.handle) && (stmt_id.name_mode != stmt_handle) &&
      ((stmtHandle & 0x0001) != 0))
    stmt_id.handle = (void*)(stmtHandle | 0x0004);
  retcode = SQLCLI_GetRootTdb_Internal(GetCliGlobals(),
                                       rootTdbBuf, 
                                       rootTdbSize,
                                       srcStrBuf, 
                                       srcStrSize,
                                       &stmt_id);
  if (retcode)
    return ((retcode < 0) ? -1 : (short)retcode);

  if (srcStrSize > 0)
    srcStrBuf[srcStrSize] = 0;

  return (short)retcode;
}

/////////////////////////////////////////////////////////////////
//
// flags:
//        0x00000001   display all expressions
//        0x00000002   display pcode expressions
//        0x00000004   display clause expressions
//        0x00000008   display TDBs
//        0x00000010   do downrev compile. Used when pcode is regenerated
//                     during display
/////////////////////////////////////////////////////////////////
static Lng32 CmpFormatPlan(SQLSTMT_ID &stmt_id, 
                          ULng32 flags,
                          char *rootTdbBuf, Lng32 rootTdbSize,
                          char *srcStrBuf, Lng32 srcStrSize,
                          NABoolean outputSrcInfo,
                          Space &space, CollHeap * heap)
{
  char buf[200];

  if (outputSrcInfo)
    {
      // output module and statement name
      sprintf(buf, "\nModule: %s Statement: %s\n", 
              stmt_id.module->module_name, stmt_id.identifier);
      outputShortLine(space, buf);
      
      if (srcStrSize > 0)
        {
          Int32 i = 0;
          Int32 j = 0;
          while (i < srcStrSize)
            {
              if (srcStrBuf[i] == '\n')
                {
                  buf[j] = 0;
                  outputShortLine(space, buf);
                  j = 0;
                }
              else
                {
                  buf[j] = srcStrBuf[i];
                  j++;
                }
              
              i++;
            }
          
          buf[j] = 0;
          outputShortLine(space, buf);
          
          sprintf(buf, "\n");
          outputShortLine(space, buf);
        }
    }

  ComTdbRoot *rootTdb = (ComTdbRoot*) rootTdbBuf;

  // unpack the root TDB
  ComTdb dummyTdb;
  if ( (rootTdb = (ComTdbRoot *)
         rootTdb->driveUnpack((void *)rootTdb,&dummyTdb,NULL)) == NULL )
  {
    // ERROR during unpacking. Shouldn't occur here since we have just
    // freshly prepared the statement using the latest version of the
    // compiler. This is an unexpected error.
    //
    return -1;
  }

  UInt32 planVersion = rootTdb->getPlanVersion();

  // This fragment corresponds to the MASTER.
  // Get to the fragment directory from rootTdb.
  void * baseAddr = (void *)rootTdb;

  ExFragDir *fragDir = rootTdb->getFragDir();
  ComTdb *fragTdb = rootTdb;

  // a sanity check to see that it was the MASTER fragment.
  CMPASSERT(fragDir->getType(0) == ExFragDir::MASTER);

  // unpack each fragment independently;
  // unpacking converts offsets to actual pointers.
  for (Int32 i = 0; i < fragDir->getNumEntries(); i++)
    {
      void * fragBase = (void *)((char *)baseAddr + fragDir->getGlobalOffset(i));
      void * fragStart = (void *)((char *)fragBase + fragDir->getTopNodeOffset(i));

      switch (fragDir->getType(i))
        {       
        case ExFragDir::MASTER:
          {
            // already been unpacked. Nothing to be done.
            sprintf(buf, "MASTER Executor fragment");
            outputShortLine(space, buf);
            sprintf(buf, "========================\n");
            outputShortLine(space, buf);
            sprintf(buf, "Fragment ID: %d, Length: %d\n",
                    i, fragDir->getFragmentLength(i));
            outputShortLine(space, buf);
          }
          break;
        
        case ExFragDir::ESP:
          {
            sprintf(buf, "ESP fragment");
            outputShortLine(space, buf);
            sprintf(buf, "===========\n");
            outputShortLine(space, buf);
            sprintf(buf, "Fragment ID: %d, Parent ID: %d, Length: %d\n",
                    i, fragDir->getParentId(i), fragDir->getFragmentLength(i));
            outputShortLine(space, buf);
            ComTdb tdb;
            fragTdb = (ComTdb *)fragStart;
            fragTdb = (ComTdb *)(fragTdb->driveUnpack(fragBase,&tdb,NULL));
          }
          break;
        
        case ExFragDir::DP2:
          {
            sprintf(buf, "EID fragment");
            outputShortLine(space, buf);
            sprintf(buf, "===========\n");
            outputShortLine(space, buf);
          
            sprintf(buf, "Fragment ID: %d, Parent ID: %d, Length: %d\n",
                    i, fragDir->getParentId(i), fragDir->getFragmentLength(i));
            outputShortLine(space, buf);
          }
          break;
        case ExFragDir::EXPLAIN:
          // Not using explain frag here!
          fragTdb = NULL;
          break;
        } // switch

      // Now display expressions.
      flags = flags | 0x00000001;
      if (fragTdb)
        {
          if ((planVersion > COM_VERS_R1_8) &&
              (planVersion < COM_VERS_R2_1))
            {
              // downrev compile needed.
              flags |= 0x00000020;
            }
          else if (planVersion == COM_VERS_R2_1)
            flags |= 0x0000020;

          if ((CmpCommon::getDefaultLong(QUERY_OPTIMIZATION_OPTIONS) & QO_EXPR_OPT) == 0)
            {
              flags |= 0x00000020;
            }

          fragTdb->displayContents(&space, flags);
        }

    } // for

  return 0;
}

/////////////////////////////////////////////////////////////////
//
// flags:
//        0x00000001   display all expressions
//        0x00000002   display pcode expressions
//        0x00000004   display clause expressions
//        0x00000008   display TDBs
//        0x00000010   do downrev compile. Used when pcode is regenerated
//                     during display
/////////////////////////////////////////////////////////////////
static Lng32 CmpGetAndFormatPlan(SQLSTMT_ID &stmt_id, 
                         ULng32 flags,
                                NABoolean outputSrcInfo,
                                Space &space, CollHeap * heap)
{
  Lng32 retcode = 0;

  char *rootTdbBuf = NULL;
  char *srcStrBuf = NULL;
  Lng32 rootTdbSize;
  Lng32 srcStrSize;
  retcode = CmpGetPlan(stmt_id, flags, space, heap,
                       rootTdbBuf, rootTdbSize, 
                       srcStrBuf, srcStrSize);
  if (retcode)
    return ((retcode < 0) ? -1 : retcode);
  
  retcode = CmpFormatPlan(stmt_id, flags, 
                          rootTdbBuf, rootTdbSize, 
                          srcStrBuf, srcStrSize,
                          outputSrcInfo,
                          space, heap);
  if (retcode)
    return ((retcode < 0) ? -1 : retcode);

  return retcode;
}

static short CmpDescribePlan(
   const char   *query,
   ULng32 flags,
   char         *&outbuf,
   ULng32 &outbuflen,
   NAMemory      *heap)

{
  // prepare this query.
  Lng32 retcode;
  Lng32 resetRetcode;

  SQLSTMT_ID stmt_id; // added for multi charset module names
  SQLMODULE_ID module;
  SQLDESC_ID sql_src;

  Space space;
  ULng32 originalParserFlags = Get_SqlParser_Flags(0xFFFFFFFF);

  char *genCodeBuf = NULL;
  Lng32 genCodeSize = 0;
  char uniqueStmtId[ComSqlId::MAX_QUERY_ID_LEN+1];
  Lng32 uniqueStmtIdLen = ComSqlId::MAX_QUERY_ID_LEN;

  init_SQLCLI_OBJ_ID(&stmt_id);
  init_SQLMODULE_ID(&module);
  init_SQLCLI_OBJ_ID(&sql_src);

  retcode = SQL_EXEC_ClearDiagnostics(NULL);
  if (retcode)
    return ((retcode < 0) ? -1 : (short)retcode);

  //Allocate a SQL statement
  stmt_id.name_mode = stmt_handle;
  stmt_id.module = &module;
  module.module_name = 0;
  stmt_id.identifier = 0;
  stmt_id.handle = 0;
  retcode = SQL_EXEC_AllocStmt(&stmt_id, 0);
  if (retcode)
    return ((retcode < 0) ? -1 : (short)retcode);
        
  // Allocate a descriptor which will hold the SQL statement source
  sql_src.name_mode = desc_handle;
  sql_src.module    = &module;
  module.module_name = 0;
  sql_src.identifier = 0;
  sql_src.handle = 0;
  retcode = SQL_EXEC_AllocDesc(&sql_src, (SQLDESC_ID *)0);
  if (retcode)
    return ((retcode < 0) ? -1 : (short)retcode);

  retcode = SQL_EXEC_SetDescItem(&sql_src, 1, SQLDESC_CHAR_SET,
                                 SQLCHARSETCODE_UTF8
                                 , 0);
  if (retcode)
    return ((retcode < 0) ? -1 : (short)retcode);

  // The arkcmp that will be used to compile this query does not
  // know about the control statements that were issued.
  // Send all the CONTROL statements before preparing this statement.
  retcode = sendAllControls(TRUE, FALSE, TRUE);
  if (retcode)
    return (short)retcode;

  // send control session to indicate showplan is being done
  retcode = exeImmedOneStmt("CONTROL SESSION SET 'SHOWPLAN' 'ON';");
  if (retcode)
    goto label_error;

  // Now prepare the query to be 'showplan'ed
  retcode = SQL_EXEC_SetDescItem(&sql_src, 1, SQLDESC_VAR_PTR,
                                 (Long)query, 0);
  if (retcode)
  {
    goto label_error_1;
  }

  retcode = SQL_EXEC_SetDescItem(&sql_src, 1, SQLDESC_LENGTH,
                                 strlen(query) + 1, 0);
  if (retcode)
  {
    goto label_error_1;
  }

  SQL_EXEC_SetParserFlagsForExSqlComp_Internal (originalParserFlags);

  // prepare it and get the generated code size
  strcpy(uniqueStmtId, "    ");
  retcode = SQL_EXEC_Prepare2(&stmt_id, &sql_src, 
                              NULL, 0, 
                              &genCodeSize, NULL, NULL,
                              uniqueStmtId,
                              &uniqueStmtIdLen,0);
  if ((retcode == -CLI_GENCODE_BUFFER_TOO_SMALL) &&
      (genCodeSize > 0))
    {
      retcode = SQL_EXEC_ClearDiagnostics(NULL);

      // retrieve the generated code
      genCodeBuf = new (heap) char[genCodeSize];
      strcpy(uniqueStmtId, "    ");
      retcode = SQL_EXEC_Prepare2(&stmt_id, NULL, 
                                  genCodeBuf, genCodeSize, 
                                  NULL, NULL, NULL,
                                  uniqueStmtId,
                                  &uniqueStmtIdLen,0);
    }
  
  if (retcode < 0) // ignore warnings
  {
    resetRetcode = SQL_EXEC_MergeDiagnostics_Internal(*CmpCommon::diags());

    goto label_error_1;
  }

  retcode = CmpFormatPlan(stmt_id, flags, 
                          genCodeBuf, genCodeSize, 
                          NULL, 0,
                          FALSE,
                          space, heap);
  if (retcode)
  {
    goto label_error_1;
  }

  outbuflen = space.getAllocatedSpaceSize();
  outbuf = new (heap) char[outbuflen];
  space.makeContiguous(outbuf, outbuflen);

  retcode = exeImmedOneStmt("CONTROL SESSION RESET 'SHOWPLAN';");
  if (retcode)
    goto label_error;
        
   // free up resources
  retcode = SQL_EXEC_DeallocDesc(&sql_src);
  if (retcode)
    return ((retcode < 0) ? -1 : (short)retcode);

  retcode = SQL_EXEC_DeallocStmt(&stmt_id);
  if (retcode)
    return ((retcode < 0) ? -1 : (short)retcode);

  return 0;

 label_error_1:
    resetRetcode = exeImmedOneStmt("CONTROL SESSION RESET 'SHOWPLAN';");

 label_error:
    return ((retcode < 0) ? -1 : (short)retcode);
} // CmpDescribePlan


// TRUE if ident is nonempty and contained in comparand string c1.
inline static NABoolean identSpecifiedMatches(const NAString &id, const char *c1)
{ return (!id.isNull() && strstr(c1, id)); }    // strstr(fullstring, substring)

// TRUE if ident is empty (not specified), or is contained in c1.
inline static NABoolean identMatches(const NAString &id, const char *c1)
{ return (id.isNull() || strstr(c1, id)); }     // strstr(fullstring, substring)

// If not exact match:
//   TRUE if ident is empty, or is contained in c1, or c1 equals c2.
// If exact:
//   TRUE if ident equals c1, period.
inline static NABoolean identMatches2(const NAString &id, const char *c1, const char *c2,
                            NABoolean exactMatch)
{ 
  return exactMatch ? (strcmp(id, c1) == 0)
                    : (identMatches(id, c1) || strcmp(c1, c2) == 0);
}

#define IDENTMATCHES(c1,c2)     identMatches2(ident, c1, c2, exactMatch)

// defined in generator/GenExplain.cpp
NABoolean displayDuringRegressRun(DefaultConstants attr);

// these CQDs are used for internal purpose and are set/sent by SQL.
// Do not display them.
static NABoolean isInternalCQD(DefaultConstants attr)
{
  if ((attr == SESSION_ID) ||
      (attr == SESSION_IN_USE) ||
      (attr == LDAP_USERNAME) ||
      (attr == VOLATILE_CATALOG) ||
      (attr == VOLATILE_SCHEMA_IN_USE) ||
      (attr == IS_SQLCI) ||
      (attr == NVCI_PROCESS))
    return TRUE;
  
  if ((getenv("SQLMX_REGRESS")) &&
      ((NOT displayDuringRegressRun(attr)) ||
       (attr == TRAF_ALIGNED_ROW_FORMAT)))
    return TRUE;
  
  return FALSE;
}
short CmpDescribeControl(Describe *d,
                         char *&outbuf,
                         ULng32 &outbuflen,
                         CollHeap *heap)
{
  // Two kludgy passing mechanisms from SqlParser.y ...
  NAString ident =
    d->getDescribedTableName().getQualifiedNameObj().getObjectName();
  NABoolean exactMatch =
    !d->getDescribedTableName().getCorrNameAsString().isNull();
  CMPASSERT(!(ident.isNull() && exactMatch));

  NAString fmtdIdent;
  if (!ident.isNull()) fmtdIdent = NAString(' ') + ident;
  static const char fmtNoMatch []   = "No CONTROL %s%s settings are in effect.";
  static const char fmtNoMatch2[]   = "No DEFAULTS attribute%s exists.";
  static const char fmtNoMatch3[]   = "No externalized DEFAULTS attribute%s exists.";

//static const char fmtExactMatch[] = "\n%-32s\t%s";
  static const char fmtFuzzyMatch[] = "  %-30s\t%s";
//const char *fmtInfo = exactMatch ? fmtExactMatch : fmtFuzzyMatch;
  const char *fmtInfo = fmtFuzzyMatch;

  // SHOWCONTROL DEFAULT CAT will display both CATALOG and SCHEMA defaults;
  // SHOWCONTROL DEFAULT SCH will too.
  static const char *defCatSchStrings[] = { "", "CATALOG", "SCHEMA" };
  size_t ix = identSpecifiedMatches(ident, "CATALOG") ? 2 :
              identSpecifiedMatches(ident, "SCHEMA")  ? 1 : 0;
  const char *defCatSch = defCatSchStrings[ix];

  NABoolean showShape, showSession, showTable, showDefault, match = FALSE;
  NABoolean showAll = d->getFormat() == Describe::CONTROL_ALL_;

  if (showAll)
    {
      showShape   = TRUE;
      showSession = TRUE;
      showTable   = TRUE;
      showDefault = TRUE;
    }
  else
    {
      showShape   = d->getFormat() == Describe::CONTROL_SHAPE_;
      showSession = d->getFormat() == Describe::CONTROL_SESSION_;
      showTable   = d->getFormat() == Describe::CONTROL_TABLE_;
      showDefault = d->getFormat() == Describe::CONTROL_DEFAULTS_;
    }

  Space space;
  char * buf = new (heap) char[15000];
  CMPASSERT(buf);

  ControlDB *cdb = ActiveControlDB();

  if (showShape)
    {
      outputShortLine(space, "");
      if ((cdb->getRequiredShape()) &&
          (cdb->getRequiredShape()->getShape()))
        {
          NAString shape(cdb->getRequiredShape()->getShapeText());
          PrettifySqlText(shape);
          outputLine(space, shape, 2);
        }
      else
        {
          // Only one shape at a time (no history-list), so no ident-matching...
          sprintf(buf, fmtNoMatch, "QUERY SHAPE", "");
          outputLine(space, buf, 0);
        }
    } // SHOWCONTROL SHAPE

  if (showSession)
    {
      outputShortLine(space, "");
      match = FALSE;
      for (CollIndex i = 0; i < cdb->getCSList().entries(); i++)
        {
          ControlSessionOption *cso = cdb->getCSList()[i];
          if (IDENTMATCHES(cso->getToken().data(), "*"))
            {
              if (!match)
                outputShortLine(space, "CONTROL SESSION");
              sprintf(buf, fmtInfo,
                cso->getToken().data(),
                cso->getValue().data());
              outputLine(space, buf, 2);
              match = TRUE;
            }
        }
      if (!match)
        {
          sprintf(buf, fmtNoMatch, "SESSION", fmtdIdent.data());
          outputLine(space, buf, 0);
        }
    } // SHOWCONTROL SESSION

  if (showTable)
    {
      outputShortLine(space, "");
      match = FALSE;
      for (CollIndex i = 0; i < cdb->getCTList().entries(); i++)
        {
          ControlTableOptions *cto = cdb->getCTList()[i];
          if (IDENTMATCHES(cto->tableName().data(), "*") &&
              cto->numEntries() > 0)
            {
              sprintf(buf, "CONTROL TABLE %s", cto->tableName().data());
              outputLine(space, buf, 0);
              match = TRUE;
              for (CollIndex j = 0; j < cto->numEntries(); j++)
                {
                  sprintf(buf, fmtInfo,
                    cto->getToken(j).data(),
                    cto->getValue(j).data());
                  outputLine(space, buf, 2);
                } // j
            } // ident absent or it matches
        } // i
      if (!match)
        {
          sprintf(buf, fmtNoMatch, "TABLE", fmtdIdent.data());
          outputLine(space, buf, 0);
        }
    } // SHOWCONTROL TABLE

  if (showDefault)
    {
        {
          if (d->getHeader())
            outputShortLine(space, "");
          match = FALSE;
          for (CollIndex i = 0; i < cdb->getCQDList().entries(); i++)
            {
              ControlQueryDefault * cqd = cdb->getCQDList()[i];
              if (IDENTMATCHES(cqd->getToken().data(), defCatSch))
                {
                  // This is the NO HEADER option. We just want the value of the default.
                  if (!(d->getHeader())) {
                    strcpy(buf, cqd->getValue().data());
                    outputLine(space, buf, 2);
                    outbuflen = space.getAllocatedSpaceSize();
                    outbuf = new (heap) char[outbuflen];
                    space.makeContiguous(outbuf, outbuflen);
                    NADELETEBASIC(buf, heap);
                    return 0;
                  }  
                  else {
                    if (NOT isInternalCQD(cqd->getAttrEnum()))
                      {
                        if (!match)
                          outputShortLine(space, "CONTROL QUERY DEFAULT");
                        sprintf(buf, fmtInfo,
                                cqd->getToken().data(),
                                cqd->getValue().data());
                        outputLine(space, buf, 2);
                        match = TRUE;
                      }
                  }
                }
            }
          if (!match && d->getHeader())
            {
              sprintf(buf, fmtNoMatch, "QUERY DEFAULT", fmtdIdent.data());
              outputLine(space, buf, 0);
            }
        }

      // This is a nice little extra for partial-match, and
      // essential for exactMatch:
      if (!ident.isNull()) showAll = TRUE;

    } // SHOWCONTROL DEFAULTS

  if (showAll)
    {
      if (d->getHeader())
        outputShortLine(space, "");
      match = FALSE;
      NADefaults &defs = CmpCommon::context()->schemaDB_->getDefaults();

      for (CollIndex i = 0; i < defs.numDefaultAttributes(); i++)
        {
          const char *attrName, *val;
          if (defs.getCurrentDefaultsAttrNameAndValue(i, attrName, val, FALSE) &&
              IDENTMATCHES(attrName, defCatSch)) 
            {
              // This is the NO HEADER option. We just want the value of the default.
              if (!(d->getHeader())) {
                strcpy(buf, val);
                outputLine(space, buf, 2);
                outbuflen = space.getAllocatedSpaceSize();
                outbuf = new (heap) char[outbuflen];
                space.makeContiguous(outbuf, outbuflen);
                NADELETEBASIC(buf, heap);
                return 0;
              } 
              else {  
                if (!match)
                  outputShortLine(space, "Current DEFAULTS");
                sprintf(buf, fmtInfo, attrName, val);
                outputLine(space, buf, 2);
                match = TRUE;
              }
            }
        }
      if (!match)
        {
          if (CmpCommon::getDefault(SHOWCONTROL_SHOW_ALL) != DF_ON)
            sprintf(buf, fmtNoMatch3, fmtdIdent.data());
          else
            sprintf(buf, fmtNoMatch2, fmtdIdent.data());
          outputLine(space, buf, 0);
        }
    }

  outbuflen = space.getAllocatedSpaceSize();
  outbuf = new (heap) char[outbuflen];
  space.makeContiguous(outbuf, outbuflen);

  NADELETEBASIC(buf, heap);

  return 0;
}


static short CmpDescribeShape(
   const char    *query,
   char         *&outbuf,
   ULng32 &outbuflen,
   NAMemory      *heap)

{

  // prepare this query.
  char * qTree = NULL;
  ULng32 dummyLen;

  CmpMain sqlcomp;

  CmpMain::CompilerPhase phase = CmpMain::PRECODEGEN;
#ifdef _DEBUG
  char * sp = getenv("SHOWSHAPE_PHASE");
  if (sp)
    {
      switch (sp[0])
        {
        case 'S':
          phase = CmpMain::PARSE;
          break;
          
        case 'B':
          phase = CmpMain::BIND;
          break;
          
        case 'T':
          phase = CmpMain::TRANSFORM;
          break;
          
        case 'N':
          phase = CmpMain::NORMALIZE;
          break;
          
        case 'O':
          phase = CmpMain::OPTIMIZE;
          break;
          
        case 'P':
          phase = CmpMain::PRECODEGEN;
          break;
          
        case 'G':
          phase = CmpMain::GENERATOR;
          break;
          
        default:
          phase = CmpMain::PRECODEGEN;
          break;
        } // switch
    } // if
#endif

  QueryText qText((char*)query, SQLCHARSETCODE_UTF8);
  if (sqlcomp.sqlcomp(qText, 0, &qTree, &dummyLen, heap, phase))
    {
      CMPASSERT(query);
      return -1;
    }

  RelExpr * queryTree = (RelExpr *)qTree;

  Space space;
  char buf[1000];

  sprintf(buf, "control query shape ");

  queryTree->generateShape(&space, buf);

  strcat(buf, ";");
  outputShortLine(space, buf);

  outbuflen = space.getAllocatedSpaceSize();
  outbuf = new (heap) char[outbuflen];
  space.makeContiguous(outbuf, outbuflen);
  return 0;
}

// show transaction
static short CmpDescribeTransaction(
   char         *&outbuf,
   ULng32 &outbuflen,
   NAMemory      *heap)

{

  Space space;
  char buf[100];

  if (CmpCommon::transMode()->getIsolationLevel() == TransMode::READ_UNCOMMITTED_)
    outputShortLine(space, "ISOLATION LEVEL  : READ UNCOMMITTED");
  else if (CmpCommon::transMode()->getIsolationLevel() == TransMode::READ_COMMITTED_)
    outputShortLine(space, "ISOLATION LEVEL  : READ COMMITTED");
  else if (CmpCommon::transMode()->getIsolationLevel() == TransMode::REPEATABLE_READ_)
    outputShortLine(space, "ISOLATION LEVEL  : REPEATABLE READ");
  else if (CmpCommon::transMode()->getIsolationLevel() == TransMode::SERIALIZABLE_)
    outputShortLine(space, "ISOLATION LEVEL  : SERIALIZABLE");
  else if (CmpCommon::transMode()->getIsolationLevel() == TransMode::IL_NOT_SPECIFIED_)
    outputShortLine(space, "ISOLATION LEVEL  : NOT SPECIFIED");

  if (CmpCommon::transMode()->getAccessMode() == TransMode::READ_ONLY_ ||
        CmpCommon::transMode()->getAccessMode() == TransMode::READ_ONLY_SPECIFIED_BY_USER_)
    outputShortLine(space, "ACCESS MODE      : READ ONLY");
  else if (CmpCommon::transMode()->getAccessMode() == TransMode::READ_WRITE_)
    outputShortLine(space, "ACCESS MODE      : READ WRITE");
  else if (CmpCommon::transMode()->getAccessMode() == TransMode::AM_NOT_SPECIFIED_)
    outputShortLine(space, "ACCESS MODE      : NOT SPECIFIED");

  if (CmpCommon::transMode()->getAutoCommit() == TransMode::ON_)
    outputShortLine(space, "AUTOCOMMIT       : ON");
  else if (CmpCommon::transMode()->getAutoCommit() == TransMode::OFF_)
    outputShortLine(space, "AUTOCOMMIT       : OFF");
  else if (CmpCommon::transMode()->getAutoCommit() == TransMode::AC_NOT_SPECIFIED_)
    outputShortLine(space, "AUTOCOMMIT       : NOT SPECIFIED");


  if (CmpCommon::transMode()->getRollbackMode() == TransMode::NO_ROLLBACK_)
    outputShortLine(space, "NO ROLLBACK      : ON");
  else if (CmpCommon::transMode()->getRollbackMode() == TransMode::ROLLBACK_MODE_WAITED_)
    outputShortLine(space, "NO ROLLBACK      : OFF");
  else 
    outputShortLine(space, "NO ROLLBACK      : NOT SPECIFIED");

  sprintf(buf,             "DIAGNOSTICS SIZE : %d", CmpCommon::transMode()->getDiagAreaSize());
  outputLine(space, buf, 0);

  Lng32 val = CmpCommon::transMode()->getAutoAbortIntervalInSeconds() ;
  if (val == -2)  // user specified never abort setting
    val = 0;
  if (val == -3)  // user specified reset to TMFCOM setting
    val = -1;
  sprintf(buf,             "AUTOABORT        : %d SECONDS", val);
  outputLine(space, buf, 0);

  if (CmpCommon::transMode()->getMultiCommit() == TransMode::MC_ON_)
    outputShortLine(space, "MULTI COMMIT     : ON");
  else if (CmpCommon::transMode()->getMultiCommit() == TransMode::MC_OFF_)
    outputShortLine(space, "MULTI COMMIT     : OFF");
  else if (CmpCommon::transMode()->getMultiCommit() == TransMode::MC_NOT_SPECIFIED_)
    outputShortLine(space, "MULTI COMMIT     : NOT SPECIFIED");

  if (CmpCommon::transMode()->getAutoBeginOn())
    outputShortLine(space, "AUTOBEGIN        : ON");
  else if (CmpCommon::transMode()->getAutoBeginOff())
    outputShortLine(space, "AUTOBEGIN        : OFF");
  else
    outputShortLine(space, "AUTOBEGIN        : NOT SPECIFIED");
  
  outbuflen = space.getAllocatedSpaceSize();
  outbuf = new (heap) char[outbuflen];
  space.makeContiguous(outbuf, outbuflen);

  return 0;
}

static NAString CmpDescribe_ptiToInfCS(const NAString &inputInLatin1)
{
  return Latin1StrToUTF8 ( inputInLatin1 // in - const NAString & latin1Str
                         , STMTHEAP      // in - NAMemory * heap
                         );
}


short CmpDescribeHiveTable ( 
                             const CorrName  &dtName,
                             short type, // 1, invoke. 2, showddl. 3, createLike
                             char* &outbuf,
                             ULng32 &outbuflen,
                             CollHeap *heap,
                             NABoolean isDetail,
                             UInt32 columnLengthLimit)
{
  const NAString& tableName =
    dtName.getQualifiedNameObj().getQualifiedNameAsString();
 
  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  NATable *naTable = bindWA.getNATable((CorrName&)dtName); 
  if (naTable == NULL || bindWA.errStatus())
    return -1;
  else
    {
      // if showddl and this hive table has an associated external table,
      // show the ddl of underlying hive table first.
      if ((type == 2) &&
          (naTable->isHiveTable()) &&
          (naTable->hasHiveExtTable()))
        {
          // remove current cache key and turn off ext table attr cqd.
          // This will return the underlying hive natable.
          bindWA.getSchemaDB()->getNATableDB()->remove(naTable->getKey());

          // retrieve underlying hive table definition
          bindWA.setReturnHiveTableDefn(TRUE);
          naTable = bindWA.getNATable((CorrName&)dtName); 
          if (naTable == NULL || bindWA.errStatus())
            return -1;
        }

      bindWA.createTableDesc(naTable, (CorrName&)dtName);
      if (bindWA.errStatus())
        return -1;
    }

  if (NOT naTable->isHiveTable())
    return -1;

  char * buf = new (heap) char[15000];
  CMPASSERT(buf);

  time_t tp;
  time(&tp);
  
  Space space;

  // emit an initial newline
  outputShortLine(space, " ");

  if (!CmpDescribeIsAuthorized(SQLOperation::UNKNOWN,
                               naTable->getPrivInfo(),
                               COM_BASE_TABLE_OBJECT))
    return -1;


  NABoolean isView = (naTable->getViewText() ? TRUE : FALSE);

  if ((type == 2) && (isView))
    {
      outputShortLine(space,"Original native Hive view text:");
      NAString origViewText(naTable->getHiveOriginalViewText());
      outputLongLine(space, origViewText, 0);
      outputShortLine(space, " ");

      outputShortLine(space,"Expanded native Hive view text:");
      NAString viewtext(naTable->getViewText());

      viewtext = viewtext.strip(NAString::trailing, ';');
      viewtext += " ;";

      outputLongLine(space, viewtext, 0);

      // if this hive view is registered in traf metadata, show that.
      if (naTable->isRegistered())
        {
          Int64 objectUID = (Int64)naTable->objectUid().get_value();

          outputShortLine(space, " ");

          sprintf(buf,  "REGISTER%sHIVE VIEW %s;",
                  (naTable->isInternalRegistered() ? " /*INTERNAL*/ " : " "),
                  naTable->getTableName().getQualifiedNameAsString().data());
          NAString bufnas(buf);
          outputLongLine(space, bufnas, 0);

          str_sprintf(buf, "/* ObjectUID = %ld */", objectUID);
          outputShortLine(space, buf);
        }

      outbuflen = space.getAllocatedSpaceSize();
      outbuf = new (heap) char[outbuflen];
      space.makeContiguous(outbuf, outbuflen);
      
      NADELETEBASIC(buf, heap);
      
      return 0;
    }

  if (type == 1)
    {
      if (isView)
        sprintf(buf, "-- Definition of native Hive view %s\n"
                "-- Definition current  %s",
                tableName.data(), ctime(&tp));
      else
        sprintf(buf, "-- Definition of hive table %s\n"
                "-- Definition current  %s",
                tableName.data(), ctime(&tp));
      
      outputShortLine(space, buf);
    }
  else if (type == 2)
    {
      outputShortLine(space,"/* Hive DDL */");

      if (naTable->isHiveExternalTable())
        sprintf(buf,  "CREATE EXTERNAL TABLE %s",
                tableName.data());
      else
        sprintf(buf,  "CREATE TABLE %s",
                tableName.data());
            
      outputShortLine(space, buf);
    }
  
  outputShortLine(space, "  ( ");

  for (Int32 i = 0; i < (Int32)naTable->getColumnCount(); i++)
    {
      NAColumn * nac = naTable->getNAColumnArray()[i];

      const NAString &colName = nac->getColName();
      const NAType * nat = nac->getType();

      sprintf(buf, "%-*s ", CM_SIM_NAME_LEN,
              ANSI_ID(colName.data()));
      
      NAString nas;
      if ((type == 1) || (type == 3))
        nat->getMyTypeAsText(&nas, FALSE);
      else
        nat->getMyTypeAsHiveText(&nas);
      
      // if it is a character type and it is longer than the length
      // limit in bytes, then shorten the target type
      if ((type == 3) &&
          (nat->getTypeQualifier() == NA_CHARACTER_TYPE) &&
          (!nat->isLob()) &&
          (columnLengthLimit < UINT_MAX))
        {
          const CharType * natc = (const CharType *)nat;
          if (natc->getDataStorageSize() > columnLengthLimit)
            {
              CharType * newType = (CharType *)natc->newCopy(NULL);
              newType->setDataStorageSize(columnLengthLimit);
              nas.clear();
              newType->getMyTypeAsText(&nas, FALSE);
              delete newType;
            }
        }

      sprintf(&buf[strlen(buf)], "%s", nas.data());

      NAString colString(buf);
      Int32 j = i;
      outputColumnLine(space, colString, j);
  }

  outputShortLine(space, "  )");

  // show hive table partitions and buckets definition.
  // this default database of hive which is mapped as schema HIVE in Trafodion
  HiveMetaData* md = bindWA.getSchemaDB()->getNATableDB()->getHiveMetaDB();
  NAString defSchema = ActiveSchemaDB()->getDefaults().getValue(HIVE_DEFAULT_SCHEMA);
  defSchema.toUpper();
  NAString tableNameInt = dtName.getQualifiedNameObj().getObjectName();
  NAString schemaNameInt = dtName.getQualifiedNameObj().getSchemaName();
  if (dtName.getQualifiedNameObj().getUnqualifiedSchemaNameAsAnsiString() == defSchema)
     schemaNameInt = md->getDefaultSchemaName();
   // Hive stores names in lower case
   // Right now, just downshift, could check for mixed case delimited
   // identifiers at a later point, or wait until Hive supports delimited identifiers
  schemaNameInt.toLower();
  tableNameInt.toLower();
  hive_tbl_desc* htd = md->getTableDesc(schemaNameInt, tableNameInt, FALSE,
                // reread Hive Table Desc from MD.
                (CmpCommon::getDefault(TRAF_RELOAD_NATABLE_CACHE) == DF_ON),
                TRUE);
  if( htd && htd->getPartKey() && type == 2)
  {
       NAString partStr = "  PARTITIONED BY (";
       NAString colNameUpper;
       for(hive_pkey_desc* hpd = htd->getPartKey(); hpd; hpd = hpd->next_)
       {
           colNameUpper = hpd->name_;
           colNameUpper.toUpper();
           partStr += colNameUpper;
           partStr += ' ';
           partStr += hpd->type_;
           partStr += ",";
       }
       partStr[partStr.length()-1] = ')';
       outputShortLine(space, partStr);
  }
  if(htd && htd->getBucketingKeys() && type == 2)
  {
       NAString bktStr = "  CLUSTERED BY (";
       for(hive_bkey_desc* hbd = htd->getBucketingKeys(); hbd; hbd = hbd->next_)
       {
           bktStr += hbd->name_;
           bktStr += ",";
       }
       bktStr[bktStr.length()-1] = ')';
       outputShortLine(space, bktStr);

       hive_skey_desc* sortkeyHeader = htd->getSortKeys();
       if(sortkeyHeader)
       {
           NAString skStr = "  SORTED BY (";
           for(hive_skey_desc* hsd = sortkeyHeader; hsd; hsd = hsd->next_)
           {
               skStr += hsd->name_;
               if (hsd->orderInt_ == 0)
                 skStr += " DESC";
               skStr += ",";
           }
           skStr[skStr.length()-1] = ')';
           outputShortLine(space, skStr);
       }

       bktStr = "  INTO ";
       bktStr += std::to_string((long long)(htd->sd_->buckets_)).c_str();
       bktStr += " BUCKETS ";
       outputShortLine(space, bktStr);
  }
  if( htd && htd->getPartKey() && type == 3 && 
      (CmpCommon::getDefault(HIVE_CREATE_TABLE_LIKE_PARTITION_NO_NULL) 
       == DF_ON))
  {
       NAString storeStr = "  STORE BY (";
       NAString colNameUpper;
       for(hive_pkey_desc* hpd = htd->getPartKey(); hpd; hpd = hpd->next_)
       {
           colNameUpper = hpd->name_;
           colNameUpper.toUpper();
           storeStr += colNameUpper;
           storeStr += ",";
       }
       storeStr[storeStr.length()-1] = ')';
       outputShortLine(space, storeStr);
       if (gpClusterInfo->numOfSMPs() > 1)
       {
	 NAString saltStr = "  SALT USING ";
	 char numNodes[20];
	 str_itoa(gpClusterInfo->numOfSMPs(), numNodes);
	 saltStr += numNodes;
	 saltStr += " PARTITIONS ";
	 outputShortLine(space, saltStr);
       }
  }
  if( htd && htd->getPartKey() && type == 3 && 
      (CmpCommon::getDefault(HIVE_CREATE_TABLE_LIKE_PARTITION_NO_NULL)
       == DF_ON))
  {
       NAString storeStr = "  STORE BY (";
       NAString colNameUpper;
       for(hive_pkey_desc* hpd = htd->getPartKey(); hpd; hpd = hpd->next_)
       {
           colNameUpper = hpd->name_;
           colNameUpper.toUpper();
           storeStr += colNameUpper;
           storeStr += ",";
       }
       storeStr[storeStr.length()-1] = ')';
       outputShortLine(space, storeStr);
  }
  if( htd && htd->getPartKey() && type == 3 && 
      (CmpCommon::getDefault(HIVE_CREATE_TABLE_LIKE_PARTITION_NO_NULL)
       == DF_ON))
  {
       NAString storeStr = "  STORE BY (";
       NAString colNameUpper;
       for(hive_pkey_desc* hpd = htd->getPartKey(); hpd; hpd = hpd->next_)
       {
           colNameUpper = hpd->name_;
           colNameUpper.toUpper();
           storeStr += colNameUpper;
           storeStr += ",";
       }
       storeStr[storeStr.length()-1] = ')';
       outputShortLine(space, storeStr);
  }

  const HHDFSTableStats* hTabStats = NULL;
  if (naTable->getClusteringIndex())
    hTabStats = naTable->getClusteringIndex()->getHHDFSTableStats();
  if (hTabStats)
    {
      if (hTabStats->isOrcFile())
        {
          if (type == 1)
            outputShortLine(space, "  /* stored as orc */");
          else if (type == 2)
            outputShortLine(space, "  stored as orc ");
        }
      else if (hTabStats->isTextFile())
        {
          if (type == 1)
            outputShortLine(space, "  /* stored as textfile */");
          else if (type == 2)
            outputShortLine(space, "  stored as textfile ");
        }
      else if (hTabStats->isSequenceFile())
        {
          if (type == 1)
            outputShortLine(space, "  /* stored as sequencefile */");
          else if (type == 2)
            outputShortLine(space, "  stored as sequencefile ");
        }
    }

  if (type == 2)
    {
      outputShortLine(space, ";");

      outputShortLine(space," ");
      outputShortLine(space,"/* Trafodion DDL */");
    }

  // if this hive table is registered in traf metadata, show that.
  if (type == 2)
    {
      if (naTable->isRegistered())
        {
          Int64 objectUID = (Int64)naTable->objectUid().get_value();
          
          outputShortLine(space, " ");
          
          sprintf(buf,  "REGISTER%sHIVE %s %s;",
                  (naTable->isInternalRegistered() ? " /*INTERNAL*/ " : " "),
                  (isView ? "VIEW" : "TABLE"),
                  naTable->getTableName().getQualifiedNameAsString().data());
          
          NAString bufnas(buf);
          outputLongLine(space, bufnas, 0);
          
          str_sprintf(buf, "/* ObjectUID = %ld */", objectUID);
          outputShortLine(space, buf);
        }
      else if (isDetail)
        {
          // show a comment that this object should be registered
          outputShortLine(space, " ");

          outputShortLine(space, "-- Object is not registered in Trafodion Metadata.");
          outputShortLine(space, "-- Register it using the next command:");
          sprintf(buf, "--   REGISTER HIVE %s %s;",
                  (isView ? "VIEW" : "TABLE"),
                  naTable->getTableName().getQualifiedNameAsAnsiString().data());
          NAString bufnas(buf);
          outputLongLine(space, bufnas, 0);
        }
    }

  // if this hive table has an associated external table, show ddl
  // for that external table.
  if ((type == 2) &&
      (bindWA.returnHiveTableDefn()))
    {
      // remove table key from natable cache. Next call to get natable
      // will get the external table defn, if one exists
      bindWA.getSchemaDB()->getNATableDB()->remove(naTable->getKey());

      bindWA.setReturnHiveTableDefn(FALSE);

      char * dummyBuf;
      ULng32 dummyLen;
      
      NAString extName = ComConvertNativeNameToTrafName(
           dtName.getQualifiedNameObj().getCatalogName(),
           dtName.getQualifiedNameObj().getSchemaName(),
           dtName.getQualifiedNameObj().getObjectName());
      
      QualifiedName qn(extName, 3);
      CorrName cn(qn);

      short rc = CmpDescribeSeabaseTable(cn, 
                                         type,
                                         dummyBuf, dummyLen, heap, 
                                         NULL, NULL,
                                         TRUE, FALSE, FALSE, FALSE, 
                                         FALSE,
                                         UINT_MAX, TRUE,
                                         NULL, 0, NULL, NULL, &space);

      outputShortLine(space, ";");
    }

  // If SHOWDDL and authorization is enabled, display GRANTS
  if (type == 2)
  {
    int64_t objectUID = (int64_t)naTable->objectUid().get_value();

    char * sqlmxRegr = getenv("SQLMX_REGRESS");
    NABoolean displayPrivilegeGrants = TRUE;
    if (((CmpCommon::getDefault(SHOWDDL_DISPLAY_PRIVILEGE_GRANTS) == DF_SYSTEM) && sqlmxRegr) ||
        (CmpCommon::getDefault(SHOWDDL_DISPLAY_PRIVILEGE_GRANTS) == DF_OFF) ||
        (NOT CmpCommon::context()->isAuthorizationEnabled()) ||
        (objectUID <= 0))
      displayPrivilegeGrants = FALSE;

    if (displayPrivilegeGrants)
    {
      // Used for context switches
      CmpSeabaseDDL cmpSBD((NAHeap*)heap);

      // now get the grant stmts
      std::string privMDLoc(ActiveSchemaDB()->getDefaults().getValue(SEABASE_CATALOG));
      privMDLoc += std::string(".\"") + 
                   std::string(SEABASE_PRIVMGR_SCHEMA) + 
                   std::string("\"");
      PrivMgrCommands privInterface(privMDLoc, CmpCommon::diags(),
                                    PrivMgr::PRIV_INITIALIZED);
      PrivMgrObjectInfo objectInfo(naTable);
      std::string privilegeText;
      if (cmpSBD.switchCompiler())
      {
        *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);
        return -1;
      }
 
      if (privInterface.describePrivileges(objectInfo, privilegeText))
      {
        outputShortLine(space, " ");
        outputLine(space, privilegeText.c_str(), 0);
      }

      cmpSBD.switchBackCompiler();
    }
  }

  outbuflen = space.getAllocatedSpaceSize();
  outbuf = new (heap) char[outbuflen];
  space.makeContiguous(outbuf, outbuflen);
  
  NADELETEBASIC(buf, heap);

  return 0;
}

// type:  1, invoke. 2, showddl. 3, create_like
short cmpDisplayColumn(const NAColumn *nac,
                       char * inColName,
                       const NAType *inNAT,
                       short displayType,
                       Space *inSpace,
                       char * buf,
                       Lng32 &ii,
                       NABoolean namesOnly,
                       NABoolean &identityCol,
                       NABoolean isExternalTable,
                       NABoolean isAlignedRowFormat,
                       UInt32 columnLengthLimit,
                       NAList<const NAColumn *> * truncatedColumnList)
{
  Space lSpace;
  
  Space * space;
  if (inSpace)
    space = inSpace;
  else
    space = &lSpace;

  identityCol = FALSE;
  
  NAString colName(inColName ? inColName : nac->getColName());
  NATable * naTable = (NATable*)nac->getNATable();

  NAString colFam;
  if ((nac->getNATable() && nac->getNATable()->isSQLMXAlignedTable()) || 
      (nac->getHbaseColFam() == SEABASE_DEFAULT_COL_FAMILY) ||
      isExternalTable)
    colFam = "";
  else if (nac->getNATable() && nac->getNATable()->isSeabaseTable())
    {
      const char * col_fam = NULL;
      if (nac->getNATable()->isHbaseMapTable())
        {
          col_fam = nac->getHbaseColFam().data();
        }
      else
        {
          int index = 0;
          CmpSeabaseDDL::extractTrafColFam(nac->getHbaseColFam(), index);
          
          if (index >= naTable->allColFams().entries())
            return -1;

          col_fam = naTable->allColFams()[index].data();
        }
      
      colFam = ANSI_ID(col_fam);

      colFam += ".";
    }

  if (displayType == 3)
    {
      NAString quotedColName = "\"";
      quotedColName += colName.data(); 
      quotedColName += "\"";
      sprintf(buf, "%s%-*s ", 
              colFam.data(),
              CM_SIM_NAME_LEN,
              quotedColName.data());
    }
  else
    {
      sprintf(buf, "%s%-*s ", 
              colFam.data(),
              CM_SIM_NAME_LEN-(int)colFam.length(),
              ANSI_ID(colName.data()));
    }
  
  if (namesOnly)
    {
      NAString colString(buf);
      Int32 j = ii;
      outputColumnLine(*space, colString, j);
      
      return 0;
    }
  
  const NAType * nat = (inNAT ? inNAT : nac->getType());
  
  NAString nas;
  nat->getMyTypeAsText(&nas, FALSE);

  // if it is a character type and it is longer than the length
  // limit in bytes, then shorten the target type
  
  if ((nat->getTypeQualifier() == NA_CHARACTER_TYPE) &&
      (!nat->isLob()) &&
      (columnLengthLimit < UINT_MAX) &&
      (truncatedColumnList))
    {
      const CharType * natc = (const CharType *)nat;
      if (natc->getDataStorageSize() > columnLengthLimit)
        {
          CharType * newType = (CharType *)natc->newCopy(NULL);
          newType->setDataStorageSize(columnLengthLimit);
          nas.clear();
          newType->getMyTypeAsText(&nas, FALSE);
          delete newType;
          truncatedColumnList->insert(nac);
        }
    }
  
  NAString attrStr;
  
  NAString defVal;
  if (nac->getDefaultClass() == COM_NO_DEFAULT)
    defVal = "NO DEFAULT";
  else if (nac->getDefaultClass() == COM_NULL_DEFAULT)
    defVal = "DEFAULT NULL";
  else if (nac->getDefaultClass() == COM_CURRENT_DEFAULT)
    defVal = "DEFAULT CURRENT";
  else if (nac->getDefaultClass() == COM_CURRENT_UT_DEFAULT)
    defVal = "DEFAULT CURRENT UNIXTIME";
  else if (nac->getDefaultClass() == COM_USER_FUNCTION_DEFAULT)
    defVal = "DEFAULT USER";
  else if (nac->getDefaultClass() == COM_UUID_DEFAULT)
    defVal = "DEFAULT UUID";
  else if (nac->getDefaultClass() == COM_USER_DEFINED_DEFAULT || nac->getDefaultClass() == COM_FUNCTION_DEFINED_DEFAULT)
    {
      defVal = "DEFAULT ";
      
      if (displayDefaultValue(nac->getDefaultValue(), colName.data(),
                              defVal))
        {
          return -1;
        }
      
    }
  else if ((nac->getDefaultClass() == COM_IDENTITY_GENERATED_BY_DEFAULT) ||
           (nac->getDefaultClass() == COM_IDENTITY_GENERATED_ALWAYS))
    {
      if (nac->getDefaultClass() == COM_IDENTITY_GENERATED_BY_DEFAULT)
        defVal = "GENERATED BY DEFAULT AS IDENTITY";
      else
        defVal = "GENERATED ALWAYS AS IDENTITY";
      
      NAString idOptions;
      if ((displayType != 1) && (nac->getNATable()->getSGAttributes()))
        nac->getNATable()->getSGAttributes()->display(NULL, &idOptions, TRUE);
      
      if (NOT idOptions.isNull())
        defVal += " (" + idOptions + " )";
      
      identityCol = TRUE;
    }
  else
    defVal = "NO DEFAULT";
  
  attrStr = defVal;
  
  if (nac->getHeading())
    {
      NAString heading = "HEADING ";
      
      NAString externalHeading = "";
      ToQuotedString(externalHeading, nac->getHeading());
      heading += externalHeading;
      
      attrStr += " ";
      attrStr += heading;
    }
  
  if (NOT nat->supportsSQLnull())
    {
      NAString nullVal = "NOT NULL NOT DROPPABLE";
      
      attrStr += " ";
      attrStr += nullVal;
    }
  
  char * sqlmxRegr = getenv("SQLMX_REGRESS");
  if ((! sqlmxRegr) ||
      (displayType == 3))
    {
      if ((NOT isAlignedRowFormat) &&
            CmpSeabaseDDL::isSerialized(nac->getHbaseColFlags()))
        attrStr += " SERIALIZED";
      else if ((CmpCommon::getDefault(HBASE_SERIALIZATION) == DF_ON) ||
                (displayType == 3))
        attrStr += " NOT SERIALIZED";
    }
  
  if (displayType != 3)
    {
      if (nac->isAlteredColumn())
        {
          attrStr += " /*altered_col*/ ";
        }
      else if (nac->isAddedColumn())
        {
          attrStr += " /*added_col*/ ";
        }
    }

  sprintf(&buf[strlen(buf)], "%s %s", 
          nas.data(), 
          attrStr.data());
  
  NAString colString(buf);
  Int32 j = ii;
  if (inSpace)
    outputColumnLine(*space, colString, j);

  return 0;
}

// type:  1, invoke. 2, showddl. 3, create_like
short cmpDisplayColumns(const NAColumnArray & naColArr,
                        short displayType,
                        Space &space, char * buf, 
                        NABoolean displaySystemCols,
                        NABoolean namesOnly,
                        Lng32 &identityColPos,
                        NABoolean isExternalTable,
                        NABoolean isAlignedRowFormat,
                        NABoolean omitLobColumns = FALSE,
                        char * inColName = NULL,
                        short ada = 0, // 0,add. 1,drop. 2,alter
                        const NAColumn * nacol = NULL,
                        const NAType * natype = NULL,
                        UInt32 columnLengthLimit = UINT_MAX,
                        NAList<const NAColumn *> * truncatedColumnList = NULL)
{
  Lng32 ii = 0;
  identityColPos = -1;
  NABoolean identityCol = FALSE;
  for (Int32 i = 0; i < (Int32)naColArr.entries(); i++)
    {
      NAColumn * nac = naColArr[i];

      if ((NOT displaySystemCols) &&
          (nac->isSystemColumn()))
        {
          continue;
        }

      if (omitLobColumns &&
          (nac->getType()->isLob()))
        {
          continue;
        }

      const NAString &colName = nac->getColName();

      if ((inColName) && (ada == 1))
        {
          if (colName == inColName) // remove this column
            continue;
        }
      
      if ((inColName) && (colName == inColName) &&
          (ada == 2) && (nacol) && (natype))
        {
          if (cmpDisplayColumn(nac, NULL, natype, displayType, &space, buf, ii, namesOnly, identityCol, 
                               isExternalTable, isAlignedRowFormat, columnLengthLimit, truncatedColumnList))
            return -1;
        }
      else
        {
          if (cmpDisplayColumn(nac, NULL, NULL, displayType, &space, buf, ii, namesOnly, identityCol, 
                               isExternalTable, isAlignedRowFormat, columnLengthLimit, truncatedColumnList))
            return -1;
        }

      if (identityCol)
        identityColPos = i;

      ii++;
    }

  if ((inColName) && (ada == 0) && (nacol))
    {
      if (cmpDisplayColumn(nacol, NULL, NULL, displayType, &space, buf, ii, namesOnly, identityCol, 
                           isExternalTable, isAlignedRowFormat, columnLengthLimit, truncatedColumnList))
        return -1;
    }

  return 0;
}

short cmpDisplayPrimaryKey(const NAColumnArray & naColArr,
                           Lng32 numKeys,
                           NABoolean displaySystemCols,
                           Space &space, char * buf, 
                           NABoolean displayCompact,
                           NABoolean displayAscDesc,
                           NABoolean displayParens)
{
  if (numKeys > 0)
    {
      if (displayParens)
        {
          if (displayCompact)
            sprintf(&buf[strlen(buf)],  "(");
          else
            {
              outputShortLine(space, "  ( ");
            }
        }

      NABoolean isFirst = TRUE;
      Int32 j = -1;
      for (Int32 jj = 0; jj < numKeys; jj++)
        {
          NAColumn * nac = naColArr[jj];

          if ((NOT displaySystemCols) &&
              (nac->isSystemColumn()))
            {
              continue;
            }
          else
            j++; // increment num of columns displayed
          
          const NAString &keyName = nac->getColName();
          if (displayCompact)
            {
              sprintf(&buf[strlen(buf)], "%s%s%s", 
                      (NOT isFirst ? ", " : ""),
                      ANSI_ID(keyName.data()),
                      (displayAscDesc ?
                       (! naColArr.isAscending(jj) ? " DESC" : " ASC") :
                       " "));
            }
          else
            {
              sprintf(buf, "%s%s", 
                      ANSI_ID(keyName.data()),
                      (displayAscDesc ?
                       (! naColArr.isAscending(jj) ? " DESC" : " ASC") :
                       " "));
              
              NAString colString(buf);
              outputColumnLine(space, colString, j);
            }

          isFirst = FALSE;
        } // for

      if (displayParens)
        {
          if (displayCompact)
            {
              sprintf(&buf[strlen(buf)],  ")");
              outputLine(space, buf, 2);
            }
          else
            {
              outputShortLine(space, "  )");
            }
        }
    } // if
  
  return 0;
}


// type:  1, invoke. 2, showddl. 3, create_like
short CmpDescribeSeabaseTable ( 
                               const CorrName  &dtName,
                               short type,
                               char* &outbuf,
                               ULng32 &outbuflen,
                               CollHeap *heap,
                               const char * pkeyName,
                               const char * pkeyStr,
                               NABoolean withPartns,
                               NABoolean withoutSalt,
                               NABoolean withoutDivisioning,
                               NABoolean withoutRowFormat,
                               NABoolean withoutLobColumns,
                               UInt32 columnLengthLimit,
                               NABoolean noTrailingSemi,
                               char * colName,
                               short ada,
                               const NAColumn * nacol,
                               const NAType * natype,
                               Space *inSpace,
                               NABoolean isDetail)
{
  const NAString& tableName =
    dtName.getQualifiedNameObj().getQualifiedNameAsAnsiString(TRUE);

  const NAString& objectName =
    dtName.getQualifiedNameObj().getObjectName();
 
  if (CmpCommon::context()->isUninitializedSeabase())
     {
       *CmpCommon::diags() << DgSqlCode(CmpCommon::context()->uninitializedSeabaseErrNum());

      return -1;
    }

  // set isExternalTable to allow Hive External tables to be described
  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);

  NATable *naTable = bindWA.getNATableInternal((CorrName&)dtName); 

  TableDesc *tdesc = NULL;
  if (naTable == NULL || bindWA.errStatus())
    return -1;
  else
    {
      tdesc = bindWA.createTableDesc(naTable, (CorrName&)dtName);
      if (bindWA.errStatus())
        return -1;
    }

  if (NOT naTable->isHbaseTable())
    {
      if (CmpCommon::diags()->getNumber() == 0)
        *CmpCommon::diags() << DgSqlCode(-CAT_UNSUPPORTED_COMMAND_ERROR);
      return -1;
    }

  NABoolean isVolatile = naTable->isVolatileTable();
  NABoolean isExternalTable = naTable->isTrafExternalTable();
  NABoolean isImplicitExternalTable = naTable->isImplicitTrafExternalTable();
  NABoolean isHbaseMapTable = naTable->isHbaseMapTable();
  NABoolean isHbaseCellOrRowTable = 
    (naTable->isHbaseCellTable() || naTable->isHbaseRowTable());

  NABoolean isExternalHbaseTable = FALSE;
  NABoolean isExternalHiveTable = FALSE;
  NAString extName;
  if (isExternalTable)
    {
      extName = ComConvertTrafNameToNativeName(
           dtName.getQualifiedNameObj().getCatalogName(),
           dtName.getQualifiedNameObj().getUnqualifiedSchemaNameAsAnsiString(),
           dtName.getQualifiedNameObj().getUnqualifiedObjectNameAsAnsiString());

      QualifiedName qn(extName, 3);
      if (qn.getCatalogName() == HBASE_SYSTEM_CATALOG)
        isExternalHbaseTable = TRUE;
      else if (qn.getCatalogName() == HIVE_SYSTEM_CATALOG)
        isExternalHiveTable = TRUE;
    }

  char * buf = new (heap) char[15000];
  CMPASSERT(buf);

  time_t tp;
  time(&tp);
  
  Space lSpace;

  Space * space;
  if (inSpace)
    space = inSpace;
  else
    space = &lSpace;

  char * sqlmxRegr = getenv("SQLMX_REGRESS");

  NABoolean displayPrivilegeGrants = TRUE;
  if (((CmpCommon::getDefault(SHOWDDL_DISPLAY_PRIVILEGE_GRANTS) == DF_SYSTEM) && sqlmxRegr) ||
      (CmpCommon::getDefault(SHOWDDL_DISPLAY_PRIVILEGE_GRANTS) == DF_OFF)) 
    displayPrivilegeGrants = FALSE;
 
  // display syscols for invoke if not running regrs
  //
  NABoolean displaySystemCols = (type == 1);

  NABoolean isView = (naTable->getViewText() ? TRUE : FALSE);

  // emit an initial newline
  outputShortLine(*space, " ");

  // Used for context switches
  CmpSeabaseDDL cmpSBD((NAHeap*)heap);

  // Verify that user can perform the describe command
  // No need to check privileges for create like operations (type 3)
  // since the create code performs authorization checks
  // Nor for hiveExternal tables - already checked
  if (!isExternalHiveTable)
  {
    if (type != 3)
      {
        // For native HBase tables that have no objectUID, we can't check 
        // user privileges so only allow operation for privileged users
        if (isHbaseCellOrRowTable && 
            !ComUser::isRootUserID() && !ComUser::currentUserHasRole(HBASE_ROLE_ID) &&
            naTable->objectUid().get_value() == 0) 
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);
          return -1;
        }
 
        PrivMgrUserPrivs privs; 
        PrivMgrUserPrivs *pPrivInfo = NULL;
    
        // metadata tables do not cache privilege information, go get it now
        if (CmpCommon::context()->isAuthorizationEnabled() &&
            naTable->getPrivInfo() == NULL)
          {
            std::string privMDLoc(ActiveSchemaDB()->getDefaults().getValue(SEABASE_CATALOG));
            privMDLoc += std::string(".\"") +
               std::string(SEABASE_PRIVMGR_SCHEMA) +
               std::string("\"");
            PrivMgrCommands privInterface(privMDLoc, CmpCommon::diags(), 
                                          PrivMgr::PRIV_INITIALIZED);


            // we should switch to another CI only if we are in an embedded CI
            if (cmpSBD.switchCompiler())
            {
               *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);
               return -1;
            }
           
            PrivStatus retcode = privInterface.getPrivileges((int64_t)naTable->objectUid().get_value(),
                                                             naTable->getObjectType(),
                                                             ComUser::getCurrentUser(),
                                                             privs);

            // switch back the original commpiler, ignore error for now
            cmpSBD.switchBackCompiler();

            if (retcode == STATUS_ERROR)
              {
                *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);
                return -1;
              }
            pPrivInfo = &privs;
          }
        else
          pPrivInfo = naTable->getPrivInfo();

        // Allow object owners to perform showddl operation
        if ((naTable->getOwner() != ComUser::getCurrentUser()) &&
            !ComUser::currentUserHasRole(naTable->getOwner()))
        {
          if (!CmpDescribeIsAuthorized(SQLOperation::UNKNOWN, 
                                       pPrivInfo,
                                       COM_BASE_TABLE_OBJECT))
            return -1;
        }
      }
    }
  
  Int64 objectUID = (Int64) naTable->objectUid().get_value();

  if ((type == 2) && (isView))
    {
      NAString viewtext(naTable->getViewText());

      viewtext = viewtext.strip(NAString::trailing, ';');
      viewtext += " ;";

      outputLongLine(*space, viewtext, 0);

      //display comment for VIEW
      if (objectUID > 0)
        {
          if (cmpSBD.switchCompiler())
            {
              *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_COMMENTS);
              return -1;
            }

          ComTdbVirtObjCommentInfo objCommentInfo;
          if (cmpSBD.getSeabaseObjectComment(objectUID, COM_VIEW_OBJECT, objCommentInfo, heap))
            {
              *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_COMMENTS);
              cmpSBD.switchBackCompiler();
              return -1;
            }

          //display VIEW COMMENT statements
          if (objCommentInfo.objectComment != NULL)
            {
              //new line
              outputLine(*space, "", 0);

              sprintf(buf, "COMMENT ON VIEW %s IS '%s' ;",
                           tableName.data(),
                           objCommentInfo.objectComment);
              outputLine(*space, buf, 0);
            }

          if (objCommentInfo.numColumnComment > 0 && objCommentInfo.columnCommentArray != NULL)
            {
              //display Column COMMENT statements
              outputLine(*space, "", 0);
              for (int idx = 0; idx < objCommentInfo.numColumnComment; idx++)
                {
                  sprintf(buf,  "COMMENT ON COLUMN %s.%s IS '%s' ;",
                           tableName.data(),
                           objCommentInfo.columnCommentArray[idx].columnName,
                           objCommentInfo.columnCommentArray[idx].columnComment);
                   outputLine(*space, buf, 0);
                }
            }

          //do a comment info memory clean
          NADELETEARRAY(objCommentInfo.columnCommentArray, objCommentInfo.numColumnComment, ComTdbVirtColumnCommentInfo, heap);

          cmpSBD.switchBackCompiler();
        }

      // Display grant statements
      if (CmpCommon::context()->isAuthorizationEnabled() && displayPrivilegeGrants)
      {
        std::string privMDLoc(ActiveSchemaDB()->getDefaults().getValue(SEABASE_CATALOG));
        privMDLoc += std::string(".\"") +    
             std::string(SEABASE_PRIVMGR_SCHEMA) +    
             std::string("\"");
        PrivMgrCommands privInterface(privMDLoc, CmpCommon::diags(),
                                      PrivMgr::PRIV_INITIALIZED);
        std::string privilegeText;
        PrivMgrObjectInfo objectInfo(naTable);
        if (cmpSBD.switchCompiler())
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);
          return -1;
        }
 
 
        if (privInterface.describePrivileges(objectInfo, privilegeText))
        {
          outputShortLine(*space, " ");
          outputLine(*space, privilegeText.c_str(), 0);
        }

        cmpSBD.switchBackCompiler();
      }

      outbuflen = space->getAllocatedSpaceSize();
      outbuf = new (heap) char[outbuflen];
      space->makeContiguous(outbuf, outbuflen);
      
      NADELETEBASIC(buf, heap);
      
      return 0;
    }

  if (type == 1)
    {
      if (isView)
        sprintf(buf,  "-- Definition of Trafodion view %s\n"
                "-- Definition current  %s",
                tableName.data(), ctime(&tp));
      else
        sprintf(buf,  "-- Definition of Trafodion%stable %s\n"
                "-- Definition current  %s",
                (isVolatile ? " volatile " : 
                 (isHbaseMapTable ? " HBase mapped " :
                  (isExternalTable ? " external " : " "))), 
                (isHbaseMapTable ? objectName.data() : 
                 tableName.data()),
                ctime(&tp));
      outputShortLine(*space, buf);
    }
  else if (type == 2)
    {
      if (isHbaseCellOrRowTable)
        outputShortLine(*space, "/*");

      NAString tabType;
      if (isVolatile)
        tabType = " VOLATILE ";
      else if (isExternalTable)
        {
          if (isImplicitExternalTable)
            tabType = " /*IMPLICIT*/ EXTERNAL ";
          else
            tabType = " EXTERNAL ";
        }
      else
        tabType = " ";

      sprintf(
           buf,  "CREATE%sTABLE %s",
           tabType.data(),
           (isExternalTable ? objectName.data() : tableName.data()));
      outputShortLine(*space, buf);
    }

  Lng32 identityColPos = -1;
  NABoolean closeParan = FALSE;
  NAList<const NAColumn *> truncatedColumnList(heap);
  if ((NOT isExternalTable) ||
      ((isExternalTable) && 
       ((isExternalHbaseTable && ((type == 1) || (type == 3))) ||
        (isExternalHbaseTable && naTable->isHbaseMapTable() && (type == 2)) ||
        (isExternalHiveTable && (type != 2)) ||
        (isExternalHiveTable && (type == 2) && (naTable->hiveExtColAttrs())))))
    {
      outputShortLine(*space, "  ( ");
      cmpDisplayColumns(naTable->getNAColumnArray(), 
                        type, *space, buf, 
                        displaySystemCols, 
                        FALSE,
                        identityColPos,
                        (isExternalTable && (NOT isHbaseMapTable)),
                        naTable->isSQLMXAlignedTable(),
                        withoutLobColumns,
                        colName, ada, nacol, natype,
                        columnLengthLimit,
                        &truncatedColumnList);
      closeParan = TRUE;
    }

  Int32 nonSystemKeyCols = 0;
  NABoolean isStoreBy = FALSE;
  NABoolean forceStoreBy = FALSE;
  NABoolean isSalted = FALSE;
  NABoolean isDivisioned = FALSE;
  ItemExpr *saltExpr = NULL;
  LIST(NAString) divisioningExprs(heap);
  LIST(NABoolean) divisioningExprAscOrders(heap);

  if (naTable->getClusteringIndex())
    {
      NAFileSet * naf = naTable->getClusteringIndex();
      for (Lng32 i = 0; i < naf->getIndexKeyColumns().entries(); i++)
        {
          NAColumn * nac = naf->getIndexKeyColumns()[i];
          if (nac->isComputedColumnAlways())
            {
              if (nac->isSaltColumn()  && !withoutSalt)
                {
                  isSalted = TRUE;
                  ItemExpr * saltBaseCol =
                    tdesc->getColumnList()[nac->getPosition()].getItemExpr();
                  CMPASSERT(saltBaseCol->getOperatorType() == ITM_BASECOLUMN);
                  saltExpr = ((BaseColumn *) saltBaseCol)->getComputedColumnExpr().getItemExpr();
                }
              else if (nac->isDivisioningColumn() && !withoutDivisioning)
                {
                  NABoolean divColIsAsc = naf->getIndexKeyColumns().isAscending(i);
                  // any other case of computed column is treated as divisioning for now
                  isDivisioned = TRUE;
                  divisioningExprs.insert(NAString(nac->getComputedColumnExprString()));
                  divisioningExprAscOrders.insert(divColIsAsc);
                }
            }
          if (NOT nac->isSystemColumn())
            nonSystemKeyCols++;
          else if (nac->isSyskeyColumn())
            isStoreBy = TRUE;

          // if we are shortening a column, set isStoreBy to TRUE since truncating
          // a character string may cause formerly distinct values to become equal
          // (that is, we want to change PRIMARY KEY to STORE BY)
          for (Lng32 j = 0; j < truncatedColumnList.entries(); j++)
            {
              const NAColumn * truncatedColumn = truncatedColumnList[j];
              if (nac->getColName() == truncatedColumn->getColName())
                {
                  isStoreBy = TRUE;
                  forceStoreBy = TRUE;
                }
            }
        }
    }
  
  if ((nonSystemKeyCols == 0) && (!forceStoreBy))
    isStoreBy = FALSE;

  if (type == 1)
    outputShortLine(*space, "  )");

  Lng32 numBTpkeys = 0;

  NAFileSet * naf = naTable->getClusteringIndex();
  NABoolean isAudited = (naf ? naf->isAudited() : TRUE);

  NABoolean isAligned = naTable->isSQLMXAlignedTable();

  if ((type == 3) && (pkeyStr))
    {
      if (pkeyName)
        {
          NAString pkeyPrefix(", CONSTRAINT ");
          pkeyPrefix += NAString(pkeyName) + " PRIMARY KEY ";
          outputLine(*space, pkeyPrefix.data(), 0);
        }
      else
        {
          outputShortLine(*space, " , PRIMARY KEY ");
        }

      outputLine(*space, pkeyStr, 2);
    }
  else
    {
      if ((naf) &&
          (nonSystemKeyCols > 0) &&
          (NOT isStoreBy))
        {
          NAString pkeyConstrName;
          NAString pkeyConstrObjectName;
          if (type == 2) // showddl
            {
              const AbstractRIConstraintList &uniqueList = naTable->getUniqueConstraints();
              
              for (Int32 i = 0; i < uniqueList.entries(); i++)
                {
                  AbstractRIConstraint *ariConstr = uniqueList[i];
                  
                  UniqueConstraint * uniqConstr = (UniqueConstraint*)ariConstr;
                  if (uniqConstr->isPrimaryKeyConstraint())
                    {                  
                      pkeyConstrName =
                        uniqConstr->getConstraintName().getQualifiedNameAsAnsiString(TRUE);
                      pkeyConstrObjectName =
                        uniqConstr->getConstraintName().getObjectName();
                      break;
                    }
                } // for
            } // type 2

          numBTpkeys = naf->getIndexKeyColumns().entries();
          
          if (type == 1)
            sprintf(buf,  "  PRIMARY KEY ");
          else
            {
              // Display primary key name for showddl (type == 2).
              // First check to see if pkey name is a generated random name or 
              // a user specified name.
              // If it is a generated random name, then dont display it.
              // This is being done for backward compatibility in showddl
              // output as well as avoid the need to update multiple
              // regressions files.
              // Currently we check to see if the name has random generated
              // format to determine whether to display it or not.
              // If it so happens that a user specified primary key constraint 
              // has that exact format, then it will not be displayed.
              // At some point in future, we can store in metadata if pkey 
              // name was internally generated or user specified.
              if ((type == 2) &&
                  (NOT pkeyConstrObjectName.isNull()) &&
                  (NOT ComIsRandomInternalName(pkeyConstrObjectName)))
                sprintf(buf, " , CONSTRAINT %s PRIMARY KEY ",
                        pkeyConstrName.data());
              else
                sprintf(buf,  "  , PRIMARY KEY ");
            }

          // if all primary key columns are 'not serialized primary key',
          // then display that.
          NABoolean serialized = FALSE;
          for (Int32 jj = 0; 
               ((NOT serialized) && (jj <  naf->getIndexKeyColumns().entries())); jj++)
            {
              NAColumn * nac = (naf->getIndexKeyColumns())[jj];
              if (NOT nac->isPrimaryKeyNotSerialized())
                serialized = TRUE;
            }

          if (((type == 1) || (type == 2)) && (NOT serialized))
            {
              strcat(&buf[strlen(buf)], "NOT SERIALIZED ");
            }

          cmpDisplayPrimaryKey(naf->getIndexKeyColumns(), 
                               naf->getIndexKeyColumns().entries(),
                               displaySystemCols,
                               *space, buf, TRUE, TRUE, TRUE);
        } // if
    }

  if (type != 1)
    {
      if (closeParan)
        outputShortLine(*space, "  )");
      if (isStoreBy)
        {
          sprintf(buf,  "  STORE BY ");
          
          cmpDisplayPrimaryKey(naf->getIndexKeyColumns(), 
                               naf->getIndexKeyColumns().entries(),
                               displaySystemCols,
                               *space, buf, TRUE, TRUE, TRUE);
        }
      
      if ((isSalted) && !withoutSalt)
        {
          Lng32 currPartitions = naf->getCountOfPartitions();
          Lng32 numPartitions = naf->numSaltPartns();

          if (numPartitions != currPartitions)
            sprintf(buf,  "  SALT USING %d PARTITIONS /* ACTUAL PARTITIONS %d */", numPartitions, currPartitions);
          else
            sprintf(buf,  "  SALT USING %d PARTITIONS", numPartitions);
            
          outputShortLine(*space, buf);

          ValueIdList saltCols;

          // get leaf nodes in left-to-right order from the salt expression,
          // those are the salt columns
          saltExpr->getLeafValueIds(saltCols);

          // remove any nodes that are not base columns (e.g. # of partns)
          CollIndex i = 0;
          while (i < saltCols.entries())
            if (saltCols[i].getItemExpr()->getOperatorType() == ITM_BASECOLUMN)
              i++;
            else
              saltCols.removeAt(i);

          // print a list of salt columns if they are a subset of the
          // (non-system) clustering key columns or if they appear
          // in a different order
          NABoolean printSaltCols = (saltCols.entries() < nonSystemKeyCols);

          if (!printSaltCols)
            for (CollIndex j=0; j<saltCols.entries(); j++)
              {
                BaseColumn *bc = (BaseColumn *) saltCols[j].getItemExpr();
                // compare col # with clustering key col #,
                // but skip leading salt col in clustering key
                if (bc->getColNumber() != naf->getIndexKeyColumns()[j+1]->getPosition())
                  printSaltCols = TRUE;
              }

          if (printSaltCols)
            {
              NAString sc = "       ON (";
              for (CollIndex k=0; k<saltCols.entries(); k++)
                {
                  BaseColumn *bc = (BaseColumn *) saltCols[k].getItemExpr();
                  if (k > 0)
                    sc += ", ";
                  sc += ANSI_ID(bc->getColName().data());
                }
              sc += ")";
              outputShortLine(*space, sc.data());
            }
        }
      else if ((NOT isSalted) && (withPartns))
        {
          Lng32 currPartitions = naf->getCountOfPartitions();

          if (currPartitions > 1)
            {
              sprintf(buf,  "  /* ACTUAL PARTITIONS %d */", currPartitions);
              
              outputShortLine(*space, buf);
            }
        }

      if (isDivisioned && !withoutDivisioning)
        {
          NAString divByClause = "  DIVISION BY (";

          for (CollIndex d=0; d<divisioningExprs.entries(); d++)
            {
              if (d > 0)
                divByClause += ", ";
              divByClause += divisioningExprs[d];
              if (!divisioningExprAscOrders[d])
                divByClause += " DESC";
            }
          outputShortLine(*space, divByClause.data());
          divByClause = "     NAMED AS (";

          NAFileSet * naf = naTable->getClusteringIndex();
          NABoolean firstDivCol = TRUE;

          for (Lng32 i = 0; i < naf->getIndexKeyColumns().entries(); i++)
            {
              NAColumn * nac = naf->getIndexKeyColumns()[i];
              if (nac->isDivisioningColumn())
                {
                  if (firstDivCol)
                    firstDivCol = FALSE;
                  else
                    divByClause += ", ";
                  divByClause += "\"";
                  divByClause += nac->getColName();
                  divByClause += "\"";
                }
            }

          divByClause += "))";
          outputShortLine(*space, divByClause.data());
        }

      NABoolean attributesSet = FALSE;
      NABoolean formatSet = FALSE;
      char attrs[2000];
      if ((type == 3/*create like*/) && (NOT withoutRowFormat))
        {
          strcpy(attrs, " ATTRIBUTES ");
          if (isAligned)
            strcat(attrs, "ALIGNED FORMAT ");
          else
            strcat(attrs, "HBASE FORMAT ");
          attributesSet = TRUE;
          formatSet = TRUE;
        }

      if (((NOT isAudited) || (isAligned)) ||
          ((sqlmxRegr) && (type == 3) && ((NOT isAudited) || (isAligned))) ||
          ((NOT naTable->defaultColFam().isNull()) && 
           (naTable->defaultColFam() != SEABASE_DEFAULT_COL_FAMILY)))
        {
          if (NOT attributesSet)
            strcpy(attrs, " ATTRIBUTES ");

          if (NOT isAudited)
            strcat(attrs, "NO AUDIT ");
          if ((NOT formatSet) && isAligned)
            strcat(attrs, "ALIGNED FORMAT ");
          if ((NOT naTable->defaultColFam().isNull()) &&
              (naTable->defaultColFam() != SEABASE_DEFAULT_COL_FAMILY))
            {
              strcat(attrs, "DEFAULT COLUMN FAMILY '");
              strcat(attrs, naTable->defaultColFam());
              strcat(attrs, "'");
            }

          attributesSet = TRUE;
        }

      if (attributesSet)
        outputShortLine(*space, attrs);
        
      if (!isView && (naTable->hbaseCreateOptions()) &&
          (naTable->hbaseCreateOptions()->entries() > 0))
        {
          outputShortLine(*space, "  HBASE_OPTIONS ");
          outputShortLine(*space, "  ( ");
          
          for (Lng32 i = 0; i < naTable->hbaseCreateOptions()->entries(); i++)
            {
              HbaseCreateOption * hco = (*naTable->hbaseCreateOptions())[i];

              NABoolean comma = FALSE;
              if (i < naTable->hbaseCreateOptions()->entries() - 1)
                comma = TRUE;
              sprintf(buf, "    %s = '%s'%s", hco->key().data(), hco->val().data(),
                      (comma ? "," : " "));
              outputShortLine(*space, buf);
            }

          outputShortLine(*space, "  ) ");
        }

      if ((isExternalTable) &&
          (NOT isHbaseMapTable) &&
          (type == 2))
        {
          sprintf(buf, "  FOR %s", extName.data());
          outputShortLine(*space, buf);
        }

      if ((isHbaseMapTable) && (type != 3))
        {
          sprintf(buf, "  MAP TO HBASE TABLE %s DATA FORMAT %s",
                  dtName.getQualifiedNameObj().getObjectName().data(),
                  (naTable->isHbaseDataFormatString() ? "VARCHAR" : "NATIVE"));
          outputShortLine(*space, buf);
        }

      if (NOT noTrailingSemi)
        outputShortLine(*space, ";");

      if ((type == 2) && isHbaseCellOrRowTable)
        outputShortLine(*space, "*/");
    }

  // showddl internal sequences created for identity cols
  if ((identityColPos >= 0) && (type == 2) && (NOT sqlmxRegr))
    {
      NAString seqName;
      SequenceGeneratorAttributes::genSequenceName
        (dtName.getQualifiedNameObj().getCatalogName(), 
         dtName.getQualifiedNameObj().getSchemaName(), 
         dtName.getQualifiedNameObj().getObjectName(), 
         naTable->getNAColumnArray()[identityColPos]->getColName(),
         seqName);

      CorrName csn(seqName, STMTHEAP,
                   dtName.getQualifiedNameObj().getSchemaName(), 
                   dtName.getQualifiedNameObj().getCatalogName());
      outputLine(*space, "\n-- The following sequence is a system created sequence --", 0);
      
      char * dummyBuf;
      ULng32 dummyLen;
      CmpDescribeSequence(csn, dummyBuf, dummyLen, STMTHEAP, space);
    }

  if (((type == 1) && (NOT sqlmxRegr)) || (type == 2))
    {
      const NAFileSetList &indexList = naTable->getIndexList();
      
      for (Int32 i = 0; i < indexList.entries(); i++)
	{
	  const NAFileSet * naf = indexList[i];
          isAligned = naf->isSqlmxAlignedRowFormat();
	  if (naf->getKeytag() == 0)
	    continue;
	  
	  const QualifiedName &qn = naf->getFileSetName();
	  const NAString& indexName =
	    qn.getObjectName();
	  
	  if (type == 1)
	    {
	      char vu[40];
	      strcpy(vu, " ");
	      if (isVolatile)
		strcat(vu, "volatile ");
	      if (naf->uniqueIndex())
		strcat(vu, "unique ");

	      sprintf(buf,  "\n-- Definition of%sTrafodion%sindex %s\n"
		      "-- Definition current  %s",
		      ((NOT naf->isCreatedExplicitly()) ? " implicit " : " "),
		      vu,
		      indexName.data(),
		      ctime(&tp));
	      outputShortLine(*space, buf);
	    }
	  else
	    {
	      char vu[40];
	      strcpy(vu, " ");
	      if (isVolatile)
		strcat(vu, "VOLATILE ");
	      if (naf->uniqueIndex())
		strcat(vu, "UNIQUE ");

	      if (NOT naf->isCreatedExplicitly())
		{
		  outputLine(*space, "\n-- The following index is a system created index --", 0);
		}

	      sprintf(buf,  "%sCREATE%sINDEX %s ON %s",
		      (naf->isCreatedExplicitly() ? "\n" : ""),
		      vu,
		      indexName.data(),
		      tableName.data());
	      outputLine(*space, buf, 0);
	    }
	  
	  if (type == 1)
	    {
              Lng32 dummy;
	      outputShortLine(*space, "  ( ");
	      cmpDisplayColumns(naf->getAllColumns(), 
                                type, *space, buf,
				displaySystemCols,
				(type == 2),
                                dummy,
                                isExternalTable,
                                isAligned,
                                withoutLobColumns);
	      outputShortLine(*space, "  )");
	      
	      sprintf(buf,  "  PRIMARY KEY ");
	      outputShortLine(*space, buf);
	    }
	  
	  Lng32 numIndexCols = ((type == 1) ? 
                                naf->getIndexKeyColumns().entries() :
                                naf->getCountOfColumns(
                                     TRUE,
                                     TRUE,  // user-spedified index cols only
                                     FALSE,
                                     FALSE));

	  cmpDisplayPrimaryKey(naf->getIndexKeyColumns(), numIndexCols, 
			       displaySystemCols,
			       *space, buf, FALSE, TRUE, TRUE);

          if ((NOT sqlmxRegr) && isAligned)
          {
             char attrs[2000];
             strcpy(attrs, " ATTRIBUTES ");
             if (isAligned)
                strcat(attrs, "ALIGNED FORMAT ");
             outputShortLine(*space, attrs);
          }

          if ((naf->hbaseCreateOptions()) && (type == 2) &&
               (naf->hbaseCreateOptions()->entries() > 0))
           {
             outputShortLine(*space, "  HBASE_OPTIONS ");
             outputShortLine(*space, "  ( ");
          
             for (Lng32 i = 0; i < naf->hbaseCreateOptions()->entries(); i++)
             {
               HbaseCreateOption * hco = (*naf->hbaseCreateOptions())[i];
               char separator = 
                 ((i < naf->hbaseCreateOptions()->entries() - 1) ?
                  ',' : ' ') ;
               sprintf(buf, "    %s = '%s'%c", hco->key().data(), 
                       hco->val().data(),separator);
               outputShortLine(*space, buf);
             }

             outputShortLine(*space, "  ) ");
           }

          if ((naf->numSaltPartns() > 0) && (type == 2))
            outputShortLine(*space, " SALT LIKE TABLE ");
	  
	  if (type == 2)
	    outputShortLine(*space, ";");
	  
	} // for

      if (type == 2) // showddl
        {
          const AbstractRIConstraintList &uniqueList = naTable->getUniqueConstraints();
          
          for (Int32 i = 0; i < uniqueList.entries(); i++)
            {
              AbstractRIConstraint *ariConstr = uniqueList[i];

              if (ariConstr->getOperatorType() != ITM_UNIQUE_CONSTRAINT)
                continue;

              UniqueConstraint * uniqConstr = (UniqueConstraint*)ariConstr;
              if (uniqConstr->isPrimaryKeyConstraint())
                continue;

              const NAString& ansiTableName = 
                uniqConstr->getDefiningTableName().getQualifiedNameAsAnsiString(TRUE);

              const NAString &ansiConstrName =
                uniqConstr->getConstraintName().getQualifiedNameAsAnsiString(TRUE);

              sprintf(buf,  "\nALTER TABLE %s ADD CONSTRAINT %s UNIQUE ",
                      ansiTableName.data(),
                      ansiConstrName.data());
              outputLine(*space, buf, 0);

              NAColumnArray nacarr;

              for (Lng32 j = 0; j < uniqConstr->keyColumns().entries(); j++)
                {
                  nacarr.insert(uniqConstr->keyColumns()[j]);
                }

              cmpDisplayPrimaryKey(nacarr, 
                                   uniqConstr->keyColumns().entries(),
                                   FALSE,
                                   *space, &buf[strlen(buf)], FALSE, FALSE, TRUE);

              outputShortLine(*space, ";");
            } // for

          const AbstractRIConstraintList &refList = naTable->getRefConstraints();

          for (Int32 i = 0; i < refList.entries(); i++)
            {
              AbstractRIConstraint *ariConstr = refList[i];

              if (ariConstr->getOperatorType() != ITM_REF_CONSTRAINT)
                continue;

              RefConstraint * refConstr = (RefConstraint*)ariConstr;
              const ComplementaryRIConstraint &uniqueConstraintReferencedByMe
                = refConstr->getUniqueConstraintReferencedByMe();

              NATable *otherNaTable = NULL;
              CorrName otherCN(uniqueConstraintReferencedByMe.getTableName());
              otherNaTable = bindWA.getNATable(otherCN);
              if (otherNaTable == NULL || bindWA.errStatus())
                return -1;
              
              const NAString& ansiTableName = 
                refConstr->getDefiningTableName().getQualifiedNameAsAnsiString(TRUE);

              const NAString &ansiConstrName =
                refConstr->getConstraintName().getQualifiedNameAsAnsiString(TRUE);

              sprintf(buf,  "\nALTER TABLE %s ADD CONSTRAINT %s FOREIGN KEY ",
                      ansiTableName.data(),
                      ansiConstrName.data());
              outputLine(*space, buf, 0);

              NAColumnArray nacarr;

              for (Lng32 j = 0; j < refConstr->keyColumns().entries(); j++)
                {
                  nacarr.insert(refConstr->keyColumns()[j]);
                }

              cmpDisplayPrimaryKey(nacarr, 
                                   refConstr->keyColumns().entries(),
                                   FALSE,
                                   *space, &buf[strlen(buf)], FALSE, FALSE, TRUE);

              const NAString& ansiOtherTableName = 
                uniqueConstraintReferencedByMe.getTableName().getQualifiedNameAsAnsiString(TRUE);             
              sprintf(buf,  " REFERENCES %s ",
                      ansiOtherTableName.data());
              outputLine(*space, buf, 0);

              AbstractRIConstraint * otherConstr = 
                refConstr->findConstraint(&bindWA, refConstr->getUniqueConstraintReferencedByMe());
              
              NAColumnArray nacarr2;
              for (Lng32 j = 0; j < otherConstr->keyColumns().entries(); j++)
                {
                  nacarr2.insert(otherConstr->keyColumns()[j]);
                }

              cmpDisplayPrimaryKey(nacarr2, 
                                   otherConstr->keyColumns().entries(),
                                   FALSE,
                                   *space, &buf[strlen(buf)], FALSE, FALSE, TRUE);

              if (NOT refConstr->getIsEnforced())
                {
                  outputShortLine(*space, " NOT ENFORCED ");
                }

              outputShortLine(*space, ";");
            } // for

      const CheckConstraintList &checkList = naTable->getCheckConstraints();
      
      for (Int32 i = 0; i < checkList.entries(); i++)
        {
          CheckConstraint *checkConstr = (CheckConstraint*)checkList[i];
          
          const NAString &ansiConstrName =
            checkConstr->getConstraintName().getQualifiedNameAsAnsiString(TRUE);

          sprintf(buf,  "\nALTER TABLE %s ADD CONSTRAINT %s CHECK %s",
                  tableName.data(),
                  ansiConstrName.data(), 
                  checkConstr->getConstraintText().data());
          outputLine(*space, buf, 0);
          
        } // for

        } // showddl
    }

  if ((type == 2) &&
      (naTable->isHbaseCellTable() || naTable->isHbaseRowTable()) &&
      (NOT isView))
    {
      outputShortLine(*space, " ");

      outputShortLine(*space,"/* HBase DDL */");
      sprintf(buf,  "CREATE HBASE TABLE %s ( COLUMN FAMILY '%s') ",
              naTable->getTableName().getObjectName().data(),
              "#1");
      outputShortLine(*space, buf);
      
      // if this hbase table is registered in traf metadata, show that.
      if (naTable->isRegistered())
        {
          outputShortLine(*space, " ");

          sprintf(buf,  "REGISTER%sHBASE %s %s;",
                  (naTable->isInternalRegistered() ? " /*INTERNAL*/ " : " "),
                  "TABLE",
                  naTable->getTableName().getObjectName().data());
          
          NAString bufnas(buf);
          outputLongLine(*space, bufnas, 0);

          str_sprintf(buf, "/* ObjectUID = %ld */", objectUID);
          outputShortLine(*space, buf);
        }
      else if (isDetail)
        {
          // show a comment that this object should be registered
          outputShortLine(*space, " ");

          outputShortLine(*space, "-- Object is not registered in Trafodion Metadata.");
          outputShortLine(*space, "-- Register it using the next command:");
          sprintf(buf, "--   REGISTER HBASE TABLE %s;",
                  naTable->getTableName().getQualifiedNameAsAnsiString().data());
          NAString bufnas(buf);
          outputLongLine(*space, bufnas, 0);
        }
    }

  //display comments
  if (type == 2 && objectUID > 0)
    {
      enum ComObjectType objType = COM_BASE_TABLE_OBJECT;

      if (isView)
        {
          objType = COM_VIEW_OBJECT;
        }

      if (cmpSBD.switchCompiler())
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_COMMENTS);
          return -1;
        }
 
      ComTdbVirtObjCommentInfo objCommentInfo;
      if (cmpSBD.getSeabaseObjectComment(objectUID, objType, objCommentInfo, heap))
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_COMMENTS);
          cmpSBD.switchBackCompiler();
          return -1;
        }

      //display Table COMMENT statements
      if (objCommentInfo.objectComment != NULL)
        {
           //new line
           outputLine(*space, "", 0);

           sprintf(buf,  "COMMENT ON %s %s IS '%s' ;",
                   objType == COM_BASE_TABLE_OBJECT? "TABLE" : "VIEW",
                   tableName.data(),
                   objCommentInfo.objectComment);
           outputLine(*space, buf, 0);
        }

      //display Column COMMENT statements
      if (objCommentInfo.numColumnComment > 0 && objCommentInfo.columnCommentArray != NULL)
        {
          outputLine(*space, "", 0);
          for (int idx = 0; idx < objCommentInfo.numColumnComment; idx++)
            {
              sprintf(buf,  "COMMENT ON COLUMN %s.%s IS '%s' ;",
                       tableName.data(),
                       objCommentInfo.columnCommentArray[idx].columnName,
                       objCommentInfo.columnCommentArray[idx].columnComment);
               outputLine(*space, buf, 0);
            }
        }

      //display Index COMMENT statements
      if (objCommentInfo.numIndexComment > 0 && objCommentInfo.indexCommentArray != NULL)
        {
          outputLine(*space, "", 0);
          for (int idx = 0; idx < objCommentInfo.numIndexComment; idx++)
            {
              sprintf(buf,  "COMMENT ON INDEX %s IS '%s' ;",
                       objCommentInfo.indexCommentArray[idx].indexFullName,
                       objCommentInfo.indexCommentArray[idx].indexComment);
               outputLine(*space, buf, 0);
            }
        }

      //do a comment info memory clean
      NADELETEARRAY(objCommentInfo.columnCommentArray, objCommentInfo.numColumnComment, ComTdbVirtColumnCommentInfo, heap);
      NADELETEARRAY(objCommentInfo.indexCommentArray, objCommentInfo.numIndexComment, ComTdbVirtIndexCommentInfo, heap);

      cmpSBD.switchBackCompiler();
    }

  // If SHOWDDL and authorization is enabled, display GRANTS
  if (type == 2)
  {
    //objectUID = (int64_t)naTable->objectUid().get_value();
    if ((CmpCommon::context()->isAuthorizationEnabled()) && 
        displayPrivilegeGrants &&
        (objectUID > 0))
    {
      // now get the grant stmts
      std::string privMDLoc(ActiveSchemaDB()->getDefaults().getValue(SEABASE_CATALOG));
      privMDLoc += std::string(".\"") + 
                   std::string(SEABASE_PRIVMGR_SCHEMA) + 
                   std::string("\"");
      PrivMgrCommands privInterface(privMDLoc, CmpCommon::diags(),
                                    PrivMgr::PRIV_INITIALIZED);
      PrivMgrObjectInfo objectInfo(naTable);
      std::string privilegeText;
      if (cmpSBD.switchCompiler())
      {
        *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);
        return -1;
      }
 
      if (privInterface.describePrivileges(objectInfo, privilegeText))
      {
        outputShortLine(*space, " ");
        outputLine(*space, privilegeText.c_str(), 0);
      }

      cmpSBD.switchBackCompiler();
    }
  }

  outbuflen = space->getAllocatedSpaceSize();
  outbuf = new (heap) char[outbuflen];
  space->makeContiguous(outbuf, outbuflen);
  
  NADELETEBASIC(buf, heap);

  return 0;
}

short CmpDescribeSequence(
                               const CorrName  &dtName,
                               char* &outbuf,
                               ULng32 &outbuflen,
                               CollHeap *heap,
                               Space *inSpace)
{
  CorrName cn(dtName, heap);

  cn.setSpecialType(ExtendedQualName::SG_TABLE);

  // remove NATable for this table so latest values in the seq table could be read.
  ActiveSchemaDB()->getNATableDB()->removeNATable
    (cn, 
     ComQiScope::REMOVE_MINE_ONLY, COM_SEQUENCE_GENERATOR_OBJECT,
     FALSE, FALSE);

  ULng32 savedParserFlags = Get_SqlParser_Flags (0xFFFFFFFF);
  Set_SqlParser_Flags(ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  NATable *naTable = bindWA.getNATable(cn); 

  Assign_SqlParser_Flags (savedParserFlags);

  TableDesc *tdesc = NULL;
  if (naTable == NULL || bindWA.errStatus())
    return -1;

   // Verify user can perform commands
  if (!CmpDescribeIsAuthorized(SQLOperation::UNKNOWN, 
                               naTable->getPrivInfo(),
                               COM_SEQUENCE_GENERATOR_OBJECT))
    return -1;

  const SequenceGeneratorAttributes* sga = naTable->getSGAttributes();

  const NAString& seqName =
    cn.getQualifiedNameObj().getQualifiedNameAsAnsiString(TRUE);
 
  Space lSpace;
  
  Space * space;
  if (inSpace)
    space = inSpace;
  else
    space = &lSpace;

  char buf[1000];
  //  CMPASSERT(buf);

  outputShortLine(*space, " ");

  char seqType[40];
  if (sga->getSGSGType() == COM_INTERNAL_SG)
    {
      strcpy(seqType, "/* INTERNAL */ ");
    }
  else
    strcpy(seqType, " ");

  sprintf(buf,  "CREATE SEQUENCE %s %s",
          seqName.data(), seqType);
  outputShortLine(*space, buf);

  sga->display(space, NULL, FALSE);

  outputShortLine(*space, ";");

  char * sqlmxRegr = getenv("SQLMX_REGRESS");
  NABoolean displayPrivilegeGrants = TRUE;
  if (((CmpCommon::getDefault(SHOWDDL_DISPLAY_PRIVILEGE_GRANTS) == DF_SYSTEM) && sqlmxRegr) ||
      (CmpCommon::getDefault(SHOWDDL_DISPLAY_PRIVILEGE_GRANTS) == DF_OFF))
    displayPrivilegeGrants = FALSE;

  int64_t objectUID = (int64_t)naTable->objectUid().get_value();
  CmpSeabaseDDL cmpSBD((NAHeap*)heap);

  //display comment
  if (objectUID > 0)
    {
      if (cmpSBD.switchCompiler())
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_COMMENTS);
          return -1;
        }
 
      ComTdbVirtObjCommentInfo objCommentInfo;
      if (cmpSBD.getSeabaseObjectComment(objectUID, COM_SEQUENCE_GENERATOR_OBJECT, objCommentInfo, heap))
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_COMMENTS);
          cmpSBD.switchBackCompiler();
          return -1;
        }
 
      if (objCommentInfo.objectComment != NULL)
        {
          //new line
          outputLine(*space, "", 0);
 
          sprintf(buf,  "COMMENT ON SEQUENCE %s IS '%s' ;",
                        cn.getQualifiedNameObj().getQualifiedNameAsAnsiString(TRUE).data(),
                        objCommentInfo.objectComment);
          outputLine(*space, buf, 0);
        }
 
      cmpSBD.switchBackCompiler();
    }


  // If authorization enabled, display grant statements
  if (CmpCommon::context()->isAuthorizationEnabled() && displayPrivilegeGrants)
  {
    // now get the grant stmts
    NAString privMDLoc;
    CONCAT_CATSCH(privMDLoc, CmpSeabaseDDL::getSystemCatalogStatic(), SEABASE_MD_SCHEMA);
    NAString privMgrMDLoc;
    CONCAT_CATSCH(privMgrMDLoc, CmpSeabaseDDL::getSystemCatalogStatic(), SEABASE_PRIVMGR_SCHEMA);
    PrivMgrCommands privInterface(std::string(privMDLoc.data()), 
                                  std::string(privMgrMDLoc.data()),
                                  CmpCommon::diags());

    std::string privilegeText;
    PrivMgrObjectInfo objectInfo(naTable); 
    if (cmpSBD.switchCompiler())
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);
      return -1;
    }
 
    if (privInterface.describePrivileges(objectInfo, privilegeText))
    {
      outputShortLine(*space, " ");
      outputLine(*space, privilegeText.c_str(), 0);
    }

    cmpSBD.switchBackCompiler();
  }

  if (! inSpace)
    {
      outbuflen = space->getAllocatedSpaceSize();
      outbuf = new (heap) char[outbuflen];
      space->makeContiguous(outbuf, outbuflen);
    }

  return 0;
}

// ----------------------------------------------------------------------------
// Function: CmpDescribeIsAuthorized
//
// Determines if current user is authorized to perform operations
//
// parameters:  PrivMgrUserPrivs *privs - pointer to granted privileges
//              operation - SQLOperation to check in addition to SHOW 
//
// returns:  TRUE if authorized, FALSE otherwise
// ----------------------------------------------------------------------------
bool CmpDescribeIsAuthorized( 
   SQLOperation operation,
   PrivMgrUserPrivs *privs,
   ComObjectType objectType)
   
{

  if (!CmpCommon::context()->isAuthorizationEnabled())
    return true;

  if (ComUser::isRootUserID())
    return true;

  // check to see if user has select privilege
  if (privs)
  {
     switch (objectType)
     {
        case COM_LIBRARY_OBJECT:
        case COM_SEQUENCE_GENERATOR_OBJECT:
           if (privs->hasUsagePriv())
              return true;
           break;
        case COM_STORED_PROCEDURE_OBJECT:
        case COM_USER_DEFINED_ROUTINE_OBJECT:
           if (privs->hasExecutePriv())
              return true;
           break;
        case COM_PRIVATE_SCHEMA_OBJECT:
        case COM_SHARED_SCHEMA_OBJECT:
          break;
        case COM_VIEW_OBJECT:
        case COM_BASE_TABLE_OBJECT:
        default:
           if (privs->hasSelectPriv())
              return true;
     }
  }
  
  // check to see if user has SHOW component privilege
  std::string privMDLoc(ActiveSchemaDB()->getDefaults().getValue(SEABASE_CATALOG));
  privMDLoc += std::string(".\"") +
       std::string(SEABASE_PRIVMGR_SCHEMA) +
       std::string("\"");

  PrivMgrComponentPrivileges componentPrivileges(privMDLoc,CmpCommon::diags());
  if (componentPrivileges.hasSQLPriv(ComUser::getCurrentUser(),SQLOperation::SHOW,true))
    return true;

  if (operation != SQLOperation::UNKNOWN && 
      componentPrivileges.hasSQLPriv(ComUser::getCurrentUser(),operation,true))
    return true;
    
  *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
  
  return false;
  
}


// *****************************************************************************
// *                                                                           *
// * Function: CmpDescribeLibrary                                              *
// *                                                                           *
// *    Describes the DDL for a library object with normalized syntax.         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <corrName>                      const CorrName  &               In       *
// *    is a reference to correlation name for the library object.             *
// *                                                                           *
// *  <outbuf>                        char * &                        Out      *
// *    is a reference to a pointed to a character array.  The desribe output  *
// *    is stored here.                                                        *
// *                                                                           *
// *  <outbuflen>                     ULng32 &                        Out      *
// *    is the number of characters stored in outbuf.  Note, ULng32 is an      *
// *    unsigned 32-bit integer, aka uint32_t or unsigned int.  Not a long.    *
// *                                                                           *
// *  <heap>                          CollHeap *                      In       *
// *    is the heap to use for any dynamic allocations.                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// *  true: Describe text added to outbuf.                                     *
// * false: Desribe text not added. A CLI error is put into the diags area.    *
// *                                                                           *
// *****************************************************************************
bool CmpDescribeLibrary(
   const CorrName   & corrName,
   char           * & outbuf,
   ULng32           & outbuflen,
   CollHeap         * heap)
   
{

  CmpSeabaseDDL cmpSBD((NAHeap *)heap);
  CorrName cn(corrName, heap);

  cn.setSpecialType(ExtendedQualName::LIBRARY_TABLE);

  const NAString& libraryName = cn.getQualifiedNameObj().getQualifiedNameAsAnsiString(TRUE);


  ComObjectName libName(libraryName);
  NAString libCatNamePart = libName.getCatalogNamePartAsAnsiString();
  NAString libSchNamePart = libName.getSchemaNamePartAsAnsiString(TRUE);
  NAString libObjNamePart = libName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extLibraryName = libName.getExternalName(TRUE);

  ExeCliInterface cliInterface(heap);
        

  TrafDesc *tDesc = cmpSBD.getSeabaseLibraryDesc(libCatNamePart, 
                                                    libSchNamePart, 
                                                    libObjNamePart);
  if (tDesc == NULL)
  {
    *CmpCommon::diags() << DgSqlCode(-4082)
                        << DgTableName(extLibraryName.data());
    return 0;
  }


  Int64 libraryUID = tDesc->libraryDesc()->libraryUID;

   if (libraryUID <= 0) // does not exist
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_LIBRARY_DOES_NOT_EXIST) 
                          << DgTableName(extLibraryName.data());
      return false;
   }

// For libraries, we need to check if the user has the USAGE privilege;
// if so, they can perform SHOWDDL on this library.
   
NAString privMgrMDLoc;

   CONCAT_CATSCH(privMgrMDLoc,CmpSeabaseDDL::getSystemCatalogStatic(),SEABASE_PRIVMGR_SCHEMA);

PrivMgrCommands privInterface(privMgrMDLoc.data(),CmpCommon::diags());

   if (CmpCommon::context()->isAuthorizationEnabled())
   {
      PrivMgrUserPrivs privs;
      if (cmpSBD.switchCompiler())
      {
        *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);
        return -1  ;
      }

      PrivStatus retcode = privInterface.getPrivileges(libraryUID, COM_LIBRARY_OBJECT, 
                                                       ComUser::getCurrentUser(), 
                                                       privs);

      cmpSBD.switchBackCompiler();

      // Verify user can perform the SHOWDDL LIBRARY command on the specified library.
      if (!CmpDescribeIsAuthorized(SQLOperation::MANAGE_LIBRARY,&privs,
                                   COM_LIBRARY_OBJECT))
         return false;
   }
   
Space localSpace;
Space * space = &localSpace;

char buf[1000];

   sprintf(buf,"CREATE LIBRARY %s FILE '%s'",
           extLibraryName.data(), tDesc->libraryDesc()->libraryFilename);
           
   outputShortLine(*space,buf);
   outputShortLine(*space,";");

   // Determine if privilege grants should be displayed
   NABoolean displayPrivilegeGrants = 
    ((CmpCommon::getDefault(SHOWDDL_DISPLAY_PRIVILEGE_GRANTS) == DF_OFF) ||
       ((CmpCommon::getDefault(SHOWDDL_DISPLAY_PRIVILEGE_GRANTS) == DF_SYSTEM)
           && getenv("SQLMX_REGRESS"))) ? FALSE : TRUE;

//display library comment
   if (libraryUID > 0)
   {
     if (cmpSBD.switchCompiler())
       {
         *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);
         return -1;
       }

     ComTdbVirtObjCommentInfo objCommentInfo;
     if (cmpSBD.getSeabaseObjectComment(libraryUID, COM_LIBRARY_OBJECT, objCommentInfo, heap))
       {
         *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_COMMENTS);
         cmpSBD.switchBackCompiler();
         return -1;
       }

     if (objCommentInfo.objectComment != NULL)
       {
         //new line
         outputLine(*space, "", 0);

         sprintf(buf,  "COMMENT ON LIBRARY %s IS '%s' ;",
                       cn.getQualifiedNameObj().getQualifiedNameAsAnsiString(TRUE).data(),
                       objCommentInfo.objectComment);
         outputLine(*space, buf, 0);

       }

     cmpSBD.switchBackCompiler();
   }

// If authorization is enabled, display grant statements for library
   if (CmpCommon::context()->isAuthorizationEnabled() && displayPrivilegeGrants)
   {
      // now get the grant stmts
      NAString trafMDLoc;
      
      CONCAT_CATSCH(trafMDLoc,CmpSeabaseDDL::getSystemCatalogStatic(),SEABASE_MD_SCHEMA);

      PrivMgrCommands privInterface(std::string(trafMDLoc.data()), 
                                    std::string(privMgrMDLoc.data()),
                                    CmpCommon::diags());

      std::string privilegeText;
      PrivMgrObjectInfo objectInfo (
        libraryUID, extLibraryName.data(),
        tDesc->libraryDesc()->libraryOwnerID,
        tDesc->libraryDesc()->librarySchemaOwnerID,
        COM_LIBRARY_OBJECT );
      if (cmpSBD.switchCompiler())
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);
          return -1;
        }
 
      if (privInterface.describePrivileges(objectInfo,privilegeText))
      {
         outputShortLine(*space," ");
         outputLine(*space,privilegeText.c_str(),0);
      }

      cmpSBD.switchBackCompiler();
   }

   outbuflen = space->getAllocatedSpaceSize();
   outbuf = new (heap) char[outbuflen];
   space->makeContiguous(outbuf, outbuflen);

   return true;

}
//************************ End of CmpDescribeLibrary ***************************


// Routine to support SHOWDDL [ PROCEDURE | FUNCTION | TABLE_MAPPING FUNCTION ]
// <routine-name>
short CmpDescribeRoutine (const CorrName   & cn,
                          char           * & outbuf,
                          ULng32           & outbuflen,
                          CollHeap         * heap)
{

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  NARoutine *routine = bindWA.getNARoutine(cn.getQualifiedNameObj());
  const NAString& rName =
    cn.getQualifiedNameObj().getQualifiedNameAsAnsiString(TRUE);
  if (routine == NULL || bindWA.errStatus())
  {
    *CmpCommon::diags() << DgSqlCode(-4082) 
                        << DgTableName(rName.data());
    return 0;
  }

   // Verify user can perform commands
  if (!CmpDescribeIsAuthorized(SQLOperation::UNKNOWN, 
                               routine->getPrivInfo(),
                               COM_USER_DEFINED_ROUTINE_OBJECT))
    return 0;


  NABoolean logFormat = 
    (CmpCommon::getDefault(SHOWDDL_DISPLAY_FORMAT) == DF_LOG);


  Space localSpace;
  Space * space = &localSpace;

  outputShortLine(*space," ");

  // Start populating our output buffer
  // buffer is reused for each line of output. Longets line will be the 
  // java signature for SPJs. Currently this has an upper limit of 8K.
  // We allocate another 2K bytes for the name and any small changes 
  // in size due to formatting.
  char * buf = new (STMTHEAP) char[10000];
  CMPASSERT(buf);

  NABoolean buildRETURNSclause = TRUE;
  switch (routine->getRoutineType())
  {
  case COM_SCALAR_UDF_TYPE:
    sprintf ( buf, "CREATE FUNCTION %s", rName.data() );
    break;
  case COM_TABLE_UDF_TYPE:
    sprintf ( buf, "CREATE TABLE_MAPPING FUNCTION %s", rName.data() );
    break;
  case COM_PROCEDURE_TYPE:
    sprintf ( buf, "CREATE PROCEDURE %s", rName.data() );
    buildRETURNSclause = FALSE;
    break;
  default:
    ComASSERT(FALSE);
  }
  if (logFormat)
      outputShortLine(*space,"--> UR ");
  outputShortLine(*space, buf);
  outputShortLine (*space, "  (" );

  // Format the parameters
  Int32 numParams = routine->getParamCount();
  Int32 firstRETURNSparamIndex = -1;
  const NAColumnArray &paramList = routine->getParams();
  for ( Int32 i=0; i < numParams; i++ )
  {
    NAColumn &param = *(paramList[i]);
    ComColumnDirection direction = param.getColumnMode ();

    if (buildRETURNSclause AND direction EQU COM_OUTPUT_COLUMN)
    {
      firstRETURNSparamIndex = i;
      i = numParams; // i.e., exit loop
      continue;
    }

    if (i == 0)
      strcpy(buf, "    ");
    else
      strcpy(buf, "  , ");

      switch ( direction )
      {
      case COM_INPUT_COLUMN:
        strcat ( buf, "IN " );
        break;
      case COM_OUTPUT_COLUMN:
        strcat ( buf, "OUT " );
        break;
      case COM_INOUT_COLUMN:
        strcat ( buf, "INOUT " );
        break;
      } // switch

      // Next step for this parameter is to print its name. If a heading
      // is defined we print that. Otherwise if a column name is defined
      // we print that. Otherwise it is an unnamed parameter and we do
      // nothing.
      const NAString &heading = param.getHeading ();
      const NAString &colName = param.getColName ();
      if (heading.compareTo("") != 0)
      {
        strcat(buf, ANSI_ID(heading));
        strcat(buf, " ");
      }
      else if (colName.compareTo("") != 0)
      {
        strcat(buf, ANSI_ID(colName));
        strcat(buf, " ");
      }

      char typeString[80];
      strcpy (typeString, (param.getType ())->getTypeSQLname (TRUE));
      if (routine->getLanguage() EQU COM_LANGUAGE_JAVA)
      {
    // Java does not support unsigned / signed as
        // it converts NUMERIC types to a Java BigDecimal.
        Int32 typeLen = static_cast<Int32>(strlen(typeString));
        if (typeLen >= 9 &&
            0 == strcmp ( &typeString[typeLen - 9], " UNSIGNED" ))
        {
          typeString[typeLen - 9] = '\0';
        }
        else if (typeLen >= 7 &&
                 0 == strcmp ( &typeString[typeLen - 7], " SIGNED" ))
        {
          typeString[typeLen - 7] = '\0';
        }
      } // if Language Java
      strcat ( buf, typeString );

      outputShortLine (*space, buf);
  } // for

  // add the closing ")" for the formal parameter list
  strcpy ( buf, "  )");
  outputShortLine (*space, buf);

  if (buildRETURNSclause AND firstRETURNSparamIndex > -1)
  {
    strcpy ( buf, "  RETURNS " );
    outputShortLine (*space, buf);
    outputShortLine (*space, "  (" );
    // Build RETURN[S] clause from param list
    for (Int32 i7 = firstRETURNSparamIndex; i7 < numParams; i7++)
    {
      NAColumn &param7 = *(paramList[i7]);
      ComColumnDirection direction7 = param7.getColumnMode ();

      if (i7 EQU firstRETURNSparamIndex)
        strcpy(buf, "    ");
      else
        strcpy(buf, "  , ");

      switch ( direction7 )
      {
      case COM_INPUT_COLUMN:
        strcat ( buf, "IN " );     // not supposed to happen
        break;
      case COM_OUTPUT_COLUMN:
        strcat ( buf, "OUT " );
        break;
      case COM_INOUT_COLUMN:
        strcat ( buf, "INOUT " );  // not supposed to happen
        break;
      } // switch

      // Next step for this parameter is to print its name. If a heading
      // is defined we print that. Otherwise if a column name is defined
      // we print that. Otherwise it is an unnamed parameter and we do
      // nothing.
      const NAString &heading7 = param7.getHeading ();
      const NAString &colName7 = param7.getColName ();
      if (heading7.compareTo("") != 0)
      {
        strcat(buf, ANSI_ID(heading7));
        strcat(buf, " ");
      }
      else if (colName7.compareTo("") != 0)
      {
        strcat(buf, ANSI_ID(colName7));
        strcat(buf, " ");
      }

      char typeString7[80];
      strcpy (typeString7, (param7.getType ())->getTypeSQLname (TRUE));
      if (routine->getLanguage() EQU COM_LANGUAGE_JAVA)
      {
        // Java does not support UNSIGNED and SIGNED as
        // it converts NUMERIC types to a Java BigDecimal.
        Int32 typeLen7 = static_cast<Int32>(strlen(typeString7));
        if (typeLen7 >= 9 AND 0 EQU
            strcmp ( &typeString7[typeLen7 - 9], " UNSIGNED" ))
        {
          typeString7[typeLen7 - 9] = '\0';
        }
        else if (typeLen7 >= 7 AND 0 EQU
                 strcmp ( &typeString7[typeLen7 - 7], " SIGNED" ))
        {
          typeString7[typeLen7 - 7] = '\0';
        }
      } // if Language Java
      strcat (buf, typeString7);

      outputShortLine (*space, buf);
    } // for

    strcpy ( buf, "  )");
    outputShortLine (*space, buf);

  } // if (buildRETURNSclause AND firstRETURNSparamIndex > -1)

  // EXTERNAL NAME clause
  NABoolean isProcedure = 
    (routine->getRoutineType() EQU COM_PROCEDURE_TYPE ? TRUE : FALSE);

  if (isProcedure)
  {
    strcpy ( buf, "  EXTERNAL NAME '" );

    NAString exNamePrefixInStrLitFormat44;
    NAString exNamePrefixInInternalFormat;
    exNamePrefixInInternalFormat += routine->getFile();
    exNamePrefixInInternalFormat += ".";
    exNamePrefixInInternalFormat += routine->getExternalName();
    ToQuotedString ( exNamePrefixInStrLitFormat44  // out - NAString &
                   , exNamePrefixInInternalFormat  // in  - const NAString &
                   , FALSE                         // in  - NABoolean encloseInQuotes
                   );
    strcat ( buf, exNamePrefixInStrLitFormat44.data() );

    // Get the actual signature size

    LmJavaSignature lmSig(routine->getSignature().data(), heap);
    Int32 sigSize = lmSig.getUnpackedSignatureSize();

    if ( 0 == sigSize)
    {
      *CmpCommon::diags() << DgSqlCode(-11223) 
                          << DgString0(". Unable to determine signature size.");
      return 0;
    }

    char *sigBuf = new (STMTHEAP) char[sigSize + 1];

    if (lmSig.unpackSignature(sigBuf) == -1)
    {
      *CmpCommon::diags() << DgSqlCode(-11223) 
                          << DgString0(". Unable to determine signature."); 
      return 0;
    }

    sigBuf[sigSize] = '\0';

    strcat ( buf, " " );
    strcat ( buf, sigBuf );
    strcat ( buf, "'" );
    outputShortLine (*space, buf);

  }
  else // this is not a procedure
  {
      strcpy ( buf, "  EXTERNAL NAME " );

      NAString externalNameInStrLitFormat(STMTHEAP);
      ToQuotedString ( externalNameInStrLitFormat          // out - NAString &
                     , routine->getExternalName()  // in  - const NAString &
                     , TRUE               // in  - NABoolean encloseInQuotes
                     );
      strcat ( buf, externalNameInStrLitFormat.data() );
      outputShortLine (*space, buf);
  }

  // LIBRARY clause
  NAString libName(routine->getLibrarySqlName().getExternalName()); 
  if ((libName.length() > 0) && (libName.data()[0] != ' '))
    {
      strcpy ( buf, "  LIBRARY " );
      strcat ( buf, libName.data());
      outputShortLine (*space, buf);    
    }   
 
  // EXTERNAL SECURITY clause
  if (isProcedure)
  {
    switch ( routine->getExternalSecurity ())
    {
    case COM_ROUTINE_EXTERNAL_SECURITY_INVOKER:
      outputShortLine (*space, "  EXTERNAL SECURITY INVOKER");
      break;
    case COM_ROUTINE_EXTERNAL_SECURITY_DEFINER:
      outputShortLine (*space, "  EXTERNAL SECURITY DEFINER");
      break;
    default:
      ComASSERT(FALSE);
      break;
    }
  }



  switch ( routine->getLanguage ())
  {
  case COM_LANGUAGE_JAVA:
    outputShortLine (*space, "  LANGUAGE JAVA");
    break;
  case COM_LANGUAGE_C:
    outputShortLine (*space, "  LANGUAGE C");
    break;
  case COM_LANGUAGE_SQL:
    outputShortLine (*space, "  LANGUAGE SQL");
    break;
  case COM_LANGUAGE_CPP:
    outputShortLine (*space, "  LANGUAGE CPP");
    break;
  default:
    ComASSERT(FALSE);
    break;
  }

  switch ( routine->getParamStyle ())
  {
  case COM_STYLE_GENERAL:
    outputShortLine (*space, "  PARAMETER STYLE GENERAL");
    break;
  case COM_STYLE_JAVA_CALL:
    outputShortLine (*space, "  PARAMETER STYLE JAVA");
    break;
  case COM_STYLE_SQL:
    outputShortLine (*space, "  PARAMETER STYLE SQL");
    break;
  case COM_STYLE_SQLROW:
    outputShortLine (*space, "  PARAMETER STYLE SQLROW");
    break;
  case COM_STYLE_JAVA_OBJ:
  case COM_STYLE_CPP_OBJ:
  case COM_STYLE_SQLROW_TM:
    break;
  default:
    ComASSERT(FALSE);
    break;
  }

  switch (routine->getSqlAccess ())
  {
  case COM_NO_SQL:
    outputShortLine (*space, "  NO SQL");
    break;
  case COM_MODIFIES_SQL:
    outputShortLine (*space, "  MODIFIES SQL DATA");
    break;
  case COM_CONTAINS_SQL:
    outputShortLine (*space, "  CONTAINS SQL");
    break;
  case COM_READS_SQL:
    outputShortLine (*space, "  READS SQL DATA");
    break;
  default:
    // Unknown SQL access mode
    ComASSERT(FALSE);
    break;
  } // switch

  if (isProcedure)
  {
    // max result sets
    strcpy ( buf, "  DYNAMIC RESULT SETS " );
    sprintf ( &buf[strlen (buf)], "%d", (Int32) routine->getMaxResults () );
    outputShortLine (*space, buf);

    // transaction required clause needs to be shown in the output from M9
    switch ( routine->getTxAttrs () )
    {
    case COM_NO_TRANSACTION_REQUIRED:
      strcpy ( buf, "  NO TRANSACTION REQUIRED" );
      break;
    case COM_TRANSACTION_REQUIRED:
      strcpy ( buf, "  TRANSACTION REQUIRED" );
      break;
    default:
      ComASSERT(FALSE);
      break;
    }
    outputShortLine (*space, buf);
  }
  else
  {
    if (routine->getLanguage() == COM_LANGUAGE_C)
      {
        // final call
        if (routine->isFinalCall()) // same as isExtraCall()
          strcpy ( buf, "  FINAL CALL" );
        else
          strcpy ( buf, "  NO FINAL CALL" );
        outputShortLine (*space, buf);

        // state area size
        if ( routine->getStateAreaSize () > 0 )
          {
            strcpy ( buf, "  STATE AREA SIZE " );
            sprintf ( &buf[strlen (buf)], "%d", routine->getStateAreaSize ());
          }
        else
          {
            strcpy ( buf, "  NO STATE AREA" );
          }
        outputShortLine (*space, buf);
      }
  }

  if (routine->isScalarUDF())
  {
    if ( routine->getParallelism() == "AP")
      strcpy ( buf, "  ALLOW ANY PARALLELISM" );
    else
      strcpy ( buf, "  NO PARALLELISM" );
    outputShortLine (*space, buf);

    // Deterministic clause
    if ( routine->isDeterministic ())
      strcpy ( buf, "  DETERMINISTIC" );
    else
      strcpy ( buf, "  NOT DETERMINISTIC" );
    outputShortLine (*space, buf);
  }

  if (!isProcedure)
  {
    switch ( routine->getExecutionMode() )
    {
    case COM_ROUTINE_FAST_EXECUTION:
      strcpy ( buf, "  FAST EXECUTION MODE" );
      break;
    case COM_ROUTINE_SAFE_EXECUTION:
      strcpy ( buf, "  SAFE EXECUTION MODE" );
      break;
    default:
      ComASSERT(FALSE);
      break;
    }
    outputShortLine (*space, buf);
  }

  if (isProcedure)
  {
    if ( routine->isIsolate ())
    {
      strcpy ( buf, "  ISOLATE" );
    }
    else
    {
      strcpy ( buf, "  NO ISOLATE" );
    }
    outputShortLine (*space, buf);
  } // routine type is SPJ

  outputShortLine (*space, "  ;");

  CmpSeabaseDDL cmpSBD((NAHeap*)heap);

  char * sqlmxRegr = getenv("SQLMX_REGRESS");
  NABoolean displayPrivilegeGrants = TRUE;
  if (((CmpCommon::getDefault(SHOWDDL_DISPLAY_PRIVILEGE_GRANTS) == DF_SYSTEM) && sqlmxRegr) ||
       (CmpCommon::getDefault(SHOWDDL_DISPLAY_PRIVILEGE_GRANTS) == DF_OFF))
    displayPrivilegeGrants = FALSE;

  //display comment of routine
  Int64 routineUID = routine->getRoutineID();
  if ( routineUID > 0)
    {
      if (cmpSBD.switchCompiler())
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);
         return -1;
      }

      ComTdbVirtObjCommentInfo objCommentInfo;
      if (cmpSBD.getSeabaseObjectComment(routineUID, COM_USER_DEFINED_ROUTINE_OBJECT, objCommentInfo, heap))
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_COMMENTS);
          cmpSBD.switchBackCompiler();
          return -1;
        }

     
      if (objCommentInfo.objectComment != NULL)
        {
          //new line
          outputLine(*space, "", 0);
     
          sprintf(buf,  "COMMENT ON %s %s IS '%s' ;",
                        routine->getRoutineType() == COM_PROCEDURE_TYPE ? "PROCEDURE" : "FUNCTION",
                        cn.getQualifiedNameObj().getQualifiedNameAsAnsiString(TRUE).data(),
                        objCommentInfo.objectComment);
          outputLine(*space, buf, 0);
        }
     
      cmpSBD.switchBackCompiler();

    }


  // If authorization enabled, display grant statements
  if (CmpCommon::context()->isAuthorizationEnabled() && displayPrivilegeGrants)
  {
    // now get the grant stmts
    int64_t objectUID = (int64_t)routine->getRoutineID();
    NAString privMDLoc;
    CONCAT_CATSCH(privMDLoc, CmpSeabaseDDL::getSystemCatalogStatic(), SEABASE_MD_SCHEMA);
    NAString privMgrMDLoc;
    CONCAT_CATSCH(privMgrMDLoc, CmpSeabaseDDL::getSystemCatalogStatic(), SEABASE_PRIVMGR_SCHEMA);
    PrivMgrCommands privInterface(std::string(privMDLoc.data()), 
                                  std::string(privMgrMDLoc.data()),
                                  CmpCommon::diags());

    std::string objectName(rName);
    std::string privilegeText;
    PrivMgrObjectInfo objectInfo(
      objectUID, objectName,
      (int32_t)routine->getObjectOwner(),
      (int32_t)routine->getSchemaOwner(),
      COM_USER_DEFINED_ROUTINE_OBJECT);

    if (cmpSBD.switchCompiler())
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);
      return -1;
    }
 
    if (privInterface.describePrivileges(objectInfo, privilegeText))
    {
      outputShortLine(*space, " ");
      outputLine(*space, privilegeText.c_str(), 0);
    }

    cmpSBD.switchBackCompiler();
  }

  outbuflen = space->getAllocatedSpaceSize();
  outbuf = new (heap) char[outbuflen];
  space->makeContiguous(outbuf, outbuflen);

  
  NADELETEBASIC(buf, CmpCommon::statementHeap());  
  return 1;
} // CmpDescribeShowddlProcedure
