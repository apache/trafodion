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

public class StatementSample
{
    public static void main(String args[])
    {

    Connection          connection;
    Statement           stmt;
    PreparedStatement   pStmt;
    ResultSet           rs;
    DatabaseMetaData    dbMeta;
    int            rowNo;
    String              table = "StatementSample";

    try
    {
        connection = sampleUtils.getPropertiesConnection();
        sampleUtils.dropTable(connection, table);
        sampleUtils.initialData(connection, table);
        sampleUtils.initialCurrentData(connection, table);

        for (int i = 0; i < 1; i++)
        {
            switch (i)
            {
            case 0:
                System.out.println("");
                System.out.println("Simple Select ");
                stmt = connection.createStatement();
                rs = stmt.executeQuery("select * from " + table);
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
