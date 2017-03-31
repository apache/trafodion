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

public class HoldCursorSample
{
    public static void main(String args[])
    {

        Connection          connection;
        Statement           stmt;
        PreparedStatement   pStmt;
        DatabaseMetaData    dbMeta;
        int                 numRows=10;
        long                msgId=0;

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
            connection = DriverManager.getConnection("jdbc:t2jdbc:");
            DatabaseMetaData dbmeta;
            dbmeta = connection.getMetaData();

            System.out.println("HOLD_CURSORS_OVER_COMMIT = " + dbmeta.supportsResultSetHoldability(ResultSet.HOLD_CURSORS_OVER_COMMIT) );
            System.out.println("CLOSE_CURSORS_AT_COMMIT = " + dbmeta.supportsResultSetHoldability(ResultSet.CLOSE_CURSORS_AT_COMMIT) );
            stmt = connection.createStatement();

            try
            {
                stmt.executeUpdate("drop table holdJdbc");
            }
            catch (SQLException e)
            {
            }

            stmt.executeUpdate("create table holdJdbc (dest_id integer unsigned, msg_id largeint, msg_object varchar(70))");

            // Insert rows into table
            pStmt = connection.prepareStatement("insert into holdJdbc values ( ?, ?, ?)");
            pStmt.setInt(1,100);

            for (int i = 0; i < numRows ; i++)
            {
                msgId = System.currentTimeMillis();
                pStmt.setLong(2, msgId);
                pStmt.setString(3,"test object "+i);
                pStmt.execute();
            }
        }
        catch (SQLException sqle)
        {
            SQLException nextException;
            nextException = sqle;
            do
            {
                System.out.println(nextException.getMessage());
                System.out.println(nextException.getSQLState());
            } while ((nextException = nextException.getNextException()) != null);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
        Thread subThread;

        subThread = new subscribeThread();
        subThread.start();
    }

}

class subscribeThread extends Thread
{
    public void run()
    {
        Connection          connection;
        Statement           stmt;
        PreparedStatement   pStmt;
        ResultSet           rs;
        int         rowNo;
        boolean         ok=true;
        try
        {
            connection = DriverManager.getConnection("jdbc:t2jdbc:");
            stmt = connection.createStatement();
            stmt.executeUpdate("CONTROL QUERY DEFAULT STREAM_TIMEOUT '500'");

            System.out.println("");
            System.out.println("Wait for the stream timeout of 5 secs after end of data ");
            System.out.println("");
            connection.setAutoCommit(false);
            pStmt = connection.prepareStatement
            ("SELECT * FROM stream(holdJdbc) WHERE msg_id > ? "
            ,ResultSet.TYPE_FORWARD_ONLY
            ,ResultSet.CONCUR_UPDATABLE
            ,ResultSet.HOLD_CURSORS_OVER_COMMIT);

            pStmt.setLong(1, 0);
            rs = pStmt.executeQuery();

            System.out.println("Fetching rows...");
            rowNo = 0;

            while (ok)
            {
                try
                {
                    if (ok = rs.next())
                    {
                        rowNo++;
                        System.out.println("Row " + rowNo + ": " + rs.getString(1)+ ", " + rs.getString(2)+ ", " + rs.getString(3));
                        connection.commit();
                    }
                }
                catch (SQLException sqle)
                {
                    if (sqle.getErrorCode() == -8006)
                    {
                        System.out.println("Stream timeout ...");
                        break;
                    }
                    else
                    {
                        throw sqle;
                    }
                }
            } //end while

            System.out.println("");
            System.out.println("End of Data");
            connection.commit();

            rs.close();

            try
            {
                stmt.executeUpdate("drop table holdJdbc");
            }
            catch (SQLException e)
            {
                e.printStackTrace();
            }

            connection.close();
        }
        catch (SQLException sqle)
        {
            SQLException nextException;
            nextException = sqle;
            do
            {
                System.out.println(nextException.getMessage());
                System.out.println(nextException.getSQLState());
            } while ((nextException = nextException.getNextException()) != null);
        }
        catch (Exception e)
        {
            e.printStackTrace();
        }
    }
}
