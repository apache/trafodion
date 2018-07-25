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
#ifndef LMJAVAOPTIONS_H
#define LMJAVAOPTIONS_H
/* -*-C++-*-
******************************************************************************
*
* File:         LmJavaOptions.h
* Description:  A container class for JVM option strings
*
* Created:      June 2003
* Language:     C++
*
*
******************************************************************************
*/

#include "SqlLmDllDefines.h"
#include "Platform.h"
#include "NABoolean.h"
#include "NABasicObject.h"
#include "Collections.h"

// Contents of this file
class LmJavaOptions;

// Forward declarations
class NAMemory;

//------------------------------------------------------------------------
// class LmJavaOptions
//
// This class is a wrapper around a list of C strings. LM callers pass
// an instance of this class to the LM constructor and each element in
// the list becomes a JVM startup option.
//
// But there is one twist. If the LM caller specifies a search path
// for Java classes by assigning a value to the java.class.path system
// property, the LM intercepts that setting. LM needs java.class.path
// to contain certain jars such as LM and SQLJ only. LM stores the
// caller's class path and gives it to all the EXTERNAL PATH class
// loaders, so user code is able to load classes from the LM caller's
// search path, and user code has the impression that its class path
// is in effect.
//
//------------------------------------------------------------------------

// Some data members such as the NAList come from outside this
// DLL. The Windows compiler generates a warning about them requiring
// a DLL interface in order to be used by LmJavaOptions clients. We
// will suppress such warnings.

class SQLLM_LIB_FUNC LmJavaOptions : public NABasicObject
{
public:

  LmJavaOptions();
  ~LmJavaOptions();

  ULng32 entries() const;
  const char *getOption(ULng32 i) const;

  // Add a single option and optionally trim whitespace from both ends
  // of the string
  void addOption(const char *option, NABoolean trim);

  // Add a set of options specified in a single string, treat any
  // character in the delimiters set as a separator. Optionally trim
  // whitespace from each individual option before inserting it.
  void addOptions(const char *options, const char *delimiters, NABoolean trim);

  // find an option by its prefix and return its index or NULL_COLL_INDEX
  CollIndex findByPrefix(const char *prefix) const;

  // Convenience function to add a single "-Dname=value" option
  void addSystemProperty(const char *name, const char *value);

  // Remove the option at index i
  void removeOption(ULng32 i);

  // Remove all options
  void removeAllOptions();

  // Remove all -D options for a given system property. Optionally the
  // caller can request to see the value assigned to that property in
  // the rightmost -D occurence. A non-NULL value for valueForCaller
  // specifies that the caller wants to see the value. If the caller
  // wants the value, it will be allocated as a character string on
  // callersHeap. If valueForCaller is non-NULL then callersHeap must
  // also be non-NULL.
  NABoolean removeSystemProperty(const char *name,
                                 char **valueForCaller,
                                 NAMemory *callersHeap);

  // Get the value for a -D option (and optionally reomve) a given 
  // system property. The caller can request to see the value assigned 
  // to that property in the rightmost -D occurence. A non-NULL value 
  // for valueForCaller specifies that the caller wants to see the value.
  // If the caller wants the value, it will be allocated as a character 
  // string on callersHeap. If valueForCaller is non-NULL then callersHeap must
  // also be non-NULL.
  NABoolean getSystemProperty(const char *name,
                              char **valueForCaller,
                              NAMemory *callersHeap,
                              NABoolean remove = FALSE);

  // Append all options from other to the list stored in this instance
  void merge(const LmJavaOptions &other);

  // Print all options to the LM_DEBUG output stream. See LmDebug.h
  // and LmDebug.cpp to find out how LM_DEBUG output works.
  void display();

protected:

  NAList<char*> options_;

}; // class LmJavaOptions


#endif // LMJAVAOPTIONS_H
