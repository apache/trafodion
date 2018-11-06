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
  import="org.apache.hadoop.conf.Configuration"
  import="org.trafodion.dcs.master.DcsMaster"
  import="org.trafodion.dcs.master.RunningServer"
  import="org.trafodion.dcs.master.RegisteredServer"
  import="org.trafodion.dcs.master.ServerItem"
  import="org.trafodion.dcs.util.DcsConfiguration"
  import="org.trafodion.dcs.Constants"
  import="org.trafodion.dcs.util.Bytes"
%>
<% 
  DcsMaster master = (DcsMaster)getServletContext().getAttribute(DcsMaster.MASTER);
  Configuration conf = master.getConfiguration();
  List<ServerItem> serverItemList = null;
  if(master.getServerManager() != null)
    serverItemList = master.getServerManager().getServerItemList();
  String masterServerName = master.getServerName();
  int masterInfoPort = master.getInfoPort();
  String masterIP = master.getNetConf().getExtHostAddress();
  String version = org.trafodion.dcs.util.VersionInfo.getVersion();
  String revision = org.trafodion.dcs.util.VersionInfo.getRevision();
  String buildDate = org.trafodion.dcs.util.VersionInfo.getDate();
  String user = org.trafodion.dcs.util.VersionInfo.getUser();
  Date startTime = new Date(master.getStartTime());
  int port = master.getPort();
  int portRange = master.getPortRange();
  String zkQuorumServers = master.getZKQuorumServersString();
  String zkParentZnode = master.getZKParentZnode(); 
  String metrics = master.getMetrics();
  String trafodionLog = master.getTrafodionLog();
  boolean trafodionLogs = conf.getBoolean(Constants.DCS_MASTER_TRAFODION_LOGS, Constants.DEFAULT_DCS_MASTER_TRAFODION_LOGS);
  boolean trafodionQueryTools = conf.getBoolean(Constants.DCS_MASTER_TRAFODION_QUERY_TOOLS, Constants.DEFAULT_DCS_MASTER_TRAFODION_QUERY_TOOLS);
  String type = request.getParameter("type");
  boolean pageSizeSelected = false;
  String pageSize = request.getParameter("pagesize");
  if(pageSize == null ) {
      //System.out.println("pageSize is null");
  } else if (pageSize.isEmpty()) {
      //System.out.println("pageSize is null"); 
  } else {
      //System.out.println("pageSize=" + pageSize);
      pageSizeSelected = true;
  }
  
  if(pageSizeSelected) {
      session.setAttribute("pagesize", pageSize);
  } else {
      session.setAttribute("pagesize", "All");  
  } 
   
  pageSize = (String)session.getAttribute("pagesize");
  Integer pageSizeInteger = null;
  if(! pageSize.equals("All"))
    try { 
        pageSizeInteger = new Integer(pageSize);
    } catch (NumberFormatException nfe) {
      nfe.printStackTrace();
    }
 //System.out.println("pageSize=" + pageSize + ",pageSizeInteger=" + pageSizeInteger);
%>
<?xml version="1.0" encoding="UTF-8" ?>
<!-- Commenting out DOCTYPE so our blue outline shows on hadoop 0.20.205.0, etc.
     See tail of HBASE-2110 for explaination.
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN" 
  "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd"> 
-->
<html xmlns="http://www.w3.org/1999/xhtml">
<head><meta http-equiv="Content-Type" content="text/html;charset=UTF-8"/>
<title>DcsMaster: <%= masterServerName %></title>
<link rel="stylesheet" type="text/css" href="/static/dcs.css" />
</head>
<body>
<h1 id="page_title">DcsMaster: <%= masterIP %>:<%= masterInfoPort %></h1>
<p id="links_menu">
  <a href="http://<%= masterIP %>:<%= masterInfoPort %>?pagesize=<%= pageSize %>">Home</a>,
  <a href="/logs/">Dcs local logs</a><% if(! trafodionLog.isEmpty()) { %><% if(trafodionLogs) { %>, <a href="/TrafodionLogs/">Trafodion local logs</a><% } %><% if(trafodionQueryTools) { %>, <a href="repository.jsp?type=<%= Constants.TRAFODION_REPOS_CATALOG_SCHEMA %>">Trafodion query tools</a><% } %><% } %>
</p>
<hr id="head_rule" />
<h2>Attributes</h2>
<table id="attributes_table">
<col style="width: 10%;"/>
<col />
<col style="width: 20%;"/>
<tr><th>Attribute Name</th><th>Value</th><th>Description</th></tr>
<tr><td>Version</td><td><%= version %>, <%= revision %></td><td> The version and revision</td></tr>
<tr><td>Compiled</td><td><%= buildDate %>, <%= user %></td><td>When this version was compiled and by whom</td></tr>
<tr><td>Start Time</td><td><%= startTime %></td><td>When this server was started</td></tr>
<tr><td>Listener</td><td><%= port %>:<%= portRange %></td><td>Listener starting port and range</td></tr>
<tr><td>Zookeeper Quorum</td><td><%= zkQuorumServers %></td><td>Addresses of all registered ZK servers</td></tr>
<tr><td>Zookeeper Znode</td><td><%= zkParentZnode %></td><td>Parent ZK znode of this cluster</td></tr>
<tr><td>Metrics</td><td><%= metrics %></td><td>Server Metrics; heap sizes are in megabytes</td></tr>
</table>

<h2>Dcs Servers</h2>

<form name="pagesize">
Show
<select name="selector" 
  OnChange="location.href=pagesize.selector.options[selectedIndex].value">
  <% if(pageSize.equals("All")) { %>
    <option value="servers.jsp?pagesize="All" selected>All</option>
  <% } else { %>
    <option value="servers.jsp?pagesize="All">All</option> 
  <% } %>    
  <% if(pageSize.equals("25")) { %>
    <option value="servers.jsp?pagesize=25" selected>25</option>
  <% } else { %>
    <option value="servers.jsp?pagesize=25">25</option> 
  <% } %>      
  <% if(pageSize.equals("50")) { %>
    <option value="servers.jsp?pagesize=50" selected>50</option>
  <% } else { %>
    <option value="servers.jsp?pagesize=50">50</option> 
  <% } %>   
  <% if(pageSize.equals("100")) { %>
    <option value="servers.jsp?pagesize=100" selected>100</option>
  <% } else { %>
    <option value="servers.jsp?pagesize=100">100</option> 
  <% } %>  
  <% if(pageSize.equals("200")) { %>
    <option value="servers.jsp?pagesize=200" selected>200</option>
  <% } else { %>
    <option value="servers.jsp?pagesize=200">200</option> 
  <% } %>          
</select>
entries 
</form>
<br>
    <jsp:scriptlet> request.setAttribute( "serverItemList", serverItemList ); </jsp:scriptlet>
    <%
    if(pageSize.equals("All")) {
    %>
    <display:table name="serverItemList" id="currow"> 
        <display:column escapeXml="false" title="Host Name" sortable="true" group="1">
		        <a href='http://${currow.ipAddress}:${currow.infoPort}'>${currow.hostname}</a>
		  </display:column>    
        <display:column property="instance" title="Instance" sortable="true" group="2"/>
        <display:column property="startTime" title="Start Time" sortable="true"/>         
        <display:column property="isRegistered" title="Registered" sortable="true"/>
        <display:column property="state" title="State" sortable="true"/>
        <display:column property="nid" title="Node" sortable="true"/>
        <display:column property="pid" title="Process Id" sortable="true"/>
        <display:column property="processName" title="Process Name" sortable="true"/>
        <display:column property="ipAddress" title="Ip Address" sortable="true"/>
        <display:column property="port" title="Port" sortable="true"/>
        <display:column property="clientName" title="Client Name" sortable="true"/>
        <display:column property="clientAppl" title="Client Appl" sortable="true"/>
        <display:column property="clientIpAddress" title="Client Ip Address" sortable="true"/>
        <display:column property="clientPort" title="Client Port" sortable="true"/>    
    </display:table>
    <%
    } else {
    %>
        <display:table name="serverItemList" id="currow" pagesize="<%= pageSizeInteger %>"> 
        <display:column escapeXml="false" title="Host Name" sortable="true" group="1">
		        <a href='http://${currow.ipAddress}:${currow.infoPort}'>${currow.hostname}</a>
		  </display:column>    
        <display:column property="instance" title="Instance" sortable="true" group="2"/>
        <display:column property="startTime" title="Start Time" sortable="true"/>         
        <display:column property="isRegistered" title="Registered" sortable="true"/>
        <display:column property="state" title="State" sortable="true"/>
        <display:column property="nid" title="Node" sortable="true"/>
        <display:column property="pid" title="Process Id" sortable="true"/>
        <display:column property="processName" title="Process Name" sortable="true"/>
        <display:column property="ipAddress" title="Ip Address" sortable="true"/>
        <display:column property="port" title="Port" sortable="true"/>
        <display:column property="clientName" title="Client Name" sortable="true"/>
        <display:column property="clientAppl" title="Client Appl" sortable="true"/>
        <display:column property="clientIpAddress" title="Client Ip Address" sortable="true"/>
        <display:column property="clientPort" title="Client Port" sortable="true"/>    
    </display:table>
    <%
    }  
    %>
<br>
<h2> List all configurations</h2>
<table>
<tr><th>Properties Name</th><th>Value</th></tr>
<%
TreeMap<String, String> sm = new TreeMap<String, String>();
for(Map.Entry<String, String> obj: conf){
	sm.put(obj.getKey(), obj.getValue());
}

for(Map.Entry<String, String> entry: sm.entrySet()){
%>
	<tr><td><%=entry.getKey()%></td> <td> <%=entry.getValue()%></td></tr>	
<%
}
%>
</table>
</body>
</html>        
