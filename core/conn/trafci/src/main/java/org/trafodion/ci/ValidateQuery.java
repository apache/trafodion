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

/* 
 * This class verifies the first key word of any sql string passed and  
 * categorizes the type of sql. The query type is set in the query object
 */

import java.util.HashMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
//import java.util.Iterator;
//import java.util.Map;
//import java.util.TreeMap;


public class ValidateQuery implements SessionDefaults
{
   private Query qryObj=null;
   private HashMap<String, String> fKeyMap;
   NullKeyWordException nke;          // if the user enters blank throw this exception
   InterfaceQueryException iqe;     // if the token is not listed throw this exception
   Session sessObj=null;
   
   


   ValidateQuery(Session sessObj)
   {
      fKeyMap=new HashMap<String, String>();
      loadfKeyConstants();
      nke=new NullKeyWordException();
      iqe=new InterfaceQueryException();
      this.sessObj=sessObj;
   }

   public HashMap<String, String> getfKeyMap(){
	   return fKeyMap;
   }

   public Query getQuery()
   {
      return qryObj;
   }
   public void setQuery(Query qryObj)
   {
      this.qryObj = qryObj;
   }

   public void validate(Query qryObj,String sqlTerminator) throws NullKeyWordException
   {
      this.qryObj=qryObj;
      String qryText= qryObj.getQueryText();

      if (qryText != null)
      {
         qryText=Utils.trimSQLTerminator(qryText.trim(),sqlTerminator);
      }

      // check the query string and return NullKeyWord if its blank 
      if ((qryText== null)||( qryText !=null && qryText.trim().equals("")))
      {
         throw nke;
      }

      String fKeyWord=getFirstToken(qryText);

      // this case should not happen because we are checking for non space characters
      if (fKeyWord==null)
      {
         throw nke;
      }
     
      /*Check for Alias*/
      if(sessObj.getAliasMap()!=null && qryText!=null)
      {    
        if(!(qryText.trim().equals("")))
        {
            if(sessObj.getAliasMap().containsKey(fKeyWord.toUpperCase()))
    	    {
            	qryText = qryObj.getQueryText().replaceFirst(replaceRegexCharacter(fKeyWord), sessObj.getAlias(fKeyWord.toUpperCase())); 
       	   	    qryObj.resetQueryText(qryText);
       	   	    qryText=Utils.trimSQLTerminator(qryText.trim(),sqlTerminator);
       	   	    fKeyWord=getFirstToken(qryText);
    	    }
        }
      }
      
      // if the first key word starts with @ , replace it with obey
      if (fKeyWord.startsWith("@"))
      {
         fKeyWord="@";
		 qryText = qryText.replaceFirst("@", "@ ");
		 qryObj.resetQueryText(qryText);
      }

      //replace the first key word with its associated constant value
      // from fKeyMap has table
		fKeyWord = fKeyMap.get(fKeyWord.toUpperCase());

      // if the constant value is not found, its defintely a sql or some other
      // unknown tokens. so set the query type to SQL 
      // set the query id -1 as its unknown at this time
      if (fKeyWord == null)
      {
		if (sessObj.getMode() == SQL_MODE )
					qryObj.setQueryType(SQLQ);

         qryObj.setMultiLine(true);
         qryObj.setPassThrough(true);
         qryObj.setQueryId(-1);
         return;
      }

      // convert the string integer to int
      // we dont need to catch exception here as we made sure
      // that the constant is int and present in the fKeyMap
      int fKeyId=Integer.parseInt(fKeyWord);

      switch (fKeyId)
      {
         case PRUN:

            qryObj.setQueryType(PRUNQ);
            qryObj.setMultiLine(false);
            qryObj.setPassThrough(false);
            qryObj.setQueryId(fKeyId);
            break;


         case ALIAS:
        	 qryObj.setQueryType(IQ);
        	 qryObj.setMultiLine(true);
        	 qryObj.setPassThrough(false);
        	 qryObj.setQueryId(fKeyId);
        	 break;

         case EXIT:
         case ED:
         case EDIT:
         case FC:
         case CLEAR:
         case SLASH:
         case RUN:
         case SECTION:
         case LOCALHOST:
            //case AT:
            qryObj.setQueryType(IQ);
            qryObj.setMultiLine(false);
            qryObj.setPassThrough(false);
            qryObj.setQueryId(fKeyId);
            break;

         case SET:
         case SPOOL:
         case SHOW:
         case OBEY:
         case LOG:
         case MODE:
         case RESET:
         case ENV:
         case SESSION:
         case HISTORY:
         case REPEAT:
         case HELP:
         case SAVEHIST:
         case VERSION:
         case DISCONNECT:
         case CONNECT:
         case RECONNECT:
         case SRUN:
         case ERROR:
         case ONLINEDBDUMP:
         case AUDIT:
         case SCHEDULE:
         case ALLOW:
         case DENY:
         case LIST:
         case DELAY:
            qryObj.setQueryType(IQ);
            qryObj.setMultiLine(false);
            qryObj.setPassThrough(false);
            qryObj.setQueryId(fKeyId);
            break;

         case INVOKE:
         case SHOWDDL:
         case SHOWSHAPE:
         case SHOWCONTROL:
         case SHOWTRANSACTION:
         case REORG:
         case REORGANIZE:
         case MAINTAIN:
         case REPLICATE:
         case SHOWLABEL:
         case SHOWPLAN:
         case EXPLAIN:
         case GET:

            //qryObj.setQueryType(SQLQ);
			if (SQL_MODE == sessObj.getMode() || CS_MODE == sessObj.getMode())
				qryObj.setQueryType(SQLQ);
			else if (sessObj.getMode() == WMS_MODE)
				qryObj.setQueryType(NSQ);
            qryObj.setMultiLine(true);
            qryObj.setPassThrough(true);
            qryObj.setQueryId(fKeyId);
            qryObj.setTrimOut(true);    // set the trimout to true as we need to trim the result sets for all these cases
            break;

         case IF_THEN:
         case LABEL:
         case GOTO:
            qryObj.setQueryType(CQ);
            qryObj.setMultiLine((fKeyId == IF_THEN));
            qryObj.setPassThrough(false);
            qryObj.setQueryId(fKeyId);
            break;
         
            
         case DOTSQL:
         case DOTCS:
         case DOTNS:
         case DOTWMS:
         case DOTSEC:
        	 qryObj.setQueryType(IQ);
        	 qryObj.setMultiLine(false);
        	 qryObj.setPassThrough(false);
        	 qryObj.setQueryId(fKeyId);
        	 break;
         
         default:
//            if (sessObj.getMode()==SQL_MODE)
			if (SQL_MODE == sessObj.getMode() || CS_MODE == sessObj.getMode())
				qryObj.setQueryType(SQLQ);
			else if (sessObj.getMode() == WMS_MODE)
				qryObj.setQueryType(NSQ);
            qryObj.setMultiLine(false);
            qryObj.setPassThrough(false);
            qryObj.setQueryId(fKeyId);
            break;

      }

   }

   // the first token will be any non-space characters as a word
   // prefixed and suffixed with one or more spaces or
   // line end
   String getFirstToken(String fLine)
   {
      Pattern pat=Pattern.compile("^\\s*(\\S*)(\\s+.*|$)");
      Matcher mat=pat.matcher(fLine);
      if (mat.find())
      {
         return mat.group(1);
      }
      else
         return null;
   }

   int getQueryId(String qryStr)
   {
	   String fToken= getFirstToken(qryStr);
		fToken = fKeyMap.get(fToken.toUpperCase());
	   if (fToken !=null)
		   return Integer.parseInt(fToken);
	   else 
		   return -1;
	   
   }
   
//   public String trimSQLTerminator(String qryStr,String sqlTerminator)
//   {
//      if (qryStr.endsWith(sqlTerminator))
//      {
//         return qryStr.substring(0,qryStr.length()- sqlTerminator.length());
//      }
//      return qryStr;
//   }

   // load all first keywords which needs to be processed
   private void loadfKeyConstants()
   {
      fKeyMap.put("PRUN",""+PRUN);
      fKeyMap.put("EXIT",""+EXIT);
      fKeyMap.put("QUIT",""+EXIT);
      fKeyMap.put("DISCONNECT",""+DISCONNECT);
      fKeyMap.put("SET",""+SET);
      fKeyMap.put("RESET",""+RESET);
      fKeyMap.put("ED",""+ED);
      fKeyMap.put("SPOOL",""+SPOOL);
      fKeyMap.put("LOG",""+LOG);
      fKeyMap.put("/",""+SLASH);
      fKeyMap.put("RUN",""+RUN);
      fKeyMap.put("SHOW",""+SHOW);
      fKeyMap.put("@",""+OBEY);
      fKeyMap.put("MODE",""+MODE);
      fKeyMap.put("OBEY",""+OBEY);
      fKeyMap.put("HISTORY",""+HISTORY);
      fKeyMap.put("REPEAT",""+REPEAT);
      fKeyMap.put("INVOKE",""+INVOKE);
      fKeyMap.put("SHOWDDL",""+SHOWDDL);
      fKeyMap.put("SHOWSHAPE",""+SHOWSHAPE);
      fKeyMap.put("SHOWCONTROL",""+SHOWCONTROL);
      fKeyMap.put("REORG",""+REORG);
      fKeyMap.put("REORGANIZE",""+REORGANIZE);
      fKeyMap.put("MAINTAIN",""+MAINTAIN);
      fKeyMap.put("SHOWLABEL",""+SHOWLABEL);
      fKeyMap.put("SHOWPLAN",""+SHOWPLAN);
      fKeyMap.put("PREPARE",""+PREPARE);
      fKeyMap.put("EXECUTE",""+EXECUTE);
      fKeyMap.put("EXPLAIN",""+EXPLAIN);
      fKeyMap.put("INFOSTATS",""+INFOSTATS);
      fKeyMap.put("FC",""+FC);
      fKeyMap.put("CALL",""+CALL);
      fKeyMap.put("CLEAR",""+CLEAR);
      fKeyMap.put("HELP",""+HELP);
      fKeyMap.put("?SECTION",""+SECTION);
      fKeyMap.put("SAVEHIST",""+SAVEHIST);
      fKeyMap.put("VERSION",""+VERSION);
      fKeyMap.put("ENV",""+ENV);
      fKeyMap.put("SESSION",""+SESSION);
      fKeyMap.put("CONNECT",""+CONNECT);
      fKeyMap.put("RECONNECT",""+RECONNECT);
      fKeyMap.put("GET",""+GET);
      fKeyMap.put("LOCALHOST",""+LOCALHOST);
      fKeyMap.put("LH",""+LOCALHOST);
      fKeyMap.put("ERROR",""+ERROR);
      fKeyMap.put("ONLINEDBDUMP",""+ONLINEDBDUMP);
      fKeyMap.put("SRUN",""+SRUN);
      fKeyMap.put("ALLOW",""+ALLOW);
      fKeyMap.put("DENY",""+DENY);
      fKeyMap.put("SCHEDULE",""+SCHEDULE);
      fKeyMap.put("LIST",""+LIST);
      fKeyMap.put("AUDIT",""+AUDIT);
      fKeyMap.put("DELAY",""+DELAY);
      fKeyMap.put("IF",""+IF_THEN);
      fKeyMap.put("LABEL",""+LABEL);
      fKeyMap.put("GOTO",""+GOTO);
      fKeyMap.put("ALIAS",""+ALIAS);
      fKeyMap.put(".SQL",""+DOTSQL);
      fKeyMap.put(".CS",""+DOTCS);
      fKeyMap.put(".NS",""+DOTNS);
      fKeyMap.put(".WMS",""+DOTWMS);
      fKeyMap.put(".SEC",""+DOTSEC);
      fKeyMap.put("CONTROL",""+CONTROL);
      fKeyMap.put("REPLICATE",""+REPLICATE);
      fKeyMap.put("SHOWTRANSACTION",""+SHOWTRANSACTION);

   }
   /**
    * Added  2010-02-26
    * Replace reserved characters of regex with '\'+characters
    * @param source
    * @return
    */
   public String replaceRegexCharacter(String source)
   {
	   if(source==null || source.length()==0)
		   return "";
	   StringBuilder stbReturn = new StringBuilder();
	   char[] analyStr=source.toCharArray();
	   if(analyStr.length>0)
	   {
		   for(int i=0;i<analyStr.length;i++)
		   {
			   switch(analyStr[i])
			   {
			   	case '+':
			   	case '?':
			   	case '*':
			   	case '.':
			   	case '^':
			   	case '{':
			   	case '[':
			   	case '(':
			   	case ')':
			   	case '$':
			   		stbReturn.append('\\');
			   		break;
			   		
			   }
			   stbReturn.append(analyStr[i]);
		   }
	   }
	   
	   return stbReturn.toString();
   }
   

}
