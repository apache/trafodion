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
// This program demonstrates the use of the jdbcmx.ISO88591 property.
//
import java.sql.*;

public class ISO88591Sample
{
    private static final String PROLOG = "ISO88591Sample: ";
    private static final String JDBCDRIVER = "org.apache.trafodion.jdbc.t2.T2Driver";
    private static final String URL = "jdbc:t2jdbc:";
    private static final String TBLNAME = "ISOTbl";

    private static final String Yen = ("”\");
    private static final String Overline = ("“`");
    private static final String Graveaccent = ("‹~");

    public static void main(String args[])
    {
	String sql  = null;
	String str2 = null;
	String str3 = null;
	String str4 = null;

	Connection		conn	= null;
	Statement		stmt	= null;
	PreparedStatement	pstmt	= null;
	ResultSet		rs	= null;
	int                     errCnt	= 0;

	try
	{
	    System.out.println(PROLOG + "Start");
	    Class.forName(JDBCDRIVER);

	    System.out.println(PROLOG + "Getting a connection");
	    conn = DriverManager.getConnection(URL);

	    System.out.println(PROLOG + "Creating a statement");
	    stmt = conn.createStatement();
	    try
	    {
		System.out.println(PROLOG + "Dropping table: " + TBLNAME);
		stmt.executeUpdate("drop table " + TBLNAME);
	    }
	    catch (SQLException e)
	    {
		// Do nothing
	    }

	    // Create table
	    sql = "create table " + TBLNAME +
		" (col1 int not null not droppable, " +
		"col2 varchar(8), " +
		"col3 varchar(8), " +
		"col4 varchar(8), " +
		"primary key (col1))";

	    System.out.println(PROLOG + "Create table: " + TBLNAME);
	    stmt.executeUpdate(sql);

	    // Insert Shift_JIS chars using PrepareStatement.setString()
	    sql = "insert into " + TBLNAME + " values (?,?,?,?)";
	    pstmt = conn.prepareStatement(sql);
	    pstmt.setInt(1, 150);
	    pstmt.setString(2, Yen);
	    pstmt.setString(3, Overline);
	    pstmt.setString(4, Graveaccent);

	    System.out.println(PROLOG + "Insert: Shift_JIS characters into table: " + TBLNAME);
	    pstmt.execute();
	    pstmt.close();

	    // Select Shift_JIS chars using ResultSet.getString()
	    sql = "select * from "  + TBLNAME + " where col1 = ?";
	    pstmt = conn.prepareStatement(sql);
	    pstmt.setInt(1, 150);

	    System.out.println(PROLOG + "Select: Shift_JIS characters from table: " + TBLNAME);
	    rs = pstmt.executeQuery();

	    if(rs.next())
	    {
		str2 = rs.getString(2);
		str3 = rs.getString(3);
		str4 = rs.getString(4);

		if ( !str2.equals(Yen) )
		{
		    System.out.println(PROLOG + "Insert/Select: Column 2 did not compare");
		    errCnt++;
		}
		if ( !str3.equals(Overline) )
		{
		    System.out.println(PROLOG + "Insert/Select: Column 3 did not compare");
		    errCnt++;
		}
		if ( !str4.equals(Graveaccent) )
		{
		    System.out.println(PROLOG + "Insert/Select: Column 4 did not compare");
		    errCnt++;
		}
	    }
	    else
	    {
		System.out.println(PROLOG + "Empty ResultSet returned, exiting demo");
		errCnt++;
	    }
	    // Display completion status indication
	    if(errCnt == 0)
		System.out.println(PROLOG + "Completed successfully");
	    else
		System.out.println(PROLOG + "Failed");
       }
       catch (SQLException sqle)
       {
           SQLException nextException;
	   nextException = sqle;
	   do
	   {
	       System.out.println(PROLOG + nextException.getMessage());
	       System.out.println(PROLOG + nextException.getSQLState());
	   } while ((nextException = nextException.getNextException()) != null);
           sqle.printStackTrace();
	   System.out.println(" ");
	   System.out.println(PROLOG + "Failed");
       }
       catch (Exception e)
       {
	   e.printStackTrace();
	   System.out.println(PROLOG + "Failed");
       }
       finally
       {
	   try
	   {
	       if(rs != null) rs.close();
	       if(pstmt != null) pstmt.close();
	       if(conn != null) conn.close();
	   }
	   catch (SQLException sqle)
	   {
	       sqle.printStackTrace();
	   }
       }
    }
} // End of ISO88591Sample
