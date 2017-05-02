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
// Filename : TestConnectionPool.java
//
// This program demonstrates the benefits of connection and statement pooling.
// You need to run CreateDataSource.java before running this program. Refer to
// CreateDataSource.java or README for instructions.
//
// Use the 'time' command to measure the performance benefits of connection
// and statement pooling in this program. This program can be run in different
// modes as follows:
//
// time java TestConnectionPool <dataSourceDir> <TestType>
//
// Where <dataSourceDir> is the directory containing the datasource to use.
//
// Where <TestType> can be ");
//  0 - Using DataSource - Connection and Statement pooling enabled
//  1 - Using DataSource - Connection enabled, but Statement pooling disabled
//  2 - Using DataSource - Both Connection and Statement pooling disabled
//
import java.sql.*;
import javax.sql.DataSource;
import javax.naming.*;
import java.math.BigDecimal;
import java.util.*;

public class TestConnectionPool
{
    public static void main(String args[])
    {

        Connection          connection = null;
        Statement           stmt = null;
        PreparedStatement   pStmt = null;
        ResultSet           rs = null;
        int                 rowNo;
        String              s;
        int			        error = 0;
        DataSource          ds;
        int                 TestType;

        if ( args.length != 2 )
        {
            System.out.println("TestConnectionPool: The dataSources directory path is required.");
            System.out.println("TestConnectionPool: The test type argument is also required.");
            System.out.println("Usage : java TestConnectionPool <dataSourceDir> <testType>");
            System.out.println("Where <dataSource> is the directory that contains the datasource to use");
            System.out.println("Where <TestType> can be ");
            System.out.println("  0 - Using DataSource - Connection and Statement pooling enabled");
            System.out.println("  1 - Using DataSource - Connection pooling enabled, but Statement pooling disabled");
            System.out.println("  2 - Using DataSource - Both Connection and Statement pooling disabled");
            return;
        }

        System.out.println("TestConnectionPool: Started");

        String RootDir = "file://" + args[0] + "/dataSources";
        Hashtable<String,String> env = new Hashtable<String,String>();

        env.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.fscontext.RefFSContextFactory");
        env.put(Context.PROVIDER_URL, RootDir);
        try
        {
            Context ctx = new InitialContext(env);
            TestType    = Integer.parseInt(args[1]);

            switch (TestType)
            {
                case 0:
                    // Connection using plain DataSource
                    // Statement pooling is enabled
                    ds = (DataSource)ctx.lookup("jdbc/TestDataSource");
                    ((org.apache.trafodion.jdbc.t2.SQLMXDataSource)ds).setMaxStatements(100);
                    ((org.apache.trafodion.jdbc.t2.SQLMXDataSource)ds).setMaxPoolSize(5);
                    ((org.apache.trafodion.jdbc.t2.SQLMXDataSource)ds).setMinPoolSize(5);
                    break;
                case 1:
                    // Connection using plain DataSource
                    // Statement pooling is disabled
                    ds = (DataSource)ctx.lookup("jdbc/TestDataSource");
                    ((org.apache.trafodion.jdbc.t2.SQLMXDataSource)ds).setMaxStatements(0);
                    ((org.apache.trafodion.jdbc.t2.SQLMXDataSource)ds).setMaxPoolSize(5);
                    ((org.apache.trafodion.jdbc.t2.SQLMXDataSource)ds).setMinPoolSize(5);
                    break;
                case 2:
                    // Connection using plain DataSource
                    // Statement pooling is disabled
                    ds = (DataSource)ctx.lookup("jdbc/TestDataSource");
                    ((org.apache.trafodion.jdbc.t2.SQLMXDataSource)ds).setMaxStatements(0);
                    ((org.apache.trafodion.jdbc.t2.SQLMXDataSource)ds).setMaxPoolSize(-1);
                    break;
                default:
                    System.out.println("Invalid test type.");
                    System.out.println("Usage : java TestConnectionPool <dataSourceDir> <testType>");
                    System.out.println("Where <dataSource> is the directory that contains the datasource to use");
                    System.out.println("Where <TestType> can be ");
                    System.out.println("  0 - Using DataSource - Connection and Statement pooling enabled");
                    System.out.println("  1 - Using DataSource - Connection enabled, but Statement pooling disabled");
                    System.out.println("  2 - Using DataSource - Both Connection and Statement pooling disabled");
                    return;
            }
        }
        catch (Exception ex)
        {
            ex.printStackTrace();
            return;
        }
        try
        {
            connection = ds.getConnection();
            stmt = connection.createStatement();
            try
            {
                stmt.executeUpdate("drop table tconpool");
            }
            catch (SQLException se1)
            {
            }
            stmt.executeUpdate("create table tconpool (c1 char(20), c2 smallint, c3 integer, c4 largeint, c5 varchar(120), c6 numeric(10,2), c7 decimal(10,2),c8 date, c9 time, c10 timestamp, c11 float, c12 double precision)");
            stmt.executeUpdate("insert into tconpool values('Moe', 100, 12345678, 123456789012, 'Moe', 100.12, 100.12, {d '2000-05-16'}, {t '10:11:12'}, {ts '2000-05-06 10:11:12.0'}, 100.12, 100.12)");
            stmt.executeUpdate("insert into tconpool values('Larry', -100, -12345678, -123456789012, 'Larry', -100.12, -100.12, {d '2000-05-16'}, {t '10:11:12'}, {ts '2000-05-06 10:11:12'}, -100.12, -100.12)");
            stmt.executeUpdate("insert into tconpool values('Curly', 100, -12345678, 123456789012, 'Curly', -100.12, 100.12, {d '2000-05-16'}, {t '10:11:12'}, {ts '2000-05-06 10:11:12.0'}, -100.12, 100.12)");
            connection.close();
        }
        catch (SQLException e)
        {
            SQLException nextException;
            error++;

            nextException = e;
            do
            {
                System.out.println(nextException.getMessage());
                System.out.println("SQLState   " + nextException.getSQLState());
                System.out.println("Error Code " + nextException.getErrorCode());

            } while ((nextException = nextException.getNextException()) != null);
            e.printStackTrace();
            try
            {
                if (connection != null)
                    connection.close();
            }
            catch (SQLException se1)
            {
            }
        }
        for (int k= 1; k < 100 ; k++)
        {
            try
            {
                connection = ds.getConnection();
                //System.out.println("***************** "+ k);
                for (int i = 0; i < 6 ; i++)
                {
                    switch (i)
                    {
                        case 0:
                            //System.out.println("");
                            //System.out.println("Simple Select ");
                            stmt = connection.createStatement();
                            rs = stmt.executeQuery("select * from tconpool");
                            break;
                        case 1:
                            pStmt = connection.prepareStatement("select c1, c2 from tconpool where c1 = ?");
                            pStmt.setString(1, "Moe");
                            rs = pStmt.executeQuery();
                            break;
                        case 2:
                            pStmt = connection.prepareStatement("select c1, c2, c3 from tconpool where c2 = ?  or c2 = ?");
                            pStmt.setInt(1, 100);
                            pStmt.setInt(2, -100);
                            rs = pStmt.executeQuery();
                            break;
                        case 3:
                            pStmt = connection.prepareStatement("select c1, c2, c3, c10 from tconpool where c10 = ?");
                            pStmt.setTimestamp(1, Timestamp.valueOf("2000-05-06 10:11:12.0"));
                            rs = pStmt.executeQuery();
                            break;
                        case 4:
                            pStmt = connection.prepareStatement("select c1, c2, c3, c7 from tconpool where c7 = ? or c7 = ?");
                            pStmt.setBigDecimal(1, new BigDecimal("100.12"));
                            pStmt.setBigDecimal(2, new BigDecimal("-100.12"));
                            rs = pStmt.executeQuery();
                            break;
                        case 5:
                            pStmt = connection.prepareStatement("select c1, c2, c3, c6 from tconpool where c6 = ? or c6 = ?");
                            pStmt.setBigDecimal(1, new BigDecimal("100.12"));
                            pStmt.setBigDecimal(2, new BigDecimal("-100.12"));
                            rs = pStmt.executeQuery();
                            break;
                        default:
                            rs = null;
                            continue;
                    }

                    ResultSetMetaData rsMD = rs.getMetaData();
                    for (int j = 1; j <= rsMD.getColumnCount(); j++)
                    {
                        s= rsMD.getColumnTypeName(j);
                    }
                    rowNo = 0;
                    while (rs.next())
                    {
                        rowNo++;
                        for (int j=1; j <= rsMD.getColumnCount(); j++)
                        {
                            s = rs.getString(j) ;
                        }
                    }
                    rs.close();
                }
                connection.close();
            }
            catch (SQLException e1)
            {
                SQLException nextException;
                error++;

                nextException = e1;
                do
                {
                    System.out.println(nextException.getMessage());
                    System.out.println("SQLState   " + nextException.getSQLState());
                    System.out.println("Error Code " + nextException.getErrorCode());

                } while ((nextException = nextException.getNextException()) != null);
                e1.printStackTrace();
                try
                {
                    if (rs != null)
                        rs.close();
                    if (stmt != null)
                        stmt.close();
                    if (pStmt != null)
                        pStmt.close();
                    if (connection != null)
                        connection.close();
                }
                catch (SQLException ex2)
                {
                    ex2.printStackTrace();
                }
            }
        }
        System.out.println("Number of times error has occurred : " + error);
        try
        {
            connection = ds.getConnection();
            stmt = connection.createStatement();
            stmt.executeUpdate("drop table tconpool");
        }
        catch (SQLException ex3)
        {
        }
    }
}

