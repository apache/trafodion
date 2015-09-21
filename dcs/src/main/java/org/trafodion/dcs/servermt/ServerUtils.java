/**
* @@@ START COPYRIGHT @@@

* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at

*   http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.

* @@@ END COPYRIGHT @@@
 */
package org.trafodion.dcs.servermt;

import java.util.*;
import java.nio.*;
import java.io.*;
import java.sql.*;
import java.util.Calendar;
import java.lang.management.ManagementFactory;
import org.trafodion.dcs.util.*;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.Watcher;
import org.apache.zookeeper.WatchedEvent;
import org.apache.zookeeper.KeeperException;

import org.trafodion.dcs.Constants;
import org.trafodion.dcs.zookeeper.ZkClient;

import org.trafodion.dcs.servermt.serverHandler.*;

public final class ServerUtils  {
    private static  final Log LOG = LogFactory.getLog(ServerUtils.class);
    
    private static final Double julianOffset = 210866803200000000.0;
    private ServerHandler server;
    
    private ZkClient zkc;
    private String registeredPath = "";
    private String threadRegisteredPath = "";
    private byte[] threadRegisteredData = null;
    private Stat stat;

    public ServerUtils(){
        server = null;
        zkc = null;
    }
    public ServerUtils(ServerHandler server, ZkClient zkc){
        this.server = server;
        this.zkc = zkc;
        
        init();
    }
    
    private void init(){
    
        registeredPath = server.parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED;
        if (false == registeredPath.startsWith("/"))
            registeredPath = "/" + registeredPath;
         zkc.sync(registeredPath,null,null);
    }
  
    public static String julianTimestamp() {
        Calendar calendar = Calendar.getInstance();
        java.sql.Timestamp currentTimestamp = new java.sql.Timestamp(calendar.getTime().getTime());
        Double fvalue = currentTimestamp.getTime() * 1000 + julianOffset;
        return String.format("%18.0f",fvalue);
    }
    
    public static String processId() {
        return ManagementFactory.getRuntimeMXBean().getName().split("@")[0];
    }
    
    public String getThreadRegisteredPath(){
        return threadRegisteredPath;
    }
    
    public String getThreadRegisteredData(){
        return new String(threadRegisteredData);
    }

    public static String convertStateToString(int serverState){
        String stServerState = "";
        
        switch(serverState){
        case ServerConstants.SERVER_STATE_INIT:stServerState = "[INIT]";break;
        case ServerConstants.SERVER_STATE_AVAILABLE:stServerState = "[AVAILABLE]";break;
        case ServerConstants.SERVER_STATE_CONNECTING:stServerState = "[CONNECTING]";break;
        case ServerConstants.SERVER_STATE_CONNECTED:stServerState = "[CONNECTED]";break;
        case ServerConstants.SERVER_STATE_CONNECT_FAILED:stServerState = "[CONNECT_FAILED]";break;
        case ServerConstants.SERVER_STATE_CONNECT_REJECTED:stServerState = "[CONNECT_REJECTED]";break;
        case ServerConstants.SERVER_STATE_READ_TIMEOUTED:stServerState = "[READ_TIMEOUTED]";break;
        case ServerConstants.SERVER_STATE_WRITE_TIMEOUTED:stServerState = "[WRITE_TIMEOUTED]";break;
        case ServerConstants.SERVER_STATE_CLIENT_TIMEOUTED:stServerState = "[CLIENT_TIMEOUTED]";break;
        case ServerConstants.SERVER_STATE_DISCONNECTED:stServerState = "[DISCONNECTED]";break;
        case ServerConstants.SERVER_STATE_PORTINUSE:stServerState = "[PORT_IN_USE]";break;
        default: stServerState = "Unknown [" + serverState + "]";
        }
        return stServerState;
    }

    public static String convertRequestToString(int request){
        String stRequest = "";
        
        switch(request){
        case ServerConstants.REQUST_INIT:stRequest = "[INIT]";break;
        case ServerConstants.REQUST_CLOSE:stRequest = "[CLOSE]";break;
        case ServerConstants.REQUST_WRITE_READ:stRequest = "[WRITE_READ]";break;
        case ServerConstants.REQUST_WRITE_CLOSE:stRequest = "[WRITE_CLOSE]";break;
        default: stRequest = "UNKNOWN REQUEST :[" + request + "]";
        }
        return stRequest;
    }
    
    public static String convertOpIdToString(int opId){

        String stOpId = "";
        
        switch (opId){
        case ServerConstants.SRVR_API_INIT:stOpId = "[INIT]"; break;
        case ServerConstants.SRVR_API_SQLCONNECT:stOpId = "[SQLCONNECT]"; break;
        case ServerConstants.SRVR_API_SQLDISCONNECT:stOpId = "[SQLDISCONNECT]"; break;
        case ServerConstants.SRVR_API_SQLSETCONNECTATTR:stOpId = "[SQLSETCONNECTATTR]"; break;
        case ServerConstants.SRVR_API_SQLENDTRAN:stOpId = "[SQLENDTRAN]"; break;
        case ServerConstants.SRVR_API_SQLPREPARE:stOpId = "[SQLPREPARE]"; break;
        case ServerConstants.SRVR_API_SQLEXECUTE_ROWSET:stOpId = "[SQLEXECUTE_ROWSET]"; break;
        case ServerConstants.SRVR_API_SQLEXECDIRECT_ROWSET:stOpId = "[SQLEXECDIRECT_ROWSET]"; break;
        case ServerConstants.SRVR_API_SQLEXECDIRECT:stOpId = "[SQLEXECDIRECT]"; break;
        case ServerConstants.SRVR_API_SQLEXECUTE:stOpId = "[SQLEXECUTE]"; break;
        case ServerConstants.SRVR_API_SQLEXECUTECALL:stOpId = "[SQLEXECUTECALL]"; break;
        case ServerConstants.SRVR_API_SQLEXECUTE2:stOpId = "[SQLEXECUTE2]"; break;
        case ServerConstants.SRVR_API_SQLFETCH:stOpId = "[SQLFETCH]"; break;
        case ServerConstants.SRVR_API_SQLFREESTMT:stOpId = "[SQLFREESTMT]"; break;
        case ServerConstants.SRVR_API_GETCATALOGS:stOpId = "[GETCATALOGS]"; break;
        case ServerConstants.SRVR_API_STOPSRVR:stOpId = "[STOPSRVR]"; break;
        case ServerConstants.SRVR_API_ENABLETRACE:stOpId = "[ENABLETRACE]"; break;
        case ServerConstants.SRVR_API_DISABLETRACE:stOpId = "[DISABLETRACE]"; break;
        case ServerConstants.SRVR_API_ENABLE_SERVER_STATISTICS:stOpId = "[ENABLE_SERVER_STATISTICS]"; break;
        case ServerConstants.SRVR_API_DISABLE_SERVER_STATISTICS:stOpId = "[DISABLE_SERVER_STATISTICS]"; break;
        case ServerConstants.SRVR_API_UPDATE_SERVER_CONTEXT:stOpId = "[UPDATE_SERVER_CONTEXT]"; break;
        default:
            stOpId = "UNKNOWN OPERATION_ID :[" + opId + "]";
        }
        return stOpId;
    }

    public static String convertFreeResourceOptToString(int freeResourceOpt){

        String stOpId = "";
        
        switch( freeResourceOpt )
        {
        case ServerConstants.SQL_DROP: stOpId = "[SQL_DROP]"; break;
        case ServerConstants.SQL_CLOSE: stOpId = "[SQL_CLOSE]"; break;
        case ServerConstants.SQL_UNBIND: stOpId = "[SQL_UNBIND]"; break;
        case ServerConstants.SQL_RESET_PARAMS: stOpId = "[SQL_RESET_PARAMS]"; break;
        case ServerConstants.SQL_REALLOCATE: stOpId = "[SQL_REALLOCATE]"; break;
        default:
            stOpId = "UNKNOWN OPERATION_ID :[" + freeResourceOpt + "]";
        }
        return stOpId;
    }

    //================= AVAILABLE =======================================================
    public void updateServerStateAvailable() throws KeeperException, InterruptedException {

        if(LOG.isDebugEnabled())
            LOG.debug(server.serverName + ". Update State to [AVAILABLE]");
        
        threadRegisteredData = Bytes.toBytes(String.format("AVAILABLE:%s::%d:%s:%s:%s:%d:::::",
                julianTimestamp(),
                1,                                  //Nid
                processId(),
                server.serverName,
                server.hostName,
                server.fport));
        zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
        zkc.sync(threadRegisteredPath,null,null);
//
// wait for CONNECTING state
//
        threadRegisteredData = zkc.getData(threadRegisteredPath, new RunningWatcher(), stat);
        
    }
//================== Update Server State ===========================================
    
    public void updateServerState(int serverState) {

        try {
            if (serverState == ServerConstants.SERVER_STATE_INIT){
                updateStateToStarting();
            }
            else if (serverState == ServerConstants.SERVER_STATE_CONNECTED){
                updateStateToConnected();
            }
            else if (serverState == ServerConstants.SERVER_STATE_DISCONNECTED){
                updateStateDisconnected();
                updateServerStateAvailable();
            }
            else if (serverState == ServerConstants.SERVER_STATE_CONNECT_FAILED){
                updateStateConnectFailed();
                updateServerStateAvailable();
            }
            else if (serverState == ServerConstants.SERVER_STATE_CONNECT_REJECTED){
                updateStateConnectRejected();
                updateServerStateAvailable();
            }
            else if (serverState == ServerConstants.SERVER_STATE_CONNECTING_TIMEOUTED){
                updateStateConnectingTimeouted();
                updateServerStateAvailable();
            }
            else if (serverState == ServerConstants.SERVER_STATE_READ_TIMEOUTED){
                updateStateReadTimeouted();
                updateServerStateAvailable();
            }
            else if (serverState == ServerConstants.SERVER_STATE_WRITE_TIMEOUTED){
                updateStateWriteTimeouted();
                updateServerStateAvailable();
            }
            else if (serverState == ServerConstants.SERVER_STATE_CLIENT_TIMEOUTED){
                updateStateClientTimeouted();
                updateServerStateAvailable();
            }
            else if (serverState == ServerConstants.SERVER_STATE_PORTINUSE){
                updateStatePortInUse();
            }
        }
        catch(KeeperException ke){
            LOG.error(server.serverName + ". KeeperException: " + ke.getMessage());
        }
        catch(InterruptedException ie){
            LOG.error(server.serverName + ". InterruptedException: " + ie.getMessage());
        }
    }
//================= STARTING =======================================================
    public void updateStateToStarting() throws KeeperException, InterruptedException {
        
        if(LOG.isDebugEnabled())
            LOG.debug(server.serverName + ". Update State to [AVAILABLE]");

        stat = zkc.exists(registeredPath, false);
        if(stat != null) {
            threadRegisteredPath = registeredPath + "/" + server.hostName + ":" + server.instance + ":" + server.serverThread;
            threadRegisteredData = Bytes.toBytes(String.format("AVAILABLE:%s::%d:%s:%s:%s:%d:::::",
                    julianTimestamp(),
                    1,                                  //Nid
                    processId(),
                    server.serverName,
                    server.hostName,
                    server.fport));
            stat = zkc.exists(threadRegisteredPath, false);
            if(stat != null)
                zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
            else
                zkc.create(threadRegisteredPath,threadRegisteredData,ZooDefs.Ids.OPEN_ACL_UNSAFE,CreateMode.EPHEMERAL);
            zkc.sync(threadRegisteredPath,null,null);
        }
    }
//================== CONNECTED =======================================================
    public void updateStateToConnected() throws KeeperException, InterruptedException {

        threadRegisteredData = zkc.getData(threadRegisteredPath, false, stat);
        String s = new String(threadRegisteredData);
        if (true != s.startsWith("CONNECTING:")){
            while (true){
                zkc.sync(threadRegisteredPath,null,null);
                threadRegisteredData = zkc.getData(threadRegisteredPath, false, stat);
                s = new String(threadRegisteredData);
                if ( s.startsWith("CONNECTING:")){
                    break;
                }
                Thread.currentThread().sleep(1000);
            }
        }
        if(LOG.isDebugEnabled())
            LOG.debug(server.serverName + ". Update State to [CONNECTED]");
        s=s.replace("CONNECTING:", "CONNECTED:");
        zkc.setData(threadRegisteredPath, s.getBytes(), -1);
        zkc.sync(threadRegisteredPath,null,null);
    }
//================== CONNECT_FAILED =======================================================
    public void updateStateConnectFailed() throws KeeperException, InterruptedException {
        
        if(LOG.isDebugEnabled())
            LOG.debug(server.serverName + ". Update State to [CONNECT_FAILED]");

        threadRegisteredData = Bytes.toBytes(String.format("CONNECT_FAILED:%s::%d:%s:%s:%s:%d:::::",
                julianTimestamp(),
                1,                                  //Nid
                processId(),
                server.serverName,
                server.hostName,
                server.fport));
        
        zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
        zkc.sync(threadRegisteredPath,null,null);
        try {
            Thread.sleep(ServerConstants.SERVER_STATUS_DELAY);
        } catch (InterruptedException e) {  }
    }

//================== CONNECT_REJECTED =======================================================
    public void updateStateConnectRejected() throws KeeperException, InterruptedException {
        
        if(LOG.isDebugEnabled())
            LOG.debug(server.serverName + ". Update State to [CONNECT_REJECTED]");

        threadRegisteredData = Bytes.toBytes(String.format("CONNECT_REJECTED:%s::%d:%s:%s:%s:%d:::::",
                julianTimestamp(),
                1,                                  //Nid
                processId(),
                server.serverName,
                server.hostName,
                server.fport));
        
        zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
        zkc.sync(threadRegisteredPath,null,null);
        try {
            Thread.sleep(ServerConstants.SERVER_STATUS_DELAY);
        } catch (InterruptedException e) {  }
    }

//================== READ_TIMEOUTED =======================================================
    public void updateStateReadTimeouted() throws KeeperException, InterruptedException {
        
        if(LOG.isDebugEnabled())
            LOG.debug(server.serverName + ". Update State to [READ_TIMEOUTED]");
        
        threadRegisteredData = Bytes.toBytes(String.format("READ_TIMEOUTED:%s::%d:%s:%s:%s:%d:::::",
                julianTimestamp(),
                1,                                  //Nid
                processId(),
                server.serverName,
                server.hostName,
                server.fport));
        
        zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
        zkc.sync(threadRegisteredPath,null,null);
        try {
            Thread.sleep(ServerConstants.SERVER_STATUS_DELAY);
        } catch (InterruptedException e) {  }
    }

//================== WRITE_TIMEOUTED =======================================================
    public void updateStateWriteTimeouted() throws KeeperException, InterruptedException {
        
        if(LOG.isDebugEnabled())
            LOG.debug(server.serverName + ". Update State to [WRITE_TIMEOUTED]");
        
        threadRegisteredData = Bytes.toBytes(String.format("WRITE_TIMEOUTED:%s::%d:%s:%s:%s:%d:::::",
                julianTimestamp(),
                1,                                  //Nid
                processId(),
                server.serverName,
                server.hostName,
                server.fport));
        
        zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
        zkc.sync(threadRegisteredPath,null,null);
        try {
            Thread.sleep(ServerConstants.SERVER_STATUS_DELAY);
        } catch (InterruptedException e) {  }
    }
    
//================== CLIENT_TIMEOUTED =======================================================
    public void updateStateClientTimeouted() throws KeeperException, InterruptedException {
        
        if(LOG.isDebugEnabled())
            LOG.debug(server.serverName + ". Update State to [CLIENT_TIMEOUTED]");

        threadRegisteredData = Bytes.toBytes(String.format("CLIENT_TIMEOUTED:%s::%d:%s:%s:%s:%d:::::",
                julianTimestamp(),
                1,                                  //Nid
                processId(),
                server.serverName,
                server.hostName,
                server.fport));
        
        zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
        zkc.sync(threadRegisteredPath,null,null);
        try {
            Thread.sleep(ServerConstants.SERVER_STATUS_DELAY);
        } catch (InterruptedException e) {  }
    }
//================== PORT_IN_USE =======================================================
    public void updateStatePortInUse() throws KeeperException, InterruptedException   {
        
        if(LOG.isDebugEnabled())
            LOG.debug(server.serverName + ". Update State to [PORT_IN_USE]");
    
        threadRegisteredData = Bytes.toBytes(String.format("PORT_IN_USE:%s::%d:%s:%s:%s:%d:::::",
                julianTimestamp(),
                1,                                  //Nid
                processId(),
                server.serverName,
                server.hostName,
                server.fport));
        
        zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
        zkc.sync(threadRegisteredPath,null,null);
    }
//================== CONNECTING_TIMEOUTED =======================================================
    public void updateStateConnectingTimeouted() throws KeeperException, InterruptedException   {
        
        if(LOG.isDebugEnabled())
            LOG.debug(server.serverName + ". Update State to [CONNECTING_TIMEOUTED]");
    
        threadRegisteredData = Bytes.toBytes(String.format("CONNECTING_TIMEOUTED:%s::%d:%s:%s:%s:%d:::::",
                julianTimestamp(),
                1,                                  //Nid
                processId(),
                server.serverName,
                server.hostName,
                server.fport));
        
        zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
        zkc.sync(threadRegisteredPath,null,null);
        try {
            Thread.sleep(ServerConstants.SERVER_STATUS_DELAY);
        } catch (InterruptedException e) {  }
    }
//================== DISCONNECTED =======================================================
    public void updateStateDisconnected() throws KeeperException, InterruptedException {
        
        if(LOG.isDebugEnabled())
            LOG.debug(server.serverName + ". Update State to [DISCONNECTED]");

        threadRegisteredData = zkc.getData(threadRegisteredPath, false, stat);
        String s = new String(threadRegisteredData);
        s=s.replace("CONNECTED:", "DISCONNECTED:");
        zkc.setData(threadRegisteredPath, s.getBytes(), -1);
        zkc.sync(threadRegisteredPath,null,null);
        try {
            Thread.sleep(ServerConstants.SERVER_STATUS_DELAY);
        } catch (InterruptedException e) {  }
    }
//=================== Check server state =====================================================
    public boolean checkServerState(int serverState) throws KeeperException, InterruptedException {
        
        threadRegisteredData = zkc.getData(threadRegisteredPath, false, stat);
        String s = new String(threadRegisteredData);
        if(LOG.isDebugEnabled())
            LOG.debug(server.serverName + ". Check Server State : " + s);
        if (serverState == ServerConstants.SERVER_STATE_CONNECTING ){
            return s.startsWith("CONNECTING:");
        }
        return false;
    }
//=================== Watcher ================================================================   
   class RunningWatcher implements Watcher {
       public void process(WatchedEvent event) {
           if(event.getType() == Event.EventType.NodeDataChanged) {
               if(LOG.isDebugEnabled()){
                   LOG.debug(server.serverName + ": " + event.toString());
               }
               try {
                   threadRegisteredData = zkc.getData(event.getPath(), false, stat);
                   String s = new String(threadRegisteredData);
                   if (true == s.startsWith("CONNECTING:")){
                       if(LOG.isDebugEnabled())
                           LOG.debug(server.serverName + ". Update State to [CONNECTING] timeout set to :" + server.getConnectingTimeout());
                       server.setConnectingTimeout();
                   }
               } catch(KeeperException ke){
                   if(LOG.isDebugEnabled())
                       LOG.debug(server.serverName + ": Watcher KeeperException: " + ke);
               } catch(InterruptedException ie){
                   if(LOG.isDebugEnabled())
                       LOG.debug(server.serverName + ": Watcher InterruptedException: " + ie);
               }
           }
       }
   }
}