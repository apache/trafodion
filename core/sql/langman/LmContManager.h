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
#ifndef LMCONTMANAGER_H
#define LMCONTMANAGER_H
/* -*-C++-*-
******************************************************************************
*
* File:         LmContManager.h
* Description:  Language Manager's Container Manager definitions
* Created:      07/01/1999
* Language:     C++
*
******************************************************************************
*/
#ifdef LANGMAN
#include "LmComQueue.h"
#else
#include "ComQueue.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>

#include "LmLangManager.h"

//////////////////////////////////////////////////////////////////////
//
// Contents of this file
//
//////////////////////////////////////////////////////////////////////
class LmContainerManager;
class LmMetaContainer;
class LmContainer;

// LmContainerManagerCache -- A container manager (CM) subclass that
// manages a collection of meta-containers (MCs) and uses a caching
// mechanism to track how many bytes of memory are used by the
// containers. Used by the Java language manager.
class LmContainerManagerCache;

// LmMetaContainerCache -- An MC subclass that supports caching. Used
// by LmContainerManagerCache.
class LmMetaContainerCache;

// LmContainerMC -- The container subclass used by MCs
class LmContainerMC;

// LmContainerManagerSimple -- A CM subclass that manages containers
// without the user of MCs. Used by the C language manager.
class LmContainerManagerSimple;

// LmContainerSimple -- A container subclass that carries a reference
// count.
class LmContainerSimple;

//////////////////////////////////////////////////////////////////////
//
// LmContainerManager 
//
// The LmContainerManager (CM) is an abstract class defining a set of 
// services used by a LM to access and de-access containers (Java class
// files, C DLLs, etc.) in a language-independent fashion. 
// 
//////////////////////////////////////////////////////////////////////
class LmContainerManager : public NABasicObject
{
public:

  LmContainerManager(LmLanguageManager *lm)
    : lm_(lm)
  {
  }

  virtual ~LmContainerManager()
  {
  }

  LmLanguageManager *getLM() { return lm_; }

  //////////////////////////////////////////////////////////////////////
  // getContainer service: Called by a LM to load a container. Returns
  // an LmContainer representing the underlying external container.
  //////////////////////////////////////////////////////////////////////
  virtual LmResult getContainer(const char *containerName, 
                                const char *externalPath,
                                LmContainer **container,
                                ComDiagsArea *diags) = 0;

  //////////////////////////////////////////////////////////////////////
  // putContainer service: Called by LM when it is done with a container.
  //////////////////////////////////////////////////////////////////////
  virtual LmResult putContainer(LmContainer *container) = 0;

protected:

  LmLanguageManager *lm_;

};

//////////////////////////////////////////////////////////////////////
//
// LmMetaContainer 
//
// The LmMetaContainer is an abstract class representing a MC. It is
// used internally by a CM. An MC manages access to a set of
// containers under the external path that it manages. The MC may rely
// on an LM specific external loader (e.g., a Java class loader) in
// order to provide access to its container set.
//
//////////////////////////////////////////////////////////////////////
class LmMetaContainer : public NABasicObject
{
public:
  LmMetaContainer(
    const char         *path,
    LmContainerManager *contManager,
    ComDiagsArea       *diagsArea);

  virtual ~LmMetaContainer();

  // CM Services.
  virtual LmContainer *getContainer(const char *name, ComDiagsArea *diags) = 0;
  virtual LmResult putContainer(LmContainer*) = 0;

  // Accessors.
  const char *getPath() const { return path_; }

  LmLanguageManager *getLM() { return contManager_->getLM(); }

  LmHandle getLoader() const { return loader_; }

  NABoolean isDir() const { return isDir_; }

protected:
  NAString path_;                   // Path managed by MC.
  LmContainerManager *contManager_; // Owning CM.
  LmHandle loader_;                 // Language specific container loader.
  NABoolean isDir_;                 // TRUE for directories,
                                    //  FALSE for jar files
};

//////////////////////////////////////////////////////////////////////
//
// LmContainer 
//
// The LmContainer class represents a code library such as a Java
// class file or C DLL.
//
//////////////////////////////////////////////////////////////////////
class LmContainer : public NABasicObject
{
public:
  LmContainer(const char *name,
              LmHandle handle,
              const char *fileSystemName);

  virtual ~LmContainer();

  LmHandle getHandle() const { return handle_; }

  const char *getName() const { return name_; }

  time_t getFileModTime() const { return fileModTime_; }

  void setFileModTime(time_t time) { fileModTime_ = time; }

  const char *getFileSystemName() const { return fileSystemName_; }

  NABoolean isContainerCurrent(ComDiagsArea *diagsArea) const;

protected:
  
  // Name of the container. This will be the simple name of a Java
  // class or C DLL. It will not contain a file system directory name.
  NAString name_;
  
  // A language-specific handle. Could be the handle to a Java class
  // object or a C library handle.
  LmHandle handle_;
  
  // Fully qualified file system name. Could be a DLL, jar file, or
  // a Java class file.
  NAString fileSystemName_;
  
  // Disk file modification time
  time_t fileModTime_;
};

//////////////////////////////////////////////////////////////////////
//
// LmContainerMC
//
// LmContainerMC is a container subclass representing a container such
// as a Java class file that is managed as part of a meta-container
// such as a Jar file or directory. LmContainerMC instances will only
// be created by CM subclasses that manage meta-containers.
//
//////////////////////////////////////////////////////////////////////
class LmContainerMC : public LmContainer
{
public:
  LmContainerMC(const char *name,
                   LmHandle handle,
                   LmMetaContainer *owner,
                   const char *fileSystemName);

  virtual ~LmContainerMC();

  LmMetaContainer *getOwner() { return owner_; }

protected:
  LmMetaContainer *owner_;
};

//////////////////////////////////////////////////////////////////////
//
// LmContainerManagerCache
//
// The LmContainerManagerCache is a concrete class representing a CM 
// that manages a set of MCs, including using a caching scheme to decide
// when a MC should be removed from its cache. The cache removal scheme
// used by this CM is to remove an entire MC from its cache rather than
// just a single container managed by an MC. This scheme was chosen 
// because it is inline with how a Java class loader works--i.e.,
// a class loader never unloads classes, but rather all classes are 
// unloaded when the class loader is gargbage colleted. 
//
// Internally the model is that a CM manages a set of MCs and each MC
// manages the containers under a specific external path in the
// underlying OS file space. Additionally the CM and MC use services
// provided by the LM so that they are not external language specific.
//
// One LmContainerManagerCache instance is created and used by the
// Java LM.
//
// Another variation on a caching CM would be to remove entries at
// the container level.
//
//////////////////////////////////////////////////////////////////////
class LmContainerManagerCache : public LmContainerManager
{
public:
  LmContainerManagerCache(LmLanguageManager *lm,
                          ComUInt32 capacity, 
                          ComBoolean enforceLimits,
                          ComDiagsArea *diagsArea,
                          ComUInt32 hashTableSize = 11);
  
  virtual ~LmContainerManagerCache();

  // Service methods.
  virtual LmResult getContainer(const char *containerName, 
                                const char *externalPath,
                                LmContainer **container,
                                ComDiagsArea *diagsArea);

  virtual LmResult putContainer(LmContainer *container);

  // MC support methods.
  void containerLoaded(ComUInt32 size);

  ComUInt32 hashTableSize()
    { return metaContainers_->size(); }

protected:

  virtual LmMetaContainer *addMetaContainer(const char *externalPath, 
                                            ComDiagsArea *diagsArea);

  virtual NABoolean deleteMetaContainer(LmMetaContainer *mCont);

  virtual LmMetaContainer *getMetaContainer(const char *externalPath);

private:
  // Cache management methods.
  void checkCache();
  void cleanCache(ComBoolean purge);
  void decayCache();

private:
  // The MCs are maintained in a hash table.
  HashQueue *metaContainers_; 

  // Max size (bytes) for cache (i.e., bytes used by containers).
  ComUInt32 maxSize_;  // Max size (bytes) for the cache.

  // Current size (bytes) of the cache.
  ComUInt32 curSize_;  

  // Lo-water mark: used to trigger decaying of cache entries.
  ComUInt32 loWater_;  

  // Mid-water mark: point at which cache cleaning (removal of cache
  // entires using usage stats) or cache purging (removal of entries
  // ignoring usage stats), can stop.
  ComUInt32 midWater_; 

  // Hi-water mark:  used to trigger cache cleaning.
  ComUInt32 hiWater_;  

  // Number of cache additions until the cache is checked.
  ComUInt16 checkPeriod_;

  // Cache update counter.
  ComUInt16 updates_;

  // Flag to specify that cache purging is needed.
  ComUInt16 purge_;
  ComUInt16 unused_;

  // Cache stats.
  ComUInt16 cleans_;  // # of cache cleans.
  ComUInt16 purges_;  // # of cache purges. 
  ComUInt16 hits_;    // # of cache (MC) hits.
  ComUInt16 misses_;  // # of cache (MC) misses.

  // Boolean to control whether or not cache limits will be
  // enforced. If this is FALSE then we never refuse a request to
  // create a new meta container even if our cache max size has been
  // reached.
  ComBoolean enforceLimits_;

};

//////////////////////////////////////////////////////////////////////
//
// LmMetaContainerCache
//
// The LmMetaContainerCache is a concrete class representing a caching
// MC as defined by the caching CM (LmContainerManagerCache) above.
//
//////////////////////////////////////////////////////////////////////
class LmMetaContainerCache : public LmMetaContainer
{
friend class LmContainerManagerCache;

public:
  LmMetaContainerCache(
    const char         *path,
    LmContainerManager *contManager,
    ComDiagsArea       *diagsArea);

  virtual ~LmMetaContainerCache();

  // CM Services.
  virtual LmContainer *getContainer(const char *name, ComDiagsArea *diagsArea);
  virtual LmResult putContainer(LmContainer*);

  // Accessor/mutators.
  ComUInt32 capacity()
    { return capacity_; }

  ComBoolean clean()
    { return refcnt_ < 1; }

  void decay(float scale)
    { usecnt_ = (ComUInt16)(usecnt_ * scale); }

  ComUInt16 usecnt()
    { return usecnt_; }

private:
  // Internal support methods.
  LmContainerManagerCache *cm()
   { return (LmContainerManagerCache*)contManager_; }

  void ref(ComBoolean hit=TRUE)
    { ++refcnt_; 
      ++usecnt_;
      if (hit) ++hits_; else ++misses_;
    }

  void deref()
    { --refcnt_;
      if (refcnt_ == 0)
        delete this;
    }

private:

  // The containers are maintained in a hash table.
  HashQueue *containers_; 

  // Bytes occupied by the loaded containers.
  ComUInt32 capacity_; 

  // Container hit/miss stats.
  ComUInt16 hits_;       
  ComUInt16 misses_;    

  // Ref count: # of active containers.
  ComUInt16 refcnt_;  

  // Usage count for MC.
  ComUInt16 usecnt_;
};

//////////////////////////////////////////////////////////////////////
//
// LmContainerManagerSimple
//
// LmContainerManagerSimple is a concrete class representing a CM that
// manages a set of containers. No MCs are used.
//
//////////////////////////////////////////////////////////////////////
class LmContainerManagerSimple : public LmContainerManager
{
public:
  LmContainerManagerSimple(LmLanguageManager *lm);
  
  virtual ~LmContainerManagerSimple();

  virtual LmResult getContainer(const char *containerName, 
                                const char *externalPath,
                                LmContainer **container,
                                ComDiagsArea *diagsArea);
  
  virtual LmResult putContainer(LmContainer *container);

protected:

  // Containers are maintained in a hash table.
  HashQueue *containers_; 

};

//////////////////////////////////////////////////////////////////////
//
// LmContainerSimple
//
// LmContainerSimple is a concrete class representing a container that
// carries a reference count.
//
//////////////////////////////////////////////////////////////////////
class LmContainerSimple : public LmContainer
{
public:
  LmContainerSimple(const char *name,
                    LmHandle handle,
                    const char *fileSystemName);

  virtual ~LmContainerSimple();

  ComUInt32 getRefCount() const { return refCount_; }

  void incrRefCount() { refCount_++; }
  void decrRefCount() { refCount_--; }

protected:
  ComUInt32 refCount_;
};

#endif 
