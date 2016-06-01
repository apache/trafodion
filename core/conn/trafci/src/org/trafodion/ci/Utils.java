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

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.util.HashMap;
import java.util.regex.*;

public class Utils
{

   static HashMap<String, String> xmlHashMap = new HashMap<String, String>();

   Utils()
   {

   }



  static String trimSQLTerminator(String qryStr,String sqlTerminator)
   {
	  if (qryStr.toUpperCase().endsWith(sqlTerminator))
      {
		  qryStr = qryStr.substring(0,qryStr.length()- sqlTerminator.length());
      }
	  
      return qryStr;
   }

   String formatString(String output,int colWidth,char fillchar,String appendStr)
   {
      if (output == null)
      {
         return null;
      }



      if (appendStr != null)
      {
         colWidth=colWidth - appendStr.length();
      }

      output=formatString(output,colWidth,fillchar);

      if (appendStr != null && output != null)
      {
         output+=appendStr;
      }


      return output;

   }
 //dead code. Removed by Kevin Xu
/*
   String formatStringWordwrap(String output,int colWidth,char fillchar,String appendStr)
   {
      int beginIndex,endIndex;
      String valueStr;

      if (output == null)
      {
         return null;
      }

      // This is the case where the length of the value exceeds the
      // column width
      if ((appendStr !=null) && (appendStr.length()> colWidth-output.length()))
      {
         //print atleast five fill characters
         int labelWidth = output.length() + 5;
         StringBuffer blankOutput = new StringBuffer();
         for (int i=0;i<labelWidth;i++)
            blankOutput.append(' ');

         output=formatString(output,labelWidth,fillchar);

         //break the value into multiple strings to print
         //on separate lines
         colWidth = colWidth-labelWidth;
         for (beginIndex=0,endIndex=colWidth;
            endIndex<=appendStr.length();
            beginIndex=endIndex,endIndex+=colWidth)
         {
            valueStr = appendStr.substring(beginIndex,endIndex);
            if (beginIndex==0)
               output+=valueStr;
            else
               output = output + SessionDefaults.lineSeperator + blankOutput + valueStr ;

         }
         if (endIndex > appendStr.length())
            output = output + SessionDefaults.lineSeperator + blankOutput + appendStr.substring(beginIndex);

         return output;
      }

      if (appendStr != null)
      {
         colWidth=colWidth - appendStr.length();
      }

      output=formatString(output,colWidth,fillchar);

      if (appendStr != null && output != null)
      {
         output+=appendStr;
      }


      return output;

   }
*/
   String formatString(String output,int colWidth,char fillchar)
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

   // Change the string to lowerCase and capitalize the starting character and any character
   // followed by _ and remove _(under score)
 //dead code. Removed by Kevin Xu
/*
   String initCap(String value)
   {
      StringBuffer buffer=null;

      if (value == null)
         return value;

      buffer=new StringBuffer();

      char[] valueArray=value.toLowerCase().toCharArray();

      // capitalize the first character

      buffer.append(Character.toUpperCase(valueArray[0]));

      boolean changeCase=false;

      for (int i=1;i<valueArray.length;i++)
      {
         if (valueArray[i] =='_')
         {
            changeCase=true;
            i++;
         }
         else
            changeCase=false;

         if (changeCase)
            buffer.append(Character.toUpperCase(valueArray[i]));
         else
            buffer.append(valueArray[i]);
      }

      return buffer.toString();


   }
  
   public String trimSingleQuote(String value)
   {
      if (value!=null && value.startsWith("'") && value.endsWith("'"))
      {
         value=value.substring(1,value.length()-1);
         value=value.replaceAll("''","'");
      }
      return value;
   }
 */
   public String trimDoubleQuote(String value)
   {
      if (value!=null && value.startsWith("\"") && value.endsWith("\""))
      {
         value=value.substring(1,value.length()-1);
         //value=value.replaceAll("''","'");
      }
      return value;
   }
 //dead code. Removed by Kevin Xu
/*
   public String uppQuoteStr(String strValue)
   {
      if (strValue != null && !isQuoted(strValue))
         return strValue.toUpperCase();
      else
         return strValue;

   }

   boolean isQuoted(String source)
   {
      if (source != null)
      {
         source=source.trim();
         if (source.startsWith("\"") && source.endsWith("\""))
            return true;
         else
            return false;
      }

      return false;
   }
*/
   public String getTimeinWords(long time)
   {
      int hrs = getHrs(time);
      int mins = getMinutes(time);
      int secs = getSeconds(time);
      int msecs = getMilliseconds(time);
      StringBuffer sb = new StringBuffer();

      if (hrs > 0)
      {
         sb.append(hrs);
         sb.append(" hours and ");
      }
      if (mins > 0)
      {
         sb.append(mins);
         sb.append(" minutes and ");
      }
      //if (secs != 0) {
      if (secs > 0 && secs != 1)
      {
         sb.append(secs);
         sb.append(" seconds.");
      }
      if ((secs == 0 || secs == 1) && msecs >= 0)
         sb.append("1 second.");

      return sb.toString();
   }

   public String millisecondsToString(long time)
   {
      int milliseconds = getMilliseconds(time);
      int seconds = getSeconds(time);
      int minutes = getMinutes(time);
      int hours = getHrs(time);
      String millisecondsStr = (milliseconds<10 ? "00" : (milliseconds<100 ? "0" : ""))+milliseconds;
      String secondsStr = (seconds<10 ? "0" : "")+seconds;
      String minutesStr = (minutes<10 ? "0" : "")+minutes;
      String hoursStr = (hours<10 ? "0" : "")+hours;
      return new String(hoursStr+":"+minutesStr+":"+secondsStr+"."+millisecondsStr);

   }

   public int getMilliseconds(long time)
   {
      return (int)(time % 1000);
   }
   public int getSeconds(long time)
   {
      return (int)((time/1000) % 60);
   }
   public int getMinutes(long time)
   {
      return (int)((time/60000) % 60);
   }
   public int getHrs(long time)
   {
      return (int)((time/3600000) % 24);
   }

   public String formatXMLdata(String xmlData)
   {

      Pattern p = Pattern.compile("(<|&|\'|\"|>)");
      Matcher m = p.matcher(xmlData);
      StringBuffer strBuf = new StringBuffer();
      while (m.find())
      {
         m.appendReplacement(strBuf, (String) xmlHashMap.get(m.group() ));
      }
      m.appendTail(strBuf);
      return strBuf.toString();
   }

   static
   {
      xmlHashMap.put("&", "&amp;");
      xmlHashMap.put("<", "&lt;");
      xmlHashMap.put(">", "&gt;");
      xmlHashMap.put("\"", "&quot;");
      xmlHashMap.put("\'", "&apos;");
   }

   public String rtrim(String tmpStr)
   {
      tmpStr = tmpStr.replaceAll("\\s+$","");
      return tmpStr;
   }

   public String removeSpaces(String str)
   {
      str = str.replaceAll("\\s+", "");
      return str;
   }
   
   public static String execLocalhostCmd(String cmd)  throws IOException 
   {

      Runtime runtime = Runtime.getRuntime();
      Process proc = runtime.exec(cmd);

      InputStream is = proc.getInputStream();
      InputStreamReader isr = new InputStreamReader(is);
      BufferedReader bufReader = new BufferedReader(isr);

      String line="";
      StringBuffer sbuf =new StringBuffer("");
      while ((line = bufReader.readLine()) != null) {
        sbuf.append(line);
      }
   
      try {
        if (proc.waitFor() != 0) {
          //System.err.println("Exit Value = " +   proc.exitValue());
        }
      }
      catch (InterruptedException e) {
        System.err.println(e);
      }
      return sbuf.toString();
   }
   
   public static String internalFormat2(String name)
   {
       int index = -1;
       while((index = name.indexOf('"', index+2)) != -1)
       {
       	name = name.substring(0,index) + "\\" +name.substring(index);
       }
       return name;
   }
   
}
