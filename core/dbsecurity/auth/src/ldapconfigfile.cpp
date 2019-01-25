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

// LCOV_EXCL_START -- lets be a little paranoid and not let any in-line funcs
//                    from the header files slip into the coverage count


#include "ldapconfigfile.h" 
#include <sys/stat.h>
#include <stdio.h>  
#include <stdlib.h>
#include <string.h> 
#include <errno.h>
#include <string>
#include <vector>


// LCOV_EXCL_STOP

#define DEFAULT_PORT                        389
#define DEFAULT_NETWORK_TIMEOUT              30
#define DEFAULT_TIMEOUT                      30
#define DEFAULT_TIMELIMIT                    30
#define DEFAULT_RETRYCOUNT                    5
#define DEFAULT_RETRYDELAY                    2
#define DEFAULT_EXCLUDELISTSIZE               3

enum Check_Status {
   Check_OK = 0,
   Check_MissingHostName = 2,
   Check_MissingUniqueIdentifier,
   Check_MissingSection,
   Check_MissingCACERTFilename,
   Check_CantOpenLDAPRC,
   Check_MissingLDAPRC
};

enum Parse_Status {
   Parse_Unknown = -2,
   Parse_Next = 2,
   Parse_SwitchToPrimary,
   Parse_SwitchToSecondary,
   Parse_SwitchToDefaults,
   Parse_BadAttributeName,
   Parse_MissingValue,
   Parse_ValueOutofRange,
   Parse_MissingCACERTFilename
};

enum {
   OneDay = 86400,
   OneHour = 3600,
   FiveMinutes = 300,
   ThirtyMinutes = 1800
};

static Check_Status checkRequiredAttributes(LDAPFileContents & config);

static inline bool icmpPrefix(const char *buf, const char *prefix);

static void initializeConfig(LDAPFileContents &config);

static void initializeLDAPHost(LDAPHostConfig &hostConfig);

static inline bool isDefaultsSectionLabel(const char *str);

static inline bool isPrimarySectionLabel(const char *str);

static inline bool isSecondarySectionLabel(const char *str);

static char *getAuthConfigFilename(void);

static LDAPConfigFileErrorCode parse(
   FILE              * fp,
   LDAPFileContents  & config,
   int               & lineNumber,
   string            & badLine);

static Parse_Status parseDefaultsLine(
   LDAPFileContents & config,
   char             * inbuf);

static inline int parseIntValue(
   char       * inBuf,
   const char * nameString);   

static Parse_Status parseLDAPConfigLine(
   LDAPFileContents & config,
   LDAPHostConfig   & configLDAP,
   char             * inbuf);

static Parse_Status parseSectionLine(
   LDAPFileContents & config,
   char             * inbuf);
   
static char * parseStringValue(
   char       * inbuf,
   const char * nameString);
   
static bool parseTLSCACertFileLine(
   char       * buffer, 
   const char * prefix,
   string     & TLSCACertFilename);   

static Check_Status readLDAPRC(LDAPFileContents & config);

static void reset();

static inline int stricmp(
   const char * left, 
   const char * right);
  
static inline char *trimLeadingandTrailingBlanks(char *buf);

static bool LDAPFileContentsInitialized = false;
static bool freeConfigFilename = false;
static char * configFilename = NULL; 
static bool defaultSectionLineRead = false;
static vector<string> loadBalanceHostNames;

// *****************************************************************************
// *                                                                           *
// * Function: LDAPHostConfig::selfCheck                                       *
// *                                                                           *
// *                                                                           *
// *    Determines if this instance of LDAPHostConfig is consistent.           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <isInitialized>                 bool                            In       *
// *    if true, instance must be initialized to be consistent.                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:                                                                 *
// *                                                                           *
// *  TRUE: LDAPHostConfig instance is consistent                              *
// *  FALSE: LDAPHostConfig instance is not consistent                         *
// *                                                                           *
// *****************************************************************************
bool LDAPHostConfig::selfCheck(bool isInitialized) const

{

// If we have not read from the configuration file but we expect
// initialization to be complete, then we have an error. If we are still
// initializing, it is OK if we have not yet read the configuration file.
   if (!this->sectionRead)
   {
      if (isInitialized)
         return false;
      else
         return true;
   }
   
// We only check two items, host name and unique identifier.  We don't have 
// defaults for these and at least one must be defined in the configuration
// file.  Note, we check for these when we first read the file, but this 
// is a check to verify our cache is still good.
   
   if (this->hostName.size() == 0)
      return false;
      
   if (this->uniqueIdentifier.size() == 0)
      return false;
   
   return true;  

}
//******************* End of LDAPHostConfig::selfCheck *************************

#pragma page "LDAPConfigFile::TestGetConfigFilename"
// *****************************************************************************
// *                                                                           *
// * Function: LDAPConfigFile::TestGetConfigFilename                           *
// *                                                                           *
// *    Returns name used for LDAP config file.  Test only.                    *
// *                                                                           *
// *****************************************************************************

// LCOV_EXCL_START 
const char * TestGetConfigFilename()

{

   return configFilename;

}
// LCOV_EXCL_STOP  
//************* End of LDAPConfigFile::TestGetConfigFilename *******************


// *****************************************************************************
// *                                                                           *
// * Function: LDAPConfigFile::LDAPConfigFile                                  *
// *                                                                           *
// *    Constructor of LDAPConfigFile object.                                  *
// *                                                                           *
// *****************************************************************************
LDAPConfigFile::LDAPConfigFile()
: configFilename(NULL),
  isInitialized_(false)

{

}
//******************* End of LDAPConfigFile::LDAPConfigFile ********************

// *****************************************************************************
// *                                                                           *
// * Function: LDAPConfigFile::LDAPConfigFile                                  *
// *                                                                           *
// *    Destructor of LDAPConfigFile object.                                   *
// *                                                                           *
// *****************************************************************************
LDAPConfigFile::~LDAPConfigFile()

{

   if (freeConfigFilename)
      delete configFilename;

}
//******************* End of LDAPConfigFile::~LDAPConfigFile *******************



#pragma page "LDAPConfigFile::GetDefaultConfiguration"
// *****************************************************************************
// *                                                                           *
// * Function: LDAPConfigFile::GetDefaultConfiguration                         *
// *                                                                           *
// *    Creates an LDAP configuration using default values.                    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <configType>                    LDAPConfigType                  In       *
// *    is the configuration type of the node to be obtained.  If configType   *
// *  is UnknownConfiguration, a configType is chosen based on the setting     *
// *  in the configuration file data.                                          *
// *                                                                           *
// *****************************************************************************
void LDAPConfigFile::GetDefaultConfiguration(LDAPFileContents &defaultConfig)

{

   initializeConfig(defaultConfig);

}
//*************** End of LDAPConfigFile::GetDefaultConfiguration ***************


// *****************************************************************************
// *                                                                           *
// * Function: read                                                            *
// *                                                                           *
// *    Reads and validates a Database Authentication LDAP configuration file. *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <filename>                      string &                        In/Out   *
// *    if non-empty, is the name of the file to read and parse. If empty,     *
// *  on return is the filename that was parsed.                               *
// *                                                                           *
// *  <config>                        LDAPFileContents &              Out      *
// *    if validation is successful, passes back the configuration values.     *
// *                                                                           *
// *  <lineNumber>                    int                             Out      *
// *    if validation fails, passes back the line number where the error       *
// *  was detected.                                                            *
// *                                                                           *
// *  <badLine>                       string &                        Out      *
// *    if validation fails, passes back the line that failed.                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: LDAPConfigFileErrorCode                                          *
// *                                                                           *
// *****************************************************************************

LDAPConfigFileErrorCode LDAPConfigFile::read(
   string            & filename,
   LDAPFileContents  & config,
   int               & lineNumber,
   string            & badLine)
   
{

// Get the name of the file that contains the configuration.  If we can't
// get the name, give up.
          
const char *connFilename;

   if (filename.size() != 0)
      connFilename = filename.c_str();
   else
   {
      connFilename = getAuthConfigFilename();   
      filename = connFilename;
   }
// If we could not get a filename, return an error.
   if (connFilename == NULL || strlen(connFilename) == 0)                     
      return LDAPConfigFile_NoFileProvided;

struct stat sts;

// If the filename does not exist, return an error.
   if ((stat(connFilename,&sts)) == -1 && errno == ENOENT)
      return LDAPConfigFile_FileNotFound;

FILE *fp;      
LDAPConfigFileErrorCode returnCode = LDAPConfigFile_CantOpenFile;

   try
   {
      // Open the file read-only.  If the open fails, give up.

      fp = fopen(connFilename,"r");

      if (fp == NULL)
         return LDAPConfigFile_CantOpenFile;
         
      // Clear global parsing values.
      reset();
     
      //
      // Parse the file.  Parsing looks for syntactical errors such as 
      // bad attribute names or section names, missing required values, 
      // values not legal, etc. 
      defaultSectionLineRead = false;
      returnCode = parse(fp,config,lineNumber,badLine);
  }
  
  catch (...)
  {
     fclose(fp);
     return returnCode;
  }

// Check to see if we exited loop due to a read error
   if (ferror(fp))  
      return LDAPConfigFile_CantReadFile;      

   fclose(fp);
   
// If there was a parsing error, return it.
   if (returnCode != LDAPConfigFile_OK)
      return returnCode; 
   
// 
// Although the file is syntactically correct, there may be semantic errors.
// Examples include missing required attributes (e.g. LDAPHostName) or 
// missing related fields (TLS_CACERTFilename is required if LDAPSSL is 2, etc.
//
   
Check_Status checkCode = checkRequiredAttributes(config);

   switch (checkCode)
   {
      case Check_OK:
         LDAPFileContentsInitialized = true;
         break;
      case Check_MissingHostName:
         return LDAPConfigFile_MissingHostName;
      case Check_MissingUniqueIdentifier:
         return LDAPConfigFile_MissingUniqueIdentifier;
      case Check_MissingSection:
         return LDAPConfigFile_MissingSection;
      case Check_MissingCACERTFilename:
         return LDAPConfigFile_MissingCACERTFilename;
      default:
         return LDAPConfigFile_ParseError;
   }
   
//
// The LDAP config file is syntactically and semantically correct.  
// Environment errors (network, LDAP, etc.) still possible.
//
   isInitialized_ = true;
   return LDAPConfigFile_OK;
         
}
//********************** End of LDAPConfigFile::read ***************************


// *****************************************************************************
// *                                                                           *
// *                   Begin private functions                                 *
// *                                                                           *
// *****************************************************************************


// *****************************************************************************
// *                                                                           *
// * Function: checkRequiredAttributes                                         *
// *                                                                           *
// *    Checks the attribute values read and looks for missing required        *
// * values or missing references.                                             *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <config>                       LDAPFileContents &               In/Out   *
// *    is the LDAP attributes.                                                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: Check_Status                                                     *
// *                                                                           *
// *****************************************************************************
static Check_Status checkRequiredAttributes(LDAPFileContents & config)

{

// 
// If a section was defined, it must contain at least one LDAPHostName and 
// at least one UniqueIdentifier (oxymoron?).

   if (config.primary.sectionRead)
   {
      if (config.primary.hostName.size() == 0)
         return Check_MissingHostName;
         
      if (config.primary.uniqueIdentifier.size() == 0)
         return Check_MissingUniqueIdentifier;
   }

   if (config.secondary.sectionRead)
   {
      if (config.secondary.hostName.size() == 0)
         return Check_MissingHostName;
         
      if (config.secondary.uniqueIdentifier.size() == 0)
         return Check_MissingUniqueIdentifier;
   }
   
//
// If only a secondary section was defined and no default section was 
// specified in the defaults section, change the default from primary.
//
  
   if (!config.primary.sectionRead && config.secondary.sectionRead &&
       !defaultSectionLineRead)
      config.defaultToPrimary = false;
   
// If the user specified a default section but did not define that section, 
// we have a problem.  
  
   if (config.configSectionRead &&                 
       ((config.defaultToPrimary && !config.primary.sectionRead) ||
        (!config.defaultToPrimary && !config.secondary.sectionRead)))
      return Check_MissingSection;

// If there were no sections, mark the primary section as read.

   if (!config.primary.sectionRead && !config.secondary.sectionRead)
      config.primary.sectionRead = true;
               
//
// If TLS was requested but there is no CACERT filename specified,
// authentication will fail.  
//
   
   if ((config.primary.SSL_Level == YES_TLS || 
        config.secondary.SSL_Level == YES_TLS) && 
        config.TLS_CACERTFilename.size() == 0)
      return Check_MissingCACERTFilename;
      
   return Check_OK;

}
//******************** End of checkRequiredAttributes **************************


// *****************************************************************************
// *                                                                           *
// * Function: getAuthConfigFilename                                           *
// *                                                                           *
// * This function returns the name of the text file which holds               *
// * configuration information needed to access the identity store used        *
// * by this instance.                                                         *
// *                                                                           *
// * Three environment variables may affect where we expect to find the file:  *
// * TRAFAUTH_CONFIGFILE -- If present, fully qualified name of config file    *
// *       no other env variables are used if this is present                  *
// * TRAFAUTH_CONFIGDIR -- If present, directory in which file will be found.  *
// *       overrides any other defaults from below                             *
// * TRAF_CONF -- Standard location:                                *
// *       $TRAF_CONF/.traf_authentication_config                          *
// *                                                                           *
// * Our algorithm for picking the name/location for the config file is:       *
// *  IF AUTHLDAP_CONFIGFILE is set to a non-empty string                      *
// *    the contents of that var are used as the name.                         *
// *  ELSE                                                                     *
// *    IF TRAFAUTH_CONFIGDIR is set to a non-empty string                     *
// *       the contents are used as the dir and .traf_authentication_config    *
// *       is the name of the file in that dir                                 *
// *    ELSE                                                                   *
// *        use TRAF_CONF/.traf_authentication_config                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns: char                                                            *
// *                                                                           *
// *  NULL: Unable to get authentication configuration filename                *
// *  *   : pointer to a fully qualified authentication configuration filename.*
// *                                                                           *
// *****************************************************************************
static char *getAuthConfigFilename(void)

{

#define AUTH_CONFIG_SIMPLE_NAME "/.traf_authentication_config"

   if (configFilename != NULL)
      return configFilename;
      
   freeConfigFilename = false;
     
size_t bufSize;
char *clusterName;
char *configDir;
char *configFile = getenv("TRAFAUTH_CONFIGFILE");

   if (configFile != NULL)
   {
      configFile = trimLeadingandTrailingBlanks(configFile);
      if (strlen(configFile) > 0)
      {
         // a full qualified name is specified.  Just use it
         configFilename = configFile;  
         return configFilename;
      } 
   }    

   configDir = getenv("TRAFAUTH_CONFIGDIR");
   if (configDir != NULL) 
   {
      configDir = trimLeadingandTrailingBlanks(configDir);
      if (strlen(configDir) > 0)
      {
         // Directory is specified.  Use it with default file name
         bufSize = strlen(configDir) + strlen(AUTH_CONFIG_SIMPLE_NAME) + 1;
         configFilename = new char[bufSize];  
         if (configFilename == NULL)
            return configFilename;

         freeConfigFilename = true;
         strcpy(configFilename,configDir);
         strcat(configFilename,AUTH_CONFIG_SIMPLE_NAME);
         return configFilename;
      }
   }    

// If we get here the full name or the explicit directory were
// not specified.  Append the name to the value of TRAF_CONF.
char *sqRoot = getenv("TRAF_CONF"); //ACH changing for Trafodion?

   if (sqRoot == NULL || strlen(sqRoot) == 0)
      return configFilename;

   bufSize = strlen(sqRoot) + strlen(AUTH_CONFIG_SIMPLE_NAME) + 1;
   configFilename = new char[bufSize];  
   if (configFilename == NULL)
      return configFilename;

   freeConfigFilename = true;
   strcpy(configFilename,sqRoot);
   strcat(configFilename,AUTH_CONFIG_SIMPLE_NAME);
   return configFilename;
   
}
//************************* End of getLDAPConfigFile ***************************




// *****************************************************************************
// *                                                                           *
// * Function: icmpPrefix                                                      *
// *                                                                           *
// *    Performs a case-insensitive compare of the start of 'buffer' with      *
// * the contents of 'prefix'.                                                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <buffer>                       const char *                     In       *
// *    is the character string to be compared.                                *
// *                                                                           *
// *  <prefix>                       const char *                     In       *
// *    is the text to be compared to.                                         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:                                                                 *
// *                                                                           *
// *  true: entire contents of 'prefix' is case-insensitive match to the       *
// *        start of buffer                                                    *
// *  false: not a case-insentive match.                                       *
// *                                                                           *
// *****************************************************************************
static inline bool icmpPrefix(
   const char *buffer, 
   const char *prefix)

{

   while ((*prefix != 0) && (*buffer != 0))
   {
      // Upshift value locally and compare.  Note we do not want to 
      // pre-upshift the buffer since it could contain case-sensitive 
      // values used by a shorter prefixed option.
      if (toupper(*buffer) != toupper(*prefix))
         return false;
      buffer++;
      prefix++;
   }
   
// If we got to the end of the prefix, the start of the buffer matches 
// the prefix.   
   
   if (*prefix == 0)
      return true;  

// Buffer does not start with prefix.      
   return false;  
   
}
//************************** End of icmpPrefix *********************************

// *****************************************************************************
// *                                                                           *
// * Function: initializeConfig                                                *
// *                                                                           *
// *    Initialize the contents of the LDAP configuration buffer to default    *
// * values.                                                                   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <config>                       LDAPFileContent &                Out      *
// *    passes back the buffer with default values.                            *
// *                                                                           *
// *****************************************************************************
static void initializeConfig(LDAPFileContents &config)


{

   config.configSectionRead = false;
   config.refreshTime = ThirtyMinutes;
// Default to primary section
   config.defaultToPrimary = true;
   defaultSectionLineRead = false;
   initializeLDAPHost(config.primary);
   initializeLDAPHost(config.secondary);

}
//*********************** End of initializeConfig ******************************



// *****************************************************************************
// *                                                                           *
// * Function: initializeLDAPHost                                              *
// *                                                                           *
// *    Initialize the contents of the LDAP host configuration buffer to       *
// * default values.                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <hostConfig>                   LDAPHostConfig &                 Out      *
// *    passes back the buffer with default values.                            *
// *                                                                           *
// *****************************************************************************
static void initializeLDAPHost(LDAPHostConfig &hostConfig)

{

   hostConfig.hostName.clear();
   hostConfig.isLoadBalancer.clear();
   hostConfig.excludeBadHosts = true;
   hostConfig.maxExcludeListSize = DEFAULT_EXCLUDELISTSIZE;
   hostConfig.networkTimeout = DEFAULT_NETWORK_TIMEOUT;
   hostConfig.portNumber = DEFAULT_PORT;
   hostConfig.preserveConnection = false; 
   hostConfig.retryCount = DEFAULT_RETRYCOUNT; 
   hostConfig.retryDelay = DEFAULT_RETRYDELAY;
   hostConfig.searchDN.clear();
   hostConfig.searchPwd.clear();
   hostConfig.sectionRead = false;
   hostConfig.SSL_Level = NO_SSL;
   hostConfig.timeLimit = DEFAULT_TIMELIMIT;
   hostConfig.timeout = DEFAULT_TIMEOUT;
   hostConfig.uniqueIdentifier.clear();
   
}
//********************** End of initializeLDAPHost *****************************

// *****************************************************************************
// *                                                                           *
// * Function: isDefaultsSectionLabel                                          *
// *                                                                           *
// *    This function determines if a string matches one of the names used     *
// * to designate the defaults section of the LDAP configuration file.         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <str>                          const char *                     In       *
// *    is the string to be checked.                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:                                                                 *
// *                                                                           *
// *  true: input string is a defaults label                                   *
// *  false: input string is not a defaults label                              *
// *                                                                           *
// *****************************************************************************
static inline bool isDefaultsSectionLabel(const char *str)

{

   if (stricmp(str,"DEFAULTS") == 0)  
      return true;
      
   return false;

}
//******************** End of isDefaultsSectionLabel ***************************


// *****************************************************************************
// *                                                                           *
// * Function: isLoadBalanceHost                                               *
// *                                                                           *
// *    This function determines if a string matches one of the load balance   *
// * host names.                                                               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <str>                          const char *                     In       *
// *    is the string to be checked.                                           *
// *                                                                           *
// *  <hostNames>                    vector<string> &                 In       *
// *    is the list of host names.                                             *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:                                                                 *
// *                                                                           *
// *  true: input string is a load balance host name.                          *
// *  false: input string is not a load balance host name.                     *
// *                                                                           *
// *****************************************************************************
static inline bool isLoadBalanceHost(
   const char *    str,
   vector<string> &hostNames)

{

   for (int hostIndex = 0; hostIndex < hostNames.size(); hostIndex++)
      if (stricmp(str,hostNames[hostIndex].c_str()) == 0)
         return true;
         
   return false;

}
//******************** End of isDefaultsSectionLabel ***************************





// *****************************************************************************
// *                                                                           *
// * Function: isPrimarySectionLabel                                           *
// *                                                                           *
// *    This function determines if a string matches one of the names used     *
// * to designate a section as the primary LDAP connection configuration.      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <str>                          const char *                     In       *
// *    is the string to be checked.                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:                                                                 *
// *                                                                           *
// *  true: input string is a primary label                                    *
// *  false: input string is not a primary label                               *
// *                                                                           *
// *****************************************************************************
static inline bool isPrimarySectionLabel(const char *str)

{

   if (stricmp(str,"LOCAL") == 0 || stricmp(str,"ENTERPRISE") == 0)
      return true;
      
   return false;

}
//******************** End of isPrimarySectionLabel ****************************


// *****************************************************************************
// *                                                                           *
// * Function: isSecondarySectionLabel                                         *
// *                                                                           *
// *    This function determines if a string matches one of the names used     *
// * to designate a section as the secondary LDAP connection configuration.    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <str>                          const char *                     In       *
// *    is the string to be checked.                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:                                                                 *
// *                                                                           *
// *  true: input string is a secondary label                                  *
// *  false: input string is not a secondary label                             *
// *                                                                           *
// *****************************************************************************
static inline bool isSecondarySectionLabel(const char *str)

{

   if (stricmp(str,"REMOTE") == 0 || stricmp(str,"CLUSTER") == 0)  
      return true;
      
   return false;

}
//******************* End of isSecondarySectionLabel ***************************



// *****************************************************************************
// *                                                                           *
// * Function: parse                                                           *
// *                                                                           *
// *    Parses a Database Authentication LDAP configuration file.              *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <fp>                            FILE *                          In       *
// *    is the file pointer for an open LDAP configuration file.               *
// *                                                                           *
// *  <config>                        LDAPFileContents &              Out      *
// *    if validation is successful, passes back the configuration values.     *
// *                                                                           *
// *  <lineNumber>                    int                             Out      *
// *    if validation fails, passes back the line number where the error       *
// *  was detected.                                                            *
// *                                                                           *
// *  <badLine>                       string &                        Out      *
// *    if validation fails, passes back the line that failed.                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: LDAPConfigFileErrorCode                                          *
// *                                                                           *
// *****************************************************************************
static LDAPConfigFileErrorCode parse(
   FILE              * fp,
   LDAPFileContents  & config,
   int               & lineNumber,
   string            & badLine)

{

// Initialize all the fields to the their default values in case the 
// attribute is not specified in the configuration file.
   initializeConfig(config);

// we start off with primary LDAP definition as the one to configure.
// a "section:" directive in the file is used to switch between
// primary and secondary
struct LDAPHostConfig *currentDefinition = &config.primary; 
bool readingDefaultLines = false;

LDAPConfigFileErrorCode returnCode = LDAPConfigFile_OK;
Parse_Status status = Parse_Unknown;
char inBuf[300];

// Read each line of the file, skipping over comments (begins with #) and 
// lines shorter than 3 characters.  Compare to option prefixes, and 
// store value in corresponding field. Note, if an option is specified more
// than once, we use the last value.     
 
   lineNumber = 0;//Let's start at the very beginning...
   while (fgets(inBuf,sizeof(inBuf),fp) != NULL)
   {
      lineNumber++;
      // Skip over leading blanks   
      char *ptr = inBuf;
      
      while (*ptr == ' ')
         ptr++;
    
      // Skip comment lines
      if (ptr[0] == '#' || strlen(ptr) < 3)  
         continue;
      
      // Let's parse that line!   
      if (readingDefaultLines)
         status = parseDefaultsLine(config,ptr);
      else
         status = parseLDAPConfigLine(config,*currentDefinition,ptr);
      
      //
      // Handle the parse status.  Note, several of the values returned
      // simply change the context, while others are fatal parsing errors.
      //   
      switch (status)
      {
         case Parse_Next:
            break;
         case Parse_SwitchToPrimary:
            currentDefinition = &config.primary;
            readingDefaultLines = false;
            break;
         case Parse_SwitchToSecondary:
            currentDefinition = &config.secondary;
            readingDefaultLines = false;
            break;
         case Parse_SwitchToDefaults:
            currentDefinition = NULL;
            readingDefaultLines = true;
            break;
         case Parse_BadAttributeName:
            badLine = inBuf;
            return LDAPConfigFile_BadAttributeName;  
         case Parse_MissingValue:
            badLine = inBuf;
            return LDAPConfigFile_MissingValue;
         case Parse_ValueOutofRange:
            badLine = inBuf;
            return LDAPConfigFile_ValueOutofRange;
         case Parse_MissingCACERTFilename:
            badLine = inBuf;
            return LDAPConfigFile_MissingCACERTFilename;
         default:
            badLine = inBuf;
            return LDAPConfigFile_BadAttributeName;
      }
   }
   
   return LDAPConfigFile_OK;

}
//******************************* End of parse *********************************




// *****************************************************************************
// *                                                                           *
// * Function: parseDefaultsLine                                               *
// *                                                                           *
// *    Parses a text line in the Defaults section and updates the             *
// * corresponding value in the LDAPFileContents structure.                    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <config>                       LDAPFileContents &               Out      *
// *    passes back the buffer with corresponding value updated.               *
// *                                                                           *
// *  <inBuf>                        const char *                     In       *
// *    is the line to be parsed.                                              *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: Parse_Status                                                     *
// *                                                                           *
// *****************************************************************************
static Parse_Status parseDefaultsLine(
   LDAPFileContents & config,
   char             * inBuf)

{

#define PREFIX_DEFAULT_SECTION                 "defaultsectionname:"
#define PREFIX_REFRESH_TIME                    "refreshtime:"
#define PREFIX_TLS_CACERTFILENAME              "TLS_CACERTFILENAME:"

char *valuePtr = NULL;

   if (icmpPrefix(inBuf,PREFIX_DEFAULT_SECTION))
   {
      valuePtr = parseStringValue(inBuf,PREFIX_DEFAULT_SECTION);
      if (strlen(valuePtr) == 0)
         return Parse_MissingValue;
      if (isPrimarySectionLabel(valuePtr))
         config.defaultToPrimary = true;
      else 
         if (isSecondarySectionLabel(valuePtr))
            config.defaultToPrimary = false;
         else
            return Parse_ValueOutofRange;
      defaultSectionLineRead = true;
      return Parse_Next;    
   }         
   
   if (icmpPrefix(inBuf,PREFIX_REFRESH_TIME))
   {
      int refreshTime = parseIntValue(inBuf,PREFIX_REFRESH_TIME);
      if (refreshTime < 0 || refreshTime > OneDay)
         return Parse_ValueOutofRange;
      
      config.refreshTime = refreshTime; 
      return Parse_Next;    
   }
   
   if (icmpPrefix(inBuf,PREFIX_TLS_CACERTFILENAME))
   {
      if (!parseTLSCACertFileLine(inBuf,PREFIX_TLS_CACERTFILENAME,
                                  config.TLS_CACERTFilename))
         return Parse_MissingCACERTFilename;
         
      return Parse_Next;    
   }  

// It is not a valid line in the Defaults section, but it could be another 
// Section: line.        
   return parseSectionLine(config,inBuf);

}
//************************* End of parseDefaultsLine ***************************


// *****************************************************************************
// *                                                                           *
// * Function: parseIntValue                                                   *
// *                                                                           *
// *    Parses an int value from a line.  The name/prefix is skipped over      *
// * and the numeric string is converted to a number. Characters following the *
// * numeric string are ignored.  (Behavior retained for compatibility.)       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <inBuf>                        char *                           In       *
// *    is the line to be parsed.                                              *
// *                                                                           *
// *  <nameString>                   const char *                     In       *
// *    is the Name/Prefix to skip over.                                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: int                                                              *
// *                                                                           *
// * NOTE: 0 is returned if the numeric conversion fails.                      *
// *                                                                           *
// *****************************************************************************
static inline int parseIntValue(
   char       * inBuf,
   const char * nameString)
   
{

// endPtr is not used, but could be used to check for stray characters.

char *endPtr;
char *valuePtr; 

   valuePtr = inBuf + strlen(nameString);
   return strtol(valuePtr,&endPtr,10);

}
//*************************** End of parseIntValue *****************************





// *****************************************************************************
// *                                                                           *
// * Function: parseLDAPConfigLine                                             *
// *                                                                           *
// *    Parses a text line in the one of the host sections and updates the     *
// * corresponding value in the LDAPHostConfig structure.                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <config>                       LDAPFileContents &               Out      *
// *    passes back the buffer with corresponding value updated.               *
// *                                                                           *
// *  <configLDAP>                   LDAPHostConfig &                 Out      *
// *    passes back the buffer with corresponding value updated.               *
// *                                                                           *
// *  <inBuf>                        char *                           In       *
// *    is the line to be parsed.                                              *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: Parse_Status                                                     *
// *                                                                           *
// *****************************************************************************
static Parse_Status parseLDAPConfigLine(
   LDAPFileContents & config,
   LDAPHostConfig   & configLDAP,
   char             * inBuf)
   
{

//
// This is the list of attributes supported in the configuration sections.  
// The attribute names must match (case insensitive) and contain a trailing
// colon (:), with no space before the colon.
//
// There are three subsets of configuration attributes:
// 
// 1) Attributes used to connect to the OpenLDAP server
// 2) Attributes used by our code to manage the communication
// 3) Attributes that are no longer used, but we still accept
// 

// OpenLDAP configuration attributes
#define PREFIX_HOST_NAME               "ldaphostname:"
#define PREFIX_PORT                    "ldapport:"
#define PREFIX_SEARCH_DN               "ldapsearchdn:"
#define PREFIX_SEARCH_PWD              "ldapsearchpwd:" 
#define PREFIX_SSL                     "ldapssl:" 
#define PREFIX_UNIQUE_IDENTIFIER       "uniqueidentifier:"

// Attributes used to manage the communication
#define PREFIX_NETWORK_TIMEOUT         "ldapnetworktimeout:" 
#define PREFIX_LDAP_TIMEOUT            "ldaptimeout:" 
#define PREFIX_TIMELIMIT               "ldaptimelimit:" 
#define PREFIX_RETRY_COUNT             "retrycount:" 
#define PREFIX_RETRY_DELAY             "retrydelay:" 
#define PREFIX_PRESERVE_CONNECTION     "preserveconnection:"

// Attributes used to manage the hostnames
#define PREFIX_EXCLUDE_BAD_HOSTS       "ExcludeBadHosts:" 
#define PREFIX_LOAD_BALANCE_HOST_NAME  "LoadBalanceHostName:"
#define PREFIX_MAX_EXCLUDE_LIST_SIZE   "MaxExcludeListSize:" 

char *valuePtr = NULL;

//
// For all attributes, if they appear more than once, it is not an error.
// For two of the attributes (LDAPHostName and UniqueIdentifier) we accept
// and store a list of values.  For all other attributes the last value
// read is used.
//

//
// OpenLDAP configuration attributes
//

   if (icmpPrefix(inBuf,PREFIX_HOST_NAME))
   {
      valuePtr = parseStringValue(inBuf,PREFIX_HOST_NAME);
      if (strlen(valuePtr) == 0)
         return Parse_MissingValue;
      if (strlen(valuePtr) > MAX_HOSTNAME_LENGTH)
         return Parse_ValueOutofRange;
      configLDAP.hostName.push_back(valuePtr);
      configLDAP.isLoadBalancer.push_back(isLoadBalanceHost(valuePtr,loadBalanceHostNames));

      return Parse_Next;
   }
   
   if (icmpPrefix(inBuf,PREFIX_PORT))
   {
      configLDAP.portNumber = parseIntValue(inBuf,PREFIX_PORT);
      return Parse_Next;
   }
   
   if (icmpPrefix(inBuf,PREFIX_SEARCH_DN))
   {
      configLDAP.searchDN = parseStringValue(inBuf,PREFIX_SEARCH_DN);
      return Parse_Next;
   }
   
   if (icmpPrefix(inBuf,PREFIX_SEARCH_PWD))
   {
      configLDAP.searchPwd = parseStringValue(inBuf,PREFIX_SEARCH_PWD);
      return Parse_Next;
   }
   
   if (icmpPrefix(inBuf,PREFIX_SSL))
   {
      int SSL_Level = parseIntValue(inBuf,PREFIX_SSL);
      
      // unencrypted (NO_SSL), TLS (YES_TLS) and SSL (YES_SSL) 
      // are supported 
      if ((SSL_Level != NO_SSL) &&
          (SSL_Level != YES_TLS) &&
          (SSL_Level != YES_SSL))  
         return Parse_ValueOutofRange;    
      configLDAP.SSL_Level = SSL_Level;                    
      return Parse_Next;
   }
   
   if (icmpPrefix(inBuf,PREFIX_LOAD_BALANCE_HOST_NAME))
   {
      valuePtr = parseStringValue(inBuf,PREFIX_LOAD_BALANCE_HOST_NAME);
      loadBalanceHostNames.push_back(valuePtr);
      return Parse_Next;
   }
   
   if (icmpPrefix(inBuf,PREFIX_UNIQUE_IDENTIFIER))
   {
      valuePtr = parseStringValue(inBuf,PREFIX_UNIQUE_IDENTIFIER);
      configLDAP.uniqueIdentifier.push_back(valuePtr);
      return Parse_Next;
   }
   
//   
// Attributes used to manage the communication
//

   if (icmpPrefix(inBuf,PREFIX_NETWORK_TIMEOUT))
   {
      int networkTimeout = parseIntValue(inBuf,PREFIX_NETWORK_TIMEOUT);

      // -1 mean no timeout, wait forever
      if ((networkTimeout > 0 && networkTimeout <= OneHour) || networkTimeout == -1)
         configLDAP.networkTimeout = networkTimeout;
      else
         return Parse_ValueOutofRange;
      return Parse_Next;
   }
   
   if (icmpPrefix(inBuf,PREFIX_LDAP_TIMEOUT))
   {
      int LDAPTimeout = parseIntValue(inBuf,PREFIX_LDAP_TIMEOUT);

      // -1 mean no timeout, wait forever
      if ((LDAPTimeout > 0 && LDAPTimeout <= OneHour) || LDAPTimeout == -1)
         configLDAP.timeout = LDAPTimeout;
      else
         return Parse_ValueOutofRange;
      return Parse_Next;
   }
   
   if (icmpPrefix(inBuf,PREFIX_TIMELIMIT))
   {
      int timeLimit = parseIntValue(inBuf,PREFIX_TIMELIMIT);

      if (timeLimit > 0 && timeLimit <= OneHour)
         configLDAP.timeLimit = timeLimit;
      else
         return Parse_ValueOutofRange;
      return Parse_Next;
   }
   
   if (icmpPrefix(inBuf,PREFIX_RETRY_COUNT))
   {
      int retryCount = parseIntValue(inBuf,PREFIX_RETRY_COUNT);

      if (retryCount < 0 || retryCount > 10)
         return Parse_ValueOutofRange;
      configLDAP.retryCount = retryCount;
      return Parse_Next;
   }
   
   if (icmpPrefix(inBuf,PREFIX_RETRY_DELAY))
   {
      int retryDelay = parseIntValue(inBuf,PREFIX_RETRY_DELAY);

      if (retryDelay < 0 || retryDelay > FiveMinutes)
         return Parse_ValueOutofRange;
      configLDAP.retryDelay = retryDelay;
      return Parse_Next;
   }
   
   if (icmpPrefix(inBuf,PREFIX_PRESERVE_CONNECTION))
   {
      valuePtr = parseStringValue(inBuf,PREFIX_PRESERVE_CONNECTION);
      if (stricmp(valuePtr,"YES") == 0)
         configLDAP.preserveConnection = true;                    
      else
         if (stricmp(valuePtr,"NO") == 0)
            configLDAP.preserveConnection = false;                    
         else
            return Parse_ValueOutofRange;
      return Parse_Next;
   }
   
   if (icmpPrefix(inBuf,PREFIX_EXCLUDE_BAD_HOSTS))
   {
      valuePtr = parseStringValue(inBuf,PREFIX_EXCLUDE_BAD_HOSTS);
      if (stricmp(valuePtr,"YES") == 0)
         configLDAP.excludeBadHosts = true;                    
      else
         if (stricmp(valuePtr,"NO") == 0)
            configLDAP.excludeBadHosts = false;                    
         else
            return Parse_ValueOutofRange;
      return Parse_Next;
   }
   
   if (icmpPrefix(inBuf,PREFIX_MAX_EXCLUDE_LIST_SIZE))
   {
      int maxExcludeListSize = parseIntValue(inBuf,PREFIX_MAX_EXCLUDE_LIST_SIZE);

      if (maxExcludeListSize < 0 || maxExcludeListSize > 1024)
         return Parse_ValueOutofRange;
      configLDAP.maxExcludeListSize = maxExcludeListSize;
      return Parse_Next;
   }
   
// Not a configuration attribute, see if it is a section line.   
   return parseSectionLine(config,inBuf);

}
//*********************** End of parseLDAPConfigLine ***************************

// *****************************************************************************
// *                                                                           *
// * Function: parseSectionLine                                                *
// *                                                                           *
// *    Parses a text line in the config file.  Assumption is that it is a     *
// * Section: line, otherwise an error is returned.                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <config>                       LDAPFileContents &               Out      *
// *    passes back the buffer with corresponding value updated.               *
// *                                                                           *
// *  <inbuf>                        char *                           In       *
// *    is the line to be parsed.                                              *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: Parse_Status                                                     *
// *                                                                           *
// *****************************************************************************
static Parse_Status parseSectionLine(
   LDAPFileContents & config,
   char             * inBuf)
   
   
{

#define PREFIX_SECTION  "section:"

//
// If this is not a section line, we have an unrecognized attribute name.
//

   if (!icmpPrefix(inBuf,PREFIX_SECTION))
      return Parse_BadAttributeName;
      
//
// It's a section line; see if we recognize the section name.
//
         
char *valuePtr = parseStringValue(inBuf,PREFIX_SECTION);


   if (isPrimarySectionLabel(valuePtr))
   {
      config.primary.sectionRead = true;
      return Parse_SwitchToPrimary;
   }
   
   if (isSecondarySectionLabel(valuePtr))
   {
      config.secondary.sectionRead = true;
      return Parse_SwitchToSecondary;
   }
   
   if (isDefaultsSectionLabel(valuePtr))  
   {
      config.configSectionRead = true;
      return Parse_SwitchToDefaults;
   }
   
//
// Nope, not a section name we know about.
//
   
   return Parse_ValueOutofRange;
     
}
//************************* End of parseSectionLine ****************************



// *****************************************************************************
// *                                                                           *
// * Function: parseStringValue                                                *
// *                                                                           *
// *    Parses a string value from a line.  The name/prefix is skipped over    *
// * and leading and trailing blanks are removed from the value.               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <inBuf>                        char *                           In       *
// *    is the line to be parsed.                                              *
// *                                                                           *
// *  <nameString>                   const char *                     In       *
// *    is the Name/Prefix to skip over.                                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: char *                                                           *
// *                                                                           *
// *****************************************************************************
static inline char * parseStringValue(
   char       * inBuf,
   const char * nameString)
   
{

char * vPtr = inBuf + strlen(nameString);

   vPtr = trimLeadingandTrailingBlanks(vPtr);
   return vPtr;
   
}
//************************* End of parseStringValue ****************************

// *****************************************************************************
// *                                                                           *
// * Function: parseTLSCACertFileLine                                          *
// *                                                                           *
// *    Parses a text line in the Defaults section and updates the             *
// * corresponding value in the LDAPFileContents structure.                    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <buffer>                       char *                           In       *
// *    is the line to be parsed.                                              *
// *                                                                           *
// *  <prefix>                       const char *                     In       *
// *    is the value of the prefix for the TLS CACERTFLE label.                *
// *                                                                           *
// *  <TLSCACertFilename>            string &                         Out      *
// *    is the value of the prefix for the TLS CACERTFLE label.                *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// *  true: a file was found at the specified location.                        *
// *  false: no file specified or file not found.                              *
// *                                                                           *
// *****************************************************************************
static bool parseTLSCACertFileLine(
   char       * buffer, 
   const char * prefix,
   string     & TLSCACertFilename)

{

#define CACERTS "/cacerts/"

struct stat sts;
char *valuePtr = NULL;

   valuePtr = parseStringValue(buffer,prefix);

   if (strlen(valuePtr) == 0)
   {
      TLSCACertFilename = valuePtr;
      return true;
   }

// If the value is fully qualified (begins with slash), use as is

   if (valuePtr[0] == '/')
   {
      TLSCACertFilename = valuePtr;
      if ((stat(valuePtr,&sts)) == 0)
         return true;
         
      return false;
   }
      
// Name is not fully qualified.  See if specified filename can be found at
// $TRAF_HOME/cacerts.

char *sqRoot = getenv("TRAF_HOME");

   if (sqRoot == NULL)
      return false;

   sqRoot = trimLeadingandTrailingBlanks(sqRoot);
   if (strlen(sqRoot) == 0)
      return false;
      
int bufSize = strlen(sqRoot) + strlen(CACERTS) + strlen(valuePtr) + 1;

char *certFilename = new char[bufSize];
  
   if (certFilename == NULL)
      return false;
      
   strcpy(certFilename,sqRoot);
   strcat(certFilename,CACERTS);
   strcat(certFilename,valuePtr);
   
   TLSCACertFilename = certFilename;
   delete certFilename;

// If the filename exists in $TRAF_HOME/cacerts directory, use it.
   if ((stat(TLSCACertFilename.c_str(),&sts)) == 0)
      return true;
   
// Certificate was not in $TRAF_HOME/cacerts directory.  Check $CACERTS_DIR.

char *CACertsDir = getenv("CACERTS_DIR");

   if (CACertsDir == NULL)
      return false;
      
CACertsDir = trimLeadingandTrailingBlanks(CACertsDir);
   if (strlen(CACertsDir) == 0)
      return false;
   
   bufSize = strlen(CACertsDir) + strlen(valuePtr) + 2;
   certFilename = new char[bufSize];
  
   if (certFilename == NULL)
      return false;
 
   strcpy(certFilename,CACertsDir);
   strcat(certFilename,"/");
   strcat(certFilename,valuePtr);
   
   TLSCACertFilename = certFilename;
   delete certFilename;
   
   if ((stat(TLSCACertFilename.c_str(),&sts)) == 0)
      return true;
      
   return false;
 
}
//********************** End of parseTLSCACertFileLine *************************



// *****************************************************************************
// *                                                                           *
// * Function: readLDAPRC                                                      *
// *                                                                           *
// *    Older LDAP configuration files do not specify the CACERT filename even *
// * when TLS is used; instead, the filename is stored in $HOME/.ldparc.  This *
// * function reads the .ldaprc file and extracts the CACERT filename.         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <config>                       LDAPFileContents &               Out      *
// *    passes back the buffer with CACERT filename value updated.             *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: Check_Status                                                     *
// *                                                                           *
// *****************************************************************************
static Check_Status readLDAPRC(LDAPFileContents & config)

{

char *home = getenv("HOME");

   if ((home == NULL) || (strlen(home) == 0))
      return Check_MissingCACERTFilename;

string LDAPRCFilename(home);

    LDAPRCFilename += "/.ldaprc";
   
struct stat sts;

// If the filename does not exist, return an error.
   if ((stat(LDAPRCFilename.c_str(),&sts)) == -1 && errno == ENOENT)
// LCOV_EXCL_START 
      return Check_MissingLDAPRC;
// LCOV_EXCL_STOP 
      
char inBuf[300];

// Open the file read-only.  If the open fails, give up.

FILE *fp = fopen(LDAPRCFilename.c_str(),"r");

   if (fp == NULL)
// LCOV_EXCL_START 
      return Check_CantOpenLDAPRC;
// LCOV_EXCL_STOP 

   while (fgets(inBuf, sizeof(inBuf),fp) != NULL)
      if (icmpPrefix(inBuf,"TLS_CACERT"))
      {
         char *valuePtr = parseStringValue(inBuf,"TLS_CACERT");
         if (strlen(valuePtr) == 0)
         {
            fclose(fp);
            return Check_MissingCACERTFilename;
         }
         config.TLS_CACERTFilename = valuePtr;
         fclose(fp);
         return Check_OK;
      }
      
   fclose(fp);
   return Check_MissingCACERTFilename;

}
//**************************** End of readLDAPRC *******************************


// *****************************************************************************
// *                                                                           *
// * Function: reset                                                           *
// *                                                                           *
// * Resets parsing values.  Not thread safe.                                  *
// *                                                                           *
// *****************************************************************************
static void reset()

{

   LDAPFileContentsInitialized = false;
   defaultSectionLineRead = false;
   loadBalanceHostNames.clear();

}
//****************************** End of reset **********************************

// *****************************************************************************
// *                                                                           *
// * Function: stricmp                                                         *
// *                                                                           *
// * Performs a case-insensitive compare of the two char * buffers             *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <left>                       const char *                       In       *
// *  <right>                      const char *                       In       *
// *    the two buffers to be compared                                         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:                                                                 *
// *                                                                           *
// *  -1 : <left> is less than <right>                                         *
// *   0 : <left> and <right> are (case-insensitive) equal                     *
// *   1 : <left> is greater than <right>                                      *
// *                                                                           *
// *****************************************************************************
static inline int stricmp(
   const char *left, 
   const char *right)
   
{

   while ((*left != 0) && (*right != 0))
   {
      // Upshift value locally and compare.  
      if (toupper(*left) != toupper(*right))
      {
         if (toupper(*left) < toupper(*right)) 
            return -1;
         else
            return 1;
      }
      left++;
      right++;
   }
   if ((*left == 0) && (*right == 0))
      return 0;
      
   if (*left == 0) 
      return -1;
     
   return 1;
    
}
//****************************** End of stricmp ********************************




// *****************************************************************************
// *                                                                           *
// * Function: trimLeadingandTrailingBlanks                                    *
// *                                                                           *
// *  Removes trailing blanks (replaces with null) and returns pointer to      *
// *  first non-blank character in the buffer.                                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <buf>                          char *                           In/Out   *
// *    is a null-terminated character buffer whose leading and trailing       *
// *  blanks are to be trimmed.                                                *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:  pointer to first non-blank character in the buffer.            *
// *                                                                           *
// *****************************************************************************
static inline char *trimLeadingandTrailingBlanks(char *buf)

{


// Skip over any blanks, point to first non-blank character
        
   while ((*buf != 0) && (isspace(*buf)))
      buf++;
      
// Point to last character in the buffer.  If the last character is a blank,
// set to null and decrement the pointer until the character is non-blank.      
   
char *endPtr = endPtr = buf + strlen(buf) - 1;     //ACH extra endPtr?

   while ((buf <= endPtr) && (isspace(*endPtr)))
      *(endPtr--) = 0;
      
   return buf;
   
}
//****************** End of trimLeadingandTrailingBlanks ***********************
