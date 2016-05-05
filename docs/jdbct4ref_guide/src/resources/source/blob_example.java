// @@@ START COPYRIGHT @@@
//  Licensed to the Apache Software Foundation (ASF) under one
//  or more contributor license agreements. See the NOTICE file
//  distributed with this work for additional information
//  regarding copyright ownership.  The ASF licenses this file
//  to you under the Apache License, Version 2.0 (the
//  "License"); you may not use this file except in compliance
//  with the License.  You may obtain a copy of the License at
// 
//      http://www.apache.org/licenses/LICENSE-2.0
// 
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//  @@@ END COPYRIGHT @@@
// 

// LOB operations may be performed through the Blob, or
// PreparedStatement interface. This program shows examples of
// using both interfaces taking a byte[] variable and putting
// it into the cat.sch.blobtiff table.
//
// The LOB base table for this example is created as:
//    >> CREATE TABLE blobtiff
//      ( col1 INT NOT NULL NOT DROPPABLE
//      , tiff BLOB
//      , PRIMARY KEY ( col1 )
//      ) ;
//
// The LOB table for this example is created through the
// T4LobAdmin utility as:
//    >> CREATE TABLE seabase.sch.blobdatatbl
//       ( table_name CHAR(128) NOT NULL NOT DROPPABLE
//       , data_locator LARGEINT NOT NULL NOT DROPPABLE
//       , chunk_no INT NOT NULL NOT DROPPABLE
//       , lob_data VARCHAR(3880)
//       , PRIMARY KEY ( table_name, data_locator, chunk_no )
//       ) ;
//
// ***** The following is the blob interface...
// - insert the base row with EMPTY_BLOB() as value for
// the LOB column
// - select the lob column 'for update'
// - load up a byte[] with the data
// - use Outputstream.write(byte[])
//
// ***** The following is the prep stmt interface...
// - need an Inputstream object that already has data
// - need a PreparedStatement object that contains the
// 'insert...' DML of the base table
// - ps.setAsciiStream() for the lob data
// - ps.executeupdate(); for the DML
//
// To run this example, issue the following:
// # java TestBLOB 1 TestBLOB.class 1000
//
import java.sql.* ;
import java.io.* ;

public class TestBLOB
{
  public static void main (String[] args) throws java.io.FileNotFoundException, java.io.IOException
  {
    int numBytes ;
    int recKey ;
    long start ;
    long end ;
    Connection conn1 = null ;

    // Set hpt4jdbc.blobTableName System Property. This property
    // can also be added to the command line through
    // "-Dhpt4jdbc.blobTableName=...", or a
    // java.util.Properties object can be used and passed to
    // getConnection.
    System.setProperty( "hpt4jdbc.blobTableName", "Seabase.sch.blobdatatbl" );

    if ( args.length < 2 )
    {
      System.out.println( "arg[0]=; arg[1]=file; arg[2]=") ;
      return ;
    }

    // byte array for the blob
    byte[] whatever = new byte[5000] ;
    for ( int i=0; i<5000; i++ ) whatever[i] = 71 ; // "G"

    String k = "K";
    for ( int i=0; i<5000; i++) k = k + "K" ;
    System.out.println( "string length = " + k.length() ) ;

    java.io.ByteArrayInputStream iXstream =
       new java.io.ByteArrayInputStream( whatever ) ;

    numBytes = iXstream.available( );
    if ( args.length == 3 )
       numBytes = Integer.parseInt( args[2] ) ;
    recKey = Integer.parseInt( args[0] ) ;
    System.out.println( "Key: "
                      + recKey
		      + "; Using "
		      + numBytes
		      + " of file "
                      + args[1]
		      ) ;

    try
    {
      Class.forName( "org.trafodion.t4jdbc.HPT4Driver" ) ;
      start = System.currentTimeMillis() ;

      // url should be of the form:
      //   jdbc:hpt4jdbc://ip_address|host_name:37800/:"
      // where host_name is the database host name
      String url = "jdbc:hpt4jdbc://host_name:37800/:" ;
      conn1 = DriverManager.getConnection( url ) ;

      System.out.println( "Cleaning up test tables..." ) ;
      Statement stmt0 = conn1.createStatement() ;
      stmt0.execute( "DELETE FROM blobdatatbl" ) ;
      stmt0.execute( "DELETE FROM blobtiff" ) ;
      conn1.setAutoCommit(false) ;
    }
    catch (Exception e1)
    {
       e1.printStackTrace();
    }

    // PreparedStatement interface example - This technique is
    // suitable if the LOB data is already on disk
    try
    {
      System.out.println( "PreparedStatement interface LOB insert..." ) ;

      String stmtSource1 = "INSERT INTO blobtiff VALUES ( ?, ? ) " ;
      PreparedStatement stmt1 = conn1.prepareStatement( stmtSource1 ) ;
      stmt1.setInt( 1, recKey ) ;
      stmt1.setBinaryStream( 2, iXstream, numBytes ) ;
      stmt1.executeUpdate() ;
      conn1.commit() ;
    }
    catch ( SQLException e )
    {
       e.printStackTrace() ;
       SQLException next = e ;

       do
       {
          System.out.println( "Messge : " + e.getMessage() ) ;
          System.out.println( "Error Code : " + e.getErrorCode() ) ;
          System.out.println( "SQLState : " + e.getSQLState() ) ;
       }
       while ( ( next = next.getNextException() ) != null ) ;
    }

    // Blob interface example - This technique is suitable when
    // the LOB data is already in the app, such as having been
    // transfered in a msgbuf.
    try
    {
      // insert a second base table row with empty LOB column
      System.out.println( "BLOB interface LOB insert..." ) ;
      String stmtSource2 = "INSERT INTO blobtiff VALUES ( ?, EMPTY_BLOB() ) " ;
      PreparedStatement stmt2 = conn1.prepareStatement( stmtSource2 ) ;
      stmt2.setInt( 1, recKey+1 ) ;
      stmt2.executeUpdate() ;
      Blob tiff = null ;

      System.out.println( "Obtaining BLOB data to update (EMPTY in this case)..." ) ;
      PreparedStatement stmt3 =
         conn1.prepareStatement( "SELECT tiff FROM blobtiff WHERE col1 = ? FOR UPDATE" ) ;
      stmt3.setInt( 1, recKey+1 ) ;
      ResultSet rs = stmt3.executeQuery() ;

      if ( rs.next() )
         // has to be there else the base table insert failed
         tiff = rs.getBlob( 1 ) ; 

      System.out.println( "Writing data to previously empty BLOB..." ) ;
      OutputStream os = tiff.setBinaryStream( 1 ) ;

      byte[] bData = k.getBytes() ;
      os.write( bData ) ;
      os.close() ;
      conn1.commit() ;
    }
    catch ( SQLException e )
    {
      e.printStackTrace() ;
      SQLException next = e ;

      do
      {
         System.out.println( "Messge : " + e.getMessage() ) ;
         System.out.println( "Vendor Code : " + e.getErrorCode() ) ;
         System.out.println( "SQLState : " + e.getSQLState() ) ;
      }
      while ( ( next = next.getNextException()) != null ) ;
    }
  } // main
} // class
