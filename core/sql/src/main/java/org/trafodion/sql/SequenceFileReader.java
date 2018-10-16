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
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

import org.apache.log4j.PropertyConfigurator;
import org.apache.hadoop.conf.Configuration;
import org.trafodion.sql.TrafConfiguration;
import org.apache.hadoop.fs.Path;
import org.apache.hadoop.io.IOUtils;
import org.apache.hadoop.io.SequenceFile;
import org.apache.hadoop.io.Writable;
import org.apache.hadoop.util.ReflectionUtils;
//import org.apache.hadoop.hive.serde2.lazy.LazySimpleSerDe;
//import org.apache.hadoop.hive.serde2.objectinspector.ObjectInspector;
//import org.apache.hadoop.hive.serde2.objectinspector.ObjectInspectorUtils;
//import org.apache.hadoop.hive.serde2.objectinspector.ObjectInspectorUtils.ObjectInspectorCopyOption;
//import org.apache.hadoop.hive.serde2.objectinspector.StructField;
//import org.apache.hadoop.hive.serde2.objectinspector.StructObjectInspector;


public class SequenceFileReader {

  static Configuration conf = null;           // File system configuration
  SequenceFile.Reader reader = null;   // The HDFS SequenceFile reader object.
  Writable key = null;
  Writable row = null;
//    LazySimpleSerDe serde = null;
  boolean isEOF = false;
  String lastError = null;  
  static { 
    String confFile = System.getProperty("trafodion.log4j.configFile");
    if (confFile == null) {
   	System.setProperty("trafodion.sql.log", System.getenv("TRAF_LOG") + "/trafodion.sql.java.log");
    	confFile = System.getenv("TRAF_CONF") + "/log4j.sql.config";
    }
    PropertyConfigurator.configure(confFile);
    conf = TrafConfiguration.create(TrafConfiguration.HDFS_CONF);
  }
    
  /**
  * Class Constructor
  */
  SequenceFileReader() {
  }
    
  String getLastError() {
      return lastError;
  }
    
	/**
	 * Initialize the SerDe object. Needed only before calling fetchArrayOfColumns(). 
	 * @param numColumns The number of columns in the table.
	 * @param fieldDelim The delimiter between fields.
	 * @param columns A comma delimited list of column names. 
	 * @param colTypes A comma delimited list of column types.
	 * @param nullFormat NULL representation.
	 */
//	public void initSerDe(String numColumns, String fieldDelim, String columns, String colTypes, String nullFormat) throws IllegalStateException {
//		
//            serde = new LazySimpleSerDe();
//            Properties tbl = new Properties();
//            tbl.setProperty("serialization.format", numColumns);
//            tbl.setProperty("field.delim", fieldDelim);
//            tbl.setProperty("columns", columns);
//            tbl.setProperty("columns.types", colTypes);
//            tbl.setProperty("serialization.null.format", colTypes);
//            serde.initialize(conf, tbl);
//	}
	
	/**
	 * Open the SequenceFile for reading.
	 * @param path The HDFS path to the file.
	 */
	public String open(String path) throws IOException {

        Path filename = new Path(path);
        	
        reader = new SequenceFile.Reader(conf, SequenceFile.Reader.file(filename));
	
        key = (Writable) ReflectionUtils.newInstance(reader.getKeyClass(), conf);
        row = (Writable) ReflectionUtils.newInstance(reader.getValueClass(), conf);
        
        return null;
            
	}
	
	/**
	 * Get the current position in the file.
	 * @return The current position or -1 if error.
	 */
	public long getPosition() throws IOException {

    lastError = null;		
		if (reader == null) {
			lastError = "open() was not called first.";
			return -1;
		}
		
        return reader.getPosition();
	}	
	
    /**
     * Have we reached the end of the file yet?
     * @return
     */
  public boolean isEOF() {
		return isEOF;
	}

	/**
	 * Seek to the specified position in the file, and then to the beginning 
	 * of the record after the next sync mark.
	 * @param pos Required file position.
	 * @return null if OK, or error message.
	 */
	public String seeknSync(long pos) throws IOException {

		if (reader == null) {
			return "open() was not called first.";
		}
		
			reader.sync(pos);
			return null;
	}
	
	/**
	 * Fetch the next row as an array of columns.
	 * @return An array of columns.
	 */
//	public String[] fetchArrayOfColumns() throws IllegalStateException {
//		if (reader == null)
//			throw new IllegalStateException("open() was not called first.");
//		if (serde == null)
//			throw new IllegalStateException("initSerDe() was not called first.");
//		
//		ArrayList<String> result = new ArrayList<String>();
//            boolean theresMore = reader.next(key, row);
//            if (!theresMore)
//            	return null;
//            StructObjectInspector soi = (StructObjectInspector) serde.getObjectInspector();
//            List<? extends StructField> fieldRefs = soi.getAllStructFieldRefs();
//            Object data = serde.deserialize(row);
//            
//            for (StructField fieldRef : fieldRefs) {
//                ObjectInspector oi = fieldRef.getFieldObjectInspector();
//                Object obj = soi.getStructFieldData(data, fieldRef);
//                Object column = convertLazyToJava(obj, oi);
//                if (column == null)
//                	result.add(null);
//                else
//                	result.add(column.toString());
//              }
//    		String[] resultArray = new String[result.size()];
//    		result.toArray(resultArray);
//    		return resultArray;
//	}
	
	/**
	 * Fetch the next row as a single String, that still needs to be parsed.
	 * @return The next row.
	 */
	public String fetchNextRow() throws IOException {

    lastError = null;		
		if (reader == null) {		
			lastError = "open() was not called first.";
			return null;
		}
		
			boolean result = reader.next(key, row);
			if (result)	{
				return row.toString();
			}
			else {				
				return null;
			}
	}
	
	/**
	 * @param minSize Minimum size of the result. If the file is compressed, 
	 * the result may be much larger. The reading starts at the current 
	 * position in the file, and stops once the limit has been reached.
	 * @return An array of result rows.
	 * @throws IllegalStateException
	 */
	public String[] fetchArrayOfRows(int minSize) throws IOException {

    lastError = "";		
		if (reader == null) {		
			lastError = "open() was not called first.";
			return null;
		}
		
		ArrayList<String> result = new ArrayList<String>();
		long initialPos = getPosition();
		boolean stop = false;
		do {
			String newRow = fetchNextRow();
			
			if (newRow==null && lastError!=null)
			  return null;
			  
			boolean reachedEOF = (newRow == null || newRow == "");
			if (!reachedEOF)
				result.add(newRow);
			
			long bytesRead = getPosition() - initialPos;
			stop = reachedEOF || (bytesRead > minSize);
		} while (!stop);
		
		String[] resultArray = new String[result.size()];
		result.toArray(resultArray);
		return resultArray;
	}
	
	/**
	 * Read a block of data from the file and return it as an array of rows.
	 * First sync to startOffset, and skip the first row, then keep reading
	 * Until passing stopOffset and passing the next Sync marker.
	 * @param startOffset
	 * @param stopOffset
	 * @return
	 * @throws IllegalStateException
	 * @throws IOException
	 */
	public String[] fetchArrayOfRows(int startOffset, int stopOffset)
                  throws IOException  {

    lastError = "";		
		if (reader == null) {		
			lastError = "open() was not called first.";
			return null;
		}
		
		seeknSync(startOffset);
		
		ArrayList<String> result = new ArrayList<String>();
		boolean stop = false;
		do {
			long startingPosition = getPosition();
			String newRow = fetchNextRow();

			if (newRow==null && lastError!=null)
			  return null;
			  
			boolean reachedEOF = (newRow == null || newRow == "");
			
			boolean reachedSize = (startingPosition > stopOffset);
			boolean lastSyncSeen = (reachedSize && reader.syncSeen());
			// Stop reading if there is no more data, or if we have read 
			// enough bytes and have seen the Sync mark.
			stop = reachedEOF || (reachedSize && lastSyncSeen);
			
			if (!stop)
				result.add(newRow);
			
		} while (!stop);
		
		String[] resultArray = new String[result.size()];
		result.toArray(resultArray);
		return resultArray;
	}
	
	/**
	 * Fetch the next row from the file.
	 * @param stopOffset File offset at which to start looking for a sync marker
	 * @return The next row, or null if we have reached EOF or have passed stopOffset and then
	 *         the sync marker.
	 */
	public String fetchNextRow(long stopOffset) throws IOException {

    lastError = "";		
		if (reader == null) {		
			lastError = "open() was not called first.";
			return null;
		}

		long startingPosition = getPosition();
		
		String newRow = fetchNextRow();
		
    if (newRow==null && lastError!=null)
	    return null;

		if (newRow == null)
			isEOF = true;
		
		if (newRow == "")
			newRow = null;
		
		// If we have already read past the stopOffset on a previous row, 
		// and have seen the sync marker, then this row belongs to the next block.
		if ((startingPosition > stopOffset) && reader.syncSeen())
			newRow = null;
		
		return newRow;
	}
	
	/**
	 * Close the reader.
	 */
	public String close() {

    lastError = "";		
		if (reader == null) {		
			lastError = "open() was not called first.";
			return null;
		}

      IOUtils.closeStream(reader);            
    
    return null;
	}

	private boolean ReadnPrint(int start, int end) 
                       throws IOException {
		System.out.println("Beginning position: " + getPosition());
		String[] batch;
    batch = fetchArrayOfRows(start, end);
    if (batch==null)
      return false;
      
		boolean theresMore = (batch.length > 0);
		for (String newRow : batch)
			System.out.println(newRow);
		System.out.println("Ending position: " + getPosition());
		System.out.println("===> Buffer Split <===");
		return theresMore;
	}

	private boolean ReadnPrint2(int start, int end) throws IOException {
			System.out.println("Read from: " + start + " to: " + end + ".");
			seeknSync(start);
			System.out.println("Beginning position: " + getPosition());
			String newRow = null;
			do {
				newRow = fetchNextRow(end);
				
				if (newRow != null)
					System.out.println(newRow);
			} while (newRow != null); 
			
		System.out.println("Ending position: " + getPosition());
		System.out.println("===> Buffer Split <===");
		return !isEOF();
	}

	/**
	 * @param args
	 * @throws IOException 
	 */
	public static void main(String[] args) throws IOException {
		
		SequenceFileReader sfReader = new SequenceFileReader();
		byte[] fieldDelim = new byte[2];
		fieldDelim[0] = 1;
		fieldDelim[1] = 0;
		//sfReader.initSerDe("19", "\01",
                //           "p_promo_sk,p_promo_id,p_start_date_sk,p_end_date_sk,p_item_sk,p_cost,p_response_target,p_promo_name,p_channel_dmail,p_channel_email,p_channel_catalog,p_channel_tv,p_channel_radio,p_channel_press,p_channel_event,p_channel_demo,p_channel_details,p_purpose,p_discount_active",
                //           "int,string,int,int,int,float,int,string,string,string,string,string,string,string,string,string,string,string,string",
                //          "NULL");
                          
		//sfReader.open("hdfs://localhost:9000/user/hive/warehouse/promotion_seq/000000_0");
		sfReader.seeknSync(300);

		int opType = 4;
		switch (opType)
		{
//		case 1:
//			boolean theresMoreRows = true;
//			do {
//				String[] columns = sfReader.fetchArrayOfColumns();
//				theresMoreRows = (columns != null);
//				if (theresMoreRows)
//				{
//					for (String col : columns)
//					{
//						if (col == null)
//							System.out.print("<NULL>, ");
//						else
//							System.out.print(col + ", ");
//					}
//					System.out.println();
//				}
//			} while (theresMoreRows); 
//			break;
			
		case 2: // Return row as String
			String row;
			do {
				row = sfReader.fetchNextRow();
				if (row != null)
					System.out.println(row);
			} while (row != null);
			break;
			
		case 3:
		case 4:
			int size = 3000;
			int start = 0;
			int end = size;
			boolean theresMore3 = true;
			
			while (theresMore3) {
				if (opType == 3)
					theresMore3 = sfReader.ReadnPrint(start, end);
				else
					theresMore3 = sfReader.ReadnPrint2(start, end);
				start += size;
				end += size;				
			}
			break;

		}
		
		sfReader.close();
	}

//	private static Object convertLazyToJava(Object o, ObjectInspector oi) {
//	    Object obj = ObjectInspectorUtils.copyToStandardObject(o, oi, ObjectInspectorCopyOption.JAVA);
//
//	    // for now, expose non-primitive as a string
//	    // TODO: expose non-primitive as a structured object while maintaining JDBC compliance
//	    if (obj != null && oi.getCategory() != ObjectInspector.Category.PRIMITIVE) {
//	      obj = obj.toString();
//	    }
//
//	    return obj;
//	  }
}
