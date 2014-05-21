// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2014 Hewlett-Packard Development Company, L.P.
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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;

import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.hbase.HBaseConfiguration;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.security.access.AccessController;
import org.apache.hadoop.hbase.security.access.UserPermission;
import org.apache.hadoop.hbase.security.access.Permission;
import org.apache.hadoop.hbase.MasterNotRunningException;
import org.apache.hadoop.hbase.ZooKeeperConnectionException;
import org.apache.hadoop.hbase.io.hfile.CacheConfig;
import org.apache.hadoop.hbase.io.hfile.Compression;
import org.apache.hadoop.hbase.io.hfile.HFile;
//import org.apache.hadoop.hbase.io.compress.Compression.Algorithm;
import org.apache.hadoop.hbase.io.hfile.Compression.Algorithm;
import org.apache.hadoop.hbase.io.encoding.DataBlockEncoding;
import org.apache.hadoop.hbase.mapreduce.LoadIncrementalHFiles;
//import org.apache.hadoop.hbase.regionserver.BloomType; in v0.97
import org.apache.hadoop.hbase.regionserver.StoreFile.BloomType ;
//import org.apache.hadoop.hbase.client.Durability;

import org.apache.hadoop.fs.Path;
import org.trafodion.sql.HBaseAccess.StringArrayList;
import org.trafodion.sql.HBaseAccess.HTableClient;
//import org.trafodion.sql.HBaseAccess.HBaseClient;
import org.trafodion.sql.HBaseAccess.HTableClient.QualifiedColumn;

import java.nio.ByteBuffer;

public class HBulkLoadClient
{
  public static int BLOCKSIZE = 64*1024;
  public static String COMPRESSION = Compression.Algorithm.NONE.getName();
  String lastError;
  static Logger logger = Logger.getLogger(HBulkLoadClient.class.getName());
  Configuration config;
  HFile.Writer writer;

  HashMap< Integer, HTableClient.QualifiedColumn> qualifierMap = null;

  public HBulkLoadClient()
  {
    logger.debug("HBulkLoadClient.HBulkLoadClient() called.");

  }

  public HBulkLoadClient(Configuration conf) throws IOException
  {
    logger.debug("HBulkLoadClient.HBulkLoadClient(...) called.");
    if (conf == null)
    {
      throw new IOException("config not set");
    }
    config = conf;
  }

  public String getLastError() {
    return lastError;
  }

  void setLastError(String err) {
      lastError = err;
  }
  public boolean createHFile(String hFileLoc, String hfileName) throws IOException
  {
    //config initialized during init
    logger.debug("HBulkLoadClient.createHFile() called.");
    FileSystem fs = FileSystem.get(config);
    Path hfilePath = new Path(new Path(hFileLoc ), hfileName);

    logger.debug("HBulkLoadClient.createHFile Path-- ");

    try
    {
    writer =    HFile.getWriterFactory(config, new CacheConfig(config))
                     .withPath(fs, hfilePath)
                     .withBlockSize(BLOCKSIZE)
                     .withCompression(COMPRESSION)
                     .withComparator(KeyValue.KEY_COMPARATOR)
                     .create();
    }
    catch (IOException e)
    {
        logger.debug("HBulkLoadClient.createHFile EXecption" + e.getMessage());
        throw e;
    }
    return true;
  }
  public boolean addToHFile( RowsToInsert rows) throws IOException
  {
    logger.debug("Enter addToHFile() ");

    if (qualifierMap == null)
    {
      qualifierMap = new HashMap<Integer, HTableClient.QualifiedColumn>();

      RowsToInsert.RowInfo firstrow = rows.firstElement();
      HTableClient htc = new HTableClient();

      //for (RowsToInsert.ColToInsert col : firstrow.columns)
      for (int pos = 0 ; pos < firstrow.columns.size(); pos++)
      {
         HTableClient.QualifiedColumn qc =
                  htc.new QualifiedColumn(firstrow.columns.get(pos).qualName);
         qualifierMap.put(pos, qc);
      }
    }

    long now = System.currentTimeMillis();
    for (RowsToInsert.RowInfo row : rows)
    {
       byte[] rowId = row.rowId;

       //for (RowsToInsert.ColToInsert col : row.columns)
       for (int pos = 0 ; pos < row.columns.size(); pos++)
       {
          HTableClient.QualifiedColumn qc = qualifierMap.get(pos);
          //htc.new QualifiedColumn(col.qualName);
          KeyValue kv = new KeyValue(rowId,
                                     Arrays.copyOf(qc.getFamily(),qc.getFamily().length) ,
                                     Arrays.copyOf(qc.getName(),qc.getName().length),
                                     now,
                                     row.columns.get(pos).colValue);
          writer.append(kv);
       }
   }

    logger.debug("End addToHFile() ");
       return true;
  }
  public boolean closeHFile() throws IOException
  {
    String s = "not null";
    if (writer == null)
        s = "NULL";
    logger.debug("HBulkLoadClient.closeHFile() called." + s);

    writer.close();
    return true;
  }

  public boolean doBulkLoad(String location, String tableName) throws Exception
  {
      Path dir = new Path(location );
      FileSystem fs = FileSystem.get(config);
      dir = dir.makeQualified(fs);
      HTable table = new HTable(config, tableName);

      LoadIncrementalHFiles loader = new LoadIncrementalHFiles(config);
      loader.doBulkLoad(dir, table);
      return true;
  }

  public boolean bulkLoadCleanup(String location) throws Exception
  {
      Path dir = new Path(location );
      FileSystem fs = FileSystem.get(config);
      dir = dir.makeQualified(fs);

      return fs.delete(dir, true);

  }
}
