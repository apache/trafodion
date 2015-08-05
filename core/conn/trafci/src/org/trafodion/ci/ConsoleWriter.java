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

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.PrintStream;

public class ConsoleWriter
{
   private PrintStream ps=null;
   private PrintStream systemOS = System.out;
   private ByteArrayOutputStream out=null;
   private boolean consoleOut=true;

   ConsoleWriter()
   {
   }

   /*public void initializeByteArray()
   {
   out=new ByteArrayOutputStream();
   System.setOut(new PrintStream(out));
   consoleOut=false;
   }*/

   public void initialize() throws IOException
   {	   
	   setupOutputStream();
   }
   
   private void setupOutputStream() {
      if (consoleOut) {
    	  System.setOut(systemOS);
      }
      else
      {
         out=new ByteArrayOutputStream();
         System.setOut(new PrintStream(out));
      }
      
      this.ps = System.out;
   }


   public void println(String line)
   {
      this.ps.println(line);
   }

   public void print(String line)
   {
      this.ps.print(line);
   }

   public void println()
   {
      this.ps.println();
   }

   public void setConsoleOut(boolean consoleOut)
   {
      this.consoleOut=consoleOut;
      setupOutputStream();
   }

   public boolean getConsoleOut()
   {
      return this.consoleOut;
   }
   /*
   public void println(String line)
   {
   System.out.println(line);
   }

   public void print(String line)
   {
   System.out.print(line);
   }

   public void println()
   {
   System.out.println();
   }
   */
   public String getResult()
   {
      if (out == null)
      {
         return null;
      }
      else
      {
         return out.toString();
      }
   }

   public void resetStream()
   {
      if (this.out != null)
      {
         this.out.reset();
      }
   }

   public void close()
   {
      this.ps.close();
   }

}
