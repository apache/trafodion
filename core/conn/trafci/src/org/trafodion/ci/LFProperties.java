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
 * Class to load and display the look and feel properties file.
 * 
 */

package org.trafodion.ci;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.Enumeration;
import java.util.Properties;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class LFProperties
{

   private Properties lfProps=null;
   private Session sessObj=null;

   LFProperties(){}

   public String getStatus(Query qryObj) {
	  String qryString = Utils.trimSQLTerminator(deleteCStyleComments(qryObj.getQueryText().trim()),sessObj.getSessionSQLTerminator());
      String queryString=sessObj.getStrMode()+"_"+qryString.trim().toUpperCase().replaceAll("\\s+"," ");
      Enumeration<Object> enumKeys=lfProps.keys();
      int keySize=0;
      String matchedElement=null;
      String matchedValue=null;

      while (enumKeys.hasMoreElements())
      {
         String nextElement=enumKeys.nextElement().toString();

         if (queryString.matches(nextElement))
         {
            if (nextElement.length() > keySize)
            {
               keySize=nextElement.length();
               matchedElement=nextElement;
            }
         }

      }

      if (keySize > 0)
      {
         String stmtType=qryObj.getStmtType();

         if (((matchedElement.startsWith("SQL_EXECUTE")) && !qryObj.isTrimOut() &&
            ( (stmtType.equalsIgnoreCase("SELECT"))  ||
            (stmtType.equalsIgnoreCase("INSERT"))  ||
            (stmtType.equalsIgnoreCase("UPDATE"))  ||
            (stmtType.equalsIgnoreCase("DELETE"))  ) ) || ( stmtType != null && stmtType.equals("SELECT_LIST_COUNT")))
         {
            matchedElement=sessObj.getStrMode()+"_"+stmtType+" .*";
         }

         if ((matchedElement.startsWith("SQL_EXECUTE")) && !qryObj.isTrimOut() &&
                 (stmtType.equalsIgnoreCase("CREATE_TABLE_AS")))
         {
        	 matchedElement = "SQL_CREATE TABLE .* AS SELECT .*";
         }

         
         matchedValue = lfProps.getProperty(matchedElement);
         if (matchedValue.indexOf("@rownum@") != -1)
         {
               return (matchedValue.replaceFirst("@rownum@",qryObj.getRowCount()));
         }
         return (matchedValue);
      }
      return (lfProps.getProperty(sessObj.getStrMode()+"_UNKNOWN_CMD"));
   }

   /*
    * Delete C-style comments in a String
    */
   private String deleteCStyleComments(String originalString)
   {
	   Pattern p = Pattern.compile( "/[*]{1,2}.*?[*]/",Pattern.DOTALL); 
	   Matcher m = p.matcher(originalString); 
	   return m.replaceAll("");
   }

   //  load the look and feel properties 
   public void loadLookAndFeelProps(Session sessObj, boolean specialModeEnabled) 
   		throws FileNotFoundException, IOException {
	  String laFileName=null;
      String laOption=System.getProperty(SessionDefaults.PROP_TRAFCILF);
      this.sessObj = sessObj;
      
      laFileName= SessionDefaults.PROP_DEFAULT_NAME;
      sessObj.setSessView(SessionDefaults.CIDEFAULT_VIEW);

      loadProperties(laFileName);
   }

   public void loadProperties(String fileName) throws FileNotFoundException, IOException
   {

      lfProps=new Properties();
      lfProps.load(this.getClass().getResourceAsStream(fileName));

   }


}
