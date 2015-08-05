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
import java.util.HashMap;
import java.util.regex.Pattern;

public class ConditionalQuery extends QueryWrapper implements SessionDefaults{
	
	private HashMap<String, String> cqKeyMap = null;
   	private String gotoLabel = "";				// the label we are waiting to see
   	private int variableType = -1;
	private int valueType = -1;
	private String strPendingAction = null;
   	
   	/* types of values passed to conditional statements */
   	private static final int VARIABLE = 0;
   	private static final int STRING = 1;
   	private static final int INTEGER = 2;
   	/* enums not available in 1.4 :( */
   	
   	
	/* Constructor */
	public ConditionalQuery(){}
	
	public ConditionalQuery(Session sessObj){
	    super(sessObj);
	    cqKeyMap = new HashMap<String, String>();
	    loadCqKeyWordMap();
	}
	
	public void execute() throws ConditionalQueryException, UserInterruption, IOException
	{
		// reset error
		cqe.setError(null);
		
		String label = null;
		init();
		isMultiLine = sessObj.getQuery().isMultiLine();
		blankLiner = false;
		readQuery();
		blankLiner = true;
		
        // get the QueryId and directly process that command
        switch (sessObj.getQuery().getQueryId())
        {
        	case IF_THEN:
				if(!getGotoLabel().equals(""))
					break;
				
				// reset types
				valueType = -1;
				variableType = -1;
				
				String variable = parser.getConditionalVariable(this.queryStr);
				String operator = parser.getConditionalOperator(this.queryStr);
				String value = parser.getConditionalValue(this.queryStr);
				String action = parser.getConditionalAction(this.queryStr);
				
				if(variable == null || operator == null || value == null || action == null){
					throw cqe;
				}
				
				/* strip begin/end quotes and beginning % from variable */
				variable = format(variable, true);
				
				if(variableType != STRING)
				{
					Object tmpObj = getValue(variable);
					
					if(tmpObj == null)
					{
						if(isInteger(variable))
						{
							variableType = INTEGER;
						}
						else
						{
							/* not a valid integer, must have been an unresolved variable */
						    ErrorObject varNotFound = new ErrorObject(SessionError.VARIABLE_NOT_FOUND.errorCode(), 
						                                              SessionError.VARIABLE_NOT_FOUND.errorMessage() + "'" + variable + "'.");
							cqe.setError(varNotFound);
							throw cqe;
						}
					}
					else
					{
						variable = tmpObj.toString();
						variableType = VARIABLE;
					}
					
				}
				
				/* strip begin/end quotes and beginning % from value */
				value = format(value, false);
				
				if(valueType != STRING)
				{
					Object tmpObj = getValue(value);
					
					if(tmpObj == null)
					{
						if(isInteger(value))
							valueType = INTEGER;
						else
						{
							/* not a valid integer, must have been an unresolved variable */
						    ErrorObject varNotFound = new ErrorObject(SessionError.VARIABLE_NOT_FOUND.errorCode(), 
						                                              SessionError.VARIABLE_NOT_FOUND.errorMessage() + "'" + value + "'.");
							cqe.setError(varNotFound);
							throw cqe;
						}
					}
					else
					{
						value = tmpObj.toString();
						valueType = VARIABLE;
					}
				}
				
				if(compare(variable, operator, value))
				{
					/* condition passed, set pending action */
					strPendingAction = action + sessObj.getSessionSQLTerminator();
				}
				else
				{
					/* condition failed, do not set a pending action */
					strPendingAction = null;
				}
				break;
				
			case GOTO:
				if(!getGotoLabel().equals(""))
					break;
				
				parser.getNextKeyToken(); // skip first token
				label = parser.getNextValueToken();
                
				if(parser.hasMoreTokens())
                    writeSyntaxError(this.queryStr,parser.getRemainderStr());
                else
                {
    				if(label != null){
    					setGotoLabel(label);
                        writer.writeln();
                        
                        ErrorObject goto_msg = new ErrorObject(SessionError.GOTO_MESSAGE.errorCode(), 
												SessionError.GOTO_MESSAGE.errorMessage() + label + "'command is encountered.",SessionError.GOTO_MESSAGE.errorType);
                        writer.writeError(sessObj, goto_msg);
                        
    				}else
    				    writeSyntaxError(this.queryStr,parser.getRemainderStr());
                }
                
				break;
				
			case LABEL:
				parser.getNextKeyToken(); // skip first token
				label = parser.getNextValueToken();
				
				if(parser.hasMoreTokens())
				    writeSyntaxError(this.queryStr,parser.getRemainderStr());
				else
				{
    				if(label != null)
    				{
    				    if(label.equals(getGotoLabel())){
    				        writer.writeln();
                            writer.writeError(sessObj, SessionError.MATCHING_LABEL);
    				        setGotoLabel("");
    				    }
    				}else if(getGotoLabel().equals(""))
    				    writeSyntaxError(this.queryStr,parser.getRemainderStr());
    				else
    				{
                        /* give a warning */
                        ErrorObject labelWarning = new ErrorObject(SessionError.LABEL_WARNING.errorCode(), 
                                                                  SessionError.LABEL_WARNING.errorMessage(),'W');
                        cqe.setError(labelWarning);
                        throw cqe;
    				}
				}
				
				break;
		}
	}

	// change throws UserInterruption to custom error
	public Object getValue(String variable) throws ConditionalQueryException
	{
		/* get the variable's value */
		Object variableValue = null;
		
		if(variable.startsWith("?"))
		{
			variableValue = sessObj.getSessParams(variable.substring(1));
			if(variableValue == null)
			{
			    ErrorObject paramNotFound = new ErrorObject(SessionError.PARAM_NOT_FOUND.errorCode(), 
			                                                SessionError.PARAM_NOT_FOUND.errorMessage() + variable + " was not found");
				cqe.setError(paramNotFound);
				throw cqe;
			}
		}
		else
		{
			variableValue = sessObj.getEnv(variable);
		}
		
		return variableValue;
	}
	public boolean compare(String variable, String operator, String value) throws ConditionalQueryException
	{
		int compareType = -1;
		
		if(valueType != variableType)
		{
			if(valueType == VARIABLE)
				compareType = variableType;
			else if(variableType == VARIABLE)
				compareType = valueType;
			else if(variableType == INTEGER && valueType == STRING ||
						variableType == STRING && valueType == INTEGER)
			{
				if(variableType == STRING)
					cqe.setError(SessionError.STR_INT_COMPARISON);
				else
					cqe.setError(SessionError.INT_STR_COMPARISON);
				
				throw cqe;
			}
		}
		else{
			/* if both are variables, see if both are integers, 
			 * otherwise treat them as strings
			 */
			if(valueType == VARIABLE)
			{
				if(isInteger(variable) && isInteger(value))
					valueType = INTEGER;
				else
					valueType = STRING;
			}
			compareType = valueType;
		}
		
		switch(compareType){
			case STRING:
					if(operator.equals("=") || operator.equals("=="))
					{
						return variable.equals(value);	
					}
					else if(operator.equals("<>") || operator.equals("!=") || 
					        operator.equals("~=") || operator.equals("^="))
					{
						return !variable.equals(value);
					}
					else
					{
						
					    if(operator.equals("<") || operator.equals(">") || operator.equals("<=") ||  operator.equals(">="))
					    {
				             ErrorObject invalidStrComp = new ErrorObject(SessionError.INVALID_STRING_COMPARE.errorCode(), 
                                                                         SessionError.INVALID_STRING_COMPARE.errorMessage() + "'" + operator + "'.");
      
					        cqe.setError(invalidStrComp);
					    }
					    
					    // invalid operator exception
						throw cqe;
					}
					/* break not necessary, this is unreachable */
			case INTEGER:
					try
					{
						if(!(isInteger(variable) && isInteger(value)))
						{
							if(isInteger(value))
								cqe.setError(SessionError.STR_INT_COMPARISON);
							else
								cqe.setError(SessionError.INT_STR_COMPARISON);
							
							throw cqe;
						}
						else
						{
							
							int variableInt = Integer.parseInt(variable);
							int valueInt = Integer.parseInt(value);
							
							if(operator.equals("=") || operator.equals("=="))
							{
								return variableInt == valueInt;	
							}
							else if(operator.equals("<>") || operator.equals("!=") || 
							        operator.equals("~=") || operator.equals("^="))
							{
								return variableInt != valueInt;
							}
							else if(operator.equals("<"))
							{
								return variableInt < valueInt;
							}
							else if(operator.equals(">"))
							{
								return variableInt > valueInt;
							}
							else if(operator.equals("<="))
							{
								return variableInt <= valueInt;
							}
							else if(operator.equals(">="))
							{
								return variableInt >= valueInt;
							}
							else
							{
								// invalid operator exception
								throw cqe;
							}
						}
					}
					catch(NumberFormatException nfe)
					{
						// TODO: FIX ERROR
						nfe.printStackTrace();
					}
					/* break not necessary, this is unreachable */
		}
		
		return false;
	}
	
	/* strip out beginning % and quotes */
	private String format(String var, boolean isVariable)
	{
		String temp = var;
		
		if(var.startsWith("%"))
			temp = var.substring(1, var.length());
		
		
		if(temp.startsWith("\"") && temp.endsWith("\""))
		{
			if(temp.length() > 2)
				temp = temp.substring(1, temp.length()-1);
			else
				temp = "";
			
			if(isVariable)
				variableType = STRING;
			else
				valueType = STRING;
		}
		
		return temp;
	}
	
	private void loadCqKeyWordMap()
	{
		cqKeyMap.put("IF", ""+IF_THEN);
		cqKeyMap.put("LABEL", ""+LABEL);
		cqKeyMap.put("GOTO", ""+GOTO);
	}
	
	public boolean isInteger(String str)
	{
		return Pattern.matches("^-?\\d+$",str); 
	}
	
	
	/* 
     * repeated code from interface query, perhaps move out of IQ and make public somewhere else?
     */
    private void writeSyntaxError(String queryStr, String remainderStr) throws IOException
    {
        sessObj.getQuery().setStatusCode(-1); // update the error code to -1
        
        if(sessObj.getDisplayFormat() != SessionDefaults.XML_FORMAT)
            writer.writeln();
        
        writer.writeSyntaxError(sessObj, this.queryStr,remainderStr);
    }
    
    /* Getter/Setter Methods ****/
	
	private void setGotoLabel(String label){
		this.gotoLabel = label;
	}
	
	public String getGotoLabel(){
		return this.gotoLabel;
	}

	public String getPendingAction(){
		String tmpAction = strPendingAction;
		strPendingAction = null;
		return tmpAction;
	}
	
	public boolean isPendingAction()
	{
		return strPendingAction != null;
	}
	
	public void setQueryString(String q){
		this.queryStr = q;
	}
	
	public String getQueryString(){
		return this.queryStr;
	}
	
}
