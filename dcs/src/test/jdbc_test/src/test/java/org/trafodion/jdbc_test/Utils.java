/*
# @@@ START COPYRIGHT @@@   
#   
# (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.   
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
//import java.util.logging.*;
import java.net.*;

public class Utils
{
	public static String url;
    public static String trafjdbc_version;
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
            String propFile = System.getProperty("trafjdbc.properties");
            if (propFile != null)
            {
                FileInputStream fs = new FileInputStream(new File(propFile));
                props = new Properties();
                props.load(fs);

                url = props.getProperty("url");
                System.out.println("url: " + url);
                usr = props.getProperty("user");
                System.out.println("usr: " + usr);
                pwd = props.getProperty("password");
                System.out.println("password: " + pwd);
                catalog = props.getProperty("catalog");
                System.out.println("catalog: " + catalog);
                schema = props.getProperty("schema");
                System.out.println("schema: " + schema);
                trafjdbc_version=props.getProperty("trafjdbc_version");
                System.out.println("trafjdbc_version : " + trafjdbc_version);
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
        	Class.forName(trafjdbc_version.trim());
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
	try{
       	stmt.execute("CREATE SCHEMA " + catalog + "." + schema);
	}
	catch (SQLException e){
		//ignore
	}
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
       Statement stmt = connection.createStatement();
	try{
       	stmt.execute("CREATE SCHEMA " + catalog + "." + schema);
	}
	catch (SQLException e){
		//ignore
	}
       stmt.execute("SET SCHEMA " + catalog + "." + schema);
        return connection;
      }

  //------------------------------------------------------------------------
      public static Connection getUrlConnection() throws SQLException
      {
         Connection connection = null;
         checkprops();
         String newurl = "jdbc:t4jdbc:zz/zz@//abc.xyz.com:37800/:";
         connection = DriverManager.getConnection(newurl);
       Statement stmt = connection.createStatement();
	try{
       	stmt.execute("CREATE SCHEMA " + catalog + "." + schema);
	}
	catch (SQLException e){
		//ignore
	}
       stmt.execute("SET SCHEMA " + catalog + "." + schema);
         return connection;
       }

  //------------------------------------------------------------------------
      public static Connection getUrlConnection(String newUrl) throws SQLException
      {
         Connection connection = null;
         checkprops();

         connection = DriverManager.getConnection(newUrl);
       Statement stmt = connection.createStatement();
	try{
       	stmt.execute("CREATE SCHEMA " + catalog + "." + schema);
	}
	catch (SQLException e){
		//ignore
	}
       stmt.execute("SET SCHEMA " + catalog + "." + schema);

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
 
      public static void createTable12(Connection conn, String table) throws SQLException {
          Statement stmt = null;

          try {
          	stmt = conn.createStatement();
          	stmt.executeUpdate("create table " + table
          			+ "(seqno	integer		not null	not droppable,"
          			+ "smin1	smallint			default null,"
          			+ "smin2	smallint unsigned		default null, "
          			+ "inte1	integer				default null, "
          			+ "inte2	integer unsigned		default null, "
          			+ "lint1	largeint			default null, "
          			+ "nume1	numeric(4,2)			default null, "
          			+ "nume2	numeric(4,2) unsigned		default null, "
          			+ "nume3	numeric(9,3)			default null, "
          			+ "nume4	numeric(9,3) unsigned		default null, "
          			+ "deci1	decimal(18,9)			default null, "
          			+ "deci2	decimal(9,4) unsigned			default null, "
          			+ "pict1	pic s9(13)v9(5)			default null, "
          			+ "pict2	pic s9(13)v9(5) comp		default null, "
          			+ "flot1	float (36)			default null, "
          			+ "real1	real				default null, "
          			+ "dblp1	double precision		default null, "
          			+ "char1	char (12)			default null, "
          			+ "char2	char (12)			character set ucs2 default null, "
          			+ "vchr1	varchar (12)			default null, "
          			+ "vchr2	varchar (12)			character set ucs2 default null, "
          			+ "vchr3	varchar (32000) 		default null, "
          			+ "date1	date				default null, "
          			+ "time1	time				default null, "
          			+ "time2	time(5)				default null, "
          			+ "tims1	timestamp			default null, "
          			+ "tims2	timestamp(3)			default null, "
          			+ "intv00  interval year                   default null, "
          			+ "intv01  interval year (5) to year      default null, "
          			+ "intv02  interval year (6) to month     default null, "
          			+ "intv03  interval month                  default null, "
          			+ "intv04  interval month (5) to month    default null, "
          			+ "intv05  interval day                    default null, "
          			+ "intv06  interval day (5) to day        default null, "
          			+ "intv07  interval day (13) to hour       default null, "
          			+ "intv08  interval day (12) to minute     default null, "
          			+ "intv09  interval day (7) to second      default null, "
          			+ "intv10  interval hour                   default null, "
          			+ "intv11  interval hour (3) to hour      default null, "
          			+ "intv12  interval hour (9) to minute     default null, "
          			+ "intv13  interval hour (8) to second     default null, "
          			+ "intv14  interval minute                 default null, "
          			+ "intv15  interval minute (3) to minute   default null, "
          			+ "intv16  interval minute to second       default null, "
          			+ "intv17  interval second                 default null, "
          			+ "intv18  interval second (6) to second   default null, "
          			+ "primary key (seqno)) no partition ");

          	stmt.close();

          } catch (SQLException e) {
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
