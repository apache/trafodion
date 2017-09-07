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
****************************************************************************
*
* File:         NAVersionedObject.cpp
*
* Description:  The NAVersionedObject class encapsulates the version header
*               added to each SQL Executor Plan object stored on disk. It
*               is the base class for the classes these objects belong. It
*               also provides driver routines for the packing and unpacking
*               processes on those objects.
*
*               During packing, the objects are converted to a predefined
*               reference layout and pointers in the objects are converted
*               into offsets. During unpacking, object images are modified
*               to fit in the local platform; older objects are migrated to
*               their current versions.
*
*               The NAVersionedObjectPtr class encapsulate a pointer to a
*               NAVersionedObject. The class is packed with fillers on a
*               32-bit platform so that the class size is always 64 bits.
*               This is done to smoothen transition to 64-bit platforms in
*               the future. Objects of the class are used inside all plan
*               objects derived from NAVersionedObject, so that these plan
*               objects will have the same size on 64-bit platforms.
*
* Created:      9/8/98
* Language:     C++
* Status:       $State: Exp $
*
*
*
****************************************************************************
*/

#include "Platform.h"


#include "NAVersionedObject.h"


// =====================================================================
// Function bodies of class NAVersionedObject
// =====================================================================

// ---------------------------------------------------------------------
// Constructor
//
// Note that imageSize_ and versionIDArray_ are only filled in at
// packing time. These values are not useful on the source platform
// where the objects are constructed. They are only useful at the
// destination while the objects are unpacked. Before the objects are
// packed, drivePack() will make sure their values are correctly set.
// This arrangement avoids the dependence on the constructors of sub-
// classes of NAVersionedObject on setting these values correctly.
// ---------------------------------------------------------------------
NAVersionedObject::NAVersionedObject(Int16 classID)
    : classID_(classID),
      reallocatedAddress_(NULL),
      imageSize_(0)
  {
    clearFillers();
    initFlags();    // set to state of "not packed".
    clearVersionIDArray();
    str_cpy_all(eyeCatcher_,VOBJ_EYE_CATCHER,VOBJ_EYE_CATCHER_SIZE);
  }

// ---------------------------------------------------------------------
// All subclasses could redefine convertToReference/LocalPlatform() to
// convert their members to the reference platform from the local
// platform and vice versa. Typically, this only involves toggling the
// endianness of some members.
// ---------------------------------------------------------------------
void NAVersionedObject::convertToReferencePlatform()
  {
#ifndef NA_LITTLE_ENDIAN
    toggleEndianness();
#endif
  }

void NAVersionedObject::convertToLocalPlatform()
  {
#ifndef NA_LITTLE_ENDIAN
    toggleEndianness();
#endif
  }

// ---------------------------------------------------------------------
// reallocateImage() provides the basic implementation for the virtual
// function migrateToNewVersion(). It is called when the new version
// object has a larger size than the older version object image we
// have. A new object is allocated. The old image is overlay onto it.
// The left-over space will be zero'ed. Finally, reallocatedAddress_
// field in the older object is set to the address of the new object.
// ---------------------------------------------------------------------
NAVersionedObject *NAVersionedObject::reallocateImage(void * reallocator)
  {
    Space *space = (Space *)reallocator;

    short size = getClassSize();
    char *newObjPtr = (char *)( (space == NULL) ?
                                (::operator new(size)) :
                                (space->allocateAlignedSpace(size)) );
    str_pad(newObjPtr,size,0);
    str_cpy_all(newObjPtr,(char *)this,imageSize_);
    setReallocatedAddress((NAVersionedObject *)newObjPtr);
    ((NAVersionedObject *)(newObjPtr))->setImageSize(size);
    return (NAVersionedObject *)(newObjPtr);
  }

// ---------------------------------------------------------------------
// This is a utility for use by redefined migrateToNewVersion() at the
// subclass level. Given the old class size and the new class size, it
// expands the room for members of a particular subclass inside an image
// of possibly another subclass down the derivation chain. It assumes
// the image has been reallocated so that it is big enough to make this
// expansion.
// ---------------------------------------------------------------------
void NAVersionedObject::makeRoomForNewVersion(
                                                  short oldSubClassSize,
                                                  short newSubClassSize)
  {
    // -----------------------------------------------------------------
    // This is an intermediate subclass in derivation chain which needs
    // expansion. Shift all members of subclass(es) lower in the deri-
    // vation chain downwards to make room.
    // -----------------------------------------------------------------
    if (imageSize_ != newSubClassSize)
    {
      char *src = (char *)this + oldSubClassSize;
      char *des = (char *)this + newSubClassSize;
#pragma nowarn(1506)   // warning elimination 
      short siz = imageSize_ - newSubClassSize;
#pragma warn(1506)  // warning elimination 
      str_cpy_all(des,src,siz);
    }
  }
                                    
// ---------------------------------------------------------------------
// Subclasses could redefine migrateToNewVersion() when a new version
// is introduced according to the following template:
//
// long SubClass::migrateToNewVersion(NAVersionedObject *&newImage)
// {
//   if (newImage == NULL)
//   {
//     newImage = ( getImageSize() == getClassSize() ?
//                  this :
//                  reallocateNewImage() );
//   }
//
//   // Version not supported when migrating base class.
//   if (BaseClass::migrateToNewVersion(newImage)) return -1;
//
//   // ?n is the current version of SubClass. ?cs1 is the class size
//   // of version 1, ?cs2 is the class size of version 2, ..., etc.
//   //
//   const short classSizesArray[?n] = { ?cs1, ?cs2, ..., ?csn };
//
//   short newClassSize = SubClass::getClassSize(); // or ?csn
//
//   // ?SUBCLASS_LEVEL begins with 0 if SubClass is directly derived
//   // from NAVersionedObject and increases down the derivation chain.
//   //
//   unsigned char version = getImageVersionID(?SUBCLASS_LEVEL);
//   short oldClassSize = classSizesArray[version];
// 
//   if (oldClassSize != newClassSize)
//     makeRoomForNewVersion(oldClassSize,newClassSize);
//
//   // Implement migration of old members other than size difference OR
//   // return -1 if it was decided that a particular version shouldn't
//   // be supported anymore. Note that new members should be initialized
//   // only later at initNewMembers().
//   //
//   switch (version)
//   {
//     // provides code to migrate image from version 1 to 2.
//     case 1:
//     // provides code to migrate image from version 2 to 3.
//     case 2:
//     // ... upto version ?(n-1) to ?n.
//   };
//
//   return 0;
// }
//       
// This method is redefined by following a strategy similar to RelExpr::
// copyTopNode() in the optimizer directory. Each subclass invokes the
// same method on its base class and then handles the migration of its
// own members. The object is reallocated if needed at the "real" sub-
// class the object belongs.
//
// A return code of -1 is used to indicate that the object has a version
// which is not supported anymore. Also notice that the versionIDArray_
// should only be updated at initNewMembers().
// ---------------------------------------------------------------------
Lng32 NAVersionedObject::migrateToNewVersion(
                                           NAVersionedObject *&newImage)
  {
    short tempimagesize = getClassSize();
    // -----------------------------------------------------------------
    // The base class implementation of migrateToNewVersion() is only
    // called with newImage == NULL when the same function is not
    // redefined at the subclass. That means no new version of that
    // subclass has been invented yet.
    // -----------------------------------------------------------------
    if (newImage == NULL)
    {
      assert(imageSize_ == getClassSize());

      // ---------------------------------------------------------------
      // Should also assert this->versionIDArray_ is same as the one
      // generated from populateVersionIDArray(). But it takes too much
      // time.
      // ---------------------------------------------------------------
      // !!! -----------------------------------------------------------
      // CODE SPECIFIC TO FIRST RELEASE: to be removed at the second
      // release : to guard against the case where a 1st release exec-
      // utor is used to execute 2nd release plan. A more general mech-
      // anism should be developed at the 2nd release to guard against
      // this case.
      // ---------------------------------------------------------------
      unsigned char * versionIDArray = getImageVersionIDArray();
      for (Int32 i = 0; i < VERSION_ID_ARRAY_SIZE; i++)
      {
        if ((versionIDArray[i] != 0) && 
            (versionIDArray[i] != 1) && 
            (versionIDArray[i] != 2) &&
            (versionIDArray[i] != 3))
           return -1;
      }

      // ---------------------------------------------------------------
      // Simply set newImage to this since there are no new version.
      // ---------------------------------------------------------------
      newImage = this;
    }
    else
    {
      // ---------------------------------------------------------------
      // This is called from the subclass implementation of the same
      // function. The subclass should have called reallocateImage() if
      // imageSize_ was different from class size. Therefore, imageSize_
      // should be the same as the most recent version class size now.
      // ---------------------------------------------------------------
      assert(newImage->imageSize_ == getClassSize());
    }

    return 0;
  }

// ---------------------------------------------------------------------
// Driver for Packing
//
// Should return a 64 bit integer on a 64 bit platform. Could be fixed
// later when 64-bit platforms are really available since it doesn't
// affect object layout.
// ---------------------------------------------------------------------
Long NAVersionedObject::drivePack(void *space, short isSpacePtr)
  {
    // -----------------------------------------------------------------
    // If the object has already been packed, just convert the pointer
    // of the object to an offset and return its value. That value will
    // be used by the caller to replace the pointer stored there.
    // -----------------------------------------------------------------
    if (isPacked())
    {
      if (isSpacePtr)
        return ((Space *)space)->convertToOffset((char *)this);
      else
        return ((char *)space - (char *)this);
    }

    // -----------------------------------------------------------------
    // Make sure the image size and the version ID are set properly
    // before proceeding on to pack the object.
    // -----------------------------------------------------------------
    Int16 classSize = getClassSize();
    if ((classSize % 8) != 0)
      assert((classSize % 8) == 0);
    setImageSize(classSize);
    populateImageVersionIDArray();

    // -----------------------------------------------------------------
    // Toggle the Endianness of the Version Header if it's not stored
    // in little-endian form.
    //
    // *** DON'T DO THIS JUST YET: PLAN IS TO SUPPORT THIS ONLY FROM
    // *** SECOND RELEASE.
    // -----------------------------------------------------------------
#ifndef NA_LITTLE_ENDIAN
    // toggleEndiannessOfVersionHeader();
#endif

    // -----------------------------------------------------------------
    // Convert members of this object from local platform to reference
    // platform.
    //
    // *** DON'T DO THIS JUST YET: PLAN IS TO SUPPORT THIS ONLY FROM
    // *** SECOND RELEASE.
    // -----------------------------------------------------------------
    // convertToReferencePlatform();

    // -----------------------------------------------------------------
    // Mark object as packed, despite it is not completely packed yet.
    // It is needed because the call that follows to the virtual method
    // pack() drives the packing of all objects referenced by this
    // object. If this object is subsequently referenced by another
    // object down the row, drivePack() will be called on this object
    // again. At that point of time, we should see the packed flag set
    // so that "double-packing" can be avoided.
    // -----------------------------------------------------------------
    markAsPacked();

    // -----------------------------------------------------------------
    // pack() is a virtual method the subclass should redefine to drive
    // the packing of all objects it references by pointers and convert
    // those pointers to offsets. It should also convert the endianness
    // of its members to the reference if necessary.
    // -----------------------------------------------------------------
    setIsSpacePtr( isSpacePtr !=0 );
    Long offset = pack(space);

    //    long offset = (isSpacePtr ? pack(space) : pack(space,0));

    // -----------------------------------------------------------------
    // Make sure the eyeCatcher_ field of the object is proper. Also
    // clean up the virtual table function pointer, so that the image
    // look identical each time.
    // -----------------------------------------------------------------
    str_cpy_all(eyeCatcher_,VOBJ_EYE_CATCHER,VOBJ_EYE_CATCHER_SIZE);
    setVTblPtr(NULL);

    return offset;
  }

// ---------------------------------------------------------------------
// Driver for Unpacking
//
// In a nutshell, unpacking consists of the following major steps:
//
// 1. fix the endianness of the version header (members of this class)
// 2. fix up the virtual table function pointer for the object
// 3. migrate an object of a previous version to the current version
// 4. fix the endianness of all other members in the subclasses
// 5. convert pointers in this object from offsets back to addresses
// 5. initiate unpacking for other objects referenced by this object
// 6. initialize new members added in the new version
//
// Parameters:
// base             is the base address (added to offset to get pointer)
// vtblPtr          (first form) is the pointer to the virtual function table
//                  of the class
// ptrToAnchorClass (second form, see below) is a pointer to an object
//                  that has the desired virtual function pointer
// ---------------------------------------------------------------------
NAVersionedObject *NAVersionedObject::driveUnpack(
                                    void *base,
				    char *vtblPtr,
				    void * reallocator)
  {
    // -----------------------------------------------------------------
    // Make sure we are really dealing with a NAVersionedObject by
    // examining its eyeCatcher_.
    // -----------------------------------------------------------------
    if (str_cmp(eyeCatcher_,VOBJ_EYE_CATCHER,VOBJ_EYE_CATCHER_SIZE))
      return NULL;

    // -----------------------------------------------------------------
    // If object has already been unpacked, just return either its own
    // address or the reallocated address if the object was reallocated
    // to somewhere else.
    // -----------------------------------------------------------------
    if (!isPacked())
    {
      if (reallocatedAddress_.isNull())
        return this;
      else
        return reallocatedAddress_.getPointer();
    }
    else
      reallocatedAddress_ = (NAVersionedObjectPtr) NULL ;

    // -----------------------------------------------------------------
    // Fix the Version Header to the endianness of the local platform
    // if necessary. Correct classID_ field is necessary to determine
    // the right subclass the object belongs.
    //
    // *** DON'T DO THIS JUST YET: PLAN IS TO SUPPORT THIS ONLY FROM
    // *** SECOND RELEASE.
    // -----------------------------------------------------------------
#ifndef NA_LITTLE_ENDIAN
    // toggleEndiannessOfVersionHeader();
#endif

    // -----------------------------------------------------------------
    // The method findVTblPtr() was called on an object of the Anthor
    // Class T to find out what subclass of the Anchor Class this object
    // belongs based on its classID_. Now, the virtual function table
    // pointer is used to fix up this object.
    // -----------------------------------------------------------------
    if (vtblPtr == NULL)
      return NULL;                                // unknown class ID //
    else
      setVTblPtr(vtblPtr);

    // -----------------------------------------------------------------
    // Call the virtual method migrateToNewVersion() so that older
    // objects can be migrated to fit in the new class template.
    // -----------------------------------------------------------------
    NAVersionedObject *objPtr = NULL;

    // -----------------------------------------------------------------
    // migrateToNewVersion() should either set objPtr to this or to an
    // reallocated image.
    // -----------------------------------------------------------------
    if (migrateToNewVersion(objPtr))
      return NULL;                           // version not supported //

    // -----------------------------------------------------------------
    // Convert members of this object from reference platform to local
    // platform.
    //
    // *** DON'T DO THIS JUST YET: PLAN IS TO SUPPORT THIS ONLY FROM
    // *** SECOND RELEASE.
    // -----------------------------------------------------------------
    // objPtr->convertToLocalPlatform();

    // -----------------------------------------------------------------
    // Mark object as not packed, despite it is not completely unpacked
    // yet. This is needed because the call that follows to the virtual
    // method unpack() drives the unpacking of all objects referenced by
    // this object. If this object is subsequently referenced by another
    // object down the row, driveUnpack() will be called on this object
    // again. At that point of time, we should see the packed flag set
    // to "not packed" so that "double-unpacking" can be avoided.
    // -----------------------------------------------------------------
    markAsNotPacked();
    objPtr->markAsNotPacked();

    // -----------------------------------------------------------------
    // After migration, the object might have been reallocated. In that
    // case objPtr will be changed to point to the reallocated object.
    // The following calls are then made on the reallocated object
    // instead of this.
    // -----------------------------------------------------------------
    if(objPtr->unpack(base, reallocator))
      return NULL;              // Should this ever happen? Internal Error.

    objPtr->initNewMembers();
    return objPtr;
  }

NAVersionedObject *NAVersionedObject::driveUnpack(
                                    void *base,
				    NAVersionedObject *ptrToAnchorClass,
				    void * reallocator)
{
  // this method uses more stack space, due to the inline method
  // findVTblPtr
  char *vtblPtr = ptrToAnchorClass->findVTblPtr(classID_);
  return driveUnpack(base,vtblPtr,reallocator);
}

// ------------------------------------------------------------ EOF ----

