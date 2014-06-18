package org.trafodion.wms.server.rpc.thrift;

import java.net.*;
import java.io.IOException;
import java.nio.charset.Charset;

import org.apache.thrift.server.TServer;
import org.apache.thrift.server.TServer.Args;
import org.apache.thrift.server.TSimpleServer;
import org.apache.thrift.server.TThreadPoolServer;
import org.apache.thrift.server.TNonblockingServer;
import org.apache.thrift.transport.TSSLTransportFactory;
import org.apache.thrift.transport.TServerSocket;
import org.apache.thrift.transport.TServerTransport;
import org.apache.thrift.transport.TNonblockingTransport;
import org.apache.thrift.transport.TNonblockingSocket;
import org.apache.thrift.transport.TNonblockingServerTransport;
import org.apache.thrift.transport.TNonblockingServerSocket;
import org.apache.thrift.transport.TTransportException;
import org.apache.thrift.transport.TSSLTransportFactory.TSSLTransportParameters;

import org.apache.hadoop.conf.Configuration;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.trafodion.wms.zookeeper.ZkClient;
import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.Constants;
import org.trafodion.wms.server.WmsServer;
import org.trafodion.wms.server.rpc.thrift.ThriftRpcHandler;
import org.trafodion.wms.thrift.generated.*;

public class ThriftRpcServer implements Runnable {
	private static final Log LOG = LogFactory.getLog(ThriftRpcServer.class.getName());
	private static final Charset CHARSET = Charset.forName("UTF-8");
	private WmsServer wmsServer;
	private Thread thrd;
	private ThriftRpcHandler trpch = null;
//	private WmsService.Processor processor=null;
	
	public ThriftRpcServer(WmsServer wmsServer){
		this.wmsServer = wmsServer;
		trpch = new ThriftRpcHandler(wmsServer);
		//processor = new WmsService.Processor(trpch);
		thrd = new Thread(this);
		thrd.start();
	}
	
	public ThriftRpcHandler getHandler() {
		return this.trpch;
	}
	
	public void run() {
		TServer server=null;
		int port = wmsServer.getThriftPort();
		
		try {
			//TServerTransport serverTransport = new TServerSocket(new InetSocketAddress(port));
			//server = new TSimpleServer(new Args(serverTransport).processor(new WmsService.Processor(trpch)));
			// Use this for a multithreaded server
			//server = new TThreadPoolServer(new TThreadPoolServer.Args(serverTransport).processor(new WmsService.Processor(trpch)));
			// Use this for non blocking server
			TNonblockingServerTransport serverTransport = new TNonblockingServerSocket(new InetSocketAddress(port));
			server = new TNonblockingServer(new TNonblockingServer.Args(serverTransport).processor(new WmsService.Processor(trpch)));
			LOG.info("Thrift RPC listening to [" + wmsServer.getServerName() + ":" + port + "]");
		} catch (TTransportException e) {
			LOG.error("TTransportException " + e);
			System.exit(-1);
		} 
		
		server.serve();
	}
}
