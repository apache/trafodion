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

public class StatusThread extends Thread
{

   boolean stop=false;
   String beforeStatus=" In Progress";
   String afterStatus=" Complete";
   ConsoleWriter cwObj=null;


   /*
    *  Variable to hold the session for resetting the connection idle timer.
    */
   Session _sessionObj = null;


   StatusThread()
   {
   }


   /*
    *  Save the sessionObject as well to reset the connection idle timer.
    */
   StatusThread(ConsoleWriter cwObj, Session sessObj)
   {
      this.cwObj=cwObj;
      this._sessionObj = sessObj;
   }

   public void run()
   {
      /*
       *  Counter to hold the number of seconds that have elapsed since our
       *  last I_AM_ALIVE message w/ the NDCS server.
       */
      int elapsedSeconds = 0;


      cwObj.println();
      while (!stop)
      {
         cwObj.print("\rStatus :"+beforeStatus);
         for (int j=0;j<10;j++)
         {
            if (!stop)
            {
               try
               {
                  sleep(100);
               }
               catch (Exception e)
               {
               }
               System.out.print(".");
            }

         }
         cwObj.print("\rStatus:"+beforeStatus+"                       ");


         /*
          *  Reset the Connection idle timer if 60 seconds have elapsed.
          */
         elapsedSeconds++;
         if (60 < elapsedSeconds) {
            this._sessionObj.resetConnectionIdleTimer();
            elapsedSeconds = 0;
         }


      }
      cwObj.println("\rStatus:"+afterStatus+"                     ");

   }


}
