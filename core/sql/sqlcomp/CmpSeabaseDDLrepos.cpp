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
 * File:         CmpSeabaseDDLrepos.cpp
 * Description:  Implements common methods and operations for SQL/hbase tables.
 *
 *
 * Created:     6/30/2013
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#include "CmpSeabaseDDLincludes.h"
#include "CmpSeabaseDDLrepos.h"
#include "logmxevent_traf.h"


short CmpSeabaseDDL::createRepos(ExeCliInterface * cliInterface)
{
  Lng32 cliRC = 0;

  char queryBuf[20000];

  NABoolean xnWasStartedHere = FALSE;

  cliRC = cliInterface->holdAndSetCQD
    ("TRAF_MAX_CHARACTER_COL_LENGTH", "1000000");

  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  if (beginXnIfNotInProgress(cliInterface, xnWasStartedHere))
    goto label_error;

  // Create the _REPOS_ schema
  str_sprintf(queryBuf, "create schema %s.\"%s\" ; ",
              getSystemCatalog(),SEABASE_REPOS_SCHEMA);

  cliRC = cliInterface->executeImmediate(queryBuf);
  if (cliRC == -1022)  // schema already exists
    {
      // ignore error.
      cliRC = 0;
    }
  else if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
    }

  if (endXnIfStartedHere(cliInterface, xnWasStartedHere, cliRC) < 0)
    goto label_error;

  for (Int32 i = 0; i < sizeof(allReposUpgradeInfo)/sizeof(MDUpgradeInfo); i++)
    {
      const MDUpgradeInfo &rti = allReposUpgradeInfo[i];

      if (! rti.newName)
        continue;

      for (Int32 j = 0; j < NUM_MAX_PARAMS; j++)
	{
	  param_[j] = NULL;
	}

      const QString * qs = NULL;
      Int32 sizeOfqs = 0;

      qs = rti.newDDL;
      sizeOfqs = rti.sizeOfnewDDL; 

      Int32 qryArraySize = sizeOfqs / sizeof(QString);
      char * gluedQuery;
      Lng32 gluedQuerySize;
      glueQueryFragments(qryArraySize,  qs,
			 gluedQuery, gluedQuerySize);

 
      param_[0] = getSystemCatalog();
      param_[1] = SEABASE_REPOS_SCHEMA;

      str_sprintf(queryBuf, gluedQuery, param_[0], param_[1]);
      NADELETEBASICARRAY(gluedQuery, STMTHEAP);

      if (beginXnIfNotInProgress(cliInterface, xnWasStartedHere))
        goto label_error;
      
      cliRC = cliInterface->executeImmediate(queryBuf);
      if (cliRC == -1390)  // table already exists
	{
	  // ignore error.
          cliRC = 0;
	}
      else if (cliRC < 0)
	{
	  cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
	}

      if (endXnIfStartedHere(cliInterface, xnWasStartedHere, cliRC) < 0)
        goto label_error;
      
    } // for
  
  cliRC = cliInterface->restoreCQD("TRAF_MAX_CHARACTER_COL_LENGTH");
  return 0;

  label_error:
   cliRC = cliInterface->restoreCQD("TRAF_MAX_CHARACTER_COL_LENGTH");
   return -1;
}

short CmpSeabaseDDL::dropRepos(ExeCliInterface * cliInterface,
                               NABoolean oldRepos,
                               NABoolean dropSchema,
                               NABoolean inRecovery)
{
  Lng32 cliRC = 0;
  NABoolean xnWasStartedHere = FALSE;
  char queryBuf[1000];

  for (Int32 i = 0; i < sizeof(allReposUpgradeInfo)/sizeof(MDUpgradeInfo); i++)
    {
      const MDUpgradeInfo &rti = allReposUpgradeInfo[i];

      // If we are dropping the new repository as part of a recovery action,
      // and there is no "old" table (because the table didn't change in this
      // upgrade), then don't drop the new table. (If we did, we would be 
      // dropping the existing data.)
      if (!oldRepos && inRecovery && !rti.oldName)
        continue;

      if ((oldRepos && !rti.oldName) || (NOT oldRepos && ! rti.newName))
        continue;

      str_sprintf(queryBuf, "drop table %s.\"%s\".%s cascade; ",
                  getSystemCatalog(), SEABASE_REPOS_SCHEMA,
                  (oldRepos ? rti.oldName : rti.newName));
    
      if (beginXnIfNotInProgress(cliInterface, xnWasStartedHere))
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        }    

      cliRC = cliInterface->executeImmediate(queryBuf);
      if (cliRC == -1389)  // table doesn't exist
	{
	  // ignore the error.
          cliRC = 0;
	}
      else if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
        }
 
      if (endXnIfStartedHere(cliInterface, xnWasStartedHere, cliRC) < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        }
 
      if (cliRC < 0)
        {
          return -1;  
        }

    }

  if (dropSchema)
    {
      // Drop the _REPOS_ schema
      str_sprintf(queryBuf, "drop schema %s.\"%s\" cascade; ",
                  getSystemCatalog(),SEABASE_REPOS_SCHEMA);

      if (beginXnIfNotInProgress(cliInterface, xnWasStartedHere))
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        } 

      cliRC = cliInterface->executeImmediate(queryBuf);
      if (cliRC == -1003)  // schema doesnt exist
        {
          // ignore the error.
          cliRC = 0;
        }
      else if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
        }

      if (endXnIfStartedHere(cliInterface, xnWasStartedHere, cliRC) < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        }
 
      if (cliRC < 0)
        {
          return -1;  
        }
    }

  return 0;
}

short CmpSeabaseMDupgrade::dropReposTables(ExpHbaseInterface *ehi,
                                           NABoolean oldRepos)
{
  Lng32 retcode = 0;
  Lng32 errcode = 0;

  for (Int32 i = 0; i < sizeof(allReposUpgradeInfo)/sizeof(MDUpgradeInfo); i++)
    {
      const MDUpgradeInfo &rti = allReposUpgradeInfo[i];

      if ((NOT oldRepos) && (!rti.newName))
	continue;

      HbaseStr hbaseTable;
      NAString extNameForHbase = TRAFODION_SYSCAT_LIT;
      extNameForHbase += ".";
      extNameForHbase += SEABASE_REPOS_SCHEMA;
      extNameForHbase +=  ".";

      if (oldRepos)
	{
          if (!rti.oldName)
            continue;
          
          extNameForHbase += rti.oldName;
	}
      else
	extNameForHbase += rti.newName;
      
      hbaseTable.val = (char*)extNameForHbase.data();
      hbaseTable.len = extNameForHbase.length();
      
      retcode = dropHbaseTable(ehi, &hbaseTable, FALSE, FALSE);
      if (retcode < 0)
	{
	  errcode = -1;
	}
      
    } // for
  
  return errcode;
}

short CmpSeabaseDDL::alterRenameRepos(ExeCliInterface * cliInterface,
                                      NABoolean newToOld)
{
  Lng32 cliRC = 0;

  char queryBuf[10000];

  NABoolean xnWasStartedHere = FALSE;

  // alter table rename cannot run inside of a transaction.
  // return an error if a xn is in progress
  if (xnInProgress(cliInterface))
    {
      *CmpCommon::diags() << DgSqlCode(-20123);
      return -1;
    }

  for (Int32 i = 0; i < sizeof(allReposUpgradeInfo)/sizeof(MDUpgradeInfo); i++)
    {
      const MDUpgradeInfo &rti = allReposUpgradeInfo[i];

      if ((! rti.newName) || (! rti.oldName) || (NOT rti.upgradeNeeded))
        continue;

      if (newToOld)
        str_sprintf(queryBuf, "alter table %s.\"%s\".%s rename to %s skip view check; ",
                    getSystemCatalog(), SEABASE_REPOS_SCHEMA, rti.newName, rti.oldName);
      else
        str_sprintf(queryBuf, "alter table %s.\"%s\".%s rename to %s skip view check; ",
                    getSystemCatalog(), SEABASE_REPOS_SCHEMA, rti.oldName, rti.newName);
        
      cliRC = cliInterface->executeImmediate(queryBuf);
      if (cliRC == -1389 || cliRC == -1390)
        {
          // ignore.
          cliRC = 0;
        }
      else if (cliRC < 0)
	{
	  cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
	}
    }

  return 0;
}

short CmpSeabaseDDL::copyOldReposToNew(ExeCliInterface * cliInterface)
{
  Lng32 cliRC = 0;

  char queryBuf[10000];

  for (Int32 i = 0; i < sizeof(allReposUpgradeInfo)/sizeof(MDUpgradeInfo); i++)
    {
      const MDUpgradeInfo &rti = allReposUpgradeInfo[i];

      if ((! rti.newName) || (! rti.oldName) || (NOT rti.upgradeNeeded))
        continue;

      str_sprintf(queryBuf, "upsert using load into %s.\"%s\".%s %s%s%s select %s from %s.\"%s\".%s SRC %s;",
                  TRAFODION_SYSCAT_LIT,
                  SEABASE_REPOS_SCHEMA,
                  rti.newName, 
                  (rti.insertedCols ? "(" : ""),
                  (rti.insertedCols ? rti.insertedCols : ""),
                  (rti.insertedCols ? ")" : ""),
                  (rti.selectedCols ? rti.selectedCols : "*"),
                  TRAFODION_SYSCAT_LIT,
                  SEABASE_REPOS_SCHEMA,
                  rti.oldName,
                  (rti.wherePred ? rti.wherePred : ""));

      cliRC = cliInterface->executeImmediate(queryBuf);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        }

    } // for

  return 0;
}

short CmpSeabaseDDL::dropAndLogReposViews(ExeCliInterface * cliInterface,
                                          NABoolean & someViewSaved /* out */)
{
  short retcode = 0;  // assume success
  someViewSaved = FALSE;

  // For each table that has been migrated, drop any views that existed
  // on the old table and save their view text. (In the future, we can
  // add logic to recreate the views on the new tables.)

  for (Int32 i = 0; i < sizeof(allReposUpgradeInfo)/sizeof(MDUpgradeInfo); i++)
    {
      const MDUpgradeInfo &rti = allReposUpgradeInfo[i];

      if ((! rti.oldName) || (NOT rti.upgradeNeeded))
        continue;

      Int64 tableUID = getObjectUID(cliInterface,
        getSystemCatalog(), SEABASE_REPOS_SCHEMA, rti.oldName,
        COM_BASE_TABLE_OBJECT_LIT, NULL, NULL, FALSE, FALSE /* ignore error */);

      if (tableUID != -1)  // if we got it
        {
          NAList<NAString> viewNameList(STMTHEAP);
          NAList<NAString> viewDefnList(STMTHEAP);
 
          short retcode1 = 
             saveAndDropUsingViews(tableUID, cliInterface,
                                   viewNameList /* out */,viewDefnList /* out */);
          if (retcode1)
            retcode = -1;  // couldn't get views for old repository table
          else if (viewDefnList.entries() > 0)  // if we dropped some views
            {
              // Save the view text in the logs. 
              //
              // Aside: 
              // 
              // In the future we could add logic to recreate the views. One
              // complication is that we'd need to preserve the privileges as
              // well, so replaying the view text is not enough. Two possible
              // designs are:
              //
              // 1. Add logic to capture privileges as GRANT statements that can
              // be replayed, or
              //
              // 2. Change repository upgrade to be like metadata upgrade; that is
              // instead of using vanilla DDL to drop and recreate tables, we do
              // HBase rename trickery underneath. That preserves object UIDs and
              // allows the view definitions to survive the upgrade untouched.
              //
              // With either of these designs, the question of what to do about
              // views that are no longer valid needs to be addressed. For example,
              // a view might reference a column that no longer exists. In the
              // first design, the view text replay will fail, and we can just
              // log the view text as we have done here. In the second design,
              // we have to face the possibility that there is invalid view text
              // already in the metadata. Perhaps we can compile a query that 
              // does a select * from each view and if that returns an error we
              // can either drop the view and log its text, or perhaps mark the
              // view as invalid.
              //
              // End aside.
              for (int i = 0; i < viewDefnList.entries(); i++)
                {
                  SQLMXLoggingArea::logSQLMXPredefinedEvent(
                    "The following view on a Repository table was dropped as part of upgrade processing:", LL_ERROR);
                  SQLMXLoggingArea::logSQLMXPredefinedEvent(
                    viewDefnList[i].data(), LL_ERROR);               
                }          
              someViewSaved = TRUE;       
            }
        }
      else
        retcode = -1;  // couldn't get objectUID for old repository table
    }
          
  return retcode;
}

short CmpSeabaseDDL::upgradeRepos(ExeCliInterface * cliInterface,
                                  CmpDDLwithStatusInfo *mdui)
{
  Lng32 cliRC = 0;

  while (1) // exit via return stmt in switch
    {
      switch (mdui->subStep())
        {
        case 0:
          {
            mdui->setMsg("Upgrade Repository: Started");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
        
            return 0;
          }
          break;
      
        case 1:
          {
            mdui->setMsg("  Start: Drop Old Repository");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
        
            return 0;
          }
          break;

        case 2:
          {
            // drop old repository
            if (dropRepos(cliInterface, TRUE/*old repos*/, FALSE/*no schema drop*/))
              return -3;  // error, but no recovery needed (and in fact
                          // doing such things as dropping the *new* repository
                          // would just destroy the existing repository data
                          // that we wish to save)
        
            mdui->setMsg("  End:   Drop Old Repository");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
        
            return 0;
          }
          break;

        case 3:
          {
            mdui->setMsg("  Start: Rename Current Repository");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
        
            return 0;
          }
          break;
        
        case 4:
          {
            // rename current repository tables to *_OLD_REPOS
            if (alterRenameRepos(cliInterface, TRUE))
              return -2;  // error, need to undo the rename only

            mdui->setMsg("  End:   Rename Current Repository");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
        
            return 0;
          }
          break;

        case 5:
          {
            mdui->setMsg("  Start: Create New Repository");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
        
            return 0;
          }
          break;
         
        case 6:
          {
            // create new repository
            if (createRepos(cliInterface))
              return -1;  // error, need to drop new repository then undo rename
        
            mdui->setMsg("  End:   Create New Repository");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
  
            return 0;
          }
          break;

        case 7:
          {
            mdui->setMsg("  Start: Copy Old Repository Contents ");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
        
            return 0;
          }
          break;

        case 8:
          {
            // copy old contents into new repository
            if (copyOldReposToNew(cliInterface))
              return -1;  // error, need to drop new repository then undo rename
        
            mdui->setMsg("  End:   Copy Old Repository Contents ");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
         
            return 0;
          }
          break;

        case 9:
          {
            mdui->setMsg("Upgrade Repository: Done except for cleaning up");
            mdui->setSubstep(0);
            mdui->setEndStep(TRUE);
        
            return 0;
          }
          break;

        default:
          return -1;
        }
    } // while

  return 0;
}
      

short CmpSeabaseDDL::upgradeReposComplete(ExeCliInterface * cliInterface,
                                          CmpDDLwithStatusInfo *mdui)
{
  Lng32 cliRC = 0;

  while (1) // exit via return stmt in switch
    {
      switch (mdui->subStep())
        {
        case 0:
          {
            mdui->setMsg("Upgrade Repository: Check for views on Repository");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
        
            return 0;
          }
          break;

        case 1:
          {
            // If there were views on the old Repository tables, they are
            // still there, by virtue of the fact that we did "SKIP VIEW CHECK"
            // on the ALTER TABLE RENAME. Now we will drop the views and 
            // capture their view definitions (which will contain the old table
            // name, not the renamed table name) and log the view text so that
            // the user can recreate these views later if he/she wishes.
            // In the future, perhaps, we will migrate these views automatically.
            NABoolean someViewSaved = FALSE;
            dropAndLogReposViews(cliInterface,someViewSaved /* out */);

            mdui->setMsg("Upgrade Repository: View check done");
            mdui->subStep()++;
            if (!someViewSaved)
              mdui->subStep()++;  // skip view text saved message step
            mdui->setEndStep(FALSE);
       
            return 0;
          }
          break;

        case 2:
          {
            // This state is here in case we want to report to the user that
            // there was a view that was dropped. But we saved the view text
            // in the logs.
            mdui->setMsg("One or more views on a Repository table were dropped. View text was saved in the logs. Look for 'CREATE VIEW' to find it.");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
        
            return 0;
          }
          break;

        case 3:
          {
            mdui->setMsg("Upgrade Repository: Drop Old Repository");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
        
            return 0;
          }
          break;

        case 4:
          {
            // drop old repository; ignore errors
            dropRepos(cliInterface, TRUE/*old repos*/, FALSE/*no schema drop*/);
        
            mdui->setMsg("Upgrade Repository: Drop Old Repository done");
            mdui->setEndStep(TRUE);
            mdui->setSubstep(0);
         
            return 0;
          }
          break;

        default:
          return -1;
        }
    } // while

  return 0;
}

short CmpSeabaseDDL::upgradeReposUndo(ExeCliInterface * cliInterface,
                                      CmpDDLwithStatusInfo *mdui)
{
  Lng32 cliRC = 0;

  while (1) // exit via return stmt in switch
    {
      switch (mdui->subStep())
        {
        // error return codes from upgradeRepos can be mapped to
        // the right recovery substep by this formula: substep = -(retcode + 1)
        case 0: // corresponds to -1 return code from upgradeRepos (or
                // to full recovery after some error after upgradeRepos)
        case 1: // corresponds to -2 return code from upgradeRepos
        case 2: // corresponds to -3 return code from upgradeRepos
          {
            mdui->setMsg("Upgrade Repository: Restoring Old Repository");
            mdui->setSubstep(2*mdui->subStep()+3); // go to appropriate case
            mdui->setEndStep(FALSE);
        
            return 0;
          }
          break;

        case 3:
          {
            mdui->setMsg(" Start: Drop New Repository");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
        
            return 0;
          }
          break;

        case 4:
          {
            // drop new repository; ignore errors
            dropRepos(cliInterface, FALSE/*new repos*/, FALSE/*no schema drop*/,
                      TRUE /* don't drop new tables that haven't been upgraded */);
            cliInterface->clearGlobalDiags();
            mdui->setMsg(" End: Drop New Repository");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
        
            return 0;
          }
          break;

        case 5:
          {
            mdui->setMsg(" Start: Rename Old Repository back to New");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
        
            return 0;
          }
          break;
 
        case 6:
          {
            // rename old repos to current; ignore errors
            alterRenameRepos(cliInterface, FALSE);
            cliInterface->clearGlobalDiags();
            mdui->setMsg(" End: Rename Old Repository back to New");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
        
            return 0;
          }
          break;

        case 7:
          {
            mdui->setMsg("Upgrade Repository: Restore done");
            mdui->setSubstep(0);
            mdui->setEndStep(TRUE);
        
            return 0;
          }
          break;

        default:
          return -1;
        }
    } // while

  return 0;
}

void CmpSeabaseDDL::processRepository(
                                      NABoolean createR, NABoolean dropR, NABoolean upgradeR)
{
  ExeCliInterface cliInterface(STMTHEAP, 0, NULL,
    CmpCommon::context()->sqlSession()->getParentQid());

  if (createR)
    createRepos(&cliInterface);
  else if (dropR)
    dropRepos(&cliInterface);

  return;
}
