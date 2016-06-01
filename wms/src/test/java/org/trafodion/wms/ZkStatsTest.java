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

package org.trafodion.wms;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;

import java.io.*;
import java.util.*;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.util.List;

import org.apache.avro.*;
import org.apache.avro.io.*;
import org.apache.avro.generic.*;

import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.WatchedEvent;
import org.apache.zookeeper.Watcher;
import org.apache.zookeeper.ZooKeeper;
import org.apache.zookeeper.ZooDefs.Ids;
import org.apache.zookeeper.data.Stat;

import org.trafodion.wms.Constants;
import org.trafodion.wms.util.Bytes;
import org.junit.experimental.categories.*;

/**
 * Unit test for simple App.
 */
@Category(SmallTests.class)
public class ZkStatsTest extends TestCase
{
	/**
	 * Create the test case
	 *
	 * @param testName name of the test case
	 */
	public ZkStatsTest( String testName )
	{
		super( testName );
	}

	/**
	 * @return the suite of tests being tested
	 */
	public static Test suite()
	{
		return new TestSuite( ZkStatsTest.class );
	}

	/**
	 * Rigourous Test :-)
	 */
	public void testApp()
	{
		assertTrue( true );
	}

	class StatsTest implements Watcher {

		ZooKeeper zk = null;
		Integer mutex;
		Schema schema;

		StatsTest(String address) {
			try {
				System.out.println("My CanonicalName is:" + InetAddress.getLocalHost().getCanonicalHostName().toString());

				if(zk == null ){
					try {
						System.out.println("Starting ZK:");
						zk = new ZooKeeper(address, 3000, this);
						mutex = new Integer(-1);
						System.out.println("Finished starting ZK: " + zk);
					} catch (IOException e) {
						System.out.println(e.toString());
						System.exit(1);
					}
				}
				//else mutex = new Integer(-1);

				Stat stat = zk.exists(Constants.DEFAULT_ZOOKEEPER_ZNODE_STATS,false);
				if(stat == null){
					System.out.println(Constants.DEFAULT_ZOOKEEPER_ZNODE_STATS + " does not exist");
					System.exit(1);
				}
				String schema = Bytes.toString(zk.getData(Constants.DEFAULT_ZOOKEEPER_ZNODE_STATS, false, stat));
				System.out.println("schema " + schema);
				Schema s = new Schema.Parser().parse(schema);
				DatumReader<GenericRecord> reader = new GenericDatumReader<GenericRecord>(s);

				List<String> servers = zk.getChildren(Constants.DEFAULT_ZOOKEEPER_ZNODE_STATS, false);
				for(String e: servers) {

					String path = Constants.DEFAULT_ZOOKEEPER_ZNODE_STATS + "/" + e;
					System.out.println("path " + path);
					byte[] b = zk.getData(path, false, stat);
					Decoder decoder = DecoderFactory.get().binaryDecoder(b, null);
					GenericRecord result = reader.read(null, decoder);
					System.out.println("nodename " + result.get("nodename").toString());
					System.out.println("cpubusy " + result.get("cpubusy").toString());
					System.out.println("memusage " + result.get("memusage").toString());
				}
			} catch (KeeperException e) {
				System.out.println("Keeper exception : " + e.toString());
			} catch (InterruptedException e) {
				System.out.println("Interrupted exception");
			} catch (UnknownHostException e) {
				System.out.println(e.toString());
			} catch (IOException io){
				System.out.println(io.toString());
			}
		}
		synchronized public void process(WatchedEvent event) {
			synchronized (mutex) {
				//System.out.println("Process: " + event.getType());
				mutex.notify();
			}
		}
		//For a producer in one shell try the following:
		//>wms org.trafodion.wms.teststats.StatsTest localhost:2183
		//
		//public static void main(String args[]) {
		//	StatsTest zks = new StatsTest(args[0]);
		//}
	}
}
