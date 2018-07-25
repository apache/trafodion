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

#include "sqludr.h"

using namespace tmudr;

// sample invocation of the SERIES TMUDF:
//
// SELECT * FROM UDF(series(1, 100, 2));
// SELECT * FROM UDF(series(100, 1, -2));
// SELECT * FROM UDF(series(1, 100));

/* Step 1: derive a class from tmudr::UDR*/

class series : public UDR
{
public:
  // determine output columns dynamically at compile time
  void describeParamsAndColumns(UDRInvocationInfo &info);
  
  // override the runtime method
  virtual void processData(UDRInvocationInfo &info,
                           UDRPlanInfo &plan);
};

/* Step 2: Create a factory method*/

extern "C" UDR * SERIES()
{
  return new series();
}

/*  Step 3: Write the actual UDF code*/
void series::describeParamsAndColumns(
                           UDRInvocationInfo &info)
{
  // We always expect one input parameter
  int paramCount = info.par().getNumColumns();
  if (paramCount != 2 && paramCount != 3)
    throw UDRException(38001,
                       "Expecting two or three input parameters");

  if (info.par().getType(0).getSQLTypeClass() != info.par().getType(1).getSQLTypeClass())
    throw UDRException(38001,
                       "The data type of the first two parameters should be same");

  if (info.par().getType(0).getSQLTypeClass() != TypeInfo::NUMERIC_TYPE)
    throw UDRException(38001,
                       "Only support to generate series for exact numerics type");

  for (int i=0; i<paramCount; i++)
    info.addFormalParameter(info.par().getColumn(i));

  info.out().addLongColumn("ELEMENT");

  // Set the function type, sessionize behaves like
  // a reducer in MapReduce. Session ids are local
  // within rows that share the same id column value.
  info.setFuncType(UDRInvocationInfo::MAPPER);
}

void series::processData(UDRInvocationInfo &info,
                                UDRPlanInfo &plan)
{
    long begin = info.par().getLong(0);
    long end = info.par().getLong(1);
    long step;
    if (info.par().getNumColumns() == 2)
      step = 1;
    else
      step = info.par().getLong(2);

    if (0 == step)
      throw UDRException(38001,
                       "Step cannot be a zero");
    if (begin < end && step < 0)
      return;

    if (step > 0 )
      for (long i=begin; i<=end;)
      {
          info.out().setLong(0, i);
          emitRow(info);
          i += step;
      }
    else
      for (long i=begin; i>=end;)
      {
          info.out().setLong(0, i);
          emitRow(info);
          i += step;
      }
}
