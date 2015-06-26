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
 * File:         GenResources.cpp
 * Description:  Code to retrieve resource-related defaults from the defaults
 *               table and to add them to a structure in the generated plan.
 *
 * Created:      1/9/99
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "DefaultValidator.h"
#include "Generator.h"
#include "ComResourceInfo.h"
#include "GenResources.h"
#include "SchemaDB.h"

#include "OptimizerSimulator.h"

// static helper function to compare to ExScratchDiskDrive objects
/*
static int compareScratchDiskDrives(const void * ax, const void * bx)
{
  ExScratchDiskDrive * a = (ExScratchDiskDrive *) ax;
  ExScratchDiskDrive * b = (ExScratchDiskDrive *) bx;

  // do a multi-column comparison of cluster#, node#, and disk name
  // $$$ Change this eventually to compare cluster names instead $$$
  if (a->getClusterNumber() < b->getClusterNumber() ||
      (a->getClusterNumber() == b->getClusterNumber() &&
       (a->getNodeNumber() < b->getNodeNumber() ||
	(a->getNodeNumber() == b->getNodeNumber() &&
	 strcmp(a->getDiskName(), b->getDiskName()) < 0))))
    return -1;
  else if (a->getClusterNumber() == b->getClusterNumber() &&
	   a->getNodeNumber() == b->getNodeNumber() &&
	   strcmp(a->getDiskName(), b->getDiskName()) == 0)
    return 0;
  else
    return 1;
} */

// static helper function to generate one list of disks
// (returns an array of ExScratchDiskDrive objects)
static ExScratchDiskDrive * genScratchDriveList(const NAString &def,
						Lng32 &numDrives,
						Generator *generator,
						const char *defName)
{
  ExScratchDiskDrive *result = NULL;

  // temporary
  //  numDrives = 0;
  //  return result;
  // end temporary

  const char *str = def.data();
  if (!*str)
    {
      numDrives = 0;
      return result;		// fast return if empty NADefaults val
    }

  // ---------------------------------------------------------------------
  // Convert the strings into a temporary list of ExScratchDiskDrive
  // objects (temporary because we want to make the final list a
  // contiguous array)
  // ---------------------------------------------------------------------
  LIST(ExScratchDiskDrive *) tempList;
  CollHeap *heap = generator->wHeap();
  Space *space = generator->getSpace();

  // ---------------------------------------------------------------------
  // process the NT default
  // ---------------------------------------------------------------------
  while (str && *str)
    {
      Lng32 nodeNum;
      char *driveLetter = new(heap) char[2];

      driveLetter[1] = 0;
      if (ValidateDiskListNT::getNextDriveLetterAndAdvance(
	   str,nodeNum,driveLetter[0]))
	{
	  // syntax error in default, issue a warning (not an error)
	  *CmpCommon::diags() << DgSqlCode(2055)
			      << DgString0(def)
			      << DgString1(defName);
	  // don't continue after a syntax error
	  str = NULL;
	}
      else
	{
	  tempList.insert(new(heap) ExScratchDiskDrive(
	       driveLetter,
	       1, // Thanks to Bill Gates
	       nodeNum));
	}

      NADELETEBASIC(driveLetter, heap);
      driveLetter = NULL;

    }

  // ---------------------------------------------------------------------
  // Calculate total generated space needed and allocate it
  // ---------------------------------------------------------------------
#pragma nowarn(1506)   // warning elimination 
  numDrives = tempList.entries();
#pragma warn(1506)  // warning elimination 
  Lng32 allClusterNamesLen = 0;
  Lng32 allDiskNamesLen = 0;
  char *generatedClusterNames = NULL;
  char *generatedDiskNames = NULL;

  Int32 i=0;
  for (; i<numDrives; i++)
    {
      allClusterNamesLen += tempList[i]->getClusterNameLength()+1;
      allDiskNamesLen += str_len(tempList[i]->getDiskName())+1;
    }

  if (numDrives >0)
      {
	result = new(space) ExScratchDiskDrive[numDrives];
	generatedClusterNames = new(space) char[allClusterNamesLen];
	generatedDiskNames = new(space) char[allDiskNamesLen];
      }

  // ---------------------------------------------------------------------
  // Loop over the temporary list and copy it into the generated space
  // ---------------------------------------------------------------------
  for (i=0; i<numDrives; i++)
    {
      ExScratchDiskDrive *src = tempList[i];
      Lng32 clusterNameLen = src->getClusterNameLength();
      Lng32 diskNameLen = src->getDiskNameLength();
      if (clusterNameLen)
	{
	  str_cpy_all(generatedClusterNames, src->getClusterName(),
		      clusterNameLen);
	  generatedClusterNames[clusterNameLen] = 0;
	  result[i].setClusterName(generatedClusterNames);
	  generatedClusterNames += clusterNameLen+1;
	}
      else
	{
	  result[i].setClusterName(NULL);
	}
      result[i].setClusterNameLength(clusterNameLen);
      result[i].setClusterNumber(src->getClusterNumber());
      result[i].setNodeNumber(src->getNodeNumber());
      str_cpy_all(generatedDiskNames, src->getDiskName(), diskNameLen);
      generatedDiskNames[diskNameLen] = 0;
      result[i].setDiskName(generatedDiskNames);
      result[i].setDiskNameLength(diskNameLen);
      generatedDiskNames += diskNameLen+1;
    }
  return result;
}

ExScratchFileOptions *genScratchFileOptions(Generator *generator)
{
  Space *space = generator->getSpace();
  ExScratchFileOptions *result = new(space) ExScratchFileOptions;

  // ---------------------------------------------------------------------
  // Compile the defaults table entries into internal format
  // ---------------------------------------------------------------------

  NAString sDisks;
  NAString xDisks;
  NAString pDisks;
  enum DefaultConstants sEnum;
  enum DefaultConstants xEnum;
  enum DefaultConstants pEnum;
  const char *sDefaultName;
  const char *xDefaultName;
  const char *pDefaultName;

  // use two different sets of defaults, dependent on the platform

  sEnum = SCRATCH_DRIVE_LETTERS;
  xEnum = SCRATCH_DRIVE_LETTERS_EXCLUDED;
  pEnum = SCRATCH_DRIVE_LETTERS_PREFERRED;
  sDefaultName = "SCRATCH_DRIVE_LETTERS";
  xDefaultName = "SCRATCH_DRIVE_LETTERS_EXCLUDED";
  pDefaultName = "SCRATCH_DRIVE_LETTERS_PREFERRED";
 
  // look up defaults
  CmpCommon::getDefault(sEnum,sDisks,0);
  CmpCommon::getDefault(xEnum,xDisks,0);
  CmpCommon::getDefault(pEnum,pDisks,0);

  // convert into executor structures and give warnings for syntax errors
  Lng32 numEntries;
  ExScratchDiskDrive *l;

  l = genScratchDriveList(
       sDisks, numEntries, generator, sDefaultName);
  result->setSpecifiedScratchDisks(l,numEntries);

  l = genScratchDriveList(
       xDisks, numEntries, generator, xDefaultName);
  result->setExcludedScratchDisks(l,numEntries);

  l = genScratchDriveList(
       pDisks, numEntries, generator, pDefaultName);
  result->setPreferredScratchDisks(l,numEntries);

  NADefaults &defs = ActiveSchemaDB()->getDefaults(); 
  result->setScratchMgmtOption((Lng32)defs.getAsULong(SCRATCH_MGMT_OPTION));
  result->setScratchMaxOpensHash((Lng32)defs.getAsULong(SCRATCH_MAX_OPENS_HASH));
  result->setScratchMaxOpensSort((Lng32)defs.getAsULong(SCRATCH_MAX_OPENS_SORT));
  if(CmpCommon::getDefault(SCRATCH_PREALLOCATE_EXTENTS) == DF_ON)
    result->setScratchPreallocateExtents(TRUE);
  if(CmpCommon::getDefault(SCRATCH_DISK_LOGGING) == DF_ON)
    result->setScratchDiskLogging(TRUE);
  return result;
}
