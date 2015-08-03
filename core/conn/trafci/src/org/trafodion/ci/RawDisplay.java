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

import java.awt.FontMetrics;
import java.io.IOException;
import java.util.ArrayList;

public class RawDisplay implements IDisplay {

	private Writer writer=null;
	private Session sessObj=null;
	private int isoMapping;
	private FontMetrics fontMetrics = null;
	
	public RawDisplay(Session sessObj)
	{
		if(sessObj!=null)
		{
			this.sessObj = sessObj;
			this.writer=sessObj.getWriter();
			isoMapping = sessObj.getISOMapping();
		}
		fontMetrics = MultiByteUtils.getFontMetrics();
	}

	public void output(OutputContents contents) throws IOException {
		// TODO Auto-generated method stub

		if(contents!=null)
		{
			this.writer=sessObj.getWriter();
			//display header
			ArrayList<OutputColumnAttribute> columns = contents.getColumns();
			if(columns!=null && columns.size()>0)
			{
				//display columns name
				for(int i=0;i<columns.size();i++)
				{
					OutputColumnAttribute columnAttr = columns.get(i);
					formatOutput(columnAttr.name,columnAttr.width,' ',columnAttr.align);					
					writer.write(contents.getColumnSeparator());
				}
				writeln();
				
				//display column header separator
				for(int i=0;i<columns.size();i++)
				{
					OutputColumnAttribute columnAttr = columns.get(i);
					formatOutput("",columnAttr.width,'-',columnAttr.align);
					writer.write(contents.getColumnSeparator());
				}
				writeln();
			}
			
			//display contents
			ArrayList<ArrayList<String>> alRows = contents.getRows();
			if(alRows!=null && alRows.size()>0)
			{
				for(int i=0;i<alRows.size();i++)
				{
					ArrayList<String> alRow = alRows.get(i);
					if(alRow!=null && alRow.size()>0)
					{
						for(int j=0;j<alRow.size();j++)
						{
							OutputColumnAttribute columnAttr = columns.get(j);
							formatOutput(alRow.get(j),columnAttr.width,' ',columnAttr.align);
							
							writer.write(contents.getColumnSeparator());
						}
						writeln();
					}
					
				}
				writeln();
			}
		}
	}

	   void formatOutput(String output,int colWidth,char fillchar,int align) throws IOException
	   {

	     int outputStrLen = 0;
	      
	      if (output != null && output.length() > colWidth)
	      {
	         output=output.substring(0,colWidth);
	      } 
	      StringBuffer outBuffer=new StringBuffer(output);

	      if (sessObj.isMultiByteAlign() && (isoMapping == 10 || isoMapping == 15)) //SJIS or UTF8
	      {
	         try {
	         outputStrLen = getMultiByteColWidth(outBuffer);
	         }catch (Exception ex)
	         {
	            ex.printStackTrace();
	            throw new IOException(ex.getMessage());
	         }
	      }
	      else
	      {
	         outputStrLen = outBuffer.length();
	      }
	      //If the output is greater than the column width,
	      //truncate the output
	      
	      if (outputStrLen > colWidth)
	      {
	         outBuffer = new StringBuffer(outBuffer.substring(0,Math.min(colWidth, outBuffer.length())));
	         outputStrLen = outBuffer.length();
	      }

	      if ( outputStrLen <= colWidth)
	      {
	         for (int i=outputStrLen;i<colWidth;i++)
	         {
	            if (align == 0)
	            {
	               outBuffer.append(fillchar);
	            }
	            else
	            {
	               outBuffer.insert(0,fillchar);
	            }
	         }

	         writer.write( outBuffer.toString());

	      }
	      outBuffer=null;
	   }

	   protected int getMultiByteColWidth(StringBuffer output)
	   {
	      if (null == fontMetrics)
	         return output.length();
	      
	      int refWidth = fontMetrics.charWidth('w');
	      int width=0;
	     
	       for (int i=0; i< output.length(); i++)
	      {
	         int tmpLen=fontMetrics.charWidth(output.charAt(i));
	         //incorrect value of char width. Treat it as no-ASCII.
	         if(tmpLen==0 || tmpLen>fontMetrics.getMaxAdvance())
	         {
	        	 if(output.charAt(i)>256)
	        	 {
	        		 tmpLen=refWidth*2-1;
	        	 }
	        	 else
	        	 {
	        		 tmpLen=refWidth;
	        	 }
	         } 
	         
	         width += (tmpLen/refWidth) + (tmpLen%refWidth>0?1:0);
	      }
	      
	      return width;
	   }

	   void writeln() throws IOException
	   {
		   writer.write(SessionDefaults.lineSeperator);
	   }

}
