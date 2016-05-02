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
import common.*;

import java.sql.*;
import java.math.BigDecimal;

public class PreparedStatementSample
{
    public static void main(String args[])
    {

    Connection          connection;
    Statement           stmt;
    PreparedStatement   pStmt;
    ResultSet           rs;
    DatabaseMetaData    dbMeta;
    int                 rowNo;
    String              table = "PreparedStatementSample";

    try
    {
        connection = sampleUtils.getPropertiesConnection();
        sampleUtils.dropTable(connection, table);
        sampleUtils.initialData(connection, table);
        sampleUtils.initialCurrentData(connection, table);


        for (int i = 0; i < 7; i++)
        {
                switch (i)
                    {
            case 0:
                System.out.println("");
                System.out.println("Simple Select ");
                stmt = connection.createStatement();
                rs = stmt.executeQuery("select * from " + table);
                break;
            case 1:
                System.out.println("");
                System.out.println("Parameterized Select - CHAR");
                pStmt = connection.prepareStatement("select c1, c2 from " + table + " where c1 = ?");
                pStmt.setString(1, "Selva");
                rs = pStmt.executeQuery();
                break;
            case 2:
                System.out.println("");
                System.out.println("Parameterized Select - INT");
                pStmt = connection.prepareStatement("select c1, c2, c3 from " + table + " where c2 = ?  or c2 = ?");
                pStmt.setInt(1, 100);
                pStmt.setInt(2, -100);
                rs = pStmt.executeQuery();
                break;
            case 3:
                System.out.println("");
                System.out.println("Parameterized Select - TIMESTAMP");
                pStmt = connection.prepareStatement("select c1, c2, c3, c10 from " + table + " where c10 = ?");
                pStmt.setTimestamp(1, Timestamp.valueOf("2000-05-06 10:11:12.0"));
                rs = pStmt.executeQuery();
                break;
            case 4:
                System.out.println("");
                System.out.println("Parameterized Select - DECIMAL");
                pStmt = connection.prepareStatement("select c1, c2, c3, c7 from " + table + " where c7 = ? or c7 = ?");
                pStmt.setBigDecimal(1, new BigDecimal("100.12"));
                pStmt.setBigDecimal(2, new BigDecimal("-100.12"));
                rs = pStmt.executeQuery();
                break;
            case 5:
                System.out.println("");
                System.out.println("Parameterized Select - NUMERIC");
                pStmt = connection.prepareStatement("select c1, c2, c3, c6 from " + table + " where c6 = ? or c6 = ?");
                pStmt.setBigDecimal(1, new BigDecimal("100.12"));
                pStmt.setBigDecimal(2, new BigDecimal("-100.12"));
                rs = pStmt.executeQuery();
                break;
            case 6:
                System.out.println("");
                System.out.println("Parameterized Select - DATE");
                pStmt = connection.prepareStatement(
                   "select c11, c12 from " + table + " where c8 = ?");
                pStmt.setDate(1, Date.valueOf("2000-05-06"));
                rs = pStmt.executeQuery();
                break;
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

        sampleUtils.dropTable(connection, table);
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
