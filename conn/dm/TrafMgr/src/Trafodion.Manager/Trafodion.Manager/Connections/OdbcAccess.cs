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
using System.Data;
using System.Data.Odbc;
using System.Text;
using System.Runtime.InteropServices;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using System.Reflection;

using System.Windows.Forms;

namespace Trafodion.Manager.Framework.Connections
{
    public class OdbcAccess
    {
        #region
        private static int m_errors = 0;
        private const string TRACE_SUB_AREA_NAME = "Odbc Access";
        #endregion

        #region ODBC DLL Imports

        public const ushort SQL_HANDLE_ENV = 1;
        public const ushort SQL_HANDLE_DBC = 2;
        public const ushort SQL_HANDLE_STMT = 3;
        public const ushort SQL_HANDLE_DESC = 4;
        public const ushort SQL_ATTR_LOGIN_TIMEOUT = 103;
        public const ushort SQL_ATTR_CURRENT_CATALOG = 109;
        public const ushort SQL_ATTR_CONNECTION_TIMEOUT = 113;
        public const ushort SQL_ATTR_QUERY_TIMEOUT = 0;
        public const short  SQL_SUCCESS = 0;
        public const short  SQL_ERROR = -1;
        public const short  SQL_SUCCESS_WITH_INFO = 1;
        public const short  SQL_NO_DATA = 100;
        public const long    SQL_ATTR_ODBC_VERSION = 200;
        public const int    SQL_OV_ODBC3 = 3;
        public const short  SQL_FETCH_NEXT = 1;
        public const short  SQL_FETCH_FIRST = 2;

        /* SQL data type codes */
        public const short  SQL_UNKNOWN_TYPE = 0;
        public const short  SQL_CHAR = 1;
        public const short  SQL_NUMERIC = 2;
        public const short  SQL_DECIMAL = 3;
        public const short  SQL_INTEGER = 4;
        public const short  SQL_SMALLINT = 5;
        public const short  SQL_FLOAT = 6;
        public const short  SQL_REAL = 7;
        public const short  SQL_DOUBLE = 8;

        /* SQL connect driver codes */
        public const uint   SQL_DRIVER_NOPROMPT = 0;
        public const uint   SQL_DRIVER_COMPLETE = 1;
        public const uint   SQL_DRIVER_PROMPT = 2;
        public const uint   SQL_DRIVER_COMPLETE_REQUIRED = 3;

        /* encryption attribute code */
        public const ushort SQL_ATTR_CERTIFICATE_DIR = 5200;
        public const ushort SQL_ATTR_CERTIFICATE_FILE = 5201;
        public const ushort SQL_ATTR_CERTIFICATE_FILE_ACTIVE = 5202;
        public const ushort SQL_ATTR_ENCRYPTBUFFER_LEN = 5203;
        public const ushort SQL_ATTR_ENCRYPTDATA = 5204;

        public const int SQL_C_TYPE_BINARY = -2;
        public const int SQL_NTS = -3;
        public const int SQL_IS_POINTER = -4;
        public const int SQL_IS_UINTEGER = -5;
        public const int SQL_IS_INTEGER = -6;
        public const int SQL_IS_USMALLINT = -7;
        public const int SQL_IS_SMALLINT = -8;

        /* sql info type */
        public const ushort SQL_INFO_TYPE_SERVER_NAME = 13;

        //SQLRETURN SQLDataSources(SQLHENV EnvironmentHandle,
        //                         SQLUSMALLINT Direction,
        //                         SQLCHAR* ServerName,
        //                         SQLSMALLINT BufferLength1,
        //                         SQLSMALLINT* NameLength1Ptr,
        //                         SQLCHAR* Description,
        //                         SQLSMALLINT BufferLength2,
        //                         SQLSMALLINT* NameLength2Ptr);
        [DllImport("odbc32.dll")]
        static extern short SQLDataSources(
                                   IntPtr EnvironmentHandle,
                                   short Direction,
                                   StringBuilder ServerName,
                                   short BufferLength1,
                                   ref short NameLength1Ptr,
                                   StringBuilder Description,
                                   short BufferLength2,
                                   ref short NameLength2Ptr);

        //SQLRETURN SQLSetEnvAttr(SQLHENV EnvironmentHandle,
        //                        SQLINTEGER Attribute,
        //                        SQLPOINTER ValuePtr,
        //                        SQLINTEGER StringLength);
        [DllImport("odbc32.dll")]
        extern static short SQLSetEnvAttr(
            IntPtr envHandle,
            long attribute,
            IntPtr attrValue,
            int stringLength);

        //SQLRETURN SQLAllocHandle(SQLSMALLINT    HandleType,
        //                         SQLHANDLE      InputHandle,
        //                         SQLHANDLE *    OutputHandlePtr);
        [DllImport("odbc32.dll")]
        extern static short SQLAllocHandle(
            ushort HandleType,
            int InputHandle,
            out IntPtr OutputHandle);

        //SQLRETURN SQLFreeHandle(SQLSMALLINT   HandleType,
        //                        SQLHANDLE     Handle);
        [DllImport("odbc32.dll")]
        static extern short SQLFreeHandle(
            ushort HandleType,
            IntPtr InputHandle);

        /*SQLRETURN SQLSetConnectAttr(
                                 SQLHDBC ConnectionHandle,
                                 SQLINTEGER Attribute,
                                 SQLPOINTER ValuePtr,
                                 SQLINTEGER StringLength); */

        [DllImport("odbc32.dll")]
        extern static short SQLSetConnectAttr(
                                IntPtr ConnectionHandle,
                                ushort Attribute,
                                String ValuePtr,
                                int length);

        [DllImport("odbc32.dll")]
        //extern static short SQLGetConnectAttr(
        //                        IntPtr ConnectionHandle,
        //                        ushort Attribute,
        //                        byte[] ValuePtr,
        //                        int BufferLength,
        //                        out int length);
        extern static short SQLGetConnectAttr(
                                IntPtr ConnectionHandle,
                                ushort Attribute,
                                byte[] ValuePtr,
                                int BufferLength,
                                out int length);

        [DllImport("odbc32.dll")]
        //SQLRETURN SQLGetInfo(
        //SQLHDBC         ConnectionHandle,
        //SQLUSMALLINT    InfoType,
        //SQLPOINTER      InfoValuePtr,
        //SQLSMALLINT     BufferLength,
        //SQLSMALLINT *   StringLengthPtr);
        extern static short SQLGetInfo(
                                IntPtr ConnectionHandle,
                                ushort InfoType,
                                byte[] ValuePtr,
                                short BufferLength,
                                out int length);

        #endregion

        public OdbcAccess()
        {
        }

        public static List<String> getOdbcDataSources()
        {
            IntPtr sql_env_handle = IntPtr.Zero;
            StringBuilder dsn_name = new StringBuilder(32);
            short dsn_name_len = 0;
            StringBuilder desc_name = new StringBuilder(256);
            short desc_len = 0;
            short rc = 0;
            List<String> list = new List<String>();

            try
            {
                rc = SQLAllocHandle(SQL_HANDLE_ENV, 0, out sql_env_handle);
                if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
                {
                    throw new Exception("SQLAllocHandle: failed to allocate SQL_HANDLE_ENV");
                }
                rc = SQLSetEnvAttr(sql_env_handle, SQL_ATTR_ODBC_VERSION, (IntPtr)SQL_OV_ODBC3, 0);
                if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
                {
                    throw new Exception("SQLSetEnvAttr: failed to set SQL_ATTR_ODBC_VERSION");
                }

                rc = SQLDataSources(sql_env_handle, SQL_FETCH_FIRST, dsn_name, (short)dsn_name.Capacity,
                                    ref dsn_name_len, desc_name, (short)desc_name.Capacity, ref desc_len);
                if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) && (rc != SQL_NO_DATA))
                {
                    throw new Exception("SQLDataSources: failed to get ODBC data source");
                }
                while ((rc == SQL_SUCCESS) || (rc == SQL_SUCCESS_WITH_INFO))
                {
                    String desc = desc_name.ToString();
                    if (desc.StartsWith("HP"))
                    {
                        list.Add(dsn_name.ToString());
                    }

                    rc = SQLDataSources(sql_env_handle, SQL_FETCH_NEXT, dsn_name, (short)dsn_name.Capacity,
                                        ref dsn_name_len, desc_name, (short)desc_name.Capacity, ref desc_len);
                    if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) && (rc != SQL_NO_DATA))
                    {
                        throw new Exception("SQLDataSources: failed to get ODBC data source");
                    }
                }
            }
            finally
            {
                if (sql_env_handle != IntPtr.Zero)
                {
                    rc = SQLFreeHandle(SQL_HANDLE_ENV, sql_env_handle);
#if DEBUG
					if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
					{
						throw new Exception("SQLFreeHandle: failed to free SQL_HANDLE_ENV");
					}
#endif
                }
            }

            return list;
        }
        public static bool IsUsingClientDSN(ConnectionDefinition aConnectionDefinition)
        {
            List<string> datasources = getOdbcDataSources();
            if (datasources.Contains(aConnectionDefinition.ClientDataSource))
            {
                String serverInfo = Utilities.getODBCStringValue(aConnectionDefinition.ClientDataSource, Utilities.REGISTRY_SERVER_KEY);
                string host = getHostNameFromServer(serverInfo).Trim();
                string port = getPortNumberFromServer(serverInfo).Trim();
                return (aConnectionDefinition.Host.Equals(host) && aConnectionDefinition.Port.Equals(port));
            }
            else
            {
                return false;
            }
        }

        public static string getHostNameFromServer(string serverName)
        {
            string hostName = "";

            if (serverName == null)
                return hostName;

            int startIndex = serverName.IndexOf("TCP:");
            int endIndex = serverName.IndexOf("/");

            if (startIndex != -1 && endIndex != -1)
            {
                startIndex += 4;
                int length = endIndex - startIndex;
                hostName = serverName.Substring(startIndex, length);
            }

            return hostName;
        }

        public static string getPortNumberFromServer(string serverName)
        {
            string portNumber = "";

            if (serverName == null)
                return portNumber;

            int startIndex = serverName.IndexOf("/");

            if (startIndex != -1)
            {
                startIndex += 1;
                portNumber = serverName.Substring(startIndex);
            }

            return portNumber;
        }

        private static String getDecodedColumnValueAsString(OdbcDataReader reader, int colIndex,
                                                            Encoding columnDataEncoding)
        {

            long maxBufferSize = 32000;

            MemoryStream memStream = new MemoryStream();
            byte[] readBuffer = new byte[maxBufferSize];
            long numBytesRead = reader.GetBytes(colIndex, memStream.Position, readBuffer, 0, (int)maxBufferSize);
            memStream.Write(readBuffer, 0, (int)numBytesRead);

            while (numBytesRead == maxBufferSize)
            {
                readBuffer = new byte[maxBufferSize];
                numBytesRead = reader.GetBytes(colIndex, memStream.Position, readBuffer, 0, (int)maxBufferSize);
                memStream.Write(readBuffer, 0, (int)numBytesRead);
            }

            readBuffer = memStream.ToArray();
            String iconvString = columnDataEncoding.GetString(readBuffer);
            memStream.Close();
            return iconvString;
        }



        public static List<String> getListObjects(OdbcCommand cmd, Encoding columnDataEncoding)
        {
            if (null == cmd)
                return null;

            List<String> list = new List<String>();

            OdbcDataReader reader = null;
            try
            {
                reader = Utilities.ExecuteReader(cmd, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Framework, TRACE_SUB_AREA_NAME, true);

                while (reader.Read())
                {
                    String colValue = null;

                    try
                    {
                        colValue = getDecodedColumnValueAsString(reader, 0, columnDataEncoding);

                    }
                    catch (Exception)
                    {
                        colValue = reader.GetValue(0).ToString();
                    }

                    list.Add(colValue);
                }

            }
            finally
            {
                if (null != reader)
                    reader.Close();

            }

            return list;
        }

        public static DataTable executeQuery(OdbcCommand cmd, int limit)
        {
            return executeQuery(cmd, limit, null);
        }


        public static DataTable executeQuery(OdbcCommand cmd, int limit,
            /*NCCRepositoryVersion rv*/string dummy)
        {
            if (null == cmd)
                return null;


            bool doTracing = Logger.IsTracingEnabled;
            Encoding[] columnEncodingFunction = null;
            DataTable dt = new DataTable();

            // critical section
            int timeoutSecs = cmd.CommandTimeout;
            if (0 >= timeoutSecs)
                timeoutSecs = -1;

            //if (!Monitor.TryEnter(cmd.Connection, TimeSpan.FromSeconds(timeoutSecs)))
            //    throw new Exception("Statement execution timed out. Timeout interval = " + timeoutSecs + " seconds. ");


            try
            {
                OdbcDataReader reader = null;
                int recordsRetrieved = 0;

                try
                {
                    reader = Utilities.ExecuteReader(cmd, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.Framework, TRACE_SUB_AREA_NAME, true);

                    int columnCount = reader.FieldCount;
                    columnEncodingFunction = new Encoding[columnCount];

                    for (int idx = 0; idx < columnCount; idx++)
                    {
                        String theColumnNameToMap = null;
                        try
                        {
                            String colName = reader.GetName(idx);
                            if ((null != colName) && "(EXPR)".Equals(colName.ToUpper().Trim()))
                                colName = "";

                            columnEncodingFunction[idx] = null;
                            DataColumn dc = new DataColumn(colName);
                            dc.DataType = reader.GetFieldType(idx);
                            dt.Columns.Add(dc);

                            if (typeof(string) == dc.DataType)
                                theColumnNameToMap = colName;

                        }
                        catch (Exception e)
                        {
                            if (Logger.IsTracingEnabled)
                            {
                                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                                   TraceOptions.TraceArea.Framework,
                                                   TRACE_SUB_AREA_NAME,
                                                   "OdbcAccess::executeQuery(): Error getting " +
                                                            "column #" + idx + " name. Using default name. " +
                                                            "Error Details = " + e.Message);
                            }

                            dt.Columns.Add(new DataColumn(null));
                        }

                        //if ((null != rv) && (null != theColumnNameToMap))
                        //{
                        //    columnEncodingFunction[idx] = rv.getColumnDataEncoding(theColumnNameToMap);

                        //    if(Logger.IsTracingEnabled)
                        //      Logger.OutputToLog("*** Column Name '" + theColumnNameToMap +
                        //                                    "' data encoder used is " + columnEncodingFunction[idx].ToString());
                        //}

                    }


                    //If limit is 0, there is no need to go further
                    if (limit == 0)
                        return dt;


                    while (reader.Read())
                    {
                        DataRow dr = dt.NewRow();
                        for (int idx = 0; idx < columnCount; idx++)
                        {
                            try
                            {
                                Object theValue = DBNull.Value;

                                if ((null != columnEncodingFunction[idx]) &&
                                    (Encoding.UTF8 != columnEncodingFunction[idx]))
                                    theValue = getDecodedColumnValueAsString(reader, idx, columnEncodingFunction[idx]);
                                else
                                    theValue = reader.GetValue(idx);

                                if (DBNull.Value != theValue)
                                    dr[idx] = theValue;

                            }
                            catch (Exception e)
                            {
                                if (0 == (recordsRetrieved % 100))
                                    if (Logger.IsTracingEnabled)
                                    {
                                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                                           TraceOptions.TraceArea.Framework,
                                                           TRACE_SUB_AREA_NAME,
                                                           "OdbcAccess::executeQuery(): *** Row #" + recordsRetrieved +
                                                                    " :: Exception '" + e.Message + "' w/ column " +
                                                                    dt.Columns[idx].ColumnName + ", datatype = " +
                                                                    dt.Columns[idx].DataType.ToString());
                                    }
                            }

                        }


                        dt.Rows.Add(dr);
                        recordsRetrieved++;

                        if ((0 <= limit) && (recordsRetrieved >= limit))
                            break;
                    }
                }
                finally
                {
                    if (null != reader)
                        reader.Close();

                }
            }
            finally
            {
                //Monitor.Exit(cmd.Connection);
            }

            return dt;
        }

        public static DataTable getDataTableFromSQL(OdbcCommand selectStmt)
        {
            if (selectStmt == null)
                return null;

            DataTable dt = new DataTable();
            m_errors = 0;
            lock (selectStmt.Connection)
            {
                try
                {
                    OdbcDataAdapter da = new OdbcDataAdapter(selectStmt);
#if DEBUG
					da.FillError += new FillErrorEventHandler(da_FillError);
#endif

                    da.Fill(dt);

                }
                catch (Exception e)
                {
                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                           TraceOptions.TraceArea.Framework,
                                           TRACE_SUB_AREA_NAME,
                                           "OdbcAccess::executeQuery(): ODBC Exception e : " + e.Message);
                    }

                    throw e;
                }
            }

            return dt;
        }

        static void da_FillError(object sender, FillErrorEventArgs e)
        {
            if (m_errors++ == 0)
            {
                Console.WriteLine();
            }
            Console.WriteLine(m_errors + ")" + " da_FillError: " + e.Errors.Message);
            e.Continue = true;
            DataRow[] dr = e.DataTable.GetErrors();
            for (int i = 0; i < dr.Length; i++)
            {
                Console.WriteLine("dr[" + i + "]=" + dr[i].RowError);
            }
        }

        /// <summary>
        /// Supporting function for password encryption support. 
        /// </summary>
        /// <param name="odbcConnection"></param>
        /// <returns></returns>
        public static IntPtr GetDBConnectHandle(OdbcConnection odbcConnection)
        {
            Type t = odbcConnection.GetType();
            FieldInfo f = t.GetField("_connectionHandle", BindingFlags.NonPublic | BindingFlags.Instance);
            if (f.GetValue(odbcConnection) != null)
            {
                Object connectionHandle = f.GetValue(odbcConnection);
                Type t1 = connectionHandle.GetType();
                FieldInfo f1 = t1.GetField("handle", BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
                Object hdle = f1.GetValue(connectionHandle);
                return (IntPtr)hdle;
            }

            return IntPtr.Zero;
        }

        /// <summary>
        /// Supporting function for invoking native ODBC calls.
        /// </summary>
        /// <param name="length"></param>
        /// <returns></returns>
        private static int SQL_LEN_BINARY_ATTR(int length)
        {
            const int SQL_LEN_BINARY_ATTR_OFFSET = (-100);
            return (-(length)+SQL_LEN_BINARY_ATTR_OFFSET);
        }

        /// <summary>
        /// To encrypt passwords via the ODBC connection. The driver provides the function.
        /// </summary>
        /// <param name="odbcConnection"></param>
        /// <param name="password"></param>
        /// <returns></returns>
        public static string EncryptPassword(OdbcConnection odbcConnection, string password)
        {
            IntPtr sql_dbc_handle = IntPtr.Zero;
            byte[] buffer = new byte[2048];
            int length = 0;
            short rc = 0;
            try
            {
                //for (int i = 0; i < 2048; i++)
                //{
                //    buffer[i] = 0;
                //}

                // Get the dbc Handle first. 
                sql_dbc_handle = GetDBConnectHandle(odbcConnection);
                if (sql_dbc_handle == IntPtr.Zero)
                {
                    throw new Exception("Encryption: failed to get dbc handle.");
                }

                // Set the password to be encrypted as connector attribute. 
                rc = SQLSetConnectAttr(sql_dbc_handle, SQL_ATTR_ENCRYPTDATA, password, SQL_NTS);
                if ((SQL_SUCCESS != rc) && (SQL_SUCCESS_WITH_INFO != rc))
                {
                    throw new Exception("Encryption SQLSetConnectAttr: failed to set encryption data");
                }

                // Get the total length of encrypted password.
                rc = SQLGetConnectAttr(sql_dbc_handle, SQL_ATTR_ENCRYPTBUFFER_LEN, buffer, SQL_IS_UINTEGER, out length);
                if ((SQL_SUCCESS != rc) && (SQL_SUCCESS_WITH_INFO != rc))
                {
                    throw new Exception("Encryption SQLGetConnectAttr: failed to get encryption data length");
                }

                length = BitConverter.ToInt32(buffer, 0);

                // Get the encrypted password.
                rc = SQLGetConnectAttr(sql_dbc_handle, SQL_ATTR_ENCRYPTDATA, buffer, SQL_IS_POINTER, out length);
                if ((SQL_SUCCESS != rc) && (SQL_SUCCESS_WITH_INFO != rc))
                {
                    throw new Exception("Encryption SQLGetConnectAttr: failed to get encryption data");
                }
            }
            finally
            {
                // nothing at this time.
            }

            System.Text.ASCIIEncoding enc = new System.Text.ASCIIEncoding();
            return enc.GetString(buffer, 0, length);
        }

        public static string GetODBCServerName(OdbcConnection odbcConnection)
        {
            IntPtr sql_env_handle = IntPtr.Zero;
            int name_len = 0;
            short desc_len = 0;
            short rc = 0;
            List<String> list = new List<String>();
            IntPtr sql_dbc_handle = IntPtr.Zero;
            byte[] buffer = new byte[2048];
            int length = 0;

            try
            {
                rc = SQLAllocHandle(SQL_HANDLE_ENV, 0, out sql_env_handle);
                if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
                {
                    throw new Exception("SQLAllocHandle: failed to allocate SQL_HANDLE_ENV");
                }
                rc = SQLSetEnvAttr(sql_env_handle, SQL_ATTR_ODBC_VERSION, (IntPtr)SQL_OV_ODBC3, 0);
                if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
                {
                    throw new Exception("SQLSetEnvAttr: failed to set SQL_ATTR_ODBC_VERSION");
                }

                sql_dbc_handle = GetDBConnectHandle(odbcConnection);
                if (sql_dbc_handle == IntPtr.Zero)
                {
                    throw new Exception("Encryption: failed to get dbc handle.");
                }

                rc = SQLGetInfo(sql_dbc_handle, 13, buffer, (short)buffer.Length, out name_len);
                if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO) && (rc != SQL_NO_DATA))
                {
                    throw new Exception("SQLDataSources: failed to get ODBC data source");
                }
            }
            finally
            {
                if (sql_env_handle != IntPtr.Zero)
                {
                    rc = SQLFreeHandle(SQL_HANDLE_ENV, sql_env_handle);
#if DEBUG
                    if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
                    {
                        throw new Exception("SQLFreeHandle: failed to free SQL_HANDLE_ENV");
                    }
#endif
                }
            }

            System.Text.ASCIIEncoding enc = new System.Text.ASCIIEncoding();
            string serverinfo = enc.GetString(buffer, 0, name_len);
            if (Logger.IsTracingEnabled)
            {
                Logger.OutputToLog(TraceOptions.TraceOption.DEBUG,
                                   TraceOptions.TraceArea.Framework,
                                   TRACE_SUB_AREA_NAME,
                                   string.Format("OdbcAccess::GetODBCServerName(): {0}", serverinfo));
            }

            return getHostNameFromServer(serverinfo);
        }
    }
}
