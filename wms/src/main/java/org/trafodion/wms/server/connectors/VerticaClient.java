package org.trafodion.wms.server.connectors;

import java.io.IOException;
import java.util.*;
import java.sql.*;
import java.text.*;

import org.apache.hadoop.conf.Configuration;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.thrift.TException;
import org.apache.thrift.transport.TSSLTransportFactory;
import org.apache.thrift.transport.TTransport;
import org.apache.thrift.transport.TSocket;
import org.apache.thrift.transport.TFramedTransport;
import org.apache.thrift.transport.TSSLTransportFactory.TSSLTransportParameters;
import org.apache.thrift.protocol.TBinaryProtocol;
import org.apache.thrift.protocol.TProtocol;
import org.trafodion.wms.thrift.generated.*;
import org.trafodion.wms.thrift.generated.Action;
import org.trafodion.wms.client.WmsClient;
import org.trafodion.wms.client.ClientData;
import org.trafodion.wms.util.WmsConfiguration;

import com.vertica.jdbc.Driver;

public class VerticaClient implements Runnable {  
	private static final Log LOG = LogFactory.getLog(VerticaClient.class.getName());
	private Configuration conf = WmsConfiguration.create();
	private Thread thrd;

	private static final String USER_NAME_KEY = "vertica.jdbc.url.user";
	private static final String DEFAULT_USER_NAME = "vertica";
    
	private static final String USER_PASSWORD_KEY = "vertica.jdbc.url.password";
	private static final String DEFAULT_USER_PASSWORD = "redhat06";
	
	private static final String JDBC_URL_KEY = "vertica.jdbc.url";
	private static final String DEFAULT_JDBC_URL = "jdbc:vertica://sq151.houston.hp.com:5433/vmartdb";
    
	private static final String COLLECT_DELAY_KEY = "vertica.jdbc.collect.delay"; //delay in seconds when connector starts to collect data measured from start of the query
	private static final int DEFAULT_COLLECT_DELAY = 10;
    
	private static final String COLLECT_TIMEOUT_KEY = "vertica.jdbc.collect.timeout"; //timeout in seconds between collecting cicles
	private static final int DEFAULT_COLLECT_TIMEOUT = 10;

	public VerticaClient(){
		thrd = new Thread(this);
		thrd.setDaemon(true);
		thrd.start();
	}
	public VerticaClient(String args[]){
		thrd = new Thread(this);
		thrd.setDaemon(true);
		thrd.start();
	}
	public void run() {
		LOG.info("run...");
		class JobStore {
			ClientData request;
			ClientData response;
			Map<String,KeyValue> m;
			
		    Timestamp timestamp;
			
			JobStore(Timestamp timestamp) {
				request = new ClientData();
				response = new ClientData();
				m = new HashMap<String, KeyValue>();
				request.setKeyValues(m);
				this.timestamp = timestamp;
			}
		}
        LOG.info("VerticaClient started.");
// Load JDBC driver
	try {
		Class.forName("com.vertica.jdbc.Driver");
	} catch (ClassNotFoundException e) {
// Could not find the driver class. Likely an issue
// with finding the .jar file.
		LOG.info("Could not find the JDBC driver class." + e);
		return;
	}
// Create property object to hold username & password
	Properties myProp = new Properties();
	
	myProp.put("user", conf.get(USER_NAME_KEY, DEFAULT_USER_NAME));
	myProp.put("password", conf.get(USER_PASSWORD_KEY, DEFAULT_USER_PASSWORD));
	int collectDelay = conf.getInt(COLLECT_DELAY_KEY, DEFAULT_COLLECT_DELAY);
	int collectTimeout = conf.getInt(COLLECT_TIMEOUT_KEY, DEFAULT_COLLECT_TIMEOUT);
	Connection conn;
	Statement stmt;
    PreparedStatement pstmt;
    PreparedStatement pstmtErrorMessage;
	ResultSet rs;
	ResultSet rsErrorMessage;
	ResultSetMetaData md;
	
	String sessionPool = "SET SESSION RESOURCE_POOL = sysquery";
	
	String queryErrorMessage = 
				"SELECT Error_Code, Message "
				+ "FROM error_messages "
				+ "WHERE Session_ID = ? and Transaction_ID = ? and Statement_ID = ?";

    String query = "AT EPOCH LATEST SELECT /* aaaa */ "
                    + "t1.Session_ID, "
                    + "t1.Transaction_ID, "
                    + "t1.Statement_ID, "
                    + "t1.Request_Type, "
                    + "t1.Request, "
                    + "t1.Start_Timestamp, "
                    + "t1.user_name, "
                    + "case when t2.pool_name is null then '' else t2.pool_name end, "
                    + "t3.counter_name, "
                    + "sum(t3.counter_value) "
                + "FROM query_requests as t1 "
                    + "left join v_monitor.resource_acquisitions as t2 "
                    +   "on t2.Transaction_ID = t1.Transaction_ID and t2.Statement_ID = t1.Statement_ID "
                    + "left join execution_engine_profiles as t3 "
                    +   "on t3.Session_ID = t1.Session_ID and t3.Transaction_ID = t1.Transaction_ID and "
                    +       "t3.Statement_ID = t1.Statement_ID "
                + "WHERE t3.counter_name in ('rows sent', 'memory allocated (bytes)') and "
                    +   "substr(t1.request,0,34) <> 'AT EPOCH LATEST SELECT /* aaaa */' and "
                    +   "t3.counter_name is not null and "
                    +   "t1.is_executing is true "
                + "group by "
                    +   "t1.Session_ID, "
                    +   "t1.Transaction_ID, "
                    +   "t1.Statement_ID, "
                    +   "t1.Request_Type, " 
                    +   "t1.Request, "
                    +   "t1.Start_Timestamp, " 
                    +   "t1.user_name, " 
                    +   "t2.pool_name, " 
                    +   "t3.counter_name "
                + "order by t1.Session_ID, "
                    +   "t1.Transaction_ID, "
                    +   "t1.Statement_ID, "
                    +   "t3.counter_name ";
	
	LOG.debug("query: |" + query + "|");

	String jdbcUrl = conf.get(JDBC_URL_KEY, DEFAULT_JDBC_URL); 

	try {
		conn = DriverManager.getConnection(jdbcUrl, myProp);
	} catch (SQLException e) {
		LOG.info("Could not connect to database." + e);
		return;
	}
	try {
		stmt = conn.createStatement();
	} catch (SQLException e) {
		LOG.info("Could not create Statement." + e);
		return;
	}
	LOG.debug("VerticaClient JDBC Driver started.");
//==========================================================
    try {
    	stmt.execute(sessionPool);
    	stmt.close();
    } catch (SQLException e) {
        LOG.info("Could not execute sessionPool." + e);
        return;
    }
//==========================================================
	try {
		pstmtErrorMessage = conn.prepareStatement(queryErrorMessage);
	} catch (SQLException e) {
		LOG.info("Could not prepare Query." + e);
	return;
	}
//==========================================================
	try {
		pstmt = conn.prepareStatement(query);
	} catch (SQLException e) {
		LOG.info("Could not prepare Query." + e);
		return;
	}
	
	HashMap< String, JobStore> progressMap = new HashMap<String, JobStore>();

	String key;
	String sessionId;
	long transactionId;
	long statementId;
	Timestamp timestamp;
	Timestamp startTimestamp;
	Timestamp endTimestamp;
	long duration = 0L;
	boolean isExecuting = false;
	String counterName = "";
	long counterSum = 0L;
	
	long startTime = 0L;
    long endTime = 0L;
	
	JobStore job = null;
    WmsClient wmsConn = null;
	
    wmsConn = new WmsClient();
    wmsConn.open();
	
	while (true){
		long jobTimeoutMins = 0;
		
        synchronized(progressMap){
            
		    try {
                startTime = System.currentTimeMillis();
                LOG.debug("***********************************Query started.");
                rs = pstmt.executeQuery();
                endTime = System.currentTimeMillis();
                LOG.debug("***********************************Query elapse time: " + (endTime - startTime));
                
                timestamp = new Timestamp(System.currentTimeMillis());
                
                while (rs.next()) {
    
                    sessionId = rs.getString("Session_Id").trim();
                    transactionId = rs.getLong("Transaction_Id");
                    statementId = rs.getLong("Statement_Id");
                    key = sessionId + "_" + Long.toString(transactionId) + "_" + Long.toString(statementId);
                    
                    startTimestamp = rs.getTimestamp("Start_Timestamp");
                    
                    LOG.debug("startTimestamp: " + startTimestamp);
    
                        if (!progressMap.containsKey(key)){
                            LOG.debug("NOT in MAP BEGIN startTimestamp: " + startTimestamp);
    
                            endTimestamp = new Timestamp(System.currentTimeMillis());
                            duration =  endTimestamp.getTime() - startTimestamp.getTime();
    
                            job = new JobStore(timestamp);
                            job.request.putKeyValue("operation", Operation.OPERATION_BEGIN);
                            job.request.putKeyValue("state","RUNNING");
                            job.request.putKeyValue("subState","BEGIN");
                            job.request.putKeyValue("beginTimestamp",startTimestamp.getTime());
                            job.request.putKeyValue("endTimestamp",endTimestamp.getTime());

                            job.request.putKeyValue("type","vertica");
                            job.request.putKeyValue("userName", rs.getString("USER_NAME").trim());
                            job.request.putKeyValue("applicationName","verticaConnector");
                            job.request.putKeyValue("sessionId", sessionId);
                            job.request.putKeyValue("transactionId", transactionId);
                            job.request.putKeyValue("statementId", statementId);
                            job.request.putKeyValue("requestType", rs.getString("REQUEST_TYPE").trim());
                            job.request.putKeyValue("request", rs.getString("REQUEST").trim());
                            job.request.putKeyValue("poolName", rs.getString("POOL_NAME").trim());
                            counterName = rs.getString("COUNTER_NAME").trim();
                            counterSum = rs.getLong("SUM");
                            if (counterName.equals("rows sent")){
                            	job.request.putKeyValue("rowsSent", counterSum);
                            } else if (counterName.equals("memory allocated (bytes)")){
                            	job.request.putKeyValue("memoryAllocated", counterSum);
                            }
                            progressMap.put(key, job);
                            job.response = wmsConn.writeread(job.request);
							executeAction(job.response.getKeyValueAction(), transactionId, sessionId, statementId);
							LOG.debug("Begin workload request..." + job.request);
                            LOG.debug("Begin workload response..." + job.response);
        
                        } else {
                            LOG.debug("In MAP UPDATE startTimestamp: " + startTimestamp);
                            endTimestamp = new Timestamp(System.currentTimeMillis());
                            duration =  endTimestamp.getTime() - startTimestamp.getTime();
    
                            job = progressMap.get(key);
                            if (timestamp != job.timestamp){
                                job.timestamp = timestamp;
                                job.request.putKeyValue("state","RUNNING");
                                job.request.putKeyValue("subState","UPDATE");

                            }
                            job.request.putKeyValue("operation",Operation.OPERATION_UPDATE);
                            job.request.putKeyValue("workloadId",job.response.getKeyValueAsString("workloadId"));
                            job.request.putKeyValue("beginTimestamp",startTimestamp.getTime());
                            job.request.putKeyValue("endTimestamp",endTimestamp.getTime());
                            job.request.putKeyValue("duration", duration);
                            counterName = rs.getString("COUNTER_NAME").trim();
                            counterSum = rs.getLong("SUM");
                            if (counterName.equals("rows sent")){
                            	job.request.putKeyValue("rowsSent", counterSum);
                            } else if (counterName.equals("memory allocated (bytes)")){
                            	job.request.putKeyValue("memoryAllocated", counterSum);
                            }
                            job.response = wmsConn.writeread(job.request);
							executeAction(job.response.getKeyValueAction(), transactionId, sessionId, statementId);
							LOG.debug("Update workload request..." + job.request);
                            LOG.debug("Update workload response..." + job.response);
                        }
                    } // while rs
                    rs.close();
                    
                    for (Iterator<String> i = progressMap.keySet().iterator(); i.hasNext(); )   
                    {   
                        key = i.next();   
                        job = progressMap.get(key);
                        
                        LOG.debug("progressMap timestamp: " + timestamp + ", job.timestamp: " + job.timestamp);
        
						if (job.timestamp != timestamp){
			 
							LOG.debug("In MAP DELETE startTimestamp: ");
								
							job = progressMap.get(key);
							job.request.putKeyValue("operation",Operation.OPERATION_END);
							job.request.putKeyValue("state","COMPLETED");
							
                            endTimestamp = new Timestamp(System.currentTimeMillis());
                            duration =  endTimestamp.getTime() - job.request.getKeyValueAsLong("beginTimestamp");

							pstmtErrorMessage.setString(1, job.request.getKeyValueAsString("sessionId"));
							pstmtErrorMessage.setLong(2, job.request.getKeyValueAsLong("transactionId"));
							pstmtErrorMessage.setLong(3, job.request.getKeyValueAsLong("statementId"));
							rsErrorMessage = pstmtErrorMessage.executeQuery();
							
							LOG.debug("session_Id :" + job.request.getKeyValueAsString("sessionId"));
							LOG.debug("transaction_Id :" + job.request.getKeyValueAsLong("transactionId"));
							LOG.debug("statement_Id :" + job.request.getKeyValueAsLong("transactionId"));
							
							if (rsErrorMessage.next()) {
								LOG.debug("Message :" + rsErrorMessage.getString("Message"));
//								job.request.putKeyValue("subState","FAILED(" + rsErrorMessage.getString("Message") + ")");
								job.request.putKeyValue("subState","CANCELED BY ADMIN");
							}
							else {
								job.request.putKeyValue("subState","SUCCESS");
							}
							job.request.putKeyValue("endTimestamp",endTimestamp.getTime());
							job.request.putKeyValue("duration", duration);
							job.response = wmsConn.writeread(job.request);
							LOG.debug("End workload..." + job.request);
							i.remove();
							rsErrorMessage.close();
						}
                    }
				}catch (SQLException e) {
					LOG.error("SQL error." + e);
					return;
				}
 
		    } //synchronize
        
			try {
				Thread.sleep(1000 * collectTimeout); //in miliseconds
			} catch (Exception e) {
				LOG.error("Unable to sleep until next cycle" + e);
				break;
			}
		} //while true
	} // run
	
	private void executeAction(Action action, Long transactionId, String sessionId, long statementId){

		String query = "";
		
		switch(action){
			case ACTION_CANCEL: case ACTION_KILL:
                LOG.debug("ACTION_CANCEL : sessionId: " + sessionId + " statementId: " + statementId);
				query = "Select INTERRUPT_STATEMENT( '" + sessionId + "', " + statementId + ")";
				break;
			case ACTION_WARNING:
                LOG.debug("ACTION_WARNING");
			    return;
			case ACTION_PRIORITY_LOW:
                LOG.debug("ACTION_PRIORITY_LOW : transactionId: " + transactionId);
				query = "Select CHANGE_CURRENT_STATEMENT_RUNTIME_PRIORITY( " + transactionId + ", 'low')";
				break;
			case ACTION_PRIORITY_MEDIUM:
                LOG.debug("ACTION_PRIORITY_MEDIUM : transactionId: " + transactionId);
				query = "Select CHANGE_CURRENT_STATEMENT_RUNTIME_PRIORITY( " + transactionId + ",'medium')";
				break;
			case ACTION_PRIORITY_HIGH:
                LOG.debug("ACTION_PRIORITY_HIGH : transactionId: " + transactionId);
				query = "Select CHANGE_CURRENT_STATEMENT_RUNTIME_PRIORITY( " + transactionId + ",'high')";
				break;
			case ACTION_CONTINUE:
                LOG.debug("ACTION_CONTINUE");
				return;
			default:
				LOG.debug("Unknown action :" + action);
				return;
		}
		LOG.debug("query: " + query);

		Properties myProp = new Properties();
	    myProp.put("user", conf.get(USER_NAME_KEY, DEFAULT_USER_NAME));
	    myProp.put("password", conf.get(USER_PASSWORD_KEY, DEFAULT_USER_PASSWORD));
		Connection conn;
		Statement stmt;
		ResultSet rs;

		String jdbcUrl = conf.get(JDBC_URL_KEY, DEFAULT_JDBC_URL);

		try {
			conn = DriverManager.getConnection(jdbcUrl, myProp);
		} catch (SQLException e) {
			LOG.error("Could not connect to database." + e);
			return;
		}
		try {
			stmt = conn.createStatement();
		} catch (SQLException e) {
			LOG.error("Could not create Statement." + e);
			return;
		}
		
        try {
			rs = stmt.executeQuery(query);
			while (rs.next()) {
				LOG.debug(rs.getString("interrupt_statement").trim());
			}
			rs.close();
		}catch (SQLException e) {
			LOG.error("SQL error." + e);
		}
        try {
        	stmt.close();
		}catch (SQLException e) {
			LOG.error("SQL error." + e);
		}
        try {
         	conn.close();
		}catch (SQLException e) {
			LOG.error("SQL error." + e);
		}
		return;
	}

	public static void main(String args[]) {
		VerticaClient client = new VerticaClient(args);
		client.run();
	}
}
