/*******************************************************************************
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
 *******************************************************************************/

package org.trafodion.jdbc.t2;

import java.io.File;
import java.io.UnsupportedEncodingException;
import java.lang.Long;
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
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.util.Hashtable;
import java.util.Locale;
import java.util.logging.Handler;
import java.util.logging.Level;


class InterfaceConnection {
    static final int MODE_SQL = 0;
    static final int MODE_WMS = 1;
    static final int MODE_CMD = 2;

    static final short SQL_COMMIT = 0;
    static final short SQL_ROLLBACK = 1;
    private int txnIsolationLevel = Connection.TRANSACTION_READ_COMMITTED;
    private boolean autoCommit = true;
    private boolean isReadOnly = false;
    private boolean isClosed_;
    private long txid;
    private Locale locale;
    private boolean useArrayBinding_;
    private short transportBufferSize_;
    Handler t2FileHandler;
    private InterfaceNativeConnect t2connection_ = null;
    private String m_ncsSrvr_ref;
    private long dialogueId_;
    private String m_sessionName;

    // character set information
    private int isoMapping_ = 15;
    private int termCharset_ = 15;
    private boolean enforceISO = false;
    private boolean byteSwap = true;  // we use big endian in JDBC, always need to swap byte orders in native/server side
    private String _serverDataSource;

    private int _mode = MODE_SQL;

    T2Properties t2props_;
    SQLWarning sqlwarning_;

    Hashtable encoders = new Hashtable(11);
    Hashtable decoders = new Hashtable(11);

    // static fields from odbc_common.h and sql.h
    static final int SQL_TXN_READ_UNCOMMITTED = 1;
    static final int SQL_TXN_READ_COMMITTED = 2;
    static final int SQL_TXN_REPEATABLE_READ = 4;
    static final int SQL_TXN_SERIALIZABLE = 8;
    static final short SQL_ATTR_CURRENT_CATALOG = 109;
    static final short SQL_ATTR_ACCESS_MODE = 101;
    static final short SQL_ATTR_AUTOCOMMIT = 102;
    static final short SQL_TXN_ISOLATION = 108;

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
    //private SecPwd _security;
    long currentTime;

    private SQLMXConnection _t2Conn;
    private String _remoteProcess;
    private String _connStringHost = "";

    InterfaceConnection(SQLMXConnection conn, T2Properties t2props) throws SQLException {
        _t2Conn = conn;
        t2props_ = t2props;
        _remoteProcess = "";
        dialogueId_ = new Long(conn.getDialogueId_()).longValue();

        locale = conn.getLocale();
        txid = 0;
        isClosed_ = false;
        transportBufferSize_ = 32000;


        // Connection context details
        m_ncsSrvr_ref = t2props.getUrl();
        _ignoreCancel = false;

        sqlwarning_ = null;
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

    private void setISOMapping(int isoMapping) {
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
        termCharset_ = InterfaceUtilities.getCharsetValue("UTF-8");;
    }

    int getTerminalCharset() {
        return termCharset_;
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

    InterfaceNativeConnect getT2Connection() {
        return t2connection_;
    }

    long getDialogueId() {
        return dialogueId_;
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

    // @deprecated
    void isConnectionClosed() throws SQLException {
        if (isClosed_ == false) {
            throw Messages.createSQLException(t2props_, locale, "invalid_connection", null);
        }
    }

    // @deprecated
    void isConnectionOpen() throws SQLException {
        if (isClosed_) {
            throw Messages.createSQLException(t2props_, locale, "invalid_connection", null);
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

    long getTxid() {
        return txid;
    }

    void setTxid(long txid) {
        this.txid = txid;
    }

    boolean useArrayBinding() {
        return useArrayBinding_;
    }

    short getTransportBufferSize() {
        return transportBufferSize_;
    }

    // methods for handling weak connections
    void removeElement(SQLMXConnection conn) {
        refTosrvrCtxHandle_.remove(conn.pRef_);
        conn.pRef_.clear();
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
                    /*
                       if(key == InterfaceUtilities.SQLCHARSETCODE_UNICODE && this.byteSwap == true) {
                       charsetName = "UTF-16LE";
                       } */

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
                cd.replaceWith(this.t2props_.getReplacementString());
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

    /*
       public String getApplicationName() {
       return this.t2props_.getApplicationName();
       }
       */
}
