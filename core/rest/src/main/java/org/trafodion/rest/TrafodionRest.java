/**
* @@@ START COPYRIGHT @@@

Licensed to the Apache Software Foundation (ASF) under one
or more contributor license agreements.  See the NOTICE file
distributed with this work for additional information
regarding copyright ownership.  The ASF licenses this file
to you under the Apache License, Version 2.0 (the
"License"); you may not use this file except in compliance
with the License.  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an
"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
KIND, either express or implied.  See the License for the
specific language governing permissions and limitations
under the License.

* @@@ END COPYRIGHT @@@
 */

package org.trafodion.rest;

import java.util.List;
import java.util.ArrayList;
import java.io.IOException;
import java.lang.InterruptedException;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.PosixParser;
import org.apache.commons.cli.ParseException;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.apache.hadoop.conf.Configuration;
import org.trafodion.rest.util.RestConfiguration;
import org.trafodion.rest.util.VersionInfo;
import org.trafodion.rest.script.ScriptManager;
import org.trafodion.rest.script.ScriptContext;

import org.eclipse.jetty.server.Connector;
import org.eclipse.jetty.server.HttpConfiguration;
import org.eclipse.jetty.server.HttpConnectionFactory;
import org.eclipse.jetty.server.SecureRequestCustomizer;
import org.eclipse.jetty.server.Server;
import org.eclipse.jetty.server.ServerConnector;
import org.eclipse.jetty.server.SslConnectionFactory;
import org.eclipse.jetty.servlet.ServletContextHandler;
import org.eclipse.jetty.servlet.ServletHolder;
import org.eclipse.jetty.util.security.Password;
import org.eclipse.jetty.util.*;
import org.eclipse.jetty.util.ssl.SslContextFactory;
import org.eclipse.jetty.util.thread.QueuedThreadPool;

import com.sun.jersey.spi.container.servlet.ServletContainer;

public class TrafodionRest implements Runnable, RestConstants {  
	private static final Log LOG = LogFactory.getLog(TrafodionRest.class.getName());
	private Configuration conf;
	private Thread thrd;
	private String[] args;
	private RESTServlet servlet;
	
	private static void printUsageAndExit(Options options, int exitCode) {
		HelpFormatter formatter = new HelpFormatter();
		formatter.printHelp("bin/rest rest start", "", options,
				"\nTo run the REST server as a daemon, execute " +
				"bin/rest-daemon.sh start|stop rest [-p <port>] [-sp <https port>] [-ro]\n", true);
		System.exit(exitCode);
	}

	public TrafodionRest(String[] args) {
		this.args = args;
		conf = RestConfiguration.create();
		
		Options options = new Options();
		options.addOption("p", "port", true, "Http port to bind to [default: 4200]");
	    options.addOption("sp", "httpsport", true, "Https port to bind to [default: 4201]");
		options.addOption("ro", "readonly", false, "Respond only to GET HTTP " +
		"method requests [default: false]");
		//options.addOption(null, "infoport", true, "Port for web UI");
		
		try {
			servlet = RESTServlet.getInstance(conf);
		} catch (IOException e) {
			LOG.error("Exception " + e);
			e.printStackTrace();
		    System.exit(-1);
		}
		
		CommandLine commandLine = null;
		try {
			commandLine = new PosixParser().parse(options, args);
		} catch (ParseException e) {
			LOG.error("Could not parse: ", e);
			printUsageAndExit(options, -1);
		}

		// check for user-defined port setting, if so override the conf
		if (commandLine != null && commandLine.hasOption("port")) {
			String val = commandLine.getOptionValue("port");
			servlet.getConfiguration().setInt("rest.port", Integer.valueOf(val));
		    LOG.info("port set to " + val);
		}
		
	    // check for user-defined https port setting, if so override the conf
        if (commandLine != null && commandLine.hasOption("httpsport")) {
            String val = commandLine.getOptionValue("httpsport");
            servlet.getConfiguration().setInt("rest.https.port", Integer.valueOf(val));
            LOG.info("https port set to " + val);
        }

		// check if server should only process GET requests, if so override the conf
		if (commandLine != null && commandLine.hasOption("readonly")) {
			servlet.getConfiguration().setBoolean("rest.port.readonly", true);
		    LOG.info("readonly set to true");
		}

		@SuppressWarnings("unchecked")
		List<String> remainingArgs = commandLine != null ?	commandLine.getArgList() : new ArrayList<String>();
		if (remainingArgs.size() != 1) {
			printUsageAndExit(options, 1);
		}

		String command = remainingArgs.get(0);
		if ("start".equals(command)) {
			// continue and start container
		} else if ("stop".equals(command)) {
			System.exit(1);
		} else {
			printUsageAndExit(options, 1);
		}
		
		thrd = new Thread(this);
		thrd.start();	
	}
	
	public TrafodionRest(Configuration conf) {
		try {
			servlet = RESTServlet.getInstance(conf);
		} catch (IOException e) {
			LOG.error("Exception " + e);
			e.printStackTrace();
			return;
		}
		
		thrd = new Thread(this);
		thrd.start();	
	}

    public void run() {
	    VersionInfo.logVersion();

	    // Set the default max thread number to 100 to limit
        // the number of concurrent requests so that REST server doesn't OOM easily.
        // Jetty sets the default max thread number to 250, if we don't set it.
        //
        // Our default min thread number 2 is the same as that used by Jetty.
        int maxThreads = servlet.getConfiguration().getInt("rest.threads.max", 100);
        int minThreads = servlet.getConfiguration().getInt("rest.threads.min", 2);
        QueuedThreadPool threadPool = new QueuedThreadPool(maxThreads,minThreads);
        Server server = new Server(threadPool);
        
        int httpPort = servlet.getConfiguration().getInt("rest.port", 4200);
        int httpsPort = servlet.getConfiguration().getInt("rest.https.port", 4201);
        
        HttpConfiguration httpConfig = new HttpConfiguration();
        httpConfig.setOutputBufferSize(32768);

        ServerConnector http = new ServerConnector(server,
                new HttpConnectionFactory(httpConfig));
        http.setPort(httpPort);
        http.setIdleTimeout(30000);
        LOG.info("Setup HTTP on port:" + httpPort);
	    
	    //Get the obfuscated SSL keystore password property. This is set by Trafodion installer
	    //Presence of property triggers SSL setup otherwise SSL disabled.
	    //See www.eclipse.org/jetty/documentation/current/configuring-security-secure-passwords.html for more detail
	    boolean setupSSL = false;
	    String sKeyStoreObfPswd = servlet.getConfiguration().get(Constants.REST_SSL_PASSWORD, ""); 
	    if(sKeyStoreObfPswd.length() > 0) {
	        setupSSL = true;
	    } else {
	        LOG.info("SSL disabled");
	    }

	    if(setupSSL) {
	        LOG.info("Setup HTTPS on port:" + httpsPort);
	        String sKeyStore = servlet.getConfiguration().get(Constants.REST_KEYSTORE, Constants.DEFAULT_REST_KEYSTORE); 
	        String sKeyStorePswd = new Password(sKeyStoreObfPswd).toString();
	        SslContextFactory sslContextFactory = new SslContextFactory();
	        sslContextFactory.setKeyStorePath(sKeyStore);
	        sslContextFactory.setKeyStoreType("JKS");
	        sslContextFactory.setKeyStorePassword(sKeyStorePswd);
	        sslContextFactory.setKeyManagerPassword(sKeyStorePswd);

	        //Create the keystore 
	        String keyStoreCommand = servlet.getConfiguration().get(Constants.REST_KEYSTORE_COMMAND,Constants.DEFAULT_REST_KEYSTORE_COMMAND); 
	        if(LOG.isDebugEnabled())
	            LOG.debug("keyStoreCommand:" + keyStoreCommand);
	        String command = keyStoreCommand
	                .replace(Constants.REST_KEYSTORE, sKeyStore)
	                .replace(Constants.REST_SSL_PASSWORD, sKeyStorePswd);
	        if(LOG.isDebugEnabled())
	            LOG.debug("command:" + command);
	        ScriptContext scriptContext = new ScriptContext();
	        scriptContext.setScriptName(Constants.SYS_SHELL_SCRIPT_NAME);
	        scriptContext.setCommand(command);
	        if(LOG.isDebugEnabled())
	            LOG.debug("keystore exec [" + scriptContext.getCommand() + "]");
	        ScriptManager.getInstance().runScript(scriptContext);// This will block while script runs
	        if(LOG.isDebugEnabled())
	            LOG.debug("keystore exit [" + scriptContext.getExitCode() + "]");
	        StringBuilder sb = new StringBuilder();
	        sb.append("exit code [" + scriptContext.getExitCode() + "]");
	        if (!scriptContext.getStdOut().toString().isEmpty())
	            sb.append(", stdout [" + scriptContext.getStdOut().toString()  + "]");
	        if (!scriptContext.getStdErr().toString().isEmpty())
	            sb.append(", stderr [" + scriptContext.getStdErr().toString()  + "]");
	        if(LOG.isDebugEnabled())
	            LOG.debug(sb.toString());

	        switch (scriptContext.getExitCode()) {
	        case 0:
	            LOG.info("Keystore created successfully");
	            break;
	        default:
	            LOG.error("Keystore creation error [" + scriptContext.getStdOut().toString() + "]...exiting");
	            System.exit(-1);
	        }
	        
	        HttpConfiguration httpsConfig = new HttpConfiguration(httpConfig);
	        httpsConfig.addCustomizer(new SecureRequestCustomizer());
	        httpsConfig.setSecureScheme("https");
	        httpConfig.setSecurePort(httpsPort);

	        ServerConnector https = new ServerConnector(server,
	                new SslConnectionFactory(sslContextFactory, "http/1.1"),
	                new HttpConnectionFactory(httpsConfig));
	        https.setPort(httpsPort);
	        https.setIdleTimeout(500000);
	        server.setConnectors(new Connector[] { http, https });     
	    } else {
	        server.setConnectors(new Connector[] { http });
	    }

	    // set up the Jersey servlet container for Jetty
	    ServletHolder sh = new ServletHolder(ServletContainer.class);
	    sh.setInitParameter(
	            "com.sun.jersey.config.property.resourceConfigClass",
	            ResourceConfig.class.getCanonicalName());
	    sh.setInitParameter("com.sun.jersey.config.property.packages","org.trafodion.rest");

	    ServletContextHandler ctxHandler = new ServletContextHandler(
	            ServletContextHandler.SESSIONS);
	    ctxHandler.setContextPath("/");
	    ctxHandler.addServlet(sh, "/*");
	    server.setHandler(ctxHandler);
	    server.setStopAtShutdown(true);

	    try {
	        server.start();
	        server.join();
	    } catch (InterruptedException e) {
	        LOG.error("InterruptedException " + e);
	    } catch (Exception e) {
	        LOG.error("Exception " + e);
	    }
	}

	public static void main(String[] args) throws Exception {
		TrafodionRest server = new TrafodionRest(args);
	}
}