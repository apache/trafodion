// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2014-2015 Hewlett-Packard Development Company, L.P.
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
import java.net.URI;
import java.net.URISyntaxException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Iterator;
import java.io.File;

import org.apache.commons.io.FileUtils;
import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FileUtil;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.hbase.client.HBaseAdmin;
import org.apache.hadoop.hbase.client.HTable;
import org.apache.hadoop.hbase.client.Put;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.filter.FilterList;
import org.apache.hadoop.hbase.filter.SingleColumnValueFilter;
import org.apache.hadoop.hbase.filter.CompareFilter.CompareOp;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.security.access.AccessController;
import org.apache.hadoop.hbase.security.access.UserPermission;
import org.apache.hadoop.hbase.security.access.Permission;
import org.apache.hadoop.hbase.MasterNotRunningException;
import org.apache.hadoop.hbase.ZooKeeperConnectionException;
import org.apache.hadoop.hbase.snapshot.SnapshotCreationException;
import org.apache.hadoop.hbase.protobuf.generated.HBaseProtos.SnapshotDescription;
import org.apache.hadoop.hbase.protobuf.generated.HBaseProtos.SnapshotDescription.Type;
import org.apache.hadoop.hbase.snapshot.RestoreSnapshotException;
import org.apache.hadoop.hbase.io.hfile.CacheConfig;
//import org.apache.hadoop.hbase.io.hfile.Compression;
import org.apache.hadoop.hbase.io.hfile.HFile;
import org.apache.hadoop.hbase.io.hfile.HFileContext;
import org.apache.hadoop.hbase.io.hfile.HFileContextBuilder;
import org.apache.hadoop.hbase.io.compress.*;
//import org.apache.hadoop.hbase.io.compress.Compression;
//import org.apache.hadoop.hbase.io.hfile.Compression;
//import org.apache.hadoop.hbase.io.hfile.Compression.Algorithm;
import org.apache.hadoop.hbase.io.encoding.DataBlockEncoding;
import org.apache.hadoop.hbase.mapreduce.LoadIncrementalHFiles;
import org.apache.hadoop.hbase.regionserver.BloomType; 
//import org.apache.hadoop.hbase.regionserver.StoreFile.BloomType ;
//import org.apache.hadoop.hbase.client.Durability;

import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.permission.FsPermission;
import org.trafodion.sql.HBaseAccess.HTableClient;
//import org.trafodion.sql.HBaseAccess.HBaseClient;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.client.ResultScanner;

import java.nio.ByteBuffer;
import java.nio.file.Files;

import org.apache.hive.jdbc.HiveDriver;
import java.sql.Connection;
import java.sql.Statement;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.lang.ClassNotFoundException;

public class HBulkLoadClient
{
  
  private final static FsPermission PERM_ALL_ACCESS = FsPermission.valueOf("-rwxrwxrwx");
  private final static FsPermission PERM_HIDDEN = FsPermission.valueOf("-rwx--x--x");
  private final static String BULKLOAD_STAGING_DIR = "hbase.bulkload.staging.dir";
  private final static long MAX_HFILE_SIZE = 10737418240L; //10 GB
  
  public static int BLOCKSIZE = 64*1024;
  public static String COMPRESSION = Compression.Algorithm.NONE.getName();
  String lastError;
  static Logger logger = Logger.getLogger(HBulkLoadClient.class.getName());
  Configuration config;
  HFile.Writer writer;
  String hFileLocation;
  String hFileName;
  long maxHFileSize = MAX_HFILE_SIZE;
  FileSystem fileSys = null;
  String compression = COMPRESSION;
  int blockSize = BLOCKSIZE;
  DataBlockEncoding dataBlockEncoding = DataBlockEncoding.NONE;

  public HBulkLoadClient()
  {
    if (logger.isDebugEnabled()) logger.debug("HBulkLoadClient.HBulkLoadClient() called.");
  }

  public HBulkLoadClient(Configuration conf) throws IOException
  {
    if (logger.isDebugEnabled()) logger.debug("HBulkLoadClient.HBulkLoadClient(...) called.");
    config = conf;
  }

  public String getLastError() {
    return lastError;
  }

  void setLastError(String err) {
      lastError = err;
  }
  public boolean initHFileParams(String hFileLoc, String hFileNm, long userMaxSize /*in MBs*/, String tblName,
                                 String sampleTblName, String sampleTblDDL) 
  throws UnsupportedOperationException, IOException, SQLException, ClassNotFoundException
  {
    if (logger.isDebugEnabled()) logger.debug("HBulkLoadClient.initHFileParams() called.");
    
    hFileLocation = hFileLoc;
    hFileName = hFileNm;
    
    HTable myHTable = new HTable(config, tblName);
    HTableDescriptor hTbaledesc = myHTable.getTableDescriptor();
    HColumnDescriptor[] hColDescs = hTbaledesc.getColumnFamilies();
    if (hColDescs.length != 1 )
    {
      myHTable.close();
      throw new UnsupportedOperationException ("only one family is supported.");
    }
    
    compression= hColDescs[0].getCompression().getName();
    blockSize= hColDescs[0].getBlocksize();
    dataBlockEncoding = hColDescs[0].getDataBlockEncoding();
    
    if (userMaxSize == 0)
    {
      if (hTbaledesc.getMaxFileSize()==-1)
      {
        maxHFileSize = MAX_HFILE_SIZE;
      }
      else
      {
        maxHFileSize = hTbaledesc.getMaxFileSize();
      }
    }
    else 
      maxHFileSize = userMaxSize * 1024 *1024;  //maxSize is in MBs

    myHTable.close();

    if (sampleTblDDL.length() > 0)
    {
      Class.forName("org.apache.hive.jdbc.HiveDriver");
      Connection conn = DriverManager.getConnection("jdbc:hive2://", "hive", "");
      Statement stmt = conn.createStatement();
      stmt.execute("drop table if exists " + sampleTblName);
      //System.out.println("*** DDL for Hive sample table is: " + sampleTblDDL);
      stmt.execute(sampleTblDDL);
    }

    return true;
  }
  public boolean doCreateHFile() throws IOException, URISyntaxException
  {
    if (logger.isDebugEnabled()) logger.debug("HBulkLoadClient.doCreateHFile() called.");
    
    if (hFileLocation == null )
      throw new NullPointerException(hFileLocation + " is not set");
    if (hFileName == null )
      throw new NullPointerException(hFileName + " is not set");
    
    closeHFile();
    
    if (fileSys == null)
     fileSys = FileSystem.get(config); 

    Path hfilePath = new Path(new Path(hFileLocation ), hFileName + "_" +  System.currentTimeMillis());
    hfilePath = hfilePath.makeQualified(hfilePath.toUri(), null);

    if (logger.isDebugEnabled()) logger.debug("HBulkLoadClient.createHFile Path: " + hfilePath);

    try
    {
      HFileContext hfileContext = new HFileContextBuilder()
                                 .withBlockSize(blockSize)
                                 .withCompression(Compression.getCompressionAlgorithmByName(compression))
                                 .withDataBlockEncoding(dataBlockEncoding)
                                 .build();

      writer =    HFile.getWriterFactory(config, new CacheConfig(config))
                     .withPath(fileSys, hfilePath)
                     .withFileContext(hfileContext)
                     .withComparator(KeyValue.COMPARATOR)
                     .create();
      if (logger.isDebugEnabled()) logger.debug("HBulkLoadClient.createHFile Path: " + writer.getPath() + "Created");
    }
    catch (IOException e)
    {
       if (logger.isDebugEnabled()) logger.debug("HBulkLoadClient.doCreateHFile Exception" + e.getMessage());
       throw e;
    }
    return true;
  }
  
  public boolean isNewFileNeeded() throws IOException
  {
    if (writer == null)
      return true;
    
    if (fileSys == null)
      fileSys = FileSystem.get(writer.getPath().toUri(),config);
    
    if (fileSys.getFileStatus(writer.getPath()).getLen() > maxHFileSize)
     return true;

    return false;
  }

  public boolean addToHFile(short rowIDLen, Object rowIDs,
                Object rows) throws IOException, URISyntaxException
  {
     if (logger.isDebugEnabled()) logger.debug("Enter addToHFile() ");
     Put put;
    if (isNewFileNeeded())
    {
      doCreateHFile();
    }
     ByteBuffer bbRows, bbRowIDs;
     short numCols, numRows;
     short colNameLen;
     int colValueLen;
     byte[] colName, colValue, rowID;

     bbRowIDs = (ByteBuffer)rowIDs;
     bbRows = (ByteBuffer)rows;
     numRows = bbRowIDs.getShort();
     HTableClient htc = new HTableClient();
     long now = System.currentTimeMillis();
     for (short rowNum = 0; rowNum < numRows; rowNum++) 
     {
        rowID = new byte[rowIDLen];
        bbRowIDs.get(rowID, 0, rowIDLen);
        numCols = bbRows.getShort();
        for (short colIndex = 0; colIndex < numCols; colIndex++)
        {
            colNameLen = bbRows.getShort();
            colName = new byte[colNameLen];
            bbRows.get(colName, 0, colNameLen);
            colValueLen = bbRows.getInt();
            colValue = new byte[colValueLen];
            bbRows.get(colValue, 0, colValueLen);
            KeyValue kv = new KeyValue(rowID,
                                htc.getFamily(colName), 
                                htc.getName(colName), 
                                now,
                                colValue);
            writer.append(kv);
        } 
    }
    if (logger.isDebugEnabled()) logger.debug("End addToHFile() ");
       return true;
  }

  public boolean closeHFile() throws IOException
  {
    if (logger.isDebugEnabled()) logger.debug("HBulkLoadClient.closeHFile() called." + ((writer == null) ? "NULL" : "NOT NULL"));

    if (writer == null)
      return false;
    
    writer.close();
    return true;
  }

  private boolean createSnapshot( String tableName, String snapshotName)
  throws MasterNotRunningException, IOException, SnapshotCreationException, InterruptedException
  {
    HBaseAdmin admin = null;
    try 
    {
      admin = new HBaseAdmin(config);
      List<SnapshotDescription>  lstSnaps = admin.listSnapshots();
      if (! lstSnaps.isEmpty())
      {
        for (SnapshotDescription snpd : lstSnaps) 
        {
            if (snpd.getName().compareTo(snapshotName) == 0)
            {
              if (logger.isDebugEnabled()) logger.debug("HbulkLoadClient.createSnapshot() -- deleting: " + snapshotName + " : " + snpd.getName());
              admin.deleteSnapshot(snapshotName);
            }
        }
      }
      admin.snapshot(snapshotName, tableName);
   }
    catch (Exception e)
    {
      //log exeception and throw the exception again to teh parent
      if (logger.isDebugEnabled()) logger.debug("HbulkLoadClient.createSnapshot() - Exception: " + e);
      throw e;
    }
    finally
    {
      //close HBaseAdmin instance 
      if (admin !=null)
        admin.close();
    }
    return true;
  }
  
  private boolean restoreSnapshot( String snapshotName, String tableName)
  throws IOException, RestoreSnapshotException
  {
    HBaseAdmin admin = null;
    try
    {
      admin = new HBaseAdmin(config);
      if (! admin.isTableDisabled(tableName))
          admin.disableTable(tableName);
      
      admin.restoreSnapshot(snapshotName);
  
      admin.enableTable(tableName);
    }
    catch (Exception e)
    {
      //log exeception and throw the exception again to the parent
      if (logger.isDebugEnabled()) logger.debug("HbulkLoadClient.restoreSnapshot() - Exception: " + e);
      throw e;
    }
    finally
    {
      //close HBaseAdmin instance 
      if (admin != null) 
        admin.close();
    }

    return true;
  }
  private boolean deleteSnapshot( String snapshotName, String tableName)
      throws IOException
  {
    
    HBaseAdmin admin = null;
    boolean snapshotExists = false;
    try
    {
      admin = new HBaseAdmin(config);
      List<SnapshotDescription>  lstSnaps = admin.listSnapshots();
      if (! lstSnaps.isEmpty())
      {
        for (SnapshotDescription snpd : lstSnaps) 
        {
          //System.out.println("here 1: " + snapshotName + snpd.getName());
          if (snpd.getName().compareTo(snapshotName) == 0)
          {
            //System.out.println("deleting: " + snapshotName + " : " + snpd.getName());
            snapshotExists = true;
            break;
          }
        }
      }
      if (!snapshotExists)
        return true;
      if (admin.isTableDisabled(tableName))
          admin.enableTable(tableName);
      admin.deleteSnapshot(snapshotName);
    }
    catch (Exception e)
    {
      //log exeception and throw the exception again to the parent
      if (logger.isDebugEnabled()) logger.debug("HbulkLoadClient.restoreSnapshot() - Exception: " + e);
      throw e;
    }
    finally 
    {
      //close HBaseAdmin instance 
      if (admin != null) 
        admin.close();
    }
    return true;
  }
  
  private void doSnapshotNBulkLoad(Path hFilePath, String tableName, HTable table, LoadIncrementalHFiles loader, boolean snapshot)
  throws MasterNotRunningException, IOException, SnapshotCreationException, InterruptedException, RestoreSnapshotException
  {
    HBaseAdmin admin = new HBaseAdmin(config);
    String snapshotName= null;
    if (snapshot)
    {
      snapshotName = tableName + "_SNAPSHOT";
      createSnapshot(tableName, snapshotName);
      if (logger.isDebugEnabled()) logger.debug("HbulkLoadClient.doSnapshotNBulkLoad() - snapshot created: " + snapshotName);
    }
    try
    {
      if (logger.isDebugEnabled()) logger.debug("HbulkLoadClient.doSnapshotNBulkLoad() - bulk load started ");
      loader.doBulkLoad(hFilePath, table);
      if (logger.isDebugEnabled()) logger.debug("HbulkLoadClient.doSnapshotNBulkLoad() - bulk load is done ");
    }
    catch (IOException e)
    {
      if (logger.isDebugEnabled()) logger.debug("HbulkLoadClient.doSnapshotNBulkLoad() - Exception: " + e.toString());
      if (snapshot)
      {
        restoreSnapshot(snapshotName, tableName);
        if (logger.isDebugEnabled()) logger.debug("HbulkLoadClient.doSnapshotNBulkLoad() - snapshot restored: " + snapshotName);
        deleteSnapshot(snapshotName, tableName);
        if (logger.isDebugEnabled()) logger.debug("HbulkLoadClient.doSnapshotNBulkLoad() - snapshot deleted: " + snapshotName);
        throw e;
      }
    }
    finally
    {
      if  (snapshot)
      {
        deleteSnapshot(snapshotName, tableName);
        if (logger.isDebugEnabled()) logger.debug("HbulkLoadClient.doSnapshotNBulkLoad() - snapshot deleted: " + snapshotName);
      }
    }
    
  }
  public boolean doBulkLoad(String prepLocation, String tableName, boolean quasiSecure, boolean snapshot) throws Exception
  {
    if (logger.isDebugEnabled()) logger.debug("HBulkLoadClient.doBulkLoad() - start");
    if (logger.isDebugEnabled()) logger.debug("HBulkLoadClient.doBulkLoad() - Prep Location: " + prepLocation + 
                                             ", Table Name:" + tableName + 
                                             ", quasisecure : " + quasiSecure +
                                             ", snapshot: " + snapshot);

      
    HTable table = new HTable(config, tableName);
    LoadIncrementalHFiles loader = new LoadIncrementalHFiles(config);    
    Path prepPath = new Path(prepLocation );
    prepPath = prepPath.makeQualified(prepPath.toUri(), null);
    FileSystem prepFs = FileSystem.get(prepPath.toUri(),config);
    
    Path[] hFams = FileUtil.stat2Paths(prepFs.listStatus(prepPath));

    if (quasiSecure)
    {
      throw new Exception("HBulkLoadClient.doBulkLoad() - cannot perform load. Trafodion on secure HBase mode is not implemented yet");
    }
    else
    {
      if (logger.isDebugEnabled()) logger.debug("HBulkLoadClient.doBulkLoad() - adjusting hfiles permissions");
      for (Path hfam : hFams) 
      {
         Path[] hfiles = FileUtil.stat2Paths(prepFs.listStatus(hfam));
         prepFs.setPermission(hfam,PERM_ALL_ACCESS );
         for (Path hfile : hfiles)
         {
           if (logger.isDebugEnabled()) logger.debug("HBulkLoadClient.doBulkLoad() - adjusting hfile permissions:" + hfile);
           prepFs.setPermission(hfile,PERM_ALL_ACCESS);
           
         }
         //create _tmp dir used as temp space for Hfile processing
         FileSystem.mkdirs(prepFs, new Path(hfam,"_tmp"), PERM_ALL_ACCESS);
      }
      if (logger.isDebugEnabled()) logger.debug("HBulkLoadClient.doBulkLoad() - bulk load started. Loading directly from preparation directory");
      doSnapshotNBulkLoad(prepPath,tableName,  table,  loader,  snapshot);
      if (logger.isDebugEnabled()) logger.debug("HBulkLoadClient.doBulkLoad() - bulk load is done ");
    }
    return true;
  }

  public boolean bulkLoadCleanup(String location) throws Exception
  {
      Path dir = new Path(location );
      dir = dir.makeQualified(dir.toUri(), null);
      FileSystem fs = FileSystem.get(dir.toUri(),config);
      fs.delete(dir, true);
      
      return true;

  }
  
  public boolean release( ) throws IOException {
    if (writer != null)
    {
       writer.close();
       writer = null;
    }
    if (fileSys !=null)
    {
      fileSys.close();
      fileSys = null;
    }
    if (config != null) 
    {
      config = null;
    }
    if (hFileLocation != null)
    {
      hFileLocation = null;
    }
    if (hFileName != null)
    {
      hFileName = null;
    }

    if (compression != null)
    {
      compression = null;
    }
    return true;
}


}
