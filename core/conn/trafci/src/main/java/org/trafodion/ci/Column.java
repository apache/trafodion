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

public class Column
{

   Column()
   {
   }

   int maxColSize = 0;
   int actualColValSize = 0;
   int maxDisplayColSize = 0;
   int currentIndexPos = 0;
   String columnName = null;

   boolean isColWrap = false;
   boolean isLeftAligned = true;
   boolean isDone = false;
   boolean isTrim=false;
   boolean isHeader = false;

   String colVal = null;
   String fillerStr = null;
   char fillChar = ' ';

   // This method set the length for each column
   public void setLength (int maxColSize, int maxDisplayColSize)
   {
      this.maxColSize = maxColSize;
      this.maxDisplayColSize = maxDisplayColSize;
      //Set columns wrapped to true if metadata colsize is greater than display colsize
      this.isColWrap = this.maxColSize >  this.maxDisplayColSize?true:false;
      //If columns are wrapped then, set the maximum display size to be that set by the session object
		if (!isColWrap)
			this.maxDisplayColSize = this.maxColSize;

      //this.maxDisplayColSize = this.isColWrap?this.maxDisplayColSize:this.maxColSize;
   }

   // This method sets the filler character of space for each column based on the data for each column
   public void setFillChar(char fillChar)
   {
      StringBuffer fillBuf = new StringBuffer();

      for (int i = 0; i < this.maxDisplayColSize; i++)
      {
         fillBuf.append(fillChar);
      }

      this.fillerStr = fillBuf.toString();
      fillBuf = null;
   }

   // This method set the column value
   public void setColValue(String colVal)
   {
      if (this.isTrim)
      {
         colVal= colVal.trim(); //Remove the extra blanks 
      }
      this.actualColValSize = colVal.length();
      //if column data length is greater than meta data legth then substring it to metadata length
      this.colVal = this.actualColValSize > this.maxColSize?colVal.substring(0, this.maxColSize):colVal;
      this.currentIndexPos = 0;
      this.isDone = false;
      this.isHeader =false;
   }

   public String getColValue()
   {

      if (isDone)
         return this.fillerStr;

      if (!this.isColWrap)
      {
         isDone = true;

         if (this.isLeftAligned || this.isHeader)
            return this.colVal.substring(this.currentIndexPos)+this.fillerStr.substring(this.colVal.substring(this.currentIndexPos).length(),this.maxDisplayColSize);
         else
            return this.fillerStr.substring(this.colVal.substring(this.currentIndexPos).length(),this.maxDisplayColSize)+this.colVal.substring(this.currentIndexPos);
      }

      if (currentIndexPos + this.maxDisplayColSize <= this.actualColValSize)
      {
         this.currentIndexPos += this.maxDisplayColSize;
         return this.colVal.substring(this.currentIndexPos - this.maxDisplayColSize,this.currentIndexPos);
      }
      isDone = true;
      if (this.isLeftAligned || this.isHeader)
         return this.colVal.substring(this.currentIndexPos)+this.fillerStr.substring(this.colVal.substring(this.currentIndexPos).length(),this.maxDisplayColSize);
      else
         return this.fillerStr.substring(this.colVal.substring(this.currentIndexPos).length(),this.maxDisplayColSize)+this.colVal.substring(this.currentIndexPos);
   }

   public void setLeftAligned(boolean isLeftAligned)
   {
      this.isLeftAligned = isLeftAligned;
   }

   public boolean isDone()
   {
      return this.isDone;
   }

   public void setTrim(boolean isTrim)
   {
      this.isTrim=isTrim;
   }
}
