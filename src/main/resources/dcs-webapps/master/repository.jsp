<%--
/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
--%>
<%@ taglib uri="http://displaytag.sf.net" prefix="display" %>
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
 %>
<% 
  DcsMaster master = (DcsMaster)getServletContext().getAttribute(DcsMaster.MASTER);
  Configuration conf = master.getConfiguration();
  boolean readOnly = conf.getBoolean("dcs.master.ui.readonly", false);
  String masterServerName = master.getServerName();
  int masterInfoPort = master.getInfoPort();
  String trafodionHome = master.getTrafodionHome();
  boolean trafodionLogs = conf.getBoolean(Constants.DCS_MASTER_TRAFODION_LOGS, Constants.DEFAULT_DCS_MASTER_TRAFODION_LOGS);
  List<JSONObject> metricSessionList = master.getServerManager().getRepositoryItemList(Constants.TRAFODION_REPOS_METRIC_SESSION_TABLE);
  List<JSONObject> metricQueryList = master.getServerManager().getRepositoryItemList(Constants.TRAFODION_REPOS_METRIC_QUERY_TABLE);
  List<JSONObject> metricQueryAggrList = master.getServerManager().getRepositoryItemList(Constants.TRAFODION_REPOS_METRIC_QUERY_AGGR_TABLE);
  String type = request.getParameter("type");
%>
<?xml version="1.0" encoding="UTF-8" ?>
<!-- Commenting out DOCTYPE so our blue outline shows on hadoop 0.20.205.0, etc.
     See tail of HBASE-2110 for explaination.
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" 
  "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"> 
-->
<html xmlns="http://www.w3.org/1999/xhtml">
<head><meta http-equiv="Content-Type" content="text/html;charset=UTF-8"/>
<title>Repository: <%= type %></title>
<link rel="stylesheet" type="text/css" href="/static/dcs.css" />
</head>
<body>
<h1 id="page_title">Repository: <%= type %></h1>
<p id="links_menu">
  <a href="http://<%= masterServerName %>:<%= masterInfoPort %>">Home</a>,
  <a href="/logs/">Dcs local logs</a><% if(! trafodionHome.isEmpty()) { %><% if(trafodionLogs) { %>, <a href="/TrafodionLogs/">Trafodion local logs</a><% } %><% } %>
</p>
<hr id="head_rule" />
<h2>Table: <%= Constants.TRAFODION_REPOS_METRIC_SESSION_TABLE %></h2>
<br>
    <jsp:scriptlet> request.setAttribute( "metricSessionList", metricSessionList ); </jsp:scriptlet>
    <display:table name="metricSessionList" id="metricSession" pagesize="50">
    <%
    if(metricSessionList != null && ! metricSessionList.isEmpty()) {
        try {
            JSONObject aJsonObject = metricSessionList.get(0);
            Iterator itr = aJsonObject.keys(); 
            while(itr.hasNext()) {
                Object element = itr.next();
    %>
                <display:column title="<%= (String)element %>" sortable="true"><%= ((JSONObject)metricSessionList.get(metricSession_rowNum - 1)).getString((String)element) %></display:column> 
    <%
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    %>
    </display:table>
   
<h2>Table: <%= Constants.TRAFODION_REPOS_METRIC_QUERY_TABLE %></h2>
<br>
    <jsp:scriptlet> request.setAttribute( "metricQueryList", metricQueryList ); </jsp:scriptlet>
    <display:table name="metricQueryList" id="metricQuery" pagesize="50">
    <%
    if(metricQueryList != null && ! metricQueryList.isEmpty()) {
        try {
            JSONObject aJsonObject = metricQueryList.get(0);
            Iterator itr = aJsonObject.keys(); 
            while(itr.hasNext()) {
                Object element = itr.next();
    %>
                <display:column title="<%= (String)element %>" sortable="true"><%= ((JSONObject)metricQueryList.get(metricQuery_rowNum - 1)).getString((String)element) %></display:column> 
    <%
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    %>
    </display:table>
    
<h2>Table: <%= Constants.TRAFODION_REPOS_METRIC_QUERY_AGGR_TABLE %></h2>
<br>
    <jsp:scriptlet> request.setAttribute( "metricQueryAggrList", metricQueryAggrList ); </jsp:scriptlet>
    <display:table name="metricQueryAggrList" id="metricQueryAggr" pagesize="50">
    <%
    if(metricQueryAggrList != null && ! metricQueryAggrList.isEmpty()) {
        try {
            JSONObject aJsonObject = metricQueryAggrList.get(0);
            Iterator itr = aJsonObject.keys(); 
            while(itr.hasNext()) {
                Object element = itr.next();
    %>
                <display:column title="<%= (String)element %>" sortable="true"><%= ((JSONObject)metricQueryAggrList.get(metricQueryAggr_rowNum - 1)).getString((String)element) %></display:column> 
    <%
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
    }            
    %>
    </display:table>
</body>
</html>        