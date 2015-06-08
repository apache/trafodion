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
using Trafodion.Manager.Framework.Connections;

namespace Trafodion.Manager.DatabaseArea.Model
{
    /// <summary>
    /// Base class for a partitioned schema level object
    /// </summary>
    abstract public class PartitionedSchemaObject : TrafodionSchemaObject
    {
        // Member Variables
        List<Partition> _partitions = null;
        private bool _isIndex = false;

        /// <summary>
        /// Constructs a partitioned schema level object
        /// </summary>
        /// <param name="aTrafodionSchema">Parent schema</param>
        /// <param name="anInternalName">Internal name</param>
        /// <param name="aUID">UID</param>
        /// <param name="aCreateTime">Creation time</param>
        /// <param name="aRedefTime">Redefinition time</param>
        /// <param name="aSecurityClass">Security class</param>
        /// <param name="anOwner">Owner of the partitioned schema object</param>
        public PartitionedSchemaObject(TrafodionSchema aTrafodionSchema, string anInternalName, long aUID, long aCreateTime, long aRedefTime, string aSecurityClass, string anOwner)
            : base(aTrafodionSchema, anInternalName, aUID, aCreateTime, aRedefTime, aSecurityClass, anOwner)
        {
        }        

        /// <summary>
        /// The partition list of the object
        /// </summary>
        public List<Partition> Partitions
        {
            get
            {
                if (_partitions == null)
                {
                    LoadPartitions();
                }
                return _partitions;
            }
            set { _partitions = value; }
        }

        /// <summary>
        /// Whether or not this is an index object
        /// </summary>
        public bool IsIndex
        {
            get { return _isIndex; }
            set { _isIndex = value; }
        }


        /// <summary>
        /// Resets the PartitionedSchemaObject
        /// </summary>
        override public void Refresh()
        {
            base.Refresh();
            if (_partitions != null)
            {
                _partitions.Clear();
            }
            _partitions = null;
        }

        public void FetchPartitions()
        {
            if(_partitions == null)
            {
                LoadPartitions();
            }
        }

        private void LoadPartitions()
        {
            _partitions = new List<Partition>();

            Connection theConnection = null;

            try
            {
                theConnection = GetConnection();
                OdbcDataReader theReader = Queries.ExecuteSelectPartitionInfo(theConnection, GetPartitionsColumnNames(TheTrafodionSchema.Version), TheTrafodionCatalog, TheTrafodionSchema.Version, UID);
                int index = 0;
                while (theReader.Read())
                {
                    Partition part = new Partition(theReader);
                    part.ThePartitionNum = index;
                    _partitions.Add(part);
                    index++;
                }
                if (theConnection != null)
                {
                    theConnection.Close();
                }
                //We are updating the partitions object created earlier
                //with the details obtained from the DISK LABEL STATISTICS
                OdbcDataReader theDetailReader = Queries.ExecuteSelectPartitionDetailInfo(theConnection, GetPartitionDetailsColumnNames(TheTrafodionSchema.ConnectionDefinition), GetObjectFullName(), IsIndex);
                while (theDetailReader.Read())
                {
                    String partitionFileName = theDetailReader.GetString(3);
                    if (partitionFileName != null)
                    {
                        partitionFileName = partitionFileName.Trim(new char[] { '\0', ' ' });
                        Partition tempPartition = this.FindPartition(partitionFileName);
                        if (tempPartition != null)
                        {
                            tempPartition.PopulateFromDetailReader(this.ConnectionDefinition, theDetailReader);
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
        }

        private Partition FindPartition(string aPartitionFileName)
        {
            Partition theMatchedPartition = _partitions.Find(delegate(Partition aPartition)
            {
                String fullyQualifiedName = aPartition.TheSystemName + "." + aPartition.ThePartitionFileName;
                return fullyQualifiedName.Equals(aPartitionFileName);
            });

            return theMatchedPartition;
        }

        private string GetObjectFullName()
        {
            return TheTrafodionCatalog.ExternalName + "." + TheTrafodionSchema.ExternalName + "." + ExternalName;
        }
        /// <summary>
        /// Returns the column names that contain partitioning information in the metadata tables
        /// </summary>
        /// <param name="aTrafodionSchemaVersion">schema version</param>
        /// <returns></returns>
        static public String GetPartitionsColumnNames(int aTrafodionSchemaVersion)
        {
            return theV1200PartitionsColumnNames;
        }

        const string theV1200PartitionsColumnNames =
        "SYSTEM_NAME,"
        + " DATA_SOURCE,"
        + " FILE_SUFFIX,"
        + " FIRST_KEY,"
        + " (PRI_EXT + (SEC_EXT * (MAX_EXT -1))) * 2048,"
        + " PRI_EXT,"
        + " SEC_EXT,"
        + " MAX_EXT,"
        + " PARTITION_STATUS,"
        + " PARTITION_NAME";


      /// <summary>
        /// Returns the column names that contain partitioning information in the metadata tables
        /// </summary>
        /// <param name="aTrafodionSchemaVersion">schema version</param>
        /// <returns></returns>
        static public String GetPartitionDetailsColumnNames(ConnectionDefinition aConnectionDefinition)
        {
            if (aConnectionDefinition != null && aConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                return SQ120PartitionDetailsColumnNames;
            }
            else
            return SQ110PartitionDetailsColumnNames;
        }

        const string SQ110PartitionDetailsColumnNames =
        "CATALOG_NAME,"
        + "SCHEMA_NAME,"
        + "OBJECT_NAME,"
        + "PARTITION_NAME,"
        + "PARTITION_NUM,"
        + "ROW_COUNT,"
        + "INSERTED_ROW_COUNT,"
        + "DELETED_ROW_COUNT,"
        + "UPDATED_ROW_COUNT,"
        + "PRIMARY_EXTENTS,"
        + "SECONDARY_EXTENTS,"
        + "MAX_EXTENTS,"
        + "ALLOCATED_EXTENTS,"
        + "(PRIMARY_EXTENTS + (SECONDARY_EXTENTS * (MAX_EXTENTS -1))) * 2048,"
        + "CURRENT_EOF,"
        + "COMPRESSION_TYPE,"
        + "COMPRESSED_EOF_SECTORS,"
        + "COMPRESSION_RATIO";

        const string SQ120PartitionDetailsColumnNames =
        "CATALOG_NAME,"
        + "SCHEMA_NAME,"
        + "OBJECT_NAME,"
        + "PARTITION_NAME,"
        + "PARTITION_NUM,"
        + "ROW_COUNT,"
        + "INSERTED_ROW_COUNT,"
        + "DELETED_ROW_COUNT,"
        + "UPDATED_ROW_COUNT,"
        + "PRIMARY_EXTENTS,"
        + "SECONDARY_EXTENTS,"
        + "MAX_EXTENTS,"
        + "ALLOCATED_EXTENTS,"
        + "(PRIMARY_EXTENTS + (SECONDARY_EXTENTS * (MAX_EXTENTS -1))) * 2048,"
        + "CURRENT_EOF,"
        + "COMPRESSION_TYPE,"
        + "COMPRESSED_EOF_SECTORS,"
        + "COMPRESSION_RATIO,"
        + "RFORK_EOF,"
        + "ACCESS_COUNTER"
        ;
    }

    public class Partition
    {

        public Partition(OdbcDataReader aReader)
        {
            // Get the system name
            TheSystemName = aReader.GetString(0).Trim();

            // Get the volume name
            TheVolumeName = aReader.GetString(1).Trim();

            // Get the subvol and the file
            {
                String[] theSubvolAndFile = aReader.GetString(2).Trim().Split(new char[] { '.' });

                TheSubvolName = theSubvolAndFile[0].Trim();
                TheFilename = theSubvolAndFile[1].Trim();
            }

            // Get the first key
            TheFirstKey = aReader.GetString(3).Trim();

            // Get the extent info
            ThePrimaryExtentSize = aReader.GetInt16(5);
            TheSecondaryExtentSize = aReader.GetInt16(6);
            TheMaxExtents = aReader.GetInt16(7);

            // Get the status
            TheStatus = aReader.GetString(8).Trim();

            // Get the partition name
            TheName = aReader.GetString(9).Trim();

        }

        public void PopulateFromDetailReader(ConnectionDefinition aConnectionDefinition, OdbcDataReader aReader)
        {
            ThePartitionNum = aReader.GetInt32(4);
            TheRowCount = aReader.GetInt64(5);
            TheInsertedRowCount = aReader.GetInt64(6);
            TheDeletedRowCount = aReader.GetInt64(7);
            TheUpdatedRowCount = aReader.GetInt64(8);
            ThePrimaryExtentSize = aReader.GetInt32(9);
            TheSecondaryExtentSize = aReader.GetInt32(10);
            TheMaxExtents = aReader.GetInt32(11);

            TheAllocatedExtents = aReader.GetInt32(12);
            TheMaxSize = aReader.GetInt64(13);
            TheCurrentEOF = aReader.GetInt64(14);
            ThePercentAllocated = (TheMaxSize > 0) ? ((double)TheCurrentEOF * 100 / (double)TheMaxSize) : 0;
            _theCompressionType = aReader.GetInt32(15);
            _theCompressedEOF = aReader.GetInt64(16) * 512; //compressed EOF is returned as # of sectors. we need to multiply by 512 to convert to bytes.
            _theCompressionRatio = aReader.GetDouble(17);
            if (aConnectionDefinition != null && aConnectionDefinition.ServerVersion >= ConnectionDefinition.SERVER_VERSION.SQ120)
            {
                _theInternalOverhead = aReader.GetDecimal(18);
                _theAccessCount = aReader.GetInt64(19);
            }
        }

        private string theSystemName;

        public string TheSystemName
        {
            get { return theSystemName; }
            set { theSystemName = value; }
        }

        private string theVolumeName;

        public string TheVolumeName
        {
            get { return theVolumeName; }
            set { theVolumeName = value; }
        }

        private string theSubvolName;

        public string TheSubvolName
        {
            get { return theSubvolName; }
            set { theSubvolName = value; }
        }

        private string theFilename;

        public string TheFilename
        {
            get { return theFilename; }
            set { theFilename = value; }
        }

        private string theFirstKey;

        public string TheFirstKey
        {
            get { return theFirstKey; }
            set { theFirstKey = value; }
        }

        private int thePrimaryExtentSize;

        public int ThePrimaryExtentSize
        {
            get { return thePrimaryExtentSize; }
            set { thePrimaryExtentSize = value; }
        }

        private int theSecondaryExtentSize;

        public int TheSecondaryExtentSize
        {
            get { return theSecondaryExtentSize; }
            set { theSecondaryExtentSize = value; }
        }

        private int theMaxExtents;

        public int TheMaxExtents
        {
            get { return theMaxExtents; }
            set { theMaxExtents = value; }
        }

        private string theStatus;

        public string TheStatus
        {
            get { return theStatus; }
            set { theStatus = value; }
        }

        private string theName;

        public string TheName
        {
            get { return theName; }
            set { theName = value; }
        }

        public string ThePartitionFileName
        {
            get { return TheVolumeName + "." + TheSubvolName + "." + TheFilename; }
        }

        private int _thePartitionNum;

        public int ThePartitionNum
        {
            get { return _thePartitionNum; }
            set { _thePartitionNum = value; }
        }

        private long _theRowCount;

        public long TheRowCount
        {
            get { return _theRowCount; }
            set { _theRowCount = value; }
        }
        private long _theInsertedRowCount;

        public long TheInsertedRowCount
        {
            get { return _theInsertedRowCount; }
            set { _theInsertedRowCount = value; }
        }
        private long _theDeletedRowCount;

        public long TheDeletedRowCount
        {
            get { return _theDeletedRowCount; }
            set { _theDeletedRowCount = value; }
        }
        private long _theUpdatedRowCount;

        public long TheUpdatedRowCount
        {
            get { return _theUpdatedRowCount; }
            set { _theUpdatedRowCount = value; }
        }

        private int _theAllocatedExtents;

        public int TheAllocatedExtents
        {
            get { return _theAllocatedExtents; }
            set { _theAllocatedExtents = value; }
        }

        private long _theMaxSize;

        public long TheMaxSize
        {
            get { return _theMaxSize; }
            set { _theMaxSize = value; }
        }

        private long _theCurrentEOF;

        public long TheCurrentEOF
        {
            get { return _theCurrentEOF; }
            set { _theCurrentEOF = value; }
        }

        private double _thePercentAllocated;

        public double ThePercentAllocated
        {
            get { return _thePercentAllocated; }
            set { _thePercentAllocated = value; }
        }

        private long _theCompressedEOF;

        public long TheCompressedEOF
        {
            get { return _theCompressedEOF; }
        }

        private int _theCompressionType;

        public int TheCompressionType
        {
            get { return _theCompressionType; }
        }

        public string FormattedCompressionType
        {
            get
            {
                switch (_theCompressionType)
                {
                    case 1: return "Software";
                    case 2: return "Hardware";
                    case 0:
                    default: return "None";
                }
            }
        }

        private double _theCompressionRatio;

        public double TheCompressionRatio
        {
            get { return _theCompressionRatio; }
        }

        private decimal _theInternalOverhead;
        public decimal TheInternalOverhead
        {
            get { return _theInternalOverhead; }
        }

        private Int64 _theAccessCount;
        public Int64 TheAccessCount
        {
            get { return _theAccessCount; }
        }
    }

}
