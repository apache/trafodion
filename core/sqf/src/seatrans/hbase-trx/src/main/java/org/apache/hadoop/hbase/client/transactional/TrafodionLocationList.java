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

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.commons.codec.binary.Hex;

import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.ByteArrayKey;
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.TableName;

import java.util.HashSet;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.ArrayList;
//import java.util.TreeMap;

import com.google.protobuf.ByteString;

import java.io.IOException;

public class TrafodionLocationList {

  static final Log LOG = LogFactory.getLog(TrafodionLocationList.class);

  private HashMap<String, HashMap<ByteArrayKey, TransactionRegionLocation>> list;
 
  public TrafodionLocationList() throws IOException {
    list = new HashMap<String, HashMap<ByteArrayKey, TransactionRegionLocation>>();
  }

  public synchronized boolean add(TransactionRegionLocation trl) {
     if (LOG.isDebugEnabled()) LOG.debug("add Entry:  trl: " + trl);
     boolean added = false;
     String tableString = trl.getRegionInfo().getTable().getNameAsString();
     if (LOG.isDebugEnabled()) LOG.debug("Retrieving locations map for table: " + tableString);

     ByteArrayKey startKey =  new ByteArrayKey(trl.getRegionInfo().getStartKey());
     HashMap<ByteArrayKey, TransactionRegionLocation> locations = null;
     locations = list.get(tableString);
     if (locations != null) {
        // TableName already in the map.  Add the location to the set
        if (! locations.containsKey(startKey)) {
           added = true;
           locations.put(startKey,trl);
        }
     }
     else {
        // TableName not in the Map.  We can add it immediately.
        locations = new HashMap<ByteArrayKey,TransactionRegionLocation>();
        locations.put(startKey,trl);
        list.put(tableString, locations);
        added = true;
        if (LOG.isDebugEnabled()) LOG.debug("created locations map and added entry for keyString: " + startKey);
     }
     return added;
  }

  public HashMap<String, HashMap<ByteArrayKey,TransactionRegionLocation>> getList() {
     if (LOG.isTraceEnabled()) LOG.trace("getList Entry");
     return list;
  }

  public int tableCount() {
     return list.size();
  }

  public int regionCount() {
     int count = 0;
    
     for (Map.Entry<String, HashMap<ByteArrayKey, TransactionRegionLocation>> tableMap : list.entrySet())
        count +=  tableMap.getValue().size();
     return count;
  }


  public synchronized void clear() {
     list = new HashMap<String, HashMap<ByteArrayKey, TransactionRegionLocation>>();
     return;
  }
  
  public int size()
  {
     int size = 0;
     for (Map.Entry<String, HashMap<ByteArrayKey,TransactionRegionLocation>> tableMap : list.entrySet()) {
        size +=  tableMap.getValue().size();
     }
     return size;
  }


  /**
   * toString
   * @return String this
   *
   */
  @Override
  public String toString() {
     StringBuilder builder = new StringBuilder();
     for (Map.Entry<String, HashMap<ByteArrayKey,TransactionRegionLocation>> tableMap : list.entrySet()) {
        builder.append( "table " + tableMap.getKey() + "\n");
        for (TransactionRegionLocation loc : tableMap.getValue().values()) {
           builder.append("   start key: " + ((loc.getRegionInfo().getStartKey() != null) ?
                    (Bytes.equals(loc.getRegionInfo().getStartKey(), HConstants.EMPTY_START_ROW) ?
                          "INFINITE" : Hex.encodeHexString(loc.getRegionInfo().getStartKey())) : "NULL")
            + "   end key: " + ((loc.getRegionInfo().getEndKey() != null) ?
                    (Bytes.equals(loc.getRegionInfo().getEndKey(), HConstants.EMPTY_END_ROW) ?
                          "INFINITE" : Hex.encodeHexString(loc.getRegionInfo().getEndKey())) : "NULL")
            + "\n");
       }
    }
    return builder.toString();
  }
}
