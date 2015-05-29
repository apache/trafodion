package org.trafodion.wms.server.store;

import java.util.*;
import java.io.IOException;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

public class WorkloadStats {
	private static  final Log LOG = LogFactory.getLog(WorkloadStats.class);
	private long timestamp;
	
	WorkloadStats() {
		timestamp = System.currentTimeMillis();
	}
	
	long getTimestamp() {
		return timestamp;
	}

}