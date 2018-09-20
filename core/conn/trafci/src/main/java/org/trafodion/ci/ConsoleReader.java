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
import java.io.InputStream;
import java.io.IOException;
import java.io.UnsupportedEncodingException;
import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Date;

import jline.console.UserInterruptException;
import sun.misc.Signal;
import sun.misc.SignalHandler;

public class ConsoleReader
{

   private BufferedReader br=null;
   private InputStream in=null;
   private String line=null;
   String newLine=SessionDefaults.lineSeperator;
   String defaultEncoding="ISO-8859-1";
   SignalHandler CTRLCHandler=null;
   Signal INTSignal=null;
   private boolean queryInterrupted=false;
   private boolean doTrace = Boolean.getBoolean("trafci.enableTrace");
   
   StringBuffer lineBuffer=null;
   private String prompt = SessionDefaults.DEFAULT_SQL_PROMPT;
   private boolean time = false;
   private boolean ampmFmt = false;
   private MaskingThread mt=null;
   WCIUtils wcs=null;

    private boolean isJline = false;
    jline.console.ConsoleReader cr = null;

    ConsoleReader() {
        this(false);
    }

    public ConsoleReader(boolean isJline) {
        this.isJline = isJline;
        newLine = System.getProperty("line.separator");
        defaultEncoding = System.getProperty("file.encoding");

        CTRLCHandler = new MySignalHandlerClass();
        try {
            INTSignal = new Signal("INT");
        } catch (Exception e) {
        }
    }

   public void setPrompt(String ps, boolean time, boolean ampmFmt)
   {
      if (isJline) {
          this.cr.setPrompt(ps);
      }
      this.prompt = ps;
      this.time = time;
      this.ampmFmt = ampmFmt;
   }

   public void setMaskingThread(MaskingThread mt, WCIUtils wcs)
   {
      this.mt = mt;
      this.wcs=wcs;
   }
 
   private class MySignalHandlerClass implements SignalHandler
   {
      public void handle(Signal sig)
      {
         if (null != lineBuffer)  
            lineBuffer.setLength(0);

         queryInterrupted = true;
        
         if (mt!=null)
         {
            mt.stopMasking();
            if (wcs!= null)wcs.enableEcho();
         }

         String timeStamp = "";
         SimpleDateFormat sdf = new SimpleDateFormat("HH:mm:ss ");
         if (time)
         {
            if (ampmFmt)
               timeStamp = DateFormat.getTimeInstance().format(new Date())+ " ";
            else
               timeStamp = sdf.format(new Date());
         }

         System.out.print("^C"+SessionDefaults.lineSeperator+SessionDefaults.lineSeperator + timeStamp+prompt);
      }
   }

   public void initialize() throws IOException
   {
      this.in=System.in;
      if (isJline) {
          cr = new jline.console.ConsoleReader();
          cr.setPrompt(this.prompt);
          cr.setHandleUserInterrupt(true);
          cr.setExpandEvents(false);
      }
   }

   public String getLine() throws IOException, UserInterruption
   {
      next();
      return this.line;
   }

   public String readLine() throws UnsupportedEncodingException, UserInterruption, IOException
   {
        if (!isJline) {
            StringBuffer lineBuffer = null;
            lineBuffer = new StringBuffer();
            int i = -1;

            try {
                if (CTRLCHandler != null) {
                    Signal.handle(INTSignal, CTRLCHandler);
                }
            } catch (Exception e) {
            }

            while (true) {
                queryInterrupted = false;
                this.in.mark(1);
                try {
                    if ((i = this.in.read()) != -1) {
                        if (queryInterrupted) {
                            in.reset();
                            throw new UserInterruption();
                        }

                        lineBuffer.append((char) i);
                        line = lineBuffer.toString();

                        if (!defaultEncoding.equalsIgnoreCase("Cp1047")) {
                            byte[] ba = line.getBytes("ISO-8859-1"); // added for nls character support
                            line = new String(ba, defaultEncoding);
                        }

                        if (line != null && line.endsWith(newLine)) {
                            return line.substring(0, line.length() - newLine.length());
                        }
                    }
                } catch (ArrayIndexOutOfBoundsException e) {
                    System.out.println(SessionError.OUT_OF_BOUNDS);

                    if (doTrace) {
                        e.printStackTrace();
                    }
                }

                if (queryInterrupted) {
                    in.reset();
                    throw new UserInterruption();
                }
            }
        } else {
            String readLine = null;
            try {
                readLine = cr.readLine();
            } catch (UserInterruptException e) {
            }
            return readLine;
        }
   }

   public boolean next() throws IOException, UserInterruption
   {
      //line=this.br.readLine();
      line=this.readLine();
      return true;
   }

   public void close() throws IOException
   {
      this.br.close();
      this.br=null;
   }

   public boolean isQueryInterrupted()
   {
      return queryInterrupted;
   }

   public void setQueryInterrupted(boolean queryInterrupted)
   {
      this.queryInterrupted = queryInterrupted;
   }

   public boolean isInteractive()
   {
      int num = 1;
      try
      {
         num = this.in.available();
         //System.out.println("\nNumber of input stream bytes=" + num);
      }
      catch (IOException ioe)
      {
         return false;
      }

      return (num <= 0 ? true:false);
   }

    public boolean isJline() {
        return isJline;
    }
}

