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

package org.trafodion.sql;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.ClusterStatus;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.RegionLoad;
import org.apache.hadoop.hbase.ServerLoad;
import org.apache.hadoop.hbase.ServerName;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.HTable;

import java.io.IOException;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;
import org.apache.hadoop.hbase.util.Bytes;

class SizeInfo {
    int  numStores;
    int  numStoreFiles;
    Long storeUncompSize;
    Long storeFileSize;
    Long memStoreSize;
    Long readRequestsCount;
    Long writeRequestsCount;
};

public class TrafRegionStats {
    
    private final Map<byte[], SizeInfo> sizeInfoMap = 
        new TreeMap<byte[], SizeInfo>(Bytes.BYTES_COMPARATOR);
    
    static final String ENABLE_REGIONSIZECALCULATOR = "hbase.regionsizecalculator.enable";
    
    boolean enabled(Configuration configuration) {
        return configuration.getBoolean(ENABLE_REGIONSIZECALCULATOR, true);
    }
    
    /**
     * Returns size of given region in bytes. Returns 0 if region was not found.
     * */
    public SizeInfo getRegionSizeInfo(byte[] regionId) {
        SizeInfo sizeInfo = sizeInfoMap.get(regionId);
        return sizeInfo;
    }
    
    public Map<byte[], SizeInfo> getRegionSizeMap() {
        return Collections.unmodifiableMap(sizeInfoMap);
    }
    
    /**
     * Computes size of each region for table and given column families.
     * */
    public TrafRegionStats(HTable table) throws IOException {
        this(table, new HBaseAdmin(table.getConfiguration()));
    }
    
    public TrafRegionStats (HTable table, HBaseAdmin admin) throws IOException {
        
        try {
            if (!enabled(table.getConfiguration())) {
                System.out.println("Region size calculation disabled.");
                return;
            }
            
            //get regions for table
            Set<HRegionInfo> tableRegionInfos = table.getRegionLocations().keySet();
            Set<byte[]> tableRegions = new TreeSet<byte[]>(Bytes.BYTES_COMPARATOR);
            for (HRegionInfo regionInfo : tableRegionInfos) {
                tableRegions.add(regionInfo.getRegionName());
            }
            
            ClusterStatus clusterStatus = admin.getClusterStatus();
            Collection<ServerName> servers = clusterStatus.getServers();
            final long megaByte = 1024L * 1024L;
            
            //iterate all cluster regions, filter regions from our table and compute their size
            for (ServerName serverName: servers) {
                ServerLoad serverLoad = clusterStatus.getLoad(serverName);
                
                for (RegionLoad regionLoad: serverLoad.getRegionsLoad().values()) {
                    byte[] regionId = regionLoad.getName();
                    
                    if (tableRegions.contains(regionId)) {
                       
                        int  numStores = regionLoad.getStores();
                        int  numStoreFiles = regionLoad.getStorefiles();
                        Long storeUncompSizeBytes = regionLoad.getStoreUncompressedSizeMB() * megaByte;
                        Long storeFileSizeBytes = regionLoad.getStorefileSizeMB() * megaByte;
                        Long memStoreSizeBytes = regionLoad.getMemStoreSizeMB() * megaByte;

                        // this method is available in HBase 2.0.0
                        //                        Long lastMajorCompactionTs = regionLoad.getLastMajorCompactionTs();

                        SizeInfo sizeInfo = new SizeInfo();
                        sizeInfo.numStores = numStores;
                        sizeInfo.numStoreFiles = numStoreFiles;
                        sizeInfo.storeUncompSize = storeUncompSizeBytes;
                        sizeInfo.storeFileSize = storeFileSizeBytes;
                        sizeInfo.memStoreSize = memStoreSizeBytes;

                        sizeInfo.readRequestsCount = regionLoad.getReadRequestsCount();
                        sizeInfo.writeRequestsCount = regionLoad.getWriteRequestsCount();

                        sizeInfoMap.put(regionId, sizeInfo);
                        
                        //                        System.out.println("RegionNameAsString " + regionLoad.getNameAsString());
                     }
                }
            }
            
        } finally {
            admin.close();
        }
    }
}

