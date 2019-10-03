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

package org.apache.trafodion.jdbc.t2;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.PrintWriter;
import java.sql.SQLException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.Properties;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.naming.Reference;
import javax.naming.StringRefAddr;

public class T2Properties {

    WeakConnection weakConnection = new WeakConnection();

    private String catalog_;
    private String schema_;
    private int batchBindingSize_;
    private int traceFlag_;
    private String traceFile_;
    private int inlineLobChunkSize_;
    private int lobChunkSize_; 
    private int transactionMode_;
    private String iso88591EncodingOverride_;
    private String stmtatomicity_;
    private int stmtatomicityval_;
    private String Spjrs_;
    private int Spjrsval_;
    private String contBatchOnError_;
    private boolean contBatchOnErrorval_;
    String description_;
    String dataSourceName_;
    String language_;
    String url_;
    private String enableLog_;
    private String idMapFile;
    
    //Publishing
    private int statisticsIntervalTime_;
    private int statisticsLimitTime_;
    private String statisticsType_;
    private String programStatisticsEnabled_;
    private String statisticsSqlPlanEnabled_;

    //Pooling
    private int maxPoolSize_ = -1;
    private int minPoolSize_;
    private int initialPoolSize_;
    private int maxIdleTime_;
    private int maxStatements_;

    private String externalCallHandler = "NONE";
    private String externalCallPrefix = "EXT";

    int loginTimeout_;

    private Properties defaults_;
    private Properties inprops_;
    private final String propPrefix_ = "t2jdbc.";

    private long queryExecuteTime_;
    private String T2QueryExecuteLogFile_;

    private PrintWriter logWriter_=null;

    private Pattern namePat_ = Pattern.compile("(\"(([^\"]|\"\")+)\")");
    private Pattern catalogPat_ =Pattern.compile("((\"([^\"]|\"\")*\")|.+)(\\.)");
    /**
     * @return the catalog_
     */
    public String getCatalog() {
        return catalog_;
    }

    /**
     * @param catalog_ the catalog_ to set
     */
    public void setCatalog(String catalog_) {
//		this.catalog_ = catalog_;
        if (catalog_ != null) {
            if (!catalog_.startsWith("\""))
                this.catalog_ = catalog_.toUpperCase();
            else
                this.catalog_ = catalog_;
        }
    }

    /**
     * @return the schema_
     */
    public String getSchema() {
        return schema_;
    }

    /**
     * @param schema_ the schema_ to set
     */
    public void setSchema(String schema_) {
//		this.schema_ = schema_;
        if (schema_ != null) {
            if (schema_.contains(".")) {  // To split catalog & schema separately, though user gives as cat.sch
                String catalogName = "";
                catalogName = getCatalogToken(schema_);
                if (catalogName != null) {
                    setCatalog(catalogName);
                    schema_ = schema_.substring(catalogName.length() + 1);
                }
            }
            if (!schema_.startsWith("\""))
                this.schema_ = schema_.toUpperCase();
            else {
                this.schema_ = schema_;
            }
        }
    }

    private String getCatalogToken(String schemaStr)
    {
        Matcher mat=catalogPat_.matcher(schemaStr);
        if (mat.find()) {
            String temp = mat.group().substring(0, mat.group().length() - 1);
            if (temp.startsWith("\"")) {
                if (!namePat_.matcher(schemaStr).matches())
                    return (temp);
                else
                    return null;
            } else
                return (temp.toUpperCase());
        } else {
            return null;
        }

    }

    /**
     * @return the batchBindingSize_
     */
    public int getBatchBinding() {
        return batchBindingSize_;
    }

    /**
     * @param batchBindingSize_ the batchBindingSize_ to set
     */
    public void setBatchBinding(int batchBindingSize_) {
        if(batchBindingSize_ > -1){
        this.batchBindingSize_ = batchBindingSize_;
        }
        else
        {
            try{
            throw new SQLException(
            "batchBinding value should be a positive value.");
            }
            catch(SQLException e){

            }
        }
    }

    /**
     * @param batchBindingSize_ the batchBindingSize_ to set
     */
    public void setBatchBinding(String batchBindingSize_) {
        int bbSize = 0;

        if(batchBindingSize_ != null){
            bbSize = Integer.parseInt(batchBindingSize_);
        }

        setBatchBinding(bbSize);
    }


    /**
     * @return the traceFlag_
     */
    public int getTraceFlag() {
        return traceFlag_;
    }

    /**
     * @param traceFlag_ the traceFlag_ to set
     */
    public void setTraceFlag(int traceFlag_) {
        this.traceFlag_ = traceFlag_;
    }

    public void setTraceFlag(String traceFlag_) {
        int tf = 0;
        if(traceFlag_ != null){
            tf = Integer.parseInt(traceFlag_);
        }
        setTraceFlag(tf);
    }

    /**
     * @return the traceFile_
     */
    public String getTraceFile() {
        return traceFile_;
    }

    /**
     * @param traceFile_ the traceFile_ to set
     */
    public void setTraceFile(String traceFile) {
//		this.traceFile_ = traceFile_;

        if (traceFile != null) {
            File directory = new File(traceFile);
            if (directory.isDirectory()) {
                // Generate a file name.
                SimpleDateFormat df = new SimpleDateFormat("yyyyMMdd-HHmmss-");
                String pid = df.format(new Date())
                        + Integer.toString(T2Driver.pid_);
                try {
                    this.traceFile_ = directory.getCanonicalPath() + "/" + pid
                            + ".log";
                } catch (IOException e) {
                    // TODO Auto-generated catch block
                    RuntimeException ex = new RuntimeException(e.getMessage());

                    ex.setStackTrace(e.getStackTrace());
                    throw ex;

                }
            }else {
                this.traceFile_ = traceFile;
            }
        }
    }

    public int getInlineLobChunkSize()
    {
       return inlineLobChunkSize_;
    }

    public void setInlineLobChunkSize(int size)
    {
       inlineLobChunkSize_ = size;       
    }

    public void setInlineLobChunkSize(String sz) {
       int size = 0;
       if (sz !=  null) 
          size = Integer.parseInt(sz);
       setInlineLobChunkSize(size);
    }

    public int getLobChunkSize()
    {
        return lobChunkSize_;
    }

    public void setLobChunkSize(int size)
    {
       lobChunkSize_ = size;       
    }

    public void setLobChunkSize(String sz) {
       int size = 0;
       if (sz !=  null) 
          size = Integer.parseInt(sz);
       setLobChunkSize(size);
    }

    /**
     * @return the transactionMode_
     */
    public String getTransactionMode() {
        return (SQLMXConnection.mapTxnModeToString(transactionMode_));
    }


    public void setTransactionMode(String transactionMode_) {
        int txnMode;

        if(transactionMode_ !=null){
            txnMode = SQLMXConnection.mapTxnMode(transactionMode_);
            if (txnMode != SQLMXConnection.TXN_MODE_INVALID)
                this.transactionMode_ = txnMode;
            else
                try {
                    throw Messages.createSQLException(null,"invalid_transaction_mode", null);
                } catch (SQLException e) {
                    // TODO Auto-generated catch block
                }
        }else {
            this.transactionMode_ = SQLMXConnection.TXN_MODE_MIXED;
        }
    }

    /**
     * @return the iso88591EncodingOverride_
     */
    public String getIso88591EncodingOverride() {
        return iso88591EncodingOverride_;
    }

    /**
     * @param iso88591EncodingOverride_ the iso88591EncodingOverride_ to set
     */
    public void setIso88591EncodingOverride(String iso88591EncodingOverride_) {
//		this.iso88591EncodingOverride_ = iso88591EncodingOverride_;
        if (iso88591EncodingOverride_ != null)
            this.iso88591EncodingOverride_ = iso88591EncodingOverride_.toUpperCase();
    }

    /**
     * @return the stmtatomicity_
     */
    public String getStmtatomicity() {
        return stmtatomicity_;
    }

    /**
     * @param stmtatomicity_ the stmtatomicity_ to set
     */
    public void setStmtatomicity(String stmtatomicity_) {
        this.stmtatomicity_ = stmtatomicity_;
    }

    /**
     * @return the stmtatomicityval_
     */
    public int getStmtatomicityval() {
        return stmtatomicityval_;
    }

    /**
     * @param stmtatomicityval_ the stmtatomicityval_ to set
     */
    public void setStmtatomicityval(int stmtatomicityval_) {
        this.stmtatomicityval_ = stmtatomicityval_;
    }

    /**
     * @return the spjrs_
     */
    public String getSpjrs() {
        return Spjrs_;
    }

    /**
     * @param spjrs_ the spjrs_ to set
     */
    public void setSpjrs(String spjrs_) {
//		Spjrs_ = spjrs_;
        if (spjrs_ != null) {
            if (spjrs_.equalsIgnoreCase("on")) {
                Spjrsval_ = 1;
                Spjrs_ = spjrs_.toUpperCase();
            } else
                Spjrs_ = "OFF";
        } else
            Spjrs_ = "OFF";
    }

    public boolean isSpjrsOn() {
        if(Spjrsval_ == 1)
            return true;
        else
            return false;
    }

    /**
     * @return the contBatchOnError_
     */
    public String getContBatchOnError() {
        return contBatchOnError_;
    }

    /**
     * @param contBatchOnError_ the contBatchOnError_ to set
     */
    public void setContBatchOnError(String contBatchOnError_) {
        this.contBatchOnError_ = contBatchOnError_;
    }

    /**
     * @return the contBatchOnErrorval_
     */
    public boolean isContBatchOnErrorval() {
        return contBatchOnErrorval_;
    }

    /**
     * @param contBatchOnErrorval_ the contBatchOnErrorval_ to set
     */
    public void setContBatchOnErrorval(boolean contBatchOnErrorval_) {
        this.contBatchOnErrorval_ = contBatchOnErrorval_;
    }

    /**
     * @return the description_
     */
    public String getDescription() {
        return description_;
    }

    /**
     * @param description_ the description_ to set
     */
    public void setDescription(String description_) {
        this.description_ = description_;
    }

    /**
     * @return the dataSourceName_
     */
    public String getDataSourceName() {
        return dataSourceName_;
    }

    /**
     * @param dataSourceName_ the dataSourceName_ to set
     */
    public void setDataSourceName(String dataSourceName_) {
        this.dataSourceName_ = dataSourceName_;
    }

    /**
     * @return the language_
     */
    public String getLanguage() {
        return language_;
    }

    /**
     * @param language_ the language_ to set
     */
    public void setLanguage(String language_) {
        this.language_ = language_;
    }


    /**
     * @return the url_
     */
    public String getUrl() {
        return url_;
    }

    /**
     * @param url_ the url_ to set
     */
    public void setUrl(String url_) {
        this.url_ = url_;
    }

    /**
     * @return the enableLog_
     */
    public String getEnableLog() {
        return enableLog_;
    }

    /**
     * @param enableLog_ the enableLog_ to set
     */
    public void setEnableLog(String enableLog_) {
//		this.enableLog_ = enableLog_;
        if(enableLog_ != null && enableLog_.equalsIgnoreCase("ON")){
            this.enableLog_ = "ON";
        }else {
            this.enableLog_ = "OFF";
        }
    }

    /**
     * @return the maxPoolSize_
     */
    public int getMaxPoolSize() {
        return maxPoolSize_;
    }
    public String getIdMapFile() {
        return idMapFile;
    }
    public void setIdMapFile(String idMapFile) {
        this.idMapFile = idMapFile;
    }
    
    /**
     * @param maxPoolSize_ the maxPoolSize_ to set
     */
    public void setMaxPoolSize(int maxPoolSize_) {
        this.maxPoolSize_ = maxPoolSize_;
    }

    public void setMaxPoolSize(String maxPoolSize_) {
        int maxPS = -1;
        if(maxPoolSize_ != null){
            maxPS = Integer.parseInt(maxPoolSize_);
        }
        setMaxPoolSize(maxPS);
    }

    /**
     * @return the minPoolSize_
     */
    public int getMinPoolSize() {
        return minPoolSize_;
    }

    /**
     * @param minPoolSize_ the minPoolSize_ to set
     */
    public void setMinPoolSize(int minPoolSize_) {
        this.minPoolSize_ = minPoolSize_;
    }

    public void setMinPoolSize(String minPoolSize_) {
        int mps = 0;
        if(minPoolSize_ != null){
            mps = Integer.parseInt(minPoolSize_);
        }
        setMinPoolSize(mps);
    }
    /**
     * @return the initialPoolSize_
     */
    public int getInitialPoolSize() {
        return initialPoolSize_;
    }

    /**
     * @param initialPoolSize_ the initialPoolSize_ to set
     */
    public void setInitialPoolSize(int initialPoolSize_) {
        this.initialPoolSize_ = initialPoolSize_;
    }

    /**
     * @param initialPoolSize_ the initialPoolSize_ to set
     */
    public void setInitialPoolSize(String initialPoolSize_) {
        int initialPS =0;
        if(initialPoolSize_ != null){
            initialPS = Integer.parseInt(initialPoolSize_);
        }
        setInitialPoolSize(initialPS);
    }
    /**
     * @return the maxIdleTime_
     */
    public int getMaxIdleTime() {
        return maxIdleTime_;
    }

    /**
     * @param maxIdleTime_ the maxIdleTime_ to set
     */
    public void setMaxIdleTime(int maxIdleTime_) {
        if(maxIdleTime_ > 0)
            this.maxIdleTime_ = maxIdleTime_;
        else
            this.maxIdleTime_ = 0;
    }


    public void setMaxIdleTime(String maxIdleTime_) {
    int maxIT = 0;
    if (maxIdleTime_ != null) {
        maxIT = Integer.parseInt(maxIdleTime_);
    }
        setMaxIdleTime(maxIT);
    }
    /**
     * @return the maxStatements_
     */
    public int getMaxStatements() {
        return maxStatements_;
    }

    /**
     * @param maxStatements_ the maxStatements_ to set
     */
    public void setMaxStatements(int maxStatements_) {
        this.maxStatements_ = maxStatements_;
    }

    public void setMaxStatements(String maxStatements_) {
        int maxStmt = 0;
        if (maxStatements_ != null) {
            maxStmt = Integer.parseInt(maxStatements_);
        }
        setMaxStatements(maxStmt);

    }

    /**
     * @return the externalCallHandler
     */
    public String getExternalCallHandler() {
        return externalCallHandler;
    }

    /**
     * @param externalCallHandler the externalCallHandler to set
     */
    public void setExternalCallHandler(String externalCallHandler) {
        if(externalCallHandler !=null){
            this.externalCallHandler = externalCallHandler;
        }else
            this.externalCallHandler = "NONE";

    }

    /**
     * @return the externalCallPrefix
     */
    public String getExternalCallPrefix() {
        return externalCallPrefix;
    }

    /**
     * @param externalCallPrefix the externalCallPrefix to set
     */
    public void setExternalCallPrefix(String externalCallPrefix) {
        if(externalCallPrefix !=null)
            this.externalCallPrefix = externalCallPrefix;
        else
            this.externalCallPrefix = "EXT";
    }

    /**
     * @return the loginTimeout_
     */
    public int getLoginTimeout() {
        return loginTimeout_;
    }

    /**
     * @param loginTimeout_ the loginTimeout_ to set
     */
    public void setLoginTimeout(int loginTimeout_) {
        this.loginTimeout_ = loginTimeout_;
    }

    public void setLoginTimeout(String loginTimeout_) {
        int loginTimeout = 0;
        if(loginTimeout_ != null){
            loginTimeout = Integer.parseInt(loginTimeout_);
        }
        setLoginTimeout(loginTimeout);
    }

    /**
     * @return the queryExecuteTime_
     */
    public long getQueryExecuteTime() {
        return queryExecuteTime_;
    }

    /**
     * @param queryExecuteTime_ the queryExecuteTime_ to set
     */
    public void setQueryExecuteTime(long queryExecuteTime_) {
        this.queryExecuteTime_ = queryExecuteTime_;
    }

    /**
     * @param queryExecuteTime_ the queryExecuteTime_ to set
     */
    public void setQueryExecuteTime(String queryExecuteTime_) {
        long queryET = 0;
        if(queryExecuteTime_ !=null){
            queryET =  Long.parseLong(queryExecuteTime_);
        }
        setQueryExecuteTime(queryET);
    }

    /**
     * @return the t2QueryExecuteLogFile_
     */
    public String getT2QueryExecuteLogFile() {
        return T2QueryExecuteLogFile_;
    }

    /**
     * @param queryExecuteLogFile_ the t2QueryExecuteLogFile_ to set
     */
    public void setT2QueryExecuteLogFile(String queryExecuteLogFile_) {
        T2QueryExecuteLogFile_ = queryExecuteLogFile_;
    }

//====================================================================
    // Publishing

    /**
     * @return the statisticsIntervalTime_
     */
    public int getStatisticsIntervalTime() {
        return statisticsIntervalTime_;
    }

    /**
     * @param statisticsIntervalTime_ the statisticsIntervalTime_ to set
     */
    public void setStatisticsIntervalTime(int statisticsIntervalTime_) {
        this.statisticsIntervalTime_ = statisticsIntervalTime_;
    }

    public void setStatisticsIntervalTime(String statisticsIntervalTime_) {
        int st = 60;
        if(statisticsIntervalTime_ != null){
            st = Integer.parseInt(statisticsIntervalTime_);
        }
        setStatisticsIntervalTime(st);
    }

    /**
     * @return the statisticsLimitTime_
     */
    public int getStatisticsLimitTime() {
        return statisticsLimitTime_;
    }

    /**
     * @param statisticsLimitTime_ the statisticsLimitTime_ to set
     */
    public void setStatisticsLimitTime(int statisticsLimitTime_) {
        this.statisticsLimitTime_ = statisticsLimitTime_;
    }

    public void setStatisticsLimitTime(String statisticsLimitTime_) {
        int st = 60;
        if(statisticsLimitTime_ != null){
            st = Integer.parseInt(statisticsLimitTime_);
        }
        setStatisticsLimitTime(st);
    }

    /**
     * @return the statisticsType_
     */
    public String getStatisticsType() {
        return statisticsType_;
    }

    /**
     * @param statisticsType_ the statisticsType_ to set
     */
    public void setStatisticsType(String statisticsType_) {
        if (statisticsType_ != null)
            this.statisticsType_ = statisticsType_;
        else
            this.statisticsType_ = "aggregated";
    }
    
    /**
     * @return the programStatisticsEnabled_
     */
    public String getProgramStatisticsEnabled() {
        return programStatisticsEnabled_;
    }

    /**
     * @param programStatisticsEnabled_ the programStatisticsEnabled_ to set
     */
    public void setProgramStatisticsEnabled(String programStatisticsEnabled_) {
        if (programStatisticsEnabled_ != null)
            this.programStatisticsEnabled_ = programStatisticsEnabled_;
        else
            this.programStatisticsEnabled_ = "true";
    }
    
    /**
     * @return the statisticsSqlPlanEnabled_
     */
    public String getStatisticsSqlPlanEnabled() {
        return statisticsSqlPlanEnabled_;
    }

    /**
     * @param statisticsSqlPlanEnabled_ the statisticsSqlPlanEnabled_ to set
     */
    public void setStatisticsSqlPlanEnabled(String statisticsSqlPlanEnabled_) {
        if (statisticsSqlPlanEnabled_ != null)
            this.statisticsSqlPlanEnabled_ = statisticsSqlPlanEnabled_;
        else
            this.statisticsSqlPlanEnabled_ = "true";
    }
    
    //=========================================================
    /**
     * @param logWriter_ the logWriter_ to set
     */
    /**
     * Sets the log writer for this <code>SQLMXConnectionPoolDataSource</code>
     * object to the given <code>java.io.PrintWriter</code> object.
     *
     * <p>
     * The log writer is a character output stream to which all logging and
     * tracing messages for this <code>SQLMXConnectionPoolDataSource</code>
     * object are printed. This includes messages printed by the methods of this
     * object, messages printed by methods of other objects manufactured by this
     * object, and so on. Messages printed to a data source-specific log writer
     * are not printed to the log writer associated with the
     * <code>java.sql.DriverManager</code> class. When a data source object is
     * created, the log writer is initially null; in other words, the default is
     * for logging to be disabled.
     *
     * @param out
     *            the new log writer; <code>null</code> to disable logging
     * @exception SQLException
     *                if a database access error occurs
     * @see #getLogWriter
     */
    public void setLogWriter(PrintWriter logWriter_)  {
        this.logWriter_ = logWriter_;
    }

    /**
     * @return the logWriter_
     */
    public PrintWriter getLogWriter() {
        return this.logWriter_;
    }

    /**
     * @return the inprops_
     */
    public Properties getInprops() {
        return inprops_;
    }

    /**
     * @param inprops_ the inprops_ to set
     */
    public void setInprops(Properties inprops_) {
        this.inprops_ = inprops_;
    }

    public T2Properties() {
        initialize(null);
    }

    public T2Properties(Properties props) {

        initialize(props);
    }


    void initialize(Properties props) {
        inprops_ = props;
        inlineLobChunkSize_ = 16*1024;
        lobChunkSize_ = 16*1024*1024;
        setProperties();
    }

    public static String getPropertyValue(java.util.Properties prop_list,
            String prop_name) {

        String prop_value = prop_list.getProperty("t2jdbc." + prop_name);
        if (prop_value == null) {
            // Try the non-prolog case for compatibility
            prop_value = prop_list.getProperty(prop_name);
        }

        return (prop_value);
    }
    private String getProperty(String token) {
        String ret = null;

        // check input props first
        if (inprops_ != null) {
        // ret = inprops_.getProperty(token);
        // getProperty returns null for int datatypes used in props.put("propname",int),hence changed getProperty to get
        ret = (String) inprops_.get(token);
        }
        // props file next
		if (ret == null && defaults_ != null) {
			ret = defaults_.getProperty(token);
		}
        // system properties with the t4sqlmx prefix
        if (ret == null) {
            ret = System.getProperty(propPrefix_ + token);
        }
        if(ret == null){
            ret = System.getProperty(token);
        }

        return ret;
    }
    private void setProperties() {
        // TODO Auto-generated method stub

	defaults_ = null;
	String propsFile = getProperty("properties");
	if (propsFile != null) {
	    propsFile = propsFile.trim();
	    if (propsFile.length() != 0) {
		FileInputStream fis = null;
		try {
		    fis = new FileInputStream(new File(propsFile));
		    defaults_ = new Properties();
		    defaults_.load(fis);
		} catch (Exception ex) {
		    fis = null;
		} finally {
		    try {
			if (fis != null) {
			    fis.close();
			}
		    }catch (IOException ioe) {
			// ignore
		    }
		}
	    }
	}
        setCatalog(getProperty("catalog"));
        setSchema(getProperty("schema"));
        setBatchBinding(getProperty("batchBinding"));
        setLanguage(getProperty("language"));
        setSpjrs(getProperty("Spjrs"));
        setStmtatomicity(getProperty("stmtatomicity"));
//		setStmtatomicityval(getProperty(""));
        setTransactionMode(getProperty("transactionMode"));
        setIso88591EncodingOverride(getProperty("ISO88591"));
        setContBatchOnError(getProperty("contBatchOnError"));

        setMaxIdleTime(getProperty("maxIdleTime"));
        setMaxPoolSize(getProperty("maxPoolSize"));
        setMaxStatements(getProperty("maxStatements"));
        setMinPoolSize(getProperty("minPoolSize"));
        setInitialPoolSize(getProperty("initialPoolSize"));
	setInlineLobChunkSize(getProperty("inlineLobChunkSize"));
	setLobChunkSize(getProperty("lobChunkSize"));

//		setContBatchOnErrorval(getProperty(""));

//		setEnableLog(getProperty("enableLog"));
        setTraceFlag(getProperty("traceFlag"));
        setTraceFile(getProperty("traceFile"));

        setExternalCallHandler(getProperty("externalCallHandler"));
        setExternalCallPrefix(getProperty("externalCallPrefix"));

        setQueryExecuteTime(getProperty("queryExecuteTime"));
        setT2QueryExecuteLogFile(getProperty("T2QueryExecuteLogFile"));
        setEnableLog(getProperty("enableLog"));
        setIdMapFile(getProperty("idMapFile"));
        
        setStatisticsIntervalTime(getProperty("statisticsIntervalTime"));
        setStatisticsLimitTime(getProperty("statisticsLimitTime"));
        setStatisticsType(getProperty("statisticsType"));
        setProgramStatisticsEnabled(getProperty("programStatisticsEnabled"));
        setStatisticsSqlPlanEnabled(getProperty("statisticsSqlPlanEnabled"));
        
    }
    public Properties getProperties(){
        Properties props = new Properties();

        if (getCatalog() != null)
            props.setProperty("catalog", catalog_);
        if (getSchema() != null)
            props.setProperty("schema", schema_);

        props.setProperty("batchBinding", String.valueOf(batchBindingSize_));
        if (getSpjrs() != null)
            props.setProperty("Spjrs", Spjrs_);

        if (getStmtatomicity() != null)
            props.setProperty("stmtatomicity", stmtatomicity_);
        // props.setProperty("",);
        props.setProperty("transactionMode", String.valueOf(transactionMode_));
        if (getIso88591EncodingOverride() != null)
            props.setProperty("ISO88591", iso88591EncodingOverride_);
        if (getContBatchOnError() != null)
            props.setProperty("contBatchOnError", contBatchOnError_);

        props.setProperty("maxIdleTime", String.valueOf(maxIdleTime_));
        props.setProperty("maxPoolSize", String.valueOf(maxPoolSize_));
        props.setProperty("maxStatements", String.valueOf(maxStatements_));
        props.setProperty("minPoolSize", String.valueOf(minPoolSize_));
        props.setProperty("initialPoolSize", String.valueOf(initialPoolSize_));
 	props.setProperty("inlineLobChunkSize", String.valueOf(inlineLobChunkSize_));
 	props.setProperty("lobChunkSize", String.valueOf(lobChunkSize_));

        // props.setProperty("",);
        // props.setProperty("enableLog",);

        props.setProperty("traceFlag", String.valueOf(traceFlag_));

        if(getTraceFile() !=null)
            props.setProperty("traceFile",traceFile_);

        if (getExternalCallHandler() != null)
            props.setProperty("externalCallHandler", externalCallHandler);
        if (getExternalCallPrefix() != null)
            props.setProperty("externalCallPrefix", externalCallPrefix);

        props.setProperty("queryExecuteTime", Long.toString(queryExecuteTime_));
        if (T2QueryExecuteLogFile_ != null)
            props.setProperty("T2QueryExecuteLogFile", T2QueryExecuteLogFile_);
        if (enableLog_ != null)
            props.setProperty("enableLog", enableLog_);
        if (idMapFile != null)
            props.setProperty("idMapFile", idMapFile);

        props.setProperty("statisticsIntervalTime", Integer.toString(statisticsIntervalTime_));
        props.setProperty("statisticsLimitTime", Integer.toString(statisticsLimitTime_));
        props.setProperty("statisticsType", statisticsType_);
        props.setProperty("programStatisticsEnabled", programStatisticsEnabled_);
        props.setProperty("statisticsSqlPlanEnabled", statisticsSqlPlanEnabled_);
        
        return props;
    }
    public java.sql.DriverPropertyInfo[] getPropertyInfo(String url,
            java.util.Properties info) {
        java.sql.DriverPropertyInfo[] propertyInfo = new java.sql.DriverPropertyInfo[15];
        int i = 0;

        propertyInfo[i] = new java.sql.DriverPropertyInfo("catalog",
                catalog_);
        propertyInfo[i].description = "The default catalog used to access SQL objects referenced in SQL statements if the SQL objects are not fully qualified.";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo("schema",
                schema_);
        propertyInfo[i].description = "The default schema used to access SQL objects referenced in SQL statements if the SQL objects are not fully qualified.";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "batchBinding", Integer.toString(batchBindingSize_));
        propertyInfo[i].description = "Specifies that statements are batched together in the executeBatch() operation.";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "maxPoolSize", Integer.toString(maxPoolSize_));
        propertyInfo[i].description = "Sets the maximum number of physical connections that the pool can contain.";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "minPoolSize", Integer.toString(minPoolSize_));
        propertyInfo[i].description = "Limits the number of physical connections that can be in the free connection pool";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "maxStatements", Integer.toString(maxStatements_));
        propertyInfo[i].description = "Sets the total number of PreparedStatement objects that the connection pool should cache.";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "initialPoolSize", Integer.toString(initialPoolSize_));
        propertyInfo[i].description = "Sets the number of physical connections that must exist when the pool is created.";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "maxIdleTime", Integer.toString(maxIdleTime_));
        propertyInfo[i].description = "Sets the number of seconds a physical connection should remain unused in the pool before the connection is closed.";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo("traceFlag",
                Integer.toString(traceFlag_));
        propertyInfo[i].description = "Sets the tracing level for application servers that use the traceFile property.";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo("traceFile",
                traceFile_);
        propertyInfo[i].description = "Specifies the trace file or directory.";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "inlineLobChunkSize", Integer.toString(inlineLobChunkSize_));
        propertyInfo[i].description = "Specifies the LOB chunk size that can be inlined along with row.";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "lobChunkSize", Integer.toString(lobChunkSize_));
        propertyInfo[i].description = "Specifies the LOB chunk size for streaming.";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "transactionMode", SQLMXConnection
                        .mapTxnModeToString(transactionMode_));
        propertyInfo[i].description = "Specifies the transaction mode for the connection.";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo("ISO88591",
                iso88591EncodingOverride_);
        propertyInfo[i].description = "Specifies the charset encoding override for ISO88591 columns.";
        propertyInfo[i++].choices = null;
//
//		propertyInfo[i] = new java.sql.DriverPropertyInfo("language",
//				locale_.getLanguage());
        propertyInfo[i].description = "Specifies the language for the connection.";
        propertyInfo[i++].choices = Locale.getISOLanguages();

        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "stmtatomicity", stmtatomicity_);
        propertyInfo[i].description = "Providing atomicity at statement level";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "contBatchOnError", contBatchOnError_);
        propertyInfo[i].description = "Specifies whether batch execution continues on errors";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "enableLog", enableLog_);
        propertyInfo[i].description = "Enables logging of SQL statement IDs and the corresponding JDBC SQL statements.";
        propertyInfo[i++].choices = null;
        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "idMapFile", idMapFile);
        propertyInfo[i].description = "Specifies the file to which the trace facility logs SQL statement IDs and the corresponding JDBC SQL statements.";
        propertyInfo[i++].choices = null;
        propertyInfo[i] = new java.sql.DriverPropertyInfo("queryExecuteTime",Long.toString(queryExecuteTime_));
        propertyInfo[i].description="Sets the queryExecuteTime";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo("T2QueryExecuteLogFile",	T2QueryExecuteLogFile_);
        propertyInfo[i].description="set the Trace file to log sql queries which are taking more the queryExecuteTime";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "statisticsIntervalTime", Integer.toString(statisticsIntervalTime_));
        propertyInfo[i].description = "Time in seconds on how often the aggregation data should be published. Default is 60";
        propertyInfo[i++].choices = null;
        
        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "statisticsLimitTime", Integer.toString(statisticsLimitTime_));
        propertyInfo[i].description = "Time in seconds for how long the query has been executing before publishing. Default is 60";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "statisticsType", statisticsType_);
        propertyInfo[i].description = "Type of statistics to be published. Default is 'aggregated'";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "programStatisticsEnabled", programStatisticsEnabled_);
        propertyInfo[i].description = "If statistics publication is enabled. Default is 'true'";
        propertyInfo[i++].choices = null;

        propertyInfo[i] = new java.sql.DriverPropertyInfo(
                "statisticsSqlPlanEnabled", statisticsSqlPlanEnabled_);
        propertyInfo[i].description = "If statistics sql plan is enabled. Default is 'true'";
        propertyInfo[i++].choices = null;

        for (i = 0; i < propertyInfo.length; i++) {
            propertyInfo[i].required = false;
        }

        return propertyInfo;
    }
    /**
     * @return Reference object containing all the connection properties.
     *         The reference object can be used to register with naming
     *         services.
     */
    Reference addReferences(Reference ref) {

        ref.add(new StringRefAddr("dataSourceName", getDataSourceName()));
        ref.add(new StringRefAddr("description", getDescription()));
        ref.add(new StringRefAddr("catalog", getCatalog()));
        ref.add(new StringRefAddr("schema", getSchema()));
        ref.add(new StringRefAddr("language", getLanguage()));
        /* Description: Adding the reference to ISO88591 encoding */
        ref.add(new StringRefAddr("ISO88591", getIso88591EncodingOverride()));
        ref.add(new StringRefAddr("batchBinding", Integer
                .toString(getBatchBinding())));
        ref.add(new StringRefAddr("maxStatements", Integer
                .toString(getMaxStatements())));
        ref.add(new StringRefAddr("initialPoolSize", Integer
                .toString(getInitialPoolSize())));
        ref.add(new StringRefAddr("minPoolSize", Integer
                .toString(getMinPoolSize())));
        ref.add(new StringRefAddr("maxPoolSize", Integer
                .toString(getMaxPoolSize())));
        ref.add(new StringRefAddr("maxIdleTime", Integer
                .toString(getMaxIdleTime())));
//		ref.add(new StringRefAddr("propertyCycle", Integer
//				.toString(propertyCycle_)));
        ref.add(new StringRefAddr("inlineLobChunkSize", Integer
                .toString(getInlineLobChunkSize())));
        ref.add(new StringRefAddr("lobChunkSize", Integer
                .toString(getLobChunkSize())));
        ref.add(new StringRefAddr("transactionMode",getTransactionMode()));
        ref.add(new StringRefAddr("contBatchOnError",getContBatchOnError()));
        ref.add(new StringRefAddr("queryExecuteTime",Long.toString(queryExecuteTime_)));
        ref.add(new StringRefAddr("T2QueryExecuteLogFile",T2QueryExecuteLogFile_));

        ref.add(new StringRefAddr("traceFile", traceFile_));
        ref.add(new StringRefAddr("traceFlag",Integer.toString(traceFlag_)));

        ref.add(new StringRefAddr("enableLog",enableLog_));
        ref.add(new StringRefAddr("idMapFile",getIdMapFile()));

        return ref;
    }

    T2Properties getT2Properties(){
        return this;
    }
}
