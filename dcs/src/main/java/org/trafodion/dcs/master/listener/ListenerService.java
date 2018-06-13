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

import java.io.*;
import java.nio.*;
import java.nio.channels.*;
import java.nio.channels.spi.*;
import java.net.*;
import java.util.*;
import java.util.LinkedList;
import java.util.List;
import java.util.HashMap;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.commons.io.IOUtils;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.GnuParser;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.zookeeper.ZooKeeper;

import org.apache.hadoop.conf.Configuration;

import org.trafodion.dcs.zookeeper.ZkClient;
import org.trafodion.dcs.Constants;
import org.trafodion.dcs.util.DcsConfiguration;
import org.trafodion.dcs.util.DcsNetworkConfiguration;
import org.trafodion.dcs.master.Metrics;

public class ListenerService extends Thread{
    private static  final Log LOG = LogFactory.getLog(ListenerService.class);
    private int selectorTimeout;
    private int requestTimeout;
    private ServerSocketChannel server=null;
    private Selector selector=null;
    private String[] args;
    private ZkClient zkc = null;
    private String parentZnode;
    private int port;
    private int portRange;
    private Configuration conf;
    private DcsNetworkConfiguration netConf;
    public Metrics metrics;
    private ListenerWorker worker=null;
    private List<PendingRequest> pendingChanges = new LinkedList<PendingRequest>();	//list of PendingRequests instances
    private HashMap<SelectionKey, Long> timeouts = new HashMap<SelectionKey, Long>(); // hash map of timeouts

    private void init(){
        if(metrics != null)metrics.initListenerMetrics(System.nanoTime());
        worker = new ListenerWorker(zkc,parentZnode);
        worker.start();
        this.start();
    }

    public ListenerService(String[] args) {
        Options opt = new Options();
        CommandLine cmd;
        try {
            cmd = new GnuParser().parse(opt, args);
        } catch (NullPointerException e) { //No args found
        } catch (ParseException e) { //Could not parse
        }

        this.args = args;

        try {
            this.conf = DcsConfiguration.create();
            this.netConf = new DcsNetworkConfiguration(conf);
            this.requestTimeout = conf.getInt(Constants.DCS_MASTER_LISTENER_REQUEST_TIMEOUT,Constants.DEFAULT_LISTENER_REQUEST_TIMEOUT);
            this.selectorTimeout = conf.getInt(Constants.DCS_MASTER_LISTENER_SELECTOR_TIMEOUT,Constants.DEFAULT_LISTENER_SELECTOR_TIMEOUT);
            this.port = conf.getInt(Constants.DCS_MASTER_PORT,Constants.DEFAULT_DCS_MASTER_PORT);
            this.portRange = conf.getInt(Constants.DCS_MASTER_PORT_RANGE,Constants.DEFAULT_DCS_MASTER_PORT_RANGE);
            this.parentZnode = conf.get(Constants.ZOOKEEPER_ZNODE_PARENT,Constants.DEFAULT_ZOOKEEPER_ZNODE_PARENT);	   	
            this.metrics = null;
            this.zkc = new ZkClient(3000,0,0);
            zkc.connect();
            while (zkc.getZk() == null || zkc.getZk().getState() != ZooKeeper.States.CONNECTED) {
                try {
                    Thread.sleep(10000); //wait 10 seconds
                } catch (Exception e) {	}
                LOG.info("Waiting for ZooKeeper");
            }
            LOG.info("Connected to ZooKeeper");
            init();
        } catch (Exception e){
            LOG.error("Cannot connect to ZooKeeper: " + e.getMessage());
            System.exit(1);
        }
    }

    public ListenerService(ZkClient zkc,DcsNetworkConfiguration netConf,int port,int portRange,int requestTimeout, int selectorTimeout, Metrics metrics, String parentZnode) {
        this.zkc = zkc;
        while (zkc.getZk() == null || zkc.getZk().getState() != ZooKeeper.States.CONNECTED) {
            try {
                Thread.sleep(10000); //wait 10 seconds
            } catch (Exception e) {	}
            LOG.info("Waiting for ZooKeeper");
        }
        LOG.info("Connected to ZooKeeper");
        this.netConf = netConf;
        this.port = port;
        this.portRange = portRange;
        this.requestTimeout = requestTimeout;
        this.selectorTimeout = selectorTimeout;
        this.metrics = metrics;
        this.parentZnode = parentZnode;
        init();
    }

    public void send(PendingRequest preq) {
        synchronized (this.pendingChanges) {
            this.pendingChanges.add(preq);
        }
        this.selector.wakeup();
    }
    
    private int setSelectorTimeout(){
        if (false == timeouts.isEmpty()){
            return selectorTimeout;
        }
        else {
            return 0;
        }
    }

    private static void gc() {
         Object obj = new Object();
         java.lang.ref.WeakReference ref = new java.lang.ref.WeakReference<Object>(obj);
         obj = null;
         while(ref.get() != null) {
             System.gc();
         }
    }

    public void run () {
        try {
            selector = SelectorProvider.provider().openSelector();
            if(LOG.isDebugEnabled())
                LOG.debug("ServerSocketChannel.open()");
            server = ServerSocketChannel.open();
            server.configureBlocking(false);
//          InetSocketAddress isa = new InetSocketAddress(ia, port);
            InetSocketAddress isa = new InetSocketAddress(port); //use any ip address for this port
            server.socket().setReuseAddress(true);
            server.socket().bind(isa);
            SelectionKey serverkey = server.register(selector, SelectionKey.OP_ACCEPT );
            int keysAdded = 0;
            PendingRequest preq = null;

            while(true){
                synchronized (this.pendingChanges) {
                    Iterator<PendingRequest> changes = this.pendingChanges.iterator();
                    while (changes.hasNext()) {
                        preq = changes.next();
                        SelectionKey key = preq.key;
                        int request = preq.request;
                        switch(request){
                        case ListenerConstants.REQUST_WRITE_EXCEPTION:
                            if(metrics != null) metrics.listenerNoAvailableServers();
                        case ListenerConstants.REQUST_WRITE:
                            key.interestOps(key.interestOps() | SelectionKey.OP_WRITE);
                            break;
                        case ListenerConstants.REQUST_CLOSE:
                            try {
                                SocketChannel client = (SocketChannel)key.channel();
                                client.close();
                                ClientData clientData = (ClientData) key.attachment();
                                clientData.header = null;
                                clientData.body = null;
                                clientData.hdr = null;
                                clientData.conectContex = null;
                                clientData.clientSocketAddress = null;
                                key.cancel();
                                if(metrics != null) metrics.listenerRequestRejected();
                            } catch (IOException e) {}
                            break;
                        }
                        preq.key = null;
                    }
                    this.pendingChanges.clear();
                }

                while ((keysAdded = selector.select(setSelectorTimeout())) > 0) {
                    Set<SelectionKey> keys = selector.selectedKeys();
                    Iterator<SelectionKey> i = keys.iterator();

                    while (i.hasNext()) {
                        SelectionKey key = i.next();
                        i.remove();
                        if (!key.isValid()) continue;

                        if ((key.readyOps() & SelectionKey.OP_ACCEPT) == SelectionKey.OP_ACCEPT) {
                            if(LOG.isDebugEnabled())
                                LOG.debug("Ready to process ACCEPT");
                            processAccept(key);
                        } else if ((key.readyOps() & SelectionKey.OP_READ) == SelectionKey.OP_READ) {
                            if(LOG.isDebugEnabled())
                                LOG.debug("Ready to process READ");
                            processRead(key);
                        } else if ((key.readyOps() & SelectionKey.OP_WRITE) == SelectionKey.OP_WRITE) {
                            if(LOG.isDebugEnabled())
                                LOG.debug("Ready to process WRITE");
                            processWrite(key);
                        }
                    }
                }
                if (false == timeouts.isEmpty()){
                    long currentTime = System.currentTimeMillis();
                    Iterator<SelectionKey> i = timeouts.keySet().iterator();
                    while(i.hasNext()){
                      SelectionKey key = i.next();
                      if (currentTime - timeouts.get(key) > requestTimeout){
                          long timeout = (currentTime - timeouts.get(key))/1000;
                          SocketChannel client = (SocketChannel) key.channel();
                          Socket s = client.socket();
                          if ((key.interestOps() & SelectionKey.OP_READ) == SelectionKey.OP_READ){
                              if(LOG.isDebugEnabled())
                                  LOG.debug("Read from client timeouted[" + timeout + " seconds] from: " + s.getRemoteSocketAddress());
                              if(metrics != null) metrics.listenerReadTimeout();
                          }
                          else if ((key.interestOps() & SelectionKey.OP_WRITE) == SelectionKey.OP_WRITE) {
                              if(LOG.isDebugEnabled())
                                  LOG.debug("Write to client timeouted[" + timeout + " seconds] from: " + s.getRemoteSocketAddress());
                              if(metrics != null) metrics.listenerWriteTimeout();
                          }
                          else {
                              if(LOG.isDebugEnabled())
                                  LOG.debug("Client timeouted[" + timeout + " seconds] from: " + s.getRemoteSocketAddress());
                              if(metrics != null) metrics.listenerRequestRejected();
                          }
                          try {
                              client.close();
                          } catch (IOException ex){;}
                          ClientData clientData = (ClientData) key.attachment();
                          clientData.header = null;
                          clientData.body = null;
                          clientData.hdr = null;
                          clientData.conectContex = null;
                          clientData.clientSocketAddress = null;
                          key.attach(null);
                          key.cancel();
                          i.remove();
                        }
                    }
                }
                if (this.isInterrupted()) {
                    throw new InterruptedException();
                }
                //gc();
            }
        } catch (InterruptedException e) {
            LOG.error("ListenerService be interrupted by DCS master, exit thread.", e);
        } catch (IOException e) {
            LOG.error(e);
            System.exit(1);
        } finally {
            LOG.info("close ServerSocketChannel...");
            if (server != null) {
                try {
                    server.socket().close();
                    server.close();
                } catch (IOException e) {
                    LOG.error(e.getMessage(), e);
                }
            }
        }
    }
    private void processAccept(SelectionKey key) {
        try {

            ServerSocketChannel server = (ServerSocketChannel)key.channel();
            SocketChannel client = server.accept();
            Socket s = client.socket();
            // Accept the request
            this.metrics.listenerStartRequest(System.nanoTime());
            if(LOG.isDebugEnabled())
                LOG.debug("Received an incoming connection from: " + s.getRemoteSocketAddress());
            client.configureBlocking( false );
            SelectionKey clientkey = client.register( selector, SelectionKey.OP_READ );
            clientkey.attach(new ClientData(s.getRemoteSocketAddress()));
            if(LOG.isDebugEnabled())
                LOG.debug(s.getRemoteSocketAddress() + ": " + "Accept processed");
        } catch (IOException ie) {
            LOG.error("Cannot Accept connection: " + ie.getMessage());
        }
    }

    private void processRead(SelectionKey key) {

        key.interestOps( key.interestOps() ^ SelectionKey.OP_READ);

        SocketChannel client = (SocketChannel) key.channel();
        Socket s = client.socket();
        long readLength=0;

        ClientData clientData = (ClientData) key.attachment();
        if (false == timeouts.isEmpty() && true == timeouts.containsKey(key)){
            timeouts.remove(key);
        }

        try {
            while ((readLength = client.read(clientData.buf)) > 0) {
                clientData.total_read += readLength;
                if(LOG.isDebugEnabled())
                    LOG.debug(s.getRemoteSocketAddress() + ": " + "Read readLength  " + readLength + "  total " + clientData.total_read);
            }
            if (readLength == -1 ){
                throw new IOException(s.getRemoteSocketAddress() + ": " + "Connection closed by peer on READ");
            }

            if (clientData.total_read < ListenerConstants.HEADER_SIZE){
                key.interestOps(key.interestOps() | SelectionKey.OP_READ);
                key.attach(clientData);
                timeouts.put(key, System.currentTimeMillis());
                if(LOG.isDebugEnabled())
                    LOG.debug(s.getRemoteSocketAddress() + ": " + "Read length less than HEADER size. Added timeout on READ");
                return;
            }

            if (ListenerConstants.BUFFER_INIT == clientData.buffer_state){
                clientData.buffer_state = ListenerConstants.HEADER_PROCESSED;
                ByteBuffer hdr = clientData.buf[0];
                hdr.flip();
                clientData.hdr.extractFromByteArray(hdr);
                if (clientData.hdr.signature_ == ListenerConstants.LITTLE_ENDIAN_SIGNATURE){
                    clientData.switchEndian();
                    hdr = clientData.buf[0];
                    clientData.hdr.extractFromByteArray(hdr);
                }
                if (clientData.hdr.signature_ != ListenerConstants.BIG_ENDIAN_SIGNATURE)
                    throw new IOException("Wrong signature in read Header : " + clientData.hdr.signature_);
            }
            if (clientData.total_read < (clientData.hdr.getTotalLength() + ListenerConstants.HEADER_SIZE)){
                key.interestOps(key.interestOps() | SelectionKey.OP_READ);
                key.attach(clientData);
                timeouts.put(key, System.currentTimeMillis());
                if(LOG.isDebugEnabled())
                    LOG.debug(s.getRemoteSocketAddress() + ": " + "Total length less than in read HEADER. Added timeout on READ");
                return;
            }
            if (clientData.total_read > (clientData.hdr.getTotalLength() + ListenerConstants.HEADER_SIZE)){
                throw new IOException("Wrong total length in read Header : total_read " + clientData.total_read + ", hdr_total_length + hdr_size " + clientData.hdr.getTotalLength() +  + ListenerConstants.HEADER_SIZE);
            }
            key.attach(clientData);
            this.worker.processData(this, key);
            if(LOG.isDebugEnabled())
                LOG.debug(s.getRemoteSocketAddress() + ": " + "Read processed");

        } catch (IOException ie){
            LOG.error("IOException: " + s.getRemoteSocketAddress() + ": " + ie.getMessage());
            try {
                client.close();
            } catch (IOException ex){;}
            clientData = (ClientData) key.attachment();
            clientData.header = null;
            clientData.body = null;
            clientData.hdr = null;
            clientData.conectContex = null;
            clientData.clientSocketAddress = null;
            key.attach(null);
            key.cancel();
            if(metrics != null) metrics.listenerRequestRejected();
        }
    }

    private void processWrite(SelectionKey key) {
    
        key.interestOps(key.interestOps() ^ SelectionKey.OP_WRITE);

        SocketChannel client = (SocketChannel) key.channel();
        Socket s = client.socket();
        long writeLength=0;

        ClientData clientData = (ClientData) key.attachment();
        if (false == timeouts.isEmpty() && true == timeouts.containsKey(key)){
            timeouts.remove(key);
        }
        try {
            while ((writeLength = client.write(clientData.buf)) > 0) {
                clientData.total_write += writeLength;
            }
            if (writeLength == -1 ){
                throw new IOException(s.getRemoteSocketAddress() + ": " + "Connection closed by peer on WRITE");
            }

            if (clientData.buf[0].hasRemaining() || clientData.buf[1].hasRemaining()){
                key.attach(clientData);
                key.interestOps(key.interestOps() | SelectionKey.OP_WRITE);
                timeouts.put(key, System.currentTimeMillis());
                if(LOG.isDebugEnabled())
                    LOG.debug(s.getRemoteSocketAddress() + ": " + "Write length less than total write length. Added timeout on WRITE");
            } else {
                client.close();
                clientData.header = null;
                clientData.body = null;
                clientData.hdr = null;
                clientData.conectContex = null;
                clientData.clientSocketAddress = null;
                key.attach(null);
                key.cancel();
                if(metrics != null)metrics.listenerEndRequest(System.nanoTime());
                if(LOG.isDebugEnabled())
                    LOG.debug(s.getRemoteSocketAddress() + ": " + "Write processed");
            }
        } catch (IOException ie){
            LOG.error("IOException: " + s.getRemoteSocketAddress() + ": " + ie.getMessage());
            try {
                client.close();
            } catch (IOException ex){;}
            clientData.header = null;
            clientData.body = null;
            clientData.hdr = null;
            clientData.conectContex = null;
            clientData.clientSocketAddress = null;
            key.attach(null);
            key.cancel();
            if(metrics != null)metrics.listenerRequestRejected();
        }
    }

    public static void main(String [] args) {
        ListenerService as = new ListenerService(args);
    }

    public ListenerWorker getWorker() {
        return worker;
    }
}
