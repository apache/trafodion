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

import java.io.*;
import java.sql.*;
import java.util.*;

// ===================================================================
// Class:  Utils
//
//  This is a helper class for the other SPJs that: 
//     - gets connection (myConnection)
//     - gets current user executing the SPJ (getCurrentUser)
//     - gets session user if definer rights are used (getSessionUser)
//     - gets current timestamp ( getCurrentTime )
//     - transaction management ops
//        - BEGIN
//        - COMMIT
//        - ROLLBACK
//     - log debug statements (log)
//     - log execeptions (printExceptionStack)
//
// ===================================================================
public class Utils 
{
   static boolean log = false;
   static String logFileName = "udr_tools.log";
   static String logLocation = null;
   static String sqLogs = null;
   static String userName = null;

   Utils () 
   {
     if (!log)
       log = Boolean.getBoolean("debugOn");
     
     if (log) { 
       sqLogs = System.getenv("TRAF_LOG");
       logLocation = sqLogs + "/";
     }
   }

   // Returns a JDBC Connection for use within the SPJ 
   // returns Connection and throws Exception

    public static Connection myConnection() throws SQLException
    {
        Connection conn = null;
        try
        {
           String url = "jdbc:default:connection";
           java.util.Properties props = new java.util.Properties();
           props.setProperty("maxStatements", "0"); // disable JDBC statement caching
           conn = DriverManager.getConnection(url, props);
           log("Returning connection from myConnection " + (conn != null));
        }
        catch (Throwable t)
        {
           log("Error encountered while getting connection " + t.getMessage());
           printExceptionStack(t);
           throw new SQLException(t.getMessage());
        }
        return conn;
    }

    private static String getLogFile(String path) 
    {
       String file=null;

       if (path != null)
        file = path + logFileName;
       else 
        file = logFileName;
       return file;
    }

    // Log a message to a log file.    
    protected static void log(String message)
    {
      if (log)
      {
       try
       {
          String file = getLogFile(logLocation);
          FileWriter fw = new FileWriter(file, true);
          PrintWriter pw = new PrintWriter(fw);
          pw.println("@" + Calendar.getInstance().getTime() + ": " + message);
          pw.close();
          fw.flush();
          fw.close();
       } catch (Exception ex) {}
      }
    }
    
     // Print exception stack.
    protected static void printExceptionStack(Throwable t)
    {
      if (log)
      {
       try
       {
          String file = getLogFile(logLocation);
          FileWriter fw = new FileWriter(file, true);
          PrintWriter pw = new PrintWriter(fw);
          t.printStackTrace(pw);
          pw.close();
          fw.flush();
          fw.close();
        } catch (Exception ex) { }
      }
    }

    public static String getCurrentUser(Connection conn) throws SQLException
    {
        log("In method getCurrentUser");
        try {
          Statement stmt = conn.createStatement();
          log("In method getCurrentUser:gotStatement");
          ResultSet rset = stmt.executeQuery("VALUES(USER)");
          while (rset != null && rset.next())
          {
            userName = rset.getString(1);
          }
          log("In method getCurrentUser:doneExecuteQuery::" + userName);
          rset.close(); stmt.close();
        } catch (SQLException e) {
          log("Error encountered in getCurrentUser " + e.getMessage());
          printExceptionStack(e);
          throw new SQLException(e.getMessage());
        }
        return userName;
    }
    
    public static String getSessionUser(Connection conn) throws SQLException
    {
        log("In method getSessionUser");
        try {
          Statement stmt = conn.createStatement();
          log("In method getSessionUser:gotStatement");
          ResultSet rset = stmt.executeQuery("VALUES(SESSION_USER)");
          while (rset != null && rset.next())
          {
            userName = rset.getString(1);
          }
          log("In method getSessionUser:doneExecuteQuery::" + userName);
          rset.close(); stmt.close();
        } catch (SQLException e) {
          log("Error encountered in getSessionUser " + e.getMessage());
          printExceptionStack(e);
          throw new SQLException(e.getMessage());
        }
        return userName;
    }

    public static String getCurrentTime(Connection conn) throws SQLException
    {
        log("In method getCurrentTime");
        try {
          Statement stmt = conn.createStatement();
          log("In method getCurrentTime:gotStatement");
          ResultSet rset = stmt.executeQuery("VALUES(CURRENT_TIMESTAMP(2))");
          while (rset != null && rset.next())
          {
            userName = rset.getString(1);
          }
          log("In method getCurrentTime:doneExecuteQuery::" + userName);
          rset.close(); stmt.close();
        } catch (SQLException e) {
          log("Error encountered in getCurrentTime " + e.getMessage());
          printExceptionStack(e);
          throw new SQLException(e.getMessage());
        }
        return userName;
    }

    public static void beginTxn(Connection conn) throws SQLException
    {
        log("In method beginTxn");
        try {
          log("In method beginTxn:gotStatement");
          PreparedStatement stmt = conn.prepareStatement("begin work;");
          stmt.executeUpdate();

          log("In method beginTxn:doneExecuteUpdate");
          stmt.close();
        } catch (SQLException e) {
          if (e.getErrorCode() != -8603) {
            log("Error encountered in beginTxn " + e.getMessage());
            printExceptionStack(e);
            throw new SQLException(e.getMessage());
          }
        }
    }

    public static void commitTxn(Connection conn) throws SQLException
    {
        log("In method commitTxn");
        try {
          log("In method commitTxn:gotStatement");
          PreparedStatement stmt = conn.prepareStatement("commit work;");
          stmt.executeUpdate();

          log("In method commitTxn:doneExecuteUpdate");
          stmt.close();
        } catch (SQLException e) {
          if (e.getErrorCode() != -8605) {
            log("Error encountered in commitTxn " + e.getMessage());
            printExceptionStack(e);
            throw new SQLException(e.getMessage());
          }
        }
    }

    public static void rollbackTxn(Connection conn) throws SQLException
    {
        log("In method rollbackTxn");
        try {
          log("In method rollbackTxn:gotStatement");
          PreparedStatement stmt = conn.prepareStatement("rollback work;");
          stmt.executeUpdate();

          log("In method rollbackTxn:doneExecuteUpdate");
          stmt.close();
        } catch (SQLException e) {
          if (e.getErrorCode() != -8609) {
            log("Error encountered in rollbackTxn " + e.getMessage());
            printExceptionStack(e);
            throw new SQLException(e.getMessage());
          }
        }
    }

    InputStream origIn_;
    PrintStream origOut_;
    PrintStream origErr_;
    
    public static void RedirectStdOut(String filename) throws Exception
    {
       InputStream origIn_  = System.in;
       PrintStream origOut_ = System.out;
       PrintStream origErr_ = System.err;
        
       String sqRoot      = System.getenv("TRAF_HOME");
       String stdoutFile  = sqRoot + "/sql/scripts/" + filename;
       PrintStream stdout = null;

       try
       {
          stdout = new PrintStream(new FileOutputStream(stdoutFile));
       }
       catch (Exception e)
       {
          System.out.println("RedirectStdOut: unable to open " + stdoutFile);
          throw new Exception(e.getMessage());
       }
       
       System.setOut(stdout);
    } // RedirectStdOut  

} // class utils

