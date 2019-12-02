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

/**
 * 
 */
package org.trafodion.sql;

import java.io.IOException;
import java.io.OutputStream;

import org.apache.hadoop.conf.Configuration;
import org.trafodion.sql.TrafConfiguration;
import org.apache.hadoop.fs.FSDataInputStream;
import org.apache.hadoop.fs.FSDataOutputStream;
import org.apache.hadoop.fs.FileStatus;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.FileUtil;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.hbase.ZooKeeperConnectionException;
import org.apache.hadoop.hbase.util.Bytes;
import org.apache.hadoop.hbase.util.FSUtils;
import org.apache.hadoop.hbase.util.HFileArchiveUtil;
import org.apache.hadoop.io.IOUtils;
import org.apache.hadoop.io.SequenceFile;
import org.apache.hadoop.io.Writable;
import org.apache.hadoop.io.ByteWritable;
import org.apache.hadoop.io.BytesWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.io.compress.CodecPool;
import org.apache.hadoop.io.compress.CompressionCodec;
import org.apache.hadoop.io.compress.Compressor;
import org.apache.hadoop.io.compress.GzipCodec;
import org.apache.hadoop.io.SequenceFile.CompressionType;
import org.apache.hadoop.util.ReflectionUtils;
import org.apache.hadoop.io.compress.*;
import org.apache.hadoop.io.compress.zlib.*;
import org.apache.hadoop.fs.*;

import java.io.*;
import java.util.List;

import org.apache.hadoop.util.*;
import org.apache.hadoop.io.*;
import org.apache.log4j.Logger;

import com.google.common.collect.Lists;
import com.google.protobuf.ServiceException;

import org.apache.hadoop.hbase.TableName;
import org.apache.hadoop.fs.permission.AclEntry;
import org.apache.hadoop.fs.permission.FsPermission;
public class SequenceFileWriter {

    static Logger logger = Logger.getLogger(SequenceFileWriter.class.getName());
    static Configuration conf = null;           // File system configuration
    
    SequenceFile.Writer writer = null;

    FSDataOutputStream fsOut = null;
    OutputStream outStream = null;
    boolean sameStream = true;

    FileSystem  fs = null;
    /**
     * Class Constructor
     */
    static {
       conf = TrafConfiguration.create(TrafConfiguration.HDFS_CONF);
       System.setProperty("hostName", System.getenv("HOSTNAME"));
    }
    SequenceFileWriter() throws IOException
    {
    }
	
    public String open(String path) throws IOException {
        Path filename = new Path(path);
        writer = SequenceFile.createWriter(conf, 
          	       SequenceFile.Writer.file(filename),
          	       SequenceFile.Writer.keyClass(ByteWritable.class),
          	       SequenceFile.Writer.valueClass(BytesWritable.class),
          	       SequenceFile.Writer.compression(CompressionType.NONE));
        return null;
    }
	
    public String open(String path, int compressionType) throws IOException	{
        Path filename = new Path(path);
        
        CompressionType compType=null;
        switch (compressionType) {
          case 0:
            compType = CompressionType.NONE;
            break;
            
          case 1:
            compType = CompressionType.RECORD;
            break;
            
          case 2:
            compType = CompressionType.BLOCK;
            break;
          
          default:
            throw new IOException("Wrong argument for compression type.");
        }   
        writer = SequenceFile.createWriter(conf, 
          	                               SequenceFile.Writer.file(filename),
          	                               SequenceFile.Writer.keyClass(BytesWritable.class),
          	                               SequenceFile.Writer.valueClass(Text.class),
          	                               SequenceFile.Writer.compression(compType));
        return null;
    }
	
    public String write(String data) throws IOException {
        if (writer == null)
           throw new IOException("open() was not called first.");
			
	writer.append(new BytesWritable(), new Text(data.getBytes()));
        return null;
    }
	
    public String close() throws IOException {
        if (writer != null) {
           writer.close();
           writer = null;
        }
        return null;
    }
}
