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
******************************************************************************
*
* File:         LmContManager.cpp
* Description:  LM's Container Manager
* Created:      07/01/1999
* Language:     C++
*
******************************************************************************
*/

#include "LmAssert.h"
#include "LmContManager.h"
#include "LmDebug.h"
#include "errno.h"
#include <sys/types.h>
#include <sys/stat.h>

// Helper function to determine if a given filename is a directory
NABoolean isDirectory(const char *filename)
{
  NABoolean result = FALSE;

  struct stat statBuf;
  Int32 retcode = stat(filename, &statBuf);

  if (retcode == 0 && S_ISDIR(statBuf.st_mode))
    result = TRUE;

  return result;
}

// Return the file name of a container. Cases to consider:
// * The meta container is a file. Return: path_
// * The meta container is a directory and
//    * the LM is not for Java. Return: path_/containerName
//    * the LM is for Java. To account for Java package names, replace
//      dots in containerName with forward slashes.
//      Return: path_/containerNameWithSlashes + ".class"
static void getContainerFileName(LmMetaContainer &metaContainer, // IN
                                 const char *containerName, // IN
                                 NAString &s) // OUT
{
  s = metaContainer.getPath();
  if (metaContainer.isDir())
  {
    s += "/";
    if (metaContainer.getLM()->getLanguage() == COM_LANGUAGE_JAVA)
    {
      ComUInt32 len = strlen(containerName);
      char *nameWithSlashes = (char *) malloc(len + 1);

      for (ComUInt32 i = 0; i < len; i++)
      {
        char c = containerName[i];
        if (c == '.')
          nameWithSlashes[i] = '/';
        else
          nameWithSlashes[i] = c;
      }

      nameWithSlashes[len] = 0;

      s += nameWithSlashes;
      s += ".class";

      free(nameWithSlashes);
    }
    else
    {
      s += containerName;
    }
  }
}

// Local function to look at the modified time of a given file
static time_t getFileModifiedTime(const char *filename)
{
  time_t mod_time = -1;
  struct stat statBuf;

  Int32 retcode = stat(filename, &statBuf);

  if (retcode == 0)
  {
    mod_time = statBuf.st_mtime;
  }
  else
  {
    // We tolerate if the file is removed or the file is not available
    // because of a down node after LM loaded the file. 
    // This is to allow the processes to continue that have loaded
    // files from a node that is down.
    if (errno == ENOENT || errno == EIO)
      mod_time = 0;
  }

  LM_DEBUG2("getFileModTime(%s) returned %d", filename, (Lng32) mod_time);
  return mod_time;
}

static time_t getModifiedTime(const char *filename, ComDiagsArea *diags)
{
  time_t new_modified_time = getFileModifiedTime(filename);
  
  if (diags && new_modified_time == -1)
  {
    NAString msg = " while checking file ";
    msg += filename;
    msg += ".";
    *diags << DgSqlCode(-LME_INTERNAL_ERROR)
           << DgString0(msg);
  }
  
  return new_modified_time;
}

//////////////////////////////////////////////////////////////////////
//
// Class LmMetaContainer
//
//////////////////////////////////////////////////////////////////////
LmMetaContainer::LmMetaContainer(const char           *path,
                                 LmContainerManager   *contManager,
                                 ComDiagsArea         *diagsArea)
  : path_(path, collHeap()),
    contManager_(contManager),
    isDir_(FALSE)
{
  loader_ = getLM()->createLoader(getPath(), diagsArea);
  if (isDirectory(getPath()))
    isDir_ = TRUE;
}

LmMetaContainer::~LmMetaContainer()
{
  getLM()->deleteLoader(loader_);
}

//////////////////////////////////////////////////////////////////////
//
// Class LmContainer
//
//////////////////////////////////////////////////////////////////////
LmContainer::LmContainer(const char *name,
                         LmHandle handle,
                         const char *fileSystemName)
  : name_(name, collHeap()),
    handle_(handle),
    fileSystemName_(fileSystemName, collHeap()),
    fileModTime_(-1)
{
}

LmContainer::~LmContainer()
{
}

// Check if the file on disk has been updated
NABoolean LmContainer::isContainerCurrent(ComDiagsArea *diagsArea) const
{
  time_t new_modified_time =
    getModifiedTime(getFileSystemName(), diagsArea);

  if (new_modified_time == -1)
    return FALSE;
  
  if (new_modified_time != 0 &&
      new_modified_time != getFileModTime())
    return FALSE;
  
  return TRUE;
}

//////////////////////////////////////////////////////////////////////
//
// Class LmContainerMC
//
//////////////////////////////////////////////////////////////////////
LmContainerMC::LmContainerMC(const char *name,
                             LmHandle handle,
                             LmMetaContainer *owner,
                             const char *fileSystemName)
  : LmContainer(name, handle, fileSystemName),
    owner_(owner)
{
}

LmContainerMC::~LmContainerMC()
{
}

//////////////////////////////////////////////////////////////////////
//
// Class LmContainerManagerCache
//
//////////////////////////////////////////////////////////////////////
LmContainerManagerCache::LmContainerManagerCache(LmLanguageManager *lm,
                                                 ComUInt32 capacity,
                                                 ComBoolean enforceLimits,
                                                 ComDiagsArea *diagsArea,
                                                 ComUInt32 hashTableSize)
  : LmContainerManager(lm),
    maxSize_(capacity),
    curSize_(0),
    loWater_((ComUInt32) ((float)maxSize_  * (float)0.50)),
    midWater_((ComUInt32)((float)maxSize_  * (float)0.70)),
    hiWater_((ComUInt32) ((float)maxSize_  * (float)0.85)),
    checkPeriod_(10),
    updates_(0),
    purge_(FALSE),
    cleans_(0),
    purges_(0),
    hits_(0),
    misses_(0),
    enforceLimits_(enforceLimits)
{
  metaContainers_ = new (collHeap()) HashQueue(collHeap(), hashTableSize);
}

LmContainerManagerCache::~LmContainerManagerCache()
{
  LM_DEBUG0("LmContainerManagerCache destructor");
  LM_DEBUG5("  s=%d, h=%d, m=%d, c=%d, p=%d",
            (Int32) curSize_, (Int32) hits_, (Int32) misses_, (Int32) cleans_,
            (Int32) purges_);

  // De-allocate all MCs.
  LmMetaContainerCache *mc;
  metaContainers_->position();

  while ((mc = (LmMetaContainerCache*)metaContainers_->getNext()) != NULL)
    delete mc;

  // De-allocate the hash table.
  delete metaContainers_;
}

//////////////////////////////////////////////////////////////////////
// CM service: getContainer.
//////////////////////////////////////////////////////////////////////
LmResult LmContainerManagerCache::getContainer(const char *containerName, 
                                               const char *externalPath,
                                               LmContainer **container,
                                               ComDiagsArea *diagsArea)
{
  // Get the MC responsible for this path.
  LmMetaContainer *metaContainer = getMetaContainer(externalPath);
  if (metaContainer == NULL)
  {
    if (curSize_ > maxSize_)
    {
      LM_DEBUG0("*** WARNING: The container cache is full");
      LM_DEBUG2("***   curSize_ %u, maxSize_ %u", curSize_, maxSize_);
      LM_DEBUG1("***   Cache limits are currently %sbeing enforced",
                (enforceLimits_ ? "" : "NOT"));

      if (enforceLimits_)
      {
        // No space for another MC. Right now we return LM out of memory
        // error. But we need to give control over cache to user.
        *diagsArea << DgSqlCode(-LME_OUT_OF_MEMORY)
                   << DgString0(". Could not create metaContainer.");
        return LM_ERR;
      }
    }
    // Create a new MC
    metaContainer = addMetaContainer(externalPath, diagsArea);
    if (metaContainer == NULL)
      return LM_ERR;
  }

  // Get the container from the MC. Diags is filled for us.
  *container = metaContainer->getContainer(containerName, diagsArea);

  // Check if the disk file is changed after the container was created
  if (*container != NULL)
  {
    NABoolean result = (*container)->isContainerCurrent(diagsArea);

    if (result == FALSE)
    {
      if (diagsArea->getNumber(DgSqlCode::ERROR_) > 0)
      {
        // problem checking the file modification time
        return LM_ERR;
      }

      // The disk file is changed. Let's unload meta container
      // and redrive getContainer() to load the new file.
      putContainer(*container);
      *container = NULL;

      // Release this CM's reference on the MC and remove the MC from
      // the hash table. Releasing the reference can potentially
      // destroy the MC if it's ref count drops to zero.
      deleteMetaContainer(metaContainer);
      metaContainer = NULL;

      // Reload the container by calling the getContainer() method. If
      // the MC for this external path has been destroyed, this call
      // to getContainer() will create a new MC instance.
      getContainer(containerName,
		   externalPath,
		   container,
		   diagsArea);
    }
  }

// Exclude the following lines for coverage as caching is not used.
  // Check the cache as required.
  if (updates_ % checkPeriod_ == 0)
    checkCache();

  return (*container != NULL)? LM_OK: LM_ERR;
}

//////////////////////////////////////////////////////////////////////
// CM service: putContainer.
//////////////////////////////////////////////////////////////////////
LmResult LmContainerManagerCache::putContainer(LmContainer *container)
{
  // De-access the container. The call to mc->putContainer() can
  // potentially destroy the mc and container objects.
  LmMetaContainer *mc = ((LmContainerMC *) container)->getOwner();
  LmResult result = mc->putContainer(container);

  // Purge the cache as required.
  if (purge_)
    cleanCache(purge_);

  return result;
}

void LmContainerManagerCache::containerLoaded(ComUInt32 size)
{
  LM_DEBUG1("LmContainerManagerCache::containerLoaded(%lu)", size);
  ++updates_;
  curSize_ += size;
  LM_DEBUG0("  After updating data members:");
  LM_DEBUG3("    updates_ %lu, curSize_ %lu, maxSize_ %lu",
            (ULng32) updates_, curSize_, maxSize_);
}

//////////////////////////////////////////////////////////////////////
// addMetaContainer: Add a new MC to the CM.
//////////////////////////////////////////////////////////////////////
LmMetaContainer *LmContainerManagerCache::addMetaContainer(
  const char        *externalPath,
  ComDiagsArea      *diagsArea)
{
  LmMetaContainerCache *mc;

  LM_DEBUG0("About to call the LmMetaContainerCache constructor");
  LM_DEBUG2("  curSize_ %lu, maxSize_ %lu", curSize_, maxSize_);
  LM_DEBUG1("  EXTERNAL PATH is %s", externalPath);

  // Create the MC.
  mc = new (collHeap()) 
    LmMetaContainerCache(externalPath, this, diagsArea);

  LM_DEBUG1("LmMetaContainerCache constructor returned %p", mc);
  LM_DEBUG2("  curSize_ %u, maxSize_ %u", curSize_, maxSize_);

  if (mc == NULL)
  {
    *diagsArea << DgSqlCode(-LME_OUT_OF_MEMORY)
	       << DgString0(". Could not create metaContainer.");
    return NULL;
  }

  // if LM cannot create a loader for this MC, return NULL. LM has already
  // written diags information.
  if (mc->getLoader() == NULL)
  {
    delete mc;
    return NULL;
  }

  // Add the MC to the cache (hash table).
  metaContainers_->insert((char *) mc->getPath(), str_len(mc->getPath()), mc);

  // Acquire a reference on the MC. Each container loaded by the MC
  // will also own a reference.
  ((LmMetaContainerCache*) mc)->ref();

  return mc;
}

//////////////////////////////////////////////////////////////////////
//// deleteMetaContainer: Delete an MC from the CM.
////////////////////////////////////////////////////////////////////////
NABoolean LmContainerManagerCache::deleteMetaContainer(
  LmMetaContainer *metaCont)
{
  if (metaCont != NULL)
  {
    metaContainers_->remove(metaCont);
    ((LmMetaContainerCache*) metaCont)->deref();
  }

  return TRUE;
}

//////////////////////////////////////////////////////////////////////
// getMetaContainer: Look-up a MC in the cache.
//////////////////////////////////////////////////////////////////////
LmMetaContainer *LmContainerManagerCache::getMetaContainer(const char *path)
{
  Int32 len = str_len(path);
  LmMetaContainerCache *mc;
  metaContainers_->position((char*)path, len);

  while ((mc = (LmMetaContainerCache*)metaContainers_->getNext()) != NULL)
  {
    if (str_cmp(path, mc->getPath(), len) == 0)
      break;
  }

  if (mc != NULL)
    ++hits_;
  else
    ++misses_;

  return mc;
}

// Exclude the following lines for coverage as caching is not used.
//////////////////////////////////////////////////////////////////////
// checkCache: Check if the cached needs cleaning or decaying.
//////////////////////////////////////////////////////////////////////
void LmContainerManagerCache::checkCache()
{
  updates_ = 0; 

  if (curSize_ > hiWater_)
  {
    cleanCache(FALSE);
  }
  else if (curSize_ > loWater_)
  {
    decayCache();
  }
}

//////////////////////////////////////////////////////////////////////
// cleanCache: Reduces the size of the cache. If purge is FALSE, MC
// usage stats are honored when looking for victims. If purge is TRUE,
// usage stats are not honored. Purging only happens if cleaning initially
// does not reduce the cache to acceptable limit.
//////////////////////////////////////////////////////////////////////
void LmContainerManagerCache::cleanCache(ComBoolean purge)
{
  // Try a finite number of times.
  Int32 attempts = metaContainers_->entries() / 4 + 1;

  // Update stats.
  if (purge)
    ++purges_;
  else
    ++cleans_;

  // Reduce the cache size.
  for (Int32 i = 0; i < attempts && curSize_ > midWater_; i++)
  {
    LmMetaContainerCache *mc;
    LmMetaContainerCache *mr = NULL;

    // Globally iterate over the hash table.
    metaContainers_->position();

    while ((mc = (LmMetaContainerCache*)metaContainers_->getNext()) != NULL)
    {
      // Only clean (un-referenced) MCs can be considered.
      if (mc->clean()) 
      {
        // If purging, evict it.
        if (purge) 
	{
	  mr = mc;
	  break;
	}

        // If cleaning, find the least-used MC.
        if (mr != NULL && mc->usecnt() < mr->usecnt())
          mr = mc;
        else
          mr = mc;
      }
    }

    // A victim for removal was found.
    if (mr) 
    {
      metaContainers_->position((char *) mr->getPath(),
                                str_len(mr->getPath()));

      while ((mc = (LmMetaContainerCache*)metaContainers_->getNext()) != mr);
      LM_ASSERT(mc == mr);

      metaContainers_->remove(mr);
      curSize_ -= mr->capacity();
      delete mr;
    }
  }

  // If the cache is still to big, purge it now if not in a purge cycle 
  // (purge=FALSE) or later if already in a purge cycle.
  purge_ = (curSize_ > midWater_);

  if (purge_ && !purge)
    cleanCache(purge_); 
}

//////////////////////////////////////////////////////////////////////
// decayCache: Decay the usage stats for all MCs. The rate of decay
// for a MC is a function of its usage and the current capcity of the
// cache.
//////////////////////////////////////////////////////////////////////
void LmContainerManagerCache::decayCache()
{
  float d;
  LmMetaContainerCache *mc;
  metaContainers_->position();

  d = (curSize_ <= maxSize_)? 
      ((float)1 - (float)curSize_ / (float)maxSize_): 0;

  while ((mc = (LmMetaContainerCache*)metaContainers_->getNext()) != NULL)
    mc->decay(d);
}

//////////////////////////////////////////////////////////////////////
//
// Class LmMetaContainerCache
//
//////////////////////////////////////////////////////////////////////
LmMetaContainerCache::LmMetaContainerCache(
  const char           *path,
  LmContainerManager   *contManager,
  ComDiagsArea         *diagsArea) :
  LmMetaContainer(path, contManager, diagsArea),
  capacity_(0),
  hits_(0),
  misses_(0),
  refcnt_(0),
  usecnt_(0)
{
  containers_ = new (collHeap()) HashQueue(collHeap(), cm()->hashTableSize());
}

LmMetaContainerCache::~LmMetaContainerCache()
{
  LM_DEBUG0("LmMetaContainerCache destructor");
  LM_DEBUG5("  %s, c=%d, s=%d, h=%d, m=%d\n", 
            getPath(), (Int32) containers_->entries(), (Int32) capacity_,
            (Int32) hits_, (Int32) misses_);

  // De-allocate all containers.
  LmContainer *c;
  containers_->position();

  while ((c = (LmContainer*)containers_->getNext()) != NULL)
  {
    getLM()->unloadContainer(c->getHandle());
    delete c;
  }

  // De-allocate the hash table.
  delete containers_;
}

//////////////////////////////////////////////////////////////////////
// getContainer: Get the specified container.
//////////////////////////////////////////////////////////////////////
LmContainer *LmMetaContainerCache::getContainer(const char *containerName,
                                                ComDiagsArea *diagsArea)
{
  LmContainer *c;
  LmHandle h;
  Int32 containerNameLen = str_len(containerName);
  ComUInt32 size;

  // First, look for it in the cache.
  containers_->position((char*) containerName, containerNameLen);

  while ((c = (LmContainer*)containers_->getNext()) != NULL)
  {
    if (str_cmp(containerName, c->getName(), containerNameLen) == 0) 
    {
      // Found it.
      ref();
      return c;
    }
  }

  // Otherwise, call the LM to have it accessed from the underlying loader.
  h = getLM()->loadContainer(containerName, getPath(),
                             loader_, &size, diagsArea);
  if (h == 0)
  {
    // Not found--i.e, this container does not exist!
    // diagsArea is filled by LM. Just return.
    ++misses_;
    return NULL;
  }

  NAString nameToCheck;
  getContainerFileName(*this,         // IN
                       containerName, // IN
                       nameToCheck);  // OUT

  time_t modified_time = getModifiedTime(nameToCheck.data(), diagsArea);
  if (modified_time == -1)
  {
    // Diags are already filled
    return NULL;
  }

  // Create a new container to represent the actual container.
  c = new (collHeap()) LmContainerMC(containerName,
                                     h,
                                     this,
                                     nameToCheck.data());
  if (! c)
  {
    *diagsArea << DgSqlCode(-LME_OUT_OF_MEMORY)
	       << DgString0(". Could not create container.");
    return NULL;
  }

  c->setFileModTime(modified_time);

  // Add it to the cache (hash table).
  containers_->insert((char *) c->getName(), str_len(c->getName()), c);
  ref(FALSE);

  // Adjust the CM's cache size.
  LM_DEBUG0("Inside LmMetaContainerCache::getContainer()...");
  LM_DEBUG2("  path %s, container %s", getPath(), containerName);
  LM_DEBUG2("  LM just loaded %u bytes. MC capacity_ is now %u",
            size, capacity_);
  LM_DEBUG1("  About to notify our CM that we loaded %u bytes",
            (ULng32) (size - capacity_));
  cm()->containerLoaded(size - capacity_);

  // Re-set the capacity.
  capacity_ = size;
  LM_DEBUG1("  MC capacity_ is now %u", capacity_);

  return c;
}

//////////////////////////////////////////////////////////////////////
// putContainer: de-access the container.
//////////////////////////////////////////////////////////////////////
LmResult LmMetaContainerCache::putContainer(LmContainer *c)
{
  // Note: the call to deref() can potentially destroy this instance
  // if refcnt_ drops to zero.
  deref();
  return LM_OK;
}

//////////////////////////////////////////////////////////////////////
//
// Class LmContainerSimple
//
/////////////////////////////////////////////////////////////////////
LmContainerSimple::LmContainerSimple(const char *name,
                                     LmHandle handle,
                                     const char *fileSystemName)
  : LmContainer(name, handle, fileSystemName),
    refCount_(0)
{
}

LmContainerSimple::~LmContainerSimple()
{
}

//////////////////////////////////////////////////////////////////////
//
// Class LmContainerManagerSimple
//
/////////////////////////////////////////////////////////////////////
LmContainerManagerSimple::LmContainerManagerSimple(LmLanguageManager *lm)
  : LmContainerManager(lm)
{
  containers_ = new (collHeap()) HashQueue(collHeap());
}
  
LmContainerManagerSimple::~LmContainerManagerSimple()
{
  // Release all containers
  LmContainerSimple *c = NULL;
  containers_->position();
  while ((c = (LmContainerSimple *) containers_->getNext()) != NULL)
  {
    getLM()->unloadContainer(c->getHandle());
    delete c;
  }
  
  // Delete the hash table
  delete containers_;
}

LmResult LmContainerManagerSimple::getContainer(const char *containerName, 
                                                const char *externalPath,
                                                LmContainer **container,
                                                ComDiagsArea *diagsArea)
{
  LM_ASSERT(container);
  *container = NULL;

  NAString fileSystemName = externalPath;
  fileSystemName += "/";
  fileSystemName += containerName;

  const char *fsName = fileSystemName.data();
  ComUInt32 len = fileSystemName.length();

  LmContainer *lmc = NULL;
  LmHandle h = 0;
  LmResult result = LM_OK;

  // Cases to consider
  // a. The container is not found. Load it, set the ref count to
  //    1, and return the container.
  // b. The container is found and has a ref count > 0. We cannot 
  //    load a new image. There is no need for a timestamp check. 
  //    Increment the ref count and return the container.
  // c. The container is found, the ref count is 0, and the 
  //    timestamp on disk has not changed. Increment the ref count
  //    and return the container.
  // d. The container is found, the ref count is 0, and the 
  //    timestamp on disk has changed. Unload the container, load
  //    the new image, set the ref count to 1, and return the 
  //    container.

  containers_->position((char *) fsName, len);
  while ((lmc = (LmContainerSimple *) containers_->getNext()) != NULL)
  {
    if (str_cmp(fsName, lmc->getFileSystemName(), (Int32) len) == 0)
      break;
  }

  if (lmc == NULL)
  {
    // Case a. The container is not found.

    // Retrieve the current timestamp
    time_t modTime = getModifiedTime(fsName, diagsArea);
    if (modTime == -1)
      result = LM_ERR;

    if (result == LM_OK)
    {
      // Load the library
      h = getLM()->loadContainer(containerName, externalPath,
                             NULL, NULL, diagsArea);
      if (h == 0)
        result = LM_ERR;
    }

    if (result == LM_OK)
    {
      // Create a new container and insert it into the hash table
      lmc = new (collHeap()) LmContainerSimple(containerName, h, fsName);
      lmc->setFileModTime(modTime);
      ((LmContainerSimple *) lmc)->incrRefCount();
      containers_->insert((char *) fsName, len, lmc);
    }

  } // if (lmc == NULL)
  
  else
  {
    // Cases b, c, and d. The container is found.

    LmContainerSimple *simple = (LmContainerSimple *) lmc;
    if (simple->getRefCount() > 0)
    {
      // Case b. The ref count is > 0. Do not refresh the image.
      simple->incrRefCount();
    }
    else
    {
      // Cases c and d. The ref count is 0.

      // See if the timestamp on disk has changed.
      NABoolean isCurrent = lmc->isContainerCurrent(diagsArea);

      if (isCurrent)
      {
        // Case c. The timetamp has not changed. Do not refresh the
        // image.
        simple->incrRefCount();
      }
      else
      {
        // Case d. The timestamp changed. Load a new image.

        // First see if errors were encountered during the timestamp
        // check.
        if (diagsArea->getNumber(DgSqlCode::ERROR_) > 0)
          result = LM_ERR;
        
        // Unload the current image and load a new image by redriving
        // getContainer().
        if (result == LM_OK)
        {
          getLM()->unloadContainer(lmc->getHandle());
          containers_->remove(lmc);
          delete lmc;
          lmc = NULL;
          result = getContainer(containerName, externalPath,
                                &lmc, diagsArea);
        }
      }

    } // if (ref count > 0) else ...
  } // if (lmc == NULL) else ...
  
  if (result == LM_OK)
    *container = lmc;
  
  return result;
}

LmResult LmContainerManagerSimple::putContainer(LmContainer *container)
{
  // All we do is decrement the ref count. The container remains
  // loaded. The only time a container gets unloaded is when
  // getContainer() detects a new timestamp on disk.
  LmContainerSimple *c = (LmContainerSimple *) container;
  LM_ASSERT(c->getRefCount() > 0);
  c->decrRefCount();
  return LM_OK;
}

