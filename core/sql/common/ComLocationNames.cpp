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
 * File:         ComLocationNames.C
 * Description:  methods for classes associating with location names
 *
 *               
 * Created:      10/19/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "Platform.h"
  #include "cextdecs/cextdecs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "BaseTypes.h"
#include "Collections.h"
#include "ComASSERT.h"
#include "ComLocationNames.h"
#include "ComRegAPI.h"
#include "ComSqlText.h"
#include "NAMemory.h"
#include "SQLCLIdev.h"
#include "ComDistribution.h"

// -----------------------------------------------------------------------
// File-scope helper functions
// -----------------------------------------------------------------------

char
ComGetGuardianLocationNamePartSeparator()
{
  return ComSqlText.getPeriod();
}


// -----------------------------------------------------------------------
// methods for class ComLocationName
// -----------------------------------------------------------------------

//
// constructors
//

ComLocationName::ComLocationName()
  : inputFormat_(INPUT_NOT_SPECIFIED)
  , isExpanded_(FALSE)
{
}

ComLocationName::ComLocationName(const char * locationName)
  : isExpanded_(FALSE)
{
  //
  // remaining data members will be initialized by copy()
  //
  copy(locationName, GUARDIAN_LOCATION_NAME_FORMAT);
  if (NOT this->isValid())
  {
    clear();
    copy(locationName, OSS_LOCATION_NAME_FORMAT);
  }
}

ComLocationName::ComLocationName(const char * locationName,
                                 locationNameFormat locNameFormat)
  : isExpanded_(FALSE)
{
  //
  // remaining data members will be initialized by copy()
  //
  copy(locationName, locNameFormat);
}

//
// copy constructor
//

ComLocationName::ComLocationName(const ComLocationName & locationName)
: inputFormat_(locationName.inputFormat_)
, isExpanded_(locationName.isExpanded_)
, systemNamePart_(locationName.systemNamePart_)
, volumeNamePart_(locationName.volumeNamePart_)
, subvolumeNamePart_(locationName.subvolumeNamePart_)
, fileNamePart_(locationName.fileNamePart_)
{
}

// constructor using metadata format
ComLocationName::ComLocationName( const ComNodeName & nodeName
                                , const ComVolumeName & volumeName
                                , const char * fileSuffix)
: inputFormat_ (NODE_VOLUME_SUBVOLUME_FILE_INPUT_FORMAT)
, isExpanded_ (FALSE)
, systemNamePart_(nodeName)
, volumeNamePart_(volumeName)
{
  // Make room for 2 name parts, a period and a null-terminator
  char copyOfFileSuffix[2*(ComGUARDIAN_SYSTEM_NAME_PART_CHAR_MAX_LEN+1)];
  size_t suffixLength;

  suffixLength = strlen(fileSuffix);
  // Don't accept an invalid suffix length
  ComASSERT (suffixLength < (2*(ComGUARDIAN_SYSTEM_NAME_PART_CHAR_MAX_LEN+1)));
  memcpy (copyOfFileSuffix, fileSuffix, suffixLength+1);

  char * dot = strchr (copyOfFileSuffix, '.');
  ComASSERT(dot);   // Don't accept an invalid format suffix

  *(dot++) = 0;
  subvolumeNamePart_ = copyOfFileSuffix;
  fileNamePart_ = dot;

}
//
// virtual destructor
//

ComLocationName::~ComLocationName()
{
}

//
// assignment operators
//

ComLocationName &
ComLocationName::operator=(const ComLocationName & rhs)
{
  if (this EQU &rhs)
    return *this;

  systemNamePart_       = rhs.systemNamePart_;
  volumeNamePart_       = rhs.volumeNamePart_;
  subvolumeNamePart_    = rhs.subvolumeNamePart_;
  fileNamePart_         = rhs.fileNamePart_;
  inputFormat_          = rhs.inputFormat_;
  isExpanded_           = rhs.isExpanded_;

  return *this;
}

ComLocationName &
ComLocationName::operator=(const char * rhs)
{
  clear();
  copy(rhs, GUARDIAN_LOCATION_NAME_FORMAT);
  if (NOT this->isValid())
  {
    clear();
    copy(rhs, OSS_LOCATION_NAME_FORMAT);
  }

  return *this;
}

//
// accessors
//

NAString
ComLocationName::getGuardianFullyQualifiedName(void) const
{
  //
  // This method uses the conceptual constness approach
  //
  if (NOT isValid())
  {
    return NAString();
  }

  if (NOT isExpanded())
  {
    ((ComLocationName *)this)->expand();
    if (NOT isExpanded())  // an error occurred during the expansion
      return NAString();
  }

  NAString locName;
  locName += systemNamePart_;
  ComASSERT(NOT locName.isNull());
  locName += ComGetGuardianLocationNamePartSeparator();
  ComASSERT(NOT volumeNamePart_.isNull());
  locName += volumeNamePart_;

  if (NOT fileNamePart_.isNull())
  {
    ComASSERT(NOT subvolumeNamePart_.isNull());
    locName += ComGetGuardianLocationNamePartSeparator();
    locName += subvolumeNamePart_;
    locName += ComGetGuardianLocationNamePartSeparator();
    locName += fileNamePart_;
  }

  return locName;
}

NAString
ComLocationName::getInputFormatAsNAString() const
{
  NAString inFormat;
  switch (getInputFormat())
  {
  case UNKNOWN_INPUT_FORMAT:
    inFormat = "UNKNOWN_INPUT_FORMAT";
    break;
  case INPUT_NOT_SPECIFIED:
    inFormat = "INPUT_NOT_SPECIFIED";
    break;
  case VOLUME_INPUT_FORMAT:
    inFormat = "VOLUME_INPUT_FORMAT";
    break;
  case NODE_VOLUME_INPUT_FORMAT:
    inFormat = "NODE_VOLUME_INPUT_FORMAT";
    break;
  case VOLUME_SUBVOLUME_FILE_INPUT_FORMAT:
    inFormat = "VOLUME_SUBVOLUME_FILE_INPUT_FORMAT";
    break;
  case NODE_VOLUME_SUBVOLUME_FILE_INPUT_FORMAT:
    inFormat = "NODE_VOLUME_SUBVOLUME_FILE_INPUT_FORMAT";
    break;
  default:
    ABORT("internal logic error");
    break;
  };

  return inFormat;
}


NAString
ComLocationName::getFileSuffixPart() const
{
  return NAString (subvolumeNamePart_) + NAString(ComGetGuardianLocationNamePartSeparator()) + fileNamePart_;
}

//
// mutators
//

void
ComLocationName::clear()
{
  isExpanded_                           = FALSE;
  inputFormat_                          = INPUT_NOT_SPECIFIED;
  systemNamePart_.clear();
  volumeNamePart_.clear();
  subvolumeNamePart_ .clear();
  fileNamePart_.clear();
}

void
ComLocationName::copy(const char * locName,
                      locationNameFormat locNameFormat)
{
  switch (locNameFormat)
  {
  case GUARDIAN_LOCATION_NAME_FORMAT :
    copyGuardianLocationName(locName);
    break;
  case OSS_LOCATION_NAME_FORMAT :
    copyOssLocationName(locName);
    break;
  default :
    ComASSERT(FALSE);
    break;
  }
}

void
ComLocationName::copyGuardianLocationName(const char * guardianLocName)
{
  clear();
  inputFormat_ = UNKNOWN_INPUT_FORMAT;
  //
  // Note that guardianLocName may or may not contain the system
  // name part.  For more information about the proper syntax of
  // the input location, please read the comments associating
  // with the initialize constructor in the header file.
  //

  if (guardianLocName[0] == 0)
  {
    // Empty input name.
    // Note that the object is still empty.
    // Sets inputFormat_ to INPUT_NOT_SPECIFIED to
    // indicate that the object is valid.
    //
    inputFormat_ = INPUT_NOT_SPECIFIED;
    ComASSERT(isEmpty() AND isValid());
    return;
  }

  // Interpret the guardian location name
  if (ComInterpretGuardianFileName ( guardianLocName
                                   , systemNamePart_
                                   , volumeNamePart_
                                   , subvolumeNamePart_
                                   , fileNamePart_
                                   ))
  {
    //
    // The input location name is valid.
    // Figure out the input format
    if (systemNamePart_.isNull())
    {
      // no node name, either $<vol> or $<vol>.<svol>.<file>
      if (subvolumeNamePart_.isNull())
        inputFormat_ = VOLUME_INPUT_FORMAT;
      else
        inputFormat_ = VOLUME_SUBVOLUME_FILE_INPUT_FORMAT;
    }
    else
    {
      // Have a node name, either \<node>.$<vol> or fully qualified name
      if (subvolumeNamePart_.isNull())
        inputFormat_ = NODE_VOLUME_INPUT_FORMAT;
      else
        inputFormat_ = NODE_VOLUME_SUBVOLUME_FILE_INPUT_FORMAT;
    }
  }

} // ComLocationName::copyGuardianLocationName()

void
ComLocationName::copyOssLocationName(const char * ossLocationName)
{
  clear();
  inputFormat_ = UNKNOWN_INPUT_FORMAT;

  //
  // Note that ossLocationName may or may not contain the system
  // name part.  For more information about the proper syntax of
  // the input location, please read the comments associating
  // with the initialize constructor in the corresponding  header
  // file.
  //

  if (ossLocationName[0] == 0)
  {
    //
    // Empty input name.
    // Note that the object is still empty.  The data member
    // inputFormat_ is set to INPUT_NOT_SPECIFIED.
    //
    inputFormat_ = INPUT_NOT_SPECIFIED;
    ComASSERT(isEmpty() AND isValid());
    return;
  }

  // OSS Path names are no longer supported


} // ComLocationName::copyOssLocationName()

NABoolean
ComLocationName::expand(void)
{
  ComNodeName defaultNodeName;
  defaultNodeName.setDefault();
  ComVolumeName defaultVolumeName;
  defaultVolumeName = "$SYSTEM"; 

  // Expand the object, using the current defaults 
  return expand( defaultNodeName, defaultVolumeName);
                  
} // ComLocationName::expand(void)

NABoolean
ComLocationName::expand(const ComNodeName & defaultSystemName,
                        const ComVolumeName & defaultVolumeName)
{
  // Expand the object, using specified defaults
  if (isExpanded())
    //
    // Already expanded by previous invocation of expand().
    // Ignores the input parameters, and does nothing.
    // 
    return TRUE;

  if (inputFormat_ EQU UNKNOWN_INPUT_FORMAT)
  {
      // The object is not valid, return.
      ComASSERT(NOT isValid());
      ComASSERT(NOT isExpanded());
      return FALSE;
  }

  if (!defaultSystemName.isValid() || !defaultVolumeName.isValid())
    // The input is not valid, return.
    return FALSE;

  if (systemNamePart_.isNull())
    systemNamePart_ = defaultSystemName;

  if (volumeNamePart_.isNull())
    volumeNamePart_ = defaultVolumeName;

  isExpanded_ = TRUE;
  return TRUE;
} // ComLocationName::expand()

//
// End of File
//
