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

import java.sql.SQLException;
import java.sql.Timestamp;
import java.util.Date;
import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
import java.net.*;
import java.util.*;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooKeeper;

import org.trafodion.dcs.util.*;
import org.trafodion.dcs.Constants;
import org.trafodion.dcs.zookeeper.ZkClient;

class ConnectReply {
    private static  final Log LOG = LogFactory.getLog(ConnectReply.class);
    
    private ZkClient zkc = null;
    private String parentZnode = "";
    
    private GetObjRefException exception = null;
    private Integer dialogueId = 0;
    private String dataSource = "";
    private byte[] userSid;
    private VersionList versionList = null;
    private int isoMapping = 0;
    
    private String serverHostName="";
    private Integer serverNodeId=0;
    private Integer serverProcessId=0;
    private String serverProcessName="";
    private String serverIpAddress="";
    private Integer serverPort=0;
    private Long timestamp=0L;
    private String clusterName = "";
    
    private  Random random = null;
    
    ConnectReply(ZkClient zkc,String parentZnode){
        this.zkc = zkc;
        this.parentZnode = parentZnode;
        exception = new GetObjRefException();
        versionList = new VersionList();
        random = new Random();
    }
    // ----------------------------------------------------------
    void insertIntoByteBuffer(ByteBuffer buf)  throws java.io.UnsupportedEncodingException {
        exception.insertIntoByteBuffer(buf);
        buf.putInt(dialogueId);
        Util.insertString(dataSource,buf);
        Util.insertByteString(userSid,buf);
        versionList.insertIntoByteBuffer(buf);
        buf.putInt(isoMapping);
    
        Util.insertString(serverHostName,buf);
        buf.putInt(serverNodeId);
        buf.putInt(serverProcessId);
        Util.insertString(serverProcessName,buf);
        Util.insertString(serverIpAddress,buf);
        buf.putInt(serverPort);
        buf.putLong(timestamp);
        Util.insertString(clusterName,buf);
    }
    
    boolean buildConnectReply  (Header hdr, ConnectionContext cc, SocketAddress clientSocketAddress ) throws java.io.UnsupportedEncodingException {
    
        boolean replyException = false;
        Integer serverInstance=0;
        String serverTimestamp="";
    
        versionList = new VersionList();
    
        versionList.list[0].componentId = ListenerConstants.DCS_MASTER_COMPONENT;
        versionList.list[0].majorVersion = ListenerConstants.DCS_MASTER_VERSION_MAJOR_1;
        versionList.list[0].minorVersion = ListenerConstants.DCS_MASTER_VERSION_MINOR_0;
        versionList.list[0].buildId	= ListenerConstants.DCS_MASTER_BUILD_1 | ListenerConstants.CHARSET | ListenerConstants.PASSWORD_SECURITY;
    
        versionList.list[1].componentId = ListenerConstants.MXOSRVR_ENDIAN + ListenerConstants.ODBC_SRVR_COMPONENT;
        versionList.list[1].majorVersion = ListenerConstants.MXOSRVR_VERSION_MAJOR;
        versionList.list[1].minorVersion = ListenerConstants.MXOSRVR_VERSION_MINOR;
        versionList.list[1].buildId	= ListenerConstants.MXOSRVR_VERSION_BUILD;
    
        exception.exception_nr=0;
        exception.exception_detail=0;
        exception.ErrorText=null;
    
        Stat stat = null;
        byte[] data = null;
        boolean found = false;
        boolean exceptionThrown = false;
        
        int length = 0;
        int index = 0;
        int maxIndex = -1;
        int randomPicks = 0;
        String server = "";
        
        try {
            String registeredPath = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_SERVERS_REGISTERED;
            String nodeRegisteredPath = "";
            
            if (false == registeredPath.startsWith("/"))
                registeredPath = "/" + registeredPath;

            zkc.sync(registeredPath,null,null);
            List<String> servers = zkc.getChildren(registeredPath, null);
            length = servers.size();
            if (length == 0){
                throw new IOException("No Available Servers - length is 0");
            }
            if(LOG.isDebugEnabled()){
                int dbgLength = length > 4 ? 4 : length;
                for (int i = 0; i < dbgLength; i++){
                    LOG.debug(clientSocketAddress + ": " + i + " server " + servers.get(i) );
                }
            }
            switch(length) {
                    case 1: randomPicks = 1;break;
                    case 2: randomPicks = 1;break;
                    case 3: case 4: randomPicks = 2;break;
                    default:randomPicks = 3;
            }
            if(LOG.isDebugEnabled())
                LOG.debug(clientSocketAddress + ": " + "randomPicks " + randomPicks + ", length " + length );
            
            int[] indexArr = new int[length];
            Arrays.fill(indexArr, 0, length - 1, -1);
            indexArr[0] = -1;
//
// pick randomly the AVAILABLE server 
//                
            for(int i=0; i < randomPicks; i++){
                while(true){
                    index = random.nextInt();
                    index = index > 0? index : -index;
                    index %= length;
                    if (indexArr[index] != index) break;
                }
                indexArr[index] = index;
                maxIndex = index > maxIndex ? index : maxIndex;
                server = servers.get(index);
                if(LOG.isDebugEnabled())
                    LOG.debug(clientSocketAddress + ": " + " index " + index + " server picked " + server );
                
                nodeRegisteredPath = registeredPath + "/" + server;
                stat = zkc.exists(nodeRegisteredPath,false);
                if(stat != null){
                    data = zkc.getData(nodeRegisteredPath, false, stat);
                    if (false == (new String(data)).startsWith("AVAILABLE:"))
                        continue;
                    else {
                        found = true;
                        break;
                    }
                }
                else
                    continue;
            }
//
// search sequentially for AVAILABLE server starting from highest random index + 1 to length
//
            if (found == false){
                for(index=maxIndex+1; index<length; index++){
                    if (indexArr[index] != index){
                        server = servers.get(index);
                        if(LOG.isDebugEnabled())
                            LOG.debug(clientSocketAddress + ": " + "server selected in search 1 " + server );
                            
                        nodeRegisteredPath = registeredPath + "/" + server;
                        stat = zkc.exists(nodeRegisteredPath,false);
                        if(stat != null){
                            data = zkc.getData(nodeRegisteredPath, false, stat);
                            if (false == (new String(data)).startsWith("AVAILABLE:"))
                                continue;
                            else {
                                found = true;
                                break;
                            }
                        }
                        else
                            continue;
                    }
                }
            }
//
// search sequentially for AVAILABLE server starting from index 0 to max random index - 1
//
            if (found == false){
                for(index=0; index<maxIndex; index++){
                    if (indexArr[index] != index){
                        server = servers.get(index);
                        if(LOG.isDebugEnabled())
                            LOG.debug(clientSocketAddress + ": " + "server selected in search 2 " + server );
                            
                        nodeRegisteredPath = registeredPath + "/" + server;
                        stat = zkc.exists(nodeRegisteredPath,false);
                        if(stat != null){
                            data = zkc.getData(nodeRegisteredPath, false, stat);
                            if (false == (new String(data)).startsWith("AVAILABLE:"))
                                continue;
                            else {
                                found = true;
                                break;
                            }
                        }
                        else
                            continue;
                    }
                }
            }
            if (found == true){

                String[] stNode = server.split(":");
                serverHostName=stNode[0];
                serverInstance=Integer.parseInt(stNode[1]);
                
                String[] stData = (new String(data)).split(":");
                timestamp=Long.parseLong(stData[1]);
                serverNodeId=Integer.parseInt(stData[3]);
                serverProcessId=Integer.parseInt(stData[4]);
                serverProcessName=stData[5];
                serverIpAddress=stData[6];
                serverPort=Integer.parseInt(stData[7]);
                
                if(LOG.isDebugEnabled()){
                    LOG.debug(clientSocketAddress + ": " + "serverHostName " + serverHostName );
                    LOG.debug(clientSocketAddress + ": " + "serverInstance " + serverInstance );
                    LOG.debug(clientSocketAddress + ": " + "serverNodeId " + serverNodeId );
                    LOG.debug(clientSocketAddress + ": " + "serverProcessId " + serverProcessId);
                    LOG.debug(clientSocketAddress + ": " + "serverProcessName " + serverProcessName );
                    LOG.debug(clientSocketAddress + ": " + "serverIpAddress " + serverIpAddress );
                    LOG.debug(clientSocketAddress + ": " + "serverPort " + serverPort );
                    LOG.debug(clientSocketAddress + ": " + "timestamp " + timestamp );
                }
                
                dialogueId = random.nextInt();
                dialogueId = (dialogueId < 0 )? -dialogueId : dialogueId;
                if(LOG.isDebugEnabled())
                    LOG.debug(clientSocketAddress + ": " + "dialogueId: " + dialogueId);
                data = Bytes.toBytes(String.format("CONNECTING:%d:%d:%d:%d:%s:%s:%d:%s:%s:%s:",
                        timestamp,
                        dialogueId, 
                        serverNodeId,
                        serverProcessId,
                        serverProcessName,
                        serverIpAddress,
                        serverPort,
                        cc.computerName, 
                        clientSocketAddress, 
                        cc.windowText ));
                zkc.setData(nodeRegisteredPath, data, -1);
            }
        } catch (KeeperException.NodeExistsException e) {
            LOG.error(clientSocketAddress + ": " + "do nothing...some other server has created znodes: " + e.getMessage());
            exceptionThrown = true;
        } catch (KeeperException e) {
            LOG.error(clientSocketAddress + ": " + "KeeperException: " + e.getMessage());
            exceptionThrown = true;
        } catch (InterruptedException e) {
            LOG.error(clientSocketAddress + ": " + "InterruptedException: " + e.getMessage());
            exceptionThrown = true;
        } catch (IOException ie){
            LOG.error(clientSocketAddress + ": " + ie.getMessage());
            exceptionThrown = true;
        }
        if (found == false || exceptionThrown == true){
            exception.exception_nr = ListenerConstants.DcsMasterNoSrvrHdl_exn; //no available servers
            replyException = true;
            if(LOG.isDebugEnabled()){
                if (found == false)
                    LOG.info(clientSocketAddress + ": " + "No Available Servers");
                else
                    LOG.info(clientSocketAddress + ": " + "No Available Servers - exception thrown");
            }
        } else {
            
            if (cc.datasource.length() == 0)
                dataSource = "TDM_Default_DataSource";
            else
                dataSource = cc.datasource;

            if(LOG.isDebugEnabled()){
                LOG.debug(clientSocketAddress + ": " + "userName: " + cc.user.userName);
                LOG.debug(clientSocketAddress + ": " + "password: XXXXXX");
                LOG.debug(clientSocketAddress + ": " + "client: " + cc.client);
                LOG.debug(clientSocketAddress + ": " + "location: " + cc.location);
                LOG.debug(clientSocketAddress + ": " + "windowText: " + cc.windowText);
                LOG.debug(clientSocketAddress + ": " + "dataSource: " + dataSource);
                LOG.debug(clientSocketAddress + ": " + "client computer name:ipaddress:port " + cc.computerName+ ":" + clientSocketAddress);
            }

            userSid = new String(cc.user.userName).getBytes("UTF-8");

            isoMapping = 0;
            StringTokenizer st = new StringTokenizer(serverHostName, ".");
            if(st.hasMoreTokens()) {
                clusterName = String.format("%s_%s", st.nextToken(), cc.client);
            }
            else {
                clusterName = String.format("%s_%s", serverHostName, cc.client);
            }
            if(LOG.isDebugEnabled())
                LOG.debug(clientSocketAddress + ": " + "clusterName " + clusterName );
        }
    return replyException;
    }
}
