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

public class FCQuery
{

   private Reader        reader;
   private Writer        writer;
   private Session       sessObj;
   private StringBuffer  qryText;
   private StringBuffer  newQryText;

   private int qryTextMaxLen;
   private int qryTextPos;
   private int commandPos;
   private char[] fcCommand;

   private String  fcPrompt;

   final static int INSERT_O             =  0;
   final static int DELETE_O             =  1;
   final static int REPLACE_O            =  2;
   final static int EXPLICIT_REPLACE_O   =  3;
   final static int ADVANCE_O            =  4;
   final static int ABORT_O              =  5;
   final static int END_O                =  6;
   final static int DONE_O               =  7;
   final static int AGAIN_O              =  8;
   final static int EMPTY_O              =  9;

   /*enum Option
   {
   INSERT_O, DELETE_O, REPLACE_O, EXPLICIT_REPLACE_O, 
   ADVANCE_O, ABORT_O, END_O, DONE_O, AGAIN_O, EMPTY_O
   };*/

   FCQuery(Session sessObj,Query qryObj)
   {
      this.sessObj = sessObj;
      this.reader = sessObj.getReader();
      this.writer = sessObj.getWriter();
      this.setFcPrompt();
   }

   public boolean editCommand() throws IOException, UserInterruption
   {
      boolean returnVal = true;
      newQryText = new StringBuffer();
      int option;

      String[] multiLineQry = sessObj.getQuery().getQueryText().split(SessionDefaults.lineSeperator);
      for (int lineNo=0; lineNo < multiLineQry.length ;lineNo++)
      {
         if (sessObj.isQuietEnabled() || !sessObj.isLogCmdText()) {
            writer.getConsoleWriter().println(sessObj.getSessionPrompt() + multiLineQry[lineNo]);
            writer.getConsoleWriter().print(fcPrompt);
         } 
         if (sessObj.isLogCmdText()) {
               writer.writeln(sessObj.getSessionPrompt() + multiLineQry[lineNo]); // print the line to be edited
               if (reader.getConsoleReader().isJline()) {
                   reader.getConsoleReader().setPrompt(fcPrompt, false, false);
               } else {
                   writer.write(fcPrompt); // print the fc prompt
               }
         }
         //read the fc command
         fcCommand= reader.getLine().toCharArray();
         qryText = new StringBuffer(multiLineQry[lineNo]);
         qryTextMaxLen = qryText.length();
         qryTextPos=0;
         commandPos=0;
         option=-1;

         while ((option != DONE_O) && (option != ABORT_O))
         {
            option = nextOption();
            switch (option)
            {
               case INSERT_O:
                  processInsert();
                  break;

               case REPLACE_O:
                  processReplace();
                  break;

               case EXPLICIT_REPLACE_O:
                  processReplace();
                  qryTextPos++;
                  break;

               case DELETE_O:
                  processDelete();
                  break;

               case ADVANCE_O:
                  qryTextPos += 1;
                  break;

               case END_O:
                  qryTextPos += 2;
                  break;

               case ABORT_O:
                  returnVal = false;
                  break;

               case AGAIN_O:
                  if (sessObj.isQuietEnabled() || !sessObj.isLogCmdText()) {
                     writer.getConsoleWriter().println(sessObj.getSessionPrompt() + qryText);
                     writer.getConsoleWriter().print(fcPrompt);
                  }
                  if (sessObj.isLogCmdText()) {
                       writer.writeln(sessObj.getSessionPrompt() + qryText );
                       if (reader.getConsoleReader().isJline()) {
                           reader.getConsoleReader().setPrompt(fcPrompt, false, false);
                       } else {
                           writer.write(fcPrompt);
                       }
                  }
                  fcCommand= reader.getLine().toCharArray();

                  if (!(fcCommand.length==0))
                  {
                     commandPos = 0;
                     qryTextPos = 0;
                  }
                  else
                  {
                     option = DONE_O;
                  }

                  break;

               case DONE_O:
                  break;

               default:
                  break;

            }//switch
         }//while

         if (option != ABORT_O)
         {
            if (newQryText.length() == 0)
               newQryText.append(qryText);
            else
               newQryText.append(SessionDefaults.lineSeperator + qryText);
         }
         else
         {
            return false;
         }
      }//for
      sessObj.getQuery().resetQueryText(newQryText.toString());
      return returnVal;
   }


   private int nextOption()
   {
      int option;

      //If the user just hits an enter key for the command, then we are done
      //with editing the current qryText
      if (fcCommand.length==0)
      {
         return (DONE_O);
      }
      // If you have reached the end of the FC command
      // give another chance to edit the command
      if (commandPos >= fcCommand.length)
      {
         return (AGAIN_O);
      }

      switch (fcCommand[commandPos])
      {
         case 'i':
         case 'I':
            option = INSERT_O;
            commandPos += 1;
            break;

         case 'd':
         case 'D':
            option = DELETE_O;
            commandPos += 1;
            break;

         case 'r':
         case 'R':
            option = EXPLICIT_REPLACE_O;
            commandPos += 1;
            break;

         case ' ':
            option = ADVANCE_O;
            commandPos += 1;
            break;

         case '/':

            if ((commandPos < fcCommand.length-1) && (fcCommand[commandPos+1] == '/'))
            {
               if ((commandPos == 0) && (fcCommand.length == 2))
                  option = ABORT_O;
               else
               {
                  option = END_O;
                  commandPos += 2;
               }
            }
            else
            {
               option = REPLACE_O;
            }

            break;

         default:
            option = REPLACE_O;
            break;
      }

      return option;
   }  // nextOption()

   private int getCommandLen()
   {
      int done = 0;
      int i = 0;

      while (done != -1)
      {
         if (commandPos + i >= fcCommand.length)
            done = -1;
         else if ((commandPos +i < fcCommand.length-1) && (fcCommand[commandPos+i] == '/') &&
            (fcCommand[commandPos+i+1] == '/'))
            done = -1;
         else
            i++;
      }
      return i;
   }  // getCommandLen

   void processInsert()
   {
      int commandLen = getCommandLen();

      if (commandLen > 0)
      {
         if (qryTextPos > qryText.length())
         {
            for (int j=qryText.length();j<qryTextPos;j++)
               qryText.append(" ");
            qryText.append(new String(fcCommand).substring(commandPos,commandPos+commandLen));
         }
         else
            qryText = qryText.insert(qryTextPos, fcCommand, commandPos, commandLen);

         commandPos += commandLen;
         qryTextPos += 2 * commandLen + 1;
         qryTextMaxLen += commandLen;

         if (qryTextPos > qryTextMaxLen)
            qryTextMaxLen = qryTextPos;

      }
   }  // processInsert()


   private void processDelete()
   {
      if (qryTextPos < qryTextMaxLen)
      {
         //qryText=new StringBuffer(qryText.substring(0,qryTextPos)+qryText.substring(qryTextPos+1));
         qryText = qryText.deleteCharAt(qryTextPos);
         qryTextMaxLen-=1;
      }

   }

   private void processReplace()
   {
      int commandLen = getCommandLen();

      if (commandLen > 0)
      {
         if (qryTextPos > qryText.length())
         {
            for (int j=qryText.length();j<qryTextPos;j++)
               qryText.append(" ");
            qryText.append(new String(fcCommand).substring(commandPos,commandPos+commandLen));
         }
         else
            qryText.replace(qryTextPos, qryTextPos+commandLen, new String(fcCommand).substring(commandPos,commandPos+commandLen));
         commandPos += commandLen;
         qryTextPos += commandLen;
         if (qryTextPos > qryTextMaxLen)
            qryTextMaxLen = qryTextPos;
      }
   }  // processReplace()


   public String getFcPrompt()
   {
      return this.fcPrompt;
   }


   public void setFcPrompt()
   {
      int len = sessObj.getSessionPrompt().length();
      String fcPrompt="";
      for (int i=0;i<len;i++)
         fcPrompt+=".";
      this.fcPrompt = fcPrompt;
   }
}
