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

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FilenameFilter;
import java.io.IOException;
import java.sql.SQLException;
import java.sql.Statement;
import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import java.util.Properties;
import java.sql.ResultSet;
import java.util.Vector;
import sun.misc.Signal;
import sun.misc.SignalHandler;

public class ParallelRun extends Thread
{

   String scriptsDir=null;
   String scriptsExt=null;
   String logsDir=null;
   boolean overwriteLogs=false;
   int connectionCnt=2;
   ConsoleReader crObj=null;
   ConsoleWriter cwObj=null;
   String summaryFile=null;
   PrunUserInterruption pui=null;
   Properties lfProps=null;
   final String SCRIPT_EXT="sql";
   int MAX_THREADS=64;
   int MIN_THREADS=2;
   List<String> scriptsList=null;
   Session sessObj=null;
   FileWriter summaryWriter=null;
   int threadNumber=0;
   int totalScriptFiles=0;
   int totalScriptFilesProcessed=0;
   int totalSQLsProcessed=0;
   int totalSQLErrors=0;
   int totalSQLWarnings=0;
   int totalConnections=0;
   int seqNo=0;
   Utils utils=null;
   SignalHandler CTRLCHandler=null;
   Signal INTSignal=null;
   boolean isUserInterrupt=false;
   ThreadGroup prunGroup=null;
   List<Session> activeSessionLists=null;
   StatusThread status=null;
   int noOfArgs=0;
   String[] args=null;
   HTMLObject htmlObj = null;
   XMLObject xmlObj = null;
   boolean errorMsg = false;
   Writer writeObj = null;
   PrunSummary summaryObj = new PrunSummary();
   Query qryObj=null;

   final String TIMER_DEFAULT="60";
   int timerValue = 60;
   long endTime = 0;
   long nowTime = 0;
   boolean timerMode = false;
   List<String> scriptsListBak=null;
   int connDelta = 0;
   
   //Added for debugging prun
   boolean prunDebug=false; 


   ParallelRun()
   {

   }

   ParallelRun(ConsoleReader crObj, ConsoleWriter cwObj,Properties lfProps,Session sessObj) throws IOException
   {
      threadNumber=0;
      this.lfProps=lfProps;
      this.sessObj=sessObj;
      this.htmlObj = sessObj.getHtmlObj();
      this.xmlObj = sessObj.getXmlObj();
      this.crObj=crObj;
      this.cwObj=cwObj;
      this.summaryWriter=new FileWriter();
      pui=new PrunUserInterruption();
      utils=new Utils();
      writeObj = sessObj.getWriter();
      qryObj = sessObj.getQuery();
      CTRLCHandler =new SignalHandler ()
      {
         public void handle(Signal sig)
         {
            isUserInterrupt=true;
            if (prunGroup  != null)
            {
               status.beforeStatus="Cancelling";
               status.afterStatus="Cancelled";
            }
            while (activeSessionLists !=null && activeSessionLists.size() > 0)
            {

               ((Session)(activeSessionLists.get(0))).setQueryInterrupted(true);
               ((Session)(activeSessionLists.get(0))).setDBConnExists(false);
               if (((Session)(activeSessionLists.get(0))).getCurrentStmtObj() != null)
               {
                  try
                  {
                     ((Statement)((Session)(activeSessionLists.get(0))).getCurrentStmtObj()).cancel();
                  } catch (Exception e)
                  {

                  }
               }
               activeSessionLists.remove(0);
            }
         }
      };
      try {
      INTSignal=new Signal("INT");
      } catch (Exception e) {}

      // get the maxservers allowed for the current datasource
      // undocumented arg to disable calling getMaxServers(), -Dtrafci.prun.maxconn=n
      int maxThreads=0;
      String maxConn=System.getProperty("trafci.prun.maxconn");

      try
      {
         if ((maxConn!=null) && ((maxThreads = Integer.parseInt(maxConn.trim() )) > MAX_THREADS ))
            MAX_THREADS=maxThreads;
         else
            MAX_THREADS=getMaxServers();
      } catch (NumberFormatException nfe)
      {
       	this.writePrunErrors(SessionError.INVALID_MAXCONN);
         throw nfe;
      }
      MIN_THREADS = MAX_THREADS == 1 ? MAX_THREADS:MIN_THREADS;

      String strTimerMode = System.getProperty("trafci.prun.timermode");
      if ((strTimerMode!=null) &&  strTimerMode.equalsIgnoreCase("y"))
      {
         timerMode=true;
         String strConnDelta = System.getProperty("trafci.prun.connection.delta");
         if (strConnDelta != null)
            connDelta = Integer.parseInt(strConnDelta);
      }

   }

   public void execute() throws IOException, PrunUserInterruption, InvalidNumberOfArguments, UserInterruption
   {
      //Signal.handle(INTSignal, CTRLCHandler);
      String queryStr = sessObj.getQuery().getQueryText();
      queryStr = queryStr.replaceAll("\\s+"," ").trim();
      boolean okay = validateArgs(queryStr);
      if (!okay)
      {
         throw new PrunUserInterruption();
      }

      if (noOfArgs == 1)
         getInputs();
      try {
      Signal.handle(INTSignal, CTRLCHandler);
      }catch (Exception e) {}
      
      scriptsList=null;

      scriptsList=new ArrayList<String>();

      activeSessionLists=new ArrayList<Session>();

      FilenameFilter filter = new FilenameFilter()
      {
         public boolean accept(File dir, String name)
         {
            if (System.getProperty("os.name").toUpperCase()
                  .startsWith("WINDOW"))
            {
               return name.matches("(?i).*\\." + scriptsExt +"$" );
            }
            else
            {
               return name.endsWith("."+scriptsExt);
            }
         }
         
         
       };

      File dir=new File(scriptsDir);
      File[] fl=dir.listFiles(filter);
      //String separator=File.separator;
      for (int i=0;i< fl.length; i++)
      {
         scriptsList.add(fl[i].getName().toString());
      }

      prunDebug = Boolean.getBoolean("trafci.prun.debug");
      if (scriptsList.size() == 0)
      {
         if (sessObj.getDisplayFormat() == SessionDefaults.HTML_FORMAT)
         {
            htmlObj.init();
            cwObj.println(htmlObj._beginTableTag);
            cwObj.println(htmlObj._startCommentTag);
            cwObj.println("No files present with this extension.");
            cwObj.println(htmlObj._endCommentTag);
            cwObj.println(htmlObj._endTableTag);
         }
         else if (sessObj.getDisplayFormat() == SessionDefaults.XML_FORMAT)
         {
            xmlObj.init();
            xmlObj.handleStartTags();
            cwObj.println(xmlObj._beginStatusTag);
            cwObj.println(xmlObj._beginCdataTag + "No files present with this extension." + xmlObj._endCdataTag);
            cwObj.println(xmlObj._endStatusTag);
            xmlObj.handleEndTags();
         }
         else
         {
            cwObj.println();
            cwObj.println("No files present with this extension.");
         }
         return;
      }

      scriptsListBak=new ArrayList<String>(scriptsList);

      totalScriptFiles=this.scriptsList.size();

      try
      {
         String summaryDir=logsDir+File.separator+ "error" ;
         if (!isValid(summaryDir,"dir","write"))
         {
            if (new File(summaryDir).mkdir())
            {
               if (!isValid(summaryDir,"dir","write"))
               {
                  summaryDir=logsDir;
               }
            }
            else
            {
               summaryDir=logsDir;
            }
         }
         summaryFile=summaryDir+File.separator+"prun.err.log";
         String summaryTitle = "PRUN started at "+DateFormat.getDateTimeInstance().format(new Date());
         if (!this.overwriteLogs)
            summaryWriter.setAppend(true);
         summaryWriter.initialize(summaryFile);
         if (sessObj.getDisplayFormat() == SessionDefaults.HTML_FORMAT)
         {
            summaryWriter.writeln(htmlObj._beginTableTag);
            summaryWriter.writeln(htmlObj._startCommentTag + summaryTitle + htmlObj._endCommentTag);
            summaryWriter.writeln(htmlObj._beginRowTag);
            summaryWriter.writeln(htmlObj._beginTblHeadTag +"Scripts directory " + htmlObj._endTblHeadTag);
            summaryWriter.writeln(htmlObj._beginTblDataTag + this.scriptsDir + htmlObj._endTblDataTag);
            summaryWriter.writeln(htmlObj._endRowTag);
            summaryWriter.writeln(htmlObj._beginRowTag);
            summaryWriter.writeln(htmlObj._beginTblHeadTag +"Logs directory " + htmlObj._endTblHeadTag);
            summaryWriter.writeln(htmlObj._beginTblDataTag + this.logsDir + htmlObj._endTblDataTag);
            summaryWriter.writeln(htmlObj._endRowTag);
            summaryWriter.writeln(htmlObj._beginRowTag);
            summaryWriter.writeln(htmlObj._beginTblHeadTag +"Logs overwritten " + htmlObj._endTblHeadTag);
            summaryWriter.writeln(htmlObj._beginTblDataTag + (this.overwriteLogs ? "y":"n") + htmlObj._endTblDataTag);
            summaryWriter.writeln(htmlObj._endRowTag);
            summaryWriter.writeln(htmlObj._beginRowTag);
            summaryWriter.writeln(htmlObj._beginTblHeadTag +"Number of connections " + htmlObj._endTblHeadTag);
            summaryWriter.writeln(htmlObj._beginTblDataTag + this.connectionCnt + htmlObj._endTblDataTag);
            summaryWriter.writeln(htmlObj._endRowTag);
            if (timerMode)
            {
               summaryWriter.writeln(htmlObj._beginRowTag);
               summaryWriter.writeln(htmlObj._beginTblHeadTag +"Time to run (mins) " + htmlObj._endTblHeadTag);
               summaryWriter.writeln(htmlObj._beginTblDataTag + this.timerValue + htmlObj._endTblDataTag);
               summaryWriter.writeln(htmlObj._endRowTag);
            }
         }
         else if (sessObj.getDisplayFormat() == SessionDefaults.XML_FORMAT)
         {
            summaryWriter.writeln(xmlObj._xmlNameSpaceTag);
            summaryWriter.writeln(xmlObj._beginRootTag);
            summaryWriter.writeln(xmlObj._beginCdataTag + summaryTitle + xmlObj._endCdataTag);
            summaryWriter.writeln(xmlObj._beginScriptsDirTag + this.scriptsDir + xmlObj._endStriptsDirTag);
            summaryWriter.writeln(xmlObj._beginLogsDirTag + this.logsDir + xmlObj._endLogsDirTag);
            summaryWriter.writeln(xmlObj._beginLogsO + (this.overwriteLogs ? "y":"n") + xmlObj._endLogsO);
            summaryWriter.writeln(xmlObj._beginConnTag + this.connectionCnt + xmlObj._endConnTag);
            if (timerMode)
               summaryWriter.writeln(xmlObj._beginMinTag + this.timerValue + xmlObj._endMinTag);

         }
         else
         {
            summaryWriter.write((utils.formatString("=",80,'=')));
            summaryWriter.writeln();
            summaryWriter.write(summaryTitle);
            summaryWriter.writeln();
            summaryWriter.write((utils.formatString("=",80,'=')));
            summaryWriter.writeln();
            summaryWriter.writeln();
            summaryWriter.writeln("Scripts directory: "+this.scriptsDir);
            summaryWriter.writeln("Logs directory: "+this.logsDir);
            summaryWriter.writeln("Logs overwritten: "+ (this.overwriteLogs ? "y":"n") );
            summaryWriter.writeln("Number of connections: "+this.connectionCnt);
            if (timerMode)
            {
               summaryWriter.writeln("Time to run (mins): "+this.timerValue);
               summaryWriter.writeln("Connection delay (secs): "+this.connDelta);
            }
            if (prunDebug)
            {
               summaryWriter.writeln();
            }

         }
      }
      catch (IOException ioe)
      {
         cwObj.println("Could not create the summary file.");
         return;
      }


      prunGroup=new ThreadGroup("prgroup");
      Thread prunThread=null;

      status = new StatusThread(cwObj, sessObj);

      // start the status thread
      if (cwObj.getConsoleOut())
         status.start();

      if (this.connectionCnt > totalScriptFiles)
      {
         this.connectionCnt=totalScriptFiles;
      }

      sessObj.setQryStartTime();
      for (int i=1; i <= this.connectionCnt ; i++)
      {
         if (this.scriptsList.size()== 0 || this.isUserInterrupt)
            break;
         prunThread =new Thread(prunGroup,this);
         prunThread.setName("Thread"+i);
         prunThread.start();

         try
         {
            Thread.sleep(connDelta * 1000);

         } catch (InterruptedException ie)
         {
         }

      }
      //this.start();

      //Lets wait for all child threads to complete before proceeding further
      Thread[] activeLists= new Thread[prunGroup.activeCount()];
      int activeCounts=prunGroup.enumerate(activeLists);
      for (int t=0;t < activeCounts; t++)
      {
         try
         {
            activeLists[t].join();
         } catch (InterruptedException e)
         {

         }
      }

      // this loop is not required but need
      while (prunGroup.activeCount() > 0)
      {
         status.stop=false;
      }


      status.stop=true;

      if (cwObj.getConsoleOut())
      {
         try
         {
            status.join();
         } catch (InterruptedException e)
         {
            // TODO Auto-generated catch block
            //e.printStackTrace();
         }
         while (status.isAlive())
         {

         }
      }

      summaryObj.setTotalScriptFiles(this.totalScriptFiles);
      summaryObj.setTotalScriptFilesProcessed(this.totalScriptFilesProcessed);
      summaryObj.setTotalSQLsProcessed(this.totalSQLsProcessed);
      summaryObj.setTotalSQLErrors(this.totalSQLErrors);
      summaryObj.setTotalSQLWarnings(this.totalSQLWarnings);
      summaryObj.setTotalConnections(this.totalConnections);
      summaryObj.setTotalSQLSuccess(this.totalSQLsProcessed - this.totalSQLErrors);
      summaryObj.setTotalConnectionFailures(this.connectionCnt - this.totalConnections);

      sessObj.setQryEndTime();
      String elapsedTime = writeObj.getElapsedTime(sessObj,qryObj,utils);
      if (sessObj.getDisplayFormat() == SessionDefaults.XML_FORMAT)
      {
         sessObj.getXmlObj().handlePrunSummary(cwObj,summaryObj, elapsedTime);
      }
      else if (sessObj.getDisplayFormat() == SessionDefaults.HTML_FORMAT)
      {
         sessObj.getHtmlObj().handlePrunSummary(cwObj,summaryObj, elapsedTime);
      }
      else
      {
         cwObj.println();
         cwObj.println("\t"+utils.formatString("_",45,'_'));
         cwObj.println("\t"+utils.formatString("          PARALLELRUN(PRUN) SUMMARY",45,' '));
         cwObj.println("\t"+utils.formatString("_",45,'_'));
         cwObj.println("\t"+utils.formatString("Total files present ",45,'.',""+this.totalScriptFiles));
         cwObj.println("\t"+utils.formatString("Total files processed ",45,'.',""+(this.totalScriptFilesProcessed)));
         cwObj.println("\t"+utils.formatString("Total queries processed ",45,'.',""+this.totalSQLsProcessed));
         cwObj.println("\t"+utils.formatString("Total errors ",45,'.',""+this.totalSQLErrors));
         cwObj.println("\t"+utils.formatString("Total warnings ",45,'.',""+this.totalSQLWarnings));
         cwObj.println("\t"+utils.formatString("Total successes ",45,'.',""+(this.totalSQLsProcessed-this.totalSQLErrors)));
         cwObj.println("\t"+utils.formatString("Total connections ",45,'.',""+ (this.totalConnections) ));
         cwObj.println("\t"+utils.formatString("Total connection failures ",45,'.',""+ (connectionCnt-this.totalConnections) ));
         cwObj.println(SessionDefaults.lineSeperator + writeObj.getElapsedTime(sessObj,qryObj,utils));
         cwObj.println();
      }
      if (this.totalSQLErrors > 0)
      {
         String errorLogInfo = "Please verify the error log file " + summaryFile + " for error summary.";
         cwObj.println(formatSummaryStr(errorLogInfo));
      } 
      endPrunConsoleTags();
      
      if ((this.totalScriptFiles - this.scriptsList.size()) == 0)
      {
         String errorMsgStr = "";
         if (errorMsg) 
            errorMsgStr = "No script files have been processed. See preceeding error message(s).";
         else
            errorMsgStr = "No script files have been processed successfully.";

         summaryWriter.writeln(formatSummaryStr(errorMsgStr));
      }
      else if ((this.totalScriptFiles - this.scriptsList.size()) < this.totalScriptFiles)
      {
         String scriptFileMsg ="";
         if (errorMsg) 
            scriptFileMsg = "Not all script files have been processed successfully. See preceeding error message(s).";
         else
            scriptFileMsg = "Not all script files have been processed successfully.";
         
         summaryWriter.writeln(formatSummaryStr(scriptFileMsg));
      }
      else
      {
         String finalMsg = "";
         if (errorMsg)
         {
            finalMsg = "All the script files have been processed successfully. See preceeding error message(s).";
         }
         else if (this.totalSQLErrors == 0)
         {
            finalMsg = "All the script files have been processed successfully.";
         }

         summaryWriter.writeln(formatSummaryStr( finalMsg));
      }

      String summaryEnd = "PRUN completed at " + DateFormat.getDateTimeInstance().format(new Date());


      if (sessObj.getDisplayFormat() == SessionDefaults.XML_FORMAT)
      {
         sessObj.getXmlObj().handlePrunSummary(summaryWriter, summaryObj, summaryEnd, elapsedTime);
      }
      else if (sessObj.getDisplayFormat() == SessionDefaults.HTML_FORMAT)
      {
         sessObj.getHtmlObj().handlePrunSummary(summaryWriter,summaryObj, summaryEnd, elapsedTime);
      }
      else
      {
         summaryWriter.writeln();
         summaryWriter.writeln("\t"+utils.formatString("_",45,'_'));
         summaryWriter.writeln("\t"+utils.formatString("          PARALLELRUN(PRUN) SUMMARY",45,' '));
         summaryWriter.writeln("\t"+utils.formatString("_",45,'_'));
         summaryWriter.writeln("\t"+utils.formatString("Total files present ",45,'.',""+this.totalScriptFiles));
         summaryWriter.writeln("\t"+utils.formatString("Total files processed ",45,'.',""+(this.totalScriptFilesProcessed)));
         summaryWriter.writeln("\t"+utils.formatString("Total queries processed ",45,'.',""+this.totalSQLsProcessed));
         summaryWriter.writeln("\t"+utils.formatString("Total errors ",45,'.',""+this.totalSQLErrors));
         summaryWriter.writeln("\t"+utils.formatString("Total warnings ",45,'.',""+this.totalSQLWarnings));
         summaryWriter.writeln("\t"+utils.formatString("Total successes ",45,'.',""+(this.totalSQLsProcessed-this.totalSQLErrors)));
         summaryWriter.writeln("\t"+utils.formatString("Total connections ",45,'.',""+ (this.totalConnections) ));
         summaryWriter.writeln("\t"+utils.formatString("Total connection failures ",45,'.',""+ (connectionCnt-this.totalConnections) ));
         summaryWriter.writeln();
         summaryWriter.write((utils.formatString("=",80,'=')));
         summaryWriter.writeln();
         summaryWriter.write(summaryEnd);
         summaryWriter.write(" " + elapsedTime);
         summaryWriter.writeln();
         summaryWriter.write((utils.formatString("=",80,'=')));
         summaryWriter.writeln();
      }

      this.summaryWriter.close();
      errorMsg = false;
   }

   private void getInputs() throws IOException, PrunUserInterruption, UserInterruption
   {
      cwObj.print(utils.formatString("Enter * as input to stop the current prun session",50,' '));
      cwObj.println();
      cwObj.println(utils.formatString("-",50,'-'));
      cwObj.println();

      while (true)
      {
         this.scriptsDir=getInput("Enter the scripts directory               :",null);

         if (isValid(scriptsDir,"dir","read"))
         {
            scriptsDir = this.getCanonicalPath(scriptsDir);
            break;
         }
         cwObj.println(SessionError.DIR_NOT_FOUND.errorMessage());
         cwObj.println();

      }

      this.scriptsExt=getInput("Enter the script file extension["+SCRIPT_EXT+"]      :",SCRIPT_EXT);

      while (true)
      {
         this.logsDir=getInput("Enter the logs directory[scripts dir]     :",this.scriptsDir);

         if (isValid(logsDir,"dir","write"))
         {
            logsDir = this.getCanonicalPath(logsDir);
            break;
         }
         cwObj.println(SessionError.DIR_NOT_FOUND.errorMessage());
         cwObj.println();

      }

      while (true)
      {
         String overWriteVal=getInput("Overwrite the log files (y/n)[n]?         :","n");
         if (overWriteVal.equalsIgnoreCase("y"))
         {
            this.overwriteLogs=true;
            break;
         }
         else if (overWriteVal.equalsIgnoreCase("n"))
         {
            this.overwriteLogs=false;
            break;
         }
         else
         {
            cwObj.println(SessionError.INCORRECT_OVERWRITE_OPTION.errorMessage());
            cwObj.println();
         }

      }

      while (true)
      {
         String connValue=getInput("Enter the number of connections("+ MIN_THREADS + "-" + utils.formatString(MAX_THREADS+")["+MIN_THREADS+"]",8,' ')+":",""+MIN_THREADS);
         try
         {
            this.connectionCnt=Integer.parseInt(connValue);
            if (this.connectionCnt >= MIN_THREADS && this.connectionCnt <= MAX_THREADS)
            {
               break;
            }
            cwObj.println(SessionError.PRUN_CONN_CNT_ERR.errorMessage());
            cwObj.println();

         }catch (NumberFormatException nfe)
         {
            cwObj.println("Invalid value specified for connections."+SessionDefaults.lineSeperator);
         }

      }

      if (timerMode)
      {
         while (true)
         {
            String strTimerValue=getInput("Enter the time to run in minutes (0=single interation) ["+TIMER_DEFAULT+"]:",TIMER_DEFAULT);   //0=run to completion
            try
            {

               timerValue= Integer.parseInt(strTimerValue);

               if (timerValue >= 0)
               {
                  if (timerValue > 0)
                  {
                     this.endTime = (timerValue*1000*60) + System.currentTimeMillis();
                     timerMode=true;
                  }
                  else
                     timerMode=false;

                  break;
               }

               cwObj.println("Invalid timer value");
               cwObj.println();

            }
            catch (NumberFormatException nfe)
            {

            }

         } //end while
      }


   }

   private boolean isValid(String input,String type, String permissions)
   {
      if (input != null)
      {
         File file=new File(input);

         if (!file.exists())
         {
            return false;
         }
         if ("dir".equals(type) && !file.isDirectory())
         {
            return false;
         }
         else if ("file".equals(type) && file.isDirectory())
         {
            return false;
         }

         if (("read".equals(permissions)) && !file.canRead())
         {
            return false;
         }
         if (("write".equals(permissions)) && !file.canWrite())
         {
            return false;
         }
         return true;

      }
      return false;
   }

   private String getInput(String requestString,String defaultVal) throws IOException, PrunUserInterruption, UserInterruption
   {

      cwObj.print((utils.formatString(requestString,46,' ')));
      String input=crObj.getLine();
      if ((input == null || input.trim().equals("")) && defaultVal != null)
      {
         input=defaultVal;
      }
      else if (input != null)
      {
         input=input.trim();
      }
      if ((input != null && input.equals("*")) || isUserInterrupt)
      {
         throw pui;
      }

      return input;
   }

   /*
   *  Gets the number of connection available for parallelload
   *  the maxthread value is set by the return value of this method
   */
   int getMaxServers()
   {
      boolean moreResults=false;
      int defaultservercnt=MAX_THREADS;
      Statement statement=null;
      try
      {
         if ((statement=sessObj.getStmtObj()) != null)
         {
            moreResults=statement.execute("cfgcmd:INFO DS "+sessObj.getSessionDsn());
         }

         if (moreResults)
         {
            ResultSet resultSet=statement.getResultSet();
            if (resultSet.next())
               defaultservercnt=resultSet.getInt("MAX_SRVR_CNT");

         }
      }
      catch (SQLException sqle)
      {
         //printAllExceptions(sqle);
      }
      return defaultservercnt;

   }


   public void run()
   {

      SessionInterface shareObj=new SessionInterface();
      Session sessObjt=null;
      ErrorObject errObj=null;
      StringBuffer errBuf = new StringBuffer();
      String threadName=currentThread().getName();
      String logFile=null;
      ArrayList<Vector<String>> errArr = new ArrayList<Vector<String>>();
      Vector<String> errVec=null;


      try
      {
         sessObjt=new Session(   this.sessObj.getSessionUser(),
            this.sessObj.getSessionPass(),
            this.sessObj.getSessionRole(),
            this.sessObj.getSessionServer(),
            this.sessObj.getSessionPort(),
            this.sessObj.getSessionDsn(),
            crObj,
            cwObj);
         // copy the required values from the main session
         sessObjt.setSessionCtlg(this.sessObj.getSessionCtlg());
         sessObjt.setSessionSchema(this.sessObj.getSessionSchema());
         sessObjt.setSessionSQLTerminator(this.sessObj.getSessionSQLTerminator());
         sessObjt.setSessionPrompt(this.sessObj.getPrompt());
         sessObjt.setSessionTiming(this.sessObj.isSessionTimingOn());
         sessObjt.setSessionTime(this.sessObj.isSessionTimeOn());
         sessObjt.setSessionColSep(this.sessObj.getSessionColSep());
         sessObjt.setListCount(this.sessObj.getListCount());
         sessObjt.setStrDisplayFormat(this.sessObj.getStrDisplayFormat());
         sessObjt.setMxosrvrVersion(this.sessObj.getMxosrvrVersion());
         sessObjt.setSutVersion(this.sessObj.getSutVersion());
         sessObjt.setT4verNum(sessObj.getT4verNum());
         sessObjt.setSessionAutoPrepare(sessObj.isSessionAutoPrepare());
         sessObjt.setSessionStatsEnabled(sessObj.isSessionStatsEnabled());

         sessObjt= shareObj.createSession(sessObjt, 1, false);

      } catch (FileNotFoundException e)
      {
    	  errObj=SessionError.SCRIPT_FILE_NOT_FOUND;
         return;
      } catch (SQLException e)
      {
         if (e.getErrorCode() == SessionDefaults.SQL_ERR_CONN_MAX_LIMIT)
         {
        	 errObj=SessionError.CONN_MAX_LIMIT_ERR;
         }else
         {
			errObj = new ErrorObject(e.toString(), e.getErrorCode());
         }

         return;
      } catch (InstantiationException e)
      {
    	  errObj=SessionError.DRIVER_INIT_ERR;
         return;
      } catch (IllegalAccessException e)
      {
    	  errObj=SessionError.DRIVER_INIT_ILLEGAL_ERR;
         return;
      } catch (ClassNotFoundException e)
      {
    	  errObj=SessionError.DRIVER_CLASS_ERR;
         return;
      } catch (IOException e)
      {
    	  errObj=new ErrorObject(SessionError.INTERNAL_ERR, "", " "+e.toString());
         return;
      }
      finally
      {
         if (this.isUserInterrupt)
         {
            try
            {

               if (sessObjt.getConnObj() != null)
               {
                  sessObjt.getConnObj().close();
               }
            } catch (SQLException e)
            {
               //
            }
            sessObjt.setConnObj(null);
            shareObj=null;
            return;
         }
         if (errObj != null)
         {
            synchronized(this)
            {
               //if(this.summaryWriter !=null)
               //this.summaryWriter.write(" Failed");
               try
               {
                  this.summaryWriter.writeln();
                  this.summaryWriter.writeln(DateFormat.getDateTimeInstance().format(new Date()) + " An error occurred in "+threadName+" when connecting to the database: "+errObj.errorMessage());
                  errorMsg = true;
               } catch (IOException e)
               {

               }
            }
         }
         synchronized(this)
         {
            activeSessionLists.add(sessObjt);
         }


      } //end finally

      this.totalConnections++;
      //cwObj.println("\nConnection made for "+threadName);

      while (true)
      {

         if (timerMode && ((nowTime=System.currentTimeMillis()) >= endTime))
         {
            //cwObj.println("\n" + threadName + " run time reached, terminating ..."); 
            break;
         }

         logFile=null;
         synchronized(this.scriptsList)
         {
            if (this.scriptsList.size() > 0)
            {

               if (this.isUserInterrupt)
               {
                  break;
               }
               String fileName=this.scriptsList.get(0).toString();
               logFile=fileName+".log";
               shareObj.setLogFile(this.logsDir+File.separator+logFile,this.overwriteLogs);
               shareObj.setScriptFile(this.scriptsDir+File.separator+fileName);
               this.scriptsList.remove(0);
               this.totalScriptFilesProcessed++;
               //debug information, prints the thread name and the script file it picks up for processing
               if (prunDebug ){
                  try
                  {
                     String debugMsg = DateFormat.getDateTimeInstance().format(new Date()) + ": Thread " + threadName +  " processing " + shareObj.getScriptFile();
                     if (sessObj.getDisplayFormat() == SessionDefaults.XML_FORMAT)
                        this.summaryWriter.writeln(xmlObj._beginCdataTag + debugMsg + xmlObj._endCdataTag);
                     else if (sessObj.getDisplayFormat() == SessionDefaults.HTML_FORMAT)   
                        this.summaryWriter.writeln(htmlObj._startCommentTag + debugMsg + htmlObj._endCommentTag);
                     else
                        this.summaryWriter.writeln( debugMsg );
                  } catch (IOException ioEx)
                  {
                  }
               }

            }
            else
            { // no more files to process

               if (timerMode)
               {
                  this.scriptsList.addAll(this.scriptsListBak);
                  //cwObj.println("List addAll");
                  continue;
               }

               break;
            }
         }
         try
         {
            int sqlerrCnt=shareObj.getSqlErrorCount();
            int sqlQueryCnt=shareObj.getSqlQueryCount();
            int sqlWarningsCnt=shareObj.getSqlWarningsCount();
            shareObj.invokeSession(sessObjt);
            //if there are any error occurred in the current file ..write them in to the errBuf
            if (shareObj.getSqlErrorCount() > sqlerrCnt || shareObj.getSqlWarningsCount() > sqlWarningsCnt)
            {
               errVec = new Vector<String>();
               String scriptSqls=""+(shareObj.getSqlQueryCount() - sqlQueryCnt);
               String scriptErrs=""+(shareObj.getSqlErrorCount() - sqlerrCnt);
               String scriptWarns=""+(shareObj.getSqlWarningsCount() - sqlWarningsCnt);
               if (sessObj.getDisplayFormat() == SessionDefaults.RAW_FORMAT || sessObj.getDisplayFormat() == SessionDefaults.CSV_FORMAT)
               {
                  errBuf.append(utils.formatString(logFile,65,' ',"")+"  "+utils.formatString("",10,' ',""+scriptSqls)+"  "+utils.formatString("",7,' ',""+scriptErrs)+"  "+utils.formatString("",10,' ',""+scriptWarns));
                  errBuf.append("####");
               }
               else
               {
                  errVec.add(0,logFile);
                  errVec.add(1,scriptSqls);
                  errVec.add(2, scriptErrs);
                  errVec.add(3, scriptWarns);
                  errArr.add(errVec);
               }

            }
         } catch (IOException e)
         {
            // where to write this exception
         }
      }
      try
      {
         //if(sessObjt.getConnObj() != null)
         sessObjt.getConnObj().close();
         sessObjt.setConnObj(null);
      }catch (SQLException sqle)
      {

      }finally
      {
         sessObjt.setConnObj(null);
         synchronized(this)
         {
            //this.totalScriptFilesProcessed++;
            int sqlerrors=this.totalSQLErrors;
            int sqlwarnings=this.totalSQLWarnings;
            boolean isErrHeadPrinted=true;
            this.totalSQLErrors+=shareObj.getSqlErrorCount();
            this.totalSQLWarnings+=shareObj.getSqlWarningsCount();
            if ((sqlerrors == 0 && this.totalSQLErrors != 0) || 
                (sqlwarnings == 0 && this.totalSQLWarnings !=0) )
            {
               isErrHeadPrinted=false;
            }
            this.totalSQLsProcessed+=shareObj.getSqlQueryCount();
            try
            {
               if (!isErrHeadPrinted)
               {
                  if (sessObj.getDisplayFormat() == SessionDefaults.RAW_FORMAT || sessObj.getDisplayFormat() == SessionDefaults.CSV_FORMAT)
                  {

                     this.summaryWriter.writeln();
                     this.summaryWriter.writeln("Check the following log files for the detailed error message.");
                     this.summaryWriter.writeln(utils.formatString("-",4+65+27+8,'-'));  // 65 - size of the log file name + 27 - size of the remaining fields + 6 - spaces between the heading columns
                     this.summaryWriter.writeln(utils.formatString("Seq#",4,' ',"")+"  "+utils.formatString("Log File Name",65,' ',"")+"  "+utils.formatString("",10,' ',"Total Qrys")+"  "+utils.formatString("",7,' ',"Errors")+"  "+utils.formatString("",10,' ',""+"Warnings"));
                     this.summaryWriter.writeln(utils.formatString("-",4+65+27+8,'-',""));
                  }
                  else if (sessObj.getDisplayFormat() == SessionDefaults.HTML_FORMAT)
                  {
                     this.summaryWriter.writeln(htmlObj._beginRowTag);
                     this.summaryWriter.writeln(htmlObj._beginTblHeadTag + "Seq # " + htmlObj._endTblHeadTag);
                     this.summaryWriter.writeln(htmlObj._beginTblHeadTag + "Logfile " + htmlObj._endTblHeadTag);
                     this.summaryWriter.writeln(htmlObj._beginTblHeadTag + "Total Qrys" + htmlObj._endTblHeadTag);
                     this.summaryWriter.writeln(htmlObj._beginTblHeadTag + "Errors" + htmlObj._endTblHeadTag);
                     this.summaryWriter.writeln(htmlObj._beginTblHeadTag + "Warnings" + htmlObj._endTblHeadTag);
                     this.summaryWriter.writeln(htmlObj._endRowTag);
                  }
                  isErrHeadPrinted=true;
               }

               if (errBuf.length() > 0)
               {
                  int newLineIdx=-1;

                  while ((newLineIdx=errBuf.indexOf("####")) != -1)
                  {
                     this.seqNo++;
                     this.summaryWriter.writeln(utils.formatString("",4,' ',this.seqNo+"")+"  "+errBuf.substring(0,newLineIdx).toString());
                     errBuf.delete(0,newLineIdx+4);
                  }
               }
               else
               {
                  for (int i=0; i< errArr.size();i++)
                  {
                     Vector<String> vect = errArr.get(i);
                     this.seqNo++;
                     if (sessObj.getDisplayFormat() == SessionDefaults.XML_FORMAT)
                     {
                        this.summaryWriter.writeln(xmlObj._beginSeqIdTag + this.seqNo + xmlObj._endAttributeTag);
                        this.summaryWriter.writeln(xmlObj._beginLogFileNameTag + vect.get(0).trim() + xmlObj._endLogFileNameTag);
                        this.summaryWriter.writeln(xmlObj._beginTotalSqlsTag + vect.get(1).trim() + xmlObj._endTotalSqlsTag);
                        this.summaryWriter.writeln(xmlObj._beginTotalErrors + vect.get(2).trim() + xmlObj._endTotalErrors);
                        this.summaryWriter.writeln(xmlObj._beginTotalWarnings + vect.get(3).trim() +xmlObj._endTotalWarnings);
                        this.summaryWriter.writeln(xmlObj._endSeqIdTag);
                     }
                     else if ((sessObj.getDisplayFormat() == SessionDefaults.HTML_FORMAT))
                     {
                        this.summaryWriter.writeln(htmlObj._beginRowTag);
                        this.summaryWriter.writeln(htmlObj._beginTblDataAlignTag + this.seqNo + htmlObj._endTblDataTag);
                        this.summaryWriter.writeln(htmlObj._beginTblDataTag + vect.get(0) + htmlObj._endTblDataTag);
                        this.summaryWriter.writeln(htmlObj._beginTblDataAlignTag +vect.get(1) + htmlObj._endTblDataTag);
                        this.summaryWriter.writeln(htmlObj._beginTblDataAlignTag + vect.get(2)+ htmlObj._endTblDataTag);
                        this.summaryWriter.writeln(htmlObj._beginTblDataAlignTag + vect.get(3) + htmlObj._endTblDataTag);
                        this.summaryWriter.writeln(htmlObj._endRowTag);
                     }
                  }
               }
               errBuf=null;
            } catch (IOException e)
            {

            }
         }
      }
      sessObjt=null;



   } //end run

   public boolean validateArgs(String query) throws InvalidNumberOfArguments, IOException
   {
      if (query.endsWith(sessObj.getSessionSQLTerminator()))
      {
         query = query.substring(0, query.length()-sessObj.getSessionSQLTerminator().length());
      }

      setArgs(query.split(" "));

      if (noOfArgs == 1)
         return true;

      if (sessObj.isDebugOn())
    	  System.out.println("No.of args :: " + noOfArgs);
      if (noOfArgs > 11)
      {
         //writeObj.writeln();
         throw new InvalidNumberOfArguments();
      }

      //Set defaults for non-interactive mode
      scriptsDir=System.getProperty("user.dir");
      scriptsExt = SCRIPT_EXT;
      connectionCnt = MIN_THREADS;

      for (int i=1; i < noOfArgs; i++)
      {
         String value = null;
         String option=this.args[i++].trim();

         if (i < noOfArgs)
         {
            value=this.args[i].trim();
         }
         else
         {
            if (!option.equalsIgnoreCase("-d") && !option.equalsIgnoreCase("-defaults"))
            {
               //writeObj.writeln();
               throw new InvalidNumberOfArguments();
            }
         }

         if (option.equalsIgnoreCase("-sd")|| option.equalsIgnoreCase("-scriptsdir"))
         {
            scriptsDir=this.getCanonicalPath(value);
         }
         else if (option.equalsIgnoreCase("-e")|| option.equalsIgnoreCase("-extension"))
         {
            scriptsExt=value;
         }
         else if (option.equalsIgnoreCase("-ld")|| option.equalsIgnoreCase("-logsdir"))
         {
            logsDir=this.getCanonicalPath(value);
         }
         else if (option.equalsIgnoreCase("-o")|| option.equalsIgnoreCase("-overwrite"))
         {
            if (value.equalsIgnoreCase("y"))
            {
               this.overwriteLogs=true;
            }
            else if (value.equalsIgnoreCase("n"))
            {
               this.overwriteLogs=false;
            }
            else
            {
               this.writePrunErrors(SessionError.INCORRECT_OVERWRITE_OPTION);
               return false;
            }
         }
         else if (option.equalsIgnoreCase("-c")|| option.equalsIgnoreCase("-connections"))
         {
            try
            {
               connectionCnt = Integer.parseInt(value);
            } catch (NumberFormatException nfe)
            {
               this.writePrunErrors(SessionError.INVALID_CONN_VALUE);
               return false;
            }
         }
         else if (option.equalsIgnoreCase("-m")|| option.equalsIgnoreCase("-minutes"))
         {
            if (!timerMode)
            {
                   this.writePrunErrors(new ErrorObject(SessionError.UNKOWN_OPTION, "" ,option));
                   //If not in XML, HTML, or CSV markup mode, display help usage.
                   if(sessObj.getDisplayFormat() == SessionDefaults.RAW_FORMAT)
                        printUsage();
               return (false);
            }

            timerValue = Integer.parseInt(value);

         }
         else if (option.equalsIgnoreCase("-d")|| option.equalsIgnoreCase("-defaults"))
         {
            if (noOfArgs > 2)
            {
             	this.writePrunErrors(SessionError.DEFAULT_OPTION_ERR);
                //If not in XML, HTML, or CSV markup mode, display help usage.
                if(sessObj.getDisplayFormat() == SessionDefaults.RAW_FORMAT)
                       printUsage();
               return (false);
            }
         }
         else
         {
         	this.writePrunErrors(new ErrorObject(SessionError.UNKOWN_OPTION.errorCode(),SessionError.UNKOWN_OPTION.errorMessage()+ option));
             //If not in XML, HTML, or CSV markup mode, display help usage.
            if(sessObj.getDisplayFormat() == SessionDefaults.RAW_FORMAT)
                printUsage();
            return (false);
         }

      } //end for

      if (!isValid(scriptsDir,"dir","read"))
      {
       	this.writePrunErrors(SessionError.SCRIPTS_DIR_NOT_FOUND);
         return (false);
      }

      if (logsDir == null)
         logsDir = scriptsDir;
      else if (!isValid(logsDir,"dir","read"))
      {
       	this.writePrunErrors(SessionError.LOGS_DIR_NOT_FOUND);
         return (false);
      }

      if (connectionCnt < MIN_THREADS || connectionCnt > MAX_THREADS)
      {
       	this.writePrunErrors(SessionError.PRUN_CONN_CNT_ERR);
         return (false);
      }

      if (timerMode)
      {
         if (timerValue > 0)
         {
            endTime= (timerValue * 1000*60) + System.currentTimeMillis();
         }
         else if (timerValue < 0)
         {
            cwObj.println("Invalid timer value");
            cwObj.println();
            return (false);
         }
         else //timerValue=0
         {
            cwObj.println(SessionDefaults.lineSeperator+"Timer mode ignored, -m is 0"+SessionDefaults.lineSeperator);
            timerMode = false;
         }

      }

      if (cwObj.getConsoleOut())
      {
         cwObj.println(SessionDefaults.lineSeperator+"PRUN options are -scriptsdir  " + scriptsDir +
            SessionDefaults.lineSeperator+"                 -logsdir     " + logsDir +
            SessionDefaults.lineSeperator+"                 -extension   " + scriptsExt +
            SessionDefaults.lineSeperator+"                 -overwrite   " + (this.overwriteLogs ? "y":"n") +
            SessionDefaults.lineSeperator+"                 -connections " + connectionCnt);

         if (timerMode)
            cwObj.println(SessionDefaults.lineSeperator+"                 -minutes     " + timerValue );
      }

      return (true);


   } //end validateArgs

   private void setArgs(String[] args)
   {
      noOfArgs=args.length;
      this.args=args;
   }

   private String getCanonicalPath(String fileName)
   {
      File file = new File(fileName);

      try
      {
         return (file.getCanonicalPath());
      }catch (IOException io)
      {

      }
      return (fileName);
   }

   public void printUsage() throws IOException
   {
      writeObj.writeln();
      writeObj.writeln("prun -sd|scriptsdir <directory-name> -e|extension <extension> ");
      writeObj.writeln("     -ld|logsdir <directory-name>  -o|overwrite {y|n} ");
      writeObj.writeln("     -c|connections <no-of-connections>" );
      if (timerMode)
         writeObj.writeln("     -m|minutes <no-of-minutes>" );

      writeObj.writeln();
      writeObj.writeln("\t----- OR -----");
      writeObj.writeln();
      writeObj.writeln("prun -d|defaults");
      writeObj.writeln();
      writeObj.writeln("where:");
      writeObj.writeln("\t-defaults \tspecifies default values for all the options.");
      writeObj.writeln("\t-scriptsdir \tspecifies the directory containing the script files.");
      writeObj.writeln("\t-extension \tspecifies the extension of script files.");
      writeObj.writeln("\t-logsdir \tspecifies the directory where logs files are created.");
      writeObj.writeln("\t-overwrite \tspecifies if the log files have to be overwritten.");
      writeObj.writeln("\t-connections \tspecifies the number of connections.");
      if (timerMode)
      {
         writeObj.writeln("\t-minutes \tspecifies the time to run test.");
         writeObj.writeln();
         writeObj.writeln("\t[ optional property -Dtrafci.prun.connection.delta=n ]");
      }
   }

   private String formatSummaryStr(String msg) 
   {
      String summaryStr = null;
      if (sessObj.getDisplayFormat() == SessionDefaults.XML_FORMAT)
      {
         summaryStr = xmlObj._beginCdataTag + msg + xmlObj._endCdataTag;
      }
      else if (sessObj.getDisplayFormat() == SessionDefaults.HTML_FORMAT)
      {
         summaryStr = htmlObj._startCommentTag + msg + htmlObj._endCommentTag;
      }
      else
      {
         summaryStr = "\t" + msg + SessionDefaults.lineSeperator;
      }
      return summaryStr;
   }
   
   private void writePrunErrors(ErrorObject prunError) throws IOException
   {
       if(sessObj.getDisplayFormat() != SessionDefaults.XML_FORMAT)
           writeObj.writeln();
       
       writeObj.writeInterfaceErrors(this.sessObj, prunError);
   }

   private void endPrunConsoleTags() 
   {
      if (sessObj.getDisplayFormat() == SessionDefaults.XML_FORMAT)
      {
         cwObj.println(xmlObj._endRootTag);
         xmlObj._beginRootElement = false;
      }
      else if (sessObj.getDisplayFormat() == SessionDefaults.HTML_FORMAT)
      {
         cwObj.println(htmlObj._endTableTag);
         htmlObj._startTags = false;
      }
   }
   
} //end class
