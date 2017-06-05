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
import java.util.Date;
import java.util.List;

import org.apache.log4j.Logger;
import org.trafodion.sql.udr.TypeInfo.SQLTypeCode;

public abstract class SimpleUDR extends UDR {
    private static Logger log = Logger.getLogger(SimpleUDR.class.getName());
    private List<ColumnInfo> columns = new ArrayList<ColumnInfo>();
    UDRInvocationInfo info = null;


    public abstract void prepare(UDRInvocationInfo info, UDRPlanInfo plan);

    /**
 *      * recycle the init value
 *           */
    public abstract void close(UDRInvocationInfo info, UDRPlanInfo plan);

    /**
 *      * @throws UDRException
 *           * 
 *                */
    public abstract void execute(TupleInfo tuple) throws UDRException;

    public List<ColumnInfo> getColumns() {
        return columns;
    }

    public int getNumColumns() {
        return columns.size();
    }

    @Override
    public final void processData(UDRInvocationInfo info, UDRPlanInfo plan) throws UDRException {
        this.info = info;
        TableInfo tblInfo = info.in();
        for (int i = 0; i < tblInfo.getNumColumns(); i++) {
            columns.add(tblInfo.getColumn(i));
        }

        prepare(info, plan);

        while (getNextRow(info)) {
            tblInfo = info.in(0);

            execute(tblInfo);
        }

        close(info, plan);
    }

    public void emitRow(Object... values) throws UDRException {
        if (log.isDebugEnabled()) {
            log.debug("enter emitRow...");
        }
        for (int i = 0; i < values.length; i++) {
            if (null == values[i]) {
                info.out().setNull(i);
                continue;
            }
            if (log.isDebugEnabled()) {
                log.debug("info.out().setValue=" + values[i] + " , value type=" + values[i].getClass());
            }
            TypeInfo type = info.out().getColumn(i).getType();
            SQLTypeCode sqlType_ = type.getSQLType();

            if (values[i] instanceof Integer) {
                info.out().setInt(i, Integer.valueOf(values[i].toString()));
            } else if (values[i] instanceof Long) {
                info.out().setLong(i, Long.valueOf(values[i].toString()));
            } else if (values[i] instanceof String) {
                info.out().setString(i, values[i].toString());
            } else if (values[i] instanceof Date) {
                info.out().setTime(i, new Date(Date.parse(values[i].toString())));
            } else {
                // throws new UDRException(sqlState, printf_format, args);
            }
        }
        emitRow(info);
    }
}
