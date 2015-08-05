#if ! defined(LDAPCONFIGFILE_H)
#define LDAPCONFIGFILE_H
//******************************************************************************
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
//******************************************************************************
#include <string>
#include <vector>
using namespace std;

// *****************************************************************************
// *                                                                           *
// *  Class  LDAPConfigFile                                                    *
// *                                                                           *
// *         This class represents the LDAP connection configuration file      *
// *         .sqldapconfig.  The file consists of a set of name/value pairs.   *
// *         Two different LDAP configurations may be defined, each in a       *
// *         separate section.  Internally these are referred to as the        *
// *         primary and secondary configurations.  External names include     *
// *         CLUSTER, ENTERPRISE, LOCAL, and REMOTE.                           *
// *                                                                           *
// *         The file is parsed and the values of name/value pairs are         *
// *         returned to the caller.  Two data-only classes (LDAPHostConfig    *
// *         and LDAPFileContents, declared below) are used to hold the parsed *
// *         contents of the configuration file.                               *
// *                                                                           *
// *         There are three versions of the configuration file:               *
// *                                                                           *
// *         Pre-M8:                                                           *
// *         No sections, only name/values pairs. Contents are assumed to      *
// *         apply to the primary configuration.                               *
// *                                                                           *
// *         M8/M9:                                                            *
// *         May contain a LOCAL and/or REMOTE section.                        *
// *                                                                           *
// *         M10+:                                                             *
// *         Section names may be ENTERPRISE/LOCAL or CLUSTER/REMOTE.          *
// *         A DEFAULTS section is added to specify default behavior           *
// *         related to the configuration file and LDAP connection.            *
// *                                                                           *
// *  Qualities                                                                *
// *         Abstract:    No                                                   *
// *         Assignable:  No                                                   *
// *         Copyable:    No                                                   *
// *         Derivable:   Yes                                                  *
// *                                                                           *
// *****************************************************************************

enum LDAPConfigFileErrorCode {   
   LDAPConfigFile_OK = 0,
   LDAPConfigFile_FileNotFound = 1,
   LDAPConfigFile_BadAttributeName = 2,
   LDAPConfigFile_MissingValue = 3,
   LDAPConfigFile_ValueOutofRange = 4,
   LDAPConfigFile_CantOpenFile = 5,
   LDAPConfigFile_CantReadFile = 6,
   LDAPConfigFile_NoFileProvided = 7,
   LDAPConfigFile_MissingCACERTFilename = 8,
   LDAPConfigFile_MissingHostName = 9,
   LDAPConfigFile_MissingUniqueIdentifier = 10,
   LDAPConfigFile_MissingSection = 11,
   LDAPConfigFile_ParseError = 12,
   LDAPConfigFile_CantOpenLDAPRC = 13,
   LDAPConfigFile_MissingLDAPRC = 14
   
};

//define the SSL Options 
enum SSL_Option {
   NO_SSL  = 0,   //No SSL -- unencrypted connection
   YES_SSL = 1,   //SSL connection - LDAP Secured - ldaps://
   YES_TLS = 2    //TLS connection - ldap:// + StartTLS
};

enum {MAX_HOSTNAME_LENGTH = 255};

//
// This class holds the values specific to an LDAP configuration.
//
class LDAPHostConfig
{
public:

   bool selfCheck(bool isInitialized) const;

   vector<string> hostName;
   vector<bool>   isLoadBalancer;
   bool           excludeBadHosts;
   long           maxExcludeListSize;
   long           portNumber;
   string         searchDN;
   string         searchPwd;
   long           SSL_Level;
   vector<string> uniqueIdentifier;
   long           networkTimeout;
   long           timeout;
   long           timeLimit;
   long           retryCount;
   long           retryDelay;
   bool           preserveConnection;
   bool           sectionRead;
};

//
// This class represents the contents of a LDAP configuration file.
//
class LDAPFileContents
{
public:
   bool           configSectionRead;
   long           refreshTime;
   string         TLS_CACERTFilename;
   bool           defaultToPrimary;
   LDAPHostConfig primary;
   LDAPHostConfig secondary;
};

//
// This is the implementation class.  
//

class LDAPConfigFile
{
public:

   static void GetDefaultConfiguration(LDAPFileContents &defaultConfig);

   static const char * TestGetConfigFilename();

   LDAPConfigFile();
   ~LDAPConfigFile();
   
   inline bool isInitialized() const {return isInitialized_;}
   
   LDAPConfigFileErrorCode read(
      string            & configFilename,
      LDAPFileContents  & configFileContents,
      int               & lastLineNumber,
      string            & lastLine);
      
private:
   char * configFilename;
   LDAPConfigFile(LDAPConfigFile &);
   bool isInitialized_;
   
};
   
#endif
