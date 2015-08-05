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

#include "ComCextdecs.h"
#include "ComGuardianFileNameParts.h"

#include "ComASSERT.h"

  #ifndef __DERROR__
    #include "fs/feerrors.h"
  #endif //__DERROR

#include "str.h"
//-------------------------------------------------------------------
//
//  Helper functions
//
//-------------------------------------------------------------------

// Only know about one node on NT: \NSK, with node number zero.
// Also, support local (-1) and default (-2) node (both of which are \NSK).
__int64 ComGetNodeNameAsInt64 (const Int32 nodeNumber)
{
  ComASSERT (nodeNumber <= 0); 
  __int64 nodeNameAsInt64 = 0;
  strcpy ((char *)&nodeNameAsInt64, "\\NSK");
  return nodeNameAsInt64;
}

// Separate an (assumed) Guardian file name into its separate parts.
// Returns TRUE if everything is OK, FALSE otherwise
// The following formats are considered legal
//    $<vol>
//    \<node>.$<vol>
//    $<vol>.<svol>.<file>
//    \<node>.$<vol>.<svol>.<file>
NABoolean ComInterpretGuardianFileName ( const char * fileName
                                       , ComNodeName & nodeNamePart
                                       , ComVolumeName & volumeNamePart
                                       , ComSubvolumeName & subvolNamePart
                                       , ComFileName & fileNamePart
                                       )
{
  char copyOfFileName [ComFULLY_QUALIFIED_FILE_NAME_MAX_LEN+1];
  char * ptr = copyOfFileName;
  char * dot;

  // Clear output names
  nodeNamePart.clear();
  volumeNamePart.clear();
  subvolNamePart.clear();
  fileNamePart.clear();

  size_t length = strlen (fileName);
  if (length > ComFULLY_QUALIFIED_FILE_NAME_MAX_LEN)
    // name too long, quit
    return FALSE;

  memcpy (copyOfFileName, fileName, length+1);
  dot = (char *)strchr (copyOfFileName, '.');

  if (dot)
  {
    // At least two parts to the name. First part could be node name.

    if (*ptr == '\\')
    {
      // Null-terminate node-name part
      *dot = 0;
      nodeNamePart = ptr;
      if (!nodeNamePart.isValid())
      {
        // Invalid node name, quit.
        nodeNamePart.clear();
        return FALSE;
      }

      ptr = ++dot;
      dot = (char *)strchr (ptr, '.');
    }

  }

  if (*ptr != '$')
    // must have a volume name - quit if we don't
    return FALSE;

  if (dot)
    // Null-terminate volume-name part, if required
    *dot = 0;

  // Got a volume name part
  volumeNamePart = ptr;
  if (!volumeNamePart.isValid())
  {
    // Invalid volume name, quit.
    nodeNamePart.clear();
    volumeNamePart.clear();
    return FALSE;
  }

  if (!dot)
    // A valid $<vol> or \<node>.$<vol> type of name
    return TRUE;

  ptr = ++dot;
  dot = (char *)strchr (ptr, '.');

  if (!dot)
  {
    // Invalid file name, we require one more dot. Quit.
    nodeNamePart.clear();
    volumeNamePart.clear();
    return FALSE;
  }

  // Null-terminate subvolume part of name & get remaining two parts
  *dot = 0;
  subvolNamePart = ptr;
  fileNamePart = ++dot;

  return (subvolNamePart.isValid() && fileNamePart.isValid());

}


//-------------------------------------------------------------------
//
//  ComGuardianFileNamePart methods
//
//-------------------------------------------------------------------
//
#define upshift1Char(x) (((x >= 'a') && (x <= 'z')) ? (char)(x - 32) : x)

const ComGuardianFileNamePart & ComGuardianFileNamePart::operator= (const char * part)
{
  // Initialise to upshifted name part
  clear();
  size_t length = strlen (part);
  if (length == 0 || length > ComFILE_NAME_PART_CHAR_MAX_LEN)
    // Invalid length, return with no modification
    return *this;

  for (size_t i=0;i<length;i++)
    ((char *)&namePart_)[i] = upshift1Char(part[i]);

  return *this;
}

// Strip for trailing blanks
void ComGuardianFileNamePart::trim (void)
{
  size_t i = length() - 1;

  while (castToConstChar()[i] == ' ')
    ((char *)&namePart_)[i--] = 0;
}

// Return an indication if a name part is valid. Optionally, the first character
// can be special (like '\' and '$').
NABoolean ComGuardianFileNamePart::isNamePartValid ( const char requiredFirstChar
                                                   , const NABoolean emptyIsOk
                                                   , const size_t minLength) const
{
  if (namePart_ == 0)
    // An empty part is valid only if specified
    return emptyIsOk;

  const char * ptr = castToConstChar();
  if (requiredFirstChar)
  {
    // Required first char
    if (*(ptr++) != requiredFirstChar)
      return FALSE;
  }
  // First char (after required first char) must be a letter
  if (*ptr < 'A' || *ptr > 'Z')
    return FALSE;

  while (*(++ptr))
  {
    // subsequent chars must be letter or digit
    if (! ((*ptr >= 'A' && *ptr <= 'Z') || (*ptr >= '0' && *ptr <= '9')))
      return FALSE;
  }
  return (length() >= minLength);
}

//-------------------------------------------------------------------
//
//  ComNodeName methods
//
//-------------------------------------------------------------------
//
// Get the node number - return value is FE error
Int32 ComNodeName::getNodeNumber (Lng32 & nodeNumber) const
{
  if (!strcmp(castToConstChar(),"\\NSK"))
  {
    nodeNumber = 0;
    return 0;
  }
  else
  {
    if (isValid())
      return FENOSUCHSYS;   // unknown system
    else
      return FEBADNAME;     // invalid name
  }
}


//-------------------------------------------------------------------
//
//  ComVolumeName methods
//
//-------------------------------------------------------------------
//


//-------------------------------------------------------------------
//
//  ComSubvolumeName methods
//
//-------------------------------------------------------------------
//
NABoolean ComSubvolumeName::isMXSubvol (const size_t svMinLength) const
{
  // A valid MX subvolume name starts with 'ZSD'
  return ( isNamePartValid ('Z', FALSE, svMinLength)        &&
           castToConstChar()[1] == 'S'  &&
           castToConstChar()[2] == 'D'
         );
      
}

NABoolean ComSubvolumeName::isMXMetaDataSubvol (void) const
{
  // A valid MX metadata subvol starts with ZSD<digit> and is 
  // at least 4 char long
  return ( isMXSubvol(4)                &&
           castToConstChar()[3] >= '0'  &&
           castToConstChar()[3] <= '9'
         );
}

NABoolean ComSubvolumeName::isMXUserDataSubvol (void) const
{
  // A valid MX metadata subvol starts with ZSD<letter> and is
  // exactly 8 char long
  return ( isMXSubvol()                 &&
           castToConstChar()[3] >= 'A'  &&
           castToConstChar()[3] <= 'Z'
         );
}

//-------------------------------------------------------------------
//
//  ComFileName methods
//
//-------------------------------------------------------------------
//

NABoolean ComFileName::isMXFile (void) const
{
  // A valid MX file name is exactly 8 characters, and
  // ends with '00' or '01'
  return ( isNamePartValid (0, FALSE, ComFILE_NAME_PART_CHAR_MAX_LEN) &&
           castToConstChar()[6] == '0'  &&
           castToConstChar()[7] >= '0'  &&
           castToConstChar()[7] <= '1'
         );
}

NABoolean ComFileName::isMXDataFork (void) const
{
  // A data fork name is an MX file name that ends with '00'
  return ( isMXFile() && castToConstChar()[7] == '0' );
}

NABoolean ComFileName::isMXResourceFork (void) const
{
  // A resource fork name is an MX file name that ends with '01'
  return ( isMXFile() && castToConstChar()[7] == '1' );
}

ComFileName ComFileName::getResourceFork (void) const
{
  // return the resource fork name for a data fork.
  ComFileName tempFileName (*this);
  tempFileName.makeResourceFork();
  return tempFileName;
}
    

ComFileName ComFileName::getDataFork (void) const
{
  // return the data fork name for a resource fork.
  ComFileName tempFileName (*this);
  tempFileName.makeDataFork();
  return tempFileName;
}

void ComFileName::makeResourceFork (void)
{
  if (isMXFile())
    ((char*)&namePart_)[7] = '1';
  else
    clear();
}

void ComFileName::makeDataFork (void)
{
  if (isMXFile())
    ((char*)&namePart_)[7] = '0';
  else
    clear();
}
