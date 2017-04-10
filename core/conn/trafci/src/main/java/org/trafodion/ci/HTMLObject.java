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
/**
 *Class to display the Errors/Warnings/ResultSets/Status Messages in HTML format 
 */

package org.trafodion.ci;

import java.io.*;

public class HTMLObject
{

//   private Parser parser = null;
   private Query qryObj = null;
   private Session sessObj = null;
   private Writer writer = null;
   private QueryUtils qryUtilObj = null;

   String _beginHtmlTag  = "<HTML>";
   String _endHtmlTag = "</HTML>";
   String _beginBodyTag = "<BODY>";
   String _endBodyTag = "</BODY>";
   public String _beginTableTag = "<TABLE>";
   public String _beginWarningsTableTag = "<TABLE border='2'>";
   public String _endTableTag="</TABLE>";
   String _beginRowTag="<tr>";
   String _endRowTag="</tr>";
   String _beginTblHeadTag="<th>";
   String _beginTblDataTag="<td>";
   String _beginTblDataColspanAttrTag="<td colspan=";
   String _beginTblDataAlignTag="<td align=\"right\">";
   String _endTblHeadTag="</th>";
   String _endTblDataTag="</td>";
   String _startCommentTag="<!-- ";
   String _endCommentTag=" -->";
   String _endAttributeTag=">";

   String error_Id = "Error Id";
   String error_Code = "Error Code";
   String error_Msg = "Error Message";
   
   String message_Code = "Message Code";
   String message_Msg = "Message Information";
   
   String warn_Id = "Warning Id";
   String warn_Code = "Warning Code";
   String warn_Msg = "Warning Message";
   String statusMsg = "";

   int rowCount = 0;
   int colCount = 1;
   int errorCount = 0;

   boolean _startTags = false;
   boolean _columnHeading = true;
   boolean _initDone = false;
   boolean perTableStats = false;

   HTMLObject()
   {
   }

   HTMLObject(Session sessObj)
   {
//      parser = new Parser();
      this.sessObj = sessObj;
      qryUtilObj = new QueryUtils();
   }

   public void init()
   {
      this.writer=sessObj.getWriter();
      this.qryObj=sessObj.getQuery();
   }

   public void processHtml(String output) throws IOException
   {

      if (!_initDone)
      {
         this.init();
         this.handleStartTags();
         _initDone = true;
      }

      if (qryObj.getRowCount() != null)
      {
         if ((rowCount==0 && Integer.parseInt(qryObj.getRowCount())==0)  || ((rowCount-1) != Integer.parseInt(qryObj.getRowCount())))
         {
            this.handleStartTags();
         }
         if ( (qryObj.isTrimOut() && !qryUtilObj.isGetStatsCmd(qryObj)) || (perTableStats)) {
            writer.writeln(_endCommentTag);
            perTableStats = false;
         }
         if ((sessObj.isImplicitGetStatsQry()) && qryUtilObj.isGetStatsCmd(qryObj)) 
         {
            statusMsg = "";
            sessObj.getDbQryObj().resetQryObj();
            sessObj.setImplicitGetStatsQry(false);
         }
         else
         {
            statusMsg = sessObj.getLfProps().getStatus(qryObj);
         }
         if (!((output.equals("")) && statusMsg.equals("")))
            writer.writeln(_startCommentTag + statusMsg + output+ _endCommentTag);

         if (qryObj.getRsCount() > 0 && sessObj.isSPJRS())
         {
            writer.write(_endTableTag+SessionDefaults.lineSeperator);
            _columnHeading = true;
            rowCount = 0;
         }
         if (!sessObj.isSPJRS())
            this.handleEndTags();
         return;
      }
      if (sessObj.isInOutandRS() && qryObj.getRsCount() >= 0)
      {
         rowCount = 0;
         colCount = 1;
         _columnHeading = true;
         qryObj.resetQueryText("SELECT *");
         sessObj.setInOutandRS(false);
      }

      if (qryObj.isTrimOut())
      {
         if (qryUtilObj.isGetStatsCmd(qryObj) && !perTableStats)
         {
            if (!(output.trim().equals("")))
               printGetStatsOutput(output.trim());
         }
         else
         {
            writer.write(output.trim());
         }
      }
      else
      {
         if ((_columnHeading))
         {  // Only for Column headers
            if (colCount == 1)
            {
               if (sessObj.isSPJRS())
                  writer.write(_beginTableTag+SessionDefaults.lineSeperator);
               writer.write(_beginRowTag+SessionDefaults.lineSeperator);
            }
            writer.write("  "+_beginTblHeadTag+output.trim()+_endTblHeadTag);
         }
         else
         {
            if ((colCount == 1)) //Only for Column Data and not headers
               writer.write(_beginRowTag+SessionDefaults.lineSeperator);
            writer.write("  "+ _beginTblDataTag+output.trim()+_endTblDataTag);
         }

         if (qryObj.getColCount() != null)
         {
            colCount++;
            if (colCount > Integer.parseInt(qryObj.getColCount()))
            {
               writer.write(SessionDefaults.lineSeperator+_endRowTag);
               _columnHeading = false;
               rowCount++;
               colCount=1;
            }
         }
      }
   }
   public void handleStartTags() throws IOException
   {

      if (!_startTags)
      {
         writer.write(_beginTableTag+SessionDefaults.lineSeperator);
         _startTags = true;

         if (sessObj.isSPJRS() && qryObj.getRowCount() == "0")
            qryObj.resetQueryText(sessObj.getDbQryObj().qryText);

         writer.writeln(_startCommentTag + sessObj.getQuery().getQueryText() + _endCommentTag);

         if (sessObj.isSPJRS() && qryObj.getRsCount() > 0)
         {
            writer.write(_beginTableTag+SessionDefaults.lineSeperator);
            qryObj.resetQueryText("SELECT *");
         }

         if (qryObj.isTrimOut() && !qryUtilObj.isGetStatsCmd(qryObj)) 
            writer.writeln(_startCommentTag);
      }
   }

   public void handleEndTags() throws IOException
   {

      if (!(sessObj.isImplicitGetStatsQry())) {
      writer.write(_endTableTag+SessionDefaults.lineSeperator);
      _startTags = false;
      rowCount = 0;
      _columnHeading = true;
      errorCount = 0;
      _initDone = false;
      }
   }

   
   
   public void handleErrors(ErrorObject errorObj) throws IOException
   {
	   if (!_initDone)
	   {
	      this.init();
	      _initDone = true;
	   }
	   
	   if (errorObj.errorCode() == Parser.UNKNOWN_ERROR_CODE)
	   { 
		   writeServerMessage(errorObj);
	   }
	   else if(errorObj.errorType == 'I')
		   writeInformational(errorObj);
	   else if(errorObj.errorType == 'W')
       {
           handleWarnings(errorObj);
       }
	   else
		   writeErrors(errorObj);
   }
  
   public void writeServerMessage(ErrorObject errorObj) throws IOException
   {
      if (!_startTags)
      {
         this.handleStartTags();
         writer.write(_beginRowTag+SessionDefaults.lineSeperator);
         writer.write("  "+_beginTblHeadTag + this.message_Msg +_endTblHeadTag+ SessionDefaults.lineSeperator);
         writer.write(_endRowTag+SessionDefaults.lineSeperator);
      }

      if (errorObj != null)
      {
          writer.write(_beginRowTag+SessionDefaults.lineSeperator);
          //Escape "<" and ">" in the error message
          writer.write("  "+ _beginTblDataTag + errorObj.errorMessage().replaceAll("<", "&lt;").replaceAll(">", "&gt;") + _endTblDataTag + SessionDefaults.lineSeperator);
          writer.write(_endRowTag+SessionDefaults.lineSeperator);
      }
   }
   
   public void writeInformational(ErrorObject errorObj) throws IOException
   {

      if (!_startTags)
      {
         this.handleStartTags();
         writer.write(_beginRowTag+SessionDefaults.lineSeperator);
    //     writer.write("  "+_beginTblHeadTag + this.message_Code +_endTblHeadTag + SessionDefaults.lineSeperator);
         writer.write("  "+_beginTblHeadTag + this.message_Msg +_endTblHeadTag+ SessionDefaults.lineSeperator);
         writer.write(_endRowTag+SessionDefaults.lineSeperator);
      }

      if (errorObj != null)
      {
          writer.write(_beginRowTag+SessionDefaults.lineSeperator);
          // For all informational message we are not displaying the errorCode
    //      writer.write("  "+ _beginTblDataTag + errorObj.errorCode() + _endTblDataTag + SessionDefaults.lineSeperator);
         
          //Escape "<" and ">" in the error message
          writer.write("  "+ _beginTblDataTag + errorObj.errorMessage().replaceAll("<", "&lt;").replaceAll(">", "&gt;") + _endTblDataTag + SessionDefaults.lineSeperator);
          writer.write(_endRowTag+SessionDefaults.lineSeperator);
      }
   }

   
   public void writeErrors(ErrorObject errorObj) throws IOException
   {
      if (!_startTags)
      {
         this.handleStartTags();
         writer.write(_beginRowTag+SessionDefaults.lineSeperator);
         writer.write("  "+_beginTblHeadTag + this.error_Id +_endTblHeadTag + SessionDefaults.lineSeperator);
         writer.write("  "+_beginTblHeadTag + this.error_Code +_endTblHeadTag + SessionDefaults.lineSeperator);
         writer.write("  "+_beginTblHeadTag + this.error_Msg +_endTblHeadTag+ SessionDefaults.lineSeperator);
         writer.write(_endRowTag+SessionDefaults.lineSeperator);
      }

      if (errorObj != null)
      {
          writer.write(_beginRowTag+SessionDefaults.lineSeperator);
          writer.write("  "+ _beginTblDataTag + ++errorCount + _endTblDataTag + SessionDefaults.lineSeperator);
          writer.write("  "+ _beginTblDataTag + errorObj.errorCode() + _endTblDataTag + SessionDefaults.lineSeperator);
         
          //Escape "<" and ">" in the error message
          writer.write("  "+ _beginTblDataTag + errorObj.errorMessage().replaceAll("<", "&lt;").replaceAll(">", "&gt;") + _endTblDataTag + SessionDefaults.lineSeperator);
          writer.write(_endRowTag+SessionDefaults.lineSeperator);
      }
   }


   /**
    *  Method to print the start of the warning list table for all warnings
    *  encountered during a fetch operation. 
    */
   public void startFetchWarningListTable(int numCols)  throws IOException {

      /*  Check if we need to do the initialization.  */
      if (!_initDone) {
         this.init();
         _initDone = true;
      }


      /*  Check if we need to setup and create a table.  */
      if (!_startTags)
         this.handleStartTags();


      /*  Get the current line seperator (CRLF).  */
      String crlf = SessionDefaults.lineSeperator;


      /*
       *  Start a new row with a single cell spanning the entire table.
       *  <tr><td colspan='n'> ... 
       */
      writer.write(_beginRowTag + crlf);
      writer.write(_beginTblDataColspanAttrTag + "'" + numCols + "'" + 
                   _endAttributeTag + crlf);


      /*
       *  And now start an inner table with all the warnings information.
       *  <TABLE border="2">
       *     <tr>
       *        <th>Warning Id</th>
       *        <th>Warning Code</th>
       *        <th>Warning Message</th>
       *     </tr>
       */
      writer.write("    " + _beginWarningsTableTag  + crlf);
      writer.write("    " + _beginRowTag + crlf);
      writer.write("        " + _beginTblHeadTag + this.warn_Id +
                   _endTblHeadTag + crlf);
      writer.write("        " + _beginTblHeadTag + this.warn_Code +
                   _endTblHeadTag + crlf);
      writer.write("        " + _beginTblHeadTag + this.warn_Msg +
                   _endTblHeadTag + crlf);
      writer.write("    " + _endRowTag + crlf);


   }  /*  End of  startFetchWarningListTable  method.  */



   /**
    *  Method to end printing the warning list table for all warnings
    *  encountered during a fetch operation. 
    */
   public void endFetchWarningListTable()  throws IOException {

      /*  Get the current line seperator (CRLF).  */
      String crlf = SessionDefaults.lineSeperator;

      /*
       *  First end the  inner table with all the warnings information.
       *  <TABLE border="2">
       *     <tr>
       *        <th>Warning Id</th>
       *        <th>Warning Code</th>
       *        <th>Warning Message</th>
       *     </tr>
       *     ...
       *  </table>
       */
      writer.write("    " + _endTableTag + crlf);


      /*
       *  End the cell and row for the warning list table. 
       *  </td>
       *  </tr>
       */
      writer.write(_endTblDataTag + crlf);
      writer.write(_endRowTag + crlf);

   }  /*  End of  endFetchWarningListTable  method.  */



   /**
    *  Method to print warnings encountered during a fetch operation. These 
    *  warnings need to printed in a separate table within its own scope.
    *  Oh, this whole xml/html markup piece is so very messy!! :^(
    */
   public void handleFetchWarnings(ErrorObject errorObj) 
			throws IOException {

      /*  Check if we need to do the initialization.  */
      if (!_initDone) {
         this.init();
         _initDone = true;
      }


      /*  Check if we need to setup and create a table.  */
      if (!_startTags)
         this.handleStartTags();


      if (null != errorObj) {
         /*  Get the current line seperator (CRLF).  */
         String crlf = SessionDefaults.lineSeperator;

         /*  Print the warning information.  */
         writer.write(_beginRowTag + crlf);
         ++errorCount;
         writer.write("  " + _beginTblDataTag + errorCount +
                      _endTblDataTag + crlf);
         writer.write("  " + _beginTblDataTag + errorObj.errorCode() +
                      _endTblDataTag + crlf);

         String msgtxt = errorObj.errorMessage().replaceAll("<", "&lt;").replaceAll(">", "&gt;");
         writer.write("  " + _beginTblDataTag + msgtxt + _endTblDataTag + crlf);
         writer.write(_endRowTag + crlf);
      }

   }   /*  End of  handleFetchWarnings  method.  */

   public void handleWarnings(ErrorObject errorObj) throws IOException
   {

      if (!_initDone)
      {
         this.init();
         _initDone = true;
      }
      if (!_startTags)
      {
         this.handleStartTags();
         writer.write(_beginRowTag+SessionDefaults.lineSeperator);
         writer.write("  "+_beginTblHeadTag + this.warn_Id +_endTblHeadTag + SessionDefaults.lineSeperator);
         writer.write("  "+_beginTblHeadTag + this.warn_Code +_endTblHeadTag + SessionDefaults.lineSeperator);
         writer.write("  "+_beginTblHeadTag + this.warn_Msg +_endTblHeadTag+ SessionDefaults.lineSeperator);

         writer.write(_endRowTag + SessionDefaults.lineSeperator);
     }

      if (errorObj != null)
      {
         writer.write(_beginRowTag+SessionDefaults.lineSeperator);
         writer.write("  "+ _beginTblDataTag + ++errorCount + _endTblDataTag + SessionDefaults.lineSeperator);
         writer.write("  "+ _beginTblDataTag + errorObj.errorCode() + _endTblDataTag + SessionDefaults.lineSeperator);

          String msgtxt = errorObj.errorMessage().replaceAll("<", "&lt;").replaceAll(">", "&gt;");
          writer.write("  "+ _beginTblDataTag + msgtxt + _endTblDataTag + SessionDefaults.lineSeperator);
         writer.write(_endRowTag + SessionDefaults.lineSeperator);

      }
   }

   public void handlePrunSummary(ConsoleWriter cWriter, PrunSummary summaryObj, String elapsedTime) throws IOException
   {
      this.init();
      this.handleStartTags();
      cWriter.println(_startCommentTag);
      cWriter.println(SessionDefaults.lineSeperator+"PARALLELRUN(PRUN) SUMMARY");
      cWriter.println(_endCommentTag);
      cWriter.println(_beginRowTag);
      cWriter.println("  "+_beginTblHeadTag + "Total Files Present"+_endTblHeadTag);
      cWriter.println("  "+ _beginTblDataTag + summaryObj.getTotalScriptFiles() + _endTblDataTag);
      cWriter.println(_endRowTag);
      cWriter.println(_beginRowTag);
      cWriter.println("  "+_beginTblHeadTag + "Total Files Processed"+_endTblHeadTag);
      cWriter.println("  "+ _beginTblDataTag + summaryObj.getTotalScriptFilesProcessed() + _endTblDataTag);
      cWriter.println(_endRowTag);
      cWriter.println(_beginRowTag);
      cWriter.println("  "+_beginTblHeadTag + "Total Queries Processed"+_endTblHeadTag);
      cWriter.println("  "+ _beginTblDataTag + summaryObj.getTotalSQLsProcessed() + _endTblDataTag);
      cWriter.println(_endRowTag);
      cWriter.println(_beginRowTag);
      cWriter.println("  "+_beginTblHeadTag + "Total Errors"+_endTblHeadTag);
      cWriter.println("  "+ _beginTblDataTag + summaryObj.getTotalSQLErrors() + _endTblDataTag);
      cWriter.println(_endRowTag);
      cWriter.println(_beginRowTag);
      cWriter.println("  "+_beginTblHeadTag + "Total Warnings"+_endTblHeadTag);
      cWriter.println("  "+ _beginTblDataTag + summaryObj.getTotalSQLWarnings() + _endTblDataTag);
      cWriter.println(_endRowTag);
      cWriter.println(_beginRowTag);
      cWriter.println("  "+_beginTblHeadTag + "Total Successes"+_endTblHeadTag);
      cWriter.println("  "+ _beginTblDataTag + summaryObj.getTotalSQLSuccess() + _endTblDataTag);
      cWriter.println(_endRowTag);
      cWriter.println(_beginRowTag);
      cWriter.println("  "+_beginTblHeadTag + "Total Connections"+_endTblHeadTag);
      cWriter.println("  "+ _beginTblDataTag + summaryObj.getTotalConnections()  + _endTblDataTag);
      cWriter.println(_endRowTag);
      cWriter.println(_beginRowTag);
      cWriter.println("  "+_beginTblHeadTag + "Total Connection Failures"+_endTblHeadTag);
      cWriter.println("  "+ _beginTblDataTag + summaryObj.getTotalConnectionFailures() + _endTblDataTag);
      cWriter.println(_endRowTag);
      if (sessObj.isSessionTimingOn())
         cWriter.println(_startCommentTag + elapsedTime + _endCommentTag);
   }

   public void handlePrunSummary(FileWriter fWriter, PrunSummary summaryObj, String summaryEnd, String elapsedTime) throws IOException
   {

      fWriter.writeln(_startCommentTag + "PARALLELRUN(PRUN) SUMMARY" + _endCommentTag);
      fWriter.writeln(_beginRowTag);
      fWriter.writeln("  "+_beginTblHeadTag + "Total Files Present"+_endTblHeadTag);
      fWriter.writeln("  "+ _beginTblDataTag + summaryObj.getTotalScriptFiles() + _endTblDataTag);
      fWriter.writeln(_endRowTag);
      fWriter.writeln(_beginRowTag);
      fWriter.writeln("  "+_beginTblHeadTag + "Total Files Processed"+_endTblHeadTag);
      fWriter.writeln("  "+ _beginTblDataTag + summaryObj.getTotalScriptFilesProcessed() + _endTblDataTag);
      fWriter.writeln(_endRowTag);
      fWriter.writeln(_beginRowTag);
      fWriter.writeln("  "+_beginTblHeadTag + "Total Queries Processed"+_endTblHeadTag);
      fWriter.writeln("  "+ _beginTblDataTag + summaryObj.getTotalSQLsProcessed() + _endTblDataTag);
      fWriter.writeln(_endRowTag);
      fWriter.writeln(_beginRowTag);
      fWriter.writeln("  "+_beginTblHeadTag + "Total Errors"+_endTblHeadTag);
      fWriter.writeln("  "+ _beginTblDataTag + summaryObj.getTotalSQLErrors() + _endTblDataTag);
      fWriter.writeln(_endRowTag);
      fWriter.writeln(_beginRowTag);
      fWriter.writeln("  "+_beginTblHeadTag + "Total Warnings"+_endTblHeadTag);
      fWriter.writeln("  "+ _beginTblDataTag + summaryObj.getTotalSQLWarnings() + _endTblDataTag);
      fWriter.writeln(_endRowTag);
      fWriter.writeln(_beginRowTag);
      fWriter.writeln("  "+_beginTblHeadTag + "Total Successes"+_endTblHeadTag);
      fWriter.writeln("  "+ _beginTblDataTag + summaryObj.getTotalSQLSuccess() + _endTblDataTag);
      fWriter.writeln(_endRowTag);
      fWriter.writeln(_beginRowTag);
      fWriter.writeln("  "+_beginTblHeadTag + "Total Connections"+_endTblHeadTag);
      fWriter.writeln("  "+ _beginTblDataTag + summaryObj.getTotalConnections()  + _endTblDataTag);
      fWriter.writeln(_endRowTag);
      fWriter.writeln(_beginRowTag);
      fWriter.writeln("  "+_beginTblHeadTag + "Total Connection Failures"+_endTblHeadTag);
      fWriter.writeln("  "+ _beginTblDataTag + summaryObj.getTotalConnectionFailures() + _endTblDataTag);
      fWriter.writeln(_endRowTag);
      fWriter.writeln(_startCommentTag + summaryEnd + _endCommentTag);
      if (sessObj.isSessionTimingOn())
         fWriter.writeln(_startCommentTag + elapsedTime + _endCommentTag);
      fWriter.writeln(_endTableTag);
      this._initDone = false;
   }

   public void handleQryExecutionTime(String qryExecTime)  throws IOException
   {

      if (!_initDone)
      {
         this.init();
         this.handleStartTags();
         _initDone = true;
      }
      writer.write(_startCommentTag + qryExecTime +  _endCommentTag +SessionDefaults.lineSeperator);
   }
   
   public void printGetStatsOutput(String line) throws IOException
   {
      int i=0;
      String columnNameTag = "";
      String columnData = null;
      
      if (line.trim().equals(""))
            return;
      
      if (line.startsWith("Table Name") || line.startsWith("Id"))
      {
         writer.writeln(" " + _startCommentTag); 
         writer.writeln(" " + line); 
         perTableStats = true;
         return;
      }
      String outputArr[] = line.split(" ");
      for (i=0;i<outputArr.length; i++)
      {
    	 if (sessObj.isDebugOn())
    	  System.out.println("****outputArr["+i+"]::" + outputArr[i]);
         if (!outputArr[i].matches("^(\\-[0-9]|[0-9]|\\\\|NONE|CLOSE|SQL_|MX|PERTABLE|ACCUMULATED|PROGRESS|DEFAULT|OPERATOR|DEALLOCATED|select).*"))
            columnNameTag += outputArr[i];
         else
            break;
      }
      if (sessObj.isDebugOn()) {
    	  System.out.println("***i:"+i);
    	  System.out.println("***outputArr.length:"+outputArr.length);
      }
      for (int j=Math.max(0,(i-1));j<outputArr.length; j++)
      {
         if (!outputArr[j].trim().equals(""))
         {
            if (columnData == null)
               columnData = outputArr[j].trim();
            else
               columnData+=" " + outputArr[j].trim();
         }
      }
      writer.writeln(_beginRowTag);
      writer.writeln("  "+ _beginTblHeadTag + columnNameTag + _endTblHeadTag);
      writer.writeln("  "+ _beginTblDataTag + columnData + _endTblDataTag);
      writer.write(_endRowTag);
      
      
   }
   
}
