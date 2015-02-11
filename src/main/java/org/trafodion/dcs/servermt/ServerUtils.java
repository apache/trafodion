/**
 *(C) Copyright 2015 Hewlett-Packard Development Company, L.P.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
    private int serverState=ServerConstants.SERVER_STATE_INIT;
    
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

//================== Next ServerState ==============================================
    public void setNextServerStateAvailable(){
        if (serverState == ServerConstants.SERVER_STATE_CONNECTED){
            updateServerState(ServerConstants.SERVER_STATE_DISCONNECTED);
        }
        serverState = ServerConstants.SERVER_STATE_INIT;
    }
//================== Update Server State ===========================================
    
    public void updateServerState(int serverState) {

        try {
            if (serverState == ServerConstants.SERVER_STATE_INIT){
                updateStateToStarting();
            }
            else if (serverState == ServerConstants.SERVER_STATE_AVAILABLE){
                updateStateToAvailable();
            }
            else if (serverState == ServerConstants.SERVER_STATE_CONNECTED){
                updateStateToConnected();
            }
            else if (serverState == ServerConstants.SERVER_STATE_DISCONNECTED){
                updateStateDisconnected();
            }
            else if (serverState == ServerConstants.SERVER_STATE_CONNECT_FAILED){
                updateStateConnectFailed();
                setNextServerStateAvailable();
            }
            else if (serverState == ServerConstants.SERVER_STATE_CONNECT_REJECTED){
                updateStateConnectRejected();
                setNextServerStateAvailable();
            }
            else if (serverState == ServerConstants.SERVER_STATE_CONNECTING_TIMEOUTED){
                updateStateConnectingTimeouted();
            }
            else if (serverState == ServerConstants.SERVER_STATE_READ_TIMEOUTED){
                updateStateReadTimeouted();
                setNextServerStateAvailable();
            }
            else if (serverState == ServerConstants.SERVER_STATE_WRITE_TIMEOUTED){
                updateStateWriteTimeouted();
                setNextServerStateAvailable();
            }
            else if (serverState == ServerConstants.SERVER_STATE_CLIENT_TIMEOUTED){
                updateStateClientTimeouted();
                setNextServerStateAvailable();
            }
            else if (serverState == ServerConstants.SERVER_STATE_PORTINUSE){
                if (this.serverState != ServerConstants.SERVER_STATE_PORTINUSE){
                    updateStatePortInUse();
                    this.serverState = serverState;
                }
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
        
        LOG.info(server.serverName + ". Update State to [STARTING] from " + convertStateToString(serverState));

        stat = zkc.exists(registeredPath, false);
        if(stat != null) {
            threadRegisteredPath = registeredPath + "/" + server.hostName + ":" + server.instance + ":" + server.serverThread;
            threadRegisteredData = Bytes.toBytes(String.format("STARTING:%s::%d:%s:%s::::::",
                    julianTimestamp(),
                    1,                                  //Nid
                    processId(),
                    server.serverName));
            stat = zkc.exists(threadRegisteredPath, false);
            if(stat != null)
                zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
            else
                zkc.create(threadRegisteredPath,threadRegisteredData,ZooDefs.Ids.OPEN_ACL_UNSAFE,CreateMode.EPHEMERAL);
            serverState = ServerConstants.SERVER_STATE_INIT;
        }
    }
//================= AVAILABLE =======================================================
    public void updateStateToAvailable() throws KeeperException, InterruptedException {

        if(serverState != ServerConstants.SERVER_STATE_INIT) return;
       
        LOG.info(server.serverName + ". Update State to [AVAILABLE] from " + convertStateToString(serverState));
        
        threadRegisteredData = Bytes.toBytes(String.format("AVAILABLE:%s::%d:%s:%s:%s:%d:::::",
                julianTimestamp(),
                1,                                  //Nid
                processId(),
                server.serverName,
                server.hostName,
                server.fport));
        zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
        serverState=ServerConstants.SERVER_STATE_AVAILABLE;
//
// wait for CONNECTING state
//
        threadRegisteredData = zkc.getData(threadRegisteredPath, new RunningWatcher(), stat);
    }
//================== CONNECTED =======================================================
    public void updateStateToConnected() throws KeeperException, InterruptedException {

        if(serverState == ServerConstants.SERVER_STATE_CONNECTED) return;
        
        threadRegisteredData = zkc.getData(threadRegisteredPath, false, stat);
        String s = new String(threadRegisteredData);
        if (true == s.startsWith("CONNECTING:")){
            LOG.info(server.serverName + ". Update State to [CONNECTED] from " + convertStateToString(serverState));
            s=s.replace("CONNECTING:", "CONNECTED:");
            LOG.info(server.serverName + ". Updated state: [" + s + "]");
            zkc.setData(threadRegisteredPath, s.getBytes(), -1);
            serverState=ServerConstants.SERVER_STATE_CONNECTED;
        }
    }
//================== CONNECT_FAILED =======================================================
    public void updateStateConnectFailed() throws KeeperException, InterruptedException {
        
        LOG.info(server.serverName + ". Update State to [CONNECT_FAILED] from " + convertStateToString(serverState));

        threadRegisteredData = Bytes.toBytes(String.format("CONNECT_FAILED:%s::%d:%s:%s:%s:%d:::::",
                julianTimestamp(),
                1,                                  //Nid
                processId(),
                server.serverName,
                server.hostName,
                server.fport));
        
        zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
        try {
            Thread.sleep(30000);
        } catch (InterruptedException e) {  }
    }

//================== CONNECT_REJECTED =======================================================
    public void updateStateConnectRejected() throws KeeperException, InterruptedException {
        
        LOG.info(server.serverName + ". Update State to [CONNECT_REJECTED] from " + convertStateToString(serverState));

        threadRegisteredData = Bytes.toBytes(String.format("CONNECT_REJECTED:%s::%d:%s:%s:%s:%d:::::",
                julianTimestamp(),
                1,                                  //Nid
                processId(),
                server.serverName,
                server.hostName,
                server.fport));
        
        zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
        try {
            Thread.sleep(30000);
        } catch (InterruptedException e) {  }
    }

//================== READ_TIMEOUTED =======================================================
    public void updateStateReadTimeouted() throws KeeperException, InterruptedException {
        
        LOG.info(server.serverName + ". Update State to [READ_TIMEOUTED] from " + convertStateToString(serverState));
        
        threadRegisteredData = Bytes.toBytes(String.format("READ_TIMEOUTED:%s::%d:%s:%s:%s:%d:::::",
                julianTimestamp(),
                1,                                  //Nid
                processId(),
                server.serverName,
                server.hostName,
                server.fport));
        
        zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
        try {
            Thread.sleep(30000);
        } catch (InterruptedException e) {  }
    }

//================== WRITE_TIMEOUTED =======================================================
    public void updateStateWriteTimeouted() throws KeeperException, InterruptedException {
        
        LOG.info(server.serverName + ". Update State to [WRITE_TIMEOUTED] from " + ServerUtils.convertStateToString(serverState));
        
        threadRegisteredData = Bytes.toBytes(String.format("WRITE_TIMEOUTED:%s::%d:%s:%s:%s:%d:::::",
                julianTimestamp(),
                1,                                  //Nid
                processId(),
                server.serverName,
                server.hostName,
                server.fport));
        
        zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
        
        try {
            Thread.sleep(30000);
        } catch (InterruptedException e) {  }
    }
    
//================== CLIENT_TIMEOUTED =======================================================
    public void updateStateClientTimeouted() throws KeeperException, InterruptedException {
        
        LOG.info(server.serverName + ". Update State to [CLIENT_TIMEOUTED] from " + ServerUtils.convertStateToString(serverState));

        threadRegisteredData = Bytes.toBytes(String.format("CLIENT_TIMEOUTED:%s::%d:%s:%s:%s:%d:::::",
                julianTimestamp(),
                1,                                  //Nid
                processId(),
                server.serverName,
                server.hostName,
                server.fport));
        
        zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
        
        try {
            Thread.sleep(30000);
        } catch (InterruptedException e) {  }
    }
//================== PORT_IN_USE =======================================================
    public void updateStatePortInUse() throws KeeperException, InterruptedException   {
        
        LOG.info(server.serverName + ". Update State to [PORT_IN_USE] from "+ ServerUtils.convertStateToString(serverState));
    
        threadRegisteredData = Bytes.toBytes(String.format("PORT_IN_USE:%s::%d:%s:%s:%s:%d:::::",
                julianTimestamp(),
                1,                                  //Nid
                processId(),
                server.serverName,
                server.hostName,
                server.fport));
        
        zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
    }
//================== CONNECTING_TIMEOUTED =======================================================
    public void updateStateConnectingTimeouted() throws KeeperException, InterruptedException   {
        
        LOG.info(server.serverName + ". Update State to [CONNECTING_TIMEOUTED] from " + ServerUtils.convertStateToString(serverState));
    
        threadRegisteredData = Bytes.toBytes(String.format("CONNECTING_TIMEOUTED:%s::%d:%s:%s:%s:%d:::::",
                julianTimestamp(),
                1,                                  //Nid
                processId(),
                server.serverName,
                server.hostName,
                server.fport));
        
        zkc.setData(threadRegisteredPath, threadRegisteredData, -1);
        
        try {
            Thread.sleep(30000);
        } catch (InterruptedException e) {  }
    }
//================== DISCONNECTED =======================================================
    public void updateStateDisconnected() throws KeeperException, InterruptedException {
        
        LOG.info(server.serverName + ". Update State to [DISCONNECTED] from " + convertStateToString(serverState));

        threadRegisteredData = zkc.getData(threadRegisteredPath, false, stat);
        String s = new String(threadRegisteredData);
        s=s.replace("CONNECTED:", "DISCONNECTED:");
        zkc.setData(threadRegisteredPath, s.getBytes(), -1);
        serverState=ServerConstants.SERVER_STATE_CONNECTED;
        
        try {
            Thread.sleep(30000);
        } catch (InterruptedException e) {  }
    }
//=================== Watcher ================================================================   
   class RunningWatcher implements Watcher {
       public void process(WatchedEvent event) {
           if(event.getType() == Event.EventType.NodeDataChanged) {
               LOG.debug(server.serverName + ": Watcher: AVAILABLE znode changed [" + event.toString() + "]");
               try {
                   threadRegisteredData = zkc.getData(event.getPath(), false, stat);
                   String s = new String(threadRegisteredData);
                   LOG.debug(server.serverName + ": Watcher: " + s);
                   if (true == s.startsWith("CONNECTING:")){
//                    timeouts.put(serverkey, System.currentTimeMillis());
                        server.setConnectingTimeout();
                   }
               } catch(KeeperException ke){
                   LOG.debug(server.serverName + ": Watcher KeeperException: " + ke);
               } catch(InterruptedException ie){
                   LOG.debug(server.serverName + ": Watcher InterruptedException: " + ie);
               }
           }
       }
   }
}