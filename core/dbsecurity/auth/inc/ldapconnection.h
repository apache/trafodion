#if ! defined(LDAPCONNECTION_H)
#define LDAPCONNECTION_H

//*********************************************************************
//*                                                                   *
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
//*                                                                   *
//*********************************************************************
#include "ldapconfignode.h"
#include "ldapconfigparam.h"

#pragma page "Class LdapConnection"
// *****************************************************************************
// *                                                                           *
// *  Class       LdapConnection                                               *
// *                                                                           *
// *              This class represents server connections to LDAP serves.     *
// *              It contains a list of server nodes, LdapConfigNode.          *
// *              This class loads the server configuration info from CONFIG   *
// *              table, then open a LDAP connection for each servers          *
// *                                                                           *
// *  Qualities                                                                *
// *              Abstract:    No                                              *
// *              Assignable:  Yes                                             *
// *              Copyable:    Yes                                             *
// *              Derivable:   Yes                                             *
// *                                                                           *
// *****************************************************************************
class LdapConnection
{
  public:
    LdapConnection();
    LdapConnection( const LdapConnection & other );
    LdapConnection & operator = ( const LdapConnection & other );
    virtual ~LdapConnection();

    int openConnection();
    int closeConnection();
    int updateConnection();
    int reopenConnection();
    void checkLDAPConfig(void); 

    int addDefaultConfig();
    int prepareConfigTxt();
    int initConnection();

    void addNode( LdapConfigNode *configNode ) 
      { ldapConfigNodes_.push_back(configNode); };

    vector<LdapConfigNode *> * getLdapConfigNodes() 
      { return &ldapConfigNodes_; }

    LdapConfigNode * getDefaultConfigNode() 
      { return defaultConfigNode_; };

    LdapConfigNode * getSearchUserIdConfigNode() 
      { return searchUserIdConfigNode_; };

    vector<LdapConfigNode *> * getSearchUniqueIdConfigNode() 
      { return &searchUniqueIdConfigNode_; };

    LdapConfigNode * getAuthConfigNode() 
      { return authConfigNode_; };

    LdapConfigNode * getUserFoundNode() 
      { return userFoundNode_; };

    short getTotalActiveNodes() 
      { return totalActiveNodes_; };

    int setDefaultConfigNode();
    int setSearchConfigNode();
    int setAuthConfigNode();
    
    void setUserFoundNode(LdapConfigNode *node)
      { userFoundNode_ = node; }

    bool isValid()
      { return isValid_; } 

  private:
    bool isValid_;  // true iff this object represents a valid connection
    vector<LdapConfigNode *> ldapConfigNodes_; 
    LdapConfigNode *defaultConfigNode_;
    LdapConfigNode *searchUserIdConfigNode_;
    vector<LdapConfigNode *> searchUniqueIdConfigNode_;
    LdapConfigNode *authConfigNode_;
    LdapConfigNode *userFoundNode_;

    short totalActiveNodes_;
    long long lastUpdateTimestamp_;

    int loadConfigParams();
    int loadConfigNodes();
    int checkConfigNodes();
    int searchAuthGroup(LdapConfigNode *node);
};

#endif
