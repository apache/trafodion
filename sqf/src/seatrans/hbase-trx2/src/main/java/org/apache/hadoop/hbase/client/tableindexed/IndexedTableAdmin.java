/**
 * Copyright 2009 The Apache Software Foundation Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements. See the NOTICE file distributed with this work for additional information regarding
 * copyright ownership. The ASF licenses this file to you under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0 Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and limitations under the
 * License.
 */
package org.apache.hadoop.hbase.client.tableindexed;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map.Entry;
import java.util.NavigableMap;
import java.util.Set;
import java.util.SortedMap;
import java.util.TreeMap;
import java.util.TreeSet;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.MasterNotRunningException;
import org.apache.hadoop.hbase.ZooKeeperConnectionException;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.regionserver.tableindexed.IndexMaintenanceUtils;
import org.apache.hadoop.hbase.util.Bytes;

/**
 * Extension of HBaseAdmin that creates indexed tables.
 */
public class IndexedTableAdmin extends HBaseAdmin {

    private static final Log LOG = LogFactory.getLog(IndexedTableAdmin.class);
    private static final int DEFAULT_BATCH_SIZE = 100;
    private int batchSize = DEFAULT_BATCH_SIZE;

    private Configuration config = null;
    /**
     * Constructor
     * 
     * @param conf Configuration object
     * @throws MasterNotRunningException
     * @throws ZooKeeperConnectionException
     */
    public IndexedTableAdmin(final Configuration conf) throws MasterNotRunningException, ZooKeeperConnectionException {
        super(conf);
        config = conf;
    }

    /**
     * Creates a new indexed table
     * 
     * @param desc table descriptor for table
     * @throws IOException
     */
    public void createIndexedTable(final IndexedTableDescriptor desc) throws IOException {
        super.createTable(desc.getBaseTableDescriptor());
        this.createIndexTables(desc);
    }

    private void createIndexTables(final IndexedTableDescriptor indexDesc) throws IOException {
        byte[] baseTableName = indexDesc.getBaseTableDescriptor().getName();
        for (IndexSpecification indexSpec : indexDesc.getIndexes()) {
            HTableDescriptor indexTableDesc = createIndexTableDesc(baseTableName, indexSpec);
            super.createTable(indexTableDesc);
        }
    }

    private HTableDescriptor createIndexTableDesc(final byte[] baseTableName, final IndexSpecification indexSpec) {
        HTableDescriptor indexTableDesc = new HTableDescriptor(indexSpec.getIndexedTableName(baseTableName));
        Set<byte[]> families = new TreeSet<byte[]>(Bytes.BYTES_COMPARATOR);
        families.add(IndexedTable.INDEX_COL_FAMILY);
        for (byte[] column : indexSpec.getAllColumns()) {
            families.add(KeyValue.parseColumn(column)[0]);
        }

        for (byte[] colFamily : families) {
            indexTableDesc.addFamily(new HColumnDescriptor(colFamily));
        }

        return indexTableDesc;
    }

    /**
     * Remove an index for a table.
     * 
     * @throws IOException
     */
    public void removeIndex(final byte[] baseTableName, final String indexId) throws IOException {
        super.disableTable(baseTableName);
        HTableDescriptor desc = super.getTableDescriptor(baseTableName);
        IndexedTableDescriptor indexDesc = new IndexedTableDescriptor(desc);
        IndexSpecification spec = indexDesc.getIndex(indexId);
        indexDesc.removeIndex(indexId);
        byte[] indexTableName = spec.getIndexedTableName(baseTableName);
        this.disableTable(indexTableName);
        this.deleteTable(indexTableName);
        super.modifyTable(baseTableName, desc);
        super.enableTable(baseTableName);
        LOG.info("Dropped index " + Bytes.toString(indexTableName));
    }

    /** Add an index to a table. */
    public void addIndex(final byte[] baseTableName, final IndexSpecification indexSpec) throws IOException {
        LOG.info("Adding index [" + indexSpec.getIndexId() + "] to existing table [" + Bytes.toString(baseTableName)
                + "], this may take a long time");
        // TODO, make table read-only
        LOG.warn("Not putting table in readonly, if its being written to, the index may get out of sync");
        HTableDescriptor indexTableDesc = createIndexTableDesc(baseTableName, indexSpec);
        super.createTable(indexTableDesc);
        super.disableTable(baseTableName);
        IndexedTableDescriptor indexDesc = new IndexedTableDescriptor(super.getTableDescriptor(baseTableName));
        indexDesc.addIndex(indexSpec);
        super.modifyTable(baseTableName, indexDesc.getBaseTableDescriptor());
        super.enableTable(baseTableName);
        reIndexTable(baseTableName, indexSpec);
    }

    private void reIndexTable(final byte[] baseTableName, final IndexSpecification indexSpec) throws IOException {

    	HTable baseTable = new HTable(config, baseTableName);
        HTable indexTable = new HTable(config, indexSpec.getIndexedTableName(baseTableName));
        Scan scan = new Scan();
        List<Put> batch = new ArrayList<Put>(batchSize + 1);

        byte[][] allColumns = indexSpec.getAllColumns();
        
        for(byte[] column : allColumns) {
        	scan.addColumn(column, Bytes.toBytes(""));
        }
        //scan.addColumns(indexSpec.getAllColumns());
        for (Result rowResult : baseTable.getScanner(scan)) {
            SortedMap<byte[], byte[]> columnValues = new TreeMap<byte[], byte[]>(Bytes.BYTES_COMPARATOR);
            for (Entry<byte[], NavigableMap<byte[], byte[]>> familyEntry : rowResult.getNoVersionMap().entrySet()) {
                for (Entry<byte[], byte[]> cellEntry : familyEntry.getValue().entrySet()) {
                    columnValues.put(Bytes.add(familyEntry.getKey(), Bytes.toBytes(":"), cellEntry.getKey()),
                        cellEntry.getValue());
                }
            }
            if (IndexMaintenanceUtils.doesApplyToIndex(indexSpec, columnValues)) {
                Put indexUpdate = IndexMaintenanceUtils.createIndexUpdate(indexSpec, rowResult.getRow(), columnValues);
                batch.add(indexUpdate);
                if (batch.size() >= batchSize) {
                    flushBatch(batch, indexTable);
                }
            }

        }

        flushBatch(batch, indexTable);
    }

    private void flushBatch(final List<Put> batch, final HTable indexTable) throws IOException {
        if (!batch.isEmpty()) {
            indexTable.put(batch);
            batch.clear();
        }
    }

    /**
     * Set the batch size with which to write to the index when re-indexing. If not specified, DEFAULT_BATCH_SIZE used.
     * 
     * @param batchSize The batchSize to set.
     */
    public void setBatchSize(final int batchSize) {
        this.batchSize = batchSize;
    }
}
