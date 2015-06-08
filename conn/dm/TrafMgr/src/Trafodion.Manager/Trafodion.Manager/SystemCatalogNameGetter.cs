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
using System.Collections.Generic;
using System.Collections;
using System.Data.Odbc;
using System.Data;
using Trafodion.Manager;
using System.ComponentModel;
using System.Windows.Forms;
using System.Text;
using System.Runtime.InteropServices;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.Framework.Controls;

namespace Trafodion.Manager.Framework
{
    public class SystemCatalogNameGetter
    {
        #region ODBC DLL Imports.  Contains Native ODBC declarations

        public const ushort SQL_HANDLE_ENV = 1;
        public const ushort SQL_HANDLE_DBC = 2;
        public const ushort SQL_HANDLE_STMT = 3;
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
        public const short SQL_UNKNOWN_TYPE = 0;
        public const short SQL_CHAR = 1;
        public const short SQL_NUMERIC = 2;
        public const short SQL_DECIMAL = 3;
        public const short SQL_INTEGER = 4;
        public const short SQL_SMALLINT = 5;
        public const short SQL_FLOAT = 6;
        public const short SQL_REAL = 7;
        public const short SQL_DOUBLE = 8;

        /* SQL connect driver codes */
        public const uint SQL_DRIVER_NOPROMPT = 0;
        public const uint SQL_DRIVER_COMPLETE = 1;
        public const uint SQL_DRIVER_PROMPT = 2;
        public const uint SQL_DRIVER_COMPLETE_REQUIRED = 3;



        /*SQLRETURN SQLAllocHandle(
                                SQLSMALLINT HandleType,
                                SQLHANDLE InputHandle,
                                SQLHANDLE* OutputHandlePtr);*/

        [DllImport("odbc32.dll")]
        extern static short SQLAllocHandle(
                                ushort HandleType,
                                int InputHandle,
                                out IntPtr OutputHandle);



        /*SQLRETURN SQLSetEnvAttr(
                                SQLHENV EnvironmentHandle,
                                SQLINTEGER Attribute,
                                SQLPOINTER ValuePtr,
                                SQLINTEGER StringLength);*/
        [DllImport("odbc32.dll")]
        extern static short SQLSetEnvAttr(
                                IntPtr EnvironmentHandle,
                                Int16 attribute,
                                IntPtr attrValue,
                                Int32 stringLength);



        /*SQLRETURN SQLSetConnectAttr(
                                 SQLHDBC ConnectionHandle,
                                 SQLINTEGER Attribute,
                                 SQLPOINTER ValuePtr,
                                 SQLINTEGER StringLength); */

        [DllImport("odbc32.dll")]
        extern static short SQLSetConnectAttr(
                                IntPtr ConnectionHandle,
                                Int16 Attribute,
                                String ValuePtr,
                                int length);

        /* SQLRETURN SQLSetStmtAttr(SQLHSTMT StatementHandle, SQLINTEGER Attribute, 
                                    SQLPOINTER ValuePtr, SQLINTEGER StringLength);
        */
        [DllImport("odbc32.dll")]
        extern static short SQLSetStmtAttr(
                                IntPtr StatementHandle,
                                int Attribute,
                                int ValuePtr,
                                int length);


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
                                string ServerName,
                                int ServerNameLength,
                                string UserName,
                                int UserNameLength,
                                string Authentication,
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
                                string InConnectionString,
                                int StringLength1,
                                StringBuilder OutConnectionString,
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
        extern static short SQLPrepare(
                                IntPtr StatementHandle,
                                String StatementText,
                                int TextLength);




        /*SQLRETURN SQLExecute( SQLHSTMT StatementHandle);*/

        [DllImport("odbc32.dll")]
        extern static short SQLExecute(
                                 IntPtr StatementHandle);



        /*SQLRETURN SQLExecDirect(
                                 SQLHSTMT StatementHandle,
                                 SQLCHAR* StatementText,
                                 SQLINTEGER TextLength);
        */
        [DllImport("odbc32.dll")]
        extern static short SQLExecDirect(
                                 IntPtr StatementHandle,
                                 StringBuilder StatementText,
                                 int TextLength);


        /*SQLRETURN SQLFetch(
                                 SQLHSTMT StatementHandle);*/

        [DllImport("odbc32.dll")]
        extern static short SQLFetch(
                                 IntPtr StatementHandle);


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
        SQLRETURN SQLGetCursorName(
               SQLHSTMT StatementHandle,  /  * hstmt * /
               SQLCHAR* CursorName,      /  * szCursor * /
               SQLSMALLINT BufferLength,     / * cbCursorMax * /
               SQLSMALLINT* NameLengthPtr);  / * pcbCursor * /
         * 
         */
        [DllImport("odbc32.dll")]
        extern static short SQLGetCursorName(
                                 IntPtr StatementHandle,
                                 StringBuilder TargetValuePtr,
                                 short BufferLength,
                                 out short StrLen);

        //SQLRETURN SQLFreeHandle(
        //                        SQLSMALLINT   HandleType,
        //                        SQLHANDLE     Handle);
        [DllImport("odbc32.dll")]
        static extern short SQLFreeHandle(
                                  ushort HandleType,
                                  IntPtr InputHandle);


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
        static extern short SQLGetDiagRec(
                                 ushort handleType,
                                 IntPtr Handle,
                                 short RecNumber,
                                 StringBuilder Sqlstate,
                                 ref int NativeErrorPtr,
                                 StringBuilder MessageText,
                                 short bufferlength,
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
        static extern short SQLTables(IntPtr StatementHandle,
                                         StringBuilder CatalogName,
                                         short NameLength1,
                                         StringBuilder SchemaName,
                                         short NameLength2,
                                         StringBuilder TableName,
                                         short NameLength3,
                                         StringBuilder TableType,
                                         short NameLength4);

        #endregion

        ConnectionDefinition theConnectionDefinition;

        public ConnectionDefinition TheConnectionDefinition
        {
            get { return theConnectionDefinition; }
            set { theConnectionDefinition = value; }
        }

        public SystemCatalogNameGetter(ConnectionDefinition aConnectionDefinition)
        {
            TheConnectionDefinition = aConnectionDefinition;
        }

        public string Get()
        {
            try
            {
                return GetSystemCatalogName();
            }
            catch (Exception e)
            {
                MessageBox.Show(Utilities.GetForegroundControl(), e.Message, Properties.Resources.ProductName, MessageBoxButtons.OK);
                throw new Exception("Could not obtain system catalog name");
            }
        }

        /// <summary>
        /// Returns the system catalogs for the trafodion system.
        /// </summary>
        /// <returns></returns>
        public string[] SystemCatalogNames()
        {
            try
            {
                return GetSystemCatalogNames();
            }
            catch (Exception e)
            {
                //MessageBox.Show(Utilities.GetForegroundControl(), e.Message, Properties.Resources.ProductName, MessageBoxButtons.OK);
                throw new Exception("Could not obtain system catalog names : " + e.Message);
            }
        }

        //Get an odbc environment handle
        private IntPtr GetSQLEnvironmentHandle()
        {
            int rc = 0;
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
            return sql_env_handle;
        }


		//ErrorMessage displays internal error messages
		//displayMode = turn on displaying message to message box. true = display to message box. false = return message string to caller.
        private String ErrorMessage(IntPtr handle, ushort HandleType, string exceptionMessage, bool displayMode)
        {

            StringBuilder sqlState = new StringBuilder(8);
            short bufferlength = 1000;
            StringBuilder messageText = new StringBuilder(bufferlength);
            int nativePtr = 0;
            short textLengthPtr = 0;
            short error;
            short recNumber = 1;

			String wholeMessage = "";

			try {
				error = SQLGetDiagRec(HandleType, handle, recNumber,
								   sqlState, ref nativePtr, messageText, bufferlength, ref textLengthPtr);

				String state = sqlState.ToString();
				String message = messageText.ToString();

				wholeMessage = exceptionMessage + Environment.NewLine +
							   "\t\tError: " + message + Environment.NewLine +
							   "\t\tState: " + state;

			} catch (Exception e) {
				wholeMessage = "Failed to get SQL error diagnostic record." + Environment.NewLine +
							   "\t\tError: " + e.Message;
			}



			if (displayMode == true)
			{

                MessageBox.Show(Utilities.GetForegroundControl(), wholeMessage, Properties.Resources.ProductName,
							    MessageBoxButtons.OK, MessageBoxIcon.Error);
				
			}
			
			return wholeMessage;
		                
        }


        //Get an odbc connection handle.  Connects to odbc
        private IntPtr GetDBConnectHandle(IntPtr envHandle, bool showConnectingDialog)
        {

            int rc = 0;

            try
            {

                rc = SQLAllocHandle(SQL_HANDLE_DBC, (int)envHandle.ToInt32(), out sql_dbc_handle);
                if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
                {
                    throw new Exception("SQLAllocHandle: failed to allocate SQL_HANDLE_DBC");
                }


                //Set timeout attribute
                String connTimeout = "60"; // seconds
                rc = SQLSetConnectAttr(sql_dbc_handle, (Int16)SQL_ATTR_LOGIN_TIMEOUT,
                                       connTimeout, connTimeout.Length);
                if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
                {
                    throw new Exception("SQLSetConnectAttr: failed to set SQL_ATTR_LOGIN_TIMEOUT");
                }

                String catName = "TRAFODION";
                rc = SQLSetConnectAttr(sql_dbc_handle, (Int16)SQL_ATTR_CURRENT_CATALOG,
                                       catName, catName.Length);
                if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
                {
                    throw new Exception("SQLSetConnectAttr: failed to set SQL_ATTR_LOGIN_TIMEOUT");
                }

                StringBuilder outConnectionStr = new StringBuilder(1024);
                int outConnectionStrLength;

                string inString = "";
                bool usingClientDSN = OdbcAccess.IsUsingClientDSN(TheConnectionDefinition);

                if (usingClientDSN)
                {
                    inString = string.Format("{0};ServerDSN={1};{2}",
                        TheConnectionDefinition.ClientDSNConnectionString,
                        TheConnectionDefinition.ConnectedDataSource,
                        Connection.DefaultSessionString);
                }
                else
                {
                    inString = string.Format("{0};ServerDSN={1};{2}",
                        TheConnectionDefinition.ConnectionString,
                        TheConnectionDefinition.ConnectedDataSource,
                        Connection.DefaultSessionString);
                }

                    rc = SQLDriverConnect(sql_dbc_handle, IntPtr.Zero,
                                        inString,
                                        inString.Length,
                                        outConnectionStr,
                                        outConnectionStr.MaxCapacity,
                                        out  outConnectionStrLength,
                                        SQL_DRIVER_NOPROMPT);

                    if ((rc != SQL_SUCCESS) && (rc != SQL_SUCCESS_WITH_INFO))
                    {
                        throw new Exception(ErrorMessage(sql_dbc_handle, SQL_HANDLE_DBC, "", false));
                    }

            }
            finally
            {
            }

            return sql_dbc_handle;
        }

        IntPtr sql_dbc_handle = IntPtr.Zero;
        IntPtr sql_env_handle = IntPtr.Zero;

        private string[] GetSystemCatalogNames()
        {
            ArrayList theSystemCatalogNames = new ArrayList();

            Hashtable schemas_ht = new Hashtable();
            IntPtr stmtHandle = IntPtr.Zero;
            short rc = 0;

            try
            {
                sql_env_handle = GetSQLEnvironmentHandle();
                sql_dbc_handle = GetDBConnectHandle(sql_env_handle, false);

                // Allocate SQL statement handle.
                rc = SQLAllocHandle(SQL_HANDLE_STMT, (int)sql_dbc_handle.ToInt32(), out stmtHandle);
                if ((SQL_SUCCESS != rc) && (SQL_SUCCESS_WITH_INFO != rc))
                {
                    throw new Exception("Catalogs SQLAllocHandle: failed to allocate SQL_HANDLE_STMT");
                }

                rc = SQLSetStmtAttr(stmtHandle, SQL_ATTR_QUERY_TIMEOUT, 60, 0);
                if ((SQL_SUCCESS != rc) && (SQL_SUCCESS_WITH_INFO != rc))
                {
                    throw new Exception("Catalogs SQLSetStmtAttr: failed to set command timeout");
                }


                /*  For now get all the catalogs on the system.  */
                StringBuilder catalogName = new StringBuilder("%"); ;
                StringBuilder schemaName = new StringBuilder("");
                StringBuilder tableName = new StringBuilder("");
                StringBuilder tableType = new StringBuilder("");

                rc = SQLTables(stmtHandle, catalogName, (short)catalogName.Length,
                                  schemaName, (short)schemaName.Length,
                                  tableName, (short)tableName.Length,
                                  tableType, (short)tableType.Length);
                if ((SQL_SUCCESS != rc) && (SQL_SUCCESS_WITH_INFO != rc))
                    throw new Exception("Catalogs SQLGetTables return code =  " + rc);

                int idx = 0;
                while (true)
                {
                    rc = SQLFetch(stmtHandle);
                    if ((SQL_ERROR == rc))
                        throw new Exception("Catalogs SQLFetch failed with return code = " + rc);

                    idx++;

                    if ((SQL_SUCCESS == rc) || (rc == SQL_SUCCESS_WITH_INFO))
                    {
                        int length;
                        StringBuilder catalogNameSB = new StringBuilder(1024);

                        rc = SQLGetData(stmtHandle, 1, SQL_CHAR, catalogNameSB, catalogNameSB.Capacity, out length);
                        if ((SQL_SUCCESS != rc) && (SQL_SUCCESS_WITH_INFO != rc))
                            throw new Exception("Catalogs SQLGetData failed with return code = " + rc);


                        String theCatalogName = catalogNameSB.ToString().Trim();

                        if (theCatalogName.StartsWith("TRAFODION"))
                        {
                            theSystemCatalogNames.Add(theCatalogName);
                        }

                    }
                    else
                    {
                        break;
                    }
                }
            }
            finally
            {
                //Free statement handle
                if (IntPtr.Zero != stmtHandle)
                {
                    rc = SQLFreeHandle(SQL_HANDLE_STMT, stmtHandle);
                }
                SQLDisconnect(sql_dbc_handle);

                if (sql_dbc_handle != IntPtr.Zero)
                {
                    rc = SQLFreeHandle(SQL_HANDLE_DBC, sql_dbc_handle);
                }

                if (sql_env_handle != IntPtr.Zero)
                {
                    rc = SQLFreeHandle(SQL_HANDLE_ENV, sql_env_handle);
                }
            }

            return (String[])theSystemCatalogNames.ToArray(typeof(String));
        }

        private string GetSystemCatalogName()
        {
            String[] systemcatalogs = GetSystemCatalogNames();
            string theSystemCatalogName = null;
            for (int i = 0; i < systemcatalogs.Length; i++)
            {
                if ((theSystemCatalogName == null) || (systemcatalogs[i].CompareTo(theSystemCatalogName) < 0))
                {
                    theSystemCatalogName = systemcatalogs[i];
                }
            }
            return theSystemCatalogName;
        }

    }
}
