/**
 *(C) Copyright 2015 Hewlett-Packard Development Company, L.P.
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
package org.trafodion.rest.util;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.*;
import java.sql.DriverManager;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.Connection;
import java.sql.PreparedStatement;
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
import org.trafodion.rest.util.RestConfiguration;
import org.trafodion.rest.Constants;
import org.trafodion.jdbc.t4.TrafT4Connection;
import org.trafodion.jdbc.t4.TrafT4PreparedStatement;
import org.trafodion.jdbc.t4.HPT4DataSource;

public final class JdbcT4Util
{
	private static final Log LOG = LogFactory.getLog(JdbcT4Util.class);
    private Configuration conf;
    private NetworkConfiguration netConf;
	private HPT4DataSource cpds = null;
 
	static	{
		try {
			Class.forName(Constants.T4_DRIVER_CLASS_NAME);
		} catch (ClassNotFoundException e) {
			e.printStackTrace();
			if(LOG.isErrorEnabled())
				LOG.error(e.getMessage());
		}
	}

	public void init(Configuration conf,NetworkConfiguration netConf) throws SQLException {
		this.conf = conf;
		this.netConf = netConf;
	   	cpds = new HPT4DataSource();
		String url = Constants.T4_DRIVER_URL + "//" + netConf.getHostName() + ":" + conf.getInt(Constants.DCS_MASTER_PORT,Constants.DEFAULT_DCS_MASTER_PORT) + "/:";
		cpds.setURL(url);
		cpds.setMinPoolSize(conf.getInt(Constants.T4_DRIVER_MIN_POOL_SIZE,Constants.DEFAULT_T4_DRIVER_MIN_POOL_SIZE));
		cpds.setMaxPoolSize(conf.getInt(Constants.T4_DRIVER_MAX_POOL_SIZE,Constants.DEFAULT_T4_DRIVER_MAX_POOL_SIZE));   
		String s = conf.get(Constants.T4_DRIVER_USERNAME_PASSWORD,Constants.DEFAULT_T4_DRIVER_USERNAME_PASSWORD);
    	String base64UserPassword = null;
		if (s != null) 
			base64UserPassword = s.trim();
		//LOG.debug("base64UserPassword=" + base64UserPassword);
		String userPassword = new String(Base64.decode(base64UserPassword));
		//LOG.debug("userPassword=" + userPassword);
		final String[] values = userPassword.split(":",2);
		cpds.setUser(values[0]);
		cpds.setPassword(values[1]);
	}
	
	public JdbcT4Util() throws Exception {
		Configuration conf = RestConfiguration.create();
	   	NetworkConfiguration netConf = new NetworkConfiguration(conf);
		init(conf,netConf);
	}
	
	public JdbcT4Util(Configuration conf, NetworkConfiguration netConf) throws SQLException {
		init(conf,netConf);
	}

	public java.sql.Connection getConnection() throws SQLException {
		return cpds.getConnection();
	}
	
	public static JSONArray convertResultSetToJSON(java.sql.ResultSet rs) throws Exception {
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
			
			if(json.length() == 0){

				int numColumns = rsmd.getColumnCount(); 
				JSONObject obj = new JSONObject(); 

				for (int i=1; i<numColumns+1; i++) { 

					String column_name = rsmd.getColumnName(i);
					obj.put(column_name, "");
				}
				json.put(obj);
			}

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

	
	public static void main(String args[]) {
		Options options = new Options();
		options.addOption("h","help",false,"print this message");
		options.addOption("q","queryText",true,"SQL query text to execute");
		HelpFormatter formatter = new HelpFormatter();
			  
		CommandLine commandLine;
		String queryText = null;
		StringBuilder sb = new StringBuilder();
		
		try {
			commandLine = new GnuParser().parse(options, args);
			if (commandLine.hasOption("help")) {
				formatter.printHelp("JdbcT4Util",options);
				System.exit(0);
			}
			if (commandLine.hasOption("queryText")) {
				queryText = commandLine.getOptionValue("q");
			}
			if (queryText == null) {
				formatter.printHelp("JdbcT4Util",options);
				System.exit(-1);
			}
		} catch (Exception e) {
			formatter.printHelp("JdbcT4Util",options);
			System.exit(-1);
		}

		try	{
			JdbcT4Util jdbcT4Util = new JdbcT4Util();
			TrafT4Connection connection = (TrafT4Connection) jdbcT4Util.getConnection();
			
			//Regular Statement
			Statement stmt = connection.createStatement();
			ResultSet rs = stmt.executeQuery(queryText);
			JSONArray js = new JSONArray();
			js = convertResultSetToJSON(rs);
			if(LOG.isErrorEnabled()) 
				LOG.error(js.toString());
			rs.close();
			stmt.close();
			
			//PreparedStatement with explain
			TrafT4PreparedStatement pStmt = (TrafT4PreparedStatement) connection.prepareStatement(queryText, "SQL_CURSOR_DEMO");
			rs = pStmt.executeQuery("SELECT * FROM TABLE(explain(null, 'SQL_CURSOR_DEMO'))");
			js = new JSONArray();
			js = convertResultSetToJSON(rs);
			if(LOG.isErrorEnabled()) 
				LOG.error(js.toString());
			rs.close();
			stmt.close();

			System.exit(0);
		} catch (SQLException e) {
			SQLException nextException = e;
			do {
				sb.append(nextException.getMessage());
				sb.append("\nSQLState   " + nextException.getSQLState());
				sb.append("\nError Code " + nextException.getErrorCode());
			} while ((nextException = nextException.getNextException()) != null);
			if(LOG.isErrorEnabled()) 
				LOG.error("SQLException [" + sb.toString() + "]");	
			System.exit(1);
		} catch (Exception e) {
			e.printStackTrace();
			sb.append(e.getMessage());
			if(LOG.isDebugEnabled())
				LOG.debug("Exception [" + sb.toString() + "]");
			System.exit(1);
		} 
	}
	
}
