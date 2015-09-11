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
 %>
<% 
  DcsMaster master = (DcsMaster)getServletContext().getAttribute(DcsMaster.MASTER);
  Configuration conf = master.getConfiguration();
  boolean readOnly = conf.getBoolean("dcs.master.ui.readonly", false);
  String masterServerName = master.getServerName();
  int masterInfoPort = master.getInfoPort();
  String trafodionHome = master.getTrafodionHome();
  String type = request.getParameter("type");
%>
<?xml version="1.0" encoding="UTF-8" ?>
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html;charset=UTF-8"/>
<title>Repository: <%= type %></title>
<!--
<link rel="stylesheet" type="text/css" href="/static/dcs.css" />
-->
    <title>Trafodion Query Tools</title>
    <link rel="stylesheet" type="text/css" href="css/stylesheet.css" />
    <link rel="stylesheet" type="text/css" href="jquery-ui/jquery-ui.css" />
    <script type="text/javascript" src="js/lib/jquery-1.11.0.js"></script>
    <script type="text/javascript" src="jquery-ui/jquery-ui.js"> </script> 
        
    <script type="text/javascript">
    $(function() {
        $("#tabs").tabs({
            ajaxOptions: {
                eror: function(xhr, status, index, anchor) {
                    $(anchor.hash).html("Failed to load this tab!");
                }
            }
        });

        $('#tabs div.ui-tabs-panel').height(function() {
            return $('#tabs-container').height()
                   - $('#tabs-container #tabs ul.ui-tabs-nav').outerHeight(true)
                   - ($('#tabs').outerHeight(true) - $('#tabs').height())
                                   // visible is important here, sine height of an invisible panel is 0
                   - ($('#tabs div.ui-tabs-panel:visible').outerHeight(true)  
                      - $('#tabs div.ui-tabs-panel:visible').height());
        });
    });
    </script>

    <style type="text/css">
        .ui-tabs .ui-tabs-panel {
            overflow: auto;
        }
    </style>        
</head>
<body>

        <div class="bannerArea">
            <div class="container1">
                <div style="padding-bottom: 5px; font-weight: bold; font-size: 28px; position: absolute;left:5px;top: 5px; color: #FFF; font-family: 'Palatino Linotype', 'Book Antiqua', Palatino, serif; font-style: italic;">
                    <span id="Label1">Trafodion Query Tools</span>
                </div>
            </div>
        </div>

        <div id="tabs-container" style="height:1000px; border:1px #aaa solid;">
            <div id="tabs">
                <ul>
                    <li><a href="explain.html">Query Plan</a></li>
                    <li><a href="querystats.html">Query Statistics</a></li>
                    <li><a href="sessions.html">Sessions</a></li>
                    <li><a href="aggr_querystats.html">Aggregated Query Statistics</a></li>
                </ul>
            </div>        
        </div>

        <script>
            $(document).ready(function() {
                $("#tabs").tabs().css({'height': '100%','overflow': 'auto'})
            });
        </script>
</body>
</html>        