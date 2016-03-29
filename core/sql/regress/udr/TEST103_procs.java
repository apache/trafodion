import java.io.*;
import java.sql.*;
import java.util.*;

// ===================================================================
// Class: subscriptions
//
// ===================================================================
public class TEST103_procs
{
  // --------------------------------------------------------------------------
  // Main code for subscriptions procedure
  // --------------------------------------------------------------------------
  public static void updateSubscriptions (
    String operationIn,
    String valueIn,
    String userIn,
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
    util.log ("***** Starting subscriptions request *****");

    // Set up the connection
    Connection conn = null;
    String sessionUser = null;
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
    currentTime = util.getCurrentTime(conn);

    results[0] = "Session user: " + sessionUser + '\n';
    results[0] += "Current time: " + currentTime + '\n';

    util.log("Session user: " + sessionUser);
    util.log("Current time: " + currentTime);


    boolean isAdd = false;
    boolean isDrop = false;
    boolean isSub = false;
    boolean isRegion = false;

    String operation = operationIn.toUpperCase().trim();
    String inputRqst = "Operation: " + operation + " Change: " + valueIn + "User: " + userIn + '\n';
    util.log (inputRqst);
    if (operation.equals("ADD_SUB"))
    {
        isAdd = true;
        isSub = true;
    }
    else if (operation.equals("DROP_SUB"))
    {
        isDrop = true;
        isSub = true;
    }
    else if (operation.equalsIgnoreCase("ADD_REGIONS"))
    {
        isAdd = true;
        isRegion = true;
    }
    else if (operation.equals("DROP_REGIONS"))
    {
        isDrop = true;
        isRegion = true;
    }
    else //help
    {
         results[0] += "Help option requested: " + '\n' + '\n';
         results[0] += "PROCEDURE SUBSCRIPTIONS " + '\n';
         results[0] += " (<operation>, <newvalue>, <results>) " + '\n';
         results[0] += "    <operation> ::= one of the following" + '\n';
         results[0] += "       ADD_SUB - add a subscription" + '\n';
         results[0] += "       DROP_SUB - drop a subscription" + '\n';
         results[0] += "       ADD_REGIONS - adds regions" + '\n';
         results[0] += "       DROP_REGIONS - drops regions" + '\n';
         results[0] += "       HELP - add a subscription" + '\n';
         results[0] += "    <newValue> ::= what to add or drop " + '\n';
         results[0] += "    <results>  ::= details of the operation";
         return;
    }

    // get subscription record         
    int updatedSub = 0;
    int updatedDevice = 0;
    String userName = userIn.toUpperCase().trim();
    String updatedRegions = "----------";
    try
    {
      String selectQuery  = "select subscription_package, zones_available, ";
      selectQuery += "devices_available from subscribers.subscribers ";
      selectQuery += "where upper(subscriber_user) = '";
      selectQuery += userName;
      selectQuery += "';";
      //results[0] += "selectQuery: " + selectQuery + '\n';
      util.log("selecting subscriber: " + selectQuery);


      Statement selectStmt = conn.createStatement();
      ResultSet selectRS = selectStmt.executeQuery(selectQuery);
      if (selectRS.next())
      {
        updatedSub = selectRS.getInt(1);
        updatedRegions = selectRS.getString(2);
        updatedDevice = selectRS.getInt(3);
      }
      util.log("retrieved values - sub: " + updatedSub + " device: " + updatedDevice + " regions: " + updatedRegions + '\n');

    }
    catch(SQLException e)
    {
      util.log("ERROR: unable to retrieve subscription information" + e.getLocalizedMessage());
      results[0] += "ERROR: unable to retrieve subscription information: ";
      results[0] += "code: " + e.getErrorCode() + " msg: ";
      results[0] += e.getLocalizedMessage();
      throw new SQLException (results[0]);
    }

    try
    {
       // Calculate new values for subscription
       if (isSub)
       {
          String sub = valueIn.trim();
          if (sub.equals("1"))
            if (isAdd)
              updatedSub = updatedSub |= 1;
            else
              updatedSub = updatedSub &= ~1;
          else if (sub.equals("2"))
            if (isAdd)
              updatedSub = updatedSub |= 2;
            else
              updatedSub = updatedSub &= ~2;
          else if (sub.equals("3"))
            if (isAdd)
              updatedSub = updatedSub |= 4;
            else
              updatedSub = updatedSub &= ~4;
          else
          {
            results[0] += "ERROR: invalid subscription value";
            return;
          }
          util.log("New subscription type: " + updatedSub + '\n');
       }
       else if (isRegion)
       {
          String rangeValues = "ABCDEFGHI-";

          // Calculate new values for regions
          char[] tmpRegions = updatedRegions.toCharArray();
          String inRegions = valueIn.trim();
          for (int i = 0; i < inRegions.length(); i++)
          {
            char in = valueIn.charAt(i);
            int ndx = rangeValues.indexOf(in);
            if (ndx == -1)
            {
              results[0] += "ERROR: invalid range value specified: " + in + '\n';
              return;
            }
            if (isAdd)
              tmpRegions[ndx] = in;
            else
              tmpRegions[ndx] = '-';

          }
          updatedRegions = String.valueOf(tmpRegions);
          util.log("New regions: " + updatedRegions + '\n');
       }

       String updateQuery = "update subscribers.subscribers set subscription_package = ";
       updateQuery += updatedSub;
       updateQuery += ", zones_available = '";
       updateQuery += updatedRegions;
       updateQuery += "' where upper(subscriber_user) = '";
       updateQuery += userName;
       updateQuery += "';";

       //results[0] += "updateQuery: " + updateQuery + '\n';
       util.log("update subscriber: " + updateQuery);
       PreparedStatement updateStmt = null;
       updateStmt = conn.prepareStatement (updateQuery);
       updateStmt.executeUpdate();
    }

    catch(SQLException e)
    {
      util.log("ERROR: unable to update subscriptions " + e.getLocalizedMessage());
      results[0] += "ERROR: unable to update subscriptions - ";
      results[0] += e.getLocalizedMessage();
      throw new SQLException (results[0]);
    }

    results[0] += "Operation successful";
    util.log ("***** Completed subscriptions request *****");
  }
}

