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

import java.sql.Connection;
import java.sql.PreparedStatement;
import java.sql.Statement;
import java.sql.ResultSet;
import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.text.SimpleDateFormat;
import java.util.Vector;
import java.lang.reflect.Method;
import org.trafodion.jdbc.t4.TrafT4Connection;

public class Session extends RepObjInterface
{

   private int caller=-1;                             // caller id. -1 nobody 0- USERI 1- PRUNI
   private int mode=SessionDefaults.SQL_MODE; // 0 for sql 1 for cs 
   private boolean sessionType=true; // true - interactive false - non interactive
   private boolean sessionStatus=true; // true - active false- in active
   private String sessionUser=null; // session user
   private String sessionRole=null;
   private String tempSessionRole="";
   private String sessionPass=null; // session password
   private String sessionServer=null;
   private String sessionPort=null;
   private String sessionDsn=null;
   private String sessionCtlg="TRAFODION"; // session ctlg
   private String sessionSchema="TRAFODION"; // session schema
   private String sessionSQLPrompt=SessionDefaults.DEFAULT_SQL_PROMPT; // session prompt
   private String sessionCprompt="+>";
   private String sessionWMSPrompt= SessionDefaults.DEFAULT_WMS_PROMPT;
   private String sessionCSPrompt= SessionDefaults.DEFAULT_CS_PROMPT;
   private String sessionSQLTerminator=";"; // session sql terminator
   private String sessionCSTerminator=";"; // session sql terminator
   private String sessionColSep=" "; // column separator
   private int sessionIdletime=30; // session idletimeout period
   private HashMap<String,String> sessParams=null;
   private HashMap<String,Object[]> prepStmtMap=null;
   private boolean sessionTime=false; // false - if time off true - if time on
   private boolean sessionTiming=false; // false - if timing off true - if timing on
   private boolean sessionLogging=false; // is logging enabled
   private String sessNull="NULL";
   private Query qryObj=null; // current query properties
   private Connection connObj=null; // current connection obj - one per session
   private Statement stmtObj=null; // statement object to execute database queries
   private PreparedStatement pstmtObj=null; // current statement obj - one per session
   private FileReader frObj=null;
   private FileWriter fwObj=null;
   private ConsoleReader crObj=null;
   private ConsoleWriter cwObj=null;
   private boolean termOut=true; // print the output in the terminal
   private boolean isQueryInterrupted=false; // set to true when the user cancels the current operation using ctrl +C
   private boolean isDBConnExists=false; // flag to check if the database connection exists or not.
   private FileWriter logWriter=null;
   private FileReader scriptReader=null;
   private Reader readObj=null;
   private Writer writerObj=null;
   private long lastQryExecTime=0;
   private List<Vector<String>> qryHistory=null;
   private String prevSQLQuery=null; // previous SQL query
   private int prevQryStatusCode=0;
   private int listCount=0;
   private long qryStartTime=0;
   private long qryEndTime=0;
   private int sessLF=0; // Look and feel property
   private int sqlWarningsCnt=0;
   private int qsDisplayColSize = SessionDefaults.DISPLAY_COLSIZE;
   private String strDisplayFormat=SessionDefaults.DEFAULT_DISPLAY_FORMAT;
   private Object theEncryptObject=null;
   private String processName=null;
   private boolean patternsLoaded=false;
   private String ndcsVersion = null;
   private String databaseVersion = null;
   private String databaseEdition = null;
   public String serverType=null;

   ValidateQuery     vQryObj    = null;
   InterfaceQuery    iQryObj    = null;
   ConditionalQuery  cQryObj    = null;
   DatabaseQuery     dbQryObj   = null;
   
   SessionTimeoutTask sTimeoutTask=null;
   boolean qsOpen=false;
   private boolean sessionHistoryAll=false;
   String serviceName=null;
   String varSessionSQLPrompt=SessionDefaults.DEFAULT_SQL_PROMPT;
   boolean ampmFmt = false;
   boolean sessionStats = false;
   String sessionStatsType;
   LFProperties lfProps = null;
   Process procObj=null;
   private HTMLObject htmlObj=null;
   private XMLObject xmlObj=null;
   private String termEvent="0";
   private int histCmdNo=0;
   private HashMap<String,String> termEventMap=null;
   private String prevTermEvent="0";
   private boolean sessionAutoPrepare=false;
   private String autoPrepStmtName="STMT_CI";
   private int lastErrorCode = 0;
   private String sutVersion = null;
   private String mxosrvrVersion = null;
   private long qryExecEndTime=0;
   private boolean writeParams = false;
   private String totalRecordCount = "0";
   private float t4verNum = 0;
   private boolean spjRs = false;
   private int exitCode = 0;
   private boolean inOutandRS=false;
   private float dbProdVersion=0;
   private boolean logCmdText = true;
   private boolean quietEnabled = false;
   private boolean logAppend = true;
   Object currentDbExecObj=null;
   String spoolFileName=null;
   private boolean isInteractiveRead=false;
   private boolean isImplicitGetStatsQry=false;
   private boolean isCmdEchoEnabled=false;
   // Changes for charset alignment
   private int isoMapping=-1;
   private boolean multiByteAlign=true;
   private HashMap<String,String> aliasParamsMap=null;
   private int fetchSize = SessionDefaults.DEFAULT_FETCH_SIZE;
   private HashMap<String,RepObjInterface> patternHashMap=null;
   private HashMap<String,String> envMap = null;
   private HashMap<String,String> regExpMap = null;
   private int maxPatternDepth = SessionDefaults.MAX_PATTERN_DEPTH;
   private boolean debugOn = false;
   private boolean logCmdEcho = true;
   private boolean sessionStartup = false;
   
   private int prevMode = SessionDefaults.SQL_MODE;
   private boolean dotModeCmd = false; 
   
   private boolean histOn = true; 
   private String dotMode = null;

   /*
    *  Constants for CI's default connection timeout value and for a 
    *  no timeout value. 
    */
   public static final int  DEFAULT_CONNECTION_TIMEOUT = -999; // Defaults.
   public static final int  INFINITE_CONNECTION_TIMEOUT = 0;   // No timeout.


   /*
    *  Static variables containing the connection timeout and reset idle timer
    *  method. The _s_ndcsConnTimeout value indicates whether or not the
    *  user overrode the server side connection timeout defaults.
    */
   private static int _s_ndcsConnTimeout = DEFAULT_CONNECTION_TIMEOUT;
   private static Method _s_resetIdleTimerMethod = null;

   private int versionInfo = 2000;

   public int getVersionInfo() {
	return versionInfo;
   }

   public void setVersionInfo(int versionInfo) {
	this.versionInfo = versionInfo;
   }

static {
      String connTimeoutValue = "";

      try {
         /*
          *  Check if there was a trafci.connectionTimeout property
          *  passed to us and use its value if its valid.
          */
         connTimeoutValue = System.getProperty("trafci.connectionTimeout");
         if (null != connTimeoutValue)
            _s_ndcsConnTimeout = Integer.parseInt(connTimeoutValue);

	  } catch(NumberFormatException nfe) {
         /*
          *  Parsing failed -- log an error and use the default value.
          */
         System.err.println("Invalid value specified for " + 
                            "trafci.connectionTimeout [" + connTimeoutValue + 
                            "]. Using defaults ... ");
         _s_ndcsConnTimeout = DEFAULT_CONNECTION_TIMEOUT;
      }


      try {
         /*
          *  Get the class for the TrafT4Connection class and find the
          *  reset connection (server) idle timer.
          */
         Class<?>  clz = Class.forName("org.trafodion.jdbc.t4.TrafT4Connection");
         _s_resetIdleTimerMethod = clz.getMethod("resetServerIdleTimer",
                                                 new Class[] { } );
 
       } catch(Exception e) {
         _s_resetIdleTimerMethod = null;
       }


   }  /*  End of  static initializer.  */



   Session()
   {

   }

   Session(String sessionUser,   String sessionPass, String sessionRole ,String sessionServer, String sessionPort,
      String sessionDsn,ConsoleReader crObj, ConsoleWriter cwObj)
   {
      this.sessionUser=sessionUser;
      this.sessionPass=sessionPass;
      this.sessionRole=sessionRole;
      this.sessionServer=sessionServer;
      this.sessionPort=sessionPort;
      this.sessionDsn=sessionDsn;
      this.crObj=crObj;
      this.cwObj=cwObj;


   }

   /*
    *  Gets the Connection Timeout setting. This value can be overriden by 
    *  the user by specifying a property "trafci.connectionTimeout".
    *
    *  @returns  the currently used Connection Timeout setting.  
    */
   public static int getConnectionTimeout() {
      return _s_ndcsConnTimeout;

   }  /*  End of  getConnectionTimeout  static method.  */



   /*
    *  Resets the connection idle timer on the server. Uses reflection to 
    *  invoke the method, so that we don't need to compile with a specific
    *  JDBC/T4 driver version.
    */
   public void resetConnectionIdleTimer() {
        /*
         *  Don't need to do any work, if we set an infinite timeout at
         *  connection time.
         */
        if (INFINITE_CONNECTION_TIMEOUT == _s_ndcsConnTimeout)
           return;


        /*
         *  Okay, gotta reset the connection idle timer if we have a valid
         *  resetIdleTimer method handle.
         */
        if (null != _s_resetIdleTimerMethod) {
		   //  System.out.println("@@@TRC: invoking reset timer.");

           try {
              _s_resetIdleTimerMethod.invoke(connObj, new Object[] { } );

              //  We did our job, so just return back.
              return;

           } catch(Exception e) {
		      //  System.out.println("@@@TRC: failed invoking reset timer. " + 
              //                     "Details = " + e.getMessage() );


              //  Don't want to do this again if it failed.
              _s_resetIdleTimerMethod = null;

             // if (Boolean.getBoolean("trafci.enableTrace") )
             //    e.printStackTrace();

           }

        }   /*  End of  IF  resetIdleTimerMethod  is valid.  */



        /*
         *  Okay, if we got here -- means we failed to invoke the reset
         *  connection idle timer method -- run a dummy query to reset 
         *  the timer on the NDCS end.
         */
        try {
		   //  System.out.println("@@@TRC: running infostats keepalive.");
           Statement stmt = connObj.createStatement();
           ResultSet rs = stmt.executeQuery("INFOSTATS _KEEPALIVE_HANDLE_");
           rs.close();
           stmt.close();

        } catch(Exception exc) {
           //  Ignore any errors.
        }


   }  /*  End of  resetConnectionIdleTimer  method.  */



   public Connection getConnObj()
   {
      return connObj;
   }

   public void setConnObj(Connection connObj)
   {
      this.connObj = connObj;
      this.prepStmtMap=null;

   }

   public int getMode()
   {
      return mode;
   }

   public int getDisplayFormat()
   {
      if (this.strDisplayFormat.equalsIgnoreCase("HTML"))
         return SessionDefaults.HTML_FORMAT;
      else if (this.strDisplayFormat.equalsIgnoreCase("XML"))
         return SessionDefaults.XML_FORMAT;
      else if (this.strDisplayFormat.equalsIgnoreCase("CSV") || this.strDisplayFormat.equalsIgnoreCase("COLSEP"))
         return SessionDefaults.CSV_FORMAT;
      else
         return SessionDefaults.RAW_FORMAT;
   }

   public String getStrMode()
   {
      switch (this.mode)
      {
         case SessionDefaults.SQL_MODE:
            return "SQL";
         default:
            return "SQL";
      }
   }

   public String getEnv(String var)
   {
       var=var.toUpperCase();
       
       if(var.equals("RECCOUNT") || var.equals("ACTIVITYCOUNT"))
       {
           String tmp = this.getTotalRecordCount();
           if(tmp == null)
               return "-1";
           else
               return tmp;
       }
       
       if(var.equals("LASTERROR") || var.equals("ERRORCODE"))
           return this.getLastError() + "";
       
       if(var.equals("COLSEP"))
           return this.getSessionColSep();
       
       if(var.equals("DATASOURCE"))
           return this.getSessionDsn();
       
       if(var.equals("HISTOPT"))
           return this.isSessionHistoryAll()?"ALL":"DEFAULT [No expansion of script files]";
   
       
       if(var.equals("IDLETIMEOUT"))
           return this.getSessionIdletime()+""; 
       
       if(var.equals("LIST_COUNT"))
           return this.getListCount()+"";
       
       if(var.equals("LOG_FILE"))
       {
           if(this.isSessionLogging())
               return this.getSpoolFileName();
           else
               return "OFF";  
       }
       
       if(var.equals("LOOK_AND_FEEL"))
           return this.getStrSessView();
       
       if(var.equals("MARKUP"))
           return this.getStrDisplayFormat();
       
       if(var.equals("MODE"))
           return this.getStrMode();
       
       if(var.equals("PROMPT"))
           return this.getSessionPrompt();
       
       if(var.equals("ROLE"))
    	   return this.getSessionRole();
       
       if(var.equals("SCHEMA"))
           return this.getSessionSchema();
       
       if(var.equals("SERVER"))
           return this.getSessionServer() + this.getSessionPort();
       
       if(var.equals("SQLTERMINATOR"))
           return this.getSessionSQLTerminator();
       
       if(var.equals("TIME"))
           return this.isSessionTimeOn()?"ON":"OFF";
           
       if(var.equals("TIMING"))
           return this.isSessionTimingOn()?"ON":"OFF";
           
       if(var.equals("USER"))
           return this.getSessionUser();
       
       return null;
   }
   
   public void setNewPrompt()
   {
      // include all possible combinations of env variables here
      String[][] envNameVal = { {"%USER",this.sessionUser},
                              {"%SCHEMA",this.sessionSchema},
                              {"%SERVER",this.sessionServer+this.sessionPort},
                              {"%MODE",this.getStrMode()},
                              {"%DATASOURCE", this.sessionDsn},
                              {"%ROLE", this.sessionRole}
                        };
      String prompt=this.varSessionSQLPrompt;
      switch (this.mode)
      {
         case SessionDefaults.SQL_MODE:
            prompt = this.varSessionSQLPrompt;
            break;
         default:
            break;
      }
      int envCount=0;
      String replaceString;
      while (this.connObj != null && prompt.indexOf("%") != -1 && envCount < envNameVal.length)
      {
    	
      	// Every $ needs to be replaced with \$. Do not delete the backslashes.
    	 replaceString = envNameVal[envCount][1];
    	 replaceString = replaceString.replaceAll("\\$","\\\\\\$");
         prompt=prompt.replaceAll("(?i)"+envNameVal[envCount][0]+"(?![^_\\W])",replaceString);
         envCount++;
      }

      if (!prompt.trim().endsWith(">"))
    	  prompt += ">";
      switch (this.mode)
      {
         case SessionDefaults.SQL_MODE:
            this.sessionSQLPrompt = prompt;
            break;
         default:
            break;
      }
      
      crObj.setPrompt(prompt,this.sessionTime, ampmFmt);
   }

   public void setMode(int mode)
   {
      this.mode = mode;
      this.setNewPrompt();
   }

   public PreparedStatement getPreparedStmt()
   {
      return pstmtObj;
   }

   public void setPreparedStmt(PreparedStatement pstmtObj)
   {
      this.pstmtObj = pstmtObj;
   }

   public Query getQuery()
   {
      return qryObj;
   }

   public void setQuery(Query qryObj)
   {
      this.qryObj = qryObj;
   }

   public String getSessionCtlg()
   {
      return sessionCtlg;
   }

   public void setSessionCtlg(String sessionCtlg)
   {
      this.sessionCtlg = sessionCtlg;
   }

   public String getSessionPass()
   {
      return sessionPass;
   }

   public void setSessionPass(String sessionPass)
   {
      this.sessionPass = sessionPass;
   }

   public String getSessionPrompt()
   {
      String timeStamp = "";
      SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss ");
      String prompt="";

      if (this.isSessionTimeOn())
      {
         if (this.ampmFmt)
            timeStamp = DateFormat.getTimeInstance().format(new Date())+ " ";
         else
            timeStamp = sdf.format(new Date());
      }
      
      prompt = this.getPrompt();
      crObj.setPrompt(prompt, sessionTime, ampmFmt);
      return (timeStamp + prompt);
   }

   public String getPrompt() {
      
      String prompt="";
     
      switch (this.mode)
      {
         case SessionDefaults.SQL_MODE:
            prompt =  sessionSQLPrompt;
            break;
         default:
            prompt =  sessionSQLPrompt;
      }
      return prompt;
   }
   
   public void setSessionPrompt(String sessionPrompt)
   {
      switch (this.mode)
      {
         case SessionDefaults.SQL_MODE:
            this.sessionSQLPrompt = this.varSessionSQLPrompt = sessionPrompt;
            break;
         default: break;
      }
      this.setNewPrompt();
   }

   public String getSessionSchema()
   {
      return sessionSchema;
   }

   public void setSessionSchema(String sessionSchema)
   {
      this.sessionSchema = sessionSchema;
      this.setNewPrompt();
   }

   public boolean isSessionActive()
   {
      return sessionStatus;
   }

   public void setSessionStatus(boolean sessionStatus)
   {
      this.sessionStatus = sessionStatus;
   }

   public boolean isSessionTimeOn()
   {
      return sessionTime;
   }

   public void setSessionTime(boolean sessionTime)
   {
      this.sessionTime = sessionTime;
   }

   public boolean isSessionTimingOn()
   {
      return sessionTiming;
   }

   public void setSessionTiming(boolean sessionTiming)
   {
      this.sessionTiming = sessionTiming;
   }

   public boolean isSessionInteractive()
   {
      return sessionType;
   }

   public void setSessionType(boolean sessionType)
   {
      this.sessionType = sessionType;
   }

   public boolean getSessionType()
   {
      return this.sessionType;
   }

   public String getSessionUser()
   {
      return sessionUser;
   }

   public void setSessionUser(String sessionUser)
   {
      this.sessionUser = sessionUser;
      this.setNewPrompt();
   }


   public ConsoleReader getConsoleReader()
   {
      return crObj;
   }


   public void setConsoleReader(ConsoleReader crObj)
   {
      this.crObj = crObj;
   }


   public ConsoleWriter getConsoleWriter()
   {
      return cwObj;
   }


   public void setConsoleWriter(ConsoleWriter cwObj)
   {
      this.cwObj = cwObj;
   }


   public FileReader getFileReader()
   {
      return frObj;
   }


   public void setFileReader(FileReader frObj)
   {
      if (frObj == null)
      {
         frObj=new FileReader();
      }
      this.frObj = frObj;
   }


   public FileWriter getFileWriter()
   {
      return fwObj;
   }


   public void setFileWriter(FileWriter fwObj)
   {
      if (fwObj == null)
      {
         fwObj=new FileWriter();
      }
      this.fwObj = fwObj;
   }

   public String getStrDisplayFormat()
   {
      return strDisplayFormat;
   }

   public void setStrDisplayFormat(String strDisplayFormat)
   {
      this.strDisplayFormat = strDisplayFormat;
   }

   public String getSessionDsn()
   {
      return sessionDsn;
   }

   public void setSessionDsn(String sessionDsn)
   {
      this.sessionDsn = sessionDsn;
      this.setNewPrompt();
   }

   public String getSessionPort()
   {
      return sessionPort;
   }

   public void setSessionPort(String sessionPort)
   {
      this.sessionPort = sessionPort;
   }

   public String getSessionServer()
   {
      return sessionServer;
   }

   public void setSessionSever(String sessionServer)
   {
      this.sessionServer = sessionServer;
      this.setNewPrompt();
   }

   public String getSessionSQLTerminator()
   {
      if (this.mode != SessionDefaults.SQL_MODE)
      {
         return sessionCSTerminator;
      }
      else
      {
         return sessionSQLTerminator;
      }
   }

   public void setSessionSQLTerminator(String sessionSQLTerminator)
   {
      this.sessionSQLTerminator = sessionSQLTerminator.toUpperCase();
   }

   public int getSessionIdletime()
   {
      return sessionIdletime;
   }

   public void setSessionIdletime(int sessionIdletime)
   {
      this.sessionIdletime = sessionIdletime;
      if (this.sTimeoutTask != null)
      {
         this.sTimeoutTask.idleTime=sessionIdletime;
      }
   }

   public HashMap<String,Object[]> getPrepStmtMap()
   {
      return prepStmtMap;
   }

   public PreparedStatement getPrepStmtMap(String stmtName)
   {
      if (prepStmtMap !=null)
      {
         Object[] pStmtObj=(Object[]) prepStmtMap.get(stmtName);
         if (pStmtObj != null)
         {
            return (PreparedStatement) (pStmtObj[0]);
         }
      }
      return null;
   }

   public boolean getPrepTrimOut(String stmtName)
   {
      if (prepStmtMap !=null)
      {
         Object[] pStmtObj=(Object[]) prepStmtMap.get(stmtName);

         return (Boolean.valueOf((String)pStmtObj[1]).booleanValue());
      }
      else
         return false;
   }

   public void setPrepStmtMap(String stmtName,Object pStmtObj,boolean trimOut)
   {
      Object[] objectArr=new Object[2];
      if (this.prepStmtMap == null)
      {
         this.prepStmtMap=new HashMap<String,Object[]>();
      }
      objectArr[0]=pStmtObj;
      if (trimOut)
      {
         objectArr[1]="true";
      }
      else
      {
         objectArr[1]="false";
      }
      this.prepStmtMap.put(stmtName,objectArr);

   }

   public void removePrepStmtMap(String stmtName)
   {
      if (this.prepStmtMap != null)
      {
         this.prepStmtMap.remove(stmtName);
      }
   }

   public HashMap<String,String> getSessParams()
   {
      return sessParams;
   }

   public void resetPrepStmtMap(HashMap<String,Object[]> prepStmtMap)
   {
      this.prepStmtMap=null;
   }

   public String getSessParams(String param)
   {
      if (sessParams!=null)
         return (String) sessParams.get(param);
      else
         return null;

   }

   public void setSessParams(String param,String value)
   {
      if (this.sessParams == null)
      {
         this.sessParams=new HashMap<String,String>();
      }
      this.sessParams.put(param,value);
   }

   public void resetSessionParams(HashMap<String,String> sessParams)
   {
      this.sessParams=sessParams;
   }

   public void resetSessionParams(String param)
   {
      if (this.sessParams != null)
      {
         this.sessParams.remove(param);
      }
   }

   public boolean isSessionLogging()
   {
      return sessionLogging;
   }

   public void setSessionLogging(boolean sessionLogging)
   {
      this.sessionLogging = sessionLogging;
   }

   public Statement getStmtObj()
   {
      return stmtObj;
   }

   public void setStmtObj(Statement stmtObj)
   {
      this.stmtObj = stmtObj;
   }

   public String getSessNull()
   {
      return sessNull;
   }

   public void setSessNull(String sessNull)
   {
      this.sessNull = sessNull;
   }

   public String getSessionCprompt()
   {
      return sessionCprompt;
   }

   public void setSessionCprompt(String sessionCprompt)
   {
      this.sessionCprompt = sessionCprompt;
   }

   public boolean isTermOut()
   {
      return termOut;
   }

   public void setTermOut(boolean termOut)
   {
      this.termOut = termOut;
   }

   public FileWriter getLogWriter()
   {
      return logWriter;
   }

   public void setLogWriter(FileWriter logWriter)
   {
      this.logWriter = logWriter;
   }

   public FileReader getScriptReader()
   {
      return scriptReader;
   }

   public void setScriptReader(FileReader scriptReader)
   {
      this.scriptReader = scriptReader;
   }

   public int getCaller()
   {
      return caller;
   }

   public void setCaller(int caller)
   {
      this.caller = caller;
   }

   public DatabaseQuery getDbQryObj()
   {
      return dbQryObj;
   }

   public void setDbQryObj(DatabaseQuery dbQryObj)
   {
      this.dbQryObj = dbQryObj;
   }

   public InterfaceQuery getIQryObj()
   {
      return iQryObj;
   }

   public void setIQryObj(InterfaceQuery qryObj)
   {
      iQryObj = qryObj;
   }
   
   public ConditionalQuery getCQryObj(){
       return cQryObj;
   }
   
   public void setCQryObj(ConditionalQuery qryObj) {
       cQryObj = qryObj;
   }

   public ValidateQuery getVQryObj()
   {
      return vQryObj;
   }

   public void setVQryObj(ValidateQuery qryObj)
   {
      vQryObj = qryObj;
   }

   public Reader getReader()
   {
      return readObj;
   }

   public void setReader(Reader readObj)
   {
      this.readObj = readObj;
   }

   public Writer getWriter()
   {
      return writerObj;
   }

   public void setWriter(Writer writerObj)
   {
      this.writerObj = writerObj;
   }

   public long getLastQryExecTime()
   {
      return lastQryExecTime;
   }

   public void setLastQryExecTime(long lastQryExecTime)
   {
      this.lastQryExecTime = lastQryExecTime;
      if (this.sTimeoutTask != null)
      {
         this.sTimeoutTask.lastQueryExecTime=lastQryExecTime;
         this.sTimeoutTask.checkRequired=true;
      }
   }

   public SessionTimeoutTask getTimeoutTask()
   {
      return sTimeoutTask;
   }

   public void setTimeoutTask(SessionTimeoutTask timeoutTask)
   {
      sTimeoutTask = timeoutTask;

      if (null != sTimeoutTask)
         sTimeoutTask.setSessionObj(this);

   }

   public void setTimerHold()
   {
      if (this.sTimeoutTask != null)
      {
         this.sTimeoutTask.checkRequired=false;
      }

   }

   public List<Vector<String>> getQryHistory()
   {
      return qryHistory;
   }

   public void addQryToHistory(String qryText)
   {
	   if (isDotModeCmd()) {
		   qryText = "."+ this.getDotMode() + " " + qryText;
		   if (this.debugOn)
			   System.out.println(this.getClass().getName()+"::"+ qryText);
	   }

	   if (!histOn)
	   {
		   this.setHistOn(true);
		   return;
	   }
		   
      histCmdNo++;
      if (this.qryHistory == null)
      {
         this.qryHistory =new ArrayList<Vector<String>>();
      }
      if (this.termEventMap == null)
      {
         this.termEventMap=new HashMap<String, String>();
         termEventMap.put("0",";");
      }
      if (this.qryHistory.size() >= SessionDefaults.QRY_HISTORY_MAX_LMT)
      {
         this.qryHistory.remove(0);
      }
      Vector<String> histVect = new Vector<String>();
      //mark
      //histVect.add(new Integer(histCmdNo));
      histVect.add(String.valueOf(histCmdNo));
      histVect.add(qryText);
      if (Integer.parseInt(termEvent) == histCmdNo)
      {
         histVect.add(prevTermEvent);
      }
      else
      {
         histVect.add(termEvent);
      }
      this.qryHistory.add(histVect);

   }

   public String getPrevSQLQuery()
   {
      return prevSQLQuery;
   }

   public void setPrevSQLQuery(String prevSQLQuery)
   {
      this.prevSQLQuery = prevSQLQuery;
   }

   public int getPrevQryStatusCode()
   {
      return prevQryStatusCode;
   }

   public void setPrevQryStatusCode(int prevQryStatusCode)
   {
      this.prevQryStatusCode = prevQryStatusCode;
   }

   public int getListCount()
   {
      return listCount;
   }

   public void setListCount(int listCount)
   {
      this.listCount = listCount;
   }

   public int getDisplayColSize()
   {
      return qsDisplayColSize;
   }

   public void setDisplayColSize(int colSize)
   {
      this.qsDisplayColSize = colSize;
   }

   public String getSessionColSep()
   {
      return sessionColSep;
   }

   public void setSessionColSep(String sessionColSep)
   {
      this.sessionColSep = sessionColSep;
   }

   public void setQryStartTime()
   {
      qryStartTime=System.currentTimeMillis();
   }

   public void setQryEndTime()
   {
      qryEndTime=System.currentTimeMillis();
      if (qryObj != null)
         qryObj.setElapsedTime(this.qryEndTime-this.qryStartTime);
   }



   public void resetQryStartEndTime()
   {
      qryStartTime=qryEndTime=0;
   }

   public int getSessView()
   {
      return sessLF;
   }

   public String getStrSessView()
   {
      switch (sessLF)
      {
         case SessionDefaults.CIDEFAULT_VIEW:
            return "TRAFCI";

         case SessionDefaults.MXCI_VIEW:
            return "MXCI";

         default:
            return "TRAFCI";

      }
   }

   public void setSessView(int sessLF)
   {
      this.sessLF = sessLF;
   }

   public int getSqlWarningsCnt()
   {
      return sqlWarningsCnt;
   }

   public void setSqlWarningsCnt(int sqlWarningsCnt)
   {
      this.sqlWarningsCnt = sqlWarningsCnt;
   }

   public boolean isQueryInterrupted()
   {
      return isQueryInterrupted;
   }

   public void setQueryInterrupted(boolean isQueryInterrupted)
   {
      this.isQueryInterrupted = isQueryInterrupted;
   }

   public boolean isDBConnExists()
   {
      return isDBConnExists;
   }

   public void setDBConnExists(boolean isDBConnExists)
   {
      this.isDBConnExists = isDBConnExists;
      if (!isDBConnExists)
      {
         setMxosrvrVersion(null);
      }
   }

   public void setCurrentStmtObj(Object currentDbExecObj)
   {
      this.currentDbExecObj=currentDbExecObj;
   }
   public Object getCurrentStmtObj()
   {
      return this.currentDbExecObj;
   }

   public boolean isSessionHistoryAll()
   {
      return sessionHistoryAll;
   }

   public void setSessionHistoryAll(boolean sessionHistoryAll)
   {
      this.sessionHistoryAll = sessionHistoryAll;
   }


   public boolean isSessionStatsEnabled()
   {
      return sessionStats;
   }

   public void setSessionStatsEnabled(boolean sessionStats)
   {
      this.sessionStats = sessionStats;
   }

   public LFProperties getLfProps()
   {
      return lfProps;
   }

   public String getSessionStatsType()
   {
      return sessionStatsType;
   }

   public void setSessionStatsType(String sessionStatsType)
   {
      this.sessionStatsType = sessionStatsType;
   }

   public void setLFProps(LFProperties lfProps)
   {
      this.lfProps = lfProps;
   }

   public boolean isAmPmFmt()
   {
      return ampmFmt;
   }

   public void setAmPmFmt(boolean ampmFmt)
   {
      this.ampmFmt = ampmFmt;
   }

   public Process getProcObj()
   {
      return procObj;
   }

   public void setProcObj(Process procObj)
   {
      this.procObj = procObj;
   }

   public HTMLObject getHtmlObj()
   {
      return htmlObj;
   }

   public void setHtmlObj(HTMLObject htmlObj)
   {
      this.htmlObj = htmlObj;
   }

   public XMLObject getXmlObj()
   {
      return xmlObj;
   }

   public void setXmlObj(XMLObject xmlObj)
   {
      this.xmlObj = xmlObj;
   }

   public String getTermEvent()
   {
      return termEvent;
   }

   public void setTermEvent(String termEvent)
   {
      prevTermEvent = this.termEvent;
      this.termEvent = termEvent;
   }

   public int getHistCmdNo()
   {
      return histCmdNo;
   }

   public void setHistCmdNo(int histCmdNo)
   {
      this.histCmdNo = histCmdNo;
   }

   public HashMap<String, String> getTermEventMap()
   {
      return termEventMap;
   }

   public void setTermEventMap(String eventNum, String sqlTerminator)
   {
      if (this.termEventMap == null)
      {
         this.termEventMap=new HashMap<String, String>();
         termEventMap.put("0",";");
      }

      this.termEventMap.put(eventNum,sqlTerminator);

   }

   public long getQryExecEndTime()
   {
      return qryExecEndTime;
   }

   public void setQryExecEndTime()
   {
      this.qryExecEndTime = (System.currentTimeMillis() - this.qryStartTime);
   }

   public boolean isSessionAutoPrepare()
   {
      return sessionAutoPrepare;
   }

   public void setSessionAutoPrepare(boolean sessionAutoPrepare)
   {
      this.sessionAutoPrepare = sessionAutoPrepare;
   }

   public String getAutoPrepStmtName()
   {
      return autoPrepStmtName;
   }

   public void setAutoPrepStmtName(String autoPrepStmtName)
   {
      this.autoPrepStmtName = autoPrepStmtName;
   }

   public int getLastError()
   {
      return lastErrorCode;
   }

   public void setLastError(int lastErrorCode)
   {
      this.lastErrorCode = lastErrorCode;
   }
   
   public String getSutVersion()
   {
      return sutVersion;
   }

   public void setSutVersion(String sutVersion)
   {
      this.sutVersion = sutVersion;
   }
   
   public String getMxosrvrVersion()
   {
      return mxosrvrVersion;
   }

   public void setMxosrvrVersion(String mxosrvrVersion)
   {
      this.mxosrvrVersion = mxosrvrVersion;
   }

/* public NSCmd getNscmd()
   {
      return nscmd;
   }

   public void setNscmd(NSCmd nscmd)
   {
      this.nscmd = nscmd;
   }
*/

   public boolean isWriteParams()
   {
      return writeParams;
   }

   public void setWriteParams(boolean writeParams)
   {
      this.writeParams = writeParams;
   }

   public String getTotalRecordCount()
   {
      return totalRecordCount;
   }

   public void setTotalRecordCount(String totalRecordCount)
   {
      this.totalRecordCount = totalRecordCount;
   }

   public void setT4verNum(float num)
   {
      t4verNum = num;
   }
   public float getT4verNum()
   {
      return t4verNum;
   }

   public boolean isSPJRS()
   {
      return spjRs;
   }

   public void setSPJRS(boolean spjRs)
   {
      this.spjRs = spjRs;
   }
   
   public void setExitCode(int code){
       this.exitCode = code;
   }
   
   public int getExitCode(){
       return this.exitCode;
   }

   public boolean isInOutandRS()
   {
      return inOutandRS;
   }

   public void setInOutandRS(boolean inOutandRS)
   {
      this.inOutandRS = inOutandRS;
   }

   public float getDatabaseProdVersion()
   {
      return dbProdVersion;
   }

   public void setDatabaseProdVersion(float dbProdVersion)
   {
      this.dbProdVersion = dbProdVersion;
   }

   public boolean isLogCmdText()
   {
      return logCmdText;
   }

   public void setLogCmdText(boolean logCmdText)
   {
      this.logCmdText = logCmdText;
   }

   public boolean isQuietEnabled()
   {
      return quietEnabled;
   }

   public void setQuietEnabled(boolean quietEnabled)
   {
      this.quietEnabled = quietEnabled;
   }

   public boolean isLogAppend()
   {
      return logAppend;
   }

   public void setLogAppend(boolean logAppend)
   {
      this.logAppend = logAppend;
   }

   public String getSpoolFileName()
   {
      return spoolFileName;
   }

   public void setSpoolFileName(String spoolFileName)
   {
      this.spoolFileName = spoolFileName;
   }
   
   protected void setSessionValues() 
   {	  
	   
      try
      { 
    	this.sessionRole = ((TrafT4Connection)this.connObj).getRoleName();
    	if (this.debugOn)
    		System.out.println("Session::"  + this.sessionRole);
    	if (0 == this.sessionRole.trim().length())  // If T4 returns empty string
    	{
    		this.sessionRole = this.getUserRoleName();
        	if (this.debugOn)
        		System.out.println("Session getUserRoleName::"  + this.sessionRole);
        }    			  
      } 
      catch (NullPointerException npe) 
      {    	 
    	this.sessionRole = this.getUserRoleName();
       	if (this.debugOn)
    		System.out.println("Session NPE getUserRoleName::"  + this.sessionRole);
      } 
      catch (Exception se)
      {
      }
      finally {
    	  this.setNewPrompt();
      }
   }
   
  protected void setBinaryCQDs()
    {
        try
            {
                Statement stmt = this.connObj.createStatement();
                stmt.executeUpdate("CQD TRAF_BINARY_SUPPORT 'ON'");
            } catch (Exception se)
            {
                System.out.println("error:"+se.getMessage());
            }

        try
            {
                Statement stmt = this.connObj.createStatement();
                stmt.executeUpdate("CQD TRAF_BINARY_OUTPUT 'ON'");
            } catch (Exception se)
            {
                System.out.println("error:"+se.getMessage());
            }


    }

   protected String getUserRoleName()
   {

      //String userName = null;
      String userRole = null;
      try
      {
         Statement stmt = this.connObj.createStatement();
         ResultSet rs = stmt.executeQuery("VALUES(USER, CURRENT_ROLE)");
         while (rs!=null && rs.next())
         {
            //userName = rs.getString(1);
            userRole = rs.getString(2);
         }
         rs.close();
         stmt.close();
      } catch (Exception se)
      {
        // System.out.println("error:"+se.getMessage());
      }
      if (userRole != null)
      {
   		 int i = userRole.indexOf(".");
   		 this.sessionRole = userRole.substring(i+1); // Prints the second half of the role
      }
      else
      {
    	  sessionRole = "Information not available";
      }
      return sessionRole;
   }

   
   public boolean isInteractiveRead()
   {
      return isInteractiveRead;
   }

   public void setInteractiveRead(boolean isInteractiveRead)
   {
      this.isInteractiveRead = isInteractiveRead;
   }

   public boolean isImplicitGetStatsQry()
   {
      return isImplicitGetStatsQry;
   }

   public void setImplicitGetStatsQry(boolean isImplicitGetStatsQry)
   {
      this.isImplicitGetStatsQry = isImplicitGetStatsQry;
   }

   public boolean isCmdEchoEnabled()
   {
      return isCmdEchoEnabled;
   }

   public void setCmdEcho(boolean isCmdEchoEnabled)
   {
      this.isCmdEchoEnabled = isCmdEchoEnabled;
   }

   public int getISOMapping()
   {
      return isoMapping;
   }

   public void setISOMapping(int isoMapping)
   {
      this.isoMapping = isoMapping;
   }

   public boolean isMultiByteAlign()
   {
      return multiByteAlign;
   }

   public void setMultiByteAlign(boolean multiByteAlign)
   {
      this.multiByteAlign = multiByteAlign;
   }
   public String getSessionRole()
   {
      return sessionRole;
   }
   
   public void setSessionRole(String sessionRole)
   {
	   this.sessionRole = sessionRole;
	   this.setNewPrompt();
   }
  
   public String getTempSessionRole() {
	return tempSessionRole;
   }

  public void setTempSessionRole(String tempSessionRole) {
	this.tempSessionRole = tempSessionRole;
  }
   public HashMap<String, String> getAliasMap()
   {
      return aliasParamsMap;
   }
   
   public String getAlias(String param)
   {
      if (aliasParamsMap!=null)
         return (String) aliasParamsMap.get(param);
      else
         return null;
   }
   
   public void setAlias(String param,String value)
   {	   
	 if (this.aliasParamsMap == null)
     {
         this.aliasParamsMap=new HashMap<String, String>();
     }
     this.aliasParamsMap.put(param.toUpperCase(),value);
    }

	public int getFetchSize() {
		return fetchSize;
	}
	
	public void setFetchSize(int fetchSize) {
		this.fetchSize = fetchSize;
	}

	public HashMap<String, RepObjInterface> getPatternHashMap() {
		return patternHashMap;
	}
	
	public void setPatternHashMap(HashMap<String, RepObjInterface> patternHashMap) {
		this.patternHashMap = patternHashMap;
	}

	public void setPattern(String patternKey, String patternVal)
	{
		if (patternHashMap == null)
			patternHashMap = new HashMap<String, RepObjInterface>();
		patternHashMap.put(patternKey.toUpperCase(), new ReplacementObj(patternKey, patternVal));
		
	}
	
	
	public String getValue (String key)
	{
		return (String)this.getEnvMap().get(key.toUpperCase());
		
	}
	
	public String getPatternValue(String key)
	{
		if (patternHashMap.containsKey(key.toUpperCase()))
		{
			RepObjInterface repObj = (RepObjInterface)(patternHashMap.get(key.toUpperCase()));
			return (repObj).getValue(key);
		}
		return null;
		
	
	}

	public HashMap<String, String> getEnvMap() {
		if (envMap == null)
			envMap = new HashMap<String, String>();
		return envMap;
	}

	public void setEnvMap(HashMap<String,String> envMap) {
		this.envMap = envMap;
	}

	public HashMap<String, String> getRegExpMap() {
		return regExpMap;
	}

		
	public void setRegExpPattern(String patternKey, String patternVal)
	{
		if (regExpMap == null)
			regExpMap = new HashMap<String, String>();
		regExpMap.put(patternKey, patternVal);
		
	}

	public int getMaxPatternDepth() {
		return maxPatternDepth;
	}

	public void setMaxPatternDepth(int maxPatternDepth) {
		this.maxPatternDepth = maxPatternDepth;
	}

	public boolean isDebugOn() {
		return debugOn;
	}

	public void setDebugOn(boolean debugOn) {
		this.debugOn = debugOn;
	}
	
	public boolean isLogCmdEcho() {
		return logCmdEcho;
	}

	public void setLogCmdEcho(boolean logCmdEcho) {
		this.logCmdEcho = logCmdEcho;
	}

	public boolean isSessionStartup() {
		return sessionStartup;
	}

	public void setSessionStartup(boolean sessionStartup) {
		this.sessionStartup = sessionStartup;
		setSessionLogging(sessionStartup);
		this.logCmdEcho = !sessionStartup;
	}

	public int getPrevMode() {
		return prevMode;
	}

	public void setPrevMode(int prevMode) {
		this.prevMode = prevMode;
	}

	public boolean isDotModeCmd() {
		return dotModeCmd;
	}

	public void setDotModeCmd(boolean dotModeCmd) {
		this.dotModeCmd = dotModeCmd;
	}

	public boolean isHistOn() {
		return histOn;
	}

	public void setHistOn(boolean histOn) {
		this.histOn = histOn;
	}

	public void setTheEncryptObject(Object theEncryptObject) {
		this.theEncryptObject = theEncryptObject;
	}

	public Object getTheEncryptObject() {
		return theEncryptObject;
	}

	public String getNeoProcessName() {
		return processName;
	}

	public void setNeoProcessName(String processName) {
		this.processName = processName;
	}

	public boolean isPatternsLoaded() {
		return patternsLoaded;
	}

	public void setPatternsLoaded(boolean patternsLoaded) {
		this.patternsLoaded = patternsLoaded;
	}

	/**
	 * @return
	 * true : can get Platform version
	 * false: can't get Platform version
	 */
	protected boolean getPlatformObjectVersions()
	{
	      try
	      {
	         Statement stmt = this.connObj.createStatement();
	         ResultSet rs = null; 
	         if (stmt.execute("info system"))
	        	 rs = stmt.getResultSet();
	         
	    	  if (this.isDebugOn() ) {
	    		  System.out.println("rs for info system = " + rs);
	    	  }
	    	  
  	         if (rs!=null && rs.next()) {
	             {
	                ndcsVersion = rs.getString(2);
	                if (ndcsVersion != null){
	                     ndcsVersion = ndcsVersion.substring(ndcsVersion.indexOf("Version"));
	                     sutVersion = ndcsVersion.substring(ndcsVersion.indexOf("Release"),ndcsVersion.indexOf("("));
	                     String platformVersion = sutVersion.substring(8);
	                }
                        databaseVersion = rs.getString(5);
                        databaseEdition = rs.getString(6);

	             }
	             if (this.debugOn) {
	                System.out.println("ndcs version:: " + ndcsVersion);
	                System.out.println("database version:: " + databaseVersion);
	                System.out.println("database edition:: " + databaseEdition);
	             }
	             rs.close();
	             stmt.close();
	         }
	      } catch (Exception se) {
	    	  if (this.isDebugOn() ) {
	    		  System.out.println("error:"+se.getMessage() );
	    		  se.printStackTrace(System.out);
	    	  }
	    	  
	    	  ndcsVersion = null;
	    	  return false;
	      }
	      this.setNdcsVersion(ndcsVersion);
	      return true;
	   }

	public String getNdcsVersion() {
		return ndcsVersion;
	}

	public void setNdcsVersion(String ndcsVersion) {
		this.ndcsVersion = ndcsVersion;
	}

	public String getDotMode() {
		return dotMode;
	}

	public void setDotMode(String dotMode) {
		this.dotMode = dotMode;
	}

	public String getServerType() {
		return serverType;
	}

	public void setServerType(String serverType) {
		this.serverType = serverType;
	}

	public String getDatabaseVersion() {
		return databaseVersion;
	}

	public String getDatabaseEdition() {
 		return databaseEdition;
	}
}

