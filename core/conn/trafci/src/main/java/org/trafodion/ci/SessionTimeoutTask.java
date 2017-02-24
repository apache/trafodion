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

import java.sql.Connection;
import java.util.TimerTask;

public class SessionTimeoutTask extends TimerTask
{
   Connection conn=null;
   int idleTime=0;
   long lastQueryExecTime=0;
   boolean checkRequired=true;

   /*
    *  Variable to hold the session we are associated with.
    */
   private Session _sessionObj = null;


   SessionTimeoutTask(Connection conn)
   {
      this.conn=conn;
   }




   /*
    *  Sets the session this timeout task is associated with.
    *
    *  @param sessObj  the session this timeout task is associated with.
    */
   public void setSessionObj(Session sessObj) {
       this._sessionObj = sessObj;

   }  /*  End of  setSessionObj  method.  */



   public void run()
   {

      if (checkRequired)           // check if this task has to run
      {
         /*
          *  Reset the connection idle timer (if we can!!).
          */
         if (null != this._sessionObj)
            this._sessionObj.resetConnectionIdleTimer();


         if (idleTime != 0)       // check only if the idletime is not set to 0
         {
            if (((System.currentTimeMillis() - this.lastQueryExecTime)/(1000*60)) > idleTime)
            {
               try
               {
                  conn.close();
               }catch (Exception e)
               {
                  // dont care..
               }
               System.out.println();
               System.out.println("Session Expired.");
               System.exit(SessionDefaults.abruptExit);
            }
         }
      }


   }

}
