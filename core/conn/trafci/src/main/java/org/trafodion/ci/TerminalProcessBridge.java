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

import java.io.*;


/**
 *   Virtual Terminal proxy (a process to a terminal bridge) -- reads from one end and writes to
 *   the other. Can't handle terminal sequences as yet. A "terminal" process bridge!! :^)
 */
class  TerminalProcessBridge  extends  Thread
{

   /**
   *   Static constants/defines -- DOOMDAY and time to yield the scheduler in milliseconds.
   */
   public  static  final  boolean  DOOMSDAY                  = true;
   public  static  final  int      DEFAULT_CPU_YIELD_TIME_MS = 300;   // default/first yield processor every 300 milliseconds.
   public  static  final  int      ONREAD_CPU_YIELD_TIME_MS  = 20;   // on read yield processor every 20 milliseconds.


   /**
   *   Static variable/knob to show/hide debugging details.
   */
   public  static  boolean  _debugOn = Boolean.getBoolean("trafci" + "." + "_lhdebug");


   /**
   *   Instance variables -- input and output streams + whether or not we should read 
   *   waited and whether or not we should keep reading.
   */
   InputStream   _is        = null;
   OutputStream  _os        = null;
   String        _tag       = "";
   boolean       _waitedOps = true;
   boolean       _readOn    = true;
   Writer        _writer     = null;




   /**
   *   TerminalProcessBridge Constructor. Serves as in output bridge between the 2 buddies.
   *
   *   @param  is  - Input stream to read from.
   *   @param  os  - Output stream to write to.
   */
   protected  TerminalProcessBridge(InputStream is, OutputStream os)
   {
      this(is, os, true);
   }


   protected  TerminalProcessBridge(InputStream is, Writer writer)
   {
      this(is, writer, true);
   }
   
   /**
   *   TerminalProcessBridge Constructor. Serves as an output bridge between 'em.
   *
   *   @param  is      - Input stream to read from.
   *   @param  os      - Output stream to write to.
   *   @param  waited  - Do waited reads on the input stream.
   */
   protected  TerminalProcessBridge(InputStream is, OutputStream os, boolean waited)
   {
      this._is        = is;
      this._os        = os;
      this._waitedOps = waited;
   }

   protected  TerminalProcessBridge(InputStream is, Writer writer, boolean waited)
   {
      this._is        = is;
      this._writer    = writer;
      this._waitedOps = waited;
   }


   /**
   *   Control method to indicate process went away and we don't need to read any more.
   *   Job's all done -- let just bow out gracefully.
   *
   */
   protected  void  stopReading()
   {
      synchronized(this)
      {
         this._readOn = false;
      }
   }



   /**
   *   Control method to set a tag for debugging purposes.
   *   @param  tag  - Tag Name.
   *
   */
   protected  void  setTag(String  tag)
   {
      this._tag = tag;

   }



   /**
   *   Thread entry point (main method). Does all the work. Reads from console/obey file
   *   as input is available and pipes it to the process' input stream.
   *
   *   @overrides  Thread.run
   */
   public void run()
   {
      try
      {
         BufferedWriter      output=null;
         if (this._os != null) {
            OutputStreamWriter  sendTo   = new OutputStreamWriter(this._os);
                                output   = new BufferedWriter(sendTo);
         }

         int  yieldTime = DEFAULT_CPU_YIELD_TIME_MS;

         /* put a new line at top of first output */
         boolean firstLine = true;

         /** 
         *  Keep doing this until we know the process completed and we don't need to
         *  read anymore.
         */
         while (DOOMSDAY)
         {
            synchronized(this)
            {
               if (!this._readOn)
                  break;

            }

            if (this._waitedOps ||  (0 < this._is.available()))
            {
               String  line        = null;
               int     numReadable = this._is.available();
               byte[]  bytes       = new byte[1];

               if (_debugOn)
                  System.out.println("** Debug*** ==>  available : " + numReadable);

               if (0 < numReadable)
                  bytes = new byte[numReadable];


               int  kitchenSink = this._is.read(bytes, 0, bytes.length);
               if (-1 == kitchenSink)
                  break;

               if (_debugOn)
                  System.out.println("** Debug*** ==>  read terminated with '" + kitchenSink +
                     "' bytes [length asked for was " + bytes.length + "]. ");


               line = new String(bytes);
               
               if(firstLine)
               {
                   line = "\n"+line;
                   firstLine = false;
               }
               
               if (_debugOn)
                  line = this._tag + line;

               if (_writer == null)
               {
                  output.write(line, 0, line.length() );
                  output.flush();
               }
               else
               {
                  _writer.write(line);
               }

               yieldTime = ONREAD_CPU_YIELD_TIME_MS;

            }
            else
            {
               try
               {
//                  Thread.currentThread().sleep(yieldTime);
            	   Thread.sleep(yieldTime);
               } catch (Exception e)
               {
               }

            }


         }   /*  End of  while(DOOMSDAY)  loop.  */

      } catch (Exception e)
      {
         if (TerminalProcessBridge._debugOn)
         {
            System.err.println("Internal error occurred.  Details : " + e.getMessage() );
            e.printStackTrace();
         }

      }

   }  /*  End of  run  method.  */


}  /*  End of  inner class  TerminalProcessBridge.  */










