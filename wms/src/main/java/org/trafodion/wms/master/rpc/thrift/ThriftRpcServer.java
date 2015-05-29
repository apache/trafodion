package org.trafodion.wms.master.rpc.thrift;

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
import org.trafodion.wms.master.WmsMaster;
import org.trafodion.wms.master.rpc.thrift.ThriftRpcHandler;
import org.trafodion.wms.thrift.generated.*;

public class ThriftRpcServer implements Runnable {
	private static final Log LOG = LogFactory.getLog(ThriftRpcServer.class.getName());
	private static final Charset CHARSET = Charset.forName("UTF-8");
	private WmsMaster wmsMaster;
	private Thread thrd;
	private ThriftRpcHandler trpch = null;
//	private WmsService.Processor processor=null;
	
	public ThriftRpcServer(WmsMaster wmsMaster){
		this.wmsMaster = wmsMaster;
		trpch = new ThriftRpcHandler(wmsMaster);
		//processor = new WmsService.Processor(trpch);
		thrd = new Thread(this);
		thrd.start();
	}
	
	public ThriftRpcHandler getHandler() {
		return this.trpch;
	}
	
	public void run() {
		TServer server=null;
		int port = wmsMaster.getThriftPort();
		
		try {
			//TServerTransport serverTransport = new TServerSocket(new InetSocketAddress(port));
			//server = new TSimpleServer(new Args(serverTransport).processor(new WmsService.Processor(trpch)));
			// Use this for a multithreaded server
			//server = new TThreadPoolServer(new TThreadPoolServer.Args(serverTransport).processor(new WmsService.Processor(trpch)));
			// Use this for non blocking server
			TNonblockingServerTransport serverTransport = new TNonblockingServerSocket(new InetSocketAddress(port));
			server = new TNonblockingServer(new TNonblockingServer.Args(serverTransport).processor(new WmsAdminService.Processor(trpch)));
			LOG.info("Thrift RPC listening to [" + wmsMaster.getServerName() + ":" + port + "]");
		} catch (TTransportException e) {
			LOG.error("TTransportException " + e);
			System.exit(-1);
		} 
		
		server.serve();
	}
}
