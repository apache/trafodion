/*
package org.trafodion.wms.rest;

import java.io.IOException;
import java.util.List;
import org.mortbay.jetty.Connector;
import org.mortbay.jetty.Server;
import org.mortbay.jetty.nio.SelectChannelConnector;
import org.apache.hadoop.conf.Configuration;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;
import org.trafodion.wms.util.WmsConfiguration;
import org.trafodion.wms.util.VersionInfo;
import org.trafodion.wms.server.WorkloadsQueue;

public class RestServer implements Runnable {  
	private static final Log LOG = LogFactory.getLog(RestServer.class.getName());
	private Configuration conf;
	private WorkloadsQueue wlq;
	private Thread thrd;
	
	public RestServer(WorkloadsQueue wlq){
	    conf = WmsConfiguration.create();
	    this.wlq = wlq; 
		thrd = new Thread(this);
		thrd.start();
	}
	
	public void run() {
		VersionInfo.logVersion();
		
		Server server = new Server(conf.getInt("wms.rest.port", 9999));
		server.setSendServerVersion(false);
		server.setSendDateHeader(false);
		server.setStopAtShutdown(true);
		server.setHandler(new RestHandler(this.wlq));
		try {
			server.start();
			LOG.info("REST Server listening on port:" + conf.getInt("wms.rest.port", 9999));
			server.join();
		} catch (InterruptedException e) {
			LOG.error("InterruptedException " + e);
		} catch (Exception e) {
			LOG.error("Exception " + e);
		}

	}
}
*/