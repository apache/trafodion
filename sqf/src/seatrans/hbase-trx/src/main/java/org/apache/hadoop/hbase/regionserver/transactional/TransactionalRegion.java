/**
 * Copyright 2009 The Apache Software Foundation Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements. See the NOTICE file distributed with this work for additional information regarding
 * copyright ownership. The ASF licenses this file to you under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain a copy of the License at
 * http://www.apache.org/licenses/LICENSE-2.0 Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND,
 * either express or implied. See the License for the specific language governing permissions and limitations under the
 * License.
 */
package org.apache.hadoop.hbase.regionserver.transactional;

import java.io.*;
import java.util.List;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.HRegionInfo;
import org.apache.hadoop.hbase.HTableDescriptor;
import org.apache.hadoop.hbase.client.Scan;
import org.apache.hadoop.hbase.regionserver.HRegion;
import org.apache.hadoop.hbase.regionserver.HRegionFileSystem;
import org.apache.hadoop.hbase.regionserver.KeyValueScanner;
import org.apache.hadoop.hbase.regionserver.RegionScanner;
import org.apache.hadoop.hbase.regionserver.RegionServerServices;
import org.apache.hadoop.hbase.regionserver.wal.HLog;
import org.apache.hadoop.hbase.util.CancelableProgressable;

/**
 * HRegion extension providing access to the HRegion protected method,
 *  public RegionScanner getScanner(final Scan scan, 
 *                                  final List<KeyValueScanner> scanners)
 */
public class TransactionalRegion extends HRegion {

	static final Log LOG = LogFactory.getLog(TransactionalRegion.class);

	/**
	 * @param basedir
	 * @param log
	 * @param fs
	 * @param conf
	 * @param regionInfo
	 * @param htd           
	 * @param rsServices    
	 */
  
	public TransactionalRegion(final Path basedir,
                                   final HLog log,
			           final FileSystem fs,
                                   final Configuration conf,
		                   final HRegionInfo regionInfo,
                                   final HTableDescriptor htd,
			           final RegionServerServices rsServices) {
		super(basedir, log, fs, conf, regionInfo, htd, rsServices);
	}
   
	/**
	 * @param fs
	 * @param log
	 * @param conf
	 * @param regionInfo
	 * @param htd           
	 * @param rsServices    
	 */

	public TransactionalRegion(final HRegionFileSystem fs,
                                   final HLog log,
			           final Configuration conf,
			           final HTableDescriptor htd,
			           final RegionServerServices rsServices) {
		super(fs, log, conf, htd, rsServices);
	}

	/**
	 * openHRegion
	 * @param reporter
	 *
	 * Placeholder for any TransactionalRegion-specific logic.
	 */
 
        @Override
	protected HRegion openHRegion(final CancelableProgressable reporter)
			throws IOException {
                LOG.trace("openHRegion -- ENTRY");
		super.openHRegion(reporter);
                LOG.trace("openHRegion -- EXIT");
		return this;
	}
   
	/**
	 * Get a transactional scanner.
	 * @param scan
	 * @param scanners
	 *
	 * Allows for access to the protected HRegion getScanner method.
	 */
        @Override
	public RegionScanner getScanner(final Scan scan, 
                                        final List<KeyValueScanner> scanners)
			throws IOException {
                LOG.trace("TransactionalRegion getScanner -- Calling super getScanner" );
		return super.getScanner(scan, scanners);
        }
}
