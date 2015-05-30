package org.trafodion.wms.rest;

import java.io.IOException;
import java.net.InetSocketAddress;
import java.util.*;

import org.trafodion.wms.Constants;
import org.trafodion.wms.rest.RestConstants;
import org.trafodion.wms.rest.model.WorkloadModel;
import org.trafodion.wms.rest.model.WorkloadListModel;

import org.apache.hadoop.conf.Configuration;
import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

/**
 * Singleton class encapsulating global REST servlet state and functions.
 */
public class RESTServlet implements RestConstants {
    private static final Log LOG = LogFactory.getLog(RESTServlet.class);
    private static RESTServlet INSTANCE;
    private final Configuration conf;

    /**
     * @return the RESTServlet singleton instance
     * @throws IOException
     */
    public synchronized static RESTServlet getInstance() throws IOException {
        assert (INSTANCE != null);
        return INSTANCE;
    }

    /**
     * @param conf
     *            Existing configuration to use in rest servlet
     * @return the RESTServlet singleton instance
     * @throws IOException
     */
    public synchronized static RESTServlet getInstance(Configuration conf)
            throws IOException {
        if (INSTANCE == null) {
            INSTANCE = new RESTServlet(conf);
        }
        return INSTANCE;
    }

    public synchronized static void stop() {
        if (INSTANCE != null)
            INSTANCE = null;
    }

    /**
     * Constructor with existing configuration
     * 
     * @param conf
     *            existing configuration
     * @throws IOException.
     */
    RESTServlet(Configuration conf) throws IOException {
        this.conf = conf;
    }

    Configuration getConfiguration() {
        return conf;
    }

    /**
     * Helper method to determine if server should only respond to GET HTTP
     * method requests.
     * 
     * @return boolean for server read-only state
     */
    boolean isReadOnly() {
        return getConfiguration().getBoolean("wms.rest.readonly", false);
    }
}
