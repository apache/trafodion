<%--
/**
 * @@@ START COPYRIGHT @@@  
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 *
 * @@@ END COPYRIGHT @@@     
 */
--%>
<%@ taglib uri="http://displaytag.sf.net" prefix="display" %>
<%@ page contentType="text/html;charset=UTF-8"
  import="java.io.*"
  import="java.util.*"
  import="java.sql.*"
  import="org.apache.hadoop.conf.Configuration"
  import="org.trafodion.dcs.master.DcsMaster"
  import="org.trafodion.dcs.util.DcsConfiguration"
  import="org.trafodion.dcs.Constants"
  import="org.trafodion.dcs.util.Bytes"
  import="org.trafodion.dcs.util.JdbcT4Util"
  import="org.codehaus.jettison.json.JSONArray"
  import="org.codehaus.jettison.json.JSONException"
  import="org.codehaus.jettison.json.JSONObject"
 %>
<% 

  java.sql.Connection connection = null;
  java.sql.Statement stmt = null;
  java.sql.ResultSet rs = null;
  
  try {
      DcsMaster master = (DcsMaster)getServletContext().getAttribute(DcsMaster.MASTER);
      Configuration conf = master.getConfiguration();
  
      BufferedReader br = new BufferedReader(new InputStreamReader(request.getInputStream()));
      String json = "";
      if(br != null) {
          json = br.readLine();
      }
  
      JSONObject jsonObject = new JSONObject(json);
      String sQuery = jsonObject.getString("sQuery");
      if(sQuery == null || sQuery.trim().length() < 1) {
          response.setContentType("text/plain");
          response.setStatus(response.SC_INTERNAL_SERVER_ERROR);
          response.getWriter().print("The query text cannot be empty");
          return;
      }
      
      String sControlStmts = jsonObject.getString("sControlStmts");
      String[] controlStatements = new String[0];
      if (sControlStmts != null && sControlStmts.length() > 0) {
         controlStatements = sControlStmts.split(";");
      }
  
      JSONArray result = null;
 
      JdbcT4Util jdbcT4Util = master.getServerManager().getJdbcT4Util();
      connection = jdbcT4Util.getConnection();
      stmt = connection.createStatement();
      for (String controlText : controlStatements) {
         stmt.execute(controlText);
      }
      stmt.close();
      
      stmt = connection.createStatement();
      rs = stmt.executeQuery(sQuery);
      result = jdbcT4Util.convertResultSetToJSON(rs);
      rs.close();
      stmt.close();
      connection.close();
      response.setContentType("application/json");
      response.getWriter().print(result);
  } catch (SQLException e) {
      SQLException nextException = e;
      StringBuilder sb = new StringBuilder();
      do {
          sb.append(nextException.getMessage());
          sb.append("\nSQLState   " + nextException.getSQLState());
          sb.append("\nError Code " + nextException.getErrorCode());
      } while ((nextException = nextException.getNextException()) != null);
      response.setContentType("text/plain");
      response.setStatus(response.SC_INTERNAL_SERVER_ERROR);
      response.getWriter().print(sb.toString());
   } catch (Exception e) {
      response.setContentType("text/plain");
      response.setStatus(response.SC_INTERNAL_SERVER_ERROR);
      response.getWriter().print(e.getMessage());
  } finally {
      if (rs != null)
        rs.close();
      if (stmt != null)
        stmt.close();
      if (connection != null)
        connection.close();
  }  
  %>