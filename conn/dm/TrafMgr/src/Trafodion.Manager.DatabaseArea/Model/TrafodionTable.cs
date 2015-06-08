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
using System.Data.Odbc;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// The model object for a Sql Table.
    /// </summary>
    public class TrafodionTable : IndexedSchemaObject, IHasTrafodionColumns
    {
        #region Constants

        // These must match the constants above
        const string theV1200AttributesColumnNames =
        "CLUSTERING_SCHEME,"
        + "BLOCK_SIZE,"
        + "KEY_LENGTH,"
        + "LOCK_LENGTH,"
        + "AUDIT_COMPRESS,"
        + "CLEAR_ON_PURGE,"
        + "BUFFERED,"
        + "DATA_COMPRESSED,"
        + "INDEX_COMPRESSED,"
        + "AUDITED,"
        + "PARTITIONING_SCHEME"
        ;

        // These must match the constants above
        const string theV2000AttributesColumnNames =
        theV1200AttributesColumnNames + ","
        + "ROW_FORMAT,"
        + "INSERT_MODE,"
        + "RECORD_SIZE,"
        + "MAX_TABLE_SIZE"
        ;

        /// <summary>
        /// The Table namespace
        /// </summary>
        public const string ObjectNameSpace = "TA";

        /// <summary>
        /// The table object type
        /// </summary>
        public const string ObjectType = "BT";

        /// <summary>
        /// Name for Maintain Update stats operation
        /// </summary>
        public const string MaintainUpdateStatsOperationName = "UPD_TABLE_STATS";

        /// <summary>
        /// Name for Maintain reorg operation
        /// </summary>
        public const string MaintainReorgOperationName = "REORG";

        /// <summary>
        /// Name of catalog that has the view for maintain information
        /// </summary>
        public const string MaintainViewCatalogName = "TRAFODION";

        /// <summary>
        /// Status of the stats operation
        /// </summary>
        public const String MaintainStatusDisabled = "DISABLE";

        #endregion

        #region Fields

        private bool _isNotNullAttributesLoaded = false;
        private bool _isPrimaryKeyLoaded = false;
        private bool _areAttributesLoaded = false;
        private bool _isInsertLogStatusLoaded = false;
        private bool _isMaintainStatusLoaded = false;
        private bool _isLastUpdateStatsTimestampLoaded = false;
        private bool _areConstraintsLoaded = false;
        private bool _areCheckConstraintsLoaded = false;
        private bool _areUniqueConstraintsLoaded = false;
        private bool _areForeignKeysLoaded = false;
        private bool _isRowCountFetched = false;
        private long _blockSize;
        private int _keyLength;
        private int _lockLength;
        private bool _isAuditCompress;
        private bool _isClearOnPurge;
        private bool _isBuffered;
        private bool _isDataCompressed;
        private bool _isIndexCompressed;
        private bool _isAudited;
        private string _partitioningScheme;
        private string _rowFormat = "";
        private string _insertMode = "";
        private int _recordSize = 0;
        private long _maxTableSize = 0;
        private bool _isInsertLog;
        private bool _isReorgEnabled;
        private bool _isUpdateStatsEnabled;
        private long _lastUpdateStatsTimestamp;
        private string _clusteringScheme;
        private long _rowCount;
        private long _totalCurrentSize;
        private double _percentAllocated;


        private DefaultHasTrafodionColumns<TrafodionTableColumn> _columnsDelegate;
        private List<TrafodionTrigger> _sqlMxTableTriggers = null;
        private List<TrafodionSynonym> _sqlMxSynonyms = null;
        private List<TrafodionMaterializedView> _sqlMxMaterializedViews = null;
        private List<TrafodionView> _sqlMxViews = null;
        private List<TrafodionHashKeyColumnDef> _TrafodionHashKeyColumnDefs = null;

        #endregion


        /// <summary>
        /// Constructs the table model
        /// </summary>
        /// <param name="aTrafodionSchema">Parent schema</param>
        /// <param name="anInternalName">Internal name of the table</param>
        /// <param name="aUID">UID of the table</param>
        /// <param name="aCreateTime">Table creation time</param>
        /// <param name="aRedefTime">Table redefinition time</param>
        /// <param name="aSecurityClass">Security class of the table</param>
        /// <param name="anOwner">The owner of the object.</param>
        public TrafodionTable(TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
            : base(aTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
        {
            
            _columnsDelegate = new DefaultHasTrafodionColumns<TrafodionTableColumn>(this);

            //Find the not null columns and update their attribute
            _columnsDelegate.OnLoadColumnsDelegate = LoadNotNullAttributeForColumns;
        }

        /// <summary>
        /// Clears the columns from the table.
        /// </summary>
        public void ClearColumns()
        {
            _isNotNullAttributesLoaded = false;
            _isPrimaryKeyLoaded = false;
            _columnsDelegate.ClearColumns();
        }

        /// <summary>
        /// Clear the list of triggers on the table
        /// </summary>
        public void ClearTriggers()
        {
            _sqlMxTableTriggers = null;
        }

        /// <summary>
        /// Loads the not null attribute for columns 
        /// </summary>
        private void LoadNotNullAttributeForColumns()
        {
            //If not null attributes are already loaded, return
            if (_isNotNullAttributesLoaded)
            {
                return;
            }

            //Load the not null constraints, if they are not already loaded
            LoadTableConstraints();

            Connection theConnection = null;

            try
            {
                theConnection = GetConnection();

                //For each not null constraint defined on the table, find the column names
                //that are part of the not null constraint and set their not null attribute
                //Make a copy to avoid the recursive update of the list.
                List<TrafodionTableConstraint> constraintsCopy = new List<TrafodionTableConstraint>(theNotNullCheckConstraints);

                foreach (TrafodionTableConstraint theNotNullConstraint in constraintsCopy)
                {
                    OdbcDataReader theReader = Queries.ExecuteSelectTableNotNullColumns(theConnection, TheTrafodionCatalog.ExternalName, TheTrafodionSchema.Version, theNotNullConstraint.UID, this.UID);
                    while (theReader.Read())
                    {
                        string columnName = theReader.GetString(0).TrimEnd();
                        TrafodionColumn theTrafodionColumn = FindColumnByInternalName(columnName);
                        theTrafodionColumn.IsNullable = false;
                    }
                }
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
            _isNotNullAttributesLoaded = true;
        }

        /// <summary>
        /// Loads the table attributes
        /// </summary>
        private void LoadAttributes()
        {
            // If attributes already loaded, just return
            if (_areAttributesLoaded)
            {
                return;
            }

            Connection theConnection = null;

            //try
            //{
            //    theConnection = GetConnection();

            //    OdbcDataReader theReader = Queries.ExecuteSelectAccessPathsInfo(theConnection, TrafodionTable.GetAttributesColumnNames(TheTrafodionSchema.Version), TheTrafodionCatalog, TheTrafodionSchema.UID, TheTrafodionSchema.Version, UID);

            //    if(theReader.Read())
            //    {
            //        _clusteringScheme = theReader.GetString(2).Trim();
            //        _blockSize = theReader.GetInt32(3);
            //        _keyLength = theReader.GetInt16(4);
            //        _lockLength = theReader.GetInt16(5);
            //        _isAuditCompress = theReader.GetString(6).Trim().Equals("Y");
            //        _isClearOnPurge = theReader.GetString(7).Trim().Equals("Y");
            //        _isBuffered = theReader.GetString(8).Trim().Equals("Y");
            //        _isDataCompressed = theReader.GetString(9).Trim().Equals("Y");
            //        _isIndexCompressed = theReader.GetString(10).Trim().Equals("Y");
            //        _isAudited = theReader.GetString(11).Trim().Equals("Y");
            //        _partitioningScheme = theReader.GetString(12).Trim();
            //        if (TheTrafodionSchema.Version >= 2000)
            //        {
            //            _rowFormat = theReader.GetString(13).Trim();
            //            _insertMode = theReader.GetString(14).Trim();
            //            _recordSize = theReader.GetInt16(15);
            //            _maxTableSize = theReader.GetInt32(16);
            //        }
            //    }

            //}
            //finally
            //{
            //    if (theConnection != null)
            //    {
            //        theConnection.Close();
            //    }
            //}
            _areAttributesLoaded = true;
        }

        /// <summary>
        /// Load the last update statistics timestamp of the table
        /// </summary>
        private void LoadLastUpdateStatsTimestamp()
        {
            //If last update stats timestamp is already loaded, return
            if (_isLastUpdateStatsTimestampLoaded)
            {
                return ;
            }

            //if (!TrafodionName.IsASystemSchemaName(TheTrafodionSchema.InternalName))
            // Get the last update stats timestamp from HISTOGRAMS table.  But, only do it if this is not a Metadata table. 
            if (!IsMetadataObject)
            {
                Connection theConnection = null;

                try
                {
                    theConnection = GetConnection();

                    OdbcDataReader theReader = Queries.ExecuteSelectTableLastUpdateStatsTime(theConnection, TheTrafodionCatalog.ExternalName, TheTrafodionSchema.ExternalName, UID);

                    if (theReader.Read())
                    {
                        _lastUpdateStatsTimestamp = theReader.GetInt64(0); //julian timestamp, a long value
                    }
                }
                finally
                {
                    if (theConnection != null)
                    {
                        theConnection.Close();
                    }
                }
            }

            _isLastUpdateStatsTimestampLoaded = true;
        }

        /// <summary>
        /// Load the attribute that indicates if only inserts into the table are logged
        /// </summary>
        private void LoadInsertLogStatus()
        {
            //If insert log information is already loaded, return
            if (_isInsertLogStatusLoaded)
            {
                return ;
            }

            Connection theConnection = null;
            _isInsertLog = false;

            try
            {
                theConnection = GetConnection();

                OdbcDataReader theReader = Queries.ExecuteSelectTableInsertLogStatus(theConnection, TheTrafodionCatalog.ExternalName, TheTrafodionSchema.Version, UID);
                if (theReader.Read())
                {
                    _isInsertLog = theReader.GetString(0).Trim().Equals("Y");
                }
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }

            _isInsertLogStatusLoaded = true;
        }

        /// <summary>
        /// Load the enable/disable status of reorg and updatestats
        /// </summary>
        private void LoadMaintainStatus()
        {
            //If the maintain status is already loaded, return
            if (_isMaintainStatusLoaded)
            {
                return ;
            }

            //Ignore metadata tables and set the status to false 
            if (IsMetadataObject)
            {
                _isMaintainStatusLoaded = true;
                _isReorgEnabled = false;
                _isUpdateStatsEnabled = false;
                return ;
            }

            Connection theConnection = null;
            _isReorgEnabled = true;
            _isUpdateStatsEnabled = true;
            string disableStatus = "DISABLE";

            try
            {
                theConnection = GetConnection();

                OdbcDataReader theReader = Queries.ExecuteSelectTableMaintainStatus(theConnection, TheTrafodionCatalog.InternalName, 
                                            TheTrafodionSchema.InternalName, InternalName);

                while (theReader.Read())
                {
                    string operationName = theReader.GetString(0).TrimEnd();
                    string operationStatus = theReader.GetString(1).Trim();

                    if (operationName.Equals(MaintainReorgOperationName))
                    {
                        //If status equals disabled, then reorg enabled is false
                        _isReorgEnabled = !operationStatus.Equals(disableStatus);
                    }
                    else
                    if (operationName.Equals(MaintainUpdateStatsOperationName))
                    {
                        //If status equals disabled, then updatestats enabled is false
                        _isUpdateStatsEnabled = !operationStatus.Equals(disableStatus);
                    }
                }
                _isMaintainStatusLoaded = true;
            }
            catch (Exception)
            {
                //close the connection so it is returned to cache and gets reused for the fallback invocation
                if (theConnection != null)
                {
                    theConnection.Close();
                }
                //If there is an exception in accessing the Maintain view able, use a fallback approach
                //and fetch the status by explicity executing the maintain utility
                LoadMaintainStatusUsingFallbackApproach();
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }
        /// <summary>
        /// If there is an exception in accessing the Maintain view able, the fallback approach is used to 
        /// fetch the status by explicity executing the maintain utility
        /// </summary>
        private void LoadMaintainStatusUsingFallbackApproach()
        {
            Connection theConnection = null;
            _isReorgEnabled = true;
            _isUpdateStatsEnabled = true;
            string disableStatus = "Disabled";

            try
            {
                theConnection = GetConnection();

                OdbcDataReader theReader = Queries.ExecuteFallbackSelectMaintainStatus(theConnection, RealAnsiName);
                while (theReader.Read())
                {
                    string operationStatus = theReader.GetString(0).Trim();
                    if (operationStatus.Contains(MaintainReorgOperationName))
                    {
                        _isReorgEnabled = !operationStatus.Contains(disableStatus);
                    }
                    else
                        if (operationStatus.Contains(MaintainUpdateStatsOperationName))
                    {
                        _isUpdateStatsEnabled = !operationStatus.Contains(disableStatus);
                    }
                }
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
            _isMaintainStatusLoaded = true;
        }

        /// <summary>
        /// Load the enable/disable status of reorg and updatestats
        /// </summary>
        private void LoadRowCount()
        {
            //If the row count is already loaded, return
            if (_isRowCountFetched)
            {
                return;
            }

            //Ignore metadata tables and set the status to true 
            if (IsMetadataObject)
            {
                _isRowCountFetched = true;
                return;
            }

            Connection theConnection = null;

            try
            {
                theConnection = GetConnection();

                OdbcDataReader theReader = Queries.ExecuteSelectTableRowCount(theConnection, this.RealAnsiName);

                if(theReader.Read())
                {
                    _rowCount = theReader.GetInt64(0);
                }
            }
            catch (Exception)
            {
                _rowCount = 0;
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }

        /// <summary>
        /// Reads the histogram statistics for all columns in table
        /// </summary>
        public void LoadTableStatistics()
        {
            Connection theConnection = null;

            try
            {
                theConnection = GetConnection();

                OdbcDataReader odbcDataReader = null;
                if (TheTrafodionSchema.Version >= 2300)
                    odbcDataReader = Queries.ExecuteSelectTableStatistics2300(theConnection, this);
                else
                    odbcDataReader = Queries.ExecuteSelectTableStatistics2000(theConnection, this);

                long tableRowCount = 0;
                while (odbcDataReader.Read())
                {
                    int columnNumber = odbcDataReader.GetInt32(0);
                    TrafodionTableColumn sqlMxTableColumn = FindColumnByColumnNumber(columnNumber);
                    if (sqlMxTableColumn != null)
                    {
                        // The histogram stats is stored in the table column model
                        TableColumnHistogramStats columnStatistics = new TableColumnHistogramStats(sqlMxTableColumn);
                        columnStatistics.SetAttributes(odbcDataReader);
                        tableRowCount = (tableRowCount > columnStatistics.RowCount ? tableRowCount : columnStatistics.RowCount);
                        sqlMxTableColumn.HistogramStatistics = columnStatistics;
                    }
                }

                LoadRowCount(); 
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
        }

        /// <summary>
        /// Load all the table constraints
        /// </summary>
        private void LoadTableConstraints()
        {
            //If table constraints already loaded, return

            lock (this)
            {
                if (_areConstraintsLoaded)
                {
                    return;
                }

                Connection theConnection = null;

                try
                {
                    theConnection = GetConnection();
                    theUniqueConstraints.Clear();
                    theNotNullCheckConstraints.Clear();
                    theCheckConstraints.Clear();
                    theForeignKeyConstraints.Clear();
                    theTrafodionPrimaryKey = null;

                    OdbcDataReader theReader = Queries.ExecuteSelectFromTBL_CONSTRAINTS(theConnection, TheTrafodionCatalog.ExternalName, TheTrafodionSchema.InternalName, UID);

                    string constraintName;
                    long constraintUID;
                    char constraintType;
                    bool isConstraintDroppable;
                    long constraintIndexUID;
                    string objectType;
                    string enforced = "Y";
                    bool boolEnforced = true;

                    while (theReader.Read())
                    {
                        //SELECT T1.CONSTRAINT_UID, T1.CONSTRAINT_TYPE, T1.INDEX_UID, T2.OBJECT_NAME, T2.OBJECT_TYPE"
                        constraintUID = theReader.GetInt64(0);
                        constraintType = theReader.GetString(1)[0];
                        constraintIndexUID = theReader.GetInt64(2);
                        constraintName = theReader.GetString(3).Trim();
                        objectType = theReader.GetString(4).Trim();


                        TrafodionTableConstraint tableConstraint = null;

                        switch (constraintType)
                        {

                            case 'P': // Primary Key constraint
                                {
                                    tableConstraint = new TrafodionPrimaryKey(constraintName, constraintUID, this);
                                    break;
                                }
                            case 'F':
                                {
                                    tableConstraint = new TrafodionForeignKey(constraintName, constraintUID, true, this);
                                    break;
                                }
                            case 'C':
                                {
                                    tableConstraint = new TrafodionCheckConstraint(constraintName, constraintUID, this);
                                    break;
                                }
                            case 'U':
                                {
                                    tableConstraint = new TrafodionUniqueConstraint(constraintName, constraintUID, this);
                                    break;
                                }
                            default:
                                {
                                    tableConstraint = new TrafodionTableConstraint(constraintName, constraintUID, this);
                                    break;
                                }
                        }

                        tableConstraint.Type = constraintType;
                        tableConstraint.IndexUID = constraintIndexUID;
                        tableConstraint.ObjectType = objectType;

                        switch (constraintType)
                        {
                            case 'P': // Primary key constraint
                                {
                                    theTrafodionPrimaryKey = (TrafodionPrimaryKey)tableConstraint;
                                    break;
                                }
                            case 'C': // Check constraint
                                {
                                    theCheckConstraints.Add(tableConstraint as TrafodionCheckConstraint);
                                    if (tableConstraint.ObjectType.Trim().Equals("NN"))
                                        theNotNullCheckConstraints.Add(tableConstraint);
                                    break;
                                }
                            case 'U': // Unique constraint
                                {
                                    theUniqueConstraints.Add(tableConstraint as TrafodionUniqueConstraint);
                                    break;
                                }
                            case 'F': // Foreign key constraint
                                {
                                    theForeignKeyConstraints.Add((TrafodionForeignKey)tableConstraint);
                                    break;
                                }
                            default: // We are not interested in the other constraint types here
                                {
                                    break;
                                }
                        }
                    }
                }
                finally
                {
                    if (theConnection != null)
                    {
                        theConnection.Close();
                    }
                }
                _areConstraintsLoaded = true;
            }
        }

        /// <summary>
        /// Load the primary key column definition of the table
        /// </summary>
        internal void LoadPrimaryKeyColumns()
        {
            //If primary key column definition is already loaded, return
            if (_isPrimaryKeyLoaded)
            {
                return ;
            }

            //Load the primary key table constraints, if it has not been loaded already
            LoadTableConstraints();

            Connection theConnection = null;

            try
            {
                // Process the primary key constraint if any
                if (theTrafodionPrimaryKey != null)
                {

                    theConnection = GetConnection();

                    // Get the columns of the primary key

                    TrafodionPrimaryKeyColumnDef primaryKeyColumnDef;
                    TrafodionTableColumn tableColumnDef;
                    string internalColumnName;
                    int positionInRow;
                    int columnNumber;
                    String sortOrder;
                    int partKeySeqNum;
                    int clusteringKeySeqNum;
                    String systemAddedColumn;

                    // Check to see if primary key is clustering key
                    if (ThePrimaryKeyIndexUID != 0)
                    {

                        // The primary key is not the clustering key
                        OdbcDataReader theReader = Queries.ExecuteSelectPrimaryKeyColumnInfo(theConnection, TheTrafodionCatalog.ExternalName, TheTrafodionSchema.Version, UID, ThePrimaryKeyIndexUID);

                        while (theReader.Read())
                        {
                            internalColumnName = theReader.GetString(0).TrimEnd();
                            sortOrder = theReader.GetString(1).Trim();
                            clusteringKeySeqNum = theReader.GetInt32(2);
                            systemAddedColumn = theReader.GetString(3).Trim();
                            columnNumber = theReader.GetInt32(4);

                            // Find the definition for this column
                            tableColumnDef = FindColumnByInternalName(internalColumnName);

                            // Create a primary key def for the column
                            primaryKeyColumnDef = new TrafodionPrimaryKeyColumnDef(tableColumnDef, sortOrder, clusteringKeySeqNum, systemAddedColumn, columnNumber);

                            // Link the column to the primary key column
                            tableColumnDef.TheTrafodionPrimaryKeyColumnDef = primaryKeyColumnDef;

                            // Add it to the list of primary key columns
                            TheTrafodionPrimaryKeyColumnDefs.Add(primaryKeyColumnDef);
                        }
                    }
                    else
                    {

                        // The primary key is the clustering key
                        OdbcDataReader theReader = Queries.ExecuteSelectPrimaryKeyColumnInfo(theConnection, TheTrafodionCatalog.ExternalName, TheTrafodionSchema.Version, UID, TheTrafodionPrimaryKey);

                        while (theReader.Read())
                        {
                            internalColumnName = theReader.GetString(0).TrimEnd();
                            positionInRow = theReader.GetInt32(1);
                            columnNumber = theReader.GetInt32(2);
                            sortOrder = theReader.GetString(3).Trim();
                            partKeySeqNum = theReader.GetInt32(4);
                            clusteringKeySeqNum = theReader.GetInt32(5);
                            systemAddedColumn = theReader.GetString(6).Trim();

                            // Find the definition for this column
                            tableColumnDef = FindColumnByInternalName(internalColumnName);

                            // Create a primary key def for the column
                            primaryKeyColumnDef = new TrafodionPrimaryKeyColumnDef(tableColumnDef, positionInRow, columnNumber, sortOrder, partKeySeqNum, clusteringKeySeqNum, systemAddedColumn);

                            // Link the column to the primary key column
                            tableColumnDef.TheTrafodionPrimaryKeyColumnDef = primaryKeyColumnDef;

                            // Add it to the list of primary key columns
                            TheTrafodionPrimaryKeyColumnDefs.Add(primaryKeyColumnDef);
                        }
                    }

                }
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
            _isPrimaryKeyLoaded = true;
 
        }

        /// <summary>
        /// Creates a Trigger model given the trigger name
        /// </summary>
        /// <param name="aTriggerName"></param>
        /// <returns></returns>
        internal TrafodionTrigger LoadTriggerByName(string aTriggerName)
        {
            return new TrafodionTableTriggersLoader().LoadObjectByName(this, aTriggerName);
        }

        /// <summary>
        /// Load the foreign key definitions of the table
        /// </summary>
        internal void LoadForeignKeys()
        {
            //If foreign key column definition is already loaded, return
            if (_areForeignKeysLoaded)
            {
                return;
            }

            //Load the table constraints, if it has not been loaded already
            LoadTableConstraints();

            Connection theConnection = null;

            try
            {
                    theConnection = GetConnection();

                    // Get the columns of each foreign key
                    foreach (TrafodionForeignKey theForeignKey in theForeignKeyConstraints)
                    {
                        // Define some temporary variables to share between queries
                        long theUniqueConstraintUID;
                        TrafodionCatalog theForeignCatalog;
                        TrafodionSchema theForeignSchema;

                        // Populate the Local Columns
                        OdbcDataReader theReader = Queries.ExecuteSelectConstraintColumnName(theConnection, TheTrafodionCatalog.ExternalName, 
                            TheTrafodionSchema.Version, theForeignKey.UID, this.UID);                                                
                        while (theReader.Read())
                        {
                            string columnName = theReader.GetString(0).TrimEnd();
                            int columnNumber = theReader.GetInt16(1);
                            TrafodionForeignKey.ForeignKeyLocalColumn theForeignKeyLocalColumn = new TrafodionForeignKey.ForeignKeyLocalColumn(columnName, columnNumber);
                            theForeignKey.TheForeignKeyLocalColumns.Add(theForeignKeyLocalColumn);
                        }
                        if (theConnection != null)
                        {
                            theConnection.Close();
                        }
                        //Populate Foreign Table's Catalog and Schema, and Unique Constraint UID
                        theReader = Queries.ExecuteSelectForeignKeyUniqueConstraint(theConnection, TheTrafodionCatalog.ExternalName, 
                            TheTrafodionSchema.Version, theForeignKey.UID);
                        theReader.Read();
                        // Find the catalog from the system.  Use the catalog to find the schema.  
                        theForeignCatalog = TheTrafodionCatalog.TrafodionSystem.FindCatalog(theReader.GetInt64(0));
                        theForeignSchema = theForeignCatalog.FindSchema(theReader.GetInt64(1));
                        theUniqueConstraintUID = theReader.GetInt64(2);
                        if (theConnection != null)
                        {
                            theConnection.Close();
                        }
                        //Populate the Foreign Table of the Foreign Key
                        theReader = Queries.ExecuteSelectConstraintForeignTable(theConnection, theForeignCatalog.ExternalName,
                            theForeignSchema.Version, theUniqueConstraintUID);
                        theReader.Read();
                        theForeignKey.ForeignTable = theForeignSchema.FindTable(theReader.GetString(0).TrimEnd());
                        if (theConnection != null)
                        {
                            theConnection.Close();
                        }                        
                        //Populate the Foreign Key's Foreign Columns
                        theReader = Queries.ExecuteSelectConstraintColumnName(theConnection, theForeignCatalog.ExternalName,
                            theForeignSchema.Version, theUniqueConstraintUID, theForeignKey.ForeignTable.UID);
                        while (theReader.Read())
                        {
                            string columnName = theReader.GetString(0).TrimEnd();
                            int columnNumber = theReader.GetInt16(1);
                            TrafodionForeignKey.ForeignKeyForeignColumn theForeignKeyForeignColumn = new TrafodionForeignKey.ForeignKeyForeignColumn(columnName, columnNumber);
                            theForeignKey.TheForeignKeyForeignColumns.Add(theForeignKeyForeignColumn);
                        }
                    }                
                }            
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
            _areForeignKeysLoaded = true;

        }

        /// <summary>
        /// Load the check constraint column definitions of the table
        /// </summary>
        internal void LoadCheckConstraints()
        {
            //If check constraints are already loaded, return
            if (_areCheckConstraintsLoaded)
            {
                return;
            }

            //Load the check constraints, if it has not been loaded already
            LoadTableConstraints();

            Connection theConnection = null;

            try
            {
                theConnection = GetConnection();

                //Read the constraint text for all table check constraints
                OdbcDataReader theReader = Queries.ExecuteSelectConstraintText(theConnection, TheTrafodionCatalog.ExternalName, 
                    TheTrafodionSchema.Version, this.UID, 'C');

                while(theReader.Read())
                {
                    string name = theReader.GetString(0).TrimEnd();
                    //Find the check constraint model and update its text.
                    TrafodionCheckConstraint constraint = FindCheckConstraint(name);
                    if (constraint != null)
                    {
                        constraint.Text = theReader.GetString(1).Trim();
                    }
                }
            }

            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
            _areCheckConstraintsLoaded = true;

        }

        /// <summary>
        /// Load the unique constraint attributes
        /// </summary>
        internal void LoadUniqueConstraints()
        {
            //If Unique constraints are already loaded, return
            if (_areUniqueConstraintsLoaded)
            {
                return;
            }

            //Load the check constraints, if it has not been loaded already
            LoadTableConstraints();
            foreach(TrafodionUniqueConstraint sqlMxUniqueConstraint in theUniqueConstraints)
            {
                sqlMxUniqueConstraint.Load();
            }

            _areUniqueConstraintsLoaded = true;

        }

        /// <summary>
        /// Get a list of triggers from the table
        /// </summary>
        public List<TrafodionTrigger> TrafodionTriggers
        {
            get
            {
                if (_sqlMxTableTriggers == null)
                {
                    _sqlMxTableTriggers = new TrafodionTableTriggersLoader().Load(this);
                }
                return _sqlMxTableTriggers;
            }
        }


        /// <summary>
        /// Object type of the table
        /// </summary>
        override public string SchemaObjectType
        {
            get
            {
                return ObjectType;
            }
        }

        /// <summary>
        /// Displayable Object type for the table
        /// </summary>
        override public string DisplayObjectType
        {
            get
            {
                return Properties.Resources.Table;
            }
        }        /// <summary>
        /// Object name space of the table
        /// </summary>
        override public string SchemaObjectNameSpace
        {
            get
            {
                return ObjectNameSpace;
            }
        }

        /// <summary>
        /// Get list of attribute column names specific to the schema version
        /// </summary>
        /// <param name="aTrafodionSchemaVersion">schema version</param>
        /// <returns></returns>
        static public String GetAttributesColumnNames(int aTrafodionSchemaVersion)
        {
            return (aTrafodionSchemaVersion == 1200) ? theV1200AttributesColumnNames : theV2000AttributesColumnNames;
        }

        /// <summary>
        /// Resets the table model
        /// </summary>
        override public void Refresh()
        {
            //Create a temp model
            TrafodionTable aTable = this.TheTrafodionSchema.LoadTableByName(this.InternalName);

            //If temp model is null, the object has been removed
            //So cleanup and notify the UI
            if (aTable == null)
            {
                this.TheTrafodionSchema.TrafodionTables.Remove(this);
                OnModelRemovedEvent();
                return;
            }
            if (this.CompareTo(aTable) != 0)
            {
                //If sql object has been recreated, attach the new sql model to the parent.
                this.TheTrafodionSchema.TrafodionTables.Remove(this);
                this.TheTrafodionSchema.TrafodionTables.Add(aTable);
                this.OnModelReplacedEvent(aTable);
            }
            else
            {
                base.Refresh();
                ClearColumns();
                theTrafodionPrimaryKey = null;
                _areAttributesLoaded = false;
                _isInsertLogStatusLoaded = false;
                _isMaintainStatusLoaded = false;
                _isLastUpdateStatsTimestampLoaded = false;
                _areConstraintsLoaded = false;
                _areCheckConstraintsLoaded = false;
                _areForeignKeysLoaded = false;
                _sqlMxSynonyms = null;
                _sqlMxViews = null;
                _sqlMxMaterializedViews = null;
                _sqlMxStoreOrderColumnDefs = null;
                _ddlText = null;
                _sqlMxTableTriggers = null;
                _TrafodionHashKeyColumnDefs = null;

                // The following lists are not initialized to null.
                TheTrafodionPrimaryKeyColumnDefs.Clear();
                theUniqueConstraints.Clear();
                theNotNullCheckConstraints.Clear();
                theCheckConstraints.Clear();
                theForeignKeyConstraints.Clear();
                ResetColumnPrivileges();
            }
        }

        public void ResetColumnPrivileges()
        {
            _columnsDelegate.ResetColumnPrivileges();
        }

        /// <summary>
        /// Finds a column definition, given a column name
        /// </summary>
        /// <param name="anInternalName">Name of the column</param>
        /// <returns></returns>
        public TrafodionTableColumn FindColumnByInternalName(string anInternalName)
        {
            TrafodionColumn sqlMxColumn = Columns.Find(delegate(TrafodionColumn aTrafodionColumn)
            {
                return aTrafodionColumn.InternalName.Equals(anInternalName);
            });

            if (sqlMxColumn == null)
            {
                //Clearing the columns will force us to fetch the columns list again
                ClearColumns();

                sqlMxColumn = Columns.Find(delegate(TrafodionColumn aTrafodionColumn)
                {
                    return aTrafodionColumn.InternalName.Equals(anInternalName);
                });

                // TODO: Deal with exception if column is null even after reloading the list
                throw new Exception(String.Format(Properties.Resources.TableColumnNotFound,
                    new object[] { anInternalName, "Table", ExternalName }));
            }

            // Cast as a table column as this is all that we hold.
            TrafodionTableColumn tableColumn = sqlMxColumn as TrafodionTableColumn;
#if DEBUG
            if (tableColumn == null)
            {
                // A debug build will actually throw an exception for an unexpected type.
                throw new ApplicationException(
                    "A non-table column named \"" + sqlMxColumn.ExternalName +
                    "\" was found in table \"" + this.ExternalName + "\"");
            }
#endif

            return tableColumn;
        }

        /// <summary>
        /// Finds a table check constraint
        /// </summary>
        /// <param name="aConstraintName">Name of the constraint to find</param>
        /// <returns>The model representing the check constraint</returns>
        public TrafodionCheckConstraint FindCheckConstraint(string aConstraintName)
        {
            TrafodionCheckConstraint theTrafodionCheckConstraint = theCheckConstraints.Find(delegate(TrafodionCheckConstraint aTrafodionCheckConstraint)
            {
                return aTrafodionCheckConstraint.InternalName.Equals(aConstraintName);
            });

            if (theTrafodionCheckConstraint == null)
            {
                //If schema cannot be resolved, read the database and reload all schemas for catalog 
                LoadTableConstraints();
                theTrafodionCheckConstraint = theCheckConstraints.Find(delegate(TrafodionCheckConstraint aTrafodionCheckConstraint)
                {
                    return aTrafodionCheckConstraint.InternalName.Equals(aConstraintName);
                });
            }

            return theTrafodionCheckConstraint;
        }

        /// <summary>
        /// Finds a column definition, given a column number
        /// </summary>
        /// <param name="aColumnNumber">The column number</param>
        /// <returns></returns>
        public TrafodionTableColumn FindColumnByColumnNumber(int aColumnNumber)
        {
            TrafodionColumn sqlMxColumn = Columns.Find(delegate(TrafodionColumn aTrafodionColumn)
            {
                return aTrafodionColumn.TheColumnNumber == aColumnNumber;
            });

            if (sqlMxColumn == null)
            {
                //Clearing the columns will force us to fetch the columns list again
                ClearColumns();

                sqlMxColumn = Columns.Find(delegate(TrafodionColumn aTrafodionColumn)
                {
                    return aTrafodionColumn.TheColumnNumber == aColumnNumber;
                });

                // TODO: Deal with exception if column is null even after reloading the list
                throw new Exception(String.Format(Properties.Resources.TableColumnNotFound,
                    new object[] { aColumnNumber, "Table", ExternalName }));
            }

            // Cast as a table column as this is all that we hold.
            TrafodionTableColumn tableColumn = sqlMxColumn as TrafodionTableColumn;
            return tableColumn;
        }
        /// <summary>
        /// List of synonyms defined on this table
        /// </summary>
        public List<TrafodionSynonym> TheTrafodionSynonyms
        {
            get
            {
                if (_sqlMxSynonyms == null)
                {
                    _sqlMxSynonyms = new TableSynonymsLoader().Load(this);
                }
                return _sqlMxSynonyms;
            }
        }

        /// <summary>
        /// List of materialized views using this table
        /// </summary>
        public List<TrafodionMaterializedView> TheTrafodionMaterializedViews
        {
            get 
            {
                if (_sqlMxMaterializedViews == null)
                {
                    _sqlMxMaterializedViews = new MVsUsedByTableLoader().Load(this);
                }
                return _sqlMxMaterializedViews;
            }
        }

        /// <summary>
        /// List of views using this table
        /// </summary>
        public List<TrafodionView> TheTrafodionViews
        {
            get
            {
                if (_sqlMxViews == null)
                {
                    _sqlMxViews = new ViewsUsingTableLoader().Load(this);
                }
                return _sqlMxViews;
            }
        }



        /// <summary>
        /// List of this table's column privileges.
        /// </summary>
        public List<ColumnPrivilege> ColumnPrivileges
        {
            get { return _columnsDelegate.ColumnPrivileges; }
        }

        /// <summary>
        /// The number of columns in this table.
        /// </summary>
        public int ColumnCount
        {
            get
            {
                return _columnsDelegate.ColumnCount;
            }
        }

        /// <summary>
        /// List of table columns
        /// </summary>
        public List<TrafodionColumn> Columns
        {
            get 
            {
                return _columnsDelegate.Columns;
            }
        }

        /// <summary>
        /// Clustering scheme of this table
        /// </summary>
        public string TheClusteringScheme
        {
            get { LoadAttributes(); return _clusteringScheme; }
        }

        /// <summary>
        /// Table block size
        /// </summary>
        public long TheBlockSize
        {
            get { LoadAttributes(); return _blockSize; }
        }

        /// <summary>
        /// Key length
        /// </summary>
        public int TheKeyLength
        {
            get { LoadAttributes(); return _keyLength; }
        }

        /// <summary>
        /// Lock length
        /// </summary>
        public int TheLockLength
        {
            get { LoadAttributes(); return _lockLength; }
        }

        /// <summary>
        /// Indicates if Audit information is compressed
        /// </summary>
        public bool IsAuditCompress
        {
            get { LoadAttributes(); return _isAuditCompress; }
        }

        /// <summary>
        /// Indicates if deleted records will be cleared
        /// </summary>
        public bool IsClearOnPurge
        {
            get { LoadAttributes(); return _isClearOnPurge; }
        }

        /// <summary>
        /// Indicates if only inserts into this table is logged
        /// </summary>
        public bool IsInsertLog
        {
            get { LoadInsertLogStatus(); return _isInsertLog; }
        }
        
        /// <summary>
        /// For future use
        /// </summary>
        public bool IsBuffered
        {
            get { LoadAttributes(); return _isBuffered; }
        }

        /// <summary>
        /// For future use
        /// </summary>
        public bool IsDataCompressed
        {
            get { LoadAttributes(); return _isDataCompressed; }
        }

        /// <summary>
        /// Indicates if index is compressed
        /// </summary>
        public bool IsIndexCompressed
        {
            get { LoadAttributes(); return _isIndexCompressed; }
        }

        /// <summary>
        /// Indicates if table is audited
        /// </summary>
        public bool IsAudited
        {
            get { LoadAttributes(); return _isAudited; }
        }

        /// <summary>
        /// Partitioning scheme of the table
        /// </summary>
        public string ThePartitioningScheme
        {
            get { LoadAttributes(); return _partitioningScheme; }
        }

        /// <summary>
        /// Table row format
        /// </summary>
        public string TheRowFormat
        {
            get { LoadAttributes(); return _rowFormat; }
        }

        /// <summary>
        /// Insert mode
        /// </summary>
        public string TheInsertMode
        {
            get { LoadAttributes(); return _insertMode; }
        }

        /// <summary>
        /// Row length or record size
        /// </summary>
        public int TheRecordSize
        {
            get { LoadAttributes(); return _recordSize; }
        }

        /// <summary>
        /// Maximum size specified during the table create. If size is 0 then it is system defined.
        /// </summary>
        public long TheMaxTableSize
        {
            get { LoadAttributes(); return _maxTableSize; }
        }
        /// <summary>
        /// The datetime when update stats was last run on this table
        /// </summary>
        public long TheLastUpdateStatsTimestamp
        {
            get { LoadLastUpdateStatsTimestamp(); return _lastUpdateStatsTimestamp; }
        }

        /// <summary>
        /// Indicates if REORG is currently allowed on this table
        /// </summary>
        public bool IsReorgEnabled
        {
            get { LoadMaintainStatus();  return _isReorgEnabled; }
        }

        /// <summary>
        /// Indicates if update statistics is currently allowed on this table
        /// </summary>
        public bool IsUpdateStatsEnabled
        {
            get { LoadMaintainStatus(); return _isUpdateStatsEnabled; }
        }

        /// <summary>
        /// Number of rows in the table
        /// </summary>
        public long RowCount
        {
            get 
            {
                LoadRowCount();
                return _rowCount; 
            }
        }

        public long TheTotalCurrentSize
        {
            get { return _totalCurrentSize; }
        }

        public double ThePercentAllocated
        {
            get { return _percentAllocated; }
        }

        /// <summary>
        /// Gets the store order value on the table, formatted to be displayed
        /// </summary>
        public string FormattedStoreOrder
        {
            get
            {
                LoadAttributes();
                if (_clusteringScheme.Equals("ES"))
                {
                    return "Entry";
                }
                else
                {
                    return "Specified Columns";
                }
            }
        }

        /// <summary>
        /// Indicates if this table is a metadata table
        /// </summary>
        override public bool IsMetadataObject
        {
            get
            {
                return !TheSecurityClass.Trim().Equals("UT");
            }
        }


        #region FORMATTED

        /// <summary>
        /// Indicates if Audit information is compressed
        /// </summary>
        public string FormattedIsAuditCompress
        {
            get { return Utilities.OnOff(IsAuditCompress); }
        }
        /// <summary>
        /// Indicates if deleted records will be cleared
        /// </summary>
        public string FormattedIsClearOnPurge
        {
            get {return Utilities.OnOff(IsClearOnPurge); }
        }
        /// <summary>
        /// Indicates if only inserts into this table is logged
        /// </summary>
        public string FormattedIsInsertLog
        {
            get { return Utilities.TrueFalse(IsInsertLog); }

        }
        /// <summary>
        /// Block size of this table
        /// </summary>
        public string FormattedBlockSize
        {
            get { return Utilities.FormatSize(TheBlockSize); }
        }
        /// <summary>
        /// Record size formatted for display
        /// </summary>
        public string FormattedRecordSize
        {
            get { return Utilities.FormatSize(TheRecordSize, Utilities.SizeType.Bytes); }
        }
        /// <summary>
        /// Max Table size formatted for display
        /// </summary>
        public string FormattedMaxTableSize
        {
            get
            {
                return TheMaxTableSize > 0 ?
                    Utilities.FormatSize(TheMaxTableSize, Utilities.SizeType.MB) : Properties.Resources.DeterminedBySystem;
            }
        }
        /// <summary>
        /// Last update stats timestamp formatted for display
        /// </summary>
        public JulianTimestamp FormattedLastUpdateStatsTimestamp
        {
            get { return new JulianTimestamp(TheLastUpdateStatsTimestamp); }
        }
        /// <summary>
        /// Reorg status formatted for display
        /// </summary>
        public string FormattedIsReorgEnabled
        {
            get { return Utilities.TrueFalse(IsReorgEnabled); }
        }
        /// <summary>
        /// Update stats status formatted for display
        /// </summary>
        public string FormattedIsUpdateStatsEnabled
        {
            get { return Utilities.TrueFalse(IsUpdateStatsEnabled); }
        }


        #endregion

        #region STORE_ORDER

        private List<TrafodionStoreOrderColumnDef> _sqlMxStoreOrderColumnDefs = null;
        /// <summary>
        /// List of store order column defs on this table
        /// </summary>
        public List<TrafodionStoreOrderColumnDef> TheTrafodionStoreOrderColumnDefs
        {
            get
            {
                if (_sqlMxStoreOrderColumnDefs == null)
                {
                    _sqlMxStoreOrderColumnDefs = new TrafodionStoreOrderColumnDefsLoader().Load(this);
                }
                return _sqlMxStoreOrderColumnDefs;
            }
        }

        #endregion

        #region UNIQUE_CONSTRAINTS

        private List<TrafodionUniqueConstraint> theUniqueConstraints = new List<TrafodionUniqueConstraint>();
        /// <summary>
        /// List of unique constraints on this table
        /// </summary>
        public List<TrafodionUniqueConstraint> TheUniqueConstraints
        {
            get 
            {
                LoadUniqueConstraints();
                return theUniqueConstraints; 
            }
        }

        #endregion

        #region FOREIGN_KEY_CONSTRAINTS

        private List<TrafodionForeignKey> theForeignKeyConstraints = new List<TrafodionForeignKey>();
        /// <summary>
        /// List of foreign key constraints on this table
        /// </summary>
        public List<TrafodionForeignKey> TheForeignKeyConstraints
        {
            get 
            {
                LoadForeignKeys();
                return theForeignKeyConstraints; 
            }
        }

        #endregion

        #region CHECK_CONSTRAINTS

        private List<TrafodionCheckConstraint> theCheckConstraints = new List<TrafodionCheckConstraint>();
        /// <summary>
        /// List of check constraints on this table
        /// </summary>
        public List<TrafodionCheckConstraint> TheCheckConstraints
        {
            get 
            {
                LoadCheckConstraints();
                return theCheckConstraints; 
            }            
        }

        #endregion

        #region NOT_NULL_CHECK_CONSTRAINTS

        private List<TrafodionTableConstraint> theNotNullCheckConstraints = new List<TrafodionTableConstraint>();
        /// <summary>
        /// List of not null constraints on this table
        /// </summary>
        public List<TrafodionTableConstraint> TheNotNullCheckConstraints
        {
            get 
            {
                LoadTableConstraints();
                return theNotNullCheckConstraints; 
            }
            set { theNotNullCheckConstraints = value; }
        }

        #endregion
        
        #region PRIMARY_KEY

        private TrafodionPrimaryKey theTrafodionPrimaryKey = null;
        /// <summary>
        /// The primary key of this table
        /// </summary>
        public TrafodionPrimaryKey TheTrafodionPrimaryKey
        {
            get { return theTrafodionPrimaryKey; }
            set { theTrafodionPrimaryKey = value; }
        }
        /// <summary>
        /// The primary key constraint uid
        /// </summary>
        public long ThePrimaryKeyConstraintUID
        {
            get { return TheTrafodionPrimaryKey.UID; }
            set { TheTrafodionPrimaryKey.UID = value; }
        }
        /// <summary>
        /// The primary key constraint's index uid
        /// </summary>
        public long ThePrimaryKeyIndexUID
        {
            get { return TheTrafodionPrimaryKey.IndexUID; }
            set { TheTrafodionPrimaryKey.IndexUID = value; }
        }
        /// <summary>
        /// Indicates if the primary key can be dropped or not
        /// </summary>
        public bool CanDropPrimaryKey
        {
            get { return TheTrafodionPrimaryKey.CanDropPrimaryKey; }
            set { TheTrafodionPrimaryKey.CanDropPrimaryKey = value; }
        }

        /// <summary>
        /// List of column definitions of the primary key
        /// </summary>
        public List<TrafodionPrimaryKeyColumnDef> TheTrafodionPrimaryKeyColumnDefs
        {
            get 
            { 
                return (TheTrafodionPrimaryKey != null? TheTrafodionPrimaryKey.TheTrafodionPrimaryKeyColumnDefs : new List<TrafodionPrimaryKeyColumnDef>()); 
            }
            set { TheTrafodionPrimaryKey.TheTrafodionPrimaryKeyColumnDefs = value; }
        }

        #endregion

        #region HASH_KEY

        /// <summary>
        /// List of column definitions of the hash key
        /// </summary>
        public List<TrafodionHashKeyColumnDef> TheTrafodionHashKeyColumnDefs
        {
            get
            {
                if (_TrafodionHashKeyColumnDefs == null)
                {
                    // If the partitioning schema is NOT start with "H", 
                    // then the partition key is not hash key
                    if (this.ThePartitioningScheme.StartsWith("H"))
                    {
                        _TrafodionHashKeyColumnDefs = new TrafodionHashKeyColumnDefsLoader().Load(this);
                    }
                    else
                    {
                        // NO hash key columns
                        _TrafodionHashKeyColumnDefs = new List<TrafodionHashKeyColumnDef>();
                    }
                }
                return _TrafodionHashKeyColumnDefs;
            }
        }

        #endregion

        class MVsUsedByTableLoader : TrafodionObjectsLoader<TrafodionTable, TrafodionMaterializedView>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionTable aTrafodionTable)
            {
                return Queries.ExecuteSelectMVsUsingObject(aConnection, aTrafodionTable.TheTrafodionCatalog.ExternalName, aTrafodionTable.TheTrafodionSchema.Version, aTrafodionTable.UID);
            }

            /// <summary>
            /// Retrieve the materialized views from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionTable"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionMaterializedView> aList, TrafodionTable aTrafodionTable, OdbcDataReader aReader)
            {
                string materializedViewName = aReader.GetString(0).TrimEnd();
                long schemaUID = aReader.GetInt64(1);

                //MVs in other schemas can use this table. 
                //Using the uid of the schema in which the MV is located, get the TrafodionSchema object
                TrafodionSchema sqlMxSchema = aTrafodionTable.TheTrafodionCatalog.FindSchema(schemaUID);

                //Find the instance of the TrafodionMaterializedView from this TrafodionSchema object and add it to the list
                TrafodionMaterializedView materializedView = sqlMxSchema.FindMaterializedView(materializedViewName);
                aList.Add(materializedView);

            }
        }

        class ViewsUsingTableLoader : TrafodionObjectsLoader<TrafodionTable, TrafodionView>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionTable aTrafodionTable)
            {
                return Queries.ExecuteSelectViewsUsingObject(aConnection, aTrafodionTable.TheTrafodionCatalog.ExternalName, aTrafodionTable.TheTrafodionSchema.Version, aTrafodionTable.UID);
            }

            /// <summary>
            /// Retrieve the materialized views from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionTable"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionView> aList, TrafodionTable aTrafodionTable, OdbcDataReader aReader)
            {
                string viewName = aReader.GetString(0).TrimEnd();
                long schemaUID = aReader.GetInt64(1);

                //MVs in other schemas can use this table. 
                //Using the uid of the schema in which the MV is located, get the TrafodionSchema object
                TrafodionSchema sqlMxSchema = aTrafodionTable.TheTrafodionCatalog.FindSchema(schemaUID);
                if (sqlMxSchema != null)
                {
                    //Find the instance of the TrafodionView from this TrafodionSchema object and add it to the list
                    TrafodionView aView = sqlMxSchema.FindView(viewName);
                    if (aView != null)
                    {
                        if (aView.TheTrafodionSchema.Version >= 2500)
                        {
                            aView.ViewType = aReader.GetString(2);
                        }
                        else
                        {
                            string textPreview = aReader.GetString(2);
                            aView.ViewType = textPreview.StartsWith("CREATE SYSTEM VIEW") ? "SV" : "UV";
                        }  
                        aList.Add(aView);
                    }
                }

            }
        }

        class TableSynonymsLoader : TrafodionObjectsLoader<TrafodionTable, TrafodionSynonym>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionTable aTrafodionTable)
            {
                return Queries.ExecuteSelectSynonymsOnObjects(aConnection, aTrafodionTable.TheTrafodionCatalog.ExternalName, aTrafodionTable.TheTrafodionSchema.Version, aTrafodionTable.UID);
            }

            /// <summary>
            /// Retrieve the synonyms from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionTable"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionSynonym> aList, TrafodionTable aTrafodionTable, OdbcDataReader aReader)
            {
                string synonymName = aReader.GetString(0).TrimEnd();
                long schemaUID = aReader.GetInt64(1);

                //Synonyms can be defined in other schemas to use this table. 
                //Using the uid of the schema in which the Synonym is located, get the TrafodionSchema object
                TrafodionSchema sqlMxSchema = aTrafodionTable.TheTrafodionCatalog.FindSchema(schemaUID);

                //Find the instance of the TrafodionSynonym from this TrafodionSchema object and add it to the list
                TrafodionSynonym synonym = sqlMxSchema.FindSynonym(synonymName);
                aList.Add(synonym);
            }
        }

        /// <summary>
        /// This class is used to load the store order information on a Table
        /// </summary>
        class TrafodionStoreOrderColumnDefsLoader : TrafodionObjectsLoader<TrafodionTable, TrafodionStoreOrderColumnDef>
        {
            /// <summary>
            /// Returns the ODBC Reader that contains the result of a query to get the list of index names
            /// </summary>
            /// <param name="aConnection">an ODBC Connection</param>
            /// <param name="aTrafodionTable">a TrafodionTable</param>
            /// <returns>returns an ODBCDataReader with data from the query</returns>
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionTable aTrafodionTable)
            {
                return Queries.ExecuteSelectStoreOrderColumnDefs(
                        aConnection, aTrafodionTable.TheTrafodionCatalog, aTrafodionTable.TheTrafodionSchema.Version, aTrafodionTable);
            }

            /// <summary>
            /// Overrides the load method to add store order column definitions to the list
            /// </summary>
            /// <param name="aList">the list of Store Order Column Definitions that is to be loaded with data</param>
            /// <param name="aTrafodionTable">the TrafodionTable that is supplying data</param>
            /// <param name="aReader">the Reader that contains the data to be loaded into the list</param>
            override protected void LoadOne(List<TrafodionStoreOrderColumnDef> aList, TrafodionTable aTrafodionTable, OdbcDataReader aReader)
            {
                aList.Add(new TrafodionStoreOrderColumnDef(aTrafodionTable,
                    aReader.GetString(0).TrimEnd(),
                    aReader.GetInt32(1),
                    aReader.GetString(3).Trim(), 
                    aReader.GetString(6).Trim()
                    ));
            }
        }


        /// <summary>
        /// This class is used to load the Triggers of a Table
        /// </summary>
        class TrafodionTableTriggersLoader : TrafodionObjectsLoader<TrafodionTable, TrafodionTrigger>
        {
            /// <summary>
            /// Returns the ODBC Reader that contains the result of a query to get the list of trigger names
            /// </summary>
            /// <param name="aConnection">an ODBC Connection</param>
            /// <param name="aTrafodionTable">a TrafodionTable</param>
            /// <returns>returns an ODBCDataReader with data from the query</returns>
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionTable aTrafodionTable)
            {
                return Queries.ExecuteSelectTriggerNames(
                        aConnection, aTrafodionTable.TheTrafodionCatalog, aTrafodionTable.TheTrafodionSchema.Version, aTrafodionTable.UID);
            } 

            /// <summary>
            /// Overrides the load method to add table triggers to the list
            /// </summary>
            /// <param name="aList">the list of TrafodionTriggers that is to be loaded with data</param>
            /// <param name="aTrafodionTable">the TrafodionTable that is supplying data</param>
            /// <param name="aReader">the Reader that contains the data to be loaded into the list</param>
            override protected void LoadOne(List<TrafodionTrigger> aList, TrafodionTable aTrafodionTable, OdbcDataReader aReader)
            {
                aList.Add(new TrafodionTrigger(aTrafodionTable, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4).Trim(), aReader.GetString(5).Trim(), aReader.GetString(6).Trim(), aReader.GetString(7).Trim(), aTrafodionTable.TheSecurityClass, aReader.GetString(8).Trim().Replace("\0", "")));
            }
            override protected OdbcDataReader GetReaderForSqlObject(Connection aConnection, TrafodionTable aTrafodionTable, string objectName)
            {
                return Queries.ExecuteSelectTriggerByName(
                        aConnection, aTrafodionTable.TheTrafodionCatalog, aTrafodionTable.TheTrafodionSchema.Version, aTrafodionTable.UID, objectName);
            }
            protected override TrafodionTrigger ReadSqlObject(TrafodionTable aTrafodionTable, OdbcDataReader aReader)
            {
                return new TrafodionTrigger(aTrafodionTable, aReader.GetString(0).TrimEnd(), aReader.GetInt64(1), aReader.GetInt64(2), aReader.GetInt64(3), aReader.GetString(4).Trim(), aReader.GetString(5).Trim(), aReader.GetString(6).Trim(), aReader.GetString(7).Trim(), aTrafodionTable.TheSecurityClass, aReader.GetString(8).Trim().Replace("\0", ""));
            }
        }

        /// <summary>
        /// This class is used to load the hash key column information on a Table
        /// </summary>
        class TrafodionHashKeyColumnDefsLoader : TrafodionObjectsLoader<TrafodionTable, TrafodionHashKeyColumnDef>
        {
            /// <summary>
            /// Returns the ODBC Reader that contains the result of a query to get the list of hash key columns
            /// </summary>
            /// <param name="aConnection">an ODBC Connection</param>
            /// <param name="aTrafodionTable">a TrafodionTable</param>
            /// <returns>returns an ODBCDataReader with data from the query</returns>
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionTable aTrafodionTable)
            {
                return Queries.ExecuteSelectHashKeyColumnInfo(
                        aConnection, aTrafodionTable.TheTrafodionCatalog, aTrafodionTable.TheTrafodionSchema.Version, aTrafodionTable);
            }

            /// <summary>
            /// Overrides the load method to add hash key column definitions to the list
            /// </summary>
            /// <param name="aList">the list of Hash Key Column Definitions that is to be loaded with data</param>
            /// <param name="aTrafodionTable">the TrafodionTable that is supplying data</param>
            /// <param name="aReader">the Reader that contains the data to be loaded into the list</param>
            override protected void LoadOne(List<TrafodionHashKeyColumnDef> aList, TrafodionTable aTrafodionTable, OdbcDataReader aReader)
            {
                aList.Add(new TrafodionHashKeyColumnDef(aTrafodionTable,
                    aReader.GetString(0).TrimEnd(),  // the column external name
                    aReader.GetInt32(1),             // the column position in base table
                    aReader.GetInt32(2),             // the column position in ordinal row within access path
                    aReader.GetInt32(3),             // the column hash key sequence order
                    aReader.GetString(4).Trim()     // the ordering
                    ));
            }
        }

        #region IHasTrafodionColumns Members


        public bool DoesUserHaveColumnPrivilege(string userName, string columnName, string privilegeType)
        {
            return _columnsDelegate.DoesUserHaveColumnPrivilege(userName, columnName, privilegeType);
        }

        #endregion

    }
}
