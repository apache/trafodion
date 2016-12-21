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
//
// This is the Java stored procedure for adding and deleting from the USTAT_AUTO_TABLES.
// Compile this as: javac ChgAutoList.java;  
//  (Note that the class must be the same name as the file).
//
package org.trafodion.sql.ustat;

import java.sql.*;
import java.io.*;

public class ChgAutoList {
 
  public static void chg(String  operation,     // Input
                         String  schema,        // Input
                         String  table,         // Input
                         String[] result)       // Output
      throws SQLException
  {
    String tableCat  = "NEO";
    String autoCat   = "MANAGEABILITY";
    String autoSch   = "HP_USTAT";
    String autoTable = autoCat + "." + autoSch + ".USTAT_AUTO_TABLES";

    operation = operation.toUpperCase().trim();
    schema    = schema.trim();
    table     = table.trim();
    if (schema.length() > 0)
      if (schema.charAt(0) != '"') schema = schema.toUpperCase();
      else                         schema = internalFormat(schema);
    if (table.length() > 0)    
      if (table.charAt(0)  != '"') table  = table.toUpperCase();
      else                         table  = internalFormat(table);

    String intSchInStrLit = schema;
    String intTblInStrLit = table;
    intSchInStrLit = "_UCS2'" + intSchInStrLit.replaceAll("'", "''") + "'";
    intTblInStrLit = "_UCS2'" + intTblInStrLit.replaceAll("'", "''") + "'";

    String extSchName = schema;
    String extTblName = table;
    extSchName = "\"" + extSchName.replaceAll("\"", "\"\"") + "\"";
    extTblName = "\"" + extTblName.replaceAll("\"", "\"\"") + "\"";
    String extSchDotTbl = extSchName+"."+extTblName;

    String addStr  = "INSERT";
    String inclStr = "INCLUDE"; // This is a synonym for INSERT.
    String exclStr = "EXCLUDE";
    String delStr  = "DELETE";
    Connection conn   = DriverManager.getConnection("jdbc:default:connection");

    // Check for valid schema and table names.
    if      (schema.length() > 128) result[0]="Schema name too long. No changes made.";
    else if (table.length()  > 128) result[0]="Table name too long. No changes made.";
    else if (( schema.equals("*") && !table.equals("*")) ||
             (!schema.equals("*") &&  table.equals("*")))
      result[0]="You must specify '*' for both schema and table. No changes made.";
    else if (schema.equals("") || table.equals(""))
      result[0]="\"" + schema + "\".\"" + table + 
                "\" is an invalid name. No changes made.";
    else try {
      if(operation.equals(addStr) || operation.equals(inclStr) || operation.equals(exclStr))
      {          
        // Perform INSERT, INCLUDE, and EXCLUDE command.
        if (!operation.equals(exclStr) && schema.equals("*") && table.equals("*")) 
        {
          // Perform INSERT or INCLUDE of all tables ('*'.'*' for schema and table).          
          try 
          {

            String os = System.getProperty("os.name").toLowerCase();
            String sys = "";
  
            if ( os.indexOf("linux") >=0 ) {
              sys = "NSK"; 
            } 
   
            else { // assume NSK
              // Obtain system name, which is needed for query to get all tables.
              String shellCmd ="/bin/gtacl -c SYSINFO";
              Process p = Runtime.getRuntime().exec(shellCmd);
              BufferedReader stdInput = new BufferedReader(new 
                                        InputStreamReader(p.getInputStream()));
              String s;
              int pos;
              while ((s = stdInput.readLine()) != null)
              if ((pos = s.indexOf("System name")) >= 0)
              {
                pos = s.indexOf("\\"); // Find beginning of system name.
                sys = s.substring(pos+1);
              }
            }

            PreparedStatement findSchemaVersion, insStmt, delStmt, cntStmt;
            // Obtain a list of all schema versions >= 2300 present on system.
            String verCmd="SELECT DISTINCT S.SCHEMA_VERSION " +
                       " FROM HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.CATSYS C, " +
                       "     HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.SCHEMATA S " +
                       " WHERE C.CAT_UID=S.CAT_UID AND " +
                       "      C.CAT_NAME=_UCS2'NEO' AND " +
                       "      S.SCHEMA_VERSION >= 2300";
            findSchemaVersion = conn.prepareStatement(verCmd);
            ResultSet rs = findSchemaVersion.executeQuery();

            String ver, cmd;
            int autoListCnt=0;
            // Loop through all schema versions >= 2300:
            while (rs.next()) // Advance to next row in result set
            {
              ver=""+rs.getInt(1); // Get current row (version) from result set.

              String cqdCmd="CONTROL QUERY DEFAULT BLOCK_TO_PREVENT_HALLOWEEN 'ON'";
              PreparedStatement cqdStmt = conn.prepareStatement(cqdCmd);
              cqdStmt.executeUpdate();

              // Insert all tables and MVs in NEO catalog that don't already exist in list.
              cmd="INSERT INTO " + autoTable +
                " SELECT C.CAT_NAME, S.SCHEMA_NAME, O.OBJECT_NAME, " +
                "       TIMESTAMP '0001-01-01 00:00:00', " +
                "       TIMESTAMP '0001-01-01 00:00:00', " +
                "       0, _UCS2'', _ISO88591'SYSTEM' " +
                " FROM HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.CATSYS C, " +
                "     HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.SCHEMATA S, " +
                "     "+tableCat+".HP_DEFINITION_SCHEMA.OBJECTS O " +
                " WHERE C.CAT_UID=S.CAT_UID AND " +
                "      S.SCHEMA_UID=O.SCHEMA_UID AND " +
                "     (O.OBJECT_TYPE=_ISO88591'BT' OR O.OBJECT_TYPE=_ISO88591'MV') AND " +
                "      O.OBJECT_NAME_SPACE=_ISO88591'TA' AND " +
                "      C.CAT_NAME=_UCS2'NEO' AND " +
                "      S.SCHEMA_NAME<>_UCS2'HP_DEFINITION_SCHEMA' AND " +
                "      S.SCHEMA_NAME<>_UCS2'PUBLIC_ACCESS_SCHEMA' AND " +
                "      S.SCHEMA_NAME NOT LIKE _UCS2'HP\\_%' ESCAPE _UCS2'\\' AND " +
                "      S.SCHEMA_NAME NOT LIKE _UCS2'VOLATILE\\_SCHEMA\\_%' ESCAPE _UCS2'\\' AND " +
                "      O.OBJECT_NAME<>_UCS2'HISTOGRAMS' AND " +
                "      O.OBJECT_NAME<>_UCS2'HISTOGRAM_INTERVALS' AND " +
                "      O.OBJECT_NAME<>_UCS2'HISTOGRAMS_FREQ_VALS' AND " +
                "      O.OBJECT_NAME<>_UCS2'MVS_TABLE_INFO_UMD' AND " +
                "      O.OBJECT_NAME<>_UCS2'MVS_UMD' AND " +
                "      O.OBJECT_NAME<>_UCS2'MVS_USED_UMD' AND " +
                "      (C.CAT_NAME, S.SCHEMA_NAME, O.OBJECT_NAME) NOT IN " +
                "        (SELECT CAT_NAME, SCH_NAME, TBL_NAME FROM " + autoTable + ")";
              insStmt = conn.prepareStatement(cmd);
              insStmt.executeUpdate();

              // Delete all tables and MVs in list that no longer exist in NEO catalog.
              cmd="DELETE FROM " + autoTable + " WHERE ADDED_BY<>_ISO88591'EXCLUD' AND " +
                " (CAT_NAME, SCH_NAME, TBL_NAME) NOT IN " +
                " (SELECT C.CAT_NAME, S.SCHEMA_NAME, O.OBJECT_NAME " +
                " FROM HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.CATSYS C, " +
                "     HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.SCHEMATA S, " +
                "     "+tableCat+".HP_DEFINITION_SCHEMA.OBJECTS O " +
                " WHERE C.CAT_UID=S.CAT_UID AND " +
                "      S.SCHEMA_UID=O.SCHEMA_UID AND " +
                "     (O.OBJECT_TYPE=_ISO88591'BT' OR O.OBJECT_TYPE=_ISO88591'MV') AND " +
                "      O.OBJECT_NAME_SPACE=_ISO88591'TA' AND " +
                "      C.CAT_NAME=_UCS2'NEO' AND " +
                "      S.SCHEMA_NAME<>_UCS2'HP_DEFINITION_SCHEMA' AND " +
                "      S.SCHEMA_NAME<>_UCS2'PUBLIC_ACCESS_SCHEMA' AND " +
                "      S.SCHEMA_NAME NOT LIKE _UCS2'HP\\_%' ESCAPE _UCS2'\\' AND " +
                "      S.SCHEMA_NAME NOT LIKE _UCS2'VOLATILE\\_SCHEMA\\_%' ESCAPE _UCS2'\\' AND " +
                "      O.OBJECT_NAME<>_UCS2'HISTOGRAMS' AND " +
                "      O.OBJECT_NAME<>_UCS2'HISTOGRAM_INTERVALS' AND " +
                "      O.OBJECT_NAME<>_UCS2'HISTOGRAMS_FREQ_VALS' AND " +
                "      O.OBJECT_NAME<>_UCS2'MVS_TABLE_INFO_UMD' AND " +
                "      O.OBJECT_NAME<>_UCS2'MVS_UMD' AND " +
                "      O.OBJECT_NAME<>_UCS2'MVS_USED_UMD')";
              delStmt = conn.prepareStatement(cmd);
              delStmt.executeUpdate();                                                                     
            }
            // Get current count of tables that will be automated.
            cmd="SELECT COUNT(*) FROM " + autoTable + " WHERE ADDED_BY<>_ISO88591'EXCLUD'";
            cntStmt = conn.prepareStatement(cmd);
            rs = cntStmt.executeQuery();
            rs.next(); 
            autoListCnt = rs.getInt(1);
            
            result[0]="INSERTed " + autoListCnt + " table names (all) into list.";
            rs.close();
          }
          catch(IOException err)
          {
            // Shell failure message.
            result[0] = "Unable to " + operation + ".  Error: " + err.getMessage().trim();
            if (result[0].charAt(result[0].length()-1) == ']') // Remove date/time.
              result[0]=result[0].substring(0,result[0].length()-21);
          }   
        }
        else if (operation.equals(exclStr) && 
                 schema.equals("*") && table.equals("*")) 
          result[0] = "EXCLUDE failed. Specifying '*', '*' not allowed.";
        else
        {
          // User has requested to INSERT, INCLUDE, or EXCLUDE a specific table.
          String addedBy="USER";
          String action=operation+"d";
          if (operation.equals(addStr))  action=operation+"ed";
          if (operation.equals(exclStr))
          {
            addedBy="EXCLUD";
            // For EXCLUDE, always delete the blank entry created when all entries are deleted.
            // (See DELETE below.)  In addition, if EXCLUDing, and an entry already exists for
            // this schema and table with ADDED_BY='SYSTEM', remove so it can be EXCLUDEd.
            PreparedStatement delStmt1 =
              conn.prepareStatement("DELETE FROM " + autoTable + " WHERE CAT_NAME=_UCS2''");
            // Do not check for errors.
            delStmt1.executeUpdate();
            PreparedStatement delStmt2 =
              conn.prepareStatement("DELETE FROM " + autoTable + " WHERE CAT_NAME=_UCS2'NEO' " +
                                    "AND SCH_NAME=" + intSchInStrLit + " AND TBL_NAME=" + intTblInStrLit +
                                    " AND ADDED_BY=_ISO88591'SYSTEM'");
            // Do not check for errors.
            delStmt2.executeUpdate();
          }
          
          PreparedStatement insStmt =
            conn.prepareStatement("INSERT INTO " + autoTable + " VALUES (_UCS2'NEO'," +
                                  " ?, ?, TIMESTAMP '0001-01-01 00:00:00'," +
                                  " TIMESTAMP '0001-01-01 00:00:00', 0, _UCS2'', _ISO88591'" +
                                  addedBy + "')");
          insStmt.setString(1, schema);  // Set first  argument in statement (1st '?').
          insStmt.setString(2, table);   // Set second argument in statement (2nd '?').
          if (insStmt.executeUpdate() == 1) 
            result[0]="Table name "+extSchDotTbl+" " + action +".";

        }
      }
      else if(operation.equals(delStr)) 
      {
        // Perform DELETE command.
        if (schema.equals("*") && table.equals("*")) 
        {
          // If the user has specified '*'.'*' for schema and table, remove all 
          // entries in list, then add an empty entry.
          PreparedStatement delStmt = conn.prepareStatement("DELETE FROM " + autoTable);
          delStmt.executeUpdate();
          result[0]="All entries DELETEd.  Automation disabled.";
          
          // Add the empty entry, which is needed so that USAS.sh does not later insert all
          // existing tables.  It would do so if the USTAT_AUTO_TABLES table were empty.
          PreparedStatement insStmt =
            conn.prepareStatement("INSERT INTO " + autoTable + 
								  " VALUES (_UCS2'', _UCS2'', _UCS2'', " +
                                  " TIMESTAMP '0001-01-01 00:00:00'," +
                                  " TIMESTAMP '0001-01-01 00:00:00', 0, _UCS2'', _ISO88591'USER')");
          insStmt.executeUpdate();

          try {
            // Remove USTAT_AUTOMATION_INTERVAL entry from SYSTEM_DEFAULTS tables.

            String os = System.getProperty("os.name").toLowerCase();
  
	    if ( os.indexOf("linux") >=0 ) {
                PreparedStatement delStmt2 =
                   conn.prepareStatement(
        "DELETE FROM HP_SYSTEM_CATALOG.SYSTEM_DEFAULTS_SCHEMA.SYSTEM_DEFAULTS " +
        "WHERE ATTRIBUTE = 'USTAT_AUTOMATION_INTERVAL'");
               // Do not check for errors.
               delStmt2.executeUpdate();
  
               // Now remove AUTO_CQDS_SET file from the cluster.
               String shellCmd;

               String sqroot = System.getenv("TRAF_HOME");

               shellCmd = "rm " + sqroot + "/export/lib/mx_ustat/autodir/USTAT_CQDS_SET";
               Process p = Runtime.getRuntime().exec(shellCmd);

               shellCmd = "rm " + sqroot + "/export/lib/mx_ustat/autoprev/USTAT_CQDS_SET";
               p = Runtime.getRuntime().exec(shellCmd);

            } else {
  
              // assume NSK
              // Obtain system name.
              String sys="";
              String shellCmd = "/bin/gtacl -c SYSINFO";
              Process p = Runtime.getRuntime().exec(shellCmd);
              BufferedReader stdInput = new BufferedReader(new 
                InputStreamReader(p.getInputStream()));
              String s;
              int pos;
              while ((s = stdInput.readLine()) != null)
                if ((pos = s.indexOf("System name")) >= 0)
                {
                  pos = s.indexOf("\\"); // Find beginning of system name.
                  sys = s.substring(pos+1);
                }
  
              // Obtain all segment names.  The grep here is really to avoid getting names
              // of systems that are on expand which are not segments.
              String sysprefix=sys.substring(0,3).toLowerCase();
              shellCmd = "ls /E";
              p = Runtime.getRuntime().exec(shellCmd);
              stdInput = new BufferedReader(new InputStreamReader(p.getInputStream()));
              // For each segment, remove USTAT_AUTOMATION_INTERVAL from system defaults table.
              // (make sure the segment name returned starts with 'sysprefix').
              while ((s = stdInput.readLine()) != null && s.indexOf(sysprefix) == 0)
              {
                PreparedStatement delStmt2 =
                  conn.prepareStatement("DELETE FROM HP_SYSTEM_CATALOG" + 
                                        ".SYSTEM_DEFAULTS_SCHEMA.SYSTEM_DEFAULTS " +
                                        "WHERE ATTRIBUTE = 'USTAT_AUTOMATION_INTERVAL'");
                // Do not check for errors.
                delStmt2.executeUpdate();
              }
  
              // Now remove AUTO_CQDS_SET file from primary segment.
              shellCmd = "rm /E/" + sysprefix + "0101/usr/tandem/mx_ustat/autodir/USTAT_CQDS_SET";
              p = Runtime.getRuntime().exec(shellCmd);
              shellCmd = "rm /E/" + sysprefix + "0101/usr/tandem/mx_ustat/autoprev/USTAT_CQDS_SET";
              p = Runtime.getRuntime().exec(shellCmd);
            }

          }
          catch(IOException err)
          {
            // Shell failure message.
            result[0] = "Unable to remove USTAT_AUTOMATION_INTERVAL from SYSTEM_DEFAULTS " + 
                        "tables.  You must do this manually.";
            if (result[0].charAt(result[0].length()-1) == ']') // Remove date/time.
              result[0]=result[0].substring(0,result[0].length()-21);
          }   
        }
        else 
        {          
          // User has requested to delete a specific table.
          // First see if the table is 'EXCLUD' and can be deleted.  Note that deletion
          // of the last 'USER' added table results in a blank 'USER' entry being added.
          // This is not done for deletion of 'EXCLUD'ed tables.
          PreparedStatement delete1 =
            conn.prepareStatement("DELETE FROM " + autoTable + 
            " WHERE SCH_NAME = ? AND TBL_NAME = ? AND ADDED_BY=_ISO88591'EXCLUD'");
            delete1.setString(1, schema);  // Set first  argument in statement (1st '?').
            delete1.setString(2, table);   // Set second argument in statement (2nd '?').
          if (delete1.executeUpdate() == 0) 
          {
            // Failed to delete (0 rows deleted).  Either the table did not have 
            // ADDED_BY='EXCLUD' or entry does not exist.  Try to delete for any ADDED_BY.
            PreparedStatement delete2 =
              conn.prepareStatement("DELETE FROM " + autoTable +
              " WHERE SCH_NAME = ? AND TBL_NAME = ?");
              delete2.setString(1, schema);  // Set first  argument in statement (1st '?').
              delete2.setString(2, table);   // Set second argument in statement (2nd '?').
            if (delete2.executeUpdate() == 0) 
              result[0]="Table name  "+extSchDotTbl+" not found, not DELETEd.";
            else
            { 
              // A 'SYSTEM' or 'USER' table DELETEd.
              result[0]="Table name "+extSchDotTbl+" DELETEd.";

              // Add the empty entry, if there are no rows with the ADDED_BY field set to
              // 'USER'.  This keeps USAS.sh from inserting all existing tables later
              // on.  It would do so if all 'USER' entries from USTAT_AUTO_TABLES table had
              // been deleted.
              PreparedStatement FindUserEnteredTables =
                conn.prepareStatement("SELECT COUNT(*) FROM " + autoTable +
                " WHERE ADDED_BY = _ISO88591'USER'" + 
                " FOR READ UNCOMMITTED ACCESS");
              ResultSet rs = FindUserEnteredTables.executeQuery();
              rs.next();
              if (rs.getInt(1) == 0)
              {
                PreparedStatement insStmt =
                  conn.prepareStatement("INSERT INTO " + autoTable + 
				  " VALUES (_UCS2'', _UCS2'', _UCS2'', " +
                  " TIMESTAMP '0001-01-01 00:00:00'," +
                  " TIMESTAMP '0001-01-01 00:00:00', 0, _UCS2'', _ISO88591'USER')");
                insStmt.executeUpdate();
              }
              rs.close();
            }
          }
          // 'EXCLUD' table was successfully DELETEd, set result string.
          else result[0]="Table name "+extSchDotTbl+"\" DELETEd.";
        }
      }
      else 
      {
        result[0] = operation + " is not a valid operation.";
      }
    } 
    catch(SQLException err)
    {
      result[0] = err.getMessage().trim(); // Issue SQL error.
      if (result[0].charAt(result[0].length()-1) == ']') // Remove date/time.
        result[0]=result[0].substring(0,result[0].length()-21);
    } 
    finally 
    {
      conn.close();
    }
    if (result[0].length() > 80) result[0]=result[0].substring(0,79);
  }

  public static String internalFormat(String name)
  {
    // Remove enclosing quotes
    name=name.substring(1, name.length()-1);

    // Change all occurrences of "" to ".
    int index=-1;
    while((index=name.indexOf("\"\"", index+1)) != -1)
      name=name.substring(0,index+1)+name.substring(index+2);  

    return name;
  }
}
