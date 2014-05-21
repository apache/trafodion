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

#ifndef WRAPPER_EXTERNROUTINGKEY
#define WRAPPER_EXTERNROUTINGKEY


#include <string>

#ifndef WIN32
#define SP_DLL_EXPORT
#else
#define SP_DLL_EXPORT __declspec(dllexport)
#endif

//************************************************************************
//
//  ExternRoutingKey
//
//  This file contains the externals for ExternRoutingKey, which
//  translates from external strings into an internal AMQP routing
//  key.  These strings are intended to be used in external
//  interfaces.  The spelling for these strings might diverge from the
//  spellings of the routing key components in the internal
//  AMQPRoutingKey class. The purpose of this class is to build a
//  stable set of routing key components that are stable across any
//  spelling and order changes in the internal routing key.
//
//  This library contains the following:
//
//    o Public Routing Key String Literals
//
//    o Translator routines to translate complete external strings to
//        internal routing keys
//
//    o Translator routines to translate external strings to internal
//        subscription strings
//
//    o Error->Text function to translate error codes to a string
//          text suitable for user consumption
//
//  This is a set of library functions that currently has no persistent
//  members.
//
//  A word on error handling.  These library routines return an error
//  of the type SPExtRoutingKeyErr.  An error can be translated using
//  the ExternRoutingKey_ErrToText library call.  This is a stopgap
//  until we decide how we *really* want to handle library errors.
//
//************************************************************************

//**********************************
//  External Routing Key Component Separator
//**********************************
#define EXTROUTINGKEY_LEVELSEPARATOR "."
#define EXTROUTINGKEY_LEVELSEPARATOR_LEN 1

//**********************************
//
//  External Component Group Types
//
//**********************************

enum spExtRoutingKeyComponentGroupType
{
  SPERKCOMPGROUP_NONE,
  SPERKCOMPGROUP_CATEGORY,
  SPERKCOMPGROUP_PACKAGE,
  SPERKCOMPGROUP_SCOPE,
  SPERKCOMPGROUP_SECURITY,
  SPERKCOMPGROUP_PROTOCOL,
  SPERKCOMPGROUP_PUBLICATIONNAME,
  SPERKCOMPGROUP_MAXCOMPONENT
};

//**************************************************************************
//
//  External Component Types
//
//  Important note: The enums defined here are in 1-1 correspondence
//  with the entries in the extStringText static array defined below.
//  If you add an enum here, make sure you add an entry in extStringText
//  AT THE SAME POSITION.
//
//  Important note: The semantics for determining what kind of item
//  a given string or literal represent are implemented in
//  seapilot/source/wrapper/externroutingkey.cpp. When you add a literal
//  here, there is doubtless a switch statement in externroutingkey.cpp
//  that needs a new case for your literal. Be sure to read that module
//  and add the appropriate cases there. You'll discover too while doing
//  this that you need to make changes to seapilot/source/wrapper/routingkey.h
//  as well.
//
//**************************************************************************
enum spExtRoutingKeyCompType
{
  SPERK_NULLCOMPONENT,
  SPERK_NULLPACKAGE,
  SPERK_COMMONPACKAGE,
  SPERK_DTMPACKAGE,
  SPERK_INCIDENTANALYSISPACKAGE,
  SPERK_LIGHTHOUSEPACKAGE,
  SPERK_MONITORPACKAGE,
  SPERK_NDCSPACKAGE,
  SPERK_SEPACKAGE,
  SPERK_SUMMARYPACKAGE,
  SPERK_UNCPACKAGE,
  SPERK_SQLPACKAGE,
  SPERK_SPAPACKAGE,
  SPERK_NVTSVCPACKAGE,
  SPERK_PROBLEMMANAGEMENTPACKAGE,
  SPERK_NULLSCOPE,
  SPERK_CLUSTERSCOPE,
  SPERK_INSTANCESCOPE,
  SPERK_TENANTSCOPE,
  SPERK_NULLSECURITY,
  SPERK_PUBLICSECURITY,
  SPERK_PRIVATESECURITY,
  SPERK_NULLCATEGORY,
  SPERK_EVENTCATEGORY,
  SPERK_PERF_STATCATEGORY,
  SPERK_HEALTH_STATECATEGORY,
  SPERK_SECURITYCATEGORY,
  SPERK_NULLPROTOCOL,
  SPERK_GPBPROTOCOL,
  SPERK_XMLPROTOCOL,
  SPERK_LUNPACKAGE,
  SPERK_SSDPACKAGE,
  SPERK_MANAGEABILITY_INFRASTRUCTUREPACKAGE,
  SPERK_STATEPACKAGE,
  SPERK_MANAGEABILITY_TESTPACKAGE,
  SPERK_WMSPACKAGE,
  SPERK_TPGPACKAGE,
  SPERK_CHAMPACKAGE,
  SPERK_ACTIONPACKAGE,
  SPERK_OSPACKAGE,
  SPERK_LINUXCOUNTERSPACKAGE,
  SPERK_ACCESSLAYERPACKAGE,
  SPERK_DATABASELAYERPACKAGE,
  SPERK_FOUNDATIONLAYERPACKAGE,
  SPERK_OSLAYERPACKAGE,
  SPERK_SERVERLAYERPACKAGE,
  SPERK_STORAGELAYERPACKAGE,
  SPERK_DBSPACKAGE,                         // DB Security
  SPERK_COMPRESSIONPACKAGE,
  //  Add new components above this line! They *DO* *NOT* have to be
  //  grouped by component!
  SPERK_MAXCOMPONENTTYPE
};

//**********************************
//
//  Component Text Strings Revealed
//
//**********************************

// If it ain't here, it must be a publication name.  Or a typo!

// Right now there are some 26 components, so order doesn't
// matter so much -- 27 string compares at worst.  Still and all, it
// would be better to build a list with some order in it for
// searching....

#define SPERK_NULLCOMPONENT_TEXT						"nullcomponent"
#define SPERK_NULLPACKAGE_TEXT							"nullpackagetext"
#define SPERK_COMMONPACKAGE_TEXT						"common"
#define SPERK_DTMPACKAGE_TEXT							"dtm"
#define SPERK_INCIDENTANALYSISPACKAGE_TEXT				"incidentanalysis"
#define SPERK_LIGHTHOUSEPACKAGE_TEXT					"lighthouse"
#define SPERK_MONITORPACKAGE_TEXT						"monitor"
#define SPERK_NDCSPACKAGE_TEXT							"ndcs"
#define SPERK_SEPACKAGE_TEXT							"se"
#define SPERK_SUMMARYPACKAGE_TEXT						"summary"
#define SPERK_UNCPACKAGE_TEXT							"unc"
#define SPERK_SQLPACKAGE_TEXT							"sql"
#define SPERK_SPAPACKAGE_TEXT							"space"
#define SPERK_NVTSVCPACKAGE_TEXT						"nvtsvc"
#define SPERK_PROBLEMMANAGEMENTPACKAGE_TEXT             "problem_management"
#define SPERK_NULLSCOPE_TEXT							"nullscopetext"
#define SPERK_CLUSTERSCOPE_TEXT							"cluster"
#define SPERK_INSTANCESCOPE_TEXT						"instance"
#define SPERK_TENANTSCOPE_TEXT							"tenant"
#define SPERK_NULLSECURITY_TEXT							"nullsecuritytext"
#define SPERK_PUBLICSECURITY_TEXT						"public"
#define SPERK_PRIVATESECURITY_TEXT						"private"
#define SPERK_NULLCATEGORY_TEXT							"nullcategorytext"
#define SPERK_EVENTCATEGORY_TEXT						"event"
#define SPERK_PERF_STATCATEGORY_TEXT					"performance_stat"
#define SPERK_HEALTH_STATECATEGORY_TEXT					"health_state"
#define SPERK_SECURITYCATEGORY_TEXT                     "security"
#define SPERK_NULLPROTOCOL_TEXT							"nullprotocoltext"
#define SPERK_GPBPROTOCOL_TEXT							"gpb"
#define SPERK_XMLPROTOCOL_TEXT							"xml"
#define SPERK_MAXCOMPONENTTYPE_TEXT						"maxcomponent"
#define SPERK_LUNPACKAGE_TEXT							"lun"
#define SPERK_SSDPACKAGE_TEXT							"ssd"
#define SPERK_MANAGEABILITY_INFRASTRUCTUREPACKAGE_TEXT	"manageability_infrastructure"
#define SPERK_STATEPACKAGE_TEXT                         "state"
#define SPERK_MANAGEABILITY_TESTPACKAGE_TEXT            "manageability_test"
#define SPERK_WMSPACKAGE_TEXT                           "wms"
#define SPERK_TPGPACKAGE_TEXT                           "tpg"
#define SPERK_CHAMPACKAGE_TEXT                          "chameleon"
#define SPERK_ACTIONPACKAGE_TEXT                        "action"
#define SPERK_OSPACKAGE_TEXT                            "os"
#define SPERK_LINUXCOUNTERSPACKAGE_TEXT                 "linuxcounters"
#define SPERK_ACCESSLAYERPACKAGE_TEXT                   "accesslayer"
#define SPERK_DATABASELAYERPACKAGE_TEXT                 "databaselayer"
#define SPERK_FOUNDATIONLAYERPACKAGE_TEXT               "foundationlayer"
#define SPERK_OSLAYERPACKAGE_TEXT                       "oslayer"
#define SPERK_SERVERLAYERPACKAGE_TEXT                   "serverlayer"
#define SPERK_STORAGELAYERPACKAGE_TEXT                  "storagelayer"
#define SPERK_DBSPACKAGE_TEXT                           "dbs"
#define SPERK_COMPRESSIONPACKAGE_TEXT                   "compression"

// Map strings to enums

//  Important note: The array elements defined here are in 1-1
//  correspondence with the enum spExtRoutingKeyCompType symbols
//  defined above. If you add an entry here, make sure you add a
//  new symbol in the enum AT THE SAME POSITION.

static const string extStringText [SPERK_MAXCOMPONENTTYPE+1] = {
  SPERK_NULLCOMPONENT_TEXT,
  SPERK_NULLPACKAGE_TEXT,
  SPERK_COMMONPACKAGE_TEXT,
  SPERK_DTMPACKAGE_TEXT,
  SPERK_INCIDENTANALYSISPACKAGE_TEXT,
  SPERK_LIGHTHOUSEPACKAGE_TEXT,
  SPERK_MONITORPACKAGE_TEXT,
  SPERK_NDCSPACKAGE_TEXT,
  SPERK_SEPACKAGE_TEXT,
  SPERK_SUMMARYPACKAGE_TEXT,
  SPERK_UNCPACKAGE_TEXT,
  SPERK_SQLPACKAGE_TEXT,
  SPERK_SPAPACKAGE_TEXT,
  SPERK_NVTSVCPACKAGE_TEXT,
  SPERK_PROBLEMMANAGEMENTPACKAGE_TEXT,
  SPERK_NULLSCOPE_TEXT,
  SPERK_CLUSTERSCOPE_TEXT,
  SPERK_INSTANCESCOPE_TEXT,
  SPERK_TENANTSCOPE_TEXT,
  SPERK_NULLSECURITY_TEXT,
  SPERK_PUBLICSECURITY_TEXT,
  SPERK_PRIVATESECURITY_TEXT,
  SPERK_NULLCATEGORY_TEXT,
  SPERK_EVENTCATEGORY_TEXT,
  SPERK_PERF_STATCATEGORY_TEXT,
  SPERK_HEALTH_STATECATEGORY_TEXT,
  SPERK_SECURITYCATEGORY_TEXT,
  SPERK_NULLPROTOCOL_TEXT,
  SPERK_GPBPROTOCOL_TEXT,
  SPERK_XMLPROTOCOL_TEXT,
  SPERK_LUNPACKAGE_TEXT,
  SPERK_SSDPACKAGE_TEXT,
  SPERK_MANAGEABILITY_INFRASTRUCTUREPACKAGE_TEXT,
  SPERK_STATEPACKAGE_TEXT,
  SPERK_MANAGEABILITY_TESTPACKAGE_TEXT,
  SPERK_WMSPACKAGE_TEXT,
  SPERK_TPGPACKAGE_TEXT,
  SPERK_CHAMPACKAGE_TEXT,
  SPERK_ACTIONPACKAGE_TEXT,
  SPERK_OSPACKAGE_TEXT,
  SPERK_LINUXCOUNTERSPACKAGE_TEXT,
  SPERK_ACCESSLAYERPACKAGE_TEXT,
  SPERK_DATABASELAYERPACKAGE_TEXT,
  SPERK_FOUNDATIONLAYERPACKAGE_TEXT,
  SPERK_OSLAYERPACKAGE_TEXT,
  SPERK_SERVERLAYERPACKAGE_TEXT,
  SPERK_STORAGELAYERPACKAGE_TEXT,
  SPERK_DBSPACKAGE_TEXT,
  SPERK_COMPRESSIONPACKAGE_TEXT,
  // add new entries above this one -- ORDER MATTERS!!!!!!
  SPERK_MAXCOMPONENTTYPE_TEXT
};
// Build mapping table here


//**********************************
//
//  Error related stuff:  Mister/Madame SPERKERR!
//
//**********************************

enum SPExtRoutingKeyErr
{
   SPERKERR_NOERR,                    // No error
   SPERKERR_TOOMANYCOMPONENTS,        // Too many components
   SPERKERR_NOMEM,                    // Memory Allocation error
   SPERKERR_TOOMANYFREESTRINGS,       // More than one unrecognized tring
   SPERKERR_REPEATEDCOMPONENT,        // Component level specified more than once
   SPERKERR_ERROUTOFBOUNDS,           // To-Text routine got an invalid error code.  Link err or bug.

   // Add new errors above this line!
   SPERKERR_LASTERR                   // Final Error
 };

 #define SPERKERR_NOERR_TEXT "NO ERROR"
 #define SPERKERR_TOOMANYCOMPONENTS_TEXT "TOO MANY COMPONENTS SPECIFIED"
 #define SPERKERR_NOMEM_TEXT "MEMORY ALLOCATION ERROR"
 #define SPERKERR_TOOMANYFREESTRINGS_TEXT "TOO MANY FREE STRINGS SPECIFIED"
 #define SPERKERR_REPEATEDCOMPONENT_TEXT "COMPONENT REPEATED IN EXTERNAL STRING"
 #define SPERKERR_ERROUTOFBOUNDS_TEXT "TEXT TRANSLATOR ERROR CODE OUT OF BOUNDS"

 // Add new errors above this line!
 #define SPERKERR_LASTERR_TEXT "ERROR OUT OF BOUNDS:  UNKNOWN ERROR"

 const static string spERKErrText [ SPERKERR_LASTERR+1] = {
   SPERKERR_NOERR_TEXT,
   SPERKERR_TOOMANYCOMPONENTS_TEXT,
   SPERKERR_NOMEM_TEXT,
   SPERKERR_TOOMANYFREESTRINGS_TEXT,
   SPERKERR_REPEATEDCOMPONENT_TEXT,
   SPERKERR_ERROUTOFBOUNDS_TEXT,

   // Add new errors above this line!
   SPERKERR_LASTERR_TEXT
 };

 //********************************************************************
 //**** Compares an external RK to another. 
 //********************************************************************
 SP_DLL_EXPORT bool ExtRoutingKey_Compare(const string &externRoutingKey1,
                                          const string &externRoutingKey2);

 //********************************************************************
 //**** Converts an external RK to a complete internal RK, overwriting
 //**** any previous contents
 //********************************************************************
 SP_DLL_EXPORT SPExtRoutingKeyErr ExtRoutingKey_ToInternal (string externRoutingKey,
                          AMQPRoutingKey *internRoutingKey);

 //********************************************************************
 //**** Converts an external RK to a complete internal subscription
 //**** key, overwriting any previous contents.  Missing components are
 //**** defaulted using # and * as appropriate
 //********************************************************************
 SP_DLL_EXPORT SPExtRoutingKeyErr ExtRoutingKey_ToSubsKey (const string externRoutingKey,
                         string *subsRoutingKey);



 //********************************************************************
 //**** Converts an AMQPRoutingKey to an external RK string.
 //**** Any NULL components are omitted from the final external RK
 //**** string.
 //********************************************************************

 SP_DLL_EXPORT SPExtRoutingKeyErr ExtRoutingKey_FromInternal (AMQPRoutingKey internRoutingKey,
                            string &newRoutingKey);


 //********************************************************************
 //**** Gets the text string associated with the specifed
 //**** SPExtRoutingKeyErr.
 //********************************************************************
 SP_DLL_EXPORT SPExtRoutingKeyErr ExtRoutingKey_ErrToText (SPExtRoutingKeyErr err,
                         string *text);

 #endif
