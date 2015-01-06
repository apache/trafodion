// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2014 Hewlett-Packard Development Company, L.P.
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
//
// Filename : TestDataSource.java
//
// This program demonstrates the use of javax.sql.DataSource interface.
// DataSource separates out the driver dependent information from the
// application.
//
// You need to run CreateDataSource java program before running this program.
// Refer to CreateDataSource.java program for instructions.
//
import javax.naming.*;
import java.sql.*;
import javax.sql.DataSource;
import java.util.*;

public class TestDataSource
{
    public static void main(String[] args)
    {
        if ( args.length == 0 )
        {
            System.out.println("TestDataSource: The dataSources directory path is required.");
            System.out.println("TestDataSource: If the dataSource is in current directory use:");
            System.out.println("TestDataSource:     java TestDataSource `pwd`");
            return;
        }

        System.out.println("TestDataSource: Started");

        String RootDir = "file://" + args[0] + "/dataSources";
        Hashtable<String,String> env = new Hashtable<String,String>();

        env.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.fscontext.RefFSContextFactory");
        env.put(Context.PROVIDER_URL, RootDir);
        try
        {
            Context ctx = new InitialContext(env);
            DataSource ds = (DataSource)ctx.lookup("jdbc/TestDataSource");
            System.out.println("TestDataSource: Name Lookup Completed");
            Connection con = null;
            Statement  stmt = null;
            ResultSet rs;
            ResultSetMetaData rsMetaData;

            try
            {
                con = ds.getConnection();
                stmt = con.createStatement();
                try
                {
                    stmt.executeUpdate("drop table tdata");     // Remove previous table if it exists
                }
                catch (SQLException e)
                {                        // This exception is used to clean up old table
                }
                stmt.executeUpdate("create table tdata (c1 char(10))");
                System.out.println("TestDataSource: Created Table tdata");
                stmt.executeUpdate("insert into tdata values ('Test1')");
                System.out.println("TestDataSource: Added data to table tdata");
                String sqlString = "select * from tdata";
                rs = stmt.executeQuery(sqlString);
                rsMetaData = rs.getMetaData();
                System.out.println("--Listing rows -- " + sqlString);
                System.out.println("");
                System.out.println(rsMetaData.getColumnName(1));
                System.out.println("-----");
                while (rs.next())
                {
                    System.out.println(rs.getString(1));

                }

            }
            catch (SQLException e)
            {
                System.out.println("TestDataSource: getConnection/createStatement: Exception Occurred");
                System.out.println("TestDataSource: getConnection/createStatement: " + e.getMessage());
                System.out.println("TestDataSource: getConnection/createStatement: " + e.getSQLState());
                System.out.println("TestDataSource: getConnection/createStatement: " + e.getErrorCode());
            }
            stmt.executeUpdate("drop table tdata");
            stmt.close();
            con.close();

        }
        catch ( Exception e)
        {
            System.out.println("TestDataSource: executeUpdate: Exception Occurred");
            System.out.println("TestDataSource: Message: " + e.getMessage());
            System.out.println("TestDataSource: Class: " + e.getClass());
        }
    }
}
