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

package org.trafodion.sql.udr;

/**
 *  Compile time data owned by the UDR writer
 *
 *  <p> When attached to a UDRInvocationInfo object, keeps context
 *  between compiler interface calls for this object. This class
 *  can also be attached to a UDRPlanInfo object, to keep state
 *  between plan alternatives for a UDR invocation. The
 *  info is NOT passed to the run time methods, use
 *  UDRPlanInfo#addPlanData() for that.
 */
public class UDRWriterCompileTimeData
{  
    /**
     *  Default constructor.
     *
     *  <p> UDR writers can derive from this class to store state between
     *  the calls of the compiler interface.
     */
    public UDRWriterCompileTimeData() {
    }
    
    // Functions for debugging
    public void print() {
        System.out.print("no print method provided for UDR Writer compile time data\n");
    }
    
};


