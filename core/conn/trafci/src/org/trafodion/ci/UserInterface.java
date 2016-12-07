// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

package org.trafodion.ci;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.net.UnknownHostException;
import java.sql.SQLException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Enumeration;
import java.util.List;
import java.util.Properties;

import org.trafodion.ci.pwdencrypt.RefLookup;


public class UserInterface {

	static String[] myArgs = null;

	/**
	 * Main program called when the user logins using the windows/*nux wrapper
	 * scripts. Creates a console reader & console write for interactive
	 * input/output from the user.
	 */
	public static void main(String[] args) {
		// create the console reader and writer objects
		// for interactive session
		ConsoleReader crObj = null;
		ConsoleWriter cwObj = null;
		myArgs = args;
		int exitCode = 0;
		boolean doTrace = Boolean.getBoolean(SessionDefaults.PROP_TRACE);
		String mySQRoot = System.getenv("TRAF_HOME");
		


		/* handle AWT exceptions */
		try {
			Class.forName("org.trafodion.ci.AWTExceptionHandler");
			System.setProperty("sun.awt.exception.handler",
					AWTExceptionHandler.class.getName());
		} catch (Throwable t) {
			if (doTrace)
				t.printStackTrace();
		}

		cwObj = new ConsoleWriter();

		try {
			cwObj.initialize();
		} catch (IOException e1) {
			System.out.println(SessionError.CONSOLE_WRITER_ERR.errorMessage()
					+ e1);
			if (doTrace)
				e1.printStackTrace();
			System.exit(SessionDefaults.abruptExit);
		}
		crObj = new ConsoleReader();

		try {
			crObj.initialize();
		} catch (IOException e1) {
			System.out.println(SessionError.CONSOLE_READER_ERR.errorMessage()
					+ e1);
			if (doTrace)
				e1.printStackTrace();
			System.exit(SessionDefaults.abruptExit);
		}

		banner();
        
		// parse the arguments
		ParseArguments paObj = new ParseArguments(crObj, cwObj);

		// set default value to rolename
		paObj.roleName = "";

		paObj.setDefaults(SessionDefaults.dsnName, SessionDefaults.portNumber);

		
		if ( args.length %2 == 0 && mySQRoot != null && new File(mySQRoot).exists()) {
			String uname=null, passwd = null, hostname = null, portNumber = null;
			//EncryptUtil eu = new EncryptUtil();
			Properties p = new Properties();
			String matchedValue = null;
			List<String> argsList= new ArrayList<String>(Arrays.asList(args));
			int argLength=argsList.size();

			for (int i=0;i < argLength; i++)
		    {
		         String option=args[i++].trim();
		         String value=args[i].trim();
		         
		         if (option.equalsIgnoreCase("-u")|| option.equalsIgnoreCase("-user"))
		         {
		            uname=value;
		         }
		         else if (option.equalsIgnoreCase("-p")|| option.equalsIgnoreCase("-password"))
		         {
		            passwd=value;
		         }
		         else if (option.equalsIgnoreCase("-h")|| option.equalsIgnoreCase("-host"))
		         {
		            String[] hostAddr=value.split(":");
		            if (hostAddr.length > 0)
		            	hostname=hostAddr[0];
		            
		            String portValue="";

		            for (int j=1; j < hostAddr.length;j++)
		            {
		               portValue+=":"+hostAddr[j];
		            }
		            if (portValue.length() > 0)
		            {
		               portNumber=portValue;
		            }

		         }
		    }
			
			try 
			{
				
				if (uname != null)
				{
					paObj.userName = uname;
				}
				else
				{
					paObj.userName = System.getProperty("user.name");
				}
				
				if (passwd != null)
				{
					paObj.password = passwd;
				}
				else
				{
					String userHome =  System.getenv("USERHOME");//System.getProperty("user.dir");				
					if (userHome != null)
					{					
						File f = new File(userHome + "/.trafciconf/encprops.txt");
						if (f.exists())
						{
							try
							{
								p.load(new FileInputStream(f));
								Enumeration<?> enumKeys = p.keys();
								while (enumKeys.hasMoreElements())
							      {
									uname=enumKeys.nextElement().toString();
									if (uname.equals(paObj.userName)){
										matchedValue = p.getProperty(uname);
										if (matchedValue != null)
										{
											passwd = RefLookup.resolve(uname+" = "+matchedValue);
											break;
										}
									}
							      }
								
								paObj.password = passwd;
							}
							catch(Exception e)
							{
								System.out.println("Error looking up user's password.");
								if (doTrace)
								{
									System.out.println(e.getMessage());
								}
							}
						}
					}
				}
				
				if (hostname != null)
				{
					paObj.serverName = hostname;
				}
				else
				{
					String serverName =  System.getenv("TRAFCI_SERVERNAME");										
					if (serverName == null)
					{					
						//System.out.println("Can't obtain hostname, please enter hostname:port");
						//System.exit(SessionDefaults.abruptExit);
					}
					else		
						paObj.serverName = serverName;
				}
				
				if (portNumber != null)
				{
					paObj.portNumber = portNumber;
				}
				else
				{
					String portName = System.getenv("TRAFCI_PORT");					
					if (portName == null)
					{					
						//System.out.println("Can't obtain port number, please enter hostname:port");
						//System.exit(SessionDefaults.abruptExit);
					}		
					paObj.portNumber = portName;
				}
					
				if (paObj.serverName != null && paObj.portNumber != null)
					System.out.println("Host Name/IP Address: "+paObj.serverName+paObj.portNumber);
				if (paObj.userName != null)
					System.out.println("User Name: "+paObj.userName);
				
         	} catch (Exception e)
			{
				e.printStackTrace();
			}
         	
		}
		
		while (paObj.retryCnt > 0) {

			try {
				// validate the user credentials
				myArgs = paObj.validateArgs(myArgs, true);
			} catch (UserInterruption ui) {
				System.exit(SessionDefaults.abruptExit);
			} catch (InvalidNumberOfArguments e) {
				System.out.println(e);
				if (doTrace)
					e.printStackTrace();
				System.exit(SessionDefaults.abruptExit);
			} catch (UnknownHostException e) {
				if (doTrace)
					e.printStackTrace();
				System.exit(SessionDefaults.abruptExit);
			} catch (IOException e) {
				System.out.println(e);
				if (doTrace)
					e.printStackTrace();
				System.exit(SessionDefaults.abruptExit);
			}

			// create a new session
			// pass the required credentials details and caller id
			// for this user interface the caller id will be USERI
			// A session object is returned by SessionInterface
			// with the required DB connection, writer and reader objects
			SessionInterface siObj = new SessionInterface(crObj, cwObj);
			Session sessObj = null;
			try {
				// set the script file if -script option is specified
				siObj.setScriptFile(paObj.fileName);
				sessObj = siObj.createSession(paObj.userName,
						paObj.roleName, paObj.password, paObj.serverName,
						paObj.portNumber, paObj.dsnName, SessionDefaults.USERI,
						paObj.noConnectOption);

				// DFM MXCI Migration Settings
				if (paObj.dfm) {
					sessObj.setSessView(SessionDefaults.MXCI_VIEW);
					sessObj.setSessionPrompt(">>");
					sessObj.setCmdEcho(true);
				}

				// All credentials are valid, do not prompt for login params
				// again.
				paObj.retryCnt = 0;

				// set the mode to query mode if the -sql|-q is option is
				// specified
				// show the welcome banner only when the session is interactive
				// mode ( non-query mode)
				if (paObj.queryStr != null) {
					siObj.setQueryOptions(true, paObj.queryStr);
				} else {
					System.out.println();
					if (paObj.noConnectOption)
						System.out.println("Not connected.");
					else
						System.out.println("Connected to " + sessObj.getDatabaseEdition());

				}

				// call the session
				exitCode = siObj.invokeSession(sessObj);
				siObj = null;
			} catch (FileNotFoundException e) {
				// thrown when the script file specified in the command line is
				// not found
				System.out.println();
				System.out.println(SessionError.SCRIPT_FILE_NOT_FOUND);
				if (doTrace)
					e.printStackTrace();
				System.exit(SessionDefaults.abruptExit);
			} catch (SQLException sqle) {
				if (doTrace) {
					System.out.println("Caught a SQL exception. Error code = "
							+ sqle.getErrorCode() + ", msg="
							+ sqle.getMessage());
					sqle.printStackTrace();
				}

				System.out.println();

				// catch the known errors report them with an user friendly text
				if (sqle.getErrorCode() == SessionDefaults.SQL_ERR_CONN_MAX_LIMIT) {
					System.out.println(SessionError.CONN_MAX_LIMIT_ERR);
					if (doTrace)
						sqle.printStackTrace();
					System.exit(SessionDefaults.abruptExit);
				} else {
					int errCode = sqle.getErrorCode();
					String errStr = sqle.toString();

					if (errStr.indexOf("org.trafodion.jdbc") != -1)
						errStr = errStr.substring(errStr.indexOf(":") + 1)
								.trim();

					if (errStr.startsWith("***"))
						System.out.println(errStr
								+ SessionDefaults.lineSeperator);
					else
						System.out.println(SessionError.ERROR_CODE_PREFIX
								+ Math.abs(errCode)
								+ SessionError.ERROR_CODE_SUFFIX + " " + errStr
								+ SessionDefaults.lineSeperator);

					// identify those args that caused the exception
					if (SessionDefaults.SQL_ERR_INVALID_AUTH == errCode
							|| SessionDefaults.SQL_ERR_CLI_AUTH == errCode) {
						paObj.userName = null;
						paObj.password = null;
						paObj.roleName = null;
					} else if (SessionDefaults.NDCS_ERR_DSN_NOT_AVAILABLE == errCode) {
						paObj.dsnName = null;
					} else {
						paObj.serverName = null;
					}

					// rebuild args list and remove only those that caused the
					// exception
					myArgs = paObj.rebuildArgList(myArgs);

					if (--paObj.retryCnt == 0) {
						if (doTrace)
							sqle.printStackTrace();
						System.exit(SessionDefaults.abruptExit);
					}
				}

			} // end SQLException
			catch (InstantiationException e) {
				System.out.println(SessionError.DRIVER_INIT_ERR);
				if (doTrace)
					e.printStackTrace();
				System.exit(SessionDefaults.abruptExit);
			} catch (IllegalAccessException e) {
				System.out.println(SessionError.DRIVER_INIT_ILLEGAL_ERR);
				if (doTrace)
					e.printStackTrace();
				System.exit(SessionDefaults.abruptExit);
			} catch (ClassNotFoundException e) {
				System.out.println(SessionError.DRIVER_CLASS_ERR);
				if (doTrace)
					e.printStackTrace();
				System.exit(SessionDefaults.abruptExit);
			} catch (IOException e) {
				System.out.println(e);
				if (doTrace)
					e.printStackTrace();
				System.exit(SessionDefaults.abruptExit);
			} catch (Exception e) {
				System.out.println(e);
				//if (doTrace)
					e.printStackTrace();
			} finally {
				if (sessObj != null && sessObj.getConnObj() != null) {
					try {
						sessObj.getConnObj().close();
					} catch (SQLException e) {
					}
				}
			}

			siObj = null;
		} // end while

		System.exit(exitCode);

	}

	private static void banner() {
		System.out.println("\nWelcome to " + SessionDefaults.PROD_NAME);
		String copyright_str = "Copyright (c) "+System.getenv("PRODUCT_COPYRIGHT_HEADER");
		System.out.println(copyright_str);
		System.out.println();
	}
}
