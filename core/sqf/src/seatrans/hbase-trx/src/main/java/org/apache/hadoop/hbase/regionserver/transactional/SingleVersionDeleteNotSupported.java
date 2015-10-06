/**
* @@@ START COPYRIGHT @@@
*
* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at
*
*   http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.
*
* @@@ END COPYRIGHT @@@
**/

package org.apache.hadoop.hbase.regionserver.transactional;

import java.util.Collection;
import java.util.List;

import org.apache.hadoop.hbase.DoNotRetryIOException;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.KeyValue.Type;
import org.apache.hadoop.hbase.client.Delete;

/**
 * Used to fail-fast when Delete.addColumn is used instead of Delete.addColumns. The transactional layer does not yet
 * support the deletion of a single row.cell.version.
 */
public class SingleVersionDeleteNotSupported extends DoNotRetryIOException {

    private static final long serialVersionUID = 1297446454511704139L;

    /**
     * default constructor
     */
    public SingleVersionDeleteNotSupported() {
        super("Transaction Manager" 
                + " does not support single version deletes. Use Delete.addColumns()"
                + " to remove all versions of the given row, column.");
    }

    /**
     * Validate that a single-version delete is not used. Having to do this for now because transactional delete
     * mechansim will currently treat DeleteColumn the same as Delete which could cause confusion.
     */
    public static void validateDelete(final Delete delete) throws SingleVersionDeleteNotSupported {
        Collection<List<KeyValue>> values = delete.getFamilyMap().values();
        for (List<KeyValue> value : values) {
            for (KeyValue kv : value) {
                if (Type.Delete.getCode() == kv.getType()) {
                    throw new SingleVersionDeleteNotSupported();
                }
            }
        }
    }
}
