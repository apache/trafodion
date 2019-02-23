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

import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.sql.Connection;
import java.sql.Driver;
import java.sql.DriverManager;
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.sql.Statement;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Properties;
import java.util.Timer;
import java.util.List;

import sun.misc.Signal;
import sun.misc.SignalHandler;

import org.trafodion.jdbc.t4.TrafT4Connection;


/**
 * Session Interface is a router interface and acts like a dispatcher for query execution.
 *
 * To request for a new session, call the method createSession with the required database credentials. Once the session is 
 * created, its returned to the caller.
 * Caller will then need to call invokeSession method with the sessionObject.
 * 
 */

public class SessionInterface implements SessionDefaults
{
   private static String defaultDataSource=DEFAULT_DATA_SOURCE;
   private ConsoleReader crObj=null;
   private ConsoleWriter cwObj=null;
   private FileReader scriptReader=null;
   private FileWriter logWriter=null;
   private String scriptFile=null;                    // script file used when in prun mode or -script option is specified thru command line
   private String logFileName=null;                   // log file used for prun script file process logging
   private boolean overWriteLog=false;                // log file overwrite mode
   private Properties lfProps=null;                   // look & feel properties.
   private Session sessObj=null;
   //private int sessionInput=-1;                     // 0 - thru console 1- thru file
   private int caller=-1;                             // caller id. -1 nobody 0- USERI 1- PRUNI
   private int sqlErrorCount=0;                       // sql error count..Number of sql errors reported in a session. If a query more than one, then its counted as 1.Needed for prun summary
   private int sqlQueryCount=0;                       // sql query count. Number of sqls executed in a session. Needed for prun summary
   private int sqlWarningsCount=0;
   private boolean queryMode=false;                   // set the session in query mode. As soon as the query is executed..the control is returned back to the caller
   private boolean tmpQueryMode=false;                // temporary flag to identify if the current session is in query mode
   private String qryString=null;                     // caller updates this query string when in query mode
   private Utils utilObj;
   private Parser parser=null;
   Thread threadName=null;
   SignalHandler CTRLCHandler=null;
   Signal INTSignal=null;
   boolean nestedObey=false;
   boolean specialModeEnabled = false;
   LFProperties lfPropsObj = new LFProperties();

   Query qryObj= null;
   private static SimpleDateFormat sdf = new SimpleDateFormat("MMM dd, yyyy HH:mm:ss.SSS a");
   private boolean showTimeInMillis = false;
   //never used
//   private boolean doTrace = Boolean.getBoolean("trafci.enableTrace");
   private String remoteProcess=null;
   
   SessionInterface()
   {

   }


   //Initializes the session interface with a console reader & writer objects
   SessionInterface(ConsoleReader crObj,
      ConsoleWriter cwObj)
   {
      this.crObj=crObj;
      this.cwObj=cwObj;
      CTRLCHandler =new SignalHandler ()
      {
         public void handle(Signal sig)
         {
            sessObj.setQueryInterrupted(true);
         }
      };
      try {
      INTSignal=new Signal("INT");
      } catch (Exception e) {}
   }

   SessionInterface(Session sessObj)
   {
      this.sessObj=sessObj;
      this.caller=sessObj.getCaller();
   }


   //get a statement handle for the current session
   public Statement getStatement(Connection conn) throws SQLException
   {
      return conn.createStatement();
   }

   public Session createSession(String userName,
	  String roleName,
	  String password,
      String serverName,
      String portNumber,
      String dsnName,
      int caller,
      boolean noConnectOption) throws SQLException, InstantiationException, IllegalAccessException, ClassNotFoundException, FileNotFoundException, IOException
   {

      Session session=new Session(   userName,
         password,
         roleName,
         serverName,
         portNumber,
         dsnName,
         crObj,
         cwObj);
      
      return createSession(session,
         caller,
         noConnectOption);
   }

   // create a new session and return it to the caller
   // Any exceptions arise during this process should be returned to the caller,
   // caller will decide where to report them (to the console or to the log file)
   // Each session object is created with one connection, validateQueryObject,
   // Interface query object, dbQuery objects for the entire session to use them 

   public Session createSession(Session session,
      int caller,
      boolean noConnectOption) throws SQLException, InstantiationException, IllegalAccessException, ClassNotFoundException, FileNotFoundException, IOException   {
      utilObj = new Utils();
      
      parser = new Parser();

      sessObj=session;

      // if the user opts to connect to the UI without creating database connection then create db connection and stmt obj
      if (!noConnectOption)
      {
         sessObj.setConnObj(getConnection());
         sessObj.setStmtObj(getStatement(sessObj.getConnObj()));
         sessObj.setSessionValues();
         sessObj.setBinaryCQDs();
         this.getDriverVersion();     
      }
            
      sessObj.setCaller(caller);
      sessObj.setVQryObj(new ValidateQuery(sessObj));
      sessObj.setIQryObj(new InterfaceQuery(sessObj));
      sessObj.setCQryObj(new ConditionalQuery(sessObj));
      sessObj.setDbQryObj(new DatabaseQuery(sessObj));
      sessObj.setHtmlObj(new HTMLObject(sessObj));
      sessObj.setXmlObj(new XMLObject(sessObj));
     

      lfPropsObj.loadLookAndFeelProps(sessObj, specialModeEnabled);
      sessObj.setLFProps(lfPropsObj);

      // start the timer thread to monitor the session idletime.
      // this is need only when the session is not in prun mode. PRUN can enjoy!!!
      // this thread starts immediately after the first minute and runs every one minute then on
      if ((caller != PRUNI) && (!noConnectOption))
      {
         sessObj.setTimeoutTask(new SessionTimeoutTask(sessObj.getConnObj()));
         sessObj.getTimeoutTask().idleTime=sessObj.getSessionIdletime();
         sessObj.getTimeoutTask().lastQueryExecTime=System.currentTimeMillis();
         Timer timer = new Timer();
         timer.schedule(sessObj.getTimeoutTask(), 1 * 60 * 1000, 1 * 60*1000);

      }

      String showTimeStr = System.getProperty("trafci.showTimeInMS");
      if ((showTimeStr != null) && showTimeStr.trim().equalsIgnoreCase("true"))  
         showTimeInMillis = true;

      // return the session object to the caller.
      return sessObj;
   }

   public Session createSession(String userName,
	  String roleName,
	  String password,
      String serverName,
      String portNumber,
      String dsnName,
      int caller
      ) throws SQLException, InstantiationException, IllegalAccessException, ClassNotFoundException, FileNotFoundException, IOException
   {
      return createSession(userName,
    	 roleName,
    	 password,
         serverName,
         portNumber,
         dsnName,
         caller,
         false);
   }

   // start the session with the session object passed
   // in prun mode, this method is called for each script file as prun has to some intermediate calculations between the file processing
   // in useri mode, if the user has logged in non -q mode, called once
   // in useri mode or any other modes which require just one query execution at a time, called as many as times depends on the number of queries
   // returns exit status code
   public int invokeSession(Session sessObj) throws IOException
   {
      this.sessObj=sessObj;
      ValidateQuery vQryObj=sessObj.getVQryObj();
      InterfaceQuery iQryObj=sessObj.getIQryObj();
      ConditionalQuery cQryObj=sessObj.getCQryObj();
      DatabaseQuery dbQryObj=sessObj.getDbQryObj();
      this.caller=sessObj.getCaller();
      Reader reader = new Reader();
      Writer writer = new Writer(sessObj);
      tmpQueryMode=false;
      
      // if the caller is interactive user interface
      // set the reader and writer for the console
      // the read and write modes are set to console modes assuming that there are no script files or obey files 
      if (caller == USERI)
      {
         reader.setConsoleReader(this.crObj);
         writer.setConsoleWriter(this.cwObj);
         reader.setReadMode(CONSOLE_READ_MODE);
         writer.setWriterMode(CONSOLE_WRITE_MODE);
      }

      // if the user has passed a script file to run it while launching the session
      // so read the script file first and later can move to console mode
      // set the read mode to script file read mode.
      // for prun its always script file reading
      // return an exception to the caller if the specified script file is not found or if some error occurred
      
      if (scriptFile != null)
      {

         try
         {
            scriptReader=new FileReader();
            scriptReader.initialize(scriptFile);
            reader.setReadMode(SCRIPT_READ_MODE);
            reader.setScriptReader(scriptReader);
         }catch (FileNotFoundException fnfe)
         {
            throw fnfe;
         }catch (IOException ioe)
         {
            throw ioe;
         }

      }
    
    	  
      // if the log file is specified. log all the results ( errors/warnings and output) to the logfile instead of 
      // console
      // write the header for the log file here..with all blaahhaa.blaah..start time
      // return to the caller if the log file is not found or io exception occurred
      if (logFileName != null)
      {
         try
         {
            logWriter=new FileWriter();
            if (!this.overWriteLog)
               logWriter.setAppend(true);
            logWriter.initialize(logFileName);
            logWriter.write((this.formatString(SessionDefaults.lineSeperator+"=",82,'=')));
            logWriter.writeln();
            if (!showTimeInMillis)
            logWriter.write("Logging started at "+DateFormat.getDateTimeInstance().format(new Date())); //Feb 13, 2007 11:32:31 AM 
            else
               logWriter.write("Logging started at "+  sdf.format(new Date(System.currentTimeMillis())) ); //Feb 13, 2007 11:32:31.999 AM
            logWriter.writeln();
            logWriter.write((this.formatString("=",80,'=')));
            logWriter.writeln();
            writer.setLogWriter(logWriter);
            writer.setWriterMode(LOG_WRITE_MODE);

         }catch (FileNotFoundException fnfe)
         {
            throw fnfe;
         }catch (IOException e)
         {
            throw e;
         }

      }

      // at this point, we know from where to read and where to write. set them in the session so that all other
      // objects will know
      sessObj.setReader(reader);
      sessObj.setWriter(writer);

      // to handle sqlprompts when EOF is reached
      boolean OS_EOF=false;  // obey file & script file EOF indicator
      boolean OBEY_DEPTH_ERROR=false; // for use with OS_EOF, display prompt again after error
      
      threadName=Thread.currentThread();

      try {
         if (CTRLCHandler != null)
         {
            Signal.handle(INTSignal, CTRLCHandler);
         }
      }catch (Exception e) {}

      boolean userPressedCtrlC = false;
      boolean curQryConditionalAction = false;
      String lastObeyFilename = "";

      // loop thru till when the session is active
      if (caller != PRUNI)
         sessObj.setInteractiveRead(reader.getConsoleReader().isInteractive());
    
   
      while (sessObj.isSessionActive())
      {
         sessObj.setQueryInterrupted(false);
         curQryConditionalAction = false;

        //store the previous query for history command

         if (caller == USERI && sessObj.getQuery() != null && !sessObj.getQuery().getQueryText().trim().equals(""))
         {
            // If the history option is set to ALL add all commands  
            // (except OBEY command) in the script file to the history buffer
            // If the history option is set to default, do not add the 
            // commands in script file to history. history should contain
            // only the obey command.
            if ((sessObj.getReader().getReadMode() != OBEY_READ_MODE || !sessObj.getReader().isReallyObeyfile()) ||
               ( !sessObj.isSessionHistoryAll() && sessObj.getQuery().getQueryId() == SessionDefaults.OBEY && !nestedObey) ||
               ( sessObj.isSessionHistoryAll() && sessObj.getQuery().getQueryId() != SessionDefaults.OBEY))
            {
                  sessObj.addQryToHistory(sessObj.getQuery().getQueryText());
            	
               nestedObey = sessObj.getReader().getReadMode() == OBEY_READ_MODE ? true : false;
            }
         }

         // store the previous query's status code for conditional exit
         if (sessObj.getQuery() != null && sessObj.getQuery().getQueryText() != null && !sessObj.getQuery().getQueryText().trim().equals(""))
         {
            sessObj.setPrevQryStatusCode(sessObj.getQuery().getStatusCode());
         }
         // write a new line before each prompt..
         if (!OS_EOF && !cQryObj.isPendingAction())
         {   
            if ( (!userPressedCtrlC) &&  (caller != PRUNI) && sessObj.isLogCmdEcho() &&
            	 !sessObj.isSessionStartup() &&
                 (writer.getConsoleWriter().getConsoleOut()) &&
                 !((sessObj.getSessView() == SessionDefaults.MXCI_VIEW))
                 /* Commenting the following lines of code temporarily. 
                  * When markup is set to csv, there are no new lines 
                  * displayed before the sql prompt. Although commenting
                  * these lines fixes this problem, it does not fix the extra
                  * new lines for insert/update/delete commands in the log files
                  * for CSV markup. Since the first issue was more visible, for 
                  * now commenting this fix till we find a permanent solution
                  * to both these problems.
                  */
                 /*&&
                 !(sessObj.getDisplayFormat() == SessionDefaults.CSV_FORMAT && 
                   (sessObj.getQuery()!= null) &&
                     (sessObj.getQuery().getQueryText().trim().matches("^(?i)(INSERT|UPDATE|DELETE)(.*)")))
                 */    
                  ) 
                  writer.writeln();
         }
         
         // write the current time in the session object so that timer bud knows when to kill 
         sessObj.setLastQryExecTime(System.currentTimeMillis());

         // tmp query mode is initially set to false and when in query mode changed to true after the first query
         // to break the current loop
         if (tmpQueryMode && sessObj.isSessionInteractive())
         {
            break;
         }
         reader=sessObj.getReader();
         writer=sessObj.getWriter();
         //tmpQueryMode=this.queryMode;
         try
         {

             curQryConditionalAction = cQryObj.isPendingAction();
             if (!this.queryMode && !curQryConditionalAction)
             {
               if (!OS_EOF)
               {
            	   if(!sessObj.isSessionStartup()) {
	                  if  (!userPressedCtrlC && sessObj.isLogCmdText() && sessObj.isLogCmdEcho() && reader.getConsoleReader() != null  && !reader.getConsoleReader().isJline() )
	                  {
	                     writer.write(sessObj.getSessionPrompt());
	                  }
	                  
	                  // Need to show the prompt on Console for LOG command with QuietEnabled
	                  if ((sessObj.isQuietEnabled()) && sessObj.isLogCmdEcho() ) 
	                      writer.getConsoleWriter().print(SessionDefaults.lineSeperator+sessObj.getSessionPrompt());
	                  else if(!sessObj.isLogCmdText() && sessObj.isLogCmdEcho())
	                      writer.getConsoleWriter().print(sessObj.getSessionPrompt());
            	  }
               }
               else
               {
                  OS_EOF=false;
               }
            }
            userPressedCtrlC = false;
            qryObj=null;
            sessObj.setQuery(null);

            // for each query create a new query object
            qryObj=new Query();

            String qryLine=cQryObj.getPendingAction();          // if conditional action passed, pendingAction exists

            // only if not in query mode read from the console or script file
            if (queryMode && sessObj.isSessionInteractive())
            {
               qryLine=this.qryString;
               if (!qryLine.endsWith(sessObj.getSessionSQLTerminator()))
               {
                  qryLine+=sessObj.getSessionSQLTerminator();
               }
               tmpQueryMode=queryMode;

            }
            else
            {
               sessObj.setQueryInterrupted(false);      // to avoid the timing issues
               boolean flag = true;
               
               
               /* conditional stmts can set this to values other then null */
               if(qryLine == null)
               {
                   while (flag)
                   {
                      try
                      { 
                         qryLine = reader.getNonBlankLine();
                         if (sessObj.isCmdEchoEnabled() &&
                        	(qryLine != null) && !qryLine.trim().equals(""))
                             writer.getConsoleWriter().println(qryLine);
                         flag = false;
                      }
                      catch (UserInterruption ui)
                      {
                        qryLine = "";
                      }
                      catch(IOException ex){  // thrown by Reader if the obey command loops
                          writer.writeInterfaceErrors(sessObj, ex.getMessage());
                          qryLine = null;
                          flag = false;
                          OBEY_DEPTH_ERROR = true;
                      }
                   }
               }
               if ((null != reader) && (null != reader.getConsoleReader()))
                  reader.getConsoleReader().setQueryInterrupted(false);

            }

            // check if the db connection exists when processing the script or obey files
            // if the connection doesnt exists, skip the remaining lines from the files
            /*
            if (!sessObj.isDBConnExists() && reader.getReadMode() != SessionDefaults.CONSOLE_READ_MODE)
            {
               qryLine=null;
            }
            */
            // if the query is not null..write the query to log file if the current session is
            // writing to the log file. or to the spool file if spooling is enabled
            // if the query is read from an obey file. write the query to the console


            if (qryLine != null) {
               qryObj.setQueryText(parser.ignoreComments(qryLine));
               
               // do not write out a conditional action since it was already written during the IF...THEN statement
               if(!curQryConditionalAction) {
            	   /**
            	    * Bug 2325- Password is saved in log
            	    * Fix Description- if queryStr starts with Connect then log pwd to stars
            	    */
            	   if (qryLine.toUpperCase().startsWith("CONNECT")) {
            		   
            		   int slashIndex = qryLine.lastIndexOf("/");
                		 if (slashIndex > -1) {
                			 // connect user/pwd, mode
                			 int lastCommaIndex = qryLine.lastIndexOf(",");
                			 StringBuilder qryBuilder = new StringBuilder(qryLine);
                			 if (lastCommaIndex > -1)
                				 qryBuilder.replace(slashIndex+1, lastCommaIndex, "*****");
                			 else
                				 qryBuilder.replace(slashIndex+1, qryLine.length(), "*****");
                			 
                			 qryLine = qryBuilder.toString();
                		 }
                	 }
            	   if ((writer.getWriterMode() == SessionDefaults.LOG_WRITE_MODE) && (!sessObj.isSessionStartup()) && sessObj.isLogCmdEcho())
                      writer.getLogWriter().writeln(qryLine);
                   else if (reader.getReadMode() == CONSOLE_READ_MODE && writer.getWriterMode() == SessionDefaults.CONSOLE_SPOOL_WRITE_MODE)
                   {
                      if (sessObj.isLogCmdText())
                         writer.getSpoolWriter().writeln(qryLine);
                   }else if (reader.getReadMode() == OBEY_READ_MODE || ( caller !=PRUNI && reader.getReadMode() == SCRIPT_READ_MODE ))
                   {
                       boolean showPromptAgain = false;

                       if(reader.getReadMode() == OBEY_READ_MODE && reader.obeyMultipleFiles() && reader.getObeyReader()!=null && !sessObj.isDotModeCmd())
                       {
                           // test if we are reading a different obey file
                           if(!lastObeyFilename.equals(reader.getObeyReader().getFileName()))
                           {
                               lastObeyFilename = reader.getObeyReader().getFileName();
                               
                               if(sessObj.isLogCmdText() && writer.getWriterMode() == CONSOLE_SPOOL_WRITE_MODE)
                               {
                                   writer.getSpoolWriter().writeln();
                                   writer.getSpoolWriter().writeln(this.formatString("=",80,'='));
                                   writer.getSpoolWriter().writeln("Script : " + lastObeyFilename);
                                   writer.getSpoolWriter().writeln(this.formatString("=",80,'='));
                                   showPromptAgain = true;
                               }
                           }
                       }    
                         
                       if(sessObj.isLogCmdText() && writer.getWriterMode() == CONSOLE_SPOOL_WRITE_MODE && sessObj.isLogCmdEcho())
                           writer.getSpoolWriter().writeln(((showPromptAgain == true) ? sessObj.getSessionPrompt() : "") + qryLine);
                       
                       if (sessObj.isLogCmdEcho())
                    	   writer.getConsoleWriter().println(qryLine);
                   }
               }
            }
            else
            {
               // if the query line is null, then we have reached the end of line
               // if the current mode is non-interactive mode(reading from  an obey file - Note- reading from script
               // is treated as interactive)..close the obey file 
               // and move to the previous mode
                
               OS_EOF=true;
                             
               if (!sessObj.isSessionInteractive())  // reading from an obey file
               {
                   
                  // cleanup open files, close files listed in the obey hash map
                  try
                  {
                     reader.getObeyReader().close();
                     List<FileReader> obeyReaders = reader.getObeyReaderList();
                     if(obeyReaders != null){
                         for(int i=0;i<obeyReaders.size();i++)
                             (obeyReaders.get(i)).close();
                     }
                  }catch (IOException ioe)
                  {
                    System.out.println(ioe);
                  }finally
                  {
                     reader.setObeyReader(null);
                     if(OBEY_DEPTH_ERROR)
                     {
                         OBEY_DEPTH_ERROR = false;
                         OS_EOF = false;
                     }
                  }
                  sessObj.setSessionType(true); // change it back to the interactive mode
                  reader.setReadMode(reader.getPrevReadMode());
                  nestedObey = sessObj.getReader().getReadMode() == OBEY_READ_MODE ? true : false;
                  continue;
               }

               // if the session is interactive mode and the caller is PRUN, then we need to inform ParallelRun Thread to send us the next file
               // close the current script file and log file and return to the prun
               if (caller == PRUNI || caller == SessionDefaults.PYTHI)
               {
                  try
                  {
                     scriptReader.close();
                  }catch (IOException ioe)
                  {
                     System.out.println(ioe);
                  }finally
                  {
                     reader.setScriptReader((scriptReader =null));
                  }

                  if (writer.getLogWriter() != null)
                  {
                     try
                     {
                        writer.getLogWriter().write((this.formatString(SessionDefaults.lineSeperator+SessionDefaults.lineSeperator+"=",84,'=')));
                        writer.getLogWriter().writeln();
                        if (!showTimeInMillis)
                        writer.getLogWriter().write("Logging ended at "+DateFormat.getDateTimeInstance().format(new Date())); //Feb 13, 2007 11:32:31 AM 
                        else
                           writer.getLogWriter().write("Logging ended at "+  sdf.format(new Date(System.currentTimeMillis())) ); //Feb 13, 2007 11:32:31.999 AM
                        writer.getLogWriter().writeln();
                        writer.getLogWriter().write((this.formatString("=",80,'=')));
                        writer.getLogWriter().writeln();

                        writer.getLogWriter().close();
                     }catch (IOException ioe)
                     {
                     }finally
                     {
                        writer.setLogWriter((logWriter =null));
                     }
                  }
                  return sessObj.getExitCode();
               }

               // for USERI, if the current mode is script read mode then move to the interactive 
               if (caller == USERI)
               {
                  if (reader.getReadMode() == SCRIPT_READ_MODE)
                  {
                     try
                     {
                        scriptReader.close();
                     }catch (IOException ioe)
                     {
                        System.out.println(ioe);
                     }finally
                     {
                        reader.setScriptReader((scriptReader =null));
                     }
                     reader.setReadMode(CONSOLE_READ_MODE);
                     continue;
                  }
                  else
                  {
                     if (reader.getReadMode() == SessionDefaults.CONSOLE_READ_MODE)
                         return sessObj.getExitCode();
                  }

               }
            }

            // Now lets validate the query and route it to the correct object for execution
            // identify the type of query 
            try
            {
               vQryObj.validate(qryObj,sessObj.getSessionSQLTerminator());
            } catch (NullKeyWordException nke)
            {
               // if blank lines entered ..its fine..move on
               continue;
            }

            // at this point, we know the first key word..save the query object in the session object

            sessObj.setQuery(qryObj);
          
            //"if" statement added  201002-26
       	   //check whether we need add sql query count for prun.
       	   if(caller==PRUNI && !sessObj.isSessionStartup() && !sessObj.isDotModeCmd())
       	   {
       		   this.sqlQueryCount++;
       	   }
       	 
       	   //    check if the current session has database connection otherwise request user to connect to the db first

           if (!sessObj.isDBConnExists())
           {
              if (qryObj.getQueryType() != IQ && qryObj.getQueryType() != CQ)
              {
                  if(sessObj.getDisplayFormat() != SessionDefaults.XML_FORMAT)
                  	  writer.writeln();
                 //writer.writeln(SessionError.DB_CONN_NOT_EXIST);
                 writer.writeInterfaceErrors(sessObj,SessionError.DB_CONN_NOT_EXIST );
                 continue;
              }
              else if(qryObj.getQueryType()==IQ)
              {
              }
           }
           
            boolean printIgnoringCmdMsg = true;            
            
            //if the query type is conditional query type execute using conditional query object
            if(qryObj.getQueryType() == CQ){
            	if (sessObj.getCQryObj().getGotoLabel().equals("") )
                    printIgnoringCmdMsg = false;
                try{
                    cQryObj.execute();
                }catch(ConditionalQueryException ex){
                    
                    if(sessObj.getDisplayFormat() != SessionDefaults.XML_FORMAT)
                        writer.writeln();
                    
                    if(ex.getErrorMsg() != null)
                        writer.writeInterfaceErrors(sessObj,ex.getErrorMsg());
                    else
                        writer.writeConditionalSyntaxError(sessObj, cQryObj.getQueryString());
                  
                    //Added  2010-02-26
                	if(caller == PRUNI && sessObj.getQuery().getStatusCode() == -1)
                    {
                  	  this.sqlErrorCount++;
                    }
                    continue;
                }
                catch(UserInterruption ui){
                    userPressedCtrlC = true;
                    if (sessObj.isQueryInterrupted())
                    {
                       sessObj.setQueryInterrupted(false);
                       writer.writeln();
                       writer.writeInterfaceErrors(sessObj,SessionError.OPERATION_CANCELLED);
                    }
                    continue;
                }
                catch (IOException e)
                {
                   System.out.println("IO Exception occurred while processing conditional query :"+e);
                }

            }
            
            /* Conditional Query:
             *  if a GOTO <label> is set ignore all commands until
             *  the <label> is hit, which unsets the "getGotoLabel()" value
             */
            if(!sessObj.getCQryObj().getGotoLabel().equals(""))
            {
                if(qryObj.getQueryType() != CQ)
                {
                    /* still show continuation prompts for ignored queries, 
                     * useful for maintaining a correct command history
                     * */
                    try{
                        cQryObj.execute();
                        //For prun warning counting.
                        int iGotoErrorCode = Integer.parseInt(SessionError.GOTO_MESSAGE.errorCode());
	                    if (caller == PRUNI && sessObj.getLastError()==iGotoErrorCode)
	                    {
	                       this.sqlWarningsCount++;
	                    }
                    }catch(Exception ex){
                    	if(caller == PRUNI)
                        {
                      	  this.sqlErrorCount++;
                        }
                    }
                }
                String currLabel = sessObj.getCQryObj().getGotoLabel();
                if (printIgnoringCmdMsg  &&  !currLabel.equals("") )                 	
                	{
	                	ErrorObject msg_warning = new ErrorObject(SessionError.MESSAGE_WARNING.errorCode(), 
	                								SessionError.MESSAGE_WARNING.errorMessage() + currLabel + "' command is encountered.",SessionError.MESSAGE_WARNING.errorType);
	                	writer.write(SessionDefaults.lineSeperator);
	                	writer.writeError(sessObj, msg_warning);
	                    if (caller == PRUNI)
	                    {
	                       this.sqlWarningsCount++;
	                    }
	                    
                	}
                
                continue;
            }
           
            //if the query type is interface query type execute using interface query object
            if (qryObj.getQueryType() == IQ)
               try
               {

                  iQryObj.execute();
                  //"if" statement added  2009-12-07
                  //calculate error count for some exception(not throw out). 
                  if(caller == PRUNI && sessObj.getQuery().getStatusCode() == -1)
                  {
                	  //Sometimes if we get no data, the statuscode will be set to -1.
                	  //So here, we judge whether there is an error or not by last error code.
                	  if(sessObj.getLastError()!=0) 
                		  this.sqlErrorCount++;
                  }

                  continue;
               } catch (UnKnownInterfaceCommand e)
            {
            	   //if the command is not an interface command and db connection does not exist,
            	   //write error out and terminate the command;
	               if (!sessObj.isDBConnExists())
	               {
	                  if(sessObj.getDisplayFormat() != SessionDefaults.XML_FORMAT)
	                      	  writer.writeln();
	                 //writer.writeln(SessionError.SYNTAX_ERROR_PREFIX);
	                 writer.writeInterfaceErrors(sessObj,SessionError.UNKOWN_OPTION );
	                 continue;
	                  
	               }
	               
               // if the command is not an interface command set the query type to SQL/CQ command
               // depends on the current session mode
               if (qryObj.getQueryType() != PRUNQ)
               {
                  switch (sessObj.getMode())
                  {
                     case SQL_MODE:
                        qryObj.setQueryType(SQLQ);
                        break;
                  }

               }
            }catch (UserInterruption ui)
            {
               userPressedCtrlC = true;
               if (sessObj.isQueryInterrupted())
               {
                  userPressedCtrlC = true;
                  sessObj.setQueryInterrupted(false);
                  writer.writeln();
                  // writer.writeln(SessionError.OPERATION_CANCELLED);
                  writer.writeInterfaceErrors(sessObj,SessionError.OPERATION_CANCELLED);
               }
               //System.in.read();
               continue;
            }
            catch (SQLException e)
            {
               sessObj.getQuery().setStatusCode(Math.abs(e.getErrorCode()));
               //to handle unexpected server error message..connection does not exist occurs after socket error
               if (e.getErrorCode()==-29154 || e.getErrorCode()==-29002)
               {
            	   if(sessObj.isDBConnExists())
            		   sessObj.setSessionStatus(false);
               }
               if (e.getErrorCode() == -29157)
               {
                  sessObj.setDBConnExists(false);
               }

               if (sessObj.isQueryInterrupted() && !sessObj.isDBConnExists())
               {
                  writer.writeInterfaceErrors(sessObj,SessionError.DB_DISCONNECT_ON_USER_REQ);
                  continue;
               }
               if (caller == PRUNI)
               {
                  this.sqlErrorCount++;
               }
               writer.writeln();
               writer.writeAllSQLExceptions(sessObj,e);
               continue;
            }catch (IOException e)
            {
               System.out.println("IO Exception occurred while processing interface query :"+e);
            }

            //Prun not supported in CS mode
            if (qryObj.getQueryType() == PRUNQ && sessObj.getMode() != SessionDefaults.SQL_MODE)
            {
               //String errorStr = "ERROR: This command is not supported in "+sessObj.getStrMode()+" mode.";
               if(sessObj.getDisplayFormat() != SessionDefaults.XML_FORMAT)
             	  writer.writeln();
               writer.writeInterfaceErrors(sessObj, SessionError.CMD_NOT_MODE_SUPPORTED);
               continue;
            }

            //
            //if the query is of type parallel run
            //
            if (qryObj.getQueryType() == PRUNQ && caller != PRUNI)
            {
               sessObj.setTimerHold();
               ParallelRun prunObj=null;
               try
               {
                  prunObj=new ParallelRun(crObj,cwObj,lfProps,sessObj);
                  prunObj.execute();
               } catch (PrunUserInterruption e)
               {
                  // if the user stop the prun session..no worries..continue
                  continue;
               } catch (UserInterruption ui)
               {
                  userPressedCtrlC = true;
                  continue;
               } catch (IOException ioe)
               {
                  // if the file processing is screwed up..
                  System.out.println("IO Exception while processing prun: "+ioe);
                  continue;

               }
               catch (InvalidNumberOfArguments e)
               {
                  // writer.writeln(e.getMessage());
                  if(sessObj.getDisplayFormat() != SessionDefaults.XML_FORMAT)
                        writer.writeln();
                  writer.writeInterfaceErrors(sessObj,e.getMessage());

                  if(sessObj.getDisplayFormat() == SessionDefaults.RAW_FORMAT)
                       prunObj.printUsage();
                  continue;
               }
               catch (NumberFormatException  nfe)
               {
                  // Number Format Exception is caught in ParalleRun.java
                  // and rethrown. Error message is printed in ParallelRun.java.
                  // Just continue with the next command from here.
                  continue;
               }
               finally
               {
                  prunObj=null;
               }
               continue;
            }

            try
            {
            	boolean showStatusMsg = true;
               
               /*
                * Running EXPORT WMS command from the normal prompt 
                */
				int queryType = -1;
				String queryText = null;
				if(qryObj.getQueryText() != null)
					queryText = qryObj.getQueryText().toUpperCase().trim();
				if ((queryText != null && sessObj.qsOpen && SessionDefaults.WMS_MODE == sessObj .getMode()) || (queryText != null && queryText .startsWith("EXPORT WMS"))) {
					qryObj.setQueryType(NSQ);
					queryType = NSQ;
				} else if (queryText != null && (queryText.startsWith(SessionDefaults.WMSOPEN) || queryText .startsWith(SessionDefaults.WMSCLOSE))) {
					qryObj.setQueryType(NSQ);
					queryType = NSQ;
				} 
				else
					queryType = qryObj.getQueryType();
				
               switch (queryType)
               {
                  case SQLQ:
                     try
                     {
						 queryText = Utils.trimSQLTerminator(queryText,sessObj.getSessionSQLTerminator()).trim();
                    	 if(SessionDefaults.CMDOPEN.equalsIgnoreCase(queryText) && 
                    			 SessionDefaults.CS_MODE == sessObj.getMode()){
                    		 writer.writeln();
                    		 writer.writeError(sessObj, SessionError.CMD_ONLY_SQL_SUPPORTED);
                    		 showStatusMsg = false;
                    		 break;
                    	 }
                         else if(SessionDefaults.CMDCLOSE.equalsIgnoreCase(queryText) && 
                        		 SessionDefaults.SQL_MODE == sessObj.getMode()){
                        	 writer.writeln();
                    		 writer.writeError(sessObj, SessionError.CMD_ONLY_CS_SUPPORTED);
                    		 showStatusMsg = false;
                    		 break;
                         }//prevent cmdopen;; and cmdclose;; like cheat
							else if (queryText.toUpperCase().matches(
									SessionDefaults.CMDOPEN + ".+")
									|| queryText.toUpperCase().matches(
											SessionDefaults.CMDCLOSE + ".+")) {
							writer.writeln();
							writer.writeError(sessObj,
										SessionError.CMD_SYNTAX_ERROR);
							showStatusMsg = false;
							break;
						}
                        dbQryObj.execute();
                        if(SessionDefaults.CMDOPEN.equalsIgnoreCase(queryText))
                        	sessObj.setMode(SessionDefaults.CS_MODE);
                        else if(SessionDefaults.CMDCLOSE.equalsIgnoreCase(queryText))
                        	sessObj.setMode(SessionDefaults.SQL_MODE);
                        if (sessObj.isSessionStatsEnabled() &&
                           (false == dbQryObj.userExecutedGetStatistics()) &&
                           (sessObj.getDisplayFormat() == SessionDefaults.RAW_FORMAT && !dbQryObj.blockGetStats() ))
                           showStatusMsg = false;
                        
                        if (caller == PRUNI)
                        {
                           this.sqlWarningsCount= this.sqlWarningsCount + sessObj.getSqlWarningsCnt();
                        }
                     }catch (UnKnownInterfaceCommand e)
                     {
                        continue;
                     }
                     break;
                	  
                  default:
                     continue;
               }
               if ((qryObj.isTrimOut() && 
                   Utils.trimSQLTerminator(qryObj.getQueryText(), sessObj.getSessionSQLTerminator())
                   .matches("(?i)get\\s+all\\s+volatile\\s+schemas\\s*")) ||
                  (!qryObj.isTrimOut()) )
            	   sessObj.setTotalRecordCount(qryObj.getRowCount());
					if (!showStatusMsg) {
						String elapsedTimeMsg = writer.getElapsedTime(sessObj,
								qryObj, utilObj);
						writer.writeStatusMsg(sessObj, null, elapsedTimeMsg,
								writer);
					} else {
						writer.writeStatusMsg(sessObj, qryObj, utilObj, writer);
					}

               sessObj.setLastError(sessObj.getQuery().getStatusCode());
            }catch (UserInterruption ui)
            {
               userPressedCtrlC = true;
               if (sessObj.isQueryInterrupted())
               {
                  sessObj.setQueryInterrupted(false);
                  if(sessObj.getDisplayFormat() != SessionDefaults.XML_FORMAT)
                  	  writer.writeln();
                  writer.writeInterfaceErrors(sessObj,SessionError.OPERATION_CANCELLED);
               }

               continue;
            } catch (SQLException e) {
               if (e.getErrorCode() == -29157)
               {
                  sessObj.setDBConnExists(false);
               }

               if (sessObj.isQueryInterrupted() && !sessObj.isDBConnExists())
               {
                   if(sessObj.getDisplayFormat() != SessionDefaults.XML_FORMAT)
                   	  writer.writeln();
                   writer.writeInterfaceErrors(sessObj,SessionError.DB_DISCONNECT_ON_USER_REQ);
                   if (sessObj.isDotModeCmd())
                	   sessObj.setLogCmdEcho(true);
                  continue;
               }

               sessObj.getQuery().setStatusCode(Math.abs(e.getErrorCode()));
               //  to handle unexpected server error message..connection does not exist occurs after socket error
               if (e.getErrorCode()==-29154 || e.getErrorCode()==-29002)
               {
                  sessObj.setSessionStatus(false);
               }
               // if QSMGR is stopped. 
               //  if (e.getErrorCode()==WMS_PROCESS_DOES_NOT_EXIST)
               if (e.getMessage().matches("Unexpected.*(error message = error=201,.*)"))
               {
                  if (sessObj.qsOpen)
                  {
                     try
                     {
                        sessObj.getStmtObj().execute(SessionDefaults.WMSCLOSE);
                     } catch (SQLException sqle)
                     {
                        System.out.println("Should not come here...");
                     }
                     sessObj.qsOpen = false;
                  }
               }
               if (caller == PRUNI)
               {
                  this.sqlErrorCount++;
               }
               qryObj.setTrimOut(false);


               sessObj.setTotalRecordCount("-1");
               
               writer.writeAllSQLExceptions(sessObj,e);
              
              
               if (sessObj.getMode() != SessionDefaults.WMS_MODE)
                  sessObj.setLastError(Math.abs(e.getErrorCode()));
               else
                  sessObj.setLastError(e.getErrorCode());
               
               sessObj.setQryEndTime();
            } catch (IOException ioe)
            {
               System.out.println("IO Exception while processing "+sessObj.getStrMode()+" command: "+ioe);
               sessObj.setQryEndTime();
               continue;

            }

            continue;



         } catch (IOException e)
         {
            throw e;
         }
      }

      return sessObj.getExitCode();
   }

   // get the connection to the database
   public Connection getConnection() throws SQLException, InstantiationException, IllegalAccessException, ClassNotFoundException
   {

      Driver driver=null;
      Connection conn=null;
      Properties connProp=new Properties();
     
      String connectStr="jdbc:t4jdbc://"+sessObj.getSessionServer()+sessObj.getSessionPort();

      driver = (Driver)Class.forName(SessionDefaults.DRIVER_NAME).newInstance();

      driver.getMajorVersion();
      driver.getMinorVersion();

      connProp.put("useLobHandle", String.valueOf(true));
      connProp.put("user",sessObj.getSessionUser());
      connProp.put("password",sessObj.getSessionPass());

      /*
       *  Only set the connection timeout if its not the default.
       */
      int connTimeout = Session.getConnectionTimeout();
      if (Session.DEFAULT_CONNECTION_TIMEOUT != connTimeout)
         connProp.put("connectionTimeout", String.valueOf(connTimeout) );

      //connProp.put("serverDataSource",sessObj.getSessionDsn());
      //connProp.put("catalog",sessObj.getSessionCtlg());
      //connProp.put("schema", sessObj.getSessionSchema());
      String theRole = sessObj.getTempSessionRole();
 
      if ((null == theRole)  ||  (0 <= theRole.length() ) )
    	  theRole = sessObj.getSessionRole();
      
      connProp.put("roleName", theRole);
      connProp.put("applicationName", SessionDefaults.APP_NAME);
      
      if (sessObj.isDebugOn())
      {
    	  System.out.println("getConnection::" +
    			  " HOST:" + sessObj.getSessionServer() +
    			  "::PORT:" + sessObj.getSessionPort() + 
    			  "::USR:" + sessObj.getSessionUser() +
     			  "::ROLE:"+ theRole + 
    			  "::DSN:" + defaultDataSource);
    	  System.out.println("Driver version:: "+ sessObj.getT4verNum());
      }
      
      conn = DriverManager.getConnection(connectStr,connProp);
        
      SQLWarning dsw=conn.getWarnings();
      sessObj.setDBConnExists(true);

      /*while ((dsw!=null))
      {
	      if (dsw.getErrorCode()==UNKNOWN_DATA_SOURCE)
	      {
	         defaultDataSource=(dsw.toString().lastIndexOf(": ")!= -1 ?dsw.toString().substring(dsw.toString().lastIndexOf(": ")+1).trim():defaultDataSource);
	
	         if (this.caller != PRUNI)
	         {
	            //System.out.println(SessionDefaults.lineSeperator+"WARNING: Data Source, "+sessObj.getSessionDsn()+", does not exist.");
	         }
	
	         sessObj.setSessionDsn(defaultDataSource);
	
	      } else
	      {
	   		 System.out.println(SessionDefaults.lineSeperator + dsw.getMessage());
	      }
	      dsw=dsw.getNextWarning();
	  } */
      setDbProdVersion(conn);
      setISOMapping(conn);
      
      specialModeEnabled = ((TrafT4Connection)conn).getDateConversion();
      String schName = ((TrafT4Connection)conn).getSchema();
      if (schName != null)
    	  sessObj.setSessionSchema(schName);
      
      if (sessObj.isDebugOn())
    	  System.out.println("schName:" + schName);
      
      // clear off any warnings generated
      conn.clearWarnings();

	  try {
		  remoteProcess=((TrafT4Connection)conn).getRemoteProcess();
	  	  sessObj.setNeoProcessName(remoteProcess);
	  	  if (sessObj.isDebugOn())
	  	  {
	  		  System.out.println("Connected to process: " + remoteProcess);
	  	  }
	  } catch (Exception e) {}
      return conn;
   }

   //  get the connection to the database
   public Connection getConnection(String user, String password, String role, String server, String dataSource, String port) 
      throws SQLException, InstantiationException, IllegalAccessException, ClassNotFoundException
   {

      Driver driver=null;
      Connection conn=null;
      Properties connProp=new Properties();
      

      String connectStr="jdbc:t4jdbc://"+(server==null?sessObj.getSessionServer():server)+(port==null?sessObj.getSessionPort():port);

      driver = (Driver)Class.forName(SessionDefaults.DRIVER_NAME).newInstance();

      if (sessObj.isDebugOn())
      {
    	  System.out.println("getConnection with args::" + " HOST:" + server +
    			"::PORT:" + port + "::USR:" + user +
    			"::ROLE:"+ role + "::DSN:" + dataSource); 
      }
      
      driver.getMajorVersion();
      driver.getMinorVersion();

      connProp.put("useLobHandle", String.valueOf(true));
      connProp.put("user", (user==null?sessObj.getSessionUser():user) );
      connProp.put("password",(password==null?sessObj.getSessionPass():password));
      connProp.put("roleName", (null==role?"":role));

      /*
       *  Only set the connection timeout if its not the default.
       */
      int connTimeout = Session.getConnectionTimeout();
      if (Session.DEFAULT_CONNECTION_TIMEOUT != connTimeout)
         connProp.put("connectionTimeout", String.valueOf(connTimeout) );

      //connProp.put("serverDataSource",dataSource==null?sessObj.getSessionDsn():dataSource);
      //connProp.put("catalog",sessObj.getSessionCtlg());
      //connProp.put("schema", sessObj.getSessionSchema());
      connProp.put("applicationName", "TrafCI");
      conn = DriverManager.getConnection(connectStr,connProp);
      
      SQLWarning dsw=conn.getWarnings();
      sessObj.setDBConnExists(true);
      boolean wasSessionDsnSet=false;
      
      /*while ((dsw!=null))
      {      
     	  if (dsw.getErrorCode()==UNKNOWN_DATA_SOURCE)
    	  {
    		  defaultDataSource=(dsw.toString().lastIndexOf(": ")!= -1 ?dsw.toString().substring(dsw.toString().lastIndexOf(": ")+1).trim():defaultDataSource);

    		  if (this.caller != PRUNI)
    		  {
    			  //System.out.println(SessionDefaults.lineSeperator+"WARNING: Data Source, "+ dataSource +", does not exist.");
    		  }
    		  sessObj.setSessionDsn(defaultDataSource); 
    		  wasSessionDsnSet=true;
    	  } else 
    	  {
    		  System.out.println(SessionDefaults.lineSeperator + dsw.getMessage());
    	  }
       	  dsw=dsw.getNextWarning();
      } */
      
	  if (!wasSessionDsnSet) {
		  wasSessionDsnSet=true;
		  sessObj.setSessionDsn(dataSource);
	  }
      // clear off any warnings generated
      conn.clearWarnings();

      try
      {
         lfPropsObj.loadLookAndFeelProps(sessObj,specialModeEnabled);
         sessObj.setLFProps(lfPropsObj);
         
       } catch (FileNotFoundException fnfe)
      {
      }
      catch (IOException ioe)
      {
      }
      catch (Exception e)
      {
      }
      try {
    	  remoteProcess=((TrafT4Connection)conn).getRemoteProcess();
    	  sessObj.setNeoProcessName(remoteProcess);
    	  if (sessObj.isDebugOn())
    	  {
    		  System.out.println("Connected to process: " + remoteProcess);
    	  }     
      } catch (Exception e) {}
      
      setDbProdVersion(conn);
      setISOMapping(conn);
      specialModeEnabled = ((TrafT4Connection)conn).getDateConversion();
	  String schName = ((TrafT4Connection)conn).getSchema();
 
	  if (sessObj.isDebugOn())
    	  System.out.println("schName:" + schName);

      if (schName != null)
    	  sessObj.setSessionSchema(schName);
      return conn;

   }
   
   public void setISOMapping(Connection conn) throws SQLException
   {
      
      try{
          sessObj.setISOMapping(((TrafT4Connection)conn).getISOMapping());
      }catch(NoSuchMethodError nsme){
         ;
      }
      String multibyteAlign = System.getProperty("trafci.charset.align");
      if (multibyteAlign != null && multibyteAlign.equalsIgnoreCase("N"))
         sessObj.setMultiByteAlign(false);
      

   }
   
   public void getDriverVersion() throws IOException
   {
      float verNum = 2.2F;
      if (sessObj.getConnObj() != null)
      {
         try
         {
            if (parser == null)
               parser = new Parser();
            if (utilObj == null) 
               utilObj = new Utils();
            String versionStr = parser.gett4Version(sessObj.getConnObj().getMetaData().getDriverVersion());
            verNum = Float.parseFloat(versionStr);
         }
         catch (Exception e) {}
         finally {
            sessObj.setT4verNum(verNum);
            try {
               if (sessObj.getPlatformObjectVersions())
               {
            	   sessObj.setMxosrvrVersion(sessObj.getNdcsVersion());
               }
            } catch( Exception e) {}
         }
      }
   }

   public void setDbProdVersion(Connection conn) throws SQLException
   {
      String prodVer = conn.getMetaData().getDatabaseProductVersion();
      if (prodVer != null)
         sessObj.setDatabaseProdVersion(Float.parseFloat(prodVer));
   }
   
   private String formatString(String output,int colWidth,char fillchar)
   {
      StringBuffer outBuffer=null;

      if (output == null)
      {
         return null;
      }

      outBuffer=new StringBuffer(output);


      if (outBuffer.length() <= colWidth)
      {
         for (int i=outBuffer.length();i<colWidth;i++)
            outBuffer.append(fillchar);

      }

      return outBuffer.toString();

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

   public String getScriptFile()
   {
      return scriptFile;
   }

   public void setScriptFile(String scriptFile)
   {
      this.scriptFile = scriptFile;
   }

   public String getLogFile()
   {
      return logFileName;
   }

   public void setLogFile(String logFileName,boolean overWriteLog)
   {
      this.logFileName = logFileName;
      this.overWriteLog=overWriteLog;

   }

   public int getSqlErrorCount()
   {
      return sqlErrorCount;
   }

   public void setSqlErrorCount(int sqlErrorCount)
   {
      this.sqlErrorCount = sqlErrorCount;
   }

   public int getSqlQueryCount()
   {
      return sqlQueryCount;
   }

   public void setSqlQueryCount(int sqlQueryCount)
   {
      this.sqlQueryCount = sqlQueryCount;
   }

   public boolean isQueryMode()
   {
      return queryMode;
   }

   public void setQueryOptions(boolean queryMode, String qryString)
   {
      this.queryMode = queryMode;
      this.qryString=qryString;
   }

   public int getSqlWarningsCount()
   {
      return sqlWarningsCount;
   }


   public void setSqlWarningsCount(int sqlWarningsCount)
   {
      this.sqlWarningsCount = sqlWarningsCount;
   }
  

}

