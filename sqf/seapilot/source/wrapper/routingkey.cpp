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
#include "internalroutingkey.h"

using namespace std;


//**********************************************************************
//
//  Implmentation notes and assumptions
//
//   o  Most access and usage is for the whole routing key string.
//      Hence m_routingKey_ is a string, and we're not storing
//      component parts.  If performance for components becomes
//      critical, we need to store separately or store both.
//
//   o  This code is very firmly based on our routing key and
//      protofile naming standards.  Two key methods in each direction
//      build the routing keys and map to/from the proto file names.
//      All other methods rely on these methods, so if you change the
//      routing key or proto file definition, those are the methods to
//      change.  See  SetFromComponents and GetRoutingKeyComponents,
//      plus SetFromProtofileName and GetAsMsgName.  One other method
//      builds a routing key subscription string based on this format.
//      Everything else is gravy.
//
//   o  Unimplemented prototype methods assert (false);.  If you hit
//      one, be a dear (deer?) and please implement :D
//
//   o  A totally unitialized routing key has 0 length.
//
//   o  A partially initialized routing key has GARBAGELEVEL
//      components for all uninitialized components.
//
//   o  The presence of any GARBAGELEVEL component renders a routing
//      key invalid. We could write a test-level consumer that scans
//      for GARBAGELEVEL and publishes.  Or could assert in the
//      wrappers so that we trap anyone who tries.  Mwahahahaha!
//      Either way, if you're playing with garbage in routing
//      keys or protofile names, you need to clean up.
//
//   o  First pass is optimized for readability and maintainability.
//      There are a few performance improvements we could do if
//      needed.  In GetNextLevel and a couple other places, we do
//      extra copying into a temp string and then into user variables.
//      We could return pointers into the routing key string instead.
//      That's less isolated from end users though.
//
//   o  If an invalid routing key is passed in (one with not enough
//      levels or too many levels, or with an unrecognized type), this
//      module makes the following  choices: 
//
//         Not Enough Levels: the entire routing key is the
//           publication name and .proto name. Information up to the
//           first separator is considered the package name, and all
//           other information is the package key.
//        
//         Too Many Levels:  Assuming that the message has a standard
//           type, the extra fields are included in the publication name
//           field. 
//             
//         Non-Standard type: the entire routing key is the
//           publication name and .proto name. Information up to the
//           first separator is considered the package name, and all
//           other information is the package key.
//
//   o  This package contains one library routine that's not part of a
//      class:  AMQPRoutingKey_GetSubscriptionRoutingKeyFromParams,
//      which builds a subscription routing key from the passed
//      parameters, defaulting those that aren't specified to * or #.
//      It's included in this module because this module contains the
//      current definition of the routing key.
//
//*************************************************************************

//**************************************************
//  AMQPRoutingKey - Default Constructor
//**************************************************
AMQPRoutingKey::AMQPRoutingKey()
{

  m_routingKey_ = "";
  m_numLevels_ = 0;

}



//**************************************************
//  AMQPRoutingKey -- Parameterized Constructor
//       String routing key parameter.
//**************************************************

AMQPRoutingKey::AMQPRoutingKey (const string &routingKey) {

  m_routingKey_ = routingKey;
  m_numLevels_ = GetNumLevels();

}

//**************************************************
//  AMQPRoutingKey -- Parameterized Constructor
//       Components parameter
//**************************************************

AMQPRoutingKey::AMQPRoutingKey ( SP_PublicationCategoryType category, 
				 SP_PublicationPackageType package,
				 SP_PublicationScopeType scope, 
				 SP_PublicationSecurityType security,
				 SP_PublicationProtocolType protocol,
				 const string &publicationName )
{

  SetFromComponents (category, package, scope, security, protocol, publicationName);
  
  // Look for embedded levels in the publication name

  int numberOfLevelsInPubName = 0;
  size_t pos = 0;

  while (pos < publicationName.length()) 
  {

    string junk = GetNextLevel (publicationName, pos);
    if (( junk != ROUTINGKEY_GARBAGELEVEL) && ( junk.length() > 0) )
    {
      numberOfLevelsInPubName ++;
    }

    pos += junk.length() + ROUTINGKEY_LEVELSEPARATOR_LEN;
  }

  m_numLevels_ = ROUTINGKEY_NUMFIXEDLEVELS + numberOfLevelsInPubName;

}

//*******************************************
// GetNumLevels
//*******************************************

int AMQPRoutingKey::GetNumLevels ()
{

  // Look for embedded levels in the routingkey

  int numberOfLevels = 0;
  size_t pos = 0;

  while (pos < m_routingKey_.length()) 
  {

    string junk = GetNextLevel (m_routingKey_, pos);
    if (( junk != ROUTINGKEY_GARBAGELEVEL) && ( junk.length() > 0) )
    {
      numberOfLevels ++;
    }

    pos += junk.length() + ROUTINGKEY_LEVELSEPARATOR_LEN;
  }

  return numberOfLevels;

}

//**************************************************
//  GetPackage
//**************************************************
string AMQPRoutingKey::GetPackageString ()
{
  string localPackage = "";

  if (m_routingKey_ != "")
  {
    GetRoutingKeyComponents (NULL, &localPackage, NULL, NULL, NULL,NULL);
  }

  return localPackage;
}

SP_PublicationPackageType AMQPRoutingKey::GetPackage()
{

  SP_PublicationPackageType localPackage = SP_NULLPACKAGE;

  string localPackageString = "";

  if (m_routingKey_ != "")
  {

    GetRoutingKeyComponents (NULL, &localPackageString, NULL, NULL, NULL, NULL);

    // find the string in the text table
    for (size_t i = 0; i < SP_PACKAGEMAX; i++) 
    {

      if (localPackageString == publicationPackageText[i]) 
      { 
	localPackage = (SP_PublicationPackageType) i;
	break;
      }
    }
  }

  return localPackage;

}


//**************************************************
//  GetCategory
//**************************************************
string AMQPRoutingKey::GetCategoryString ()
{

  string type = "";

  if (m_routingKey_ != "")
  {
    GetRoutingKeyComponents ( &type, NULL, NULL, NULL, NULL, NULL);
  }
  return type;
}

SP_PublicationCategoryType AMQPRoutingKey::GetCategory()
{

  SP_PublicationCategoryType localCategory = SP_NULLCATEGORY;

  string localCategoryString = "";

  if (m_routingKey_ != "")
  {

    GetRoutingKeyComponents (&localCategoryString, NULL, NULL, NULL, NULL, NULL);

    // find the string in the text table.
    for (size_t i = 0; i < SP_MAXCATEGORY; i++) 
    {

      if (localCategoryString == publicationCategoryText[i]) 
      { 
	localCategory = (SP_PublicationCategoryType) i;
	break;
      }
    }
  }

  return localCategory;

}



//**************************************************
//  GetScope
//**************************************************
string AMQPRoutingKey::GetScopeString ()
{

  string scope = "";

  if (m_routingKey_ != "")
  {
    GetRoutingKeyComponents ( NULL, NULL, &scope, NULL, NULL,NULL);
  }

  return scope;
}

SP_PublicationScopeType AMQPRoutingKey::GetScope()
{

  SP_PublicationScopeType localScope = SP_NULLSCOPE;

  string localScopeString = "";

  if (m_routingKey_ != "")
  {

    GetRoutingKeyComponents ( NULL, NULL, &localScopeString, NULL, NULL,NULL);

    // find the string in the text table.
    for (size_t i = 0; i < SP_MAXSCOPE; i++) 
    {

      if (localScopeString == publicationScopeText[i]) 
      { 
	localScope = (SP_PublicationScopeType) i;
	break;
      }
    }
  }

  return localScope;

}


//**************************************************
//  GetSecurity
//**************************************************}
string AMQPRoutingKey::GetSecurityString ()
{
  string security = "";

  if (m_routingKey_ != "")
  {
    GetRoutingKeyComponents ( NULL, NULL, NULL, &security, NULL, NULL);
  }

  return security;
}

SP_PublicationSecurityType AMQPRoutingKey::GetSecurity()
{

  SP_PublicationSecurityType localSecurity = SP_NULLSECURITY;

  string localSecurityString = "";

  if (m_routingKey_ != "")
  {

    GetRoutingKeyComponents ( NULL, NULL, NULL, &localSecurityString, NULL, NULL);

    // find the string in the text table
    for (size_t i = 0; i < SP_MAXSECURITY; i++) 
    {

      if (localSecurityString == publicationSecurityText[i]) 
      { 
	localSecurity = (SP_PublicationSecurityType) i;
	break;
      }
    }
  }

  return localSecurity;

}


//**************************************************
//  GetProtocol
//**************************************************}
string AMQPRoutingKey::GetProtocolString ()
{
  string protocol = "";

  if (m_routingKey_ != "")
  {
    GetRoutingKeyComponents ( NULL, NULL, NULL, NULL, &protocol, NULL);
  }

  return protocol;
}

SP_PublicationProtocolType AMQPRoutingKey::GetProtocol()
{

  SP_PublicationProtocolType localProtocol = SP_NULLPROTOCOL;

  string localProtocolString = "";

  if (m_routingKey_ != "")
  {

    GetRoutingKeyComponents ( NULL, NULL, NULL, NULL, &localProtocolString, NULL);

    // find the string in the text table
    for (size_t i = 0; i < SP_MAXPROTOCOL; i++) 
    {

      if (localProtocolString == publicationProtocolText[i]) 
      { 
	localProtocol = (SP_PublicationProtocolType) i;
	break;
      }
    }
  }

  return localProtocol;

}



//**************************************************
//  GetPublicationName
//**************************************************
string AMQPRoutingKey::GetPublicationName()
{
  string publicationName = "";

  if (m_routingKey_ != "")
  {
    GetRoutingKeyComponents ( NULL, NULL, NULL, NULL, NULL, &publicationName);
  }

  return publicationName;
}


//**************************************************
//  GetAsProtofileName
//**************************************************
string AMQPRoutingKey::GetAsProtofileName()
{

  string protofileName = this->GetAsMessageName();

  protofileName = protofileName + "." + ROUTINGKEY_PROTOFILEEXTENSION;

  return protofileName;
}

//**************************************************
//  GetAsMessageName()
//**************************************************
string AMQPRoutingKey::GetAsMessageName()
{

  //  Here's where the mapping from publication to proto file name
  //  occurs.  To change this standard, adjust the protoFileName
  //  build, perhaps getting different components if we change the
  //  naming convention.

  string package = ROUTINGKEY_GARBAGELEVEL;
  string publicationName = ROUTINGKEY_GARBAGELEVEL;

  if (m_routingKey_.length() > 3) 
  {
    GetRoutingKeyComponents (NULL, &package, NULL, NULL, NULL, &publicationName);
  }

  string messageName = package 
                           + "." + publicationName ;

  return messageName;
}

//**************************************************
//  GetAsString
//**************************************************
string AMQPRoutingKey::GetAsString ()
{
  return m_routingKey_;
}


//**************************************************
//  SetFromString
//*************************************************

void AMQPRoutingKey::SetFromString (const string &routingKeyString)
{
  m_routingKey_ = routingKeyString;
}



//**************************************************
//  SetFromComponents
//**************************************************
void AMQPRoutingKey::SetFromComponents (  SP_PublicationCategoryType category, 
					  SP_PublicationPackageType package, 
					  SP_PublicationScopeType scope,
					  SP_PublicationSecurityType security,
					  SP_PublicationProtocolType protocol,
					  const string &publicationName)
{

  string localCategory;
  localCategory = publicationCategoryText[category];

  string localPackage;
  localPackage = publicationPackageText[package];

  string localScope;
  localScope = publicationScopeText[scope];
  
  string localSecurity;
  localSecurity = publicationSecurityText[security];

  string localProtocol;
  localProtocol = publicationProtocolText[protocol];

  string localPublicationName;
  if (publicationName.length()>0)
  {

    localPublicationName = publicationName;
  }
  else
  {
    localPublicationName = "";
  }


  // Here's where the routing key is built!  To change the structure,
  // change the order here and change GetRoutingKeyComponents

  m_routingKey_ =   localCategory + "." 
                  + localPackage+ "." 
                  + localScope + "." 
                  + localSecurity + "." 
                  + localProtocol + "."
                  + localPublicationName;

  m_numLevels_ = ROUTINGKEY_NUMFIXEDLEVELS;
  if (localPublicationName.length() > 0)
  {
    m_numLevels_++;
  }
}

//**************************************************
//  SetFromProtofileName
//**************************************************
void AMQPRoutingKey::SetFromProtofileName(const string &protoFileName)
{

  // Here's where a protofile name gets broken up into its
  // components.  If the standard for constructing the protofilename
  // changes, change this method to reflect that.


  // This code assumes that proto files follow our standard, and that
  // their publication names follow our standard as well

  // unused components in current standard


  string packageString ="";
  string publicationName = ROUTINGKEY_GARBAGELEVEL;

  if (protoFileName.length() > 3)
  {
    string sectionCopy = "";
    size_t pos = 0;

    packageString = GetNextLevel (protoFileName, pos);
    pos += packageString.length() + ROUTINGKEY_LEVELSEPARATOR_LEN;

    publicationName = GetNextLevel (protoFileName, pos);
    pos += publicationName.length() + ROUTINGKEY_LEVELSEPARATOR_LEN;


    // following assert blows if we add components to the publication
    // file name.  
    assert ( (packageString.length() 
	      + publicationName.length() 
	      + ROUTINGKEY_PROTOFILEEXTENSION_LENGTH)    // .proto tacked on end
                                         == protoFileName.length());
  }


  SP_PublicationPackageType package = SP_NULLPACKAGE;

    // find the string in the text table.  Start at 1 to skip over the NULL 
    for (size_t i = 1; i < (sizeof (publicationPackageText)-1); i++) 
    {

      if (packageString == publicationPackageText[i]) 
      { 
	package = (SP_PublicationPackageType) i;
	break;
      }
    }


  SetFromComponents (SP_NULLCATEGORY,package, 
		     SP_NULLSCOPE,SP_NULLSECURITY,SP_NULLPROTOCOL, publicationName);


}
							     
//**************************************************
//  isValid
//**************************************************
bool AMQPRoutingKey::isValid()
{
  if (m_routingKey_.length() < ROUTINGKEY_MINLENGTH)
  {
    // Routing key doesn't have enough info.  
    return false;
  }

  size_t pos = 0;

  int numLevels = 0;

  while (pos < m_routingKey_.length()) 
  {
    string junk = GetNextLevel (m_routingKey_, pos);
    
    if (junk == ROUTINGKEY_GARBAGELEVEL) 
    {
      // Valid routing keys don't contain garbage levels!
      return false;
    }

    if (junk.length() == 0) {
      // we don't allow 0-length levels!
      return false;
    }

    pos += junk.length() + ROUTINGKEY_LEVELSEPARATOR_LEN;

    // read a valid level; let's make sure someone didn't go
    // level-crazy and throw in a few extra!
    numLevels ++;

    if (numLevels > ROUTINGKEY_NUMLEVELS) 
    {
      return false;
    }

  }  // while lop

  if (numLevels < ROUTINGKEY_NUMLEVELS)
  {
    // scanned the whole string but didn't have enough levels
    return false;
  }


  // party on dudes!
  return true;

}


//*********************************************************
//    GetRoutingKeyComponents
//*********************************************************

void AMQPRoutingKey::GetRoutingKeyComponents (string *category, string *package, 
					      string *scope, string *security,
					      string *protocol, 
					      string *publicationName) 
{

  // might not have a valid routing key if built from a publication
  // name.  But we start with an assumption that there are 6
  // components.  If that's not true, we'll find out soon enough.
  // OTOH, if there are more than 6 components, we assert so that we
  // know we missed changing something for the routing key components.

  // Here's where we break apart a routing key into its components.
  // If the composition or ordering of a routing key changes, this
  // method must change to match it.

  string junk = "";
  size_t pos = 0;
  int numLevels = 1;

  //***********************
  // FIRST level is category
  //***********************

  junk = "";

  if (numLevels <= m_numLevels_) 
  {
    junk = GetNextLevel (m_routingKey_, pos);

    if (junk.length() > 0) 
    {
      pos = pos + junk.length() + ROUTINGKEY_LEVELSEPARATOR_LEN;
    }

  }
  if (category != NULL) 
  {
    // this handles "" length strings too.
    *category = junk;
  }


  //***********************
  // SECOND level is package
  //***********************
  numLevels ++;
  junk = "";

  if (numLevels <= m_numLevels_)
  {
    junk = GetNextLevel (m_routingKey_, pos);


    if (junk.length() > 0) 
    {
      pos = pos + junk.length() + ROUTINGKEY_LEVELSEPARATOR_LEN;
    }
  }
  if (package != NULL) 
  {
    // this handles "" length strings too.
    *package = junk;
  }

  //***********************
  // THIRD level is scope
  //***********************
  numLevels ++;
  junk = "";

  if (numLevels <= m_numLevels_)
  {
    junk = GetNextLevel (m_routingKey_, pos);

    if (junk.length() > 0) 
    {
      pos = pos + junk.length()  + ROUTINGKEY_LEVELSEPARATOR_LEN;
    }
  }

  if (scope != NULL) 
  {
    // this handles "" length strings too.
    *scope = junk;
  }

  //***********************
  // FOURTH level is security
  //***********************
  numLevels ++;
  junk = "";

  if (numLevels <= m_numLevels_)
  {
    junk = GetNextLevel (m_routingKey_, pos);

    if (junk.length() > 0) 
    {
      pos = pos + junk.length() + ROUTINGKEY_LEVELSEPARATOR_LEN;
    }
  }

  if (security != NULL) 
  {
    // this handles "" length strings too.
    *security = junk;
  }

  //***********************
  // FIFTH level is protocol
  //***********************
  numLevels++;
  junk = "";

  if (numLevels <= m_numLevels_)
  {
    junk = GetNextLevel (m_routingKey_, pos);

    if (junk.length() > 0) 
    {
      pos = pos + junk.length() + ROUTINGKEY_LEVELSEPARATOR_LEN;
    }
  }

  if (protocol != NULL) 
  {
    // this handles "" length strings too.
    *protocol = junk;
  }

  //***********************
  // SIXTH level is publicationName
  //***********************
  numLevels ++;
  junk = "";

  if (numLevels <= m_numLevels_)
  {
    junk = GetNextLevel (m_routingKey_, pos);


    if (junk.length() > 0) 
    {
      pos = pos + junk.length() + ROUTINGKEY_LEVELSEPARATOR_LEN;
    }
  }

  if (publicationName != NULL) 
  {
    // this handles "" length strings too.
    *publicationName = junk;
  }

}

//********************************************************
//   GetNextLevel
//********************************************************
string AMQPRoutingKey::GetNextLevel (const string &scanString, size_t startPoint)
{

  // scans the scanstring from the starting point til it gets to the
  // next level separator or until the end of the string


  string junk = "";

  if (startPoint == scanString.length() ) 
  {
    // We already are at the end of the string.  Nothing to do!
    return junk;
  }

  size_t endPoint = scanString.find_first_of (ROUTINGKEY_LEVELSEPARATOR, startPoint);

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


//*******************************************************************
// Library routine
//*******************************************************************

//Ugly, but DRY.  Inline adds pointer complexity, just need
//substitution and avoiding needless pointer-dom.  Clean. This. Up.
//Now.  but get it to work first :)

#define AddDotsIfNeeded {	\
    if (localRoutingKeyEmpty == false)                             \
    {                                               \
      localRoutingKey = localRoutingKey + ".";                              \
    }                                               \
    if (currNullSequentialLevels > 0)                                 \
    {                                               \
      if (currNullSequentialLevels > 1)                               \
      {                                             \
	localRoutingKey = localRoutingKey + "#";                            \
      }                                             \
      else                                          \
      {                                             \
	localRoutingKey = localRoutingKey + "*";                            \
      }                                             \
      localRoutingKey = localRoutingKey + ".";                              \
      currNullSequentialLevels = 0;                                   \
    }                                               \
}                                                   

//  END OF UGLINESS*********************************************

string AMQPRoutingKey_BuildSubscriptionRoutingKey 
                                    ( SP_PublicationCategoryType category, 
				      SP_PublicationPackageType package,
				      SP_PublicationScopeType scope, 
				      SP_PublicationSecurityType security,
				      SP_PublicationProtocolType protocol,
				      const string &publicationName )
{

  //*****************************
  //  Setup and initialization
  //*****************************

  string localRoutingKey = "";

  bool localRoutingKeyEmpty = true;

  int currNullSequentialLevels = 0;

  //*****************************
  // Add in Category
  //*****************************

  // for the first one *ONLY*, we don't worry about leading "."s.  If
  // something comes before category, we need to shuffl that code
  // around. 

  if  (category != SP_NULLCATEGORY)
  {
    localRoutingKey = localRoutingKey + publicationCategoryText[category];
    localRoutingKeyEmpty = false;

  }
  else
  {
    currNullSequentialLevels++;
  }

  //*****************************
  // Add in package
  //*****************************

  if  (package != SP_NULLPACKAGE)
  {

    AddDotsIfNeeded;

    localRoutingKey = localRoutingKey + publicationPackageText[package];
    localRoutingKeyEmpty = false;

  }
  else
  {
    currNullSequentialLevels++;
  }



  //*****************************
  // Add in scope
  //*****************************

  if  (scope != SP_NULLSCOPE)
  {

    AddDotsIfNeeded;

    localRoutingKey = localRoutingKey + publicationScopeText[scope];
    localRoutingKeyEmpty = false;

  }
  else
  {
    currNullSequentialLevels++;
  }

  //*****************************
  // Add in security
  //*****************************

  if  (security != SP_NULLSECURITY)
  {

    AddDotsIfNeeded;

    localRoutingKey = localRoutingKey + publicationSecurityText[security];
    localRoutingKeyEmpty = false;

  }
  else
  {
    currNullSequentialLevels++;
  }

  //*****************************
  // Add in protocol
  //*****************************

  if  (protocol != SP_NULLPROTOCOL)
  {

    AddDotsIfNeeded;

    localRoutingKey = localRoutingKey + publicationProtocolText[protocol];
    localRoutingKeyEmpty = false;

  }
  else
  {
    currNullSequentialLevels++;
  }

  //*****************************
  // Add in publicationName
  //*****************************

  if (publicationName.length() > 0 )
  {
    AddDotsIfNeeded;

    localRoutingKey = localRoutingKey + publicationName;
    localRoutingKeyEmpty = false;
  }
  else 
  {

    // bump the sequential Null levels by one to account for the last field.
    currNullSequentialLevels ++;

  //*****************************
  // Add in final wildcards if needed
  //*****************************

    //  only need to do this if last component wasn't sent in, so
    //  it's an else case, rather than its own happy little world.  If
    //  additional components are added after publicationName, then we
    //  need to add a "normal" else case to publicationName

    if (localRoutingKeyEmpty == false)
    {
      // This is the else case for the very end of the routing key, so
      // we know that we need to add a "." for at least the last
      // component routing key.  We aren't empty, so there's at least
      // one real component in there.  Add a "."

      localRoutingKey = localRoutingKey + ".";
    }

    // Else case for localRoutingKeyEmpty not necessary.  If it's
    // true, then this is the first, last, and only entry of the
    // routing key!  So no dot required!  Time for a cold drink!

    // to "*" or to "#", that is the question...  The answer depends,
    // as usual, on whether there's more than one level.

    if (currNullSequentialLevels == 1) 
    {
      localRoutingKey = localRoutingKey + "*";
    }
    else
    {
      localRoutingKey = localRoutingKey + "#";
    }
  }

  //*****************************
  // Finis!
  //*****************************
				     
 return localRoutingKey;

}
#undef AddDotsIfNeeded


