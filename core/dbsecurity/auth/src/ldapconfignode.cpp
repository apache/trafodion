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



#include "ldapconfignode.h" 
#include "ldapconfigfile.h"
#include <sys/stat.h>

// These defines affect openLDAP header files and must appear before s
// those includes.

#undef HAVE_LDAPSSL_INIT
#undef HAVE_LDAP_INIT
#undef HAVE_LDAP_START_TLS_S
#undef LDAP_SET_REBIND_PROC_ARGS

#define HAVE_LDAP_INITIALIZE 1
#define HAVE_LDAP_SET_OPTION 1
#define LDAP_DEPRECATED 1
#define HAVE_LDAP_PARSE_RESULT 1
#define HAVE_LDAP_CONTROLS_FREE 1
#define HAVE_LDAP_SASL_BIND 1
#define LDAP_SASL_SIMPLE 1
//#define OPENLDAP_DEBUG 1

#include <lber.h>
#include <stdio.h>  
#include <string.h> 
#include <strings.h> 
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include <string>
#include <vector>
#include <memory>
#include <sys/time.h>
#include <sys/param.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>
#include <ldap.h>
#include <ctime>
#include <netdb.h>

#include "ld_globals.h"
#include "common/evl_sqlog_eventnum.h"

// LCOV_EXCL_STOP

enum LD_Status {
   LD_STATUS_OK = 200,
   LD_STATUS_RESOURCE_FAILURE = 201
};

enum NodeState {
   NODE_NOT_INITIALIZED = 1000,   
   NODE_ACTIVE = 1001
}; 

enum LDAP_VERSIONS { LDAP_VERSION_2 = 2, LDAP_VERSION_3 = 3};


// define max size this module uses for an EMS message
#define EMS_MSG_SIZE 500

#define LOG_AUTH_EVENT(eventID,eventText) logAuthEvent(eventID,eventText,__FILE__,__LINE__)

static size_t numBindRetries = 0;
static size_t numSearchRetries = 0;

class ConfigHostContents
{
public:
   ConfigHostContents();
   ConfigHostContents(ConfigHostContents &rhs);
   ConfigHostContents& operator=( const ConfigHostContents& rhs );  
   void refresh(LDAPHostConfig &);
 
   LDAPHostConfig      *LDAPConfig_;
   long                 lastHostIndex_;
   string               lastHostName_;
   vector<string>       excludedHostNames;
};

class ConfigNodeContents
{
public:
   ConfigNodeContents(
      LDAPConfigNode::LDAPConfigType     configType,
      LDAPConnectionType                 connectionType);
 
   LDAPConfigNode::LDAPConfigType     configType_;
   LDAPConnectionType                 connectionType_;
   LDAP *                             searchLD_;    // OpenLDAP connection for search
   LDAP *                             authLD_;      // OpenLDAP connection for auth
   NodeState                          status_;
   ConfigHostContents *               host_;
};

static void addExcludedHostName(
   ConfigNodeContents  & self,
   const char *          hostName);
   
static LDAuthStatus bindUser(
   ConfigNodeContents  & self,
   const char          * username, 
   const char          * password, 
   bool                  reconnect,
   int                 & LDAPError);
     
static int closeConnection(ConfigNodeContents  & self);

static inline bool connectToHost(
   ConfigNodeContents & self,
   const char *         hostName,
   bool                 isLoadBalanceHost,
   LDAPURLDesc &        url);
   
static LD_Status connectToURL(
   ConfigNodeContents & self,
   LDAPURLDesc        & url);

static void convertUsername(string & username);

static bool getEffectiveHostName(
   const char * hostName,
   char *     effectiveHostName);

static bool getNonExcludedHostName(
   const char *          hostName,
   char *                effectiveHostName,
   const vector<string> &excludedHosts,
   bool                  isLoadBalanceHost,
   bool                  shouldExcludeBadHosts,
   int                   retryCount,
   int                   retryDelay);
   
static LD_Status initConnection(
   ConfigNodeContents & self,
   char *               hostName,
   bool                 skipHost = false);

static bool isHostNameExcluded(
   const char *           hostName,
   const vector<string> & excludedHosts);

inline static void logConfigFileError(
   LDAPConfigFileErrorCode   fileCode,
   int                       lastLineNumber,
   string                  & lastLine);
   
static bool readLDAPConfigFile();
   
static LDSearchStatus searchUser(
   ConfigNodeContents   & self,
   const char           * inputName, 
   string               & userDN);
   
static LDSearchStatus searchUserByDN(
   ConfigNodeContents   & self,
   const string         & userDN);
   
static bool selfCheck(
   ConfigNodeContents   & self,
   bool                   isInitialized);
   
inline static bool shouldReadLDAPConfig();
// Should have an array of configurations.  Mgr hands out.  
static LDAPConfigFile configFile;
static time_t lastRefreshTime = 0;
static LDAPFileContents config;
static ConfigHostContents primaryHost;
static ConfigHostContents secondaryHost;
// The following static pointers hold the connection node for each of the four 
// connection/configuration combinations
// There are "search" and "authenticate" connection types and for each type  
// there can be a "primary" and "secondary" LDAP server configuration.
static LDAPConfigNode * primarySearchMe = NULL;     
static LDAPConfigNode * primaryAuthMe = NULL;       
static LDAPConfigNode * secondarySearchMe = NULL;    
static LDAPConfigNode * secondaryAuthMe = NULL;     

#pragma page "ConfigHostContents::ConfigHostContents"
// *****************************************************************************
// *                                                                           *
// * Function: ConfigHostContents::ConfigHostContents                          *
// *                                                                           *
// *    Constructor of a ConfigHostContents object.                            *
// *                                                                           *
// *****************************************************************************

ConfigHostContents::ConfigHostContents()
: lastHostIndex_(0),
  LDAPConfig_(NULL)

{

}
//**************** End of ConfigHostContents::ConfigHostContents ***************
ConfigHostContents::ConfigHostContents(ConfigHostContents &rhs)

{

   lastHostIndex_ = rhs.lastHostIndex_;
   LDAPConfig_ = rhs.LDAPConfig_;
   lastHostName_ = rhs.lastHostName_;
   excludedHostNames = rhs.excludedHostNames;
   
}

ConfigHostContents& ConfigHostContents::operator=(const ConfigHostContents& rhs) 

{

   lastHostIndex_ = rhs.lastHostIndex_;
   LDAPConfig_ = rhs.LDAPConfig_;
   lastHostName_ = rhs.lastHostName_;
   excludedHostNames = rhs.excludedHostNames;

} 

#pragma page "ConfigHostContents::refresh"
// *****************************************************************************
// *                                                                           *
// * Function: ConfigHostContents::refresh                                     *
// *                                                                           *
// *    Refreshes the fields of a ConfigHostContents instance.                 *
// *                                                                           *
// *****************************************************************************

void ConfigHostContents::refresh(LDAPHostConfig &LDAPHostConfig)

{

   LDAPConfig_ = &LDAPHostConfig;
   lastHostIndex_ = 0;
   lastHostName_.clear();
   excludedHostNames.clear();
   
}
//********************* End of ConfigHostContents::refresh *********************

#pragma page "ConfigNodeContents::ConfigNodeContents"
// *****************************************************************************
// *                                                                           *
// * Function: ConfigNodeContents::ConfigNodeContents                          *
// *                                                                           *
// *    constructor of a ConfigNodeContents object.                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameter:                                                               *
// *                                                                           *
// *  <configType>              LDAPConfigType                         In      *
// *    specifies the configuration (primary or secondary) the node should use.*
// *                                                                           *
// *  <connectionType>          LDAPConnectionType                     In      *
// *    is the type of connection (auth or search) the node will be used for.  *
// *                                                                           *
// *****************************************************************************

ConfigNodeContents::ConfigNodeContents(
   LDAPConfigNode::LDAPConfigType     configType,
   LDAPConnectionType                 connectionType)
: configType_(configType),
  connectionType_(connectionType), 
  searchLD_(NULL),
  authLD_(NULL),
  status_(NODE_NOT_INITIALIZED) 

{

}
//**************** End of ConfigNodeContents::ConfigNodeContents ***************



#pragma page "LDAPConfigNode::ClearRetryCounts"
// *****************************************************************************
// *                                                                           *
// * Function: LDAPConfigNode::ClearRetryCounts                                *
// *                                                                           *
// *    Resets the retry counts to zero.                                       *
// *                                                                           *
// *****************************************************************************
void LDAPConfigNode::ClearRetryCounts()

{

   numBindRetries = 0;
   numSearchRetries = 0;

}
//****************** End of LDAPConfigNode::ClearRetryCounts *******************



#pragma page "LDAPConfigNode::CloseConnection"
// *****************************************************************************
// *                                                                           *
// * Function: LDAPConfigNode::CloseConnection                                 *
// *                                                                           *
// *    Closes any currently opened connection.  No error returned to caller   *
// * if close operation fails or if no connections are open.                   *
// *                                                                           *
// *****************************************************************************

void LDAPConfigNode::CloseConnection()

{

   if (primaryAuthMe != NULL)
      closeConnection(primaryAuthMe->self);

   if (secondaryAuthMe != NULL)
      closeConnection(secondaryAuthMe->self);

   if (primarySearchMe != NULL)
      closeConnection(primarySearchMe->self);

   if (secondarySearchMe != NULL)
      closeConnection(secondarySearchMe->self);

}
//****************** End of LDAPConfigNode::CloseConnection ********************




#pragma page "LDAPConfigNode::FreeInstance"
// *****************************************************************************
// *                                                                           *
// * Function: LDAPConfigNode::FreeInstance                                    *
// *                                                                           *
// *    Destroys the singleton instance of a LDAPConfigNode object.            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <configType>                    LDAPConfigType                  In       *
// *    is the configuration type (primary or secondary) of the node to        *
// *    be freed.                                                              *
// *                                                                           *
// *  <connectionType>                LDAPConnectionType              In       *
// *    is the connection type (auth or search) of the node to be freed.       *
// *                                                                           *
// *****************************************************************************
// LCOV_EXCL_START  -- not called in normal testing

void LDAPConfigNode::FreeInstance(
   LDAPConfigType     configType,
   LDAPConnectionType connectionType)

{

   if (configType == PrimaryConfiguration)
   {
      if (connectionType == AuthenticationConnection)
      {
         if (primaryAuthMe != NULL)
         {
            delete primaryAuthMe;
            primaryAuthMe = NULL;
         }
      }
      else
      {
         if (primarySearchMe != NULL)
         {
            delete primarySearchMe;
            primarySearchMe = NULL;
         }
      }
   }  
   else
      if (connectionType == AuthenticationConnection)
      {
         if (secondaryAuthMe != NULL)
         {
            delete secondaryAuthMe;
            secondaryAuthMe = NULL;
         }
      }
      else
      {
         if (secondarySearchMe != NULL)
         {
            delete secondarySearchMe;
            secondarySearchMe = NULL;
         }
      }
      
}
// LCOV_EXCL_STOP  
//******************** End of LDAPConfigNode::FreeInstance *********************




// *****************************************************************************
// *                                                                           *
// * Function: GetBindRetryCount                                               *
// *                                                                           *
// *    Returns the number of times this instance has retried a bind operation *
// * since the last initialize() call.                                         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:  size_t                                                         *
// *                                                                           *
// *****************************************************************************
size_t LDAPConfigNode::GetBindRetryCount()

{

   return numBindRetries;
   
}
//****************** End of LDAPConfigNode::GetBindRetryCount ******************


#pragma page "LDAPConfigNode::GetConfiguration"
// *****************************************************************************
// *                                                                           *
// * Function: LDAPConfigNode::GetConfiguration                                *
// *                                                                           *
// *    Gets an LDAP configuration based on the configuration type.  Data is   *
// * either read from a configuration file (.traf_authentication_config) or    *
// * internal values are used.                                                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// *  true - LDAP configuration nodes created.                                 *
// * false - Unable to create LDAP configuration nodes.                        *
// *                                                                           *
// *****************************************************************************
bool LDAPConfigNode::GetConfiguration(LDAPConfigType &configType)

{

   
// If we have not yet read the config file, attempt to read it.  If that
// fails, return false.  We could create an internal configurations and hope
// for the best.  If so, we would need to report the problem accessing the
// LDAP configuration file.   
   if (!configFile.isInitialized())
   {
      if (!readLDAPConfigFile())
         return false;
      // Configuration was successfully read, setup primary and secondary cache   
      primaryHost.refresh(config.primary);
      secondaryHost.refresh(config.secondary);
   }      
      
   return true;

}
//****************** End of LDAPConfigNode::GetConfiguration *******************



#pragma page "LDAPConfigNode::GetInstance"
// *****************************************************************************
// *                                                                           *
// * Function: LDAPConfigNode::GetInstance                                     *
// *                                                                           *
// *    Constructs or returns the singleton instance of a LDAPConfigNode       *
// * object.  There are four subtypes of LDAPConfigNodes, one for each         *
// * connection type and one for each configuration.  Future work could        *
// * merge the two connection types into one instance and manage the           *
// * configurations using a map.                                               *
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
// *  <connectionType>                LDAPConnectionType              In       *
// *    is the connection type (auth or search) of the node to be obtained.    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: LDAPConfigNode *                                                 *
// *                                                                           *
// * NULL - Unable to obtain the node (not likely)                             *
// *    * - pointer to node matching the config and connection type.           *
// *                                                                           *
// *****************************************************************************

LDAPConfigNode * LDAPConfigNode::GetInstance(
   LDAPConfigType     configType,
   LDAPConnectionType connectionType)

{

   if (!GetConfiguration(configType))
      return NULL;   

// If the config type is not known (not specified, user requests default),
// use the value from the config file defaults section.
   if (configType == UnknownConfiguration)
   {
      if (config.defaultToPrimary)
         configType = PrimaryConfiguration;
      else
         configType = SecondaryConfiguration;
   } 
   
//
// There are four types of nodes
//
// 1) Primary Authentication (Bind) Node
// 2) Secondary Authentication (Bind) Node
// 3) Primary Search (Lookup) Node
// 4) Secondary Search (Lookup) Node
// 
// We may have already create a node of the type desired. Check the static
// pointers, otherwise, instantiate a new instance.
//  

   if (connectionType == AuthenticationConnection)
   {
      if (configType == PrimaryConfiguration)
      {
         if (primaryAuthMe == NULL)
            primaryAuthMe = new LDAPConfigNode(PrimaryConfiguration,AuthenticationConnection);
         return primaryAuthMe;
      }
      else //Secondary Configuration
      {
         if (secondaryAuthMe == NULL)
            secondaryAuthMe = new LDAPConfigNode(SecondaryConfiguration,AuthenticationConnection);
         return secondaryAuthMe;
      }
   }
   
   if (connectionType == SearchConnection)
   {
      if (configType == PrimaryConfiguration)
      {
         if (primarySearchMe == NULL)
            primarySearchMe = new LDAPConfigNode(PrimaryConfiguration,SearchConnection);
         return primarySearchMe;
      }
      else //Secondary Configuration
      {
         if (secondarySearchMe == NULL)
            secondarySearchMe = new LDAPConfigNode(SecondaryConfiguration,SearchConnection);
         return secondarySearchMe;
      }
   }
   
// We should not get here.
   
// LCOV_EXCL_START 
   return NULL;
// LCOV_EXCL_STOP 

}
//******************** End of LDAPConfigNode::GetInstance **********************

// *****************************************************************************
// *                                                                           *
// * Function: LDAPConfigNode::GetLDAPConnection                               *
// *                                                                           *
// *    Obtains and initializes a connection node.                             *
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
// *  <connectionType>                LDAPConnectionType              In       *
// *    is the connection type (auth or search) of the node to be obtained.    *
// *                                                                           *
// *  <hostName>                     char *                           [Out]    *
// *    if specified (non-NULL), passes back the name of the host if the       *
// *  connection is successful.                                                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: LDAPConfigNode *                                                 *
// *                                                                           *
// * NULL - Unable to obtain or initialize the node                            *
// *    * - Valid connection node                                              *
// *                                                                           *
// *****************************************************************************
LDAPConfigNode *LDAPConfigNode::GetLDAPConnection(
   LDAPConfigType     configType,
   LDAPConnectionType connectionType,
   char *             hostName)
   
{

LDAPConfigNode *node = LDAPConfigNode::GetInstance(configType,connectionType); 

   if (node == NULL || !node->initialize(hostName))
      return NULL;

   return node;

}
//******************* End of LDAPConfigNode::GetLDAPConnection *****************

// *****************************************************************************
// *                                                                           *
// * Function: GetSearchRetryCount                                             *
// *                                                                           *
// *    Returns the number of times this instance has retried a search         *
// * operation since the last initialize() call.                               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:  size_t                                                         *
// *                                                                           *
// *****************************************************************************
size_t LDAPConfigNode::GetSearchRetryCount()

{

   return numSearchRetries;
   
}
//**************** End of LDAPConfigNode::GetSearchRetryCount ******************



#pragma page "LDAPConfigNode::Refresh"
// *****************************************************************************
// *                                                                           *
// * Function: LDAPConfigNode::Refresh                                         *
// *                                                                           *
// *    Closes any currently opened connections and rereads configuration file *
// *                                                                           *
// *****************************************************************************

void LDAPConfigNode::Refresh()

{

   CloseConnection();
   readLDAPConfigFile();
   
}
//********************* End of LDAPConfigNode::Refresh *************************



#pragma page "LDAPConfigNode::LDAPConfigNode"
// *****************************************************************************
// *                                                                           *
// * Function: LDAPConfigNode::LDAPConfigNode                                  *
// *                                                                           *
// *    constructor of LDAPConfigNode object.                                  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameter:                                                               *
// *                                                                           *
// *  <configType>              LDAPConfigType                         In      *
// *    is the configuration type (primary or secondary) of the node.          *
// *                                                                           *
// *  <connectionType>          LDAPConnectionType                     In      *
// *    is the type of connection (auth or search) the node will be used for.  *
// *                                                                           *
// *****************************************************************************

LDAPConfigNode::LDAPConfigNode(
   LDAPConfigType     configType,
   LDAPConnectionType connectionType)
: self(*new ConfigNodeContents(configType,connectionType))
{
}
//******************* End of LDAPConfigNode::LDAPConfigNode ********************


#pragma page "LDAPConfigNode::~LDAPConfigNode"
// *****************************************************************************
// *                                                                           *
// * Function: LDAPConfigNode::~LDAPConfigNode                                 *
// *                                                                           *
// *    Destructor of LDAPConfigNode.  Not called for database authentications *
// * until the process is stopped, so clearing pointers is well, pointless.    *
// * However, Live Feed chose to load this code as a DLL and subsequently      *
// * unload and reload.  For M9SP1 Live Feed has changed to only load once,    *
// * but in case any other application chooses to load/unload/load, we need    *
// * to reset the static pointers.                                             *
// *                                                                           *
// *****************************************************************************
// LCOV_EXCL_START 
LDAPConfigNode::~LDAPConfigNode()
{

   closeConnection(self); 
   
   
   if (self.configType_ == PrimaryConfiguration)
   {
      if (self.connectionType_ == AuthenticationConnection)
         primaryAuthMe = NULL;
      else
         primarySearchMe = NULL;
   }  
   else
      if (self.connectionType_ == AuthenticationConnection)
         secondaryAuthMe = NULL;
      else
         secondarySearchMe = NULL;
   
}
// LCOV_EXCL_STOP 
//******************* End of LDAPConfigNode::~LDAPConfigNode *******************


// *****************************************************************************
// *                                                                           *
// * Function: authenticateUser                                                *
// *                                                                           *
// *    Performs a LDAP user authentication by calling LDAP bind APIs.         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <username>                      const char *                    In       *
// *    is the external username. Must be defined on LDAP server.              *
// *                                                                           *
// *  <password>                      const char *                    In       *
// *    is the password.  Cannot be blank.                                     *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:  LDAuthStatus                                                   *
// *                                                                           *
// *       LDAuthSuccessful: Username and password match values on server.     *
// *         LDAuthRejected: Either username or password does not match        *
// *                         the values on the server.                         *
// *  LDAuthResourceFailure: Problem communicating with LDAP server.  Details  *
// *                         in stdout.  For non-Live Feed authentications     *
// *                         error details are also in repository.             *
// *                                                                           *
// *****************************************************************************
LDAuthStatus LDAPConfigNode::authenticateUser(
   const char   * username, 
   const char   * password)  
   
{

int LDAPError = LDAP_SUCCESS;
LD_Status status = LD_STATUS_OK;
char emsMsg[EMS_MSG_SIZE];

   LDAuthStatus authStatus = bindUser(self,username,password,true,LDAPError);
                                                 
   if (!self.host_->LDAPConfig_->preserveConnection)   
      closeConnection(self);
     
// Unless we encountered a resource error, return the results 
// of the authentication.    
   if (authStatus != LDAuthResourceFailure)
      return authStatus;
      
//
// We had a problem communicating with the LDAP server.  Retry as many times
// as requested, pausing between attempts as configured.
//

int retry_count = self.host_->LDAPConfig_->retryCount; 

   while (retry_count--)
   {
      numBindRetries++;
      closeConnection(self);
      sleep(self.host_->LDAPConfig_->retryDelay);
      status = initConnection(self,NULL,true);
      if (status == LD_STATUS_OK)
      {
         authStatus = bindUser(self,username,password,true,LDAPError);

         if (!self.host_->LDAPConfig_->preserveConnection)
            closeConnection(self);
     
         // If the retry was successful, then return the results of the
         // authentication.  Otherwise, keep at it until we run out of retries.
         if (authStatus != LDAuthResourceFailure)
            return authStatus;
      }
      else
      {
      // Should we call initialize and try to read the config file again ?
      //   Refresh();
      }
    
   }
   
//
// We tried, but the LDAP server(s) did not want to cooperate. 
// Log how hard we tried so we don't get blamed.
//  

   if (self.host_->LDAPConfig_->retryCount)
      sprintf(emsMsg, "Failed to authenticate LDAP user %s after %d retries\n",
              username,self.host_->LDAPConfig_->retryCount);
   else
      sprintf(emsMsg, "Failed to authenticate LDAP user %s\n",username);

   LOG_AUTH_EVENT(DBS_UNKNOWN_AUTH_STATUS_ERROR,emsMsg);
   return LDAuthResourceFailure;

}
//****************** End of LDAPConfigNode::authenticateUser *******************


// *****************************************************************************
// *                                                                           *
// * Function: LDAPConfigNode::getConfigType                                   *
// *                                                                           *
// *    Returns the configuration type assigned to this node.  Useful when     *
// * defaulting and the type is chosen within the class.                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:  LDAPConfigType                                                 *
// *                                                                           *
// *****************************************************************************
LDAPConfigNode::LDAPConfigType LDAPConfigNode::getConfigType() const
   
{

   return self.configType_;
      
}
//******************* End of LDAPConfigNode::getConfigType *********************

// *****************************************************************************
// *                                                                           *
// * Function: LDAPConfigNode::initialize                                      *
// *                                                                           *
// *    This member function calls LDAP APIs to initialize a connection.       *
// *    This signature of initConnection takes param which is the name of a    *
// *    file that holds the configuration information.   That file is read and *
// *    this configuration's attributes are set accordingly.   The connection  *
// *    is then "initialzied" using those attributes                           * 
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <hostName>                     char *                           [Out]    *
// *    if specified (non-NULL), passes back the name of the host if the       *
// *  connection is successful.                                                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:                                                                 *
// *                                                                           *
// *  TRUE: Node initialized and connection is opened.                         *
// *  FALSE: Initialization failed.                                            *
// *                                                                           *
// *****************************************************************************

bool LDAPConfigNode::initialize(char * hostName)

{

// Verify node has been setup correctly before we attempt to initialize the
// connection and setup the rest of the node.
   if (!selfCheck(self,false))
   {
      LOG_AUTH_EVENT(DBS_UNKNOWN_AUTH_STATUS_ERROR,"Self check failed in initialize");
      return false;
   }   

bool doRead = shouldReadLDAPConfig();  //ACH pass in self and config

   if (self.status_ == NODE_ACTIVE)
   {
      if (!doRead)
         return true;
    
      // Preserve connection was enabled, but we are re-reading the LDAP 
      // config file, which could change connection settings.
      closeConnection(self);
   }
      
   if (doRead)
   {
      // If we cannot read the LDAP configuration file (.traf_authentication_config), 
      // we can't initialize the node.
      if (!readLDAPConfigFile())
         return false;
      
      // If we have refreshed the configuration, refresh all host config values
      if (self.configType_ == PrimaryConfiguration)
         primaryHost.refresh(config.primary);
      else
         secondaryHost.refresh(config.secondary);
   }
      
   if (self.configType_ == PrimaryConfiguration)
      self.host_ = &primaryHost;
   else
      self.host_ = &secondaryHost;
  
LD_Status retCode = initConnection(self,hostName);

   if (retCode == LD_STATUS_OK)
      return true;

int retry_count = self.host_->LDAPConfig_->retryCount;

   while (retry_count-- > 0)
   {
      if (self.connectionType_ == AuthenticationConnection)
         numBindRetries++;
      else
         numSearchRetries++;
      sleep(self.host_->LDAPConfig_->retryDelay);
      retCode = initConnection(self,hostName);
      if (retCode == LD_STATUS_OK)
         return true;
   } 
   
char emsMsg[EMS_MSG_SIZE];

   if (self.host_->LDAPConfig_->retryCount > 0)
      sprintf(emsMsg,"Unable to establish initial LDAP connection after %d retries, error %d\n",
              self.host_->LDAPConfig_->retryCount,retCode);
   else
      sprintf(emsMsg,"Unable to establish initial LDAP connection, error %d\n",
              retCode);

   LOG_AUTH_EVENT(DBS_NO_LDAP_SEARCH_CONNECTION,emsMsg);
   
   return false;
   
}
//********************* End of LDAPConfigNode::initialize **********************



// *****************************************************************************
// *                                                                           *
// * Function: LDAPConfigNode::lookupUser                                      *
// *                                                                           *
// *    Searches for a username on the directory server(s) associated with     *
// * this node.                                                                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <inputName>                     const char *                    In       *
// *    is the external username to lookup.                                    *
// *                                                                           *
// *  <userDN>                        string &                        Out      *
// *    passes back the distingushed name for this user.                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: LDSearchStatus                                                   *
// *                                                                           *
// *            LDSearchFound: Username was found on the server.               *
// *         LDSearchNotFound: Username was not found on the server.           *
// *  LDSearchResourceFailure: Problem communicating with LDAP server.         *
// *                           Details in stdout.  For non-Live Feed           *
// *                           authentications, error details are also in      *
// *                           repository.  For Live Feed authentications and  *
// *                           SQL username lookups, data is only stdout.      *
// *                                                                           *
// *****************************************************************************
LDSearchStatus LDAPConfigNode::lookupUser(
   const char             * inputName, 
   string                 & userDN)
   
{

int rc = 0;
char emsMsg[EMS_MSG_SIZE];

LDSearchStatus searchStatus = searchUser(self,inputName,userDN);
                                                 
   if (!self.host_->LDAPConfig_->preserveConnection)
      closeConnection(self);
      
// Unless we could not lookup the user due to a resource error, return
// the results of the search.    
   if (searchStatus != LDSearchResourceFailure)
      return searchStatus;
      
//
// We had a problem communicating with the LDAP server.  Retry as many times
// as requested, pausing between attempts as configured.
//
      
int retry_count = self.host_->LDAPConfig_->retryCount; 

   while (retry_count-- > 0)
   {
      numSearchRetries++;
      closeConnection(self);
      sleep(self.host_->LDAPConfig_->retryDelay);
      LD_Status rc = initConnection(self,NULL,true);
      if (rc == LD_STATUS_OK)
      {
         searchStatus = searchUser(self,inputName,userDN);

         if (!self.host_->LDAPConfig_->preserveConnection)
            closeConnection(self);
    
         // Again, if we did not get a resource failure, then return the 
         // results of the search.  Otherwise, keep at it until we 
         // exhaust the number of retries.
         if (searchStatus != LDSearchResourceFailure)
            return searchStatus;
      }
      else
      {
      // Should we call initialize and try to read the config file again ?
      }
    
   }
   
//
// OK, we gave it our best shot, but we could not communicate with the 
// configured directory server(s).  Log how hard we tried.
//  

   if (self.host_->LDAPConfig_->retryCount > 0)
      sprintf(emsMsg, "Failed to search for LDAP user %s after %d retries\n",
              inputName,self.host_->LDAPConfig_->retryCount);
   else
      sprintf(emsMsg, "Failed to search for LDAP user %s\n",inputName);
   
   LOG_AUTH_EVENT(DBS_NO_LDAP_SEARCH_CONNECTION,emsMsg);
   return LDSearchResourceFailure;

}
//********************* End of LDAPConfigNode::lookupUser **********************

// *****************************************************************************
// *                                                                           *
// *  Begin file static functions.                                             *
// *                                                                           *
// *****************************************************************************


// *****************************************************************************
// *                                                                           *
// * Function: addExcludedHostName                                             *
// *                                                                           *
// *  This function adds a host name to the list of excluded host names, and   *
// *  removes old entries if the list is too large.                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <self>                         ConfigNodeContents &             In       *
// *    is a reference to an instance of a ConfigNodeContents object.          *
// *                                                                           *
// *  <hostName>                     const char *                     In       *
// *    is name of the host to be excluded.                                    *
// *                                                                           *
// *****************************************************************************
static void addExcludedHostName(
   ConfigNodeContents  & self,
   const char *          hostName)


{

char emsMsg[EMS_MSG_SIZE];

// If the size of the excluded host list is being limited, clear out 
// older excluded hosts to make room for the newest entry.
   if (self.host_->LDAPConfig_->maxExcludeListSize > 0)
      while (self.host_->excludedHostNames.size() >= self.host_->LDAPConfig_->maxExcludeListSize)
      {
         sprintf(emsMsg,"Exclude list full, LDAP server %s removed from exclude list\n",
                 self.host_->excludedHostNames[0].c_str());
         LOG_AUTH_EVENT(DBS_UNKNOWN_AUTH_STATUS_ERROR,emsMsg); 
         
         self.host_->excludedHostNames.erase(self.host_->excludedHostNames.begin());
      }
          
   self.host_->excludedHostNames.push_back(hostName);
   
   sprintf(emsMsg,"LDAP server %s added to exclude list\n",hostName);
   LOG_AUTH_EVENT(DBS_UNKNOWN_AUTH_STATUS_ERROR,emsMsg); 
   
}
//************************ End of addExcludedHostName **************************




// *****************************************************************************
// *                                                                           *
// * Function: bindUser                                                        *
// *                                                                           *
// * Performs a LDAP user authentication by calling LDAP bind.                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <self>                         ConfigNodeContents &             In       *
// *    is a reference to an instance of a ConfigNodeContents object.          *
// *                                                                           *
// *  <username>                     const char *                     In       *
// *    is the name of the user to attempt to bind (logon).                    *
// *                                                                           *
// *  <password>                     const char *                     In       *
// *    is the password of the user being logged on.                           *
// *                                                                           *
// *  <reconnect>                    bool                             In       *
// *    is true, if the bind operation fails, attempt to reconnect (once).     *
// *                                                                           *
// *  <LDAPError>                    int &                            Out      *
// *    passes back the LDAP error associated the the last suboperation        *
// *  of the bind operation.                                                   *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns:  LDAuthStatus                                                    *
// *                                                                           *
// * LDAuthSuccessful: username and password are good.                         *
// * LDAuthRejected: password does not match username.                         *
// * LDAuthResourceFailure: LDAP operation failed unexpectedly.                *
// *                                                                           *
// *****************************************************************************
static LDAuthStatus bindUser(
   ConfigNodeContents  & self,
   const char          * username, 
   const char          * password, 
   bool                  reconnect,
   int                 & LDAPError)  

{

int rc, msgid, err;
struct timeval timeout;
LDAP *ld;
LDAPMessage *result;  
char emsMsg[EMS_MSG_SIZE];

int parserc;
LDAPControl **psrvctrls = NULL;
struct berval userpw;
char *errorTextString; 
bool isInitialized = reconnect;

   LDAPError = LDAP_SUCCESS;

   while (true)
   {
      if (!selfCheck(self,isInitialized))
      {
         LOG_AUTH_EVENT(DBS_UNKNOWN_AUTH_STATUS_ERROR,"Self check failed in bindUser");
      
         return LDAuthResourceFailure;
      }
         
      if (self.connectionType_ == AuthenticationConnection) 
         ld = self.authLD_;
      else
         ld = self.searchLD_;
      userpw.bv_val = (char *)password;
      userpw.bv_len = (userpw.bv_val != 0) ? strlen (userpw.bv_val) : 0;
      LDAPError = ldap_sasl_bind(ld,username,LDAP_SASL_SIMPLE,&userpw,psrvctrls, 
                                 0,&msgid);

      if (LDAPError != LDAP_SUCCESS || msgid == -1)
      {
// LCOV_EXCL_START 
         if (LDAPError == LDAP_SERVER_DOWN && reconnect) 
         {
            // reconnect & retry
            closeConnection(self);
            LD_Status status = initConnection(self,NULL,true);
            if (status != LD_STATUS_OK)
            {
               LOG_AUTH_EVENT(DBS_UNKNOWN_AUTH_STATUS_ERROR,"LDAP Auth Error in bindUser; unable to connect to server");
               return LDAuthResourceFailure;
            }
            reconnect = false;
            continue;            
         }
         sprintf(emsMsg, "LDAP Auth Error in bindUser; error code: %ld, ", (long) LDAPError);
         errorTextString = ldap_err2string(LDAPError);
         strncat(emsMsg, errorTextString, (EMS_MSG_SIZE - (strlen(emsMsg)+4)));
         strcat(emsMsg,"\n");
         LOG_AUTH_EVENT(DBS_NO_LDAP_AUTH_CONNECTION,emsMsg);
         return LDAuthResourceFailure;
// LCOV_EXCL_STOP 
      }

      // Use value set by LDAP_OPT_TIMEOUT, pass NULL as argument
      // timeout.tv_sec = 5;
      // timeout.tv_usec = 0;   // set timeout to 5 sec

      rc = ldap_result (ld, msgid, 1, NULL, &result);
      if (rc == -1 || rc == 0)    // timeout if rc =0, server down if rc = -1
      {
         // retry if timeout
         if (reconnect)   
         {
            // reconnect & retry
            closeConnection(self);
            LD_Status status = initConnection(self,NULL,true);
            if (status != LD_STATUS_OK)
            {
// LCOV_EXCL_START 
               LOG_AUTH_EVENT(DBS_UNKNOWN_AUTH_STATUS_ERROR,"LDAP Auth Error in bindUser; unable to connect to server");
               return LDAuthResourceFailure;
// LCOV_EXCL_STOP 
            }
            reconnect = false;
            continue;
         }
         ldap_get_option(ld, LDAP_OPT_ERROR_NUMBER, &err);
         sprintf(emsMsg, "LDAP Auth Error in bindUser; error code: %ld, ", (long)err);
         errorTextString = ldap_err2string(err);
         strncat(emsMsg, errorTextString, (EMS_MSG_SIZE - (strlen(emsMsg)+4)));
         strcat(emsMsg, "\n");
         LOG_AUTH_EVENT(DBS_NO_LDAP_AUTH_CONNECTION,emsMsg);
         LDAPError = err;
         return LDAuthResourceFailure;
      }


      LDAPControl **ctrls;
      parserc = ldap_parse_result (ld, result, &rc, 0, 0, 0, &ctrls, 1);
      if (parserc != LDAP_SUCCESS)
      {
// LCOV_EXCL_START 
         char *p = ldap_err2string(parserc);
         if (p != NULL)
         {
            strcpy(emsMsg, "LDAP Auth Error in bindUser; Failed to get bind result: ");
            strncat(emsMsg, p, (EMS_MSG_SIZE - (strlen(emsMsg)+4)) );
            strcat(emsMsg, "\n");
         }
         else
            strcpy(emsMsg, "LDAP Auth Error in bindUser; Failed to get bind result.\n");
         LOG_AUTH_EVENT(DBS_UNKNOWN_AUTH_STATUS_ERROR,emsMsg);
         LDAPError = parserc;
         return LDAuthResourceFailure;
// LCOV_EXCL_STOP 
      }
      
// *****************************************************************************
// *                                                                           *
// *    Password policy is not supported.  Support would include notify users  *
// * that their authentication failed because their password has expired or    *
// * warning that is about to expire, or that they are in a grace period and   *
// * need to change their password before a certain date or some number of     *
// * authentications.  The code is still present, but beyond the check is not  *
// * executed. If supported is ever added, the structure needs to be           *
// * initialized and passed back to callers.                                   *
// *                                                                           *
// *****************************************************************************
// Error code returned from password policy control
#define PSWPOLICY_PASSWORDEXPIRED   0
#define PSWPOLICY_ACCOUNTLOCKED     1
#define PSWPOLICY_FIRST_AFTER_RESET 2
#define PSWPOLICY_NOERROR           65535

      if (ctrls)
      {
// LCOV_EXCL_START 
         class LdapPasswordPolicy {
         public:
            int error;  // error code returned from password policy control
            int expire;
            int grace;
         }; 
         // pre-set password policy (not supported)
         //if (pwdPolicy != NULL)
         //{
         //   pwdPolicy->error = PSWPOLICY_NOERROR;
         //   pwdPolicy->expire = -1;
         //   pwdPolicy->grace = -1;
         //}
         LDAPControl *ctrl = ldap_find_control( LDAP_CONTROL_PASSWORDPOLICYRESPONSE, ctrls );
         if (ctrl) 
         {
            LdapPasswordPolicy pwdPolicy;
            ldap_parse_passwordpolicy_control(ld,ctrl,&pwdPolicy.expire, 
                                              &pwdPolicy.grace, 
                                              (LDAPPasswordPolicyError *)&pwdPolicy.error );
         }
// LCOV_EXCL_STOP  
      }

      switch (rc)
      {
         case LDAP_SUCCESS:
            return LDAuthSuccessful;
            break;
         case LDAP_NO_SUCH_OBJECT:  
         case LDAP_INVALID_CREDENTIALS:
            return LDAuthRejected; 
            break;
         default:
// LCOV_EXCL_START 
            sprintf(emsMsg, "LDAP Auth Error in bindUser; error code: %ld, ", (long)rc);
            errorTextString = ldap_err2string(rc);
            strncat(emsMsg, errorTextString, (EMS_MSG_SIZE - (strlen(emsMsg)+4)));
            strcat(emsMsg, "\n"); 
            LOG_AUTH_EVENT(DBS_NO_LDAP_AUTH_CONNECTION,emsMsg);
            LDAPError = rc;
            return LDAuthResourceFailure;
            break; 
// LCOV_EXCL_STOP  
      }
   }  // while (true)
  
}
//***************************** End of bindUser ********************************



// *****************************************************************************
// *                                                                           *
// * Function: closeConnection                                                 *
// *    This function calls LDAP APIs to close a connection.                   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <self>                         ConfigNodeContents &             In       *
// *    is a reference to an instance of a ConfigNodeContents object.          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns: OpenLDAP error code                                             *
// *                                                                           *
// *****************************************************************************
static int closeConnection(ConfigNodeContents  & self)

{

int rc = LDAP_SUCCESS;

   if (self.connectionType_ == AuthenticationConnection && self.authLD_ != NULL)
   {
      rc = ldap_unbind(self.authLD_);
      self.authLD_ = NULL;
      self.status_ = NODE_NOT_INITIALIZED;
      return rc;
   }

   if (self.connectionType_ == SearchConnection && self.searchLD_ != NULL)
   {
      rc = ldap_unbind(self.searchLD_);
      self.searchLD_ = NULL;
      self.status_ = NODE_NOT_INITIALIZED;
      return rc;
   }

   return rc;
   
}
//*************************** End of closeConnection ***************************


// *****************************************************************************
// *                                                                           *
// * Function: connectToHost                                                   *
// *                                                                           *
// *  This function attempts to connect to an LDAP host.                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <self>                         ConfigNodeContents &             In       *
// *    is a reference to an instance of a ConfigNodeContents object.          *
// *                                                                           *
// *  <hostName>                     const char *                     In       *
// *    is a string containing the host name.                                  *
// *                                                                           *
// *  <isLoadBalanceHost>            bool                             In       *
// *    is true if <hostName> is a load balancing host.                        *
// *                                                                           *
// *  <url>                          LDAPURLDesc &                    In       *
// *    is a reference to a LDAP URL Description.                              *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true:  a "good" host name was found.                                      *
// * false: Could not find a host name not on the exclude list. :(             *
// *                                                                           *
// *****************************************************************************
static inline bool connectToHost(
   ConfigNodeContents & self,
   const char *         hostName,
   bool                 isLoadBalanceHost,
   LDAPURLDesc &        url)
   
{

char effectiveHostName[MAX_HOSTNAME_LENGTH + 1];

// If we can't get a good host, don't even bother trying to connect (waste of
// time!), just return false and let the caller deal with it.
   if (!getNonExcludedHostName(hostName,effectiveHostName,
                               self.host_->excludedHostNames,
                               isLoadBalanceHost,
                               self.host_->LDAPConfig_->excludeBadHosts,
                               self.host_->LDAPConfig_->retryCount,
                               self.host_->LDAPConfig_->retryDelay))
      return false;
 
// We have a good host.  Let's try to connect.
   url.lud_host = effectiveHostName;
   LD_Status status = connectToURL(self,url);
   if (status == LD_STATUS_OK)
   {
      self.host_->lastHostName_ = url.lud_host;
      return true;
   }

// Could not connect to that host.  If we are excluding bad hosts, add it 
// to the exclude host name list. 
   if (self.host_->LDAPConfig_->excludeBadHosts)
      addExcludedHostName(self,url.lud_host);
   
   return false;

}
//**************************** End of connectToHost ****************************


// *****************************************************************************
// *                                                                           *
// * Function: connectToURL                                                    *
// *                                                                           *
// *    This function calls LDAP APIs to initialize a connection.              *
// *    It will also make an LDAP call to bind with search user so to make     *
// *    sure later a user search can be performed.                             *
// *    This is called by the outer initConnection member function for each    *
// *    url configured.                                                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <self>                         ConfigNodeContents &             In       *
// *    is a reference to an instance of a ConfigNodeContents object.          *
// *                                                                           *
// *  <url>                          LDAPURLDesc &                    In       *
// *    is a reference to a LDAP URL Description.                              *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: LD_Status                                                        *
// *                                                                           *
// * LD_STATUS_OK: a connection was made                                       *
// * LD_STATUS_RESOURCE_FAILURE: a connection could not be made                *
// *                                                                           *
// *****************************************************************************
static LD_Status connectToURL(
   ConfigNodeContents & self,
   LDAPURLDesc        & url)

{

int version;
int debug = 0;
int rc;
char emsMsg[EMS_MSG_SIZE];
char *errorTextString; 

LDAP *ld = NULL;
struct timeval tv; 

   char *ldapuri = ldap_url_desc2str( &url );
   rc = ldap_initialize (&ld, ldapuri);
   if (rc != LDAP_SUCCESS)
   {
// LCOV_EXCL_START 
      sprintf(emsMsg, "ldap_initialize failed for LDAP server %s. Error: %d, ",url.lud_host, rc);
      errorTextString = ldap_err2string(rc);
      strncat(emsMsg, errorTextString, (EMS_MSG_SIZE - (strlen(emsMsg)+4)));
      strcat(emsMsg, "\n");    
      LOG_AUTH_EVENT(DBS_NO_LDAP_SEARCH_CONNECTION,emsMsg); 
      return LD_STATUS_RESOURCE_FAILURE;
// LCOV_EXCL_STOP 
   }

   if (ld == NULL)
   {
// LCOV_EXCL_START 
      sprintf(emsMsg, "Failed to initialize the connection to LDAP server %s.  Error: ld is NULL", url.lud_host);
      LOG_AUTH_EVENT(DBS_NO_LDAP_SEARCH_CONNECTION,emsMsg); 
      return LD_STATUS_RESOURCE_FAILURE;
// LCOV_EXCL_STOP 
   }

   version = LDAP_VERSION_3;
   (void) ldap_set_option (ld, LDAP_OPT_PROTOCOL_VERSION, &version);

// set dereference alias entry to LDAP_DEREF_ALWAYS.  
// disable this option if LDAP server has alias entries cause performace issues.
int ldapderef = LDAP_DEREF_ALWAYS;

   (void) ldap_set_option (ld, LDAP_OPT_DEREF, &ldapderef);

   // set password policy controls
   LDAPControl *ctrls[2], c;
   c.ldctl_oid = (char *) LDAP_CONTROL_PASSWORDPOLICYREQUEST;
   c.ldctl_value.bv_val = NULL;
   c.ldctl_value.bv_len = 0;
   c.ldctl_iscritical = 0;
   ctrls[0] = &c;
   ctrls[1] = NULL;
   ldap_set_option( ld, LDAP_OPT_SERVER_CONTROLS, ctrls );


   // Note: set timeout value to -1 means wait until it finishes
   //       Default is -1 - no limit.

   if (self.host_->LDAPConfig_->timeout > 0)
   {
      tv.tv_sec = self.host_->LDAPConfig_->timeout;
      tv.tv_usec = 0;
      (void) ldap_set_option (ld, LDAP_OPT_TIMEOUT, &tv);
   }

   // Note: set timelimit value to 0 means wait until it finishes
   //       Default is LDAP_NO_LIMIT (0) - no limit.

   if (self.host_->LDAPConfig_->timeLimit > 0)
   {
      tv.tv_sec = self.host_->LDAPConfig_->timeLimit;
      tv.tv_usec = 0;
      (void) ldap_set_option (ld, LDAP_OPT_TIMELIMIT, &tv);
   }

   // Note: If not set then LDAP NETWORK_TIMEOUT 
   //       Default is -1 - infinite timeout

   if (self.host_->LDAPConfig_->networkTimeout > 0)
   {
      tv.tv_sec = self.host_->LDAPConfig_->networkTimeout;
      tv.tv_usec = 0;
      (void) ldap_set_option (ld, LDAP_OPT_NETWORK_TIMEOUT, &tv);
   }

   (void) ldap_set_option (ld, LDAP_OPT_REFERRALS, LDAP_OPT_ON);
   (void) ldap_set_option (ld, LDAP_OPT_RESTART, LDAP_OPT_ON);

// LCOV_EXCL_START 
   // Regular SSL mode
   if (self.host_->LDAPConfig_->SSL_Level == YES_SSL)
   {
      // set for SSL port, not for TLS port
      int tls = LDAP_OPT_X_TLS_HARD;
      (void) ldap_set_option (ld, LDAP_OPT_X_TLS, &tls);
   }
// LCOV_EXCL_STOP  

   // startTLS
   if (self.host_->LDAPConfig_->SSL_Level == YES_TLS)
   {
      int demand = LDAP_OPT_X_TLS_DEMAND;
      rc = ldap_set_option(ld,LDAP_OPT_X_TLS_REQUIRE_CERT,&demand);
      if (rc != LDAP_SUCCESS)
      {
         sprintf(emsMsg, "Require TLS certificate failed for LDAP server %s.  Error: %d, ", url.lud_host, rc);
         errorTextString = ldap_err2string(rc);
         strncat(emsMsg, errorTextString, (EMS_MSG_SIZE - (strlen(emsMsg)+4)));
         strcat(emsMsg, "\n");
         LOG_AUTH_EVENT(DBS_NO_LDAP_SEARCH_CONNECTION,emsMsg); 
         return LD_STATUS_RESOURCE_FAILURE;
      }
      
      rc = ldap_set_option(NULL,LDAP_OPT_X_TLS_CACERTFILE,
                           config.TLS_CACERTFilename.c_str());
      if (rc != LDAP_SUCCESS)
      {
         sprintf(emsMsg, "Set TLS certificate file failed for LDAP server %s.  Error: %d, ", url.lud_host, rc);
         errorTextString = ldap_err2string(rc);
         strncat(emsMsg, errorTextString, (EMS_MSG_SIZE - (strlen(emsMsg)+4)));
         strcat(emsMsg, "\n");
         LOG_AUTH_EVENT(DBS_NO_LDAP_SEARCH_CONNECTION,emsMsg); 
         return LD_STATUS_RESOURCE_FAILURE;
      }
      
      rc = ldap_start_tls_s (ld, NULL, NULL);
      if (rc != LDAP_SUCCESS)
      {
         sprintf(emsMsg, "StartTLS failed for LDAP server %s.  Error: %d, ", url.lud_host, rc);
         errorTextString = ldap_err2string(rc);
         strncat(emsMsg, errorTextString, (EMS_MSG_SIZE - (strlen(emsMsg)+4)));
         strcat(emsMsg, "\n");
         LOG_AUTH_EVENT(DBS_NO_LDAP_SEARCH_CONNECTION,emsMsg); 
         return LD_STATUS_RESOURCE_FAILURE;
      }
   }
  
   if (debug)
   {
// LCOV_EXCL_START 
      ber_set_option( NULL, LBER_OPT_DEBUG_LEVEL, &debug );
      ldap_set_option( NULL, LDAP_OPT_DEBUG_LEVEL, &debug );
// LCOV_EXCL_STOP 
   }
   
int LDAPError = LDAP_SUCCESS;
LDAuthStatus authStatus;

   if (self.connectionType_ == AuthenticationConnection)  
   {  
      self.authLD_ = ld;
      authStatus = bindUser(self,"","",false,LDAPError);
      if (authStatus != LDAuthSuccessful) 
      {
// LCOV_EXCL_START 
         sprintf(emsMsg,"Initial bind failed for LDAP server %s. Error: %d, ", 
                 url.lud_host,LDAPError);
         errorTextString = ldap_err2string(LDAPError);
         strncat(emsMsg, errorTextString, (EMS_MSG_SIZE - (strlen(emsMsg)+4)));
         strcat(emsMsg, "\n");
         LOG_AUTH_EVENT(DBS_NO_LDAP_AUTH_CONNECTION,emsMsg);  
         return LD_STATUS_RESOURCE_FAILURE;
// LCOV_EXCL_STOP  
      }
      self.status_ = NODE_ACTIVE;
      return LD_STATUS_OK;
   }
  
// Search Connection
   self.searchLD_ = ld;
   authStatus = bindUser(self,
                 self.host_->LDAPConfig_->searchDN.c_str(), 
                 self.host_->LDAPConfig_->searchPwd.c_str(), 
                 false,
                 LDAPError);
   if (authStatus != LDAuthSuccessful) 
   {
// LCOV_EXCL_START 
      sprintf(emsMsg, "Initial bind with search user failed for LDAP server %s. Error: %d, ", url.lud_host, LDAPError);
      errorTextString = ldap_err2string(LDAPError);
      strncat(emsMsg, errorTextString, (EMS_MSG_SIZE - (strlen(emsMsg)+4)));
      strcat(emsMsg, "\n");
      LOG_AUTH_EVENT(DBS_NO_LDAP_AUTH_CONNECTION,emsMsg);  
      return LD_STATUS_RESOURCE_FAILURE;
// LCOV_EXCL_STOP 
   }

   self.status_ = NODE_ACTIVE;
   return LD_STATUS_OK;

}
//***************************** End of connectToURL ****************************



// *****************************************************************************
// *                                                                           *
// * Function: convertUsername                                                 *
// *                                                                           *
// *    Scans string and for all reserved characters inserts the escape        *
// * character (\).                                                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <username>                     string &                         In/Out   *
// *    is a string containing the username to be converted.                   *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
static void convertUsername(string & username)
{

const char *reserveChar = ",+\"\\<>;#=";
char newName[513]; // Must be twice as big as largest name, plus 1
char *nPtr = newName;
const char *uPtr = username.c_str();

unsigned int uLen = strlen(uPtr);

   for (int i = 0; i < uLen; i++)
   {
      if (strchr(reserveChar, *uPtr) != NULL)
      {
         // found a reserved character
         *nPtr++ = '\\';
      }
      *nPtr++ = *uPtr++;
   }
   
   *nPtr = '\0';
   username = newName;
  
}
//************************* End of convertUsername *****************************


// *****************************************************************************
// *                                                                           *
// * Function: getEffectiveHostName                                            *
// *                                                                           *
// *  This function resolves a host name and returns the effective host name.  *
// *  e.g. load balancer host name abc.com                                     *
// *       effective host name server1.abc.com                                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <hostName>                     const char *                     In       *
// *    is a string containing the host name.                                  *
// *                                                                           *
// *                                                                           *
// *  <effectiveHostName>            char *                           Out      *
// *    passes back a string containing the effective host name.               *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true:  the name was resolved                                              *
// * false: error resolving the name, use the original name.                   *
// *                                                                           *
// *****************************************************************************
static bool getEffectiveHostName(
   const char * hostName,
   char *       effectiveHostName)
   
   
{

struct hostent *hstnm;

   hstnm = gethostbyname(hostName);
   if (!hstnm)
   {
      strcpy(effectiveHostName,hostName);
      return false;
   }

// May need to support IPv6 in the future, this is good enough for now. 
   hstnm = gethostbyaddr((const void *)*hstnm->h_addr_list,4,AF_INET);
   if (!hstnm)
   {
      strcpy(effectiveHostName,hostName);
      return false;
   }
   
   strcpy(effectiveHostName,hstnm->h_name);
   return true;

}
//********************** End of getEffectiveHostName ***************************



// *****************************************************************************
// *                                                                           *
// * Function: initConnection                                                  *
// *                                                                           *
// *    This function calls LDAP APIs to initialize a connection.              *
// *    It will also make an LDAP call to bind with search user so to make     *
// *    sure later a user search can be performed.                             *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <self>                         ConfigNodeContents &             In       *
// *    is a reference to an instance of a ConfigNodeContents object.          *
// *                                                                           *
// *  <hostName>                     char *                           [Out]    *
// *    if specified (non-NULL), passes back the name of the host if the       *
// *  connection is successful.                                                *
// *                                                                           *
// *  <skipHost>                     bool                             [In]     *
// *    if specified and true, the last connected host is added to the         *
// *  excluded hosts list and the connection is attempted with the next        *
// *  host in the list.                                                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: LD_Status                                                        *
// *                                                                           *
// * LD_STATUS_OK: a connection was made                                       *
// * LD_STATUS_RESOURCE_FAILURE: a connection could not be made                *
// *                                                                           *
// *****************************************************************************

static LD_Status initConnection(
   ConfigNodeContents & self,
   char *               hostName,
   bool                 skipHost)

{

// Bailout if the section requested was not present in the config file
   if (!self.host_->LDAPConfig_->sectionRead)
      return LD_STATUS_RESOURCE_FAILURE;//ACH Need to log details.  User registration type not defined in .traf_authentication_config.

LDAPURLDesc url;

   memset( &url, 0, sizeof(url));
  
   if (self.host_->LDAPConfig_->SSL_Level == NO_SSL || 
       self.host_->LDAPConfig_->SSL_Level == YES_TLS)
      url.lud_scheme = (char *) "ldap";
// LCOV_EXCL_START 
   else
      url.lud_scheme = (char *) "ldaps";
// LCOV_EXCL_STOP  

   url.lud_port = self.host_->LDAPConfig_->portNumber;
   url.lud_scope = LDAP_SCOPE_DEFAULT;

vector<string> hostNames;

//ACH why make a copy?
   hostNames = self.host_->LDAPConfig_->hostName;
   
// If the caller has encountered a problem with the current connection,
// mark that host as bad and move on to the next one in the list.
   if (skipHost)
   {
      addExcludedHostName(self,self.host_->lastHostName_.c_str());
      
      self.host_->lastHostIndex_ = (self.host_->lastHostIndex_ + 1) % hostNames.size(); 
   }
   
// Just in case lastHostIndex_ is overwritten, check for large value and reset
   if (self.host_->lastHostIndex_ >= hostNames.size())
      self.host_->lastHostIndex_ = 0;
      
// Start from the host that we last made a good connection
int lastHostIndex = self.host_->lastHostIndex_;

   for (int hostIndex = lastHostIndex; hostIndex < hostNames.size(); hostIndex++)
      if (connectToHost(self,(char *)hostNames[hostIndex].c_str(),
                        self.host_->LDAPConfig_->isLoadBalancer[hostIndex],url))
      {
         self.host_->lastHostIndex_ = hostIndex;
         if (hostName != NULL)
            strcpy(hostName,url.lud_host);
         return LD_STATUS_OK;
      }

// Start from the first Host Name and try the remaining hosts in the list
   for (int hostIndex = 0; hostIndex < lastHostIndex; hostIndex++)
      if (connectToHost(self,(char *)hostNames[hostIndex].c_str(),
                        self.host_->LDAPConfig_->isLoadBalancer[hostIndex],url))
      {
         self.host_->lastHostIndex_ = hostIndex;
         if (hostName != NULL)
            strcpy(hostName,url.lud_host);
         return LD_STATUS_OK;
      }

   return LD_STATUS_RESOURCE_FAILURE;
   
}
//**************************** End of initConnection ***************************

// *****************************************************************************
// *                                                                           *
// * Function: isHostNameExcluded                                              *
// *                                                                           *
// *  This function determines if a host name is on the naughty list.          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <hostName>                     const char *                     In       *
// *    is a string containing the host name.                                  *
// *                                                                           *
// *  <excludedHosts>                const vector<string> &           In       *
// *    is a list of hosts that should not be used.                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true:  host name is on excluded list.   :(                                *
// * false: host name is not on the excluded list.                             *
// *                                                                           *
// *****************************************************************************
static bool isHostNameExcluded(
   const char *           hostName,
   const vector<string> & excludedHosts)

{

   for (size_t index = 0; index < excludedHosts.size(); index++)
      if (excludedHosts[index].compare(hostName) == 0)
         return true;
         
   return false;
   
}
//************************** End of isHostNameExcluded *************************


// *****************************************************************************
// *                                                                           *
// * Function: logConfigFileError                                              *
// *                                                                           *
// *    This function logs the error encountered while processing the          *
// *  LDAP connection config file (.traf_authentication_config).               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <fileCode>                     LDAPConfigFileErrorCode          In       *
// *    is a reference to an instance of a ConfigNodeContents object.          *
// *                                                                           *
// *  <lastLineNumber>                int                             In       *
// *    is the number of the last line that was read.                          *
// *                                                                           *
// *  <lastLine>                      string &                        In       *
// *    is the text of the last line that was read.                            *
// *                                                                           *
// *****************************************************************************
inline static void logConfigFileError(
   LDAPConfigFileErrorCode   fileCode,
   int                       lastLineNumber,
   string                  & lastLine)
   
{

char emsMsg[EMS_MSG_SIZE];

   switch (fileCode)
   {
      case LDAPConfigFile_OK:
         return;
         break;
      case LDAPConfigFile_NoFileProvided:
      case LDAPConfigFile_FileNotFound:
         sprintf(emsMsg, "****** .traf_authentication_config file not found\n");
         break;
      case LDAPConfigFile_BadAttributeName:
         sprintf(emsMsg,"****** Unrecognized attribute in .traf_authentication_config configuration file.  Line %d %s",
                 lastLineNumber,lastLine.c_str());
         break;
      case LDAPConfigFile_MissingValue:
         sprintf(emsMsg,"****** Missing required value in .traf_authentication_config configuration file.  Line %d %s",
                 lastLineNumber,lastLine.c_str());
         break;
      case LDAPConfigFile_ValueOutofRange:
         sprintf(emsMsg,"****** Value out of range in .traf_authentication_config configuration file.  Line %d %s",
                 lastLineNumber,lastLine.c_str());
         break;
      case LDAPConfigFile_CantOpenFile:
         sprintf(emsMsg,"****** Unable to open .traf_authentication_config configuration file");
         break;
      case LDAPConfigFile_CantReadFile:
         sprintf(emsMsg,"****** Unable to read .traf_authentication_config configuration file");
         break;
      case LDAPConfigFile_MissingCACERTFilename:
         sprintf(emsMsg,"****** TLS requested but no TLS CACERTFilename was provided");
         break;
      case LDAPConfigFile_MissingHostName:
         sprintf(emsMsg,"****** Missing host name in .traf_authentication_config");
         break;
      case LDAPConfigFile_MissingUniqueIdentifier:
         sprintf(emsMsg,"****** Missing unique identifier in .traf_authentication_config");
         break;
      case LDAPConfigFile_MissingSection:
         sprintf(emsMsg,"****** Missing directory server configuration in .traf_authentication_config");
         break;
      case LDAPConfigFile_ParseError:
         sprintf(emsMsg,"****** Internal error parsing .traf_authentication_config");
         break;
      case LDAPConfigFile_CantOpenLDAPRC:
         sprintf(emsMsg,"****** Unable to open .ldaprc to determine TLS CACERTFilename");
         break;
      case LDAPConfigFile_MissingLDAPRC:
         sprintf(emsMsg,"****** Missing .ldaprc and TLS_CACERTFilename not provided; cannot determine TLS CACERT filename");
         break;
      default:
         sprintf(emsMsg,"****** Error parsing .traf_authentication_config configuration file");
   }

   LOG_AUTH_EVENT(DBS_AUTH_CONFIG,emsMsg); 

}
//************************** End of logConfigFileError *************************

// *****************************************************************************
// *                                                                           *
// * Function: getNonExcludedHostName                                          *
// *                                                                           *
// *  This function gets a host name to use for LDAP operations.               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <hostName>                     const char *                     In       *
// *    is a string containing the host name.                                  *
// *                                                                           *
// *  <effectiveHostName>            char *                           Out      *
// *    passes back a string containing the effective host name.               *
// *                                                                           *
// *  <excludedHosts>                const vector<string> &           In       *
// *    is a list of hosts that should not be used.                            *
// *                                                                           *
// *  <isLoadBalanceHost>            bool                             In       *
// *    is true if <hostName> is a load balancing host.                        *
// *                                                                           *
// *  <shouldExcludeBadHosts>        bool                             In       *
// *    is true if the <effectiveHostName> should not be in <excludedHost>.    *
// *                                                                           *
// *  <retryCount>                   int                              In       *
// *    is the number of times a load balancer should be called to get a       *
// *  non-excluded host name if <shouldExcludeBadHost> is true.                *
// *                                                                           *
// *  <retryDelay>                   int                              In       *
// *    is the time (in seconds) to delay between retry the load balancer.     *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true:  a "good" host name was found.                                      *
// * false: Could not find a host name not on the exclude list. :(             *
// *                                                                           *
// *****************************************************************************
static bool getNonExcludedHostName(
   const char *           hostName,
   char *                 effectiveHostName,
   const vector<string> & excludedHosts,
   bool                   isLoadBalanceHost,
   bool                   shouldExcludeBadHosts,
   int                    retryCount,
   int                    retryDelay)
   
{

// If this is a load balance host (e.g. abc.com, get the effective 
// host name.  This name will then be used directly and any error messages
// will include the actual host name.  If not the load balancer, use the 
// host name that was passed in.
   if (isLoadBalanceHost)
      getEffectiveHostName(hostName,effectiveHostName);
   else
      strcpy(effectiveHostName,hostName);
 
   if (!shouldExcludeBadHosts)
      return true;
      
// If the effective host name is not on the list of excluded hosts, use it!
   if (!isHostNameExcluded(effectiveHostName,excludedHosts))
      return true;
      
// The host is on the excluded list.  If this is not a load balancing host 
// there is nothing that can be done here.  Return false and hope caller's 
// retry logic enables a successful connection.
   if (!isLoadBalanceHost)
      return false;

// Poke the load balancing host until we get a non-excluded name or 
// exhaust the retries.
   for (size_t i = 0; i < retryCount; i++)
   {
       sleep(retryDelay);
       getEffectiveHostName(hostName,effectiveHostName);
       if (isHostNameExcluded(effectiveHostName,excludedHosts))
          continue;
       return true;
   }
   
   return false;
   
}
//************************ End of getNonExcludedHostName ***********************


// *****************************************************************************
// *                                                                           *
// * Function: readLDAPConfigFile                                              *
// *                                                                           *
// *    Calls LDAPConfigFile class to read and parse the LDAP configuration    *
// *  file.  If successful, the results are cached and the current time is     *
// *  recorded.  If unsuccessful, an error is logged to stdout (and eventually *
// *  the repository.)                                                         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true:  LDAP config file was read and parsed successfully                  *
// * false: Error while reading/parsing LDAP config file                       *
// *                                                                           *
// *****************************************************************************
static bool readLDAPConfigFile()

{

string configFilename;
LDAPFileContents contents;
int lastLineNumber = -1;
string lastLine;
            
LDAPConfigFileErrorCode fileCode = configFile.read(configFilename,
                                                   config,
                                                   lastLineNumber,
                                                   lastLine);

   if (fileCode != LDAPConfigFile_OK)
   {
      logConfigFileError(fileCode,lastLineNumber,lastLine);
      return false;
   }
   
// Only update lastRefreshTime if reading and parsing was successful.
   lastRefreshTime = time(0);
   return true;

}
//************************** End of readLDAPConfigFile *************************


// *****************************************************************************
// *                                                                           *
// * Function: searchUser                                                      *
// *                                                                           *
// *  This function performs a LDAP user search based on the                   *
// *  input username, it also returns userDN, domainName, domainAttr           *
// *  about the user that was found.                                           *
// *  This funtion is called by ld_auth_user to look for the user first,       *
// *  then based on the information returned to perform user bind.             *
// *                                                                           *
// *  The processing steps of this function:                                   *
// *                                                                           *
// *  if UIF exists, parse domainName, userName                                *
// *                                                                           *
// *  if User Identifier exists                                                *
// *    Search by User Identifier, get DN                                       *
// *  else                                                                     *
// *    Search by Unique Identifier, get DN                                    *
// *                                                                           *
// *  if domainName entered by user                                            *
// *    find the node matches to domainName, do bind with DN                   *
// *  else                                                                     *
// *    if Domain Attribute exists                                             *
// *      Find the node matches to domainName in Domain Attribute,             *
// *       do bind with DN                                                     *
// *    else                                                                   *
// *      do bind with DN in highest priority node?                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <self>                          ConfigNodeContents &            In       *
// *    is a reference to a ConfigNodeContents.                                *
// *                                                                           *
// *  <inputName>                     const char *                    In       *
// *    is the name supplied by the user.                                      *
// *                                                                           *
// *  <userDN>                        string &                        Out      *
// *    passes back the distingushed name for the user.                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: LDSearchStatus                                                   *
// *                                                                           *
// *****************************************************************************
static LDSearchStatus searchUser(
   ConfigNodeContents   & self,
   const char           * inputName, 
   string               & userDN)
   
{

LDSearchStatus searchStatus = LDSearchNotFound; 
vector<string> uniqueIdentifiers;
string uniqueIdentifier;
string username = inputName;

   convertUsername(username);

   uniqueIdentifiers = self.host_->LDAPConfig_->uniqueIdentifier;
   for (int j = 0; j < uniqueIdentifiers.size(); j++)
   {
      uniqueIdentifier = uniqueIdentifiers[j];
      size_t pos = uniqueIdentifier.find('=', 0);  // look for =
      uniqueIdentifier.insert(pos + 1,username);    // insert username to make the DN
      searchStatus = searchUserByDN(self,uniqueIdentifier);
      if (searchStatus == LDSearchFound )
      {
         // user found
         userDN = uniqueIdentifier;
         return LDSearchFound ;
      }
      
      if (searchStatus == LDSearchNotFound)// continue to search
         continue;

// any other errors, stop the search and return
// LCOV_EXCL_START 
      return searchStatus;      
// LCOV_EXCL_STOP 
   }
   
   return searchStatus; 
    
}
//****************************** End of searchUser *****************************

// *****************************************************************************
// *                                                                           *
// * Function: searchUserByDN                                                  *
// *                                                                           *
// *  Performs a LDAP user search by calling LDAP search APIs.                 *
// *  The search call uses a DN as the search base.                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <self>                          ConfigNodeContents &            In       *
// *    is a reference to a ConfigNodeContents.                                *
// *                                                                           *
// *  <userDN>                        string &                        In       *
// *    is the distingushed name for the user.                                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:  error code                                                     *
// *                                                                           *
// *****************************************************************************
static LDSearchStatus searchUserByDN(
   ConfigNodeContents   & self,
   const string         & userDN)
   
{

int rc;
LDAP *ld;
LDAPMessage *res = NULL, *entry = NULL;
auto_ptr<LDAPMessage> test(NULL);
char *filter = NULL;
char *attrs[3];
char *attr, **vals;
BerElement *ptr = 0;
char createTimestamp[16];
char emsMsg[EMS_MSG_SIZE];

// Use "createTimestamp" as our "unique ID" for the user on the LDAP server.
   strcpy(createTimestamp, "createTimestamp");
   attrs[0] = createTimestamp;   
   attrs[1] = NULL;
 
// *****************************************************************************
// *                                                                           *
// *    When we search for the username on the LDAP server, there are four     *
// * possible error returns:                                                   *
// *                                                                           *
// *    1) Username is found (LDAP_SUCCESS)                                    *
// *       This is the most common. We continue on with authentication.        *
// *                                                                           *
// *    2) Username is not found (LDAP_NO_SUCH_OBJECT)                         *
// *       Could be a typo or someone trying to gain unauthorized access.      *
// *       We stop the authentication process.                                 *
// *                                                                           *
// *    3) Could not communicate with server (LDAP_SERVER_DOWN)                *
// *       Could be a transient problem, maybe a network issue.                *
// *       We retry once within this function, then if it fails                *
// *       again, log the error, and return an internal resource error.        *
// *       This triggers retry logic.                                          *
// *                                                                           *
// *    4) Some other error (e.g. LDAP_OTHER)                                  *
// *       We receive one of several dozen other possible errors.              *
// *       We log the error and return an internal resource error.             *
// *       This triggers retry logic.                                          *
// *                                                                           *
// *****************************************************************************

int reconnect = 1;

   while (true)
   {
      if (!selfCheck(self,true))
      {
         LOG_AUTH_EVENT(DBS_UNKNOWN_AUTH_STATUS_ERROR,"Self check failed in searchUserByDN");
      
         return LDSearchResourceFailure;
      }
      
      ld = self.searchLD_;  
      rc = ldap_search_ext_s(ld, 
                             userDN.c_str(),   // use the DN for base
                             LDAP_SCOPE_BASE,  // search only the base
                             filter,           // no filter
                             attrs,            // return attributes
                             0,                // get both attribute and value
                             NULL,             // server control
                             NULL,             // client control
                             NULL,             // timeout
                             0,                // result size limit
                             &res );           // search result
                             
      // The username was found on the LDAP server, break out of loop.  
      if (rc == LDAP_SUCCESS) 
         break;
      
      if (res != NULL)
         ldap_msgfree(res);
      
      // Username not found on the LDAP server; similar to bad password, we
      // do not retry this error.  
      // Return and report to the user - No soup for you.
      if (rc == LDAP_NO_SUCH_OBJECT)
         return LDSearchNotFound;
      
      //Retry server down error once.   
      if (rc == LDAP_SERVER_DOWN && reconnect != 0) 
      {
         // reconnect & retry
         closeConnection(self);
         LD_Status status = initConnection(self,NULL,true);
         // If the reconnect failed, report the resource error and return a 
         // resource failure error so we will retry per configuration settings.
         if (status != LD_STATUS_OK)
         {
            sprintf(emsMsg, "LDAP search error.   Unable to connect to server\n");  
            LOG_AUTH_EVENT(DBS_LDAP_SEARCH_ERROR,emsMsg);
            return LDSearchResourceFailure;
         }
         reconnect--;
         continue;            
      }

      // For all other search errors, report an error and return a resource 
      // failure (LDAP server or network problem) so we will retry per
      // configuration settings.   
      sprintf(emsMsg, "LDAP search error.  Error code: %d, %s\n", 
                      rc, ldap_err2string(rc)); 
      LOG_AUTH_EVENT(DBS_LDAP_SEARCH_ERROR,emsMsg);
      return LDSearchResourceFailure;
   }
   
// Check the number of entries found.  We only need one.  If none were
// returned, the user is not defined on the directory server.
int numberFound = ldap_count_entries(ld,res);

   if (numberFound <= 0) 
   {
      if (res != NULL)
         ldap_msgfree(res);

      return LDSearchNotFound;
   }

// Get the first entry returned.  If there was no entry returned, that means
// the user is not defined on the directory server. We already check for 
// number of entries, but let's be extra careful.
   entry = ldap_first_entry(ld, res);
   if (entry == NULL)
   {
      //this should not happen
      if (res != NULL)
         ldap_msgfree(res);

      // log error message
      return LDSearchNotFound;
   }

   attr = ldap_first_attribute(ld, entry, &ptr);
   if (attr == NULL)
   {
      //this should not happen
      if (res != NULL)
         ldap_msgfree(res);

      // log error message
      sprintf(emsMsg, "LDAP search error.   Attribute %s does not exist in the entry %s", attrs[0], userDN.c_str());  
      LOG_AUTH_EVENT(DBS_LDAP_SEARCH_ERROR,emsMsg);
      return LDSearchResourceFailure;
   }

// OpenLDAP allocates memory and then requires the caller to free it using
// specific OpenLDAP APIs.  

   ldap_memfree(attr);

   if (ptr != NULL)
      ber_free(ptr, 0);
   
   if (res != NULL)
      ldap_msgfree(res);
   
   return LDSearchFound;
  
}
//*************************** End of searchUserByDN ****************************

// *****************************************************************************
// *                                                                           *
// * Function: selfCheck                                                       *
// *                                                                           *
// *    Determines if this instance is internally consistent.  Based on the    *
// * node type and the status, dependent values are checked.                   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <self>                          ConfigNodeContents &            In       *
// *    is a reference to a ConfigNodeContents.                                *
// *                                                                           *
// *  <isInitialized>                 bool                            In       *
// *    if true, instance must be initialized to be consistent.                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns:                                                                 *
// *                                                                           *
// *  TRUE: Node is consistent                                                 *
// *  FALSE: Node is inconsistent                                              *
// *                                                                           *
// *****************************************************************************
static bool selfCheck(
   ConfigNodeContents   & self,
   bool                   isInitialized)
   
{

//
// We only support two configurations.  Got to be one or the other.
//

   if (self.configType_ != LDAPConfigNode::PrimaryConfiguration &&
       self.configType_ != LDAPConfigNode::SecondaryConfiguration)
      return false;

//
// We only support two types of connections.  Got to be one or the other.
//

   if (self.connectionType_ != AuthenticationConnection &&
       self.connectionType_ != SearchConnection)
      return false;

//
// If the node has not (yet) been initialized, that is a consistent state.
// However, if the caller expects the node to have been initialized, we have problem.
//

   if (self.status_ == NODE_NOT_INITIALIZED)
   {
      if (isInitialized)
         return false;
      else
         return true;
   }
     
//
// There are only two states.  Anything else is a problem. This catches cases
// where the class data has been overwritten.
//

   if (self.status_ != NODE_ACTIVE)      
      return false;
      
//
// We have an active node.  For active nodes, the ld for the connection type
// must be setup.  
//  

   if (self.connectionType_ == AuthenticationConnection && self.authLD_ == NULL)
      return false; 
   if (self.connectionType_ == SearchConnection && self.searchLD_ == NULL)
      return false; 
      
// 
// All data members are consistent with an active state.  Check the 
// LDAP Config for consistency.
//   
   return self.host_->LDAPConfig_->selfCheck(isInitialized);
      
}
//***************************** End of selfCheck *******************************


// *****************************************************************************
// *                                                                           *
// * Function: shouldReadLDAPConfig                                            *
// *                                                                           *
// *  Determines if the LDAP config file needs to be (re)read or if we can use *
// *  the data we have already read.                                           *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true:  LDAP config file should be read                                    *
// * false: Use existing data                                                  *
// *                                                                           *
// *****************************************************************************
inline static bool shouldReadLDAPConfig()

{

// If we have never successfully read and parsed the LDAP configuration file,
// then we need to read.
   if (lastRefreshTime == 0)
      return true;

// We have previously read the file.  See if we need to refresh.
      
// Zero indicates never refresh, so don't read.
   if (config.refreshTime == 0)
      return false;
      
// If not enough time has elapsed since we last read the LDAP config file 
// then we don't need to re-read the file.
   if (time(0) - lastRefreshTime < config.refreshTime)
      return false;

// Time to freshen up! 
   return true;

}
//************************ End of shouldReadLDAPConfig *************************

