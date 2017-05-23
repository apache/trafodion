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
import org.apache.hadoop.hbase.HConstants;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.ServerName;

public class TransactionRegionLocation extends HRegionLocation {

  static final Log LOG = LogFactory.getLog(TransactionRegionLocation.class);

  public boolean tableRecordedDropped;
  /*
   public TransactionRegionLocation(HRegionInfo regionInfo, final String hostname, final int port) {
     //ServerName
     ServerName sn = new ServerName(hostname, port, 0);
     //regionInfo, hostname, port);
   }
   */

  public TransactionRegionLocation(HRegionInfo regionInfo, ServerName servName) {
    super(regionInfo, servName);
    tableRecordedDropped = false;
  }

  public void setTableRecordedDropped()
  {
    tableRecordedDropped = true;
    if (LOG.isTraceEnabled()) LOG.trace("Table recorded dropped for region:" + super.getRegionInfo());
  }
  public boolean isTableRecodedDropped()
  {
    return tableRecordedDropped;
  }

   @Override
   public int compareTo(HRegionLocation o) {
      if (o == null) {
        if (LOG.isDebugEnabled()) LOG.debug("CompareTo TransactionRegionLocation object is null");
        return 1;
      }

      if (LOG.isDebugEnabled()) LOG.debug("CompareTo TransactionRegionLocation Entry:  TableNames :\n      mine: "
              + this.getRegionInfo().getTable().getNameAsString() + "\n object's : " + o.getRegionInfo().getTable().getNameAsString());

      // Make sure this is the same table
      int result = this.getRegionInfo().getTable().compareTo(o.getRegionInfo().getTable());
      if (result != 0){
         if (LOG.isDebugEnabled()) LOG.debug("compareTo TransactionRegionLocation TableNames are different: result is " + result);
         return result;
      }

      if (LOG.isDebugEnabled()) LOG.debug("Tables match - comparing keys for "
              + this.getRegionInfo().getTable().getNameAsString()
              + "\n This start key    : " + Hex.encodeHexString(this.getRegionInfo().getStartKey())
              + "\n Object's start key: " + Hex.encodeHexString(o.getRegionInfo().getStartKey())
              + "\n This end key    : " + Hex.encodeHexString(this.getRegionInfo().getEndKey())
              + "\n Object's end key: " + Hex.encodeHexString(o.getRegionInfo().getEndKey()));

      // Here we are going to compare the keys as a range we can return 0
      // For these comparisons it's important to remember that 'this' is the object that is being added
      // and that 'object' is an object already added in the participationRegions set.
      //
      // We are trying to limit the registration of daughter regions after a region split.
      // So if a location is already added whose startKey is less than ours and whose end
      // key is greater than ours we will return 0 so that 'this' does not get added into
      // the participatingRegions list.

      // firstKeyInRange will be true if object's startKey is less than ours.
      int startKeyResult = Bytes.compareTo(this.getRegionInfo().getStartKey(), o.getRegionInfo().getStartKey());
      boolean firstKeyInRange = startKeyResult >= 0;
      boolean objLastKeyInfinite = Bytes.equals(o.getRegionInfo().getEndKey(), HConstants.EMPTY_END_ROW);
      boolean thisLastKeyInfinite = Bytes.equals(this.getRegionInfo().getEndKey(), HConstants.EMPTY_END_ROW);
      int endKeyResult = Bytes.compareTo(this.getRegionInfo().getEndKey(), o.getRegionInfo().getEndKey());

      // lastKey is in range if the existing object has an infinite end key, no matter what this end key is.
      boolean lastKeyInRange =  objLastKeyInfinite || ( ! thisLastKeyInfinite && endKeyResult <= 0);
      if (LOG.isDebugEnabled()) LOG.debug("firstKeyInRange " + firstKeyInRange + " lastKeyInRange " + lastKeyInRange);

      if (firstKeyInRange && lastKeyInRange) {
         if (LOG.isDebugEnabled()) LOG.debug("Object's region contains this region's start and end keys.  Regions match for "
                                   + o.getRegionInfo().getTable().getNameAsString());
         return 0;
      }

      if (startKeyResult != 0){
         if (LOG.isDebugEnabled()) LOG.debug("compareTo TransactionRegionLocation startKeys don't match: result is " + startKeyResult);
         return startKeyResult;
      }

      if (objLastKeyInfinite) {
         if (LOG.isInfoEnabled()) LOG.info("Object's region contains this region's end keys for "
                  + o.getRegionInfo().getTable().getNameAsString()
                  + "\n This start key    : " + Hex.encodeHexString(this.getRegionInfo().getStartKey())
                  + "\n Object's start key: " + Hex.encodeHexString(o.getRegionInfo().getStartKey())
                  + "\n This end key    : " + Hex.encodeHexString(this.getRegionInfo().getEndKey())
                  + "\n Object's end key: " + Hex.encodeHexString(o.getRegionInfo().getEndKey()));
      }

      if (this.getRegionInfo().getStartKey().length != 0 && this.getRegionInfo().getEndKey().length == 0) {
         if (LOG.isDebugEnabled()) LOG.debug("compareTo TransactionRegionLocation \"this\" is the last region: result is 1");
         return 1; // this is last region
      }
      if (o.getRegionInfo().getStartKey().length != 0 && o.getRegionInfo().getEndKey().length == 0) {
         if (LOG.isDebugEnabled()) LOG.debug("compareTo TransactionRegionLocation \"object\" is the last region: result is -1");
         return -1; // o is the last region
      }
      if (LOG.isDebugEnabled()) LOG.debug("compareTo TransactionRegionLocation endKeys comparison: result is " + endKeyResult);
      return endKeyResult;

   }

  /**
   * @see java.lang.Object#equals(java.lang.Object)
   */
  @Override
  public boolean equals(Object o) {
    if (LOG.isDebugEnabled()) LOG.debug("equals ENTRY: this: " + this + " o: " + o);
    if (this == o) {
      if (LOG.isDebugEnabled()) LOG.debug("equals same object: " + o);
      return true;
    }
    if (o == null) {
      if (LOG.isDebugEnabled()) LOG.debug("equals o is null");
      return false;
    }
    if (!(o instanceof HRegionLocation)) {
      if (LOG.isDebugEnabled()) LOG.debug("equals o is not an instance of: " + o);
      return false;
    }
    return this.compareTo((HRegionLocation)o) == 0;
  }

  /**
   * toString
   * @return String this
   *
   */
  @Override
  public String toString() {
    return super.toString() + "encodedName " + super.getRegionInfo().getEncodedName()
            + " start key: " + Hex.encodeHexString(super.getRegionInfo().getStartKey())
            + " end key: " + Hex.encodeHexString(super.getRegionInfo().getEndKey())
            + " tableRecodedDropped " + isTableRecodedDropped();
  }
}
