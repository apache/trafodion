/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
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
 * File:         CmpSeabaseDDLcommon.cpp
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
#include "CmpDDLCatErrorCodes.h"
#include "CmpSeabaseDDLupgrade.h"

// get software major and minor versions from -D defs defined in sqlcomp/Makefile.
// These defs pick up values from export vars defined in sqf/sqenvcom.sh.
#define SOFTWARE_MAJOR_VERSION TRAF_SOFTWARE_VERS_MAJOR
#define SOFTWARE_MINOR_VERSION TRAF_SOFTWARE_VERS_MINOR
#define SOFTWARE_UPDATE_VERSION TRAF_SOFTWARE_VERS_UPDATE

NABoolean CmpSeabaseMDupgrade::isOldMDtable(const NAString &objName)
{
  if (objName.contains(OLD_MD_EXTENSION))
    return TRUE;
  else
    return FALSE;
}

short CmpSeabaseMDupgrade::dropMDtables(ExpHbaseInterface *ehi, 
					NABoolean oldTbls,
					NABoolean useOldNameForNewTables)
{
  Lng32 retcode = 0;
  Lng32 errcode = 0;

  for (Lng32 i = 0; i < sizeof(allMDtablesInfo)/sizeof(MDTableInfo); i++)
    {
      const MDTableInfo &mdti = allMDtablesInfo[i];
      
      if ((NOT oldTbls) && (!mdti.newName))
	continue;
      
      HbaseStr hbaseTable;
      NAString extNameForHbase;
      extNameForHbase = TRAFODION_SYSCAT_LIT;
      extNameForHbase += ".";
      extNameForHbase += SEABASE_MD_SCHEMA;
      extNameForHbase += ".";

      if (oldTbls)
	{
	  if (useOldNameForNewTables)
	    {
	      if (! mdti.newName)
		continue;

	      extNameForHbase += mdti.newName;
	      extNameForHbase += OLD_MD_EXTENSION;
	    }
	  else
	    {
	      if (!mdti.oldName)
		continue;
	      
	      extNameForHbase += mdti.oldName;
	    }
	}
      else
	extNameForHbase += mdti.newName;
	
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

short CmpSeabaseMDupgrade::restoreOldMDtables(ExpHbaseInterface *ehi)
{
  Lng32 retcode = 0;
  Lng32 errcode = 0;

  // drop all the new MD tables. Ignore errors.
  dropMDtables(ehi, FALSE);

  for (Lng32 i = 0; i < sizeof(allMDtablesInfo)/sizeof(MDTableInfo); i++)
    {
      const MDTableInfo &mdti = allMDtablesInfo[i];
      
      if ((!mdti.newName) || (!mdti.oldName) ||
	  (mdti.addedTable))
	continue;
      
      HbaseStr oldHbaseTable;
      NAString extNameForOldHbase;
      extNameForOldHbase = TRAFODION_SYSCAT_LIT;
      extNameForOldHbase += ".";
      extNameForOldHbase += SEABASE_MD_SCHEMA;
      extNameForOldHbase += ".";
      extNameForOldHbase += mdti.oldName;
      oldHbaseTable.val = (char*)extNameForOldHbase.data();
      oldHbaseTable.len = extNameForOldHbase.length();
      
      HbaseStr currHbaseTable;
      NAString extNameForCurrHbase;
      extNameForCurrHbase = TRAFODION_SYSCAT_LIT;
      extNameForCurrHbase += ".";
      extNameForCurrHbase += SEABASE_MD_SCHEMA;
      extNameForCurrHbase += ".";
      extNameForCurrHbase += mdti.newName;
      currHbaseTable.val = (char*)extNameForCurrHbase.data();
      currHbaseTable.len = extNameForCurrHbase.length();
      
      retcode = copyHbaseTable(ehi, &oldHbaseTable/*src*/, &currHbaseTable/*tgt*/);
      if (retcode < 0)
	{
	  dropMDtables(ehi, FALSE);
	  return -1;
	}
    } // for
  
  
  return 0;
}

short CmpSeabaseMDupgrade::executeSeabaseMDupgrade(CmpMDupgradeInfo &mdui,
						   NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  char msgBuf[1000];
  char buf[2000];

  ExeCliInterface cliInterface(STMTHEAP);
  ExpHbaseInterface * ehi = NULL;

  while (1)
    {
      switch (mdui.step())
	{
	case UPGRADE_START: 
	  {
	    if (xnInProgress(&cliInterface))
	      {
		*CmpCommon::diags() << DgSqlCode(-20123);
		return -1;
	      }

	    if (mdui.getMDVersion())
	      {
		mdui.setStep(GET_MD_VERSION);
		mdui.setSubstep(0);
		break;
	      }

	    if (mdui.getSWVersion())
	      {
		mdui.setStep(GET_SW_VERSION);
		mdui.setSubstep(0);
		break;
	      }

	    mdui.setMsg("Metadata Upgrade: started");
	    mdui.setStep(VERSION_CHECK);
	    mdui.setSubstep(0);
	    mdui.setEndStep(TRUE);
	    
	    if (sendAllControlsAndFlags())
	      {
		mdui.setStep(UPGRADE_FAILED);
		mdui.setSubstep(0);
	      }
	    
	    return 0;
	  }
	  break;
	  
	case GET_MD_VERSION:
	  {
	    switch (mdui.subStep())
	      {
	      case 0:
		{
		  ehi = allocEHI(&ActiveSchemaDB()->getDefaults());
		  
		  Int64 mdCurrMajorVersion;
		  Int64 mdCurrMinorVersion;
		  retcode = validateVersions(&ActiveSchemaDB()->getDefaults(), ehi,
					     &mdCurrMajorVersion,
					     &mdCurrMinorVersion);
		  
		  deallocEHI(ehi);
		  
		  if (retcode == 0)
		    {
		      // no version mismatch detected.
		      // Metadata is uptodate.
		      mdui.setSubstep(1);
		    }
		  else if (retcode == -1395) // mismatch in MAJOR version. Need to upgrade
		    {
		      if ((mdCurrMajorVersion == METADATA_OLD_MAJOR_VERSION) &&
			  (mdCurrMinorVersion == METADATA_OLD_MINOR_VERSION))
			{
			  mdui.setSubstep(2);
			}
		      else
			{
			  // metadata cannot be upgraded with the current release.
			  mdui.setSubstep(3);
			}
		    }
		  else if (retcode == -1394) // mismatch in MAJOR version. Need to upgrade
		    {
		      // metadata cannot be upgraded with the current release.
		      mdui.setSubstep(3);
		    }
		  else
		    {
		      *CmpCommon::diags() << DgSqlCode(retcode);
		      mdui.setStep(DONE_RETURN);
		      
		      mdui.setSubstep(0);
		      mdui.setEndStep(TRUE);
		      break;
		    }
		  
		  str_sprintf(msgBuf, "  Current Version %Ld.%Ld. Expected Version %Ld.%Ld.",
			      mdCurrMajorVersion, mdCurrMinorVersion,
			      METADATA_MAJOR_VERSION, METADATA_MINOR_VERSION);   
		  mdui.setMsg(msgBuf);
		  mdui.setEndStep(FALSE);
		  
		  return 0;
		} // case 0
		break;
		
	      case 1:
		{
		  str_sprintf(msgBuf, "  Metadata is current. Upgrade is not needed.");
		  mdui.setMsg(msgBuf);
		  mdui.setEndStep(FALSE);
		  
		  mdui.setStep(DONE_RETURN);
		  mdui.setSubstep(0);
		  
		  return 0;
		}
		break;
		
	      case 2:
		{
		  str_sprintf(msgBuf, "  Metadata need to be upgraded or reinitialized.");
		  mdui.setMsg(msgBuf);
		  mdui.setEndStep(FALSE);
		  
		  mdui.setStep(DONE_RETURN);
		  mdui.setSubstep(0);
		  
		  return 0;
		}
		break;
		
	      case 3:
		{
		  str_sprintf(msgBuf, "  Metadata cannot be upgraded with this version of software.");
		  mdui.setMsg(msgBuf);
		  mdui.setEndStep(FALSE);
		  
		  mdui.setSubstep(4);
		  
		  return 0;
		}
		break;

	      case 4:
		{
		  str_sprintf(msgBuf, "  Install previous version of software or reinitialize metadata.");
		  mdui.setMsg(msgBuf);
		  mdui.setEndStep(FALSE);
		  
		  mdui.setStep(DONE_RETURN);
		  mdui.setSubstep(0);
		  
		  return 0;
		}
		break;

	      } // switch
	  } // GET_MD_VERSION
	  break;
	  
	case GET_SW_VERSION:
	  {
	    switch (mdui.subStep())
	      {
	      case 0:
		{
		  ehi = allocEHI(&ActiveSchemaDB()->getDefaults());
		  
		  Int64 sysSWMajorVersion;
		  Int64 sysSWMinorVersion;
                  Int64 sysSWUpdVersion;
                  Int64 mdSWMajorVersion;
                  Int64 mdSWMinorVersion;
		  retcode = validateVersions(&ActiveSchemaDB()->getDefaults(), ehi,
                                             NULL, NULL,
					     &sysSWMajorVersion,
					     &sysSWMinorVersion,
                                             &sysSWUpdVersion,
                                             &mdSWMajorVersion,
                                             &mdSWMinorVersion);
		  
		  deallocEHI(ehi);
		  
		  if (retcode == 0)
		    {
		      // no version mismatch detected between system and expected software.
                      if ((mdSWMajorVersion == sysSWMajorVersion) &&
                          (mdSWMinorVersion == sysSWMinorVersion))
                        // software version stored in metadata is uptodate
                        mdui.setSubstep(1);
                      else
                        // metadata need to be updated with current software version.
                        mdui.setSubstep(2);
		    }
		  else if (retcode == -1397) // mismatch in software version
		    {
                      mdui.setSubstep(3);
		    }

                  Int64 expSWMajorVersion = SOFTWARE_MAJOR_VERSION;
                  Int64 expSWMinorVersion = SOFTWARE_MINOR_VERSION;
                  Int64 expSWUpdVersion = SOFTWARE_UPDATE_VERSION;
		  str_sprintf(msgBuf, "  System Version %Ld.%Ld.%Ld. Expected Version %Ld.%Ld.%Ld. Metadata Version %Ld.%Ld",
			      sysSWMajorVersion, sysSWMinorVersion, sysSWUpdVersion,
			      expSWMajorVersion, expSWMinorVersion, expSWUpdVersion,
                              mdSWMajorVersion, mdSWMinorVersion);
		  mdui.setMsg(msgBuf);
		  mdui.setEndStep(FALSE);
		  
		  return 0;
		} // case 0
		break;
		
	      case 1:
		{
		  str_sprintf(msgBuf, "  Software is current.");
		  mdui.setMsg(msgBuf);
		  mdui.setEndStep(FALSE);
		  
		  mdui.setStep(DONE_RETURN);
		  mdui.setSubstep(0);
		  
		  return 0;
		}
		break;
		
	      case 2:
		{
		  str_sprintf(msgBuf, "  Metadata need to be updated with current software version. Run 'initialize trafodion, update software version' to update it.");
		  mdui.setMsg(msgBuf);
		  mdui.setEndStep(FALSE);
		  
		  mdui.setStep(DONE_RETURN);
		  mdui.setSubstep(0);
		  
		  return 0;
		}
		break;

	      case 3:
		{
		  str_sprintf(msgBuf, "  Version of software being used is not compatible with version of software on the system.");
		  mdui.setMsg(msgBuf);
		  mdui.setEndStep(FALSE);
		  
		  mdui.setStep(DONE_RETURN);
		  mdui.setSubstep(0);
		  
		  return 0;
		}
		break;
		
	      } // switch
	  } // GET_SW_VERSION
	  break;
	  
	case VERSION_CHECK:
	  {
	    switch (mdui.subStep())
	      {
	      case 0:
		{
		  mdui.setMsg("Version Check: started");
		  mdui.subStep()++;
		  mdui.setEndStep(FALSE);

		  return 0;
		}
		break;

	      case 1:
		{
		  // check if upgrade has already been done.
		  ehi = allocEHI(&ActiveSchemaDB()->getDefaults());

		  Int64 mdCurrMajorVersion;
		  Int64 mdCurrMinorVersion;
		  retcode = validateVersions(&ActiveSchemaDB()->getDefaults(), ehi,
					     &mdCurrMajorVersion,
					     &mdCurrMinorVersion);

		  deallocEHI(ehi);

		  if (retcode == 0)
		    {
		      // no version mismatch detected.
		      // Metadata is uptodate.
		      str_sprintf(msgBuf, "  Metadata is already at Version %Ld.%Ld. Upgrade is not needed.",
				  mdCurrMajorVersion, mdCurrMinorVersion);
		      mdui.setMsg(msgBuf);
		      mdui.setEndStep(FALSE);

		      mdui.setSubstep(2);
		      
		      return 0;
		    }
		  else if (retcode == -1395) // mismatch in MAJOR version. Need to upgrade
		    {
		      if ((mdCurrMajorVersion == METADATA_OLD_MAJOR_VERSION) &&
			  (mdCurrMinorVersion == METADATA_OLD_MINOR_VERSION))
			{
			  str_sprintf(msgBuf, "  Metadata need to be upgraded from Version %Ld.%Ld to %Ld.%Ld",
				      mdCurrMajorVersion, mdCurrMinorVersion,
				      METADATA_MAJOR_VERSION, METADATA_MINOR_VERSION);
			  
			  mdui.setMsg(msgBuf);
			  
			  mdui.setSubstep(3);
			  
			  return 0;
			}
		      else
			{
			  // metadata cannot be upgraded with the current release.
			  str_sprintf(msgBuf, "  Metadata cannot to be upgraded from Version %Ld.%Ld to %Ld.%Ld with this software",
				      mdCurrMajorVersion, mdCurrMinorVersion,
				      METADATA_MAJOR_VERSION, METADATA_MINOR_VERSION);
			  
			  mdui.setMsg(msgBuf);
			  
			  mdui.setSubstep(4);
			  
			  return 0;
			}
		    }
		  else
		    {
		      *CmpCommon::diags() << DgSqlCode(retcode);
		      mdui.setStep(UPGRADE_DONE);

		      mdui.setSubstep(0);
		      mdui.setEndStep(TRUE);
		    }

		  return 0;
		}
		break;

	      case 2:
		{
		  mdui.setMsg("Version Check: done");
		  mdui.setSubstep(0);
		  mdui.setEndStep(TRUE);

		  mdui.setStep(UPGRADE_DONE);

		  return 0;
		}
		break;

	      case 3:
		{
		  mdui.setMsg("Version Check: done");
		  mdui.setSubstep(0);
		  mdui.setEndStep(TRUE);

		  mdui.setStep(OLD_MD_DROP_PRE);

		  return 0;
		}
		break;

	      case 4:
		{
		  mdui.setMsg("Version Check: done");
		  mdui.setSubstep(0);
		  mdui.setEndStep(TRUE);

		  mdui.setStep(UPGRADE_FAILED);

		  return 0;
		}
		break;

	      } // case
	  } // case VERSION_CHECK
	  break;

	case OLD_MD_DROP_PRE:
	  {
	    switch (mdui.subStep())
	      {
	      case 0:
		{
		  mdui.setMsg("Drop Old Metadata: started");
		  mdui.subStep()++;
		  mdui.setEndStep(FALSE);

		  return 0;
		}
		break;
		
	      case 1:
		{
		  ehi = allocEHI();

		  //
		  // drop metadata tables using hbase drop command
		  //
		  if (dropMDtables(ehi, TRUE)) // drop old tables
		    {
		      deallocEHI(ehi); 

		      mdui.setStep(UPGRADE_FAILED);
		      mdui.setSubstep(0);

		      break;
		    }

		  deallocEHI(ehi); 

		  mdui.setMsg("Drop Old Metadata: done");
		  mdui.setStep(CURR_MD_BACKUP);
		  mdui.setSubstep(0);
		  mdui.setEndStep(TRUE);

		  return 0;
		}
		break;
	      } // case
	  }
	  break;

	case CURR_MD_BACKUP:
	  {
	    switch (mdui.subStep())
	      {
	      case 0:
		{
		  mdui.setMsg("Backup Current Metadata: started");
		  mdui.subStep()++;
		  mdui.setEndStep(FALSE);

		  return 0;
		}
		break;
		
	      case 1:
		{
		  //
		  // backup current metadata using hbase snapshot command
		  //
		  ehi = allocEHI();

		  for (Lng32 i = 0; i < sizeof(allMDtablesInfo)/sizeof(MDTableInfo); i++)
		    {
		      const MDTableInfo &mdti = allMDtablesInfo[i];

		      //		      if ((!mdti.newName) || (!mdti.oldName))
		      if (mdti.addedTable)
			continue;

		      HbaseStr currHbaseTable;
		      NAString extNameForCurrHbase;
		      extNameForCurrHbase = TRAFODION_SYSCAT_LIT;
		      extNameForCurrHbase += ".";
		      extNameForCurrHbase += SEABASE_MD_SCHEMA;
		      extNameForCurrHbase += ".";
		      extNameForCurrHbase += mdti.newName;
		      currHbaseTable.val = (char*)extNameForCurrHbase.data();
		      currHbaseTable.len = extNameForCurrHbase.length();
		      
		      HbaseStr oldHbaseTable;
		      NAString extNameForOldHbase;
		      extNameForOldHbase = TRAFODION_SYSCAT_LIT;
		      extNameForOldHbase += ".";
		      extNameForOldHbase += SEABASE_MD_SCHEMA;
		      extNameForOldHbase += ".";
		      extNameForOldHbase += mdti.oldName;
		      oldHbaseTable.val = (char*)extNameForOldHbase.data();
		      oldHbaseTable.len = extNameForOldHbase.length();
		      
		      retcode = copyHbaseTable(ehi, &currHbaseTable, &oldHbaseTable);
		      if (retcode < 0)
			{
			  mdui.setStep(UPGRADE_FAILED_DROP_OLD_MD);
			  mdui.setSubstep(0);
			  break;
			}
		      
		    } // for

		  if (mdui.step() != CURR_MD_BACKUP)
		    break;

		  deallocEHI(ehi); 

		  mdui.setMsg("Backup Current Metadata: done");
		  mdui.setStep(CURR_MD_DROP);
		  mdui.setSubstep(0);
		  mdui.setEndStep(TRUE);

		  return 0;
		}
		break;
	      } // case
	    
	  }
	  break;

	case CURR_MD_DROP:
	  {
	    switch (mdui.subStep())
	      {
	      case 0:
		{
		  mdui.setMsg("Drop Current Metadata: started");
		  mdui.subStep()++;
		  mdui.setEndStep(FALSE);

		  return 0;
		}
		break;
		
	      case 1:
		{
		  ehi = allocEHI();

		  //
		  // drop metadata tables using hbase drop command
		  //
		  if (dropMDtables(ehi, FALSE)) // drop curr/new MD tables
		    {
		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);
		      break;
		    }

		  // cleanup cached entries in client object.
		  ehi->cleanupClient();

		  deallocEHI(ehi); 

		  mdui.setMsg("Drop Current Metadata: done");
		  mdui.setStep(INITIALIZE_TRAF);
		  mdui.setSubstep(0);
		  mdui.setEndStep(TRUE);

		  return 0;
		}
		break;
	      } // case
	  }
	  break;

	case INITIALIZE_TRAF:
	  {
	    switch (mdui.subStep())
	      {
	      case 0:
		{
		  if (xnInProgress(&cliInterface))
		    {
		      *CmpCommon::diags() << DgSqlCode(-20123);

		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);
		      break;
		    }

		  mdui.setMsg("Initialize New Metadata: started");
		  mdui.subStep()++;
		  mdui.setEndStep(FALSE);

		  return 0;
		}
		break;
		
	      case 1:
		{
		  //
		  // initialize trafodion
		  CmpCommon::context()->setIsUninitializedSeabase(TRUE);
		  CmpCommon::context()->uninitializedSeabaseErrNum() = -1393; // MD doesn't exist

		  str_sprintf(buf, "initialize trafodion;");
		  
		  cliRC = cliInterface.executeImmediate(buf);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
		      
		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);
		      break;
		    }

		  if (xnInProgress(&cliInterface))
		    {
		      *CmpCommon::diags() << DgSqlCode(-20123);

		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);
		      break;
		    }

		  // After initialize, VERSIONS table contain the new metadata version
		  // values. Delete these values since the upgrade process is not yet done.
		  // At the end of upgrade, VERSIONS table will be updated with the
		  // new version values.
		  cliRC = beginXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);
		      break;
		    }

		  str_sprintf(buf, "delete from %s.\"%s\".%s;",
			      TRAFODION_SYSCAT_LIT,
			      SEABASE_MD_SCHEMA,
			      SEABASE_VERSIONS);
		  cliRC = cliInterface.executeImmediate(buf);

		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
		      
		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);
		      break;
		    }

		  cliRC = commitXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
		      
		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);

		      break;
		    }

		  mdui.setMsg("Initialize New Metadata: done");
		  mdui.setStep(COPY_MD_TABLES_PROLOGUE);
		  mdui.setSubstep(0);
		  mdui.setEndStep(TRUE);

		  return 0;
		}
		break;
	      } // case
	  }
	  break;

	case COPY_MD_TABLES_PROLOGUE:
	  {
	    mdui.setMsg("Copy Old Metadata: started");
	    mdui.setStep(COPY_MD_TABLES);
	    mdui.setSubstep(0);
	    mdui.setEndStep(FALSE);

	    if (xnInProgress(&cliInterface))
	      {
		*CmpCommon::diags() << DgSqlCode(-20123);

		mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		mdui.setSubstep(0);

		break;
	      }

	    cliRC = beginXn(&cliInterface);
	    if (cliRC < 0)
	      {
		cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

		mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		mdui.setSubstep(0);

		break;
	      }

	    return 0;
	  }
	  break;

	case COPY_MD_TABLES:
	  {
	    const MDTableInfo &mdti = allMDtablesInfo[mdui.subStep()];
	  
	    if ((mdui.subStep() < sizeof(allMDtablesInfo)/sizeof(MDTableInfo)) &&
		((NOT mdti.addedTable) && (NOT mdti.droppedTable) && (NOT mdti.isIndex)))
	      {
		str_sprintf(buf, "upsert into %s.\"%s\".%s %s%s%s select %s from %s.\"%s\".%s SRC %s;",
			    TRAFODION_SYSCAT_LIT,
			    SEABASE_MD_SCHEMA,
			    mdti.newName, 
			    (mdti.insertedCols ? "(" : ""),
			    (mdti.insertedCols ? mdti.insertedCols : ""),
			    (mdti.insertedCols ? ")" : ""),
			    (mdti.selectedCols ? mdti.selectedCols : "*"),
			    TRAFODION_SYSCAT_LIT,
			    SEABASE_MD_SCHEMA,
			    mdti.oldName,
			    (mdti.wherePred ? mdti.wherePred : ""));

		// copy this table as is.
		if (NOT mdui.startStep())
		  {
		    str_sprintf(msgBuf, "  Start: Copy %20s ==> %s",
				mdti.oldName, mdti.newName);
		      
		    mdui.setStartStep(TRUE);
		    mdui.setEndStep(FALSE);

		    //mdui.setMsg(buf);
		    mdui.setMsg(msgBuf);

		    break;
		    //return 0;
		  }
		
		mdui.setStartStep(FALSE);
		mdui.setEndStep(TRUE);

		// execute the insert...select
		cliRC = cliInterface.executeImmediate(buf);
		if (cliRC < 0)
		  {
		    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
		    
		    mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		    mdui.setSubstep(0);

		    break;
		  }
		
		str_sprintf(msgBuf, "  End:   Copy %20s ==> %s",
			    mdti.oldName, mdti.newName);
		
		mdui.setMsg(msgBuf);
		mdui.subStep()++;

		break;
		//return 0;
	      } // if

	    mdui.subStep()++;
	    if (mdui.subStep() >= sizeof(allMDtablesInfo)/sizeof(MDTableInfo))
	      {
		mdui.setStep(COPY_MD_TABLES_EPILOGUE);
		mdui.setSubstep(0);
	      }

	  }
	  break;
	  
	case COPY_MD_TABLES_EPILOGUE:
	  {
	    mdui.setMsg("Copy Old Metadata: done");
	    mdui.setStep(VALIDATE_DATA_COPY);
	    mdui.setSubstep(0);
	    mdui.setEndStep(TRUE);

	    cliRC = commitXn(&cliInterface);
	    if (cliRC < 0)
	      {
		cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
		
		mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		mdui.setSubstep(0);

		break;
	      }

	    return 0;
	  }
	  break;

	case VALIDATE_DATA_COPY:
	  {
	    switch (mdui.subStep())
	      {
	      case 0:
		{
		  mdui.setMsg("Validate Metadata Copy: started");
		  mdui.subStep()++;
		  mdui.setEndStep(FALSE);

		  return 0;
		}
		break;
		
	      case 1:
		{
		  //
		  // validate that data got copied. Do some counts, etc.
		  //
		  // TBD
		  //
		  mdui.setMsg("Validate Metadata Copy: done");
		  mdui.setStep(CUSTOMIZE_NEW_MD);
		  mdui.setSubstep(0);
		  mdui.setEndStep(TRUE);

		  return 0;
		}
		break;
	      } // case
	  }
	  break;

	case CUSTOMIZE_NEW_MD:
	  {
	    switch (mdui.subStep())
	      {
	      case 0:
		{
		  mdui.setMsg("Customize New Metadata: started");
		  mdui.subStep()++;
		  mdui.setEndStep(FALSE);

		  // for V11 to V21, customization is needed to populate primary key
		  // constraint info and view text info.
		  // For other upgrades, it is not needed.
		  // Customize this section as needed.
		  if (NOT ((METADATA_OLD_MAJOR_VERSION == 1) && (METADATA_OLD_MINOR_VERSION == 1) &&
			   (METADATA_MAJOR_VERSION == 2) && (METADATA_MINOR_VERSION == 1)))
		    {
		      mdui.setSubstep(5);
		      mdui.setEndStep(FALSE);

		      return 0;
		    }

		  if (xnInProgress(&cliInterface))
		    {
		      *CmpCommon::diags() << DgSqlCode(-20123);

		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);

		      break;
		    }
		  
		  return 0;
		}
		break;
		
	      case 1:
		{
		  mdui.setMsg("  Start: Populate Primary Key Constraint");
		  mdui.subStep()++;

		  cliRC = beginXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);

		      break;
		    }
		  
		  return 0;
		}
		break;

	      case 2:
		{
		  str_sprintf(buf, "select o.catalog_name, o.schema_name, object_name, count(*) from %s.\"%s\".%s o, %s.\"%s\".%s k where o.object_uid = k.object_uid and schema_name != '_MD_' and object_type = 'BT'  group by k.object_uid, o.catalog_name, o.schema_name, o.object_name having (k.object_uid, count(*)) = (select k2.object_uid, count(*) from %s.\"%s\".%s k2 where k2.column_name != 'SYSKEY' and k2.object_uid = k.object_uid group by k2.object_uid)",
			      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
			      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
			      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS);
		  
		  Queue * keysQueue = NULL;
		  cliRC = cliInterface.fetchAllRows(keysQueue, buf, 0, FALSE, FALSE, TRUE);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);

		      break;
		    }
		  
		  keysQueue->position();
		  for (int idx = 0; idx < keysQueue->numEntries(); idx++)
		    {
		      OutputInfo * oi = (OutputInfo*)keysQueue->getNext(); 

		      char * catName = (char*)oi->get(0);
		      char * schName = (char*)oi->get(1);
		      char * objName  = (char*)oi->get(2);
     
 		      Int64 colCount;
		      colCount = *(Int64*)oi->get(3);
		      
		      Int64 pkeyUID = 0;
		      Int64 tableUID = 0;
		      if (updatePKeyInfo(NULL, catName, schName, objName, colCount, 
					 &pkeyUID, &tableUID, NULL, &cliInterface))
			{
			  mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
			  mdui.setSubstep(0);

			  break;
			}

		      str_sprintf(buf, "insert into %s.\"%s\".%s select %Ld, column_name, keyseq_number, column_number, ordering, nonkeycol from %s.\"%s\".%s where object_uid = %Ld",
				  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
				  pkeyUID,
				  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
				  tableUID);
		      cliRC = cliInterface.executeImmediate(buf);
		      
		      if (cliRC < 0)
			{
			  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
			  
			  *CmpCommon::diags() << DgSqlCode(-1423)
					      << DgString0(SEABASE_KEYS);
			  
			  mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
			  mdui.setSubstep(0);

			  break;
			}
		    } // for

		  if (mdui.step() != CUSTOMIZE_NEW_MD)
		    break;

		  mdui.setMsg("  End: Populate Primary Key Constraint");
		  mdui.subStep()++;

		  if (xnInProgress(&cliInterface))
		    {
		      cliRC = commitXn(&cliInterface);
		      if (cliRC < 0)
			{
			  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

			  mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
			  mdui.setSubstep(0);

			  break;
			}
		    }
		  
		  return 0;
		}
		break;

	      case 3:
		{
		  mdui.setMsg("  Start: Populate View Text");
		  mdui.subStep()++;

		  cliRC = beginXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);

		      break;
		    }

		  return 0;
		}
		break;

	      case 4:
		{
		  str_sprintf(buf, "select view_uid, view_text from %s.\"%s\".%s",
			      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_VIEWS_OLD_MD);
		  
		  Queue * viewsQueue = NULL;
		  cliRC = cliInterface.fetchAllRows(viewsQueue, buf, 0, FALSE, FALSE, TRUE);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
		      
		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);

		      break;
		    }

		  viewsQueue->position();
		  for (int idx = 0; idx < viewsQueue->numEntries(); idx++)
		    {
		      OutputInfo * oi = (OutputInfo*)viewsQueue->getNext(); 

		      Int64 viewUID = *(Int64*)oi->get(0);
		      NAString currViewText((char*)oi->get(1));

		      NAString viewText(STMTHEAP);
		      for (Lng32 i = 0; i < currViewText.length(); i++)
			{
			  if (currViewText.data()[i] == '\'')
			    viewText += "''";
			  else
			    viewText += currViewText.data()[i];
			}

		      Lng32 viewTextLen = viewText.length();
		      Lng32 numRows = (viewTextLen / TEXTLEN) + 1;
		      Lng32 currPos = 0;
		      for (Lng32 i = 0; i < numRows; i++)
			{
			  NAString temp = 
			    (i < numRows-1 ? viewText(currPos, TEXTLEN)
			     : viewText(currPos, (viewTextLen - currPos)));
			  
			  str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, %d, '%s')",
				      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TEXT,
				      viewUID,
				      i,
				      temp.data());
			  cliRC = cliInterface.executeImmediate(buf);
			  
			  if (cliRC < 0)
			    {
			      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

			      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
			      mdui.setSubstep(0);

			      break;
			    }
			  
			  currPos += TEXTLEN;
			}
		    }

		  if (mdui.step() != CUSTOMIZE_NEW_MD)
		    break;

		  mdui.setMsg("  End: Populate View Text");
		  mdui.subStep()++;

		  if (xnInProgress(&cliInterface))
		    {
		      cliRC = commitXn(&cliInterface);
		      if (cliRC < 0)
			{
			  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

			  mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
			  mdui.setSubstep(0);

			  break;
			}
		    }
		  
		  return 0;
		}
		break;

	      case 5:
		{
		  mdui.setMsg("Customize New Metadata: done");
		  mdui.setStep(OLD_TABLES_MD_DELETE);
		  mdui.setSubstep(0);
		  mdui.setEndStep(TRUE);
		  
		  return 0;
		}
		break;
	      }
	  }
	  break;

	case OLD_TABLES_MD_DELETE:
	  {
	    switch (mdui.subStep())
	      {
	      case 0:
		{
		  if (xnInProgress(&cliInterface))
		    {
		      *CmpCommon::diags() << DgSqlCode(-20123);

		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);

		      break;
		    }
		  
		  cliRC = beginXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);

		      break;
		    }

		  mdui.setMsg("Delete Old Metadata Info: started");
		  mdui.subStep()++;
		  mdui.setEndStep(FALSE);

		  return 0;
		}
		break;
		
	      case 1:
		{
		  ehi = allocEHI();

		  // drop info about old tables from the new metadata.
		  // do not drop the actual hbase tables. 
		  for (Lng32 i = 0; i < sizeof(allMDtablesInfo)/sizeof(MDTableInfo); i++)
		    {
		      const MDTableInfo &mdti = allMDtablesInfo[i];

		      if (! mdti.oldName)
			continue;

		      NAString catName(TRAFODION_SYSCAT_LIT);
		      NAString schName;
		      schName += "\"";
		      schName += SEABASE_MD_SCHEMA;
		      schName += "\"";
		      if (dropSeabaseObject(ehi, mdti.oldName,
					    catName, schName,
					    (mdti.isIndex ? COM_INDEX_OBJECT_LIT : COM_BASE_TABLE_OBJECT_LIT),
					    TRUE, FALSE))
			{
			  deallocEHI(ehi); 

			  mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
			  mdui.setSubstep(0);

			  break;
			}
		    } // for

		  if (mdui.step() != OLD_TABLES_MD_DELETE)
		    break;

		  // drop info about old metadata views from the new metadata.
		  for (Int32 i = 0; i < sizeof(allMDviewsInfo)/sizeof(MDViewInfo); i++)
		    {
		      const MDViewInfo &mdi = allMDviewsInfo[i];
		      
		      if (! mdi.viewName)
			continue;
		      
		      NAString oldViewName(mdi.viewName);
		      oldViewName += OLD_MD_EXTENSION;
		      
		      NAString catName(TRAFODION_SYSCAT_LIT);
		      NAString schName;
		      schName += "\"";
		      schName += SEABASE_MD_SCHEMA;
		      schName += "\"";

		      Int64 objUID = getObjectUID(&cliInterface,
						  TRAFODION_SYSCAT_LIT, SEABASE_MD_SCHEMA,
						  oldViewName.data(),
						  //						  catName.data(), schName.data(), oldViewName.data(),
						  COM_VIEW_OBJECT_LIT);
		      if (objUID < 0)
			{
			  // could not find the view or an error.
			  // Ignore and continue.
			  CmpCommon::diags()->clear();
			  continue;
			}

		      if (dropSeabaseObject(ehi, oldViewName,
					    catName, schName,
					    COM_VIEW_OBJECT_LIT))
			{
			  deallocEHI(ehi); 

			  mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
			  mdui.setSubstep(0);

			  break;
			}
		    } // for

		  if (mdui.step() != OLD_TABLES_MD_DELETE)
		    break;

		  deallocEHI(ehi); 

		  mdui.setMsg("Delete Old Metadata Info: done");
		  mdui.setStep(UPDATE_VERSION);
		  mdui.setSubstep(0);
		  mdui.setEndStep(TRUE);

		  if (xnInProgress(&cliInterface))
		    {
		      cliRC = commitXn(&cliInterface);
		      if (cliRC < 0)
			{
			  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
			  
			  mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
			  mdui.setSubstep(0);

			  break;
			}
		    }

		  return 0;
		}
		break;
	      } // case

	  }
	  break;

	case UPDATE_VERSION:
	  {
	    switch (mdui.subStep())
	      {
	      case 0:
		{
		  mdui.setMsg("Update Metadata Version: started");
		  mdui.subStep()++;
		  mdui.setEndStep(FALSE);

		  return 0;
		}
		break;
		
	      case 1:
		{
		  if (xnInProgress(&cliInterface))
		    {
		      *CmpCommon::diags() << DgSqlCode(-20123);

		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);

		      break;
		    }
		  
		  cliRC = beginXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);

		      break;
		    }
		  
		  cliRC = updateSeabaseVersions(&cliInterface, TRAFODION_SYSCAT_LIT);
		  if (cliRC < 0)
		    {
		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);

		      break;
		    }

		  cliRC = commitXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
		    
		      mdui.setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui.setSubstep(0);

		      break;
		    }
		  
		  mdui.setMsg("Update Metadata Version: done");
		  mdui.setStep(OLD_MD_TABLES_HBASE_DELETE);
		  mdui.setSubstep(0);
		  mdui.setEndStep(TRUE);

		  return 0;
		}
		break;
	      } // case
	  }
	  break;

	case OLD_MD_TABLES_HBASE_DELETE:
	  {
	    switch (mdui.subStep())
	      {
	      case 0:
		{
		  if (xnInProgress(&cliInterface))
		    {
		      *CmpCommon::diags() << DgSqlCode(-20123);

		      mdui.setStep(METADATA_UPGRADED);
		      break;
		    }
		  
		  cliRC = beginXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

		      mdui.setStep(METADATA_UPGRADED);
		      break;
		    }

		  mdui.setMsg("Drop Old Metadata from Hbase: started");
		  mdui.subStep()++;
		  mdui.setEndStep(FALSE);

		  return 0;
		}
		break;
		
	      case 1:
		{
		  ehi = allocEHI();

		  // drop old tables from hbase.
		  for (Lng32 i = 0; i < sizeof(allMDtablesInfo)/sizeof(MDTableInfo); i++)
		    {
		      const MDTableInfo &mdti = allMDtablesInfo[i];

		      if (! mdti.oldName)
			continue;

		      NAString catName(TRAFODION_SYSCAT_LIT);
		      NAString schName;
		      schName += "\"";
		      schName += SEABASE_MD_SCHEMA;
		      schName += "\"";
		      if (dropSeabaseObject(ehi, mdti.oldName,
					    catName, schName,
					    (mdti.isIndex ? COM_INDEX_OBJECT_LIT : COM_BASE_TABLE_OBJECT_LIT),
					    FALSE, TRUE))
			{
			  // ignore errors. Continue dropping old md tables.
			}
		    } // for

		  deallocEHI(ehi); 

		  mdui.setMsg("Drop Old Metadata from Hbase: done");
		  mdui.setStep(METADATA_UPGRADED);
		  mdui.setSubstep(0);
		  mdui.setEndStep(TRUE);

		  if (xnInProgress(&cliInterface))
		    {
		      cliRC = commitXn(&cliInterface);
		      if (cliRC < 0)
			{
			  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

			  mdui.setStep(METADATA_UPGRADED);
			  break;
			}
		    }

		  return 0;
		}
		break;
	      } // case

	  }
	  break;

	case METADATA_UPGRADED:
	  {
	    str_sprintf(msgBuf, "Metadata Upgrade to Version %Ld.%Ld: done",
				  METADATA_MAJOR_VERSION, METADATA_MINOR_VERSION);

	    mdui.setMsg(msgBuf);
	    mdui.setDone(TRUE);

	    return 0;
	  }
	  break;

	case UPGRADE_DONE:
	  {
	    mdui.setMsg("Metadata Upgrade: done");
	    mdui.setDone(TRUE);

	    return 0;
	  }
	  break;

	case DONE_RETURN:
	  {
	    mdui.setMsg("");
	    mdui.setDone(TRUE);

	    return 0;
	  }
	  break;

	case UPGRADE_FAILED:
	case UPGRADE_FAILED_RESTORE_OLD_MD:
	case UPGRADE_FAILED_DROP_OLD_MD:
	  {
	    switch (mdui.subStep())
	      {
	      case 0:
		{
		  if (mdui.step() == UPGRADE_FAILED_DROP_OLD_MD)
		    {
		      mdui.setSubstep(1);
		      break;
		    }
		  else if (mdui.step() == UPGRADE_FAILED_RESTORE_OLD_MD)
		    {
		      mdui.setSubstep(4);
		      break;
		    }
		  else
		    {
		      mdui.setSubstep(7);
		      break;
		    }
		}
		break;
		
	      case 1:
		{
		  mdui.setMsg("Drop Old Metadata: started");
		  mdui.subStep()++;
		  mdui.setEndStep(FALSE);
		  
		  return 0;
		}
		break;

	      case 2:
		{
		  if (ehi == NULL)
		    ehi = allocEHI();
		  
		  dropMDtables(ehi, TRUE); // drop old MD
		  deallocEHI(ehi);
		  
		  mdui.subStep()++;
		  mdui.setEndStep(FALSE);
		}
		break;

	      case 3:
		{
		  mdui.setMsg("Drop Old Metadata: done");
		  mdui.setSubstep(7);
		  mdui.setEndStep(TRUE);

		  return 0;
		}
		break;

	      case 4:
		{
		  mdui.setMsg("Restore from Old Metadata: started");
		  mdui.subStep()++;
		  mdui.setEndStep(FALSE);

		  return 0;
		}
		break;

	      case 5:
		{
		  if (ehi == NULL)
		    ehi = allocEHI();
		  
		  restoreOldMDtables(ehi); // restore old MD and make them current
		  deallocEHI(ehi);

		  mdui.subStep()++;
		  mdui.setEndStep(FALSE);
		}
		break;
		
	      case 6:
		{
		  mdui.setMsg("Restore from Old Metadata: done");
		  mdui.setSubstep(1); // now drop old metadata tables
		  mdui.setEndStep(TRUE);
		  
		  return 0;
		}
		break;

	      case 7: 
		{
		  deallocEHI(ehi);
		  
		  mdui.setMsg("Metadata Upgrade: failed");
		  mdui.setDone(TRUE);
		  
		  return 0;
		}
		break;
	      } // switch
	  } 
	  break;

	} // step
    } // while

  return 0;
}

