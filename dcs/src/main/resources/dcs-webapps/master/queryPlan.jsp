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
<%@ page contentType="text/html;charset=UTF-8"
  import="java.io.*"
  import="java.util.*"
  import="org.apache.hadoop.conf.Configuration"
  import="org.trafodion.dcs.master.DcsMaster"
  import="org.trafodion.dcs.util.DcsConfiguration"
  import="org.trafodion.dcs.Constants"
  import="org.trafodion.dcs.util.Bytes"
  import="org.codehaus.jettison.json.JSONArray"
  import="org.codehaus.jettison.json.JSONException"
  import="org.codehaus.jettison.json.JSONObject"
  import="org.trafodion.dcs.master.QueryPlanModel"
  import="org.trafodion.dcs.master.QueryPlanResponse"
  import="org.codehaus.jackson.JsonGenerationException"
  import="org.codehaus.jackson.map.JsonMappingException"
  import="org.codehaus.jackson.map.ObjectMapper"
 %>
<% 
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
      String sControlStmts = jsonObject.getString("sControlStmts");
  
      if(sQuery == null || sQuery.trim().length() < 1) {
          response.setContentType("text/plain");
          response.setStatus(response.SC_INTERNAL_SERVER_ERROR);
          response.getWriter().print("The query text cannot be empty");
          return;
      }
 
      QueryPlanModel qpm = new QueryPlanModel();
      qpm.GeneratePlan(master.getServerManager().getJdbcT4Util(),sQuery, sControlStmts);
      QueryPlanResponse qpr = qpm.getQueryPlanResponse();
      ObjectMapper mapper = new ObjectMapper();
      JSONObject queryPlanJson = null;
      queryPlanJson = new JSONObject(mapper.writeValueAsString(qpr));
      response.setContentType("application/json");
      response.getWriter().print(queryPlanJson);
  } catch (Exception e) {
       response.setContentType("text/plain");
       response.setStatus(response.SC_INTERNAL_SERVER_ERROR);
       response.getWriter().print(e.getMessage());
  }
  %>