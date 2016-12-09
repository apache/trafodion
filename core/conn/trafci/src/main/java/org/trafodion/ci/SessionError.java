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

public interface SessionError
{

         //The third arugment represents the ERROR TYPE. I = Informational, W = Warning
       ErrorObject CONSOLE_READER_ERR = new ErrorObject("29400", "Could not initialize the console reader.");
       ErrorObject CONSOLE_WRITER_ERR =  new ErrorObject("29401", "Could not initialize the console writer.");
       ErrorObject CMD_LINE_ARGS_ERR = new ErrorObject("29402", "Invalid Number of arguments passed.");
       ErrorObject SCRIPT_FILE_NOT_FOUND = new ErrorObject("29403", "Could not find the script file specified.");
       ErrorObject DRIVER_INIT_ERR = new ErrorObject("29404", "Could not instantiate the driver class.");
       ErrorObject DRIVER_INIT_ILLEGAL_ERR = new ErrorObject("29405", "Could not instantiate the driver class because of an IllegalAccessException.");
       ErrorObject DRIVER_CLASS_ERR = new ErrorObject("29406", "Could not find JDBC Type 4 Driver in the classpath.");

       ErrorObject CONN_MAX_LIMIT_ERR = new ErrorObject("29407", "Failed to connect to the database. Connection limit exceeded");

       ErrorObject SET_NO_OPTION = new ErrorObject("29408", "SET command must have an option specified.");
       ErrorObject SHOW_NO_OPTION = new ErrorObject("29409", "SHOW command must have an option specified.");
       ErrorObject OBEY_FILE_NOT_FOUND = new ErrorObject("29410", "Could not find the file specified.");
       ErrorObject SPOOL_FILE_NOT_FOUND = new ErrorObject("29411", "Could not create the spool file.");
       ErrorObject CMD_NOT_CS_SUPPORTED = new ErrorObject("29412", "This command is not supported in CS mode.");
       ErrorObject CMD_NOT_SQL_SUPPORTED = new ErrorObject("29414", "This command is not supported in SQL mode.");
       ErrorObject CMD_NOT_MODE_SUPPORTED = new ErrorObject("29415", "This command is not supported in current mode.");   
       
       //Warnings
       ErrorObject SHOW_TAB_NOT_FOUND = new ErrorObject("29416", "No tables found.", 'I');
       ErrorObject SHOW_CAT_NOT_FOUND = new ErrorObject("29417", "No catalogs found.", 'I');
       ErrorObject SHOW_SCH_NOT_FOUND = new ErrorObject("29418", "No schemas found.", 'I');
       ErrorObject SHOW_VIEW_NOT_FOUND = new ErrorObject("29419", "No views found.", 'I');
       ErrorObject SHOW_SYNONYM_NOT_FOUND = new ErrorObject("29420", "No synonyms found.", 'I');
       ErrorObject SHOW_MV_NOT_FOUND = new ErrorObject("29421", "No materialized views found.", 'I');
       ErrorObject SHOW_MVG_NOT_FOUND = new ErrorObject("29422", "No materialized view groups found.", 'I');
       ErrorObject SHOW_PROC_NOT_FOUND = new ErrorObject("29423", "No procedures found.", 'I');
       ErrorObject SHOW_TAB_IDX_NOT_FOUND = new ErrorObject("29424", "No indexes present for object, ", 'I');
       ErrorObject SHOW_TAB_MVS_NOT_FOUND = new ErrorObject("29425", "No materialized views present for object, ", 'I');
       ErrorObject SHOW_TAB_SYN_NOT_FOUND = new ErrorObject("29426", "No synonyms present for object, ", 'I');
       ErrorObject SHOW_PREP_NOT_FOUND = new ErrorObject("29427", "No prepared statements found.", 'I');
       ErrorObject SHOW_PARAM_NOT_FOUND = new ErrorObject("29428", "No parameters found.", 'I');
       
       //More Warnings
       ErrorObject SPOOL_ON_ON = new ErrorObject("29429", "Spooling is already ON.", 'I');
       ErrorObject SPOOL_OFF_OFF = new ErrorObject("29430", "Spooling is already OFF.", 'I');
       ErrorObject STMT_NOT_FOUND = new ErrorObject("29431", "Prepared statement %s was not found.");
       ErrorObject INVALID_PARAM_NUM = new ErrorObject("29432", "Invalid number of parameters for prepared statement %s.");
       ErrorObject NUMERIC_VAL_REQ = new ErrorObject("29433", "Non-numeric value cannot be specified for numeric data type.");
       ErrorObject SET_IDLETIMEOUT_VAL_ERR = new ErrorObject("29434", "Invalid IdleTimeout value. The value must be in the range 0-2147483647.");
       ErrorObject SET_LISTCOUNT_VAL_ERR = new ErrorObject("29435", "Invalid ListCount value. The value must be in the range 0-2147483647.");
       ErrorObject REPEAT_ERR = new ErrorObject("29436", "The specified statement does not exist in the history buffer.");
       ErrorObject HISTORY_ERR = new ErrorObject("29437", "The maximum size of the history buffer is "+SessionDefaults.QRY_HISTORY_MAX_LMT+".");
       ErrorObject HISTORY_BUF_ERR = new ErrorObject("29438", "The history buffer is empty.");
       ErrorObject SLASH_ERR = new ErrorObject("29439", "The query buffer is empty.");
       ErrorObject DIR_NOT_FOUND = new ErrorObject("29440", "Directory not found or"+SessionDefaults.lineSeperator+"does not have READ/WRITE permissions.");   
       ErrorObject OBEY_SECTION_NOT_FOUND = new ErrorObject("29441", "was not found in file ");
       ErrorObject SAVE_FILE_CREATE_ERROR = new ErrorObject("29442", "Could not create the specified file.");
       ErrorObject DB_CONN_NOT_EXIST = new ErrorObject("29443", "Database connection does not exist. Please connect to the database by using the connect or the reconnect command.");
       ErrorObject DB_DISCONNECT_ON_USER_REQ = new ErrorObject("29444", "The current operation has been cancelled and the connection to the database has been lost."+SessionDefaults.lineSeperator+"Please connect to the database by using the connect or the reconnect command.");
       ErrorObject OPERATION_CANCELLED = new ErrorObject("29445", "The current operation has been cancelled.");
       ErrorObject PRUN_CONN_CNT_ERR = new ErrorObject("29446", "Number of connections entered is not within the allowable range.");
       ErrorObject INCORRECT_OVERWRITE_OPTION = new ErrorObject("29447", "Incorrect value specified for overwrite option.");
       ErrorObject EXECUTE_PRIVILEGE_ERR = new ErrorObject("29448", "The user does not have EXECUTE privilege.");
       ErrorObject SERVER_PRODUCT_ERR = new ErrorObject("29449", "Server product may not be installed or an internal error has occurred. Please contact support.");
       ErrorObject CALL_ERR = new ErrorObject("29450", "This command is not supported by this JDBC Type 4 Driver version.");
       ErrorObject INTERNAL_ERR = new ErrorObject("29451", "Internal error processing command.");
       
       ErrorObject INVALID_MAXCONN = new ErrorObject("29452", "Invalid value specified for property, trafci.prun.maxconn.");
       ErrorObject NO_FILES_OF_EXTENSION = new ErrorObject("29453", "No files present with this extension.");
       ErrorObject INVALID_CONN_VALUE = new ErrorObject("29454", "Invalid value specified for -c/connections option.");
       ErrorObject UNKOWN_OPTION = new ErrorObject("29455", "Unknown option specified: ");
       ErrorObject DEFAULT_OPTION_ERR = new ErrorObject("29456", "-d|defaults option cannot be specified with any other options.");
       ErrorObject NWMS_NOT_STARTED = new ErrorObject("29457", "NWMS not started on this server.", 'W');
       
       ErrorObject SCRIPTS_DIR_NOT_FOUND = new ErrorObject("29458", "Scripts Directory not found or"+SessionDefaults.lineSeperator+"does not have READ/WRITE permissions.");
       ErrorObject LOGS_DIR_NOT_FOUND = new ErrorObject("29459", "Logs Directory not found or"+SessionDefaults.lineSeperator+"does not have READ/WRITE permissions.");  
       
       
       String GENERIC_SYNTAX_ERROR_CODE = "29460";
       String SYNTAX_ERROR_PREFIX = "A syntax error occurred at or before:";
       String ERROR_CODE_PREFIX = "*** ERROR[";
       String ERROR_CODE_SUFFIX = "]";


       ErrorObject LIST_NO_OPTION = new ErrorObject("29461", "LIST command must have an option specified.");
       ErrorObject OBEY_PATH_NOT_FOUND = new ErrorObject("29462", "Could not find the file or directory specified.");
       ErrorObject OBEY_WILDCARD_NOT_FOUND  =new ErrorObject("29463", "Could not find any file(s) with the specified extension.");
       ErrorObject PARAM_NOT_FOUND = new ErrorObject("29464", "Param "); //Add Parameter name as suffix
       
       ErrorObject OBEY_BLANK_SESSION_ERR = new ErrorObject("29465","Obey commands inside of files are executed in non-interactive mode and require a filename or wildcard.");
       ErrorObject OBEY_DEPTH_MAX = new ErrorObject("29466","The max obey depth has been reached, breaking current obey loop. Recursion detected in ");
      String CS_SYNTAX_ERROR_CODE = "29467";
      
      ErrorObject SESSION_DISCONNECT_ERR = new ErrorObject("29468", "Session Disconnected. Please connect to the database by using connect/reconnect command.", 'I');
      ErrorObject MULTIBYTE_DISP_WARN = new ErrorObject("29469", "Unable to align display for multibyte characters." +
                                       " Try relaunching TrafCI with -Djava.awt.headless=true", 'W');

      /* Conditional Errors */
      ErrorObject VARIABLE_NOT_FOUND = new ErrorObject("29471", "Unable to find TrafCI variable ");
      // use 29464 for PARAM_NOT_FOUND
      ErrorObject INT_STR_COMPARISON = new ErrorObject("29472", "Unable to compare an integer with a string.");
      ErrorObject STR_INT_COMPARISON = new ErrorObject("29473", "Unable to compare a string with an integer.");
      ErrorObject INVALID_STRING_COMPARE = new ErrorObject("29474", "Unable to compare strings using the operator ");

      ErrorObject MAX_DELAY_LIMIT = new ErrorObject("29475", "Invalid delay time specified. The value must be in the range 0-");
      
      // warning for label:
      ErrorObject LABEL_WARNING = new ErrorObject("29476", "TrafCI encountered a blank label. Use the format LABEL <name>",'W');
      ErrorObject OUT_OF_BOUNDS = new ErrorObject("29477","ArrayIndexOutOfBounds exception has occurred.");
      ErrorObject MESSAGE_WARNING = new ErrorObject("29478", "Skipping command until a matching 'LABEL ",'W');
      ErrorObject GOTO_MESSAGE = new ErrorObject("29479", "GOTO statement executed, ignoring all commands until a 'LABEL ",'W');
      ErrorObject MATCHING_LABEL = new ErrorObject("29480", "Matching label encountered, no longer ignoring commands.",'I');
      
      /* Alias commands */
      ErrorObject SHOW_ALIAS_NOT_FOUND = new ErrorObject("29481", "No aliases found.", 'I');
      ErrorObject COMMAND_NAME_NOT_ALLOWED= new ErrorObject("29482", "Command name, ", 'I');
      String COMMAND_NAME_NOT_ALLOWED_SUFFIX= " not allowed as Alias.";
      ErrorObject INVALID_COMMAND_FOR_ALIAS = new ErrorObject("29483", "Alias on an alias not supported., ", 'I');
            
      ErrorObject SET_FETCHSIZE_VAL_ERR = new ErrorObject("29484", "Invalid fetch size value. The value must be in the range 0-2147483647.");
            
      /* Pattern commands */
      ErrorObject SET_PATTERN_DEPTH_VAL_ERR = new ErrorObject("29485", "Invalid pattern depth value. The value must be in the range 0-" + SessionDefaults.PATTERN_DEPTH_LIMIT+ ".");
      ErrorObject SHOW_PATTERN_NOT_FOUND = new ErrorObject("29486", "No patterns found.",'I');
      String PATTERN_COMMAND_NAME_NOT_ALLOWED_SUFFIX= " not allowed as Pattern.";
      ErrorObject INVALID_REGEXP = new ErrorObject("29487", "Invalid regular expression.",'I');
      
      /* Security commands */
      ErrorObject CMD_NOT_SUPPORTED_SERVER = new ErrorObject("29488", "This command is not supported on this server version.");
      ErrorObject FILE_NO_READ_PERMISSIONS = new ErrorObject("29489", "Unable to read the specified file.");
      ErrorObject CMD_NOT_SEC_SUPPORTED = new ErrorObject("29490", "This command is not supported in SEC mode.");
      ErrorObject CMD_NOT_WMS_SUPPORTED = new ErrorObject("29491", "This command is not supported in WMS mode.");
      ErrorObject INVALID_NUMBER_FORMAT = new ErrorObject("29492", "Numeric value out of range ");
      ErrorObject INFO_SEC_NO_ROWS = new ErrorObject("29493", "No server configuration found.",'I');
      ErrorObject MISSING_PARM = new ErrorObject("29494", "Required Parameter ",'E');
      ErrorObject INVALID_PWORD = new ErrorObject("29495", "Passwords do not match.",'E');
      ErrorObject DUPLICATE_PARAM = new ErrorObject("29496", "Duplicate Parameter ",'E');
      ErrorObject SET_DUMPPRIORITY_VAL_ERR = new ErrorObject("29497", "Invalid dump priority specified. The value must be in the range 1...199");
      /* Param commands */
      ErrorObject INVALID_PARAM_STRING_VALUE = new ErrorObject("29498", "Invalid parameter value. ");
      ErrorObject SET_BACKUPSYNC_DURATION_ERR = new ErrorObject("29499", "Invalid backup sync duration specified. The value must be in the range 1...31");
      //SPJ deployment error
      ErrorObject JAR_EXTENSION_MISSING = new ErrorObject("29500", "JAR extension is missing. ");
      ErrorObject JAR_EMPTY = new ErrorObject("29501", "JAR file is empty. Failed to upload jar file ", 'I');
      ErrorObject JAR_EMPTY_FOLDER = new ErrorObject("29502", "Specified directory contains no JAR file. SPJ Uploading failed.");
      
      //Server Side errors start from 29550 - 29599
      
      ErrorObject MISSING_NODE_DUMP = new ErrorObject("29550","Missing Node Dump Pattern.");
      ErrorObject MISSING_CORE_DUMP = new ErrorObject("29551","Missing Core File Pattern.");
      ErrorObject MISSING_CORE_NODE_DUMP = new ErrorObject("29552","Missing Node Dump and Core File Pattern.");

      ErrorObject NOT_SUPPORT = new ErrorObject("29553", "This platform does not support ", 'I');     
      ErrorObject SHOW_TAB_TRIG_NOT_FOUND = new ErrorObject("29554", "No triggers present for object, ", 'I');
     ErrorObject CMD_NOT_SEAQUEST_SUPPORTED = new ErrorObject("29555", "This command is not supported.");
     ErrorObject CMD_ONLY_CS_SUPPORTED = new ErrorObject("29556", "This command is only supported in CS mode.");
     ErrorObject CMD_ONLY_WMS_SUPPORTED = new ErrorObject("29557", "This command is only supported in WMS mode.");
     ErrorObject CMD_ONLY_SQL_SUPPORTED = new ErrorObject("29558", "This command is only supported in SQL mode.");
     ErrorObject CMD_SYNTAX_ERROR = new ErrorObject("29559", "A syntax error occurred in the command.");
     ErrorObject SQL_RESERVED_WORD_ERROR = new ErrorObject("29560", "SQL is a reserved word. Please specify another string.");
     ErrorObject WMS_RESERVED_WORD_ERROR = new ErrorObject("29561", "WMS is a reserved word. Please specify another string.");
     ErrorObject CS_RESERVED_WORD_ERROR = new ErrorObject("29562", "CS is a reserved word. Please specify another string.");
}
