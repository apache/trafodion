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
#ifndef COMLOCATIONNAMES_H
#define COMLOCATIONNAMES_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ComLocationNames.h
 * Description:  classes representing location names
 *
 * Created:      10/20/95
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "ComOperators.h"
#include "NABoolean.h"
#include "NAString.h"
#include "NABasicObject.h"
#include "ComDiags.h"
#include "ComGuardianFileNameParts.h"
#include "ComSizeDefs.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ComLocationName;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Definitions relating to location names
// -----------------------------------------------------------------------

const Int32 ComGUARDIAN_NAME_PART_WITHOUT_PREFIX = 0,
          COM_GUARDIAN_NAME_PART_WITHOUT_PREFIX =
                ComGUARDIAN_NAME_PART_WITHOUT_PREFIX;

// Various sizes - eventually, those that redefine sizes from ComSizeDefs
// should be substituted with those definitions wherever used.

//
// maximum length allowed for Guardian system names (including
// the backslash prefix)
//
const unsigned short ComGUARDIAN_SYSTEM_NAME_PART_CHAR_MAX_LEN = ComMAX_GUARDIAN_NAME_PART_LEN;

//
// maximum length allowed for Guardian volume names (including
// the dollar sign prefix)
//
const unsigned short ComGUARDIAN_VOLUME_NAME_PART_CHAR_MAX_LEN = ComMAX_GUARDIAN_NAME_PART_LEN;

// -----------------------------------------------------------------------
// File-scope helper functions
// -----------------------------------------------------------------------
//
char
ComGetGuardianLocationNamePartSeparator();

        // returns the separator separating the system name
        // part and volume name part of a fully qualified
        // location name in Guardian format.

// -----------------------------------------------------------------------
// definition of class ComLocationName
// -----------------------------------------------------------------------

class ComLocationName
{
public:

  enum locationNameFormat { UNKNOWN_LOCATION_NAME_FORMAT
                          , GUARDIAN_LOCATION_NAME_FORMAT
                          , OSS_LOCATION_NAME_FORMAT
                          };

  enum inputFormat { UNKNOWN_INPUT_FORMAT
                   , INPUT_NOT_SPECIFIED
                   , VOLUME_INPUT_FORMAT
                   , NODE_VOLUME_INPUT_FORMAT
                   , VOLUME_SUBVOLUME_FILE_INPUT_FORMAT
                   , NODE_VOLUME_SUBVOLUME_FILE_INPUT_FORMAT
                   };

  //
  // constructors
  //

  ComLocationName();

        // Default constructor.  The object is considered
        // as empty and valid until the method expand() is
        // invoked and returns a successful condition.
        // After expand() is invoked and returns a successful
        // condition the object is no longer empty.

  ComLocationName(const char * locationName);

        // Please read the comments associating with the
        // following initializing contructors for information
        // about this initializing constructor.  The two
        // constructors are almost the same.  The difference
        // is that this contructor accepts locationName in
        // either the NSK Guardian or NSK OSS format.

  ComLocationName(const char * locationName,
                  locationNameFormat locNameFormat);

        // If locNameFormat contains OSS_LOCATION_NAME_FORMAT,
        // the input location name is considered as valid
        // if it is either empty or a valid OSS path name
        // with one of the following formats:
        //
        //      /G/volume
        //      /E/system/G/volume
        //      /G/volume/subvol/file00
        //      /E/system/G/volume/subvol/file00
        //
        // where
        //
        // system is a Guardian system name part without
        //      the backslash prefix.  Currently, system
        //      can only be the case-insensitive string
        //      "NSK" (without quotes).
        // volume is a Guardian volume name part without
        //      the dollar sign prefix
        // subvol is the Guardian subvolume name part
        // file00 is the Guardian file name part with the
        //        "00" (two zero digits, not including
        //        quotes) suffix.
        //
        // If locNameFormat contains GUARDIAN_LOCATION_NAME_FORMAT,
        // the input location name is considered as valid
        // if it is empty, a valid Guardian volume name, or
        // a valid Guardian file name with the "00" (two zero
        // digits, not including quotes) suffix.  For example,
        // 
        //      $volume
        //      \system.$volume
        //      $volume.subvol.file00
        //      \system.$volume.subvol.file00
        //
        // If the input location name is empty, the behavior
        // of this contructor is the same as that of the
        // default constructor.
        // 
        // If the input location name is valid, copies the
        // name to this object; otherwise, the object is
        // empty and considered as being invalid.  Note that
        // leading white spaces in the input location name
        // is not allowed.
        //
        // If the system name part is not specified, the
        // object will not contain any system name part
        // until the method expand() is invoked and returns
        // a successful condition.
        //
        // If the subvolume and file name parts do not
        // appear, the object will not contain any subvolume
        // and file name parts.  The invocation of the method
        // expand() does not affect the subvolume and file
        // name parts in this object.

  ComLocationName(const ComLocationName & locationName);

        // copy constructor

  ComLocationName( const ComNodeName & nodeName
                 , const ComVolumeName & volumeName
                 , const char * fileSuffix);

        // Constructor using metadata representation 

  //
  // virtual destructor
  //

  virtual ~ComLocationName();

  //
  // assignment operators
  //

  ComLocationName & operator=(const ComLocationName & rhs);

  ComLocationName & operator=(const char * rhsLocName);

        // The specified location name on the right-hand side
        // must be a valid location name; otherwise, this
        // object (on the left-hand side) will be cleared and
        // considered as empty and invalid.
        //
        // For information about finding out whether the specified
        // location name is valid or not, please read the
        // comments associating with the initialize constructor.
        // Note that rhsLocName can contain the location name in
        // either NSK Guardian format or NSK OSS format.

  //
  // accessors
  //

  NAString getGuardianFullyQualifiedName(void) const;

        // returns the resolved, fully-qualified location name in
        // Guardian format.  If the object has not yet fully-expanded,
        // this method invokes the method expand() first
        // (before returning the expanded location name).  If the object
        // is invalid or empty, returns an empty string.
        //
        // The subvolume and file name parts will not be included in
        // the fully expanded name unless they appeared in the input
        // location name (passed to an initialize constructor).

  inline const ComNodeName & getGuardianSystemNamePart() const;

        // returns the system name part, in Guardian format,
        // of the location name.  If the system name part
        // is not specified and the method expand() has not
        // been invoked, returns an empty string.  If the
        // sytem name part is not specified but the method
        // expand() has been invoked, returns the default
        // system name.  If the input location name (passed
        // to an initialize constructor) was invalid, returns
        // an empty name.

  inline const ComVolumeName & getGuardianVolumeNamePart() const;

        // returns the volume name part, in Guardian format,
        // of the location name.  If the default constructor
        // was invoked (that is, no input OSS location name
        // was specified), returns an empty string.  If the
        // input location name (passed to an initialize
        // constructor) was invalid, returns an empty name.

  inline const ComSubvolumeName & getSubvolumeNamePart() const;

        // returns the subvolume name part of the input
        // location name (passed to an initialize constructor).
        // If the subvolume name part is not specified or the
        // input location name is invalid, returns an empty
        // name.

  inline const ComFileName & getFileNamePart() const;

        // returns the file name part of the input location
        // name (passed to the initialize constructor).  If the
        // file name part was not specified, returns an empty
        // string.  If the input location name (passed to an
        // initialize constructor) is invalid, returns an empty
        // name.

  NAString getFileSuffixPart() const;

        // the MetaData stores the subvolume name and file
        // name part in a single field.  This method returns
        // this information in this form.

  inline inputFormat getInputFormat() const;

        // returns the format of the input OSS path name.
        // If the default constructor was invoked, returns
        // INPUT_NOT_SPECIFIED.  If the format of the input
        // location name is not one of the allowed format,
        // returns UNKNOWN_INPUT_FORMAT.

  NAString getInputFormatAsNAString() const;

        // Same as getInputFormat() except that this method
        // returns a string instead of an enumerated constant.
        // This method is used for tracing purposes.

  inline const char * getSystemNamePartWithoutBackslashPrefix() const;

        // returns the system name part, in Guardian format
        // without the backslash prefix, of the location
        // name.  For more information, please read the comments
        // associating with the method getGuardianSystemNamePart().

  inline const char * getVolumeNamePartWithoutDollarPrefix() const;

        // returns the volume name part, in Guardian format
        // without the dollar sign prefix, of the location
        // name.  For more information, please read the comments
        // associating with the method getGuardianVolumeNamePart().

  inline NABoolean isEmpty() const;

        // returns TRUE if this object is empty; returns FALSE
        // otherwise.  This object can only be empty when
        // (1) the specified (input) location name is invalid or
        // (2) the object was instantiated by the invocation of the
        // default constructor and the method expand() has not been
        // invoked (or it has been invoked but returned an
        // unsuccessful condition).

  inline NABoolean isExpanded() const;

        // returns TRUE if the object has been expanded; returns
        // FALSE otherwise.  The object is expanded when the method
        // expand() is invoked and returns successful condition.
        // For more information, please refer the comments describing
        // the method expand().

  inline NABoolean isValid() const;

        // returns TRUE if (1) the object contains a valid location or
        // (2) the object was instantiated by the default constructor
        // and the method expand() has not been invoked (or it has been
        // invoked but returned an unsuccessful condition).
        // 
        // Returns FALSE otherwise.

  //
  // mutators
  //

  void copy(const char * locName, locationNameFormat locNameFormat);

        // assigns the location name contained in locName to the
        // object.  The format of the input location name can be
        // either NSK OSS format or NSK Guardian format
        // (locNameFormat contains OSS_LOCATION_NAME_FORMAT or
        // GUARDIAN_LOCATION_NAME_FORMAT, respectively).

  NABoolean expand(void);

        // If the object is invalid, does nothing and returns FALSE;
        // otherwise, makes sure that the object contains system name
        // part and volume name part.  If they were not specified,
        // uses the default values.  If the operation was successful,
        // sets the isExpanded() bit in the object to TRUE and
        // return TRUE.
        //
        // If the object does not contain the volume name part and
        // this method cannot find a default volume, it inserts an
        // error message to diagsArea, does not change the content
        // of the isExpanded() bit (in the object) and returns FALSE.
  
  NABoolean expand(const ComNodeName & defaultSystemName,
                   const ComVolumeName & defaultVolumeName);

        // returns FALSE and does nothing if the object is
        // invalid.  Please note that the method isExpanded()
        // returns FALSE when expand() has not yet been invoked
        // or when expand() has been invoked but has returned
        // FALSE.
        //
        // Otherwise, expands the location name (using the input
        // default value(s) if needed) and then returns TRUE
        // this is the first time expand() is invoked.  After
        // expand() is invoked and returns TRUE, subsequent
        // invocations of expand() will not alter the contents of
        // the object, and the input parameters will be ignored
        // by expand(); expand() will continue to return TRUE
        // however.  Please note that after expand() is invoked
        // and returns TRUE, the method isExpanded() will return
        // TRUE when invoked.
        //
        // Please note that the letters in the input system and
        // volume names can be in any cases (either upper- or
        // lower-case).  expand() will convert all letters in
        // the specified default name parts to upper-case letters
        // before using them.  

private:

  //
  // methods
  //

  void clear();
  void copyOssLocationName(const char * ossLocName);
  void copyGuardianLocationName(const char * guardianLocName);

  //
  // data members
  //

  NABoolean            isExpanded_;
  inputFormat          inputFormat_;
  ComNodeName          systemNamePart_;
  ComVolumeName        volumeNamePart_;
  ComSubvolumeName     subvolumeNamePart_;
  ComFileName          fileNamePart_;
  
}; // class ComLocationName

// -----------------------------------------------------------------------
// definitions of inline methods for class ComLocationName
// -----------------------------------------------------------------------

//
// accessors
//
inline const ComNodeName &
ComLocationName::getGuardianSystemNamePart() const
{
  return systemNamePart_;
}

inline const ComVolumeName &
ComLocationName::getGuardianVolumeNamePart() const
{
  return volumeNamePart_;
}

inline const ComSubvolumeName &
ComLocationName::getSubvolumeNamePart() const
{
  return subvolumeNamePart_;
}

inline const ComFileName &
ComLocationName::getFileNamePart() const
{
  return fileNamePart_;
}

inline ComLocationName::inputFormat
ComLocationName::getInputFormat() const
{
  return inputFormat_;
}

inline const char *
ComLocationName::getSystemNamePartWithoutBackslashPrefix() const
{
  return systemNamePart_.getNodeNameWithoutBackslash();
}

inline const char *
ComLocationName::getVolumeNamePartWithoutDollarPrefix() const
{
  return volumeNamePart_.getVolumeNameWithoutDollar();
}

inline NABoolean
ComLocationName::isEmpty() const
{
  //
  // Please note that every input location name passed to the
  // initializing constructor always includes the volume name
  // part.  Therefore, the volumeNamePart_
  // data member is used to help determining whether the object
  // is empty or not.
  // 
  return volumeNamePart_.isNull();
}

inline NABoolean
ComLocationName::isExpanded() const
{
  return isExpanded_;
}

inline NABoolean
ComLocationName::isValid() const
{
  return (inputFormat_ NEQ UNKNOWN_INPUT_FORMAT);
}

#endif // COMLOCATIONNAMES_H
