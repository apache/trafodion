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
#include <sys/stat.h>
#include "OptimizerSimulator.h"



// static helper function to generate one list of disks
// (returns an array of ExScratchDiskDrive objects)
static ExScratchDiskDrive * genScratchDisks(const NAString &def,
						Lng32 &numDirs,
						Generator *generator,
						const char *defName)
{
  ExScratchDiskDrive *result = NULL;

  // temporary
  //  numDrives = 0;
  //  return result;
  // end temporary

  const char *str = def.data();
  if (!str || str[0]=='\0')
    {
      numDirs = 0;
      return result;		// fast return if empty NADefaults val
    }

  
  // ---------------------------------------------------------------------
  // Convert the strings into a temporary list of ExScratchDiskDrive
  // objects (temporary because we want to make the final list a
  // contiguous array)
  // ---------------------------------------------------------------------
  CollHeap *heap = generator->wHeap();
  Space *space = generator->getSpace();
  LIST(ExScratchDiskDrive *) tempList(heap);
  struct stat st;
  
 
  Lng32 nodeNum;
 
  char *token,*saveptr = NULL;
  //save the pointer to this since token will keep changing.

  char *sep = (char *)":";
  token = strtok_r((char *)str,sep,&saveptr);
  while (token != NULL)
    {
      //validate the directory
      if ((stat(token,&st) != 0 ) &&  !S_ISDIR(st.st_mode) ) //&& (numDirs > MAX_SCRATCH_LOCATIONS))
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
                               token,
                               strlen(token) ));
        }
      token = strtok_r(NULL,sep,&saveptr);
    }
      
 
  token  = NULL;
    

  // ---------------------------------------------------------------------
  // Calculate total generated space needed and allocate it
  // ---------------------------------------------------------------------
  numDirs = tempList.entries();
 
  Lng32 allDirNamesLen = 0;
  char *generatedDirNames = NULL;

  Int32 i=0;
  for (; i<numDirs; i++)
    {
      allDirNamesLen += str_len(tempList[i]->getDirName())+1;
    }

  if (numDirs >0)
    {
      result = new(space) ExScratchDiskDrive[numDirs];
      generatedDirNames = new(space) char[allDirNamesLen];
    }

  // ---------------------------------------------------------------------
  // Loop over the temporary list and copy it into the generated space
  // ---------------------------------------------------------------------
  for (i=0; i<numDirs; i++)
    {
      ExScratchDiskDrive *src = tempList[i];
      Lng32 dirNameLen = src->getDirNameLength();
        
      str_cpy_all(generatedDirNames, src->getDirName(), dirNameLen);
      generatedDirNames[dirNameLen] = 0;
      result[i].setDirName(generatedDirNames);
      result[i].setDirNameLength(dirNameLen);
      generatedDirNames += dirNameLen+1;
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

  NAString sDirs;
  
  enum DefaultConstants sEnum;
  
  const char *sDefaultName;
  
  sEnum = SCRATCH_DIRS;
  sDefaultName = "SCRATCH_DIRS";
 
  // look up defaults
  CmpCommon::getDefault(sEnum,sDirs,0);
  
  // convert into executor structures and give warnings for syntax errors
  Lng32 numEntries;
  ExScratchDiskDrive *l;

  l = genScratchDisks(sDirs, numEntries, generator, sDefaultName);
  result->setSpecifiedScratchDirs(l,numEntries);
 
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
