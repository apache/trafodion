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

package org.trafodion.sql;

import java.io.IOException;
import java.io.FileNotFoundException;
import java.util.*;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;

import org.apache.hadoop.conf.Configuration;
import org.trafodion.sql.TrafConfiguration;
import org.apache.hadoop.fs.Path;

import org.apache.hadoop.hive.serde2.objectinspector.*;
import org.apache.hadoop.hive.ql.io.orc.*;

import org.apache.log4j.PropertyConfigurator;		 
import org.apache.log4j.Logger;

public class OrcFileReader
{

    static Logger logger = Logger.getLogger(OrcFileReader.class.getName());;
    static Configuration               m_conf;
    
    static {

	/* 
	   The following code takes care of initializing log4j for the org.trafodion.sql package 
	   (in case of an ESP, e.g.) when the class:org.trafodion.sql.HBaseClient (which initializes log4j 
           for the org.trafodion.sql package) hasn't been loaded.
	*/
    	String confFile = System.getProperty("trafodion.log4j.configFile");
    	if (confFile == null) {
    		System.setProperty("trafodion.sql.log", System.getenv("TRAF_LOG") + "/trafodion.sql.java.log");
    		confFile = System.getenv("TRAF_CONF") + "/log4j.sql.config";
    	}
    	PropertyConfigurator.configure(confFile);
	m_conf = TrafConfiguration.create(TrafConfiguration.HDFS_CONF);
    }
    
    Path                        m_file_path;
    
    Reader                      m_reader;
    List<OrcProto.Type>         m_types;
    StructObjectInspector       m_oi;
    List<? extends StructField> m_fields;
    RecordReader                m_rr;
    String                      lastError = null;
    Reader.Options		m_options;

public class OrcRowReturnSQL
{
		int m_row_length;
		int m_column_count;
		long m_row_number;
		byte[] m_row_ba = new byte[4096];
}

    OrcRowReturnSQL		rowData;	//TEMP!!


    OrcFileReader() {
	rowData = new OrcRowReturnSQL();	//TEMP: was in fetch
    }

//********************************************************************************

//  ORIGINAL VERSION BEFORE ADDING SUPPORT FOR COLUMN SELECTION
    public String open(String pv_file_name) throws IOException, FileNotFoundException {
//    pv_file_name= pv_file_name + "/000000_0";

	m_file_path = new Path(pv_file_name);

  	m_reader = OrcFile.createReader(m_file_path, OrcFile.readerOptions(m_conf));
	m_types = m_reader.getTypes();
	m_oi = (StructObjectInspector) m_reader.getObjectInspector();
	m_fields = m_oi.getAllStructFieldRefs();
        m_rr = m_reader.rows();
	return null;
    }

//********************************************************************************
/*
    public String open(String pv_file_name) throws IOException, FileNotFoundException {
//    pv_file_name= pv_file_name + "/000000_0";
	m_file_path = new Path(pv_file_name);
	m_reader = OrcFile.createReader(m_file_path, OrcFile.readerOptions(m_conf));
	m_types = m_reader.getTypes();
	m_oi = (StructObjectInspector) m_reader.getObjectInspector();
	m_fields = m_oi.getAllStructFieldRefs();
	
//	m_rr = m_reader.rows();		//RESTORE THIS as working code!
//						boolean[] includes = new boolean[29];
  						boolean[] includes = new boolean[] 					{true,true,false,false,false,false,false,false,false,false,false,false,
  											false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,true};
  						 m_options = new Reader.Options();
//  						my_options.include(includes);
//  						System.out.println("Array size: " + includes.length);
 					m_rr = m_reader.rowsOptions(m_options.include(includes));
// 					m_rr = m_reader.rowsOptions(m_options.include(new boolean[] {false,true,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false,false}));
//{true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true,true}));

	return null;
    }
*/
//********************************************************************************
    
	public String close()
	{
				m_reader = null;
				m_rr = null; 
				m_file_path = null;            
    return null;
	}


    public void printFileInfo() throws IOException {

	System.out.println("Reader: " + m_reader);


	System.out.println("# Rows: " + m_reader.getNumberOfRows());
	System.out.println("# Types in the file: " + m_types.size());
	for (int i=0; i < m_types.size(); i++) {
	    System.out.println("Type " + i + ": " + m_types.get(i).getKind());
	}

	System.out.println("Compression: " + m_reader.getCompression());
	if (m_reader.getCompression() != CompressionKind.NONE) {
	    System.out.println("Compression size: " + m_reader.getCompressionSize());
	}

	m_oi = (StructObjectInspector) m_reader.getObjectInspector();
	
	System.out.println("object inspector type category: " + m_oi.getCategory());
	System.out.println("object inspector type name    : " + m_oi.getTypeName());

	System.out.println("Number of columns in the table: " + m_fields.size());

	// Print the type info:
	for (int i = 0; i < m_fields.size(); i++) {
	    System.out.println("Column " + i + " name: " + m_fields.get(i).getFieldName());
	    ObjectInspector lv_foi = m_fields.get(i).getFieldObjectInspector();
	    System.out.println("Column " + i + " type category: " + lv_foi.getCategory());
	    System.out.println("Column " + i + " type name: " + lv_foi.getTypeName());
	}

    }

    public boolean seekToRow(long pv_rowNumber) throws IOException {

	if (m_reader == null) {
	    return false;
	}

	if ((pv_rowNumber < 0) ||
	    (pv_rowNumber >= m_reader.getNumberOfRows())) {
	    return false;
	}

	m_rr.seekToRow(pv_rowNumber);

	return true;
    }

    public String seeknSync(long pv_rowNumber) throws IOException {
	if (m_reader == null) {
	    return "Looks like a file has not been opened. Call open() first.";
	}

	if ((pv_rowNumber < 0) ||
	    (pv_rowNumber >= m_reader.getNumberOfRows())) {
	    return "Invalid rownumber: " + pv_rowNumber + " provided.";
	}

	m_rr.seekToRow(pv_rowNumber);

	return null;
    }

    public long getNumberOfRows() throws IOException {

	return m_reader.getNumberOfRows();

    }

    public long getPosition() throws IOException {

	return m_rr.getRowNumber();

    }

    // Dumps the content of the file. The columns are '|' separated.
    public void readFile_String() throws IOException {

	seeknSync(0);
	OrcStruct lv_row = null;
	Object lv_field_val = null;
   	StringBuilder lv_row_string = new StringBuilder(1024);
	while (m_rr.hasNext()) {
	    lv_row = (OrcStruct) m_rr.next(lv_row);
	    lv_row_string.setLength(0);
	    for (int i = 0; i < m_fields.size(); i++) {
		lv_field_val = m_oi.getStructFieldData(lv_row, m_fields.get(i));
		if (lv_field_val != null) {
		    lv_row_string.append(lv_field_val);
		}
		lv_row_string.append('|');
	    }
	    System.out.println(lv_row_string);
	}

    }


    // Dumps the contents of the file as ByteBuffer.
    public void readFile_ByteBuffer() throws IOException {

	OrcStruct lv_row = null;
	Object lv_field_val = null;
   	ByteBuffer lv_row_buffer;

	seeknSync(0);
	while (m_rr.hasNext()) {
	    byte[] lv_row_ba = new byte[4096];
	    lv_row_buffer = ByteBuffer.wrap(lv_row_ba);
	    lv_row = (OrcStruct) m_rr.next(lv_row);
	    for (int i = 0; i < m_fields.size(); i++) {
		lv_field_val = m_oi.getStructFieldData(lv_row, m_fields.get(i));
		if (lv_field_val == null) {
		    lv_row_buffer.putInt(0);
		    continue;
		}
		String lv_field_val_str = lv_field_val.toString();
		lv_row_buffer.putInt(lv_field_val_str.length());
		if (lv_field_val != null) {
		    lv_row_buffer.put(lv_field_val_str.getBytes());
		}
	    }
	    System.out.println(lv_row_buffer);
	    //	    System.out.println(new String(lv_row_buffer.array()));
	}
    }

    public String getNext_String(char pv_ColSeparator) throws IOException {

	if ( ! m_rr.hasNext()) {
	    return null;
	}

	OrcStruct lv_row = null;
	Object lv_field_val = null;
   	StringBuilder lv_row_string = new StringBuilder(1024);

	lv_row = (OrcStruct) m_rr.next(lv_row);
	for (int i = 0; i < m_fields.size(); i++) {
	    lv_field_val = m_oi.getStructFieldData(lv_row, m_fields.get(i));
	    if (lv_field_val != null) {
		lv_row_string.append(lv_field_val);
	    }
	    lv_row_string.append(pv_ColSeparator);
	}
	
	return lv_row_string.toString();
    }

    // returns the next row as a byte array
    public byte[] fetchNextRow() throws IOException {

	if ( ! m_rr.hasNext()) {
	    return null;
	}

//	OrcStruct lv_row = (OrcStruct) m_rr.next(null);
 OrcStruct lv_row = (OrcStruct) m_rr.next(null);
	Object lv_field_val = null;
   	ByteBuffer lv_row_buffer;

	byte[] lv_row_ba = new byte[4096];
	lv_row_buffer = ByteBuffer.wrap(lv_row_ba);
	for (int i = 0; i < m_fields.size(); i++) {
	    lv_field_val = m_oi.getStructFieldData(lv_row, m_fields.get(i));
	    if (lv_field_val == null) {
  		lv_row_buffer.putInt(0);
		continue;
	    }
	    String lv_field_val_str = lv_field_val.toString();
	    lv_row_buffer.putInt(lv_field_val_str.length());
	    if (lv_field_val != null) {
		lv_row_buffer.put(lv_field_val_str.getBytes());
	    }
	}
	return lv_row_buffer.array();
    }
    
    
//****************************************************************************
	
//THIS IS THE ORIGINAL FORM BEFORE ADDING SUPPORT FOR COLUMN SELECTION !!!!
public OrcRowReturnSQL fetchNextRowObj() throws IOException
{
//		int	lv_integerLength = Integer.Bytes;
		int	lv_integerLength = 4;
//		OrcRowReturnSQL rowData = new OrcRowReturnSQL();
	 
	 	if ( ! m_rr.hasNext()) {
	    return null;
	}

	OrcStruct lv_row = (OrcStruct) m_rr.next(null);
	Object lv_field_val = null;
   	ByteBuffer lv_row_buffer;

//	lv_row_buffer.order(ByteOrder.LITTLE_ENDIAN);
	lv_row_buffer = ByteBuffer.wrap(rowData.m_row_ba);
	lv_row_buffer.order(ByteOrder.LITTLE_ENDIAN);
	
	rowData.m_row_length = 0;
	rowData.m_column_count = m_fields.size();
	rowData.m_row_number = m_rr.getRowNumber();
	
	for (int i = 0; i < m_fields.size(); i++) {
	    lv_field_val = m_oi.getStructFieldData(lv_row, m_fields.get(i));
	    if (lv_field_val == null) {
  		lv_row_buffer.putInt(0);
  		rowData.m_row_length = rowData.m_row_length + lv_integerLength;
		continue;
	    }
	    String lv_field_val_str = lv_field_val.toString();
	    lv_row_buffer.putInt(lv_field_val_str.length());
  			rowData.m_row_length = rowData.m_row_length + lv_integerLength;
	    if (lv_field_val != null) {
		lv_row_buffer.put(lv_field_val_str.getBytes());
  		rowData.m_row_length = rowData.m_row_length + lv_field_val_str.length();
	    }
	}
    	 
	 return rowData;
	
}

//****************************************************************************
/*
public OrcRowReturnSQL fetchNextRowObj() throws IOException
{
//		int	lv_integerLength = Integer.Bytes;
		int	lv_integerLength = 4;
		boolean[]	lv_include;
		
		OrcRowReturnSQL rowData = new OrcRowReturnSQL();
	 
	 	if ( ! m_rr.hasNext()) {
	    return null;
	}

	OrcStruct lv_row = (OrcStruct) m_rr.next(null);
	Object lv_field_val = null;
   	ByteBuffer lv_row_buffer;

//	lv_row_buffer.order(ByteOrder.LITTLE_ENDIAN);
	lv_row_buffer = ByteBuffer.wrap(rowData.m_row_ba);
	lv_row_buffer.order(ByteOrder.LITTLE_ENDIAN);
//	rowData.m_column_count = m_fields.size();
	rowData.m_column_count = 0;;
	rowData.m_row_number = m_rr.getRowNumber();
	lv_include = m_options.getInclude();
	
	for (int i = 0; i < m_fields.size(); i++) {
					if (lv_include[i+1] == false) continue;
	    lv_field_val = m_oi.getStructFieldData(lv_row, m_fields.get(i));
	    if (lv_field_val == null) {
  				lv_row_buffer.putInt(0);
  				rowData.m_row_length = rowData.m_row_length + lv_integerLength;
						rowData.m_column_count++;;
						continue;
	    }
	    String lv_field_val_str = lv_field_val.toString();
	    lv_row_buffer.putInt(lv_field_val_str.length());
  			rowData.m_row_length = rowData.m_row_length + lv_integerLength;
	    if (lv_field_val != null) {
		lv_row_buffer.put(lv_field_val_str.getBytes());
  		rowData.m_row_length = rowData.m_row_length + lv_field_val_str.length();
				rowData.m_column_count++;;

	    }
	}
    	 
	 return rowData;
	
}
*/
//****************************************************************************
String getLastError() {
      return lastError;
  }

//****************************************************************************
public boolean isEOF() throws IOException
{ 
	if (m_rr.hasNext())
	{
	    return false;
	}
	else
			{
					return true;
			}
}  
//****************************************************************************
 public String fetchNextRow(char pv_ColSeparator) throws IOException {

	if ( ! m_rr.hasNext()) {
	    return null;
	}

	OrcStruct lv_row = null;
	Object lv_field_val = null;
   	StringBuilder lv_row_string = new StringBuilder(1024);

	lv_row = (OrcStruct) m_rr.next(lv_row);
	for (int i = 0; i < m_fields.size(); i++) {
	    lv_field_val = m_oi.getStructFieldData(lv_row, m_fields.get(i));
	    if (lv_field_val != null) {
		lv_row_string.append(lv_field_val);
	    }
	    lv_row_string.append(pv_ColSeparator);
	}
	
	return lv_row_string.toString();
    }
    


    public static void main(String[] args) throws IOException
    {
	System.out.println("OrcFile Reader main");

	OrcFileReader lv_this = new OrcFileReader();

	lv_this.open(args[0]);

	lv_this.printFileInfo();

	lv_this.readFile_String();

	lv_this.readFile_ByteBuffer();

	// Gets rows as byte[]  (starts at row# 4)
	boolean lv_done = false;
	if (lv_this.seeknSync(4) == null) {
	    while (! lv_done) {
		System.out.println("Next row #: " + lv_this.getPosition());
		byte[] lv_row_bb = lv_this.fetchNextRow();
		if (lv_row_bb != null) {
		    System.out.println("First 100 bytes of lv_row_bb: " + new String(lv_row_bb, 0, 100));
		    System.out.println("Length lv_row_bb: " + lv_row_bb.length);
		}
		else {
		    lv_done = true;
		}
	    }
	}

	// Gets rows as String (starts at row# 10)
	lv_done = false;
	String lv_row_string;
	if (lv_this.seeknSync(10) == null) {
	    while (! lv_done) {
		lv_row_string = lv_this.getNext_String('|');
		if (lv_row_string != null) {
		    System.out.println(lv_row_string);
		}
		else {
		    lv_done = true;
		}
	    }
	}
        System.out.println("Shows the change in place");
    }
}
