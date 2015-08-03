#ifndef GEN_RESOURCES_H
#define GEN_RESOURCES_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         GenResources.h
 * Description:  Code to retrieve resource-related defaults from the defaults
 *               table and to add them to a structure in the generated
 *               plan.
 *		 
 *               
 * Created:      1/9/99
 * Language:     C++
 *
 *
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
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------
// Forward declarations
// -----------------------------------------------------------------------
class Generator;
class ExScratchFileOptions;


// -----------------------------------------------------------------------
// Contents of this file
// -----------------------------------------------------------------------

ExScratchFileOptions *genScratchFileOptions(Generator *generator);


#endif /* GEN_RESOURCES_H */
