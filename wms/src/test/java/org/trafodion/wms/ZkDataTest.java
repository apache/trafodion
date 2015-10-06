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
import java.util.Random;

import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.WatchedEvent;
import org.apache.zookeeper.Watcher;
import org.apache.zookeeper.ZooKeeper;
import org.apache.zookeeper.ZooDefs.Ids;
import org.apache.zookeeper.data.Stat;
import org.apache.avro.*;
import org.apache.avro.io.*;
import org.apache.avro.generic.*;
import org.junit.experimental.categories.*;

/**
 * Unit test for simple App.
 */
@Category(SmallTests.class)
public class ZkDataTest extends TestCase
{
	/**
	 * Create the test case
	 *
	 * @param testName name of the test case
	 */
	public ZkDataTest( String testName )
	{
		super( testName );
	}

	/**
	 * @return the suite of tests being tested
	 */
	public static Test suite()
	{
		return new TestSuite( ZkDataTest.class );
	}

	/**
	 * Rigourous Test :-)
	 */
	public void testApp()
	{
		//For a producer in one shell try the following:
		//>wms org.trafodion.wms.test.DataTest qTest localhost:2182 1 p
		//
		//For a consumer in another shell try the following:
		//>wms org.trafodion.wms.test.DataTest qTest localhost:2182 1 c
		/*
		Queue q = new Queue(args[1], "/app1");

		System.out.println("Input: " + args[1]);
		int i;
		Integer max = new Integer(args[2]);

		if (args[3].equals("p")) {
			System.out.println("Producer");
			for (i = 0; i < max; i++)
				try{
					q.produce(10 + i);
				} catch (KeeperException e){

				} catch (InterruptedException e){

				}
		} else {
			System.out.println("Consumer");
			for (i = 0; i < max; i++) {
				try{
					int r = q.consume();
					//System.out.println("Item: " + r);
				} catch (KeeperException e){
					i--;
				} catch (InterruptedException e){

				}
			}
		}
		*/
		assertTrue( true );
	}

	class DataTest implements Watcher {
		ZooKeeper zk = null;
		Integer mutex;
		//private static String SCHEMA_FILENAME = "/StringPair.avsc";
		String SCHEMA_FILENAME = "/workload.avsc";

		String root;
		Schema schema;


		DataTest(String address) {
			try {
				System.out.println("My CanonicalName is:" + InetAddress.getLocalHost().getCanonicalHostName().toString());
			} catch (UnknownHostException e) {
				System.out.println(e.toString());
			}

			if(zk == null ){
				try {
					System.out.println("Starting ZK:");
					zk = new ZooKeeper(address, 3000, this);
					mutex = new Integer(-1);
					System.out.println("Finished starting ZK: " + zk);
				} catch (IOException e) {
					System.out.println(e.toString());
					zk = null;
				}
			}
			//else mutex = new Integer(-1);

			try {
				schema = new Schema.Parser().parse(getClass().getResourceAsStream(SCHEMA_FILENAME));
				System.out.println("Loaded schema " + SCHEMA_FILENAME);
			} catch (IOException e) {
				e.printStackTrace();
				System.exit(-1);
			}
		}

		synchronized public void process(WatchedEvent event) {
			synchronized (mutex) {
				//System.out.println("Process: " + event.getType());
				mutex.notify();
			}
		}

		/**
		 * Producer-Consumer queue
		 */
		public class Queue extends DataTest {

			/**
			 * Constructor of producer-consumer queue
			 *
			 * @param address
			 * @param name
			 */
			Queue(String address, String name) {
				super(address);
				this.root = name;
				// Create ZK node name
				if (zk != null) {
					try {
						Stat s = zk.exists(root, false);
						if (s == null) {
							zk.create(root, new byte[0], Ids.OPEN_ACL_UNSAFE,
									CreateMode.PERSISTENT);
						}
					} catch (KeeperException e) {
						System.out
						.println("Keeper exception when instantiating queue: "
								+ e.toString());
					} catch (InterruptedException e) {
						System.out.println("Interrupted exception");
					}
				}
			}

			boolean produce(int i) throws KeeperException, InterruptedException{
				ByteArrayOutputStream out = new ByteArrayOutputStream();
				try {
					GenericRecord datum = new GenericData.Record(schema);
					datum.put("operation", 1);
					datum.put("jobId", "1234");
					datum.put("userName", "unify");
					datum.put("jobType", "HADOOP");
					datum.put("jobState","INIT");
					datum.put("jobSubState","UPDATE");
					datum.put("jobText","xyz");
					datum.put("workloadId","HWMS");
					datum.put("mapPct",100);
					datum.put("reducePct",10);
					datum.put("startTimestamp",System.currentTimeMillis());
					datum.put("endTimestamp",System.currentTimeMillis());
					datum.put("duration",System.currentTimeMillis());
					DatumWriter<GenericRecord> writer = new GenericDatumWriter<GenericRecord>(schema);
					Encoder encoder = EncoderFactory.get().binaryEncoder(out,null);
					writer.write(datum, encoder);
					encoder.flush();
					out.close();
				} catch (IOException e) {
					e.printStackTrace();
					System.exit(-1);
				}

				zk.create(root + "/element", out.toByteArray(), Ids.OPEN_ACL_UNSAFE,
						CreateMode.PERSISTENT_SEQUENTIAL);
				return true;
			}

			int consume() throws KeeperException, InterruptedException{
				int retvalue = -1;
				Stat stat = null;

				// Get the first element available
				while (true) {
					synchronized (mutex) {

						List<String> list = zk.getChildren(root, true);
						if (list.size() == 0) { 
							System.out.println("Going to wait"); 
							mutex.wait(); 
						} else {
							Integer min = new Integer(list.get(0).substring(7));
							int i=0 ,p=0;

							for(String s : list) { 
								Integer tempValue = new Integer(s.substring(7)); 
								if(tempValue < min) p=i; i++; 
							} 

							byte[] b = zk.getData(root + "/element" +
									list.get(p).substring(7), false, stat);
							zk.delete(root + "/element" +
									list.get(p).substring(7), 0);

							try {
								DatumReader<GenericRecord> reader = new GenericDatumReader<GenericRecord>(schema);
								Decoder decoder = DecoderFactory.get().binaryDecoder(b, null);
								GenericRecord result = reader.read(null, decoder);
								System.out.println("operation:" + result.get("operation").toString());
								System.out.println("jobId:" + result.get("jobId").toString());
								System.out.println("userName:" + result.get("userName").toString());
								System.out.println("jobType:" + result.get("jobType").toString());
								System.out.println("jobState:" + result.get("jobState").toString());
								System.out.println("jobSubState:" + result.get("jobSubState").toString());
								System.out.println("jobText:" + result.get("jobText").toString());
								System.out.println("mapPct:" + result.get("mapPct").toString());
								System.out.println("reducePct:" + result.get("reducePct").toString());
								System.out.println("startTimestamp:" + result.get("startTimestamp").toString());
								System.out.println("endTimestamp:" + result.get("endTimestamp").toString());
								System.out.println("duration:" + result.get("duration").toString());
							} catch (IOException e) {
								e.printStackTrace();
								System.exit(-1);
							}
							return retvalue;
						}
					}
				}
			}
		}
	}
}
