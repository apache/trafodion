/**
 *(C) Copyright 2014 Hewlett-Packard Development Company, L.P.
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
package org.trafodion.dcs.util;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;

import java.util.*;

import java.sql.*;
import java.math.BigDecimal;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.GnuParser;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.apache.commons.cli.HelpFormatter;

import org.codehaus.jettison.json.JSONArray;
import org.codehaus.jettison.json.JSONException;
import org.codehaus.jettison.json.JSONObject;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

import org.apache.hadoop.conf.Configuration;

import org.trafodion.dcs.util.DcsConfiguration;
import org.trafodion.dcs.Constants;

public final class JdbcT4Util
{
	private static final Log LOG = LogFactory.getLog(JdbcT4Util.class);
    private Configuration conf;
    private DcsNetworkConfiguration netConf;
	private ConnectionContext connContext = null;
 
	static
	{
		try {
			Class.forName(Constants.T4_DRIVER_CLASS_NAME);
		} catch (ClassNotFoundException e) {
			e.printStackTrace();
			if(LOG.isErrorEnabled())
				LOG.error(e.getMessage());
		}
	}
	
	class ConnectionContext {
		java.sql.Connection conn = null;
		java.sql.Statement stmt = null;
		java.sql.ResultSet rs = null; 
		String url = Constants.T4_DRIVER_URL + "//" + netConf.getHostName() + ":" + conf.getInt(Constants.DCS_MASTER_PORT,Constants.DEFAULT_DCS_MASTER_PORT) + "/:";
		String user = null;
		String password = null;
		
		private void open() throws SQLException {
			if(LOG.isDebugEnabled())
				LOG.debug("Begin ConnectionContext.open()");
			
			if((conn == null) || (conn.isClosed())) {
				if(LOG.isDebugEnabled())
					LOG.debug("DriverManager.getConnection(" + "url=" + url + ",user=" + user + ",password=" + password + ")");
				conn = DriverManager.getConnection(url,user,password);
				stmt = conn.createStatement();
				if(LOG.isDebugEnabled())
					LOG.debug("new connection created");
			} else {
				if(LOG.isDebugEnabled())
					LOG.debug("using existing connection");
			}
			
			if(LOG.isDebugEnabled())
				LOG.debug("End ConnectionContext.open()");
		}
		
		private void close() throws SQLException {
			if(LOG.isDebugEnabled())
				LOG.debug("Begin ConnectionContext.close()");
			stmt = null;
			conn = null;
			if(LOG.isDebugEnabled())
				LOG.debug("Begin ConnectionContext.close()");
		}
		
		boolean exec(String text) {
			if(LOG.isDebugEnabled())
				LOG.debug("Begin ConnectionContext.exec()");

			boolean result = true;

			try {
				open();
				if(LOG.isDebugEnabled())
					LOG.debug("stmt.executeQuery(" + text + ")");
				rs = stmt.executeQuery(text);
			} catch (SQLException e) {
				SQLException nextException = e;
				StringBuilder sb = new StringBuilder();
				do {
					sb.append(nextException.getMessage());
					sb.append("\nSQLState   " + nextException.getSQLState());
					sb.append("\nError Code " + nextException.getErrorCode());
				} while ((nextException = nextException.getNextException()) != null);
				if(LOG.isErrorEnabled()) 
					LOG.error("SQLException [" + sb.toString() + "]");	
				result = false;
			}

			if(LOG.isDebugEnabled())
				LOG.debug("End ConnectionContext.exec()");

			return result;
		}
		
		public void setUserPassword(String value) {
			LOG.debug("value=" + value);
			
			String base64UserPassword = null;
			if (value != null) 
				base64UserPassword = value.trim();
			LOG.debug("base64UserPassword=" + base64UserPassword);
			String userPassword = new String(Base64.decode(base64UserPassword));
			LOG.debug("userPassword=" + userPassword);
			final String[] values = userPassword.split(":",2);
			this.user = values[0];
			this.password = values[1];
			LOG.debug("user=" + user + ",password=" + password);
		}

		java.sql.ResultSet getResultSet(){
			return rs;
		}
	}
	
	public void init(Configuration conf,DcsNetworkConfiguration netConf) {
		this.conf = conf;
		this.netConf = netConf;
		connContext = new ConnectionContext();
    	String s = conf.get(Constants.T4_DRIVER_USERNAME_PASSWORD,Constants.DEFAULT_T4_DRIVER_USERNAME_PASSWORD);
    	connContext.setUserPassword(s);
	}
	
	public JdbcT4Util() throws Exception {
		Configuration conf = DcsConfiguration.create();
	   	DcsNetworkConfiguration netConf = new DcsNetworkConfiguration(conf);
		init(conf,netConf);
	}
	
	public JdbcT4Util(Configuration conf,DcsNetworkConfiguration netConf) {
		init(conf,netConf);
	}
	
	public synchronized JSONArray exec(String command){
		if(LOG.isDebugEnabled())
			LOG.debug("Begin exec()");

		JSONArray js = null;

		int retryCount = 2;
		while(retryCount > 0) {
			try	{
				if(connContext.exec(command)) {
					js = new JSONArray();
					js = convertResultSetToJSON(connContext.getResultSet());
					retryCount = 0;
				} else {
					retryCount--;
				}
			} catch (Exception e) {
				StringBuilder sb = new StringBuilder();
				e.printStackTrace();
				sb.append(e.getMessage());
				if(LOG.isErrorEnabled()) 
					LOG.error("Exception [" + sb.toString() + "]");
				retryCount--;
			}
		}

		if(LOG.isDebugEnabled())
			LOG.debug("End exec()");

		return js;
	}
	
	private synchronized JSONArray convertResultSetToJSON(java.sql.ResultSet rs) throws Exception {
		if(LOG.isDebugEnabled())
			LOG.debug("Begin convertResultSetToJSON");
		
		JSONArray json = new JSONArray();

		try { 

			java.sql.ResultSetMetaData rsmd = rs.getMetaData(); 

			while(rs.next()){ 
				int numColumns = rsmd.getColumnCount(); 
				JSONObject obj = new JSONObject(); 

				for (int i=1; i<numColumns+1; i++) { 

					String column_name = rsmd.getColumnName(i); 

					if(rsmd.getColumnType(i)==java.sql.Types.ARRAY){ 
						obj.put(column_name, rs.getArray(column_name)); 
					} 
					else if(rsmd.getColumnType(i)==java.sql.Types.BIGINT){ 
						obj.put(column_name, rs.getLong(column_name)); 
					} 
					else if(rsmd.getColumnType(i)==java.sql.Types.BOOLEAN){ 
						obj.put(column_name, rs.getBoolean(column_name)); 
					} 
					else if(rsmd.getColumnType(i)==java.sql.Types.BLOB){ 
						obj.put(column_name, rs.getBlob(column_name)); 
					} 
					else if(rsmd.getColumnType(i)==java.sql.Types.DOUBLE){ 
						obj.put(column_name, rs.getDouble(column_name));  
					} 
					else if(rsmd.getColumnType(i)==java.sql.Types.FLOAT){ 
						obj.put(column_name, rs.getFloat(column_name)); 
					} 
					else if(rsmd.getColumnType(i)==java.sql.Types.INTEGER){ 
						obj.put(column_name, rs.getInt(column_name)); 
					} 
					else if(rsmd.getColumnType(i)==java.sql.Types.NVARCHAR){ 
						obj.put(column_name, rs.getNString(column_name)); 
					} 
					else if(rsmd.getColumnType(i)==java.sql.Types.CHAR || 
							rsmd.getColumnType(i)==java.sql.Types.VARCHAR){ 
						//prevent obj.put from removing null key value from JSONObject
						String s = rs.getString(column_name);
						if(s == null)
							obj.put(column_name, new String("")); 
						else
							obj.put(column_name, rs.getString(column_name)); 
					} 					
					else if(rsmd.getColumnType(i)==java.sql.Types.TINYINT){ 
						obj.put(column_name, rs.getInt(column_name)); 
					} 
					else if(rsmd.getColumnType(i)==java.sql.Types.SMALLINT){ 
						obj.put(column_name, rs.getInt(column_name)); 
					} 
					else if(rsmd.getColumnType(i)==java.sql.Types.DATE){ 
						obj.put(column_name, rs.getDate(column_name)); 
					} 
					else if(rsmd.getColumnType(i)==java.sql.Types.TIMESTAMP){ 
						obj.put(column_name, rs.getTimestamp(column_name));    
					} 
					else{ 
						obj.put(column_name, rs.getObject(column_name)); 
					}  

				}//end foreach 
				json.put(obj); 

			}//end while 

		} catch (SQLException e) { 
			e.printStackTrace(); 
			if(LOG.isDebugEnabled())
				LOG.error(e.getMessage());
			throw e;
		} catch (Exception e) { 
			e.printStackTrace(); 
			if(LOG.isDebugEnabled())
				LOG.error(e.getMessage());
			throw e;
		}
		
		if(LOG.isDebugEnabled())
			LOG.debug("End convertResultSetToJSON");

		return json; 
	}

	
	public static void main(String args[])
	{
		Options options = new Options();
		options.addOption("h","help",false,"print this message");
		options.addOption("c","command",true,"SQL query to execute");
		HelpFormatter formatter = new HelpFormatter();
			  
		CommandLine commandLine;
		String command = null;
		
		StringBuilder sb = new StringBuilder();
		
		try {
			commandLine = new GnuParser().parse(options, args);
			if (commandLine.hasOption("help")) {
				formatter.printHelp("JdbcT4Util",options);
				System.exit(0);
			}
			if (commandLine.hasOption("command")) {
				command = commandLine.getOptionValue("c");
			}
			if (command == null) {
				formatter.printHelp("JdbcT4Util",options);
				System.exit(-1);
			}
		} catch (Exception e) {
			formatter.printHelp("JdbcT4Util",options);
			System.exit(-1);
		}

		try	{
			JdbcT4Util JdbcT4Util = new JdbcT4Util();
			JSONArray js = JdbcT4Util.exec(command);
			if(LOG.isDebugEnabled())
				LOG.debug("JSONArray [" + js.toString() + "]");
			System.exit(0);
		} catch (Exception e) {
			e.printStackTrace();
			sb.append(e.getMessage());
			if(LOG.isDebugEnabled())
				LOG.debug("Exception [" + sb.toString() + "]");
			System.exit(1);
		}
	}
	
}
