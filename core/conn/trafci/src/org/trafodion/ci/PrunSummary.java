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
/**
 * 
 * Object to handle PRUN summary fields 
 * 
 */
package org.trafodion.ci;

public class PrunSummary
{

   private int totalScriptFiles = 0;
   private int totalScriptFilesProcessed = 0;
   private int totalSQLsProcessed = 0;
   private int totalSQLErrors = 0;
   private int totalSQLWarnings = 0;
   private int totalConnections = 0;
   private int totalConnectionFailures = 0;
   private int totalSQLSuccesses = 0;

   public int getTotalConnections()
   {
      return totalConnections;
   }

   public void setTotalConnections(int totalConnections)
   {
      this.totalConnections = totalConnections;
   }

   public int getTotalScriptFiles()
   {
      return totalScriptFiles;
   }

   public void setTotalScriptFiles(int totalScriptFiles)
   {
      this.totalScriptFiles = totalScriptFiles;
   }

   public int getTotalScriptFilesProcessed()
   {
      return totalScriptFilesProcessed;
   }

   public void setTotalScriptFilesProcessed(int totalScriptFilesProcessed)
   {
      this.totalScriptFilesProcessed = totalScriptFilesProcessed;
   }

   public int getTotalSQLErrors()
   {
      return totalSQLErrors;
   }

   public void setTotalSQLErrors(int totalSQLErrors)
   {
      this.totalSQLErrors = totalSQLErrors;
   }

   public int getTotalSQLsProcessed()
   {
      return totalSQLsProcessed;
   }

   public void setTotalSQLsProcessed(int totalSQLsProcessed)
   {
      this.totalSQLsProcessed = totalSQLsProcessed;
   }

   public int getTotalSQLWarnings()
   {
      return totalSQLWarnings;
   }

   public void setTotalSQLWarnings(int totalSQLWarnings)
   {
      this.totalSQLWarnings = totalSQLWarnings;
   }

   public int getTotalSQLSuccess()
   {
      return totalSQLSuccesses;
   }

   public int getTotalConnectionFailures()
   {
      return totalConnectionFailures;
   }

   public void setTotalConnectionFailures(int totalConnectionFailures)
   {
      this.totalConnectionFailures = totalConnectionFailures;
   }

   public void setTotalSQLSuccess(int totalSQLSuccesses)
   {
      this.totalSQLSuccesses = totalSQLSuccesses;
   }
}
