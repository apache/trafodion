// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2010-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <assert.h>

#include "wrapper/routingkey.h"
#include "wrapper/externroutingkey.h"
#include "externroutingkey_int.h"

//*********************************************************************
//
//   Implementation notes and assumptions
//
//     This file contains a class that implementation translation for
//     routing and subscription keys from external format to internal
//     format.  The intention is that users specify keywords for the
//     protected formats of the routing key (currently category,
//     package, scope, security, and protocol).  These are fixed, but
//     are translated to the internal representations by this module.
//     The intent is that this module can be used in conjunction with
//     the AMQPRoutingKey class to build the appropriate routing
//     key.
//
//     Why??
//     =====
//
//     We need our routing keys to be flexible internally, but without
//     requiring that we change a bunch of xml files, protofiles, and
//     registry entries every time we change the routing key order or
//     the spelling of a component or something.  So the goals of this
//     class are to:
//
//     1) Isolate our externals and command-line processing from the
//        internal representation of the routing key.
//
//     2) Make our routing key command line parameter parsing
//        consistent across all Transducer components
//
//     What??
//     ======
//
//     This class includes:
//
//     1) External keyword definitions for routing key components
//     2) Parsing functions for external routing key components
//     3) Defaulting mechanism for non-specified components (allows
//        for upward compatibility as we add new components)
//     4) Methods to build both routing keys and subscription keys
//
//     How??
//     =====
//
//     o Routing key components are "." separated and have fixed
//       lower-case keywords to identify, except for publication name.
//       They might be specified in any order.
//
//     o Need to be thread-safe:  Some folks using this class might be
//       in thread mode
//
//     o Order not specified:  The routing key component keywords can be
//       specified in any order in the string.
//
//     o Performance critical, need quick lookup of some number of
//       strings of varying length, total < 100
//
//     o In case of invalid keyword or more than one free-string, we
//       return an error.   Error bounces immediately, so if you fix
//       one problem, you might find another.
//
//     o Publication names that are "close" to routing key component
//       names could be construed as typos.  So a mistyped
//       "evnt.dtm.instance.public.gpb.blah" routing key will get a "two
//       freeformstrings" error, not a "yo, dude, add another e to
//       *event* error."  We can't detect spelling errors easily if we
//       use free-form strings and any order.
//
//     o Similarly if there's not another free-form string, well, a
//       misspelled component name becomes the publication name.
//       So if you're trying to spot the reason why something isn't
//       showing up, CHECK THE SPELLING OF THE EXTERNAL PARAMETER VERY
//       CAREFULLY! Because, y'know, we might be thinking that "instacne"
//       is a publication name, rather than a typo for the spelling of
//       our keyword "instance".
//
//     Assumptions???

//     ==============
//     o At most *one* (ONE!)(1) routing key component is a free-form
//       string that does not match a component string, and that
//       free-form string is always the publication name.
//
//     o Routing key components are case insensitive (including
//       publication name) and are always used as lower case.  First
//       thing we do is to downshift all the components.
//
//     o Routing key component names are unique across all
//       components.  That is, there is no keyword "event" in the
//       category component and in some other component as well.
//
//     o AMQPRoutingKey presumes that all keys are internal, not
//       external, and validates accordingly.
//
//     o Most of the time routing keys are configured automatically,
//       then set and forgotten.  So our typo considerations above
//       aren't that big of a deal.  Internal developers and other
//       internal users will fix their .xml files and command line
//       parameters and such, then update appropriately so that
//       subsequent usage is automated.  IF WE CHANGE THIS ASSUMPTION,
//       WE SHOULD GO WITH FIXED-ORDER ROUTING KEYS, defaulting
//       anything that's missing.
//
//*********************************************************************


//********************************************************************
//  ExtRoutingKey_ToInternal
//
//     Converts an external RK to a complete internal RK, overwriting
//     any previous contents
//
//     This method takes an external routing key with parameters in
//     any order and converts it to an internal routingkey string.
//     Any missing parameters are defaulted using the NO* component,
//     so the routing key is not guaranteed to be valid and complete
//     if components are missing from the external key.
//
//    Errors:
//     Returns a SPERKERR_* error if the external routing key
//     contains too many components, repeated components, or an
//     invalid freestring.  If an error other than SPERKERR_NOERR is
//     returned, then internalRoutingKey is a null routing key.
//     Otherwise, internalRoutingKey is presumed to be valid, though
//     might be missing components if they were not included in the
//     external routing key
//
//********************************************************************

//********************
// Ugly but DRY!   Inline adds pointer complexity, just need
// substitution.  Clean This Up but get it to work first :)
//********************
# define ResetIRKComponents {                   \
  scope = SP_NULLSCOPE;                     \
  security = SP_NULLSECURITY;                   \
  category = SP_NULLCATEGORY;                   \
  protocol = SP_NULLPROTOCOL;                   \
  package = SP_NULLPACKAGE;                 \
  publicationName = "";                     \
  }

//****************************************Ugliness END

//********************************************************************
//**** Compares an external RK to another. 
//********************************************************************
SP_DLL_EXPORT bool ExtRoutingKey_Compare(const string &externRoutingKey1,
                                         const string &externRoutingKey2)
{
  // Right now this function simply does a string compare.
  // NOTE: There is no check that the strings actually represent 
  //       external routing keys.
  return (externRoutingKey1 == externRoutingKey2);
}

//********************************************************************
//**** Converts an external RK to a complete internal RK, overwriting
//**** any previous contents
//********************************************************************
SP_DLL_EXPORT SPExtRoutingKeyErr ExtRoutingKey_ToInternal (string externRoutingKey,
                         AMQPRoutingKey *internRoutingKey)
{

  //***********************
  // Init and setup
  //***********************

  SPExtRoutingKeyErr retErr = SPERKERR_NOERR;

  internRoutingKey->SetFromString ("");

  // bools to detect repeat categories
  bool foundCategory = false;
  bool foundPackage = false;
  bool foundScope = false;
  bool foundSecurity = false;
  bool foundProtocol = false;
  bool foundPublicationName = false;

  //the actual parameters to pass to the routing key setter

  SP_PublicationScopeType scope = SP_NULLSCOPE;
  SP_PublicationSecurityType security = SP_NULLSECURITY;
  SP_PublicationCategoryType category = SP_NULLCATEGORY;
  SP_PublicationProtocolType protocol = SP_NULLPROTOCOL;
  SP_PublicationPackageType package = SP_NULLPACKAGE;
  string publicationName = "";

  //***********************
  //  Loop over components
  //***********************

  size_t pos = 0;

  while ( (pos < externRoutingKey.length() )  &&
      (retErr == SPERKERR_NOERR) )
  {
    //***********************
    //  Get next component type and value
    //***********************
    string junk = GetNextLevel (externRoutingKey, pos);

    spExtRoutingKeyCompType componentType = GetComponentTypeFromString(&junk);

    spExtRoutingKeyComponentGroupType componentGroup
                                        = GetCompGroupFromCompType (componentType);


  //***********************
  //  SetRoutingKey Value if not duplicate
  //***********************

    switch (componentGroup)
    {
      case SPERKCOMPGROUP_CATEGORY:
      {

    if (foundCategory == true)
    {
      //* whoops, we already had a category!
      retErr = SPERKERR_REPEATEDCOMPONENT;
      ResetIRKComponents;
    }
    else
    {
      foundCategory = true;
      category = GetCategoryFromComponentType (componentType);
    }

    break;
      }

      case SPERKCOMPGROUP_PACKAGE:
      {
    if (foundPackage == true)
    {
      //* whoops, we already had a package!
      retErr = SPERKERR_REPEATEDCOMPONENT;
      ResetIRKComponents;
    }
    else
    {
      foundPackage = true;
      package = GetPackageFromComponentType (componentType);
    }

    break;
      }

      case SPERKCOMPGROUP_SCOPE:
      {
    if (foundScope == true)
    {
      //* whoops, we already had a scope!
      retErr = SPERKERR_REPEATEDCOMPONENT;
      ResetIRKComponents;
    }
    else
    {
      foundScope = true;
      scope = GetScopeFromComponentType (componentType);
    }

    break;
      }

      case SPERKCOMPGROUP_SECURITY:
      {
    if (foundSecurity == true)
    {
      //* whoops, we already had a security!
      retErr = SPERKERR_REPEATEDCOMPONENT;
      ResetIRKComponents;
    }
    else
    {
      foundSecurity = true;
      security = GetSecurityFromComponentType (componentType);
    }

    break;
      }

      case SPERKCOMPGROUP_PROTOCOL:
      {
    if (foundProtocol == true)
    {
      //* whoops, we already had a protocol!
      retErr = SPERKERR_REPEATEDCOMPONENT;
      ResetIRKComponents;
    }
    else
    {
      foundProtocol = true;
      protocol = GetProtocolFromComponentType (componentType);
    }

    break;
      }

      case SPERKCOMPGROUP_PUBLICATIONNAME:
      {
    if (foundPublicationName == true)
    {
      //* whoops, we already had a publicationName!
      retErr = SPERKERR_TOOMANYFREESTRINGS;
      ResetIRKComponents;
    }
    else
    {
      foundPublicationName = true;
      publicationName = junk;
    }

      break;
      }

      case SPERKCOMPGROUP_NONE:
      case SPERKCOMPGROUP_MAXCOMPONENT:
      default:
      {
    //* one of the undefined types!
    retErr = SPERKERR_TOOMANYCOMPONENTS;
    ResetIRKComponents;
    break;
      }

    } // switch

    //***********************
    // Bump parsing position
    //***********************

    pos += junk.length() + EXTROUTINGKEY_LEVELSEPARATOR_LEN;

    //***********************
    // loop control end
    //***********************
  } // While loop

  //***********************
  // set the internal routing key
  //***********************
  if (retErr == SPERKERR_NOERR)
  {
    internRoutingKey->SetFromComponents (category, package, scope, security, protocol,
                     publicationName);
  }

  //***********************
  // return and cleanup
  //***********************

  return retErr;

  //***********************
  //  Finis!
  //***********************
}


//********************************************************************
//**** Converts an external RK to a complete internal subscription
//**** key, overwriting any previous contents.  Missing components are
//**** defaulted using # and * as appropriate
//********************************************************************
SP_DLL_EXPORT SPExtRoutingKeyErr ExtRoutingKey_ToSubsKey (const string externRoutingKey,
                        string *subsRoutingKey)
{
  //***********************
  // Init and Setup
  //***********************
  SPExtRoutingKeyErr retErr = SPERKERR_NOERR;

  AMQPRoutingKey intRoutingKey;

  //***********************
  // Build Routing Key
  //***********************

  retErr = ExtRoutingKey_ToInternal (externRoutingKey,
                     &intRoutingKey);

  if (retErr == SPERKERR_NOERR)
  {
    //***********************
    // Convert to subscription string
    //***********************

    *subsRoutingKey = AMQPRoutingKey_BuildSubscriptionRoutingKey
                             (intRoutingKey.GetCategory(),
                  intRoutingKey.GetPackage(),
                  intRoutingKey.GetScope(),
                  intRoutingKey.GetSecurity(),
                  intRoutingKey.GetProtocol(),
                  intRoutingKey.GetPublicationName() );

  }

  //***********************
  // Cleanup and Return
  //***********************

  return retErr;

  //***********************
  // FINIS!
  //***********************
}

//********************************************************************
//**** Converts an AMQPRoutingKey to an external RK string.
//**** Any NULL components are omitted from the final external RK
//**** string.
//********************************************************************
SP_DLL_EXPORT SPExtRoutingKeyErr ExtRoutingKey_FromInternal (AMQPRoutingKey internRoutingKey,
                            string &newRoutingKey)
{
  newRoutingKey = internRoutingKey.GetAsString();
  return SPERKERR_NOERR;
}

//********************************************************************
//**** Gets the text string associated with the specifed
//**** SPExtRoutingKeyErr.
//********************************************************************
SP_DLL_EXPORT SPExtRoutingKeyErr ExtRoutingKey_ErrToText (SPExtRoutingKeyErr err,
                        string *text)
{

  //*************************
  // Init and setup
  //*************************
  SPExtRoutingKeyErr retErr = SPERKERR_NOERR;
  *text = "GARBAGE";

  //*************************
  // Get error text from string if error is valid
  //*************************

  if ( err < SPERKERR_LASTERR )
  {
    *text = spERKErrText [ size_t (err) ];
  }
  else
  {
    *text = spERKErrText [ size_t (SPERKERR_LASTERR) ];
    retErr=SPERKERR_ERROUTOFBOUNDS;
  }

  return retErr;
}

//**********************************************************************
//   GetNextLevel  - scans to next "." or end of string.  Interal only
//**********************************************************************
SP_DLL_EXPORT string GetNextLevel (const string &scanString, size_t startPoint)
{

  // scans the scanstring from the starting point til it gets to the
  // next level separator or until the end of the string


  string junk = "";

  if (startPoint == scanString.length() )
  {
    // We already are at the end of the string.  Nothing to do!
    return junk;
  }

  size_t endPoint = scanString.find_first_of (EXTROUTINGKEY_LEVELSEPARATOR, startPoint);

  int copyBytes = 0;

  if (endPoint == string::npos)
  {
    // No LEVELSEPARATOR found; must be last level in scanString
    copyBytes = scanString.length() - startPoint;
  }
  else
  {
    copyBytes= endPoint - startPoint;
  }

  junk = scanString.substr (startPoint, copyBytes);

  return junk;
}
//********************************************************
// GetComponentTypeFromString - maps publication to component type
//********************************************************

SP_DLL_EXPORT spExtRoutingKeyCompType GetComponentTypeFromString (const string *componentString)
{

  //*************************
  // Initialization and setup
  //*************************
  spExtRoutingKeyCompType retType = SPERK_NULLCOMPONENT;

  //*************************
  // Search over text table to find string
  //*************************

  // for now, we just do a simple direct search.  We could alphabetize
  // and do a binary search or hash 'em, if performance gets to be an
  // issue.  But there are fewer than 30 entries, so we're not
  // searching megabytes of data here...

  for (size_t i = 0; i < SPERK_MAXCOMPONENTTYPE; i++)
  {

    if ( *componentString == extStringText[i])
    {
      retType = spExtRoutingKeyCompType (i);
      break;
    }
  }

  //*************************
  // Return found type or NULL if not found
  //*************************

  return retType;

  //*************************
  //  Finis!
  //*************************
}

//********************************************************
// GetCompGroupFromCompType - maps publication to component group
//********************************************************

SP_DLL_EXPORT spExtRoutingKeyComponentGroupType GetCompGroupFromCompType (spExtRoutingKeyCompType keyType)
{

  //************************
  //  Setup and initialization
  //************************

  spExtRoutingKeyComponentGroupType retGroup = SPERKCOMPGROUP_NONE;

  //***********************
  // Switch based on keyType
  //***********************

  // the SPERK_types are grouped based on components.  Adding a new
  // value?  Put it here with the appropriate component group....

  switch (keyType)
  {

    case SPERK_NULLPACKAGE:
    case SPERK_COMMONPACKAGE:
    case SPERK_DTMPACKAGE:
    case SPERK_INCIDENTANALYSISPACKAGE:
    case SPERK_LIGHTHOUSEPACKAGE:
    case SPERK_MONITORPACKAGE:
    case SPERK_NDCSPACKAGE:
    case SPERK_SEPACKAGE:
    case SPERK_SUMMARYPACKAGE:
    case SPERK_UNCPACKAGE:
    case SPERK_SPAPACKAGE:
    case SPERK_SQLPACKAGE:
    case SPERK_NVTSVCPACKAGE:
    case SPERK_PROBLEMMANAGEMENTPACKAGE:
    case SPERK_LUNPACKAGE:
    case SPERK_SSDPACKAGE:
    case SPERK_MANAGEABILITY_INFRASTRUCTUREPACKAGE:
    case SPERK_STATEPACKAGE:
    case SPERK_TPGPACKAGE:
    case SPERK_CHAMPACKAGE:
    case SPERK_ACTIONPACKAGE:
    case SPERK_COMPRESSIONPACKAGE:
    case SPERK_MANAGEABILITY_TESTPACKAGE:
    case SPERK_WMSPACKAGE:
    case SPERK_OSPACKAGE:
    case SPERK_LINUXCOUNTERSPACKAGE:
    case SPERK_ACCESSLAYERPACKAGE:
    case SPERK_DATABASELAYERPACKAGE:
    case SPERK_FOUNDATIONLAYERPACKAGE:
    case SPERK_OSLAYERPACKAGE:
    case SPERK_SERVERLAYERPACKAGE:
    case SPERK_STORAGELAYERPACKAGE:
    case SPERK_DBSPACKAGE:
    {
    retGroup = SPERKCOMPGROUP_PACKAGE;
    break;
    }

    case SPERK_NULLSCOPE:
    case SPERK_CLUSTERSCOPE:
    case SPERK_INSTANCESCOPE:
    case SPERK_TENANTSCOPE:
    {
    retGroup = SPERKCOMPGROUP_SCOPE;
    break;
    }

    case SPERK_NULLSECURITY:
    case SPERK_PUBLICSECURITY:
    case SPERK_PRIVATESECURITY:
    {
    retGroup = SPERKCOMPGROUP_SECURITY;
    break;
    }

    case SPERK_NULLCATEGORY:
    case SPERK_EVENTCATEGORY:
    case SPERK_PERF_STATCATEGORY:
    case SPERK_HEALTH_STATECATEGORY:
    case SPERK_SECURITYCATEGORY:
    {
    retGroup = SPERKCOMPGROUP_CATEGORY;
    break;
    }

    case SPERK_NULLPROTOCOL:
    case SPERK_GPBPROTOCOL:
    case SPERK_XMLPROTOCOL:
    {
    retGroup = SPERKCOMPGROUP_PROTOCOL;
    break;
    }

    case SPERK_NULLCOMPONENT:
    case SPERK_MAXCOMPONENTTYPE:
    default:
    {
      //  No matches means that we've got a string here!
    retGroup = SPERKCOMPGROUP_PUBLICATIONNAME;
    break;
    }

  }
  //***********************
  // Return Component Group
  //***********************

  return retGroup;

  //***********************
  // Finis!
  //***********************

}

//********************************************************
// Get*FromComponentType -- maps component type to internal
//              componentEnum.  One for each componenent level
//********************************************************

SP_DLL_EXPORT SP_PublicationCategoryType GetCategoryFromComponentType ( const spExtRoutingKeyCompType compType)
{

  //***********************
  //  Init and setup
  //***********************
  SP_PublicationCategoryType retCategory = SP_NULLCATEGORY;

  //***********************
  // Map external category type to internal category type
  //***********************
  switch (compType)
  {
    case SPERK_EVENTCATEGORY:
    {
      retCategory = SP_EVENT;
      break;
    }

    case SPERK_PERF_STATCATEGORY:
    {
      retCategory = SP_PERF_STAT;
      break;
    }

    case SPERK_HEALTH_STATECATEGORY:
    {
      retCategory = SP_HEALTH_STATE;
      break;
    }

    case SPERK_SECURITYCATEGORY:
    {
      retCategory = SP_SECURITY;
      break;
    }

    case SPERK_NULLCATEGORY:
    default:
    {
      retCategory = SP_NULLCATEGORY;
      break;
    }

  } // Switch statement

  //***********************
  //  Return found category
  //***********************
  return retCategory;

  //***********************
  // FINIS!
  //***********************
}


SP_DLL_EXPORT SP_PublicationPackageType GetPackageFromComponentType ( const spExtRoutingKeyCompType compType)
{

  //***********************
  //  Init and setup
  //***********************
  SP_PublicationPackageType retPackage = SP_NULLPACKAGE;

  //***********************
  // Map external package type to internal package type
  //***********************
  switch (compType)
  {
    case SPERK_COMMONPACKAGE:
    {
      retPackage = SP_COMMONPACKAGE;
      break;
    }

    case SPERK_DTMPACKAGE:
    {
      retPackage = SP_DTMPACKAGE;
      break;
    }

    case SPERK_INCIDENTANALYSISPACKAGE:
    {
      retPackage = SP_INCIDENTANALYSISPACKAGE;
      break;
    }

    case SPERK_LIGHTHOUSEPACKAGE:
    {
      retPackage = SP_LIGHTHOUSEPACKAGE;
      break;
    }

    case SPERK_MONITORPACKAGE:
    {
      retPackage = SP_MONITORPACKAGE;
      break;
    }

    case SPERK_NDCSPACKAGE:
    {
      retPackage = SP_NDCSPACKAGE;
      break;
    }

    case SPERK_SEPACKAGE:
    {
      retPackage = SP_SEPACKAGE;
      break;
    }

    case SPERK_SUMMARYPACKAGE:
    {
      retPackage = SP_SUMMARYPACKAGE;
      break;
    }

    case SPERK_UNCPACKAGE:
    {
      retPackage = SP_UNCPACKAGE;
      break;
    }

    case SPERK_SPAPACKAGE:
    {
      retPackage = SP_SPAPACKAGE;
      break;
    }

    case SPERK_SQLPACKAGE:
    {
      retPackage = SP_SQLPACKAGE;
      break;
    }

    case SPERK_NVTSVCPACKAGE:
    {
      retPackage = SP_NVTSVCPACKAGE;
      break;
    }

    case SPERK_PROBLEMMANAGEMENTPACKAGE:
    {
      retPackage = SP_PROBLEMMANAGEMENTPACKAGE;
      break;
    }

    case SPERK_LUNPACKAGE:
    {
      retPackage = SP_LUNPACKAGE;
      break;
    }

    case SPERK_SSDPACKAGE:
    {
      retPackage = SP_SSDPACKAGE;
      break;
    }

    case SPERK_MANAGEABILITY_INFRASTRUCTUREPACKAGE:
    {
      retPackage = SP_MANAGEABILITY_INFRASTRUCTUREPACKAGE;
      break;
    }

    case SPERK_STATEPACKAGE:
    {
      retPackage = SP_STATEPACKAGE;
      break;
    }

    case SPERK_MANAGEABILITY_TESTPACKAGE:
    {
      retPackage = SP_MANAGEABILITY_TESTPACKAGE;
      break;
    }

    case SPERK_WMSPACKAGE:
    {
      retPackage = SP_WMSPACKAGE;
      break;
    }

    case SPERK_TPGPACKAGE:
    {
      retPackage = SP_TPGPACKAGE;
      break;
    }

    case SPERK_CHAMPACKAGE:
    {
      retPackage = SP_CHAMPACKAGE;
      break;
    }

    case SPERK_ACTIONPACKAGE:
    {
      retPackage = SP_ACTIONPACKAGE;
      break;
    }

    case SPERK_OSPACKAGE:
    {
      retPackage = SP_OSPACKAGE;
      break;
    }

    case SPERK_LINUXCOUNTERSPACKAGE:
    {
      retPackage = SP_LINUXCOUNTERSPACKAGE;
      break;
    }

    case SPERK_ACCESSLAYERPACKAGE:
    {
      retPackage = SP_ACCESSLAYERPACKAGE;
      break;
    }

    case SPERK_DATABASELAYERPACKAGE:
    {
      retPackage = SP_DATABASELAYERPACKAGE;
      break;
    }

    case SPERK_FOUNDATIONLAYERPACKAGE:
    {
      retPackage = SP_FOUNDATIONLAYERPACKAGE;
      break;
    }

    case SPERK_OSLAYERPACKAGE:
    {
      retPackage = SP_OSLAYERPACKAGE;
      break;
    }

    case SPERK_SERVERLAYERPACKAGE:
    {
      retPackage = SP_SERVERLAYERPACKAGE;
      break;
    }

    case SPERK_STORAGELAYERPACKAGE:
    {
      retPackage = SP_STORAGELAYERPACKAGE;
      break;
    }

    case SPERK_DBSPACKAGE:
    {
      retPackage = SP_DBSPACKAGE;
      break;
    }

    case SPERK_COMPRESSIONPACKAGE:
    {
        retPackage = SP_COMPRESSIONPACKAGE;
        break;
    }

    case SPERK_NULLPACKAGE:
    default:
    {
      retPackage = SP_NULLPACKAGE;
      break;
    }

  } // Switch statement

  //***********************
  //  Return found category
  //***********************
  return retPackage;

  //***********************
  // FINIS!
  //***********************
}

SP_DLL_EXPORT SP_PublicationScopeType GetScopeFromComponentType ( const spExtRoutingKeyCompType compType)
{

  //***********************
  //  Init and setup
  //***********************
  SP_PublicationScopeType retScope = SP_NULLSCOPE;

  //***********************
  // Map external scope type to internal scope type
  //***********************
  switch (compType)
  {

    case SPERK_CLUSTERSCOPE:
    {
      retScope = SP_CLUSTER;
      break;
    }

    case SPERK_INSTANCESCOPE:
    {
      retScope = SP_INSTANCE;
      break;
    }

    case SPERK_TENANTSCOPE:
    {
      retScope = SP_TENANT;
      break;
    }

    case SPERK_NULLSCOPE:
    default:
    {
      retScope = SP_NULLSCOPE;
      break;
    }

  } // Switch statement

  //***********************
  //  Return found category
  //***********************
  return retScope;

  //***********************
  // FINIS!
  //***********************
}

SP_DLL_EXPORT SP_PublicationSecurityType GetSecurityFromComponentType ( const spExtRoutingKeyCompType compType)
{

  //***********************
  //  Init and setup
  //***********************
  SP_PublicationSecurityType retSecurity = SP_NULLSECURITY;

  //***********************
  // Map external category type to internal category type
  //***********************
  switch (compType)
  {

    case SPERK_PUBLICSECURITY:
    {
      retSecurity = SP_PUBLIC;
      break;
    }

    case SPERK_PRIVATESECURITY:
    {
      retSecurity = SP_PRIVATE;
      break;
    }

    case SPERK_NULLSECURITY:
    default:
    {
      retSecurity = SP_NULLSECURITY;
      break;
    }

  } // Switch statement

  //***********************
  //  Return found category
  //***********************
  return retSecurity;

  //***********************
  // FINIS!
  //***********************
}

SP_DLL_EXPORT SP_PublicationProtocolType GetProtocolFromComponentType ( const spExtRoutingKeyCompType compType)
{

  //***********************
  //  Init and setup
  //***********************
  SP_PublicationProtocolType retProtocol = SP_NULLPROTOCOL;

  //***********************
  // Map external category type to internal category type
  //***********************
  switch (compType)
  {

    case SPERK_GPBPROTOCOL:
    {
      retProtocol = SP_GPBPROTOCOL;
      break;
    }

    case SPERK_XMLPROTOCOL:
    {
      retProtocol = SP_XMLPROTOCOL;
      break;
    }

    case SPERK_NULLPROTOCOL:
    default:
    {
      retProtocol = SP_NULLPROTOCOL;
      break;
    }

  } // Switch statement

  //***********************
  //  Return found category
  //***********************
  return retProtocol;

  //***********************
  // FINIS!
  //***********************
}

