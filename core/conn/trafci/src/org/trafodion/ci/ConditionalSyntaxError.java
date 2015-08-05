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

import java.util.regex.Pattern;
import java.util.regex.Matcher;

public class ConditionalSyntaxError  {

	
	private StringBuffer errMesg=null;
	private String errPrefix= "";

	Pattern patVariable = Pattern.compile("^((?i:IF)(\\s|\\n)+((\"(\"(?=\")|\\\\(?=\")|.)*?\")|[^(\\n|\\s|!=|<>|>=|<=|>|<|=|==)]+)?)(.+)?\\z", Pattern.DOTALL);
	Pattern patOperator = Pattern.compile("^((?i:IF)(\\s|\\n)+((\"(\"(?=\")|\\\\(?=\")|.)*?\")|[^(\\n|\\s|!=|<>|>=|<=|>|<|=|==)]+)(\\s|\\n)*(!=|<>|>=|<=|>|<|==|=)?)(.*)\\z", Pattern.DOTALL);
	Pattern patValue    = Pattern.compile("^((?i:IF)(\\s|\\n)+((\"(\"(?=\")|\\\\(?=\")|.)*?\")|[^(\\n|\\s|!=|<>|>=|<=|>|<|=|==)]+)(\\s|\\n)*(!=|<>|>=|<=|>|<|==|=)(\\s|\\n)*((\"(\"(?=\")|\\\\(?=\")|.)*?\")|[^(\\s|\\n|!=|<>|>=|<=|>|<|=|==)]+)?)(.+)\\z", Pattern.DOTALL);

	ConditionalSyntaxError()
	{
		
	}
	
	public String testVariable(String qryString){
		Matcher m = patVariable.matcher(qryString);
		if(m.find())
			return m.group(m.groupCount());
		
		return null;
	}
	
	public String testOperator(String qryString){
		Matcher m = patOperator.matcher(qryString);
		if(m.find())
			return m.group(m.groupCount());
		
		return null;	
	}
	
	public String testValue(String qryString){
		Matcher m = patValue.matcher(qryString);
		if(m.find())
			return m.group(m.groupCount()).trim();		
		
		return null;
	}
	
	public void setSyntaxError(String qryString, String errLoc)
	{
		if(errMesg == null)
		{
			errMesg=new StringBuffer();
		}
	
		this.errMesg.delete(0,errMesg.length());
		
		this.errMesg.append(errPrefix);
		this.errMesg.append("\n");
		this.errMesg.append(qryString);
		this.errMesg.append("\n");

		for(int i=0; i < qryString.length()-errLoc.length();i++)
			this.errMesg.append(" ");
		
		this.errMesg.append("^");		
	}
	
	public ErrorObject getSyntaxError(String qryString)
	{
		
		if(qryString != null)
		{
			qryString=qryString.replaceAll("\\n"," ").trim();
		}

		String testVar = testVariable(qryString);
		String testOp = testOperator(qryString);
		String testVal = testValue(qryString);

		String errLoc = "";
		
		if(testVal != null)
		{
			errPrefix = "A conditional syntax error occurred at or before:";
			errLoc = testVal;
		}
		else if(testOp != null)
		{
			errPrefix = "A conditional operator syntax error occurred at or before:";
			errLoc = testOp;
		}
		else
		{
			errPrefix = "A conditional variable syntax error occurred at or before:";
			if(testVar == null)
				testVar = qryString.charAt(qryString.length()-1)+"";

			errLoc = testVar;
		}
		
		setSyntaxError(qryString, errLoc);
		return new ErrorObject(SessionError.GENERIC_SYNTAX_ERROR_CODE, errMesg.toString());
	}
	

}
