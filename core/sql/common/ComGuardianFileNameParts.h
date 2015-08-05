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
#ifndef ComGuardianFileNamePart_H
#define ComGuardianFileNamePart_H
#include "NABoolean.h"
#include "NAStdlib.h"

// Size defs, moved here from ComLocationNames.h
//
// maximum length allowed for file name part
//
const unsigned short ComFILE_NAME_PART_CHAR_MAX_LEN = 8;
//
// maximum length allowed for fully qualified Guardian file names,
// including separating periods
//
const unsigned short ComFULLY_QUALIFIED_FILE_NAME_MAX_LEN = (4*ComFILE_NAME_PART_CHAR_MAX_LEN)+3;

//
// maximum length allowed for fully qualified oss path names
// that represent Guardian files, including slashes and /E and /G specifiers
//
const unsigned short ComFULLY_QUALIFIED_OSS_NAME_MAX_LEN = (4*ComFILE_NAME_PART_CHAR_MAX_LEN)+6;  // 4 name parts, 6 slashes

class ComGuardianFileNamePart;
class ComNodeName;
class ComVolumeName;
class ComSubvolumeName;
class ComFileName;
//-------------------------------------------------------------------
//
//  Helper functions
//
//-------------------------------------------------------------------
//

// Return the node name corresponding to a node number as an __int64.
// Node number -1 means the local node
// Node number -2 means the Guardian default node
__int64 ComGetNodeNameAsInt64 (const Int32 nodeNumber);

// Separate an (assumed) Guardian file name into its separate parts.
// Returns the actual number of parts - or zero if name is invalid.
// The following formats are considered legal
//    $<vol>
//    \<node>.$<vol>
//    $<vol>.<svol>.<file>
//    \<node>.$<vol>.<svol>.<file>

Int32 ComInterpretGuardianFileName ( const char * fileName
                                 , ComNodeName & nodeNamePart
                                 , ComVolumeName & volumeNamePart
                                 , ComSubvolumeName & subvolNamePart
                                 , ComFileName & fileNamePart
                                 );


//-------------------------------------------------------------------
//
// Class ComGuardianFileNamePart represents a single component of a Guardian name 
// as a 64 bit integer.
// The name part, being max. 8 characters, will fit well into such a thing.
// The advantage of this representation, compared to a character string representation:
// - faster assignment between objects of this class
// - faster comparisons between objects of this class

class ComGuardianFileNamePart
{
protected:
  // All constructors are protected, since objects of this class shouldn't
  // be constructed on their own. That could be achieved by making this class
  // an abstract base class, however the overhead of virtual methods would be
  // undesirable.

  // Default constructor: An empty name part
  ComGuardianFileNamePart (void)
    : namePart_ (0)
    , nullTerminator_ (0)
  {};

  // Copy constructor
  ComGuardianFileNamePart ( const ComGuardianFileNamePart & rhs)
    : namePart_ (rhs.namePart_)
    , nullTerminator_ (0) 
  {};

  // Assignment constructors
  ComGuardianFileNamePart ( const char * namePart )          // Name part from string
    : namePart_ (0)
    , nullTerminator_ (0)
  { (*this) = namePart;};

  ComGuardianFileNamePart ( const __int64 namePartAsInt64)   // Name part from 64 bit int
    : namePart_ (namePartAsInt64)
    , nullTerminator_ (0)
  {};

public:
  // Assignment operators

  // Assignment from char. Will do upshifting but no stripping of blanks.
  // Will validate length.
  const ComGuardianFileNamePart & operator= (const char * rhs);
  // Assignment from ComGuardianFileNamePart and from 64 bit integer will simply
  // do a C++ 64 bit integer assignment.
  const ComGuardianFileNamePart & operator= (const ComGuardianFileNamePart & rhs)
    { namePart_ = rhs.namePart_;return *this; };
  const ComGuardianFileNamePart & operator= (const __int64  rhs)
    { namePart_ = rhs; return *this; };

  // Comparison operators - between objects
  NABoolean operator== (const ComGuardianFileNamePart & rhs) const
    { return (namePart_ == rhs.namePart_); };
  NABoolean operator!= (const ComGuardianFileNamePart & rhs) const
    { return (namePart_ != rhs.namePart_); };


  // Comparison operators - between object and string
  NABoolean operator== (const char * rhs) const
    { return (!strcmp((char *)&namePart_, rhs)); };
  NABoolean operator!= (const char * rhs) const
    { return (strcmp((char *)&namePart_, rhs)); };
  NABoolean operator<= (const char * rhs) const
    { return (strcmp((char *)&namePart_, rhs) <= 0); };
  NABoolean operator<  (const char * rhs) const
    { return (strcmp((char *)&namePart_, rhs) <  0); };
  NABoolean operator>= (const char * rhs) const
    { return (strcmp((char *)&namePart_, rhs) >= 0); };
  NABoolean operator>  (const char * rhs) const
    { return (strcmp((char *)&namePart_, rhs) >  0); };


  // Type conversion operators
  operator const char*() const
    {return (const char *) &namePart_;};

  // Accessors
  NABoolean isEmpty (void) const
    { return (namePart_ == 0); };
  NABoolean isNull (void) const
    { return isEmpty(); };
  size_t length (void) const
    { return strlen (*this); };

  const char * castToConstChar() const
    { return (const char *) this; };

  // Return an indication if the name part is valid. Optionally, the first character
  // can be special (like '\' and '$').
  // Rules:
  // - if special first character is required then the first character must match that
  // - the next character (or the first, if special first character is not required)
  //   must be a letter ('A' .. 'Z')
  // - remaining characters must be letters or digits
  NABoolean isNamePartValid ( const char requiredFirstChar = 0
                            , const NABoolean emptyIsOK = FALSE
                            , const size_t minLength = ComFILE_NAME_PART_CHAR_MAX_LEN) const;
  // Mutators
  void clear (void)
    { namePart_ = 0; };
  void trim (void);         // Remove trailing blanks

protected:
  __int64 namePart_;        // The name part, as a 64 bit integer
  char nullTerminator_;     // Null-terminator character. Allows for strcpy/strcmp
                            // on 8-character names.

};


//-------------------------------------------------------------------
//
//  Class ComNodeName - represents an expand node name
//
//-------------------------------------------------------------------
//
class ComNodeName : public ComGuardianFileNamePart
{
public:
  // Default constructor                            // An empty node name
  ComNodeName (void)
  {};

  // Copy constructor
  ComNodeName (const ComNodeName & rhs)
    : ComGuardianFileNamePart (rhs)                       // The other node
  {};

  // Assignment constructor: A named node
  ComNodeName (const ComGuardianFileNamePart & rhs)
    : ComGuardianFileNamePart (rhs)                       // The named node
  {};

  // Assignment constructor: A named node
  ComNodeName (const char * nodeName)
    : ComGuardianFileNamePart (nodeName)                  // The named node
  {};

  // Assignment constructor: A node number
  ComNodeName (const Int32 nodeNumber)
    : ComGuardianFileNamePart (ComGetNodeNameAsInt64(nodeNumber)) // The numbered node
  {};

  // Assignment operators
  const ComNodeName & operator= (const ComNodeName & rhs)
    { namePart_ = rhs.namePart_;return *this; };
  const ComNodeName & operator= (const char * rhs)
    {ComGuardianFileNamePart::operator= (rhs);return *this;};

  // Comparison operators 
  NABoolean operator== (const ComNodeName & rhs) const
    { return ComGuardianFileNamePart::operator== (rhs); };
  NABoolean operator!= (const ComNodeName & rhs) const
    { return ComGuardianFileNamePart::operator!= (rhs); };
  NABoolean operator<= (const ComNodeName & rhs) const
    { return ComGuardianFileNamePart::operator<= (rhs); };
  NABoolean operator<  (const ComNodeName & rhs) const
    { return ComGuardianFileNamePart::operator< (rhs); };
  NABoolean operator>= (const ComNodeName & rhs) const
    { return ComGuardianFileNamePart::operator>= (rhs); };
  NABoolean operator>  (const ComNodeName & rhs) const
    { return ComGuardianFileNamePart::operator> (rhs); };

  // Accessors
  NABoolean isLocalNode (void) const 
    { return (namePart_ == ComGetNodeNameAsInt64(-1)); };
  NABoolean isDefaultNode (void) const 
    { return (namePart_ == ComGetNodeNameAsInt64(-2)); };
  const char * getNodeNameWithoutBackslash (void) const
    { return &((char *)(&namePart_))[1]; };
  NABoolean isValid (const NABoolean emptyIsOK = FALSE) const
    { return isNamePartValid ( '\\', emptyIsOK, 2); };
  // Get the node number - return value is FE error
  Int32 getNodeNumber (Lng32 & nodeNumber) const;

  // Mutators
  void setLocal (void)
    { namePart_ = ComGetNodeNameAsInt64(-1); };
  void setDefault (void)
    { namePart_ = ComGetNodeNameAsInt64(-2); };

};


//-------------------------------------------------------------------
//
//  Class ComVolumeName - represents a Guardian volume name
//
//-------------------------------------------------------------------
//
class ComVolumeName : public ComGuardianFileNamePart
{
public:
  // Default constructor                            // An empty volume name
  ComVolumeName (void)
  {};

  // Copy constructor
  ComVolumeName (const ComVolumeName & rhs)
    : ComGuardianFileNamePart (rhs)                       // The other volume
  {};

  // Assignment constructor: A named volume
  ComVolumeName (const ComGuardianFileNamePart & rhs)
    : ComGuardianFileNamePart (rhs)                       // The named volume
  {};

  // Assignment constructor: A named volume
  ComVolumeName (const char * volumeName)
    : ComGuardianFileNamePart (volumeName)                // The named volume
  {};

  // Assignment operators
  const ComVolumeName & operator= (const ComVolumeName & rhs)
    { namePart_ = rhs.namePart_;return *this; };
  const ComVolumeName & operator= (const char * rhs)
    {ComGuardianFileNamePart::operator= (rhs);return *this;};


  // Accessors
  NABoolean isValid (const NABoolean emptyIsOK = FALSE) const
    { return isNamePartValid ( '$', emptyIsOK, 2); };
  const char * getVolumeNameWithoutDollar (void) const
    { return &((char *)(&namePart_))[1]; };

  // Mutators

};

//-------------------------------------------------------------------
//
//  Class ComSubvolumeName - represents a Guardian subvolume name
//
//-------------------------------------------------------------------
//
class ComSubvolumeName : public ComGuardianFileNamePart
{
public:
  // Default constructor                            // An empty  name
  ComSubvolumeName (void)
  {};

  // Copy constructor
  ComSubvolumeName (const ComSubvolumeName & rhs)
    : ComGuardianFileNamePart (rhs)                       // The other one
  {};

  // Assignment constructor: A named subvolume
  ComSubvolumeName (const ComGuardianFileNamePart & rhs)
    : ComGuardianFileNamePart (rhs)                       // The named item
  {};

  // Assignment constructor: A named subvolume
  ComSubvolumeName (const char * subvolumeName)
    : ComGuardianFileNamePart (subvolumeName)             // The named subvolume
  {};

  // Assignment operators
  const ComSubvolumeName & operator= (const ComSubvolumeName & rhs)
    { namePart_ = rhs.namePart_;return *this; };
  const ComSubvolumeName & operator= (const char * rhs)
    {ComGuardianFileNamePart::operator= (rhs);return *this;};

  // Accessors
  NABoolean isValid (const NABoolean emptyIsOK = FALSE) const
    { return isNamePartValid (0, emptyIsOK, 1); };

  NABoolean isMXSubvol (const size_t svMinLength = 8) const;
  NABoolean isMXMetaDataSubvol (void) const;
  NABoolean isMXUserDataSubvol (void) const;

  // Mutators

};


//-------------------------------------------------------------------
//
//  Class ComFileName - represents a Guardian file name part
//
//-------------------------------------------------------------------
//
class ComFileName : public ComGuardianFileNamePart
{
public:
  // Default constructor                            // An empty  name
  ComFileName (void)
  {};

  // Copy constructor
  ComFileName (const ComFileName & rhs)
    : ComGuardianFileNamePart (rhs)                       // The other one
  {};

  // Assignment constructor: A named file name
  ComFileName (const ComGuardianFileNamePart & rhs)
    : ComGuardianFileNamePart (rhs)                       // The named item
  {};

  // Assignment constructor: A named file
  ComFileName (const char * fileName)
    : ComGuardianFileNamePart (fileName)                  // The named file
  {};

  // Assignment operators
  const ComFileName & operator= (const ComFileName & rhs)
    { namePart_ = rhs.namePart_;return *this; };
  const ComFileName & operator= (const char * rhs)
    {ComGuardianFileNamePart::operator= (rhs);return *this;};

  // Accessors
  NABoolean isValid (const NABoolean emptyIsOK = FALSE) const
    { return isNamePartValid (0, emptyIsOK, 1); };

  NABoolean isMXFile (void) const;
  NABoolean isMXDataFork (void) const;
  NABoolean isMXResourceFork (void) const;

  // Return the resource fork for a data fork, and vice versa.
  // Will return the file name itself if already a data fork/resource fork.
  // Will return an empty name if the file name is not an MX name.
  ComFileName getResourceFork (void) const;
  ComFileName getDataFork (void) const;

  // Mutators

  // Make the file a resource fork/a data fork. Will clear non-MX files
  void makeResourceFork (void);
  void makeDataFork (void);

};

#endif
