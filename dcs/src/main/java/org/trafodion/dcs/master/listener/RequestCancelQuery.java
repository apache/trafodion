/**
* @@@ START COPYRIGHT @@@

Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.

* @@@ END COPYRIGHT @@@
 */
package org.trafodion.dcs.master.listener;

import java.sql.*;
import java.net.*;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
import java.util.List;
import java.util.ArrayList;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooKeeper;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.*;
import org.trafodion.dcs.zookeeper.ZkClient;

public class RequestCancelQuery {
    private static  final Log LOG = LogFactory.getLog(RequestCancelQuery.class);

    private ZkClient zkc = null;
    private String parentZnode = "";
    
    private int dialogueId = 0;
    private int srvrType = 0;
    private String srvrObjRef = "";
    private int stopType = 0;
    private int port = 0;

    private boolean cancelConnection = false;
    private GetObjRefException exception = null;
    private ByteBuffer header = null;
    private ByteBuffer body = null;
    private Header hdr = null;
    private SocketAddress clientSocketAddress = null;

    RequestCancelQuery(ZkClient zkc,String parentZnode){
        this.zkc = zkc;
        this.parentZnode = parentZnode;
        init ();
    }

    void init (){
        dialogueId = 0;
        srvrType = 0;
        srvrObjRef = "";
        stopType = 0;
        cancelConnection = false;
        exception = null;
        header = null;
        body = null;
        hdr = null;
        clientSocketAddress = null;
    }

    void reset (){
        dialogueId = 0;
        srvrType = 0;
        srvrObjRef = "";
        stopType = 0;
        cancelConnection = false;
        exception = null;
        header = null;
        body = null;
        hdr = null;
        clientSocketAddress = null;
    }

    ClientData processRequest(ClientData clientData, Socket s) { 
        cancelConnection = false;
        exception = new GetObjRefException();
        header = clientData.header;
        body = clientData.body;
        hdr = clientData.hdr;
        clientSocketAddress = clientData.clientSocketAddress;

        try {
// get input
            header.flip();
            hdr.extractFromByteArray(header);
            body.flip();
            dialogueId = body.getInt();
            srvrType = body.getInt();
            srvrObjRef = ByteBufferUtils.extractString(body);
            stopType = body.getInt();
            if(LOG.isDebugEnabled()){
                LOG.debug(clientSocketAddress + ". dialogueId :" + dialogueId);
                LOG.debug(clientSocketAddress + ". srvrType :" + srvrType);
                LOG.debug(clientSocketAddress + ". srvrObjRef :" + srvrObjRef);
                LOG.debug(clientSocketAddress + ". stopType :" + stopType);
            }
            String sPort = srvrObjRef;          //JDBC --- port #
            if (srvrObjRef.startsWith("TCP:")){ //ODBC --- TCP:<IpAddress>/<portNumber>:ODBC
                String[] st = srvrObjRef.split(":");
                String ip[] = st[1].split("/");
                sPort = ip[1];
            } 
            port = Integer.parseInt(sPort);
// process request
            String registeredPath = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED;
            String nodeRegisteredPath = "";
            List<String> servers = null;
            Stat stat = null;
            String data = "";
            boolean found = false;
            String errorText = "";
            Integer nodeId = 0;
            Integer processId = 0;
            
            if (false == registeredPath.startsWith("/"))
                registeredPath = "/" + registeredPath;
    
            zkc.sync(registeredPath,null,null);
            servers =  zkc.getChildren(registeredPath, null);
            if( ! servers.isEmpty()) {
                for(String server : servers) {
                    nodeRegisteredPath = registeredPath + "/" + server;
                    stat = zkc.exists(nodeRegisteredPath,false);
                    if(stat != null){
                        data = new String(zkc.getData(nodeRegisteredPath, false, stat));
                        if (false == data.startsWith("CONNECTED:"))
                            continue;
                        else {
                            String[] stData = data.split(":");
                            if (dialogueId == Long.parseLong(stData[2]) && port == Integer.parseInt(stData[7])){
                                nodeId=Integer.parseInt(stData[3]);
                                processId=Integer.parseInt(stData[4]);
                                found = true;
                                break;
                            }
                        }
                    }
                }
                if (found == true){
                    if(LOG.isDebugEnabled())
                        LOG.debug(clientSocketAddress + ". Server found - dialogueId :" + dialogueId + " port :" + port + " nodeId :" + nodeId + " processId :" + processId);
                        errorText = cancelQuery(nodeId, processId);
                } else {
                    errorText = "Server not found.";
                    if(LOG.isDebugEnabled())
                        LOG.debug(clientSocketAddress + ". Server not found - dialogueId :" + dialogueId + " port :" + port);
               }
             }
// build output
            header.clear();
            body.clear();
            if (false == errorText.equals("")){
                exception.exception_nr = ListenerConstants.DcsMasterParamError_exn;
                exception.ErrorText = errorText;
                if(LOG.isDebugEnabled())
                    LOG.debug(clientSocketAddress + ": " + exception.ErrorText);
            }
            exception.insertIntoByteBuffer(body);
            body.flip();
            hdr.setTotalLength(body.limit());
            hdr.insertIntoByteBuffer(header);
            header.flip();

            clientData.header = header;
            clientData.body = body;
            clientData.hdr = hdr;
        } catch (UnsupportedEncodingException ue){
            if(LOG.isErrorEnabled())
                LOG.error("RequestCancelQuery.UnsupportedEncodingException: " + s.getRemoteSocketAddress() + ": " + ue.getMessage(), ue);
            cancelConnection = true;
        } catch (KeeperException ke){
            if(LOG.isErrorEnabled())
                LOG.error("RequestCancelQuery.KeeperException: " + s.getRemoteSocketAddress() + ": " + ke.getMessage(), ke);
            cancelConnection = true;
        } catch (InterruptedException ie){
            if(LOG.isErrorEnabled())
                LOG.error("RequestCancelQuery.InterruptedException: " + s.getRemoteSocketAddress() + ": " + ie.getMessage(), ie);
            cancelConnection = true;
        } catch (Exception e){
            if(LOG.isErrorEnabled())
                LOG.error("RequestCancelQuery.Exception: " + s.getRemoteSocketAddress() + ": " + e.getMessage(), e);
            cancelConnection = true;
        }
        header = null;
        body = null;
        hdr = null;
        clientSocketAddress = null;
        exception = null;

        if (cancelConnection == true)
            clientData.requestReply = ListenerConstants.REQUST_CLOSE;
        else
            clientData.requestReply = ListenerConstants.REQUST_WRITE_EXCEPTION;
        reset();
        return clientData;
    }
    private static final String queryFormatQid = "select cast(substr(variable_info, position('recentQid:' in variable_info) + 11) as varchar(175) CHARACTER SET UTF8) QUERY_ID  from table (statistics(NULL, 'PROCESS_STATS=%d,CPU=%d'))";
    private static final String queryFormatCancel = "CONTROL QUERY CANCEL QID \"%s\"";

    String cancelQuery(int nodeId, int processId){
        java.sql.Connection conn = null;
        java.sql.Statement stmt1 = null;
        java.sql.Statement stmt2 = null;
        java.sql.ResultSet rs = null; 
        String query = "";
        String errorText = "";
        
        try {
            query = String.format(queryFormatQid, processId, nodeId);
            conn = DriverManager.getConnection(Constants.T2_DRIVER_URL);
            stmt1 = conn.createStatement();
            stmt2 = conn.createStatement();
            rs = stmt1.executeQuery(query);
            if (rs.next()){
                String queryId = rs.getString("QUERY_ID").trim();
                if(LOG.isDebugEnabled())
                    LOG.debug("cancelQuery.queryId :" + queryId);
                if(true == queryId.endsWith("_PUBLICATION")){
                    if(LOG.isDebugEnabled())
                        LOG.debug("cancelQuery: Publication Query - Cancel Query Request is ignored.");
                }
                else {
                    query = String.format(queryFormatCancel, queryId);
                    if(LOG.isDebugEnabled())
                        LOG.debug("cancelQuery.query :" + query);
                    stmt2.execute(query);
                }
            }
            else {
                errorText = "QueryId not found for [" + nodeId + "/" + processId + "]";
                if(LOG.isDebugEnabled())
                    LOG.debug("cancelQuery.errorText :" + errorText);
            }
        } catch (SQLException se){
            SQLException nextException;
            nextException = se;
            StringBuilder sb = new StringBuilder();
            do {
                sb.append(nextException.getMessage());
                sb.append("\nSQLState   " + nextException.getSQLState());
                sb.append("\nError Code " + nextException.getErrorCode());
            } while ((nextException = nextException.getNextException()) != null);
            errorText = sb.toString();
            if(LOG.isDebugEnabled())
                LOG.debug("cancelQuery.SQLException :" + errorText);
        }
        finally {
            if (conn != null)
                try { conn.close(); } catch(SQLException e){ conn = null;}
        }
        return errorText;
    }
}
