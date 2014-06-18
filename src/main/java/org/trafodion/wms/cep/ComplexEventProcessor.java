package org.trafodion.wms.cep;

import java.util.*; 

import org.apache.commons.io.IOUtils;
import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.GnuParser;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.hadoop.conf.Configuration;

import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.ZooDefs;
import org.apache.zookeeper.data.Stat;
import org.apache.zookeeper.Watcher;
import org.apache.zookeeper.WatchedEvent;
import org.apache.zookeeper.KeeperException;

import org.apache.thrift.TBase;
import org.apache.thrift.TException;
import org.apache.thrift.TSerializer;
import org.apache.thrift.TDeserializer;
import org.apache.thrift.transport.TSSLTransportFactory;
import org.apache.thrift.transport.TTransport;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TSSLTransportFactory.TSSLTransportParameters;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;

import org.trafodion.wms.util.RetryCounter;
import org.trafodion.wms.util.RetryCounterFactory;
import org.trafodion.wms.util.Bytes;
import org.trafodion.wms.thrift.generated.*;
import org.trafodion.wms.Constants;
import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.util.VersionInfo;
import org.trafodion.wms.zookeeper.ZkClient;
import org.trafodion.wms.server.ServerLeaderElection;

import org.wso2.siddhi.core.SiddhiManager;
import org.wso2.siddhi.core.event.Event;
import org.wso2.siddhi.core.stream.input.InputHandler;
import org.wso2.siddhi.core.stream.output.StreamCallback;
import org.wso2.siddhi.core.util.EventPrinter;
import org.wso2.siddhi.query.compiler.exception.SiddhiPraserException;

public class ComplexEventProcessor {
	private static  final Log LOG = LogFactory.getLog(ComplexEventProcessor.class);
	private String[] args;
	private static Configuration conf;
    private ZkClient zkc = null;
	private RetryCounterFactory retryCounterFactory;
    private int maxWaitAttempts;
    private int retryIntervalMillis;
	private static String parentZnode;
	private ServerLeaderElection sle = null;
	private TSerializer serializer = new TSerializer(new TBinaryProtocol.Factory());
    private TDeserializer deserializer = new TDeserializer(new TBinaryProtocol.Factory());
	private SiddhiManager siddhiManager = new SiddhiManager();
	private CepSubjectImpl actionSubject = new CepSubjectImpl();
	private Map<String,StreamContext> streamMap = new HashMap<String, StreamContext>();
	
	public ComplexEventProcessor(ZkClient zkc,String parentZnode,Configuration conf,ServerLeaderElection sle) throws Exception {
		this.zkc = zkc;
		this.parentZnode = parentZnode;
		this.conf = conf;
		this.sle = sle;//WmsMaster always calls with null value
		maxWaitAttempts = conf.getInt(Constants.WMS_CEP_WAIT_INIT_ATTEMPTS,Constants.DEFAULT_WMS_CEP_WAIT_INIT_ATTEMPTS);
		retryIntervalMillis = conf.getInt(Constants.WMS_CEP_WAIT_INIT_RETRY_INTERVAL_MILLIS,Constants.DEFAULT_WMS_CEP_WAIT_INIT_RETRY_INTERVAL_MILLIS);
		retryCounterFactory = new RetryCounterFactory(maxWaitAttempts, retryIntervalMillis);
		init();		
	}
	
	public void init() throws Exception {
		LOG.debug("Init");
		checkStreams();
		checkRules();
		watchZkStreams();
		watchZkRules();
		LOG.info("Complex event processor started");
	}
	
	public CepSubjectImpl getActionSubject(){
		return actionSubject;
	}
	
	void checkStreams() throws Exception {
		List<String> children = getChildren(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS,null);
		if(children.isEmpty() && sle != null){ //WmsServer
			LOG.info("No children found in znode" + parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS);	
			LOG.info("Waiting for WmsMaster to initialize CEP streams/rules");
			RetryCounter retryCounter = retryCounterFactory.create();
			while(true){
				String znode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS + "/" + Constants.VERTICA;
				Stat stat = zkc.exists(znode,false);
				if(stat != null) {
					znode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS + "/" + Constants.TRAFODION;
					stat = zkc.exists(znode,false);
					if(stat != null) {
						LOG.info("Found CEP streams/rules");
						continue;
					} 
				}
				
				if (! retryCounter.shouldRetry()) {
					LOG.error("WmsMaster failed to initialize CEP streams/rules");
					throw new Exception("WmsMaster failed to initialize CEP streams/rules");
				} else {
					LOG.info("Waiting for WmsMaster to initialize CEP streams/rules");
					retryCounter.sleepUntilNextRetry();
					retryCounter.useRetry();
				}
			}
		} else if(children.isEmpty() && sle == null){ //WmsMaster
			LOG.info("No children found in znode " + parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS);	
			LOG.info("Adding default streams");
			
			//Add Default vertica stream
			String streamId = Constants.VERTICA;
			String keys = conf.get("wms.server.cep.vertica.keys","");
			keys = keys.replaceAll("[\\n\\r\\t]", "");
			keys = keys.replaceAll(" ", "");
			addStream(streamId,keys);
			
			Stream stream = new Stream(streamId,keys,"Added by administrator",System.currentTimeMillis());
			String znode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS + "/" + Constants.VERTICA;
			Stat stat = zkc.exists(znode,false);
			if(stat == null) {
				byte[] bytes = serializer.serialize(stream);
				zkc.create(znode,bytes,ZooDefs.Ids.OPEN_ACL_UNSAFE,CreateMode.PERSISTENT);
				LOG.info("Created znode [" + znode + "]");
			}
			//default trafodion stream
			streamId = Constants.TRAFODION;
			keys = conf.get("wms.server.cep.trafodion.keys","");
			keys = keys.replaceAll("[\\n\\r\\t]", "");
			keys = keys.replaceAll(" ", "");
			addStream(streamId,keys);
			
			stream = new Stream(streamId,keys,"Added by administrator",System.currentTimeMillis());
			znode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS + "/" + Constants.TRAFODION;
			stat = zkc.exists(znode,false);
			if(stat == null) {
				byte[] bytes = serializer.serialize(stream);
				zkc.create(znode,bytes,ZooDefs.Ids.OPEN_ACL_UNSAFE,CreateMode.PERSISTENT);
				LOG.info("Created znode [" + znode + "]");
			}

			//default action stream
			streamId = "action";
			addStream(streamId,"action,string");
			addCallback(streamId);
			stream = new Stream(streamId,"action,string","Added by administrator",System.currentTimeMillis());
			znode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS + "/" + Constants.ACTION;
			stat = zkc.exists(znode,false);
			if(stat == null) {
				byte[] bytes = serializer.serialize(stream);
				zkc.create(znode,bytes,ZooDefs.Ids.OPEN_ACL_UNSAFE,CreateMode.PERSISTENT);
				LOG.info("Created znode [" + znode + "]");
			}
			
			return;
		} 
		
		children = getChildren(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS,null);
		if(! children.isEmpty()){
			LOG.info("Found children in znode [" + parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS + "]");
			for(String aChild : children) {
				LOG.debug("child [" + aChild + "]");
				Stream stream = new Stream();
				String znode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS + "/" + aChild;
				Stat stat = zkc.exists(znode, false);
				if (stat != null){
					byte[] bytes = zkc.getData(znode, false, stat);
					try {
						deserializer.deserialize(stream, bytes);
						String keys = stream.getValue();
						keys = keys.replaceAll("[\\n\\r\\t]", "");
						keys = keys.replaceAll(" ", "");
						addStream(stream.getName(),keys);
						if(stream.getName().equals("action")) 
							addCallback(stream.getName());
					} catch (TException e) {
						e.printStackTrace();
					}
				}
			}
		} else {
			LOG.info("No children found in znode [" + parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS + "]");
		}
	}
	
	void checkRules() throws Exception {
		LOG.info("Looking for CEP rules in znode " + parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_RULES);	
		String rulesZnode = parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_RULES;
		List<String> children = getChildren(rulesZnode,null);
		if( ! children.isEmpty()) {
			for(String aChild : children) {
				Rule rule = new Rule();
				String znode = rulesZnode + "/" + aChild;
				Stat stat = zkc.exists(znode, false);
				if (stat != null){
					byte[] bytes = zkc.getData(znode, false, stat);
					try {
						deserializer.deserialize(rule, bytes);
						addQuery(rule.getValue());
					} catch (TException e) {
						e.printStackTrace();
					}
				}
			}
		} else {
			LOG.debug("No children found in znode [" + rulesZnode + "]");		
		}
	}
	
	public void workload(String streamName,HashMap<String,KeyValue> keyValues) throws Exception {
		LOG.debug("workload " + keyValues);
		StreamContext streamContext = streamMap.get(streamName);
		send(getInputHandler(streamName),streamContext.keyValuesToArray(keyValues));
	}
	
	public void addStream(String name,String keys) throws Exception {
		StreamContext streamContext = new StreamContext(name,keys);
		streamMap.put(name,streamContext);
		defineStream(streamContext.getDefineStreamString());
	}
	
	public void alterStream(String name,String keys) throws Exception {
		deleteStream(name);
		addStream(name,keys);
	}
	
	public void deleteStream(String name) throws Exception {
		StreamContext streamContext = streamMap.get(name);
		if(streamContext != null){
			streamMap.remove(name);
			removeStream(name);
		}
	}

	//Siddhi specific methods
	public void defineStream(String value) throws Exception {
		LOG.debug("defineStream [" + value + "]");
		try {
			siddhiManager.defineStream(value);
		} catch (Exception e) {
			LOG.error(e.getMessage());
		}
	}
	
	public void removeStream(String value) {
		LOG.debug("removeStream [" + value + "]");
		siddhiManager.removeStream(value);
	}
	
	public void addQuery(String text) throws Exception {
		LOG.debug("addQuery [" + text + "]");
		try {
			siddhiManager.addQuery(text);
		} catch (Exception e){
			LOG.error(e.getMessage());
			throw e;
		}
	}
	
	public void deleteQuery(String text) {
		LOG.debug("deleteQuery [" + text + "]");
	    siddhiManager.removeQuery(text);
	}
	
	void addCallback(String name) {
		LOG.debug("addCallback [" + name + "]");
		siddhiManager.addCallback(name, new StreamCallback() {

			@Override
			public void receive(Event[] events) {
				//EventPrinter.print(events);
				LOG.debug("event received on stream [" + (String) events[0].getStreamId() + "], Data [" + (String) events[0].getData0() + "]");
				actionSubject.post((String) events[0].getData0());
			}
		});
	}

	public InputHandler getInputHandler(String name) throws InterruptedException {
		LOG.debug("getInputHandler [" + name + "]");
		return siddhiManager.getInputHandler(name);
    }
	
	public void send(InputHandler handler, Object[] value) throws InterruptedException {
		LOG.debug("send [" + handler + "," + value + "]");
		handler.send(value);
	}
	
	class StreamsWatcher implements Watcher {
		public void process(WatchedEvent event) {
			LOG.debug("StreamsWatcher fired [" + event.getPath() + "]");
			if(event.getType() == Event.EventType.NodeChildrenChanged
					|| event.getType() == Event.EventType.NodeDeleted) {
				LOG.debug("Streams children changed [" + event.getPath() + "]");
				try {
					watchZkStreams();
				} catch (Exception e) {
					e.printStackTrace();
					LOG.error(e);
				}
			} else if(event.getType() == Event.EventType.NodeCreated) {
				LOG.debug("Streams znode created [" + event.getPath() + "]");
				try {
					Stream stream = new Stream();
					String znode = event.getPath();
					Stat stat = zkc.exists(znode, false);
					if (stat != null){
						byte[] bytes = zkc.getData(znode, false, stat);
						try {
							deserializer.deserialize(stream, bytes);
							defineStream(stream.getValue());
							addCallback(stream.getName());
						} catch (TException e) {
							e.printStackTrace();
						}
					}
				} catch (Exception e) {
					e.printStackTrace();
					LOG.error(e);
				}
			} else if(event.getType() == Event.EventType.NodeDataChanged) {
				String znodePath = event.getPath();
				LOG.debug("Streams znode data changed [" + znodePath + "]");
				try {
					Stream stream = new Stream();
					String znode = event.getPath();
					Stat stat = zkc.exists(znode, false);
					if (stat != null){
						byte[] bytes = zkc.getData(znode, false, stat);
						try {
							deserializer.deserialize(stream, bytes);
							removeStream(stream.getName());
							defineStream(stream.getValue());
							addCallback(stream.getName());
						} catch (TException e) {
							e.printStackTrace();
						}
					}
				} catch (Exception e) {
					e.printStackTrace();
					LOG.error(e);
				}
			} else if(event.getType() == Event.EventType.NodeDeleted) {
				String znodePath = event.getPath();
				LOG.debug("Streams znode deleted [" + znodePath + "]");
				try {
					Stream stream = new Stream();
					String znode = event.getPath();
					Stat stat = zkc.exists(znode, false);
					if (stat != null){
						byte[] bytes = zkc.getData(znode, false, stat);
						try {
							deserializer.deserialize(stream, bytes);
							removeStream(stream.getName());
						} catch (TException e) {
							e.printStackTrace();
						}
					}
				} catch (Exception e) {
					e.printStackTrace();
					LOG.error(e);
				}
			}
		}
	}
	
	class RulesWatcher implements Watcher {
		public void process(WatchedEvent event) {
			LOG.debug("RulesWatcher fired [" + event.getPath() + "]");
			if(event.getType() == Event.EventType.NodeChildrenChanged
					|| event.getType() == Event.EventType.NodeDeleted) {
				LOG.debug("Rules children changed [" + event.getPath() + "]");
				try {
					watchZkRules();
				} catch (Exception e) {
					e.printStackTrace();
					LOG.error(e);
				}
			} else if(event.getType() == Event.EventType.NodeCreated) {
				LOG.debug("Rules znode created [" + event.getPath() + "]");
				try {
					Rule rule = new Rule();
					String znode = event.getPath();
					Stat stat = zkc.exists(znode, false);
					if (stat != null){
						byte[] bytes = zkc.getData(znode, false, stat);
						try {
							deserializer.deserialize(rule, bytes);
							addQuery(rule.getValue());
							watchZkRules();
						} catch (TException e) {
							e.printStackTrace();
						}
					}
				} catch (Exception e) {
					e.printStackTrace();
					LOG.error(e);
				}
			} else if(event.getType() == Event.EventType.NodeDataChanged) {
				String znodePath = event.getPath();
				LOG.debug("Rules znode data changed [" + znodePath + "]");
				try {
					Rule rule = new Rule();
					String znode = event.getPath();
					Stat stat = zkc.exists(znode, false);
					if (stat != null){
						byte[] bytes = zkc.getData(znode, false, stat);
						try {
							deserializer.deserialize(rule, bytes);
							deleteQuery(rule.getValue());
							addQuery(rule.getValue());
							watchZkRules();
						} catch (TException e) {
							e.printStackTrace();
						}
					}
				} catch (Exception e) {
					e.printStackTrace();
					LOG.error(e);
				}
			} else if(event.getType() == Event.EventType.NodeDeleted) {
				String znodePath = event.getPath();
				LOG.debug("Rules znode deleted [" + znodePath + "]");
				try {
					Rule rule = new Rule();
					String znode = event.getPath();
					Stat stat = zkc.exists(znode, false);
					if (stat != null){
						byte[] bytes = zkc.getData(znode, false, stat);
						try {
							deserializer.deserialize(rule, bytes);
							deleteQuery(rule.getName());
							watchZkRules();
						} catch (TException e) {
							e.printStackTrace();
						}
					}
				} catch (Exception e) {
					e.printStackTrace();
					LOG.error(e);
				}
			}
		}
	}
	
	List<String> getChildren(String znode,Watcher watcher) throws Exception {
		List<String> children=null;
		children = zkc.getChildren(znode,watcher);
        if( ! children.isEmpty()) 
        	Collections.sort(children);
		return children;
	}
	
	List<String> watchZkStreams() throws Exception {
		LOG.debug("Reading " + parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS);
		return getChildren(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_STREAMS, new StreamsWatcher());
	}
	
	List<String> watchZkRules() throws Exception {
		LOG.debug("Reading " + parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_RULES);
		return getChildren(parentZnode + Constants.DEFAULT_ZOOKEEPER_ZNODE_RULES, new RulesWatcher());
	}

/*	
	String getStreamMap(String checksum) {	
		String name = streamMap.get(checksum);
		if(name == null){
			//LOG.debug("Stream map [" + name + "]" + " not found);
			System.out.println("getStreamMap " + checksum + " not found");
		} else {
			//LOG.debug("Stream map [" + name + "]" + " found");
			System.out.println("getStreamMap " + checksum + " is " + name);
		}
		return name;
	}
	
	void addStreamMap(String checksum,String name) {
		streamMap.put(checksum,name);
		//LOG.debug("addStreamMap [" + checksum + "," + name + "]");
		System.out.println("addStreamMap [" + checksum + "," + name + "]");
	}
 	
	String keysToString(HashMap<String,String> keyValues) {	
		StringBuilder sb = new StringBuilder();
		if(keyValues.keySet() != null){
			for (String key: keyValues.keySet()) {
				sb.append(key); 
			} 
		}
		return sb.toString();
	}
 
	String getChecksum(String keyString) {
		BigInteger bi = null;
	    try {
	        MessageDigest md = MessageDigest.getInstance( "SHA1" );
	        md.update( keyString.getBytes() );
	        bi = new BigInteger(1, md.digest());
	    }
	    catch (NoSuchAlgorithmException e) {
			LOG.debug(e.getMessage());
	    }
	    
	    return bi.toString(16);
	}
 	
	String buildDefineStreamString(String name,HashMap<String,String> keyValues) {
		StringBuilder sb = new StringBuilder();
		if(keyValues.keySet() != null && name != null){
			sb.append("define stream " + name + "(");
			boolean isFirst = true;
			for (String key: keyValues.keySet()) {
				if(isFirst) {
					sb.append(key + " string");
					isFirst = false;
				} else {
					sb.append(", " + key + " string");
				}
			}
			sb.append(")");
			//LOG.debug("keyValuesToDefineStreamString [" sb.toString() + "]");
			System.out.println("keyValuesToDefineStreamString [" + sb.toString() + "]");
		}
		return sb.toString();
	}
*/	

}
