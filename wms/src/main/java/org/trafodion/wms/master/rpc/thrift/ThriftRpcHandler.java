package org.trafodion.wms.master.rpc.thrift;

import java.net.*;
import java.util.*; 
import java.io.Writer;
import java.io.StringWriter;
import java.io.PrintWriter;
 
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.log4j.Logger;

import org.apache.thrift.TSerializer;
import org.apache.thrift.TDeserializer;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.apache.thrift.protocol.TProtocolFactory;
import org.apache.thrift.transport.TIOStreamTransport;
import org.apache.thrift.transport.TTransport;

import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.KeeperException;

import org.trafodion.wms.zookeeper.ZkClient;
import org.trafodion.wms.master.WmsMaster;
import org.trafodion.wms.master.ServerManager;
import org.trafodion.wms.script.ScriptManager;
import org.trafodion.wms.Constants;
import org.trafodion.wms.cep.ComplexEventProcessor;
import org.trafodion.wms.thrift.generated.WmsAdminService;
import org.trafodion.wms.thrift.generated.IOError;
import org.trafodion.wms.thrift.generated.IllegalArgument;
import org.trafodion.wms.thrift.generated.Stream;
import org.trafodion.wms.thrift.generated.StreamResponse;
import org.trafodion.wms.thrift.generated.Rule;
import org.trafodion.wms.thrift.generated.RuleResponse;
import org.trafodion.wms.thrift.generated.WorkloadResponse;
import org.trafodion.wms.thrift.generated.Request;

public class ThriftRpcHandler implements WmsAdminService.Iface {
	private static final Log LOG = LogFactory.getLog(ThriftRpcHandler.class.getName());
	private WmsMaster wmsMaster;
	private long beginTs;
	private long endTs;
	private long totalTime;
	private ZkClient zkc;
	private String parentZnode;   
	private ComplexEventProcessor cep;
	private TSerializer serializer = new TSerializer(new TBinaryProtocol.Factory());
    private TDeserializer deserializer = new TDeserializer(new TBinaryProtocol.Factory());
	
	public ThriftRpcHandler(WmsMaster wmsMaster){
		this.wmsMaster = wmsMaster;
		this.zkc = wmsMaster.getZkClient();
		this.parentZnode = wmsMaster.getZKParentZnode();
		this.cep = wmsMaster.getServerManager().getComplexEventProcessor(); 
	}
	
	public long ping(long timestamp) throws IOError {
	   LOG.debug("ping [" + timestamp + "]");
	   return System.currentTimeMillis();
	}
	
	public void addStream(Stream stream) throws IOError, IllegalArgument  {
		LOG.debug("addStream [" + stream.toString() + "]");
		
		String streamZnode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS + "/" + stream.getName();
		try {
			cep.addStream(stream.getName(),stream.getValue());
			byte[] bytes = serializer.serialize(stream);
			Stat stat = zkc.exists(streamZnode, false);
			if (stat != null){
				zkc.setData(streamZnode,bytes,-1);
			} else {
				zkc.create(streamZnode,bytes,ZooDefs.Ids.OPEN_ACL_UNSAFE,CreateMode.PERSISTENT);
			}
		} catch (Exception e) {
			IOError ioe = new IOError();
			ioe.setMessage("Error adding " + streamZnode + ", " + e.getMessage());
			throw ioe;
		}

	}
	public void alterStream(Stream stream) throws IOError, IllegalArgument  {
		LOG.debug("alterStream [" + stream.toString() + "]");
		
		String streamZnode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS + "/" + stream.getName();
		try {
			cep.alterStream(stream.getName(),stream.getValue());
			byte[] bytes = serializer.serialize(stream);
			Stat stat = zkc.exists(streamZnode, false);
			if (stat != null){
				zkc.setData(streamZnode,bytes,-1);
			} else {
				IOError ioe = new IOError();
				ioe.setMessage("Stream " + streamZnode + " not found");
				throw ioe;
			}
		} catch (Exception e) {
			IOError ioe = new IOError();
			ioe.setMessage("Error altering " + streamZnode + ", " + e.getMessage());
			throw ioe;
		}
	}
	public void deleteStream(Stream stream) throws IOError, IllegalArgument  {
		LOG.debug("deleteStream [" + stream.toString() + "]");

		String streamZnode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS + "/" + stream.getName();
		try {
			LOG.debug("znode=" + streamZnode);
			cep.deleteStream(stream.getName());
			Stat stat = zkc.exists(streamZnode, false);
			if (stat != null){
				zkc.delete(streamZnode,-1);
			} else {
				IOError ioe = new IOError();
				ioe.setMessage("Stream " + streamZnode + " not found");
				throw ioe;
			}
		} catch (Exception e) {
			IOError ioe = new IOError();
			ioe.setMessage("Error deleting " + streamZnode + ", " + e.getMessage());
			throw ioe;
		}
	}
	public StreamResponse stream() throws IOError {
		LOG.debug("stream");
		
		StreamResponse response = new StreamResponse();
		ArrayList<Stream> streams = wmsMaster.getServerManager().getStreamsList();
		
		if( ! streams.isEmpty()) {
			for(Stream aStream : streams) {
				response.addToStreamList(aStream);
			}
		}
		
		return response;	
	}
	
	public void addRule(Rule rule) throws IOError, IllegalArgument  {
		LOG.debug("addRule [" + rule.toString() + "]");
		
		String ruleZnode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_RULES + "/" + rule.getName();
		try {
			cep.addQuery(rule.getValue());
			byte[] bytes = serializer.serialize(rule);
			Stat stat = zkc.exists(ruleZnode, false);
			if (stat != null){
				zkc.setData(ruleZnode,bytes,-1);
			} else {
				zkc.create(ruleZnode,bytes,ZooDefs.Ids.OPEN_ACL_UNSAFE,CreateMode.PERSISTENT);
			}
		} catch (Exception e) {
			IOError ioe = new IOError();
			ioe.setMessage("Error adding " + ruleZnode + ", " + e.getMessage());
			throw ioe;
		}

	}
	public void alterRule(Rule rule) throws IOError, IllegalArgument  {
		LOG.debug("alterRule [" + rule.toString() + "]");
		
		String ruleZnode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_RULES + "/" + rule.getName();
		try {
			cep.deleteQuery(rule.getName());
			cep.addQuery(rule.getValue());
			byte[] bytes = serializer.serialize(rule);
			Stat stat = zkc.exists(ruleZnode, false);
			if (stat != null){
				zkc.setData(ruleZnode,bytes,-1);
			} else {
				IOError ioe = new IOError();
				ioe.setMessage("Rule " + ruleZnode + " not found");
				throw ioe;
			}
		} catch (Exception e) {
			IOError ioe = new IOError();
			ioe.setMessage("Error altering " + ruleZnode + ", " + e.getMessage());
			throw ioe;
		}
	}
	public void deleteRule(Rule rule) throws IOError, IllegalArgument  {
		LOG.debug("deleteRule [" + rule.toString() + "]");

		String ruleZnode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_RULES + "/" + rule.getName();
		try {
			LOG.debug("znode=" + ruleZnode);
			cep.deleteQuery(rule.getName());
			Stat stat = zkc.exists(ruleZnode, false);
			if (stat != null){
				zkc.delete(ruleZnode,-1);
			} else {
				IOError ioe = new IOError();
				ioe.setMessage("Rule " + ruleZnode + " not found");
				throw ioe;
			}
		} catch (Exception e) {
			IOError ioe = new IOError();
			ioe.setMessage("Error deleting " + ruleZnode + ", " + e.getMessage());
			throw ioe;
		}
	}
	public RuleResponse rule() throws IOError {
		LOG.debug("rule");
		
		RuleResponse response = new RuleResponse();
		ArrayList<Rule> rules = wmsMaster.getServerManager().getRulesList();
		
		if( ! rules.isEmpty()) {
			for(Rule aRule : rules) {
				response.addToRuleList(aRule);
			}
		}
		
		return response;	
	}
	
	public WorkloadResponse workload() throws IOError {
		LOG.debug("workload");
		
		WorkloadResponse response = new WorkloadResponse();
		ArrayList<Request> workloads = wmsMaster.getServerManager().getWorkloadsList();
		
		if( ! workloads.isEmpty()) {
			for(Request aWorkload : workloads) {
				response.addToWorkloadList(aWorkload);
			}
		}
		
		return response;
	}
		
	String stackTraceToString(Exception e) {
		Writer writer = new StringWriter();
		PrintWriter printWriter = new PrintWriter(writer);
		e.printStackTrace(printWriter);
		String s = writer.toString();
		return s;
	}
	
}
