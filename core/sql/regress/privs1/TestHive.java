import java.io.*;
import java.sql.*;
import java.util.*;

// ===================================================================
// Class: testHive
//
// ===================================================================
public class TestHive
{
  // --------------------------------------------------------------------------
  // Main code for credentials procedure
  // --------------------------------------------------------------------------
  public static void accessHive (
    String operationIn,
    String[] results)

  throws SQLException
  {
    // Initialize the Utils structure
    // Utils provides connection, user, current time, and logging information
    Utils util = null;

    try
    {
       util = new Utils();
    }
    catch(Exception e)
    {
      String theError = "ERROR: unable to create the Util object";
      throw e;
    }

    util.log ("");
    util.log ("***** Starting credentials request *****");

    // Set up the connection
    Connection conn = null;
    String sessionUser = null;
    String currentUser = null;
    String currentTime = null;
    try
    {
       conn = util.myConnection();
    }
    catch(Exception e)
    {
      String theError = "ERROR: unable to get a connection";
      throw e;
    }

    sessionUser = util.getSessionUser(conn);
    currentUser = util.getCurrentUser(conn);
    currentTime = util.getCurrentTime(conn);

    results[0] = "External user: " + sessionUser + '\n';
    results[0] += "Current user: " + currentUser + '\n';
    results[0] += "Current time: " + currentTime + '\n';

    util.log("Session user: " + sessionUser);
    util.log("Current user: " + currentUser);
    util.log("Current time: " + currentTime);

    try
    {
      int numRows;
      String selectQuery = "select count(*) from hive.hive.";
      selectQuery += operationIn;
      Statement stmt = conn.createStatement();
      util.log("Executing select query");
      ResultSet rset = stmt.executeQuery(selectQuery);
      if (rset.next())
      {
        numRows = rset.getInt(1);
        results[0] += "Number rows:" + numRows + '\n';
      }
      else
        results[0] += "none found" + '\n';

      rset.close(); stmt.close();
    } 
    catch (SQLException e) {
      util.log("Error encountered in getCurrentUser " + e.getMessage());
      throw new SQLException(e.getMessage());
    }

    results[0] += "Operation successful";
    util.log ("***** Completed credentials request *****");
  }
}

