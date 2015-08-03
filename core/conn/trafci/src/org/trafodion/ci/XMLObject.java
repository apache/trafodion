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
 * 
 * 
 * Class to display the Errors/Warnings/ResultSets/Status Messages in XML format 
 * 
 */

package org.trafodion.ci;

import java.io.*;

public class XMLObject
{

   private Utils utilObj = null;
//   private Parser parser = null;
   private Session sessObj = null;
   private Writer writer = null;
   private Query qryObj = null;
   private QueryUtils qryUtilObj=null;

   int rowCount = 0;
   int colCount = 1;
   int errorCount = 0;

   public String _xmlNameSpaceTag="<?xml version=\"1.0\"?>";
   private String encodingTag="";
   public String _beginRootTag = "<Results>";
   public String _endRootTag = "</Results>";

   String _beginQueryTag = " <Query>";
   String _endQueryTag = " </Query>";

   boolean _beginRootElement = false;
   boolean _errors = false;
   boolean _warnings = false;
   boolean _initDone = false;

   String _beginRowTag = " <Row id=\"" ;
   String _endRowTag = " </Row>";

   String _beginCdataTag=" <![CDATA[";
   String _endCdataTag = " ]]>";
   String _endCdataTagEscaped = "]]&#62;";

   String _beginStatusTag = "<Status>";
   String _endStatusTag = "</Status>";

   String _beginErrorListTag = " <ErrorList>";
   String _endErrorListTag = " </ErrorList>";

   String _beginErrorCountTag = "  <Error id=\"" ;
   String _endErrorCountTag = "  </Error>";

   String _beginErrorCodeTag = "    <ErrorCode>";
   String _endErrorCodeTag = "</ErrorCode>";
   
   String _beginMessageCodeTag = "    <MessageCode>";
   String _endMessageCodeTag = "</MessageCode>";

   String _beginErrorMsgTag = "    <ErrorMsg>";
   String _endErrorMsgTag = "</ErrorMsg>";

   String _beginMessageMsgTag = "    <MessageText>";
   String _endMessageMsgTag = "</MessageText>";
   
   String _beginWarnListTag = " <WarningList>";
   String _endWarnListTag = " </WarningList>";

   String _beginWarnCountTag = "  <Warning id=\"" ;
   String _endWarnCountTag = "  </Warning>";

   String _beginWarnCodeTag = "    <WarningCode>";
   String _endWarnCodeTag = "</WarningCode>";

   String _beginWarnMsgTag = "    <WarningMsg>";
   String _endWarnMsgTag = "</WarningMsg>";

   public String _beginScriptsDirTag = "<ScriptsDirectory>";
   public String _endStriptsDirTag = "</ScriptsDirectory>";

   public String _beginLogsDirTag = "<LogsDirectory>";
   public String _endLogsDirTag = "</LogsDirectory>";

   public String _beginLogsO = "<OverWriteLog>";
   public String _endLogsO = "</OverWriteLog>";

   public String _beginConnTag = "<TotalConnections>";
   public String _endConnTag = "</TotalConnections>";

   public String _beginSeqIdTag = "<Seq id = \"";
   public String _endSeqIdTag = "</Seq>";

   public String _beginLogFileNameTag = "  <LogFileName>";
   public String _endLogFileNameTag = "  </LogFileName>";

   public String _beginTotalSqlsTag = "  <TotalSqls>";
   public String _endTotalSqlsTag = "  </TotalSqls>";

   public String _beginTotalErrors = "  <Errors>";
   public String _endTotalErrors = "  </Errors>";

   public String _beginTotalWarnings = "  <Warnings>" ;
   public String _endTotalWarnings = "  </Warnings>" ;

   public String _beginMinTag = "<TotalMinutes>";
   public String _endMinTag = "</TotalMinutes>";

   public String _endAttributeTag = "\">";

   String _beginResultSetIdTag = " <ResultSet id=\"" ;
   String _beginResultSetTag = " <ResultSet>" ;
   String _endResultSetTag = " </ResultSet>";
   
   String _beginExecutionTimeTag = " <ExecutionTime>";
   String _endExecutionTimeTag = "</ExecutionTime>";
   
   String statusMsg = "";
   boolean perTableStats = false;
   
   

   XMLObject ()
   {
   }

   XMLObject (Session sessObj)
   {
      this.sessObj = sessObj;
//      parser = new Parser();
      utilObj = new Utils();
      qryUtilObj = new QueryUtils();
      
   }

   public void init()
   {
      this.writer=sessObj.getWriter();
      this.qryObj=sessObj.getQuery();
      _initDone = true;
    }

   public void processXml(String columnName, String output) throws IOException
   {

      if (!_initDone)
      {
         this.init();
         this.handleStartTags();
         //_initDone = true;
      }
      if (sessObj.isInOutandRS() && qryObj.getRsCount() >= 0)
      {
         //   writer.writeln(" " + _beginResultSetTag);
         qryObj.resetQueryText("SELECT *");
         rowCount = 0;
         colCount = 1;

      }
      if ((qryObj.getRowCount() != null))
      {

         if ((_warnings) || ((rowCount==0 && Integer.parseInt(qryObj.getRowCount())==0)  || (rowCount != Integer.parseInt(qryObj.getRowCount()))))
         {
            this.handleStartTags();
         }
         
         if ((qryObj.isTrimOut() && !qryUtilObj.isGetStatsCmd(qryObj)) || (perTableStats) )
         {
            writer.writeln(" " + _endCdataTag);
            perTableStats = false;
         }
         
         if (_warnings && !sessObj.isWriteParams())
            writer.writeln(_endWarnListTag);
         
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
            writer.write(_beginStatusTag + _beginCdataTag + statusMsg + output +  _endCdataTag + _endStatusTag +SessionDefaults.lineSeperator);
         this.handleEndTags();
         return;
      }

      if (qryObj.isTrimOut())
      {
         if (qryUtilObj.isGetStatsCmd(qryObj) && !perTableStats)
         {
            if (!(output.trim().equals("")))
               printGetStatsOutput(output.trim());
         }
         else
               writer.write(output.trim());
      }else
      {
         if (colCount == 1)
         {
            if (_warnings && sessObj.isWriteParams())
               writer.writeln(_endWarnListTag);
            if (sessObj.isSPJRS() && rowCount == 0 || (sessObj.isInOutandRS()))
            {
               writer.write( _beginResultSetIdTag + (qryObj.getRsCount()+1) + _endAttributeTag + SessionDefaults.lineSeperator);
               sessObj.setInOutandRS(false);
            }
            writer.write(_beginRowTag + (rowCount+1) + _endAttributeTag + SessionDefaults.lineSeperator);

         }
         writer.write("   <"+columnName+">"+utilObj.formatXMLdata(output.trim())+"</"+columnName+">");

         if (qryObj.getColCount() != null)
         {
            colCount++;
            if (colCount > Integer.parseInt(qryObj.getColCount()))
            {
               writer.write(SessionDefaults.lineSeperator+_endRowTag);
               rowCount++;
               colCount=1;
            }
         }
      }
   }

   public void handleStartTags() throws IOException
   {
      if (!_beginRootElement)
      {
         if (sessObj.getISOMapping() == 10)
         {
            encodingTag = "encoding=\"" + "Shift_JIS"+ "\"";
            _xmlNameSpaceTag = "<?xml version=\"1.0\" " + encodingTag + "?>";
         }
         else
         {
            _xmlNameSpaceTag = "<?xml version=\"1.0\"?>";
         }

         writer.writeln(_xmlNameSpaceTag);
         writer.writeln(_beginRootTag);

         if (qryObj.getQueryText() != null)
         {
            writer.writeln(_beginQueryTag);

            if (sessObj.isSPJRS() && qryObj.getRowCount() == "0")
               qryObj.resetQueryText(sessObj.getDbQryObj().qryText);

            //Escape "]]>" if it appears within QueryText
            writer.writeln(" " + _beginCdataTag + sessObj.getQuery().getQueryText().replaceAll(_endCdataTag.trim(), _endCdataTagEscaped)+ _endCdataTag);
            writer.writeln(_endQueryTag);

            if (qryObj.isTrimOut()) 
               writer.writeln(" " + _beginCdataTag);

            if (sessObj.isSPJRS() && qryObj.getRsCount() > 0)
            {
               writer.writeln(" " + _beginResultSetTag);
               qryObj.resetQueryText("SELECT *");
            }
         }
         
         _beginRootElement = true;
      }

   }

   public void handleEndTags() throws IOException
   {
	if (_initDone)
	{
      if (_errors)
         writer.writeln(_endErrorListTag);

      if (sessObj.isSPJRS())
      {
         writer.write(SessionDefaults.lineSeperator+_endResultSetTag);
      }
      else
      {
         if (!(sessObj.isImplicitGetStatsQry())) {
            writer.write(_endRootTag+SessionDefaults.lineSeperator);
            _beginRootElement = false;
         }
      }

      _errors = false;
      _warnings = false;
      errorCount = 0;
      rowCount = 0;
      colCount = 1;
      _initDone = false;
      sessObj.setWriteParams(false);
	}
   }

   public void handleErrors(ErrorObject errorObj) throws IOException
   {
       if (errorObj != null)
           {
	           if (!_initDone)
	               {
	               this.init();
	               }
	
	           boolean notBegin=false;
	           if(notBegin = !_beginRootElement)
	           {
	        	   this.handleStartTags();
	           }
	           
	           if (errorObj.errorCode() == Parser.UNKNOWN_ERROR_CODE)
	           {
	               writeServerMessage(errorObj);
	           }
	           else if(errorObj.errorType == 'I')
	           {
	               writeInformational(errorObj);
	           }
	           else if(errorObj.errorType == 'W')
	           {
	               handleWarnings(errorObj);
	           }
	           else 
	           {
	               if (notBegin)
	               {
	                   writer.writeln(_beginErrorListTag);
	                   _errors = true;
	               }
	               writeErrors(errorObj);
               }
         }
   }
   public void writeServerMessage(ErrorObject errorObj) throws IOException
   {
         String escapedErrorMessage = errorObj.errorMessage().replaceAll(_endCdataTag.trim(), _endCdataTagEscaped);
         //escape any instance of the substring "]]>" that appears in errorStr. It is escaped by replacing it with the _endCdataTagEscaped string.
         writer.writeln(_beginCdataTag + escapedErrorMessage + _endCdataTag);
   }
   public void writeInformational(ErrorObject errorObj) throws IOException
   {
         String escapedErrorMessage = errorObj.errorMessage().replaceAll(_endCdataTag.trim(), _endCdataTagEscaped);
         //  writer.writeln(_beginMessageCodeTag + errorObj.errorCode() + _endMessageCodeTag);
         //escape any instance of the substring "]]>" that appears in errorStr. It is escaped by replacing it with the _endCdataTagEscaped string.
         writer.writeln(_beginMessageMsgTag + _beginCdataTag + escapedErrorMessage + _endCdataTag + _endMessageMsgTag);
   }
   
   public void writeErrors(ErrorObject errorObj) throws IOException
   {
         String escapedErrorMessage = errorObj.errorMessage().replaceAll(_endCdataTag.trim(), _endCdataTagEscaped);
    	 writer.write(_beginErrorCountTag + ++errorCount + _endAttributeTag + SessionDefaults.lineSeperator );
         writer.writeln(_beginErrorCodeTag + errorObj.errorCode() + _endErrorCodeTag);

         //escape any instance of the substring "]]>" that appears in errorStr. It is escaped by replacing it with the _endCdataTagEscaped string.
         writer.writeln(_beginErrorMsgTag + _beginCdataTag + escapedErrorMessage + _endCdataTag + _endErrorMsgTag);
         writer.writeln(_endErrorCountTag);
   }
 //dead code. Removed by Kevin Xu
/* 
   public void writeWarnings(ErrorObject errorObj) throws IOException
   {
         String escapedErrorMessage = errorObj.errorMessage().replaceAll(_endCdataTag.trim(), _endCdataTagEscaped);
    	 writer.write(_beginErrorCountTag + ++errorCount + _endAttributeTag + SessionDefaults.lineSeperator );
         writer.writeln(_beginErrorCodeTag + errorObj.errorCode() + _endErrorCodeTag);

         //escape any instance of the substring "]]>" that appears in errorStr. It is escaped by replacing it with the _endCdataTagEscaped string.
         writer.writeln(_beginErrorMsgTag + _beginCdataTag + escapedErrorMessage + _endCdataTag + _endErrorMsgTag);
         writer.writeln(_endErrorCountTag);
   }
*/
   /**
    *  Method to print a top level xml tag below the root element of the
    *  marked up xml document.
    */
   private void printTopLevelXMLTag(String theTag)  throws IOException {
      /*  Check if we need to do the initialization.  */
      if (!_initDone)
         this.init();

      /*  Check if we need to setup and create the tags.  */
      if (!_beginRootElement)
         this.handleStartTags();


      if ((null != theTag)  &&  (0 < theTag.length() ) )
         writer.writeln(theTag);
        
   }  /*  End  of  printTopLevelXMLTag  method.  */



   /**
    *  Method to print the warning list start tag for all warnings
    *  encountered during a fetch operation. 
    */
   public void startFetchWarningListTag()  throws IOException {
      printTopLevelXMLTag(_beginWarnListTag);

   }  /*  End of  startFetchWarningListTag  method.  */



   /**
    *  Method to print the warning list end tag for all warnings
    *  encountered during a fetch operation. 
    */
   public void endFetchWarningListTag()  throws IOException {
      printTopLevelXMLTag(_endWarnListTag);

   }  /*  End of  endFetchWarningListTag  method.  */



   /**
    *  Method to print warnings encountered during a fetch operation. These 
    *  warnings need to printed in a separate warning list within the scope 
    *  of the fetch.
    *  Oh, this whole xml/html markup piece is so very messy!! :^(
    */
   public void handleFetchWarnings(ErrorObject errorObj) throws IOException {
      /*  Check if we need to do the initialization.  */
      if (!_initDone)
         this.init();

      /*  Check if we need to setup and create the tags.  */
      if (!_beginRootElement)
         this.handleStartTags();


      if (errorObj != null) {
         writer.writeln(_beginWarnCountTag + ++errorCount + _endAttributeTag);
         writer.writeln(_beginWarnCodeTag + errorObj.errorCode() + 
                       _endWarnCodeTag);
         writer.writeln(_beginWarnMsgTag + errorObj.errorMessage() + 
                        _endWarnMsgTag);
         writer.writeln(_endWarnCountTag);
      }

   }   /*  End of  handleFetchWarnings  method.  */

   public void handleWarnings(ErrorObject errorObj) throws IOException
   {
      if (!_initDone)
      {
         this.init();
         //_initDone = true;
      }

      if (!_beginRootElement)
      {
         this.handleStartTags();
         writer.writeln(_beginWarnListTag);
         _warnings = true;
      }

      if (errorObj != null)
      {
         writer.write(_beginWarnCountTag + ++errorCount + _endAttributeTag + SessionDefaults.lineSeperator );
         writer.writeln(_beginWarnCodeTag + errorObj.errorCode() + _endWarnCodeTag);
         writer.writeln(_beginWarnMsgTag + errorObj.errorMessage() + _endWarnMsgTag);
         writer.writeln(_endWarnCountTag);
      }
   }

   public void handlePrunSummary(ConsoleWriter cWriter, PrunSummary summaryObj,String elapsedTime) throws IOException
   {
      this.init();
      this.handleStartTags();
      cWriter.println(_beginCdataTag);
      cWriter.println(" " + SessionDefaults.lineSeperator+"PARALLELRUN(PRUN) SUMMARY");
      cWriter.println(_endCdataTag);
      cWriter.println("<Summary>");
      cWriter.println("  <TotalFilesPresent>" + summaryObj.getTotalScriptFiles() + "</TotalFilesPresent>");
      cWriter.println("  <TotalFilesProcessed>" + summaryObj.getTotalScriptFilesProcessed() + "</TotalFilesProcessed>");
      cWriter.println("  <TotalQueriesProcessed>" + summaryObj.getTotalSQLsProcessed() + "</TotalQueriesProcessed>");
      cWriter.println("  <TotalErrors>" + summaryObj.getTotalSQLErrors() + "</TotalErrors>");
      cWriter.println("  <TotalWarnings>" + summaryObj.getTotalSQLWarnings() + "</TotalWarnings>");
      cWriter.println("  <TotalSuccesses>" + summaryObj.getTotalSQLSuccess() + "</TotalSuccesses>");
      cWriter.println("  <TotalConnections>" + summaryObj.getTotalConnections() + "</TotalConnections>");
      cWriter.println("  <TotalConnectionFailures>" + summaryObj.getTotalConnectionFailures() + "</TotalConnectionFailures>");
      if (sessObj.isSessionTimingOn())
         cWriter.println("  <ElapsedTime>" + elapsedTime.trim() + "</ElapsedTime>");
      cWriter.println("</Summary>");
      
      //reset _initDone to false since it is automatically set to true in this.init();
      _initDone = false;
   }

   public void handlePrunSummary(FileWriter fWriter, PrunSummary summaryObj, String summaryEnd, String elapsedTime) throws IOException
   {

      fWriter.writeln(_beginCdataTag + "PARALLELRUN(PRUN) SUMMARY" + _endCdataTag);
      fWriter.writeln("<Summary>");
      fWriter.writeln("  <TotalFilesPresent>" + summaryObj.getTotalScriptFiles() + "</TotalFilesPresent>");
      fWriter.writeln("  <TotalFilesProcessed>" + summaryObj.getTotalScriptFilesProcessed() + "</TotalFilesProcessed>");
      fWriter.writeln("  <TotalQueriesProcessed>" + summaryObj.getTotalSQLsProcessed() + "</TotalQueriesProcessed>");
      fWriter.writeln("  <TotalErrors>" + summaryObj.getTotalSQLErrors() + "</TotalErrors>");
      fWriter.writeln("  <TotalWarnings>" + summaryObj.getTotalSQLWarnings() + "</TotalWarnings>");
      fWriter.writeln("  <TotalSuccesses>" + summaryObj.getTotalSQLSuccess() + "</TotalSuccesses>");
      fWriter.writeln("  <TotalConnections>" + summaryObj.getTotalConnections() + "</TotalConnections>");
      fWriter.writeln("  <TotalConnectionFailures>" + summaryObj.getTotalConnectionFailures() + "</TotalConnectionFailures>");
      fWriter.writeln("</Summary>");
      fWriter.writeln(_beginCdataTag + summaryEnd + _endCdataTag);
      if (sessObj.isSessionTimingOn())
         fWriter.writeln(_beginCdataTag + elapsedTime.trim() + _endCdataTag);
      fWriter.writeln(_endRootTag);
      this._initDone = false;
   }

   public void handleQryExecutionTime(String qryExecTime)  throws IOException
   {

      if (!_initDone)
      {
         this.init();
         this.handleStartTags();
         //_initDone = true;
      }
      writer.write(_beginExecutionTimeTag + _beginCdataTag + qryExecTime +  _endCdataTag + _endExecutionTimeTag +SessionDefaults.lineSeperator);
   }

   public String checkColumnNames(String colHeader)
   {
      String colName = null;
      String xmlColHeader = colHeader;
      if (xmlColHeader.startsWith("(", 0) && xmlColHeader.endsWith(")"))
         colName = xmlColHeader.substring(1, xmlColHeader.length()-1);
      else
         colName = colHeader;
      return colName;
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
         writer.writeln(" " + _beginCdataTag); 
         writer.writeln(" " + line); 
         perTableStats = true;
         return;
      }
      String outputArr[] = line.split(" ");
      for (i=0;i<outputArr.length; i++)
      {
         if (!outputArr[i].matches("^(\\-[0-9]|[0-9]|\\\\|NONE|CLOSE|SQL_|MX|PERTABLE|ACCUMULATED|PROGRESS|DEFAULT|OPERATOR|DEALLOCATED|select).*"))
            columnNameTag += outputArr[i];
         else
            break;
      }

      for (int j=Math.max(0,(i-1));j<outputArr.length; j++)
      {
         if (!outputArr[j].trim().equals(""))
         {
            if (columnData == null)
               columnData = outputArr[j];
            else
               columnData+=" " + outputArr[j];
         }
      }
      
      String xmlColTag = "Missing_tag"; 
      if (null != columnNameTag && 0 < columnNameTag.length())
         xmlColTag = checkColumnNames(columnNameTag);

      if (columnData!= null)
         writer.write("<"+xmlColTag+">"+utilObj.formatXMLdata(columnData.trim())+"</"+xmlColTag+">");
      
   }
   
   
   
}
