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

import java.io.IOException;
import java.util.ArrayList;

public class HTMLDisplay implements IDisplay {

	private Query qryObj;
	private Writer writer=null;
	private Session sessObj=null;
	
	public HTMLDisplay(Session sessObj)
	{
		if(sessObj!=null)
		{
			this.sessObj = sessObj;
			this.writer=sessObj.getWriter();
			this.qryObj=sessObj.getQuery();
		}
	}

	public void output(OutputContents contents) throws IOException {
		// TODO Auto-generated method stub
		if(contents!=null)
		{
			this.writer=sessObj.getWriter();
			this.qryObj=sessObj.getQuery();
			//display header
			ArrayList<OutputColumnAttribute> columns = contents.getColumns();
			qryObj.setColCount(String.valueOf(columns.size()));
			//display columns name
			for(int i=0;i<columns.size();i++)
			{
				OutputColumnAttribute columnAttr = columns.get(i);
				sessObj.getHtmlObj().processHtml(columnAttr.getDisplayFormatName());
	            writer.writeln();
			}
			
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
							sessObj.getHtmlObj().processHtml(alRow.get(j));
				            writer.writeln();
						}
					}
					
				}
			}
		}
	}

}
