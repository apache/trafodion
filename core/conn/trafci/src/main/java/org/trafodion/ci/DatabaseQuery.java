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
import java.io.InputStream;
import java.sql.ParameterMetaData;
import java.sql.ResultSet;
import java.sql.ResultSetMetaData;
import java.sql.SQLException;
import java.sql.SQLWarning;
import java.sql.Types;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.sql.PreparedStatement;
import org.trafodion.jdbc.t4.TrafT4Connection;
import org.trafodion.jdbc.t4.TrafT4Statement;
import java.sql.CallableStatement;


public class DatabaseQuery extends QueryWrapper
{

   private HashMap<String,String> dbKeyMap=null;
   private int[] colSize={};
   private int[] colAlign={};
   private List<String> procParamList=null;
   CallableStatement cStmt =null;
   String[] columnNameArray={};
   boolean getStatsDone = false;
   boolean implicitQry  = false;
   String qryText = null;
   String getStatsCmd = "get statistics";
   String getStatsNcCmd = "get statistics, options 'nc'";
   public ResultSet rsObj = null;
   String userQry = null;
   boolean userQryTrimOut = false;
   
   //Create a pattern for matching parameter names
   private static Pattern queryParamNamesPattern =
	   Pattern.compile("^(.*)?([=\\+\\-~( ]\\s*\\?)([a-zA-Z0-9_]*)(.*)",Pattern.MULTILINE);

   // this string array constants should match the cases grouped in ValidateQuery object.
   // All commands beginning with SHOW have the trimOut flag set to true in the default case
   // of executeInternal method. 
   
   private final String[] trimKeysList={ "INVOKE",
                                "SHOWDDL",
                                "SHOWSHAPE",
                                "SHOWCONTROL",
                                "SHOWTRANSACTION",
                                "REORG",
                                "REORGANIZE",
                                "MAINTAIN",
                                "REPLICATE",
                                "SHOWLABEL",
                                "SHOWPLAN",
                                "EXPLAIN",
                                "GET"
                                       };

   HashMap<Integer,String> inOutParamMap=null;
   DatabaseQuery()
   {

   }

   DatabaseQuery(Session sessObj)
   {
      super(sessObj);
      loadDbKeyWordMap();
   }
   
   public void execute() throws IOException, SQLException, UnKnownInterfaceCommand, UserInterruption
   {
	   try{
           executeInternal();
       }catch(IOException ioe){
           throw ioe;
       }catch(SQLException sqlex){
           throw sqlex;
       }catch(UnKnownInterfaceCommand uic){
           throw uic;
       }catch(UserInterruption ui){
           throw ui;
       }catch(Exception ex){
           if(Boolean.getBoolean("trafci.enableTrace"))
             ex.printStackTrace();
           
           ErrorObject internalError = new ErrorObject(SessionError.INTERNAL_ERR.errorCode(), 
                   SessionError.INTERNAL_ERR.errorMessage() + " Details=" + ex.getMessage());
           
           writer.writeError(sessObj,internalError);
                  
           throw new UnKnownInterfaceCommand();
       }
   }
   
   public void executeInternal() throws IOException, SQLException, UnKnownInterfaceCommand, UserInterruption
   {
	   
      String stmtName=null;
      String cmdToken=null;
      getStatsDone=false;
      sessObj.setImplicitGetStatsQry(false);

      init();

      qryObj.resetRowCount();
      qryObj.setStmtType(null);

      readQuery();
  
      /* Check for pattern*/
      String key,value=null;
      boolean matchedPattern = false;
      
      for (int i=0;i < sessObj.getMaxPatternDepth() ; i++)
      {
    	  //Look for $$KEY$$ pattern
    	  Matcher mat = parser.getKeyPatternMatcher(queryStr);
    	  StringBuffer sb = new StringBuffer();
    	  while (mat.find()) {
    		  key = mat.group(2);
    		  value = sessObj.getPatternValue(key);
			  if (value!=null)
			  {
				  key = "$$" + key + "$$";
			      key = key.replaceAll("\\$","\\\\\\$"); // Every $ needs to be replaced with \$. Do not delete the backslashes.se
			      if (mat.group(1)!=null)
			    	  mat.appendReplacement(sb,mat.group(1) + value);
			      else
			    	  mat.appendReplacement(sb, value);
			      matchedPattern = true;
		      }
    	  }
    	  
    	  if (matchedPattern) 
    	  {
    		  mat.appendTail(sb);
    		  queryStr = sb.toString();
    		  parser.setRemainderStr(queryStr);
    	  }
    	 
	      //Look for regular expression pattern
    	  HashMap<String,String> regExpMap = sessObj.getRegExpMap();
    	  if (regExpMap != null )
    	  {
    		  Iterator<String> regExpMapIt = regExpMap.keySet().iterator();
    		  String regExpPat = null;
    		  while (regExpMapIt.hasNext())
    		  {
    			  regExpPat =  (String)regExpMapIt.next();
    			  if (sessObj.isDebugOn())
                	  System.out.println(this.getClass().getName() +  ":: regExpPat :"+ regExpPat);
    			  if (queryStr.matches(regExpPat))
    			  {
    				  matchedPattern = true;
    				  queryStr = queryStr.replaceFirst(regExpPat,regExpMap.get(regExpPat).toString());
    			  }
    		  }
	    	  
	      }//End IF-ELSE
    	  qryObj.setQueryId(sessObj.getVQryObj().getQueryId(queryStr));
    	  parser.setRemainderStr(queryStr);
      }//End For

      if (sessObj.isDebugOn())
    	  System.out.println(this.getClass().getName() +  ":: QueryStr :"+ queryStr);
      
      /* Commenting this for now until I fix all the issues for interface commands */
      /*
      if ((matchedPattern) && dbKeyMap.get(sessObj.getVQryObj().getFirstToken(queryStr))==null) {
    	  InputStream theStream = new ByteArrayInputStream(queryStr.getBytes());
    	  sessObj.setLogCmdEcho(false);
    	  sessObj.iQryObj.execObeyStream(theStream, "Pattern Replacement");
    	  return;
      }
      */

      switch (qryObj.getQueryId())
      {

         // the first key word is set
         case SessionDefaults.SET:
            parser.getNextKeyToken();
            execSet();
            break;

         case SessionDefaults.PREPARE:
            parsePrepareStmt();
            break;

         case SessionDefaults.EXECUTE:
            bind();
            break;

         case SessionDefaults.CALL:
            procParamList = getParamNames(parser.getRemainderStr());
            if (sessObj.isSessionAutoPrepare())
            {
               prepare(parser.getRemainderStr(), sessObj.getAutoPrepStmtName());
               executeProcedure(procParamList,false);
            }
            else
            {
               prepareCallStmt(null);
               executeProcedure(procParamList,true);
            }

            break;

         case SessionDefaults.INFOSTATS:
            cmdToken = parser.getNextKeyToken();
            stmtName = parser.getNextValueToken();
            if (cmdToken != null && (stmtName !=null && !stmtName.trim().equals("")))
            {
               stmtName = "\"" + stmtName + "\"";
               if (parser.getRemainderStr() != null)
               {
                  queryStr=cmdToken + " " + stmtName + parser.getRemainderStr();
               }
               else
               {
                  queryStr=cmdToken + " " + stmtName ;
               }

            }
            executeQuery(true);
            break;

         case SessionDefaults.GET:
            execGet(true);
            break;
            
         case SessionDefaults.CONTROL:
             execControl();
             break;
            
         default:
            
            //For commands beginning with SHOW (SHOWTRANSACTION,SHOWSET)
            //set the trimout flag to true
            if (qryObj.getQueryText().matches("(?is)SHOW.+"))
               qryObj.setTrimOut(true);
            if (sessObj.isSessionAutoPrepare())
            {
               prepare(parser.getRemainderStr(),sessObj.getAutoPrepStmtName());
               parser.setRemainderStr("EXECUTE " + sessObj.getAutoPrepStmtName());
               bind();
            }
            else
            {
               executeQuery(true);
            }
      }

      /*
      send an implicit GET STATISTICS command to
      display the statistics for the previously
      executed SQL statement. This is done only
      when statistics in enabled via a SET STATISTICS
      ON command.
      */
      if (sessObj.isSessionStatsEnabled() && !getStatsDone && !blockGetStats())
      {
         boolean oldstate = sessObj.isSessionTimingOn();
         /* Store the query and reset the query text with this
          * value once get statistics is executed. This is needed to
          * avoid get statistics to be added in history.
          */
         userQry = qryObj.getQueryText();
         userQryTrimOut = qryObj.isTrimOut();
         sessObj.setImplicitGetStatsQry(true);
         try 
         {
            sessObj.setSessionTiming(false);
            writer.writeStatusMsg( sessObj,  qryObj,  utils,  writer);
            writer.writeln();
         } finally 
         {
            sessObj.setSessionTiming(oldstate);
         }

         //Overwrite the queryStr with the 'GET STATISTICS'
         //command. dbExec executes the command stored in
         //queryStr.
         String qryRowCnt = qryObj.getRowCount();
         if (sessObj.getSessionStatsType().equals("ALL"))
         {
            queryStr = "GET STATISTICS FOR QID CURRENT PROGRESS, OPTIONS 'SL'";
            qryObj.resetQueryText(queryStr);
            parser.setRemainderStr(queryStr);
            qryObj.setRowCount(null);
            execGet(false);
         }
         if (sessObj.getSessionStatsType().equals("PERTABLE"))
            queryStr = "GET STATISTICS FOR QID CURRENT PERTABLE, OPTIONS 'SL'";
         else if (sessObj.getSessionStatsType().equals("PROGRESS"))
            queryStr = "GET STATISTICS FOR QID CURRENT PROGRESS, OPTIONS 'SL'";
         else if (sessObj.getSessionStatsType().equals("DEFAULT"))
            queryStr = "GET STATISTICS FOR QID CURRENT DEFAULT";
         else if (sessObj.getSessionStatsType().equals("ALL"))
            queryStr = "GET STATISTICS FOR QID CURRENT DEFAULT";
         else
            queryStr = getStatsCmd;
         qryObj.resetQueryText(queryStr);
         parser.setRemainderStr(queryStr);
         qryObj.setRowCount(null);
         execGet(false);
         //Reset the record count to row count of the
         //original query
         qryObj.setRowCount(qryRowCnt);
      }

   }

   private void parsePrepareStmt() throws IOException, SQLWarning, SQLException, UnKnownInterfaceCommand
   {
//      PreparedStatement pStmt=null;
      String stmtName=null;
      String query=null;
      String fromKeyToken=null;

      parser.getNextKeyToken();
      stmtName = parser.getNextPreparedStmtNameToken();
      if (stmtName==null)
      {
         writeln(ise.getSyntaxError(this.queryStr,parser.getRemainderStr()).errorMessage());
         throw uic;
      }
      fromKeyToken = parser.getNextKeyToken();
      if (fromKeyToken == null || !fromKeyToken.equalsIgnoreCase("FROM"))
      {
         writeln(ise.getSyntaxError(this.queryStr,parser.getRemainderStr()).errorMessage());
         throw uic;
      }
      query = parser.getRemainderStr().trim();
      if (query == null || query.equalsIgnoreCase(""))
      {
         writeln(ise.getSyntaxError(this.queryStr,parser.getRemainderStr()).errorMessage());
         throw uic;
      }

      prepare(query,stmtName);
   }

   private void prepare(String qry,String stmtName) throws SQLException, IOException
   {
      PreparedStatement pStmt = null;
      boolean trimFlag = false;
      for (int i=0;i < trimKeysList.length;i++)
      {
         if (qry.toUpperCase().startsWith(trimKeysList[i]))
         {
            trimFlag=true;
            break;
         }
      }

      //Changes to support PREPARE on SPJ's
      if (qry.toUpperCase().startsWith("CALL"))
      {
         // Assign the sql query to queryStr. queryStr will be used
         // to prepare the statement
         queryStr = qry;
         try
         {
            prepareCallStmt(stmtName);
         }catch (SQLException sqlEx)
         {
            sessObj.removePrepStmtMap(stmtName);
            throw sqlEx;
         }
         sessObj.setPrepStmtMap(stmtName,(Object)cStmt,trimFlag);
         return;
      }

      sessObj.setQryStartTime();
      if(sessObj.getPrepStmtMap(stmtName) != null)
      {
         sessObj.getPrepStmtMap(stmtName).close();
         sessObj.removePrepStmtMap(stmtName);
      } 
      try
      {
         pStmt =((TrafT4Connection)conn).prepareStatement(qry,"\"" + stmtName + "\"");
      }catch (SQLException sqlEx)
      {
         sessObj.removePrepStmtMap(stmtName);
         throw sqlEx;
      }
     
      sessObj.setQryEndTime();
      
      sessObj.setPrepStmtMap(stmtName,(Object)pStmt,trimFlag);
      writeAllSQLWarnings(pStmt.getWarnings());
      pStmt.clearWarnings();
      /* This is to print the status message for XML/HTML correctly */
      qryObj.setRowCount("0");
   }

   /**
    *  Returns the evaluated parameter value.
    *
    *  @param   inputStr  The input string as the user specified it
    *  @return  the evaluated input string (parameter value)
    *
    */
   private String evaluateParameterValue(String inputStr) {
       try {
          ParamStringObject psv = parser.getParamValue(inputStr);
          return psv.getParameterValue();

      } catch(Exception e) {
      }

      return  inputStr;

   }   /*  End of  evaluateParameterValue  method.  */
   
   private void bind() throws IOException, UnKnownInterfaceCommand, SQLException, UserInterruption
   {
      String keyToken = null;
      String stmtName = null;
      PreparedStatement pStmt;
      List<String> paramList=null;
      String[] errParams=new String[1];


      keyToken = parser.getNextKeyToken();
      stmtName = parser.getNextPreparedStmtNameToken();
      if (stmtName == null)
      {
         writeln(ise.getSyntaxError(this.queryStr,parser.getRemainderStr()).errorMessage());
         throw uic;
      }
      if (parser.hasMoreTokens())
      {
         keyToken = parser.getNextKeyToken();
         if ((keyToken == null) || !(keyToken.equalsIgnoreCase("USING")))
         {
            writeln(ise.getSyntaxError(this.queryStr,parser.getRemainderStr()).errorMessage());
            throw uic;
         }

         paramList = parser.getNextParamListToken();
         if (paramList == null || paramList.size()==0)
         {
            writeln(ise.getSyntaxError(this.queryStr,parser.getRemainderStr()).errorMessage());
            throw uic;

         }
         for (int i=0;i < paramList.size(); i++)
         {
            String paramValue=null;
            if (paramList.get(i).toString().matches("^\\?\\S+"))
            {
               paramValue=sessObj.getSessParams(paramList.get(i).toString().substring(1));
               if (paramValue != null)
               {
                  paramList.remove(i);
                  paramList.add(i,evaluateParameterValue(paramValue));
               }

            }
         }
      }

      // code repeated in blockGetStats, any changes here needs to
      // be updated in the blockGetStats method also.
      pStmt = (PreparedStatement)sessObj.getPrepStmtMap(stmtName);
      org.trafodion.jdbc.t4.TrafT4PreparedStatement hpt4Stmt=(org.trafodion.jdbc.t4.TrafT4PreparedStatement)sessObj.getPrepStmtMap(stmtName);
 
      if (pStmt == null)
      {
         errParams[0]=stmtName;
         writer.writeError(sessObj, 'E',SessionError.STMT_NOT_FOUND,errParams);
         throw uic;
      }

      ParameterMetaData paramMetaData=null;
      paramMetaData = pStmt.getParameterMetaData();

      String sqlQueryStr=((TrafT4Statement)pStmt).getSQL();
      List<String> namedParamList=getParamNames(sqlQueryStr);
      
      // if using class is specified, merge the named parameters
      // and unnamed params and create one param list
      if (paramList != null)
      {
         int paramListIdx=0;
         for (int j=0;j < namedParamList.size(); j++)
         {
            if (namedParamList.get(j).toString().matches("^\\?"))
            {
               if (paramList.size() < paramListIdx+1)
               {
                  break;
               };
               namedParamList.remove(j);
               namedParamList.add(j,paramList.get(paramListIdx));
               paramListIdx++;
            };
         }

      }
      paramList=namedParamList;

      if (sqlQueryStr.toUpperCase().startsWith("CALL"))
      {
         qryObj.setStmtType(hpt4Stmt.getStatementType());
         cStmt = (CallableStatement)pStmt;
         sessObj.setQryStartTime();
         executeProcedure(paramList,false);
         return;
      }

      if (paramList !=null)
      {
         int namedParamErrors=0;
         int index=0;

         for (int i=0;i<paramList.size();i++)
         {
            String value=paramList.get(i).toString();
            String paramName=value;
            if (!(paramList.subList(0,i).contains(paramName)) || !paramList.get(i).toString().startsWith("?"))
            {
               if (value.matches("^\\?\\S+"))
               {
            	   String pv = (String) sessObj.getSessParams(value.substring(1));
                   value = evaluateParameterValue(pv);
               }
               if ((value == null) || (paramName.equals("?")))
               {
                  if (paramName.equals("?"))
                     paramName="?(UNNAMED_"+ (i+1) +")";

                  writer.writeInterfaceErrors(sessObj, new ErrorObject(SessionError.PARAM_NOT_FOUND, "", paramName + " was not found"));
                  
                  namedParamErrors++;
                  continue;

               }
//               dead code 
//               if (value== null) 
//            	   value=paramList.get(i).toString();
               value=value.replaceAll("\'\'","\'");
               try
               {
                  if (value!=null)
                  {
                     if (value.equalsIgnoreCase("NULL"))
                        pStmt.setNull(index+1,Types.NULL);
                     else
                     {
                int paramType = paramMetaData.getParameterType(index+1);
                if ((paramType == Types.BINARY) ||
                    (paramType == Types.VARBINARY))
                    {
                        pStmt.setBytes(index+1, value.getBytes());
                    }
                else
                    pStmt.setObject(index+1,new String(value.toString()));
                     }
                  }
                  
               }catch (NumberFormatException nfe)
               {
                  writer.writeError(sessObj,SessionError.NUMERIC_VAL_REQ);
                  namedParamErrors++;
                  continue;
                  //throw uic;
               }
               finally
               {
            	   index++;
               }
            }
         }

         if (namedParamErrors >0)
         {
            throw uic;
         }
      }

      sessObj.setQryStartTime();
      if (!sessObj.getPrepTrimOut(stmtName))
      {
         pStmt.setMaxRows(sessObj.getListCount());
      }else
      {
         qryObj.setTrimOut(true);
      }
               
      boolean moreResults = dbExec(pStmt);
      writeAllSQLWarnings(pStmt.getWarnings());
      pStmt.clearWarnings();
      qryObj.setStmtType(hpt4Stmt.getStatementType());

      if (hpt4Stmt.getSQL().matches("(?i)\\s*CREATE\\s+((SET|VOLATILE|SET VOLATILE)\\s+)?TABLE\\s+.*\\s+AS\\s+SELECT\\s+.*"))
    	  qryObj.setStmtType("CREATE_TABLE_AS");
      
      if (moreResults)
      {
         fetchResults(pStmt.getResultSet());
      }
      else
      {
         super.setQueryRowCount(pStmt);
         
         /* Modified this fix to call the execSet method for
          * EXECUTE on SET SCHEMA and SET CATALOG
          * instead of repeating the code again.
          */
         if ((sqlQueryStr.trim().matches("(?i)(?s)SET\\s+SCHEMA.*")) ||
             (sqlQueryStr.trim().matches("(?i)(?s)SET\\s+CATALOG.*"))
            )
         {
            String origQuery = qryObj.getQueryText();
            qryObj.resetQueryText(sqlQueryStr);
            queryStr = sqlQueryStr;
            parser.setRemainderStr(sqlQueryStr);
            parser.getNextKeyToken();
            execSet();
            qryObj.resetQueryText(origQuery);
         }
      
         sessObj.setQryEndTime();
         
      }


   }

   private void prepareCallStmt(String stmtName) throws SQLException, IOException
   {
      sessObj.setQryStartTime();
      //create a callable statement object for invoking SPJ's
      try
      {
         if(sessObj.getPrepStmtMap(stmtName) != null)
         {
            sessObj.getPrepStmtMap(stmtName).close();
            sessObj.removePrepStmtMap(stmtName);
         } 
      }catch(SQLException sqlEx){
         ;
      }
      
      try
      {
         // here if stmtName = null, then it will pass a "null" string, this will skip driver's conditional judgment (if (stmtName ==null) throw exception).
         cStmt = ((TrafT4Connection)conn).prepareCall(queryStr,"\"" + stmtName + "\"");
      } catch (NoSuchMethodError nsme)
      {
         if (stmtName == null)
            cStmt = conn.prepareCall(queryStr);
         else
         {
            throw new SQLException(SessionError.CALL_ERR.errorMessage());
         }
      }
      writeAllSQLWarnings(cStmt.getWarnings());
      cStmt.clearWarnings();
      sessObj.setQryEndTime();
      /* This is to print the status message for XML/HTML correctly */
      qryObj.setRowCount("0");

   }

   private void executeProcedure(List<String> callParamList,boolean deAllocStmt) throws IOException, UnKnownInterfaceCommand, SQLException
   {
      ParameterMetaData paramMetaData=null;
      String paramName=null;
      inOutParamMap = new HashMap<Integer,String>(); // contains the parameter index and the param name for INOUT and OUT params
      boolean paramsExist = false; // this will be set to true only if the SPJ contains INOUT and OUT params
      String value=null;
      int namedParamErrors=0;
      ResultSet rs=null;

      if (callParamList.size() != 0)
      {
         paramMetaData = cStmt.getParameterMetaData();
         if ((paramMetaData !=null))
         {
            //Set the values for dynamic parameters and register the
            //out parameters
            for (int i=1;i<=paramMetaData.getParameterCount();i++)
            {
               value = callParamList.get(i-1).toString();
               paramName = value;

               switch (paramMetaData.getParameterMode(i))
               {
                  case ParameterMetaData.parameterModeIn:
                     //If named parameters have been specified, retrieve the value
                     //from the session params hashmap and set the value in the
                     //callable statement.
                     if (value.matches("^\\?(\\S+)"))
                     {
                    	 String pv = (String) sessObj.getSessParams(value.substring(1));
                         value = evaluateParameterValue(pv);
                         if (sessObj.isDebugOn())
                         {
                             System.out.println("@@@Debug: DatabaseQuery:: pv = " +  pv);
                             System.out.println("@@@Debug: DatabaseQuery:: value = " + value);
                         }
                     }
                     if ((value == null) || paramName.equals("?"))
                     {
                        if (paramName.equals("?"))
                           paramName="?(UNNAMED_"+ i +")";
                        //writer.writeln("ERROR: Param "+paramName+ " was not found.");
                        //String errorStr = "ERROR: Param "+paramName+ " was not found.";
                        writer.writeInterfaceErrors(sessObj, new ErrorObject(SessionError.PARAM_NOT_FOUND, "", paramName + " was not found"));
                        namedParamErrors++;
                        continue;
                     }
                     try
                     {
                        if (value.equalsIgnoreCase("NULL"))
                        {
                           cStmt.setNull(i,Types.NULL);
                        }
                        else
                        {
                           cStmt.setObject(i,value);
                        }
                     }catch (NumberFormatException nfe)
                     {
                        writer.writeError(sessObj, SessionError.NUMERIC_VAL_REQ);
                        namedParamErrors++;
                        continue;
                        //throw uic;
                     }


                     break;

                  case ParameterMetaData.parameterModeInOut:

                     if (value.matches("^\\?(\\S+)"))
                     {
                    	 String pv = (String) sessObj.getSessParams(value.substring(1));
                         value = evaluateParameterValue(pv);
                     }
                     if ((value == null) || paramName.equals("?"))
                     {
                        if (paramName.equals("?"))
                           paramName="?(UNNAMED_"+ i +")";
                        //writer.writeln("ERROR: Param "+paramName+ " was not found.");
                        //String errorStr = "ERROR: Param "+paramName+ " was not found.";
                        writer.writeInterfaceErrors(sessObj, new ErrorObject(SessionError.PARAM_NOT_FOUND, "", paramName + " was not found"));
                        namedParamErrors++;
                        continue;
                     }
                     try
                     {
                        if (value.equalsIgnoreCase("NULL"))
                        {
                           cStmt.setNull(i,Types.NULL);
                        }
                        else
                        {
                           cStmt.setObject(i,value);
                        }
                     }catch (NumberFormatException nfe)
                     {
                        writer.writeError(sessObj, SessionError.NUMERIC_VAL_REQ);
                        namedParamErrors++;
                        continue;
                        //throw uic;
                     }

                     if (paramName.matches("^\\?(\\S+)"))
                        inOutParamMap.put(new Integer(i),paramName.substring(1));

                     if (!paramsExist)
                        paramsExist=true;

                     break;

                  case ParameterMetaData.parameterModeOut:
                     cStmt.registerOutParameter(i,paramMetaData.getParameterType(i));
                     // Add the param name and the index to the inOut Hashmap.
                     // This will be used to bind the inout and out param values after
                     // execute
                     if (paramName.matches("^\\?(\\S+)"))
                     {
                        inOutParamMap.put(new Integer(i),paramName.substring(1));
                     }

                     if (!paramsExist)
                        paramsExist=true;
                     break;

               } // end switch

               //}// end if match()
            } // end for
         } //end if

      }

      if (namedParamErrors > 0)
         throw uic;

      sessObj.setInOutandRS(false);
      qryText = qryObj.getQueryText();
      ResultSetMetaData rsmd = cStmt.getMetaData();
      boolean moreResults=dbExec(cStmt);
      sessObj.setQryEndTime();
      SQLWarning sqlWarn = cStmt.getWarnings();
      writer.writeAllSQLWarnings(sessObj,sqlWarn);
      qryObj.setRowCount(null);

      if (rsmd !=null)
      {
         //call the write method only if the SPJ contains INOUT and OUT params
         if (paramsExist)
         {
           if (sessObj.isSPJRS())
               sessObj.setSPJRS(false);
           writeOutParams(rsmd,rsmd.getColumnCount(),paramMetaData,cStmt);
         
         writer.writeln();
         }
      }

      if (moreResults)
      {
         if (paramsExist)
            sessObj.setInOutandRS(true);
         sessObj.setSPJRS(true);
         qryObj.setRsCount(0);
         qryObj.setRowCount(null);
         while (moreResults)
         {
            rs = cStmt.getResultSet();
            fetchResults(rs);
            qryObj.incrRsCount();
            qryObj.resetQueryText("SELECT *"); // Need to reset the qrytext to display no. of rows selected for status msg
            writer.writeStatusMsg(sessObj,qryObj,utils, writer);
            qryObj.resetQueryText(qryText); // Resetting the qry back to the original query Text for final status msg
            writer.writeln();
            qryObj.setRowCount(null);
            sessObj.setInOutandRS(false);
            //Check this without type casting to TrafT4Statement
            moreResults = ((TrafT4Statement)cStmt).getMoreResults();
         }
         qryObj.setRowCount(new Integer(qryObj.getRsCount()).toString());
         sessObj.setSPJRS(false);
      }
      else if (paramsExist)
      {
         qryObj.setRowCount("1");
      }
      else
         qryObj.setRowCount("0");
      if (deAllocStmt)
         cStmt.close();
   }

   
   private void execControl() throws SQLException, IOException, UnKnownInterfaceCommand, UserInterruption
   {
	   String remStr = parser.getRemainderStr();
	   
	   if (remStr.matches("(?is)\\s*CONTROL\\s+QUERY\\s+DEFAULT\\s+SCHEMA\\s+(.*)") || 
		   remStr.matches("(?is)\\s*CONTROL\\s+QUERY\\s+DEFAULT\\s+DEFAULT_SCHEMA_NAMETYPE\\s+(.*)"))
	   {
		   executeQuery(true);
		   queryStr = "showcontrol default schema, match full, no header";
		   try {
			   java.sql.Statement stmtSch = conn.createStatement();
			   ResultSet rs = stmtSch.executeQuery(queryStr);
			   
			   if (rs!=null && rs.next())
		       {
				   sessObj.setSessionSchema(rs.getString(1));
		       }
		       rs.close();
		       stmtSch.close();
		    } catch (Exception ex)
		    {
		        // System.out.println("Error : ExecControl :"+ex.getMessage());
		    }
	   }
	   else
	   {
		   executeQuery(true);
		  
	   }
   }
   
   private void execSet() throws SQLException, IOException, UnKnownInterfaceCommand, UserInterruption
   {
      String setOption=parser.getNextKeyToken();
      
      // not a keyword..then its not ours
      if (setOption == null)
      {
         executeQuery(true);
         return;
      }
      else
      {
         setOption=(String) dbKeyMap.get("SET_"+setOption.toUpperCase());

         if (setOption == null)
         {
            executeQuery(true);
            return;
         }

         int setId=Integer.parseInt(setOption);
         switch (setId)
         {
            case SessionDefaults.SET_SCHEMA:
               executeQuery(true);
               List<String> csList=parser.getCSTList(parser.getRemainderStr());
               switch (csList.size())
               {
                  case 2:
                     conn.setCatalog(csList.get(0).toString());
                     sessObj.setSessionCtlg(conn.getCatalog());
                     sessObj.setSessionSchema(csList.get(1).toString());
                     break;
                  case 1:
                     sessObj.setSessionSchema(csList.get(0).toString());
                     break;
               }
               sessObj.getEnvMap().put("SCHEMA", sessObj.getSessionSchema());
               // This is needed in the blockGetStats method
               qryObj.setQueryId(setId);
               break;
               
            case SessionDefaults.SET_CATALOG:
               sessObj.setQryStartTime();
               String ctlgValue = parser.getCatalogToken();
               if (ctlgValue == null)
                  ctlgValue="";
               conn.setCatalog(ctlgValue);
               sessObj.setQryEndTime();
               sessObj.setSessionCtlg(conn.getCatalog());
               sessObj.getEnvMap().put("CATALOG", sessObj.getSessionCtlg());
               qryObj.setRowCount("0");
               qryObj.setQueryId(setId);
               break;

            case SessionDefaults.SET_SERVICE:
               executeQuery(true);
               break;
         
               
            default:
               executeQuery(true);
               break;
         }
      }



   }
   
   public boolean  userExecutedGetStatistics()
   {
       return getStatsDone;
   }


   private void execGet(boolean setQueryStartTimeFlag) throws SQLException, IOException, UnKnownInterfaceCommand
   {
	  float prodVersion = 0.0f;
     
      parser.getNextKeyToken();
      if (!Utils.trimSQLTerminator(qryObj.getQueryText(), sessObj.getSessionSQLTerminator()).matches("(?i)\\s*get\\s+service\\s*"))
         qryObj.setTrimOut(true);
      else
         qryObj.setTrimOut(false);

      String getOption=parser.getNextWordToken();
      if ((getOption != null) && (getOption.toUpperCase().equals("STATISTICS")))
      {
         try {
        	 
        	 if (sessObj.isDebugOn())
            	 System.out.println("Database Query::execGet:: " + prodVersion);

         } catch (NullPointerException npe){}
         getOption = parser.getNextKeyToken();
         if (getOption == null)
         {
               queryStr = getStatsNcCmd;
         }
         if (sessObj.isDebugOn())
         {
        	 System.out.println("Database Query::execGet:: " + queryStr);
         }
         try
         {
            executeQuery(setQueryStartTimeFlag);
            if (setQueryStartTimeFlag)
               getStatsDone=true;
            
         }catch (SQLException sqlEx)
         {
            getStatsDone=false;
            throw sqlEx;
         }
      }
      else
      {
         executeQuery(true);
      }

   }
   
   public void resetQryObj()
   {
      if ((sessObj.getDisplayFormat() != SessionDefaults.XML_FORMAT) &&
          (sessObj.getDisplayFormat() != SessionDefaults.HTML_FORMAT) &&
          (sessObj.isImplicitGetStatsQry())
          )
      {
         qryObj.resetQueryText(userQry);
         qryObj.setTrimOut(userQryTrimOut);
      }
  
   }


   private void executeQuery(boolean setQueryStartTimeFlag) throws SQLException, IOException, UnKnownInterfaceCommand
   {
      if (setQueryStartTimeFlag)
         sessObj.setQryStartTime();
      //    if the list count is set..
      if (!qryObj.isTrimOut() && sessObj.getListCount() !=0)
      {
         stmt.setMaxRows(sessObj.getListCount());
      }else
      {
         stmt.setMaxRows(0);
      }
      
      // Check to see if query string has ? mark characters
      boolean paramsPatternFound = false;
      boolean queryHasParams = ( queryStr.indexOf("?") != -1 );
      if (queryHasParams)
      {
    	  // Doing a more expensive pattern matching check to see if the ? is a 
    	  // parameter and not a literal.
    	  try {
    		  Matcher mat = queryParamNamesPattern.matcher(queryStr);
    		  paramsPatternFound = mat.find();
    		  if (sessObj.isDebugOn())
    			  System.out.println(this.getClass().getName() +
    					  ":: Matcher.find : " + queryHasParams);
    	  } catch (Exception ex) 
    	  {
    		  if (sessObj.isDebugOn())
    			  System.out.println(this.getClass().getName() +
    					 ":: Param name regexp pattern error : " +
    					 ex.getMessage());
    	  }
      }
            
      if ((queryHasParams || paramsPatternFound)  &&
          (qryObj.getQueryId() != SessionDefaults.EXPLAIN)   &&
          (qryObj.getQueryId() != SessionDefaults.SHOWPLAN)  &&
          (qryObj.getQueryId() != SessionDefaults.SHOWSHAPE) )
      {
         List<String> paramList=getParamNames(queryStr);
         if (paramList.size() > 0)
         {
            PreparedStatement ps=null;
            ps=conn.prepareStatement(queryStr);
            String paramName=null;
            String paramValue=null;
            int namedParamErrors=0;

            ParameterMetaData paramMetaData=null;
            paramMetaData = ps.getParameterMetaData();

            int index=0;
            for (int i=0;i < paramList.size(); i++)
            {
               paramName=paramList.get(i).toString();
               if (!( paramList.subList(0,i).contains(paramName)))
               {
                  if (paramName.equals("?"))
                  {
                     paramName="?(UNNAMED_"+ (i+1) +")";
                  }
                  if ((paramValue=sessObj.getSessParams(paramName.substring(1))) == null)
                  {
                     writer.writeInterfaceErrors(sessObj, new ErrorObject(SessionError.PARAM_NOT_FOUND, "", paramName + " was not found"));
                     namedParamErrors++;
                     continue;
                  }
                  paramValue = evaluateParameterValue(paramValue);
                  try
                  {
                     if (paramValue.equalsIgnoreCase("NULL"))
                     {
                        ps.setNull(index+1,Types.NULL);
                     }else
                     {
                       int paramType = paramMetaData.getParameterType(index+1);
                       if ((paramType == Types.BINARY) ||
                           (paramType == Types.VARBINARY))
                           {
                               ps.setBytes(index+1, paramValue.getBytes());
                               //ps.setString(index+1, s);
                           }
                       else
                           ps.setObject(index+1,paramValue);
                     }
                  }catch (NumberFormatException nfe)
                  {
                     writer.writeError(sessObj, SessionError.NUMERIC_VAL_REQ);
                     namedParamErrors++;
                     //throw uic;
                     continue;
                  }
                  finally
                  {
                	  index++;  
                  }
               }
            }
            if (namedParamErrors>0)
            {
               throw uic;
            }
            ps.setMaxRows(sessObj.getListCount());

            sessObj.setCurrentStmtObj(ps);

            if (dbExec(ps))
            {
              
               writeAllSQLWarnings(ps.getWarnings());
               ps.clearWarnings();
               fetchResults(ps.getResultSet());
            }
            else
            {
               writeAllSQLWarnings(ps.getWarnings());
               ps.clearWarnings();
                
               sessObj.setCurrentStmtObj(null);
               super.setQueryRowCount(ps);
               sessObj.setQryEndTime();
            }
            
            resetQryObj();
            return;
         }
      }

      sessObj.setCurrentStmtObj(stmt);

      if (dbExec(stmt))
      {
         writeAllSQLWarnings(stmt.getWarnings());
         
         stmt.clearWarnings();
         rsObj= stmt.getResultSet();
         if (qryObj.getQueryId()==SessionDefaults.SHOW_SERVICE)
            return; 
         fetchResults(stmt.getResultSet());
      }
      else
      {
         writeAllSQLWarnings(stmt.getWarnings());
         stmt.clearWarnings();
         super.setQueryRowCount(stmt);
         sessObj.setQryEndTime();
      }

      stmt.close();
      sessObj.setStmtObj(sessObj.getConnObj().createStatement());
      this.stmt=sessObj.getStmtObj();
      resetQryObj();
   }

   private void fetchResults(ResultSet rs) throws SQLException, IOException
   {
      boolean isHeadingPrinted=false;
      ResultSetMetaData rsmd=rs.getMetaData();
      int numColumns = rsmd.getColumnCount();
      int rowCnt=0;
      qryObj.setColCount(String.valueOf(numColumns));
      if (sessObj.getFetchSize() > 0)
      {
    	  rs.setFetchSize(sessObj.getFetchSize());
      }

      try {
    	  for (rowCnt=0; rs.next();rowCnt++)
          {
             sessObj.setCurrentStmtObj(null);
             if (sessObj.isQueryInterrupted())
             {
                writer.writeln(SessionDefaults.lineSeperator+"*** WARNING: User interrupt [^C] received. Truncating data ... ");
                break;
             }
             
             /**
              *   Warnings are returned as part of fetch so
              *   we need to handle them.
              */
             isHeadingPrinted = printAnyWarningsDuringFetch(rs, isHeadingPrinted);
             
             if (!isHeadingPrinted)
             {
                writeHeader(rsmd,numColumns);
                isHeadingPrinted=true;
             }
             writeData(rs,numColumns);
          }
      } catch(java.lang.OutOfMemoryError e) {
    	  /**
    	   * Internal Analysis: JDBC Driver throws java.lang.OutOfMemoryError: Heap Out Space
    	   * the Exception was thrown by interfaceResultSet.fetch() method.
    	   * BufferedSize in JDBC exceeds and only can hold up to certain limit. 
    	   * "length = 1493630996", which value is  very big that caused
           *  the OutofMemoryEror
    	   * 
    	   * catch the OutOfMemory Exception 
    	   */
    	  throw new SQLException(
    	     "Unable to display result data - OutOfMemory.");
      }
      
      /**
       *   Warnings are returned as part of fetch so
       *   we need to handle them even if there are no results.
       */
      isHeadingPrinted = printAnyWarningsDuringFetch(rs, isHeadingPrinted);
       
      
      // if heading printed, then there are some rows retrieved from the db. so write a new line at the end
      if (isHeadingPrinted && sessObj.getDisplayFormat() == SessionDefaults.RAW_FORMAT)
      {
         writer.writeln();
      }

      if (sessObj.getListCount() != 0 && sessObj.getListCount() == rowCnt && !qryObj.isTrimOut())
      {
         qryObj.setStmtType("SELECT_LIST_COUNT");
      }
      if (!sessObj.isImplicitGetStatsQry())
         qryObj.setRowCount(String.valueOf(rowCnt));

      try
      {
         rs.close();
      } catch (SQLException sqle)
      {
      }
      finally
      {
         sessObj.setQryEndTime();
      }
   }

   /**
    *  Prints any warnings encountered as part of the fetch and displays
    *  them inline with the results. The method takes care of automatically
    *  printing headings for XML and HTML markups before the warnings are
    *  printed out.
    *
    *  @param   rs                The ResultSet object
    *  @param   isHeadingPrinted  Controls whether or not we need to print
    *                             the headings
    *
    *  @return  Whether or not headings were printed out
    *
    */
   private boolean printAnyWarningsDuringFetch(ResultSet rs,
                                               boolean isHeadingPrinted)
              throws SQLException, IOException {

      /**
       *   Check if the ResultSet parameter is valid.
       */
      if (null == rs)
         return isHeadingPrinted;


      /**
       *   Second check is to see if we have any warnings.
       */
      SQLWarning  rsWarnings = rs.getWarnings();
      if (null == rsWarnings)
         return isHeadingPrinted;



      /**
       *   We could have probably passed these down as parameters -- but
       *   rather than "burden" the stack, we just call these APIs when
       *   there are warnings. These calls should anyway be "thin" calls.
       */
      ResultSetMetaData rsmd       = rs.getMetaData();
      int               numColumns = rsmd.getColumnCount();


      /**
       *   Warnings are returned as part of fetch and
       *   need to be displayed along with the results. However we need 
       *   to print headings for XML and HTML markups before the warnings.
       */
      if (!isHeadingPrinted) {
         /*  Get the display format currently in use.  */
         int displayFmt = sessObj.getDisplayFormat();


         /*
          *  If heading was not printed, do so for HTML and XML markups.
          *  This allows any warnings to be embedded within the output.
          */
         if ((SessionDefaults.XML_FORMAT == displayFmt)  ||
             (SessionDefaults.HTML_FORMAT == displayFmt) ) {
            writeHeader(rsmd, numColumns);
            isHeadingPrinted = true;
         }

      }    /*  End of  IF  headings weren't printed.  */



      /**
       *   Warnings are returned as part of fetch.
       *   We already checked for warnings, now just print 'em out.
       *   Note calling rs.getWarnings() should clear the warnings, so
       *   we don't have any warnings to do the next time around.
       *   But JDBC/T4 didn't implement that part of the spec correctly
       *   in older drivers, so we do some double work here and call
       *   ResultSet.clearWarnings(). That way:
       *      1>  We will work w/ older T4 drivers, which don't clear these.
       *          So we got to step in and give a bit of a nudge!!
       *      2>  On newer well-behaved T4 drivers, this becomes a NOOP.
       *          We do the clearWarnings work twice (automatically done in 
       *          the T4 driver) and once here when we have any warnings.
       */
      writer.writeAllFetchWarnings(sessObj, rsWarnings, numColumns);
      sessObj.setSqlWarningsCnt(sessObj.getSqlWarningsCnt() + 1);
      rs.clearWarnings();


      /**
       *  Returns the state of whether or not we printed the headings out.
       */
      return isHeadingPrinted;

   }  /*  End of  printAnyWarningsDuringFetch  method.  */

   private String writeData(ResultSet rs,int numColumns) throws SQLException, IOException
   {
     
      for (int i=1;i<=numColumns ;i++)
      {
         ResultSetMetaData rsmd = rs.getMetaData();
         String value=null;
         try
         {
             if ((rsmd.getColumnType(i)==Types.BINARY) ||
                 (rsmd.getColumnType(i)==Types.VARBINARY))
                 {
                     // extract binary data from input and convert to hex.
                     // This avoids displaying unprintable characters.
                     value = "";
                     InputStream is = rs.getBinaryStream(i);
                     int numBytes = is.available();
                     for(int ii = 0; ii < numBytes; ii++){
                         int v = is.read();
                         String thisByte = Integer.toHexString(v).toUpperCase();
                         // leading zero pad
                         if (thisByte.length() == 1)
                             thisByte = "0" + thisByte;
                         value += thisByte;
                     }
                 }
             else
                 value=rs.getString(i);
            if (sessObj.getQuery().getQueryId() == SessionDefaults.SHOW_SERVICE)
               return value;
         } catch (SQLException sqlEx)
         {
            //For blob objects, getString() throws a SQLException.
            //Displaying null in this case.
         }

         if(value != null && rsmd.getColumnType(i)==Types.TIMESTAMP && value.length() < rsmd.getColumnDisplaySize(i))
         {
          	 value = (value.trim()+"000000").substring(0,rsmd.getColumnDisplaySize(i));
         } 
         
         if (value == null)
         {
            value=sessObj.getSessNull();
            if (sessObj.getSessView() == SessionDefaults.MXCI_VIEW)
                value = "?";
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
               formatCsvOutput(value);
               break;
         }

      }

      if (sessObj.getDisplayFormat() == SessionDefaults.RAW_FORMAT)
         writeln();

      return null;
   }

   private void writeHeader(ResultSetMetaData rsmd,int numColumns) throws SQLException, IOException
   {
      columnNameArray = new String[numColumns];
      colSize=new int[numColumns];
      colAlign=new int[numColumns];
      
      int totalWidth = 0;
      
      if (qryObj.isTrimOut())
      {
         return;
      }
      for (int i=1;i<=numColumns ;i++)
      {
          
         int colNameSize = rsmd.getColumnName(i).length(); 
         if (sessObj.isMultiByteAlign() && (isoMapping == 10 || isoMapping == 15)) //SJIS or UTF8
         {
             try
             {
                 colNameSize = getMultiByteColWidth(new StringBuffer(rsmd.getColumnName(i)));
             }
             catch (Exception ex)
             {
                 ex.printStackTrace();
             }
         }  
         

         if ((rsmd.getColumnType(i)==Types.BINARY) ||
             (rsmd.getColumnType(i)==Types.VARBINARY))
             {
                 // binary value will be displayed in hex.
                 colNameSize = 2 * rsmd.getColumnDisplaySize(i);
             }
         
         if (colNameSize > rsmd.getColumnDisplaySize(i))
            colSize[i-1]=colNameSize;
         else
            colSize[i-1]=rsmd.getColumnDisplaySize(i);

         if (colSize[i-1] < SessionDefaults.MIN_COL_DISPLAY_SIZE)
         {
            colSize[i-1] = SessionDefaults.MIN_COL_DISPLAY_SIZE;
         }

         totalWidth += colSize[i-1];
         
         switch (sessObj.getDisplayFormat())
         {
            case SessionDefaults.RAW_FORMAT:
               if (numColumns ==1)
                  formatOutput(rsmd.getColumnName(i),colNameSize,' ',0);
               else
               {
                  if(sessObj.getSessView() == SessionDefaults.MXCI_VIEW && i == numColumns && totalWidth > SessionDefaults.MXCI_TRIM_FIX)
                      formatOutput(rsmd.getColumnName(i),colNameSize,' ',0);
                  else
                      formatOutput(rsmd.getColumnName(i),colSize[i-1],' ',0);
               }
               if (i < numColumns)
               {
                  writeSeparator();
                  
                  /* total width has to be added twice in MXCI mode b/c it has
                   * two separators
                   */
                  if(sessObj.getSessView() == SessionDefaults.MXCI_VIEW)
                      totalWidth++;
                  
                  totalWidth++;
               }
               break;

            case SessionDefaults.XML_FORMAT:
               columnNameArray[i-1] = sessObj.getXmlObj().checkColumnNames(utils.removeSpaces(rsmd.getColumnName(i)));
               break;

            case SessionDefaults.HTML_FORMAT:
               formatHtmlOutput(rsmd.getColumnName(i));
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

      if (sessObj.getDisplayFormat() == SessionDefaults.RAW_FORMAT)
      {
         writeln();
         for (int i=1;i<=numColumns ;i++)
         {
            formatOutput("",colSize[i-1],'-',0);
            if (i < numColumns)
            {
               writeSeparator();
            }
         }
         writeln();
         if (sessObj.getSessView() == SessionDefaults.MXCI_VIEW)
            writeln();
      }
   }

   private void writeOutParams(ResultSetMetaData rsmd,int numColumns,ParameterMetaData paramMetaData,CallableStatement callStmt) throws SQLException, IOException
   {
      sessObj.setWriteParams(true);
      columnNameArray = new String[numColumns];
      colSize=new int[numColumns];
      colAlign=new int[numColumns];
      qryObj.setColCount(String.valueOf(numColumns));

      if (qryObj.isTrimOut())
      {
         return;
      }
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
      writeData(callStmt,numColumns,paramMetaData);
      qryObj.setRowCount("1");
   }

   private void writeData(CallableStatement callStmt,int numColumns, ParameterMetaData paramMetaData) throws SQLException, IOException
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
            // bind the inout and out params
            Integer paramIdxObj = new Integer(i);
            if (inOutParamMap != null && inOutParamMap.containsKey(paramIdxObj))
            {
               sessObj.setSessParams(inOutParamMap.get(paramIdxObj).toString(),value);
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
                  formatCsvOutput(value);
                  break;
            }
         }
      }
      inOutParamMap=null;
      if (sessObj.getDisplayFormat() == SessionDefaults.RAW_FORMAT)
         writeln();
   }

   private void writeAllSQLWarnings(SQLWarning sqlw) throws IOException
   {
      if (sqlw != null)
      {
         writer.writeAllSQLWarnings(sessObj,sqlw);
         sessObj.setSqlWarningsCnt(sessObj.getSqlWarningsCnt()+1);
      }
   }

   private void loadDbKeyWordMap()
   {

      if (dbKeyMap == null)
      {
         dbKeyMap = new HashMap<String, String>();
      }

      dbKeyMap.put("SET_SCHEMA",""+SessionDefaults.SET_SCHEMA);
      dbKeyMap.put("SET_CATALOG",""+SessionDefaults.SET_CATALOG);
      dbKeyMap.put("SET_SERVICE",""+SessionDefaults.SET_SERVICE);
      dbKeyMap.put("CALL",""+SessionDefaults.CALL);
      dbKeyMap.put("INFOSTATS",""+SessionDefaults.INFOSTATS);
      dbKeyMap.put("PREPARE",""+SessionDefaults.PREPARE);
      dbKeyMap.put("EXECUTE",""+SessionDefaults.EXECUTE);
   }
   
   public boolean blockGetStats()
   {
      String query=null;
      switch(qryObj.getQueryId())
      {
         case SessionDefaults.SET_SCHEMA:
         case SessionDefaults.SET_CATALOG:
            return true;
            
         case SessionDefaults.EXECUTE :
            query = Utils.trimSQLTerminator(qryObj.getQueryText(),sessObj.getSessionSQLTerminator());
            parser.setRemainderStr(query);
            parser.getNextKeyToken();
            String stmtName = parser.getNextPreparedStmtNameToken();
            PreparedStatement pStmt = (PreparedStatement)sessObj.getPrepStmtMap(stmtName);
            String sqlQueryStr=((TrafT4Statement)pStmt).getSQL();
            if ((sqlQueryStr.matches("(?i)(?s)^SET\\s+SCHEMA.*")) ||
                (sqlQueryStr.matches("(?i)(?s)^SET\\s+CATALOG.*")))
               return true;
            else
               return false;
               
         default:
               return false;
         
      }
   }

   private List<String> getParamNames(String qryString)
   {
      qryString=qryString+" ";
      char[] rQryStr=qryString.toCharArray();
      boolean quoteFound=false;
      final char sQUOTE='\'';
      final char dQUOTE='"';
      final char pIDENTITY='?';
      char currentQuote=sQUOTE;
      boolean paramFound=false;
      int paramStartPos=0;
      List<String> paramList=new java.util.ArrayList<String>();


      for (int i=0;i < rQryStr.length;i++)
      {
         // if we encounter quote skip all the characters till another quote.

         if ((quoteFound && rQryStr[i] != sQUOTE && rQryStr[i] != dQUOTE)    ||
            (quoteFound && rQryStr[i] == sQUOTE && currentQuote != sQUOTE)  ||
            (quoteFound && rQryStr[i] == dQUOTE && currentQuote != dQUOTE))
         {
            continue;
         }


         if ((quoteFound && rQryStr[i] == sQUOTE && currentQuote == sQUOTE) ||
            (quoteFound && rQryStr[i] == dQUOTE && currentQuote == dQUOTE))
         {
            quoteFound=false;
            continue;
         }

         if ((!quoteFound && rQryStr[i] == sQUOTE) ||
            (!quoteFound && rQryStr[i] == dQUOTE))
         {
            quoteFound = true;
            currentQuote=rQryStr[i];
            continue;
         }

         if (rQryStr[i] == pIDENTITY && !paramFound)
         {
            paramFound=true;
            paramStartPos=i;
            continue;
         }

         if (paramFound && ( ( (int)rQryStr[i] >= 97 && (int)rQryStr[i] <= 122 ) || ((int)rQryStr[i] >= 65 && (int)rQryStr[i] <= 91 ) ||
            ((int)rQryStr[i] >= 48 && (int)rQryStr[i] <= 57 ) || rQryStr[i]=='_'  ))
         {
            continue;
         }

         if (paramFound)
         {
            paramList.add(qryString.substring(paramStartPos,i));
            paramFound=false;
            paramStartPos=0;
         }

      }
      return paramList;

   }
}

