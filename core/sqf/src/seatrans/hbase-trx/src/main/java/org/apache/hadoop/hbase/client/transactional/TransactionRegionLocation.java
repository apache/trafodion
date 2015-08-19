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
package org.apache.hadoop.hbase.client.transactional;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.commons.codec.binary.Hex;

import org.apache.hadoop.hbase.util.Bytes;

import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HRegionLocation;
import org.apache.hadoop.hbase.ServerName;

public class TransactionRegionLocation extends HRegionLocation {

  static final Log LOG = LogFactory.getLog(TransactionRegionLocation.class);

  /*
   public TransactionRegionLocation(HRegionInfo regionInfo, final String hostname, final int port) {
     //ServerName
     ServerName sn = new ServerName(hostname, port, 0);
     //regionInfo, hostname, port);
   }
   */

  public TransactionRegionLocation(HRegionInfo regionInfo, ServerName servName) {
    super(regionInfo, servName);
  }

   @Override
   public int compareTo(HRegionLocation o) {
      if (LOG.isTraceEnabled()) LOG.trace("compareTo ENTRY: " + o);
      int result = super.getHostname().compareTo(o.getHostname());
      if (result != 0){
         if (LOG.isTraceEnabled()) LOG.trace("compareTo hostnames differ: mine: " + super.getHostname() + " hex: " +
              Hex.encodeHexString(Bytes.toBytes(super.getHostname())) + " object's: " + o.getHostname() + " hex: " + Hex.encodeHexString(Bytes.toBytes(o.getHostname())));
         return result;
      }
      result = super.getPort() - o.getPort();
      if (result != 0) {
         if (LOG.isTraceEnabled()) LOG.trace("compareTo ports differ: mine: " + super.getPort() + " object's: " + o.getPort());
         return result;
      }
      result = super.getRegionInfo().compareTo(o.getRegionInfo());
      if (result != 0) {
         if (LOG.isTraceEnabled()) LOG.trace("compareTo regionInfo differs: mine: " + super.getRegionInfo() + " hex: " +
             Hex.encodeHexString(super.getRegionInfo().getRegionName()) + " object's: " + o.getRegionInfo() + "hex: " +
             Hex.encodeHexString(o.getRegionInfo().getRegionName()));
      }
      if (LOG.isTraceEnabled()) LOG.trace("compareTo HRegionLocation result is: " + result );
      return result;
   }

   public int compareTo(TransactionRegionLocation o) {
      if (LOG.isTraceEnabled()) LOG.trace("compareTo TransactionRegionLocation ENTRY: " + o);
      int result = super.getHostname().compareTo(o.getHostname());
      if (result != 0){
         if (LOG.isTraceEnabled()) LOG.trace("compareTo hostnames differ: mine: " + super.getHostname() + " hex: " +
              Hex.encodeHexString(Bytes.toBytes(super.getHostname())) + " object's: " + o.getHostname() + " hex: " + Hex.encodeHexString(Bytes.toBytes(o.getHostname())));
         return result;
      }
      result = super.getPort() - o.getPort();
      if (result != 0) {
         if (LOG.isTraceEnabled()) LOG.trace("compareTo ports differ: mine: " + super.getPort() + " object's: " + o.getPort());
         return result;
      }
      result = super.getRegionInfo().compareTo(o.getRegionInfo());
      if (result != 0) {
         if (super.getRegionInfo().getEncodedName().compareTo(o.getRegionInfo().getEncodedName()) == 0) {
            if (LOG.isTraceEnabled()) LOG.trace("compareTo TransactionRegionLocation regionInfo differs:\n    mine: end key: " + Hex.encodeHexString(super.getRegionInfo().getEndKey()) + " hex: " +
               Hex.encodeHexString(super.getRegionInfo().getRegionName()) + "\n object's end key: " + Hex.encodeHexString(o.getRegionInfo().getEndKey()) + " hex: " +
               Hex.encodeHexString(o.getRegionInfo().getRegionName()) + "result is " + result);
         }
      }
      return result;
   }

  /**
   * @see java.lang.Object#equals(java.lang.Object)
   */
  @Override
  public boolean equals(Object o) {
    if (LOG.isTraceEnabled()) LOG.trace("equals ENTRY: this: " + this + " o: " + o);
    if (this == o) {
      if (LOG.isTraceEnabled()) LOG.trace("equals same object: " + o);
      return true;
    }
    if (o == null) {
      if (LOG.isTraceEnabled()) LOG.trace("equals o is null");
      return false;
    }
    if (!(o instanceof TransactionRegionLocation)) {
      if (LOG.isTraceEnabled()) LOG.trace("equals o is not an instance of: " + o);
      return false;
    }
    return this.compareTo((TransactionRegionLocation)o) == 0;
  }

}
