/* @@@ START COPYRIGHT @@@
**
** (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**      http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
**
** @@@ END COPYRIGHT @@@ */

#ifndef WRAPPER_ROUTINGKEY
#define WRAPPER_ROUTINGKEY


#include <string>

#ifndef WIN32
#define SP_DLL_EXPORT
#else
#define SP_DLL_EXPORT __declspec(dllexport)
#endif

#define SP_SUCCESS 0
#define SP_CONTENT_TYPE_APP  "application/x-protobuf"
#define SP_ROUTING_KEY_PARTS 6

using std::string;

enum SP_PublicationScopeType
  {
    SP_NULLSCOPE,
    SP_CLUSTER,
    SP_INSTANCE,
    SP_TENANT,
    SP_MAXSCOPE
  };

enum SP_PublicationSecurityType
  {
    SP_NULLSECURITY,
    SP_PUBLIC,
    SP_PRIVATE,
    SP_MAXSECURITY
  };

enum SP_PublicationCategoryType
  {
    SP_NULLCATEGORY,
    SP_EVENT,
    SP_PERF_STAT,
    SP_HEALTH_STATE,
    SP_SECURITY,
    SP_MAXCATEGORY
  };

enum SP_PublicationProtocolType
  {
    SP_NULLPROTOCOL,
    SP_GPBPROTOCOL,
    SP_XMLPROTOCOL,
    SP_MAXPROTOCOL
  };


//**********************************************************************
//
// Ahoy there!  This is your SeaPilot speaking....
//
//  Adding a new package???  Welcome aboard!  It's easy to add a new
//  package.  There are 3 simple steps:
//
//   1)  Add the enum below, right before SP_PACKAGE_MAX
//   2)  Add the text literal below
//   3)  Add the text literal name to the const string
//       publicationPackageText array before SP_MAX_PUBLICATION_TEXT.
//
//  Note that there is a positional 1-1 relationship between
//  the enum SP_PublicationPackageType and the string array
//  publicationPackageText. If you add an entry to one, you must
//  add the corresponding entry to the other AT THE SAME POSITION.
//
//  Did you come here because you had an compiler error in
//  checking the package array size and you're not sure why?
//  That means you didn't add it correctly, because the number of
//  entries in the text array doesn't match the MAX below.
//
//**********************************************************************

enum SP_PublicationPackageType
  {
    SP_NULLPACKAGE,
    SP_COMMONPACKAGE,
    SP_DTMPACKAGE,
    SP_INCIDENTANALYSISPACKAGE,
    SP_LIGHTHOUSEPACKAGE,
    SP_MONITORPACKAGE,
    SP_NDCSPACKAGE,
    SP_SEPACKAGE,
    SP_SUMMARYPACKAGE,
    SP_UNCPACKAGE,
    SP_SQLPACKAGE,
    SP_SPAPACKAGE,
    SP_NVTSVCPACKAGE,
    SP_PROBLEMMANAGEMENTPACKAGE,
    SP_LUNPACKAGE,
    SP_SSDPACKAGE,
    SP_MANAGEABILITY_INFRASTRUCTUREPACKAGE,
    SP_STATEPACKAGE,
    SP_MANAGEABILITY_TESTPACKAGE,
    SP_WMSPACKAGE,
    SP_TPGPACKAGE,
    SP_CHAMPACKAGE,
    SP_ACTIONPACKAGE,
    SP_OSPACKAGE,
    SP_LINUXCOUNTERSPACKAGE,
    SP_ACCESSLAYERPACKAGE,
    SP_DATABASELAYERPACKAGE,
    SP_FOUNDATIONLAYERPACKAGE,
    SP_OSLAYERPACKAGE,
    SP_SERVERLAYERPACKAGE,
    SP_STORAGELAYERPACKAGE,
    SP_DBSPACKAGE,             // DB Security
    SP_COMPRESSIONPACKAGE,
    //Add new package types above this line!
    SP_PACKAGEMAX
  };

// You might have noticed that there are similar string defines
// in the externroutingkey.h file. The strings here are for
// internal use, while the strings there are external. They may
// diverge over time.

#define SP_NULLPACKAGE_TEXT                          "nullpackagetext"
#define SP_COMMONPACKAGE_TEXT                        "common"
#define SP_DTMPACKAGE_TEXT                           "dtm"
#define SP_INCIDENTANALYSISPACKAGE_TEXT              "incidentanalysis"
#define SP_LIGHTHOUSEPACKAGE_TEXT                    "lighthouse"
#define SP_MONITORPACKAGE_TEXT                       "monitor"
#define SP_NDCSPACKAGE_TEXT                          "ndcs"
#define SP_SEPACKAGE_TEXT                            "se"
#define SP_SUMMARYPACKAGE_TEXT                       "summary"
#define SP_UNCPACKAGE_TEXT                           "unc"
#define SP_SQLPACKAGE_TEXT                           "sql"
#define SP_SPAPACKAGE_TEXT                           "space"
#define SP_NVTSVCPACKAGE_TEXT                        "nvtsvc"
#define SP_PROBLEMMANAGEMENTPACKAGE_TEXT             "problem_management"
#define SP_LUNPACKAGE_TEXT                           "lun"
#define SP_SSDPACKAGE_TEXT                           "ssd"
#define SP_MANAGEABILITY_INFRASTRUCTUREPACKAGE_TEXT  "manageability_infrastructure"
#define SP_STATEPACKAGE_TEXT                         "state"
#define SP_MANAGEABILITY_TESTPACKAGE_TEXT            "manageability_test"
#define SP_WMSPACKAGE_TEXT                           "wms"
#define SP_TPGPACKAGE_TEXT                           "tpg"
#define SP_CHAMPACKAGE_TEXT                          "chameleon"
#define SP_ACTIONPACKAGE_TEXT                        "action"
#define SP_OSPACKAGE_TEXT                            "os"
#define SP_LINUXCOUNTERSPACKAGE_TEXT                 "linuxcounters"
#define SP_ACCESSLAYERPACKAGE_TEXT                   "accesslayer"
#define SP_DATABASELAYERPACKAGE_TEXT                 "databaselayer"
#define SP_FOUNDATIONLAYERPACKAGE_TEXT               "foundationlayer"
#define SP_OSLAYERPACKAGE_TEXT                       "oslayer"
#define SP_SERVERLAYERPACKAGE_TEXT                   "serverlayer"
#define SP_STORAGELAYERPACKAGE_TEXT                  "storagelayer"
#define SP_DBSPACKAGE_TEXT                           "dbs"
#define SP_COMPRESSIONPACKAGE_TEXT                   "compression"
// Add your new package above this line!
#define SP_PACKAGEMAX_TEXT                           "maxpackagetext"


const static string publicationPackageText[SP_PACKAGEMAX+1] = {
    SP_NULLPACKAGE_TEXT,
    SP_COMMONPACKAGE_TEXT,
    SP_DTMPACKAGE_TEXT,
    SP_INCIDENTANALYSISPACKAGE_TEXT,
    SP_LIGHTHOUSEPACKAGE_TEXT,
    SP_MONITORPACKAGE_TEXT,
    SP_NDCSPACKAGE_TEXT,
    SP_SEPACKAGE_TEXT,
    SP_SUMMARYPACKAGE_TEXT,
    SP_UNCPACKAGE_TEXT,
    SP_SQLPACKAGE_TEXT,
    SP_SPAPACKAGE_TEXT,
    SP_NVTSVCPACKAGE_TEXT,
    SP_PROBLEMMANAGEMENTPACKAGE_TEXT,
    SP_LUNPACKAGE_TEXT,
    SP_SSDPACKAGE_TEXT,
    SP_MANAGEABILITY_INFRASTRUCTUREPACKAGE_TEXT,
    SP_STATEPACKAGE_TEXT,
    SP_MANAGEABILITY_TESTPACKAGE_TEXT,
    SP_WMSPACKAGE_TEXT,
    SP_TPGPACKAGE_TEXT,
    SP_CHAMPACKAGE_TEXT,
    SP_ACTIONPACKAGE_TEXT,
    SP_OSPACKAGE_TEXT,
    SP_LINUXCOUNTERSPACKAGE_TEXT,
    SP_ACCESSLAYERPACKAGE_TEXT,
    SP_DATABASELAYERPACKAGE_TEXT,
    SP_FOUNDATIONLAYERPACKAGE_TEXT,
    SP_OSLAYERPACKAGE_TEXT,
    SP_SERVERLAYERPACKAGE_TEXT,
    SP_STORAGELAYERPACKAGE_TEXT,
    SP_DBSPACKAGE_TEXT,
    SP_COMPRESSIONPACKAGE_TEXT,
    //Add new package types above this line!
    SP_PACKAGEMAX_TEXT
};

//********************************************************************************
//  Library Method AMQPRoutingKey_BuildSubscriptionRoutingKey
//
//  This library method can be used to build a subscription key
//  suitable for setting up a queue based on the specified routing key
//  components.  Not part of the AMQPRoutingKey class because there's
//  not really a routing key to construct.
//
//*******************************************************************************

SP_DLL_EXPORT string AMQPRoutingKey_BuildSubscriptionRoutingKey
                                    ( SP_PublicationCategoryType category,
                      SP_PublicationPackageType package,
                      SP_PublicationScopeType scope,
                      SP_PublicationSecurityType security,
                      SP_PublicationProtocolType protocol,
                      const string &publicationName );


//********************************************************************************
//
//  Class AMQPRoutingKey
//
//  This class encapsulates a publication routing key and provides methods to
//  construct it, decompose it, and map it to related constructs such as the
//  associated proto buff file.
//
//********************************************************************************

class SP_DLL_EXPORT AMQPRoutingKey
{

 public:

  //******************************************************************************
  //
  // Standard c'tor, d'tor
  //
  //******************************************************************************

  AMQPRoutingKey();
  ~AMQPRoutingKey() {}

  //*****************************************************************************
  //  Parameterized Constructors
  //*****************************************************************************

  //String routing key parameter.
  AMQPRoutingKey (const string &routingKey);

  //Routing Key Components
  AMQPRoutingKey (SP_PublicationCategoryType category,
          SP_PublicationPackageType package,
          SP_PublicationScopeType scope,
          SP_PublicationSecurityType security,
          SP_PublicationProtocolType protocol,
          const string &publicationName );


  //*******************************************************************************
  // getters/setters
  //********************************************************************************

  SP_PublicationPackageType GetPackage ();
  string GetPackageString();
  SP_PublicationCategoryType GetCategory ();
  string GetCategoryString ();
  SP_PublicationScopeType GetScope ();
  string  GetScopeString ();
  SP_PublicationSecurityType GetSecurity ();
  string GetSecurityString ();
  SP_PublicationProtocolType GetProtocol();
  string  GetProtocolString();
  string GetPublicationName();

  string GetAsProtofileName();  // msg name includes protofile extension
  string GetAsMessageName();    // msg name as found in protofile
                // (package and publication name

  string GetAsString ();

  void SetFromString (const string &routingKeyString);
  void SetFromComponents (  SP_PublicationCategoryType category,
                SP_PublicationPackageType package,
                SP_PublicationScopeType scope,
                SP_PublicationSecurityType security,
                SP_PublicationProtocolType protocol,
                const string &publicationName);

  // Right now we don't have individual setters.  We could, but to do
  // so in a relatively high-performing way, we'd need to store the
  // individual routing key components in as member variables.
  // Otherwise, we need to parse out each component of the routing key
  // and then rebuild it with the single item changed.  If there's a
  // demand for it, then we can go, but for now, storing just the
  // string saves memory.  Life is full of tradeoffs....

  //***** proto file name doesn't currently include type/scope/security, so this
  //***** doesn't result in a valid routing key
  void SetFromProtofileName(const string &protoFileName);  //includes protofile extension



 private:
  bool isValid();      // Internal Validation

  int GetNumLevels();  // Returns the number of levels in the current
               // routing key


  void GetRoutingKeyComponents (string *category, string *package, string *scope,
                string *security, string *protocol,
                string *publicationName);

  string GetNextLevel (const string &scanstring, size_t startPoint);

  string m_routingKey_;

  int    m_numLevels_;

};

#endif
