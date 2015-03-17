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


package org.apache.hadoop.hbase.client;

import java.io.IOException;
import java.nio.charset.Charset;
import java.util.List;

import org.apache.hadoop.hbase.HColumnDescriptor;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.KeyValue;
import org.apache.hadoop.hbase.client.Result;
import org.apache.hadoop.hbase.util.Bytes;


public class SsccConst {

  public static final Charset META_CHARSET = Charset.forName("UTF-8");

  // Trasaction metatable --->
  public static final byte[] TRANSACTION_STATUS = "st".getBytes(META_CHARSET);
  public static final byte[] TRANSACTION_COMMIT_ID = "cid"
      .getBytes(META_CHARSET);
  // <--- Trasaction metatable
  
  public static final byte TRX_ACTIVE_STATE = 1;
  public static final byte TRX_COMMITTED_STATE = 2;
  public static final byte TRX_ABORTED_STATE = 3;

  public static final byte[] STATUS_COL = "_st".getBytes(META_CHARSET);
  public static final byte[] VERSION_COL = "_ver".getBytes(META_CHARSET);
  public static final byte[] COLUMNS_COL = "_col".getBytes(META_CHARSET);
  
  public static final byte S_STATEFUL_BYTE = 0x1;
  public static final byte S_STATELESS_BYTE = 0x2;
  public static final byte S_DELETE_BYTE = 0x3;

  public static final byte V_NORMAL_BYTE = 0x1;
  public static final byte V_DELETE_BYTE = 0x2;

  /**
    Three in-row metadata are required for SSCC: status[], versions[], columns[]
    status[]  is a list of key:value pair to save which transaction currently update a row
    each element in this list indicate an update, so it should tell which transaction did the update
    and what type of update: stateful or stateless.
    format of each element in status[]:
      A byte[] array, start with 1 byte as value, 8 bytes (Long) as transactionID.

        type     | Byte               Long   
      ==========  =============   ====================
        content  | S_xx_BYTE          transactionID            
			  
    version[] is a list of key:value pair to save info about committed rows
    each element in this list indicate a commit by a transaction. And it can tell us which transaction already
    commit the update to a row.
    format of each element in version[]:
      A byte[] array, start with 1 byte as value, 8 bytes (Long) as startId.
      
        type     | Byte               Long   
      ==========  =============   ====================
        content  | S_xx_BYTE          startId   
	
	column[]  is a list of column names
	          TBD
	
  **/
  
  // This API will combine the two input value into a byte[]: 
  // Input transcation startID and ths delete flag
  // return a byte[] which can be put into the version[] directly.
  public static byte[] generateVersionValue(long transStartId, boolean isDelete) {
    byte[] ret = new byte[Bytes.SIZEOF_LONG + 1];
    ret[0] = isDelete ? V_DELETE_BYTE : V_NORMAL_BYTE;
    Bytes.putLong(ret, 1, transStartId);
    return ret;
  }
  
  // Returns the startId from a byte[] in version list
  public static long getVersionStartID(byte[] value)
  {
	return Bytes.toLong(value, 1);
  }
  
  // Returns the value of a given version element.
  // could be two types: 
  //    V_NORMAL_BYTE  : this is a normal put
  //    V_DELETE_BYTE  : this is a delete put
  public static byte getVersionType(byte[] value) 
  {
    return value[0];
  }
 
  // generate a byte[] for status[], currently only a byte is required:
  // it can be three possible values:
  //       S_STATEFUL_BYTE    : this is a stateful put
  //       S_STATELESS_BYTE   : this is a stateless put
  //       S_DELETE_BYTE      : this is a delete put
  // the return byte[] can be write into metadata status[] directly
  public static byte[] generateStatusValue(byte s, long id)
  {
    byte[] ret = new byte[1 + Bytes.SIZEOF_LONG];
	ret[0]=s;
    Bytes.putLong(ret, 1, id);
	return ret;
  }
  
  // returns the value of a given status[] element
  public static byte getStatusValue(byte[] value)
  {
    return value[0];
  }

    /**
     * Check the status value to see if this row is DELETE
    */
    public static boolean isDeleteStatus(byte[] value) {
        if (value == null || value.length < 1) return false;
        return value[0] == S_DELETE_BYTE;
    }

    /**
     * Check the version value to see if this row is DELETE
    */
    public static boolean isDeleteVersion(byte[] value) {
        if (value == null || value.length < 1) return false;
        return value[0] == V_DELETE_BYTE;
    }

    public static boolean isSelfUpdate(List<byte[]> statusList, long startId)
    {
        for( int i=0; i<statusList.size(); i++)
        {
            if(isDeleteStatus(statusList.get(i)) != true)
                return true;
        }
        return false;
    }

    /**
     * Check the status value to see if this row is DELETE
    */
    public static boolean isStateful(byte[] value) {
        if (value == null || value.length < 1) return false;
        return value[0] == S_DELETE_BYTE;
    }    
    
    public static long getTransactionId(byte[] value)
    {
        return Bytes.toLong(value, 1);
    }
}
