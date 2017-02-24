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

import java.io.ByteArrayInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.File;
import java.io.FilenameFilter;
import java.io.InputStream;
import java.sql.DatabaseMetaData;
import java.sql.ResultSet;
import java.sql.SQLException;
import java.sql.Statement;
import java.text.DateFormat;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.Date;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Timer;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.regex.PatternSyntaxException;
import java.sql.PreparedStatement;

import org.trafodion.jdbc.t4.TrafT4Statement;

import java.lang.InterruptedException;

import java.util.Map;
import java.util.TreeMap;
import java.util.Vector;
import java.sql.Connection;
import sun.misc.Signal;
import sun.misc.SignalHandler;
import java.net.UnknownHostException;

public class InterfaceQuery extends QueryWrapper implements SessionDefaults {
	private HashMap<String, String> iqKeyMap = null;
	private HashMap<String, String> envMap = null;
	private FileWriter spoolWriter = null;
	private FileReader obeyReader = null;
	private String spoolFileName = null;
	private Connection newConn = null;
	private SignalHandler LHCTRLCHandler = null;
	private SignalHandler DelayCTRLCHandler = null;
	private Signal INTSignal = null;
	private int totalCount = 0;
	private boolean doTrace = Boolean.getBoolean(SessionDefaults.PROP_TRACE);
	private boolean printConnTime = false;
	private String strFilter = "";

	private Thread curThread = null;
	private int maxDelayVal = -1;

	/**
	 * Final String Initialization.
	 */
	private final String infoNotAvailable = "Information not available.";
	private final String ciServerErrMsg = "An internal server error has occurred. Please contact support.";

	InterfaceQuery() {
	}

	InterfaceQuery(Session sessObj) {
		super(sessObj);
		iqKeyMap = new HashMap<String,String>();
		envMap = sessObj.getEnvMap();
		loadIqKeyWordMap();
		loadEnvVarMap(); // loads the env variables in the pattern hashmap
		addEnvValuesMap(); // loads env values into the env hash map
		if (sessObj.getCaller() != SessionDefaults.PRUNI) {
			LHCTRLCHandler = new SignalHandler() {
				public void handle(Signal sig) {
					killProcess();
				}
			};

			/* Ctrl+C handler for the delay command */
			DelayCTRLCHandler = new SignalHandler() {
				public void handle(Signal sig) {
					if (curThread != null)
						curThread.interrupt();

				}
			};

			try {
				INTSignal = new Signal("INT");
				Signal.handle(INTSignal, LHCTRLCHandler);
			} catch (Exception e) {
			}
		}

		String strConnTime = System
				.getProperty(SessionDefaults.PROP_PRINT_TIME);
		if ((strConnTime != null) && strConnTime.equalsIgnoreCase("y")) {
			printConnTime = true;
		}

		String maxDelayStr = System.getProperty(SessionDefaults.PROP_MAX_DELAY);
		if (maxDelayStr != null)
			maxDelayVal = Integer.parseInt(maxDelayStr) * 1000;
	}

	void killProcess() {
		Query qryObj = sessObj.getQuery();
		if (qryObj != null && qryObj.getQueryId() == LOCALHOST) {
			if (sessObj.getProcObj() != null) {
				sessObj.getProcObj().destroy();
				sessObj.setProcObj(null);
			}
		}
	}

	public void execute() throws UnKnownInterfaceCommand, SQLException,
			IOException, UserInterruption {
		String cmdStr = null;
		int cmdNo = 0;
		init();
		isMultiLine = false; // flag to check if the input line can be entered
		// in multline.
		blankLiner = false;

		readQuery();

		// Move the pointer to the next key word as we already know the
		// first key word for all interface commands which would have been
		// set by the validateQuery
		//
		String fToken = parser.getNextKeyToken();

		// put the timer on hold
		// sessObj.setTimerHold();

		// get the QueryId and directly process that command
		sessObj.setQryStartTime();

		switch (sessObj.getQuery().getQueryId()) {

		case SECTION:
			sessObj.getQuery().resetQueryText("");
			break;

		case HELP:
			handleHelp();
			break;

		case CLEAR:
			handleClearCommand();
			break;

		case MODE:
			writer.writeln();
			writer.writeError(sessObj, SessionError.CMD_NOT_SEAQUEST_SUPPORTED);
			break;

		case SET:
			execSet();
			break;

		case SHOW:
			execShow();
			break;

		case GET:
			execGet();
			break;

		case CONNECT:
			handleConnect();
			break;

		case RECONNECT:
			handleReconnect();
			break;

		case EXIT:
		case DISCONNECT:
			handleDisConnect();
			break;

		case ED:
		case EDIT:
			break;

		case SPOOL:
		case LOG:
			execSpool();
			break;

		case SLASH:
		case RUN:
			handleRunCommand();
			break;

		case AT:
		case OBEY:
			execObey();
			break;

		case RESET:
			handleResetCommand();
			break;

		case HISTORY:
			handleHistory();
			break;

		case FC:
		case REPEAT:
			handleRepeatCommand(cmdStr, cmdNo);
			break;

		case SESSION:
		case ENV:
			execEnv();
			break;

		case SAVEHIST:
			handleSaveHistory();
			break;

		case VERSION:
			handleVersion();
			break;

		case ERROR:
			formatResults(SessionError.NOT_SUPPORT + SqlEnum.ERROR.toString() + " command.");
			break;

		case LOCALHOST:
			handleHostCommand();
			break;

		case DELAY:
			handleDelay();
			break;

		case ALIAS:
			handleAlias();
			break;

		case DOTSEC:
		case DOTCS:
		case DOTSQL:
		case DOTNS:
		case DOTWMS:

			if (!sessObj.isDBConnExists()) // Added  2010-03-01
			{
				displayNoConnInfo();
				break;
			}

			if (!parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			String secondToken = parser.lookAtNextToken().toUpperCase();

			if (secondToken.equalsIgnoreCase(".SEC")
					|| secondToken.equalsIgnoreCase(".CS")
					|| secondToken.equalsIgnoreCase(".NS")) {
				formatResults(SessionError.NOT_SUPPORT
						+ secondToken.substring(1) + " mode.");
			}

			if (secondToken.equalsIgnoreCase(".SQL")
					|| secondToken.equalsIgnoreCase(".WMS")) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			rewriteQryString(sessObj.getStrMode(), fToken.substring(1));
			sessObj.setDotMode(fToken.substring(1));
			ByteArrayInputStream bais = new ByteArrayInputStream(qryObj
					.getQueryText().getBytes());
			sessObj.setLogCmdEcho(false);
			sessObj.setDotModeCmd(true);
			sessObj.setHistOn(false);
			sessObj.setPrevMode(sessObj.getMode());
			this.execObeyStream(bais, "");
			break;

		} // end switch
		sessObj.setQryEndTime();
		// sessObj.setLastError(sessObj.getQuery().getStatusCode());
	} // end execute

	/* parameterized types not available in 1.4, must cast below */
	class FilenameComparator implements Comparator<File> {

		// sort alphabetically
		public int compare(File fa, File fb) {
			return fb.getName().compareToIgnoreCase(
					fa.getName());
		}
	}

	public void execObeyStream(InputStream theStream, String clzName)
			throws IOException {
		obeyReader = new FileReader();

		try {
			obeyReader.initializeStream(theStream, clzName);
			sessObj.getReader().setObeyReader(obeyReader);
			if (sessObj.getReader().getReadMode() != SessionDefaults.OBEY_READ_MODE) {
				sessObj.getReader().setReadMode(OBEY_READ_MODE);
				sessObj.setSessionType(false);
			}

		} catch (IOException e) {
			throw e;
		}

	}

	public void execObey() throws IOException {
		FilenameFilter filter;
		File file = null;
		Comparator<File> fileAlphabetize = new FilenameComparator();

		String obeyFileName = parser.getNextFileToken();
		obeyFileName = (obeyFileName == null) ? "" : obeyFileName.trim();

		if (obeyFileName.equals("")) {
			// if user just hit enter, prompt for a file
			if (sessObj.isSessionInteractive()) {
				try {

					String defaultVal = "*.sql";
					String prompt = "Enter the script filename [" + defaultVal
							+ "]      :";
					sessObj.getConsoleWriter()
							.print(utils.formatString(prompt,
									prompt.length() + 1, ' '));
					obeyFileName = sessObj.getReader().getLine();
					if ((obeyFileName == null || obeyFileName.trim().equals(""))) {
						obeyFileName = defaultVal;
					}

					obeyFileName = obeyFileName.trim();

				} catch (UserInterruption ui) {
					return;
				}
			} else {
				sessionError('E', new ErrorObject(
						SessionError.OBEY_BLANK_SESSION_ERR.errorCode(),
						SessionError.OBEY_BLANK_SESSION_ERR.errorMessage()));
				return;
			}
		}

		// obtain filter at the end of the filename
		file = new File(obeyFileName);

		// test for slash, add on at end
		boolean addSlash = obeyFileName.endsWith(File.separator) ? true : false;
		if (file.getParent() != null) {
			obeyFileName = file.getAbsolutePath(); // java strips last slash
			if (addSlash)
				obeyFileName += File.separator;
		} else if (file.exists() && file.isDirectory() && !addSlash) {
			obeyFileName += File.separator;
			file = new File(obeyFileName);
		}

		if (file.exists()) {
			if (file.isDirectory())
				strFilter = "*.sql";
			else
				file = new File(obeyFileName);
		} else if (!obeyFileName.endsWith(File.separator)
				&& (obeyFileName.indexOf("*") != -1 || obeyFileName
						.indexOf("?") != -1)) {
			strFilter = file.getName();
			obeyFileName = file.getParent();
			if (obeyFileName == null)
				obeyFileName = System.getProperty("user.dir");

			file = new File(obeyFileName);
		} else {
			sessionError(
					'E',
					new ErrorObject(SessionError.OBEY_PATH_NOT_FOUND
							.errorCode(), SessionError.OBEY_PATH_NOT_FOUND
							.errorMessage()));
			return;
		}

		// windows/dos-style wildcards, * really means .* and ? means . in regex
		// terms
		// replace asterisks with regex something like .* but remember to
		// replace existing dots with \.
		strFilter = strFilter.replaceAll("\\.", "\\\\.");
		strFilter = strFilter.replaceAll("\\*", "\\.\\*");
		strFilter = strFilter.replaceAll("\\?", ".");

		// escape all other regex symbols
		Pattern p = Pattern.compile("([$+|\\[\\](){}])");
		Matcher m = p.matcher(strFilter);
		strFilter = m.replaceAll("\\\\$1");

		filter = new FilenameFilter() {
			public boolean accept(File dir, String name) {
				File testFile = new File(dir.getAbsolutePath() + File.separator
						+ name);

				Pattern pattern;
				if (System.getProperty("os.name").toUpperCase()
						.startsWith("WINDOW"))
					pattern = Pattern.compile(strFilter,
							Pattern.CASE_INSENSITIVE);
				else
					pattern = Pattern.compile(strFilter);

				Matcher matcher = pattern.matcher(name);
				return matcher.matches() && testFile.isFile();
			}
		};

		File[] fl;
		if (file != null && file.exists() && file.isDirectory()) {
			fl = file.listFiles(filter);
			Arrays.sort(fl, fileAlphabetize);
		} else {
			fl = new File[1];
			fl[0] = file;
		}

		if (obeyFileName == null || obeyFileName.trim().equals("")) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}

		String sectionName = parser.getNextSectionToken();

		if (parser.hasMoreTokens()) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}

		if (fl.length == 0) {

			sessionError('E', SessionError.OBEY_WILDCARD_NOT_FOUND);
			return;
		}

		if (sessObj.isSessionInteractive()
				|| sessObj.getReader().getReadMode() == SessionDefaults.OBEY_READ_MODE) {

			File f;
			for (int i = 0; i < fl.length; i++) {
				f = fl[i];

				/* ignore the log file if its in the obey list */
				if (sessObj.isSessionLogging()) {
					if (f.getAbsolutePath().equals(sessObj.getSpoolFileName()))
						continue;
				}

				obeyReader = new FileReader();

				try {
					obeyReader.initialize(f.getAbsolutePath());

					if (sectionName != null) {
						try {
							obeyReader.setSectionRead(true, sectionName);
						} catch (ScriptSectionNotFound e) {
							sessionError(
									'E',
									new ErrorObject(
											SessionError.OBEY_SECTION_NOT_FOUND
													.errorCode(),
											"Section "
													+ sectionName
													+ SessionError.OBEY_SECTION_NOT_FOUND
															.errorMessage()
													+ f.getName()));
							obeyReader.close();
							obeyReader = null;
							return;
						}
					}
					sessObj.getReader().setObeyReader(obeyReader);
					if (sessObj.getReader().getReadMode() != SessionDefaults.OBEY_READ_MODE) {
						sessObj.getReader().setReadMode(OBEY_READ_MODE);
						sessObj.setSessionType(false);
					}

				} catch (FileNotFoundException fnfe) {
					sessionError('E', new ErrorObject(
							SessionError.OBEY_FILE_NOT_FOUND.errorCode(),
							SessionError.OBEY_FILE_NOT_FOUND.errorMessage()));
					return;
				} catch (IOException e) {
					throw e;
				}
			}
		}

	}

	private void resetSpoolOptions(FileWriter sWriter) throws IOException {
		sessObj.setSessionLogging(false);
		sessObj.getWriter().setWriterMode(CONSOLE_WRITE_MODE);
		sessObj.setQuietEnabled(false);
		sessObj.setLogCmdText(true);
		sessObj.setLogAppend(true);
		sessObj.setLogCmdEcho(true);
		sessObj.setSessionStartup(false);
		try {
			if (sWriter != null)
				sWriter.close();
			sWriter = null;
		} catch (IOException e) {
			throw e;
		}
	}

	private void execSpool() throws IOException {
		spoolFileName = parser.getNextFileToken();
		if (sessObj.getCaller() == PRUNI) {
			if (spoolFileName.equalsIgnoreCase("OFF")) {
				sessObj.setSessionStartup(false);
				sessObj.setSessionLogging(false);
			}
			return;
		}
		if (spoolFileName == null || spoolFileName.trim().equals("")) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}
		if (spoolFileName.endsWith(","))
			spoolFileName = spoolFileName.substring(0,
					spoolFileName.length() - 1);

		if (!spoolFileName.equalsIgnoreCase("OFF")) {
			if (spoolFileName.equalsIgnoreCase("ON")) {
				spoolFileName = "sqlspool.lst";
			}
			if (sessObj.isSessionLogging()) {
				resetSpoolOptions(spoolWriter);
				spoolWriter = null;
			}
		} else if (spoolFileName.equalsIgnoreCase("OFF")) {

			if ((spoolWriter == null) && !sessObj.isSessionStartup()) {
				sessionError('E', SessionError.SPOOL_OFF_OFF);
				return;
			} else {
				resetSpoolOptions(spoolWriter);
				spoolWriter = null;
				return;
			}
		}
		sessObj.setSpoolFileName(spoolFileName);
		String spoolOption = null;
		String cmdStr, cmdTextValue = null;
		String[] formatStrArr = parser.getRemainderStr().trim().split(",");

		for (int i = 0; i < formatStrArr.length; i++) {
			spoolOption = parser.getNextToken().trim();
			if (spoolOption != null
					&& !spoolOption.equalsIgnoreCase("")
					&& !spoolOption
							.matches("(?i)\\s*((CMDTEXT|CMDECHO)\\s+(ON|OFF))|(QUIET|CLEAR)")) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				resetSpoolOptions(spoolWriter);
				return;
			}
			if (spoolOption.matches("(?i)\\s*(CMDTEXT|CMDECHO)\\s+(ON|OFF)")) {
				cmdStr = spoolOption.substring(0, 7);
				cmdTextValue = spoolOption.trim().substring(8);
				if (cmdStr.equalsIgnoreCase("CMDECHO")) {
					if (cmdTextValue == null
							|| cmdTextValue.trim().equalsIgnoreCase("ON"))
						sessObj.setLogCmdEcho(true);
					else
						sessObj.setLogCmdEcho(false);

				} else if (cmdTextValue == null
						|| cmdTextValue.trim().equalsIgnoreCase("ON"))
					sessObj.setLogCmdText(true);
				else
					sessObj.setLogCmdText(false);
			} else if (spoolOption.equalsIgnoreCase("QUIET")) {
				sessObj.setQuietEnabled(true);
			} else if (spoolOption.equalsIgnoreCase("CLEAR")) {
				sessObj.setLogAppend(false);
			}
		}
		if (!spoolFileName.equalsIgnoreCase("OFF")) {
			try {
				spoolWriter = new FileWriter();

				if (sessObj.isLogAppend())
					spoolWriter.setAppend(true);
				spoolWriter.initialize(spoolFileName);
				sessObj.setSessionLogging(true);
				sessObj.getWriter().setSpoolWriter(spoolWriter);
				sessObj.getWriter().setWriterMode(CONSOLE_SPOOL_WRITE_MODE);
				if (sessObj.isLogCmdText()
						&& sessObj.getSessView() != SessionDefaults.MXCI_VIEW) {
					spoolWriter.writeln((utils.formatString("=", 80, '=')));
					spoolWriter.writeln("Spooling started at "
							+ DateFormat.getDateTimeInstance().format(
									new Date()));
					spoolWriter.writeln((utils.formatString("=", 80, '=')));
				}
			} catch (FileNotFoundException fnfe) {
				sessionError('E',
						new ErrorObject(SessionError.INTERNAL_ERR.errorCode(),
								fnfe.getMessage()));
				spoolWriter = null;
				return;
			} catch (IOException e) {
				spoolWriter = null;
				throw e;
			}
		}
	}

	private void execEnv() throws IOException, UnKnownInterfaceCommand,
			UserInterruption {
		if (parser.hasMoreTokens()) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}

		// writer.writeln("CATALOG        ",(sessObj.getSessionCtlg()));
		handleOutput("COLSEP         ", "\"" + sessObj.getSessionColSep()
				+ "\"");
		handleOutput("HISTOPT        ", (sessObj.isSessionHistoryAll() ? "ALL"
				: "DEFAULT [No expansion of script files]"), false);
		handleOutput("IDLETIMEOUT    ", (sessObj.getSessionIdletime())
				+ " min(s) "
				+ (sessObj.getSessionIdletime() == 0 ? "[Never Expires]" : ""),
				false);
		if (sessObj.getListCount() == 0)
			handleOutput("LIST_COUNT     ", (sessObj.getListCount())
					+ " [All Rows]", false);
		else
			handleOutput("LIST_COUNT     ",
					String.valueOf((sessObj.getListCount())), false);
		if (sessObj.isSessionLogging()) {
			handleOutput("LOG FILE       ", sessObj.getSpoolFileName(), false);
			handleOutput("LOG OPTIONS    ", (sessObj.isLogAppend() ? "APPEND"
					: "CLEAR")
					+ (sessObj.isQuietEnabled() ? ",QUIET" : "")
					+ ",CMDTEXT " + (sessObj.isLogCmdText() ? "ON" : "OFF"),
					false);
		} else
			handleOutput("LOG            ", "OFF", false);
		handleOutput("MARKUP         ", (sessObj.getStrDisplayFormat()), false);
		handleOutput("PROMPT         ", (sessObj.getSessionPrompt()), false);
		if (sessObj.isDBConnExists())
		{
			handleOutput("SCHEMA         ", (sessObj.getSessionSchema()), false);
			handleOutput("SERVER         ", sessObj.getSessionServer()
					+ sessObj.getSessionPort(), false);
		} else {
			handleOutput("SCHEMA         ", (this.infoNotAvailable), false);
			handleOutput("SERVER         ", (this.infoNotAvailable), false);
		}

		handleOutput("SQLTERMINATOR  ", (sessObj.getSessionSQLTerminator()),
				false);
		handleOutput("STATISTICS     ", (sessObj.isSessionStatsEnabled() ? "ON"
				: "OFF"), false);
		handleOutput("TIME           ", (sessObj.isSessionTimeOn() ? "ON"
				: "OFF"), false);
		handleOutput("TIMING         ", (sessObj.isSessionTimingOn() ? "ON"
				: "OFF"), false);
		if (sessObj.isDBConnExists())// Changed  2010-03-01
		{
			handleOutput("USER           ", (sessObj.getSessionUser()), false);
		} else {
			handleOutput("USER           ", (this.infoNotAvailable), false);
		}
		writer.writeEndTags(sessObj);
	}

	private void execShow() throws UnKnownInterfaceCommand, SQLException,
			IOException, UserInterruption {
		String showOption = parser.getNextKeyToken();

		if (showOption == null) {
			if (parser.hasMoreTokens()) {
				throw uic;
			}
			sessionError('E', SessionError.SHOW_NO_OPTION);
			return;
		}

		showOption = iqKeyMap.get("SHOW_" + showOption.toUpperCase());

		if (showOption == null) {
			throw uic;
		}

		int showId = Integer.parseInt(showOption);
		sessObj.getQuery().setQueryId(showId);

//		String setValue;

		switch (showId) {
		case SHOW_TIME:
			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			handleOutput("TIME", (sessObj.isSessionTimeOn() ? "ON" : "OFF"));
			break;

		case SHOW_TIMING:

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			handleOutput("TIMING", (sessObj.isSessionTimingOn() ? "ON" : "OFF"));
			break;

		case SHOW_SQLTERMINATOR:

			if (sessObj.getMode() != SessionDefaults.SQL_MODE) {
				this.invalidCmdforMode(sessObj.getMode());
				break;
			}

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			handleOutput("SQLTERMINATOR", (sessObj.getSessionSQLTerminator()));
			break;

		case SHOW_SQLPROMPT:

			//change for show schemas can't executed in cs mode
			if (sessObj.getMode() != SessionDefaults.SQL_MODE && sessObj.getMode() != SessionDefaults.CS_MODE) {
//			if (sessObj.getMode() != SessionDefaults.SQL_MODE) {
				this.invalidCmdforMode(sessObj.getMode());
				break;
			}

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			handleOutput("SQLPROMPT", (sessObj.getSessionPrompt()));
			break;

		case SHOW_CATALOG:

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			if (sessObj.isDBConnExists()) // Modified  2010-03-01
			{
				handleOutput("CATALOG", (sessObj.getSessionCtlg()));
			} else {
				displayNoConnInfo();
			}
			break;

		case SHOW_SESSION:

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			execEnv();
			break;

		case SHOW_SCHEMA:

			if (parser.getNextKeyToken() != null
					|| parser.getNextValueToken() != null) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			if (sessObj.isDBConnExists()) // Modified  2010-03-01
			{
				handleOutput("SCHEMA", (sessObj.getSessionSchema()));
			} else {
				displayNoConnInfo();
			}

			break;

		case SHOW_ROLE:

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			if (sessObj.isDBConnExists()) // Modified  2010-03-01
			{
				handleOutput("ROLE", (sessObj.getSessionRole()));
			} else {
				displayNoConnInfo();
			}

			break;

		case SHOW_IDLETIMEOUT:

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			handleOutput("IDLETIMEOUT", (sessObj.getSessionIdletime())
					+ " min(s) "
					+ (sessObj.getSessionIdletime() == 0 ? "[Never Expires]"
							: ""));
			break;

		case SHOW_LISTCOUNT:

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			handleOutput(
					"LIST_COUNT",
					(sessObj.getListCount())
							+ (sessObj.getListCount() == 0 ? " [All Rows]" : ""));
			break;

		case SHOW_DISPLAY_COLSIZE:

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			handleOutput("DISPLAY COLSIZE",
					String.valueOf(sessObj.getDisplayColSize()));
			break;

		case SHOW_PROCESSNAME:

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			if (sessObj.isDBConnExists()) // Modified  2010-03-01
			{
				String processName = sessObj.getNeoProcessName();
				handleOutput("REMOTE PROCESS", processName);
			} else {
				displayNoConnInfo();
			}

			break;

		case SHOW_MODE:
			writer.writeln();
			writer.writeError(sessObj, SessionError.CMD_NOT_SEAQUEST_SUPPORTED);
/*
 * remove show mode command for seaquest
 */
//			if (parser.hasMoreTokens()) {
//				writeSyntaxError(this.queryStr, parser.getRemainderStr());
//				break;
//			}
//			handleOutput("MODE", sessObj.getStrMode());
			break;

		case SHOW_COLSEP:

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			handleOutput("COLSEP", ("\"" + sessObj.getSessionColSep() + "\""));
			break;

		case SHOW_MARKUP:

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			handleOutput("MARKUP", sessObj.getStrDisplayFormat());
			break;

		case SHOW_PARAM:
		case SHOW_PARAMS:

			totalCount = 0;
			String paramName = parser.getNextParamToken();
			if (paramName != null) {
				if (parser.hasMoreTokens()) {
					writeSyntaxError(this.queryStr, parser.getRemainderStr());
				} else if (sessObj.getSessParams() != null) {
					if (sessObj.getSessParams(paramName) != null) {
						handleOutput(paramName,
								sessObj.getSessParams(paramName), false);
						totalCount++;
					} else {
						sessionError('E', SessionError.SHOW_PARAM_NOT_FOUND);
					}

				}
				break;
			}

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			} else {
				HashMap<String,String> paramHash = sessObj.getSessParams();
				if ((paramHash != null) && !paramHash.isEmpty()) {
					Map<String, String> sortedMap = new TreeMap<String, String>(paramHash);
					Iterator<String> paramNames = sortedMap.keySet().iterator();
					for (int paramCnt = 0; paramCnt < paramHash.size(); paramCnt++) {
						if (paramNames.hasNext()) {
							String pName = (String) paramNames.next();
							handleOutput(pName,
									paramHash.get(pName).toString(), false);
							totalCount++;
						}
					}
					printElapsedTime();
					writer.writeEndTags(sessObj);
				} else {
					sessionError('E', SessionError.SHOW_PARAM_NOT_FOUND);
				}

			}

			sessObj.setTotalRecordCount(String.valueOf(totalCount));
			break;

		case SHOW_TABLES:
		case SHOW_SCHEMAS:
		case SHOW_CATALOGS:
		case SHOW_VIEWS:
		case SHOW_SYNONYMS:
		case SHOW_MVS:
		case SHOW_MVGROUPS:
		case SHOW_PROCEDURES:

			totalCount = 0;
			String tableNamePattern = null;

			if (sessObj.getMode() != SessionDefaults.SQL_MODE && sessObj.getMode() != SessionDefaults.CS_MODE) {
				this.invalidCmdforMode(sessObj.getMode());
				break;
			}
			tableNamePattern = parser.getNextQValueToken();
			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			if (!sessObj.isDBConnExists()) // Modified  2010-03-01
			{
				displayNoConnInfo();
				break;
			}

			ResultSet rsmd = null;
			// default is TABLE_NAME
			String getStr = SqlEnum.TABLE_NAME.toString();
			;

			DatabaseMetaData dbmd = conn.getMetaData();

			String displayStr = null;
			ErrorObject errObj = null;

			if (tableNamePattern != null) {

				if ((tableNamePattern.indexOf("*") != -1)
						|| (tableNamePattern.indexOf("?") != -1))

					tableNamePattern = parser
							.replaceShowPattern(tableNamePattern);

			}
			try {
				switch (showId) {
				case SHOW_TABLES:
					String[] ttype = { SqlEnum.TABLE.toString() };
					rsmd = dbmd
							.getTables(sessObj.getSessionCtlg(),
									sessObj.getSessionSchema(),
									tableNamePattern, ttype);
					displayStr = "TABLE NAMES";
					errObj = SessionError.SHOW_TAB_NOT_FOUND;
					tableNamePattern = null;
					break;

				case SHOW_VIEWS:
					String[] vtype = { "VIEW" };
					rsmd = dbmd
							.getTables(sessObj.getSessionCtlg(),
									sessObj.getSessionSchema(),
									tableNamePattern, vtype);
					displayStr = "VIEW NAMES";
					errObj = SessionError.SHOW_VIEW_NOT_FOUND;
					tableNamePattern = null;
					break;

				case SHOW_SYNONYMS:
					String[] stype = { SqlEnum.SYNONYM.toString() };
					rsmd = dbmd
							.getTables(sessObj.getSessionCtlg(),
									sessObj.getSessionSchema(),
									tableNamePattern, stype);
					displayStr = "SYNONYM NAMES";
					errObj = SessionError.SHOW_SYNONYM_NOT_FOUND;
					tableNamePattern = null;
					break;

				case SHOW_MVS:
					String[] mvtype = { "MV" };
					rsmd = dbmd.getTables(sessObj.getSessionCtlg(),
							sessObj.getSessionSchema(), tableNamePattern,
							mvtype);
					displayStr = "MATERIALIZED VIEW NAMES";
					errObj = SessionError.SHOW_MV_NOT_FOUND;
					tableNamePattern = null;
					break;

				case SHOW_MVGROUPS:
					String[] mvgtype = { "MVG" };
					rsmd = dbmd.getTables(sessObj.getSessionCtlg(),
							sessObj.getSessionSchema(), tableNamePattern,
							mvgtype);
					displayStr = "MATERIALIZED VIEW GROUP NAMES";
					errObj = SessionError.SHOW_MVG_NOT_FOUND;
					tableNamePattern = null;
					break;

				case SHOW_CATALOGS:
					rsmd = dbmd.getCatalogs();
					getStr = "TABLE_CAT";
					displayStr = "CATALOG NAMES";
					errObj = SessionError.SHOW_CAT_NOT_FOUND;
					break;

				case SHOW_SCHEMAS:
					rsmd = dbmd.getSchemas();
					getStr = "TABLE_SCHEM";
					displayStr = "SCHEMA NAMES";
					errObj = SessionError.SHOW_SCH_NOT_FOUND;
					break;

				case SHOW_PROCEDURES:
					rsmd = dbmd.getProcedures(sessObj.getSessionCtlg(),
							sessObj.getSessionSchema(), tableNamePattern);
					getStr = "PROCEDURE_NAME";
					displayStr = "PROCEDURE NAMES";
					errObj = SessionError.SHOW_PROC_NOT_FOUND;
					tableNamePattern = null;
					break;

				}
				// Temporary fix till JDBC fixes this issue.
			} catch (NullPointerException npe) {
				errObj = SessionError.INTERNAL_ERR;
			}

			if ((tableNamePattern != null)
					&& (tableNamePattern.trim().equals(""))) {
				tableNamePattern = null;
			}
			if (tableNamePattern != null) {
				tableNamePattern = utils.trimDoubleQuote(tableNamePattern);
				try {
					tableNamePattern = tableNamePattern.replaceAll("_", ".");
					tableNamePattern = tableNamePattern.replaceAll("%", ".*");
				} catch (PatternSyntaxException pse) {
					tableNamePattern = "";
				}

			}

			List<String> results = new ArrayList<String>();
			int maxDisplaySize = 0;
			int displayCols = 0;
			if (rsmd == null || !rsmd.next()) {
				sessionError('E', errObj);
			} else {
				int currentColLength = 0;

				do {
					if ((currentColLength = rsmd.getString(getStr).getBytes().length) > maxDisplaySize) {
						maxDisplaySize = currentColLength;
					}
					if (tableNamePattern == null) {
						results.add(rsmd.getString(getStr));

					} else {
						try {
							if (rsmd.getString(getStr)
									.matches(tableNamePattern)) {
								results.add(rsmd.getString(getStr));

							}
						} catch (PatternSyntaxException pse) {
							tableNamePattern = "";
						}

					}
				} while (rsmd.next());

				if (results.size() == 0) {
					sessionError('E', errObj);
				} else {

					switch (sessObj.getDisplayFormat()) {
					case SessionDefaults.HTML_FORMAT:
						sessObj.getHtmlObj().init();
						sessObj.getHtmlObj().handleStartTags();
						writer.writeln("<tr>");
						writer.writeln("  <th>" + displayStr + "</th>");
						writer.writeln("</tr>");
						break;
					case SessionDefaults.XML_FORMAT:
						sessObj.getXmlObj().init();
						sessObj.getXmlObj().handleStartTags();
						writer.writeln("   <" + utils.removeSpaces(displayStr)
								+ ">");
						break;
					case SessionDefaults.CSV_FORMAT:
						writer.writeln();
						break;
					default:
						// print the header label
						writer.writeln();
						writer.writeln(displayStr);
						writer.write(utils.formatString("-", 80, '-'));
						writer.writeln();
						break;
					}

					if (maxDisplaySize == 0 || maxDisplaySize > 60) {
						displayCols = 1;
					} else {
						displayCols = (int) Math
								.floor(96 / (maxDisplaySize + 2));

					}

					for (int j = 0; j < results.size(); j++) {
						switch (sessObj.getDisplayFormat()) {
						case SessionDefaults.XML_FORMAT:
							writer.writeln("     <Name>"
									+ utils.formatXMLdata(results.get(j)
											.toString()) + "</Name>");
							break;
						case SessionDefaults.HTML_FORMAT:
							writer.writeln("<tr>");
							writer.writeln("  <td>" + results.get(j).toString()
									+ "</td>");
							writer.writeln("</tr>");
							break;
						case SessionDefaults.CSV_FORMAT:
							writer.writeln(results.get(j).toString());
							break;
						default:
							formatOutput(results.get(j).toString(),
									maxDisplaySize + 2, ' ', 0);
							if ((j + 1) % displayCols == 0) {
								writer.writeln();
							}
							break;
						}
						totalCount++;
					}

					sessObj.setQryEndTime();

				}
			}

			switch (sessObj.getDisplayFormat()) {
			case SessionDefaults.HTML_FORMAT:
				if (sessObj.isSessionTimingOn())
					writer.writeln(sessObj.getHtmlObj()._startCommentTag
							+ writer.getElapsedTime(sessObj, qryObj, utils)
							+ sessObj.getHtmlObj()._endCommentTag);
				writer.printElapsedQuietMode(writer.getElapsedTime(sessObj,
						qryObj, utils));
				sessObj.getHtmlObj().handleEndTags();
				break;
			case SessionDefaults.XML_FORMAT:
				if (results.size() != 0)
					writer.writeln("   </" + utils.removeSpaces(displayStr)
							+ ">");
				if (sessObj.isSessionTimingOn())
					writer.writeln(sessObj.getXmlObj()._beginCdataTag
							+ writer.getElapsedTime(sessObj, qryObj, utils)
							+ sessObj.getXmlObj()._endCdataTag);

				writer.printElapsedQuietMode(writer.getElapsedTime(sessObj,
						qryObj, utils));
				sessObj.getXmlObj().handleEndTags();
				break;
			case SessionDefaults.CSV_FORMAT:
				writer.writeln();
				if (results.size() != 0 && sessObj.isSessionTimingOn()) {
					writer.writeln(writer
							.getElapsedTime(sessObj, qryObj, utils));
					writer.printElapsedQuietMode(writer.getElapsedTime(sessObj,
							qryObj, utils));
				}
				break;
			default:
				if (results.size() != 0 && results.size() % displayCols != 0) {
					writer.writeln();
				}
				if (sessObj.isSessionTimingOn()) {
					writer.writeln();
					writer.writeln(writer
							.getElapsedTime(sessObj, qryObj, utils));
					writer.printElapsedQuietMode(writer.getElapsedTime(sessObj,
							qryObj, utils));
				}
				break;
			}
			results = null;

			sessObj.setTotalRecordCount(String.valueOf(totalCount));
			sessObj.setLastError(0);
			break;
		/**
		 * show indexes [order by table] By default: order by index names
		 */
		case SHOW_INDEXES:
			boolean orderByIndex = true;
			if (parser.hasMoreTokens()) {
				String remainderStr = parser.getRemainderStr();
				if (!remainderStr.equalsIgnoreCase(" order by table")) {
					writeSyntaxError(this.queryStr, remainderStr);
					break;
				} else
					orderByIndex = false;
			}
			if (sessObj.isDBConnExists())
				this.handleShowIndexes(orderByIndex);
			else
				displayNoConnInfo();

			break;

		case SHOW_TABLE:

			String ctlgName = null;
			String schemaName = null;
			String tableName = null;
			String prepTableName = null;
			String[] validOptions = { SqlEnum.INDEXES.toString(),
					SqlEnum.SYNONYMS.toString(), SqlEnum.MVS.toString(),
					SqlEnum.TRIGGERS.toString() };
			int headingLength = 80;

			if (sessObj.getMode() != SessionDefaults.SQL_MODE) {
				this.invalidCmdforMode(sessObj.getMode());
				break;
			}

			if (!parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				return;
			}

			if (!sessObj.isDBConnExists()) {
				displayNoConnInfo();
				break;
			}

			List<String> cstList = parser.getCSTList(parser.getRemainderStr());

			if (cstList.size() == 0) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			if (parser.showTableIdxRemainder != null) {
				prepTableName = parser.getRemainderStr().substring(
						0,
						parser.getRemainderStr().length()
								- parser.showTableIdxRemainder.length());
				parser.setRemainderStr(parser.getRemainderStr().substring(
						prepTableName.length(),
						parser.getRemainderStr().length()));
			} else {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			if (!parser.getRemainderStr().trim().startsWith(",")) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			} else {
				parser.setRemainderStr(parser.getRemainderStr().substring(1,
						parser.getRemainderStr().length()));
			}

			switch (cstList.size()) {
			case 3:
				ctlgName = cstList.get(0);
				schemaName = cstList.get(1);
				tableName = cstList.get(2);
				break;
			case 2:
				ctlgName = sessObj.getSessionCtlg();
				schemaName = cstList.get(0);
				tableName = cstList.get(1);
				break;
			case 1:
				ctlgName = sessObj.getSessionCtlg();
				schemaName = sessObj.getSessionSchema();
				tableName = cstList.get(0);

				break;
			}

			String keyWordToken = parser.getNextKeyToken();
			String[] tmpOptions = null;
			if (keyWordToken == null) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			if (keyWordToken.equalsIgnoreCase("ALL")) {
				tmpOptions = validOptions;
			} else {
				tmpOptions = new String[1];
				tmpOptions[0] = keyWordToken;
			}

			int howManyOptions = tmpOptions.length;
			handleShowTableOptions(howManyOptions, keyWordToken, prepTableName,
					tmpOptions, ctlgName, schemaName, tableName, headingLength);
			break;

		case SHOW_PREPARED:

			totalCount = 0;
			if (sessObj.getMode() != SessionDefaults.SQL_MODE) {
				this.invalidCmdforMode(sessObj.getMode());
				break;
			}

			PreparedStatement pStmt = null;
			String stmtNamePattern = parser.getNextQValueToken();
			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			if (!sessObj.isDBConnExists()) // Modified  2010-03-01
			{
				displayNoConnInfo();
				break;
			}

			HashMap<String, Object[]> prepStmtMap = sessObj.getPrepStmtMap();
			if ((prepStmtMap == null) || (prepStmtMap.isEmpty())) {
				sessionError('E', SessionError.SHOW_PREP_NOT_FOUND);
				break;
			}
			if (stmtNamePattern != null) {

				if ((stmtNamePattern.indexOf("*") != -1)
						|| (stmtNamePattern.indexOf("?") != -1))

					stmtNamePattern = parser
							.replaceShowPrepPattern(stmtNamePattern);

				stmtNamePattern = utils.trimDoubleQuote(stmtNamePattern);
				try {
					stmtNamePattern = stmtNamePattern.replaceAll("_", ".");
					stmtNamePattern = stmtNamePattern.replaceAll("%", ".*");
				} catch (PatternSyntaxException pse) {
					stmtNamePattern = "";
				}
			}
			Map<String, Object[]> sortedMap = new TreeMap<String, Object[]>(prepStmtMap);
			Iterator<String> it = sortedMap.keySet().iterator();
			String key = null;
			boolean found = false;

			while (it.hasNext()) {
				key = it.next();
				try {
					if ((stmtNamePattern == null || key
							.matches(stmtNamePattern))
							&& !key.equals(sessObj.getAutoPrepStmtName())) {
						pStmt = (PreparedStatement) sessObj.getPrepStmtMap(key);
						handleOutput(key, ((TrafT4Statement) pStmt).getSQL(),
								false);
						found = true;
						totalCount++;
					}
				} catch (PatternSyntaxException pse) {
					// No action required here
				}
			}

			// If no matching statement names were found
			// display a status message
			if (!found) {
				sessionError('E', SessionError.SHOW_PREP_NOT_FOUND);
			}
			sessObj.setTotalRecordCount(String.valueOf(totalCount));
			printElapsedTime();
			writer.writeEndTags(sessObj);
			break;

		case SHOW_HISTOPT:

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			handleOutput("HISTOPT", (sessObj.isSessionHistoryAll() ? "ALL"
					: "DEFAULT [No expansion of script files]"));
			break;

		case SHOW_AUTOPREPARE:

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			handleOutput("AUTOPREPARE", (sessObj.isSessionAutoPrepare() ? "ON"
					: "OFF"));
			break;

		case SHOW_ERRORCODE:
		case SHOW_LASTERROR:

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			if (showId == SHOW_LASTERROR)
				handleOutput("LASTERROR",
						String.valueOf(sessObj.getLastError()));
			else
				handleOutput("ERRORCODE",
						String.valueOf(sessObj.getLastError()));

			break;

		case SHOW_ACTIVITYCOUNT:
		case SHOW_RECCOUNT:

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			if (!sessObj.isDBConnExists()) // Modified  2010-03-01
			{
				displayNoConnInfo();
				break;
			}

			String recCount = sessObj.getTotalRecordCount();
			if (showId == SHOW_RECCOUNT)
				handleOutput("RECCOUNT", (recCount != null ? recCount : "0"));
			else
				handleOutput("ACTIVITYCOUNT", (recCount != null ? recCount
						: "0"));

			break;

		case SHOW_STATISTICS:
			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			handleOutput("STATISTICS", (sessObj.isSessionStatsEnabled() ? "ON"
					: "OFF"));
			break;

		case SHOW_ACCESS:
			if (!parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			handleDumpFileAccess();

			break;

		case SHOW_LOOKANDFEEL:

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			handleOutput("LOOK AND FEEL", (sessObj.getStrSessView()));
			break;

		case SHOW_ALIAS:
			handleShowAlias();
			break;

		case SHOW_ALIASES:
			handleShowAliases();
			break;

		case SHOW_FETCHSIZE:

			if (!checkForMoreTokens())
				handleOutput(
						"FETCHSIZE",
						String.valueOf(sessObj.getFetchSize())
								+ (sessObj.getFetchSize() == 0 ? " [Default]"
										: ""));
			break;

		case SHOW_PATTERNDEPTH:

			if (!checkForMoreTokens())
				handleOutput("PATTERNDEPTH",
						String.valueOf(sessObj.getMaxPatternDepth()));
			break;

		case SHOW_PATTERN:
			handleShowPattern();
			break;

		case SHOW_PATTERNS:
			handleShowPatterns();
			break;

		case SHOW_DEBUG:
			checkForMoreTokens();
			handleOutput("DEBUG", (sessObj.isDebugOn() ? "ON" : "OFF"));
			break;

		case SHOW_CMDDISPLAY:
			checkForMoreTokens();
			handleOutput("CMDDISPLAY", (sessObj.isLogCmdEcho() ? "ON" : "OFF"));
			break;

		case SHOW_USER:
			checkForMoreTokens();
			if (sessObj.isDBConnExists()) {
				handleOutput(
						"USER",
						sessObj.getSessionUser() + " ("
								+ sessObj.getSessionRole() + ")");
			} else {
				displayNoConnInfo();
			}
			break;

		case SHOW_SERVER:
			checkForMoreTokens();
			if (sessObj.isDBConnExists())
				handleOutput("SERVER:PORT", sessObj.getSessionServer()
						+ sessObj.getSessionPort());
			else
				displayNoConnInfo();
			break;

		case SHOW_CONNECTOPT:
			checkForMoreTokens();
			handleOutput("CONNECTOPT",
					(sessObj.getTempSessionRole().length() == 0 ? "DEFAULT"
							: sessObj.getTempSessionRole()));
			break;
		}
	}

	private void execSet() throws UnKnownInterfaceCommand, IOException,
			SQLException, UserInterruption {

		String setOption = parser.getNextKeyToken();
		int setOpt;

		// not a keyword..then its not ours

		if (setOption == null) {
			if (parser.hasMoreTokens()) {
				throw uic;
			}
			sessionError('E', SessionError.SET_NO_OPTION);
			return;
		}

		setOption = iqKeyMap.get("SET_" + setOption.toUpperCase());

		if (setOption == null) {
			throw uic;
		}

		int setId = Integer.parseInt(setOption);

		String setValue = "";

		switch (setId) {
		case SET_CONNECTOPT:
			setValue = parser.getNextKeyToken();
			if (setValue == null) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			if (!setValue.toUpperCase().equals("ROLE")) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			setValue = parser.getNextValueToken();
			if (setValue == null) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			} else {
				sessObj.setTempSessionRole(setValue);
			}
			break;

		case SET_TIME:

			setValue = parser.getNextKeyToken();
			if (setValue == null) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			setValue = setValue.toUpperCase();
			if ((!setValue.equals("ON") && !setValue.equals("OFF"))) {
				writeSyntaxError(this.queryStr,
						setValue + parser.getRemainderStr());
				break;
			}
			if (setValue.equals("OFF") && parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr,
						setValue + parser.getRemainderStr());
				break;
			}
			String timeFormat = parser.getNextKeyToken();

			if ((timeFormat != null) && !timeFormat.equalsIgnoreCase("12h")) {
				writeSyntaxError(this.queryStr,
						setValue + parser.getRemainderStr());
				break;
			}
			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			if (setValue.equals("ON")) {
				sessObj.setSessionTime(true);
				if ((timeFormat != null)
						&& (timeFormat.equalsIgnoreCase("12h")))
					sessObj.setAmPmFmt(true);
				else
					sessObj.setAmPmFmt(false);

			} else {
				sessObj.setSessionTime(false);
			}
			envMap.put("TIME", (sessObj.isSessionTimeOn() ? "ON" : "OFF"));

			break;

		case SET_TIMING:

			setOpt = getSetValue();
			if (setOpt != -1) {
				sessObj.setSessionTiming(setOpt == 1 ? true : false);
			}
			envMap.put("TIMING", (sessObj.isSessionTimingOn() ? "ON" : "OFF"));
			break;

		case SET_SQLPROMPT:

			if (sessObj.getMode() != SessionDefaults.SQL_MODE) {
				this.invalidCmdforMode(sessObj.getMode());
				break;
			}

		case SET_PROMPT:

			setValue = parser.getNextValueToken();
			String regx1 = "\\s*(?i)";
			String regx2 = "\\s*[>\\s]*\\s*";
			if (setValue == null) {
				if (SessionDefaults.SQL_MODE == sessObj.getMode())
					sessObj.setSessionPrompt(SessionDefaults.DEFAULT_SQL_PROMPT);
				else if (SessionDefaults.CS_MODE == sessObj.getMode())
					sessObj.setSessionPrompt(SessionDefaults.DEFAULT_CS_PROMPT);
				else if (SessionDefaults.WMS_MODE == sessObj.getMode())
					sessObj.setSessionPrompt(SessionDefaults.DEFAULT_WMS_PROMPT);
				envMap.put("PROMPT", sessObj.getSessionPrompt());
				break;
			} else if (setValue.matches(regx1 + SessionDefaults.SQL + regx2)) {
				sessionError('E', SessionError.SQL_RESERVED_WORD_ERROR);
				break;
			} else if (setValue.matches(regx1 + SessionDefaults.CS + regx2)) {
				sessionError('E', SessionError.CS_RESERVED_WORD_ERROR);
				break;
			} else if (setValue.matches(regx1 + SessionDefaults.WMS + regx2)) {
				sessionError('E', SessionError.WMS_RESERVED_WORD_ERROR);
				break;
			}

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			} else {
				// Added  to deal with non-connection situation
				if (!sessObj.isDBConnExists()
						&& (setValue.toUpperCase().indexOf("%USER") != -1
								|| setValue.toUpperCase().indexOf("%SCHEMA") != -1
								|| setValue.toUpperCase().indexOf("%SERVER") != -1
								|| setValue.toUpperCase().indexOf("%MODE") != -1
								|| setValue.toUpperCase()
										.indexOf("%DATASOURCE") != -1 || setValue
								.toUpperCase().indexOf("%ROLE") != -1)) {
					displayNoConnInfo();
					break;
				}
				sessObj.setSessionPrompt(setValue);
			}
			envMap.put("PROMPT", sessObj.getSessionPrompt());
			break;

		case SET_SQLTERMINATOR:

			if (sessObj.getMode() != SessionDefaults.SQL_MODE) {
				this.invalidCmdforMode(sessObj.getMode());
				break;
			}
			setValue = parser.getNextValueToken();
			if (setValue == null) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			} else {
				sessObj.setSessionSQLTerminator(setValue);
				sessObj.setTermEventMap(
						Integer.toString(sessObj.getHistCmdNo() + 1), setValue);
				sessObj.setTermEvent(Integer.toString(sessObj.getHistCmdNo() + 1));
			}
			envMap.put("SQLTERMINATOR", sessObj.getSessionSQLTerminator());
			break;

		case SET_IDLETIMEOUT:
			int intSetValue = 0;
			setValue = parser.getNextNumberToken();
			if (setValue == null || setValue.trim().equals("")) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			} else {
				try {
					intSetValue = Integer.parseInt(setValue);
					if (intSetValue < 0)
						throw (new NumberFormatException());
					else
						sessObj.setSessionIdletime(intSetValue);
					envMap.put("IDLETIMEOUT",
							Integer.toString(sessObj.getSessionIdletime()));
				} catch (NumberFormatException nfe) {
					sessionError('E', SessionError.SET_IDLETIMEOUT_VAL_ERR);
				}
			}

			break;

		case SET_PARAM:

			String paramName = parser.getNextParamToken();
			if (paramName == null) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			/**
			 * Use the new method to get the parameter value and validate that
			 * the value is syntactically correct.
			 */
			ParamStringObject psv = parser.getParamValue(null);
			if (null != psv) {
				setValue = psv.toString();

				try {
					if (sessObj.isDebugOn())
						System.out.println("@@@Debug: InterfaceQuery:: psv = "
								+ psv.toString());

					String theActualValue = psv.getParameterValue();

					if (sessObj.isDebugOn())
						System.out.println("@@@Debug: InterfaceQuery:: psv "
								+ "getParamValue= " + theActualValue);

				} catch (Exception exc) {
					setValue = null;

					ErrorObject eo = new ErrorObject(
							SessionError.INVALID_PARAM_STRING_VALUE, "",
							exc.getMessage());
					sessionError('E', eo);
					break;
				}

			} else
				setValue = null;

			if (sessObj.isDebugOn())
				System.out.println("@@@Debug: InterfaceQuery:: setValue "
						+ setValue);

			if (setValue == null) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			if (sessObj.isDebugOn())
				System.out.println("@@@Debug: InterfaceQuery:: parser has "
						+ "more tokens = " + parser.hasMoreTokens());

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			sessObj.setSessParams(paramName, setValue);
			break;

		case SET_DISPLAY_COLSIZE:
			if (sessObj.getMode() != SessionDefaults.WMS_MODE) {
				this.invalidCmdforMode(sessObj.getMode());
				break;
			}
			setValue = parser.getNextValueToken();
			if (setValue == null) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			} else {
				try {
					int dispColSize = Integer.parseInt(setValue);
					if (dispColSize <= 0)
						throw (new NumberFormatException());
					else {
						sessObj.setDisplayColSize(dispColSize);
					}
				} catch (NumberFormatException nfe) {
					// sessionError('E',SessionError.SET_LISTCOUNT_VAL_ERR);
				}
			}
			break;

		case SET_LISTCOUNT:
			if (sessObj.getMode() != SessionDefaults.SQL_MODE) {
				this.invalidCmdforMode(sessObj.getMode());
				break;
			}
			int intListCntValue = 0;
			setValue = parser.getNextNumberToken();
			if (setValue == null || setValue.trim().equals("")) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}

			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			} else {
				try {
					intListCntValue = Integer.parseInt(setValue);
					if (intListCntValue < 0)
						throw (new NumberFormatException());
					else {
						sessObj.setListCount(intListCntValue);
					}
					envMap.put("LIST_COUNT",
							Integer.toString(sessObj.getListCount()));
				} catch (NumberFormatException nfe) {
					sessionError('E', SessionError.SET_LISTCOUNT_VAL_ERR);
				}
			}
			break;

		case SET_COLSEP:

			setValue = parser.getNextValueToken();
			if (setValue == null) {
				sessObj.setSessionColSep(" ");
				break;
			}
			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			} else {
				sessObj.setSessionColSep(setValue);
			}
			envMap.put("COLSEP", sessObj.getSessionColSep());
			break;

		case SET_HISTOPT:

			setValue = parser.getNextKeyToken();
			if (setValue == null) {
				sessObj.setSessionHistoryAll(false);
				break;
			}
			setValue = setValue.toUpperCase();
			if (!setValue.equals("ALL") && !setValue.equals("DEFAULT")) {
				writeSyntaxError(this.queryStr,
						setValue + parser.getRemainderStr());
				break;
			}
			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			if (setValue.equals("ALL")) {
				sessObj.setSessionHistoryAll(true);
			} else {
				sessObj.setSessionHistoryAll(false);
			}
			envMap.put("HISTOPT", sessObj.isSessionHistoryAll() ? "ALL"
					: "DEFAULT [No expansion of script files]");
			break;

		case SET_STATISTICS:

			setValue = parser.getNextKeyToken();
			if (setValue == null) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			setValue = setValue.toUpperCase();
			if (!setValue.equals("ON") && !setValue.equals("OFF")
                              && !setValue.equals("PERTABLE") && !setValue.equals("PROGRESS") && !setValue.equals("DEFAULT")
                              && !setValue.equals("ALL")) {
				writeSyntaxError(this.queryStr,
						setValue + parser.getRemainderStr());
				break;
			}
			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			if (setValue.equals("OFF")) {
				sessObj.setSessionStatsEnabled(false);
			} else {
				sessObj.setSessionStatsEnabled(true);
			}
                        sessObj.setSessionStatsType(setValue);
			envMap.put("STATISTICS", setValue);
			break;

		case SET_MARKUP:

			setValue = parser.getNextValueToken();
			if (setValue == null) {
				sessObj.setStrDisplayFormat("RAW");
				break;
			}

			if ((!setValue.toUpperCase().equals("RAW"))
					&& (!setValue.toUpperCase().equals("XML"))
					&& (!setValue.toUpperCase().equals("HTML"))
					&& (!setValue.toUpperCase().equals("CSV"))
					&& (!setValue.toUpperCase().equals("COLSEP"))) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			} else {
				sessObj.setStrDisplayFormat(setValue);
			}
			envMap.put("MARKUP", sessObj.getStrDisplayFormat());
			break;

		case SET_AUTOPREPARE:

			if (sessObj.getMode() != SessionDefaults.SQL_MODE) {
				this.invalidCmdforMode(sessObj.getMode());
				break;
			}
			setOpt = getSetValue();
			if (setOpt != -1) {
				sessObj.setSessionAutoPrepare(setOpt == 1 ? true : false);
			}
			break;

		case SET_LOOKANDFEEL:

			setValue = parser.getNextValueToken();
			if (setValue == null || setValue == "") {
				sessObj.setSessView(SessionDefaults.CIDEFAULT_VIEW);
				sessObj.setSessionPrompt(SessionDefaults.DEFAULT_SQL_PROMPT);
				break;
			}
			if ((!setValue.matches("(?i)\\s*(MXCI|TRAFCI)"))
					|| parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			} else {
				if (setValue.equalsIgnoreCase("MXCI")) {
					sessObj.setSessView(SessionDefaults.MXCI_VIEW);
					sessObj.setSessionPrompt(">>");
				} else {
					if (sessObj.isCmdEchoEnabled()
							&& sessObj.getSessView() == SessionDefaults.MXCI_VIEW)
						sessObj.setCmdEcho(false);

					sessObj.setSessView(SessionDefaults.CIDEFAULT_VIEW);
					sessObj.setSessionPrompt(SessionDefaults.DEFAULT_SQL_PROMPT);
				}
			}
			envMap.put("LOOK_AND_FEEL", sessObj.getStrSessView());
			break;

		case SET_CMDECHO:

			setValue = parser.getNextKeyToken();
			if (setValue == null) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			setValue = setValue.toUpperCase();
			if ((!setValue.equals("ON") && !setValue.equals("OFF"))) {
				writeSyntaxError(this.queryStr,
						setValue + parser.getRemainderStr());
				break;
			}
			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			if (setValue.equals("ON")) {
				sessObj.setCmdEcho(true);
			} else {
				sessObj.setCmdEcho(false);
			}
			break;

		case SET_FETCHSIZE:
			if (sessObj.getMode() != SessionDefaults.SQL_MODE) {
				this.invalidCmdforMode(sessObj.getMode());
				break;
			}
			setValue = parser.getNextNumberToken();
			if (setValue == null || parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			try {
				int fetchSize = Integer.parseInt(setValue);

				if (fetchSize < 0)
					throw (new NumberFormatException());
				else {
					sessObj.setFetchSize(fetchSize);
				}
			} catch (NumberFormatException nfe) {
				sessionError('E', SessionError.SET_FETCHSIZE_VAL_ERR);
			}
			break;

		case SET_PATTERN:
			String key = null;

			if ((key = parser.getPatternKeyPattern()) != null) {
				if (envMap.containsKey(key.toUpperCase())) {
					sessionError(
							'E',
							new ErrorObject(
									SessionError.COMMAND_NAME_NOT_ALLOWED,
									"",
									key.toUpperCase()
											+ SessionError.PATTERN_COMMAND_NAME_NOT_ALLOWED_SUFFIX));
					return;
				}
				if (sessObj.isDebugOn())
					System.out.println("SET_PATTERN :: key:" + key);

				setValue = parser.getPatternValueToken();

				if (sessObj.isDebugOn())
					System.out.println("SET_PATTERN :: value:" + setValue + "");

				if (setValue == null || parser.hasMoreTokens()) {
					writeSyntaxError(this.queryStr, parser.getRemainderStr());
					break;
				}
				sessObj.setPattern(key, setValue);
				break;
			} else if ((key = parser.getRegexpPattern()) != null) {
				try {
					Pattern.compile(key);
				} catch (PatternSyntaxException pse) {
					sessionError('E', SessionError.INVALID_REGEXP);
					break;
				}
				if (sessObj.isDebugOn())
					System.out.println("SET_PATTERN :: RegExp:" + key);

				setValue = parser.getRegexpValue();

				if (sessObj.isDebugOn())
					System.out.println("SET_PATTERN :: value:" + setValue);

				if ((setValue == null) || parser.hasMoreTokens()) {
					writeSyntaxError(this.queryStr, "");
					break;
				}
				sessObj.setRegExpPattern(key, setValue);
			} else {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			break;

		case SET_PATTERNDEPTH:
			if (sessObj.getMode() != SessionDefaults.SQL_MODE) {
				this.invalidCmdforMode(sessObj.getMode());
				break;
			}
			setValue = parser.getNextNumberToken();
			if (setValue == null || parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			try {
				int maxPatternDepth = Integer.parseInt(setValue);

				if (maxPatternDepth < 0
						|| maxPatternDepth > SessionDefaults.PATTERN_DEPTH_LIMIT)
					throw (new NumberFormatException());
				else {
					sessObj.setMaxPatternDepth(maxPatternDepth);
				}
			} catch (NumberFormatException nfe) {
				sessionError('E', SessionError.SET_PATTERN_DEPTH_VAL_ERR);
			}
			break;

		case SET_DEBUG:
			setValue = parser.getNextKeyToken();
			if (setValue == null) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			setValue = setValue.toUpperCase();
			if ((!setValue.equals("ON") && !setValue.equals("OFF"))) {
				writeSyntaxError(this.queryStr,
						setValue + parser.getRemainderStr());
				break;
			}
			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			if (setValue.equals("ON")) {
				sessObj.setDebugOn(true);
			} else {
				sessObj.setDebugOn(false);
			}
			break;

		case SET_CMDDISPLAY:

			setValue = parser.getNextKeyToken();
			if (setValue == null) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				break;
			}
			setValue = setValue.toUpperCase();
			if ((!setValue.equals("ON") && !setValue.equals("OFF"))) {
				writeSyntaxError(this.queryStr,
						setValue + parser.getRemainderStr());
				break;
			}
			if (parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr,
						setValue + parser.getRemainderStr());
				break;
			}
			if (setValue.equals("ON")) {
				sessObj.setLogCmdEcho(true);
				if (sessObj.isDotModeCmd()) {
					sessObj.setDotModeCmd(false);
					sessObj.setHistOn(false);
				}
			} else {
				sessObj.setLogCmdEcho(false);
			}
			break;
		}
	}

	int getSetValue() throws IOException {
		String setValue = parser.getNextKeyToken();
		if (setValue == null) {
			return 0;
		}
		if (parser.hasMoreTokens()) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return -1;
		}
		if (setValue.equalsIgnoreCase("ON"))
			return (1);
		else if (setValue.equalsIgnoreCase("OFF"))
			return (0);
		else {
			writeSyntaxError(this.queryStr, setValue + parser.getRemainderStr());
			return -1;
		}
	}

	private void writeSyntaxError(String queryStr, String remainderStr)
			throws IOException {
		sessObj.getQuery().setStatusCode(-1); // update the error code to -1

		if (sessObj.getDisplayFormat() != SessionDefaults.XML_FORMAT)
			writer.writeln();

		writer.writeSyntaxError(sessObj, this.queryStr, remainderStr);
	}

	private void invalidCmdforMode(int mode) throws IOException {
		switch (mode) {
		case SessionDefaults.WMS_MODE:
			sessionError('E', SessionError.CMD_NOT_WMS_SUPPORTED);
			break;
		case SessionDefaults.SQL_MODE:
			sessionError('E', SessionError.CMD_NOT_SQL_SUPPORTED);
			break;
		case SessionDefaults.CS_MODE:
			sessionError('E', SessionError.CMD_NOT_CS_SUPPORTED);
			break;
		}
	}

	private void sessionError(char errType, ErrorObject errObj)
			throws IOException {
		sessObj.getQuery().setStatusCode(-1); // update the error code to -1

		if (sessObj.getDisplayFormat() != SessionDefaults.XML_FORMAT)
			writer.writeln();

		sessObj.setLastError(Integer.parseInt(errObj.errorCode()));
		writer.writeError(sessObj, errObj);
	}

	void saveFile(String fileName, boolean append) throws IOException {
		FileWriter historyWriter = new FileWriter();
		try {
			historyWriter.setAppend(append);
			historyWriter.initialize(fileName);

			List<Vector<String>> historyList = sessObj.getQryHistory();
			if (historyList != null) {
				for (int i = 0; i < historyList.size(); i++) {
					historyWriter.writeln((historyList.get(i)).get(1));
				}
			}
			historyWriter.close();
		} catch (FileNotFoundException fnfe) {
			sessionError('E', SessionError.SAVE_FILE_CREATE_ERROR);
			return;
		} catch (IOException e) {
			historyWriter = null;
			throw e;
		}
	}

	private void execGet() throws UnKnownInterfaceCommand, IOException,
			SQLException {

		// We dont have any interface commands with GET currently
		// This method has been created for future use
		throw uic;
	}

	private void handleVersion() throws IOException, SQLException {
		if (parser.hasMoreTokens()) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		} else
			displayVersionInfo(!sessObj.getPlatformObjectVersions());
	}

	private void displayVersionInfo(boolean srvrErrMsg) throws IOException {
		String platformVersion = sessObj.getSutVersion();
		String mxoSrvrVersion  = sessObj.getNdcsVersion();
                String databaseVersion = sessObj.getDatabaseVersion();
                String databaseEdition = sessObj.getDatabaseEdition();

		if (databaseVersion != null &&  databaseEdition != null) {
                    handleOutput("Database Version            : Release ", databaseVersion);
                    handleOutput("Database Edition            :", databaseEdition,false);
                } else {
                    handleOutput("Database Version            :", this.infoNotAvailable, false);
                    handleOutput("Database Edition            :", this.infoNotAvailable, false);
                }
                handleOutput("JDBC Type 4 Driver Build ID :", JDBCVproc.getVproc(), false);
                handleOutput("Command Interface Build ID  :", Vproc.getVproc(), false);
               
		writer.writeEndTags(sessObj);
	}

	private void handleConnect() throws IOException, UserInterruption {
		List<String> connectArgsList = new ArrayList<String>();

		String connArgStr = null;
		String newServer = null;
		connArgStr = parser.getConnArg("user");

		// if the user name is not specified but other args are specified..then
		// throw an error
		if (connArgStr != null) {
			connectArgsList.add("-u");
			connectArgsList.add(connArgStr);
		} else if (connArgStr == null && parser.hasMoreTokens()) {
			connectArgsList = null;
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}
		connArgStr = parser.getConnArg("pass");

		// if the password is not specified but the other args specified..then
		// throw syntax error
		if (connArgStr != null) {
			connectArgsList.add("-p");
			connectArgsList.add(connArgStr);
		}
		connArgStr = newServer = parser.getConnArg("server");

		if (connArgStr != null) {
			connectArgsList.add("-h");
			connectArgsList.add(connArgStr);
		}
		/*connArgStr = parser.getConnArg("dsn");

		if (connArgStr != null) {
			connectArgsList.add("-dsn");
			connectArgsList.add(connArgStr);
		} */

		if (parser.hasMoreTokens()) {
			connectArgsList = null;
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}

		String[] connectArgs = new String[connectArgsList.size()];
		connectArgsList.toArray(connectArgs);

		// parse the arguments and validate the user credentials
		ParseArguments paObj = new ParseArguments(sessObj.getConsoleReader(),
				sessObj.getConsoleWriter());
		SessionInterface siObj = null;
		paObj.setDefaults(SessionDefaults.dsnName, SessionDefaults.portNumber);

		paObj.serverName = sessObj.getSessionServer();
		if (sessObj.isDebugOn()) {
			System.out.println(this.getClass().getName() + "NewServer:: "
					+ newServer);
			System.out.println(this.getClass().getName() + ":: "
					+ paObj.serverName);
			System.out.println(this.getClass().getName() + "::SessionPort:: "
					+ sessObj.getSessionPort());
			System.out.println(this.getClass().getName() + "::SessionServer:: "
					+ sessObj.getSessionServer());
		}
		if (newServer != null) {
			String newServer1[] = newServer.split(":");
			if (newServer1.length > 0
					&& newServer1[0].equalsIgnoreCase(sessObj
							.getSessionServer()))
				paObj.portNumber = sessObj.getSessionPort();
			else
				paObj.portNumber = ":" + SessionDefaults.portNumber;
		} else
			paObj.portNumber = sessObj.getSessionPort();

		//paObj.dsnName = sessObj.getSessionDsn();

		if (sessObj.isDebugOn()) {
			System.out.println("Session Role::" + sessObj.getSessionRole());
			System.out.println("Temp Session Role::"
					+ sessObj.getTempSessionRole());
		}

		// String theRole=null;
		// if (sessObj.getConnObj() != null) {
		String theRole = sessObj.getTempSessionRole();
		if ((null == theRole) || (0 >= theRole.length()))
			theRole = "";
		// }
		paObj.roleName = theRole;

		// Set the retry count to 1 if connect command is being invoked
		// from OBEY or PRUN script files.
		if ((sessObj.getReader().getReadMode() == SessionDefaults.OBEY_READ_MODE)
				|| (sessObj.getCaller() == PRUNI)) {
			paObj.retryCnt = 1;
		}
		while (paObj.retryCnt > 0) {
			sessObj.setLastError(0);
			try {
				paObj.setAutoLogin(false);
				connectArgs = paObj.validateArgs(connectArgs);
				siObj = new SessionInterface(sessObj);
				sessObj.setQryStartTime();

				if (sessObj.isDebugOn())
					System.out.println("CONNECT: " + paObj.userName + "|"
							+ paObj.roleName + "|" + paObj.serverName + "|"
							+ paObj.dsnName + "|" + paObj.portNumber);
				// Get a new connection with the args specified, if success then
				// close the previous connection
				newConn = siObj.getConnection(paObj.userName, paObj.password,
						paObj.roleName, paObj.serverName, paObj.dsnName,
						paObj.portNumber);

				if (newConn != null) {
					try {
						if (sessObj.getConnObj() != null) {
							sessObj.getConnObj().getWarnings();
							sessObj.getConnObj().close();
							sessObj.setDBConnExists(false);
						} else {
							startTimerTask(newConn);
						}
					} catch (SQLException sqle) {
					}

					sessObj.setConnObj(newConn);
					sessObj.setSessionUser(paObj.userName);
					sessObj.setSessionPass(paObj.password);
					sessObj.setSessionSever(paObj.serverName);
					sessObj.setSessionPort(paObj.portNumber);
					sessObj.setSessionValues();
					sessObj.setStmtObj(siObj.getStatement(newConn));
					sessObj.qsOpen = false;
					sessObj.setDBConnExists(true);
                                        boolean trafver = sessObj.getPlatformObjectVersions();

					// Uncomment this line if you need to reset role to DEFAULT;
					// TempSessionRole is set using SET CONNECTIOPT command
					// sessObj.setTempSessionRole("");
					//writer.writeln();

			                writer.writeln("Connected to " + sessObj.getDatabaseEdition());

					sessObj.setQryEndTime();
					if (printConnTime) {
						printElapsedTime();
						writer.writeEndTags(sessObj);
					}

				}

				paObj.retryCnt = 0;

			} // end try
			catch (Exception e) {
				paObj.retryCnt--;
				connectArgs = handleConnException(e, paObj, connectArgs);
			}

			finally {
				newConn = null;
			}
		} // end while

	}

	private void handleReconnect() throws IOException, UserInterruption {
		boolean prevConn = false;
		if (parser.hasMoreTokens()) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}

		// if a connection exists already, close it before creating new one.
		if (sessObj.getConnObj() != null) {
			prevConn = true;
			try {
				sessObj.getConnObj().getWarnings();
				sessObj.getConnObj().close();
				// sessObj.setConnObj(null);
			} catch (SQLException sqle) {

			}
		}
		sessObj.setDBConnExists(false);

		List<String> reconnectArgsList = new ArrayList<String>();

		if (sessObj.getSessionUser() != null) {
			reconnectArgsList.add("-u");
			reconnectArgsList.add(sessObj.getSessionUser());
		}
		if (sessObj.getSessionPass() != null) {
			reconnectArgsList.add("-p");
			reconnectArgsList.add(sessObj.getSessionPass());
		}
		if (sessObj.getSessionServer() != null) {
			reconnectArgsList.add("-h");
			reconnectArgsList.add(sessObj.getSessionServer()
					+ sessObj.getSessionPort());
		}
		/*if (sessObj.getSessionDsn() != null) {
			reconnectArgsList.add("-dsn");
			reconnectArgsList.add(sessObj.getSessionDsn());
		}*/

		if (sessObj.getSessionRole() != null) {
			reconnectArgsList.add("-r");
			reconnectArgsList.add(sessObj.getSessionRole());
		}

		String[] reconnectArgs = new String[reconnectArgsList.size()];
		reconnectArgsList.toArray(reconnectArgs);
		reconnectArgsList = null;

		// parse the arguments and validate the user credentials
		ParseArguments rcPaObj = new ParseArguments(sessObj.getConsoleReader(),
				sessObj.getConsoleWriter());
		SessionInterface rcSIObj = null;

		rcPaObj.setDefaults(SessionDefaults.dsnName, SessionDefaults.portNumber);

		// Set the retry count to 1 if connect command is being invoked
		// from OBEY or PRUN script files.
		if ((sessObj.getReader().getReadMode() == SessionDefaults.OBEY_READ_MODE)
				|| (sessObj.getCaller() == PRUNI)) {
			rcPaObj.retryCnt = 1;
		}
		while (rcPaObj.retryCnt > 0) {
			sessObj.setLastError(0);
			try {
				rcPaObj.setAutoLogin(false);
				reconnectArgs = rcPaObj.validateArgs(reconnectArgs);
				sessObj.setQryStartTime();

				sessObj.setSessionUser(rcPaObj.userName);
				sessObj.setSessionPass(rcPaObj.password);
				sessObj.setSessionSever(rcPaObj.serverName);
				sessObj.setSessionPort(rcPaObj.portNumber);
				//sessObj.setSessionDsn(rcPaObj.dsnName);
				// Session role will be null only in noconnect mode
				if (sessObj.getSessionRole() == null)
					sessObj.setSessionRole(rcPaObj.roleName);

				rcSIObj = new SessionInterface(sessObj);

				if (sessObj.isDebugOn())
					System.out.println("RECONNECT: " + rcPaObj.userName + "|"
							+ rcPaObj.roleName + "|" + rcPaObj.serverName + "|"
							+ rcPaObj.dsnName + "|" + rcPaObj.portNumber);

				sessObj.setConnObj(rcSIObj.getConnection());
				sessObj.setSessionValues();
				if (!sessObj.isDBConnExists())
					startTimerTask(sessObj.getConnObj());
				sessObj.setStmtObj(rcSIObj.getStatement(sessObj.getConnObj()));
				sessObj.qsOpen = false;
				sessObj.setDBConnExists(true);
                                boolean trafver = sessObj.getPlatformObjectVersions();
				writer.writeln();
				writer.writeln("Connected to " + sessObj.getDatabaseEdition());
				rcPaObj.retryCnt = 0;

				sessObj.setQryEndTime();

			} catch (Exception ex) {
				rcPaObj.retryCnt--;
				reconnectArgs = handleConnException(ex, rcPaObj, reconnectArgs);
			}

		} // end while
		if (printConnTime) {
			printElapsedTime();
			writer.writeEndTags(sessObj);
		}

	}

	private String[] handleConnException(Exception e, ParseArguments pa,
			String[] args) throws IOException, UserInterruption {
		sessObj.getQuery().setStatusCode(-1); // update the error code to -1
		sessObj.setLastError(-1);
		if (e instanceof InvalidNumberOfArguments) {
			writer.writeln(e.toString());
		} else if (e instanceof IOException) {
			if (!(e instanceof UnknownHostException))
				writer.writeln(e.toString());

		} else if (e instanceof InstantiationException) {
			writer.writeln(e.toString());
		} else if (e instanceof IllegalAccessException) {
			writer.writeln(e.toString());
		} else if (e instanceof ClassNotFoundException) {
			writer.writeln(e.toString());
		} else if (e instanceof UserInterruption) {
			throw ((UserInterruption) e);
		} else if (e instanceof SQLException) {
			writer.writeln();
			sessObj.setLastError(((SQLException) e).getErrorCode());
			// catch the known errors report them with an user friendly text
			if (((SQLException) e).getErrorCode() == SessionDefaults.SQL_ERR_CONN_MAX_LIMIT) {
				System.out.println(SessionError.CONN_MAX_LIMIT_ERR);
				if (doTrace)
					e.printStackTrace();
				System.exit(SessionDefaults.abruptExit);
			} else {
				String errStr = e.toString();

				if (errStr.indexOf("org.trafodion.jdbc.t4") != -1)
					errStr = errStr.substring(errStr.indexOf(":") + 1);

				writer.write(errStr + SessionDefaults.lineSeperator);
				// identify those args that caused the exception

				if ((((SQLException) e).getErrorCode() == SessionDefaults.SQL_ERR_INVALID_AUTH)
						|| (((SQLException) e).getErrorCode() == SessionDefaults.SQL_ERR_CLI_AUTH)) {
					pa.userName = null;
					pa.password = null;
					pa.roleName = null;

				} else
					pa.serverName = null;

				// rebuild args list and remove only those that caused the
				// exception
				args = pa.rebuildArgList(args);

			}
		} // end SQLException

		return args;
	}

	private void handleDisConnect() throws IOException {
		boolean exitFlag = false;
		String errorCodeReturn = "";
		int exitCode = SessionDefaults.DEFAULT_EXIT_CODE;
		String strPendingAction = null;

		if (parser.hasMoreTokens()) {

			Matcher exitMat = parser.exitPat.matcher(parser.getRemainderStr());

			if (exitMat.find()) {

				errorCodeReturn = exitMat.group(3);
				if (errorCodeReturn == null)
					errorCodeReturn = exitMat.group(4);

				errorCodeReturn = (errorCodeReturn == null || errorCodeReturn
						.equals("")) ? SessionDefaults.DEFAULT_EXIT_CODE + ""
						: errorCodeReturn;

				try {
					exitCode = Integer.parseInt(errorCodeReturn.trim());
				} catch (NumberFormatException e) {
					return;
				}

				String strCondition = exitMat.group(5);

				if (strCondition != null) {
					String strConditional = strCondition.trim() + " THEN EXIT"
							+ sessObj.getSessionSQLTerminator();
					ConditionalQuery cqObj = sessObj.getCQryObj();

					sessObj.getQuery().setQueryId(IF_THEN);
					sessObj.getQuery().resetQueryText(strConditional);

					try {
						cqObj.execute();
						strPendingAction = cqObj.getPendingAction();
						if (strPendingAction != null
								&& strPendingAction.equals("EXIT"
										+ sessObj.getSessionSQLTerminator())) {
							exitFlag = true;
							sessObj.getQuery().setQueryId(EXIT);
						}

					} catch (ConditionalQueryException ex) {
						if (ex.getErrorMsg() != null)
							writer.writeInterfaceErrors(sessObj,
									ex.getErrorMsg());
						else
							writer.writeConditionalSyntaxError(sessObj,
									cqObj.getQueryString());

						return;
					} catch (UserInterruption ui) {
						if (sessObj.isQueryInterrupted()) {
							sessObj.setQueryInterrupted(false);
							writer.writeln();
							writer.writeInterfaceErrors(sessObj,
									SessionError.OPERATION_CANCELLED);
						}
						return;
					} catch (IOException e) {
						System.out
								.println("IO Exception occurred while processing conditional query :"
										+ e);
					}

				} else {
					exitFlag = true;
				}
			} else
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
		} else {
			exitFlag = true;
		}

		if (sessObj.getQuery().getQueryId() == DISCONNECT && exitFlag) {
			if (sessObj.getConnObj() != null) {
				try {
					// Added if statement  2010-03-25
					// Move the if statement from handleModeCommand to here
					// For Mode commmand can run on non-connection status.
					if (sessObj.getMode() == SessionDefaults.WMS_MODE) {
						sessObj.getStmtObj().execute("WMSCLOSE");
						sessObj.qsOpen = false;
					}

					sessObj.getConnObj().getWarnings();
					sessObj.getConnObj().close();
				} catch (SQLException sqle) {

				}
			}
			sessObj.setMxosrvrVersion(null);
			sessObj.setSutVersion(null);
			// sessObj.setConnObj(null);
			sessionError('E', SessionError.SESSION_DISCONNECT_ERR);
			sessObj.setDBConnExists(false);
			return;
		}

		if (sessObj.getQuery().getQueryId() == EXIT && exitFlag) {
			// Need to close the connection so that MXCS can do
			// the cleanup of resources
			try {
				if (conn != null)
					conn.close();
			} catch (SQLException sqlEx) {
				// Ignore the exception
			}
			sessObj.setPatternsLoaded(false);
			sessObj.setSessionStatus(false);
			sessObj.setExitCode(exitCode);
		}
	}

	private void handleHostCommand() throws IOException {
		if (sessObj.getCaller() != PRUNI)
			try {
				Signal.handle(INTSignal, LHCTRLCHandler);
			} catch (Exception e) {
			}

		if (!parser.hasMoreTokens()) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}

		TerminalProcessBridge procStdOut = null;
		TerminalProcessBridge procStdErr = null;
		TerminalProcessBridge procStdIn = null;

		try {
//			int idx;
			String osName = System.getProperty("os.name");
			StringBuffer osCommand = new StringBuffer();

			if ((null != osName)
					&& osName.trim().toUpperCase().startsWith("WINDOW")) {
				osCommand.append("cmd /c");

				if (osName.trim().toUpperCase().equals("WINDOWS 95")) {
					osCommand.setLength(0);
					osCommand.append("command /c");
				}
			}
			osCommand.append(parser.getRemainderStr());
			if (osCommand.lastIndexOf(" cd ") >= 0)
				System.out.println("cd is not supported by localhost command");
			else {
				if (TerminalProcessBridge._debugOn)
					System.out.println("** Debug*** ==>  running command : "
							+ osCommand.toString());
				Process osProcess;
				if ((null != osName)
						&& osName.trim().toUpperCase().startsWith("WINDOW"))
					osProcess = Runtime.getRuntime().exec(osCommand.toString());
				else
					osProcess = Runtime.getRuntime().exec(
							new String[] { "sh", "-c", osCommand.toString() });

				procStdOut = new TerminalProcessBridge(
						osProcess.getInputStream(), writer);
				procStdOut.setTag("STDOUT");
				procStdErr = new TerminalProcessBridge(
						osProcess.getErrorStream(), System.err);
				procStdErr.setTag("STDERR");
				procStdIn = new TerminalProcessBridge(System.in,
						osProcess.getOutputStream(), false);
				procStdIn.setTag("STDIN");

				// Start the threads.
				procStdOut.start();
				procStdErr.start();
				procStdIn.start();

				// check for errors.
				int exitVal = osProcess.waitFor();
				if (TerminalProcessBridge._debugOn)
					System.out
							.println("** Debug*** ==>  ExitValue: " + exitVal);
			}
		} catch (Throwable t) {
			String msg = t.getMessage();
			writer.writeln(msg.substring(msg.indexOf(":") + 1));

		} finally {
			if (null != procStdIn)
				procStdIn.stopReading();

			try {
//				Thread.currentThread().yield();
				Thread.yield();
			} catch (Exception e) {
			}
		}
		sessObj.setQryEndTime();
		printElapsedTime();
		writer.writeEndTags(sessObj);
	}

	private void handleRepeatCommand(String cmdStr, int cmdNo)
			throws IOException, SQLException, UserInterruption,
			UnKnownInterfaceCommand {
		// If no commands present in history return
		List<Vector<String>> historyList = sessObj.getQryHistory();

		if (sessObj.getQryHistory() == null) {
			sessionError('E', SessionError.REPEAT_ERR);
			sessObj.setQuery(null);
			return;
		}
		int historySize = sessObj.getQryHistory().size();
		// If no tokens present, return the previous command
		if (!parser.hasMoreTokens()) {
			cmdNo = historySize;
		}

		// FC could be followed by text or a command number
		// first check for the number - this could be a negative
		// integer too.
		else {
			try {
				cmdStr = parser.getNextNumberToken();

				if ((cmdStr != null) && !(cmdStr.equals(""))) {
					if (parser.hasMoreTokens()) {
						// throw a syntax error
						writeSyntaxError(this.queryStr,
								parser.getRemainderStr());
						return;
					}
					cmdNo = Integer.parseInt(cmdStr);
					if (cmdNo <= 0)
						cmdNo = historySize + cmdNo;

					if ((cmdNo >= 1) && (cmdNo <= historySize)) {
						// sessObj.getQuery().resetQueryText(sessObj.getQryHistory().get(cmdNo-1).toString());
					} else {
						sessionError('E', SessionError.REPEAT_ERR);
						sessObj.setQuery(null);
						return;
					}
				} else // text is passed
				{
					cmdStr = parser.getNextKeyToken().toUpperCase();
					if (cmdStr != null) {
						// search the history buffer for a matching
						// command
						historyList = sessObj.getQryHistory();
						if (historyList != null) {
							int i;
							for (i = historyList.size() - 1; i >= 0; i--) {
								if ((historyList.get(i))
										.get(1)
										.toUpperCase()
										.replaceAll("\\s+", " ")
										.trim()
										.startsWith(
												(cmdStr + parser
														.getRemainderStr()
														.toUpperCase())
														.replaceAll("\\s+", " "))) {
									cmdNo = i + 1;
									break;
								}
							}
							if (i < 0) {
								sessionError('E', SessionError.REPEAT_ERR);
								sessObj.setQuery(null);
								return;
							}

						}
					}
				}
			} catch (NumberFormatException nfe) {
				sessionError('E', SessionError.REPEAT_ERR);
				sessObj.setQuery(null);
				return;
			}
		}

		String queryText = (historyList.get(cmdNo - 1)).get(1);
		String termEvent = (historyList.get(cmdNo - 1)).get(2);
		String sqlTerminator = sessObj.getTermEventMap().get(termEvent);

		// if query ends with sqlterminator then trim and append new
		// term else retain the same

		if (queryText.toUpperCase().endsWith(sqlTerminator)) {
			queryText = queryText.substring(0, queryText.length()
					- sqlTerminator.length());
			queryText = queryText + sessObj.getSessionSQLTerminator();
		}
		sessObj.getQuery().resetQueryText(queryText);
		if (sessObj.getQuery().getQueryId() == FC) {
			FCQuery fcQryObj = new FCQuery(sessObj, sessObj.getQuery());
			boolean execute = fcQryObj.editCommand();
			// If user has aborted the fc session do not
			// execute the command, return to the prompt
			if (!execute) {
				sessObj.setQuery(null);
				return;
			}
		}
		try {
			if (sessObj.getQuery().getQueryId() == REPEAT) {
				writer.writeln(sessObj.getQuery().getQueryText());
			}

			sessObj.getVQryObj().validate(sessObj.getQuery(),
					sessObj.getSessionSQLTerminator());

			if (sessObj.getQuery().getQueryType() == SessionDefaults.IQ) {
				this.execute();
				return;
			} else {
				throw uic;
			}
		} catch (NullKeyWordException nke) {
			// if blank lines entered ..its fine..move on
			return;
		}
	}

	private void handleClearCommand() throws IOException {
		if (parser.hasMoreTokens()) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}

		String clsCmdString = "clear";

		// if it is windows envrionment use the dll to clear the screen
		if (System.getProperty("os.name").toUpperCase().startsWith("WINDOW")) {
			try {
				WCIUtils wcs = new WCIUtils();
				wcs.cls();
				wcs = null;
				return;
			} catch (Throwable t) {
				if (sessObj.isDebugOn())
					System.out.println(this.getClass().getName()
							+ ": Clear command failed. Details = "
							+ t.getMessage());
			}
			clsCmdString = "cmd /c cls";
		}
		/**
		 * Only if wcs.cls failed, do we get down here and try to run a
		 * localhost command (clear).
		 */
		try {
			HostQuery hq = new HostQuery();
			hq.startInputThread = false;
			hq.execute(clsCmdString, sessObj.getReader(), sessObj.getWriter());
			hq = null;

		} catch (Exception e) {
		}
	}

	private void handleRunCommand() throws IOException, SQLException,
			UnKnownInterfaceCommand, UserInterruption {
		String sqlTerminator = null;
		if (parser.hasMoreTokens()) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}
		if (sessObj.getPrevSQLQuery() == null) {
			sessionError('E', SessionError.SLASH_ERR);
			return;
		}
		if (!sessObj.isDBConnExists()) // Added  2010-03-19
		{
			displayNoConnInfo();
			return;
		}
		if (isCreateTriggerCmd(sessObj.getPrevSQLQuery()))
			sqlTerminator = "/";
		else
			sqlTerminator = sessObj.getSessionSQLTerminator();

		sessObj.getQuery().resetQueryText(
				sessObj.getPrevSQLQuery() + sqlTerminator);

		try {
			sessObj.getVQryObj().validate(sessObj.getQuery(),
					sessObj.getSessionSQLTerminator());
			if (sessObj.getQuery().getQueryType() == SessionDefaults.IQ) {
				this.execute();
				return;
			} else {
				throw uic;
			}
		} catch (NullKeyWordException nke) {
			// if blank lines entered ..its fine..move on
			return;
		}
	}

	private void handleHelp() throws IOException {
		String command = "";
		String tmpStr = null;
		Helper helpObj = new Helper();
		String section = null;
		while ((tmpStr = parser.getNextKeyToken()) != null) {
			command += tmpStr + " ";
		}

		if (parser.hasMoreTokens()) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			helpObj = null;
			return;
		}
		helpObj.printHelpText(sessObj.getWriter(), sessObj.getStrMode(),
				sessObj.getSessionRole(), command, section);
		helpObj = null;
	}

	private void handleResetCommand() throws IOException,
			UnKnownInterfaceCommand {
		String paramName = parser.getNextKeyToken();
		HashMap<String, String> params = null;

		if (paramName != null) {
			if ((paramName.equalsIgnoreCase("LASTERROR") || paramName
					.equalsIgnoreCase("ERRORCODE")) && !parser.hasMoreTokens()) {
				sessObj.setLastError(0);
			} else if (paramName.equalsIgnoreCase("PARAM")) {
				if (!parser.hasMoreTokens()) {
					sessObj.resetSessionParams(params);
				} else if ((paramName = parser.getNextParamToken()) != null
						&& !parser.hasMoreTokens()) {
					sessObj.resetSessionParams(paramName.trim());
				} else {
					writeSyntaxError(this.queryStr, parser.getRemainderStr());
				}
			} else {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
			}
			return;
		} else {
			throw uic;
		}

	}

	private void handleHistory() throws IOException {
		String historyValue = parser.getNextNumberToken();
		if ((historyValue == null && parser.hasMoreTokens())
				|| (historyValue != null && historyValue.startsWith("-"))) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}

		if (parser.hasMoreTokens()) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}

		// if the history number is not passed, then show the last 10 commands
		//
		if (historyValue == null || historyValue.trim().equals("")) {
			if (sessObj.getQryHistory() != null) {
				if (sessObj.getQryHistory().size() > 10) {
					historyValue = "" + 10;
				} else {
					historyValue = "" + sessObj.getQryHistory().size();
				}
			} else {
				historyValue = 0 + "";
			}
		}

		int historyVal = 0;
		if (historyValue != null) {
			try {
				historyVal = Integer.parseInt(historyValue);
			}

			catch (NumberFormatException nfe) {
				// sessObj.getQuery().resetQueryText("");
				// writer.writeln(SessionError.HISTORY_ERR);
				historyVal = Integer.MAX_VALUE;
			}
		}

		if (historyVal > SessionDefaults.QRY_HISTORY_MAX_LMT) {
			sessionError('W', SessionError.HISTORY_ERR);
			writer.writeln();
		}
		if (sessObj.getQryHistory() == null) {
			sessionError('I', SessionError.HISTORY_BUF_ERR);
			return;
		}
		int j = sessObj.getQryHistory().size() - historyVal;
		if (j <= 0) {
			j = 0;
		}
		j++;
		if (sessObj.getQryHistory().size() < SessionDefaults.QRY_HISTORY_MAX_LMT) {
			for (int i = 0; (i < sessObj.getQryHistory().size() && i < historyVal); i++) {
				writer.writeln(j
						+ ">"
						+ (sessObj.getQryHistory().get(j - 1)).get(1)
								.replaceAll("(?m)^", "\t"));
				j++;
			}
		} else {
			for (int i = 0; (i < sessObj.getQryHistory().size() - 1 && i < historyVal - 1); i++) {
				writer.writeln(j
						+ ">"
						+ (sessObj.getQryHistory().get(j)).get(1).replaceAll(
								"(?m)^", "\t"));
				j++;
			}
			writer.writeln(j + ">\t" + sessObj.getQuery().getQueryText());
		}
	}

	private void handleSaveHistory() throws IOException {
		boolean append = true;
		String saveFileName = parser.getNextFileToken();
		if (saveFileName == null || saveFileName.trim().equals("")) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}
		if (parser.hasMoreTokens()) {
			String saveOption = parser.getNextKeyToken();

			if (parser.hasMoreTokens()
					|| (saveOption != null && !saveOption
							.equalsIgnoreCase("CLEAR"))) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				return;
			}
			append = false;
		}
		saveFile(saveFileName, append);
	}

	private void handleProcedureException(SQLException se) throws IOException,
			SQLException {
		if (Math.abs(se.getErrorCode()) == 8905) {
			sessionError('E', SessionError.EXECUTE_PRIVILEGE_ERR);
		} else
			throw se;
	}


	/**
	 * Helper method for handleShowIndexes() method to build the index table for
	 * Table and MVS
	 *
	 * @param type
	 *            - Table or MV
	 * @param indexTable
	 *            - holds the index name and name of table or MV
	 * @return Hashtable holds indexname and table/MV
	 * @throws SQLException
	 * @throws IOException
	 */
	private Vector<IndexStruct> buildIndexStructList(String[] ttype,
			Vector<IndexStruct> indexStructList) throws SQLException,
			IOException {
		DatabaseMetaData dbmd = conn.getMetaData();
		String sessionCat = sessObj.getSessionCtlg();
		String sessionSchema = sessObj.getSessionSchema();
		ResultSet rsmd = dbmd.getTables(sessionCat, sessionSchema, null, ttype);

		final String keyword = SqlEnum.INDEX.toString();
		while (rsmd.next()) {
			String tableName = rsmd.getString(SqlEnum.TABLE_NAME.toString());
			// Regular Identifier: a-z, A-Z, 0-9, underscore
			if (!tableName.matches("[a-zA-Z_0-9]*"))
				tableName = "\"" + tableName + "\"";// surround with QUOTEs

			// get the index info and build the index list
			ResultSet indexResultSet = conn.prepareStatement(
					"showddl " + sessionCat + "." + sessionSchema + "."
							+ tableName).executeQuery();

			while (indexResultSet.next()) {
				String indexname = "";
				indexname = this.extractNameFromDdl(keyword, indexResultSet);
				if (indexname != null && indexname.length() > 0)
					indexStructList.add(new IndexStruct(indexname, tableName,
							ttype[0].trim()));
			}
		}
		return indexStructList;
	}

	/**
	 * Show indexes at schema level.
	 *
	 * @throws IOException
	 */
	private void handleShowIndexes(final boolean orderByIndex)
			throws SQLException, IOException {
		final String[] TTYPE = { SqlEnum.TABLE.toString() };
		final String[] MVTYPE = { SqlEnum.MV.toString() };
		final int headingLength = 80;
		final int columnWidth = 30;
		/**
		 * Holds the base table name, MVS and list of indexes for that table
		 */
		Vector<IndexStruct> indexStructList = new Vector<IndexStruct>();
		// getIndexInfo doesn't work on Seaquest platform

		// For Table
		indexStructList = this.buildIndexStructList(TTYPE, indexStructList);
		// For Materialized View
		indexStructList = this.buildIndexStructList(MVTYPE, indexStructList);

		if (indexStructList.isEmpty()) {
			formatResults(SessionError.SHOW_TAB_IDX_NOT_FOUND
					+ sessObj.getSessionSchema());
		} else { // Sort indexes base on index name or table name
			int j; // insertion sort
			for (int i = 1; i < indexStructList.size(); i++) {
				j = i;
				IndexStruct newStruct = indexStructList.get(i);
				if (orderByIndex) { // default
					while (j > 0
							&& indexStructList.get(j - 1).getIndexName()
									.compareTo(newStruct.getIndexName()) > 0) {
						indexStructList.set(j, indexStructList.get(j - 1));
						j--;
					}
				} else { // order by table
					while (j > 0
							&& indexStructList.get(j - 1).getTableName()
									.compareTo(newStruct.getTableName()) > 0) {
						indexStructList.set(j, indexStructList.get(j - 1));
						j--;
					}
				}
				indexStructList.set(j, newStruct);
			}
			// Display Indexes
			final int SESSION_DISPLAY_FORMAT = sessObj.getDisplayFormat();
			switch (SESSION_DISPLAY_FORMAT) {
			case SessionDefaults.XML_FORMAT:
				sessObj.getXmlObj().init();
				sessObj.getXmlObj().handleStartTags();
				writer.writeln(" <Indexes>");
				break;
			case SessionDefaults.HTML_FORMAT:
				sessObj.getHtmlObj().init();
				sessObj.getHtmlObj().handleStartTags();

				writer.writeln("<tr>");
				writer.writeln(" INDEXES");
				writer.writeln("</tr>");
				writer.writeln("<tr>");
				writer.writeln("  <th>" + "INDEX NAME" + "</th>");
				writer.writeln("  <th>" + "BASE TABLE NAME" + "</th>");
				writer.writeln("  <th>" + "BASE TABLE TYPE" + "</th>");
				writer.writeln("</tr>");
				break;
			default:
				writer.writeln();
				writer.write(utils.formatString("INDEX NAME", columnWidth, ' ',
						" "));
				writer.write(utils.formatString("BASE TABLE NAME", columnWidth,
						' ', " "));
				writer.writeln(utils.formatString("BASE TABLE TYPE",
						columnWidth, ' ', " "));
				writer.writeln(utils.formatString("-", headingLength, '-', " "));
				break;
			}
			for (IndexStruct aIndexStruct : indexStructList) {
				String indexName = aIndexStruct.getIndexName();
				String tableName = aIndexStruct.getTableName();
				String tableType = aIndexStruct.getTableType();
				switch (SESSION_DISPLAY_FORMAT) {
				case SessionDefaults.XML_FORMAT:
					writer.writeln("  <index name=\""
							+ utils.formatXMLdata(indexName) + "\">");
					writer.writeln("  <table name=\""
							+ utils.formatXMLdata(tableName) + "\">");
					writer.writeln("  <table type=\""
							+ utils.formatXMLdata(tableType) + "\">");
					break;
				case SessionDefaults.HTML_FORMAT:
					writer.writeln(" <tr>");
					writer.writeln("   <td>" + indexName + "</td>");
					writer.writeln("   <td>" + tableName + "</td>");
					writer.writeln("   <td>" + tableType + "</td>");
					writer.writeln(" </tr>");

					break;
				default:
					writer.writeln();
					writer.write(utils.formatString(indexName, columnWidth,
							' ', " "));
					writer.write(utils.formatString(tableName, columnWidth,
							' ', " "));
					writer.writeln(utils.formatString(tableType, columnWidth,
							' ', " "));
					break;
				}
			}
			switch (SESSION_DISPLAY_FORMAT) {
			case SessionDefaults.XML_FORMAT:
				writer.writeln("</Indexes>");
				break;
			}
		}
	}

	/**
	 * Method to extract name from showdll
	 *
	 * @param keyword
	 * @param resultSet
	 * @return
	 * @throws SQLException
	 */
	private String extractNameFromDdl(String keyword, ResultSet resultSet)
			throws SQLException {
		final String QUOTE = "\"";
		// parse synonym/mvs/view/trigger/index name from showddl DESCRIBE__COL
		String ddl = resultSet.getString(SqlEnum.DESCRIBE__COL.toString());
		if (keyword != null && keyword.length() > 0) {
			if (keyword.equalsIgnoreCase(SqlEnum.TRIGGERS.toString()))
				keyword = SqlEnum.TRIGGER.toString();
			else if (keyword.equalsIgnoreCase(SqlEnum.INDEXES.toString()))
				keyword = SqlEnum.INDEX.toString();
			else if (keyword.equalsIgnoreCase(SqlEnum.SYNONYMS.toString()))
				keyword = SqlEnum.SYNONYM.toString();
		}
		if (ddl.indexOf(keyword) > -1) {
			String[] splitArray = ddl.split(" ");
			for (int i = 0; i < splitArray.length; i++) {
				// extract trigger/index name
				if (splitArray[i].equalsIgnoreCase(keyword)) {
					String name = splitArray[i + 1];
					if (name.startsWith(QUOTE) && !name.endsWith(QUOTE)) { // delimited
																			// name
						for (int j = ddl.indexOf(name) + name.length(); j < ddl
								.length(); j++) {
							name += ddl.charAt(j);
							// check if ends with END-QUOTE not inner quote
							if (name.endsWith(QUOTE)
									&& name.charAt(name.length() - 2) != '\\')
								break;
						}
					}
					return name;
				}
			}
		}
		return "";
	}

	private void handleShowTableOptions(int howManyOptions,
			String keyWordToken, String prepTableName, String[] tmpOptions,
			String ctlgName, String schemaName, String tableName,
			int headingLength) throws IOException, SQLException {
//		int idxColCount = 0;
		for (int options = 0; options < howManyOptions; options++) {
			keyWordToken = tmpOptions[options];
			if (keyWordToken == null
					|| !keyWordToken.equalsIgnoreCase(SqlEnum.INDEXES
							.toString())
					&& !keyWordToken.equalsIgnoreCase(SqlEnum.SYNONYMS
							.toString())
					&& !keyWordToken.equalsIgnoreCase(SqlEnum.MVS.toString())
					&& !keyWordToken.equalsIgnoreCase(SqlEnum.TRIGGERS
							.toString()) || parser.hasMoreTokens()) {
				writeSyntaxError(this.queryStr, parser.getRemainderStr());
				return;
			}
			conn.prepareStatement("select 1 from " + prepTableName);
			DatabaseMetaData tbdbmd = conn.getMetaData();
			/**
			 * Bug 1906- some modules of JDBC driver doesn't work on SEA
			 * Workaround- use showddl to get index, synonym, trigger
			 */

			// mvs works with getMaterializedViewInfo
			ResultSet tbrsmd = null;
			if (keyWordToken.equalsIgnoreCase(SqlEnum.MVS.toString()))
				tbrsmd = ((org.trafodion.jdbc.t4.T4DatabaseMetaData) tbdbmd)
						.getMaterializedViewInfo(ctlgName, schemaName,
								tableName);
			else
				tbrsmd = conn.prepareStatement(
						"showddl "
								+ SqlEnum.getFullQualifiedName(ctlgName,
										schemaName, tableName)).executeQuery();


			// for triggers, mvs & synonyms print all the columns retrieved from
			// the database

			// contruct an array list with a list all column values first
			// before
			// displaying them as we need to calculate the display size from
			// the
			// actual value. The display size cant be calculated from
			// Resultsetmetadata since
			// the values are returned from system(dictionary) tables.
			List<String> result = new ArrayList<String>();

			// Enter this loop only when the result set is not empty
			// otherwise display info message that object nt found
			if (tbrsmd.next()) {
				int colCount = 1;
				do {
					String objectName = "";

					// keyword: triggers/synonyms/indexes/materialized view
					if (keyWordToken.equalsIgnoreCase(SqlEnum.MVS.toString()))
						objectName = tbrsmd.getString("SCHEMA_NAME")
								+ "."
								+ tbrsmd.getString(SqlEnum.MATERIALIZED_NAME
										.toString());
					else
						objectName = this.extractNameFromDdl(keyWordToken,
								tbrsmd);
					if (objectName.length() > 0)
						result.add(objectName);
				} while (tbrsmd.next());

				String titleStr = "";
				if (result.size() > 0) {
					if (keyWordToken.equalsIgnoreCase(SqlEnum.SYNONYMS
							.toString()))
						titleStr = howManyOptions > 1 ? SqlEnum.SYNONYMS
								.toString() : SqlEnum.SYNONYM_NAME.getLabel();
					else if (keyWordToken.equalsIgnoreCase(SqlEnum.TRIGGERS
							.toString()))
						titleStr = howManyOptions > 1 ? SqlEnum.TRIGGERS
								.toString() : SqlEnum.TRIGGER_NAME.getLabel();
					else if (keyWordToken.equalsIgnoreCase(SqlEnum.INDEXES
							.toString()))
						titleStr = howManyOptions > 1 ? SqlEnum.INDEXES
								.toString() : SqlEnum.INDEX_NAME.getLabel();
					else
						titleStr = howManyOptions > 1 ? "MATERIALIZED VIEWS"
								: SqlEnum.MATERIALIZED_NAME.getLabel();
					result.add(0, titleStr);
				} else {
					if (keyWordToken.equalsIgnoreCase("MVS"))
						formatResults(SessionError.SHOW_TAB_MVS_NOT_FOUND
								+ schemaName + "." + tableName);
					else if (keyWordToken.equalsIgnoreCase(SqlEnum.TRIGGERS
							.toString()))
						formatResults(SessionError.SHOW_TAB_TRIG_NOT_FOUND
								+ schemaName + "." + tableName);
					else if (keyWordToken.equalsIgnoreCase(SqlEnum.INDEXES
							.toString()))
						formatResults(SessionError.SHOW_TAB_IDX_NOT_FOUND
								+ schemaName + "." + tableName);
					else
						formatResults(SessionError.SHOW_TAB_SYN_NOT_FOUND
								+ schemaName + "." + tableName);

				}

				// calculate the columnwise maximum display size by looping
				// through each column
				// values. By default the max column size is set to 97, same
				// value length of show table <tabname>,INDEXES.
				int[] maxLength = new int[colCount];
				int currRecLen = 0;

				for (int cc = 0; cc < colCount; cc++) {
					maxLength[cc] = howManyOptions > 1 ? headingLength : 0;
					for (int i = cc; i < result.size();) {
						currRecLen = result.get(i) == null ? 4 : result.get(i)
								.toString().length();
						if (maxLength[cc] < currRecLen) {
							maxLength[cc] = currRecLen;
						}
						i = i + colCount;
					}
				}

				// display the result set from the locally build array list
				// for the display size, use the columnwise display size
				// calculated above.
				int displayPos = 0;
				for (int k = 0; k < result.size(); k++) {
					switch (sessObj.getDisplayFormat()) {
					case SessionDefaults.XML_FORMAT:
						sessObj.getXmlObj().init();
						sessObj.getXmlObj().handleStartTags();
						if (keyWordToken.equalsIgnoreCase(SqlEnum.SYNONYMS
								.toString()) && k == 0) {
							writer.writeln("  <" + utils.removeSpaces(titleStr)
									+ ">");
						} else if (keyWordToken.equalsIgnoreCase(SqlEnum.MVS
								.toString()) && k == 0)
							writer.writeln("  <" + utils.removeSpaces(titleStr)
									+ ">");
						if (k > 0)
							writer.writeln("  <Name>"
									+ utils.formatXMLdata(String.valueOf(result
											.get(k))) + "</Name>");
						break;

					case SessionDefaults.HTML_FORMAT:
						sessObj.getHtmlObj().init();
						sessObj.getHtmlObj().handleStartTags();
						if (keyWordToken.equalsIgnoreCase(SqlEnum.SYNONYMS
								.toString()) && k == 0) {
							writer.writeln("<tr>"
									+ SessionDefaults.lineSeperator + "  <th>"
									+ titleStr + "</th>"
									+ SessionDefaults.lineSeperator + "</tr>");
						} else if (keyWordToken.equalsIgnoreCase(SqlEnum.MVS
								.toString()) && k == 0)
							writer.writeln("<tr>"
									+ SessionDefaults.lineSeperator + "  <th>"
									+ titleStr + "</th>"
									+ SessionDefaults.lineSeperator + "</tr>");
						if (k > 0) {
							writer.writeln("<tr>");
							writer.writeln("  <td>"
									+ String.valueOf(result.get(k)) + "</td>");
							writer.writeln("</tr>");
						}
						break;

					default:
						writer.write(utils.formatString(
								String.valueOf(result.get(k)),
								maxLength[displayPos] + 1, ' ', " "));
						if (k + 1 == colCount) {
							for (int j = colCount; j < colCount + colCount; j++) {
								result.add(
										j,
										utils.formatString("-", maxLength[j
												- colCount], '-', "-"));
							}
						}

						if (k + 1 >= colCount && (k + 1) % colCount == 0)
							writer.writeln();

						if (displayPos + 1 == colCount) {
							displayPos = -1;
						}
						displayPos++;
						break;
					}
				}
				switch (sessObj.getDisplayFormat()) {
				case SessionDefaults.XML_FORMAT:
					writer.writeln("  </" + utils.removeSpaces(titleStr) + ">");
					break;
				}
				result = null;

			} else if (keyWordToken.equalsIgnoreCase(SqlEnum.MVS.toString()))
				formatResults(SessionError.SHOW_TAB_MVS_NOT_FOUND + schemaName
						+ "." + tableName);
			else if (keyWordToken.equalsIgnoreCase(SqlEnum.TRIGGERS.toString()))
				formatResults(SessionError.SHOW_TAB_TRIG_NOT_FOUND + schemaName
						+ "." + tableName);
			else if (keyWordToken.equalsIgnoreCase(SqlEnum.INDEXES.toString()))
				formatResults(SessionError.SHOW_TAB_IDX_NOT_FOUND + schemaName
						+ "." + tableName);
			else
				formatResults(SessionError.SHOW_TAB_SYN_NOT_FOUND + schemaName
						+ "." + tableName);

			continue;
		}
		// temporary unable to obtain these info for SEA
		/*
		 * if (tbrsmd.next()) {
		 *
		 * switch (sessObj.getDisplayFormat()) { case
		 * SessionDefaults.XML_FORMAT: sessObj.getXmlObj().init();
		 * sessObj.getXmlObj().handleStartTags(); if (howManyOptions > 1)
		 * writer.writeln(" <Indexes>"); break; case
		 * SessionDefaults.HTML_FORMAT: sessObj.getHtmlObj().init();
		 * sessObj.getHtmlObj().handleStartTags(); if (howManyOptions > 1) {
		 * writer.writeln("<tr>"); writer.writeln(" INDEXES");
		 * writer.writeln("</tr>"); } writer.writeln("<tr>");
		 * writer.writeln("  <th>" + "COLUMN NAME" + "</th>");
		 * writer.writeln("  <th>" + "ORDER" + "</th>"); writer.writeln("  <th>"
		 * + "INDEX TYPE" + "</th>"); writer.writeln("  <th>" + "UNIQUE" +
		 * "</th>"); writer.writeln("  <th>" + "CARIDNALITY" + "</th>");
		 * writer.writeln("  <th>" + "POSITION" + "</th>");
		 * writer.writeln("</tr>"); break; default: writer.writeln(); if
		 * (howManyOptions > 1) {
		 * writer.writeln(utils.formatString(SqlEnum.INDEXES .toString(), 52,
		 * ' ', " ")); writer.writeln(utils.formatString("-", headingLength,
		 * '-', " ")); } writer.write(utils .formatString("COLUMN NAME", 52,
		 * ' ', " ")); writer.write(utils.formatString("ORDER", 6, ' ', " "));
		 * writer .write(utils.formatString("INDEX TYPE", 11, ' ', " "));
		 * writer.write(utils.formatString("UNIQUE", 7, ' ', " "));
		 * writer.write(utils .formatString("CARDINALITY", 12, ' ', " "));
		 * writer.writeln(utils.formatString("POSITION", 9, ' ', " "));
		 * writer.write(utils.formatString("-", 52, '-', " "));
		 * writer.write(utils.formatString("-", 6, '-', " "));
		 * writer.write(utils.formatString("-", 11, '-', " "));
		 * writer.write(utils.formatString("-", 7, '-', " "));
		 * writer.write(utils.formatString("-", 12, '-', " "));
		 * writer.writeln(utils.formatString("-", 9, '-', " ")); break; }
		 *
		 * index_outer_loop: while (true) { if
		 * (tbrsmd.getString(SqlEnum.INDEX_NAME.toString()) == null) { break; }
		 * else { indexName = tbrsmd.getString(SqlEnum.INDEX_NAME .toString());
		 * ++indexCnt; switch (sessObj.getDisplayFormat()) { case
		 * SessionDefaults.XML_FORMAT: writer.writeln("  <index id=\"" +
		 * indexCnt + "\" name=\"" + utils.formatXMLdata(indexName) + "\">");
		 * break; case SessionDefaults.HTML_FORMAT: writer.writeln("<tr id=\"" +
		 * indexCnt + "\" name=\"" + indexName + "\">"); break; default:
		 * writer.writeln(); writer.writeln("Index " + indexCnt + " :" +
		 * indexName); writer.writeln(utils.formatString("-", indexName
		 * .length() + 9, '-')); break; }
		 *
		 * do { if (tbrsmd.getString(SqlEnum.INDEX_NAME.toString()) == null) {
		 * break index_outer_loop; } else if (!indexName.equalsIgnoreCase(tbrsmd
		 * .getString(SqlEnum.INDEX_NAME.toString()))) { idxColCount = 0; switch
		 * (sessObj.getDisplayFormat()) { case SessionDefaults.XML_FORMAT:
		 * writer.writeln("  </Index>"); break; case
		 * SessionDefaults.HTML_FORMAT: writer.writeln("</tr>"); break; }
		 * continue index_outer_loop; } String indexType = ""; switch
		 * (tbrsmd.getShort("TYPE")) { case
		 * DatabaseMetaData.tableIndexClustered: indexType = "Clustered"; break;
		 * case DatabaseMetaData.tableIndexHashed: indexType = "Hashed"; break;
		 * case DatabaseMetaData.tableIndexStatistic: indexType = "Statistic";
		 * break; case DatabaseMetaData.tableIndexOther: indexType = "Other";
		 * break; } String indexColName = tbrsmd .getString("COLUMN_NAME");
		 * String indexOrder = tbrsmd.getString("ASC_OR_DESC")
		 * .equalsIgnoreCase("A") ? "ASC" : "DESC"; String indexUniq =
		 * tbrsmd.getBoolean("NON_UNIQUE") == true ? "No" : "Yes"; int
		 * indexCardinality = tbrsmd.getInt("CARDINALITY"); int indexOrdinalPos
		 * = tbrsmd .getInt("ORDINAL_POSITION");
		 *
		 * switch (sessObj.getDisplayFormat()) { case
		 * SessionDefaults.XML_FORMAT: writer.writeln("    <col num=\"" +
		 * ++idxColCount + "\">"); writer.writeln("      <columnName>" +
		 * indexColName + "</columnName>"); writer.writeln("      <order>" +
		 * indexOrder + "</order>"); writer.writeln("      <type>" + indexType +
		 * "</type>"); writer.writeln("      <unique>" + indexUniq +
		 * "</unique>"); writer.writeln("      <carinality>" + indexCardinality
		 * + "</cardinality>"); writer.writeln("      <position>" +
		 * indexOrdinalPos + "</position>"); writer.writeln("    </col>");
		 * break; case SessionDefaults.HTML_FORMAT: writer.writeln(" <tr>");
		 * writer.writeln("   <td>" + indexColName + "</td>"); writer
		 * .writeln("   <td>" + indexOrder + "</td>"); writer.writeln("   <td>"
		 * + indexType + "</td>"); writer.writeln("   <td>" + indexUniq +
		 * "</td>"); writer.writeln("   <td>" + indexCardinality + "</td>");
		 * writer.writeln("   <td>" + indexOrdinalPos + "</td>");
		 * writer.writeln(" </tr>"); break; default:
		 * writer.write(utils.formatString(indexColName, 52, ' ', " "));
		 * writer.write(utils.formatString(indexOrder, 6, ' ', " "));
		 * writer.write(utils.formatString(indexType, 11, ' ', " "));
		 * writer.write(utils.formatString(indexUniq, 7, ' ', " "));
		 * writer.write(utils.formatString("", 12, ' ', indexCardinality +
		 * " ")); writer.write(utils.formatString("", 9, ' ', indexOrdinalPos +
		 * " ")); writer.writeln(); break; } } while (tbrsmd.next()); break; } }
		 * switch (sessObj.getDisplayFormat()) { case
		 * SessionDefaults.XML_FORMAT: writer.writeln("  </Index>");
		 * writer.writeln("</Indexes>"); break; case
		 * SessionDefaults.HTML_FORMAT: writer.writeln("</tr>"); break; } //
		 * writer
		 * .write(utils.formatString(tbrsmd.getString(SqlEnum.INDEX_NAME.toString
		 * ()),30,' ')); // writer.writeln();
		 *
		 * }
		 *
		 * }
		 */
		printElapsedTime();
		writer.writeEndTags(sessObj);
	}

	private void formatResults(String objInfo) throws IOException

	{
		switch (sessObj.getDisplayFormat()) {
		case SessionDefaults.XML_FORMAT:
			sessObj.getXmlObj().init();
			sessObj.getXmlObj().handleStartTags();
			if ((objInfo != null) && (!objInfo.trim().equals(""))) {
				writer.writeln(sessObj.getXmlObj()._beginCdataTag);
				writer.writeln(objInfo);
				writer.writeln(sessObj.getXmlObj()._endCdataTag);
			}
			break;
		case SessionDefaults.HTML_FORMAT:
			sessObj.getHtmlObj().init();
			sessObj.getHtmlObj().handleStartTags();
			if ((objInfo != null) && (!objInfo.trim().equals(""))) {
				writer.writeln(sessObj.getHtmlObj()._startCommentTag);
				writer.writeln(objInfo);
				writer.writeln(sessObj.getHtmlObj()._endCommentTag);
			}
			break;
		default:
			writer.writeln();
			if ((objInfo != null) && (!objInfo.trim().equals(""))) {
				writer.writeln(objInfo);
			}
			break;
		}
		if ((objInfo != null) && objInfo.matches(ciServerErrMsg))
			sessObj.setLastError(-1);
		else
			sessObj.setLastError(0);

	}

	private void handleOutput(String desc, String val) throws IOException {
		handleOutput(desc, val,
				(sessObj.getDisplayFormat() != SessionDefaults.XML_FORMAT));
	}

	private void handleOutput(String desc, String val, boolean leadingNewLine)
			throws IOException {
		if (leadingNewLine) {
			writer.writeln("");
		}
		String qStr = Utils.trimSQLTerminator(sessObj.getQuery().getQueryText()
				.trim(), sessObj.getSessionSQLTerminator());
		switch (sessObj.getDisplayFormat()) {
		case SessionDefaults.HTML_FORMAT:
			sessObj.getHtmlObj().init();
			sessObj.getHtmlObj().handleStartTags();
			writer.writeln("<tr>");
			if (desc.endsWith(":"))
				desc = desc.substring(0, desc.length() - 1).trim();
			writer.writeln("   <th>" + desc.trim() + "</th>");
			writer.writeln("   <td>" + val + "</td>");
			writer.writeln("</tr>");

			if (!isMultiLineResult()) {
				if (sessObj.isSessionTimingOn())
					writer.writeln(sessObj.getHtmlObj()._startCommentTag
							+ writer.getElapsedTime(sessObj, qryObj, utils)
							+ sessObj.getHtmlObj()._endCommentTag);

				writer.printElapsedQuietMode(writer.getElapsedTime(sessObj,
						qryObj, utils));
				sessObj.getHtmlObj().handleEndTags();
			}
			break;
		case SessionDefaults.XML_FORMAT:
			sessObj.getXmlObj().init();
			sessObj.getXmlObj().handleStartTags();
			if (desc.endsWith(":"))
				desc = desc.substring(0, desc.length() - 1).trim();
			writer.writeln(" <" + utils.removeSpaces(desc) + ">"
					+ utils.formatXMLdata(val) + "</"
					+ utils.removeSpaces(desc) + ">");
			if (!isMultiLineResult()) {
				if (sessObj.isSessionTimingOn())
					writer.writeln(sessObj.getXmlObj()._beginCdataTag
							+ writer.getElapsedTime(sessObj, qryObj, utils)
							+ sessObj.getXmlObj()._endCdataTag);

				writer.printElapsedQuietMode(writer.getElapsedTime(sessObj,
						qryObj, utils));
				sessObj.getXmlObj().handleEndTags();
			}
			break;
		case SessionDefaults.CSV_FORMAT:
			writer.write(desc);
			if (sessObj.getStrDisplayFormat().equalsIgnoreCase("COLSEP"))
				writer.write(sessObj.getSessionColSep());
			else
				writer.write(",");
			writer.write(val);
			writer.writeln();
			break;
		default:
			if (qStr.equalsIgnoreCase("show alias")
					|| qStr.equalsIgnoreCase("show alias " + desc)
					|| qStr.equalsIgnoreCase("show aliases")) {
				writer.writeln(desc + " AS " + val);
			} else if (!qStr.equalsIgnoreCase("show prepared")
					&& !qStr.equalsIgnoreCase("show prepared " + desc))
				writer.writeln(desc + " " + val);
			else {
				writer.writeln(SessionDefaults.lineSeperator + desc);
				writer.writeln(" "
						+ val.replaceAll(SessionDefaults.lineSeperator,
								SessionDefaults.lineSeperator + " "));
			}

			if (!isMultiLineResult() && sessObj.isSessionTimingOn()) {
				writer.writeln();
				writer.writeln(writer.getElapsedTime(sessObj, qryObj, utils));
				writer.printElapsedQuietMode(writer.getElapsedTime(sessObj,
						qryObj, utils));
			}
			break;
		}
	}

	private void handleDumpFileAccess() throws IOException, SQLException,
			UnKnownInterfaceCommand {
		/**
		 * ALLOW & DENY command are unavailable for SeaQuest
		 *
		 */

		String cmd = SqlEnum.ALLOW.toString();
		if (sessObj.getQuery().getQueryId() == SessionDefaults.DENY)
			cmd = SqlEnum.DENY.toString();

		formatResults(SessionError.NOT_SUPPORT + cmd + " command.");
		return;
	}

	private boolean isMultiLineResult() {
		switch (sessObj.getQuery().getQueryId()) {
		case ENV:
		case SESSION:
		case SHOW_SESSION:
		case SHOW_PARAM:
		case SHOW_PARAMS:
		case SHOW_PREPARED:
		case VERSION:
		case SHOW_ALIAS:
		case SHOW_ALIASES:
		case SHOW_PATTERN:
		case SHOW_PATTERNS:
			return true;

		default:
			return false;

		}

	}

	private void printElapsedTime() throws IOException {
		if (sessObj.isSessionTimingOn()) {
			String elapsedTimeMsg = writer.getElapsedTime(sessObj, qryObj,
					utils);
			this.formatResults(elapsedTimeMsg);

			writer.printElapsedQuietMode(elapsedTimeMsg);

		}
	}

	private void startTimerTask(Connection connObj) {
		sessObj.setTimeoutTask(new SessionTimeoutTask(connObj));
		sessObj.getTimeoutTask().idleTime = sessObj.getSessionIdletime();
		sessObj.getTimeoutTask().lastQueryExecTime = System.currentTimeMillis();
		Timer timer = new Timer();
		timer.schedule(sessObj.getTimeoutTask(), 1 * 60 * 1000, 1 * 60 * 1000);
	}

	private void handleDelay() throws IOException {
		int timeVal = 0;
		if (maxDelayVal == -1)
			maxDelayVal = SessionDefaults.MAX_DELAY_LIMIT * 1000;
		timeVal = parser.getDelayTime(parser.getRemainderStr());

		if (timeVal < 0 || timeVal > maxDelayVal) {
			sessionError('E', new ErrorObject(SessionError.MAX_DELAY_LIMIT, "",
					maxDelayVal / 1000 + " seconds."));
			return;
		}
		if (timeVal != -1) {
			// Catch Exception so that the cron job doesn't fail.
			try {
				Signal.handle(INTSignal, DelayCTRLCHandler);
			} catch (Exception e) {

			}
			try {
//				curThread = Thread.currentThread();
//				curThread.sleep(timeVal);
				Thread.sleep(timeVal);
			} catch (InterruptedException interruptEx) {

			}
		} else {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}
	}

	private void handleAlias() throws IOException, SQLException,
			UnKnownInterfaceCommand {
		String paramName = parser.getNextKeyToken();
		String setValue = "";

		if ((paramName == null) || (paramName.equalsIgnoreCase("as"))) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}
		if (!(sessObj.vQryObj.getQuery().getQueryText().trim().endsWith(sessObj
				.getSessionSQLTerminator()))) {
			writeSyntaxError(this.queryStr, "");
			return;
		}
		if (sessObj.getVQryObj().getfKeyMap()
				.containsKey(paramName.toUpperCase())) {
			sessionError(
					'E',
					new ErrorObject(
							SessionError.COMMAND_NAME_NOT_ALLOWED,
							"",
							paramName.toUpperCase()
									+ SessionError.COMMAND_NAME_NOT_ALLOWED_SUFFIX));
			return;
		}

		String nextToken = parser.getNextKeyToken();
		if (nextToken == null || !nextToken.trim().toUpperCase().equals("AS")) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}
		setValue = parser.getNextValueToken();

		if (setValue == null || setValue.trim().equals("")
				|| (parser.hasMoreTokens())) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return;
		}

		if (sessObj.getAliasMap() != null) {
			if ((sessObj.getAliasMap().containsKey(setValue))) {
				sessionError('E', new ErrorObject(
						SessionError.INVALID_COMMAND_FOR_ALIAS, "", setValue
								+ ""));
				return;
			}
		}

		sessObj.setAlias(paramName, setValue);

	}

	private void handleShowAliases() throws IOException, SQLException {
		if (!checkForMoreTokens())
			displayAlias(null);
	}

	private void handleShowAlias() throws IOException, SQLException {
		String paramName = parser.getNextKeyToken();
		if (!checkForMoreTokens())
			displayAlias(paramName);
	}

	private void displayAlias(String paramName) throws IOException {
		int totalCount = 0;
		boolean matchFound = false;

		if (sessObj.getAliasMap() != null) {
			if (paramName != null) {
				paramName = paramName.toUpperCase();
				if ((paramName.indexOf("*") != -1)
						|| (paramName.indexOf("?") != -1))
					paramName = parser.replaceShowPrepPattern(paramName);

				paramName = utils.trimDoubleQuote(paramName);
				try {
					paramName = paramName.replaceAll("_", ".");
					paramName = paramName.replaceAll("%", ".*");
				} catch (PatternSyntaxException pse) {
					paramName = "";
				}
			}

			HashMap<String, String> aliasHash = sessObj.getAliasMap();
			Map<String,String> sortedMap = new TreeMap<String, String>(aliasHash);
			Iterator<String> aliasMapIt = sortedMap.keySet().iterator();

			while (aliasMapIt.hasNext()) {
				String pNameKey = aliasMapIt.next();
				if ((paramName == null || pNameKey.matches(paramName))) {
					handleOutput(pNameKey, aliasHash.get(pNameKey), false);
					matchFound = true;
					totalCount++;
				}
			}

			printElapsedTime();
			writer.writeEndTags(sessObj);
		}

		if (!matchFound) {
			sessionError('E', SessionError.SHOW_ALIAS_NOT_FOUND);
			return;

		}
		sessObj.setTotalRecordCount(String.valueOf(totalCount));

	}

	private void handleShowPattern() throws IOException {
		String key = null;

		String patternName = parser.getNextValueToken();
		if (checkForMoreTokens())
			return;

		if (patternName != null) {
			key = parser.matchKeyPat(patternName);
			// If the patternName does not match the $$key$$ format,
			// retain the user specified patternName value.
			if (key != null)
				patternName = key;
			displayPattern(patternName);
		} else {
			sessionError('E', SessionError.SHOW_PATTERN_NOT_FOUND);
			return;
		}

	}

	private void handleShowPatterns() throws IOException {
		if (!checkForMoreTokens())
			displayPattern(null);
	}

	private void displayPattern(String patternName) throws IOException {
		boolean displayPatMsg = true;
		boolean displayRegExpMsg = true;
		int totalCount = 0;

		Iterator<String> it = null;
		HashMap<String, RepObjInterface> patHMap = sessObj.getPatternHashMap();
		if (patHMap != null && !patHMap.isEmpty()) {
			it = patHMap.keySet().iterator();
			while (it.hasNext()) {
				String key = it.next();
				if (!sessObj.getEnvMap().containsKey(key)
						&& ((patternName == null) || key
								.equalsIgnoreCase(patternName))) {
					if (displayPatMsg) {
						displayPatMsg = false;
						writer.writeln("Patterns");
						writer.writeln("--------");
					}
					handleOutput(key, sessObj.getPatternValue(key));
					totalCount++;
				}
			}
		}

		if (!displayPatMsg)
			writer.writeln();

		HashMap<String, String> regExpHMap = sessObj.getRegExpMap();
		if (regExpHMap != null && !regExpHMap.isEmpty()) {
			it = regExpHMap.keySet().iterator();
			while (it.hasNext()) {
				String key = it.next();
				if ((patternName == null) || key.equalsIgnoreCase(patternName)) {
					if (displayRegExpMsg) {
						displayRegExpMsg = false;
						writer.writeln("Regular Expressions");
						writer.writeln("-------------------");
					}
					handleOutput("/" + key, "/" + regExpHMap.get(key)
							+ "/");
					totalCount++;

				}
			}

		}
		if ((displayPatMsg) && (displayRegExpMsg)) {
			sessionError('E', SessionError.SHOW_PATTERN_NOT_FOUND);
			return;
		}

		printElapsedTime();
		writer.writeEndTags(sessObj);
		sessObj.setTotalRecordCount(String.valueOf(totalCount));

	}

	private String getAnsiName(String oName, int objectTokens) {

		List<String> objName = parser.getCSTList(oName);
		String ansiName = null;
		switch (objName.size()) {
		case 3:
			ansiName = objName.get(0) + "."
					+ objName.get(1) + "."
					+ objName.get(2);
			break;
		case 2:
			if (objectTokens == 2)
				ansiName = objName.get(0) + "."
						+ objName.get(1);
			else
				ansiName = sessObj.getSessionCtlg() + "."
						+ objName.get(0) + "."
						+ objName.get(1);
			break;
		case 1:
			if (objectTokens == 2)
				ansiName = sessObj.getSessionCtlg() + "."
						+ objName.get(0);
			else
				ansiName = sessObj.getSessionCtlg() + "."
						+ sessObj.getSessionSchema() + "."
						+ objName.get(0);
			break;
		}
		return ansiName;
	}

	private void loadEnvVarMap() {
		HashMap<String, RepObjInterface> patternMap = sessObj.getPatternHashMap();

		if (patternMap == null)
			patternMap = new HashMap<String, RepObjInterface>();

		String[] envVarList = { "COLSEP", "DATASOURCE", "HISTOPT",
				"IDLETIMEOUT", "LIST_COUNT", "LOG_FILE", "LOG_OPTIONS", "LOG",
				"LOOK_AND_FEEL", "MARKUP", "MODE", "PROMPT", "ROLE", "SCHEMA",
				"SERVER", "SERVICE_NAME", "SQLTERMINATOR", "STATISTICS",
				"TIME", "TIMING", "USER" };

		for (int i = 0; i < envVarList.length; i++) {
			patternMap.put(envVarList[i], sessObj);
		}

		sessObj.setPatternHashMap(patternMap);
	}

	private boolean checkForMoreTokens() throws IOException {
		if (parser.hasMoreTokens()) {
			writeSyntaxError(this.queryStr, parser.getRemainderStr());
			return true;
		}
		return false;
	}

	private void addEnvValuesMap() {
		envMap.put("COLSEP", "\"" + sessObj.getSessionColSep() + "\"");
		envMap.put("DATASOURCE", sessObj.getSessionDsn());
		envMap.put("HISTOPT", sessObj.isSessionHistoryAll() ? "ALL"
				: "DEFAULT [No expansion of script files]");
		envMap.put(
				"IDLETIMEOUT",
				(sessObj.getSessionIdletime())
						+ " min(s) "
						+ (sessObj.getSessionIdletime() == 0 ? "[Never Expires]"
								: "").trim());

		if (sessObj.getListCount() == 0)
			envMap.put("LIST_COUNT", ((sessObj.getListCount()) + " [All Rows]"));
		else
			envMap.put("LIST_COUNT", String.valueOf((sessObj.getListCount())));

		envMap.put("LOG_FILE", sessObj.getSpoolFileName());
		envMap.put("LOG_OPTIONS", (sessObj.isLogAppend() ? "APPEND" : "CLEAR")
				+ (sessObj.isQuietEnabled() ? ",QUIET" : "") + ",CMDTEXT "
				+ (sessObj.isLogCmdText() ? "ON" : "OFF").trim());
		envMap.put("LOG", sessObj.isSessionLogging() ? "ON" : "OFF");

		envMap.put("LOOK_AND_FEEL", sessObj.getStrSessView());
		envMap.put("MARKUP", (sessObj.getStrDisplayFormat()));
		envMap.put("MODE", (sessObj.getStrMode()));
		envMap.put("PROMPT", (sessObj.getSessionPrompt()));
		envMap.put("ROLE", (sessObj.getSessionRole()));
		envMap.put("SCHEMA", (sessObj.getSessionSchema()));
		envMap.put("SERVER",
				(sessObj.getSessionServer() + sessObj.getSessionPort()));

		envMap.put("SQLTERMINATOR", (sessObj.getSessionSQLTerminator()));
		envMap.put("STATISTICS", (sessObj.isSessionStatsEnabled() ? "ON"
				: "OFF"));
		envMap.put("TIME", (sessObj.isSessionTimeOn() ? "ON" : "OFF"));
		envMap.put("TIMING", (sessObj.isSessionTimingOn() ? "ON" : "OFF"));
		envMap.put("USER", (sessObj.getSessionUser()));

	}


	private void rewriteQryString(String currentMode, String toMode) {
		String newQry = "MODE " + toMode + sessObj.getSessionSQLTerminator()
				+ "\n" + parser.getRemainderStr().trim()
				+ sessObj.getSessionSQLTerminator() + "\n" + "MODE "
				+ currentMode + sessObj.getSessionSQLTerminator() + "\n"
				+ "SET CMDDISPLAY ON" + sessObj.getSessionSQLTerminator();
		qryObj.resetQueryText(newQry);
		if (sessObj.isDebugOn())
			System.out.println(this.getClass().getName() + ":: Query :"
					+ newQry);

	}

	private void displayNoConnInfo() throws IOException {
		if (sessObj.getDisplayFormat() != SessionDefaults.XML_FORMAT)
			writer.writeln();
		writer.writeInterfaceErrors(sessObj, SessionError.DB_CONN_NOT_EXIST);
	}

	private void loadIqKeyWordMap() {
		iqKeyMap.put("SET_TIME", "" + SET_TIME);
		iqKeyMap.put("SET_TIMING", "" + SET_TIMING);
		iqKeyMap.put("SET_SQLPROMPT", "" + SET_SQLPROMPT);
		iqKeyMap.put("SET_SQLTERMINATOR", "" + SET_SQLTERMINATOR);
		iqKeyMap.put("SET_PARAM", "" + SET_PARAM);
		iqKeyMap.put("SET_IDLETIMEOUT", "" + SET_IDLETIMEOUT);
		iqKeyMap.put("SET_LIST_COUNT", "" + SET_LISTCOUNT);
		iqKeyMap.put("SET_PROMPT", "" + SET_PROMPT);
		iqKeyMap.put("SET_COLSEP", "" + SET_COLSEP);
		iqKeyMap.put("SET_HISTOPT", "" + SET_HISTOPT);
		iqKeyMap.put("SET_MARKUP", "" + SET_MARKUP);
		iqKeyMap.put("SET_DISPLAY_COLSIZE", "" + SET_DISPLAY_COLSIZE);

		iqKeyMap.put("SHOW_TIME", "" + SHOW_TIME);
		iqKeyMap.put("SHOW_TIMING", "" + SHOW_TIMING);
		iqKeyMap.put("SHOW_SQLPROMPT", "" + SHOW_SQLPROMPT);
		iqKeyMap.put("SHOW_SQLTERMINATOR", "" + SHOW_SQLTERMINATOR);
		iqKeyMap.put("SHOW_SCHEMA", "" + SHOW_SCHEMA);
		iqKeyMap.put("SHOW_CATALOG", "" + SHOW_CATALOG);
		iqKeyMap.put("SHOW_SCHEMAS", "" + SHOW_SCHEMAS);
		iqKeyMap.put("SHOW_CATALOGS", "" + SHOW_CATALOGS);
		iqKeyMap.put("SHOW_TABLES", "" + SHOW_TABLES);
		iqKeyMap.put("SHOW_PARAM", "" + SHOW_PARAM);
		iqKeyMap.put("SHOW_PARAMS", "" + SHOW_PARAMS);
		iqKeyMap.put("SHOW_VIEWS", "" + SHOW_VIEWS);
		iqKeyMap.put("SHOW_TABLE", "" + SHOW_TABLE);
		iqKeyMap.put("SHOW_IDLETIMEOUT", "" + SHOW_IDLETIMEOUT);
		iqKeyMap.put("SHOW_SYNONYMS", "" + SHOW_SYNONYMS);
		iqKeyMap.put("SHOW_MVS", "" + SHOW_MVS);
		iqKeyMap.put("SHOW_MVGROUPS", "" + SHOW_MVGROUPS);
		iqKeyMap.put("SHOW_PROCEDURES", "" + SHOW_PROCEDURES);
		iqKeyMap.put("SHOW_LIST_COUNT", "" + SHOW_LISTCOUNT);
		iqKeyMap.put("SHOW_MODE", "" + SHOW_MODE);

		iqKeyMap.put("SHOW_SESSION", "" + SHOW_SESSION);
		iqKeyMap.put("SHOW_SERVICE", "" + SHOW_SERVICE);
		iqKeyMap.put("SHOW_DISPLAY_COLSIZE", "" + SHOW_DISPLAY_COLSIZE);
		iqKeyMap.put("SHOW_COLSEP", "" + SHOW_COLSEP);
		iqKeyMap.put("SHOW_PREPARED", "" + SHOW_PREPARED);
		iqKeyMap.put("SHOW_HISTOPT", "" + SHOW_HISTOPT);
		iqKeyMap.put("SET_STATISTICS", "" + SET_STATISTICS);
		iqKeyMap.put("SHOW_MARKUP", "" + SHOW_MARKUP);
		iqKeyMap.put("SET_AUTOPREPARE", "" + SET_AUTOPREPARE);
		iqKeyMap.put("SHOW_AUTOPREPARE", "" + SHOW_AUTOPREPARE);

		iqKeyMap.put("SHOW_LASTERROR", "" + SHOW_LASTERROR);
		iqKeyMap.put("SHOW_ERRORCODE", "" + SHOW_ERRORCODE);

		iqKeyMap.put("SHOW_INVENTORY", "" + SHOW_INVENTORY);

		iqKeyMap.put("SHOW_RECCOUNT", "" + SHOW_RECCOUNT);
		iqKeyMap.put("SHOW_ACTIVITYCOUNT", "" + SHOW_ACTIVITYCOUNT);

		iqKeyMap.put("SHOW_STATISTICS", "" + SHOW_STATISTICS);
		iqKeyMap.put("SHOW_ACCESS", "" + SHOW_ACCESS);
		iqKeyMap.put("LIST_OPENS", "" + LIST_OPENS);
		iqKeyMap.put("LIST_LOCKS", "" + LIST_LOCKS);
		iqKeyMap.put("SET_LOOKANDFEEL", "" + SET_LOOKANDFEEL);
		iqKeyMap.put("SHOW_LOOKANDFEEL", "" + SHOW_LOOKANDFEEL);
		iqKeyMap.put("SET_CMDECHO", "" + SET_CMDECHO);
		iqKeyMap.put("ALIAS", "" + ALIAS);
		iqKeyMap.put("SHOW_ALIAS", "" + SHOW_ALIAS);
		iqKeyMap.put("SHOW_ALIASES", "" + SHOW_ALIASES);
		iqKeyMap.put("SET_FETCHSIZE", "" + SET_FETCHSIZE);
		iqKeyMap.put("SHOW_FETCHSIZE", "" + SHOW_FETCHSIZE);
		iqKeyMap.put("SET_PATTERN", "" + SET_PATTERN);
		iqKeyMap.put("SHOW_PATTERN", "" + SHOW_PATTERN);
		iqKeyMap.put("SHOW_PATTERNS", "" + SHOW_PATTERNS);
		iqKeyMap.put("SET_PATTERNDEPTH", "" + SET_PATTERNDEPTH);
		iqKeyMap.put("SHOW_PATTERNDEPTH", "" + SHOW_PATTERNDEPTH);
		iqKeyMap.put("SET_DEBUG", "" + SET_DEBUG);
		iqKeyMap.put("SHOW_DEBUG", "" + SHOW_DEBUG);

		iqKeyMap.put("SHOW_ROLE", "" + SHOW_ROLE);
		iqKeyMap.put("SET_CONNECTOPT", "" + SET_CONNECTOPT);
		iqKeyMap.put("SET_CMDDISPLAY", "" + SET_CMDDISPLAY);
		iqKeyMap.put("SHOW_CMDDISPLAY", "" + SHOW_CMDDISPLAY);
		iqKeyMap.put("SHOW_REMOTEPROCESS", "" + SHOW_PROCESSNAME);
		iqKeyMap.put("SHOW_USER", "" + SHOW_USER);
		iqKeyMap.put("SHOW_SERVER", "" + SHOW_SERVER);
		iqKeyMap.put("SHOW_CONNECTOPT", "" + SHOW_CONNECTOPT);

		iqKeyMap.put("SHOW_INDEXES", "" + SHOW_INDEXES);
	}

}
