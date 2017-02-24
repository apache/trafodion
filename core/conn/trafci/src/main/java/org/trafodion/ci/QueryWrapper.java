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

import java.awt.FontMetrics;
import java.io.IOException;
import java.sql.CallableStatement;
import java.sql.Connection;
import java.sql.ParameterMetaData;
import java.sql.PreparedStatement;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.Statement;
import java.sql.Types;

import org.trafodion.jdbc.t4.TrafT4Statement;



import sun.misc.Signal;
import sun.misc.SignalHandler;

/**
 * 
 * 
 * Abstract class for executing queries and formatting the result sets.
 *
 */

public abstract class QueryWrapper
{

   Session sessObj=null;
   Writer writer=null;
   Reader reader=null;
   Query qryObj=null;
   String queryStr=null;
   Parser parser=null;
   Connection conn=null;
   Statement stmt=null;
   Utils utils=null;
   UnKnownInterfaceCommand uic=null;
   InterfaceSyntaxError ise=null;
   ConditionalQueryException cqe=null;
   ConditionalSyntaxError cse=null;
   UserInterruption ui=null;
   boolean isMultiLine=true;
   boolean blankLiner=true;
   SignalHandler CTRLCHandler=null;
   Signal INTSignal=null;
   int i = 0;
   HTMLObject htmlObj = null;
   XMLObject xmlObj = null;
   String columnName="";
   int colCount=1;
   String crTrigTerminator ="/";
   FontMetrics fontMetrics = null;
   protected int isoMapping;
   boolean useGetUpdateCount64 = true;
   // false-show complete msg, true-no show
   public boolean showStatusMsg = false;
   
   int[] colSize   ={};
   int[] colAlign  ={};
   String[] columnNameArray={};   
   
   /**
   * creates an empty constructor.
   *
   */
   QueryWrapper()
   {

   }

   /**
   * creates the query wrapper for t
   */
   QueryWrapper(Session sessObj)
   {
      this.sessObj=sessObj;
      this.parser=new Parser();
      this.utils=new Utils();
      this.uic=new UnKnownInterfaceCommand();
      this.ise=new InterfaceSyntaxError();
      this.cqe=new ConditionalQueryException();
      this.cse=new ConditionalSyntaxError();
      this.ui=new UserInterruption();
      if (sessObj.getCaller() != SessionDefaults.PRUNI)
      {
         CTRLCHandler =new SignalHandler ()
         {
            public void handle(Signal sig)
            {
               cancelQuery();
            }
         };
         try {
         INTSignal=new Signal("INT");
         } catch (Exception e) {}
      }
      
      fontMetrics = MultiByteUtils.getFontMetrics();
      isoMapping = sessObj.getISOMapping();
    
      
   }


   /**
   * Initializes the reader and writer objects
   *
   */
   void init()
   {
      this.writer=sessObj.getWriter();
      this.reader=sessObj.getReader();
      this.qryObj=sessObj.getQuery();
      this.conn=sessObj.getConnObj();
      this.stmt=sessObj.getStmtObj();
      try {
         if (CTRLCHandler != null)
         {
            Signal.handle(INTSignal, CTRLCHandler);
         }
      }catch (Exception e) {}
      sessObj.setCurrentStmtObj(null);

   }



   /**
   * Reads the query string from the console or from a file depending on the 
   * read mode.
   * @throws IOException
   * @throws UserInterruption 
   */
   void readQuery() throws IOException, UserInterruption
   {
      boolean matchTrigStmt=false;
      String[] qryLineArr = null;
      //queryStr=qryObj.getQueryText().trim();
      queryStr=qryObj.getQueryText();

      try {
         if (CTRLCHandler != null)
         {
            Signal.handle(INTSignal, CTRLCHandler);
         }
      }catch (Exception e) {}

      matchTrigStmt = isCreateTriggerCmd(qryObj.getQueryText());
      //This is needed if user does a repeat
      //on create trigger
      if ((matchTrigStmt) && (queryStr.indexOf(SessionDefaults.lineSeperator) != -1))
      {
         qryLineArr = queryStr.split(SessionDefaults.lineSeperator);
         if (qryLineArr[qryLineArr.length-1].equals(crTrigTerminator))
         {
            isMultiLine = false;
         }
      }

      while (isMultiLine)
      {
         if ((matchTrigStmt)&& (utils.rtrim(queryStr).equals(crTrigTerminator)))
            break;
         else if ((!matchTrigStmt) && (queryStr.trim().toUpperCase().endsWith(sessObj.getSessionSQLTerminator())))
            break;
         if (( (writer.getWriterMode() == SessionDefaults.CONSOLE_WRITE_MODE ||
            writer.getWriterMode() == SessionDefaults.CONSOLE_SPOOL_WRITE_MODE))  )
         {
            if((reader.getReadMode() == SessionDefaults.OBEY_READ_MODE  && !sessObj.isQuietEnabled()) || !(reader.getReadMode() == SessionDefaults.OBEY_READ_MODE) )            
                writer.getConsoleWriter().print(sessObj.getSessionCprompt());
         }
         try
         {
           
            queryStr = reader.getNonBlankLine();
            if (sessObj.isCmdEchoEnabled())
               writer.getConsoleWriter().println(queryStr);
         }
         catch (UserInterruption ui)
         {
            sessObj.setQuery(null);
            sessObj.setQueryInterrupted(false);
            throw ui;
         }

         if ((queryStr == null))
         {
            if (matchTrigStmt)
               queryStr = crTrigTerminator;
            else
               queryStr=sessObj.getSessionSQLTerminator();
         }

         if (writer.getWriterMode() == SessionDefaults.LOG_WRITE_MODE)
            writer.getLogWriter().writeln(queryStr);
         else if (reader.getReadMode() == SessionDefaults.CONSOLE_READ_MODE && writer.getWriterMode() == SessionDefaults.CONSOLE_SPOOL_WRITE_MODE)
         {
            if (sessObj.isLogCmdText())
               writer.getSpoolWriter().writeln(queryStr);
         }else if ((reader.getReadMode() == SessionDefaults.OBEY_READ_MODE ) || ( sessObj.getCaller() !=SessionDefaults.PRUNI && reader.getReadMode() == SessionDefaults.SCRIPT_READ_MODE ))
         {
            writer.getConsoleWriter().println(queryStr);
            if (sessObj.isLogCmdText() && writer.getWriterMode() == SessionDefaults.CONSOLE_SPOOL_WRITE_MODE)
               writer.getSpoolWriter().writeln(queryStr);
         }

         queryStr=parser.ignoreComments(queryStr);
         qryObj.setQueryText(SessionDefaults.lineSeperator+queryStr);
         matchTrigStmt = isCreateTriggerCmd(qryObj.getQueryText());

         //queryStr=queryStr.trim();
      }
      sessObj.setTimerHold();
      if ((blankLiner) && (sessObj.getDisplayFormat() != SessionDefaults.XML_FORMAT))
      {
          writer.writeln();
      }
      if (matchTrigStmt)
      {
         this.queryStr = Utils.trimSQLTerminator(sessObj.getQuery().getQueryText().trim(),"/");
         isMultiLine = true;
      }
      else
         this.queryStr = Utils.trimSQLTerminator(sessObj.getQuery().getQueryText().trim(),sessObj.getSessionSQLTerminator());
      
      this.queryStr = utils.rtrim(this.queryStr);
      if (qryObj.getQueryType()==SessionDefaults.SQLQ)
         sessObj.setPrevSQLQuery(this.queryStr);
      parser.setRemainderStr(null);
      parser.setRemainderStr(this.queryStr);

      this.fixPopIndxUpdStatsCmds();
   }
   
   protected void setQueryRowCount(Statement theStmt) throws SQLException
   {
       if(!useGetUpdateCount64){
           qryObj.setRowCount(String.valueOf(theStmt.getUpdateCount()));
           return;
       }
           
       try
       {
           qryObj.setRowCount(String.valueOf(((TrafT4Statement)theStmt).getUpdateCount64())); 
       }
       catch(NoSuchMethodError nsme)
       {
           // for backwards compatibility
           useGetUpdateCount64 = false;
           qryObj.setRowCount(String.valueOf(theStmt.getUpdateCount()));
       }
    }

   boolean isCreateTriggerCmd(String qryText)
   {
      if (qryText.toUpperCase().matches("(?s)^\\s*CREATE\\s+TRIGGER\\s+(.*)"))
      {
         return true;
      }
      return false;
   }

   void fixPopIndxUpdStatsCmds()
   {
      if ((qryObj.getQueryText().toUpperCase().matches("(?s)^\\s*POPULATE\\s+INDEX\\s+(.*)"))  
         )
      {
         queryStr = queryStr.replaceAll(SessionDefaults.lineSeperator," ");
      }
   }
   
   /**
   * cancels the current query if ctrl + C is pressed. 
   * 
   */
   void cancelQuery()
   {
      sessObj.setQueryInterrupted(true);
      if (sessObj.getCurrentStmtObj() != null)
      {
         try
         {
            ((Statement)sessObj.getCurrentStmtObj()).cancel();
              sessObj.setDBConnExists(false);
              if (sessObj.isDotModeCmd() ){
            	sessObj.setLogCmdEcho(true);
            	sessObj.setMode(sessObj.getPrevMode());
              }
         } catch (SQLException e){}
      }
      
   }



   public abstract void execute() throws IOException, SQLException, UnKnownInterfaceCommand, UserInterruption, ConditionalQueryException ;

   boolean dbExecute(Object st ) throws SQLException, IOException
   {
	   boolean status=false;

      try {
         if (CTRLCHandler != null)
         {
            Signal.handle(INTSignal, CTRLCHandler);
         }
      }catch (Exception e) {}

      try
      {

         sessObj.setCurrentStmtObj(st);
         sessObj.setQueryInterrupted(false);

         if (st instanceof CallableStatement)
         {
            status=((CallableStatement)st).execute();
            sessObj.setSPJRS(status);
         }else if (st instanceof PreparedStatement)
         {
            status=((PreparedStatement)st).execute();
         }
         else if (st instanceof Statement)
         {
            status=((Statement)st).execute(queryStr);
         }

      }catch (SQLException sqle)
      {
         if (sessObj.isQueryInterrupted() && sqle.getErrorCode() == -29157)
         {
            sessObj.setDBConnExists(false);

         }
         throw sqle;
      }
	   sessObj.setQryExecEndTime();
	   
      return status;
   }
   
   boolean dbExec(Object st) throws SQLException, IOException
   {
	   boolean status = dbExecute(st);
	   
	   /*if (sessObj.getMode() == SessionDefaults.SQL_MODE &&
	         writer != null &&
	         qryObj.getQueryType() != SessionDefaults.IQ)
	         writer.writeExecutionTime(sessObj, utils, writer);
           */	   
	   return status;
   }

   void writeSeparator() throws IOException
   {
      if (sessObj.getSessView() == SessionDefaults.MXCI_VIEW)
         write(sessObj.getSessionColSep());
      write(sessObj.getSessionColSep());
   }

   void write(String output) throws IOException
   {
      switch (sessObj.getDisplayFormat())
      {
         //HTML Format
         case SessionDefaults.HTML_FORMAT:
            sessObj.getHtmlObj().processHtml(output);
            writer.writeln();
            break;

         //XML Format
         case SessionDefaults.XML_FORMAT:
            sessObj.getXmlObj().processXml(this.columnName,output);
            writer.writeln();
            break;

         //CSV Format
         case SessionDefaults.CSV_FORMAT:
            writer.write(output);
            if (qryObj.getColCount() != null)
            {
               if (colCount == Integer.parseInt(qryObj.getColCount()))
               {
                  writer.write(SessionDefaults.lineSeperator);
                  colCount=1;
                  break;
               }
            }
            colCount++;
            if (sessObj.getStrDisplayFormat().equalsIgnoreCase("COLSEP"))
               writer.write(sessObj.getSessionColSep());
            else
               writer.write(",");
            break;

         //Default Raw Format
         default: writer.write(output);
            break;
      }
   }


   void writeln(String output) throws IOException
   {
      write(output);
      write(SessionDefaults.lineSeperator);
   }

   void writeln() throws IOException
   {
      write(SessionDefaults.lineSeperator);
   }

   void formatOutput(String output,int colWidth,char fillchar,int align) throws IOException
   {

     int outputStrLen = 0;
      
      if (output != null && output.length() > colWidth)
      {
         output=output.substring(0,colWidth);
      } 
      StringBuffer outBuffer=new StringBuffer(output);

      if (sessObj.isMultiByteAlign() && (isoMapping == 10 || isoMapping == 15)) //SJIS or UTF8
      {
         try {
         outputStrLen = getMultiByteColWidth(outBuffer);
         }catch (Exception ex)
         {
            ex.printStackTrace();
            throw new IOException(ex.getMessage());
         }
      }
      else
      {
         outputStrLen = outBuffer.length();
      }
      //If the output is greater than the column width,
      //truncate the output
      
      if (outputStrLen > colWidth)
      {
         outBuffer = new StringBuffer(outBuffer.substring(0,Math.min(colWidth, outBuffer.length())));
         outputStrLen = outBuffer.length();
      }

      if ( outputStrLen <= colWidth)
      {
    	 if (colWidth > 128)
    		 colWidth = 128;
         for (int i=outputStrLen;i<colWidth;i++)
         {
            if (align == 0)
            {
               outBuffer.append(fillchar);
            }
            else
            {
               outBuffer.insert(0,fillchar);
            }
         }

         write( outBuffer.toString());

      }
      outBuffer=null;
   }

   void formatHtmlOutput(String output) throws IOException
   {
      write(output);
   }

   void formatXmlOutput(String columnName, String output) throws IOException
   {
      this.columnName = columnName;
      write(output);
   }

   void formatCsvOutput(String output) throws IOException
   {
      write(output);
   }

   protected int getMultiByteColWidth(StringBuffer output)
   {
      if (null == fontMetrics)
         return output.length();
      
      int refWidth = fontMetrics.charWidth('w');
      int width=0;
     
       for (int i=0; i< output.length(); i++)
      {
         int tmpLen=fontMetrics.charWidth(output.charAt(i));
         //incorrect value of char width. Treat it as no-ASCII.
         if(tmpLen==0 || tmpLen>fontMetrics.getMaxAdvance())
         {
        	 if(output.charAt(i)>256)
        	 {
        		 tmpLen=refWidth*2-1;
        	 }
        	 else
        	 {
        		 tmpLen=refWidth;
        	 }
         } 
         
         width += (tmpLen/refWidth) + (tmpLen%refWidth>0?1:0);
      }
      
      return width;
   }

   void writeQryOutParams(ResultSetMetaData rsmd,int numColumns,ParameterMetaData paramMetaData,CallableStatement callStmt) throws SQLException, IOException
   {
	   //   sessObj.setWriteParams(true);
	      columnNameArray = new String[numColumns];
	      colSize=new int[numColumns];
	      colAlign=new int[numColumns];
	      //qryObj.setColCount(String.valueOf(numColumns));
	      for (int i=1;i<=numColumns;i++)
	      {
	         if (paramMetaData.getParameterMode(i) != ParameterMetaData.parameterModeIn &&
	            paramMetaData.getParameterMode(i) != ParameterMetaData.parameterModeUnknown)
	         {
	            int colNameSize = rsmd.getColumnName(i).length();
	            if (colNameSize > rsmd.getColumnDisplaySize(i))
	               colSize[i-1]=colNameSize;
	            else
	               colSize[i-1]=rsmd.getColumnDisplaySize(i);

	            if (colSize[i-1] < SessionDefaults.MIN_COL_DISPLAY_SIZE)
	            {
	               colSize[i-1] = SessionDefaults.MIN_COL_DISPLAY_SIZE;
	            }

	            switch (sessObj.getDisplayFormat())
	            {
	               case SessionDefaults.RAW_FORMAT:
	                  formatOutput(rsmd.getColumnName(i),colSize[i-1],' ',0);
	                  if (i < numColumns)
	                  {
	                     writeSeparator();
	                  }
	                  break;

	               case SessionDefaults.XML_FORMAT:
	                  columnNameArray[i-1] = sessObj.getXmlObj().checkColumnNames(rsmd.getColumnName(i));
	                  break;

	               case SessionDefaults.HTML_FORMAT:
	                  formatHtmlOutput(rsmd.getColumnName(i));
	                  break;
	               case SessionDefaults.CSV_FORMAT:
	            	  formatCsvOutput(rsmd.getColumnName(i));
	                  break;

	               default:
	                  break;
	            }

	            switch (rsmd.getColumnType(i))
	            {
	               case Types.BIGINT:
	               case Types.BIT:
	               case Types.DECIMAL:
	               case Types.DOUBLE:
	               case Types.FLOAT:
	               case Types.INTEGER:
	               case Types.NUMERIC:
	               case Types.REAL:
	               case Types.SMALLINT:
	               case Types.TINYINT:
	                  colAlign[i-1]=1;
	                  break;
	               default:
	                  colAlign[i-1]=0;
	                  break;

	            }
	         }
	      }
	      if (sessObj.getDisplayFormat() == SessionDefaults.RAW_FORMAT)
	      {
	         writeln();
	         for (int i=1;i<=numColumns ;i++)
	         {
	            if (paramMetaData.getParameterMode(i) != ParameterMetaData.parameterModeIn &&
	               paramMetaData.getParameterMode(i) != ParameterMetaData.parameterModeUnknown)
	            {
	               formatOutput("",colSize[i-1],'-',0);
	               if (i < numColumns)
	               {
	                  writeSeparator();
	               }
	            }
	         }
	         writeln();

	         if (sessObj.getSessView() == SessionDefaults.MXCI_VIEW)
	            writeln();
	      }
	      writeQryData(callStmt,numColumns,paramMetaData);
	      qryObj.setRowCount("1");		      
   }

   private void writeQryData(CallableStatement callStmt,int numColumns, ParameterMetaData paramMetaData) throws SQLException, IOException
   {
      for (int i=1;i<=numColumns ;i++)
      {
         if (paramMetaData.getParameterMode(i) != ParameterMetaData.parameterModeIn &&
            paramMetaData.getParameterMode(i) != ParameterMetaData.parameterModeUnknown)
         {
            String value=callStmt.getString(i);
            if (value == null)
            {
               value=sessObj.getSessNull();
            }

            switch (sessObj.getDisplayFormat())
            {
               case SessionDefaults.RAW_FORMAT :
                  if (qryObj.isTrimOut())
                  {
                     formatOutput(value,value.length(),' ',0);
                  }
                  else
                  {
                     formatOutput(value,colSize[i-1],' ',colAlign[i-1]);
                  }

                  if (i < numColumns)
                  {
                     writeSeparator();
                  }
                  break;


               case SessionDefaults.XML_FORMAT :
                  formatXmlOutput(columnNameArray[i-1],value);
                  break;

               case SessionDefaults.HTML_FORMAT:
                  formatHtmlOutput(value);
                  break;

               case SessionDefaults.CSV_FORMAT:
                  formatCsvOutput(value.trim());
                  break;
            }
         }
      }

      if (sessObj.getDisplayFormat() == SessionDefaults.RAW_FORMAT)
         writeln();
   }
   
   void formatOutputVertical(String colHeading, String output) throws IOException
   {
	   if (colHeading != null)
		   write( colHeading + ": " + output + SessionDefaults.lineSeperator);
	   else
		   write( output + SessionDefaults.lineSeperator);
   }
   
	public boolean getShowStatusMsg() {
		return this.showStatusMsg;
	}

	public void setShowStatusMsg(boolean showStatusMsg) {
		this.showStatusMsg = showStatusMsg;
	}
}


