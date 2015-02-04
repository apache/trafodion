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

  for (Int32 i = 0; i < sizeof(allReposTablesInfo)/sizeof(ReposTableInfo); i++)
    {
      const ReposTableInfo &rti = allReposTablesInfo[i];

      if (! rti.tableName)
        continue;

      for (Int32 j = 0; j < NUM_MAX_PARAMS; j++)
	{
	  param_[j] = NULL;
	}

      const QString * qs = NULL;
      Int32 sizeOfqs = 0;

      qs = rti.tableDefnQuery;
      sizeOfqs = rti.sizeOfDefnArr; 

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

short CmpSeabaseDDL::dropRepos(ExeCliInterface * cliInterface)
{
  Lng32 cliRC = 0;

  char queryBuf[1000];

  for (Int32 i = 0; i < sizeof(allReposTablesInfo)/sizeof(ReposTableInfo); i++)
    {
      const ReposTableInfo &rti = allReposTablesInfo[i];

      if (! rti.tableName)
        continue;

      str_sprintf(queryBuf, "drop table %s.\"%s\".%s cascade; ",
                  getSystemCatalog(), SEABASE_REPOS_SCHEMA,
                  rti.tableName);
      
      cliRC = cliInterface->executeImmediate(queryBuf);
      if (cliRC == -1389)  // table doesnt exist
	{
	  // ignore the error.
	}
      else if (cliRC < 0)
	{
	  cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
	  return -1;
	}

    }

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

  return 0;
}

short CmpSeabaseMDupgrade::dropReposTables(ExpHbaseInterface *ehi,
                                           NABoolean oldRepos)
{
  Lng32 retcode = 0;
  Lng32 errcode = 0;

  for (Int32 i = 0; i < sizeof(allReposTablesInfo)/sizeof(ReposTableInfo); i++)
    {
      const ReposTableInfo &rti = allReposTablesInfo[i];

      if (! rti.tableName)
        continue;

      HbaseStr hbaseTable;
      NAString extNameForHbase = TRAFODION_SYSCAT_LIT;
      extNameForHbase += ".";
      extNameForHbase += SEABASE_REPOS_SCHEMA;
      extNameForHbase +=  ".";
      extNameForHbase += rti.tableName;
	
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

short CmpSeabaseDDL::upgradeRepos(ExeCliInterface * cliInterface)
{
  return 0;
}

void CmpSeabaseDDL::processRepository(
                                      NABoolean createR, NABoolean dropR, NABoolean upgradeR)
{
  ExeCliInterface cliInterface(STMTHEAP);

  if (createR)
    createRepos(&cliInterface);
  else if (dropR)
    dropRepos(&cliInterface);
  else /* upgradeR */
    upgradeRepos(&cliInterface);

  return;
}
