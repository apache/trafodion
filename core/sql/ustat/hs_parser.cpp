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
* File:         hs_parser.C
* Description:  Functions used by parser.
* Created:      09/25/96
* Language:     C++
*
*
*****************************************************************************/
#define SQLPARSERGLOBALS_NADEFAULTS            // must be first
#define HS_FILE "hs_parser"

#include <time.h>
#include <sys/time.h>
#include "CmpCommon.h"
#include "CmpContext.h"
#include "ComAnsiNamePart.h"
#include "SchemaDB.h"


//IMPORTANT: The following header is required or else the main sqlparser will be invoked
#include "hs_parser_defs.h"   
#include "hs_parser.h"
#include "hs_globals.h"
#include "hs_cli.h"
#include "hs_la.h"
#include "SqlParserGlobals.h"                   // must be last #include

#include "ComSchemaName.h" // for ComSchemaName

extern Int32 yyparse(void*);
extern void HSFuncResetLexer(void*);
extern void init_scanner (void* &);
extern void destroy_scanner(void* &scanner);
Lng32 AddSingleColumn(const Lng32 colNumber);

    
// -----------------------------------------------------------------------
// Invoke yyparse, set hsGlobal structure for each column group.
// -----------------------------------------------------------------------
Lng32 HSFuncParseStmt()
  {
    HSGlobalsClass *hs_globals = GetHSContext();
    Lng32 retcode;
    HSColGroupStruct *mgroup = NULL;

    void* scanner;
    init_scanner (scanner);
    HSFuncResetLexer(scanner);
    retcode = yyparse(scanner);
    HSHandleError(retcode);
    destroy_scanner (scanner);
    
    // The parser does not always return immediately following an error, so that
    // it can report as many errors as possible. This can result in a nonzero
    // return code being overwritten. To detect this case, we check the diagnostics
    // area and return with the appropriate sqlcode if necessary.
    retcode = hs_globals->getRetcodeFromDiags();
    HSHandleError(retcode);
    
    hs_globals->parserError = HSGlobalsClass::ERROR_SEMANTICS;

    //  We are done here if it is the showstats command
    if (hs_globals->optFlags & SHOWSTATS_OPT) 
        return 0;

    // Automatically generate single-column histograms for all multi-column
    // histograms requested. This is required to calculate UEC counts for
    // multi-column histograms.
    // However, do not generate them when the CLEAR option is requested, unless
    // ON EVERY COLUMN or ON EVERY KEY is also specified.
    if (NOT (hs_globals->optFlags & CLEAR_OPT) ||
        hs_globals->optFlags & EVERYCOL_OPT    ||
        hs_globals->optFlags & EVERYKEY_OPT)
      {
        mgroup = hs_globals->multiGroup;
        while (mgroup != NULL)
          {
            for (Int32 i=0; i<mgroup->colCount; i++)
              {
                HSColumnStruct &col = mgroup->colSet[i];
                if (NOT ColumnExists(col.colnum))
                  {
                    retcode = AddSingleColumn(col.colnum);
                    HSHandleError(retcode);
                  }
              }
            mgroup = mgroup->next;
          }
      }

    // ----------------------------------------------------------------------
    // Construct statistics time in the format: YYYY-MM-DD:HH:MM:SS.
    // We use GMT time.
    // ----------------------------------------------------------------------
    time_t t;
    time(&t);
    char pt[30];

    strftime(pt, 30, "%Y-%m-%d:%H:%M:%S", gmtime(&t));
    *hs_globals->statstime = pt;

    // Also store a numerical timestamp with 10 digits MMDDHHMMSS.
    hs_globals->statsTimeInt = (pt[5] - '0')  * 1000000000 +
                            (pt[6] - '0')  * 100000000 +
                            (pt[8] - '0')  * 10000000 +
                            (pt[9] - '0')  * 1000000 +
                            (pt[11] - '0') * 100000 +
                            (pt[12] - '0') * 10000 +
                            (pt[14] - '0') * 1000 +
                            (pt[15] - '0') * 100 +
                            (pt[17] - '0') * 10 +
                            (pt[18] - '0');
    return 0;
  }
 
// -----------------------------------------------------------------------
// Construct a fully qualified table name.
// -----------------------------------------------------------------------
Lng32 AddTableName( const hs_table_type type
                 , const char *table
                 , const char *schema
                 , const char *catalog
                 )
  {
    HSGlobalsClass *hs_globals = GetHSContext();
    
    NAString catName, schName, objName;
    NAString extName;
    NAString defaultCat, defaultSch;
    NAString userLocation;
    Lng32 retcode = 0;

    hs_globals->tableType = type;
    HSLogMan *LM = HSLogMan::Instance();
  
    // SET MPLOC is converted to CQD (setting values for default
    // attributes MP_SYSTEM, MP_VOLUME, MP_SUBVOLUME).  It does not
    // update the global MPLOC value stored in SqlParser_MPLOC.  The
    // following updates the global MPLOC value to be consistent with
    // the default attribute values set by SET MPLOC/CQD.
    ActiveSchemaDB()->getDefaults().getSqlParser_NADefaults();

    if (type == GUARDIAN_TABLE)
      {
        if (*table == '$')
          { // Qualify with system name.
            extName  = SqlParser_MPLOC.getSystemName();
            extName += ".";
            extName += table;
          }
        else
          extName = table;
        hs_globals->tableFormat = SQLMP;
      }
    else
      {
        if (catalog)
          catName = catalog;
        else
          {
            catName = ActiveSchemaDB()->getDefaultSchema().getCatalogName();
          }
  
        if (schema) 
          schName = schema;
        else
          {
            schName = ActiveSchemaDB()->getDefaultSchema().getSchemaName();
          }
  
        objName = table;
        extName = catName + "." + schName + "." + objName;
      }
  
    hs_globals->objDef = NULL;
    
    // Search in volatile schema first. If not found, search in regular cat/sch.
    if ((CmpCommon::context()->sqlSession()->volatileSchemaInUse()) &&
        (type != GUARDIAN_TABLE) &&
        (! catalog))
      {
        // search using the volatile schema name.
        NAString &volCatName = CmpCommon::context()->sqlSession()->volatileCatalogName();
        NAString &volSchName = CmpCommon::context()->sqlSession()->volatileSchemaName();
        NAString volObjName = table;
	
        
        ComObjectName volIntName(volCatName, volSchName, volObjName,
                                 COM_UNKNOWN_NAME, 
                                 ComAnsiNamePart::INTERNAL_FORMAT);
        if (NOT volIntName.isValid())
          {
            LM->Log("***[ERROR] Unable to create an ObjectClass");
            HSFuncMergeDiags(-UERR_OBJECT_INACCESSIBLE, extName);
            retcode = -1;
            HSHandleError(retcode);
          }
	
        if (LM->LogNeeded())
          {
            LM->Log("Searching in volatile schema, since catalog not specified.\n");
            sprintf(LM->msg, "Checking volatile name (volIntName) %s.%s.%s\n",
                    volIntName.getCatalogNamePart().getInternalName().data(),
                    volIntName.getSchemaNamePart().getInternalName().data(),
                    volIntName.getObjectNamePart().getInternalName().data());
            LM->Log(LM->msg);
          }

        hs_globals->objDef = HSTableDef::create(STMTHEAP,
                                                volIntName,
                                                hs_globals->tableType,
                                                hs_globals->nameSpace);

      	if (NOT hs_globals->objDef->objExists(hs_globals->isUpdatestatsStmt))
          {
            // now look into the regular schema
            delete hs_globals->objDef;
            hs_globals->objDef = NULL;
          }
        else 
          {
            // if schema name was specified, validate that it is the
            // current username.
            if (schema)
              {
                QualifiedName qn(volObjName, schName);
                if (NOT CmpCommon::context()->sqlSession()->validateVolatileQualifiedName(qn))
                  {
                    // table was found in the volatile schema but it is
                    // not a valid volatile name.
                    // Look for it in regular schema.
                    // error info was moved to CmpCommon::diags. Clear it.
                    CmpCommon::diags()->clear();
                    delete hs_globals->objDef;
                    hs_globals->objDef = NULL;
                  }
              }
          }
      }      
    
    if (hs_globals->objDef == NULL)
      {
	ComObjectName intName(catName, schName, objName,
			      COM_UNKNOWN_NAME, 
			      ComAnsiNamePart::INTERNAL_FORMAT);
	if (NOT intName.isValid())
	  {
	    LM->Log("***[ERROR] Unable to create an ObjectClass");
	    HSFuncMergeDiags(-UERR_OBJECT_INACCESSIBLE, extName);
	    retcode = -1;
	    HSHandleError(retcode);
	  }
	

       hs_globals->objDef = HSTableDef::create(STMTHEAP,
                                          intName,
                                          hs_globals->tableType,
                                          hs_globals->nameSpace);

       // just do this check once since it side effects diags (not to mention
       // multiple calls do multiple metadata lookups in failure scenarios)
       NABoolean objExists = hs_globals->objDef->objExists(hs_globals->isUpdatestatsStmt);

       // try public schema if an object is not qualified and not found
       if ((NOT schema) && 
           (NOT objExists))
       {
          NAString pubSch = ActiveSchemaDB()->getDefaults().getValue(PUBLIC_SCHEMA_NAME);
          ComSchemaName pubSchema(pubSch);
          if (NOT pubSchema.getSchemaNamePart().isEmpty())
          {
            NAString pubSchName = pubSchema.getSchemaNamePart().getInternalName();
            NAString pubCatName = (pubSchema.getCatalogNamePart().isEmpty() ? 
              catName:pubSchema.getCatalogNamePart().getInternalName());
      	    ComObjectName pubIntName(pubCatName, pubSchName, objName,
                                     COM_UNKNOWN_NAME, ComAnsiNamePart::INTERNAL_FORMAT);
	      
            if (pubIntName.isValid())
	     {
                HSTableDef *pubObjDef = HSTableDef::create(STMTHEAP,
                                                           pubIntName, 
                                                           hs_globals->tableType,
                                                           hs_globals->nameSpace);

                if (pubObjDef->objExists(hs_globals->isUpdatestatsStmt))
                {
                  hs_globals->objDef = pubObjDef;
                  objExists = TRUE;
                }
	     }
          }
       }

      if (NOT objExists)
      {
         ComDiagsArea & diagsArea = GetHSContext()->diagsArea;
         if (!diagsArea.findCondition(-UERR_OBJECT_INACCESSIBLE))
           {
             // only add this error in if objExists check didn't already
             // (it's annoying to have the same error repeated)
             HSFuncMergeDiags(-UERR_OBJECT_INACCESSIBLE, extName);
           }
         retcode = -1;
         HSHandleError(retcode);
      }
    }

    //10-040123-2660 We only support tables. We do not allow views.
    // Tables cannot be metadata tables.
    if (((hs_globals->objDef->getObjectType() != COM_BASE_TABLE_OBJECT) &&
         (hs_globals->objDef->getObjectType() != COM_MV_OBJECT)) ||
        (hs_globals->objDef->getNATable()->isSeabaseMDTable()) ||
        (hs_globals->objDef->getNATable()->isSeabasePrivSchemaTable())) 
      {
        HSFuncMergeDiags(-UERR_INVALID_OBJECT, extName);
        retcode = -1;
        HSHandleError(retcode);
      }
    retcode = hs_globals->objDef->getColumnNames();
    HSFuncExecQuery("CONTROL QUERY DEFAULT DISPLAY_DIVISION_BY_COLUMNS RESET");
    HSHandleError(retcode);

    hs_globals->tableFormat = hs_globals->objDef->getObjectFormat();
    *hs_globals->catSch     = hs_globals->objDef->getPrimaryLoc(HSTableDef::EXTERNAL_FORMAT);
    *hs_globals->user_table = hs_globals->objDef->getObjectFullName();
    hs_globals->tableFormat = hs_globals->objDef->getObjectFormat();
    hs_globals->isHbaseTable = HSGlobalsClass::isHbaseCat(catName);
    hs_globals->isHiveTable = HSGlobalsClass::isHiveCat(catName);

    if (hs_globals->tableFormat == SQLMX)
      {
        // Determine the schema version for this MX table.
        if (LM->LogNeeded())
         {
            sprintf(LM->msg, "\nCHECK SCHEMA VERSION FOR TABLE: %s\n", 
              hs_globals->user_table->data());
            LM->Log(LM->msg);
         }
        HSGlobalsClass::schemaVersion = getTableSchemaVersion(hs_globals->user_table->data());
        if (HSGlobalsClass::schemaVersion == COM_VERS_UNKNOWN)
        {
           HSFuncMergeDiags(-UERR_INTERNAL_ERROR, "GET_SCHEMA_VERSION");
           return -1;
        }


        if (HSGlobalsClass::schemaVersion >= COM_VERS_2300) 
          HSGlobalsClass::autoInterval = CmpCommon::getDefaultLong(USTAT_AUTOMATION_INTERVAL);
        if (LM->LogNeeded())
         {
            sprintf(LM->msg, "\nUpdateStats: TABLE: %s; SCHEMA VERSION: %d; AUTOMATION INTERVAL: %d\n", 
                  hs_globals->user_table->data(), 
                  HSGlobalsClass::schemaVersion, HSGlobalsClass::autoInterval);
            LM->Log(LM->msg);
         }

        NAString catName(hs_globals->objDef->getCatName());

        *hs_globals->hstogram_table = getHistogramsTableLocation(hs_globals->catSch->data(), FALSE);

        *hs_globals->hsintval_table = getHistogramsTableLocation(hs_globals->catSch->data(), FALSE);

        *hs_globals->hsperssamp_table = getHistogramsTableLocation(hs_globals->catSch->data(), FALSE);

        NABoolean isHbaseOrHive = HSGlobalsClass::isHbaseCat(catName) ||
                                  HSGlobalsClass::isHiveCat(catName);

        if (isHbaseOrHive) {
          hs_globals->hstogram_table->append(".").append(HBASE_HIST_NAME);
          hs_globals->hsintval_table->append(".").append(HBASE_HISTINT_NAME);
          hs_globals->hsperssamp_table->append(".").append(HBASE_PERS_SAMP_NAME);
        } else {
          hs_globals->hstogram_table->append(".HISTOGRAMS");
          hs_globals->hsintval_table->append(".HISTOGRAM_INTERVALS");
          hs_globals->hsperssamp_table->append(".PERSISTENT_SAMPLES");
        }
      }
    else
      {
        *hs_globals->hstogram_table = hs_globals->objDef->getCatalogLoc(HSTableDef::EXTERNAL_FORMAT);
        hs_globals->hstogram_table->append(".HISTOGRM");
        
        *hs_globals->hsintval_table = hs_globals->objDef->getCatalogLoc(HSTableDef::EXTERNAL_FORMAT);
        hs_globals->hsintval_table->append(".HISTINTS");        

        // RESET CQDS:
        HSFuncExecQuery("CONTROL QUERY DEFAULT POS RESET");
        HSFuncExecQuery("CONTROL QUERY DEFAULT POS_NUM_OF_PARTNS RESET");
      }
  
    return 0;
  }


NABoolean ColumnExists(const Lng32 colNumber)
  {
    HSGlobalsClass *hs_globals = GetHSContext();
    HSColGroupStruct *group = hs_globals->singleGroup;
    NABoolean found = FALSE;

    while (!found && group != NULL)
      {
        HSColumnStruct   &column = group->colSet[0];
        if (group->colCount == 1 &&
            column.colnum == colNumber)
          found = TRUE;
        group = group->next;
      }

    return found;
  }

HSColGroupStruct* AddSingleColumn(const Lng32 colNumber, HSColGroupStruct*& groupStart, NABoolean prepend = TRUE)
  {
    HSGlobalsClass *hs_globals = GetHSContext();
    HSColGroupStruct *newGroup = new(STMTHEAP) HSColGroupStruct;
    HSColumnStruct   newColumn = HSColumnStruct(hs_globals->objDef->getColInfo(colNumber));

    bool isOverSized = DFS2REC::isAnyCharacter(newColumn.datatype) &&
              (newColumn.length > hs_globals->maxCharColumnLengthInBytes);
    if (isOverSized)
      {
        hs_globals->hasOversizedColumns = TRUE;
      }

    newColumn.colnum  = colNumber;
    newGroup->colSet.insert((const HSColumnStruct) newColumn);
    newGroup->colCount = 1;
    *newGroup->colNames = ToAnsiIdentifier(newColumn.colname->data());
          // Note: ToAnsiIdentifier() determines whether a name needs to be delimited
          // with quotes.  This function works for shift-JIS but may not work for other
          // non-ISO88591 char sets such as Korean, BIG5, GB2312, and GB18030, ...
    if (groupStart == NULL)    // first group entry
      {
        groupStart = newGroup;
      }
    else                  // append to front of list
      {
        if ( prepend ) {
          newGroup->next = groupStart;
          groupStart->prev = newGroup;
          groupStart = newGroup;
        } else {
          HSColGroupStruct* tailGroup = groupStart;
          while ( tailGroup->next )
             tailGroup = tailGroup->next;

          tailGroup->next = newGroup;
          newGroup->prev = tailGroup;
          newGroup->next = NULL;
        }
      }

    return newGroup;
  }

Lng32 AddSingleColumn(const Lng32 colNumber)
  {
    HSGlobalsClass *hs_globals = GetHSContext();
    AddSingleColumn(colNumber, hs_globals->singleGroup);
    hs_globals->groupCount++;
    hs_globals->singleGroupCount++;
    return 0;
  }

Lng32 AddColumnSet(HSColSet &colSet)
  {
    HSGlobalsClass *hs_globals = GetHSContext();
    Lng32 retcode = 0;
    HSColGroupStruct *newGroup  = NULL;
    Lng32 colCount = 0;
    NABoolean badColList = FALSE;
    NAString colNames = "";
    NAString temp;
    HSLogMan *LM = HSLogMan::Instance();
    Int32 numCols = colSet.entries();
    Int32 i;

    if (numCols < 2)          // Must have at least 2 columns in multi-col set.
      {
        if (LM->LogNeeded())
          {
            sprintf(LM->msg, "\t\tIgnoring Column Group with single unique entry (%s)", 
                             colSet[0].colname->data());
            LM->Log(LM->msg);
          }
        return HS_WARNING;
      }

    Lng32 maxNumCols =
      (ActiveSchemaDB()->getDefaults()).getAsLong(USTAT_MULTI_COLUMN_LIMIT);
    if (numCols > maxNumCols)
      {
        // parsing reversed the column order; for error messages, we
        // unreverse it to make more sense to the user
        NAString columnNames(colSet[numCols-1].colname->data()); 
        for (i = numCols-2; i >= 0; i--)
          {
            columnNames += ", ";
            columnNames += colSet[i].colname->data();
          }

        if (LM->LogNeeded())
          {
            *LM << "\t\tToo many columns in multi-column histogram (" << columnNames.data() 
                << "); limit is " << maxNumCols << " (CQD USTAT_MULTI_COLUMN_LIMIT)";
            LM->FlushToLog();
          }
        char temp[20];
        sprintf(temp,"%d",maxNumCols);
        HSFuncMergeDiags(- UERR_MULTI_COLUMN_LIMIT_EXCEEDED, temp, columnNames.data());
        retcode = -1;
        HSHandleError(retcode);
      }     

    for (i=0; i<numCols; i++)          // update column numbers, position & NO DUPLICATES
      {
        HSColumnStruct &col = colSet[i];
        temp = " ";
        temp += ToAnsiIdentifier(col.colname->data());
          // Note: ToAnsiIdentifier() determines whether a name needs to be delimited
          // with quotes.  This function works for shift-JIS but may not work for other
          // non-ISO88591 char sets such as Korean, BIG5, GB2312, and GB18030, ...
        temp += ",";

        if (colNames.contains(temp))
          badColList = TRUE;
        else
          {
            col.colnum  = hs_globals->objDef->getColNum((char*)col.colname->data());
            if (col.colnum < 0)
              {
                retcode = -1;
                HSHandleError(retcode);
              }
            col.position = colCount;
            colCount++;
          }
        colNames += temp;
      }
    colNames.remove(0,1);    // remove first blank
    colNames.remove(colNames.length() - 1);    // remove last comma

    if (badColList)          // column list contains repeating columns
      {
        if (LM->LogNeeded())
          {
            *LM << "\t\tNon-Unique Column Group (" << colNames.data() <<")";
            LM->FlushToLog();
          }
        HSFuncMergeDiags(- UERR_COLUMNLIST_NOT_UNIQUE, colNames.data());
        retcode = -1;
        HSHandleError(retcode);
      }
    else
      {
        if (GroupExists(colSet))
          {
            if (LM->LogNeeded())
              {
                sprintf(LM->msg, "\t\tDuplicate Column Group (%s) has been ignored.", colNames.data());
                LM->Log(LM->msg);
              }
            retcode = HS_WARNING;
          }
        else
          {
            newGroup  = new(STMTHEAP) HSColGroupStruct;
            newGroup->colSet = colSet;
            newGroup->colCount = colCount;
            *newGroup->colNames = colNames.data();

            if (hs_globals->multiGroup == NULL)    // first group entry
              {
                hs_globals->multiGroup = newGroup;
              }
            else                  // append to front of list
              {
                newGroup->next = hs_globals->multiGroup;
                hs_globals->multiGroup->prev = newGroup;
                hs_globals->multiGroup = newGroup;
              }

            hs_globals->groupCount++;
          }
      }

    return retcode;
  }

// For debugging only..
void showColSet(HSColSet &colSet, const char *title)
{
  HSLogMan *LM = HSLogMan::Instance();
  if (LM->LogNeeded())
    {
      UInt32 i;
      sprintf(LM->msg, title);
      LM->Log(LM->msg);
      for (i=0; i<colSet.entries(); i++)
        {
          sprintf(LM->msg, "\t\tcolSet[%d]: :%s: %d", i, colSet[i].colname->data(), colSet[i].colnum);
          LM->Log(LM->msg);
        }
    }
}

// TRUE if there exists a multi-column group list that has identical columns
// to what we want to add now (colSet); i.e. we will not add duplicates.
NABoolean GroupExists(HSColSet &colSet)
  {
  HSGlobalsClass *hs_globals = GetHSContext();
    NABoolean         found = FALSE;
    HSColGroupStruct *mgroup;

    mgroup = hs_globals->multiGroup;
    showColSet(colSet, "GroupExists: argument: colSet");
    while (mgroup != NULL)
      {
        showColSet(mgroup->colSet, "GroupExists: mgroup->colSet");
        if (mgroup->colSet == colSet)
            return TRUE;
        mgroup = mgroup->next;
      }
    return FALSE;
  }

// Returns TRUE iff the n columns of the group match, without regard to order,
// columns 1 through n of the index (column 0 of the index being "_SALT_"),
// or columns 0 through n-1 of the index. The saltMatched output parameter is
// set to indicate which of these is the case (if the function returns TRUE).
// Duplicate columns have been removed from the group, so it is enough to
// check that each one matches one of the target columns of the index.
NABoolean MatchesIndexPrefix(HSColGroupStruct* group,
                             NAFileSet* index,
                             NABoolean& saltMatched)
{
  const NAColumnArray& inxCols = index->getIndexKeyColumns();
  CollIndex numInxCols = inxCols.entries();
  Lng32 numGrpCols = group->colCount;
  CollIndex lastColInxToCheck;

  if (numGrpCols < numInxCols)
    lastColInxToCheck = numGrpCols;
  else if (numGrpCols == numInxCols)
    lastColInxToCheck = numGrpCols - 1;
  else  // more group cols than index cols; no chance of match
    return FALSE;

  HSColumnStruct* col;
  NABoolean match;
  NABoolean firstInxColMatched = FALSE;
  NABoolean lastInxColMatched = FALSE;
  saltMatched = FALSE;
  for (Int32 grpColInx=0; grpColInx<numGrpCols; grpColInx++)
    {
      Lng32 grpColPosInTable = group->colSet[grpColInx].colnum;
      match = FALSE;
      for (CollIndex inxColInx=0; inxColInx<=lastColInxToCheck && !match; inxColInx++)
        {
          if (grpColPosInTable == inxCols[inxColInx]->getPosition())
            {
              match = TRUE;
              if (inxCols[inxColInx]->isSaltColumn())
                saltMatched = TRUE;
              if (inxColInx == 0)
                firstInxColMatched = TRUE;
              else if (inxColInx == lastColInxToCheck)
                lastInxColMatched = TRUE;
            }
        }

      if (!match)
        return FALSE;
    }

  // If _SALT_ alone is specified, no action is needed.
  if (numGrpCols == 1 && saltMatched)
    return FALSE;

  // Each of the n group columns matched one of the initial n+1 index columns.
  // If both the first and last index columns were matched and the index has more
  // columns than the group, then something in the middle was left out, and we
  // can not say the group matches the index.
  if (numGrpCols < numInxCols && firstInxColMatched && lastInxColMatched)
    return FALSE;
  else
    return TRUE;
}

// This is called by AddSaltToIndexPrefixes() when a group (SC or MC) is found
// that coincides with the first n columns of an index (possibly excluding _SALT_),
// where n is the number of columns in the group. The function adds an MC group
// that adds _SALT_ (if missing) to the columns of the index contained in matchedGroup,
// in index order.
Lng32 AddSaltedIndexPrefix(NAFileSet* index,
                           HSColGroupStruct* matchedGroup,
                           NABoolean groupHasSalt)
{
  HSLogMan *LM = HSLogMan::Instance();
  HSGlobalsClass *hs_globals = GetHSContext();
  const NAColumnArray& inxCols = index->getIndexKeyColumns();
  HSColSet* saltedColSet = new(STMTHEAP) HSColSet(STMTHEAP);
  HSColumnStruct* colStruct;

  // If the group already contains _SALT_, there will be 1 less column of the
  // index to include.
  CollIndex lastColInxToInclude = matchedGroup->colCount;
  if (groupHasSalt)
    lastColInxToInclude--;

  // Create an MC that includes the columns of the matching MC with the columns
  // in index order, adding _SALT_ at the beginning if it wasn't present in the
  // original MC.
  for (CollIndex inxColInx=0; inxColInx<=lastColInxToInclude; inxColInx++)
    {
      colStruct = new(STMTHEAP) HSColumnStruct;
      *colStruct =  hs_globals->objDef
                              ->getColInfo(inxCols[inxColInx]->getPosition());
      colStruct->position = inxColInx;  // position in MC
      saltedColSet->insert(*colStruct);
    }

  if (!groupHasSalt && LM->LogNeeded())
    {
      snprintf(LM->msg, sizeof(LM->msg),
               "Adding an MC to duplicate index subset (%s) with \"_SALT_\" prefix added.",
               matchedGroup->colNames->data());
      LM->Log(LM->msg);
    }

  // If we formed the new, index-ordered group and had to add _SALT_, we leave
  // the original group in place. However, if the new group has the same cols
  // and only the order was changed, delete the original first, or the new one
  // will be rejected as a duplicate when we try to add it.
  if (groupHasSalt)
    hs_globals->removeGroup(matchedGroup);

  return AddColumnSet(*saltedColSet);
}

// Look for groups that constitute a leading prefix of the primary key, possibly
// excluding the "_SALT_" column. For each such group that omits "_SALT_", add
// another group consisting of that group plus "_SALT_", with the columns in index
// order. For each such group that already includes "_SALT_", replace it with a
// group containing the same set of columns, but in index order. This function
// should only be called for a salted table.
Lng32 AddSaltToIndexPrefixes()
{
  Lng32 retcode = 0;
  HSGlobalsClass *hs_globals = GetHSContext();
  NATable* naTbl = hs_globals->objDef->getNATable();
  NAFileSet* clusteringIndex = naTbl->getClusteringIndex();
  NABoolean groupHasSalt;
  HSColGroupStruct* nextGroup;

  NABoolean doingSingles = TRUE;
  HSColGroupStruct* group = hs_globals->singleGroup;
  if (!group)
    {
      group = hs_globals->multiGroup;
      doingSingles = FALSE;
    }

  while (group)
    {
      // AddSaltedIndexPrefix may remove the group it is passed from the group
      // list and deallocate it, so we grab the link to the next one from it first.
      nextGroup = group->next;

      // See if the group matches a prefix of the key, allowing _SALT_ to not
      // be present. groupHasSalt will indicate whether it was present. If it
      // matches, add the appropriate group.
      if (MatchesIndexPrefix(group, clusteringIndex, groupHasSalt))
        retcode = AddSaltedIndexPrefix(clusteringIndex, group, groupHasSalt);

      group = nextGroup;
      if (!group && doingSingles)
        {
          doingSingles = FALSE;
          group = hs_globals->multiGroup;
        }
    }

  return retcode;
}

Lng32 AddKeyGroups()
  {
  HSGlobalsClass *hs_globals = GetHSContext();
    if (HSGlobalsClass::isHiveCat(hs_globals->objDef->getCatName()))
      {
        // HSHiveTableDef::getKeyList()/getIndexArray() not yet implemented.
        *CmpCommon::diags() << DgSqlCode(-UERR_NO_ONEVERYKEY) << DgString0("hive");
        return -1;
      }

    Lng32 retcode = 0;
    Lng32 numColsInGroup = 0;
    HSColumnStruct col;
    NAString tempColList = "";
    NAString tempCol;
    NAString autoGroup;
    ULng32 numKeys;
    ULng32 i, j;
    NATable* naTbl = hs_globals->objDef->getNATable();
    HSLogMan *LM = HSLogMan::Instance();

    // ----------------------------------------------------------
    // Generate histograms for KEY
    // ----------------------------------------------------------
    // The clustering index is included in the list of indices returned by
    // NATable::getIndexList(), so we store its pointer so we can skip it
    // when the other indexes are processed below.
    NAFileSet* clusteringIndex = naTbl->getClusteringIndex();
    const NAColumnArray& keyCols = clusteringIndex->getIndexKeyColumns();
    Lng32 colPos;
    numKeys = keyCols.entries();

    if (numKeys == 1)     // SINGLE-COLUMN KEY
      {
        colPos = keyCols[0]->getPosition();
        if (LM->LogNeeded())
          {
            sprintf(LM->msg, "\t\tKEY:\t\t(%s)", hs_globals->objDef->getColName(colPos));
            LM->Log(LM->msg);
          }

        if (ColumnExists(colPos)) // avoid duplicates
          {
            LM->Log("\t\t** duplicate column group has been ignored.");
          }
        else                                 // add to single-column group list
          {
            retcode = AddSingleColumn(colPos);
          }
      }
    else if (numKeys > 1) // MULTI-COLUMN KEY
      {  
        // Create multiple MC group(s) if numkeys > 1.  Subset MC groups will
        // also be created if numkeys > 2,  E.g. If numkeys = 5, then
        // MC groups with 5, 4, 3, and 2 columns will be created using
        // the key columns.  Note that if numkeys is larger than CQD 
        // USTAT_NUM_MC_GROUPS_FOR_KEYS (default = 5), then the number
        // of groups created will be limited by this value.  So, e.g. if
        // numkeys = 10, then MC groups with 5, 4, 3, and 2 columns will
        // be created (that is, 5 groups will be created - incl the single).

        ULng32 minMCGroupSz = 2;
        ULng32 maxMCGroups  = (ULng32)
          CmpCommon::getDefaultNumeric(USTAT_NUM_MC_GROUPS_FOR_KEYS);

        // Generate no MCs with more cols than specified by the cqd.
        if (numKeys > maxMCGroups)
          numKeys = maxMCGroups;

        // For salted table, generate only the longest MC for the key (subject
        // to max cols determined above) unless a cqd is set to gen all MCs of
        // allowable sizes.
        if (CmpCommon::getDefault(USTAT_ADD_SALTED_KEY_PREFIXES_FOR_MC) == DF_OFF &&
            hs_globals->objDef->getColNum("_SALT_", FALSE) >= 0)
          minMCGroupSz = numKeys;

        while (numKeys >= minMCGroupSz)  // Create only MC groups not single cols
          {
            HSColSet colSet(STMTHEAP);

            autoGroup = "(";
            for (j = 0; j < numKeys; j++)
              {
                colPos = keyCols[j]->getPosition();
                col = hs_globals->objDef->getColInfo(colPos);
                col.colnum = colPos;
                colSet.insert(col);
                autoGroup += col.colname->data();
                autoGroup += ",";
              }

            if (LM->LogNeeded())
              {
                autoGroup.replace(autoGroup.length()-1,1,")");    // replace comma with close parenthesis
                sprintf(LM->msg, "\t\tKEY:\t\t%s", autoGroup.data());
                LM->Log(LM->msg);
              }

            if (retcode = AddColumnSet(colSet))
              {
                HSHandleError(retcode);
              }
            numKeys--;
          }
      }
  
    // ----------------------------------------------------------
    // Generate histograms for all INDEXES
    // ----------------------------------------------------------
    const NAFileSetList& indexes = naTbl->getIndexList();
    NAFileSet* index;
    for (i = 0; i < indexes.entries(); i++ )
      {
        index = indexes[i];
        if (index == clusteringIndex)
          continue;  // clustering index processed above already
        const NAColumnArray& keyCols = index->getIndexKeyColumns();
        numKeys = keyCols.entries();
        if (numKeys == 1)                            // SINGLE-COLUMN INDEX
          {
            colPos = keyCols[0]->getPosition();
            if (LM->LogNeeded())
              {
                sprintf(LM->msg, "\t\tINDEX[%d]\t(%s)", i, 
                        hs_globals->objDef->getColName(colPos));
                LM->Log(LM->msg);
              }
            if (ColumnExists(colPos)) // avoid duplicates
              {
                LM->Log("\t\t*** duplicate column group has been ignored.");
              }
            else                                 // add to single-column group list
              {
                retcode = AddSingleColumn(colPos);
              }
          }
        else // MULTI-COLUMN INDEX
          {  
            // Create multiple MC group(s) if numkeys > 1.  Subset MC groups will
            // also be created if numkeys > 2,  E.g. If numkeys = 5, then
            // MC groups with 5, 4, 3, and 2 columns will be created using
            // the key columns.  Note that if numkeys is larger than CQD 
            // USTAT_NUM_MC_GROUPS_FOR_KEYS (default = 5), then the number
            // of groups created will be limited by this value.  So, e.g. if
            // numkeys = 10, then MC groups with 10, 9, 8, 7, 6 columns will
            // be created (that is, 5 groups will be created).

            ULng32 minMCGroupSz = 2;
            ULng32 maxMCGroups  = (ULng32)
              CmpCommon::getDefaultNumeric(USTAT_NUM_MC_GROUPS_FOR_KEYS);
            if (numKeys > maxMCGroups) 
              minMCGroupSz = numKeys - maxMCGroups + 1;
            while (numKeys >= minMCGroupSz)  // MinMCGroupSz is greater than 1.
              {
		HSColSet colSet(STMTHEAP);

              tempColList = "";
              autoGroup = "(";
              for (j = 0; j < numKeys; j++)
                {
                  colPos = keyCols[j]->getPosition();
                  tempCol = ".";
                  tempCol += LongToNAString(colPos);
                  tempCol += ".";

                  // Eliminate duplicate columns in the index;
                  // They may have been introduced by appending the key to the specified index.
                  if (!tempColList.contains(tempCol))
                    {
                      col = hs_globals->objDef->getColInfo(colPos);
                      col.colnum = colPos;
                      colSet.insert((const struct HSColumnStruct) col);

                      tempColList += tempCol.data();
                      numColsInGroup++;
                      autoGroup += col.colname->data();
                      autoGroup += ",";
                    }
                }

              if (colSet.entries())
                {
                  if (numColsInGroup > 1)
                    {
                      if (LM->LogNeeded())
                        {
                          autoGroup.replace(autoGroup.length()-1,1,")");    // replace comma with close parenthesis
                          sprintf(LM->msg, "\t\tINDEX[%d]\t%s", i, autoGroup.data());
                          LM->Log(LM->msg);
                        }

                      if (retcode = AddColumnSet(colSet))
                        {
                          HSHandleError(retcode);
                        }
                    }
                  numColsInGroup = 0;
                }
              numKeys--;
              }
          }
      }

    return retcode;
  }


// -----------------------------------------------------------------------
// Add the single-column groups from startColumn to endColumn. If these
// parameters are NULL, the function has been called for the ON EVERY COLUMN
// clause, and we will add all single column groups, as well as key groups.
// -----------------------------------------------------------------------
Lng32 AddEveryColumn(const char *startColumn, const char *endColumn)
  {
    HSGlobalsClass *hs_globals = GetHSContext();
    Lng32 colNumber, retcode;
    NAString colName;
    Lng32 start, upto;
    HSLogMan *LM = HSLogMan::Instance();
    hs_globals->parserError = HSGlobalsClass::ERROR_SEMANTICS;

    // Can't use EVERYCOL_OPT flag for this test, it may have been set on a
    // previous call (making this a redundant, or incorrect, request to ustat
    // an individual column name). startColumn will always be NULL if this fn
    // is called to add all the columns for a table.
    if (!startColumn)
      {
        HS_ASSERT(hs_globals->optFlags & EVERYCOL_OPT);
        start = 0;
        upto  = hs_globals->objDef->getNumCols() - 1;
      }
    else
      {
        start = hs_globals->objDef->getColNum(startColumn);
        if (start < 0)
          {
            retcode = -1;
            HSHandleError(retcode);
          }
        upto  = hs_globals->objDef->getColNum(endColumn);
        if (upto < 0)
          {
            retcode = -1;
            HSHandleError(retcode);
          }

        if (start > upto)
          {
            Lng32 tmp = upto;
            upto = start;
            start = tmp;
          }
      }

    for (colNumber = start; colNumber <= upto; colNumber++)
      {
        if (ColumnExists(colNumber))      // avoid duplicates
          {
            colName = hs_globals->objDef->getColName(colNumber);
            if (LM->LogNeeded())
              {
                sprintf(LM->msg, "\t\t****Duplicate Column group (%s) has been ignored", colName.data());
                LM->Log(LM->msg);
              }
          }
        else if (!DFS2REC::isLOB(hs_globals->objDef->getColInfo(colNumber).datatype))
          {
            // add to single-column group list
            retcode = AddSingleColumn(colNumber);
          }
        // else it's a LOB column; silently exclude it (the column was only
        // implicitly referenced)
      }

    if (!startColumn &&  // ON EVERY COLUMN causes key groups to be added as well
        !HSGlobalsClass::isHiveCat(hs_globals->objDef->getCatName()))  // No ustat on keys yet for hive tables
      {
        retcode = AddKeyGroups();
        HSHandleError(retcode);
      }

    hs_globals->parserError = HSGlobalsClass::ERROR_SYNTAX;
    return 0;
  }


// Get the current set of histogrammed column groups for the table, then add
// each of these to either the singleGroup or multiGroup list for histograms
// to be created.
//
Lng32 AddExistingColumns()
  {
    HSGlobalsClass *hs_globals = GetHSContext();
    HSColGroupStruct *group, *groupList;
    Lng32 retcode = 0;

    // Introduce new scope for instance of HSTranController; its destructor
    // terminates the transaction. We need a transaction for this because
    // groupListFromTable() executes a query to get the current histograms,
    // and we want any acquired locks to be released before we proceed. An
    // implicit transaction started by the query would remain open and retain
    // the locks.
    { 
      HSTranController TC("GET GROUP LIST FOR EXISTING", &retcode);
      HSHandleError(retcode);
      retcode = hs_globals->groupListFromTable(groupList,
                                               TRUE);  // pass 'TRUE' to skip empty histograms.
      HSHandleError(retcode);
    }

    while (groupList != NULL)
      {
        // Detach the current first node from the list. It will be added to a
        // different list (hs_globals->singleGroup or hs_globals->multiGroup).
        // Don't change group->next until groupList points to something else.
        //
        group = groupList;
        groupList = groupList->next;
        if (groupList)
          groupList->prev = NULL;
        group->next = NULL;

        // Set oldHistid to 0 so hist id will be reread during FlushStatistics.
        // Reading it in the same transaction that writes the histograms keeps
        // 2 or more concurrent Update Stats statements from coming up with the
        // same new hist id.
        group->oldHistid = 0;

        // Look through the columns in this group for any oversized columns.
        for (UInt32 i = 0; i < group->colSet.entries(); i++)
          {
            bool isOverSized = DFS2REC::isAnyCharacter(group->colSet[i].datatype) &&
              (group->colSet[i].length > hs_globals->maxCharColumnLengthInBytes);
            if (isOverSized)
              {
                hs_globals->hasOversizedColumns = TRUE;
              }
          }

        hs_globals->addGroup(group);
      }
    return retcode;
  }
