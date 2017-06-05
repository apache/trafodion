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

import java.util.ArrayList;
import java.util.List;

import org.apache.log4j.Logger;

public class SimpleUDRTest extends SimpleUDR {
    private static Logger log = Logger.getLogger(SimpleUDRTest.class.getName());

    @Override
    public void prepare(UDRInvocationInfo info, UDRPlanInfo plan) {

    }

    @Override
    public void close(UDRInvocationInfo info, UDRPlanInfo plan) {

    }

    @Override
    public void execute(TupleInfo tuple) throws UDRException {
        if (log.isDebugEnabled()) {
            log.debug("enter exectue...");
        }
        List emitList = new ArrayList();
        for (int i = 0; i < getColumns().size(); i++) {
            if (log.isDebugEnabled()) {
                log.debug("tuple.getString(" + i + ") = " + tuple.getString(i));
            }
            emitList.add(tuple.getString(i));
        }
        emitRow(emitList.toArray());
        if (log.isDebugEnabled()) {
            log.debug("leave exectue...");
        }
    }

}

