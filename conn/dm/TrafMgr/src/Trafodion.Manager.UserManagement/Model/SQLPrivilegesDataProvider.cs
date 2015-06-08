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
using Trafodion.Manager.DatabaseArea;
using Trafodion.Manager.DatabaseArea.Model;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;
using Trafodion.Manager.UniversalWidget;

namespace Trafodion.Manager.UserManagement.Model
{
    public enum WorkMode { UserMode, RoleMode }

    class SQLPrivilegesDataProvider : DatabaseDataProvider
    {
        ConnectionDefinition _connectionDefinition;
        DataTable _resultsDataTable;
        string TRACE_SUB_AREA_NAME = "Fetch SQL Privileges for User";
        private static WorkMode _workMode = WorkMode.UserMode;
        public SQLPrivilegesDataProvider(WorkMode aWorkMode,ConnectionDefinition aConnectionDefinition, DatabaseDataProviderConfig dbConfig)
            : base(dbConfig)
        {
            _workMode = aWorkMode;
            _connectionDefinition = aConnectionDefinition;
        }

        public override System.Data.DataTable GetDataTable()
        {
            return _resultsDataTable;
        }

        public override void DoFetchData(Trafodion.Manager.Framework.WorkerThread worker, System.ComponentModel.DoWorkEventArgs e)
        {
            if (_theDefaultParameters != null && _theDefaultParameters.ContainsKey("PRIV_FOR_USER_NAME"))
            {
                string granteeName = _theDefaultParameters["PRIV_FOR_USER_NAME"] as string;
                granteeName = granteeName.Replace("'", "''");

                _resultsDataTable = new DataTable();
                TrafodionSystem sqlMxSystem = TrafodionSystem.FindTrafodionSystem(_connectionDefinition);
                Connection aConnection = new Connection(_connectionDefinition);
                try
                {
                    OdbcConnection odbcConnection = aConnection.OpenOdbcConnection;
                    DataTable currentCatalogInfo = ExecuteSQLQuery(odbcConnection, SelectCatalogInfo());

                    foreach (DataRow row in currentCatalogInfo.Rows)
                    {
                        string catalogName = row[0] as string;
                        long schemaCount = (long)row[1];
                        catalogName = TrafodionName.ExternalForm(catalogName);

                        if (schemaCount > 0)
                        {
                            DataTable catalogResults = ExecuteSQLQuery(odbcConnection, SelectSchemaPrivilegesForUser(granteeName, catalogName));
                            if (_resultsDataTable.Columns.Count == 0)
                            {
                                if (catalogResults != null)
                                {
                                    _resultsDataTable = catalogResults;
                                }
                            }
                            else
                            {
                                if (catalogResults != null)
                                    _resultsDataTable.Merge(catalogResults, true);
                            }

                            catalogResults = ExecuteSQLQuery(odbcConnection, SelectObjectPrivilegesForUser(granteeName, catalogName));
                            if (_resultsDataTable.Columns.Count == 0)
                            {
                                if (catalogResults != null)
                                {
                                    _resultsDataTable = catalogResults;
                                }
                            }
                            else
                            {
                                if (catalogResults != null)
                                    _resultsDataTable.Merge(catalogResults, true);
                            }
                            catalogResults = ExecuteSQLQuery(odbcConnection, SelectColumnPrivilegesForUser(granteeName, catalogName));
                            if (_resultsDataTable.Columns.Count == 0)
                            {
                                if (catalogResults != null)
                                {
                                    _resultsDataTable = catalogResults;
                                }
                            }
                            else
                            {
                                if (catalogResults != null)
                                    _resultsDataTable.Merge(catalogResults, true);
                            }
                        }
                    }
                }
                finally
                {
                    if (aConnection != null)
                    {
                        aConnection.Close();
                    }
                }
            }
        }

        DataTable ExecuteSQLQuery(OdbcConnection odbcConnection, string sqlCommandString)
        {
            DataTable dataTable = new DataTable();
            OdbcCommand theOdbcCommand = new OdbcCommand(sqlCommandString);
            theOdbcCommand.Connection = odbcConnection;
            OdbcDataReader theReader = Utilities.ExecuteReader(theOdbcCommand, TraceOptions.TraceOption.SQL, TraceOptions.TraceArea.UserManagement, TRACE_SUB_AREA_NAME, false);
            //Create the columns for the table
            try
            {
                //Try to get columns from result set metadata
                GetColumnsFromResultSetMetatData(theReader, ref dataTable);
            }
            catch (Exception ex)
            {
                //fall back approach and manually figure out the column info
                CreateColumnsFromResultSet(theReader, ref dataTable);
            }
            while (theReader.Read())
            {
                object[] theCurrRow = new object[theReader.FieldCount];
                for (int currField = 0; currField < theReader.FieldCount; currField++)
                {
                    try
                    {
                        //For TIME types, .NET truncates the milliseconds when converting to TimeSpan
                        //So read the timespan objects as strings and then convert it back to timespan.
                        if (dataTable.Columns[currField].DataType == typeof(System.TimeSpan))
                        {
                            string timeSpanString = theReader.GetString(currField);
                            try
                            {
                                theCurrRow[currField] = TimeSpan.Parse(timeSpanString);
                            }
                            catch (Exception oe)
                            {
                                theCurrRow[currField] = timeSpanString;
                            }
                        }
                        else
                        {
                            theCurrRow[currField] = theReader.GetValue(currField);
                        }
                    }
                    catch (Exception ex)
                    {
                        if (Logger.IsTracingEnabled)
                        {
                            Logger.OutputToLog(
                                TraceOptions.TraceOption.DEBUG,
                                TraceOptions.TraceArea.UserManagement,
                                TRACE_SUB_AREA_NAME,
                                "Reader fetch exception: " + ex.Message);
                        }

                        try
                        {
                            theCurrRow[currField] = theReader.GetString(currField);
                        }
                        catch (Exception e1)
                        {
                        }
                    }
                }
                //Add rows to the result data table
                dataTable.Rows.Add(theCurrRow);
            }
            return dataTable;
        }

        string SelectCatalogInfo()
        {
            return "SELECT TRIM(T1.CAT_NAME), COUNT(T2.SCHEMA_UID) " +
                    "FROM TRAFODION.SYSTEM_SCHEMA.CATSYS AS T1, " +
                    "TRAFODION.SYSTEM_SCHEMA.SCHEMATA AS T2 " +
                    "WHERE T1.CAT_UID = T2.CAT_UID " +
                    "GROUP BY T1.CAT_NAME " +
                    "FOR READ COMMITTED ACCESS";
        }

        static public string SelectSchemaPrivilegesForUser(string granteeName, string catalogName)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(" SELECT CATALOG_NAME, SCHEMA_NAME, OBJECT_NAME, OBJECT_TYPE, PRIVILEGE_TYPE, COLUMN_NAME, AUTHNAME(GRANTOR) AS GRANTOR, IS_GRANTABLE ");
            sb.Append(" FROM");
            sb.Append(" (");
            sb.Append(" SELECT CATALOG_NAME,  SCHEMA_NAME, '' AS OBJECT_NAME, 'SCHEMA' AS OBJECT_TYPE,  SCP.GRANTOR_LL AS GRANTOR,  SCP.PRIVILEGE_TYPE, ");
            sb.Append(" ' ' AS COLUMN_NAME,  ");
            sb.Append(" CASE IS_GRANTABLE ");
            sb.Append(" WHEN 'Y' THEN 'YES'   ");
            sb.Append(" ELSE 'NO'  ");
            sb.Append(" END AS IS_GRANTABLE");
            sb.AppendFormat(" FROM {0}.TRAFODION_INFORMATION_SCHEMA.SCHEMA_PRIVILEGES SCP,              ", catalogName);
             
            if (_workMode== WorkMode.UserMode)
            {
                sb.AppendFormat(" {0}.TRAFODION_INFORMATION_SCHEMA.USER_INFO UI         ", catalogName);
                sb.AppendFormat(" WHERE UI.USER_NAME = '{0}' AND               ", granteeName);
                sb.Append(" SCP.GRANTEE_LL = UI.USER_ID AND               ");
            }
            else if (_workMode == WorkMode.RoleMode)
            {
                sb.AppendFormat(" {0}.TRAFODION_INFORMATION_SCHEMA.ROLE_INFO RI         ", catalogName);
                sb.AppendFormat(" WHERE RI.ROLE_NAME = '{0}' AND               ", granteeName);
                sb.Append(" SCP.GRANTEE_LL = RI.ROLE_ID AND               ");
            }

            sb.Append(" NOT (SCP.SCHEMA_NAME LIKE '"_MD_"%' OR                     ");
            sb.Append(" SCP.SCHEMA_NAME = '@MAINTAIN_SCHEMA@' OR                     ");
            sb.Append(" SCP.SCHEMA_NAME LIKE 'HP\\_%')    ");
            sb.Append(" FOR READ UNCOMMITTED ACCESS");
            sb.Append(" ) AS CP");
            return sb.ToString();
        }

        static public string SelectObjectPrivilegesForUser(string granteeName, string catalogName)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(" SELECT CATALOG_NAME, SCHEMA_NAME, OBJECT_NAME, OBJECT_TYPE, PRIVILEGE_TYPE, COLUMN_NAME, AUTHNAME(GRANTOR) AS GRANTOR, IS_GRANTABLE ");
            sb.Append(" FROM");
            sb.Append(" (");
            sb.Append(" SELECT OBJ.CATALOG_NAME, OBJ.SCHEMA_NAME, OBJ.OBJECT_NAME,          ");
            sb.Append(" CASE OBJ.OBJECT_TYPE            ");
            sb.Append(" WHEN 'BT' THEN 'TABLE'            ");
            sb.Append(" WHEN 'VI' THEN 'VIEW'            ");
            sb.Append(" WHEN 'UR' THEN 'PROCEDURE'            ");
            sb.Append(" WHEN 'MV' THEN 'MV'            ");
            sb.Append(" WHEN 'SY' THEN 'SYNONYM'            ");
            sb.Append(" ELSE  OBJ.OBJECT_TYPE ");
            sb.Append(" END AS OBJECT_TYPE,  	");
            sb.Append(" OBJ.GRANTOR_LL AS GRANTOR,          ");
            sb.Append(" OBJ.PRIVILEGE_TYPE ,         ");
            sb.Append(" ' ' AS COLUMN_NAME, ");
            sb.Append(" CASE IS_GRANTABLE ");
            sb.Append(" WHEN 'Y' THEN 'YES'   ");
            sb.Append(" ELSE 'NO'  ");
            sb.Append(" END AS IS_GRANTABLE");
            sb.AppendFormat(" FROM {0}.TRAFODION_INFORMATION_SCHEMA.OBJECT_PRIVILEGES OBJ,              ", catalogName);
            if (_workMode == WorkMode.UserMode)
            {
                sb.AppendFormat(" {0}.TRAFODION_INFORMATION_SCHEMA.USER_INFO UI         ", catalogName);
                sb.AppendFormat(" WHERE UI.USER_NAME = '{0}' AND               ", granteeName);
                sb.Append(" OBJ.GRANTEE_LL = UI.USER_ID AND             ");
            }
            else if (_workMode == WorkMode.RoleMode)
            {
                sb.AppendFormat(" {0}.TRAFODION_INFORMATION_SCHEMA.ROLE_INFO RI         ", catalogName);
                sb.AppendFormat(" WHERE RI.ROLE_NAME = '{0}' AND               ", granteeName);
                sb.Append(" OBJ.GRANTEE_LL = RI.ROLE_ID AND             ");
            }
            sb.Append(" NOT (OBJ.OBJECT_TYPE = 'NN' OR                   ");
            sb.Append(" OBJ.OBJECT_TYPE = 'PK' OR                   ");
            sb.Append(" OBJ.OBJECT_TYPE = 'UC' OR                   ");
            sb.Append(" OBJ.OBJECT_TYPE = 'IX' OR                   ");
            sb.Append(" OBJ.OBJECT_TYPE = 'SL') AND                  ");
            sb.Append(" NOT ( OBJ.SCHEMA_NAME = '@MAINTAIN_SCHEMA@' OR                      ");
            sb.Append(" OBJ.SCHEMA_NAME LIKE 'HP\\_%') ");
            sb.Append(" FOR READ UNCOMMITTED ACCESS  ");
            sb.Append(" ) AS CP");
            return sb.ToString();
        }

        static public string SelectColumnPrivilegesForUser(string granteeName, string catalogName)
        {
            StringBuilder sb = new StringBuilder();
            sb.Append(" SELECT CATALOG_NAME, SCHEMA_NAME, OBJECT_NAME, OBJECT_TYPE, PRIVILEGE_TYPE, COLUMN_NAME, AUTHNAME(GRANTOR) AS GRANTOR, IS_GRANTABLE ");
            sb.Append(" FROM");
            sb.Append(" (");
            sb.Append(" SELECT COLP.CATALOG_NAME, COLP.SCHEMA_NAME, COLP.OBJECT_NAME,          ");
            sb.Append(" CASE COLP.OBJECT_TYPE            ");
            sb.Append(" WHEN 'BT' THEN 'TABLE'            ");
            sb.Append(" WHEN 'VI' THEN 'VIEW'");
            sb.Append(" WHEN 'MV' THEN 'MV'            ");
            sb.Append(" WHEN 'SY' THEN 'SYNONYM'            ");
            sb.Append(" ELSE  COLP.OBJECT_TYPE          ");
            sb.Append(" END AS OBJECT_TYPE,  	");
            sb.Append(" COLP.GRANTOR_LL AS GRANTOR,          ");
            sb.Append(" COLP.PRIVILEGE_TYPE,          ");
            sb.Append(" COLP.COLUMN_NAME , ");
            sb.Append(" CASE IS_GRANTABLE ");
            sb.Append(" WHEN 'Y' THEN 'YES'   ");
            sb.Append(" ELSE 'NO'  ");
            sb.Append(" END AS IS_GRANTABLE");
            sb.AppendFormat(" FROM {0}.TRAFODION_INFORMATION_SCHEMA.OBJECTS_COLUMN_PRIVILEGES COLP,              ", catalogName);
            if (_workMode == WorkMode.UserMode)
            {
                sb.AppendFormat(" {0}.TRAFODION_INFORMATION_SCHEMA.USER_INFO UI         ", catalogName);
                sb.AppendFormat(" WHERE UI.USER_NAME = '{0}' AND               ", granteeName);
                sb.Append(" COLP.GRANTEE_LL = UI.USER_ID AND               ");
            }
            else if (_workMode == WorkMode.RoleMode)
            {
                sb.AppendFormat(" {0}.TRAFODION_INFORMATION_SCHEMA.ROLE_INFO RI         ", catalogName);
                sb.AppendFormat(" WHERE RI.ROLE_NAME = '{0}' AND               ", granteeName);
                sb.Append(" COLP.GRANTEE_LL = RI.ROLE_ID AND               ");
            }
            sb.Append(" NOT (COLP.SCHEMA_NAME = '@MAINTAIN_SCHEMA@' OR                      ");
            sb.Append(" COLP.SCHEMA_NAME LIKE 'HP\\_%')   ");
            sb.Append(" FOR READ UNCOMMITTED ACCESS ");
            sb.Append(" ) AS CP");
            return sb.ToString();
        }

        /// <summary>
        /// Get the columns for the grid datatable by reading the result set metadata
        /// You get the column name, type, precision and scale
        /// Will not work if result set has interval columns
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="gridDataTable"></param>
        void GetColumnsFromResultSetMetatData(OdbcDataReader reader, ref DataTable gridDataTable)
        {
            DataTable schemaTable = reader.GetSchemaTable();
            gridDataTable.Columns.Clear();

            foreach (DataRow row in schemaTable.Rows)
            {
                string colName = row["ColumnName"] as string;
                Type type = (Type)row["DataType"];
                short precision = (short)row["NumericPrecision"];
                short scale = (short)row["NumericScale"];

                DataColumn col = new DataColumn();
                if (gridDataTable.Columns.Contains(colName))
                {
                    col.Caption = colName;
                }
                else
                {
                    col.ColumnName = colName;
                }
                col.DataType = (type != null) ? type : typeof(System.Object);
                if (type == typeof(System.DateTime))
                {
                    if (precision == 10)
                    {
                        col.ExtendedProperties.Add("SQL_TYPE", "DATE");
                    }
                    else
                    {
                        col.ExtendedProperties.Add("SQL_TYPE", "TIMESTAMP");
                        col.ExtendedProperties.Add("SCALE", scale);
                    }
                }
                if (type == typeof(System.TimeSpan))
                {
                    col.ExtendedProperties.Add("SQL_TYPE", "TIME");
                    col.ExtendedProperties.Add("SCALE", scale);
                }
                gridDataTable.Columns.Add(col);
            }
        }

        /// <summary>
        /// This is a fallback approach to create the columns of the grid data table
        /// Iterate through the result set column set and create a data column
        /// using the result column's name and datatype
        /// If an exception is thrown like for interval datatypes, treat it as a string column
        /// </summary>
        /// <param name="reader"></param>
        /// <param name="gridDataTable"></param>
        void CreateColumnsFromResultSet(OdbcDataReader reader, ref DataTable gridDataTable)
        {
            int theFieldCount = reader.FieldCount;
            for (int colNum = 0; colNum < theFieldCount; colNum++)
            {
                string colName = reader.GetName(colNum);
                try
                {
                    System.Type type = reader.GetFieldType(colNum);
                    string typeName = reader.GetDataTypeName(colNum);
                    if (gridDataTable.Columns.Contains(colName))
                    {
                        DataColumn column = gridDataTable.Columns.Add();
                        column.Caption = colName;
                        column.DataType = type;
                        column.ExtendedProperties.Add("SQL_TYPE", typeName);
                    }
                    else
                    {
                        DataColumn column = gridDataTable.Columns.Add(colName, type);
                        column.ExtendedProperties.Add("SQL_TYPE", typeName);
                    }
                }
                catch (Exception ex)
                {
                    if (Logger.IsTracingEnabled)
                    {
                        Logger.OutputToLog(
                            TraceOptions.TraceOption.DEBUG,
                            TraceOptions.TraceArea.UserManagement,
                            TRACE_SUB_AREA_NAME,
                            "Create columns from resultset exception: " + ex.Message);
                    }

                    if (gridDataTable.Columns.Contains(colName))
                    {
                        DataColumn column = gridDataTable.Columns.Add();
                        column.Caption = colName;
                    }
                    else
                    {
                        gridDataTable.Columns.Add(new DataColumn(colName));
                    }
                }
            }
        }

    }
}
