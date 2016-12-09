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

public class Display {

	private Session sessObj;
	private IDisplay rawDisplay;
	private IDisplay xmlDisplay;
	private IDisplay htmlDisplay;
	private IDisplay csvDisplay;
	
	public Display(Session sessObj)
	{
		this.sessObj = sessObj;
		rawDisplay = new RawDisplay(sessObj);
		xmlDisplay = new XMLDisplay(sessObj);
		htmlDisplay = new HTMLDisplay(sessObj);
		csvDisplay = new CSVDisplay(sessObj);
	}
	
	public void output(OutputContents contents) throws IOException
	{
		IDisplay display=rawDisplay;
		switch (sessObj.getDisplayFormat())
	      {
	         //HTML Format
	         case SessionDefaults.HTML_FORMAT:
	        	 display=htmlDisplay;
	        	 break;
	         //XML Format
	         case SessionDefaults.XML_FORMAT:
	        	 display=xmlDisplay;
	        	 break;
	         //CSV Format
	         case SessionDefaults.CSV_FORMAT:
	        	 display=csvDisplay;
	        	 break;
	         //Default Raw Format
	         default:
	        	 display=rawDisplay;
	        	 break;
	      }
		display.output(contents);
	}
	
}
