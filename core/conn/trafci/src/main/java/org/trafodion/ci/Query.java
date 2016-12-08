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

public class Query
{
   private int queryType=-1 ;  // 0 -tool query 1- sql query
   private boolean passThrough=false; // false -for all interface query
   private boolean multiLine=false; // false for all interface query
   private StringBuffer queryText;
   private String rowCount=null; // false if the query does not succeed
   private int queryId=-1;// query identifier
   private boolean trimOut=false;
   private boolean isActive=false;
   private long elapsedTime;
   private String stmtType=null;
   private int statusCode=0; // 0-after successful execution of the query. corresponding error number otherwise
   private String colCount=null;
   private int rsCount=0;

	Query() {
		queryText = new StringBuffer();
	}

   public void setQueryType(int queryType)
   {
      this.queryType=queryType;
   }

   public int getQueryType()
   {
      return this.queryType;
   }

   public void setPassThrough(boolean  passThrough)
   {
      this.passThrough=passThrough;
   }

   public boolean getPassThrough()
   {
      return this.passThrough;
   }

   public void setMultiLine(boolean  multiLine)
   {
      this.multiLine=multiLine;
   }

   public boolean isMultiLine()
   {
      return this.multiLine;
   }

   public void setQueryText(String queryText)
   {
      this.queryText.append(queryText);
   }

   public void resetQueryText(String queryText)
   {
      this.queryText.replace(0,this.queryText.length(),queryText);
   }

   public String getQueryText()
   {
      return this.queryText.toString();
   }

   public void setRowCount(String rowCount)
   {
      this.rowCount=rowCount;
   }

   public void resetRowCount()
   {
      this.rowCount=null;
   }

   public String getRowCount()
   {
      return this.rowCount;
   }

   public void setColCount(String colCount)
   {
      this.colCount=colCount;
   }

   public String getColCount()
   {
      return this.colCount;
   }

   public int getQueryId()
   {
      return queryId;
   }

   public void setQueryId(int queryId)
   {
      this.queryId = queryId;
   }

   public boolean isTrimOut()
   {
      return trimOut;
   }

   public void setTrimOut(boolean trimOut)
   {
      this.trimOut = trimOut;
   }

   public boolean isActive()
   {
      return isActive;
   }

   public void setActive(boolean isActive)
   {
      this.isActive = isActive;
   }
   public long getElapsedTime()
   {
      return this.elapsedTime;
   }

   public void setElapsedTime(long elapsedTime)
   {
      this.elapsedTime=elapsedTime;
   }

   public String getStmtType()
   {
      return stmtType;
   }

   public void setStmtType(String stmtType)
   {
      this.stmtType = stmtType;
   }

   public int getStatusCode()
   {
      return statusCode;
   }

   public void setStatusCode(int statusCode)
   {
      this.statusCode = statusCode;
   }

   public int getRsCount()
   {
      return rsCount;
   }

   public void incrRsCount()
   {
      rsCount++;

   }

   public void setRsCount(int rsCount)
   {
      this.rsCount = rsCount;
   }
}
