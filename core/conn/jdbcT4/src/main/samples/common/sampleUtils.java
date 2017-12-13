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
package common;

import java.sql.Date;
import java.sql.Time;
import java.sql.Timestamp;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.PreparedStatement;
import java.sql.Connection;
import java.sql.DriverManager;
import java.util.*;
import java.io.*;
import java.util.logging.*;
import java.net.*;


public class sampleUtils
{
    static String url;
    static String usr;
    static String pwd;
    public static Properties props;

  //------------------------------------------------------------------------
    static
    {
        try
        {
            String propFile = System.getProperty("t4jdbc.properties");
            if (propFile != null)
            {
                FileInputStream fs = new FileInputStream(new File(propFile));
                props = new Properties();
                props.load(fs);

                url = props.getProperty("url");
                usr = props.getProperty("user");
                pwd = props.getProperty("password");
            } else {
                System.out.println("Error: t4jdbc.properties is not set. Exiting.");
                System.exit(0);
            }
        }
        catch (Exception e)
        {
            e.printStackTrace();
            System.out.println(e.getMessage());
        }

        try
        {
            Class.forName("org.trafodion.jdbc.t4.T4Driver");
            Logger.global.setLevel(Level.FINEST);
        } catch (Exception e) {
            e.printStackTrace();
            System.out.println(e.getMessage());
            System.exit(0);
        }
    }

  //------------------------------------------------------------------------
    static void checkprops() throws SQLException
    {
       if (props == null)
            throw new SQLException ("Error: t4jdbc.properties is null. Exiting.");
    }

  //------------------------------------------------------------------------
    public static Connection getUserConnection() throws SQLException
    {
       Connection connection = null;
       checkprops();

       Logger.global.log(Level.FINE,"DriverManager.getConnection(url, usr, pwd)");
       connection = DriverManager.getConnection(url, usr, pwd);
       Logger.global.log(Level.INFO, "DriverManager.getConnection(url, usr, pwd) passed");
       Logger.global.log(Level.FINE, "==============\n\n");

       return connection;
     }


  //------------------------------------------------------------------------
     public static Connection getPropertiesConnection() throws SQLException
     {

        Connection connection = null;
        checkprops();

        Logger.global.log(Level.FINE, "DriverManager.getConnection(url, props)");
        Logger.global.log(Level.FINEST, "Properties = " + props);
        connection = DriverManager.getConnection(url, props);
        Logger.global.log(Level.INFO, "DriverManager.getConnection(url, props) passed");
        Logger.global.log(Level.FINE, "==============\n\n");

        return connection;
      }

  //------------------------------------------------------------------------
      public static Connection getUrlConnection() throws SQLException
      {
         Connection connection = null;
         checkprops();

         Logger.global.log(Level.FINE, "DriverManager.getConnection(url)");
         connection = DriverManager.getConnection(url);
         Logger.global.log(Level.INFO, "DriverManager.getConnection(url) passed");
         Logger.global.log(Level.FINE, "==============\n\n");

         return connection;
       }

  //------------------------------------------------------------------------
      public static Connection getUrlConnection(String newUrl) throws SQLException
      {
         Connection connection = null;
         checkprops();

         Logger.global.log(Level.FINE, "DriverManager.getConnection(newUrl)  newUrl = " + newUrl);
         connection = DriverManager.getConnection(newUrl);
         Logger.global.log(Level.INFO, "DriverManager.getConnection(newUrl) passed  mewUrl = " + newUrl);
         Logger.global.log(Level.FINE, "==============\n\n");

         return connection;
       }

  //------------------------------------------------------------------------
       public static void main(String args[])
       {

         Connection          connection, connection1, connection2;

         try
         {
             connection = getUserConnection();
             connection1 = getPropertiesConnection();
             connection2 = getUrlConnection();

             Logger.global.log(Level.INFO, "testing valid setCatalog");
             connection.setCatalog("Velu");
             Logger.global.log(Level.INFO, "testing valid setCatalog done");

             Logger.global.log(Level.INFO, "testing invalid setTransactionIsolation");
             connection.setTransactionIsolation(Connection.TRANSACTION_READ_COMMITTED);
             Logger.global.log(Level.INFO, "testing invalid TransactionIsolation done");

             Logger.global.log(Level.FINE, "testing connection.close for (url)");
             connection.close();
             Logger.global.log(Level.INFO, "testing connection.close for (url) passed");
             Logger.global.log(Level.FINE, "==============\n\n");

             Logger.global.log(Level.FINE, "testing connection.close for (url, usr, pwd)");
             connection1.close();
             Logger.global.log(Level.INFO, "testing connection.close for (url) passed");
             Logger.global.log(Level.FINE, "==============\n\n");

             Logger.global.log(Level.FINE, "testing connection.close for (url, info)");
             connection2.close();
             Logger.global.log(Level.INFO, "testing connection.close for (url) passed");
             Logger.global.log(Level.FINE, "==============\n\n");

         }
         catch (Exception e)
         {
             e.printStackTrace();
         }
       }

       public static void dropTable(Connection conn, String table)
       {
           Statement stmt = null;

           try
           {
               stmt = conn.createStatement();
               stmt.executeUpdate("drop table " + table);
           }
           catch (SQLException e)
           {
               Logger.global.log(Level.FINE, "Drop table failed for = " + table);
               Logger.global.log(Level.FINE, "==============\n\n");
           } finally {
               try {
                   stmt.close();
               } catch (Exception ex) {}
           }
       }

       public static void initialData(Connection conn, String table) throws SQLException
       {
           Statement stmt = null;

           try
           {
               stmt = conn.createStatement();
               stmt.executeUpdate("create table " + table + " (c1 char(20), c2 smallint, c3 integer, c4 largeint, c5 varchar(120), c6 numeric(10,2), c7 decimal(10,2),c8 date, c9 time, c10 timestamp, c11 real, c12 double precision) NO PARTITION");

               stmt.executeUpdate("insert into " + table + " values('Row1', 100, 12345678, 123456789012, 'Selva', 100.12, 100.12, {d '2000-05-06'}, {t '10:11:12'}, {ts '2000-05-06 10:11:12.0'}, 100.12, 100.12)");

               stmt.executeUpdate("insert into " + table + " values('Row2', -100, -12345678, -123456789012, 'Selva', -100.12, -100.12, {d '2000-05-16'}, {t '10:11:12'}, {ts '2000-05-06 10:11:12'}, -100.12, -100.12)");
               stmt.close();

           }
           catch (SQLException e)
           {
               Logger.global.log(Level.FINE, "InitialData failed = " + e);
               Logger.global.log(Level.FINE, "==============\n\n");
               try {
                   stmt.close();
               } catch (Exception ex) {}
               throw e;
           }
       }

       public static void initialCurrentData(Connection conn, String table) throws SQLException
       {
           PreparedStatement pStmt = null;

           try
           {
               System.out.println("");
               System.out.println("Inserting TimeStamp ");
               pStmt = conn.prepareStatement(
                    "insert into " + table + " values('TimeStamp', -100, -12345678, -123456789012, 'Selva', -100.12, -100.12, ?, ?, ?, -100.12, -100.12)"
               );

               pStmt.setDate(1, new Date(new java.util.Date().getTime()));
               pStmt.setTime(2, new Time(new java.util.Date().getTime()));
               Timestamp t1 = new Timestamp( (new java.util.Date()).getTime());
               pStmt.setTimestamp(3, t1);
               if (pStmt.executeUpdate() != 1)
               {
                   System.out.println("executeUpdate of TimeStamp failed");
               }
               pStmt.close();

           }
           catch (SQLException e)
           {
               Logger.global.log(Level.FINE, "InitialCurrentData failed =" + e);
               Logger.global.log(Level.FINE, "==============\n\n");
               try {
                   pStmt.close();
               } catch (Exception ex) {}
               throw e;
           }
       }

	public static void initialTable(Connection conn, String table) throws SQLException {
		Statement stmt = null;
		try {
			stmt = conn.createStatement();
			stmt.executeUpdate("create table " + table + " (c1 int, c2 char(20), c3 int)");
			stmt.close();

		} catch (SQLException e) {
			Logger.global.log(Level.FINE, "InitialData failed = " + e);
			Logger.global.log(Level.FINE, "==============\n\n");
			try {
				stmt.close();
			} catch (Exception ex) {
			}
			throw e;
		}
	}
}
