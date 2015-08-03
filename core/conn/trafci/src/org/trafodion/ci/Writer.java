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

import java.io.IOException;
import java.sql.SQLException;
import java.sql.SQLWarning;

public class Writer
{

   private ConsoleWriter cWriter=null;
   private FileWriter lWriter=null;
   private FileWriter sWriter=null;
   private InterfaceSyntaxError ise=null;
   private ConditionalSyntaxError cse=null;
   private Parser parser = new Parser();
   private int writerMode=SessionDefaults.CONSOLE_WRITE_MODE;
   private Session sessObj=null;

   Writer()
   {
   }

   Writer(Session sessObj)
   {
      this.sessObj = sessObj;
   }



   public void write(String line) throws IOException
   {
      switch (writerMode)
      {
         case SessionDefaults.CONSOLE_WRITE_MODE:
            cWriter.print(line);
            break;
         case SessionDefaults.LOG_WRITE_MODE:
            lWriter.write(line);
            break;
         case SessionDefaults.SPOOL_WRITE_MODE:
            sWriter.write(line);
            break;
         case SessionDefaults.CONSOLE_SPOOL_WRITE_MODE:      	 
        	 if (!sessObj.isQuietEnabled()) 
             	cWriter.print(line);
             sWriter.write(line);
             
            break;
      }
   }

   public void writeln() throws IOException
   {
      switch (writerMode)
      {
         case SessionDefaults.CONSOLE_WRITE_MODE:
            cWriter.println();
            break;
         case SessionDefaults.LOG_WRITE_MODE:
            lWriter.writeln();
            break;
         case SessionDefaults.SPOOL_WRITE_MODE:
            sWriter.writeln();
            break;
         case SessionDefaults.CONSOLE_SPOOL_WRITE_MODE:
        	if (!sessObj.isQuietEnabled()) 
             	cWriter.println();
             sWriter.writeln();

            break;
      }
   }

   public void writeAllSQLExceptions(Session sessObj, SQLException sqle) throws IOException
   {
      do
      {
         if (!sessObj.isSessionAutoPrepare())
         {
			String errorInfo = parser.getErrorCode(
						formatErrStr(sqle.toString(), sessObj),
						sqle.getErrorCode());
            if (errorInfo.equals("8822") && !sessObj.getQuery().getQueryText().trim().toUpperCase().startsWith("PREPARE "))
            {
               continue;
            }
         }
		this.writeInterfaceError(sessObj,
					new ErrorObject(formatErrStr(sqle.toString(), sessObj),
							sqle.getErrorCode()));
      } while ((sqle=sqle.getNextException()) != null);

      // Print end tags if MARKUP is XML or HTML
      this.writeEndTags(sessObj);
   }

   /*
    *  Writes all the warnings that are returned as part of the ResultSet
    *  whilst we are doing a fetch.
    */
   public void writeAllFetchWarnings(Session sessObj, SQLWarning sqlw,
                                     int numCols) throws IOException {

      /*  Get the markup being currently used for the display.  */
      int outputMarkup = sessObj.getDisplayFormat();


      /*
       *  Print starting tags.
       */
      if (SessionDefaults.XML_FORMAT == outputMarkup)
         sessObj.getXmlObj().startFetchWarningListTag();
      else if (SessionDefaults.HTML_FORMAT == outputMarkup)
         sessObj.getHtmlObj().startFetchWarningListTable(numCols);


      /*
       *  Loop thru' all the warnings.
       */
      while (null != sqlw) {
         String errStr = formatErrStr(sqlw.toString(), sessObj);

         switch(outputMarkup) {
            case SessionDefaults.XML_FORMAT  : 
            case SessionDefaults.HTML_FORMAT :
                 ErrorObject eo = new ErrorObject(parser.getErrorCode(errStr,sqlw.getErrorCode()),
                                                  errStr, 'W');
                 if (SessionDefaults.XML_FORMAT == outputMarkup)
                    sessObj.getXmlObj().handleFetchWarnings(eo);
                 else
                    sessObj.getHtmlObj().handleFetchWarnings(eo);

                 break;

            default:
                writeln(errStr);
                break;

         }  /*  End of  switch on display output markup.  */


         /*  Get the next warning in this warning chain.  */
         sqlw=sqlw.getNextWarning();


      }  /*  End of  WHILE  there are more warnings to process.  */
 

      /*
       *  Print the ending tags.
       */
      if (SessionDefaults.XML_FORMAT == outputMarkup)
         sessObj.getXmlObj().endFetchWarningListTag();
      else if (SessionDefaults.HTML_FORMAT == outputMarkup)
         sessObj.getHtmlObj().endFetchWarningListTable();

   }   /*  End of  writeAllFetchWarnings  method.  */

   public void writeAllSQLWarnings(Session sessObj, SQLWarning sqlw) throws IOException
   {
      while ((sqlw!=null))
      {
			if (sessObj.getDisplayFormat() == SessionDefaults.XML_FORMAT)
				sessObj.getXmlObj().handleWarnings(
						new ErrorObject(parser.getErrorCode(
								formatErrStr(sqlw.toString(), sessObj),
								sqlw.getErrorCode()),
								formatErrStr(sqlw.toString(), sessObj), 'W'));
			else if (sessObj.getDisplayFormat() == SessionDefaults.HTML_FORMAT)
				sessObj.getHtmlObj().handleWarnings(
						new ErrorObject(parser.getErrorCode(
								formatErrStr(sqlw.toString(), sessObj),
								sqlw.getErrorCode()),
								formatErrStr(sqlw.toString(), sessObj), 'W'));
			else
				writeln(formatErrStr(sqlw.toString(), sessObj));

         sqlw=sqlw.getNextWarning();
      }
   }

   public void writeSyntaxError(Session sessObj, String qryString, String remainderString) throws IOException
   {
      if (ise == null)
      {
         ise=new InterfaceSyntaxError();
      }
      writeInterfaceErrors(sessObj, ise.getSyntaxError(qryString,remainderString));
   }

   public void writeConditionalSyntaxError(Session sessObj, String qryString) throws IOException
   {
       if(cse == null )
       {
           cse=new ConditionalSyntaxError();
       }
       writeInterfaceErrors(sessObj, cse.getSyntaxError(qryString));
   }
   
   public void writeln(String line) throws IOException
   {
      write(line);
      writeln();
   }

   private String formatErrStr(String errStr, Session sessObj)
   {

	if ((errStr.indexOf("org.trafodion.jdbc.t4") != -1)
				|| (errStr.indexOf("java.sql") != -1)
				|| (errStr.indexOf("org.trafodion.jdbc.t4.TrafT4") != -1))
      {
         if (errStr.indexOf(":")!=-1)
         {
            errStr =errStr.substring(errStr.indexOf(":")+1).trim();
         }
         else
         {
            return errStr;
         }
      }
     
      return errStr;
   }

   public void writeElapsedTime(String time) throws IOException
   {
      writeln();
      writeln("Elapsed: "+ time);
   }

   public ConsoleWriter getConsoleWriter()
   {
      return cWriter;
   }
   public void setConsoleWriter(ConsoleWriter writer)
   {
      cWriter = writer;
   }
   public FileWriter getSpoolWriter()
   {
      return sWriter;
   }
   public void setSpoolWriter(FileWriter sWriter)
   {
      this.sWriter = sWriter;
   }
   public FileWriter getLogWriter()
   {
      return lWriter;
   }
   public void setLogWriter(FileWriter lWriter)
   {
      this.lWriter = lWriter;
   }
   public int getWriterMode()
   {
      return this.writerMode;
   }
   public void setWriterMode(int writerMode)
   {
      this.writerMode = writerMode;
   }

 public void writeError(Session sessObj, ErrorObject errObj) throws IOException
     {
      this.writeInterfaceErrors(sessObj, errObj);
   }
      
   public void writeError(Session sessObj, char errType,ErrorObject errObj, String[] values) throws IOException
   {
	     String errStr = errObj.errorMessage();
	      for (int i=0;i<values.length;i++)
	    	  errStr = errStr.replaceFirst("%s",values[i].toString());
	      this.writeError(sessObj, new ErrorObject(errObj.errorCode(), errStr));
   }

   public void writeExecutionTime(Session sessObj, Utils utilObj, Writer writer) throws IOException
   {
      String qryExecTime= "";
      qryExecTime = " *** Total Query time was " + utilObj.getTimeinWords(sessObj.getQryExecEndTime());

      if (sessObj.getDisplayFormat() == SessionDefaults.HTML_FORMAT)
         sessObj.getHtmlObj().handleQryExecutionTime(qryExecTime);
      else if (sessObj.getDisplayFormat() == SessionDefaults.XML_FORMAT)
         sessObj.getXmlObj().handleQryExecutionTime(qryExecTime);
      else if (sessObj.getDisplayFormat() == SessionDefaults.RAW_FORMAT)
         writer.writeln(qryExecTime+SessionDefaults.lineSeperator);
   }

   public void writeStatusMsg(Session sessObj, Query qryObj, Utils utilObj, Writer writer) throws IOException
   {
       String elapsedTimeMsg = getElapsedTime(sessObj,qryObj, utilObj);
       String statusMsg = sessObj.getLfProps().getStatus(qryObj) + SessionDefaults.lineSeperator;
       writeStatusMsg(sessObj, statusMsg, elapsedTimeMsg, writer);
   }

   public void writeStatusMsg(Session sessObj, String statusMsg, String elapsedTimeMsg, Writer writer) throws IOException
   {
      if (sessObj.getDisplayFormat() == SessionDefaults.HTML_FORMAT)
         sessObj.getHtmlObj().processHtml(elapsedTimeMsg);
      else if (sessObj.getDisplayFormat() == SessionDefaults.XML_FORMAT)
      {
         sessObj.getXmlObj().processXml("",elapsedTimeMsg);
      }
      else if (sessObj.getDisplayFormat() == SessionDefaults.RAW_FORMAT)
      {
         if (null != statusMsg)
            writer.write(statusMsg);
         if (sessObj.isSessionTimingOn())
         {
            if (null != statusMsg)
               writer.writeln(); 
            writer.writeln(elapsedTimeMsg);
            
            writer.printElapsedQuietMode(elapsedTimeMsg);
         }
      }

   }

   public String getElapsedTime(Session sessObj,Query qryObj,Utils utilObj)
   {
      String elapsedTimeMsg="";
      if (sessObj.isSessionTimingOn())
      {
            elapsedTimeMsg =  "Elapsed: " + utilObj.millisecondsToString(qryObj.getElapsedTime());
      }
      return elapsedTimeMsg;
   }
   
   public void printElapsedQuietMode(String elapsedTimeMsg){
       if (sessObj.isSessionTimingOn())
       {
           if(writerMode == SessionDefaults.CONSOLE_SPOOL_WRITE_MODE && 
                    sessObj.isQuietEnabled() && sessObj.getCaller() != SessionDefaults.PRUNI)
           {
               try{
            	     sessObj.setQuietEnabled(false);
                   writeln(); 
                   this.getConsoleWriter().println(elapsedTimeMsg);
               }catch(IOException ioex){
                   ;
               }
               sessObj.setQuietEnabled(true);
           }
       }
   }

   // Print end tags if MARKUP is XML or HTML
   public void writeEndTags (Session sessObj) throws IOException
   {
      if (sessObj.getDisplayFormat() == SessionDefaults.XML_FORMAT)
         sessObj.getXmlObj().handleEndTags();
      if (sessObj.getDisplayFormat() == SessionDefaults.HTML_FORMAT)
         sessObj.getHtmlObj().handleEndTags();
   }

   //called when an SQL query returns an error.
   public void writeInterfaceErrors(Session sessObj, String errorStr) throws IOException
   {
	   writeInterfaceErrors(sessObj,new ErrorObject(errorStr));
   }

   // called when a single interface error occurs.
   public void writeInterfaceErrors(Session sessObj, ErrorObject errorObj) throws IOException
   {
      writeInterfaceError(sessObj, errorObj);
      //Catalog API calls - print the end tags in InterfaceQuery 
      int errorNo = Integer.parseInt(errorObj.errorCode());
      if (!(errorNo >= 29416 && errorNo <= 29423))
         writeEndTags(sessObj);
   }

   // called once for each error message in a series of errors.
   public void writeInterfaceError(Session sessObj, ErrorObject errorObj) throws IOException
   {
      switch (sessObj.getDisplayFormat())
      {
         case SessionDefaults.HTML_FORMAT:
         sessObj.getHtmlObj().handleErrors(errorObj);
         break;
      case SessionDefaults.XML_FORMAT: 
         sessObj.getXmlObj().handleErrors(errorObj);
         break;
      default:
         writeln(errorObj.RAWOutputError());
         break;
      }
      
     if (!errorObj.errorCode().equalsIgnoreCase(Parser.UNKNOWN_ERROR_CODE))
         sessObj.setLastError(Integer.parseInt(errorObj.errorCode()));
   }
}



