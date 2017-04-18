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

import java.sql.*;
import java.math.BigDecimal;

public class T2Sample
{
    public static void main(String args[])
    {

        Connection          connection;
        Statement           stmt;
        PreparedStatement   pStmt;
        ResultSet           rs;
        DatabaseMetaData    dbMeta;
        int		    rowNo;

        try
        {
            Class.forName("org.apache.trafodion.jdbc.t2.T2Driver");
        }
        catch (Exception e)
        {
            e.printStackTrace();
            System.out.println(e.getMessage());
            return;
        }
        try
        {
            String url = "jdbc:t2jdbc:";
            connection = DriverManager.getConnection(url);
            stmt = connection.createStatement();
            System.out.println("Successfully connected");
            try
            {
                stmt.executeUpdate("drop table sample");
		System.out.println("Successfully droppped sample table");
            }
            catch (SQLException e)
            {
		e.printStackTrace();
            }
            stmt.executeUpdate("create table sample (c1 char(20), c2 smallint, c3 integer, c4 largeint, c5 varchar(120), c6 numeric(10,2), c7 decimal(10,2),c8 date, c9 time, c10 timestamp, c11 float, c12 double precision)");
	    System.out.println("Successfully created sample table");

            stmt.executeUpdate("insert into sample values('Moe', 100, 12345678, 123456789012, 'Moe', 100.12, 100.12, {d '2000-05-16'}, {t '10:11:12'}, {ts '2000-05-06 10:11:12.0'}, 100.12, 100.12)");
            stmt.executeUpdate("insert into sample values('Larry', -100, -12345678, -123456789012, 'Larry', -100.12, -100.12, {d '2000-05-16'}, {t '10:11:12'}, {ts '2000-05-06 10:11:12'}, -100.12, -100.12)");
            stmt.executeUpdate("insert into sample values('Curly', 100, -12345678, 123456789012, 'Curly', -100.12, 100.12, {d '2000-05-16'}, {t '10:11:12'}, {ts '2000-05-06 10:11:12'}, -100.12, 100.12)");
	    System.out.println("Successfully inserted values into sample table");

            for (int i = 0; i < 10 ; i++)
            {
                switch (i)
                {
                    case 0:
                        System.out.println("");
                        System.out.println("Simple Select ");
                        stmt = connection.createStatement();
                        rs = stmt.executeQuery("select * from sample");
                        break;
                    case 1:
                        System.out.println("");
                        System.out.println("Parameterized Select - CHAR");
                        pStmt = connection.prepareStatement("select c1, c2 from sample where c1 = ?");
                        pStmt.setString(1, "Moe");
                        rs = pStmt.executeQuery();
                        break;
                    case 2:
                        System.out.println("");
                        System.out.println("Parameterized Select - INT");
                        pStmt = connection.prepareStatement("select c1, c2, c3 from sample where c2 = ?  or c2 = ?");
                        pStmt.setInt(1, 100);
                        pStmt.setInt(2, -100);
                        rs = pStmt.executeQuery();
                        break;
                    case 3:
                        System.out.println("");
                        System.out.println("Parameterized Select - TIMESTAMP");
                        pStmt = connection.prepareStatement("select c1, c2, c3, c10 from sample where c10 = ?");
                        pStmt.setTimestamp(1, Timestamp.valueOf("2000-05-06 10:11:12.0"));
                        rs = pStmt.executeQuery();
                        break;
                    case 4:
                        System.out.println("");
                        System.out.println("Parameterized Select - DECIMAL");
                        pStmt = connection.prepareStatement("select c1, c2, c3, c7 from sample where c7 = ? or c7 = ?");
                        pStmt.setBigDecimal(1, new BigDecimal("100.12"));
                        pStmt.setBigDecimal(2, new BigDecimal("-100.12"));
                        rs = pStmt.executeQuery();
                        break;
                    case 5:
                        System.out.println("");
                        System.out.println("Parameterized Select - NUMERIC");
                        pStmt = connection.prepareStatement("select c1, c2, c3, c6 from sample where c6 = ? or c6 = ?");
                        pStmt.setBigDecimal(1, new BigDecimal("100.12"));
                        pStmt.setBigDecimal(2, new BigDecimal("-100.12"));
                        rs = pStmt.executeQuery();
                        break;
                      /*case 6:
                        System.out.println("");
                        System.out.println("Accessing Metadata");
                        System.out.println("getTypeInfo() ");
                            dbMeta = connection.getMetaData();
                            rs = dbMeta.getTypeInfo();
                        break;
                     case 7:
                        System.out.println("");
                        System.out.println("getCatalogs()");
                        dbMeta = connection.getMetaData();
                        rs = dbMeta.getCatalogs();
                        break;
                    case 8:
                        System.out.println("");
                        System.out.println("getTables() ");
                        dbMeta = connection.getMetaData();
                        rs = dbMeta.getTables(null, null, "SAM%", null);
                        break;
                    case 9:
                        System.out.println("");
                        System.out.println("getColumns()");
                        dbMeta = connection.getMetaData();
                        rs = dbMeta.getColumns(null, null, "SAMPLE", "%");
                        break;*/
                    default:
                        rs = null;
                        continue;
                }

                ResultSetMetaData rsMD = rs.getMetaData();
                System.out.println("");
                System.out.println("Printing ResultSetMetaData ...");
                System.out.println("No. of Columns " + rsMD.getColumnCount());
                for (int j = 1; j <= rsMD.getColumnCount(); j++)
                {
                    System.out.println("Column " + j + " Data Type: " + rsMD.getColumnTypeName(j) + " Name: " + rsMD.getColumnName(j));
                }
                System.out.println("");
                System.out.println("Fetching rows...");
                rowNo = 0;
                while (rs.next())
                {
                    rowNo++;
                    System.out.println("");
                    System.out.println("Printing Row " + rowNo + " using getString(), getObject()");
                    for (int j=1; j <= rsMD.getColumnCount(); j++)
                    {
                        System.out.println("Column " + j + " - " + rs.getString(j) + "," + rs.getObject(j));
                    }
                }
                System.out.println("");
                System.out.println("End of Data");
                rs.close();
            }
            stmt = connection.createStatement();
            stmt.executeUpdate("drop table sample");
            connection.close();
        }
        catch (SQLException e)
        {
            SQLException nextException;

            nextException = e;
            do
            {
                System.out.println(nextException.getMessage());
                System.out.println("SQLState   " + nextException.getSQLState());
                System.out.println("Error Code " + nextException.getErrorCode());
            } while ((nextException = nextException.getNextException()) != null);
        }
    }
}
