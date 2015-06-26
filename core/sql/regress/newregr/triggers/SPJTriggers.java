// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2015 Hewlett-Packard Development Company, L.P.
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

import java.io.*;
import java.sql.*;

public class SPJTriggers
{

	// NO SQL: Write the sum of four integers into given file.
	public static void WriteSumOfFourIntsToFile(
			String outFileName, int val1, int val2, int val3, int val4)
        throws SQLException
	{
		int res = val1 + val2 + val3 + val4;

		try {
			FileOutputStream f = new FileOutputStream(outFileName);
			PrintWriter w = new PrintWriter(f);
			w.println(res);
			w.close();
			f.close();
		} catch (Exception e) {
			throw new SQLException("Could not write to file: " + outFileName,
					"S00001");
		}
	}

	// READS SQL DATA: select sum(?col) from ?tab into gievn file.
    public static void WriteSumOfOneColumnToFile(
            String outFileName, String colName, String tabName)
        throws SQLException
    {
		int res = -1;

        // get connection
        // execute direct
        String stmt = "SELECT SUM(" + colName + ") FROM " + tabName;
        Connection conn = DriverManager.getConnection("jdbc:default:connection");
        PreparedStatement sumQry = conn.prepareStatement(stmt);

        ResultSet rs = sumQry.executeQuery();
        rs.next();

		res = rs.getInt(1);

        rs.close();
        conn.close();

		try {
			FileOutputStream f = new FileOutputStream(outFileName);
			PrintWriter w = new PrintWriter(f);
			w.println(res);
			w.close();
			f.close();
		} catch (Exception e) {
			throw new SQLException("Could not write to file: " + outFileName,
					"S00001");
		}
	}

    // General purpose insert into table with four columns
    public static void InsertFourCoulmns(
            String tabName, int val1, int val2, int val3, int val4)
        throws SQLException
    {
        String stmt = "INSERT INTO " + tabName + " VALUES(" +
                        val1 + ", " + val2 + ", " +
                        val3 + ", " + val4 + ")";
        Connection conn = DriverManager.getConnection("jdbc:default:connection");
        PreparedStatement insStmt = conn.prepareStatement(stmt);

        insStmt.execute();
        conn.close();
    }



    // insert into destTab select sum(), ... from srcTab;
    public static void InsertSelectOfSumOfFourColumns(
            String srcTab, String destTab,
            String col1, String col2, String col3, String col4)
        throws SQLException
    {
        String stmt = "INSERT INTO " + destTab + " " +
                        "SELECT SUM(" + col1 + "), SUM(" + col2 +
                         "), SUM(" + col3 + "), SUM(" + col4 + ") " +
                        "FROM " + srcTab;

        Connection conn = DriverManager.getConnection("jdbc:default:connection");
        PreparedStatement insStmt = conn.prepareStatement(stmt);

        insStmt.execute();
        conn.close();
    }

}
