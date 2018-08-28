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
 * File:         CmpSeabaseDDLupgrade.cpp
 * Description:  Implements upgrade of metadata.
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
#include "CmpSeabaseDDLrepos.h"
#include "PrivMgrMD.h"

NABoolean CmpSeabaseMDupgrade::isOldMDtable(const NAString &objName)
{
  if (objName.contains(OLD_MD_EXTENSION))
    return TRUE;
  else
    return FALSE;
}

NABoolean CmpSeabaseMDupgrade::isMDUpgradeNeeded()
{
  for (Lng32 i = 0; i < sizeof(allMDupgradeInfo)/sizeof(MDUpgradeInfo); i++)
    {
      const MDUpgradeInfo &mdti = allMDupgradeInfo[i];
      if (mdti.upgradeNeeded)
        return TRUE;
    }

  return FALSE;
}

NABoolean CmpSeabaseMDupgrade::isViewsUpgradeNeeded()
{
  for (Lng32 i = 0; i < sizeof(allMDviewsInfo)/sizeof(MDViewInfo); i++)
    {
      const MDViewInfo &mdti = allMDviewsInfo[i];
      if (mdti.upgradeNeeded)
        return TRUE;
    }

  return FALSE;
}

NABoolean CmpSeabaseMDupgrade::isReposUpgradeNeeded()
{
  for (Lng32 i = 0; i < sizeof(allReposUpgradeInfo)/sizeof(MDUpgradeInfo); i++)
    {
      const MDUpgradeInfo &mdti = allReposUpgradeInfo[i];
      if (mdti.upgradeNeeded)
        return TRUE;
    }
  return FALSE;
}

// ----------------------------------------------------------------------------
// isPrivsUpgradeNeeded
//
// Checks to see PrivMgr metadata tables need to be upgraded
// returns:
//   TRUE - upgrade maybe needed
//   FALSE - upgrade not needed
// ----------------------------------------------------------------------------
NABoolean CmpSeabaseMDupgrade::isPrivsUpgradeNeeded()
{
  // If PrivMgr tables don't exist (0), then authorization is not enabled
  NABoolean checkAllTables = FALSE;
  if (isPrivMgrMetadataInitialized(&ActiveSchemaDB()->getDefaults(), checkAllTables) == 0)
  {
    // During a Trafodion upgrade, the customer can choose to enable security 
    // features through a new installation option.  When chosen, the installer 
    // sets the environment variable TRAFODION_ENABLE_AUTHENTICATION to YES.
    // Check to see if this envvar has been set:
    char * env = getenv("TRAFODION_ENABLE_AUTHENTICATION");
    if (env)
       return (strcmp(env, "YES") == 0) ? TRUE : FALSE;

    // authorization is not enabled -> no need to upgrade
    return FALSE;
  }

  // Otherwise assume upgrade is required
  return TRUE;
}

short CmpSeabaseMDupgrade::dropMDtables(ExpHbaseInterface *ehi, 
					NABoolean oldTbls,
					NABoolean useOldNameForNewTables)
{
  Lng32 retcode = 0;
  Lng32 errcode = 0;

  for (Lng32 i = 0; i < sizeof(allMDupgradeInfo)/sizeof(MDUpgradeInfo); i++)
    {
      const MDUpgradeInfo &mdti = allMDupgradeInfo[i];
      
      if ((NOT oldTbls) && (!mdti.newName))
	continue;
      
      if (mdti.mdOnly)
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
      
      retcode = dropHbaseTable(ehi, &hbaseTable, FALSE, FALSE);
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

  for (Lng32 i = 0; i < sizeof(allMDupgradeInfo)/sizeof(MDUpgradeInfo); i++)
    {
      const MDUpgradeInfo &mdti = allMDupgradeInfo[i];
      
      if ((!mdti.newName) || (!mdti.oldName) ||
	  (mdti.addedTable) || (mdti.mdOnly))
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

NABoolean CmpSeabaseDDL::getOldMDInfo(const MDTableInfo  &mdti,
                                      const char* &oldName,
                                      const QString* &oldDDL, Lng32 &sizeOfoldDDL)
{
  if (! mdti.newName)
    return FALSE;

  Lng32 numEntries = sizeof(allMDupgradeInfo) / sizeof(MDUpgradeInfo);

  for (Lng32 i = 0; i < numEntries; i++)
    {
      const MDUpgradeInfo &mdui = allMDupgradeInfo[i];
      if (! mdui.newName)
        continue;

      if (strcmp(mdti.newName, mdui.newName) == 0)
        {
          oldName = mdui.oldName;
          oldDDL = mdui.oldDDL;
          sizeOfoldDDL = mdui.sizeOfoldDDL;
          return TRUE;
        }
    }

  return FALSE;
}

// Return value:
//   0: not all old metadata tables exist, metadata is not initialized
//   1: all old metadata tables exists, metadata is initialized
//  -ve: error code
short CmpSeabaseDDL::isOldMetadataInitialized(ExpHbaseInterface * ehi)
{
  short retcode ;

  Lng32 numTotal = 0;
  Lng32 numExists = 0;
  retcode = 0;
  for (Int32 i = 0; 
       (((retcode == 0) || (retcode == -1)) && (i < sizeof(allMDtablesInfo)/sizeof(MDTableInfo))); i++)
    {
      const MDUpgradeInfo &mdi = allMDupgradeInfo[i];
      
      if (! mdi.oldName)
        continue;

      if (mdi.addedTable)
        continue;

      NAString oldName(mdi.oldName);
      size_t pos = oldName.index(OLD_MD_EXTENSION);
      oldName = oldName.remove(pos, sizeof(OLD_MD_EXTENSION));

      numTotal++;
      HbaseStr hbaseTables;
      NAString hbaseTablesStr(getSystemCatalog());
      hbaseTablesStr += ".";
      hbaseTablesStr += SEABASE_MD_SCHEMA;
      hbaseTablesStr += ".";
      hbaseTablesStr += oldName;
      hbaseTables.val = (char*)hbaseTablesStr.data();
      hbaseTables.len = hbaseTablesStr.length();
      
      retcode = ehi->exists(hbaseTables);
      if (retcode == -1) // exists
        numExists++;
    }
  
  if ((retcode != 0) && (retcode != -1))
    return retcode; // error accessing metadata

  if (numExists < numTotal) 
    return 0; // corrupted or uninitialized old metadata

  if (numExists == numTotal)
    return 1; // metadata is initialized
  
  return -1;
}

short CmpSeabaseMDupgrade::executeSeabaseMDupgrade(CmpDDLwithStatusInfo *mdui,
                                                   NABoolean ddlXns,
						   NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  char msgBuf[1000];
  char buf[10000];

  // interfaces for upgrading subsystems; OK to create on stack for
  // now as they are stateless
  CmpSeabaseUpgradeRepository upgradeRepository;
  CmpSeabaseUpgradePrivMgr upgradePrivMgr;

  ExeCliInterface cliInterface(STMTHEAP, 0, NULL,
    CmpCommon::context()->sqlSession()->getParentQid());
  ExpHbaseInterface * ehi = NULL;

  while (1)
    {
      // The line below is useful when debugging upgrade; it shows the step progression
      // through the upgrade state machine.
      // cout << "mdui->step() is " << mdui->step() 
      //      << ", mdui->subStep() is " << mdui->subStep() << endl;

      switch (mdui->step())
	{
	case UPGRADE_START: 
	  {
	    if (xnInProgress(&cliInterface))
	      {
		*CmpCommon::diags() << DgSqlCode(-20123);
		return -1;
	      }

	    if (mdui->getMDVersion())
	      {
		mdui->setStep(GET_MD_VERSION);
		mdui->setSubstep(0);
		break;
	      }

	    if (mdui->getSWVersion())
	      {
		mdui->setStep(GET_SW_VERSION);
		mdui->setSubstep(0);
		break;
	      }

	    mdui->setMsg("Metadata Upgrade: started");
	    mdui->setStep(VERSION_CHECK);
	    mdui->setSubstep(0);
	    mdui->setEndStep(TRUE);
	    
            if (!ComUser::isRootUserID())
              {
                 mdui->setMsg("Metadata Upgrade: not authorized");
                 mdui->setStep(UPGRADE_FAILED);
                 mdui->setSubstep(0);
              }

	    if (sendAllControlsAndFlags())
	      {
		mdui->setStep(UPGRADE_FAILED);
		mdui->setSubstep(0);
	      }

	    return 0;
	  }
	  break;
	  
	case GET_MD_VERSION:
	  {
	    switch (mdui->subStep())
	      {
	      case 0:
		{
		  ehi = allocEHI(&ActiveSchemaDB()->getDefaults());
		  
		  Int64 mdCurrMajorVersion;
		  Int64 mdCurrMinorVersion;
                  Int64 mdCurrUpdateVersion;
		  retcode = validateVersions(&ActiveSchemaDB()->getDefaults(), ehi,
					     &mdCurrMajorVersion,
					     &mdCurrMinorVersion,
                                             &mdCurrUpdateVersion);
		  
		  deallocEHI(ehi);
		  
		  if (retcode == 0)
		    {
		      // no version mismatch detected.
		      // Metadata is uptodate.
		      mdui->setSubstep(1);
		    }
		  else if (retcode == -1395) // mismatch in MAJOR version. Need to upgrade
		    {
		      if ((mdCurrMajorVersion == METADATA_OLD_MAJOR_VERSION) &&
			  (mdCurrMinorVersion == METADATA_OLD_MINOR_VERSION) &&
			  (mdCurrUpdateVersion == METADATA_OLD_UPDATE_VERSION))
			{
			  mdui->setSubstep(2);
			}
		      else
			{
			  // metadata cannot be upgraded with the current release.
			  mdui->setSubstep(3);
			}
		    }
		  else if (retcode == -1394) // mismatch in MAJOR version. Need to upgrade
		    {
		      // metadata cannot be upgraded with the current release.
		      mdui->setSubstep(3);
		    }
		  else
		    {
		      *CmpCommon::diags() << DgSqlCode(retcode);
		      mdui->setStep(DONE_RETURN);
		      
		      mdui->setSubstep(0);
		      mdui->setEndStep(TRUE);
		      break;
		    }
		  
		  str_sprintf(msgBuf, "  Current Version %ld.%ld.%ld. Expected Version %d.%d.%d.",
			      mdCurrMajorVersion, 
                              mdCurrMinorVersion,
                              mdCurrUpdateVersion,
			      METADATA_MAJOR_VERSION, 
                              METADATA_MINOR_VERSION,
                              METADATA_UPDATE_VERSION);
		  mdui->setMsg(msgBuf);
		  mdui->setEndStep(FALSE);
		  
		  return 0;
		} // case 0
		break;
		
	      case 1:
		{
		  str_sprintf(msgBuf, "  Metadata is current.");
		  mdui->setMsg(msgBuf);
		  mdui->setEndStep(FALSE);
		  
		  mdui->setStep(DONE_RETURN);
		  mdui->setSubstep(0);
		  
		  return 0;
		}
		break;
		
	      case 2:
		{
		  str_sprintf(msgBuf, "  Metadata needs to be upgraded or reinitialized.");
		  mdui->setMsg(msgBuf);
		  mdui->setEndStep(FALSE);
		  
		  mdui->setStep(DONE_RETURN);
		  mdui->setSubstep(0);
		  
		  return 0;
		}
		break;
		
	      case 3:
		{
		  str_sprintf(msgBuf, "  Metadata cannot be upgraded with this version of software.");
		  mdui->setMsg(msgBuf);
		  mdui->setEndStep(FALSE);
		  
		  mdui->setSubstep(4);
		  
		  return 0;
		}
		break;

	      case 4:
		{
		  str_sprintf(msgBuf, "  Install previous version of software or reinitialize metadata.");
		  mdui->setMsg(msgBuf);
		  mdui->setEndStep(FALSE);
		  
		  mdui->setStep(DONE_RETURN);
		  mdui->setSubstep(0);
		  
		  return 0;
		}
		break;

	      } // switch
	  } // GET_MD_VERSION
	  break;
	  
	case GET_SW_VERSION:
	  {
	    switch (mdui->subStep())
	      {
	      case 0:
		{
		  ehi = allocEHI(&ActiveSchemaDB()->getDefaults());
		  
		  Int64 mdCurrMajorVersion;
		  Int64 mdCurrMinorVersion;
                  Int64 mdCurrUpdateVersion;
		  Int64 sysSWMajorVersion;
		  Int64 sysSWMinorVersion;
                  Int64 sysSWUpdVersion;
                  Int64 mdSWMajorVersion;
                  Int64 mdSWMinorVersion;
                  Int64 mdSWUpdateVersion;
		  retcode = validateVersions(&ActiveSchemaDB()->getDefaults(), ehi,
					     &mdCurrMajorVersion,
					     &mdCurrMinorVersion,
                                             &mdCurrUpdateVersion,
					     &sysSWMajorVersion,
					     &sysSWMinorVersion,
                                             &sysSWUpdVersion,
                                             &mdSWMajorVersion,
                                             &mdSWMinorVersion,
                                             &mdSWUpdateVersion);
		  
		  deallocEHI(ehi);
		  
		  if ((retcode == 0) ||
                      (retcode == -TRAF_NOT_INITIALIZED) ||
                      (retcode == -1395))
		    {
		      // no version mismatch detected between system and expected software.
                      if ((mdSWMajorVersion == sysSWMajorVersion) &&
                          (mdSWMinorVersion == sysSWMinorVersion) &&
                          (mdSWUpdateVersion == sysSWUpdVersion))
                        // software version stored in metadata is uptodate
                        mdui->setSubstep(1);
                      else
                        // Software version stored in metadata is not current.
                        // Initialize scripts need to be enhanced to store current software 
                        // version in metadata when trafodion is initialized or upgraded. 
                        // Until scripts are enhanced to do that, do not return an error.
                        mdui->setSubstep(1);
		    }
		  else if (retcode == -1397) // mismatch in software version
		    {
                      mdui->setSubstep(3);
		    }
                  else
                    {
                      if (retcode == -1395)
                        str_sprintf(msgBuf, "   Metadata needs to be upgraded or reinitialized (Current Version %ld.%ld.%ld, Expected Version %ld.%ld.%ld).",
                                    mdCurrMajorVersion, mdCurrMinorVersion, mdCurrUpdateVersion,
                                    (Int64)METADATA_MAJOR_VERSION, (Int64)METADATA_MINOR_VERSION, (Int64)METADATA_UPDATE_VERSION);   
                      else
                        str_sprintf(msgBuf, " Error %d returned while accessing metadata. Fix that error before running this command.",
                                    retcode);

                      mdui->setSubstep(4);

                      mdui->setMsg(msgBuf);
                      mdui->setEndStep(FALSE);
                  
                      return 0;
                    }

                  Int64 expSWMajorVersion = SOFTWARE_MAJOR_VERSION;
                  Int64 expSWMinorVersion = SOFTWARE_MINOR_VERSION;
                  Int64 expSWUpdVersion = SOFTWARE_UPDATE_VERSION;
		  str_sprintf(msgBuf, "  System Version %ld.%ld.%ld. Expected Version %ld.%ld.%ld.",
			      sysSWMajorVersion, sysSWMinorVersion, sysSWUpdVersion,
			      expSWMajorVersion, expSWMinorVersion, expSWUpdVersion);
		  mdui->setMsg(msgBuf);
		  mdui->setEndStep(FALSE);
		  
		  return 0;
		} // case 0
		break;
		
	      case 1:
		{
		  str_sprintf(msgBuf, "  Software is current.");
		  mdui->setMsg(msgBuf);
		  mdui->setEndStep(FALSE);
		  
		  mdui->setStep(DONE_RETURN);
		  mdui->setSubstep(0);
		  
		  return 0;
		}
		break;
		
	      case 2:
		{
		  str_sprintf(msgBuf, "  Metadata needs to be updated with current software version. Run 'initialize trafodion, update software version' to update it.");
		  mdui->setMsg(msgBuf);
		  mdui->setEndStep(FALSE);
		  
		  mdui->setStep(DONE_RETURN);
		  mdui->setSubstep(0);
		  
		  return 0;
		}
		break;

	      case 3:
		{
		  str_sprintf(msgBuf, "  Version of software being used is not compatible with version of software on the system.");
		  mdui->setMsg(msgBuf);
		  mdui->setEndStep(FALSE);
		  
		  mdui->setStep(DONE_RETURN);
		  mdui->setSubstep(0);
		  
		  return 0;
		}
		break;
		
              case 4:
                {
		  mdui->setEndStep(FALSE);
		  
		  mdui->setStep(DONE_RETURN);
		  mdui->setSubstep(0);
                }
                break;

	      } // switch
	  } // GET_SW_VERSION
	  break;
	  
	case VERSION_CHECK:
	  {
	    switch (mdui->subStep())
	      {
	      case 0:
		{
		  mdui->setMsg("Version Check: started");
		  mdui->subStep()++;
		  mdui->setEndStep(FALSE);

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
		      str_sprintf(msgBuf, "  Metadata is already at Version %ld.%ld.",
				  mdCurrMajorVersion, mdCurrMinorVersion);
		      mdui->setMsg(msgBuf);
		      mdui->setEndStep(FALSE);

		      mdui->setSubstep(2);
		      
		      return 0;
		    }
		  else if (retcode == -1395) // mismatch in MAJOR version. Need to upgrade
		    {
		      if ((mdCurrMajorVersion == METADATA_OLD_MAJOR_VERSION) &&
			  (mdCurrMinorVersion == METADATA_OLD_MINOR_VERSION))
			{
                          NAString upgItems;
                          if (isUpgradeNeeded())
                            {
                              upgItems = "\n  Upgrade needed for";

                              if (isMDUpgradeNeeded())
                                {
                                  upgItems += " Catalogs,";
                                }
                              if (isViewsUpgradeNeeded())
                                {
                                  upgItems += " Views,";
                                }
                              if (upgradePrivMgr.needsUpgrade(this))
                                {
                                  upgItems += " Privileges,";
                                }
                              if (upgradeRepository.needsUpgrade(this))
                                {
                                  upgItems += " Repository,";
                                }
                              if (NOT upgItems.isNull())
                                {
                                  upgItems = upgItems.strip(NAString::trailing, ',');
                                  upgItems += ".";
                                }
                            }
			  str_sprintf(msgBuf, "  Metadata needs to be upgraded from Version %ld.%ld.%ld to %d.%d.%d.%s",
				      mdCurrMajorVersion, mdCurrMinorVersion/10, 
                                      (mdCurrMinorVersion - (mdCurrMinorVersion/10)*10),
				      METADATA_MAJOR_VERSION, METADATA_MINOR_VERSION,
                                      METADATA_UPDATE_VERSION,
                                      (upgItems.isNull() ? " " : upgItems.data()));
			  
			  mdui->setMsg(msgBuf);
			  
			  mdui->setSubstep(3);
			  
			  return 0;
			}
		      else
			{
			  // metadata cannot be upgraded with the current release.
			  str_sprintf(msgBuf, "  Metadata cannot to be upgraded from Version %ld.%ld to %d.%d with this software",
				      mdCurrMajorVersion, mdCurrMinorVersion,
				      METADATA_MAJOR_VERSION, METADATA_MINOR_VERSION);
			  
			  mdui->setMsg(msgBuf);
			  
			  mdui->setSubstep(4);
			  
			  return 0;
			}
		    }
		  else
		    {
		      *CmpCommon::diags() << DgSqlCode(retcode);
		      mdui->setStep(UPGRADE_DONE);

		      mdui->setSubstep(0);
		      mdui->setEndStep(TRUE);
		    }

		  return 0;
		}
		break;

	      case 2:
		{
		  mdui->setMsg("Version Check: done");
		  mdui->setSubstep(0);
		  mdui->setEndStep(TRUE);

		  mdui->setStep(UPGRADE_DONE);

		  return 0;
		}
		break;

	      case 3:
		{
		  mdui->setMsg("Version Check: done");
		  mdui->setSubstep(0);
		  mdui->setEndStep(TRUE);
                  mdui->setStep(OLD_MD_DROP_PRE);

		  return 0;
		}
		break;

	      case 4:
		{
		  mdui->setMsg("Version Check: done");
		  mdui->setSubstep(0);
		  mdui->setEndStep(TRUE);

		  mdui->setStep(UPGRADE_FAILED);

		  return 0;
		}
		break;

	      } // case
	  } // case VERSION_CHECK
	  break;

	case OLD_MD_DROP_PRE:
	  {
	    switch (mdui->subStep())
	      {
	      case 0:
		{
                  if ((NOT isMDUpgradeNeeded()) &&
                      (NOT isViewsUpgradeNeeded()))
                    {
                      mdui->setStep(UPDATE_MD_VIEWS);
                      mdui->setSubstep(0);
                      break;
                    }

		  mdui->setMsg("Drop Old Metadata: started");
		  mdui->subStep()++;
		  mdui->setEndStep(FALSE);

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

		      mdui->setStep(UPGRADE_FAILED);
		      mdui->setSubstep(0);

		      break;
		    }

		  deallocEHI(ehi); 

		  mdui->setMsg("Drop Old Metadata: done");
		  mdui->setStep(CURR_MD_BACKUP);
		  mdui->setSubstep(0);
		  mdui->setEndStep(TRUE);

		  return 0;
		}
		break;
	      } // case
	  }
	  break;

	case CURR_MD_BACKUP:
	  {
	    switch (mdui->subStep())
	      {
	      case 0:
		{
		  mdui->setMsg("Backup Current Metadata: started");
		  mdui->subStep()++;
		  mdui->setEndStep(FALSE);

		  return 0;
		}
		break;
		
	      case 1:
		{
		  //
		  // backup current metadata using hbase snapshot command
		  //
		  ehi = allocEHI();

		  for (Lng32 i = 0; i < sizeof(allMDupgradeInfo)/sizeof(MDUpgradeInfo); i++)
		    {
		      const MDUpgradeInfo &mdti = allMDupgradeInfo[i];

		      if ((mdti.addedTable) || (mdti.mdOnly))
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
			  mdui->setStep(UPGRADE_FAILED_DROP_OLD_MD);
			  mdui->setSubstep(0);
			  break;
			}
		      
		    } // for

		  if (mdui->step() != CURR_MD_BACKUP)
		    break;

		  deallocEHI(ehi); 

		  mdui->setMsg("Backup Current Metadata: done");
		  mdui->setStep(CURR_MD_DROP);
		  mdui->setSubstep(0);
		  mdui->setEndStep(TRUE);

		  return 0;
		}
		break;
	      } // case
	    
	  }
	  break;

	case CURR_MD_DROP:
	  {
	    switch (mdui->subStep())
	      {
	      case 0:
		{
                 if (NOT isMDUpgradeNeeded())
                    {
                      mdui->setStep(UPDATE_MD_VIEWS);
                      mdui->setSubstep(0);
                      break;
                    }

		  mdui->setMsg("Drop Current Metadata: started");
		  mdui->subStep()++;
		  mdui->setEndStep(FALSE);

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
		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui->setSubstep(0);
		      break;
		    }

		  deallocEHI(ehi); 

		  mdui->setMsg("Drop Current Metadata: done");
		  mdui->setStep(INITIALIZE_TRAF);
		  mdui->setSubstep(0);
		  mdui->setEndStep(TRUE);

		  return 0;
		}
		break;
	      } // case
	  }
	  break;

	case INITIALIZE_TRAF:
	  {
	    switch (mdui->subStep())
	      {
	      case 0:
		{
		  if (xnInProgress(&cliInterface))
		    {
		      *CmpCommon::diags() << DgSqlCode(-20123);

		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui->setSubstep(0);
		      break;
		    }

		  mdui->setMsg("Initialize New Metadata: started");
		  mdui->subStep()++;
		  mdui->setEndStep(FALSE);

		  return 0;
		}
		break;
		
	      case 1:
		{
		  //
		  // initialize trafodion
		  CmpCommon::context()->setIsUninitializedSeabase(TRUE);
		  CmpCommon::context()->uninitializedSeabaseErrNum() = -TRAF_NOT_INITIALIZED; // MD doesn't exist

                  // Use "initialize trafodion, minimal" so we only create the metadata
                  // tables. The other tables (repository and privilege manager) already
                  // exist; we will upgrade them later in this method.
		  str_sprintf(buf, "initialize trafodion, minimal, no return status;");
		  
		  cliRC = cliInterface.executeImmediate(buf);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
		      
		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui->setSubstep(0);
		      break;
		    }

		  if (xnInProgress(&cliInterface))
		    {
		      *CmpCommon::diags() << DgSqlCode(-20123);

		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui->setSubstep(0);
		      break;
		    }

                  CmpCommon::context()->setIsUninitializedSeabase(FALSE);
                  CmpCommon::context()->uninitializedSeabaseErrNum() = 0;

		  // After initialize, VERSIONS table contain the new metadata version
		  // values. Delete these values since the upgrade process is not yet done.
		  // At the end of upgrade, VERSIONS table will be updated with the
		  // new version values.
		  cliRC = beginXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui->setSubstep(0);
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
		      
		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui->setSubstep(0);
		      break;
		    }

		  cliRC = commitXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
		      
		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui->setSubstep(0);

		      break;
		    }

		  mdui->setMsg("Initialize New Metadata: done");
		  mdui->setStep(COPY_MD_TABLES_PROLOGUE);
		  mdui->setSubstep(0);
		  mdui->setEndStep(TRUE);

		  return 0;
		}
		break;
	      } // case
	  }
	  break;

	case COPY_MD_TABLES_PROLOGUE:
	  {
	    mdui->setMsg("Copy Old Metadata: started");
	    mdui->setStep(COPY_MD_TABLES);
	    mdui->setSubstep(0);
	    mdui->setEndStep(FALSE);

	    if (xnInProgress(&cliInterface))
	      {
		*CmpCommon::diags() << DgSqlCode(-20123);

		mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		mdui->setSubstep(0);

		break;
	      }

            cliRC = autoCommit(&cliInterface, TRUE); // set autocommit ON.
	    if (cliRC < 0)
	      {
		cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

		mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		mdui->setSubstep(0);

		break;
	      }

	    return 0;
	  }
	  break;

	case COPY_MD_TABLES:
	  {
	    const MDUpgradeInfo &mdti = allMDupgradeInfo[mdui->subStep()];
	  
	    if ((mdui->subStep() < sizeof(allMDupgradeInfo)/sizeof(MDUpgradeInfo)) &&
		((NOT mdti.addedTable) && (NOT mdti.droppedTable) && 
                 (NOT mdti.isIndex) && (NOT mdti.mdOnly)))
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
		if (NOT mdui->startStep())
		  {
		    str_sprintf(msgBuf, "  Start: Copy %20s ==> %s",
				mdti.oldName, mdti.newName);
		      
		    mdui->setStartStep(TRUE);
		    mdui->setEndStep(FALSE);

		    //mdui->setMsg(buf);
		    mdui->setMsg(msgBuf);

		    break;
		    //return 0;
		  }
		
		mdui->setStartStep(FALSE);
		mdui->setEndStep(TRUE);

		// execute the insert...select
		cliRC = cliInterface.executeImmediate(buf);
		if (cliRC < 0)
		  {
		    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
		    
		    mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		    mdui->setSubstep(0);

		    break;
		  }
		
		str_sprintf(msgBuf, "  End:   Copy %20s ==> %s",
			    mdti.oldName, mdti.newName);
		
		mdui->setMsg(msgBuf);
		mdui->subStep()++;

		break;
		//return 0;
	      } // if

	    mdui->subStep()++;
	    if (mdui->subStep() >= sizeof(allMDupgradeInfo)/sizeof(MDUpgradeInfo))
	      {
		mdui->setStep(COPY_MD_TABLES_EPILOGUE);
		mdui->setSubstep(0);
	      }

	  }
	  break;
	  
	case COPY_MD_TABLES_EPILOGUE:
	  {
	    mdui->setMsg("Copy Old Metadata: done");
	    mdui->setStep(VALIDATE_DATA_COPY);
	    mdui->setSubstep(0);
	    mdui->setEndStep(TRUE);

            cliRC = autoCommit(&cliInterface, FALSE); // set to OFF
	    if (cliRC < 0)
	      {
		cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
		
		mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		mdui->setSubstep(0);

		break;
	      }

	    return 0;
	  }
	  break;

	case VALIDATE_DATA_COPY:
	  {
	    switch (mdui->subStep())
	      {
	      case 0:
		{
		  mdui->setMsg("Validate Metadata Copy: started");
		  mdui->subStep()++;
		  mdui->setEndStep(FALSE);

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
		  mdui->setMsg("Validate Metadata Copy: done");
		  mdui->setStep(CUSTOMIZE_NEW_MD);
		  mdui->setSubstep(0);
		  mdui->setEndStep(TRUE);

		  return 0;
		}
		break;
	      } // case
	  }
	  break;

	case CUSTOMIZE_NEW_MD:
	  {
            short rc = customizeNewMD(mdui, cliInterface);
            if (rc == 1)
              {
                mdui->setStep(OLD_TABLES_MD_DELETE);
                mdui->setSubstep(0);
                mdui->setEndStep(TRUE);
              }
            else if (rc == -1)
              break;
            
            return 0;
          }
          break;

	case OLD_TABLES_MD_DELETE:
	  {
	    switch (mdui->subStep())
	      {
	      case 0:
		{
		  if (xnInProgress(&cliInterface))
		    {
		      *CmpCommon::diags() << DgSqlCode(-20123);

		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui->setSubstep(0);

		      break;
		    }
		  
		  cliRC = beginXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui->setSubstep(0);

		      break;
		    }

		  mdui->setMsg("Delete Old Metadata Info: started");
		  mdui->subStep()++;
		  mdui->setEndStep(FALSE);

		  return 0;
		}
		break;
		
	      case 1:
		{
		  ehi = allocEHI();

		  // drop info about old tables from the new metadata.
		  // do not drop the actual hbase tables. 
		  for (Lng32 i = 0; i < sizeof(allMDupgradeInfo)/sizeof(MDUpgradeInfo); i++)
		    {
		      const MDUpgradeInfo &mdti = allMDupgradeInfo[i];

		      if (! mdti.oldName)
			continue;

                      if ((mdti.addedTable) || (mdti.droppedTable)) // || (mdti.isIndex))
                        continue;

		      NAString catName(TRAFODION_SYSCAT_LIT);
		      NAString schName;
		      schName += "\"";
		      schName += SEABASE_MD_SCHEMA;
		      schName += "\"";

                      char objType[5];
                      if (mdti.isIndex)
                        strcpy(objType, COM_INDEX_OBJECT_LIT);
                      else
                        strcpy(objType, COM_BASE_TABLE_OBJECT_LIT);

		      Int64 objUID = getObjectUID(&cliInterface,
						  TRAFODION_SYSCAT_LIT, SEABASE_MD_SCHEMA,
						  mdti.oldName,
                                                  objType);
		      if (objUID < 0)
			{
                          CmpCommon::diags()->clear();

                          strcpy(objType, "LB");
                          objUID = getObjectUID(&cliInterface,
                                                TRAFODION_SYSCAT_LIT, SEABASE_MD_SCHEMA,
                                                mdti.oldName,
                                                objType);
                          if (objUID < 0)
                            {
                              CmpCommon::diags()->clear();

                              strcpy(objType, "UR");
                              objUID = getObjectUID(&cliInterface,
                                                    TRAFODION_SYSCAT_LIT, SEABASE_MD_SCHEMA,
                                                    mdti.oldName,
                                                    objType);
                              if (objUID < 0)
                                {
                                  // could not find the object or an error.
                                  // Ignore and continue.
                                  CmpCommon::diags()->clear();
                                  continue;
                                }
                            }
			}
                      ComObjectType objectType = PrivMgr::ObjectLitToEnum(objType);
		      if (dropSeabaseObject(ehi, mdti.oldName,
					    catName, schName,
                                            objectType,
                                            FALSE,
					    TRUE, FALSE))
			{
			  deallocEHI(ehi); 

			  mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
			  mdui->setSubstep(0);

			  break;
			}
		    } // for

		  if (mdui->step() != OLD_TABLES_MD_DELETE)
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
					    COM_VIEW_OBJECT,
                                            FALSE))
			{
			  deallocEHI(ehi); 

			  mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
			  mdui->setSubstep(0);

			  break;
			}
		    } // for

		  if (mdui->step() != OLD_TABLES_MD_DELETE)
		    break;

		  deallocEHI(ehi); 

		  mdui->setMsg("Delete Old Metadata Info: done");
		  mdui->setStep(UPDATE_MD_VIEWS);
		  mdui->setSubstep(0);
		  mdui->setEndStep(TRUE);

		  if (xnInProgress(&cliInterface))
		    {
		      cliRC = commitXn(&cliInterface);
		      if (cliRC < 0)
			{
			  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
			  
			  mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
			  mdui->setSubstep(0);

			  break;
			}
		    }

		  return 0;
		}
		break;
	      } // case

	  }
	  break;

        case UPDATE_MD_VIEWS:
          {
	    switch (mdui->subStep())
	      {
	      case 0:
		{
                  if (NOT isViewsUpgradeNeeded())
                    {
                      mdui->setStep(UPGRADE_REPOS);
                      mdui->setSubstep(0);
                      break;
                    }

		  mdui->setMsg("Update Metadata Views: started");
		  mdui->subStep()++;
		  mdui->setEndStep(FALSE);

		  return 0;
		}
		break;
		
	      case 1:
		{
		  if (xnInProgress(&cliInterface))
		    {
		      *CmpCommon::diags() << DgSqlCode(-20123);

		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui->setSubstep(0);

		      break;
		    }
		  
		  cliRC = beginXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui->setSubstep(0);

		      break;
		    }
		  
		  cliRC = dropMetadataViews(&cliInterface);
		  if (cliRC < 0)
		    {
		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui->setSubstep(0);

		      break;
		    }

		  cliRC = createMetadataViews(&cliInterface);
		  if (cliRC < 0)
		    {
		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui->setSubstep(0);

		      break;
		    }

		  cliRC = commitXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
		    
		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
		      mdui->setSubstep(0);

		      break;
		    }
		  
		  mdui->setMsg("Update Metadata Views: done");
		  mdui->setStep(UPGRADE_REPOS);
		  mdui->setSubstep(0);
		  mdui->setEndStep(TRUE);

		  return 0;
		}
		break;
	      } // case

          }
          break;

        case UPGRADE_PRIV_MGR:
          {
            // Note: We used to do this case before UPGRADE_REPOS, but
            // that was changed in Trafodion 2.1 to do it afterwards instead.
            // The reason is that the Privilege Manager does not adhere
            // to the CmpSeabaseUpgradeSubsystem class contract; its drop
            // and recovery logic is packaged in its upgrade method. So,
            // we have to do the Privilege Manager upgrade last since there
            // is no way to undo it if we have an error afterward.
	    switch (mdui->subStep())
	      {
	      case 0:
		{
                  if (NOT upgradePrivMgr.needsUpgrade(this))
                    {
                      mdui->setStep(UPDATE_VERSION);
                      mdui->setSubstep(0);
                      break;
                    }

		  mdui->setMsg("Upgrade Priv Mgr: started");
		  mdui->subStep()++;
		  mdui->setEndStep(FALSE);

		  return 0;
		}
		break;
		
	      case 1:
		{
		  if (xnInProgress(&cliInterface))
		    {
		      *CmpCommon::diags() << DgSqlCode(-20123);

		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_REPOS);
		      mdui->setSubstep(0);

		      break;
		    }
		  
		  cliRC = beginXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_REPOS);
		      mdui->setSubstep(0);

		      break;
		    }
		  
		  cliRC = upgradePrivMgr.doUpgrade(&cliInterface,mdui,this,ddlXns);
		  if (cliRC != 0)
		    {
		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_REPOS);
		      mdui->setSubstep(0);

		      break;
		    }

		  cliRC = commitXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
		    
		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_REPOS);
		      mdui->setSubstep(0);

		      break;
		    }
		  
                  // mdui->setMsg() was called in upgradePrivMgr.doUpgrade()
		  mdui->setStep(UPDATE_VERSION);
		  mdui->setSubstep(0);
		  mdui->setEndStep(TRUE);

		  return 0;
		}
		break;
	      } // case

          }
          break;

        case UPGRADE_REPOS:
          {
            if (NOT isReposUpgradeNeeded())
              {
                mdui->setStep(UPGRADE_PRIV_MGR);
                mdui->setSubstep(0);
                break;
              }

            if (xnInProgress(&cliInterface))
              {
                *CmpCommon::diags() << DgSqlCode(-20123);
                
                mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
                mdui->setSubstep(0);
                
                break;
              }
            
            cliRC = upgradeRepository.doUpgrade(&cliInterface, mdui, this, ddlXns);
            if (cliRC != 0)
              {
                mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_REPOS);
                mdui->setSubstep(-(cliRC+1));
                
                break;
              }
            
            if (mdui->endStep())
              {
                mdui->setStep(UPGRADE_PRIV_MGR);
                mdui->setSubstep(0);
              }
            
            return 0;
          }
          break;

	case UPDATE_VERSION:
	  {
	    switch (mdui->subStep())
	      {
	      case 0:
		{
		  mdui->setMsg("Update Metadata Version: started");
		  mdui->subStep()++;
		  mdui->setEndStep(FALSE);

		  return 0;
		}
		break;
		
	      case 1:
		{
		  if (xnInProgress(&cliInterface))
		    {
		      *CmpCommon::diags() << DgSqlCode(-20123);

		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_REPOS);
		      mdui->setSubstep(0);

		      break;
		    }
		  
		  cliRC = beginXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_REPOS);
		      mdui->setSubstep(0);

		      break;
		    }
		  
		  cliRC = updateSeabaseVersions(&cliInterface, TRAFODION_SYSCAT_LIT);
		  if (cliRC < 0)
		    {
		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_REPOS);
		      mdui->setSubstep(0);

		      break;
		    }

                  CmpCommon::context()->setIsUninitializedSeabase(FALSE);
                  CmpCommon::context()->uninitializedSeabaseErrNum() = 0;

		  cliRC = commitXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
		    
		      mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_REPOS);
		      mdui->setSubstep(0);

		      break;
		    }
		  
		  mdui->setMsg("Update Metadata Version: done");

                  if ((NOT isMDUpgradeNeeded()) &&
                      (NOT isViewsUpgradeNeeded()))
                    mdui->setStep(METADATA_UPGRADED);
                  else
                    mdui->setStep(OLD_REPOS_DROP);
		  mdui->setSubstep(0);
		  mdui->setEndStep(TRUE);
                  
		  return 0;
		}
		break;
	      } // case
	  }
	  break;

	case OLD_REPOS_DROP:
	  {
            if (upgradeRepository.needsUpgrade(this))
              {
                if (upgradeRepository.doDrops(&cliInterface,mdui,this))
                  {
                    // no status message in this case so no return
                    cliInterface.clearGlobalDiags();
                    mdui->setStep(OLD_MD_TABLES_HBASE_DELETE);
                    mdui->setSubstep(0);
                    mdui->setEndStep(TRUE);
                  }
                else
                  {
                    if (mdui->endStep())
                      {
                        mdui->setStep(OLD_MD_TABLES_HBASE_DELETE);
                        mdui->setSubstep(0);
                        mdui->setEndStep(TRUE);
                      }                   
                    return 0;
                  }
              }
	  }
	  break;

	case OLD_MD_TABLES_HBASE_DELETE:
	  {
	    switch (mdui->subStep())
	      {
	      case 0:
		{
		  if (xnInProgress(&cliInterface))
		    {
		      *CmpCommon::diags() << DgSqlCode(-20123);

		      mdui->setStep(METADATA_UPGRADED);
		      break;
		    }
		  
		  cliRC = beginXn(&cliInterface);
		  if (cliRC < 0)
		    {
		      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

		      mdui->setStep(METADATA_UPGRADED);
		      break;
		    }

		  mdui->setMsg("Drop Old Metadata from Hbase: started");
		  mdui->subStep()++;
		  mdui->setEndStep(FALSE);

		  return 0;
		}
		break;
		
	      case 1:
		{
		  ehi = allocEHI();

		  // drop old tables from hbase.
		  for (Lng32 i = 0; i < sizeof(allMDupgradeInfo)/sizeof(MDUpgradeInfo); i++)
		    {
		      const MDUpgradeInfo &mdti = allMDupgradeInfo[i];

		      if ((! mdti.oldName) || (mdti.mdOnly))
			continue;

		      NAString catName(TRAFODION_SYSCAT_LIT);
		      NAString schName;
		      schName += "\"";
		      schName += SEABASE_MD_SCHEMA;
		      schName += "\"";
		      if (dropSeabaseObject(ehi, mdti.oldName,
					    catName, schName,
					    (mdti.isIndex ? COM_INDEX_OBJECT : COM_BASE_TABLE_OBJECT),
                                            FALSE,
					    FALSE, TRUE))
			{
			  // ignore errors. Continue dropping old md tables.
			}
		    } // for

		  deallocEHI(ehi); 

		  mdui->setMsg("Drop Old Metadata from Hbase: done");
		  mdui->setStep(METADATA_UPGRADED);
		  mdui->setSubstep(0);
		  mdui->setEndStep(TRUE);

		  if (xnInProgress(&cliInterface))
		    {
		      cliRC = commitXn(&cliInterface);
		      if (cliRC < 0)
			{
			  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

			  mdui->setStep(METADATA_UPGRADED);
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
	    str_sprintf(msgBuf, "Metadata Upgrade to Version %d.%d.%d: done",
                        METADATA_MAJOR_VERSION, 
                        METADATA_MINOR_VERSION,
                        METADATA_UPDATE_VERSION);

	    mdui->setMsg(msgBuf);
	    mdui->setDone(TRUE);

	    return 0;
	  }
	  break;

	case UPGRADE_DONE:
	  {
	    mdui->setMsg("Metadata Upgrade: done");
	    mdui->setDone(TRUE);

	    return 0;
	  }
	  break;

	case DONE_RETURN:
	  {
	    mdui->setMsg("");
	    mdui->setDone(TRUE);

	    return 0;
	  }
	  break;

        case UPGRADE_FAILED_RESTORE_OLD_REPOS:
          {
            // Note: We can't combine this case with UPGRADE_FAILED etc.
            // below, because the subsystem code uses mdui->subStep()
            // to keep track of its progress.

            if (upgradeRepository.needsUpgrade(this))
              {
                if (xnInProgress(&cliInterface))
                  {
                    cliRC = rollbackXn(&cliInterface);
                    if (cliRC < 0)
                      {
                        // ignore errors
                      }
                  }

                if (upgradeRepository.doUndo(&cliInterface,mdui,this))
                  {
                    // ignore errors; no status message so just continue on
                    cliInterface.clearGlobalDiags();
                    mdui->setStep(UPGRADE_FAILED_DROP_OLD_MD);
                    mdui->setSubstep(0);
                    mdui->setEndStep(TRUE);
                  }
                else
                  {
                    if (mdui->endStep())
                      {
                        mdui->setStep(UPGRADE_FAILED_DROP_OLD_MD);
                        mdui->setSubstep(0);
                        mdui->setEndStep(TRUE);
                      }
                    return 0;
                  }
              }
            else
              {
                mdui->setStep(UPGRADE_FAILED_DROP_OLD_MD);
                mdui->setSubstep(0);
                mdui->setEndStep(TRUE);
              }
          }
          break;

	case UPGRADE_FAILED:
	case UPGRADE_FAILED_RESTORE_OLD_MD:
	case UPGRADE_FAILED_DROP_OLD_MD:
	  {
            if (xnInProgress(&cliInterface))
              {
                cliRC = rollbackXn(&cliInterface);
                if (cliRC < 0)
                  {
                    // ignore errors
                  }
              }
            
	    switch (mdui->subStep())
	      {
	      case 0:
		{
		  if (mdui->step() == UPGRADE_FAILED_DROP_OLD_MD)
		    {
		      mdui->setSubstep(1);
		      break;
		    }
		  else if (mdui->step() == UPGRADE_FAILED_RESTORE_OLD_MD)
		    {
		      mdui->setSubstep(4);
		      break;
		    }
		  else
		    {
		      mdui->setSubstep(7);
		      break;
		    }
		}
		break;
		
	      case 1:
		{
		  mdui->setMsg("Drop Old Metadata: started");
		  mdui->subStep()++;
		  mdui->setEndStep(FALSE);
		  
		  return 0;
		}
		break;

	      case 2:
		{
		  if (ehi == NULL)
		    ehi = allocEHI();
		  
		  dropMDtables(ehi, TRUE); // drop old MD
		  deallocEHI(ehi);
		  
		  mdui->subStep()++;
		  mdui->setEndStep(FALSE);
		}
		break;

	      case 3:
		{
		  mdui->setMsg("Drop Old Metadata: done");
		  mdui->setSubstep(7);
		  mdui->setEndStep(TRUE);

		  return 0;
		}
		break;

	      case 4:
		{
		  mdui->setMsg("Restore from Old Metadata: started");
		  mdui->subStep()++;
		  mdui->setEndStep(FALSE);

		  return 0;
		}
		break;

	      case 5:
		{
		  if (ehi == NULL)
		    ehi = allocEHI();
		  
		  restoreOldMDtables(ehi); // restore old MD and make them current
		  deallocEHI(ehi);

		  mdui->subStep()++;
		  mdui->setEndStep(FALSE);
		}
		break;
		
	      case 6:
		{
		  mdui->setMsg("Restore from Old Metadata: done");
		  mdui->setSubstep(1); // now drop old metadata tables
		  mdui->setEndStep(TRUE);
		  
		  return 0;
		}
		break;

	      case 7: 
		{
		  deallocEHI(ehi);
		  
		  mdui->setMsg("Metadata Upgrade: failed");
		  mdui->setDone(TRUE);
		  
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

short CmpSeabaseMDupgrade::customizeNewMD(CmpDDLwithStatusInfo *mdui,
                                          ExeCliInterface &cliInterface)
{
  if ((METADATA_OLD_MAJOR_VERSION == 2) && (METADATA_OLD_MINOR_VERSION == 3) &&
      (METADATA_MAJOR_VERSION == 3) && (METADATA_MINOR_VERSION == 0))
    return customizeNewMDv23tov30(mdui, cliInterface);

  // done, move to next step.
  return 1;
}

short CmpSeabaseMDupgrade::customizeNewMDv23tov30(CmpDDLwithStatusInfo *mdui,
                                                  ExeCliInterface &cliInterface)
{
  Lng32 cliRC = 0;
  char buf[10000];

  if (NOT ((METADATA_OLD_MAJOR_VERSION == 2) && (METADATA_OLD_MINOR_VERSION == 3) &&
           (METADATA_MAJOR_VERSION == 3) && (METADATA_MINOR_VERSION == 0)))
    {
      // done, move to next step.
      return 1;
    }

  switch (mdui->subStep())
    {
    case 0:
      {
        mdui->setMsg("Customize New Metadata: started");
        mdui->subStep()++;
        mdui->setEndStep(FALSE);
        
        // For other upgrades, it is not needed.
        // Customize this section as needed.
        if (NOT ((METADATA_OLD_MAJOR_VERSION == 2) && (METADATA_OLD_MINOR_VERSION == 3) &&
                 (METADATA_MAJOR_VERSION == 3) && (METADATA_MINOR_VERSION == 0)))
          {
            mdui->setSubstep(11);
            mdui->setEndStep(FALSE);
            
            return 0;
          }
        
        if (xnInProgress(&cliInterface))
          {
            *CmpCommon::diags() << DgSqlCode(-20123);
            
            mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
            mdui->setSubstep(0);
            
            return -1;
          }
        
        return 0;
      }
      return -1;
      
    case 1:
      {
        mdui->setMsg("  Start: Update COLUMNS");
        mdui->subStep()++;
        
        cliRC = beginXn(&cliInterface);
        if (cliRC < 0)
          {
            cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
            
            mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
            mdui->setSubstep(0);
            
            return -1;
          }
        
        return 0;
      }
      return -1;
      
    case 2:
      {
        str_sprintf(buf, "update %s.\"%s\".%s set sql_data_type = cast( case when fs_data_type = 130 then '" COM_SMALLINT_SIGNED_SDT_LIT"' when fs_data_type = 131 then '" COM_SMALLINT_UNSIGNED_SDT_LIT"' when fs_data_type = 132 then '" COM_INTEGER_SIGNED_SDT_LIT"' when fs_data_type = 133 then '" COM_INTEGER_UNSIGNED_SDT_LIT"' when fs_data_type = 134 then '" COM_LARGEINT_SIGNED_SDT_LIT"' when fs_data_type = 135 then '" COM_SMALLINT_UNSIGNED_SDT_LIT"' when fs_data_type = 140 then '" COM_REAL_SDT_LIT"' when fs_data_type = 141 then '" COM_DOUBLE_SDT_LIT"' when fs_data_type = 150 then '" COM_DECIMAL_UNSIGNED_SDT_LIT"' when fs_data_type = 151 then '" COM_DECIMAL_SIGNED_SDT_LIT"' when fs_data_type = 155 then '" COM_NUMERIC_UNSIGNED_SDT_LIT"' when fs_data_type = 156 then '" COM_NUMERIC_SIGNED_SDT_LIT"' when fs_data_type = 0     then '" COM_CHARACTER_SDT_LIT"' when fs_data_type = 2     then '" COM_CHARACTER_SDT_LIT"' when fs_data_type = 70    then '" COM_LONG_VARCHAR_SDT_LIT"' when fs_data_type = 64    then '" COM_VARCHAR_SDT_LIT"' when fs_data_type = 66    then '" COM_VARCHAR_SDT_LIT"' when fs_data_type = 100   then '" COM_VARCHAR_SDT_LIT"' when fs_data_type = 101   then '" COM_VARCHAR_SDT_LIT"' when fs_data_type = 192 then '" COM_DATETIME_SDT_LIT"' when fs_data_type >= 196 and fs_data_type <= 207 then '" COM_INTERVAL_SDT_LIT"' else '' end as char(24))    ",
                       getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS);
        cliRC = cliInterface.executeImmediate(buf);
        
        if (cliRC < 0)
          {
            cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
            
            *CmpCommon::diags() << DgSqlCode(-1423)
                                << DgString0(SEABASE_TEXT);
            
            mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
            mdui->setSubstep(0);
            
            return -1;
          }
        
        if (mdui->step() != CUSTOMIZE_NEW_MD)
          return -1;
        
        mdui->setMsg("  End: Update COLUMNS");
        mdui->subStep()++;
        
        if (xnInProgress(&cliInterface))
          {
            cliRC = commitXn(&cliInterface);
            if (cliRC < 0)
              {
                cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
                
                mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
                mdui->setSubstep(0);
                
                return -1;
              }
          }
        
        return 0;
      }
      return -1;
      
    case 3:
      {
        mdui->setMsg("  Start: Update TABLES");
        mdui->subStep()++;
        
        cliRC = beginXn(&cliInterface);
        if (cliRC < 0)
          {
            cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
            
            mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
            mdui->setSubstep(0);
            
            return -1;
          }
        
        return 0;
      }
      return -1;
      
    case 4:
      {
        str_sprintf(buf, "select table_uid, hbase_create_options from %s.\"%s\".%s where char_length(hbase_create_options) > 0",
                    getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLES_OLD_MD);
        
        Queue * tablesQueue = NULL;
        cliRC = cliInterface.fetchAllRows(tablesQueue, buf, 0, FALSE, FALSE, TRUE);
        if (cliRC < 0)
          {
            cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
               
            mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
            mdui->setSubstep(0);
               
            return -1;
          }
           
        tablesQueue->position();
        for (int idx = 0; idx < tablesQueue->numEntries(); idx++)
          {
            OutputInfo * oi = (OutputInfo*)tablesQueue->getNext(); 
               
            Int64 tableUID = *(Int64*)oi->get(0);
            NAString hbaseCreateOptions((char*)oi->get(1));
               
            Lng32 numSaltPartns = 0;
               
            // get num salt partns from hbaseCreateOptions.
            // It is stored as:  NUM_SALT_PARTNS=>NNNN
            size_t idx2 = hbaseCreateOptions.index("NUM_SALT_PARTNS=>");
            if ((int)idx2 >= 0)
              {
                char  numSaltPartnsCharStr[5];
                const char * startNumSaltPartns = 
                  &hbaseCreateOptions.data()[idx2 + strlen("NUM_SALT_PARTNS=>")];
                memcpy(numSaltPartnsCharStr, startNumSaltPartns, 4);
                numSaltPartnsCharStr[4] = 0;
                   
                numSaltPartns = str_atoi(numSaltPartnsCharStr, 4);
                   
                hbaseCreateOptions.remove(idx2, strlen("NUM_SALT_PARTNS=>")+4);
                hbaseCreateOptions = hbaseCreateOptions.strip();
              }
               
            str_sprintf(buf, "update %s.\"%s\".%s set num_salt_partns = %d where table_uid = %ld",
                        getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLES,
                        numSaltPartns,
                        tableUID);
            cliRC = cliInterface.executeImmediate(buf);
               
            if (cliRC < 0)
              {
                cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
                   
                *CmpCommon::diags() << DgSqlCode(-1423)
                                    << DgString0(SEABASE_TABLES);
                   
                mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
                mdui->setSubstep(0);
                   
                return -1;
              }
               
            if (NOT hbaseCreateOptions.isNull())
              {
                if (updateTextTable(&cliInterface, tableUID, 
                                    COM_HBASE_OPTIONS_TEXT, 0,
                                    hbaseCreateOptions))
                  {
                    *CmpCommon::diags() << DgSqlCode(-1423)
                                        << DgString0(SEABASE_TABLES);
                       
                    mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
                    mdui->setSubstep(0);
                       
                    return -1;
                  }
              }
          } // for
           
        if (mdui->step() != CUSTOMIZE_NEW_MD)
          return -1;
           
        str_sprintf(buf, "merge into %s.\"%s\".%s using (select C.object_uid, sum(C.column_size  + case when C.nullable != 0 then 1 else 0 end) from %s.\"%s\".%s C, %s.\"%s\".%s K where C.object_uid = K.object_uid and C.column_number = K.column_number group by 1) T(a, b) on table_uid = T.a when matched then update set key_length = T.b",
                    getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLES,
                    getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
                    getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS);
        cliRC = cliInterface.executeImmediate(buf);
           
        if (cliRC < 0)
          {
            cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
               
            *CmpCommon::diags() << DgSqlCode(-1423)
                                << DgString0(SEABASE_TABLES);
               
            mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
            mdui->setSubstep(0);
               
            return -1;
          }
           
        str_sprintf(buf, "merge into %s.\"%s\".%s using (select object_uid, sum(column_size + case when nullable != 0 then 1 else 0 end), sum(column_size + case when nullable != 0 then 1 else 0 end + %lu + %d), count(*) from %s.\"%s\".%s group by 1) T(a,b,c,d) on table_uid = T.a when matched then update set (row_data_length, row_total_length) = (T.b, T.c + key_length * T.d)",
                    getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLES,
                    sizeof(Int64),
                    5,
                    getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS);
        cliRC = cliInterface.executeImmediate(buf);
           
        if (cliRC < 0)
          {
            cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
               
            *CmpCommon::diags() << DgSqlCode(-1423)
                                << DgString0(SEABASE_TABLES);
               
            mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
            mdui->setSubstep(0);
               
            return -1;
          }
           
        if (mdui->step() != CUSTOMIZE_NEW_MD)
          return -1;
           
        mdui->setMsg("  End: Update TABLES");
        mdui->subStep()++;
           
        if (xnInProgress(&cliInterface))
          {
            cliRC = commitXn(&cliInterface);
            if (cliRC < 0)
              {
                cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
                   
                mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
                mdui->setSubstep(0);
                   
                return -1;
              }
          }
           
        return 0;
      }
      return -1;
         
    case 5:
      {
        mdui->setMsg("  Start: Update TEXT");
        mdui->subStep()++;
           
        cliRC = beginXn(&cliInterface);
        if (cliRC < 0)
          {
            cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
               
            mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
            mdui->setSubstep(0);
               
            return -1;
          }
           
        return 0;
      }
      return -1;
         
    case 6:
      {
        str_sprintf(buf, "update %s.\"%s\".%s set text_type = 1 where text not like 'CREATE VIEW %%' and text not like 'HBASE_OPTIONS=>%%' and NOT (text_type = 4 and text like 'HASH2PARTFUNC%%' ) ",
                    getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TEXT);
        cliRC = cliInterface.executeImmediate(buf);
           
        if (cliRC < 0)
          {
            cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
               
            *CmpCommon::diags() << DgSqlCode(-1423)
                                << DgString0(SEABASE_TEXT);
               
            mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
            mdui->setSubstep(0);
               
            return -1;
          }
           
        if (mdui->step() != CUSTOMIZE_NEW_MD)
          return -1;
           
        mdui->setMsg("  End: Update TEXT");
        mdui->subStep()++;
           
        if (xnInProgress(&cliInterface))
          {
            cliRC = commitXn(&cliInterface);
            if (cliRC < 0)
              {
                cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
                   
                mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
                mdui->setSubstep(0);
                   
                return -1;
              }
          }
           
        return 0;
      }
      return -1;
         
    case 7:
      {
        mdui->setMsg("  Start: Update SEQ_GEN");
        mdui->subStep()++;
           
        cliRC = beginXn(&cliInterface);
        if (cliRC < 0)
          {
            cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
               
            mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
            mdui->setSubstep(0);
               
            return -1;
          }
           
        return 0;
      }
      return -1;
         
    case 8:
      {
        str_sprintf(buf, "delete from %s.\"%s\".%s where object_type = 'PK' and object_uid = (select TC.constraint_uid from %s.\"%s\".%s TC where TC.constraint_type = 'P' and TC.table_uid = (select O2.object_uid from %s.\"%s\".%s O2 where O2.catalog_name = '%s' and O2.schema_name = '%s' and O2.object_name = '%s' and O2.object_type = 'BT')) ",
                    TRAFODION_SYSCAT_LIT, SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                    TRAFODION_SYSCAT_LIT, SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
                    TRAFODION_SYSCAT_LIT, SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                    TRAFODION_SYSCAT_LIT, SEABASE_MD_SCHEMA, SEABASE_SEQ_GEN_OLD_MD);                          
           
        cliRC = cliInterface.executeImmediate(buf);
        if (cliRC < 0)
          {
            cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
               
            mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
            mdui->setSubstep(0);
               
            return -1;
          }
           
        str_sprintf(buf, "delete from %s.\"%s\".%s  where constraint_type = 'P' and table_uid = (select O.object_uid from %s.\"%s\".%s O where O.catalog_name = '%s' and O.schema_name = '%s' and O.object_name = '%s' and O.object_type = 'BT') ",
                    TRAFODION_SYSCAT_LIT, SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
                    TRAFODION_SYSCAT_LIT, SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                    TRAFODION_SYSCAT_LIT, SEABASE_MD_SCHEMA, SEABASE_SEQ_GEN_OLD_MD);                          
           
        cliRC = cliInterface.executeImmediate(buf);
        if (cliRC < 0)
          {
            cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
               
            mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
            mdui->setSubstep(0);
               
            return -1;
          }
           
        if (mdui->step() != CUSTOMIZE_NEW_MD)
          return -1;
           
        mdui->setMsg("  End: Update SEQ_GEN");
        mdui->subStep()++;
           
        if (xnInProgress(&cliInterface))
          {
            cliRC = commitXn(&cliInterface);
            if (cliRC < 0)
              {
                cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
                   
                mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
                mdui->setSubstep(0);
                   
                return -1;
              }
          }
           
        return 0;
      }
      return -1;
         
    case 9:
      {
        mdui->setMsg("  Start: Create Schema Objects");
        mdui->subStep()++;
           
        return 0;
      }
      return -1;
         
    case 10:
      {
        cliRC = createSchemaObjects(&cliInterface);
        if (cliRC < 0)
          {
            mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
            mdui->setSubstep(0);
               
            return -1;
          }
           
        if (mdui->step() != CUSTOMIZE_NEW_MD)
          return -1;
           
        mdui->setMsg("  End: Create Schema Objects");
        mdui->subStep()++;
           
        return 0;
      }
      return -1;
         
    case 11:
      {
        mdui->setMsg("Customize New Metadata: done");

        // done, move to next step.
        return 1;
      }
      return -1;
      
    default:
      {
        mdui->setStep(UPGRADE_FAILED_RESTORE_OLD_MD);
        mdui->setSubstep(0);
      }
      return -1;
    }

  return -1;
}

// ----------------------------------------------------------------------------
// Methods for class CmpSeabaseUpgradeRepository
// ----------------------------------------------------------------------------
NABoolean CmpSeabaseUpgradeRepository::needsUpgrade(CmpSeabaseMDupgrade * ddlState)
{ 
  return ddlState->isReposUpgradeNeeded();
}

short CmpSeabaseUpgradeRepository::doUpgrade(ExeCliInterface * cliInterface,
  CmpDDLwithStatusInfo * mdui,
  CmpSeabaseMDupgrade * ddlState,
  NABoolean /* ddlXns */)
{
  return ddlState->upgradeRepos(cliInterface,mdui);
}

short CmpSeabaseUpgradeRepository::doDrops(ExeCliInterface * cliInterface,
  CmpDDLwithStatusInfo * mdui, CmpSeabaseMDupgrade * ddlState)
{
  return ddlState->upgradeReposComplete(cliInterface,mdui);
}

short CmpSeabaseUpgradeRepository::doUndo(ExeCliInterface * cliInterface,
  CmpDDLwithStatusInfo * mdui, CmpSeabaseMDupgrade * ddlState)
{
  return ddlState->upgradeReposUndo(cliInterface,mdui);
}

// ----------------------------------------------------------------------------
// Methods for class CmpSeabaseUpgradePrivMgr
// ----------------------------------------------------------------------------
NABoolean CmpSeabaseUpgradePrivMgr::needsUpgrade(CmpSeabaseMDupgrade * ddlState)
{ 
  return ddlState->isPrivsUpgradeNeeded();
}

short CmpSeabaseUpgradePrivMgr::doUpgrade(ExeCliInterface * cliInterface,
  CmpDDLwithStatusInfo * mdui,
  CmpSeabaseMDupgrade * ddlState,
  NABoolean ddlXns)
{
  // If this is refactored in the future to return multiple messages
  // then mdui will need to be passed in, and setEndStep called when
  // we want to stop redriving this method.
  NAString doneString;
  short retcode = ddlState->upgradePrivMgr(cliInterface, ddlXns, doneString);
  if (!retcode)
    {
      mdui->setMsg(doneString.data());
      mdui->setEndStep(TRUE);
    }
  return retcode;
}

short CmpSeabaseUpgradePrivMgr::doDrops(ExeCliInterface * cliInterface,
  CmpDDLwithStatusInfo * mdui, CmpSeabaseMDupgrade * ddlState)
{
  // At the moment, the privilege manager does not adhere to the contract.
  // It does its drops inside the CmpSeabaseMDupgrade::upgradePrivMgr
  // method. That is why the upgrade state machine above must do the
  // privilege manager last. Because if there were an error afterward
  // that required us to revert to previous state, we couldn't since
  // the drops have already been done.
  return 0;  // stubbed, until Privilege Manager upgrade is refactored
}

short CmpSeabaseUpgradePrivMgr::doUndo(ExeCliInterface * cliInterface,
  CmpDDLwithStatusInfo * mdui, CmpSeabaseMDupgrade * ddlState)
{
  // At the moment, the privilege manager does not adhere to the contract.
  // It does its recovery inside the CmpSeabaseMDupgrade::upgradePrivMgr
  // method. That is why the upgrade state machine above must do the
  // privilege manager last. Because if there were an error afterward
  // that required us to revert to previous state, we couldn't since
  // the drops have already been done.
  return 0;  // stubbed, until Privilege Manager upgrade is refactored
}
// ----------------------------------------------------------------------------
// method: upgradePrivMgr
//
// This method upgrades the privilege manager repository
//
// Params:
//   cliInterface - pointer to a CLI helper class
//   privMgrDoneMsg - message indicating the result
//
// returns:
//   0 - successful
//  -1 - failed
//
// The diags area is populated with any unexpected errors
// ----------------------------------------------------------------------------
short CmpSeabaseMDupgrade::upgradePrivMgr (
  ExeCliInterface *cliInterface, 
  NABoolean ddlXns,
  NAString &privMgrDoneMsg)
{
  std::vector<std::string> tablesCreated;
  std::vector<std::string> tablesUpgraded;

  // initSeabaseAuthorization will create or upgrade PrivMgr metadata tables
  if (initSeabaseAuthorization(cliInterface, ddlXns, TRUE /*isUpgrade*/,
                               tablesCreated, tablesUpgraded) < 0)
    return -1;

  // Report which tables were created and which were upgraded
  if (tablesCreated.size() > 0)
    privMgrDoneMsg += "  Privilege manager tables created: ";
    {
      for (int i = 0; i < tablesCreated.size(); i++)
        {
          privMgrDoneMsg += tablesCreated[i].c_str();
          privMgrDoneMsg += " ";
        }
    }

  if (tablesUpgraded.size() > 0)
    {
      privMgrDoneMsg +=  "\n  Privilege manager tables upgraded: ";
      for (int i = 0; i < tablesUpgraded.size(); i++)
        {
          privMgrDoneMsg += tablesUpgraded[i].c_str();
          privMgrDoneMsg += " ";
        }
    }
  privMgrDoneMsg += "\nUpgrade Priv Mgr: ended";

  return 0;
}
