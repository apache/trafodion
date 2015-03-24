/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
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


short CmpSeabaseDDL::createRepos(ExeCliInterface * cliInterface)
{
  Lng32 cliRC = 0;

  char queryBuf[20000];

  NABoolean xnWasStartedHere = FALSE;

  if (beginXnIfNotInProgress(cliInterface, xnWasStartedHere))
    return -1;

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
    return -1;

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
      NADELETEBASIC(gluedQuery, STMTHEAP);

      if (beginXnIfNotInProgress(cliInterface, xnWasStartedHere))
        return -1;
      
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
        return -1;
      
    } // for
  
  return 0;
}

short CmpSeabaseDDL::dropRepos(ExeCliInterface * cliInterface,
                               NABoolean oldRepos,
                               NABoolean dropSchema)
{
  Lng32 cliRC = 0;

  char queryBuf[1000];

  for (Int32 i = 0; i < sizeof(allReposUpgradeInfo)/sizeof(MDUpgradeInfo); i++)
    {
      const MDUpgradeInfo &rti = allReposUpgradeInfo[i];

      if ((oldRepos && !rti.oldName) || (NOT oldRepos && ! rti.newName))
        continue;

      str_sprintf(queryBuf, "drop table %s.\"%s\".%s cascade; ",
                  getSystemCatalog(), SEABASE_REPOS_SCHEMA,
                  (oldRepos ? rti.oldName : rti.newName));
      
      cliRC = cliInterface->executeImmediate(queryBuf);
      if (cliRC == -1389)  // table doesnt exist
	{
	  // ignore the error.
          //          CmpCommon::diags()->clear();
	}
      else if (cliRC < 0)
	{
	  cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
	  return -1;
	}

    }

  if (dropSchema)
    {
      // Drop the _REPOS_ schema
      str_sprintf(queryBuf, "drop schema %s.\"%s\" cascade; ",
                  getSystemCatalog(),SEABASE_REPOS_SCHEMA);
      cliRC = cliInterface->executeImmediate(queryBuf);
      if (cliRC == -1003)  // schema doesnt exist
        {
          // ignore the error.
        }
      else if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
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
      
      retcode = dropHbaseTable(ehi, &hbaseTable);
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

  for (Int32 i = 0; i < sizeof(allReposUpgradeInfo)/sizeof(MDUpgradeInfo); i++)
    {
      const MDUpgradeInfo &rti = allReposUpgradeInfo[i];

      if ((! rti.newName) || (! rti.oldName) || (NOT rti.upgradeNeeded))
        continue;

      if (newToOld)
        str_sprintf(queryBuf, "alter table %s.\"%s\".%s rename to %s; ",
                    getSystemCatalog(), SEABASE_REPOS_SCHEMA, rti.newName, rti.oldName);
      else
        str_sprintf(queryBuf, "alter table %s.\"%s\".%s rename to %s; ",
                    getSystemCatalog(), SEABASE_REPOS_SCHEMA, rti.oldName, rti.newName);
        
      if (beginXnIfNotInProgress(cliInterface, xnWasStartedHere))
        return -1;

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

      if (endXnIfStartedHere(cliInterface, xnWasStartedHere, cliRC) < 0)
        return -1;

    }

  return 0;
}

short CmpSeabaseDDL::copyOldReposToNew(ExeCliInterface * cliInterface)
{
  Lng32 cliRC = 0;

  char queryBuf[10000];

  NABoolean xnWasStartedHere = FALSE;

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
            mdui->setMsg("Upgrade Repository: started");
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
              return -1;
        
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
              {
                mdui->setSubstep(12); // label_error1
                break;
              }

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
              {
                mdui->setSubstep(13); // label_error2
                break;
              }
        
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
              {
                mdui->setSubstep(13); // label_error2
                break;
              }
        
            mdui->setMsg("  End:   Copy Old Repository Contents ");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
         
            return 0;
          }
          break;
      
        case 9:
          {
            mdui->setMsg("  Start: Drop Old Repository ");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
        
            return 0;
          }
          break;

        case 10:
          {
            // drop old repository
            dropRepos(cliInterface, TRUE/*old repos*/, FALSE/*no schema drop*/);
        
            mdui->setMsg("  End:   Drop Old Repository");
            mdui->subStep()++;
            mdui->setEndStep(FALSE);
         
            return 0;
          }
          break;

        case 11:
          {
            mdui->setMsg("Upgrade Repository: done");
            mdui->subStep()++;
            mdui->setEndStep(TRUE);
            mdui->setSubstep(0);
  
            return 0;
          }
          break;
 
        case 12: // label_error1
          {
            // rename old repos to current
            alterRenameRepos(cliInterface, FALSE);
            return -1;
          }
          break;

        case 13: // label_error2
          {
            // drop new repository
            dropRepos(cliInterface, FALSE/*new repos*/, FALSE/*no schema drop*/);
        
            // rename old to new
            alterRenameRepos(cliInterface, FALSE);
        
            return -1;
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
  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL,
    CmpCommon::context()->sqlSession()->getParentQid());

  if (createR)
    createRepos(&cliInterface);
  else if (dropR)
    dropRepos(&cliInterface);
  
  return;
}
