// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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

import java.io.IOException;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.ArrayList;


import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.KeyValue;
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

//import org.apache.hadoop.hbase.io.compress.Compression.Algorithm;
import org.apache.hadoop.hbase.io.hfile.Compression.Algorithm;
import org.apache.hadoop.hbase.io.encoding.DataBlockEncoding;
//import org.apache.hadoop.hbase.regionserver.BloomType; in v0.97
import org.apache.hadoop.hbase.regionserver.StoreFile.BloomType ;
import org.apache.hadoop.hbase.regionserver.KeyPrefixRegionSplitPolicy;
//import org.apache.hadoop.hbase.client.Durability;
import org.trafodion.sql.HBaseAccess.StringArrayList;
import org.trafodion.sql.HBaseAccess.HTableClient;

public class HBaseClient {

    static Logger logger = Logger.getLogger(HBaseClient.class.getName());
    Configuration config;
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

    void setupLog4j() {
        String confFile = System.getenv("MY_SQROOT")
            + "/logs/log4j.hdfs.config";
        PropertyConfigurator.configure(confFile);
    }

    public boolean init(String zkServers, String zkPort) 
              throws MasterNotRunningException, ZooKeeperConnectionException {
         setupLog4j();
         logger.debug("HBaseClient.init(" + zkServers + ", " + zkPort
                         + ") called.");
         config = HBaseConfiguration.create();
	 if (zkServers.length() > 0)
            config.set("hbase.zookeeper.quorum", zkServers);
	 if (zkPort.length() > 0)
            config.set("hbase.zookeeper.property.clientPort", zkPort);
         HBaseAdmin.checkHBaseAvailable(config);
         return true;
    }
 
    private void  cleanup(PoolMap hTableClientsPool) throws IOException
    {
       Collection hTableClients;
       Iterator<HTableClient> iter;
       HTableClient htable;
       boolean clearRegionCache = false;

       hTableClients = hTableClientsPool.values();
       iter = hTableClients.iterator();
       while (iter.hasNext())
       {
         htable = iter.next();
         htable.close(clearRegionCache);          
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
 
       iter = hTableClients.iterator();
       while (iter.hasNext())
       {
          htable = iter.next();
          htable.close(clearRegionCache);     
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

    public boolean create(String tblName, StringArrayList colFamNameList) 
        throws IOException, MasterNotRunningException {
            logger.debug("HBaseClient.create(" + tblName + ") called.");
            cleanupCache(tblName);
            HTableDescriptor desc = new HTableDescriptor(tblName);
            for (String colFam : colFamNameList) {
                HColumnDescriptor colDesc = new HColumnDescriptor(colFam);
                colDesc.setMaxVersions(1);
                desc.addFamily(colDesc);
            }
            HBaseAdmin admin = new HBaseAdmin(config);
            admin.createTable(desc);
            admin.close();
            return true;
   } 

    public boolean createk(String tblName, StringArrayList tableOptions, 
        ByteArrayList beginEndKeys) 
        throws IOException, MasterNotRunningException {
            logger.debug("HBaseClient.create(" + tblName + ") called.");
            String trueStr = "TRUE";
            cleanupCache(tblName);
            HTableDescriptor desc = new HTableDescriptor(tblName);
            HColumnDescriptor colDesc = new HColumnDescriptor(tableOptions.get(HBASE_NAME));
            for (int i = 0; i < tableOptions.size(); i++) {
                if (i == HBASE_NAME) continue ;
                if ((i != HBASE_MAX_VERSIONS) && (tableOptions.get(i).isEmpty())) continue ;
                switch (i) {
                case HBASE_MAX_VERSIONS:
                    if (tableOptions.get(i).isEmpty())
                        colDesc.setMaxVersions(1);
                    else 
                        colDesc.setMaxVersions
                            (Integer.parseInt(tableOptions.get(i)));
                    break ;
                case HBASE_MIN_VERSIONS:
                    colDesc.setMinVersions
                        (Integer.parseInt(tableOptions.get(i)));
                    break ;
                case HBASE_TTL:
                    colDesc.setTimeToLive
                        (Integer.parseInt(tableOptions.get(i)));
                    break ;
                case HBASE_BLOCKCACHE:
                    if (tableOptions.get(i).equalsIgnoreCase(trueStr))
                        colDesc.setBlockCacheEnabled(true);
                    else
                        colDesc.setBlockCacheEnabled(false);
                    break ;
                case HBASE_IN_MEMORY:
                    if (tableOptions.get(i).equalsIgnoreCase(trueStr))
                        colDesc.setInMemory(true);
                    else
                        colDesc.setInMemory(false);
                    break ;
                case HBASE_COMPRESSION:
                    if (tableOptions.get(i).equalsIgnoreCase("GZ"))
                        colDesc.setCompressionType(Algorithm.GZ);
                    else if (tableOptions.get(i).equalsIgnoreCase("LZ4"))
                        colDesc.setCompressionType(Algorithm.LZ4);
                    else if (tableOptions.get(i).equalsIgnoreCase("LZO"))
                        colDesc.setCompressionType(Algorithm.LZO);
                    else if (tableOptions.get(i).equalsIgnoreCase("NONE"))
                        colDesc.setCompressionType(Algorithm.NONE);
                    else if (tableOptions.get(i).equalsIgnoreCase("SNAPPY"))
                    colDesc.setCompressionType(Algorithm.SNAPPY); 
                    break ;
                case HBASE_BLOOMFILTER:
                      if (tableOptions.get(i).equalsIgnoreCase("NONE"))
                        colDesc.setBloomFilterType(BloomType.NONE);
                    else if (tableOptions.get(i).equalsIgnoreCase("ROW"))
                        colDesc.setBloomFilterType(BloomType.ROW);
                    else if (tableOptions.get(i).equalsIgnoreCase("ROWCOL"))
                    colDesc.setBloomFilterType(BloomType.ROWCOL); 
                    break ;
                case HBASE_BLOCKSIZE:
                    colDesc.setBlocksize
                        (Integer.parseInt(tableOptions.get(i)));
                    break ;
                case HBASE_DATA_BLOCK_ENCODING:
                    if (tableOptions.get(i).equalsIgnoreCase("DIFF"))
                        colDesc.setDataBlockEncoding(DataBlockEncoding.DIFF);
                    else if (tableOptions.get(i).equalsIgnoreCase("FAST_DIFF"))
                        colDesc.setDataBlockEncoding(DataBlockEncoding.FAST_DIFF);
                    else if (tableOptions.get(i).equalsIgnoreCase("NONE"))
                        colDesc.setDataBlockEncoding(DataBlockEncoding.NONE);
                    else if (tableOptions.get(i).equalsIgnoreCase("PREFIX"))
                        colDesc.setDataBlockEncoding(DataBlockEncoding.PREFIX);
             /*     else if (tableOptions.get(i).equalsIgnoreCase("PREFIX_TREE"))
                    colDesc.setDataBlockEncoding(DataBlockEncoding.PREFIX_TREE); */
                    break ;
                case HBASE_CACHE_BLOOMS_ON_WRITE:
                    if (tableOptions.get(i).equalsIgnoreCase(trueStr))
                        colDesc.setCacheBloomsOnWrite(true);
                    else
                        colDesc.setCacheBloomsOnWrite(false);
                    break ;
                case HBASE_CACHE_DATA_ON_WRITE:
                    if (tableOptions.get(i).equalsIgnoreCase(trueStr))
                        colDesc.setCacheDataOnWrite(true);
                    else
                        colDesc.setCacheDataOnWrite(false);
                    break ;
                case HBASE_CACHE_INDEXES_ON_WRITE:
                    if (tableOptions.get(i).equalsIgnoreCase(trueStr))
                        colDesc.setCacheIndexesOnWrite(true);
                    else
                        colDesc.setCacheIndexesOnWrite(false);
                    break ;
                case HBASE_COMPACT_COMPRESSION:
                    if (tableOptions.get(i).equalsIgnoreCase("GZ"))
                        colDesc.setCompactionCompressionType(Algorithm.GZ);
                    else if (tableOptions.get(i).equalsIgnoreCase("LZ4"))
                        colDesc.setCompactionCompressionType(Algorithm.LZ4);
                    else if (tableOptions.get(i).equalsIgnoreCase("LZO"))
                        colDesc.setCompactionCompressionType(Algorithm.LZO);
                    else if (tableOptions.get(i).equalsIgnoreCase("NONE"))
                        colDesc.setCompactionCompressionType(Algorithm.NONE);
                    else if (tableOptions.get(i).equalsIgnoreCase("SNAPPY"))
                    colDesc.setCompactionCompressionType(Algorithm.SNAPPY); 
                    break ;
                case HBASE_PREFIX_LENGTH_KEY:
                    desc.setValue(KeyPrefixRegionSplitPolicy.PREFIX_LENGTH_KEY,
                                  tableOptions.get(i));
                    break ;
                case HBASE_EVICT_BLOCKS_ON_CLOSE:
                    if (tableOptions.get(i).equalsIgnoreCase(trueStr))
                        colDesc.setEvictBlocksOnClose(true);
                    else
                        colDesc.setEvictBlocksOnClose(false);
                    break ;
                case HBASE_KEEP_DELETED_CELLS:
                    if (tableOptions.get(i).equalsIgnoreCase(trueStr))
                        colDesc.setKeepDeletedCells(true);
                    else
                        colDesc.setKeepDeletedCells(false);
                    break ;
                case HBASE_REPLICATION_SCOPE:
                    colDesc.setScope
                        (Integer.parseInt(tableOptions.get(i)));
                    break ;
                case HBASE_MAX_FILESIZE:
                    desc.setMaxFileSize
                        (Long.parseLong(tableOptions.get(i)));
                    break ;
                case HBASE_COMPACT:
                    // Available in HBase 0.97
                    /*  if (tableOptions.get(i).equalsIgnoreCase(trueStr))
                        desc.setCompactionEnabled(true);
                    else
                    desc.setCompactionEnabled(false); */
                    break ;
                case HBASE_DURABILITY:
                    // Available in HBase 0.97
                    /*     if (tableOptions.get(i).equalsIgnoreCase("ASYNC_WAL"))
                        desc.setDurability(Durability.ASYNC_WAL);
                    else if (tableOptions.get(i).equalsIgnoreCase("FSYNC_WAL"))
                        desc.setDurability(Durability.FSYNC_WAL);
                    else if (tableOptions.get(i).equalsIgnoreCase("SKIP_WAL"))
                        desc.setDurability(Durability.SKIP_WAL);
                    else if (tableOptions.get(i).equalsIgnoreCase("SYNC_WAL"))
                        desc.setDurability(Durability.SYNC_WAL);
                    else if (tableOptions.get(i).equalsIgnoreCase("USE_DEFAULT"))
                    desc.setDurability(Durability.USE_DEFAULT); */
                    break ;
                case HBASE_MEMSTORE_FLUSH_SIZE:
                    desc.setMemStoreFlushSize
                        (Long.parseLong(tableOptions.get(i)));
                    break ;
                case HBASE_SPLIT_POLICY:
                    // This method not yet available in earlier versions
                    // desc.setRegionSplitPolicyClassName(tableOptions.get(i));
                    desc.setValue(desc.SPLIT_POLICY, tableOptions.get(i));
                    break ;
                default:
                    break;
                }
            }
            desc.addFamily(colDesc);
            HBaseAdmin admin = new HBaseAdmin(config);
            byte[][] keys = new byte[beginEndKeys.size()][];
            for (int i = 0; i < beginEndKeys.size(); i++) 
            {
                keys[i] = beginEndKeys.get(i); 
            }

            if (beginEndKeys.size() == 0)
                admin.createTable(desc);
            else
                admin.createTable(desc, keys);

            admin.close();
        return true;
    }

    public boolean drop(String tblName) 
             throws MasterNotRunningException, IOException {
            logger.debug("HBaseClient.drop(" + tblName + ") called.");
            HBaseAdmin admin = new HBaseAdmin(config);
            //			admin.disableTableAsync(tblName);
           admin.disableTable(tblName);
           admin.deleteTable(tblName);
           admin.close();
           return cleanupCache(tblName);
    }

    public boolean dropAll(String pattern) 
             throws MasterNotRunningException, IOException {
            logger.debug("HBaseClient.dropAll(" + pattern + ") called.");
            HBaseAdmin admin = new HBaseAdmin(config);

	    //	    System.out.println(pattern);

	    HTableDescriptor[] htdl = admin.listTables(pattern);
	    if (htdl == null) // no tables match the given pattern.
		return true;

	    for (HTableDescriptor htd : htdl) {
		String tblName = htd.getNameAsString();

		//		System.out.println(tblName);
		admin.disableTable(tblName);
		admin.deleteTable(tblName);
	    }
 	    
	    /*
            HTableDescriptor[] htd = admin.disableTables(pattern);
	    if (htd != null) {
		System.out.println("here");
		System.out.println(pattern);


		return false;
	    }

            htd = admin.deleteTables(pattern);
	    if (htd != null) {
		return false;
	    }
	    */

            admin.close();
            return cleanup();
    }

    public boolean copy(String currTblName, String oldTblName)
	throws MasterNotRunningException, IOException, SnapshotCreationException, InterruptedException {
            logger.debug("HBaseClient.copy(" + currTblName + oldTblName + ") called.");
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
            logger.debug("HBaseClient.exists(" + tblName + ") called.");
            HBaseAdmin admin = new HBaseAdmin(config);
            boolean result = admin.tableExists(tblName);
            admin.close();
            return result;
    }

    public HTableClient getHTableClient(String tblName, 
                  boolean useTRex) throws IOException 
    {
       logger.debug("HBaseClient.getHTableClient(" + tblName
                         + (useTRex ? ", use TRX" : ", no TRX") + ") called.");
       HTableClient htable = hTableClientsFree.get(tblName);
       if (htable == null) {
          htable = new HTableClient();
          if (htable.init(tblName, config, useTRex) == false) {
             logger.debug("  ==> Error in init(), returning empty.");
             return null;
          }
          logger.debug("  ==> Created new object.");
          hTableClientsInUse.put(htable.getTableName(), htable);
          return htable;
       } else {
            logger.debug("  ==> Returning existing object, removing from container.");
            hTableClientsInUse.put(htable.getTableName(), htable);
            htable.resetAutoFlush();
            return htable;
       }
    }


    public void releaseHTableClient(HTableClient htable) 

                    throws IOException {
        if (htable == null)
            return;
	                
        logger.debug("HBaseClient.releaseHTableClient(" + htable.getTableName() + ").");
        htable.flush();
        if (hTableClientsInUse.remove(htable.getTableName(), htable))
            hTableClientsFree.put(htable.getTableName(), htable);

        else
            logger.debug("Table not found in inUse Pool");

    }

    public boolean flushAllTables() throws IOException {
        logger.debug("HBaseClient.flushAllTables() called.");
       if (hTableClientsInUse.isEmpty()) {
          return true;
        }
        for (HTableClient htable : hTableClientsInUse.values()) {
		  htable.flush();
        }
	    return true; 
	}

    public boolean grant(byte[] user, byte[] tblName,
                         StringArrayList actionCodes) throws IOException {
        logger.debug("HBaseClient.grant(" + new String(user) + ", "
                     + new String(tblName) + ") called.");
            byte[] colFamily = null;

            int len = 0;
            for (String actionCode : actionCodes) {
                len++;
            }

            Permission.Action[] assigned = new Permission.Action[len];
            int i = 0;
            for (String actionCode : actionCodes) {
                assigned[i] = Permission.Action.valueOf(actionCode);
                i++;
            }

            UserPermission userPerm = new UserPermission(user, tblName,
                                                         colFamily, assigned);

            AccessController accessController = new AccessController();
            accessController.grant(userPerm);
        return true;
    }

    public boolean revoke(byte[] user, byte[] tblName,
                          StringArrayList actionCodes) 
                     throws IOException {
        logger.debug("HBaseClient.revoke(" + new String(user) + ", "
                     + new String(tblName) + ") called.");
            byte[] colFamily = null;

            int len = 0;
            for (String actionCode : actionCodes) {
                len++;
            }

            Permission.Action[] assigned = new Permission.Action[len];
            int i = 0;
            for (String actionCode : actionCodes) {
                assigned[i] = Permission.Action.valueOf(actionCode);
                i++;
            }

            UserPermission userPerm = new UserPermission(user, tblName,
                                                         colFamily, assigned);

            AccessController accessController = new AccessController();
            accessController.revoke(userPerm);
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
    logger.debug("HBaseClient.getHBulkLoadClient() called.");
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
  
}

    


