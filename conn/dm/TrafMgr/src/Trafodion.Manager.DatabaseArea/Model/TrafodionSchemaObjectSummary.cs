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
using System.Data.Odbc;
using System.Data;
using System.Collections.Generic;
using System.Text;
using Trafodion.Manager.Framework;
using Trafodion.Manager.Framework.Connections;


namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// This class will encapsulate all of the DB calls and return 
    /// the moldel for the partition summary information. 
    /// Note: This model does not try to re-populate the existing 
    /// models.
    /// </summary>
    class TrafodionSchemaObjectsSummary :  IHasTrafodionCatalog
    {
        #region Member Variables
        TrafodionSystem _theTrafodionSystem;
        TrafodionCatalog _theTrafodionCatalog;
        TrafodionSchema _theTrafodionSchema;
        OdbcCommand theQuery;
        #endregion

        #region Properties
        public TrafodionSystem TheTrafodionSystem
        {
            get { return _theTrafodionSystem; }
            set { _theTrafodionSystem = value; }
        }

        public TrafodionCatalog TheTrafodionCatalog
        {
            get { return _theTrafodionCatalog; }
            set { _theTrafodionCatalog = value; }
        }

        public TrafodionSchema TheTrafodionSchema
        {
            get { return _theTrafodionSchema; }
            set { _theTrafodionSchema = value; }
        }

        public OdbcCommand TheQuery
        {
            get { return theQuery; }
            set { theQuery = value; }
        }
        #endregion

        #region Constructor
        public TrafodionSchemaObjectsSummary(TrafodionSystem aTrafodionSystem, 
            TrafodionCatalog aTrafodionCatalog,
            TrafodionSchema aTrafodionSchema)
        {
            _theTrafodionSystem = aTrafodionSystem;
            _theTrafodionCatalog = aTrafodionCatalog;
            _theTrafodionSchema = aTrafodionSchema;
        }
        #endregion

        #region Methods
        /// <summary>
        /// Returns the summary information for tables in a schema. The results are an
        /// aggregate of all the partitions of the table.
        /// The columns returned by the Datatable are as follows
        /// 1. Name
        /// 2. Last updated time
        /// 3. Update stats enabled status
        /// 4. Reorg operations enabled status
        /// 5. Max size
        /// 6. Current EOF
        /// 7. Percent full
        /// </summary>
        /// <returns></returns>
        public DataTable getTablesSummary()
        {
            Connection theConnection = null;
            OdbcDataReader theReader = null;
            DataTable dataTable = new DataTable();
            DataRow aRow = null;
            long lastUpdate;
            String objectName;
            long uid ;
            long previousUid = -1;
            String objectSecurityClass;
            long rowCount;
            decimal maxSize;
            decimal currentEof;
            String maintainOperation;
            String maintainStatus;

            try
            {
                bool isTrafodionCatalog = TheTrafodionCatalog.ExternalName.Equals("TRAFODION");
                theConnection = GetConnection();
                TheQuery = Queries.ExecuteSelectTableDetailsSummary(theConnection, TheTrafodionSchema);
                theReader = Queries.ExecuteReader(TheQuery, false);

                // The modification is to remove the use of MaintainInfoView, which is still considered as
                // internal and may not be available on all customer's system. Leave the code here is to
                // allow easy restore once this view is made available for customer use.
                // 1/27/2009
                dataTable.Columns.Add(Properties.Resources.Name);
                dataTable.Columns.Add(Properties.Resources.StatisticsLastUpdated, typeof(JulianTimestamp));
                //dataTable.Columns.Add(Properties.Resources.ReorganizeEnabled);
                //dataTable.Columns.Add(Properties.Resources.UpdateStatisticsEnabled);
                dataTable.Columns.Add(Properties.Resources.TotalCurrentRowCount, typeof(long));
                dataTable.Columns.Add(Properties.Resources.TotalMaximumSize, typeof(DecimalSizeObject));
                dataTable.Columns.Add(Properties.Resources.TotalCurrentSize, typeof(DecimalSizeObject));
                dataTable.Columns.Add(Properties.Resources.PercentAllocated, typeof(PercentObject));

                //We can get at most 2 rows for each table for the 2 maintain operations that we are
                //tracking (update stats and Reorg). The following code has the logic to merge them 
                //into one row.
                while (theReader.Read())
                {
                    uid = (long)theReader[2];
                    lastUpdate = getLongFromReader(theReader, 0);
                    objectName = getStringFromReader(theReader, 1);
                    objectName = (objectName != null) ? TrafodionName.ExternalForm(objectName) : "";
                    objectSecurityClass = getStringFromReader(theReader, 3);
                    rowCount = getLongFromReader(theReader, 4);
                    maxSize = getDecimalFromReader(theReader, 5);
                    currentEof = getDecimalFromReader(theReader, 6);
                    //maintainOperation = getStringFromReader(theReader, 7);
                    //maintainStatus = getStringFromReader(theReader, 8);

                    if (previousUid != uid)
                    {
                        aRow = dataTable.NewRow();

                        aRow[0] = objectName;
                        aRow[1] = new JulianTimestamp(lastUpdate);
                        aRow[2] = rowCount;
                        aRow[3] = new DecimalSizeObject(maxSize);
                        aRow[4] = new DecimalSizeObject(currentEof);
                        aRow[5] = new PercentObject((double)currentEof * 100 / (double)maxSize);
                        if (cannotDisplayMetadataTable(objectSecurityClass))
                        {
                            continue;
                        }
                        dataTable.Rows.Add(aRow);
                        previousUid = uid;
                    }
                    else
                    {
                        //aRow[2] = bool.Parse((string)aRow[2]) && isMaintainStatus(maintainOperation, maintainStatus, TrafodionTable.MaintainUpdateStatsOperationName);
                        //aRow[3] = bool.Parse((string)aRow[3]) && isMaintainStatus(maintainOperation, maintainStatus, TrafodionTable.MaintainReorgOperationName);
                    }
                }

            }
            catch (OdbcException oe)
            {
                throw new Exception(oe.Message);
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
            dataTable.AcceptChanges();
            return dataTable;
        }

        /// <summary>
        /// Returns the summary information for indexes in a schema. The results are an
        /// aggregate of all the partitions of the indexes.
        /// The columns returned by the Datatable are as follows
        /// 1. Name
        /// 2. Max size
        /// 3. Current EOF
        /// 4. Percent full
        /// </summary>
        /// <returns></returns>
        public DataTable getIndexesSummary()
        {
            Connection theConnection = null;
            OdbcDataReader theReader = null;
            DataTable dataTable = new DataTable();
            DataRow aRow = null;
            String objectName;
            long uid;
            String objectSecurityClass;
            long rowCount;
            decimal maxSize;
            decimal currentEof;

            dataTable.Columns.Add(Properties.Resources.Name);
            dataTable.Columns.Add(Properties.Resources.TotalCurrentRowCount, typeof(long));
            dataTable.Columns.Add(Properties.Resources.TotalMaximumSize, typeof(DecimalSizeObject));
            dataTable.Columns.Add(Properties.Resources.TotalCurrentSize, typeof(DecimalSizeObject));
            dataTable.Columns.Add(Properties.Resources.PercentAllocated, typeof(PercentObject));

            try
            {
                theConnection = GetConnection();
                TheQuery = Queries.ExecuteSelectIndexDetailsSummary(theConnection, TheTrafodionSchema);
                theReader = Queries.ExecuteReader(TheQuery, false);

                while (theReader.Read())
                {
                    aRow = dataTable.NewRow();

                    objectName = (theReader[0] == null) ? "" : ((String)theReader[0]).TrimEnd();
                    rowCount = (long)theReader[1];
                    maxSize = getDecimalFromReader(theReader, 2);
                    currentEof = getDecimalFromReader(theReader, 3);

                    aRow[0] = objectName;
                    aRow[1] = rowCount;
                    aRow[2] = new DecimalSizeObject(maxSize);
                    aRow[3] = new DecimalSizeObject(currentEof);
                    aRow[4] = new PercentObject((double)currentEof * 100 / (double)maxSize);
                    dataTable.Rows.Add(aRow);
                }
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
            dataTable.AcceptChanges();
            return dataTable;
        }

        /// <summary>
        /// Returns the summary information for indexes of a table. The results are an
        /// aggregate of all the partitions of the indexes.
        /// The columns returned by the Datatable are as follows
        /// 1. Name
        /// 2. Max size
        /// 3. Current EOF
        /// 4. Percent full
        /// </summary>
        /// <returns></returns>
        public DataTable getIndexesForTable(String aTableName)
        {
            return getIndexesForSchemaObject(aTableName, "table");
        }

        /// <summary>
        /// Returns the summary information for indexes of a table. The results are an
        /// aggregate of all the partitions of the indexes.
        /// The columns returned by the Datatable are as follows
        /// 1. Name
        /// 2. Max size
        /// 3. Current EOF
        /// 4. Percent full
        /// </summary>
        /// <returns></returns>
        public DataTable getIndexesForMV(String aMVName)
        {
            return getIndexesForSchemaObject(aMVName, "mv");
        }

        /// <summary>
        /// Returns the summary information for MVs in a schema. The results are an
        /// aggregate of all the partitions of the MV.
        /// The columns returned by the Datatable are as follows
        /// 1. Name
        /// 2. Last refreshed time
        /// 3. Max size
        /// 4. Current EOF
        /// 5. Percent full
        /// </summary>
        /// <returns></returns>
        public DataTable getMVsSummary()
        {
            Connection theConnection = null;
            OdbcDataReader theReader = null;
            DataTable dataTable = new DataTable();
            DataRow aRow = null;
            String objectName;
            long uid;
            String objectSecurityClass;
            long rowCount;
            decimal maxSize;
            decimal currentEof;
            long refreshTime;

            try
            {
                theConnection = GetConnection();
                TheQuery = Queries.ExecuteSelectMVDetailsSummary(theConnection, TheTrafodionSchema);
                theReader = Queries.ExecuteReader(TheQuery, false);

                // The modification is to remove the use of MaintainInfoView, which is still considered as
                // internal and may not be available on all customer's system. Leave the code here is to
                // allow easy restore once this view is made available for customer use.
                // 1/27/2009
                dataTable.Columns.Add(Properties.Resources.Name);
                //dataTable.Columns.Add(Properties.Resources.LastRefreshedTimestamp, typeof(TimestampObject));
                dataTable.Columns.Add(Properties.Resources.TotalCurrentRowCount, typeof(long));
                dataTable.Columns.Add(Properties.Resources.TotalMaximumSize, typeof(DecimalSizeObject));
                dataTable.Columns.Add(Properties.Resources.TotalCurrentSize, typeof(DecimalSizeObject));
                dataTable.Columns.Add(Properties.Resources.PercentAllocated, typeof(PercentObject));

                while (theReader.Read())
                {
                    //objectSecurityClass = getStringFromReader(theReader, 2);
                    //if (cannotDisplayMetadataTable(objectSecurityClass))
                    //{
                    //    continue;
                    //}

                    aRow = dataTable.NewRow();
                    //uid = getLongFromReader(theReader, 1);
                    objectName = getStringFromReader(theReader, 0);
                    rowCount = getLongFromReader(theReader, 1);
                    maxSize = getDecimalFromReader(theReader, 2);
                    currentEof = getDecimalFromReader(theReader, 3);
                    //refreshTime = getLongFromReader(theReader, 6);

                    aRow[0] = objectName;
                    //aRow[1] = new TimestampObject(refreshTime);
                    aRow[1] = rowCount;
                    aRow[2] = new DecimalSizeObject(maxSize);
                    aRow[3] = new DecimalSizeObject(currentEof);
                    aRow[4] = new PercentObject((double)currentEof * 100 / (double)maxSize);

                    dataTable.Rows.Add(aRow);
                }
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
            dataTable.AcceptChanges();
            return dataTable;
        }


        /// <summary>
        /// Returns the aggregate size of all Tables, Table Indexes, MVs and MV indexes 
        /// in a schema
        /// The columns returned by the Datatable are as follows
        /// 1. Object Type
        /// 2. Total size
        /// 
        /// The returned datatable will have 4 rows; One for each type.
        /// </summary>
        /// <returns></returns>
        public DataTable getObjectAggregate()
        {
            Connection theConnection = null;
            OdbcDataReader theReader = null;
            DataTable dataTable = new DataTable();

            dataTable.Columns.Add("Object Type", typeof(String) );
            dataTable.Columns.Add("Current Size", typeof( DecimalSizeObject));
            try
            {
                //For Table
                theConnection = GetConnection();
                TheQuery =  Queries.ExecuteSelectTableAggregate(theConnection, TheTrafodionSchema);
                theReader = Queries.ExecuteReader(TheQuery, false);
                if (theReader.Read())
                {
                    addRowToAggregateTable(theReader, "Tables", dataTable);
                }

                //For Indexes
                TheQuery = Queries.ExecuteSelectAllIndexAggregate(theConnection, TheTrafodionSchema);
                theReader = Queries.ExecuteReader(TheQuery, false);
                if (theReader.Read())
                {
                    addRowToAggregateTable(theReader, "Indexes", dataTable);
                }

                //For MVs
                TheQuery = Queries.ExecuteSelectMVAggregate(theConnection, TheTrafodionSchema);
                theReader = Queries.ExecuteReader(TheQuery, false);
                if (theReader.Read())
                {
                    addRowToAggregateTable(theReader, "Materialized Views", dataTable);
                }

            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
            dataTable.AcceptChanges();
            return dataTable;
        }

        #endregion

        #region Private methods
        /// <summary>
        /// Returns all indexes for the object type specified
        /// </summary>
        /// <returns></returns>
        private DataTable getIndexesForSchemaObject(String aSchemaObjectName, String aType)
        {
            Connection theConnection = null;
            OdbcDataReader theReader = null;
            DataTable dataTable = new DataTable();
            DataRow aRow = null;
            String objectName;
            long rowCount;
            decimal maxSize;
            decimal currentEof;

            dataTable.Columns.Add(Properties.Resources.Name);
            dataTable.Columns.Add(Properties.Resources.TotalCurrentRowCount, typeof(long));
            dataTable.Columns.Add(Properties.Resources.TotalMaximumSize, typeof(DecimalSizeObject));
            dataTable.Columns.Add(Properties.Resources.TotalCurrentSize, typeof(DecimalSizeObject));
            dataTable.Columns.Add(Properties.Resources.PercentAllocated, typeof(PercentObject));

            try
            {
                theConnection = GetConnection();
                TheQuery = Queries.ExecuteSelectIndexDetailsForSchemaObject(theConnection,
                    TheTrafodionCatalog.ExternalName,
                    TheTrafodionSchema.ExternalName,
                    aSchemaObjectName,
                    aType);
                theReader = Queries.ExecuteReader(TheQuery, false);
                while (theReader.Read())
                {
                    aRow = dataTable.NewRow();

                    objectName = (theReader[0] == null) ? "" : ((String)theReader[0]).TrimEnd();
                    rowCount = (long)theReader[1];
                    maxSize = getDecimalFromReader(theReader, 2);
                    currentEof = getDecimalFromReader(theReader, 3); 

                    aRow[0] = objectName;
                    aRow[1] = rowCount;
                    aRow[2] = new DecimalSizeObject(maxSize);
                    aRow[3] = new DecimalSizeObject(currentEof);
                    aRow[4] = new PercentObject((double)currentEof * 100 / (double)maxSize);
                    dataTable.Rows.Add(aRow);
                }
            }
            finally
            {
                if (theConnection != null)
                {
                    theConnection.Close();
                }
            }
            dataTable.AcceptChanges();
            return dataTable;
        }
        private bool cannotDisplayMetadataTable(String objectSecurityClass)
        {
            return false;
            //TODO:ROLEBasedDisplay
            //            return (!objectSecurityClass.Equals("UT"));
        }

        private long getLongFromReader(OdbcDataReader aReader, int index)
        {
            if (aReader[index] is DBNull)
            {
                return 0L;
            }
            return (long)aReader[index];
        }
        private decimal getDecimalFromReader(OdbcDataReader aReader, int index)
        {
            if (aReader[index] is DBNull)
            {
                return 0L;
            }
            return (decimal)aReader[index];
        }

        private string getStringFromReader(OdbcDataReader aReader, int index)
        {
            if (aReader[index] is DBNull)
            {
                return null;
            }
            return ((string)aReader[index]).TrimEnd();
        }

        private void addRowToAggregateTable(OdbcDataReader aReader, String aType, DataTable aDataTable)
        {
            decimal maxSize;
            decimal currentEof;
            DataRow aRow = null;

            aRow = aDataTable.NewRow();
            maxSize = 0;
            currentEof = 0;
            try
            {
                maxSize = getDecimalFromReader(aReader, 0);
                currentEof = getDecimalFromReader(aReader, 1);
            }
            catch (Exception ex)
            {
                //Empty value returned
            }
            aRow[0] = aType;
            aRow[1] = new DecimalSizeObject(currentEof);
            aDataTable.Rows.Add(aRow);
        }

        private bool isMaintainStatus(String maintainOperation, String maintainStatus, String statusToMatch)
        {
            bool ret = true;
            if ((maintainOperation != null) && (maintainOperation.Equals(statusToMatch)))
            {
                if (maintainStatus != null)
                {
                    return (maintainStatus.Equals(TrafodionTable.MaintainStatusDisabled) ? false : true);
                }
            }
            return ret;
        }

        /// <summary>
        /// Returns a connection
        /// </summary>
        /// <returns></returns>
        private Connection GetConnection()
        {
            return TheTrafodionSchema.GetConnection();
        }
        #endregion

    }

   /// <summary>
    /// This object shall be used to display the sizes/timestamps of the objects in a DataGridView
    /// This will allow the sort to happen correctly
    /// </summary>
    public class LongObject : IComparable
    {
        long _theSize;
        public LongObject(long aSize)
        {
            _theSize = aSize;
        }

        public long TheValue
        {
            get { return _theSize; }
            set { _theSize = value; }
        }

        public int CompareTo(Object obj)
        {
            if (obj is long)
            {
                return _theSize.CompareTo((long)obj);
            }
            else if (obj is SizeObject)
            {
                return _theSize.CompareTo(((LongObject)obj).TheValue);
            }
            return -1;
        }

        public override string ToString()
        {
            return Utilities.FormatSize(_theSize);
        }
    }

    /// <summary>
    /// This object shall be used to display the sizes of the objects in a DataGridView
    /// This will allow the sort to happen correctly
    /// </summary>
    public class SizeObject : LongObject
    {
        public SizeObject(long aSize):base(aSize)
        {
        }

        public override string ToString()
        {
            return Utilities.FormatSize(TheValue);
        }
    }

    public class DecimalSizeObject : IComparable
    {
        decimal _theSize;
        public DecimalSizeObject(decimal aSize)
        {
            _theSize = aSize;
        }

        public decimal TheValue
        {
            get { return _theSize; }
            set { _theSize = value; }
        }

        public int CompareTo(Object obj)
        {
            if (obj is decimal)
            {
                return _theSize.CompareTo((decimal)obj);
            }
            else if (obj is DecimalSizeObject)
            {
                return _theSize.CompareTo(((DecimalSizeObject)obj).TheValue);
            }
            return -1;
        }

        public override string ToString()
        {
            return Utilities.FormatSize(_theSize);
        }
    }
    /// <summary>
    /// This object shall be used to display the Percent column
    /// </summary>
    public class PercentObject : IComparable
    {
        double _thePercent;
        public PercentObject(double aPercent)
        {
            _thePercent = aPercent;
        }

        public double ThePercent
        {
            get { return _thePercent; }
            set { _thePercent = value; }
        }

        public int CompareTo(Object obj)
        {
            if (obj is double)
            {
                return _thePercent.CompareTo((double)obj);
            }
            else if (obj is PercentObject)
            {
                return _thePercent.CompareTo(((PercentObject)obj).ThePercent);
            }
            return -1;
        }

        public override string ToString()
        {
            return Utilities.FormatPercent(_thePercent);
        }
    }
}
