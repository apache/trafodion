// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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
// @@@ END COPYRIGHT @@@

/**
 * 
 */
package org.trafodion.sql.HBaseAccess;

import java.io.IOException;
import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.SequenceFile;
import org.apache.hadoop.io.Writable;
import org.apache.hadoop.io.ByteWritable;
import org.apache.hadoop.io.BytesWritable;
import org.apache.hadoop.io.Text;
import org.apache.hadoop.io.compress.CompressionCodec;
import org.apache.hadoop.io.SequenceFile.CompressionType;



public class SequenceFileWriter {

    Configuration conf = null;           // File system configuration
    SequenceFile.Writer writer = null;

    /**
     * Class Constructor
     */
    SequenceFileWriter() {
      conf = new Configuration();
      conf.set("fs.hdfs.impl","org.apache.hadoop.hdfs.DistributedFileSystem");
    }
    
	
    public String open(String path)	{
      try {
        Path filename = new Path(path);
        writer = SequenceFile.createWriter(conf, 
          	       SequenceFile.Writer.file(filename),
          	       SequenceFile.Writer.keyClass(ByteWritable.class),
          	       SequenceFile.Writer.valueClass(BytesWritable.class),
          	       SequenceFile.Writer.compression(CompressionType.NONE));
        return null;
      } catch (Exception e) {
        //e.printStackTrace();
        return e.getMessage();
      }	
    }
	
    public String open(String path, int compressionType)	{
      try {
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
            return "Wrong argument for compression type.";
        }
        
        writer = SequenceFile.createWriter(conf, 
          	                               SequenceFile.Writer.file(filename),
          	                               SequenceFile.Writer.keyClass(BytesWritable.class),
          	                               SequenceFile.Writer.valueClass(Text.class),
          	                               SequenceFile.Writer.compression(compType));
        return null;
      } catch (Exception e) {
        //e.printStackTrace();
        return e.getMessage();
      }	
    }
	
    public String write(String data) {
		  if (writer == null)
			  return "open() was not called first.";
			
      try {
	      writer.append(new BytesWritable(), new Text(data.getBytes()));
        return null;
    	} catch (IOException e) {
    	  //e.printStackTrace();
        return e.getMessage();
    	}
    }
	
    public String close() {
		  if (writer == null)
			  return "open() was not called first.";
			
      try {
        writer.close();
        return null;
      } catch (Exception e) {
        //e.printStackTrace();
        return e.getMessage();
      }
    }

}
