// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2008-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef __SPREGISTRY_H
#define __SPREGISTRY_H

#include <cstdlib>
#include <ctime>
#include <cstdio>
#include <iostream>
#include <sstream>

#include <string.h>
#include "seabed/ms.h"

#define CLUSTER_GROUP "CLUSTER"

int sp_reg_get (MS_Mon_ConfigType   pv_type,
                char                *pp_group,
                char                *pp_key,
                std::string         &pp_value);

int sp_reg_set (MS_Mon_ConfigType const   pv_type,
                char const * const        pp_group,
                char const * const        pp_key,
                char const * const        pp_value);



#endif


