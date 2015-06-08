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
using System.Linq;
using System.Collections.Generic;
using System.Data.Odbc;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// This class is used to assist TrafodionSchemaObjects to implement the IHasTrafodionColumns interface.
    /// It handles retrieving columns as well as efficiently retrieving column counts without having
    /// to populate itself with column objects unnecessarily.
    /// </summary>
    /// <typeparam name="ColumnType">The type of column that this delegate will handle.</typeparam>
    public class DefaultHasTrafodionColumns<ColumnType> : IHasTrafodionColumns
        where ColumnType : TrafodionColumn, new()
    {
        /// <summary>
        /// This delegate is called when the columns are loaded.
        /// </summary>
        public delegate void OnLoadColumns();

        /// <summary>
        /// This delegate is called when the columns are cleared.
        /// </summary>
        public delegate void OnClearColumns();

        #region Fields

        private OnLoadColumns _onColumnsLoad = null;
        private OnClearColumns _onClearColumns = null;

        private TrafodionSchemaObject _schemaObject = null;
        private List<TrafodionColumn> _columns = null;
        private int _columnCount = -1;
        private List<ColumnPrivilege> _columnPrivileges = null;

        #endregion

        #region Properties

        /// <summary>
        /// The number of columns that is associated with this object. Call
        /// this function if you do not actually need the columns but only
        /// the count as it runs really fast!
        /// </summary>
        public int ColumnCount
        {
            get
            {
                // Return the number of columns from the retrieved list if we have it.
                if (_columns != null)
                {
                    _columnCount = _columns.Count;
                }
                else if (_columnCount < 0)
                {
                    // Call the more efficient means of finding the column count.
                    Connection connection = null;

                    try
                    {
                        connection = _schemaObject.GetConnection();
                        OdbcDataReader dataReader = Queries.ExecuteSelectColumnCount(connection, _schemaObject.TheTrafodionSchema, _schemaObject);
                        if (dataReader.Read())
                        {
                            _columnCount = dataReader.GetInt32(0);
                        }
                    }
                    finally
                    {
                        if (connection != null)
                        {
                            connection.Close();
                        }
                    }
                }

                return _columnCount;
            }
        }

        /// <summary>
        /// A list of columns.
        /// </summary>
        public List<TrafodionColumn> Columns
        {
            get
            {
                if (_columns == null)
                {
                    // Retrieve the list of ColType elements.
                    List<ColumnType> columnTypeList = null;

                    if (_schemaObject is TrafodionRoutine && _schemaObject.TheTrafodionSchema.Version >= 2500)
                    {
                        columnTypeList = new RoutinesColumnLoader<ColumnType>().Load(_schemaObject);
                    }
                    else
                    {
                       columnTypeList = new ColumnsLoader<ColumnType>().Load(_schemaObject);
                    }

                    // Convert to a list of TrafodionColumn elements.
                    _columns = columnTypeList.ConvertAll<TrafodionColumn>(delegate(ColumnType column)
                        {
                            return column as TrafodionColumn;
                        });

                    // Call the OnLoad delegate for any special processing.
                    if (OnLoadColumnsDelegate != null)
                    {
                        OnLoadColumnsDelegate();
                    }
                }

                return _columns;
            }
        }

        /// <summary>
        /// The column privileges for this schema object.
        /// </summary>
        public List<ColumnPrivilege> ColumnPrivileges
        {
            get
            {
                if (_columnPrivileges == null)
                {
                    _columnPrivileges = PrivilegesLoader.LoadColumnPrivileges(_schemaObject);
                }
                return _columnPrivileges;
            }
        }

        /// <summary>
        /// The function that is called when the columns are loaded.
        /// </summary>
        public OnLoadColumns OnLoadColumnsDelegate
        {
            private get
            {
                return _onColumnsLoad;
            }

            set
            {
                _onColumnsLoad = value;
            }
        }

        /// <summary>
        /// The function that is called when the columns are cleared.
        /// </summary>
        public OnClearColumns OnClearColumnsDelegate
        {
            private get
            {
                return _onClearColumns;
            }

            set
            {
                _onClearColumns = value;
            }
        }

        #endregion

        /// <summary>
        /// Constructs a new object.
        /// </summary>
        /// <param name="schemaObject">The schema object that this object is supporting.</param>
        public DefaultHasTrafodionColumns(TrafodionSchemaObject schemaObject)
        {
            _schemaObject = schemaObject;
        }

        /// <summary>
        /// Removes all column information.
        /// </summary>
        public void ClearColumns()
        {
            _columns.Clear();
            _columns = null;
            _columnCount = -1;

            if (OnClearColumnsDelegate != null)
            {
                OnClearColumnsDelegate();
            }
        }

        public void ResetColumnPrivileges()
        {
            if (_columnPrivileges != null)
            {
                _columnPrivileges.Clear();
                _columnPrivileges = null;
            }
        }

        /// <summary>
        /// Checks if the given user has the specificed privilege type on this column
        /// </summary>
        /// <param name="userName">User name</param>
        /// <param name="privilegeType">Privilege Type</param>
        /// <returns>True or False</returns>
        public bool DoesUserHaveColumnPrivilege(string userName, string columnName, string privilegeType)
        {
            IEnumerable<ColumnPrivilege> userColumnPrivileges = (from priv in ColumnPrivileges 
                                                      where priv.ColumnName.Equals(columnName) && 
                                                      (priv.GranteeName.Equals(userName) || priv.GranteeName.ToUpper().Equals("PUBLIC")) &&
                                                      priv.DoesPrivilegExist(privilegeType)
                                                      select priv).Distinct();
            return userColumnPrivileges.Count<ColumnPrivilege>() > 0;
        }


        #region ColumnLoader

        /// <summary>
        /// Loads the columns.
        /// </summary>
        /// <typeparam name="ColType">The type of column to be loaded.</typeparam>
        class ColumnsLoader<ColType> : TrafodionObjectsLoader<TrafodionSchemaObject, ColType>
            where ColType : TrafodionColumn, new()
        {
            #region Static Fields/Methods

            // These must match the constants above
            const string theV1200TrafodionColumnsColumnNames =
            "COLUMN_NAME,"
            + "OBJECT_UID,"
            + "COLUMN_NUMBER,"
            + "DIRECTION,"
            + "COLUMN_CLASS,"
            + "COLUMN_SIZE,"
            + "FS_DATA_TYPE,"
            + "CHARACTER_SET,"
            + "ENCODING,"
            + "COLLATION_SEQUENCE,"
            + "FS_DATA_TYPE,"
            + "COL_SCALE,"
            + "COL_PRECISION,"
            + "UPSHIFTED,"
            + "NULL_HEADER_SIZE,"
            + "VARLEN_HEADER_SIZE,"
            + "DEFAULT_CLASS,"
            + "LOGGABLE,"
            + "DATETIME_START_FIELD,"
            + "DATETIME_END_FIELD,"
            + "DATETIME_LEADING_PRECISION,"
            + "DATETIME_TRAILING_PRECISION,"
            + "DATETIME_QUALIFIER,"
            + "DEFAULT_VALUE,"
            + "HEADING_TEXT,"
            + "PICTURE_TEXT,"
            + "COLUMN_VALUE_DRIFT_PER_DAY"
            ;

            // These must match the constants above
            const string theV2000TrafodionColumnsColumnNames =
            theV1200TrafodionColumnsColumnNames + ","
            + "DATE_DISPLAY_FORMAT,"
            + "CASE_SENSITIVE_COMPARISON,"
            + "DISPLAY_DATA_TYPE"
            ;

            /// <summary>
            /// Get list of column name list as a comma separated string
            /// </summary>
            /// <param name="aTrafodionSchemaVersion">Schema version</param>
            /// <returns>A comma separated string value</returns>
            static public String GetColumnsColumnNames(int aTrafodionSchemaVersion)
            {
                //return (aTrafodionSchemaVersion == 1200) ? theV1200TrafodionColumnsColumnNames : theV2000TrafodionColumnsColumnNames;
                return "COLUMN_NAME, COLUMN_NUMBER, SQL_DATA_TYPE, FS_DATA_TYPE, COLUMN_SIZE, "
                    + "COLUMN_SCALE, COLUMN_PRECISION, NULLABLE, CHARACTER_SET, DATETIME_START_FIELD, DATETIME_END_FIELD,"
                    + "DT_CODE, DATETIME_QUALIFIER, DEFAULT_VALUE"
                    ;

            }

            #endregion

            /// <summary>
            /// Returns the reader which contains information to be loaded from the metadata tables.
            /// </summary>
            /// <param name="connection">The connection used to connect to the system.</param>
            /// <param name="schemaObj">The schema object whose columns are to be loaded.</param>
            /// <returns></returns>
            override protected OdbcDataReader GetQueryReader(Connection connection, TrafodionSchemaObject schemaObj)
            {
                /*if (schemaObj.SchemaObjectType.Equals(TrafodionView.ObjectType))
                {
                    return Queries.ExecuteSelectViewColumnInfo(connection, schemaObj); 
                }
                else
                {
                    return Queries.ExecuteSelectTableColumnInfo(connection, schemaObj);
                }*/
                return Queries.ExecuteSelectSchemaObjectColumnInfo(connection, schemaObj);
            }

            /// <summary>
            /// Retrieve the columns 
            /// </summary>
            /// <param name="list"></param>
            /// <param name="schemaObj"></param>
            /// <param name="reader"></param>
            override protected void LoadOne(List<ColType> list, TrafodionSchemaObject schemaObj, OdbcDataReader reader)
            {
                ColType column = new ColType();

                column.TrafodionSchemaObject = schemaObj;
                /*return "COLUMN_NAME, COLUMN_NUMBER, SQL_DATA_TYPE, FS_DATA_TYPE, COLUMN_SIZE, "
                    + "COLUMN_SCALE, COLUMN_PRECISION, NULLABLE, CHARACTER_SET, DATETIME_START_FIELD, DATETIME_END_FIELD,"
                    + "DATETIME_QUALIFIER, DEFAULT_VALUE FROM {1}.\"_MD_\".COLUMNS_VIEW"
                    ;*/

                column.InternalName = reader.GetString(0).TrimEnd();
                column.TheColumnNumber = reader.GetInt32(1);
                column.TheSQLDataType = reader.GetString(2).Trim();
                column.TheFSDataType = reader.GetInt32(3);
                column.TheColumnSize = reader.GetInt32(4);
                column.TheColScale = reader.GetInt32(5);
                column.TheColPrecision = reader.GetInt32(6);
                column.IsNullable = reader.GetBoolean(7);
                column.TheCharacterSet = reader.GetString(8).Trim();
                column.TheDatetimeStartField = reader.GetInt32(9);
                column.TheDatetimeEndField = reader.GetInt32(10);
                column.TheDatetimeQualifier = reader.GetString(11).Trim();
                column.TheDefaultValue = reader.GetString(12).Trim();

                list.Add(column);
            }
        }

        #endregion

        #region RoutinesColumnLoader
        /// <summary>
        /// Loads the columns.
        /// </summary>
        /// <typeparam name="ColType">The type of column to be loaded.</typeparam>
        class RoutinesColumnLoader<ColType> : ColumnsLoader<ColType> where ColType : TrafodionColumn, new()
        {
            #region Static Fields/Methods

            // These must match the constants above
            const string theV2500TrafodionRoutineColumnNames =
                "PARAM_NAME,"
                 + "ROUTINE_UID,"
                 + "PARAM_NUMBER,"
                 + "DIRECTION,"
                 + "PARAM_SIZE,"
                 + "SQL_DATA_TYPE,"
                 + "CHARACTER_SET,"
                 + "ENCODING,"
                 + "COLLATION_SEQUENCE,"
                 + "FS_DATA_TYPE,"
                 + "PARAM_SCALE,"
                 + "PARAM_PRECISION,"
                 + "UPSHIFTED,"
                 + "NULL_HEADER_SIZE,"
                 + "VARLEN_HEADER_SIZE,"
                 + "PARAM_DEFAULT_CLASS,"
                 + "DATETIME_START_FIELD,"
                 + "DATETIME_END_FIELD,"
                 + "DATETIME_LEADING_PRECISION,"
                 + "DATETIME_TRAILING_PRECISION,"
                 + "DATETIME_QUALIFIER,"
                 + "DEFAULT_VALUE,"
                 + "HEADING_TEXT,"
                 + "DATE_DISPLAY_FORMAT,"
                 + "CASE_SENSITIVE_COMPARISON,"
                 + "DISPLAY_DATA_TYPE,"
                 + "ROUTINE_PARAM_TYPE,"
                 + "IS_OPTIONAL,"
                 + "OUTPUT_UEC"
            ;

            /// <summary>
            /// Get list of column name list as a comma separated string
            /// </summary>
            /// <param name="aTrafodionSchemaVersion">Schema version</param>
            /// <returns>A comma separated string value</returns>
            static public new String GetColumnsColumnNames(int aTrafodionSchemaVersion)
            {
                return theV2500TrafodionRoutineColumnNames;
            }

            #endregion

            /// <summary>
            /// Returns the reader which contains information to be loaded from the metadata tables.
            /// </summary>
            /// <param name="connection">The connection used to connect to the system.</param>
            /// <param name="schemaObj">The schema object whose columns are to be loaded.</param>
            /// <returns></returns>
            override protected OdbcDataReader GetQueryReader(Connection connection, TrafodionSchemaObject schemaObj)
            {
                return Queries.ExecuteSelectRoutineColumnInfo(
                    connection,
                    GetColumnsColumnNames(schemaObj.TheTrafodionSchema.Version),
                    ((TrafodionRoutine)schemaObj)
                    );
            }

            /// <summary>
            /// Retrieve the columns 
            /// </summary>
            /// <param name="list"></param>
            /// <param name="schemaObj"></param>
            /// <param name="reader"></param>
            override protected void LoadOne(List<ColType> list, TrafodionSchemaObject schemaObj, OdbcDataReader reader) 
            {
                ColType column = new ColType();

                column.TrafodionSchemaObject = schemaObj;

                column.InternalName = reader.GetString(0).TrimEnd();
                column.UID = reader.GetInt64(1);
                column.TheColumnNumber = reader.GetInt32(2);
                column.TheDirection = reader.GetString(3).Trim();
                column.TheColumnSize = reader.GetInt32(4);
                column.TheSQLDataType = reader.GetString(5).Trim();
                column.TheCharacterSet = reader.GetString(6).Trim();

                column.TheEncoding = reader.GetString(7).Trim();
                column.TheCollationSequence = reader.GetString(8).Trim();
                column.TheFSDataType = reader.GetInt32(9);
                column.TheColScale = reader.GetInt32(10);
                column.TheColPrecision = reader.GetInt32(11);
                column.IsUpshifted = reader.GetString(12).Trim().Equals("Y");
                column.TheNullHeaderSize = reader.GetInt16(13);
                column.IsNullDroppable = (column.TheNullHeaderSize != 0);
                column.IsNullable = column.IsNullDroppable;
                column.TheVarlenHeaderSize = reader.GetInt32(14);
                column.TheDefaultClass = reader.GetString(15).Trim();
                column.TheDatetimeStartField = reader.GetInt32(16);
                column.TheDatetimeEndField = reader.GetInt32(17);
                column.TheDatetimeLeadingPrecision = reader.GetInt32(18);
                column.TheDatetimeTrailingPrecision = reader.GetInt32(19);
                column.TheDatetimeQualifier = reader.GetString(20).Trim();
                column.TheDefaultValue = reader.GetString(21).Trim();
                column.TheHeadingText = reader.GetString(22).Trim();
                column.TheDateDisplayFormat = reader.GetString(23).Trim();
                column.CaseSensitiveComparison = reader.GetString(24).Equals("Y");
                column.IsDisplayDataType = reader.GetString(25).Equals("Y");
                column.RoutineParameterType = reader.GetString(26);
                column.IsOptional = reader.GetString(27).Equals("Y");
                column.OutputUEC = reader.GetInt64(28);
                //If the column is of type UCS2, then the column size is really 
                //half the size of what is defined in the metadata
                if (column.TheCharacterSet.Equals("UCS2"))
                {
                    column.TheColumnSize = column.TheColumnSize / 2;
                }
                else if (column.TheCharacterSet.Equals("UTF8") || column.TheCharacterSet.Equals("SJIS"))
                {
                    if (column.TheColPrecision > 0)
                    {
                        column.TheColumnSize = column.TheColPrecision;
                    }
                }

                list.Add(column);
            }
        }
        #endregion RoutinesColumnLoader
    }
}
