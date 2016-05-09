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

public interface SessionDefaults {
   final String DATABASE_EDITION = System.getenv("TRAFODION_VER_PROD");
   final String PROD_NAME = System.getenv("TRAFODION_VER_PROD")+" Command Interface ";
   final String APP_NAME = "TrafCI";
   final String DRIVER_NAME = "org.trafodion.jdbc.t4.T4Driver"; 
   final String PKG_NAME = "/org/trafodion/ci/";
   final String PROP_DEFAULT_NAME = PKG_NAME + "Properties/trafciDefaultLookAndFeel.properties";
   final String PROP_TRAFCILF = "trafciLF";
   final String PROP_TRACE = "trafci.enableTrace";
   final String PROP_OS = "os.name";
   final String PROP_PRINT_TIME = "trafci.printConnTime";
   final String PROP_MAX_DELAY = "trafci.maxDelayLimit";
   
   String dsnName="Default_DataSource";
   String portNumber="18650";
   String lineSeperator=System.getProperty("line.separator");
   

	String DEFAULT_SQL_PROMPT = "SQL>";
	String DEFAULT_WMS_PROMPT = "WMS>";
	String DEFAULT_CS_PROMPT = "CS>";
	String SQL = "SQL";
	String WMS = "WMS";
	String CS = "CS";
   
   String DEFAULT_DISPLAY_FORMAT="RAW";
   
   final int trafciMajorVersion=1;
   final int trafciMinorVersion=0;

   final int QRY_HISTORY_MAX_LMT=100;
   final String DEFAULT_DATA_SOURCE="Default_DataSource";

   final int abruptExit=-9999;
   final int DEFAULT_EXIT_CODE = 0;
   
   final int SQL_ERR_CONN_MAX_LIMIT=-29154;
   final int NDCS_ERR_DSN_NOT_AVAILABLE = -29164;
   final int UNKNOWN_DATA_SOURCE=29182;
   final int MIN_COL_DISPLAY_SIZE = 4;
   final int DISPLAY_COLSIZE = 160;
   final int WMS_PROCESS_DOES_NOT_EXIST=-29162;
   final int SQL_ERR_INVALID_AUTH=-29153;
   final int MAX_DELAY_LIMIT=3600; //in seconds
   final int DEFAULT_FETCH_SIZE=0; 
   final int MAX_PATTERN_DEPTH=1; 
   final int PATTERN_DEPTH_LIMIT = 10;
   final int SQL_ERR_CLI_AUTH=-8837;
   
   /* don't ask me why mxci does this, but look at SqlCmd.cpp of
    * mxci's code line 1534, and you'll see that 72 is their limit,
    * after that they trim the whole last column header
    */
   final int MXCI_TRIM_FIX = 72;
   
   final int IQ=0;
   final int SQLQ=1;
   final int PRUNQ=2;
   //final int CSQ=3;
   final int NSQ=4;
   final int CQ=5;      // conditional query
   //final int SECQ=6; 
   final int SPJQ=7;    // SPJ deployment

   final int USERI=0;
   final int PRUNI=1;
   final int PERLI=2;
   final int PYTHI=3;

   final int SQL_MODE=0;
   final int CS_MODE=2;
   final int WMS_MODE=4;  

   final int CONSOLE_INPUT=0;
   final int FILE_INPUT=1;

   final int CONSOLE_READ_MODE=0;
   final int SCRIPT_READ_MODE=1;
   final int OBEY_READ_MODE=2;

   final int CONSOLE_WRITE_MODE=0;
   final int LOG_WRITE_MODE=1;
   final int SPOOL_WRITE_MODE=2;
   final int CONSOLE_SPOOL_WRITE_MODE=3;
   final int SILENT_SPOOL_WRITE_MODE=4;

   final int CIDEFAULT_VIEW=0;
   final int MXCI_VIEW=4;

   final int RAW_FORMAT=0;
   final int HTML_FORMAT=1;
   final int XML_FORMAT=2;
   final int CSV_FORMAT=3;

   // include only the interface commands first tokens
   final int iqKeyWordBase=100;
   final int EXIT=iqKeyWordBase + 1;
   //final int QUIT=iqKeyWordBase + 2;
   final int DISCONNECT=iqKeyWordBase + 3;
   final int SET=iqKeyWordBase + 4;
   final int ED=iqKeyWordBase + 5;
   final int EDIT=iqKeyWordBase + 6;
   final int SPOOL=iqKeyWordBase + 7;
   final int SLASH=iqKeyWordBase + 8;
   final int RUN=iqKeyWordBase + 9;
   final int SHOW=iqKeyWordBase + 10;
   final int AT=iqKeyWordBase + 11;
   final int OBEY=iqKeyWordBase + 12;
   final int LOG=iqKeyWordBase + 13;
   final int MODE=iqKeyWordBase+14;
   final int RESET=iqKeyWordBase+15;
   final int PRUN=iqKeyWordBase + 16;
   final int SET_TIME=iqKeyWordBase+17;
   final int SHOW_TIME=iqKeyWordBase+18;
   final int SET_TIMING=iqKeyWordBase+19;
   final int SHOW_TIMING=iqKeyWordBase+20;
   final int SET_SQLPROMPT=iqKeyWordBase+21;
   final int SHOW_SQLPROMPT=iqKeyWordBase+22;
   final int SET_SQLTERMINATOR=iqKeyWordBase+23;
   final int SHOW_SQLTERMINATOR=iqKeyWordBase+24;
   final int SET_PARAM=iqKeyWordBase+25;
   final int SHOW_PARAM=iqKeyWordBase+26;
   final int SET_IDLETIMEOUT=iqKeyWordBase+27;
   final int SHOW_IDLETIMEOUT=iqKeyWordBase+28;
   final int SHOW_SCHEMA=iqKeyWordBase+29;
   final int SHOW_CATALOG=iqKeyWordBase+30;
   final int SHOW_CATALOGS=iqKeyWordBase+31;
   final int SHOW_SCHEMAS=iqKeyWordBase+32;
   final int SHOW_TABLES=iqKeyWordBase+33;
   final int SHOW_VIEWS=iqKeyWordBase+34;
   final int SHOW_TABLE=iqKeyWordBase+35;
   final int SHOW_SYNONYMS=iqKeyWordBase+36;
   final int SHOW_MVS=iqKeyWordBase+37;
   final int SHOW_MVGROUPS=iqKeyWordBase+38;
   final int HISTORY=iqKeyWordBase+39;
   final int REPEAT=iqKeyWordBase+40;
   final int FC=iqKeyWordBase+41;
   final int CLEAR=iqKeyWordBase+42;
   final int SET_LISTCOUNT=iqKeyWordBase+43;
   final int SHOW_LISTCOUNT=iqKeyWordBase+44;
   final int SECTION=iqKeyWordBase+45;
   final int HELP=iqKeyWordBase+46;
   final int SHOW_PROCEDURES=iqKeyWordBase+47;
   final int ENV=iqKeyWordBase+48;
   final int SHOW_MODE=iqKeyWordBase+49;
   final int SESSION=iqKeyWordBase+50;
   final int SHOW_SESSION=iqKeyWordBase+51;
   final int SET_PROMPT=iqKeyWordBase+52;
   final int SAVEHIST=iqKeyWordBase+53;
   final int VERSION=iqKeyWordBase+54;
   final int SET_LOOKANDFEEL=iqKeyWordBase+55;
   final int SHOW_LOOKANDFEEL=iqKeyWordBase+56;
   final int SET_SERVICE=iqKeyWordBase+57;
   final int SHOW_SERVICE=iqKeyWordBase+58;
   final int SET_DISPLAY_COLSIZE=iqKeyWordBase+59;
   final int SHOW_DISPLAY_COLSIZE=iqKeyWordBase+60;
   
   final int SET_COLSEP=iqKeyWordBase+61;
   final int SHOW_COLSEP=iqKeyWordBase+62;
   final int CONNECT=iqKeyWordBase+63;
   final int RECONNECT=iqKeyWordBase+64;
   final int SHOW_PREPARED=iqKeyWordBase+65;
   final int SET_HISTOPT=iqKeyWordBase+66;
   final int SHOW_HISTOPT=iqKeyWordBase+67;
   final int GET=iqKeyWordBase+68;
   final int SET_STATISTICS=iqKeyWordBase+69;

   final int LOCALHOST=iqKeyWordBase+70;

   final int SET_MARKUP=iqKeyWordBase+71;
   final int SHOW_MARKUP=iqKeyWordBase+72;
   final int SHOW_PARAMS=iqKeyWordBase+73;
   final int AUDIT=iqKeyWordBase+74;
   final int SET_AUTOPREPARE=iqKeyWordBase+75;
   final int SHOW_AUTOPREPARE=iqKeyWordBase+76;
   final int ERROR=iqKeyWordBase+77;
   final int SHOW_LASTERROR=iqKeyWordBase+78;
   final int ONLINEDBDUMP=iqKeyWordBase+79;
   final int SHOW_INVENTORY=iqKeyWordBase+80;
   final int SHOW_RECCOUNT=iqKeyWordBase+81;
   final int SRUN=iqKeyWordBase+82;
   final int SHOW_STATISTICS=iqKeyWordBase+83;
   final int ALLOW=iqKeyWordBase+84;
   final int DENY=iqKeyWordBase+85;
   final int SCHEDULE=iqKeyWordBase+86;
   final int SHOW_ACCESS=iqKeyWordBase+87;
   final int LIST=iqKeyWordBase+88;
   final int LIST_OPENS=iqKeyWordBase+89;
   final int LIST_LOCKS=iqKeyWordBase+90;
   final int DELAY=iqKeyWordBase+91;
   final int SHOW_ERRORCODE=iqKeyWordBase+92;
   final int SHOW_ACTIVITYCOUNT=iqKeyWordBase+93;
   final int SET_CMDECHO=iqKeyWordBase+94;

   final int ALIAS=iqKeyWordBase+95;
   final int SHOW_ALIAS=iqKeyWordBase+96;
   final int SHOW_ALIASES=iqKeyWordBase+97;

   final int SET_FETCHSIZE=iqKeyWordBase+98;
   final int SHOW_FETCHSIZE=iqKeyWordBase+99;
   
   final int SET_PATTERN=iqKeyWordBase+100;
   final int SHOW_PATTERN=iqKeyWordBase+101;
   final int SHOW_PATTERNS=iqKeyWordBase+102;
   final int SET_PATTERNDEPTH=iqKeyWordBase+103;
   final int SHOW_PATTERNDEPTH=iqKeyWordBase+104;
   final int SET_DEBUG=iqKeyWordBase+105;
   final int SHOW_DEBUG=iqKeyWordBase+106;
   
   final int DOTSQL = iqKeyWordBase+107;
   final int DOTCS  = iqKeyWordBase+108;
   final int DOTNS  = iqKeyWordBase+109;
   final int DOTWMS = iqKeyWordBase+110;
   final int DOTSEC = iqKeyWordBase+111;
   
   final int SET_CMDDISPLAY  =  iqKeyWordBase+112;
   final int SHOW_CMDDISPLAY =  iqKeyWordBase+113;
   final int CREATE_SERVER   =  iqKeyWordBase+114;
   final int ALTER_SERVER    =  iqKeyWordBase+115;
   final int DROP_SERVER     =  iqKeyWordBase+116;
   final int INFO_SERVER     =  iqKeyWordBase+117;
   final int CREATE_USER     =  iqKeyWordBase+118;
   final int ALTER_USER      =  iqKeyWordBase+119;
   final int DROP_USER       =  iqKeyWordBase+120;
   final int CREATE_CSR      =  iqKeyWordBase+121;
   final int CREATE_CERT     =  iqKeyWordBase+122;
   final int INFO_CERT_POLICY = iqKeyWordBase+123;
   final int INSERT_CERT     =  iqKeyWordBase+124;
   final int ALTER_CERT_POLICY = iqKeyWordBase+125;
   final int CHANGE_PASS     =  iqKeyWordBase+126;
   final int SHOW_PROCESSNAME=  iqKeyWordBase+127;
   final int CREATE_ROLE     =  iqKeyWordBase+128;
   final int DROP_ROLE       =  iqKeyWordBase+129;   
   final int GRANT_ROLE      =  iqKeyWordBase+130;
   final int REVOKE_ROLE     =  iqKeyWordBase+131;
   final int SET_DEFAULTROLE =  iqKeyWordBase+132;   
   final int CREATE_PLATFORM_USER     = iqKeyWordBase+133;
   final int SHOW_USER       =  iqKeyWordBase+136;
   final int SHOW_SERVER     =  iqKeyWordBase+137;
   final int ALTER_ROLE      =  iqKeyWordBase+138;
   final int SHOW_ROLE       =  iqKeyWordBase+139;
   final int SET_CONNECTOPT  =  iqKeyWordBase+140;
   final int SHOW_CONNECTOPT =  iqKeyWordBase+141; 
   final int INFO_POLICY     =  iqKeyWordBase+142; 
   final int ALTER_POLICY    =  iqKeyWordBase+143;
   final int INFO_USER       =  iqKeyWordBase+144;
   final int INFO_POLICY_WITH_OPTION  =  iqKeyWordBase+145;
   final int ALTER_LOG_POLICY =  iqKeyWordBase+146;
   final int ALTER_PWD_COMPLEXITY_POLICY =  iqKeyWordBase+147;
   final int ALTER_RESET_PWD_POLICY =  iqKeyWordBase+148;
   final int ALTER_RESET_MOSTSECURE =  iqKeyWordBase+149;
   final int ALTER_RESET_ALLDEFAULT =  iqKeyWordBase+150; 
   final int ALTER_PWD_QUALITY_POLICY =  iqKeyWordBase+151;
   final int INFO_ROLE      =  iqKeyWordBase+152;

   final int LIST_JAR	= iqKeyWordBase+153;
   final int LIST_JARS	= iqKeyWordBase+154;
   final int UPLOAD_JAR	= iqKeyWordBase+155;
   final int DROP_JAR	= iqKeyWordBase+156;

   final int ADD_LIB	= iqKeyWordBase+157;
   final int MODIFY_LIB	= iqKeyWordBase+158;
   final int REMOVE_LIB	= iqKeyWordBase+160;

   final int SHOW_INDEXES=iqKeyWordBase+159;
   
   // these are special query cases for 
   // only to trim the output
   final int sqKeyBase=500;
   final int SHOWDDL=sqKeyBase+ 1;
   final int REORG=sqKeyBase + 2;
   final int REORGANIZE=sqKeyBase + 3;
   final int MAINTAIN=sqKeyBase + 4;
   final int SHOWLABEL=sqKeyBase + 5;
   final int SHOWPLAN=sqKeyBase + 6;
   final int SHOWSHAPE=sqKeyBase + 7;
   final int SHOWCONTROL=sqKeyBase + 8;
   final int INVOKE=sqKeyBase + 9;
   final int REPLICATE=sqKeyBase + 10;
   final int SHOWTRANSACTION=sqKeyBase + 11;

   // these are only db queries
   // but not a pass thru

   final int dbqKeyBase=1000;
   final int PREPARE=dbqKeyBase+1;
   final int EXECUTE=dbqKeyBase+2;
   final int EXPLAIN=dbqKeyBase+3;
   final int INFOSTATS=dbqKeyBase+4;
   final int SET_SCHEMA=dbqKeyBase+5;
   final int SET_CATALOG=dbqKeyBase+6;
   final int CALL=dbqKeyBase+7;
   final int CONTROL=dbqKeyBase+8;

   // conditional queries
   final int condKeyBase=1500;
   final int IF_THEN=condKeyBase+1;
   final int LABEL=condKeyBase+2;
   final int GOTO=condKeyBase+3;
   
   //version queries
   final int versionKeyBase = 2000; 
   final int VERSION_M5 = versionKeyBase;
   final int VERSION_M6 = versionKeyBase+100;
   final int VERSION_M7 = versionKeyBase+200;
   final int VERSION_M8 = versionKeyBase+300;
   final int VERSION_M9 = versionKeyBase+400;
   final int VERSION_M10 = versionKeyBase+500;
   final int VERSION_M11 = versionKeyBase+600;
   
   //mode change command string
	final String CMDOPEN = "CMDOPEN";
	final String WMSOPEN = "WMSOPEN";
	final String CMDCLOSE = "CMDCLOSE";
	final String WMSCLOSE = "WMSCLOSE";
   
   
   
}
