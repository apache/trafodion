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

import java.util.ArrayList;

public class OutputContents {

	public static int LiftAlign = 0;
	public static int RightAlign = 1;
	public static int CenterAlign = 2;
	
	private String title="";
	private String itemName="";
	private String headerSeparator = "-";
	private String columnSeparator = " ";
	private ArrayList<OutputColumnAttribute> alColumns;
	private ArrayList<ArrayList<String>> alRows;
	
	public OutputContents()
	{
		alColumns = new ArrayList<OutputColumnAttribute>();
		alRows = new ArrayList<ArrayList<String>>();
	}

	public OutputContents(int columnCount)
	{
		alColumns = new ArrayList<OutputColumnAttribute>(columnCount);
		alRows = new ArrayList<ArrayList<String>>();
	}
	
	public ArrayList<String> newRow()
	{
		ArrayList<String> row = new ArrayList<String>(alColumns.size());
		alRows.add(row);
		return row;
	}
	
	public ArrayList<ArrayList<String>> getRows()
	{
		return this.alRows;
	}
	
	public OutputColumnAttribute newColumn()
	{
		OutputColumnAttribute column = new OutputColumnAttribute();
		alColumns.add(column);
		
		return column;
	}
	
	public void setColumnWidth(String columnName, int width)
	{
		for(int i=0; i<alColumns.size();i++)
		{
			OutputColumnAttribute column = (OutputColumnAttribute)alColumns.get(i);
			if(column!=null && column.name.equals(columnName))
			{
				if(column.width<width)
					column.width=width;
				break;
			}
		}
	}

	public void setColumnAlign(String columnName, int align)
	{
		for(int i=0; i<alColumns.size();i++)
		{
			OutputColumnAttribute column = (OutputColumnAttribute)alColumns.get(i);
			if(column!=null && column.name.equals(columnName))
			{
				column.align=align;
				break;
			}
		}
	}
	
	public String getHeaderSeparator()
	{
		return this.headerSeparator;
	}

	public void setHeaderSeparator(String hdSeparator)
	{
		this.headerSeparator = hdSeparator;
	}

	public String getColumnSeparator()
	{
		return this.columnSeparator;
	}

	public void setColumnSeparator(String clSeparator)
	{
		this.columnSeparator = clSeparator;
	}
	
	public String getTitle()
	{
		return this.title;
	}
	
	public void setTitle(String t)
	{
		this.title=t;
	}
	
	public String getItemName()
	{
		return itemName;
	}

	public void setItemName(String name)
	{
		itemName = name;
	}
	public ArrayList<OutputColumnAttribute> getColumns()
	{
		return alColumns;
	}
	
	
	
}
