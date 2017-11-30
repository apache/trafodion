/**
* @@@ START COPYRIGHT @@@

* Licensed to the Apache Software Foundation (ASF) under one
* or more contributor license agreements.  See the NOTICE file
* distributed with this work for additional information
* regarding copyright ownership.  The ASF licenses this file
* to you under the Apache License, Version 2.0 (the
* "License"); you may not use this file except in compliance
* with the License.  You may obtain a copy of the License at

*   http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
* KIND, either express or implied.  See the License for the
* specific language governing permissions and limitations
* under the License.

* @@@ END COPYRIGHT @@@
 */
package org.trafodion.dcs.servermt;

import java.math.*;

public final class ServerConstants {
    // read buffer stat
    public static final int BUFFER_INIT                     = 0;
    public static final int HEADER_PROCESSED                = 1;
    public static final int BODY_PROCESSED                  = 2;
    // errors returned by DcsMaster
    public static final int DcsMasterNoSrvrHdl_exn          = 5;
    //
    public static final int REQUST_INIT                     = 0;
    public static final int REQUST_CLOSE                    = 1;
    public static final int REQUST_WRITE_READ               = 2;
    public static final int REQUST_WRITE_CLOSE              = 3;
    //
    // Fixed values taken from TransportBase.h
    //
    public static final int CLIENT_HEADER_VERSION_BE        = 101;
    public static final int CLIENT_HEADER_VERSION_LE        = 102;
    public static final int SERVER_HEADER_VERSION_BE        = 201;
    public static final int SERVER_HEADER_VERSION_LE        = 202;

    public static final char YES                            = 'Y';
    public static final char NO                             = 'N';

    public static final int SUCCESS                         =0;
    public static final int FAIL                            =-1;

    // header size
    public static final int HEADER_SIZE                     = 40;
    // default body size
    public static final int BODY_SIZE                       = 1024;
    //
    public static final int SIGNATURE                       = 12345; // 0x3039

    // Literals used in the outContext between Driver and Server
    public static final int DCS_MASTER_COMPONENT            = 2;
    public static final int SQL_COMPONENT                   = 3;
    public static final int ODBC_SRVR_COMPONENT             = 4;
    public static final int DRVR_COMPONENT                  = 7;
    public static final int APP_COMPONENT                   = 8;
    public static final int JDBC_DRVR_COMPONENT             = 20;
    public static final int LINUX_DRVR_COMPONENT            = 21;
    public static final int DOT_NET_DRVR_COMPONENT          = 25;
    public static final int WIN_UNICODE_DRVR_COMPONENT      = 26;
    public static final int LINUX_UNICODE_DRVR_COMPONENT    = 27;

    public static final int DCS_MASTER_VERSION_MAJOR_1      = 3;
    public static final int DCS_MASTER_VERSION_MINOR_0      = 0;
    public static final int DCS_MASTER_BUILD_1              = 1;

    public static final int CHARSET                         = 268435456; //(2^28) For charset changes compatibility
    public static final int PASSWORD_SECURITY               = 67108864; //(2^26)

    //=================MXOSRVR versions ===================
    public static final int MXOSRVR_ENDIAN                  = 256;
    public static final int MXOSRVR_VERSION_MAJOR           = 3;
    public static final int MXOSRVR_VERSION_MINOR           = 5;
    public static final int MXOSRVR_VERSION_BUILD           = 1;

    public static final short DCS_MASTER_GETSRVRAVAILABLE   = 1000 + 19;

    public static final String SERVER_NAME                  = "$serverHandler";
    public static final String SERVER_WORKER_NAME           = "$serverWorkerHandler";

    public static final int SERVER_STATE_INIT               =0;
    public static final int SERVER_STATE_AVAILABLE          =2;
    public static final int SERVER_STATE_CONNECTING         =3;
    public static final int SERVER_STATE_CONNECTED          =4;
    public static final int SERVER_STATE_CONNECT_FAILED     =5;
    public static final int SERVER_STATE_CONNECT_REJECTED   =6;
    public static final int SERVER_STATE_READ_TIMEOUTED     =7;
    public static final int SERVER_STATE_WRITE_TIMEOUTED    =8;
    public static final int SERVER_STATE_CLIENT_TIMEOUTED   =9;
    public static final int SERVER_STATE_CONNECTING_TIMEOUTED=10;
    public static final int SERVER_STATE_DISCONNECTED       =11;
    public static final int SERVER_STATE_PORTINUSE          =12;

// All server APIs. Some of them are from AS

    public static final int SRVR_API_START=3000;

    public static final int SRVR_API_INIT                   =SRVR_API_START;
    public static final int SRVR_API_SQLCONNECT             =SRVR_API_START+1;
    public static final int SRVR_API_SQLDISCONNECT          =SRVR_API_START+2;
    public static final int SRVR_API_SQLSETCONNECTATTR      =SRVR_API_START+3;
    public static final int SRVR_API_SQLENDTRAN             =SRVR_API_START+4;
    public static final int SRVR_API_SQLPREPARE             =SRVR_API_START+5;
    public static final int SRVR_API_SQLPREPARE_ROWSET      =SRVR_API_START+6;
    public static final int SRVR_API_SQLEXECUTE_ROWSET      =SRVR_API_START+7;
    public static final int SRVR_API_SQLEXECDIRECT_ROWSET   =SRVR_API_START+8;
    public static final int SRVR_API_SQLFETCH               =SRVR_API_START+9;
    public static final int SRVR_API_SQLFETCH_ROWSET        =SRVR_API_START+10;
    public static final int SRVR_API_SQLEXECUTE             =SRVR_API_START+11;
    public static final int SRVR_API_SQLEXECDIRECT          =SRVR_API_START+12;
    public static final int SRVR_API_SQLEXECUTECALL         =SRVR_API_START+13;
    public static final int SRVR_API_SQLFETCH_PERF          =SRVR_API_START+14;
    public static final int SRVR_API_SQLFREESTMT            =SRVR_API_START+15;
    public static final int SRVR_API_GETCATALOGS            =SRVR_API_START+16;
    public static final int SRVR_API_STOPSRVR               =SRVR_API_START+17;
    public static final int SRVR_API_ENABLETRACE            =SRVR_API_START+18;
    public static final int SRVR_API_DISABLETRACE           =SRVR_API_START+19;
    public static final int SRVR_API_ENABLE_SERVER_STATISTICS=SRVR_API_START+20;
    public static final int SRVR_API_DISABLE_SERVER_STATISTICS=SRVR_API_START+21;
    public static final int SRVR_API_UPDATE_SERVER_CONTEXT  =SRVR_API_START+22;
    public static final int SRVR_API_MONITORCALL            =SRVR_API_START+23;
    public static final int SRVR_API_SQLPREPARE2            =SRVR_API_START+24;
    public static final int SRVR_API_SQLEXECUTE2            =SRVR_API_START+25;
    public static final int SRVR_API_SQLFETCH2              =SRVR_API_START+26;
    public static final int SRVR_API_SQLFASTEXECDIRECT      =SRVR_API_START+27;
    public static final int SRVR_API_SQLFASTFETCH_PERF      =SRVR_API_START+28;
    public static final int SRVR_API_GETSEGMENTS            =SRVR_API_START+29;
    public static final int SRVR_API_LASTAPI                =SRVR_API_START+30;

    // InContext Connection1 Options Bits
    public static final long INCONTEXT_OPT1_SESSIONNAME             =2147483648L; // (2^31)
    public static final long INCONTEXT_OPT1_FETCHAHEAD              =1073741824L; // (2^30)
    public static final long INCONTEXT_OPT1_CERTIFICATE_TIMESTAMP   =536870912L; // (2^29)
    public static final long INCONTEXT_OPT1_CLIENT_USERNAME         =268435456L; // (2^28)

    // OutContext Connection1 Options Bits
    public static final long OUTCONTEXT_OPT1_ENFORCE_ISO88591       =1L; // (2^0)
    public static final long OUTCONTEXT_OPT1_DOWNLOAD_CERTIFICATE   =536870912L; //(2^29)
    public static final long OUTCONTEXT_OPT1_IGNORE_SQLCANCEL       =1073741824L; //(2^30)
    public static final long OUTCONTEXT_OPT1_ROLENAME               =2147483648L; //(2^31)
    public static final long OUTCONTEXT_OPT1_EXTRA_OPTIONS          = 2147483648L; // (2^31)

    public static final int SQL_PASSWORD_EXPIRING           = 8857;
    public static final int SQL_PASSWORD_GRACEPERIOD        = 8837;

    public static final int SQL_SUCCESS                     = 0; // ODBC Standard
    public static final int SQL_SUCCESS_WITH_INFO           = 1; // ODBC Standard
    public static final int SQL_NO_DATA_FOUND               = 100;
    public static final int SQL_NO_DATA                     = 100;
    public static final int SQL_ERROR                       = -1;
//
//
    public static final int SQL_CLOSE                       = 0;
    public static final int SQL_DROP                        = 1;
    public static final int SQL_UNBIND                      = 2;
    public static final int SQL_RESET_PARAMS                = 3;
    public static final int SQL_REALLOCATE                  = 4;
    public static final int SQL_CALL_NO_RESULT_SETS         = 13;
    public static final int SQL_CALL_WITH_RESULT_SETS       = 14;
    public static final int SQL_SP_RESULT_SET               = 15;
    public static final int SQL_INSERT_RWRS                 = 16;
//
// SetAttributes
//
    public static final int SQL_ATTR_ROWSET_RECOVERY        = 2000;
    public static final short SQL_ATTR_ACCESS_MODE          = 101;
    public static final short SQL_ATTR_AUTOCOMMIT           = 102;
    public static final short SQL_TXN_ISOLATION             = 108;

// The enum SQLTYPE_CODE from sqlcli.h. Trafodion extenstion are negative values.

    public static final int SQLTYPECODE_CHAR                   = 1;
    public static final int SQLTYPECODE_NUMERIC                = 2;
    public static final int SQLTYPECODE_NUMERIC_UNSIGNED       = -201;
    public static final int SQLTYPECODE_DECIMAL                = 3;
    public static final int SQLTYPECODE_DECIMAL_UNSIGNED       = -301;
    public static final int SQLTYPECODE_DECIMAL_LARGE          = -302;
    public static final int SQLTYPECODE_DECIMAL_LARGE_UNSIGNED = -303;
    public static final int SQLTYPECODE_INTEGER                = 4;
    public static final int SQLTYPECODE_INTEGER_UNSIGNED       = -401;
    public static final int SQLTYPECODE_LARGEINT               = -402;
    public static final int SQLTYPECODE_SMALLINT               = 5;
    public static final int SQLTYPECODE_SMALLINT_UNSIGNED      = -502;
    public static final int SQLTYPECODE_BPINT_UNSIGNED         = -503;
    public static final int SQLTYPECODE_IEEE_FLOAT             = 6;
    public static final int SQLTYPECODE_IEEE_REAL              = 7;
    public static final int SQLTYPECODE_IEEE_DOUBLE            = 8;
    public static final int SQLTYPECODE_FLOAT                  = 6;
    public static final int SQLTYPECODE_REAL                   = 7;
    public static final int SQLTYPECODE_DOUBLE                 = 8;
    public static final int SQLTYPECODE_TDM_FLOAT              = -411;
    public static final int SQLTYPECODE_TDM_REAL               = -412;
    public static final int SQLTYPECODE_TDM_DOUBLE             = -413;
    public static final int SQLTYPECODE_DATETIME               = 9;
    public static final int SQLTYPECODE_INTERVAL               = 10;
    public static final int SQLTYPECODE_VARCHAR                = 12;
    public static final int SQLTYPECODE_VARCHAR_WITH_LENGTH    = -601;
    public static final int SQLTYPECODE_VARCHAR_LONG           = -1;
    public static final int SQLTYPECODE_BIT                    = 14;  // not supported
    public static final int SQLTYPECODE_BITVAR                 = 15;  // not supported

    /* Date/Time/TimeStamp related constants */
    public static final int SQLDTCODE_DATE                     = 1;
    public static final int SQLDTCODE_TIME                     = 2;
    public static final int SQLDTCODE_TIMESTAMP                = 3;
    public static final int SQLDTCODE_MPDATETIME               = 4;
    public static final int dateLength                         = 10;
    public static final int timeLength                         = 8;
    public static final int timestampLength                    = 26;

    /* specifies the type of interval data type */
    public static final int SQLINTCODE_YEAR                    =  1;
    public static final int SQLINTCODE_MONTH                   =  2;
    public static final int SQLINTCODE_DAY                     =  3;
    public static final int SQLINTCODE_HOUR                    =  4;
    public static final int SQLINTCODE_MINUTE                  =  5;
    public static final int SQLINTCODE_SECOND                  =  6;
    public static final int SQLINTCODE_YEAR_MONTH              =  7;
    public static final int SQLINTCODE_DAY_HOUR                =  8;
    public static final int SQLINTCODE_DAY_MINUTE              =  9;
    public static final int SQLINTCODE_DAY_SECOND              = 10;
    public static final int SQLINTCODE_HOUR_MINUTE             = 11;
    public static final int SQLINTCODE_HOUR_SECOND             = 12;
    public static final int SQLINTCODE_MINUTE_SECOND           = 13;

    public static final int SQLDTCODE_YEAR                     = 4;
    public static final int SQLDTCODE_YEAR_TO_MONTH            = 5;
//    public static final int SQLDTCODE_YEAR_TO_DAY            = 1; //SQL DATE
    public static final int SQLDTCODE_YEAR_TO_HOUR             = 7; //ODBC TIMESTAMP(0)
    public static final int SQLDTCODE_YEAR_TO_MINUTE           = 8;
//    public static final int SQLDTCODE_YEAR_TO_SECOND         = 3; //SQL TIMESTAMP(0)
//    public static final int SQLDTCODE_YEAR_TO_FRACTION       = 3; //SQL TIMESTAMP(1 - 5)
    public static final int SQLDTCODE_MONTH                    = 10;
    public static final int SQLDTCODE_MONTH_TO_DAY             = 11;
    public static final int SQLDTCODE_MONTH_TO_HOUR            = 12;
    public static final int SQLDTCODE_MONTH_TO_MINUTE          = 13;
    public static final int SQLDTCODE_MONTH_TO_SECOND          = 14;
    public static final int SQLDTCODE_MONTH_TO_FRACTION        = 14;
    public static final int SQLDTCODE_DAY                      = 15;
    public static final int SQLDTCODE_DAY_TO_HOUR              = 16;
    public static final int SQLDTCODE_DAY_TO_MINUTE            = 17;
    public static final int SQLDTCODE_DAY_TO_SECOND            = 18;
    public static final int SQLDTCODE_DAY_TO_FRACTION          = 18;
    public static final int SQLDTCODE_HOUR                     = 19;
    public static final int SQLDTCODE_HOUR_TO_MINUTE           = 20;
//    public static final int SQLDTCODE_HOUR_TO_SECOND         = 2;  //SQL TIME(0)
//    public static final int SQLDTCODE_HOUR_TO_FRACTION       = 2;  //SQL TIME(1 - 6)
    public static final int SQLDTCODE_MINUTE                   = 22;
    public static final int SQLDTCODE_MINUTE_TO_SECOND         = 23;
    public static final int SQLDTCODE_MINUTE_TO_FRACTION       = 23;
    public static final int SQLDTCODE_SECOND                   = 24;
    public static final int SQLDTCODE_SECOND_TO_FRACTION       = 24;
    public static final int SQLDTCODE_FRACTION_TO_FRACTION     = 29;

    public static final String CLOB_HEADING            ="JDBC_CLOB_COLUMN -";
    public static final String BLOB_HEADING            ="JDBC_BLOB_COLUMN -";

    // type codes from SQL/MP include file sql.h, for TYPE_FS descriptor fields
    //(with additional SQL/MX datatypes) from sqlcli.h

    public static final int SQLDT_16BIT_SIGNED                = 130;
    public static final int SQLDT_16BIT_UNSIGNED              = 131;
    public static final int SQLDT_32BIT_SIGNED                = 132;
    public static final int SQLDT_32BIT_UNSIGNED              = 133;
    public static final int SQLDT_64BIT_SIGNED                = 134;
    // Big Num Changes
    public static final int SQLDT_NUM_BIG_S                   = 156;
    public static final int SQLDT_NUM_BIG_U                   = 155;
    // Big Num Changes

    // enum sqlCharset_CODE from sqlcli.h

    public static final int sqlCharsetCODE_UNKNOWN            =  0;
    public static final int sqlCharsetCODE_ISO88591           =  1;
    public static final int sqlCharsetCODE_KANJI              = -1;
    public static final int sqlCharsetCODE_KSC5601            = -2;
    public static final int sqlCharsetCODE_SJIS               = 10;
    public static final int sqlCharsetCODE_UCS2               = 11;
    public static final int sqlCharsetCODE_EUCJP              = 12;
    public static final int sqlCharsetCODE_BIG5               = 13;
    public static final int sqlCharsetCODE_GB18030            = 14;
    public static final int sqlCharsetCODE_UTF8               = 15;
    public static final int sqlCharsetCODE_MB_KSC5601         = 16;
    public static final int sqlCharsetCODE_GB2312             = 17;
    public static final int sqlCharsetCODE_GBK                = 18;


    public static final String sqlCharsetSTRING_UNKNOWN         = "UNKNOWN";
    public static final String sqlCharsetSTRING_ISO88591        = "ISO88591";
    public static final String sqlCharsetSTRING_KANJI           = "KANJI";
    public static final String sqlCharsetSTRING_KSC5601         = "KSC5601";
    public static final String sqlCharsetSTRING_SJIS            = "SJIS";
    public static final String sqlCharsetSTRING_UNICODE         = "UCS2";

    public static final int UNKNOWN_DATA_FORMAT                 = 0;
    public static final int ROWWISE_ROWSETS                     = 1;
    public static final int COLUMNWISE_ROWSETS                  = 2;

    public static final int INVALID_SQL_QUERY_STMT_TYPE         = 255;
    public static final int SQL_OTHER                           = -1;
    public static final int SQL_UNKNOWN                         = 0;
    public static final int SQL_SELECT_UNIQUE                   = 1;
    public static final int SQL_SELECT_NON_UNIQUE               = 2;
    public static final int SQL_INSERT_UNIQUE                   = 3;
    public static final int SQL_INSERT_NON_UNIQUE               = 4;
    public static final int SQL_UPDATE_UNIQUE                   = 5;
    public static final int SQL_UPDATE_NON_UNIQUE               = 6;
    public static final int SQL_DELETE_UNIQUE                   = 7;
    public static final int SQL_DELETE_NON_UNIQUE               = 8;
    public static final int SQL_CONTROL                         = 9;
    public static final int SQL_SET_TRANSACTION                 = 10;
    public static final int SQL_SET_CATALOG                     = 11;
    public static final int SQL_SET_SCHEMA                      = 12;

    public static final short TYPE_UNKNOWN                      = 0;
    public static final short TYPE_SELECT                       = 0x0001;
    public static final short TYPE_UPDATE                       = 0x0002;
    public static final short TYPE_DELETE                       = 0x0004;
    public static final short TYPE_INSERT                       = 0x0008;
    public static final short TYPE_INSERT_PARAM                 = 0x0120; //Modified for CQDs filter from 0x0100 to 0x0120
    public static final short TYPE_EXPLAIN                      = 0x0010;
    public static final short TYPE_CREATE                       = 0x0020;
    public static final short TYPE_GRANT                        = 0x0040;
    public static final short TYPE_DROP                         = 0x0080;
    public static final short TYPE_CALL                         = 0x0800;
    public static final short TYPE_CONTROL                      = 0x0900;
    public static final short TYPE_CATOLOG                      = 0x1000;

    public static final short TYPE_BLOB                         = 2004;
    public static final short TYPE_CLOB                         = 2005;

    public static final int SQL_ATTR_CURSOR_HOLDABLE            = -3;
    public static final int SQL_ATTR_INPUT_ARRAY_MAXSIZE        = -2;
    public static final int SQL_ATTR_QUERY_TYPE                 = -4;
    public static final int SQL_ATTR_ROWSET_ATOMICITY           = -5;
    public static final int SQL_ATTR_NOT_ATOMIC_FAILURE_LIMIT   = -6;
    public static final int SQL_ATTR_XN_NEEDED                  = -7;
    public static final int SQL_ATTR_UNIQUE_STMT_ID             = -8;
    public static final int SQL_ATTR_UNIQUE_QUERY_ID            = -8;
    public static final int SQL_ATTR_MAX_RESULT_SETS            = -9;
    public static final int SQL_ATTR_UNIQUE_STMT_ID_NO_DIAGS    = -10;
    public static final int SQL_ATTR_RS_PROXY_SYNTAX            = -11;
    public static final int SQL_ATTR_CONSUMER_QUERY_TEXT        = -12;
    public static final int SQL_ATTR_CONSUMER_CPU               = -13;
    public static final int SQL_ATTR_COPY_STMT_ID_TO_DIAGS      = -14;
    public static final int SQL_ATTR_PARENT_QID                 = -15;
    public static final int SQL_ATTR_CURSOR_UPDATABLE           = -16;
    public static final int SQL_ATTR_SUBQUERY_TYPE              = -17;

    /** shell size in bytes */
    public static final int OBJECT_SHELL_SIZE                   = 8;
    public static final int OBJREF_SIZE                         = 4;
    public static final int LONG_FIELD_SIZE                     = 8;
    public static final int INT_FIELD_SIZE                      = 4;
    public static final int SHORT_FIELD_SIZE                    = 2;
    public static final int CHAR_FIELD_SIZE                     = 2;
    public static final int BYTE_FIELD_SIZE                     = 1;
    public static final int BOOLEAN_FIELD_SIZE                  = 1;
    public static final int DOUBLE_FIELD_SIZE                   = 8;
    public static final int FLOAT_FIELD_SIZE                    = 4;

    public static final int SERVER_STATUS_DELAY                 = 30000;

    // declarations for processing metadata
    public static final short SQL_API_SQLGETTYPEINFO = 47;
    public static final short SQL_API_SQLCOLUMNS = 40;
    public static final short SQL_API_SQLSPECIALCOLUMNS = 52;
    public static final short SQL_API_SQLSTATISTICS = 53;
    public static final short SQL_API_SQLTABLES = 54;
    public static final short SQL_API_SQLCOLUMNPRIVILEGES = 56;
    public static final short SQL_API_SQLFOREIGNKEYS = 60;
    // public static final short SQL_API_TBLSYNONYM = 63;
    // //dbscripts_mv_synonym
    // public static final short SQL_API_TBLMVS = 64; //dbscripts_mv_synonym
    public static final short SQL_API_SQLPRIMARYKEYS = 65;
    public static final short SQL_API_SQLPROCEDURECOLUMNS = 66;
    public static final short SQL_API_SQLPROCEDURES = 67;
    public static final short SQL_API_SQLTABLEPRIVILEGES = 70;
    public static final short SQL_API_TBLSYNONYM = 1917; // dbscripts_mv_synonym
    public static final short SQL_API_TBLMVS = 1918; // dbscripts_mv_synonym

    public static final short SQL_API_JDBC = 9999;
    public static final short SQL_API_SQLTABLES_JDBC = (short) (SQL_API_SQLTABLES + SQL_API_JDBC);
    public static final short SQL_API_SQLCOLUMNS_JDBC = (short) (SQL_API_SQLCOLUMNS + SQL_API_JDBC);
    public static final short SQL_API_SQLSPECIALCOLUMNS_JDBC = (short) (SQL_API_SQLSPECIALCOLUMNS + SQL_API_JDBC);
    public static final short SQL_API_SQLGETTYPEINFO_JDBC = (short) (SQL_API_SQLGETTYPEINFO + SQL_API_JDBC);

    // values of NULLABLE field in descriptor
    public static final long SQL_NO_NULLS = 0;
    public static final long SQL_NULLABLE = 1;

    // Reserved values for UNIQUE argument of SQLStatistics()
    public static final int SQL_INDEX_UNIQUE = 0;
    public static final int SQL_INDEX_ALL = 1;

    // Column types and scopes in SQLSpecialColumns.
    public static final int SQL_BEST_ROWID = 1;
    public static final int SQL_ROWVER = 2;

}

