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

import java.io.IOException;
import java.io.FileNotFoundException;
import java.util.HashMap;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.lang.reflect.Field;
import java.util.Iterator;

import org.apache.log4j.PropertyConfigurator;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;

import org.apache.hadoop.util.StringUtils;

import org.apache.hadoop.hive.metastore.HiveMetaStoreClient;
import org.apache.hadoop.hive.metastore.api.Table;
import org.apache.hadoop.hive.metastore.api.Database;
import org.apache.hadoop.hive.metastore.api.MetaException;
import org.apache.hadoop.hive.metastore.api.NoSuchObjectException;
import org.apache.hadoop.hive.metastore.api.UnknownDBException;
import org.apache.hadoop.hive.metastore.api.StorageDescriptor;
import org.apache.hadoop.hive.metastore.api.FieldSchema;
import org.apache.hadoop.hive.metastore.api.SerDeInfo;
import org.apache.hadoop.hive.metastore.api.Order;
import org.apache.hadoop.hive.metastore.api.SerDeInfo;

// These are needed for the DDL_TIME constant. This class is different in Hive 0.10.
// We use Java reflection instead of importing the class statically. 
// For Hive 0.9 or lower
// import org.apache.hadoop.hive.metastore.api.Constants;
// For Hive 0.10 or higher
// import org.apache.hadoop.hive.metastore.api.hive_metastoreConstants;
import org.apache.hadoop.hive.conf.HiveConf;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.fs.FileSystem;

import java.sql.SQLException;
import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.Statement;
import java.sql.DriverManager;

public class HiveClient {
    public static final int Table_TABLE_NAME = 0;
    public static final int Table_DB_NAME = 1;
    public static final int Table_OWNER = 2;
    public static final int Table_CREATE_TIME = 3;
    public static final int Table_TABLE_TYPE = 4;
    public static final int Table_VIEW_ORIGINAL_TEXT = 5;
    public static final int Table_VIEW_EXPANDED_TEXT = 6;
    public static final int Table_SD_COMPRESSED = 7;
    public static final int Table_SD_LOCATION = 8;
    public static final int Table_SD_INPUT_FORMAT = 9;
    public static final int Table_SD_OUTPUT_FORMAT = 10;
    public static final int Table_SD_NUM_BUCKETS = 11;
    public static final int Table_NULL_FORMAT = 12;
    public static final int Table_FIELD_DELIM = 13;
    public static final int Table_LINE_DELIM = 14;
    public static final int Table_FIELD_COUNT  = 15;
    

    public static final int Col_NAME = 0;
    public static final int Col_TYPE = 1;
    public static final int Col_FIELD_COUNT  = 2;


    private static Logger logger = Logger.getLogger(HiveClient.class.getName());
    private final String lockPath="/trafodion/traflock";

    private static HiveConf hiveConf = null;
    private static ThreadLocal<HiveMetaStoreClient> hiveMetaClient  ;
    private static HiveMetaStoreClient hmsClient;
    private static String ddlTimeConst = null;

    private static Statement stmt = null;

    static {
         String confFile = System.getProperty("trafodion.log4j.configFile");
         System.setProperty("trafodion.root", System.getenv("TRAF_HOME"));
         if (confFile == null) 
         confFile = System.getenv("TRAF_CONF") + "/log4j.sql.config";
         PropertyConfigurator.configure(confFile);
         hiveConf = new HiveConf();
         hiveMetaClient = new ThreadLocal<HiveMetaStoreClient>();
         try {
             hmsClient = getHiveMetaClient();
             ddlTimeConst = getDDLTimeConstant();
         } catch (MetaException me)
         {
             throw new RuntimeException("Checked MetaException from HiveClient static block");
         }
    }
 
    public static boolean init() 
    { 
        return true;
    }

   private static HiveMetaStoreClient getHiveMetaClient() throws org.apache.hadoop.hive.metastore.api.MetaException 
   {
       HiveMetaStoreClient ts_hmsClient;
       ts_hmsClient = hiveMetaClient.get(); 
       if (ts_hmsClient == null) {
          ts_hmsClient = new HiveMetaStoreClient(hiveConf, null);
          hiveMetaClient.set(ts_hmsClient);
       }
       return ts_hmsClient;
   }  

    public static boolean close() 
    {	
        return true;	
    }
 
    public static boolean exists(String schName, String tblName)  
        throws MetaException, TException, UnknownDBException 
    {
        if (logger.isDebugEnabled()) logger.debug("HiveClient.exists(" + schName + " , " + tblName + ") called.");
        boolean result = getHiveMetaClient().tableExists(schName, tblName);
        return result;
    }


    public static long getRedefTime(String schName, String tblName)
        throws MetaException, TException, IOException
    {
        Table table;
        long modificationTime;
        if (logger.isDebugEnabled()) logger.debug("HiveClient.getRedefTime(" + schName + " , " + 
                     tblName + ") called.");
        try {
            table = getHiveMetaClient().getTable(schName, tblName);
        }
        catch (NoSuchObjectException x) {
            if (logger.isDebugEnabled()) logger.debug("Hive table no longer exists.");
            return 0;
        }
        long redefTime = table.getCreateTime();
        if (table.getParameters() != null){
            // those would be used without reflection
            //String rfTime = table.getParameters().get(Constants.DDL_TIME);
            //String rfTime = table.getParameters().get(hive_metastoreConstants.DDL_TIME);
            // determing the constant using reflection instead
            String rfTime = table.getParameters().get(ddlTimeConst);
            if (rfTime != null)
                redefTime = Long.parseLong(rfTime);
        }
        // createTime is in seconds
        // Assuming DDL_TIME is also in seconds
        redefTime *= 1000;
        // Get the lastest partition/file timestamp 
        int numPartKeys = table.getPartitionKeysSize();
        String rootDir = table.getSd().getLocation();
        long dirTime = 0;
        if (rootDir != null) {
           try {
              dirTime = HDFSClient.getHiveTableMaxModificationTs(rootDir, numPartKeys);
           } catch (FileNotFoundException e) {
           // ignore this exception
           }
        }
        if (dirTime > redefTime)
           modificationTime = dirTime;
        else
           modificationTime = redefTime;
        if (logger.isDebugEnabled()) logger.debug("RedefTime is " + redefTime);
        return modificationTime;
    }

    public static Object[] getAllSchemas() throws MetaException 
    {
        List<String> schemaList = (getHiveMetaClient().getAllDatabases());
        if (schemaList != null)
           return schemaList.toArray();
        else
           return null; 
    }

    public static Object[] getAllTables(String schName) 
        throws MetaException, TException 
    {
        try {
        Database db = getHiveMetaClient().getDatabase(schName);
        if (db == null)
            return null;

        List<String> tableList = getHiveMetaClient().getAllTables(schName);
        if (tableList != null)
           return tableList.toArray();
        else
           return null;
        } catch (NoSuchObjectException e) {
          return null;
        }
    }

    // Because Hive changed the name of the class containing internal constants changed
    // in Hive 0.10, we are using Java Reflection to get the value of the DDL_TIME constant.
    public static String getDDLTimeConstant()
        throws MetaException 
    {

        Class constsClass = null;
        Object constsFromReflection = null; 
        Field ddlTimeField = null;
        Object fieldVal = null;

        // Using the class loader, try to load either class by name.
        // Note that both classes have a default constructor and both have a static
        // String field DDL_TIME, so the rest of the code is the same for both.
        try { 
            try {
                constsClass = Class.forName(
                   // Name in Hive 0.10 and higher
                   "org.apache.hadoop.hive.metastore.api.hive_metastoreConstants");
            } catch (ClassNotFoundException e) { 
                // probably not found because we are using Hive 0.10 or later
                constsClass = null;
            } 
            if (constsClass == null) {
                constsClass = Class.forName(
                    // Name in Hive 0.9 and lower
                    "org.apache.hadoop.hive.metastore.api.Constants");
            }

            // Make a new object for this class, using the default constructor
            constsFromReflection = constsClass.newInstance(); 
        } catch (InstantiationException e) { 
            throw new MetaException("Instantiation error for metastore constants class");
        } catch (IllegalAccessException e) { 
            throw new MetaException("Illegal access exception");
        } catch (ClassNotFoundException e) { 
            throw new MetaException("Could not find Hive Metastore constants class");
        } 
        // Using Java reflection, get a reference to the DDL_TIME field
       try {
            ddlTimeField = constsClass.getField("DDL_TIME");
        } catch (NoSuchFieldException e) {
            throw new MetaException("Could not find DDL_TIME constant field");
        }
        // get the String object that represents the value of this field
      try {
            fieldVal = ddlTimeField.get(constsFromReflection);

        } catch (IllegalAccessException e) {
            throw new MetaException("Could not get value for DDL_TIME constant field");
        }

        return fieldVal.toString();
  }

  public static void executeHiveSQL(String ddl) 
        throws ClassNotFoundException, SQLException
  {
      if (stmt == null) {
          Class.forName("org.apache.hive.jdbc.HiveDriver");
          Connection con = null;
          String isSecureHadoop = System.getenv("SECURE_HADOOP");
          //If Kerberos is enabled, then we need to connect to remote hiveserver2 using hive principal
          if(isSecureHadoop != null && isSecureHadoop.equalsIgnoreCase("Y")){
              String hiveServer2Url = System.getenv("HIVESERVER2_URL");
              if(hiveServer2Url == null || hiveServer2Url.isEmpty()){
                  hiveServer2Url = "localhost:10000";
              }
              String hivePrincipal = System.getenv("HIVE_PRINCIPAL");
              con = DriverManager.getConnection("jdbc:hive2://" + hiveServer2Url+"/;principal=" + hivePrincipal, "hive", "");
          }else{
              con = DriverManager.getConnection("jdbc:hive2://", "hive", "");
          }
          stmt = con.createStatement();
      }

      try {

          stmt.execute(ddl);
      } catch (SQLException s) {
          throw s;            
      }
  }


  public boolean getHiveTableInfo(long jniObject, String schName, String tblName, boolean readPartn)
       throws MetaException, TException
  {
     Table table;
     try {
        table = getHiveMetaClient().getTable(schName, tblName);
     } catch (NoSuchObjectException x) {
         return false; 
     } 
     String[] tableInfo = new String[Table_FIELD_COUNT];
     tableInfo[Table_TABLE_NAME] = table.getTableName();
     tableInfo[Table_DB_NAME]= table.getDbName();
     tableInfo[Table_OWNER] = table.getOwner();
     tableInfo[Table_CREATE_TIME] = Integer.toString(table.getCreateTime());
     tableInfo[Table_TABLE_TYPE] = table.getTableType();
     tableInfo[Table_VIEW_ORIGINAL_TEXT] = table.getViewOriginalText();
     tableInfo[Table_VIEW_EXPANDED_TEXT] = table.getViewExpandedText();

     StorageDescriptor sd = table.getSd();
     tableInfo[Table_SD_COMPRESSED] = Boolean.toString(sd.isCompressed());
     tableInfo[Table_SD_LOCATION] = sd.getLocation();
     tableInfo[Table_SD_INPUT_FORMAT] = sd.getInputFormat();
     tableInfo[Table_SD_OUTPUT_FORMAT] = sd.getOutputFormat();
     tableInfo[Table_SD_NUM_BUCKETS] = Integer.toString(sd.getNumBuckets());

     SerDeInfo serDe = sd.getSerdeInfo(); 
     Map<String,String> serDeParams = serDe.getParameters();
     tableInfo[Table_NULL_FORMAT] = serDeParams.get("serialization.null.format");
     tableInfo[Table_FIELD_DELIM] = serDeParams.get("field.delim");
     tableInfo[Table_LINE_DELIM] = serDeParams.get("line.delim");

     // Columns in the table
     int numCols = sd.getColsSize();
     String[][] colInfo = new String[numCols][Col_FIELD_COUNT];
     Iterator<FieldSchema> fieldIterator = sd.getColsIterator();
     int i = 0;
     FieldSchema field;
     while (fieldIterator.hasNext()) {
        field = fieldIterator.next();
        colInfo[i][Col_NAME] = field.getName(); 
        colInfo[i][Col_TYPE] = field.getType(); 
        i++;
     }
     String[][] partKeyInfo = null;
     String[][] partKeyValues = null;
     String[] partNames = null;
     String[] bucketCols = null;
     String[] sortCols = null;
     int[] sortColsOrder = null;
     String[] paramsKey = null;
     String[] paramsValue = null;
     int numPartKeys = table.getPartitionKeysSize();
     if (numPartKeys > 0) {
        partKeyInfo = new String[numPartKeys][Col_FIELD_COUNT];
        Iterator<FieldSchema> partKeyIterator = table.getPartitionKeysIterator();
        i = 0;
        FieldSchema partKey;
        while (partKeyIterator.hasNext()) {
           partKey = partKeyIterator.next();
           partKeyInfo[i][Col_NAME] = partKey.getName(); 
           partKeyInfo[i][Col_TYPE] = partKey.getType(); 
           i++;
        }
        if (readPartn) {
           List<String> partNamesList = getHiveMetaClient().listPartitionNames(schName, tblName, (short)-1);
           if (partNamesList != null) {
              partNames = new String[partNamesList.size()];
              partNames = partNamesList.toArray(partNames); 
              i = 0;
              partKeyValues = new String[partNames.length][];
              for (i = 0; i < partNames.length; i++) {
                  partKeyValues[i] = new String[numPartKeys];
                  partKeyValues[i] = (String[])getHiveMetaClient().partitionNameToVals(partNames[i]).toArray(partKeyValues[i]); 
              }
           }
        } 
     }
     
     // Bucket Columns
     int numBucketCols = sd.getBucketColsSize();
     if (numBucketCols > 0) {
        bucketCols = new String[numBucketCols];
        Iterator<String> bucketColsIterator = sd.getBucketColsIterator();
        i = 0;
        while (bucketColsIterator.hasNext()) {
           bucketCols[i++] = bucketColsIterator.next();
        }
     }
    
     // Sort Columns
     int numSortCols = sd.getSortColsSize();
     if (numSortCols > 0) {
        sortCols = new String[numSortCols];
        sortColsOrder = new int[numSortCols];
        Iterator<Order> sortColsIterator = sd.getSortColsIterator();
        i = 0;
        Order sortOrder;
        while (sortColsIterator.hasNext()) {
           sortOrder = sortColsIterator.next();
           sortCols[i] = sortOrder.getCol();
           sortColsOrder[i] = sortOrder.getOrder(); 
           i++;
        } 
     }

     // Table Params
     Map<String, String> unsortedParams = table.getParameters();
     Map<String, String> params = new TreeMap<String, String>(unsortedParams);
     paramsKey = new String[params.size()];
     paramsValue = new String[params.size()]; 
     i = 0;
     for (Map.Entry<String,String> entry : params.entrySet()) {
          paramsKey[i] = entry.getKey();
          paramsValue[i] = entry.getValue();
          i++;
     }
     // Replace creation time with redefineTime 
     String rfTime = params.get(ddlTimeConst);
     if (rfTime != null)
        tableInfo[Table_CREATE_TIME] = rfTime;
     setTableInfo(jniObject, tableInfo, colInfo, partKeyInfo, bucketCols, sortCols, sortColsOrder, paramsKey, paramsValue, partNames, partKeyValues);     
     return true;
  }
  
  private native void setTableInfo(long jniObject, String[] tableInfo, String[][] colInfo, String[][] partKeyInfo,
                         String[] bucketCols, String[] sortCols, int[] sortColsOrder, String[] paramsKey, String[] paramsValue,
                         String[] partNames, String[][] partKeyValues);
  
}
