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

import java.io.RandomAccessFile;
import java.io.ByteArrayOutputStream;
import java.nio.MappedByteBuffer;
import java.nio.channels.FileChannel;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.avro.*;
import org.apache.avro.io.*;
import org.apache.avro.generic.*;
import org.junit.experimental.categories.*;

/**
 * Unit test for simple App.
 */
@Category(MediumTests.class)
public class MemoryMapTest 
    extends TestCase
{
	static  final Log LOG = LogFactory.getLog(MemoryMapTest.class);
	static int count = 10485760 ; //10 MB	
	static String SCHEMA_FILENAME = "/workload.avsc";
	Schema schema;
	RandomAccessFile memoryMappedFile;
	MappedByteBuffer out;
	
    /**
     * Create the test case
     *
     * @param testName name of the test case
     */
    public MemoryMapTest( String testName )
    {
        super( testName );
    }

    /**
     * @return the suite of tests being tested
     */
    public static Test suite()
    {
        return new TestSuite( MemoryMapTest.class );
    }

    /**
     * Rigourous Test :-)
     */
    public void testApp()
    {
/*
		try {
			init();
			long startTime = System.currentTimeMillis();
			write();
			LOG.info("Total write time [" + (System.currentTimeMillis() - startTime) + "] millis");
			read();
			LOG.info("Total read time [" + (System.currentTimeMillis() - startTime) + "] millis");
		} catch (Exception e) {
			LOG.error(e);
			e.printStackTrace();			
		}
*/
        assertTrue( true );
    }
    
	void init() throws Exception {
		schema = new Schema.Parser().parse(getClass().getResourceAsStream(SCHEMA_FILENAME));
		LOG.info("Loaded schema " + SCHEMA_FILENAME);
		LOG.info("Creating map file");
		memoryMappedFile = new RandomAccessFile("wmsmemmap.txt", "rw");                
		LOG.info("Mapping a file into memory");      
		out = memoryMappedFile.getChannel().map(FileChannel.MapMode.READ_WRITE, 0, count);               

	}
	
	void write() throws Exception {
		/*
		for (int i = 0; i < count; i++) {            
			out.put((byte) 'A');        
		}
		LOG.info(count + " bytes written to Memory Mapped File");  
		*/

		ByteArrayOutputStream baos = new ByteArrayOutputStream();
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
		Encoder encoder = EncoderFactory.get().binaryEncoder(baos,null);
		writer.write(datum, encoder);
		encoder.flush();
		baos.close();
		out.put(baos.toByteArray());
		LOG.info(baos.size() + " bytes written to Memory Mapped File");  

	}
	
	void read() throws Exception {
		/*
		for (int i = 0; i < count ; i++) {            
			out.get(i);        
		} 
		LOG.info(count + " bytes read from Memory Mapped File"); 
		*/
		/*
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
		*/
	}
}
