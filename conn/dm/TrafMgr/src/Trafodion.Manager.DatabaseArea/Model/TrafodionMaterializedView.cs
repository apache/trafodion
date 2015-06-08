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
    /// A model of a Materialized View.
    /// </summary>
    public class TrafodionMaterializedView : IndexedSchemaObject, IHasTrafodionColumns
    {
        #region Static Fields

        /// <summary>
        /// A materialized view's name space.
        /// </summary>
        public const string ObjectNameSpace = "TA";

        /// <summary>
        /// A materialized view's object type.
        /// </summary>
        public const string ObjectType = "MV";

        #endregion

        #region Member Variables

        private bool _areAttributesLoaded = false;

        // Attribute Variables
        private string _isAuditCompress;
        private string _isClearOnPurge;
        private long _blockSize;
        private long _refreshedAtTimestamp;
        private string _commitRefreshEach;
        private string _creationRefreshType;
        private string _initializationType;
        private string _mvUID;
        private long _rowCount;
        private bool _isRowCountFetched = false;

        private List<TrafodionSynonym> _sqlMxSynonyms = null;
        private List<TrafodionMaterializedView> _sqlMxMaterializedViews = null;
        private List<TrafodionMaterializedView> _usedTrafodionMaterializedViews = null;
        private List<TrafodionTable> _usedSqlTables = null;
        private List<TrafodionRoutine> _usedRoutine = null;
        private List<TrafodionView> _sqlMxViews = null;
        private DefaultHasTrafodionColumns<TrafodionMaterializedViewColumn> _columnsDelegate = null;
        private List<TrafodionObject> _columnSourceObjects = null;

        #endregion

        #region Properties

        /// <summary>
        /// The object type of the materialized view.
        /// </summary>
        override public string SchemaObjectType
        {
            get
            {
                return ObjectType;
            }
        }

        /// <summary>
        /// Displayable Object type for the MV
        /// </summary>
        override public string DisplayObjectType
        {
            get
            {
                return Properties.Resources.MaterializedView;
            }
        }

        /// <summary>
        /// The materialized view's name space.
        /// </summary>
        override public string SchemaObjectNameSpace
        {
            get
            {
                return ObjectNameSpace;
            }
        }

        /// <summary>
        /// The number of columns in this materialized view.
        /// </summary>
        public int ColumnCount
        {
            get { return _columnsDelegate.ColumnCount; }
        }

        /// <summary>
        /// The columns contained within this materialized view.
        /// </summary>
        public List<TrafodionColumn> Columns
        {
            get { return _columnsDelegate.Columns; }
        }

        /// <summary>
        /// The privileges that pertain to this Materialized View's columns.
        /// </summary>
        public List<ColumnPrivilege> ColumnPrivileges
        {
            get { return _columnsDelegate.ColumnPrivileges; }
        }

       
        /// Initialization Type is done seperately
            string initText;

        /// <summary>  
        /// Indicates whether the MV is audit compressed
        /// </summary>
        public string FormattedIsAuditCompress
        {
            get
            {
                LoadAttributes();
                return Utilities.OnOff(_isAuditCompress);
            }
        }

   
        /// <summary>
        /// Indates whether the MV is cleared on a purge
        /// </summary>
        public string FormattedIsClearOnPurge
        {
            get { LoadAttributes(); return Utilities.OnOff(_isClearOnPurge); }
        }

        /// <summary>
        /// Returns the block size of the MV
        /// </summary>
        public string FormattedBlockSize
        {
            get { LoadAttributes(); return Utilities.FormatSize(_blockSize); }
        }

        /// <summary>
        /// Returns the Refreshed At Timestamp of the MV
        /// </summary>
        public JulianTimestamp FormattedRefreshedAtTimestamp
        {
            get { LoadAttributes(); return new JulianTimestamp(_refreshedAtTimestamp); }
        }

        /// <summary>
        /// Returns the # of Refresh Commit Each rows of the MV
        /// </summary>
        public string FormattedCommitRefreshEach
        {
            get { LoadAttributes(); return _commitRefreshEach + " " + Properties.Resources.CommitRefreshEachRows; }
        }

        /// <summary>
        /// Returns the Creation Refresh Type of the MV
        /// </summary>
        public string FormattedCreationRefreshType
        {
            get
            {
                string refreshType = refreshType = Properties.Resources.Unknown;
                LoadAttributes();

                if (_creationRefreshType == "R") refreshType = Properties.Resources.OnRequest;
                if (_creationRefreshType == "C") refreshType = Properties.Resources.Recompute;
                if (_creationRefreshType == "S") refreshType = Properties.Resources.OnStatement;

                return refreshType;
            }
        }

        /// <summary>
        /// Returns the Initialization Type of the MV
        /// </summary>
        public string initializationType
        {
            get
            {
                LoadAttributes();

                return _initializationType;
            }
        }

        /// <summary>
        /// List of synonyms defined on this MV
        /// </summary>
        public List<TrafodionSynonym> TheTrafodionSynonyms
        {
            get
            {
                if (_sqlMxSynonyms == null)
                {
                    _sqlMxSynonyms = new MaterializedViewSynonymsLoader().Load(this);
                }
                return _sqlMxSynonyms;
            }
        }

        /// <summary>
        /// List of materialized views using this MV
        /// </summary>
        public List<TrafodionMaterializedView> TheTrafodionMaterializedViews
        {
            get
            {
                if (_sqlMxMaterializedViews == null)
                {
                    _sqlMxMaterializedViews = new MVsUsedByMVLoader().Load(this);
                }
                return _sqlMxMaterializedViews;
            }
        }

        /// <summary>
        /// List of materialized views used by this MV
        /// </summary>
        public List<TrafodionMaterializedView> TheUsedMVs
        {
            get
            {
                if (_usedTrafodionMaterializedViews == null)
                {
                    _usedTrafodionMaterializedViews = new MVsUsesMVLoader().Load(this);
                }
                return _usedTrafodionMaterializedViews;
            }
        }

        /// <summary>
        /// List of Tables used by this MV
        /// </summary>
        public List<TrafodionTable> TheUsedTables
        {
            get
            {
                if (_usedSqlTables == null)
                {
                    _usedSqlTables = new MVsUsesTableLoader().Load(this);
                }
                return _usedSqlTables;
            }
        }

        /// <summary>
        /// List of Routines used by this MV
        /// </summary>
        public List<TrafodionRoutine> TheUsedTrafodionRoutines
        {
            get
            {
                if (_usedRoutine == null)
                {
                    _usedRoutine = new MVsUsesRoutineLoader().Load(this);
                }
                return _usedRoutine;
            }
        }

        /// <summary>
        /// List of views using this MV
        /// </summary>
        public List<TrafodionView> TheTrafodionViews
        {
            get
            {
                if (_sqlMxViews == null)
                {
                    _sqlMxViews = new ViewsUsingMVLoader().Load(this);
                }
                return _sqlMxViews;
            }
        }

        /// <summary>
        /// Number of rows in the MV
        /// </summary>
        public long RowCount
        {
            get
            {
                LoadRowCount();
                return _rowCount;
            }
        }


        #endregion

        /// <summary>
        /// Creates a new model of a materialized view.
        /// </summary>
        /// <param name="aTrafodionSchema">The schema in which this resides.</param>
        /// <param name="anInternalName">The internal name of this object.</param>
        /// <param name="aUID">This objet's unique identifier.</param>
        /// <param name="aCreateTime">This object's creation time.</param>
        /// <param name="aRedefTime">The last time this object was redefined.</param>
        /// <param name="aSecurityClass">This object's security class.</param>
        /// <param name="anOwner">The owner of the object.</param>
        public TrafodionMaterializedView(TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
            : base(aTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
        {
            _columnsDelegate = new DefaultHasTrafodionColumns<TrafodionMaterializedViewColumn>(this);
        }

        /// <summary>
        /// Refreshes the model
        /// </summary>
        public override void Refresh()
        {
            //Create a temp model
            TrafodionMaterializedView aMaterializedView = this.TheTrafodionSchema.LoadMVByName(this.InternalName);

            //If temp model is null, the object has been removed
            //So cleanup and notify the UI
            if (aMaterializedView == null)
            {
                this.TheTrafodionSchema.TrafodionMaterializedViews.Remove(this);
                OnModelRemovedEvent();
                return;
            }
            if (this.CompareTo(aMaterializedView) != 0)
            {
                //If sql object has been recreated, attach the new sql model to the parent.
                this.TheTrafodionSchema.TrafodionMaterializedViews.Remove(this);
                this.TheTrafodionSchema.TrafodionMaterializedViews.Add(aMaterializedView);
                //Notify the UI
                this.OnModelReplacedEvent(aMaterializedView);
            }
            else
            {
                base.Refresh();
                _areAttributesLoaded = false;
                _isAuditCompress = "";
                _isClearOnPurge = "";
                _blockSize = 0;
                _refreshedAtTimestamp = 0;
                _commitRefreshEach = "";
                _creationRefreshType = "";
                _initializationType = "";
                _sqlMxSynonyms = null;
                _sqlMxMaterializedViews = null;
                _usedTrafodionMaterializedViews = null;
                _usedSqlTables = null;
                _sqlMxViews = null;
                _columnsDelegate.ClearColumns();
                _rowCount = 0;
                ResetColumnPrivileges();
            }

        }

        public void ResetColumnPrivileges()
        {
            _columnsDelegate.ResetColumnPrivileges();
        }
        
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

                if (theReader.Read())
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
        /// Load the attributes of the MV
        /// </summary>
        /// <returns></returns>
        public bool LoadAttributes()
        {
            if (_areAttributesLoaded)
            {
                return true;
            }

            Connection theConnection = null;

            try
            {
                theConnection = GetConnection();

                OdbcDataReader theReader = Queries.ExecuteSelectMVAttributes(theConnection, TheTrafodionCatalog, this, TheTrafodionSchema.Version);
                theReader.Read();
                SetAttributes(theReader);
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }

            // Get MV Initializaton Type seperately from Text metadata table
            _initializationType = Properties.Resources.None;
            try
            {
                theConnection = GetConnection();

                OdbcDataReader theReader = Queries.ExecuteSelectMVDDLText(theConnection, TheTrafodionCatalog.ExternalName, TheTrafodionSchema.Version, this.UID);
                while (theReader.Read())
                {
                    string MVText = theReader.GetString(0).Trim();

                    // Look for specific DDL string
                    if (MVText.IndexOf("INITIALIZE ON REFRESH") != -1)
                    {
                        _initializationType = Properties.Resources.OnRefresh;
                        break;
                    }
                    if (MVText.IndexOf("INITIALIZE ON CREATE") != -1)
                    {
                        _initializationType = Properties.Resources.OnCreate;
                        break;
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
            _areAttributesLoaded = true;
            return true;
        }

        /// <summary>
        /// Store the data you get back from the query in LoadAttributes
        /// </summary>
        /// <param name="aReader"></param>
        public void SetAttributes(OdbcDataReader aReader)
        {
            _isAuditCompress = aReader.GetString(0).Trim();
            _isClearOnPurge = aReader.GetString(1).Trim();
            _blockSize = aReader.GetInt64(2);
            _refreshedAtTimestamp = aReader.GetInt64(3);
            _commitRefreshEach = aReader.GetString(4).Trim(); ;
            _creationRefreshType = aReader.GetString(5).Trim();
            _mvUID = aReader.GetString(6).Trim();
       
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

                long MVRowCount = 0;
                while (odbcDataReader.Read())
                {
                    int columnNumber = odbcDataReader.GetInt32(0);
                    TrafodionMaterializedViewColumn sqlMxMVColumn = FindColumnByColumnNumber(columnNumber);
                    if (sqlMxMVColumn != null)
                    {
                        // The histogram stats is stored in the table column model
                        MVColumnHistogramStats columnStatistics = new MVColumnHistogramStats(sqlMxMVColumn);
                        columnStatistics.SetAttributes(odbcDataReader);
                        MVRowCount = (MVRowCount > columnStatistics.RowCount ? MVRowCount : columnStatistics.RowCount);
                        sqlMxMVColumn.HistogramStatistics = columnStatistics;
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
        /// Finds a column definition, given a column number
        /// </summary>
        /// <param name="aColumnNumber">The column number</param>
        /// <returns></returns>
        public TrafodionMaterializedViewColumn FindColumnByColumnNumber(int aColumnNumber)
        {
            TrafodionColumn sqlMxColumn = Columns.Find(delegate(TrafodionColumn aTrafodionColumn)
            {
                return aTrafodionColumn.TheColumnNumber == aColumnNumber;
            });

            if (sqlMxColumn == null)
            {
                //Clearing the columns will force us to fetch the columns list again
                _columnsDelegate.ClearColumns();

                sqlMxColumn = Columns.Find(delegate(TrafodionColumn aTrafodionColumn)
                {
                    return aTrafodionColumn.TheColumnNumber == aColumnNumber;
                });

                // TODO: Deal with exception if column is null even after reloading the list
                throw new Exception(String.Format(Properties.Resources.TableColumnNotFound,
                    new object[] { aColumnNumber, "Materialized View", ExternalName }));
            }

            // Cast as a table column as this is all that we hold.
            TrafodionMaterializedViewColumn mvColumn = sqlMxColumn as TrafodionMaterializedViewColumn;
            return mvColumn;
        }

        public TrafodionObject GetUsedTable(int columnNum)
        {

            if (_columnSourceObjects == null)
            {
                _columnSourceObjects = new TrafodionMVColumnsLoader().Load(this);
            }

            // Object were fetched in column order
            if (_columnSourceObjects.Count > 0 && _columnSourceObjects.Count > columnNum)
                return _columnSourceObjects[columnNum];

            return null;
        }

        #region Loaders
        
        /// <summary>
        /// The object loader for used objects.
        /// </summary>
        class TrafodionMVColumnsLoader : TrafodionObjectsLoader<TrafodionMaterializedView, TrafodionObject>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionMaterializedView aTrafodionMaterializedView)
            {
                return Queries.ExecuteSelectMVColUsage(aConnection, aTrafodionMaterializedView);
            }

            /// <summary>
            /// Retrieve the used objects from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionView"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionObject> aList, TrafodionMaterializedView aTrafodionMaterializedView, OdbcDataReader aReader)
            {
                TrafodionObject aObject = null;

                string objectInternalName = aReader.GetString(0).TrimEnd();
                long schemaUID = aReader.GetInt64(1);

                // Get the schema of the used table
                TrafodionSchema schema = aTrafodionMaterializedView.TheTrafodionCatalog.FindSchema(schemaUID);
                if (schema == null)
                {
                    return;
                }

                // Find the instance of the used object from this TrafodionSchema object and add it to the list
                // could be a Table, View or MV
                aObject = schema.FindSchemaObjectByName(objectInternalName, schema.TrafodionTables);

                if (aObject == null)
                {
                    aObject = schema.FindSchemaObjectByName(objectInternalName, schema.TrafodionViews);
                    if (aObject == null)
                        aObject = schema.FindSchemaObjectByName(objectInternalName, schema.TrafodionMaterializedViews);
                }

                if (aObject != null)
                {
                    aList.Add(aObject);
                }
            }
        }

        class ViewsUsingMVLoader : TrafodionObjectsLoader<TrafodionMaterializedView, TrafodionView>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionMaterializedView aTrafodionMV)
            {
                return Queries.ExecuteSelectViewsUsingObject(aConnection, aTrafodionMV.TheTrafodionCatalog.ExternalName, aTrafodionMV.TheTrafodionSchema.Version, aTrafodionMV.UID);
            }

            /// <summary>
            /// Retrieve the materialized views from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionMV"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionView> aList, TrafodionMaterializedView aTrafodionMV, OdbcDataReader aReader)
            {
                string viewName = aReader.GetString(0).TrimEnd();
                long schemaUID = aReader.GetInt64(1);

                //MVs in other schemas can use this table. 
                //Using the uid of the schema in which the MV is located, get the TrafodionSchema object
                TrafodionSchema sqlMxSchema = aTrafodionMV.TheTrafodionCatalog.FindSchema(schemaUID);
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

        class MaterializedViewSynonymsLoader : TrafodionObjectsLoader<TrafodionMaterializedView, TrafodionSynonym>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionMaterializedView aTrafodionMaterializedView)
            {
                return Queries.ExecuteSelectSynonymsOnObjects(aConnection, aTrafodionMaterializedView.TheTrafodionCatalog.ExternalName, aTrafodionMaterializedView.TheTrafodionSchema.Version, aTrafodionMaterializedView.UID);
            }

            /// <summary>
            /// Retrieve the synonyms from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionMaterializedView"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionSynonym> aList, TrafodionMaterializedView aTrafodionMaterializedView, OdbcDataReader aReader)
            {
                string synonymName = aReader.GetString(0).TrimEnd();
                long schemaUID = aReader.GetInt64(1);

                //Synonyms can be defined in other schemas to use this table. 
                //Using the uid of the schema in which the Synonym is located, get the TrafodionSchema object
                TrafodionSchema sqlMxSchema = aTrafodionMaterializedView.TheTrafodionCatalog.FindSchema(schemaUID);

                //Find the instance of the TrafodionSynonym from this TrafodionSchema object and add it to the list
                TrafodionSynonym synonym = sqlMxSchema.FindSynonym(synonymName);
                aList.Add(synonym);
            }
        }

        class MVsUsedByMVLoader : TrafodionObjectsLoader<TrafodionMaterializedView, TrafodionMaterializedView>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionMaterializedView aTrafodionMaterializedView)
            {
                return Queries.ExecuteSelectMVsUsingObject(aConnection, aTrafodionMaterializedView.TheTrafodionCatalog.ExternalName, aTrafodionMaterializedView.TheTrafodionSchema.Version, aTrafodionMaterializedView.UID);
            }

            /// <summary>
            /// Retrieve the materialized views from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionMaterializedView"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionMaterializedView> aList, TrafodionMaterializedView aTrafodionMaterializedView, OdbcDataReader aReader)
            {
                string materializedViewName = aReader.GetString(0).TrimEnd();
                long schemaUID = aReader.GetInt64(1);

                //MVs in other schemas can use this MV. 
                //Using the uid of the schema in which the MV is located, get the TrafodionSchema object
                TrafodionSchema sqlMxSchema = aTrafodionMaterializedView.TheTrafodionCatalog.FindSchema(schemaUID);

                //Find the instance of the TrafodionMaterializedView from this TrafodionSchema object and add it to the list
                TrafodionMaterializedView materializedView = sqlMxSchema.FindMaterializedView(materializedViewName);
                aList.Add(materializedView);

            }
        }

        class MVsUsesMVLoader : TrafodionObjectsLoader<TrafodionMaterializedView, TrafodionMaterializedView>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionMaterializedView aTrafodionMaterializedView)
            {
                return Queries.ExecuteSelectMVsUsedByMe(aConnection, aTrafodionMaterializedView.TheTrafodionCatalog.ExternalName, aTrafodionMaterializedView.TheTrafodionSchema.Version, aTrafodionMaterializedView.UID);
            }

            /// <summary>
            /// Retrieve the materialized views from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionMaterializedView"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionMaterializedView> aList, TrafodionMaterializedView aTrafodionMaterializedView, OdbcDataReader aReader)
            {
                string materializedViewName = aReader.GetString(0).TrimEnd();
                long schemaUID = aReader.GetInt64(1);

                //MVs in other schemas can use this MV. 
                //Using the uid of the schema in which the MV is located, get the TrafodionSchema object
                TrafodionSchema sqlMxSchema = aTrafodionMaterializedView.TheTrafodionCatalog.FindSchema(schemaUID);

                //Find the instance of the TrafodionMaterializedView from this TrafodionSchema object and add it to the list
                TrafodionMaterializedView materializedView = sqlMxSchema.FindMaterializedView(materializedViewName);
                aList.Add(materializedView);

            }
        }
      
        class MVsUsesTableLoader : TrafodionObjectsLoader<TrafodionMaterializedView, TrafodionTable>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionMaterializedView aTrafodionMaterializedView)
            {
                return Queries.ExecuteSelectTablesUsedByMe(aConnection, aTrafodionMaterializedView.TheTrafodionCatalog.ExternalName, aTrafodionMaterializedView.TheTrafodionSchema.Version, aTrafodionMaterializedView.UID);
            }

            /// <summary>
            /// Retrieve the materialized views from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionMaterializedView"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionTable> aList, TrafodionMaterializedView aTrafodionMaterializedView, OdbcDataReader aReader)
            {
                string tableName = aReader.GetString(0).TrimEnd();
                long schemaUID = aReader.GetInt64(1);

                //MVs in other schemas can use this MV. 
                //Using the uid of the schema in which the MV is located, get the TrafodionSchema object
                TrafodionSchema sqlMxSchema = aTrafodionMaterializedView.TheTrafodionCatalog.FindSchema(schemaUID);

                //Find the instance of the table from this TrafodionSchema object and add it to the list
                TrafodionTable table = sqlMxSchema.FindTable(tableName);
                aList.Add(table);

            }
        }
        class MVsUsesRoutineLoader : TrafodionObjectsLoader<TrafodionMaterializedView, TrafodionRoutine>
        {
            override protected OdbcDataReader GetQueryReader(Connection aConnection, TrafodionMaterializedView aTrafodionMaterializedView)
            {
                return Queries.ExecuteSelectRoutinesUsedByMV(aConnection, aTrafodionMaterializedView);
            }

            /// <summary>
            /// Retrieve the materialized views from their respective schemas.
            /// </summary>
            /// <param name="aList"></param>
            /// <param name="aTrafodionMaterializedView"></param>
            /// <param name="aReader"></param>
            override protected void LoadOne(List<TrafodionRoutine> aList, TrafodionMaterializedView aTrafodionMaterializedView, OdbcDataReader aReader)
            {
                string routineName = aReader.GetString(0).Trim();
                long schemaUID = aReader.GetInt64(1);
                string udrType = aReader.GetString(2).Trim();

                TrafodionSchema schema = aTrafodionMaterializedView.TheTrafodionCatalog.FindSchema(schemaUID);
                if (schema == null)
                {
                    return;
                }
                TrafodionRoutine sqlMxRoutine = null;
                if ("F".Equals(udrType))
                {
                    sqlMxRoutine = schema.FindFunction(routineName);
                }
                else if ("AC".Equals(udrType))
                {
                    sqlMxRoutine = schema.FindFunctionAction(routineName);
                }
                else if ("T".Equals(udrType))
                {
                    sqlMxRoutine = schema.FindTableMappingFunction(routineName);
                }

                if (sqlMxRoutine != null)
                {
                    aList.Add(sqlMxRoutine);
                }
            }
        }
        #endregion

        #region IHasTrafodionColumns Members


        public bool DoesUserHaveColumnPrivilege(string userName, string columnName, string privilegeType)
        {
            return _columnsDelegate.DoesUserHaveColumnPrivilege(userName, columnName, privilegeType);
        }

        #endregion
    }
}
