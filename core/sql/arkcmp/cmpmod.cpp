/**********************************************************************
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
**********************************************************************/
/* -*-C++-*-
******************************************************************************
*
* File:         cmpmod.cpp
* Description:
*
* Created:      10/10/1995
* Language:     C++
*
*
******************************************************************************
*/

// runtime recompilation & rebinding of a statically compiled SQL module's:
//   SQL binary module, 
//   SQL statement, 
//   SQL cursor, 
//   SQL string
// was some Tandem developer's pipe dream that was coded but 
// was apparently never used and never worked!

// The design was to read from a module file and create another
// module file that contains the rebind object code.  After the
// rebind is successful, CATMAN will remove the original module
// file and rename the new module file to be the same name as
// the original file.

