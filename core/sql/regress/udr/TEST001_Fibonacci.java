/**********************************************************************
Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.
**********************************************************************/

import org.trafodion.sql.udr.*;

class TEST001_Fibonacci extends UDR
{

  public TEST001_Fibonacci()
  {}

  // override any methods where the UDF author would
  // like to change the default behavior

  @Override
  public void processData(UDRInvocationInfo info,
                   UDRPlanInfo plan)
      throws UDRException
  {
    // input parameters: (int startRow, int numResultRows)
    int startRow = info.par().getInt(0);
    int numResultRows = info.par().getInt(1);
    long fibonacciNumber = 0;
    long previousResult = 1;
    long temp = 0;
    int ordinal = 0;

    // produce fibonacci numbers and emit rows
    // ---------------------------------------
    while (true)
      {
        if (ordinal >= startRow)
          {
            // set result parameters (int ordinal, long fibonacci_number)
            info.out().setInt(0, ordinal);
            info.out().setLong(1, fibonacciNumber);
            emitRow(info);
          }

        // did we produce numResultRows already?
        if (++ordinal >= startRow+numResultRows)
          break;

        if (fibonacciNumber > Long.MAX_VALUE/2)
          throw new UDRException(38001, "Upper limit exceeded");

        // pre-compute the next row
        temp = fibonacciNumber;
        fibonacciNumber += previousResult;
        previousResult = temp;
      }
  }

};
