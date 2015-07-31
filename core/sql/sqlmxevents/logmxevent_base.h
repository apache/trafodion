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
 * File:         logmxevent_base.h
 * RCS:          $Id: logmxevent_base.h,v 1.1 2007/08/09 21:56:23  Exp $
 * Description:  Eventlogging functions for SQL
 *
 *
 *
 * Created:      04/25/2007
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 ****************************************************************************/
#ifndef LOGMXEVENT_BASE_H
#define LOGMXEVENT_BASE_H

// 
// Platform independent definitions and classes
//

//-----------------------------------------------------------------------------
// Enum for event token tyes
// The following are the types of tokens that can be logged (at present)
//--------------------------------------------------------------------------
// NOTE - I (jc) wish we have a generic and platform-neutral enumeration types. However, we need to
// retrofit into existing codes so that no change is required (some source files
// call logAnMxEvent directly).
//

enum TOKENTYPE
{ STRING,        // of type const char* with null termintator (maxlength = 260)
  UNICODESTRING, // of type wchar * with null terminator
  INTEGER,       // of type DWORD
  FILENAME,      // of type char * (null terminated) and of form  
                 // $Vnnnnnn.Snnnnnn.Fnnnnnn (fixed length)where
                 // $Vnnnnnn - is the Volume name
                 // Snnnnnnn - is the subvolume name
                 // Fnnnnnnn - is the Filename
  INTEGER64,     // of type long long in NSK
  LONGINT32,     // of type long (32 bit signed integer) NSK
  SHORT16,       // of type short - 16 bit integer
  COPYBUF,       // of type short *, holds copy of event msg on output
                 // must have a length of at least SQLEVENT_BUF_SIZE
  REPORTONLY     // of type bool: if true then event msg is not written to collector
};

#define EMS_SEVERITY_LEN          5
#define EMS_EVENT_TARGET_LEN      7
#define EMS_EXPERIENCE_LEVEL_LEN  8
#define EMS_BUFFER_SIZE           4000

//-------------------------------------------------------------------------
// This is a class that is used by the caller to indicate to the Type and
// value of the runtime data that needs to be logged. 
// Any number of the tokenClass instances (upto a maximum of 25) 
// can be passed in as arguments to the logAnMxEvent routine
// The length of data pointed to by pointerToInput_ cannot exceed 260
//-------------------------------------------------------------------------
class tokenClass
{
public:
  tokenClass() : type_(INTEGER), tokenCode_(0), pointerToInput_(0) {};

  tokenClass( TOKENTYPE tt, void *pointerToInput ) :
    type_(tt),
    tokenCode_(0),
    pointerToInput_(pointerToInput)
  {};

  tokenClass( TOKENTYPE tt, ULng32 tc, void *pointerToInput ) :
    type_(tt),
    tokenCode_(tc),
    pointerToInput_(pointerToInput)
  {};

  tokenClass & operator = (const tokenClass &t)
  {
    this->type_ = t.type_;
    this->tokenCode_ = t.tokenCode_;
    this->pointerToInput_ = t.pointerToInput_;
    
    return *this;
  };

  TOKENTYPE	getType()	  { return type_; };
  void *	getDataPointer()  { return pointerToInput_; };
  ULng32 getTokenCode()	  { return tokenCode_; };
  
private:
  TOKENTYPE type_;
  ULng32 tokenCode_;
  void *pointerToInput_;
  
};
  
#endif // LOGMXEVENT_BASE_H
