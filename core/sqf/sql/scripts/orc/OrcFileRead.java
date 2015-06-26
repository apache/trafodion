package org.apache.hadoop.hive.ql.io.orc;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintStream;
import java.util.*;
import java.nio.ByteBuffer;

import org.apache.hadoop.conf.Configuration;
import org.apache.hadoop.fs.FileSystem;
import org.apache.hadoop.fs.Path;

import org.apache.hadoop.hive.conf.*;
import org.apache.hadoop.hive.ql.io.sarg.SearchArgument;
import org.apache.hadoop.hive.serde2.objectinspector.*              ;

import org.apache.hadoop.io.IntWritable;
import org.apache.hadoop.io.Text;

import org.apache.hive.common.util.HiveTestUtils;

import static org.junit.Assert.assertEquals;
import org.junit.Before;
import org.junit.Test;
import static org.junit.Assert.assertNull;

public class OrcFileRead 
{

    Configuration m_conf;
    Path m_file_path;
    Reader m_reader;
    List<OrcProto.Type> m_types;
    StructObjectInspector m_oi;
    List<? extends StructField> m_fields;
    RecordReader m_rr;
    long m_totalNumberOfRows;

    OrcFileRead(String pv_file_name) {
	m_conf = new Configuration();
	m_file_path = new Path(pv_file_name);
    }

    // This method is just for experimentation.
    public void testRead() throws Exception {

	m_reader = OrcFile.createReader(m_file_path, OrcFile.readerOptions(m_conf));

	System.out.println("Reader: " + m_reader);

	System.out.println("# Rows: " + m_reader.getNumberOfRows());
	m_types = m_reader.getTypes();
	System.out.println("# Types in the file: " + m_types.size());

	for (int i=0; i < m_types.size(); i++) {
	    System.out.println("Type " + i + ": " + m_types.get(i).getKind());
	}

	System.out.println("Compression: " + m_reader.getCompression());
	if (m_reader.getCompression() != CompressionKind.NONE) {
	    System.out.println("Compression size: " + m_reader.getCompressionSize());
	}

	StructObjectInspector m_oi = (StructObjectInspector) m_reader.getObjectInspector();
	
	System.out.println("object inspector type category: " + m_oi.getCategory());
	System.out.println("object inspector type name    : " + m_oi.getTypeName());

	m_fields = m_oi.getAllStructFieldRefs();
	System.out.println("Number of columns in the table: " + m_fields.size());

	RecordReader m_rr = m_reader.rows();
	
	// Print the type info:
	for (int i = 0; i < m_fields.size(); i++) {
		System.out.println("Column " + i + " name: " + m_fields.get(i).getFieldName());
		ObjectInspector lv_foi = m_fields.get(i).getFieldObjectInspector();
		System.out.println("Column " + i + " type category: " + lv_foi.getCategory());
		System.out.println("Column " + i + " type name: " + lv_foi.getTypeName());
		//		Object lv_column_val = m_oi.getStructFieldData(lv_row, m_fields.get(i));
		//System.out.print("Column " + i + " value: " + lv_row.getFieldValue(i));
	    }

	OrcStruct lv_row = null;
	Object lv_field_val = null;
   	StringBuilder lv_row_string = new StringBuilder(1024);
	while (m_rr.hasNext()) {
	    lv_row = (OrcStruct) m_rr.next(lv_row);
	    lv_row_string.setLength(0);
	    for (int i = 0; i < m_fields.size(); i++) {
		lv_field_val = lv_row.getFieldValue(i);
		if (lv_field_val != null) {
		    lv_row_string.append(lv_field_val);
		}
		lv_row_string.append('|');
	    }
	    System.out.println(lv_row_string);
	}

	/** Typecasting to appropriate type based on the 'kind'
	if (OrcProto.Type.Kind.INT == m_types.get(1).getKind()) {
	    IntWritable lvf_1_val = (IntWritable) lv_row.getFieldValue(0);
	    System.out.println("Column 1 value: " + lvf_1_val);
	}
	**/

    }

    public int openFile() throws Exception {

	m_reader = OrcFile.createReader(m_file_path, OrcFile.readerOptions(m_conf));
	m_types = m_reader.getTypes();
	m_oi = (StructObjectInspector) m_reader.getObjectInspector();
	m_fields = m_oi.getAllStructFieldRefs();
	
	m_rr = m_reader.rows();

	return 0;
    }

    public void printFileInfo() throws Exception {

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
	if ((pv_rowNumber < 0) ||
	    (pv_rowNumber >= m_reader.getNumberOfRows())) {
	    return false;
	}

	m_rr.seekToRow(pv_rowNumber);

	return true;
    }

    // Dumps the content of the file. The columns are '|' separated.
    public void readFile_String() throws Exception {

	OrcStruct lv_row = null;
	Object lv_field_val = null;
   	StringBuilder lv_row_string = new StringBuilder(1024);
	while (m_rr.hasNext()) {
	    lv_row = (OrcStruct) m_rr.next(lv_row);
	    lv_row_string.setLength(0);
	    for (int i = 0; i < m_fields.size(); i++) {
		lv_field_val = lv_row.getFieldValue(i);
		if (lv_field_val != null) {
		    lv_row_string.append(lv_field_val);
		}
		lv_row_string.append('|');
	    }
	    System.out.println(lv_row_string);
	}

    }

    // Dumps the contents of the file as ByteBuffer.
    public void readFile_ByteBuffer() throws Exception {

	OrcStruct lv_row = null;
	Object lv_field_val = null;
   	ByteBuffer lv_row_buffer;
	while (m_rr.hasNext()) {
	    byte[] lv_row_ba = new byte[4096];
	    lv_row_buffer = ByteBuffer.wrap(lv_row_ba);
	    lv_row = (OrcStruct) m_rr.next(lv_row);
	    for (int i = 0; i < m_fields.size(); i++) {
		lv_field_val = lv_row.getFieldValue(i);
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

    public byte[] getNext() throws Exception {

	if ( ! m_rr.hasNext()) {
	    return null;
	}

	OrcStruct lv_row = (OrcStruct) m_rr.next(null);
	Object lv_field_val = null;
   	ByteBuffer lv_row_buffer;

	byte[] lv_row_ba = new byte[4096];
	lv_row_buffer = ByteBuffer.wrap(lv_row_ba);
	for (int i = 0; i < m_fields.size(); i++) {
	    lv_field_val = lv_row.getFieldValue(i);
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
	return lv_row_buffer.array();

    }

    public static void main(String[] args) throws Exception
    {
	System.out.println("OrcFile Reader");

	OrcFileRead lv_this = new OrcFileRead(args[0]);

	lv_this.openFile();

	lv_this.printFileInfo();

	lv_this.readFile_String();

	if (lv_this.seekToRow(4)) {
	    byte[] lv_row_bb = lv_this.getNext();
	    if (lv_row_bb != null) {
		System.out.println("First 100 bytes of lv_row_bb: " + new String(lv_row_bb, 0, 100));
		System.out.println("Length lv_row_bb: " + lv_row_bb.length);
	    }
	}

	//	lv_this.testRead();

    }
}
