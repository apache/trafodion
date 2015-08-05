/* -*-C++-*-
 *****************************************************************************
 *
 * File:         test1.C
 * Description:  driver to test my exception handling mechanism
 *               
 *               
 * Created:      5/20/95
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


#include "EHCommonDefs.h"


#include <iostream>

#include "EHException.h"                                // <---

// -----------------------------------------------------------------------
// global test routine forward declarations
// -----------------------------------------------------------------------

void DoSomething();
void DoMore(); 

// -----------------------------------------------------------------------
// main entry
// -----------------------------------------------------------------------
main()
{
  cout << "in main - beginning of test program" << endl;

  EH_REGISTER(EH_ARITHMETIC_OVERFLOW);                  // <---
  EH_REGISTER(EH_OUT_OF_RANGE);                         // <---
  EH_TRY                                                // <---
  {
    cout << "in main - in try block before calling DoSomething" << endl;
    DoSomething();
  }
  EH_END_TRY                                            // <---
  EH_CATCH(EH_ARITHMETIC_OVERFLOW)                      // <---
  {
    cout << "in main - in Arithmetic Overflow catch block" << endl;
  }
  EH_CATCH(EH_OUT_OF_RANGE)                             // <---
  {
    cout << "in main - in Out of Range catch block" << endl;
  }

  cout << "in main - end of test program" << endl
       << "-----------------------------" << endl;
  return 0;
}

// -----------------------------------------------------------------------
// global test routines
// -----------------------------------------------------------------------

void
DoSomething ()
{
  cout << "in DoSomething - before try block" << endl;

  DoMore();

  cout << "file " << __FILE__ << " - line " << __LINE__ << " : "
    << "This line is not supposed to be printed" << endl;
}

void
DoMore()
{
  cout << "in DoMore - before throw Out of Range statement" << endl;

  EH_THROW(EH_OUT_OF_RANGE);                            // <---

  cout << "file " << __FILE__ << " - line " << __LINE__ << " : "
    << "This line is not supposed to be printed" << endl;
}
