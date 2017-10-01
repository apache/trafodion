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
#ifndef EXTQUALMODULENAMES__H
#define EXTQUALMODULENAMES__H

/* -*-C++-*-
 *****************************************************************************
 * File:         ExtQualModuleNames.h
 * Description:  This class encapsulates the scanning, parsing, accessing of 
 *               externally qualified module names for mxCompileUserModule.
 * Created:      04/11/2003
 * Language:     C++
 *****************************************************************************
 */

#include <string>
#include <vector>

struct ThreePartModuleName {
  std::string catalog, schema, module;
  bool found;
  // default constructor
  ThreePartModuleName() : catalog(), schema(), module(), found(false) {}
  // assemble 3 part name into string
  void setFullyQualifiedName(std::string& name) const {
    name = catalog + '.' + schema + '.' + module;
  }
};

class ExtQualModuleNames {
 public:
  // constructor
  ExtQualModuleNames(char* argv[], Int32 startIndex, Int32 argc,
                     const char* cat, const char* sch,
                     const char* grp, const char* tgt, const char* ver);

  // destructor
  virtual ~ExtQualModuleNames();

  // process externally qualified module names
  void processExtQualModuleNames();

  // return true iff module is in this list
  bool contains(std::string &moduleName);

  // return number of module names
  Int32 count() { return modules_.size(); }

  // return ith externally qualified module name
  const ThreePartModuleName& at(Int32 x) { return modules_.at(x); }

  static void usage();

 private:
  std::vector<ThreePartModuleName> modules_;// module names in canonical format

  // scanner & parser data members & methods
  std::string                 buffer_; // module names as typed on command line
  std::string::const_iterator bp_;     // pointer to next character in buffer_

  const char  currentChar()       { return *bp_; }
  void        advanceChar()       { bp_++; }
  const char  returnAdvanceChar() { return *bp_++; }
  bool        atEnd()             { return bp_ == buffer_.end(); }

  enum tokenType {
    COMMA  = ',', 
    DOT    = '.', 
    EQUAL  = '=', 
    LBRACE = '{',
    RBRACE = '}',
    ID, MODULEGROUP, MODULETABLESET, MODULEVERSION, SCANEOF, SCANERROR
  };
  tokenType   currentTokenCode_;
  std::string currentToken_;
  const char* tokenString(tokenType t);
  const char* currentTokenString();

  tokenType scanner();          // scan & return next token
  tokenType nextToken()         { return currentTokenCode_; }
  void      match(tokenType t); // match a terminal symbol t
  tokenType checkReserved();    // return one of: ID, MODULEGROUP, 
                                // MODULETABLESET, MODULEVERSION
  // parser methods
  void parseExternallyQualifiedModuleNames();
  void parseModuleNameList();
  void parseModuleName();
  void parseModule(std::string& cat, std::string& sch, std::string& mod);
  void parseQualifiers(std::string& grp, std::string& tgt, std::string& ver);

  // compose externally qualified module name from its component names
  void composeEQMN(std::string& grp, std::string& cat, std::string& sch,
                   std::string& mod, std::string& tgt, std::string& ver,
                   ThreePartModuleName& eqmn);

  std::string moduleCatalog_; // default module catalog
  std::string moduleSchema_;  // default module schema
  std::string moduleGroup_;   // default module group
  std::string moduleTableSet_;// default module tableset
  std::string moduleVersion_; // default module version
};

#endif // EXTQUALMODULENAMES__H
