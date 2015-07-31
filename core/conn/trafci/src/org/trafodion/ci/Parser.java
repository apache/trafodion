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

import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.ArrayList;
import java.util.List;

public class Parser
{
   private String remainderStr=null;
   Pattern filePat=Pattern.compile("^\\s*((\"(\"(?=\")|.)*\")|\\S*)(\\s+(.*)|$)");
   Pattern numberPat=Pattern.compile("^\\s*((-)?\\d+)(\\s+(.*)|$)");
   Pattern paramPat=Pattern.compile("^\\s*(\\??)([a-zA-Z_]\\w*)(\\s+(.*)|$)");
   Pattern keyPat=Pattern.compile("^\\s*(\\S+)(\\s+(.*)|\\z)",Pattern.DOTALL);
   Pattern ignoreCommentsPat=Pattern.compile("(?m)((^--)|^)((((?!--|\').)*)*(\\s*((\'(\'(?=\')|\\\\(?=\')|(?!\').)*\')*(((?!--|\').)*)*)\\s*)*)");
   Pattern pstmtPat=Pattern.compile("^\\s*((\"[a-zA-Z_]\\w*\")|[a-zA-Z_]\\w*)(\\s+(.*)|$)",Pattern.DOTALL);
   Pattern explainPat = Pattern.compile( "(^\\s*(?i)EXPLAIN(\\s+(?i)OPTIONS\\s+\'f\')?)\\s+((\"[a-zA-Z_]\\w*\")|[a-zA-Z_]\\w*)\\s*$",Pattern.DOTALL);
   Pattern valPat=Pattern.compile("^\\s*((\"(\"(?=\")|\\\\(?=\")|.)*?\")|(\\S+))(\\s*(.*)|$)");
   Pattern val2Pat=Pattern.compile("^\\s*((\'(\'(?=\')|\\\\(?=\')|.)*\')+|(\\S+))(\\s+(.*)|$)");
   /**
    * Support _UTF8 for set PARAM
    */
   Pattern paramValuePat=Pattern.compile("^\\s*(((_[iI][sS][oO]88591|_[uU][cC][sS]2|_[uU][tT][fF]8|_[kK][aA][nN][jJ][iI]|_[kK][sS][cC]5601)?(\\s*[xX])?(\'(.*)\'))|(\\S+))(\\s+(.*)|$)");
   Pattern catalogPat=Pattern.compile("(\"(?:[^\"\\\\]|\"\"|\\\\.)*\"|\\S+)(\\s*.*|$)");   
   Pattern tabPat=Pattern.compile("^\\s*((\"(\"(?=\")|\\\\(?=\")|.)*?\")|(\\w+));*\\s*(\\.(.*)|$|(,\\s*(?i)[A-Zz-z]*(.*)?$))",Pattern.DOTALL);
   //Pattern paramListPat=Pattern.compile("^\\s*(\\??)((\'(\'(?=\')|\\\\(?=\')|.)*?\')|([^ ,]+))\\s*(,\\s*(.*)|$)",Pattern.DOTALL);
   Pattern paramListPat=Pattern.compile("^\\s*((\'(\'(?=\')|\\\\(?=\')|.)*?\')|([^ ,]+))\\s*(,\\s*(.*)|$)",Pattern.DOTALL);
   Pattern mapListPat=Pattern.compile("^\\s*((\"(\"(?=\")|\\\\(?=\")|.)*?\")*([^,]+))\\s*(,\\s*(.*)|$)",Pattern.DOTALL);
   Pattern exitPat=Pattern.compile("(?i)^\\s*((WITH\\s+(-?\\d+))|(-?\\d+))?(\\s+IF(?!.*THEN.*).+)?$");
   Pattern sectionPat=Pattern.compile("^\\s*\\(\\s*([a-zA-Z_]\\w*)\\s*\\)(\\s*$)");
   Pattern mxciParamPat = Pattern.compile("^\\?(.*)");
   Pattern dbFunctionPat = Pattern.compile("(?i)FN:(\\w+\\((.*)\\))");
   Pattern mapFileFixedWidthColPat = Pattern.compile("^\\s*COLPOS\\((\\d+):(\\d+)\\).*$");
   Pattern envPat=Pattern.compile("%([a-zA-Z]+)(.*)");
   Pattern scriptPat=Pattern.compile("(?i)^\\s*(SRUN)\\s+(.*)");
   Pattern connUserPat=Pattern.compile("([^,/@]+)(.*)");
   Pattern connUserPreparePat=Pattern.compile("(^\".*?\")(.*)");
   Pattern connPassPat=Pattern.compile("^/((\"(\"(?=\")|\\\\(?=\")|.)*?\")|[^@,]*)(@.*$|,.*$|$)");
   Pattern connServerPat=Pattern.compile("^@([^,]+)(,.*$|$)");
   Pattern connDsnPat=Pattern.compile("^,(.+)($)");
   Pattern errPat = Pattern.compile("\\*\\*\\* (ERROR|WARNING)\\[(\\w*)\\](.*)",Pattern.DOTALL);
   Pattern ndcsProductInfo = Pattern.compile("T7970([^ ])*",Pattern.DOTALL);
   Pattern sysInfo = Pattern.compile("Current SYSnn(\\s+(.*))");
   Pattern t4Version= Pattern.compile("T1249_.*[(]R(.[^)]*)");
   Pattern wmsErrPat= Pattern.compile("Unexpected.*(error message = (.*))");
   String showWildCardPattern = "(?<!\\\\)\\*";
   String showWildCardPattern2 = "\\\\\\*";
   String showWildCardPatternChar = "(?<!\\\\)\\?";
   Pattern wordPattern=Pattern.compile("^\\s*(\\w+)(\\s*(.*)|$)");  
   Pattern dbProdVersionPat=Pattern.compile("([A-Z]((\\d+)\\.(\\d+))(.*))((is the SUT identifier\\.)|(for NEO no ipms)|(plus integration build(.*)))");
   Pattern condPat = Pattern.compile("^(?i:IF)(\\s|\\n)+((\"(\"(?=\")|\\\\(?=\")|.)*?\")|[^(\\s|\\n|~=|\\^=|!=|<>|>=|<=|>|<|=|==)]+)(\\s|\\n)*(~=|\\^=|!=|<>|>=|<=|>|<|==|=)(\\s|\\n)*((\"(\"(?=\")|\\\\(?=\")|.)*?\")|\\S+)(\\s|\\n)+(?i:THEN)(\\s|\\n)+(.+)\\z", Pattern.DOTALL);
   Pattern delayTimePat = Pattern.compile("(?i)\\s*((-?\\d+)\\s*(((sec(ond)?(s)?)?)|(min(ute)?(s)?)))?");
   String tmpTableName=null;
   String showTableIdxRemainder=null;
   static final String  UNKNOWN_ERROR_CODE = "UNKNOWN ERROR CODE";

   Pattern patternKeyPat = Pattern.compile("\\s*\\$\\$([a-zA-Z0-9_]+)\\$\\$\\s+(.*)",Pattern.DOTALL);
   Pattern patternValPat = Pattern.compile("\\s*(.*)(\\s*(.*)|$)");
   Pattern containsKeyPat = Pattern.compile("(\\.)?\\$\\$([a-zA-Z0-9_]+)\\$\\$",Pattern.DOTALL);
   Pattern regExpPat = Pattern.compile("\\s*/(.*)/(.*/)",Pattern.DOTALL);
   Pattern regExpVal = Pattern.compile("(.+)/(.*)",Pattern.DOTALL);
   
   // For LDAP
   Pattern errPosPat = Pattern.compile("(.*)\\s+at line \\d+, column (\\d+)(.*)",Pattern.DOTALL);
   Pattern secErrPat = Pattern.compile(".*Details:[^:]*:(.*)", Pattern.DOTALL);

   private boolean isTracingOn = Boolean.getBoolean("trafci.enableTrace");

   
   Parser() {
	}
   public String getRemainderStr()
   {
      return remainderStr;
   }
   public void setRemainderStr(String remainderStr)
   {
      this.remainderStr=remainderStr;
   }

   String getScriptToken()
   {
      Matcher mat=scriptPat.matcher(remainderStr);
      if (mat.find())
      {
         return mat.group(2);
      }
      else
         return null;
   }

   String getNextFileToken()
   {
      if (remainderStr == null)
         return null;
      Matcher mat=filePat.matcher(remainderStr);

      if (mat.find())
      {
         remainderStr=mat.group(4);

         if (mat.group(1) != null)
         {

            if (mat.group(1).startsWith("\"") && mat.group(1).endsWith("\""))
               return mat.group(1).substring(1,mat.group(1).length()-1);
            else
               return mat.group(1);
         }
         else
         {
            return null;
         }
      }
      else
         return null;
   }

   String getNextSectionToken()
   {
      if (remainderStr == null)
         return null;
      Matcher mat=sectionPat.matcher(remainderStr);

      if (mat.find())
      {
         remainderStr=mat.group(2);

         if (mat.group(1)!=null)
         {
            return (mat.group(1));
         }
         else
         {
            return null;
         }
      }
      else
         return null;
   }

   String getNextNumberToken()
   {
      if (remainderStr == null)
         return null;

      Matcher mat=numberPat.matcher(remainderStr);
      if (mat.find())
      {
         remainderStr=mat.group(4);
         return mat.group(1);
      }
      else
         return null;
   }

   String getNextKeyToken()
   {
      if (remainderStr == null)
         return null;

      Matcher mat=keyPat.matcher(remainderStr);
      if (mat.find())
      {
         remainderStr=mat.group(2);
         return mat.group(1);
      }
      else
         return null;
   }
   
   String lookAtNextToken()
   {
      if (remainderStr == null)
         return null;

      Matcher mat=valPat.matcher(remainderStr);
      if (mat.find())
      {
         return mat.group(1);
      }
      else
         return null;
   }

   String getNextParamToken()
   {
      if (remainderStr == null)
         return null;

      Matcher mat=paramPat.matcher(remainderStr);
      if (mat.find())
      {
         remainderStr=mat.group(3);
         return mat.group(2);
      }
      else
         return null;
   }

   String getNextPreparedStmtNameToken()
   {
      if (remainderStr == null)
         return null;
      Matcher mat=pstmtPat.matcher(remainderStr);
      if (mat.find())
      {
         remainderStr=mat.group(3);

         if (mat.group(2)!=null)
         {
            return (mat.group(2).substring(1,mat.group(2).length()-1));
         }
         else
         {
            return mat.group(1).toUpperCase();
         }
      }
      else
         return null;

   }

   public List<String> getNextParamListToken()
   {
      List<String> paramList=null;
      paramList=new ArrayList<String>();
      String paramName=null;
      while (true)
      {
         if ((paramName=getNextParam2Token()) != null)
         {
            paramList.add(paramName);
         }
         else
         {
            break;
         }
      }

      return paramList;
   }

   public String getNextParam2Token()
   {


      if (remainderStr == null)
      {
         return null;
      }
      Matcher mat=paramListPat.matcher(remainderStr);
      if (mat.find())
      {
         //System.out.println(mat.group(3));
         remainderStr=mat.group(6);

         //showTableIdxRemainder=mat.group(7);

         if (mat.group(2) != null)
         {
            return mat.group(2).substring(1,mat.group(2).length()-1);
         }
         else if (mat.group(4) != null)
         {
            return mat.group(4).trim();
         }
         else
         {
            return null;
         }
      }
      else
      {
         if (remainderStr != null)
         {
            String tmpStr=remainderStr.trim();
            remainderStr=null;
            return tmpStr;
         }
         return null;
      }

   }
 //dead code. Removed by Kevin Xu
/*
   boolean matchExplainSyntax(List tokenList)
   {
      if (remainderStr == null)
         return false;

      Matcher mat=explainPat.matcher(remainderStr);
      if (mat.find())
      {
         //System.out.println("Return true");
         tokenList.add(mat.group(1));
         if (mat.group(4)!=null)
         {
            tokenList.add(mat.group(4).substring(1,mat.group(4).length()-1));
         }
         else
         {
            tokenList.add(mat.group(3).toUpperCase());
         }
         return true;

      }
      //System.out.println("Return False");
      return false;

   }
*/
   // the value can be any word without quotes
   // or any values including special characters enclosed within 
   // double quotes
   // if the value is enclosed within double quotes return value should 
   // retain the case and remove the double quotes otherwise the return
   // value should have all upper case 
   String getNextValueToken()
   {
      if (remainderStr == null)
         return null;

      Matcher mat=valPat.matcher(remainderStr);

      if (mat.find())
      {
         remainderStr=mat.group(5);
         if (mat.group(2) != null)
         {
            return mat.group(2).substring(1,mat.group(2).length()-1);
         }
         else if (mat.group(4) != null)
         {
            return mat.group(4).toUpperCase();
         }
         else
         {
            return null;
         }
      }
      else
      {
         return null;
      }
   }


   String getNextQValueToken()
   {
      if (remainderStr == null)
         return null;

      Matcher mat=valPat.matcher(remainderStr);

      if (mat.find())
      {
         remainderStr=mat.group(5);
         if (mat.group(2) != null)
         {
            return mat.group(2);
         }
         else if (mat.group(4) != null)
         {
            return mat.group(4).toUpperCase();
         }
         else
         {
            return null;
         }
      }
      else
      {
         return null;
      }
   }
   
   String getCatalogToken()
   {
	  if (remainderStr == null)
	     return null;

	  Matcher mat=catalogPat.matcher(remainderStr);

	  if (mat.find())
	  {
	    remainderStr=mat.group(2);
	    if (mat.group(1).startsWith("\"") )
           return mat.group(1);
        else
           return mat.group(1).toUpperCase();
	  }
	  else
	  {
	    return null;
	  }
   }

   /**
    *  Gets the value of a parameter (for a  set param call). Uses a
    *  regular expression to match the input to a grammar/syntax.
    *
    *  @param   cmdInput  if its null uses the remainder string we have
    *                     stored internally in here.
    *  @return  Null or a parameter value object (with the charset and input)
    *  @since   R2.4 SP2
    *
    */

    ParamStringObject  getParamValue(String cmdInput) {
    boolean  setRemainder = false;

      try {

         /**
          *  Check if we any parameter passed -- if not use the remaining
          *  input we have.  And also remember that we need to set the 
          *  remainder if there's any input remaining.
          */
         if (null == cmdInput) {
            cmdInput = remainderStr;
            setRemainder = true;
         }

        
         /**
          *  Check if we have any input to work on.
          */
         if (null == cmdInput)
            return null;


         if (isTracingOn)
            System.out.println("@@@Trace: getParamValue:: cmd input = " +
                               cmdInput);

         /**
          *  Check if we have a match and get the value for the parameter.
          */
         Matcher mat = paramValuePat.matcher(cmdInput);

         if (!mat.find() )
            return null;


         /**
          *  All the unmatched portion is in the eighth group. Use that to
          *  set the remainder string IFF we need to do that.
          */
         if (setRemainder)
            remainderStr = mat.group(8);

         if (isTracingOn)
            System.out.println("@@@Trace: getParamValue:: remainderStr = " +
                               mat.group(8) );


         /**
          *  The seventh group is any simple string values or error cases.
          *  If we have a seventh group and its not null return that value.
          *  This is match for the (\\S+) component of the regexp.
          */
         String theValue = mat.group(7);
         if (null != theValue)
            return  new ParamStringObject(null, null, theValue);


         /**
          *  Otherwise its a charset + hexinput + value type of string.
          *  Group 3 is the charset, group 4 is the hex input prefix 'x'
          *  and group 5 is the actual String value. Reassemble those.
          */
         ParamStringObject psv = new ParamStringObject(mat.group(3), mat.group(4),
                                                   mat.group(5) );

         if (isTracingOn)
            System.out.println("@@@Trace: getParamValue:: theValue = " +
                               psv.toString() );

         return psv;

      } catch(Throwable t) {
         if (isTracingOn) {
            System.out.println("Internal error getting parameter value. " + 
                               "Details = " + t.getMessage() );

            t.printStackTrace();
         }

      }

      return null;

   }   /*  End of  getParamValue  method.  */

   String getNextValueType2Token()
   {
      if (remainderStr == null)
         return null;

      Matcher mat=val2Pat.matcher(remainderStr);

      if (mat.find())
      {
         remainderStr=mat.group(5);
         if (mat.group(2) != null)
         {
            return mat.group(2).substring(1,mat.group(2).length()-1);
         }
         else if (mat.group(4) != null)
         {
            return mat.group(4).toUpperCase();
         }
         else
         {
            return null;
         }
      }
      else
         return null;
   }


   boolean hasMoreTokens()
   {
      if (remainderStr == null || remainderStr.trim().length()== 0)
      {
         return false;
      }
      else
      {
         return true;
      }
   }
 //dead code. Removed by Kevin Xu
/*
   String ignoreCommentsOld(String queryStr)
   {
      if (queryStr != null)
      {
         if (queryStr.indexOf("--") != -1)
         {
            StringBuffer outBuffer=null;
            outBuffer=new StringBuffer();
            Matcher ignoreCommentsPat = this.ignoreCommentsPat.matcher(queryStr);
            while (ignoreCommentsPat.find())
            {
               if (ignoreCommentsPat.group(2) != null)
               {
                  //group 2 is not null..dont have to print anything

               }
               else if (ignoreCommentsPat.group(4) != null || ignoreCommentsPat.group(6) != null)
               {
                  outBuffer.append(ignoreCommentsPat.group(3));
               }
               else
               {

                  outBuffer.append(ignoreCommentsPat.group(0));
               }
            }
            return outBuffer.toString();
         }
      }
      return queryStr;
   }
*/
   public List<String> getCSTList(String tablePattern)
   {
      List<String> cstList=null;
      cstList=new ArrayList<String>();
      String tabName=null;
      tmpTableName=null;
      tmpTableName=tablePattern;
      while (true)
      {
         if ((tabName=getNextTableToken(tmpTableName)) != null)
         {
            cstList.add(tabName);
         }
         else
         {
            break;
         }
      }
      return cstList;
   }



   public String getNextTableToken(String tablePattern)
   {
      if (tablePattern == null)
      {
         return null;
      }
      Matcher mat=tabPat.matcher(tablePattern);

      if (mat.find())
      {
         tmpTableName=mat.group(6);

         showTableIdxRemainder=mat.group(7);

         if (mat.group(2) != null)
         {
            return mat.group(2).replaceAll("\"","\\\"");
         }
         else if (mat.group(4) != null)
         {
            return mat.group(4).toUpperCase();
         }
         else
         {
            return null;
         }
      }
      else
      {
         return null;
      }

   }
   
 //dead code. Removed by Kevin Xu
/*
   boolean matchMxciParamPat(String param)
   {
      Matcher mat = mxciParamPat.matcher(param);
      if (mat.find())
      {
         remainderStr = mat.group(1).trim();
         return true;
      }
      else
      {
         remainderStr = null;
         return false;
      }
   }
*/
   public List<String> splitLines()
   {
      List<String> paramList=null;
      paramList=new ArrayList<String>();
      String paramName=null;
      while (true)
      {
         if ((paramName=getNextToken()) != null)
         {
            paramList.add(paramName);
         }
         else
         {
            break;
         }
      }

      return paramList;
   }

   public String getNextToken()
   {

      if (remainderStr == null)
      {
         return null;
      }
      Matcher mat=mapListPat.matcher(remainderStr);
      if (mat.find())
      {
         remainderStr=mat.group(6);

         if (mat.group(1) != null)
         {
            return mat.group(1);
         }
         else
         {
            return null;
         }
      }
      else
      {
         if (remainderStr != null)
         {
            String tmpStr=remainderStr.trim();
            remainderStr=null;
            return tmpStr;
         }
         return null;
      }

   }
 //dead code. Removed by Kevin Xu
/*
   List split(String line)
   {
      List tokList=null;
      tokList=new ArrayList();
      String token=null;
      this.setRemainderStr(line);

      while (true)
      {
         if ((token=getNextValueType2Token()) != null)
         {
            tokList.add(token);
         }
         else
         {
            break;
         }
      }
      return tokList;
   }

   List matchFixedWidthPattern(String param)
   {
      List posList=null;
      Matcher mat = mapFileFixedWidthColPat.matcher(param);
      if (mat.find())
      {
         posList = new ArrayList();
         posList.add(mat.group(1));
         posList.add(mat.group(2));

      }
      return posList;
   }

   public String matchDbFnPattern(String str)
   {
      Matcher  mat = dbFunctionPat.matcher(str);
      if (mat.find())
      {
         String fnname = mat.group(1);
         String tmp[] = mat.group(2).split(",");
         if (tmp != null)
            str = str.replaceFirst(tmp[0],"?");
         else
            str = str.replaceFirst(fnname,"?");
         return fnname;
      }
      return null;
   }

   public String getEnvVarName()
   {
      if (remainderStr == null)
         return null;
      Matcher mat = envPat.matcher(remainderStr);
      if (mat.find())
      {
         remainderStr=mat.group(2);
         return mat.group(1);
      }
      else
      {
         return null;
      }
   }
*/
   public String getConnArg(String argString)
   {
      if (remainderStr == null || argString==null)
      {
         return null;
      }
      Matcher mat =null;
      if (argString.equalsIgnoreCase("user"))
      {
    	  mat=connUserPreparePat.matcher(remainderStr.trim());
     	 if(mat.find())
     	 {
     		 String userPrepare = mat.group(1).trim();
			 if (userPrepare.indexOf("/") < 0
						&& !(userPrepare.indexOf("\"") == 0 && userPrepare
								.lastIndexOf("\"") == userPrepare.length() - 1))
     			 mat=connUserPat.matcher(remainderStr);
     		 else
     			 mat.reset();
     	 }
     	 else
     		 mat=connUserPat.matcher(remainderStr);
      }
      else if (argString.equalsIgnoreCase("pass"))
         mat=connPassPat.matcher(remainderStr);
      else if (argString.equalsIgnoreCase("server"))
         mat=connServerPat.matcher(remainderStr);
      else if (argString.equalsIgnoreCase("dsn"))
         mat=connDsnPat.matcher(remainderStr);
      else
         return null;

      if (mat.find())
      {
         remainderStr=mat.group(mat.groupCount());
         if (argString.equalsIgnoreCase("pass") && 
        		 mat.group(1).startsWith("\"") && mat.group(1).endsWith("\""))
         {
        	String password = mat.group(1).substring(1,mat.group(1).length()-1);
        	password = password.replaceAll("\\\\\"","\\\"");
            return password;
         }
         if (argString.equalsIgnoreCase("user"))
         {
        	 String user = mat.group(1).trim();
        	 if (user.startsWith("\"") && user.endsWith("\""))
        		 return user.substring(1,user.length()-1);
         }
                 
         return mat.group(1);
      }
      else
      {
         return null;
      }
   }
   
	public String getErrorCode(String errStr) {
		Matcher mat = errPat.matcher(errStr);
		if (mat.find())
			return mat.group(2);
		else
			return UNKNOWN_ERROR_CODE;
	}
	   
	public String getErrorCode(String errStr, int errCode) {
		Matcher mat = errPat.matcher(errStr);
		if (mat.find())
			return mat.group(2);
		else if (errCode != 0)
			return String.valueOf(errCode);
		else
			return UNKNOWN_ERROR_CODE;
	}

   public String getErrorMsg(String errStr)
   {

      Matcher mat = errPat.matcher(errStr);
      if (mat.find())
         return mat.group(3).trim();
      else
         return errStr;
   }

   public String getNDCSProduct(String vproc)
   {
      Matcher mat = ndcsProductInfo.matcher(vproc);
      if (mat.find())
      {
         return mat.group();
      }
      else
      {
         return null;
      }
   }
   public String getSysInfo(String sysinfo)
   {

      Matcher mat = sysInfo.matcher(sysinfo);
      if (mat.find())
         return mat.group(1);
      else
         return null;
   }

   public String gett4Version(String verTxt)
   {

      Matcher mat = t4Version.matcher(verTxt);
      if (mat.find())
         return mat.group(1);
      else
         return null;
   }

   public String getWMSErr(String errTxt)
   {

      Matcher mat = wmsErrPat.matcher(errTxt);
      if (mat.find())
         return mat.group(2);
      else
         return errTxt;
   }
    
   /************** Begin Conditional Parser Functions ***************************/
   // group(2) is condition
   // group(6) is operator
   // group(8) is value
   // group(13) is action
   
   public String getConditionalVariable(String txt){
       Matcher mat = condPat.matcher(txt);

       if(mat.find())
       {
           return mat.group(2).trim();
       }
       else
           return null;
   }
   
   public String getConditionalOperator(String txt){
       Matcher mat = condPat.matcher(txt);
       if(mat.find())
           return mat.group(6);
       else
           return null;
   }
   
   public String getConditionalValue(String txt){
       Matcher mat = condPat.matcher(txt);
       if(mat.find())
           return mat.group(8);
       else
           return null;
   }
   
   public String getConditionalAction(String txt){
       String action = null;
       Matcher mat = condPat.matcher(txt);
       if(mat.find())
       {
           action = mat.group(13).replaceAll("\\n", " ").trim();
       }
       
       return action;
   }

   /************** End Conditional Parser Functions *****************************/   

   //   Identifies the comments in a given SQL query line
   // if a comment string "--" is embedded between single quotes
   // then it is treated as string literal otherwise its a comment
   String ignoreComments(String qryLine)
   {
      String qryLineCopy=qryLine;
      int quoteStartPos=-1;                          // first single quote position
      int quoteEndPos=-1;                            // second single quote or end quote position
      int commentPos=-1;                             // comments position
      int fromIndex=0;                               // search start position in the querystring for quotes
      int commentFromIndex=0;                        // search start position in the querystring for comment
      String quoteStr="'";
      String commentStr="--";

      while (qryLineCopy !=null)
      {

         quoteStartPos=qryLine.indexOf(quoteStr,fromIndex);
         commentPos=qryLine.indexOf(commentStr,commentFromIndex);

         // if no comments found return qryLine as it is.
         if (commentPos == -1) return qryLine;

         // if no quotes found or if the comment appears before the quote remove the comment portion
         // from the query string and return it
         // else set the fromIndex position to find the ending quote.
         if (quoteStartPos == -1 || quoteStartPos > commentPos) return qryLine.substring(0,commentPos);
         else
            fromIndex=quoteStartPos+1;

         quoteEndPos=qryLine.indexOf(quoteStr,fromIndex);

         // if no ending quote is found remove the comments portion and return the string
         if (quoteEndPos==-1 && commentPos != -1) return qryLine.substring(0,commentPos);

         // if comment begins after ending quote , search for more quotes without resetting commentsPosition
         if (quoteEndPos < commentPos)
         {
            qryLineCopy=qryLine.substring(quoteEndPos+1,qryLine.length());
            fromIndex=quoteEndPos+1;
            continue;
         }

         // if comment appears between quotes , search for more quotes and comments and repeat the while loop
         // by resetting comments position
         if (quoteStartPos < commentPos && commentPos < quoteEndPos && qryLine.length()>quoteEndPos+1)
            qryLineCopy=qryLine.substring(quoteEndPos+1,qryLine.length()-1);

         fromIndex=quoteEndPos+1;
         commentFromIndex=fromIndex;

      }

      return qryLine;

   }

   public String replaceShowPattern(String showPatStr)
   {
      showPatStr = showPatStr.replaceAll(showWildCardPattern, "%");
      showPatStr = showPatStr.replaceAll(showWildCardPatternChar, "_");
      return showPatStr;

   }

   public String replaceShowPrepPattern(String showPatStr)
   {
      showPatStr = showPatStr.replaceAll(showWildCardPattern, ".*");
      showPatStr = showPatStr.replaceAll(showWildCardPattern2, "*");
      showPatStr = showPatStr.replaceAll(showWildCardPatternChar, "_");
      return showPatStr;

   }
   
   String getNextWordToken()
   {
      if(remainderStr == null)
        return null;
       
      Matcher mat=wordPattern.matcher(remainderStr);
      if(mat.find())
      {
         remainderStr=mat.group(2);
         return mat.group(1);
      }
      else
      {
         return null;
      }
   }
   
   String getDbVersion(String sutVer)
   {
      if (sutVer == null)
         return null;
       
      Matcher mat=dbProdVersionPat.matcher(sutVer);
      if(mat.find())
      {
         String minorVersion = mat.group(4);
         if (minorVersion.length() > 1 && minorVersion.startsWith("0"))
            minorVersion = minorVersion.substring(1);
         return mat.group(3) + "." + minorVersion;
       }
       else
       {
           return null;
       }
   }
   
   String formatSutVer(String sutVer)
   {
	   if (sutVer == null)
	         return "Information not available";
	       
	   Matcher mat=dbProdVersionPat.matcher(sutVer);
	   if(mat.find())
	   {
	         return mat.group(1);
	   }
	   else
	   {
	           return "Information not available";
	   }
	   
   }
   
   public int getDelayTime(String cmdStr)
   {
      int timeVal=-1;
      Matcher mat = delayTimePat.matcher(cmdStr);
      if (mat.matches())
      {
         if (mat.group(2)!=null)
         {
            try {
                timeVal = Integer.parseInt(mat.group(2)) * 1000;
            }catch (NumberFormatException nfe)
            {
               return -1;
            }
         }
         if (mat.group(8)!=null)
         {
            timeVal = timeVal * 60 ;
         }
       
      }
      return timeVal;
   }
   
   
   public String matchKeyPat(String cmdStr)
   {
	   Matcher patMat = containsKeyPat.matcher(cmdStr);
	   if (patMat.matches())
	   {
		   return patMat.group(2);
	   }
	   return null;
   }
   
   public Matcher getKeyPatternMatcher(String cmdStr)
   {
	   Matcher patMat = containsKeyPat.matcher(cmdStr);
	   return patMat;
	  
   }
   
   public String getPatternKeyPattern()
   {
	   Matcher patMat = patternKeyPat.matcher(remainderStr);
	   if (patMat.matches())
	   {
		  remainderStr=patMat.group(2);
	   	  return patMat.group(1);
	   	  
	   }
	   return null;
   }
   
   public String getPatternValueToken()
   {
	   Matcher patMat = patternValPat.matcher(remainderStr);
	   if (patMat.matches())
	   {
		  remainderStr=patMat.group(2);
	   	  return patMat.group(1);
	   	  
	   }
	   return null;
   }
   public String getRegexpPattern()
   {
	   Matcher regExpMat = regExpPat.matcher(remainderStr);
	   if (regExpMat.matches())
	   {
		  remainderStr=regExpMat.group(2);
	   	  return regExpMat.group(1);
	   	  
	   }
	   return null;
   }
   
   public String getRegexpValue()
   {
	   Matcher regExpMat = regExpVal.matcher(remainderStr);
	   if (regExpMat.matches())
	   {
		  remainderStr=regExpMat.group(2);
	   	  return regExpMat.group(1);
	   	  
	   }
	   return null;
   }
   
   
   public int getErrorPos(String errorMsg)
   {
	   Matcher errorPosMat = errPosPat.matcher(errorMsg);
	   if (errorPosMat.matches())
	   {
		 return new Integer(errorPosMat.group(2)).intValue();
	   }
	   return 0;
   }
   
   
   public String getSECErr(String errTxt)
   {
      Matcher mat = secErrPat.matcher(errTxt);
      if (mat.find())
         return mat.group(1);
      else
         return errTxt;
   }
}

