// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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

#include "sqludr.h"

using namespace tmudr;

class Sessionize : public UDR
{
public:

  // override the runtime method
  void processData(UDRInvocationInfo &info,
                   UDRPlanInfo &plan);
};

extern "C" UDR * SESSIONIZE()
{
  return new Sessionize();
}

void Sessionize::processData(UDRInvocationInfo &info,
                             UDRPlanInfo &plan)
{
  // this is just a dummy implementation, the test
  // does not rely on the generated results

  // loop over input rows
  while (getNextRow(info))
  {
    info.out().setString(0, "userid");
    info.out().setLong(1, 999);
    info.out().setLong(2, 9999);

    emitRow(info);
  }
}
