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

package org.apache.hadoop.hbase.client.transactional;
import org.apache.zookeeper.KeeperException;

import java.io.IOException;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;


/**
 * Multi Data Center specific
 */
public class PeerInfo {

    static final Log LOG = LogFactory.getLog(PeerInfo.class);

    public static final String TRAFODION_UP             = "tup";
    public static final String TRAFODION_DOWN           = "tdn";
    public static final String HBASE_UP                 = "hup";
    public static final String HBASE_DOWN               = "hdn";
    public static final String STR_UP                   = "sup";
    public static final String STR_DOWN                 = "sdn";

    private String m_id;
    private String m_quorum;
    private String m_port;
    private boolean m_HBaseUp;
    private boolean m_TrafodionUp;
    private boolean m_STRUp;

    public PeerInfo() {
	if (LOG.isTraceEnabled()) LOG.trace("PeerInfo (s,s,s,s) -- ENTRY");

	m_id = null;
	m_quorum = null;
	m_port = null;
	m_HBaseUp = false;
	m_TrafodionUp = false;
	m_STRUp = false;

    }

    public PeerInfo(String pv_id, 
		    String pv_quorum, 
		    String pv_port, 
		    String pv_status) 
    {
	if (LOG.isTraceEnabled()) LOG.trace("PeerInfo (s,s,s,s) -- ENTRY");

	m_id = pv_id;
	m_quorum = pv_quorum;
	m_port = pv_port;
	m_HBaseUp = false;
	m_TrafodionUp = false;
	m_STRUp = false;
	set_internal_status_fields(pv_status);
    }

    public String get_id() {
	return m_id;
    }

    public void set_id(String pv_id) {
	m_id = pv_id;
    }

    public String get_quorum() {
	return m_quorum;
    }

    public void set_quorum(String pv_quorum) {
	m_quorum = pv_quorum;
    }

    public void set_quorum(byte[] pv_quorum) {
	if (pv_quorum != null) {
	    m_quorum = new String(pv_quorum);
	}
    }

    public String get_port() {
	return m_port;
    }

    public void set_port(String pv_port) {
	m_port = pv_port;
    }

    public void set_port(byte[] pv_port) {
	if (pv_port != null) {
	    m_port = new String(pv_port);
	}
    }

    public String get_status() {
	StringBuilder lv_sb = new StringBuilder();
	
	get_status(lv_sb);

	return lv_sb.toString();
    }

    public void get_status(StringBuilder pv_sb) {

	if (pv_sb == null) {
	    return;
	}

	if (m_TrafodionUp) {
	    pv_sb.append(TRAFODION_UP);
	}
	else {
	    pv_sb.append(TRAFODION_DOWN);
	}
	pv_sb.append("-");

	if (m_STRUp) {
	    pv_sb.append(STR_UP);
	}
	else {
	    pv_sb.append(STR_DOWN);
	}

    }

    private void set_internal_status_fields(String pv_status) {
	if (pv_status.contains(HBASE_UP)) {
	    m_HBaseUp = true;
	}
	if (pv_status.contains(HBASE_DOWN)) {
	    m_HBaseUp = false;
	}
	if (pv_status.contains(TRAFODION_UP)) {
	    m_TrafodionUp = true;
	}
	if (pv_status.contains(TRAFODION_DOWN)) {
	    m_TrafodionUp = false;
	}
	if (pv_status.contains(STR_UP)) {
	    m_STRUp = true;
	}
	if (pv_status.contains(STR_DOWN)) {
	    m_STRUp = false;
	}
    }

    public void set_status(String pv_status) {
	set_internal_status_fields(pv_status);
    }
	
    public void set_status(byte[] pv_status) {
	if (pv_status != null) {
	    String lv_status = new String(pv_status);
	    set_internal_status_fields(lv_status);
	}
    }

    public void setHBaseStatus(boolean pv_status) {
	m_HBaseUp = pv_status;
    }

    public void setTrafodionStatus(boolean pv_status) {
	m_TrafodionUp = pv_status;
    }

    public void setSTRStatus(boolean pv_status) {
	m_STRUp = pv_status;
    }

    public boolean isHBaseUp() {
	return m_HBaseUp;
    }

    public boolean isTrafodionUp() {
	return m_TrafodionUp;
    }

    public boolean isSTRUp() {
	return m_STRUp;
    }

    public String toString()  
    {
	StringBuilder lv_sb = new StringBuilder();
	lv_sb = lv_sb
	    .append(m_id)
	    .append(":")
	    .append(m_quorum)
	    .append(":")
	    .append(m_port)
	    .append(":");

	get_status(lv_sb);


	return lv_sb.toString();
    }

    public static void main(String [] Args) throws Exception 
    {
	PeerInfo lv_peer = new PeerInfo("1", "q", "24000", "sup");
	System.exit(0);
    }

}
