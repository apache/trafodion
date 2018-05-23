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

package org.trafodion.sql.ustat;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.util.Properties;
import java.util.Enumeration;


public class UstatUtil extends Thread
{

   static BufferedWriter bw=null;
   static boolean doneflag=false;
   static StringBuffer outputStr=null;
   static StringBuffer errStr=null;
   static boolean nextCommand=false;
   static boolean errorFlag=false;
   static boolean statusFlag=false;
   static Runtime rt=Runtime.getRuntime();
   static boolean srunStatus=false;
   final static String scriptIndexFile=".scriptIndex";  // script Index property file which contains the list of scripts that can be executed
   static String lineSeperator=System.getProperty("line.separator");


   public static void runStatsProfile(String arguments,String[] output) throws IOException
   {
      Process p;
      String command=" ";

      String os = System.getProperty("os.name").toLowerCase();

      String cmd_path;

      if ( os.indexOf("linux") >=0 ) {
         cmd_path = "sh " + System.getenv("TRAF_HOME") + "/export/lib";
      } else { // assume NSK
         cmd_path = "/usr/tandem";
      }

      String cmd= cmd_path + "/mx_ustat/stats_profile ";
      cmd=cmd+arguments;

      p = rt.exec(cmd);
      try
      {
         execute(command, p, output, false);
      } catch (IOException ioe)
      {
         statusFlag = false;
      }
   }

   public static void USASstop() throws IOException
   {
      Process p;
      String command=" ";

      String os = System.getProperty("os.name").toLowerCase();
      String cmd_path;

      if ( os.indexOf("linux") >=0 ) {
         cmd_path = "sh " + System.getenv("TRAF_HOME") + "/export/lib";
      } else { // assume NSK
         cmd_path = "/usr/tandem";
      }

      String cmd= cmd_path + "/mx_ustat/StopAutoStats.sh";

      String[] output=cmd.split("\\s+"); // Unused.

      p = rt.exec(cmd);
      try
      {
         execute(command, p, output, false);
      } catch (IOException ioe)
      {
         statusFlag = false;
      }
   }

/*
   public static void  handleSrun(String command, String[] output) throws IOException
   {

      Properties props=null;
      props=new Properties();
      String[] envList={};

      try
      {
         props.load(new FileInputStream("/usr/tandem/nvscript/admin/.scriptIndex"));
      } catch (FileNotFoundException fnfe)
      {
         output[0]="Could not find the index file.";
         return;
      }

      String[] commandArr=command.split("\\s+");
      if (props.getProperty(commandArr[0].trim()) == null)
      {
         if (commandArr[0] != null && !commandArr[0].trim().equals(""))
         {
            output[0]= "Invalid script.";
         }
         if (props.size() >0)
         {
            output[0]="The valid scripts are:" + lineSeperator + lineSeperator;
            Enumeration scriptNames=props.propertyNames();
            while (scriptNames.hasMoreElements())
            {
               String scriptName=(String)scriptNames.nextElement();
               output[0]+=format(scriptName,(String)props.get(scriptName))+ lineSeperator;
            }
         }
         outputStr=null;
         errStr=null;
         return;
      }

      srunStatus = true;
      Process p=rt.exec("/usr/bin/sh eval "+command,envList,new File("/usr/tandem/nvscript/script"));
      try
      {
         execute(command, p, output, srunStatus);
      } catch (IOException ioe)
      {
         statusFlag = false;
         outputStr.append("Could not create the sub process"+ioe);
      }
   }
*/
   public static void execute(String command,Process p, String[] output, boolean cmdStatus) throws IOException
   {

      outputStr=new StringBuffer();
      errStr=new StringBuffer();
      output[0]="";
      statusFlag = false;

      InputStream is = p.getInputStream();
      OutputStream os = p.getOutputStream();
      InputStream es = p.getErrorStream();

      InputStreamReader isr = new InputStreamReader(is);
      InputStreamReader iser = new InputStreamReader(es);
      OutputStreamWriter osw = new OutputStreamWriter(os);

      final BufferedReader br=new BufferedReader(isr);
      final BufferedReader ber=new BufferedReader(iser);

      bw = new BufferedWriter(osw);

      // output thread
      class OutputThread extends Thread
      {

         StringBuffer outputBuf=null;
         OutputThread(StringBuffer outputBuf)
         {
            this.outputBuf=outputBuf;
         }

         public void run()
         {
            int i=0;
            try
            {
               while ((i=br.read()) != -1)
               {
                  statusFlag = !statusFlag?true:statusFlag;
                  if (errorFlag)
                  {
                     Thread.yield();
                     errorFlag=false;
                     try
                     {
                        sleep(100);
                     } catch (InterruptedException ie)
                     {
                     }
                  }
                  if (nextCommand)
                  {
                     br.readLine();
                     nextCommand=false;
                  }else
                  {
                     outputBuf.append((char)i);
                  }
               }
               doneflag=true;
            } catch (IOException ote)
            {
               System.out.println("Error occurred in output Thread "+ote);
            }
         }
      };

      OutputThread outputt=new OutputThread(outputStr);
      outputt.start();

      // error thread
      class ErrorThread extends Thread
      {

         StringBuffer outputBuf=null;
         ErrorThread(StringBuffer outputBuf)
         {
            this.outputBuf=outputBuf;
         }

         public void run()
         {
            int i=0;
            try
            {
               while ((i=ber.read()) != -1)
               {
                  errorFlag=true;
                  outputBuf.append((char)i);
               }
            }catch (IOException ete)
            {
               System.out.println(" Error occurred in error thread "+ete);
            }
         }
      };

      ErrorThread errort=new ErrorThread(errStr);
      errort.start();

      // input thread
      try
      {
         p.waitFor();
         outputt.join();
         errort.join();
         if (!cmdStatus)
         {
            if (errStr.length() > 0)
            {
               errStr.delete(0, errStr.length());
	       //     errStr.append("An internal server error has occurred. Please contact support.");
            }
         }
         int count = errStr.indexOf("/sh:");
         if (count > 0)
            errStr.delete(0, count+5);

         outputStr.append(errStr);
      } catch (InterruptedException e)
      {
         // TODO Auto-generated catch block
         //.printStackTrace();
      }
      isr=null;
      iser=null;
      osw=null;
      bw=null;
      outputt=null;
      errort=null;
      cmdStatus=false;

      output[0]=outputStr.toString();
      outputStr=null;
      errStr=null;
   }
/*
   private static String format(String scriptName,String description)
   {

      if (scriptName == null)
      {
         return null;
      }
      StringBuffer sb=null;
      sb=new StringBuffer(scriptName);
      while (sb.length() < 12)
      {
         sb.append(" ");
      }
      if (description != null)
      {
         sb.append("-");
         sb.append(description);
      }
      return sb.toString().replaceAll(lineSeperator,lineSeperator + "            ");
   }
*/
/*
   public static void getTaclInfo(String command,String[] output) throws IOException
   {

      String[] commandArr=command.split("\\s+");
      Process p;

      if (commandArr.length == 1 && commandArr[0].equalsIgnoreCase("sutver"))
      {
         p = rt.exec("/usr/bin/sh eval  gtacl -c 'sutver'");
      }
      else if (commandArr.length == 2 && commandArr[0].equalsIgnoreCase("vproc") && commandArr[1].equalsIgnoreCase("$SYSTEM.ZMXODBC.MXOSRVR"))
      {
         p = rt.exec("/usr/bin/sh eval gtacl -c 'vproc $SYSTEM.ZMXODBC.MXOSRVR'");
      }
      else
      {
         output[0] = handleExceptions(commandArr[0]);
         return;
      }
      try
      {
         execute(command, p, output, false);
      } catch (IOException ioe)
      {
         statusFlag = false;
      }
   }

   public static void onlineDBdump(String command,String[] output) throws IOException
   {
      handleDbaCmd(command, output);
   }

   public static void handleDbaCmd(String command,String[] output) throws IOException
   {
      String[] envList = {};
      String[] commandArr=command.split("\\s+");
      Process p = null;
      String dbaScriptName = null;

      int len = commandArr.length;
      if (commandArr[0].equalsIgnoreCase("dbonlinedump"))
         dbaScriptName = "dbonlinedump";
      else if (commandArr[0].equalsIgnoreCase("updatestats"))
         dbaScriptName = "updatestats";

      if (dbaScriptName != null)
      {
         switch (len)
         {
            case 1:
               p = rt.exec("/usr/bin/sh eval " + dbaScriptName, envList, new File("/usr/tandem/nvscript/dbascripts"));
               break;
            case 2:
               if (commandArr[1].equalsIgnoreCase("INFO"))
                  p = rt.exec("/usr/bin/sh eval " + dbaScriptName, envList, new File("/usr/tandem/nvscript/dbascripts"));

               else
                  output[0] = handleExceptions(commandArr[0]);
               break;
            case 3:
               if (commandArr[1].equalsIgnoreCase("AT"))
                  p = rt.exec("/usr/bin/sh eval " + dbaScriptName + " AT " + commandArr[2], envList, new File("/usr/tandem/nvscript/dbascripts"));

               else
                  output[0] = handleExceptions(commandArr[0]);
               break;
            default:
               output[0] = handleExceptions(commandArr[0]);
               return;
         }
      }
      else
      {
         output[0] = handleExceptions(commandArr[0]);
         return;
      }

      try
      {
         execute(command, p, output, false);
      } catch (IOException ioe)
      {
         statusFlag = false;
      }
   }
*/

   public static String handleExceptions(String str)
   {

      str = "Invalid Command.";
      return str;
   }

}

/*
DROP PROCEDURE NEO.HP_USTAT.STATS_PROFILE;
CREATE PROCEDURE NEO.HP_USTAT.STATS_PROFILE
  (
    IN cmd VARCHAR(4000),
    OUT response VARCHAR(240)
  )
  EXTERNAL NAME 'UstatUtil.runStatsProfile'
  EXTERNAL PATH '/usr/tandem/mx_ustat'
  LANGUAGE JAVA
  PARAMETER STYLE JAVA
  NO SQL
  DYNAMIC RESULT SETS 0
  ;
DROP PROCEDURE NEO.HP_USTAT.STOP_AUTOMATED_STATS;
CREATE PROCEDURE NEO.HP_USTAT.STOP_AUTOMATED_STATS
  ()
  EXTERNAL NAME 'UstatUtil.USASstop'
  EXTERNAL PATH '/usr/tandem/mx_ustat'
  LANGUAGE JAVA
  PARAMETER STYLE JAVA
  NO SQL
  DYNAMIC RESULT SETS 0
  ;

*/
