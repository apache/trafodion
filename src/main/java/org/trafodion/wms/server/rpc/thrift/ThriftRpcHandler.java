package org.trafodion.wms.server.rpc.thrift;

import java.net.*;
import java.io.Writer;
import java.io.StringWriter;
import java.io.PrintWriter;
import java.util.*;  

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.log4j.Logger;
import org.apache.thrift.TException;
import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.KeeperException;

import org.trafodion.wms.zookeeper.ZkClient;
import org.trafodion.wms.server.WmsServer;
import org.trafodion.wms.script.ScriptManager;
import org.trafodion.wms.Constants;
import org.trafodion.wms.server.workload.WorkloadStore;
import org.trafodion.wms.thrift.generated.*;
import org.trafodion.wms.cep.ComplexEventProcessor;
import org.trafodion.wms.cep.CepObserverImpl;

public class ThriftRpcHandler implements WmsService.Iface {
	private static final Log LOG = LogFactory.getLog(ThriftRpcHandler.class.getName());
	private WmsServer wmsServer;
	private long beginTs;
	private long endTs;
	private long totalTime;
	private Response response = new Response(new Header(),new Data());
	private WorkloadStore workloadStore; 
    private ComplexEventProcessor cep;
    private CepObserverImpl actionObserver = new CepObserverImpl("ThriftRpcHandler",this);
	
	public ThriftRpcHandler(WmsServer wmsServer){
		this.wmsServer = wmsServer;
		this.workloadStore = wmsServer.getServerManager().getWorkloadStore();
		this.cep = wmsServer.getServerManager().getComplexEventProcessor(); 
		cep.getActionSubject().register(actionObserver);
		actionObserver.setSubject(cep.getActionSubject());
	}
	
	public long ping(long timestamp) throws IOError {
	   LOG.debug("ping:" + timestamp + ")");
	   return System.currentTimeMillis();
	}
	
	public Response writeread(Request request) throws IOError, IllegalArgument {
		LOG.debug("writeread:" + request);

		if(! request.getData().isSetKeyValues())
 			throw new IllegalArgument("No key values found");
		
		//Setup the default response
		response.getData().setKeyValues(new HashMap<String, KeyValue>());
		Map<String,KeyValue> rspkv = response.getData().getKeyValues();
		response.getHeader().setServerLastUpdated(System.currentTimeMillis());
  		rspkv.put(Constants.ACTION,new KeyValue().setIntValue(Action.ACTION_CONTINUE.getValue()));
  		
  		//Check the request
  		Map<String,KeyValue> reqkv = request.getData().getKeyValues();
  		validate(reqkv);
 
		switch(Operation.findByValue(reqkv.get(Constants.OPERATION).getIntValue())) {
		case OPERATION_BEGIN: 
			LOG.debug("OPERATION_BEGIN:");
			beginTs = System.currentTimeMillis();
			//ScriptManager.getInstance().runScript(request,response);
			
			try{
				String streamName = reqkv.get(Constants.TYPE).getStringValue();
				cep.workload(streamName,new HashMap(reqkv));
				if(rspkv.get(Constants.ACTION).getIntValue() != Action.ACTION_REJECT.getValue() ) {
					String workloadId = workloadStore.createWorkloadId();
					reqkv.put(Constants.WORKLOAD_ID,new KeyValue().setStringValue(workloadId));
					workloadStore.put(workloadId,request);
					rspkv.put(Constants.WORKLOAD_ID,new KeyValue().setStringValue(workloadId));
				}
			} catch (Exception e){
				LOG.error(stackTraceToString(e));
				IOError ioe = new IOError();
				ioe.setMessage("IO error occurred [" + stackTraceToString(e) + "]");
				throw ioe;
			}

			endTs = System.currentTimeMillis();
			totalTime = endTs - beginTs;
			LOG.debug("Operation:[" + Operation.findByValue(reqkv.get(Constants.OPERATION).getIntValue()) + "] took [" + totalTime + "] millis");
			break;
		case OPERATION_UPDATE: 	
			LOG.debug("OPERATION_UPDATE:");
 			beginTs = System.currentTimeMillis();
			//ScriptManager.getInstance().runScript(request,response);
			
			try{
				String streamName = reqkv.get(Constants.TYPE).getStringValue();
				String workloadId = reqkv.get(Constants.WORKLOAD_ID).getStringValue();
				rspkv.put(Constants.WORKLOAD_ID,new KeyValue().setStringValue(workloadId));
				cep.workload(streamName,new HashMap(reqkv));
				workloadStore.put(workloadId,request);
			} catch (Exception e){
				LOG.error(stackTraceToString(e));
				IOError ioe = new IOError();
				ioe.setMessage("IO error occurred [" + stackTraceToString(e) + "]");
				throw ioe;
			}
			
			endTs = System.currentTimeMillis();
			totalTime = endTs - beginTs;
			LOG.debug("Operation:[" + Operation.findByValue(reqkv.get(Constants.OPERATION).getIntValue()) + "] took [" + totalTime + "] millis");
			break;
		case OPERATION_END:  	
			LOG.debug("OPERATION_END:");
 			beginTs = System.currentTimeMillis();
			//ScriptManager.getInstance().runScript(request,response);
			
			try{
				String streamName = reqkv.get(Constants.TYPE).getStringValue();
				String workloadId = reqkv.get(Constants.WORKLOAD_ID).getStringValue();
				rspkv.put(Constants.WORKLOAD_ID,new KeyValue().setStringValue(workloadId));
				cep.workload(streamName,new HashMap(reqkv));
				workloadStore.put(workloadId,request);
			} catch (Exception e){
				LOG.error(stackTraceToString(e));
				IOError ioe = new IOError();
				ioe.setMessage("IO error occurred [" + stackTraceToString(e) + "]");
				throw ioe;
			}
			
			endTs = System.currentTimeMillis();
			totalTime = endTs - beginTs;
			LOG.debug("Operation:[" + Operation.findByValue(reqkv.get(Constants.OPERATION).getIntValue()) + "] took [" + totalTime + "] millis");
			break;
		case OPERATION_UPDATE_PARENT_ID:		
			LOG.debug("Workload UPDATE_PARENT_ID received");
			//Response.setWorkloadId(Request.getWorkloadId());
			//workloadQueue.processRequest(Request,Response,workloadThresholds);
			break;
		case OPERATION_CANCEL_CHILDREN:		
			LOG.debug("Workload CANCEL_CHILDREN received");
			//Response.setWorkloadId(Request.getWorkloadId());
			//workloadQueue.processRequest(Request,Response,workloadThresholds);
			break;
		default:
			IllegalArgument ia = new IllegalArgument();
			ia.setMessage("Invalid or unknown operation specified");
			throw ia;
		}
		
		LOG.debug("writeread:" + response);

		return response;
	}
	
	void validate(Map<String,KeyValue> reqkv) throws IllegalArgument {
		
		if(reqkv.get(Constants.TYPE).getStringValue().isEmpty()) {
			IllegalArgument ia = new IllegalArgument();
			ia.setMessage("Missing type in request");
			throw ia;
		}

		switch(Operation.findByValue(reqkv.get(Constants.OPERATION).getIntValue())) {
		case OPERATION_BEGIN: 
			break;
		case OPERATION_UPDATE: 	
			if(reqkv.get(Constants.WORKLOAD_ID).getStringValue().isEmpty()) {
				IllegalArgument ia = new IllegalArgument();
				ia.setMessage("Missing workloadId in request");
				throw ia;
			}
			break;
		case OPERATION_END:  			
			if(reqkv.get(Constants.WORKLOAD_ID).getStringValue().isEmpty()) {
				IllegalArgument ia = new IllegalArgument();
				ia.setMessage("Missing workloadId in request");
				throw ia;
			}
			break;
		case OPERATION_UPDATE_PARENT_ID:		
			break;
		case OPERATION_CANCEL_CHILDREN:		
			if(reqkv.get(Constants.WORKLOAD_ID).getStringValue().isEmpty()) {
				IllegalArgument ia = new IllegalArgument();
				ia.setMessage("Missing workloadId in request");
				throw ia;
			}
			break;
		default:
			IllegalArgument ia = new IllegalArgument();
			ia.setMessage("Invalid or unknown operation specified");
			throw ia;
		}
	}
	
	String stackTraceToString(Exception e) {
		Writer writer = new StringWriter();
		PrintWriter printWriter = new PrintWriter(writer);
		e.printStackTrace(printWriter);
		String s = writer.toString();
		return s;
	}
	
	public Response getResponse() {
		return response;
	}
}
