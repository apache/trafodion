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
 * File:         ExtQualModuleNames.cpp
 * Description:  This class encapsulates the scanning, parsing, accessing of
 *               externally qualified module names for mxCompileUserModule.
 * Created:      04/11/2003
 * Language:     C++
 *
 *****************************************************************************
 */

#include <ctype.h>
#include <iostream>
#include "ComDiags.h"
#include "DgBaseType.h"
#include "ExtQualModuleNames.h"
#include "mxCompileUserModule.h"
#include "NAString.h"

// constructor
ExtQualModuleNames::ExtQualModuleNames(char* argv[], Int32 startIndex, Int32 argc,
                                       const char* cat, const char* sch,
                                       const char* grp, const char* tgt,
                                       const char* ver)
  : modules_(), buffer_(), currentTokenCode_(SCANEOF), currentToken_(),
    moduleCatalog_(cat), moduleSchema_(sch), moduleGroup_(grp),
    moduleTableSet_(tgt), moduleVersion_(ver)
{
  // make sure we have reasonable arguments
  if (!argv || startIndex >= argc) {
    bp_ = buffer_.end();
    return;
  }
  for (Int32 x = startIndex; x < argc; x++) {
    buffer_ += argv[x]; // collect arguments into buffer_
    buffer_ += ' ';
  }

  // prime scanner
  bp_ = buffer_.begin();
}

// destructor
ExtQualModuleNames::~ExtQualModuleNames()
{
  // all data members are in-line, so their destructors are called
  // automatically.
}

// return one of: ID, MODULEGROUP, MODULETABLESET, MODULEVERSION
ExtQualModuleNames::tokenType ExtQualModuleNames::checkReserved()
{
  if (currentToken_ == "MODULEGROUP")
    return MODULEGROUP;
  if (currentToken_ == "MODULETABLESET")
    return MODULETABLESET;
  if (currentToken_ == "MODULEVERSION")
    return MODULEVERSION;
  return ID;
}

// return string form of token t (used for composing error messages)
const char* ExtQualModuleNames::tokenString(tokenType t)
{
  switch (t) {
  default    : return "";
  case LBRACE: return "{";
  case RBRACE: return "}";
  case COMMA : return ",";
  case DOT   : return ".";
  case EQUAL : return "=";
  case ID    : return "identifier";
  case MODULEGROUP   : return "MODULEGROUP";
  case MODULETABLESET: return "MODULETABLESET";
  case MODULEVERSION : return "MODULEVERSION";
  case SCANEOF       : return "end-of-string";
  }
}

// return next token
ExtQualModuleNames::tokenType ExtQualModuleNames::scanner()
{
  currentToken_ = "";
  if (atEnd()) {
    return SCANEOF;
  }
  while (!atEnd()) {
    const char cc = returnAdvanceChar();
    if (isspace((unsigned char)cc)) {   // For VS2003...
      continue; // do nothing
    }
    else if (isalpha(cc)) { // regular identifier
      currentToken_ += cc;
      while (isalnum(currentChar()) || currentChar() == '_') {
        currentToken_ += currentChar();
        advanceChar();
      }
      // convert id to internal format (ie, uppercase it)
      NAString id(currentToken_.c_str());
      if (ToInternalIdentifier(id) != 0) {
        *mxCUMptr << FAIL << DgSqlCode(-2215)
                  << DgString0(currentToken_.c_str());
      }
      currentToken_ = id.data();
      return checkReserved();
    }
    currentToken_ += cc;
    switch (cc) {
    case '{' :
    case '}' :
    case ',' :
    case '.' :
    case '=' :
      return tokenType(cc);
    case '"':
      // "delimited identifier" specified by \"([^\"]|"\"\"")*\"
      while (!atEnd()) {
        const char c1 = returnAdvanceChar();
        currentToken_ += c1;
        if (c1 == '"') {
          if (currentChar() == '"') {
            currentToken_ += currentChar();
          }
          else { // end of delimited identifier
            // convert delimited id to internal format
            NAString id(currentToken_.c_str());
            if (ToInternalIdentifier(id, FALSE, TRUE) != 0) {
              *mxCUMptr << FAIL << DgSqlCode(-2209)
                        << DgString0(currentToken_.c_str());
            }
            currentToken_ = id.data();
            return ID;
          }
        }
      }
      *mxCUMptr << FAIL << DgSqlCode(-2210); // unterminated string
      return ID;
    default:
      advanceChar();
      *mxCUMptr << FAIL << DgSqlCode(-2211)
                << DgString0(currentToken_.c_str());
      return SCANERROR;
    }
  }
  return SCANEOF;
}

// return current token string (used in syntax error message)
const char* ExtQualModuleNames::currentTokenString()
{
  if (currentToken_.length())
    return currentToken_.c_str();

  if (atEnd())
    return "end-of-string";

  return &(*bp_); // Return pointer to _bp location
}

// match a terminal symbol t
void ExtQualModuleNames::match(tokenType t)
{
  if (nextToken() == t) { // matches t. all is OK.
    currentTokenCode_ = scanner(); // get next token
  }
  else {
    *mxCUMptr << FAIL << DgSqlCode(-2212) << DgString0(tokenString(t))
              << DgString1(currentTokenString());
  }
}

void ExtQualModuleNames::parseExternallyQualifiedModuleNames()
{
  // goal ::= ( <module name> [,<module name>]... )
  match(LBRACE);
  parseModuleNameList();
  match(RBRACE);
  match(SCANEOF);
}

void ExtQualModuleNames::parseModuleNameList()
{
  // <module name list> ::= <module name> [,<module name>]...
  parseModuleName();
  while (true) {
    if (nextToken() == COMMA) {
      match(COMMA);
      parseModuleName();
    }
    else {
      return;
    }
  }
}

void ExtQualModuleNames::parseModuleName()
{
  // <module name> ::= [[<catalog>.]<schema>.]<module> <qualifiers>
  std::string cat, sch, mod, grp, tgt, ver;
  ThreePartModuleName eqmn;
  switch(nextToken()) {
  case ID:
    parseModule(cat, sch, mod);
    parseQualifiers(grp, tgt, ver);
    composeEQMN(grp, cat, sch, mod, tgt, ver, eqmn);
    modules_.push_back(eqmn);
    break;
  default:
    break;
  }
}

void ExtQualModuleNames::parseModule
(std::string& cat, std::string& sch, std::string& mod)
{
  // parse [[<catalog>.]<schema>.]<module>
  std::string id1, id2;
  if (nextToken() == ID) {
    id1 = currentToken_;
    match(ID);
    if (nextToken() == DOT) {
      match(DOT);
      if (nextToken() == ID) {
        id2 = currentToken_;
        match(ID);
        if (nextToken() == DOT) {
          match(DOT);
          if (nextToken() == ID) { // module is <cat>.<sch>.<mod>
            cat = id1;
            sch = id2;
            mod = currentToken_;
            match(ID);
          }
          else {
            *mxCUMptr << FAIL << DgSqlCode(-2213)
                      << DgString0(currentToken_.c_str());
          }
        }
        else { // nextToken != DOT
          sch = id1; // module is <sch>.<mod>
          mod = id2;
          cat = moduleCatalog_;
        }
      }
      else {
        *mxCUMptr << FAIL << DgSqlCode(-2213)
                  << DgString0(currentToken_.c_str());
      }
    }
    else { // nextToken != DOT
      mod = id1; // module is just <mod>
      sch = moduleSchema_;
      cat = moduleCatalog_;
    }
  }
}

void ExtQualModuleNames::parseQualifiers
(std::string& grp, std::string& tgt, std::string& ver)
{
  // parse [MODULEGROUP=[<grp>]] [MODULETABLESET=[<tgt>]]
  //       [MODULEVERSION=[<ver>]]
  grp = moduleGroup_;
  tgt = moduleTableSet_;
  ver = moduleVersion_;
  if (nextToken() == MODULEGROUP) {
    match(MODULEGROUP);
    match(EQUAL);
    if (nextToken() == ID) {
      grp = currentToken_;
      match(ID);
    }
  }
  if (nextToken() == MODULETABLESET) {
    match(MODULETABLESET);
    match(EQUAL);
    if (nextToken() == ID) {
      tgt = currentToken_;
      match(ID);
    }
  }
  if (nextToken() == MODULEVERSION) {
    match(MODULEVERSION);
    match(EQUAL);
    if (nextToken() == ID) {
      ver = currentToken_;
      match(ID);
    }
  }
}

// compose externally qualified module name from its component names
void ExtQualModuleNames::composeEQMN
(std::string& grp, std::string& cat, std::string& sch,
 std::string& mod, std::string& tgt, std::string& ver,
 ThreePartModuleName& eqmn)
{
  if (cat.length()) {
    eqmn.catalog = cat;
  }
  if (sch.length()) {
    eqmn.schema = sch;
  }
  if (!grp.length() && !tgt.length() && !ver.length()) {
    // no group, tableset, nor version -- so no ^
    eqmn.module = mod;
  }
  else { // has at least one of: group, tableset, version
    // is mod externally qualified?
    if (strchr(mod.c_str(), '^')) { // found a ^
      // it's already an EQMN, so no need to qualify it
      eqmn.module = mod;
    }
    else { // eqmn has no ^ yet, so externally qualify it
      eqmn.module += grp;
      eqmn.module += '^';
      eqmn.module += mod;
      eqmn.module += '^';
      eqmn.module += tgt;
      eqmn.module += '^';
      eqmn.module += ver;
    }
  }
}

void ExtQualModuleNames::usage()
{
  cerr
    << "A module name list should be" << endl
    << "  (<module name>[,<module name>]...)" << endl
    << "where each <module name> can be" << endl
    << "  [[<catalog>.]<schema>.]<module> [MODULEGROUP=[<group>]]" << endl
    << "  [MODULETABLESET=[<target>]][MODULEVERSION=[<version>]]" << endl;
  exit(EXIT_FAILURE);
}

// return true iff module is in this list
bool ExtQualModuleNames::contains(std::string &moduleName)
{
  for (Int32 x=0; x<count(); x++) {
    std::string name;
    modules_[x].setFullyQualifiedName(name);
    if (name == moduleName) {
      modules_[x].found = true;
      return true;
    }
  }
  return false;
}

// process externally qualified module names
void ExtQualModuleNames::processExtQualModuleNames()
{
  // scan, parse, normalize, store externally qualified module names
  // into modules_
  currentTokenCode_ = scanner();
  parseExternallyQualifiedModuleNames();
}
