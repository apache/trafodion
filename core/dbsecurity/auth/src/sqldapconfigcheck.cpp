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
//******************************************************************************
//                                                                             *
//     This program is used to test the correctness of an LDAP configuration   *
//  file.  It may also be used to test the correctness of the code that        *
//  parses the configuration file.                                             *
//                                                                             *
//   Instance does not need to be started to run this program.                 *
//                                                                             *
//******************************************************************************
#include "ldapconfigfile.h"

#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <iostream>

using namespace std;

// *****************************************************************************
// *                                                                           *
// * Function: printUsage                                                      *
// *                                                                           *
// *    Displays usage information for this program.                           *
// *                                                                           *
// *****************************************************************************
void printUsage()

{

   cout << "Usage: ldapconfigcheck [option]..." << endl;
   cout << "option ::= --help|-h           display usage information" << endl;
   cout << "           -file <config-filename>" << endl << endl;
   cout << "If no option is specified, the Trafodion LDAP config file specified" << endl;
   cout << "by environment variables is checked in the following order:."  << endl << endl;
   cout << "TRAFAUTH_CONFIGFILE: fully qualified filename" << endl;
   cout << "TRAFAUTH_CONFIGDIR: Filename '/.traf_authentication_config' is appended" << endl;
   cout << "Otherwise, '/sql/scripts/.traf_authentication_config' is appended to value of TRAF_HOME." << endl;

}
//***************************** End of printUsage ******************************




// *****************************************************************************
// *                                                                           *
// * Function: reportOutcome                                                   *
// *                                                                           *
// *    Report on the success or failure of the parse of the LDAP              *
// * configuration file.  For parsing errors, the line number and text of the  *
// * last line read is displayed.                                              *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <errorCode>                     LDAPConfigFileErrorCode         In       *
// *    is the                                                                 *
// *                                                                           *
// *  <configFilename>                string &                        In       *
// *    is the name of the file that was to be checked.                        *
// *                                                                           *
// *  <lineNumber>                    int                             In       *
// *    is the number of the last line that was read.                          *
// *                                                                           *
// *  <badLine>                       string &                        In       *
// *    is the text of the last line that was read.                            *
// *                                                                           *
// *****************************************************************************
void reportOutcome(
   LDAPConfigFileErrorCode  errorCode,
   string                 & configFilename,
   int                      lineNumber,
   string                 & badLine)

{

   switch (errorCode)
   {
      case LDAPConfigFile_OK:
         cout << "File " << configFilename << " is valid." << endl;
         break;
      case LDAPConfigFile_FileNotFound:
         cout << "File " << configFilename << " not found." << endl;
         break;
      case LDAPConfigFile_BadAttributeName:
         cout << "File: " << configFilename << endl;
         cout << "Invalid attribute name found on line " << lineNumber << "." << endl;
         cout << badLine << endl;
         break;
      case LDAPConfigFile_MissingValue:
         cout << "File: " << configFilename << endl;
         cout << "Missing required value on line " << lineNumber << "." << endl;
         cout << badLine << endl;
         break;
      case LDAPConfigFile_ValueOutofRange:
         cout << "File: " << configFilename << endl;
         cout << "Value out of range on line " << lineNumber << "." << endl;
         cout << badLine << endl;
         break;
      case LDAPConfigFile_CantOpenFile:
         cout << "File: " << configFilename << endl;
         cout << "Open of sqldapconfig file failed." << endl;
         break;
      case LDAPConfigFile_CantReadFile:
         cout << "File: " << configFilename << endl;
         cout << "Read of sqldapconfig file failed." << endl;
         break;
      case LDAPConfigFile_NoFileProvided:
         cout << "No file provided.  Either specify a file parameter or verify environment variables." << endl;
         break;
      case LDAPConfigFile_MissingCACERTFilename:
         cout << "File: " << configFilename << endl;
         cout << "TLS was requested in at least one section, but TLS_CACERTFilename was not provided and could not be read from .ldaprc." << endl;
         break;
      case LDAPConfigFile_MissingHostName:
         cout << "File: " << configFilename << endl;
         cout << "Missing host name in at least one section." << endl;
         cout << "Each LDAP connection configuration section must provide at least one hostname." << endl;
         break;
      case LDAPConfigFile_MissingUniqueIdentifier:
         cout << "File: " << configFilename << endl;
         cout << "Missing unique identifier in at least one section." << endl;
         cout << "Each LDAP connection configuration section must provide at least one unique identifier." << endl;
         break;
      case LDAPConfigFile_MissingSection:
         cout << "File: " << configFilename << endl;
         cout << "At least one LDAP connection configuration section must be specified." << endl;
         break;
      case LDAPConfigFile_ParseError:
         cout << "File: " << configFilename << endl;
         cout << "Internal error parsing .sqldapconfig." << endl;
         break;
      case LDAPConfigFile_CantOpenLDAPRC:
         cout << "Unable to to open .ldaprc file." << endl;
         break;
      case LDAPConfigFile_MissingLDAPRC:
         cout << "File: " << configFilename << endl;
         cout << "Missing .ldaprc and TLS_CACERTFilename is not provided; cannot determine TLS CACERT filename." << endl;
         break;
      default:
         cout << "Internal error, code " << errorCode << endl;
   }
      
}
//*************************** End of reportOutcome *****************************




#pragma page "main"
// *****************************************************************************
// *                                                                           *
// * Function: main                                                            *
// *                                                                           *
// *    Parses the arguments and calls LDAPConfigFile class to parse a         *
// * LDAP config file.  The outcome is reported either as success or the       *
// * specific error and line of the failure.                                   *
// *                                                                           *
// * Instance does not need to be started to run this program.                 *
// *                                                                           *
// *****************************************************************************
int main(int argc,char *argv[])

{

LDAPConfigFile configFile; // Class that performs the parsing
LDAPFileContents contents; // Holds the parsed results; ignored
LDAPConfigFileErrorCode errorCode = LDAPConfigFile_OK;
string line; // Text of last line read
int lineNumber;  // Line number of last line read
string sqldapconfigFilename; 

//
// If no options were specified, pass the empty filename.  On return, the 
// filename will be fully qualified with value used by the LDAPConfigFile class.
//

   if (argc <= 1)
   {
      errorCode = configFile.read(sqldapconfigFilename,contents,lineNumber,line); 
      reportOutcome(errorCode,sqldapconfigFilename,lineNumber,line);
      exit(errorCode);
   }
   
//
// Help!
//
   
   if (strcmp(argv[1],"-h") == 0 || strcmp(argv[1],"--help") == 0)
   {
      printUsage();
      exit(0);
   } 
   
//
// If a file was provided, use it.  Must be fully qualified.
// 

   if (strcmp(argv[1],"-file") == 0)
   {
      sqldapconfigFilename = argv[2];
      if (sqldapconfigFilename.size() == 0)
      {
         cout << "Missing filename" << endl;
         exit(1);
      }
      errorCode = configFile.read(sqldapconfigFilename,contents,lineNumber,line); 
      reportOutcome(errorCode,sqldapconfigFilename,lineNumber,line);
      exit(errorCode);
   } 
   
// 
// Somebody doesn't know the syntax.  Help 'em out.
//

   cout << "Unrecognized option" << endl;
   printUsage();
   exit(1);
   
}
//******************************** End of main *********************************
