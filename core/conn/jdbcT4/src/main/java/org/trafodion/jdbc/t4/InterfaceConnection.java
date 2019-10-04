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

package org.trafodion.jdbc.t4;

import java.io.File;
import java.io.UnsupportedEncodingException;
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.CharBuffer;
import java.nio.charset.CharacterCodingException;
import java.nio.charset.Charset;
import java.nio.charset.CharsetDecoder;
import java.nio.charset.CharsetEncoder;
import java.nio.charset.CodingErrorAction;
import java.nio.charset.UnsupportedCharsetException;
import java.sql.Connection;
import java.sql.SQLClientInfoException;
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.util.Hashtable;
import java.util.HashMap;
import java.util.Locale;
import java.util.Properties;
import java.util.logging.Handler;
import java.util.logging.Level;


class InterfaceConnection {
	static final int MODE_SQL = 0;
	static final int MODE_WMS = 1;
	static final int MODE_CMD = 2;
	
	static final short SQL_COMMIT = 0;
	static final short SQL_ROLLBACK = 1;
	private int activeTimeBeforeCancel = -1;
	private int txnIsolationLevel = Connection.TRANSACTION_READ_COMMITTED;
	private boolean autoCommit = true;
	private boolean isReadOnly = false;
	private boolean isClosed_;
	private long txid;
	private Locale locale;
	private USER_DESC_def userDesc;
	private CONNECTION_CONTEXT_def inContext;
	OUT_CONNECTION_CONTEXT_def outContext;
	private boolean useArrayBinding_;
	private short transportBufferSize_;
	Handler t4FileHandler;
	private NCSAddress ncsAddr_;
	private T4Connection t4connection_;
	private String m_ncsSrvr_ref;
	private int dialogueId_;
	private String m_sessionName;

	// character set information
	private int isoMapping_ = 15;
	private int termCharset_ = 15;
	private boolean enforceISO = false;
	private boolean byteSwap = false;
	private String _serverDataSource;

	private int _mode = MODE_SQL;
	
	T4Properties t4props_;
	SQLWarning sqlwarning_;

	HashMap encoders = new HashMap(11);
	HashMap decoders = new HashMap(11);

	// static fields from odbc_common.h and sql.h
	static final int SQL_TXN_READ_UNCOMMITTED = 1;
	static final int SQL_TXN_READ_COMMITTED = 2;
	static final int SQL_TXN_REPEATABLE_READ = 4;
	static final int SQL_TXN_SERIALIZABLE = 8;
	static final short SQL_ATTR_CURRENT_CATALOG = 109;
	static final short SQL_ATTR_ACCESS_MODE = 101;
	static final short SQL_ATTR_AUTOCOMMIT = 102;
	static final short SQL_TXN_ISOLATION = 108;
	static final short SET_SCHEMA = 1001; // this value is follow server side definition

	// spj proxy syntax support
	static final short SPJ_ENABLE_PROXY = 1040;

	static final int PASSWORD_SECURITY = 0x4000000; //(2^26)
	static final int ROWWISE_ROWSET = 0x8000000; // (2^27);
	static final int CHARSET = 0x10000000; // (2^28)
	static final int STREAMING_DELAYEDERROR_MODE = 0x20000000; // 2^29
	// Zbig added new attribute on 4/18/2005
	static final short JDBC_ATTR_CONN_IDLE_TIMEOUT = 3000;
	static final short RESET_IDLE_TIMER = 1070;

	// for handling WeakReferences
	static ReferenceQueue refQ_ = new ReferenceQueue();
	static Hashtable refTosrvrCtxHandle_ = new Hashtable();

	//3196 - NDCS transaction for SPJ
	static final short SQL_ATTR_JOIN_UDR_TRANSACTION = 1041;
	static final short SQL_ATTR_SUSPEND_UDR_TRANSACTION = 1042;
	long transId_ = 0;
	boolean suspendRequest_ = false; 

	private String _roleName = "";
	private boolean _ignoreCancel;
	
	private long _seqNum = 0;
	private SecPwd _security;
	long currentTime;
	
	private TrafT4Connection _t4Conn;
	private String _remoteProcess;
	private String _connStringHost = "";

        int getActiveTimeBeforeCancel() { return activeTimeBeforeCancel; }
	InterfaceConnection(TrafT4Connection conn, T4Properties t4props) throws SQLException {
		_t4Conn = conn;
		t4props_ = t4props;
		_remoteProcess = "";

		// close any weak connections that need to be closed.
		gcConnections();

		if (t4props.getSQLException() != null) {
			throw TrafT4Messages.createSQLException(t4props_, t4props.getLocale(), "invalid_property", t4props
					.getSQLException());
		}

		m_sessionName = t4props_.getSessionName();

		if (m_sessionName != null && m_sessionName.length() > 0) {
			if (m_sessionName.length() > 24)
				m_sessionName = m_sessionName.substring(0, 24);

			if (!m_sessionName.matches("\\w+"))
				throw new SQLException("Invalid sessionName.  Session names can only contain alphnumeric characters.");
		}

		locale = t4props.getLocale();
		txid = 0;
		isClosed_ = false;
		useArrayBinding_ = t4props.getUseArrayBinding();
		// transportBufferSize_ = t4props.getTransportBufferSize();
		transportBufferSize_ = 32000;

		userDesc = getUserDescription(t4props.getUser());

		// Connection context details
		inContext = getInContext(t4props);
		m_ncsSrvr_ref = t4props.getUrl();
		_ignoreCancel = t4props.getIgnoreCancel();

		if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities.makeParams(t4props_, t4props);
			String temp = "url is = " + t4props.getUrl();
			t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "", temp, p);
			p = T4LoggingUtilities.makeParams(t4props_, t4props);
			temp = "user is = " + userDesc.userName;
			t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "", temp, p);
		}
		sqlwarning_ = null;

        this.termCharset_ =
                t4props.getClientCharset() == null ? InterfaceUtilities.SQLCHARSETCODE_UTF8
                        : InterfaceUtilities.getCharsetValue(t4props.getClientCharset());

		connect();
	}
	
	public void setConnStrHost(String host) {
		this._connStringHost = host;
	}
	
	public int getMode() {
		return this._mode;
	}
	
	public void setMode(int mode) {
		this._mode = mode;
	}
	
	public long getSequenceNumber() {
		if(++_seqNum < 0) {
			_seqNum = 1;
		}
		
		return _seqNum;
	}
	
	public String getRemoteProcess() throws SQLException {
		return _remoteProcess;
	}

	public boolean isClosed() {
		return this.isClosed_;
	}

	String getRoleName() {
		return this._roleName;
	}

	boolean getIgnoreCancel() {
		return this._ignoreCancel;
	}

	CONNECTION_CONTEXT_def getInContext() {
		return inContext;
	}

	private CONNECTION_CONTEXT_def getInContext(T4Properties t4props) {
		inContext = new CONNECTION_CONTEXT_def();
		inContext.catalog = t4props.getCatalog();
        inContext.setSchema(t4props.getSchema());
		inContext.datasource = t4props.getServerDataSource();
		inContext.userRole = t4props.getRoleName();
		inContext.cpuToUse = t4props.getCpuToUse();
		inContext.cpuToUseEnd = -1; // for future use by DBTransporter

		inContext.accessMode = (short) (isReadOnly ? 1 : 0);
		inContext.autoCommit = (short) (autoCommit ? 1 : 0);

		inContext.queryTimeoutSec = t4props.getQueryTimeout();
		inContext.idleTimeoutSec = (short) t4props.getConnectionTimeout();
		inContext.clipVarchar = (short) t4props.getClipVarchar();
		inContext.loginTimeoutSec = (short) t4props.getLoginTimeout();
		inContext.txnIsolationLevel = (short) SQL_TXN_READ_COMMITTED;
		inContext.rowSetSize = t4props.getFetchBufferSize();
		inContext.diagnosticFlag = 0;
		inContext.processId = (int) System.currentTimeMillis() & 0xFFF;

		try {
			inContext.computerName = InetAddress.getLocalHost().getHostName();
		} catch (UnknownHostException uex) {
			inContext.computerName = "Unknown Client Host";
		}
		inContext.windowText = t4props.getApplicationName();

		inContext.ctxDataLang = 15;
		inContext.ctxErrorLang = 15;

		inContext.ctxACP = 1252;
		inContext.ctxCtrlInferNXHAR = -1;
		inContext.clientVersionList.list = getVersion(inContext.processId);
		return inContext;
	}

	private VERSION_def[] getVersion(int pid) {
		short majorVersion = 3;
		short minorVersion = 0;
		int buildId = 0;

		VERSION_def version[] = new VERSION_def[2];

		// Entry [0] is the Driver Version information
		version[0] = new VERSION_def();
		version[0].componentId = 20;
		version[0].majorVersion = majorVersion;
		version[0].minorVersion = minorVersion;
		version[0].buildId = buildId | ROWWISE_ROWSET | CHARSET | PASSWORD_SECURITY;
		
		if (this.t4props_.getDelayedErrorMode())
	    {
	      version[0].buildId |= STREAMING_DELAYEDERROR_MODE;
	    }

		// Entry [1] is the Application Version information
		version[1] = new VERSION_def();
		version[1].componentId = 8;
		version[1].majorVersion = 3;
		version[1].minorVersion = 0;
		version[1].buildId = 0;

		return version;
	}

	USER_DESC_def getUserDescription() {
		return userDesc;
	}

	private void setISOMapping(int isoMapping) {
//		if (InterfaceUtilities.getCharsetName(isoMapping) == InterfaceUtilities.SQLCHARSET_UNKNOWN)
//			isoMapping = InterfaceUtilities.getCharsetValue("ISO8859_1");

		isoMapping_ = InterfaceUtilities.getCharsetValue("UTF-8");;
	}

	String getServerDataSource() {
		return this._serverDataSource;
	}

	boolean getEnforceISO() {
		return enforceISO;
	}

	int getISOMapping() {
		return isoMapping_;
	}

	public String getSessionName() {
		return m_sessionName;
	}

	private void setTerminalCharset(int termCharset) {
//		if (InterfaceUtilities.getCharsetName(termCharset) == InterfaceUtilities.SQLCHARSET_UNKNOWN)
//			termCharset = InterfaceUtilities.getCharsetValue("ISO8859_1");

		termCharset_ = InterfaceUtilities.getCharsetValue("UTF-8");;
	}

	int getTerminalCharset() {
		return termCharset_;
	}

	private USER_DESC_def getUserDescription(String user) throws SQLException {
		userDesc = new USER_DESC_def();
		userDesc.userDescType = (this.t4props_.getSessionToken()) ? TRANSPORT.PASSWORD_ENCRYPTED_USER_TYPE
				: TRANSPORT.UNAUTHENTICATED_USER_TYPE;
		userDesc.userName = (user.length() > 128) ? user.substring(0, 128) : user;
		userDesc.domainName = "";

		userDesc.userSid = null;
		userDesc.password = null; //we no longer want to send the password to the MXOAS

		return userDesc;
	}
	
	void writeToOutFile(byte[] input, String file)
	   {
		   java.io.DataOutputStream os = null;
		   try {
		      os = new java.io.DataOutputStream
		      (new java.io.FileOutputStream(file));
	          os.write(input, 0, input.length);
		   }catch (java.io.IOException io) {
			   System.out.println("IO exception");
		   }finally {
			   if (os != null)
				   try {
				      os.close();
				   }catch (java.io.IOException io) {
					   System.out.println("IO exception");
				   }
		   }
	   }
	
	private void oldEncryptPassword() throws SQLException {
		String pwd = this.t4props_.getPassword();
		
		if (pwd.length() > 386)
			pwd = pwd.substring(0, 386);

		byte [] authentication;
		try {
			authentication = pwd.getBytes("US-ASCII");
		} catch (UnsupportedEncodingException uex) {
			throw TrafT4Messages.createSQLException(t4props_, locale, uex.getMessage(), null);
		}

		if (authentication.length > 0) {
			Utility.Encryption(authentication, authentication, authentication.length);
		}
		
		userDesc.password = authentication;
	}

	T4Connection getT4Connection() {
		return t4connection_;
	}

	int getDialogueId() {
		return dialogueId_;
	}

	int getQueryTimeout() {
		return inContext.queryTimeoutSec;
	}

	int getLoginTimeout() {
		return inContext.loginTimeoutSec;
	}

	int getConnectionTimeout() {
		return inContext.idleTimeoutSec;
	}
	short getClipVarchar() {
		return inContext.clipVarchar;
	}

	String getCatalog() {
		if (outContext != null) {
			return outContext.catalog;
		} else {
			return inContext.catalog;
		}
	}

	boolean getDateConversion() {
		return ((outContext.versionList.list[0].buildId & 512) > 0);
	}

	int getServerMajorVersion() {
		return outContext.versionList.list[1].majorVersion;
	}

	int getServerMinorVersion() {
		return outContext.versionList.list[1].minorVersion;
	}

	String getUid() {
		return userDesc.userName;
	}

    String getSchema() {
        if (outContext != null) {
            return outContext.getSchema();
        } else {
            return inContext.getSchema();
        }
    }

    void setSchemaDirect(String schema) {
        outContext.setSchema(schema);
    }
    void setSchema(TrafT4Connection conn, String schema) throws SQLException {
        if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
            Object p[] = T4LoggingUtilities.makeParams(conn.props_, schema);
            String temp = "Setting connection schema = " + schema;
            t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "setSchema", temp, p);
        }
        if (schema == null || schema.length() == 0) {
            return;
        }
        setConnectionAttr(conn, SET_SCHEMA, 0, schema);
        setSchemaDirect(schema);
        if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
            Object p[] = T4LoggingUtilities.makeParams(conn.props_, schema);
            String temp = "Setting connection schema = " + schema + " is done.";
            t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "setSchema", temp, p);
        }
    }

	void setLocale(Locale locale) {
		this.locale = locale;
	}

	Locale getLocale() {
		return locale;
	}

	boolean getByteSwap() {
		return this.byteSwap;
	}

	NCSAddress getNCSAddress() {
		return ncsAddr_;
	}
	
	void commit() throws SQLException {
		endTransaction(SQL_COMMIT);
	}

	void rollback() throws SQLException {
		endTransaction(SQL_ROLLBACK);
	}

	void cancel(long startTime) throws SQLException 
	{
		String errorText = null;
		long currentTime;
		if (startTime != -1) {
			if (activeTimeBeforeCancel != -1) {
				currentTime = System.currentTimeMillis();
				if ((activeTimeBeforeCancel * 1000) < (currentTime - startTime))
					return; 
			}
		}

		String srvrObjRef = "" + ncsAddr_.getPort();
		// String srvrObjRef = t4props_.getServerID();
		int srvrType = 2; // AS server
		CancelReply cr_ = null;
	
			if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(t4props_);
			String temp = "cancel request received for " + srvrObjRef;
				t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "connect", temp, p);
		}
	
		cr_ = T4_Dcs_Cancel.cancel(t4props_, this, dialogueId_, srvrType, srvrObjRef, 0);
	
		switch (cr_.m_p1_exception.exception_nr) {
		case TRANSPORT.CEE_SUCCESS:
				if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
					Object p[] = T4LoggingUtilities.makeParams(t4props_);
					String temp = "Cancel successful";
					t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "connect", temp, p);
				}
			break;
		default:
	
			//
			// Some unknown error
			//
				if (cr_.m_p1_exception.clientErrorText != null) {
					errorText = "Client Error text = " + cr_.m_p1_exception.clientErrorText;
				}
				errorText = errorText + "  :Exception = " + cr_.m_p1_exception.exception_nr;
				errorText = errorText + "  :" + "Exception detail = " + cr_.m_p1_exception.exception_detail;
				errorText = errorText + "  :" + "Error code = " + cr_.m_p1_exception.errorCode;
	
				if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
					Object p[] = T4LoggingUtilities.makeParams(t4props_);
					String temp = errorText;
					t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "cancel", temp, p);
				}
				throw TrafT4Messages.createSQLException(t4props_, locale, "as_cancel_message_error", errorText);
		} // end switch
	}
	
	private void initDiag(boolean setTimestamp, boolean downloadCert) throws SQLException {
		short retryCount = 3;
		InitializeDialogueReply idr = null;
		long endTime = (inContext.loginTimeoutSec > 0) ? currentTime + inContext.loginTimeoutSec * 1000 : Long.MAX_VALUE;
		int tryNum = 0;
		boolean done = false;

		boolean socketException = false;
		SQLException seSave = null;

		do {
			if (t4props_.t4Logger_.isLoggable(Level.INFO)) {
				String temp = "Attempting initDiag.  Try " + (tryNum + 1) + " of " + retryCount;
				t4props_.t4Logger_.logp(Level.INFO, "InterfaceConnection", "connect", temp, t4props_);
			}

			socketException = false;
			try {
				t4connection_ = new T4Connection(this);
				idr = t4connection_.InitializeDialogue(setTimestamp, downloadCert);
			} catch (SQLException se) {
				//
				// We will retry socket exceptions, but will fail on all other
				// exceptions.
				//
				int sc = se.getErrorCode();
				int s1 = TrafT4Messages.createSQLException(t4props_, locale, "socket_open_error", null).getErrorCode();
				int s2 = TrafT4Messages.createSQLException(t4props_, locale, "socket_write_error", null).getErrorCode();
				int s3 = TrafT4Messages.createSQLException(t4props_, locale, "socket_read_error", null).getErrorCode();

				if (sc == s1 || sc == s2 || sc == s3) {
					if (t4props_.t4Logger_.isLoggable(Level.INFO)) {
						String temp = "A socket exception occurred: " + se.getMessage();
						t4props_.t4Logger_.logp(Level.INFO, "InterfaceConnection", "connect", temp, t4props_);
					}

					socketException = true;
					seSave = se;
				} else {
					if (t4props_.t4Logger_.isLoggable(Level.INFO)) {
						String temp = "A non-socket fatal exception occurred: " + se.getMessage();
						t4props_.t4Logger_.logp(Level.INFO, "InterfaceConnection", "connect", temp, t4props_);
					}

					try {
						t4connection_.getInputOutput().closeIO();
					} catch (Exception e) {
						// ignore error
					}
					
					throw se;
				}
			}

			if (socketException == false) {
				if (idr.exception_nr == TRANSPORT.CEE_SUCCESS) {
					done = true;
					if (t4props_.t4Logger_.isLoggable(Level.INFO)) {
						String temp = "initDiag Successful.";
						t4props_.t4Logger_.logp(Level.INFO, "InterfaceConnection", "connect", temp, t4props_);
					}
				} else if (idr.exception_nr == odbc_SQLSvc_InitializeDialogue_exc_.odbc_SQLSvc_InitializeDialogue_SQLError_exn_ || 
						idr.exception_nr == odbc_SQLSvc_InitializeDialogue_exc_.odbc_SQLSvc_InitializeDialogue_InvalidUser_exn_) {
					if (t4props_.t4Logger_.isLoggable(Level.INFO)) {
						String temp = "A SQL Warning or Error occurred during initDiag: " + idr.SQLError;
						t4props_.t4Logger_.logp(Level.INFO, "InterfaceConnection", "connect", temp, t4props_);
					}

					int ex_nr = idr.exception_nr;
					int ex_nr_d = idr.exception_detail;

					if (ex_nr_d == odbc_SQLSvc_InitializeDialogue_exc_.SQL_PASSWORD_EXPIRING ||
							ex_nr_d == odbc_SQLSvc_InitializeDialogue_exc_.SQL_PASSWORD_GRACEPERIOD) {
						TrafT4Messages.setSQLWarning(this.t4props_, this._t4Conn, idr.SQLError);
						done = true;
					} else {
						TrafT4Messages.throwSQLException(t4props_, idr.SQLError);
					}
				}
			}

			currentTime = System.currentTimeMillis();
			tryNum = tryNum + 1;
		} while (done == false && endTime > currentTime && tryNum < retryCount);

		if (done == false) {
			SQLException se1;
			SQLException se2;

			if (socketException == true) {
				throw seSave;
			}

			if (currentTime >= endTime) {
				se1 = TrafT4Messages.createSQLException(t4props_, locale, "ids_s1_t00", null);
			} else if (tryNum >= retryCount) {
				se1 = TrafT4Messages.createSQLException(t4props_, locale, "as_connect_message_error",
						"exceeded retry count");
			} else {
				se1 = TrafT4Messages.createSQLException(t4props_, locale, "as_connect_message_error", null);
			}
			throw se1;
		}

		//
		// Set the outcontext value returned by the ODBC MX server in the
		// serverContext
		//
		outContext = idr.outContext;
		enforceISO = outContext._enforceISO;
		this._roleName = outContext._roleName;
		this._ignoreCancel = outContext._ignoreCancel;

		t4props_.setDialogueID(Integer.toString(dialogueId_));
		t4props_.setServerID(m_ncsSrvr_ref);

		t4props_.setNcsMajorVersion(idr.outContext.versionList.list[0].majorVersion);
		t4props_.setNcsMinorVersion(idr.outContext.versionList.list[0].minorVersion);
		t4props_.setSqlmxMajorVersion(idr.outContext.versionList.list[1].majorVersion);
		t4props_.setSqlmxMinorVersion(idr.outContext.versionList.list[1].minorVersion);

		if (t4props_.t4Logger_.isLoggable(Level.INFO)) {
			String temp = "Connection process successful";
			t4props_.t4Logger_.logp(Level.INFO, "InterfaceConnection", "connect", temp, t4props_);
		}
	}
	
	private void encryptPassword() throws SecurityException, SQLException {
		byte [] pwBytes;
		byte [] roleBytes;
		
		String roleName = t4props_.getRoleName();
		
		try {
			pwBytes = t4props_.getPassword().getBytes("US-ASCII");
			roleBytes = (roleName != null && roleName.length() > 0)?roleName.getBytes("US-ASCII"):null;
		}
		catch (UnsupportedEncodingException uex) {
			//ERROR MESSAGE
			throw new SQLException("failed to find encoding");
		}
		
		userDesc.password = new byte[_security.getPwdEBufferLen()];
	
		_security.encryptPwd(pwBytes, roleBytes, userDesc.password);
	}
	
	private byte [] createProcInfo(int pid, int nid, byte [] timestamp) throws SQLException {
		byte [] procInfo;

		procInfo = new byte[16];
		
		ByteBuffer bb = ByteBuffer.allocate(16);
		bb.order(ByteOrder.LITTLE_ENDIAN);
		bb.putInt(pid);
		bb.putInt(nid);
		bb.put(timestamp);
		bb.rewind();
		bb.get(procInfo, 0, 16);
		
		return procInfo;
	}

	private void secureLogin(ConnectReply cr) throws SQLException {
		try {
			byte [] procInfo = createProcInfo(cr.processId, cr.serverNode, cr.timestamp);
			boolean tokenAuth = this.t4props_.getSPJEnv() && this.t4props_.getTokenAuth();
			
			_security = SecPwd.getInstance(
					this._t4Conn,
					this.t4props_.getCertificateDir(),
					this.t4props_.getCertificateFile(),
					cr.clusterName, 
					tokenAuth,
					procInfo
					);
		}
		catch(SecurityException se) {
			CleanupServer(); //MXOSRVR is expecting InitDiag, clean it up since we failed
			throw se;
		}

		try {
			_security.openCertificate();
			this.encryptPassword();
		}catch(SecurityException se) {	
			if(se.getErrorCode() != 29713 && se.getErrorCode() != 29721) {
				throw se; //we have a fatal error
			}
				
			DownloadCertificate(); //otherwise, download and continue
		}
		
		try {
			inContext.connectOptions = new String(_security.getCertExpDate());
			initDiag(true,false);
		}catch(SQLException e) {
			if(outContext != null && outContext.certificate != null) { //we got a certificate back, switch to it, continue
				_security.switchCertificate(outContext.certificate); 
			}
			else { 
				throw e;
			}
			
			inContext.connectOptions = new String(_security.getCertExpDate());
			this.encryptPassword();  //re-encrypt
			this.initDiag(true,false); //re-initdiag
		}
	}
	
	private void CleanupServer() throws SQLException {
		this.userDesc.userName = null;
		this.userDesc.password = null;
		
		try {
			initDiag(false,false); //send dummy init diag to clean up server
		}catch(SQLException e) {
			
		}

	}
	
	private void DownloadCertificate() throws SQLException {
		//attempt download
		this.userDesc.userName = null;
		this.userDesc.password = null;
		inContext.connectOptions = null;
		
		try {
			initDiag(true,true);
		}catch(SQLException e) {
			if(outContext == null || outContext.certificate == null) {
				SQLException he = TrafT4Messages.createSQLException(t4props_, this.locale, "certificate_download_error", e.getMessage());
				he.setNextException(e);
				throw he;
			}
		}
		
		this.userDesc.userName = this.t4props_.getUser();
		
		try {
			_security.switchCertificate(outContext.certificate);
			this.encryptPassword();
		}catch(SecurityException se1) {
			throw se1;
		}
	}

	private void connect() throws SQLException {
		short retryCount = 3;
		int srvrType = 2; // AS server
		ConnectReply cr = null;

		if (t4props_.t4Logger_.isLoggable(Level.INFO)) {
			String msg = "Association Server URL: " + m_ncsSrvr_ref;
			t4props_.t4Logger_.logp(Level.INFO, "InterfaceConnection", "connect", msg, t4props_);
		}

		//
		// Connect to the association server.
		//
		String errorText = null;
		boolean done = false;
		int tryNum = 0;
		String errorMsg = null;
		String errorMsg_detail = null;
		currentTime = System.currentTimeMillis();
		long endTime = (inContext.loginTimeoutSec > 0) ? currentTime + inContext.loginTimeoutSec * 1000
				: Long.MAX_VALUE;

		do {
			if (t4props_.t4Logger_.isLoggable(Level.INFO)) {
				String temp = "Attempting getObjRef.  Try " + (tryNum + 1) + " of " + retryCount;
				t4props_.t4Logger_.logp(Level.INFO, "InterfaceConnection", "connect", temp, t4props_);
			}

			cr = T4_Dcs_Connect.getConnection(t4props_, this, inContext, userDesc, srvrType,
					retryCount);

			switch (cr.m_p1_exception.exception_nr) {
			case TRANSPORT.CEE_SUCCESS:
				done = true;
				if (t4props_.t4Logger_.isLoggable(Level.INFO)) {
					String msg = "getObjRef Successful.  Server URL: " + cr.m_p2_srvrObjRef;
					t4props_.t4Logger_.logp(Level.INFO, "InterfaceConnection", "connect", msg, t4props_);
				}
				if (!cr.m_p4_dataSource.equals(t4props_.getServerDataSource())) {
					Object[] messageArguments = new Object[1];
					messageArguments[0] = cr.m_p4_dataSource;
					sqlwarning_ = TrafT4Messages.createSQLWarning(t4props_, "connected_to_Default_DS", messageArguments);
				}
				break;
			case odbc_Dcs_GetObjRefHdl_exc_.odbc_Dcs_GetObjRefHdl_ASTryAgain_exn_:
				done = false;
				tryNum = tryNum + 1;
				errorMsg = "as_connect_message_error";
				errorMsg_detail = "try again request";
				if(tryNum < retryCount) {
					try {
						Thread.sleep(5000);
					}catch(Exception e) {}
				}
				break;
			case odbc_Dcs_GetObjRefHdl_exc_.odbc_Dcs_GetObjRefHdl_ASNotAvailable_exn_:
				done = false;
				tryNum = tryNum + 1;
				errorMsg = "as_connect_message_error";
				errorMsg_detail = "association server not available";
				break;
			case odbc_Dcs_GetObjRefHdl_exc_.odbc_Dcs_GetObjRefHdl_DSNotAvailable_exn_:
				done = false;
				tryNum = tryNum + 1;
				errorMsg = "as_connect_message_error";
				errorMsg_detail = "data source not available";
				break;
			case odbc_Dcs_GetObjRefHdl_exc_.odbc_Dcs_GetObjRefHdl_PortNotAvailable_exn_:
				done = false;
				tryNum = tryNum + 1;
				errorMsg = "as_connect_message_error";
				errorMsg_detail = "port not available";
				break;
			case odbc_Dcs_GetObjRefHdl_exc_.odbc_Dcs_GetObjRefHdl_ASNoSrvrHdl_exn_:
				done = false;
				tryNum = tryNum + 1;
				errorMsg = "as_connect_message_error";
				errorMsg_detail = "server handle not available";
				break;
			default:

				//
				// Some unknown error
				//
				if (cr.m_p1_exception.clientErrorText != null) {
					errorText = "Client Error text = " + cr.m_p1_exception.clientErrorText;

				}
				errorText = errorText + "  :Exception = " + cr.m_p1_exception.exception_nr;
				errorText = errorText + "  :" + "Exception detail = " + cr.m_p1_exception.exception_detail;
				errorText = errorText + "  :" + "Error code = " + cr.m_p1_exception.errorCode;

				if (cr.m_p1_exception.ErrorText != null) {
					errorText = errorText + "  :" + "Error text = " + cr.m_p1_exception.ErrorText;

				}
				throw TrafT4Messages.createSQLException(t4props_, locale, "as_connect_message_error", errorText);
			}

			if (done == false && t4props_.t4Logger_.isLoggable(Level.INFO)) {
				String msg = "getObjRef Failed. Message from Association Server: " + errorMsg_detail;
				t4props_.t4Logger_.logp(Level.INFO, "InterfaceConnection", "connect", msg, t4props_);
			}

			currentTime = System.currentTimeMillis();
		} while (done == false && endTime > currentTime && tryNum < retryCount);

		if (done == false) {
			SQLException se1;
			SQLException se2;

			if (currentTime >= endTime) {
				se1 = TrafT4Messages.createSQLException(t4props_, locale, "ids_s1_t00", null);
				se2 = TrafT4Messages.createSQLException(t4props_, locale, errorMsg, errorMsg_detail);
				se1.setNextException(se2);
			} else {
				se1 = TrafT4Messages.createSQLException(t4props_, locale, errorMsg, errorMsg_detail);
			}

			throw se1;
		}
		
		dialogueId_ = cr.m_p3_dialogueId;
		m_ncsSrvr_ref = cr.m_p2_srvrObjRef;
		_remoteProcess = "\\" + cr.remoteHost + "." + cr.remoteProcess;
	
		ncsAddr_ = cr.getNCSAddress();
		this.byteSwap = cr.byteSwap;
		this._serverDataSource = cr.m_p4_dataSource;

		setISOMapping(cr.isoMapping);

		if (cr.isoMapping == InterfaceUtilities.getCharsetValue("ISO8859_1")) {
			this.inContext.ctxDataLang = 0;
			this.inContext.ctxErrorLang = 0;
		}
		
		if(cr.securityEnabled) {
			this.secureLogin(cr);
		}
		else {
			this.oldEncryptPassword();
			this.initDiag(false,false);
		}
        this.setConnectionAttr(this._t4Conn, TRANSPORT.SQL_ATTR_CLIPVARCHAR, this.inContext.clipVarchar, String.valueOf(this.inContext.clipVarchar));
	}

	// @deprecated
	void isConnectionClosed() throws SQLException {
		if (isClosed_ == false) {
			throw TrafT4Messages.createSQLException(t4props_, locale, "invalid_connection", null);
		}
	}

	// @deprecated
	void isConnectionOpen() throws SQLException {
		if (isClosed_) {
			throw TrafT4Messages.createSQLException(t4props_, locale, "invalid_connection", null);
		}
	}

	// @deprecated
	boolean getIsClosed() {
		return isClosed_;
	}

	void setIsClosed(boolean isClosed) {
		this.isClosed_ = isClosed;
	}

	String getUrl() {
		return m_ncsSrvr_ref;
	}

	void setCatalog(TrafT4Connection conn, String catalog) throws SQLException {
		if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities.makeParams(conn.props_, catalog);
			String temp = "Setting connection catalog = " + catalog;
			t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "setCatalog", temp, p);
		}
		if (catalog != null && catalog.length() == 0) {
			catalog = T4Properties.DEFAULT_CATALOG;
		}
		setConnectionAttr(conn, SQL_ATTR_CURRENT_CATALOG, 0, catalog);
		outContext.catalog = catalog;
		if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities.makeParams(conn.props_, catalog);
			String temp = "Setting connection catalog = " + catalog + " is done.";
			t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "setCatalog", temp, p);
		}
	};

	// enforces the connection timeout set by the user
	// to be called by the connection pooling mechanism whenever a connection is
	// given to the user from the pool
	void enforceT4ConnectionTimeout(TrafT4Connection conn) throws SQLException {
		if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities.makeParams(conn.props_, (short) t4props_.getConnectionTimeout());
			String temp = "Enforcing connection timeout = " + (short) t4props_.getConnectionTimeout();
			t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "enforceT4ConnectionTimeout", temp, p);
		}
		inContext.idleTimeoutSec = (short) t4props_.getConnectionTimeout();
		setConnectionAttr(conn, JDBC_ATTR_CONN_IDLE_TIMEOUT, inContext.idleTimeoutSec, String
				.valueOf(inContext.idleTimeoutSec));
		if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities.makeParams(conn.props_, (short) t4props_.getConnectionTimeout());
			String temp = "Enforcing connection timeout = " + (short) t4props_.getConnectionTimeout() + " is done.";
			t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "enforceT4ConnectionTimeout", temp, p);
		}
	};

	// disregards the T4's connectionTimeout value (set during initialize
	// dialog) and
	// enforces the connection timeout set by the NCS datasource settings
	// to be called by the connection pooling mechanism whenever a connection is
	// put into the pool (after a user has called connection.close())
	void disregardT4ConnectionTimeout(TrafT4Connection conn) throws SQLException {
		if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities.makeParams(conn.props_, "-1");
			String temp = "Setting connection timeout = -1";
			t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "disregardT4ConnectionTimeout", temp, p);
		}
		setConnectionAttr(conn, JDBC_ATTR_CONN_IDLE_TIMEOUT, -1, "-1");
		if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities.makeParams(conn.props_, "-1");
			String temp = "Setting connection timeout = -1 is done.";
			t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "disregardT4ConnectionTimeout", temp, p);
		}
	};

	void setConnectionAttr(TrafT4Connection conn, short attr, int valueNum, String valueString) throws SQLException {
		SetConnectionOptionReply scr_;
		isConnectionOpen();

		try {
			scr_ = t4connection_.SetConnectionOption(attr, valueNum, valueString);
			//3196 - NDCS transaction for SPJ
			if (attr == SQL_ATTR_JOIN_UDR_TRANSACTION) {
				transId_ = Long.valueOf(valueString);
				suspendRequest_ = true;
			}
			else if (attr == SQL_ATTR_SUSPEND_UDR_TRANSACTION) {
				transId_ = Long.valueOf(valueString);
				suspendRequest_ = false;
			}
		} catch (SQLException tex) {
			if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(conn.props_, attr, valueNum, valueString);
				String temp = "NDCS or SQLException occurred.";
				t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "setConnectionAttr", temp, p);
			}
			throw tex;
		}

		switch (scr_.m_p1.exception_nr) {
		case TRANSPORT.CEE_SUCCESS:

			// do the warning processing
			if (scr_.m_p2.length != 0) {
				TrafT4Messages.setSQLWarning(conn.props_, conn, scr_.m_p2);
			}
			if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(conn.props_, attr, valueNum, valueString);
				String temp = "Setting connection attribute is done.";
				t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "setConnectionAttr", temp, p);
			}
			break;
		case odbc_SQLSvc_SetConnectionOption_exc_.odbc_SQLSvc_SetConnectionOption_SQLError_exn_:
			if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(conn.props_, attr, valueNum, valueString);
				String temp = "odbc_SQLSvc_SetConnectionOption_SQLError_exn_ occurred.";
				t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "setConnectionAttr", temp, p);
			}
			TrafT4Messages.throwSQLException(t4props_, scr_.m_p1.errorList);
		default:
			if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(conn.props_, attr, valueNum, valueString);
				String temp = "UnknownException occurred.";
				t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "setConnectionAttr", temp, p);
			}
			throw TrafT4Messages.createSQLException(conn.props_, locale, "ids_unknown_reply_error", null);
		}
	};

	void setTransactionIsolation(TrafT4Connection conn, int level) throws SQLException {
		if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities.makeParams(conn.props_, level);
			String temp = "Setting transaction isolation = " + level;
			t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "setTransactionIsolation", temp, p);
		}
		isConnectionOpen();

		if (level != Connection.TRANSACTION_NONE && level != Connection.TRANSACTION_READ_COMMITTED
				&& level != Connection.TRANSACTION_READ_UNCOMMITTED && level != Connection.TRANSACTION_REPEATABLE_READ
				&& level != Connection.TRANSACTION_SERIALIZABLE) {
			throw TrafT4Messages.createSQLException(conn.props_, locale, "invalid_transaction_isolation", null);
		}

		txnIsolationLevel = level;

		switch (txnIsolationLevel) {
		case Connection.TRANSACTION_NONE:
			inContext.txnIsolationLevel = (short) SQL_TXN_READ_COMMITTED;
			break;
		case Connection.TRANSACTION_READ_COMMITTED:
			inContext.txnIsolationLevel = (short) SQL_TXN_READ_COMMITTED;
			break;
		case Connection.TRANSACTION_READ_UNCOMMITTED:
			inContext.txnIsolationLevel = (short) SQL_TXN_READ_UNCOMMITTED;
			break;
		case Connection.TRANSACTION_REPEATABLE_READ:
			inContext.txnIsolationLevel = (short) SQL_TXN_REPEATABLE_READ;
			break;
		case Connection.TRANSACTION_SERIALIZABLE:
			inContext.txnIsolationLevel = (short) SQL_TXN_SERIALIZABLE;
			break;
		default:
			inContext.txnIsolationLevel = (short) SQL_TXN_READ_COMMITTED;
			break;
		}

		setConnectionAttr(conn, SQL_TXN_ISOLATION, inContext.txnIsolationLevel, String
				.valueOf(inContext.txnIsolationLevel));
		if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities.makeParams(conn.props_, level);
			String temp = "Setting transaction isolation = " + level + " is done.";
			t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "setTransactionIsolation", temp, p);
		}
	};

	int getTransactionIsolation() throws SQLException {
		return txnIsolationLevel;
	}

	long getTxid() {
		return txid;
	}

	void setTxid(long txid) {
		this.txid = txid;
	}

	boolean getAutoCommit() {
		return autoCommit;
	}

	void setAutoCommit(TrafT4Connection conn, boolean autoCommit) throws SQLException {
		isConnectionOpen();
		boolean commit = this.autoCommit;

		this.autoCommit = autoCommit;

		if (autoCommit == false) {
			inContext.autoCommit = 0;
		} else {
			inContext.autoCommit = 1;

		}
		try {
			setConnectionAttr(conn, SQL_ATTR_AUTOCOMMIT, inContext.autoCommit, String.valueOf(inContext.autoCommit));
		} catch (SQLException sqle) {
			this.autoCommit = commit;
			throw sqle;
		}
		if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities.makeParams(conn.props_, autoCommit);
			String temp = "Setting autoCommit = " + autoCommit + " is done.";
			t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "setAutoCommit", temp, p);
		}
	}

	void enableNARSupport(TrafT4Connection conn, boolean NARSupport) throws SQLException {
		int val = NARSupport ? 1 : 0;
		setConnectionAttr(conn, TRANSPORT.SQL_ATTR_ROWSET_RECOVERY, val, String.valueOf(val));
	}

	void enableProxySyntax(TrafT4Connection conn) throws SQLException {
		setConnectionAttr(conn, InterfaceConnection.SPJ_ENABLE_PROXY, 1, "1");
	}

	boolean isReadOnly() {
		return isReadOnly;
	}

	void setReadOnly(boolean readOnly) throws SQLException {
		isConnectionOpen();
		this.isReadOnly = readOnly;
	}
	void setReadOnly(TrafT4Connection conn, boolean readOnly) throws SQLException {
		isConnectionOpen();
		this.isReadOnly = readOnly;
		if (readOnly == false) {
			inContext.accessMode = 0;
		} else {
			inContext.accessMode = 1;

		}
		setConnectionAttr(conn, SQL_ATTR_ACCESS_MODE, inContext.accessMode, String.valueOf(inContext.accessMode));
		if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities.makeParams(conn.props_, readOnly);
			String temp = "Setting readOnly = " + readOnly + " is done.";
			t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "readOnly", temp, p);
		}

	}

	void close() throws SQLException {
		TerminateDialogueReply tdr_ = null;
		if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
			Object p[] = T4LoggingUtilities.makeParams(t4props_);
			String temp = "Terminate Dialogue.";
			t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "close", temp, p);
		}
			
		//3196 - NDCS transaction for SPJ
		if (suspendRequest_) {
			_t4Conn.suspendUDRTransaction();
		}
		
		SecPwd.removeInstance(this._t4Conn);
		
		try {
			tdr_ = t4connection_.TerminateDialogue();
		} catch (SQLException tex) {
			if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(t4props_);
				String temp = "SQLException during TerminateDialogue.";
				t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "close", temp, p);
			}
			throw tex;
		}

		switch (tdr_.m_p1.exception_nr) {
		case TRANSPORT.CEE_SUCCESS:
			break;
		case odbc_SQLSvc_TerminateDialogue_exc_.odbc_SQLSvc_TerminateDialogue_SQLError_exn_:
			//ignore errors
		}

		// needs work here. This should be proxy destroy. close the logfiles
		try {
			if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(t4props_);
				String temp = "Terminate Dialogue successful.";
				t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "close", temp, p);
			}
		} catch (java.lang.SecurityException sex) {
		}
	};

	void endTransaction(short commitOption) throws SQLException {
		EndTransactionReply etr_ = null;
		if (autoCommit) { 
			throw TrafT4Messages.createSQLException(t4props_, locale, "invalid_commit_mode", null);
		}

		isConnectionOpen();
		try {
			etr_ = t4connection_.EndTransaction(commitOption);
		} catch (SQLException tex) {
			if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(t4props_, commitOption);
				String temp = "SQLException during EndTransaction." + tex.toString();
				t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "endTransaction", temp, p);
			}
			throw tex;
		}

		switch (etr_.m_p1.exception_nr) {
		case TRANSPORT.CEE_SUCCESS:
			break;
		case odbc_SQLSvc_EndTransaction_exc_.odbc_SQLSvc_EndTransaction_ParamError_exn_:
			if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(t4props_, commitOption);
				String temp = "odbc_SQLSvc_EndTransaction_ParamError_exn_ :";
				t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "endTransaction", temp, p);
			}
			throw TrafT4Messages.createSQLException(t4props_, locale, "ParamError:" + etr_.m_p1.ParamError, null);
		case odbc_SQLSvc_EndTransaction_exc_.odbc_SQLSvc_EndTransaction_InvalidConnection_exn_:
			if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(t4props_, commitOption);
				String temp = "odbc_SQLSvc_EndTransaction_InvalidConnection_exn_:";
				t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "endTransaction", temp, p);
			}
			throw new SQLException("odbc_SQLSvc_EndTransaction_InvalidConnection_exn", "HY100002", 10001);
		case odbc_SQLSvc_EndTransaction_exc_.odbc_SQLSvc_EndTransaction_SQLError_exn_:
			if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(t4props_, commitOption);
				String temp = "odbc_SQLSvc_EndTransaction_SQLError_exn_:" + etr_.m_p1.SQLError;
				t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "endTransaction", temp, p);
			}
			TrafT4Messages.throwSQLException(t4props_, etr_.m_p1.SQLError);
		case odbc_SQLSvc_EndTransaction_exc_.odbc_SQLSvc_EndTransaction_SQLInvalidHandle_exn_:
			if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(t4props_, commitOption);
				String temp = "odbc_SQLSvc_EndTransaction_SQLInvalidHandle_exn_:";
				t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "endTransaction", temp, p);
			}
			throw new SQLException("odbc_SQLSvc_EndTransaction_SQLInvalidHandle_exn", "HY100004", 10001);
		case odbc_SQLSvc_EndTransaction_exc_.odbc_SQLSvc_EndTransaction_TransactionError_exn_:
			if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(t4props_, commitOption);
				String temp = "odbc_SQLSvc_EndTransaction_TransactionError_exn_:";
				t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "endTransaction", temp, p);
			}
			throw new SQLException("odbc_SQLSvc_EndTransaction_TransactionError_exn", "HY100005", 10001);
		default:
			if (t4props_.t4Logger_.isLoggable(Level.FINEST) == true) {
				Object p[] = T4LoggingUtilities.makeParams(t4props_, commitOption);
				String temp = "UnknownError:";
				t4props_.t4Logger_.logp(Level.FINEST, "InterfaceConnection", "endTransaction", temp, p);
			}
			throw new SQLException("Unknown Error during EndTransaction", "HY100001", 10001);
		}

	};

	long beginTransaction() throws SQLException {
		isConnectionOpen();

		return txid;
	};

	void reuse() {
		txnIsolationLevel = Connection.TRANSACTION_READ_COMMITTED;
		autoCommit = true;
		isReadOnly = false;
		isClosed_ = false;
		txid = 0;
		t4connection_.reuse();
	};

	boolean useArrayBinding() {
		return useArrayBinding_;
	}

	short getTransportBufferSize() {
		return transportBufferSize_;
	}

	// methods for handling weak connections
	void removeElement(TrafT4Connection conn) {
		refTosrvrCtxHandle_.remove(conn.pRef_);
		conn.pRef_.clear();
	}

	void gcConnections() {
		Reference pRef;
		InterfaceConnection ic;
		while ((pRef = refQ_.poll()) != null) {
			ic = (InterfaceConnection) refTosrvrCtxHandle_.get(pRef);
			// All PreparedStatement objects are added to Hashtable
			// Only Statement objects that produces ResultSet are added to
			// Hashtable
			// Hence stmtLabel could be null
			if (ic != null) {
				try {
					ic.close();
				} catch (SQLException e) {
				} finally {
					refTosrvrCtxHandle_.remove(pRef);
				}
			}
		}
	}

	public byte[] encodeString(String str, int charset) throws CharacterCodingException, UnsupportedCharsetException {
		Integer key = new Integer(charset);
		CharsetEncoder ce;
		byte[] ret = null;

		if (str != null) {
			if (this.isoMapping_ == InterfaceUtilities.SQLCHARSETCODE_ISO88591 && !this.enforceISO) {
				ret = str.getBytes(); //convert the old way
			} else {
				if ((ce = (CharsetEncoder) encoders.get(key)) == null) { //only create a new encoder if its the first time
					String charsetName = InterfaceUtilities.getCharsetName(charset);
					
					//encoder needs to be based on our current swap flag for UTF-16 data
					//this should be redesigned when we fixup character set issues for SQ
					if(key == InterfaceUtilities.SQLCHARSETCODE_UNICODE && this.byteSwap == true) {
						charsetName = "UTF-16LE";
					}
					
					Charset c = Charset.forName(charsetName);
					ce = c.newEncoder();
					ce.onUnmappableCharacter(CodingErrorAction.REPORT);
					encoders.put(key, ce);
				}
				
				synchronized(ce) { //since one connection shares encoders
					ce.reset();
					ByteBuffer buf = ce.encode(CharBuffer.wrap(str));
					ret = new byte[buf.remaining()];
					buf.get(ret, 0, ret.length);
				}
			}
		}

		return ret;
	}

	public String decodeBytes(byte[] data, int charset) throws CharacterCodingException, UnsupportedCharsetException {
		Integer key = new Integer(charset);
		CharsetDecoder cd;
		String str = null;

		// we use this function for USC2 columns as well and we do NOT want to
		// apply full pass-thru mode for them
		if (this.isoMapping_ == InterfaceUtilities.SQLCHARSETCODE_ISO88591 && !this.enforceISO
				&& charset != InterfaceUtilities.SQLCHARSETCODE_UNICODE) {
			str = new String(data);
		} else {
			// the following is a fix for JDK 1.4.2 and MS932. For some reason
			// it does not handle single byte entries properly
			boolean fix = false;
			if (charset == 10 && data.length == 1) {
				data = new byte[] { 0, data[0] };
				fix = true;
			}

			if ((cd = (CharsetDecoder) decoders.get(key)) == null) { //only create a decoder if its the first time
				String charsetName = InterfaceUtilities.getCharsetName(charset);
				
				//encoder needs to be based on our current swap flag for UTF-16 data
				//this should be redesigned when we fixup character set issues for SQ
				if(key == InterfaceUtilities.SQLCHARSETCODE_UNICODE && this.byteSwap == true) {
					charsetName = "UTF-16LE";
				}
				
				Charset c = Charset.forName(charsetName);
				cd = c.newDecoder();
				cd.replaceWith(this.t4props_.getReplacementString());
				cd.onUnmappableCharacter(CodingErrorAction.REPLACE);
				decoders.put(key, cd);
			}
			
			synchronized(cd) { //one decoder for the entire connection
				cd.reset();
				str = cd.decode(ByteBuffer.wrap(data)).toString();
			}

			if (fix)
				str = str.substring(1);
		}

		return str;
	}

	public String getApplicationName() {
		return this.t4props_.getApplicationName();
	}
	
	void setClientInfoProperties(Properties prop) {
		this.t4props_.setClientInfoProperties(prop);
	}

	Properties getClientInfoProperties() {
		return this.t4props_.getClientInfoProperties();
	}

	public void setClientInfo(String name, String value) throws SQLClientInfoException {
		this.t4props_.getClientInfoProperties().setProperty(name, value);
	}

	String getClientInfo(String name) {
		return this.t4props_.getClientInfoProperties().getProperty(name);
	}
}
