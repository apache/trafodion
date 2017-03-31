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

/* -*-java-*-
 * Filename    : Messages.java
 */
 
package org.apache.trafodion.jdbc.t2;

import java.sql.*;
import java.util.MissingResourceException;
import java.util.Locale;
import java.util.ResourceBundle;
import java.util.PropertyResourceBundle;
import java.text.MessageFormat;

 
class Messages
{
	static SQLException createSQLException(Locale msgLocale, String messageId, Object[] messageArguments)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_createSQLException].methodEntry();
		try
		{
			Locale currentLocale;
			int sqlcode;

			if (msgLocale == null)
				currentLocale = Locale.getDefault();
			else
				currentLocale = msgLocale;
			try 
			{
				PropertyResourceBundle messageBundle =
					(PropertyResourceBundle) ResourceBundle.getBundle("SQLMXT2Messages", currentLocale);//R321 changed property file name to SQLMXT2Messages_en.properties
				MessageFormat formatter = new MessageFormat("");
				formatter.setLocale(currentLocale);
				formatter.applyPattern(messageBundle.getString(messageId+"_msg"));
				String message = formatter.format(messageArguments);
				String sqlState = messageBundle.getString(messageId+"_sqlstate");
				String sqlcodeStr = messageBundle.getString(messageId+"_sqlcode");
				if (sqlcodeStr != null)
				{
					try
					{
						sqlcode = Integer.parseInt(sqlcodeStr);
						sqlcode = -sqlcode;
					}
					catch (NumberFormatException e1)
					{
						sqlcode = -1;
					}
				}
				else
					sqlcode = -1;
				return new SQLException(message, sqlState, sqlcode);
			}
			catch (MissingResourceException e) 
			{
				// If the resource bundle is not found, concatenate the messageId and the parameters 
				String message;
				int i = 0;
			
				message = "The message id: " + messageId;
				if (messageArguments != null)
				{
					message = message.concat(" With parameters: ");
					while (true)
					{
						message = message.concat(messageArguments[i++].toString());
						if (i >= messageArguments.length)
							break;
						else
							message = message.concat(",");
					}
				}
				return new SQLException(message, "HY000", -1);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_createSQLException].methodExit();
		}
	}
	
	static SQLWarning createSQLWarning(Locale msgLocale, String messageId, Object[] messageArguments)
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_createSQLWarning].methodEntry();
		try
		{
			Locale currentLocale;
			int sqlcode;
		
			if (msgLocale == null)
				currentLocale = Locale.getDefault();
			else
				currentLocale = msgLocale;
			try 
			{
				PropertyResourceBundle messageBundle = 
					(PropertyResourceBundle) ResourceBundle.getBundle("SQLMXT2Messages", currentLocale);//R321 changed property file name to SQLMXT2Messages_en.properties
				MessageFormat formatter = new MessageFormat("");
				formatter.setLocale(currentLocale);
				formatter.applyPattern(messageBundle.getString(messageId+"_msg"));
				String message = formatter.format(messageArguments);
				String sqlState = messageBundle.getString(messageId+"_sqlstate");
				String sqlcodeStr = messageBundle.getString(messageId+"_sqlcode");
				if (sqlcodeStr != null)
				{
					try
					{
						sqlcode = Integer.parseInt(sqlcodeStr);
					}
					catch (NumberFormatException e1)
					{
						sqlcode = 1;
					}
				}
				else
					sqlcode = 1;
				return new SQLWarning(message, sqlState, sqlcode);
			}
			catch (MissingResourceException e) 
			{
				// If the resource bundle is not found, concatenate the messageId and the parameters 
				String message;
				int i = 0;
			
				message = "The message id: " + messageId;
				if (messageArguments != null)
				{
					message = message.concat(" With parameters: ");
					while (true)
					{
						message = message.concat(messageArguments[i++].toString());
						if (i >= messageArguments.length)
							break;
						else
							message = message.concat(",");
					}
				}
				return new SQLWarning(message, "01000", 1);
			}
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_createSQLWarning].methodExit();
		}
	}

	static void throwUnsupportedFeatureException(Locale locale, String s) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_throwUnsupportedFeatureException].methodEntry();
		try
		{
			Object[] messageArguments = new Object[1];
			messageArguments[0] = s;
			throw Messages.createSQLException(locale, "unsupported_feature", messageArguments);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_throwUnsupportedFeatureException].methodExit();
		}
	}
	static void throwDeprecatedMethodException(Locale locale, String s) throws SQLException
	{
		if (JdbcDebugCfg.entryActive) debug[methodId_throwDeprecatedMethodException].methodEntry();
		try
		{
			Object[] messageArguments = new Object[1];
			messageArguments[0] = s;
			throw Messages.createSQLException(locale, "deprecated_method", messageArguments);
		}
		finally
		{
			if (JdbcDebugCfg.entryActive) debug[methodId_throwDeprecatedMethodException].methodExit();
		}
	} 

	private static int methodId_createSQLException					= 0;
	private static int methodId_createSQLWarning					= 1;
	private static int methodId_throwUnsupportedFeatureException	= 2;
	private static int methodId_throwDeprecatedMethodException		= 3;
	private static int totalMethodIds								= 4;
	private static JdbcDebug[] debug;
	
	static
	{
		String className = "Messages";
		if (JdbcDebugCfg.entryActive)
		{
			debug = new JdbcDebug[totalMethodIds];
			debug[methodId_createSQLException] = new JdbcDebug(className,"createSQLException"); 
			debug[methodId_createSQLWarning] = new JdbcDebug(className,"createSQLWarning"); 
			debug[methodId_throwUnsupportedFeatureException] = new JdbcDebug(className,"throwUnsupportedFeatureException"); 
			debug[methodId_throwDeprecatedMethodException] = new JdbcDebug(className,"throwDeprecatedMethodException"); 
		}
	}
}
