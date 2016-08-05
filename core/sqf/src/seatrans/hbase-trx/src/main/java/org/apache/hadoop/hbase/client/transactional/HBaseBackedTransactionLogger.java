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

package org.apache.hadoop.hbase.client.transactional;

import java.io.IOException;
import java.util.Random;

import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.client.Delete;
import org.apache.hadoop.hbase.client.Get;
import org.apache.hadoop.hbase.client.Admin;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.hbase.client.Connection;
import org.apache.hadoop.hbase.client.HTableInterface;
import org.apache.hadoop.hbase.client.HTablePool;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.util.Bytes;

public class HBaseBackedTransactionLogger implements TransactionLogger {

    /** The name of the transaction status table. */
    public static final String TABLE_NAME = "__GLOBAL_TRX_LOG__";


    private static final byte[] INFO_FAMILY = Bytes.toBytes("Info");
    /**
     * Column which holds the transaction status.
     */
    private static final byte[] STATUS_QUALIFIER = Bytes.toBytes("Status");


    /**
     * Create the global transaction table with the given configuration.
     * 
     * @param Connection
     * @throws IOException
     */
    public static void createTable(Connection connection) throws IOException {
        HTableDescriptor tableDesc = new HTableDescriptor(TABLE_NAME);
        tableDesc.addFamily(new HColumnDescriptor(INFO_FAMILY));
        Admin admin = connection.getAdmin();
        if (! admin.tableExists(TableName.valueOf(TABLE_NAME))) {
            admin.createTable(tableDesc);
        }
        admin.close();
    }

    private Random random = new Random();
    private HTablePool tablePool = new HTablePool();

    private HTableInterface getTable() {
        return tablePool.getTable(TABLE_NAME);
    }

    private void putTable(final HTableInterface t) throws IOException {
        if (t == null) {
            return;
        }
        tablePool.putTable(t);
    }

    public HBaseBackedTransactionLogger(Connection connection) throws IOException {
        initTable(connection);
    }

    private void initTable(Connection connection) throws IOException {
        boolean retcode ;
        Admin admin = connection.getAdmin();
        retcode =  (!admin.tableExists(TableName.valueOf(TABLE_NAME)));
        admin.close();
        if (retcode) 
            throw new RuntimeException("Table not created. Call createTable() first");
    }

    public long createNewTransactionLog() {
        long id;
        TransactionStatus existing;

        do {
            id = random.nextLong();
            try {
                existing = getStatusForTransaction(id);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        } while (existing != null);

        setStatusForTransaction(id, TransactionStatus.PENDING);

        return id;
    }
    
    public long createNewTransactionLog(long transactionId) {
        TransactionStatus existing;

        do {
            //id = random.nextLong();
            try {
                existing = getStatusForTransaction(transactionId);
            } catch (IOException e) {
                throw new RuntimeException(e);
            }
        } while (existing != null);

        setStatusForTransaction(transactionId, TransactionStatus.PENDING);

        return transactionId;
    }

    public TransactionStatus getStatusForTransaction(final long transactionId) throws IOException {
        HTableInterface table = getTable();
        try {
            Result result = table.get(new Get(getRow(transactionId)));
            if (result == null || result.isEmpty()) {
                return null;
            }
            byte[] statusCell = result.getValue(INFO_FAMILY, STATUS_QUALIFIER);
            if (statusCell == null) {
                throw new RuntimeException("No status cell for row " + transactionId);
            }
            String statusString = Bytes.toString(statusCell);
            return TransactionStatus.valueOf(statusString);

        } catch (IOException e) {
            throw new RuntimeException(e);
        } finally {
            putTable(table);
        }
    }

    private byte[] getRow(final long transactionId) {
        return Bytes.toBytes("" + transactionId);
    }

    public void setStatusForTransaction(final long transactionId, final TransactionStatus status)  {
        Put update = new Put(getRow(transactionId));
        update.add(INFO_FAMILY, STATUS_QUALIFIER, HConstants.LATEST_TIMESTAMP, Bytes.toBytes(status.name()));

        HTableInterface table = getTable();
        try {
            table.put(update);
        } catch (IOException e) {
            throw new RuntimeException(e);
        } finally {
            try {
				putTable(table);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
        }
    }

    public void forgetTransaction(final long transactionId) {
        Delete delete = new Delete(getRow(transactionId));

        HTableInterface table = getTable();
        try {
            table.delete(delete);
        } catch (IOException e) {
            throw new RuntimeException(e);
        } finally {
            try {
				putTable(table);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
        }
    }

}
