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
import org.apache.hadoop.hbase.client.Admin;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.Connection;
import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;

import java.io.IOException;
import java.util.Arrays;
import java.util.Collection;
import java.util.Collections;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;
import org.apache.hadoop.hbase.util.Bytes;

import java.lang.Byte;

class SizeInfo {
    String serverName;
    String regionName;
    String tableName;
    int  numStores;
    int  numStoreFiles;
    Long storeUncompSize;
    Long storeFileSize;
    Long memStoreSize;
    Long readRequestsCount;
    Long writeRequestsCount;
};

public class TrafRegionStats {

    static Logger logger = Logger.getLogger(TrafRegionStats.class.getName());
    private Admin hbAdmin;
    private ClusterStatus clusterStatus;
    private Collection<ServerName> servers;
    private ServerName server;
    private ServerLoad serverLoad;
    private Iterator iterServer;
    private Iterator iterRegion;
    private boolean firstTime;
    final long megaByte = 1024L * 1024L;


    private final Map<byte[], SizeInfo> sizeInfoMap = 
        new TreeMap<byte[], SizeInfo>(Bytes.BYTES_COMPARATOR);

    private SizeInfo currRegionSizeInfo = null;

    static {
    	String confFile = System.getProperty("trafodion.log4j.configFile");
        System.setProperty("trafodion.root", System.getenv("TRAF_HOME"));
    	if (confFile == null) 
           confFile = System.getenv("TRAF_CONF") + "/log4j.sql.config";
      	PropertyConfigurator.configure(confFile);
    }

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

    

    public TrafRegionStats (HTable table, Admin admin) throws IOException {
        
            if (!enabled(table.getConfiguration())) {
                logger.error("Region size calculation disabled for table " + table.getTableName());
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

                        byte[][] regNameParts = HRegionInfo.parseRegionName(regionLoad.getName());

                        SizeInfo sizeInfo = new SizeInfo();
                        sizeInfo.serverName = serverName.toShortString();
                        sizeInfo.regionName = new String(regNameParts[2]); 
                        sizeInfo.tableName = new String(regNameParts[0]);
                        sizeInfo.numStores = numStores;
                        sizeInfo.numStoreFiles = numStoreFiles;
                        sizeInfo.storeUncompSize = storeUncompSizeBytes;
                        sizeInfo.storeFileSize = storeFileSizeBytes;
                        sizeInfo.memStoreSize = memStoreSizeBytes;

                        sizeInfo.readRequestsCount = regionLoad.getReadRequestsCount();
                        sizeInfo.writeRequestsCount = regionLoad.getWriteRequestsCount();

                        sizeInfoMap.put(regionId, sizeInfo);
                        
                     }
                }
            }
            
    }


    public TrafRegionStats (Connection connection) throws IOException {
        hbAdmin = connection.getAdmin();
    }

    public boolean Open () throws IOException {
        clusterStatus = hbAdmin.getClusterStatus();
        servers = clusterStatus.getServers();
        iterServer = servers.iterator();

        firstTime = true;

        return true;
    }

    public boolean GetNextServer () throws IOException {
        
        if (! iterServer.hasNext())
            return false;
        
        server = (ServerName)iterServer.next();
        serverLoad = clusterStatus.getLoad(server);

        return true;
    }

    public boolean GetNextRegion () throws IOException {
        
        if ((firstTime) || (! iterRegion.hasNext())) {
            firstTime = false;
            if (! GetNextServer())
                return false;
            else
              iterRegion = serverLoad.getRegionsLoad().values().iterator();  
        }

        RegionLoad regionLoad = (RegionLoad)iterRegion.next();

        byte[] regionId = regionLoad.getName();
        
        int  numStores = regionLoad.getStores();
        int  numStoreFiles = regionLoad.getStorefiles();
        Long storeUncompSizeBytes = regionLoad.getStoreUncompressedSizeMB() * megaByte;
        Long storeFileSizeBytes = regionLoad.getStorefileSizeMB() * megaByte;
        Long memStoreSizeBytes = regionLoad.getMemStoreSizeMB() * megaByte;
        
        // this method is available in HBase 2.0.0
        // Long lastMajorCompactionTs = regionLoad.getLastMajorCompactionTs();
        
        byte[][] regNameParts = HRegionInfo.parseRegionName(regionLoad.getName());

        currRegionSizeInfo = new SizeInfo();

        currRegionSizeInfo.serverName = new String(server.toShortString());
        currRegionSizeInfo.regionName = new String(regNameParts[2]);

  
        currRegionSizeInfo.tableName = new String(regNameParts[0]);
        currRegionSizeInfo.numStores = numStores;
        currRegionSizeInfo.numStoreFiles = numStoreFiles;
        currRegionSizeInfo.storeUncompSize = storeUncompSizeBytes;
        currRegionSizeInfo.storeFileSize = storeFileSizeBytes;
        currRegionSizeInfo.memStoreSize = memStoreSizeBytes;
        
        currRegionSizeInfo.readRequestsCount = regionLoad.getReadRequestsCount();
        currRegionSizeInfo.writeRequestsCount = regionLoad.getWriteRequestsCount();

        return true;
    }

    public SizeInfo getCurrRegionSizeInfo() {
        return currRegionSizeInfo;
    }
    
    public boolean Close () throws IOException {
        
        hbAdmin.close();

        return true;
    }

}

