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
//
// The program creates the large object (LOB) base and data tables,
// inserts binary large objects (BLOB)/ character large objects (CLOB)
// data via the PreparedStatement interface, inserts an empty BLOB/CLOB
// row, then updates the empty BLOB/CLOB row with data using the Blob
// and Clob API interfaces.

import java.sql.*;
import java.io.*;

public class LobSample
{
 public static void main (String[] args)
	throws FileNotFoundException, IOException
 {
  int		Key=100;
  String	clobTestFile = "LobSample.java";
  String	blobTestFile = "LobSample.class";
  String	catalog=null;
  String	schema=null;
  String	blobDataTbl=null;
  String	clobDataTbl=null;
  Connection	conn1 = null;
  ResultSet	rs = null;
  PreparedStatement stmt1 = null;
  PreparedStatement stmt2 = null;
  PreparedStatement stmt3 = null;

  catalog = System.getProperty("jdbcmx.catalog");
  schema = System.getProperty("jdbcmx.schema");

  if ((catalog == null) || (schema == null) ) {
	System.out.println("");
	System.out.println("The LobSample demo requires a valid and existing user catalog and schema");
	System.out.println("for the creation of the BLOB and CLOB Trafodion data tables");
	System.out.println("");
	System.out.println("Ensure the -Djdbcmx.catalog and -Djdbcmx.schema properties are used.");
	System.out.println("");
	return;
  } else {
	blobDataTbl = catalog + "." + schema + ".blobdatatbl";
	clobDataTbl = catalog + "." + schema + ".clobdatatbl";
	System.out.println("LobSample using catalog=" + catalog + " schema=" + schema + " for demo tables...");
  }

  // Set jdbcmx.blobTableName and jdbcmx.clobTableName System properties.
  System.setProperty("jdbcmx.blobTableName", blobDataTbl);
  System.setProperty("jdbcmx.clobTableName", clobDataTbl);

  // files used as BLOB/CLOB data to insert
  FileInputStream clobFs = new FileInputStream(clobTestFile);
  int clobFsLen = clobFs.available();
  FileInputStream blobFs = new FileInputStream(blobTestFile);
  int blobFsLen = blobFs.available();

  // byte array for the blob update
  byte[] byteData1 = new byte[5000];
  for (int i=0; i<5000; i++) {
	byteData1[i] = (byte)i;
  }

  // String data for the clob update
  String  stringData = "Test CLOB Data ";
  for (int i=0; i<300; i++) {
	stringData = stringData + "Test CLOB Data ";
  }

  try {
	Class.forName("org.apache.trafodion.jdbc.t2.T2Driver");
  }
  catch (Exception e) {
	e.printStackTrace();
	System.out.println(e.getMessage());
	return;
  }

  try {
    conn1 = DriverManager.getConnection("jdbc:t2jdbc:");
    Statement stmt = conn1.createStatement();

    try {
	stmt.execute("drop table " + blobDataTbl);
	stmt.execute("drop table " + clobDataTbl);
	stmt.execute("drop table lobbase");
    }
    catch (SQLException e) {
    }

    System.out.println("Creating demo tables...");
    stmt.executeUpdate("create table lobbase (col1 int not null not droppable, col2 blob, col3 clob, primary key (col1))");
    stmt.executeUpdate("create table " + clobDataTbl + " (table_name char(128) not null not droppable, data_locator largeint not null not droppable, chunk_no int not null not droppable, lob_data varchar(3886), primary key(table_name, data_locator, chunk_no)) attributes extent(1024), maxextents 768");
    stmt.executeUpdate("create table " + blobDataTbl + " (table_name char(128) not null not droppable, data_locator largeint not null not droppable, chunk_no int not null not droppable, lob_data varchar(3886), primary key(table_name, data_locator, chunk_no)) attributes extent(1024), maxextents 768");

    conn1.setAutoCommit(false);

    System.out.println("PreparedStatement interface LOB insert...");
    String stmtSource1 = "insert into lobbase values (?,?,?)";
    stmt1 = conn1.prepareStatement(stmtSource1);
    stmt1.setInt(1,Key);
    stmt1.setBinaryStream(2,blobFs,blobFsLen);
    stmt1.setAsciiStream(3,clobFs,clobFsLen);
    stmt1.executeUpdate();
    conn1.commit();

    System.out.println("BLOB/CLOB interface LOB insert...");
    String stmtSource2 = "insert into lobbase values (?,EMPTY_BLOB(),EMPTY_CLOB())";
    stmt2 = conn1.prepareStatement(stmtSource2);
    stmt2.setInt(1,Key+1);
    stmt2.executeUpdate();

    Blob blob = null;
    Clob clob = null;

    System.out.println("Obtaining BLOB/CLOB data to update (EMPTY in this case)...");
    stmt3 = conn1.prepareStatement("select col2,col3 from lobbase where col1 = ? for update");
    stmt3.setInt(1,Key+1);
    rs = stmt3.executeQuery();

    if (rs.next()) {
	blob = rs.getBlob(1);
	clob = rs.getClob(2);
    } else {
	System.out.println("Empty ResultSet returned, exiting demo");
	return;
    }

    System.out.println("Writing data to previously empty BLOB...");
    OutputStream os1 = blob.setBinaryStream(1);
    os1.write(byteData1);
    os1.close();

    System.out.println("Writing data to previously empty CLOB...");
    OutputStream os2 = clob.setAsciiStream(1);
    byte[] byteData2 = stringData.getBytes();
    os2.write(byteData2);
    os2.close();
    conn1.commit();

    System.out.println("Removing demo tables...");
    stmt.executeUpdate("drop table " + blobDataTbl);
    stmt.executeUpdate("drop table " + clobDataTbl);
    stmt.executeUpdate("drop table lobbase");
    System.out.println("LOB Demo Complete");

  }
  catch (SQLException e) {
    e.printStackTrace();
    SQLException next = e;
    do {
        System.out.println("Messge : " + e.getMessage());
        System.out.println("Vendor Code : " + e.getErrorCode());
        System.out.println("SQLState : " + e.getSQLState());
    }  while ((next = next.getNextException()) != null);
  }
  finally {
    try {
        if(rs != null) rs.close();
        if(stmt1 != null) stmt1.close();
        if(stmt2 != null) stmt2.close();
        if(stmt3 != null) stmt3.close();
        if(conn1 != null) conn1.close();
     }
     catch (SQLException ex) {
        ex.printStackTrace();
     }
  }
 } // end main
}
