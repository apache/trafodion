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

#ifndef EXTERNROUTINGKEY_INT_H
#define EXTERNROUTINGKEY_INT_H

// internal-only definitions for the external->internal routing key
// translator

#include "wrapper/externroutingkey.h"

#ifndef WIN32
#define SP_DLL_EXPORT
#else
#define SP_DLL_EXPORT __declspec(dllexport)
#endif

//********************************************************
//  GetNextLevel - scans to next "." or end of string
//********************************************************

SP_DLL_EXPORT string GetNextLevel (const string &scanString, size_t startPoint);

//********************************************************
// GetCompGroupFromCompType - maps publication to component group
//********************************************************

SP_DLL_EXPORT spExtRoutingKeyComponentGroupType GetCompGroupFromCompType (spExtRoutingKeyCompType keyType);

//********************************************************
// GetComponentTypeFromString - maps publication to component type
//********************************************************

SP_DLL_EXPORT spExtRoutingKeyCompType GetComponentTypeFromString (const string *componentString);

//********************************************************
// Get*FromComponentType -- maps component type to internal
//              componentEnum.  One for each componenent level
//********************************************************

SP_DLL_EXPORT SP_PublicationCategoryType GetCategoryFromComponentType ( const spExtRoutingKeyCompType compType);

SP_DLL_EXPORT SP_PublicationPackageType GetPackageFromComponentType ( const spExtRoutingKeyCompType compType);

SP_DLL_EXPORT SP_PublicationScopeType GetScopeFromComponentType ( const spExtRoutingKeyCompType compType);

SP_DLL_EXPORT SP_PublicationSecurityType GetSecurityFromComponentType ( const spExtRoutingKeyCompType compType);

SP_DLL_EXPORT SP_PublicationProtocolType GetProtocolFromComponentType ( const spExtRoutingKeyCompType compType);

#endif
