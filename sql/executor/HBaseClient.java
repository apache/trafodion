// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@

package org.trafodion.sql.HBaseAccess;

import com.google.protobuf.ServiceException;
import java.io.IOException;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.ArrayList;
import java.net.URI;
import java.net.URISyntaxException;

import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.NamespaceDescriptor;
import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.PoolMap;
import org.apache.hadoop.hbase.util.PoolMap.PoolType;


import org.apache.hadoop.hbase.security.access.AccessController;
import org.apache.hadoop.hbase.security.access.UserPermission;
import org.apache.hadoop.hbase.security.access.Permission;
import org.apache.hadoop.hbase.MasterNotRunningException;
import org.apache.hadoop.hbase.ZooKeeperConnectionException;
import org.apache.hadoop.hbase.snapshot.SnapshotCreationException;
import org.apache.hadoop.hbase.protobuf.generated.HBaseProtos.SnapshotDescription;
import org.apache.hadoop.hbase.protobuf.generated.HBaseProtos.SnapshotDescription.Type;
//import org.apache.hadoop.hbase.protobuf.generated.HBaseProtos;

import org.apache.hadoop.hbase.io.compress.Compression.Algorithm;
//import org.apache.hadoop.hbase.io.hfile.Compression.Algorithm;
import org.apache.hadoop.hbase.io.encoding.DataBlockEncoding;
import org.apache.hadoop.hbase.regionserver.BloomType; 
//import org.apache.hadoop.hbase.regionserver.StoreFile.BloomType ;
import org.apache.hadoop.hbase.regionserver.KeyPrefixRegionSplitPolicy;
import org.apache.hadoop.hbase.client.Durability;
import org.trafodion.sql.HBaseAccess.HTableClient;

import org.apache.hadoop.hbase.ServerLoad;
import org.apache.hadoop.hbase.RegionLoad;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.ClusterStatus;
import org.apache.hadoop.hbase.ServerName;
import java.util.Set;
import java.util.TreeSet;

import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.hbase.io.hfile.CacheConfig;
import org.apache.hadoop.hbase.io.hfile.HFile;
import org.apache.hadoop.hbase.io.hfile.HFileScanner;
import org.apache.hadoop.hbase.regionserver.StoreFileInfo;
import org.apache.hadoop.hbase.HConstants;

public class HBaseClient {

    static Logger logger = Logger.getLogger(HBaseClient.class.getName());
    public static Configuration config = HBaseConfiguration.create();
    String lastError;
    private PoolMap<String, HTableClient> hTableClientsFree;
    private PoolMap<String, HTableClient> hTableClientsInUse;
    // this set of constants MUST be kept in sync with the C++ enum in
    // ExpHbaseDefs.h
    public static final int HBASE_NAME = 0;
    public static final int HBASE_MAX_VERSIONS = 1;
    public static final int HBASE_MIN_VERSIONS = 2;
    public static final int HBASE_TTL = 3;
    public static final int HBASE_BLOCKCACHE = 4;
    public static final int HBASE_IN_MEMORY = 5;
    public static final int HBASE_COMPRESSION = 6;
    public static final int HBASE_BLOOMFILTER = 7;
    public static final int HBASE_BLOCKSIZE = 8;
    public static final int HBASE_DATA_BLOCK_ENCODING = 9;
    public static final int HBASE_CACHE_BLOOMS_ON_WRITE = 10;
    public static final int HBASE_CACHE_DATA_ON_WRITE = 11;
    public static final int HBASE_CACHE_INDEXES_ON_WRITE = 12;
    public static final int HBASE_COMPACT_COMPRESSION = 13;
    public static final int HBASE_PREFIX_LENGTH_KEY = 14;
    public static final int HBASE_EVICT_BLOCKS_ON_CLOSE = 15;
    public static final int HBASE_KEEP_DELETED_CELLS = 16;
    public static final int HBASE_REPLICATION_SCOPE = 17;
    public static final int HBASE_MAX_FILESIZE = 18;
    public static final int HBASE_COMPACT = 19;
    public static final int HBASE_DURABILITY = 20;
    public static final int HBASE_MEMSTORE_FLUSH_SIZE = 21;
    public static final int HBASE_SPLIT_POLICY = 22;

    
    public HBaseClient() {
      if (hTableClientsFree == null)
         hTableClientsFree = new PoolMap<String, HTableClient>
                 (PoolType.Reusable, Integer.MAX_VALUE);
      hTableClientsInUse = new PoolMap<String, HTableClient>
               (PoolType.Reusable, Integer.MAX_VALUE);
    }

    public String getLastError() {
        return lastError;
    }

    void setLastError(String err) {
        lastError = err;
    }

    static {
    	//Some clients of this class e.g., DcsServer/JdbcT2 
    	//want to use use their own log4j.properties file instead
    	//of the /conf/lo4j.hdf.config so they can see their
    	//log events in their own log files or console.
    	//So, check for alternate log4j.properties otherwise
    	//use the default HBaseClient config.
    	String confFile = System.getProperty("hbaseclient.log4j.properties");
    	if(confFile == null) {
    		System.setProperty("trafodion.hdfs.log", System.getenv("MY_SQROOT") + "/logs/trafodion.hdfs.log");
    		confFile = System.getenv("MY_SQROOT") + "/conf/log4j.hdfs.config";
    	}
    	PropertyConfigurator.configure(confFile);
    }

    public boolean init(String zkServers, String zkPort) 
	throws MasterNotRunningException, ZooKeeperConnectionException, ServiceException, IOException
    {
         if (logger.isDebugEnabled()) logger.debug("HBaseClient.init(" + zkServers + ", " + zkPort
                         + ") called.");
         HBaseAdmin.checkHBaseAvailable(config);
         return true;
    }
 
    private void  cleanup(PoolMap hTableClientsPool) throws IOException
    {
       Collection hTableClients;
       Iterator<HTableClient> iter;
       HTableClient htable;
       boolean clearRegionCache = false;
       boolean cleanJniObject = true;

       hTableClients = hTableClientsPool.values();
       iter = hTableClients.iterator();
       while (iter.hasNext())
       {
         htable = iter.next();
         htable.close(clearRegionCache, cleanJniObject);          
       }
       hTableClientsPool.clear();
    }

    public boolean cleanup() throws IOException {
       cleanup(hTableClientsInUse);
       cleanup(hTableClientsFree);
       return true;
    }

   public void cleanupCache(Collection hTableClients) throws IOException
    {
       Iterator<HTableClient> iter;
       HTableClient htable;
       boolean clearRegionCache = true;
       boolean cleanJniObject = false;
 
       iter = hTableClients.iterator();
       while (iter.hasNext())
       {
          htable = iter.next();
          htable.close(clearRegionCache, cleanJniObject);     
       }
    }

    public boolean cleanupCache(String tblName) throws IOException
    {
       Collection hTableClients;
       hTableClients = hTableClientsFree.values(tblName);
       cleanupCache(hTableClients);  
       hTableClientsFree.remove(tblName);
       hTableClients = hTableClientsInUse.values(tblName);
       cleanupCache(hTableClients);  
       hTableClientsInUse.remove(tblName);
       return true;
    }

    public boolean create(String tblName, Object[]  colFamNameList) 
        throws IOException, MasterNotRunningException {
            if (logger.isDebugEnabled()) logger.debug("HBaseClient.create(" + tblName + ") called.");
            cleanupCache(tblName);
            HTableDescriptor desc = new HTableDescriptor(tblName);
            for (int i = 0; i < colFamNameList.length ; i++) {
		String  colFam = (String)colFamNameList[i];
                HColumnDescriptor colDesc = new HColumnDescriptor(colFam);
                colDesc.setMaxVersions(1);
                desc.addFamily(colDesc);
            }
            HBaseAdmin admin = new HBaseAdmin(config);
            admin.createTable(desc);
            admin.close();
            return true;
   } 

    public boolean createk(String tblName, Object[] tableOptions, 
        Object[]  beginEndKeys) 
        throws IOException, MasterNotRunningException {
            if (logger.isDebugEnabled()) logger.debug("HBaseClient.createk(" + tblName + ") called.");
            String trueStr = "TRUE";
            cleanupCache(tblName);
            HTableDescriptor desc = new HTableDescriptor(tblName);
            HColumnDescriptor colDesc = new 
		HColumnDescriptor((String)tableOptions[HBASE_NAME]);
            for (int i = 0; i < tableOptions.length; i++) {
                if (i == HBASE_NAME) 
			continue ;
		String tableOption = (String)tableOptions[i];
                if ((i != HBASE_MAX_VERSIONS) && (tableOption.isEmpty()))
			 continue ;
                switch (i) {
                case HBASE_MAX_VERSIONS:
                    if (tableOption.isEmpty())
                        colDesc.setMaxVersions(1);
                    else 
                        colDesc.setMaxVersions
                            (Integer.parseInt(tableOption));
                    break ;
                case HBASE_MIN_VERSIONS:
                    colDesc.setMinVersions
                        (Integer.parseInt(tableOption));
                    break ;
                case HBASE_TTL:
                    colDesc.setTimeToLive
                        (Integer.parseInt(tableOption));
                    break ;
                case HBASE_BLOCKCACHE:
                    if (tableOption.equalsIgnoreCase(trueStr))
                        colDesc.setBlockCacheEnabled(true);
                    else
                        colDesc.setBlockCacheEnabled(false);
                    break ;
                case HBASE_IN_MEMORY:
                    if (tableOption.equalsIgnoreCase(trueStr))
                        colDesc.setInMemory(true);
                    else
                        colDesc.setInMemory(false);
                    break ;
                case HBASE_COMPRESSION:
                    if (tableOption.equalsIgnoreCase("GZ"))
                        colDesc.setCompressionType(Algorithm.GZ);
                    else if (tableOption.equalsIgnoreCase("LZ4"))
                        colDesc.setCompressionType(Algorithm.LZ4);
                    else if (tableOption.equalsIgnoreCase("LZO"))
                        colDesc.setCompressionType(Algorithm.LZO);
                    else if (tableOption.equalsIgnoreCase("NONE"))
                        colDesc.setCompressionType(Algorithm.NONE);
                    else if (tableOption.equalsIgnoreCase("SNAPPY"))
                    colDesc.setCompressionType(Algorithm.SNAPPY); 
                    break ;
                case HBASE_BLOOMFILTER:
                      if (tableOption.equalsIgnoreCase("NONE"))
                        colDesc.setBloomFilterType(BloomType.NONE);
                    else if (tableOption.equalsIgnoreCase("ROW"))
                        colDesc.setBloomFilterType(BloomType.ROW);
                    else if (tableOption.equalsIgnoreCase("ROWCOL"))
                    colDesc.setBloomFilterType(BloomType.ROWCOL); 
                    break ;
                case HBASE_BLOCKSIZE:
                    colDesc.setBlocksize
                        (Integer.parseInt(tableOption));
                    break ;
                case HBASE_DATA_BLOCK_ENCODING:
                    if (tableOption.equalsIgnoreCase("DIFF"))
                        colDesc.setDataBlockEncoding(DataBlockEncoding.DIFF);
                    else if (tableOption.equalsIgnoreCase("FAST_DIFF"))
                        colDesc.setDataBlockEncoding(DataBlockEncoding.FAST_DIFF);
                    else if (tableOption.equalsIgnoreCase("NONE"))
                        colDesc.setDataBlockEncoding(DataBlockEncoding.NONE);
                    else if (tableOption.equalsIgnoreCase("PREFIX"))
                        colDesc.setDataBlockEncoding(DataBlockEncoding.PREFIX);
                  else if (tableOption.equalsIgnoreCase("PREFIX_TREE"))
                    colDesc.setDataBlockEncoding(DataBlockEncoding.PREFIX_TREE);
                    break ;
                case HBASE_CACHE_BLOOMS_ON_WRITE:
                    if (tableOption.equalsIgnoreCase(trueStr))
                        colDesc.setCacheBloomsOnWrite(true);
                    else
                        colDesc.setCacheBloomsOnWrite(false);
                    break ;
                case HBASE_CACHE_DATA_ON_WRITE:
                    if (tableOption.equalsIgnoreCase(trueStr))
                        colDesc.setCacheDataOnWrite(true);
                    else
                        colDesc.setCacheDataOnWrite(false);
                    break ;
                case HBASE_CACHE_INDEXES_ON_WRITE:
                    if (tableOption.equalsIgnoreCase(trueStr))
                        colDesc.setCacheIndexesOnWrite(true);
                    else
                        colDesc.setCacheIndexesOnWrite(false);
                    break ;
                case HBASE_COMPACT_COMPRESSION:
                    if (tableOption.equalsIgnoreCase("GZ"))
                        colDesc.setCompactionCompressionType(Algorithm.GZ);
                    else if (tableOption.equalsIgnoreCase("LZ4"))
                        colDesc.setCompactionCompressionType(Algorithm.LZ4);
                    else if (tableOption.equalsIgnoreCase("LZO"))
                        colDesc.setCompactionCompressionType(Algorithm.LZO);
                    else if (tableOption.equalsIgnoreCase("NONE"))
                        colDesc.setCompactionCompressionType(Algorithm.NONE);
                    else if (tableOption.equalsIgnoreCase("SNAPPY"))
                    colDesc.setCompactionCompressionType(Algorithm.SNAPPY); 
                    break ;
                case HBASE_PREFIX_LENGTH_KEY:
                    desc.setValue(KeyPrefixRegionSplitPolicy.PREFIX_LENGTH_KEY,
                                  tableOption);
                    break ;
                case HBASE_EVICT_BLOCKS_ON_CLOSE:
                    if (tableOption.equalsIgnoreCase(trueStr))
                        colDesc.setEvictBlocksOnClose(true);
                    else
                        colDesc.setEvictBlocksOnClose(false);
                    break ;
                case HBASE_KEEP_DELETED_CELLS:
                    if (tableOption.equalsIgnoreCase(trueStr))
                        colDesc.setKeepDeletedCells(true);
                    else
                        colDesc.setKeepDeletedCells(false);
                    break ;
                case HBASE_REPLICATION_SCOPE:
                    colDesc.setScope
                        (Integer.parseInt(tableOption));
                    break ;
                case HBASE_MAX_FILESIZE:
                    desc.setMaxFileSize
                        (Long.parseLong(tableOption));
                    break ;
                case HBASE_COMPACT:
                   if (tableOption.equalsIgnoreCase(trueStr))
                        desc.setCompactionEnabled(true);
                    else
                    desc.setCompactionEnabled(false); 
                    break ;
                case HBASE_DURABILITY:
                    if (tableOption.equalsIgnoreCase("ASYNC_WAL"))
                        desc.setDurability(Durability.ASYNC_WAL);
                    else if (tableOption.equalsIgnoreCase("FSYNC_WAL"))
                        desc.setDurability(Durability.FSYNC_WAL);
                    else if (tableOption.equalsIgnoreCase("SKIP_WAL"))
                        desc.setDurability(Durability.SKIP_WAL);
                    else if (tableOption.equalsIgnoreCase("SYNC_WAL"))
                        desc.setDurability(Durability.SYNC_WAL);
                    else if (tableOption.equalsIgnoreCase("USE_DEFAULT"))
                    desc.setDurability(Durability.USE_DEFAULT); 
                    break ;
                case HBASE_MEMSTORE_FLUSH_SIZE:
                    desc.setMemStoreFlushSize
                        (Long.parseLong(tableOption));
                    break ;
                case HBASE_SPLIT_POLICY:
                    // This method not yet available in earlier versions
                    // desc.setRegionSplitPolicyClassName(tableOption));
                    desc.setValue(desc.SPLIT_POLICY, tableOption);
                    break ;
                default:
                    break;
                }
            }
            desc.addFamily(colDesc);
            HBaseAdmin admin = new HBaseAdmin(config);
            if (beginEndKeys != null && beginEndKeys.length > 0)
            {
               byte[][] keys = new byte[beginEndKeys.length][];
               for (int i = 0; i < beginEndKeys.length; i++) 
                   keys[i] = (byte[])beginEndKeys[i]; 
               admin.createTable(desc, keys);
            }
            else
               admin.createTable(desc);
            admin.close();
        return true;
    }

    public boolean drop(String tblName) 
             throws MasterNotRunningException, IOException {
            if (logger.isDebugEnabled()) logger.debug("HBaseClient.drop(" + tblName + ") called.");
            HBaseAdmin admin = new HBaseAdmin(config);
            //			admin.disableTableAsync(tblName);
           admin.disableTable(tblName);
           admin.deleteTable(tblName);
           admin.close();
           return cleanupCache(tblName);
    }

    public boolean dropAll(String pattern) 
             throws MasterNotRunningException, IOException {
            if (logger.isDebugEnabled()) logger.debug("HBaseClient.dropAll(" + pattern + ") called.");
            HBaseAdmin admin = new HBaseAdmin(config);

	    HTableDescriptor[] htdl = admin.listTables(pattern);
	    if (htdl == null) // no tables match the given pattern.
		return true;

	    for (HTableDescriptor htd : htdl) {
		String tblName = htd.getNameAsString();

                // do not drop DTM log files which have the format: TRAFODION._DTM_.*
                int idx = tblName.indexOf("TRAFODION._DTM_");
                if (idx == 0)
                    continue;
                
                //                System.out.println(tblName);
                admin.disableTable(tblName);
                admin.deleteTable(tblName);
	    }
 	    
            admin.close();
            return cleanup();
    }

    public boolean copy(String currTblName, String oldTblName)
	throws MasterNotRunningException, IOException, SnapshotCreationException, InterruptedException {
            if (logger.isDebugEnabled()) logger.debug("HBaseClient.copy(" + currTblName + oldTblName + ") called.");
            HBaseAdmin admin = new HBaseAdmin(config);
	    
	    String snapshotName = currTblName + "_SNAPSHOT";
	    
	    List<SnapshotDescription> l = new ArrayList<SnapshotDescription>(); 
	    //	    l = admin.listSnapshots(snapshotName);
	    l = admin.listSnapshots();
	    if (! l.isEmpty())
		{
		    for (SnapshotDescription sd : l) {
			//			System.out.println("here 1");
			//			System.out.println(snapshotName);
			//			System.out.println(sd.getName());
			if (sd.getName().compareTo(snapshotName) == 0)
			    {
				//				System.out.println("here 2");
				//			    admin.enableTable(snapshotName);
				//				System.out.println("here 3");
				admin.deleteSnapshot(snapshotName);
				//				System.out.println("here 4");
			    }
		    }
		}
	    //	    System.out.println(snapshotName);
	    if (! admin.isTableDisabled(currTblName))
		admin.disableTable(currTblName);
	    //	    System.out.println("here 5");
	    admin.snapshot(snapshotName, currTblName);
	    admin.cloneSnapshot(snapshotName, oldTblName);
	    admin.deleteSnapshot(snapshotName);
	    //	    System.out.println("here 6");
	    admin.enableTable(currTblName);
            admin.close();
            return true;
    }

    public boolean exists(String tblName)  
           throws MasterNotRunningException, IOException {
            if (logger.isDebugEnabled()) logger.debug("HBaseClient.exists(" + tblName + ") called.");
            HBaseAdmin admin = new HBaseAdmin(config);
            boolean result = admin.tableExists(tblName);
            admin.close();
            return result;
    }

    public HTableClient getHTableClient(String tblName, 
                  boolean useTRex) throws IOException 
    {
       if (logger.isDebugEnabled()) logger.debug("HBaseClient.getHTableClient(" + tblName
                         + (useTRex ? ", use TRX" : ", no TRX") + ") called.");
       HTableClient htable = hTableClientsFree.get(tblName);
       if (htable == null) {
          htable = new HTableClient();
          if (htable.init(tblName, useTRex) == false) {
             if (logger.isDebugEnabled()) logger.debug("  ==> Error in init(), returning empty.");
             return null;
          }
          if (logger.isDebugEnabled()) logger.debug("  ==> Created new object.");
          hTableClientsInUse.put(htable.getTableName(), htable);
          return htable;
       } else {
            if (logger.isDebugEnabled()) logger.debug("  ==> Returning existing object, removing from container.");
            hTableClientsInUse.put(htable.getTableName(), htable);
            htable.resetAutoFlush();
            return htable;
       }
    }


    public void releaseHTableClient(HTableClient htable) 
                    throws IOException {
        if (htable == null)
            return;
	                
        if (logger.isDebugEnabled()) logger.debug("HBaseClient.releaseHTableClient(" + htable.getTableName() + ").");
        boolean cleanJniObject = false;
        if (htable.release(cleanJniObject))
        // If the thread is interrupted, then remove the table from cache
        // because the table connection is retried when the table is used
        // next time

           cleanupCache(htable.getTableName());
        else
        {
           if (hTableClientsInUse.remove(htable.getTableName(), htable))
              hTableClientsFree.put(htable.getTableName(), htable);
           else
              if (logger.isDebugEnabled()) logger.debug("Table not found in inUse Pool");
        }
    }

    public boolean flushAllTables() throws IOException {
        if (logger.isDebugEnabled()) logger.debug("HBaseClient.flushAllTables() called.");
       if (hTableClientsInUse.isEmpty()) {
          return true;
        }
        for (HTableClient htable : hTableClientsInUse.values()) {
		  htable.flush();
        }
	return true; 
    }

    public boolean grant(byte[] user, byte[] tblName,
                         Object[] actionCodes) throws IOException {
        if (logger.isDebugEnabled()) logger.debug("HBaseClient.grant(" + new String(user) + ", "
                     + new String(tblName) + ") called.");
		byte[] colFamily = null;

		Permission.Action[] assigned = new Permission.Action[actionCodes.length];
		for (int i = 0 ; i < actionCodes.length; i++) {
			String actionCode = (String)actionCodes[i];
			assigned[i] = Permission.Action.valueOf(actionCode);
		}

	    //HB98
	    TableName htblName = TableName.valueOf(new String(NamespaceDescriptor.DEFAULT_NAMESPACE_NAME)
						   ,new String(tblName));
            UserPermission userPerm = new UserPermission(user, htblName,
                                                         colFamily, assigned);

            AccessController accessController = new AccessController();
	    //HB98 The grant() method is very different in HB98 (commenting out for now)
            //accessController.grant(userPerm);
        return true;
    }

   public boolean revoke(byte[] user, byte[] tblName,
                          Object[] actionCodes) 
                     throws IOException {
        if (logger.isDebugEnabled()) logger.debug("HBaseClient.revoke(" + new String(user) + ", "
                     + new String(tblName) + ") called.");
        byte[] colFamily = null;

        Permission.Action[] assigned = new Permission.Action[actionCodes.length];
        for (int i = 0 ; i < actionCodes.length; i++) {
            String actionCode = (String)actionCodes[i];
            assigned[i] = Permission.Action.valueOf(actionCode);
        }

	    //HB98
	    TableName htblName = TableName.valueOf(new String(NamespaceDescriptor.DEFAULT_NAMESPACE_NAME)
						   ,new String(tblName));
            UserPermission userPerm = new UserPermission(user, htblName,
                                                         colFamily, assigned);

            AccessController accessController = new AccessController();
	    
	    //HB98 The revoke() method is very different in HB98 (commenting out for now)
            //accessController.revoke(userPerm);
        return true;
    }

    // Debugging method to display initial set of KeyValues and sequence
    // of column qualifiers.
    private void printQualifiers(HFile.Reader reader, int maxKeys) 
                 throws IOException {
      String qualifiers = new String();
      HFileScanner scanner = reader.getScanner(false, false, false);
      scanner.seekTo();
      int kvCount = 0;
      int nonPuts = 0;
      do {
        KeyValue kv = scanner.getKeyValue();
        System.out.println(kv.toString());
        if (kv.getType() == KeyValue.Type.Put.getCode())
          qualifiers = qualifiers + kv.getQualifier()[0] + " ";
        else
          nonPuts++;
      } while (++kvCount < maxKeys && scanner.next());
      System.out.println("First " + kvCount + " column qualifiers: " + qualifiers);
      if (nonPuts > 0)
        System.out.println("Encountered " + nonPuts + " non-PUT KeyValue types.");
    }

    // Estimates the number of rows still in the MemStores of the regions
    // associated with the passed table name. The number of bytes in the
    // MemStores is divided by the passed row size in bytes, which is
    // derived by comparing the row count for an HFile (which in turn is
    // derived by the number of KeyValues in the file and the number of
    // columns in the table) to the size of the HFile.
    private long estimateMemStoreRows(String tblName, int rowSize)
                 throws MasterNotRunningException, IOException {
      if (rowSize == 0)
        return 0;

      HBaseAdmin admin = new HBaseAdmin(config);
      HTable htbl = new HTable(config, tblName);
      long totalMemStoreBytes = 0;
      try {
        // Get a set of all the regions for the table.
        Set<HRegionInfo> tableRegionInfos = htbl.getRegionLocations().keySet();
        Set tableRegions = new TreeSet(Bytes.BYTES_COMPARATOR);
        for (HRegionInfo regionInfo : tableRegionInfos) {
          tableRegions.add(regionInfo.getRegionName());
        }
     
        // Get collection of all servers in the cluster.
        ClusterStatus clusterStatus = admin.getClusterStatus();
        Collection<ServerName> servers = clusterStatus.getServers();
        final long bytesPerMeg = 1024L * 1024L;
     
        // For each server, look at each region it contains and see if 
        // it is in the set of regions for the table. If so, add the
        // size of its the running total.
        for (ServerName serverName : servers) {
          ServerLoad serverLoad = clusterStatus.getLoad(serverName);
          for (RegionLoad regionLoad: serverLoad.getRegionsLoad().values()) {
            byte[] regionId = regionLoad.getName();
            if (tableRegions.contains(regionId)) {
              long regionMemStoreBytes = bytesPerMeg * regionLoad.getMemStoreSizeMB();
              if (logger.isDebugEnabled()) logger.debug("Region " + regionLoad.getNameAsString()
                           + " has MemStore size " + regionMemStoreBytes);
              totalMemStoreBytes += regionMemStoreBytes;
            }
          }
        }
      }
      finally {
        admin.close();
      }

      // Divide the total MemStore size by the size of a single row.
      if (logger.isDebugEnabled()) logger.debug("Estimating " + (totalMemStoreBytes / rowSize)
                   + " rows in MemStores of table's regions.");
      return totalMemStoreBytes / rowSize;
    }


    public float getBlockCacheFraction()
    {
        float defCacheFraction = 0.4f;
        return config.getFloat("hfile.block.cache.size",defCacheFraction);
    }
    // Estimates row count for tblName by iterating over the HFiles for
    // the table, extracting the KeyValue entry count from the file's
    // trailer block, summing the counts, and dividing by the number of
    // columns in the table. An adjustment is made for the estimated
    // number of missing (null) values by sampling the first several
    // hundred KeyValues to see how many are missing.
    public boolean estimateRowCount(String tblName, int partialRowSize,
                                    int numCols, long[] rc)
                   throws MasterNotRunningException, IOException, ClassNotFoundException, URISyntaxException {
      if (logger.isDebugEnabled()) logger.debug("HBaseClient.estimateRowCount(" + tblName + ") called.");

      final String REGION_NAME_PATTERN = "[0-9a-f]*";
      final String HFILE_NAME_PATTERN  = "[0-9a-f]*";

      // To estimate incidence of nulls, read the first 500 rows worth
      // of KeyValues.
      final int ROWS_TO_SAMPLE = 500;
      int putKVsSampled = 0;
      int nonPutKVsSampled = 0;
      int nullCount = 0;
      long totalEntries = 0;   // KeyValues in all HFiles for table
      long totalSizeBytes = 0; // Size of all HFiles for table 
      long estimatedTotalPuts = 0;
      boolean more = true;

      // Access the file system to go directly to the table's HFiles.
      // Create a reader for the file to access the entry count stored
      // in the trailer block, and a scanner to iterate over a few
      // hundred KeyValues to estimate the incidence of nulls.
      long nano1, nano2;
      nano1 = System.nanoTime();
      FileSystem fileSystem = FileSystem.get(config);
      nano2 = System.nanoTime();
      if (logger.isDebugEnabled()) logger.debug("FileSystem.get() took " + ((nano2 - nano1) + 500000) / 1000000 + " milliseconds.");
      CacheConfig cacheConf = new CacheConfig(config);
      String hbaseRootPath = config.get(HConstants.HBASE_DIR).trim();
      if (hbaseRootPath.charAt(0) != '/')
        hbaseRootPath = new URI(hbaseRootPath).getPath();
      if (logger.isDebugEnabled()) logger.debug("hbaseRootPath = " + hbaseRootPath);
      FileStatus[] fsArr = fileSystem.globStatus(new Path(
                               hbaseRootPath + "/data/default/" +
                               tblName + "/" + REGION_NAME_PATTERN +
                               "/#1/" + HFILE_NAME_PATTERN));
      for (FileStatus fs : fsArr) {
        // Make sure the file name conforms to HFile name pattern.
        if (!StoreFileInfo.isHFile(fs.getPath())) {
          if (logger.isDebugEnabled()) logger.debug("Skipped file " + fs.getPath() + " -- not a valid HFile name.");
          continue;
        }
        HFile.Reader reader = HFile.createReader(fileSystem, fs.getPath(), cacheConf, config);
        try {
          totalEntries += reader.getEntries();
          totalSizeBytes += reader.length();
          //printQualifiers(reader, 100);
          if (ROWS_TO_SAMPLE > 0 &&
              totalEntries == reader.getEntries()) {  // first file only
            // Trafodion column qualifiers are ordinal numbers, which
            // makes it easy to count missing (null) values. We also count
            // the non-Put KVs (typically delete-row markers) to estimate
            // their frequency in the full file set.
            HFileScanner scanner = reader.getScanner(false, false, false);
            scanner.seekTo();  //position at beginning of first data block
            byte currQual = 0;
            byte nextQual;
            do {
              KeyValue kv = scanner.getKeyValue();
              if (kv.getType() == KeyValue.Type.Put.getCode()) {
                nextQual = kv.getQualifier()[0];
                if (nextQual <= currQual)
                  nullCount += ((numCols - currQual)  // nulls at end of this row
                              + (nextQual - 1));      // nulls at start of next row
                else
                  nullCount += (nextQual - currQual - 1);
                currQual = nextQual;
                putKVsSampled++;
              } else {
                nonPutKVsSampled++;  // don't count these toward the number
              }                      //   we want to scan
            } while ((putKVsSampled + nullCount) < (numCols * ROWS_TO_SAMPLE)
                     && (more = scanner.next()));

            // If all rows were read, count any nulls at end of last row.
            if (!more && putKVsSampled > 0)
              nullCount += (numCols - currQual);

            if (logger.isDebugEnabled()) logger.debug("Sampled " + nullCount + " nulls.");
          }  // code for first file
        } finally {
          reader.close(false);
        }
      } // for

      long estimatedEntries = (ROWS_TO_SAMPLE > 0
                                 ? 0               // get from sample data, below
                                 : totalEntries);  // no sampling, use stored value
      if (putKVsSampled > 0) // avoid div by 0 if no Put KVs in sample
        {
          estimatedTotalPuts = (putKVsSampled * totalEntries) / 
                               (putKVsSampled + nonPutKVsSampled);
          estimatedEntries = ((putKVsSampled + nullCount) * estimatedTotalPuts)
                                   / putKVsSampled;
        }

      // Calculate estimate of rows in all HFiles of table.
      rc[0] = (estimatedEntries + (numCols/2)) / numCols; // round instead of truncate

      // Estimate # of rows in MemStores of all regions of table. Pass
      // a value to divide the size of the MemStore by. Base this on the
      // ratio of bytes-to-rows in the HFiles, or the actual row size if
      // the HFiles were empty.
      int rowSize;
      if (rc[0] > 0)
        rowSize = (int)(totalSizeBytes / rc[0]);
      else {
        // From Traf metadata we have calculated and passed in part of the row
        // size, including size of column qualifiers (col names), which are not
        // known to HBase.  Add to this the length of the fixed part of the
        // KeyValue format, times the number of columns.
        int fixedSizePartOfKV = KeyValue.KEYVALUE_INFRASTRUCTURE_SIZE // key len + value len
                              + KeyValue.KEY_INFRASTRUCTURE_SIZE;     // rowkey & col family len, timestamp, key type
        rowSize = partialRowSize   // for all cols: row key + col qualifiers + values
                      + (fixedSizePartOfKV * numCols);

        // Trafodion tables have a single col family at present, so we only look
        // at the first family name, and multiply its length times the number of
        // columns. Even if more than one family is used in the future, presumably
        // they will all be the same short size.
        HTable htbl = new HTable(config, tblName);
        HTableDescriptor htblDesc = htbl.getTableDescriptor();
        HColumnDescriptor[] families = htblDesc.getColumnFamilies();
        rowSize += (families[0].getName().length * numCols);
      }

      // Get the estimate of MemStore rows. Add to total after logging
      // of individual sums below.
      long memStoreRows = estimateMemStoreRows(tblName, rowSize);

      if (logger.isDebugEnabled()) logger.debug(tblName + " contains a total of " + totalEntries + " KeyValues in all HFiles.");
      if (logger.isDebugEnabled()) logger.debug("Based on a sample, it is estimated that " + estimatedTotalPuts +
                   " of these KeyValues are of type Put.");
      if (putKVsSampled + nullCount > 0)
        if (logger.isDebugEnabled()) logger.debug("Sampling indicates a null incidence of " + 
                     (nullCount * 100)/(putKVsSampled + nullCount) +
                     " percent.");
      if (logger.isDebugEnabled()) logger.debug("Estimated number of actual values (including nulls) is " + estimatedEntries);
      if (logger.isDebugEnabled()) logger.debug("Estimated row count in HFiles = " + estimatedEntries +
                   " / " + numCols + " (# columns) = " + rc[0]);
      if (logger.isDebugEnabled()) logger.debug("Estimated row count from MemStores = " + memStoreRows);

      rc[0] += memStoreRows;  // Add memstore estimate to total
      if (logger.isDebugEnabled()) logger.debug("Total estimated row count for " + tblName + " = " + rc[0]);
      return true;
    }

    void printCell(KeyValue kv) {
        String rowID = new String(kv.getRow());
        String colFamily = new String(kv.getFamily());
        String colName = new String(kv.getQualifier());
        String colValue = new String(kv.getValue());
        String row = rowID + ", " + colFamily + ", " + colName + ", "
            + colValue + ", " + kv.getTimestamp();
        System.out.println(row);
    }

    
  public  HBulkLoadClient getHBulkLoadClient() throws IOException 
  {
    if (logger.isDebugEnabled()) logger.debug("HBaseClient.getHBulkLoadClient() called.");
    HBulkLoadClient hblc = null;
    try 
    {
       hblc = new HBulkLoadClient( config);
    
    if (hblc == null)
      throw new IOException ("hbkc is null");
    }
    catch (IOException e)
    {
      return null;
    }
    
    return hblc;
    
  }
  public void releaseHBulkLoadClient(HBulkLoadClient hblc) 
      throws IOException 
  {
     if (hblc == null)
       return;
          
      if (logger.isDebugEnabled()) logger.debug("HBaseClient.releaseHBulkLoadClient().");
      hblc.release();
   }
}

    


