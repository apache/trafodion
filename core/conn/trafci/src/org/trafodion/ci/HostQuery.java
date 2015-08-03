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
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.io.OutputStreamWriter;


public class HostQuery extends Thread
{


   BufferedWriter bw=null;
   boolean doneflag=false;
   StringBuffer outputStr=null ;
   StringBuffer errorStr=null;
   boolean nextCommand=false;
   boolean errorFlag=false;
   boolean startInputThread=true;


   HostQuery()
   {
      bw=null;
      doneflag=false;
      outputStr=new StringBuffer() ;
      errorStr=new StringBuffer();
      nextCommand=false;
      errorFlag=false;
   }

   public void execute(String command,Reader reader, Writer writer)
   {
      Runtime rt=Runtime.getRuntime();
      if (reader == null)
      {
         System.out.println("reader is null");
      }
      if (writer == null)
      {
         System.out.println("writer is null");
      }
      final String newLine=System.getProperty("line.separator");


      try
      {
         final Process p=rt.exec(command);

         InputStream is=null;
         OutputStream os=null;
         InputStream es=null;

         is=p.getInputStream();
         os=p.getOutputStream();
         es=p.getErrorStream();

         InputStreamReader isr=null;
         InputStreamReader iser=null;
         OutputStreamWriter osw=null;

         isr=new InputStreamReader(is);
         iser=new InputStreamReader(es);
         osw=new OutputStreamWriter(os);

         final BufferedReader br=new BufferedReader(isr);
         final BufferedReader ber=new BufferedReader(iser);

         bw=new BufferedWriter(osw);
         //String tmpline=null;

         // output thread
         class OutputThread extends Thread
         {
            Writer writer;
            OutputThread(Writer writer)
            {
               this.writer=writer;
            }
            public void run()
            {
               int i=0;
               try
               {
                  while ((i=br.read()) != -1)
                  {
                     if (errorFlag)
                     {
                        Thread.yield();
                        errorFlag=false;
                        try
                        {
                           sleep(100);
                        }
                        catch (InterruptedException ie)
                        {
                        }
                     }
                     if (nextCommand)
                     {
                        br.readLine();
                        nextCommand=false;
                     }else
                     {
                        //System.out.print((char)i);
                        writer.write(""+(char)i);
                     }
                  }
                  doneflag=true;

               }catch (IOException ote)
               {
                  System.out.println("output thread exception "+ote);
               }

            }
         };

         OutputThread outputt=new OutputThread(writer);
         outputt.start();



         // error thread
         class ErrorThread extends Thread
         {
            Writer writer=null;
            ErrorThread(Writer writer)
            {
               this.writer=writer;
            }
            public void run()
            {
               int i=0;
               try
               {
                  while ((i=ber.read()) != -1)
                  {
                     errorFlag=true;
                     //System.out.print((char)i);
                     writer.write(""+(char)i);
                  }
               }catch (IOException ete)
               {
                  System.out.println("error thread exception "+ete);
               }
            }
         };

         ErrorThread errort=new ErrorThread(writer);
         errort.start();

         // input thread
         class InputThread extends Thread
         {
            Reader reader=null;
            InputThread(Reader reader)
            {
               this.reader=reader;
            }
            public void run()
            {
               String input=null;

               try
               {

                  while (!doneflag)
                  {
                     if (doneflag) break;
                     try
                     {
                        sleep(100);
                     } catch (InterruptedException e)
                     {
                        break;
                     }
                     input=reader.getNonBlankLine();
                     if (input != null)
                     {

                        try
                        {
                           //if the subprocess exited already..dont write anything to the pipe.
                           p.exitValue();

                           break;
                        }catch (IllegalThreadStateException itse)
                        {
                        }

                        nextCommand=true;
                        bw.write(input+newLine);
                        bw.flush();


                     }

                  }
               } catch (UserInterruption ui)
               {

               }catch (IOException ite)
               {
                  System.out.println("input thread exception "+ite);
               }
            }

         };

         InputThread inpt=new InputThread(reader);

         if (this.startInputThread)
         {
            inpt.start();
         }

         try
         {
            p.waitFor();
            errort.join();
            outputt.join();

            if (this.startInputThread)
            {
               inpt.interrupt();
               inpt.join();
            }


         } catch (InterruptedException e)
         {
            // TODO Auto-generated catch block
            //.printStackTrace();
         }



         isr=null;
         iser=null;
         osw=null;
         bw=null;
         inpt=null;
         outputt=null;
         errort=null;
         outputStr=null;
         errorStr=null;

      }
      catch (IOException ioe)
      {
         try
         {
            writer.write("Could not create the sub process"+ioe);
         } catch (IOException e)
         {
            // TODO Auto-generated catch block
            //e.printStackTrace();
         }
      }



   }

}
