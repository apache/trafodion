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

#ifndef WRAPPER_INTERNALROUTINGKEY
#define WRAPPER_INTERNALROUTINGKEY

//*****************************************************************************
//
//  This file contains the internal definitions needed to support the
//  routing key class.  These constructs aren't really necessary for
//  clients of the class to understand and are subject to change, so
//  they're encapsulated here.
//
//*****************************************************************************



#include <string>

using std::string;

//*****************************************************************************
//
//  A full routing key has a minimum length based on the required
//  number of levels.  Right now, those levels include 6 different
//  components, so the minimum length is the minimum component length
//  (1 character!) and the level separators.  "a.b.c.d.e.f"
//
//*****************************************************************************

#define ROUTINGKEY_MINLENGTH 11

//*****************************************************************************
//  The number of levels in a routing key.
//*****************************************************************************
#define ROUTINGKEY_NUMLEVELS 6

#define ROUTINGKEY_NUMFIXEDLEVELS 5

//*****************************************************************************
// The level separater on a routing key
//*****************************************************************************
#define ROUTINGKEY_LEVELSEPARATOR "."
#define ROUTINGKEY_LEVELSEPARATOR_LEN 1


//*****************************************************************************
//
//  GARBAGELEVEL is used to build routing keys at a component level
//  when not all fields are available.  A routing key containing
//  GARBAGELEVEL is by definition not valid.  Sending a message with a
//  GARBAGELEVEL routing key component is an internal programming
//  error
//
//*****************************************************************************

#define ROUTINGKEY_GARBAGELEVEL "GARBAGELEVEL"

//*****************************************************************************
//
// Protofile extension accounting when we're mapping between proto files and
// routing keys.
//
//*****************************************************************************

#define ROUTINGKEY_PROTOFILEEXTENSION "proto"
#define ROUTINGKEY_PROTOFILEEXTENSION_LENGTH 6


//*****************************************************************************
//*****************************************************************************
//
//  String array tables for the various levels of publication strings.
//  Caveat Emptor -- if you add a new enum in routingkey.h, you need
//  to add a string here *and* add it to the corresponding static
//  array as well.
//
//*****************************************************************************
//*****************************************************************************


//****************************
// PublicationScopeType
//****************************

#define SP_NULLSCOPE_TEXT        "nullscopetext"
#define SP_CLUSTER_TEXT          "cluster"
#define SP_INSTANCE_TEXT         "instance"
#define SP_TENANT_TEXT           "tenant"
#define SP_MAXSCOPE_TEXT         "maxscopetext"

const static string publicationScopeText [SP_MAXSCOPE+1] =
  {
    SP_NULLSCOPE_TEXT,
    SP_CLUSTER_TEXT,
    SP_INSTANCE_TEXT,
    SP_TENANT_TEXT,
    SP_MAXSCOPE_TEXT
  };

//****************************
// PublicationSecurityType
//****************************

#define SP_NULLSECURITY_TEXT     "nullsecuritytext"
#define SP_PUBLIC_TEXT           "public"
#define SP_PRIVATE_TEXT          "private"
#define SP_MAXSECURITY_TEXT      "maxsecuritytext"

const static string publicationSecurityText [SP_MAXSECURITY+1] =
  {
    SP_NULLSECURITY_TEXT,
    SP_PUBLIC_TEXT,
    SP_PRIVATE_TEXT,
    SP_MAXSECURITY_TEXT
  };

//****************************
// PublicationCategoryType
//****************************

#define SP_NULLCATEGORY_TEXT     "nullcategorytext"
#define SP_EVENT_TEXT            "event"
#define SP_PERF_STAT_TEXT        "performance_stat"
#define SP_HEALTH_STATE_TEXT     "health_state"
#define SP_SECURITY_TEXT         "security"
#define SP_MAXCATEGORY_TEXT      "maxcategorytext"

const static string publicationCategoryText [SP_MAXCATEGORY+1] =
  {
    SP_NULLCATEGORY_TEXT,
    SP_EVENT_TEXT,
    SP_PERF_STAT_TEXT,
    SP_HEALTH_STATE_TEXT,
    SP_SECURITY_TEXT,
    SP_MAXCATEGORY_TEXT
  };

//****************************
// PublicationProtocolType
//****************************

#define SP_NULLPROTOCOL_TEXT     "nullprotocoltext"
#define SP_GPBPROTOCOL_TEXT      "gpb"
#define SP_XMLPROTOCOL_TEXT      "xml"
#define SP_MAXPROTOCOL_TEXT      "maxprotocoltext"

const static string publicationProtocolText [SP_MAXPROTOCOL+1] =
  {
    SP_NULLPROTOCOL_TEXT,
    SP_GPBPROTOCOL_TEXT,
    SP_XMLPROTOCOL_TEXT,
    SP_MAXPROTOCOL_TEXT
  };

#endif
