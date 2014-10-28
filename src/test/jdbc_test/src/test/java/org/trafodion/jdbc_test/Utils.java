/*
# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2013 Hewlett-Packard Development Company, L.P.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# @@@ END COPYRIGHT @@@
*/

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
import java.net.*;

public class Utils
{
    public static String url;
    public static String hpjdbc_version;
    public static String usr;
    public static String pwd;
    public static String catalog;
    public static String schema;
    public static Properties props;

    //------------------------------------------------------------------------
    static
    {
    	try
        {
            String propFile = System.getProperty("hpjdbc.properties");
            if (propFile != null)
            {
                FileInputStream fs = new FileInputStream(new File(propFile));
                props = new Properties();
                props.load(fs);

                url = props.getProperty("url");
                usr = props.getProperty("user");
                pwd = props.getProperty("pwd");
                catalog = props.getProperty("catalog");
                schema = props.getProperty("schema");
                hpjdbc_version=props.getProperty("hpjdbc_version");
            } else {
                System.out.println("Error: prop is not set. Exiting.");
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
            Class.forName(hpjdbc_version.trim());
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
            throw new SQLException ("Error: prop is null. Exiting.");
    }

  //------------------------------------------------------------------------
    public static Connection getUserConnection() throws SQLException
    {
       Connection connection = null;
       checkprops();

       connection = DriverManager.getConnection(url, usr, pwd);
       Statement stmt = connection.createStatement();
       stmt.execute("SET SCHEMA " + catalog + "." + schema);
       stmt.close();

       return connection;
     }

  //------------------------------------------------------------------------
     public static Connection getPropertiesConnection() throws SQLException
     {

        Connection connection = null;
        checkprops();

        connection = DriverManager.getConnection(url, props);

        return connection;
      }

  //------------------------------------------------------------------------
      public static Connection getUrlConnection() throws SQLException
      {
         Connection connection = null;
         checkprops();

         connection = DriverManager.getConnection(url);

         return connection;
       }

  //------------------------------------------------------------------------
      public static Connection getUrlConnection(String newUrl) throws SQLException
      {
         Connection connection = null;
         checkprops();

         connection = DriverManager.getConnection(newUrl);

         return connection;
       }
  
      public static void createTable(Connection conn, String table) throws SQLException 
      {
          Statement stmt = null;
          try {
          	stmt = conn.createStatement();
          	stmt.executeUpdate("create table "
          					+ table
          					+ " (TITLE CHAR(32) NO DEFAULT NOT NULL NOT DROPPABLE, ID INT NO DEFAULT NOT NULL NOT DROPPABLE, POINTS INT UNSIGNED NOT NULL, DEPT VARCHAR(32), PRIMARY KEY(ID)) NO PARTITION");
          	stmt.close();
          } 
          catch (SQLException e) {
          	e.printStackTrace();
          	System.out.println(e.getMessage());
          	System.out.println("ERROR: Fail to create " + table);
          	try {
          		stmt.close();
          	} catch (Exception ex) {
          	}
          	throw e;
          }
      }
      
      public static void dropTable(Connection conn, String table) 
      {
          Statement stmt = null;
          try {
          	stmt = conn.createStatement();
          	stmt.executeUpdate("drop table " + table + " cascade");
          } 
          catch (SQLException e) {
          	// Ignore the unable to drop error
          } 
          finally {
          	try {
          		stmt.close();
          	} 
          	catch (Exception ex) {
          	}
          }
      }

//------------------------------------------------------------------------
//       public static void main(String args[])
//       {
//       }
}
