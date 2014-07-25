// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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
import common.*;

import java.sql.*;
import java.math.BigDecimal;

public class DBMetaSample
{
    public static void main(String args[])
    {

    Connection          connection;
    Statement           stmt;
    PreparedStatement   pStmt;
    ResultSet           rs;
    DatabaseMetaData    dbMeta;
    int                 rowNo;
    String              table = "DBMETASAMPLE";

    try
    {
        connection = sampleUtils.getPropertiesConnection();
        sampleUtils.dropTable(connection, table);
        sampleUtils.initialData(connection, table);
        sampleUtils.initialCurrentData(connection, table);


        System.out.println("");


        for (int i = 0; i < 6; i++)
        {
                switch (i)
                    {
            case 0:
                System.out.println("");
                System.out.println("getTypeInfo() ");
                dbMeta = connection.getMetaData();
                rs = dbMeta.getTypeInfo();
                break;
            case 2:
                System.out.println("");
                System.out.println("getCatalogs()");
                dbMeta = connection.getMetaData();
                rs = dbMeta.getCatalogs();
                break;
            case 3:
                System.out.println("");
                System.out.println("getTables() ");
                dbMeta = connection.getMetaData();
                rs = dbMeta.getTables(connection.getCatalog(), sampleUtils.props.getProperty("schema"), "DBMETASAMPLE", null);
                break;
            case 4:
                System.out.println("");
                System.out.println("getColumns()");
                dbMeta = connection.getMetaData();
                rs = dbMeta.getColumns(connection.getCatalog(), sampleUtils.props.getProperty("schema"), "DBMETASAMPLE", "C1");
                break;
            case 5:
                System.out.println("");
                System.out.println("getProcedures()");
                dbMeta = connection.getMetaData();
                rs = dbMeta.getProcedures(connection.getCatalog(), sampleUtils.props.getProperty("schema"), "Integer_Proc");
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
