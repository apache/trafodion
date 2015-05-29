package org.trafodion.wms.cep; 

import java.util.*;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.log4j.Logger;
import org.trafodion.wms.server.rpc.thrift.ThriftRpcHandler;
import org.trafodion.wms.thrift.generated.*;
import org.trafodion.wms.Constants;

public class CepObserverImpl implements CepObserver {    
	private static final Log LOG = LogFactory.getLog(CepObserverImpl.class.getName());
	private String name;     
	private CepSubject subject;
	private ThriftRpcHandler trpch;
	
	public CepObserverImpl(String name,ThriftRpcHandler trpch) {     
		LOG.debug("CepObserverImpl " + name); 
		this.name = name; 
		this.trpch = trpch;
	} 
	
	@Override    
	public void update() {      
		LOG.debug("update ");    
		String message = (String) subject.getUpdate(this);         
		if(message == null){             
			LOG.debug(name + "No new message");         
		} else {       
			LOG.debug(name + "Consuming message [" + message + "]"); 
			Map<String,KeyValue> rspkv = trpch.getResponse().getData().getKeyValues();
			if(message.equalsIgnoreCase("REJECT"))
				rspkv.put(Constants.ACTION,new KeyValue().setIntValue(Action.ACTION_REJECT.getValue()));
			else if (message.equalsIgnoreCase("CANCEL"))
				rspkv.put(Constants.ACTION,new KeyValue().setIntValue(Action.ACTION_CANCEL.getValue()));
		}
	} 
	
	@Override    
	public void setSubject(CepSubject subject) 
	{ 
		LOG.debug("setSubject " + subject);   
		this.subject = subject;    
	}   
}
