/*******************************************************************************
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
 *******************************************************************************/

/* -*-java-*-
Filename    : Messages.java
 */
 
package org.trafodion.jdbc.t2;

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
	
    // ------------------------------------------------------------------------------------------------
    static TrafT2Exception createSQLException(T2Properties t2props, Locale msgLocale, String messageId, Object mA1,
            Object mA2) {

        Object[] mAs = new Object[2];

        mAs[0] = mA1;
        mAs[1] = mA2;

        return createSQLException(t2props, msgLocale, messageId, mAs);

    } // end createSQLException

    // ------------------------------------------------------------------------------------------------
    static TrafT2Exception createSQLException(T2Properties t2props, Locale msgLocale, String messageId,
            Object messageArgument) {
        Object[] mAs = new Object[1];

        mAs[0] = messageArgument;

        return createSQLException(t2props, msgLocale, messageId, mAs);

    } // end createSQLException

    // ------------------------------------------------------------------------------------------------
    static TrafT2Exception createSQLException(T2Properties t2props, Locale msgLocale, String messageId,
            Object[] messageArguments) {
        Locale currentLocale;
        int sqlcode;

        if (msgLocale == null) {
            currentLocale = Locale.getDefault();
        } else {
            currentLocale = msgLocale;
        }

        try {
            PropertyResourceBundle messageBundle = (PropertyResourceBundle) ResourceBundle.getBundle("Messages",
                    currentLocale);

            MessageFormat formatter = new MessageFormat("");
            formatter.setLocale(currentLocale);
            formatter.applyPattern(messageBundle.getString(messageId + "_msg"));

            String message = formatter.format(messageArguments);
            String sqlState = messageBundle.getString(messageId + "_sqlstate");
            String sqlcodeStr = messageBundle.getString(messageId + "_sqlcode");

            if (sqlcodeStr != null) {
                try {
                    sqlcode = Integer.parseInt(sqlcodeStr);
                    sqlcode = -sqlcode;
                } catch (NumberFormatException e1) {
                    sqlcode = -1;
                }
            } else {
                sqlcode = -1;

            }
            return new TrafT2Exception(message, sqlState, sqlcode, messageId);
        } catch (MissingResourceException e) {
            // If the resource bundle is not found, concatenate the messageId
            // and the parameters
            String message;
            int i = 0;

            message = "The message id: " + messageId;
            if (messageArguments != null) {
                message = message.concat(" With parameters (Line 169): ");
                while (true) {
                    message = message.concat(messageArguments[i++].toString());
                    if (i >= messageArguments.length) {
                        break;
                    } else {
                        message = message.concat(",");
                    }
                }
            } // end if

            return new TrafT2Exception(message, "HY000", -1, messageId);
        } // end catch
    } // end createSQLException

    static void setSQLWarning(T2Properties t2props, SQLMXHandle handle, SQLWarningOrError[] we1) {
        //Logger log = getMessageLogger(t2props);

        int curErrorNo;
        SQLWarning sqlWarningLeaf;

        if (we1.length == 0) {
            handle.setSqlWarning(null);
            return;
        }

        for (curErrorNo = 0; curErrorNo < we1.length; curErrorNo++) {
            /* if (log != null && log.isLoggable(Level.WARNING)) {
               Object p[] = new Object[] { t2props, "Text: " + we1[curErrorNo].text,
               "SQLState: " + we1[curErrorNo].sqlState, "SQLCode: " + we1[curErrorNo].sqlCode };
               log.logp(Level.WARNING, "Messages", "setSQLWarning", "", p);
               } */

            sqlWarningLeaf = new SQLWarning(we1[curErrorNo].text, we1[curErrorNo].sqlState, we1[curErrorNo].sqlCode);
            handle.setSqlWarning(sqlWarningLeaf);
        } // end for
        return;
    }

    static void setSQLWarning(T2Properties t2props, SQLMXHandle handle, ERROR_DESC_LIST_def sqlWarning) {
        //Logger log = getMessageLogger(t2props);

        int curErrorNo;
        ERROR_DESC_def error_desc_def[];
        SQLWarning sqlWarningLeaf;

        if (sqlWarning.length == 0) {
            handle.setSqlWarning(null);
            return;
        }

        error_desc_def = sqlWarning.buffer;
        for (curErrorNo = 0; curErrorNo < sqlWarning.length; curErrorNo++) {
            /* if (log != null && log.isLoggable(Level.WARNING)) {
               Object p[] = new Object[] { t2props, "Text: " + error_desc_def[curErrorNo].errorText,
               "SQLState: " + error_desc_def[curErrorNo].sqlstate,
               "SQLCode: " + error_desc_def[curErrorNo].sqlcode };
               log.logp(Level.WARNING, "Messages", "setSQLWarning", "", p);
               } */

            sqlWarningLeaf = new SQLWarning(error_desc_def[curErrorNo].errorText, error_desc_def[curErrorNo].sqlstate,
                    error_desc_def[curErrorNo].sqlcode);
            handle.setSqlWarning(sqlWarningLeaf);
        }
        return;
    } // end setSQLWarning

    static void throwSQLException(T2Properties t2props, ERROR_DESC_LIST_def SQLError) throws TrafT2Exception {
        //Logger log = getMessageLogger(t2props);
        Locale locale = /* (t2props != null) ? t2props.getLocale() : */ Locale.getDefault();

        TrafT2Exception sqlException = null;
        TrafT2Exception sqlExceptionHead = null;
        int curErrorNo;

        if (SQLError.length == 0) {
            throw createSQLException(t2props, locale, "No messages in the Error description", null);
        }

        for (curErrorNo = 0; curErrorNo < SQLError.length; curErrorNo++) {
            /* if (log != null && log.isLoggable(Level.SEVERE)) {
               Object p[] = new Object[] { t2props, "Text: " + SQLError.buffer[curErrorNo].errorText,
               "SQLState: " + SQLError.buffer[curErrorNo].sqlstate,
               "SQLCode: " + SQLError.buffer[curErrorNo].sqlcode };
               log.logp(Level.SEVERE, "Messages", "throwSQLException", "", p);
               } */

            if (SQLError.buffer[curErrorNo].errorCodeType == TRANSPORT.ESTIMATEDCOSTRGERRWARN) {
                //
                // NCS said it was an SQL error, but it really wasn't it was a
                // NCS resource governing error
                //
                sqlException = Messages.createSQLException(t2props, locale, "resource_governing", null);
            } else {
                sqlException = new TrafT2Exception(SQLError.buffer[curErrorNo].errorText,
                        SQLError.buffer[curErrorNo].sqlstate, SQLError.buffer[curErrorNo].sqlcode, null);
            }
            if (curErrorNo == 0) {
                sqlExceptionHead = sqlException;
            } else {
                sqlExceptionHead.setNextException(sqlException);
            }
        }

        throw sqlExceptionHead;
    }

    static void throwSQLException(T2Properties t2props, SQLWarningOrError[] we1) throws TrafT2Exception {
        Locale locale = Locale.getDefault();

        TrafT2Exception sqlException = null;
        TrafT2Exception sqlExceptionHead = null;
        int curErrorNo;

        if (we1.length == 0) {
            throw createSQLException(t2props, locale, "No messages in the Error description", null);
        }

        for (curErrorNo = 0; curErrorNo < we1.length; curErrorNo++) {
            sqlException = new TrafT2Exception(we1[curErrorNo].text, we1[curErrorNo].sqlState, we1[curErrorNo].sqlCode,
                    null);
            if (curErrorNo == 0) {
                sqlExceptionHead = sqlException;
            } else {
                sqlExceptionHead.setNextException(sqlException);
            }
        } // end for

        throw sqlExceptionHead;
    } // end throwSQLException


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
