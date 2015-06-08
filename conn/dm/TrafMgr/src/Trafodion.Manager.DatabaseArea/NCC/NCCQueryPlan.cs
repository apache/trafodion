//
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2007-2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
//

using System;
using System.Collections;
using System.Data;
using System.Data.Odbc;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.RegularExpressions;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

//Ported from NPA SQ R1.0. 
//Provides functionality to retrieve query explain plans, set schemas, and
//set Controls 
namespace Trafodion.Manager.DatabaseArea.NCC
{
    /// <summary>
    /// The model object of getting explain plan and table stats. 
    /// </summary>
    public class NCCQueryPlan
    {

        #region Fields

        /// <summary>
        /// Definition of constants
        /// </summary>
		public const String TRAFODION_SQL_PREPARE_ERROR_PREFIX = "ERROR[";
		public const int    TRAFODION_MAX_ERRORS_TO_DISPLAY = 10;
        public const String TRAFODION_QLIST_NAMESPACE_PREFIX = "TrafodionManager:";
        public const String TRAFODION_QLIST_UNFILED_NAMESPACE = TRAFODION_QLIST_NAMESPACE_PREFIX + "Unfiled Entries";
        public const String TRAFODION_QLIST_TRIAGE_NAMESPACE = TRAFODION_QLIST_NAMESPACE_PREFIX + "Triage Space";
        public const String TRACE_SUB_AREA_NAME = "Query Plan";


        /// <summary>
        ///  For keep tracking the current session state
        /// </summary>
        enum DBConnectionState { INITIALIZED = 0, NOT_CONNECTED, CONNECTED, DISCONNECTED, SERVER_DIED  };

        // Current session state
        private DBConnectionState _connectionState = DBConnectionState.INITIALIZED;

        // Pointers for all of the handles for ODBC native operations
		private IntPtr sql_dbc_handle = IntPtr.Zero;
		private IntPtr sql_env_handle = IntPtr.Zero;
		private IntPtr _stmtHandle = IntPtr.Zero;
        private IntPtr _lastExecuteHandle = IntPtr.Zero;
        private OdbcConnection _theOdbcConnection = null;
        private Connection _theConnection = null;
        #endregion Fields

        #region Constructor
        /// <summary>
        /// NCCQueryPlan constructor
        /// </summary>
        /// <param name="anOdbcConnection"></param>
        //public NCCQueryPlan(OdbcConnection anOdbcConnection)
        //{
        //    _theOdbcConnection = anOdbcConnection;
        //}
        public NCCQueryPlan(Connection aConnection)
        {
            _theConnection = aConnection;
            _theOdbcConnection = aConnection.OpenNonCachedOdbcConnection;
        }

        /// <summary>
        /// The destructor
        /// </summary>
		~NCCQueryPlan() 
        {
        }

        #endregion Constructor

        #region Public Methods

        /// <summary>
        /// Calls SQL and retrieves the Explain plan for a given query string.
        ///    Multipurpose function gets an explain plan, creates and loads up a query data object if needed, 
        ///    prepares the summary and steps hash table for the the query tree control.  This function will
        ///    cache the data.  
        ///    The error is reported via errorStr for the error encountered while getting explain plan.  
        ///    The tableStatsErrorStr reports the accumulated error reported while retrieving table stats. 
        ///    Note that getting explain plan and table stats are two different distinct operations while table stats 
        ///    is retrieved after the plan is retrieved.  Therefore, you might have got the plan perfectly while 
        ///    encountering error while getting table stats. 
        /// </summary>
        /// <param name="queryofInterest">query of interest to explain</param>
        /// <param name="errorStr">returns error string</param>
        /// <param name="tableStatsErrorStr">returns error string while retrieving table stats</param>
        /// <returns>NCCWorkbenchQueryData</returns>
        public NCCWorkbenchQueryData GetExplainPlan(String queryofInterest, out String errorStr, out String tableStatsErrorStr, bool fetchTableStats)
        {
            return GetExplainPlan(queryofInterest, out errorStr, out tableStatsErrorStr, null, fetchTableStats);
        }
        public NCCWorkbenchQueryData GetExplainPlan(String queryofInterest, out String errorStr, out String tableStatsErrorStr, Trafodion.Manager.Framework.WorkerThread aWorker)
        {
            return GetExplainPlan(queryofInterest, out errorStr, out tableStatsErrorStr, aWorker, true);
        }

        public NCCWorkbenchQueryData GetExplainPlan(String queryofInterest, out String errorStr, out String tableStatsErrorStr, Trafodion.Manager.Framework.WorkerThread aWorker, bool fetchTableStats)
        {
            ArrayList planDataArray = new ArrayList();
            string tableStatsError = "";

            NCCWorkbenchQueryData wbqd = new NCCWorkbenchQueryData(TRAFODION_QLIST_UNFILED_NAMESPACE, 100000);//this._workspace.Options.FetchLimit);

            planDataArray = GetPlan(queryofInterest, out errorStr);

            //Reset the error string because some things rely in it being reset
            wbqd.planErrorString = null;

            //Throw an exception if there is an error
            if (!String.IsNullOrEmpty(errorStr))
            {
                throw new Exception(errorStr);
            }

            if (IsCancelled(aWorker))
            {
                throw new Exception("The user has cancelled the operation.");
            }

            //Error checking
            if (planDataArray.Count == 0)
            {
                wbqd.planErrorString = errorStr;
                tableStatsErrorStr = "";
                wbqd.FetchTableStatsError = tableStatsError;
                wbqd.FetchPlanError = errorStr;
                return wbqd;
            }

            wbqd.QueryPlanArray.Clear();
            //Cache away the explain data
            for (int i = 0; i != planDataArray.Count; i++)
            {
                //Load the object with the a plan item
                wbqd.queryPlanData = (NCCWorkbenchQueryData.QueryPlanData)planDataArray[i];

                //Store the plan item in the plan array
                wbqd.QueryPlanArray.Add(wbqd.queryPlanData);
            }

            //Prepare tree steps
            wbqd.SavePlanSteps(planDataArray);

            //Save summary info
            wbqd.CreateSummary(planDataArray);

            // Also get the control query shape if there were no errors. 
            wbqd.controlQueryShape = null;

            if (String.IsNullOrEmpty(errorStr))
                wbqd.controlQueryShape = GetQueryShape(queryofInterest);

            if (fetchTableStats && String.IsNullOrEmpty(errorStr))
            {
                foreach (NCCWorkbenchQueryData.QueryPlanData qpd in wbqd.QueryPlanArray)
                {
                    String theTableName = qpd.GetTableName();
                    bool cancelled = IsCancelled(aWorker);
                    //Console.WriteLine(DateTime.Now + "- Cancel flag status - " + theTableName + "-" + cancelled);
                    if (cancelled)
                    {
                        throw new Exception("The user has cancelled the operation.");
                    }

                    if (!string.IsNullOrEmpty(theTableName))
                    {
                        string error = "";
                        GetTableStatisticsData(theTableName, out error, wbqd);
                        tableStatsError += error;
                    }
                }
            }

            wbqd.FetchTableStatsError = tableStatsError;
            wbqd.FetchPlanError = errorStr;
            tableStatsErrorStr = tableStatsError;
            wbqd.SkipTableStats = !fetchTableStats;
            return wbqd;
        }

        #endregion Public methods

        #region Private methods

        private bool IsCancelled(Trafodion.Manager.Framework.WorkerThread aWorker)
        {
            if (aWorker != null)
            {
                return aWorker.CancellationPending;
            }
            return false;
        }

        /// <summary>
        /// Setting Connection handles
        /// </summary>
        private void Connect()
        {
            sql_env_handle = GetSQLEnvironmentHandle();

            sql_dbc_handle = OdbcAccess.GetDBConnectHandle(_theOdbcConnection);

            if (sql_dbc_handle != IntPtr.Zero)
            {
                _connectionState = DBConnectionState.CONNECTED;
            }
        }

        /// <summary>
        /// Get values from row info in 64-bit number format
        /// </summary>
        /// <param name="rowInfo"></param>
        /// <param name="colName"></param>
        /// <returns></returns>
        private Object GetValueAs64BitNumber(Hashtable rowInfo, String colName)
        {
            Object theValue = rowInfo[colName];
            try
            {
                UInt64 intVal = UInt64.Parse(theValue.ToString());
                theValue = intVal;
            }
            catch (Exception)
            {
            }

            return theValue;
        }

        /// <summary>
        /// Get table stats data
        /// </summary>
        /// <param name="tableName">table name</param>
        /// <param name="errorStr">reporting any error</param>
        /// <param name="wbqd">where table stats info will be stored</param>
        /// <returns></returns>
        private bool GetTableStatisticsData(String tableName, out String errorStr, NCCWorkbenchQueryData wbqd)
        {
            if (string.IsNullOrEmpty(tableName))
            {
                errorStr = "";
                return false;
            }

            // Skip retriving table stats for metadata tables, it is not supported.
            if (Trafodion.Manager.DatabaseArea.Model.TrafodionName.IsMetadataObject(_theConnection, tableName))
            {
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(
                        TraceOptions.TraceOption.DEBUG,
                        TraceOptions.TraceArea.Database,
                        TRACE_SUB_AREA_NAME,
                        "Skip table stats for metadata table: " + tableName);
                }

                errorStr = "";
                return false;
            }

            if (wbqd.TableDetailsHashtable.Contains(tableName))
            {
                try
                {
                    DateTime now = DateTime.Now.ToUniversalTime();
                    DateTime lastUpdateTime = (DateTime)wbqd.LastTableDetailUpdateHashtable[tableName];
                    TimeSpan timeDiff = now.Subtract(lastUpdateTime);
                    TimeSpan defaultCacheTime = TimeSpan.FromMinutes(30);
                    if (defaultCacheTime.TotalSeconds >= timeDiff.TotalSeconds)
                    {
                        errorStr = "";
                        return true;
                    }
                }
                catch (Exception)
                {
                }
            }

            String showStatsErrMsg = "";
            String showStatsQry = "SHOWSTATS  for table " + tableName + " on every column";

            try
            {
                DataTable dt = ExecuteQuery(showStatsQry, -1, false, out showStatsErrMsg);
                if (showStatsErrMsg.Contains("ERROR[4193] The schema name prefix VOLATILE_SCHEMA"))
                {
                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(
                            TraceOptions.TraceOption.DEBUG,
                            TraceOptions.TraceArea.Database,
                            TRACE_SUB_AREA_NAME,
                            "Error retrieving table statistics: " + tableName + " Details: " + showStatsErrMsg);
                    }
                    errorStr = "";
                    return false;
                }

                if (showStatsErrMsg.Contains("ERROR[9205] UPDATE STATISTICS is not supported"))
                {
                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(
                            TraceOptions.TraceOption.DEBUG,
                            TraceOptions.TraceArea.Database,
                            TRACE_SUB_AREA_NAME,
                            "Error retrieving table statistics: " + tableName + " Details: " + showStatsErrMsg);
                    }
                    errorStr = "";
                    return false;
                }

                if (!String.IsNullOrEmpty(showStatsErrMsg))
                {
                    String theMessage = "Error retrieving table statistics." + Environment.NewLine +
                                        "\t Problem:  Unable to get table statistics." + Environment.NewLine +
                                        "\t Solution: Please check error details for recovery information." + Environment.NewLine +
                                        "\t Details:  Table = " + tableName + Environment.NewLine +
                                        "\t\t message = " + showStatsErrMsg + Environment.NewLine;
                    errorStr = theMessage;
                    return false;
                }

                ArrayList tableInfo = new ArrayList();

                for (int i = 0; i < dt.Rows.Count; i++)
                {
                    foreach (string st in dt.Rows[i].ItemArray)
                    {
                        try
                        {
                            Regex r = new Regex(" +");
                            string[] stringARray = r.Split(st.TrimStart(' '));
                            if (stringARray.Length > 1)
                            {
                                string HistID = stringARray[0];
                                string Ints = stringARray[1];
                                string Rowcount = stringARray[2];
                                string UEC = stringARray[3];
                                string Colnames = stringARray[4];

                                for (int idx = 5; idx < stringARray.Length; idx++)
                                    Colnames += " " + stringARray[idx];

                                Hashtable rowInfo = new Hashtable();
                                rowInfo.Add("HistID", HistID);
                                rowInfo.Add("Ints", Ints);
                                rowInfo.Add("Rowcount", Rowcount);
                                rowInfo.Add("UEC", UEC);
                                rowInfo.Add("Colnames", Colnames);
                                tableInfo.Add(rowInfo);
                            }

                        }
                        catch (Exception e)
                        {
                            if (Logger.IsTracingEnabled)
                            {
                                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                                   TraceOptions.TraceArea.Database,
                                                   TRACE_SUB_AREA_NAME,
                                                   "NCC::getTableStatisticsData error parsing Row #" + i + ". Details = " + e.Message);
                            }
                        }
                    }

                }

                if (!wbqd.TableDetailsHashtable.Contains(tableName))
                {
                    wbqd.TableDetailsHashtable.Add(tableName, tableInfo);
                    wbqd.LastTableDetailUpdateHashtable.Add(tableName, DateTime.Now.ToUniversalTime());
                }

                errorStr = "";
                return true;
            }
            catch (Exception e)
            {
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "NCC::getTableStatisticsData error" + e.Message);
                }

                if (e.Message.Contains("ERROR[9205] UPDATE STATISTICS is not supported"))
                {
                    errorStr = "";
                }
                else
                {
                    errorStr = "Error retrieving table statistics." + Environment.NewLine + e.Message;
                }
            }

            errorStr = "";
            return false;
        }

        //private void GetListOfTrafodionSchemas() {
        //    if (DBConnectionState.CONNECTED != _connectionState)
        //        return;

        //    //  ODBC is flaky -- try thrice just to be sure.
        //    int retries = 3;
        //    bool displayErrors = false;
        //    while (0 < retries) {
        //        fetchSchemaNames(displayErrors);
        //        if (0 < this._schemaNames.Count)
        //            break;

        //        try {
        //            Thread.Sleep(1000 * (5 - retries));  // sleep for 2, 3 and 4 seconds.
        //        } catch (Exception) {
        //        }

        //        retries--;
        //        if (1 == retries)
        //            displayErrors = true;

        //    }

        //}

        #region ODBC DLL Imports.  Contains Native ODBC declarations

        public const short SQL_HANDLE_ENV  = 1;
        public const short SQL_HANDLE_DBC  = 2;
        public const short SQL_HANDLE_STMT = 3;
        public const ushort SQL_HANDLE_DESC = 4;
        public const ushort SQL_ATTR_LOGIN_TIMEOUT = 103;
		public const ushort SQL_ATTR_CURRENT_CATALOG = 109;
		public const ushort SQL_ATTR_CONNECTION_TIMEOUT = 113;
		public const ushort SQL_ATTR_QUERY_TIMEOUT = 0;
		public const short SQL_SUCCESS = 0;
		public const short SQL_ERROR = -1;
        public const short SQL_SUCCESS_WITH_INFO = 1;
        public const short SQL_NO_DATA = 100;
        public const int SQL_ATTR_ODBC_VERSION = 200;
        public const int SQL_OV_ODBC3 = 3;
        public const short SQL_FETCH_NEXT = 1;
        public const short SQL_FETCH_FIRST = 2;

        /* SQL data type codes */
        public const short SQL_UNKNOWN_TYPE =0;
        public const short SQL_CHAR = 1;
        public const short SQL_NUMERIC = 2;
        public const short SQL_DECIMAL = 3;
        public const short SQL_INTEGER = 4;
        public const short SQL_SMALLINT = 5;
        public const short SQL_FLOAT = 6;
        public const short SQL_REAL = 7;
        public const short SQL_DOUBLE = 8;

        /* SQL connect driver codes */
        public const uint SQL_DRIVER_NOPROMPT =  0;
        public const uint SQL_DRIVER_COMPLETE = 1;
        public const uint SQL_DRIVER_PROMPT = 2;
        public const uint SQL_DRIVER_COMPLETE_REQUIRED = 3;



        /*SQLRETURN SQLAllocHandle(
                                SQLSMALLINT HandleType,
                                SQLHANDLE InputHandle,
                                SQLHANDLE* OutputHandlePtr);*/

        [DllImport("odbc32.dll")]
		extern static short SQLAllocHandle(short HandleType, IntPtr InputHandle, out IntPtr OutputHandle);


        /*SQLRETURN SQLSetEnvAttr(
                                SQLHENV EnvironmentHandle,
                                SQLINTEGER Attribute,
                                SQLPOINTER ValuePtr,
                                SQLINTEGER StringLength);*/
        [DllImport("odbc32.dll")]
        extern static short SQLSetEnvAttr(IntPtr EnvironmentHandle, int attribute, IntPtr attrValue,
										  int stringLength);

       

       /*SQLRETURN SQLSetConnectAttr(
                                SQLHDBC ConnectionHandle,
                                SQLINTEGER Attribute,
                                SQLPOINTER ValuePtr,
                                SQLINTEGER StringLength); */
 
        [DllImport("odbc32.dll")]
        extern static short SQLSetConnectAttr(IntPtr ConnectionHandle, int Attribute,
											  String ValuePtr, int length);

		/* SQLRETURN SQLSetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, 
		 							SQLPOINTER ValuePtr, SQLINTEGER StringLength);
		*/
		[DllImport("odbc32.dll")]
		extern static short SQLSetStmtAttr(IntPtr StatementHandle, int Attribute, int ValuePtr, int length);

        
      /*  SQLRETURN SQLConnect(
                                SQLHDBC ConnectionHandle,
                                SQLCHAR* ServerName,
                                SQLSMALLINT NameLength1,
                                SQLCHAR* UserName,
                                SQLSMALLINT NameLength2,
                                SQLCHAR* Authentication,
                                SQLSMALLINT NameLength3); */

        [DllImport("odbc32.dll")]
        extern static short SQLConnect(
                                IntPtr ConnectionHandle,
								[MarshalAs(UnmanagedType.LPStr)] String ServerName,
                                int ServerNameLength,
							    [MarshalAs(UnmanagedType.LPStr)] String UserName,
                                int UserNameLength,
							    [MarshalAs(UnmanagedType.LPStr)] String Authentication,
                                int AuthenticationLength);
       
        
        /* SQLRETURN SQLDriverConnect(
                                SQLHDBC     ConnectionHandle,
                                SQLHWND     WindowHandle,
                                SQLCHAR *     InConnectionString,
                                SQLSMALLINT     StringLength1,
                                SQLCHAR *     OutConnectionString,
                                SQLSMALLINT     BufferLength,
                                SQLSMALLINT *     StringLength2Ptr,
                                SQLUSMALLINT     DriverCompletion);*/


        [DllImport("odbc32.dll")]
        extern static short SQLDriverConnect(
                                IntPtr ConnectionHandle,
                                IntPtr WindowHandle,
								[MarshalAs(UnmanagedType.LPStr)] StringBuilder InConnectionString,
                                int StringLength1,
							    [MarshalAs(UnmanagedType.LPStr)] StringBuilder OutConnectionString,
                                int BufferLength,
                                out int StringLength2Ptr,
                                uint DriverCompletion);



		[DllImport("odbc32.dll")]
		extern static short SQLDisconnect(IntPtr ConnectionHandle);




       /* SQLRETURN SQLPrepare(
                                SQLHSTMT StatementHandle,
                                SQLCHAR* StatementText,
                                SQLINTEGER TextLength); */


        [DllImport("odbc32.dll")]
        extern static short SQLPrepare(IntPtr StatementHandle,
									   [MarshalAs(UnmanagedType.LPStr)] StringBuilder StatementText,
									   int TextLength);



        
        /*SQLRETURN SQLExecute( SQLHSTMT StatementHandle);*/

        [DllImport("odbc32.dll")]
        extern static short SQLExecute(IntPtr StatementHandle);



        /*SQLRETURN SQLExecDirect(
                                 SQLHSTMT StatementHandle,
                                 SQLCHAR* StatementText,
                                 SQLINTEGER TextLength);
        */
        [DllImport("odbc32.dll")]
        extern static short SQLExecDirect(IntPtr StatementHandle,
										  [MarshalAs(UnmanagedType.LPStr)] StringBuilder StatementText,
										  int TextLength);

		/*
		 *  SQLRETURN SQLNumResultCols(SQLHSTMT StatementHandle,
		 *							   SQLSMALLINT* ColumnCountPtr);
		 */
		[DllImport("odbc32.dll")]
		extern static short SQLNumResultCols(IntPtr StatementHandle,
											 out int ColumnCount);


		/*
		 *  SQLRETURN SQLDescribeCol(SQLHSTMT     StatementHandle,
		 *                           SQLSMALLINT     ColumnNumber,
		 * 		                     SQLCHAR *     ColumnName,
		 * 							 SQLSMALLINT     BufferLength,
		 * 							 SQLSMALLINT *     NameLengthPtr,
		 *                           SQLSMALLINT *     DataTypePtr,
		 * 	                         SQLULEN *     ColumnSizePtr,
		 * 	                         SQLSMALLINT *     DecimalDigitsPtr,
		 * 	                         SQLSMALLINT *     NullablePtr);
		 */
		[DllImport("odbc32.dll")]
		extern static short SQLDescribeCol(IntPtr StatementHandle,
										   int ColumnNumber,
										   [MarshalAs(UnmanagedType.LPStr)] StringBuilder ColumnName,
										   int BufferLength,
										   out int NameLength,
										   out int DataType,
										   out uint ColumnSize,
										   out int DecimalDigits,
										   out int Nullable);


        /*SQLRETURN SQLFetch(
                                 SQLHSTMT StatementHandle);*/
        
        [DllImport("odbc32.dll")]
        extern static short SQLFetch(IntPtr StatementHandle);

        
        /*SQLRETURN SQLGetData(
                                 SQLHSTMT StatementHandle,
                                 SQLUSMALLINT ColumnNumber,
                                 SQLSMALLINT TargetType,
                                 SQLPOINTER TargetValuePtr,
                                 SQLINTEGER BufferLength,
                                 SQLINTEGER* StrLen_or_IndPtr); */


        [DllImport("odbc32.dll")]
        extern static short SQLGetData(
                                 IntPtr StatementHandle,
                                 ushort ColumnNumber,
                                 int TargetType,
                                 out long TargetValuePtr,
                                 int BufferLength,
                                 out int StrLen);
        
        
        [DllImport("odbc32.dll")]
        extern static short SQLGetData(
                                 IntPtr StatementHandle,
                                 ushort ColumnNumber,
                                 int TargetType,
                                 out int TargetValuePtr,
                                 int BufferLength,
                                 out int StrLen);

        [DllImport("odbc32.dll")]
        extern static short SQLGetData(
                                 IntPtr StatementHandle,
                                 ushort ColumnNumber,
                                 short TargetType,
                                 StringBuilder TargetValuePtr,
                                 int BufferLength,
                                 out int StrLen);


		/*
		 * SQLRETURN SQLCloseCursor(SQLHSTMT StatementHandle);
		 */
		[DllImport("odbc32.dll")]
		extern static short SQLCloseCursor(IntPtr StatementHandle);

        /*
        SQLRETURN SQLGetCursorName(
               SQLHSTMT StatementHandle,  /  * hstmt * /
               SQLCHAR* CursorName,      /  * szCursor * /
               SQLSMALLINT BufferLength,     / * cbCursorMax * /
               SQLSMALLINT* NameLengthPtr);  / * pcbCursor * /
         * 
         */
        [DllImport("odbc32.dll")]
        extern static short SQLGetCursorName(IntPtr StatementHandle, StringBuilder TargetValuePtr,
											 short BufferLength, out short StrLen);


        /* SQLRETURN SQLCancel(SQLHSTMT StatementHandle);  */
        [DllImport("odbc32.dll")]
        extern static short SQLCancel(IntPtr StatementHandle);


        //SQLRETURN SQLFreeHandle(
        //                        SQLSMALLINT   HandleType,
        //                        SQLHANDLE     Handle);
        [DllImport("odbc32.dll")]
        static extern short SQLFreeHandle(short HandleType, IntPtr InputHandle);


       /*SQLRETURN SQLGetDiagRec(
                                 SQLSMALLINT HandleType,
                                 SQLHANDLE Handle,
                                 SQLSMALLINT RecNumber,
                                 SQLCHAR* Sqlstate,
                                 SQLINTEGER* NativeErrorPtr,
                                 SQLCHAR* MessageText,
                                 SQLSMALLINT BufferLength,
                                 SQLSMALLINT* TextLengthPtr); */

        [DllImport("odbc32.dll")]
        static extern short SQLGetDiagRec(short handleType, IntPtr Handle, short RecNumber,
										  StringBuilder Sqlstate, ref int NativeErrorPtr,
										  StringBuilder MessageText, short bufferlength,
										  ref short textLengthPtr);


		/*
		SQLRETURN SQLTables(SQLHSTMT     StatementHandle,
		 					SQLCHAR *     CatalogName,
							SQLSMALLINT     NameLength1,
							SQLCHAR *     SchemaName,
		 					SQLSMALLINT     NameLength2,
							SQLCHAR *     TableName,
							SQLSMALLINT     NameLength3,
							SQLCHAR *     TableType,
		 					SQLSMALLINT     NameLength4);
		*/
        [DllImport("odbc32.dll")]
        static extern short SQLTables(IntPtr StatementHandle, StringBuilder CatalogName, short NameLength1,
									  StringBuilder SchemaName, short NameLength2,
									  StringBuilder TableName, short NameLength3,
									  StringBuilder TableType, short NameLength4);

        #endregion

        /// <summary>
        /// Handle any ODBC Error with throwing an exception
        /// </summary>
        /// <param name="rc"></param>
        /// <param name="msg"></param>
		private void HandleODBCErrors(short rc, String msg) 
        {
            if ((SQL_SUCCESS != rc) && (SQL_SUCCESS_WITH_INFO != rc))
            {
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(
                        TraceOptions.TraceOption.DEBUG, 
                        TraceOptions.TraceArea.Database, 
                        TRACE_SUB_AREA_NAME,
                        "Odbc error " + msg + " - return code = " + rc);
                }

                throw new Exception(msg + " - return code = " + rc);
            }
		}

        /// <summary>
        /// Get an odbc environment handle
        /// </summary>
        /// <returns></returns>
        private IntPtr GetSQLEnvironmentHandle() 
        {
            short rc = 0;
            
            try {
                rc = SQLAllocHandle(SQL_HANDLE_ENV, IntPtr.Zero, out sql_env_handle);
				HandleODBCErrors(rc, "Failed to allocate SQL_HANDLE_ENV");
				
                rc = SQLSetEnvAttr(sql_env_handle, SQL_ATTR_ODBC_VERSION, (IntPtr)SQL_OV_ODBC3, 0);
                HandleODBCErrors(rc, "Failed to set ENV attribute SQL_ATTR_ODBC_VERSION");
			}
            catch (Exception e) {
                String errMsg = ErrorMessage(sql_env_handle,SQL_HANDLE_ENV, e.Message, false);

                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "NCC::GetSQLEnvironmentHandler(): " + errMsg);
                }

                throw new Exception("GetSQLEnvironmentHandle failed, error = " + errMsg);
            }
                               
            return sql_env_handle;
        }

        /// <summary>
        /// Get an odbc statement handle 
        /// </summary>
        /// <returns></returns>
		private IntPtr GetStatementHandle() 
        {
			IntPtr stmtHandle = IntPtr.Zero;
			short rc = 0;

			rc = SQLAllocHandle(SQL_HANDLE_STMT, sql_dbc_handle, out stmtHandle);
			HandleODBCErrors(rc, "Failed to allocate SQL_HANDLE_STMT");

            if (Logger.IsTracingEnabled)
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                   TraceOptions.TraceArea.Database,
                                   TRACE_SUB_AREA_NAME,
                                   "NCC::getStatementHandle(): Allocated Statement handle=" + stmtHandle);
            }

            rc = SQLSetStmtAttr(stmtHandle, SQL_ATTR_QUERY_TIMEOUT, GeneralOptions.GetOptions().ConnectionTimeOut, 0);
			HandleODBCErrors(rc, "Failed to set statement attribute SQL_ATTR_QUERY_TIMEOUT");
            _lastExecuteHandle = stmtHandle;
			return stmtHandle;
		}

        /// <summary>
        /// Free an odbc statement handle
        /// </summary>
        /// <param name="stmtHandle"></param>
        /// <param name="msgPrefix"></param>
		private void FreeStatementHandle(IntPtr stmtHandle, String msgPrefix) 
        {
			short rc = 0;

			try {
				if (IntPtr.Zero != stmtHandle) {
					rc = SQLFreeHandle(SQL_HANDLE_STMT, stmtHandle);
					HandleODBCErrors(rc, "Failed to deallocate statement handle [SQLFreeHandle]");
                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                           TraceOptions.TraceArea.Database,
                                           TRACE_SUB_AREA_NAME,
                                           "NCC::freeStatementHandle(): " + msgPrefix + " - Freed statement handle " + stmtHandle);
                    }
				}

			} catch (Exception e) {
				String errMsg = ErrorMessage(stmtHandle, SQL_HANDLE_STMT, e.Message, false);
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "NCC::freeStatementHandle(): " + msgPrefix + " - " + errMsg);
                }
			}

		}

        ///// <summary>
        ///// Set schema
        ///// </summary>
        ///// <param name="schemaName"></param>
        ///// <param name="displayErrors"></param>
        //public void SetSchema(String schemaName, bool displayErrors)
        //{
        //    IntPtr stmtHandle = IntPtr.Zero;
        //    short rc = 0;

        //    //  Do not do a set schema if schema restrictions are enabled.
        //    if (SchemaRestrictions)
        //    {
        //        return;
        //    }

        //    try 
        //    {
        //        // Allocate and execute the set schema statement if needed.
        //        if ((null != schemaName)  &&  (0 < schemaName.Trim().Length)  &&
        //             (false == this._lastSetSchema.Equals(schemaName.Trim().ToUpper())) ) {
        //            stmtHandle = GetStatementHandle();

        //            String escapedName = NCCUtils.convertSchemaForI18NForm(schemaName);
        //            StringBuilder schemaSB = new StringBuilder("set schema " + escapedName + " ");
        //            schemaSB.Length = schemaSB.Length * Math.Max(1, Marshal.SystemDefaultCharSize);

        //            if (Logger.IsTracingEnabled)
        //            {
        //                Logger.OutputToLog(TraceOptions.TraceOption.SQL,
        //                                   TraceOptions.TraceArea.Database,
        //                                   "SQLExecDirect: " + schemaSB.ToString());
        //            }

        //            rc = SQLExecDirect(stmtHandle, schemaSB, schemaSB.Length);
        //            HandleODBCErrors(rc, "Failed to set schema [SQLExecDirect]");

        //            this._lastSetSchema = schemaName.Trim().ToUpper();
        //        }

        //    }
        //    catch (Exception e) 
        //    {
        //        String errMsg = ErrorMessage(stmtHandle, SQL_HANDLE_STMT, e.Message, displayErrors);
        //        if (Logger.IsTracingEnabled)
        //        {
        //            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
        //                               TraceOptions.TraceArea.Database,
        //                               "NCC::SetSchema(): Failed to set current schema. " + "Error details = " + errMsg);
        //        }
        //    }
        //    finally 
        //    {
        //        FreeStatementHandle(stmtHandle, "In setSchema");
        //    }

        //}

        ///// <summary>
        ///// Set controls
        ///// </summary>
        ///// <param name="controlSettings"></param>
        ///// <param name="displayErrors"></param>
        //private void SetControls(ArrayList controlSettings, bool displayErrors)
        //{
        //    IntPtr stmtHandle = IntPtr.Zero;
        //    short rc = 0;

        //    try 
        //    {
        //        stmtHandle = GetStatementHandle();

        //        for (int i = 0; controlSettings.Count > i; i++) {
        //            StringBuilder controlStatement = new StringBuilder(controlSettings[i].ToString() );
        //            controlStatement.Length = controlStatement.Length *
        //                                      Math.Max(1, Marshal.SystemDefaultCharSize);

        //            if (Logger.IsTracingEnabled)
        //            {
        //                Logger.OutputToLog(TraceOptions.TraceOption.SQL,
        //                                   TraceOptions.TraceArea.Database,
        //                                   "SQLExecDirect: " + controlStatement.ToString());
        //            }

        //            rc = SQLExecDirect(stmtHandle, controlStatement, controlStatement.Length);
        //            HandleODBCErrors(rc, "Failed to execute control [SQLExecDirect]");
        //        }

        //    } 
        //    catch (Exception e) 
        //    {
        //        String errMsg = ErrorMessage(stmtHandle, SQL_HANDLE_STMT, e.Message, displayErrors);
        //        if (Logger.IsTracingEnabled)
        //        {
        //            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
        //                               TraceOptions.TraceArea.Database,
        //                               "NCC::SetControls(): Failed to set controls. Error details = " + errMsg);
        //        }

        //    } 
        //    finally 
        //    {
        //        FreeStatementHandle(stmtHandle, "In setControls");
        //    }

        //}

        /// <summary>
        /// Get Query shapes
        /// </summary>
        /// <param name="queryString"></param>
        /// <returns></returns>
        private String GetQueryShape(String queryString)
        {
			IntPtr showShapeStmtHandle = IntPtr.Zero;
			short rc = 0;

			try {
				showShapeStmtHandle = GetStatementHandle();

				StringBuilder showShapeStatement_sb = new StringBuilder("SHOWSHAPE ");
				showShapeStatement_sb.Append(queryString);

				showShapeStatement_sb.Length = showShapeStatement_sb.Length *
											   Math.Max(1, Marshal.SystemDefaultCharSize);

				rc = SQLExecDirect(showShapeStmtHandle, showShapeStatement_sb, showShapeStatement_sb.Length);
				HandleODBCErrors(rc, "Failed to get query shape [SQLExecDirect]");

				int outputLen;
				StringBuilder showShapeOutput = new StringBuilder(1024);
				StringBuilder outputLine = new StringBuilder(1024);

				while (true) 
                {
					rc = SQLFetch(showShapeStmtHandle);
                    if (SQL_ERROR == rc)
                    {
                        if (Logger.IsTracingEnabled)
                        {
                            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                               TraceOptions.TraceArea.Database,
                                               TRACE_SUB_AREA_NAME,
                                               "Failed to get query shape [SQLFetch], return code = " + rc);
                        }

                        throw new Exception("Failed to get query shape [SQLFetch], return code = " + rc);
                    }

                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                           TraceOptions.TraceArea.Database,
                                           TRACE_SUB_AREA_NAME,
                                           "NCC::getQueryShape(): Fetching query shape handle=" + showShapeStmtHandle + ", return code = " + rc);
                    }

                    if ((SQL_SUCCESS == rc) || (SQL_SUCCESS_WITH_INFO == rc))
                    {
                        rc = SQLGetData(showShapeStmtHandle, 1, SQL_CHAR, outputLine,
                                        outputLine.Capacity, out outputLen);
                        HandleODBCErrors(rc, "Failed to get query shape line [SQLGetData]");

                        String trimmedSpaceLine = outputLine.ToString().Trim() + " ";
                        showShapeOutput.Append(trimmedSpaceLine);
                    }
                    else
                    {
                        break;
                    }
				}


				String qryShape = showShapeOutput.ToString(); 
				qryShape = qryShape.Replace("control query shape ", "");
                if (qryShape.Trim().Equals("anything;"))
                {
                    qryShape = "";
                }

				return qryShape;

			} 
            catch (Exception e) 
            {
				String errorMsg = ErrorMessage(showShapeStmtHandle, SQL_HANDLE_STMT, e.Message, false);
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "NCC::getQueryShape(): Failed to get query shape. " + "Details = " + errorMsg);
                }

				return "Failed to get query shape. \nError details = " + errorMsg;
			}
            finally 
            {
				FreeStatementHandle(showShapeStmtHandle, "In getQueryShape");
			}

		}

        /// <summary>
        /// Get Plan
        /// </summary>
        /// <param name="queryString">query in which to retrieve explain plan for</param>
        /// <param name="errorStr">string to hold the error string</param>
        /// <returns></returns>
        private ArrayList GetPlan(String queryString, out String errorStr) {
			IntPtr exp_stmt_handle = IntPtr.Zero;
            short rc = 0;

			//Temp to hold plan data that gets returned to caller
			ArrayList planArray = new ArrayList();

			errorStr = "";

            if (DBConnectionState.CONNECTED != _connectionState)
                Connect();

            try 
            {
				// Allocate SQL statement handle for the query.
                if (IntPtr.Zero == this._stmtHandle)
                {
                    this._stmtHandle = GetStatementHandle();
                }

                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "NCC::GetPlan(): Using Statement handle=" + _stmtHandle);
                }

                StringBuilder explainSB = new StringBuilder();
                if (queryString.StartsWith("QID="))
                {
                    explainSB.Append("SELECT * FROM TABLE(explain(null, '");
                    explainSB.Append(queryString).Append("')) ");
                }
                else
                {

                    StringBuilder prepareStatement = new StringBuilder(queryString);
				    prepareStatement.Length = prepareStatement.Length *
										      Math.Max(1, Marshal.SystemDefaultCharSize);

                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.SQL,
                                           TraceOptions.TraceArea.Database,
                                           TRACE_SUB_AREA_NAME,
                                           "SQLPrepare: Length=" + prepareStatement.Length + ", sql=" + prepareStatement);
                    }

				    // Prepare the statement.
                    rc = SQLPrepare(_stmtHandle, prepareStatement, prepareStatement.Length);
				    HandleODBCErrors(rc, "Failed to prepare query [SQLPrepare]. Unable to retrieve explain plan");

                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                           TraceOptions.TraceArea.Database,
                                           TRACE_SUB_AREA_NAME,
                                           "NCC::GetPlan(): Successfully prepared the query.");
                    }
				    // Get the cursor/statement name.
				    StringBuilder cursorName = new StringBuilder(1024);
				    short cNameLength;
				    rc = SQLGetCursorName(_stmtHandle, cursorName, 1000, out cNameLength);
				    HandleODBCErrors(rc, "Failed to get cursor name [SQLGetCursorName]");

                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                           TraceOptions.TraceArea.Database,
                                           TRACE_SUB_AREA_NAME,
                                           "NCC::GetPlan(): Prepared statement label = " + cursorName);
                    }

				    // Build the explain statement.
                    explainSB.Append("SELECT * FROM TABLE(explain(null, '");
                    explainSB.Append(cursorName.ToString()).Append("')) ");
                }

				explainSB.Length = explainSB.Length * Math.Max(1, Marshal.SystemDefaultCharSize);

                // Allocate explain statement handle.
                exp_stmt_handle = GetStatementHandle();
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "NCC::GetPlan(): Explain statement handle = " + exp_stmt_handle);
                }


                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.SQL,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "SQLExecDirect: " + explainSB.ToString());
                }

                rc = SQLExecDirect(exp_stmt_handle, explainSB , explainSB.Length);
				HandleODBCErrors(rc, "Failed explaining the query [SQLExecDirect]");

                int length;
                StringBuilder dataString = new StringBuilder(100);

				while(true) 
                {
					rc = SQLFetch(exp_stmt_handle);
					if (SQL_ERROR == rc)
						throw new Exception("Failed to fetch explain plan output [SQLFetch], " +
											"return code = " + rc);

                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                           TraceOptions.TraceArea.Database,
                                           TRACE_SUB_AREA_NAME,
                                           "NCC::GetPlan(): Fetching explain plan output [SQLFetch], handle=" + exp_stmt_handle + ", return code = " + rc);
                    }

                    if ((SQL_SUCCESS == rc) || (SQL_SUCCESS_WITH_INFO == rc))
                    {
                        StringBuilder m_moduleName = new StringBuilder(512);        //Column 1
                        StringBuilder m_statementName = new StringBuilder(512);     //Column 2
                        StringBuilder m_planID = new StringBuilder(100);            //Column 3
                        StringBuilder m_sequenceNumber = new StringBuilder(100);    //column 4
                        StringBuilder m_operator = new StringBuilder(512);          //Column 5
                        StringBuilder m_leftChildSeqNum = new StringBuilder(100);   //Column 6
                        StringBuilder m_rightChildSeqNum = new StringBuilder(100);  //Column 7
                        StringBuilder m_TableName = new StringBuilder(512);         //Column 8
                        StringBuilder m_cardinality = new StringBuilder(100);       //Column 9
                        StringBuilder m_operatorCost = new StringBuilder(100);      //Column 10
                        StringBuilder m_totalCost = new StringBuilder(100);         //Column 11
                        StringBuilder m_detailCost = new StringBuilder(512);        //Column 12
                        StringBuilder m_description = new StringBuilder(4096);      //Column 13

                        rc = SQLGetData(exp_stmt_handle, 1, SQL_CHAR, m_moduleName, m_moduleName.Capacity, out length);
                        rc = SQLGetData(exp_stmt_handle, 2, SQL_CHAR, m_statementName, m_statementName.Capacity, out length);
                        rc = SQLGetData(exp_stmt_handle, 3, SQL_CHAR, m_planID, m_planID.Capacity, out length);
                        rc = SQLGetData(exp_stmt_handle, 4, SQL_CHAR, m_sequenceNumber, m_sequenceNumber.Capacity, out length);
                        rc = SQLGetData(exp_stmt_handle, 5, SQL_CHAR, m_operator, m_operator.Capacity, out length);
                        rc = SQLGetData(exp_stmt_handle, 6, SQL_CHAR, m_leftChildSeqNum, m_leftChildSeqNum.Capacity, out length);
                        rc = SQLGetData(exp_stmt_handle, 7, SQL_CHAR, m_rightChildSeqNum, m_rightChildSeqNum.Capacity, out length);
                        rc = SQLGetData(exp_stmt_handle, 8, SQL_CHAR, m_TableName, m_TableName.Capacity, out length);
                        rc = SQLGetData(exp_stmt_handle, 9, SQL_CHAR, m_cardinality, m_cardinality.Capacity, out length);
                        rc = SQLGetData(exp_stmt_handle, 10, SQL_CHAR, m_operatorCost, m_operatorCost.Capacity, out length);
                        rc = SQLGetData(exp_stmt_handle, 11, SQL_CHAR, m_totalCost, m_totalCost.Capacity, out length);
                        rc = SQLGetData(exp_stmt_handle, 12, SQL_CHAR, m_detailCost, m_detailCost.Capacity, out length);
                        rc = SQLGetData(exp_stmt_handle, 13, SQL_CHAR, m_description, m_description.Capacity, out length);

                        HandleODBCErrors(rc, "Failed fetching explain plan output [SQLgetData]");

                        NCCWorkbenchQueryData.QueryPlanData qpd = new NCCWorkbenchQueryData.QueryPlanData();

                        qpd.sequenceNumber = m_sequenceNumber.ToString();
                        qpd.theOperator = m_operator.ToString().Trim();
                        qpd.operatorCost = m_operatorCost.ToString();
                        qpd.totalCost = m_totalCost.ToString();
                        qpd.cardinality = m_cardinality.ToString();
                        qpd.tableName = m_TableName.ToString();
                        qpd.leftChildSeqNum = m_leftChildSeqNum.ToString();
                        qpd.rightChildSeqNum = m_rightChildSeqNum.ToString();
                        qpd.detailCost = m_detailCost.ToString();
                        qpd.description = m_description.ToString();

                        string str_TableName = extractTableName(m_description.ToString());
                        if (!string.IsNullOrEmpty(str_TableName))
                        {
                            str_TableName = GetExternalTableName(str_TableName);
                        }
                        qpd.tableName = str_TableName;

                        String explainRow = String.Format("{0}|{1}|{2}|{3}|{4}|{5}|{6}|{7}|{8}|{9}",
                            new object[] {
                                qpd.sequenceNumber,
                                qpd.theOperator,
                                qpd.operatorCost,
                                qpd.totalCost,
                                qpd.cardinality,
                                qpd.tableName,
                                qpd.leftChildSeqNum,
                                qpd.rightChildSeqNum,
                                qpd.detailCost,
                                qpd.description
                            });
                        Console.WriteLine(explainRow);
                        planArray.Add(qpd);
                    }
                    else
                    {
                        break;
                    }
				} // wnile (true)
            }
            catch(Exception e) {
                if (IntPtr.Zero != exp_stmt_handle)
                {
                    errorStr = ErrorMessage(exp_stmt_handle, SQL_HANDLE_STMT, e.Message, false);
                }
                else
                {
                    errorStr = ErrorMessage(this._stmtHandle, SQL_HANDLE_STMT, e.Message, false);
                }

                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "NCC::GetPlan(): Failed to get query plan, details = " + errorStr);
                }
            } 
			finally 
            {
				//Now free the statement handles 
				FreeStatementHandle(exp_stmt_handle, "In GetPlan");
				if (0 < errorStr.Length) 
                {
					FreeStatementHandle(this._stmtHandle, "In GetPlan");
					this._stmtHandle = IntPtr.Zero;
				}
            }

			return planArray; 
        }

        /// <summary>
        /// Get the external name of the table.
        /// </summary>
        /// <param name="anAnsiName"></param>
        /// <returns></returns>
        public string GetExternalTableName(string anAnsiName)
        {
            anAnsiName = anAnsiName.Trim();
            if (string.IsNullOrEmpty(anAnsiName))
                return "";

            bool inQuotes = false;
            int theBeginOffset = 0;

            int theResultPart = 0;

            int theAnsiLength = anAnsiName.Length;

            string[] theResult = new string[3];

            for (int theCurrentOffset = 0; theCurrentOffset < theAnsiLength; theCurrentOffset++)
            {
                char theCharacter = anAnsiName[theCurrentOffset];

                switch (theCharacter)
                {
                    case '"':
                        {
                            inQuotes = !inQuotes;
                            break;
                        }
                    case '.':
                        {
                            if (!inQuotes)
                            {
                                theResult[theResultPart] = anAnsiName.Substring(theBeginOffset, theCurrentOffset - theBeginOffset);
                                theResultPart++;
                                theBeginOffset = theCurrentOffset + 1;
                            }
                            break;
                        }
                    default:
                        {
                            break;
                        }
                }

            }
            theResult[theResultPart] = anAnsiName.Substring(theBeginOffset);
            string externalTableName = "";

            for (int i = 0; i < theResult.Length; i++)
            {
                if (string.IsNullOrEmpty(theResult[i]))
                    continue;

                if (i < theResult.Length && !string.IsNullOrEmpty(externalTableName))
                {
                    externalTableName = externalTableName + ".";
                }

                if (System.Text.RegularExpressions.Regex.IsMatch(theResult[i], "^[A-Z0-9_]+$"))
                {
                    externalTableName = externalTableName + theResult[i];
                }
                else
                {
                    if (theResult[i].StartsWith("\"") && theResult[i].EndsWith("\""))
                    {
                        externalTableName = externalTableName + theResult[i];
                    }
                    else
                    {
                        theResult[i] = theResult[i].Replace("\"", "\"\"");
                        // It has specials; delimit it.
                        externalTableName = externalTableName + "\"" + theResult[i] + "\"";
                    }
                }

            }
            return externalTableName;
        }

        /// <summary>
        /// Extracts the table name from the explain description
        /// </summary>
        /// <param name="description"></param>
        /// <returns></returns>
        public string extractTableName(string description)
        {
            string tableName = "";
            int iud_type_pos = description.IndexOf("iud_type: ");
            if (iud_type_pos > -1)
            {
                //format for the iud_type is 
                //iud_type: <operator> <table name>
                tableName = description.Substring(iud_type_pos + 10);
                int theBeginOffset = 0;
                int tokenIndex = 0;
                for (int theCurrentOffset = 0; theCurrentOffset < tableName.Length; theCurrentOffset++)
                {
                    char theCharacter = tableName[theCurrentOffset];

                    switch (theCharacter)
                    {
                        case ' ':
                            {
                                string currentToken = tableName.Substring(theBeginOffset, theCurrentOffset - theBeginOffset);
                                theBeginOffset = theCurrentOffset + 1;
                                if (tokenIndex == 1)
                                {
                                    tableName = currentToken; //tablename is the the second token after the uid_type token.
                                    return tableName;
                                }
                                tokenIndex++;
                                break;
                            }
                        default:
                            break;
                    }
                }

            }
            else
            {
                int tpos = description.IndexOf(" of table ");
                int ipos = description.IndexOf(" of index ");
                int ppos = description.IndexOf('(');
                int start = -1;
                if ((tpos > 0 && tpos < ipos) || ipos < 1)
                {
                    if (tpos > -1)
                    {
                        start = tpos + 10;
                    }
                }
                else
                {
                    if (ppos > -1)
                    {
                        start = ppos + 1;
                    }
                }
                if (start > -1)
                {
                    tableName = description.Substring(start);

                    bool inQuotes = false;
                    int theBeginOffset = 0;
                    bool inDoubleDoubleQuotes = false;
                    for (int theCurrentOffset = 0; theCurrentOffset < tableName.Length; theCurrentOffset++)
                    {
                        char theCharacter = tableName[theCurrentOffset];

                        switch (theCharacter)
                        {
                            case '"':
                                {
                                    if (inQuotes)
                                    {
                                        if (tableName[theCurrentOffset + 1] != '"')
                                        {
                                            if (inDoubleDoubleQuotes)
                                            {
                                                inDoubleDoubleQuotes = !inDoubleDoubleQuotes;
                                            }
                                            else
                                            {
                                                inQuotes = !inQuotes;
                                            }
                                        }
                                        else
                                        {
                                            inDoubleDoubleQuotes = !inDoubleDoubleQuotes;
                                        }
                                    }
                                    else
                                    {
                                        inQuotes = !inQuotes;
                                    }
                                    break;
                                }
                            case ' ':
                            case ')':
                                {
                                    if (!inQuotes)
                                    {
                                        tableName = tableName.Substring(theBeginOffset, theCurrentOffset - theBeginOffset);
                                        theBeginOffset = theCurrentOffset + 1;
                                    }
                                    break;
                                }
                            default:
                                {
                                    break;
                                }
                        }

                    }
                }
            }
            return tableName;
        }
        
        /// <summary>
        /// -- Because the tname field fetch from queryPlan table can only store 60 characters, so if table name is more than 60 characters,
        /// the table name will be cut at the end, and so the cutted table name will cause generating problem
        /// The logic of this function is: check whether tname is >=60 characters, if yes, get the full value from description value which can store 4096 characters.
        /// </summary>
        /// <param name="tname"></param>
        /// <param name="description"></param>        
        /// <returns>tname (full value)</returns>
        private string GetTNameFromDescription(StringBuilder tname, StringBuilder description)
        {
            string str_tname = tname.ToString().Trim();
            string str_description = description.ToString().Trim();
            int startPozition = str_description.IndexOf(str_tname);
            if (startPozition < 0)
            {
                return str_tname;
            }

            int endPosition = str_description.IndexOf(" ", startPozition);
            if (endPosition > startPozition)
            {
                str_tname = str_description.Substring(startPozition, endPosition - startPozition);
            }
            return str_tname;
        }

        /// <summary>
        /// Execute a query
        /// </summary>
        /// <param name="queryString"></param>
        /// <param name="maxRows"></param>
        /// <param name="usePreparedStatement"></param>
        /// <param name="errorMsg"></param>
        /// <returns></returns>
		private DataTable ExecuteQuery(String queryString, int maxRows, bool usePreparedStatement, out String errorMsg) 
        {
			IntPtr stmtHandle = IntPtr.Zero;
			short rc = 0;
			DataTable outputDataTable = new DataTable();

			errorMsg = "";

            if (DBConnectionState.CONNECTED != _connectionState)
                Connect();

			try
            {
				StringBuilder sqlText_sb = new StringBuilder(queryString);
				sqlText_sb.Length = sqlText_sb.Length * Math.Max(1, Marshal.SystemDefaultCharSize);

				// If we aren't using the prepared statement handle, then free it.
                if (usePreparedStatement)
                {
                    stmtHandle = this._stmtHandle;
                }

                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(
                        TraceOptions.TraceOption.SQL, 
                        TraceOptions.TraceArea.Database,
                        TRACE_SUB_AREA_NAME,
                        "SQLExecDirect: " + queryString);
                }

				//  Allocate SQL statement handle for the query if needed.
				//  But do an ExecuteDirect we are doing this.
				if (IntPtr.Zero == stmtHandle) 
                {
					stmtHandle = GetStatementHandle();

                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG, 
                                           TraceOptions.TraceArea.Database,
                                           TRACE_SUB_AREA_NAME,
                                           "NCC::executeQuery(): Direct execution, allocated a new statement handle = " + stmtHandle);
                    }

                    this._lastExecuteHandle = stmtHandle;
					rc = SQLExecDirect(stmtHandle, sqlText_sb, sqlText_sb.Length);
					HandleODBCErrors(rc, "Failed to execute statement [SQLExecDirect]");
				} 
                else 
                {
                    this._lastExecuteHandle = stmtHandle;
					rc = SQLExecute(stmtHandle);
					HandleODBCErrors(rc, "Failed to execute query [SQLExecute]");
				}

				int colsCount = 0;
				rc = SQLNumResultCols(stmtHandle, out colsCount);
				HandleODBCErrors(rc, "Failed to get num of result columns [SQLNumResultCols]");

				StringBuilder theBuffer = new StringBuilder(32000);
                bool[] colIsFloatRealDouble = new bool[colsCount + 1];

				for (int colIndex = 1; colIndex <= colsCount; colIndex++) 
                {
					int colNameLen = 0;
					int colDataType;
					uint colSize;
					int colDecimalDigits;
					int colNullable;

                    colIsFloatRealDouble[colIndex] = false;

					theBuffer.Length = 0;
					rc = SQLDescribeCol(stmtHandle, colIndex, theBuffer, theBuffer.Capacity, out colNameLen,
										out colDataType, out colSize, out colDecimalDigits, out colNullable);
					if ((SQL_SUCCESS != rc)  &&  (SQL_SUCCESS_WITH_INFO != rc) ) 
                    {
						outputDataTable.Columns.Add(new DataColumn(null));

                        if (Logger.IsTracingEnabled)
                        {
                            Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                               TraceOptions.TraceArea.Database,
                                               TRACE_SUB_AREA_NAME,
                                               "NCC::executeQuery(): Failed to get description information for " +
                                               "        column #" + colIndex + ", SQLDescribeCol return code = " + rc +
                                               "        colNameLen = " + colNameLen + ", colDataType = " + colDataType +
                                               "        colSize = " + colSize + ", colDecimalDigits = " + colDecimalDigits +
                                               "        colNullable = " + colNullable);
                        }
						continue;
					}

                    if ((SQL_FLOAT == colDataType) || (SQL_REAL == colDataType) || (SQL_DOUBLE == colDataType))
                    {
                        colIsFloatRealDouble[colIndex] = true;
                    }

					String colName = theBuffer.ToString();
                    if ((null != colName) && "(EXPR)".Equals(colName.ToUpper().Trim()))
                    {
                        colName = "";
                    }

					int uniqueIdx = 0;
					String theColumnName = colName;
					while (outputDataTable.Columns.Contains(theColumnName)) 
                    {
						uniqueIdx++;
						theColumnName = colName + "-" + uniqueIdx;
					}

					outputDataTable.Columns.Add(new DataColumn(theColumnName) );
				}

				// Check if number of cols > 0 and if so, then fetch the results.
				int rowsRetrieved = 0;

				while (0 < colsCount) {
					rc = SQLFetch(stmtHandle);
					if (SQL_ERROR == rc)
						throw new Exception("Error fetching results [SQLFetch], return code = " + rc);

                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                           TraceOptions.TraceArea.Database,
                                           TRACE_SUB_AREA_NAME,
                                           "NCC::executeQuery(): Fetching results on on statement handle = " + stmtHandle + ", return code = " + rc);
                    }

                    if ((SQL_SUCCESS != rc) && (SQL_SUCCESS_WITH_INFO != rc))
                    {
                        break;
                    }

					rowsRetrieved++;
                    if ((0 < maxRows) && (rowsRetrieved > maxRows))
                    {
                        break;
                    }

					DataRow aRow = outputDataTable.NewRow();

					for (int idx = 1; idx <= colsCount; idx++) 
                    {
						int dataLength = 0;

                        if (colIsFloatRealDouble[idx]) 
                        {
                            theBuffer = new StringBuilder(32000);
                        } 
                        else 
                        {
                            theBuffer = new StringBuilder(256000);
                        }

						rc = SQLGetData(stmtHandle, (ushort) idx, SQL_CHAR, theBuffer, theBuffer.Capacity, out dataLength);
						if ((SQL_SUCCESS != rc)  &&  (SQL_SUCCESS_WITH_INFO != rc) ) 
                        {
                            if (Logger.IsTracingEnabled)
                            {
                                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                                   TraceOptions.TraceArea.Database,
                                                   TRACE_SUB_AREA_NAME,
                                                   "NCC::ExecuteQuery(): Row #" + rowsRetrieved +
                                                   " - failed to get data for column #" + idx +
                                                   ", return code = " + rc);
                            }
							continue;
						}

                        string[] textArray = theBuffer.ToString().Split('\n');
                        aRow[idx - 1] = textArray[textArray.Length - 1];
                    }

					// Add the row to the datatable.
					outputDataTable.Rows.Add(aRow);
				}

			} catch (Exception e) {
				errorMsg = ErrorMessage(stmtHandle, SQL_HANDLE_STMT, e.Message, false);
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "NCC::executeQuery(): Failed to execute query. Details = " + errorMsg);
                }
                throw new Exception(errorMsg);
			} 
            finally 
            {
                this._lastExecuteHandle = IntPtr.Zero;
				FreeStatementHandle(stmtHandle, "In executeQuery");
                if (usePreparedStatement)
                {
                    this._stmtHandle = IntPtr.Zero;
                }
			}

			return outputDataTable; 
		}

        /// <summary>
        /// Cancel query
        /// </summary>
        /// <returns></returns>
        public String CancelLastExecutedQuery() 
        {
            String errorMsg = "";

            try 
            {
                if (null != _lastExecuteHandle) 
                {
                    short rc = SQLCancel(this._lastExecuteHandle);

                    if ((SQL_SUCCESS != rc) && (SQL_SUCCESS_WITH_INFO != rc))
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                           TraceOptions.TraceArea.Database,
                                           TRACE_SUB_AREA_NAME,
                                           "NCC::CancelLastExecutedQuery(): Failure cancelling query, " + "return code = " + rc);
                    }
                }

            } catch (Exception e) {
                errorMsg = ErrorMessage(this._lastExecuteHandle, SQL_HANDLE_STMT, e.Message, false);
                if (Logger.IsTracingEnabled)
                {
                    Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                       TraceOptions.TraceArea.Database,
                                       TRACE_SUB_AREA_NAME,
                                       "NCC::cancelLastExecutedQuery(): Failed to cancel query. Details = " + errorMsg);
                }
            }

            
            return errorMsg;
        }

        /// <summary>
        /// ErrorMessage displays internal error messages
        /// displayMode = turn on displaying message to message box. true = display to message box. false = return message string to caller.
        /// </summary>
        /// <param name="handle"></param>
        /// <param name="HandleType"></param>
        /// <param name="exceptionMessage"></param>
        /// <param name="displayMode"></param>
        /// <returns></returns>
        private String ErrorMessage(IntPtr handle, short HandleType, string exceptionMessage, bool displayMode)
        {
            StringBuilder sqlState = new StringBuilder(8);
            short bufferlength = 32767;
            StringBuilder messageText = new StringBuilder(bufferlength);

            int nativePtr = 0;
            short textLengthPtr = 0;
            short error;
            short recNumber = 1;

			String wholeMessage = "";

			try 
            {
				error = SQLGetDiagRec(HandleType, handle, recNumber,
									  sqlState, ref nativePtr, messageText, bufferlength, ref textLengthPtr);

				String state = sqlState.ToString();
				String message = messageText.ToString();
				bool moreThanMaxErrors = false;

				if (message.Contains(TRAFODION_SQL_PREPARE_ERROR_PREFIX) ) 
                {
					recNumber = 2;
                    short retCode = SQLGetDiagRec(HandleType, handle, recNumber, sqlState, ref nativePtr,
                                                  messageText, bufferlength, ref textLengthPtr);

					while ((SQL_SUCCESS == retCode)  ||  (SQL_SUCCESS_WITH_INFO == retCode) ) 
                    {
						recNumber++;

                        if (TRAFODION_MAX_ERRORS_TO_DISPLAY >= (recNumber - 1))
                        {
                            message += Environment.NewLine + "\t\t          " + messageText.ToString();
                            state += ", " + sqlState.ToString();

                            if (SQL_SUCCESS_WITH_INFO == retCode)
                                message += Environment.NewLine + Environment.NewLine +
                                           "\t\t          ... diagnostic message truncated." + Environment.NewLine;
                        }
                        else
                        {
                            moreThanMaxErrors = true;
                        }

                        retCode = SQLGetDiagRec(HandleType, handle, recNumber, sqlState, ref nativePtr,
                                                     messageText, bufferlength, ref textLengthPtr);
					}
				}

                if (moreThanMaxErrors)
                {
                    message += Environment.NewLine + "\t\t          ...  and " +
                               (recNumber - TRAFODION_MAX_ERRORS_TO_DISPLAY - 1) + " more errors  ...";
                }

				wholeMessage = exceptionMessage + Environment.NewLine +
							   "\t\tError: " + message + Environment.NewLine +
							   "\t\tState: " + state;
			} 
            catch (Exception e) 
            {
				wholeMessage = "Failed to get SQL error diagnostic record." + Environment.NewLine +
							   "\t\tError: " + e.Message;
			}

			return wholeMessage;
        }

        #endregion Private methods
    }
}
