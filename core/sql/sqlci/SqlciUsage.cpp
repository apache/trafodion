/********************************************************************  
//
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
********************************************************************/

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         SqlciUsage.cpp
 * Description:  Methods to return table and module usage info.
 *               
 *               
 * Created:      1/15/2003
 * Language:     C++
 * Status:       
 *
 *
 *
 *
 *****************************************************************************
 */

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#define ACCESS access
#define RW_ACCESS 6
#include <errno.h>
#include "Platform.h"
#include "ComCextdecs.h"
#include "str.h"
#include "NAString.h"
#include "sqlcmd.h"
#include "SqlciError.h"


extern volatile Int32 breakReceived;

Usage::Usage(char * module_dir, char * module, char * table,
	     NABoolean noe)
     : SqlCmd(SqlCmd::USAGE_TYPE, NULL),
       moduleDir_(NULL), module_(NULL), table_(NULL),
       noe_(noe)
{

  if (module_dir)
    {
      moduleDir_ = new char[strlen(module_dir)+1];
      strcpy(moduleDir_, module_dir);
    }
  else
    {
      moduleDir_ = new char[strlen("c:/tdm_sqlmodules")+1];
      strcpy(moduleDir_, "c:/tdm_sqlmodules");
    }

  if (module)
    {
      module_ = new char[strlen(module)+1];
      strcpy(module_, module);
    }
  else
    {
      module_ = new char[strlen("*") + 1];
      strcpy(module_, "*");
    }

  if (table)
    {
      table_ = new char[strlen(table)+1];
      strcpy(table_, table);
    }
  else
    {
      table_ = NULL;
    }
}

Usage::~Usage()
{
  delete moduleDir_;
  delete module_;
  delete table_;
}

class ErrorModule
{
public:
  ErrorModule(const char * modName, Int32 error);
  ~ErrorModule();

  char * getModName() { return modName_; }
  Int32    getError()   { return error_;}

private:
  char * modName_;
  Int32  error_;
};

ErrorModule::ErrorModule(const char * modName, Int32 error)
     : error_(error)
{
  modName_ = new char[strlen(modName) + 1];
  strcpy(modName_, modName);
}

ErrorModule::~ErrorModule()
{
  delete modName_;
}

#pragma nowarn(1313)  // warning elimination
static const char * moduleUsageQuery[] =
{
  " select distinct  ",
  "   case ",

  "               when operator = 'FILE_SCAN' ",
  "               then 'Table: ' || substring(description FROM position('scan_type: file_scan ' IN description) + 21  ",
  "                    FOR (position(' ' IN substring(description FROM position('scan_type: file_scan ' IN description) + 21))   ",
  "                         ))  ",

  "               when operator = 'INDEX_SCAN'  ",
  "               then 'Index: ' || substring(description FROM position('scan_type: index_scan ' IN description) + 22  ",
  "                    FOR (position(' ' IN substring(description FROM position('scan_type: index_scan ' IN description) + 22))   ",
  "                         ))  ",

  "               when operator = 'FILE_SCAN_UNIQUE'  ",
  "               then 'Table: ' || substring(description FROM position('scan_type: file_scan_unique ' IN description) + 28  ",
  "                    FOR (position(' ' IN substring(description FROM position('scan_type: file_scan_unique ' IN description) + 28))   ",
  "                         ))  ",

  "               when operator = 'INDEX_SCAN_UNIQUE'  ",
  "               then 'Index: ' || substring(description FROM position('scan_type: index_scan_unique ' IN description) + 29  ",
  "                    FOR (position(' ' IN substring(description FROM position('scan_type: index_scan_unique ' IN description) + 29))   ",
  "                         ))  ",

  "               when operator = 'UNIQUE_UPDATE'  ",
  "               then ",
  "                 case ",
  "                   when position('iud_type: index_' IN description) > 0 ",
  "                     then 'Index: ' || substring(description from position('iud_type: index_unique_update ' IN description) + 30 ",
  "                          FOR (position(' ' IN substring(description FROM position('iud_type: index_unique_update ' IN description) + 30))))   ",

  "                     else 'Table: ' || substring(description from position('iud_type: unique_update ' IN description) + 24 ",
  "                    FOR (position(' ' IN substring(description FROM position('iud_type: unique_update ' IN description) + 24))))   ",
  "                 end ",

  "               when operator = 'CURSOR_UPDATE'  ",
  "               then ",
  "                 case ",
  "                   when position('iud_type: index_' IN description) > 0 ",
  "                     then 'Index: ' || substring(description from position('iud_type: index_cursor_update ' IN description) + 30 ",
  "                          FOR (position(' ' IN substring(description FROM position('iud_type: index_cursor_update ' IN description) + 30))))   ",

  "                     else 'Table: ' || substring(description from position('iud_type: cursor_update ' IN description) + 24 ",
  "                    FOR (position(' ' IN substring(description FROM position('iud_type: cursor_update ' IN description) + 24))))   ",
  "                 end ",

  "               when operator = 'SUBSET_UPDATE'  ",
  "               then ",
  "                 case ",
  "                   when position('iud_type: index_' IN description) > 0 ",
  "                     then 'Index: ' || substring(description from position('iud_type: index_subset_update ' IN description) + 30 ",
  "                          FOR (position(' ' IN substring(description FROM position('iud_type: index_subset_update ' IN description) + 30))))   ",

  "                     else 'Table: ' || substring(description from position('iud_type: subset_update ' IN description) + 24 ",
  "                    FOR (position(' ' IN substring(description FROM position('iud_type: subset_update ' IN description) + 24))))   ",
  "                 end ",

  "               when operator = 'UNIQUE_DELETE'  ",
  "               then ",
  "                 case ",
  "                   when position('iud_type: index_' IN description) > 0 ",
  "                     then 'Index: ' || substring(description from position('iud_type: index_unique_delete ' IN description) + 30 ",
  "                          FOR (position(' ' IN substring(description FROM position('iud_type: index_unique_delete ' IN description) + 30))))   ",

  "                     else 'Table: ' || substring(description from position('iud_type: unique_delete ' IN description) + 24 ",
  "                    FOR (position(' ' IN substring(description FROM position('iud_type: unique_delete ' IN description) + 24))))   ",
  "                 end ",

  "               when operator = 'CURSOR_DELETE'  ",
  "               then ",
  "                 case ",
  "                   when position('iud_type: index_' IN description) > 0 ",
  "                     then 'Index: ' || substring(description from position('iud_type: index_cursor_delete ' IN description) + 30 ",
  "                          FOR (position(' ' IN substring(description FROM position('iud_type: index_cursor_delete ' IN description) + 30))))   ",

  "                     else 'Table: ' || substring(description from position('iud_type: cursor_delete ' IN description) + 24 ",
  "                    FOR (position(' ' IN substring(description FROM position('iud_type: cursor_delete ' IN description) + 24))))   ",
  "                 end ",

  "               when operator = 'SUBSET_DELETE'  ",
  "               then ",
  "                 case ",
  "                   when position('iud_type: index_' IN description) > 0 ",
  "                     then 'Index: ' || substring(description from position('iud_type: index_subset_delete ' IN description) + 30 ",
  "                          FOR (position(' ' IN substring(description FROM position('iud_type: index_subset_delete ' IN description) + 30))))   ",

  "                     else 'Table: ' || substring(description from position('iud_type: subset_delete ' IN description) + 24 ",
  "                    FOR (position(' ' IN substring(description FROM position('iud_type: subset_delete ' IN description) + 24))))   ",
  "                 end ",

  "               when operator = 'INSERT'  ",
  "               then ",
  "                 case ",
  "                   when position('iud_type: index_' IN description) > 0 ",
  "                     then 'Index: ' || substring(description from position('iud_type: index_insert ' IN description) + 23 ",
  "                          FOR (position(' ' IN substring(description FROM position('iud_type: index_insert ' IN description) + 23))))   ",

  "                     else 'Table: ' || substring(description from position('iud_type: insert ' IN description) + 17 ",
  "                    FOR (position(' ' IN substring(description FROM position('iud_type: insert ' IN description) + 17))))   ",
  "                 end ",

  "  end ",
  " from table(explain('%s', '%%')) ",
  "   where operator in ('FILE_SCAN', 'FILE_SCAN_UNIQUE', ",
  "                      'INDEX_SCAN', 'INDEX_SCAN_UNIQUE', ",
  "                      'UNIQUE_UPDATE', 'CURSOR_UPDATE', 'SUBSET_UPDATE', ",
  "                      'UNIQUE_DELETE', 'CURSOR_DELETE', 'SUBSET_DELETE', ",
  "                      'INSERT') ",                  
  " order by 1;"
};

static const char * tableUsageQuery[] =
{
  " select [first 1] operator ",

  " from table(explain('%s', '%%')) ",
  " where ",
  " (  ",
  "  operator in ('INDEX_SCAN', 'INDEX_SCAN_UNIQUE') ",
  "  and ",

  "  (  ",
  "   case ",
  "               when operator = 'INDEX_SCAN'  ",
  "               then substring(description FROM position('scan_type: index_scan ' IN description) + 22  ",
  "                    FOR (position(' ' IN substring(description FROM position('scan_type: index_scan ' IN description) + 22))   ",
  "                         ))  ",

  "               when operator = 'INDEX_SCAN_UNIQUE'  ",
  "               then substring(description FROM position('scan_type: index_scan_unique ' IN description) + 29  ",
  "                    FOR (position(' ' IN substring(description FROM position('scan_type: index_scan_unique ' IN description) + 29))   ",
  "                         ))  ",
  "   end ",
  "  LIKE '%s(%%' ",

  "   or ",

  "   case ",
  "               when operator = 'INDEX_SCAN'  ",
  "               then substring(description FROM position('scan_type: index_scan ' IN description) + 22  ",
  "                    FOR (position(' ' IN substring(description FROM position('scan_type: index_scan ' IN description) + 22))   ",
  "                         ))  ",

  "               when operator = 'INDEX_SCAN_UNIQUE'  ",
  "               then substring(description FROM position('scan_type: index_scan_unique ' IN description) + 29  ",
  "                    FOR (position(' ' IN substring(description FROM position('scan_type: index_scan_unique ' IN description) + 29))   ",
  "                         ))  ",
  "   end ",
  "  LIKE '%%(%s)%%' ",
  "   )  ",
  "  )  ",

  " or ",

  "      (  ",
  "         operator in ('FILE_SCAN', 'FILE_SCAN_UNIQUE', ",
  "                      'UNIQUE_UPDATE', 'CURSOR_UPDATE', 'SUBSET_UPDATE', ",
  "                      'UNIQUE_DELETE', 'CURSOR_DELETE', 'SUBSET_DELETE', ",
  "                      'INSERT') ",                  

  "  and ",

  "   case ",

  "               when operator = 'FILE_SCAN' ",
  "               then substring(description FROM position('scan_type: file_scan ' IN description) + 21  ",
  "                    FOR (position(' ' IN substring(description FROM position('scan_type: file_scan ' IN description) + 21))   ",
  "                         ))  ",

  "               when operator = 'FILE_SCAN_UNIQUE'  ",
  "               then substring(description FROM position('scan_type: file_scan_unique ' IN description) + 28  ",
  "                    FOR (position(' ' IN substring(description FROM position('scan_type: file_scan_unique ' IN description) + 28))   ",
  "                         ))  ",

  "               when operator = 'UNIQUE_UPDATE'  ",
  "               then ",
  "                 case ",
  "                   when position('iud_type: index_' IN description) > 0 ",
  "                     then substring(description from position('iud_type: index_unique_update ' IN description) + 30 ",
  "                          FOR (position(' ' IN substring(description FROM position('iud_type: index_unique_update ' IN description) + 30))))   ",

  "                     else substring(description from position('iud_type: unique_update ' IN description) + 24 ",
  "                    FOR (position(' ' IN substring(description FROM position('iud_type: unique_update ' IN description) + 24))))   ",
  "                 end ",

  "               when operator = 'CURSOR_UPDATE'  ",
  "               then ",
  "                 case ",
  "                   when position('iud_type: index_' IN description) > 0 ",
  "                     then substring(description from position('iud_type: index_cursor_update ' IN description) + 30 ",
  "                          FOR (position(' ' IN substring(description FROM position('iud_type: index_cursor_update ' IN description) + 30))))   ",

  "                     else substring(description from position('iud_type: cursor_update ' IN description) + 24 ",
  "                    FOR (position(' ' IN substring(description FROM position('iud_type: cursor_update ' IN description) + 24))))   ",
  "                 end ",

  "               when operator = 'SUBSET_UPDATE'  ",
  "               then ",
  "                 case ",
  "                   when position('iud_type: index_' IN description) > 0 ",
  "                     then substring(description from position('iud_type: index_subset_update ' IN description) + 30 ",
  "                          FOR (position(' ' IN substring(description FROM position('iud_type: index_subset_update ' IN description) + 30))))   ",

  "                     else substring(description from position('iud_type: subset_update ' IN description) + 24 ",
  "                    FOR (position(' ' IN substring(description FROM position('iud_type: subset_update ' IN description) + 24))))   ",
  "                 end ",

  "               when operator = 'UNIQUE_DELETE'  ",
  "               then ",
  "                 case ",
  "                   when position('iud_type: index_' IN description) > 0 ",
  "                     then substring(description from position('iud_type: index_unique_delete ' IN description) + 30 ",
  "                          FOR (position(' ' IN substring(description FROM position('iud_type: index_unique_delete ' IN description) + 30))))   ",

  "                     else substring(description from position('iud_type: unique_delete ' IN description) + 24 ",
  "                    FOR (position(' ' IN substring(description FROM position('iud_type: unique_delete ' IN description) + 24))))   ",
  "                 end ",

  "               when operator = 'CURSOR_DELETE'  ",
  "               then ",
  "                 case ",
  "                   when position('iud_type: index_' IN description) > 0 ",
  "                     then substring(description from position('iud_type: index_cursor_delete ' IN description) + 30 ",
  "                          FOR (position(' ' IN substring(description FROM position('iud_type: index_cursor_delete ' IN description) + 30))))   ",

  "                     else substring(description from position('iud_type: cursor_delete ' IN description) + 24 ",
  "                    FOR (position(' ' IN substring(description FROM position('iud_type: cursor_delete ' IN description) + 24))))   ",
  "                 end ",

  "               when operator = 'SUBSET_DELETE'  ",
  "               then ",
  "                 case ",
  "                   when position('iud_type: index_' IN description) > 0 ",
  "                     then substring(description from position('iud_type: index_subset_delete ' IN description) + 30 ",
  "                          FOR (position(' ' IN substring(description FROM position('iud_type: index_subset_delete ' IN description) + 30))))   ",

  "                     else substring(description from position('iud_type: subset_delete ' IN description) + 24 ",
  "                    FOR (position(' ' IN substring(description FROM position('iud_type: subset_delete ' IN description) + 24))))   ",
  "                 end ",

  "               when operator = 'INSERT'  ",
  "               then ",
  "                 case ",
  "                   when position('iud_type: index_' IN description) > 0 ",
  "                     then substring(description from position('iud_type: index_insert ' IN description) + 23 ",
  "                          FOR (position(' ' IN substring(description FROM position('iud_type: index_insert ' IN description) + 23))))   ",

  "                     else substring(description from position('iud_type: insert ' IN description) + 17 ",
  "                    FOR (position(' ' IN substring(description FROM position('iud_type: insert ' IN description) + 17))))   ",
  "                 end ",
  "  end ",

  "  =  '%s' ",
  "    )  ",

  " ;"
};
#pragma warn(1313)  // warning elimination

short Usage::process(SqlciEnv * sqlci_env)
{
  short retcode = 0;
  Lng32 error = 0;

  NAString cmd;

  // a temporary file where the modules will be listed.
  // The filename will be of the form:  __mxci_usage__<juliantimestamp>
  // This temp file will be created in /tmp, if it exists. If not, then
  // it will be created in moduleDir_. If it cannot be written and read
  // in one of these two dirs, an error will be returned.

  NAString tempFile;
  // check for RW access in /tmp dir.
  tempFile = "/tmp";
  Int32 rc = ACCESS(tempFile, RW_ACCESS);
  if (rc == -1)
    {
      // now try module dir
      tempFile = moduleDir_;
      rc = ACCESS(tempFile, RW_ACCESS);
      ErrorParam ep_tmp("/tmp");
      ErrorParam ep_md(tempFile);
      if (rc == -1)
	{
	  SqlciError (2087,
		      &ep_tmp,
		      (ErrorParam *) 0
		      );
	  SqlciError (2087,
		      &ep_md,
		      (ErrorParam *) 0
		      );
	  return 0;
	}
    }

  Int64 ts = NA_JulianTimestamp();
  char tsBuf[100];
  convertInt64ToAscii(ts, tsBuf);

  //  tempFile = NAString(moduleDir_) + "/" + "__mxci_usage__" + tsBuf;
  tempFile = tempFile + "/" + "__mxci_usage__" + tsBuf;
  
  // remove the temporary file, if it exists
  cmd = "rm -f " + tempFile;
  system(cmd);

  // insert a backslash in front of any '$', single or double quote.
  // We are not handling any quoted double/single quotes.
  char * tempModule = new char[strlen(module_) * 2 + 1];
  UInt32 i = 0;
  Int32 j = 0;
  while (i < strlen(module_))
    {
      if ((module_[i] == '"') ||
	  (module_[i] == '\'') ||
	  (module_[i] == '$'))
	{
	  tempModule[j++] = '\\';
	}
      tempModule[j++] = module_[i++];
    }
  tempModule[j] = 0;

  // 'ls' the modules.
  NAString module(tempModule);
  NAString modDir;
  NABoolean modDirContainsWildcard = FALSE;
    {
      modDir = moduleDir_;
      if (modDir.contains('*'))
	modDirContainsWildcard = TRUE;
    }

  if ((module == "*") || (module == "*.*") || (module == "*.*.*"))
  {
    if (modDirContainsWildcard) 
      cmd = "find " + modDir + " -type f > " + tempFile;
    else
      cmd = "ls " + modDir + " > " + tempFile;
  }
  else
  {
    if (modDirContainsWildcard)
      cmd = "find " + modDir + " -type f -name \"" + module + "\" > " + tempFile;
	    
    else
      cmd = "ls " + modDir + "/" + module + " > " + tempFile;
  }  

  system(cmd);

  // read each module name from the temp file and process it.
  FILE * file_stream = fopen(tempFile, "r");
  if (! file_stream)
    {
      SqlciError (SQLCI_OBEY_FOPEN_ERROR,
		  new ErrorParam (errno),
		  new ErrorParam (tempFile),
		  (ErrorParam *) 0
		  );
      return 0;
    }
  
  char buf[2000];
  char * ok;

  char outBuf[200];

  Logfile *log = sqlci_env->get_logfile();

  SqlciList<ErrorModule> * errModList = new SqlciList<ErrorModule>;

  Int32 maxModNameLen = 0;
  NABoolean displayObjectName = TRUE;

  NABoolean done = FALSE;
  NABoolean usageInfoFound = FALSE;
  while (NOT done)
    {
      ok  = fgets(buf, 2000, file_stream);
      if (feof(file_stream))
	{
	  done = TRUE;
	  continue;
	}

      if (! ok)
	{
	  SqlciError (SQLCI_OBEY_FOPEN_ERROR,
		      new ErrorParam (errno),
		      new ErrorParam (tempFile),
		      (ErrorParam *) 0
		      );
	  delete errModList;
	  return 0;
	}

      // nuke the EOL character.
      buf[strlen(buf)-1]=0;
		//soln:10-050829-0917
      NAString modName;

      // if module parameter is omitted, then the names are displayed with
      // "ls <dir>" command. This command will NOT prepend the dir name.
      // So no need to skip the dirname out of the name returned in 'buf'.
      if ((module == '*') || (module == "*.*") || (module == "*.*.*") ||
	  (modDirContainsWildcard))
 	modName = buf;
      else
	modName = &buf[strlen(moduleDir_) + 1];

      // do not read files of length 0 or any file which starts with
      // "__mxci_usage__". The latter are internal temp files.
      if ((strlen(buf) <= 0) || 
	  (strncmp(modName.data(), "__mxci_usage__", strlen("__mxci_usage__") == 0)))
	continue;
	  //soln:10-050829-0917 Begin
	  // if the modName is not a 3-part name, skip this file. It is
      // not a module name.
      size_t pos = 0;
      Int32 noOfDots = 0;
      while (pos != NA_NPOS)
	{
	  pos = modName.index(".", pos);
	  if (pos != NA_NPOS)
	    {
	      noOfDots++;
	      pos++;
	    }
	}

      if (noOfDots != 2)
	continue;

      // create a new context in which the embedded statement will be
      // loaded and DUO'ed from.
      SQLCTX_HANDLE ctxtHandle;
      error = SQL_EXEC_CreateContext(&ctxtHandle,
				       NULL, // char *sqlAuthId
				       0     // Int32 suppressAutoXact
				       );
      if (error)
	{
	  HandleCLIError(error, sqlci_env);
	  
	  return 0;
	}

      SQLCTX_HANDLE prevCtxtHandle;
      error = SQL_EXEC_SwitchContext(ctxtHandle, &prevCtxtHandle);
      if (error)
	{
	  HandleCLIError(error, sqlci_env);
	  
	  return 0;
	}
//soln:10-050829-0917 End

      Int32 usage_query_size = 0;
      Int32 usage_qry_array_size = 0;
      if (table_)
	usage_qry_array_size =
	  sizeof(tableUsageQuery) / sizeof(char*);
      else
	usage_qry_array_size =
	  sizeof(moduleUsageQuery) / sizeof(char*);

      const char ** usageQuery = NULL;
      if (table_)
	usageQuery = tableUsageQuery;
      else
	usageQuery = moduleUsageQuery;

      Int32 i = 0;
      for (i = 0; i < usage_qry_array_size; i++)
	{
	  Int32 j = 0;
	  const char * usage_frag = usageQuery[i];
#pragma warning (disable : 4018)   //warning elimination
	  while ((j < strlen(usageQuery[i])) &&
		 (usage_frag[j] == ' '))
	    j++;
	  if (j < strlen(usageQuery[i]))
	    usage_query_size += strlen(&usage_frag[j]);
#pragma warning (default : 4018)   //warning elimination
	}
      
      char * source_fmt = new char[usage_query_size + 100];
      source_fmt[0] = 0;
      for (i = 0; i < usage_qry_array_size; i++)
	{
	  Int32 j = 0;
	  const char * usage_frag = usageQuery[i];
#pragma warning (disable : 4018)   //warning elimination
	  while ((j < strlen(usageQuery[i])) &&
		 (usage_frag[j] == ' '))
	    j++;
	  
 	  if (j < strlen(usageQuery[i]))
	    strcat(source_fmt, &usage_frag[j]);
#pragma warning (default : 4018)   //warning elimination
	}
      
      char * sqlStrBuf = NULL;
      NAString fullyQualifiedModName;
      
      if (modDirContainsWildcard)
	fullyQualifiedModName = modName;
      else
        fullyQualifiedModName = modName;
 
     if (table_)
	sqlStrBuf = new char[usage_query_size + strlen(fullyQualifiedModName.data()) + 3 * strlen(table_) + 50];
      else
	sqlStrBuf = new char[usage_query_size + strlen(fullyQualifiedModName.data()) + 50];
 	
      if (table_)
	sprintf(sqlStrBuf, source_fmt, fullyQualifiedModName.data(), table_, table_, table_);
      else
	sprintf(sqlStrBuf, source_fmt, fullyQualifiedModName.data());
      
      SqlCmd sqlCmd(SqlCmd::DML_TYPE, sqlStrBuf);
      PrepStmt * prepStmt = new PrepStmt("__MXCI_USAGE__");

      sqlCmd.clearCLIDiagnostics();

      retcode = sqlCmd.do_prepare(sqlci_env, prepStmt, get_sql_stmt(),
				  FALSE);

      Logfile *log = sqlci_env->get_logfile();
      Int32 numParams = 0;
      char * paramArray[10];
      retcode = sqlCmd.doExec(sqlci_env, 
			      prepStmt->getStmt(),
			      prepStmt,
			      numParams,
			      paramArray,
			      NULL,
			      FALSE);
      if (retcode < 0)
	{
	  if ((retcode == -8834) ||
	      (retcode == -8808) ||
	      (retcode == -8809) ||
	      ((retcode <= -8860) &&
	       (retcode >= -8866)))
	    {
	      ErrorModule * em = new ErrorModule(modName.data(), retcode);
	      errModList->append(em);
#pragma warning (disable : 4018)   //warning elimination
	      if (strlen(modName.data()) > maxModNameLen)
#pragma nowarn(1506)   // warning elimination 
		maxModNameLen = strlen(modName.data());
#pragma warn(1506)  // warning elimination 
#pragma warning (default : 4018)   //warning elimination
	    }
	  else
	    {
	      error = retcode;
	      HandleCLIError(error, sqlci_env);
	      
	      SqlciError (15992, (ErrorParam *) 0);
	    }
	}

      //      retcode = SQL_Success;
      NABoolean displayModuleName = TRUE;
      NABoolean firstTime = TRUE;
      while ((retcode >= 0) && 
	     (retcode != SQL_Eof))
	{
	  retcode = sqlCmd.doFetch(sqlci_env, 
				   prepStmt->getStmt(),
				   prepStmt,
				   FALSE);
	  if (retcode < 0)
	    {
	      if ((retcode == -8834) ||
		  (retcode == -8808) ||
		  (retcode == -8809) ||
		  ((retcode <= -8860) &&
		   (retcode >= -8866)))
		{
		  ErrorModule * em = new ErrorModule(modName.data(), retcode);
		  errModList->append(em);
#pragma warning (disable : 4018)   //warning elimination
		  if (strlen(modName.data()) > maxModNameLen)
#pragma nowarn(1506)   // warning elimination 
		    maxModNameLen = strlen(modName.data());
#pragma warn(1506)  // warning elimination 
#pragma warning (default : 4018)   //warning elimination
		}
	      else
		{
		  error = retcode;
		  HandleCLIError(error, sqlci_env);

		  SqlciError (15992, (ErrorParam *) 0);
		}
	    }
	  else if ((firstTime) &&
		   (retcode == SQL_Eof))
	    {
	      //SqlciError (-15992, (ErrorParam *) 0);
	    }
	  firstTime = FALSE;

	  if ((retcode != SQL_Eof) &&
	      (retcode >= 0))
	    {
	      usageInfoFound = TRUE;
	      if (table_)
		{
		  if (displayObjectName)
		    {
		      displayObjectName = FALSE;
		      sprintf(outBuf, "\n \nObject: %s\n", table_);
		      log->WriteAll(outBuf);
		    }

		  sqlCmd.displayRow(sqlci_env, prepStmt);
		  NAString oper(prepStmt->outputBuf());
		  oper = oper.strip();
		  char format[100];
		  if ((oper == "FILE_SCAN") ||
		      (oper == "FILE_SCAN_UNIQUE"))
		    {
		      sprintf(format, "  Table: %%-" PFSZ "s  Module: %%s", strlen(table_));
		      sprintf(outBuf, format, table_, modName.data());
		    }
		  else if ((oper == "INDEX_SCAN") ||
			   (oper == "INDEX_SCAN_UNIQUE"))
		    {
		      sprintf(format, "  Index: %%-" PFSZ "s  Module: %%s", strlen(table_));
		      sprintf(outBuf, format, table_, modName.data());
		    }
		  else
		    {
		      sprintf(format, "  Object: %%-" PFSZ "s  Module: %%s", strlen(table_));
		      sprintf(outBuf, format, table_, modName.data());
		    }
		    
		  log->WriteAll(outBuf);
		}
	      else
		{
		  if (displayModuleName)
		    {
		      displayModuleName = FALSE;
		      sprintf(outBuf, "\n \nModule: %s\n", modName.data());
		      log->WriteAll(outBuf);
		    }

		  sqlCmd.displayRow(sqlci_env, prepStmt);
		  NAString inTableName(prepStmt->outputBuf());
		  NAString outTableName;
		  size_t start = 0;
		  size_t end   = 0;
		  if (inTableName.index("Table: ", 0) == 0) 
		    {
		      start = inTableName.index(" ");
		      outTableName  = inTableName(start+1, inTableName.length() - start -1);
		      end = outTableName.index(" ");
		      outTableName = outTableName(0, end);
		      outTableName = "  Table: " + outTableName;
		    }
		  else if (inTableName.index("Index: ") == 0)
		    {
		      start = inTableName.index(" ");
		      outTableName  = inTableName(start+1, inTableName.length() - start -1);
		      end = outTableName.index(")");
		      if (end != NA_NPOS)
			{
			  outTableName = outTableName(0, end+1);
			}
		      else
			{
			  size_t lParen = outTableName.index("(");
			  end = outTableName.index(" ");
			  outTableName = outTableName(0, end);
			  if (lParen != NA_NPOS)
			    outTableName += ")";
			}
		      outTableName = "  Index: " + outTableName;
		    }
		  else
		    {
		      outTableName = "  Object: " + inTableName;
		    }
		  
		  log->WriteAll(outTableName.data());
		} // else
	    }
	} // while (retcode >= 0)
      
      if (retcode >= 0)
	retcode = (short)SQL_EXEC_CloseStmt(prepStmt->getStmt());
      
      if (prepStmt)
        sqlCmd.deallocate(sqlci_env, prepStmt);
      delete sqlStrBuf; 

      if (breakReceived)
        done = TRUE;
      // switch back to original context.
      error = SQL_EXEC_SwitchContext(prevCtxtHandle, NULL);
      if (error < 0)
	{
	  HandleCLIError(error, sqlci_env);
	}

      // and drop the new context that was created
      error = SQL_EXEC_DropContext(ctxtHandle);
      if (error < 0)
	{
	  HandleCLIError(error, sqlci_env);
	}
    } // while (NOT done)

  if ((retcode >= 0) && 
      (NOT usageInfoFound))
    SqlciError (-15992, (ErrorParam *) 0); // issue a warning.

  // print out the modules which got errors
  ErrorModule * em = errModList->getFirst();
  if (em)
    {
      sprintf(outBuf, "\n\nModules not loaded:\n");
      log->WriteAll(outBuf);
    }

  char format[100];
  sprintf(format, "  Module: %%-%ds   Error: %%d", maxModNameLen);
  while (em)
    {
      sprintf(outBuf, format, em->getModName(), em->getError());
      log->WriteAll(outBuf);

      em = errModList->getNext();
    }

  log->WriteAll("");

  fclose(file_stream);
  
  // remove the temporary file
  cmd = "rm -f " + tempFile;
  system(cmd);

  delete errModList;

  return 0;
}

