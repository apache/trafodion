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
#ifndef ELEMDDLLOCATION_H
#define ELEMDDLLOCATION_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLLocation.h
 * Description:  class for information about location (in Location
 *               clauses) in DDL statements
 *
 *               
 * Created:      4/6/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ComLocationNames.h"
#include "ElemDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLLocation;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// Location element in DDL statement.
// -----------------------------------------------------------------------
class ElemDDLLocation : public ElemDDLNode
{

public:

  enum locationNameTypeEnum { LOCATION_DEFAULT_NAME_TYPE,
                              LOCATION_OSS_NAME = LOCATION_DEFAULT_NAME_TYPE,
                              LOCATION_GUARDIAN_NAME,
                              LOCATION_ENVIRONMENT_VARIABLE };

  // enum values below needs to be multiple of 2.
  enum {
    INVALIDSUBVOLNAME = 0x0001,
    INVALIDFILENAME = 0x0002
  };

  // constructor
  ElemDDLLocation(locationNameTypeEnum locationNameType,
                  const NAString & aLocationName);

  // virtual destructor
  virtual ~ElemDDLLocation();

  // casting
  virtual ElemDDLLocation * castToElemDDLLocation();

  //
  // accessors
  //

  inline locationNameTypeEnum getDefaultLocationNameType() const;

  inline const NAString & getLocation() const;

        // Same as method getLocationName.  This method is kept
        // so existing code that calls this methods continues
        // to work.

  inline const NAString & getLocationName() const;

        // returns the location name appears in the LOCATION
        // clause/phrase.

  inline const NAString & getPartitionName()const;
        // returns the partition name in the LOCATION clause

  inline ComLocationName::inputFormat getLocationNameInputFormat() const;

        // returns ComLocationName::UNKNOWN_INPUT_FORMAT if the
        // location name appears in the specified LOCATION is not one
        // of the recognized location name formats; otherwise, returns
        // an enumerated constant of type ComLocationName::inputFormat
        // to described the format of the specified location name.
  
  inline locationNameTypeEnum getLocationNameType() const;

        // returns the type of the location name (e.g., an OSS
        // path name, a Guardian device name, an OSS environment
        // variable name, etc.)
        //
        // Currently, the LOCATION clause only accepts an OSS
        // path name.

  NAString getLocationNameTypeAsNAString() const;

        // Same as getLocationNameType() except that this method
        // returns a string instead of an enumerated constant.
        // This method is used for tracing purposes.

  //
  // methods for tracing
  //
  
  virtual const NAString getText() const;
  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual const NAString displayLabel3() const;

  // method for building text
  virtual NAString getSyntax() const;

  //Method to get partition name
  inline void setPartitionName(const NAString & partitionName);

  // Methods to check flags_ field. These are called from parser
  inline NABoolean isSubvolumeNameValid();
  inline NABoolean isFilenameValid();

private:

  ComLocationName::inputFormat locationNameInputFormat_;
  locationNameTypeEnum locationNameType_;
  unsigned short flags_;
  //
  // The syntax of Guardian location name is
  //   [ \system . ] $volume [ . subvol [ . file ] ]
  // (without embedded white spaces)
  //
  // The syntax of the input OSS location name is
  //   { /G/volume                      }
  //   { /E/system/G/volume             }
  //   { /G/volume/subvol/file          }
  //   { /E/system/G/volume/subvol/file }
  //
  // The syntax of input OSS environment variable name is
  //   an_environment_variable_name
  // This syntax is currently not supported by the parser.
  //      
  NAString locationName_;
  NAString partitionName_;
  
}; // class ElemDDLLocation

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLLocation
// -----------------------------------------------------------------------

//
// accessors
//

inline ElemDDLLocation::locationNameTypeEnum
ElemDDLLocation::getDefaultLocationNameType() const
{
  return LOCATION_DEFAULT_NAME_TYPE;
}

inline const NAString &
ElemDDLLocation::getLocation() const
{
  return locationName_;
}

inline const NAString &
ElemDDLLocation::getLocationName() const
{
  return locationName_;
}

inline const NAString &
ElemDDLLocation::getPartitionName() const
{
  return partitionName_;
}

inline ComLocationName::inputFormat
ElemDDLLocation::getLocationNameInputFormat() const
{
  return locationNameInputFormat_;
}

inline ElemDDLLocation::locationNameTypeEnum
ElemDDLLocation::getLocationNameType() const
{
  return locationNameType_;
}

inline void ElemDDLLocation::setPartitionName(const NAString & partitionName)
{
	partitionName_ = partitionName;
}

inline NABoolean ElemDDLLocation::isSubvolumeNameValid()
{
  return (flags_ & INVALIDSUBVOLNAME) ? FALSE : TRUE;
}

inline NABoolean ElemDDLLocation::isFilenameValid()
{
  return (flags_ & INVALIDFILENAME) ? FALSE : TRUE;
}

#endif // ELEMDDLLOCATION_H
